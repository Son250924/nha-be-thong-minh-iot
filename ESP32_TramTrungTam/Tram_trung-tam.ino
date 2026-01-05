#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"

// ================= 1. CẤU HÌNH PHẦN CỨNG =================
#define M0  25
#define M1  26
#define AUX 27
#define RX_LORA 16
#define TX_LORA 17
#define BUZZER_PIN 4

// --- PHÂN TÁCH NÚT BẤM ---
#define BTN_SOS     15  // Nút gắn ngoài: Xác nhận SOS
#define BTN_RESET   0   // Nút BOOT trên mạch: Reset WiFi

// --- CẤU HÌNH LCD (SDA=32, SCL=33) ---
#define I2C_SDA 32
#define I2C_SCL 33

String openWeatherKey = "afe165931f131e85312bf93b1ac55150";
String city = "Ho Chi Minh"; // Mặc định
String weatherTemp = "--";
String weatherHum = "--";
String weatherWind = "--";
String weatherDesc = "Dang cap nhat...";
unsigned long lastWeatherUpdate = 0;
const unsigned long WEATHER_INTERVAL = 900000; // 15 phút

LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);
Preferences preferences;

String lastLoRaMsg = "Cho tin hieu...";
String sosLat = "0.000000";
String sosLon = "0.000000";
String sosName = "---";
bool isAlerting = false;     
bool buzzerState = false;    
unsigned long lastBuzzerToggle = 0;

unsigned long lastWifiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 5000; 

const char* ap_ssid = "TRAM_TRUNG_TAM";
const char* ap_pass = "12345678";
String st_ssid = "";
String st_pass = "";

const char* ntpServer = "time.google.com";
const long  gmtOffset_sec = 25200;
const int   daylightOffset_sec = 0;

// Biến quản lý hiển thị
int currentScreen = 0; 
unsigned long lastScreenUpdate = 0;

// Biến xử lý nút bấm
bool lastBtnSosState = HIGH;
bool lastBtnResetState = HIGH;
unsigned long lastDebounceTime = 0;

// Biến WiFi Scan
String wifiScanResult = "[]"; 

