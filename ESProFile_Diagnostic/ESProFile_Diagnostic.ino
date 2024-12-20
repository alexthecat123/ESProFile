//***********************************************************************************
//* ESProFile ProFile Diagnostic Software                                           *
//* By: Alex Anderson-McLeod                                                        *
//* Email address: alexelectronicsguy@gmail.com                                     *
//***********************************************************************************



int busOffset = 12;
int CMDPin = 21;
int BSYPin = 22;
int RWPin = 23;
int STRBPin = 25;
int PRESPin = 26;
int PARITYPin = 27;

byte packetNum = 0x01;
bool testMenu = false;
bool confirmOperation = false;
byte notPacketNum = 0xFE;
bool failed = false;
bool repeat = false;
byte SOH = 0x01;
byte STX = 0x02;
byte ACK = 0x06;
byte NAK = 0x15;
byte EOT = 0x04;
byte CAN = 0x18;
byte driveSize[3];
byte padding = 0x1A; //0x1A
byte inData;
byte blockData[1064];
byte crcArray[1024];
uint16_t backupErrors = 0;
uint32_t currentIndex;
uint32_t writeIndex;
uint32_t dataIndex = 0;
byte lowCRC;
byte highCRC;
byte checksum = 0x00;
uint16_t CRC;
uint16_t actualCRC;
uint32_t currentBlock = 0;
byte ackStatus = 0x02;
byte rawData[1029];
long int start;
//byte dataStripped[1024];
int done = 0;
//File xModemData;
uint16_t highestBlock = 0;


byte driveStatus[5];
//byte blockData[532];
char input[6];
uint8_t serialBytes[50];
byte address[3];
byte commandBuffer[6];
bool commandResponseMatters = true;
byte commandBufferTenMegDiag[15];
byte commandBufferWidget[15];
bool isTenMegDiagCommand = false;
bool isWidgetCommand = false;

//byte parity[532];

byte inputData;
char x;


const char *statusMessages[3][8] = {{"Operation Unsuccessful", "Unrecoverable Servo Error (Widget Only)", "Timeout Error", "CRC Error", "Seek Error - Cannot Read Header", "Resend Data", "More Than 532 Bytes Sent", "Host Acknowledge Was Not 0x55"},
                          {"Controller Aborted Last Operation (Widget Only)", "Seek Error - Wrong Track", "Sparing Occured", "Cannot Read Spare Table", "Bad Block Table Full", "Five or Fewer Spare Blocks Available (Widget Only)", "Spare Table Full", "Seek Error - Cannot Read Error"},
                          {"Parity Error", "Drive Gave Bad Response", "Drive Was Reset", "This bit is unused", "This bit is unused", "Block ID Mismatch", "Invalid Block Number", "Drive Has Been Reset"}};

const char *widgetStatusMessages[3][8] = {{"Operation Failed", "Unrecoverable Servo Error", "No Matching Header Found", "Read Error", "This bit is unused", "This bit is unused", "Write Buffer Overflow", "Host Acknowledge Was Not 0x55"},
                          {"Controller Aborted Last Operation", "Seek to Wrong Track Occurred", "Spare Table Has Been Updated", "Controller Self-Test Failure", "This bit is unused", "Five or Fewer Spare Blocks Available", "Spare Table Overflow", "This bit is unused"},
                          {"This bit is unused", "This bit is unused", "This bit is unused", "This bit is unused", "This bit is unused", "This bit is unused", "Last Logical Block Number Was Out of Range", "First Status Response Since Power-On Reset"}};

const uint16_t abortStatusAddresses[3][42] = {{0x02ED, 0x03BB, 0x048A, 0x04CE, 0x0516, 0x1114, 0x1204, 0x1216, 0x122A, 0x1329, 0x1408, 0x1542, 0x15B8, 0x16E0, 0x192C, 0x1B0A, 0x1B5F, 0x5555, 0x1BC3, 0x1C00, 0x1C0F, 0x1C63, 0x1CF8, 0x1E49, 0x1F3C, 0x2026, 0x21E7, 0x236D, 0x2493, 0x24B3, 0x2525, 0x5555, 0x264A, 0x29C3, 0x29F5, 0x2CD2, 0x18E8, 0x226F, 0x2858, 0x287A, 0x2940, 0x2D0B}, //Codes of 0x5555 mean that the code is defined in the Widget ERS document, but not in the code, so I just gave them a random value that should never actually be presented in the abort status. If I can ever find them in the code, I'll change them back to actual addresses.
                                       {0x09, 0x00, 0x0A, 0x00, 0x0A, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x0A, 0xFF, 0xFF, 0x09, 0xFF, 0xFF, 0x0A, 0x0A, 0x0A, 0x0C, 0xFF, 0xFF, 0xFF, 0x09, 0xFF, 0xFF, 0x0A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                       {0xFF, 0xFF, 0xFF, 0xFF, 0x0B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

const char *abortStatusMessages[3][42] = {{"Illegal interface response, or host NAK", "Illegal RAM bank selection", "Format error: Illegal state machine state", "Illegal bank switch: Either call or return", "Illegal interrupt or dead man timeout", "Format error: Error while writing sector", "Command checkbyte error", "ProFile or system command attempted while there was a self-test error", "Illegal interface instruction", "Unrecoverable servo error while reading", "Sparing attempted on nonexistent spared block", "Sparing attempted while spare table is full", "Deletion attempted of nonexistent bad block", "Illegal exception instruction", "Unrecoverable servo error while writing", "Servo status request sent as servo command", "Restore error: Non-recal parameter", "Store Map error: Parameter larger than number of sectors", "Illegal password sent for Write Spare Table command", "Illegal password sent for Format command", "Illegal format parameters", "Illegal password sent for Init Spare Table command", "Zero block count sent for multiblock transfer", "Write error: Illegal state machine state", "Read error: Illegal state machine state", "Read Header error: Illegal state machine state", "Request for illegal logical block", "Search for spare table failed", "No spare table structure found in spare table", "Update of spare table failed", "Illegal spare count instruction", "Unrecoverable servo error while performing overlapped seek", "Unrecoverable servo error while seeking", "Servo error after servo reset", "Servo communication error after servo reset", "Scan attempted without spare table", "Write buffer overrun??", "Stack underflow", "Failed to send servo command after 4 attempts", "Failed to read servo status after 4 attempts", "Failed to read header after restore", "Sector not found in interleave map table duing scan??????"},
                                    {"Response received from host: ", "Bank number of attempted select: ", "State machine state at time of failure: ", "Bank number of attempted bank select: ", "High byte of the address of the routine at the time of timeout: ", "Error status from the FormatBlock routine: ", "", "", "", "", "", "", "", "", "", "", "Parameter that was sent: ", "Parameter that was sent: ", "", "", "Specified format offset: ", "", "", "State machine state at the time of the error: ", "State machine state at the time of the error: ", "State machine state at the time of the error: ", "High byte of the requested logical block: ", "", "", "", "Value of illegal instruction: ", "", "", "Value of controller status port at the time of the error: ", "", "", "", "", "", "", "", ""},
                                    {"", "", "", "", "Low byte of the address of the routine at the time of timeout: ", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "Specified interleave: ", "", "", "", "", "", "Middle byte of the requested logical block: ", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""}};



char acceptableHex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'a', 'b', 'c', 'd', 'e', 'f'};

byte readCommand = 0x00;
byte writeCommand = 0x01;
byte writeVerify = 0x02;
byte writeForceSparing = 0x03;

byte defaultRetries = 0x0A;
byte defaultSpareThreshold = 0x03;

String command;
String inputCommand;
char userInput;

const int red = 32;
const int green = 4;
//const int blue = 33;

byte fence[] = {0xF0, 0x78, 0x3C, 0x1E};

void setup(){
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  //pinMode(blue, OUTPUT);
  Serial.begin(115200);
  Serial.setTimeout(10);
  setPinModes();
  setParallelDir(0);
  //DDRC = B00011101; NOTE: ESP32 version of this has been moved into initPins().
  initPins();
  mainMenu();
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

bool readSerialValue(int desiredLength, bool zeroOkay=false, bool minusAllowed=false){
  serialBytes[desiredLength] = 0x00;
  bool minusFound = false;
  char buffer[1024];
  int length = 0;
  while(1){
    if(Serial.available()){
      char c = Serial.read();
      bool inArray = false;
      for(int i = 0; i < 22; i++){
        if((minusAllowed == true) and (c == '-') and (length == 0)){
          minusFound = true;
          inArray = true;
        }
        if(c == acceptableHex[i]){
          inArray = true;
        }
      }
      if(c == '\r'){
        inArray = true;
      }


      if(inArray == false){
        length = 0;
        delay(50);
        flushInput();
        //Serial.println("false not in array");

        return false;
      }

      if (c == '\r'){
        if(length > desiredLength or (length == 0 and zeroOkay == false)){ // !=
          length = 0;
          flushInput();
          //Serial.println("false wrong length");
          return false;
        }
        if(length < desiredLength){
          char tempBuffer[1024];
          for(int i = 0; i < 1024; i++){
            tempBuffer[i] = '0';
          }
          for(int i = 0; i < length; i++){
            tempBuffer[i + desiredLength - length] = buffer[i];
          }
          for(int i = 0; i < desiredLength; i++){
            buffer[i] = tempBuffer[i];
          }
        }
        buffer[desiredLength] = '\0';
        unsigned int byte_count = desiredLength/2;
        hex2bin(serialBytes, buffer, &byte_count);
        if((minusAllowed == true) and (minusFound == true)){
          serialBytes[0] = '-';
        }
        if(length == 0 and zeroOkay == true){
          serialBytes[desiredLength] = 0x55;
        }
        length = 0;
        flushInput();
        return true;
      }
      else if ((length < 1023) and (c != '-')) {
        buffer[length++] = c;
      }
    }
  }
}

bool profileRead(byte address0, byte address1, byte address2, byte retryCount=defaultRetries, byte spareThreshold=defaultSpareThreshold){
  isTenMegDiagCommand = false;
  isWidgetCommand = false;
  setLEDColor(0, 0, 0);
  bool handshakeSuccessful = profileHandshake();
  byte commandResponse = sendCommandBytes(readCommand, address0, address1, address2, retryCount, spareThreshold);
  readStatusBytes();
  readData();
  if(handshakeSuccessful == 0){
    Serial.println(F("Handshake failed!"));
    setLEDColor(1, 0, 0);
    return false;
  }
  if(commandResponse != 0x02 and commandResponseMatters){
    Serial.println(F("Command confirmation failed!"));
    setLEDColor(1, 0, 0);
    return false;
  }
  setLEDColor(0, 1, 0);
  return true;
}


bool profileWrite(byte address0, byte address1, byte address2, byte retryCount=defaultRetries, byte spareThreshold=defaultSpareThreshold){
  isTenMegDiagCommand = false;
  isWidgetCommand = false;
  setLEDColor(0, 0, 0);
  bool handshakeSuccessful = profileHandshake();
  byte commandResponse = sendCommandBytes(writeCommand, address0, address1, address2, retryCount, spareThreshold);
  writeData(532);
  readStatusBytes();
  if(handshakeSuccessful == 0){
    Serial.println(F("Handshake failed!"));
    setLEDColor(1, 0, 0);
    return false;
  }
  if(commandResponse != 0x03 and commandResponseMatters){
    Serial.println(F("Command confirmation failed!"));
    setLEDColor(1, 0, 0);
    return false;
  }
  setLEDColor(0, 1, 0);
  return true;
}

bool customProfileRead(byte commandByte, byte address0, byte address1, byte address2, byte retryCount=defaultRetries, byte spareThreshold=defaultSpareThreshold, bool lowLevelFormat=false){
  isTenMegDiagCommand = false;
  isWidgetCommand = false;
  byte desiredCommandResponse = commandByte + 2;
  setLEDColor(0, 0, 0);
  bool handshakeSuccessful = profileHandshake();
  byte commandResponse = sendCommandBytes(commandByte, address0, address1, address2, retryCount, spareThreshold);
  if(lowLevelFormat == true){
    clearCMD();
    while(readBsy() != 1);
  }
  readStatusBytes();
  readData();
  if(handshakeSuccessful == 0){
    Serial.println(F("Handshake failed!"));
    setLEDColor(1, 0, 0);
    return false;
  }
  if(commandResponse != desiredCommandResponse and commandResponseMatters){
    Serial.println(F("Command confirmation failed!"));
    setLEDColor(1, 0, 0);
    return false;
  }
  setLEDColor(0, 1, 0);
  return true;
}


bool customProfileWrite(byte commandByte, byte address0, byte address1, byte address2, byte retryCount=defaultRetries, byte spareThreshold=defaultSpareThreshold, uint16_t writeBytes=532){
  isTenMegDiagCommand = false;
  isWidgetCommand = false;
  byte desiredCommandResponse = commandByte + 2;
  setLEDColor(0, 0, 0);
  bool handshakeSuccessful = profileHandshake();
  byte commandResponse = sendCommandBytes(commandByte, address0, address1, address2, retryCount, spareThreshold);
  writeData(writeBytes);
  readStatusBytes();
  if(handshakeSuccessful == 0){
    Serial.println(F("Handshake failed!"));
    setLEDColor(1, 0, 0);
    return false;
  }
  if(commandResponse != desiredCommandResponse and commandResponseMatters){
    Serial.println(F("Command confirmation failed!"));
    setLEDColor(1, 0, 0);
    return false;
  }
  setLEDColor(0, 1, 0);
  return true;
}

bool profileWriteVerify(byte address0, byte address1, byte address2, byte retryCount=defaultRetries, byte spareThreshold=defaultSpareThreshold){
  isTenMegDiagCommand = false;
  isWidgetCommand = false;
  setLEDColor(0, 0, 0);
  bool handshakeSuccessful = profileHandshake();
  byte commandResponse = sendCommandBytes(writeVerify, address0, address1, address2, retryCount, spareThreshold);
  writeData(532);
  readStatusBytes();
  if(handshakeSuccessful == 0){
    Serial.println(F("Handshake failed!"));
    setLEDColor(1, 0, 0);
    return false;
  }
  if(commandResponse != 0x04 and commandResponseMatters){
    Serial.println(F("Command confirmation failed!"));
    setLEDColor(1, 0, 0);
    return false;
  }
  setLEDColor(0, 1, 0);
  return true;
}

bool tenMegDiagRead(bool actuallyReadData=true, bool commandTimeout=true){
  isTenMegDiagCommand = true;
  isWidgetCommand = false;
  setLEDColor(0, 0, 0);
  bool handshakeSuccessful = profileHandshake();
  byte commandResponse = sendTenMegDiagCommandBytes();
  if(commandTimeout == false){
    clearCMD();
    while(readBsy() != 1);
  }
  readStatusBytes();
  if(actuallyReadData == true){
    readData();
  }
  clearCMD();
  if(handshakeSuccessful == 0){
    Serial.println(F("Handshake failed!"));
    setLEDColor(1, 0, 0);
    return false;
  }
  if((commandResponse != commandBufferTenMegDiag[1] + 2) and commandResponseMatters){
    Serial.println(F("Command confirmation failed!"));
    setLEDColor(1, 0, 0);
    return false;
  }
  setLEDColor(0, 1, 0);
  return true;
}


bool tenMegDiagWrite(bool commandTimeout=true){
  isTenMegDiagCommand = true;
  isWidgetCommand = false;
  setLEDColor(0, 0, 0);
  bool handshakeSuccessful = profileHandshake();
  byte commandResponse = sendTenMegDiagCommandBytes();
  writeData(532);
  if(commandTimeout == false){
    clearCMD();
    while(readBsy() != 1);
  }
  readStatusBytes();
  if(handshakeSuccessful == 0){
    Serial.println(F("Handshake failed!"));
    setLEDColor(1, 0, 0);
    return false;
  }
  if((commandResponse != commandBufferTenMegDiag[1] + 2) and commandResponseMatters){
    Serial.println(F("Command confirmation failed!"));
    setLEDColor(1, 0, 0);
    return false;
  }
  setLEDColor(0, 1, 0);
  return true;
}

byte sendTenMegDiagCommandBytes(){ //Returns the response byte from the profile or 0xFF if the repsonse is invalid
  byte commandResponse = commandBufferTenMegDiag[1] + 2;
  while(readBsy() != 1){

  }
  byte success;
  setParallelDir(1);
  delayMicroseconds(1);
  setSTRB(); //For some reason, STRB seems to be active high during the command byte phase of the transfer, so set it low to start this phase
  //delay(1);
  for(int i = 0; i < commandBufferTenMegDiag[0] + 1; i++){
    sendData(commandBufferTenMegDiag[i]); //Send command byte 0 (command type) and pulse the strobe
    clearSTRB();
    setSTRB();
  }
  clearSTRB(); //Now the strobe is back in active-low mode again, so clear it after the command bytes are sent
  //delay(1);
  clearRW(); //We want to read the status bytes from the drive now, so set R/W to read mode
  //delay(1);
  setCMD(); //Lower CMD to tell the drive that we're ready for its confirmation byte
  long int startTime = millis();
  setParallelDir(0);
  delayMicroseconds(1);
  while(1){
    success = receiveData();
    if((success == commandResponse) and readBsy() == 0){
      break;
    }
    if(millis() - startTime >= 5000){
      //Serial.println(F("Command Confirmation Failed!!!")); //If more than 5 seconds pass and the drive hasn't responded with an $02, halt the program
      success = 0xFF;
      break;
    }
  }

  //delay(1);
  setSTRB(); //Acknowledge that we read the $02 confirmation by pulsing the strobe
  setParallelDir(0);
  delayMicroseconds(1);
  success = receiveData();
  //delay(1);
  clearSTRB();
  setRW(); //Tell the drive that we're writing to the bus and respond to its $02 with a $55
  setParallelDir(1);
  delayMicroseconds(1);
  sendData(0x55);
  //delay(1);
  clearCMD();
  return success;
}

void calcChecksum(){
  byte sum = 0x00;
  for(int i = 0; i < commandBufferTenMegDiag[0]; i++){
    sum += commandBufferTenMegDiag[i];
  }
  commandBufferTenMegDiag[commandBufferTenMegDiag[0]] = ~sum; 
}

//Widget Command Format: Command, Instruction, Params, Checksum. 
//Upper nybble of Command is 1 for diag commands and 2 for SysRead, SysWrite, and SysWriteVerify.
//Lower nybble of Command is command length. Just like the 10MB ProFile, this excludes the command byte but includes the checksum.
//The Instruction is the actual command itself.
//As with the 10MB ProFile, the number of Params varies between commands.
//The Checksum is calculated identically to how it's done on the 10MB ProFile (sum all previous bytes and then invert the sum).


bool widgetRead(bool actuallyReadData=true, bool commandTimeout=true){
  isWidgetCommand = true;
  isTenMegDiagCommand = false;
  setLEDColor(0, 0, 0);
  bool handshakeSuccessful = profileHandshake();
  byte commandResponse = sendWidgetCommandBytes();
  if(commandTimeout == false){
    clearCMD();
    while(readBsy() != 1);
  }
  readStatusBytes();
  if(actuallyReadData == true){
    readData();
  }
  clearCMD();
  if(handshakeSuccessful == 0){
    Serial.println(F("Handshake failed!"));
    setLEDColor(1, 0, 0);
    return false;
  }
  if((commandResponse != commandBufferWidget[1] + 2) and commandResponseMatters){
    Serial.println(F("Command confirmation failed!"));
    setLEDColor(1, 0, 0);
    return false;
  }
  setLEDColor(0, 1, 0);
  return true;
}


bool widgetWrite(bool commandTimeout=true){
  isWidgetCommand = true;
  isTenMegDiagCommand = false;
  setLEDColor(0, 0, 0);
  bool handshakeSuccessful = profileHandshake();
  byte commandResponse = sendWidgetCommandBytes();
  writeData(532);
  if(commandTimeout == false){
    clearCMD();
    while(readBsy() != 1);
  }
  readStatusBytes();
  if(handshakeSuccessful == 0){
    Serial.println(F("Handshake failed!"));
    setLEDColor(1, 0, 0);
    return false;
  }
  if((commandResponse != commandBufferWidget[1] + 2) and commandResponseMatters){
    Serial.println(F("Command confirmation failed!"));
    setLEDColor(1, 0, 0);
    return false;
  }
  setLEDColor(0, 1, 0);
  return true;
}

byte sendWidgetCommandBytes(){ //Returns the response byte from the profile or 0xFF if the repsonse is invalid
  byte commandResponse = commandBufferWidget[1] + 2;
  while(readBsy() != 1){

  }
  byte success;
  setParallelDir(1);
  delayMicroseconds(1);
  setSTRB(); //For some reason, STRB seems to be active high during the command byte phase of the transfer, so set it low to start this phase
  //delay(1);
  for(int i = 0; i < (commandBufferWidget[0] & 0x0F) + 1; i++){
    sendData(commandBufferWidget[i]); //Send command byte 0 (command type) and pulse the strobe
    clearSTRB();
    setSTRB();
  }
  clearSTRB(); //Now the strobe is back in active-low mode again, so clear it after the command bytes are sent
  //delay(1);
  clearRW(); //We want to read the status bytes from the drive now, so set R/W to read mode
  //delay(1);
  setCMD(); //Lower CMD to tell the drive that we're ready for its confirmation byte
  long int startTime = millis();
  setParallelDir(0);
  delayMicroseconds(1);
  while(1){
    success = receiveData();
    if((success == commandResponse) and readBsy() == 0){
      break;
    }
    if(millis() - startTime >= 5000){
      //Serial.println(F("Command Confirmation Failed!!!")); //If more than 5 seconds pass and the drive hasn't responded with an $02, halt the program
      success = 0xFF;
      break;
    }
  }

  //delay(1);
  setSTRB(); //Acknowledge that we read the $02 confirmation by pulsing the strobe
  setParallelDir(0);
  delayMicroseconds(1);
  success = receiveData();
  //delay(1);
  clearSTRB();
  setRW(); //Tell the drive that we're writing to the bus and respond to its $02 with a $55
  setParallelDir(1);
  delayMicroseconds(1);
  sendData(0x55);
  //delay(1);
  clearCMD();
  return success;
}

void calcWidgetChecksum(){
  byte sum = 0x00;
  for(int i = 0; i < (commandBufferWidget[0] & 0x0F); i++){
    sum += commandBufferWidget[i];
  }
  commandBufferWidget[commandBufferWidget[0] & 0x0F] = ~sum; 
}

bool diagMenu = false;
bool diagMenuTenMeg = false;
bool widgetMenu = false;
bool widgetServoMenu = false;

void mainMenu(){
  clearScreen();
  setLEDColor(0, 1, 0);
  Serial.println(F("Welcome to the ArduinoFile!"));
  Serial.println(F("By: Alex Anderson-McLeod"));
  Serial.println(F("Firmware Version D2.0"));
  Serial.println(F("If you find any bugs, please email me at alexelectronicsguy@gmail.com!"));
  Serial.println();
  Serial.println(F("1 - Reset Drive"));
  Serial.println(F("2 - Get Drive Info"));
  Serial.println(F("3 - Read Block Into Data Buffer"));
  Serial.println(F("4 - Modify Data Buffer"));
  Serial.println(F("5 - Fill Buffer With Pattern"));
  Serial.println(F("6 - Write Buffer to Block"));
  Serial.println(F("7 - Write-Verify Buffer to Block"));
  Serial.println(F("8 - Write Buffer to Entire Drive"));
  Serial.println(F("9 - Write Zeros to Drive"));
  Serial.println(F("A - Compare Every Block With Buffer"));
  Serial.println(F("B - Search Entire Drive For String"));
  Serial.println(F("C - Backup Entire Drive"));
  Serial.println(F("D - Restore Drive From Backup"));
  Serial.println(F("E - Show Command, Status, and Data Buffers"));
  Serial.println(F("F - Send Custom Command"));
  Serial.println(F("G - Drive Tests"));
  Serial.println(F("H - 5MB ProFile Diagnostic Z8 Commands"));
  Serial.println(F("I - 10MB ProFile Diagnostic Z8 Commands"));
  Serial.println(F("J - Widget-Specific Commands"));
  Serial.println();
  Serial.println(F("Note: All numbers are in hex unless otherwise specified."));
  Serial.print(F("Please select an option: "));
}

void testSubMenu(){
  clearScreen();
  setLEDColor(0, 1, 0);
  Serial.println(F("Welcome to the ArduinoFile!"));
  Serial.println(F("By: Alex Anderson-McLeod"));
  Serial.println(F("Firmware Version D2.0"));
  Serial.println(F("If you find any bugs, please email me at alexelectronicsguy@gmail.com!"));
  Serial.println();
  Serial.println(F("Drive Tests"));
  Serial.println(F("1 - Sequential Read"));
  Serial.println(F("2 - Sequential Write"));
  Serial.println(F("3 - Repeatedly Read Block"));
  Serial.println(F("4 - Repeatedly Write Block"));
  Serial.println(F("5 - Random Read"));
  Serial.println(F("6 - Random Write"));
  Serial.println(F("7 - Butterfly Read"));
  Serial.println(F("8 - Butterfly Write"));
  Serial.println(F("9 - Read-Write-Read"));
  Serial.println(F("A - Return to Main Menu"));
  Serial.println();
  Serial.println(F("Note: All numbers are in hex unless otherwise specified."));
  Serial.print(F("Please select an option: "));
}

void Z8SubMenu(){
  clearScreen();
  setLEDColor(0, 1, 0);
  Serial.println(F("Welcome to the ArduinoFile!"));
  Serial.println(F("By: Alex Anderson-McLeod"));
  Serial.println(F("Firmware Version D2.0"));
  Serial.println(F("If you find any bugs, please email me at alexelectronicsguy@gmail.com!"));
  Serial.println();
  Serial.println(F("5MB ProFile Diagnostic Z8 Commands"));
  Serial.println(F("1 - Read CHS Into Data Buffer"));
  Serial.println(F("2 - Write Buffer to CHS"));
  Serial.println(F("3 - Repeatedly Read CHS"));
  Serial.println(F("4 - Repeatedly Write CHS"));
  Serial.println(F("5 - Write-Verify Buffer to CHS"));
  Serial.println(F("6 - Low-Level Format"));
  Serial.println(F("7 - Read Drive RAM Into Buffer"));
  Serial.println(F("8 - Write Buffer Contents to Drive RAM"));
  Serial.println(F("9 - Format"));
  Serial.println(F("A - Scan"));
  Serial.println(F("B - Init Spare Table"));
  Serial.println(F("C - Disable Head Stepper"));
  Serial.println(F("D - Modify Data Buffer"));
  Serial.println(F("E - Fill Buffer With Pattern"));
  Serial.println(F("F - Show Command, Status, and Data Buffers"));
  Serial.println(F("G - Return to Main Menu"));
  Serial.println();
  Serial.println(F("Note: All numbers are in hex unless otherwise specified."));
  Serial.print(F("Please select an option: "));
}

void tenMegZ8SubMenu(){
  clearScreen();
  setLEDColor(0, 1, 0);
  Serial.println(F("Welcome to the ArduinoFile!"));
  Serial.println(F("By: Alex Anderson-McLeod"));
  Serial.println(F("Firmware Version D2.0"));
  Serial.println(F("If you find any bugs, please email me at alexelectronicsguy@gmail.com!"));
  Serial.println();
  Serial.println(F("10MB ProFile Diagnostic Z8 Commands"));
  Serial.println(F("1 - Read CHS Into Data Buffer"));
  Serial.println(F("2 - Write Buffer to CHS"));
  Serial.println(F("3 - Repeatedly Read CHS"));
  Serial.println(F("4 - Repeatedly Write CHS"));
  Serial.println(F("5 - Seek Heads"));
  Serial.println(F("6 - Low-Level Format"));
  Serial.println(F("7 - Format Track(s)"));
  Serial.println(F("8 - Scan"));
  Serial.println(F("9 - Init Spare Table"));
  Serial.println(F("A - Test ProFile RAM"));
  Serial.println(F("B - Read Header"));
  Serial.println(F("C - Erase Track(s)"));
  Serial.println(F("D - Get Result Table"));
  Serial.println(F("E - Disable Head Stepper"));
  Serial.println(F("F - Park Heads (Do This Before Exiting!)"));
  Serial.println(F("G - Send Custom 10MB Diagnostic Command"));
  Serial.println(F("H - Modify Data Buffer"));
  Serial.println(F("I - Fill Buffer With Pattern"));
  Serial.println(F("J - Show Command, Status, and Data Buffers"));
  Serial.println(F("K - Return to Main Menu"));
  Serial.println();
  Serial.println(F("Note: All numbers are in hex unless otherwise specified."));
  Serial.print(F("Please select an option: "));
}

void widgetSubMenu(){
  clearScreen();
  setLEDColor(0, 1, 0);
  Serial.println(F("Welcome to the ArduinoFile!"));
  Serial.println(F("By: Alex Anderson-McLeod"));
  Serial.println(F("Firmware Version D2.0"));
  Serial.println(F("If you find any bugs, please email me at alexelectronicsguy@gmail.com!"));
  Serial.println();
  Serial.println(F("Widget-Specific Commands"));
  Serial.println(F("1 - Soft Reset"));
  Serial.println(F("2 - Reset Servo"));
  Serial.println(F("3 - Get Drive ID"));
  Serial.println(F("4 - Read Spare Table"));
  Serial.println(F("5 - Get Controller Status"));
  Serial.println(F("6 - Get Servo Status"));
  Serial.println(F("7 - Get Abort Status"));
  Serial.println(F("8 - Diagnostic Read"));
  Serial.println(F("9 - Diagnostic Write"));
  Serial.println(F("A - Read Header"));
  Serial.println(F("B - Write Buffer to Spare Table"));
  Serial.println(F("C - Seek"));
  Serial.println(F("D - Send Servo Command"));
  Serial.println(F("E - Send Restore"));
  Serial.println(F("F - Set Recovery"));
  Serial.println(F("G - Set Auto-Offset"));
  Serial.println(F("H - View Track Offsets"));
  Serial.println(F("I - Low-Level Format"));
  Serial.println(F("J - Format Track(s)"));
  Serial.println(F("K - Init Spare Table"));
  Serial.println(F("L - Scan"));
  Serial.println(F("M - Park Heads (Do This Before Exiting!)"));
  Serial.println(F("N - Send Custom Widget Command"));
  Serial.println(F("O - Modify Data Buffer"));
  Serial.println(F("P - Fill Buffer With Pattern"));
  Serial.println(F("Q - Show Command, Status, and Data Buffers"));
  Serial.println(F("R - Return to Main Menu"));
  Serial.println();
  Serial.println(F("Note: All numbers are in hex unless otherwise specified."));
  Serial.print(F("Please select an option: "));
}

void servoSubMenu(){
  clearScreen();
  setLEDColor(0, 1, 0);
  Serial.println(F("Welcome to the ArduinoFile!"));
  Serial.println(F("By: Alex Anderson-McLeod"));
  Serial.println(F("Firmware Version D2.0"));
  Serial.println(F("If you find any bugs, please email me at alexelectronicsguy@gmail.com!"));
  Serial.println();
  Serial.println(F("Widget Servo Commands"));
  Serial.println(F("1 - Soft Reset"));
  Serial.println(F("2 - Reset Servo"));
  Serial.println(F("3 - Get Controller Status"));
  Serial.println(F("4 - Get Servo Status"));
  Serial.println(F("5 - Get Abort Status"));
  Serial.println(F("6 - Read Header"));
  Serial.println(F("7 - Recal"));
  Serial.println(F("8 - Access"));
  Serial.println(F("9 - Access With Offset"));
  Serial.println(F("A - Offset"));
  Serial.println(F("B - Home (Issue a soft reset after this to get the drive working again!)"));
  Serial.println(F("C - Show Command, Status, and Data Buffers"));
  Serial.println(F("D - Return to Widget Menu"));
  Serial.println();
  Serial.println(F("Note: All numbers are in hex unless otherwise specified."));
  Serial.print(F("Please select an option: "));
}

int readDelay = 1;

void clearScreen(){
  Serial.write(27);
  Serial.print(F("[2J"));
  Serial.write(27);
  Serial.print(F("[H"));
}

void flushInput(){
  while(Serial.available()){
    x = Serial.read();
  }
}


void printStatus(){
  bool statusGood = 1;
  Serial.print(F("Drive Status: "));
  for(int i = 0; i < 4; i++){
    printRawBinary(driveStatus[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println();
  Serial.println(F("Status Interpretation:"));
  for(int i = 0; i < 3; i++){
    for(int j = 0; j < 8; j++){
      if(bitRead(driveStatus[i], j) == 1){
        statusGood = 0;
        Serial.println(statusMessages[i][j]);
      }
    }
  }
  if(statusGood == 1){
    Serial.println(F("Status looks good!"));
  }
  if((driveStatus[3] | B00000000) != B00000000){
    Serial.print(F("Number of Retries: "));
    Serial.print(driveStatus[3]>>4, HEX);
    Serial.print(driveStatus[3]&0x0F, HEX);
    Serial.println();
  }
  Serial.println();
}

void printWidgetStatus(){
  bool statusGood = 1;
  Serial.print(F("Drive Status: "));
  for(int i = 0; i < 4; i++){
    printRawBinary(driveStatus[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println();
  Serial.println(F("Status Interpretation:"));
  for(int i = 0; i < 3; i++){
    for(int j = 0; j < 8; j++){
      if(bitRead(driveStatus[i], j) == 1){
        statusGood = 0;
        Serial.println(widgetStatusMessages[i][j]);
      }
    }
  }
  if(bitRead(driveStatus[3], 4) == 1){
    Serial.println("This bit is unused");
  }
  if(bitRead(driveStatus[3], 5) == 1){
    Serial.println("Header Timeout on Last Read");
  }
  if(bitRead(driveStatus[3], 6) == 1){
    Serial.println("Read Error Detected By CRC Circuitry");
  }
  if(bitRead(driveStatus[3], 7) == 1){
    Serial.println("Read Error Detected By ECC Circuitry");
  }
  if(statusGood == 1){
    Serial.println(F("Status looks good!"));
  }
  if((driveStatus[3] & 0x0F) != 0){
    Serial.print(F("Number of Retries: "));
    printDataNoSpace(driveStatus[3] & 0x0F);
    Serial.println();
  }
  Serial.println();
}


void loop() {
  /*pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(15, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(17, OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(19, OUTPUT);
  
  while(1){
    for(byte i = 0x00; i <= 0xFF; i++){
      sendData(i);
    }
  }*/
  
  if(Serial.available()){
    command = Serial.readStringUntil('\r');
    command.trim();
    if(command.equals("1") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false and widgetServoMenu == false){
      setLEDColor(0, 0, 0);
      clearScreen();
      Serial.println(F("Resetting drive..."));
      resetDrive();
      Serial.print(F("Reset successful! Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }
    else if(command.equals("2") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false and widgetServoMenu == false){
      clearScreen();
      Serial.println(F("Reading drive info..."));
      Serial.println(F("Command: 00 FF FF FF 0A 03"));
      bool readSuccess = profileRead(0xFF, 0xFF, 0xFF);
      if(readSuccess == 0){
        Serial.println();
        Serial.println(F("WARNING: Errors were encountered during the read operation. The following data may be incorrect."));
      }
      /*for(int i = 0; i < 532; i++){
        if(checkParity(blockData[i], parity[i]) == false){
          Serial.println("The parity was all screwed up!");
          printDataNoSpace(blockData[i]);
          Serial.println();
        }
      }*/
      Serial.println();
      printRawData();
      Serial.println();
      //printRawParity();
      //Serial.println();
      printStatus();
      Serial.println(F("Data Analysis:"));
      Serial.print(F("Device Name: "));
      for(int i = 0; i < 13; i++){
        Serial.write(blockData[i]);
      }
      Serial.println();
      Serial.print(F("Device Number: "));
      for(int i = 13; i < 16; i++){
        Serial.print(blockData[i]>>4, HEX);
        Serial.print(blockData[i]&0x0F, HEX);
      }
      bool widgetInterpretation = false;
      if(blockData[13] == 0x00 and blockData[14] == 0x00 and blockData[15] == 0x00){
        Serial.println(F(" (5MB ProFile)"));
      }
      else if(blockData[13] == 0x00 and blockData[14] == 0x00 and blockData[15] == 0x10){
        Serial.println(F(" (10MB ProFile)"));
      }
      else if(blockData[13] == 0x00 and blockData[14] == 0x01 and blockData[15] == 0x00){
        Serial.println(F(" (10MB Widget)"));
        widgetInterpretation = true;
      }
      else if(blockData[13] == 0x00 and blockData[14] == 0x01 and blockData[15] == 0x10){
        Serial.println(F(" (20MB Widget)"));
        widgetInterpretation = true;
      }
      else if(blockData[13] == 0x00 and blockData[14] == 0x01 and blockData[15] == 0x20){
        Serial.println(F(" (40MB Widget)"));
        widgetInterpretation = true;
      }
      else{
        Serial.println(F(" (Unknown Drive Type)"));
      }
      Serial.print(F("Firmware Revision: "));
      Serial.print(blockData[16], HEX);
      Serial.print(F("."));
      Serial.print(blockData[17]>>4, HEX);
      Serial.println(blockData[17]&0x0F, HEX);
      Serial.print(F("Total Blocks: "));
      for(int i = 18; i < 21; i++){
        Serial.print(blockData[i]>>4, HEX);
        Serial.print(blockData[i]&0x0F, HEX);
      }
      Serial.println();
      Serial.print(F("Bytes Per Block: "));
      for(int i = 21; i < 23; i++){
        Serial.print(blockData[i]>>4, HEX);
        Serial.print(blockData[i]&0x0F, HEX);
      }
      if(widgetInterpretation == true){
        Serial.println();
        Serial.print(F("Number of Cylinders: "));
        printDataNoSpace(blockData[23]);
        printDataNoSpace(blockData[24]);
        Serial.println();
        Serial.print(F("Number of Heads: "));
        printDataNoSpace(blockData[25]);
        Serial.println();
        Serial.print(F("Sectors Per Track: "));
        printDataNoSpace(blockData[26]);
        Serial.println();
        Serial.print(F("Total Spares: "));
        printDataNoSpace(blockData[27]);
        printDataNoSpace(blockData[28]);
        printDataNoSpace(blockData[29]);
        Serial.println();
        Serial.print(F("Spares Allocated: "));
        printDataNoSpace(blockData[30]);
        printDataNoSpace(blockData[31]);
        printDataNoSpace(blockData[32]);
        Serial.println();
        Serial.print(F("Bad Blocks: "));
        printDataNoSpace(blockData[33]);
        printDataNoSpace(blockData[34]);
        printDataNoSpace(blockData[35]);
        Serial.println();
        Serial.println();
      }
      else{
        Serial.println();
        Serial.print(F("Total Spares: "));
        Serial.print(blockData[23]>>4, HEX);
        Serial.print(blockData[23]&0x0F, HEX);
        Serial.println();
        Serial.print(F("Spares Allocated: "));
        Serial.print(blockData[24]>>4, HEX);
        Serial.print(blockData[24]&0x0F, HEX);
        Serial.println();
        Serial.print(F("Bad Blocks: "));
        Serial.print(blockData[25]>>4, HEX);
        Serial.print(blockData[25]&0x0F, HEX);
        Serial.println();
        int spareBlockIndex = 0;
        if(blockData[24] != 0){
          Serial.print(F("List of Spared Blocks: "));
        }
        bool firstTime = true;
        while((spareBlockIndex < blockData[24]) and ((blockData[26 + spareBlockIndex*3] != 0xFF) or (blockData[27 + spareBlockIndex*3] != 0xFF) or (blockData[28 + spareBlockIndex*3] != 0xFF))){
          if(firstTime == true){
            firstTime = false;
          }
          else{
            Serial.print(F(", "));
          }
          printDataNoSpace(blockData[26 + spareBlockIndex*3]);
          printDataNoSpace(blockData[27 + spareBlockIndex*3]);
          printDataNoSpace(blockData[28 + spareBlockIndex*3]);
          spareBlockIndex += 1;
        }
        if(blockData[24] != 0){
          Serial.println();
        }
        firstTime = true;
        int badBlockIndex = 0;
        spareBlockIndex += 1;
        if(blockData[25] != 0){
          Serial.print(F("List of Bad Blocks: "));
        }
        while((badBlockIndex < blockData[25]) and ((blockData[26 + badBlockIndex*3 + spareBlockIndex*3] != 0xFF) or (blockData[27 + badBlockIndex*3 + spareBlockIndex*3] != 0xFF) or (blockData[28 + badBlockIndex*3 + spareBlockIndex*3] != 0xFF))){
          if(firstTime == true){
            firstTime = false;
          }
          else{
            Serial.print(F(", "));
          }
          printDataNoSpace(blockData[26 + badBlockIndex*3 + spareBlockIndex*3]);
          printDataNoSpace(blockData[27 + badBlockIndex*3 + spareBlockIndex*3]);
          printDataNoSpace(blockData[28 + badBlockIndex*3 + spareBlockIndex*3]);
          badBlockIndex += 1;
        }
        if(blockData[25] != 0){
          Serial.println();
        }
        Serial.println();
      }
      setLEDColor(0, 1, 0);
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }
    else if(command.equals("3") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false and widgetServoMenu == false){
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.print(F("Please enter the block number that you want to read: "));
      while(1){
        if(readSerialValue(6) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the block number that you want to read: "));
        }
      }
      for(int i = 0; i < 3; i++){
        address[i] = serialBytes[i];
      }
      int retries = defaultRetries;
      int spare = defaultSpareThreshold;
      Serial.print(F("Use the default retry count of "));
      printDataNoSpace(retries);
      Serial.print(F(" (return for yes, 'n' for no)? "));
      while(1){
        if(Serial.available()) {
          delay(50);
          userInput = Serial.read();
          flushInput();
          if(userInput == 'n'){
            Serial.print(F("Please enter the desired retry count: "));
            while(1){
              if(readSerialValue(2) == true){
                retries = serialBytes[0];
                break;
              }
              else{
                Serial.print(F("Please enter the desired retry count: "));
              }
            }
            break;
          }
          else if(userInput == '\r'){
            break;
          }
          else{
            while(Serial.available()){
              Serial.read();
            }
            Serial.print(F("Use the default retry count of "));
            printDataNoSpace(retries);
            Serial.print(F(" (return for yes, 'n' for no)? "));
          }
        }
      }
      Serial.print(F("Use the default spare threshold of "));
      printDataNoSpace(spare);
      Serial.print(F(" (return for yes, 'n' for no)? "));
      while(1){
        if(Serial.available()) {
          delay(50);
          userInput = Serial.read();
          flushInput();
          if(userInput == 'n'){
            Serial.print(F("Please enter the desired spare threshold: "));
            while(1){
              if(readSerialValue(2) == true){
                spare = serialBytes[0];
                break;
              }
              else{
                Serial.print(F("Please enter the desired spare threshold: "));
              }
            }
            break;
          }
          else if(userInput == '\r'){
            break;
          }
          else{
            while(Serial.available()){
              Serial.read();
            }
            Serial.print(F("Use the default spare threshold of "));
            printDataNoSpace(spare);
            Serial.print(F(" (return for yes, 'n' for no)? "));
          }
        }
      }
      Serial.println();
      Serial.print(F("Reading block "));
      for(int i = 0; i < 3; i++){
        printDataNoSpace(address[i]);
      }
      Serial.println(F("..."));
      Serial.print(F("Command: 00 "));
      for(int i = 0; i < 3; i++){
        printDataSpace(address[i]);
      }
      printDataSpace(retries);
      printDataNoSpace(spare);
      Serial.println();
      Serial.println();
      bool readSuccess = profileRead(address[0], address[1], address[2], retries, spare);
      if(readSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered during the read operation. The following data may be incorrect."));
        Serial.println();
      }
      printRawData();
      Serial.println();
      printStatus();
      Serial.println();



      Serial.print(F("Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }
    if((command.equals("4") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false) or (command.equalsIgnoreCase("O") and widgetMenu == true) or (command.equalsIgnoreCase("D") and diagMenu == true) or (command.equalsIgnoreCase("H") and diagMenuTenMeg == true)){
      clearScreen();
      Serial.println(F("Current contents of the data buffer:"));
      Serial.println();
      printRawData();
      Serial.println();
      uint16_t bufferIndex = 0;
      Serial.print(F("Please enter the address at which you want to modify the buffer: "));
      while(1){
        if(readSerialValue(4) == true){
          bufferIndex = (serialBytes[0] << 8) | serialBytes[1];
            if(bufferIndex < 532){
              break;
            }
        }
        Serial.print(F("Please enter the address at which you want to modify the buffer: "));
      }
      char charInput[2128];
      byte hexInput[1064];
      unsigned int charIndex = 0;
      Serial.print(F("Please enter the data bytes that you wish to insert at that address: "));
      while(1){
        while(1){
          if(Serial.available()){
            char inByte = Serial.read();
            if(inByte == '\r'){
              charInput[charIndex] = '\0';
              hex2bin(hexInput, charInput, &charIndex);
              break;
            }
            charInput[charIndex] = inByte;
            if(charIndex < 2126){
              charIndex++;
            }
          }
        }
        if(bufferIndex + charIndex - 1 < 532){
          break;
        }
        else{
          Serial.print(F("Please enter the data bytes that you wish to insert at that address: "));
          charIndex = 0;
        }
      }
      for(int i = 0; i < charIndex; i++){
        blockData[i + bufferIndex] = hexInput[i];
      }
      Serial.println();
      Serial.println(F("New contents of the data buffer:"));
      Serial.println();
      printRawData();
      Serial.println();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }
    
    if((command.equals("5") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false) or (command.equalsIgnoreCase("P") and widgetMenu == true) or (command.equalsIgnoreCase("E") and diagMenu == true) or (command.equalsIgnoreCase("I") and diagMenuTenMeg == true)){ // fill buffer with pattern
      clearScreen();
      Serial.println(F("Current contents of the data buffer:"));
      Serial.println();
      printRawData();
      Serial.println();
      char charInput[2128];
      byte hexInput[1064];
      unsigned int charIndex = 0;
      Serial.print(F("Please enter the byte pattern you wish to fill the buffer with: "));
      while(1){
        while(1){
          if(Serial.available()){
            char inByte = Serial.read();
            if(inByte == '\r'){
              charInput[charIndex] = '\0';
              hex2bin(hexInput, charInput, &charIndex);
              break;
            }
            charInput[charIndex] = inByte;
            if(charIndex < 2126){
              charIndex++;
            }
          }
        }
        if(charIndex - 1 < 532){
          break;
        }
        else{
          Serial.print(F("Please enter the byte pattern you wish to fill the buffer with: "));
          charIndex = 0;
        }
      }
      for(int i = 0; i < 532; i++){
        blockData[i] = hexInput[i % charIndex];
      }
      Serial.println();
      Serial.println(F("New contents of the data buffer:"));
      Serial.println();
      printRawData();
      Serial.println();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }
    
    else if(command.equals("6") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.print(F("Please enter the block number that you want to write to: "));
      while(1){
        if(readSerialValue(6) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the block number that you want to write to: "));
        }
      }
      for(int i = 0; i < 3; i++){
        address[i] = serialBytes[i];
      }
      Serial.println();
      Serial.print(F("Writing the buffer to block "));
      for(int i = 0; i < 3; i++){
        printDataNoSpace(address[i]);
      }
      Serial.println(F("..."));
      Serial.print(F("Command: 01 "));
      for(int i = 0; i < 3; i++){
        printDataSpace(address[i]);
      }
      printDataSpace(defaultRetries);
      printDataNoSpace(defaultSpareThreshold);
      Serial.println();
      Serial.println();
      bool writeSuccess = profileWrite(address[0], address[1], address[2]);
      if(writeSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered during the write operation. The data may have been written incorrectly."));
        Serial.println();
        setLEDColor(1, 0, 0);
      }
      else{
        setLEDColor(0, 1, 0);
      }
      printStatus();
      Serial.println();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equals("7") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){ //write-verify block
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.print(F("Please enter the block number that you want to write-verify to: "));
      while(1){
        if(readSerialValue(6) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the block number that you want to write-verify to: "));
        }
      }
      for(int i = 0; i < 3; i++){
        address[i] = serialBytes[i];
      }
      Serial.println();
      Serial.print(F("Write-verifying the buffer to block "));
      for(int i = 0; i < 3; i++){
        printDataNoSpace(address[i]);
      }
      Serial.println(F("..."));
      Serial.print(F("Command: 02 "));
      for(int i = 0; i < 3; i++){
        printDataSpace(address[i]);
      }
      printDataSpace(defaultRetries);
      printDataNoSpace(defaultSpareThreshold);
      Serial.println();
      Serial.println();
      bool writeSuccess = profileWriteVerify(address[0], address[1], address[2]);
      if(writeSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered during the write-verify operation. The data may have been written incorrectly."));
        Serial.println();
        setLEDColor(1, 0, 0);
      }
      else{
        setLEDColor(0, 1, 0);
      }
      printStatus();
      Serial.println();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equals("8") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){ //write buffer to entire drive
      clearScreen();
      setLEDColor(0, 1, 0);
      confirm();
      uint16_t highestBlock = 0;
      bool abort = false;
      flushInput();
      if(confirmOperation == true){
        getDriveType();
        Serial.println();
        highestBlock = (driveSize[0]<<16) | (driveSize[1]<<8) | (driveSize[2]);
        for(long int i = 0; i < highestBlock; i++){
          if(Serial.available()){
            abort = true;
            break;
          }
          driveSize[0] = i >> 16;
          driveSize[1] = i >> 8;
          driveSize[2] = i;
          profileWrite(driveSize[0], driveSize[1], driveSize[2]);
          Serial.print(F("Now writing the contents of the buffer to block "));
          for(int j = 0; j < 3; j++){
            printDataNoSpace(driveSize[j]);
          }
          Serial.print(F(" of "));
          printDataNoSpace((highestBlock - 1) >> 16);
          printDataNoSpace((highestBlock - 1) >> 8);
          printDataNoSpace(highestBlock - 1);
          Serial.print(F(". Progress: "));
          Serial.print(((float)i/(highestBlock - 1))*100);
          Serial.print(F("%"));
          Serial.write("\033[1000D");
          if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
            Serial.println();
            Serial.println();
            Serial.print(F("Error writing block "));
            printDataNoSpace(i >> 16);
            printDataNoSpace(i >> 8);
            printDataNoSpace(i);
            Serial.println(F("!"));
            Serial.println(F("Status Interpretation:"));
            for(int k = 0; k < 3; k++){
              for(int l = 0; l < 8; l++){
                if(bitRead(driveStatus[k], l) == 1){
                  //statusGood = 0;
                  Serial.println(statusMessages[k][l]); //All status errors show up every time
                }
              }
            }
            Serial.println();
          }
        }
        if(abort == true){
          Serial.println();
          Serial.println();
          Serial.println("Operation terminated by keypress.");
        }
      }

      Serial.println();
      setLEDColor(0, 1, 0);
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equals("9") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      clearScreen();
      setLEDColor(0, 1, 0);
      confirm();
      bool abort = false;
      uint16_t highestBlock = 0;
      if(confirmOperation == true){
        getDriveType();
        Serial.println();
        flushInput();
        highestBlock = (driveSize[0]<<16) | (driveSize[1]<<8) | (driveSize[2]);
        for(long int i = 0; i < highestBlock; i++){
          if(Serial.available()){
            abort = true;
            break;
          }
          for(int j = 0; j < 532; j++){
            blockData[j] = 0x00;
          }
          driveSize[0] = i >> 16;
          driveSize[1] = i >> 8;
          driveSize[2] = i;
          profileWrite(driveSize[0], driveSize[1], driveSize[2]);
          Serial.print(F("Now writing zeros to block "));
          for(int j = 0; j < 3; j++){
            printDataNoSpace(driveSize[j]);
          }
          Serial.print(F(" of "));
          printDataNoSpace((highestBlock - 1) >> 16);
          printDataNoSpace((highestBlock - 1) >> 8);
          printDataNoSpace(highestBlock - 1);
          Serial.print(F(". Progress: "));
          Serial.print(((float)i/(highestBlock - 1))*100);
          Serial.print(F("%"));
          Serial.write("\033[1000D");
          if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
            Serial.println();
            Serial.println();
            Serial.print(F("Error writing block "));
            printDataNoSpace(i >> 16);
            printDataNoSpace(i >> 8);
            printDataNoSpace(i);
            Serial.println(F("!"));
            Serial.println(F("Status Interpretation:"));
            for(int k = 0; k < 3; k++){
              for(int l = 0; l < 8; l++){
                if(bitRead(driveStatus[k], l) == 1){
                  //statusGood = 0;
                  Serial.println(statusMessages[k][l]); //All status errors show up every time
                }
              }
            }
            Serial.println();
          }


        }
        if(abort == true){
          Serial.println();
          Serial.println();
          Serial.println("Operation terminated by keypress.");
        }
      }

      Serial.println();
      setLEDColor(0, 1, 0);
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("A") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){ //Compare Blocks With Buffer
      bool noOutliers = true;
      byte comparisonData[532];
      for(int i = 0; i < 532; i++){
        comparisonData[i] = blockData[i];
      }
      clearScreen();
      getDriveType();
      setLEDColor(0, 1, 0);
      bool abort = false;
      uint16_t highestBlock = 0;
      Serial.println();
      highestBlock = (driveSize[0]<<16) | (driveSize[1]<<8) | (driveSize[2]);
      flushInput();
      Serial.write("\033[1000D");
      Serial.print("                                                                                         ");
      Serial.write("\033[1000D");
      for(long int i = 0; i < highestBlock; i++){
        if(Serial.available()){
          abort = true;
          break;
        }
        driveSize[0] = i >> 16;
        driveSize[1] = i >> 8;
        driveSize[2] = i;
        profileRead(driveSize[0], driveSize[1], driveSize[2]);
        Serial.print(F("Now comparing block "));
        for(int j = 0; j < 3; j++){
          printDataNoSpace(driveSize[j]);
        }
        Serial.print(F(" of "));
        printDataNoSpace((highestBlock - 1) >> 16);
        printDataNoSpace((highestBlock - 1) >> 8);
        printDataNoSpace(highestBlock - 1);
        Serial.print(F(" with the buffer. Progress: "));
        Serial.print(((float)i/(highestBlock - 1))*100);
        Serial.print(F("%"));
        Serial.write("\033[1000D");
        bool differentBlock = false;
        for(int i = 0; i < 532; i++){
          if(blockData[i] != comparisonData[i]){
            differentBlock = true;
            break;
          }
        }
        if(differentBlock == true){
          Serial.println();
          Serial.println();
          Serial.print(F("Block "));
          for(int j = 0; j < 3; j++){
            printDataNoSpace(driveSize[j]);
          }
          Serial.print(F(" doesn't match the contents of the data buffer!"));
          noOutliers = false;
          Serial.println();
          Serial.println();
          differentBlock = false;
        }
        if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
          setLEDColor(1, 0, 0);
          Serial.println();
          Serial.println();
          Serial.print(F("Error reading block "));
          printDataNoSpace(i >> 16);
          printDataNoSpace(i >> 8);
          printDataNoSpace(i);
          Serial.println(F("!"));
          Serial.println(F("Status Interpretation:"));
          for(int k = 0; k < 3; k++){
            for(int l = 0; l < 8; l++){
              if(bitRead(driveStatus[k], l) == 1){
                //statusGood = 0;
                Serial.println(statusMessages[k][l]);
              }
            }
          }
          Serial.println();
        }
      }
      if(abort == true){
        Serial.println();
        Serial.println();
        Serial.println("Comparison terminated by keypress.");
      }
      for(int i = 0; i < 532; i++){
        blockData[i] = comparisonData[i];
      }
      Serial.println();
      if(noOutliers == true){
        Serial.println();
        Serial.println(F("All blocks match the buffer!"));
        Serial.println();
      }
      Serial.print(F("Block comparison completed. Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("B") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){ //Search For Blocks
      clearScreen();
      getDriveType();
      setLEDColor(0, 1, 0);
      bool abort = false;
      uint16_t highestBlock = 0;
      Serial.println();
      highestBlock = (driveSize[0]<<16) | (driveSize[1]<<8) | (driveSize[2]);
      flushInput();
      Serial.print(F("Please enter 'h' to search for a series of hex bytes, 's' to search for a text string, or leave this prompt blank to search the drive for the full contents of the data buffer: "));
      int searchMode = 0;
      while(1){
        if(Serial.available()){
          delay(50);
          userInput = Serial.read();
          flushInput();
          if(userInput == 'h'){
            searchMode = 0;
            break;
          }
          else if(userInput == 's'){
            searchMode = 1;
            break;
          }
          else if(userInput == '\r'){
            searchMode = 2;
            break;
          }
          else{
            Serial.print(F("Please enter 'h' to search for a series of hex bytes, 's' to search for a text string, or leave this prompt blank to search the drive for the full contents of the data buffer: "));
            while(Serial.available()){
              Serial.read();
            }
          }
        }
      }
      bool noMatches = true;
      if(searchMode == 0){
        Serial.println();
        Serial.print(F("Please enter the series of hex bytes that you want to search for: "));
        char charInput[2128];
        byte hexInput[1064];
        unsigned int charIndex = 0;
        bool goodInput = true;
        while(1){
          if(Serial.available()){
            char inByte = Serial.read();
            bool inArray = false;
            if(inByte == '\r'){
              inArray = true;
            }
            for(int i = 0; i < 22; i++){
              if(inByte == acceptableHex[i]){
                inArray = true;
              }
            }
            if(inArray == false){
              goodInput = false;
            }
            if(inByte == '\r' and goodInput == true){
              charInput[charIndex] = '\0';
              hex2bin(hexInput, charInput, &charIndex);
              break;
            }
            else if(inByte == '\r' and goodInput == false){
              Serial.print(F("Please enter the series of hex bytes that you want to search for: "));
              charIndex = 0;
              goodInput = true;
            }
            else{
              charInput[charIndex] = inByte;
              if(charIndex < 2126){
                charIndex++;
              }
            }
          }
        }
        Serial.println();
        int stringLength = charIndex;
        byte searchData[532];
        for(int i = 0; i < stringLength; i++){
          searchData[i] = hexInput[i];
        }
        Serial.print(F("Now searching every block on the disk for the hex data "));
        for(int i = 0; i < stringLength; i++){
          printDataSpace(searchData[i]);
        }
        Serial.println(F("..."));
        Serial.println();
        for(long int i = 0; i < highestBlock; i++){
          if(Serial.available()){
            abort = true;
            break;
          }
          driveSize[0] = i >> 16;
          driveSize[1] = i >> 8;
          driveSize[2] = i;
          profileRead(driveSize[0], driveSize[1], driveSize[2]);
          Serial.print(F("Now searching block "));
          for(int j = 0; j < 3; j++){
            printDataNoSpace(driveSize[j]);
          }
          Serial.print(F(" of "));
          printDataNoSpace((highestBlock - 1) >> 16);
          printDataNoSpace((highestBlock - 1) >> 8);
          printDataNoSpace(highestBlock - 1);
          Serial.print(F(" for the provided hex data. Progress: "));
          Serial.print(((float)i/(highestBlock - 1))*100);
          Serial.print(F("%"));
          Serial.write("\033[1000D");
          bool match = false;
          bool searchStart = false;
          for(int i = 0; i < (532 - stringLength); i++){
            if(blockData[i] == searchData[0]){
              for(int j = 0; j < stringLength; j++){
                match = true;
                if(blockData[i + j] != searchData[j]){
                  match = false;
                  break;
                }
              }
              if(match == true){
                break;
              }
            }
          }
          if(match == true){
            noMatches = false;
            Serial.println();
            Serial.println();
            Serial.print(F("Block "));
            for(int j = 0; j < 3; j++){
              printDataNoSpace(driveSize[j]);
            }
            Serial.print(F(" contains the desired data!"));
            Serial.println();
            Serial.println();
          }
          if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
            setLEDColor(1, 0, 0);
            Serial.println();
            Serial.println();
            Serial.print(F("Error reading block "));
            printDataNoSpace(i >> 16);
            printDataNoSpace(i >> 8);
            printDataNoSpace(i);
            Serial.println(F("!"));
            Serial.println(F("Status Interpretation:"));
            for(int k = 0; k < 3; k++){
              for(int l = 0; l < 8; l++){
                if(bitRead(driveStatus[k], l) == 1){
                  //statusGood = 0;
                  Serial.println(statusMessages[k][l]);
                }
              }
            }
            Serial.println();
          }
        }
        if(abort == true){
          Serial.println();
          Serial.println();
          Serial.println("Search terminated by keypress.");
        }
        Serial.println();
        if(noMatches == true){
          Serial.println();
          Serial.println(F("No matches found!"));
          Serial.println();
        }
        else{
          Serial.println();
        }
      }
      else if(searchMode == 1){
        Serial.println();
        Serial.print(F("Please enter the text string that you want to search for: "));
        char charInput[2128];
        byte hexInput[1064];
        unsigned int charIndex = 0;
        bool goodInput = true;
        while(1){
          if(Serial.available()){
            char inByte = Serial.read();

            if(inByte == '\r'){
              charInput[charIndex] = '\0';
              break;
            }

            charInput[charIndex] = inByte;
            if(charIndex < 2126){
              charIndex++;
            }

          }
        }
        Serial.println();
        int stringLength = charIndex;
        byte searchData[532];
        for(int i = 0; i < stringLength; i++){
          searchData[i] = charInput[i];
        }
        Serial.print(F("Now searching every block on the disk for the text string '"));
        for(int i = 0; i < stringLength; i++){
          Serial.write(searchData[i]);
        }
        Serial.println(F("'..."));
        Serial.println();
        for(long int i = 0; i < highestBlock; i++){
          if(Serial.available()){
            abort = true;
            break;
          }
          driveSize[0] = i >> 16;
          driveSize[1] = i >> 8;
          driveSize[2] = i;
          profileRead(driveSize[0], driveSize[1], driveSize[2]);
          Serial.print(F("Now searching block "));
          for(int j = 0; j < 3; j++){
            printDataNoSpace(driveSize[j]);
          }
          Serial.print(F(" of "));
          printDataNoSpace((highestBlock - 1) >> 16);
          printDataNoSpace((highestBlock - 1) >> 8);
          printDataNoSpace(highestBlock - 1);
          Serial.print(F(" for the provided text string. Progress: "));
          Serial.print(((float)i/(highestBlock - 1))*100);
          Serial.print(F("%"));
          Serial.write("\033[1000D");
          bool match = false;
          bool searchStart = false;
          for(int i = 0; i < (532 - stringLength); i++){
            if(blockData[i] == searchData[0]){
              for(int j = 0; j < stringLength; j++){
                match = true;
                if(blockData[i + j] != searchData[j]){
                  match = false;
                  break;
                }
              }
              if(match == true){
                break;
              }
            }
          }
          if(match == true){
            noMatches = false;
            Serial.println();
            Serial.println();
            Serial.print(F("Block "));
            for(int j = 0; j < 3; j++){
              printDataNoSpace(driveSize[j]);
            }
            Serial.print(F(" contains the desired string!"));
            Serial.println();
            Serial.println();
          }
          if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
            setLEDColor(1, 0, 0);
            Serial.println();
            Serial.println();
            Serial.print(F("Error reading block "));
            printDataNoSpace(i >> 16);
            printDataNoSpace(i >> 8);
            printDataNoSpace(i);
            Serial.println(F("!"));
            Serial.println(F("Status Interpretation:"));
            for(int k = 0; k < 3; k++){
              for(int l = 0; l < 8; l++){
                if(bitRead(driveStatus[k], l) == 1){
                  //statusGood = 0;
                  Serial.println(statusMessages[k][l]);
                }
              }
            }
            Serial.println();
          }
        }
        if(abort == true){
          Serial.println();
          Serial.println();
          Serial.println("Search terminated by keypress.");
        }
        Serial.println();
        if(noMatches == true){
          Serial.println();
          Serial.println(F("No matches found!"));
          Serial.println();
        }
        else{
          Serial.println();
        }
      }
      else if(searchMode == 2){
        byte searchData[532];
        for(int i = 0; i < 532; i++){
          searchData[i] = blockData[i];
        }
        Serial.println();
        Serial.write("\033[1000D");
        Serial.print("                                                                                         ");
        Serial.write("\033[1000D");
        for(long int i = 0; i < highestBlock; i++){
          if(Serial.available()){
            abort = true;
            break;
          }
          driveSize[0] = i >> 16;
          driveSize[1] = i >> 8;
          driveSize[2] = i;
          profileRead(driveSize[0], driveSize[1], driveSize[2]);
          Serial.print(F("Now searching block "));
          for(int j = 0; j < 3; j++){
            printDataNoSpace(driveSize[j]);
          }
          Serial.print(F(" of "));
          printDataNoSpace((highestBlock - 1) >> 16);
          printDataNoSpace((highestBlock - 1) >> 8);
          printDataNoSpace(highestBlock - 1);
          Serial.print(F(" for the contents of the buffer. Progress: "));
          Serial.print(((float)i/(highestBlock - 1))*100);
          Serial.print(F("%"));
          Serial.write("\033[1000D");
          bool match = true;
          for(int i = 0; i < 532; i++){
            if(blockData[i] != searchData[i]){
              match = false;
              break;
            }
          }
          if(match == true){
            noMatches = false;
            Serial.println();
            Serial.println();
            Serial.print(F("Block "));
            for(int j = 0; j < 3; j++){
              printDataNoSpace(driveSize[j]);
            }
            Serial.print(F(" matches the contents of the data buffer!"));
            Serial.println();
            Serial.println();
          }
          if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
            setLEDColor(1, 0, 0);
            Serial.println();
            Serial.println();
            Serial.print(F("Error reading block "));
            printDataNoSpace(i >> 16);
            printDataNoSpace(i >> 8);
            printDataNoSpace(i);
            Serial.println(F("!"));
            Serial.println(F("Status Interpretation:"));
            for(int k = 0; k < 3; k++){
              for(int l = 0; l < 8; l++){
                if(bitRead(driveStatus[k], l) == 1){
                  //statusGood = 0;
                  Serial.println(statusMessages[k][l]);
                }
              }
            }
            Serial.println();
          }
        }
        if(abort == true){
          Serial.println();
          Serial.println();
          Serial.println("Search terminated by keypress.");
        }
        for(int i = 0; i < 532; i++){
          blockData[i] = searchData[i];
        }
        Serial.println();
        if(noMatches == true){
          Serial.println();
          Serial.println(F("No matches found!"));
          Serial.println();
        }
        else{
          Serial.println();
        }
      }
      Serial.print(F("Search completed. Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }
 
    else if(command.equalsIgnoreCase("C") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      clearScreen();
      backupErrors = 0;
      getDriveType();
      setLEDColor(0, 1, 0);
      Serial.println();
      Serial.println(F("Start XMODEM receiver now..."));
      delay(2000);
      packetNum = 0x01;
      notPacketNum = 0xFE;
      uint32_t totalBlocks = 0;
      currentIndex = 0;
      currentBlock = 0;
      totalBlocks = (driveSize[0]<<16 | driveSize[1]<<8 | driveSize[2]);
      driveSize[0] = currentBlock >> 16;
      driveSize[1] = currentBlock >> 8;
      driveSize[2] = currentBlock;
      byte successPacket;
      bool failure = false;
      if(startTransmission() == false){
        failure = true;
      }
      while(failure == false){
        startNewPacket();
        for(int i = 0; i < 1024; i++){
          if(currentIndex % 1064 == 0 and currentBlock != totalBlocks + 2){
            driveSize[0] = currentBlock >> 16;
            driveSize[1] = currentBlock >> 8;
            driveSize[2] = currentBlock;
            readTwoBlocks(driveSize[0], driveSize[1], driveSize[2]);
            currentBlock += 2;
          }
          if(currentIndex % 1064 == 0 and currentBlock == totalBlocks + 2){
            for(int j = 0; j < 1064; j++){
              blockData[j] = padding;
            }
            currentBlock += 2;
          }
          crcArray[i] = blockData[currentIndex % 1064];
          Serial.write(blockData[currentIndex % 1064]);
          currentIndex += 1;
          }
        successPacket = finishPacket();
        while(successPacket == 0x01){
          startNewPacket();
          for(int i = 0; i < 1024; i++){
           Serial.write(crcArray[i]);
          }
          successPacket = finishPacket();
        }
        if(successPacket == 0x00){
          failure = true;
        }
        if(currentBlock >= totalBlocks + 2){
          break;
        }
      }
        if(failure == false){
          finishTransmission();
          delay(2000);
          if(backupErrors != 0){
            setLEDColor(1, 0, 0);
            Serial.print(F("Warning: "));
            Serial.print(backupErrors);
            Serial.print(F(" disk errors were encountered during the operation!"));
            Serial.println();
          }
          else{
            setLEDColor(0, 1, 0);
          }
          Serial.println();
          Serial.println(F("XMODEM transfer complete!"));
        }
        else{
          setLEDColor(1, 0, 0);
          Serial.println();
          Serial.println(F("Transfer timed out!"));
        }
        while(Serial.available()){
          inputData = Serial.read();
        }
        Serial.print(F("Press return to continue..."));
        flushInput();
        while(!Serial.available());
        flushInput();
      //}
    }
    
    else if(command.equalsIgnoreCase("D") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      setLEDColor(0, 1, 0);
      clearScreen();
      confirm();
      uint16_t highestBlock = 0;
      backupErrors = 0;
      failed = false;
      packetNum = 0x01;
      notPacketNum = 0xFE;
      currentIndex = 0;
      dataIndex = 0;
      writeIndex = 0;
      actualCRC = 0;
      CRC = 0;
      currentBlock = 0;
      done = 0;
      ackStatus = 0x02;
      failed = false;
      if(confirmOperation == true){
        getDriveType();

        Serial.println();

        highestBlock = (driveSize[0]<<16) | (driveSize[1]<<8) | (driveSize[2]);
        Serial.println(F("Start XMODEM sender when ready..."));
        while(1){
          for(writeIndex = 0; writeIndex < 1064; writeIndex++){
            if(currentIndex % 1024 == 0){
              receivePacket();
            }
            if(done == 1){
              delay(2000);
              break;
            }
            blockData[writeIndex] = crcArray[currentIndex % 1024];
            currentIndex += 1;
          }
          if(done == 1){
            break;
          }
          driveSize[0] = currentBlock >> 16;
          driveSize[1] = currentBlock >> 8;
          driveSize[2] = currentBlock;
          //Serial.println("Thanks for the packet!");
          writeTwoBlocks(driveSize[0], driveSize[1], driveSize[2]);
          currentBlock += 2;
        }
        Serial.println();
        //for(int i = 0; i < 1064; i++){
        //  printDataSpace(blockData[i]);
        //}
        //Serial.println();
        bool removePadding = true;
        for(int i = 0; i < 1024; i++){
          if(blockData[i] != padding){
            //Serial.print("Byte ");
            //Serial.print(i, HEX);
            //Serial.print("is not padding. Instead, it's actually ");
            //printDataSpace(blockData[i]);
            removePadding = false;
          }
        }
        if(removePadding == true){
          //Serial.print("There's padding! So currentIndex has been reduced from __ to __ ");
          //Serial.print(currentIndex, HEX);
          //Serial.print(" ");
          currentIndex -= 1024;
          //Serial.print(currentIndex, HEX);
          //Serial.println();
          //Serial.print("Highest block is ");
          //Serial.print(highestBlock, HEX);
          //Serial.println();
        }
        //Serial.println();
        //printRawData();
        //Serial.println();
        //for(int i = 0; i < 1064; i++){
        //  printDataSpace(blockData[i]);
        //}
        flushInput();
        delay(500);
        flushInput();
        if((currentIndex * 2) / 1064 != highestBlock and failed == false){
          Serial.println();
          setLEDColor(1, 0, 0);
          Serial.print(F("File Size Mismatch: Drive size is "));
            printDataNoSpace((highestBlock - 1) >> 16);
            printDataNoSpace((highestBlock - 1) >> 8);
            printDataNoSpace(highestBlock - 1);
          Serial.print(F(" blocks, but received file was "));
            printDataNoSpace(((currentIndex * 2) / 1064) >> 16);
            printDataNoSpace(((currentIndex * 2) / 1064) >> 8);
            printDataNoSpace(((currentIndex * 2) / 1064));
          Serial.println(F(" blocks!"));
        }
        Serial.println();
        if(backupErrors != 0){
          setLEDColor(1, 0, 0);
          Serial.print(F("Warning: "));
          Serial.print(backupErrors);
          Serial.print(F(" disk errors were encountered during the operation!"));
          Serial.println();
        }
        if(failed == false){
          setLEDColor(0, 1, 0);
          Serial.println(F("Drive restore succeeded!"));
        }
        else{
          setLEDColor(1, 0, 0);
          Serial.println(F("Drive restore failed!"));
        }
      }

      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }
 
    if((command.equalsIgnoreCase("E") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false) or (command.equalsIgnoreCase("Q") and widgetMenu == true) or (command.equalsIgnoreCase("F") and diagMenu == true) or (command.equalsIgnoreCase("J") and diagMenuTenMeg == true) or (command.equalsIgnoreCase("C") and widgetServoMenu == true)){
      clearScreen();
      Serial.print(F("Command Buffer Contents: "));
      if(isTenMegDiagCommand == true){
        for(int i = 0; i <= commandBufferTenMegDiag[0]; i++){
          printDataSpace(commandBufferTenMegDiag[i]);
        }
      }
      else if(isWidgetCommand == true){
        for(int i = 0; i <= (commandBufferWidget[0] & 0x0F); i++){
          printDataSpace(commandBufferWidget[i]);
        }
      }
      else{
        for(int i = 0; i < 6; i++){
          printDataSpace(commandBuffer[i]);
        } 
      }
      Serial.println();
      Serial.println();
      if(isTenMegDiagCommand == false and isWidgetCommand == false){
        Serial.println(F("Command Interpretation (Assumes R/W Z8):"));
        Serial.print(F("Command Type: "));
        if(commandBuffer[0] == 0){
          Serial.println(F("Read"));
        }
        else if(commandBuffer[0] == 1){
          Serial.println(F("Write"));
        }
        else if(commandBuffer[0] == 2){
          Serial.println(F("Write-Verify"));
        }
        else{
          Serial.println(F("Unknown (Or maybe a diagnostic command!)"));
        }
        Serial.print(F("Block Number: "));
        for(int i = 1; i < 4; i++){
          printDataNoSpace(commandBuffer[i]);
        }
        Serial.println();
        Serial.print(F("Retry Count: "));
        printDataNoSpace(commandBuffer[4]);
        Serial.println();
        Serial.print(F("Spare Threshold: "));
        printDataNoSpace(commandBuffer[5]);
      }
      else if(isTenMegDiagCommand == true){
        Serial.print("This is a 10MB ProFile diagnostic command. Since all 10MB diagnostic commands have different formats, a command interpretation isn't available!");
      }
      else if(isWidgetCommand == true){
        Serial.print("This is a Widget command. Since all Widget commands have different formats, a command interpretation isn't available!");
      }
      Serial.println();
      Serial.println();
      printWidgetStatus();
      Serial.println(F("Data Buffer Contents: "));
      Serial.println();
      printRawData();
      Serial.println();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }
    
    else if(command.equalsIgnoreCase("F") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      clearScreen();
      Serial.print(F("Please enter the 6-byte command that you want to send: "));
      char charInput[2128];
      byte hexInput[1064];
      unsigned int charIndex = 0;
      bool goodInput = true;
      while(1){
        if(Serial.available()){
          char inByte = Serial.read();
          bool inArray = false;
          if(inByte == '\r'){
            inArray = true;
          }
          for(int i = 0; i < 22; i++){
            if(inByte == acceptableHex[i]){
              inArray = true;
            }
          }
          if(inArray == false){
            goodInput = false;
          }
          if(inByte == '\r' and goodInput == true and charIndex == 12){
            charInput[charIndex] = '\0';
            hex2bin(hexInput, charInput, &charIndex);
            break;
          }
          else if((inByte == '\r') and ((charIndex != 12) or (goodInput == false))){
            Serial.print(F("Please enter the 6-byte command that you want to send: "));
            charIndex = 0;
            goodInput = true;
          }
          else{
            charInput[charIndex] = inByte;
            if(charIndex < 2126){
              charIndex++;
            }
          }
        }
      }
      /*
      char charInput[2128];
      byte hexInput[1064];
      unsigned int charIndex = 0;
      while(1){
        while(1){
          if(Serial.available()){
            char inByte = Serial.read();
            if(inByte == '\r'){
              charInput[charIndex] = '\0';
              hex2bin(hexInput, charInput, &charIndex);
              break;
            }
            charInput[charIndex] = inByte;
            if(charIndex < 2126){
              charIndex++;
            }
          }
        }
        if(charIndex == 6){
          break;
        }
        else{
          Serial.print(F("Please enter the 6-byte command you want to send: "));
          charIndex = 0;
        }
      }*/
      for(int i = 0; i < 6; i++){
        commandBuffer[i] = hexInput[i];
      }
      Serial.println();
      if(commandBuffer[0] == 0){
        setLEDColor(0, 0, 0);
        Serial.print(F("Auto-detected as a read command. Reading block "));
        for(int i = 1; i < 4; i++){
          printDataNoSpace(commandBuffer[i]);
        }
        Serial.println(F("..."));
        Serial.print(F("Command: "));
        for(int i = 0; i < 6; i++){
          printDataSpace(commandBuffer[i]);
        }
        Serial.println();
        Serial.println();
        bool readSuccess = profileRead(commandBuffer[1], commandBuffer[2], commandBuffer[3], commandBuffer[4], commandBuffer[5]);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered during the read operation. The following data may be incorrect."));
          Serial.println();
        }
        printRawData();
        Serial.println();
        printStatus();
      }
      else if(commandBuffer[0] == 1){
        setLEDColor(0, 0, 0);
        Serial.print(F("Auto-detected as a write command. Writing the buffer to block "));
        for(int i = 1; i < 4; i++){
          printDataNoSpace(commandBuffer[i]);
        }
        Serial.println(F("..."));
        Serial.print(F("Command: "));
        for(int i = 0; i < 6; i++){
          printDataSpace(commandBuffer[i]);
        }
        Serial.println();
        Serial.println();
        bool writeSuccess = profileWrite(commandBuffer[1], commandBuffer[2], commandBuffer[3], commandBuffer[4], commandBuffer[5]);
        if(writeSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered during the write operation."));
          Serial.println();
        }
        printStatus();
      }
      else if(commandBuffer[0] == 2){
        setLEDColor(0, 0, 0);
        Serial.print(F("Auto-detected as a write-verify command. Write-verifying the buffer to block "));
        for(int i = 1; i < 4; i++){
          printDataNoSpace(commandBuffer[i]);
        }
        Serial.println(F("..."));
        Serial.print(F("Command: "));
        for(int i = 0; i < 6; i++){
          printDataSpace(commandBuffer[i]);
        }
        Serial.println();
        Serial.println();
        bool writeVerifySuccess = profileWriteVerify(commandBuffer[1], commandBuffer[2], commandBuffer[3], commandBuffer[4], commandBuffer[5]);
        if(writeVerifySuccess == 0){
          Serial.println(F("WARNING: Errors were encountered during the write-verify operation."));
          Serial.println();
        }
        printStatus();
      }
      else{
        Serial.println(F("Unknown if command is of read or write type. "));
        Serial.println(F("Read commands return data and the status bytes are read before the data is sent to the ArduinoFile."));
        Serial.println(F("Write commands don't return data and the status bytes are read after data has been written to the drive."));
        Serial.print(F("Is this a read command (r) or a write command (w)? "));
        while(1){
          if(Serial.available()) {
            delay(50);
            userInput = Serial.read();
            flushInput();
            if(userInput == 'w' or userInput == 'r'){
              break;
            }
            else{
              while(Serial.available()){
                Serial.read();
              }
              Serial.print(F("Is this a read command (r) or a write command (w)? "));
            }
          }
        }
        Serial.println();
        Serial.print(F("Executing command: "));
        for(int i = 0; i < 6; i++){
          printDataSpace(commandBuffer[i]);
        }
        Serial.println();
        setLEDColor(0, 0, 0);
        if(userInput == 'r'){
          Serial.println();
          Serial.println();
          bool readSuccess = customProfileRead(commandBuffer[0], commandBuffer[1], commandBuffer[2], commandBuffer[3], commandBuffer[4], commandBuffer[5]);
          if(readSuccess == 0){
            Serial.println(F("WARNING: Errors were encountered during the operation. The following data may be incorrect."));
            Serial.println();
          }
          printRawData();
          Serial.println();
          printStatus();
        }
        if(userInput == 'w'){
          Serial.println();
          Serial.println();
          bool writeSuccess = customProfileWrite(commandBuffer[0], commandBuffer[1], commandBuffer[2], commandBuffer[3], commandBuffer[4], commandBuffer[5]);
          if(writeSuccess == 0){
            Serial.println(F("WARNING: Errors were encountered during the operation."));
            Serial.println();
          }
          printStatus();
        }
      }
      Serial.print(F("Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("G") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      testMenu = true;
      clearScreen();
      flushInput();
    }

    else if(command.equalsIgnoreCase("I") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      command = "Z";
      clearScreen();
      byte oldDriveData[1064];
      for(int i = 0; i < 532; i++){
        oldDriveData[i] = blockData[i];
      }
      customProfileRead(0x00, 0xFF, 0xFF, 0xFF, defaultRetries, defaultSpareThreshold);
      bool continueToMenu = true;
      if(blockData[16] != 0xD6 or blockData[17] != 0x06){
        Serial.println(F("It doesn't look like you have the diagnostic Z8 ROM fitted in your ProFile, which is needed for all commands in this menu."));
        Serial.print(F("Do you want to continue anyway (return for yes, 'n' to cancel)? "));
        while(1){
          if(Serial.available()){
            delay(50);
            userInput = Serial.read();
            flushInput();
            if(userInput == 'n'){
              continueToMenu = false;
              break;
              }
            else if(userInput == '\r'){
              continueToMenu = true;
              break;
            }
            else{
              while(Serial.available()){
                Serial.read();
              }
              Serial.print(F("Do you want to continue anyway (return for yes, 'n' to cancel)? "));
            }
          }
        }
      }
      for(int i = 0; i < 532; i++){
        blockData[i] = oldDriveData[i];
      }
      if(continueToMenu == true){
        diagMenuTenMeg = true;
        clearScreen();
        flushInput();
      }
      else{
        diagMenuTenMeg = false;
        clearScreen();
        flushInput();
      }
    }

    else if(command.equalsIgnoreCase("H") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      clearScreen();
      byte oldDriveData[1064];
      for(int i = 0; i < 532; i++){
        oldDriveData[i] = blockData[i];
      }
      customProfileRead(0x00, 0xFF, 0xFF, 0xFF, defaultRetries, defaultSpareThreshold);
      bool continueToMenu = true;
      if(blockData[16] != 0xD3 or blockData[17] != 0x11){
        Serial.println(F("It doesn't look like you have the diagnostic Z8 ROM fitted in your ProFile, which is needed for all commands in this menu."));
        Serial.print(F("Do you want to continue anyway (return for yes, 'n' to cancel)? "));
        while(1){
          if(Serial.available()){
            delay(50);
            userInput = Serial.read();
            flushInput();
            if(userInput == 'n'){
              continueToMenu = false;
              break;
              }
            else if(userInput == '\r'){
              continueToMenu = true;
              break;
            }
            else{
              Serial.print(F("Do you want to continue anyway (return for yes, 'n' to cancel)? "));
              while(Serial.available()){
                Serial.read();
              }
            }
          }
        }
      }
      for(int i = 0; i < 532; i++){
        blockData[i] = oldDriveData[i];
      }
      if(continueToMenu == true){
        diagMenu = true;
        clearScreen();
        flushInput();
      }
      else{
        diagMenu = false;
        clearScreen();
        flushInput();
      }
    }

    else if(command.equalsIgnoreCase("J") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      clearScreen();
      byte oldDriveData[1064];
      for(int i = 0; i < 532; i++){
        oldDriveData[i] = blockData[i];
      }
      customProfileRead(0x00, 0xFF, 0xFF, 0xFF, defaultRetries, defaultSpareThreshold);
      bool continueToMenu = true;
      if(blockData[16] != 0x1A or blockData[17] != 0x45){
        Serial.println(F("It doesn't look like your drive is a Widget (or maybe it just has the wrong firmware version). All commands in this menu require a Widget drive and some commands require Widget firmware version 1A45."));
        Serial.print(F("Do you want to continue anyway (return for yes, 'n' to cancel)? "));
        while(1){
          if(Serial.available()){
            delay(50);
            userInput = Serial.read();
            flushInput();
            if(userInput == 'n'){
              continueToMenu = false;
              break;
              }
            else if(userInput == '\r'){
              continueToMenu = true;
              break;
            }
            else{
              Serial.print(F("Do you want to continue anyway (return for yes, 'n' to cancel)? "));
              while(Serial.available()){
                Serial.read();
              }
            }
          }
        }
      }
      for(int i = 0; i < 532; i++){
        blockData[i] = oldDriveData[i];
      }
      if(continueToMenu == true){
        widgetMenu = true;
        clearScreen();
        flushInput();
        command = "0";
      }
      else{
        widgetMenu = false;
        command = "0";
        clearScreen();
        flushInput();
      }
    }
    
    else if(command.equalsIgnoreCase("A") and testMenu == true and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      testMenu = false;
      clearScreen();
      flushInput();
    }

    else if(command.equalsIgnoreCase("3") and testMenu == true and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){ //Repeatedly Read CHS
      clearScreen();
      clearScreen();
      setLEDColor(0, 1, 0);
      uint32_t passCount = 1;
      Serial.print(F("Please enter the block number that you want to repeatedly read from: "));
      while(1){
        if(readSerialValue(6) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the block number that you want to repeatedly read from: "));
        }
      }
      for(int i = 0; i < 3; i++){
        address[i] = serialBytes[i];
      }

      Serial.println();
      while(!Serial.available()){
        Serial.print(F("Repeatedly reading block "));
        printDataNoSpace(address[0]);
        printDataNoSpace(address[1]);
        printDataNoSpace(address[2]);
        Serial.print(F(" - Pass "));
        printDataNoSpace(passCount >> 24);
        printDataNoSpace(passCount >> 16);
        printDataNoSpace(passCount >> 8);
        printDataNoSpace(passCount);
        Serial.print(F("..."));
        Serial.write("\033[1000D");
        bool readSuccess = customProfileRead(0x00, address[0], address[1], address[2], defaultRetries, defaultSpareThreshold);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered during the read operation. The following data may be incorrect."));
          Serial.println();
        }
        if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
          setLEDColor(1, 0, 0);
          Serial.println();
          Serial.println();
          Serial.print(F("Error reading on pass "));
          printDataNoSpace(passCount >> 24);
          printDataNoSpace(passCount >> 16);
          printDataNoSpace(passCount >> 8);
          printDataNoSpace(passCount);
          Serial.println(F("!"));
          Serial.println(F("Status Interpretation:"));
          for(int k = 0; k < 3; k++){
            for(int l = 0; l < 8; l++){
              if(bitRead(driveStatus[k], l) == 1){
                //statusGood = 0;
                Serial.println(statusMessages[k][l]);
              }
            }
          }
          Serial.println();
        }
        passCount += 1;
      }
      flushInput();
      Serial.println();
      Serial.println();
      Serial.println(F("Repeated read test terminated."));
      Serial.println();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("4") and testMenu == true and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){ //Repeatedly Write CHS
      clearScreen();
      clearScreen();
      setLEDColor(0, 1, 0);
      uint32_t passCount = 1;
      Serial.print(F("Please enter the block number that you want to repeatedly write to: "));
      while(1){
        if(readSerialValue(6) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the block number that you want to repeatedly write to: "));
        }
      }
      for(int i = 0; i < 3; i++){
        address[i] = serialBytes[i];
      }

      Serial.println();
      while(!Serial.available()){
        Serial.print(F("Repeatedly writing block "));
        printDataNoSpace(address[0]);
        printDataNoSpace(address[1]);
        printDataNoSpace(address[2]);
        Serial.print(F(" - Pass "));
        printDataNoSpace(passCount >> 24);
        printDataNoSpace(passCount >> 16);
        printDataNoSpace(passCount >> 8);
        printDataNoSpace(passCount);
        Serial.print(F("..."));
        Serial.write("\033[1000D");
        bool writeSuccess = customProfileWrite(0x01, address[0], address[1], address[2]);
        if(writeSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered during the write operation. The following data may be incorrect."));
          Serial.println();
        }
        if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
          setLEDColor(1, 0, 0);
          Serial.println();
          Serial.println();
          Serial.print(F("Error writing on pass "));
          printDataNoSpace(passCount >> 24);
          printDataNoSpace(passCount >> 16);
          printDataNoSpace(passCount >> 8);
          printDataNoSpace(passCount);
          Serial.println(F("!"));
          Serial.println(F("Status Interpretation:"));
          for(int k = 0; k < 3; k++){
            for(int l = 0; l < 8; l++){
              if(bitRead(driveStatus[k], l) == 1){
                //statusGood = 0;
                Serial.println(statusMessages[k][l]);
              }
            }
          }
          Serial.println();
        }
        passCount += 1;
      }
      flushInput();
      Serial.println();
      Serial.println();
      Serial.println(F("Repeated write test terminated."));
      Serial.println();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equals("1") and testMenu == true and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      clearScreen();
      getDriveType();
      setLEDColor(0, 1, 0);
      repeatTest();
      bool abort = false;
      uint16_t highestBlock = 0;
      bool firstTime = true;
      long int passes = 1;
      Serial.println();
      highestBlock = (driveSize[0]<<16) | (driveSize[1]<<8) | (driveSize[2]);
      flushInput();
      while((repeat == true or firstTime == true) and abort == false){
        Serial.write("\033[1000D");
        Serial.print("                                                                                         ");
        Serial.write("\033[1000D");
        for(long int i = 0; i < highestBlock; i++){
          if(Serial.available()){
            abort = true;
            break;
          }
          driveSize[0] = i >> 16;
          driveSize[1] = i >> 8;
          driveSize[2] = i;
          profileRead(driveSize[0], driveSize[1], driveSize[2]);
          Serial.print(F("Now reading block "));
          for(int j = 0; j < 3; j++){
            printDataNoSpace(driveSize[j]);
          }
          Serial.print(F(" of "));
          printDataNoSpace((highestBlock - 1) >> 16);
          printDataNoSpace((highestBlock - 1) >> 8);
          printDataNoSpace(highestBlock - 1);
          Serial.print(F(". Progress: "));
          Serial.print(((float)i/(highestBlock - 1))*100);
          Serial.print(F("%"));
          if(repeat == true){
            Serial.print(F(" - Pass "));
            Serial.print(passes);
          }
          Serial.write("\033[1000D");
          if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
            setLEDColor(1, 0, 0);
            Serial.println();
            Serial.println();
            Serial.print(F("Error reading block "));
            printDataNoSpace(i >> 16);
            printDataNoSpace(i >> 8);
            printDataNoSpace(i);
            Serial.println(F("!"));
            Serial.println(F("Status Interpretation:"));
            for(int k = 0; k < 3; k++){
              for(int l = 0; l < 8; l++){
                if(bitRead(driveStatus[k], l) == 1){
                  //statusGood = 0;
                  Serial.println(statusMessages[k][l]);
                }
              }
            }
            Serial.println();
          }
        }
        firstTime = false;
        passes += 1;
      }
      if(abort == true){
        Serial.println();
        Serial.println();
        Serial.println("Read test terminated by keypress.");
      }

      Serial.println();
      Serial.print(F("Read test completed. Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }



    else if(command.equals("9") and testMenu == true and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      clearScreen();
      setLEDColor(0, 1, 0);
      confirm();
      bool abort = false;
      uint16_t highestBlock = 0;
      if(confirmOperation == true){
        getDriveType();
        repeatTest();
        bool firstTime = true;
        long int passes = 1;
        Serial.println();
        highestBlock = (driveSize[0]<<16) | (driveSize[1]<<8) | (driveSize[2]);
        flushInput();
        while((repeat == true or firstTime == true) and abort == false){
          Serial.write("\033[1000D");
          Serial.print("                                                                                         ");
          Serial.write("\033[1000D");
          for(long int i = 0; i < highestBlock; i++){
            if(Serial.available()){
              abort = true;
              break;
            }
            driveSize[0] = i >> 16;
            driveSize[1] = i >> 8;
            driveSize[2] = i;
            profileRead(driveSize[0], driveSize[1], driveSize[2]);
            Serial.print(F("Phase 1 - Now reading block "));
            for(int j = 0; j < 3; j++){
              printDataNoSpace(driveSize[j]);
            }
            Serial.print(F(" of "));
            printDataNoSpace((highestBlock - 1) >> 16);
            printDataNoSpace((highestBlock - 1) >> 8);
            printDataNoSpace(highestBlock - 1);
            Serial.print(F(". Progress: "));
            Serial.print(((float)i/(highestBlock - 1))*100);
            Serial.print(F("%"));
            if(repeat == true){
              Serial.print(F(" - Pass "));
              Serial.print(passes);
            }
            Serial.write("\033[1000D");
            if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
              setLEDColor(1, 0, 0);
              Serial.println();
              Serial.println();
              Serial.print(F("Error reading block "));
              printDataNoSpace(i >> 16);
              printDataNoSpace(i >> 8);
              printDataNoSpace(i);
              Serial.println(F("!"));
              Serial.println(F("Status Interpretation:"));
              for(int k = 0; k < 3; k++){
                for(int l = 0; l < 8; l++){
                  if(bitRead(driveStatus[k], l) == 1){
                    //statusGood = 0;
                    Serial.println(statusMessages[k][l]); //All status errors show up every time
                  }
                }
              }
              Serial.println();
            }
          }
          for(int i = 0; i < 532; i+=3){
            blockData[i] = 0x55;
            crcArray[i] = 0x55;
          }
          for(int i = 1; i < 532; i+=3){
            blockData[i] = 0x5A;
            crcArray[i] = 0x5A;
          }
          for(int i = 2; i < 532; i+=3){
            blockData[i] = 0xAA;
            crcArray[i] = 0xAA;
          }
          blockData[530] = 0x70;
          blockData[531] = 0x75;
          crcArray[530] = 0x70;
          crcArray[531] = 0x75;
          Serial.write("\033[1000D");
          Serial.print("                                                                                         ");
          Serial.write("\033[1000D");
          for(long int i = 0; i < highestBlock; i++){
            if(abort == true or Serial.available()){
              break;
            }
            driveSize[0] = i >> 16;
            driveSize[1] = i >> 8;
            driveSize[2] = i;
            profileWrite(driveSize[0], driveSize[1], driveSize[2]);
            Serial.print(F("Phase 2 - Now writing test pattern to block "));
            for(int j = 0; j < 3; j++){
              printDataNoSpace(driveSize[j]);
            }
            Serial.print(F(" of "));
            printDataNoSpace((highestBlock - 1) >> 16);
            printDataNoSpace((highestBlock - 1) >> 8);
            printDataNoSpace(highestBlock - 1);
            Serial.print(F(". Progress: "));
            Serial.print(((float)i/(highestBlock - 1))*100);
            Serial.print(F("%"));
            if(repeat == true){
              Serial.print(F(" - Pass "));
              Serial.print(passes);
            }
            Serial.write("\033[1000D");
            if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
              setLEDColor(1, 0, 0);
              Serial.println();
              Serial.println();
              Serial.print(F("Error writing block "));
              printDataNoSpace(i >> 16);
              printDataNoSpace(i >> 8);
              printDataNoSpace(i);
              Serial.println(F("!"));
              Serial.println(F("Status Interpretation:"));
              for(int k = 0; k < 3; k++){
                for(int l = 0; l < 8; l++){
                  if(bitRead(driveStatus[k], l) == 1){
                    //statusGood = 0;
                    Serial.println(statusMessages[k][l]); //All status errors show up every time
                  }
                }
              }
              Serial.println();
            }
          }
          bool blockError = false;
          Serial.write("\033[1000D");
          Serial.print("                                                                                         ");
          Serial.write("\033[1000D");
          for(long int i = 0; i < highestBlock; i++){
            if(abort == true or Serial.available()){
              break;
            }
            driveSize[0] = i >> 16;
            driveSize[1] = i >> 8;
            driveSize[2] = i;
            profileRead(driveSize[0], driveSize[1], driveSize[2]);
            Serial.print(F("Phase 3 - Now rereading block "));
            for(int j = 0; j < 3; j++){
              printDataNoSpace(driveSize[j]);
            }
            Serial.print(F(" of "));
            printDataNoSpace((highestBlock - 1) >> 16);
            printDataNoSpace((highestBlock - 1) >> 8);
            printDataNoSpace(highestBlock - 1);
            Serial.print(F(". Progress: "));
            Serial.print(((float)i/(highestBlock - 1))*100);
            Serial.print(F("%"));
            if(repeat == true){
              Serial.print(F(" - Pass "));
              Serial.print(passes);
            }
            Serial.write("\033[1000D");
            for(int m = 0; m < 532; m++){
              if(blockData[m] != crcArray[m]){
                blockError = true;
              }
            }
            if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0 or blockError == true){ //Make it so that we go back up to the progress line after printing an error
              setLEDColor(1, 0, 0);
              Serial.println();
              Serial.println();
              blockError = false;
              Serial.print(F("Error rereading block "));
              printDataNoSpace(i >> 16);
              printDataNoSpace(i >> 8);
              printDataNoSpace(i);
              Serial.print(F("!"));
              if(blockError == true){
                Serial.print(F(" The data that we read back didn't match what we wrote to the block!"));
              }
              Serial.println();
              Serial.println(F("Status Interpretation:"));
              for(int k = 0; k < 3; k++){
                for(int l = 0; l < 8; l++){
                  if(bitRead(driveStatus[k], l) == 1){
                    //statusGood = 0;
                    Serial.println(statusMessages[k][l]); //All status errors show up every time
                  }
                }
              }
              Serial.println();
            }
          }
          firstTime = false;
          passes += 1;
        }
      }

      if(abort == true){
        Serial.println();
        Serial.println("Read-Write-Read test terminated by keypress.");
      }

      Serial.println();
      Serial.print(F("Read-Write-Read test completed. Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }




    else if(command.equals("2") and testMenu == true and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      clearScreen();
      setLEDColor(0, 1, 0);
      bool abort = false;
      confirm();
      uint16_t highestBlock = 0;
      if(confirmOperation == true){
        getDriveType();
        repeatTest();
        bool firstTime = true;
        long int passes = 1;
        Serial.println();
        highestBlock = (driveSize[0]<<16) | (driveSize[1]<<8) | (driveSize[2]);
        flushInput();
        while((repeat == true or firstTime == true) and abort == false){
          Serial.write("\033[1000D");
          Serial.print("                                                                                         ");
          Serial.write("\033[1000D");
          for(long int i = 0; i < highestBlock; i++){
            if(Serial.available()){
              abort = true;
              break;
            }
            driveSize[0] = i >> 16;
            driveSize[1] = i >> 8;
            driveSize[2] = i;
            for(int i = 0; i < 532; i++){
              blockData[i] = B01010101;
            }
            profileWrite(driveSize[0], driveSize[1], driveSize[2]);
            Serial.print(F("Now writing block "));
            for(int j = 0; j < 3; j++){
              printDataNoSpace(driveSize[j]);
            }
            Serial.print(F(" of "));
            printDataNoSpace((highestBlock - 1) >> 16);
            printDataNoSpace((highestBlock - 1) >> 8);
            printDataNoSpace(highestBlock - 1);
            Serial.print(F(". Progress: "));
            Serial.print(((float)i/(highestBlock - 1))*100);
            Serial.print(F("%"));
            if(repeat == true){
              Serial.print(F(" - Pass "));
              Serial.print(passes);
            }
            Serial.write("\033[1000D");
            if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
              setLEDColor(1, 0, 0);
              Serial.println();
              Serial.println();
              Serial.print(F("Error writing block "));
              printDataNoSpace(i >> 16);
              printDataNoSpace(i >> 8);
              printDataNoSpace(i);
              Serial.print(F(" with pattern 01010101 (0x55)"));
              Serial.println(F("!"));
              Serial.println(F("Status Interpretation:"));
              for(int k = 0; k < 3; k++){
                for(int l = 0; l < 8; l++){
                  if(bitRead(driveStatus[k], l) == 1){
                    //statusGood = 0;
                    Serial.println(statusMessages[k][l]); //All status errors show up every time
                  }
                }
              }
              Serial.println();
            }
            for(int i = 0; i < 532; i++){
              blockData[i] = B10101010;
            }
            profileWrite(driveSize[0], driveSize[1], driveSize[2]);
            if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
              setLEDColor(1, 0, 0);
              Serial.println();
              Serial.println();
              Serial.print(F("Error writing block "));
              printDataNoSpace(i >> 16);
              printDataNoSpace(i >> 8);
              printDataNoSpace(i);
              Serial.print(F(" with pattern 10101010 (0xAA)"));
              Serial.println(F("!"));
              Serial.println(F("Status Interpretation:"));
              for(int k = 0; k < 3; k++){
                for(int l = 0; l < 8; l++){
                  if(bitRead(driveStatus[k], l) == 1){
                    //statusGood = 0;
                    Serial.println(statusMessages[k][l]); //All status errors show up every time
                  }
                }
              }
              Serial.println();
            }
          }
          firstTime = false;
          passes += 1;
        }
      }

      if(abort == true){
        Serial.println();
        Serial.println();
        Serial.println(F("Write test terminated by keypress."));
      }

      Serial.println();
      Serial.print(F("Write test completed. Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equals("5") and testMenu == true and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      clearScreen();
      getDriveType();
      setLEDColor(0, 1, 0);
      bool abort = false;
      repeatTest();
      uint16_t highestBlock = 0;
      bool firstTime = true;
      randomSeed(analogRead(0));
      long int passes = 1;
      uint16_t randomBlock = 0;
      Serial.println();
      highestBlock = (driveSize[0]<<16) | (driveSize[1]<<8) | (driveSize[2]);
      flushInput();
      while((repeat == true or firstTime == true) and abort == false){
        Serial.write("\033[1000D");
        Serial.print("                                                                                         ");
        Serial.write("\033[1000D");
        for(long int i = 0; i < highestBlock; i++){
          if(Serial.available()){
            abort = true;
            break;
          }
          randomBlock = random(0, highestBlock);
          driveSize[0] = randomBlock >> 16;
          driveSize[1] = randomBlock >> 8;
          driveSize[2] = randomBlock;
          profileRead(driveSize[0], driveSize[1], driveSize[2]);
          Serial.print(F("Now reading randomly selected block "));
          for(int j = 0; j < 3; j++){
            printDataNoSpace(driveSize[j]);
          }
          Serial.print(F(" of "));
          printDataNoSpace((highestBlock - 1) >> 16);
          printDataNoSpace((highestBlock - 1) >> 8);
          printDataNoSpace(highestBlock - 1);
          Serial.print(F(". Progress: "));
          Serial.print(((float)i/(highestBlock - 1))*100);
          Serial.print(F("%"));
          if(repeat == true){
            Serial.print(F(" - Pass "));
            Serial.print(passes);
          }
          Serial.write("\033[1000D");
          if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
            setLEDColor(1, 0, 0);
            Serial.println();
            Serial.println();
            Serial.print(F("Error reading block "));
            printDataNoSpace(randomBlock >> 16);
            printDataNoSpace(randomBlock >> 8);
            printDataNoSpace(randomBlock);
            Serial.println(F("!"));
            Serial.println(F("Status Interpretation:"));
            for(int k = 0; k < 3; k++){
              for(int l = 0; l < 8; l++){
                if(bitRead(driveStatus[k], l) == 1){
                  //statusGood = 0;
                  Serial.println(statusMessages[k][l]); //All status errors show up every time
                }
              }
            }
            Serial.println();
          }
        }
        firstTime = false;
        passes += 1;
      }
      if(abort == true){
        Serial.println();
        Serial.println();
        Serial.println(F("Read test terminated by keypress."));
      }

      Serial.println();
      Serial.print(F("Read test completed. Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }






    else if(command.equals("6") and testMenu == true and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      clearScreen();
      setLEDColor(0, 1, 0);
      confirm();
      bool abort = false;
      uint16_t highestBlock = 0;
      if(confirmOperation == true){
        getDriveType();
        repeatTest();
        bool firstTime = true;
        randomSeed(analogRead(0));
        long int passes = 1;
        uint16_t randomBlock = 0;
        Serial.println();
        highestBlock = (driveSize[0]<<16) | (driveSize[1]<<8) | (driveSize[2]);
        flushInput();
        while((repeat == true or firstTime == true) and abort == false){
          Serial.write("\033[1000D");
          Serial.print("                                                                                         ");
          Serial.write("\033[1000D");
          for(long int i = 0; i < highestBlock; i++){
            if(Serial.available()){
              abort = true;
              break;
            }
            randomBlock = random(0, highestBlock);
            driveSize[0] = randomBlock >> 16;
            driveSize[1] = randomBlock >> 8;
            driveSize[2] = randomBlock;
            for(int i = 0; i < 532; i++){
              blockData[i] = B01010101;
            }
            profileWrite(driveSize[0], driveSize[1], driveSize[2]);
            Serial.print(F("Now writing randomly selected block "));
            for(int j = 0; j < 3; j++){
              printDataNoSpace(driveSize[j]);
            }
            Serial.print(F(" of "));
            printDataNoSpace((highestBlock - 1) >> 16);
            printDataNoSpace((highestBlock - 1) >> 8);
            printDataNoSpace(highestBlock - 1);
            Serial.print(F(". Progress: "));
            Serial.print(((float)i/(highestBlock - 1))*100);
            Serial.print(F("%"));
            if(repeat == true){
              Serial.print(F(" - Pass "));
              Serial.print(passes);
            }
            Serial.write("\033[1000D");
            if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
              setLEDColor(1, 0, 0);
              Serial.println();
              Serial.println();
              Serial.print(F("Error writing block "));
              printDataNoSpace(randomBlock >> 16);
              printDataNoSpace(randomBlock >> 8);
              printDataNoSpace(randomBlock);
              Serial.print(F(" with pattern 01010101"));
              Serial.println(F("!"));
              Serial.println(F("Status Interpretation:"));
              for(int k = 0; k < 3; k++){
                for(int l = 0; l < 8; l++){
                  if(bitRead(driveStatus[k], l) == 1){
                    //statusGood = 0;
                    Serial.println(statusMessages[k][l]); //All status errors show up every time
                  }
                }
              }
              Serial.println();
            }
            for(int i = 0; i < 532; i++){
              blockData[i] = B10101010;
            }
            profileWrite(driveSize[0], driveSize[1], driveSize[2]);
            if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
              setLEDColor(1, 0, 0);
              Serial.println();
              Serial.println();
              Serial.print(F("Error writing block "));
              printDataNoSpace(i >> 16);
              printDataNoSpace(i >> 8);
              printDataNoSpace(i);
              Serial.print(F(" with pattern 01010101"));
              Serial.println(F("!"));
              Serial.println(F("Status Interpretation:"));
              for(int k = 0; k < 3; k++){
                for(int l = 0; l < 8; l++){
                  if(bitRead(driveStatus[k], l) == 1){
                    //statusGood = 0;
                    Serial.println(statusMessages[k][l]); //All status errors show up every time
                  }
                }
              }
              Serial.println();
            }
          }
          firstTime = false;
          passes += 1;
        }
      }

      if(abort == true){
        Serial.println();
        Serial.println();
        Serial.println(F("Write test terminated by keypress."));
      }

      Serial.println();
      Serial.print(F("Write test completed. Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }


    else if(command.equals("7") and testMenu == true and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      clearScreen();
      getDriveType();
      setLEDColor(0, 1, 0);
      bool abort = false;
      repeatTest();
      uint16_t highestBlock = 0;
      bool firstTime = true;
      long int passes = 1;
      uint16_t butterflyBlock;
      Serial.println();
      highestBlock = (driveSize[0]<<16) | (driveSize[1]<<8) | (driveSize[2]);
      flushInput();
      while((repeat == true or firstTime == true) and abort == false){
        Serial.write("\033[1000D");
        Serial.print("                                                                                         ");
        Serial.write("\033[1000D");
        for(long int i = 0; i < highestBlock; i++){
          if(Serial.available()){
            abort = true;
            break;
          }
          if(i % 2 == 0){
            butterflyBlock = i;
          }
          else{
            butterflyBlock = highestBlock - i;
          }
          driveSize[0] = butterflyBlock >> 16;
          driveSize[1] = butterflyBlock >> 8;
          driveSize[2] = butterflyBlock;
          profileRead(driveSize[0], driveSize[1], driveSize[2]);
          Serial.print(F("Now performing butterfly read test on block "));
          for(int j = 0; j < 3; j++){
            printDataNoSpace(driveSize[j]);
          }
          Serial.print(F(" of "));
          printDataNoSpace((highestBlock - 1) >> 16);
          printDataNoSpace((highestBlock - 1) >> 8);
          printDataNoSpace(highestBlock - 1);
          Serial.print(F(". Progress: "));
          Serial.print(((float)i/(highestBlock - 1))*100);
          Serial.print(F("%"));
          if(repeat == true){
            Serial.print(F(" - Pass "));
            Serial.print(passes);
          }
          Serial.write("\033[1000D");
          if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
            setLEDColor(1, 0, 0);
            Serial.println();
            Serial.println();
            Serial.print(F("Error reading block "));
            printDataNoSpace(butterflyBlock >> 16);
            printDataNoSpace(butterflyBlock >> 8);
            printDataNoSpace(butterflyBlock);
            Serial.println(F("!"));
            Serial.println(F("Status Interpretation:"));
            for(int k = 0; k < 3; k++){
              for(int l = 0; l < 8; l++){
                if(bitRead(driveStatus[k], l) == 1){
                  //statusGood = 0;
                  Serial.println(statusMessages[k][l]); //All status errors show up every time
                }
              }
            }
            Serial.println();
          }
        }
        firstTime = false;
        passes += 1;
      }

      if(abort == true){
        Serial.println();
        Serial.println();
        Serial.println(F("Butterfly read test terminated by keypress."));
      }

      Serial.println();
      Serial.print(F("Butterfly read test completed. Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equals("8") and testMenu == true and diagMenu == false and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      clearScreen();
      confirm();
      bool abort = false;
      if(confirmOperation == true){
        getDriveType();
        setLEDColor(0, 1, 0);
        repeatTest();
        uint16_t highestBlock = 0;
        bool firstTime = true;
        long int passes = 1;
        uint16_t butterflyBlock;
        Serial.println();
        highestBlock = (driveSize[0]<<16) | (driveSize[1]<<8) | (driveSize[2]);
        flushInput();
        while((repeat == true or firstTime == true) and abort == false){
          Serial.write("\033[1000D");
          Serial.print("                                                                                         ");
          Serial.write("\033[1000D");
          for(long int i = 0; i < highestBlock; i++){
            if(Serial.available()){
              abort = true;
              break;
            }
            if(i % 2 == 0){
              butterflyBlock = i;
            }
            else{
              butterflyBlock = highestBlock - i;
            }
            driveSize[0] = butterflyBlock >> 16;
            driveSize[1] = butterflyBlock >> 8;
            driveSize[2] = butterflyBlock;
            for(int i = 0; i < 532; i++){
              blockData[i] = 0x55;
            }
            profileWrite(driveSize[0], driveSize[1], driveSize[2]);
            Serial.print(F("Now performing butterfly write test on block "));
            for(int j = 0; j < 3; j++){
              printDataNoSpace(driveSize[j]);
            }
            Serial.print(F(" of "));
            printDataNoSpace((highestBlock - 1) >> 16);
            printDataNoSpace((highestBlock - 1) >> 8);
            printDataNoSpace(highestBlock - 1);
            Serial.print(F(". Progress: "));
            Serial.print(((float)i/(highestBlock - 1))*100);
            Serial.print(F("%"));
            if(repeat == true){
              Serial.print(F(" - Pass "));
              Serial.print(passes);
            }
            Serial.write("\033[1000D");
            if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
              setLEDColor(1, 0, 0);
              Serial.println();
              Serial.println();
              Serial.print(F("Error writing block with pattern 01010101 (0x55)"));
              printDataNoSpace(butterflyBlock >> 16);
              printDataNoSpace(butterflyBlock >> 8);
              printDataNoSpace(butterflyBlock);
              Serial.println(F("!"));
              Serial.println(F("Status Interpretation:"));
              for(int k = 0; k < 3; k++){
                for(int l = 0; l < 8; l++){
                  if(bitRead(driveStatus[k], l) == 1){
                    //statusGood = 0;
                    Serial.println(statusMessages[k][l]); //All status errors show up every time
                  }
                }
              }
              Serial.println();
            }
            for(int i = 0; i < 532; i++){
              blockData[i] = 0xAA;
            }
            profileWrite(driveSize[0], driveSize[1], driveSize[2]);
            if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
              setLEDColor(1, 0, 0);
              Serial.println();
              Serial.println();
              Serial.print(F("Error writing block with pattern 10101010 (0xAA)"));
              printDataNoSpace(butterflyBlock >> 16);
              printDataNoSpace(butterflyBlock >> 8);
              printDataNoSpace(butterflyBlock);
              Serial.println(F("!"));
              Serial.println(F("Status Interpretation:"));
              for(int k = 0; k < 3; k++){
                for(int l = 0; l < 8; l++){
                  if(bitRead(driveStatus[k], l) == 1){
                    //statusGood = 0;
                    Serial.println(statusMessages[k][l]); //All status errors show up every time
                  }
                }
              }
              Serial.println();
            }
          }
          firstTime = false;
          passes += 1;
        }

        if(abort == true){
          Serial.println();
          Serial.println();
          Serial.println(F("Butterfly write test terminated by keypress."));
        }

        Serial.println();
        Serial.print(F("Butterfly write test completed. "));
      }
      else{
        Serial.println();
      }
      Serial.print(F("Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }
    else if(command.equals("1") and testMenu == false and diagMenu == true and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      clearScreen();
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.print(F("Please enter the cylinder, head, and sector that you want to read from in the format CCHHSS: "));
      while(1){
        if(readSerialValue(6) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the cylinder, head, and sector that you want to read from in the format CCHHSS: "));
        }
      }
      for(int i = 0; i < 3; i++){
        address[i] = serialBytes[i];
      }
      int retries = defaultRetries;
      int spare = defaultSpareThreshold;
      Serial.print(F("Use the default retry count of "));
      printDataNoSpace(retries);
      Serial.print(F(" (return for yes, 'n' for no)? "));
      while(1){
        if(Serial.available()) {
          delay(50);
          userInput = Serial.read();
          flushInput();
          if(userInput == 'n'){
            Serial.print(F("Please enter the desired retry count: "));
            while(1){
              if(readSerialValue(2) == true){
                retries = serialBytes[0];
                break;
              }
              else{
                Serial.print(F("Please enter the desired retry count: "));
              }
            }
            break;
          }
          else if(userInput == '\r'){
            break;
          }
          else{
            while(Serial.available()){
              Serial.read();
            }
            Serial.print(F("Use the default retry count of "));
            printDataNoSpace(retries);
            Serial.print(F(" (return for yes, 'n' for no)? "));
          }
        }
      }
      Serial.print(F("Use the default spare threshold of "));
      printDataNoSpace(spare);
      Serial.print(F(" (return for yes, 'n' for no)? "));
      while(1){
        if(Serial.available()) {
          delay(50);
          userInput = Serial.read();
          flushInput();
          if(userInput == 'n'){
            Serial.print(F("Please enter the desired spare threshold: "));
            while(1){
              if(readSerialValue(2) == true){
                spare = serialBytes[0];
                break;
              }
              else{
                Serial.print(F("Please enter the desired spare threshold: "));
              }
            }
            break;
          }
          else if(userInput == '\r'){
            break;
          }
          else{
            while(Serial.available()){
              Serial.read();
            }
            Serial.print(F("Use the default spare threshold of "));
            printDataNoSpace(spare);
            Serial.print(F(" (return for yes, 'n' for no)? "));
          }
        }
      }
      Serial.println();
      Serial.print(F("Reading cylinder "));
      printDataNoSpace(address[0]);
      Serial.print(F(", head "));
      printDataNoSpace(address[1]);
      Serial.print(F(", and sector "));
      printDataNoSpace(address[2]);
      Serial.println(F("..."));
      Serial.print(F("Command: 00 "));
      for(int i = 0; i < 3; i++){
        printDataSpace(address[i]);
      }
      printDataSpace(retries);
      printDataNoSpace(spare);
      Serial.println();
      Serial.println();
      bool readSuccess = customProfileRead(0x00, address[0], address[1], address[2], retries, spare);
      if(readSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered during the read operation. The following data may be incorrect."));
        Serial.println();
      }
      printRawData();
      Serial.println();
      printStatus();
      Serial.println();
      Serial.print(F("Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }
    else if(command.equals("2") and testMenu == false and diagMenu == true and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.print(F("Please enter the cylinder, head, and sector that you want to write to in the format CCHHSS: "));
      while(1){
        if(readSerialValue(6) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the cylinder, head, and sector that you want to write to in the format CCHHSS: "));
        }
      }
      for(int i = 0; i < 3; i++){
        address[i] = serialBytes[i];
      }
      Serial.println();
      Serial.print(F("Writing the buffer to cylinder "));
      printDataNoSpace(address[0]);
      Serial.print(F(", head "));
      printDataNoSpace(address[1]);
      Serial.print(F(", and sector "));
      printDataNoSpace(address[2]);
      Serial.println(F("..."));
      Serial.print(F("Command: 01 "));
      for(int i = 0; i < 3; i++){
        printDataSpace(address[i]);
      }
      printDataSpace(defaultRetries);
      printDataNoSpace(defaultSpareThreshold);
      Serial.println();
      Serial.println();
      bool writeSuccess = customProfileWrite(0x01, address[0], address[1], address[2]);
      if(writeSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered during the write operation. The data may have been written incorrectly."));
        Serial.println();
        setLEDColor(1, 0, 0);
      }
      else{
        setLEDColor(0, 1, 0);
      }
      printStatus();
      Serial.println();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("3") and testMenu == false and diagMenu == true and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){ //Repeatedly Read CHS
      clearScreen();
      clearScreen();
      setLEDColor(0, 1, 0);
      uint32_t passCount = 1;
      Serial.print(F("Please enter the cylinder, head, and sector that you want to repeatedly read from in the format CCHHSS: "));
      while(1){
        if(readSerialValue(6) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the cylinder, head, and sector that you want to repeatedly read from in the format CCHHSS: "));
        }
      }
      for(int i = 0; i < 3; i++){
        address[i] = serialBytes[i];
      }

      Serial.println();
      while(!Serial.available()){
        Serial.print(F("Repeatedly reading cylinder "));
        printDataNoSpace(address[0]);
        Serial.print(F(", head "));
        printDataNoSpace(address[1]);
        Serial.print(F(", and sector "));
        printDataNoSpace(address[2]);
        Serial.print(F(" - Pass "));
        printDataNoSpace(passCount >> 24);
        printDataNoSpace(passCount >> 16);
        printDataNoSpace(passCount >> 8);
        printDataNoSpace(passCount);
        Serial.print(F("..."));
        Serial.write("\033[1000D");
        bool readSuccess = customProfileRead(0x00, address[0], address[1], address[2], defaultRetries, defaultSpareThreshold);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered during the read operation. The following data may be incorrect."));
          Serial.println();
        }
        if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
          setLEDColor(1, 0, 0);
          Serial.println();
          Serial.println();
          Serial.print(F("Error reading on pass "));
          printDataNoSpace(passCount >> 24);
          printDataNoSpace(passCount >> 16);
          printDataNoSpace(passCount >> 8);
          printDataNoSpace(passCount);
          Serial.println(F("!"));
          Serial.println(F("Status Interpretation:"));
          for(int k = 0; k < 3; k++){
            for(int l = 0; l < 8; l++){
              if(bitRead(driveStatus[k], l) == 1){
                //statusGood = 0;
                Serial.println(statusMessages[k][l]);
              }
            }
          }
          Serial.println();
        }
        passCount += 1;
      }
      flushInput();
      Serial.println();
      Serial.println();
      Serial.println(F("Repeated read test terminated."));
      Serial.println();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("4") and testMenu == false and diagMenu == true and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){ //Repeatedly Write CHS
      clearScreen();
      clearScreen();
      setLEDColor(0, 1, 0);
      uint32_t passCount = 1;
      Serial.print(F("Please enter the cylinder, head, and sector that you want to repeatedly write to in the format CCHHSS: "));
      while(1){
        if(readSerialValue(6) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the cylinder, head, and sector that you want to repeatedly write to in the format CCHHSS: "));
        }
      }
      for(int i = 0; i < 3; i++){
        address[i] = serialBytes[i];
      }

      Serial.println();
      while(!Serial.available()){
        Serial.print(F("Repeatedly writing cylinder "));
        printDataNoSpace(address[0]);
        Serial.print(F(", head "));
        printDataNoSpace(address[1]);
        Serial.print(F(", and sector "));
        printDataNoSpace(address[2]);
        Serial.print(F(" - Pass "));
        printDataNoSpace(passCount >> 24);
        printDataNoSpace(passCount >> 16);
        printDataNoSpace(passCount >> 8);
        printDataNoSpace(passCount);
        Serial.print(F("..."));
        Serial.write("\033[1000D");
        bool writeSuccess = customProfileWrite(0x01, address[0], address[1], address[2]);
        if(writeSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered during the write operation. The following data may be incorrect."));
          Serial.println();
        }
        if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
          setLEDColor(1, 0, 0);
          Serial.println();
          Serial.println();
          Serial.print(F("Error writing on pass "));
          printDataNoSpace(passCount >> 24);
          printDataNoSpace(passCount >> 16);
          printDataNoSpace(passCount >> 8);
          printDataNoSpace(passCount);
          Serial.println(F("!"));
          Serial.println(F("Status Interpretation:"));
          for(int k = 0; k < 3; k++){
            for(int l = 0; l < 8; l++){
              if(bitRead(driveStatus[k], l) == 1){
                //statusGood = 0;
                Serial.println(statusMessages[k][l]);
              }
            }
          }
          Serial.println();
        }
        passCount += 1;
      }
      flushInput();
      Serial.println();
      Serial.println();
      Serial.println(F("Repeated write test terminated."));
      Serial.println();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equals("5") and testMenu == false and diagMenu == true and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.print(F("Please enter the cylinder, head, and sector that you want to write-verify to in the format CCHHSS: "));
      while(1){
        if(readSerialValue(6) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the cylinder, head, and sector that you want to write-verify to in the format CCHHSS: "));
        }
      }
      for(int i = 0; i < 3; i++){
        address[i] = serialBytes[i];
      }
      Serial.println();
      Serial.print(F("Write-verifying the buffer to cylinder "));
      printDataNoSpace(address[0]);
      Serial.print(F(", head "));
      printDataNoSpace(address[1]);
      Serial.print(F(", and sector "));
      printDataNoSpace(address[2]);
      Serial.println(F("..."));
      Serial.print(F("Command: 02 "));
      for(int i = 0; i < 3; i++){
        printDataSpace(address[i]);
      }
      printDataSpace(defaultRetries);
      printDataNoSpace(defaultSpareThreshold);
      Serial.println();
      Serial.println();
      bool writeSuccess = customProfileWrite(0x02, address[0], address[1], address[2]);
      if(writeSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered during the write-verify operation. The data may have been written incorrectly."));
        Serial.println();
        setLEDColor(1, 0, 0);
      }
      else{
        setLEDColor(0, 1, 0);
      }
      printStatus();
      Serial.println();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }
    else if(command.equalsIgnoreCase("6") and testMenu == false and diagMenu == true and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.println("Note: This low-level format command will show the number of bad blocks and sectors that are encountered while formatting the drive, but it will NOT show a list of the actual blocks and sectors that are bad. If you want that, manually run the Format, Scan, and Init Spare Table commands in that order to achieve the same low-level format with more error information.");
      Serial.println();
      confirm();
      if(confirmOperation == true){
        Serial.println();
        Serial.print(F("Install jumper and press return to continue..."));
        flushInput();
        while(!Serial.available());
        flushInput();
        Serial.println();
        setLEDColor(0, 0, 0);
        Serial.println(F("Step 1 - Formatting the drive..."));
        Serial.println(F("Command: 03 00 00 00 0A 03"));
        bool readSuccess = customProfileRead(0x03, 0x00, 0x00, 0x00, defaultRetries, defaultSpareThreshold, true);
        if(readSuccess == 0){
          Serial.println();
          setLEDColor(1, 0, 0);
          Serial.println(F("WARNING: Errors were encountered during the format operation. The format may not have completed successfully."));
        }
        else{
          setLEDColor(0, 1, 0);
        }
        Serial.println();
        Serial.print(F("Number of bad blocks found during the format procedure: "));
        printDataNoSpace(driveStatus[3]);
        Serial.println();
        Serial.println();
        Serial.print(F("Remove jumper and press return to continue..."));
        flushInput();
        while(!Serial.available());
        Serial.println();
        flushInput();
        setLEDColor(0, 0, 0);
        Serial.println(F("Step 2 - Performing a surface scan..."));
        Serial.println(F("Command: 04 00 00 00 0A 03"));
        readSuccess = customProfileRead(0x04, 0x00, 0x00, 0x00, defaultRetries, defaultSpareThreshold, true);
        if(readSuccess == 0){
          Serial.println();
          setLEDColor(1, 0, 0);
          Serial.println(F("WARNING: Errors were encountered during the surface scan operation. The scan may not have completed successfully."));
        }
        else{
          setLEDColor(0, 1, 0);
        }
        Serial.println();
        Serial.print(F("Number of bad blocks found during the surface scan: "));
        printDataNoSpace(driveStatus[3]);
        Serial.println();
        Serial.println();
        setLEDColor(0, 0, 0);
        Serial.println(F("Step 3 - Creating the spare table..."));
        Serial.println(F("Command: 05 00 00 00 0A 03"));
        readSuccess = customProfileRead(0x05, 0x00, 0x00, 0x00, defaultRetries, defaultSpareThreshold);
        if(readSuccess == 0){
          Serial.println();
          setLEDColor(1, 0, 0);
          Serial.println(F("WARNING: Errors were encountered while creating the spare table. The spare table may not have been created successfully."));
        }
        else{
          setLEDColor(0, 1, 0);
        }
        Serial.println();
        Serial.print(F("Number of bad sectors within the spare table (cylinder 4D) found during spare table initialization: "));
        printDataNoSpace(driveStatus[3]);
        Serial.println();
        Serial.println();
        setLEDColor(0, 1, 0);
        Serial.println(F("Low-level format finished! "));
      }
      Serial.println();
      setLEDColor(0, 1, 0);
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }
    else if(command.equals("7") and testMenu == false and diagMenu == true and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){ //Read RAM
      clearScreen();
      setLEDColor(0, 1, 0);
      uint16_t RAMIndex = 0;
      Serial.print(F("Please enter the base RAM address that you want to view: "));
      while(1){
        if(readSerialValue(4) == true){
          RAMIndex = (serialBytes[0] << 8) | serialBytes[1];
          if(RAMIndex < 1024){
            break;
          }
        }
        Serial.print(F("Please enter the base RAM address that you want to view: "));
      }
      Serial.println();
      setLEDColor(0, 0, 0);
      Serial.print(F("Reading from RAM address 00"));
      printDataNoSpace(RAMIndex >> 8);
      printDataNoSpace(RAMIndex);
      Serial.print(F(" through 00"));
      int finalRAMIndex = RAMIndex + 531;
      if(finalRAMIndex > 1023){
        finalRAMIndex = 1023;
      }
      printDataNoSpace(finalRAMIndex >> 8);
      printDataNoSpace(finalRAMIndex);
      Serial.println(F("..."));
      Serial.print(F("Command: 08 "));
      printDataSpace(RAMIndex >> 8);
      printDataSpace(RAMIndex);
      printDataSpace(0x00);
      printDataSpace(defaultRetries);
      printDataNoSpace(defaultSpareThreshold);
      Serial.println();
      bool readSuccess = customProfileRead(0x08, (RAMIndex >> 8), RAMIndex, 0x00, defaultRetries, defaultSpareThreshold);
      if(readSuccess == 0){
        Serial.println();
        setLEDColor(1, 0, 0);
        Serial.println(F("WARNING: Errors were encountered during the read operation. The following data may be incorrect."));
      }
      else{
        setLEDColor(0, 1, 0);
      }
      byte tempDataArray[1064];
      for(int i = 0; i < 532; i++){
        tempDataArray[i + 4] = blockData[i];
      }
      for(int i = 0; i < 4; i++){
        tempDataArray[i] = driveStatus[i];
      }
      for(int i = 0; i < 532; i++){
        blockData[i] = tempDataArray[i];
      }
      Serial.println();
      printRawData();
      Serial.println();
      setLEDColor(0, 1, 0);
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }
    else if(command.equals("8") and testMenu == false and diagMenu == true and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){ //Write RAM
      clearScreen();
      setLEDColor(0, 1, 0);
      uint16_t RAMIndex = 0;
      Serial.print(F("Please enter the base RAM address that you want to write the buffer to: "));
      while(1){
        if(readSerialValue(4) == true){
          RAMIndex = (serialBytes[0] << 8) | serialBytes[1];
          if(RAMIndex < 1024){
            break;
          }
        }
        Serial.print(F("Please enter the base RAM address that you want to write the buffer to: "));
      }
      uint16_t bytesToWrite = 1024 - RAMIndex;
      if(bytesToWrite > 532){
        bytesToWrite = 532;
      }
      Serial.println();
      setLEDColor(0, 0, 0);
      Serial.print(F("Writing the buffer to RAM addresses 00"));
      printDataNoSpace(RAMIndex >> 8);
      printDataNoSpace(RAMIndex);
      Serial.print(F(" through 00"));
      printDataNoSpace((RAMIndex + bytesToWrite - 1) >> 8);
      printDataNoSpace(RAMIndex + bytesToWrite - 1);
      Serial.println(F("..."));
      Serial.print(F("Command: 09 "));
      printDataSpace(RAMIndex >> 8);
      printDataSpace(RAMIndex);
      printDataSpace(0x00);
      printDataSpace(defaultRetries);
      printDataNoSpace(defaultSpareThreshold);
      Serial.println();
      bool writeSuccess = customProfileWrite(0x09, RAMIndex >> 8, RAMIndex, 0x00, defaultRetries, defaultSpareThreshold, bytesToWrite);
      if(writeSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered during the write operation. The data may have been written incorrectly."));
        Serial.println();
        setLEDColor(1, 0, 0);
      }
      else{
        setLEDColor(0, 1, 0);
      }
      Serial.println();
      setLEDColor(0, 1, 0);
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }
    else if(command.equals("9") and testMenu == false and diagMenu == true and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){ //Format
      clearScreen();
      setLEDColor(0, 1, 0);
      confirm();
      if(confirmOperation == true){
        Serial.print(F("Install jumper and press return to continue..."));
        flushInput();
        while(!Serial.available());
        flushInput();
        Serial.println();
        setLEDColor(0, 0, 0);
        Serial.println(F("Formatting drive..."));
        Serial.println(F("Command: 03 00 00 00 0A 03"));
        bool readSuccess = customProfileRead(0x03, 0x00, 0x00, 0x00, defaultRetries, defaultSpareThreshold, true);
        if(readSuccess == 0){
          Serial.println();
          setLEDColor(1, 0, 0);
          Serial.println(F("WARNING: Errors were encountered during the format operation. The format may not have completed successfully. You can remove the jumper now."));
          Serial.println();
        }
        else{
          Serial.println();
          setLEDColor(0, 1, 0);
          Serial.println(F("Format complete! You can remove the jumper now."));
        }
        Serial.print(F("Number of bad blocks found during the format procedure: "));
        printDataNoSpace(driveStatus[3]);
        Serial.println();
        Serial.println();
        Serial.println(F("Bad block data returned by the format command:"));
        Serial.println();
        printRawData();
        Serial.println();
        Serial.println(F("Data Interpretation:"));
        if(blockData[0] == 0xFF and blockData[1] == 0xFF and blockData[2] == 0xFF and blockData[3] == 0xFF){
          Serial.println(F("No bad blocks found!"));
        }
        else{
          for(int i = 0; i < 532; i += 4){
            if((blockData[i] == 0xFF and blockData[i + 1] == 0xFF and blockData[i + 2] == 0xFF and blockData[i + 3] == 0xFF) or (i > driveStatus[3] * 4)){
              break;
            }
            Serial.print(F("Bad block on cylinder "));
            printDataNoSpace(blockData[i]);
            Serial.print(F(", head "));
            printDataNoSpace(blockData[i + 1]);
            Serial.print(F(", and sector "));
            printDataNoSpace(blockData[i + 2]);
            Serial.print(F(" (AKA block "));
            uint32_t LBA = 0;
            if(blockData[i] < 77){
              LBA = 64 * blockData[i] + 16 * blockData[i + 1] + blockData[i + 2];
              printDataNoSpace(LBA >> 16);
              printDataNoSpace(LBA >> 8);
              printDataNoSpace(LBA);
            }
            else if(blockData[i] > 77){
              LBA = 64 * (blockData[i] - 1) + 16 * blockData[i + 1] + blockData[i + 2];
              printDataNoSpace(LBA >> 16);
              printDataNoSpace(LBA >> 8);
              printDataNoSpace(LBA);
            }
            else{
              Serial.print(F("SPARE TABLE"));
            }
            Serial.print(F("). Reason for block being bad: "));
            bool statusGood = 1;
            for(int j = 0; j < 8; j++){
              if(bitRead(blockData[i + 3], j) == 1){
                statusGood = 0;
                Serial.print(statusMessages[2][j]);
                Serial.print(F(" "));
              }
            }
            if(statusGood == 1){
              Serial.println(F("Unknown"));
            }
            else{
              Serial.println();
            }
          }
        }
      }
      Serial.println();
      setLEDColor(0, 1, 0);
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }
    else if(command.equalsIgnoreCase("A") and testMenu == false and diagMenu == true and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){ //Scan
      clearScreen();
      setLEDColor(0, 0, 0);
      Serial.println(F("Performing a surface scan on the drive..."));
      Serial.println(F("Command: 04 00 00 00 0A 03"));
      bool readSuccess = customProfileRead(0x04, 0x00, 0x00, 0x00, defaultRetries, defaultSpareThreshold, true);
      if(readSuccess == 0){
        Serial.println();
        setLEDColor(1, 0, 0);
        Serial.println(F("WARNING: Errors were encountered during the surface scan operation. The scan may not have completed successfully."));
        Serial.println();
      }
      else{
        setLEDColor(0, 1, 0);
        Serial.println();
        Serial.println(F("Surface scan complete!"));
      }
      Serial.print(F("Number of bad blocks found during the scan: "));
      printDataNoSpace(driveStatus[3]);
      Serial.println();
      Serial.println();
      Serial.println(F("List of these bad blocks:"));
      Serial.println();
      printRawData();
      Serial.println();
      Serial.println(F("Data Interpretation:"));
      if(blockData[0] == 0xFF and blockData[1] == 0xFF and blockData[2] == 0xFF and blockData[3] == 0xFF){
        Serial.println(F("No bad blocks found!"));
      }
      else{
        for(int i = 0; i < 532; i += 4){
          if((blockData[i] == 0xFF and blockData[i + 1] == 0xFF and blockData[i + 2] == 0xFF and blockData[i + 3] == 0xFF) or (i > driveStatus[3] * 4)){
            break;
          }
          Serial.print(F("Bad block on cylinder "));
          printDataNoSpace(blockData[i]);
          Serial.print(F(", head "));
          printDataNoSpace(blockData[i + 1]);
          Serial.print(F(", and sector "));
          printDataNoSpace(blockData[i + 2]);
          Serial.print(F(" (AKA block "));
          uint32_t LBA = 0;
          if(blockData[i] < 77){
            LBA = 64 * blockData[i] + 16 * blockData[i + 1] + blockData[i + 2];
            printDataNoSpace(LBA >> 16);
            printDataNoSpace(LBA >> 8);
            printDataNoSpace(LBA);
          }
          else if(blockData[i] > 77){
            LBA = 64 * (blockData[i] - 1) + 16 * blockData[i + 1] + blockData[i + 2];
            printDataNoSpace(LBA >> 16);
            printDataNoSpace(LBA >> 8);
            printDataNoSpace(LBA);
          }
          else{
            Serial.print(F("SPARE TABLE"));
          }
          Serial.print(F("). Reason for block being bad: "));
          bool statusGood = 1;
          for(int j = 0; j < 8; j++){
            if(bitRead(blockData[i + 3], j) == 1){
              statusGood = 0;
              Serial.print(statusMessages[2][j]);
              Serial.print(F(" "));
            }
          }
          if(statusGood == 1){
            Serial.println(F("Unknown"));
          }
          else{
            Serial.println();
          }
        }
      }
      Serial.println();
      setLEDColor(0, 1, 0);
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }
    else if(command.equalsIgnoreCase("B") and testMenu == false and diagMenu == true and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){ //Init Spare Table
      clearScreen();
      setLEDColor(0, 0, 0);
      Serial.println(F("Initializing the drive's spare table..."));
      Serial.println(F("Command: 05 00 00 00 0A 03"));
      bool readSuccess = customProfileRead(0x05, 0x00, 0x00, 0x00, defaultRetries, defaultSpareThreshold);
      if(readSuccess == 0){
        Serial.println();
        Serial.println();
        setLEDColor(1, 0, 0);
        Serial.println(F("WARNING: Errors were encountered during the init spare table operation. The spare table may not have been created successfully."));
        Serial.println();
      }
      else{
        Serial.println();
        Serial.println();
        setLEDColor(0, 1, 0);
        Serial.println(F("Spare table initialization complete!"));
      }
      Serial.print(F("Number of bad sectors within the spare table (cylinder 4D) found during spare table initialization: "));
      printDataNoSpace(driveStatus[3]);
      Serial.println();
      Serial.println();
      Serial.println(F("List of these bad sectors:"));
      Serial.println();
      printRawData();
      Serial.println();
      Serial.println(F("Data Interpretation:"));
      if(driveStatus[3] == 0){
        Serial.println(F("No bad sectors found!"));
      }
      else{
        for(int i = 0; i < driveStatus[3]; i++){
          Serial.print(F("Sector "));
          printDataNoSpace(blockData[i] & 0x0F);
          Serial.print(F(" on head "));
          printDataNoSpace(blockData[i] >> 4);
          Serial.println(F(" is bad."));
        }
      }
      Serial.println();
      setLEDColor(0, 1, 0);
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }
    
    else if(command.equalsIgnoreCase("C") and testMenu == false and diagMenu == true and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false and widgetMenu == false){ //Disable Stepper
      clearScreen();
      setLEDColor(0, 0, 0);
      Serial.println(F("Disabling the head stepper motor..."));
      Serial.println(F("Command: 10 00 00 00 0A 03"));
      byte oldDriveData[1064];
      for(int i = 0; i < 532; i++){
        oldDriveData[i] = blockData[i];
      }
      bool readSuccess = customProfileRead(0x10, 0x00, 0x00, 0x00, defaultRetries, defaultSpareThreshold);
      for(int i = 0; i < 532; i++){
        blockData[i] = oldDriveData[i];
      }
      if(readSuccess == 0){
        Serial.println();
        setLEDColor(1, 0, 0);
        Serial.println(F("WARNING: Errors were encountered while disabling the stepper motor. The motor might still be engaged."));
        Serial.println();
      }
      else{
        Serial.println();
        setLEDColor(0, 1, 0);
        Serial.println(F("Stepper has been disabled!"));
      }
      Serial.println();
      setLEDColor(0, 1, 0);
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }
    else if(command.equalsIgnoreCase("G") and testMenu == false and diagMenu == true and diagMenuTenMeg == false and widgetMenu == false and widgetServoMenu == false){ //Main Menu
      diagMenu = false;
      clearScreen();
      flushInput();
    }

    else if(command.equalsIgnoreCase("1") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){ //Read
      clearScreen();
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.print(F("Please enter the cylinder, head, and sector that you want to read from in the format (CC)CCHHSS: "));
      while(1){
        if(readSerialValue(8) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the cylinder, head, and sector that you want to read from in the format (CC)CCHHSS: "));
        }
      }
      for(int i = 0; i < 4; i++){
        commandBufferTenMegDiag[i + 2] = serialBytes[i];
      }
      commandBufferTenMegDiag[0] = 0x06;
      commandBufferTenMegDiag[1] = 0x00;
      calcChecksum();
      Serial.println();
      Serial.print(F("Reading cylinder "));
      printDataNoSpace(commandBufferTenMegDiag[2]);
      printDataNoSpace(commandBufferTenMegDiag[3]);
      Serial.print(F(", head "));
      printDataNoSpace(commandBufferTenMegDiag[4]);
      Serial.print(F(", and sector "));
      printDataNoSpace(commandBufferTenMegDiag[5]);
      Serial.println(F("..."));
      Serial.print(F("Command: "));
      for(int i = 0; i < 7; i++){
        printDataSpace(commandBufferTenMegDiag[i]);
      }
      Serial.println();
      Serial.println();
      bool readSuccess = tenMegDiagRead();
      if(readSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered during the read operation. The following data may be incorrect."));
        Serial.println();
        setLEDColor(1, 0, 0);
      }
      else{
        setLEDColor(0, 1, 0);
      }
      printRawData();
      Serial.println();
      printStatus();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("2") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){ //Write
      clearScreen();
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.print(F("Please enter the cylinder, head, and sector that you want to write to in the format (CC)CCHHSS: "));
      while(1){
        if(readSerialValue(8) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the cylinder, head, and sector that you want to write to in the format (CC)CCHHSS: "));
        }
      }
      for(int i = 0; i < 4; i++){
        commandBufferTenMegDiag[i + 2] = serialBytes[i];
      }
      commandBufferTenMegDiag[0] = 0x06;
      commandBufferTenMegDiag[1] = 0x01;
      calcChecksum();
      Serial.println();
      Serial.print(F("Writing the buffer to cylinder "));
      printDataNoSpace(commandBufferTenMegDiag[2]);
      printDataNoSpace(commandBufferTenMegDiag[3]);
      Serial.print(F(", head "));
      printDataNoSpace(commandBufferTenMegDiag[4]);
      Serial.print(F(", and sector "));
      printDataNoSpace(commandBufferTenMegDiag[5]);
      Serial.println(F("..."));
      Serial.print(F("Command: "));
      for(int i = 0; i < 7; i++){
        printDataSpace(commandBufferTenMegDiag[i]);
      }
      Serial.println();
      Serial.println();
      bool writeSuccess = tenMegDiagWrite();
      if(writeSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered during the write operation. The data may have been written incorrectly."));
        Serial.println();
        setLEDColor(1, 0, 0);
      }
      else{
        setLEDColor(0, 1, 0);
      }
      printStatus();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("3") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){ //Repeatedly Read CHS
      clearScreen();
      clearScreen();
      setLEDColor(0, 1, 0);
      uint32_t passCount = 1;
      Serial.print(F("Please enter the cylinder, head, and sector that you want to repeatedly read from in the format (CC)CCHHSS: "));
      while(1){
        if(readSerialValue(8) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the cylinder, head, and sector that you want to repeatedly read from in the format (CC)CCHHSS: "));
        }
      }
      for(int i = 0; i < 4; i++){
        commandBufferTenMegDiag[i + 2] = serialBytes[i];
      }
      commandBufferTenMegDiag[0] = 0x06;
      commandBufferTenMegDiag[1] = 0x00;
      calcChecksum();
      Serial.println();
      while(!Serial.available()){
        Serial.print(F("Repeatedly reading cylinder "));
        printDataNoSpace(commandBufferTenMegDiag[2]);
        printDataNoSpace(commandBufferTenMegDiag[3]);
        Serial.print(F(", head "));
        printDataNoSpace(commandBufferTenMegDiag[4]);
        Serial.print(F(", and sector "));
        printDataNoSpace(commandBufferTenMegDiag[5]);
        Serial.print(F(" - Pass "));
        printDataNoSpace(passCount >> 24);
        printDataNoSpace(passCount >> 16);
        printDataNoSpace(passCount >> 8);
        printDataNoSpace(passCount);
        Serial.print(F("..."));
        Serial.write("\033[1000D");
        tenMegDiagRead();
        if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
          setLEDColor(1, 0, 0);
          Serial.println();
          Serial.println();
          Serial.print(F("Error reading on pass "));
          printDataNoSpace(passCount >> 24);
          printDataNoSpace(passCount >> 16);
          printDataNoSpace(passCount >> 8);
          printDataNoSpace(passCount);
          Serial.println(F("!"));
          Serial.println(F("Status Interpretation:"));
          for(int k = 0; k < 3; k++){
            for(int l = 0; l < 8; l++){
              if(bitRead(driveStatus[k], l) == 1){
                //statusGood = 0;
                Serial.println(statusMessages[k][l]);
              }
            }
          }
          Serial.println();
        }
        passCount += 1;
      }
      flushInput();
      Serial.println();
      Serial.println();
      Serial.println(F("Repeated read test terminated."));
      Serial.println();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("4") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){ //Repeatedly Write CHS
      clearScreen();
      clearScreen();
      setLEDColor(0, 1, 0);
      uint32_t passCount = 1;
      Serial.print(F("Please enter the cylinder, head, and sector that you want to repeatedly write to in the format (CC)CCHHSS: "));
      while(1){
        if(readSerialValue(8) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the cylinder, head, and sector that you want to repeatedly write to in the format (CC)CCHHSS: "));
        }
      }
      for(int i = 0; i < 4; i++){
        commandBufferTenMegDiag[i + 2] = serialBytes[i];
      }
      commandBufferTenMegDiag[0] = 0x06;
      commandBufferTenMegDiag[1] = 0x01;
      calcChecksum();
      Serial.println();
      while(!Serial.available()){
        Serial.print(F("Repeatedly writing cylinder "));
        printDataNoSpace(commandBufferTenMegDiag[2]);
        printDataNoSpace(commandBufferTenMegDiag[3]);
        Serial.print(F(", head "));
        printDataNoSpace(commandBufferTenMegDiag[4]);
        Serial.print(F(", and sector "));
        printDataNoSpace(commandBufferTenMegDiag[5]);
        Serial.print(F(" - Pass "));
        printDataNoSpace(passCount >> 24);
        printDataNoSpace(passCount >> 16);
        printDataNoSpace(passCount >> 8);
        printDataNoSpace(passCount);
        Serial.print(F("..."));
        Serial.write("\033[1000D");
        tenMegDiagWrite();
        if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
          setLEDColor(1, 0, 0);
          Serial.println();
          Serial.println();
          Serial.print(F("Error writing on pass "));
          printDataNoSpace(passCount >> 24);
          printDataNoSpace(passCount >> 16);
          printDataNoSpace(passCount >> 8);
          printDataNoSpace(passCount);
          Serial.println(F("!"));
          Serial.println(F("Status Interpretation:"));
          for(int k = 0; k < 3; k++){
            for(int l = 0; l < 8; l++){
              if(bitRead(driveStatus[k], l) == 1){
                //statusGood = 0;
                Serial.println(statusMessages[k][l]);
              }
            }
          }
          Serial.println();
        }
        passCount += 1;
      }
      flushInput();
      Serial.println();
      Serial.println();
      Serial.println(F("Repeated write test terminated."));
      Serial.println();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("5") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){ //Seek
      clearScreen();
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.println(F("Note: If you plan to format the cylinder that you're seeking to, you must use a sector of 00 or else the format will fail!"));
      Serial.print(F("Please enter the cylinder, head, and sector that you want to seek to in the format (CC)CCHHSS: "));
      while(1){
        if(readSerialValue(8) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the cylinder, head, and sector that you want to seek to in the format (CC)CCHHSS: "));
        }
      }
      for(int i = 0; i < 4; i++){
        commandBufferTenMegDiag[i + 2] = serialBytes[i];
      }
      commandBufferTenMegDiag[0] = 0x06;
      commandBufferTenMegDiag[1] = 0x03;
      calcChecksum();
      Serial.println();
      Serial.print(F("Seeking to cylinder "));
      printDataNoSpace(commandBufferTenMegDiag[2]);
      printDataNoSpace(commandBufferTenMegDiag[3]);
      Serial.print(F(", head "));
      printDataNoSpace(commandBufferTenMegDiag[4]);
      Serial.print(F(", and sector "));
      printDataNoSpace(commandBufferTenMegDiag[5]);
      Serial.println(F("..."));
      Serial.print(F("Command: "));
      for(int i = 0; i < 7; i++){
        printDataSpace(commandBufferTenMegDiag[i]);
      }
      Serial.println();
      Serial.println();
      bool readSuccess = tenMegDiagRead(false);
      if(readSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered during the seek operation. The heads may not be positioned over the desired track."));
        Serial.println();
        setLEDColor(1, 0, 0);
      }
      else{
        setLEDColor(0, 1, 0);
      }
      printStatus();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("6") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){ //LLF
      clearScreen();
      clearScreen();
      Serial.println("Note: This low-level format command will show the number of bad blocks and sectors that are encountered while formatting the drive, but it will NOT show a list of the actual blocks and sectors that are bad. If you want that (or if you want the ability to skip verifying the integrity of the spare table sectors), manually run the Format Track(s), Init Spare Table, and Scan commands in that order to achieve the same low-level format with more error information and control.");
      Serial.println();
      confirm();
      if(confirmOperation == true){
        int prevErrorCount = 0;
        Serial.print(F("Install jumper and press return to continue..."));
        flushInput();
        while(!Serial.available());
        flushInput();
        Serial.println();
        uint16_t seekErrors = 0; 
        for(uint16_t currentCylinder = 0x00; currentCylinder < 0x0132; currentCylinder++){
          for(byte currentHead = 0x00; currentHead < 0x04; currentHead++){
            Serial.print("Step 1 - Now formatting cylinder ");
            printDataNoSpace(currentCylinder >> 8);
            printDataNoSpace(currentCylinder);
            Serial.print(" of 0131 on head ");
            printDataNoSpace(currentHead);
            Serial.print(" of 03. Progress: ");
            Serial.print(((float)currentCylinder/0x0131)*100);
            Serial.print(F("%"));
            Serial.write("\033[1000D");
            commandBufferTenMegDiag[0] = 0x06;
            commandBufferTenMegDiag[1] = 0x03;
            commandBufferTenMegDiag[2] = currentCylinder >> 8;
            commandBufferTenMegDiag[3] = currentCylinder;
            commandBufferTenMegDiag[4] = currentHead;
            commandBufferTenMegDiag[5] = 0x00; //Is this right? BLU seeks to sector zero, but the LLF source code seems to indicate that we should seek to sector one.
            calcChecksum();
            tenMegDiagRead(false);
            if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
              seekErrors += 1;
            }
            commandBufferTenMegDiag[0] = 0x02;
            commandBufferTenMegDiag[1] = 0x04;
            calcChecksum();
            if(currentCylinder == 0x0131 and currentHead == 0x03){
              tenMegDiagRead();
            }
            else{
              tenMegDiagRead(false);
            }
          }
        }
        Serial.println();
        Serial.println();
        Serial.print(F("Number of seek errors during the format procedure: "));
        printDataNoSpace(seekErrors >> 8);
        printDataNoSpace(seekErrors);
        Serial.println();
        Serial.print(F("Number of bad blocks found during the format procedure: "));
        printDataNoSpace(driveStatus[3]);
        Serial.println();
        Serial.println();
        Serial.print(F("Remove jumper and press return to continue..."));
        flushInput();
        while(!Serial.available());
        Serial.println();
        flushInput();
        uint16_t spareValidationErrors = 0;
        uint16_t commErrors = 0;
        bool failure = false;
        byte testPattern[532];
        for(int iteration = 0; iteration < 5; iteration++){
          if(iteration == 0){
            for(int i = 0; i < 532; i += 2){
              testPattern[i] = 0xD6;
              testPattern[i+1] = 0xB9;
            }
          }
          if(iteration == 1){
            for(int i = 0; i < 532; i += 2){
              testPattern[i] = 0x6D;
              testPattern[i+1] = 0xB6;
              if(i > 529){
                testPattern[i] = 0x00;
              }
            }
          }
          if(iteration == 2){
            for(int i = 0; i < 532; i++){
              testPattern[i] = 0xC6;
              if(i > 529){
                testPattern[i] = 0x00;
              }
            }
          }
          if(iteration == 3){
            for(int i = 0; i < 532; i++){
              testPattern[i] = i;
              if(i > 529){
                testPattern[i] = 0x00;
              }
            }
          }
          if(iteration == 4){
            for(int i = 0; i < 532; i++){
              testPattern[i] = 531 - i;
              if(i > 529){
                testPattern[i] = 0x00;
              }
            }
          }
          for(byte currentHead = 0x00; currentHead <= 0x03; currentHead++){
            for(byte currentSector = 0x00; currentSector <= 0x0F; currentSector++){
              Serial.print("Step 2 - Now checking the integrity of sector ");
              printDataNoSpace(currentSector);
              Serial.print(" on head ");
              printDataNoSpace(currentHead);
              Serial.print(" of cylinder 0099");
              Serial.print(" with the test pattern ");
              if(iteration == 0){
                Serial.print("D6B9");
              }
              if(iteration == 1){
                Serial.print("6DB6");
              }
              if(iteration == 2){
                Serial.print("C6");
              }
              if(iteration == 3){
                Serial.print("of ascending numbers");
              }
              if(iteration == 4){
                Serial.print("of descending numbers");
              }
              Serial.print(".");
              Serial.write("\033[1000D");
              for(int i = 0; i < 532; i++){
                blockData[i] = testPattern[i];
              }
              commandBufferTenMegDiag[0] = 0x06;
              commandBufferTenMegDiag[1] = 0x01;
              commandBufferTenMegDiag[2] = 0x00;
              commandBufferTenMegDiag[3] = 0x99;
              commandBufferTenMegDiag[4] = currentHead;
              commandBufferTenMegDiag[5] = currentSector;
              calcChecksum();
              tenMegDiagWrite();
              if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
                commErrors += 1;
              }
              commandBufferTenMegDiag[0] = 0x06;
              commandBufferTenMegDiag[1] = 0x00;
              commandBufferTenMegDiag[2] = 0x00;
              commandBufferTenMegDiag[3] = 0x99;
              commandBufferTenMegDiag[4] = currentHead;
              commandBufferTenMegDiag[5] = currentSector;
              calcChecksum();
              tenMegDiagRead();
              if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
                commErrors += 1;
              }
              for(int i = 0; i < 532; i++){
                if(blockData[i] != testPattern[i]){
                  spareValidationErrors += 1;
                  break;
                }
              }
              //Serial.println();
            }
          }
          Serial.println();
        }
        Serial.println();
        Serial.print(F("Number of communications errors: "));
        printDataNoSpace(commErrors >> 8);
        printDataNoSpace(commErrors);
        Serial.println();
        Serial.print(F("Number of tests that failed the integrity check: "));
        printDataNoSpace(spareValidationErrors >> 8);
        printDataNoSpace(spareValidationErrors);
        Serial.println();
        Serial.println();
        Serial.println("Step 3 - Creating the spare table...");
        commandBufferTenMegDiag[0] = 0x02;
        commandBufferTenMegDiag[1] = 0x05;
        calcChecksum();
        Serial.print(F("Command: "));
        for(int i = 0; i < 3; i++){
          printDataSpace(commandBufferTenMegDiag[i]);
        }
        Serial.println();
        bool readSuccess = tenMegDiagRead();
        if(readSuccess == 0){
          Serial.println();
          setLEDColor(1, 0, 0);
          Serial.println(F("WARNING: Errors were encountered while creating the spare table. The table may not have may not have been created successfully."));
          Serial.println();
        }
        Serial.println();
        Serial.print(F("Number of bad sectors within the spare table (cylinder 0099) found during spare table initialization: "));
        printDataNoSpace(driveStatus[3]);
        Serial.println();
        Serial.println();
        Serial.println(F("Step 4 - Performing a surface scan..."));
        commandBufferTenMegDiag[0] = 0x02;
        commandBufferTenMegDiag[1] = 0x02;
        calcChecksum();
        Serial.print(F("Command: "));
        for(int i = 0; i < 3; i++){
          printDataSpace(commandBufferTenMegDiag[i]);
        }
        Serial.println();
        readSuccess = tenMegDiagRead(true, false);
        if(readSuccess == 0){
          Serial.println();
          setLEDColor(1, 0, 0);
          Serial.println(F("WARNING: Errors were encountered during the surface scan operation. The scan may not have completed successfully."));
          Serial.println();
        }
        Serial.println();
        Serial.print(F("Number of bad blocks found during the surface scan: "));
        printDataNoSpace(driveStatus[3]);
        Serial.println();
        Serial.println();
        Serial.println(F("Step 5 - Parking the drive's heads..."));
        commandBufferTenMegDiag[0] = 0x06;
        commandBufferTenMegDiag[1] = 0x03;
        commandBufferTenMegDiag[2] = 0x00;
        commandBufferTenMegDiag[3] = 0x99;
        commandBufferTenMegDiag[4] = 0x00;
        commandBufferTenMegDiag[5] = 0x00;
        calcChecksum();
        tenMegDiagRead();
        commandBufferTenMegDiag[0] = 0x02;
        commandBufferTenMegDiag[1] = 0x0C;
        calcChecksum();
        Serial.print(F("Command: "));
        for(int i = 0; i < 3; i++){
          printDataSpace(commandBufferTenMegDiag[i]);
        }
        Serial.println();
        Serial.println();
        tenMegDiagRead(false, true);
        Serial.println(F("Low-level format finished! "));
      }
      Serial.println();
      setLEDColor(0, 1, 0);
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("7") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){ //Format Track(s)
    clearScreen();
    clearScreen();
    bool formatAllTracks = false;
      Serial.print(F("Do you want to just format the current track or all tracks on the disk (return for current track, 'a' for all tracks)? "));
      while(1){
        if(Serial.available()) {
          delay(50);
          userInput = Serial.read();
          flushInput();
          if(userInput == 'a'){
            formatAllTracks = true;
            break;
          }
          else if(userInput == '\r'){
            break;
          }
          else{
            while(Serial.available()){
              Serial.read();
            }
            Serial.print(F("Do you want to just format the current track or all tracks on the disk (return for current track, 'a' for all tracks)? "));
          }
        }
      }
      Serial.println();
      if(formatAllTracks == false){
        commandBufferTenMegDiag[0] = 0x02;
        commandBufferTenMegDiag[1] = 0x04;
        calcChecksum();
        Serial.print(F("Install jumper and press return to continue..."));
        flushInput();
        while(!Serial.available());
        flushInput();
        Serial.println();
        Serial.println("Formatting current track...");
        Serial.print("Command: ");
        for(int i = 0; i < 3; i++){
          printDataSpace(commandBufferTenMegDiag[i]);
        }
        Serial.println();
        setLEDColor(0, 0, 0);
        bool readSuccess = tenMegDiagRead();
        if(readSuccess == 0){
          setLEDColor(1, 0, 0);
          Serial.println();
          Serial.println(F("WARNING: Errors were encountered while formatting the track. The format may not have completed successfully. You can remove the jumper now."));
          Serial.println();
        }
        else{
          Serial.println();
          setLEDColor(0, 1, 0);
          Serial.println(F("Format complete! You can remove the jumper now."));
        }
        Serial.print(F("Number of bad blocks found while formatting: "));
        printDataNoSpace(driveStatus[3]);
        Serial.println();
        Serial.println();
        Serial.println(F("List of these bad blocks:"));
        Serial.println();
        printRawData();
        Serial.println();
        Serial.println(F("Data Interpretation:"));
        if(blockData[0] == 0xFF and blockData[1] == 0xFF and blockData[2] == 0xFF and blockData[3] == 0xFF){
          Serial.println(F("No bad blocks found!"));
        }
        else{
          for(int i = 0; i < 532; i += 4){
            if((blockData[i] == 0xFF and blockData[i + 1] == 0xFF and blockData[i + 2] == 0xFF and blockData[i + 3] == 0xFF) or (i > driveStatus[3] * 4)){
              break;
            }
            Serial.print(F("Bad block on cylinder "));
            printDataNoSpace(blockData[i]);
            printDataNoSpace(blockData[i + 1]);
            Serial.print(F(", head "));
            printDataNoSpace((blockData[i + 2] & 0xF0) >> 4);
            Serial.print(F(", and sector "));
            printDataNoSpace(blockData[i + 2] & 0x0F);
            Serial.print(F(" (AKA block "));
            uint32_t LBA = 0;
            uint16_t completeCylinder = blockData[i];
            completeCylinder = completeCylinder << 8;
            completeCylinder += blockData[i + 1];
            if(completeCylinder < 153){
              LBA = 64 * completeCylinder + 16 * ((blockData[i + 2] & 0xF0) >> 4) + (blockData[i + 2] & 0x0F);
              printDataNoSpace(LBA >> 16);
              printDataNoSpace(LBA >> 8);
              printDataNoSpace(LBA);
            }
            else if(completeCylinder > 153){
              LBA = 64 * (completeCylinder - 1) + 16 * ((blockData[i + 2] & 0xF0) >> 4) + (blockData[i + 2] & 0x0F);
              printDataNoSpace(LBA >> 16);
              printDataNoSpace(LBA >> 8);
              printDataNoSpace(LBA);
            }
            else{
              Serial.print(F("SPARE TABLE"));
            }
            Serial.print(F("). Reason for block being bad: "));
            bool statusGood = 1;
            for(int j = 0; j < 8; j++){
              if(bitRead(blockData[i + 3], j) == 1){
                statusGood = 0;
                Serial.print(statusMessages[2][j]);
                Serial.print(F(" "));
              }
            }
            if(statusGood == 1){
              Serial.println(F("Unknown"));
            }
            else{
              Serial.println();
            }
          }
        }
      }
      else{
        confirm();
        bool abort = false;
        if(confirmOperation == true){
          int prevErrorCount = 0;
          Serial.print(F("Install jumper and press return to continue..."));
          flushInput();
          while(!Serial.available());
          flushInput();
          Serial.println();
          for(uint16_t currentCylinder = 0x00; currentCylinder < 0x0132; currentCylinder++){
            if(Serial.available()){
              abort = true;
              break;
            }
            for(byte currentHead = 0x00; currentHead < 0x04; currentHead++){
              Serial.print("Now formatting cylinder ");
              printDataNoSpace(currentCylinder >> 8);
              printDataNoSpace(currentCylinder);
              Serial.print(" of 0131 on head ");
              printDataNoSpace(currentHead);
              Serial.print(" of 03. Progress: ");
              Serial.print(((float)currentCylinder/0x0131)*100);
              Serial.print(F("%"));
              Serial.write("\033[1000D");
              commandBufferTenMegDiag[0] = 0x06;
              commandBufferTenMegDiag[1] = 0x03;
              commandBufferTenMegDiag[2] = currentCylinder >> 8;
              commandBufferTenMegDiag[3] = currentCylinder;
              commandBufferTenMegDiag[4] = currentHead;
              commandBufferTenMegDiag[5] = 0x00; //Is this right? BLU seeks to sector zero, but the LLF source code seems to indicate that we should seek to sector one.
              calcChecksum();
              tenMegDiagRead(false);
              if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
                Serial.println();
                Serial.println();
                Serial.print(F("Error seeking to cylinder "));
                printDataNoSpace(currentCylinder >> 8);
                printDataNoSpace(currentCylinder);
                Serial.print(F(" on head "));
                printDataNoSpace(currentHead);
                Serial.println(F("!"));
                Serial.println(F("Status Interpretation:"));
                for(int k = 0; k < 3; k++){
                  for(int l = 0; l < 8; l++){
                    if(bitRead(driveStatus[k], l) == 1){
                      //statusGood = 0;
                      Serial.println(statusMessages[k][l]); //All status errors show up every time
                    }
                  }
                }
                Serial.println();
              }
              commandBufferTenMegDiag[0] = 0x02;
              commandBufferTenMegDiag[1] = 0x04;
              calcChecksum();
              if(currentCylinder == 0x0131 and currentHead == 0x03){
                tenMegDiagRead();
              }
              else{
                tenMegDiagRead(false);
              }
              if(driveStatus[3] > prevErrorCount){
                Serial.println();
                Serial.println();
                prevErrorCount = driveStatus[3];
                Serial.print("Error while formatting cylinder ");
                printDataNoSpace(currentCylinder >> 8);
                printDataNoSpace(currentCylinder);
                Serial.print(F(" on head "));
                printDataNoSpace(currentHead);
                Serial.println(F("!"));
                Serial.println();
              }
            }
          }
          Serial.println();
          Serial.println();
          if(abort == false){
            Serial.println(F("Format complete! You can remove the jumper now."));
          }
          else{
            Serial.println(F("Format terminated by keypress. You can remove the jumper now."));
            Serial.println();
          }
          Serial.print(F("Number of bad blocks found while formatting: "));
          printDataNoSpace(driveStatus[3]);
          Serial.println();
          Serial.println();
          Serial.println(F("List of these bad blocks:"));
          Serial.println();
          printRawData();
          Serial.println();
          Serial.println(F("Data Interpretation:"));
          if(blockData[0] == 0xFF and blockData[1] == 0xFF and blockData[2] == 0xFF and blockData[3] == 0xFF){
            Serial.println(F("No bad blocks found!"));
          }
          else{
            for(int i = 0; i < 532; i += 4){
              if((blockData[i] == 0xFF and blockData[i + 1] == 0xFF and blockData[i + 2] == 0xFF and blockData[i + 3] == 0xFF) or (i > driveStatus[3] * 4)){
                break;
              }
              Serial.print(F("Bad block on cylinder "));
              printDataNoSpace(blockData[i]);
              printDataNoSpace(blockData[i + 1]);
              Serial.print(F(", head "));
              printDataNoSpace((blockData[i + 2] & 0xF0) >> 4);
              Serial.print(F(", and sector "));
              printDataNoSpace(blockData[i + 2] & 0x0F);
              Serial.print(F(" (AKA block "));
              uint32_t LBA = 0;
              uint16_t completeCylinder = blockData[i];
              completeCylinder = completeCylinder << 8;
              completeCylinder += blockData[i + 1];
              if(completeCylinder < 153){
                LBA = 64 * completeCylinder + 16 * ((blockData[i + 2] & 0xF0) >> 4) + (blockData[i + 2] & 0x0F);
                printDataNoSpace(LBA >> 16);
                printDataNoSpace(LBA >> 8);
                printDataNoSpace(LBA);
              }
              else if(completeCylinder > 153){
                LBA = 64 * (completeCylinder - 1) + 16 * ((blockData[i + 2] & 0xF0) >> 4) + (blockData[i + 2] & 0x0F);
                printDataNoSpace(LBA >> 16);
                printDataNoSpace(LBA >> 8);
                printDataNoSpace(LBA);
              }
              else{
                Serial.print(F("SPARE TABLE"));
              }
              Serial.print(F("). Reason for block being bad: "));
              bool statusGood = 1;
              for(int j = 0; j < 8; j++){
                if(bitRead(blockData[i + 3], j) == 1){
                  statusGood = 0;
                  Serial.print(statusMessages[2][j]);
                  Serial.print(F(" "));
                }
              }
              if(statusGood == 1){
                Serial.println(F("Unknown"));
              }
              else{
                Serial.println();
              }
            }
          }
        }
      }
      Serial.println();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("8") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){ //Scan
      clearScreen();
      setLEDColor(0, 0, 0);
      Serial.println(F("Performing a surface scan on the drive..."));
      commandBufferTenMegDiag[0] = 0x02;
      commandBufferTenMegDiag[1] = 0x02;
      calcChecksum();
      Serial.print(F("Command: "));
      for(int i = 0; i < 3; i++){
        printDataSpace(commandBufferTenMegDiag[i]);
      }
      Serial.println();
      bool readSuccess = tenMegDiagRead(true, false);
      if(readSuccess == 0){
        Serial.println();
        setLEDColor(1, 0, 0);
        Serial.println(F("WARNING: Errors were encountered during the surface scan operation. The scan may not have completed successfully."));
        Serial.println();
      }
      else{
        setLEDColor(0, 1, 0);
        Serial.println();
        Serial.println(F("Surface scan complete!"));
      }
      Serial.print(F("Number of bad blocks found during the scan: "));
      printDataNoSpace(driveStatus[3]);
      Serial.println();
      Serial.println();
      Serial.println(F("List of these bad blocks:"));
      Serial.println();
      printRawData();
      Serial.println();
      Serial.println(F("Data Interpretation:"));
      if(blockData[0] == 0xFF and blockData[1] == 0xFF and blockData[2] == 0xFF and blockData[3] == 0xFF){
        Serial.println(F("No bad blocks found!"));
      }
      else{
        for(int i = 0; i < 532; i += 4){
          if((blockData[i] == 0xFF and blockData[i + 1] == 0xFF and blockData[i + 2] == 0xFF and blockData[i + 3] == 0xFF) or (i > driveStatus[3] * 4)){
            break;
          }
          Serial.print(F("Bad block on cylinder "));
          printDataNoSpace(blockData[i]);
          printDataNoSpace(blockData[i + 1]);
          Serial.print(F(", head "));
          printDataNoSpace((blockData[i + 2] & 0xF0) >> 4);
          Serial.print(F(", and sector "));
          printDataNoSpace(blockData[i + 2] & 0x0F);
          Serial.print(F(" (AKA block "));
          uint32_t LBA = 0;
          uint16_t completeCylinder = blockData[i];
          completeCylinder = completeCylinder << 8;
          completeCylinder += blockData[i + 1];
          if(completeCylinder < 153){
            LBA = 64 * completeCylinder + 16 * ((blockData[i + 2] & 0xF0) >> 4) + (blockData[i + 2] & 0x0F);
            printDataNoSpace(LBA >> 16);
            printDataNoSpace(LBA >> 8);
            printDataNoSpace(LBA);
          }
          else if(completeCylinder > 153){
            LBA = 64 * (completeCylinder - 1) + 16 * ((blockData[i + 2] & 0xF0) >> 4) + (blockData[i + 2] & 0x0F);
            printDataNoSpace(LBA >> 16);
            printDataNoSpace(LBA >> 8);
            printDataNoSpace(LBA);
          }
          else{
            Serial.print(F("SPARE TABLE"));
          }
          Serial.print(F("). Reason for block being bad: "));
          bool statusGood = 1;
          for(int j = 0; j < 8; j++){
            if(bitRead(blockData[i + 3], j) == 1){
              statusGood = 0;
              Serial.print(statusMessages[2][j]);
              Serial.print(F(" "));
            }
          }
          if(statusGood == 1){
            Serial.println(F("Unknown"));
          }
          else{
            Serial.println();
          }
        }
      }
      Serial.println();
      setLEDColor(0, 1, 0);
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }
    else if(command.equalsIgnoreCase("9") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){ //Init Spare Table
      clearScreen();
      clearScreen();
      bool checkSectors = true;
      Serial.print(F("Do you want to check the integrity of the spare table sectors before attempting to initialize the spare table (return for yes, 'n' for no)? "));
      while(1){
        if(Serial.available()) {
          delay(50);
          userInput = Serial.read();
          flushInput();
          if(userInput == 'n'){
            checkSectors = false;
            break;
          }
          else if(userInput == '\r'){
            break;
          }
          else{
            while(Serial.available()){
              Serial.read();
            }
            Serial.print(F("Do you want to check the integrity of the spare table sectors before attempting to initialize the spare table (return for yes, 'n' for no)? "));
          }
        }
      }
      Serial.println();
      bool failure = false;
      if(checkSectors == true){
        byte testPattern[532];
        for(int iteration = 0; iteration < 5; iteration++){
          if(iteration == 0){
            for(int i = 0; i < 532; i += 2){
              testPattern[i] = 0xD6;
              testPattern[i+1] = 0xB9;
            }
          }
          if(iteration == 1){
            for(int i = 0; i < 532; i += 2){
              testPattern[i] = 0x6D;
              testPattern[i+1] = 0xB6;
              if(i > 529){
                testPattern[i] = 0x00;
              }
            }
          }
          if(iteration == 2){
            for(int i = 0; i < 532; i++){
              testPattern[i] = 0xC6;
              if(i > 529){
                testPattern[i] = 0x00;
              }
            }
          }
          if(iteration == 3){
            for(int i = 0; i < 532; i++){
              testPattern[i] = i;
              if(i > 529){
                testPattern[i] = 0x00;
              }
            }
          }
          if(iteration == 4){
            for(int i = 0; i < 532; i++){
              testPattern[i] = 531 - i;
              if(i > 529){
                testPattern[i] = 0x00;
              }
            }
          }
          for(byte currentHead = 0x00; currentHead <= 0x03; currentHead++){
            for(byte currentSector = 0x00; currentSector <= 0x0F; currentSector++){
              Serial.print("Now checking the integrity of sector ");
              printDataNoSpace(currentSector);
              Serial.print(" on head ");
              printDataNoSpace(currentHead);
              Serial.print(" of cylinder 0099");
              Serial.print(" with the test pattern ");
              if(iteration == 0){
                Serial.print("D6B9");
              }
              if(iteration == 1){
                Serial.print("6DB6");
              }
              if(iteration == 2){
                Serial.print("C6");
              }
              if(iteration == 3){
                Serial.print("of ascending numbers");
              }
              if(iteration == 4){
                Serial.print("of descending numbers");
              }
              Serial.print(".");
              Serial.write("\033[1000D");
              for(int i = 0; i < 532; i++){
                blockData[i] = testPattern[i];
              }
              commandBufferTenMegDiag[0] = 0x06;
              commandBufferTenMegDiag[1] = 0x01;
              commandBufferTenMegDiag[2] = 0x00;
              commandBufferTenMegDiag[3] = 0x99;
              commandBufferTenMegDiag[4] = currentHead;
              commandBufferTenMegDiag[5] = currentSector;
              calcChecksum();
              tenMegDiagWrite();
              if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
                Serial.println();
                Serial.println();
                Serial.print(F("Error writing to sector "));
                failure = true;
                printDataNoSpace(currentSector);
                Serial.print(F(" on head "));
                printDataNoSpace(currentHead);
                Serial.println(F("!"));
                Serial.println(F("Status Interpretation:"));
                for(int k = 0; k < 3; k++){
                  for(int l = 0; l < 8; l++){
                    if(bitRead(driveStatus[k], l) == 1){
                      //statusGood = 0;
                      Serial.println(statusMessages[k][l]); //All status errors show up every time
                    }
                  }
                }
                Serial.println();
              }
              commandBufferTenMegDiag[0] = 0x06;
              commandBufferTenMegDiag[1] = 0x00;
              commandBufferTenMegDiag[2] = 0x00;
              commandBufferTenMegDiag[3] = 0x99;
              commandBufferTenMegDiag[4] = currentHead;
              commandBufferTenMegDiag[5] = currentSector;
              calcChecksum();
              tenMegDiagRead();
              if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
                Serial.println();
                Serial.println();
                failure = true;
                Serial.print(F("Error rereading sector "));
                printDataNoSpace(currentSector);
                Serial.print(F(" on head "));
                printDataNoSpace(currentHead);
                Serial.println(F("!"));
                Serial.println(F("Status Interpretation:"));
                for(int k = 0; k < 3; k++){
                  for(int l = 0; l < 8; l++){
                    if(bitRead(driveStatus[k], l) == 1){
                      //statusGood = 0;
                      Serial.println(statusMessages[k][l]); //All status errors show up every time
                    }
                  }
                }
                Serial.println();
              }
              for(int i = 0; i < 532; i++){
                if(blockData[i] != testPattern[i]){
                  Serial.print("Data mismatch when rereading sector ");
                  printDataNoSpace(currentSector);
                  Serial.print(" on head ");
                  printDataNoSpace(currentHead);
                  Serial.print(". Expected to see ");
                  printDataNoSpace(testPattern[i]);
                  Serial.print(" at offset ");
                  printDataNoSpace(i >> 8);
                  printDataNoSpace(i);
                  Serial.print(", but read back a ");
                  printDataNoSpace(blockData[i]);
                  Serial.println(" instead!");
                  failure = true;
                  break;
                }
              }
              //Serial.println();
            }
          }
          Serial.println();
        }
      }
      if(checkSectors == true and failure == false){
        Serial.println();
        Serial.println(F("Integrity check passed!"));
        Serial.println();
      }
      if(failure == true){
        Serial.println();
        Serial.print(F("Errors were encountered while validating the spare table sectors. The spare table initialization process cannot continue. To bypass this warning, run the command again without choosing to verify the integrity of the sectors."));
        Serial.println();
      }
      else{
        Serial.println("Initializing spare table...");
        commandBufferTenMegDiag[0] = 0x02;
        commandBufferTenMegDiag[1] = 0x05;
        calcChecksum();
        Serial.print("Command: ");
        for(int i = 0; i < 3; i++){
          printDataSpace(commandBufferTenMegDiag[i]);
        }
        bool readSuccess = tenMegDiagRead();
        if(readSuccess == 0){
          Serial.println();
          Serial.println();
          setLEDColor(1, 0, 0);
          Serial.println(F("WARNING: Errors were encountered during the init spare table operation. The spare table may not have been created successfully."));
          Serial.println();
        }
        else{
          Serial.println();
          Serial.println();
          setLEDColor(0, 1, 0);
          Serial.println(F("Spare table initialization complete!"));
        }
        Serial.print(F("Number of bad sectors within the spare table (cylinder 0099) found during spare table initialization: "));
        printDataNoSpace(driveStatus[3]);
        Serial.println();
        Serial.println();
        Serial.println(F("List of these bad sectors:"));
        Serial.println();
        printRawData();
        Serial.println();
        Serial.println(F("Data Interpretation:"));
        if(driveStatus[3] == 0){
          Serial.println(F("No bad sectors found!"));
        }
        else{
          for(int i = 0; i < driveStatus[3]; i++){
            Serial.print(F("Sector "));
            printDataNoSpace(blockData[i] & 0x0F);
            Serial.print(F(" on head "));
            printDataNoSpace(blockData[i] >> 4);
            Serial.println(F(" is bad."));
          }
        }
        Serial.println();
      }
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("A") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){ //RAM Test
      clearScreen();
      clearScreen();
      Serial.println("Testing the ProFile's RAM...");
      commandBufferTenMegDiag[0] = 0x02;
      commandBufferTenMegDiag[1] = 0x06;
      calcChecksum();
      Serial.print("Command: ");
      for(int i = 0; i < 3; i++){
        printDataSpace(commandBufferTenMegDiag[i]);
      }
      Serial.println();
      bool readSuccess = tenMegDiagRead(true, false);
      if(readSuccess == 0){
        Serial.println();
        setLEDColor(1, 0, 0);
        Serial.println(F("WARNING: Errors were encountered while testing the drive's RAM. The following pass/fail information may be incorrect."));
        Serial.println();
      }
      if(driveStatus[0] == 0){
        Serial.println();
        Serial.println(F("RAM test passed!"));
      }
      else{
        Serial.println();
        Serial.print(F("RAM test failed at address"));
        printDataNoSpace(blockData[0]);
        printDataNoSpace(blockData[1]);
        Serial.print(F(" with the test pattern "));
        printDataNoSpace(blockData[2]);
        Serial.print(F(", which was incorrectly read back as "));
        printDataNoSpace(blockData[3]);
        Serial.print(F("."));
        Serial.println();
        Serial.println();
        Serial.println(F("Raw error data:"));
        printRawData();
      }
      Serial.println();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("B") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){ //Read Header
      byte tenMegInterleaveTable[4][16] = {{0x00, 0x0D, 0x0A, 0x07, 0x04, 0x01, 0x0E, 0x0B, 0x08, 0x05, 0x02, 0x0F, 0x0C, 0x09, 0x06, 0x03},
                                           {0x03, 0x00, 0x0D, 0x0A, 0x07, 0x04, 0x01, 0x0E, 0x0B, 0x08, 0x05, 0x02, 0x0F, 0x0C, 0x09, 0x06},
                                           {0x06, 0x03, 0x00, 0x0D, 0x0A, 0x07, 0x04, 0x01, 0x0E, 0x0B, 0x08, 0x05, 0x02, 0x0F, 0x0C, 0x09},
                                           {0x09, 0x06, 0x03, 0x00, 0x0D, 0x0A, 0x07, 0x04, 0x01, 0x0E, 0x0B, 0x08, 0x05, 0x02, 0x0F, 0x0C}};
      clearScreen();
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.print(F("Please enter the cylinder, head, and sector that you want to read the header from in the format (CC)CCHHSS: "));
      while(1){
        if(readSerialValue(8) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the cylinder, head, and sector that you want to read the header from in the format (CC)CCHHSS: "));
        }
      }
      for(int i = 0; i < 4; i++){
        commandBufferTenMegDiag[i + 2] = serialBytes[i];
      }
      commandBufferTenMegDiag[0] = 0x06;
      commandBufferTenMegDiag[1] = 0x07;
      Serial.println();
      Serial.print(F("Reading the header from cylinder "));
      printDataNoSpace(commandBufferTenMegDiag[2]);
      printDataNoSpace(commandBufferTenMegDiag[3]);
      Serial.print(F(", head "));
      printDataNoSpace(commandBufferTenMegDiag[4]);
      Serial.print(F(", and sector "));
      printDataNoSpace(commandBufferTenMegDiag[5]);
      Serial.println(F("..."));
      commandBufferTenMegDiag[5] = 0;
      for(int i = 0; i < 16; i++){
        if(tenMegInterleaveTable[serialBytes[2]][i] == serialBytes[3]){
          commandBufferTenMegDiag[5] = i - 1;
          if(commandBufferTenMegDiag[5] > 15){
            commandBufferTenMegDiag[5] = 15;
          }
          break;
        }
      }
      calcChecksum();
      Serial.print(F("Command: "));
      for(int i = 0; i < 7; i++){
        printDataSpace(commandBufferTenMegDiag[i]);
      }
      Serial.println();
      Serial.println();
      bool readSuccess = tenMegDiagRead();
      if(readSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered while reading the header. The following data may be incorrect."));
        Serial.println();
        setLEDColor(1, 0, 0);
      }
      else{
        setLEDColor(0, 1, 0);
      }
      bool headerGood = true;
      Serial.print(F("Header Contents: "));
      for(int i = 0; i < 0x14; i++){
        printDataSpace(blockData[i]);
      }
      Serial.println();
      Serial.println();
      Serial.println(F("Header Analysis: "));
      Serial.print(F("Starting Fence: "));
      printDataSpace(blockData[0]);
      printDataSpace(blockData[1]);
      /*if((blockData[0] != 0xD2) or (blockData[1] != 0x00)){
        headerGood = false;
        Serial.print(F("- Fence is incorrect; should be D2 00!"));
      }*/
      Serial.println();
      Serial.print(F("Cylinder: "));
      printDataNoSpace(blockData[2]);
      printDataNoSpace(blockData[3]);
      if((blockData[2] != serialBytes[0]) or (blockData[3] != serialBytes[1])){
        headerGood = false;
        Serial.print(F(" - Cylinder number doesn't match!"));
      }
      Serial.println();
      Serial.print(F("Head: "));
      printDataNoSpace(blockData[4]);
      if(blockData[4] != serialBytes[2]){
        headerGood = false;
        Serial.print(F(" - Head number doesn't match!"));
      }
      Serial.println();
      Serial.print(F("Sector: "));
      printDataNoSpace(blockData[5]);
      if(blockData[5] != serialBytes[3]){
        headerGood = false;
        Serial.print(F(" - Sector number doesn't match!"));
      }
      Serial.println();
      Serial.print(F("/Cylinder: "));
      byte notHighCylinder = ~blockData[2];
      byte notLowCylinder = ~blockData[3];
      printDataNoSpace(blockData[6]);
      printDataNoSpace(blockData[7]);
      if(blockData[6] != notHighCylinder){
        headerGood = false;
        Serial.print(F(" - Inverted cylinder doesn't match regular cylinder number; should be "));
        printDataNoSpace(~blockData[2]);
        printDataNoSpace(~blockData[3]);
      }
      else if(blockData[7] != notLowCylinder){
        headerGood = false;
        Serial.print(F(" - Inverted cylinder doesn't match regular cylinder number; should be "));
        printDataNoSpace(~blockData[2]);
        printDataNoSpace(~blockData[3]);
      }
      Serial.println();
      Serial.print(F("/Head: "));
      byte notHead = ~blockData[4];
      printDataNoSpace(blockData[8]);
      if(notHead != blockData[8]){
        headerGood = false;
        Serial.print(F(" - Inverted head doesn't match regular head number; should be "));
        printDataNoSpace(~blockData[4]);
      }
      Serial.println();
      Serial.print(F("/Sector: "));
      byte notSector = ~blockData[5];
      printDataNoSpace(blockData[9]);
      if(notSector != blockData[9]){
        headerGood = false;
        Serial.print(F(" - Inverted sector doesn't match regular sector number; should be "));
        printDataNoSpace(~blockData[5]);
      }

      Serial.println();
      Serial.print(F("Gap: "));
      for(int i = 0; i < 6; i++){
        printDataSpace(blockData[i + 0x0A]);
      }
      bool goodGap = true;
      for(int i = 0; i < 6; i++){
        if(blockData[i + 0x0A] != 0x00){
          goodGap = false;
        }
      }
      if(goodGap == false){
        headerGood = false;
        Serial.print(F(" - Incorrect gap value - gap should be all zeros!"));
      }
      Serial.println();
      Serial.print(F("Ending Fence: "));
      printDataSpace(blockData[0x10]);
      printDataSpace(blockData[0x11]);
      printDataSpace(blockData[0x12]);
      printDataSpace(blockData[0x13]);
      /*if((blockData[0x10] != 0x31) or (blockData[0x11] != 0x30) or (blockData[0x12] != 0x4D) or (blockData[0x13] != 0x00)){
        headerGood = false;
        Serial.print(F("- Fence is incorrect; should be 31 30 4D 00!"));
      }*/
      Serial.println();
      if(headerGood == true){
        Serial.print(F("Header integrity looks good!"));
      }
      else{
        Serial.print(F("Header seems to be invalid!"));
      }
      Serial.println();
      Serial.println();
      printStatus();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("C") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){ //Erase Track(s)
      clearScreen();
      clearScreen();
      bool abort = false;
      bool eraseAllTracks = false;
      Serial.print(F("Do you want to just erase one track or all tracks on the disk (return for one track, 'a' for all tracks)? "));
      while(1){
        if(Serial.available()) {
          delay(50);
          userInput = Serial.read();
          flushInput();
          if(userInput == 'a'){
            eraseAllTracks = true;
            break;
          }
          else if(userInput == '\r'){
            break;
          }
          else{
            while(Serial.available()){
              Serial.read();
            }
            Serial.print(F("Do you want to just erase one track or all tracks on the disk (return for one track, 'a' for all tracks)? "));
          }
        }
      }
      Serial.println();
      if(eraseAllTracks == false){
        Serial.print(F("Please enter the cylinder and head that you want to erase in the format (CC)CCHH: "));
        while(1){
          if(readSerialValue(6) == true){
            break;
          }
          else{
            Serial.print(F("Please enter the cylinder and head that you want to erase in the format (CC)CCHH: "));
          }
        }
        for(int i = 0; i < 3; i++){
          commandBufferTenMegDiag[i + 2] = serialBytes[i];
        }
        commandBufferTenMegDiag[0] = 0x05;
        commandBufferTenMegDiag[1] = 0x08;
        calcChecksum();
        Serial.print(F("Install jumper and press return to continue..."));
        flushInput();
        while(!Serial.available());
        flushInput();
        Serial.println();
        Serial.print("Erasing cylinder ");
        printDataNoSpace(commandBufferTenMegDiag[2]);
        printDataNoSpace(commandBufferTenMegDiag[3]);
        Serial.print(" on head ");
        printDataNoSpace(commandBufferTenMegDiag[4]);
        Serial.println("...");
        Serial.print("Command: ");
        for(int i = 0; i < 6; i++){
          printDataSpace(commandBufferTenMegDiag[i]);
        }
        Serial.println();
        setLEDColor(0, 0, 0);
        bool readSuccess = tenMegDiagRead(false);
        if(readSuccess == 0){
          setLEDColor(1, 0, 0);
          Serial.println();
          Serial.println(F("WARNING: Errors were encountered while erasing the track. The erase may not have completed successfully. You can remove the jumper now."));
          Serial.println();
        }
        else{
          Serial.println();
          setLEDColor(0, 1, 0);
          Serial.println(F("Erase complete! You can remove the jumper now."));
        }
        Serial.println();
        printStatus();
      }
      else{
        confirm();
        if(confirmOperation == true){
          Serial.print(F("Install jumper and press return to continue..."));
          flushInput();
          while(!Serial.available());
          flushInput();
          Serial.println();
          for(uint16_t currentCylinder = 0x00; currentCylinder < 0x0132; currentCylinder++){
            if(Serial.available()){
              abort = true;
              break;
            }
            for(byte currentHead = 0x00; currentHead < 0x04; currentHead++){
              Serial.print("Now erasing cylinder ");
              printDataNoSpace(currentCylinder >> 8);
              printDataNoSpace(currentCylinder);
              Serial.print(" of 0131 on head ");
              printDataNoSpace(currentHead);
              Serial.print(" of 03. Progress: ");
              Serial.print(((float)currentCylinder/0x0131)*100);
              Serial.print(F("%"));
              Serial.write("\033[1000D");
              commandBufferTenMegDiag[0] = 0x05;
              commandBufferTenMegDiag[1] = 0x08;
              commandBufferTenMegDiag[2] = currentCylinder >> 8;
              commandBufferTenMegDiag[3] = currentCylinder;
              commandBufferTenMegDiag[4] = currentHead;
              calcChecksum();
              tenMegDiagRead(false);
              if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
                Serial.println();
                Serial.println();
                Serial.print(F("Error erasing cylinder "));
                printDataNoSpace(currentCylinder >> 8);
                printDataNoSpace(currentCylinder);
                Serial.print(F(" on head "));
                printDataNoSpace(currentHead);
                Serial.println(F("!"));
                Serial.println(F("Status Interpretation:"));
                for(int k = 0; k < 3; k++){
                  for(int l = 0; l < 8; l++){
                    if(bitRead(driveStatus[k], l) == 1){
                      //statusGood = 0;
                      Serial.println(statusMessages[k][l]); //All status errors show up every time
                    }
                  }
                }
                Serial.println();
              }
            }
          }
          Serial.println();
          if(abort == false){
            Serial.println(F("Erase complete! You can remove the jumper now."));
          }
          else{
            Serial.println();
            Serial.println(F("Erase terminated by keypress. You can remove the jumper now."));
          }
          Serial.println();
        }
        else{
          Serial.println();
        }
      }
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }
/*
    else if(command.equalsIgnoreCase("D") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){ //Write Sector Mark(s)
      clearScreen();
      clearScreen();
      bool abort = false;
      bool writeAllTracks = false;
      Serial.print(F("Do you want to just write sector marks to one track or all tracks on the disk (return for one track, 'a' for all tracks)? "));
      while(1){
        if(Serial.available()) {
          delay(50);
          userInput = Serial.read();
          flushInput();
          if(userInput == 'a'){
            writeAllTracks = true;
            break;
          }
          else if(userInput == '\r'){
            break;
          }
          else{
            while(Serial.available()){
              Serial.read();
            }
            Serial.print(F("Do you want to just write sector marks to one track or all tracks on the disk (return for one track, 'a' for all tracks)? "));
          }
        }
      }
      Serial.println();
      if(writeAllTracks == false){
        Serial.print(F("Please enter the cylinder and head that you want to write the sector marks to in the format (CC)CCHH: "));
        while(1){
          if(readSerialValue(6) == true){
            break;
          }
          else{
            Serial.print(F("Please enter the cylinder and head that you want to write the sector marks to in the format (CC)CCHH: "));
          }
        }
        for(int i = 0; i < 3; i++){
          commandBufferTenMegDiag[i + 2] = serialBytes[i];
        }
        commandBufferTenMegDiag[0] = 0x07;
        commandBufferTenMegDiag[1] = 0x09;
        commandBufferTenMegDiag[5] = 0x00;
        commandBufferTenMegDiag[6] = 16;
        calcChecksum();
        Serial.print(F("Install jumper and press return to continue..."));
        flushInput();
        while(!Serial.available());
        flushInput();
        Serial.println();
        Serial.print("Writing sector marks to cylinder ");
        printDataNoSpace(commandBufferTenMegDiag[2]);
        printDataNoSpace(commandBufferTenMegDiag[3]);
        Serial.print(" on head ");
        printDataNoSpace(commandBufferTenMegDiag[4]);
        Serial.println("...");
        Serial.print("Command: ");
        for(int i = 0; i < 8; i++){
          printDataSpace(commandBufferTenMegDiag[i]);
        }
        Serial.println();
        setLEDColor(0, 0, 0);
        bool readSuccess = tenMegDiagRead(false);
        if(readSuccess == 0){
          setLEDColor(1, 0, 0);
          Serial.println();
          Serial.println(F("WARNING: Errors were encountered while writing sector marks to the track. The operation may not have completed successfully. You can remove the jumper now."));
          Serial.println();
        }
        else{
          Serial.println();
          setLEDColor(0, 1, 0);
          Serial.println(F("Writing sector marks complete! You can remove the jumper now."));
        }
        Serial.println();
        printStatus();
      }
      else{
        confirm();
        if(confirmOperation == true){
          Serial.print(F("Install jumper and press return to continue..."));
          flushInput();
          while(!Serial.available());
          flushInput();
          Serial.println();
          for(uint16_t currentCylinder = 0x00; currentCylinder < 0x0132; currentCylinder++){
            if(Serial.available()){
              abort = true;
              break;
            }
            for(byte currentHead = 0x00; currentHead < 0x04; currentHead++){
              Serial.print("Now writing sector marks to cylinder ");
              printDataNoSpace(currentCylinder >> 8);
              printDataNoSpace(currentCylinder);
              Serial.print(" of 0131 on head ");
              printDataNoSpace(currentHead);
              Serial.print(" of 03. Progress: ");
              Serial.print(((float)currentCylinder/0x0131)*100);
              Serial.print(F("%"));
              Serial.write("\033[1000D");
              commandBufferTenMegDiag[0] = 0x07;
              commandBufferTenMegDiag[1] = 0x09;
              commandBufferTenMegDiag[2] = currentCylinder >> 8;
              commandBufferTenMegDiag[3] = currentCylinder;
              commandBufferTenMegDiag[4] = currentHead;
              commandBufferTenMegDiag[5] = 0x00;
              commandBufferTenMegDiag[6] = 16;
              calcChecksum();
              tenMegDiagRead(false);
              if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
                Serial.println();
                Serial.println();
                Serial.print(F("Error writing sector marks to cylinder "));
                printDataNoSpace(currentCylinder >> 8);
                printDataNoSpace(currentCylinder);
                Serial.print(F(" on head "));
                printDataNoSpace(currentHead);
                Serial.println(F("!"));
                Serial.println(F("Status Interpretation:"));
                for(int k = 0; k < 3; k++){
                  for(int l = 0; l < 8; l++){
                    if(bitRead(driveStatus[k], l) == 1){
                      //statusGood = 0;
                      Serial.println(statusMessages[k][l]); //All status errors show up every time
                    }
                  }
                }
                Serial.println();
              }
            }
          }
          Serial.println();
          if(abort == false){
            Serial.println(F("Writing sector marks complete! You can remove the jumper now."));
          }
          else{
            Serial.println();
            Serial.println(F("Writing sector marks terminated by keypress. You can remove the jumper now."));
          }
        }
      }
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("E") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){ //Write Header(s)
      clearScreen();
      clearScreen();
      bool abort = false;
      bool writeAllTracks = false;
      Serial.print(F("Do you want to just write headers to one track or all tracks on the disk (return for one track, 'a' for all tracks)? "));
      while(1){
        if(Serial.available()) {
          delay(50);
          userInput = Serial.read();
          flushInput();
          if(userInput == 'a'){
            writeAllTracks = true;
            break;
          }
          else if(userInput == '\r'){
            break;
          }
          else{
            while(Serial.available()){
              Serial.read();
            }
            Serial.print(F("Do you want to just write headers to one track or all tracks on the disk (return for one track, 'a' for all tracks)? "));
          }
        }
      }
      Serial.println();
      if(writeAllTracks == false){
        Serial.print(F("Please enter the cylinder and head that you want to write the headers to in the format (CC)CCHH: "));
        while(1){
          if(readSerialValue(6) == true){
            break;
          }
          else{
            Serial.print(F("Please enter the cylinder and head that you want to write the headers to in the format (CC)CCHH: "));
          }
        }
        for(int i = 0; i < 3; i++){
          commandBufferTenMegDiag[i + 2] = serialBytes[i];
        }
        commandBufferTenMegDiag[0] = 0x07;
        commandBufferTenMegDiag[1] = 0x0A;
        commandBufferTenMegDiag[5] = 0x00;
        commandBufferTenMegDiag[6] = 16;
        calcChecksum();
        Serial.print(F("Install jumper and press return to continue..."));
        flushInput();
        while(!Serial.available());
        flushInput();
        Serial.println();
        Serial.print("Writing headers to cylinder ");
        printDataNoSpace(commandBufferTenMegDiag[2]);
        printDataNoSpace(commandBufferTenMegDiag[3]);
        Serial.print(" on head ");
        printDataNoSpace(commandBufferTenMegDiag[4]);
        Serial.println("...");
        Serial.print("Command: ");
        for(int i = 0; i < 8; i++){
          printDataSpace(commandBufferTenMegDiag[i]);
        }
        Serial.println();
        setLEDColor(0, 0, 0);
        bool readSuccess = tenMegDiagRead(false);
        if(readSuccess == 0){
          setLEDColor(1, 0, 0);
          Serial.println();
          Serial.println(F("WARNING: Errors were encountered while writing headers to the track. The operation may not have completed successfully. You can remove the jumper now."));
          Serial.println();
        }
        else{
          Serial.println();
          setLEDColor(0, 1, 0);
          Serial.println(F("Writing headers complete! You can remove the jumper now."));
        }
        Serial.println();
        printStatus();
      }
      else{
        confirm();
        if(confirmOperation == true){
          Serial.print(F("Install jumper and press return to continue..."));
          flushInput();
          while(!Serial.available());
          flushInput();
          Serial.println();
          for(uint16_t currentCylinder = 0x00; currentCylinder < 0x0132; currentCylinder++){
            if(Serial.available()){
              abort = true;
              break;
            }
            for(byte currentHead = 0x00; currentHead < 0x04; currentHead++){
              Serial.print("Now writing headers to cylinder ");
              printDataNoSpace(currentCylinder >> 8);
              printDataNoSpace(currentCylinder);
              Serial.print(" of 0131 on head ");
              printDataNoSpace(currentHead);
              Serial.print(" of 03. Progress: ");
              Serial.print(((float)currentCylinder/0x0131)*100);
              Serial.print(F("%"));
              Serial.write("\033[1000D");
              commandBufferTenMegDiag[0] = 0x07;
              commandBufferTenMegDiag[1] = 0x0A;
              commandBufferTenMegDiag[2] = currentCylinder >> 8;
              commandBufferTenMegDiag[3] = currentCylinder;
              commandBufferTenMegDiag[4] = currentHead;
              commandBufferTenMegDiag[5] = 0x00;
              commandBufferTenMegDiag[6] = 16;
              calcChecksum();
              tenMegDiagRead(false);
              if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0){ //Make it so that we go back up to the progress line after printing an error
                Serial.println();
                Serial.println();
                Serial.print(F("Error writing headers to cylinder "));
                printDataNoSpace(currentCylinder >> 8);
                printDataNoSpace(currentCylinder);
                Serial.print(F(" on head "));
                printDataNoSpace(currentHead);
                Serial.println(F("!"));
                Serial.println(F("Status Interpretation:"));
                for(int k = 0; k < 3; k++){
                  for(int l = 0; l < 8; l++){
                    if(bitRead(driveStatus[k], l) == 1){
                      //statusGood = 0;
                      Serial.println(statusMessages[k][l]); //All status errors show up every time
                    }
                  }
                }
                Serial.println();
              }
            }
          }
          Serial.println();
          if(abort == false){
            Serial.println(F("Writing headers complete! You can remove the jumper now."));
          }
          else{
            Serial.println();
            Serial.println(F("Writing headers terminated by keypress. You can remove the jumper now."));
          }
        }
      }
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }
*/
    else if(command.equalsIgnoreCase("D") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){ //Get Result Table
      clearScreen();
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.println(F("Reading the result table from the most recently-executed command..."));
      commandBufferTenMegDiag[0] = 0x02;
      commandBufferTenMegDiag[1] = 0x0D;
      calcChecksum();
      Serial.print(F("Command: "));
      for(int i = 0; i < 3; i++){
        printDataSpace(commandBufferTenMegDiag[i]);
      }
      Serial.println();
      Serial.println();
      bool readSuccess = tenMegDiagRead();
      if(readSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered while retrieving the result table. The table may be incorrect."));
        Serial.println();
        setLEDColor(1, 0, 0);
      }
      else{
        setLEDColor(0, 1, 0);
      }
      printRawData();
      Serial.println();
      printStatus();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("E") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){ //Disable Stepper
      clearScreen();
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.println(F("Disabling the head stepper motor..."));
      commandBufferTenMegDiag[0] = 0x02;
      commandBufferTenMegDiag[1] = 0x0B;
      calcChecksum();
      Serial.print(F("Command: "));
      for(int i = 0; i < 3; i++){
        printDataSpace(commandBufferTenMegDiag[i]);
      }
      Serial.println();
      Serial.println();
      bool readSuccess = tenMegDiagRead(false, true);
      if(readSuccess == 0){
        setLEDColor(1, 0, 0);
        Serial.println(F("WARNING: Errors were encountered while disabling the stepper motor. The motor might still be engaged."));
        Serial.println();
      }
      else{
        setLEDColor(0, 1, 0);
        Serial.println(F("Stepper has been disabled!"));
      }
      Serial.println();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("F") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){ //Park Heads
      clearScreen();
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.println(F("Parking the drive's heads..."));
      commandBufferTenMegDiag[0] = 0x06;
      commandBufferTenMegDiag[1] = 0x03;
      commandBufferTenMegDiag[2] = 0x00;
      commandBufferTenMegDiag[3] = 0x99;
      commandBufferTenMegDiag[4] = 0x00;
      commandBufferTenMegDiag[5] = 0x00;
      calcChecksum();
      tenMegDiagRead();
      
      commandBufferTenMegDiag[0] = 0x02;
      commandBufferTenMegDiag[1] = 0x0C;
      calcChecksum();
      Serial.print(F("Command: "));
      for(int i = 0; i < 3; i++){
        printDataSpace(commandBufferTenMegDiag[i]);
      }
      Serial.println();
      Serial.println();
      bool readSuccess = tenMegDiagRead(false, true);
      if(readSuccess == 0){
        setLEDColor(1, 0, 0);
        Serial.println(F("WARNING: Errors were encountered while parking the heads. The heads may not have been parked correctly."));
        Serial.println();
      }
      else{
        setLEDColor(0, 1, 0);
        Serial.println(F("Heads have been parked!"));
      }
      Serial.println();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("G") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){
      clearScreen();
      Serial.print(F("Please enter the 10MB diagnostic command that you want to send. Leave off the command length and checksum; they will be added automatically: "));
      char charInput[2128];
      byte hexInput[1064];
      unsigned int charIndex = 0;
      bool goodInput = true;
      while(1){
        if(Serial.available()){
          char inByte = Serial.read();
          bool inArray = false;
          if(inByte == '\r'){
            inArray = true;
          }
          for(int i = 0; i < 22; i++){
            if(inByte == acceptableHex[i]){
              inArray = true;
            }
          }
          if(inArray == false){
            goodInput = false;
          }
          if(inByte == '\r' and goodInput == true){
            charInput[charIndex] = '\0';
            hex2bin(hexInput, charInput, &charIndex);
            break;
          }
          else if(inByte == '\r' and goodInput == false){
            Serial.print(F("Please enter the 10MB diagnostic command that you want to send. Leave off the command length and checksum; they will be added automatically: "));
            charIndex = 0;
            goodInput = true;
          }
          else{
            charInput[charIndex] = inByte;
            if(charIndex < 2126){
              charIndex++;
            }
          }
        }
      }
      for(int i = 0; i < charIndex; i++){
        commandBufferTenMegDiag[i + 1] = hexInput[i];
      }
      commandBufferTenMegDiag[0] = charIndex + 1;
      calcChecksum();
      Serial.println();
      Serial.println(F("Read commands return data and the status bytes are read before the data is sent to the ArduinoFile."));
      Serial.println(F("Write commands don't return data and the status bytes are read after data has been written to the drive."));
      Serial.print(F("Is this a read command (r) or a write command (w)? "));
      while(1){
        if(Serial.available()) {
          delay(50);
          userInput = Serial.read();
          flushInput();
          if(userInput == 'w' or userInput == 'r'){
            break;
          }
          else{
            while(Serial.available()){
              Serial.read();
            }
            Serial.print(F("Is this a read command (r) or a write command (w)? "));
          }
        }
      }
      Serial.println();
      Serial.print(F("Executing command: "));
      for(int i = 0; i <= commandBufferTenMegDiag[0]; i++){
        printDataSpace(commandBufferTenMegDiag[i]);
      }
      setLEDColor(0, 0, 0);
      if(userInput == 'r'){
        Serial.println();
        Serial.println();
        bool readSuccess = tenMegDiagRead(true, false);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered during the operation. The following data may be incorrect."));
          Serial.println();
        }
        printRawData();
        Serial.println();
        printStatus();
      }
      if(userInput == 'w'){
        Serial.println();
        Serial.println();
        bool writeSuccess = tenMegDiagWrite(false);
        if(writeSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered during the operation."));
          Serial.println();
        }
        printStatus();
      }
      Serial.print(F("Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("K") and testMenu == false and diagMenu == false and diagMenuTenMeg == true and widgetMenu == false){ //Main Menu
      diagMenuTenMeg = false;
      clearScreen();
      flushInput();
    }

    if((command.equalsIgnoreCase("1") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true and widgetServoMenu == false) or (command.equalsIgnoreCase("1") and widgetServoMenu == true)){ //Soft Reset
      clearScreen();
      Serial.println(F("Soft-resetting the Widget controller..."));
      Serial.print(F("Command: "));
      commandBufferWidget[0] = 0x12;
      commandBufferWidget[1] = 0x07;
      calcWidgetChecksum();
      for(int i = 0; i < ((commandBufferWidget[0] & 0x0F) + 1); i++){
        printDataSpace(commandBufferWidget[i]);
      }
      Serial.println();
      Serial.println();
      commandResponseMatters = false;
      widgetRead(false, true);
      commandResponseMatters = true;
      commandBufferWidget[0] = 0x13;
      commandBufferWidget[1] = 0x01;
      commandBufferWidget[2] = 0x00;
      calcWidgetChecksum();
      bool readSuccess = widgetRead(false, true);
      if(readSuccess = true){
        Serial.println(F("Widget controller was reset!"));
      }
      else{
        Serial.println(F("Error: Unable to communicate with drive after reset!"));
      }
      Serial.println();
      printWidgetStatus();
      Serial.print(F("Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    if((command.equalsIgnoreCase("2") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true) or (command.equalsIgnoreCase("2") and widgetServoMenu == true)){ //Reset Servo
      clearScreen();
      Serial.println(F("Resetting the Widget servo board..."));
      Serial.print(F("Command: "));
      commandBufferWidget[0] = 0x12;
      commandBufferWidget[1] = 0x12;
      calcWidgetChecksum();
      for(int i = 0; i < ((commandBufferWidget[0] & 0x0F) + 1); i++){
        printDataSpace(commandBufferWidget[i]);
      }
      Serial.println();
      Serial.println();
      widgetRead(false, true);
      commandBufferWidget[0] = 0x13;
      commandBufferWidget[1] = 0x01;
      commandBufferWidget[2] = 0x05;
      calcWidgetChecksum();
      bool readSuccess = widgetRead(false, true);
      if(readSuccess == 0){
        Serial.println(F("Failed to send reset command!"));
      }
      else{
        Serial.println(F("Widget servo reset!"));
      }
      Serial.println();
      readWidgetStatus(1, 0);
      printWidgetStatus();
      Serial.print(F("Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("3") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true){ //Get Widget ID
      clearScreen();
      Serial.println(F("Reading Widget drive ID..."));
      commandBufferWidget[0] = 0x12;
      commandBufferWidget[1] = 0x00;
      calcWidgetChecksum();
      Serial.print(F("Command: "));
      for(int i = 0; i < ((commandBufferWidget[0] & 0x0F) + 1); i++){
        printDataSpace(commandBufferWidget[i]);
      }
      Serial.println();
      bool readSuccess = widgetRead();
      if(readSuccess == 0){
        Serial.println();
        Serial.println(F("WARNING: Errors were encountered while reading the drive ID. The following data may be incorrect."));
      }
      Serial.println();
      printRawData();
      Serial.println();
      printWidgetStatus();
      Serial.println(F("Data Analysis:"));
      Serial.print(F("Device Name: "));
      for(int i = 0; i < 13; i++){
        Serial.write(blockData[i]);
      }
      Serial.println();
      Serial.print(F("Device Number: "));
      for(int i = 13; i < 16; i++){
        Serial.print(blockData[i]>>4, HEX);
        Serial.print(blockData[i]&0x0F, HEX);
      }
      if(blockData[13] == 0x00 and blockData[14] == 0x00 and blockData[15] == 0x00){
        Serial.println(F(" (5MB ProFile) - NOT A WIDGET!"));
      }
      else if(blockData[13] == 0x00 and blockData[14] == 0x00 and blockData[15] == 0x10){
        Serial.println(F(" (10MB ProFile) - NOT A WIDGET!"));
      }
      else if(blockData[13] == 0x00 and blockData[14] == 0x01 and blockData[15] == 0x00){
        Serial.println(F(" (10MB Widget)"));
      }
      else if(blockData[13] == 0x00 and blockData[14] == 0x01 and blockData[15] == 0x10){
        Serial.println(F(" (20MB Widget)"));
      }
      else if(blockData[13] == 0x00 and blockData[14] == 0x01 and blockData[15] == 0x20){
        Serial.println(F(" (40MB Widget)"));
      }
      else{
        Serial.println(F(" (Unknown Drive Type) - PROBABLY NOT A WIDGET!"));
      }
      Serial.print(F("Firmware Revision: "));
      Serial.print(blockData[16], HEX);
      Serial.print(F("."));
      Serial.print(blockData[17]>>4, HEX);
      Serial.println(blockData[17]&0x0F, HEX);
      Serial.print(F("Total Blocks: "));
      for(int i = 18; i < 21; i++){
        Serial.print(blockData[i]>>4, HEX);
        Serial.print(blockData[i]&0x0F, HEX);
      }
      Serial.println();
      Serial.print(F("Bytes Per Block: "));
      for(int i = 21; i < 23; i++){
        Serial.print(blockData[i]>>4, HEX);
        Serial.print(blockData[i]&0x0F, HEX);
      }
      Serial.println();
      Serial.print(F("Number of Cylinders: "));
      printDataNoSpace(blockData[23]);
      printDataNoSpace(blockData[24]);
      Serial.println();
      Serial.print(F("Number of Heads: "));
      printDataNoSpace(blockData[25]);
      Serial.println();
      Serial.print(F("Sectors Per Track: "));
      printDataNoSpace(blockData[26]);
      Serial.println();
      Serial.print(F("Total Spares: "));
      printDataNoSpace(blockData[27]);
      printDataNoSpace(blockData[28]);
      printDataNoSpace(blockData[29]);
      Serial.println();
      Serial.print(F("Spares Allocated: "));
      printDataNoSpace(blockData[30]);
      printDataNoSpace(blockData[31]);
      printDataNoSpace(blockData[32]);
      Serial.println();
      Serial.print(F("Bad Blocks: "));
      printDataNoSpace(blockData[33]);
      printDataNoSpace(blockData[34]);
      printDataNoSpace(blockData[35]);
      Serial.println();
      Serial.println();
      Serial.print(F("Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("4") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true){ //Read Spare Table
      clearScreen();
      Serial.println(F("Reading the Widget's spare table..."));
      commandBufferWidget[0] = 0x12;
      commandBufferWidget[1] = 0x0D;
      calcWidgetChecksum();
      Serial.print(F("Command: "));
      for(int i = 0; i < ((commandBufferWidget[0] & 0x0F) + 1); i++){
        printDataSpace(commandBufferWidget[i]);
      }
      Serial.println();
      bool readSuccess = widgetRead();
      if(readSuccess == 0){
        Serial.println();
        Serial.println(F("WARNING: Errors were encountered while reading the spare table. The following data may be incorrect."));
      }
      Serial.println();
      printRawData();
      Serial.println();
      printWidgetStatus();
      Serial.println(F("Data Analysis:"));
      Serial.print(F("Starting Fence: "));
      bool correctFence = true;
      for(int i = 0; i < 4; i++){
        printDataSpace(blockData[i]);
        if(blockData[i] != fence[i]){
          correctFence = false;
        }
      }
      if(correctFence == true){
        Serial.println(F("- Fence looks good!"));
      }
      else{
        Serial.println(F("- Incorrect fence bytes!"));
      }
      Serial.print(F("Spare table has been modified "));
      for(int i = 4; i < 8; i++){
        printDataNoSpace(blockData[i]);
      }
      Serial.println(F(" times."));
      Serial.print(F("Format Offset: "));
      printDataNoSpace(blockData[8]);
      Serial.println();
      Serial.print(F("Interleave: "));
      printDataNoSpace(blockData[9]);
      Serial.println();
      Serial.print(F("Spares Allocated: "));
      printDataNoSpace(blockData[0x8A]);
      Serial.println();
      Serial.print(F("Bad Blocks: "));
      printDataNoSpace(blockData[0x8B]);
      Serial.println();
      Serial.println(F("Bitmap of Allocated Spares:"));
      Serial.print(F("    "));
      for(int i = 0x8C; i < 0x96; i++){
        printRawBinary(blockData[i]);
        Serial.print(F(" "));
      }
      Serial.println();
      bool firstTime = true;
      Serial.print(F("    This means that spares "));
      for(byte i = 1; i < 11; i++){
        for(byte j = 0; j < 8; j++){
          if(bitRead(blockData[0x8B + i], j) == 1){
            if(firstTime == true){
              firstTime = false;
            }
            else{
              Serial.print(F(", "));
            }
            printDataNoSpace(i * (0x07 - j));
            if((((0x8B + i) == 0x8F) and ((0x07 - j) == 0x01)) or (((0x8B + i) == 0x92) and ((0x07 - j) == 0x02))){
              Serial.print(F(" (probably spare table)"));
            }
          }
        }
      }
      Serial.print(F(" are currently in use."));
      Serial.println();
      Serial.println(F("Head Pointer Array:"));
      Serial.print(F("    "));
      for(int i = 0; i < 128; i++){
        if(((i % 32) == 0) and (i != 0)){
          Serial.println();
          Serial.print(F("    "));
        }
        printDataSpace(blockData[i + 0x0A]);
      }
      Serial.println();
      Serial.println(F("Linked Lists:"));
      Serial.print(F("    "));
      for(int i = 0; i < 304; i++){
        printDataNoSpace(blockData[i + 0x96]);
        if((((i + 1) % 32) == 0) and (i != 0)){
          Serial.println();
          Serial.print(F("   "));
        }
        if((((i + 1) % 4) == 0) and (i != 0)){
          Serial.print(F(" "));
        }
      }
      Serial.println();
      Serial.println(F("Interleave Map:"));
      Serial.print(F("    Block Number:    "));
      for(int i = 0; i < 19; i++){
        printDataSpace(i);
      }
      Serial.println();
      Serial.print(F("    Physical Sector: "));
      for(int i = 0x1C6; i < 0x1D9; i++){
        printDataSpace(blockData[i]);
      }
      Serial.println();
      Serial.print(F("Checksum: "));
      printDataNoSpace(blockData[0x1D9]);
      printDataNoSpace(blockData[0x1DA]);
      uint16_t correctChecksum = blockData[0x1D9] << 8;
      correctChecksum += blockData[0x1DA];
      uint16_t calculatedChecksum = 0;
      for(int i = 0; i < 0x1C6; i++){
        calculatedChecksum += blockData[i];
      }
      Serial.println();
      Serial.print(F("Ending Fence: "));
      correctFence = true;
      for(int i = 0; i < 4; i++){
        printDataSpace(blockData[i]);
        if(blockData[i] != fence[i]){
          correctFence = false;
        }
      }
      if(correctFence == true){
        Serial.println(F("- Fence looks good!"));
      }
      else{
        Serial.println(F("- Incorrect fence bytes!"));
      }
      Serial.println();
      Serial.print(F("Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    if((command.equalsIgnoreCase("5") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true) or (command.equalsIgnoreCase("3") and widgetServoMenu == true)){ //Get Controller Status
      clearScreen();
      Serial.println(F("Reading all eight Widget controller status longwords..."));
      Serial.println();

      readWidgetStatus(1, 0);
      Serial.print(F("Standard Status: "));
      for(int i = 0; i < 4; i++){
        printRawBinary(driveStatus[i]);
        Serial.print(F(" "));
      }

      Serial.println();
      Serial.println();
      Serial.print(F("    Byte 0:"));
      Serial.print(F("                                                     Byte 1:"));
      Serial.print(F("                                                     Byte 2:"));
      Serial.println(F("                                                     Byte 3:"));
      Serial.println();

      Serial.print(F("    Host didn't respond with 0x55: "));
      Serial.print(bitRead(driveStatus[0], 7));
      Serial.print(F("                            Spare table overflow: "));
      Serial.print(bitRead(driveStatus[1], 6));
      Serial.print(F("                                     First time status has been read since reset: "));
      Serial.print(bitRead(driveStatus[2], 7));
      Serial.print(F("              ECC detected a read error: "));
      Serial.println(bitRead(driveStatus[3], 7));

      Serial.print(F("    Write buffer overflow: "));
      Serial.print(bitRead(driveStatus[0], 6));
      Serial.print(F("                                    Five or fewer spares available: "));
      Serial.print(bitRead(driveStatus[1], 5));
      Serial.print(F("                           Last LBA was out of range: "));
      Serial.print(bitRead(driveStatus[2], 6));
      Serial.print(F("                                CRC detected a read error: "));
      Serial.println(bitRead(driveStatus[3], 6));

      Serial.print(F("    Read error: "));
      Serial.print(bitRead(driveStatus[0], 3));
      Serial.print(F("                                               Controller self-test failure: "));
      Serial.print(bitRead(driveStatus[1], 3));
      Serial.print(F("                                                                                         "));
      Serial.print(F("Header timeout on last read: "));
      Serial.println(bitRead(driveStatus[3], 5));

      Serial.print(F("    No matching header found: "));
      Serial.print(bitRead(driveStatus[0], 2));
      Serial.print(F("                                 Spare table has been updated: "));
      Serial.print(bitRead(driveStatus[1], 2));
      Serial.print(F("                                                                                         "));
      Serial.print(F("Unsuccessful retry count: "));
      byte badRetries = 0x00;
      for(int i = 3; i >= 0; i--){
        badRetries += bitRead(driveStatus[3], i) << i;
      }
      printDataNoSpace(badRetries);
      Serial.println();

      Serial.print(F("    Unrecoverable servo error: "));
      Serial.print(bitRead(driveStatus[0], 1));
      Serial.print(F("                                Seek to wrong track occurred: "));
      Serial.println(bitRead(driveStatus[1], 1));


      Serial.print(F("    Operation failed: "));
      Serial.print(bitRead(driveStatus[0], 0));
      Serial.print(F("                                         Controller aborted last operation: "));
      Serial.println(bitRead(driveStatus[1], 0));
      Serial.println();

      readWidgetStatus(1, 1);
      Serial.print(F("Last Logical Block: "));
      printDataNoSpace(driveStatus[1]);
      printDataNoSpace(driveStatus[2]);
      printDataNoSpace(driveStatus[3]);
      Serial.println();
      Serial.println();

      readWidgetStatus(1, 2);
      Serial.print(F("Current Seek Address (The CHS we want the heads to be on): Cylinder "));
      printDataNoSpace(driveStatus[0]);
      printDataNoSpace(driveStatus[1]);
      Serial.print(F(", head "));
      printDataNoSpace(driveStatus[2]);
      Serial.print(F(", and sector "));
      printDataNoSpace(driveStatus[3]);
      Serial.println(F("."));
      Serial.println();

      readWidgetStatus(1, 3);
      Serial.print(F("Current Cylinder (The actual cylinder the heads are on): "));
      printDataNoSpace(driveStatus[0]);
      printDataNoSpace(driveStatus[1]);
      Serial.println();
      Serial.println();

      readWidgetStatus(1, 4);
      Serial.print(F("Internal Status: "));
      for(int i = 0; i < 4; i++){
        printRawBinary(driveStatus[i]);
        Serial.print(F(" "));
      }
      
      Serial.println();
      Serial.println();
      Serial.print(F("    Byte 0:"));
      Serial.print(F("                                                     Byte 1:"));
      Serial.print(F("                                                     Byte 2:"));
      Serial.println(F("                                                     Byte 3:"));
      Serial.println();
      
      Serial.print(F("    Recovery is on: "));
      Serial.print(bitRead(driveStatus[0], 7));
      Serial.print(F("                                           Heads are on the right track: "));
      Serial.print(bitRead(driveStatus[1], 7));
      Serial.print(F("                             Seek was needed to move to the current block: "));
      Serial.println(bitRead(driveStatus[2], 7));

      Serial.print(F("    Spare table is almost full: "));
      Serial.print(bitRead(driveStatus[0], 6));
      Serial.print(F("                               Drive read a header after recal: "));
      Serial.print(bitRead(driveStatus[1], 6));
      Serial.print(F("                          Head change was needed to arrive at the current block: "));
      Serial.println(bitRead(driveStatus[2], 6));

      Serial.print(F("    Buffer structure is contaminated: "));
      Serial.print(bitRead(driveStatus[0], 5));
      Serial.print(F("                         Current operation is a write operation: "));
      Serial.print(bitRead(driveStatus[1], 5));
      Serial.print(F("                   Current block is a bad block: "));
      Serial.println(bitRead(driveStatus[3], 1));

      Serial.print(F("    Power reset has just occurred: "));
      Serial.print(bitRead(driveStatus[0], 4));
      Serial.print(F("                            Heads are parked: "));
      Serial.print(bitRead(driveStatus[1], 4));
      Serial.print(F("                                         Current block is a spare block: "));
      Serial.println(bitRead(driveStatus[1], 0));

      Serial.print(F("    Standard status is nonzero: "));
      Serial.print(bitRead(driveStatus[0], 3));
      Serial.print(F("                               Do sequential search of logical block look-ahead: "));
      Serial.println(bitRead(driveStatus[1], 3));

      Serial.print(F("    Controller LED is lit: "));
      Serial.print(bitRead(driveStatus[0], 0));
      Serial.print(F("                                    Last command was a multiblock command: "));
      Serial.println(bitRead(driveStatus[1], 2));

      Serial.print(F("                                                                "));
      Serial.print(F("Seek complete: "));
      Serial.println(bitRead(driveStatus[1], 1));

      Serial.print(F("                                                                "));
      Serial.print(F("Servo auto-offset is on: "));
      Serial.println(bitRead(driveStatus[1], 0));
      Serial.println();

      readWidgetStatus(1, 5);
      Serial.print(F("State Registers: "));
      for(int i = 0; i < 4; i++){
        printRawBinary(driveStatus[i]);
        Serial.print(F(" "));
      }

      Serial.println();
      Serial.println();
      Serial.print(F("    Byte 0:"));
      Serial.print(F("                                                     Byte 1:"));
      Serial.print(F("                                                     Byte 2:"));
      Serial.println(F("                                                     Byte 3:"));
      Serial.println();

      Serial.print(F("                                                                "));
      Serial.print(F("RAM failure: "));
      Serial.print(bitRead(driveStatus[1], 7));
      Serial.print(F("                                              Read/write direction set to read: "));
      Serial.print(bitRead(driveStatus[2], 7));
      Serial.print(F("                         /CRC error: "));
      Serial.println(bitRead(driveStatus[3], 7));

      Serial.print(F("                                                                "));
      Serial.print(F("EPROM failure: "));
      Serial.print(bitRead(driveStatus[1], 6));
      Serial.print(F("                                            Servo is able to accept a command: "));
      Serial.print(bitRead(driveStatus[2], 6));
      Serial.print(F("                        /Write not valid: "));
      Serial.println(bitRead(driveStatus[3], 6));
      
      Serial.print(F("                                                                "));
      Serial.print(F("Disk speed failure: "));
      Serial.print(bitRead(driveStatus[1], 5));
      Serial.print(F("                                       MSel1: "));
      Serial.print(bitRead(driveStatus[2], 5));
      Serial.print(F("                                                    Servo ready: "));
      Serial.println(bitRead(driveStatus[3], 5));

      Serial.print(F("                                                                "));
      Serial.print(F("Servo failure: "));
      Serial.print(bitRead(driveStatus[1], 4));
      Serial.print(F("                                            MSel0: "));
      Serial.print(bitRead(driveStatus[2], 4));
      Serial.print(F("                                                    Servo error: "));
      Serial.println(bitRead(driveStatus[3], 4));

      Serial.print(F("                                                                "));
      Serial.print(F("Sector count failure: "));
      Serial.print(bitRead(driveStatus[1], 3));
      Serial.print(F("                                     BSY: "));
      Serial.print(bitRead(driveStatus[2], 3));
      Serial.print(F("                                                      Controller state machine state: "));
      printDataNoSpace(driveStatus[3] & 0x0F);
      Serial.println();

      Serial.print(F("                                                                "));
      Serial.print(F("State machine failure: "));
      Serial.print(bitRead(driveStatus[1], 2));
      Serial.print(F("                                    CMD: "));
      Serial.println(bitRead(driveStatus[2], 2));

      Serial.print(F("                                                                "));
      Serial.print(F("Read/write failure: "));
      Serial.print(bitRead(driveStatus[1], 1));
      Serial.print(F("                                       ECC error: "));
      Serial.println(bitRead(driveStatus[2], 1));

      Serial.print(F("                                                                "));
      Serial.print(F("No spare table found: "));
      Serial.print(bitRead(driveStatus[1], 0));
      Serial.print(F("                                     State machine is running: "));
      Serial.println(bitRead(driveStatus[2], 0));
      Serial.println();

      readWidgetStatus(1, 6);
      Serial.print(F("Exception Registers: "));
      for(int i = 0; i < 4; i++){
        printRawBinary(driveStatus[i]);
        Serial.print(F(" "));
      }

      Serial.println();
      Serial.println();
      Serial.print(F("    Byte 0:"));
      Serial.print(F("                                                     Byte 1:"));
      Serial.print(F("                                                     Byte 2:"));
      Serial.println(F("                                                     Byte 3:"));
      Serial.println();

      Serial.print(F("    Read error occurred on last read attempt: "));
      Serial.print(bitRead(driveStatus[0], 7));
      Serial.print(F("                 Error detected by ECC circuitry: "));
      Serial.print(bitRead(driveStatus[1], 7));
      Serial.print(F("                          Write error occurred on last write attempt: "));
      Serial.print(bitRead(driveStatus[2], 7));
      Serial.print(F("               Number of bad retries during last write attempt: "));
      printDataNoSpace(driveStatus[3]);
      Serial.println();

      Serial.print(F("    Servo error while reading: "));
      Serial.print(bitRead(driveStatus[0], 6));
      Serial.print(F("                                Error detected by CRC circuitry: "));
      Serial.print(bitRead(driveStatus[1], 6));
      Serial.print(F("                          Servo error while writing: "));
      Serial.println(bitRead(driveStatus[2], 6));

      Serial.print(F("    At least one successful read in last read attempt: "));
      Serial.print(bitRead(driveStatus[0], 5));
      Serial.print(F("        Header timeout: "));
      Serial.print(bitRead(driveStatus[1], 5));
      Serial.print(F("                                           At least one successful write during last write attempt: "));
      Serial.println(bitRead(driveStatus[2], 5));

      Serial.print(F("    No matching header was found during last read attempt: "));
      Serial.print(bitRead(driveStatus[0], 4));
      Serial.print(F("    Number of bad retries during last read attempt: "));
      printDataNoSpace(driveStatus[1] & 0x0F);
      Serial.print(F("          No matching header found during last write attempt: "));
      Serial.println(bitRead(driveStatus[2], 4));

      Serial.print(F("    CRC or ECC error occurred during last read attempt: "));
      Serial.println(bitRead(driveStatus[0], 3));
      Serial.println();

      readWidgetStatus(1, 7);
      Serial.print(F("Last Seek Address (The CHS that we previously wanted the heads to be on): Cylinder "));
      printDataNoSpace(driveStatus[0]);
      printDataNoSpace(driveStatus[1]);
      Serial.print(F(", head "));
      printDataNoSpace(driveStatus[2]);
      Serial.print(F(", and sector "));
      printDataNoSpace(driveStatus[3]);
      Serial.println(F("."));
      Serial.println();
      readWidgetStatus(1, 0);
      printWidgetStatus();
      Serial.print(F("Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    if((command.equalsIgnoreCase("6") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true) or (command.equalsIgnoreCase("4") and widgetServoMenu == true)){ //Get Servo Status
      clearScreen();
      Serial.println(F("Reading all eight Widget servo status longwords..."));
      Serial.println();
      setLEDColor(0, 1, 0);
      byte servoStatus[32];
      for(int i = 8; i > 0; i--){
        readWidgetStatus(2, i);
        servoStatus[4*(i - 1)] = blockData[0];
        servoStatus[4*(i - 1) + 1] = blockData[1];
        servoStatus[4*(i - 1) + 2] = blockData[2];
        servoStatus[4*(i - 1) + 3] = blockData[3];
      }

      readWidgetStatus(2, 0);
      Serial.print(F("Standard Status: "));
      for(int i = 0; i < 4; i++){
        printRawBinary(driveStatus[i]);
        Serial.print(F(" "));
      }

      Serial.println();
      Serial.println();
      Serial.print(F("    Byte 0:"));
      Serial.print(F("                                                     Byte 1:"));
      Serial.print(F("                                                     Byte 2:"));
      Serial.println(F("                                                     Byte 3:"));
      Serial.println();

      Serial.print(F("    Host didn't respond with 0x55: "));
      Serial.print(bitRead(driveStatus[0], 7));
      Serial.print(F("                            Spare table overflow: "));
      Serial.print(bitRead(driveStatus[1], 6));
      Serial.print(F("                                     First time status has been read since reset: "));
      Serial.print(bitRead(driveStatus[2], 7));
      Serial.print(F("              ECC detected a read error: "));
      Serial.println(bitRead(driveStatus[3], 7));

      Serial.print(F("    Write buffer overflow: "));
      Serial.print(bitRead(driveStatus[0], 6));
      Serial.print(F("                                    Five or fewer spares available: "));
      Serial.print(bitRead(driveStatus[1], 5));
      Serial.print(F("                           Last LBA was out of range: "));
      Serial.print(bitRead(driveStatus[2], 6));
      Serial.print(F("                                CRC detected a read error: "));
      Serial.println(bitRead(driveStatus[3], 6));

      Serial.print(F("    Read error: "));
      Serial.print(bitRead(driveStatus[0], 3));
      Serial.print(F("                                               Controller self-test failure: "));
      Serial.print(bitRead(driveStatus[1], 3));
      Serial.print(F("                                                                                         "));
      Serial.print(F("Header timeout on last read: "));
      Serial.println(bitRead(driveStatus[3], 5));

      Serial.print(F("    No matching header found: "));
      Serial.print(bitRead(driveStatus[0], 2));
      Serial.print(F("                                 Spare table has been updated: "));
      Serial.print(bitRead(driveStatus[1], 2));
      Serial.print(F("                                                                                         "));
      Serial.print(F("Unsuccessful retry count: "));
      byte badRetries = 0x00;
      for(int i = 3; i >= 0; i--){
        badRetries += bitRead(driveStatus[3], i) << i;
      }
      printDataNoSpace(badRetries);
      Serial.println();

      Serial.print(F("    Unrecoverable servo error: "));
      Serial.print(bitRead(driveStatus[0], 1));
      Serial.print(F("                                Seek to wrong track occurred: "));
      Serial.println(bitRead(driveStatus[1], 1));


      Serial.print(F("    Operation failed: "));
      Serial.print(bitRead(driveStatus[0], 0));
      Serial.print(F("                                         Controller aborted last operation: "));
      Serial.println(bitRead(driveStatus[1], 0));
      Serial.println();


      Serial.print(F("Servo Status 01: "));
      for(int i = 0; i < 4; i++){
        printRawBinary(servoStatus[i]);
        Serial.print(F(" "));
      }
      Serial.println();
      Serial.println();
      Serial.print(F("    Byte 0:"));
      Serial.print(F("                                                     Byte 1:"));
      Serial.print(F("                                                     Byte 2:"));
      Serial.println(F("                                                     Byte 3:"));
      Serial.println();

      Serial.print(F("    Power amp off and heads are parked: "));
      Serial.print(bitRead(servoStatus[0], 0));
      Serial.print(F("                       Fine offset DAC value: "));
      if(bitRead(servoStatus[1], 5) == 0){
        Serial.print(F("-"));
        printDataNoSpace(servoStatus[1] & 0x1F);
      }
      else{
        printDataNoSpace(servoStatus[1] & 0x1F);
        Serial.print(F(" "));
      }
      Serial.print(F("                                  Op-amp at U2G is fast: "));
      Serial.print(bitRead(servoStatus[2], 0));
      Serial.print(F("                                    P3.2 IRQ: "));
      Serial.println(bitRead(servoStatus[3], 0));

      Serial.print(F("    HA-2405 multiplexer at U3E D0: "));
      Serial.print(bitRead(servoStatus[0], 1));
      Serial.print(F("                            HA-2405 multiplexer at U5D D0: "));
      Serial.print(bitRead(servoStatus[1], 6));
      Serial.print(F("                            On track window??: "));
      Serial.print(bitRead(servoStatus[2], 1));
      Serial.print(F("                                        P3.3 IRQ: "));
      Serial.println(bitRead(servoStatus[3], 1));

      Serial.print(F("    HA-2405 multiplexer at U3E D1: "));
      Serial.print(bitRead(servoStatus[0], 2));
      Serial.print(F("                            HA-2405 multiplexer at U5D D1: "));
      Serial.print(bitRead(servoStatus[1], 7));
      Serial.print(F("                            Position error is greater than DAC value: "));
      Serial.print(bitRead(servoStatus[2], 2));
      Serial.print(F("                 P3.1 IRQ: "));
      Serial.println(bitRead(servoStatus[3], 2));


      Serial.print(F("    HA-2405 multiplexer at U3E is enabled: "));
      Serial.print(bitRead(servoStatus[0], 3));
      Serial.print(F("                                                            "));
      Serial.print(F("                    Auto-zero the integrator at U1B: "));
      Serial.print(bitRead(servoStatus[2], 3));
      Serial.print(F("                          P3.0 IRQ: "));
      Serial.println(bitRead(servoStatus[3], 3));

      Serial.print(F("    L291 DAC offset strobe is off: "));
      Serial.print(bitRead(servoStatus[0], 4));
      Serial.print(F("                                                            "));
      Serial.print(F("                            In final track window?: "));
      Serial.print(bitRead(servoStatus[2], 4));
      Serial.print(F("                                   Timer 0 IRQ: "));
      Serial.println(bitRead(servoStatus[3], 4));

      Serial.print(F("    Recal mode selected: "));
      Serial.print(bitRead(servoStatus[0], 5));
      Serial.print(F("                                                            "));
      Serial.print(F("                                      Op-amp at U2H is fast: "));
      Serial.print(bitRead(servoStatus[2], 5));
      Serial.print(F("                                    Timer 1 IRQ: "));
      Serial.println(bitRead(servoStatus[3], 5));

      Serial.print(F("    Settling mode selected: "));
      Serial.print(bitRead(servoStatus[0], 6));
      Serial.print(F("                                                            "));
      Serial.print(F("                                   HA-2405 multiplexer at U3C D0: "));
      Serial.print(bitRead(servoStatus[2], 6));
      Serial.print(F("                            Bit 6 IRQ: "));
      Serial.println(bitRead(servoStatus[3], 6));

      Serial.print(F("    Access mode selected: "));
      Serial.print(bitRead(servoStatus[0], 7));
      Serial.print(F("                                                            "));
      Serial.print(F("                                     Odd/even is odd: "));
      Serial.print(bitRead(servoStatus[2], 7));
      Serial.print(F("                                          Bit 7 IRQ: "));
      Serial.println(bitRead(servoStatus[3], 7));
      Serial.println();


      Serial.print(F("Servo Status 02: "));
      for(int i = 4; i < 8; i++){
        printRawBinary(servoStatus[i]);
        Serial.print(F(" "));
      }
      Serial.println();
      Serial.println();
      Serial.print(F("    Byte 0:"));
      Serial.print(F("                                                     Byte 1:"));
      Serial.print(F("                                                     Byte 2:"));
      Serial.println(F("                                                     Byte 3:"));
      Serial.println();

      Serial.print(F("    Serial I/O register contents: "));
      printDataNoSpace(servoStatus[4]);
      Serial.print(F("                            Serial I/O transmit out: "));
      Serial.print(bitRead(servoStatus[5], 0));
      Serial.print(F("                                  Timer 0 load: "));
      Serial.print(bitRead(servoStatus[6], 0));
      Serial.print(F("                                             User flag F1: "));
      Serial.println(bitRead(servoStatus[7], 0));

      Serial.print(F("                                                                "));
      Serial.print(F("Servo error: "));
      Serial.print(bitRead(servoStatus[5], 1));
      Serial.print(F("                                              Timer 0 count enabled: "));
      Serial.print(bitRead(servoStatus[6], 1));
      Serial.print(F("                                    User flag F2: "));
      Serial.println(bitRead(servoStatus[7], 1));

      Serial.print(F("                                                                "));
      Serial.print(F("Servo ready: "));
      Serial.print(bitRead(servoStatus[5], 2));
      Serial.print(F("                                              Timer 1 load: "));
      Serial.print(bitRead(servoStatus[6], 2));
      Serial.print(F("                                             Half carry: "));
      Serial.println(bitRead(servoStatus[7], 2));

      Serial.print(F("                                                                "));
      Serial.print(F("Serial I/O ready: "));
      Serial.print(bitRead(servoStatus[5], 3));
      Serial.print(F("                                         Timer 1 count enabled: "));
      Serial.print(bitRead(servoStatus[6], 3));
      Serial.print(F("                                    Decimal adjust: "));
      Serial.println(bitRead(servoStatus[7], 3));

      Serial.print(F("                                                                "));
      Serial.print(F("Port 3.3: "));
      Serial.print(bitRead(servoStatus[5], 4));
      Serial.print(F("                                                            "));
      Serial.print(F("                                                 Overflow: "));
      Serial.println(bitRead(servoStatus[7], 4));

      Serial.print(F("                                                                "));
      Serial.print(F("Port 3.2: "));
      Serial.print(bitRead(servoStatus[5], 5));
      Serial.print(F("                                                            "));
      Serial.print(F("                                                 Sign: "));
      Serial.println(bitRead(servoStatus[7], 5));

      Serial.print(F("                                                                "));
      Serial.print(F("Port 3.1: "));
      Serial.print(bitRead(servoStatus[5], 6));
      Serial.print(F("                                                            "));
      Serial.print(F("                                                 Zero: "));
      Serial.println(bitRead(servoStatus[7], 6));

      Serial.print(F("                                                                "));
      Serial.print(F("Serial I/O receive in: "));
      Serial.print(bitRead(servoStatus[5], 7));
      Serial.print(F("                                                            "));
      Serial.print(F("                                    Carry: "));
      Serial.println(bitRead(servoStatus[7], 7));
      Serial.println();

      Serial.print(F("Servo Status 03: "));
      for(int i = 8; i < 12; i++){
        printRawBinary(servoStatus[i]);
        Serial.print(F(" "));
      }
      Serial.println();
      Serial.println();
      Serial.print(F("    Byte 0:"));
      Serial.print(F("                                                     Byte 1:"));
      Serial.print(F("                                                     Byte 2:"));
      Serial.println(F("                                                     Byte 3:"));
      Serial.println();

      Serial.print(F("    Timer 0 value: "));
      printDataNoSpace(servoStatus[8]);
      Serial.print(F("                                           Timer 1 value: "));
      printDataNoSpace(servoStatus[9]);
      Serial.print(F("                                           P3.2 interrupt mask: "));
      Serial.print(bitRead(servoStatus[10], 0));
      Serial.print(F("                                      Register pointer value: "));
      printDataNoSpace(servoStatus[11]);
      Serial.println();

      Serial.print(F("                                                                "));
      Serial.print(F("                                                            "));
      Serial.print(F("P3.3 interrupt mask: "));
      Serial.println(bitRead(servoStatus[10], 1));

      Serial.print(F("                                                                "));
      Serial.print(F("                                                            "));
      Serial.print(F("P3.1 interrupt mask: "));
      Serial.println(bitRead(servoStatus[10], 2));

      Serial.print(F("                                                                "));
      Serial.print(F("                                                            "));
      Serial.print(F("P3.0 interrupt mask: "));
      Serial.println(bitRead(servoStatus[10], 3));

      Serial.print(F("                                                                "));
      Serial.print(F("                                                            "));
      Serial.print(F("Timer 0 interrupt mask: "));
      Serial.println(bitRead(servoStatus[10], 4));

      Serial.print(F("                                                                "));
      Serial.print(F("                                                            "));
      Serial.print(F("Timer 1 interrupt mask: "));
      Serial.println(bitRead(servoStatus[10], 5));

      Serial.print(F("                                                                "));
      Serial.print(F("                                                            "));
      Serial.print(F("Bit 6 interrupt mask: "));
      Serial.println(bitRead(servoStatus[10], 6));

      Serial.print(F("                                                                "));
      Serial.print(F("                                                            "));
      Serial.print(F("Bit 7 interrupt mask: "));
      Serial.println(bitRead(servoStatus[10], 7));
      Serial.println();

      Serial.print(F("Servo Status 04: "));
      for(int i = 12; i < 16; i++){
        printRawBinary(servoStatus[i]);
        Serial.print(F(" "));
      }
      Serial.println();
      Serial.println();
      Serial.print(F("    Byte 0:"));
      Serial.print(F("                                                     Byte 1:"));
      Serial.print(F("                                                     Byte 2:"));
      Serial.println(F("                                                     Byte 3:"));
      Serial.println();

      Serial.print(F("    Most significant byte of stack pointer: "));
      printDataNoSpace(servoStatus[12]);
      Serial.print(F("                  Least significant byte of stack pointer: "));
      printDataNoSpace(servoStatus[13]);
      Serial.print(F("                 Upper four bits of servo command byte: "));
      Serial.print(bitRead(servoStatus[14], 3));
      Serial.print(bitRead(servoStatus[14], 2));
      Serial.print(bitRead(servoStatus[14], 1));
      Serial.print(bitRead(servoStatus[14], 0));
      Serial.print(F("                 Most significant byte of access timeout: "));
      printDataNoSpace(servoStatus[15]);
      Serial.println();
      Serial.println();
      Serial.print(F("                                     So the full stack pointer value is "));
      printDataNoSpace(servoStatus[12]);
      printDataNoSpace(servoStatus[13]);
      Serial.print(F("."));
      Serial.print(F("                                               "));
      Serial.print(F("Servo command from upper four bits of command byte: "));
      if((servoStatus[14] & 0x0F) == 0b00001000){
        Serial.println(F("Access"));
      }
      else if((servoStatus[14] & 0x0F) == 0b00001001){
        Serial.println(F("Access with offset"));
      }
      else if((servoStatus[14] & 0x0F) == 0b00000100){
        Serial.println(F("Data recal"));
      }
      else if((servoStatus[14] & 0x0F) == 0b00000111){
        Serial.println(F("Format recal"));
      }
      else if((servoStatus[14] & 0x0F) == 0b00000001){
        Serial.println(F("Offset"));
      }
      else if((servoStatus[14] & 0x0F) == 0b00000010){
        Serial.println(F("Diagnostic"));
      }
      else if((servoStatus[14] & 0x0F) == 0b00000000){
        Serial.println(F("Read status"));
      }
      else if((servoStatus[14] & 0x0F) == 0b00001100){
        Serial.println(F("Home"));
      }
      else{
        Serial.println(F("Unknown command"));
      }
      Serial.println();



      Serial.print(F("Servo Status 05: "));
      for(int i = 16; i < 20; i++){
        printRawBinary(servoStatus[i]);
        Serial.print(F(" "));
      }
      Serial.println();
      Serial.println();
      Serial.print(F("    Byte 0:"));
      Serial.print(F("                                                     Byte 1:"));
      Serial.print(F("                                                     Byte 2:"));
      Serial.println(F("                                                     Byte 3:"));
      Serial.println();

      Serial.print(F("    Scratch byte 0E: "));
      printDataNoSpace(servoStatus[16]);
      Serial.print(F("                                         Scratch byte 0C: "));
      printDataNoSpace(servoStatus[17]);
      Serial.print(F("                                         State machine fault state: "));
      printDataNoSpace(servoStatus[18]);
      Serial.print(F("                               Offset DAC value mask: "));
      if(bitRead(servoStatus[19], 5) == 0){
        Serial.print(F("-"));
        printDataNoSpace(servoStatus[19] & 0x1F);
      }
      else{
        printDataNoSpace(servoStatus[19] & 0x1F);
        Serial.print(F(" "));
      }
      Serial.println();

      Serial.print(F("                                                                "));
      Serial.print(F("                                                            "));
      Serial.print(F("                                                            "));
      Serial.print(F("HA-2405 multiplexer at U5D D0 mask: "));
      Serial.println(bitRead(servoStatus[19], 6));

      Serial.print(F("                                                                "));
      Serial.print(F("                                                            "));
      Serial.print(F("                                                            "));
      Serial.print(F("HA-2405 multiplexer at U5D D1 mask: "));
      Serial.println(bitRead(servoStatus[19], 7));
      Serial.println();

      Serial.print(F("Servo Status 06 (Last Command Received): "));
      for(int i = 20; i < 24; i++){
        printRawBinary(servoStatus[i]);
        Serial.print(F(" "));
      }
      Serial.println();
      Serial.println();
      Serial.println(F("    Command Interpretation:"));
      Serial.print(F("        Command Bits: "));
      Serial.print(bitRead(servoStatus[20], 7));
      Serial.print(bitRead(servoStatus[20], 6));
      Serial.print(bitRead(servoStatus[20], 5));
      Serial.print(bitRead(servoStatus[20], 4));
      if((servoStatus[20] & 0xF0) == 0b10000000){
        Serial.println(F(" - Access"));
      }
      else if((servoStatus[20] & 0xF0) == 0b10010000){
        Serial.println(F(" - Access with offset"));
      }
      else if((servoStatus[20] & 0xF0) == 0b01000000){
        Serial.println(F(" - Data recal"));
      }
      else if((servoStatus[20] & 0xF0) == 0b01110000){
        Serial.println(F(" - Format recal"));
      }
      else if((servoStatus[20] & 0xF0) == 0b00010000){
        Serial.println(F(" - Offset"));
      }
      else if((servoStatus[20] & 0xF0) == 0b00100000){
        Serial.println(F(" - Diagnostic"));
      }
      else if((servoStatus[20] & 0xF0) == 0b00000000){
        Serial.println(F(" - Read status"));
      }
      else if((servoStatus[20] & 0x0F) == 0b11000000){
        Serial.println(F(" - Home"));
      }
      else{
        Serial.println(F(" - Unknown command"));
      }

      Serial.print(F("        Access arguments: "));
      printDataNoSpace(servoStatus[20] & 0b00000111);
      printDataNoSpace(servoStatus[21]);
      if(((servoStatus[20] & 0b00000111) == 0) and servoStatus[21] == 0){
        Serial.println(F(" - Since all of these arguments are zero, they are not used for this command."));
      }
      else{
        Serial.print(F(" - Move the heads "));
        printDataNoSpace(servoStatus[20] & 0b00000011);
        printDataNoSpace(servoStatus[21]);
        Serial.print(F(" tracks "));
        if(bitRead(servoStatus[20], 2) == 1){
          Serial.print(F("towards"));
        }
        else{
          Serial.print(F("away from"));
        }
        Serial.println(F(" the spindle."));
      }
      Serial.print(F("        Offset arguments: "));
      printDataNoSpace(servoStatus[22]);
      if(servoStatus[30] == 0){
        Serial.print(F(" - Since all of these arguments are zero, they are not used for this command."));
      }
      else if(bitRead(servoStatus[22] , 6) == 1){
        Serial.print(F(" - Turn auto-offset on."));
      }
      else{
        Serial.print(F(" - Move the heads by a manual fine offset of "));
        printDataNoSpace(servoStatus[22] & 0b00011111);
        if(bitRead(servoStatus[22], 7) == 1){
          Serial.print(F(" towards"));
        }
        else{
          Serial.print(F(" away from"));
        }
        Serial.print(F(" the spindle with auto-offset off."));
      }
      if(bitRead(servoStatus[22], 5) == 1){
        Serial.print(F(" Then read the offset value from the DAC."));
      }
      Serial.println();
      Serial.print(F("        Status arguments: "));
      printDataNoSpace(servoStatus[23]);
      Serial.print(F(" - Serial link is operating at "));
      if(bitRead(servoStatus[23], 7) == 1){
        Serial.print(F("57600 baud, "));
      }
      else{
        Serial.print(F("19200 baud, "));
      }
      Serial.print(F("the power-on reset bit is "));
      if(bitRead(servoStatus[23], 6) == 1){
        Serial.print(F("active, "));
      }
      else{
        Serial.print(F("not active, "));
      }
      Serial.print(F("and the servo's status/diagnostic bits are "));
      Serial.print(bitRead(servoStatus[23], 3));
      Serial.print(bitRead(servoStatus[23], 2));
      Serial.print(bitRead(servoStatus[23], 1));
      Serial.print(bitRead(servoStatus[23], 0));
      Serial.println(F("."));
      Serial.println();


      Serial.print(F("Servo Status 07: "));
      for(int i = 24; i < 28; i++){
        printRawBinary(servoStatus[i]);
        Serial.print(F(" "));
      }
      Serial.println(F(" - This status longword contains the IRQ, P0, P3, and P1_Mask bytes, which we already printed in other status longwords, so there's no need to interpret them again here!"));
      Serial.println();



      Serial.print(F("Servo Status 08 (Last Command Processed): "));
      for(int i = 28; i < 32; i++){
        printRawBinary(servoStatus[i]);
        Serial.print(F(" "));
      }
      Serial.println();
      Serial.println();
      Serial.println(F("    Command Interpretation:"));
      Serial.print(F("        Command Bits: "));
      Serial.print(bitRead(servoStatus[28], 7));
      Serial.print(bitRead(servoStatus[28], 6));
      Serial.print(bitRead(servoStatus[28], 5));
      Serial.print(bitRead(servoStatus[28], 4));
      if((servoStatus[28] & 0xF0) == 0b10000000){
        Serial.println(F(" - Access"));
      }
      else if((servoStatus[28] & 0xF0) == 0b10010000){
        Serial.println(F(" - Access with offset"));
      }
      else if((servoStatus[28] & 0xF0) == 0b01000000){
        Serial.println(F(" - Data recal"));
      }
      else if((servoStatus[28] & 0xF0) == 0b01110000){
        Serial.println(F(" - Format recal"));
      }
      else if((servoStatus[28] & 0xF0) == 0b00010000){
        Serial.println(F(" - Offset"));
      }
      else if((servoStatus[28] & 0xF0) == 0b00100000){
        Serial.println(F(" - Diagnostic"));
      }
      else if((servoStatus[28] & 0xF0) == 0b00000000){
        Serial.println(F(" - Read status"));
      }
      else if((servoStatus[28] & 0x0F) == 0b11000000){
        Serial.println(F(" - Home"));
      }
      else{
        Serial.println(F(" - Unknown command"));
      }
      Serial.print(F("        Access arguments: "));
      printDataNoSpace(servoStatus[28] & 0b00000111);
      printDataNoSpace(servoStatus[29]);
      if(((servoStatus[28] & 0b00000111) == 0) and servoStatus[29] == 0){
        Serial.println(F(" - Since all of these arguments are zero, they are not used for this command."));
      }
      else{
        Serial.print(F(" - Move the heads "));
        printDataNoSpace(servoStatus[28] & 0b00000011);
        printDataNoSpace(servoStatus[29]);
        Serial.print(F(" tracks "));
        if(bitRead(servoStatus[28], 2) == 1){
          Serial.print(F("towards"));
        }
        else{
          Serial.print(F("away from"));
        }
        Serial.println(F(" the spindle."));
      }
      Serial.print(F("        Offset arguments: "));
      printDataNoSpace(servoStatus[30]);
      if(servoStatus[30] == 0){
        Serial.print(F(" - Since all of these arguments are zero, they are not used for this command."));
      }
      else if(bitRead(servoStatus[30] , 6) == 1){
        Serial.print(F(" - Turn auto-offset on."));
      }
      else{
        Serial.print(F(" - Move the heads by a manual fine offset of "));
        printDataNoSpace(servoStatus[30] & 0b00011111);
        if(bitRead(servoStatus[30], 7) == 1){
          Serial.print(F(" towards"));
        }
        else{
          Serial.print(F(" away from"));
        }
        Serial.print(F(" the spindle with auto-offset off."));
      }
      if(bitRead(servoStatus[30], 5) == 1){
        Serial.print(F(" Then read the offset value from the DAC."));
      }
      Serial.println();
      Serial.print(F("        Status arguments: "));
      printDataNoSpace(servoStatus[31]);
      Serial.print(F(" - Serial link is operating at "));
      if(bitRead(servoStatus[31], 7) == 1){
        Serial.print(F("57600 baud, "));
      }
      else{
        Serial.print(F("19200 baud, "));
      }
      Serial.print(F("the power-on reset bit is "));
      if(bitRead(servoStatus[31], 6) == 1){
        Serial.print(F("active, "));
      }
      else{
        Serial.print(F("not active, "));
      }
      Serial.print(F("and the servo's status/diagnostic bits are "));
      Serial.print(bitRead(servoStatus[31], 3));
      Serial.print(bitRead(servoStatus[31], 2));
      Serial.print(bitRead(servoStatus[31], 1));
      Serial.print(bitRead(servoStatus[31], 0));
      Serial.println(F("."));
      
      Serial.println();
      printWidgetStatus();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();

    }

    if((command.equalsIgnoreCase("7") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true) or (command.equalsIgnoreCase("5") and widgetServoMenu == true)){ //Get Abort Status
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.println(F("Note: The abort status is only meaningful if the controller has aborted an operation. If an abort hasn't happened, the information provided below might not mean anything."));
      Serial.println();
      Serial.println(F("Reading the Widget controller's abort status..."));
      Serial.print(F("Command: "));
      commandBufferWidget[0] = 0x12;
      commandBufferWidget[1] = 0x11;
      calcWidgetChecksum();
      for(int i = 0; i < 3; i++){
        printDataSpace(commandBufferWidget[i]);
      }
      bool readSuccess = widgetRead(true, true);
      if(readSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered while reading the abort status. The following abort information may be incorrect."));
        Serial.println();
      }
      Serial.println();
      Serial.println();
      Serial.print(F("Full abort status: "));
      for(int i = 0; i < 16; i++){
        printDataSpace(blockData[i]);
      }
      Serial.println();
      Serial.print(F("Return address of routine that triggered the abort: "));
      uint16_t returnAddress = blockData[14] << 8;
      returnAddress += blockData[15];
      printDataNoSpace(blockData[14]);
      printDataNoSpace(blockData[15]);
      Serial.println();
      Serial.print(F("Reason for the abort based on the return address: "));
      bool knownAbort = false;
      int abortIndex = 0;
      for(int i = 0; i < 36; i++){
        if(returnAddress == abortStatusAddresses[0][i]){
          Serial.println(abortStatusMessages[0][i]);
          knownAbort = true;
          abortIndex = i;
          break;
        }
      }
      if(knownAbort == false){
        Serial.println(F("Unknown (Maybe the previous command wasn't an abort or you're not using firmware revision 1A45!)"));
      }
      else{
        for(int i = 1; i < 3; i++){
          if(abortStatusAddresses[i][abortIndex] == 0xFF){
            break;
          }
          else{
            Serial.print(abortStatusMessages[i][abortIndex]);
            printDataNoSpace(blockData[abortStatusAddresses[i][abortIndex]]);
            Serial.println();
          }
        }
      }
      Serial.println();
      printWidgetStatus();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("8") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true){ //Diagnostic Read
      clearScreen();
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.print(F("Please enter the cylinder, head, and sector that you want to read from in the format (CC)CCHHSS or leave this value blank to read the current block: "));
      while(1){
        if(readSerialValue(8, true) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the cylinder, head, and sector that you want to read from in the format (CC)CCHHSS or leave this value blank to read the current block: "));
        }
      }
      if(serialBytes[8] == 0x55){
        commandBufferWidget[0] = 0x12;
        commandBufferWidget[1] = 0x09;
        calcWidgetChecksum();
        Serial.println();
        Serial.println(F("Reading from the block that the heads are positioned over..."));
        Serial.print(F("Command: "));
        for(int i = 0; i < 3; i++){
          printDataSpace(commandBufferWidget[i]);
        }
        Serial.println();
        Serial.println();
        bool readSuccess = widgetRead();
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered during the read operation. The following data may be incorrect."));
          Serial.println();
          setLEDColor(1, 0, 0);
        }
        else{
          setLEDColor(0, 1, 0);
        }
        printRawData();
        Serial.println();
        printWidgetStatus();
      }
      else{
        for(int i = 0; i < 4; i++){
          commandBufferWidget[i + 2] = serialBytes[i];
        }
        uint16_t cylinder = serialBytes[0] << 8;
        cylinder += serialBytes[1];
        byte head = serialBytes[2];
        byte sector = serialBytes[3];
        commandBufferWidget[0] = 0x16;
        commandBufferWidget[1] = 0x04;
        calcWidgetChecksum();
        Serial.println();
        Serial.print(F("Seeking to cylinder "));
        printDataNoSpace(commandBufferWidget[2]);
        printDataNoSpace(commandBufferWidget[3]);
        Serial.print(F(", head "));
        printDataNoSpace(commandBufferWidget[4]);
        Serial.print(F(", and sector "));
        printDataNoSpace(commandBufferWidget[5]);
        Serial.println(F("..."));
        Serial.print(F("Command: "));
        for(int i = 0; i < 7; i++){
          printDataSpace(commandBufferWidget[i]);
        }
        Serial.println();
        Serial.println();
        bool readSuccess = widgetRead(false, true);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered during the seek. The heads might not be in the desired location."));
          Serial.println();
          setLEDColor(1, 0, 0);
        }
        else{
          setLEDColor(0, 1, 0);
        }
        readWidgetStatus(1, 2);
        bool correctSeek = true;
        for(int i = 0; i < 4; i++){
          if(driveStatus[i] != serialBytes[i]){
            Serial.println();
            correctSeek = false;
            break;
          }
        }
        readWidgetStatus(1, 3);
        if(driveStatus[0] != serialBytes[0] or driveStatus[1] != serialBytes[1]){
          correctSeek = false;
        }
        readWidgetStatus(1, 4);
        if(bitRead(driveStatus[1], 1) != 1 or bitRead(driveStatus[1], 7) != 1){
          correctSeek = false;
        }
        readWidgetStatus(1, 0);
        if(bitRead(driveStatus[0], 1) != 0 or bitRead(driveStatus[1], 1) != 0){
          correctSeek = false;
        }
        if(correctSeek == true){
          Serial.print(F("Widget status confirms that the seek was successful!"));
        }
        else{
          Serial.print(F("Error: Widget status says that the seek failed!"));
        }
        Serial.println();
        Serial.println();
        Serial.print(F("Now reading from cylinder "));
        printDataNoSpace(serialBytes[0]);
        printDataNoSpace(serialBytes[1]);
        Serial.print(F(", head "));
        printDataNoSpace(serialBytes[2]);
        Serial.print(F(", and sector "));
        printDataNoSpace(serialBytes[3]);
        Serial.println(F("..."));
        commandBufferWidget[0] = 0x12;
        commandBufferWidget[1] = 0x09;
        calcWidgetChecksum();
        Serial.print(F("Command: "));
        for(int i = 0; i < 3; i++){
          printDataSpace(commandBufferWidget[i]);
        }
        Serial.println();
        readSuccess = widgetRead();
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered during the read operation. The following data may be incorrect."));
          Serial.println();
          setLEDColor(1, 0, 0);
        }
        else{
          setLEDColor(0, 1, 0);
        }
        Serial.println();
        printRawData();
        Serial.println();
        printWidgetStatus();
      }
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("9") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true){ //Diagnostic Write
      clearScreen();
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.print(F("Please enter the cylinder, head, and sector that you want to write the buffer to in the format (CC)CCHHSS or leave this value blank to write the current block: "));
      while(1){
        if(readSerialValue(8, true) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the cylinder, head, and sector that you want to write the buffer to in the format (CC)CCHHSS or leave this value blank to write the current block: "));
        }
      }
      if(serialBytes[8] == 0x55){
        commandBufferWidget[0] = 0x12;
        commandBufferWidget[1] = 0x0B;
        calcWidgetChecksum();
        Serial.println();
        Serial.println(F("Writing the buffer to the block that the heads are positioned over..."));
        Serial.print(F("Command: "));
        for(int i = 0; i < 3; i++){
          printDataSpace(commandBufferWidget[i]);
        }
        Serial.println();
        bool readSuccess = widgetWrite();
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered during the write operation. The following data may have been written incorrectly."));
          Serial.println();
          setLEDColor(1, 0, 0);
        }
        else{
          setLEDColor(0, 1, 0);
        }
        Serial.println();
        printWidgetStatus();
      }
      else{
        for(int i = 0; i < 4; i++){
          commandBufferWidget[i + 2] = serialBytes[i];
        }
        commandBufferWidget[0] = 0x16;
        commandBufferWidget[1] = 0x04;
        calcWidgetChecksum();
        Serial.println();
        Serial.print(F("Seeking to cylinder "));
        printDataNoSpace(commandBufferWidget[2]);
        printDataNoSpace(commandBufferWidget[3]);
        Serial.print(F(", head "));
        printDataNoSpace(commandBufferWidget[4]);
        Serial.print(F(", and sector "));
        printDataNoSpace(commandBufferWidget[5]);
        Serial.println(F("..."));
        Serial.print(F("Command: "));
        for(int i = 0; i < 7; i++){
          printDataSpace(commandBufferWidget[i]);
        }
        Serial.println();
        Serial.println();
        bool readSuccess = widgetRead(false, true);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered during the seek. The heads might not be in the desired location."));
          Serial.println();
          setLEDColor(1, 0, 0);
        }
        else{
          setLEDColor(0, 1, 0);
        }
        readWidgetStatus(1, 2);
        bool correctSeek = true;
        for(int i = 0; i < 4; i++){
          if(driveStatus[i] != serialBytes[i]){
            Serial.println();
            correctSeek = false;
            break;
          }
        }
        readWidgetStatus(1, 3);
        if(driveStatus[0] != serialBytes[0] or driveStatus[1] != serialBytes[1]){
          correctSeek = false;
        }
        readWidgetStatus(1, 4);
        if(bitRead(driveStatus[1], 1) != 1 or bitRead(driveStatus[1], 7) != 1){
          correctSeek = false;
        }
        readWidgetStatus(1, 0);
        if(bitRead(driveStatus[0], 1) != 0 or bitRead(driveStatus[1], 1) != 0){
          correctSeek = false;
        }
        if(correctSeek == true){
          Serial.print(F("Widget status confirms that the seek was successful!"));
        }
        else{
          Serial.print(F("Error: Widget status says that the seek failed!"));
        }
        Serial.println();
        Serial.println();
        Serial.print(F("Now writing the buffer to cylinder "));
        printDataNoSpace(serialBytes[0]);
        printDataNoSpace(serialBytes[1]);
        Serial.print(F(", head "));
        printDataNoSpace(serialBytes[2]);
        Serial.print(F(", and sector "));
        printDataNoSpace(serialBytes[3]);
        Serial.println(F("..."));
        commandBufferWidget[0] = 0x12;
        commandBufferWidget[1] = 0x0B;
        calcWidgetChecksum();
        Serial.print(F("Command: "));
        for(int i = 0; i < 3; i++){
          printDataSpace(commandBufferWidget[i]);
        }
        Serial.println();
        readSuccess = widgetWrite();
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered during the write operation. The data may have been written incorrectly."));
          Serial.println();
          setLEDColor(1, 0, 0);
        }
        else{
          setLEDColor(0, 1, 0);
        }
        Serial.println();
        printWidgetStatus();
      }
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if((command.equalsIgnoreCase("A") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true and widgetServoMenu == false) or (command.equalsIgnoreCase("6") and widgetServoMenu == true)){ //Read Header
      clearScreen();
      setLEDColor(0, 1, 0);
      byte sector = 0x00;
      Serial.print(F("Please enter the cylinder, head, and sector that you want to read the header from in the format (CC)CCHHSS or leave it blank to stay on the current cylinder and head: "));
      while(1){
        if(readSerialValue(8, true) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the cylinder, head, and sector that you want to read the header from in the format (CC)CCHHSS or leave it blank to stay on the current cylinder and head: "));
        }
      }
      if(serialBytes[8] == 0x55){
        Serial.print(F("Please enter the sector number that you want to read the header from: "));
        while(1){
          if(readSerialValue(2) == true){
            break;
          }
          else{
            Serial.print(F("Please enter the sector number that you want to read the header from: "));
          }
        }
        sector = serialBytes[0];
      }
      else{
        for(int i = 0; i < 4; i++){
          commandBufferWidget[i + 2] = serialBytes[i];
        }
        commandBufferWidget[0] = 0x16;
        commandBufferWidget[1] = 0x04;
        calcWidgetChecksum();
        Serial.println();
        Serial.print(F("Seeking to cylinder "));
        printDataNoSpace(commandBufferWidget[2]);
        printDataNoSpace(commandBufferWidget[3]);
        Serial.print(F(", head "));
        printDataNoSpace(commandBufferWidget[4]);
        Serial.print(F(", and sector "));
        printDataNoSpace(commandBufferWidget[5]);
        Serial.println(F("..."));
        Serial.print(F("Command: "));
        for(int i = 0; i < 7; i++){
          printDataSpace(commandBufferWidget[i]);
        }
        sector = commandBufferWidget[5];
        Serial.println();
        Serial.println();
        bool readSuccess = widgetRead(false, true);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered during the seek. The heads might not be in the desired location."));
          Serial.println();
          setLEDColor(1, 0, 0);
        }
        else{
          setLEDColor(0, 1, 0);
        }
        readWidgetStatus(1, 2);
        bool correctSeek = true;
        for(int i = 0; i < 4; i++){
          if(driveStatus[i] != serialBytes[i]){
            Serial.println();
            correctSeek = false;
            break;
          }
        }
        readWidgetStatus(1, 3);
        if(driveStatus[0] != serialBytes[0] or driveStatus[1] != serialBytes[1]){
          correctSeek = false;
        }
        readWidgetStatus(1, 4);
        if(bitRead(driveStatus[1], 1) != 1 or bitRead(driveStatus[1], 7) != 1){
          correctSeek = false;
        }
        readWidgetStatus(1, 0);
        if(bitRead(driveStatus[0], 1) != 0 or bitRead(driveStatus[1], 1) != 0){
          correctSeek = false;
        }
        if(correctSeek == true){
          Serial.print(F("Widget status confirms that the seek was successful!"));
          Serial.println();
        }
        else{
          Serial.print(F("Error: Widget status says that the seek failed!"));
          Serial.println();
        }
      }
      
      Serial.println();
      Serial.print(F("Now reading the header for sector "));
      printDataNoSpace(sector);
      Serial.println(F("..."));
      uint16_t cylinder = serialBytes[0] << 8;
      cylinder += serialBytes[1];
      byte highCylinder = serialBytes[0];
      byte lowCylinder = serialBytes[1];
      byte head = serialBytes[2];
      commandBufferWidget[0] = 0x13;
      commandBufferWidget[1] = 0x0A;
      commandBufferWidget[2] = sector;
      calcWidgetChecksum();
      Serial.print(F("Command: "));
      for(int i = 0; i < 4; i++){
        printDataSpace(commandBufferWidget[i]);
      }
      Serial.println();
      bool readSuccess = widgetRead();
      if(readSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered during the read operation. The following header may be incorrect."));
        Serial.println();
        setLEDColor(1, 0, 0);
      }
      else{
        setLEDColor(0, 1, 0);
      }
      Serial.println();
      Serial.print(F("Header Contents: "));
      for(int i = 0; i < 13; i++){
        printDataSpace(blockData[i]);
      }
      bool headerGood = true;
      Serial.println();
      Serial.println();
      Serial.println(F("Header Analysis: "));
      Serial.print(F("Cylinder: "));
      printDataNoSpace(blockData[0]);
      printDataNoSpace(blockData[1]);
      Serial.println();
      Serial.print(F("Head: "));
      printDataNoSpace(blockData[2] >> 6);
      Serial.println();
      Serial.print(F("Sector: "));
      printDataNoSpace(blockData[2] & 0b00111111);
      Serial.println();

      Serial.print(F("/Cylinder: "));
      byte notHighCylinder = ~blockData[0];
      byte notLowCylinder = ~blockData[1];
      printDataNoSpace(blockData[3]);
      printDataNoSpace(blockData[4]);
      if(blockData[3] != notHighCylinder){
        headerGood = false;
        Serial.print(F(" - Inverted cylinder doesn't match regular cylinder number; should be "));
        printDataNoSpace(~blockData[0]);
        printDataNoSpace(~blockData[1]);
      }
      if(blockData[4] != notLowCylinder){
        headerGood = false;
        Serial.print(F(" - Inverted cylinder doesn't match regular cylinder number; should be "));
        printDataNoSpace(~blockData[0]);
        printDataNoSpace(~blockData[1]);
      }
      Serial.println();
      Serial.print(F("/Head: "));
      printDataNoSpace(blockData[5] >> 6);
      if((blockData[5] >> 6) != ((~(blockData[2] >> 6)) & 0b00000011)){
        headerGood = false;
        Serial.print(F(" - Inverted head doesn't match regular head number; should be "));
        printDataNoSpace((~(blockData[2] >> 6)) & 0b00000011);
      }
      Serial.println();
      Serial.print(F("/Sector: "));
      printDataNoSpace(blockData[5] & 0b00111111);
      if((blockData[5] & 0b00111111) != (~blockData[2] & 0b00111111)){
        headerGood = false;
        Serial.print(F(" - Inverted sector doesn't match regular sector number; should be "));
        printDataNoSpace(~blockData[2] & 0b00111111);
      }

      Serial.println();
      Serial.print(F("Gap: "));
      for(int i = 0; i < 7; i++){
        printDataSpace(blockData[i + 6]);
      }
      bool goodGap = true;
      for(int i = 0; i < 7; i++){
        if(blockData[i + 6] != 0x00){
          goodGap = false;
        }
      }
      if(goodGap == false){
        headerGood = false;
      }
      Serial.println();
      if(headerGood == true){
        Serial.print(F("Header integrity looks good!"));
      }
      else{
        Serial.print(F("Header seems to be invalid!"));
      }
      Serial.println();
      Serial.println();
      printWidgetStatus();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("B") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true){ //Write Buffer to Spare Table
      clearScreen();
      Serial.print(F("WARNING: This command will overwrite the spare table with whatever is in the buffer. If the buffer doesn't contain a spare table, bad things could happen! Do you want to continue (return for yes, 'n' to cancel)? "));
      bool writeSpare = false;
      while(1){
        if(Serial.available()){
          delay(50);
          userInput = Serial.read();
          flushInput();
          if(userInput == 'n'){
            break;
            }
          else if(userInput == '\r'){
            writeSpare = true;
            break;
          }
          else{
            while(Serial.available()){
              Serial.read();
            }
            Serial.print(F("WARNING: This command will overwrite the spare table with whatever is in the buffer. If the buffer doesn't contain a spare table, bad things could happen! Do you want to continue (return for yes, 'n' to cancel)? "));
          }
        }
      }
      if(writeSpare == true){
        commandBufferWidget[0] = 0x16;
        commandBufferWidget[1] = 0x0E;
        commandBufferWidget[2] = 0xF0;
        commandBufferWidget[3] = 0x78;
        commandBufferWidget[4] = 0x3C;
        commandBufferWidget[5] = 0x1E;
        calcWidgetChecksum();
        Serial.println();
        Serial.println(F("Writing the buffer to both spare table copies on the Widget..."));
        Serial.print(F("Command: "));
        for(int i = 0; i < 7; i++){
          printDataSpace(commandBufferWidget[i]);
        }
        Serial.println();
        bool readSuccess = widgetWrite();
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered while writing the spare table. The table may not have been written correctly."));
          Serial.println();
          setLEDColor(1, 0, 0);
        }
        else{
          setLEDColor(0, 1, 0);
        }
        Serial.println();
        printWidgetStatus();
      }
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("C") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true){ //Seek
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.print(F("Please enter the cylinder, head, and sector that you want to seek to in the format (CC)CCHHSS: "));
      while(1){
        if(readSerialValue(8) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the cylinder, head, and sector that you want to seek to in the format (CC)CCHHSS: "));
        }
      }

      for(int i = 0; i < 4; i++){
        commandBufferWidget[i + 2] = serialBytes[i];
      }
      commandBufferWidget[0] = 0x16;
      commandBufferWidget[1] = 0x04;
      calcWidgetChecksum();
      Serial.println();
      Serial.print(F("Seeking to cylinder "));
      printDataNoSpace(commandBufferWidget[2]);
      printDataNoSpace(commandBufferWidget[3]);
      Serial.print(F(", head "));
      printDataNoSpace(commandBufferWidget[4]);
      Serial.print(F(", and sector "));
      printDataNoSpace(commandBufferWidget[5]);
      Serial.println(F("..."));
      Serial.print(F("Command: "));
      for(int i = 0; i < 7; i++){
        printDataSpace(commandBufferWidget[i]);
      }
      Serial.println();
      Serial.println();
      bool readSuccess = widgetRead(false, true);
      if(readSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered during the seek. The heads might not be in the desired location."));
        Serial.println();
        setLEDColor(1, 0, 0);
      }
      else{
        setLEDColor(0, 1, 0);
      }
      readWidgetStatus(1, 2);
      bool correctSeek = true;
      for(int i = 0; i < 4; i++){
        if(driveStatus[i] != serialBytes[i]){
          Serial.println();
          correctSeek = false;
          break;
        }
      }
      readWidgetStatus(1, 3);
      if(driveStatus[0] != serialBytes[0] or driveStatus[1] != serialBytes[1]){
        correctSeek = false;
      }
      readWidgetStatus(1, 4);
      if(bitRead(driveStatus[1], 1) != 1 or bitRead(driveStatus[1], 7) != 1){
        correctSeek = false;
      }
      readWidgetStatus(1, 0);
      if(bitRead(driveStatus[0], 1) != 0 or bitRead(driveStatus[1], 1) != 0){
        correctSeek = false;
      }
      if(correctSeek == true){
        Serial.print(F("Widget status confirms that the seek was successful!"));
      }
      else{
        Serial.print(F("Error: Widget status says that the seek failed!"));
      }
      Serial.println();
      setLEDColor(0, 1, 0);
      Serial.println();
      printWidgetStatus();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("D") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true){ //Send Servo Command
      widgetMenu = false;
      widgetServoMenu = true;
      clearScreen();
      flushInput();
    }

    else if(command.equalsIgnoreCase("E") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true){ //Send Restore
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.print(F("Do you want to send a data restore or a format restore (return for data, 'f' for format)? "));
      bool formatRestore = false;
      while(1){
        if(Serial.available()){
          delay(50);
          userInput = Serial.read();
          flushInput();
          if(userInput == 'f'){
            formatRestore = true;
            break;
          }
          else if(userInput == '\r'){
            break;
          }
          else{
            Serial.print(F("Do you want to send a data restore or a format restore (return for data, 'f' for format)? "));
            while(Serial.available()){
              Serial.read();
            }
          }
        }
      }
      Serial.println();
      if(formatRestore == false){
        Serial.println(F("Now performing a data restore on the Widget..."));
        commandBufferWidget[0] = 0x13;
        commandBufferWidget[1] = 0x05;
        commandBufferWidget[2] = 0x40;
        calcWidgetChecksum();
        Serial.print(F("Command: "));
        for(int i = 0; i < 4; i++){
          printDataSpace(commandBufferWidget[i]);
        }
      }

      else{
        Serial.println(F("Now performing a format restore on the Widget..."));
        commandBufferWidget[0] = 0x13;
        commandBufferWidget[1] = 0x05;
        commandBufferWidget[2] = 0x70;
        calcWidgetChecksum();
        Serial.print(F("Command: "));
        for(int i = 0; i < 4; i++){
          printDataSpace(commandBufferWidget[i]);
        }
      }

      Serial.println();
      Serial.println();
      bool readSuccess = widgetRead(false);
      if(readSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered while performing the restore. The restore might have failed."));
        Serial.println();
        setLEDColor(1, 0, 0);
      }
      else{
        setLEDColor(0, 1, 0);
      }
      Serial.print(F("Restore complete!"));
      Serial.println();
      setLEDColor(0, 1, 0);
      Serial.println();
      printWidgetStatus();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("F") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true){ //Set Recovery
      clearScreen();
      setLEDColor(0, 1, 0);
      readWidgetStatus(1, 4);
      if(bitRead(driveStatus[0], 7) == 1){
        Serial.print(F("Recovery is currently on. Do you want to turn it off (return for yes, 'n' for no)? "));
        bool proceed = false;
        while(1){
          if(Serial.available()){
            delay(50);
            userInput = Serial.read();
            flushInput();
            if(userInput == 'n'){
              proceed = false;
              break;
              }
            else if(userInput == '\r'){
              proceed = true;
              break;
            }
            else{
              Serial.print(F("Recovery is currently on. Do you want to turn it off (return for yes, 'n' for no)? "));
              while(Serial.available()){
                Serial.read();
              }
            }
          }
        }
        if(proceed == true){
          Serial.println();
          Serial.println(F("Turning recovery off..."));
          commandBufferWidget[0] = 0x13;
          commandBufferWidget[1] = 0x06;
          commandBufferWidget[2] = 0x00;
          calcWidgetChecksum();
          Serial.print(F("Command: "));
          for(int i = 0; i < 4; i++){
            printDataSpace(commandBufferWidget[i]);
          }
          bool readSuccess = widgetRead(false, true);
          if(readSuccess == 0){
            Serial.println(F("WARNING: Errors were encountered while turning off recovery. Recovery may still be on."));
            Serial.println();
          }
          Serial.println();
          Serial.println();
          readWidgetStatus(1, 4);
          if(bitRead(driveStatus[0], 7) == 0){
            Serial.println(F("Widget status says that recovery was disabled successfully!"));
          }
          else{
            Serial.println(F("Error: Widget status says that recovery is still on after disabling it!"));
          }
          Serial.println();
          readWidgetStatus(1, 0);
          printWidgetStatus();
        }
        else{
          Serial.println();
        }
      }
      else{
        Serial.print(F("Recovery is currently off. Do you want to turn it on (return for yes, 'n' for no)? "));
        bool proceed = false;
        while(1){
          if(Serial.available()){
            delay(50);
            userInput = Serial.read();
            flushInput();
            if(userInput == 'n'){
              proceed = false;
              break;
              }
            else if(userInput == '\r'){
              proceed = true;
              break;
            }
            else{
              Serial.print(F("Recovery is currently off. Do you want to turn it on (return for yes, 'n' for no)? "));
              while(Serial.available()){
                Serial.read();
              }
            }
          }
        }
        if(proceed == true){
          Serial.println();
          Serial.println(F("Turning recovery on..."));
          commandBufferWidget[0] = 0x13;
          commandBufferWidget[1] = 0x06;
          commandBufferWidget[2] = 0x01;
          calcWidgetChecksum();
          Serial.print(F("Command: "));
          for(int i = 0; i < 4; i++){
            printDataSpace(commandBufferWidget[i]);
          }
          Serial.println();
          Serial.println();
          bool readSuccess = widgetRead(false, true);
          if(readSuccess == 0){
            Serial.println(F("WARNING: Errors were encountered while turning on recovery. Recovery may still be off."));
            Serial.println();
          }
          readWidgetStatus(1, 4);
          if(bitRead(driveStatus[0], 7) == 1){
            Serial.println(F("Widget status says that recovery was enabled successfully!"));
          }
          else{
            Serial.println(F("Error: Widget status says that recovery is still off after enabling it!"));
          }
          Serial.println();
          readWidgetStatus(1, 0);
          printWidgetStatus();
        }
        else{
          Serial.println();
        }
      }
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("G") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true){ //Set AutoOffset
      clearScreen();
      setLEDColor(0, 1, 0);
      readWidgetStatus(1, 4);
      if(bitRead(driveStatus[1], 0) == 1){
        Serial.println(F("Widget status says that auto-offset is already on, so we'll just auto-offset the heads over the current track again..."));
      }
      else{
        Serial.println(F("Enabling auto-offset and auto-offseting the heads over the current track..."));
      }
      commandBufferWidget[0] = 0x12;
      commandBufferWidget[1] = 0x0C;
      calcWidgetChecksum();
      Serial.print(F("Command: "));
      for(int i = 0; i < 3; i++){
        printDataSpace(commandBufferWidget[i]);
      }
      bool readSuccess = widgetRead(false, true);
      if(readSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered while auto-offseting. The operation may not have succeeded."));
        Serial.println();
      }
      Serial.println();
      Serial.println();
      readWidgetStatus(1, 4);
      if(bitRead(driveStatus[1], 0) == 1){
        Serial.println(F("Auto-offset complete!"));
      }
      else{
        Serial.println(F("Error: Widget status says that the auto-offset is still disabled!"));
      }
      Serial.println();
      readWidgetStatus(1, 0);
      printWidgetStatus();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("H") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true){ //View Track Offsets
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.print(F("Please enter the cylinder and head on which you want to view the servo offset in the format CCCCHH or leave it blank to view offsets for all tracks: "));
      while(1){
        if(readSerialValue(6, true) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the cylinder and head on which you want to view the servo offset in the format CCCCHH or leave it blank to view offsets for all tracks: "));
        }
      }
      bool allTracks = false;
      if(serialBytes[6] == 0x55){
        allTracks = true;
      }
      if(allTracks == false){
        Serial.println();
        Serial.print(F("Seeking to cylinder "));
        printDataNoSpace(serialBytes[0]);
        printDataNoSpace(serialBytes[1]);
        Serial.print(F(" and head "));
        printDataNoSpace(serialBytes[2]);
        Serial.println(F("..."));
        commandBufferWidget[0] = 0x16;
        commandBufferWidget[1] = 0x04;
        commandBufferWidget[2] = serialBytes[0];
        commandBufferWidget[3] = serialBytes[1];
        commandBufferWidget[4] = serialBytes[2];
        commandBufferWidget[5] = 0x00;
        calcWidgetChecksum();
        Serial.print(F("Command: "));
        for(int i = 0; i < 7; i++){
          printDataSpace(commandBufferWidget[i]);
        }
        Serial.println();
        Serial.println();
        bool readSuccess = widgetRead(false, true);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered while seeking. The heads may not be positioned over the correct track."));
          Serial.println();
        }
        readWidgetStatus(1, 2);
        bool correctSeek = true;
        for(int i = 0; i < 3; i++){
          if(driveStatus[i] != serialBytes[i]){
            Serial.println();
            correctSeek = false;
            break;
          }
        }
        readWidgetStatus(1, 3);
        if(driveStatus[0] != serialBytes[0] or driveStatus[1] != serialBytes[1]){
          correctSeek = false;
        }
        readWidgetStatus(1, 4);
        if(bitRead(driveStatus[1], 1) != 1 or bitRead(driveStatus[1], 7) != 1){
          correctSeek = false;
        }
        readWidgetStatus(1, 0);
        if(bitRead(driveStatus[0], 1) != 0 or bitRead(driveStatus[1], 1) != 0){
          correctSeek = false;
        }
        if(correctSeek == true){
          Serial.print(F("Seek successful!"));
        }
        else{
          Serial.print(F("Error: Widget status says that the seek failed!"));
        }
        Serial.println();
        Serial.println();

        Serial.println(F("Auto-offsetting over the track three times to make sure we're centered well..."));
        commandBufferWidget[0] = 0x12;
        commandBufferWidget[1] = 0x0C;
        calcWidgetChecksum();
        Serial.print(F("Command: "));
        for(int i = 0; i < 3; i++){
          printDataSpace(commandBufferWidget[i]);
        }
        Serial.println();
        Serial.println();
        bool offsetWorked = true;
        readSuccess = widgetRead(false, true);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered on the first auto-offset. The operation may not have succeeded."));
          Serial.println();
        }
        readWidgetStatus(1, 4);
        if(bitRead(driveStatus[1], 0) == 0){
          Serial.println(F("Error: Widget status says that the auto-offset is still disabled on the first auto-offset!"));
          offsetWorked = false;
        }
        readSuccess = widgetRead(false, true);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered on the second auto-offset. The operation may not have succeeded."));
          Serial.println();
        }
        readWidgetStatus(1, 4);
        if(bitRead(driveStatus[1], 0) == 0){
          Serial.println(F("Error: Widget status says that the auto-offset is still disabled on the second auto-offset!"));
          offsetWorked = false;
        }
        readSuccess = widgetRead(false, true);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered on the third auto-offset. The operation may not have succeeded."));
          Serial.println();
        }
        readWidgetStatus(1, 4);
        if(bitRead(driveStatus[1], 0) == 0){
          Serial.println(F("Error: Widget status says that the auto-offset is still disabled on the third auto-offset!"));
          offsetWorked = false;
        }
        if(offsetWorked == true){
          Serial.println(F("Auto-offset complete!"));
        }
        else{
          Serial.println(F("Auto-offset failed!"));
        }
        Serial.println();
        Serial.println(F("Reading the servo offset..."));
        commandBufferWidget[0] = 0x13;
        commandBufferWidget[1] = 0x02;
        commandBufferWidget[2] = 0x01;
        calcWidgetChecksum();
        Serial.print(F("Command: "));
        for(int i = 0; i < 4; i++){
          printDataSpace(commandBufferWidget[i]);
        }
        Serial.println();
        Serial.println();
        readSuccess = widgetRead(true, true);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered while reading the track offset. The following offset information may be incorrect."));
          Serial.println();
        }
        Serial.print(F("Offset for cylinder "));
        printDataNoSpace(serialBytes[0]);
        printDataNoSpace(serialBytes[1]);
        Serial.print(F(" and head "));
        printDataNoSpace(serialBytes[2]);
        Serial.print(F(" is "));
        if(bitRead(blockData[1], 5) == 0){
          Serial.print(F("-"));
        }
        printDataNoSpace(blockData[1] & 0x1F);
        Serial.print(F("."));
        if((blockData[1] & 0x1F) >= 0x10){
          Serial.print(F(" This offset is 10 or larger, which is a bit concerning!"));
        }
        Serial.println();
        Serial.println();
        printWidgetStatus();
      }
      else{
        byte lowCylinder = 0x00;
        byte highCylinder = 0x00;
        Serial.println();
        bool abort = false;
        for(uint16_t cylinder = 0x00; cylinder < 0x0202; cylinder++){
          if(Serial.available()){
            abort = true;
            break;
          }
          for(byte head = 0x00; head < 0x02; head++){
            commandBufferWidget[0] = 0x16;
            commandBufferWidget[1] = 0x04;
            commandBufferWidget[2] = cylinder >> 8;
            commandBufferWidget[3] = cylinder;
            commandBufferWidget[4] = head;
            commandBufferWidget[5] = 0x00;
            highCylinder = commandBufferWidget[2];
            lowCylinder = commandBufferWidget[3];
            calcWidgetChecksum();
            bool readSuccess = widgetRead(false, true);
            if(readSuccess == 0){
              Serial.println(F("WARNING: Errors were encountered while seeking. The heads may not be positioned over the correct track."));
              Serial.println();
            }
            readWidgetStatus(1, 2);
            bool correctSeek = true;
            if((driveStatus[0] != highCylinder) or (driveStatus[1] != lowCylinder) or (driveStatus[2] != head) or (driveStatus[3] != 0)){
              correctSeek = false;
            }
            readWidgetStatus(1, 3);
            if((driveStatus[0] != highCylinder) or (driveStatus[1] != lowCylinder)){
              correctSeek = false;
            }
            readWidgetStatus(1, 4);
            if(bitRead(driveStatus[1], 1) != 1 or bitRead(driveStatus[1], 7) != 1){
              correctSeek = false;
            }
            readWidgetStatus(1, 0);
            if(bitRead(driveStatus[0], 1) != 0 or bitRead(driveStatus[1], 1) != 0){
              correctSeek = false;
            }
        
            if(correctSeek == false){
              Serial.println(F("Error: Widget status says that the seek failed!"));
            }

            commandBufferWidget[0] = 0x12;
            commandBufferWidget[1] = 0x0C;
            calcWidgetChecksum();
            readSuccess = widgetRead(false, true);
            if(readSuccess == 0){
              Serial.println(F("WARNING: Errors were encountered while auto-offseting. The operation may not have succeeded."));
              Serial.println();
            }
            readWidgetStatus(1, 4);
            if(bitRead(driveStatus[1], 0) == 0){
              Serial.println(F("Error: Widget status says that auto-offset is still disabled!"));
            }
            readSuccess = widgetRead(false, true);
            if(readSuccess == 0){
              Serial.println(F("WARNING: Errors were encountered while auto-offseting. The operation may not have succeeded."));
              Serial.println();
            }
            readWidgetStatus(1, 4);
            if(bitRead(driveStatus[1], 0) == 0){
              Serial.println(F("Error: Widget status says that auto-offset is still disabled!"));
            }
            readSuccess = widgetRead(false, true);
            if(readSuccess == 0){
              Serial.println(F("WARNING: Errors were encountered while auto-offseting. The operation may not have succeeded."));
              Serial.println();
            }
            readWidgetStatus(1, 4);
            if(bitRead(driveStatus[1], 0) == 0){
              Serial.println(F("Error: Widget status says that auto-offset is still disabled!"));
            }
            commandBufferWidget[0] = 0x13;
            commandBufferWidget[1] = 0x02;
            commandBufferWidget[2] = 0x01;
            calcWidgetChecksum();
            readSuccess = widgetRead(true, true);
            if(readSuccess == 0){
              Serial.println(F("WARNING: Errors were encountered while reading the track offset. The following offset information may be incorrect."));
              Serial.println();
            }
            Serial.print(F("Fine offset for cylinder "));
            printDataNoSpace(cylinder >> 8);
            printDataNoSpace(cylinder);
            Serial.print(F(" and head "));
            printDataNoSpace(head);
            Serial.print(F(" is "));
            if(bitRead(blockData[1], 5) == 0){
              Serial.print(F("-"));
            }
            printDataNoSpace(blockData[1] & 0x1F);
            Serial.print(F("."));
            if((blockData[1] & 0x1F) >= 0x10){
              Serial.print(F(" This offset is 10 or larger, which is a bit concerning!"));
            }
            Serial.println();
          }
        }
        if(abort == true){
          Serial.println();
          Serial.println(F("Scanning of track fine offsets terminated by keypress."));
        }
        Serial.println();
      }
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("I") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true){ //LLF
      clearScreen();
      confirm();
      if(confirmOperation == true){
        byte formatOffset = 0x00;
        byte interleave = 0x01;
        Serial.println();
        Serial.print(F("Please enter your desired 1-byte format offset or press return to use the default of 00: "));
        while(1){
          if(readSerialValue(2, true) == true){
            break;
          }
          else{
            Serial.print(F("Please enter your desired 1-byte format offset or press return to use the default of 00: "));
          }
        }
        if(serialBytes[2] != 0x55){
          formatOffset = serialBytes[0];
        }
        Serial.print(F("Please enter your desired 1-byte interleave value or press return to use the default of 01 (meaning 1:2 interleave): "));
        while(1){
          if(readSerialValue(2, true) == true){
            break;
          }
          else{
            Serial.print(F("Please enter your desired 1-byte interleave value or press return to use the default of 01 (meaning 1:2 interleave): "));
          }
        }
        if(serialBytes[2] != 0x55){
          interleave = serialBytes[0];
        }
        bool abort = false;
        while(abort == false){
          Serial.println();
          Serial.println(F("Step 1: Soft-resetting the Widget controller..."));
          Serial.print(F("Command: "));
          commandBufferWidget[0] = 0x12;
          commandBufferWidget[1] = 0x07;
          calcWidgetChecksum();
          for(int i = 0; i < ((commandBufferWidget[0] & 0x0F) + 1); i++){
            printDataSpace(commandBufferWidget[i]);
          }
          Serial.println();
          Serial.println();
          commandResponseMatters = false;
          widgetRead(false, true);
          commandResponseMatters = true;
          commandBufferWidget[0] = 0x13;
          commandBufferWidget[1] = 0x01;
          commandBufferWidget[2] = 0x00;
          calcWidgetChecksum();
          bool readSuccess = widgetRead(false, true);
          if(readSuccess = true){
            Serial.println(F("Soft-reset complete!"));
          }
          else{
            Serial.println(F("Error: Unable to communicate with drive after reset!"));
          }

          Serial.println();
          Serial.println(F("Step 2: Disabling recovery..."));
          commandBufferWidget[0] = 0x13;
          commandBufferWidget[1] = 0x06;
          commandBufferWidget[2] = 0x00;
          calcWidgetChecksum();
          Serial.print(F("Command: "));
          for(int i = 0; i < 4; i++){
            printDataSpace(commandBufferWidget[i]);
          }
          readSuccess = widgetRead(false, true);
          if(readSuccess == 0){
            Serial.println(F("WARNING: Errors were encountered while turning off recovery. Recovery may still be on."));
            Serial.println();
          }
          Serial.println();
          Serial.println();
          readWidgetStatus(1, 4);
          if(bitRead(driveStatus[0], 7) == 0){
            Serial.println(F("Widget status says that recovery was disabled successfully!"));
          }
          else{
            Serial.println(F("Error: Widget status says that recovery is still on after disabling it!"));
          }
          Serial.println();
          Serial.println(F("Step 3: Checking to see if Widget has passed all self-tests..."));
          Serial.println(F("Command: 13 01 05 DA"));
          readWidgetStatus(1, 5);
          Serial.println();
          if((driveStatus[1] != 0x00) or (driveStatus[2] != 0xDB) or (driveStatus[3] != 0xE0)){
            Serial.println(F("Error: Bad Widget state status! The format might fail!"));
          }
          else{
            Serial.println(F("Drive has passed all tests!"));
          }
          Serial.println();
          Serial.println(F("Step 4: Performing a format restore..."));
          commandBufferWidget[0] = 0x13;
          commandBufferWidget[1] = 0x05;
          commandBufferWidget[2] = 0x70;
          calcWidgetChecksum();
          Serial.print(F("Command: "));
          for(int i = 0; i < 4; i++){
            printDataSpace(commandBufferWidget[i]);
          }
          Serial.println();
          Serial.println();
          readSuccess = widgetRead(false, true);
          if(readSuccess == 0){
            Serial.println(F("WARNING: Errors were encountered while doing the format restore. The heads may not be positioned over the correct track."));
            Serial.println();
          }
          else{
            Serial.println(F("Format restore successful!"));
            Serial.println();
          }
          Serial.println(F("Step 5: Seeking to cylinder 0 on head 0..."));
          commandBufferWidget[0] = 0x16;
          commandBufferWidget[1] = 0x04;
          commandBufferWidget[2] = 0x00;
          commandBufferWidget[3] = 0x00;
          commandBufferWidget[4] = 0x00;
          commandBufferWidget[5] = 0x00;
          calcWidgetChecksum();
          Serial.print(F("Command: "));
          for(int i = 0; i < 7; i++){
            printDataSpace(commandBufferWidget[i]);
          }
          Serial.println();
          Serial.println();
          readSuccess = widgetRead(false, true);
          if(readSuccess == 0){
            Serial.println(F("WARNING: Errors were encountered while seeking. The heads may not be positioned over the correct track."));
            Serial.println();
          }

          readWidgetStatus(1, 2);
          bool correctSeek = true;
          for(int i = 0; i < 3; i++){
            if(driveStatus[i] != 0x00){
              Serial.println();
              correctSeek = false;
              break;
            }
          }
          readWidgetStatus(1, 3);
          if(driveStatus[0] != 0x00 or driveStatus[1] != 0x00){
            correctSeek = false;
          }
          readWidgetStatus(1, 4);
          if(bitRead(driveStatus[1], 1) != 1 or bitRead(driveStatus[1], 7) != 1){
            correctSeek = false;
          }
          readWidgetStatus(1, 0);
          if(bitRead(driveStatus[0], 1) != 0 or bitRead(driveStatus[1], 1) != 0){
            correctSeek = false;
          }
          if(correctSeek == true){
            Serial.print(F("Seek successful!"));
          }
          else{
            Serial.print(F("Error: Widget status says that the seek failed!"));
          }
          Serial.println();
          Serial.println();
          Serial.println(F("Step 6: Performing a trial format on cylinder 0, head 0..."));
          commandBufferWidget[0] = 0x12;
          commandBufferWidget[1] = 0x0C;
          calcWidgetChecksum();
          readSuccess = widgetRead(false, true);
          if(readSuccess == 0){
            Serial.println(F("WARNING: Errors were encountered while auto-offseting. The operation may not have succeeded."));
            Serial.println();
          }
          readWidgetStatus(1, 4);
          if(bitRead(driveStatus[1], 0) == 0){
            Serial.println(F("Error: Widget status says that auto-offset is still disabled!"));
          }
          readSuccess = widgetRead(false, true);
          if(readSuccess == 0){
            Serial.println(F("WARNING: Errors were encountered while auto-offseting. The operation may not have succeeded."));
            Serial.println();
          }
          readWidgetStatus(1, 4);
          if(bitRead(driveStatus[1], 0) == 0){
            Serial.println(F("Error: Widget status says that auto-offset is still disabled!"));
          }
          readSuccess = widgetRead(false, true);
          if(readSuccess == 0){
            Serial.println(F("WARNING: Errors were encountered while auto-offseting. The operation may not have succeeded."));
            Serial.println();
          }
          readWidgetStatus(1, 4);
          if(bitRead(driveStatus[1], 0) == 0){
            Serial.println(F("Error: Widget status says that auto-offset is still disabled!"));
          }
          commandBufferWidget[0] = 0x18;
          commandBufferWidget[1] = 0x0F;
          commandBufferWidget[2] = formatOffset;
          commandBufferWidget[3] = interleave;
          commandBufferWidget[4] = 0xF0;
          commandBufferWidget[5] = 0x78;
          commandBufferWidget[6] = 0x3C;
          commandBufferWidget[7] = 0x1E;
          calcWidgetChecksum();
          Serial.print(F("Command: "));
          for(int i = 0; i < 9; i++){
            printDataSpace(commandBufferWidget[i]);
          }
          Serial.println();
          Serial.println();
          readSuccess = widgetRead(false, true);
          if(readSuccess == 0){
            Serial.println(F("WARNING: Errors were encountered while formatting the track. The format may have failed."));
            Serial.println();
          }
          else{
            commandBufferWidget[0] = 0x13;
            commandBufferWidget[1] = 0x02;
            commandBufferWidget[2] = 0x01;
            calcWidgetChecksum();
            readSuccess = widgetRead(true, true);
            if(readSuccess == 0){
              Serial.println(F("WARNING: Errors were encountered while reading the track offset. The following offset information may be incorrect."));
              Serial.println();
            }
            Serial.print(F("Format complete! Fine offset is "));
            if(bitRead(blockData[1], 5) == 0){
              Serial.print(F("-"));
            }
            printDataNoSpace(blockData[1] & 0x1F);
            Serial.print(F("."));
            if((blockData[1] & 0x1F) >= 0x10){
              Serial.print(F(" This offset is 10 or larger, which is a bit concerning!"));
            }
            Serial.println();
            Serial.println();
          }
          Serial.println(F("Step 7: Reading cylinder 0, head 0, and sector 0 to make sure that the format worked..."));
          commandBufferWidget[0] = 0x12;
          commandBufferWidget[1] = 0x09;
          calcWidgetChecksum();
          Serial.print(F("Command: "));
          for(int i = 0; i < 3; i++){
            printDataSpace(commandBufferWidget[i]);
          }
          Serial.println();
          Serial.println();
          readSuccess = widgetRead();
          if(readSuccess == 0){
            Serial.println(F("WARNING: Errors were encountered during the read operation. The following data may be incorrect."));
            Serial.println();
            setLEDColor(1, 0, 0);
          }
          bool trialSuccess = true;
          if(driveStatus[0] != 0 or driveStatus[1] != 0 or driveStatus[2] != 0 or driveStatus[3] != 0){
            trialSuccess = false;
          }
          for(int i = 0; i < 532; i++){
            if(blockData[i] != 0xC6){
              trialSuccess = false;
            }
          }
          if(trialSuccess == true){
            Serial.println(F("Readback was successful! Continuing with format."));
          }
          else{
            Serial.print(F("Readback failed! Do you want to proceed with the rest of the format anyway (return for yes, 'n' to cancel)? "));
            while(1){
              if(Serial.available()){
                delay(50);
                userInput = Serial.read();
                flushInput();
                if(userInput == 'n'){
                  abort = true;
                  break;
                  }
                else if(userInput == '\r'){
                  abort = false;
                  break;
                }
                else{
                  Serial.print(F("Readback failed! Do you want to proceed with the rest of the format anyway (return for yes, 'n' to cancel)? "));
                  while(Serial.available()){
                    Serial.read();
                  }
                }
              }

            }
          }
          if(abort == true){
            Serial.println();
            break;
          }
          abort = true;
          Serial.println();
          Serial.println(F("Step 8: Performing another format restore..."));
          commandBufferWidget[0] = 0x13;
          commandBufferWidget[1] = 0x05;
          commandBufferWidget[2] = 0x70;
          calcWidgetChecksum();
          Serial.print(F("Command: "));
          for(int i = 0; i < 4; i++){
            printDataSpace(commandBufferWidget[i]);
          }
          Serial.println();
          Serial.println();
          readSuccess = widgetRead(false, true);
          if(readSuccess == 0){
            Serial.println(F("WARNING: Errors were encountered while doing the format restore. The heads may not be positioned over the correct track."));
            Serial.println();
          }
          else{
            Serial.println(F("Format restore successful!"));
            Serial.println();
          }
          Serial.println(F("Step 9: Formatting every track on the disk..."));
          Serial.println();
          byte lowCylinder = 0x00;
          byte highCylinder = 0x00;
          readWidgetStatus(1, 3);
          uint16_t fullCylinder = driveStatus[0] << 8;
          fullCylinder += driveStatus[1];
          fullCylinder += 1;
          for(uint16_t cylinder = fullCylinder; cylinder <= fullCylinder; cylinder--){
            for(byte head = 0x01; head <= 1; head--){
              commandBufferWidget[0] = 0x16;
              commandBufferWidget[1] = 0x04;
              commandBufferWidget[2] = cylinder >> 8;
              commandBufferWidget[3] = cylinder;
              commandBufferWidget[4] = head;
              commandBufferWidget[5] = 0x00;
              highCylinder = commandBufferWidget[2];
              lowCylinder = commandBufferWidget[3];
              calcWidgetChecksum();
              bool readSuccess = widgetRead(false, true);
              if(readSuccess == 0){
                Serial.println(F("WARNING: Errors were encountered while seeking. The heads may not be positioned over the correct track."));
                Serial.println();
              }
              readWidgetStatus(1, 2);
              bool correctSeek = true;
              if((driveStatus[0] != highCylinder) or (driveStatus[1] != lowCylinder) or (driveStatus[2] != head) or (driveStatus[3] != 0)){
                correctSeek = false;
              }
              readWidgetStatus(1, 3);
              if((driveStatus[0] != highCylinder) or (driveStatus[1] != lowCylinder)){
                correctSeek = false;
              }
              readWidgetStatus(1, 4);
              if(bitRead(driveStatus[1], 1) != 1 or bitRead(driveStatus[1], 7) != 1){
                correctSeek = false;
              }
              readWidgetStatus(1, 0);
              if(bitRead(driveStatus[0], 1) != 0 or bitRead(driveStatus[1], 1) != 0){
                correctSeek = false;
              }
          
              if(correctSeek == false){
                Serial.println(F("Error: Widget status says that the seek failed!"));
              }

              commandBufferWidget[0] = 0x12;
              commandBufferWidget[1] = 0x0C;
              calcWidgetChecksum();
              readSuccess = widgetRead(false, true);
              if(readSuccess == 0){
                Serial.println(F("WARNING: Errors were encountered while auto-offseting. The operation may not have succeeded."));
                Serial.println();
              }
              readWidgetStatus(1, 4);
              if(bitRead(driveStatus[1], 0) == 0){
                Serial.println(F("Error: Widget status says that auto-offset is still disabled!"));
              }
             /* 
             readSuccess = widgetRead(false, true);
              if(readSuccess == 0){
                Serial.println(F("WARNING: Errors were encountered while auto-offseting. The operation may not have succeeded."));
                Serial.println();
              }
              readWidgetStatus(1, 4);
              if(bitRead(driveStatus[1], 0) == 0){
                Serial.println(F("Error: Widget status says that auto-offset is still disabled!"));
              }
              readSuccess = widgetRead(false, true);
              if(readSuccess == 0){
                Serial.println(F("WARNING: Errors were encountered while auto-offseting. The operation may not have succeeded."));
                Serial.println();
              }
              readWidgetStatus(1, 4);
              if(bitRead(driveStatus[1], 0) == 0){
                Serial.println(F("Error: Widget status says that auto-offset is still disabled!"));
              }*/
              commandBufferWidget[0] = 0x13;
              commandBufferWidget[1] = 0x02;
              commandBufferWidget[2] = 0x01;
              calcWidgetChecksum();
              readSuccess = widgetRead(true, true);
              if(readSuccess == 0){
                Serial.println(F("WARNING: Errors were encountered while reading the track offset. The following offset information may be incorrect."));
                Serial.println();
              }
              
              Serial.print(F("Now formatting cylinder "));
              printDataNoSpace(highCylinder);
              printDataNoSpace(lowCylinder);
              Serial.print(F(" on head "));
              printDataNoSpace(head);
              Serial.print(F(" with an offset of "));
              printDataNoSpace(formatOffset);
              Serial.print(F(" and an interleave of "));
              printDataNoSpace(interleave);
              Serial.print(F(". "));
              commandBufferWidget[0] = 0x18;
              commandBufferWidget[1] = 0x0F;
              commandBufferWidget[2] = formatOffset;
              commandBufferWidget[3] = interleave;
              commandBufferWidget[4] = 0xF0;
              commandBufferWidget[5] = 0x78;
              commandBufferWidget[6] = 0x3C;
              commandBufferWidget[7] = 0x1E;
              calcWidgetChecksum();
              readSuccess = widgetRead(false, true);
              if(readSuccess == 0){
                Serial.println(F("WARNING: Errors were encountered while formatting the track. The format may have failed."));
                Serial.println();
              }

              Serial.print(F("Fine offset is "));
              if(bitRead(blockData[1], 5) == 0){
                Serial.print(F("-"));
              }
              printDataNoSpace(blockData[1] & 0x1F);
              Serial.print(F("."));
              if((blockData[1] & 0x1F) >= 0x10){
                Serial.print(F(" This offset is 10 or larger, which is a bit concerning!"));
              }
              Serial.println();
            }
          }
          Serial.println();
          Serial.println(F("Format complete!"));
          Serial.println();
          Serial.println(F("Step 10: Soft-resetting the Widget controller again..."));
          Serial.print(F("Command: "));
          commandBufferWidget[0] = 0x12;
          commandBufferWidget[1] = 0x07;
          calcWidgetChecksum();
          for(int i = 0; i < ((commandBufferWidget[0] & 0x0F) + 1); i++){
            printDataSpace(commandBufferWidget[i]);
          }
          Serial.println();
          Serial.println();
          commandResponseMatters = false;
          widgetRead(false, true);
          commandResponseMatters = true;
          commandBufferWidget[0] = 0x13;
          commandBufferWidget[1] = 0x01;
          commandBufferWidget[2] = 0x00;
          calcWidgetChecksum();
          readSuccess = widgetRead(false, true);
          if(readSuccess = true){
            Serial.println(F("Soft-reset complete!"));
          }
          else{
            Serial.println(F("Error: Unable to communicate with drive after reset!"));
          }
          Serial.println();
          Serial.println(F("Step 11: Checking to see if Widget has passed all self-tests..."));
          Serial.println(F("Command: 13 01 05 DA"));
          readWidgetStatus(1, 5);
          Serial.println();
          if((driveStatus[2] != 0xDB) or (driveStatus[3] != 0xE0)){
            Serial.println(F("Error: Bad Widget state status! The initialization of the spare table might fail."));
          }
          else if(driveStatus[1] != 0x01){
            Serial.println(F("Drive has passed all tests, but is somehow able to find its spare table. This really weird since we haven't created it yet!"));
          }
          else{
            Serial.println(F("Drive has passed all tests, but can't find its spare table. This is to be expected since we haven't created it yet!"));
          }
          Serial.println();
          Serial.println(F("Step 12: Initializing the drive's spare table..."));
          commandBufferWidget[0] = 0x18;
          commandBufferWidget[1] = 0x10;
          commandBufferWidget[2] = formatOffset;
          commandBufferWidget[3] = interleave;
          commandBufferWidget[4] = 0xF0;
          commandBufferWidget[5] = 0x78;
          commandBufferWidget[6] = 0x3C;
          commandBufferWidget[7] = 0x1E;
          calcWidgetChecksum();
          Serial.print(F("Command: "));
          for(int i = 0; i < 9; i++){
            printDataSpace(commandBufferWidget[i]);
          }
          Serial.println();
          Serial.println();
          readSuccess = widgetRead(false, true);
          if(readSuccess == 0){
            Serial.println(F("WARNING: Errors were encountered while initializing the spare table. The spare table may not have been created correctly."));
            Serial.println();
          }
          else{
            Serial.println(F("Spare table initialization complete!"));
          }
          Serial.println();
          Serial.println(F("Step 13: Soft-resetting the Widget controller yet again..."));
          Serial.print(F("Command: "));
          commandBufferWidget[0] = 0x12;
          commandBufferWidget[1] = 0x07;
          calcWidgetChecksum();
          for(int i = 0; i < ((commandBufferWidget[0] & 0x0F) + 1); i++){
            printDataSpace(commandBufferWidget[i]);
          }
          Serial.println();
          Serial.println();
          commandResponseMatters = false;
          widgetRead(false, true);
          commandResponseMatters = true;
          commandBufferWidget[0] = 0x13;
          commandBufferWidget[1] = 0x01;
          commandBufferWidget[2] = 0x00;
          calcWidgetChecksum();
          readSuccess = widgetRead(false, true);
          if(readSuccess = true){
            Serial.println(F("Soft-reset complete!"));
          }
          else{
            Serial.println(F("Error: Unable to communicate with drive after reset!"));
          }
          Serial.println();
          Serial.println(F("Step 14: Checking to see if Widget has passed all self-tests..."));
          Serial.println(F("Command: 13 01 05 DA"));
          readWidgetStatus(1, 5);
          Serial.println();
          if((driveStatus[2] != 0xDB) or (driveStatus[3] != 0xE0)){
            Serial.println(F("Error: Bad Widget state status! The surface scan might fail."));
          }
          else if(driveStatus[1] != 0x01){
            Serial.println(F("Drive has passed all tests and is now able to find its newly-created spare table!"));
          }
          else{
            Serial.println(F("Drive has passed all tests, but can't find its spare table. Maybe the init spare table command failed?"));
          }
          Serial.println();
          Serial.println(F("Step 15: Performing a surface scan on the Widget..."));
          commandBufferWidget[0] = 0x12;
          commandBufferWidget[1] = 0x13;
          calcWidgetChecksum();
          Serial.print(F("Command: "));
          for(int i = 0; i < 3; i++){
            printDataSpace(commandBufferWidget[i]);
          }
          Serial.println();
          Serial.println();
          readSuccess = widgetRead(false, false);
          if(readSuccess == 0){
            Serial.println(F("WARNING: Errors were encountered while performing the surface scan. The scan may not have succeeded."));
            Serial.println();
          }
          else{
            Serial.println(F("Surface scan complete! Bad blocks that were found (if any) were entered into the spare table automatically."));
          }
          Serial.println();
          readWidgetStatus(1, 0);
          printWidgetStatus();
          Serial.println(F("Low-level format complete!"));
          Serial.println();
          break;
        }
      }
      else{
        Serial.println();
      }
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();


      //Ask for interleave and offset
      //Soft reset
      //Disable recovery
      //Check if controller has passed all self-tests
      //See if state status registers are normal CMPI.L  #$0000DBE0,kStdStatus 
      //format recal
      //Seek to CHS0 (head 0 too)
      //Trial CHS0 format
      //Do a diag read on CHS0 (any sector we want) and make sure it works. If so, trial format passes!
      //Prompt user to continue if it fails?
      //format recal again
      //read the track we're on after the recal and then start formatting at track [that track + 1]. According to Current Cylinder status, we go to C220 after a format recal, so we start formattting at C221.
      //Seek to each track on each head in turn (start with highest cylinder and highest head (1) and then go down to C0 and H0 from there).
      //AutoOffset x3 on each track and head we get to
      //Print offset for each track and head we get to
      //Then format each track and head we get to
      //Soft reset
      //Check that state registers are normal CMPI.L  #$0001DBE0,kStdStatus
      //Init spare table
      //Soft reset
      //Check state registers AGAIN CMPI.L  #$0000DBE0,kStdStatus 
      //Scan
      //Restore standard status
    }

    else if(command.equalsIgnoreCase("J") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true){ //Format Track(s)
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.print(F("Please enter the cylinder and head that you want to format in the format CCCCHH or leave it blank to format all tracks on the disk: "));
      while(1){
        if(readSerialValue(6, true) == true){
          break;
        }
        else{
          Serial.print(F("Please enter the cylinder and head that you want to format in the format CCCCHH or leave it blank to format all tracks on the disk: "));
        }
      }
      byte highCylinder = serialBytes[0];
      byte lowCylinder = serialBytes[1];
      byte head = serialBytes[2];

      bool allTracks = false;
      if(serialBytes[6] == 0x55){
        allTracks = true;
      }
      byte formatOffset = 0x00;
      byte interleave = 0x01;
      Serial.print(F("Please enter your desired 1-byte format offset or press return to use the default of 00: "));
      while(1){
        if(readSerialValue(2, true) == true){
          break;
        }
        else{
          Serial.print(F("Please enter your desired 1-byte format offset or press return to use the default of 00: "));
        }
      }
      if(serialBytes[2] != 0x55){
        formatOffset = serialBytes[0];
      }
      Serial.print(F("Please enter your desired 1-byte interleave value or press return to use the default of 01 (meaning 1:2 interleave): "));
      while(1){
        if(readSerialValue(2, true) == true){
          break;
        }
        else{
          Serial.print(F("Please enter your desired 1-byte interleave value or press return to use the default of 01 (meaning 1:2 interleave): "));
        }
      }
      if(serialBytes[2] != 0x55){
        interleave = serialBytes[0];
      }


      if(allTracks == false){
        Serial.println();
        Serial.println(F("Performing a data restore before formatting the desired track..."));
        commandBufferWidget[0] = 0x13;
        commandBufferWidget[1] = 0x05;
        commandBufferWidget[2] = 0x40;
        calcWidgetChecksum();
        Serial.print(F("Command: "));
        for(int i = 0; i < 4; i++){
          printDataSpace(commandBufferWidget[i]);
        }
        Serial.println();
        Serial.println();
        bool readSuccess = widgetRead(false, true);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered while doing the data restore. The heads may not be positioned over the correct track."));
          Serial.println();
        }
        else{
          Serial.println(F("Data restore successful!"));
          Serial.println();
        }

        Serial.print(F("Now seeking to cylinder "));
        printDataNoSpace(highCylinder);
        printDataNoSpace(lowCylinder);
        Serial.print(F(" and head "));
        printDataNoSpace(head);
        Serial.println(F("..."));
        commandBufferWidget[0] = 0x16;
        commandBufferWidget[1] = 0x04;
        commandBufferWidget[2] = highCylinder;
        commandBufferWidget[3] = lowCylinder;
        commandBufferWidget[4] = head;
        commandBufferWidget[5] = 0x00;
        calcWidgetChecksum();
        Serial.print(F("Command: "));
        for(int i = 0; i < 7; i++){
          printDataSpace(commandBufferWidget[i]);
        }
        Serial.println();
        Serial.println();
        readSuccess = widgetRead(false, true);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered while seeking. The heads may not be positioned over the correct track."));
          Serial.println();
        }
        serialBytes[0] = highCylinder;
        serialBytes[1] = lowCylinder;
        serialBytes[2] = head;
        readWidgetStatus(1, 2);
        bool correctSeek = true;
        for(int i = 0; i < 3; i++){
          if(driveStatus[i] != serialBytes[i]){
            Serial.println();
            correctSeek = false;
            break;
          }
        }
        readWidgetStatus(1, 3);
        if(driveStatus[0] != serialBytes[0] or driveStatus[1] != serialBytes[1]){
          correctSeek = false;
        }
        readWidgetStatus(1, 4);
        if(bitRead(driveStatus[1], 1) != 1 or bitRead(driveStatus[1], 7) != 1){
          correctSeek = false;
        }
        readWidgetStatus(1, 0);
        if(bitRead(driveStatus[0], 1) != 0 or bitRead(driveStatus[1], 1) != 0){
          correctSeek = false;
        }
        if(correctSeek == true){
          Serial.print(F("Seek successful!"));
        }
        else{
          Serial.print(F("Error: Widget status says that the seek failed!"));
        }
        Serial.println();
        Serial.println();

        Serial.println(F("Auto-offsetting over the track to make sure we're centered well..."));
        commandBufferWidget[0] = 0x12;
        commandBufferWidget[1] = 0x0C;
        calcWidgetChecksum();
        Serial.print(F("Command: "));
        for(int i = 0; i < 3; i++){
          printDataSpace(commandBufferWidget[i]);
        }
        Serial.println();
        Serial.println();
        bool offsetWorked = true;
        readSuccess = widgetRead(false, true);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered on the auto-offset. The operation may not have succeeded."));
          Serial.println();
        }
        readWidgetStatus(1, 4);
        if(bitRead(driveStatus[1], 0) == 0){
          Serial.println(F("Error: Widget status says that the auto-offset is still disabled!"));
          offsetWorked = false;
        }
        /*
        readSuccess = widgetRead(false, true);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered on the second auto-offset. The operation may not have succeeded."));
          Serial.println();
        }
        readWidgetStatus(1, 4);
        if(bitRead(driveStatus[1], 0) == 0){
          Serial.println(F("Error: Widget status says that the auto-offset is still disabled on the second auto-offset!"));
          offsetWorked = false;
        }
        readSuccess = widgetRead(false, true);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered on the third auto-offset. The operation may not have succeeded."));
          Serial.println();
        }
        readWidgetStatus(1, 4);
        if(bitRead(driveStatus[1], 0) == 0){
          Serial.println(F("Error: Widget status says that the auto-offset is still disabled on the third auto-offset!"));
          offsetWorked = false;
        }*/
        if(offsetWorked == true){
          Serial.println(F("Auto-offset complete!"));
        }
        else{
          Serial.println(F("Auto-offset failed!"));
        }
        Serial.println();
        Serial.println(F("Reading the servo fine offset..."));
        commandBufferWidget[0] = 0x13;
        commandBufferWidget[1] = 0x02;
        commandBufferWidget[2] = 0x01;
        calcWidgetChecksum();
        Serial.print(F("Command: "));
        for(int i = 0; i < 4; i++){
          printDataSpace(commandBufferWidget[i]);
        }
        Serial.println();
        Serial.println();
        readSuccess = widgetRead(true, true);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered while reading the track offset. The following offset information may be incorrect."));
          Serial.println();
        }

        Serial.print(F("Now formatting cylinder "));
        printDataNoSpace(highCylinder);
        printDataNoSpace(lowCylinder);
        Serial.print(F(" on head "));
        printDataNoSpace(head);
        Serial.print(F(" with an offset of "));
        printDataNoSpace(formatOffset);
        Serial.print(F(" and an interleave of "));
        printDataNoSpace(interleave);
        Serial.println(F("..."));
        commandBufferWidget[0] = 0x18;
        commandBufferWidget[1] = 0x0F;
        commandBufferWidget[2] = formatOffset;
        commandBufferWidget[3] = interleave;
        commandBufferWidget[4] = 0xF0;
        commandBufferWidget[5] = 0x78;
        commandBufferWidget[6] = 0x3C;
        commandBufferWidget[7] = 0x1E;
        calcWidgetChecksum();
        Serial.print(F("Command: "));
        for(int i = 0; i < 9; i++){
          printDataSpace(commandBufferWidget[i]);
        }
        Serial.println();
        Serial.println();
        readSuccess = widgetRead(false, true);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered while formatting the track. The format may have failed."));
          Serial.println();
        }
        else{
          Serial.print(F("Format successful! Fine offset is "));
          if(bitRead(blockData[1], 5) == 0){
            Serial.print(F("-"));
          }
          printDataNoSpace(blockData[1] & 0x1F);
          Serial.print(F("."));
          if((blockData[1] & 0x1F) >= 0x10){
            Serial.print(F(" This offset is 10 or larger, which is a bit concerning!"));
          }
          Serial.println();
          Serial.println();
        }
        printWidgetStatus();
      }
      else{
        Serial.println();
        confirm();
        if(confirmOperation == true){
          byte lowCylinder = 0x00;
          byte highCylinder = 0x00;
          Serial.println();
          Serial.println(F("Performing a format restore before formatting the disk..."));
          commandBufferWidget[0] = 0x13;
          commandBufferWidget[1] = 0x05;
          commandBufferWidget[2] = 0x70;
          calcWidgetChecksum();
          Serial.print(F("Command: "));
          for(int i = 0; i < 4; i++){
            printDataSpace(commandBufferWidget[i]);
          }
          Serial.println();
          Serial.println();
          bool readSuccess = widgetRead(false, true);
          if(readSuccess == 0){
            Serial.println(F("WARNING: Errors were encountered while doing the format restore. The heads may not be positioned over the correct track."));
            Serial.println();
          }
          else{
            Serial.println(F("Format restore successful!"));
            Serial.println();
          }
          readWidgetStatus(1, 3);
          uint16_t fullCylinder = driveStatus[0] << 8;
          fullCylinder += driveStatus[1];
          fullCylinder += 1;
          head = 0x01;
          for(uint16_t cylinder = fullCylinder; cylinder <= fullCylinder; cylinder--){
            for(byte head = 0x01; head <= 1; head--){
              commandBufferWidget[0] = 0x16;
              commandBufferWidget[1] = 0x04;
              commandBufferWidget[2] = cylinder >> 8;
              commandBufferWidget[3] = cylinder;
              commandBufferWidget[4] = head;
              commandBufferWidget[5] = 0x00;
              highCylinder = commandBufferWidget[2];
              lowCylinder = commandBufferWidget[3];
              calcWidgetChecksum();
              bool readSuccess = widgetRead(false, true);
              if(readSuccess == 0){
                Serial.println(F("WARNING: Errors were encountered while seeking. The heads may not be positioned over the correct track."));
                Serial.println();
              }
              readWidgetStatus(1, 2);
              bool correctSeek = true;
              if((driveStatus[0] != highCylinder) or (driveStatus[1] != lowCylinder) or (driveStatus[2] != head) or (driveStatus[3] != 0)){
                correctSeek = false;
              }
              readWidgetStatus(1, 3);
              if((driveStatus[0] != highCylinder) or (driveStatus[1] != lowCylinder)){
                correctSeek = false;
              }
              readWidgetStatus(1, 4);
              if(bitRead(driveStatus[1], 1) != 1 or bitRead(driveStatus[1], 7) != 1){
                correctSeek = false;
              }
              readWidgetStatus(1, 0);
              if(bitRead(driveStatus[0], 1) != 0 or bitRead(driveStatus[1], 1) != 0){
                correctSeek = false;
              }
          
              if(correctSeek == false){
                Serial.println(F("Error: Widget status says that the seek failed!"));
              }

              commandBufferWidget[0] = 0x12;
              commandBufferWidget[1] = 0x0C;
              calcWidgetChecksum();
              readSuccess = widgetRead(false, true);
              if(readSuccess == 0){
                Serial.println(F("WARNING: Errors were encountered while auto-offseting. The operation may not have succeeded."));
                Serial.println();
              }
              readWidgetStatus(1, 4);
              if(bitRead(driveStatus[1], 0) == 0){
                Serial.println(F("Error: Widget status says that auto-offset is still disabled!"));
              }
              /*
              readSuccess = widgetRead(false, true);
              if(readSuccess == 0){
                Serial.println(F("WARNING: Errors were encountered while auto-offseting. The operation may not have succeeded."));
                Serial.println();
              }
              readWidgetStatus(1, 4);
              if(bitRead(driveStatus[1], 0) == 0){
                Serial.println(F("Error: Widget status says that auto-offset is still disabled!"));
              }
              readSuccess = widgetRead(false, true);
              if(readSuccess == 0){
                Serial.println(F("WARNING: Errors were encountered while auto-offseting. The operation may not have succeeded."));
                Serial.println();
              }
              readWidgetStatus(1, 4);
              if(bitRead(driveStatus[1], 0) == 0){
                Serial.println(F("Error: Widget status says that auto-offset is still disabled!"));
              }*/
              commandBufferWidget[0] = 0x13;
              commandBufferWidget[1] = 0x02;
              commandBufferWidget[2] = 0x01;
              calcWidgetChecksum();
              readSuccess = widgetRead(true, true);
              if(readSuccess == 0){
                Serial.println(F("WARNING: Errors were encountered while reading the track offset. The following offset information may be incorrect."));
                Serial.println();
              }
             
              Serial.print(F("Now formatting cylinder "));
              printDataNoSpace(highCylinder);
              printDataNoSpace(lowCylinder);
              Serial.print(F(" on head "));
              printDataNoSpace(head);
              Serial.print(F(" with an offset of "));
              printDataNoSpace(formatOffset);
              Serial.print(F(" and an interleave of "));
              printDataNoSpace(interleave);
              Serial.print(F(". "));
              commandBufferWidget[0] = 0x18;
              commandBufferWidget[1] = 0x0F;
              commandBufferWidget[2] = formatOffset;
              commandBufferWidget[3] = interleave;
              commandBufferWidget[4] = 0xF0;
              commandBufferWidget[5] = 0x78;
              commandBufferWidget[6] = 0x3C;
              commandBufferWidget[7] = 0x1E;
              calcWidgetChecksum();
              readSuccess = widgetRead(false, true);
              if(readSuccess == 0){
                Serial.println(F("WARNING: Errors were encountered while formatting the track. The format may have failed."));
                Serial.println();
              }

              Serial.print(F("Fine offset is "));
              if(bitRead(blockData[1], 5) == 0){
                Serial.print(F("-"));
              }
              printDataNoSpace(blockData[1] & 0x1F);
              Serial.print(F("."));
              if((blockData[1] & 0x1F) >= 0x10){
                Serial.print(F(" This offset is 10 or larger, which is a bit concerning!"));
              }
              Serial.println();
            }
          }
          Serial.println();
          Serial.println(F("Format complete!"));
          Serial.println();
        }
        else{
          Serial.println();
        }
      }
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }
    
    else if(command.equalsIgnoreCase("K") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true){ //Init Spare Table
      clearScreen();
      setLEDColor(0, 1, 0);
      byte formatOffset = 0x00;
      byte interleave = 0x01;
      Serial.print(F("Please enter your desired 1-byte format offset or press return to use the default of 00: "));
      while(1){
        if(readSerialValue(2, true) == true){
          break;
        }
        else{
          Serial.print(F("Please enter your desired 1-byte format offset or press return to use the default of 00: "));
        }
      }
      if(serialBytes[2] != 0x55){
        formatOffset = serialBytes[0];
      }
      Serial.print(F("Please enter your desired 1-byte interleave value or press return to use the default of 01 (meaning 1:2 interleave): "));
      while(1){
        if(readSerialValue(2, true) == true){
          break;
        }
        else{
          Serial.print(F("Please enter your desired 1-byte interleave value or press return to use the default of 01 (meaning 1:2 interleave): "));
        }
      }
      if(serialBytes[2] != 0x55){
        interleave = serialBytes[0];
      }
      Serial.println();
      Serial.print(F("Now initializing the Widget's spare tables with a format offset of "));
      printDataNoSpace(formatOffset);
      Serial.print(F(" and an interleave of "));
      printDataNoSpace(interleave);
      Serial.println(F("..."));
      commandBufferWidget[0] = 0x18;
      commandBufferWidget[1] = 0x10;
      commandBufferWidget[2] = formatOffset;
      commandBufferWidget[3] = interleave;
      commandBufferWidget[4] = 0xF0;
      commandBufferWidget[5] = 0x78;
      commandBufferWidget[6] = 0x3C;
      commandBufferWidget[7] = 0x1E;
      calcWidgetChecksum();
      Serial.print(F("Command: "));
      for(int i = 0; i < 9; i++){
        printDataSpace(commandBufferWidget[i]);
      }
      Serial.println();
      Serial.println();
      bool readSuccess = widgetRead(false, true);
      if(readSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered while initializing the spare table. The spare table may not have been created correctly."));
        Serial.println();
      }
      else{
        Serial.println(F("Spare table initialization complete!"));
      }
      Serial.println();
      printWidgetStatus();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("L") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true){ //Scan
      clearScreen();
      setLEDColor(0, 0, 0);
      Serial.println(F("Performing a surface scan on the Widget..."));
      commandBufferWidget[0] = 0x12;
      commandBufferWidget[1] = 0x13;
      calcWidgetChecksum();
      Serial.print(F("Command: "));
      for(int i = 0; i < 3; i++){
        printDataSpace(commandBufferWidget[i]);
      }
      Serial.println();
      Serial.println();
      bool readSuccess = widgetRead(false, false);
      if(readSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered while performing the surface scan. The scan may not have succeeded."));
        Serial.println();
      }
      else{
        Serial.println(F("Surface scan complete! Bad blocks that were found (if any) were entered into the spare table automatically."));
      }
      Serial.println();
      printWidgetStatus();
      Serial.print(F("Press return to continue..."));
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("M") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true){ //Park Heads
      clearScreen();
      Serial.println(F("Parking the Widget's heads..."));
      Serial.print(F("Command: "));
      commandBufferWidget[0] = 0x12;
      commandBufferWidget[1] = 0x08;
      calcWidgetChecksum();
      for(int i = 0; i < ((commandBufferWidget[0] & 0x0F) + 1); i++){
        printDataSpace(commandBufferWidget[i]);
      }
      Serial.println();
      Serial.println();
      widgetRead(false, true);
      commandBufferWidget[0] = 0x13;
      commandBufferWidget[1] = 0x01;
      commandBufferWidget[2] = 0x04;
      calcWidgetChecksum();
      widgetRead(false, true);
      if(bitRead(driveStatus[1], 4) == true){
        Serial.println(F("Heads successfully parked!"));
      }
      else{
        Serial.println(F("Error: Widget internal status says that the heads were NOT parked!"));
      }
      Serial.println();
      Serial.print(F("Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("N") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true){ //Send Custom Widget Command
      clearScreen();
      Serial.print(F("Please enter the Widget command that you want to send. Leave off the checksum; it will be added automatically: "));
      char charInput[2128];
      byte hexInput[1064];
      unsigned int charIndex = 0;
      bool goodInput = true;
      while(1){
        if(Serial.available()){
          char inByte = Serial.read();
          bool inArray = false;
          if(inByte == '\r'){
            inArray = true;
          }
          for(int i = 0; i < 22; i++){
            if(inByte == acceptableHex[i]){
              inArray = true;
            }
          }
          if(inArray == false){
            goodInput = false;
          }
          if(inByte == '\r' and goodInput == true){
            charInput[charIndex] = '\0';
            hex2bin(hexInput, charInput, &charIndex);
            break;
          }
          else if(inByte == '\r' and goodInput == false){
            Serial.print(F("Please enter the Widget command that you want to send. Leave off the checksum; it will be added automatically: "));
            charIndex = 0;
            goodInput = true;
          }
          else{
            charInput[charIndex] = inByte;
            if(charIndex < 2126){
              charIndex++;
            }
          }
        }
      }
      for(int i = 0; i < charIndex; i++){
        commandBufferWidget[i] = hexInput[i];
      }
      calcWidgetChecksum();
      Serial.println();
      Serial.println(F("Read commands return data and the status bytes are read before the data is sent to the ArduinoFile."));
      Serial.println(F("Write commands don't return data and the status bytes are read after data has been written to the drive."));
      Serial.print(F("Is this a read command (r) or a write command (w)? "));
      while(1){
        if(Serial.available()) {
          delay(50);
          userInput = Serial.read();
          flushInput();
          if(userInput == 'w' or userInput == 'r'){
            break;
          }
          else{
            while(Serial.available()){
              Serial.read();
            }
            Serial.print(F("Is this a read command (r) or a write command (w)? "));
          }
        }
      }
      Serial.println();
      Serial.print(F("Executing command: "));
      for(int i = 0; i <= (commandBufferWidget[0] & 0x0F); i++){
        printDataSpace(commandBufferWidget[i]);
      }
      setLEDColor(0, 0, 0);
      if(userInput == 'r'){
        Serial.println();
        Serial.println();
        bool readSuccess = widgetRead(true, false);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered during the operation. The following data may be incorrect."));
          Serial.println();
        }
        printRawData();
        Serial.println();
        printWidgetStatus();
      }
      if(userInput == 'w'){
        Serial.println();
        Serial.println();
        bool writeSuccess = widgetWrite(false);
        if(writeSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered during the operation."));
          Serial.println();
        }
        printWidgetStatus();
      }
      Serial.print(F("Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("R") and testMenu == false and diagMenu == false and diagMenuTenMeg == false and widgetMenu == true){ //Main Menu
      widgetMenu = false;
      clearScreen();
      flushInput();
    }


    else if(command.equalsIgnoreCase("7") and widgetServoMenu == true){ //Recal
      clearScreen();
      setLEDColor(0, 1, 0);
      Serial.print(F("Do you want to perform a data recal or a format recal (return for data, 'f' for format)? "));
      bool formatRecal = false;
      while(1){
        if(Serial.available()){
          delay(50);
          userInput = Serial.read();
          flushInput();
          if(userInput == 'f'){
            formatRecal = true;
            break;
          }
          else if(userInput == '\r'){
            break;
          }
          else{
            while(Serial.available()){
              Serial.read();
            }
            Serial.print(F("Do you want to perform a data recal or a format recal (return for data, 'f' for format)? "));
            while(Serial.available()){
              Serial.read();
            }
          }
        }
      }
      Serial.println();
      if(formatRecal == false){
        Serial.println(F("Now performing a data recal on the Widget servo..."));
        commandBufferWidget[0] = 0x16;
        commandBufferWidget[1] = 0x03;
        commandBufferWidget[2] = 0x40;
        commandBufferWidget[3] = 0x00;
        commandBufferWidget[4] = 0x00;
        commandBufferWidget[5] = 0x80;
        calcWidgetChecksum();
        Serial.print(F("Command: "));
        for(int i = 0; i < 7; i++){
          printDataSpace(commandBufferWidget[i]);
        }
      }
      else{
        Serial.println(F("Now performing a format recal on the Widget servo..."));
        commandBufferWidget[0] = 0x16;
        commandBufferWidget[1] = 0x03;
        commandBufferWidget[2] = 0x70;
        commandBufferWidget[3] = 0x00;
        commandBufferWidget[4] = 0x00;
        commandBufferWidget[5] = 0x80;
        calcWidgetChecksum();
        Serial.print(F("Command: "));
        for(int i = 0; i < 7; i++){
          printDataSpace(commandBufferWidget[i]);
        }
      }
      Serial.println();
      Serial.println();
      bool readSuccess = widgetRead(false);
      if(readSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered while performing the recal. The recal might have failed."));
        Serial.println();
        setLEDColor(1, 0, 0);
      }
      else{
        Serial.println(F("Recal command successfully sent to servo!"));
        setLEDColor(0, 1, 0);
      }
      setLEDColor(0, 1, 0);
      Serial.println();
      printWidgetStatus();
      Serial.print(F("Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("8") and widgetServoMenu == true){ //Access
      clearScreen();
      Serial.print(F("How many tracks to you want to move relative to your current location (add a '-' sign to move to lower tracks and no sign to move towards higher tracks)? "));
      while(1){
        if(readSerialValue(6, false, true) == true){
          if(serialBytes[1] == (serialBytes[1] & 0b00000011)){
            break;
          }
        }
        else{
          Serial.print(F("How many tracks to you want to move relative to your current location (add a '-' sign to move to lower tracks and no sign to move towards higher tracks)? "));
        }
      }
      Serial.println();
      byte highSteps = serialBytes[1] & 0b00000011;
      byte lowSteps = serialBytes[2];
      commandBufferWidget[0] = 0x16;
      commandBufferWidget[1] = 0x03;
      if(serialBytes[0] == '-'){
        commandBufferWidget[2] = 0x80;
      }
      else{
        commandBufferWidget[2] = 0x84;
      }
      commandBufferWidget[2] += highSteps;
      commandBufferWidget[3] = lowSteps;
      commandBufferWidget[4] = 0x00;
      commandBufferWidget[5] = 0x80;
      calcWidgetChecksum();
      Serial.print(F("Now moving the heads "));
      printDataNoSpace(highSteps);
      printDataNoSpace(lowSteps);
      Serial.print(F(" tracks "));
      if(serialBytes[0] == '-'){
        Serial.print(F("away from"));
      }
      else{
        Serial.print(F("towards"));
      }
      Serial.println(F(" the spindle..."));
      Serial.print(F("Command: "));
      for(int i = 0; i < 7; i++){
        printDataSpace(commandBufferWidget[i]);
      }
      Serial.println();
      Serial.println();
      bool readSuccess = widgetRead(false);
      if(readSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered while performing the access operation. The access might have failed."));
        Serial.println();
        setLEDColor(1, 0, 0);
      }
      else{
        setLEDColor(0, 1, 0);
        Serial.println(F("Access command successfully sent to servo!"));
      }
      Serial.println();
      printWidgetStatus();
      Serial.print(F("Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("9") and widgetServoMenu == true){ //Access With Offset
      clearScreen();
      Serial.print(F("How many tracks to you want to move relative to your current location (add a '-' sign to move to lower tracks and no sign to move towards higher tracks)? "));
      while(1){
        if(readSerialValue(6, false, true) == true){
          if(serialBytes[1] == (serialBytes[1] & 0b00000011)){
            break;
          }
        }
        else{
          Serial.print(F("How many tracks to you want to move relative to your current location (add a '-' sign to move to lower tracks and no sign to move towards higher tracks)? "));
        }
      }
      Serial.println();
      byte highSteps = serialBytes[1] & 0b00000011;
      byte lowSteps = serialBytes[2];
      commandBufferWidget[0] = 0x16;
      commandBufferWidget[1] = 0x03;
      if(serialBytes[0] == '-'){
        commandBufferWidget[2] = 0x90;
      }
      else{
        commandBufferWidget[2] = 0x94;
      }
      commandBufferWidget[2] += highSteps;
      commandBufferWidget[3] = lowSteps;
      commandBufferWidget[5] = 0x80;
      Serial.print(F("The current fine offset DAC value is "));
      readWidgetStatus(2, 1);
      if(bitRead(blockData[1], 5) == 0){
        Serial.print(F("-"));
        printDataNoSpace(blockData[1] & 0x1F);
      }
      else{
        printDataNoSpace(blockData[1] & 0x1F);
      }
      Serial.println(F("."));
      Serial.println();
      Serial.print(F("Enter a 1 or 0 to enable or disable auto-offset or leave this blank to specify a manual offset: "));
      while(1){
        if((readSerialValue(2, true) == true) and ((serialBytes[0] == 0x00) or (serialBytes[0] == 0x01))){
          break;
        }
        else{
          Serial.print(F("Enter a 1 or 0 to enable or disable auto-offset or leave this blank to specify a manual offset: "));
        }
      }
      if(serialBytes[2] != 0x55){
        Serial.println();
        if(serialBytes[0] == 0x00){
          commandBufferWidget[4] = 0x00;
        }
        else if(serialBytes[0] == 0x01){
          commandBufferWidget[4] = 0x40;
        }
      }
      else{
        Serial.println();
        Serial.println(F("Note: The offset command is weird in that the offset you specify is relative to the ends of the offset range, not the middle."));
        Serial.println(F("So an offset of 5 would take the positive end of the range (1F) and subtract 5, making the new offset 1A."));
        Serial.println(F("Likewise, an offset of -3 would take the negative end of the range (-1F) and add 3, making the new offset -1C."));
        Serial.println(F("To get the offset to zero, use either an offset of -1F or an offset of 1F."));
        Serial.println();
        Serial.print(F("How many steps do you want to offset by (add a '-' sign to start the offset at the previous track and no sign to start the offset at the next track)? "));
        while(1){
          if(readSerialValue(4, false, true) == true){
            if(serialBytes[1] == (serialBytes[1] & 0b00011111)){
              break;
            }
          }
          else{
            Serial.print(F("How many steps do you want to offset by (add a '-' sign to start the offset at the previous track and no sign to start the offset at the next track)? "));
          }
        }
        Serial.println();
        byte offset = serialBytes[1] & 0b00011111;
        if(serialBytes[0] == '-'){
          commandBufferWidget[4] = 0x80;
        }
        else{
          commandBufferWidget[4] = 0x00;
        }
        commandBufferWidget[4] += offset;
      }
      calcWidgetChecksum();
      Serial.print(F("Now moving the heads "));
      printDataNoSpace(highSteps);
      printDataNoSpace(lowSteps);
      Serial.print(F(" tracks "));
      if(bitRead(commandBufferWidget[2], 2) == 0){
        Serial.print(F("away from"));
      }
      else{
        Serial.print(F("towards"));
      }
      Serial.print(F(" the spindle and "));
      if(commandBufferWidget[4] == 0x40){
        Serial.println(F("enabling auto-offset..."));
      }
      else if(commandBufferWidget[4] == 0x00){
        Serial.println(F("disabling auto-offset..."));
      }
      else{
        Serial.print(F("offsetting "));
        printDataNoSpace(commandBufferWidget[4] & 0b00011111);
        Serial.print(F(" steps "));
        if(serialBytes[0] == '-'){
          Serial.print(F("away from the previous "));
        }
        else{
          Serial.print(F("away from the next "));
        }
        Serial.println(F("track..."));
      }
      Serial.print(F("Command: "));
      for(int i = 0; i < 7; i++){
        printDataSpace(commandBufferWidget[i]);
      }
      Serial.println();
      Serial.println();
      bool readSuccess = widgetRead(false);
      if(readSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered while performing the Access With Offset operation. The operation might have failed."));
        Serial.println();
        setLEDColor(1, 0, 0);
      }
      else{
        setLEDColor(0, 1, 0);
        Serial.println(F("Access With Offset command successfully sent to servo!"));
        Serial.print(F("The new fine offset DAC value is "));
        readWidgetStatus(2, 1);
        if(bitRead(blockData[1], 5) == 0){
          Serial.print(F("-"));
          printDataNoSpace(blockData[1] & 0x1F);
        }
        else{
          printDataNoSpace(blockData[1] & 0x1F);
        }
        Serial.println(F("."));
      }
      Serial.println();
      printWidgetStatus();
      Serial.print(F("Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("A") and widgetServoMenu == true){ //Offset
      clearScreen();
      Serial.print(F("The current fine offset DAC value is "));
      readWidgetStatus(2, 1);
      if(bitRead(blockData[1], 5) == 0){
        Serial.print(F("-"));
        printDataNoSpace(blockData[1] & 0x1F);
      }
      else{
        printDataNoSpace(blockData[1] & 0x1F);
      }
      Serial.println(F("."));
      Serial.println();
      Serial.print(F("Enter a 1 or 0 to enable or disable auto-offset or leave this blank to specify a manual offset: "));
      while(1){
        if((readSerialValue(2, true) == true) and ((serialBytes[0] == 0x00) or (serialBytes[0] == 0x01))){
          break;
        }
        else{
          Serial.print(F("Enter a 1 or 0 to enable or disable auto-offset or leave this blank to specify a manual offset: "));
        }
      }
      Serial.println();
      if(serialBytes[2] != 0x55){
        if(serialBytes[0] == 0x01){
          Serial.println(F("Telling the servo to enable auto-offset..."));
          commandBufferWidget[0] = 0x16;
          commandBufferWidget[1] = 0x03;
          commandBufferWidget[2] = 0x10;
          commandBufferWidget[3] = 0x00;
          commandBufferWidget[4] = 0x40;
          commandBufferWidget[5] = 0x80;
          calcWidgetChecksum();
          Serial.print(F("Command: "));
          for(int i = 0; i < 7; i++){
            printDataSpace(commandBufferWidget[i]);
          }
        }
        else{
          Serial.println(F("Telling the servo to disable auto-offset..."));
          commandBufferWidget[0] = 0x16;
          commandBufferWidget[1] = 0x03;
          commandBufferWidget[2] = 0x10;
          commandBufferWidget[3] = 0x00;
          commandBufferWidget[4] = 0x00;
          commandBufferWidget[5] = 0x80;
          calcWidgetChecksum();
          Serial.print(F("Command: "));
          for(int i = 0; i < 7; i++){
            printDataSpace(commandBufferWidget[i]);
          }
        }
        Serial.println();
        Serial.println();
        bool readSuccess = widgetRead(false);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered while sending the offset command. The command might not have executed successfully."));
          Serial.println();
          setLEDColor(1, 0, 0);
        }
        else{
          setLEDColor(0, 1, 0);
          if(serialBytes[0] == 0x01){
            Serial.println(F("Enable auto-offset command successfully sent to servo!"));
          }
          else{
            Serial.println(F("Disable auto-offset command successfully sent to servo!"));
          }
        }
        Serial.print(F("The new fine offset DAC value is "));
        readWidgetStatus(2, 1);
        if(bitRead(blockData[1], 5) == 0){
          Serial.print(F("-"));
          printDataNoSpace(blockData[1] & 0x1F);
        }
        else{
          printDataNoSpace(blockData[1] & 0x1F);
        }
        Serial.println(F("."));
      }
      else{
        Serial.println(F("Note: The offset command is weird in that the offset you specify is relative to the ends of the offset range, not the middle."));
        Serial.println(F("So an offset of 5 would take the positive end of the range (1F) and subtract 5, making the new offset 1A."));
        Serial.println(F("Likewise, an offset of -3 would take the negative end of the range (-1F) and add 3, making the new offset -1C."));
        Serial.println(F("To get the offset to zero, use either an offset of -1F or an offset of 1F."));
        Serial.println();
        Serial.print(F("How many steps do you want to offset by (add a '-' sign to start the offset at the previous track and no sign to start the offset at the next track)? "));
        while(1){
          if(readSerialValue(4, false, true) == true){
            if(serialBytes[1] == (serialBytes[1] & 0b00011111)){
              break;
            }
          }
          else{
            Serial.print(F("How many steps do you want to offset by (add a '-' sign to start the offset at the previous track and no sign to start the offset at the next track)? "));
          }
        }
        Serial.println();
        byte offset = serialBytes[1] & 0b00011111;
        commandBufferWidget[0] = 0x16;
        commandBufferWidget[1] = 0x03;
        commandBufferWidget[2] = 0x10;
        commandBufferWidget[3] = 0x00;
        if(serialBytes[0] == '-'){
          commandBufferWidget[4] = 0x80;
        }
        else{
          commandBufferWidget[4] = 0x00;
        }
        commandBufferWidget[4] += offset;
        commandBufferWidget[5] = 0x80;
        calcWidgetChecksum();
        Serial.print(F("Now offsetting "));
        printDataNoSpace(offset);
        Serial.print(F(" steps "));
        if(serialBytes[0] == '-'){
          Serial.print(F("away from the previous "));
        }
        else{
          Serial.print(F("away from the next "));
        }
        Serial.println(F("track..."));
        Serial.print(F("Command: "));
        for(int i = 0; i < 7; i++){
          printDataSpace(commandBufferWidget[i]);
        }
        Serial.println();
        Serial.println();
        bool readSuccess = widgetRead(false);
        if(readSuccess == 0){
          Serial.println(F("WARNING: Errors were encountered while performing the offset operation. The offset might have failed."));
          Serial.println();
          setLEDColor(1, 0, 0);
        }
        else{
          setLEDColor(0, 1, 0);
          Serial.println(F("Offset command successfully sent to servo!"));
        }
        Serial.print(F("The new fine offset DAC value is "));
        readWidgetStatus(2, 1);
        if(bitRead(blockData[1], 5) == 0){
          Serial.print(F("-"));
          printDataNoSpace(blockData[1] & 0x1F);
        }
        else{
          printDataNoSpace(blockData[1] & 0x1F);
        }
        Serial.println(F("."));
      }
      Serial.println();
      readWidgetStatus(1, 0);
      printWidgetStatus();
      Serial.print(F("Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("B") and widgetServoMenu == true){ //Home
      clearScreen();
      Serial.println(F("Note: The home command sometimes sets the Operation Failed and Controller Aborted Last Operation status bits for some reason. Ignore this; everything's fine!"));
      Serial.println();
      Serial.println(F("Telling the servo to home the heads..."));
      commandBufferWidget[0] = 0x16;
      commandBufferWidget[1] = 0x03;
      commandBufferWidget[2] = 0xC0;
      commandBufferWidget[3] = 0x00;
      commandBufferWidget[4] = 0x00;
      commandBufferWidget[5] = 0x80;
      calcWidgetChecksum();
      Serial.print(F("Command: "));
      for(int i = 0; i < 7; i++){
        printDataSpace(commandBufferWidget[i]);
      }
      Serial.println();
      Serial.println();
      bool readSuccess = widgetRead(false);
      if(readSuccess == 0){
        Serial.println(F("WARNING: Errors were encountered while homing the heads. The heads may not be homed."));
        Serial.println();
        setLEDColor(1, 0, 0);
      }
      else{
        setLEDColor(0, 1, 0);
      }
      readWidgetStatus(2, 1);
      if(bitRead(blockData[0], 0) == 1){
        Serial.println(F("Servo status reports that the heads have been homed!"));
      }
      else{
        Serial.println(F("Error: Servo status says that the heads failed to home properly."));
      }
      Serial.println();
      printWidgetStatus();
      Serial.print(F("Press return to continue..."));
      setLEDColor(0, 1, 0);
      flushInput();
      while(!Serial.available());
      flushInput();
    }

    else if(command.equalsIgnoreCase("D") and widgetServoMenu == true){ //Widget Menu
      clearScreen();
      widgetServoMenu = false;
      widgetMenu = true;
      flushInput();
      flushInput();
    }

      //Upper nybble of byte 1 is the command (0=read status, 2=diagnostic, C=home, 1=offset-track following, 7=format recal, 4=data recal, 9=access with offset, 8=access)
      //Lower nybble of byte 1 is access bits. B3 is unused, B2 is direction (1=toward spindle), and B1 and B0 are the 512's and 256's place of the seek offset itself.
      //Byte 2 is just the lower byte of the seek offset itself.
      //Byte 3 is the instruction for an offset command. B7 is the offset direction (1=toward spindle), B6 is auto-offset (1=on, making all other bits irrelevant), B5 is whether or not we want to read the offset value after this operation, and B4-B0 are the value of the offset itself.
      //Byte 4 is a status byte. B7 is the comm rate (1=57600, 0=19200), B6 is power-on reset, B5 and B4 are unused, and B3-B0 are status/diagnostic bits.
      //Byte 5 is a checksum (just add bytes 1-4 together and then invert the sum).

      //Offset uses byte 3 but not byte 2
      //Format recal uses neither 2 or 3
      //Data recal uses neither 2 or 3
      //Access uses byte 2 but not 3
      //Access_Offset uses both
      //Home uses neither

      //Access with offset seems to update the DAC value (but ONLY if the auto-offset bit is set in byte 3 OR you specify a manual offset in byte 3, but the manual offset seems a little weird), but Access does not, no matter what.
      //Home works, but gives a "controller aborted" error each time. And the Widget status LED turns off after you run it for some reason. After homing, the only way of getting back into a usable state seems to be a soft reset.
      //Read status just doesn't work.
      //Use the offset command with auto-offset enabled to turn on auto-offset. Or specify an offset manually to move the heads by that amount. But the manual offset is strange.
      //Format and data recal work just as we would expect.
      //Reset bit doesn't seem to actually reset anything.



    //else{
    if(testMenu == true){
      testSubMenu();
    }
    else if(diagMenu == true){
      Z8SubMenu();
    }
    else if(diagMenuTenMeg == true){
      tenMegZ8SubMenu();
    }
    else if(widgetMenu == true){
      widgetSubMenu();
    }
    else if(widgetServoMenu == true){
      servoSubMenu();
    }
    else{
      mainMenu();
    }
   //}
  }
}

void printRawBinary(byte aByte){
  for (int8_t aBit = 7; aBit >= 0; aBit--){
    Serial.write(bitRead(aByte, aBit) ? '1' : '0');
  }
}

void readWidgetStatus(byte whichStatus, byte statusLongwordNumber){
  byte tempBuffer[15];
  for(int i = 0; i < 14; i++){
    tempBuffer[i] = commandBufferWidget[i];
  }
  commandBufferWidget[0] = 0x13;
  commandBufferWidget[1] = whichStatus;
  commandBufferWidget[2] = statusLongwordNumber;
  calcWidgetChecksum();
  bool readSuccess = true;
  if(whichStatus == 2){
    readSuccess = widgetRead(true, true);
  }
  else{
    readSuccess = widgetRead(false, true);
  }
  if(readSuccess == 0){
    Serial.println();
    Serial.println(F("WARNING: Errors were encountered while reading the following status longword."));
    Serial.println();
  }
  for(int i = 0; i < 14; i++){
    commandBufferWidget[i] = tempBuffer[i];
  }
}

void confirm(){
  confirmOperation = false;
  Serial.print(F("WARNING: This operation will destroy all data on your drive! Are you sure you want to continue (return for yes, 'n' to cancel)? "));
  while(1){
    if(Serial.available()){
      delay(50);
      userInput = Serial.read();
      flushInput();
      if(userInput == 'n'){
        break;
        }
      else if(userInput == '\r'){
        confirmOperation = true;
        break;
      }
      else{
        while(Serial.available()){
          Serial.read();
        }
        Serial.print(F("WARNING: This operation will destroy all data on your drive! Are you sure you want to continue (return for yes, 'n' to cancel)? "));
      }
    }

  }
}


void startReceipt(){
  long int originalStart = millis();
  start = millis();
  while(1){
    if(millis() - start >= 3000){
      Serial.write(0x43);
      start = millis();
    }
    if(Serial.available()){
      break;
    }
    if(millis() - originalStart >= 30000){
      Serial.println();
      Serial.println(F("Operation failed - XMODEM sender never started!"));
      done = 1;
      failed = true;
      break;
    }
  }
}

void receivePacket(){
  if(ackStatus == 0x00){
    Serial.write(NAK);
  }
  else if(ackStatus == 0x01){
    Serial.write(ACK);
  }
  else if(ackStatus == 0x02){
    startReceipt();
  }
  start = millis();
  while(!Serial.available() and done == 0){
    if(millis() - start >= 5000){
      delay(2000);
      Serial.println(F("Operation failed - XMODEM timeout error!"));
      done = 1;
      failed = true;
      break;
    }
  }
  dataIndex = 0;
  //Serial.println(dataIndex, DEC);
  //Serial.println(done, DEC);
  //Serial.println(done, BIN);
  while(dataIndex < 1029 and done == 0){
    if(Serial.available()){
      rawData[dataIndex] = Serial.read();
      dataIndex += 1;
      if(rawData[0] == EOT){
        delay(2000);
        //xModemData.close();
        Serial.write(ACK);
        done = 1;
        break;
      }
    }
  }
  if(done == 0){
    for(int i = 0; i < 1024; i++){
      crcArray[i] = rawData[i + 3];
    }
    CRC = calc_crc(1024);

    actualCRC = rawData[1027] << 8 | rawData[1028];

    if(CRC == actualCRC and packetNum == rawData[1] and notPacketNum == rawData[2] and done == 0){
      packetNum += 1;
      notPacketNum -= 1;
      //Serial.write(ACK);
      ackStatus = 0x01;
    }
    else if((CRC != actualCRC or packetNum != rawData[1] or notPacketNum != rawData[2]) and done == 0){
      //Serial.write(NAK);
      ackStatus = 0x00;
    }
  }
}

void getDriveType(){
  setLEDColor(0, 1, 0);
  Serial.println(F("Reading spare table to determine drive size..."));
  Serial.println(F("Command: 00 FF FF FF 0A 03"));
  byte oldBlockData[532];
  for(int i = 0; i < 532; i++){
    oldBlockData[i] = blockData[i];
  }
  profileRead(0xFF, 0xFF, 0xFF);
  setLEDColor(0, 1, 0);
  if(blockData[13] == 0x00 and blockData[14] == 0x00 and blockData[15] == 0x00){
    Serial.print(F("Drive is a 5MB ProFile with "));
  }
  else if(blockData[13] == 0x00 and blockData[14] == 0x00 and blockData[15] == 0x10){
    Serial.print(F("Drive is a 10MB ProFile with "));
  }
  else if(blockData[13] == 0x00 and blockData[14] == 0x01 and blockData[15] == 0x00){
    Serial.print(F("Drive is a 10MB Widget with "));
  }
  else if(blockData[13] == 0x00 and blockData[14] == 0x01 and blockData[15] == 0x10){
    Serial.print(F("Drive is a 20MB Widget with "));
  }
  else if(blockData[13] == 0x00 and blockData[14] == 0x01 and blockData[15] == 0x20){
    Serial.print(F("Drive is a 40MB Widget with "));
  }
  else{
    Serial.print(F("Drive type is unknown with "));
  }
  //byte driveSize[3];
  for(int i = 18; i < 21; i++){
    Serial.print(blockData[i]>>4, HEX);
    Serial.print(blockData[i]&0x0F, HEX);
    driveSize[i - 18] = blockData[i];
  }
  Serial.print(F(" blocks. Is this correct (return for yes, 'n' for no)? "));
  while(1){
    if(Serial.available()){
      delay(50);
      userInput = Serial.read();
      flushInput();
      if(userInput == 'n'){
        Serial.print(F("Please enter the size of your drive in blocks: "));
        while(1){
          if(readSerialValue(6) == true and serialBytes[0] % 2 == 0){
            driveSize[0] = serialBytes[0];
            driveSize[1] = serialBytes[1];
            driveSize[2] = serialBytes[2];
            break;
          }
          else{
            Serial.print(F("Please enter the size of your drive in blocks: "));
          }
        }
        break;
      }
      else if(userInput == '\r'){
        break;
      }
      else{
        if(blockData[13] == 0x00 and blockData[14] == 0x00 and blockData[15] == 0x00){
          Serial.print(F("Drive is a 5MB ProFile with "));
        }
        else if(blockData[13] == 0x00 and blockData[14] == 0x00 and blockData[15] == 0x10){
          Serial.print(F("Drive is a 10MB ProFile with "));
        }
        else if(blockData[13] == 0x00 and blockData[14] == 0x01 and blockData[15] == 0x00){
          Serial.print(F(" Drive is a 10MB Widget with "));
        }
        else{
          Serial.print(F(" (Unknown Drive Type)"));
        }
        for(int i = 18; i < 21; i++){
          Serial.print(blockData[i]>>4, HEX);
          Serial.print(blockData[i]&0x0F, HEX);
          driveSize[i - 18] = blockData[i];
        }
        Serial.print(F(" blocks. Is this correct (return for yes, 'n' for no)? "));
      }
    }
  }
  for(int i = 0; i < 532; i++){
    blockData[i] = oldBlockData[i];
  }
}

void repeatTest(){
  repeat = false;
  Serial.print(F("Loop the test forever (return for no, 'y' for yes)? "));
  while(1){
    if(Serial.available()){
      delay(50);
      userInput = Serial.read();
      flushInput();
      if(userInput == 'y'){
        repeat = true;
        break;
      }
      else if(userInput == '\r'){
        break;
      }
      else{
        while(Serial.available()){
          Serial.read();
        }
        Serial.print(F("Loop the test forever (return for no, 'y' for yes)? "));
      }
    }
  }
}

void writeTwoBlocks(byte address0, byte address1, byte address2){
  int startBackupErrors = backupErrors;
  setLEDColor(0, 0, 0);
  uint32_t fullAddress = address0 << 16 | address1 << 8 | address2;
  uint32_t totalBlocks = (driveSize[0]<<16 | driveSize[1]<<8 | driveSize[2]);
  bool handshakeSuccessful = profileHandshake();
  byte commandResponse = sendCommandBytes(writeCommand, address0, address1, address2, defaultRetries, defaultSpareThreshold);
  if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0 or handshakeSuccessful == 0 or commandResponse != 0x03){
    if(fullAddress < totalBlocks){
      backupErrors += 1;
    }
  }
  fullAddress += 1;
  setRW();
  //delay(1);
  clearCMD();
  //delay(1);
  while(readBsy() != 1){
    long int startTime = millis();
    if(millis() - startTime >= 10000){
      break;
    }
  }
  //delay(1);
  setParallelDir(1);
  delayMicroseconds(1);
  for(int i = 0; i < 532; i++){
    sendData(blockData[i]);
    delayMicroseconds(readDelay);
    setSTRB();
    delayMicroseconds(readDelay);
    clearSTRB();
    delayMicroseconds(readDelay);
  }
  clearRW();
  //delay(1);
  setCMD();
  //delay(1);
  //setSTRB();
  delayMicroseconds(readDelay);
  setParallelDir(0);
  delayMicroseconds(1);
  while(readBsy() != 0 or receiveData() != 0x06){
    long int startTime = millis();
    if(millis() - startTime >= 10000){
      break;
    }
  }
  setSTRB();
  //delay(1);
  clearSTRB();
  //delay(1);
  setRW();
  setParallelDir(1);
  delayMicroseconds(1);
  //delay(1);
  sendData(0x55);
  //delay(1);
  clearCMD();
  readStatusBytes();
  handshakeSuccessful = profileHandshake();
  address0 = fullAddress >> 16;
  address1 = fullAddress >> 8;
  address2 = fullAddress;
  commandResponse = sendCommandBytes(writeCommand, address0, address1, address2, defaultRetries, defaultSpareThreshold);
  if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0 or handshakeSuccessful == 0 or commandResponse != 0x03){
    if(fullAddress < totalBlocks){
      backupErrors += 1;
    }
  }
  setRW();
  //delay(1);
  clearCMD();
  //delay(1);
  while(readBsy() != 1){
    long int startTime = millis();
    if(millis() - startTime >= 10000){
      break;
    }
    if(startBackupErrors != backupErrors and fullAddress < totalBlocks){
      setLEDColor(1, 0, 0);
    }
    else{
      setLEDColor(0, 1, 0);
    }
  }
  //delay(1);
  setParallelDir(1);
  delayMicroseconds(1);
  for(int i = 532; i < 1064; i++){
    sendData(blockData[i]);
    delayMicroseconds(readDelay);
    setSTRB();
    delayMicroseconds(readDelay);
    clearSTRB();
    delayMicroseconds(readDelay);
  }
  clearRW();
  //delay(1);
  setCMD();
  //delay(1);
  //setSTRB();
  delayMicroseconds(readDelay);
  setParallelDir(0);
  delayMicroseconds(1);
  while(readBsy() != 0 or receiveData() != 0x06){
    long int startTime = millis();
    if(millis() - startTime >= 10000){
      break;
    }
  }
  setSTRB();
  //delay(1);
  clearSTRB();
  //delay(1);
  setRW();
  setParallelDir(1);
  delayMicroseconds(1);
  //delay(1);
  sendData(0x55);
  //delay(1);
  clearCMD();
  readStatusBytes();
}

void readTwoBlocks(byte address0, byte address1, byte address2){
  int startBackupErrors = backupErrors;
  uint32_t fullAddress = address0 << 16 | address1 << 8 | address2;
  uint32_t totalBlocks = (driveSize[0]<<16 | driveSize[1]<<8 | driveSize[2]);
  setLEDColor(0, 0, 0);
  bool handshakeSuccessful = profileHandshake();
  byte commandResponse = sendCommandBytes(readCommand, address0, address1, address2, defaultRetries, defaultSpareThreshold);
  readStatusBytes();
  if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0 or handshakeSuccessful == 0 or commandResponse != 0x02){
    if(fullAddress < totalBlocks){
      backupErrors += 1;
    }
  }
  setParallelDir(0);
  delayMicroseconds(1);
  for(int i = 0; i <= 531; i++){ //Read all 532 bytes from the drive into the blockData array
    blockData[i] = receiveData();
    //parity[i] = readParity();
    delayMicroseconds(readDelay);
    setSTRB();
    delayMicroseconds(readDelay);
    clearSTRB();
    delayMicroseconds(readDelay);
  }
  clearCMD();
  delayMicroseconds(readDelay);
  handshakeSuccessful = profileHandshake();
  fullAddress += 1;
  commandResponse = sendCommandBytes(readCommand, fullAddress >> 16, fullAddress >> 8, fullAddress, defaultRetries, defaultSpareThreshold);
  readStatusBytes();
  if((driveStatus[0] & B11111101) != 0 or (driveStatus[1] & B11011110) != 0 or (driveStatus[2] & B01000000) != 0 or handshakeSuccessful == 0 or commandResponse != 0x02){
    if(fullAddress < totalBlocks){
      backupErrors += 1;
    }
  }
  setParallelDir(0);
  delayMicroseconds(1);
  for(int i = 532; i < 1064; i++){ //Read all 532 bytes from the drive into the blockData array
    blockData[i] = receiveData();
    //parity[i] = readParity();
    delayMicroseconds(readDelay);
    setSTRB();
    delayMicroseconds(readDelay);
    clearSTRB();
    delayMicroseconds(readDelay);
  }
  clearCMD();
  delayMicroseconds(readDelay);
  if(startBackupErrors != backupErrors and fullAddress < totalBlocks){
    setLEDColor(1, 0, 0);
  }
  else{
    setLEDColor(0, 1, 0);
  }
}


bool startTransmission(){
  start = millis();
  while(!Serial.available()){
    if(millis() - start > 30000){
      return false;
    }
  }
  while(Serial.available()){
    inData = Serial.read();
  }
  if(inData != 0x43){

  }
  return true;
}

void startNewPacket(){
  Serial.write(STX);
  Serial.write(packetNum);
  Serial.write(notPacketNum);
}
byte finishPacket(){
  CRC = calc_crc(1024);
  lowCRC = CRC;
  highCRC = CRC >> 8;
  Serial.write(highCRC);
  Serial.write(lowCRC);
  packetNum += 0x01;
  notPacketNum -= 0x01;
  checksum = 0x00;
  start = millis();
  while(!Serial.available()){
    if(millis() - start > 5000){
      return 0x00;
    }
  }
    while(Serial.available()){
      inData = Serial.read();
      if(inData == NAK){
        packetNum -= 0x01;
        notPacketNum += 0x01;
        return 0x01;
      }
    }
    return 0x02;
}

void finishTransmission(){
  Serial.write(EOT);
}

uint16_t calc_crc(int n){
// Initial value. xmodem uses 0xFFFF but this example
// requires an initial value of zero.
uint16_t x = 0x0000;


for(int i = 0; i < n; i++){
  x = crc_xmodem_update(x, (uint16_t)crcArray[i]);
}
return(x);
}

// See bottom of this page: http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
// Polynomial: x^16 + x^12 + x^5 + 1 (0x1021)
uint16_t crc_xmodem_update (uint16_t crc, uint8_t info)
{
crc = crc ^ ((uint16_t)info << 8);
for (int i=0; i<8; i++) {
  if (crc & 0x8000){
    crc = (crc << 1) ^ 0x1021; //(polynomial = 0x1021)
  }
  else{
    crc <<= 1;
  }
}
return crc;
}


static void hex2bin(uint8_t *out, const char *in, size_t *dataSize)
{
    size_t sz = 0;
    while (*in) {
        while (*in == ' ') in++;  // skip spaces
        if (!*in) break;
        uint8_t theValue = *in>='a' ? *in-'a'+10 : *in>='A' ? *in-'A'+10 : *in-'0';
        in++;
        theValue <<= 4;
        if (!*in) break;
        theValue |= *in>='a' ? *in-'a'+10 : *in>='A' ? *in-'A'+10 : *in-'0';
        in++;
        *out++ = theValue;
        sz++;
    }
    *dataSize = sz;
}

void resetDrive(){
  setPRES();
  delay(10);
  clearPRES();
  delay(10);
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
  Serial.print(F("                                              "));
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

/*void printRawParity(){
  Serial.println(F("Raw Parity Data:"));
  Serial.print(F("0000: "));
  for(int i = 0; i <= 531; i++){
    Serial.print(parity[i]>>4, HEX);
    Serial.print(parity[i]&0x0F, HEX);
    Serial.print(F(" "));
    if((i + 1) % 8 == 0 and (i + 1) % 16 != 0){
      Serial.print(F(" "));
    }
    if((i + 1) % 16 == 0){
      Serial.print(F("        "));
      for(int j = i - 15; j <= i; j++){
        if(parity[j] <= 0x1F){
          Serial.print(F("."));
        }
        else{
          Serial.write(parity[j]);
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
    if(parity[i] <= 0x1F){
      Serial.print(F("."));
    }
    else{
      Serial.write(parity[i]);
    }
  }
  Serial.println();
}*/


void printDataNoSpace(byte printingData){
  Serial.print(printingData>>4, HEX);
  Serial.print(printingData&0x0F, HEX);
}

void printDataSpace(byte printingData){
  Serial.print(printingData>>4, HEX);
  Serial.print(printingData&0x0F, HEX);
  Serial.print(F(" "));
}

/*PD 7  6  5  4  3 2 1 0
PB 13 12 11 10 9 8*/

//SETTING A BIT ALWAYS MEANS PUTTING IT INTO ITS ACTIVE STATE (MEANING LOW IF IT'S ACTIVE LOW). DO THIS IN THE CONTROL FUNCTIONS
//ADD ALL THE CONTROL LINES INSTEAD OF JUST CMD AND BSY
//READING FROM THE CONTROL BITS
//WHAT SHOULD A FUNCTION DO WHEN IT'S done? SHOULD IT LEAVE THE DIRECTION AS WHATEVER IT SET IT TO OR SHOULD IT REVERT IT TO WHAT IT WAS BEFORE?
//SET DIRECTIONS FOR CONTROL LINE R/W. SOME LINES NEED TO BE INPUTS WHILE OTHERS ARE OUTPUTS

void setParallelDir(bool dir){
  if(dir == 0){
    REG_WRITE(GPIO_ENABLE_W1TC_REG, 0b11111111 << busOffset);
    //DDRL = B00000000;
  }
  else if(dir == 1){
    REG_WRITE(GPIO_ENABLE_W1TS_REG, 0b11111111 << busOffset);
    //DDRL = B11111111;
  }
}


void initPins(){
  REG_WRITE(GPIO_ENABLE_W1TS_REG, 0b1 << CMDPin);
  REG_WRITE(GPIO_ENABLE_W1TC_REG, 0b1 << BSYPin);
  REG_WRITE(GPIO_ENABLE_W1TS_REG, 0b1 << RWPin);
  REG_WRITE(GPIO_ENABLE_W1TS_REG, 0b1 << STRBPin);
  REG_WRITE(GPIO_ENABLE_W1TS_REG, 0b1 << PRESPin);
  REG_WRITE(GPIO_ENABLE_W1TC_REG, 0b1 << PARITYPin);

  clearCMD();
  clearRW();
  clearSTRB();
  clearPRES();
  clearPRES();
  setPRES();
  delay(1);
  clearPRES();
  delay(1);
}

void sendData(byte parallelBits){
  REG_WRITE(GPIO_OUT_W1TS_REG, parallelBits << busOffset);
  REG_WRITE(GPIO_OUT_W1TC_REG, ((byte)~parallelBits << busOffset));
  //DDRL = B11111111;
  //PORTL = parallelBits;
}

byte receiveData(){
  return REG_READ(GPIO_IN_REG) >> busOffset;
  //DDRL = B00000000;
  //return PINL;
}


void halt(){
  Serial.println(F("Program halted!"));
  while(1);
}

void setCMD(){
  REG_WRITE(GPIO_OUT_W1TC_REG, 0b1 << CMDPin);
  //PORTC = PORTC & B11111110;
}

void clearCMD(){
  REG_WRITE(GPIO_OUT_W1TS_REG, 0b1 << CMDPin);
  //PORTC = PORTC | B00000001;
}

void setBSY(){
  REG_WRITE(GPIO_ENABLE_W1TS_REG, 0b1 << BSYPin);
  REG_WRITE(GPIO_OUT_W1TC_REG, 0b1 << BSYPin);
  //DDRC = DDRC | B00000010;
  //PORTC = PORTC & B11111101;
}

void clearBSY(){
  REG_WRITE(GPIO_ENABLE_W1TS_REG, 0b1 << BSYPin);
  REG_WRITE(GPIO_OUT_W1TS_REG, 0b1 << BSYPin);
  //DDRC = DDRC | B00000010;
  //PORTC = PORTC | B00000010;
}

void setRW(){
  REG_WRITE(GPIO_OUT_W1TC_REG, 0b1 << RWPin);
  //PORTC = PORTC & B11111011;
}

void clearRW(){
  REG_WRITE(GPIO_OUT_W1TS_REG, 0b1 << RWPin);
  //PORTC = PORTC | B00000100;
}

void setSTRB(){
  REG_WRITE(GPIO_OUT_W1TC_REG, 0b1 << STRBPin);
  //PORTC = PORTC & B11110111;
}

void clearSTRB(){
  REG_WRITE(GPIO_OUT_W1TS_REG, 0b1 << STRBPin);
  //PORTC = PORTC | B00001000;
}
void setPRES(){
  REG_WRITE(GPIO_OUT_W1TC_REG, 0b1 << PRESPin);
  //PORTC = PORTC & B11101111;
}

void clearPRES(){
  REG_WRITE(GPIO_OUT_W1TS_REG, 0b1 << PRESPin);
  //PORTC = PORTC | B00010000;
}

void setPARITY(){
  REG_WRITE(GPIO_ENABLE_W1TS_REG, 0b1 << PARITYPin);
  REG_WRITE(GPIO_OUT_W1TC_REG, 0b1 << PARITYPin);
  //PORTC = PORTC & B11011111;
}

void clearPARITY(){
  REG_WRITE(GPIO_ENABLE_W1TS_REG, 0b1 << PARITYPin);
  REG_WRITE(GPIO_OUT_W1TS_REG, 0b1 << PARITYPin);
  //PORTC = PORTC | B00100000;
}

/*
void setPCHK(){
  //PORTC = PORTC & B10111111;
}

void clearPCHK(){
  //PORTC = PORTC | B01000000;
}

void setPOCD(){
  //PORTC = PORTC & B01111111;
}

void clearPOCD(){
  //PORTC = PORTC | B10000000;
}
*/

bool readBsy(){
  REG_WRITE(GPIO_ENABLE_W1TC_REG, 0b1 << BSYPin);
  return bitRead(REG_READ(GPIO_IN_REG), BSYPin);
  //DDRC = DDRC & B11111101;
  //return bitRead(PINC, 1);
}

bool readParity(){
  REG_WRITE(GPIO_ENABLE_W1TC_REG, 0b1 << PARITYPin);
  return bitRead(REG_READ(GPIO_IN_REG), PARITYPin);
  //DDRC = DDRC & B11011111;
  //return bitRead(PINC, 5);
}

bool checkParity(byte parityData, byte parity){
  byte B_cnt = 0;
  for (; parityData; parityData >>= 1) {
    if (parityData & 1) {
      B_cnt++;
    }
  }
  if(B_cnt % 2 == 1 and parity == 0x00){
    return true;
  }
  else if(B_cnt % 2 == 0 and parity == 0x01){
    return true;
  }
  else{
    return false;
  }
}

bool profileHandshake(){ //Returns true if the handshake succeeds, false if it fails
  bool success = 1;
  setCMD();
  long int startTime = millis();
  setParallelDir(0);
  delayMicroseconds(1);
  while(receiveData() != 0b00000001 or readBsy() != 0){
    if(millis() - startTime >= 5000){
      //Serial.println(F("Handshake Failed!!!")); //If more than 5 seconds pass and the drive hasn't responded with a $01, halt the program
      success = 0;
      break;
    }
  }
  //setSTRB(); //Pulse strobe to tell the drive that we got its $01 MEOW
  //delay(1);
  //clearSTRB();
  //delay(1);
  setParallelDir(1);
  delayMicroseconds(1);
  sendData(0x55); //Respond to the drive's $01 with a $55
  //delay(1);
  setRW(); //Tell the drive that we're writing to it
  //delay(1);
  clearCMD(); //And raise CMD
  //delay(1);
  return success;
}

byte sendCommandBytes(byte byte0, byte byte1, byte byte2, byte byte3, byte byte4, byte byte5){ //Returns the response byte from the profile or 0xFF if the repsonse is invalid
  byte commandResponse = byte0 + 2;
  commandBuffer[0] = byte0;
  commandBuffer[1] = byte1;
  commandBuffer[2] = byte2;
  commandBuffer[3] = byte3;
  commandBuffer[4] = byte4;
  commandBuffer[5] = byte5;
  while(readBsy() != 1){

  }
  byte success;
  setParallelDir(1);
  delayMicroseconds(1);
  //setSTRB(); //For some reason, STRB seems to be active high during the command byte phase of the transfer, so set it low to start this phase MEOW
  delayMicroseconds(1);
  sendData(byte0); //Send command byte 0 (command type) and pulse the strobe
  delayMicroseconds(1);
  setSTRB();
  delayMicroseconds(1);
  clearSTRB();
  delayMicroseconds(1);
  sendData(byte1); //Send command byte 1 (byte 0 of block number) and pulse the strobe
  delayMicroseconds(1);
  setSTRB();
  delayMicroseconds(1);
  clearSTRB();
  delayMicroseconds(1);
  sendData(byte2); //Send command byte 2 (byte 1 of block number) and pulse the strobe
  delayMicroseconds(1);
  setSTRB();
  delayMicroseconds(1);
  clearSTRB();
  delayMicroseconds(1);
  sendData(byte3); //Send command byte 3 (byte 2 of block number) and pulse the strobe
  delayMicroseconds(1);
  setSTRB();
  delayMicroseconds(1);
  clearSTRB();
  delayMicroseconds(1);
  sendData(byte4); //Send command byte 4 (retry count) and pulse the strobe
  delayMicroseconds(1);
  setSTRB();
  delayMicroseconds(1);
  clearSTRB();
  delayMicroseconds(1);
  sendData(byte5); //Send command byte 5 (spare threshold) and pulse the strobe
  delayMicroseconds(1);
  setSTRB();
  delayMicroseconds(1);
  clearSTRB();
  delayMicroseconds(1);
  //clearSTRB(); //Now the strobe is back in active-low mode again, so clear it after the command bytes are sent MEOW
  //delay(1);
  clearRW(); //We want to read the status bytes from the drive now, so set R/W to read mode
  delayMicroseconds(1);
  setCMD(); //Lower CMD to tell the drive that we're ready for its confirmation byte


  //DO TWO BACKUPS WITH THE SAME DELAYS AND COMPARE THE RESULTS TO SEE IF WE NEED DELAYS IN THE READ DMA TOO. THEN REDUCE DELAYS AND DO ANOTHER BACKUP TO SEE IF IT STILL WORKS. FOR A WRITE, DO WE NEED DELAYS WHEN DMAING TO THE DRIVE?
  //55 mins for the full sun20 transfer with 1ms delays.
  //39 mins for 1us delays. Assuming we read the whole disk block by block, the 20 1us delays will only add around 1s to the total read time.
  //BLU takes 44 minutes to backup the whole disk at 115200. Double that time at 57600.
  //what would increasing the baud rate do?
  //make sure that things still work with ProFiles and Widgets too!
  
  long int startTime = millis();
  setParallelDir(0);
  delayMicroseconds(1);
  while(1){
    success = receiveData();
    if((success == commandResponse) and readBsy() == 0){
      break;
    }
    if(readBsy() == 0 and commandResponseMatters == false){
      delay(1);
      break;
    }
    if(millis() - startTime >= 5000){
      //Serial.println(F("Command Confirmation Failed!!!")); //If more than 5 seconds pass and the drive hasn't responded with an $02, halt the program
      success = 0xFF;
      break;
    }
  }

  //delay(1);
  //setSTRB(); //Acknowledge that we read the $02 confirmation by pulsing the strobe MEOW
  setParallelDir(0);
  delayMicroseconds(1);
  success = receiveData();
  //delay(1);
  //clearSTRB();
  setRW(); //Tell the drive that we're writing to the bus and respond to its $02 with a $55
  setParallelDir(1);
  delayMicroseconds(1);
  sendData(0x55);
  //delay(1);
  clearCMD();
  return success;
}

void readStatusBytes(){ //Returns the byte array containing the status bytes with a fifth byte at the end that's 0xFF if the operation succeeds and 0x00 if the drive never finishes reading the block
  driveStatus[4] = 0xFF;
  clearCMD(); //Raise CMD to tell the drive that it's time to read the desired block
  long int startTime = millis();
  while(readBsy() != 1){ //Wait until the drive finishes reading the block and time out if it doesn't finish within 10 seconds
    if(millis() - startTime >= 10000){
      driveStatus[4] = 0x00;
      break;
    }
  }
  clearRW(); //Go into read mode to get the status bytes from the drive
  delayMicroseconds(readDelay);
  setParallelDir(0);
  delayMicroseconds(1);
  for(int i = 0; i <= 3; i++){ //Fill all 4 bytes of the status array with the status bytes from the drive, pulsing strobe each time we read a byte from the drive
    driveStatus[i] = receiveData();
    delayMicroseconds(readDelay);
    setSTRB();
    delayMicroseconds(readDelay);
    clearSTRB();
    delayMicroseconds(readDelay);
  }
  //delay(1);
}

void readData(){ //Returns the 532-byte array with the data that was read from the drive
  setParallelDir(0);
  delayMicroseconds(1);
  for(int i = 0; i <= 531; i++){ //Read all 532 bytes from the drive into the blockData array
    blockData[i] = receiveData();
    //parity[i] = readParity();
    delayMicroseconds(readDelay);
    setSTRB();
    delayMicroseconds(readDelay);
    clearSTRB();
    delayMicroseconds(readDelay);
  }
  clearCMD();
  delayMicroseconds(readDelay);
}

void writeData(uint16_t writeBytes){
  setRW();
  //delay(1);
  clearCMD();
  //delay(1);
  while(readBsy() != 1){
    long int startTime = millis();
    if(millis() - startTime >= 10000){
      Serial.println(F("Drive Never Said It Was Ready To Receive Data!!!"));
      break;
    }
  }
  //delay(1);
  setParallelDir(1);
  delayMicroseconds(1);
  for(int i = 0; i < writeBytes; i++){
    sendData(blockData[i]);
    delayMicroseconds(readDelay);
    setSTRB();
    delayMicroseconds(readDelay);
    clearSTRB();
    delayMicroseconds(readDelay);
  }
  clearRW();
  //delay(1);
  setCMD();
  //delay(1);
  //setSTRB();
  delayMicroseconds(readDelay);
  setParallelDir(0);
  delayMicroseconds(1);
  while(readBsy() != 0 or (receiveData() != 0x06 and commandResponseMatters)){
    if(commandResponseMatters == false){
      delay(1);
    }
    long int startTime = millis();
    if(millis() - startTime >= 10000){
      Serial.println(F("Drive never responded after write operation!"));
      break;
    }
  }
  setSTRB();
  //delay(1);
  clearSTRB();
  //delay(1);
  setRW();
  setParallelDir(1);
  delayMicroseconds(1);
  //delay(1);
  sendData(0x55);
  //delay(1);
  clearCMD();
}

void setLEDColor(bool r, bool g, bool b){
  digitalWrite(red, r);
  digitalWrite(green, g);
  //digitalWrite(blue, b);
}

/*ProFile PD0-PD7 are Arduino pins D2-D9
CMD is A0
BSY is A1
R/W is A2
STRB is A3
PRES is A4
Parity is A5
PCHK is A6
POCD is A7 (or you can just ground POCD with the jumper)

PD0-PD7 is Arduino Mega Port L 0-7
CMD, BSY, R/W, STRB, PRES, Parity, PCHK, and POCD are Arduino Mega Port C 0-7, respectively.*/
