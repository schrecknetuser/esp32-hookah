#include "InputModule.h"

InputModule::InputModule(LCD* lcd)
{
    // TEMPORARILY DISABLED: Bot creation causing HTTP socket errors
    bot = nullptr;  // Don't create bot instance to avoid socket issues
    touchScreen = new TouchScreen();
    touchProcessor = lcd;

    pinMode(PUSHBUTTON_PIN, INPUT_PULLUP);

    clearRequests();
}

bool InputModule::isPushButtonPressed()
{
  return (digitalRead(PUSHBUTTON_PIN) == 0);
}

void InputModule::clearRequests()
{
    resetRequested = false;
    startRequested = false;
    stopRequested = false;
    setTimeRequested = false;
}

void InputModule::processPushButton()
{
    if(isPushButtonPressed())
        resetRequested = true;
}

void InputModule::processTouchScreen()
{
    if(!touchScreen->isTouched())
    {
        touchScreen->setWasTouched(false);
        return;
    }

    if(touchScreen->getWasTouched())
        return;
    touchScreen->setWasTouched(true);

    lv_point_t point;
    touchScreen->getTouchPoint(point, ROT_0);
    switch(touchProcessor->processTouch(point))
    {
        case START_REQUESTED:
            startRequested = true;
            break;
        case STOP_REQUESTED:
            stopRequested = true;
            break;
        case RESET_REQUESTED:
            resetRequested = true;
            break;
    }        
}

void InputModule::pollBot()
{
    // TEMPORARILY DISABLED: Bot functionality causing HTTP socket errors
    // Do nothing - bot is disabled to fix socket management issues
    return;
}

void InputModule::processBot()
{
    // TEMPORARILY DISABLED: Bot functionality causing HTTP socket errors
    // Do nothing - bot requests are disabled
    return;
}

void InputModule::processRequests()
{
    processPushButton();
    processTouchScreen();
    processBot();
}
