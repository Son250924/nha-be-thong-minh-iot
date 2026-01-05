#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <ESP32Servo.h>
#include <Preferences.h> 

// --- THƯ VIỆN BLUETOOTH ---
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <TimeLib.h> 

// --- THƯ VIỆN MPU6050 ---
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// =========================================================
// 1. CẤU HÌNH CHÂN (PINOUT)
// =========================================================
#define LORA_M0 25  
#define LORA_M1 26  
#define LORA_AUX 27 
#define RX_LORA 16  
#define TX_LORA 17  

#define RX_GPS 12       
#define TX_GPS 13       

#define PIN_DHT 14      
#define PIN_SOIL_1 34   
#define PIN_SOIL_2 35   

#define PIN_RELAY_1 32 // Bơm
#define PIN_RELAY_2 21 // Đèn 
#define PIN_RELAY_3 15 // Quạt
 
#define PIN_BUTTON 4
#define I2C_SDA 18      
#define I2C_SCL 19

#define PIN_MQ2     33     
#define PIN_BUZZER  5      
#define PIN_SERVO   22     // Cửa Sổ
#define PIN_LDR     39     

// --- CẤU HÌNH LOGIC RIÊNG ---
#define LAMP_ON_LEVEL  LOW
#define LAMP_OFF_LEVEL HIGH

#define PUMP_ON_LEVEL  HIGH
#define PUMP_OFF_LEVEL LOW

#define FAN_ON_LEVEL   HIGH
#define FAN_OFF_LEVEL  LOW

// --- THAM SỐ CỨNG ---
#define SOIL_HYSTERESIS 15 
#define LIGHT_HYSTERESIS 200 
#define PUMP_CYCLE_DURATION 120000 
#define LORA_MSG_TIMEOUT 5000

#define DRY_VAL 4095   
#define WET_VAL 1500   

// --- CẤU HÌNH GPS UBX ---
const byte UBX_PEDESTRIAN_MODE[] = {
  0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 
  0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0xDC
};

// --- CẤU HÌNH BLUETOOTH UUID ---
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_TX "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_RX "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"

// =========================================================
// 2. KHAI BÁO BIẾN TOÀN CỤC
// =========================================================
BLEServer* pServer = NULL;
BLECharacteristic* pTxCharacteristic = NULL;
BLECharacteristic* pRxCharacteristic = NULL;
bool deviceConnected = false;     
bool authAccepted = false;        
bool isAuthPending = false;       
uint16_t currentConnId = 0;       

DHT dht(PIN_DHT, DHT11);
LiquidCrystal_I2C lcd(0x27, 16, 2); 
HardwareSerial gpsSerial(1); 
TinyGPSPlus gps;
Adafruit_MPU6050 mpu;
Servo windowServo; 
Preferences preferences; 

// Biến cấu hình
int limitGas = 1800;      
int limitTemp = 40;       
int limitSoil = 60;       
float limitWave = 14.0;   
int limitLight = 2000;    
bool autoLampMode = true; 

// Biến trạng thái
bool gasAlert = false;
bool waveAlert = false; 
unsigned long waveAlertStartTime = 0;
unsigned long lastGasCheck = 0;
const unsigned long GAS_INTERVAL = 500; 
unsigned long lastBleAdvertise = 0; 
unsigned long lastWaveCheck = 0; 
unsigned long lastGPSDebug = 0; 
unsigned long lastLightCheck = 0;
const unsigned long LIGHT_INTERVAL = 1000;

float temp = 0;
float humi = 0;
int soil1 = 0;
int soil2 = 0;
int gasValueDisplay = 0; 
int lightValue = 0;       

bool pumpState = false;
bool lampState = false; 
bool fanState = false; 
bool windowState = false; 
bool autoPumpMode = true; 
float currentAccel = 9.8; 

unsigned long pumpStartTime = 0; 
bool isPumpRunningTimer = false; 
unsigned long timeOffset = 0;     
bool hasSyncedTime = false;   

String lastLoRaMessage = "Chua co tin"; 
unsigned long lastLoRaMessageTime = 0; 

bool hasLoRa = true;
bool hasDHT = true;
bool hasGPS = false; 
bool hasMPU = false;

