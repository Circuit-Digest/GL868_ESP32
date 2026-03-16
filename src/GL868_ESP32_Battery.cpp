/*
 * GL868_ESP32_Battery.cpp
 * Battery voltage monitoring implementation
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#include "GL868_ESP32_Battery.h"
#include "GL868_ESP32_Logger.h"
#include "GL868_ESP32_Modem.h"

// ESP32 ADC configuration
#define ADC_RESOLUTION 4095
#define ADC_REF_VOLTAGE 3300 // 3.3V in millivolts
#define VOLTAGE_DIVIDER 2    // 2x due to 10K+10K divider

// Default voltage range for LiPo batteries
#define DEFAULT_BATTERY_MIN_MV 3200
#define DEFAULT_BATTERY_MAX_MV 4200

GL868_ESP32_Battery::GL868_ESP32_Battery()
    : _minMV(DEFAULT_BATTERY_MIN_MV), _maxMV(DEFAULT_BATTERY_MAX_MV),
      _source(BATTERY_SOURCE_ADC), _modem(nullptr) {}

void GL868_ESP32_Battery::begin() {
  // Configure ADC
  analogReadResolution(12);       // 12-bit resolution (0-4095)
  analogSetAttenuation(ADC_11db); // Full range up to ~3.3V

  // Set pin mode
  pinMode(GL868_ESP32_BATTERY_PIN, INPUT);

  GL868_ESP32_LOG_D(
      "Battery monitor initialized (source: %s, range: %umV - %umV)",
      _source == BATTERY_SOURCE_ADC ? "ADC" : "MODEM", _minMV, _maxMV);
}

void GL868_ESP32_Battery::setVoltageRange(uint16_t minMV, uint16_t maxMV) {
  _minMV = minMV;
  _maxMV = maxMV;
  GL868_ESP32_LOG_D("Battery range set: %umV - %umV", _minMV, _maxMV);
}

uint16_t GL868_ESP32_Battery::readVoltageMV() {
  if (_source == BATTERY_SOURCE_MODEM) {
    return readVoltageModem();
  }
  return readVoltageADC();
}

uint16_t GL868_ESP32_Battery::readVoltageADC() {
  // Read ADC value
  uint32_t adcValue = analogRead(GL868_ESP32_BATTERY_PIN);

  // Calculate voltage considering voltage divider
  // Formula: Vbat = (ADC * Vref / Resolution) * Divider
  uint32_t voltageMV =
      (adcValue * ADC_REF_VOLTAGE / ADC_RESOLUTION) * VOLTAGE_DIVIDER;

  GL868_ESP32_LOG_V("Battery ADC: %lu, Voltage: %lumV", adcValue, voltageMV);

  return (uint16_t)voltageMV;
}

uint16_t GL868_ESP32_Battery::readVoltageModem() {
  if (!_modem) {
    GL868_ESP32_LOG_E("Modem not set for battery reading");
    return 0;
  }

  // Send AT+CBC command
  // Response format: +CBC: <bcs>,<bcl>,<voltage>
  // bcs: 0=not charging, 1=charging, 2=charging finished
  // bcl: battery level percentage
  // voltage: battery voltage in mV

  _modem->flushSerial();
  _modem->getSerial().println("AT+CBC");

  uint32_t start = millis();
  while (millis() - start < 2000) {
    if (_modem->getSerial().available()) {
      String line = _modem->getSerial().readStringUntil('\n');

      // Parse +CBC: bcs,bcl,voltage
      if (line.startsWith("+CBC:")) {
        // Find second comma for voltage
        int firstComma = line.indexOf(',');
        int secondComma = line.indexOf(',', firstComma + 1);

        if (secondComma > 0) {
          String voltageStr = line.substring(secondComma + 1);
          voltageStr.trim();
          uint16_t voltage = voltageStr.toInt();

          GL868_ESP32_LOG_V("Battery from modem: %umV", voltage);
          return voltage;
        }
      }
    }
    yield();
  }

  GL868_ESP32_LOG_E("Failed to read battery from modem");
  return 0;
}

uint16_t GL868_ESP32_Battery::readVoltageAveraged(uint8_t samples) {
  if (samples == 0)
    samples = 1;

  // Only average for ADC readings
  if (_source == BATTERY_SOURCE_MODEM) {
    return readVoltageModem();
  }

  uint32_t total = 0;
  for (uint8_t i = 0; i < samples; i++) {
    total += readVoltageADC();
    delayMicroseconds(100); // Small delay between samples
  }

  return (uint16_t)(total / samples);
}

uint8_t GL868_ESP32_Battery::readPercentage() {
  uint16_t voltage;

  if (_source == BATTERY_SOURCE_ADC) {
    voltage = readVoltageAveraged(5);
  } else {
    voltage = readVoltageModem();
  }

  // Clamp to range
  if (voltage <= _minMV)
    return 0;
  if (voltage >= _maxMV)
    return 100;

  // Calculate percentage
  uint32_t range = _maxMV - _minMV;
  uint32_t current = voltage - _minMV;
  uint8_t percentage = (uint8_t)((current * 100) / range);

  GL868_ESP32_LOG_V("Battery: %umV = %u%%", voltage, percentage);

  return percentage;
}
