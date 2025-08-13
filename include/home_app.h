#ifndef HOME_APP_H
#define HOME_APP_H

#include "base_app.h"

class HomeApp : public BaseApp {
public:
    // Constructor - registers with AppManager
    HomeApp();
    
    // Core app lifecycle
    bool init() override;
    void deinit() override;
    lv_obj_t* createScreen() override;
    void onEnter() override;
    void onExit() override;
    void update() override;
    
    // App metadata
    const char* getName() override { return "Home"; }
};

#endif // HOME_APP_H
