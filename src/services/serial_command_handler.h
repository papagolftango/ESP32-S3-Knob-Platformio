#ifndef SERIAL_COMMAND_HANDLER_H
#define SERIAL_COMMAND_HANDLER_H

#include <Arduino.h>
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include <Preferences.h>

// Command handler for serial interface - primarily for development and integration testing
class SerialCommandHandler {
private:
    static bool enabled;
    static unsigned long lastCommandTime;
    static const unsigned long COMMAND_TIMEOUT_MS = 30000; // 30 seconds timeout
    
    // Command processing methods
    static void processWiFiCommands(const String& command);
    static void processMQTTCommands(const String& command);
    static void processSystemCommands(const String& command);
    static void processInfoCommands(const String& command);
    
    // Utility methods
    static void printHelp();
    static void printDeviceStatus();
    static void printSystemInfo();
    
public:
    // Initialization and control
    static void begin(bool enableCommands = true);
    static void enable();
    static void disable();
    static bool isEnabled();
    
    // Main command processing
    static void handleSerialInput();
    
    // Command categories
    static void executeCommand(const String& command);
    
    // Development utilities
    static void setDebugMode(bool enabled);
    static void printMemoryInfo();
    static void printTaskInfo();
};

#endif // SERIAL_COMMAND_HANDLER_H
