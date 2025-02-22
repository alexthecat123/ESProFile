//***********************************************************************************
//* ESProFile ProFile Emulator Software v1.2                                        *
//* By: Alex Anderson-McLeod                                                        *
//* Email address: alexelectronicsguy@gmail.com                                     *
//***********************************************************************************

// ******** Changelog ********
// 2/12/2025 - v1.1 - Fixed an issue where printing debug information over serial would sometimes cause read errors when using ESProFile under LOS 3.0 with a 2-port parallel card
// 2/22/2025 - v1.2 - Improved performance by about 30% (including during Selector copy operations) by making some tweaks to the SPI initialization routines, copy buffer size, and inlining some functions

// Watchdog timer write enable register and value
#define TIMG1_WDT_WE 0x050D83AA1
#define TIMG1_WDT_WE_REG 0x3FF60064

// Watchdog timer configuration register and enable bit
#define TIMG1_WDT_CONF_REG 0x3FF60048
#define TIMG1_WDT_EN 1 << 31

#define readStatusOffset 4 // Status bytes are bytes 0-3 of blockData during a read
#define writeStatusOffset 532 // And bytes 532-535 during a write

#define KVMoniker 1024 // The locations of the Selector moniker and autoboot flag in the ESP32's EEPROM
#define KVAutoboot 1557

#define timeout 10000000 // Timeout value (in loop iterations) for all phases of ProFile comms except the initial handshake
#define handshakeTimeout 1000000 // Timeout value for the initial handshake phase (should be shorter since the host intentionally times out here to detect the drive)

SPIClass SD_SPI(HSPI); // These two lines make sure that we use hardware SPI at 20MHz for the SD card
#define SD_CONFIG SdSpiConfig(SD_CS, DEDICATED_SPI, SD_SCK_MHZ(20), &SD_SPI)

uint8_t blockData[536]; // The ProFile block that we're currently reading/writing, with status bytes too
uint8_t commandBuffer[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // The 6-byte command that the ProFile is currently executing
// The contents of the ProFile spare table
uint8_t spareTable[48] = {0x50, 0x52, 0x4F, 0x46, 0x49, 0x4C, 0x45, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x03, 0x98, 0x00, 0x26, 0x00, 0x02, 0x14, 0x20, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x43, 0x61, 0x6D, 0x65, 0x6F, 0x2F, 0x41, 0x70, 0x68, 0x69, 0x64, 0x20, 0x30, 0x30, 0x30, 0x31}; //the array that holds the spare table

bool parityArray[256] = {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0}; // A lookup table for parity bits

uint32_t blockNum; // The block that the current operation is being performed on

uint16_t bufferIndex = 0; // Index into blockData and commandBuffer used during rapid comms with the Lisa

bool prevState = 1; // Variables that are used for edge detection on the strobe line
bool currentState = 1;

uint32_t startTime; // These two variables are used for detecting timeouts on the bus
uint32_t currentTime = 0;

SdFat32 SDCard; // The SD card object
File32 disk; // The disk image file that the ESProFile is currently using
File32 scratchFile; // A file that we use as an intermediate during certain disk operations
File32 sourceFile; // Source and destination files for copy operations
File32 destFile;
FatFile rootDir; // The root directory of the SD card
static char buf[65536]; // A buffer that we use when copying files

char fileName[256]; // 256-character filename for disk images
char extension[255] = ".image"; // The extension that the Selector will use for disk images

uint16_t fileCount = 0; // The total number of files on the SD card
uint64_t freeSpace; // The amount of free space (in bytes) on the SD card

uint16_t nonce; // These two variables are used to determine if we need to update certain Selector attributes at any given time
uint16_t oldNonce;

uint32_t uptime = 0; // The number of seconds that the ESProFile has been running, according to the RTC

bool selectorCommandRun = true; // This gets set to true if any Selector command is run

void emulatorSetup(){
  // We need to disable the interrupt watchdog timer; we disable interrupts for so long that the watchdog triggers and ruins everything
  REG_WRITE(TIMG1_WDT_WE_REG, TIMG1_WDT_WE); // Enable writing to the watchdog timer registers
  REG_CLR_BIT(TIMG1_WDT_CONF_REG, TIMG1_WDT_EN); // And clear the timer's enable bit to disable it
  SD_SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS); // Start comms with the SD card using our hardware SPI instance
  clearScreen(); // Clear the screen
  EEPROM.begin(3000); // Initialize the ESP32's EEPROM
  nonce = EEPROM.read(5); // And read the current nonce out of EEPROM
  EEPROM.write(5, nonce + 1); // Then write it back, incrementing it by one
  EEPROM.commit();
  initPinsEmulator(); // Set all the ESProFile's pins to the correct direction and state
  setLEDColor(1, 0); // Make the LED red since the ESProFile hasn't initialized yet
  Serial.println("ESProFile Emulator Mode - Version 1.2"); // Print a welcome message
  Serial.println("If you find any bugs, please email me at alexelectronicsguy@gmail.com!");
  if(!SDCard.begin(SD_CONFIG)){ // Initialize the SD card with our hardware SPI instance
    Serial.println("SD card initialization failed! Halting..."); // And print an error/go into an infinite loop on failure
    while(1);
  }
  rootDir.open("/");

  if(!disk.open("profile.image", O_RDWR)){ // Try opening the default disk image file
    Serial.println("Default drive file profile.image not found! Halting..."); // Give another error/infinite loop on failure
    while(1);
  }

  updateSpareTable(); // If all this succeeds, update the spare table to reflect the attributes of the current disk image
  setLEDColor(0, 1); // Make the LED green to show that the ESProFile is ready
  Serial.println("ESProFile is ready!"); // And print a ready message
}