int lastButtonState = HIGH;   
int currentButtonState;       
unsigned long lastDebounceTime = 0;  
unsigned long debounceDelay = 30; 
unsigned long buttonPressTime = 0;
bool isPressing = false;
int clickCount = 0;
unsigned long lastClickReleaseTime = 0;
const int MULTI_CLICK_TIMEOUT = 300; 

bool isSOSCountingDown = false;
int sosCountdownVal = 5;
int lcdPage = 0; // 0:Main, 1:Soil, 2:GPS1, 3:GPS2, 4:Gas, 5:Msg, 10:Menu
int menuIndex = 0; 
unsigned long lastSensorRead = 0;
unsigned long lastLcdUpdate = 0;

// =========================================================
// 3. KHAI BÁO PROTOTYPE
// =========================================================
void controlPump(bool state);
void controlLamp(bool state); 
void controlFan(bool state); 
void controlWindow(bool state); 
void showGasAlertLCD(int gasValue);
void showWaveAlertLCD(float accel);
void showAuthRequestLCD();
void checkHardware();
void initLCD(); 
void sendLoRaMessage(String msg);
void refreshLCD(); 
void loadSettings(); 
void saveSettings(); 
void updateLCD();
void sendSOS();
void showPageMain();
void showPageSoil();
void showPageSignal1(); // Trang GPS 1: Sat + HDOP
void showPageSignal2(); // Trang GPS 2: Lat + Lon
void showPageGas();
void showPageMenu(); 
void showPageMsg();
void showSOSCountdown(); 
void startSOSCountdown(); 
void handleButtonV9();   
void processClicks(int count);
void processLongPress();
int readSoil(int pin);
int readLDR(); 
void sendUBX(const byte *command, int length); 

// =========================================================
// 4. BLE CALLBACKS
// =========================================================
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) { 
      deviceConnected = true;
      currentConnId = param->connect.conn_id;
      authAccepted = false;
      isAuthPending = true; 
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      authAccepted = false;
      isAuthPending = false;
    }
};

class MyBotCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      if (!authAccepted) return; 
      String rxValue = String(pCharacteristic->getValue().c_str());
      if (rxValue.length() > 0) {
        String cmd = String(rxValue.c_str());
        
        // --- XỬ LÝ NHẮN TIN APP -> CENTER (NEW) ---
        if (cmd.startsWith("MSG|")) {
            String msgContent = cmd.substring(4); // Lấy nội dung sau "MSG|"
            String loraPacket = "MSG_APP|" + msgContent;
            sendLoRaMessage(loraPacket);
        }
        else if (cmd == "SOS") {
           sendSOS(); 
        }
        else if (cmd == "PUMP_ON") { autoPumpMode = false; controlPump(true); }
        else if (cmd == "PUMP_OFF") { autoPumpMode = false; controlPump(false); }
        else if (cmd == "LAMP_ON") { controlLamp(true); }
        else if (cmd == "LAMP_OFF") { controlLamp(false); }
        else if (cmd == "FAN_ON") { controlFan(true); }
        else if (cmd == "FAN_OFF") { controlFan(false); }
        else if (cmd == "SERVO_OPEN") { controlWindow(true); } 
        else if (cmd == "SERVO_CLOSE") { controlWindow(false); } 
        else if (cmd == "AUTO_ON") autoPumpMode = true; 
        else if (cmd == "AUTO_OFF") autoPumpMode = false;
        else if (cmd == "AUTO_LAMP_ON") { autoLampMode = true; saveSettings(); }
        else if (cmd == "AUTO_LAMP_OFF") { autoLampMode = false; saveSettings(); }
        else if (cmd.startsWith("TIME|")) {
           String timeStr = cmd.substring(5);
           unsigned long unixTime = strtoul(timeStr.c_str(), NULL, 10);
           timeOffset = unixTime - (millis() / 1000);
           hasSyncedTime = true;
        }
          // Format: SET_LIMITS|Gas|Temp|Soil|Wave|Light
        else if (cmd.startsWith("SET_LIMITS|")) {
            int p1 = cmd.indexOf('|');
            int p2 = cmd.indexOf('|', p1 + 1);
            int p3 = cmd.indexOf('|', p2 + 1);
            int p4 = cmd.indexOf('|', p3 + 1);
            int p5 = cmd.indexOf('|', p4 + 1); 

            if (p1 > 0 && p2 > 0 && p3 > 0 && p4 > 0) {
                String gasStr = cmd.substring(p1 + 1, p2);
                String tempStr = cmd.substring(p2 + 1, p3);
                String soilStr = cmd.substring(p3 + 1, p4);
                String waveStr = cmd.substring(p4 + 1, p5 > 0 ? p5 : cmd.length());
                String lightStr = (p5 > 0) ? cmd.substring(p5 + 1) : "2000"; 

                limitGas = gasStr.toInt();
                limitTemp = tempStr.toInt();
                limitSoil = soilStr.toInt();
                limitWave = waveStr.toFloat();
                if(p5 > 0) limitLight = lightStr.toInt(); 

                saveSettings(); 
                Serial.println(">> DA CAP NHAT CAU HINH MOI");
                lcd.clear(); lcd.setCursor(0,0); lcd.print("CAU HINH DA LUU!");
                delay(1000); 
                refreshLCD();
        }
      }
    }
  }
};

