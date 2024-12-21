//***********************************************************************************
//* ESProFile ProFile Emulator Software v1.2                                        *
//* By: Alex Anderson-McLeod                                                        *
//* Email address: alexelectronicsguy@gmail.com                                     *
//***********************************************************************************

//Try to get the creatiom/mod dates to do something
//Add while loops and make sure that there aren't any places where I assume the Lisa will be ready when I am.
//Add support for 4-byte commands (like from Xenix and NewWidEx)

#include <Arduino.h>
#include <SPI.h>
#include "SdFat.h"
#include "sdios.h"
//#include <SD.h>
#include <EEPROM.h>


#define SD_CLK  5   // SCLK
#define SD_MISO 34   // MISO
#define SD_MOSI 2   // MOSI
#define SD_CS   33   // Chip select

#define busOffset 12
#define CMDPin 21
#define BSYPin 22
#define RWPin 23
#define STRBPin 25
#define PRESPin 26
#define PARITYPin 27

#define TIMG1_WDT_WE 0x050D83AA1
#define TIMG1_WDT_WE_REG 0x3FF60064

#define TIMG1_WDT_CONF_REG 0x3FF60048
#define TIMG1_WDT_EN 1 << 31

uint8_t blockData[536]; //the array that holds the block that's currently being read or written
uint32_t rawBlockData[536];
uint32_t rawInvBlockData[536];

bool parityArray[256] = {
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};

uint8_t spareTable[48] = {0x50, 0x52, 0x4F, 0x46, 0x49, 0x4C, 0x45, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x03, 0x98, 0x00, 0x26, 0x00, 0x02, 0x14, 0x20, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x43, 0x61, 0x6D, 0x65, 0x6F, 0x2F, 0x41, 0x70, 0x68, 0x69, 0x64, 0x20, 0x30, 0x30, 0x30, 0x31}; //the array that holds the spare table

char fileName[256];

uint16_t bufferIndex = 0;

volatile uint32_t IRAM_ATTR continueLoop = true;

const int readStatusOffset = 4;
const int writeStatusOffset = 532;

