#include "BotControl.h"
#include "common.h"

Bot::Bot(WiFiClientSecure &client)
{
    // Store client reference for proper socket management
    this->client = &client;
    
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
    
    // Configure client timeouts to prevent hanging
    client.setTimeout(5000);  // 5 second timeout
    client.setConnectTimeout(3000);  // 3 second connect timeout
    
    bot = new UniversalTelegramBot(BOTtoken, client);
    // Reduce long poll timeout to prevent socket issues
    bot->longPoll = 10;  // Shorter polling to prevent socket exhaustion
    
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
        
        // Reset connection if we've had too many consecutive errors
        if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
            resetConnection();
        }
        
        // Take mutex to coordinate HTTP requests with shorter timeout
        if (xSemaphoreTakeRecursive(httpMutex, pdMS_TO_TICKS(100)) == pdTRUE) {  // Reduced to 100ms
            Serial.println("Bot: Checking for new messages");
            int numNewMessages = 0;
            bool requestSuccessful = false;
            
            try {
                Serial.println("Bot: Getting Telegram updates");
                
                // Add pre-request check
                if (!client || !client->connected()) {
                    Serial.println("Bot: Client not connected, attempting reconnection");
                    resetConnection();
                }
                
                numNewMessages = bot->getUpdates(bot->last_message_received + 1);
                Serial.println("Bot: Got Telegram updates");
                requestSuccessful = true;
                
                // Reset error count on successful request
                consecutiveErrors = 0;
            } catch (...) {
                Serial.println("Bot: Exception getting Telegram updates");
                consecutiveErrors++;
                requestSuccessful = false;
            }
            
            if (requestSuccessful && numNewMessages > 0) {
                // Process only a limited number of messages
                const int MAX_MESSAGES_PER_CYCLE = 3; // Further reduced
                int messagesToProcess = min(numNewMessages, MAX_MESSAGES_PER_CYCLE);
                
                Serial.printf("Bot: Processing %d of %d new messages\n", messagesToProcess, numNewMessages);
                handleNewMessages(messagesToProcess);    
                Serial.println("Bot: Finished processing messages");        
            }
            
            // Release mutex quickly
            Serial.println("Bot: normal mutex release");
            xSemaphoreGive(httpMutex);
        } else {
            Serial.println("Bot: Failed to acquire HTTP mutex - skipping update");
            consecutiveErrors++;
        }
        
        lastTimeBotRan = millis();
    }
}

void Bot::resetConnection()
{
    Serial.println("Bot: Resetting connection due to socket errors");
    
    // Stop and restart the client to clear bad socket state
    if (client) {
        client->stop();
        delay(100);  // Small delay to ensure proper cleanup
        
        // Reconfigure client settings
        client->setCACert(TELEGRAM_CERTIFICATE_ROOT);
        client->setTimeout(5000);
        client->setConnectTimeout(3000);
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
    if (xSemaphoreTakeRecursive(httpMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
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