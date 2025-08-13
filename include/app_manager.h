#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include "base_app.h"

// Node for circular linked list of apps
struct AppNode {
    BaseApp* app;
    AppNode* next;
    
    AppNode(BaseApp* a) : app(a), next(nullptr) {}
};

class AppManager {
private:
    // Circular linked list of apps
    AppNode* currentNode = nullptr;
    int appCount = 0;
    
public:
    AppManager();
    
    // App registration - apps call this during their init()
    void registerApp(BaseApp* app);
    
    // Focus management - called by encoder
    void onEncoderChange(int direction);
    
    // Lifecycle
    void update();
    
    // Current app access
    BaseApp* getCurrentApp() const { return currentNode ? currentNode->app : nullptr; }
    int getAppCount() const { return appCount; }
    
private:
    void switchToCurrentApp();
};

// Global app manager instance
extern AppManager appManager;

#endif // APP_MANAGER_H
