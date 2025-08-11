#include "app_manager.h"

// Global app manager instance
AppManager appManager;

AppManager::AppManager() {
    // Simple constructor
}

void AppManager::registerApp(BaseApp* app) {
    if (!app) return;
    
    AppNode* newNode = new AppNode(app);
    
    if (!currentNode) {
        // First app - create circular list with one node
        currentNode = newNode;
        newNode->next = newNode; // Point to itself
        
        // Initialize the first app
        switchToCurrentApp();
    } else {
        // Insert new node into circular list
        newNode->next = currentNode->next;
        currentNode->next = newNode;
    }
    
    appCount++;
    Serial.printf("App '%s' registered. Total apps: %d\n", app->getName(), appCount);
}

void AppManager::onEncoderChange(int direction) {
    if (!currentNode) return;
    
    // Deinit current app
    if (currentNode->app) {
        currentNode->app->deinit();
    }
    
    // Move in circular list
    if (direction > 0) {
        // Forward - next app
        currentNode = currentNode->next;
    } else if (direction < 0) {
        // Backward - find previous app by traversing circle
        AppNode* prev = currentNode;
        while (prev->next != currentNode) {
            prev = prev->next;
        }
        currentNode = prev;
    }
    
    // Switch to new current app
    switchToCurrentApp();
}

void AppManager::update() {
    // Just update the current app
    if (currentNode && currentNode->app) {
        currentNode->app->update();
    }
}

void AppManager::switchToCurrentApp() {
    if (!currentNode || !currentNode->app) return;
    
    // Initialize and enter the current app
    if (currentNode->app->init()) {
        currentNode->app->onEnter();
        Serial.printf("Switched to app: %s\n", currentNode->app->getName());
    } else {
        Serial.printf("Failed to initialize app: %s\n", currentNode->app->getName());
    }
}
