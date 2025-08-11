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

// Simple callback
EncoderChangeCallback EncoderManager::changeCallback = nullptr;

bool EncoderManager::begin() {
    // Initialize Volos's encoder system
    mutex = xSemaphoreCreateMutex();
    if (!mutex) {
        return false;
    }
    
    knob_even_ = xEventGroupCreate();
    if (!knob_even_) {
        return false;
    }
    
    // Create knob with Volos's GPIO configuration
    knob_config_t cfg = {
        .gpio_encoder_a = EXAMPLE_ENCODER_ECA_PIN,
        .gpio_encoder_b = EXAMPLE_ENCODER_ECB_PIN,
    };
    s_knob = iot_knob_create(&cfg);
    
    if (!s_knob) {
        return false;
    }
    
    // Register Volos-style callbacks
    if (iot_knob_register_cb(s_knob, KNOB_LEFT, _knob_left_cb, NULL) != ESP_OK) {
        return false;
    }
    
    if (iot_knob_register_cb(s_knob, KNOB_RIGHT, _knob_right_cb, NULL) != ESP_OK) {
        return false;
    }
    
    // Start encoder task
    BaseType_t result = xTaskCreate(
        user_encoder_loop_task,
        "encoder_task",
        3000,
        NULL,
        2,
        NULL
    );
    
    return result == pdPASS;
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
}

void EncoderManager::setChangeCallback(EncoderChangeCallback callback) {
    changeCallback = callback;
}

bool EncoderManager::isInitialized() {
    return s_knob != 0;
}

// Volos-style callbacks - simple and clean
void EncoderManager::_knob_left_cb(void *arg, void *data) {
    if (changeCallback) {
        changeCallback(-1);  // Left = -1
    }
}

void EncoderManager::_knob_right_cb(void *arg, void *data) {
    if (changeCallback) {
        changeCallback(1);   // Right = +1
    }
}

// Volos encoder task - just handles the event loop
void EncoderManager::user_encoder_loop_task(void *arg) {
    EventBits_t event_bit;
    
    while (1) {
        event_bit = xEventGroupWaitBits(knob_even_, BIT_EVEN_ALL, true, false, portMAX_DELAY);
        
        if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
            // Just handle the event bits - no click detection needed
            xSemaphoreGive(mutex);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
