/**
 * burstfire_i2c_driver.c - Simple BurstFire I2C Driver Implementation
 * ESP-IDF v5.2+ compatible I2C driver for ATtiny202 BurstFire controllers
 */

#include "burstfire_i2c_driver.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "burstfire";
static bool g_initialized = false;
static i2c_port_t g_port;

// Internal helpers
static esp_err_t write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    uint8_t cmd[2] = {reg, val};
    return i2c_master_write_to_device(g_port, addr, cmd, 2, pdMS_TO_TICKS(100));
}

static esp_err_t read_reg(uint8_t addr, uint8_t reg, uint8_t *val) {
    uint8_t cmd = 0x80 | reg;  // Read-any mode
    return i2c_master_write_read_device(g_port, addr, &cmd, 1, val, 1, pdMS_TO_TICKS(100));
}

// Public API
esp_err_t burstfire_init(const burstfire_config_t *config) {
    if (config == NULL) return ESP_ERR_INVALID_ARG;
    
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = config->sda_pin,
        .scl_io_num = config->scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = config->clk_speed,
    };
    
    esp_err_t ret = i2c_param_config(config->port, &i2c_config);
    if (ret != ESP_OK) return ret;
    
    ret = i2c_driver_install(config->port, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) return ret;
    
    g_port = config->port;
    g_initialized = true;
    
    ESP_LOGI(TAG, "Initialized on port %d, SDA=%d, SCL=%d", 
             config->port, config->sda_pin, config->scl_pin);
    return ESP_OK;
}

esp_err_t burstfire_deinit(void) {
    if (!g_initialized) return ESP_ERR_INVALID_STATE;
    
    esp_err_t ret = i2c_driver_delete(g_port);
    g_initialized = false;
    return ret;
}

esp_err_t burstfire_set_duty(uint8_t addr, uint8_t duty) {
    if (!g_initialized) return ESP_ERR_INVALID_STATE;
    if (duty > 10) return ESP_ERR_INVALID_ARG;
    
    return write_reg(addr, BURSTFIRE_REG_DUTY, duty);
}

esp_err_t burstfire_get_duty(uint8_t addr, uint8_t *duty) {
    if (!g_initialized) return ESP_ERR_INVALID_STATE;
    if (duty == NULL) return ESP_ERR_INVALID_ARG;
    
    return read_reg(addr, BURSTFIRE_REG_DUTY, duty);
}

esp_err_t burstfire_set_grid_60hz(uint8_t addr, bool is_60hz) {
    if (!g_initialized) return ESP_ERR_INVALID_STATE;
    
    return write_reg(addr, BURSTFIRE_REG_GRID_HZ, is_60hz ? 1 : 0);
}

esp_err_t burstfire_get_status(uint8_t addr, uint8_t *status) {
    if (!g_initialized) return ESP_ERR_INVALID_STATE;
    if (status == NULL) return ESP_ERR_INVALID_ARG;
    
    return read_reg(addr, BURSTFIRE_REG_STATUS, status);
}

bool burstfire_is_connected(uint8_t addr) {
    if (!g_initialized) return false;
    
    esp_err_t ret = i2c_master_write_to_device(g_port, addr, NULL, 0, pdMS_TO_TICKS(100));
    return (ret == ESP_OK);
}

esp_err_t burstfire_scan_devices(uint8_t *addresses, uint8_t *count) {
    if (!g_initialized) return ESP_ERR_INVALID_STATE;
    if (addresses == NULL || count == NULL) return ESP_ERR_INVALID_ARG;
    
    *count = 0;
    
    // Scan ATtiny202 address range (0x20-0x23)
    for (uint8_t addr = 0x20; addr <= 0x23; addr++) {
        if (burstfire_is_connected(addr)) {
            addresses[*count] = addr;
            (*count)++;
        }
    }
    
    ESP_LOGI(TAG, "Scan found %d devices", *count);
    return ESP_OK;
}

esp_err_t burstfire_get_firmware_version(uint8_t addr, uint8_t *major, uint8_t *minor, uint8_t *patch) {
    if (!g_initialized) return ESP_ERR_INVALID_STATE;
    if (major == NULL || minor == NULL || patch == NULL) return ESP_ERR_INVALID_ARG;
    
    esp_err_t ret;
    ret = read_reg(addr, BURSTFIRE_REG_FW_MAJOR, major);
    if (ret != ESP_OK) return ret;
    
    ret = read_reg(addr, BURSTFIRE_REG_FW_MINOR, minor);
    if (ret != ESP_OK) return ret;
    
    ret = read_reg(addr, BURSTFIRE_REG_FW_PATCH, patch);
    return ret;
}

esp_err_t burstfire_get_device_info(uint8_t addr, burstfire_device_info_t *info) {
    if (!g_initialized) return ESP_ERR_INVALID_STATE;
    if (info == NULL) return ESP_ERR_INVALID_ARG;
    
    info->address = addr;
    info->connected = burstfire_is_connected(addr);
    
    if (info->connected) {
        esp_err_t ret = burstfire_get_firmware_version(addr, &info->fw_major, &info->fw_minor, &info->fw_patch);
        if (ret != ESP_OK) {
            info->connected = false;  // Mark as disconnected if can't read FW
            return ret;
        }
    } else {
        info->fw_major = info->fw_minor = info->fw_patch = 0;
    }
    
    return ESP_OK;
}
