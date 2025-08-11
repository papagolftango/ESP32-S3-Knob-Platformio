#include "serial_command_handler.h"

// Static member definitions
bool SerialCommandHandler::enabled = true;
unsigned long SerialCommandHandler::lastCommandTime = 0;

void SerialCommandHandler::begin(bool enableCommands) {
    enabled = enableCommands;
    lastCommandTime = millis();
    
    if (enabled) {
        Serial.println("\n=== ESP32-S3 Knob Command Interface ===");
        Serial.println("Type 'help' for available commands");
        Serial.println("Commands will be disabled after 30 seconds of inactivity for security");
        Serial.println("=====================================\n");
    }
}

void SerialCommandHandler::enable() {
    enabled = true;
    lastCommandTime = millis();
    Serial.println("Command interface enabled");
}

void SerialCommandHandler::disable() {
    enabled = false;
    Serial.println("Command interface disabled for security");
}

bool SerialCommandHandler::isEnabled() {
    // Auto-disable after timeout for security
    if (enabled && (millis() - lastCommandTime) > COMMAND_TIMEOUT_MS) {
        disable();
    }
    return enabled;
}

void SerialCommandHandler::handleSerialInput() {
    if (!Serial.available()) return;
    
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    
    if (command.length() == 0) return;
    
    Serial.printf(">>> %s\n", command.c_str());
    
    // Check if commands are enabled
    if (!isEnabled()) {
        if (command == "enable" || command == "unlock") {
            enable();
            return;
        } else {
            Serial.println("Command interface disabled. Type 'enable' to activate.");
            return;
        }
    }
    
    // Update last command time
    lastCommandTime = millis();
    
    // Execute the command
    executeCommand(command);
}

void SerialCommandHandler::executeCommand(const String& command) {
    Serial.printf("Executing command: %s\n", command.c_str());
    
    // System commands (highest priority)
    if (command == "help" || command == "?") {
        printHelp();
        return;
    }
    
    if (command == "status") {
        printDeviceStatus();
        return;
    }
    
    if (command == "info" || command == "sysinfo") {
        printSystemInfo();
        return;
    }
    
    if (command == "disable") {
        disable();
        return;
    }
    
    // WiFi commands
    if (command.startsWith("reset_wifi") || command.startsWith("wifi")) {
        processWiFiCommands(command);
        return;
    }
    
    // MQTT commands  
    if (command.startsWith("reset_mqtt") || command.startsWith("mqtt")) {
        processMQTTCommands(command);
        return;
    }
    
    // System commands
    if (command.startsWith("factory") || command.startsWith("restart") || 
        command.startsWith("reboot") || command.startsWith("reset")) {
        processSystemCommands(command);
        return;
    }
    
    // Development commands
    if (command == "memory" || command == "mem") {
        printMemoryInfo();
        return;
    }
    
    if (command == "tasks") {
        printTaskInfo();
        return;
    }
    
    // Unknown command
    Serial.printf("Unknown command: %s\n", command.c_str());
    Serial.println("Type 'help' for available commands");
}

void SerialCommandHandler::processWiFiCommands(const String& command) {
    if (command == "reset_wifi" || command == "resetwifi" || command == "wifi_reset") {
        Serial.println("=== WiFi Reset ===");
        Serial.println("Clearing WiFi configuration...");
        wifiManager.reset();
        Serial.println("WiFi configuration cleared. Restarting...");
        delay(1000);
        ESP.restart();
        
    } else if (command == "wifi_status" || command == "wifi") {
        Serial.println("=== WiFi Status ===");
        Serial.printf("Connected: %s\n", wifiManager.isWiFiConnected() ? "Yes" : "No");
        if (wifiManager.isWiFiConnected()) {
            Serial.printf("SSID: %s\n", wifiManager.getSSID().c_str());
            Serial.printf("IP: %s\n", wifiManager.getIP().c_str());
            Serial.printf("RSSI: %d dBm\n", wifiManager.getRSSI());
        }
        Serial.println("================");
        
    } else {
        Serial.println("WiFi commands:");
        Serial.println("  reset_wifi   - Reset WiFi configuration");
        Serial.println("  wifi_status  - Show WiFi connection status");
    }
}

