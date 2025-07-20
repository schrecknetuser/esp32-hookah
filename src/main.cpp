#include <Arduino.h>

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_wifi.h"

#include "LCD.h"
#include "InputModule.h"
#include "OutputModule.h"

// HTTP request coordination mutex
SemaphoreHandle_t httpMutex;

LCD *lcd;

InputModule *inputModule;
OutputModule *outputModule;

void mainLoop(void *context)
{
#if ENABLE_MEMORY_MONITORING
  static unsigned long lastMemoryCheck = 0;
  static size_t minFreeHeap = SIZE_MAX;  // Track minimum free heap
#endif

  while (true)
  {
#if ENABLE_MEMORY_MONITORING
    // Periodic memory monitoring
    unsigned long now = millis();
    if (now - lastMemoryCheck >= MEMORY_CHECK_INTERVAL_MS) {
      size_t currentHeap = ESP.getFreeHeap();
      if (currentHeap < minFreeHeap) {
        minFreeHeap = currentHeap;
      }
      Serial.printf("Free heap: %u bytes (min: %u bytes)\n", currentHeap, minFreeHeap);
      lastMemoryCheck = now;
    }
#endif

    inputModule->processRequests();
    if (inputModule->isStartRequested())
      outputModule->processStart();
    if (inputModule->isStopRequested())
      outputModule->processStop();
    if (inputModule->isResetRequested())
      outputModule->processReset();
    if (inputModule->isSetTimeRequested())
      outputModule->processSetTime(inputModule->getRequestedMinutes(), inputModule->getRequestedSeconds());

    // Reset power timers if any input was processed
    if (inputModule->isStartRequested() || inputModule->isStopRequested() || 
        inputModule->isResetRequested() || inputModule->isSetTimeRequested())
      outputModule->resetPowerTimers();

    inputModule->clearRequests();
    outputModule->processTick();
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_MS));
  }
}

void botLoop(void *context)
{
  // Re-enabled bot functionality with fresh client approach
  Serial.println("Bot: Starting bot loop with fresh client socket management");
  
  while (true)
  {
    inputModule->processBot();
    
    // Reset power timers if any bot input was processed
    if (inputModule->isStartRequested() || inputModule->isStopRequested() || 
        inputModule->isResetRequested() || inputModule->isSetTimeRequested())
      outputModule->resetPowerTimers();
    
    vTaskDelay(pdMS_TO_TICKS(BOT_LOOP_DELAY_MS));
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Setup started");
  
  // Initialize HTTP request coordination mutex
  httpMutex = xSemaphoreCreateMutex();
  if (httpMutex == NULL) {
    Serial.println("Failed to create HTTP mutex");
    ESP.restart();
  }
  
  // Print initial free heap memory
  Serial.print("Initial free heap: ");
  Serial.println(ESP.getFreeHeap());
  
  // Configure WiFi with power-efficient settings
  WiFi.setTxPower(WIFI_POWER_19_5dBm);  // Reduce TX power to save energy and RAM

  // Enable WiFi power saving
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, WIFIPASSWORD);

  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Configure WiFi power management
  //esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
  
  // CPU power management removed due to compatibility issues

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  lcd = new LCD();

  inputModule = new InputModule(lcd);
  outputModule = new OutputModule(lcd);

  // Print free heap before creating tasks
  Serial.print("Free heap before tasks: ");
  Serial.println(ESP.getFreeHeap());

  // Optimized stack sizes for RAM efficiency - larger botLoop for stable HTTP operations  
  xTaskCreatePinnedToCore(botLoop, "botLoop", 4096*10, NULL, 1, NULL, 1);  // Increased to 10KB for HTTP stability
  xTaskCreatePinnedToCore(mainLoop, "mainLoop", 4096*2, NULL, 1, NULL, 0); // Keep at 2KB

  // Print free heap after creating tasks
  Serial.print("Free heap after tasks: ");
  Serial.println(ESP.getFreeHeap());

  Serial.println("Setup finished");
}

void loop() {}
