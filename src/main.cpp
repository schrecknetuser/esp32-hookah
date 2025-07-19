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

LCD *lcd;

InputModule *inputModule;
OutputModule *outputModule;

void mainLoop(void *context)
{
  while (true)
  {
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
    vTaskDelay(1);
  }
}

void botLoop(void *context)
{
  while (true)
  {
    inputModule->pollBot();
    vTaskDelay(1);
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Setup started");

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
  esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
  
  // CPU power management removed due to compatibility issues

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  lcd = new LCD();

  inputModule = new InputModule(lcd);
  outputModule = new OutputModule(lcd);

  xTaskCreatePinnedToCore(botLoop, "botLoop", 4096*16, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(mainLoop, "mainLoop", 4096, NULL, 1, NULL, 0);

  Serial.println("Setup finished");
}

void loop() {}