// =========================================================
// 5. SETUP
// =========================================================
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  
  pinMode(PIN_RELAY_1, OUTPUT); digitalWrite(PIN_RELAY_1, PUMP_OFF_LEVEL);
  pinMode(PIN_RELAY_2, OUTPUT); digitalWrite(PIN_RELAY_2, LAMP_OFF_LEVEL);
  pinMode(PIN_RELAY_3, OUTPUT); digitalWrite(PIN_RELAY_3, FAN_OFF_LEVEL);
 
  WiFi.mode(WIFI_OFF); 
  Serial.begin(115200);
  delay(500); 

  Serial.println("\n>>> Dang khoi dong...");

  loadSettings();

  pinMode(PIN_MQ2, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  
  windowServo.attach(PIN_SERVO);
  windowServo.write(0); 

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_SOIL_1, INPUT);
  pinMode(PIN_SOIL_2, INPUT);
  
  pinMode(LORA_M0, OUTPUT); 
  pinMode(LORA_M1, OUTPUT);
  pinMode(LORA_AUX, INPUT); 

  digitalWrite(LORA_M0, LOW);
  digitalWrite(LORA_M1, LOW);
  
  Serial2.begin(9600, SERIAL_8N1, RX_LORA, TX_LORA);
  
  Wire.begin(I2C_SDA, I2C_SCL);
  initLCD(); 

  gpsSerial.begin(9600, SERIAL_8N1, RX_GPS, TX_GPS);
  delay(100);
  sendUBX(UBX_PEDESTRIAN_MODE, sizeof(UBX_PEDESTRIAN_MODE)); 
  Serial.println(">> GPS Configured: Pedestrian Mode");

  checkHardware();

  BLEDevice::init("NHA_BE_SMART"); 
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());
  pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyBotCallbacks());
  pService->start();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();
  Serial.println("Khoi dong hoan tat");
  
  lcd.clear(); 
}

