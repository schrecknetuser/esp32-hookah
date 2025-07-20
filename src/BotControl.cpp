#include "BotControl.h"
#include "common.h"

Bot::Bot(WiFiClientSecure &client)
{

    client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
    bot = new UniversalTelegramBot(BOTtoken, client);
    //bot->longPoll = 60;
    lastTimeBotRan = millis();
    clearRequests();
}

void Bot::clearRequests()
{
    startRequested = false;
    stopRequested = false;
    resetRequested = false;
    setTimeRequested = false;
}

void Bot::checkNewMessages()
{
    if (millis() > lastTimeBotRan + botRequestDelay)
    {
        // Check if WiFi is connected before making requests
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Bot: WiFi not connected, skipping bot check");
            lastTimeBotRan = millis();
            return;
        }
        
        // Take mutex to coordinate HTTP requests with other modules
        if (xSemaphoreTakeRecursive(httpMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
            Serial.println("Bot: Checking for new messages");
            int numNewMessages = 0;
            
            try {
                Serial.println("Bot: Getting Telegram updates");
                numNewMessages = bot->getUpdates(bot->last_message_received + 1);
                Serial.println("Bot: Got Telegram updates");
            } catch (...) {
                Serial.println("Bot: Error getting Telegram updates");
                xSemaphoreGive(httpMutex);
                lastTimeBotRan = millis();
                return;
            }

            while (numNewMessages)
            {            
                Serial.printf("Bot: Processing %d new messages\n", numNewMessages);
                handleNewMessages(numNewMessages);    
                Serial.println("Bot: Finished processing messages");        
                
                // Add safety check to prevent infinite loop
                int nextMessages = 0;
                try {
                    Serial.printf("last_message_received: %ld\n", bot->last_message_received);
                    nextMessages = bot->getUpdates(bot->last_message_received + 1);
                    Serial.printf("new last_message_received: %ld\n", bot->last_message_received);
                } catch (...) {
                    Serial.println("Bot: Error getting next Telegram updates");
                    break;
                }
                numNewMessages = nextMessages;
            }
            
            // Release mutex
            Serial.println("Bot: normal mutex release");
            xSemaphoreGive(httpMutex);
        } else {
            Serial.println("Bot: Failed to acquire HTTP mutex - skipping update");
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
    // Take mutex to coordinate HTTP requests
    Serial.println("Bot: Attempting to take semaphore in sendBotControlMessage");
    if (xSemaphoreTakeRecursive(httpMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        Serial.println("Bot: Successfully took semaphore in sendBotControlMessage");
        static const char message[] PROGMEM = "Hello";
        
        bot->sendMessage(chat_id, FPSTR(message), "");
        
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
        
        bot->sendMessageWithInlineKeyboard(chat_id, FPSTR(controlMsg), "", FPSTR(keyboardJson));
        
        // Release mutex
        Serial.println("Bot: normal mutex release in sendBotControlMessage");
        xSemaphoreGive(httpMutex);
    } else {
        Serial.println("Failed to acquire HTTP mutex for bot message - skipping");
    }
    Serial.println("Bot: Exiting sendBotControlMessage");
}

// Handle what happens when you receive new messages
void Bot::handleNewMessages(int numNewMessages)
{


    for (int i = 0; i < numNewMessages; i++)
    {
        String chat_id = String(bot->messages[i].chat_id);
        if (chat_id != CHAT_ID)
        {
            bot->sendMessage(chat_id, "Unauthorized user", "");
            continue;
        }

        String text = bot->messages[i].text;

        String from_name = bot->messages[i].from_name;

        if (text == START)
        {
            String welcome = "Welcome, " + from_name + ".\n";
            welcome += "Use the following commands to control your outputs.\n\n";
            welcome += "" RESET_REQUEST " - reset\n";
            welcome += "" SET_TIME_REQUEST " <time> - set cycle time\n";
            welcome += "" START_REQUEST " - start the timer\n";
            welcome += "" STOP_REQUEST " - stop the timer\n";
            bot->sendMessage(chat_id, welcome, "");
            sendBotControlMessage(chat_id);
        }

        if (text.startsWith(RESET_REQUEST))
            processResetCommand();            
        else if (text.startsWith(SET_TIME_REQUEST))
            processSetTimeCommand(text);
        else if (text.startsWith(START_REQUEST)) {
            Serial.printf("Calling processStartCommand for text: %s\n", text.c_str());
            processStartCommand();
            Serial.println("processStartCommand called successfully");
        }
        else if (text.startsWith(STOP_REQUEST))
            processStopCommand();        
        sendBotControlMessage(chat_id);
    }
}