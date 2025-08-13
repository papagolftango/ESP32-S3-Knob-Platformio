#ifndef CLOCK_APP_H
#define CLOCK_APP_H

#include "base_app.h"

class ClockApp : public BaseApp {
public:
    bool init() override;
    void deinit() override;
    lv_obj_t* createScreen() override;
    void onEnter() override;
    void onExit() override;
    void update() override;
    const char* getName() override { return "Clock"; }
};

#endif // CLOCK_APP_H
