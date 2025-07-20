#include "BotControl.h"
#include "common.h"

Bot::Bot()
{
    lastTimeBotRan = millis();
    consecutiveErrors = 0;  // Initialize error counter
    clearRequests();
}

void Bot::clearRequests()
{
    startRequested = false;
    stopRequested = false;
    resetRequested = false;
    setTimeRequested = false;
}

WiFiClientSecure* Bot::createSecureClient()
{
    WiFiClientSecure* client = new WiFiClientSecure();
    
    // Configure the client for Telegram API
    client->setCACert(TELEGRAM_CERTIFICATE_ROOT);
    client->setTimeout(10000);  // 10 second timeout
    client->setConnectionTimeout(5000);  // 5 second connect timeout
    
    return client;
}

void Bot::checkNewMessages()
{
    int currentDelay = botRequestDelay;
    
    // Implement backoff if we've had consecutive errors
    if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
        currentDelay = ERROR_BACKOFF_DELAY;
        Serial.printf("Bot: In error backoff mode, delaying %d ms\n", currentDelay);
    }
    
    if (millis() > lastTimeBotRan + currentDelay)
    {
        // Check if WiFi is connected before making requests
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Bot: WiFi not connected, skipping bot check");
            lastTimeBotRan = millis();
            consecutiveErrors++; // Treat WiFi disconnection as error for backoff
            return;
        }
        
        // Take mutex to coordinate HTTP requests with reasonable timeout
        if (xSemaphoreTakeRecursive(httpMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            Serial.println("Bot: Checking for new messages");
            
            // Create a fresh client for this request to avoid socket reuse issues
            WiFiClientSecure* client = createSecureClient();
            UniversalTelegramBot bot(BOTtoken, *client);
            bot.longPoll = 10;  // Short polling to prevent socket issues
            
            int numNewMessages = 0;
            bool requestSuccessful = false;
            
            try {
                Serial.println("Bot: Getting Telegram updates with fresh client");
                
                numNewMessages = bot.getUpdates(bot.last_message_received + 1);
                Serial.printf("Bot: Got %d Telegram updates\n", numNewMessages);
                requestSuccessful = true;
                
                // Reset error count on successful request
                consecutiveErrors = 0;
                
                if (numNewMessages > 0) {
                    // Process only a limited number of messages
                    const int MAX_MESSAGES_PER_CYCLE = 3;
                    int messagesToProcess = min(numNewMessages, MAX_MESSAGES_PER_CYCLE);
                    
                    Serial.printf("Bot: Processing %d of %d new messages\n", messagesToProcess, numNewMessages);
                    
                    for (int i = 0; i < messagesToProcess; i++) {
                        String text = bot.messages[i].text;
                        String from_name = bot.messages[i].from_name;
                        String chat_id = bot.messages[i].chat_id;

                        Serial.println("Message: " + text);

                        if (text == RESET_REQUEST) {
                            processResetCommand();
                            bot.sendMessage(chat_id, "Reset requested", "");
                        } else if (text == START_REQUEST) {
                            processStartCommand();
                            bot.sendMessage(chat_id, "Start requested", "");
                        } else if (text == STOP_REQUEST) {
                            processStopCommand();
                            bot.sendMessage(chat_id, "Stop requested", "");
                        } else if (text.startsWith(SET_TIME_REQUEST)) {
                            processSetTimeCommand(text);
                            bot.sendMessage(chat_id, "Set time requested", "");
                        } else if (text == START) {
                            sendBotControlMessage(chat_id);
                        }
                    }
                    
                    Serial.println("Bot: Finished processing messages");        
                }
            } catch (...) {
                Serial.println("Bot: Exception getting Telegram updates");
                consecutiveErrors++;
                requestSuccessful = false;
            }
            
            // Clean up the client immediately after use
            client->stop();
            delete client;
            client = nullptr;
            
            // Release mutex quickly
            Serial.println("Bot: Releasing HTTP mutex");
            xSemaphoreGive(httpMutex);
        } else {
            Serial.println("Bot: Failed to acquire HTTP mutex - skipping update");
            consecutiveErrors++;
        }
        
        lastTimeBotRan = millis();
    }
}

void Bot::processSetTimeCommand(String text)
{
    char stringArray[2][MAX_STRING_LENGTH];  // Static allocation instead of String array
    char tempBuffer[MAX_STRING_LENGTH];
    
    strncpy(tempBuffer, text.c_str(), sizeof(tempBuffer) - 1);
    tempBuffer[sizeof(tempBuffer) - 1] = '\0';
    
    // Simple parsing: find space and split
    char* space = strchr(tempBuffer, ' ');
    if (!space) return;
    
    *space = '\0';  // Split the string
    char* timeStr = space + 1;
    
    sscanf(timeStr, "%d:%d", &requestedMinutes, &requestedSeconds);
    setTimeRequested = true;
}

void Bot::processResetCommand()
{
    resetRequested = true;
}

void Bot::processStartCommand()
{
    startRequested = true;
}

void Bot::processStopCommand()
{
    stopRequested = true;
}

void Bot::botSetup()
{
    /*bool result = bot->setMyCommands(F("["
                            "{\"command\":\"" RESET_TO_START "\", \"description\":\"Reset to night start\"},"
                            "{\"command\":\"" SET_TIME_AND_SESSION "\", \"description\":\"Set time and session\"},"
                            "{\"command\":\"" SET_TIME "\", \"description\":\"Set time\"},"
                            "{\"command\":\"" ADD_TIME "\", \"description\":\"Add time\"},"
                            "{\"command\":\"" UPDATE_FROM_SERVER "\", \"description\":\"Update from server\"}"
                            "]"
    ));*/
}

void Bot::sendBotControlMessage(String &chat_id)
{
    // Create a fresh client for sending messages to avoid socket reuse issues
    WiFiClientSecure* client = createSecureClient();
    UniversalTelegramBot tempBot(BOTtoken, *client);
    
    static const char message[] PROGMEM = "Hello";
    tempBot.sendMessage(chat_id, FPSTR(message), "");
    
    // Store keyboard JSON in flash memory to save RAM
    static const char keyboardJson[] PROGMEM = 
        "["
        "[{\"text\":\"Reset\", \"callback_data\":\"" RESET_REQUEST "\"}, {\"text\":\"Start\", \"callback_data\":\"" START_REQUEST "\"}," 
        "{\"text\":\"Stop\", \"callback_data\":\"" STOP_REQUEST "\"}]," 
        "[{\"text\":\"5 min\", \"callback_data\":\"" SET_TIME_REQUEST " 5:00\"}," 
        "{\"text\":\"7 min\", \"callback_data\":\"" SET_TIME_REQUEST " 7:00\"},"
        "{\"text\":\"10 min\", \"callback_data\":\"" SET_TIME_REQUEST " 10:00\"}]"
        "]";
    
    static const char controlMsg[] PROGMEM = "Choose from one of the following options";
    tempBot.sendMessageWithInlineKeyboard(chat_id, FPSTR(controlMsg), "", FPSTR(keyboardJson));
    
    // Clean up the client immediately after use
    client->stop();
    delete client;
    client = nullptr;
}