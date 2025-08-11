#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>

// WiFi Manager class for handling network connections
class WiFiManagerCustom {
private:
    WiFiManager wm;
    bool isConnected = false;
    
    // Callback functions
    std::function<void()> onConfigSavedCallback = nullptr;
    std::function<void(WiFiManager*)> onConfigModeCallback = nullptr;
    
public:
    // Constructor
    WiFiManagerCustom();
    
    // Main setup and connection methods
    bool begin(const char* apName = "ESP32-Knob-Setup", const char* apPassword = "");
    bool autoConnect(const char* apName = "ESP32-Knob-Setup", const char* apPassword = "");
    void reset();
    void disconnect();
    
    // Configuration methods
    void setConfigPortalTimeout(int timeout);
    void setAPCallback(std::function<void(WiFiManager*)> func);
    void setSaveConfigCallback(std::function<void()> func);
    void addParameter(WiFiManagerParameter* parameter);
    void setCustomHeadElement(const char* element);
    
    // Status methods
    bool isWiFiConnected();
    String getSSID();
    String getIP();
    int getRSSI();
    
    // Utility methods
    void startConfigPortal(const char* apName = "ESP32-Knob-Setup", const char* apPassword = "");
    void stopConfigPortal();
    
    // Advanced configuration
    void setDebugOutput(bool debug);
    void setMinimumSignalQuality(int quality = 8);
    void setRemoveDuplicateAPs(bool removeDuplicates = true);
};

// Global instance
extern WiFiManagerCustom wifiManager;

#endif // WIFI_MANAGER_H
