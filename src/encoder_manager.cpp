#include "encoder_manager.h"
#include "lcd_config.h"

// Volos-style bit manipulation macros
#define SET_BIT(reg,bit) (reg |= ((uint32_t)0x01<<bit))
#define CLEAR_BIT(reg,bit) (reg &= (~((uint32_t)0x01<<bit)))
#define READ_BIT(reg,bit) (((uint32_t)reg>>bit) & 0x01)
#define BIT_EVEN_ALL (0x00ffffff)

// Static member definitions
EventGroupHandle_t EncoderManager::knob_even_ = NULL;
knob_handle_t EncoderManager::s_knob = 0;
SemaphoreHandle_t EncoderManager::mutex = NULL;

// Navigation state
ScreenType EncoderManager::currentScreen = SCREEN_HOME;
SettingsOption EncoderManager::currentSettingsOption = SETTINGS_WIFI_RESET;
bool EncoderManager::inSettingsMenu = false;
unsigned long EncoderManager::settingsSelectTime = 0;

// Callbacks
ScreenChangeCallback EncoderManager::screenChangeCallback = nullptr;
SettingsNavigationCallback EncoderManager::settingsNavigationCallback = nullptr;
SettingsExecuteCallback EncoderManager::settingsExecuteCallback = nullptr;

// Long hold detection
unsigned long EncoderManager::rotationStartTime = 0;
bool EncoderManager::rotationHeld = false;

bool EncoderManager::begin() {
    Serial.println("Initializing Volos encoder system...");
    
    // Initialize Volos's encoder system (exact implementation)
    mutex = xSemaphoreCreateMutex();
    if (!mutex) {
        Serial.println("Failed to create encoder mutex");
        return false;
    }
    
    knob_even_ = xEventGroupCreate();
    if (!knob_even_) {
        Serial.println("Failed to create encoder event group");
        return false;
    }
    
    // Create knob with Volos's configuration
    knob_config_t cfg = {
        .gpio_encoder_a = EXAMPLE_ENCODER_ECA_PIN,
        .gpio_encoder_b = EXAMPLE_ENCODER_ECB_PIN,
    };
    s_knob = iot_knob_create(&cfg);
    
    if (!s_knob) {
        Serial.println("Failed to create Volos knob instance");
        return false;
    }
    
    // Register Volos-style callbacks
    if (iot_knob_register_cb(s_knob, KNOB_LEFT, _knob_left_cb, NULL) != ESP_OK) {
        Serial.println("Failed to register left callback");
        return false;
    }
    
    if (iot_knob_register_cb(s_knob, KNOB_RIGHT, _knob_right_cb, NULL) != ESP_OK) {
        Serial.println("Failed to register right callback");
        return false;
    }
    
    // Start encoder task on separate core
    BaseType_t result = xTaskCreate(
        user_encoder_loop_task,
        "user_encoder_loop_task",
        3000,
        NULL,
        2,  // Priority
        NULL
    );
    
    if (result != pdPASS) {
        Serial.println("Failed to create encoder task");
        return false;
    }
    
    Serial.println("Volos encoder system initialized successfully");
    Serial.printf("Encoder pins: GPIO %d (A) and GPIO %d (B)\n", 
                  EXAMPLE_ENCODER_ECA_PIN, EXAMPLE_ENCODER_ECB_PIN);
    
    return true;
}

void EncoderManager::end() {
    if (s_knob) {
        iot_knob_delete(s_knob);
        s_knob = 0;
    }
    
    if (mutex) {
        vSemaphoreDelete(mutex);
        mutex = NULL;
    }
    
    if (knob_even_) {
        vEventGroupDelete(knob_even_);
        knob_even_ = NULL;
    }
    
    Serial.println("Encoder system deinitialized");
}

void EncoderManager::setCurrentScreen(ScreenType screen) {
    if (screen < SCREEN_COUNT) {
        currentScreen = screen;
        inSettingsMenu = (screen == SCREEN_SETTINGS);
        if (inSettingsMenu) {
            currentSettingsOption = SETTINGS_WIFI_RESET;
        }
    }
}

ScreenType EncoderManager::getCurrentScreen() {
    return currentScreen;
}

void EncoderManager::enterSettingsMenu() {
    inSettingsMenu = true;
    currentSettingsOption = SETTINGS_WIFI_RESET;
    currentScreen = SCREEN_SETTINGS;
}

void EncoderManager::exitSettingsMenu() {
    inSettingsMenu = false;
    currentScreen = SCREEN_HOME;
}

bool EncoderManager::isInSettingsMenu() {
    return inSettingsMenu;
}

void EncoderManager::setScreenChangeCallback(ScreenChangeCallback callback) {
    screenChangeCallback = callback;
}

void EncoderManager::setSettingsNavigationCallback(SettingsNavigationCallback callback) {
    settingsNavigationCallback = callback;
}

void EncoderManager::setSettingsExecuteCallback(SettingsExecuteCallback callback) {
    settingsExecuteCallback = callback;
}

// Volos-style encoder callbacks
void EncoderManager::_knob_left_cb(void *arg, void *data) {
    uint8_t eventBits_ = 0;
    SET_BIT(eventBits_, 0);
    xEventGroupSetBits(knob_even_, eventBits_);
}

