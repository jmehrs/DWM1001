#include "dwm1001.h"

byte DATA_READY_ENABLED = 1;
volatile byte DATA_READY_FLAG = 0;

const int SPI_RATE = 1000000;
SPISettings SPI_SETTINGS(SPI_RATE, MSBFIRST, SPI_MODE0);
uint32_t rxsize;
uint8_t response[256];

void DWM(){
    pinMode(SS, OUTPUT);
    SPI.begin();    
}

// SPI Processing
void SPIWrite(uint8_t tlv[]=0){
    uint8_t len = tlv[1] + 2;
    digitalWrite(SS, LOW);
    for(int i = 0; i < len; i++){tlv[i] = SPI.transfer(tlv[i]);}
    digitalWrite(SS, HIGH);
}

void SPIWaitForResponse(){
    rxsize = 0x00;
    if(DATA_READY_ENABLED == 1){
        DATA_READY_FLAG = 0;
        while(!DATA_READY_FLAG);
        DATA_READY_FLAG = 0;
        digitalWrite(SS, LOW);
        rxsize = SPI.transfer(0xFF);
        digitalWrite(SS, HIGH);
    }
    else{
      while (rxsize == 0x00) {
        digitalWrite(SS, LOW);
        rxsize = SPI.transfer(0xFF);
        digitalWrite(SS, HIGH);
        delayMicroseconds(100);
        }
    }
}

void SPIGetReturnValue(){
    response[0] = rxsize;
    if(DATA_READY_ENABLED == 1){
        while(!DATA_READY_FLAG);
        digitalWrite(SS, LOW);
        for (byte i = 0; i < rxsize; i++){response[i+1] = SPI.transfer(0xFF);}
        digitalWrite(SS, HIGH);
    }
    else{
        digitalWrite(SS, LOW);
        for (byte i = 0; i < rxsize; i++){response[i+1] = SPI.transfer(0xFF);}
        digitalWrite(SS, HIGH);
    }
}

// SPI Transaction Utils
byte ProcessReturnType(uint8_t * response){
    uint8_t resp_len = response[0];
    for(byte i=1; i<=resp_len; i++){
        

    }
    return 1;
}

void DataReady(){
    DATA_READY_FLAG = 1;
}

// DWM Genereal API Call
uint8_t* TLVcmd(uint8_t tlv[]){  
    SPI.beginTransaction(SPI_SETTINGS);
    SPIWrite(tlv);
    SPIWaitForResponse();
    SPIGetReturnValue();
    SPI.endTransaction();
    return response;
}



