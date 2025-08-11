#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <ESP32Encoder.h>
#include <lvgl.h>
#include "lcd_config.h"

// ================================
// HARDWARE DEFINITIONS
// ================================

// Use constants from lcd_config.h for proper hardware configuration
// These match Volos's working ESP32-S3 Knob setup

// Display dimensions (correct resolution for this hardware)
#define SCREEN_WIDTH  EXAMPLE_LCD_H_RES
#define SCREEN_HEIGHT EXAMPLE_LCD_V_RES

// Encoder pins (based on Volos's working configuration - NO BUTTON)
#define ENCODER_A  EXAMPLE_ENCODER_ECA_PIN
#define ENCODER_B  EXAMPLE_ENCODER_ECB_PIN
// Note: No encoder button in Volos's configuration

// ================================
// LVGL CONFIGURATION
// ================================

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[SCREEN_WIDTH * 10];
static lv_color_t buf2[SCREEN_WIDTH * 10];

// ================================
// MQTT CONFIGURATION
// ================================

struct MQTTConfig {
    char server[64] = "mqtt.local";
    int port = 1883;
    char username[32] = "";
    char password[32] = "";
    char clientId[32] = "";
    bool useSSL = false;
};

MQTTConfig mqttConfig;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Preferences preferences;

// ================================
// ROTARY ENCODER
// ================================

ESP32Encoder encoder;
int lastEncoderValue = 0;
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 200;

// ================================
// SCREEN MANAGEMENT
// ================================

enum ScreenType {
    SCREEN_HOME,
    SCREEN_ENERGY,
    SCREEN_WEATHER,
    SCREEN_HOUSE,
    SCREEN_CLOCK,
    SCREEN_SETTINGS,
    SCREEN_COUNT
};

ScreenType currentScreen = SCREEN_HOME;
lv_obj_t* screens[SCREEN_COUNT];
lv_obj_t* currentScreenObj = nullptr;

// ================================
// DISPLAY DRIVER
// ================================

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    // This is a placeholder - you'll need to implement based on your display driver
    // For now, just mark as ready
    lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    // Placeholder for touch input - we'll use rotary encoder instead
    data->state = LV_INDEV_STATE_REL;
}

// ================================
// MQTT FUNCTIONS
// ================================

void loadMQTTConfig() {
    preferences.begin("mqtt", false);
    
    preferences.getString("server", mqttConfig.server, sizeof(mqttConfig.server));
    mqttConfig.port = preferences.getInt("port", 1883);
    preferences.getString("username", mqttConfig.username, sizeof(mqttConfig.username));
    preferences.getString("password", mqttConfig.password, sizeof(mqttConfig.password));
    preferences.getString("clientId", mqttConfig.clientId, sizeof(mqttConfig.clientId));
    mqttConfig.useSSL = preferences.getBool("useSSL", false);
    
    // Generate client ID if empty
    if (strlen(mqttConfig.clientId) == 0) {
        snprintf(mqttConfig.clientId, sizeof(mqttConfig.clientId), "esp32-knob-%06X", (uint32_t)ESP.getEfuseMac());
    }
    
    preferences.end();
    
    Serial.printf("MQTT Config loaded: %s:%d, Client: %s\n", 
                  mqttConfig.server, mqttConfig.port, mqttConfig.clientId);
}

void saveMQTTConfig() {
    preferences.begin("mqtt", false);
    
    preferences.putString("server", mqttConfig.server);
    preferences.putInt("port", mqttConfig.port);
    preferences.putString("username", mqttConfig.username);
    preferences.putString("password", mqttConfig.password);
    preferences.putString("clientId", mqttConfig.clientId);
    preferences.putBool("useSSL", mqttConfig.useSSL);
    
    preferences.end();
    
    Serial.println("MQTT Config saved");
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    Serial.printf("MQTT Message [%s]: %s\n", topic, message);
    
    // Parse and handle different topic types
    if (strstr(topic, "/energy/")) {
        // Handle energy data
        JsonDocument doc;
        deserializeJson(doc, message);
        
        if (doc["power"].is<float>()) {
            float power = doc["power"];
            Serial.printf("Power: %.2f W\n", power);
        }
    }
    else if (strstr(topic, "/weather/")) {
        // Handle weather data
        JsonDocument doc;
        deserializeJson(doc, message);
        
        if (doc["temperature"].is<float>()) {
            float temp = doc["temperature"];
            Serial.printf("Temperature: %.1f°C\n", temp);
        }
    }
}

