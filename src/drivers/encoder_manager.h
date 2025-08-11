#ifndef ENCODER_MANAGER_H
#define ENCODER_MANAGER_H

#include <Arduino.h>
#include "bidi_switch_knob.h"

// Simple callback type - just direction
typedef void (*EncoderChangeCallback)(int direction);  // -1 for left, +1 for right

// Simple Encoder Manager - Volos style
class EncoderManager {
private:
    // Volos encoder system core
    static EventGroupHandle_t knob_even_;
    static knob_handle_t s_knob;
    static SemaphoreHandle_t mutex;
    
    // Simple callback
    static EncoderChangeCallback changeCallback;
    
    // Volos-style callback functions
    static void _knob_left_cb(void *arg, void *data);
    static void _knob_right_cb(void *arg, void *data);
    static void user_encoder_loop_task(void *arg);
    
public:
    // Core functionality
    static bool begin();
    static void end();
    
    // Register callback (default to AppManager)
    static void setChangeCallback(EncoderChangeCallback callback);
    
    // Status
    static bool isInitialized();
};

#endif // ENCODER_MANAGER_H
