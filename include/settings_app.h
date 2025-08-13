#ifndef SETTINGS_APP_H
#define SETTINGS_APP_H

#include "base_app.h"

class SettingsApp : public BaseApp {
public:
    bool init() override;
    void deinit() override;
    lv_obj_t* createScreen() override;
    void onEnter() override;
    void onExit() override;
    void update() override;
    const char* getName() override { return "Settings"; }
};

#endif // SETTINGS_APP_H
