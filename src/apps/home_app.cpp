#include "home_app.h"

bool HomeApp::init() {
    if (initialized) return true;
    
    Serial.println("Initializing Home App...");
    
    // Create the screen
    screen = createScreen();
    if (!screen) {
        Serial.println("Failed to create Home screen");
        return false;
    }
    
    initialized = true;
    Serial.println("Home App initialized successfully");
    return true;
}

void HomeApp::deinit() {
    if (!initialized) return;
    
    if (screen) {
        lv_obj_del(screen);
        screen = nullptr;
    }
    
    wifi_status_label = nullptr;
    mqtt_status_label = nullptr;
    nav_hint_label = nullptr;
    
    initialized = false;
    active = false;
    Serial.println("Home App deinitialized");
}

lv_obj_t* HomeApp::createScreen() {
    // Create main screen container
    lv_obj_t* scr = lv_obj_create(NULL);
    setBackgroundColor(0x001122);
    
    // Create title
    createTitle("🏠 HOME KNOB", 0x87CEEB);
    
    // Create status indicators
    wifi_status_label = createLabel("📶 WiFi: Checking...", 0xFFFF00, LV_ALIGN_CENTER, 0, -30);
    mqtt_status_label = createLabel("📡 MQTT: Checking...", 0xFFFF00, LV_ALIGN_CENTER, 0, 0);
    
    // Create navigation hint
    nav_hint_label = createLabel("🔄 Turn to navigate\n🔘 Press to select", 0xCCCCCC, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_align(nav_hint_label, LV_TEXT_ALIGN_CENTER, 0);
    
    return scr;
}

void HomeApp::onEnter() {
    if (!initialized) return;
    
    active = true;
    last_update = 0; // Force immediate update
    updateStatus();
    
    Serial.println("Home App entered");
}

void HomeApp::onExit() {
    active = false;
    Serial.println("Home App exited");
}

void HomeApp::update() {
    if (!active || !initialized) return;
    
    unsigned long now = millis();
    if (now - last_update >= UPDATE_INTERVAL) {
        updateStatus();
        last_update = now;
    }
}

void HomeApp::onWiFiStatusChange(bool connected) {
    if (active && wifi_status_label) {
        updateStatus();
    }
}

void HomeApp::updateStatus() {
    if (!wifi_status_label || !mqtt_status_label) return;
    
    // Update WiFi status
    bool wifi_connected = wifiManager.isWiFiConnected();
    if (wifi_connected) {
        String wifi_text = "📶 WiFi: " + wifiManager.getSSID();
        lv_label_set_text(wifi_status_label, wifi_text.c_str());
        lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x00FF00), 0);
    } else {
        lv_label_set_text(wifi_status_label, "📶 WiFi: Disconnected");
        lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFF0000), 0);
    }
    
    // Update MQTT status
    bool mqtt_connected = mqttManager.connected();
    if (mqtt_connected) {
        String mqtt_text = "📡 MQTT: " + mqttManager.getServer();
        lv_label_set_text(mqtt_status_label, mqtt_text.c_str());
        lv_obj_set_style_text_color(mqtt_status_label, lv_color_hex(0x00FF00), 0);
    } else {
        lv_label_set_text(mqtt_status_label, "📡 MQTT: Disconnected");
        lv_obj_set_style_text_color(mqtt_status_label, lv_color_hex(0xFF0000), 0);
    }
}
