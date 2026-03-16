/*
 * VehicleTrackerScheduled.ino
 * GL868_ESP32 Library - Basic Vehicle Tracker (Scheduled)
 *
 * GPS tracker that sends location data at fixed intervals only.
 * No motion detection - pure scheduled tracking.
 */

#include <GL868_ESP32.h>

// ============================================================================
// Configuration - EDIT THESE VALUES
// ============================================================================
#define DEVICE_ID "YOUR_DEVICE_ID"
#define API_KEY "YOUR_API_KEY"
#define SEND_INTERVAL 300 // Send interval in seconds (5 minutes)

void setup() {
  Serial.begin(115200);

  // Initialize
  GeoLinker.begin(DEVICE_ID, API_KEY);

  // Configure for scheduled-only tracking
  GeoLinker.setSendInterval(SEND_INTERVAL);
  GeoLinker.enableMotionTrigger(false); // Disable motion trigger

  Serial.println("Vehicle Tracker (Scheduled) Started");
  Serial.print("Sending data every ");
  Serial.print(SEND_INTERVAL);
  Serial.println(" seconds");
}

void loop() { GeoLinker.update(); }
