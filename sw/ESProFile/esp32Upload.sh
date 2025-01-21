#!/bin/zsh

arduino-cli compile --fqbn esp32:esp32:esp32 && arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/cu.SLAB_USBtoUART