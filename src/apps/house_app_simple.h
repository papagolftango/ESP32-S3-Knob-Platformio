#ifndef HOUSE_APP_H
#define HOUSE_APP_H

#include "base_app.h"

class HouseApp : public BaseApp {
public:
    bool init() override;
    void deinit() override;
    lv_obj_t* createScreen() override;
    void onEnter() override;
    void onExit() override;
    void update() override;
    const char* getName() override { return "House"; }
};

#endif // HOUSE_APP_H
