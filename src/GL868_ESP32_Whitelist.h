/*
 * GL868_ESP32_Whitelist.h
 * Phone number whitelist with NVS storage
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#ifndef GL868_ESP32_WHITELIST_H
#define GL868_ESP32_WHITELIST_H

#include "GL868_ESP32_Config.h"
#include <Arduino.h>

class GL868_ESP32_Whitelist {
public:
  GL868_ESP32_Whitelist();

  /**
   * Initialize and load from NVS
   * @return true if successful
   */
  bool begin();

  /**
   * Save whitelist to NVS
   */
  void save();

  /**
   * Add phone number to whitelist
   * First number added becomes admin
   * @param number Phone number to add
   * @return true if added
   */
  bool add(const char *number);

  /**
   * Remove phone number from whitelist
   * @param number Phone number to remove
   * @return true if removed
   */
  bool remove(const char *number);

  /**
   * Check if number is in whitelist
   * @param number Phone number to check
   * @return true if whitelisted
   */
  bool contains(const char *number);

  /**
   * Check if number is admin
   * @param number Phone number to check
   * @return true if admin
   */
  bool isAdmin(const char *number);

  /**
   * Clear all entries
   */
  void clear();

  /**
   * Get number of entries
   */
  uint8_t count() const { return _count; }

  /**
   * Get number at index
   * @param index Index (0-based)
   * @return Phone number or nullptr
   */
  const char *get(uint8_t index) const;

  /**
   * Check if whitelist is empty
   * Empty whitelist means all numbers are allowed
   */
  bool isEmpty() const { return _count == 0; }

  /**
   * Check if number is allowed
   * Returns true if whitelist is empty or number is in list
   */
  bool isAllowed(const char *number);

private:
  void normalizeNumber(const char *input, char *output, size_t maxLen);

  char _numbers[GL868_ESP32_MAX_WHITELIST_NUMBERS][GL868_ESP32_MAX_PHONE_LEN];
  uint8_t _count;
  char _adminNumber[GL868_ESP32_MAX_PHONE_LEN];
  bool _initialized;
};

#endif // GL868_ESP32_WHITELIST_H
