# GL868_ESP32 - GPS Tracking Library for ESP32-S3 + SIM868

[![GitHub](https://img.shields.io/github/license/Circuit-Digest/GL868_ESP32)](https://github.com/Circuit-Digest/GL868_ESP32/blob/main/LICENSE)
[![GitHub release](https://img.shields.io/github/v/release/Circuit-Digest/GL868_ESP32)](https://github.com/Circuit-Digest/GL868_ESP32/releases)

A comprehensive GPS tracking library for **ESP32-S3** with SIM868 modem and **WS2812B** LED, featuring non-blocking operation, power management, SMS/Call control, motion detection, and offline data queuing.

**Copyright (c) 2026 Jobit Joseph and Semicon Media**

---

## Table of Contents

- [Features](#features)
- [Hardware Requirements](#hardware-requirements-esp32-s3)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Examples](#examples)
- [Configuration Reference](#configuration-reference)
  - [Initialization](#initialization)
  - [Send Interval & GPS Timeout](#send-interval--gps-timeout)
  - [Timezone](#timezone)
  - [APN Configuration](#apn-configuration)
  - [Operating Modes](#operating-modes)
  - [Battery](#battery)
  - [Payload (Custom Data Fields)](#payload-custom-data-fields)
  - [SMS Control](#sms-control)
  - [Call Control](#call-control)
  - [Whitelist](#whitelist)
  - [On-Demand GPS](#on-demand-gps)
  - [Direct AT Commands](#direct-at-commands)
  - [HTTP POST (Custom Endpoints)](#http-post-custom-endpoints)
  - [Motion Sensor](#motion-sensor)
  - [Sleep & Power Management](#sleep--power-management)
  - [Heartbeat](#heartbeat)
  - [Offline Queue](#offline-queue)
  - [LED Control](#led-control)
  - [Logging](#logging)
  - [Status & Information](#status--information)
  - [Manual Control](#manual-control)
  - [Callbacks](#callbacks)
- [LED Status Chart](#led-status-chart)
- [SMS Commands (Built-in)](#sms-commands-built-in)
- [API Endpoint & JSON Format](#api-endpoint--json-format)
- [Dependencies](#dependencies)
- [License](#license)

---

## Features

- **Non-blocking State Machine** – No `delay()` calls, all operations are timer-based
- **Deep Sleep Power Management** – Configurable sleep modes for battery-powered operation
- **GPS Tracking** – Automatic fix acquisition with timeout and retry handling
- **HTTP Data Transmission** – HTTPS support with automatic fallback
- **Offline Data Queue** – RAM or filesystem-based queue for offline data storage
- **SMS Control Plane** – Built-in commands (STATUS, LOC, SEND, SLEEP, INTERVAL, WL)
- **Call Control** – React to incoming calls (e.g., trigger GPS send)
- **Motion Detection** – LIS3DHTR accelerometer support for wake-on-motion
- **WS2812B LED Status** – Addressable RGB LED with power control
- **Whitelist System** – Phone number filtering with NVS persistence
- **Heartbeat Mechanism** – Periodic "alive" messages
- **Simple Payload API** – Easy to use initializer list syntax with multi-type support
- **Operating Modes** – Data-only, SMS-only, Call-only, or any combination
- **On-Demand GPS** – Request GPS location in any operating mode
- **Direct AT Access** – Send AT commands directly from your sketch
- **Custom HTTP POST** – Send data to any URL with custom headers

---

## Hardware Requirements (ESP32-S3)

| Component | Pin | Notes |
|-----------|-----|-------|
| SIM868 TX | GPIO 17 | Fixed, not configurable |
| SIM868 RX | GPIO 18 | Fixed, not configurable |
| SIM868 PWRKEY | GPIO 42 | Controls modem power |
| SIM868 RI | GPIO 16 | Configurable via `setRIPin()` |
| LIS3DHTR SDA | GPIO 8 | I2C data |
| LIS3DHTR SCL | GPIO 9 | I2C clock |
| LIS3DHTR INT | GPIO 2 | Interrupt for wake-on-motion |
| WS2812B Data | GPIO 7 | RGB LED data line |
| WS2812B Power | GPIO 14 | TPS22919DCKR power switch |
| Status LED | GPIO 47 | Onboard status indicator |
| Battery ADC | GPIO 1 | Battery voltage measurement |

---

## Installation

### Arduino IDE

1. Download or clone this repository
2. Copy the `GL868_ESP32` folder to your Arduino libraries folder
3. Install the dependency: **ArduinoJson** (v7.0.0 or higher)
4. Restart Arduino IDE

### PlatformIO

Add to your `platformio.ini`:

```ini
lib_deps =
    https://github.com/Circuit-Digest/GL868_ESP32.git
    bblanchon/ArduinoJson@^7.0.0
```

---

## Quick Start

```cpp
#include <GL868_ESP32.h>

const char* DEVICE_ID = "GPS_TRACKER_001";
const char* API_KEY = "your_api_key_here";

void setup() {
    Serial.begin(115200);
    
    GeoLinker.begin(DEVICE_ID, API_KEY);
    GeoLinker.setSendInterval(300);  // 5 minutes
    GeoLinker.setTimeOffset(5, 30);  // IST +5:30
}

void loop() {
    GeoLinker.update();
}
```

> **Note:** `GeoLinker` is a pre-defined global instance. You only need `#include <GL868_ESP32.h>` to use it.

---

## Examples

The library includes **18 ready-to-use examples**:

### 01. Basic
| Example | Description |
|---------|-------------|
| **MinimalExample** | Simplest possible implementation |
| **BasicTracking** | Minimal GPS tracker with interval sending |
| **SendSMS** | Basic SMS sending |
| **MakeCall** | Basic voice call |
| **SendDataToServer** | HTTP POST with custom headers and response capture |

### 02. Intermediate
| Example | Description |
|---------|-------------|
| **SMSLocationTracker** | SMS commands (LOCATE/WHERE) → location reply |
| **CallForLocation** | Missed call → SMS location (free location request) |
| **ParkingSpotLocator** | Button press → SMS parking location |
| **SpeedAlert** | Speed limit monitoring with SMS/call alerts |
| **SMSHomeAutomation** | GPIO control via SMS (ON/OFF/STATUS) |

### 03. Advanced
| Example | Description |
|---------|-------------|
| **AdvancedTracking** | Full-featured with all configuration options |
| **AdvancedUsageComplete** | Comprehensive reference with all options explained |
| **VehicleTrackerScheduled** | Scheduled interval GPS tracking |
| **VehicleTrackerMotion** | Motion-triggered GPS tracking |
| **AntiTheftTracker** | Theft detection with continuous tracking until disarmed |
| **SOSPanicButton** | Emergency button with continuous tracking until disarmed |
| **RemoteSensorNode** | Custom sensor payload fields |

### 04. Factory
| Example | Description |
|---------|-------------|
| **FactoryFirmware** | SIM activation, NVS storage, API key setup |

### GPS Reliability
All location-sending examples include GPS retry logic:
- **30 second timeout** per attempt
- **3 retries** (90 seconds total max wait)
- Sends even without GPS (for emergency alerts)

---

## Configuration Reference

### Initialization

```cpp
// In setup()
GeoLinker.begin(deviceId, apiKey);

// In loop() - REQUIRED
GeoLinker.update();
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `deviceId` | `const char*` | Device identifier (max 32 chars) |
| `apiKey` | `const char*` | API authentication key (max 32 chars) |

---

### Send Interval & GPS Timeout

```cpp
GeoLinker.setSendInterval(seconds);   // How often to send data
GeoLinker.setGPSTimeout(seconds);     // How long to wait for GPS fix
```

| Function | Default | Range | Description |
|----------|---------|-------|-------------|
| `setSendInterval()` | 300 (5 min) | Any `uint32_t` | Time between data transmissions |
| `setGPSTimeout()` | 120 (2 min) | Any `uint32_t` | Max time to wait for GPS fix before giving up |

---

### Timezone

```cpp
GeoLinker.setTimeOffset(hours, minutes);
```

| Parameter | Type | Range | Example |
|-----------|------|-------|---------|
| `hours` | `int8_t` | -12 to +14 | `5` for IST |
| `minutes` | `int8_t` | 0, 15, 30, 45 | `30` for IST |

---

### APN Configuration

Set **before** `begin()`. If not called, the compile-time default (`iot.com`) is used.

```cpp
// Simple APN (most carriers)
GeoLinker.setAPN("internet");

// APN with authentication
GeoLinker.setAPN("internet.carrier.com", "username", "password");

// Read current APN
const char* apn = GeoLinker.getAPN();
```

| Function | Description |
|----------|-------------|
| `setAPN(apn)` | Set APN name only |
| `setAPN(apn, user, pass)` | Set APN with credentials |
| `getAPN()` | Get current APN string |

---

### Operating Modes

Control which features are active. Set **before** `begin()`.

```cpp
GeoLinker.setOperatingMode(MODE_DATA_ONLY);   // GPS tracking + HTTP only
GeoLinker.setOperatingMode(MODE_SMS_ONLY);    // SMS commands only
GeoLinker.setOperatingMode(MODE_CALL_ONLY);   // Call handling only
GeoLinker.setOperatingMode(MODE_DATA_SMS);    // Data + SMS
GeoLinker.setOperatingMode(MODE_DATA_CALL);   // Data + Call
GeoLinker.setOperatingMode(MODE_SMS_CALL);    // SMS + Call
GeoLinker.setOperatingMode(MODE_FULL);        // All features (default)

// Custom combination using flags
GeoLinker.setOperatingMode(MODE_DATA | MODE_CALL);

// Check mode
uint8_t mode = GeoLinker.getOperatingMode();
bool smsOn = GeoLinker.isModeEnabled(MODE_SMS);
```

**Available flags:**

| Flag | Value | Description |
|------|-------|-------------|
| `MODE_DATA` | `0x01` | GPS tracking + HTTP data transmission |
| `MODE_SMS` | `0x02` | SMS command processing |
| `MODE_CALL` | `0x04` | Incoming call handling |

**Pre-defined combinations:**

| Mode | Value | Description |
|------|-------|-------------|
| `MODE_DATA_ONLY` | `0x01` | Data only |
| `MODE_SMS_ONLY` | `0x02` | SMS only |
| `MODE_CALL_ONLY` | `0x04` | Call only |
| `MODE_DATA_SMS` | `0x03` | Data + SMS |
| `MODE_DATA_CALL` | `0x05` | Data + Call |
| `MODE_SMS_CALL` | `0x06` | SMS + Call |
| `MODE_FULL` | `0x07` | All features (default) |

**Mode behavior:**
- **Non-data modes** (SMS/Call only): After GSM registration, the device enters an **IDLE** state listening for SMS/Calls instead of starting the GPS→HTTP pipeline.
- **Combined modes** (Data + SMS/Call): After sending data, the device enters IDLE to listen for SMS/Calls instead of immediately sleeping.

---

### Battery

```cpp
// Set voltage range for percentage calculation
GeoLinker.setBatteryRange(3200, 4200);  // minMV, maxMV (default: 3000-4200)

// Set battery reading source
GeoLinker.setBatterySource(BATTERY_SOURCE_ADC);    // ESP32 ADC pin (default)
GeoLinker.setBatterySource(BATTERY_SOURCE_MODEM);  // SIM868 AT+CBC command
```

| Source | Description |
|--------|-------------|
| `BATTERY_SOURCE_ADC` | Reads from ESP32 ADC (GPIO 1) – default |
| `BATTERY_SOURCE_MODEM` | Uses SIM868's built-in battery measurement |

---

### Payload (Custom Data Fields)

Add custom sensor data to each GPS transmission. Supports **float, int, string, and bool** types.

```cpp
// Set payloads (clears existing and sets new)
GeoLinker.setPayloads({
    {"temperature", 25.5},      // float/double
    {"humidity", 65.0},         // float
    {"count", 42},              // int
    {"status", "active"},       // string (max 32 chars)
    {"enabled", true}           // bool
});

// Clear all payloads
GeoLinker.clearPayloads();
// or
GeoLinker.setPayloads({});

// Legacy method (array-based)
PayloadField fields[2] = { {"temp", 25.5f}, {"hum", 60.0f} };
GeoLinker.setPayloadFields(fields, 2);
```

| Constraint | Value |
|------------|-------|
| Max payload keys | 5 |
| Max key length | 16 chars |
| Max string value length | 32 chars |

---

### SMS Control

```cpp
// Enable/disable SMS processing
GeoLinker.enableSMS(true);

// Enable SMS monitoring during sleep (keeps modem in low-power mode)
GeoLinker.enableSMSMonitoring(true);

// Send an SMS
GeoLinker.sendSMS("+1234567890", "Hello from GPS Tracker!");

// Register custom SMS command handler
GeoLinker.registerSMSHandler("ALERT", [](const char* sender, const char* cmd, const char* args) {
    Serial.printf("ALERT command from %s, args: %s\n", sender, args);
    GeoLinker.sendSMS(sender, "Alert acknowledged!");
});
```

| Function | Description |
|----------|-------------|
| `enableSMS(bool)` | Enable/disable SMS command processing |
| `enableSMSMonitoring(bool)` | Keep modem in low-power mode during sleep to receive SMS |
| `sendSMS(number, message)` | Send an SMS (max 160 chars) |
| `registerSMSHandler(cmd, handler)` | Register custom command handler (max 10 handlers) |

**SMS Handler callback signature:**
```cpp
void handler(const char* sender, const char* command, const char* args);
```

---

### Call Control

```cpp
// Enable/disable call processing
GeoLinker.enableCalls(true);

// Enable call monitoring during sleep
GeoLinker.enableCallMonitoring(true);

// Set default action on incoming call
GeoLinker.setCallAction(CALL_SEND_GPS);   // Trigger GPS send (default)
GeoLinker.setCallAction(CALL_HANGUP);     // Just hang up
GeoLinker.setCallAction(CALL_CUSTOM);     // Only custom handlers

// Register custom call handler
GeoLinker.registerCallHandler([](const char* caller) {
    Serial.printf("Call from: %s\n", caller);
});

// Make an outgoing call
if (GeoLinker.makeCall("+1234567890", 30)) {  // 30 sec timeout
    Serial.println("Call connected");
    delay(5000);
    GeoLinker.hangupCall();
}

// Answer incoming call
GeoLinker.answerCall();

// Hang up current call
GeoLinker.hangupCall();

// Check if call is active
if (GeoLinker.isCallActive()) {
    Serial.println("Call in progress");
}
```

**Call actions:**

| Action | Description |
|--------|-------------|
| `CALL_HANGUP` | Hang up immediately |
| `CALL_SEND_GPS` | Trigger immediate GPS data send (default) |
| `CALL_CUSTOM` | Only run custom handlers |

**Call Handler callback signature:**
```cpp
void handler(const char* caller);
```

---

### Whitelist

Phone number filtering with NVS persistence. When the whitelist is **empty**, all numbers are allowed. The **first number added** becomes the admin.

```cpp
// Managed via SMS commands (see SMS Commands section)
// Or access the whitelist object directly:
GeoLinker.whitelist.add("+1234567890");
GeoLinker.whitelist.remove("+1234567890");
GeoLinker.whitelist.clear();
bool allowed = GeoLinker.whitelist.isAllowed("+1234567890");
bool admin = GeoLinker.whitelist.isAdmin("+1234567890");
uint8_t count = GeoLinker.whitelist.count();
const char* num = GeoLinker.whitelist.get(0);  // Get by index
```

| Constraint | Value |
|------------|-------|
| Max whitelist entries | 20 |
| Max phone number length | 16 chars |
| Storage | NVS (persistent across reboots) |

---

### On-Demand GPS

Available in **all operating modes**, not just data mode.

```cpp
// Blocking: powers GPS on, waits for fix, powers GPS off
GPSData location;
if (GeoLinker.getLocation(&location, 90000)) {  // 90 sec timeout
    Serial.printf("Lat: %f, Lon: %f\n", location.latitude, location.longitude);
    Serial.printf("Alt: %.1fm, Speed: %.1fkm/h\n", location.altitude, location.speed);
    Serial.printf("Sats: %d, Time: %s\n", location.satellites, location.timestamp);
}

// Manual GPS control (for non-blocking use)
GeoLinker.gpsOn();                       // Power on GPS
bool powered = GeoLinker.isGpsPowered(); // Check power state
GeoLinker.getLocationNow(&location);     // Non-blocking read (may be invalid)
GeoLinker.gpsOff();                      // Power off GPS
```

**GPSData structure:**

| Field | Type | Description |
|-------|------|-------------|
| `valid` | `bool` | True if fix is valid |
| `latitude` | `double` | Latitude in degrees |
| `longitude` | `double` | Longitude in degrees |
| `altitude` | `float` | Altitude in meters |
| `speed` | `float` | Speed in km/h |
| `heading` | `float` | Heading in degrees |
| `accuracy` | `float` | HDOP accuracy |
| `satellites` | `uint8_t` | Number of satellites |
| `timestamp` | `char[20]` | `"YYYY-MM-DD HH:MM:SS"` |

---

### Direct AT Commands

Send raw AT commands to the SIM868 modem.

```cpp
// Send command, check for OK
if (GeoLinker.sendATCommand("AT+CSQ")) {
    Serial.println("Command OK");
}

// Send command with timeout
GeoLinker.sendATCommand("AT+COPS?", 5000);

// Send command and capture response
char response[256];
GeoLinker.sendATCommand("AT+COPS?", response, sizeof(response));
Serial.println(response);

// Get last response
String resp = GeoLinker.getATResponse();

// Direct serial access (advanced)
HardwareSerial& modem = GeoLinker.getModemSerial();
```

> **Warning:** Direct AT commands bypass the library's state management. Use with caution.

---

### HTTP POST (Custom Endpoints)

Send HTTP POST requests to any URL with custom headers.

```cpp
// Simple POST
int httpCode = GeoLinker.httpPost(
    "http://httpbin.org/post",
    "{\"key\":\"value\"}"
);
Serial.printf("HTTP %d\n", httpCode);

// POST with custom headers
int httpCode = GeoLinker.httpPost(
    "http://api.example.com/data",
    "{\"temp\":25.5}",
    "Content-Type: application/json\r\nAuthorization: Bearer mytoken"
);

// POST with response capture
char response[512];
int httpCode = GeoLinker.httpPost(
    "http://api.example.com/data",
    "{\"temp\":25.5}",
    "Content-Type: application/json",
    response, sizeof(response)
);
Serial.printf("HTTP %d: %s\n", httpCode, response);
```

| Return | Meaning |
|--------|---------|
| `200` | Success |
| `400-499` | Client error |
| `500-599` | Server error |
| `-1` | Connection error |

---

### Motion Sensor

LIS3DHTR accelerometer for motion-triggered tracking.

```cpp
GeoLinker.enableMotionTrigger(true);       // Enable motion-triggered sends
GeoLinker.setMotionThreshold(300.0);       // Threshold in milligrams (recommended)
GeoLinker.enableMotionWake(true);          // Wake from deep sleep on motion

// Or use raw register value (advanced)
GeoLinker.setMotionSensitivity(40);        // 0-127, lower = more sensitive
```

**Recommended threshold values (milligrams):**

| Use Case | Threshold | Description |
|----------|-----------|-------------|
| Asset in transit | 300-500mg | Detect handling/movement |
| Parked vehicle | 200-400mg | Detect towing/theft |
| Equipment monitoring | 400-800mg | Detect usage/vibration |

| Function | Default | Description |
|----------|---------|-------------|
| `enableMotionTrigger(bool)` | `false` | Send data immediately when motion detected |
| `setMotionThreshold(mg)` | `640mg` | Threshold in milligrams (16-2032mg) |
| `setMotionSensitivity(threshold)` | `40` | Raw register: 0-127, lower = more sensitive |
| `enableMotionWake(bool)` | `false` | Wake ESP32 from deep sleep on motion interrupt |

---

### Sleep & Power Management

```cpp
// Full modem power off during sleep (saves max power)
GeoLinker.enableFullPowerOff(true);     // Default: true

// SIM868 low-power mode (when not fully powered off)
GeoLinker.setSIM868SleepMode(SIM868_CFUN0);   // Minimum functionality
GeoLinker.setSIM868SleepMode(SIM868_CFUN4);   // Disable RF only

// RI pin for SMS/Call wake detection
GeoLinker.setRIPin(16);  // Default: GPIO 16
```

**SIM868 sleep modes:**

| Mode | AT Command | Description |
|------|-----------|-------------|
| `SIM868_FULL_OFF` | PWRKEY off | Complete power off (default for sleep >5 min) |
| `SIM868_CFUN0` | `AT+CFUN=0` | Minimum functionality, no RF |
| `SIM868_CFUN4` | `AT+CFUN=4` | RF disabled, functions available |

> **Note:** When sleep interval > 5 minutes and `enableFullPowerOff(true)` is set, the modem is fully powered off to save battery.

---

### Heartbeat

Periodic "alive" signal to the server.

```cpp
GeoLinker.enableHeartbeat(true);
GeoLinker.setHeartbeatInterval(3600);  // Every 1 hour (default)
```

| Function | Default | Description |
|----------|---------|-------------|
| `enableHeartbeat(bool)` | `false` | Enable periodic heartbeat |
| `setHeartbeatInterval(seconds)` | `3600` (1 hr) | Time between heartbeat signals |

---

### Offline Queue

Store data locally when the server is unreachable.

```cpp
GeoLinker.setQueueStorage(QUEUE_RAM, 10);       // RAM storage (default)
GeoLinker.setQueueStorage(QUEUE_LITTLEFS, 50);  // Persistent filesystem storage
GeoLinker.setQueueStorage(QUEUE_SPIFFS, 50);    // Legacy SPIFFS storage
```

| Storage | Default Max | Persistent | Description |
|---------|-------------|------------|-------------|
| `QUEUE_RAM` | 10 entries | No | Lost on reboot |
| `QUEUE_LITTLEFS` | 100 entries | Yes | Survives reboots, recommended |
| `QUEUE_SPIFFS` | 100 entries | Yes | Legacy, use LittleFS instead |

---

### LED Control

```cpp
// WS2812B RGB power control (via TPS22919DCKR switch)
GeoLinker.setLEDPower(true);    // Power on WS2812B
GeoLinker.setLEDPower(false);   // Power off WS2812B (saves power)

// User Status LED control (GPIO 47)
// This LED is fully user-controllable and NOT used by the library after boot.
GeoLinker.turnOnLED();          // Turn on the onboard status LED
GeoLinker.turnOffLED();         // Turn off the onboard status LED
```

---

### Logging

```cpp
GeoLinker.setLogLevel(LOG_OFF);      // No output
GeoLinker.setLogLevel(LOG_ERROR);    // Errors only
GeoLinker.setLogLevel(LOG_INFO);     // Info + errors (default)
GeoLinker.setLogLevel(LOG_DEBUG);    // Debug + info + errors
GeoLinker.setLogLevel(LOG_VERBOSE);  // Everything
```

| Level | Value | Output Prefix | Description |
|-------|-------|---------------|-------------|
| `LOG_OFF` | 0 | – | Silent |
| `LOG_ERROR` | 1 | `[E][GLESP]` | Errors only |
| `LOG_INFO` | 2 | `[I][GLESP]` | Normal operation info |
| `LOG_DEBUG` | 3 | `[D][GLESP]` | Detailed debug info |
| `LOG_VERBOSE` | 4 | `[V][GLESP]` | Everything |

---

### Status & Information

```cpp
// System state
SystemState state = GeoLinker.getState();
WakeSource wake = GeoLinker.getWakeSource();
bool scheduled = GeoLinker.isScheduledWake();
bool motionWake = GeoLinker.isMotionWake();

// Battery
uint8_t battery = GeoLinker.getBatteryPercent();   // 0-100%
uint16_t voltage = GeoLinker.getBatteryVoltageMV(); // millivolts

// Network
int signal = GeoLinker.getSignalStrength();  // 0-31 (GSM signal quality)
String op = GeoLinker.getOperator();          // Carrier name

// GPS
const GPSData& gps = GeoLinker.getLastGPS();
uint8_t sats = GeoLinker.getSatelliteCount();

// SIM/Modem
String imei = GeoLinker.getIMEI();          // 15-digit modem IMEI
String imsi = GeoLinker.getIMSI();          // SIM IMSI
String iccid = GeoLinker.getICCID();        // 19-digit SIM ICCID
String phone = GeoLinker.getPhoneNumber();  // Phone number (if stored on SIM)
```

**System states:**

| State | Description |
|-------|-------------|
| `STATE_BOOT` | Initial boot |
| `STATE_MODEM_POWER_ON` | Powering on SIM868 |
| `STATE_GSM_INIT` | Initializing GSM |
| `STATE_GSM_REGISTER` | Registering on network |
| `STATE_GPS_POWER_ON` | Powering on GPS |
| `STATE_GPS_WAIT_FIX` | Waiting for GPS fix |
| `STATE_BUILD_JSON` | Building JSON payload |
| `STATE_GPRS_ATTACH` | Attaching to GPRS |
| `STATE_SEND_HTTP` | Sending HTTP data |
| `STATE_SEND_OFFLINE_QUEUE` | Sending queued offline data |
| `STATE_SLEEP_PREPARE` | Preparing for sleep |
| `STATE_SLEEP` | Deep sleep |
| `STATE_IDLE` | Listening for SMS/Call (non-data modes) |

**Wake sources:**

| Source | Description |
|--------|-------------|
| `WAKE_UNKNOWN` | Unknown wake source |
| `WAKE_TIMER` | Scheduled timer wake |
| `WAKE_MOTION` | Motion sensor interrupt |
| `WAKE_CALL` | Incoming call (via RI pin) |
| `WAKE_SMS` | Incoming SMS (via RI pin) |

---

### Manual Control

```cpp
GeoLinker.forceSend();   // Force immediate data transmission
GeoLinker.forceSleep();  // Force immediate sleep
```

---

### Callbacks

```cpp
// State change notification
GeoLinker.onStateChange([](SystemState from, SystemState to) {
    Serial.printf("State: %d -> %d\n", from, to);
});

// Error notification
GeoLinker.onError([](ErrorCode code) {
    Serial.printf("Error: %d\n", code);
});

// Data sent notification
GeoLinker.onDataSent([](bool success, int httpCode) {
    Serial.printf("Data sent: %s (HTTP %d)\n", success ? "OK" : "FAIL", httpCode);
});
```

**Error codes:**

| Code | Description |
|------|-------------|
| `ERROR_NONE` | No error |
| `ERROR_NO_SIM` | SIM card not detected |
| `ERROR_NO_NETWORK` | Network registration failed |
| `ERROR_NO_GPRS` | GPRS attach failed |
| `ERROR_NO_GPS` | GPS fix failed after retries |
| `ERROR_HTTP_FAIL` | HTTP send failed after retries |
| `ERROR_MODEM` | Modem communication error |
| `ERROR_MOTION_SENSOR` | Motion sensor init failed |
| `ERROR_QUEUE_FULL` | Offline queue is full |
| `ERROR_JSON_BUILD` | JSON build error |
| `ERROR_TIMEOUT` | General timeout |

---

## LED Status Chart

All timing and blink loops are configurable in `GL868_ESP32_Config.h`.
The user status LED (GPIO 47) is free for your own application via `GeoLinker.turnOnLED()`/`turnOffLED()`.

### Normal Operation (WS2812B RGB)

| State | Color | Pattern | Description |
|-------|-------|---------|-------------|
| BOOT | � Yellow | 0.5 Hz blink | System starting up |
| MODEM_POWER_ON | 🟡 Yellow | 0.5 Hz blink | Powering on SIM868 |
| GSM_INIT | � Magenta | 0.5 Hz blink | Initializing GSM |
| GSM_REGISTER | � Magenta | 0.5 Hz blink | Waiting for network registration |
| GPS_POWER_ON | � Blue | 0.5 Hz blink | GPS powering on |
| GPS_WAIT_FIX | � Blue | 0.5 Hz blink | Acquiring GPS fix |
| BUILD_JSON | ⚪ White | 0.5 Hz blink | Building data payload |
| GPRS_ATTACH | ⚪ White | 0.5 Hz blink | Attaching to GPRS |
| SEND_HTTP | ⚪ White | 0.5 Hz blink | Sending data to server |
| SEND_OFFLINE | 🩷 Pink | 0.5 Hz blink | Sending offline queue |
| IDLE | 🩷 Pink | 0.5 Hz blink | Listening for SMS/Call |
| SLEEP | ⚫ Off | Off | Deep sleep |

*(Blink pattern: ON for 250ms, OFF for 1750ms)*

### Error Indicators (🔴 Red RGB)

Triggered when retries are exhausted. Error patterns blink a specific number of times, pause for 1 second, and repeat sequence 5 times before sleep.

| Error | Blinks |
|-------|--------|
| NO_SIM | 1 blink |
| NO_NETWORK | 2 blinks |
| NO_GPRS | 3 blinks |
| NO_GPS | 4 blinks |
| HTTP_FAIL | 5 blinks |
| MODEM | Solid |

*(Blinks are 200ms ON / 200ms OFF. Solid is 2000ms ON / 1000ms OFF)*


---

## SMS Commands (Built-in)

Send these commands via SMS to control the device:

| Command | Description | Example Reply |
|---------|-------------|---------------|
| `STATUS` | Get device status | Battery: 85%, Signal: 18, Sats: 12 |
| `LOC` | Get GPS coordinates | Location: https://www.google.com/maps/@11.009,77.014,16z |
| `SEND` | Force immediate data send | Sending... |
| `SLEEP` | Enter sleep mode | Going to sleep |
| `INTERVAL=xx` | Set send interval (minutes) | Interval set to 10 min |
| `WL ADD <num>` | Add phone to whitelist | Added: +1234567890 |
| `WL DEL <num>` | Remove from whitelist | Removed: +1234567890 |
| `WL LIST` | List all whitelisted numbers | 1: +1234..., 2: +5678... |
| `WL CLEAR` | Clear entire whitelist | Whitelist cleared |

> **Whitelist behavior:** When the whitelist is empty, **all numbers** are accepted. Once a number is added, only whitelisted numbers can send commands.

---

## API Endpoint & JSON Format

### Default Endpoint

| Protocol | URL |
|----------|-----|
| HTTPS (default) | `https://www.circuitdigest.cloud/api/v1/geolinker` |
| HTTP fallback | `http://www.circuitdigest.cloud/api/v1/geolinker` |

### JSON Request Format

```json
{
  "device_id": "GPS_TRACKER_001",
  "timestamp": ["2024-01-05 10:00:00"],
  "lat": [11.010915],
  "long": [77.013209],
  "altitude": [920.5],
  "speed": [0.0],
  "heading": [135.0],
  "accuracy": [1.2],
  "satellites": [12],
  "signal_strength": [18],
  "battery": [85],
  "payload": [{"temperature": 25.5, "humidity": 65}]
}
```

---

## Dependencies

- [ArduinoJson](https://arduinojson.org/) v7.0.0 or higher

---

## License

MIT License – See LICENSE file for details.

Copyright (c) 2026 Jobit Joseph and Semicon Media

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

For architecture details and internal design, see [DEVELOPER.md](DEVELOPER.md).

## Repository

[https://github.com/Circuit-Digest/GL868_ESP32](https://github.com/Circuit-Digest/GL868_ESP32)
