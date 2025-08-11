#include "mqtt_manager.h"

// Global instance
MQTTManager mqttManager;

// Constructor
MQTTManager::MQTTManager() : mqttClient(wifiClient) {
    // Set default values
    generateClientId();
    mqttClient.setKeepAlive(config.keepAlive);
}

void MQTTManager::generateClientId() {
    if (strlen(config.clientId) == 0) {
        snprintf(config.clientId, sizeof(config.clientId), "esp32-knob-%06X", (uint32_t)ESP.getEfuseMac());
    }
}

void MQTTManager::loadConfig() {
    preferences.begin("mqtt", false);
    
    preferences.getString("server", config.server, sizeof(config.server));
    config.port = preferences.getInt("port", 1883);
    preferences.getString("username", config.username, sizeof(config.username));
    preferences.getString("password", config.password, sizeof(config.password));
    preferences.getString("clientId", config.clientId, sizeof(config.clientId));
    config.useSSL = preferences.getBool("useSSL", false);
    config.keepAlive = preferences.getInt("keepAlive", 60);
    config.reconnectInterval = preferences.getInt("reconnectInt", 5000);
    
    preferences.end();
    
    // Generate client ID if empty
    generateClientId();
    
    Serial.printf("MQTT Config loaded: %s:%d, Client: %s\n", 
                  config.server, config.port, config.clientId);
}

void MQTTManager::saveConfig() {
    preferences.begin("mqtt", false);
    
    preferences.putString("server", config.server);
    preferences.putInt("port", config.port);
    preferences.putString("username", config.username);
    preferences.putString("password", config.password);
    preferences.putString("clientId", config.clientId);
    preferences.putBool("useSSL", config.useSSL);
    preferences.putInt("keepAlive", config.keepAlive);
    preferences.putInt("reconnectInt", config.reconnectInterval);
    
    preferences.end();
    
    Serial.println("MQTT Config saved");
}

void MQTTManager::setConfig(const MQTTConfig& newConfig) {
    config = newConfig;
    generateClientId();
}

MQTTConfig MQTTManager::getConfig() const {
    return config;
}

bool MQTTManager::begin() {
    loadConfig();
    
    // Set server and port
    mqttClient.setServer(config.server, config.port);
    
    // Set our internal message handler
    mqttClient.setCallback([this](char* topic, uint8_t* payload, unsigned int length) {
        this->handleIncomingMessage(topic, payload, length);
    });
    
    // Set keep alive
    mqttClient.setKeepAlive(config.keepAlive);
    
    Serial.printf("MQTT Manager initialized for %s:%d\n", config.server, config.port);
    return true;
}

bool MQTTManager::connect() {
    if (!WiFi.isConnected()) {
        Serial.println("WiFi not connected, cannot connect to MQTT");
        return false;
    }
    
    Serial.printf("Attempting MQTT connection to %s:%d...\n", config.server, config.port);
    
    bool result = false;
    
    // Connect with or without credentials
    if (strlen(config.username) > 0) {
        result = mqttClient.connect(config.clientId, config.username, config.password);
    } else {
        result = mqttClient.connect(config.clientId);
    }
    
    if (result) {
        isConnected = true;
        Serial.printf("MQTT connected as %s\n", config.clientId);
        
        // Subscribe to default topics automatically
        subscribeToDefaultTopics();
        
        // Call connect callback if set
        if (connectCallback) {
            connectCallback();
        }
        
    } else {
        isConnected = false;
        Serial.printf("MQTT connection failed, rc=%d\n", mqttClient.state());
    }
    
    return result;
}

void MQTTManager::disconnect() {
    if (mqttClient.connected()) {
        mqttClient.disconnect();
    }
    isConnected = false;
    
    if (disconnectCallback) {
        disconnectCallback();
    }
    
    Serial.println("MQTT disconnected");
}

bool MQTTManager::reconnect() {
    if (mqttClient.connected()) {
        return true;
    }
    
    unsigned long now = millis();
    if (now - lastReconnectAttempt > config.reconnectInterval) {
        lastReconnectAttempt = now;
        
        Serial.println("Attempting MQTT reconnection...");
        if (connect()) {
            lastReconnectAttempt = 0;
            return true;
        }
    }
    
    return false;
}

void MQTTManager::loop() {
    if (mqttClient.connected()) {
        mqttClient.loop();
    } else {
        if (isConnected) {
            // We were connected but lost connection
            isConnected = false;
            if (disconnectCallback) {
                disconnectCallback();
            }
        }
        
        // Try to reconnect
        reconnect();
    }
}