void SerialCommandHandler::processMQTTCommands(const String& command) {
    if (command == "reset_mqtt" || command == "resetmqtt" || command == "mqtt_reset") {
        Serial.println("=== MQTT Reset ===");
        Serial.println("Clearing MQTT configuration...");
        mqttManager.resetConfig();
        mqttManager.saveConfig();
        Serial.println("MQTT configuration cleared.");
        Serial.println("Restart device to reconfigure MQTT.");
        
    } else if (command == "mqtt_status" || command == "mqtt") {
        Serial.println("=== MQTT Status ===");
        Serial.printf("Connected: %s\n", mqttManager.connected() ? "Yes" : "No");
        if (mqttManager.connected()) {
            Serial.printf("Server: %s:%d\n", mqttManager.getServer().c_str(), mqttManager.getPort());
            Serial.printf("Client ID: %s\n", mqttManager.getClientId().c_str());
        }
        Serial.println("==================");
        
    } else {
        Serial.println("MQTT commands:");
        Serial.println("  reset_mqtt   - Reset MQTT configuration");
        Serial.println("  mqtt_status  - Show MQTT connection status");
    }
}

void SerialCommandHandler::processSystemCommands(const String& command) {
    if (command == "factory_reset" || command == "factoryreset" || command == "factory") {
        Serial.println("=== FACTORY RESET ===");
        Serial.println("⚠️  WARNING: This will erase ALL configuration!");
        Serial.println("Clearing WiFi configuration...");
        wifiManager.reset();
        Serial.println("Clearing MQTT configuration...");
        mqttManager.resetConfig();
        Serial.println("Clearing preferences...");
        Preferences prefs;
        prefs.begin("config", false);
        prefs.clear();
        prefs.end();
        Serial.println("Factory reset complete. Restarting...");
        delay(2000);
        ESP.restart();
        
    } else if (command == "restart" || command == "reboot") {
        Serial.println("=== RESTART ===");
        Serial.println("Restarting device...");
        delay(500);
        ESP.restart();
        
    } else {
        Serial.println("System commands:");
        Serial.println("  factory_reset - Erase all configuration and restart");
        Serial.println("  restart       - Restart the device");
    }
}

void SerialCommandHandler::printHelp() {
    Serial.println("\n=== ESP32-S3 Knob Commands ===");
    Serial.println("SYSTEM:");
    Serial.println("  help          - Show this help");
    Serial.println("  status        - Show device status");
    Serial.println("  info          - Show system information");
    Serial.println("  restart       - Restart device");
    Serial.println("  factory_reset - Reset all configuration");
    Serial.println("  disable       - Disable command interface");
    Serial.println("");
    Serial.println("WIFI:");
    Serial.println("  reset_wifi    - Reset WiFi configuration");
    Serial.println("  wifi_status   - Show WiFi status");
    Serial.println("");
    Serial.println("MQTT:");
    Serial.println("  reset_mqtt    - Reset MQTT configuration");
    Serial.println("  mqtt_status   - Show MQTT status");
    Serial.println("");
    Serial.println("DEVELOPMENT:");
    Serial.println("  memory        - Show memory usage");
    Serial.println("  tasks         - Show FreeRTOS task info");
    Serial.println("");
    Serial.println("Commands auto-disable after 30s of inactivity for security.");
    Serial.println("===============================\n");
}

