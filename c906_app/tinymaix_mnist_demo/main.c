#include <stdbool.h>
#include <stdio.h>

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>

/* fs */
#include <bl_romfs.h>

/* lcd */
#include <lcd.h>

/* bl808 c906 std driver */
#include <bl808_glb.h>
#include <bl808_spi.h>

/* bl808 c906 hosal driver */
#include <bl_cam.h>

/* sipeed utils */
#include <imgtool/bilinear_interpolation.h>
#include <imgtool/rgb_cvt.h>
#include <mathtool/argmax.h>

#include "model_util.h"

// #define OPT_DEBUG
// #define STATIC_INPUT

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
    extern const uint8_t ascii_3216[][64];
    c -= ' ';
    printf("draw: %u\r\n", c);
    for (int i = 0; i < 32; i++) {
        for (int t = 0; t < 8; t++) {
            frame->raw[(y + i) * frame->w + (x + 7 - t)] = font_color[(ascii_3216[c][i * 2] >> t) & 0x01];
            frame->raw[(y + i) * frame->w + (x + 15 - +t)] = font_color[(ascii_3216[c][i * 2 + 1] >> t) & 0x01];
        }
    }
}

static inline void draw_string(rgb565_frame_t *frame, uint16_t x, uint16_t y, const char *str, uint16_t front_color,
                               uint16_t back_color)
{
    uint8_t str_len = strlen(str);
    if (str_len + x > frame->w) return;
    for (int i = 0; i < str_len; i++) {
        draw_char(frame, x + 16 * i, y, str[i], front_color, back_color);
    }
}

static void mbv2_model_out_cb(model_out_t *out, void *arg)
{
    float *output = out->output;
    float output_scale = out->output_scale;
    int output_zero_point = out->output_zero_point;
    uint32_t output_size = out->output_size;

    uint32_t idx = ARGMAX(output, output_size);
    printf("[%u/%u]output %d, %f:\r\n", idx, output_size, output_zero_point, output_scale);
    printf("label: %c\r\n", idx + '0');

    printf("res=\r\n\t[ %.3f", ((float *)output)[0]);
    for (uint32_t i = 1; i < output_size; i++) {
        printf(", %.3f", ((float *)output)[i]);
        if ((i & 0xf) == 0xf) printf(",\r\n\t");
    }
    printf("]\r\n\r\n");

    rgb565_frame_t *f = arg;
    if (output[idx] > 0.7) draw_char(f, f->w / 2, 2, idx + ' ', 0x07e0, 0x0000);
}