bool MQTTManager::publish(const char* topic, const char* payload, bool retained) {
    if (!mqttClient.connected()) {
        Serial.println("MQTT not connected, cannot publish");
        return false;
    }
    
    bool result = mqttClient.publish(topic, payload, retained);
    if (result) {
        Serial.printf("MQTT Published [%s]: %s\n", topic, payload);
    } else {
        Serial.printf("MQTT Publish failed [%s]\n", topic);
    }
    
    return result;
}

bool MQTTManager::publish(const char* topic, const String& payload, bool retained) {
    return publish(topic, payload.c_str(), retained);
}

bool MQTTManager::publishJson(const char* topic, const JsonDocument& doc, bool retained) {
    String payload;
    serializeJson(doc, payload);
    return publish(topic, payload, retained);
}

bool MQTTManager::subscribe(const char* topic, uint8_t qos) {
    if (!mqttClient.connected()) {
        Serial.println("MQTT not connected, cannot subscribe");
        return false;
    }
    
    bool result = mqttClient.subscribe(topic, qos);
    if (result) {
        Serial.printf("MQTT Subscribed to: %s\n", topic);
    } else {
        Serial.printf("MQTT Subscribe failed: %s\n", topic);
    }
    
    return result;
}

bool MQTTManager::unsubscribe(const char* topic) {
    if (!mqttClient.connected()) {
        return false;
    }
    
    bool result = mqttClient.unsubscribe(topic);
    if (result) {
        Serial.printf("MQTT Unsubscribed from: %s\n", topic);
    }
    
    return result;
}

void MQTTManager::setMessageCallback(std::function<void(char*, uint8_t*, unsigned int)> callback) {
    messageCallback = callback;
    if (mqttClient.connected()) {
        mqttClient.setCallback(callback);
    }
}

void MQTTManager::setConnectCallback(std::function<void()> callback) {
    connectCallback = callback;
}

void MQTTManager::setDisconnectCallback(std::function<void()> callback) {
    disconnectCallback = callback;
}

bool MQTTManager::connected() const {
    return const_cast<PubSubClient&>(mqttClient).connected();
}

String MQTTManager::getClientId() const {
    return String(config.clientId);
}

String MQTTManager::getServer() const {
    return String(config.server);
}

int MQTTManager::getPort() const {
    return config.port;
}

void MQTTManager::resetConfig() {
    preferences.begin("mqtt", false);
    preferences.clear();
    preferences.end();
    
    // Reset to defaults
    strcpy(config.server, "mqtt.local");
    config.port = 1883;
    config.username[0] = '\0';
    config.password[0] = '\0';
    config.clientId[0] = '\0';
    config.useSSL = false;
    config.keepAlive = 60;
    config.reconnectInterval = 5000;
    
    generateClientId();
    
    Serial.println("MQTT config reset to defaults");
}

void MQTTManager::printConfig() const {
    Serial.println("=== MQTT Configuration ===");
    Serial.printf("Server: %s:%d\n", config.server, config.port);
    Serial.printf("Client ID: %s\n", config.clientId);
    Serial.printf("Username: %s\n", strlen(config.username) > 0 ? config.username : "(none)");
    Serial.printf("Password: %s\n", strlen(config.password) > 0 ? "***" : "(none)");
    Serial.printf("SSL: %s\n", config.useSSL ? "Yes" : "No");
    Serial.printf("Keep Alive: %d seconds\n", config.keepAlive);
    Serial.printf("Reconnect Interval: %d ms\n", config.reconnectInterval);
    Serial.printf("Connected: %s\n", connected() ? "Yes" : "No");
    Serial.println("=========================");
}

String MQTTManager::buildTopic(const char* baseTopic, const char* subtopic) {
    String topic = String(baseTopic);
    if (subtopic && strlen(subtopic) > 0) {
        if (!topic.endsWith("/")) {
            topic += "/";
        }
        topic += subtopic;
    }
    return topic;
}

String MQTTManager::getDeviceTopic(const char* subtopic) {
    return buildTopic(("devices/" + getClientId()).c_str(), subtopic);
}

bool MQTTManager::parseJsonMessage(const char* payload, unsigned int length, JsonDocument& doc) {
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (error) {
        Serial.printf("JSON parse error: %s\n", error.c_str());
        return false;
    }
    
    return true;
}

