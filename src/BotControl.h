#pragma once

#include <Arduino.h>
#include <UniversalTelegramBot.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>

#include "secrets.h"

#define MAX_STRING_LENGTH 50

class Bot
{

#define RESET_REQUEST "/reset"
#define START_REQUEST "/start_request"
#define STOP_REQUEST "/stop_request"
#define SET_TIME_REQUEST "/set_time"
#define START "/start"

public:
    Bot(WiFiClientSecure &client);

    void checkNewMessages();

    bool isResetRequested(){return resetRequested;};
    bool isStartRequested(){return startRequested;};
    bool isStopRequested(){return stopRequested;};
    bool isSetTimeRequested(){return setTimeRequested;};

    void clearRequests();

    int getRequestedMinutes(){return requestedMinutes;}
    int getRequestedSeconds(){return requestedSeconds;}

private:

    void processSetTimeCommand(String text);
    void processResetCommand();
    void processStartCommand();
    void processStopCommand();

    void botSetup();

    void sendBotControlMessage(String &chat_id);

    void handleNewMessages(int numNewMessages);

    UniversalTelegramBot *bot;

    // Checks for new messages every 1 second.
    int botRequestDelay = 1000;
    unsigned long lastTimeBotRan;

    bool resetRequested;
    bool startRequested;
    bool stopRequested;
    bool setTimeRequested;

    int requestedMinutes;
    int requestedSeconds;
};