// --- HTML GIAO DIỆN CLASSIC + CHAT FIX ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Trung Tâm Giám Sát Nhà Bè</title>
    <style>
        /* GIỮ THEME CŨ */
        :root { --primary: #003366; --bg: #f0f2f5; --card: #ffffff; --text: #333; --danger: #d32f2f; --success: #2e7d32; --warn: #ed6c02; --info: #0288d1; --chat-bg-L: #e4e6eb; --chat-bg-R: #0084ff; }
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; background-color: var(--bg); color: var(--text); margin: 0; padding: 0; }
        
        .navbar { background-color: var(--primary); color: white; padding: 1rem; text-align: center; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .navbar h1 { margin: 0; font-size: 1.2rem; text-transform: uppercase; letter-spacing: 1px; }
        .navbar p { margin: 5px 0 0; font-size: 0.9rem; opacity: 0.8; }

        .container { max-width: 1200px; margin: 20px auto; padding: 0 15px; display: flex; flex-wrap: wrap; gap: 20px; justify-content: center; }
        .card { background: var(--card); border-radius: 8px; box-shadow: 0 1px 3px rgba(0,0,0,0.1); padding: 20px; flex: 1 1 300px; min-width: 280px; display: flex; flex-direction: column; }
        .card-header { font-weight: 600; font-size: 1.1rem; margin-bottom: 15px; border-bottom: 1px solid #eee; padding-bottom: 10px; color: var(--primary); }
        
        .status-box { padding: 15px; border-radius: 6px; text-align: center; font-weight: bold; margin-bottom: 15px; }
        .status-normal { background-color: #e8f5e9; color: var(--success); border: 1px solid #c8e6c9; }
        .status-alert { background-color: #ffebee; color: var(--danger); border: 1px solid #ffcdd2; animation: blink 1s infinite; }
        
        .weather-temp { font-size: 2.5rem; font-weight: bold; color: var(--info); text-align: center;}
        .weather-desc { font-size: 1.1rem; text-align: center; color: #555; text-transform: capitalize;}
        .info-row { display: flex; justify-content: space-between; margin-bottom: 10px; font-size: 0.95rem; }
        .info-label { color: #666; } .info-val { font-weight: 500; }

        /* --- CHAT BOX CSS (FIXED COLOR) --- */
        .chat-box { height: 220px; overflow-y: auto; border: 1px solid #ddd; border-radius: 8px; padding: 10px; margin-bottom: 10px; background: #f9f9f9; display: flex; flex-direction: column; gap: 8px; }
        .msg-bubble { max-width: 80%; padding: 8px 12px; border-radius: 15px; font-size: 0.9rem; line-height: 1.4; word-wrap: break-word; position: relative; }
        /* Tin nhan Trai (Nhan): Nen xam, Chu den */
        .msg-left { background-color: var(--chat-bg-L); color: #000; align-self: flex-start; border-bottom-left-radius: 2px; }
        /* Tin nhan Phai (Gui): Nen xanh, Chu trang */
        .msg-right { background-color: var(--chat-bg-R); color: #fff; align-self: flex-end; border-bottom-right-radius: 2px; }
        .msg-time { font-size: 0.7rem; display: block; text-align: right; opacity: 0.7; margin-top: 3px; }
        
        .chat-controls { display: flex; gap: 5px; }
        
        input[type="text"], input[type="password"], select { width: 100%; padding: 10px; margin: 5px 0; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
        
        .btn { width: 100%; padding: 12px; border: none; border-radius: 4px; cursor: pointer; font-size: 1rem; font-weight: 500; color: white; margin-top: 5px; }
        .btn-action { background-color: var(--primary); }
        .btn-confirm { background-color: var(--success); font-weight: bold; box-shadow: 0 2px 5px rgba(0,0,0,0.2); }
        .btn-danger { background-color: var(--danger); }
        .btn-map { background-color: var(--warn); text-decoration: none; display: block; text-align: center; padding-top: 12px; color:white;}
        
        /* Nut Scan tron */
        .btn-scan { width: 40px; height: 40px; padding: 0; border-radius: 50%; background-color: #666; font-size: 1.2rem; display: flex; align-items: center; justify-content: center; }
        
        /* Settings Toggle */
        .settings-area { display: none; margin-top: 10px; border-top: 1px dashed #ccc; padding-top: 10px; }
        .toggle-btn { background: none; border: none; color: #555; cursor: pointer; font-size: 0.9rem; text-decoration: underline; padding: 5px; width: 100%; text-align: left; }

        .overlay { position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.5); display: none; align-items: center; justify-content: center; z-index: 1000; }
        .popup { background: white; padding: 20px; border-radius: 8px; text-align: center; box-shadow: 0 4px 10px rgba(0,0,0,0.3); width: 80%; max-width: 400px; }
        .loader { border: 4px solid #f3f3f3; border-top: 4px solid var(--primary); border-radius: 50%; width: 30px; height: 30px; animation: spin 1s linear infinite; margin: 10px auto; }
        @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
        @keyframes blink { 50% { opacity: 0.6; } }
        
        .footer { text-align: center; padding: 20px; color: #888; font-size: 0.8rem; width: 100%; }
    </style>
    <script>
        var lastMsgContent = "";
        
        window.onload = function() {
            scanWifi();
            addMessage("Hệ thống", "Đã kết nối máy chủ.", "left");
        };

        setInterval(function() {
            fetch("/status").then(response => response.json()).then(data => {
                document.getElementById("sys_time").innerText = data.time;
                const statusBox = document.getElementById("status_box");
                const statusText = document.getElementById("status_text");
                const mapBtn = document.getElementById("btn_map");
                const confirmBtn = document.getElementById("btn_confirm_sos");
                
                if (data.alert) {
                    statusBox.className = "status-box status-alert";
                    statusText.innerText = "CẢNH BÁO: CÓ TÍN HIỆU SOS!";
                    mapBtn.style.display = "block";
                    mapBtn.href = "https://www.google.com/maps/search/?api=1&query=" + data.lat + "," + data.lon;
                    confirmBtn.style.display = "block"; // Hien nut xac nhan
                } else {
                    statusBox.className = "status-box status-normal";
                    statusText.innerText = "HỆ THỐNG HOẠT ĐỘNG BÌNH THƯỜNG";
                    mapBtn.style.display = "none";
                    confirmBtn.style.display = "none"; // An nut xac nhan
                }
                document.getElementById("sos_name").innerText = data.name;
                document.getElementById("sos_lat").innerText = data.lat;
                document.getElementById("sos_lon").innerText = data.lon;
                document.getElementById("last_msg").innerText = data.msg;

                document.getElementById("w_temp").innerText = data.w_temp + "°C";
                document.getElementById("w_desc").innerText = data.w_desc;
                document.getElementById("w_hum").innerText = data.w_hum + "%";
                document.getElementById("w_wind").innerText = data.w_wind + " m/s";
                document.getElementById("city_display").innerText = data.city;

                // --- CHAT LOGIC ---
                if (data.msg !== lastMsgContent && data.msg !== "Cho tin hieu..." && data.msg !== "Chua co tin") {
                    if (data.msg.startsWith("MSG_APP|")) {
                        var content = data.msg.substring(8);
                        addMessage("Nhà Bè", content, "left");
                    } else if (data.msg.startsWith("SOS|")) {
                        addMessage("CẢNH BÁO", "Tín hiệu SOS!", "left");
                    }
                    lastMsgContent = data.msg;
                }
            });
        }, 2000);

        function addMessage(sender, text, side) {
            var chatBox = document.getElementById("chat_box");
            var date = new Date();
            var timeStr = date.getHours() + ":" + (date.getMinutes()<10?'0':'') + date.getMinutes();
            var bubble = document.createElement("div");
            bubble.className = "msg-bubble " + (side === "left" ? "msg-left" : "msg-right");
            bubble.innerHTML = "<strong>" + sender + ": </strong>" + text + "<span class='msg-time'>" + timeStr + "</span>";
            chatBox.appendChild(bubble);
            chatBox.scrollTop = chatBox.scrollHeight;
        }

        function sendChat() {
            var input = document.getElementById("chat_input");
            var txt = input.value.trim();
            if (!txt) return;
            addMessage("Trung Tâm", txt, "right");
            var msgToSend = "MSG_CENTER|" + txt;
            fetch("/send?msg=" + encodeURIComponent(msgToSend));
            input.value = ""; 
        }
        
        function checkEnter(event) { if (event.key === "Enter") sendChat(); }

        function sendCommand(cmd, isCustom) {
            fetch("/send?msg=" + encodeURIComponent(cmd))
                .then(res => res.text())
                .then(txt => { if(!isCustom) alert("Đã gửi lệnh xác nhận!"); });
        }

        function scanWifi() {
            var select = document.getElementById("ssid_select");
            select.innerHTML = "<option>Đang quét...</option>";
            fetch("/scan").then(r => r.json()).then(data => {
                select.innerHTML = "<option value=''>-- Chọn mạng WiFi --</option>";
                data.forEach(ssid => {
                    var op = document.createElement("option"); op.text = ssid; op.value = ssid; select.add(op);
                });
                var op = document.createElement("option"); op.text = "Nhập tay..."; op.value = "MANUAL"; select.add(op);
            });
        }

        function onWifiSelect() {
            var val = document.getElementById("ssid_select").value;
            var inp = document.getElementById("ssid_input");
            if (val === "MANUAL") { inp.style.display = "block"; inp.value = ""; inp.focus(); } 
            else { inp.style.display = "none"; inp.value = val; }
        }

        function saveConfig() {
            var ssid = document.getElementById("ssid_input").value;
            var pass = document.getElementById("pass_input").value;
            var city = document.getElementById("city_input").value;
            if(!ssid) return alert("Vui lòng nhập tên WiFi!");
            document.getElementById("overlay").style.display = "flex";
            var url = "/save_settings?ssid=" + encodeURIComponent(ssid) + "&pass=" + encodeURIComponent(pass) + "&city=" + encodeURIComponent(city);
            fetch(url).then(r => {
                setTimeout(() => { 
                    alert("Đã lưu! Mạch đang khởi động lại..."); 
                    document.getElementById("overlay").style.display = "none"; 
                }, 3000);
            });
        }

        function resetWifi() {
            if(confirm("Bạn chắc chắn muốn XÓA WiFi và Reset mạch?")) { fetch("/reset_wifi"); }
        }
        
        function toggleSettings() {
            var el = document.getElementById("settings_area");
            if (el.style.display === "block") el.style.display = "none";
            else el.style.display = "block";
        }
    </script>
</head>
<body>
    <div id="overlay" class="overlay"><div class="popup"><div class="loader"></div><h3 id="popup_msg">Đang xử lý...</h3></div></div>

    <div class="navbar">
        <h1>TRUNG TÂM GIÁM SÁT NHÀ BÈ</h1>
        <p id="sys_time">Đang đồng bộ...</p>
    </div>

    <div class="container">
        <!-- CARD 1: THỜI TIẾT -->
        <div class="card">
            <div class="card-header">Thời Tiết: <span id="city_display">...</span></div>
            <div class="weather-main">
                <div class="weather-temp" id="w_temp">--°C</div>
                <div class="weather-desc" id="w_desc">Đang tải dữ liệu...</div>
            </div>
            <div class="info-row" style="margin-top:10px; border-top:1px solid #eee; padding-top:10px;">
                <span>Độ ẩm: <b id="w_hum">--%</b></span>
                <span>Gió: <b id="w_wind">-- m/s</b></span>
            </div>
            <div style="font-size:0.8rem; color:#888; text-align:center; margin-top:5px;">Nguồn: OpenWeatherMap</div>
        </div>

        <!-- CARD 2: KÊNH LIÊN LẠC (GIỮA) -->
        <div class="card" style="flex: 2 1 400px;">
            <div class="card-header">Kênh Liên Lạc Nội Bộ</div>
            <div id="chat_box" class="chat-box">
                <!-- Tin nhắn hiển thị ở đây -->
            </div>
            <div class="chat-controls">
                <input type="text" id="chat_input" placeholder="Nhập tin nhắn..." onkeypress="checkEnter(event)">
                <button class="btn btn-action" style="width:auto; margin-top:5px;" onclick="sendChat()">Gửi</button>
            </div>
        </div>

        <!-- CARD 3: TRẠNG THÁI SOS & XÁC NHẬN (GỘP) -->
        <div class="card">
            <div class="card-header">Trạng Thái SOS</div>
            <div id="status_box" class="status-box status-normal">
                <span id="status_text">Đang kiểm tra kết nối...</span>
            </div>
            <div class="info-row"><span class="info-label">Trạm gửi:</span><span class="info-val" id="sos_name">---</span></div>
            <div class="info-row"><span class="info-label">Vĩ độ:</span><span class="info-val" id="sos_lat">0.000000</span></div>
            <div class="info-row"><span class="info-label">Kinh độ:</span><span class="info-val" id="sos_lon">0.000000</span></div>
            
            <a id="btn_map" href="#" target="_blank" class="btn btn-map" style="display:none;">MỞ BẢN ĐỒ</a>
            
            <!-- NÚT XÁC NHẬN CỨU HỘ ĐƯA VÀO ĐÂY -->
            <button id="btn_confirm_sos" class="btn btn-confirm" style="display:none; margin-top:10px;" onclick="sendCommand('CONFIRM_SOS|CUU HO DANG TOI', false)">XÁC NHẬN CỨU HỘ</button>
            
            <div style="margin-top: 15px; padding-top: 10px; border-top: 1px dashed #ddd;">
                <small class="info-label">Tin LoRa cuối:</small><br><em id="last_msg" style="color:#000;">...</em>
            </div>
        </div>

        <!-- CARD 4: CÀI ĐẶT HỆ THỐNG (COMBO BOX) -->
        <div class="card">
            <div class="card-header">Hệ Thống</div>
            <button class="toggle-btn" onclick="toggleSettings()">▼ Mở Cài Đặt WiFi & Vị Trí</button>
            
            <div id="settings_area" class="settings-area">
                <div class="info-label" style="margin-bottom:5px;">Cấu hình WiFi:</div>
                <div style="display:flex; gap:5px;">
                    <select id="ssid_select" onchange="onWifiSelect()" style="flex:1;"><option>Đang tải...</option></select>
                    <button class="btn btn-scan" onclick="scanWifi()">↻</button>
                </div>
                <input type="text" id="ssid_input" placeholder="Tên WiFi..." style="display:none;">
                <input type="password" id="pass_input" placeholder="Mật khẩu...">
                <input type="text" id="city_input" placeholder="Thành phố (Thời tiết)...">
                
                <div style="display:flex; gap:10px; margin-top:10px;">
                    <button class="btn btn-action" onclick="saveConfig()">Lưu & Kết Nối</button>
                    <button class="btn btn-danger" onclick="resetWifi()">Reset WiFi</button>
                </div>
            </div>
        </div>
    </div>
    <div class="footer">Dự án Nhà Bè Smart - Firmware v1</div>
</body>
</html>
)rawliteral";

// ================= CÁC HÀM HỖ TRỢ =================

void refreshLCD() {
    lcd.init();
    lcd.backlight();
}

void scanWifiNetworks() {
    Serial.println("Scanning WiFi...");
    // Scan blocking - sẽ dừng hệ thống khoảng 2-3s
    int n = WiFi.scanNetworks();
    if (n == 0) {
        wifiScanResult = "[]";
    } else {
        wifiScanResult = "[";
        for (int i = 0; i < n; ++i) {
            if (i > 0) wifiScanResult += ",";
            wifiScanResult += "\"" + WiFi.SSID(i) + "\"";
        }
        wifiScanResult += "]";
    }
}

void getWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // SỬA LỖI URL ENCODE CHO THÀNH PHỐ
    String encodedCity = city;
    encodedCity.replace(" ", "%20");
    
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + encodedCity + "&appid=" + openWeatherKey + "&units=metric&lang=vi";
    Serial.println("Weather URL: " + url);
    
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      if (doc.containsKey("main")) {
          float temp = doc["main"]["temp"];
          int hum = doc["main"]["humidity"];
          float wind = doc["wind"]["speed"];
          const char* desc = doc["weather"][0]["description"];
          weatherTemp = String(temp, 1);
          weatherHum = String(hum);
          weatherWind = String(wind, 1);
          weatherDesc = String(desc);
      } else {
          weatherDesc = "Sai tên TP";
      }
    }
    http.end();
  }
}

// Xóa WiFi và Reset
void resetSystem() {
    preferences.begin("wifi_conf", false);
    preferences.remove("ssid");
    preferences.remove("pass");
    preferences.end();
    
    refreshLCD();
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("RESET WIFI!");
    lcd.setCursor(0,1); lcd.print("Restarting...");
    delay(2000);
    ESP.restart();
}

void handleRoot() { server.send(200, "text/html", index_html); }

void handleScan() {
    scanWifiNetworks();
    server.send(200, "application/json", wifiScanResult);
}

void handleStatus() {
  struct tm timeinfo;
  char timeString[30] = "--:--:--";
  if(getLocalTime(&timeinfo)){ strftime(timeString, 30, "%H:%M:%S", &timeinfo); }
  
  String json = "{";
  json += "\"time\":\"" + String(timeString) + "\",";
  json += "\"msg\":\"" + lastLoRaMsg + "\",";
  json += "\"lat\":\"" + sosLat + "\",";
  json += "\"lon\":\"" + sosLon + "\",";
  json += "\"name\":\"" + sosName + "\",";
  json += "\"alert\":" + String(isAlerting ? "true" : "false") + ",";
  json += "\"w_temp\":\"" + weatherTemp + "\",";
  json += "\"w_hum\":\"" + weatherHum + "\",";
  json += "\"w_wind\":\"" + weatherWind + "\",";
  json += "\"w_desc\":\"" + weatherDesc + "\",";
  json += "\"city\":\"" + city + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleSend() {
  if (server.hasArg("msg")) {
    String msgToSend = server.arg("msg");
    Serial2.println(msgToSend);
    if (msgToSend.indexOf("CONFIRM") >= 0) {
      isAlerting = false;
      digitalWrite(BUZZER_PIN, LOW);
      refreshLCD(); 
      lcd.clear(); lcd.setCursor(0,0); lcd.print("DA XAC NHAN!");
      currentScreen = 0; 
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleSaveSettings() {
  String s = server.arg("ssid");
  String p = server.arg("pass");
  String c = server.arg("city");
  
  preferences.begin("wifi_conf", false);
  if(s.length() > 0) {
      preferences.putString("ssid", s);
      preferences.putString("pass", p);
  }
  if(c.length() > 0) {
      preferences.putString("city", c);
  }
  preferences.end();
  
  server.send(200, "text/plain", "OK");
  delay(2000); 
  ESP.restart();
}

void handleResetWifi() {
    server.send(200, "text/plain", "OK");
    delay(1000);
    resetSystem();
}

void connectWifi() {
  // Safe delay
  delay(1000);
  
  preferences.begin("wifi_conf", true);
  st_ssid = preferences.getString("ssid", "");
  st_pass = preferences.getString("pass", "");
  city = preferences.getString("city", "Ho Chi Minh");
  preferences.end();

  refreshLCD();
  if (st_ssid.length() > 0) {
    lcd.setCursor(0,0); lcd.print("WIFI...");
    lcd.setCursor(0,1); lcd.print(st_ssid.substring(0, 16));
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(st_ssid.c_str(), st_pass.c_str());
    
    int retry = 0;
    while(WiFi.status() != WL_CONNECTED && retry < 15) {
      delay(500); retry++;
    }
    
    if(WiFi.status() == WL_CONNECTED) {
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      refreshLCD();
      lcd.clear(); lcd.setCursor(0,0); lcd.print("IP: "); lcd.print(WiFi.localIP());
      delay(1000);
      getWeather();
    } else {
      WiFi.mode(WIFI_AP_STA);
      WiFi.softAP(ap_ssid, ap_pass);
      refreshLCD();
      lcd.clear(); lcd.setCursor(0,0); lcd.print("LOI WIFI -> AP");
      delay(1000);
    }
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_pass);
    refreshLCD();
    lcd.clear(); lcd.setCursor(0,0); lcd.print("CHUA CAI WIFI");
  }
  
  // KHONG QUET WIFI O DAY NUA
}

void handleWifiAutoSwitch() {
    if (st_ssid.length() == 0) return;
    unsigned long currentMillis = millis();
    if (currentMillis - lastWifiCheck >= WIFI_CHECK_INTERVAL) {
        lastWifiCheck = currentMillis;
        if (WiFi.status() == WL_CONNECTED) {
            if (WiFi.getMode() != WIFI_STA) WiFi.mode(WIFI_STA);
        } else {
            if (WiFi.getMode() != WIFI_AP_STA) {
                WiFi.mode(WIFI_AP_STA); WiFi.softAP(ap_ssid, ap_pass); WiFi.begin(st_ssid.c_str(), st_pass.c_str());
            }
        }
    }
}

void parseSOS(String msg) {
  int p1 = msg.indexOf('|');
  int p2 = msg.indexOf('|', p1 + 1);
  int p3 = msg.indexOf('|', p2 + 1);
  if (p1 > 0 && p2 > 0 && p3 > 0) {
    sosName = msg.substring(p1 + 1, p2);
    sosLat = msg.substring(p2 + 1, p3);
    sosLon = msg.substring(p3 + 1);
    isAlerting = true; 
    
    refreshLCD(); 
    lcd.clear(); lcd.setCursor(0,0); lcd.print("SOS: "); lcd.print(sosName);
    lcd.setCursor(0,1); lcd.print(sosLat);
    currentScreen = 1;
  }
}

// === XỬ LÝ 2 NÚT BẤM RIÊNG BIỆT ===
void handleButtons() {
    // 1. Xử lý nút RESET WIFI (GPIO 0) - BOOT
    if (digitalRead(BTN_RESET) == LOW) {
        delay(50); // Debounce
        if (digitalRead(BTN_RESET) == LOW) {
            digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW);
            resetSystem(); // Bấm là reset ngay
        }
    }

    // 2. Xử lý nút SOS CONFIRM (GPIO 15) - Nút Rời
    int sosState = digitalRead(BTN_SOS);
    if (sosState == LOW && lastBtnSosState == HIGH) {
        // Vừa nhấn nút
        if (isAlerting) {
            isAlerting = false;
            Serial2.println("CONFIRM_SOS|CUU HO DANG TOI"); 
            refreshLCD();
            lcd.clear(); lcd.setCursor(0,0); lcd.print("DA XAC NHAN!");
            delay(1000); 
            currentScreen = 0;
        }
    }
    lastBtnSosState = sosState;
}

void setup() {
  Serial.begin(115200);
  
  pinMode(M0, OUTPUT); pinMode(M1, OUTPUT); pinMode(AUX, INPUT);
  digitalWrite(M0, LOW); digitalWrite(M1, LOW);
  Serial2.begin(9600, SERIAL_8N1, RX_LORA, TX_LORA);

  pinMode(BUZZER_PIN, OUTPUT); digitalWrite(BUZZER_PIN, LOW);
  
  // CẤU HÌNH 2 NÚT
  pinMode(BTN_SOS, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);

  Wire.begin(I2C_SDA, I2C_SCL); 
  lcd.init(); lcd.backlight();
  lcd.setCursor(0,0); lcd.print("Dang khoi dong...");
  
  digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW);

  connectWifi();
  
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/scan", handleScan); 
  server.on("/send", handleSend);
  server.on("/save_settings", handleSaveSettings);
  server.on("/reset_wifi", handleResetWifi); 
  server.begin();
}

void loop() {
  server.handleClient();
  handleWifiAutoSwitch();
  handleButtons(); // Gọi hàm xử lý nút mới

  if (millis() - lastWeatherUpdate > WEATHER_INTERVAL) {
      lastWeatherUpdate = millis();
      getWeather();
  }
  
  if (isAlerting) {
    if (millis() - lastBuzzerToggle > 200) { 
       lastBuzzerToggle = millis();
       buzzerState = !buzzerState;
       digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  if (Serial2.available()) {
    String msg = Serial2.readStringUntil('\n');
    msg.trim();
    if (msg.length() > 0) {
      Serial.println("RX: " + msg);
      
      if (msg.startsWith("SOS|")) {
          parseSOS(msg);
          lastLoRaMsg = msg;
      }
      else if (msg.startsWith("MSG_APP|")) {
          String content = msg.substring(8); 
          
          refreshLCD(); 
          lcd.clear();
          lcd.setCursor(0,0); lcd.print("Tin tu App:");
          lcd.setCursor(0,1); 
          if(content.length() > 16) lcd.print(content.substring(0,16)); 
          else lcd.print(content);
          
          lastLoRaMsg = msg;
          currentScreen = 2; 
          lastScreenUpdate = millis(); 
      }
      else if (!isAlerting) {
            refreshLCD();
            lcd.clear(); lcd.setCursor(0,0); lcd.print("Tin moi:");
            lcd.setCursor(0,1); 
            if(msg.length() > 16) lcd.print(msg.substring(0,16)); else lcd.print(msg);
            lastLoRaMsg = msg;
            currentScreen = 2;
      }
    }
  }
  
  static unsigned long lastTimeLCD = 0;
  if (!isAlerting && currentScreen != 2 && millis() - lastTimeLCD > 1000) {
     lastTimeLCD = millis();
     struct tm timeinfo;
     if(getLocalTime(&timeinfo)){
        lcd.setCursor(0,0); 
        lcd.printf("%02d:%02d:%02d        ", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        if (WiFi.status() == WL_CONNECTED) {
             lcd.setCursor(0,1); lcd.print(WiFi.localIP()); 
        }
     }
  }
  
  if (currentScreen == 2 && millis() - lastScreenUpdate > 10000) {
      currentScreen = 0;
      refreshLCD();
      lcd.clear();
  }
}
