#include "clock_app.h"

bool ClockApp::init() {
    if (initialized) return true;
    
    Serial.println("Initializing Clock App...");
    
    // Initialize time with default values (current system time or demo time)
    time_t now_time;
    time(&now_time);
    localtime_r(&now_time, &current_time);
    last_ntp_sync = millis();
    
    // If no valid time, set demo time
    if (current_time.tm_year < 120) { // Year 2020
        current_time.tm_hour = 12;
        current_time.tm_min = 0;
        current_time.tm_sec = 0;
        current_time.tm_mday = 15;
        current_time.tm_mon = 11; // December (0-based)
        current_time.tm_year = 124; // 2024
        current_time.tm_wday = 0; // Sunday
    }
    
    ntp_enabled = true;
    time_set = true;
    
    // Create the screen
    screen = createScreen();
    if (!screen) {
        Serial.println("Failed to create Clock screen");
        return false;
    }
    
    initialized = true;
    Serial.println("Clock App initialized successfully");
    return true;
}

void ClockApp::deinit() {
    if (!initialized) return;
    
    if (screen) {
        lv_obj_del(screen);
        screen = nullptr;
    }
    
    time_label = nullptr;
    date_label = nullptr;
    ntp_status_label = nullptr;
    
    initialized = false;
    active = false;
    Serial.println("Clock App deinitialized");
}

lv_obj_t* ClockApp::createScreen() {
    // Create main screen container
    lv_obj_t* scr = lv_obj_create(NULL);
    if (!scr) return nullptr;
    
    // Set the screen as the current context for helper methods
    screen = scr;
    setBackgroundColor(0x001122);
    
    // Create title
    createTitle("🕐 CLOCK", 0x87CEEB);
    
    // Create time display (large font)
    time_label = createLabel("12:00:00", 0xFFFFFF, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_20, 0);
    
    // Create date display
    date_label = createLabel("Sun, Dec 15 2024", 0xCCCCCC, LV_ALIGN_CENTER, 0, 10);
    
    // Create NTP status
    ntp_status_label = createLabel("📡 NTP: Enabled", 0x90EE90, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_text_font(ntp_status_label, &lv_font_montserrat_14, 0);
    
    updateDisplay();
    return scr;
}

void ClockApp::onEnter() {
    if (!initialized) return;
    
    active = true;
    last_update = 0; // Force immediate update
    updateDisplay();
    
    Serial.println("Clock App entered");
}

void ClockApp::onExit() {
    active = false;
    Serial.println("Clock App exited");
}

void ClockApp::update() {
    if (!active || !initialized) return;
    
    unsigned long now = millis();
    if (now - last_update >= UPDATE_INTERVAL) {
        simulateTimeAdvance();
        updateDisplay();
        last_update = now;
        
        // Check if NTP sync is needed
        if (ntp_enabled && (now - last_ntp_sync >= NTP_SYNC_INTERVAL)) {
            // In a real implementation, this would sync with NTP
            Serial.println("Clock: NTP sync would occur here");
            last_ntp_sync = now;
        }
    }
}

void ClockApp::onMQTTMessage(const String& topic, const String& payload) {
    if (!active) return;
    
    // Parse MQTT messages for time/date settings
    if (topic.indexOf("time") >= 0 || topic.indexOf("clock") >= 0) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            if (doc["hour"].is<int>() && doc["minute"].is<int>()) {
                int hour = doc["hour"].as<int>();
                int minute = doc["minute"].as<int>();
                int second = doc["second"].is<int>() ? doc["second"].as<int>() : 0;
                setTime(hour, minute, second);
            }
            
            if (doc["year"].is<int>() && doc["month"].is<int>() && doc["day"].is<int>()) {
                int year = doc["year"].as<int>();
                int month = doc["month"].as<int>();
                int day = doc["day"].as<int>();
                setDate(year, month, day);
            }
            
            if (doc["ntp_enabled"].is<bool>()) {
                bool enable_ntp = doc["ntp_enabled"].as<bool>();
                enableNTP(enable_ntp);
            }
            
            updateDisplay();
            Serial.printf("Clock App: Updated from MQTT - %s\n", formatTime().c_str());
        }
    }
}

void ClockApp::setTime(int hour, int minute, int second) {
    if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59) {
        current_time.tm_hour = hour;
        current_time.tm_min = minute;
        current_time.tm_sec = second;
        time_set = true;
        
        if (active) updateDisplay();
        Serial.printf("Clock: Time set to %02d:%02d:%02d\n", hour, minute, second);
    }
}

