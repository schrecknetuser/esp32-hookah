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
    Bot();  // Changed: No client parameter - we'll create fresh clients

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
    WiFiClientSecure* createSecureClient(); // Create fresh client for each request

    void sendBotControlMessage(String &chat_id);

    void handleNewMessages(int numNewMessages);

    // Checks for new messages every 5 seconds to reduce HTTP load and prevent socket issues
    int botRequestDelay = 5000;  // Increased from 3s to 5s
    unsigned long lastTimeBotRan;

    // Error recovery state
    int consecutiveErrors = 0;
    static const int MAX_CONSECUTIVE_ERRORS = 3;
    static const int ERROR_BACKOFF_DELAY = 15000; // 15 seconds (increased)

    bool resetRequested;
    bool startRequested;
    bool stopRequested;
    bool setTimeRequested;

    int requestedMinutes;
    int requestedSeconds;
};