String MQTTManager::extractStringFromJson(const JsonDocument& doc, const char* key, const String& defaultValue) {
    if (doc[key].is<const char*>()) {
        return String(doc[key].as<const char*>());
    }
    return defaultValue;
}

int MQTTManager::extractIntFromJson(const JsonDocument& doc, const char* key, int defaultValue) {
    if (doc[key].is<int>()) {
        return doc[key].as<int>();
    }
    return defaultValue;
}

float MQTTManager::extractFloatFromJson(const JsonDocument& doc, const char* key, float defaultValue) {
    if (doc[key].is<float>()) {
        return doc[key].as<float>();
    }
    return defaultValue;
}

// ================================
// WiFiManager Integration
// ================================

void MQTTManager::setupWiFiManagerParameters(WiFiManagerCustom& wifiManager) {
    // Clean up any existing parameters
    cleanupWiFiManagerParameters();
    
    // Create new parameters with current config values
    mqtt_server_param = new WiFiManagerParameter("mqtt_server", "MQTT Server", config.server, 64);
    mqtt_port_param = new WiFiManagerParameter("mqtt_port", "MQTT Port", String(config.port).c_str(), 6);
    mqtt_username_param = new WiFiManagerParameter("mqtt_username", "MQTT Username", config.username, 32);
    mqtt_password_param = new WiFiManagerParameter("mqtt_password", "MQTT Password", config.password, 32);
    mqtt_client_id_param = new WiFiManagerParameter("mqtt_client_id", "MQTT Client ID", config.clientId, 32);
    
    // Add parameters to WiFiManager
    wifiManager.addParameter(mqtt_server_param);
    wifiManager.addParameter(mqtt_port_param);
    wifiManager.addParameter(mqtt_username_param);
    wifiManager.addParameter(mqtt_password_param);
    wifiManager.addParameter(mqtt_client_id_param);
}

void MQTTManager::updateConfigFromWiFiManager(WiFiManagerCustom& wifiManager) {
    if (!mqtt_server_param) return;
    
    // Update config from WiFiManager parameters
    if (strlen(mqtt_server_param->getValue()) > 0) {
        strncpy(config.server, mqtt_server_param->getValue(), sizeof(config.server));
    }
    if (strlen(mqtt_port_param->getValue()) > 0) {
        config.port = atoi(mqtt_port_param->getValue());
    }
    if (strlen(mqtt_username_param->getValue()) > 0) {
        strncpy(config.username, mqtt_username_param->getValue(), sizeof(config.username));
    }
    if (strlen(mqtt_password_param->getValue()) > 0) {
        strncpy(config.password, mqtt_password_param->getValue(), sizeof(config.password));
    }
    if (strlen(mqtt_client_id_param->getValue()) > 0) {
        strncpy(config.clientId, mqtt_client_id_param->getValue(), sizeof(config.clientId));
    }
    
    // Save the updated config
    saveConfig();
    
    Serial.println("MQTT config updated from WiFiManager");
}

void MQTTManager::cleanupWiFiManagerParameters() {
    delete mqtt_server_param;
    delete mqtt_port_param;
    delete mqtt_username_param;
    delete mqtt_password_param;
    delete mqtt_client_id_param;
    
    mqtt_server_param = nullptr;
    mqtt_port_param = nullptr;
    mqtt_username_param = nullptr;
    mqtt_password_param = nullptr;
    mqtt_client_id_param = nullptr;
}

bool MQTTManager::setupWithWiFiManager(WiFiManagerCustom& wifiManager) {
    // Setup WiFiManager parameters for MQTT
    setupWiFiManagerParameters(wifiManager);
    
    // Initialize MQTT Manager
    if (!begin()) {
        Serial.println("MQTT manager failed to initialize");
        return false;
    }
    
    // Try to connect to MQTT
    bool mqttConnected = connect();
    if (mqttConnected) {
        Serial.println("MQTT connected successfully!");
    } else {
        Serial.println("MQTT connection failed - will retry automatically");
        Serial.println("Check MQTT server settings in configuration");
    }
    
    return true; // Return true even if MQTT connection failed - it will retry
}

// ================================
// Topic Management
// ================================

void MQTTManager::addSubscriptionTopic(const String& topic) {
    subscriptionTopics.push_back(topic);
}

void MQTTManager::clearSubscriptionTopics() {
    subscriptionTopics.clear();
}

