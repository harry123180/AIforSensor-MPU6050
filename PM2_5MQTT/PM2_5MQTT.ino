#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

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

// PMSA003i UART 定義
#define PMSA_UART uart1
#define PMSA_TX_PIN 0
#define PMSA_RX_PIN 1
#define PMSA_BAUD 9600

// 定時器參數
const unsigned long MSG_INTERVAL = 30000;  // 30秒發送一次
const unsigned long STATUS_INTERVAL = 300000;  // 5分鐘發送狀態
const unsigned long MIN_PUBLISH_INTERVAL = 5000;  // 最小發布間隔 5 秒
const unsigned long SENSOR_READ_INTERVAL = 1000;  // 1秒讀取一次感測器

// ==================== 資料結構 ====================
struct Config {
    char wifi_ssid[32];
    char wifi_password[64];
    char mqtt_server[64];
    int mqtt_port;
    char device_id[32];
    bool valid;
};

// PMSA003i 數據結構
struct PMData {
    // Standard particle concentrations (μg/m³)
    uint16_t pm10_standard;
    uint16_t pm25_standard;
    uint16_t pm100_standard;
    
    // Environmental particle concentrations (μg/m³)
    uint16_t pm10_env;
    uint16_t pm25_env;
    uint16_t pm100_env;
    
    // Particle counts (particles per 0.1L air)
    uint16_t particles_03um;
    uint16_t particles_05um;
    uint16_t particles_10um;
    uint16_t particles_25um;
    uint16_t particles_50um;
    uint16_t particles_100um;
    
    bool valid;
};

