#include "wifi_manager.h"

// Global instance
WiFiManagerCustom wifiManager;

// Constructor
WiFiManagerCustom::WiFiManagerCustom() {
    // Set default configuration
    wm.setDebugOutput(true);
    wm.setMinimumSignalQuality(8);
    wm.setRemoveDuplicateAPs(true);
}

bool WiFiManagerCustom::begin(const char* apName, const char* apPassword) {
    Serial.println("Starting WiFi Manager...");
    
    // Set callbacks
    if (onConfigModeCallback) {
        wm.setAPCallback(onConfigModeCallback);
    }
    
    if (onConfigSavedCallback) {
        wm.setSaveParamsCallback(onConfigSavedCallback);
    }
    
    // Attempt to connect
    return autoConnect(apName, apPassword);
}

bool WiFiManagerCustom::autoConnect(const char* apName, const char* apPassword) {
    Serial.printf("Attempting to connect to WiFi or start AP: %s\n", apName);
    
    bool result = wm.autoConnect(apName, apPassword);
    
    if (result) {
        isConnected = true;
        Serial.println("WiFi connected successfully!");
        Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    } else {
        isConnected = false;
        Serial.println("Failed to connect to WiFi or user cancelled");
    }
    
    return result;
}

void WiFiManagerCustom::reset() {
    Serial.println("Resetting WiFi settings...");
    wm.resetSettings();
    isConnected = false;
}

void WiFiManagerCustom::disconnect() {
    WiFi.disconnect();
    isConnected = false;
    Serial.println("WiFi disconnected");
}

void WiFiManagerCustom::setConfigPortalTimeout(int timeout) {
    wm.setConfigPortalTimeout(timeout);
}

void WiFiManagerCustom::setAPCallback(std::function<void(WiFiManager*)> func) {
    onConfigModeCallback = func;
    wm.setAPCallback(func);
}

void WiFiManagerCustom::setSaveConfigCallback(std::function<void()> func) {
    onConfigSavedCallback = func;
    wm.setSaveParamsCallback(func);
}

void WiFiManagerCustom::addParameter(WiFiManagerParameter* parameter) {
    wm.addParameter(parameter);
}

void WiFiManagerCustom::setCustomHeadElement(const char* element) {
    wm.setCustomHeadElement(element);
}

bool WiFiManagerCustom::isWiFiConnected() {
    isConnected = (WiFi.status() == WL_CONNECTED);
    return isConnected;
}

String WiFiManagerCustom::getSSID() {
    return WiFi.SSID();
}

String WiFiManagerCustom::getIP() {
    return WiFi.localIP().toString();
}

int WiFiManagerCustom::getRSSI() {
    return WiFi.RSSI();
}

void WiFiManagerCustom::startConfigPortal(const char* apName, const char* apPassword) {
    Serial.printf("Starting config portal: %s\n", apName);
    wm.startConfigPortal(apName, apPassword);
}

void WiFiManagerCustom::stopConfigPortal() {
    wm.stopConfigPortal();
}

void WiFiManagerCustom::setDebugOutput(bool debug) {
    wm.setDebugOutput(debug);
}

void WiFiManagerCustom::setMinimumSignalQuality(int quality) {
    wm.setMinimumSignalQuality(quality);
}

void WiFiManagerCustom::setRemoveDuplicateAPs(bool removeDuplicates) {
    wm.setRemoveDuplicateAPs(removeDuplicates);
}
