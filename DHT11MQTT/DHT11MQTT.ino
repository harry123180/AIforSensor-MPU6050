#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// DHT11 庫 - 使用簡單的 bit-bang 實現，不需要額外庫
// DHT11 協議實現

// ==================== 配置參數 ====================
// 配網參數
const char* AP_SSID = "Pico2W-Config";
const char* AP_PASSWORD = "12345678";

// MQTT 預設參數
const char* DEFAULT_MQTT_SERVER = "mqtt.trillionsr.com.tw";
const int DEFAULT_MQTT_PORT = 1883;

// GPIO 定義
const int LED_PIN = LED_BUILTIN;
const int RESET_PIN = 2;
const int DHT11_PIN = 0;  // DHT11 DATA 線連接到 GP0

// 定時器參數
const unsigned long MSG_INTERVAL = 3000;  // 3秒發送一次
const unsigned long STATUS_INTERVAL = 300000;  // 5分鐘發送狀態
const unsigned long MIN_PUBLISH_INTERVAL = 1000;  // 最小發布間隔 1 秒
const unsigned long SENSOR_READ_INTERVAL = 3000;  // 3秒讀取一次感測器

// ==================== 資料結構 ====================
struct Config {
    char wifi_ssid[32];
    char wifi_password[64];
    char mqtt_server[64];
    int mqtt_port;
    char device_id[32];
    bool valid;
};

// DHT11 數據結構
struct DHTData {
    float temperature;
    float humidity;
    bool valid;
    unsigned long lastRead;
};

// ==================== 全域變數 ====================
Config config;
bool configMode = false;
bool mqttConnected = false;

// 網路物件
WebServer server(80);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// 定時器
unsigned long lastMsg = 0;
unsigned long lastStatus = 0;
unsigned long lastPublishAttempt = 0;
unsigned long lastSensorRead = 0;
int msg_count = 0;
int publishFailCount = 0;
const int MAX_PUBLISH_FAILS = 3;

// DHT11 資料
DHTData currentDHT;

// MQTT 主題定義
String topic_temperature = "sensors/factory/environment/temperature";
String topic_humidity = "sensors/factory/environment/humidity";
String topic_status = "sensors/factory/status";
String topic_command;  // 命令主題將在 initConfig 中設定

// ==================== DHT11 功能 ====================
void initDHT11() {
    pinMode(DHT11_PIN, OUTPUT);
    digitalWrite(DHT11_PIN, HIGH);
    
    // 初始化數據結構
    memset(&currentDHT, 0, sizeof(currentDHT));
    currentDHT.lastRead = 0;
    
    Serial.println("DHT11 initialized");
    Serial.printf("DATA Pin: GP%d\n", DHT11_PIN);
    
    delay(2000);  // DHT11 需要時間穩定
}