void ClockApp::setDate(int year, int month, int day) {
    if (year >= 2020 && year <= 2099 && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
        current_time.tm_year = year - 1900; // tm_year is years since 1900
        current_time.tm_mon = month - 1;    // tm_mon is 0-based
        current_time.tm_mday = day;
        
        // Calculate day of week (simplified - in real app would use proper calculation)
        current_time.tm_wday = (day + month + year) % 7;
        
        time_set = true;
        
        if (active) updateDisplay();
        Serial.printf("Clock: Date set to %04d-%02d-%02d\n", year, month, day);
    }
}

void ClockApp::enableNTP(bool enable) {
    ntp_enabled = enable;
    if (active) updateDisplay();
    Serial.printf("Clock: NTP %s\n", enable ? "enabled" : "disabled");
}

void ClockApp::syncWithNTP() {
    if (!ntp_enabled) return;
    
    // In a real implementation, this would sync with NTP server
    Serial.println("Clock: Syncing with NTP server...");
    last_ntp_sync = millis();
    
    // Simulate NTP sync by updating time
    time_t now;
    time(&now);
    localtime_r(&now, &current_time);
    
    if (active) updateDisplay();
}

void ClockApp::updateTimeFromRTC() {
    // In a real implementation, this would read from RTC
    // For now, just update from system time if available
    time_t now;
    time(&now);
    if (now > 1000000000) { // Valid timestamp
        localtime_r(&now, &current_time);
        time_set = true;
        
        if (active) updateDisplay();
    }
}

void ClockApp::simulateTimeAdvance() {
    // Advance time by 1 second for demo purposes
    current_time.tm_sec++;
    
    if (current_time.tm_sec >= 60) {
        current_time.tm_sec = 0;
        current_time.tm_min++;
        
        if (current_time.tm_min >= 60) {
            current_time.tm_min = 0;
            current_time.tm_hour++;
            
            if (current_time.tm_hour >= 24) {
                current_time.tm_hour = 0;
                current_time.tm_mday++;
                current_time.tm_wday = (current_time.tm_wday + 1) % 7;
                
                // Simplified day rollover (doesn't handle month boundaries properly)
                if (current_time.tm_mday > 31) {
                    current_time.tm_mday = 1;
                    current_time.tm_mon++;
                    
                    if (current_time.tm_mon >= 12) {
                        current_time.tm_mon = 0;
                        current_time.tm_year++;
                    }
                }
            }
        }
    }
}

void ClockApp::updateDisplay() {
    if (!time_label || !date_label || !ntp_status_label) return;
    
    // Update time display
    String time_str = formatTime();
    lv_label_set_text(time_label, time_str.c_str());
    
    // Update date display
    String date_str = formatDate();
    lv_label_set_text(date_label, date_str.c_str());
    
    // Update NTP status
    String ntp_status = getNTPStatusText();
    lv_label_set_text(ntp_status_label, ntp_status.c_str());
    
    // Color based on time status
    uint32_t time_color = time_set ? 0xFFFFFF : 0xFF6B35; // White if set, orange if not
    lv_obj_set_style_text_color(time_label, lv_color_hex(time_color), 0);
}

String ClockApp::formatTime(bool use_24h) {
    char time_str[16];
    
    if (use_24h) {
        snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", 
                current_time.tm_hour, current_time.tm_min, current_time.tm_sec);
    } else {
        int hour_12 = current_time.tm_hour % 12;
        if (hour_12 == 0) hour_12 = 12;
        const char* ampm = current_time.tm_hour >= 12 ? "PM" : "AM";
        snprintf(time_str, sizeof(time_str), "%d:%02d:%02d %s", 
                hour_12, current_time.tm_min, current_time.tm_sec, ampm);
    }
    
    return String(time_str);
}

String ClockApp::formatDate() {
    const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    char date_str[32];
    snprintf(date_str, sizeof(date_str), "%s, %s %d %d",
            days[current_time.tm_wday],
            months[current_time.tm_mon],
            current_time.tm_mday,
            current_time.tm_year + 1900);
    
    return String(date_str);
}

String ClockApp::getNTPStatusText() {
    if (ntp_enabled) {
        unsigned long since_sync = millis() - last_ntp_sync;
        if (since_sync < 60000) { // Less than 1 minute
            return "📡 NTP: Synced";
        } else if (since_sync < 3600000) { // Less than 1 hour
            return "📡 NTP: Active";
        } else {
            return "📡 NTP: Enabled";
        }
    } else {
        return "📡 NTP: Disabled";
    }
}
