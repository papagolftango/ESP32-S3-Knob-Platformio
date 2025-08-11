#include "display_manager.h"

// Static member definitions
lv_disp_draw_buf_t DisplayManager::draw_buf;
lv_color_t DisplayManager::buf1[EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT];
lv_color_t DisplayManager::buf2[EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT];
lv_disp_drv_t DisplayManager::disp_drv;
lv_indev_drv_t DisplayManager::indev_drv;

bool DisplayManager::initLVGL() {
    Serial.println("Initializing LVGL system...");
    
    // Initialize LVGL core
    lv_init();
    
    Serial.println("LVGL core initialized");
    return true;
}

bool DisplayManager::initDisplay() {
    Serial.println("Initializing display system...");
    
    // Initialize hardware first
    if (!LcdDriver::initLcd()) {
        Serial.println("Failed to initialize LCD hardware");
        return false;
    }
    
    // Initialize display buffer with proper sizing
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT);
    
    // Initialize display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    disp_drv.flush_cb = LcdDriver::display_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    
    // Register the driver
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    if (!disp) {
        Serial.println("Failed to register display driver");
        return false;
    }
    
    Serial.println("Display system initialized");
    return true;
}

bool DisplayManager::initInput() {
    Serial.println("Initializing input system...");
    
    // Initialize hardware input
    if (!LcdDriver::initTouch()) {
        Serial.println("Failed to initialize touch hardware");
        return false;
    }
    
    // Initialize input device driver (for touch/encoder)
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = LcdDriver::touchpad_read_cb;
    
    // Register the input device
    lv_indev_t *indev = lv_indev_drv_register(&indev_drv);
    if (!indev) {
        Serial.println("Failed to register input driver");
        return false;
    }
    
    Serial.println("Input system initialized");
    return true;
}

void DisplayManager::handleLVGLTasks() {
    // Handle LVGL tasks - should be called regularly in main loop
    lv_timer_handler();
}

void DisplayManager::shutdown() {
    Serial.println("Shutting down display system...");
    LcdDriver::powerDown();
}

void DisplayManager::restart() {
    Serial.println("Restarting display system...");
    shutdown();
    LcdDriver::powerUp();
}

void DisplayManager::printDisplayInfo() {
    Serial.println("=== Display System Info ===");
    Serial.printf("LVGL Version: %d.%d.%d\n", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
    Serial.printf("Screen Resolution: %dx%d\n", getScreenWidth(), getScreenHeight());
    Serial.printf("Buffer Size: %d pixels\n", EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT);
    Serial.println("===========================");
    
    // Also print hardware info
    LcdDriver::printHardwareInfo();
}