bool readDHT11() {
    unsigned long now = millis();
    
    // DHT11 需要至少 2 秒間隔
    if (now - currentDHT.lastRead < 2000) {
        return currentDHT.valid;
    }
    
    uint8_t data[5] = {0};
    uint8_t bitCount = 0;
    uint8_t byteIndex = 0;
    
    // 發送開始信號
    pinMode(DHT11_PIN, OUTPUT);
    digitalWrite(DHT11_PIN, LOW);
    delay(18);  // 至少 18ms 低電平
    digitalWrite(DHT11_PIN, HIGH);
    delayMicroseconds(40);  // 20-40μs 高電平
    
    // 切換到輸入模式
    pinMode(DHT11_PIN, INPUT_PULLUP);
    
    // 等待 DHT11 回應
    unsigned long timeout = micros() + 1000;
    while (digitalRead(DHT11_PIN) == HIGH && micros() < timeout);
    if (micros() >= timeout) {
        Serial.println("DHT11 timeout waiting for start response");
        return false;
    }
    
    // 等待 80μs 低電平
    timeout = micros() + 100;
    while (digitalRead(DHT11_PIN) == LOW && micros() < timeout);
    if (micros() >= timeout) {
        Serial.println("DHT11 timeout in low response");
        return false;
    }
    
    // 等待 80μs 高電平
    timeout = micros() + 100;
    while (digitalRead(DHT11_PIN) == HIGH && micros() < timeout);
    if (micros() >= timeout) {
        Serial.println("DHT11 timeout in high response");
        return false;
    }
    
    // 讀取 40 位數據
    for (int i = 0; i < 40; i++) {
        // 等待數據位開始 (50μs 低電平)
        timeout = micros() + 100;
        while (digitalRead(DHT11_PIN) == LOW && micros() < timeout);
        if (micros() >= timeout) {
            Serial.printf("DHT11 timeout waiting for bit %d\n", i);
            return false;
        }
        
        // 測量高電平持續時間
        unsigned long startTime = micros();
        timeout = micros() + 100;
        while (digitalRead(DHT11_PIN) == HIGH && micros() < timeout);
        unsigned long duration = micros() - startTime;
        
        // 26-28μs = 0, 70μs = 1
        if (duration > 40) {
            data[byteIndex] |= (1 << (7 - bitCount));
        }
        
        bitCount++;
        if (bitCount == 8) {
            bitCount = 0;
            byteIndex++;
        }
    }
    
    // 校驗碼檢查
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        Serial.printf("DHT11 checksum error: calc=%d, recv=%d\n", checksum, data[4]);
        return false;
    }
    
    // DHT11 的數據格式：整數部分.小數部分
    currentDHT.humidity = data[0] + data[1] * 0.1;
    currentDHT.temperature = data[2] + data[3] * 0.1;
    currentDHT.valid = true;
    currentDHT.lastRead = now;
    
    Serial.printf("DHT11 read success: Temp=%.1f°C, Humidity=%.1f%%\n", 
                 currentDHT.temperature, currentDHT.humidity);
    
    return true;
}

// ==================== WiFi 配網功能 ====================
void initConfig() {
    EEPROM.begin(512);
    
    WiFi.mode(WIFI_STA);
    loadConfig();
    
    bool needSave = false;
    if (!config.valid || strlen(config.device_id) == 0 || config.device_id[0] < 32) {
        String mac = WiFi.macAddress();
        mac.replace(":", "");
        snprintf(config.device_id, sizeof(config.device_id), "dht11_%s", mac.substring(6).c_str());
        Serial.printf("Generated Device ID: %s\n", config.device_id);
        needSave = true;
    }
    
    if (!config.valid || strlen(config.mqtt_server) == 0 || config.mqtt_server[0] < 32) {
        strcpy(config.mqtt_server, DEFAULT_MQTT_SERVER);
        config.mqtt_port = DEFAULT_MQTT_PORT;
        Serial.printf("Reset MQTT Server: %s:%d\n", config.mqtt_server, config.mqtt_port);
        needSave = true;
    }
    
    if (needSave) {
        config.valid = true;
        saveConfig();
    }
    
    topic_command = "sensors/factory/" + String(config.device_id) + "/command";
    
    Serial.printf("Final Config - Device ID: %s\n", config.device_id);
    Serial.printf("Final Config - MQTT: %s:%d\n", config.mqtt_server, config.mqtt_port);
    Serial.printf("Topics - Temperature: %s\n", topic_temperature.c_str());
    Serial.printf("Topics - Humidity: %s\n", topic_humidity.c_str());
    Serial.printf("Topics - Status: %s\n", topic_status.c_str());
    Serial.printf("Topics - Command: %s\n", topic_command.c_str());
}

