#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>
#include <lvgl.h>
#include "lcd_config.h"

// Display driver class for LVGL integration
class DisplayDriver {
private:
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT];
    static lv_color_t buf2[EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT];
    static lv_disp_drv_t disp_drv;
    static lv_indev_drv_t indev_drv;
    
    // Static callback functions for LVGL
    static void display_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
    static void touchpad_read_cb(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
    
public:
    // Initialization methods
    static bool initLVGL();
    static bool initDisplay();
    static bool initInput();
    
    // Display management
    static void handleLVGLTasks();
    static int getScreenWidth();
    static int getScreenHeight();
    
    // Utility methods
    static void setBacklight(bool on);
    static void printDisplayInfo();
};

#endif // DISPLAY_DRIVER_H
