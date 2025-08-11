#include <Arduino.h>
#include <Preferences.h>
#include <lvgl.h>
#include "lcd_config.h"
#include "bidi_switch_knob.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "display_driver.h"
#include "command_handler.h"
#include "encoder_manager.h"

// ================================
// HARDWARE DEFINITIONS
// ================================

// Use constants from lcd_config.h for proper hardware configuration
// These match Volos's working ESP32-S3 Knob setup

// Encoder pins: GPIO 8 (A) and GPIO 7 (B) - Volos's working configuration (no button)

// ================================
// GLOBAL INSTANCES
// ================================

// Global preferences instance
Preferences preferences;

// ================================
// SCREEN MANAGEMENT
// ================================

// Screen state variables
ScreenType currentScreen = SCREEN_HOME;
SettingsOption currentSettingsOption = SETTINGS_WIFI_RESET;
lv_obj_t* screens[SCREEN_COUNT];
lv_obj_t* currentScreenObj = nullptr;
lv_obj_t* settingsLabels[SETTINGS_COUNT];  // For highlighting selected option
bool inSettingsMenu = false;
unsigned long settingsSelectTime = 0;

// Function declarations
void setupWiFiAndMQTT();
void onWiFiConfigSaved();
void switchToScreen(ScreenType screen);

// ================================
// WIFI AND MQTT SETUP
// ================================

void setupWiFiAndMQTT() {
    // Setup WiFi with custom styling
    wifiManager.setCustomHeadElement("<style>body{background:#1e1e1e;color:#fff;font-family:Arial,sans-serif;}.c{text-align:center;}div,input{padding:5px;font-size:1em;margin:5px 0;box-sizing:border-box;background:#333;border:1px solid #555;color:#fff;}input[type='submit']{background:#0066cc;cursor:pointer;}input[type='submit']:hover{background:#0052a3;}</style>");
    
    // Set WiFi Manager callbacks
    wifiManager.setSaveConfigCallback(onWiFiConfigSaved);
    wifiManager.setConfigPortalTimeout(300); // 5 minutes
    
    // Setup MQTT Manager with WiFiManager integration
    mqttManager.setupWiFiManagerParameters(wifiManager);
    
    // Start WiFi connection
    Serial.println("Starting WiFi connection...");
    bool wifiConnected = wifiManager.autoConnect("ESP32-Knob-Setup", "smartknob123");
    
    if (wifiConnected) {
        Serial.println("WiFi connected successfully!");
        Serial.printf("IP address: %s\n", wifiManager.getIP().c_str());
        Serial.printf("SSID: %s\n", wifiManager.getSSID().c_str());
        
        // Update MQTT config and setup MQTT
        mqttManager.updateConfigFromWiFiManager(wifiManager);
        
        // Optional: Set custom data type callbacks for specialized handling
        mqttManager.setEnergyCallback([](const JsonDocument& data, const String& topic) {
            float power = mqttManager.extractFloatFromJson(data, "power", 0.0);
            float energy = mqttManager.extractFloatFromJson(data, "energy", 0.0);
            Serial.printf("⚡ Energy Update - Power: %.2f W, Total: %.2f kWh\n", power, energy);
            // Here you could update display elements, trigger actions, etc.
        });
        
        mqttManager.setWeatherCallback([](const JsonDocument& data, const String& topic) {
            float temp = mqttManager.extractFloatFromJson(data, "temperature", 0.0);
            int humidity = mqttManager.extractIntFromJson(data, "humidity", 0);
            Serial.printf("🌡️ Weather Update - %.1f°C, %d%% humidity\n", temp, humidity);
            // Here you could update weather display, adjust heating, etc.
        });
        
        mqttManager.setupWithWiFiManager(wifiManager);
        
    } else {
        Serial.println("WiFi connection failed - check configuration portal");
        Serial.println("Connect to 'ESP32-Knob-Setup' network and configure WiFi");
        Serial.println("Default password: smartknob123");
    }
}

void onWiFiConfigSaved() {
    Serial.println("WiFi and MQTT configuration saved!");
    mqttManager.updateConfigFromWiFiManager(wifiManager);
}

// ================================
// SCREEN CREATION FUNCTIONS
// ================================