void loadConfig() {
    EEPROM.get(0, config);
    
    if (config.valid) {
        int ssidLen = strlen(config.wifi_ssid);
        bool validSSID = (ssidLen > 0 && ssidLen <= 31);
        
        if (validSSID) {
            for (int i = 0; i < ssidLen; i++) {
                if (config.wifi_ssid[i] < 32 || config.wifi_ssid[i] > 126) {
                    validSSID = false;
                    break;
                }
            }
        }
        
        int mqttLen = strlen(config.mqtt_server);
        bool validMQTT = (mqttLen > 0 && mqttLen <= 63);
        
        if (validMQTT) {
            for (int i = 0; i < mqttLen; i++) {
                if (config.mqtt_server[i] < 32 || config.mqtt_server[i] > 126) {
                    validMQTT = false;
                    break;
                }
            }
        }
        
        int deviceLen = strlen(config.device_id);
        bool validDevice = (deviceLen > 0 && deviceLen <= 31);
        
        if (validDevice) {
            for (int i = 0; i < deviceLen; i++) {
                if (config.device_id[i] < 32 || config.device_id[i] > 126) {
                    validDevice = false;
                    break;
                }
            }
        }
        
        if (!validSSID || !validMQTT || !validDevice) {
            Serial.println("Invalid saved data detected - will reset corrupted fields");
            if (!validSSID) {
                Serial.println("WiFi SSID corrupted");
                memset(config.wifi_ssid, 0, sizeof(config.wifi_ssid));
                memset(config.wifi_password, 0, sizeof(config.wifi_password));
            }
            if (!validMQTT) {
                Serial.println("MQTT server corrupted");
                memset(config.mqtt_server, 0, sizeof(config.mqtt_server));
            }
            if (!validDevice) {
                Serial.println("Device ID corrupted");
                memset(config.device_id, 0, sizeof(config.device_id));
            }
        } else {
            Serial.printf("Found saved WiFi: %s\n", config.wifi_ssid);
            Serial.printf("MQTT Server: %s:%d\n", config.mqtt_server, config.mqtt_port);
            Serial.printf("Device ID: %s\n", config.device_id);
        }
    } else {
        Serial.println("No valid config found - will initialize defaults");
    }
}

void saveConfig() {
    config.valid = true;
    EEPROM.put(0, config);
    EEPROM.commit();
    Serial.println("Configuration saved to EEPROM");
}

