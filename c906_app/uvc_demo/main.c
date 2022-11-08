#include <stdbool.h>
#include <stdio.h>

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>

/* bl808 c906 std driver */
#include <bl808_glb.h>
#include <bl_cam.h>

#include <m1s_c906_xram_usb.h>

void main()
{
    vTaskDelay(1);
    bl_cam_mipi_mjpeg_init();
    m1s_xram_usb_cam_init();
}