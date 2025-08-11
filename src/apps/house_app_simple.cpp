#include "house_app.h"
#include "../app_manager.h"

bool HouseApp::init() {
    if (initialized) return true;
    appManager.registerApp(this);
    screen = createScreen();
    initialized = true;
    return true;
}

void HouseApp::deinit() {
    if (screen) { lv_obj_del(screen); screen = nullptr; }
    initialized = false;
}

lv_obj_t* HouseApp::createScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_t* title = lv_label_create(scr);
    lv_label_set_text(title, "House");
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    return scr;
}

void HouseApp::onEnter() { if (screen) lv_scr_load(screen); }
void HouseApp::onExit() {}
void HouseApp::update() {}