void emulatorLoop() {
  clearBSY(); // Make sure we're not telling the host we're busy
  while(readCMD() == 1); // Wait for CMD to go low (the start of a ProFile handshake)
  setParallelDir(1); // Set the ProFile bus to output mode
  //delayMicroseconds(1);
  sendData(0x01); // Send an 0x01 to the host
  setBSY(); // And lower BSY to acknowledge our presence
  currentTime = 0;
  while(readCMD() == 0){ // Wait for the host to raise CMD in response, and timeout if it doesn't do this in time
    currentTime++;
    if(currentTime >= handshakeTimeout){
      Serial.println("Timeout: Initial Handshake");
      return;
    }
  }
  setParallelDir(0); // Now set the bus to input mode
  //delayMicroseconds(1);
  currentTime = 0;
  while(receiveData() != 0x55){ // And wait for the host to respond with an 0x55; timeout if it doesn't
    // Fun fact: if we don't get a 55 here, it probably means that the host was just trying to see if the drive is there, but didn't care about actually doing a data transfer
    // The Lisa does this at boot time to see if a ProFile is present
    currentTime++;
    if(currentTime >= handshakeTimeout){
      //Serial.println("Phase 1: Host didn't respond with a 55! Maybe the drive was reset?");
      return;
    }
  }

  bufferIndex = 0; // Rewind to the start of the command buffer
  for(int i = 0; i < 6; i++){ // And zero the whole thing out, in case this command ends up being less than 6 bytes
    commandBuffer[i] = 0;
  }
  noInterrupts(); // We need speed here, and can't let FreeRTOS preempt us, so disable interrupts
  clearBSY(); // Raise BSY to show that we're ready to receive a command

  while(bitRead(REG_READ(GPIO_IN_REG), CMDPin) == 1){ // Stay in the command-reception loop until the host lowers CMD
    currentState = bitRead(REG_READ(GPIO_IN_REG), STRBPin); // Read the current state of STRB
    if(currentState == 0 and prevState == 1){ // If we see a falling edge on STRB, read the bus and store the data in the command buffer
      commandBuffer[bufferIndex] = REG_READ(GPIO_IN_REG) >> busOffset;
      sendParity(commandBuffer[bufferIndex++]); // Send the parity bit out for that byte, and increment bufferIndex to the next spot in the buffer
    }
    prevState = currentState; // Update the previous state of STRB
  }

  interrupts(); // We're done with the fast part, so re-enable interrupts

  if(commandBuffer[0] == 0x00){
    readDrive(); // If the first byte of the command is 0, then it's a read command
  }
  else if(commandBuffer[0] == 0x01 or commandBuffer[0] == 0x02 or commandBuffer[0] == 0x03){
    writeDrive(); // If the first byte is a 1, 2, or 3, then it's a write command
    // The ESProFile emulator treats both writes and write-verifys exactly the same; it doesn't matter on solid-state media
  }

  else{ // If we get some other command, stop and wait for the next handshake from the host
    printCommand(); // Print the command that we didn't understand
    setParallelDir(1); // Set the bus to output mode
    //delayMicroseconds(1);
    sendData(0x55); // Put an invalid value on the bus to show that we didn't understand the command
    setBSY(); // And tell the host that it's there
  }
}

