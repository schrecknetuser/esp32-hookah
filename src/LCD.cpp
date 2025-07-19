#include "LCD.h"

LCD::LCD()
{
    pinMode(LCD_LED, OUTPUT);
    brightenScreen();

    lcd = new Adafruit_ILI9341(LCD_CS, LCD_DC, LCD_RST);
    lcd->begin();
    lcd->fillScreen(ILI9341_BLACK);
    lcd->setRotation(ROT_0);
    timerStarted = false;
    lcd->drawRGBBitmap(0, 0, (uint16_t *)HOOKAH_BITMAP, HOOKAH_BITMAP_WIDTH, HOOKAH_BITMAP_HEIGHT);

    currentMinutes = DEFAULT_MINUTES;
    currentSeconds = DEFAULT_SECONDS;

    newMinutes = DEFAULT_MINUTES;
    newSeconds = DEFAULT_SECONDS;
    startStopStateChanged = false;

    resetX = 125;
    resetY = 285;
    resetWidth = 110;
    resetHeight = 30;

    startStopX = 5;
    startStopY = 285;
    startStopWidth = 110;
    startStopHeight = 30;

    startTextX = 32;
    startTextY = 293;

    stopTextX = 38;
    stopTextY = 293;

    resetTextX = 150;
    resetTextY = 293;

    timeTextX = 80;
    timeTextY = 250;

    timeX = 5;
    timeY = 240;
    timeWidth = 230;
    timeHeight = 40;

    startWasTouched = false;
    stopWasTouched = false;
    resetWasTouched = false;

    timeColor = ILI9341_WHITE;
}

void LCD::brightenScreen()
{
    analogWrite(LCD_LED, LCD_ACTIVE_VOLTAGE);
}

void LCD::dimScreen()
{
    analogWrite(LCD_LED, LCD_POWERSAVING_VOLTAGE);
}

void LCD::deepSleepScreen()
{
    analogWrite(LCD_LED, LCD_DEEPSLEEP_VOLTAGE);
}

void LCD::drawHookah()
{
    lcd->drawRGBBitmap(0, 0, (uint16_t *)HOOKAH_BITMAP, HOOKAH_BITMAP_WIDTH, HOOKAH_BITMAP_HEIGHT);
}

void LCD::setTimerStarted(bool value) {
    timerStarted = value;
}

bool LCD::isTimerStarted() {
    return timerStarted;
}

void LCD::clearStartStop()
{
    lcd->drawRect(startStopX, startStopY, startStopWidth, startStopHeight, ILI9341_BLACK);
    lcd->setTextColor(ILI9341_BLACK);
    lcd->setCursor(startTextX, startTextY);
    lcd->setTextSize(2);
    lcd->print("START");

    lcd->drawRect(startStopX, startStopY, startStopWidth, startStopHeight, ILI9341_BLACK);
    lcd->setTextColor(ILI9341_BLACK);
    lcd->setCursor(stopTextX, stopTextY);
    lcd->setTextSize(2);
    lcd->print("STOP");
}

void LCD::drawStartStop()
{
    if(!isTimerStarted())
    {
        lcd->drawRect(startStopX, startStopY, startStopWidth, startStopHeight, ILI9341_GREEN);
        lcd->setTextColor(ILI9341_GREEN);
        lcd->setCursor(startTextX, startTextY);
        lcd->setTextSize(2);
        lcd->print("START");
    }
    else
    {
        lcd->drawRect(startStopX, startStopY, startStopWidth, startStopHeight, ILI9341_RED);
        lcd->setTextColor(ILI9341_RED);
        lcd->setCursor(stopTextX, stopTextY);
        lcd->setTextSize(2);
        lcd->print("STOP");
    }
    
}

void LCD::drawTime(bool force)
{
    if(currentMinutes == newMinutes && currentSeconds == newSeconds && !force)
        return;
    lcd->drawRect(timeX, timeY, timeWidth, timeHeight, timeColor);
    String timeString;
    convertTimeToString(currentMinutes, currentSeconds, timeString);

    lcd->setTextSize(3);
    lcd->setCursor(timeTextX, timeTextY);

    lcd->setTextColor(ILI9341_BLACK);
    lcd->print(timeString);

    convertTimeToString(newMinutes, newSeconds, timeString);

    lcd->setCursor(timeTextX, timeTextY);
    lcd->setTextColor(timeColor);
    lcd->print(timeString);

    currentMinutes = newMinutes;
    currentSeconds = newSeconds;
}

void LCD::drawReset()
{
    lcd->drawRect(resetX, resetY, resetWidth, resetHeight, ILI9341_WHITE);
    lcd->setTextColor(ILI9341_WHITE);
    lcd->setCursor(resetTextX, resetTextY);
    lcd->setTextSize(2);
    lcd->print("RESET");
}

void LCD::update(bool force)
{
    drawTime(force);
    drawReset();
    
    if(startStopStateChanged)
    {
        clearStartStop();
        startStopStateChanged = false;
    }
    
    drawStartStop();
}

int LCD::processTouch(lv_point_t &point)
{
    if(point.x >= startStopX && point.x <= startStopX + startStopWidth 
        && point.y >= startStopY && point.y <= startStopY + startStopHeight)
    {
        //processTouchStartStop();
        if(!isTimerStarted())
            return START_REQUESTED;
        return STOP_REQUESTED;
    }

    if(point.x >= resetX && point.x <= resetX + resetWidth && point.y >= resetY && point.y <= resetY + resetHeight)
    {
        //processTouchReset();
        return RESET_REQUESTED;
    }

    return NOTHING_REQUESTED;
}

void LCD::processStartRequest()
{
    if(isTimerStarted())
        return;
    setTimerStarted(true);
    startStopStateChanged = true;
}
void LCD::processStopRequest()
{
    if(!isTimerStarted())
        return;
    setTimerStarted(false);
    startStopStateChanged = true;
}

/*void LCD::processTouchStartStop()
{
    if(!isTimerStarted())
        startWasTouched = true;
    else
        stopWasTouched = true;
    setTimerStarted(!isTimerStarted());
    startStopStateChanged = true;
}*/

/*void LCD::processTouchReset()
{
    resetWasTouched = true;
}

bool LCD::getStartWasTouched() 
{
    return startWasTouched;
}

bool LCD::getStopWasTouched()
{
    return stopWasTouched;
}

bool LCD::getResetWasTouched()
{
    return resetWasTouched;
}

void LCD::clearStartWasTouched()
{
    startWasTouched = false;
}

void LCD::clearStopWasTouched()
{
    stopWasTouched = false;
}

void LCD::clearResetWasTouched()
{
    resetWasTouched = false;
}*/

void LCD::setNewTime(int minutes, int seconds)
{
    newMinutes = minutes;
    newSeconds = seconds;
}

void LCD::setRedTimeColor()
{
    timeColor = ILI9341_RED;
}

void LCD::setWhiteTimeColor()
{
    timeColor = ILI9341_WHITE;
}