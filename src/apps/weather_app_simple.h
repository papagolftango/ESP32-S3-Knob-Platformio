#ifndef WEATHER_APP_H
#define WEATHER_APP_H

#include "base_app.h"

class WeatherApp : public BaseApp {
public:
    bool init() override;
    void deinit() override;
    lv_obj_t* createScreen() override;
    void onEnter() override;
    void onExit() override;
    void update() override;
    const char* getName() override { return "Weather"; }
};

#endif // WEATHER_APP_H