void MQTTManager::subscribeToDefaultTopics() {
    if (!connected()) return;
    
    // Subscribe to device-specific topics
    String deviceTopic = getDeviceTopic("command");
    subscribe(deviceTopic.c_str());
    
    // Subscribe to default topics
    subscribe("energy/+");
    subscribe("weather/+");
    subscribe("house/+");
    
    // Subscribe to any additional topics
    for (const String& topic : subscriptionTopics) {
        subscribe(topic.c_str());
    }
    
    // Publish online status
    String statusTopic = getDeviceTopic("status");
    publish(statusTopic.c_str(), "online", true);
}

// ================================
// Message Handling
// ================================

void MQTTManager::handleIncomingMessage(char* topic, uint8_t* payload, unsigned int length) {
    // Convert payload to string
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    Serial.printf("MQTT Message [%s]: %s\n", topic, message);
    
    // Parse JSON if applicable
    JsonDocument doc;
    if (parseJsonMessage(message, length, doc)) {
        // Handle different topic types
        if (strstr(topic, "/energy/")) {
            if (energyCallback) {
                energyCallback(doc, String(topic));
            } else {
                // Default energy handling
                float power = extractFloatFromJson(doc, "power", 0.0);
                float energy = extractFloatFromJson(doc, "energy", 0.0);
                Serial.printf("Energy data - Power: %.2f W, Energy: %.2f kWh\n", power, energy);
            }
            
        } else if (strstr(topic, "/weather/")) {
            if (weatherCallback) {
                weatherCallback(doc, String(topic));
            } else {
                // Default weather handling
                float temp = extractFloatFromJson(doc, "temperature", 0.0);
                int humidity = extractIntFromJson(doc, "humidity", 0);
                Serial.printf("Weather data - Temp: %.1f°C, Humidity: %d%%\n", temp, humidity);
            }
            
        } else if (strstr(topic, "/house/")) {
            if (houseCallback) {
                houseCallback(doc, String(topic));
            } else {
                // Default house handling
                String room = extractStringFromJson(doc, "room", "unknown");
                String device = extractStringFromJson(doc, "device", "unknown");
                String state = extractStringFromJson(doc, "state", "unknown");
                Serial.printf("House data - Room: %s, Device: %s, State: %s\n", 
                             room.c_str(), device.c_str(), state.c_str());
            }
            
        } else if (strstr(topic, "/command")) {
            // Handle device commands
            String command = extractStringFromJson(doc, "command", "");
            handleDeviceCommand(command, doc);
        }
    } else {
        // Handle non-JSON messages
        Serial.printf("Non-JSON message: %s\n", message);
    }
    
    // Call the original message callback if set
    if (messageCallback) {
        messageCallback(topic, payload, length);
    }
}

void MQTTManager::handleDeviceCommand(const String& command, const JsonDocument& data) {
    Serial.printf("Processing device command: %s\n", command.c_str());
    
    if (command == "reset_wifi") {
        Serial.println("MQTT Command: Reset WiFi - Restarting...");
        ESP.restart();
        
    } else if (command == "restart") {
        Serial.println("MQTT Command: Restart device");
        ESP.restart();
        
    } else if (command == "status") {
        Serial.println("MQTT Command: Publish status");
        publishDeviceStatus();
        
    } else {
        Serial.printf("Unknown command: %s\n", command.c_str());
    }
}

void MQTTManager::publishDeviceStatus() {
    if (!connected()) return;
    
    JsonDocument statusDoc;
    statusDoc["uptime"] = millis();
    statusDoc["free_heap"] = ESP.getFreeHeap();
    statusDoc["wifi_connected"] = WiFi.isConnected();
    statusDoc["wifi_rssi"] = WiFi.RSSI();
    statusDoc["mqtt_connected"] = connected();
    statusDoc["client_id"] = getClientId();
    
    String statusTopic = getDeviceTopic("status");
    publishJson(statusTopic.c_str(), statusDoc);
    
    Serial.println("Device status published");
}

// ================================
// Callback Setters
// ================================

void MQTTManager::setEnergyCallback(std::function<void(const JsonDocument&, const String&)> callback) {
    energyCallback = callback;
}

void MQTTManager::setWeatherCallback(std::function<void(const JsonDocument&, const String&)> callback) {
    weatherCallback = callback;
}

void MQTTManager::setHouseCallback(std::function<void(const JsonDocument&, const String&)> callback) {
    houseCallback = callback;
}
