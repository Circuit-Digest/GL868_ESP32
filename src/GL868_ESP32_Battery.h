/*
 * GL868_ESP32_Battery.h
 * Battery voltage monitoring (ADC or Modem AT+CBC)
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#ifndef GL868_ESP32_BATTERY_H
#define GL868_ESP32_BATTERY_H

#include "GL868_ESP32_Config.h"
#include "GL868_ESP32_Types.h"
#include <Arduino.h>

// Forward declaration
class GL868_ESP32_Modem;

class GL868_ESP32_Battery {
public:
  GL868_ESP32_Battery();

  /**
   * Initialize battery ADC
   */
  void begin();

  /**
   * Set modem reference for AT+CBC readings
   */
  void setModem(GL868_ESP32_Modem *modem) { _modem = modem; }

  /**
   * Set battery source (ADC or Modem)
   * @param source BATTERY_SOURCE_ADC or BATTERY_SOURCE_MODEM
   */
  void setSource(BatterySource source) { _source = source; }

  /**
   * Get current battery source
   */
  BatterySource getSource() const { return _source; }

  /**
   * Set voltage range for percentage calculation
   * Default: 3200mV (0%) to 4200mV (100%)
   * @param minMV Minimum voltage (0%) in millivolts
   * @param maxMV Maximum voltage (100%) in millivolts
   */
  void setVoltageRange(uint16_t minMV, uint16_t maxMV);

  /**
   * Read battery voltage (from selected source)
   * @return Voltage in millivolts
   */
  uint16_t readVoltageMV();

  /**
   * Read battery percentage
   * @return Battery percentage (0-100)
   */
  uint8_t readPercentage();

  /**
   * Get averaged voltage reading (ADC only)
   * @param samples Number of samples to average
   * @return Averaged voltage in millivolts
   */
  uint16_t readVoltageAveraged(uint8_t samples = 10);

private:
  uint16_t readVoltageADC();
  uint16_t readVoltageModem();

  uint16_t _minMV;
  uint16_t _maxMV;
  BatterySource _source;
  GL868_ESP32_Modem *_modem;
};

#endif // GL868_ESP32_BATTERY_H
