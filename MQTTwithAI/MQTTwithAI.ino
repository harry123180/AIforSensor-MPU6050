#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <EdgeAI_inferencing.h>
#include <Wire.h>

// 記憶體優化
#define EIDSP_QUANTIZE_FILTERBANK   0

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

// MPU6050設定
const int MPU = 0x68;

// 定時器參數
const unsigned long STATUS_INTERVAL = 300000;  // 5分鐘發送狀態
const unsigned long MIN_PUBLISH_INTERVAL = 1000;  // 最小發布間隔 1 秒

// AI 推論參數
static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
const int SAMPLE_INTERVAL_MS = 1;  // 取樣間隔 1ms（1000Hz）/ Sampling interval 1 ms (1000 Hz)
unsigned long last_sample_time = 0;
int feature_ix = 0;

// ==================== 資料結構 / Data structures ====================
// 設定儲存結構 / Configuration storage struct
struct Config {
    char wifi_ssid[32];
    char wifi_password[64];
    char mqtt_server[64];
    int mqtt_port;
    char device_id[32];
    bool valid;
};

// 推論結果緩衝 / Inference result buffer
struct InferenceData {
    float case1_confidence;
    float case2_confidence;
    const char* predicted_class;
    float max_confidence;
    bool valid;
    unsigned long timestamp;
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
unsigned long lastStatus = 0;
unsigned long lastPublishAttempt = 0;
int msg_count = 0;
int publishFailCount = 0;
const int MAX_PUBLISH_FAILS = 3;

// AI 推論資料
InferenceData currentInference;

// MQTT 主題定義
String topic_inference = "sensors/factory_team01/ai/inference";
String topic_status = "sensors/factory_team01/status";
String topic_command;  // 指令主題於 initConfig 初始化 / Command topic set in initConfig

// ==================== MPU6050 & AI 功能 ====================
/**
 * @brief      取得原始資料的回調函式
 */
int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

void initMPU6050() {
    Serial.print("Initializing MPU6050...");
    Wire1.setSDA(14);
    Wire1.setSCL(15);
    Wire1.begin();
    Wire1.setClock(400000);
    
    // 檢查MPU6050
    Wire1.beginTransmission(MPU);
    if (Wire1.endTransmission() != 0) {
        Serial.println(" FAILED!");
        Serial.println("Please check MPU6050 connection.");
        Serial.println("Will continue without MPU6050 - using dummy data for testing");
        return;
    }
    
    // 喚醒MPU6050
    writeRegister(0x6B, 0x00);
    delay(100);
    
    // 設定加速度計範圍 ±2g
    writeRegister(0x1C, 0x00);
    delay(10);
    
    // 設定陀螺儀範圍 ±250°/s
    writeRegister(0x1B, 0x00);
    delay(10);
    
    // 設定採樣率 1kHz
    writeRegister(0x19, 0x00);
    delay(10);
    
    // 關閉DLPF
    writeRegister(0x1A, 0x00);
    delay(10);
    
    Serial.println(" OK");
    
    // 顯示模型資訊
    Serial.println("\nAI Model information:");
    Serial.print("  Number of classes: ");
    Serial.println(sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));
    Serial.print("  Classes: ");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        Serial.print(ei_classifier_inferencing_categories[ix]);
        if (ix != EI_CLASSIFIER_LABEL_COUNT - 1) {
            Serial.print(", ");
        }
    }
    Serial.println();
    
    Serial.print("  Sample length: ");
    Serial.print(EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE / 3);
    Serial.println(" samples");
    
    Serial.print("  DSP input size: ");
    Serial.println(EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    
    // 初始化推論資料結構
    memset(&currentInference, 0, sizeof(currentInference));
    currentInference.valid = false;
    
    Serial.println("MPU6050 and AI model initialized successfully");
}

void writeRegister(uint8_t reg, uint8_t value) {
    Wire1.beginTransmission(MPU);
    Wire1.write(reg);
    Wire1.write(value);
    Wire1.endTransmission();
}

