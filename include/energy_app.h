#ifndef ENERGY_APP_H
#define ENERGY_APP_H

#include "base_app.h"

class EnergyApp : public BaseApp {
public:
    bool init() override;
    void deinit() override;
    lv_obj_t* createScreen() override;
    void onEnter() override;
    void onExit() override;
    void update() override;
    const char* getName() override { return "Energy"; }
};

#endif // ENERGY_APP_H
