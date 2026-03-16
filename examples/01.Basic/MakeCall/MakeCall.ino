/*
 * MakeCall.ino
 * GL868_ESP32 Library - Simple Call Example
 *
 * Demonstrates how to make a voice call to a phone number.
 */

#include <GL868_ESP32.h>

// ============================================================================
// Configuration - EDIT THESE VALUES
// ============================================================================
#define DEVICE_ID "YOUR_DEVICE_ID"
#define API_KEY "YOUR_API_KEY"
#define PHONE_NUMBER "+91XXXXXXXXXX" // Number to call

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("GL868_ESP32 - Make Call Example");

  // Initialize
  GeoLinker.begin(DEVICE_ID, API_KEY);

  // Power on modem
  Serial.println("Powering on modem...");
  if (!GeoLinker.modem.powerOn()) {
    Serial.println("Failed to power on modem!");
    while (1)
      delay(1000);
  }
  Serial.println("Modem ready!");

  // Wait for network registration
  Serial.print("Waiting for network");
  while (!GeoLinker.gsm.waitNetworkRegistration(10000)) {
    Serial.print(".");
  }
  Serial.println("\nNetwork registered!");

  // Make call
  Serial.print("Calling: ");
  Serial.println(PHONE_NUMBER);

  // makeCall(number, timeout_seconds)
  // timeout = how long to wait for answer (0 = don't wait)
  if (GeoLinker.makeCall(PHONE_NUMBER, 30)) {
    Serial.println("Call connected!");

    // Wait some time then hang up
    delay(10000); // 10 seconds
    GeoLinker.hangupCall();
    Serial.println("Call ended");
  } else {
    Serial.println("Call failed or not answered");
  }
}

void loop() {
  // Handle incoming calls if any
  GeoLinker.update();
}