void clearConfig() {
    for (int i = 0; i < sizeof(Config); i++) {
        EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
    
    memset(&config, 0, sizeof(config));
    config.valid = false;
    config.mqtt_port = 0;
    
    Serial.println("Configuration completely cleared");
}

bool connectWiFi() {
    if (!config.valid || strlen(config.wifi_ssid) == 0) return false;
    
    Serial.printf("Connecting to WiFi: %s\n", config.wifi_ssid);
    
    // 確保完全斷開之前的連接
    WiFi.disconnect(true);
    WiFi.softAPdisconnect(true);
    delay(500);
    
    // 設定為 STA 模式
    WiFi.mode(WIFI_STA);
    delay(500);
    
    // 開始連接
    WiFi.begin(config.wifi_ssid, config.wifi_password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        IPAddress localIP = WiFi.localIP();
        Serial.printf("WiFi connected! IP: %s\n", localIP.toString().c_str());
        Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        Serial.printf("DNS: %s\n", WiFi.dnsIP().toString().c_str());
        Serial.printf("Signal: %d dBm\n", WiFi.RSSI());
        
        // 驗證 IP 不是 AP 模式的 IP
        if (localIP[0] == 192 && localIP[1] == 168 && localIP[2] == 42) {
            Serial.println("WARNING: Got AP mode IP, connection may be invalid");
            return false;
        }
        
        digitalWrite(LED_PIN, HIGH);
        return true;
    } else {
        Serial.println("\nWiFi connection failed");
        Serial.printf("WiFi Status: %d\n", WiFi.status());
        digitalWrite(LED_PIN, LOW);
        return false;
    }
}

void startConfigMode() {
    configMode = true;
    mqttConnected = false;
    
    Serial.println("Starting configuration mode...");
    
    mqttClient.disconnect();
    WiFi.disconnect();
    server.stop();
    delay(100);
    
    WiFi.mode(WIFI_AP);
    bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    if (apStarted) {
        delay(1000);
        IPAddress apIP = WiFi.softAPIP();
        
        Serial.println("AP started successfully");
        Serial.printf("SSID: %s\n", AP_SSID);
        Serial.printf("Password: %s\n", AP_PASSWORD);
        Serial.printf("IP: %s\n", apIP.toString().c_str());
        
        setupConfigWebServer();
        server.begin();
        Serial.printf("Web server started - browse to %s\n", apIP.toString().c_str());
        
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
    
    // 完全停止 AP 模式
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true);
    delay(500);
    
    // 切換到 STA 模式
    WiFi.mode(WIFI_STA);
    delay(1000);
    
    if (!connectWiFi()) {
        Serial.println("Failed to connect in normal mode");
        startConfigMode();
        return;
    }
    
    // 驗證是否真正連接到路由器
    IPAddress localIP = WiFi.localIP();
    Serial.printf("Connected IP: %s\n", localIP.toString().c_str());
    
    // 檢查是否還在 AP 模式的 IP 範圍
    if (localIP[0] == 192 && localIP[1] == 168 && localIP[2] == 42) {
        Serial.println("ERROR: Still in AP mode IP range - connection failed");
        startConfigMode();
        return;
    }
    
    mqttClient.setServer(config.mqtt_server, config.mqtt_port);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(1024);
    mqttClient.setKeepAlive(60);
    mqttClient.setSocketTimeout(15);
    
    server.stop();
    delay(100);
    
    setupStatusWebServer();
    server.begin();
    
    Serial.println("Normal mode - connecting to MQTT...");
    Serial.printf("Device ready at: http://%s\n", WiFi.localIP().toString().c_str());
    
    connectMQTT();
}

// ==================== Web 伺服器 ====================
void setupConfigWebServer() {
    server.on("/", handleConfigRoot);
    server.on("/scan", handleScan);
    server.on("/connect", HTTP_POST, handleConnect);
    server.on("/test", []() {
        server.send(200, "text/plain", "Pico 2 W DHT11 Configuration Mode");
    });
    server.onNotFound([]() {
        server.send(404, "text/plain", "Not Found");
    });
}

void setupStatusWebServer() {
    server.on("/", handleStatus);
    server.on("/reset", handleReset);
    server.on("/mqtt", handleMQTTStatus);
    server.on("/sensor", handleSensorData);
    server.onNotFound([]() {
        server.send(404, "text/plain", "Not Found");
    });
}

void handleConfigRoot() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>Pico 2W DHT11 Setup</title>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<style>body{font-family:Arial;padding:20px;max-width:600px;margin:0 auto;}";
    html += "input{width:100%;padding:8px;margin:5px 0;box-sizing:border-box;}";
    html += "button{background:#4CAF50;color:white;padding:10px;border:none;cursor:pointer;width:100%;}";
    html += ".network{padding:5px;border:1px solid #ddd;margin:2px;cursor:pointer;}";
    html += ".network:hover{background:#f0f0f0;}</style></head><body>";
    
    html += "<h1>Pico 2W DHT11 Setup</h1>";
    html += "<p>Device ID: <strong>" + String(config.device_id) + "</strong></p>";
    
    if (server.hasArg("scan")) {
        html += "<h3>Available Networks:</h3>";
        int n = WiFi.scanNetworks();
        for (int i = 0; i < n && i < 10; i++) {
            String ssid = WiFi.SSID(i);
            html += "<div class='network' onclick=\"document.getElementById('ssid').value='" + ssid + "'\">";
            html += ssid + " (" + String(WiFi.RSSI(i)) + " dBm)";
            if (WiFi.encryptionType(i) == ENC_TYPE_NONE) {
                html += " [Open]";
            } else {
                html += " [Secured]";
            }
            html += "</div>";
        }
        html += "<hr>";
    }
    
    html += "<form action='/connect' method='post'>";
    html += "<h3>WiFi Settings</h3>";
    html += "<p>SSID: <input type='text' id='ssid' name='ssid' required value='" + String(config.wifi_ssid) + "'></p>";
    html += "<p>Password: <input type='password' name='password'></p>";
    
    html += "<h3>MQTT Settings</h3>";
    html += "<p>MQTT Server: <input type='text' name='mqtt_server' value='" + String(config.mqtt_server) + "'></p>";
    html += "<p>MQTT Port: <input type='number' name='mqtt_port' value='" + String(config.mqtt_port) + "'></p>";
    
    html += "<p><button type='submit'>Save & Connect</button></p>";
    html += "</form>";
    
    html += "<p><a href='/?scan=1'><button type='button'>Scan Networks</button></a></p>";
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
    String mqtt_server = server.arg("mqtt_server");
    String mqtt_port = server.arg("mqtt_port");
    
    if (ssid.length() == 0) {
        server.send(400, "text/html", "<h1>Error</h1><p>SSID required</p><a href='/'>Back</a>");
        return;
    }
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta http-equiv='refresh' content='25;url=http://192.168.50.1/'>";
    html += "<title>Connecting...</title></head><body>";
    html += "<h1>Connecting to " + ssid + "...</h1>";
    html += "<p>Configuring MQTT: " + mqtt_server + ":" + mqtt_port + "</p>";
    html += "<p>Please wait... Device will restart and get new IP from router.</p>";
    html += "<p>Check your router's DHCP table or try http://192.168.50.x range</p>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
    
    // 儲存配置
    ssid.toCharArray(config.wifi_ssid, sizeof(config.wifi_ssid));
    password.toCharArray(config.wifi_password, sizeof(config.wifi_password));
    mqtt_server.toCharArray(config.mqtt_server, sizeof(config.mqtt_server));
    config.mqtt_port = mqtt_port.toInt();
    
    if (config.mqtt_port == 0) config.mqtt_port = DEFAULT_MQTT_PORT;
    
    // 儲存配置後重啟，避免網路狀態混亂
    saveConfig();
    Serial.println("Configuration saved - restarting device");
    delay(3000);
    rp2040.restart();
}

