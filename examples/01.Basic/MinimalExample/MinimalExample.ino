/*
 * MinimalExample.ino
 * GL868_ESP32 Library - Minimal Example
 *
 * Simplest possible GPS tracker with both interval and motion triggers.
 * API key is hardcoded in the sketch.
 */

#include <GL868_ESP32.h>

// ============================================================================
// Configuration - EDIT THESE VALUES
// ============================================================================
#define DEVICE_ID "YOUR_DEVICE_ID"
#define API_KEY "YOUR_API_KEY"

void setup() {
  Serial.begin(115200);

  // Initialize with device ID and API key
  GeoLinker.begin(DEVICE_ID, API_KEY);

  // Enable interval + motion (default behavior)
  GeoLinker.setSendInterval(300);      // Send every 5 minutes
  GeoLinker.enableMotionTrigger(true); // Also send on motion detection
}

void loop() { GeoLinker.update(); }
