#ifndef BASE_APP_H
#define BASE_APP_H

#include <lvgl.h>
#include <Arduino.h>

// Super simple base class for all apps
class BaseApp {
protected:
    lv_obj_t* screen = nullptr;
    bool initialized = false;
    
public:
    virtual ~BaseApp() = default;
    
    // Core app lifecycle - simple and clean
    virtual bool init() = 0;                     // Initialize app and register with AppManager
    virtual void deinit() = 0;                   // Clean up app resources
    virtual lv_obj_t* createScreen() = 0;       // Create and return the LVGL screen object
    virtual void onEnter() = 0;                  // Called when app becomes active
    virtual void onExit() = 0;                   // Called when app becomes inactive
    virtual void update() = 0;                   // Called periodically to update app logic
    
    // App metadata
    virtual const char* getName() = 0;          // App name for logging
    
    // Screen management
    virtual lv_obj_t* getScreen() { return screen; }
    virtual bool isInitialized() const { return initialized; }
};

#endif // BASE_APP_H