// 平均值計算結構
struct PMAverage {
    float pm10_standard_sum;
    float pm25_standard_sum;
    float pm100_standard_sum;
    float pm10_env_sum;
    float pm25_env_sum;
    float pm100_env_sum;
    float particles_03um_sum;
    float particles_05um_sum;
    float particles_10um_sum;
    float particles_25um_sum;
    float particles_50um_sum;
    float particles_100um_sum;
    int sample_count;
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

// PMSA003i 資料
PMData currentPM;
PMAverage pmAverage;
uint8_t pmBuffer[32];
int bufferIndex = 0;

// MQTT 主題定義
String topic_pm1 = "sensors/factory/air_quality/pm1";
String topic_pm25 = "sensors/factory/air_quality/pm25";
String topic_pm10 = "sensors/factory/air_quality/pm10";
String topic_status = "sensors/factory/status";
String topic_command;  // 命令主題將在 initConfig 中設定

// ==================== PMSA003i 功能 ====================
void initPMSA003i() {
    // 初始化 UART1 for PMSA003i
    Serial1.setTX(PMSA_TX_PIN);
    Serial1.setRX(PMSA_RX_PIN);
    Serial1.begin(PMSA_BAUD);
    
    // 初始化數據結構
    memset(&currentPM, 0, sizeof(currentPM));
    memset(&pmAverage, 0, sizeof(pmAverage));
    
    Serial.println("PMSA003i initialized on UART1");
    Serial.printf("TX Pin: GP%d, RX Pin: GP%d, Baud: %d\n", PMSA_TX_PIN, PMSA_RX_PIN, PMSA_BAUD);
    
    // 清空串口緩衝區
    delay(2000);
    while (Serial1.available()) {
        Serial1.read();
    }
    
    Serial.println("PMSA003i UART buffer cleared");
    Serial.println("Starting data analysis...");
    
    // 分析接收到的數據模式
    unsigned long startTime = millis();
    int byteCount = 0;
    
    while (millis() - startTime < 5000) {  // 分析5秒
        if (Serial1.available()) {
            uint8_t byte = Serial1.read();
            byteCount++;
            
            if (byteCount <= 50) {  // 只顯示前50個字節
                Serial.printf("Byte %d: 0x%02X ", byteCount, byte);
                if (byteCount % 16 == 0) Serial.println();
            }
            
            if (byteCount == 50) {
                Serial.printf("\n... (showing first 50 bytes)\n");
            }
        }
        delay(10);
    }
    
    Serial.printf("Total bytes received in 5 seconds: %d\n", byteCount);
    Serial.printf("Expected rate: ~6.4 bytes/sec (32 bytes every 5 seconds)\n");
    Serial.printf("Actual rate: %.1f bytes/sec\n", byteCount / 5.0);
    
    if (byteCount == 0) {
        Serial.println("ERROR: No data received - check connections!");
    } else if (byteCount > 50) {
        Serial.println("WARNING: Too much data - possible connection issue");
    }
    
    Serial.println("PMSA003i analysis complete, starting normal operation...");
}

bool readPMSA003i() {
    static unsigned long lastErrorTime = 0;
    static int errorCount = 0;
    static unsigned long lastDebugTime = 0;
    static int totalBytes = 0;
    static unsigned long lastValidData = 0;
    
    while (Serial1.available()) {
        uint8_t byte = Serial1.read();
        totalBytes++;
        
        // 每10秒顯示一次統計
        unsigned long now = millis();
        if (now - lastDebugTime > 10000) {
            Serial.printf("UART Stats: %d bytes/10s, buffer pos: %d, last valid: %lus ago\n", 
                         totalBytes, bufferIndex, (now - lastValidData)/1000);
            lastDebugTime = now;
            totalBytes = 0;
        }
        
        // 狀態機處理封包接收
        switch (bufferIndex) {
            case 0:
                // 等待第一個起始字節 0x42
                if (byte == 0x42) {
                    pmBuffer[bufferIndex++] = byte;
                }
                break;
                
            case 1:
                // 等待第二個起始字節 0x4D
                if (byte == 0x4D) {
                    pmBuffer[bufferIndex++] = byte;
                } else if (byte == 0x42) {
                    // 可能是新封包的開始
                    pmBuffer[0] = byte;
                    bufferIndex = 1;
                } else {
                    // 重新開始
                    bufferIndex = 0;
                }
                break;
                
            case 2:
                // 檢查長度高字節 (應該是 0x00)
                if (byte == 0x00) {
                    pmBuffer[bufferIndex++] = byte;
                } else {
                    // 重新開始尋找
                    bufferIndex = 0;
                    if (byte == 0x42) {
                        pmBuffer[0] = byte;
                        bufferIndex = 1;
                    }
                }
                break;
                
            case 3:
                // 檢查長度低字節 (應該是 0x1C = 28)
                if (byte == 0x1C) {
                    pmBuffer[bufferIndex++] = byte;
                } else {
                    // 重新開始尋找
                    bufferIndex = 0;
                    if (byte == 0x42) {
                        pmBuffer[0] = byte;
                        bufferIndex = 1;
                    }
                }
                break;
                
            default:
                // 收集剩餘數據
                pmBuffer[bufferIndex++] = byte;
                
                // 檢查是否意外遇到新的封包起始
                if (bufferIndex > 4 && byte == 0x42 && bufferIndex < 30) {
                    // 可能是錯位的新封包，重新開始
                    Serial.printf("Unexpected 0x42 at position %d, restarting\n", bufferIndex);
                    pmBuffer[0] = byte;
                    bufferIndex = 1;
                    break;
                }
                
                // 完整封包檢查
                if (bufferIndex == 32) {
                    // 最後檢查：確保沒有內嵌的起始標記
                    bool hasEmbeddedStart = false;
                    for (int i = 4; i < 30; i++) {
                        if (pmBuffer[i] == 0x42 && pmBuffer[i+1] == 0x4D) {
                            hasEmbeddedStart = true;
                            Serial.printf("Found embedded start at position %d\n", i);
                            break;
                        }
                    }
                    
                    if (hasEmbeddedStart) {
                        // 有內嵌起始標記，丟棄這個封包
                        Serial.println("Discarding packet with embedded start marker");
                        bufferIndex = 0;
                        break;
                    }
                    
                    // 驗證校驗碼
                    uint16_t checksum = 0;
                    for (int i = 0; i < 30; i++) {
                        checksum += pmBuffer[i];
                    }
                    
                    uint16_t receivedChecksum = (pmBuffer[30] << 8) | pmBuffer[31];
                    
                    if (checksum == receivedChecksum) {
                        // 解析數據
                        currentPM.pm10_standard = (pmBuffer[4] << 8) | pmBuffer[5];
                        currentPM.pm25_standard = (pmBuffer[6] << 8) | pmBuffer[7];
                        currentPM.pm100_standard = (pmBuffer[8] << 8) | pmBuffer[9];
                        
                        currentPM.pm10_env = (pmBuffer[10] << 8) | pmBuffer[11];
                        currentPM.pm25_env = (pmBuffer[12] << 8) | pmBuffer[13];
                        currentPM.pm100_env = (pmBuffer[14] << 8) | pmBuffer[15];
                        
                        currentPM.particles_03um = (pmBuffer[16] << 8) | pmBuffer[17];
                        currentPM.particles_05um = (pmBuffer[18] << 8) | pmBuffer[19];
                        currentPM.particles_10um = (pmBuffer[20] << 8) | pmBuffer[21];
                        currentPM.particles_25um = (pmBuffer[22] << 8) | pmBuffer[23];
                        currentPM.particles_50um = (pmBuffer[24] << 8) | pmBuffer[25];
                        currentPM.particles_100um = (pmBuffer[26] << 8) | pmBuffer[27];
                        
                        currentPM.valid = true;
                        errorCount = 0;
                        lastValidData = now;
                        
                        Serial.printf("✓ Valid PM data: PM1.0=%d, PM2.5=%d, PM10=%d μg/m³\n", 
                                     currentPM.pm10_standard, currentPM.pm25_standard, currentPM.pm100_standard);
                        
                        bufferIndex = 0;
                        addToAverage();
                        return true;
                        
                    } else {
                        errorCount++;
                        
                        if (now - lastErrorTime > 5000) {
                            Serial.printf("Checksum mismatch #%d - calc: 0x%04X, recv: 0x%04X\n", 
                                         errorCount, checksum, receivedChecksum);
                            Serial.print("Packet header: ");
                            for (int i = 0; i < 8; i++) {
                                Serial.printf("%02X ", pmBuffer[i]);
                            }
                            Serial.print("... tail: ");
                            for (int i = 28; i < 32; i++) {
                                Serial.printf("%02X ", pmBuffer[i]);
                            }
                            Serial.println();
                            lastErrorTime = now;
                        }
                        
                        // 重新開始
                        bufferIndex = 0;
                        
                        if (errorCount >= 5) {
                            Serial.println("Too many errors - full UART flush");
                            while (Serial1.available()) {
                                Serial1.read();
                            }
                            errorCount = 0;
                            delay(500);  // 更長的延遲等待穩定
                        }
                    }
                }
                break;
        }
    }
    return false;
}

void addToAverage() {
    pmAverage.pm10_standard_sum += currentPM.pm10_standard;
    pmAverage.pm25_standard_sum += currentPM.pm25_standard;
    pmAverage.pm100_standard_sum += currentPM.pm100_standard;
    pmAverage.pm10_env_sum += currentPM.pm10_env;
    pmAverage.pm25_env_sum += currentPM.pm25_env;
    pmAverage.pm100_env_sum += currentPM.pm100_env;
    pmAverage.particles_03um_sum += currentPM.particles_03um;
    pmAverage.particles_05um_sum += currentPM.particles_05um;
    pmAverage.particles_10um_sum += currentPM.particles_10um;
    pmAverage.particles_25um_sum += currentPM.particles_25um;
    pmAverage.particles_50um_sum += currentPM.particles_50um;
    pmAverage.particles_100um_sum += currentPM.particles_100um;
    pmAverage.sample_count++;
}

PMData getAverageData() {
    PMData avgData;
    
    if (pmAverage.sample_count > 0) {
        avgData.pm10_standard = pmAverage.pm10_standard_sum / pmAverage.sample_count;
        avgData.pm25_standard = pmAverage.pm25_standard_sum / pmAverage.sample_count;
        avgData.pm100_standard = pmAverage.pm100_standard_sum / pmAverage.sample_count;
        avgData.pm10_env = pmAverage.pm10_env_sum / pmAverage.sample_count;
        avgData.pm25_env = pmAverage.pm25_env_sum / pmAverage.sample_count;
        avgData.pm100_env = pmAverage.pm100_env_sum / pmAverage.sample_count;
        avgData.particles_03um = pmAverage.particles_03um_sum / pmAverage.sample_count;
        avgData.particles_05um = pmAverage.particles_05um_sum / pmAverage.sample_count;
        avgData.particles_10um = pmAverage.particles_10um_sum / pmAverage.sample_count;
        avgData.particles_25um = pmAverage.particles_25um_sum / pmAverage.sample_count;
        avgData.particles_50um = pmAverage.particles_50um_sum / pmAverage.sample_count;
        avgData.particles_100um = pmAverage.particles_100um_sum / pmAverage.sample_count;
        avgData.valid = true;
    } else {
        memset(&avgData, 0, sizeof(avgData));
        avgData.valid = false;
    }
    
    return avgData;
}

void resetAverage() {
    memset(&pmAverage, 0, sizeof(pmAverage));
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
        snprintf(config.device_id, sizeof(config.device_id), "pmsa_%s", mac.substring(6).c_str());
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
    Serial.printf("Topics - PM1.0: %s\n", topic_pm1.c_str());
    Serial.printf("Topics - PM2.5: %s\n", topic_pm25.c_str());
    Serial.printf("Topics - PM10: %s\n", topic_pm10.c_str());
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
        server.send(200, "text/plain", "Pico 2 W PMSA003i Configuration Mode");
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
    html += "<title>Pico 2W PMSA003i Setup</title>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<style>body{font-family:Arial;padding:20px;max-width:600px;margin:0 auto;}";
    html += "input{width:100%;padding:8px;margin:5px 0;box-sizing:border-box;}";
    html += "button{background:#4CAF50;color:white;padding:10px;border:none;cursor:pointer;width:100%;}";
    html += ".network{padding:5px;border:1px solid #ddd;margin:2px;cursor:pointer;}";
    html += ".network:hover{background:#f0f0f0;}</style></head><body>";
    
    html += "<h1>Pico 2W PMSA003i Setup</h1>";
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
    html += "<meta http-equiv='refresh' content='25;url=http://192.168.50.1/'>";  // 修改重定向
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
    String html = "<!DOCTYPE html><html><head><title>Pico 2W PMSA003i Status</title>";
    html += "<meta http-equiv='refresh' content='30'>";
    html += "<style>body{font-family:Arial;padding:20px;max-width:800px;margin:0 auto;}";
    html += ".status{background:#f0f0f0;padding:10px;margin:10px 0;border-radius:5px;}";
    html += ".ok{background:#d4edda;} .error{background:#f8d7da;}</style></head><body>";
    
    html += "<h1>Pico 2W PMSA003i Status</h1>";
    
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
    
    html += "<div class='status " + String(currentPM.valid ? "ok" : "error") + "'>";
    html += "<h3>PMSA003i Data</h3>";
    if (currentPM.valid) {
        html += "<p><strong>PM1.0 (Standard):</strong> " + String(currentPM.pm10_standard) + " μg/m³</p>";
        html += "<p><strong>PM2.5 (Standard):</strong> " + String(currentPM.pm25_standard) + " μg/m³</p>";
        html += "<p><strong>PM10 (Standard):</strong> " + String(currentPM.pm100_standard) + " μg/m³</p>";
        html += "<p><strong>PM1.0 (Environmental):</strong> " + String(currentPM.pm10_env) + " μg/m³</p>";
        html += "<p><strong>PM2.5 (Environmental):</strong> " + String(currentPM.pm25_env) + " μg/m³</p>";
        html += "<p><strong>PM10 (Environmental):</strong> " + String(currentPM.pm100_env) + " μg/m³</p>";
        html += "<p><strong>Sample Count:</strong> " + String(pmAverage.sample_count) + "</p>";
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
    doc["topics"]["pm1"] = topic_pm1;
    doc["topics"]["pm25"] = topic_pm25;
    doc["topics"]["pm10"] = topic_pm10;
    doc["topics"]["status"] = topic_status;
    doc["topics"]["command"] = topic_command;
    doc["message_count"] = msg_count;
    
    String jsonString;
    serializeJson(doc, jsonString);
    server.send(200, "application/json", jsonString);
}

void handleSensorData() {
    PMData avgData = getAverageData();
    
    DynamicJsonDocument doc(1024);
    doc["device_id"] = config.device_id;
    doc["sample_count"] = pmAverage.sample_count;
    
    if (avgData.valid) {
        doc["pm10_standard"] = avgData.pm10_standard;
        doc["pm25_standard"] = avgData.pm25_standard;
        doc["pm100_standard"] = avgData.pm100_standard;
        doc["pm10_env"] = avgData.pm10_env;
        doc["pm25_env"] = avgData.pm25_env;
        doc["pm100_env"] = avgData.pm100_env;
        doc["particles_03um"] = avgData.particles_03um;
        doc["particles_05um"] = avgData.particles_05um;
        doc["particles_10um"] = avgData.particles_10um;
        doc["particles_25um"] = avgData.particles_25um;
        doc["particles_50um"] = avgData.particles_50um;
        doc["particles_100um"] = avgData.particles_100um;
    }
    
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
                publishPMData();
            }
        }
    }
}