void createHomeScreen() {
    screens[SCREEN_HOME] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[SCREEN_HOME], lv_color_hex(0x001122), 0);
    
    // Title
    lv_obj_t* title = lv_label_create(screens[SCREEN_HOME]);
    lv_label_set_text(title, "🏠 HOME KNOB");
    lv_obj_set_style_text_color(title, lv_color_hex(0x87CEEB), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Status indicators
    lv_obj_t* wifi_status = lv_label_create(screens[SCREEN_HOME]);
    lv_label_set_text(wifi_status, wifiManager.isWiFiConnected() ? "📶 WiFi: Connected" : "📶 WiFi: Disconnected");
    lv_obj_set_style_text_color(wifi_status, wifiManager.isWiFiConnected() ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);
    lv_obj_align(wifi_status, LV_ALIGN_CENTER, 0, -30);
    
    lv_obj_t* mqtt_status = lv_label_create(screens[SCREEN_HOME]);
    lv_label_set_text(mqtt_status, mqttManager.connected() ? "📡 MQTT: Connected" : "📡 MQTT: Disconnected");
    lv_obj_set_style_text_color(mqtt_status, mqttManager.connected() ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);
    lv_obj_align(mqtt_status, LV_ALIGN_CENTER, 0, 0);
    
    // Navigation hint
    lv_obj_t* nav_hint = lv_label_create(screens[SCREEN_HOME]);
    lv_label_set_text(nav_hint, "🔄 Turn to navigate\n🔘 Press to select");
    lv_obj_set_style_text_color(nav_hint, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_align(nav_hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(nav_hint, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void createEnergyScreen() {
    screens[SCREEN_ENERGY] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[SCREEN_ENERGY], lv_color_hex(0x221100), 0);
    
    lv_obj_t* title = lv_label_create(screens[SCREEN_ENERGY]);
    lv_label_set_text(title, "⚡ ENERGY");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFD700), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    lv_obj_t* power_label = lv_label_create(screens[SCREEN_ENERGY]);
    lv_label_set_text(power_label, "Power: 2.5 kW");
    lv_obj_set_style_text_color(power_label, lv_color_hex(0xFF6B35), 0);
    lv_obj_set_style_text_font(power_label, &lv_font_montserrat_14, 0);
    lv_obj_align(power_label, LV_ALIGN_CENTER, 0, -20);
    
    lv_obj_t* usage_label = lv_label_create(screens[SCREEN_ENERGY]);
    lv_label_set_text(usage_label, "Daily: 45.2 kWh");
    lv_obj_set_style_text_color(usage_label, lv_color_hex(0x87CEEB), 0);
    lv_obj_align(usage_label, LV_ALIGN_CENTER, 0, 10);
    
    lv_obj_t* cost_label = lv_label_create(screens[SCREEN_ENERGY]);
    lv_label_set_text(cost_label, "Cost: £12.45");
    lv_obj_set_style_text_color(cost_label, lv_color_hex(0x90EE90), 0);
    lv_obj_align(cost_label, LV_ALIGN_CENTER, 0, 40);
}

void createWeatherScreen() {
    screens[SCREEN_WEATHER] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[SCREEN_WEATHER], lv_color_hex(0x001144), 0);
    
    lv_obj_t* title = lv_label_create(screens[SCREEN_WEATHER]);
    lv_label_set_text(title, "🌤️ WEATHER");
    lv_obj_set_style_text_color(title, lv_color_hex(0x87CEEB), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    lv_obj_t* temp_label = lv_label_create(screens[SCREEN_WEATHER]);
    lv_label_set_text(temp_label, "🌡️ 22.3°C");
    lv_obj_set_style_text_color(temp_label, lv_color_hex(0xFF6B35), 0);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_14, 0);
    lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, -20);
    
    lv_obj_t* humidity_label = lv_label_create(screens[SCREEN_WEATHER]);
    lv_label_set_text(humidity_label, "💧 65%");
    lv_obj_set_style_text_color(humidity_label, lv_color_hex(0x87CEEB), 0);
    lv_obj_align(humidity_label, LV_ALIGN_CENTER, 0, 10);
    
    lv_obj_t* condition_label = lv_label_create(screens[SCREEN_WEATHER]);
    lv_label_set_text(condition_label, "☀️ Sunny");
    lv_obj_set_style_text_color(condition_label, lv_color_hex(0xFFD700), 0);
    lv_obj_align(condition_label, LV_ALIGN_CENTER, 0, 40);
}

