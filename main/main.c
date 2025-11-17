// Complete_wr

#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

esp_err_t i2c_send_data_block(uint8_t device_addr, uint8_t *data, size_t length); // Function prototype for sending data block
esp_err_t i2c_read_bytes(uint8_t device_addr, uint8_t start_reg, uint8_t *buffer, size_t length); // Function prototype for reading bytes

// -------------------------
// I²C Configuration        
// -------------------------
#define I2C_MASTER_SCL_IO      37      // GPIO pin for I2C Clock (SCL)
#define I2C_MASTER_SDA_IO      38      // GPIO pin for I2C Data (SDA)
#define I2C_MASTER_NUM         I2C_NUM_0
#define I2C_MASTER_FREQ_HZ     400000  // 100 kHz I2C speed
#define I2C_TIMEOUT_MS         1000
#define SLAVE_ADDR             0x68    // Example I2C device address, change according to your AD0 connection


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
    uint8_t data_to_send[2] = {0x3A, 0xD6};

    // -------------------------
    // Step 3: Send Data block to I2C Device
    // -------------------------

    ESP_LOGI(TAG, "Sending data to device address 0x%02X", SLAVE_ADDR);

    ESP_LOGI(TAG, "Sending data");
    esp_err_t ret = i2c_send_data_block(SLAVE_ADDR, data_to_send, sizeof(data_to_send));

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Data sent successfully");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to send data: %s", esp_err_to_name(ret));
    }

        // Read 2 bytes starting from register 0x3A
        uint8_t read_buffer[2];
        esp_err_t ret_read = i2c_read_bytes(SLAVE_ADDR, 0x3A, read_buffer, 2);
        
        if (ret_read == ESP_OK)
        {
            ESP_LOGI(TAG, "Read bytes: 0x%02X 0x%02X", read_buffer[0], read_buffer[1]);
        }

        else
        {
            ESP_LOGE(TAG, "Failed to read bytes: %s", esp_err_to_name(ret_read));
        } 

    // -------------------------
    // Step 4: Clean Up
    // -------------------------
    ESP_LOGI(TAG, "Deleting I2C driver...");
    ESP_ERROR_CHECK(i2c_driver_delete(I2C_MASTER_NUM));
    ESP_LOGI(TAG, "I2C driver removed, program complete.");

}

// Function definition to read bytes (JI)
esp_err_t i2c_read_bytes(uint8_t device_addr, uint8_t start_reg, uint8_t *buffer, size_t length)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM,
                                        device_addr,
                                        &start_reg,
                                        1,   
                                        buffer,
                                        length,                                                                                                     
                                        pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}
esp_err_t i2c_send_data_block(uint8_t device_addr, uint8_t *data, size_t length)
{
    return i2c_master_write_to_device(I2C_MASTER_NUM,
                                      device_addr,
                                      data,
                                      length,
                                      pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}    