void EncoderManager::_knob_right_cb(void *arg, void *data) {
    uint8_t eventBits_ = 0;
    SET_BIT(eventBits_, 1);
    xEventGroupSetBits(knob_even_, eventBits_);
}

// Encoder task (runs on separate core)
void EncoderManager::user_encoder_loop_task(void *arg) {
    for(;;) {
        EventBits_t even = xEventGroupWaitBits(knob_even_, BIT_EVEN_ALL, pdTRUE, pdFALSE, pdMS_TO_TICKS(5000));
        
        if(READ_BIT(even, 0)) {  // Left rotation
            handleLeftRotation();
        }
        
        if(READ_BIT(even, 1)) {  // Right rotation
            handleRightRotation();
        }
        
        // Check for long hold detection
        checkLongHold();
    }
}

void EncoderManager::handleLeftRotation() {
    if (xSemaphoreTake(mutex, portMAX_DELAY)) { 
        rotationStartTime = millis();
        rotationHeld = false;
        
        if (inSettingsMenu) {
            // Navigate settings menu
            currentSettingsOption = (SettingsOption)((currentSettingsOption - 1 + SETTINGS_COUNT) % SETTINGS_COUNT);
            Serial.printf("Settings option: %d\n", currentSettingsOption);
            
            if (settingsNavigationCallback) {
                settingsNavigationCallback(currentSettingsOption);
            }
        } else {
            // Navigate screens
            currentScreen = (ScreenType)((currentScreen - 1 + SCREEN_COUNT) % SCREEN_COUNT);
            Serial.printf("Encoder CCW -> Screen: %d\n", currentScreen);
            
            if (currentScreen == SCREEN_SETTINGS) {
                inSettingsMenu = true;
                currentSettingsOption = SETTINGS_WIFI_RESET;
            }
            
            if (screenChangeCallback) {
                screenChangeCallback(currentScreen);
            }
        }
        xSemaphoreGive(mutex); 
    }
}

void EncoderManager::handleRightRotation() {
    if (xSemaphoreTake(mutex, portMAX_DELAY)) { 
        rotationStartTime = millis();
        rotationHeld = false;
        
        if (inSettingsMenu) {
            // Navigate settings menu
            currentSettingsOption = (SettingsOption)((currentSettingsOption + 1) % SETTINGS_COUNT);
            Serial.printf("Settings option: %d\n", currentSettingsOption);
            
            if (settingsNavigationCallback) {
                settingsNavigationCallback(currentSettingsOption);
            }
        } else {
            // Navigate screens
            currentScreen = (ScreenType)((currentScreen + 1) % SCREEN_COUNT);
            Serial.printf("Encoder CW -> Screen: %d\n", currentScreen);
            
            if (currentScreen == SCREEN_SETTINGS) {
                inSettingsMenu = true;
                currentSettingsOption = SETTINGS_WIFI_RESET;
            }
            
            if (screenChangeCallback) {
                screenChangeCallback(currentScreen);
            }
        }
        xSemaphoreGive(mutex); 
    }
}

void EncoderManager::checkLongHold() {
    // Check for long hold (3 seconds) to execute settings action
    if (inSettingsMenu && rotationStartTime > 0 && !rotationHeld) {
        if ((millis() - rotationStartTime) > LONG_HOLD_MS) {
            rotationHeld = true;
            Serial.println("Long hold detected - executing settings action");
            
            if (settingsExecuteCallback) {
                settingsExecuteCallback(currentSettingsOption);
            }
            rotationStartTime = 0;
        }
    }
    
    // Reset hold timer if no activity for 5 seconds
    if (rotationStartTime > 0 && (millis() - rotationStartTime) > RESET_TIMEOUT_MS) {
        rotationStartTime = 0;
        rotationHeld = false;
    }
}

void EncoderManager::simulateLeftRotation() {
    Serial.println("Simulating left rotation");
    handleLeftRotation();
}

void EncoderManager::simulateRightRotation() {
    Serial.println("Simulating right rotation");
    handleRightRotation();
}

void EncoderManager::simulateLongHold() {
    if (inSettingsMenu) {
        Serial.println("Simulating long hold");
        if (settingsExecuteCallback) {
            settingsExecuteCallback(currentSettingsOption);
        }
    }
}

void EncoderManager::printStatus() {
    Serial.println("\n=== ENCODER STATUS ===");
    Serial.printf("Initialized: %s\n", isInitialized() ? "Yes" : "No");
    Serial.printf("Current Screen: %d\n", currentScreen);
    Serial.printf("In Settings Menu: %s\n", inSettingsMenu ? "Yes" : "No");
    
    if (inSettingsMenu) {
        Serial.printf("Settings Option: %d\n", currentSettingsOption);
    }
    
    Serial.printf("Encoder pins: GPIO %d (A), GPIO %d (B)\n", 
                  EXAMPLE_ENCODER_ECA_PIN, EXAMPLE_ENCODER_ECB_PIN);
    Serial.printf("Long hold threshold: %lu ms\n", LONG_HOLD_MS);
    Serial.println("====================\n");
}

bool EncoderManager::isInitialized() {
    return (s_knob != 0 && mutex != NULL && knob_even_ != NULL);
}
