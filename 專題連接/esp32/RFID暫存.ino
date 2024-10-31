#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// RFID pins for ESP32
#define RST_PIN         22    
#define SS_PIN          21    
#define SCK_PIN         18    
#define MISO_PIN        19    
#define MOSI_PIN        23    

// Network credentials
const char* ssid = "LAPTOP-H200NENE 7672";
const char* password = "11024211";

// API endpoint
const char* API_URL = "http://192.168.137.1:3002/card";

MFRC522 mfrc522(SS_PIN, RST_PIN);
String lastCardUID = "";
bool cardDetected = false;
const unsigned long CHECK_INTERVAL = 1000; 

// Debug LED
#define LED_PIN 2

void blinkLED(int times) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }
}

bool initRFID() {
    Serial.println("Initializing RFID reader...");
    SPI.end();
    delay(100);
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
    SPI.setFrequency(4000000);
    digitalWrite(RST_PIN, LOW);
    delay(100);
    digitalWrite(RST_PIN, HIGH);
    delay(100);
    mfrc522.PCD_Init();
    delay(100);
    byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    Serial.print("MFRC522 Version: 0x");
    Serial.println(v, HEX);
    if (v == 0x91 || v == 0x92) {
        Serial.println("MFRC522 初始化成功");
        mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
        mfrc522.PCD_AntennaOn();
        return true;
    } else {
        Serial.println("MFRC522 初始化失敗");
        return false;
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("\nStarting setup...");
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    pinMode(RST_PIN, OUTPUT);

    if (!initRFID()) {
        Serial.println("RFID 初始化失敗！");
        while (1) {
            blinkLED(3);
            delay(1000);
        }
    }

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    int wifiAttempts = 0;
    while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
        delay(500);
        Serial.print(".");
        wifiAttempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        blinkLED(2);
    } else {
        Serial.println("\nWiFi connection failed!");
    }
    Serial.println("Setup complete - Ready to read cards");
    blinkLED(1);
}

String getCardUID() {
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) {
            uid += "0";
        }
        uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    Serial.print("Card UID: ");
    Serial.println(uid);
    return uid;
}

void updateCardStatus(String uid, bool isPresent) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(API_URL);
        int getResponse = http.GET();
        if (getResponse > 0) {
            String payload = http.getString();
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, payload);
            JsonArray cards = doc["cards"];
            bool found = false;
            for (JsonVariant card : cards) {
                if (card["machineid"] == uid) {
                    card["condition"] = isPresent;
                    found = true;
                    break;
                }
            }
            if (!found) {
                JsonObject newCard = cards.createNestedObject();
                newCard["machineid"] = uid;
                newCard["condition"] = isPresent;
            }
            String jsonString;
            serializeJson(doc, jsonString);
            http.begin(API_URL);
            http.addHeader("Content-Type", "application/json");
            int putResponse = http.PUT(jsonString);
            Serial.print("Update response code: ");
            Serial.println(putResponse);
            if (putResponse > 0) {
                Serial.println(isPresent ? "狀態更新為 true (卡片離開)" : "狀態更新為 false (偵測到卡片)");
                blinkLED(1);
            } else {
                blinkLED(2);
            }
        }
        http.end();
    }
}

bool isCardStillPresent() {
    if (!mfrc522.PICC_IsNewCardPresent()) {
        return false;
    }
    
    if (!mfrc522.PICC_ReadCardSerial()) {
        return false;
    }
    
    String currentUID = getCardUID();
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    
    return currentUID == lastCardUID;
}

void checkCardPresence() {
    if (!cardDetected) {
        // 等待新卡片
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
            String currentUID = getCardUID();
            lastCardUID = currentUID;
            cardDetected = true;
            updateCardStatus(currentUID, false);
            Serial.println("偵測到新卡片，等待卡片離開...");
            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
        }
    } else {
        // 檢查卡片是否已經離開
        if (!isCardStillPresent()) {
            delay(500);  // 等待一小段時間再次確認
            if (!isCardStillPresent()) {  // 再次確認卡片確實離開
                updateCardStatus(lastCardUID, true);
                Serial.println("卡片已離開，準備偵測新卡片...");
                lastCardUID = "";
                cardDetected = false;
            }
        }
    }
}

void loop() {
    checkCardPresence();
    delay(CHECK_INTERVAL);
}
