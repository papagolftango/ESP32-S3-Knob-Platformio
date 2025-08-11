#ifndef CLOCK_APP_H
#define CLOCK_APP_H

#include "base_app.h"
#include <time.h>
#include <ArduinoJson.h>

class ClockApp : public BaseApp {
private:
    static const unsigned long UPDATE_INTERVAL = 1000; // Update every second
    
    // UI elements
    lv_obj_t* time_label = nullptr;
    lv_obj_t* date_label = nullptr;
    lv_obj_t* ntp_status_label = nullptr;
    
    // Time data
    struct tm current_time;
    bool ntp_enabled = false;
    bool time_set = false;
    unsigned long last_ntp_sync = 0;
    const unsigned long NTP_SYNC_INTERVAL = 3600000; // Sync every hour
    
public:
    ClockApp() : BaseApp() {}
    virtual ~ClockApp() {}
    
    // BaseApp interface implementation
    bool init() override;
    void deinit() override;
    lv_obj_t* createScreen() override;
    void onEnter() override;
    void onExit() override;
    void update() override;
    void onMQTTMessage(const String& topic, const String& payload) override;
    
    // Clock-specific functionality
    void setTime(int hour, int minute, int second = 0);
    void setDate(int year, int month, int day);
    void enableNTP(bool enable = true);
    void syncWithNTP();
    void updateTimeFromRTC();
    
private:
    void updateDisplay();
    String formatTime(bool use_24h = true);
    String formatDate();
    String getNTPStatusText();
    void simulateTimeAdvance(); // For demo without RTC
};

#endif // CLOCK_APP_H
