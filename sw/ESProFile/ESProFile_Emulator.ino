//***********************************************************************************
//* ESProFile ProFile Emulator Software v1.6                                        *
//* By: Alex Anderson-McLeod                                                        *
//* Email address: alexelectronicsguy@gmail.com                                     *
//***********************************************************************************
#define APPLE_III_ONLY 1
#define USE_LEDS 1 // If I turn these off time of directory list is 16.5 seconds, on it is 8.5 seconds. Using the LEDs make it faster.
// ******** Changelog ********
// 2/12/2025  - v1.1 - Fixed an issue where printing debug information over serial would sometimes cause read errors when using ESProFile under LOS 3.0 with a 2-port parallel card.
// 2/22/2025  - v1.2 - Improved performance by about 30% (including during Selector copy operations) by making some tweaks to the SPI initialization routines, copy buffer size, and inlining some functions.
// 11/14/2025 - v1.3 - Fixed a bug where ESProFile wouldn't respond in time to satisfy the super-short timeout period of Rev. C and earlier Lisa boot ROMs, as well as a bug where a botched LOS 1.0 shutdown under the Rev. C ROMs would lead to an Error 85 on the next boot attempt.
// 4/19/2026  - v1.4 - Added support for pin definition header files to allow easy customization of ESProFile for different board layouts, and used this to create the LisaFPGA variant of ESProFile. Also cached the ProFile read/write routines to make them fast enough for LisaFPGA's 75MHz DOTCK mode.
// 8/21/2026  - v1.5 - Improved SD card access speeds by a factor of about 5 on average with the DUSE_SPI_ARRAY_TRANSFER build option.
// 9/29/2026  - v1.6 - Works now on the "Apple ///" and with the "Titan /// Plus IIe" and "SoftCard ///" if you build with APPLE_III_ONLY set to 1.
//                     Tested with the three types of .profile drivers I could find. .prof[#], .profile, and .cpm[#] on the Apple ///.
//                     Tested, made some slight fixes, and now it works on the Apple IIe Profile card! Even boots from ProDOS 2.4.2
//                     (* Gary Desrochers *)

// Let's get one thing out of the way. If you are looking at the code and see indented #if <value> statements and are wondering why they are not all
// the way to the left then let me explain.  The Arduino IDE at the point of writing this program has, what I would call, two weird bugs.
// 1) When opening and closing blocks the #if <value> are considered the end of the block if the #if <value> is all the way to the left.  This causes
//    the open and closing of sections of code to be wrong and you cannot close a block fully, for instance a while block, if there is a #if <value>
//    within the while block and the #if <value> is all the way to the left.
// 2) This bug is not a reason for the #if <value> statements to be to the right but it is what I still consider a bug.  Most other editors when
//    those editors see an "#if 0" know to grey out that section.  No clue why this IDE editor does not do that.  Kind of obnoxious.
// Alex, if you want to remove this block of comments then go right ahead. I am fine with that but wanted you to know why I used indented #if <value> statements.

#define EMULATOR_VERSION "1.6" // The version of the ESProFile emulator software; this gets printed over serial at startup

const size_t READ_STATUS_OFFSET = 4; // Status bytes are bytes 0-3 of blockData during a read
const size_t PROFILE_BLOCK_SIZE = 532;
const size_t BLOCKDATA_SIZE = 536; // Four status bytes plus the profile block size for max.
const size_t APPLE_III_WRITE_SIZE = 512; // Apple /// only handles 512 bytes of the Profile drive block
const size_t COMMAND_BUFFER_SIZE = 6;
const size_t STATUS_BUFFER_SIZE = 4;
const size_t CHECK_BYTE_BUFFER_SIZE = 6;
const size_t FILE_NAME_BUFFER_SIZE = 256;
const size_t FILE_EXTENSION_BUFFER_SIZE = 255;
#if APPLE_III_ONLY
const size_t PROFILE_SPARE_TABLE_SIZE = 21;
#else
const size_t PROFILE_SPARE_TABLE_SIZE = 48;
#endif

#define KVMoniker 1024 // The locations of the Selector moniker and autoboot flag in the ESP32's EEPROM
#define KVAutoboot 1557

const size_t timeout = 10000000; // Timeout value (in loop iterations) for all phases of ProFile comms except the initial handshake
const size_t handshakeTimeout = 1000000; // Timeout value for the initial handshake phase (should be shorter since the host intentionally times out here to detect the drive)

SPIClass SD_SPI(HSPI); // These two lines make sure that we use hardware SPI at 20MHz for the SD card
#define SD_CONFIG SdSpiConfig(SD_CS, DEDICATED_SPI, SD_SCK_MHZ(20), &SD_SPI)

