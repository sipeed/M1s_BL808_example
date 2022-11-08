#include <stdbool.h>
#include <stdio.h>

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>

/* bl808 c906 std driver */
#include <bl808_glb.h>

#define PIN_LED (8)
#define PIN_BTN1 (22)
#define PIN_BTN2 (23)

void main()
{
    GLB_GPIO_Cfg_Type cfg;
    cfg.drive = 0;
    cfg.smtCtrl = 1;
    cfg.gpioFun = GPIO_FUN_GPIO;
    cfg.outputMode = 0;
    cfg.pullType = GPIO_PULL_NONE;

    cfg.gpioPin = PIN_LED;
    cfg.gpioMode = GPIO_MODE_OUTPUT;
    GLB_GPIO_Init(&cfg);

    cfg.gpioPin = PIN_BTN1;
    cfg.gpioMode = GPIO_MODE_INPUT;
    GLB_GPIO_Init(&cfg);

    cfg.gpioPin = PIN_BTN2;
    cfg.gpioMode = GPIO_MODE_INPUT;
    GLB_GPIO_Init(&cfg);

    for (bool gpio_state = false;;) {
        GLB_GPIO_Write(PIN_LED, !gpio_state);
        bool btn1_pressed = !GLB_GPIO_Read(PIN_BTN1);
        bool btn2_pressed = !GLB_GPIO_Read(PIN_BTN2);
        if (btn1_pressed && btn2_pressed) {
            printf("btn1 and btn2 are pressed, led blink during 500ms\r\n");
            gpio_state ^= true;
            vTaskDelay(pdMS_TO_TICKS(490));
        } else if (btn1_pressed) {
            printf("btn1 is pressed, led always turn on\r\n");
            gpio_state = true;
        } else if (btn2_pressed) {
            printf("btn2 is pressed, led always turn off\r\n");
            gpio_state = false;
        } else {
            gpio_state = gpio_state;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}