void handleStatus() {
    String html = "<!DOCTYPE html><html><head><title>Pico 2W DHT11 Status</title>";
    html += "<meta http-equiv='refresh' content='10'>";
    html += "<style>body{font-family:Arial;padding:20px;max-width:800px;margin:0 auto;}";
    html += ".status{background:#f0f0f0;padding:10px;margin:10px 0;border-radius:5px;}";
    html += ".ok{background:#d4edda;} .error{background:#f8d7da;}</style></head><body>";
    
    html += "<h1>Pico 2W DHT11 Status</h1>";
    
    html += "<div class='status " + String(WiFi.status() == WL_CONNECTED ? "ok" : "error") + "'>";
    html += "<h3>WiFi Status</h3>";
    if (WiFi.status() == WL_CONNECTED) {
        html += "<p><strong>Connected to:</strong> " + WiFi.SSID() + "</p>";
        html += "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>";
        html += "<p><strong>Signal Strength:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
        html += "<p><strong>MAC Address:</strong> " + WiFi.macAddress() + "</p>";
    } else {
        html += "<p><strong>Status:</strong> Disconnected</p>";
    }
    html += "</div>";
    
    html += "<div class='status " + String(mqttConnected ? "ok" : "error") + "'>";
    html += "<h3>MQTT Status</h3>";
    html += "<p><strong>Server:</strong> " + String(config.mqtt_server) + ":" + String(config.mqtt_port) + "</p>";
    html += "<p><strong>Device ID:</strong> " + String(config.device_id) + "</p>";
    html += "<p><strong>Connection:</strong> " + String(mqttConnected ? "Connected" : "Disconnected") + "</p>";
    html += "<p><strong>Messages Sent:</strong> " + String(msg_count) + "</p>";
    html += "</div>";
    
    html += "<div class='status " + String(currentDHT.valid ? "ok" : "error") + "'>";
    html += "<h3>DHT11 Data</h3>";
    if (currentDHT.valid) {
        html += "<p><strong>Temperature:</strong> " + String(currentDHT.temperature, 1) + " °C</p>";
        html += "<p><strong>Humidity:</strong> " + String(currentDHT.humidity, 1) + " %</p>";
        html += "<p><strong>Last Reading:</strong> " + String((millis() - currentDHT.lastRead)/1000) + " seconds ago</p>";
    } else {
        html += "<p><strong>Status:</strong> No valid data</p>";
    }
    html += "<p><strong>Uptime:</strong> " + String(millis() / 1000) + " seconds</p>";
    html += "<p><strong>Free Memory:</strong> " + String(rp2040.getFreeHeap()) + " bytes</p>";
    html += "</div>";
    
    html += "<p><a href='/reset'><button style='background:#dc3545;color:white;padding:10px;border:none;'>Reset Settings</button></a></p>";
    html += "<p><a href='/mqtt'><button style='background:#007bff;color:white;padding:10px;border:none;'>MQTT Details</button></a></p>";
    html += "<p><a href='/sensor'><button style='background:#28a745;color:white;padding:10px;border:none;'>Sensor JSON</button></a></p>";
    
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

void handleReset() {
    server.send(200, "text/html", 
        "<h1>Resetting...</h1><p>Device settings cleared. Device will restart in config mode.</p>");
    delay(2000);
    clearConfig();
    rp2040.restart();
}

void handleMQTTStatus() {
    DynamicJsonDocument doc(512);
    doc["device_id"] = config.device_id;
    doc["mqtt_server"] = config.mqtt_server;
    doc["mqtt_port"] = config.mqtt_port;
    doc["mqtt_connected"] = mqttConnected;
    doc["topics"]["temperature"] = topic_temperature;
    doc["topics"]["humidity"] = topic_humidity;
    doc["topics"]["status"] = topic_status;
    doc["topics"]["command"] = topic_command;
    doc["message_count"] = msg_count;
    
    String jsonString;
    serializeJson(doc, jsonString);
    server.send(200, "application/json", jsonString);
}

void handleSensorData() {
    DynamicJsonDocument doc(512);
    doc["device_id"] = config.device_id;
    doc["temperature"] = currentDHT.temperature;
    doc["humidity"] = currentDHT.humidity;
    doc["sensor_valid"] = currentDHT.valid;
    doc["last_read"] = currentDHT.lastRead;
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["uptime"] = millis();
    doc["free_heap"] = rp2040.getFreeHeap();
    doc["timestamp"] = millis();
    
    String jsonString;
    serializeJson(doc, jsonString);
    server.send(200, "application/json", jsonString);
}

// ==================== MQTT 功能 ====================
void connectMQTT() {
    if (!config.valid || WiFi.status() != WL_CONNECTED) return;
    
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        
        if (mqttClient.connect(config.device_id)) {
            Serial.println("MQTT connected!");
            mqttConnected = true;
            
            publishStatus("online");
            
            mqttClient.subscribe(topic_command.c_str());
            Serial.printf("Subscribed to: %s\n", topic_command.c_str());
            
            digitalWrite(LED_PIN, HIGH);
            
        } else {
            Serial.printf("MQTT connection failed, rc=%d, retry in 5 seconds\n", mqttClient.state());
            mqttConnected = false;
            printMQTTError(mqttClient.state());
            delay(5000);
        }
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.printf("Message received [%s]: ", topic);
    
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);
    
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, message);
    
    if (!error) {
        const char* command = doc["command"];
        if (command != nullptr) {
            if (strcmp(command, "led_on") == 0) {
                digitalWrite(LED_PIN, HIGH);
                Serial.println("LED turned ON via MQTT");
                publishStatus("led_on");
            } else if (strcmp(command, "led_off") == 0) {
                digitalWrite(LED_PIN, LOW);
                Serial.println("LED turned OFF via MQTT");
                publishStatus("led_off");
            } else if (strcmp(command, "status") == 0) {
                publishStatus("running");
            } else if (strcmp(command, "reset") == 0) {
                Serial.println("Reset command received via MQTT");
                publishStatus("resetting");
                delay(1000);
                rp2040.restart();
            } else if (strcmp(command, "get_data") == 0) {
                publishDHTData();
            }
        }
    }
}

