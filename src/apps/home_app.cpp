#include "home_app.h"
#include "../services/app_manager.h"

HomeApp::HomeApp() {
    // Constructor - does nothing now since we register during init()
}

bool HomeApp::init() {
    if (initialized) return true;
    
    // Register this app with the AppManager
    appManager.registerApp(this);
    
    screen = createScreen();
    initialized = true;
    return true;
}

void HomeApp::deinit() {
    if (screen) { lv_obj_del(screen); screen = nullptr; }
    initialized = false;
}

lv_obj_t* HomeApp::createScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_t* title = lv_label_create(scr);
    lv_label_set_text(title, "Home");
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    return scr;
}

void HomeApp::onEnter() { if (screen) lv_scr_load(screen); }
void HomeApp::onExit() {}
void HomeApp::update() {}