void SerialCommandHandler::printDeviceStatus() {
    Serial.println("\n=== DEVICE STATUS ===");
    Serial.printf("Uptime: %lu ms (%.1f minutes)\n", millis(), millis() / 60000.0);
    Serial.printf("Free Heap: %u bytes (%.1f KB)\n", ESP.getFreeHeap(), ESP.getFreeHeap() / 1024.0);
    Serial.printf("Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("CPU Frequency: %u MHz\n", ESP.getCpuFreqMHz());
    Serial.println("");
    
    // WiFi Status
    Serial.printf("WiFi: %s", wifiManager.isWiFiConnected() ? "✅ Connected" : "❌ Disconnected");
    if (wifiManager.isWiFiConnected()) {
        Serial.printf(" (%s, %d dBm)", wifiManager.getSSID().c_str(), wifiManager.getRSSI());
    }
    Serial.println("");
    
    // MQTT Status
    Serial.printf("MQTT: %s", mqttManager.connected() ? "✅ Connected" : "❌ Disconnected");
    if (mqttManager.connected()) {
        Serial.printf(" (%s)", mqttManager.getServer().c_str());
    }
    Serial.println("");
    
    Serial.println("==================\n");
}

void SerialCommandHandler::printSystemInfo() {
    Serial.println("\n=== SYSTEM INFORMATION ===");
    Serial.printf("Chip: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("CPU Cores: %d\n", ESP.getChipCores());
    Serial.printf("CPU Frequency: %u MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash Size: %u bytes (%.1f MB)\n", ESP.getFlashChipSize(), ESP.getFlashChipSize() / 1048576.0);
    Serial.printf("Flash Speed: %u Hz\n", ESP.getFlashChipSpeed());
    Serial.printf("PSRAM Size: %u bytes (%.1f MB)\n", ESP.getPsramSize(), ESP.getPsramSize() / 1048576.0);
    Serial.printf("Sketch Size: %u bytes (%.1f KB)\n", ESP.getSketchSize(), ESP.getSketchSize() / 1024.0);
    Serial.printf("Free Sketch Space: %u bytes (%.1f KB)\n", ESP.getFreeSketchSpace(), ESP.getFreeSketchSpace() / 1024.0);
    Serial.printf("SDK Version: %s\n", ESP.getSdkVersion());
    
    uint64_t macAddress = ESP.getEfuseMac();
    Serial.printf("MAC Address: %04X%08X\n", (uint16_t)(macAddress >> 32), (uint32_t)macAddress);
    
    Serial.println("=========================\n");
}

void SerialCommandHandler::printMemoryInfo() {
    Serial.println("\n=== MEMORY INFORMATION ===");
    Serial.printf("Free Heap: %u bytes (%.1f KB)\n", ESP.getFreeHeap(), ESP.getFreeHeap() / 1024.0);
    Serial.printf("Largest Free Block: %u bytes (%.1f KB)\n", ESP.getMaxAllocHeap(), ESP.getMaxAllocHeap() / 1024.0);
    Serial.printf("Free PSRAM: %u bytes (%.1f KB)\n", ESP.getFreePsram(), ESP.getFreePsram() / 1024.0);
    Serial.printf("Min Free Heap: %u bytes (%.1f KB)\n", ESP.getMinFreeHeap(), ESP.getMinFreeHeap() / 1024.0);
    
    // Calculate heap fragmentation
    float fragmentation = 100.0 * (1.0 - (float)ESP.getMaxAllocHeap() / ESP.getFreeHeap());
    Serial.printf("Heap Fragmentation: %.1f%%\n", fragmentation);
    
    Serial.println("=========================\n");
}

void SerialCommandHandler::printTaskInfo() {
    Serial.println("\n=== FREERTOS TASK INFO ===");
    Serial.printf("Number of Tasks: %d\n", uxTaskGetNumberOfTasks());
    Serial.printf("Scheduler State: %s\n", xTaskGetSchedulerState() == taskSCHEDULER_RUNNING ? "Running" : "Suspended");
    
    // Print task list (if available in debug builds)
    #if CONFIG_FREERTOS_USE_TRACE_FACILITY
    size_t taskCount = uxTaskGetNumberOfTasks();
    TaskStatus_t* taskArray = (TaskStatus_t*)malloc(taskCount * sizeof(TaskStatus_t));
    
    if (taskArray) {
        taskCount = uxTaskGetSystemState(taskArray, taskCount, NULL);
        
        Serial.println("Task Name            | State | Priority | Stack High Water Mark");
        Serial.println("---------------------|-------|----------|---------------------");
        
        for (size_t i = 0; i < taskCount; i++) {
            const char* taskState;
            switch (taskArray[i].eTaskState) {
                case eRunning:   taskState = "RUN"; break;
                case eReady:     taskState = "RDY"; break;
                case eBlocked:   taskState = "BLK"; break;
                case eSuspended: taskState = "SUS"; break;
                case eDeleted:   taskState = "DEL"; break;
                default:         taskState = "UNK"; break;
            }
            
            Serial.printf("%-20s | %-5s | %-8lu | %lu\n",
                         taskArray[i].pcTaskName,
                         taskState,
                         taskArray[i].uxCurrentPriority,
                         taskArray[i].usStackHighWaterMark);
        }
        
        free(taskArray);
    }
    #else
    Serial.println("Task details not available (CONFIG_FREERTOS_USE_TRACE_FACILITY not enabled)");
    #endif
    
    Serial.println("=========================\n");
}

void SerialCommandHandler::setDebugMode(bool enableDebug) {
    if (enableDebug) {
        Serial.println("Debug mode enabled - verbose logging active");
    } else {
        Serial.println("Debug mode disabled");
    }
}