// DWM API List
// - ALL byte arrays are in little endian (i.e. in a 32-bit value -> byte[0]=LSB and byte[3]=MSB)
// - ALL arguments to an API call are uint8_t arrays or values
// - IN this class, all API call responses' first byte is the total size of the array (not including the first byte)
// - Google 'dwm1001 firmware api guide' for more info
uint8_t* DWM_POS_SET(uint8_t val[]){
    /*
    WRITES default position of the node (used in anchor mode but stored regardless).
    PARAMS: val[] is a 13-byte(32-bit, 32-bit, 32-bit, 8-bit) (uint8_t) array where:
                - bytes[0:3] = x-coordinate [mm]
                - bytes[4:7] = y-coordinate [mm]
                - bytes[8:11] = z-coordinate [mm]
                - byte[12] = quality factor in percent (0-100)
    RETURN: Error code response({0x40, 0x01, 0x00} if no error)
    */
    uint8_t tlv[] = {0x01, 0x0D, val[0], val[1], val[2], val[3],
                    val[4], val[5], val[6], val[7], val[8], val[9],
                    val[10], val[11], val[12]};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_POS_GET(){
    /*
    READS default position of the node (readable in anchor mode but stored regardless).
    PARAMS: N/A
    RETURN: Important part of response is a total of 14-bytes(8-bit, 32-bit, 32-bit, 32-bit, 8-bit) (uint8_t) where:
                - byte 0 = total length of response (in this case 18, not including the first byte)
                - bytes[6:9] = x-coordinate [mm]
                - bytes[10:13] = y-coordinate [mm]
                - bytes[14:17] = z-coordinate [mm]
                - byte[18] = quality factor in percent (0-100) 
    */
    uint8_t tlv[] = {0x02, 0x00};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_UPD_RATE_SET(uint8_t val[]){
    /*
    WRITES mobile update rate and stationary update rate in units of milliseconds.
    Stationary update rate must be >= mobile update rate.
    PARAMS: val[] is a 4-byte(16-bit, 16-bit) (uint8_t) array where:
                - bytes[0:1] = mobile update rate [ms]
                - bytes[2:3] = stationary update rate [ms]
    RETURN: Error code response({0x40, 0x01, 0x00} if no error)
    */
    uint8_t tlv[] = {0x03, 0x04, val[0], val[1], val[2], val[3]};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_UPD_RATE_GET(){
    /*
    READS mobile update rate and stationary update rate in units of milliseconds.
    PARAMS: N/A
    RETURN: important part of response is a total of 5-bytes(8-bit, 16-bit, 16-bit) (uint8_t) where:
                - byte 0 = total length of response (in this case 9, not including the first byte)
                - bytes[6:7] = mobile update rate [ms]
                - bytes[8:9] = stationary update rate [ms]
    */
    uint8_t tlv[] = {0x04, 0x00};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_CFG_TAG_SET(uint8_t val[]){
    /*
    WRITES tag configuration.
    CONFIGURES the node as a tag, user is advised to RESET the node to activate settings.
    PARAMS: val[] is a 2-byte(16-bit) (uint8_t) array where:
                - bytes[0:1] = tag configuration
                    |---(* BYTE 1 *)
                    |   (bits 3-7) reserved
                    |   (bit 2) accel_en
                    |   (bits 0-1) meas_mode : 0 - TWR, 1-3 reserved
                    |---(* BYTE 0 *)
                    |   (bit 7) low_power_en
                    |   (bit 6) loc_engine_en
                    |   (bit 5) reserved
                    |   (bit 4) led_en
                    |   (bit 3) ble_en
                    |   (bit 2) fw_update_en
                    |   (bits 0-1) uwb_mode : 0=off, 1=passive, 2=active
    RETURN: Error code response({0x40, 0x01, 0x00} if no error)
    */
    uint8_t tlv[] = {0x05, 0x02, val[0], val[1]};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_CFG_ANCHOR_SET(uint8_t val){
    /*
    WRITES anchor configuration.
    CONFIGURES the node as an anchor, user is advised to RESET the node to activate settings.
    PARAMS: val is a 1-byte (uint8_t) value where:
                - byte 0 = tag configuration
                    |---(* BYTE 0 *)
                    |   (bit 7) initiator
                    |   (bit 6) bridge
                    |   (bit 5) reserved
                    |   (bit 4) led_en
                    |   (bit 3) ble_en
                    |   (bit 2) fw_update_en
                    |   (bits 0-1) uwb_mode : 0=off, 1=passive, 2=active
    RETURN: Error code response({0x40, 0x01, 0x00} if no error)
    */
    uint8_t tlv[] = {0x07, 0x01, val};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_CFG_GET(){
    /*
    READS configuration of the node.
    PARAMS: N/A
    RETURN: important part of response is a total of 3-bytes(8-bit, 16-bit) (uint8_t)where:
                - byte 0 = total length of response (in this case 7, not including the first byte)
                - byte 6 = node configuration LSB of 16-bit
                    |---(* BYTE 0 *)
                    |   (bit 7) low_power_en
                    |   (bit 6) loc_engine_en
                    |   (bit 5) reserved
                    |   (bit 4) led_en
                    |   (bit 3) ble_en
                    |   (bit 2) fw_update_en
                    |   (bits 0-1) uwb_mode : 0=off, 1=passive, 2=active
                - byte 7 = tag configuration MSB of 16-bit
                    |---(* BYTE 1 *)
                    |   (bit 7) reserved
                    |   (bit 6) reserved
                    |   (bit 5) mode : 0=tag, 1=anchor
                    |   (bit 4) initiator
                    |   (bit 3) bridge
                    |   (bit 2) accel_en
                    |   (bits 0-1) meas_mode : 0=Two Way Ranging (TWR), 1:3=N/A
    */
    uint8_t tlv[] = {0x08, 0x00};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_SLEEP(){
    /*
    CONFIGURES module into sleep mode, the module must have low power enabled otherwise there is an error.
    RETURN: Error code response({0x40, 0x01, 0x00} if no error)
    */
    uint8_t tlv[] = {0x0a, 0x00};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_LOC_GET(){
    /*
    GET last distance to anchors(tag is currently ranging to) and their set positions.
    If location engine is enabled then the position of the tag is given.
    PARAMS: N/A
    RETURN: important part of response is a total minimum of 41-bytes or 35-bytes depending on if the node is an anchor or a tag 
            See firmware API guide for more info, there is too much to accurately convey here
    */
    uint8_t tlv[] = {0x0c, 0x00};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_BADDR_SET(uint8_t val[]){
    /*
    WRITES the new public bluetooth address used by the device.
    The user is advised to reset the device after this command to make use of the changes.
    PARAMS: val[] is a 6-byte (uint8_t) array where:
                - bytes[3:8] = public bluetooth address
    RETURN: Error code response({0x40, 0x01, 0x00} if no error)
    */
    uint8_t tlv[] = {0x0f, 0x06, val[0], val[1], val[2], val[3], val[4], val[5]};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_BADDR_GET(){
    /*
    READS The bluetooth address currently being used by the device.
    PARAMS: N/A
    RETURN: important part of response is a total of 7-bytes(8-bit, 6-bytes) (uint8_t) where:
                - byte 0 = total length of response (in this case 11, not including the first byte)
                - bytes[6:11] = mobile update rate [ms]
    */
    uint8_t tlv[] = {0x10, 0x00};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_RESET(){
    /*
    REBOOTS the module.
    RETURN: Error code response({0x40, 0x01, 0x00} if no error)
    */
    uint8_t tlv[] = {0x14, 0x00};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_VER_GET(){
    /*
    READS the firmware version, configuration version, and hardware version of the module
    RETURN: important part of response is a total of 7-bytes(8-bit, 4 bytes, 4 bytes, 4 bytes) (uint8_t) where:
                - byte 0 = total length of response (in this case 11, not including the first byte)
                - bytes[6:9] = firmware version
                - bytes[10:13] = configuration version
                - bytes[14:17] = hardware version
    */
    uint8_t tlv[] = {0x15, 0x00};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_GPIO_CFG_OUTPUT(uint8_t val[]){
    /*
    CONFIGURES the selected gpio pin as an output (digital high or digital low).
    NOTE that gpios 22, 30, & 31 are utilized internally during the first few seconds of reboot.
    The user is advised to reset the device after this command to make use of the changes.
    PARAMS: val[] is a 2-byte (uint8_t, uint8_t) array where:
                - byte 2 = gpio pin/index
                - byte 3 = gpio value (0 or 1)
    RETURN: Error code response({0x40, 0x01, 0x00} if no error)
    */
    uint8_t tlv[] = {0x28, 0x02, val[0], val[1]};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_GPIO_CFG_INPUT(uint8_t val[]){
    /*
    CONFIGURES the selected gpio pin as an input (no pull, pull-up, or pull-down).
    NOTE that gpios 22, 30, & 31 are utilized internally during the first few seconds of reboot.
    The user is advised to reset the device after this command to make use of the changes.
    PARAMS: val[] is a 2-byte (uint8_t, uint8_t) array where:
                - byte 2 = gpio pin/index
                - byte 3 = gpio value (0 = no pull, 1 = pull-down, 3 = pull-up)
    RETURN: Error code response({0x40, 0x01, 0x00} if no error)
    */
    uint8_t tlv[] = {0x29, 0x02, val[0], val[1]};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_GPIO_VALUE_SET(uint8_t val[]){
    /*
    CONFIGURES the selected gpio pin output to high or low (pin should already be output, this API call just changes output value).
    PARAMS: val[] is a 2-byte (uint8_t, uint8_t) array where:
                - byte 2 = gpio pin/index
                - byte 3 = gpio value (0 or 1)
    RETURN: Error code response({0x40, 0x01, 0x00} if no error)
    */
    uint8_t tlv[] = {0x2a, 0x02, val[0], val[1]};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_GPIO_VALUE_GET(uint8_t val){
    /*
    READS the selected gpio pin value.
    PARAMS: val is a 1-byte (uint8_t) value where:
                - byte 2 = gpio pin/index
    RETURN: important part of response is a total of 2-bytes (uint8_t, uint8_t) where:
                - byte 0 = total length of response (in this case 11, not including the first byte)
                - byte 6 = gpio pin value (0 or 1)
    */
    uint8_t tlv[] = {0x2b, 0x01, val};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_GPIO_VALUE_TOGGLE(uint8_t val){
    /*
    TOGGLES the selected gpio pin output.
    PARAMS: val is a 1-byte (uint8_t) value where:
                - byte 2 = gpio pin/index
    RETURN: Error code response({0x40, 0x01, 0x00} if no error)
    */
    uint8_t tlv[] = {0x2c, 0x02, val};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_STATUS_GET(){/*
    READS the system status: Location Data Ready & Connected to UWB network
    RETURN: important part of response is a total of 2-bytes (uint8_t, uint8_t) where:
                - byte 0 = total length of response (in this case 11, not including the first byte)
                - byte 6 = System status
                    |---(* BYTE 0 *)
                    |   (bit 7) reserved
                    |   (bit 6) reserved
                    |   (bit 5) reserved
                    |   (bit 4) reserved
                    |   (bit 3) reserved
                    |   (bit 2) reserved
                    |   (bit 1) uwbmac_joined : 0 = not connected, 1 = connected
                    |   (bit 0) loc_ready : 0 = no new data, 1 = new location data ready
    */
    uint8_t tlv[] = {0x32, 0x00};
    TLVcmd(tlv);
    return response;
}
uint8_t* DWM_INT_CFG_GET(){
    /*
    READS interrupt status for spi_data_ready and/or loc_ready?
    RETURN: important part of response is a total of 2-bytes (uint8_t, uint8_t) where:
                - byte 0 = total length of response (in this case 11, not including the first byte)
                - byte 3 = gpio pin value (0 or 1)
                    |---(* BYTE 0 *)
                        |   (bit 1) loc_ready
                        |   (bit 0) spi_data_ready
    */
    uint8_t tlv[] = {0x34, 0x00};
    TLVcmd(tlv);
    return response;
} 
uint8_t* DWM_INT_CFG_SET(uint8_t val){ // Enables Data Ready Feature
    /*
    CONFIGURES interrupt for spi_data_ready and/or loc_ready.
    PARAMS: val is a 1-byte (uint8_t) value where:
                - byte 2 = interrupt status
                    |---(* BYTE 0 *)
                    |   (bit 1) loc_ready
                    |   (bit 0) spi_data_ready
    RETURN: Error code response({0x40, 0x01, 0x00} if no error)
    */
    uint8_t tlv[] = {0x34, 0x01, val};
    TLVcmd(tlv);
    return response;
} 
