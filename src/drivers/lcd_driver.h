#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include <Arduino.h>
#include <lvgl.h>
#include "lcd_config.h"

// Low-level LCD hardware driver
class LcdDriver {
private:
    // Hardware-specific implementations
    static void initHardware();
    static void setupPins();
    
public:
    // Hardware initialization
    static bool initLcd();
    static bool initTouch();
    
    // LVGL hardware callbacks
    static void display_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
    static void touchpad_read_cb(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
    
    // Hardware control
    static void setBacklight(bool on);
    static void powerDown();
    static void powerUp();
    
    // Hardware info
    static int getScreenWidth() { return EXAMPLE_LCD_H_RES; }
    static int getScreenHeight() { return EXAMPLE_LCD_V_RES; }
    static void printHardwareInfo();
};

#endif // LCD_DRIVER_H