byte commandBuffer[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; //the 6-byte command buffer
uint32_t prevState = 1;
uint32_t currentState = 1; //variables that are used for falling edge detection on the strobe line
int strobeIndex = 0; //an index used when counting strobe pulses
byte *pointer;
uint32_t *rawPointer;
uint32_t byteNum;
uint32_t startTime;
const uint32_t timeout = 1000000; //around 18ms
uint32_t currentTime = 0;
SdFat32 SDCard;
File32 disk;
File32 scratchFile;
File32 sourceFile;
File32 destFile;
//File32 KVStore;
//File32 KVCache;
FatFile rootDir;
int fileCount = 0;
uint16_t nonce;
uint16_t oldNonce;
unsigned long freeSpace;
const uint16_t KVMoniker = 1024;
const uint16_t KVAutoboot = 1557;
uint8_t buf[4096];
char extension[255] = ".image";
uint32_t uptime = 0; //if this isn't originally zero, then it resets each time we do the stuff mentioned on the next line down.

const int red = 32;
const int green = 4;

bool selectorCommandRun = true;

void setup(){
  REG_WRITE(TIMG1_WDT_WE_REG, TIMG1_WDT_WE);
  REG_CLR_BIT(TIMG1_WDT_CONF_REG, TIMG1_WDT_EN);
  pointer = 0;
  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  Serial.begin(115200); //start serial communications
  //clearScreen();
  nonce = EEPROM.read(5);
  EEPROM.write(5, nonce + 1);
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  setPinModes();
  delay(10);
  initPins();
  setLEDColor(1, 0);
  Serial.println(F("ESProFile Emulator Mode - Version 1.0"));
  //pinMode(blue, OUTPUT);
  //pinMode(25, OUTPUT); PARITY CHIP ENABLEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
  //digitalWrite(25, HIGH);

  if(!SDCard.begin(SD_CS, SD_SCK_MHZ(10))){ //initialize the SD card
    Serial.println(F("SD card initialization failed! Halting..."));
    while(1);
  }
  rootDir.open("/");

  if(!disk.open("profile.image", O_RDWR)){
    Serial.println(F("Default drive file profile.image not found! Halting..."));
    while(1);
  }

  updateSpareTable();
  setLEDColor(0, 1);
  Serial.println(F("ESProFile is ready!"));
  disk.seekSet(0);
  disk.read(blockData+readStatusOffset, 532);
}


void IRAM_ATTR readISR(){
  REG_WRITE(GPIO_OUT_W1TS_REG, *pointer << busOffset);
  REG_WRITE(GPIO_OUT_W1TC_REG, ((byte)~*pointer++ << busOffset));
}

void IRAM_ATTR writeISR(){
  *pointer++ = REG_READ(GPIO_IN_REG) >> busOffset;
}

void IRAM_ATTR exitLoopISR(){
  continueLoop = false;
}

void clearScreen(){
  Serial.write(27);
  Serial.print(F("[2J"));
  Serial.write(27);
  Serial.print(F("[H"));
}

void setPinModes(){
  for(int i = busOffset; i < busOffset + 8; i++){
    pinMode(i, INPUT);
  }
  pinMode(CMDPin, INPUT);
  pinMode(BSYPin, INPUT);
  pinMode(RWPin, INPUT);
  pinMode(STRBPin, INPUT);
  pinMode(PRESPin, INPUT);
  pinMode(PARITYPin, INPUT);
}

void setParallelDir(bool dir){
  if(dir == 0){
    REG_WRITE(GPIO_ENABLE_W1TC_REG, 0b11111111 << busOffset);
  }
  else if(dir == 1){
    REG_WRITE(GPIO_ENABLE_W1TS_REG, 0b11111111 << busOffset);
  }
}

void loop() {
  clearBSY();
  while(readCMD() == 1); //wait for CMD to go low
  setParallelDir(1);
  delayMicroseconds(1);
  sendData(0x01); //send an 0x01 to the host
  setBSY(); //and lower BSY to acknowledge our presence
  currentTime = 0;
  while(readCMD() == 0){
    currentTime++;
    if(currentTime >= timeout){
      Serial.println("Timeout: Initial Handshake");
      return;
    }
  } //wait for the host to raise CMD
  //while(readCMD() == 0);
  setParallelDir(0);
  delayMicroseconds(1); //set the bus into input mode so we can read from it

  currentTime = 0;
  while(receiveData() != 0x55){ //wait for the host to respond with an 0x55 and timeout if it doesn't
    currentTime++;
    if(currentTime >= timeout){
      //Serial.println(F("Phase 1: Host didn't respond with a 55! Maybe the drive was reset?"));
      return;
    }
  }

  bufferIndex = 0;
  for(int i = 0; i < 6; i++){
    commandBuffer[i] = 0;
  }
  noInterrupts();
  clearBSY(); //if everything checks out, raise BSY

  while(bitRead(REG_READ(GPIO_IN_REG), CMDPin) == 1){ //do this for each of the remaining data bytes
    currentState = bitRead(REG_READ(GPIO_IN_REG), STRBPin);
    if(currentState == 0 and prevState == 1){ //if we're on the falling edge of the strobe, put the next data byte on the bus and increment the pointer
      commandBuffer[bufferIndex] = REG_READ(GPIO_IN_REG) >> busOffset;
      if(__builtin_parity(commandBuffer[bufferIndex++]) == 0){
        clearPARITY();
      }
      else{
        setPARITY();
      }
    }
    prevState = currentState;
  }

  interrupts();
  /*for(int i = 0; i < 6; i++){
    printDataNoSpace(commandBuffer[i]);
  }*/
  if(commandBuffer[0] == 0x00){
    Serial.print("Read         Block: ");
  }
  else if(commandBuffer[0] == 0x01 or commandBuffer[0] == 0x02 or commandBuffer[0] == 0x03){
    Serial.print("Write        Block: ");
  }
  else{
    Serial.print("Unknown      Block: ");
  }
  printDataNoSpace(commandBuffer[1]);
  printDataNoSpace(commandBuffer[2]);
  printDataNoSpace(commandBuffer[3]);
  Serial.print("        Retry Count: ");
  printDataNoSpace(commandBuffer[4]);
  Serial.print("        Spare Threshold: ");
  printDataNoSpace(commandBuffer[5]);
  //delay(1);
  //cli();
  if(commandBuffer[0] == 0x00){
    readDrive(); //if the first byte of the command is 0, we need to do a read
  }
  else if(commandBuffer[0] == 0x01 or commandBuffer[0] == 0x02 or commandBuffer[0] == 0x03){
    writeDrive(commandBuffer[0] + 0x02); //if the first byte is a 1, 2, or 3, we need to do a write
  }
  //this emulator treats both writes and write-verifys exactly the same

  else{ //if we get some other command, print an error message and wait for the next handshake from the host
    Serial.println(F("Bad Command!"));
    for(int i = 0; i < 6; i++){
      printDataNoSpace(commandBuffer[i]);
    }
    Serial.println();
    Serial.println();
    setParallelDir(1);
    delayMicroseconds(1);
    sendData(0x55);
    setBSY();
  }
}

void readDrive(){
  setParallelDir(1);
  delayMicroseconds(1);
  sendData(0x02);
  setBSY(); //put the read command confirmation of 0x02 on the bus and lower BSY
  currentTime = 0;
  while(readCMD() == 0){
    currentTime++;
    if(currentTime >= timeout){
      /*sei();
      Serial.println("Timeout: Read Command Confirmation");
      cli();*/
      return;
    }
  } //wait for the host to raise CMD
  setParallelDir(0);
  delayMicroseconds(1); //set the bus into input mode
  currentTime = 0;
  while(receiveData() != 0x55){ //wait for the host to respond with an 0x55 and timeout if it doesn't
    currentTime++;
    if(currentTime >= timeout){
      Serial.println(F(" - Read phase 2: Host didn't respond with a 55!"));
      return;
    }
  }
  byteNum = (commandBuffer[1] << 16 | commandBuffer[2] << 8 | commandBuffer[3]);
  if(commandBuffer[1] == 0xFF and commandBuffer[2] == 0xFF and commandBuffer[3] == 0xFF){
    Serial.println(" - Spare Table");
    for(int i = 0; i < 48; i++){
      blockData[i+readStatusOffset] = spareTable[i];
    }
    for(int i = 48; i < 532; i++){
      blockData[i+readStatusOffset] = 0xFF;
    }
  }
  else if(commandBuffer[1] == 0xFF and commandBuffer[2] == 0xFE and commandBuffer[3] == 0xFD){ //emulator status
    Serial.println(F(" - Selector: Sending emulator status..."));
    setLEDColor(1, 1); //fix LED issue
    uptime = esp_timer_get_time() / 1000000;
    char days[5];
    char hours[3];
    char minutes[3];
    char seconds[3];
    sprintf(days, "%4d", ((uptime%(86400*30))/86400));
    sprintf(hours, "%02d", ((uptime%86400)/3600));
    sprintf(minutes, "%02d", ((uptime%3600)/60));
    sprintf(seconds, "%02d", uptime%60);
    for(int i = 0; i < 4; i++){ //uptime
      blockData[i+readStatusOffset] = days[i % 4];
    }
    for(int i = 4; i < 6; i++){ //uptime
      blockData[i+readStatusOffset] = hours[i % 2];
    }
    for(int i = 6; i < 8; i++){ //uptime
      blockData[i+readStatusOffset] = minutes[i % 2];
    }
    for(int i = 8; i < 10; i++){ //uptime
      blockData[i+readStatusOffset] = seconds[i % 2];
    }
    if(selectorCommandRun == true){
      selectorCommandRun = false;
      freeSpace = SDCard.vol()->freeClusterCount() * SDCard.vol()->sectorsPerCluster() * 512;
    }
    else{
      delay(10); // just wait so that the user can see the LED light up
    }

    char bytesFree[16];
    ultoa(freeSpace, bytesFree, 10);
    int i;
    for(i = 0; i < 15; i++){
      if(bytesFree[i] == 0x00){
        i--;
        break;
      }
    }
    int dataIndex = 24;
    for(int j = i; j >= 0; j--){ //bytes free
      blockData[dataIndex+readStatusOffset] = bytesFree[j];
      dataIndex--;
    }
    for(i = 14 - i; i > 0; i--){ //bytes free
      blockData[dataIndex+readStatusOffset] = 0x20;
      dataIndex--;
    }
    blockData[25+readStatusOffset] = 0x30; //load
    blockData[26+readStatusOffset] = 0x00; //load
    blockData[32+readStatusOffset] = 0x30; //load
    blockData[33+readStatusOffset] = 0x00; //load
    blockData[39+readStatusOffset] = 0x30; //load
    blockData[40+readStatusOffset] = 0x00; //load
    blockData[46+readStatusOffset] = 0x31; //processes
    blockData[47+readStatusOffset] = 0x00; //processes
    blockData[51+readStatusOffset] = 0x31; //processes
    blockData[52+readStatusOffset] = 0x00; //processes
    for(int i = 56; i < 532; i++){ //null terminate for the rest of the block
      blockData[i+readStatusOffset] = 0x00;
    }
  }
  else if(commandBuffer[1] == 0xFF and commandBuffer[2] == 0xFE and commandBuffer[3] == 0xFE){ //get file info
    setLEDColor(1, 1); //fix LED issue
    selectorCommandRun = true;
    fileCount = 0;
    rootDir.rewind();
    while(scratchFile.openNext(&rootDir, O_RDONLY)){
      fileName[scratchFile.getName7(fileName, 256)] = 0x00;
      if(not scratchFile.isDir() and fileName[0] != '.' and strstr(fileName, extension) != 0){
        fileCount += 1;
      }
      scratchFile.close();
    }
    for(int i = 0; i < 4; i++){ //nonce
      blockData[i+readStatusOffset] = nonce;
    }
    blockData[4+readStatusOffset] = fileCount << 8; //num of files
    blockData[5+readStatusOffset] = fileCount;

    uint32_t fileSize;
    uint16_t dateModified[50];
    uint16_t timeModified[50];
    rootDir.rewind();
    if(((commandBuffer[4] << 8) + commandBuffer[5]) < fileCount){
      fileCount = 0;
      while(fileCount <= (commandBuffer[4] << 8) + commandBuffer[5]){
        scratchFile.openNext(&rootDir, O_RDONLY);
        fileName[scratchFile.getName7(fileName, 256)] = 0x00;
        if(not scratchFile.isDir() and fileName[0] != '.' and strstr(fileName, extension) != 0){
          fileCount += 1;
          fileSize = scratchFile.fileSize();
        }
        //scratchFile.getModifyDateTime(dateModified, timeModified);
        scratchFile.close();
      }
      Serial.print(F(" - Selector: Sending file information for file "));
      Serial.print(fileName);
      Serial.println(F("..."));
      for(int i = 6; i < 20; i++){ //last modified time
        //Serial.print(dateModified[i - 6]);
        //Serial.print(" ");
        blockData[i+readStatusOffset] = 0x42;
      }
      //Serial.println();

      blockData[20+readStatusOffset] = fileSize >> 72;
      blockData[21+readStatusOffset] = fileSize >> 64;
      blockData[22+readStatusOffset] = fileSize >> 56;
      blockData[23+readStatusOffset] = fileSize >> 48;
      blockData[24+readStatusOffset] = fileSize >> 40;
      blockData[25+readStatusOffset] = fileSize >> 32;
      blockData[26+readStatusOffset] = fileSize >> 24;
      blockData[27+readStatusOffset] = fileSize >> 16;
      blockData[28+readStatusOffset] = fileSize >> 8;
      blockData[29+readStatusOffset] = fileSize;

      char sizeString[5];
      if(fileSize == 5175296){
        blockData[30+readStatusOffset] = 0x20;
        blockData[31+readStatusOffset] = 0x20;
        blockData[32+readStatusOffset] = 0x35;
        blockData[33+readStatusOffset] = 0x4D;
      }
      else if(fileSize == 10350592){
        blockData[30+readStatusOffset] = 0x20;
        blockData[31+readStatusOffset] = 0x31;
        blockData[32+readStatusOffset] = 0x30;
        blockData[33+readStatusOffset] = 0x4D;
      }
      else{
        if(fileSize > 1089003999999){
          blockData[30+readStatusOffset] = 0x48;
          blockData[31+readStatusOffset] = 0x55;
          blockData[32+readStatusOffset] = 0x47;
          blockData[33+readStatusOffset] = 0x45;
        }
        else if(fileSize > 1089003999){
          sprintf(sizeString, "%3d", (fileSize/1089004000));
          strlcat(sizeString, "G", sizeof(sizeString));
          blockData[30+readStatusOffset] = sizeString[0];
          blockData[31+readStatusOffset] = sizeString[1];
          blockData[32+readStatusOffset] = sizeString[2];
          blockData[33+readStatusOffset] = sizeString[3];
        }
        else if(fileSize > 1089004){
          sprintf(sizeString, "%3d", (fileSize/1089004));
          strlcat(sizeString, "M", sizeof(sizeString));
          blockData[30+readStatusOffset] = sizeString[0];
          blockData[31+readStatusOffset] = sizeString[1];
          blockData[32+readStatusOffset] = sizeString[2];
          blockData[33+readStatusOffset] = sizeString[3];
        }
        else if(fileSize > 9999){
          sprintf(sizeString, "%3d", (fileSize/1090));
          strlcat(sizeString, "K", sizeof(sizeString));
          blockData[30+readStatusOffset] = sizeString[0];
          blockData[31+readStatusOffset] = sizeString[1];
          blockData[32+readStatusOffset] = sizeString[2];
          blockData[33+readStatusOffset] = sizeString[3];
        }
        else{
          sprintf(sizeString, "%4d", fileSize);
          blockData[30+readStatusOffset] = sizeString[0];
          blockData[31+readStatusOffset] = sizeString[1];
          blockData[32+readStatusOffset] = sizeString[2];
          blockData[33+readStatusOffset] = sizeString[3];
        }
      }
      for(int i = 20; blockData[i+readStatusOffset] == 0x00; i++){
        blockData[i+readStatusOffset] = 0x20;
      }
      int i = 0;
      for(i = 276; fileName[i - 276] != 0x00; i++){ //filename
        blockData[i+readStatusOffset] = fileName[i - 276];
      }
      for(; i < 532; i++){
        blockData[i+readStatusOffset] = 0x00;
      }
    }
    else{
      for(int i = 0; i < 532; i++){
        blockData[i+readStatusOffset] = 0x00;
      }
    }
  }

  else if(commandBuffer[1] == 0xFF and commandBuffer[2] == 0xFE and commandBuffer[3] == 0xFF){ //retrieve key-value entry from the cache
    selectorCommandRun = true;
    setLEDColor(1, 1); //fix LED issue
    if(commandBuffer[4] == 0x53 and commandBuffer[5] == 0x43){
      Serial.println(F(" - Selector: Sending device moniker..."));
      for(int i = 0; i < 532; i++){
        blockData[i+readStatusOffset] = EEPROM.read(KVMoniker + i);
      }
    }
    else if(commandBuffer[4] == 0x53 and commandBuffer[5] == 0x61){
      Serial.println(F(" - Selector: Sending autoboot status..."));
      for(int i = 0; i < 532; i++){
        blockData[i+readStatusOffset] = EEPROM.read(KVAutoboot + i);
      }
    }
    else{
      Serial.println(F(" - Selector Error: Unsupported key-value load operation!"));
    }
  }

  else if(commandBuffer[1] == 0xFF and commandBuffer[2] == 0xFE and commandBuffer[3] == 0xFC){ //selector rescue
    selectorCommandRun = true;
    setLEDColor(1, 1); //fix LED issue
    if(commandBuffer[4] == 0xFF and commandBuffer[5] == 0xFF){ //replace selector with a spare from the rescue folder
      Serial.println(F(" - Selector: Beginning 'Selector Rescue' procedure..."));
      int replacementIndex = 0;
      char buffer[30];
      sprintf(buffer, "profile-backup-%d.image", replacementIndex);
      while(rootDir.exists(buffer)){
        replacementIndex++;
        sprintf(buffer, "profile-backup-%d.image", replacementIndex);
      }
      Serial.print(F("Backing up current Selector to "));
      Serial.print(replacementIndex);
      Serial.println(F("..."));
      disk.close();
      if(!sourceFile.open("profile.image", O_RDWR)){
        Serial.println(F("Error opening current profile.image!"));
      }
      else if(!sourceFile.rename(buffer)){
        Serial.println(F("Error creating backup of current profile.image!"));
      }
      else if(!sourceFile.close() or !sourceFile.open("/rescue/selector.image", O_RDONLY)){
        Serial.println(F("Error opening backup selector image!"));
      }
      else if(!destFile.createContiguous("profile.image", sourceFile.fileSize())){
        Serial.println(F("Error creating new profile.image!"));
      }
      else{
        size_t n;
        while ((n = sourceFile.read(buf, sizeof(buf))) > 0){
          destFile.write(buf, n);
        }
        nonce = EEPROM.read(5);
        EEPROM.write(5, nonce + 1);
        Serial.println(F("Done with Selector Rescue procedure!"));
      }
      sourceFile.close();
      destFile.close();
      if(!disk.open("profile.image", O_RDWR)){
        Serial.println(F("Failed to reopen profile.image!"));
      }
      updateSpareTable();
    }
    else if((commandBuffer[4] >> 4) == 0x00){ //send a block of a selector ProFile image
      byteNum = ((commandBuffer[4] & B00001111) << 8 | commandBuffer[5]) * 532;
      if(!sourceFile.open("/rescue/selector.image", O_RDONLY)){
        Serial.println(F(" - Selector: Error opening backup Selector ProFile image!"));
      }
      else if((byteNum + 531) >= sourceFile.fileSize()){
        Serial.println(F(" - Selector Error: Block is past the end of the file!"));
        for(int i = 0; i < 532; i++){
          blockData[i+readStatusOffset] = 0x00;
        }
      }
      else{
        disk.seekSet(byteNum);
        sourceFile.read(blockData+readStatusOffset, 532);
        Serial.print(F(" - Selector: Sending block "));
        printDataNoSpace(commandBuffer[4] & B00001111);
        printDataNoSpace(commandBuffer[5]);
        Serial.println(F(" of backup Selector ProFile image!"));
      }
      sourceFile.close();
    }
    else if((commandBuffer[4] >> 4) == 0x01){ //send a block of a selector 3.5 inch floppy image
      byteNum = ((commandBuffer[4] & B00001111) << 8 | commandBuffer[5]) * 532;
      if(!sourceFile.open("/rescue/selector.3.5inch.dc42", O_RDONLY)){
        Serial.println(F(" - Selector: Error opening backup Selector 3.5 image!"));
      }
      else if((byteNum + 531) >= sourceFile.fileSize()){
        Serial.println(F(" - Selector Error: Block is past the end of the file!"));
        for(int i = 0; i < 532; i++){
          blockData[i+readStatusOffset] = 0x00;
        }
      }
      else{
        disk.seekSet(byteNum);
        sourceFile.read(blockData+readStatusOffset, 532);
        Serial.print(F(" - Selector: Sending block "));
        printDataNoSpace(commandBuffer[4] & B00001111);
        printDataNoSpace(commandBuffer[5]);
        Serial.println(F(" of backup Selector 3.5 DC42 image..."));
      }
      sourceFile.close();
    }
    else if((commandBuffer[4] >> 4) == 0x02){ //send a block of a selector Twiggy image
      byteNum = ((commandBuffer[4] & B00001111) << 8 | commandBuffer[5]) * 532;
      if(!sourceFile.open("/rescue/selector.twiggy.dc42", O_RDONLY)){
        Serial.println(F(" - Selector: Error opening backup Selector Twiggy image!"));
      }
      else if((byteNum + 531) >= sourceFile.fileSize()){
        Serial.println(F(" - Selector Error: Block is past the end of the file!"));
        for(int i = 0; i < 532; i++){
          blockData[i+readStatusOffset] = 0x00;
        }
      }
      else{
        disk.seekSet(byteNum);
        sourceFile.read(blockData+readStatusOffset, 532);
        Serial.print(F(" - Selector: Sending block "));
        printDataNoSpace(commandBuffer[4] & B00001111);
        printDataNoSpace(commandBuffer[5]);
        Serial.println(F(" of backup Selector Twiggy DC42 image..."));
      }
      sourceFile.close();
    }
    else{
      Serial.println(F(" - Selector: Bad 'Selector Rescue' command!"));
      for(int i = 0; i < 532; i++){
        blockData[i+readStatusOffset] = 0x00;
      }
    }
  }
  else if(byteNum < disk.fileSize()){
    byteNum *= 532;
    disk.seekSet(byteNum);
    disk.read(blockData+readStatusOffset, 532);
    Serial.println();
  }
  else{
    Serial.println(F(" - Error: Requested block is out of range!"));
    for(int i = 0; i < 532; i++){
      blockData[i+readStatusOffset] = 0x00;
    }
  }

  for(int i = 0; i < 536; i++){
    rawBlockData[i] = blockData[i] << busOffset;
    //printDataSpace(blockData[i]);
    //rawBlockData[i] |= (1 << BSYPin);
    //printDataNoSpace(rawBlockData[i]);
  }

  setParallelDir(1);
  delayMicroseconds(1); //set the bus into output mode


  for(int i = 0; i < readStatusOffset; i++){
    blockData[i] = 0x00;
    //rawBlockData[i] = 0x00000000;
    //rawBlockData[i] |= (1 << BSYPin);
  }
  /*for(int i = 0; i < 536; i++){
    rawInvBlockData[i] = ((uint8_t)~blockData[i]) << busOffset;
  }*/

  bufferIndex = 0;
  if(__builtin_parity(blockData[bufferIndex]) == 0){
    clearPARITY();
  }
  else{
    setPARITY();
  }
  sendData(blockData[bufferIndex++]);

  noInterrupts();

  clearBSY(); //and raise BSY

  while(bitRead(REG_READ(GPIO_IN_REG), CMDPin) == 1){ //do this for each of the remaining data bytes //bitRead(REG_READ(GPIO_IN_REG), CMDPin) == 1
    currentState = bitRead(REG_READ(GPIO_IN_REG), STRBPin);
    if(currentState == 1 and prevState == 0){ //if we're on the falling edge of the strobe, put the next data byte on the bus and increment the pointer
      if(__builtin_parity(blockData[bufferIndex]) == 0){
        clearPARITY();
      }
      else{
        setPARITY();
      }
      REG_WRITE(GPIO_OUT_W1TS_REG, blockData[bufferIndex] << busOffset);
      REG_WRITE(GPIO_OUT_W1TC_REG, ((byte)~blockData[bufferIndex++] << busOffset));
      //REG_WRITE(GPIO_OUT_REG, rawBlockData[i++]);
    }
    prevState = currentState;
  }

  interrupts();
}

void writeDrive(byte response){
  setParallelDir(1);
  delayMicroseconds(1);
  sendData(response); //send the appropriate response byte (the command byte plus two)
  setBSY(); //and lower BSY
  currentTime = 0;
  while(readCMD() == 0){
    currentTime++;
    if(currentTime >= timeout){
      Serial.println(" - Timeout: Write Command Confirmation");
      return;
    }
  } //wait for the host to raise CMD
  setParallelDir(0);
  delayMicroseconds(1); //set the bus into input mode
  currentTime = 0;
  while(receiveData() != 0x55){ //wait for the host to respond with an 0x55 and timeout if it doesn't
    currentTime++;
    if(currentTime >= timeout){
      Serial.println(F(" - Write phase 2: Host didn't respond with a 55!"));
      return;
    }
  }

  bufferIndex = 0;
  noInterrupts();
  clearBSY(); //if everything looks good, raise BSY
 
  
  while(bitRead(REG_READ(GPIO_IN_REG), CMDPin) == 1){ //do this for each of the 532 bytes that we're receiving
    currentState = bitRead(REG_READ(GPIO_IN_REG), STRBPin);
    if(currentState == 0 and prevState == 1){ //when we detect a falling edge on the strobe line, read the data bus, save it contents into the data array, and increment the pointer
      blockData[bufferIndex] = REG_READ(GPIO_IN_REG) >> busOffset;
      if(__builtin_parity(blockData[bufferIndex++]) == 0){
        clearPARITY();
      }
      else{
        setPARITY();
      }
    }
    prevState = currentState;
  }

  interrupts();

  setParallelDir(1);
  delayMicroseconds(1);
  sendData(0x06); //send the appropriate response of 0x06
  setBSY(); //and lower BSY
  currentTime = 0;
  while(readCMD() == 0){
    currentTime++;
    if(currentTime >= timeout){
      /*sei();
      Serial.println("Timeout: Write Command Second Handshake Part 2");
      cli();*/
      return;
    }
  } //wait for the host to raise CMD
  setParallelDir(0);
  delayMicroseconds(1); //set the bus into input mode
  currentTime = 0;
  while(receiveData() != 0x55){ //wait for the host to respond with an 0x55 and timeout if it doesn't
    currentTime++;
    if(currentTime >= timeout){
      Serial.println(F(" - Write phase 3: Host didn't respond with a 55!"));
      return;
    }
  }
  bool halt = false;
  byteNum = (commandBuffer[1] << 16 | commandBuffer[2] << 8 | commandBuffer[3]);
  if(commandBuffer[1] == 0xFF and commandBuffer[2] == 0xFF and commandBuffer[3] == 0xFD and commandBuffer[4] == 0xFE and commandBuffer[5] == 0xAF){ //built-in command
    selectorCommandRun = true;
    if(blockData[0] == 0x48 and blockData[1] == 0x41 and blockData[2] == 0x4C and blockData[3] == 0x54){ //halt
      halt = true;
      Serial.println(F(" - Selector: Halting emulator..."));
      setLEDColor(1, 0);
    }
    if(blockData[0] == 0x49 and blockData[1] == 0x4D and blockData[2] == 0x41 and blockData[3] == 0x47 and blockData[4] == 0x45 and blockData[5] == 0x3A){ //switch image files
      setLEDColor(1, 1); //fix LED issue
      Serial.print(F(" - Selector: Switching to image file "));
      int i = 6;
      while(1){
        if(blockData[i] == 0x00){
          fileName[i - 6] = 0x00;
          break;
        }
        fileName[i - 6] = blockData[i];
        Serial.write(blockData[i]);
        i++;
      }
      Serial.print("...");
    }
    disk.close();
    if(!disk.open(fileName, O_RDWR)){
      Serial.print(F(" - Error opening image file!"));
      disk.close();
      if(!disk.open("profile.image", O_RDWR)){
        Serial.print(F(" And failed to reopen profile.image!"));
      }
      updateSpareTable();
    }
    else{
      updateSpareTable();
    }
    Serial.println();
    //cli();
  }
  else if(commandBuffer[1] == 0xFF and commandBuffer[2] == 0xFE and commandBuffer[3] == 0xFF){
    selectorCommandRun = true;
    setLEDColor(1, 1); //fix LED issue
    if(commandBuffer[4] == 0xFF and commandBuffer[5] == 0xFF){
      Serial.println(" - Selector: Writing to key-value store...");
    }
    else if(commandBuffer[4] == 0x53 and commandBuffer[5] == 0x43){
      Serial.println(F(" - Selector: Updating device moniker..."));
      for(int i = 0; i < 532; i++){
        EEPROM.write(KVMoniker + i, blockData[i]);
      }
    }
    else if(commandBuffer[4] == 0x53 and commandBuffer[5] == 0x61){
      Serial.println(F(" - Selector: Updating device autoboot status..."));
      for(int i = 0; i < 532; i++){
        EEPROM.write(KVAutoboot + i, blockData[i]);
      }
    }
    else{
      Serial.println(F(" - Selector Error: Unsupported key-value write operation!"));
    }
    //cli();
  }
  else if(commandBuffer[1] == 0xFF and commandBuffer[2] == 0xFE and commandBuffer[3] == 0xFE){ //FS commands
    selectorCommandRun = true;
    setLEDColor(1, 1); //fix LED issue
    //sei();
    if(commandBuffer[4] == 0x63 and commandBuffer[5] == 0x70){ //copy
      Serial.print(F(" - Selector: Copying "));
      int i = 0;
      while(1){
        if(blockData[i] == 0x00){
          fileName[i] = 0x00;
          break;
        }
        fileName[i] = blockData[i];
        Serial.write(blockData[i]);
        i++;
      }
      i++;
      if(!sourceFile.open(fileName, O_RDONLY)){
        Serial.println(F(" (Error opening source file)"));
      }
      else{
        Serial.print(F(" to "));
        int offset = i;
        while(1){
          if(blockData[i] == 0x00){
            fileName[i - offset] = 0x00;
            break;
          }
          fileName[i - offset] = blockData[i];
          Serial.write(fileName[i - offset]);
          i++;
        }
        Serial.print(F(" with size "));
        Serial.print(sourceFile.fileSize());
        Serial.print("; this may take a while...");
        if(!destFile.createContiguous(fileName, sourceFile.fileSize())){
          Serial.println(F(" Error creating destination file"));
        }
        else{
          size_t n;
          while ((n = sourceFile.read(buf, sizeof(buf))) > 0){
            destFile.write(buf, n);
          }
          nonce = EEPROM.read(5);
          EEPROM.write(5, nonce + 1);
          Serial.println(F(" Success!"));
        }
      }
      sourceFile.close();
      destFile.close();
    }
    else if(commandBuffer[4] == 0x6D and commandBuffer[5] == 0x6B){ //create new image, normal
      setLEDColor(1, 1); //fix LED issue
      Serial.print(F(" - Selector: Creating new 5MB image called "));
      int i = 0;
      while(1){
        if(blockData[i] == 0x00){
          fileName[i] = 0x00;
          break;
        }
        fileName[i] = blockData[i];
        i++;
      }
      Serial.print("...");
      if(!destFile.createContiguous(fileName, 5175296)){
        Serial.print(F(" Error creating destination file!"));
      }
      Serial.println();
      nonce = EEPROM.read(5);
      EEPROM.write(5, nonce + 1);
      destFile.close();
    }
    else if(commandBuffer[4] == 0x6D and commandBuffer[5] == 0x78){ //create new image, extended
      setLEDColor(1, 1); //fix LED issue
      Serial.print(F(" - Selector: Creating new image of size "));
      int i = 0;
      while(1){
        if(blockData[i] == 0x00){
          fileName[i] = 0x00;
          break;
        }
        fileName[i] = blockData[i];
        i++;
      }
      uint32_t size = strtol(fileName, NULL, 10);
      Serial.print(size);
      i++;
      int offset = i;
      Serial.print(F(" called "));
      while(1){
        if(blockData[i] == 0x00){
          fileName[i - offset] = 0x00;
          break;
        }
        fileName[i - offset] = blockData[i];
        Serial.write(fileName[i - offset]);
        i++;
      }
      Serial.print("...");
      if(!destFile.createContiguous(fileName, size)){
        Serial.print(F(" Error creating destination file!"));
      }
      Serial.println();
      nonce = EEPROM.read(5);
      EEPROM.write(5, nonce + 1);
      destFile.close();
    }
    else if(commandBuffer[4] == 0x72 and commandBuffer[5] == 0x6D){ //delete
      setLEDColor(1, 1); //fix LED issue
      int i = 0;
      Serial.print(F(" - Selector: Deleting file "));
      while(1){
        if(blockData[i] == 0x00){
          fileName[i] = 0x00;
          break;
        }
        fileName[i] = blockData[i];
        Serial.write(blockData[i]);
        i++;
      }
      Serial.print("...");
      if(!rootDir.remove(fileName)){
        Serial.print(F(" Failed to delete file!"));
      }
      Serial.println();
      nonce = EEPROM.read(5);
      EEPROM.write(5, nonce + 1);
    }
    else if(commandBuffer[4] == 0x6D and commandBuffer[5] == 0x76){ //move aka rename
      setLEDColor(1, 1); //fix LED issue
      Serial.print(F(" - Selector: Renaming "));
      int i = 0;
      while(1){
        if(blockData[i] == 0x00){
          fileName[i] = 0x00;
          break;
        }
        fileName[i] = blockData[i];
        Serial.write(blockData[i]);
        i++;
      }
      i++;
      if(!sourceFile.open(fileName, O_RDWR)){
        Serial.println();
        Serial.println(F(" (Error opening source file)"));
      }
      else{
        Serial.print(" to ");
        int offset = i;
        while(1){
          if(blockData[i] == 0x00){
            fileName[i - offset] = 0x00;
            break;
          }
          fileName[i - offset] = blockData[i];
          Serial.write(blockData[i]);
          i++;
        }
        Serial.print("...");
        if(!sourceFile.rename(fileName)){
          Serial.print(F(" Error renaming file!"));
        }
        Serial.println();
      }
      sourceFile.close();
      nonce = EEPROM.read(5);
      EEPROM.write(5, nonce + 1);
    }
    else if(commandBuffer[4] == 0x73 and commandBuffer[5] == 0x78){ //sx (set extension)
      setLEDColor(1, 1); //fix LED issue
      int i = 0;
      while(1){
        if(blockData[i] == 0x00){
          extension[i] = 0x00;
          break;
        }
        extension[i] = blockData[i];
        i++;
      }
      Serial.print(F(" - Selector: Changing file extension to "));
      Serial.print(extension);
      Serial.println("...");
      nonce = EEPROM.read(5);
      EEPROM.write(5, nonce + 1);
    }
    //cli();
  }
  else if(byteNum < disk.fileSize()){
    byteNum *= 532;
    disk.seekSet(byteNum);
    disk.write(blockData, 532);
    disk.flush();
    Serial.println();
  }
  else{
    //sei();
    Serial.println(F(" - Error: Requested block is out of range!"));
    //cli();
  }
  setParallelDir(1);
  delayMicroseconds(1); //and set the bus into output mode

/*for(byte *i = blockData - 4; i < blockData; i++){
  *i = 0x00;
}*/

for(int i = writeStatusOffset; i < 536; i++){
  blockData[i] = 0x00;
}
  //pointer = blockData - 4; //make the pointer point toward our blockData array
  bufferIndex = 532;
  if(__builtin_parity(blockData[bufferIndex]) == 0){
    clearPARITY();
  }
  else{
    setPARITY();
  }
  sendData(blockData[bufferIndex++]); //and put the first status byte on the bus
  noInterrupts();
  clearBSY(); //if everything looks good, raise BSY


  while(bitRead(REG_READ(GPIO_IN_REG), CMDPin) == 1){ //do this for the remaining three status bytes
    currentState = bitRead(REG_READ(GPIO_IN_REG), STRBPin);
    if(currentState == 0 and prevState == 1){ //if we're on the falling edge of the strobe, send the next status byte and increment the index
      if(__builtin_parity(blockData[bufferIndex]) == 0){
        clearPARITY();
      }
      else{
        setPARITY();
      }
      REG_WRITE(GPIO_OUT_W1TS_REG, blockData[bufferIndex] << busOffset);
      REG_WRITE(GPIO_OUT_W1TC_REG, ((byte)~blockData[bufferIndex++] << busOffset));
    }
    prevState = currentState;
  }
  if(halt == true){
    while(1);
  }
}

void initPins(){
  REG_WRITE(GPIO_ENABLE_W1TC_REG, 0b1 << CMDPin);
  REG_WRITE(GPIO_ENABLE_W1TS_REG, 0b1 << BSYPin);
  REG_WRITE(GPIO_ENABLE_W1TC_REG, 0b1 << RWPin);
  REG_WRITE(GPIO_ENABLE_W1TC_REG, 0b1 << STRBPin);
  REG_WRITE(GPIO_ENABLE_W1TC_REG, 0b1 << PRESPin);
  REG_WRITE(GPIO_ENABLE_W1TS_REG, 0b1 << PARITYPin);

  clearBSY();
  //clearPARITY();
}

void sendData(byte parallelBits){ //makes it more user-friendly to put data on the bus
  //DDRL = B11111111; //set the bus to output mode
  //PORTL = parallelBits; //and write the parallelBits to the bus
  REG_WRITE(GPIO_OUT_W1TS_REG, parallelBits << busOffset);
  REG_WRITE(GPIO_OUT_W1TC_REG, ((byte)~parallelBits << busOffset));
}

byte receiveData(){ //makes it more user-friendly to receive data
  return REG_READ(GPIO_IN_REG) >> busOffset;
  //DDRL = B00000000; //set the bus to input mode
  //return PINL; //and return whatever's on the bus
}

void updateSpareTable(){
  if(disk.fileSize() == 5175296){
    spareTable[8] = 0x20; //Name
    spareTable[9] = 0x20;
    spareTable[10] = 0x20;
    spareTable[15] = 0x00; //Device Number
    spareTable[16] = 0x03; //ROM Revision
    spareTable[17] = 0x98;
    spareTable[18] = 0x00; //Drive Size
    spareTable[19] = 0x26;
    spareTable[20] = 0x00;
    Serial.println(F("Switching to the 5MB ProFile spare table."));
  }
  else if(disk.fileSize() == 10350592){
    spareTable[8] = 0x31; //Name
    spareTable[9] = 0x30;
    spareTable[10] = 0x4D;
    spareTable[15] = 0x10; //Device Number
    spareTable[16] = 0x04; //ROM Revision
    spareTable[17] = 0x04;
    spareTable[18] = 0x00; //Drive Size
    spareTable[19] = 0x4C;
    spareTable[20] = 0x00;
    Serial.println(F("Switching to the 10MB ProFile spare table."));
  }
  else{
    spareTable[8] = 0x20; //Name
    spareTable[9] = 0x20;
    spareTable[10] = 0x20;
    spareTable[15] = 0x00; //Device Number
    spareTable[16] = 0x03; //ROM Revision
    spareTable[17] = 0x98;
    spareTable[18] = (disk.fileSize()/532) >> 16; //Drive Size
    spareTable[19] = (disk.fileSize()/532) >> 8;
    spareTable[20] = (disk.fileSize()/532);
    Serial.print(F("Switching to a custom spare table for drive of size "));
    printDataNoSpace(spareTable[18]);
    printDataNoSpace(spareTable[19]);
    printDataNoSpace(spareTable[20]);
    Serial.println(F(" blocks."));
  }
}

//all of these functions just make it easier to set, clear, and read the control signals for the drive

void setBSY(){
  setLEDColor(0, 0);
  REG_WRITE(GPIO_OUT_W1TC_REG, 0b1 << BSYPin);
}

void clearBSY(){
  setLEDColor(0, 1);
  REG_WRITE(GPIO_OUT_W1TS_REG, 0b1 << BSYPin);
}

void setPARITY(){
  REG_WRITE(GPIO_OUT_W1TC_REG, 0b1 << PARITYPin);
}

void clearPARITY(){
  REG_WRITE(GPIO_OUT_W1TS_REG, 0b1 << PARITYPin);
}

bool readCMD(){
  return bitRead(REG_READ(GPIO_IN_REG), CMDPin);
}

bool readRW(){
  return bitRead(REG_READ(GPIO_IN_REG), RWPin);
}

bool readSTRB(){
  return bitRead(REG_READ(GPIO_IN_REG), STRBPin);
}

bool readPRES(){
  return bitRead(REG_READ(GPIO_IN_REG), PRESPin);
}

void setLEDColor(bool r, bool g){
  if(r == 1){
    REG_WRITE(GPIO_OUT1_W1TS_REG, 0b1);
  }
  else{
    REG_WRITE(GPIO_OUT1_W1TC_REG, 0b1);
  }
  if(g == 1){
    REG_WRITE(GPIO_OUT_W1TS_REG, 0b1 << green);
  }
  else{
    REG_WRITE(GPIO_OUT_W1TC_REG, 0b1 << green);
  }
}


//the following functions just format hex data for printing over serial and are only used for debugging


void printDataNoSpace(byte information){
  Serial.print(information>>4, HEX);
  Serial.print(information&0x0F, HEX);
}

void printDataSpace(byte information){
  Serial.print(information>>4, HEX);
  Serial.print(information&0x0F, HEX);
  Serial.print(F(" "));
}

void printRawData(){
  Serial.println(F("Raw Data:"));
  Serial.print(F("0000: "));
  for(int i = 0; i <= 531; i++){
    Serial.print(blockData[i]>>4, HEX);
    Serial.print(blockData[i]&0x0F, HEX);
    Serial.print(F(" "));
    if((i + 1) % 8 == 0 and (i + 1) % 16 != 0){
      Serial.print(F("  "));
    }
    if((i + 1) % 16 == 0){
      Serial.print(F("        "));
      for(int j = i - 15; j <= i; j++){
        if(blockData[j] <= 0x1F){
          Serial.print(F("."));
        }
        else{
          Serial.write(blockData[j]);
        }
      }
      Serial.println();
      if((i + 1) < 0x100){
        Serial.print(F("00"));
      }
      if((i + 1) >= 0x100){
        Serial.print(F("0"));
      }
      Serial.print((i + 1), HEX);
      Serial.print(F(": "));
    }
  }
  Serial.print("                                              ");
  for(int i = 528; i < 532; i++){
    if(blockData[i] <= 0x1F){
      Serial.print(F("."));
    }
    else{
      Serial.write(blockData[i]);
    }
  }
  Serial.println();
}
