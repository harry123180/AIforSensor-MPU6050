#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

// 配網參數
const char* AP_SSID = "Pico2W-Config";
const char* AP_PASSWORD = "12345678";

// Web伺服器
WebServer server(80);

// WiFi憑證結構
struct WiFiConfig {
    char ssid[32];
    char password[64];
    bool valid;
};

WiFiConfig wifiConfig;

// GPIO
const int LED_PIN = LED_BUILTIN;
const int RESET_PIN = 2;

// 狀態
bool configMode = false;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("Pico 2 W Simple WiFi Manager");
    
    // 初始化GPIO
    pinMode(LED_PIN, OUTPUT);
    pinMode(RESET_PIN, INPUT_PULLUP);
    
    // 初始化EEPROM
    EEPROM.begin(512);
    loadConfig();
    
    // 檢查重置按鈕
    if (digitalRead(RESET_PIN) == LOW) {
        Serial.println("Reset button pressed - clearing config");
        clearConfig();
        delay(1000);
    }
    
    // 嘗試連接已儲存的WiFi
    if (wifiConfig.valid && connectWiFi()) {
        Serial.println("Connected to saved WiFi");
        digitalWrite(LED_PIN, HIGH);
        startNormalMode();
    } else {
        Serial.println("Starting config mode");
        startConfigMode();
    }
}

void loadConfig() {
    EEPROM.get(0, wifiConfig);
    
    // 驗證資料有效性
    if (wifiConfig.valid) {
        // 檢查SSID是否包含有效字符
        bool validSSID = true;
        int ssidLen = strlen(wifiConfig.ssid);
        
        if (ssidLen == 0 || ssidLen > 31) {
            validSSID = false;
        } else {
            for (int i = 0; i < ssidLen; i++) {
                if (wifiConfig.ssid[i] < 32 || wifiConfig.ssid[i] > 126) {
                    validSSID = false;
                    break;
                }
            }
        }
        
        if (validSSID) {
            Serial.print("Found saved WiFi: ");
            Serial.println(wifiConfig.ssid);
        } else {
            Serial.println("Invalid saved data - clearing");
            wifiConfig.valid = false;
        }
    } else {
        Serial.println("No saved WiFi config");
    }
}

void saveConfig() {
    wifiConfig.valid = true;
    EEPROM.put(0, wifiConfig);
    EEPROM.commit();
    Serial.println("WiFi config saved");
}

void clearConfig() {
    memset(&wifiConfig, 0, sizeof(wifiConfig));
    wifiConfig.valid = false;
    EEPROM.put(0, wifiConfig);
    EEPROM.commit();
    Serial.println("WiFi config cleared");
}

bool connectWiFi() {
    if (!wifiConfig.valid) return false;
    
    Serial.print("Connecting to: ");
    Serial.println(wifiConfig.ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiConfig.ssid, wifiConfig.password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // 閃爍LED
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("Connected! IP: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println();
        Serial.println("Connection failed");
        return false;
    }
}

void startConfigMode() {
    configMode = true;
    
    // 停止現有連接
    WiFi.disconnect();
    delay(100);
    
    // 停止現有伺服器
    server.stop();
    delay(100);
    
    // 啟動AP模式
    WiFi.mode(WIFI_AP);
    
    bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    if (apStarted) {
        // 等待AP完全啟動
        delay(1000);
        
        IPAddress apIP = WiFi.softAPIP();
        
        Serial.println("AP started successfully");
        Serial.printf("SSID: %s\n", AP_SSID);
        Serial.printf("Password: %s\n", AP_PASSWORD);
        Serial.printf("IP: %s\n", apIP.toString().c_str());
        
        // 重新設置伺服器路由
        setupWebServer();
        server.begin();
        Serial.printf("Web server started - browse to %s\n", apIP.toString().c_str());
        
        // LED快速閃爍
        for (int i = 0; i < 10; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(100);
            digitalWrite(LED_PIN, LOW);
            delay(100);
        }
    } else {
        Serial.println("AP failed to start");
    }
}

void startNormalMode() {
    configMode = false;
    
    // 停止 AP 模式，切換到 STA 模式
    WiFi.mode(WIFI_STA);
    delay(500);
    
    // 重新連接 WiFi 確保正確的 IP
    if (!connectWiFi()) {
        Serial.println("Failed to connect in normal mode");
        startConfigMode();
        return;
    }
    
    // 設置正常模式的Web服務器
    server.stop(); // 先停止配網服務器
    delay(100);
    
    server.on("/", handleStatus);
    server.on("/reset", handleReset);
    server.begin();
    
    Serial.println("Normal mode - device ready");
    Serial.printf("Access at: http://%s\n", WiFi.localIP().toString().c_str());
}

void setupWebServer() {
    server.on("/", handleRoot);
    server.on("/test", []() {
        server.send(200, "text/plain", "Pico 2 W is working!");
    });
    server.on("/scan", handleScan);
    server.on("/connect", HTTP_POST, handleConnect);
    server.onNotFound([]() {
        server.send(404, "text/plain", "Not Found");
    });
}

void handleRoot() {
    String html = "<html><head><title>WiFi Setup</title>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'></head>";
    html += "<body style='font-family:Arial;padding:20px'>";
    html += "<h1>Pico 2 W WiFi Setup</h1>";
    
    // WiFi掃描結果
    if (server.hasArg("scan")) {
        html += "<h3>Networks:</h3>";
        int n = WiFi.scanNetworks();
        for (int i = 0; i < n && i < 10; i++) { // 限制顯示10個網路
            String ssid = WiFi.SSID(i);
            html += "<p><a href='javascript:void(0)' onclick=\"document.getElementById('ssid').value='" + ssid + "'\">";
            html += ssid + " (" + String(WiFi.RSSI(i)) + "dBm)</a></p>";
        }
        html += "<hr>";
    }
    
    // 配網表單
    html += "<form action='/connect' method='post'>";
    html += "<p>SSID: <input type='text' id='ssid' name='ssid' required></p>";
    html += "<p>Password: <input type='password' name='password'></p>";
    html += "<p><input type='submit' value='Connect'></p>";
    html += "</form>";
    html += "<p><a href='/?scan=1'>Scan Networks</a></p>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

void handleScan() {
    server.sendHeader("Location", "/?scan=1");
    server.send(302, "text/plain", "");
}

void handleConnect() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    
    if (ssid.length() == 0) {
        server.send(400, "text/html", "<h1>Error</h1><p>SSID required</p><a href='/'>Back</a>");
        return;
    }
    
    // 顯示連接中頁面
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta http-equiv='refresh' content='15;url=/status'>";
    html += "<title>Connecting...</title></head><body>";
    html += "<h1>Connecting to " + ssid + "...</h1>";
    html += "<p>Please wait... Will check status in 15 seconds.</p>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
    
    // 儲存並測試連接
    ssid.toCharArray(wifiConfig.ssid, sizeof(wifiConfig.ssid));
    password.toCharArray(wifiConfig.password, sizeof(wifiConfig.password));
    
    // 測試連接
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiConfig.ssid, wifiConfig.password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        saveConfig();
        Serial.println("WiFi connected successfully");
        delay(2000);
        startNormalMode();
        digitalWrite(LED_PIN, HIGH);
    } else {
        Serial.println("WiFi connection failed");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASSWORD);
    }
}

