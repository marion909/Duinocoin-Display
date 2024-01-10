/*
 *Display images on ST7789 IPS module sketch
 *Made by wiztaqnia.com
 *Modified date: 18/04/2022
 *Typical pin layout used:
 * ---------------------------------------------
 * Signal               ST7789 IPS    ESP8266
 *                      Module        Interface
 *                      Pin           Pin
 * ---------------------------------------------
 * VCC(3.3V)            VCC           3V3
 * GND(Ground)          GND           GND
 * SDA(Serial Data)     DIN           D7(MOSI)
 * SLK(Serial Clock)    CLK           D5(SCLK)
 * CS(Chip Select)      CS            D8(CS)
 * DC(Data/Command)     DC            D0
 * RST(Reset)           RST           D1
 * BL(Backlight)        BL            D2
  */

#include <TFT_eSPI.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include "logo.h"

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";


String serverName = "https://server.duinocoin.com/users/";
String username = "YOUR_DUINOCOIN_USERNAME";

unsigned long lastTime = 0;
unsigned long timerDelay = 5000;


TFT_eSPI ipsdisp= TFT_eSPI();           

void setup() {
 Serial.begin(115200);        //Initialise UART COmmunication with 115200 bits/s Baud Rate
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 ipsdisp.begin();             //Initiatise SPI Bus
 ipsdisp.init();              //Initialise ST7789
 ipsdisp.setRotation(1);      //Value 1 means landescape mode; Value 2 means potrait mode
 ipsdisp.setSwapBytes(true);  //Swap the byte order for pushImage() - corrects endianness
 ipsdisp.fillScreen(TFT_WHITE);
 ipsdisp.pushImage(40,0,240,240,myGraphic);
 delay(5000); 
 ipsdisp.fillScreen(TFT_ORANGE);
}

void loop() {
// wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

    // Ignore SSL certificate validation
    client->setInsecure();
    
    //create an HTTPClient instance
    HTTPClient https;
    
    //Initializing an HTTPS communication using the secure client
    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, serverName + username)) {  // HTTPS
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();
      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();
          
          DynamicJsonDocument doc(2048);
          DeserializationError error = deserializeJson(doc, payload);

           if (error) {
      Serial.print(F("Fehler beim Parsen: "));
      Serial.println(error.c_str());
      return;
    }

    double balance = doc["result"]["balance"]["balance"];

Serial.print(F("Balance: "));
    Serial.println(balance);

      // Summe der Hashraten aller Miner
    double totalHashrate = 0.0;
    int minerCount = 0;
    int difficulty = 0;
     double maxAccepted = 0.0;


  // Schleife durch alle Miner
  JsonArray miners = doc["result"]["miners"];
  for (JsonObject miner : miners) {
    double hashrate = miner["hashrate"];
    totalHashrate += hashrate;
    difficulty = miner["diff"];
    int accepted = miner["accepted"];


    if (accepted > maxAccepted) {
          maxAccepted = accepted;
          
        }

    minerCount++;
    Serial.println(difficulty);

  }
    totalHashrate /= 1000.0;
          ipsdisp.fillScreen(TFT_ORANGE);
          ipsdisp.setCursor(10,10,4);               //ipsdisp.setCurser(x axis,y axis,text font style i.e 1/2/4/6)
          ipsdisp.setTextColor(TFT_WHITE); //ipsdisp.setTextColor(text color,text background color)
          ipsdisp.println("User: " + String(username));


          ipsdisp.setCursor(10,40,4);               //ipsdisp.setCurser(x axis,y axis,text font style i.e 1/2/4/6)
          ipsdisp.setTextColor(TFT_WHITE); //ipsdisp.setTextColor(text color,text background color)
          ipsdisp.println("Balance: " + String(balance) + " Duco");


          ipsdisp.setCursor(10,70,4);               //ipsdisp.setCurser(x axis,y axis,text font style i.e 1/2/4/6)
          ipsdisp.setTextColor(TFT_WHITE); //ipsdisp.setTextColor(text color,text background color)
          ipsdisp.println("Miner: " + String(minerCount));


          ipsdisp.setCursor(10,100,4);               //ipsdisp.setCurser(x axis,y axis,text font style i.e 1/2/4/6)
          ipsdisp.setTextColor(TFT_WHITE); //ipsdisp.setTextColor(text color,text background color)
          ipsdisp.println("Hash: " + String(totalHashrate) + " kH/s");


          ipsdisp.setCursor(10,130,4);               //ipsdisp.setCurser(x axis,y axis,text font style i.e 1/2/4/6)
          ipsdisp.setTextColor(TFT_WHITE); //ipsdisp.setTextColor(text color,text background color)
          ipsdisp.println("Difficulty: " + String(difficulty));


          ipsdisp.setCursor(10,160,4);               //ipsdisp.setCurser(x axis,y axis,text font style i.e 1/2/4/6)
          ipsdisp.setTextColor(TFT_WHITE); //ipsdisp.setTextColor(text color,text background color)
          ipsdisp.println("Max Shares: " + String(maxAccepted));


          ipsdisp.setCursor(10,190,4);               //ipsdisp.setCurser(x axis,y axis,text font style i.e 1/2/4/6)
          ipsdisp.setTextColor(TFT_WHITE); //ipsdisp.setTextColor(text color,text background color)
          ipsdisp.println("IP: " + WiFi.localIP().toString());

        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }
  Serial.println();
  Serial.println("Waiting 10sec before the next round...");
  delay(10000);



}

        
