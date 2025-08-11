#include "settings_app.h"
#include "../wifi_manager.h"
#include "../mqtt_manager.h"
#include "../command_handler.h"

extern WiFiManagerCustom wifiManager;
extern MQTTManager mqttManager;
extern CommandHandler commandHandler;

bool SettingsApp::init() {
    if (initialized) return true;
    
    Serial.println("Initializing Settings App...");
    
    // Initialize state
    current_option = 0;
    last_action = "";
    action_time = 0;
    
    // Create the screen
    screen = createScreen();
    if (!screen) {
        Serial.println("Failed to create Settings screen");
        return false;
    }
    
    initialized = true;
    Serial.println("Settings App initialized successfully");
    return true;
}

void SettingsApp::deinit() {
    if (!initialized) return;
    
    if (screen) {
        lv_obj_del(screen);
        screen = nullptr;
    }
    
    status_label = nullptr;
    wifi_label = nullptr;
    mqtt_label = nullptr;
    action_label = nullptr;
    
    initialized = false;
    active = false;
    Serial.println("Settings App deinitialized");
}

lv_obj_t* SettingsApp::createScreen() {
    // Create main screen container
    lv_obj_t* scr = lv_obj_create(NULL);
    if (!scr) return nullptr;
    
    // Set the screen as the current context for helper methods
    screen = scr;
    setBackgroundColor(0x112211);
    
    // Create title
    createTitle("⚙️ SETTINGS", 0x90EE90);
    
    // Create status labels
    status_label = createLabel("System Status", 0xCCCCCC, LV_ALIGN_CENTER, 0, -60);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
    
    wifi_label = createLabel("📶 WiFi: Checking...", 0xFFD700, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_14, 0);
    
    mqtt_label = createLabel("📡 MQTT: Checking...", 0x87CEEB, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_text_font(mqtt_label, &lv_font_montserrat_14, 0);
    
    // Create action display
    action_label = createLabel("Use encoder to navigate", 0x90EE90, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_text_font(action_label, &lv_font_montserrat_14, 0);
    
    updateDisplay();
    return scr;
}

void SettingsApp::onEnter() {
    if (!initialized) return;
    
    active = true;
    current_option = 0;
    last_update = 0; // Force immediate update
    updateDisplay();
    
    Serial.println("Settings App entered");
}

void SettingsApp::onExit() {
    active = false;
    Serial.println("Settings App exited");
}

void SettingsApp::update() {
    if (!active || !initialized) return;
    
    unsigned long now = millis();
    if (now - last_update >= UPDATE_INTERVAL) {
        updateConnectionStatus();
        updateDisplay();
        last_update = now;
        
        // Clear action feedback after 3 seconds
        if (!last_action.isEmpty() && (now - action_time >= 3000)) {
            last_action = "";
            updateDisplay();
        }
    }
}

void SettingsApp::onMQTTMessage(const String& topic, const String& payload) {
    if (!active) return;
    
    // Settings app doesn't typically respond to MQTT messages
    // but could be extended for remote configuration
}

void SettingsApp::executeWiFiReset() {
    Serial.println("Settings: Executing WiFi Reset");
    
    wifiManager.reset();
    showActionFeedback("WiFi Reset Complete");
}

void SettingsApp::executeMQTTReset() {
    Serial.println("Settings: Executing MQTT Reset");
    
    mqttManager.disconnect();
    // In a full implementation, this would clear MQTT settings
    showActionFeedback("MQTT Reset Complete");
}

void SettingsApp::executeFactoryReset() {
    Serial.println("Settings: Executing Factory Reset");
    
    // Factory reset would clear all settings and restart
    showActionFeedback("Factory Reset - Restarting...");
}

void SettingsApp::executeRestart() {
    Serial.println("Settings: Executing Restart");
    
    showActionFeedback("Restarting System...");
    delay(1000);
    ESP.restart();
}

void SettingsApp::executeExit() {
    Serial.println("Settings: Executing Exit");
    
    showActionFeedback("Exit Failed");
}

void SettingsApp::updateConnectionStatus() {
    // Update WiFi status
    wifi_connected = wifiManager.isWiFiConnected();
    current_ssid = wifiManager.getSSID();
    
    // Update MQTT status  
    mqtt_connected = mqttManager.connected();
}

void SettingsApp::updateDisplay() {
    if (!wifi_label || !mqtt_label || !action_label) return;
    
    // Update WiFi status
    String wifi_text = "📶 WiFi: ";
    if (wifi_connected) {
        wifi_text += "Connected";
        if (!current_ssid.isEmpty()) {
            wifi_text += " (" + current_ssid + ")";
        }
        lv_obj_set_style_text_color(wifi_label, lv_color_hex(0x90EE90), 0); // Green
    } else {
        wifi_text += "Disconnected";
        lv_obj_set_style_text_color(wifi_label, lv_color_hex(0xFF6B35), 0); // Orange
    }
    lv_label_set_text(wifi_label, wifi_text.c_str());
    
    // Update MQTT status
    String mqtt_text = "📡 MQTT: ";
    mqtt_text += mqtt_connected ? "Connected" : "Disconnected";
    lv_label_set_text(mqtt_label, mqtt_text.c_str());
    
    uint32_t mqtt_color = mqtt_connected ? 0x90EE90 : 0xFF6B35; // Green or Orange
    lv_obj_set_style_text_color(mqtt_label, lv_color_hex(mqtt_color), 0);
    
    highlightCurrentOption();
}

void SettingsApp::highlightCurrentOption() {
    if (!action_label) return;
    
    String display_text;
    if (!last_action.isEmpty()) {
        display_text = last_action;
    } else {
        display_text = "► " + String(options[current_option]);
    }
    
    lv_label_set_text(action_label, display_text.c_str());
    
    // Color based on option type
    String color_str = getOptionColor(current_option);
    uint32_t color = 0x90EE90; // Default green
    
    if (color_str == "orange") color = 0xFF6B35;
    else if (color_str == "red") color = 0xFF4444;
    else if (color_str == "blue") color = 0x87CEEB;
    
    lv_obj_set_style_text_color(action_label, lv_color_hex(color), 0);
}

String SettingsApp::getOptionColor(int option_index) {
    switch (option_index) {
        case 0: return "orange";  // WiFi Reset
        case 1: return "orange";  // MQTT Reset
        case 2: return "red";     // Factory Reset
        case 3: return "orange";  // Restart
        case 4: return "blue";    // Exit
        default: return "green";
    }
}

void SettingsApp::showActionFeedback(const String& action) {
    last_action = action;
    action_time = millis();
    
    if (action_label) {
        lv_label_set_text(action_label, action.c_str());
        lv_obj_set_style_text_color(action_label, lv_color_hex(0xFFD700), 0); // Yellow for feedback
    }
    
    Serial.printf("Settings: %s\n", action.c_str());
}
