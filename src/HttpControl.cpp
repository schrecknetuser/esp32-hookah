#include "HttpControl.h"

// Define the URL in flash memory to save RAM
const char HttpControl::setPrimaryUrl[] = "http://led.haven/neon_led_control/led_profiles/set_primary";

void HttpControl::sendRequest(JsonDocument& doc)
{
    HTTPClient http;
    
    doc["led_profile_name"] = PROFILE_NAME;
    
    // Use a char buffer instead of String for request body
    char requestBody[256];  // Fixed size buffer
    serializeJson(doc, requestBody, sizeof(requestBody));

    http.begin(setPrimaryUrl);
    http.addHeader("Content-Type", "application/json");
    http.POST(requestBody);
}

void HttpControl::setPrimarySecondary(bool primary)
{
    JsonDocument doc;
    doc["primary"] = primary;
    sendRequest(doc);
}

void HttpControl::setPercentage(int percentage)
{
    JsonDocument doc;
    doc["percentage"] = percentage;
    sendRequest(doc);
}

void HttpControl::setPrimary()
{
    setPrimarySecondary(true);
}

void HttpControl::setSecondary()
{
    setPrimarySecondary(false);
}