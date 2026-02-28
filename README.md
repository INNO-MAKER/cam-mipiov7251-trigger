# cam-mipiov7251-trigger
-Raspberry PI Latest OS  support ov7251 sensor module

## Quick Start Guide for raspberry pi os build in driver
- Modify config.txt sudo  nano /boot/firmware/config.txt
- add dtoverlay=ov7251,cam0=1 to last line
- download json file from our github: sudo git clone https://github.com/INNO-MAKER/cam-mipiov7251-trigger.git
- cd cam-mipiov7251-trigger
- sudo cp ov7251_mono.json /usr/share/libcamera/ipa/rpi/pisp/
- sudo reboot
### After Reboot
- rpicam-hello -t 0
- or rpicam-hello -t 0 /usr/share/libcamera/ipa/rpi/pisp/ov7251_mono.json 

### Working Mode
- rpicam-hello --list-cameras

### Quick start Guide for innomaker unique driver 
- support software and hardware external trigger AND software trigger.
- Download json file from our github: sudo git clone https://github.com/INNO-MAKER/cam-mipiov7251-trigger.git
- cd cam-mipiov7251-trigger
- sudo cp ov7251_mono.json /usr/share/libcamera/ipa/rpi/pisp/
- Follower UserManual Compiler and install driver,Change working mode.

## Timeout
- If the cameras don’t all start within 1 second, the rpicam applications can time out. To prevent this, edit a configuration file on any Raspberry Pi with sink cameras.
- https://www.raspberrypi.com/documentation/accessories/camera.html#libcamera-configuration
