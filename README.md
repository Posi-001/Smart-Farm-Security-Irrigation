# Smart Farm: Integrated Irrigation & Security Dashboard

This project is a dual-microcontroller system designed for agricultural automation. It combines a 5-zone smart irrigation system (Arduino) with a perimeter threat detection and web dashboard system (ESP32).

## Features
* **Smart Irrigation:** Monitors 5 soil moisture sensors and automatically routes water to dry zones using a 12V/24V pump and solenoid valves.
* **Perimeter Security:** Monitors Front and Back zones using PIR sensors, triggering escalating alarms (Buzzers and LEDs).
* **Local Web Dashboard:** The ESP32 hosts a local web server over Wi-Fi, allowing you to view real-time security alerts and soil moisture levels from your phone.
* **Inter-Device Communication:** The Arduino actively broadcasts its sensor data to the ESP32 via SoftwareSerial.

## Hardware Required
* 1x ESP32
* 1x Arduino (Uno/Nano/Mega)
* 5x Analog Soil Moisture Sensors
* 2x PIR Motion Sensors
* Relay Modules (Optically isolated recommended)
* 12V/24V Water Pumps and Solenoid Valves
* 1x Buck Converter (Set to exactly 5V)
* Buzzers and LEDs

## Crucial Wiring Notes
1. **Serial Communication:** The ESP32 (TX: Pin 17, RX: Pin 16) talks to the Arduino (TX: Pin 11, RX: Pin 10). *Note: Ensure you use a voltage divider or logic level converter on the Arduino's TX line to protect the ESP32's 3.3V logic.*
2. **Common Ground:** The ESP32 and Arduino MUST share a common GND connection.
3. **Power Management:** Do not power the heavy inductive loads (pumps/solenoids) from the microcontrollers. Route main battery power through a Buck Converter tuned to exactly 5.0V to safely power the Arduino and ESP32 logic independently of the motors.

## Setup Instructions
1. Open the `ESP32_Threat_Dashboard.ino` file and enter your local Wi-Fi `ssid` and `password`.
2. Flash the ESP32 code.
3. Flash the `Arduino_Irrigation_Node.ino` to the Arduino.
4. Open the ESP32 Serial Monitor to find the local IP address, and type that into your phone's web browser to view the dashboard.