// =========================================================
// 6. LOOP
// =========================================================
void loop() {
  unsigned long currentMillis = millis();

  if (lastLoRaMessage != "Chua co tin" && lastLoRaMessageTime > 0) {
      if (currentMillis - lastLoRaMessageTime > LORA_MSG_TIMEOUT) {
          lastLoRaMessage = "Chua co tin";
          lastLoRaMessageTime = 0;
          if (lcdPage == 5) { lcd.clear(); updateLCD(); } // Cập nhật nếu đang ở trang tin nhắn (Index 5)
      }
  }

  if (pumpState && isPumpRunningTimer) {
      if (currentMillis - pumpStartTime >= PUMP_CYCLE_DURATION) {
          controlPump(false); 
          isPumpRunningTimer = false;
          Serial.println("Pump Timer: Tuoi xong");
      }
  }

  while (gpsSerial.available() > 0) {
      char c = gpsSerial.read();
      gps.encode(c);
  }
  
  if (Serial2.available()) {
    String incoming = Serial2.readStringUntil('\n');
    incoming.trim(); 
    
    if (incoming.length() > 0) {
        Serial.print("Received: "); 
        Serial.println(incoming);

        if (incoming == "LAMP_ON") { controlLamp(true); sendLoRaMessage("DEN: ON"); }
        else if (incoming == "LAMP_OFF") { controlLamp(false); sendLoRaMessage("DEN: OFF"); }
        else if (incoming == "FAN_ON") { controlFan(true); sendLoRaMessage("QUAT: ON"); }
        else if (incoming == "FAN_OFF") { controlFan(false); sendLoRaMessage("QUAT: OFF"); }
        else if (incoming == "SERVO_OPEN") { controlWindow(true); sendLoRaMessage("CUA: MO"); }
        else if (incoming == "SERVO_CLOSE") { controlWindow(false); sendLoRaMessage("CUA: DONG"); }
      
        else if (incoming.startsWith("MSG_CENTER|")) {
            lastLoRaMessage = incoming; // Lưu nội dung để BLE gửi đi trong loop
            lastLoRaMessageTime = millis();
            if (lcdPage != 10 && !isSOSCountingDown && !gasAlert && !waveAlert && !isAuthPending) {
              lcdPage = 5; lcd.clear(); updateLCD();
            }
        }
        else {
            lastLoRaMessage = incoming;
            lastLoRaMessageTime = millis(); 
            if (lcdPage != 10 && !isSOSCountingDown && !gasAlert && !waveAlert && !isAuthPending) {
              lcdPage = 5; lcd.clear(); updateLCD();
            }
        }
    }
  }

  handleButtonV9();

  if (isSOSCountingDown) {
      if (currentMillis - lastLcdUpdate > 1000) {
          lastLcdUpdate = currentMillis;
          sosCountdownVal--;
          if (sosCountdownVal <= 0) { isSOSCountingDown = false; sendSOS(); } 
          else { showSOSCountdown(); }
      }
      return; 
  }

  if (hasMPU && (currentMillis - lastWaveCheck >= 200)) { 
    lastWaveCheck = currentMillis;
    sensors_event_t a, g, temp_mpu;
    mpu.getEvent(&a, &g, &temp_mpu);
    float totalVector = sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z);
    currentAccel = totalVector; 
    
    if (abs(totalVector - 9.8) > (limitWave - 9.8)) { 
       if (!waveAlert && !gasAlert) { 
          waveAlert = true; waveAlertStartTime = currentMillis; digitalWrite(PIN_BUZZER, HIGH); 
       }
       if (waveAlert && !gasAlert) showWaveAlertLCD(totalVector);
    }
    
    if (waveAlert && abs(totalVector - 9.8) < (limitWave - 9.8)) {
        if (currentMillis - waveAlertStartTime > 5000) {
            waveAlert = false; digitalWrite(PIN_BUZZER, LOW); lcd.clear(); updateLCD();
        }
    }
  }

  if (currentMillis - lastSensorRead > 2000 || lastSensorRead == 0) {
    lastSensorRead = currentMillis == 0 ? 1 : currentMillis; 
    if (hasDHT) {
      float t = dht.readTemperature(); float h = dht.readHumidity();
      if (!isnan(t)) temp = t; else temp = -99;
      if (!isnan(h)) humi = h; else humi = -99;
    } else { temp = -99; humi = -99; }
    
    soil1 = readSoil(PIN_SOIL_1); soil2 = readSoil(PIN_SOIL_2);
    int avgSoil = (soil1 + soil2) / 2;
    
    if (autoPumpMode) { 
      if (!pumpState) {
         if (avgSoil < limitSoil) {
             controlPump(true); 
             isPumpRunningTimer = true; 
             pumpStartTime = currentMillis;
             Serial.println("Auto Pump: START");
         }
      } else {
         if (avgSoil > (limitSoil + SOIL_HYSTERESIS)) {
             controlPump(false);
             isPumpRunningTimer = false;
             Serial.println("Auto Pump: STOP");
         }
      }
    }
    
    if (!waveAlert && !gasAlert && lcdPage != 10 && lcdPage != 5) updateLCD(); 
  }

  if (currentMillis - lastLcdUpdate > 1000) {
    lastLcdUpdate = currentMillis;
    if (!waveAlert && !gasAlert && lcdPage == 0) updateLCD();
  }

  if (currentMillis - lastGasCheck >= GAS_INTERVAL) {
    lastGasCheck = currentMillis;
    int gasValue = analogRead(PIN_MQ2);
    gasValueDisplay = gasValue; 
    
    if (deviceConnected && authAccepted) {
        int avgSoil = (soil1 + soil2) / 2;
        String latStr = "0.000000"; String lonStr = "0.000000"; int satCount = 0;
        if (gps.location.isValid()) {
            latStr = String(gps.location.lat(), 6);
            lonStr = String(gps.location.lng(), 6);
            satCount = gps.satellites.value();
        }
        String safeLoRaMsg = lastLoRaMessage; safeLoRaMsg.replace("|", "-"); 
        
        String packet = String(gasValue) + "|" + String((int)temp) + "|" + String((int)humi) + "|" + 
                        String(avgSoil) + "|" + String(pumpState ? 1 : 0) + "|" + String(autoPumpMode ? 1 : 0) + "|" +
                        String(waveAlert ? 1 : 0) + "|" + String((int)currentAccel) + "|" + 
                        String(lampState ? 1 : 0) + "|" + String(fanState ? 1 : 0) + "|" + 
                        latStr + "|" + lonStr + "|" + String(satCount) + "|" + safeLoRaMsg + "|" + 
                        String(windowState ? 1 : 0) + "|" + String(lightValue) + "|" + String(autoLampMode ? 1 : 0);
                        
        pTxCharacteristic->setValue(packet.c_str());
        pTxCharacteristic->notify();
    }
    if (!deviceConnected) {
        if (currentMillis - lastBleAdvertise > 500) { lastBleAdvertise = currentMillis; pServer->startAdvertising(); }
    }
    
    if (gasValue > limitGas) {
      if (!gasAlert) { 
        gasAlert = true; digitalWrite(PIN_BUZZER, HIGH); controlWindow(true); controlFan(true);
      }
      showGasAlertLCD(gasValue);
    } else if (gasValue < (limitGas - 100)) {
      if (gasAlert) { 
        gasAlert = false; digitalWrite(PIN_BUZZER, LOW); controlWindow(false); controlFan(false);
        lcd.clear(); updateLCD(); 
      }
    }
  }

  if (currentMillis - lastLightCheck >= LIGHT_INTERVAL) {
    lastLightCheck = currentMillis;
    lightValue = readLDR();
    
    if (autoLampMode) {
      if (lightValue > limitLight) { 
        if (!lampState) {
          lampState = true; digitalWrite(PIN_RELAY_2, LAMP_ON_LEVEL);
          Serial.println("Auto Lamp: ON (Dark)"); updateLCD();
        }
      } else if (lightValue < (limitLight - LIGHT_HYSTERESIS)) { 
        if (lampState) {
          lampState = false; digitalWrite(PIN_RELAY_2, LAMP_OFF_LEVEL);
          Serial.println("Auto Lamp: OFF (Bright)"); updateLCD();
        }
      }
    }
  }
}

