#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi設定
const char* ssid = "LAPTOP-H200NENE 7672";
const char* password = "11024211";
const char* serverUrl = "http://192.168.137.1:3002/card"; // 更新為實際的API端點

// RFID 模組接線 (RC522)
#define SDA_PIN 5    
#define RST_PIN 0    
#define LIGHT_SENSOR_PIN 34  
#define SCK_PIN 18   
#define MOSI_PIN 23  
#define MISO_PIN 19  

MFRC522 rfid(SDA_PIN, RST_PIN);

// 全域變數
bool cardReadCompleted = false;   
bool rfidDetectionEnabled = true; 
String currentCardUID = "";       

const int LIGHT_THRESHOLD = 1500;  
const int DARK_THRESHOLD = 800;    

// 更新卡片狀態到伺服器
bool updateCardStatus(const String& cardUid, bool condition) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi未連接!");
    return false;
  }

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  // 準備JSON數據
  StaticJsonDocument<1024> doc;
  JsonArray cards = doc.createNestedArray("cards");
  
  JsonObject card = cards.createNestedObject();
  card["machineid"] = cardUid;
  card["condition"] = condition;

  String jsonString;
  serializeJson(doc, jsonString);

  // 發送PUT請求
  int httpResponseCode = http.PUT(jsonString);
  
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  Serial.print("發送的JSON: ");
  Serial.println(jsonString);

  http.end();

  return httpResponseCode == 200;
}

void printSystemStatus(int lightValue) {
  static unsigned long lastPrintTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastPrintTime >= 1000) {
    Serial.println("\n------- 系統狀態同步 -------\n");
    Serial.print("WiFi狀態: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "已連接" : "未連接");
    Serial.print("光線強度: ");
    Serial.println(lightValue);
    Serial.print("卡片讀取完成狀態: ");
    Serial.println(cardReadCompleted ? "已完成" : "未完成");
    Serial.print("RFID偵測啟用狀態: ");
    Serial.println(rfidDetectionEnabled ? "已啟用" : "已停用");
    Serial.print("當前卡片UID: ");
    Serial.println(currentCardUID.length() > 0 ? currentCardUID : "無");
    Serial.println("-------------------------\n");
    
    lastPrintTime = currentTime;
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  Serial.println("初始化開始...");

  // 連接WiFi
  WiFi.begin(ssid, password);
  Serial.println("連接WiFi中...");
  
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi連接成功!");
    Serial.print("IP位址: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi連接失敗!");
  }
  
  // 初始化 SPI
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SDA_PIN);
  delay(50);
  
  // 初始化 RFID 模組
  rfid.PCD_Init();
  delay(100);
  
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);
  
  if (!rfid.PCD_PerformSelfTest()) {
    Serial.println("RFID 模組測試失敗！");
    Serial.println("請檢查接線是否正確：");
    Serial.println("SDA (SS) -> GPIO 5");
    Serial.println("SCK -> GPIO 18");
    Serial.println("MOSI -> GPIO 23");
    Serial.println("MISO -> GPIO 19");
    Serial.println("RST -> GPIO 0");
    while (1);
  }
  
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  
  Serial.println("初始化完成!");
  Serial.print("RFID 軟體版本: 0x");
  byte version = rfid.PCD_ReadRegister(rfid.VersionReg);
  Serial.println(version, HEX);
}

void loop() {
  int lightValue = 4095 - analogRead(LIGHT_SENSOR_PIN);
  
  printSystemStatus(lightValue);
  
  if (rfidDetectionEnabled) {
    // 增加重置邏輯
    rfid.PCD_StopCrypto1();
    delay(50);  // 增加小延遲，讓讀卡器有時間重置
    
    if (!rfid.PICC_IsNewCardPresent()) {
      delay(50);  // 如果沒有卡片，稍微等待後再試
      return;
    }
    
    // 增加重試機制
    int retryCount = 0;
    const int MAX_RETRIES = 3;
    
    while (retryCount < MAX_RETRIES) {
      if (rfid.PICC_ReadCardSerial()) {
        Serial.println("成功讀取卡片序號!");
        
        String uid = "";
        for (byte i = 0; i < rfid.uid.size; i++) {
          if (rfid.uid.uidByte[i] < 0x10) {
            uid += "0";
          }
          uid += String(rfid.uid.uidByte[i], HEX);
        }
        
        uid.toUpperCase();
        
        // 印出詳細的卡片資訊用於診斷
        Serial.println("\n===== 詳細卡片資訊 =====");
        Serial.print("卡片類型: ");
        MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
        Serial.println(rfid.PICC_GetTypeName(piccType));
        Serial.print("UID 大小: ");
        Serial.println(rfid.uid.size);
        Serial.print("UID bytes: ");
        for (byte i = 0; i < rfid.uid.size; i++) {
          Serial.print(rfid.uid.uidByte[i], HEX);
          Serial.print(" ");
        }
        Serial.println();
        Serial.print("完整UID: ");
        Serial.println(uid);
        Serial.print("SAK: 0x");
        Serial.println(rfid.uid.sak, HEX);
        Serial.println("=========================\n");
        
        // 更新狀態
        cardReadCompleted = true;
        currentCardUID = uid;
        
        // 發送更新請求到伺服器
        if (updateCardStatus(uid, true)) {
          Serial.println("成功更新伺服器狀態!");
        } else {
          Serial.println("更新伺服器狀態失敗!");
        }
        
        rfid.PICC_HaltA();
        break;  // 成功讀取，跳出重試迴圈
        
      } else {
        retryCount++;
        if (retryCount < MAX_RETRIES) {
          Serial.print("讀取失敗，重試第 ");
          Serial.print(retryCount);
          Serial.println(" 次");
          delay(100);  // 重試前稍作等待
        } else {
          Serial.println("已達最大重試次數，讀取失敗");
        }
      }
    }
  }
  
  if (lightValue < DARK_THRESHOLD && cardReadCompleted) {
    if (rfidDetectionEnabled) {
      rfidDetectionEnabled = false;
      Serial.println("\n環境變暗且已讀取卡片,停止 RFID 偵測");
    }
  } else if (lightValue > LIGHT_THRESHOLD) {
    if (!rfidDetectionEnabled || cardReadCompleted) {
      rfidDetectionEnabled = true;
      cardReadCompleted = false;
      currentCardUID = "";
      Serial.println("\n環境變亮,重新啟動 RFID 偵測");
      rfid.PCD_Init();
      rfid.PCD_SetAntennaGain(rfid.RxGain_max);
    }
  }
  
  delay(100);
}
