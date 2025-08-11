#ifndef HOUSE_APP_H
#define HOUSE_APP_H

#include "base_app.h"
#include <ArduinoJson.h>

class HouseApp : public BaseApp {
private:
    lv_obj_t* lights_label = nullptr;
    lv_obj_t* temp_label = nullptr;
    lv_obj_t* security_label = nullptr;
    
    // House automation data
    int lights_on = 5;               // Number of lights on
    int total_lights = 12;           // Total lights in house
    float indoor_temp = 21.0;        // °C
    bool security_armed = true;      // Security system status
    String security_status = "Armed"; // Security status text
    
    unsigned long last_update = 0;
    const unsigned long UPDATE_INTERVAL = 10000; // Update every 10 seconds
    
    void updateDisplay();
    String getSecurityIcon(bool armed);
    
public:
    // Core app lifecycle
    bool init() override;
    void deinit() override;
    lv_obj_t* createScreen() override;
    void onEnter() override;
    void onExit() override;
    void update() override;
    
    // App metadata
    const char* getName() override { return "House"; }
    const char* getIcon() override { return "🏠"; }
    ScreenType getScreenType() override { return SCREEN_HOUSE; }
    
    // Data updates
    void onMQTTMessage(const String& topic, const String& payload) override;
    
    // Business logic
    void setLightsOn(int count);
    void setTotalLights(int total);
    void setIndoorTemperature(float temp_c);
    void setSecurityStatus(bool armed, const String& status = "");
    
    // Getters
    int getLightsOn() const { return lights_on; }
    int getTotalLights() const { return total_lights; }
    float getIndoorTemperature() const { return indoor_temp; }
    bool isSecurityArmed() const { return security_armed; }
};

#endif // HOUSE_APP_H
