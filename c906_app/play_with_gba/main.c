
#include <stdbool.h>
#include <stdio.h>

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>

/* bl808 c906 std driver */
#include <bl808_glb.h>

#include "bl808_glb.h"
#include "lcd.h"

// "a", "b", "select", "start", "right", "left", "up", "down", "r", "l",
// "turbo", "menu"
#define KEY_A_PIN (30)
#define KEY_B_PIN (31)
#define KEY_SELECT_PIN (22)
#define KEY_START_PIN (23)
#define KEY_UP_PIN (6)
#define KEY_DOWN_PIN (7)
#define KEY_LEFT_PIN (41)
#define KEY_RIGHT_PIN (40)
#define KEY_R_PIN (0xff)
#define KEY_L_PIN (0xff)
#define KEY_TRUBO_PIN (0xff)
#define KEY_MENU_PIN (0xff)
uint8_t osKeyMap[12] = {KEY_A_PIN,  KEY_B_PIN,    KEY_SELECT_PIN, KEY_START_PIN, KEY_LEFT_PIN,  KEY_RIGHT_PIN,
                        KEY_UP_PIN, KEY_DOWN_PIN, KEY_R_PIN,      KEY_L_PIN,     KEY_TRUBO_PIN, KEY_MENU_PIN};
uint32_t bl808_key_read()
{
    uint32_t ret = 0;
    for (int i = 0; i < 12; i++) {
        if (osKeyMap[i] != 0xff) {
            int res = GLB_GPIO_Read(osKeyMap[i]);
            if (res == 0) {
                ret |= 1 << i;
            }
            // printf("Key[%d]  %d\r\n", osKeyMap[i], res);
        }
    }
    return ret;
}

void bl808_key_init()
{
    GLB_GPIO_Cfg_Type cfg;
    cfg.drive = 0;
    cfg.smtCtrl = 1;
    cfg.gpioFun = GPIO_FUN_GPIO;
    cfg.gpioMode = GPIO_MODE_INPUT;
    cfg.pullType = GPIO_PULL_UP;
    GLB_GPIO_Init(&cfg);
    for (int i = 0; i < 12; i++) {
        cfg.gpioPin = osKeyMap[i];
        if (cfg.gpioPin != 0xff) {
            GLB_GPIO_Input_Enable(cfg.gpioPin);
            GLB_GPIO_Init(&cfg);
        }
    }
}

void emuMainLoop();

int main()
{
    /*
      You need to download the gba file to address 0x800000, the program will load the gba into the memory, If the gba
      file is incorrect, the program may fail to execute, see the emuMainLoop() function for details
    */
    bl808_key_init();
    st7789v_spi_init();
    st7789v_spi_set_dir(1, 0);
    st7789v_spi_clear(0x0);
    emuMainLoop();
    return 0;
}
