/*
 * GL868_ESP32_Queue.cpp
 * Offline data queue implementation
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#include "GL868_ESP32_Queue.h"
#include "GL868_ESP32_Logger.h"

#if defined(ESP32)
#include <LittleFS.h>
#include <SPIFFS.h>
#endif

GL868_ESP32_Queue::GL868_ESP32_Queue()
    : _type(QUEUE_RAM), _maxEntries(GL868_ESP32_RAM_QUEUE_SIZE),
      _initialized(false), _ramBuffer(nullptr), _head(0), _tail(0), _count(0) {}

GL868_ESP32_Queue::~GL868_ESP32_Queue() { end(); }

bool GL868_ESP32_Queue::begin(QueueStorage type, uint8_t maxEntries) {
  _type = type;
  _maxEntries = maxEntries;
  _filePath = GL868_ESP32_QUEUE_FILENAME;

  bool success = false;

  switch (type) {
  case QUEUE_RAM:
    success = initRAM();
    break;
  case QUEUE_LITTLEFS:
  case QUEUE_SPIFFS:
    success = initFS();
    break;
  }

  if (success) {
    _initialized = true;
    GL868_ESP32_LOG_I("Queue initialized (type: %d, max: %d)", type,
                      maxEntries);
  }

  return success;
}

void GL868_ESP32_Queue::end() {
  if (_ramBuffer) {
    delete[] _ramBuffer;
    _ramBuffer = nullptr;
  }

  _initialized = false;
  _count = 0;
  _head = 0;
  _tail = 0;
}

bool GL868_ESP32_Queue::initRAM() {
  // Allocate buffer
  _ramBuffer = new QueueEntry[_maxEntries];
  if (!_ramBuffer) {
    GL868_ESP32_LOG_E("Failed to allocate queue buffer");
    return false;
  }

  _head = 0;
  _tail = 0;
  _count = 0;

  GL868_ESP32_LOG_D("RAM queue initialized (%d entries, %u bytes)", _maxEntries,
                    _maxEntries * sizeof(QueueEntry));
  return true;
}

bool GL868_ESP32_Queue::initFS() {
#if defined(ESP32)
  bool mounted = false;

  if (_type == QUEUE_LITTLEFS) {
    mounted = LittleFS.begin(true); // Format if needed
    if (!mounted) {
      GL868_ESP32_LOG_E("Failed to mount LittleFS");
      return false;
    }
  } else {
    mounted = SPIFFS.begin(true); // Format if needed
    if (!mounted) {
      GL868_ESP32_LOG_E("Failed to mount SPIFFS");
      return false;
    }
  }

  // Also allocate RAM buffer for working storage
  if (!initRAM())
    return false;

  // Load existing data from filesystem
  loadFromFS();

  return true;
#else
  // Fallback to RAM for non-ESP32
  return initRAM();
#endif
}

bool GL868_ESP32_Queue::loadFromFS() {
#if defined(ESP32)
  File file;

  if (_type == QUEUE_LITTLEFS) {
    if (!LittleFS.exists(_filePath))
      return true; // No data yet
    file = LittleFS.open(_filePath, "r");
  } else {
    if (!SPIFFS.exists(_filePath))
      return true;
    file = SPIFFS.open(_filePath, "r");
  }

  if (!file) {
    GL868_ESP32_LOG_E("Failed to open queue file");
    return false;
  }

  // Read count
  uint8_t storedCount = file.read();

  // Read entries
  _count = 0;
  for (uint8_t i = 0; i < storedCount && i < _maxEntries; i++) {
    QueueEntry entry;
    size_t bytesRead = file.read((uint8_t *)&entry, sizeof(QueueEntry));

    if (bytesRead == sizeof(QueueEntry)) {
      _ramBuffer[_tail] = entry;
      _tail = (_tail + 1) % _maxEntries;
      _count++;
    }
  }

  file.close();

  GL868_ESP32_LOG_I("Loaded %d entries from filesystem", _count);
  return true;
#else
  return false;
#endif
}

bool GL868_ESP32_Queue::saveToFS() {
#if defined(ESP32)
  if (_type == QUEUE_RAM)
    return true;

  File file;

  if (_type == QUEUE_LITTLEFS) {
    file = LittleFS.open(_filePath, "w");
  } else {
    file = SPIFFS.open(_filePath, "w");
  }

  if (!file) {
    GL868_ESP32_LOG_E("Failed to open queue file for writing");
    return false;
  }

  // Write count
  file.write(_count);

  // Write entries
  uint8_t idx = _head;
  for (uint8_t i = 0; i < _count; i++) {
    file.write((uint8_t *)&_ramBuffer[idx], sizeof(QueueEntry));
    idx = (idx + 1) % _maxEntries;
  }

  file.close();

  GL868_ESP32_LOG_V("Saved %d entries to filesystem", _count);
  return true;
#else
  return false;
#endif
}

bool GL868_ESP32_Queue::push(const QueueEntry &entry) {
  if (!_initialized || !_ramBuffer)
    return false;

  // Check for duplicate
  if (isDuplicate(entry.gps.timestamp)) {
    GL868_ESP32_LOG_D("Duplicate entry ignored: %s", entry.gps.timestamp);
    return false;
  }

  if (_count >= _maxEntries) {
    // Queue full - overwrite oldest
    _head = (_head + 1) % _maxEntries;
    _count--;
    GL868_ESP32_LOG_D("Queue full, overwriting oldest entry");
  }

  // Add new entry at tail
  _ramBuffer[_tail] = entry;
  _tail = (_tail + 1) % _maxEntries;
  _count++;

  GL868_ESP32_LOG_D("Queued entry: %s (count: %d)", entry.gps.timestamp,
                    _count);

  // Save to filesystem if using FS storage
  if (_type != QUEUE_RAM) {
    saveToFS();
  }

  return true;
}

bool GL868_ESP32_Queue::pop(QueueEntry *entry) {
  if (!_initialized || !_ramBuffer || _count == 0 || !entry)
    return false;

  // Get oldest entry
  *entry = _ramBuffer[_head];
  _head = (_head + 1) % _maxEntries;
  _count--;

  GL868_ESP32_LOG_D("Popped entry: %s (remaining: %d)", entry->gps.timestamp,
                    _count);

  // Save to filesystem if using FS storage
  if (_type != QUEUE_RAM) {
    saveToFS();
  }

  return true;
}

bool GL868_ESP32_Queue::peek(QueueEntry *entry) {
  if (!_initialized || !_ramBuffer || _count == 0 || !entry)
    return false;

  *entry = _ramBuffer[_head];
  return true;
}

uint8_t GL868_ESP32_Queue::count() const { return _count; }

void GL868_ESP32_Queue::clear() {
  _head = 0;
  _tail = 0;
  _count = 0;

  if (_type != QUEUE_RAM) {
    saveToFS();
  }

  GL868_ESP32_LOG_I("Queue cleared");
}

bool GL868_ESP32_Queue::isDuplicate(const char *timestamp) {
  if (!_initialized || !_ramBuffer || _count == 0)
    return false;

  uint8_t idx = _head;
  for (uint8_t i = 0; i < _count; i++) {
    if (strcmp(_ramBuffer[idx].gps.timestamp, timestamp) == 0) {
      return true;
    }
    idx = (idx + 1) % _maxEntries;
  }

  return false;
}