void main()
{
    romfs_register();
    st7789v_spi_init();
    st7789v_spi_clear(0);
#ifndef STATIC_INPUT
    vTaskDelay(1);
    while (0 != bl_cam_mipi_yuv_init()) {
        printf("bl cam init fail!\r\n");
        vTaskDelay(pdMS_TO_TICKS(5));
    }
#endif
    st7789v_spi_set_dir(1, 0);

    uint8_t *picture = NULL;
    uint32_t length = 0;
#define DISP_W (240)
#define DISP_H (240)
#define IMG_W (28)
#define IMG_H (28)
    static uint32_t input_buf[IMG_W * IMG_H];
    static uint16_t draw_buf[DISP_W * DISP_H];
    void *mdl = load_model("/romfs/mnist_nnom.bin");

#ifdef STATIC_INPUT
    char *pic_addr = NULL;
    get_file_from_romfs("/romfs/3.bin", &pic_addr);
#endif

    for (uint32_t i = 0;; i++) {
#ifdef OPT_DEBUG
        uint64_t curr_cnt = CPU_Get_MTimer_US();
#endif

#ifndef STATIC_INPUT
        while (0 != bl_cam_mipi_rgb_frame_get(&picture, &length)) {
        }
#endif

#ifdef OPT_DEBUG
        printf("[%8u]get mipi rgb0(0x%8x) cost %fms\r\n", i, (uint32_t)picture,
               (float)(CPU_Get_MTimer_US() - curr_cnt) / 1000);
        curr_cnt = CPU_Get_MTimer_US();
#endif

#ifdef STATIC_INPUT
        memcpy(input_buf, pic_addr, IMG_W * IMG_H * 1);
#else
#define CAMERA_W (400)
#define CAMERA_H (300)
#define TARGET_WH (CAMERA_H)
        for (uint16_t ih = 0; ih < CAMERA_H; ih++) {
            memcpy(picture + sizeof(uint32_t) * ih * TARGET_WH,
                   picture + sizeof(uint32_t) * (ih * CAMERA_W + (CAMERA_W - CAMERA_H) / 2),
                   sizeof(uint32_t) * TARGET_WH);
        }
#define CROP_W (4 * IMG_W)
#define CROP_H (4 * IMG_H)
#define CROP_X ((TARGET_WH - CROP_W) / 2)
#define CROP_Y ((TARGET_WH - CROP_H) / 2)
        for (uint32_t y = 0; y < CROP_H; y++) {
            memcpy(picture + sizeof(uint32_t) * (TARGET_WH * TARGET_WH + y * CROP_W),
                   picture + sizeof(uint32_t) * (TARGET_WH * (CROP_Y + y) + CROP_X), sizeof(uint32_t) * CROP_W);
        }
        BilinearInterpolation_RGBA8888(picture + sizeof(uint32_t) * TARGET_WH * TARGET_WH, CROP_W, CROP_H, input_buf,
                                       IMG_W, IMG_H);
        RGBA88882GRAY(input_buf, input_buf, IMG_W, IMG_H);
        for (uint32_t i = 0; i < IMG_H * IMG_W; i++) {
            ((uint8_t *)input_buf)[i] = ~((uint8_t *)input_buf)[i];
            if (((uint8_t *)input_buf)[i] < 80) ((uint8_t *)input_buf)[i] = 0;
        }
#endif

        for (uint32_t ih = 0; ih < IMG_H; ih++) {
            printf("%u\t[ %3u", ih, ((uint8_t *)input_buf)[ih * IMG_W]);
            for (uint32_t iw = 1; iw < IMG_W; iw++) {
                printf(", %3u", ((uint8_t *)input_buf)[ih * IMG_W + iw]);
            }
            printf("],\r\n");
        }
        csi_dcache_clean_range((uint64_t *)input_buf, IMG_W * IMG_H * 1);

#ifdef OPT_DEBUG
        printf("and convert img cost %fms\r\n", (float)(CPU_Get_MTimer_US() - curr_cnt) / 1000);
        curr_cnt = CPU_Get_MTimer_US();
#endif

#ifndef STATIC_INPUT
        {
            for (uint32_t x = 0; x < CROP_W + 2; x++) {
                ((uint32_t *)picture)[TARGET_WH * (CROP_Y - 1) + CROP_X - 1 + x] = 0xff;
                ((uint32_t *)picture)[TARGET_WH * (CROP_Y + CROP_H) + CROP_X - 1 + x] = 0xff;
            }
            for (uint32_t y = 0; y < CROP_H; y++) {
                ((uint32_t *)picture)[TARGET_WH * (CROP_Y + y) + CROP_X - 1] = 0xff;
                ((uint32_t *)picture)[TARGET_WH * (CROP_Y + y) + CROP_X + CROP_W] = 0xff;
            }
        }
        BilinearInterpolation_RGBA88882RGB565(picture, TARGET_WH, TARGET_WH, draw_buf, DISP_W, DISP_H);
#else
        for (int ih = IMG_H - 1; ih >= 0; ih--) {
            for (int iw = IMG_W - 1; iw >= 0; iw--) {
                struct {
                    uint32_t r : 8;
                    uint32_t g : 8;
                    uint32_t b : 8;
                    uint32_t a : 8;
                } *rgba8888 = &input_buf[ih * IMG_W + iw];
                uint8_t gray = ((uint8_t *)input_buf)[ih * IMG_W + iw];
                rgba8888->r = gray;
                rgba8888->g = gray;
                rgba8888->b = gray;
                rgba8888->a = 0;
            }
        }
        BilinearInterpolation_RGBA88882RGB565(input_buf, IMG_W, IMG_H, draw_buf, DISP_W, DISP_H);
#endif

#ifdef OPT_DEBUG
        printf("and convert disp cost %fms\r\n", (float)(CPU_Get_MTimer_US() - curr_cnt) / 1000);
        curr_cnt = CPU_Get_MTimer_US();
#endif
        rgb565_frame_t f = {
            .w = DISP_W,
            .h = DISP_H,
            .raw = draw_buf,
        };

        model_forward(mdl, input_buf, 10, mbv2_model_out_cb, &f);

#ifdef OPT_DEBUG
        printf("and blai cost %fms\r\n", (float)(CPU_Get_MTimer_US() - curr_cnt) / 1000);
        curr_cnt = CPU_Get_MTimer_US();
#endif

        for (uint32_t i = 0; i < DISP_W * DISP_H; i++) {
            draw_buf[i] = __builtin_bswap16(draw_buf[i]);
        }
        uint16_t x1 = (280 - DISP_W) / 2;
        uint16_t y1 = (240 - DISP_H) / 2;
        uint16_t x2 = x1 + DISP_W - 1;
        uint16_t y2 = y1 + DISP_H - 1;
        st7789v_spi_draw_picture_nonblocking(x1, y1, x2, y2, draw_buf);

#ifdef OPT_DEBUG
        printf("and lcd_flush cost %fms\r\n", (float)(CPU_Get_MTimer_US() - curr_cnt) / 1000);
#endif

#ifndef STATIC_INPUT
        bl_cam_mipi_frame_pop();
#endif
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    bl_cam_mipi_yuv_deinit();
}