void readAcceleration(float &accX, float &accY, float &accZ) {
    Wire1.beginTransmission(MPU);
    Wire1.write(0x3B); // ACCEL_XOUT_H
    Wire1.endTransmission(false);
    Wire1.requestFrom(MPU, 6, true);
    
    if (Wire1.available() >= 6) {
        int16_t rawX = (Wire1.read() << 8) | Wire1.read();
        int16_t rawY = (Wire1.read() << 8) | Wire1.read();
        int16_t rawZ = (Wire1.read() << 8) | Wire1.read();
        
        // 轉換為g值 (±2g範圍)
        accX = rawX / 16384.0;
        accY = rawY / 16384.0;
        accZ = rawZ / 16384.0;
    } else {
        // 如果讀取失敗，返回預設值
        accX = 0.0;
        accY = 0.0;
        accZ = 1.0;
    }
}

/**
 * @brief 收集資料並執行AI推論
 */
bool performInference() {
    Serial.println("Collecting data for inference...");
    
    // 階段1: 收集資料
    feature_ix = 0;
    last_sample_time = millis();
    unsigned long start_time = millis();
    
    while (feature_ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        unsigned long current_time = millis();
        
        // 每1ms採樣一次
        if (current_time - last_sample_time >= SAMPLE_INTERVAL_MS) {
            last_sample_time = current_time;
            
            // 讀取加速度
            float accX, accY, accZ;
            readAcceleration(accX, accY, accZ);
            
            // 儲存到features陣列
            if (feature_ix + 2 < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
                features[feature_ix++] = accX;
                features[feature_ix++] = accY;
                features[feature_ix++] = accZ;
            }
        }
        
        // 防止無限迴圈
        if (millis() - start_time > 2000) {
            Serial.println("Data collection timeout!");
            return false;
        }
    }
    
    Serial.println("Data collection complete, running inference...");
    
    // 階段2: 執行推論
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    signal.get_data = &raw_feature_get_data;
    
    ei_impulse_result_t result = { 0 };
    
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
    
    if (res != EI_IMPULSE_OK) {
        Serial.print("Failed to run classifier: ");
        Serial.println(res);
        return false;
    }
    
    // 階段3: 處理結果
    currentInference.case1_confidence = 0.0;
    currentInference.case2_confidence = 0.0;
    currentInference.max_confidence = 0.0;
    currentInference.predicted_class = "unknown";
    
    // 找出各類別的信心度
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        const char* label = result.classification[ix].label;
        float confidence = result.classification[ix].value;
        
        if (strcmp(label, "case1") == 0) {
            currentInference.case1_confidence = confidence;
        } else if (strcmp(label, "case2") == 0) {
            currentInference.case2_confidence = confidence;
        }
        
        if (confidence > currentInference.max_confidence) {
            currentInference.max_confidence = confidence;
            currentInference.predicted_class = label;
        }
    }
    
    currentInference.valid = true;
    currentInference.timestamp = millis();
    
    Serial.printf("Inference complete - Predicted: %s (%.1f%%)\n", 
                 currentInference.predicted_class, 
                 currentInference.max_confidence * 100.0);
    Serial.printf("  case1: %.1f%%, case2: %.1f%%\n", 
                 currentInference.case1_confidence * 100.0,
                 currentInference.case2_confidence * 100.0);
    
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
        snprintf(config.device_id, sizeof(config.device_id), "mpu6050ai_%s", mac.substring(6).c_str());
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
    Serial.printf("Topics - Inference: %s\n", topic_inference.c_str());
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
    
    WiFi.disconnect(true);
    WiFi.softAPdisconnect(true);
    delay(500);
    
    WiFi.mode(WIFI_STA);
    delay(500);
    
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
    
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true);
    delay(500);
    
    WiFi.mode(WIFI_STA);
    delay(1000);
    
    if (!connectWiFi()) {
        Serial.println("Failed to connect in normal mode");
        startConfigMode();
        return;
    }
    
    IPAddress localIP = WiFi.localIP();
    Serial.printf("Connected IP: %s\n", localIP.toString().c_str());
    
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
        server.send(200, "text/plain", "Pico 2 W MPU6050 AI Configuration Mode");
    });
    server.onNotFound([]() {
        server.send(404, "text/plain", "Not Found");
    });
}

