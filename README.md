# CAM-MIPI7251RAW-TRIGGER Camera Module

The **CAM-MIPI7251RAW-TRIGGER** is an industrial-grade global shutter camera module for Raspberry Pi, featuring the **OmniVision OV7251** sensor. It is designed for high-speed motion capture, robotics, and machine vision applications where rolling shutter distortion must be eliminated.

This module supports both the official Raspberry Pi kernel driver and a unique InnoMaker driver that enables advanced hardware and software trigger capabilities.

---

## Key Features

*   **OmniVision OV7251 Sensor**: VGA resolution (640 x 480) with 3.0 µm pixels.
*   **Global Shutter**: Captures fast-moving objects without distortion.
*   **High Frame Rate**: Supports 30fps, 60fps, and 90fps modes.
*   **Dual Driver Support**:
    *   **Native Driver**: Easy setup for standard streaming.
    *   **InnoMaker Unique Driver**: Enables precise hardware/software external triggering.
*   **Broad Compatibility**: Fully validated on Raspberry Pi 4 and Raspberry Pi 5 (Debian Bookworm/Trixie).

---

## Quick Start Guide (Native Driver)

Use this method for standard video streaming and preview using the built-in Raspberry Pi OS driver.

### 1. Configure the Overlay
Edit the configuration file:
```bash
sudo nano /boot/firmware/config.txt
```
Add the following line to the end of the file:
```text
dtoverlay=ov7251,cam0=1
```

### 2. Install Configuration Files
Clone this repository and copy the ISP configuration:
```bash
git clone https://github.com/INNO-MAKER/cam-mipiov7251-trigger.git
cd cam-mipiov7251-trigger
sudo cp ov7251_mono.json /usr/share/libcamera/ipa/rpi/pisp/
```

### 3. Reboot & Test
Reboot your Raspberry Pi:
```bash
sudo reboot
```
After reboot, test the camera:
```bash
# List available cameras
rpicam-hello --list-cameras

# Preview the stream
rpicam-hello -t 0 --tuning-file /usr/share/libcamera/ipa/rpi/pisp/ov7251_mono.json
```

---

## Advanced Features: InnoMaker Unique Driver

The InnoMaker driver provides enhanced control for trigger-based applications.

### 1. Driver Installation
Navigate to the driver source directory and follow the instructions in the [User Manual](./1-document/CAM-MIPI7251RAW%20UserManual-V1.4.pdf) to compile and install the kernel module:
```bash
cd ov7251_driver_source_code_pi5_support/source_code
make
sudo insmod inno_mipi_ov7251.ko
```

### 2. Trigger Mode
Once the unique driver is installed, you can switch between continuous streaming and external trigger modes as described in the manual.

---

## Troubleshooting: Timeout Issues

If the camera fails to start within 1 second, the `rpicam` applications may timeout. To prevent this, increase the timeout in the libcamera configuration. Refer to the [official documentation](https://www.raspberrypi.com/documentation/accessories/camera.html#libcamera-configuration) for details.

---

## Repository Structure

*   [`1-document/`](./1-document/): 
    *   [`CAM-MIPI7251RAW UserManual-V1.4.pdf`](./1-document/CAM-MIPI7251RAW%20UserManual-V1.4.pdf): Comprehensive technical manual.
*   [`ov7251_driver_source_code_pi5_support/`](./ov7251_driver_source_code_pi5_support/): 
    *   [`source_code/`](./ov7251_driver_source_code_pi5_support/source_code/): Kernel driver source code and device tree overlays for Raspberry Pi 5.
*   [`ov7251_mono.json`](./ov7251_mono.json) / [`inno_mipi_ov7251_mono.json`](./inno_mipi_ov7251_mono.json): ISP tuning files for libcamera.

---

## Support

*   **Website**: [www.inno-maker.com](https://www.inno-maker.com)
*   **Email**: [support@inno-maker.com](mailto:support@inno-maker.com) | [sales@inno-maker.com](mailto:sales@inno-maker.com)
