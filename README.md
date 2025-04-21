 ESP32toWaves – HID Keyboard with Web Config Interface
 
Automated Command Input for DIY Waves SoundGrid Servers

This project turns an ESP32-S2 or S3 into a USB HID keyboard that automatically types a custom command into your DIY Waves SoundGrid Server on each boot. It automates startup tasks like relaunching `AG_NSServer` with the correct parameters, ensuring the server correctly detects CPU cores after a power cycle or configuration change.

A built-in Wi-Fi Access Point hosts a browser-based interface where you can update the typed command, delay, mode (HID or Serial), and network settings — all without reprogramming.

Ideal for headless or unattended servers where a consistent initialization routine is required.

## Features

- USB HID keyboard support (ESP32-S2/S3 with native USB)
- Built-in Wi-Fi AP with Web UI for configuration
- Fully customizable startup command with multi-line support
- Configurable:
  - Command execution delay
  - Per-character delay
  - HID or Serial mode
- Built-in actions:
  - Reboot
  - Shutdown
  - Reset ESP32 to defaults
- Save and restore settings via persistent flash storage

## Supported Boards

- ESP32-S2 (native USB)
- ESP32-S3 (native USB)  
Note: Not compatible with ESP32-WROOM/WROVER (these do not support HID natively)

## Hardware Requirements

- ESP32-S2 or ESP32-S3 development board
- Micro-USB or USB-C cable (depending on the board)
- Host device with USB input (PC, Mac, Raspberry Pi, etc.)

## BIOS Configuration (Important)

Ensure that the BIOS setting for USB power during shutdown is disabled.  
This prevents the ESP32 from staying powered when the host is off.

Without this, the ESP32 may execute commands unexpectedly.  
Look for settings such as:
- USB power in S5 state
- ErP Mode
- USB Standby Power

Disable these in your BIOS to ensure proper power cycling behavior.

## Software Requirements

- Arduino IDE (v1.8+ or v2.0+): https://www.arduino.cc/en/software
- ESP32 Arduino Core by Espressif

### Installing the ESP32 Board Support Package

1. Open Arduino IDE → File > Preferences
2. Under Additional Board URLs, paste:
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
3. Go to Tools > Board > Boards Manager, search for `esp32`, and install it

## Arduino IDE Setup

### 1. Select Board and Port

- Board:  
  Go to Tools > Board, and choose:
  - ESP32-S2 Dev Module or ESP32-S3 Dev Module

- Port:  
  Go to Tools > Port, and select the correct COM port.

If the port is not detected:
- Press and hold the BOOT button
- Plug in the USB cable
- Release the BOOT button

This puts the ESP32 in flashing mode.  
You must repeat this process every time if the ESP32 is in HID mode, as it will not expose a serial port automatically.

### 2. Required Libraries

These are included in the ESP32 core:

- WiFi.h
- WebServer.h
- Preferences.h
- USB.h
- USBHIDKeyboard.h

## Uploading the Project

1. Open ESP32toWaves.ino in Arduino IDE  
2. Click Verify to compile  
3. Click Upload to flash the firmware

## First Use

### Default Settings:
- Wi-Fi SSID: ESP32toWaves  
- Password: 12345678  
- Execution Delay: 40 seconds  
- Command:
  killall AG_NSServer
  AG_NSServer -w -W 0-21 -D 22 -C 23

## Web Configuration Interface

1. Connect to the ESP32 Wi-Fi network  
2. Open your browser to http://192.168.4.1

You can configure:
- Command Text  
  Each line is followed by a Return key press.
- Execution Delay  
  Delay after boot before command is typed. This ensures the server is fully ready.
- Character Delay  
  Typing delay between each character (default 30 ms).
- Mode Switching  
  Toggle between Serial and HID mode.
- Immediate Execution / Reboot / Reset  
  Buttons to send commands, reboot, or restore factory settings.

## Editing Commands

- Use line breaks directly in the text box.
- ESP32 will:
  - Wait the configured delay
  - Send an Enter key (optional)
  - Type each character with delay
  - Send Enter at the end

## OTA Firmware Update Support

Not implemented yet, but planned for future versions. Firmware must currently be updated via USB.

## Reset to Factory Defaults

Visit http://192.168.4.1  
Click Reset All Settings

## Limitations

- Only works with native USB ESP32 variants (S2/S3)
- Currently supports AP mode for configuration (STA mode support planned)
- Commands are sent blindly — no feedback from host device is received

## File Structure

ESP32toWaves/
├── ESP32toWaves.ino
├── README.md
├── LICENSE 

## License

Creative Commons Attribution-NonCommercial 4.0 International Public License (CC BY-NC 4.0)

By exercising the Licensed Rights (defined below), You accept and agree to be bound by the terms and conditions of this Creative Commons Attribution-NonCommercial 4.0 International Public License ("Public License").

You are free to:
- Share — copy and redistribute the material in any medium or format
- Adapt — remix, transform, and build upon the material

Under the following terms:
- Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made.
- NonCommercial — You may not use the material for commercial purposes.

No additional restrictions — You may not apply legal terms or technological measures that legally restrict others from doing anything the license permits.

Notices:
You do not have to comply with the license for elements of the material in the public domain or where your use is permitted by an applicable exception or limitation.

No warranties are given. The license may not give you all of the permissions necessary for your intended use. For example, other rights such as publicity, privacy, or moral rights may limit how you use the material.

Full license text available at: https://creativecommons.org/licenses/by-nc/4.0/legalcode

## Contributing

Have ideas, improvements, or bug fixes?  
Open an issue or pull request. Contributions are welcome!

## Future Plans

- Secure web interface (password or token-based access)
- OTA firmware update via browser
- OLED display support for IP and mode display
