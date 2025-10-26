/**
 * burstfire_i2c_driver.h - Simple BurstFire I2C Driver
 * ESP-IDF v5.2+ compatible I2C driver for ATtiny202 BurstFire controllers
 ** Usage examples:
 *     // Initialize I2C
 *     burstfire_i2c_config_t config = {
 *         .port = I2C_NUM_0,
 *         .sda_pin = GPIO_NUM_21,
 *         .scl_pin = GPIO_NUM_22,
 *         .clk_speed = 100000
 *     };
 *     burstfire_i2c_init(&config);
 *     
 *     // Read register
 *     uint8_t data;
 *     burstfire_i2c_read_reg(0x20, 0x01, &data, 1);
 */

#ifndef BURSTFIRE_I2C_DRIVER_H
#define BURSTFIRE_I2C_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// Framework detection and includes
#ifdef ARDUINO
  #include <Arduino.h>
  #include <Wire.h>
  // Arduino ESP32 already includes ESP-IDF headers - use them directly!
  #include "esp_err.h"
  #include "hal/gpio_types.h"
  #include "hal/i2c_types.h"
  // I2C port constants for Arduino
  #ifndef I2C_NUM_0
  #define I2C_NUM_0 ((i2c_port_t)0)
  #endif
  #ifndef I2C_NUM_1  
  #define I2C_NUM_1 ((i2c_port_t)1)
  #endif
#else
  // ESP-IDF native includes
  #include "esp_err.h"
  #include "driver/i2c.h"
  #include "driver/gpio.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ATtiny202 Register Map
#define BURSTFIRE_REG_DUTY       0x00  // R/W: Duty cycle (0-10)
#define BURSTFIRE_REG_MAX_DUTY   0x01  // R: Maximum duty (always 10)
#define BURSTFIRE_REG_GRID_HZ    0x02  // R/W: Grid frequency (0=50Hz, 1=60Hz)
#define BURSTFIRE_REG_FW_MAJOR   0x10  // R: Firmware major version
#define BURSTFIRE_REG_FW_MINOR   0x11  // R: Firmware minor version
#define BURSTFIRE_REG_FW_PATCH   0x12  // R: Firmware patch version
#define BURSTFIRE_REG_STATUS     0x13  // R: Status bits
#define BURSTFIRE_REG_I2C_ADDR   0x14  // R: Device I2C address

// Status bits
#define BURSTFIRE_STATUS_RUN     (1 << 0)  // Controller running
#define BURSTFIRE_STATUS_GRID    (1 << 1)  // Grid frequency (0=50Hz, 1=60Hz)

// Configuration and device info structures
typedef struct {
    i2c_port_t port;
    gpio_num_t sda_pin;
    gpio_num_t scl_pin;
    uint32_t clk_speed;
} burstfire_config_t;

typedef struct {
    uint8_t address;        // I2C address
    uint8_t fw_major;       // Firmware major version
    uint8_t fw_minor;       // Firmware minor version
    uint8_t fw_patch;       // Firmware patch version
    bool connected;         // Device connection status
} burstfire_device_info_t;

// Core functions
esp_err_t burstfire_init(const burstfire_config_t *config);
esp_err_t burstfire_deinit(void);

// Device control
esp_err_t burstfire_set_duty(uint8_t addr, uint8_t duty);     // duty: 0-10
esp_err_t burstfire_get_duty(uint8_t addr, uint8_t *duty);
esp_err_t burstfire_set_grid_60hz(uint8_t addr, bool is_60hz); // false=50Hz, true=60Hz
esp_err_t burstfire_get_status(uint8_t addr, uint8_t *status);
bool burstfire_is_connected(uint8_t addr);                    // Simple ping test

// Device discovery and info
esp_err_t burstfire_scan_devices(uint8_t *addresses, uint8_t *count);  // Scan 0x20-0x23 range
esp_err_t burstfire_get_firmware_version(uint8_t addr, uint8_t *major, uint8_t *minor, uint8_t *patch);
esp_err_t burstfire_get_device_info(uint8_t addr, burstfire_device_info_t *info);  // Combined info reader

#ifdef __cplusplus
}
#endif

#endif // BURSTFIRE_I2C_DRIVER_H
