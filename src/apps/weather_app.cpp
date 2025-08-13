#include "weather_app.h"
#include "app_manager.h"

bool WeatherApp::init() {
    if (initialized) return true;
    
    // Register this app with the AppManager
    appManager.registerApp(this);
    
    screen = createScreen();
    initialized = true;
    return true;
}

void WeatherApp::deinit() {
    if (screen) { lv_obj_del(screen); screen = nullptr; }
    initialized = false;
}

lv_obj_t* WeatherApp::createScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_t* title = lv_label_create(scr);
    lv_label_set_text(title, "Weather");
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    return scr;
}

void WeatherApp::onEnter() { if (screen) lv_scr_load(screen); }
void WeatherApp::onExit() {}
void WeatherApp::update() {}
