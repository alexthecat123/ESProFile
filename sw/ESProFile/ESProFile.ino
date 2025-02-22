//*************************************************************************************************************
//* ESProFile - A powerful ESP32-based emulator and diagnostic tool for the ProFile family of hard drives.    *                               *
//* By: Alex Anderson-McLeod                                                                                  *
//* Email address: alexelectronicsguy@gmail.com                                                               *
//*************************************************************************************************************

// ******** Changelog ********
// 2/22/2025 - Inlined a few functions to improve performance during ProFile comms

#include <Arduino.h> // Include the necessary libraries
#include <SPI.h>
#include "SdFat.h"
#include "sdios.h"
#include "EEPROM.h"

// Common pin definitions for both diagnostic and emulation modes

// SD card pins
#define SD_CLK  5 
#define SD_MISO 34
#define SD_MOSI 2
#define SD_CS   33

// ProFile bus pins
#define busOffset 12
#define CMDPin 21
#define BSYPin 22
#define RWPin 23
#define STRBPin 25
#define PRESPin 26
#define PARITYPin 27

// Pins for the red and green LEDs
#define red 32
#define green 4

#define switchPin 35 // Pin for the switch that selects between diagnostic and emulation modes

// The entire purpose of this file is to pick whether we want to run ESProFile_Diagnostic.ino or ESProFile_Emulator.ino
void setup(){
  pinMode(switchPin, INPUT); // Set the switch pin to an input
  bool switchState = digitalRead(switchPin); // Read its state
  pinMode(red, OUTPUT); // Set the red LED to an output
  pinMode(green, OUTPUT); // And the green LED too
  Serial.begin(115200); // Start serial comms
  clearScreen(); // Clear the screen
  setLEDColor(1, 0); // Make the LED red to show that ESProFile is initializing
  if(switchState == 1){ // If the switch is in the diagnostic position, boot into diagnostic mode
    Serial.println("ESProFile is booting into diagnostic mode...");
    delay(2000);
    diagSetup(); // Equivalents of void setup() and void loop() for the diagnostic mode
    diagLoop();
  }
  else{ // Otherwise, boot into emulation mode
    Serial.println("ESProFile is booting into emulation mode...");
    delay(2000);
    emulatorSetup(); // Equivalents of void setup() and void loop() for the emulation mode
    emulatorOuterLoop();
  }
}

// We have a while(1) here because the emulatorLoop will return on errors, in which case we just want to call it again
void emulatorOuterLoop(){
  while(1){
    emulatorLoop();
  }
}

void loop(){
  vTaskDelete(NULL);
  // We don't need to do anything here
}

// The following are helper functions that are used in both ESProFile_Diagnostic.ino and ESProFile_Emulator.ino

// Clears the screen by sending the ANSI escape codes for clearing the screen and moving the cursor to the top-left
void clearScreen(){
  Serial.write(27);
  Serial.print("[2J");
  Serial.write(27);
  Serial.print("[H");
}

// Sets the red and green sub-LEDs to the specified values
inline __attribute__((__always_inline__)) void setLEDColor(bool r, bool g){
  // Set red if r is 1, clear it if r is 0
  if(r == 1){
    REG_WRITE(GPIO_OUT1_W1TS_REG, 0b1);
  }
  else if(r == 0){
    REG_WRITE(GPIO_OUT1_W1TC_REG, 0b1);
  }
  // For green, we have to use PWM since the LED is painfully bright otherwise
  if(g == 1){
    analogWrite(green, 10); // If g is 1, set the duty cycle to 5%-ish to put the green LED at a reasonable/pleasant brightness
    //REG_WRITE(GPIO_OUT_W1TS_REG, 0b1 << green);
  }
  else if (g == 0){
    analogWrite(green, 0); // Else, set the duty cycle to 0% to turn the green LED off
    //REG_WRITE(GPIO_OUT_W1TC_REG, 0b1 << green);
  }
}

// Sets the direction of the parallel bus
inline __attribute__((__always_inline__)) void setParallelDir(bool dir){
  if(dir == 0){ // Set to an input if dir is 0
    REG_WRITE(GPIO_ENABLE_W1TC_REG, 0b11111111 << busOffset);
  }
  else if(dir == 1){ // And an output if dir is 1
    REG_WRITE(GPIO_ENABLE_W1TS_REG, 0b11111111 << busOffset);
  }
}

// A user-friendly way to send data over the bus in less time-sensitive situations
inline __attribute__((__always_inline__)) void sendData(uint8_t parallelBits){
  REG_WRITE(GPIO_OUT_W1TS_REG, parallelBits << busOffset); // Write W1TS with the data to set all the bits that need to be set
  REG_WRITE(GPIO_OUT_W1TC_REG, ((byte)~parallelBits << busOffset)); // Write W1TC with the inverted data to clear all the bits that need to be cleared
}

// A user-friendly way to receive data over the bus in less time-sensitive situations
inline __attribute__((__always_inline__)) uint8_t receiveData(){
  return REG_READ(GPIO_IN_REG) >> busOffset; // Return the 8-bit value on the bus
}

// Prints a byte in hex without a space following it
inline __attribute__((__always_inline__)) void printDataNoSpace(uint8_t data){
  Serial.print(data >> 4, HEX); // Print the high nibble
  Serial.print(data & 0x0F, HEX); // And the low nibble
}