// =========================================================
// 7. CÁC HÀM HỖ TRỢ
// =========================================================

void sendUBX(const byte *command, int length) {
  for (int i = 0; i < length; i++) {
    gpsSerial.write(command[i]);
  }
}

void checkHardware() {
  dht.begin();
  delay(100);
  float t = dht.readTemperature();
  if (isnan(t)) { delay(100); t = dht.readTemperature(); }
  if (isnan(t)) { hasDHT = false; Serial.println("!! DHT: Loi!"); } 
  else { hasDHT = true; Serial.println(">> DHT: OK"); }

  hasMPU = false;
  if (mpu.begin(0x68) || mpu.begin(0x69)) {
    hasMPU = true; Serial.println(">> MPU6050: OK");
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  } else { Serial.println("!! MPU6050: Loi!"); }

  unsigned long startWait = millis();
  bool gpsDataFound = false;
  Serial.print(">> GPS Check: ");
  while (millis() - startWait < 1000) {
    if (gpsSerial.available() > 0) { gpsDataFound = true; break; }
  }
  if (gpsDataFound) { hasGPS = true; Serial.println("OK"); } 
  else { hasGPS = false; Serial.println("NO DATA"); }
}

void loadSettings() {
    preferences.begin("config", true); 
    limitGas = preferences.getInt("gas", 1800);
    limitTemp = preferences.getInt("temp", 40);
    limitSoil = preferences.getInt("soil", 60);
    limitWave = preferences.getFloat("wave", 14.0);
    limitLight = preferences.getInt("light", 2000);
    autoLampMode = preferences.getBool("autoLamp", true);
    autoPumpMode = preferences.getBool("autoPump", true); 
    preferences.end();
}

