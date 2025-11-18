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

#define MPU6050_ADDR                0x68       // AD0 = GND
#define MPU6050_WHO_AM_I_REG        0x75
#define MPU6050_PWR_MGMT_1          0x6B
#define MPU6050_ACCEL_XOUT_H        0x3B       // Register address to begin with

static const char *TAG = "MPU6050";

esp_err_t i2c_master_init()
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

esp_err_t mpu6050_write_byte(uint8_t reg_addr, uint8_t data)
{
    uint8_t buf[2] = { reg_addr, data };
    return i2c_master_write_to_device(I2C_MASTER_NUM,
                                      MPU6050_ADDR,
                                      buf,
                                      sizeof(buf),
                                      1000 / portTICK_PERIOD_MS);
}

esp_err_t mpu6050_read_bytes(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM,
                                        MPU6050_ADDR,
                                        &reg_addr,
                                        1,
                                        data,
                                        len,
                                        1000 / portTICK_PERIOD_MS);
}

void app_main()
{
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized");

    // Wake up MPU6050 (clear sleep bit)
    ESP_ERROR_CHECK(mpu6050_write_byte(MPU6050_PWR_MGMT_1, 0x00));

    while (1) {
        uint8_t data[14];
        ESP_ERROR_CHECK(mpu6050_read_bytes(MPU6050_ACCEL_XOUT_H, data, sizeof(data)));

        int16_t ax = (data[0] << 8) | data[1];
        int16_t ay = (data[2] << 8) | data[3];
        int16_t az = (data[4] << 8) | data[5];
        int16_t gx = (data[8] << 8) | data[9];
        int16_t gy = (data[10] << 8) | data[11];
        int16_t gz = (data[12] << 8) | data[13];

        printf("Accel: X=%d Y=%d Z=%d | Gyro: X=%d Y=%d Z=%d\n",
               ax, ay, az, gx, gy, gz);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
