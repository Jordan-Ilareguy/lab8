#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define I2C_MASTER_SCL_IO           37         // SCL pin for ESP32-S3
#define I2C_MASTER_SDA_IO           38         // SDA pin for ESP32-S3
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          400000     // 400 kHz (Fast mode)
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0

#define MPU_TEMP                    0x41       // Temperature register - 16bits (JI)
#define MPU6050_ADDR                0x68       // AD0 = GND
#define MPU6050_WHO_AM_I_REG        0x75
#define MPU6050_PWR_MGMT_1          0x6B
#define MPU6050_ACCEL_XOUT_H        0x3B       // Register address to begin with
#define I2C_TIMEOUT_MS              1000

static const char *TAG = "MPU6050";

// Function prototype to read bytes (JI)
esp_err_t i2c_read_bytes(uint8_t device_addr, uint8_t start_reg, uint8_t *buffer, size_t length);
esp_err_t i2c_send_data_block(uint8_t device_addr, uint8_t *data, size_t length);

esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) return err;

    return i2c_driver_install(I2C_MASTER_NUM,
                              conf.mode,
                              I2C_MASTER_RX_BUF_DISABLE,
                              I2C_MASTER_TX_BUF_DISABLE,
                              0);
}

void app_main()
{
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized");

    // Wake device first (MPU6050 boots in sleep mode)
    uint8_t wake[2] = {0x6B, 0x00};
    i2c_send_data_block(MPU6050_ADDR, wake, sizeof(wake));
    vTaskDelay(pdMS_TO_TICKS(100));
    
    uint8_t data_to_send[2] = {0x1C, 0x08};

    // -------------------------
    // Step 3: Send Data block to I2C Device
    // -------------------------
    ESP_LOGI(TAG, "Sending data to device address 0x%02X", data_to_send[0]);
    ESP_LOGI(TAG, "Writing reg 0x%02X := 0x%02X", data_to_send[0], data_to_send[1]);
    esp_err_t ret = i2c_send_data_block(MPU6050_ADDR, data_to_send, sizeof(data_to_send));

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Data sent successfully");
        
        uint8_t read_buffer[2];
        esp_err_t ret_read = i2c_read_bytes(MPU6050_ADDR, data_to_send[0], read_buffer, 2);
        
        if (ret_read == ESP_OK)
        {
            ESP_LOGI(TAG, "Read bytes: 0x%02X", read_buffer[0]);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to read bytes: %s", esp_err_to_name(ret_read));
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to send data: %s", esp_err_to_name(ret));
    }

    while (1) {
        uint8_t data[14];
        esp_err_t err = i2c_read_bytes(MPU6050_ADDR, MPU6050_ACCEL_XOUT_H, data, sizeof(data));
        
        if (err == ESP_OK) {
            int16_t ax = (data[0] << 8) | data[1];
            int16_t ay = (data[2] << 8) | data[3];
            int16_t az = (data[4] << 8) | data[5];

            int16_t temp_raw = (data[6] << 8) | data[7]; //Raw temperature value extracted from the register.
            float temperature = (temp_raw / 340.0) + 36.53; //Value extracted is converted to Celsius through this formula.

            int16_t gx = (data[8] << 8) | data[9];
            int16_t gy = (data[10] << 8) | data[11];
            int16_t gz = (data[12] << 8) | data[13];
            printf("Accel: X=%d Y=%d Z=%d | Temp: %.2f°C | Gyro: X=%d Y=%d Z=%d\n",
                   ax, ay, az, temperature, gx, gy, gz);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* Function Name - i2c_read_bytes

* Description - This function reads a block of data from an I2C device.

* Return type - The return type is esp_err_t, which indicates the success or failure of the operation.

* Parameters - 

- parameter1 - uint8_t device_addr: The I2C address of the device to read from.

- parameter2 - uint8_t start_reg: The starting register address to read from.

- parameter3 - uint8_t *buffer: A pointer to the buffer to store the read data.

- parameter4 - size_t length: The number of bytes to read.

*/

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

/* Function Name - i2c_send_data_block

* Description - This function sends a block of data to an I2C device.

* Return type - The return type is esp_err_t, which indicates the success or failure of the operation.

* Parameters -

- parameter1 - uint8_t device_addr: The I2C address of the device to send data to.

- parameter2 - uint8_t *data: A pointer to the data buffer to send.

- parameter3 - size_t length: The length of the data buffer in bytes.

*/

esp_err_t i2c_send_data_block(uint8_t device_addr, uint8_t *data, size_t length)
{
    return i2c_master_write_to_device(I2C_MASTER_NUM,
                                      device_addr,
                                      data,
                                      length,
                                      pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}