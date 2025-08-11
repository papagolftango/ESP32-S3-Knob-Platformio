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
    
    // Set callback if provided
    if (messageCallback) {
        mqttClient.setCallback(messageCallback);
    }
    
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
        
        // Call connect callback if set
        if (connectCallback) {
            connectCallback();
        }
        
        // Subscribe to device topics
        String deviceTopic = getDeviceTopic("+");
        subscribe(deviceTopic.c_str());
        
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
