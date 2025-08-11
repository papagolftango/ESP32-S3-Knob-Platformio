#include "clock_app.h"
#include "../app_manager.h"

bool ClockApp::init() {
    if (initialized) return true;
    appManager.registerApp(this);
    screen = createScreen();
    initialized = true;
    return true;
}

void ClockApp::deinit() {
    if (screen) { lv_obj_del(screen); screen = nullptr; }
    initialized = false;
}

lv_obj_t* ClockApp::createScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_t* title = lv_label_create(scr);
    lv_label_set_text(title, "Clock");
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    return scr;
}

void ClockApp::onEnter() { if (screen) lv_scr_load(screen); }
void ClockApp::onExit() {}
void ClockApp::update() {}
