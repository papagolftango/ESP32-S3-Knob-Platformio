#include <Arduino.h>
#include <Preferences.h>
#include <lvgl.h>
#include "lcd_config.h"
#include "bidi_switch_knob.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"

// ================================
// HARDWARE DEFINITIONS
// ================================

// Use constants from lcd_config.h for proper hardware configuration
// These match Volos's working ESP32-S3 Knob setup

// Display dimensions (correct resolution for this hardware)
#define SCREEN_WIDTH  EXAMPLE_LCD_H_RES
#define SCREEN_HEIGHT EXAMPLE_LCD_V_RES

// Encoder pins: GPIO 8 (A) and GPIO 7 (B) - Volos's working configuration (no button)

// ================================
// LVGL CONFIGURATION
// ================================

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[SCREEN_WIDTH * 10];
static lv_color_t buf2[SCREEN_WIDTH * 10];

// Global preferences instance
Preferences preferences;

// ================================
// ROTARY ENCODER
// ================================

// ================================
// ENCODER STATE (Volos's proven implementation)
// ================================

// Volos-style bit manipulation macros
#define SET_BIT(reg,bit) (reg |= ((uint32_t)0x01<<bit))
#define CLEAR_BIT(reg,bit) (reg &= (~((uint32_t)0x01<<bit)))
#define READ_BIT(reg,bit) (((uint32_t)reg>>bit) & 0x01)
#define BIT_EVEN_ALL (0x00ffffff)

EventGroupHandle_t knob_even_ = NULL;
static knob_handle_t s_knob = 0;
SemaphoreHandle_t mutex;

int lastEncoderValue = 0;

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

// Function declarations
void setupWiFiAndMQTT();
void onMQTTMessage(char* topic, uint8_t* payload, unsigned int length);
void onMQTTConnect();
void onMQTTDisconnect();
void onWiFiConfigSaved();

// ================================
// WIFI AND MQTT SETUP
// ================================

void setupWiFiAndMQTT() {
    // Setup WiFi with custom styling
    wifiManager.setCustomHeadElement("<style>body{background:#1e1e1e;color:#fff;font-family:Arial,sans-serif;}.c{text-align:center;}div,input{padding:5px;font-size:1em;margin:5px 0;box-sizing:border-box;background:#333;border:1px solid #555;color:#fff;}input[type='submit']{background:#0066cc;cursor:pointer;}input[type='submit']:hover{background:#0052a3;}</style>");
    
    // Get MQTT config for WiFi Manager parameters
    MQTTConfig mqttConfig = mqttManager.getConfig();
    
    // Create WiFi Manager parameters for MQTT configuration
    WiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT Server", mqttConfig.server, 64);
    WiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT Port", String(mqttConfig.port).c_str(), 6);
    WiFiManagerParameter custom_mqtt_username("mqtt_username", "MQTT Username", mqttConfig.username, 32);
    WiFiManagerParameter custom_mqtt_password("mqtt_password", "MQTT Password", mqttConfig.password, 32);
    WiFiManagerParameter custom_mqtt_client_id("mqtt_client_id", "MQTT Client ID", mqttConfig.clientId, 32);
    
    // Add MQTT parameters to WiFi Manager
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_mqtt_username);
    wifiManager.addParameter(&custom_mqtt_password);
    wifiManager.addParameter(&custom_mqtt_client_id);
    
    // Set WiFi Manager callbacks
    wifiManager.setSaveConfigCallback(onWiFiConfigSaved);
    wifiManager.setConfigPortalTimeout(300); // 5 minutes
    
    // Start WiFi connection
    Serial.println("Starting WiFi connection...");
    bool wifiConnected = wifiManager.autoConnect("ESP32-Knob-Setup", "smartknob123");
    
    if (wifiConnected) {
        Serial.println("WiFi connected successfully!");
        Serial.printf("IP address: %s\n", wifiManager.getIP().c_str());
        Serial.printf("SSID: %s\n", wifiManager.getSSID().c_str());
        
        // Update MQTT config from WiFi Manager parameters
        MQTTConfig updatedConfig = mqttConfig;
        if (strlen(custom_mqtt_server.getValue()) > 0) {
            strncpy(updatedConfig.server, custom_mqtt_server.getValue(), sizeof(updatedConfig.server));
        }
        if (strlen(custom_mqtt_port.getValue()) > 0) {
            updatedConfig.port = atoi(custom_mqtt_port.getValue());
        }
        if (strlen(custom_mqtt_username.getValue()) > 0) {
            strncpy(updatedConfig.username, custom_mqtt_username.getValue(), sizeof(updatedConfig.username));
        }
        if (strlen(custom_mqtt_password.getValue()) > 0) {
            strncpy(updatedConfig.password, custom_mqtt_password.getValue(), sizeof(updatedConfig.password));
        }
        if (strlen(custom_mqtt_client_id.getValue()) > 0) {
            strncpy(updatedConfig.clientId, custom_mqtt_client_id.getValue(), sizeof(updatedConfig.clientId));
        }
        
        // Set updated MQTT config and save
        mqttManager.setConfig(updatedConfig);
        mqttManager.saveConfig();
        
        // Setup MQTT
        mqttManager.setMessageCallback(onMQTTMessage);
        mqttManager.setConnectCallback(onMQTTConnect);
        mqttManager.setDisconnectCallback(onMQTTDisconnect);
        
        if (mqttManager.begin()) {
            mqttManager.connect();
        }
        
    } else {
        Serial.println("WiFi connection failed - check configuration portal");
        Serial.println("Connect to 'ESP32-Knob-Setup' network and configure WiFi");
        Serial.println("Default password: smartknob123");
    }
}

