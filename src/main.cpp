#include <Arduino.h>
#include <Preferences.h>
#include <lvgl.h>
#include "lcd_config.h"
#include "bidi_switch_knob.h"
#in    // Initialize encoder with simple callback to AppManager
    if (!EncoderManager::begin()) {
        Serial.println("Failed to initialize encoder");
        return;
    }
    
    // Set up simple encoder callback - encoder just calls AppManager
    EncoderManager::setChangeCallback([](int direction) {
        appManager.onEncoderChange(direction);
    });r.h"
#include "mqtt_manager.h"
#include "display_driver.h"
#include "command_handler.h"
#include "encoder_manager.h"
#include "app_manager.h"

// Include all the apps
#include "apps/home_app.h"
#include "apps/energy_app.h"
#include "apps/weather_app.h"
#include "apps/house_app.h"
#include "apps/clock_app.h"
#include "apps/settings_app.h"

// ================================
// GLOBAL INSTANCES
// ================================

Preferences preferences;

// Global app instances - these will register themselves
HomeApp homeApp;
EnergyApp energyApp;
WeatherApp weatherApp;
HouseApp houseApp;
ClockApp clockApp;
SettingsApp settingsApp;

// Function declarations
void onMQTTMessage(const char* topic, const char* payload);

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32-S3 Knob Starting...");
    
    // Initialize preferences
    preferences.begin("config", false);
    
    // Initialize display system using DisplayManager
    if (!DisplayManager::initLVGL()) {
        Serial.println("Failed to initialize LVGL");
        return;
    }
    
    if (!DisplayManager::initDisplay()) {
        Serial.println("Failed to initialize display driver");
        return;
    }
    
    if (!DisplayManager::initInput()) {
        Serial.println("Failed to initialize input driver");
        return;
    }
    
    Serial.println("Display system initialized successfully");
    
    // Initialize WiFi
    wifiManager.begin();
    
    // Initialize MQTT
    mqttManager.begin();
    
    // Initialize each app - they will register themselves during init()
    Serial.println("Initializing apps...");
    
    homeApp.init();
    energyApp.init();
    weatherApp.init();
    houseApp.init();
    clockApp.init();
    settingsApp.init();
    
    Serial.printf("Initialized and registered %d apps\n", appManager.getAppCount());
    
    // Initialize encoder with simple callback to AppManager
    if (!EncoderManager::begin()) {
        Serial.println("Failed to initialize encoder");
        return;
    }
    
    // Set up simple encoder callback - encoder just calls AppManager
    EncoderManager::setChangeCallback([](int direction) {
        appManager.onEncoderChange(direction);
    });
    
    Serial.println("System initialization complete");
}

void loop() {
    // Handle system updates
    // WiFiManager doesn't need update/loop method
    mqttManager.loop();
    DisplayManager::handleLVGLTasks();
    
    // Update current app
    appManager.update();
    
    // Small delay to prevent watchdog issues
    vTaskDelay(pdMS_TO_TICKS(10));
}
