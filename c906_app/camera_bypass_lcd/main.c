#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>

/* lcd */
#include <lcd.h>

/* bl808 c906 std driver */
#include <bl808_glb.h>
#include <bl808_spi.h>

/* bl808 c906 hosal driver */
#include <bl_cam.h>

/* sipeed utils */
#include <imgtool/bilinear_interpolation.h>

#include "m1s_c906_xram_pwm.h"

#define PWM_PORT (0)
#define PWM_PIN (11)

void backlight_init(void)
{
    m1s_xram_pwm_init(PWM_PORT, PWM_PIN, 2000, 25);
    m1s_xram_pwm_start(PWM_PORT, PWM_PIN);
}

static uint16_t draw_buf[280 * 240];

void main()
{
    backlight_init();
    st7789v_spi_init();
    st7789v_spi_set_dir(1, 0);
    if (0 != bl_cam_mipi_rgb565_init()) {
        printf("bl cam init failed!\r\n");
        return;
    }

    uint16_t *picture = NULL;
    uint32_t length = 0, loop_cnt = 0;
    while (1) {
        // 1. snap camera frame
        uint64_t curr_cnt = CPU_Get_MTimer_US();
        while (0 != bl_cam_mipi_rgb565_frame_get((uint8_t **)&picture, &length)) {
            vTaskDelay(1);
        }
        printf("[%8u]get mipi rgb565 frame 0x%08x size %ubytes cost %fms ", loop_cnt, (uint32_t)picture, length, (float)(CPU_Get_MTimer_US() - curr_cnt) / 1000);

        // 2. draw camera frame to lcd
        BilinearInterpolation_RGB565(picture, 400, 300, draw_buf, 280, 240);
        curr_cnt = CPU_Get_MTimer_US();
        for (int i = 0; i < 280 * 240; i++) {
            draw_buf[i] = __builtin_bswap16(draw_buf[i]);
        }
        st7789v_spi_draw_picture_nonblocking(0, 0, 279, 239, draw_buf);
        printf("and lcd_flush cost %fms\r\n", (float)(CPU_Get_MTimer_US() - curr_cnt) / 1000);

        // 3. camera frame pop
        bl_cam_mipi_rgb565_frame_pop();
        vTaskDelay(1);
        loop_cnt++;
    }
    bl_cam_mipi_rgb565_deinit();
}