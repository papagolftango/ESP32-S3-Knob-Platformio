#ifndef WEATHER_APP_H
#define WEATHER_APP_H

#include "base_app.h"
#include <ArduinoJson.h>

class WeatherApp : public BaseApp {
private:
    lv_obj_t* temp_label = nullptr;
    lv_obj_t* humidity_label = nullptr;
    lv_obj_t* condition_label = nullptr;
    
    // Weather data
    float temperature = 22.3;        // °C
    int humidity = 65;               // %
    String condition = "Sunny";      // Weather condition
    String condition_icon = "☀️";    // Weather icon
    
    unsigned long last_update = 0;
    const unsigned long UPDATE_INTERVAL = 30000; // Update every 30 seconds
    
    void updateDisplay();
    String getWeatherIcon(const String& condition);
    
public:
    // Core app lifecycle
    bool init() override;
    void deinit() override;
    lv_obj_t* createScreen() override;
    void onEnter() override;
    void onExit() override;
    void update() override;
    
    // App metadata
    const char* getName() override { return "Weather"; }
    const char* getIcon() override { return "🌤️"; }
    ScreenType getScreenType() override { return SCREEN_WEATHER; }
    
    // Data updates
    void onMQTTMessage(const String& topic, const String& payload) override;
    
    // Business logic
    void setTemperature(float temp_c);
    void setHumidity(int humidity_percent);
    void setCondition(const String& weather_condition);
    
    // Getters
    float getTemperature() const { return temperature; }
    int getHumidity() const { return humidity; }
    String getCondition() const { return condition; }
};

#endif // WEATHER_APP_H
