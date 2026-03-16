/*
 * GL868_ESP32_Queue.h
 * Offline data queue with RAM and filesystem support
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#ifndef GL868_ESP32_QUEUE_H
#define GL868_ESP32_QUEUE_H

#include "GL868_ESP32_Config.h"
#include "GL868_ESP32_Types.h"
#include <Arduino.h>

class GL868_ESP32_Queue {
public:
  GL868_ESP32_Queue();
  ~GL868_ESP32_Queue();

  /**
   * Initialize queue
   * @param type Storage type (RAM, LittleFS, SPIFFS)
   * @param maxEntries Maximum entries to store
   * @return true if successful
   */
  bool begin(QueueStorage type,
             uint8_t maxEntries = GL868_ESP32_RAM_QUEUE_SIZE);

  /**
   * Cleanup queue resources
   */
  void end();

  /**
   * Push entry to queue
   * @param entry Entry to push
   * @return true if successful
   */
  bool push(const QueueEntry &entry);

  /**
   * Pop entry from queue (removes oldest)
   * @param entry Pointer to store entry
   * @return true if entry available
   */
  bool pop(QueueEntry *entry);

  /**
   * Peek at oldest entry without removing
   * @param entry Pointer to store entry
   * @return true if entry available
   */
  bool peek(QueueEntry *entry);

  /**
   * Get number of entries in queue
   */
  uint8_t count() const;

  /**
   * Check if queue is empty
   */
  bool isEmpty() const { return count() == 0; }

  /**
   * Check if queue is full
   */
  bool isFull() const { return count() >= _maxEntries; }

  /**
   * Clear all entries
   */
  void clear();

  /**
   * Check if timestamp already exists (for de-duplication)
   * @param timestamp Timestamp string to check
   * @return true if duplicate exists
   */
  bool isDuplicate(const char *timestamp);

  /**
   * Get storage type
   */
  QueueStorage getStorageType() const { return _type; }

private:
  bool initRAM();
  bool initFS();
  bool loadFromFS();
  bool saveToFS();

  QueueStorage _type;
  uint8_t _maxEntries;
  bool _initialized;

  // RAM storage (ring buffer)
  QueueEntry *_ramBuffer;
  uint8_t _head;
  uint8_t _tail;
  uint8_t _count;

  // Filesystem storage
  String _filePath;
};

#endif // GL868_ESP32_QUEUE_H
