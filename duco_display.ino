#include <TFT_eSPI.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "logo.h"
#include "configuration.h"

TFT_eSPI ipsdisp = TFT_eSPI();
unsigned long loadingStartTime = 0;

double lastBalance = -1.0;
int lastMinerCount = -1;
double lastTotalHashrate = -1.0;
int lastDifficulty = -1;
double lastMaxAccepted = -1.0;

bool dataLoaded = false;
bool wLanConnected = false;

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting");

  ipsdisp.begin();
  ipsdisp.init();
  ipsdisp.setRotation(1);
  ipsdisp.setSwapBytes(true);

  bool connectionSuccess = false;
  int prevWiFiStatus = -1;

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);

    if (WiFi.status() != prevWiFiStatus) {
      Serial.print(".");
      ipsdisp.fillScreen(TFT_WHITE);
      ipsdisp.pushImage(0, 0, 320, 240, myGraphic);
      displayMessage("Verbinde WLAN...", 190, 70, TFT_BLACK);
      prevWiFiStatus = WiFi.status();
    }

    if (WiFi.status() == WL_CONNECTED) {
      connectionSuccess = true;
      break;
    }
  }

  if (connectionSuccess) {
    ipsdisp.fillScreen(TFT_WHITE);
    ipsdisp.pushImage(0, 0, 320, 240, myGraphic);
    displayMessage("Daten werden geladen...", 190, 20, TFT_BLACK);
    ipsdisp.fillScreen(TFT_ORANGE);
    dataLoaded = true;
  } else {
    ipsdisp.fillScreen(TFT_RED);
    displayMessage("Verbindung fehlgeschlagen!", 10, 10, TFT_WHITE);
  }
}

void loop() {
  if (dataLoaded) {
    if (millis() - loadingStartTime >= 5000) {
      if (WiFi.status() == WL_CONNECTED) {
        displayData();
      }
    }
  } else {
    ipsdisp.fillScreen(TFT_ORANGE);
    displayMessage("Daten werden geladen...", 10, 10, TFT_WHITE);
  }

  Serial.println("Warte 10 Sekunden vor der nächsten Runde...");
  delay(10000);
}

void displayData() {
  WiFiClientSecure secured_client;
  secured_client.setInsecure();

  HTTPClient https;

  Serial.print("[HTTPS] begin...\n");
  String serverUrl = String(DUCO_SERVER) + DUCO_USERNAME;

  if (https.begin(secured_client, serverUrl)) {
    Serial.print("[HTTPS] GET...\n");
    int httpCode = https.GET();

    if (httpCode > 0) {
      Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

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
        double totalHashrate = 0.0;
        int difficulty = 0;
        double maxAccepted = 0.0;
        int minerCount = 0;

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
        }

        totalHashrate /= 1000.0;

        if (balance != lastBalance || minerCount != lastMinerCount ||
            totalHashrate != lastTotalHashrate || difficulty != lastDifficulty ||
            maxAccepted != lastMaxAccepted) {
          displayInfo(balance, minerCount, totalHashrate, difficulty, maxAccepted);

          lastBalance = balance;
          lastMinerCount = minerCount;
          lastTotalHashrate = totalHashrate;
          lastDifficulty = difficulty;
          lastMaxAccepted = maxAccepted;
        }

      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        displayMessage("Kein Benutzer oder HTTP-Fehler!", 10, 10, TFT_WHITE);
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] Verbindung fehlgeschlagen\n");
      displayMessage("Kein Internet!", 10, 10, TFT_WHITE);
    }
  }
}

void displayInfo(double balance, int minerCount, double totalHashrate, int difficulty, double maxAccepted) {
  ipsdisp.fillScreen(TFT_ORANGE);
  displayMessage("Benutzer: " + String(DUCO_USERNAME), 10, 10, TFT_WHITE);
  displayMessage("Guthaben: " + String(balance) + " Duco", 40, 10, TFT_WHITE);

  if (minerCount < DUCO_MINER) {
    int diff = DUCO_MINER - minerCount;
    String message = String(diff) + " Miner sind offline.";
    displayMessage("Miner: " + String(minerCount), 70, 10, TFT_RED);
    displayMessage("Achtung! ", 100, 10, TFT_RED);
    displayMessage(String(message), 130, 10, TFT_RED);
  } else {
    displayMessage("Miner: " + String(minerCount), 70, 10, TFT_WHITE);
    displayMessage("Hashrate: " + String(totalHashrate) + " kH/s", 100, 10, TFT_WHITE);
    displayMessage("Schwierigkeit: " + String(difficulty), 130, 10, TFT_WHITE);
    displayMessage("Max. Akzeptiert: " + String(maxAccepted), 160, 10, TFT_WHITE);
  }

  displayMessage("IP: " + WiFi.localIP().toString(), 190, 10, TFT_WHITE);
}

void displayMessage(const String &message, int yPos, int xPos, uint16_t textColor) {
  ipsdisp.setCursor(xPos, yPos, 4);
  ipsdisp.setTextColor(textColor);
  ipsdisp.println(message);
}
