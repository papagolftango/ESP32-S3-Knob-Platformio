#include "house_app.h"

bool HouseApp::init() {
    if (initialized) return true;
    
    Serial.println("Initializing House App...");
    
    // Initialize with default values
    lights_on = 5;
    total_lights = 12;
    indoor_temp = 21.0;
    security_armed = true;
    security_status = "Armed";
    
    // Create the screen
    screen = createScreen();
    if (!screen) {
        Serial.println("Failed to create House screen");
        return false;
    }
    
    initialized = true;
    Serial.println("House App initialized successfully");
    return true;
}

void HouseApp::deinit() {
    if (!initialized) return;
    
    if (screen) {
        lv_obj_del(screen);
        screen = nullptr;
    }
    
    lights_label = nullptr;
    temp_label = nullptr;
    security_label = nullptr;
    
    initialized = false;
    active = false;
    Serial.println("House App deinitialized");
}

lv_obj_t* HouseApp::createScreen() {
    // Create main screen container
    lv_obj_t* scr = lv_obj_create(NULL);
    if (!scr) return nullptr;
    
    // Set the screen as the current context for helper methods
    screen = scr;
    setBackgroundColor(0x220011);
    
    // Create title
    createTitle("🏠 HOUSE", 0xDDA0DD);
    
    // Create lights display
    lights_label = createLabel("💡 Lights: 5/12 on", 0xFFD700, LV_ALIGN_CENTER, 0, -20);
    
    // Create temperature display
    temp_label = createLabel("🌡️ Indoor: 21°C", 0x87CEEB, LV_ALIGN_CENTER, 0, 10);
    
    // Create security display
    security_label = createLabel("🔒 Security: Armed", 0x90EE90, LV_ALIGN_CENTER, 0, 40);
    
    updateDisplay();
    return scr;
}

void HouseApp::onEnter() {
    if (!initialized) return;
    
    active = true;
    last_update = 0; // Force immediate update
    updateDisplay();
    
    Serial.println("House App entered");
}

void HouseApp::onExit() {
    active = false;
    Serial.println("House App exited");
}

void HouseApp::update() {
    if (!active || !initialized) return;
    
    unsigned long now = millis();
    if (now - last_update >= UPDATE_INTERVAL) {
        // Simulate house automation changes for demo
        
        // Randomly turn lights on/off
        if (random(0, 100) < 20) { // 20% chance
            lights_on += random(-1, 2); // ±1 light
            lights_on = constrain(lights_on, 0, total_lights);
        }
        
        // Small temperature variations
        indoor_temp += (random(-2, 3) / 10.0); // ±0.2°C variation
        indoor_temp = constrain(indoor_temp, 15.0, 30.0);
        
        updateDisplay();
        last_update = now;
    }
}

void HouseApp::onMQTTMessage(const String& topic, const String& payload) {
    if (!active) return;
    
    // Parse MQTT messages for home automation data
    if (topic.indexOf("house") >= 0 || topic.indexOf("home") >= 0 || topic.indexOf("lights") >= 0) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            if (doc["lights_on"].is<int>()) {
                setLightsOn(doc["lights_on"].as<int>());
            }
            if (doc["total_lights"].is<int>()) {
                setTotalLights(doc["total_lights"].as<int>());
            }
            if (doc["indoor_temperature"].is<float>() || doc["temperature"].is<float>()) {
                float temp = doc["indoor_temperature"].is<float>() ? 
                           doc["indoor_temperature"].as<float>() : 
                           doc["temperature"].as<float>();
                setIndoorTemperature(temp);
            }
            if (doc["security_armed"].is<bool>()) {
                bool armed = doc["security_armed"].as<bool>();
                String status = doc["security_status"].is<String>() ? 
                              doc["security_status"].as<String>() : "";
                setSecurityStatus(armed, status);
            }
            
            updateDisplay();
            Serial.printf("House App: Updated from MQTT - Lights: %d/%d, Temp: %.1f°C\n", 
                         lights_on, total_lights, indoor_temp);
        }
    }
}

void HouseApp::setLightsOn(int count) {
    lights_on = constrain(count, 0, total_lights);
    if (active) updateDisplay();
}

void HouseApp::setTotalLights(int total) {
    total_lights = max(total, 1);
    lights_on = constrain(lights_on, 0, total_lights);
    if (active) updateDisplay();
}

void HouseApp::setIndoorTemperature(float temp_c) {
    indoor_temp = temp_c;
    if (active) updateDisplay();
}

void HouseApp::setSecurityStatus(bool armed, const String& status) {
    security_armed = armed;
    if (!status.isEmpty()) {
        security_status = status;
    } else {
        security_status = armed ? "Armed" : "Disarmed";
    }
    if (active) updateDisplay();
}

String HouseApp::getSecurityIcon(bool armed) {
    return armed ? "🔒" : "🔓";
}

void HouseApp::updateDisplay() {
    if (!lights_label || !temp_label || !security_label) return;
    
    // Update lights display
    char lights_text[32];
    snprintf(lights_text, sizeof(lights_text), "💡 Lights: %d/%d on", lights_on, total_lights);
    lv_label_set_text(lights_label, lights_text);
    
    // Update temperature display
    char temp_text[32];
    snprintf(temp_text, sizeof(temp_text), "🌡️ Indoor: %.1f°C", indoor_temp);
    lv_label_set_text(temp_label, temp_text);
    
    // Update security display
    String security_icon = getSecurityIcon(security_armed);
    String security_text = security_icon + " Security: " + security_status;
    lv_label_set_text(security_label, security_text.c_str());
    
    // Update security color based on status
    uint32_t security_color = security_armed ? 0x90EE90 : 0xFF6B35; // Green if armed, orange if disarmed
    lv_obj_set_style_text_color(security_label, lv_color_hex(security_color), 0);
}
