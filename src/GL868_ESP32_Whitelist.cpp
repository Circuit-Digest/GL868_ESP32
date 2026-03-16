/*
 * GL868_ESP32_Whitelist.cpp
 * Phone number whitelist implementation
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#include "GL868_ESP32_Whitelist.h"
#include "GL868_ESP32_Logger.h"
#include <Preferences.h>

static Preferences _nvsPrefs;

GL868_ESP32_Whitelist::GL868_ESP32_Whitelist()
    : _count(0), _initialized(false) {
  _adminNumber[0] = '\0';

  for (uint8_t i = 0; i < GL868_ESP32_MAX_WHITELIST_NUMBERS; i++) {
    _numbers[i][0] = '\0';
  }
}

bool GL868_ESP32_Whitelist::begin() {
  // Open NVS namespace
  if (!_nvsPrefs.begin(GL868_ESP32_NVS_NAMESPACE, false)) {
    GL868_ESP32_LOG_E("Failed to open NVS namespace");
    return false;
  }

  // Load count
  _count = _nvsPrefs.getUChar("wl_count", 0);

  if (_count > GL868_ESP32_MAX_WHITELIST_NUMBERS) {
    _count = 0; // Corrupted, reset
  }

  // Load numbers
  for (uint8_t i = 0; i < _count; i++) {
    char key[16];
    snprintf(key, sizeof(key), "wl_%d", i);

    String num = _nvsPrefs.getString(key, "");
    strncpy(_numbers[i], num.c_str(), GL868_ESP32_MAX_PHONE_LEN - 1);
    _numbers[i][GL868_ESP32_MAX_PHONE_LEN - 1] = '\0';
  }

  // Load admin
  String admin = _nvsPrefs.getString("wl_admin", "");
  strncpy(_adminNumber, admin.c_str(), GL868_ESP32_MAX_PHONE_LEN - 1);
  _adminNumber[GL868_ESP32_MAX_PHONE_LEN - 1] = '\0';

  _nvsPrefs.end();

  _initialized = true;
  GL868_ESP32_LOG_I("Whitelist loaded: %d numbers, admin: %s", _count,
                    _adminNumber[0] ? _adminNumber : "none");

  return true;
}

void GL868_ESP32_Whitelist::save() {
  if (!_nvsPrefs.begin(GL868_ESP32_NVS_NAMESPACE, false)) {
    GL868_ESP32_LOG_E("Failed to open NVS for save");
    return;
  }

  // Save count
  _nvsPrefs.putUChar("wl_count", _count);

  // Save numbers
  for (uint8_t i = 0; i < _count; i++) {
    char key[16];
    snprintf(key, sizeof(key), "wl_%d", i);
    _nvsPrefs.putString(key, _numbers[i]);
  }

  // Clear extra entries
  for (uint8_t i = _count; i < GL868_ESP32_MAX_WHITELIST_NUMBERS; i++) {
    char key[16];
    snprintf(key, sizeof(key), "wl_%d", i);
    _nvsPrefs.remove(key);
  }

  // Save admin
  _nvsPrefs.putString("wl_admin", _adminNumber);

  _nvsPrefs.end();

  GL868_ESP32_LOG_D("Whitelist saved: %d numbers", _count);
}

void GL868_ESP32_Whitelist::normalizeNumber(const char *input, char *output,
                                            size_t maxLen) {
  // Remove spaces, dashes, and normalize
  size_t j = 0;
  for (size_t i = 0; input[i] && j < maxLen - 1; i++) {
    char c = input[i];
    if (c == '+' || (c >= '0' && c <= '9')) {
      output[j++] = c;
    }
  }
  output[j] = '\0';
}

bool GL868_ESP32_Whitelist::add(const char *number) {
  if (_count >= GL868_ESP32_MAX_WHITELIST_NUMBERS) {
    GL868_ESP32_LOG_E("Whitelist full");
    return false;
  }

  // Normalize number
  char normalized[GL868_ESP32_MAX_PHONE_LEN];
  normalizeNumber(number, normalized, GL868_ESP32_MAX_PHONE_LEN);

  if (strlen(normalized) == 0) {
    return false;
  }

  // Check if already exists
  if (contains(normalized)) {
    GL868_ESP32_LOG_D("Number already in whitelist: %s", normalized);
    return false;
  }

  // Add number
  strncpy(_numbers[_count], normalized, GL868_ESP32_MAX_PHONE_LEN - 1);
  _numbers[_count][GL868_ESP32_MAX_PHONE_LEN - 1] = '\0';

  // First number becomes admin
  if (_count == 0 || _adminNumber[0] == '\0') {
    strncpy(_adminNumber, normalized, GL868_ESP32_MAX_PHONE_LEN - 1);
    _adminNumber[GL868_ESP32_MAX_PHONE_LEN - 1] = '\0';
    GL868_ESP32_LOG_I("Admin set: %s", _adminNumber);
  }

  _count++;
  save();

  GL868_ESP32_LOG_I("Added to whitelist: %s (count: %d)", normalized, _count);
  return true;
}

bool GL868_ESP32_Whitelist::remove(const char *number) {
  char normalized[GL868_ESP32_MAX_PHONE_LEN];
  normalizeNumber(number, normalized, GL868_ESP32_MAX_PHONE_LEN);

  // Find and remove
  for (uint8_t i = 0; i < _count; i++) {
    if (strcmp(_numbers[i], normalized) == 0) {
      // Shift remaining entries
      for (uint8_t j = i; j < _count - 1; j++) {
        strcpy(_numbers[j], _numbers[j + 1]);
      }
      _numbers[_count - 1][0] = '\0';
      _count--;

      // If removed admin, set new admin to first entry
      if (strcmp(_adminNumber, normalized) == 0) {
        if (_count > 0) {
          strcpy(_adminNumber, _numbers[0]);
        } else {
          _adminNumber[0] = '\0';
        }
      }

      save();
      GL868_ESP32_LOG_I("Removed from whitelist: %s (count: %d)", normalized,
                        _count);
      return true;
    }
  }

  return false;
}

bool GL868_ESP32_Whitelist::contains(const char *number) {
  char normalized[GL868_ESP32_MAX_PHONE_LEN];
  normalizeNumber(number, normalized, GL868_ESP32_MAX_PHONE_LEN);

  for (uint8_t i = 0; i < _count; i++) {
    // Check if numbers match (allow partial match for country code variations)
    if (strstr(_numbers[i], normalized) || strstr(normalized, _numbers[i])) {
      return true;
    }
    if (strcmp(_numbers[i], normalized) == 0) {
      return true;
    }
  }

  return false;
}

bool GL868_ESP32_Whitelist::isAdmin(const char *number) {
  if (_adminNumber[0] == '\0')
    return false;

  char normalized[GL868_ESP32_MAX_PHONE_LEN];
  normalizeNumber(number, normalized, GL868_ESP32_MAX_PHONE_LEN);

  // Allow partial match for country code variations
  if (strstr(_adminNumber, normalized) || strstr(normalized, _adminNumber)) {
    return true;
  }

  return strcmp(_adminNumber, normalized) == 0;
}

bool GL868_ESP32_Whitelist::isAllowed(const char *number) {
  // Empty whitelist = allow all
  if (_count == 0)
    return true;

  return contains(number);
}

void GL868_ESP32_Whitelist::clear() {
  _count = 0;
  _adminNumber[0] = '\0';

  for (uint8_t i = 0; i < GL868_ESP32_MAX_WHITELIST_NUMBERS; i++) {
    _numbers[i][0] = '\0';
  }

  save();
  GL868_ESP32_LOG_I("Whitelist cleared");
}

const char *GL868_ESP32_Whitelist::get(uint8_t index) const {
  if (index >= _count)
    return nullptr;
  return _numbers[index];
}