void publishDHTData() {
    if (!mqttConnected || !mqttClient.connected()) {
        Serial.println("MQTT not connected for DHT data");
        return;
    }
    
    unsigned long now = millis();
    if (now - lastPublishAttempt < MIN_PUBLISH_INTERVAL) {
        return;
    }
    lastPublishAttempt = now;
    
    if (!currentDHT.valid) {
        Serial.println("No valid DHT data to publish");
        return;
    }
    
    bool allPublishSuccess = true;
    
    // 發布溫度數據
    DynamicJsonDocument tempDoc(512);
    tempDoc["device_id"] = config.device_id;
    tempDoc["location"] = "factory";
    tempDoc["sensor_type"] = "temperature";
    tempDoc["temperature"] = round(currentDHT.temperature * 10) / 10.0;
    tempDoc["wifi_rssi"] = WiFi.RSSI();
    tempDoc["uptime"] = millis() / 1000;
    tempDoc["free_heap"] = rp2040.getFreeHeap();
    
    String tempJson;
    serializeJson(tempDoc, tempJson);
    
    if (!mqttClient.publish(topic_temperature.c_str(), tempJson.c_str())) {
        allPublishSuccess = false;
        Serial.println("Temperature publish failed");
    }
    
    delay(100);
    
    // 發布濕度數據
    DynamicJsonDocument humDoc(512);
    humDoc["device_id"] = config.device_id;
    humDoc["location"] = "factory";
    humDoc["sensor_type"] = "humidity";
    humDoc["humidity"] = round(currentDHT.humidity * 10) / 10.0;
    humDoc["wifi_rssi"] = WiFi.RSSI();
    humDoc["uptime"] = millis() / 1000;
    humDoc["free_heap"] = rp2040.getFreeHeap();
    
    String humJson;
    serializeJson(humDoc, humJson);
    
    if (!mqttClient.publish(topic_humidity.c_str(), humJson.c_str())) {
        allPublishSuccess = false;
        Serial.println("Humidity publish failed");
    }
    
    if (allPublishSuccess) {
        msg_count++;
        publishFailCount = 0;
        Serial.printf("DHT data published (#%d) - Temp: %.1f°C, Humidity: %.1f%%\n", 
                     msg_count, currentDHT.temperature, currentDHT.humidity);
        
        digitalWrite(LED_PIN, LOW);
        delay(50);
        digitalWrite(LED_PIN, HIGH);
        
    } else {
        publishFailCount++;
        Serial.printf("DHT data publish failed (fail count: %d)\n", publishFailCount);
        
        if (!mqttClient.connected()) {
            mqttConnected = false;
        }
        
        if (publishFailCount >= MAX_PUBLISH_FAILS) {
            Serial.println("Too many publish failures, reconnecting MQTT...");
            mqttClient.disconnect();
            mqttConnected = false;
            publishFailCount = 0;
        }
    }
}

