#ifndef ENCODER_MANAGER_H
#define ENCODER_MANAGER_H

#include <Arduino.h>
#include "bidi_switch_knob.h"

// Forward declarations for screen management types
enum ScreenType {
    SCREEN_HOME,
    SCREEN_ENERGY,
    SCREEN_WEATHER,
    SCREEN_HOUSE,
    SCREEN_CLOCK,
    SCREEN_SETTINGS,
    SCREEN_COUNT
};

enum SettingsOption {
    SETTINGS_WIFI_RESET,
    SETTINGS_MQTT_RESET,
    SETTINGS_FACTORY_RESET,
    SETTINGS_RESTART,
    SETTINGS_EXIT,
    SETTINGS_COUNT
};

// Callback function types
typedef void (*ScreenChangeCallback)(ScreenType newScreen);
typedef void (*SettingsNavigationCallback)(SettingsOption newOption);
typedef void (*SettingsExecuteCallback)(SettingsOption option);

// Encoder Manager class for handling Volos's multi-core encoder system
class EncoderManager {
private:
    // Volos encoder system variables
    static EventGroupHandle_t knob_even_;
    static knob_handle_t s_knob;
    static SemaphoreHandle_t mutex;
    
    // Navigation state
    static ScreenType currentScreen;
    static SettingsOption currentSettingsOption;
    static bool inSettingsMenu;
    static unsigned long settingsSelectTime;
    
    // Callbacks
    static ScreenChangeCallback screenChangeCallback;
    static SettingsNavigationCallback settingsNavigationCallback;
    static SettingsExecuteCallback settingsExecuteCallback;
    
    // Long hold detection
    static unsigned long rotationStartTime;
    static bool rotationHeld;
    static const unsigned long LONG_HOLD_MS = 3000;
    static const unsigned long RESET_TIMEOUT_MS = 5000;
    
    // Static callback functions for Volos system
    static void _knob_left_cb(void *arg, void *data);
    static void _knob_right_cb(void *arg, void *data);
    static void user_encoder_loop_task(void *arg);
    
    // Internal navigation helpers
    static void handleLeftRotation();
    static void handleRightRotation();
    static void checkLongHold();
    
public:
    // Initialization and control
    static bool begin();
    static void end();
    
    // Navigation state management
    static void setCurrentScreen(ScreenType screen);
    static ScreenType getCurrentScreen();
    static void enterSettingsMenu();
    static void exitSettingsMenu();
    static bool isInSettingsMenu();
    
    // Callback registration
    static void setScreenChangeCallback(ScreenChangeCallback callback);
    static void setSettingsNavigationCallback(SettingsNavigationCallback callback);
    static void setSettingsExecuteCallback(SettingsExecuteCallback callback);
    
    // Manual navigation (for testing/debugging)
    static void simulateLeftRotation();
    static void simulateRightRotation();
    static void simulateLongHold();
    
    // Status information
    static void printStatus();
    static bool isInitialized();
};

#endif // ENCODER_MANAGER_H
