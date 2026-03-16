/*
 * SendSMS.ino
 * GL868_ESP32 Library - Simple SMS Example
 *
 * Demonstrates how to send an SMS message to a phone number.
 */

#include <GL868_ESP32.h>

// ============================================================================
// Configuration - EDIT THESE VALUES
// ============================================================================
#define DEVICE_ID "YOUR_DEVICE_ID"
#define API_KEY "YOUR_API_KEY"
#define PHONE_NUMBER "+91XXXXXXXXXX" // Recipient phone number

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("GL868_ESP32 - Send SMS Example");

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

  // Send SMS
  Serial.print("Sending SMS to: ");
  Serial.println(PHONE_NUMBER);

  if (GeoLinker.sendSMS(PHONE_NUMBER, "Hello from GL868_ESP32!")) {
    Serial.println("SMS sent successfully!");
  } else {
    Serial.println("Failed to send SMS");
  }
}

void loop() {
  // Nothing to do here
  delay(1000);
}