void publishStatus(const char* status) {
    if (!mqttConnected || !mqttClient.connected()) return;
    
    DynamicJsonDocument doc(256);
    doc["device_id"] = config.device_id;
    doc["location"] = "factory";
    doc["sensor_type"] = "status";
    doc["status"] = status;
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["uptime"] = millis() / 1000;
    doc["free_heap"] = rp2040.getFreeHeap();
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    if (mqttClient.publish(topic_status.c_str(), jsonString.c_str())) {
        Serial.printf("Status published: %s\n", status);
    } else {
        Serial.printf("Status publish failed: %s\n", status);
    }
}

void printMQTTError(int state) {
    switch (state) {
        case -4: Serial.println("  -> Connection timeout"); break;
        case -3: Serial.println("  -> Connection lost"); break;
        case -2: Serial.println("  -> Connect failed"); break;
        case -1: Serial.println("  -> Disconnected"); break;
        case 0:  Serial.println("  -> Connected"); break;
        case 1:  Serial.println("  -> Bad protocol version"); break;
        case 2:  Serial.println("  -> Bad client ID"); break;
        case 3:  Serial.println("  -> Unavailable"); break;
        case 4:  Serial.println("  -> Bad credentials"); break;
        case 5:  Serial.println("  -> Unauthorized"); break;
        default: Serial.println("  -> Unknown error"); break;
    }
}