uint8_t blockData[BLOCKDATA_SIZE]; // The ProFile block that we're currently reading/writing, with status bytes too
uint8_t commandBuffer[COMMAND_BUFFER_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // The 6-byte command that the ProFile is currently executing
uint8_t checkBytesBuffer[CHECK_BYTE_BUFFER_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // The 6-byte returned after a write.
uint8_t statusBuffer[STATUS_BUFFER_SIZE] = {0x00, 0x00, 0x00, 0x00};
// The contents of the ProFile spare table
#if APPLE_III_ONLY
uint8_t spareTable[PROFILE_SPARE_TABLE_SIZE] = {0x52, 0x4F, 0x46, 0x49, 0x4C, 0x45, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x03, 0x98, 0x00, 0x26, 0x00, 0x02}; //the array that holds the spare table
#else
uint8_t spareTable[PROFILE_SPARE_TABLE_SIZE] = {0x50, 0x52, 0x4F, 0x46, 0x49, 0x4C, 0x45, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x03, 0x98, 0x00, 0x26, 0x00, 0x02, 0x14, 0x20, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x43, 0x61, 0x6D, 0x65, 0x6F, 0x2F, 0x41, 0x70, 0x68, 0x69, 0x64, 0x20, 0x30, 0x30, 0x30, 0x31}; //the array that holds the spare table
#endif
bool parityArray[256] = {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0}; // A lookup table for parity bits

uint32_t blockNum; // The block that the current operation is being performed on

uint16_t bufferIndex = 0; // Index into blockData and commandBuffer used during rapid comms with the Lisa
uint16_t checkBytesIndex = 0; // Index into the checkBytesBuffer
uint16_t statusBufferIndex = 0; // Index into the statusBuffer

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
char buf[65536]; // A buffer that we use when copying files

char fileName[FILE_NAME_BUFFER_SIZE]; // 256-character filename for disk images
char extension[FILE_EXTENSION_BUFFER_SIZE] = ".image"; // The extension that the Selector will use for disk images

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
  Serial.printf("ESProFile Emulator Mode - Version %s\n", EMULATOR_VERSION); // Print a welcome message
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
  noInterrupts(); // We need speed here and can't let FreeRTOS preempt us (can be up to 2ms of delay), so disable interrupts
}

void emulatorLoop() {
  clearBSY(); // Make sure we're not telling the host we're busy
  while(readCMD() == 1); // Wait for CMD to go low (the start of a ProFile handshake)
  setParallelDir(1); // Set the ProFile bus to output mode
  //delayMicroseconds(1);
 
  // When transfering bytes over a connection the parity should always be sent after the data settles.
  // This means that the data should be latched (or settles) and the parity should be sent.
  // This allows for the data to be retrieved first and then for the parity to be calculated and read and compared after that.
  sendData(0x01); // Send an 0x01 to the host
  sendParity(0x01); // And the appropriate parity bit for that value

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
  uint8_t response;
  while(((response = receiveData()) != 0x55) && (response != 0xAA)){ // And wait for the host to respond with an 0x55 or 0xAA; timeout if it doesn't
    // Fun fact: if we don't get a 55 here, it probably means that the host was just trying to see if the drive is there, but didn't care about actually doing a data transfer
    // The Lisa does this at boot time to see if a ProFile is present
    currentTime++;
    // Certain Lisa ROMs (like Rev. C) ask if the drive is there, but then proceed to send a command afterwards so fast that the timeount doesn't have time to trigger
    // So also break out of this loop if CMD goes low again, indicating the start of another command
    if(currentTime >= handshakeTimeout || readCMD() == 0){
      //Serial.println("Phase 1: Host didn't respond with a 55! Maybe the drive was reset?");
      return;
    }
  }

  // Don't forget to send parity no matter what it is.
  sendParity(response);

  // Could be an 0xAA which means it does not know what our response was.
  if (response == 0xAA){
      Serial.println("Host did not understand our response (0xAA returned).");
      return;
  }

  bufferIndex = 0; // Rewind to the start of the command buffer
  for(int i = 0; i < COMMAND_BUFFER_SIZE; i++){ // And zero the whole thing out, in case this command ends up being less than 6 bytes
    commandBuffer[i] = 0;
  }
  clearBSY(); // Raise BSY to show that we're ready to receive a command

  receiveCommand(); // Now receive the command from the host

  if(commandBuffer[0] == 0x00){
    readDrive(); // If the first byte of the command is 0, then it's a read command
  }
  
  else if(commandBuffer[0] == 0x01 or commandBuffer[0] == 0x02 or commandBuffer[0] == 0x03){
    writeDrive(); // If the first byte is a 1, 2, or 3, then it's a write command
    // The ESProFile emulator treats both writes and write-verifys exactly the same; it doesn't matter on solid-state media
  }
  #if APPLE_III_ONLY
  // Apple /// driver source says: DUMMY HANDSHAKE FOR PROFILE TO UPDATE
  // Is this needed to be different from below? No.
  // Just treating it differently because the driver does.
  else if(commandBuffer[0] == 0xFF){
    setParallelDir(1); // Set the bus to output mode

    // You might be asking why I am putting a 0x06 on the bus in this case. It is a valid question.
    // Fortunately, I know the answer to this question.  If you did not know, for almost every command that
    // the profile, possibly only my profile, drive sends/receives the last two bytes on the bus are
    // always 0xFF and then 0x06.  I do not know why that is but it looks like a length specifier for
    // all the other bytes sent. Here are some examples of the values always put on the bus vs the request
    // that I know of:
    // Reed Request - 0x00	0x00	0x00	0x00	0x0A	0x03 - trailing values on bus - 0xxx	0xFF	0xFF	0xFF	0xFF	0xFF	0xFF	0x06
    // Read Catch Up - 0xFF	0x00	0x00	0x02	0x0A	0x03 - trailing values on bus - 0x03	0x03	0x03	0x03	0x03	0x03	0xFF	0x06
    // Spare Table - 0x00	0xFF	0xFF	0xFF	0x0A	0x03 - trailing values on bus - None, based on table
    // Spare Table Catch Up - 0xFF	0x00	0x26	0x00	0x0A	0x03 - trailing values on bus - None based on table
    // Write Request - 0x02	0x00	0x00	0x0A	0x0A	0x03 - trailing values on bus - 0x00	0x00	0x00	0x00	0x00	0x00	0xFF	0x06
    // Wriet Catch Up - 0xFF	0x00	0x00	0x09	0x0A	0x03 - trailing values on bus - 0x0F	0xFF	0xFF	0xFF	0xFF	0xFF	0xFF	0x06
    // The importand of all of these is the Write Request.  The Write Request always strobes in the first four of these but all are there.
    // These values below are not strobed in.  They are just put on the bus to make the last value be the same as the real drive.
    // Also, the Write request always has either the 0x55 or 0xAA as its response.
    sendData(0x06); // Put a known value on the bus to show that we are done like the Profile Drive does.
    sendParity(0x06); // And the appropriate parity bit for that value

    delayMicroseconds(5); // Since there is no strobe request, just let it sit there.  The profile lets it sit there for 10us before moving on.

    printCommand(); // Print the command that we didn't understand
  }
  #endif
  else{ // If we get some other command, stop and wait for the next handshake from the host
    setParallelDir(1); // Set the bus to output mode
    //delayMicroseconds(1);
    // Don't use 0x55 or 0xAA as an invalid value on the bus but 0x5A should be garbage.
    sendData(0x5A); // Put an invalid value on the bus to show that we didn't understand the command
    sendParity(0x5A); // And the appropriate parity bit for that value
    // No one will care much though. Just move onto the main loop top. If the driver cares it will reset the drive.
    printCommand(); // Print the command that we didn't understand
  }
}

// This function sends multiple bytes of data in a row out of blockData, with the appropriate parity bits for each one
// We stick it in IRAM (and thus make it a separate function) to speed it up and avoid cache misses that would happen otherwise
// That wouldn't normally be a problem, but when LisaFPGA is running at 75MHz, timings are tight enough that cache misses break things
// The symptom is that the very first read op doesn't work, but subsequent ones do once this code is cached into IRAM
IRAM_ATTR void sendWriteStatusBytes() {
  prevState = 1;
  while(bitRead(REG_READ(CMD_IN_REG), CMDPin) == 1 && bitRead(REG_READ(PRES_IN_REG), PRESPin) == 1){ // Stay in the data-sending loop until the host lowers CMD or the drive is reset
    currentState = bitRead(REG_READ(STRB_IN_REG), STRBPin); // Read the current state of STRB
    if(currentState == 0 and prevState == 1){ // If we see a falling edge on STRB, send the next byte
      // STATUS_DATA_SIZE for this method is the max. Don't keep putting out more just wait for CMD to go down.
      if (statusBufferIndex < STATUS_BUFFER_SIZE)
      {
        REG_WRITE(BUS_W1TS_REG, statusBuffer[statusBufferIndex] << busOffset); // Then write the byte into W1TS to set all the bits that need to be set
        REG_WRITE(BUS_W1TC_REG, ((byte)~statusBuffer[statusBufferIndex] << busOffset)); // Then write the inverse of the byte into W1TC to clear all the bits that need to be cleared, and increment the buffer index
        // When transfering bytes over a connection the parity should always be sent after the data settles.
        // This means that the data should be latched (or settles) and the parity should be sent.
        // This allows for the data to be retrieved first and then for the parity to be calculated and read and compared after that.
        sendParity(statusBuffer[statusBufferIndex++]); // Send the parity for the byte
      }
    }
    prevState = currentState; // Update the previous state of STRB
  }
}

IRAM_ATTR void sendMultiData() {
  prevState = 1;
  while(bitRead(REG_READ(CMD_IN_REG), CMDPin) == 1 && bitRead(REG_READ(PRES_IN_REG), PRESPin) == 1){ // Stay in the data-sending loop until the host lowers CMD or the drive is reset
    currentState = bitRead(REG_READ(STRB_IN_REG), STRBPin); // Read the current state of STRB
    if(currentState == 1 and prevState == 0){ // If we see a rising edge on STRB, send the next byte
      #if !APPLE_III_ONLY
      sendParity(blockData[bufferIndex]); // Send the parity for the byte
      REG_WRITE(BUS_W1TS_REG, blockData[bufferIndex] << busOffset); // Then write the byte into W1TS to set all the bits that need to be set
      REG_WRITE(BUS_W1TC_REG, ((byte)~blockData[bufferIndex++] << busOffset)); // Then write the inverse of the byte into W1TC to clear all the bits that need to be cleared, and increment the buffer index
      #else
      // (APPLE_III_WRITE_SIZE+READ_STATUS_OFFSET) is the max. Don't keep putting out more just wait for CMD to go down.
      // On the Apple /// I was overflowing the whole blockData array.
      // Sometimes it was a pretty bad overflow of many bytes.
      // I think we should be putting some buffer overflow check bytes for
      // both sections of code and checking those sacrificial bytes randomly.
      // This is not really how much is sent but it is the max.  for instance, when something like the spare table is requested, only 25 bytes
      // are strobed in. Then cmd goes down and we stop.
      // One thing to always remember is that the buffer in the Profile Drive is a max of 256 bytes.
      if (bufferIndex < (APPLE_III_WRITE_SIZE+READ_STATUS_OFFSET+CHECK_BYTE_BUFFER_SIZE))
      {
        REG_WRITE(BUS_W1TS_REG, blockData[bufferIndex] << busOffset); // Then write the byte into W1TS to set all the bits that need to be set
        REG_WRITE(BUS_W1TC_REG, ((byte)~blockData[bufferIndex] << busOffset)); // Then write the inverse of the byte into W1TC to clear all the bits that need to be cleared, and increment the buffer index
        // When transfering bytes over a connection the parity should always be sent after the data settles.
        // This means that the data should be latched (or settles) and the parity should be sent.
        // This allows for the data to be retrieved first and then for the parity to be calculated and read and compared after that.
        sendParity(blockData[bufferIndex++]); // Send the parity for the byte
      }
      #endif
    }
    prevState = currentState; // Update the previous state of STRB
  }
}

// This function receives bytes from the host to form the command buffer, and is in IRAM for the same reason as sendMultiData
IRAM_ATTR void receiveCommand() {
  prevState = 1; // Set starting state.
  while(bitRead(REG_READ(CMD_IN_REG), CMDPin) == 1 && bitRead(REG_READ(PRES_IN_REG), PRESPin) == 1){ // Stay in the command-reception loop until the host lowers CMD or the drive is reset
    currentState = bitRead(REG_READ(STRB_IN_REG), STRBPin); // Read the current state of STRB
    if(currentState == 0 and prevState == 1){ // If we see a falling edge on STRB, read the bus and store the data in the command buffer
      commandBuffer[bufferIndex] = REG_READ(BUS_IN_REG) >> busOffset;
      sendParity(commandBuffer[bufferIndex++]); // Send the parity bit out for that byte, and increment bufferIndex to the next spot in the buffer
    }
    prevState = currentState; // Update the previous state of STRB
  }
}

#if APPLE_III_ONLY
// This function receives bytes from the host to form the write command check bytes, and is in IRAM for the same reason as sendMultiData
// Apple /// sends four bytes, Titan Apple /// Apple IIe plus card sends only three.
// Either way CMD goes down when finished. The real Profile drive does not seem to care either way.
IRAM_ATTR void receiveCheckBytes() {
  checkBytesIndex = 0; // Always start out at zero.
  prevState = 0; // Set the correct starting position.
  // Need to read the bytes as fast as possible and this will just slow things down.
  while(bitRead(REG_READ(CMD_IN_REG), CMDPin) == 1 && bitRead(REG_READ(PRES_IN_REG), PRESPin) == 1){ // Stay in the command-reception loop until the host lowers CMD or the drive is reset
    currentState = bitRead(REG_READ(STRB_IN_REG), STRBPin); // Read the current state of STRB
    if((currentState == 0) && (prevState == 1) && (checkBytesIndex < 6)){ // If we see a falling edge on STRB, read the bus and store the data in the command buffer
      checkBytesBuffer[checkBytesIndex] = REG_READ(BUS_IN_REG) >> busOffset;
      sendParity(checkBytesBuffer[checkBytesIndex++]); // Send the parity bit out for that byte, and increment bufferIndex to the next spot in the buffer
    }
    prevState = currentState; // Update the previous state of STRB
  }
}
#endif

// Another byte-receiving function, also in IRAM, but this one is for receiving data during a write command
IRAM_ATTR void receiveMultiData() {
  prevState=1;
  bufferIndex = 0; // Reset the blockData buffer index.

  #if !APPLE_III_ONLY
  // Make sure there are correct values of CMD and PRES pins
  while(bitRead(REG_READ(CMD_IN_REG), CMDPin) == 1 && bitRead(REG_READ(PRES_IN_REG), PRESPin) == 1){
  #else
  // On the Apple /// just stop and wait for CMD to be raised.  If it already is then just continue. We know why we got here.
  // This does have a possible problem with the CMDPin not being checked.  It could cause things to go out of sync.
  while(bitRead(REG_READ(CMD_IN_REG), CMDPin) == 0);

  // What this does cause is to have the CMD pin not be checked which speeds up the writing from the host.
  // If you do not do this it causes delays.
  // TODO: In the future when everything is working correctly maybe try to go back to the above but I have tried that twice so far and each try has failed.
  // Make it much faster so that we get it really quick and CMD and Resets.
  while (bufferIndex < APPLE_III_WRITE_SIZE) {
  #endif
    currentState = bitRead(REG_READ(STRB_IN_REG), STRBPin); // Read the current state of STRB
    if(currentState == 0 and prevState == 1){ // If we see a falling edge on STRB, it means the data is valid
      blockData[bufferIndex] = REG_READ(BUS_IN_REG) >> busOffset; // So read the data from the bus into the blockData buffer
      sendParity(blockData[bufferIndex++]); // And send the appropriate parity for this byte, followed by incrementing the buffer index
    }
    prevState = currentState; // Update the previous state of STRB
  }

  #if APPLE_III_ONLY
  // Apple /// only sends 512 and then it sends six check bytes.
  // Those check bytes are sent slightly differently.
  receiveCheckBytes();
  #endif
}

// ****************************************************************************************
// I made this header so big because I kept not being able to find it when scrolling down.
//
//
// Process and execute a read command
//
//
// This is a pretty important header block.
// ****************************************************************************************
void readDrive(){
  setParallelDir(1); // Set the bus to output mode
  //delayMicroseconds(1);
  sendData(0x02); // Acknowledge the read command with an 0x02 (command value + 2)
  sendParity(0x02); // And the appropriate parity bit for that value
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
  uint8_t response;
  while(((response = receiveData()) != 0x55) && (response != 0xAA)){ // And wait for the host to respond with an 0x55 or 0xAA; timeout if it doesn't
    currentTime++;
    if(currentTime >= timeout){
      Serial.println(" - Read phase 2: Host didn't respond with a 55!");
      return;
    }
  }

  // Don't forget to send parity no matter what it is.
  sendParity(response);

  // Could be an 0xAA which means it does not know what our response was.
  if (response == 0xAA){
      Serial.println("Rd phase 2: Host did not understand our response (0xAA returned).");
      return;
  }

  blockNum = (commandBuffer[1] << 16 | commandBuffer[2] << 8 | commandBuffer[3]); // Form a block number from bytes 1-3 of the command buffer
  // If the block number is 0xFFFFFF, then the host is asking for the spare table
  if(commandBuffer[1] == 0xFF and commandBuffer[2] == 0xFF and commandBuffer[3] == 0xFF){
    Serial.println(" - Spare Table"); // Print this fact to the console
    for(int i = 0; i < PROFILE_SPARE_TABLE_SIZE; i++){ // And fill our block data buffer with the spare table
      blockData[i+READ_STATUS_OFFSET] = spareTable[i];
    }
    for(int i = PROFILE_SPARE_TABLE_SIZE; i < PROFILE_BLOCK_SIZE; i++){ // Pad the rest of the space with FF's
      blockData[i+READ_STATUS_OFFSET] = 0xFF;
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
      blockData[i+READ_STATUS_OFFSET] = days[i % 4];
    }
    for(int i = 4; i < 6; i++){
      blockData[i+READ_STATUS_OFFSET] = hours[i % 2];
    }
    for(int i = 6; i < 8; i++){
      blockData[i+READ_STATUS_OFFSET] = minutes[i % 2];
    }
    for(int i = 8; i < 10; i++){
      blockData[i+READ_STATUS_OFFSET] = seconds[i % 2];
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
      blockData[dataIndex+READ_STATUS_OFFSET] = bytesFree[j];
      dataIndex--;
    }
    for(i = 14 - i; i > 0; i--){ // Pad the remaining space with 0x20's (space characters)
      blockData[dataIndex+READ_STATUS_OFFSET] = 0x20;
      dataIndex--;
    }
    // Our system is too simple to worry about load or processes, so just fill these fields with random data that looks cool
    blockData[25+READ_STATUS_OFFSET] = 0x30; // Load
    blockData[26+READ_STATUS_OFFSET] = 0x00; // Load
    blockData[32+READ_STATUS_OFFSET] = 0x30; // Load
    blockData[33+READ_STATUS_OFFSET] = 0x00; // Load
    blockData[39+READ_STATUS_OFFSET] = 0x30; // Load
    blockData[40+READ_STATUS_OFFSET] = 0x00; // Load
    blockData[46+READ_STATUS_OFFSET] = 0x31; // Processes
    blockData[47+READ_STATUS_OFFSET] = 0x00; // Processes
    blockData[51+READ_STATUS_OFFSET] = 0x31; // Processes
    blockData[52+READ_STATUS_OFFSET] = 0x00; // Processes
    for(int i = 56; i < PROFILE_BLOCK_SIZE; i++){ // Then null terminate for the rest of the block
      blockData[i+READ_STATUS_OFFSET] = 0x00;
    }
  }
  // If the block number is 0xFFFEFE, then it's a Selector "get file info" command
  else if(commandBuffer[1] == 0xFF and commandBuffer[2] == 0xFE and commandBuffer[3] == 0xFE){
    setLEDColor(1, 1); // Set the LED to amber to show that we're running a Selector command
    selectorCommandRun = true; // And set the flag that tells us that a Selector command has been run
    fileCount = 0; // Initialize the file counter to 0
    rootDir.rewind(); // And go back to the start of the SD root directory
    while(scratchFile.openNext(&rootDir, O_RDONLY)){ // Keep opening files in the root directory until we run out, incrementing fileCount for each valid file
      fileName[scratchFile.getName7(fileName, FILE_NAME_BUFFER_SIZE)] = 0x00;
      if(not scratchFile.isDir() and fileName[0] != '.' and strstr(fileName, extension) != 0){
        fileCount += 1;
      }
      scratchFile.close();
    }
    for(int i = 0; i < 4; i++){ // Set the first four bytes of the block data to the nonce
      blockData[i+READ_STATUS_OFFSET] = nonce;
    }
    // And the next two bytes to the file count
    blockData[4+READ_STATUS_OFFSET] = fileCount << 8;
    blockData[5+READ_STATUS_OFFSET] = fileCount;

    uint32_t fileSize;
    uint16_t dateModified[50];
    uint16_t timeModified[50];
    rootDir.rewind(); // Now go back to the start of the directory again
    if(((commandBuffer[4] << 8) + commandBuffer[5]) < fileCount){ // And double-check that the host is asking for info on a valid file
      fileCount = 0;
      while(fileCount <= (commandBuffer[4] << 8) + commandBuffer[5]){ // Keep opening files until we get to the one that the host is asking for
        scratchFile.openNext(&rootDir, O_RDONLY);
        fileName[scratchFile.getName7(fileName, FILE_NAME_BUFFER_SIZE)] = 0x00;
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
        blockData[i+READ_STATUS_OFFSET] = 0x42;
      }
      // Fill the next 10 bytes with the file size
      blockData[20+READ_STATUS_OFFSET] = fileSize >> 72;
      blockData[21+READ_STATUS_OFFSET] = fileSize >> 64;
      blockData[22+READ_STATUS_OFFSET] = fileSize >> 56;
      blockData[23+READ_STATUS_OFFSET] = fileSize >> 48;
      blockData[24+READ_STATUS_OFFSET] = fileSize >> 40;
      blockData[25+READ_STATUS_OFFSET] = fileSize >> 32;
      blockData[26+READ_STATUS_OFFSET] = fileSize >> 24;
      blockData[27+READ_STATUS_OFFSET] = fileSize >> 16;
      blockData[28+READ_STATUS_OFFSET] = fileSize >> 8;
      blockData[29+READ_STATUS_OFFSET] = fileSize;

      // That was the raw filesize, we also have to provide a size string like "5M or 16K"
      char sizeString[5];
      if(fileSize == 5175296){ // If it's a 5MB image, set the size string in the block data to "5M"
        blockData[30+READ_STATUS_OFFSET] = 0x20;
        blockData[31+READ_STATUS_OFFSET] = 0x20;
        blockData[32+READ_STATUS_OFFSET] = 0x35;
        blockData[33+READ_STATUS_OFFSET] = 0x4D;
      }
      else if(fileSize == 10350592){ // If it's a 10MB image, set the size string to "10M"
        blockData[30+READ_STATUS_OFFSET] = 0x20;
        blockData[31+READ_STATUS_OFFSET] = 0x31;
        blockData[32+READ_STATUS_OFFSET] = 0x30;
        blockData[33+READ_STATUS_OFFSET] = 0x4D;
      }
      else{ // Otherwise, things will need to get a bit more complicated
        if(fileSize > 1089003999999){ // If the file is bigger than GB-scale, make the size string say "HUGE"
          blockData[30+READ_STATUS_OFFSET] = 0x48;
          blockData[31+READ_STATUS_OFFSET] = 0x55;
          blockData[32+READ_STATUS_OFFSET] = 0x47;
          blockData[33+READ_STATUS_OFFSET] = 0x45;
        }
        else if(fileSize > 1089003999){ // If the file is GB-scale, then make the appropriate size string, append "G", and put it in the block data
          sprintf(sizeString, "%3d", (fileSize/1089004000));
          strlcat(sizeString, "G", sizeof(sizeString));
          blockData[30+READ_STATUS_OFFSET] = sizeString[0];
          blockData[31+READ_STATUS_OFFSET] = sizeString[1];
          blockData[32+READ_STATUS_OFFSET] = sizeString[2];
          blockData[33+READ_STATUS_OFFSET] = sizeString[3];
        }
        else if(fileSize > 1089004){ // Repeat for MB-scale
          sprintf(sizeString, "%3d", (fileSize/1089004));
          strlcat(sizeString, "M", sizeof(sizeString));
          blockData[30+READ_STATUS_OFFSET] = sizeString[0];
          blockData[31+READ_STATUS_OFFSET] = sizeString[1];
          blockData[32+READ_STATUS_OFFSET] = sizeString[2];
          blockData[33+READ_STATUS_OFFSET] = sizeString[3];
        }
        else if(fileSize > 9999){ // KB-scale
          sprintf(sizeString, "%3d", (fileSize/1090));
          strlcat(sizeString, "K", sizeof(sizeString));
          blockData[30+READ_STATUS_OFFSET] = sizeString[0];
          blockData[31+READ_STATUS_OFFSET] = sizeString[1];
          blockData[32+READ_STATUS_OFFSET] = sizeString[2];
          blockData[33+READ_STATUS_OFFSET] = sizeString[3];
        }
        else{ // And B-scale
          sprintf(sizeString, "%4d", fileSize);
          blockData[30+READ_STATUS_OFFSET] = sizeString[0];
          blockData[31+READ_STATUS_OFFSET] = sizeString[1];
          blockData[32+READ_STATUS_OFFSET] = sizeString[2];
          blockData[33+READ_STATUS_OFFSET] = sizeString[3];
        }
      }
      for(int i = 20; blockData[i+READ_STATUS_OFFSET] == 0x00; i++){ // Replace all zeros in the block between indices 20 and the next data field with 0x20s (spaces)
        blockData[i+READ_STATUS_OFFSET] = 0x20;
      }
      int i = 0;
      for(i = 276; fileName[i - 276] != 0x00; i++){ // Then put the filename into the block data starting at index 276
        blockData[i+READ_STATUS_OFFSET] = fileName[i - 276];
      }
      for(; i < PROFILE_BLOCK_SIZE; i++){ // And fill the rest of the block with zeros
        blockData[i+READ_STATUS_OFFSET] = 0x00;
      }
    }
    else{ // If the file number wasn't valid this whole time, just fill the block with 0x00's instead
      for(int i = 0; i < PROFILE_BLOCK_SIZE; i++){
        blockData[i+READ_STATUS_OFFSET] = 0x00;
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
      for(int i = 0; i < PROFILE_BLOCK_SIZE; i++){ // So read the moniker out of the ESP32's EEPROM and put it in our block data
        blockData[i+READ_STATUS_OFFSET] = EEPROM.read(KVMoniker + i);
      }
    }
    else if(commandBuffer[4] == 0x53 and commandBuffer[5] == 0x61){ // If the retry/spare are 0x53 and 0x61, then the host wants our autoboot status
      Serial.println(" - Selector: Sending autoboot script...");
      for(int i = 0; i < PROFILE_BLOCK_SIZE; i++){ // So read it out of the EEPROM and put it in the block data
        blockData[i+READ_STATUS_OFFSET] = EEPROM.read(KVAutoboot + i);
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
      blockNum = ((commandBuffer[4] & B00001111) << 8 | commandBuffer[5]) * PROFILE_BLOCK_SIZE; // The block that we want to send is formed from the lower nibble of the retry count and the full spare threshold
      if(!sourceFile.open("/rescue/selector.image", O_RDONLY)){ // Open the backup Selector image
        Serial.println(" - Selector: Error opening backup Selector ProFile image!");
      }
      else if((blockNum + 531) >= sourceFile.fileSize()){ // And make sure that the block we want to send isn't past the end of the file
        Serial.println(" - Selector Error: Block is past the end of the file!");
        for(int i = 0; i < PROFILE_BLOCK_SIZE; i++){ // If it is, fill the block data buffer with zeros
          blockData[i+READ_STATUS_OFFSET] = 0x00;
        }
      }
      // Otherwise, read the block out of the image
      else{
        disk.seekSet(blockNum); // Seek to the block in the disk image
        sourceFile.read(blockData+READ_STATUS_OFFSET, PROFILE_BLOCK_SIZE); // And read the block into the block data buffer
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
      blockNum = ((commandBuffer[4] & B00001111) << 8 | commandBuffer[5]) * PROFILE_BLOCK_SIZE;
      if(!sourceFile.open("/rescue/selector.3.5inch.dc42", O_RDONLY)){
        Serial.println(" - Selector: Error opening backup Selector 3.5 image!");
      }
      else if((blockNum + (PROFILE_BLOCK_SIZE-1)) >= sourceFile.fileSize()){
        Serial.println(" - Selector Error: Block is past the end of the file!");
        for(int i = 0; i < PROFILE_BLOCK_SIZE; i++){
          blockData[i+READ_STATUS_OFFSET] = 0x00;
        }
      }
      else{
        disk.seekSet(blockNum);
        sourceFile.read(blockData+READ_STATUS_OFFSET, PROFILE_BLOCK_SIZE);
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
      blockNum = ((commandBuffer[4] & B00001111) << 8 | commandBuffer[5]) * PROFILE_BLOCK_SIZE;
      if(!sourceFile.open("/rescue/selector.twiggy.dc42", O_RDONLY)){
        Serial.println(" - Selector: Error opening backup Selector Twiggy image!");
      }
      else if((blockNum + 531) >= sourceFile.fileSize()){
        Serial.println(" - Selector Error: Block is past the end of the file!");
        for(int i = 0; i < PROFILE_BLOCK_SIZE; i++){
          blockData[i+READ_STATUS_OFFSET] = 0x00;
        }
      }
      else{
        disk.seekSet(blockNum);
        sourceFile.read(blockData+READ_STATUS_OFFSET, PROFILE_BLOCK_SIZE);
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
      for(int i = 0; i < PROFILE_BLOCK_SIZE; i++){ // So fill the entire block with zeros
        blockData[i+READ_STATUS_OFFSET] = 0x00;
      }
    }
  }
  // If we end up here, then our command isn't a Selector command; it's just a good old-fashioned read
  else if((blockNum*PROFILE_BLOCK_SIZE) < disk.fileSize()){ // Check that the block number is within the range of the disk image
    uint8_t blockHigh = (blockNum >> 8 & 0xFF);
    uint8_t blockLow = blockNum & 0xFF;

    blockNum *= PROFILE_BLOCK_SIZE;
    disk.seekSet(blockNum); // Seek to our block
    disk.read(blockData+READ_STATUS_OFFSET, PROFILE_BLOCK_SIZE); // And read the block into the block data buffer
    #if APPLE_III_ONLY
    // This is not really for the Apple /// but for the Titan /// Plus IIe.
    // The Titan /// Plus IIe fails if the correct values are not sent.
    // In fact, it stops after the first three bytes are sent if they are not correct.
    // Then even if the first three bytes come back fine the Titan /// Plus IIe fails
    // and stops after the fourth byte if it does not see the 0xFF.
    // It will continue if it sees the 0xFF and will finally read all six but will still
    // fail the whole transaction if the last two are not correct.
    // Weird that the true Apple /// does not care. I even checked the driver and unless
    // I read the driver code for the Apple /// wrong it does not care.
    // Can't hurt to do it like the Profile drive does it for everyone.
    blockData[READ_STATUS_OFFSET+APPLE_III_WRITE_SIZE] = 0x00;
    blockData[READ_STATUS_OFFSET+APPLE_III_WRITE_SIZE+1] = blockHigh;
    blockData[READ_STATUS_OFFSET+APPLE_III_WRITE_SIZE+2] = blockLow;
    blockData[READ_STATUS_OFFSET+APPLE_III_WRITE_SIZE+3] = 0xFF;
    blockData[READ_STATUS_OFFSET+APPLE_III_WRITE_SIZE+4] = 0xFF - blockHigh;
    blockData[READ_STATUS_OFFSET+APPLE_III_WRITE_SIZE+5] = 0xFF - blockLow;
    #endif
  }
  // Otherwise, if the block is outside the range of the disk image, fill the block data buffer with zeros
  else{
    Serial.println(" - Error: Requested block is out of range!");
    for(int i = 0; i < PROFILE_BLOCK_SIZE; i++){
      blockData[i+READ_STATUS_OFFSET] = 0x00;
    }
  }

  setParallelDir(1); // Now set the bus to output mode
  for(int i = 0; i < READ_STATUS_OFFSET; i++){ // And set the 4 status bytes in blockData to 0x00 (no errors)
    blockData[i] = 0x00;
  }
  bufferIndex = 0; // Reset the blockData buffer index

  // When transfering bytes over a connection the parity should always be sent after the data settles.
  // This means that the data should be latched (or settles) and the parity should be sent.
  // This allows for the data to be retrieved first and then for the parity to be calculated and read and compared after that.
  sendData(blockData[bufferIndex]); // Followed by the byte itself, and increment the buffer index
  sendParity(blockData[bufferIndex++]); // Put the parity for the first byte on the bus

  clearBSY(); // And raise BSY to tell the host that we're ready

  sendMultiData(); // Now send the rest of the block data, with the appropriate parity bits for each byte

  Serial.println();
 }

// ****************************************************************************************
// I made this header so big because I kept not being able to find it when scrolling down.
//
//
// Process and execute a write command
//
//
// This is a pretty important header block.
// ****************************************************************************************
void writeDrive(){
  uint8_t response;

  //
  // Handshake for write with host to say we got the command.
  //
  setParallelDir(1); // Set the bus to output mode
  //delayMicroseconds(1);

  sendData(commandBuffer[0] + 0x02); // Acknowledge the write command by sending the command value + 2
  sendParity(commandBuffer[0] + 0x02); // And the appropriate parity for that value
  
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

  //
  //
  //  Handshake from host replying that it got our response.
  //
  //
  setParallelDir(0); // Set the bus to input mode
  //delayMicroseconds(1);
  
  currentTime = 0;
  while(((response = receiveData()) != 0x55) && (response != 0xAA)){ // And wait for the host to respond with an 0x55 or 0xAA; timeout if it doesn't
    currentTime++;
    if(currentTime >= timeout){
      Serial.println(" - Write phase 2: Host didn't respond with a 55!");
      return;
    }
  }

  // Don't forget to send parity no matter what value is returned. As long as we did not timeout.
  // This used to be a problem in the code on the Apple ///.
  // When the Apple /// looses parity at all the write will check for a parity error and things get out of sync.
  sendParity(response);

  // Could be an 0xAA which means it does not know what our response was.
  if (response == 0xAA){
      Serial.println(" - Write phase 2: Host did not understand our response (0xAA returned).");
      return;
  }

  clearBSY(); // Raise BSY to tell the host that we're ready to receive the data
 
  receiveMultiData(); // And receive the block data from the host

  #if APPLE_III_ONLY
  currentTime = 0;
  // Just in case, check if host has lowered CMD. For Apple /// we should have gotten everything but just in case.
  while(readCMD() == 1){ // Wait for the host to lower CMD and timeout if it doesn't
    currentTime++;
    if(currentTime >= timeout){
      Serial.println("Timeout: I have read enough waiting for host to lower the command line.");
      return;
    }
  }

  //
  //
  // Send back that we completed the write, whatever it was.
  //
  //
  // On the Apple ///:
  // I could not find a way to know if there was a parity error. If there was then we should
  // be sending back 01 and/or just retrying.
  // If we were able to figure out if there was a parity error then we should just return.
  // Hopefully, cross your fingers, not a parity error...
  // Of course, if there was a parity error the retry will come in instead and then we
  // will leave because the 0x06 will fail with an AA.
  #endif

  setParallelDir(1); // Set the bus to output mode

  //delayMicroseconds(1);
  sendData(0x06); // Acknowledge the successful reception of the data block by sending an 0x06
  sendParity(0x06); // And the appropriate parity for that value

  #if !APPLE_III_ONLY
  setBSY(); // And lower BSY to tell the host about our acknowledgement
  #else
  // TODO: maybe try again with the LED being turned on/off?  Maybe something else was wrong and we can remove this?
  // During the time this was put in the LED was causing a very long delay and the Apple /// was not getting data correctly.
  setBSYLow(); // And lower BSY to tell the host about our acknowledgement
  #endif

  currentTime = 0;
  while(readCMD() == 0){ // Wait for the host to raise CMD and timeout if it doesn't
    currentTime++;
    if(currentTime >= timeout){
      Serial.println("Timeout: Write Command Second Handshake Part 2");
      return;
    }
  }

  setParallelDir(0); // Set the bus to input mode
  //delayMicroseconds(1);

  currentTime = 0;
  while(((response = receiveData()) != 0x55) && (response != 0xAA)){ // And wait for the host to respond with an 0x55 or 0xAA; timeout if it doesn't
    currentTime++;
    if(currentTime >= timeout){
      Serial.print(" - Write phase 3: Host didn't respond with a 55 or AA but ");
      printDataNoSpace(response);
      Serial.println("!");
      return;
    }
  }

  // Don't forget to send parity no matter what it is.
  sendParity(response);

  // Could be an 0xAA which means it does not know what our response was.
  // This could be because on the Apple /// we do not have a way to recognize
  // that there was a parity problem.  This was mentioned above.
  if (response == 0xAA){
      Serial.println(" - Write phase 3: Host did not understand our response (0xAA returned).");
      return;
  }

  // Host returned 0x55 so we are good. Continue!

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
      for(int i = 0; i < PROFILE_BLOCK_SIZE; i++){ // So write the block data into the EEPROM at the moniker location
        EEPROM.write(KVMoniker + i, blockData[i]);
      }
      EEPROM.commit();
    }
    // If the retry count and spare threshold are 0x53 and 0x61, then the host wants to write to the device autoboot field
    else if(commandBuffer[4] == 0x53 and commandBuffer[5] == 0x61){
      Serial.println(" - Selector: Updating device autoboot script...");
      for(int i = 0; i < PROFILE_BLOCK_SIZE; i++){ // So write the block data into the EEPROM at the autoboot location
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
  #if 1
  else if((blockNum*PROFILE_BLOCK_SIZE) < disk.fileSize()){ // Check that the block number is within the range of the disk image
  #else
  else if(blockNum < disk.fileSize()){ // Check that the block number is within the range of the disk image
  #endif
    blockNum *= PROFILE_BLOCK_SIZE;
    
    disk.seekSet(blockNum); // Seek to the block location in the disk image by byte location.

    // Write the block data into the disk image at the location specified
    #if !APPLE_III_ONLY
    // For the Apple Lisa
    disk.write(blockData, PROFILE_BLOCK_SIZE);
    #else
    // For the Apple ///
    disk.write(blockData, APPLE_III_WRITE_SIZE); // Warning! Apple /// only does 512 bytes!!!!
    // The weird thing about this change is that I never saw the actual Profile drive ever send
    // over 536 bytes.  Maybe mine is different? I have no clue.  I do not have an Apple Lisa to
    // to check and see if there is some way the drive knows it is talking differently.  I do
    // know from all my logic probing that for the Apple /// my Profile drive only sends 512.
    // (Plus the four at the beginning and the six status at the end.)
    // I have seen online logic probe outputs that show the full 536 but not in any of mine.
    #endif

    disk.flush(); // And flush the disk image to make sure the write is complete

    Serial.println();
  }
  // If the block is outside the range of the disk image, print an error message
  else{
    Serial.println(" - Error: Requested block is out of range!");
  }

  sendWriteStatus();

  if(halt == true){ // If we set the halt flag earlier, then we need to halt the ESProFile here
    while(1);
  }
}

//
// Send Status!!!
//
void sendWriteStatus()
{
  setParallelDir(1); // Now set the bus to output mode
  //delayMicroseconds(1);

  for(int i=0;i < STATUS_BUFFER_SIZE;i++){
    statusBuffer[i] = 0x00;
  }

  statusBufferIndex = 0; // Set the buffer index to the first status byte

  // When transfering bytes over a connection the parity should always be sent after the data settles.
  // This means that the data should be latched (or settles) and the parity should be sent.
  // This allows for the data to be retrieved first and then for the parity to be calculated and read and compared after that.
  sendData(statusBuffer[statusBufferIndex]); // And put the status byte on the bus, incrementing the buffer index afterwards
  sendParity(statusBuffer[statusBufferIndex++]); // Put its parity info on the PARITY line

  clearBSY(); // Raise BSY to tell the host that we're ready to continue
  // Now send the rest of the status bytes, with the appropriate parity for each byte
  sendWriteStatusBytes();
}

// Prints whatever command the drive is currently executing
void printCommand() {
  // Old/simple way of printing raw commands for debugging
  // I've decided to actually revert to this though because it's cleaner and provides more info
  for(int i = 0; i < COMMAND_BUFFER_SIZE; i++){
    printDataNoSpace(commandBuffer[i]);
  }
  // If the command is unknown, print a warning
  if(commandBuffer[0] > 0x03){
    // Ignore 0xFF even though it is an invalid command; the Apple /// sends this as a dummy command pretty frequently
    if(commandBuffer[0] != 0xFF){
      Serial.println(" - Unknown command!");
    }
    else {
      Serial.println();
    }
  }

  // New way to print the command in a more human-readable format
  /*if(commandBuffer[0] == 0x00){ // Read command
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
  printDataNoSpace(commandBuffer[3]);*/

  // We used to print the retry count and spare threshold, but it's kind of unnecessary for an emulator and wastes transmission time
  /*Serial.print("        Retry Count: "); // The retry count
  printDataNoSpace(commandBuffer[4]);
  Serial.print("        Spare Threshold: "); // And the spare threshold
  printDataNoSpace(commandBuffer[5]);*/
}

// Set all pins to the correct initial direction and state
void initPinsEmulator(){
  for(int i = dataBusStart; i < dataBusStart + 8; i++){ // Make the bus an input
    pinMode(i, INPUT);
  }
  // And make everything else inputs too, except BSY, PARITY, and the LEDs
  pinMode(CMD_Pin, INPUT);
  pinMode(BSY_Pin, OUTPUT);
  pinMode(RW_Pin, INPUT);
  pinMode(STRB_Pin, INPUT);
  pinMode(PRES_Pin, INPUT);
  pinMode(PARITY_Pin, OUTPUT);
  pinMode(red_led, OUTPUT);
  pinMode(green_led, OUTPUT);
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
    spareTable[18] = (disk.fileSize()/PROFILE_BLOCK_SIZE) >> 16; // Drive size
    spareTable[19] = (disk.fileSize()/PROFILE_BLOCK_SIZE) >> 8; // Drive size
    spareTable[20] = (disk.fileSize()/PROFILE_BLOCK_SIZE); // Drive size
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
  #if USE_LEDS
  setLEDColor(0, 0); // Setting BSY is special because it also turns off the LEDs
  #endif
  REG_WRITE(BSY_W1TC_REG, 0b1 << BSYPin); // In addition to setting BSY low
}

inline __attribute__((__always_inline__)) void clearBSY(){
  #if USE_LEDS
  setLEDColor(0, 1); // Same for clearing BSY; we have to turn the green LED on
  #endif
  REG_WRITE(BSY_W1TS_REG, 0b1 << BSYPin); // As well as setting BSY high
}

// The other set/clear functions don't bother with the LEDs

#if APPLE_III_ONLY
inline __attribute__((__always_inline__)) void setBSYLow(){
  REG_WRITE(BSY_W1TC_REG, 0b1 << BSYPin); // In addition to setting BSY low
}

inline __attribute__((__always_inline__)) void setBSYHigh(){
  REG_WRITE(BSY_W1TS_REG, 0b1 << BSYPin); // As well as setting BSY high
}
#endif

// The parity functions are inline because they're called in a part of the code where speed is critical
inline __attribute__((__always_inline__)) void setPARITY(){
  REG_WRITE(PARITY_W1TC_REG, 0b1 << PARITYPin);
}

inline __attribute__((__always_inline__)) void clearPARITY(){
  REG_WRITE(PARITY_W1TS_REG, 0b1 << PARITYPin);
}

// The read functions just return the state of their corresponding control signals
inline __attribute__((__always_inline__)) bool readCMD(){
  return bitRead(REG_READ(CMD_IN_REG), CMDPin);
}

inline __attribute__((__always_inline__)) bool readRW(){
  return bitRead(REG_READ(RW_IN_REG), RWPin);
}

inline __attribute__((__always_inline__)) bool readSTRB(){
  return bitRead(REG_READ(STRB_IN_REG), STRBPin);
}
