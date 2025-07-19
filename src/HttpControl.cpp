#include "HttpControl.h"
#include "common.h"

// Define the URL in flash memory to save RAM
const char HttpControl::setPrimaryUrl[] = "http://led.haven/neon_led_control/led_profiles/set_primary";

void HttpControl::sendRequest(JsonDocument& doc)
{
    // Take mutex to coordinate HTTP requests
    if (xSemaphoreTake(httpMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        Serial.println("HttpControl: Sending LED control request");
        
        HTTPClient http;
        
        doc["led_profile_name"] = PROFILE_NAME;
        
        // Use a char buffer instead of String for request body
        char requestBody[256];  // Fixed size buffer
        serializeJson(doc, requestBody, sizeof(requestBody));

        http.begin(setPrimaryUrl);
        http.addHeader("Content-Type", "application/json");
        
        // Set timeouts to prevent hanging
        http.setTimeout(5000);  // 5 second timeout
        http.setConnectTimeout(3000);  // 3 second connect timeout
        
        int httpResponseCode = http.POST(requestBody);
        
        // Log the response for debugging
        if (httpResponseCode > 0) {
            Serial.printf("HTTP Response: %d\n", httpResponseCode);
        } else {
            Serial.printf("HTTP Error: %s\n", http.errorToString(httpResponseCode).c_str());
        }
        
        http.end();  // Free resources
        
        // Release mutex
        xSemaphoreGive(httpMutex);
    } else {
        Serial.println("HttpControl: Failed to acquire HTTP mutex - skipping request");
    }
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