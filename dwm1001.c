#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"

#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

#define PIN_NUM_INT1 21
#define PIN_NUM_INT2 18
#define PIN_NUM_DC 17


//To speed up transfers, every SPI transfer sends a bunch of lines. This define specifies how many. More means more memory use,
//but less overhead for setting up / finishing transfers. Make sure 240 is dividable by this.
// #define PARALLEL_LINES 16

/*
 Struct that contains all the state information of the operational DWM unit.
*/
typedef struct {
    uint8_t node_cfg[2]; //Change this into a bunch of bits after testing
    uint8_t fw_ver[4]; 
    uint8_t cfg_ver[4]; 
    uint8_t hw_ver[4]; 
	bool network_detected;
	bool int_en;
} dwm_sys_info;

/*
 The DWM needs a specific format for a message, in the form of type, length, & value.
 - type: specifies the specific API call to the DWM
 - length: specifies the length of the to-be transmitted values (if not applicable, then 0)
 - value: an array that holds teh values to be written (if not applicable then {0})
*/
typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t value[16]; 
} dwm_write_tlv, *Pdwm_write_tlv;

/*
 The DWM responds with a message, in the form of type, length, & value. However, depending on the API call
 the response may consist of multiple tlv structures, therefore we store the response in an array. This response 
 struct will be fed into a post-processing function to get usable data.
 Rxsize is the length of the response.
*/
typedef struct {
    uint8_t rxsize;
    uint8_t response[256];
} dwm_response;
// The above might need to be replaced with a struct that contains sub-structs of each message kind

/*
 The following section of 'response tlv's contains structs that are in the form of response tlvs. These are returned after
 calling one of the API functions in the DWM. Every response is composed of at least one type 0x40 response
 which tells you if there has been an error.
*/
/* Error response tlv */
typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t value; 
} dwm_resp_tlv_err;
/* Position response tlv */
typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t value[13]; 
} dwm_resp_tlv_pos;
/* Update rate response tlv */
typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t value[4]; 
} dwm_resp_tlv_upd;
/* Node configuration response tlv */
typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t value[2]; 
} dwm_resp_tlv_cfg;
/* Tag distance response tlv */
typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t value[81]; 
} dwm_resp_tlv_tdist;
/* Anchor distance response tlv */
typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t value[207]; 
} dwm_resp_tlv_adist;
/* Bluetooth address response tlv */
typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t value[6]; 
} dwm_resp_tlv_baddr;
/* Firmware version response tlv */
typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t value[4]; 
} dwm_resp_tlv_fwver;
/* Configuration version response tlv */
typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t value[4]; 
} dwm_resp_tlv_cfgver;
/* Hardware version response tlv */
typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t value[4]; 
} dwm_resp_tlv_hwver;
/* GPIO value response tlv */
typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t value; 
} dwm_resp_tlv_gpioval;
/* Status response tlv */
typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t value; 
} dwm_resp_tlv_status;
/* Interrupt configuration response tlv */
typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t value; 
} dwm_resp_tlv_intcfg;



//Place data into DRAM. For hardware level speed. DWM Initialization Procedure
DRAM_ATTR static const dwm_write_tlv dwm_init_tlv[]={
    /* dwm_cfg_get, gets the node configuration */
    {0x08, 0x00, {0}},
    /* dwm_ver_get, gets the firmware version, configuration version, & hardware version of node */
    /* dwm_int_cfg, check if interrupts are enabled */
    /* Termination struct, signifies end of array */
    {0xFF, 0x00, {0x00}}
};

/* Send tlv command to the DWM. Uses spi_device_polling_transmit, which waits until the
 * transfer is complete.
 */
void dwm_tlv(spi_device_handle_t spi, const uint8_t *value, int len)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=len*8;                 //Len is in bytes, transaction length is in bits.
    t.tx_buffer=value;              //Data
    
    ret=spi_device_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

//Initialize the dwm
void dwm_init(spi_device_handle_t spi)
{
    int cmd=0;
    const dwm_write_tlv* dwm_init_cmds;
    dwm_init_cmds = dwm_init_tlv;

    //Initialize non-SPI GPIOs
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);

    //Send all the commands
    while (dwm_init_cmds[cmd].type!=0xff) {
        dwm_tlv(spi, dwm_init_cmds[cmd].value, dwm_init_cmds[cmd].length);
        cmd++;
    }
}

void spi_write(spi_device_handle_t spi, dwm_write_tlv *tlv){
    uint8_t write_len=2+tlv->length; //Capture transaction array length (accounting for type & length)
    uint8_t* trans_buff = (uint8_t*)tlv;

    dwm_write_tlv response_tlv;
    uint8_t* resp_buff = (uint8_t*)&response_tlv;
    
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=8*write_len;                   //Len is in bytes, transaction length is in bits.
    t.rxlength = 8*write_len;
    t.tx_buffer=trans_buff;              //Data
    t.rx_buffer=resp_buff;
    
    ret=spi_device_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);      
}

void spi_wait(spi_device_handle_t spi, dwm_response *tlv){
    uint8_t txdummy = 0xFF;
    uint8_t rxsize = 0x00;
    
    while(rxsize == 0x00){
        esp_err_t ret;
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));       //Zero out the transaction
        t.length=8;                   //Len is in bytes, transaction length is in bits.
        t.rxlength = 8;
        t.tx_buffer= &txdummy;              //Data
        t.rx_buffer= &rxsize;
        
        vTaskDelay(30/ portTICK_RATE_MS);

        ret=spi_device_transmit(spi, &t);  //Transmit!
        assert(ret==ESP_OK);
    }
    tlv->rxsize = rxsize;          
    printf("%02X", rxsize);
}

void spi_receive(spi_device_handle_t spi, dwm_response *tlv){
    uint8_t rxsize = 7; // uint8_t rxsize = tlv->rxsize;
    uint8_t txdummy[rxsize]; // Create dummy array
    memset(txdummy, 0xFF, rxsize); // Initialize dummy transaction with 0xFFs

    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=8*rxsize;                   //Len is in bytes, transaction length is in bits.
    t.rxlength = 8*rxsize;
    t.tx_buffer= &txdummy;              //Data
    t.rx_buffer= tlv->response;

    vTaskDelay(5/ portTICK_RATE_MS);

    ret=spi_device_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);
}

void app_main()
{
    // uint8_t test_buff = {0x08,0x00,0x00};
    // Pdwm_write_tlv test_tlv = (Pdwm_write_tlv)test_buff;
    esp_err_t ret;
    spi_device_handle_t spi;
    spi_bus_config_t buscfg={
        .miso_io_num=PIN_NUM_MISO,
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1
    };
    spi_device_interface_config_t devcfg={
        .clock_speed_hz=1*1000*1000,            //Clock out at 800 KHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=PIN_NUM_CS,               //CS pin
        .queue_size=7                          //We want to be able to queue 7 transactions at a time
    };
    //Initialize the SPI bus
    ret=spi_bus_initialize(VSPI_HOST, &buscfg, 1);
    ESP_ERROR_CHECK(ret);
    printf("Bus Initialized-------------------------------");
    //Attach the DWM to the SPI bus
    ret=spi_bus_add_device(VSPI_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
    printf("Device Added \nSTART-------------------------------");
    while(1){
        dwm_write_tlv test_tlv = {0x08, 0x00, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};
        dwm_response response;
        vTaskDelay(2500/ portTICK_RATE_MS);

        spi_write(spi, &test_tlv);

        spi_wait(spi, &response);

        spi_receive(spi, &response);
    }
}