void saveSettings() {
    preferences.begin("config", false);
    preferences.putInt("gas", limitGas);
    preferences.putInt("temp", limitTemp);
    preferences.putInt("soil", limitSoil);
    preferences.putFloat("wave", limitWave);
    preferences.putInt("light", limitLight);
    preferences.putBool("autoLamp", autoLampMode);
    preferences.putBool("autoPump", autoPumpMode); 
    preferences.end();
}

void sendLoRaMessage(String msg) {
  Serial2.println(msg); 
  Serial.print("LoRa Sent: ");
  Serial.println(msg);
}

void refreshLCD() {
    delay(200); lcd.init(); lcd.backlight(); updateLCD(); 
}

void controlPump(bool state) {
  if (pumpState != state) {
    pumpState = state;
    digitalWrite(PIN_RELAY_1, state ? PUMP_ON_LEVEL : PUMP_OFF_LEVEL); 
    if (state) refreshLCD(); 
  }
}

void controlLamp(bool state) {
    if (lampState != state) {
        lampState = state;
        digitalWrite(PIN_RELAY_2, state ? LAMP_ON_LEVEL : LAMP_OFF_LEVEL);
        autoLampMode = false; 
        updateLCD(); 
    }
}

void controlFan(bool state) {
    if (fanState != state) {
        fanState = state;
        digitalWrite(PIN_RELAY_3, state ? FAN_ON_LEVEL : FAN_OFF_LEVEL);
        updateLCD(); 
    }
}

void controlWindow(bool state) {
    if (windowState != state) {
        windowState = state;
        if (state) { windowServo.write(90); Serial.println("Cua so: MO"); } 
        else { windowServo.write(0); Serial.println("Cua so: DONG"); }
        updateLCD(); 
    }
}

void showAuthRequestLCD() { lcd.setCursor(0, 0); lcd.print("CO KET NOI MOI! "); lcd.setCursor(0, 1); lcd.print("1:HUY  2:DONG Y "); }

void showGasAlertLCD(int gasValue) { 
  lcd.setCursor(0, 0); lcd.print("!!! CANH BAO !!!"); 
  lcd.setCursor(0, 1); lcd.print("KHI GAS: "); lcd.print(gasValue); lcd.print("    "); 
}

void showWaveAlertLCD(float accel) { lcd.setCursor(0, 0); lcd.print("!CANH BAO SONG!"); lcd.setCursor(0, 1); lcd.print("Gia toc: "); lcd.print((int)accel); lcd.print("m/s "); }

void updateLCD() {
  if (gasAlert) return; if (waveAlert) return; if (isAuthPending) { showAuthRequestLCD(); return; } if (isSOSCountingDown) { showSOSCountdown(); return; }
  lcd.noCursor(); lcd.noBlink();
  
  if (lcdPage == 0) showPageMain(); 
  else if (lcdPage == 1) showPageSoil(); 
  else if (lcdPage == 2) showPageSignal1(); 
  else if (lcdPage == 3) showPageSignal2();
  else if (lcdPage == 4) showPageGas(); 
  else if (lcdPage == 5) showPageMsg(); 
  else if (lcdPage == 10) showPageMenu();
}

