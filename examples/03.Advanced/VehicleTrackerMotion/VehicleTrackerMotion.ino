/*
 * VehicleTrackerMotion.ino
 * GL868_ESP32 Library - Basic Vehicle Tracker (Motion)
 *
 * GPS tracker that sends location data only when motion is detected.
 * No scheduled interval - pure motion-based tracking.
 */

#include <GL868_ESP32.h>

// ============================================================================
// Configuration - EDIT THESE VALUES
// ============================================================================
#define DEVICE_ID "YOUR_DEVICE_ID"
#define API_KEY "YOUR_API_KEY"
#define MOTION_THRESHOLD_MG                                                    \
  300.0 // Motion threshold in milligrams (200-500mg typical)

void setup() {
  Serial.begin(115200);

  // Initialize
  GeoLinker.begin(DEVICE_ID, API_KEY);

  // Configure for motion-only tracking
  GeoLinker.setSendInterval(0);        // Disable scheduled sending
  GeoLinker.enableMotionTrigger(true); // Enable motion trigger
  GeoLinker.setMotionThreshold(MOTION_THRESHOLD_MG);
  GeoLinker.enableMotionWake(true); // Wake from sleep on motion

  Serial.println("Vehicle Tracker (Motion) Started");
  Serial.println("Data will be sent when motion is detected");
}

void loop() { GeoLinker.update(); }
