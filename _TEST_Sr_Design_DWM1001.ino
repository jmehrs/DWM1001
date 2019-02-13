#include <SPI.h>
#include <dwm1001.h>
#define INT_PIN 2

uint8_t* arr1;

void setup() {
  pinMode(INT_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(INT_PIN), spiISR, RISING);
  DWM();
}

void loop() {
  delay(1000);
  Serial.println("");
//  uint8_t tlv[] = {0x01, 0x00, 0x05, 0x00};
  
  unsigned long strt = micros();
  arr1 = DWM_CFG_GET(); 
  unsigned long delta= micros() - strt;
  
  for(int i = 0; i < arr1[0]; i++){
    Serial.print(arr1[i+1], HEX);
    Serial.print(", ");
  }
  Serial.println();
  Serial.print(delta);
  Serial.println(" micro-seconds to complete API call");
}

void spiISR(){
  // Still goes when DATA_READY_ENEABLED is 0, find a way to incorporate this into the class.
  DataReady();
}

//-------------------------------------------------------------------------------------------------
//---------------------------------          OLD CODE           -----------------------------------
//-------------------------------------------------------------------------------------------------

//#include <SPI.h>
//#include <dwm1001.h>
//#define SPI_RATE 1000000

//DWM dwm1;

//#pragma pack(push,1)
//struct tlv{
//  uint8_t type = 0x08;
//  uint8_t len = 0x00;
//  uint8_t *value;
//  uint8_t _rxsize = len;
//};
//#pragma pack(pop)
//
//void tlv_cmd(uint32_t &, uint8_t []);
//void SPIWrite(tlv &tlv);
//void SPIWaitForResponse(tlv &tlv);
//void SPIGetReturnValue(tlv &tlv);
//void printTLV(tlv &tlv);
//
//SPISettings SPI_SETTINGS(SPI_RATE, MSBFIRST, SPI_MODE0);
//
//void setup() {
//  pinMode(SS, OUTPUT);
//  SPI.begin();
//  Serial.println("start");
//}
//
//void loop() {
//  delay(5000);
////  
//  uint8_t vals[1] = {0x00};
//  tlv msg;
//  Serial.println("start");
////
//  SPIWrite(msg);
//  Serial.print(msg.type, HEX);
//  Serial.print(", ");
//  Serial.print(msg.len, HEX);
//  Serial.println(", ");
//  
//  SPIWaitForResponse(msg);
//  Serial.print(msg.type, HEX);
//  Serial.print(", ");
//  Serial.print(msg.len, HEX);
//  Serial.println(", ");
//
//  SPIGetReturnValue(msg);
//  Serial.print(msg.type, HEX);
//  Serial.print(", ");
//  Serial.print(msg.len, HEX);
//  Serial.print(", ");
//  Serial.print(msg._rxsize, HEX);
//  Serial.println(", ");
//  
////  if(msg._rxsize){
////    for(uint8_t i = 0; i < (msg._rxsize-2); i++){
////      Serial.print( msg.value[i], HEX);
////      if(1 < (msg.len-1))
////        Serial.print(", ");
////    } 
////  }
//  Serial.println("");
//  
//  delay(10000);
//}
//
//void SPIWrite(tlv &tlv){
//  bool write = (tlv.len>0);
//
//  digitalWrite(SS, LOW);
//  tlv.type = SPI.transfer(tlv.type);
//  tlv.len = SPI.transfer(tlv.len);
//  if(write){
//    for(int i = 0; i < tlv.len; i++){
//       tlv.value[i] = SPI.transfer(tlv.value[i]);
//    }
//  }
//  digitalWrite(SS, HIGH);
//}
//
//void SPIWaitForResponse(tlv &tlv){
//  uint8_t rxsize = 0x00;
//  while (rxsize == 0x00) {
//    digitalWrite(SS, LOW);
//    rxsize = SPI.transfer(0xFF);
//    digitalWrite(SS, HIGH);
//    delayMicroseconds(50);
//  }
//  tlv._rxsize = rxsize;
//}
//
//void SPIGetReturnValue(tlv &tlv){
//  digitalWrite(SS, LOW);
//  tlv.type = SPI.transfer(0xFF);
//  tlv.len = SPI.transfer(0xFF);
////  for (int i = 0; i < (tlv._rxsize-2); i++) {  
////    tlv.value[i] = SPI.transfer(0xFF);
////  }
//  digitalWrite(SS, HIGH);
//}

