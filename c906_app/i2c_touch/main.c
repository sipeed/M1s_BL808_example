#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>

/* bl808 c906 std driver */
#include <bl808_glb.h>
#include <bl808_i2c.h>

static void cst816x_gpio_init(uint8_t i2c_scl_pin, uint8_t i2c_sda_pin, uint8_t cts_rst_pin, uint8_t cts_int_pin)
{
    GLB_GPIO_Cfg_Type cfg;
    cfg.pullType = GPIO_PULL_UP;
    cfg.drive = 0;
    cfg.smtCtrl = 1;

    cfg.gpioMode = GPIO_MODE_AF;
    cfg.gpioFun = GPIO_FUN_I2C2;
    cfg.gpioPin = i2c_scl_pin;
    GLB_GPIO_Init(&cfg);
    cfg.gpioPin = i2c_sda_pin;
    GLB_GPIO_Init(&cfg);

    cfg.outputMode = GPIO_OUTPUT_VALUE_MODE;
    cfg.gpioMode = GPIO_MODE_OUTPUT;
    cfg.gpioFun = GPIO_FUN_GPIO;
    cfg.gpioPin = cts_rst_pin;
    GLB_GPIO_Init(&cfg);

    cfg.gpioMode = GPIO_MODE_INPUT;
    cfg.gpioFun = GPIO_FUN_GPIO;
    cfg.gpioPin = cts_int_pin;
    GLB_GPIO_Init(&cfg);
}

#include "cst816x_reg.h"

#define PIN_CTP_RST (24)
#define PIN_CTP_INT (32)

#define PIN_CTP_SCK (6)
#define PIN_CTP_SDA (7)

void main()
{
    cst816x_gpio_init(PIN_CTP_SCK, PIN_CTP_SDA, PIN_CTP_RST, PIN_CTP_INT);
    uint8_t buf[6];
    I2C_Transfer_Cfg cfg;
    cfg.slaveAddr10Bit = false;
    cfg.stopEveryByte = false;
    cfg.slaveAddr = CST816X_DEV_ADDR;
    cfg.clk = 40000;

    cfg.subAddr = CST816X_REG_CHIPID;
    cfg.subAddrSize = 1;
    cfg.dataSize = 4;
    cfg.data = buf;
    /* hardware reset cst816x */
    GLB_GPIO_Write(PIN_CTP_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    GLB_GPIO_Write(PIN_CTP_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    if (SUCCESS == I2C_MasterReceiveBlocking(I2C0_MM_ID, &cfg)) {
        uint8_t chip_id = buf[0];
        uint8_t proj_id = buf[1];
        uint8_t fw_ver = buf[2];
        uint8_t factory_id = buf[3];
        printf(
            "[cst816x]\r\n"
            "\tchip_id: %d\r\n"
            "\tproj_id: %d\r\n"
            "\tfw_ver: %d\r\n"
            "\tfactory_id: %d\r\n"
            "\r\n",
            chip_id, proj_id, fw_ver, factory_id);
    } else {
        printf("timeout?\r\n");
    }

    cfg.subAddr = CST816X_REG_GESTUREID;
    cfg.subAddrSize = 1;
    cfg.dataSize = 6;
    cfg.data = buf;
    for (bool ctp_active_flag = false;;) {
        if (!GLB_GPIO_Read(PIN_CTP_INT)) {
            ctp_active_flag = true;
        }
        if (ctp_active_flag && SUCCESS == I2C_MasterReceiveBlocking(I2C0_MM_ID, &cfg)) {
            uint8_t status = buf[2] >> 4;
            uint16_t pos_x = buf[2] & 0x0F;
            pos_x = (pos_x << 8) | buf[3];
            uint16_t pos_y = buf[4] & 0x0F;
            pos_y = (pos_y << 8) | buf[5];
            printf("pos: (%-3u, %-3u)[%u]\r\n", pos_x, pos_y, status);
        } else {
            ctp_active_flag = false;
        }
        vTaskDelay(pdMS_TO_TICKS(8));
    }
}