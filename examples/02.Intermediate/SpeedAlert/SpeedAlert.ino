/*
 * SpeedAlert.ino
 * GL868_ESP32 Library - Speed Alert
 *
 * Monitors GPS speed and sends alert when traveling above a set limit.
 * Sends both SMS and makes a voice call when speed exceeds threshold.
 * GPS retry with timeout and configurable retries.
 */

#include <GL868_ESP32.h>

// ============================================================================
// Configuration - EDIT THESE VALUES
// ============================================================================
#define DEVICE_ID "YOUR_DEVICE_ID"
#define API_KEY "YOUR_API_KEY"
#define ALERT_NUMBER "+91XXXXXXXXXX" // Number for alerts

#define SPEED_LIMIT 80.0      // Speed limit in km/h
#define CHECK_INTERVAL 5000   // Check speed every 5 seconds
#define ALERT_COOLDOWN 300000 // Don't repeat alerts for 5 minutes

// GPS retry configuration
#define GPS_TIMEOUT 30000 // 30 seconds per attempt
#define GPS_MAX_RETRIES 3 // Number of retries

// ============================================================================
// Global Variables
// ============================================================================
uint32_t lastCheck = 0;
uint32_t lastAlert = 0;
bool alertActive = false;

void setup() {
  Serial.begin(115200);

  // Initialize
  GeoLinker.begin(DEVICE_ID, API_KEY);

  // Enable regular tracking
  GeoLinker.setSendInterval(60); // Send data every minute

  Serial.println("Speed Alert System Started");
  Serial.print("Speed limit set to: ");
  Serial.print(SPEED_LIMIT);
  Serial.println(" km/h");
}

void loop() {
  GeoLinker.update();

  // Check speed periodically
  if (millis() - lastCheck >= CHECK_INTERVAL) {
    lastCheck = millis();
    checkSpeed();
  }
}

// ============================================================================
// Get Valid GPS Location with Retry
// ============================================================================
bool getValidLocation(GPSData *gps) {
  for (int retry = 0; retry < GPS_MAX_RETRIES; retry++) {
    Serial.printf("  GPS attempt %d/%d\n", retry + 1, GPS_MAX_RETRIES);

    uint32_t start = millis();
    while (millis() - start < GPS_TIMEOUT) {
      if (GeoLinker.getLocationNow(gps) && gps->valid) {
        return true;
      }
      delay(1000);
    }
  }
  return false;
}

// ============================================================================
// Check Speed
// ============================================================================
void checkSpeed() {
  GPSData gps;

  // Quick check first (non-blocking)
  if (!GeoLinker.getLocationNow(&gps) || !gps.valid) {
    Serial.println("No quick GPS fix, skipping speed check");
    return;
  }

  float currentSpeed = gps.speed; // Speed in km/h

  Serial.print("Current speed: ");
  Serial.print(currentSpeed, 1);
  Serial.println(" km/h");

  if (currentSpeed > SPEED_LIMIT) {
    // Check cooldown
    if (!alertActive || (millis() - lastAlert >= ALERT_COOLDOWN)) {
      // Before sending alert, get more accurate location with retries
      Serial.println("Speed exceeded! Getting accurate location...");
      if (getValidLocation(&gps)) {
        triggerSpeedAlert(gps.speed, &gps);
      } else {
        // Still trigger alert with last known data
        triggerSpeedAlert(currentSpeed, &gps);
      }
    }
  } else {
    alertActive = false;
  }
}

// ============================================================================
// Trigger Speed Alert
// ============================================================================
void triggerSpeedAlert(float speed, GPSData *gps) {
  Serial.println("\n!!! SPEED ALERT !!!");
  Serial.print("Speed: ");
  Serial.print(speed, 1);
  Serial.print(" km/h (Limit: ");
  Serial.print(SPEED_LIMIT);
  Serial.println(" km/h)");

  alertActive = true;
  lastAlert = millis();

  // Build alert message
  char message[250];
  if (gps->valid) {
    snprintf(message, sizeof(message),
             "SPEED ALERT!\n"
             "Current: %.1f km/h\n"
             "Limit: %.1f km/h\n"
             "Location:\n"
             "https://maps.google.com/?q=%.6f,%.6f",
             speed, SPEED_LIMIT, gps->latitude, gps->longitude);
  } else {
    snprintf(message, sizeof(message),
             "SPEED ALERT!\n"
             "Current: %.1f km/h\n"
             "Limit: %.1f km/h\n"
             "Location: No GPS fix",
             speed, SPEED_LIMIT);
  }

  // Send SMS alert
  Serial.println("Sending SMS alert...");
  GeoLinker.sendSMS(ALERT_NUMBER, message);

  // Make voice call (30 second timeout)
  Serial.println("Making alert call...");
  GeoLinker.makeCall(ALERT_NUMBER, 30);

  // Also log to cloud
  // Data will be sent via GeoLinker.update() automatically
}