//-------------------------------------------------------------------------------------------------
//---------------------------------        WORKING CODE         -----------------------------------
//-------------------------------------------------------------------------------------------------

//void tlv_cmd(uint8_t[],  uint32_t &, uint8_t []);
//void SPIWrite(uint8_t[]);
//void SPIWaitForResponse(uint32_t &, int);
//void SPIGetReturnValue(uint32_t &,uint8_t[]);
//
//SPISettings SPI_SETTINGS(SPI_RATE, MSBFIRST, SPI_MODE0);
//
//void setup() {
//  pinMode(SS, OUTPUT);
//  SPI.begin();
//  Serial.println("start");
//  
//}
//
//uint8_t arr1[256];
//uint32_t rx;
//uint8_t DATA_READY_ENABLED = 0;
//uint8_t DATA_READY_FLAG = 0;
//
//void loop() {
// 
//  delay(1000);
////  uint8_t tlv[] = {0x07, 0x01, 0x9A}; //should return as 1A, 30 Anchor Init no fw_update_en
////  uint8_t tlv[] = {0x05, 0x02, 0x5A, 0x00}; //should return 5A, 00 Tag no fw_update_en
////  uint8_t tlv[] = {0x14, 0x00};
////  uint8_t tlv[] = {0x08, 0x00};
////  uint8_t tlv[] = {0x0C, 0x00};
////  uint8_t tlv[] = {0x04, 0x00}; //get update rate
////  uint8_t tlv[] = {0x03, 0x04, 0x01, 0x00, 0x05, 0x00}; //set update rate
//  uint8_t tlv[] = {0x08, 0x00, 0x90};
//  
//  unsigned long strt = micros();
//  tlv_cmd(tlv, rx, arr1); 
//  unsigned long delta= micros() - strt;
//  
//  Serial.println(delta);
//  for(int i = 0; i < rx; i++){
//    Serial.print(arr1[i], HEX);
//    Serial.print(", ");
//  }
//  Serial.println("");
//  
//  delay(1000);
//}
//
//void tlv_cmd(uint8_t tlv[], uint32_t& rxsize, uint8_t response[]){  
//  SPI.beginTransaction(SPI_SETTINGS);
//  SPIWrite(tlv);
//  SPIWaitForResponse(rxsize, 50);
//  SPIGetReturnValue(rxsize, response);
//  SPI.endTransaction();
//}
//
//void SPIWrite(uint8_t tlv[]=0){
//  uint8_t len = tlv[1] + 2;
//  digitalWrite(SS, LOW);
//  for(int i = 0; i < len; i++){tlv[i] = SPI.transfer(tlv[i]);}
//  digitalWrite(SS, HIGH);
//}
//
//void SPIWaitForResponse(uint32_t &rxsize, int usecDelay = 0){
//  rxsize = 0x00;
//  if(DATA_READY_ENABLED == 1){
//    while(!DATA_READY_FLAG);
//    digitalWrite(SS, LOW);
//    rxsize = SPI.transfer(0xFF);
//    digitalWrite(SS, HIGH);
//  }
//    else{
//      while (rxsize == 0x00) {
//        digitalWrite(SS, LOW);
//        rxsize = SPI.transfer(0xFF);
//        digitalWrite(SS, HIGH);
//        delayMicroseconds(usecDelay);
//      }
//  }
//}
//
//void SPIGetReturnValue(uint32_t &rxSize, uint8_t arr1[]){
//  digitalWrite(SS, LOW);
//  for (int i = 0; i < rxSize; i++){arr1[i] = (uint8_t)SPI.transfer(0xFF);}
//  digitalWrite(SS, HIGH);
//}

