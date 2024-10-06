
# SmartPlantCare

**SmartPlantCare** is an automated plant watering system powered by ESP32 with a web interface for configuration and real-time monitoring, soil moisture sensors, SD card logging, and optional remote control through Blynk. The system uses **ESP32 T-Display-S3** for displaying data and **LILYGO® T-Display-S3 TF Shield** for SD card handling.

## Features
- Automatic irrigation based on soil moisture levels
- Hourly data logging to an SD card
- Real-time data display on **ESP32 T-Display-S3**
- Web interface for configuration and monitoring at `http://esp32sd.local`
- Remote control via Blynk app (optional)

## Components
- **ESP32 T-Display-S3**
- **LILYGO® T-Display-S3 TF Shield** (for SD card functionality)
- Soil Moisture Sensor
- Water Pump
- Relay Module
- Jumper wires
- Power supply (5V)
- Blynk App (optional)

## Hardware Setup

### 1. ESP32 T-Display-S3
- The **ESP32 T-Display-S3** includes a built-in display for real-time data such as soil moisture, watering status, and date/time.

### 2. LILYGO® T-Display-S3 TF Shield (SD Card Module)
- This shield is used for logging data to an SD card.
- **Wiring**:
  - MISO to GPIO 19
  - MOSI to GPIO 23
  - SCK to GPIO 18
  - CS to GPIO 5

### 3. Soil Moisture Sensor
- VCC to 3.3V (or 5V depending on the sensor)
- GND to GND
- Data to ESP32 analog pin (e.g., A0)

### 4. Water Pump & Relay Module
- Relay VCC to 5V
- Relay GND to GND
- IN to GPIO pin (e.g., GPIO 13)
- Water pump connected to relay (NO, COM)

## Software Setup

### 1. Install Required Libraries
- Open Arduino IDE and install the following libraries:
  - `TFT_eSPI` (for the **ESP32 T-Display-S3** screen)
  - `Blynk` (for remote control via Blynk)
  - `SD` (for handling SD card operations)
  - `ESPAsyncWebServer` (for the web interface)
  - `AsyncTCP` (for handling asynchronous web requests)

### 2. Clone the Repository
- Clone the repository and open `SmartPlantCare.ino` in Arduino IDE:
  ```bash
  git clone https://github.com/Acigap/SmartPlantCare.git
  ```

### 3. Web Interface Configuration
- The system includes a web interface for real-time monitoring and configuration. It allows you to:
  - View current soil moisture levels
  - Manually start or stop the water pump
  - Adjust soil moisture thresholds for automatic watering

**Web Interface Setup**:
- The ESP32 hosts a web server accessible at `http://esp32sd.local`.
- Ensure your ESP32 is connected to the same Wi-Fi network. Open a web browser and type `http://esp32sd.local` to access the web interface.

### 4. Blynk Setup (Optional)
- Install the Blynk app from the app store.
- Create a new project in the Blynk app and configure it to display soil moisture levels and control the water pump.

### 5. Upload the Code
- In Arduino IDE, select the correct board (`ESP32 T-Display-S3`) and port.
- Upload the code to your ESP32 board.

### 6. SD Card Logging
- The system logs data to a CSV file on the SD card every hour.
- The CSV log includes a timestamp, soil moisture level, and any watering actions.

## Usage

1. **Initial Setup**:
   - Place the soil moisture sensor in the plant’s soil.
   - Connect the water pump and ensure the SD card is inserted.

2. **Display**:
   - Real-time data, such as soil moisture percentage, system status, and last watering time, is displayed on the **ESP32 T-Display-S3**.

3. **Web Interface**:
   - Access the web interface by typing `http://esp32sd.local` into a browser.
   - Use the web interface to configure settings, monitor soil moisture levels, and manually control the water pump.

4. **Blynk** (optional):
   - Use the Blynk app to remotely monitor and control the system.

5. **SD Card**:
   - The system logs data on the SD card in CSV format.
   - You can download the logs from the web interface.

## Wiring Diagram

*Include a diagram showing how to connect the components to ESP32.*

## Troubleshooting

- **Display not working**: Ensure the `TFT_eSPI` library is properly configured for **ESP32 T-Display-S3**.
- **Web interface not accessible**: Verify that the ESP32 is connected to Wi-Fi and try pinging `esp32sd.local` to check connectivity.
- **SD card issues**: Ensure the SD card is properly inserted and formatted correctly.

## License
This project is licensed under the MIT License.
