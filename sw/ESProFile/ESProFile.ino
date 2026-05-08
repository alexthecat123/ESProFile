//*************************************************************************************************************
//* ESProFile - A powerful ESP32-based emulator and diagnostic tool for the ProFile family of hard drives.    *
//* By: Alex Anderson-McLeod                                                                                  *
//* Email address: alexelectronicsguy@gmail.com                                                               *
//*************************************************************************************************************

// ******** Changelog ********
// 2/22/2025 - Inlined a few functions to improve performance during ProFile comms.
// 4/19/2026 - Added support for pin definition header files to allow easy customization of ESProFile for different board layouts, and used this to create the LisaFPGA variant of ESProFile. Also cached the ProFile read/write routines to make them fast enough for LisaFPGA's 75MHz DOTCK mode.

#include <Arduino.h> // Include the necessary libraries
#include <SPI.h>
#include "SdFat.h"
#include "sdios.h"
#include "EEPROM.h"
// Include this pin definition file if you've got a standard ESProFile board
//#include "PinDefs_ESProFile.h"
// But include this one instead if you're using the ESProFile that's built into the LisaFPGA board
#include "PinDefs_LisaFPGA.h"

// The entire purpose of this file is to pick whether we want to run ESProFile_Diagnostic.ino or ESProFile_Emulator.ino
void setup(){
  pinMode(switch_pin, INPUT); // Set the switch pin to an input
  bool switchState = digitalRead(switch_pin); // Read its state
  pinMode(red_led, OUTPUT); // Set the red LED to an output
  pinMode(green_led, OUTPUT); // And the green LED too
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
    analogWrite(red_led, red_pwm_duty); // If r is 1, set the duty cycle to the desired value to turn the red LED on
  }
  else if(r == 0){
    analogWrite(red_led, 0); // Else, set the duty cycle to 0% to turn the red LED off
  }
  // For green, we have to use PWM since the LED is painfully bright otherwise
  if(g == 1){
    analogWrite(green_led, green_pwm_duty); // If g is 1, set the duty cycle to the desired value to turn the green LED on
  }
  else if (g == 0){
    analogWrite(green_led, 0); // Else, set the duty cycle to 0% to turn the green LED off
  }
}

// Sets the direction of the parallel bus
inline __attribute__((__always_inline__)) void setParallelDir(bool dir){
  if(dir == 0){ // Set to an input if dir is 0
    REG_WRITE(BUS_ENABLE_W1TC_REG, 0b11111111 << busOffset);
  }
  else if(dir == 1){ // And an output if dir is 1
    REG_WRITE(BUS_ENABLE_W1TS_REG, 0b11111111 << busOffset);
  }
}

// A user-friendly way to send data over the bus in less time-sensitive situations
inline __attribute__((__always_inline__)) void sendData(uint8_t parallelBits){
  REG_WRITE(BUS_W1TS_REG, parallelBits << busOffset); // Write W1TS with the data to set all the bits that need to be set
  REG_WRITE(BUS_W1TC_REG, ((byte)~parallelBits << busOffset)); // Write W1TC with the inverted data to clear all the bits that need to be cleared
}

// A user-friendly way to receive data over the bus in less time-sensitive situations
inline __attribute__((__always_inline__)) uint8_t receiveData(){
  return REG_READ(BUS_IN_REG) >> busOffset; // Return the 8-bit value on the bus
}

// Prints a byte in hex without a space following it
inline __attribute__((__always_inline__)) void printDataNoSpace(uint8_t data){
  Serial.print(data >> 4, HEX); // Print the high nibble
  Serial.print(data & 0x0F, HEX); // And the low nibble
}