void showPageMain() {
  lcd.setCursor(0, 0); 
  int displayHour = 0; int displayMin = 0; bool timeValid = false;
  if (gps.time.isValid()) { displayHour = gps.time.hour() + 7; if (displayHour >= 24) displayHour -= 24; displayMin = gps.time.minute(); timeValid = true; } 
  else if (hasSyncedTime) { unsigned long now = (millis() / 1000) + timeOffset; unsigned long local = now + 25200; displayHour = (local % 86400L) / 3600; displayMin = (local % 3600) / 60; timeValid = true; }
  
  if (timeValid) { 
      lcd.print("TIME: "); if (displayHour < 10) lcd.print("0"); lcd.print(displayHour); 
      lcd.print(":"); if (displayMin < 10) lcd.print("0"); lcd.print(displayMin); lcd.print("   "); 
  } else { 
      lcd.print("Cho GPS/App...  "); 
  }
  lcd.setCursor(0, 1); 
  lcd.print("T:"); if(temp == -99) lcd.print("??"); else lcd.print((int)temp); 
  lcd.print("C H:"); if(humi == -99) lcd.print("??"); else lcd.print((int)humi); lcd.print("%    ");
}

void showPageGas() {
    lcd.setCursor(0, 0); lcd.print("Nong do GAS:");
    lcd.setCursor(0, 1); lcd.print(gasValueDisplay); lcd.print(" ppm ");
    if (gasValueDisplay < 600) lcd.print("(Tot)   ");
    else if (gasValueDisplay < 1500) lcd.print("(CanhBao)");
    else lcd.print("(NGUY HIEM)");
}

void showPageSoil() { lcd.setCursor(0, 0); lcd.print("Dat: "); lcd.print((soil1 + soil2)/2); lcd.print("%   "); lcd.setCursor(0, 1); lcd.print("Bom: "); if (pumpState) lcd.print("ON "); else lcd.print("OFF"); lcd.print(autoPumpMode ? "(Auto)" : "(Tay)"); }

// --- TRANG GPS 1: Sat + HDOP ---
void showPageSignal1() { 
    lcd.setCursor(0, 0); lcd.print("So Ve tinh: "); lcd.print(gps.satellites.value()); 
    lcd.setCursor(0, 1); lcd.print("Chi so do luong: "); lcd.print(gps.hdop.value());
}
void showPageSignal2() { 
    lcd.setCursor(0, 0); 
    lcd.print("VD: "); 
    if(gps.location.isValid()) lcd.print(gps.location.lat(), 4); else lcd.print("Dang Tim..");
    lcd.setCursor(0, 1); 
    lcd.print("KD: "); 
    if(gps.location.isValid()) lcd.print(gps.location.lng(), 4); else lcd.print("Dang Tim..");
}

void showPageMsg() { lcd.setCursor(0, 0); lcd.print("Tin nhan LoRa:"); lcd.setCursor(0, 1); if(lastLoRaMessage.length() > 16) lcd.print(lastLoRaMessage.substring(0, 16)); else lcd.print(lastLoRaMessage); }
void initLCD() { Wire.beginTransmission(0x27); Wire.write(0x00); Wire.endTransmission(); delay(50); lcd.init(); lcd.backlight(); lcd.noCursor(); lcd.noBlink(); lcd.clear(); lcd.setCursor(0, 0); lcd.print("NHA BE SMART"); lcd.setCursor(0, 1); lcd.print("Dang khoi dong..."); delay(1000); }

int readSoil(int pin) { 
  int val = analogRead(pin); 
  val = constrain(val, WET_VAL, DRY_VAL); 
  return map(val, DRY_VAL, WET_VAL, 0, 100); 
}

int readLDR() { return analogRead(PIN_LDR); }

void sendSOS() { 
  String lat = "0.000000"; String lon = "0.000000"; 
  if (gps.location.isValid()) { lat = String(gps.location.lat(), 6); lon = String(gps.location.lng(), 6); } 
  lcd.clear(); lcd.print("!!! SOS !!!"); 
  String msg = "SOS|NHA_BE_01|" + lat + "|" + lon; 
  sendLoRaMessage(msg); 
  lcd.setCursor(0, 1); lcd.print("DA GUI TIN!"); 
  delay(3000); lcd.clear(); lcdPage = 0; updateLCD(); 
}