// Process and execute a read command
void readDrive(){
  setParallelDir(1); // Set the bus to output mode
  //delayMicroseconds(1);
  sendData(0x02); // Acknowledge the read command with an 0x02 (command value + 2)
  setBSY(); // And lower BSY to tell the host about our acknowledgement
  printCommand(); // Now that we're in control of the pace of the bus, print the command that we're executing
  currentTime = 0;
  while(readCMD() == 0){ // Wait for the host to raise CMD and timeout if it doesn't
    currentTime++;
    if(currentTime >= timeout){
      //Serial.println("Timeout: Read Command Confirmation");
      return;
    }
  }
  setParallelDir(0); // Set the bus to input mode
  //delayMicroseconds(1);
  currentTime = 0;
  while(receiveData() != 0x55){ // And wait for the host to respond with an 0x55; timeout if it doesn't
    currentTime++;
    if(currentTime >= timeout){
      Serial.println(" - Read phase 2: Host didn't respond with a 55!");
      return;
    }
  }
  blockNum = (commandBuffer[1] << 16 | commandBuffer[2] << 8 | commandBuffer[3]); // Form a block number from bytes 1-3 of the command buffer
  // If the block number is 0xFFFFFF, then the host is asking for the spare table
  if(commandBuffer[1] == 0xFF and commandBuffer[2] == 0xFF and commandBuffer[3] == 0xFF){
    Serial.println(" - Spare Table"); // Print this fact to the console
    for(int i = 0; i < 48; i++){ // And fill our block data buffer with the spare table
      blockData[i+readStatusOffset] = spareTable[i];
    }
    for(int i = 48; i < 532; i++){ // Pad the rest of the space with FF's
      blockData[i+readStatusOffset] = 0xFF;
    }
  }
  // If the block number is 0xFFFEFD, then it's a Selector "get emulator status" command
  else if(commandBuffer[1] == 0xFF and commandBuffer[2] == 0xFE and commandBuffer[3] == 0xFD){
    Serial.println(" - Selector: Sending emulator status...");
    setLEDColor(1, 1); // Make the LED amber to signify a Selector command
    uptime = esp_timer_get_time() / 1000000; // Get the uptime of the ESProFile in seconds
    char days[5];
    char hours[3];
    char minutes[3];
    char seconds[3];
    sprintf(days, "%4d", ((uptime%(86400*30))/86400)); // Convert this to days, hours, minutes, and seconds
    sprintf(hours, "%02d", ((uptime%86400)/3600));
    sprintf(minutes, "%02d", ((uptime%3600)/60));
    sprintf(seconds, "%02d", uptime%60);
    for(int i = 0; i < 4; i++){ // And fill the block data buffer with this uptime information
      blockData[i+readStatusOffset] = days[i % 4];
    }
    for(int i = 4; i < 6; i++){
      blockData[i+readStatusOffset] = hours[i % 2];
    }
    for(int i = 6; i < 8; i++){
      blockData[i+readStatusOffset] = minutes[i % 2];
    }
    for(int i = 8; i < 10; i++){
      blockData[i+readStatusOffset] = seconds[i % 2];
    }
    // If another Selector command has been run since the last time this one was run, we need to update the free space on the SD card
    // We don't do this every time because it can take forever for large SD cards
    if(selectorCommandRun == true){
      selectorCommandRun = false;
      // In fact, it takes so long that I've commented this line out to disable it for now
      //freeSpace = SDCard.vol()->freeClusterCount() * SDCard.vol()->sectorsPerCluster() * 512; // Get the free space in bytes
    }
    else{
      delay(10); // If we don't need to update free space, just wait for 10ms so that the user can see the LED light up amber
    }

    // Uncomment these to re-enable the free space indicator
    //char bytesFree[16];
    //ultoa(freeSpace, bytesFree, 10); // Convert the free space to a string

    char bytesFree[] = "????????"; // Since we've disabled the free space indicator, just have it send "???????" instead

    int i;
    for(i = 0; i < 15; i++){ // Figure out how many digits are in the free space
      if(bytesFree[i] == 0x00){
        i--;
        break;
      }
    }
    int dataIndex = 24;
    for(int j = i; j >= 0; j--){ // And fill the block data buffer with the free space information, starting at dataIndex
      blockData[dataIndex+readStatusOffset] = bytesFree[j];
      dataIndex--;
    }
    for(i = 14 - i; i > 0; i--){ // Pad the remaining space with 0x20's (space characters)
      blockData[dataIndex+readStatusOffset] = 0x20;
      dataIndex--;
    }
    // Our system is too simple to worry about load or processes, so just fill these fields with random data that looks cool
    blockData[25+readStatusOffset] = 0x30; // Load
    blockData[26+readStatusOffset] = 0x00; // Load
    blockData[32+readStatusOffset] = 0x30; // Load
    blockData[33+readStatusOffset] = 0x00; // Load
    blockData[39+readStatusOffset] = 0x30; // Load
    blockData[40+readStatusOffset] = 0x00; // Load
    blockData[46+readStatusOffset] = 0x31; // Processes
    blockData[47+readStatusOffset] = 0x00; // Processes
    blockData[51+readStatusOffset] = 0x31; // Processes
    blockData[52+readStatusOffset] = 0x00; // Processes
    for(int i = 56; i < 532; i++){ // Then null terminate for the rest of the block
      blockData[i+readStatusOffset] = 0x00;
    }
  }
  // If the block number is 0xFFFEFE, then it's a Selector "get file info" command
  else if(commandBuffer[1] == 0xFF and commandBuffer[2] == 0xFE and commandBuffer[3] == 0xFE){
    setLEDColor(1, 1); // Set the LED to amber to show that we're running a Selector command
    selectorCommandRun = true; // And set the flag that tells us that a Selector command has been run
    fileCount = 0; // Initialize the file counter to 0
    rootDir.rewind(); // And go back to the start of the SD root directory
    while(scratchFile.openNext(&rootDir, O_RDONLY)){ // Keep opening files in the root directory until we run out, incrementing fileCount for each valid file
      fileName[scratchFile.getName7(fileName, 256)] = 0x00;
      if(not scratchFile.isDir() and fileName[0] != '.' and strstr(fileName, extension) != 0){
        fileCount += 1;
      }
      scratchFile.close();
    }
    for(int i = 0; i < 4; i++){ // Set the first four bytes of the block data to the nonce
      blockData[i+readStatusOffset] = nonce;
    }
    // And the next two bytes to the file count
    blockData[4+readStatusOffset] = fileCount << 8;
    blockData[5+readStatusOffset] = fileCount;

    uint32_t fileSize;
    uint16_t dateModified[50];
    uint16_t timeModified[50];
    rootDir.rewind(); // Now go back to the start of the directory again
    if(((commandBuffer[4] << 8) + commandBuffer[5]) < fileCount){ // And double-check that the host is asking for info on a valid file
      fileCount = 0;
      while(fileCount <= (commandBuffer[4] << 8) + commandBuffer[5]){ // Keep opening files until we get to the one that the host is asking for
        scratchFile.openNext(&rootDir, O_RDONLY);
        fileName[scratchFile.getName7(fileName, 256)] = 0x00;
        if(not scratchFile.isDir() and fileName[0] != '.' and strstr(fileName, extension) != 0){
          fileCount += 1;
          fileSize = scratchFile.fileSize(); // Once we find the file, set fileSize to its size
        }
        // Modification date and time are not implemented yet; I couldn't get them to work with the SdFat library
        //scratchFile.getModifyDateTime(dateModified, timeModified);
        scratchFile.close();
      }
      Serial.print(" - Selector: Sending file information for file ");
      Serial.print(fileName);
      Serial.println("...");
      for(int i = 6; i < 20; i++){ // Since we don't have modification date and time, just fill these fields with random data
        //Serial.print(dateModified[i - 6]);
        //Serial.print(" ");
        blockData[i+readStatusOffset] = 0x42;
      }
      // Fill the next 10 bytes with the file size
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

      // That was the raw filesize, we also have to provide a size string like "5M or 16K"
      char sizeString[5];
      if(fileSize == 5175296){ // If it's a 5MB image, set the size string in the block data to "5M"
        blockData[30+readStatusOffset] = 0x20;
        blockData[31+readStatusOffset] = 0x20;
        blockData[32+readStatusOffset] = 0x35;
        blockData[33+readStatusOffset] = 0x4D;
      }
      else if(fileSize == 10350592){ // If it's a 10MB image, set the size string to "10M"
        blockData[30+readStatusOffset] = 0x20;
        blockData[31+readStatusOffset] = 0x31;
        blockData[32+readStatusOffset] = 0x30;
        blockData[33+readStatusOffset] = 0x4D;
      }
      else{ // Otherwise, things will need to get a bit more complicated
        if(fileSize > 1089003999999){ // If the file is bigger than GB-scale, make the size string say "HUGE"
          blockData[30+readStatusOffset] = 0x48;
          blockData[31+readStatusOffset] = 0x55;
          blockData[32+readStatusOffset] = 0x47;
          blockData[33+readStatusOffset] = 0x45;
        }
        else if(fileSize > 1089003999){ // If the file is GB-scale, then make the appropriate size string, append "G", and put it in the block data
          sprintf(sizeString, "%3d", (fileSize/1089004000));
          strlcat(sizeString, "G", sizeof(sizeString));
          blockData[30+readStatusOffset] = sizeString[0];
          blockData[31+readStatusOffset] = sizeString[1];
          blockData[32+readStatusOffset] = sizeString[2];
          blockData[33+readStatusOffset] = sizeString[3];
        }
        else if(fileSize > 1089004){ // Repeat for MB-scale
          sprintf(sizeString, "%3d", (fileSize/1089004));
          strlcat(sizeString, "M", sizeof(sizeString));
          blockData[30+readStatusOffset] = sizeString[0];
          blockData[31+readStatusOffset] = sizeString[1];
          blockData[32+readStatusOffset] = sizeString[2];
          blockData[33+readStatusOffset] = sizeString[3];
        }
        else if(fileSize > 9999){ // KB-scale
          sprintf(sizeString, "%3d", (fileSize/1090));
          strlcat(sizeString, "K", sizeof(sizeString));
          blockData[30+readStatusOffset] = sizeString[0];
          blockData[31+readStatusOffset] = sizeString[1];
          blockData[32+readStatusOffset] = sizeString[2];
          blockData[33+readStatusOffset] = sizeString[3];
        }
        else{ // And B-scale
          sprintf(sizeString, "%4d", fileSize);
          blockData[30+readStatusOffset] = sizeString[0];
          blockData[31+readStatusOffset] = sizeString[1];
          blockData[32+readStatusOffset] = sizeString[2];
          blockData[33+readStatusOffset] = sizeString[3];
        }
      }
      for(int i = 20; blockData[i+readStatusOffset] == 0x00; i++){ // Replace all zeros in the block between indices 20 and the next data field with 0x20s (spaces)
        blockData[i+readStatusOffset] = 0x20;
      }
      int i = 0;
      for(i = 276; fileName[i - 276] != 0x00; i++){ // Then put the filename into the block data starting at index 276
        blockData[i+readStatusOffset] = fileName[i - 276];
      }
      for(; i < 532; i++){ // And fill the rest of the block with zeros
        blockData[i+readStatusOffset] = 0x00;
      }
    }
    else{ // If the file number wasn't valid this whole time, just fill the block with 0x00's instead
      for(int i = 0; i < 532; i++){
        blockData[i+readStatusOffset] = 0x00;
      }
    }
  }
  // If the block number is 0xFFFEFF, then it's a Selector key-value store command
  else if(commandBuffer[1] == 0xFF and commandBuffer[2] == 0xFE and commandBuffer[3] == 0xFF){
    // The Selector key-value store is currently only used for setting the device moniker and autoboot state, so that's all that is implemented here
    selectorCommandRun = true; // Set the flag indicating that a Selector command has been run
    setLEDColor(1, 1); // Set the LED color to amber to signify that a Selector command has been run
    if(commandBuffer[4] == 0x53 and commandBuffer[5] == 0x43){ // If the retry count and spare threshold are 0x53 and 0x43, then the host wants our moniker
      Serial.println(" - Selector: Sending device moniker...");
      for(int i = 0; i < 532; i++){ // So read the moniker out of the ESP32's EEPROM and put it in our block data
        blockData[i+readStatusOffset] = EEPROM.read(KVMoniker + i);
      }
    }
    else if(commandBuffer[4] == 0x53 and commandBuffer[5] == 0x61){ // If the retry/spare are 0x53 and 0x61, then the host wants our autoboot status
      Serial.println(" - Selector: Sending autoboot script...");
      for(int i = 0; i < 532; i++){ // So read it out of the EEPROM and put it in the block data
        blockData[i+readStatusOffset] = EEPROM.read(KVAutoboot + i);
      }
    }
    else{ // Any other retry count and spare threshold is for another key-value store location that we haven't implemented
      Serial.println(" - Selector Error: Unsupported key-value load operation!");
    }
  }

  // If the block number is 0xFFFEFC, then it's a Selector "selector rescue" command
  else if(commandBuffer[1] == 0xFF and commandBuffer[2] == 0xFE and commandBuffer[3] == 0xFC){
    selectorCommandRun = true; // Set the flag to indicate that a Selector command has run
    setLEDColor(1, 1); // And indicate that this is a Selector command by setting the LED amber
    if(commandBuffer[4] == 0xFF and commandBuffer[5] == 0xFF){ // If the retry count and spare threshold are 0xFF, then we want to replace the Selector with a spare from the rescue folder
      Serial.println(" - Selector: Beginning 'Selector Rescue' procedure...");
      int replacementIndex = 0;
      char buffer[30];
      sprintf(buffer, "profile-backup-%d.image", replacementIndex); // First we want to backup the corrupted Selector image
      while(rootDir.exists(buffer)){ // So look for the first N for which "profile-backup-N.image" doesn't already exist
        replacementIndex++;
        sprintf(buffer, "profile-backup-%d.image", replacementIndex);
      }
      Serial.print("Backing up current Selector to ");
      Serial.print(buffer);
      Serial.println("...");
      disk.close();
      if(!sourceFile.open("profile.image", O_RDWR)){ // Now open profile.image as the source file
        Serial.println("Error opening current profile.image!");
      }
      else if(!sourceFile.rename(buffer)){ // And rename it to the backup name
        Serial.println("Error creating backup of current profile.image!");
      }
      else if(!sourceFile.close() or !sourceFile.open("/rescue/selector.image", O_RDONLY)){ // Then close it and open the backup Selector image
        Serial.println("Error opening backup selector image!");
      }
      else if(!destFile.createContiguous("profile.image", sourceFile.fileSize())){ // Now create a new profile.image file with the same size as the backup Selector image
        Serial.println("Error creating new profile.image!");
      }
      else{
        size_t n;
        while ((n = sourceFile.read(buf, sizeof(buf))) > 0){ // And copy the backup Selector image into the new profile.image
          destFile.write(buf, n);
        }
        nonce = EEPROM.read(5); // Finally, increment the nonce and write it back to EEPROM
        EEPROM.write(5, nonce + 1);
        EEPROM.commit();
        Serial.println("Done with Selector Rescue procedure!");
      }
      sourceFile.close(); // Close both the source and destination files
      destFile.close();
      if(!disk.open("profile.image", O_RDWR)){ // And then reopen the new profile.image as our current disk image
        Serial.println("Failed to reopen profile.image!");
      }
      updateSpareTable(); // Update the spare table to reflect the attributes of the new disk image
    }
    // Otherwise, if the upper nibble of the retry count is 0, then we want to send the host a block of a backup Selector ProFile image
    else if((commandBuffer[4] >> 4) == 0x00){
      blockNum = ((commandBuffer[4] & B00001111) << 8 | commandBuffer[5]) * 532; // The block that we want to send is formed from the lower nibble of the retry count and the full spare threshold
      if(!sourceFile.open("/rescue/selector.image", O_RDONLY)){ // Open the backup Selector image
        Serial.println(" - Selector: Error opening backup Selector ProFile image!");
      }
      else if((blockNum + 531) >= sourceFile.fileSize()){ // And make sure that the block we want to send isn't past the end of the file
        Serial.println(" - Selector Error: Block is past the end of the file!");
        for(int i = 0; i < 532; i++){ // If it is, fill the block data buffer with zeros
          blockData[i+readStatusOffset] = 0x00;
        }
      }
      // Otherwise, read the block out of the image
      else{
        disk.seekSet(blockNum); // Seek to the block in the disk image
        sourceFile.read(blockData+readStatusOffset, 532); // And read the block into the block data buffer
        Serial.print(" - Selector: Sending block ");
        printDataNoSpace(commandBuffer[4] & B00001111);
        printDataNoSpace(commandBuffer[5]);
        Serial.println(" of backup Selector ProFile image!");
      }
      sourceFile.close();
    }
    // If the upper nibble of the retry count is 0x01, then we want to send the host a block of a backup Selector 3.5 image
    // The actual process is identical to the above, just with a different image
    else if((commandBuffer[4] >> 4) == 0x01){
      blockNum = ((commandBuffer[4] & B00001111) << 8 | commandBuffer[5]) * 532;
      if(!sourceFile.open("/rescue/selector.3.5inch.dc42", O_RDONLY)){
        Serial.println(" - Selector: Error opening backup Selector 3.5 image!");
      }
      else if((blockNum + 531) >= sourceFile.fileSize()){
        Serial.println(" - Selector Error: Block is past the end of the file!");
        for(int i = 0; i < 532; i++){
          blockData[i+readStatusOffset] = 0x00;
        }
      }
      else{
        disk.seekSet(blockNum);
        sourceFile.read(blockData+readStatusOffset, 532);
        Serial.print(" - Selector: Sending block ");
        printDataNoSpace(commandBuffer[4] & B00001111);
        printDataNoSpace(commandBuffer[5]);
        Serial.println(" of backup Selector 3.5 DC42 image...");
      }
      sourceFile.close();
    }
    // If the upper nibble of the retry count is 0x02, then we want to send the host a block of a backup Selector Twiggy image
    // Once again, other than a different filename, the process is identical to the above
    else if((commandBuffer[4] >> 4) == 0x02){
      blockNum = ((commandBuffer[4] & B00001111) << 8 | commandBuffer[5]) * 532;
      if(!sourceFile.open("/rescue/selector.twiggy.dc42", O_RDONLY)){
        Serial.println(" - Selector: Error opening backup Selector Twiggy image!");
      }
      else if((blockNum + 531) >= sourceFile.fileSize()){
        Serial.println(" - Selector Error: Block is past the end of the file!");
        for(int i = 0; i < 532; i++){
          blockData[i+readStatusOffset] = 0x00;
        }
      }
      else{
        disk.seekSet(blockNum);
        sourceFile.read(blockData+readStatusOffset, 532);
        Serial.print(" - Selector: Sending block ");
        printDataNoSpace(commandBuffer[4] & B00001111);
        printDataNoSpace(commandBuffer[5]);
        Serial.println(" of backup Selector Twiggy DC42 image...");
      }
      sourceFile.close();
    }
    // If the upper nibble of the retry count is something else, then the command is invalid
    else{
      Serial.println(" - Selector: Bad 'Selector Rescue' command!");
      for(int i = 0; i < 532; i++){ // So fill the entire block with zeros
        blockData[i+readStatusOffset] = 0x00;
      }
    }
  }
  // If we end up here, then our command isn't a Selector command; it's just a good old-fashioned read
  else if(blockNum < disk.fileSize()){ // Check that the block number is within the range of the disk image
    blockNum *= 532;
    disk.seekSet(blockNum); // Seek to our block
    disk.read(blockData+readStatusOffset, 532); // And read the block into the block data buffer
    Serial.println();
  }
  // Otherwise, if the block is outside the range of the disk image, fill the block data buffer with zeros
  else{
    Serial.println(" - Error: Requested block is out of range!");
    for(int i = 0; i < 532; i++){
      blockData[i+readStatusOffset] = 0x00;
    }
  }

  setParallelDir(1); // Now set the bus to output mode
  //delayMicroseconds(1);
  for(int i = 0; i < readStatusOffset; i++){ // And set the 4 status bytes in blockData to 0x00 (no errors)
    blockData[i] = 0x00;
  }
  bufferIndex = 0; // Reset the blockData buffer index
  sendParity(blockData[bufferIndex]); // Put the parity for the first byte on the bus
  sendData(blockData[bufferIndex++]); // Followed by the byte itself, and increment the buffer index

  noInterrupts(); // Now we're going to send the rest of the block and care about speed, so disable interrupts

  clearBSY(); // And raise BSY to tell the host that we're ready

  while(bitRead(REG_READ(GPIO_IN_REG), CMDPin) == 1){ // Stay in the data-sending loop until the host lowers CMD
    currentState = bitRead(REG_READ(GPIO_IN_REG), STRBPin); // Read the current state of STRB
    if(currentState == 0 and prevState == 1){ // If we see a falling edge on STRB, send the next byte of the block
      sendParity(blockData[bufferIndex]); // Send the parity for the byte
      REG_WRITE(GPIO_OUT_W1TS_REG, blockData[bufferIndex] << busOffset); // Then write the byte into W1TS to set all the bits that need to be set
      REG_WRITE(GPIO_OUT_W1TC_REG, ((byte)~blockData[bufferIndex++] << busOffset)); // Then write the inverse of the byte into W1TC to clear all the bits that need to be cleared, and increment the buffer index
    }
    prevState = currentState; // Update the previous state of STRB
  }
  interrupts(); // We're done with the read operation now, so re-enable interrupts
}

