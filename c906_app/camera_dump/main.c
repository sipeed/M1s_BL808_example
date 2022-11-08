#include <stdbool.h>
#include <stdio.h>

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>

/* bl808 c906 std driver */
#include <bl808_glb.h>

/* bl808 c906 hosal driver */
#include <bl_cam.h>

void main()
{
    while (0 != bl_cam_mipi_rgb565_init()) {
        printf("bl cam init fail!\r\n");
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    uint8_t *picture = NULL;
    uint32_t length = 0;
    for (int i = 0; i < 6; i++) {
        uint64_t curr_cnt = CPU_Get_MTimer_US();
        while (0 != bl_cam_mipi_rgb565_frame_get(&picture, &length)) {
        }
        printf("get mipi rgb565 frame %p size %ubytes cost %fms\r\n", picture, length,
               (double)(CPU_Get_MTimer_US() - curr_cnt) / 1000);
        printf("dump binary memory picture%02d.bin 0x%08x 0x%08x length:%d\r\n", i, (uint32_t)picture,
               (uint32_t)picture + length, length);
        bl_cam_mipi_rgb565_frame_pop();
        vTaskDelay(5);
    }
    bl_cam_mipi_rgb565_deinit();
}