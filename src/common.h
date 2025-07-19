#pragma once

#include <Arduino.h>

enum {
    ROT_0 = 0,
    ROT_90,
    ROT_270,
    ROT_360
};

#define MAX_WIDTH 240 
#define MAX_HEIGHT 320

typedef uint8_t touch_rot_t;
typedef uint16_t lv_coord_t;
typedef struct {
  lv_coord_t x;
  lv_coord_t y;
} lv_point_t;

#define DEFAULT_MINUTES 5
#define DEFAULT_SECONDS 0

#define POWERSAVING_TIMEOUT_MINUTES 1
#define POWERSAVING_TIMEOUT_SECONDS 0

// Power saving configuration
#define DEEP_SLEEP_TIMEOUT_MINUTES 5
#define DEEP_SLEEP_TIMEOUT_SECONDS 0
#define MAIN_LOOP_DELAY_MS 100  // Reduced from 1ms to 100ms
#define BOT_LOOP_DELAY_MS 2000  // Reduced from 1ms to 2 seconds

// Memory monitoring (debug only - should be disabled in production)
#define ENABLE_MEMORY_MONITORING 1
#define MEMORY_CHECK_INTERVAL_MS 30000  // Check every 30 seconds

#define SECONDARY_MINUTES 0
#define SECONDARY_SECONDS 15

#define SECONDS_IN_MINUTE 60
#define MILLIS_IN_SECOND 1000

#define LED_RED_PIN 32
#define LED_GREEN_PIN 22
#define LED_BLUE_PIN 25
#define PUSHBUTTON_PIN 4

void convertTimeToString(int minutes, int seconds, char* result, size_t bufferSize);
