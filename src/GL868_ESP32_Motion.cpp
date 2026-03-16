/*
 * GL868_ESP32_Motion.cpp
 * LIS3DHTR accelerometer driver implementation
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#include "GL868_ESP32_Motion.h"
#include "GL868_ESP32_Logger.h"

GL868_ESP32_Motion::GL868_ESP32_Motion()
    : _threshold(GL868_ESP32_MOTION_DEFAULT_THRESHOLD), _initialized(false),
      _interruptEnabled(false) {}

bool GL868_ESP32_Motion::begin() {
  // Initialize I2C
  Wire.begin(GL868_ESP32_MOTION_SDA, GL868_ESP32_MOTION_SCL);
  Wire.setClock(400000); // 400kHz I2C

  // Check WHO_AM_I register
  uint8_t whoAmI = readRegister(LIS3DH_REG_WHO_AM_I);
  if (whoAmI != LIS3DH_WHO_AM_I_VALUE) {
    GL868_ESP32_LOG_E(
        "Motion sensor not found (WHO_AM_I: 0x%02X, expected: 0x%02X)", whoAmI,
        LIS3DH_WHO_AM_I_VALUE);
    return false;
  }

  // Configure sensor
  // CTRL_REG1: 100Hz, normal mode, all axes enabled
  writeRegister(LIS3DH_REG_CTRL_REG1, 0x57);

  // CTRL_REG2: High-pass filter for interrupt + FDS (filtered data selection)
  // FDS bit enables HPF reference mode for cleaner interrupt baseline resets
  writeRegister(LIS3DH_REG_CTRL_REG2, 0x09);

  // CTRL_REG3: INT1 on IA1 (interrupt activity 1)
  writeRegister(LIS3DH_REG_CTRL_REG3, 0x40);

  // CTRL_REG4: ±2g, high resolution
  writeRegister(LIS3DH_REG_CTRL_REG4, 0x08);

  // CTRL_REG5: Latch interrupt on INT1
  writeRegister(LIS3DH_REG_CTRL_REG5, 0x08);

  // Configure interrupt pin
  pinMode(GL868_ESP32_MOTION_INT, INPUT);

  _initialized = true;
  GL868_ESP32_LOG_I("Motion sensor initialized (LIS3DHTR @ 0x%02X)",
                    GL868_ESP32_MOTION_ADDR);

  return true;
}

void GL868_ESP32_Motion::end() {
  if (!_initialized)
    return;

  // Put sensor in power-down mode
  writeRegister(LIS3DH_REG_CTRL_REG1, 0x00);

  _interruptEnabled = false;
  GL868_ESP32_LOG_D("Motion sensor disabled");
}

bool GL868_ESP32_Motion::isConnected() {
  Wire.beginTransmission(GL868_ESP32_MOTION_ADDR);
  return (Wire.endTransmission() == 0);
}

void GL868_ESP32_Motion::setSensitivity(uint8_t threshold) {
  _threshold = threshold & 0x7F; // Limit to 7 bits

  if (_initialized) {
    // Update INT1 threshold register
    writeRegister(LIS3DH_REG_INT1_THS, _threshold);
    GL868_ESP32_LOG_D("Motion sensitivity set to %u", _threshold);
  }
}

void GL868_ESP32_Motion::setThresholdMg(float mg) {
  // At ±2g scale, each LSB = 16mg
  // Convert mg to register value (0-127)
  uint8_t regVal = (uint8_t)(mg / 16.0f);

  // Clamp to valid range
  if (regVal < 1)
    regVal = 1;
  if (regVal > 127)
    regVal = 127;

  GL868_ESP32_LOG_I("Motion threshold: %.0fmg -> register %u", mg, regVal);
  setSensitivity(regVal);
}

void GL868_ESP32_Motion::enableInterrupt() {
  if (!_initialized)
    return;

  // Set threshold
  writeRegister(LIS3DH_REG_INT1_THS, _threshold);

  // Set duration (minimum samples above threshold) - 0 for immediate
  writeRegister(LIS3DH_REG_INT1_DURATION, 0x00);

  // Configure INT1 for movement detection on all axes
  // OR combination, high events on X, Y, Z
  writeRegister(LIS3DH_REG_INT1_CFG, 0x2A);

  // Clear any pending interrupts
  clearInterrupt();

  _interruptEnabled = true;
  GL868_ESP32_LOG_D("Motion interrupt enabled (threshold: %u)", _threshold);
}

void GL868_ESP32_Motion::disableInterrupt() {
  if (!_initialized)
    return;

  // Disable INT1 configuration
  writeRegister(LIS3DH_REG_INT1_CFG, 0x00);

  _interruptEnabled = false;
  GL868_ESP32_LOG_D("Motion interrupt disabled");
}

bool GL868_ESP32_Motion::motionDetected() {
  // Check interrupt pin state
  return digitalRead(GL868_ESP32_MOTION_INT) == HIGH;
}

void GL868_ESP32_Motion::clearInterrupt() {
  if (!_initialized)
    return;

  // Read REFERENCE register (0x26) to reset the high-pass filter baseline
  // With CTRL_REG2 FDS bit set, this establishes a new "zero" reference point
  // so current acceleration is treated as baseline (no movement)
  readRegister(0x26);

  // Reading INT1_SRC clears the latched interrupt
  uint8_t src = readRegister(LIS3DH_REG_INT1_SRC);
  GL868_ESP32_LOG_D("Motion INT cleared (INT1_SRC=0x%02X)", src);
}

float GL868_ESP32_Motion::readTemperature() {
  if (!_initialized)
    return 0.0f;

  // Enable temperature sensor and ADC
  uint8_t ctrl4 = readRegister(LIS3DH_REG_CTRL_REG4);
  writeRegister(LIS3DH_REG_CTRL_REG4, ctrl4 | 0x80); // Enable BDU

  // Read temperature registers
  int16_t temp = readRegister(LIS3DH_REG_OUT_TEMP_L);
  temp |= (readRegister(LIS3DH_REG_OUT_TEMP_H) << 8);

  // Convert to Celsius (10-bit resolution, 1 LSB = 1C, offset from 25C)
  // The temperature sensor is relative, not absolute
  float temperature = 25.0f + (temp / 256.0f);

  GL868_ESP32_LOG_V("Motion sensor temperature: %.1f C", temperature);

  return temperature;
}

void GL868_ESP32_Motion::writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(GL868_ESP32_MOTION_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t GL868_ESP32_Motion::readRegister(uint8_t reg) {
  Wire.beginTransmission(GL868_ESP32_MOTION_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);

  Wire.requestFrom((uint8_t)GL868_ESP32_MOTION_ADDR, (uint8_t)1);
  if (Wire.available()) {
    return Wire.read();
  }
  return 0;
}