// Process and execute a write command
void writeDrive(){
  setParallelDir(1); // Set the bus to output mode
  //delayMicroseconds(1);
  sendData(commandBuffer[0] + 0x02); // Acknowledge the write command by sending the command value + 2
  setBSY(); // And lower BSY to tell the host that we've acknowledged the command
  printCommand(); // Now that we're in control of the pace of the bus, print the command that we're executing
  currentTime = 0;
  while(readCMD() == 0){ // Wait for the host to raise CMD and timeout if it doesn't
    currentTime++;
    if(currentTime >= timeout){
      Serial.println(" - Timeout: Write Command Confirmation");
      return;
    }
  }
  setParallelDir(0); // Set the bus to input mode
  //delayMicroseconds(1);
  currentTime = 0;
  while(receiveData() != 0x55){ // And wait for the host to respond with an 0x55; timeout if it doesn't
    currentTime++;
    if(currentTime >= timeout){
      Serial.println(" - Write phase 2: Host didn't respond with a 55!");
      return;
    }
  }

  bufferIndex = 0; // Reset the blockData buffer index
  noInterrupts(); // It's time to receive the data block that we need to write, so disable interrupts
  clearBSY(); // Raise BSY to tell the host that we're ready to receive the data
 
  while(bitRead(REG_READ(GPIO_IN_REG), CMDPin) == 1){ // Stay in the data-receiving loop until the host lowers CMD
    currentState = bitRead(REG_READ(GPIO_IN_REG), STRBPin); // Read the current state of STRB
    if(currentState == 0 and prevState == 1){ // If we see a falling edge on STRB, it means the data is valid
      blockData[bufferIndex] = REG_READ(GPIO_IN_REG) >> busOffset; // So read the data from the bus into the blockData buffer
      sendParity(blockData[bufferIndex++]); // And send the appropriate parity for this byte, followed by incrementing the buffer index
    }
    prevState = currentState; // Update the previous state of STRB
  }

  interrupts(); // We're done with the time-sensitive part, so re-enable interrupts

  setParallelDir(1); // Set the bus to output mode
  //delayMicroseconds(1);
  sendData(0x06); // Acknowledge the successful reception of the data block by sending an 0x06
  setBSY(); // And lower BSY to tell the host about our acknowledgement
  currentTime = 0;
  while(readCMD() == 0){ // Wait for the host to raise CMD and timeout if it doesn't
    currentTime++;
    if(currentTime >= timeout){
      //Serial.println("Timeout: Write Command Second Handshake Part 2");
      return;
    }
  }
  setParallelDir(0); // Set the bus to input mode
  //delayMicroseconds(1);
  currentTime = 0;
  while(receiveData() != 0x55){ // And wait for the host to respond with an 0x55; timeout if it doesn't
    currentTime++;
    if(currentTime >= timeout){
      Serial.println(" - Write phase 3: Host didn't respond with a 55!");
      return;
    }
  }
  bool halt = false; // This flag will be set if the host wants to halt the emulator
  blockNum = (commandBuffer[1] << 16 | commandBuffer[2] << 8 | commandBuffer[3]); // Form a block number from bytes 1-3 of the command buffer
  // If the block number is 0xFFFFFD, the retry count is 0xFE, and the spare threshold is 0xAF, then it's a built-in Selector command
  if(commandBuffer[1] == 0xFF and commandBuffer[2] == 0xFF and commandBuffer[3] == 0xFD and commandBuffer[4] == 0xFE and commandBuffer[5] == 0xAF){
    selectorCommandRun = true; // Set the flag indicating that a Selector command has been run
    // If the first four bytes in the received block are "HALT", then the host wants to halt the emulator
    if(blockData[0] == 0x48 and blockData[1] == 0x41 and blockData[2] == 0x4C and blockData[3] == 0x54){
      halt = true; // Set the halt flag
      Serial.println(" - Selector: Halting emulator...");
      setLEDColor(1, 0); // And set the LED red to indicate that the ESProFile is halted
    }
    // If the first six bytes in the received block are "IMAGE:", then the host wants to switch to a different image file
    if(blockData[0] == 0x49 and blockData[1] == 0x4D and blockData[2] == 0x41 and blockData[3] == 0x47 and blockData[4] == 0x45 and blockData[5] == 0x3A){
      setLEDColor(1, 1); // Set the LED to amber to indicate that we're running a Selector command
      Serial.print(" - Selector: Switching to image file ");
      int i = 6;
      while(1){ // Read the desired filename out of the block data
        if(blockData[i] == 0x00){ // Break once we hit a null terminator
          fileName[i - 6] = 0x00;
          break;
        }
        fileName[i - 6] = blockData[i]; // Put each character into the filename buffer
        Serial.write(blockData[i]);
        i++;
      }
      Serial.print("...");
    }
    Serial.println();
    disk.close(); // Close the current disk image
    if(!disk.open(fileName, O_RDWR)){ // And try to open the new disk image
      Serial.print(" - Error opening image file!");
      disk.close(); // If we can't open the new disk image, close it again
      if(!disk.open("profile.image", O_RDWR)){ // And try to reopen the old one
        Serial.print(" And failed to reopen profile.image!");
      }
      updateSpareTable(); // Update the spare table to reflect the attributes of the new disk image after the failed swap
    }
    else{
      updateSpareTable(); // And do the same on success as well
    }
  }
  // If the block number is 0xFFFEFF, then it's a Selector key-value store command
  // As with reading from the key-value store, only writing to the autoboot and moniker fields is supported; anuthing else just won't do anything
  else if(commandBuffer[1] == 0xFF and commandBuffer[2] == 0xFE and commandBuffer[3] == 0xFF){
    selectorCommandRun = true; // Set the flag indicating that a Selector command has been run
    setLEDColor(1, 1); // Set the LED to amber to indicate that we're running a Selector command
    // If the retry count and spare threshold are 0xFF, then the host wants to write to a special location in the key-value store
    // The Selector always does this on boot for some reason, but it doesn't seem to do anything, so we just handle it with a non-error message
    if(commandBuffer[4] == 0xFF and commandBuffer[5] == 0xFF){ 
      Serial.println(" - Selector: Writing to key-value store...");
    }
    // If the retry count and spare threshold are 0x53 and 0x43, then the host wants to write to the device moniker field
    else if(commandBuffer[4] == 0x53 and commandBuffer[5] == 0x43){
      Serial.println(" - Selector: Updating device moniker...");
      for(int i = 0; i < 532; i++){ // So write the block data into the EEPROM at the moniker location
        EEPROM.write(KVMoniker + i, blockData[i]);
      }
      EEPROM.commit();
    }
    // If the retry count and spare threshold are 0x53 and 0x61, then the host wants to write to the device autoboot field
    else if(commandBuffer[4] == 0x53 and commandBuffer[5] == 0x61){
      Serial.println(" - Selector: Updating device autoboot script...");
      for(int i = 0; i < 532; i++){ // So write the block data into the EEPROM at the autoboot location
        EEPROM.write(KVAutoboot + i, blockData[i]);
      }
      EEPROM.commit();
    }
    // Otherwise, it's a write to an unsupported key-value store location
    else{
      Serial.println(" - Selector Error: Unsupported key-value write operation!");
    }
  }
  // If the block number is 0xFFFEFE, then it's a Selector filesystem command of some kind
  else if(commandBuffer[1] == 0xFF and commandBuffer[2] == 0xFE and commandBuffer[3] == 0xFE){
    selectorCommandRun = true; // Set the flag indicating that a Selector command has been run
    setLEDColor(1, 1); // Set the LED to amber to indicate that we're running a Selector command
    // If the retry count and spare threshold are 0x63 and 0x70, then the host wants to copy a file
    if(commandBuffer[4] == 0x63 and commandBuffer[5] == 0x70){
      Serial.print(" - Selector: Copying ");
      int i = 0;
      while(1){ // Read the source filename out of the block data
        if(blockData[i] == 0x00){ // Break once we hit a null terminator
          fileName[i] = 0x00;
          break;
        }
        fileName[i] = blockData[i]; // Put each character into the filename buffer
        Serial.write(blockData[i]);
        i++;
      }
      i++;
      if(!sourceFile.open(fileName, O_RDONLY)){ // Now try to open the source file
        Serial.println(" (Error opening source file)");
      }
      else{
        Serial.print(" to ");
        int offset = i;
        while(1){ // Now we need to read the destination filename out of the block data
          if(blockData[i] == 0x00){ // Break once we hit a null terminator
            fileName[i - offset] = 0x00;
            break;
          }
          fileName[i - offset] = blockData[i]; // Put each character into the filename buffer
          Serial.write(fileName[i - offset]);
          i++;
        }
        Serial.print(" with size ");
        Serial.print(sourceFile.fileSize());
        Serial.print("; this may take a while and the Selector may time out...");
        if(!destFile.createContiguous(fileName, sourceFile.fileSize())){ // Create the destination file with the same size as the source file
          Serial.println(" Error creating destination file");
        }
        else{
          size_t n;
          int startTime = millis(); // Then start a timer so we can tell the user how long the copy took
          // Now copy the source file into the destination file
          while ((n = sourceFile.read(buf, sizeof(buf))) > 0){
            destFile.write(buf, n);
          }
          double totalTime = (millis() - startTime)/1000.0; // And calculate the total time it took
          nonce = EEPROM.read(5); // And increment the nonce and write it back to EEPROM
          EEPROM.write(5, nonce + 1);
          EEPROM.commit();
          // Print out a success message with the time the copy took and the data rate
          Serial.printf(" Success! The copy took %f seconds with a data rate of %f KB/s.\n", totalTime, (sourceFile.fileSize()/totalTime)/1024);
        }
      }
      sourceFile.close(); // Then close both the source and destination files
      destFile.close();
    }
    // If the retry count and spare threshold are 0x6D and 0x6B, then the host wants to create a new "normal" image of size 5MB
    else if(commandBuffer[4] == 0x6D and commandBuffer[5] == 0x6B){
      setLEDColor(1, 1); // Set the LED to amber to indicate that we're running a Selector command
      Serial.print(" - Selector: Creating new 5MB image called ");
      int i = 0;
      while(1){ // Read the filename out of the block data
        if(blockData[i] == 0x00){ // Break once we hit a null terminator
          fileName[i] = 0x00;
          break;
        }
        fileName[i] = blockData[i]; // Put each character into the filename buffer
        i++;
      }
      Serial.print("...");
      if(!destFile.createContiguous(fileName, 5175296)){ // Create the new empty image with the appropriate size
        Serial.print(" Error creating destination file!");
      }
      Serial.println();
      nonce = EEPROM.read(5); // And increment the nonce and write it back to EEPROM
      EEPROM.write(5, nonce + 1);
      EEPROM.commit();
      destFile.close(); // Then close the new image file
    }
    // If the retry count is 0x6D and the spare threshold is 0x78, then the host wants to create a new "extended" image of a user-specified size
    else if(commandBuffer[4] == 0x6D and commandBuffer[5] == 0x78){
      setLEDColor(1, 1); // Set the LED to amber to indicate that we're running a Selector command
      Serial.print(" - Selector: Creating new image of size ");
      int i = 0;
      while(1){ // Read the size out of the block data
        if(blockData[i] == 0x00){ // Break once we hit a null terminator
          fileName[i] = 0x00;
          break;
        }
        fileName[i] = blockData[i]; // Put each character into the filename buffer (I know it's called fileName, but we're using it for the size here)
        i++;
      }
      uint32_t size = strtol(fileName, NULL, 10); // Convert the size to an integer
      Serial.print(size);
      i++;
      int offset = i;
      Serial.print(" called ");
      while(1){ // Now read the filename out of the block data
        if(blockData[i] == 0x00){ // Break once we hit a null terminator
          fileName[i - offset] = 0x00;
          break;
        }
        fileName[i - offset] = blockData[i]; // Put each character into the filename buffer
        Serial.write(fileName[i - offset]);
        i++;
      }
      Serial.print("...");
      if(!destFile.createContiguous(fileName, size)){ // Create the new empty image with the appropriate size
        Serial.print(" Error creating destination file!");
      }
      Serial.println();
      nonce = EEPROM.read(5); // And increment the nonce and write it back to EEPROM
      EEPROM.write(5, nonce + 1);
      EEPROM.commit();
      destFile.close(); // Finally, close the new image file
    }
    // If the retry count is 0x72 and the spare threshold is 0x6D, then the host wants to delete a file
    else if(commandBuffer[4] == 0x72 and commandBuffer[5] == 0x6D){
      setLEDColor(1, 1); // Set the LED to amber to indicate that we're running a Selector command
      int i = 0;
      Serial.print(" - Selector: Deleting file ");
      while(1){ // Read the filename out of the block data
        if(blockData[i] == 0x00){ // Break once we hit a null terminator
          fileName[i] = 0x00;
          break;
        }
        fileName[i] = blockData[i]; // Put each character into the filename buffer
        Serial.write(blockData[i]);
        i++;
      }
      Serial.print("...");
      if(!rootDir.remove(fileName)){ // Try to delete the file
        Serial.print(" Failed to delete file!");
      }
      Serial.println();
      nonce = EEPROM.read(5); // Then increment the nonce and write it back to EEPROM
      EEPROM.write(5, nonce + 1);
      EEPROM.commit();
    }
    // If the retry count is 0x6D and the spare threshold is 0x76, then the host wants to move AKA rename a file
    else if(commandBuffer[4] == 0x6D and commandBuffer[5] == 0x76){
      setLEDColor(1, 1); // Set the LED to amber to indicate that we're running a Selector command
      Serial.print(" - Selector: Renaming ");
      int i = 0;
      while(1){ // Read the source filename out of the block data
        if(blockData[i] == 0x00){ // Break once we hit a null terminator
          fileName[i] = 0x00;
          break;
        }
        fileName[i] = blockData[i]; // Put each character into the filename buffer
        Serial.write(blockData[i]);
        i++;
      }
      i++;
      if(!sourceFile.open(fileName, O_RDWR)){ // Try to open the source file
        Serial.println();
        Serial.println(" (Error opening source file)");
      }
      else{
        Serial.print(" to ");
        int offset = i;
        while(1){ // Read the destination filename out of the block data
          if(blockData[i] == 0x00){ // Break once we hit a null terminator
            fileName[i - offset] = 0x00;
            break;
          }
          fileName[i - offset] = blockData[i]; // Put each character into the filename buffer
          Serial.write(blockData[i]);
          i++;
        }
        Serial.print("...");
        if(!sourceFile.rename(fileName)){ // Try to rename the source file to the destination filename
          Serial.print(" Error renaming file!");
        }
        Serial.println();
      }
      sourceFile.close(); // Then close the source file
      nonce = EEPROM.read(5); // And increment the nonce and write it back to EEPROM
      EEPROM.write(5, nonce + 1);
      EEPROM.commit();
    }
    // If the retry count is 0x73 and the spare threshold is 0x78, then the host wants to set the file extension used by the emulator
    else if(commandBuffer[4] == 0x73 and commandBuffer[5] == 0x78){
      setLEDColor(1, 1); // Set the LED to amber to indicate that we're running a Selector command
      int i = 0;
      while(1){ // Read the extension out of the block data
        if(blockData[i] == 0x00){ // Break once we hit a null terminator
          extension[i] = 0x00;
          break;
        }
        extension[i] = blockData[i]; // Put each character into the extension buffer, for use by other parts of the emulator
        i++;
      }
      Serial.print(" - Selector: Changing file extension to ");
      Serial.print(extension);
      Serial.println("...");
      nonce = EEPROM.read(5); // Then increment the nonce and write it back to EEPROM
      EEPROM.write(5, nonce + 1);
      EEPROM.commit();
    }
  }
  // If we end up here, then the command is just a good old-fashioned write
  else if(blockNum < disk.fileSize()){ // So check that the block number is within the range of the disk image
    blockNum *= 532;
    disk.seekSet(blockNum); // Seek to the block in the disk image
    disk.write(blockData, 532); // Write the block data into the disk image
    disk.flush(); // And flush the disk image to make sure the write is complete
    Serial.println();
  }
  // If the block is outside the range of the disk image, print an error message
  else{
    Serial.println(" - Error: Requested block is out of range!");
  }
  setParallelDir(1); // Now set the bus to output mode
  //delayMicroseconds(1);

  // Fill the four status bytes in blockData with zeros (no errors)
  for(int i = writeStatusOffset; i < 536; i++){
    blockData[i] = 0x00;
  }

  bufferIndex = 532; // Set the buffer index to the first status byte
  sendParity(blockData[bufferIndex]); // Put its parity info on the PARITY line
  sendData(blockData[bufferIndex++]); // And put the status byte on the bus, incrementing the buffer index afterwards

  noInterrupts(); // We're about to send the remaining status bytes, so disable interrupts just to be safe
  clearBSY(); // Raise BSY to tell the host that we're ready to continue

  while(bitRead(REG_READ(GPIO_IN_REG), CMDPin) == 1){ // Stay in the byte-sending loop until the host lowers CMD
    currentState = bitRead(REG_READ(GPIO_IN_REG), STRBPin); // Read the current state of STRB
    if(currentState == 0 and prevState == 1){ // If we see a falling edge on STRB, send the next status byte
      sendParity(blockData[bufferIndex]); // Set the parity data for the status byte
      REG_WRITE(GPIO_OUT_W1TS_REG, blockData[bufferIndex] << busOffset); // Then write the byte into W1TS to set all the bits that need to be set
      REG_WRITE(GPIO_OUT_W1TC_REG, ((byte)~blockData[bufferIndex++] << busOffset)); // And write the inverse of the byte into W1TC to clear all the bits that need to be cleared, incrementing the buffer index afterwards
    }
    prevState = currentState; // Update the previous state of STRB
  }
  if(halt == true){ // If we set the halt flag earlier, then we need to halt the ESProFile here
    while(1);
  }
}

