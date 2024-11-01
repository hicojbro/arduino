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
String currentCardUID = "";        // 儲存當前卡片的 UID

// 光線強度閾值
const int LIGHT_THRESHOLD = 1500;  // 調高啟用偵測的光線閾值
const int DARK_THRESHOLD = 800;    // 調低停止偵測的光線閾值

void printSystemStatus(int lightValue) {
  static unsigned long lastPrintTime = 0;
  unsigned long currentTime = millis();
  
  // 每秒列印一次狀態
  if (currentTime - lastPrintTime >= 1000) {
    Serial.println("------- 系統狀態同步 -------");
    Serial.print("光線強度: ");
    Serial.println(lightValue);
   
    Serial.print("卡片讀取完成狀態: ");
    Serial.println(cardReadCompleted ? "已完成" : "未完成");
   
    Serial.print("RFID偵測啟用狀態: ");
    Serial.println(rfidDetectionEnabled ? "已啟用" : "已停用");
   
    Serial.print("當前卡片UID: ");
    Serial.println(currentCardUID.length() > 0 ? currentCardUID : "無");
   
    Serial.println("-------------------------");
   
    lastPrintTime = currentTime;
  }
}

void resetRFIDSystem() {
    // 重置系統狀態變數
    cardReadCompleted = false;
    rfidDetectionEnabled = true;
    currentCardUID = "";
    
    // RC522 硬體重置步驟
    rfid.PCD_Reset();           // 重置RC522硬體
    rfid.PCD_Init();           // 重新初始化RC522
    
    // 清除可能殘留的卡片選擇
    rfid.PICC_HaltA();         // 停止當前卡片通訊
    rfid.PCD_StopCrypto1();    // 停止所有加密通訊
    
    // 重新設置RC522天線增益
    rfid.PCD_SetAntennaGain(rfid.RxGain_max);
    
    // 驗證模組狀態
    if (!rfid.PCD_PerformSelfTest()) {
        Serial.println("警告: RFID模組重置後自檢失敗！");
    } else {
        Serial.println("RFID模組重置成功");
    }
    
    // 系統重置日誌
    Serial.println("===== 系統完整重置 =====");
    Serial.println("卡片讀取完成狀態: 未完成");
    Serial.println("RFID偵測啟用狀態: 已啟用");
    Serial.println("當前卡片UID: 已清除");
    Serial.println("RC522狀態: 已重置");
    Serial.println("====================");
    
    delay(50); // 給予硬體穩定的時間
}

void setup() {
  Serial.begin(115200); // 初始化序列埠
 
  // 初始化 SPI
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SDA_PIN);
 
  // 初始化 RFID 模組
  rfid.PCD_Init();      
 
  // 設置光敏電阻為輸入
  pinMode(LIGHT_SENSOR_PIN, INPUT);
 
  // 確認 RFID 模組初始化狀態
  if (!rfid.PCD_PerformSelfTest()) {
    Serial.println("RFID模組初始化失敗，請檢查連接！");
    while (true);
  }
 
  Serial.println("系統初始化完成");
}

void loop() {
  // 讀取光敏電阻值（反轉邏輯，使高數值代表亮光）
  int lightValue = 4095 - analogRead(LIGHT_SENSOR_PIN);
  
  // 狀態同步列印
  printSystemStatus(lightValue);

  // 如果已讀取卡片
  if (cardReadCompleted) {
    // 檢查光線狀態
    if (lightValue < DARK_THRESHOLD) {
      // 光線太暗，停止 RFID 偵測
      rfidDetectionEnabled = false;
      Serial.println("===== 光線變暗 =====");
      Serial.println("光線太暗，暫停 RFID 偵測");
      Serial.println("RFID偵測啟用狀態: 已停用");
      Serial.println("====================");
    } else if (lightValue > LIGHT_THRESHOLD) {
    // 光線充足，重新啟用 RFID 偵測
      Serial.println("===== 光線變亮 =====");
      Serial.println("光線充足，執行完整系統重置");
    
      resetRFIDSystem();  // 使用改進的重置函數
    }else {
      // 光線在中間範圍，保持當前狀態
      Serial.println("光線處於中間範圍，維持當前狀態");
    }
  }

  // 只有在 RFID 偵測啟用且未完成卡片讀取時才進行偵測
  if (rfidDetectionEnabled && !cardReadCompleted) {
    // 檢查是否有新卡片進入範圍
    if (rfid.PICC_IsNewCardPresent()) {
      // 檢查卡片是否可讀取
      if (rfid.PICC_ReadCardSerial()) {
        // 組裝當前卡片 UID
        String uid = "";
        for (byte i = 0; i < rfid.uid.size; i++) {
          if (rfid.uid.uidByte[i] < 0x10) {
            uid += "0"; // 補零
          }
          uid += String(rfid.uid.uidByte[i], HEX);
        }
       
        // 設置卡片讀取狀態
        cardReadCompleted = true;
        currentCardUID = uid;
        
        // 詳細的偵測日誌
        Serial.println("===== 卡片偵測 =====");
        Serial.print("偵測到卡片 UID: ");
        Serial.println(currentCardUID);
        Serial.println("卡片讀取完成狀態: 已完成");
        Serial.println("====================");
       
        // 停止讀取當前卡片
        rfid.PICC_HaltA();
      } else {
        Serial.println("無法讀取卡片序列號！");
      }
    }
  }
 
  delay(100); // 延遲避免過度讀取
}
