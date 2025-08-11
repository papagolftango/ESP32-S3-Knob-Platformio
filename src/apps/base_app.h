#ifndef BASE_APP_H
#define BASE_APP_H

#include <Arduino.h>
#include <lvgl.h>
#include "encoder_manager.h"

// Base interface for all apps
class BaseApp {
public:
    virtual ~BaseApp() = default;
    
    // Core app lifecycle
    virtual bool init() = 0;                     // Initialize app resources
    virtual void deinit() = 0;                   // Clean up app resources
    virtual lv_obj_t* createScreen() = 0;       // Create and return the LVGL screen object
    virtual void onEnter() = 0;                  // Called when app becomes active
    virtual void onExit() = 0;                   // Called when app becomes inactive
    virtual void update() = 0;                   // Called periodically to update app logic
    
    // App metadata
    virtual const char* getName() = 0;          // Get app display name
    virtual const char* getIcon() = 0;          // Get app icon (emoji/text)
    virtual ScreenType getScreenType() = 0;     // Get associated screen type
    
    // Screen management
    virtual lv_obj_t* getScreen() { return screen; }
    virtual bool isInitialized() const { return initialized; }
    virtual bool isActive() const { return active; }
    
    // Data update methods (override if app needs external data)
    virtual void onMQTTMessage(const String& topic, const String& payload) {}
    virtual void onWiFiStatusChange(bool connected) {}
    virtual void onTimeUpdate() {}
    
protected:
    lv_obj_t* screen = nullptr;
    bool initialized = false;
    bool active = false;
    unsigned long last_update = 0;
    
    // Helper methods for common UI elements
    lv_obj_t* createTitle(const char* text, uint32_t color = 0xFFFFFF, int y_offset = 20);
    lv_obj_t* createLabel(const char* text, uint32_t color = 0xFFFFFF, lv_align_t align = LV_ALIGN_CENTER, int x_offset = 0, int y_offset = 0);
    lv_obj_t* createStatusIndicator(const char* text, bool status, int y_offset = 0);
    void setBackgroundColor(uint32_t color);
};

// Inline helper implementations
inline lv_obj_t* BaseApp::createTitle(const char* text, uint32_t color, int y_offset) {
    if (!screen) return nullptr;
    
    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, text);
    lv_obj_set_style_text_color(title, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, y_offset);
    return title;
}

inline lv_obj_t* BaseApp::createLabel(const char* text, uint32_t color, lv_align_t align, int x_offset, int y_offset) {
    if (!screen) return nullptr;
    
    lv_obj_t* label = lv_label_create(screen);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_align(label, align, x_offset, y_offset);
    return label;
}

inline lv_obj_t* BaseApp::createStatusIndicator(const char* text, bool status, int y_offset) {
    if (!screen) return nullptr;
    
    lv_obj_t* indicator = lv_label_create(screen);
    lv_label_set_text(indicator, text);
    lv_obj_set_style_text_color(indicator, lv_color_hex(status ? 0x00FF00 : 0xFF0000), 0);
    lv_obj_align(indicator, LV_ALIGN_CENTER, 0, y_offset);
    return indicator;
}

inline void BaseApp::setBackgroundColor(uint32_t color) {
    if (screen) {
        lv_obj_set_style_bg_color(screen, lv_color_hex(color), 0);
    }
}

#endif // BASE_APP_H
