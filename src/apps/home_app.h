#ifndef HOME_APP_H
#define HOME_APP_H

#include "base_app.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"

class HomeApp : public BaseApp {
private:
    lv_obj_t* wifi_status_label = nullptr;
    lv_obj_t* mqtt_status_label = nullptr;
    lv_obj_t* nav_hint_label = nullptr;
    
    unsigned long last_update = 0;
    const unsigned long UPDATE_INTERVAL = 2000; // Update every 2 seconds
    
public:
    // Core app lifecycle
    bool init() override;
    void deinit() override;
    lv_obj_t* createScreen() override;
    void onEnter() override;
    void onExit() override;
    void update() override;
    
    // App metadata
    const char* getName() override { return "Home"; }
    const char* getIcon() override { return "🏠"; }
    ScreenType getScreenType() override { return SCREEN_HOME; }
    
    // Data updates
    void onWiFiStatusChange(bool connected) override;
    void updateStatus();
};

#endif // HOME_APP_H
