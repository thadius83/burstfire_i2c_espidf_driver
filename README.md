# BurstFire I2C Driver

ESP-IDF v5.2+ driver for ATtiny202 BurstFire SSR controllers. Simple, practical API with device discovery.

## Quick Start

### ESP-IDF Example

```c
#include "burstfire_i2c_driver.h"

void app_main() {
    // Initialize I2C
    burstfire_config_t config = {
        .port = I2C_NUM_0,
        .sda_pin = GPIO_NUM_21,
        .scl_pin = GPIO_NUM_22,
        .clk_speed = 100000
    };
    ESP_ERROR_CHECK(burstfire_init(&config));
    
    // Scan for devices
    uint8_t addresses[4];
    uint8_t count;
    ESP_ERROR_CHECK(burstfire_scan_devices(addresses, &count));
    
    if (count > 0) {
        uint8_t addr = addresses[0];
        
        // Set 50% duty cycle
        ESP_ERROR_CHECK(burstfire_set_duty(addr, 5));
        
        // Configure for 60Hz grid
        ESP_ERROR_CHECK(burstfire_set_grid_60hz(addr, true));
        
        printf("Device 0x%02X configured\n", addr);
    }
}
```

### Arduino Example

```cpp
#include "burstfire_i2c_driver.h"

void setup() {
    Serial.begin(115200);
    
    // Initialize I2C (Arduino auto-detects framework)
    burstfire_config_t config = {
        .port = 0,          // Not used in Arduino
        .sda_pin = 21,      // SDA pin
        .scl_pin = 22,      // SCL pin  
        .clk_speed = 100000 // 100kHz
    };
    
    if (burstfire_init(&config) == ESP_OK) {
        Serial.println("BurstFire driver initialized");
        
        // Scan for devices
        uint8_t addresses[4];
        uint8_t count;
        burstfire_scan_devices(addresses, &count);
        
        if (count > 0) {
            uint8_t addr = addresses[0];
            Serial.printf("Found device at 0x%02X\n", addr);
            
            // Set 70% duty cycle
            burstfire_set_duty(addr, 7);
            burstfire_set_grid_60hz(addr, true);
            
            Serial.println("Device configured");
        }
    }
}

void loop() {
    // Your main code here
    delay(1000);
}
```

## Core API

### Initialization
- `burstfire_init(config)` - Initialize I2C driver
- `burstfire_deinit()` - Clean up resources

### Device Control
- `burstfire_set_duty(addr, duty)` - Set duty cycle (0-10, where 10=100%)
- `burstfire_get_duty(addr, *duty)` - Read current duty cycle
- `burstfire_set_grid_60hz(addr, is_60hz)` - Set grid frequency (false=50Hz, true=60Hz)
- `burstfire_get_status(addr, *status)` - Read status register
- `burstfire_is_connected(addr)` - Test device connectivity

### Device Discovery
- `burstfire_scan_devices(*addresses, *count)` - Find all devices (0x20-0x23)
- `burstfire_get_firmware_version(addr, *major, *minor, *patch)` - Read firmware version
- `burstfire_get_device_info(addr, *info)` - Get combined device information

## Common Use Cases

### Multiple Device Control
```c
uint8_t addresses[4];
uint8_t count;
burstfire_scan_devices(addresses, &count);

// Set all devices to 30% duty cycle
for (int i = 0; i < count; i++) {
    burstfire_set_duty(addresses[i], 3);
}
```

### Device Information
```c
burstfire_device_info_t info;
if (burstfire_get_device_info(0x20, &info) == ESP_OK && info.connected) {
    printf("Device 0x%02X: FW v%d.%d.%d\n", 
           info.address, info.fw_major, info.fw_minor, info.fw_patch);
}
```

### Status Monitoring
```c
uint8_t status;
if (burstfire_get_status(0x20, &status) == ESP_OK) {
    if (status & BURSTFIRE_STATUS_RUN) {
        printf("Controller is running\n");
    }
    if (status & BURSTFIRE_STATUS_GRID) {
        printf("Grid frequency: 60Hz\n");
    } else {
        printf("Grid frequency: 50Hz\n");
    }
}
```

### Error Handling
```c
esp_err_t ret = burstfire_set_duty(0x20, 5);
if (ret == ESP_ERR_INVALID_STATE) {
    printf("Driver not initialized\n");
} else if (ret == ESP_ERR_INVALID_ARG) {
    printf("Invalid duty cycle (must be 0-10)\n");
} else if (ret != ESP_OK) {
    printf("I2C communication failed: %s\n", esp_err_to_name(ret));
}
```

## Hardware Requirements

- **ATtiny202 Devices**: Addresses 0x20-0x23 (set via DIP switches)
- **I2C Pull-ups**: 4.7kΩ resistors required on SDA/SCL lines  
- **Power Supply**: 3.3V or 5V for ATtiny202
- **SSR Connection**: PA3 output drives SSR control (through transistor)

### ESP32 Pin Examples

**ESP32 DevKit:**
```c
.sda_pin = GPIO_NUM_21,
.scl_pin = GPIO_NUM_22,
```

**ESP32-S2:**
```c
.sda_pin = GPIO_NUM_8,
.scl_pin = GPIO_NUM_9,
```

**ESP32-C6:**
```c
.sda_pin = GPIO_NUM_6,
.scl_pin = GPIO_NUM_7,
```

## Integration

### ESP-IDF Component
Create `components/burstfire_driver/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "burstfire_i2c_driver.cpp"
    INCLUDE_DIRS "."
    REQUIRES "driver" "esp_log"
)
```

Copy driver files to `components/burstfire_driver/` and include:
```c
#include "burstfire_i2c_driver.h"
```

### Arduino IDE
Copy both files (`burstfire_i2c_driver.h` and `burstfire_i2c_driver.cpp`) to your Arduino project folder and include:
```cpp
#include "burstfire_i2c_driver.h"
```

### PlatformIO
Copy files to `lib/burstfire_driver/` and use:
```ini
lib_deps = 
    lib/burstfire_driver
```

## Protocol Details

The driver uses ATtiny202's extended I2C protocol:
- **Register writes**: `[register, value]` 
- **Register reads**: `[0x80 | register]` then read data

This is handled automatically - you don't need to worry about protocol details.

## Troubleshooting

**Device Not Found:**
- Check I2C pull-up resistors (4.7kΩ)
- Verify ATtiny202 power supply
- Confirm DIP switch address settings

**Communication Errors:**
- Try lower I2C speed (50kHz): `.clk_speed = 50000`
- Check SDA/SCL pin connections
- Ensure proper ground connections

**Duty Cycle Not Working:**
- Verify SSR connections
- Check ATtiny202 PA3 output with scope/meter
- Confirm grid frequency matches actual AC frequency

Enable debug logging for detailed troubleshooting:
```c
esp_log_level_set("burstfire", ESP_LOG_DEBUG);
