#include "weather_app.h"

bool WeatherApp::init() {
    if (initialized) return true;
    
    Serial.println("Initializing Weather App...");
    
    // Initialize with default values
    temperature = 22.3;
    humidity = 65;
    condition = "Sunny";
    condition_icon = getWeatherIcon(condition);
    
    // Create the screen
    screen = createScreen();
    if (!screen) {
        Serial.println("Failed to create Weather screen");
        return false;
    }
    
    initialized = true;
    Serial.println("Weather App initialized successfully");
    return true;
}

void WeatherApp::deinit() {
    if (!initialized) return;
    
    if (screen) {
        lv_obj_del(screen);
        screen = nullptr;
    }
    
    temp_label = nullptr;
    humidity_label = nullptr;
    condition_label = nullptr;
    
    initialized = false;
    active = false;
    Serial.println("Weather App deinitialized");
}

lv_obj_t* WeatherApp::createScreen() {
    // Create main screen container
    lv_obj_t* scr = lv_obj_create(NULL);
    if (!scr) return nullptr;
    
    // Set the screen as the current context for helper methods
    screen = scr;
    setBackgroundColor(0x001144);
    
    // Create title
    createTitle("🌤️ WEATHER", 0x87CEEB);
    
    // Create temperature display
    temp_label = createLabel("🌡️ 22.3°C", 0xFF6B35, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_14, 0);
    
    // Create humidity display
    humidity_label = createLabel("💧 65%", 0x87CEEB, LV_ALIGN_CENTER, 0, 10);
    
    // Create condition display
    condition_label = createLabel("☀️ Sunny", 0xFFD700, LV_ALIGN_CENTER, 0, 40);
    
    updateDisplay();
    return scr;
}

void WeatherApp::onEnter() {
    if (!initialized) return;
    
    active = true;
    last_update = 0; // Force immediate update
    updateDisplay();
    
    Serial.println("Weather App entered");
}

void WeatherApp::onExit() {
    active = false;
    Serial.println("Weather App exited");
}

void WeatherApp::update() {
    if (!active || !initialized) return;
    
    unsigned long now = millis();
    if (now - last_update >= UPDATE_INTERVAL) {
        // Simulate weather changes for demo
        temperature += (random(-5, 6) / 10.0); // ±0.5°C variation
        if (temperature < -10) temperature = -10;
        if (temperature > 40) temperature = 40;
        
        humidity += random(-2, 3); // ±2% variation
        if (humidity < 20) humidity = 20;
        if (humidity > 95) humidity = 95;
        
        updateDisplay();
        last_update = now;
    }
}

void WeatherApp::onMQTTMessage(const String& topic, const String& payload) {
    if (!active) return;
    
    // Parse MQTT messages for weather data
    if (topic.indexOf("weather") >= 0 || topic.indexOf("temperature") >= 0) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            if (doc["temperature"].is<float>()) {
                setTemperature(doc["temperature"].as<float>());
            }
            if (doc["humidity"].is<int>()) {
                setHumidity(doc["humidity"].as<int>());
            }
            if (doc["condition"].is<String>()) {
                setCondition(doc["condition"].as<String>());
            }
            
            updateDisplay();
            Serial.printf("Weather App: Updated from MQTT - Temp: %.1f°C, Humidity: %d%%\n", temperature, humidity);
        }
    }
}

void WeatherApp::setTemperature(float temp_c) {
    temperature = temp_c;
    if (active) updateDisplay();
}

void WeatherApp::setHumidity(int humidity_percent) {
    humidity = constrain(humidity_percent, 0, 100);
    if (active) updateDisplay();
}

void WeatherApp::setCondition(const String& weather_condition) {
    condition = weather_condition;
    condition_icon = getWeatherIcon(condition);
    if (active) updateDisplay();
}

String WeatherApp::getWeatherIcon(const String& condition) {
    String cond = condition;
    cond.toLowerCase();
    
    if (cond.indexOf("sunny") >= 0 || cond.indexOf("clear") >= 0) return "☀️";
    if (cond.indexOf("cloud") >= 0) return "☁️";
    if (cond.indexOf("rain") >= 0) return "🌧️";
    if (cond.indexOf("storm") >= 0) return "⛈️";
    if (cond.indexOf("snow") >= 0) return "❄️";
    if (cond.indexOf("fog") >= 0) return "🌫️";
    if (cond.indexOf("wind") >= 0) return "💨";
    
    return "🌤️"; // Default partly cloudy
}

void WeatherApp::updateDisplay() {
    if (!temp_label || !humidity_label || !condition_label) return;
    
    // Update temperature display
    char temp_text[32];
    snprintf(temp_text, sizeof(temp_text), "🌡️ %.1f°C", temperature);
    lv_label_set_text(temp_label, temp_text);
    
    // Update humidity display
    char humidity_text[32];
    snprintf(humidity_text, sizeof(humidity_text), "💧 %d%%", humidity);
    lv_label_set_text(humidity_label, humidity_text);
    
    // Update condition display
    String condition_text = condition_icon + " " + condition;
    lv_label_set_text(condition_label, condition_text.c_str());
}
