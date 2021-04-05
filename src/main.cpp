#include <Arduino.h>
#define ESPALEXA_ASYNC
//#define ESPALEXA_DEBUG
#include <Espalexa.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#define SPIFFS LittleFS
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include <ESPAsyncWiFiManager.h>
#include <AccelStepper.h>
#include <SPIFFSEditor.h>

#include "Page_Admin.h"

#define EN_PIN           D1 // Enable
#define DIR_PIN          D3 // Direction
#define STEP_PIN         D4 // Step
constexpr uint32_t steps_per_mm = 80;
uint maxSpeed = 14;
uint accel = 7;

AccelStepper stepper = AccelStepper(stepper.DRIVER, STEP_PIN, DIR_PIN);
int direction = 1;
AsyncWebServer httpServer(80);
DNSServer dns;
Espalexa espalexa;
//fauxmoESP fauxmo;
bool running = false;
void switchOnKinetic(uint8_t brightness);
bool loadDefaultSettings();
bool saveDefaultSettings();
void printFile();

unsigned long previousMillis = 0; 
unsigned long interval = 1000;

void setup() {
    Serial.begin(74880);

    WiFi.setAutoConnect(true);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFi.persistent(true);
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin("DUSHYANT", "ahuja987");
      while (WiFi.status() != WL_CONNECTED) {
          Serial.print(".");
          delay(3000);
      }
    }
    Serial.println("Wifi Setup Completed");
    
    loadDefaultSettings();

    randomSeed(analogRead(A0));
    stepper.setMaxSpeed(maxSpeed*steps_per_mm); // 20mm/s @ 80 steps/mm
    stepper.setAcceleration(accel*steps_per_mm); // 5mm/s^2
    stepper.setEnablePin(EN_PIN);
    stepper.setPinsInverted(false, false, true);
    
    if (!SPIFFS.begin())
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    httpServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", PAGE_AdminMainPage);
    });
    httpServer.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/style.css.gz", "text/css");
        response->addHeader("Content-Encoding", "gzip");
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
        saveDefaultSettings();
        request->send_P(200, "text/html", PAGE_AdminMainPage);
    });
    httpServer.on("/Decrease", HTTP_GET, [](AsyncWebServerRequest *request) {
        maxSpeed--;
        if (maxSpeed == 0) maxSpeed = 1;
        stepper.setMaxSpeed(maxSpeed*steps_per_mm);
        saveDefaultSettings();
        request->send_P(200, "text/html", PAGE_AdminMainPage);
    });
    httpServer.on("/IncAcc", HTTP_GET, [](AsyncWebServerRequest *request) {
        accel++;
        stepper.setAcceleration(accel*steps_per_mm);
        saveDefaultSettings();
        request->send_P(200, "text/html", PAGE_AdminMainPage);
    });
    httpServer.on("/DecAcc", HTTP_GET, [](AsyncWebServerRequest *request) {
        accel--; 
        if (accel ==0) accel = 1;
        stepper.setAcceleration(accel*steps_per_mm);
        saveDefaultSettings();
        request->send_P(200, "text/html", PAGE_AdminMainPage);
    });
    httpServer.on("/save", HTTP_GET, [](AsyncWebServerRequest *request) {
        if(!saveDefaultSettings()){
            Serial.println("Unable to Save");
            request->send_P(200, "text/plain", "Not Saved");
        }
        else
            request->send_P(200, "text/plain", "Saved");
    });
    httpServer.onNotFound([](AsyncWebServerRequest *request){
      if (!espalexa.handleAlexaApiCall(request)) //if you don't know the URI, ask espalexa whether it is an Alexa control request
      {
        //whatever you want to do with 404s
        String message;
        message = "Acceleration: ";
        message += String(accel)  + '\n';
        message += "Max Speed: ";
        message += String(maxSpeed)  + '\n';
        request->send(404, "text/plain", message);
      }
    });
    httpServer.addHandler(new SPIFFSEditor("admin", "admin"));
    AsyncElegantOTA.begin(&httpServer);
    espalexa.addDevice("Kinetic", switchOnKinetic, 255);
    //httpServer.begin();
    espalexa.begin(&httpServer);
    //printFile();
    //Serial.println("");
}

void loop() {
    unsigned long currentMillis = millis();
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin("DUSHYANT", "ahuja987");
    }
    if (stepper.distanceToGo() == 0) {
        stepper.disableOutputs();
        //delay(random(10,100)*100);
        if(running && currentMillis - previousMillis >= interval) {
            stepper.move(random(500)*steps_per_mm*direction); // Move 100mm
            if(random(10)>=5) direction = -direction;
            interval = random(10,100)*100;
            previousMillis = currentMillis;
            stepper.enableOutputs();
        }
    }
    stepper.run();
    AsyncElegantOTA.loop();
    //fauxmo.handle();
    espalexa.loop();
}

void switchOnKinetic(uint8_t brightness) {
    if (brightness != 0) {
      running = true;
    }
    else {
      running = false;
    }
}

bool loadDefaultSettings(){
    //if (!SPIFFS.exists("/config.json")) return false;
    if (!SPIFFS.begin())
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return false;
    }
    File  configFile  =  SPIFFS.open("config.json","r"); 
    if(!configFile)  { 
        Serial.println( "Failed to open config file" ); 
        configFile.close();
        return  false ; 
    }

    size_t size  =  configFile.size(); 
    if(size > 256 )  { 
        Serial.println( "Config file size is too large" ); 
        configFile.close();
        return  false ; 
    }
    StaticJsonDocument<64> doc;
    DeserializationError error = deserializeJson(doc, configFile);

    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        configFile.close();
        return false;
    }
    serializeJsonPretty(doc,Serial);
    maxSpeed = doc["maxSpeed"];
    accel = doc["accel"];
    Serial.println(maxSpeed);
    Serial.println(accel);
    configFile.close();
    return true;
}

bool saveDefaultSettings(){
    DynamicJsonDocument doc(256);
    doc["maxSpeed"] = maxSpeed;
    doc["accel"] = accel;
    File configFile  =  SPIFFS.open("config.json","w"); 
    if  ( !configFile )  { 
        Serial.println( "Failed to open config file for writing" ); 
        return  false ; 
    }
    serializeJson(doc, configFile);
    serializeJsonPretty(doc, Serial);
    configFile.close();
    //SPIFFS.end();
    return true; 
}

void printFile() {
  // Open file for reading
  File file = SPIFFS.open("config.json","r"); 
  if (!file) {
    Serial.println(F("Failed to read file"));
    return;
  }

  // Extract each characters by one by one
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();

  // Close the file
  file.close();
}