// Prints whatever command the drive is currently executing
void printCommand() {
  // Old/simple way of printing raw commands for debugging
  /*for(int i = 0; i < 6; i++){
    printDataNoSpace(commandBuffer[i]);
  }*/

  // New way to print the command in a more human-readable format
  if(commandBuffer[0] == 0x00){ // Read command
    Serial.print("Read  Block: ");
  }
  else if(commandBuffer[0] == 0x01 or commandBuffer[0] == 0x02 or commandBuffer[0] == 0x03){ // Write command
    Serial.print("Write Block: ");
  }
  else{ // Unknown/bad command
    Serial.print("Unknown Command - Block: ");
  }
  printDataNoSpace(commandBuffer[1]); // Now print the block number
  printDataNoSpace(commandBuffer[2]);
  printDataNoSpace(commandBuffer[3]);

  // We used to print the retry count and spare threshold, but it's kind of unnecessary for an emulator and wastes transmission time
  /*Serial.print("        Retry Count: "); // The retry count
  printDataNoSpace(commandBuffer[4]);
  Serial.print("        Spare Threshold: "); // And the spare threshold
  printDataNoSpace(commandBuffer[5]);*/
}

// Set all pins to the correct initial direction and state
void initPinsEmulator(){
  for(int i = busOffset; i < busOffset + 8; i++){ // Make the bus an input
    pinMode(i, INPUT);
  }
  // And make everything else inputs too, except BSY, PARITY, and the LEDs
  pinMode(CMDPin, INPUT);
  pinMode(BSYPin, OUTPUT);
  pinMode(RWPin, INPUT);
  pinMode(STRBPin, INPUT);
  pinMode(PRESPin, INPUT);
  pinMode(PARITYPin, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  // Set both BSY and PARITY to their default states (high)
  clearBSY();
  clearPARITY();
}

// Calculates the (odd) parity of a byte and sends it out over the PARITY pin; inline since it's called in a part of the code where speed is critical
inline __attribute__((__always_inline__)) void sendParity(uint8_t data){
  if(parityArray[data] == 0){ // If the parity of the data is 0, set the PARITY pin high
    clearPARITY();
  }
  else{ // Otherwise, set the PARITY pin low
    setPARITY();
  }
  /*if(__builtin_parity(data) == 0){ // Use the built-in parity function and clear the parity pin (set high) if the result is 0
    clearPARITY();
  }
  else{ // If it's 1, set parity (set low)
    setPARITY();
  }*/
}

// Updates the spare table based on the attributes of the currently-loaded disk image
void updateSpareTable(){
  // If the image is 5MB, then just use the ready-made 5MB ProFile spare table
  if(disk.fileSize() == 5175296){
    spareTable[8] = 0x20; // Name
    spareTable[9] = 0x20; // Name
    spareTable[10] = 0x20; // Name
    spareTable[15] = 0x00; // Device number
    spareTable[16] = 0x03; // ROM revision
    spareTable[17] = 0x98; // ROM revision
    spareTable[18] = 0x00; // Drive size
    spareTable[19] = 0x26; // Drive size
    spareTable[20] = 0x00; // Drive size
    Serial.println("Switching to the 5MB ProFile spare table.");
  }
  // If the image is 10MB, then just use the ready-made 10MB ProFile spare table
  else if(disk.fileSize() == 10350592){
    spareTable[8] = 0x31; // Name
    spareTable[9] = 0x30; // Name
    spareTable[10] = 0x4D; // Name
    spareTable[15] = 0x10; // Device number
    spareTable[16] = 0x04; // ROM revision
    spareTable[17] = 0x04; // ROM revision
    spareTable[18] = 0x00; // Drive size
    spareTable[19] = 0x4C; // Drive size
    spareTable[20] = 0x00; // Drive size
    Serial.println("Switching to the 10MB ProFile spare table.");
  }
  // Otherwise, it's a non-standard size, so we must insert the size manually
  else{
    spareTable[8] = 0x20; // Name
    spareTable[9] = 0x20; // Name
    spareTable[10] = 0x20; // Name
    spareTable[15] = 0x00; // Device number
    spareTable[16] = 0x03; // ROM revision
    spareTable[17] = 0x98; // ROM revision
    spareTable[18] = (disk.fileSize()/532) >> 16; // Drive size
    spareTable[19] = (disk.fileSize()/532) >> 8; // Drive size
    spareTable[20] = (disk.fileSize()/532); // Drive size
    Serial.print("Switching to a custom spare table for drive of size ");
    printDataNoSpace(spareTable[18]); // Print the size too
    printDataNoSpace(spareTable[19]);
    printDataNoSpace(spareTable[20]);
    Serial.println(" blocks.");
  }
}

// All of these functions just make it easier to set, clear, and read the control signals for the drive
// Remember, all signals are active low
inline __attribute__((__always_inline__)) void setBSY(){
  setLEDColor(0, 0); // Setting BSY is special because it also turns off the LEDs
  REG_WRITE(GPIO_OUT_W1TC_REG, 0b1 << BSYPin); // In addition to setting BSY low
}

inline __attribute__((__always_inline__)) void clearBSY(){
  setLEDColor(0, 1); // Same for clearing BSY; we have to turn the green LED on
  REG_WRITE(GPIO_OUT_W1TS_REG, 0b1 << BSYPin); // As well as setting BSY high
}

// The other set/clear functions don't bother with the LEDs
// The parity functions are inline because they're called in a part of the code where speed is critical
inline __attribute__((__always_inline__)) void setPARITY(){
  REG_WRITE(GPIO_OUT_W1TC_REG, 0b1 << PARITYPin);
}

inline __attribute__((__always_inline__)) void clearPARITY(){
  REG_WRITE(GPIO_OUT_W1TS_REG, 0b1 << PARITYPin);
}

// The read functions just return the state of their corresponding control signals
inline __attribute__((__always_inline__)) bool readCMD(){
  return bitRead(REG_READ(GPIO_IN_REG), CMDPin);
}

inline __attribute__((__always_inline__)) bool readRW(){
  return bitRead(REG_READ(GPIO_IN_REG), RWPin);
}

inline __attribute__((__always_inline__)) bool readSTRB(){
  return bitRead(REG_READ(GPIO_IN_REG), STRBPin);
}

inline __attribute__((__always_inline__)) bool readPRES(){
  return bitRead(REG_READ(GPIO_IN_REG), PRESPin);
}