void createHouseScreen() {
    screens[SCREEN_HOUSE] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[SCREEN_HOUSE], lv_color_hex(0x220011), 0);
    
    lv_obj_t* title = lv_label_create(screens[SCREEN_HOUSE]);
    lv_label_set_text(title, "🏠 HOUSE");
    lv_obj_set_style_text_color(title, lv_color_hex(0xDDA0DD), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    lv_obj_t* lights_label = lv_label_create(screens[SCREEN_HOUSE]);
    lv_label_set_text(lights_label, "💡 Lights: 5 on");
    lv_obj_set_style_text_color(lights_label, lv_color_hex(0xFFD700), 0);
    lv_obj_align(lights_label, LV_ALIGN_CENTER, 0, -20);
    
    lv_obj_t* temp_label = lv_label_create(screens[SCREEN_HOUSE]);
    lv_label_set_text(temp_label, "🌡️ Indoor: 21°C");
    lv_obj_set_style_text_color(temp_label, lv_color_hex(0x87CEEB), 0);
    lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, 10);
    
    lv_obj_t* security_label = lv_label_create(screens[SCREEN_HOUSE]);
    lv_label_set_text(security_label, "🔒 Security: Armed");
    lv_obj_set_style_text_color(security_label, lv_color_hex(0x90EE90), 0);
    lv_obj_align(security_label, LV_ALIGN_CENTER, 0, 40);
}

void createClockScreen() {
    screens[SCREEN_CLOCK] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[SCREEN_CLOCK], lv_color_hex(0x111111), 0);
    
    lv_obj_t* title = lv_label_create(screens[SCREEN_CLOCK]);
    lv_label_set_text(title, "🕐 CLOCK");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    lv_obj_t* time_label = lv_label_create(screens[SCREEN_CLOCK]);
    lv_label_set_text(time_label, "14:35:22");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_14, 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -10);
    
    lv_obj_t* date_label = lv_label_create(screens[SCREEN_CLOCK]);
    lv_label_set_text(date_label, "Aug 11, 2025");
    lv_obj_set_style_text_color(date_label, lv_color_hex(0x87CEEB), 0);
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 20);
}

