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

#define DEBUG

#ifndef DEBUG_PRINT
#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#endif
#endif

#define EN_PIN           D1 // Enable
#define DIR_PIN          D3 // Direction
#define STEP_PIN         D4 // Step
constexpr uint32_t steps_per_mm = 80;
uint8_t maxSpeed = 14;
uint8_t accel = 7;
uint8_t mode = 0; // 0 - smooth sea-saw, 1 - Random

AccelStepper stepper = AccelStepper(stepper.DRIVER, STEP_PIN, DIR_PIN);
int direction = 1;
AsyncWebServer httpServer(80);
DNSServer dns;
Espalexa espalexa;
//fauxmoESP fauxmo;
bool running = false, switchOn = false;
void switchOnKinetic(uint8_t brightness);
bool loadDefaultSettings();
bool saveDefaultSettings();
void printFile();

unsigned long previousMillis = 0, previousOnMillis = 0; 
unsigned long interval = 1000;

void setup() {
    #ifdef DEBUG
        Serial.begin(74880);
    #endif

    WiFi.setAutoConnect(true);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFi.persistent(true);
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin("DUSHYANT-NEW", "ahuja987");
      while (WiFi.status() != WL_CONNECTED) {
          DEBUG_PRINT(".");
          delay(3000);
      }
    }
    DEBUG_PRINT("Wifi Setup Completed");
    DEBUG_PRINT(WiFi.localIP().toString());
    
    loadDefaultSettings();

    randomSeed(analogRead(A0));
    stepper.setMaxSpeed(maxSpeed*steps_per_mm); // 20mm/s @ 80 steps/mm
    stepper.setAcceleration(accel*steps_per_mm); // 5mm/s^2
    stepper.setEnablePin(EN_PIN);
    stepper.setPinsInverted(false, false, true);
    
    if (!SPIFFS.begin())
    {
        DEBUG_PRINT("An Error has occurred while mounting LittleFS");
        return;
    }
    httpServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String message = PAGE_AdminMainPage;
        message += "<HR><BR>Acceleration: ";
        message += String(accel)  + '\n';
        message += "<BR>Max Speed: ";
        message += String(maxSpeed)  + '\n';
        message += "<BR>Stop Delay: ";
        message += String(interval / 1000)  + '\n';
        request->send(200, "text/html", message);
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
        switchOn = true;
        previousOnMillis = millis();
        String message = PAGE_AdminMainPage;
        message += "<HR><BR>Acceleration: ";
        message += String(accel)  + '\n';
        message += "<BR>Max Speed: ";
        message += String(maxSpeed)  + '\n';
        message += "<BR>Stop Delay: ";
        message += String(interval / 1000)  + '\n';
        request->send(200, "text/html", message);
    });
    httpServer.on("/OFF", HTTP_GET, [](AsyncWebServerRequest *request) {
        running = false;
        switchOn = false;
        stepper.stop();
        String message = PAGE_AdminMainPage;
        message += "<HR><BR>Acceleration: ";
        message += String(accel)  + '\n';
        message += "<BR>Max Speed: ";
        message += String(maxSpeed)  + '\n';
        message += "<BR>Stop Delay: ";
        message += String(interval / 1000)  + '\n';
        request->send(200, "text/html", message);
    });
    httpServer.on("/Increase", HTTP_GET, [](AsyncWebServerRequest *request) {
        maxSpeed++;
        stepper.setMaxSpeed(maxSpeed*steps_per_mm);
        saveDefaultSettings();
        String message = PAGE_AdminMainPage;
        message += "<HR><BR>Acceleration: ";
        message += String(accel)  + '\n';
        message += "<BR>Max Speed: ";
        message += String(maxSpeed)  + '\n';
        message += "<BR>Stop Delay: ";
        message += String(interval / 1000)  + '\n';
        request->send(200, "text/html", message);
    });
    httpServer.on("/Decrease", HTTP_GET, [](AsyncWebServerRequest *request) {
        maxSpeed--;
        if (maxSpeed == 0) maxSpeed = 1;
        stepper.setMaxSpeed(maxSpeed*steps_per_mm);
        saveDefaultSettings();
        String message = PAGE_AdminMainPage;
        message += "<HR><BR>Acceleration: ";
        message += String(accel)  + '\n';
        message += "<BR>Max Speed: ";
        message += String(maxSpeed)  + '\n';
        message += "<BR>Stop Delay: ";
        message += String(interval / 1000)  + '\n';
        request->send(200, "text/html", message);
    });
    httpServer.on("/IncAcc", HTTP_GET, [](AsyncWebServerRequest *request) {
        accel++;
        stepper.setAcceleration(accel*steps_per_mm);
        saveDefaultSettings();
        String message = PAGE_AdminMainPage;
        message += "<HR><BR>Acceleration: ";
        message += String(accel)  + '\n';
        message += "<BR>Max Speed: ";
        message += String(maxSpeed)  + '\n';
        message += "<BR>Stop Delay: ";
        message += String(interval / 1000)  + '\n';
        request->send(200, "text/html", message);
    });
    httpServer.on("/Random", HTTP_GET, [](AsyncWebServerRequest *request) {
        //accel++;
        //stepper.setAcceleration(accel*steps_per_mm);
        //saveDefaultSettings();
        mode = 1;
        String message = PAGE_AdminMainPage;
        message += "<HR><BR>Acceleration: ";
        message += String(accel)  + '\n';
        message += "<BR>Max Speed: ";
        message += String(maxSpeed)  + '\n';
        message += "<BR>Stop Delay: ";
        message += String(interval / 1000)  + '\n';
        request->send(200, "text/html", message);
    });
    httpServer.on("/Smooth", HTTP_GET, [](AsyncWebServerRequest *request) {
        //accel++;
        //stepper.setAcceleration(accel*steps_per_mm);
        //saveDefaultSettings();
        mode = 0;
        String message = PAGE_AdminMainPage;
        message += "<HR><BR>Acceleration: ";
        message += String(accel)  + '\n';
        message += "<BR>Max Speed: ";
        message += String(maxSpeed)  + '\n';
        message += "<BR>Stop Delay: ";
        message += String(interval / 1000)  + '\n';
        request->send(200, "text/html", message);
    });
    httpServer.on("/DecAcc", HTTP_GET, [](AsyncWebServerRequest *request) {
        accel--; 
        if (accel ==0) accel = 1;
        stepper.setAcceleration(accel*steps_per_mm);
        saveDefaultSettings();
        String message = PAGE_AdminMainPage;
        message += "<HR><BR>Acceleration: ";
        message += String(accel)  + '\n';
        message += "<BR>Max Speed: ";
        message += String(maxSpeed)  + '\n';
        message += "<BR>Stop Delay: ";
        message += String(interval / 1000)  + '\n';
        request->send(200, "text/html", message);
    });
    httpServer.on("/save", HTTP_GET, [](AsyncWebServerRequest *request) {
        if(!saveDefaultSettings()){
            DEBUG_PRINT("Unable to Save");
            request->send_P(200, "text/plain", "Not Saved");
        }
        else
            request->send_P(200, "text/plain", "Saved");
    });
    httpServer.onNotFound([](AsyncWebServerRequest *request){
      if (!espalexa.handleAlexaApiCall(request)) //if you don't know the URI, ask espalexa whether it is an Alexa control request
      {
        //whatever you want to do with 404s
        String message = PAGE_AdminMainPage;
        message += "<HR><BR>Acceleration: ";
        message += String(accel)  + '\n';
        message += "<BR>Max Speed: ";
        message += String(maxSpeed)  + '\n';
        request->send(200, "text/html", message);
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
    if(mode == 1){
        if (stepper.distanceToGo() == 0) {
            stepper.disableOutputs();
            //delay(random(10,100)*100);
            if(running && currentMillis - previousMillis >= interval) {
                if(random(10)>=5) direction = -direction;
                stepper.move(random(1000)*steps_per_mm*direction); 
                interval = random(20,200)*100;
                previousMillis = currentMillis;
                stepper.enableOutputs();
            }
        }
    }
    else if(mode == 0){
        if (stepper.distanceToGo() == 0) {
            stepper.disableOutputs();
            //delay(random(10,100)*100);
            if(running && currentMillis - previousMillis >= interval) {
                direction = -direction;
                stepper.move(1000*steps_per_mm*direction); 
                interval = 1000;
                previousMillis = currentMillis;
                stepper.enableOutputs();
            }
        }
    }
    if(currentMillis - previousOnMillis >= 300000 && switchOn){
        running = !running;
        previousOnMillis = currentMillis;
    }
    stepper.run();
    //AsyncElegantOTA.loop();
    //fauxmo.handle();
    espalexa.loop();
}

void switchOnKinetic(uint8_t brightness) {
    if (brightness != 0) {
      running = true;
      switchOn = true;
    }
    else {
      running = false;
      switchOn = false;
      stepper.stop();
    }
}

bool loadDefaultSettings(){
    //if (!SPIFFS.exists("/config.json")) return false;
    if (!SPIFFS.begin())
    {
        DEBUG_PRINT("An Error has occurred while mounting SPIFFS");
        return false;
    }
    File  configFile  =  SPIFFS.open("config.json","r"); 
    if(!configFile)  { 
        DEBUG_PRINT( "Failed to open config file" ); 
        configFile.close();
        return  false ; 
    }

    size_t size  =  configFile.size(); 
    if(size > 256 )  { 
        DEBUG_PRINT( "Config file size is too large" ); 
        configFile.close();
        return  false ; 
    }
    StaticJsonDocument<64> doc;
    DeserializationError error = deserializeJson(doc, configFile);

    if (error) {
        DEBUG_PRINT(F("deserializeJson() failed: "));
        DEBUG_PRINT(error.f_str());
        configFile.close();
        return false;
    }
    serializeJsonPretty(doc,Serial);
    maxSpeed = doc["maxSpeed"];
    accel = doc["accel"];
    DEBUG_PRINT(maxSpeed);
    DEBUG_PRINT(accel);
    configFile.close();
    return true;
}

bool saveDefaultSettings(){
    DynamicJsonDocument doc(256);
    doc["maxSpeed"] = maxSpeed;
    doc["accel"] = accel;
    File configFile  =  SPIFFS.open("config.json","w"); 
    if  ( !configFile )  { 
        DEBUG_PRINT( "Failed to open config file for writing" ); 
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
    DEBUG_PRINT(F("Failed to read file"));
    return;
  }

  // Extract each characters by one by one
  while (file.available()) {
    DEBUG_PRINT((char)file.read());
  }
  DEBUG_PRINT();

  // Close the file
  file.close();
}