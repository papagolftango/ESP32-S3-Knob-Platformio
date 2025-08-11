#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <lvgl.h>
#include "../drivers/lcd_driver.h"

// High-level display system manager for LVGL
class DisplayManager {
private:
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT];
    static lv_color_t buf2[EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT];
    static lv_disp_drv_t disp_drv;
    static lv_indev_drv_t indev_drv;
    
public:
    // System initialization
    static bool initLVGL();
    static bool initDisplay();
    static bool initInput();
    
    // System management
    static void handleLVGLTasks();
    static void shutdown();
    static void restart();
    
    // Convenience methods
    static int getScreenWidth() { return LcdDriver::getScreenWidth(); }
    static int getScreenHeight() { return LcdDriver::getScreenHeight(); }
    static void setBacklight(bool on) { LcdDriver::setBacklight(on); }
    
    // System info
    static void printDisplayInfo();
};

#endif // DISPLAY_MANAGER_H
