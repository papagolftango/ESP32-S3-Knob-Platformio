#include "settings_app.h"
#include "../app_manager.h"

bool SettingsApp::init() {
    if (initialized) return true;
    appManager.registerApp(this);
    screen = createScreen();
    initialized = true;
    return true;
}

void SettingsApp::deinit() {
    if (screen) { lv_obj_del(screen); screen = nullptr; }
    initialized = false;
}

lv_obj_t* SettingsApp::createScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_t* title = lv_label_create(scr);
    lv_label_set_text(title, "Settings");
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    return scr;
}

void SettingsApp::onEnter() { if (screen) lv_scr_load(screen); }
void SettingsApp::onExit() {}
void SettingsApp::update() {}
