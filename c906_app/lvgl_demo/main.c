#include <stdbool.h>
#include <stdio.h>

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>

/* bl808 c906 std driver */
#include <bl808_glb.h>

#include "demos/lv_demos.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lvgl.h"

static void lvgl_task(void *param)
{
    while (1) {
        lv_task_handler();
        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}

void main()
{
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    lv_demo_benchmark();
    lv_task_handler();

    xTaskCreate(lvgl_task, (char *)"lvgl task", 512, NULL, 15, NULL);
}