void setupStatusWebServer() {
    server.on("/", handleStatus);
    server.on("/reset", handleReset);
    server.on("/mqtt", handleMQTTStatus);
    server.on("/inference", handleInferenceData);
    server.onNotFound([]() {
        server.send(404, "text/plain", "Not Found");
    });
}

void handleConfigRoot() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>Pico 2W MPU6050 AI Setup</title>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<style>body{font-family:Arial;padding:20px;max-width:600px;margin:0 auto;background:linear-gradient(135deg,#fff,#f0f8ff);}";
    html += "input{width:100%;padding:8px;margin:5px 0;box-sizing:border-box;border:1px solid #ddd;border-radius:4px;}";
    html += "button{background:linear-gradient(135deg,#4169e1,#1e90ff);color:white;padding:10px;border:none;cursor:pointer;width:100%;border-radius:4px;}";
    html += ".network{padding:8px;border:1px solid #ddd;margin:2px;cursor:pointer;border-radius:4px;background:#fff;}";
    html += ".network:hover{background:#f0f8ff;}</style></head><body>";
    
    html += "<h1>Pico 2W MPU6050 AI Setup</h1>";
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
    
    ssid.toCharArray(config.wifi_ssid, sizeof(config.wifi_ssid));
    password.toCharArray(config.wifi_password, sizeof(config.wifi_password));
    mqtt_server.toCharArray(config.mqtt_server, sizeof(config.mqtt_server));
    config.mqtt_port = mqtt_port.toInt();
    
    if (config.mqtt_port == 0) config.mqtt_port = DEFAULT_MQTT_PORT;
    
    saveConfig();
    Serial.println("Configuration saved - restarting device");
    delay(3000);
    rp2040.restart();
}

void handleStatus() {
    String html = "<!DOCTYPE html><html><head><title>Pico 2W MPU6050 AI Status</title>";
    html += "<meta http-equiv='refresh' content='10'>";
    html += "<style>body{font-family:Arial;padding:20px;max-width:800px;margin:0 auto;background:linear-gradient(135deg,#fff,#f0f8ff);}";
    html += ".status{background:#f8f9fa;padding:15px;margin:10px 0;border-radius:8px;border-left:4px solid #007bff;}";
    html += ".ok{border-left-color:#28a745;background:#d4edda;} .error{border-left-color:#dc3545;background:#f8d7da;}</style></head><body>";
    
    html += "<h1>Pico 2W MPU6050 AI Status</h1>";
    
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
    
    html += "<div class='status " + String(currentInference.valid ? "ok" : "error") + "'>";
    html += "<h3>AI Inference Data</h3>";
    if (currentInference.valid) {
        html += "<p><strong>Predicted Class:</strong> " + String(currentInference.predicted_class) + "</p>";
        html += "<p><strong>Confidence:</strong> " + String(currentInference.max_confidence * 100.0, 1) + "%</p>";
        html += "<p><strong>case1 Confidence:</strong> " + String(currentInference.case1_confidence * 100.0, 1) + "%</p>";
        html += "<p><strong>case2 Confidence:</strong> " + String(currentInference.case2_confidence * 100.0, 1) + "%</p>";
        html += "<p><strong>Last Inference:</strong> " + String((millis() - currentInference.timestamp)/1000) + " seconds ago</p>";
    } else {
        html += "<p><strong>Status:</strong> No valid inference data</p>";
    }
    html += "<p><strong>Uptime:</strong> " + String(millis() / 1000) + " seconds</p>";
    html += "<p><strong>Free Memory:</strong> " + String(rp2040.getFreeHeap()) + " bytes</p>";
    html += "</div>";
    
    html += "<p><a href='/reset'><button style='background:#dc3545;color:white;padding:10px;border:none;border-radius:4px;'>Reset Settings</button></a></p>";
    html += "<p><a href='/mqtt'><button style='background:#007bff;color:white;padding:10px;border:none;border-radius:4px;'>MQTT Details</button></a></p>";
    html += "<p><a href='/inference'><button style='background:#28a745;color:white;padding:10px;border:none;border-radius:4px;'>Inference JSON</button></a></p>";
    
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
    doc["topics"]["inference"] = topic_inference;
    doc["topics"]["status"] = topic_status;
    doc["topics"]["command"] = topic_command;
    doc["message_count"] = msg_count;
    
    String jsonString;
    serializeJson(doc, jsonString);
    server.send(200, "application/json", jsonString);
}

void handleInferenceData() {
    DynamicJsonDocument doc(512);
    doc["device_id"] = config.device_id;
    doc["predicted_class"] = currentInference.predicted_class;
    doc["max_confidence"] = currentInference.max_confidence;
    doc["case1_confidence"] = currentInference.case1_confidence;
    doc["case2_confidence"] = currentInference.case2_confidence;
    doc["inference_valid"] = currentInference.valid;
    doc["last_inference"] = currentInference.timestamp;
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
            } else if (strcmp(command, "get_inference") == 0) {
                publishInferenceData();
            } else if (strcmp(command, "run_inference") == 0) {
                if (performInference()) {
                    publishInferenceData();
                }
            }
        }
    }
}

