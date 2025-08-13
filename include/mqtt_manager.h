#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFiManager.h>
#include <vector>
#include <functional>
#include "wifi_manager.h"

// Forward declaration

// MQTT Configuration structure
struct MQTTConfig {
    char server[64] = "mqtt.local";
    int port = 1883;
    char username[32] = "";
    char password[32] = "";
    char clientId[32] = "";
    bool useSSL = false;
    int keepAlive = 60;
    int reconnectInterval = 5000; // milliseconds
};

// MQTT Manager class for handling MQTT connections and messaging
class MQTTManager {
private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    MQTTConfig config;
    Preferences preferences;
    
    unsigned long lastReconnectAttempt = 0;
    bool isConnected = false;
    
    // Topic management
    std::vector<String> subscriptionTopics;
    WiFiManagerParameter* mqtt_server_param = nullptr;
    WiFiManagerParameter* mqtt_port_param = nullptr;
    WiFiManagerParameter* mqtt_username_param = nullptr;
    WiFiManagerParameter* mqtt_password_param = nullptr;
    WiFiManagerParameter* mqtt_client_id_param = nullptr;
    
    // Callback functions
    std::function<void(char*, uint8_t*, unsigned int)> messageCallback = nullptr;
    std::function<void()> connectCallback = nullptr;
    std::function<void()> disconnectCallback = nullptr;
    
    // Message type callbacks
    std::function<void(const JsonDocument&, const String&)> energyCallback = nullptr;
    std::function<void(const JsonDocument&, const String&)> weatherCallback = nullptr;
    std::function<void(const JsonDocument&, const String&)> houseCallback = nullptr;
    
    // Internal methods
    void generateClientId();
    void cleanupWiFiManagerParameters();
    
public:
    // Constructor
    MQTTManager();
    
    // Configuration methods
    void loadConfig();
    void saveConfig();
    void setConfig(const MQTTConfig& newConfig);
    MQTTConfig getConfig() const;
    
    // WiFiManager integration
    void setupWiFiManagerParameters(WiFiManagerCustom& wifiManager);
    void updateConfigFromWiFiManager(WiFiManagerCustom& wifiManager);
    bool setupWithWiFiManager(WiFiManagerCustom& wifiManager);
    
    // Connection methods
    bool begin();
    bool connect();
    void disconnect();
    bool reconnect();
    void loop();
    
    // Publishing methods
    bool publish(const char* topic, const char* payload, bool retained = false);
    bool publish(const char* topic, const String& payload, bool retained = false);
    bool publishJson(const char* topic, const JsonDocument& doc, bool retained = false);
    
    // Subscription methods
    bool subscribe(const char* topic, uint8_t qos = 0);
    bool unsubscribe(const char* topic);
    void subscribeToDefaultTopics();
    
    // Topic management
    void addSubscriptionTopic(const String& topic);
    void clearSubscriptionTopics();
    
    // Message handling
    void handleIncomingMessage(char* topic, uint8_t* payload, unsigned int length);
    
    // Device commands
    void handleDeviceCommand(const String& command, const JsonDocument& data);
    void publishDeviceStatus();
    
    // Callback methods
    void setMessageCallback(std::function<void(char*, uint8_t*, unsigned int)> callback);
    void setConnectCallback(std::function<void()> callback);
    void setDisconnectCallback(std::function<void()> callback);
    
    // Data type callbacks
    void setEnergyCallback(std::function<void(const JsonDocument&, const String&)> callback);
    void setWeatherCallback(std::function<void(const JsonDocument&, const String&)> callback);
    void setHouseCallback(std::function<void(const JsonDocument&, const String&)> callback);
    
    // Status methods
    bool connected() const;
    String getClientId() const;
    String getServer() const;
    int getPort() const;
    
    // Utility methods
    void resetConfig();
    void printConfig() const;
    
    // Topic helpers
    String buildTopic(const char* baseTopic, const char* subtopic = nullptr);
    String getDeviceTopic(const char* subtopic = nullptr);
    
    // Message parsing helpers
    bool parseJsonMessage(const char* payload, unsigned int length, JsonDocument& doc);
    String extractStringFromJson(const JsonDocument& doc, const char* key, const String& defaultValue = "");
    int extractIntFromJson(const JsonDocument& doc, const char* key, int defaultValue = 0);
    float extractFloatFromJson(const JsonDocument& doc, const char* key, float defaultValue = 0.0);
};

// Global instance
extern MQTTManager mqttManager;

#endif // MQTT_MANAGER_H
