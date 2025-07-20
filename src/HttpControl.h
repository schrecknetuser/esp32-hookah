#include "Arduino.h"
#include "HTTPClient.h"
#include "ArduinoJson.h"

#define MAX_OUTPUT_VALUE 255
#define PROFILE_NAME "LED1"
#define MAX_TRIES_COUNT 5

class HttpControl 
{    
public:

    void setPrimary();
    void setSecondary();
    void setPercentage(int percentage, bool setPrimary = false);
    
private:

    void sendRequest(JsonDocument &doc);
    void setPrimarySecondary(bool primary);
    static const char setPrimaryUrl[];
};