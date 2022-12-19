#include <stdbool.h>
#include <stdio.h>

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>

/* fs */
// #include <bl_romfs.h>
#include <fatfs.h>

/* lcd */
#include <lcd.h>

/* bl808 c906 std driver */
#include <bl808_glb.h>
#include <bl808_spi.h>

/* bl808 c906 hosal driver */
#include <bl_cam.h>

/* m1s utils */
#include <imgtool/bilinear_interpolation.h>
#include <imgtool/rgb_cvt.h>
#include <mathtool/argmax.h>

#if 0
#define DBG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DBG_PRINTF(...)
#endif

typedef struct {
    uint16_t w;
    uint16_t h;
    uint16_t *raw;
} rgb565_frame_t;

static inline void draw_char(rgb565_frame_t *frame, uint16_t x, uint16_t y, uint8_t c, uint16_t front_color,
                             uint16_t back_color)
{
    uint16_t font_color[2] = {back_color, front_color};
    extern const uint8_t ascii_1608[][16];
    c -= ' ';
    for (int i = 0; i < 16; i++) {
        for (int t = 0; t < 8; t++) {
            frame->raw[(y + i) * frame->w + (x + t)] = font_color[(ascii_1608[c][i] >> t) & 0x01];
        }
    }
}

static inline void draw_string(rgb565_frame_t *frame, uint16_t x, uint16_t y, const char *str, uint16_t front_color,
                               uint16_t back_color)
{
    uint8_t str_len = strlen(str);
    if (str_len + x > frame->w) return;
    for (int i = 0; i < str_len; i++) {
        draw_char(frame, x + 8 * i, y, str[i], front_color, back_color);
    }
}

static inline void draw_rect_filled(rgb565_frame_t *frame, uint16_t sx, uint16_t sy, uint16_t w, uint16_t h,
                                    uint16_t color)
{
    uint16_t tx = sx + w, ty = sy + h;
    if (tx > frame->w - 1) tx = frame->w - 1;
    if (ty > frame->h - 1) ty = frame->h - 1;
    for (uint16_t y = sy; y < ty; y++) {
        for (uint16_t x = sx; x < tx; x++) {
            frame->raw[y * frame->w + x] = color;
        }
    }
}

#include "m1s_model_runner.h"
static void tj_model_result_cb(model_result_t *result, void *arg)
{
    uint8_t *output = result->output;
    float output_scale = result->scale;
    int output_zero_point = result->zero_point;
    uint32_t *output_shape = result->output_shape;
    // printf("output shape %ux%ux%u\r\n", output_shape[0], output_shape[1], output_shape[2]);

    uint32_t output_size = output_shape[0] * output_shape[1] * output_shape[2];
    uint32_t idx = ARGMAX(output, output_size);
    printf("[%u/%u]output %d, %f:", idx, output_size, output_zero_point, output_scale);
    DBG_PRINTF("\t[%3u", output[0]);
    for (uint32_t i = 1; i < output_size; i++) {
        DBG_PRINTF(",%3u", output[i]);
        if ((i & 0xf) == 0xf) DBG_PRINTF(",\r\n\t");
    }
    DBG_PRINTF("]");

    rgb565_frame_t *f = arg;
    draw_string(f, f->w / 2, 16, idx ? "t o m" : "jerry", 0x07e0, 0x0000);
    printf("\r\n");
}

void main()
{
    fatfs_register();

    { /* SPI LCD init... */
        DBG_PRINTF("[start] SPI LCD init..\r\n");
        st7789v_spi_init();
        st7789v_spi_clear(0);
        DBG_PRINTF("[done] SPI LCD init.\r\n");
    }

    { /* Camera init */
        DBG_PRINTF("[start] Camera init..\r\n");
        vTaskDelay(1);
        if (0 != bl_cam_mipi_yuv_init()) {
            printf("[failed] Camera init!\r\n");
            printf("PLEASE RESET BOARD TO TRY AGAIN!!!\r\n");
        }
        DBG_PRINTF("[done] Camera init.\r\n");
    }

#define DISP_W (240)
#define DISP_H (240)
    static uint16_t s_lcd_fb[DISP_W * DISP_H];

#define IMG_W (224)
#define IMG_H (224)
    static uint32_t input_buf[IMG_W * IMG_H];

    if (SM_OK != m1s_model_load("/flash/models/tj.blai")) {
        printf("[failed] load model\r\n");
        return;
    }

    for (uint64_t __loop_count = 0;; __loop_count++) {
        DBG_PRINTF("[%u] loop..\r\n", __loop_count);

        { /* fetch camera picture */
            DBG_PRINTF("[start] fetch camera picture..\r\n");
            uint8_t *picture = NULL;
            uint32_t length = 0;

            while (0 != bl_cam_mipi_rgb_frame_get(&picture, &length)) {
            }

#define CAMERA_W (400)
#define CAMERA_H (300)
#define TARGET_WH (CAMERA_H)
            for (uint32_t ih = 0; ih < CAMERA_H; ih++) {
                memcpy(picture + sizeof(uint32_t) * ih * TARGET_WH,
                       picture + sizeof(uint32_t) * (ih * CAMERA_W + (CAMERA_W - CAMERA_H) / 2),
                       sizeof(uint32_t) * CAMERA_H);
            }

            BilinearInterpolation_RGBA8888(picture, TARGET_WH, TARGET_WH, input_buf, IMG_W, IMG_H);

            /* feed model */
            m1s_model_feed(input_buf);

            BilinearInterpolation_RGBA88882RGB565(picture, TARGET_WH, TARGET_WH, s_lcd_fb, DISP_W, DISP_H);
            bl_cam_mipi_frame_pop();
            DBG_PRINTF("[done] fetch camera picture..\r\n");
        }

        rgb565_frame_t f = {
            .w = DISP_W,
            .h = DISP_H,
            .raw = s_lcd_fb,
        };

        /* forward and post process */
        m1s_model_forward(tj_model_result_cb, &f, NULL);

        { /* send fb to lcd */
            for (int i = 0; i < DISP_W * DISP_H; i++) {
                s_lcd_fb[i] = __builtin_bswap16(s_lcd_fb[i]);
            }
            uint16_t x1 = (280 - DISP_W) / 2;
            uint16_t y1 = (240 - DISP_H) / 2;
            uint16_t x2 = x1 + DISP_W - 1;
            uint16_t y2 = y1 + DISP_H - 1;
            st7789v_spi_draw_picture_nonblocking(x1, y1, x2, y2, s_lcd_fb);
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
    bl_cam_mipi_yuv_deinit();
    m1s_model_unload();
}
