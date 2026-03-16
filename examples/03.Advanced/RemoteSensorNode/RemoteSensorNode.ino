/*
 * RemoteSensorNode.ino
 * GL868_ESP32 Library - Remote Sensor Node
 *
 * Demonstrates how to add custom sensor data to the GPS payload.
 * Uses dummy sensor values - replace with your actual sensor readings.
 */

#include <GL868_ESP32.h>

// ============================================================================
// Configuration - EDIT THESE VALUES
// ============================================================================
#define DEVICE_ID "YOUR_DEVICE_ID"
#define API_KEY "YOUR_API_KEY"
#define SEND_INTERVAL 300 // Send every 5 minutes

// ============================================================================
// Simulated Sensor Readings (replace with real sensors)
// ============================================================================
float readTemperature() {
  // Replace with actual temperature sensor reading
  // Example: DHT22, DS18B20, BME280, etc.
  return 25.5 + random(-50, 50) / 10.0; // Dummy: 25.5 ± 5°C
}

float readHumidity() {
  // Replace with actual humidity sensor reading
  return 65.0 + random(-100, 100) / 10.0; // Dummy: 65 ± 10%
}

float readPressure() {
  // Replace with actual pressure sensor reading
  return 1013.25 + random(-50, 50) / 10.0; // Dummy: 1013.25 ± 5 hPa
}

int readSoilMoisture() {
  // Replace with actual soil moisture sensor reading
  return random(30, 80); // Dummy: 30-80%
}

float readLightLevel() {
  // Replace with actual light sensor reading
  return random(100, 10000); // Dummy: 100-10000 lux
}

// ============================================================================
// Setup
// ============================================================================
void setup() {
  Serial.begin(115200);

  // Initialize
  GeoLinker.begin(DEVICE_ID, API_KEY);

  // Set send interval
  GeoLinker.setSendInterval(SEND_INTERVAL);
  GeoLinker.enableMotionTrigger(false); // Location not primary focus

  // Configure custom payload fields
  updateSensorPayloads();

  Serial.println("Remote Sensor Node Started");
  Serial.print("Sending data every ");
  Serial.print(SEND_INTERVAL);
  Serial.println(" seconds");
}

void loop() {
  GeoLinker.update();

  // Update sensor readings before each send
  static uint32_t lastUpdate = 0;
  if (millis() - lastUpdate >= 10000) { // Update every 10 seconds
    lastUpdate = millis();
    updateSensorPayloads();
  }
}

// ============================================================================
// Update Sensor Payloads
// ============================================================================
void updateSensorPayloads() {
  float temp = readTemperature();
  float humidity = readHumidity();
  float pressure = readPressure();
  int soilMoisture = readSoilMoisture();
  float lightLevel = readLightLevel();

  Serial.println("\n--- Sensor Readings ---");
  Serial.printf("Temperature: %.1f °C\n", temp);
  Serial.printf("Humidity: %.1f %%\n", humidity);
  Serial.printf("Pressure: %.1f hPa\n", pressure);
  Serial.printf("Soil Moisture: %d %%\n", soilMoisture);
  Serial.printf("Light Level: %.0f lux\n", lightLevel);

  // Set custom payloads to be included in data sent to server
  GeoLinker.setPayloads({{"temperature", temp},
                         {"humidity", humidity},
                         {"pressure", pressure},
                         {"soil_moisture", soilMoisture},
                         {"light_level", lightLevel}});
}
