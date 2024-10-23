# **使用手冊：Arduino Nano 33 IoT 偵測系統**

## 目錄
1. 硬體介紹與連接
   - 1.1 Arduino Nano 33 IoT
   - 1.2 MPU6050 感測器
   - 1.3 OLED 顯示器
   - 1.4 按鈕操作
   - 1.5 RGB LED
2. 軟體安裝與設定
   - 2.1 Arduino IDE 安裝
   - 2.2 必要庫安裝
3. 程式架構與代碼說明
   - 3.1 JSON 數據處理
   - 3.2 按鈕功能 (短按與長按)
   - 3.3 動作偵測與倒數計時
   - 3.4 OLED 顯示邏輯
   - 3.5 RGB LED 狀態顯示
4. 操作流程
   - 4.1 選擇偵測動作
   - 4.2 開始偵測與暫停
   - 4.3 倒數計時與數據顯示
5. 進階功能與擴展
   - 5.1 上傳資料到數據庫
   - 5.2 擴展功能與可能的修改

---

## 1. 硬體介紹與連接

### 1.1 Arduino Nano 33 IoT
Arduino Nano 33 IoT 是一款具備 WiFi 模塊的開發板，用於物聯網應用。它擁有強大的處理能力，可以進行感測數據處理和網路連接。

### 1.2 MPU6050 感測器
MPU6050 是一款六軸加速度計與陀螺儀感測器。此感測器可以偵測運動，如伏地挺身、仰臥起坐和深蹲。

- **接線方式**：
   - VCC → Arduino 3.3V
   - GND → Arduino GND
   - SCL → Arduino A5 (SCL)
   - SDA → Arduino A4 (SDA)

### 1.3 OLED 顯示器
OLED 顯示器用於顯示當前動作名稱、使用者資訊、動作次數及倒數計時。

- **接線方式（SPI 模式）**：
   - VCC → Arduino 3.3V
   - GND → Arduino GND
   - SCL → Arduino D13 (SCK)
   - SDA → Arduino D11 (MOSI)
   - RES → Arduino D9
   - DC → Arduino D8
   - CS → Arduino D10

### 1.4 按鈕操作
系統使用一個按鈕進行模式切換和偵測控制。  
- 短按：切換偵測動作  
- 長按：開始或暫停偵測

### 1.5 RGB LED
RGB LED 用於指示系統狀態：
- **紅色**：未開始偵測
- **藍色**：偵測進行中
- **綠色**：數據上傳成功

- **接線方式**：
   - R → Arduino D6
   - G → Arduino D5
   - B → Arduino D3

---

## 2. 軟體安裝與設定

### 2.1 Arduino IDE 安裝
請從 [Arduino 官方網站](https://www.arduino.cc/en/software) 下載並安裝 Arduino IDE。

### 2.2 必要庫安裝
在 Arduino IDE 中，依次打開「工具」→「庫管理器」，並安裝以下庫：
- **WiFiNINA**：用於 WiFi 連接
- **Adafruit_MPU6050**：用於 MPU6050 感測器
- **Adafruit_SSD1306**：用於 OLED 顯示器

---

## 3. 程式架構與代碼說明

### 3.1 JSON 數據處理
接收從伺服器端傳回的 JSON 格式數據，並解析更新顯示內容。

```cpp
#include <Arduino_JSON.h>

String jsonString = "{\"ID\":\"123\",\"name\":\"John\"}";
JSONVar myObject = JSON.parse(jsonString);

if (myObject.hasOwnProperty("ID") && myObject.hasOwnProperty("name")) {
  String id = (const char*)myObject["ID"];
  String name = (const char*)myObject["name"];
  // 更新 OLED 顯示
}
```

### 3.2 按鈕功能 (短按與長按)
按鈕短按用來切換動作，長按用來開始或暫停偵測。

- **按鈕邏輯**：
```cpp
int buttonPin = 2;
unsigned long pressTime = 0;
bool isLongPress = false;

void loop() {
  if (digitalRead(buttonPin) == LOW) {
    pressTime = millis();
    while (digitalRead(buttonPin) == LOW) { 
      if (millis() - pressTime > 1000) {
        isLongPress = true;
        // 長按處理：開始/暫停偵測
        break;
      }
    }
    if (!isLongPress) {
      // 短按處理：切換動作
    }
    isLongPress = false;
  }
}
```

### 3.3 動作偵測與倒數計時
偵測完成一個動作後開始倒數，並顯示剩餘時間。

```cpp
int count = 0;
int countdownTime = 60;

void detectMotion() {
  if (motionDetected()) {
    if (count == 0) {
      startCountdown();
    }
    count++;
  }
}

void startCountdown() {
  for (int i = countdownTime; i >= 0; i--) {
    // 顯示剩餘秒數到 OLED
    delay(1000);
  }
  // 顯示 "END"
}
```

### 3.4 OLED 顯示邏輯
顯示目前偵測動作、使用者名稱、動作次數及倒數計時。

```cpp
void displayInfo(String action, String user, int count, int seconds) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("動作: " + action);
  display.setCursor(0, 16);
  display.print("使用者: " + user);
  display.setCursor(0, 32);
  display.print("次數: " + String(count));
  display.setCursor(0, 48);
  display.print("倒數: " + String(seconds) + "s");
  display.display();
}
```

### 3.5 RGB LED 狀態顯示
根據不同狀態改變 RGB LED 顏色。

```cpp
void updateLED(int state) {
  if (state == 0) {
    // 停止狀態
    digitalWrite(redPin, HIGH);
  } else if (state == 1) {
    // 偵測進行中
    digitalWrite(bluePin, HIGH);
  } else if (state == 2) {
    // 上傳成功
    digitalWrite(greenPin, HIGH);
  }
}
```

---

## 4. 操作流程

### 4.1 選擇偵測動作
短按按鈕選擇要進行的動作，OLED 顯示當前選中的動作。

### 4.2 開始偵測與暫停
長按按鈕開始動作偵測，再次長按可以暫停偵測。

### 4.3倒數計時與數據顯示
動作偵測後進入 60 秒倒數，倒數結束後顯示 "END"。

---

## 5. 進階功能與擴展

### 5.1 上傳資料到數據庫
偵測完成後可將結果上傳至伺服器。

### 5.2 擴展功能與可能的修改
未來可以增加更多偵測動作，或改進偵測的靈敏度與準確性。

---