void handleStatus() {
    String html = "<!DOCTYPE html><html><head><title>Pico 2 W Status</title></head><body>";
    html += "<h1>Pico 2 W Status</h1>";
    
    if (WiFi.status() == WL_CONNECTED) {
        html += "<h2>Connected Successfully!</h2>";
        html += "<p><strong>Network:</strong> " + WiFi.SSID() + "</p>";
        html += "<p><strong>IP:</strong> " + WiFi.localIP().toString() + "</p>";
        html += "<p><strong>Signal:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
        html += "<p><strong>MAC:</strong> " + WiFi.macAddress() + "</p>";
    } else {
        html += "<h2>Connection Failed</h2>";
        html += "<p>Returning to config mode...</p>";
        html += "<meta http-equiv='refresh' content='3;url=/'>";
    }
    
    html += "<br><a href='/reset'>Reset WiFi Settings</a>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

void handleReset() {
    server.send(200, "text/html", 
        "<h1>Resetting...</h1><p>WiFi settings cleared. Device will restart.</p>");
    delay(2000);
    clearConfig();
    rp2040.restart();
}

void loop() {
    server.handleClient();
    
    // 檢查重置按鈕
    if (digitalRead(RESET_PIN) == LOW) {
        delay(50); // 防抖
        if (digitalRead(RESET_PIN) == LOW) {
            unsigned long pressTime = millis();
            while (digitalRead(RESET_PIN) == LOW) {
                delay(10);
            }
            if (millis() - pressTime > 3000) {
                Serial.println("Reset button held - clearing config");
                clearConfig();
                rp2040.restart();
            }
        }
    }
    
    // WiFi狀態檢查
    if (!configMode && WiFi.status() != WL_CONNECTED) {
        static unsigned long lastCheck = 0;
        static int reconnectAttempts = 0;
        
        if (millis() - lastCheck > 10000) { // 10秒檢查一次，不要太頻繁
            Serial.println("WiFi disconnected - attempting reconnect");
            
            if (reconnectAttempts < 3) {
                WiFi.begin(wifiConfig.ssid, wifiConfig.password);
                reconnectAttempts++;
                
                // 等待連接結果
                int attempts = 0;
                while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                    delay(500);
                    attempts++;
                }
                
                if (WiFi.status() == WL_CONNECTED) {
                    Serial.print("Reconnected! IP: ");
                    Serial.println(WiFi.localIP());
                    digitalWrite(LED_PIN, HIGH);
                    reconnectAttempts = 0; // 重置重連計數
                }
            } else {
                Serial.println("Too many reconnection attempts - starting config mode");
                reconnectAttempts = 0;
                startConfigMode();
            }
            
            lastCheck = millis();
        }
    } else if (!configMode && WiFi.status() == WL_CONNECTED) {
        // WiFi 正常連接時確保 LED 亮起
        static bool ledStatus = false;
        if (!ledStatus) {
            digitalWrite(LED_PIN, HIGH);
            ledStatus = true;
        }
    }
    
    delay(10);
}