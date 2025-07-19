#pragma once

#include <Arduino.h>
#include "Adafruit_ILI9341.h"
#include "hookah.h"
#include "common.h"

#define LCD_CS 33
#define LCD_DC 26
#define LCD_RST 27

#define LCD_LED 21

#define LCD_ACTIVE_VOLTAGE 255
#define LCD_POWERSAVING_VOLTAGE 16
#define LCD_DEEPSLEEP_VOLTAGE 16    // Complete LCD shutdown

#define NOTHING_REQUESTED 0
#define START_REQUESTED 1
#define STOP_REQUESTED 2
#define RESET_REQUESTED 3


class LCD {
public:
    LCD();
    void drawHookah();
    void setTimerStarted(bool value);
    bool isTimerStarted();
    void update(bool force = false);
    void clearStartStop();
    void drawStartStop();
    void drawTime(bool force = false);
    void drawReset();
    int processTouch(lv_point_t &point);

    void setNewTime(int minutes, int seconds);


    void setRedTimeColor();
    void setWhiteTimeColor();

    void processStartRequest();
    void processStopRequest();

    void brightenScreen();
    void dimScreen();
    void deepSleepScreen();  // Complete LCD shutdown

private:
    

    Adafruit_ILI9341* lcd;
    bool timerStarted;
    bool startStopStateChanged;

    int currentMinutes;
    int currentSeconds;
    int newMinutes;
    int newSeconds;

    int resetX;
    int resetY;
    int resetWidth;
    int resetHeight;

    int startStopX;
    int startStopY;
    int startStopWidth;
    int startStopHeight;

    int startTextX;
    int startTextY;

    int stopTextX;
    int stopTextY;

    int resetTextX;
    int resetTextY;

    int timeX;
    int timeY;
    int timeWidth;
    int timeHeight;

    int timeTextX;
    int timeTextY;

    bool startWasTouched;
    bool stopWasTouched;
    bool resetWasTouched;

    int timeColor;
};