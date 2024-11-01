#include <SPI.h>
#include <MFRC522.h>

// RFID 模組接線 (RC522)
#define SDA_PIN 5    // ESP32 上的 SDA 引腳
#define RST_PIN 0    // ESP32 上的 RST 引腳
// 光敏電阻接線
#define LIGHT_SENSOR_PIN 34  // ESP32 上的模擬輸入引腳
// RFID SPI 接線
#define SCK_PIN 18   // SPI 時脈線
#define MOSI_PIN 23  // SPI 主機輸出從機輸入線
#define MISO_PIN 19  // SPI 主機輸入從機輸出線

MFRC522 rfid(SDA_PIN, RST_PIN); // 建立 MFRC522 實例

// 全域變數
bool cardReadCompleted = false;   // 標誌位，表示是否已完成卡片讀取
bool rfidDetectionEnabled = true; // 控制 RFID 偵測是否啟用
String currentCardUID = "";       // 儲存當前卡片的 UID

// 光線強度閾值
const int LIGHT_THRESHOLD = 1500;  // 調高啟用偵測的光線閾值
const int DARK_THRESHOLD = 800;    // 調低停止偵測的光線閾值

void printSystemStatus(int lightValue) {
  static unsigned long lastPrintTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastPrintTime >= 1000) {
    Serial.println("\n------- 系統狀態同步 -------\n");
    
    Serial.print("光線強度: ");
    Serial.println(lightValue);
    Serial.println();
    
    Serial.print("卡片讀取完成狀態: ");
    Serial.println(cardReadCompleted ? "已完成" : "未完成");
    Serial.println();
    
    Serial.print("RFID偵測啟用狀態: ");
    Serial.println(rfidDetectionEnabled ? "已啟用" : "已停用");
    Serial.println();
    
    Serial.print("當前卡片UID: ");
    Serial.println(currentCardUID.length() > 0 ? currentCardUID : "無");
    Serial.println();
    
    Serial.println("-------------------------\n");
    
    lastPrintTime = currentTime;
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);  // 等待序列埠準備就緒
  
  Serial.println("初始化開始...");
  
  // 初始化 SPI
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SDA_PIN);
  delay(50);  // 等待 SPI 穩定
  
  // 初始化 RFID 模組
  rfid.PCD_Init();
  delay(100);  // 等待 RFID 模組穩定
  
  // 設置 RFID 天線增益
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);
  
  // 測試 RFID 模組
  if (!rfid.PCD_PerformSelfTest()) {
    Serial.println("RFID 模組測試失敗！");
    Serial.println("請檢查接線是否正確：");
    Serial.println("SDA (SS) -> GPIO 5");
    Serial.println("SCK -> GPIO 18");
    Serial.println("MOSI -> GPIO 23");
    Serial.println("MISO -> GPIO 19");
    Serial.println("RST -> GPIO 0");
    while (1);  // 如果測試失敗則停止
  }
  
  // 設置光敏電阻為輸入
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  
  Serial.println("初始化完成!");
  // 列印 RFID 版本
  Serial.print("RFID 軟體版本: 0x");
  byte version = rfid.PCD_ReadRegister(rfid.VersionReg);
  Serial.println(version, HEX);
}

void loop() {
  int lightValue = 4095 - analogRead(LIGHT_SENSOR_PIN);
  
  printSystemStatus(lightValue);
  
  if (rfidDetectionEnabled) {
    // 清除之前的通訊狀態
    rfid.PCD_StopCrypto1();
    
    // 檢查是否有新卡片
    if (rfid.PICC_IsNewCardPresent()) {
      Serial.println("偵測到卡片...");
      
      // 嘗試讀取卡片
      if (rfid.PICC_ReadCardSerial()) {
        Serial.println("成功讀取卡片序號!");
        
        // 組裝卡片 UID
        String uid = "";
        for (byte i = 0; i < rfid.uid.size; i++) {
          if (rfid.uid.uidByte[i] < 0x10) {
            uid += "0";
          }
          uid += String(rfid.uid.uidByte[i], HEX);
        }
        
        // 更新狀態
        cardReadCompleted = true;
        currentCardUID = uid;
        
        Serial.println("===== 卡片資訊 =====");
        Serial.print("卡片類型: ");
        MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
        Serial.println(rfid.PICC_GetTypeName(piccType));
        Serial.print("UID: ");
        Serial.println(uid);
        Serial.print("SAK: 0x");
        Serial.println(rfid.uid.sak, HEX);
        Serial.println("==================");
        
        // 停止卡片通訊
        rfid.PICC_HaltA();
      } else {
        Serial.println("讀取卡片失敗，請重試");
      }
    }
  }
  
  // 處理光線邏輯
  if (lightValue < DARK_THRESHOLD && cardReadCompleted) {
    if (rfidDetectionEnabled) {
      rfidDetectionEnabled = false;
      Serial.println("\n環境變暗且已讀取卡片，停止 RFID 偵測");
    }
  } else if (lightValue > LIGHT_THRESHOLD) {
    if (!rfidDetectionEnabled || cardReadCompleted) {
      rfidDetectionEnabled = true;
      cardReadCompleted = false;
      currentCardUID = "";
      Serial.println("\n環境變亮，重新啟動 RFID 偵測");
      rfid.PCD_Init();
      rfid.PCD_SetAntennaGain(rfid.RxGain_max);
    }
  }
  
  delay(100);  // 延遲以避免過度讀取
}
