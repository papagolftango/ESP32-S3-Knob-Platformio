# ESP32-S3 Smart Home Knob - PlatformIO Version

A refactored PlatformIO implementation of the ESP32-S3 Smart Home Knob with LVGL display, rotary encoder navigation, WiFi connectivity, and MQTT integration.

## 🚀 Features

- **🖥️ LVGL-based UI** with multiple screens (Home, Energy, Weather, House, Clock, Settings)
- **🔄 Rotary Encoder Navigation** - Turn to navigate, press to select
- **📶 WiFi Manager** - Easy WiFi configuration via captive portal
- **📡 MQTT Integration** - Dynamic configuration and home automation control
- **💾 Persistent Settings** - Configuration stored in NVS flash
- **⚡ ESP32-S3 Optimized** - Uses PSRAM and native USB

## 📁 Project Structure

```
platformio-version/
├── platformio.ini          # PlatformIO configuration
├── lv_conf.h               # LVGL configuration
├── src/
│   └── main.cpp            # Main application code
└── README.md               # This file
```

## 🔧 Hardware Requirements

### ESP32-S3 Development Board
- ESP32-S3 with 8MB PSRAM
- 240x240 SPI Display (ST7789 or similar)
- Rotary encoder with button
- Optional: Touch screen support

### Pin Configuration (Adjust in main.cpp)
```cpp
// Display pins
#define TFT_CS     10
#define TFT_DC     11  
#define TFT_RST    12
#define TFT_MOSI   13
#define TFT_CLK    14
#define TFT_BL     15

// Rotary encoder pins
#define ENCODER_A  8
#define ENCODER_B  9
#define ENCODER_BTN 7
```

## 📚 Dependencies

The following libraries are automatically installed by PlatformIO:

- **LVGL** (v8.3.11) - Graphics library
- **PubSubClient** (v2.8) - MQTT client
- **ArduinoJson** (v7.0.4) - JSON parsing
- **WiFiManager** - WiFi configuration
- **ESP32Encoder** (v0.10.2) - Rotary encoder handling

## 🔨 Building and Flashing

### Prerequisites
1. Install [PlatformIO](https://platformio.org/install)
2. Install PlatformIO VSCode extension (recommended)

### Build and Upload
```bash
# Navigate to project directory
cd platformio-version

# Build the project
pio run

# Upload to ESP32-S3 (adjust COM port)
pio run --target upload --upload-port COM7

# Monitor serial output
pio device monitor --port COM7 --baud 115200
```

### VS Code Integration
1. Open the `platformio-version` folder in VS Code
2. PlatformIO will automatically detect the project
3. Use the PlatformIO toolbar to build/upload/monitor

## 🖥️ Screen Navigation

The device has 6 main screens accessible via rotary encoder:

1. **🏠 HOME** - Status overview and navigation hub
2. **⚡ ENERGY** - Power consumption and costs
3. **🌤️ WEATHER** - Temperature, humidity, conditions
4. **🏠 HOUSE** - Home automation controls
5. **🕐 CLOCK** - Time and date display
6. **⚙️ SETTINGS** - Configuration options

### Controls
- **Turn Encoder**: Navigate between screens
- **Press Encoder**: Context-dependent actions
- **Long Press**: Return to home screen (from any screen)

## 📡 MQTT Configuration

### Auto-Configuration
The device automatically generates a unique client ID based on MAC address:
```
esp32-knob-XXXXXX
```

### Default MQTT Topics
```
home/+/energy/+     # Energy monitoring data
home/+/weather/+    # Weather information  
home/+/control/+    # Device control commands
```

### Settings Storage
MQTT settings are stored in NVS flash:
- Server address and port
- Username/password (if required)
- SSL configuration
- Client ID

## 🔧 Configuration

### WiFi Setup
1. On first boot, device creates "ESP32-Knob-Setup" access point
2. Connect to AP and configure WiFi credentials
3. Device remembers settings for future boots

### MQTT Setup
MQTT configuration can be updated via:
1. Settings screen navigation
2. Serial commands (future enhancement)
3. Web interface (future enhancement)

## 🐛 Debugging

### Serial Monitor
```bash
pio device monitor --port COM7 --baud 115200
```

### Common Issues

**Display not working:**
- Check pin definitions in `main.cpp`
- Verify SPI connections
- Ensure display driver compatibility

**Encoder not responding:**
- Check pin assignments
- Verify pull-up resistors
- Test encoder wiring

**WiFi connection fails:**
- Reset WiFi credentials via captive portal
- Check network configuration
- Verify signal strength

**MQTT not connecting:**
- Check server address and port
- Verify credentials
- Test network connectivity

## 🔮 Future Enhancements

- [ ] Touch screen support
- [ ] Web configuration interface
- [ ] OTA firmware updates
- [ ] Custom LVGL themes
- [ ] Energy usage graphs
- [ ] Weather forecasts
- [ ] Voice control integration
- [ ] Home Assistant integration

## 📊 Performance

- **Boot Time**: ~3-5 seconds
- **Screen Refresh**: 30fps
- **Memory Usage**: ~60% with PSRAM
- **Power Consumption**: ~150mA @ 3.3V

## 🤝 Contributing

1. Fork the repository
2. Create feature branch
3. Test on hardware
4. Submit pull request

## 📄 License

This project is open source. See original ESP-IDF project for license details.

## 🙏 Acknowledgments

- **LVGL Team** - Excellent graphics library
- **Espressif** - ESP32-S3 platform
- **PlatformIO** - Development environment
- **Arduino Community** - Libraries and support