void publishPMData() {
    if (!mqttConnected || !mqttClient.connected()) {
        Serial.println("MQTT not connected for PM data");
        return;
    }
    
    unsigned long now = millis();
    if (now - lastPublishAttempt < MIN_PUBLISH_INTERVAL) {
        return;
    }
    lastPublishAttempt = now;
    
    PMData avgData = getAverageData();
    
    if (!avgData.valid || pmAverage.sample_count == 0) {
        Serial.println("No valid PM data to publish");
        return;
    }
    
    bool allPublishSuccess = true;
    
    // 發布 PM1.0 數據
    DynamicJsonDocument doc1(512);
    doc1["device_id"] = config.device_id;
    doc1["location"] = "factory";
    doc1["sensor_type"] = "pm1";
    doc1["pm10_standard"] = avgData.pm10_standard;
    doc1["pm10_env"] = avgData.pm10_env;
    doc1["particles_03um"] = avgData.particles_03um;
    doc1["particles_05um"] = avgData.particles_05um;
    doc1["particles_10um"] = avgData.particles_10um;
    doc1["sample_count"] = pmAverage.sample_count;
    doc1["wifi_rssi"] = WiFi.RSSI();
    doc1["uptime"] = millis() / 1000;
    doc1["free_heap"] = rp2040.getFreeHeap();
    
    String json1;
    serializeJson(doc1, json1);
    
    if (!mqttClient.publish(topic_pm1.c_str(), json1.c_str())) {
        allPublishSuccess = false;
        Serial.println("PM1.0 publish failed");
    }
    
    delay(100);
    
    // 發布 PM2.5 數據
    DynamicJsonDocument doc25(512);
    doc25["device_id"] = config.device_id;
    doc25["location"] = "factory";
    doc25["sensor_type"] = "pm25";
    doc25["pm25_standard"] = avgData.pm25_standard;
    doc25["pm25_env"] = avgData.pm25_env;
    doc25["particles_25um"] = avgData.particles_25um;
    doc25["sample_count"] = pmAverage.sample_count;
    doc25["wifi_rssi"] = WiFi.RSSI();
    doc25["uptime"] = millis() / 1000;
    doc25["free_heap"] = rp2040.getFreeHeap();
    
    String json25;
    serializeJson(doc25, json25);
    
    if (!mqttClient.publish(topic_pm25.c_str(), json25.c_str())) {
        allPublishSuccess = false;
        Serial.println("PM2.5 publish failed");
    }
    
    delay(100);
    
    // 發布 PM10 數據
    DynamicJsonDocument doc10(512);
    doc10["device_id"] = config.device_id;
    doc10["location"] = "factory";
    doc10["sensor_type"] = "pm10";
    doc10["pm100_standard"] = avgData.pm100_standard;
    doc10["pm100_env"] = avgData.pm100_env;
    doc10["particles_50um"] = avgData.particles_50um;
    doc10["particles_100um"] = avgData.particles_100um;
    doc10["sample_count"] = pmAverage.sample_count;
    doc10["wifi_rssi"] = WiFi.RSSI();
    doc10["uptime"] = millis() / 1000;
    doc10["free_heap"] = rp2040.getFreeHeap();
    
    String json10;
    serializeJson(doc10, json10);
    
    if (!mqttClient.publish(topic_pm10.c_str(), json10.c_str())) {
        allPublishSuccess = false;
        Serial.println("PM10 publish failed");
    }
    
    if (allPublishSuccess) {
        msg_count++;
        publishFailCount = 0;
        Serial.printf("PM data published (#%d) - %d samples averaged\n", msg_count, pmAverage.sample_count);
        Serial.printf("  PM1.0: %d μg/m³, PM2.5: %d μg/m³, PM10: %d μg/m³\n", 
                     avgData.pm10_standard, avgData.pm25_standard, avgData.pm100_standard);
        
        digitalWrite(LED_PIN, LOW);
        delay(50);
        digitalWrite(LED_PIN, HIGH);
        
        resetAverage();
        
    } else {
        publishFailCount++;
        Serial.printf("PM data publish failed (fail count: %d)\n", publishFailCount);
        
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
    
    Serial.println("=== Pico 2W PMSA003i MQTT Sensor ===");
    Serial.println("Initializing...");
    
    // 初始化 PMSA003i
    initPMSA003i();
    
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
        
        // 讀取 PMSA003i 感測器 (每秒一次)
        unsigned long now = millis();
        if (now - lastSensorRead >= SENSOR_READ_INTERVAL) {
            lastSensorRead = now;
            
            if (readPMSA003i()) {
                Serial.printf("PM Data - PM1.0: %d, PM2.5: %d, PM10: %d (samples: %d)\n", 
                             currentPM.pm10_standard, currentPM.pm25_standard, 
                             currentPM.pm100_standard, pmAverage.sample_count);
            }
        }
        
        // 定時發送 PM 資料 (30秒一次)
        if (now - lastMsg > MSG_INTERVAL) {
            lastMsg = now;
            if (mqttConnected) {
                publishPMData();
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