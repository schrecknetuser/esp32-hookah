#pragma once

#include <Arduino.h>
#include "LCD.h"
#include "Timer.h"
#include "HttpControl.h"

class OutputModule
{
public:
    OutputModule(LCD* lcdScreen);

    void processStart();
    void processStop();
    void processReset();
    void processSetTime(int minutes, int seconds);
    void processTick();
    void resetPowerTimers();  // Reset power saving timers on user activity
private:

    void setPrimaryIndication();
    void setSecondaryIndication();
    void enableLed();
    void disableLed();
    void switchToMainTimer(bool reset = true);
    void updateLcdTime(Timer *timer);
    void processElapsed();
    void resetPowerTimers();  // Reset power saving timers on user activity

    bool isOnMainTimer;
    Timer *mainTimer;
    Timer *secondaryTimer;
    Timer *powerSavingTimer;
    Timer *deepSleepTimer;    // For complete LCD shutdown
    LCD* lcd;
    HttpControl* httpControl;
    int percentage;
};