void publishInferenceData() {
    if (!mqttConnected || !mqttClient.connected()) {
        Serial.println("MQTT not connected for inference data");
        return;
    }
    
    unsigned long now = millis();
    if (now - lastPublishAttempt < MIN_PUBLISH_INTERVAL) {
        return;
    }
    lastPublishAttempt = now;
    
    if (!currentInference.valid) {
        Serial.println("No valid inference data to publish");
        return;
    }
    
    DynamicJsonDocument doc(1024);
    doc["device_id"] = config.device_id;
    doc["location"] = "factory";
    doc["sensor_type"] = "ai_inference";
    doc["predicted_class"] = currentInference.predicted_class;
    doc["predicted_class_id"] = (strcmp(currentInference.predicted_class, "case1") == 0) ? 1 : 2;
    doc["max_confidence"] = round(currentInference.max_confidence * 1000) / 1000.0;
    doc["classifications"]["case1"] = round(currentInference.case1_confidence * 1000) / 1000.0;
    doc["classifications"]["case2"] = round(currentInference.case2_confidence * 1000) / 1000.0;
    doc["case1_confidence"] = round(currentInference.case1_confidence * 1000) / 1000.0;
    doc["case2_confidence"] = round(currentInference.case2_confidence * 1000) / 1000.0;
    doc["inference_timestamp"] = currentInference.timestamp;
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["uptime"] = millis() / 1000;
    doc["free_heap"] = rp2040.getFreeHeap();
    doc["timestamp"] = millis();
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    if (mqttClient.publish(topic_inference.c_str(), jsonString.c_str())) {
        msg_count++;
        publishFailCount = 0;
        Serial.printf("Inference data published (#%d) - %s: %.1f%%\n", 
                     msg_count, currentInference.predicted_class, 
                     currentInference.max_confidence * 100.0);
        
        digitalWrite(LED_PIN, LOW);
        delay(50);
        digitalWrite(LED_PIN, HIGH);
        
    } else {
        publishFailCount++;
        Serial.printf("Inference data publish failed (fail count: %d)\n", publishFailCount);
        
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
// 初始化 Wi-Fi、MQTT 與 IMU / Initialize Wi-Fi, MQTT, and IMU
void setup() {
    pinMode(LED_PIN, OUTPUT);
    pinMode(RESET_PIN, INPUT_PULLUP);
    
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== Pico 2W MPU6050 AI MQTT Sensor ===");
    Serial.println("Initializing...");
    
    // 初始化 MPU6050 和 AI
    initMPU6050();
    
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
    
    Serial.println("Setup complete - starting continuous inference");
}

// 主迴圈：處理配置、取樣與傳輸 / Main loop: handle config, sampling, and publish
void loop() {
    server.handleClient();
    
    // 執行AI推論 (在任何模式下都執行)
    static unsigned long lastInference = 0;
    unsigned long now = millis();
    
    // 每1.5秒執行一次推論 (考慮到收集數據需要約1秒)
    if (now - lastInference > 1500) {
        lastInference = now;
        Serial.println("\n=== Starting AI Inference ===");
        
        if (performInference()) {
            if (!configMode && mqttConnected) {
                publishInferenceData();
            }
        } else {
            Serial.println("Inference failed - will retry next cycle");
        }
    }
    
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