void onWiFiConfigSaved() {
    Serial.println("WiFi and MQTT configuration saved!");
}

void onMQTTConnect() {
    Serial.println("MQTT connected - subscribing to topics");
    
    // Subscribe to device-specific topics
    String deviceTopic = mqttManager.getDeviceTopic("command");
    mqttManager.subscribe(deviceTopic.c_str());
    
    // Subscribe to general topics
    mqttManager.subscribe("energy/+");
    mqttManager.subscribe("weather/+");
    mqttManager.subscribe("house/+");
    
    // Publish online status
    String statusTopic = mqttManager.getDeviceTopic("status");
    mqttManager.publish(statusTopic.c_str(), "online", true);
}

void onMQTTDisconnect() {
    Serial.println("MQTT disconnected");
}

void onMQTTMessage(char* topic, uint8_t* payload, unsigned int length) {
    // Convert payload to string
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    Serial.printf("MQTT Message [%s]: %s\n", topic, message);
    
    // Parse JSON if applicable
    JsonDocument doc;
    if (mqttManager.parseJsonMessage(message, length, doc)) {
        // Handle different topic types
        if (strstr(topic, "/energy/")) {
            // Handle energy data
            float power = mqttManager.extractFloatFromJson(doc, "power", 0.0);
            float energy = mqttManager.extractFloatFromJson(doc, "energy", 0.0);
            Serial.printf("Energy data - Power: %.2f W, Energy: %.2f kWh\n", power, energy);
            
        } else if (strstr(topic, "/weather/")) {
            // Handle weather data
            float temp = mqttManager.extractFloatFromJson(doc, "temperature", 0.0);
            int humidity = mqttManager.extractIntFromJson(doc, "humidity", 0);
            Serial.printf("Weather data - Temp: %.1f°C, Humidity: %d%%\n", temp, humidity);
            
        } else if (strstr(topic, "/house/")) {
            // Handle house automation data
            String room = mqttManager.extractStringFromJson(doc, "room", "unknown");
            String device = mqttManager.extractStringFromJson(doc, "device", "unknown");
            String state = mqttManager.extractStringFromJson(doc, "state", "unknown");
            Serial.printf("House data - Room: %s, Device: %s, State: %s\n", 
                         room.c_str(), device.c_str(), state.c_str());
            
        } else if (strstr(topic, "/command")) {
            // Handle device commands
            String command = mqttManager.extractStringFromJson(doc, "command", "");
            if (command == "reset_wifi") {
                wifiManager.reset();
                ESP.restart();
            } else if (command == "restart") {
                ESP.restart();
            } else if (command == "status") {
                // Publish status
                JsonDocument statusDoc;
                statusDoc["uptime"] = millis();
                statusDoc["free_heap"] = ESP.getFreeHeap();
                statusDoc["wifi_rssi"] = wifiManager.getRSSI();
                statusDoc["mqtt_connected"] = mqttManager.connected();
                
                String statusTopic = mqttManager.getDeviceTopic("status");
                mqttManager.publishJson(statusTopic.c_str(), statusDoc);
            }
        }
    } else {
        // Handle non-JSON messages
        Serial.printf("Non-JSON message: %s\n", message);
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

// ================================
// ROTARY ENCODER HANDLING (Volos's Multi-core Architecture)
// ================================

// Volos-style encoder callbacks
static void _knob_left_cb(void *arg, void *data)
{
    uint8_t eventBits_ = 0;
    SET_BIT(eventBits_, 0);
    xEventGroupSetBits(knob_even_, eventBits_);
}

static void _knob_right_cb(void *arg, void *data)
{
    uint8_t eventBits_ = 0;
    SET_BIT(eventBits_, 1);
    xEventGroupSetBits(knob_even_, eventBits_);
}

// Encoder task (runs on separate core)
static void user_encoder_loop_task(void *arg)
{
    for(;;)
    {
        EventBits_t even = xEventGroupWaitBits(knob_even_, BIT_EVEN_ALL, pdTRUE, pdFALSE, pdMS_TO_TICKS(5000));
        
        if(READ_BIT(even, 0))  // Left rotation
        { 
            if (xSemaphoreTake(mutex, portMAX_DELAY)) { 
                // Counter-clockwise rotation
                currentScreen = (ScreenType)((currentScreen - 1 + SCREEN_COUNT) % SCREEN_COUNT);
                switchToScreen(currentScreen);
                Serial.printf("Encoder CCW -> Screen: %d\n", currentScreen);
                xSemaphoreGive(mutex); 
            }
        }
        
        if(READ_BIT(even, 1))  // Right rotation
        {
            if (xSemaphoreTake(mutex, portMAX_DELAY)) { 
                // Clockwise rotation
                currentScreen = (ScreenType)((currentScreen + 1) % SCREEN_COUNT);
                switchToScreen(currentScreen);
                Serial.printf("Encoder CW -> Screen: %d\n", currentScreen);
                xSemaphoreGive(mutex); 
            }
        }
    }
}

void handleEncoder() {
    // No longer needed - handled by the encoder task
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
    
    // Initialize Volos's encoder system (exact implementation)
    mutex = xSemaphoreCreateMutex();
    knob_even_ = xEventGroupCreate();
    
    // Create knob with Volos's configuration
    knob_config_t cfg = {
        .gpio_encoder_a = EXAMPLE_ENCODER_ECA_PIN,
        .gpio_encoder_b = EXAMPLE_ENCODER_ECB_PIN,
    };
    s_knob = iot_knob_create(&cfg);
    
    if (s_knob) {
        // Register Volos-style callbacks
        iot_knob_register_cb(s_knob, KNOB_LEFT, _knob_left_cb, NULL);
        iot_knob_register_cb(s_knob, KNOB_RIGHT, _knob_right_cb, NULL);
        
        // Start encoder task on separate core
        xTaskCreate(user_encoder_loop_task, "user_encoder_loop_task", 3000, NULL, 2, NULL);
        
        Serial.println("Volos encoder system initialized successfully");
    } else {
        Serial.println("Failed to initialize Volos encoder system");
    }
    
    Serial.println("Rotary encoder initialized (Volos multi-core, no button)");
    
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
    
    // Initialize WiFi and MQTT using new managers
    setupWiFiAndMQTT();
    
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
    
    // Handle MQTT connection and messages
    mqttManager.loop();
    
    // Small delay to prevent watchdog issues
    delay(5);
}
