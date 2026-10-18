#!/bin/zsh

arduino-cli compile --fqbn esp32:esp32:esp32 && arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/cu.SLAB_USBtoUART
#arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc && arduino-cli upload --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc -p /dev/cu.usbmodem11201
#/dev/cu.usbmodem2101
