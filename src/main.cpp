#include <Arduino.h>
#define ESPALEXA_ASYNC
#define ESPALEXA_DEBUG
#include <Espalexa.h>
#include <FS.h>
#include <LittleFS.h>
#define SPIFFS LittleFS
#include <ESP8266WiFi.h>
//#include "fauxmoESP.h"
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include <ESPAsyncWiFiManager.h>
#include <AccelStepper.h>
#include <SPIFFSEditor.h>

//#include <ArduinoJson.h>
#include "Page_Admin.h"

#define EN_PIN           D1 // Enable
#define DIR_PIN          D3 // Direction
#define STEP_PIN         D4 // Step
constexpr uint32_t steps_per_mm = 80;
uint maxSpeed = 5;
uint accel = 2;

AccelStepper stepper = AccelStepper(stepper.DRIVER, STEP_PIN, DIR_PIN);
int direction = 1;
AsyncWebServer httpServer(80);
DNSServer dns;
Espalexa espalexa;
//fauxmoESP fauxmo;
bool running = false;
void switchOnKinetic(uint8_t brightness);

void setup() {
    Serial.begin(74880);
    randomSeed(analogRead(A0));

    stepper.setMaxSpeed(maxSpeed*steps_per_mm); // 20mm/s @ 80 steps/mm
    stepper.setAcceleration(accel*steps_per_mm); // 5mm/s^2
    stepper.setEnablePin(EN_PIN);
    stepper.setPinsInverted(false, false, true);
    stepper.enableOutputs();

    WiFi.setAutoConnect(true);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    AsyncWiFiManager wifiManager(&httpServer, &dns);
    wifiManager.setTimeout(180);
    if (!wifiManager.autoConnect("Kinetic"))
    {
        delay(3000);
        ESP.reset();
        delay(5000);
    }
    Serial.println("Wifi Setup Completed");
    espalexa.addDevice("Kinetic", switchOnKinetic, 255);
    httpServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", PAGE_AdminMainPage);
    });
    httpServer.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/style.css", "text/css");
        //response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    httpServer.on("/microajax.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/microajax.js.gz", "text/plain");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    httpServer.on("/jscolor.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/jscolor.js.gz", "text/plain");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    httpServer.on("/ON", HTTP_GET, [](AsyncWebServerRequest *request) {
        running = true;
        request->send_P(200, "text/html", PAGE_AdminMainPage);
    });
    httpServer.on("/OFF", HTTP_GET, [](AsyncWebServerRequest *request) {
        running = false;
        request->send_P(200, "text/html", PAGE_AdminMainPage);
    });
    httpServer.on("/Increase", HTTP_GET, [](AsyncWebServerRequest *request) {
        maxSpeed++;
        stepper.setMaxSpeed(maxSpeed*steps_per_mm);
        request->send_P(200, "text/html", PAGE_AdminMainPage);
    });
    httpServer.on("/Decrease", HTTP_GET, [](AsyncWebServerRequest *request) {
        maxSpeed--;
        stepper.setMaxSpeed(maxSpeed*steps_per_mm);
        request->send_P(200, "text/html", PAGE_AdminMainPage);
    });
    httpServer.on("/IncAcc", HTTP_GET, [](AsyncWebServerRequest *request) {
        accel++;
        stepper.setAcceleration(accel*steps_per_mm);
        request->send_P(200, "text/html", PAGE_AdminMainPage);
    });
    httpServer.on("/DecAcc", HTTP_GET, [](AsyncWebServerRequest *request) {
        accel--;
        stepper.setAcceleration(accel*steps_per_mm);
        request->send_P(200, "text/html", PAGE_AdminMainPage);
    });
    httpServer.addHandler(new SPIFFSEditor("admin", "admin"));
    AsyncElegantOTA.begin(&httpServer);
    //httpServer.begin();
    espalexa.begin(&httpServer);
}

void loop() {
    if (stepper.distanceToGo() == 0) {
        stepper.disableOutputs();
        delay(random(10,100)*100);
        if(running) {
            stepper.move(random(100)*steps_per_mm*direction); // Move 100mm
            if(random(10)>=5) direction = -direction;
            stepper.enableOutputs();
        }
    }
    stepper.run();
    AsyncElegantOTA.loop();
    //fauxmo.handle();
    espalexa.loop();
}

void switchOnKinetic(uint8_t brightness) {
    if (brightness == 0) {
      running = false;
    }
    else {
      running = true;
    }
}