void createSettingsScreen() {
    screens[SCREEN_SETTINGS] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[SCREEN_SETTINGS], lv_color_hex(0x112211), 0);
    
    lv_obj_t* title = lv_label_create(screens[SCREEN_SETTINGS]);
    lv_label_set_text(title, "⚙️ SETTINGS");
    lv_obj_set_style_text_color(title, lv_color_hex(0x90EE90), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // WiFi Reset option
    settingsLabels[SETTINGS_WIFI_RESET] = lv_label_create(screens[SCREEN_SETTINGS]);
    lv_label_set_text(settingsLabels[SETTINGS_WIFI_RESET], "📶 WiFi Reset");
    lv_obj_set_style_text_color(settingsLabels[SETTINGS_WIFI_RESET], lv_color_hex(0x87CEEB), 0);
    lv_obj_align(settingsLabels[SETTINGS_WIFI_RESET], LV_ALIGN_CENTER, 0, -50);
    
    // MQTT Reset option
    settingsLabels[SETTINGS_MQTT_RESET] = lv_label_create(screens[SCREEN_SETTINGS]);
    lv_label_set_text(settingsLabels[SETTINGS_MQTT_RESET], "📡 MQTT Reset");
    lv_obj_set_style_text_color(settingsLabels[SETTINGS_MQTT_RESET], lv_color_hex(0xFFD700), 0);
    lv_obj_align(settingsLabels[SETTINGS_MQTT_RESET], LV_ALIGN_CENTER, 0, -25);
    
    // Factory Reset option
    settingsLabels[SETTINGS_FACTORY_RESET] = lv_label_create(screens[SCREEN_SETTINGS]);
    lv_label_set_text(settingsLabels[SETTINGS_FACTORY_RESET], "🔄 Factory Reset");
    lv_obj_set_style_text_color(settingsLabels[SETTINGS_FACTORY_RESET], lv_color_hex(0xFF4444), 0);
    lv_obj_align(settingsLabels[SETTINGS_FACTORY_RESET], LV_ALIGN_CENTER, 0, 0);
    
    // Restart option
    settingsLabels[SETTINGS_RESTART] = lv_label_create(screens[SCREEN_SETTINGS]);
    lv_label_set_text(settingsLabels[SETTINGS_RESTART], "🔄 Restart");
    lv_obj_set_style_text_color(settingsLabels[SETTINGS_RESTART], lv_color_hex(0xFFA500), 0);
    lv_obj_align(settingsLabels[SETTINGS_RESTART], LV_ALIGN_CENTER, 0, 25);
    
    // Exit option
    settingsLabels[SETTINGS_EXIT] = lv_label_create(screens[SCREEN_SETTINGS]);
    lv_label_set_text(settingsLabels[SETTINGS_EXIT], "⬅️ Exit Settings");
    lv_obj_set_style_text_color(settingsLabels[SETTINGS_EXIT], lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(settingsLabels[SETTINGS_EXIT], LV_ALIGN_CENTER, 0, 50);
    
    // Instructions
    lv_obj_t* instructions = lv_label_create(screens[SCREEN_SETTINGS]);
    lv_label_set_text(instructions, "Turn: Navigate • Hold 3s: Select");
    lv_obj_set_style_text_color(instructions, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_align(instructions, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(instructions, LV_ALIGN_BOTTOM_MID, 0, -10);
}

void updateSettingsHighlight() {
    // Reset all colors to normal
    lv_obj_set_style_text_color(settingsLabels[SETTINGS_WIFI_RESET], lv_color_hex(0x87CEEB), 0);
    lv_obj_set_style_text_color(settingsLabels[SETTINGS_MQTT_RESET], lv_color_hex(0xFFD700), 0);
    lv_obj_set_style_text_color(settingsLabels[SETTINGS_FACTORY_RESET], lv_color_hex(0xFF4444), 0);
    lv_obj_set_style_text_color(settingsLabels[SETTINGS_RESTART], lv_color_hex(0xFFA500), 0);
    lv_obj_set_style_text_color(settingsLabels[SETTINGS_EXIT], lv_color_hex(0xCCCCCC), 0);
    
    // Highlight current selection
    lv_obj_set_style_text_color(settingsLabels[currentSettingsOption], lv_color_hex(0x00FF00), 0);
    
    // Update text to show selection state
    const char* baseTexts[] = {
        "📶 WiFi Reset",
        "📡 MQTT Reset", 
        "🔄 Factory Reset",
        "🔄 Restart",
        "⬅️ Exit Settings"
    };
    
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        if (i == currentSettingsOption) {
            String highlightedText = "> " + String(baseTexts[i]) + " <";
            lv_label_set_text(settingsLabels[i], highlightedText.c_str());
        } else {
            lv_label_set_text(settingsLabels[i], baseTexts[i]);
        }
    }
}

void executeSettingsAction() {
    Serial.printf("Executing settings action: %d\n", currentSettingsOption);
    
    switch (currentSettingsOption) {
        case SETTINGS_WIFI_RESET:
            Serial.println("WiFi Reset selected - Clearing WiFi configuration...");
            lv_label_set_text(settingsLabels[SETTINGS_WIFI_RESET], "📶 Resetting WiFi...");
            lv_task_handler();  // Update display
            delay(1000);
            wifiManager.reset();
            ESP.restart();
            break;
            
        case SETTINGS_MQTT_RESET:
            Serial.println("MQTT Reset selected - Clearing MQTT configuration...");
            lv_label_set_text(settingsLabels[SETTINGS_MQTT_RESET], "📡 Resetting MQTT...");
            lv_task_handler();  // Update display
            delay(1000);
            mqttManager.resetConfig();
            mqttManager.saveConfig();
            Serial.println("MQTT config cleared. Restarting...");
            ESP.restart();
            break;
            
        case SETTINGS_FACTORY_RESET:
            Serial.println("Factory Reset selected - Clearing ALL configuration...");
            lv_label_set_text(settingsLabels[SETTINGS_FACTORY_RESET], "🔄 Factory Reset...");
            lv_task_handler();  // Update display
            delay(1000);
            wifiManager.reset();
            mqttManager.resetConfig();
            preferences.clear();
            Serial.println("Factory reset complete. Restarting...");
            ESP.restart();
            break;
            
        case SETTINGS_RESTART:
            Serial.println("Restart selected - Restarting device...");
            lv_label_set_text(settingsLabels[SETTINGS_RESTART], "🔄 Restarting...");
            lv_task_handler();  // Update display
            delay(1000);
            ESP.restart();
            break;
            
        case SETTINGS_EXIT:
            Serial.println("Exit selected - Returning to home screen");
            inSettingsMenu = false;
            EncoderManager::exitSettingsMenu();
            switchToScreen(SCREEN_HOME);
            break;
    }
}

void switchToScreen(ScreenType screen) {
    if (screen < SCREEN_COUNT && screens[screen] != nullptr) {
        currentScreen = screen;
        lv_scr_load(screens[screen]);
        currentScreenObj = screens[screen];
        
        // Update encoder manager state
        EncoderManager::setCurrentScreen(screen);
        
        // Handle settings screen special behavior
        if (screen == SCREEN_SETTINGS) {
            inSettingsMenu = true;
            currentSettingsOption = SETTINGS_WIFI_RESET;
            updateSettingsHighlight();
            EncoderManager::enterSettingsMenu();
        } else {
            inSettingsMenu = false;
        }
        
        const char* screenNames[] = {"HOME", "ENERGY", "WEATHER", "HOUSE", "CLOCK", "SETTINGS"};
        Serial.printf("Switched to %s screen\n", screenNames[screen]);
    }
}

// ================================
// ENCODER CALLBACKS
// ================================

void onScreenChange(ScreenType newScreen) {
    switchToScreen(newScreen);
}

void onSettingsNavigation(SettingsOption newOption) {
    currentSettingsOption = newOption;
    updateSettingsHighlight();
}

void onSettingsExecute(SettingsOption option) {
    currentSettingsOption = option;
    executeSettingsAction();
}

// ================================
// SETUP FUNCTION
// ================================

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32-S3 Knob Starting...");
    
    // Initialize preferences
    preferences.begin("config", false);
    
    // Initialize encoder system using EncoderManager
    if (!EncoderManager::begin()) {
        Serial.println("Failed to initialize encoder system");
        return;
    }
    
    // Set up encoder callbacks
    EncoderManager::setScreenChangeCallback(onScreenChange);
    EncoderManager::setSettingsNavigationCallback(onSettingsNavigation);
    EncoderManager::setSettingsExecuteCallback(onSettingsExecute);
    
    Serial.println("Encoder system initialized successfully");
    
    // Initialize display system using DisplayDriver
    if (!DisplayDriver::initLVGL()) {
        Serial.println("Failed to initialize LVGL");
        return;
    }
    
    if (!DisplayDriver::initDisplay()) {
        Serial.println("Failed to initialize display driver");
        return;
    }
    
    if (!DisplayDriver::initInput()) {
        Serial.println("Failed to initialize input driver");
        return;
    }
    
    DisplayDriver::printDisplayInfo();
    Serial.println("Display system initialized successfully");
    
    // Create all screens
    createHomeScreen();
    createEnergyScreen();
    createWeatherScreen();
    createHouseScreen();
    createClockScreen();
    createSettingsScreen();
    
    // Start with home screen
    switchToScreen(SCREEN_HOME);
    EncoderManager::setCurrentScreen(SCREEN_HOME);
    
    Serial.println("Screens created");
    
    // Initialize WiFi and MQTT using new managers
    setupWiFiAndMQTT();
    
    // Initialize command handler for development/integration commands
    CommandHandler::begin(true);  // Enable commands at startup
    
    Serial.println("Setup complete!");
}

// ================================
// MAIN LOOP
// ================================

void loop() {
    // Handle LVGL tasks using DisplayDriver
    DisplayDriver::handleLVGLTasks();
    
    // Handle serial commands using CommandHandler
    CommandHandler::handleSerialInput();
    
    // Handle MQTT connection and messages
    mqttManager.loop();
    
    // Small delay to prevent watchdog issues
    delay(5);
}