void connectMQTT() {
    if (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        
        mqttClient.setServer(mqttConfig.server, mqttConfig.port);
        mqttClient.setCallback(onMqttMessage);
        
        bool connected = false;
        if (strlen(mqttConfig.username) > 0) {
            connected = mqttClient.connect(mqttConfig.clientId, mqttConfig.username, mqttConfig.password);
        } else {
            connected = mqttClient.connect(mqttConfig.clientId);
        }
        
        if (connected) {
            Serial.println("connected");
            
            // Subscribe to topics
            mqttClient.subscribe("home/+/energy/+");
            mqttClient.subscribe("home/+/weather/+");
            mqttClient.subscribe("home/+/control/+");
            
            Serial.println("Subscribed to MQTT topics");
        } else {
            Serial.printf("failed, rc=%d\n", mqttClient.state());
        }
    }
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
    lv_label_set_text(wifi_status, WiFi.status() == WL_CONNECTED ? "📶 WiFi: Connected" : "📶 WiFi: Disconnected");
    lv_obj_set_style_text_color(wifi_status, WiFi.status() == WL_CONNECTED ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);
    lv_obj_align(wifi_status, LV_ALIGN_CENTER, 0, -30);
    
    lv_obj_t* mqtt_status = lv_label_create(screens[SCREEN_HOME]);
    lv_label_set_text(mqtt_status, mqttClient.connected() ? "📡 MQTT: Connected" : "📡 MQTT: Disconnected");
    lv_obj_set_style_text_color(mqtt_status, mqttClient.connected() ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);
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
    
    lv_obj_t* wifi_btn = lv_label_create(screens[SCREEN_SETTINGS]);
    lv_label_set_text(wifi_btn, "📶 WiFi Reset");
    lv_obj_set_style_text_color(wifi_btn, lv_color_hex(0x87CEEB), 0);
    lv_obj_align(wifi_btn, LV_ALIGN_CENTER, 0, -30);
    
    lv_obj_t* mqtt_btn = lv_label_create(screens[SCREEN_SETTINGS]);
    lv_label_set_text(mqtt_btn, "📡 MQTT Config");
    lv_obj_set_style_text_color(mqtt_btn, lv_color_hex(0xFFD700), 0);
    lv_obj_align(mqtt_btn, LV_ALIGN_CENTER, 0, 0);
    
    lv_obj_t* factory_btn = lv_label_create(screens[SCREEN_SETTINGS]);
    lv_label_set_text(factory_btn, "🔄 Factory Reset");
    lv_obj_set_style_text_color(factory_btn, lv_color_hex(0xFF4444), 0);
    lv_obj_align(factory_btn, LV_ALIGN_CENTER, 0, 30);
}

void switchToScreen(ScreenType screen) {
    if (screen < SCREEN_COUNT && screens[screen] != nullptr) {
        currentScreen = screen;
        lv_scr_load(screens[screen]);
        currentScreenObj = screens[screen];
        
        const char* screenNames[] = {"HOME", "ENERGY", "WEATHER", "HOUSE", "CLOCK", "SETTINGS"};
        Serial.printf("Switched to %s screen\n", screenNames[screen]);
    }
}

// ================================
// ROTARY ENCODER HANDLING
// ================================

void handleEncoder() {
    int currentValue = encoder.getCount();
    
    if (currentValue != lastEncoderValue) {
        int delta = currentValue - lastEncoderValue;
        lastEncoderValue = currentValue;
        
        if (delta > 0) {
            // Clockwise rotation
            currentScreen = (ScreenType)((currentScreen + 1) % SCREEN_COUNT);
        } else {
            // Counter-clockwise rotation
            currentScreen = (ScreenType)((currentScreen - 1 + SCREEN_COUNT) % SCREEN_COUNT);
        }
        
        switchToScreen(currentScreen);
        
        Serial.printf("Encoder: %d, Screen: %d\n", currentValue, currentScreen);
    }
}

void handleButton() {
    // Button functionality removed - Volos's hardware has no encoder button
    // Navigation is purely through encoder rotation
}

// ================================
// SETUP FUNCTION
// ================================

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32-S3 Knob Starting...");
    
    // Initialize preferences
    preferences.begin("config", false);
    
    // Setup rotary encoder (Volos configuration - no button)
    ESP32Encoder::useInternalWeakPullResistors = UP;
    encoder.attachHalfQuad(ENCODER_A, ENCODER_B);
    encoder.setCount(0);
    
    // No button in Volos's configuration
    
    Serial.println("Rotary encoder initialized (no button)");
    
    // Initialize LVGL
    lv_init();
    
    // Initialize display driver
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, SCREEN_WIDTH * 10);
    
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    
    // Initialize input device
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
    
    Serial.println("LVGL initialized");
    
    // Create all screens
    createHomeScreen();
    createEnergyScreen();
    createWeatherScreen();
    createHouseScreen();
    createClockScreen();
    createSettingsScreen();
    
    // Start with home screen
    switchToScreen(SCREEN_HOME);
    
    Serial.println("Screens created");
    
    // Initialize WiFi
    WiFiManager wm;
    
    // Auto-connect or start config portal
    bool connected = wm.autoConnect("ESP32-Knob-Setup");
    
    if (connected) {
        Serial.println("WiFi connected successfully");
        Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("WiFi connection failed");
    }
    
    // Load MQTT configuration
    loadMQTTConfig();
    
    // Try to connect to MQTT
    if (WiFi.status() == WL_CONNECTED) {
        connectMQTT();
    }
    
    Serial.println("Setup complete!");
}

// ================================
// MAIN LOOP
// ================================

void loop() {
    // Handle LVGL tasks
    lv_timer_handler();
    
    // Handle rotary encoder
    handleEncoder();
    handleButton();
    
    // Handle MQTT
    if (WiFi.status() == WL_CONNECTED) {
        if (!mqttClient.connected()) {
            static unsigned long lastReconnectAttempt = 0;
            if (millis() - lastReconnectAttempt > 5000) {
                lastReconnectAttempt = millis();
                connectMQTT();
            }
        } else {
            mqttClient.loop();
        }
    }
    
    // Small delay to prevent watchdog issues
    delay(5);
}
