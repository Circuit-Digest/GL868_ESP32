/*
 * BasicTracking.ino
 * Basic GPS tracking example for GL868_ESP32 library
 *
 * This example demonstrates the minimal setup required for GPS tracking.
 * The device will:
 * 1. Wake from deep sleep
 * 2. Power on modem
 * 3. Get GPS fix
 * 4. Send data to server
 * 5. Go back to deep sleep
 *
 * Hardware:
 * - ESP32
 * - SIM868 module
 * - LIS3DHTR accelerometer (optional)
 * - SIM card with data plan
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#include <GL868_ESP32.h>

// ============================================================================
// Configuration - EDIT THESE
// ============================================================================

const char *DEVICE_ID = "GPS_TRACKER_001";
const char *API_KEY = "YOUR_API_KEY_HERE"; // Replace with your API key

// ============================================================================
// Setup
// ============================================================================

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println("==========================================");
  Serial.println("GL868_ESP32 Basic Tracking Example");
  Serial.println("==========================================");

  // Initialize the library
  GeoLinker.begin(DEVICE_ID, API_KEY);

  // Set send interval (5 minutes = 300 seconds)
  GeoLinker.setSendInterval(300);

  // Set timezone offset (IST = +5:30)
  GeoLinker.setTimeOffset(5, 30);

  // Battery range is optional - default is 3200mV to 4200mV
  // Uncomment to customize:
  // GeoLinker.setBatteryRange(3000, 4200);

  // Battery source is optional - default is ADC (GPIO35)
  // Use modem for AT+CBC reading:
  // GeoLinker.setBatterySource(BATTERY_SOURCE_MODEM);

  Serial.println("Setup complete - entering main loop");
}

// ============================================================================
// Main Loop
// ============================================================================

void loop() {
  // Call update() every loop - this handles all operations non-blocking
  GeoLinker.update();
}