// ==================== 主程式 ====================
void setup() {
    pinMode(LED_PIN, OUTPUT);
    pinMode(RESET_PIN, INPUT_PULLUP);
    
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== Pico 2W DHT11 MQTT Sensor ===");
    Serial.println("Initializing...");
    
    // 初始化 DHT11
    initDHT11();
    
    // 初始化配置
    initConfig();
    
    // 檢查重置按鈕
    if (digitalRead(RESET_PIN) == LOW) {
        Serial.println("Reset button pressed - clearing config");
        clearConfig();
        delay(1000);
    }
    
    // 嘗試連接已儲存的 WiFi
    if (config.valid && strlen(config.wifi_ssid) > 0 && connectWiFi()) {
        Serial.println("Connected to saved WiFi - starting normal mode");
        startNormalMode();
    } else {
        Serial.println("No valid WiFi config - starting configuration mode");
        startConfigMode();
    }
    
    Serial.println("Setup complete");
}

void loop() {
    server.handleClient();
    
    if (!configMode) {
        // 檢查 MQTT 連接
        if (!mqttClient.connected()) {
            mqttConnected = false;
            if (WiFi.status() == WL_CONNECTED) {
                connectMQTT();
            }
        } else {
            mqttClient.loop();
        }
        
        // 讀取 DHT11 感測器 (每3秒一次)
        unsigned long now = millis();
        if (now - lastSensorRead >= SENSOR_READ_INTERVAL) {
            lastSensorRead = now;
            
            if (readDHT11()) {
                Serial.printf("DHT11 Data - Temp: %.1f°C, Humidity: %.1f%%\n", 
                             currentDHT.temperature, currentDHT.humidity);
            } else {
                Serial.println("DHT11 read failed");
            }
        }
        
        // 定時發送 DHT 資料 (3秒一次)
        if (now - lastMsg > MSG_INTERVAL) {
            lastMsg = now;
            if (mqttConnected && currentDHT.valid) {
                publishDHTData();
            }
        }
        
        // 定時發送狀態資訊
        if (now - lastStatus > STATUS_INTERVAL) {
            lastStatus = now;
            if (mqttConnected) {
                publishStatus("running");
            }
        }
        
        // 檢查 WiFi 連接
        if (WiFi.status() != WL_CONNECTED) {
            static unsigned long lastCheck = 0;
            if (now - lastCheck > 30000) {
                Serial.println("WiFi disconnected - attempting reconnect");
                WiFi.begin(config.wifi_ssid, config.wifi_password);
                lastCheck = now;
            }
        }
    }
    
    // 檢查重置按鈕 (長按3秒)
    if (digitalRead(RESET_PIN) == LOW) {
        delay(50);
        if (digitalRead(RESET_PIN) == LOW) {
            unsigned long pressTime = millis();
            while (digitalRead(RESET_PIN) == LOW) {
                delay(10);
            }
            if (millis() - pressTime > 3000) {
                Serial.println("Reset button held - clearing config and restarting");
                clearConfig();
                rp2040.restart();
            }
        }
    }
    
    // 配網模式下的 LED 慢閃
    if (configMode) {
        static unsigned long lastBlink = 0;
        unsigned long now = millis();
        if (now - lastBlink > 1000) {
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            lastBlink = now;
        }
    }
    
    delay(10);
}