void startSOSCountdown() { isSOSCountingDown = true; sosCountdownVal = 5; lcd.clear(); showSOSCountdown(); }
void showSOSCountdown() { lcd.setCursor(0, 0); lcd.print("!!! SOS !!!     "); lcd.setCursor(0, 1); lcd.print("Gui sau: "); lcd.print(sosCountdownVal); lcd.print("s   "); }

void showPageMenu() { 
    lcd.clear(); lcd.setCursor(0, 0); lcd.print(">"); 
    if (menuIndex == 0) { lcd.print("AUTO BOM: "); lcd.print(autoPumpMode ? "ON" : "OFF"); }
    else if (menuIndex == 1) { lcd.print("AUTO DEN: "); lcd.print(autoLampMode ? "ON" : "OFF"); }
    else if (menuIndex == 2) { lcd.print("BOM : "); lcd.print(pumpState ? "ON" : "OFF"); }
    else if (menuIndex == 3) { lcd.print("DEN : "); lcd.print(lampState ? "ON" : "OFF"); }
    else if (menuIndex == 4) { lcd.print("QUAT: "); lcd.print(fanState ? "ON" : "OFF"); }
    else if (menuIndex == 5) { lcd.print("CUA SO: "); lcd.print(windowState ? "MO" : "DONG"); }
    lcd.setCursor(0, 1); lcd.print("Bam 1:Next 2:Set");
}

void processLongPress() { if (isSOSCountingDown || isAuthPending) return; if (lcdPage == 10) { lcdPage = 0; lcd.clear(); updateLCD(); lcd.noBacklight(); delay(100); lcd.backlight(); } else { lcdPage = 10; menuIndex = 0; lcd.clear(); updateLCD(); } }
void handleButtonV9() { int reading = digitalRead(PIN_BUTTON); if (reading != lastButtonState) lastDebounceTime = millis(); if ((millis() - lastDebounceTime) > debounceDelay) { if (reading != currentButtonState) { currentButtonState = reading; if (currentButtonState == LOW) { if (!isPressing) { isPressing = true; buttonPressTime = millis(); } } else { if (isPressing) { unsigned long pressDuration = millis() - buttonPressTime; if (pressDuration > 1000) { processLongPress(); clickCount = 0; } else { clickCount++; lastClickReleaseTime = millis(); } isPressing = false; } } } } if (clickCount > 0 && (millis() - lastClickReleaseTime > MULTI_CLICK_TIMEOUT) && !isPressing) { processClicks(clickCount); clickCount = 0; } lastButtonState = reading; }

void processClicks(int count) { 
    if (isAuthPending) { if (count == 1) { lcd.clear(); lcd.setCursor(0,0); lcd.print("DA HUY KET NOI!"); pServer->disconnect(currentConnId); isAuthPending = false; authAccepted = false; delay(1000); lcdPage = 0; updateLCD(); } else if (count == 2) { lcd.clear(); lcd.setCursor(0,0); lcd.print("KET NOI OK!"); authAccepted = true; isAuthPending = false; delay(1000); lcdPage = 0; updateLCD(); } return; } 
    if (isSOSCountingDown) { isSOSCountingDown = false; lcd.clear(); lcd.print("DA HUY SOS!"); delay(1000); lcdPage = 0; lcd.clear(); updateLCD(); return; } 
    
    if (count == 1) { 
        if (lcdPage == 10) { 
            menuIndex++; 
            if (menuIndex > 5) menuIndex = 0; 
            updateLCD(); 
        } else { 
            lcdPage++; 
            if (lcdPage == 3 && gasAlert) lcdPage = 5; 
            if (lcdPage > 5) lcdPage = 0; 
            lcd.clear(); updateLCD(); 
        } 
    } else if (count == 2) { 
        if (lcdPage == 10) { 
            if (menuIndex == 0) autoPumpMode = !autoPumpMode; 
            else if (menuIndex == 1) autoLampMode = !autoLampMode; 
            else if (menuIndex == 2) controlPump(!pumpState); 
            else if (menuIndex == 3) controlLamp(!lampState); 
            else if (menuIndex == 4) controlFan(!fanState); 
            else if (menuIndex == 5) controlWindow(!windowState); 
            updateLCD(); 
        } 
    } else if (count >= 5) startSOSCountdown(); 
}
