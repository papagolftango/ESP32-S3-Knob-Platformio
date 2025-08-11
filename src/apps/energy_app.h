#ifndef ENERGY_APP_H
#define ENERGY_APP_H

#include "base_app.h"
#include <ArduinoJson.h>

class EnergyApp : public BaseApp {
private:
    lv_obj_t* power_label = nullptr;
    lv_obj_t* usage_label = nullptr;
    lv_obj_t* cost_label = nullptr;
    
    // Energy data
    float current_power = 0.0;      // kW
    float daily_usage = 0.0;        // kWh
    float cost_per_kwh = 0.275;     // £ per kWh (UK average)
    float daily_cost = 0.0;         // £
    
    unsigned long last_update = 0;
    const unsigned long UPDATE_INTERVAL = 5000; // Update every 5 seconds
    
    void updateDisplay();
    void calculateCost();
    
public:
    // Core app lifecycle
    bool init() override;
    void deinit() override;
    lv_obj_t* createScreen() override;
    void onEnter() override;
    void onExit() override;
    void update() override;
    
    // App metadata
    const char* getName() override { return "Energy"; }
    const char* getIcon() override { return "⚡"; }
    ScreenType getScreenType() override { return SCREEN_ENERGY; }
    
    // Data updates
    void onMQTTMessage(const String& topic, const String& payload) override;
    
    // Business logic
    void setPower(float power_kw);
    void setDailyUsage(float usage_kwh);
    void setCostPerKwh(float cost);
    
    // Getters
    float getPower() const { return current_power; }
    float getDailyUsage() const { return daily_usage; }
    float getDailyCost() const { return daily_cost; }
};

#endif // ENERGY_APP_H
