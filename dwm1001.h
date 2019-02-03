
#include <Arduino.h>
#include <SPI.h>
#include <EEPROM.h>

#ifndef DWM1001_H
#define DWM1001_H

class DWM{
    private:
        byte DATA_READY_ENABLED = 1;
        volatile byte DATA_READY_FLAG = 0;
        // SPI Processing
        void SPIWrite(uint8_t[]);
        void SPIWaitForResponse();
        void SPIGetReturnValue();
    public:
        const int SPI_RATE = 1000000;
        SPISettings SPI_SETTINGS;
        uint32_t rxsize;
        uint8_t response[256];
        // ---------- Constructors & Destructors ---------- 
        DWM();
        ~DWM();
        // ---------- SPI Transaction Utils ---------- 
        byte ProcessReturnType( uint8_t *);
        void DataReady(); // ISR for data ready
        // ---------- DWM General API Call ---------- 
        uint8_t* TLVcmd(uint8_t []);
        // ---------- DWM API List ---------- 
        uint8_t* DWM_POS_SET(uint8_t []);
        uint8_t* DWM_POS_GET();
        uint8_t* DWM_UPD_RATE_SET(uint8_t []);
        uint8_t* DWM_UPD_RATE_GET();
        uint8_t* DWM_CFG_TAG_SET(uint8_t []);
        uint8_t* DWM_CFG_ANCHOR_SET(uint8_t);
        uint8_t* DWM_CFG_GET();
        uint8_t* DWM_SLEEP();
        uint8_t* DWM_LOC_GET();
        uint8_t* DWM_BADDR_SET(uint8_t []);
        uint8_t* DWM_BADDR_GET();
        uint8_t* DWM_RESET();
        uint8_t* DWM_VER_GET();
        uint8_t* DWM_GPIO_CFG_OUTPUT(uint8_t []);
        uint8_t* DWM_GPIO_CFG_INPUT(uint8_t []);
        uint8_t* DWM_GPIO_VALUE_SET(uint8_t []);
        uint8_t* DWM_GPIO_VALUE_GET(uint8_t);
        uint8_t* DWM_GPIO_VALUE_TOGGLE(uint8_t);
        uint8_t* DWM_STATUS_GET();
        uint8_t* DWM_INT_CFG_GET();
        uint8_t* DWM_INT_CFG_SET(uint8_t val); // Enables Data Ready Feature
};

#endif