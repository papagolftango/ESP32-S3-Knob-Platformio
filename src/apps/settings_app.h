#ifndef SETTINGS_APP_H
#define SETTINGS_APP_H

#include "base_app.h"

class SettingsApp : public BaseApp {
private:
    static const unsigned long UPDATE_INTERVAL = 2000; // Update every 2 seconds
    
    // UI elements
    lv_obj_t* status_label = nullptr;
    lv_obj_t* wifi_label = nullptr;
    lv_obj_t* mqtt_label = nullptr;
    lv_obj_t* action_label = nullptr;
    
    // Settings state
    bool wifi_connected = false;
    bool mqtt_connected = false;
    String current_ssid = "";
    String last_action = "";
    unsigned long action_time = 0;
    
    // Menu navigation
    int current_option = 0;
    const int num_options = 5;
    const char* options[5] = {
        "WiFi Reset",
        "MQTT Reset", 
        "Factory Reset",
        "Restart",
        "Exit"
    };
    
public:
    SettingsApp() : BaseApp() {}
    virtual ~SettingsApp() {}
    
    // BaseApp interface implementation
    bool init() override;
    void deinit() override;
    lv_obj_t* createScreen() override;
    void onEnter() override;
    void onExit() override;
    void update() override;
    void onMQTTMessage(const String& topic, const String& payload) override;
    
    // Settings-specific functionality
    void executeWiFiReset();
    void executeMQTTReset();
    void executeFactoryReset();
    void executeRestart();
    void executeExit();
    
private:
    void updateDisplay();
    void updateConnectionStatus();
    void highlightCurrentOption();
    String getOptionColor(int option_index);
    void showActionFeedback(const String& action);
};

#endif // SETTINGS_APP_H
