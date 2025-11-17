// Complete_wr

#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

esp_err_t i2c_send_data_block(uint8_t device_addr, uint8_t *data, size_t length); // Function prototype for sending data blocks

// -------------------------
// I²C Configuration
// -------------------------
#define I2C_MASTER_SCL_IO 37 // GPIO pin for I2C Clock (SCL)
#define I2C_MASTER_SDA_IO 38 // GPIO pin for I2C Data (SDA)
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 400000 // 400 kHz I2C speed
#define I2C_TIMEOUT_MS 1000
#define SLAVE_ADDR 0x68 // Changed slave address since AD0 is grounded (JI)

static const char *TAG = "i2c-master-example";

void app_main()
{
    // -------------------------
    // Step 1: Configure I2C Master
    // -------------------------
    ESP_LOGI(TAG, "Initializing I2C master...");

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    // Apply I2C configuration
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));

    // Install I2C driver
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));
    ESP_LOGI(TAG, "I2C master initialized successfully");

    // -------------------------
    // Step 2: Prepare Data to Send
    // -------------------------
    uint8_t data_to_send[2] = {0x3A, 0xD6}; // Data block to send, register is 0x3A, data value is 0xD6 (JI)

    // -------------------------
    // Step 3: Send Data block to I2C Device
    // -------------------------
    ESP_LOGI(TAG, "Sending data to device address 0x%02X", SLAVE_ADDR);
    esp_err_t ret = i2c_send_data_block(SLAVE_ADDR, data_to_send, sizeof(data_to_send));

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Data sent successfully");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to send data: %s", esp_err_to_name(ret));
    }

    // -------------------------
    // Step 4: Clean Up
    // -------------------------
    ESP_LOGI(TAG, "Deleting I2C driver...");
    ESP_ERROR_CHECK(i2c_driver_delete(I2C_MASTER_NUM));
    ESP_LOGI(TAG, "I2C driver removed, program complete.");
}

esp_err_t i2c_send_data_block(uint8_t device_addr, uint8_t *data, size_t length)
{
    return i2c_master_write_to_device(I2C_MASTER_NUM,
                                      device_addr,
                                      data,
                                      length,
                                      pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}