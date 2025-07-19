#include "OutputModule.h"

OutputModule::OutputModule(LCD *lcdScreen)
{
    lcd = lcdScreen;
    mainTimer = new Timer();
    secondaryTimer = new Timer(0, 0, UP);
    powerSavingTimer = new Timer(POWERSAVING_TIMEOUT_MINUTES, POWERSAVING_TIMEOUT_SECONDS);
    deepSleepTimer = new Timer(DEEP_SLEEP_TIMEOUT_MINUTES, DEEP_SLEEP_TIMEOUT_SECONDS);
    httpControl = new HttpControl();

    percentage = 100;

    switchToMainTimer();

    lcd->update(true);
    lcd->dimScreen();

    // Start power saving timers
    powerSavingTimer->setRunning(true);
    deepSleepTimer->setRunning(true);

    // Don't configure LED pins until needed (power saving)
    disableLed();  // This will set initial state and configure pin correctly

    isOnMainTimer = true;
}

void OutputModule::enableLed()
{
    // Only configure LED pin when actually needed
    pinMode(LED_RED_PIN, OUTPUT);
    analogWrite(LED_RED_PIN, 255);
}

void OutputModule::setPrimaryIndication()
{
    lcd->setWhiteTimeColor();
    httpControl->setPrimary();
    disableLed();
}

void OutputModule::disableLed()
{
    analogWrite(LED_RED_PIN, 0);
    // Set pin back to input to save power when not needed
    pinMode(LED_RED_PIN, INPUT);
}

void OutputModule::setSecondaryIndication()
{
    lcd->setRedTimeColor();
    httpControl->setSecondary();
    enableLed();
}

void OutputModule::switchToMainTimer(bool reset)
{
    setPrimaryIndication();
    isOnMainTimer = true;
    if (reset)
    {
        mainTimer->reset();
        httpControl->setPercentage(100);
    }
    secondaryTimer->reset();
    secondaryTimer->setRunning(false);
    lcd->setWhiteTimeColor();
    lcd->setNewTime(mainTimer->getCurrentMinutes(), mainTimer->getCurrentSeconds());
}

void OutputModule::updateLcdTime(Timer *timer)
{
    timer->updateTimer();
    if (timer->isTimeChanged())
    {
        if(isOnMainTimer)
        {
            auto current_seconds = timer->getCurrentMinutes()*SECONDS_IN_MINUTE + timer->getCurrentSeconds();
            auto max_seconds = timer->getInitialMinutes()*SECONDS_IN_MINUTE + timer->getInitialSeconds();
            httpControl->setPercentage(100*current_seconds / max_seconds);
        }
        
        lcd->setNewTime(timer->getCurrentMinutes(), timer->getCurrentSeconds());
        timer->clearTimeChanged();
    }
}

void OutputModule::processElapsed()
{
    Timer *newTimer = isOnMainTimer ? secondaryTimer : mainTimer;
    Timer *oldTimer = isOnMainTimer ? mainTimer : secondaryTimer;

    mainTimer->reset();
    httpControl->setPercentage(100);
    secondaryTimer->reset();
    newTimer->setRunning(true);
    oldTimer->setRunning(false);
    if (isOnMainTimer)
    {        
        setSecondaryIndication();
    }
    else
    {        
        setPrimaryIndication();
    }
    lcd->setNewTime(newTimer->getCurrentMinutes(), newTimer->getCurrentSeconds());

    isOnMainTimer = !isOnMainTimer;
    oldTimer->clearElapsed();
}

void OutputModule::processStart()
{
    lcd->brightenScreen();
    mainTimer->setRunning(true);
    lcd->processStartRequest();
    resetPowerTimers();  // Reset power timers on user activity
}

void OutputModule::processStop()
{
    if (isOnMainTimer)
        mainTimer->setRunning(false);

    switchToMainTimer(false);
    lcd->brightenScreen();  // Wake up the screen
    lcd->processStopRequest();
    resetPowerTimers();  // Reset power timers on user activity
}

void OutputModule::processReset()
{
    if(isOnMainTimer && !mainTimer->timerRunning() && mainTimer->isAtStart())
    {
        processStart();
        return;
    }

    bool currentlyOnSecondaryTimer = !isOnMainTimer;
    switchToMainTimer(true);
    if (currentlyOnSecondaryTimer)
        mainTimer->setRunning(true);
    
    lcd->brightenScreen();  // Wake up the screen
    resetPowerTimers();  // Reset power timers on user activity
}

void OutputModule::processSetTime(int minutes, int seconds)
{
    mainTimer->setTime(minutes, seconds);
    lcd->setNewTime(minutes, seconds);
    lcd->brightenScreen();  // Wake up the screen
    lcd->update(true);
    resetPowerTimers();  // Reset power timers on user activity
}

void OutputModule::processTick()
{
    // Update power management timers
    powerSavingTimer->updateTimer();
    deepSleepTimer->updateTimer();
    
    // Don't dim or deep sleep the screen when any timer is running
    bool anyTimerRunning = mainTimer->timerRunning() || secondaryTimer->timerRunning();
    
    if(!anyTimerRunning)
    {
        // Check for deep sleep first (complete LCD shutdown)
        if(deepSleepTimer->isElapsed())
        {
            deepSleepTimer->setRunning(false);
            deepSleepTimer->reset();
            deepSleepTimer->clearElapsed();
            lcd->deepSleepScreen();
            return;
        }
        
        // Check for power saving (LCD dimming)
        if(powerSavingTimer->isElapsed())
        {
            powerSavingTimer->setRunning(false);
            powerSavingTimer->reset();
            powerSavingTimer->clearElapsed();
            lcd->dimScreen();
            return;
        }
    }
    
    if (isOnMainTimer)
    {
        updateLcdTime(mainTimer);
        if (mainTimer->isElapsed())
            processElapsed();
    }
    else
    {
        updateLcdTime(secondaryTimer);
        if (secondaryTimer->isElapsed())
            processElapsed();
    }

    lcd->update();
}

void OutputModule::resetPowerTimers()
{
    powerSavingTimer->reset();
    powerSavingTimer->setRunning(true);
    deepSleepTimer->reset();
    deepSleepTimer->setRunning(true);
}