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

/* bl808 ai */
#include <blai_core.h>

/* aos */
#include <aos/kernel.h>
#include <vfs.h>

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

#define ALIGNUP(x, r) (((x) + ((r)-1)) & ~((r)-1))

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

#define IMG_W (28)
#define IMG_H (28)
    static uint32_t input_buf[IMG_W * IMG_H];

    BLAI_Model_t *s_blai_model = NULL;
    { /* load model */
        s_blai_model = blai_create();
        if (NULL == s_blai_model) {
            printf("[failed] create blai handler\r\n");
            return;
        }

        int fd = -1;
        if (0 > (fd = aos_open("/flash/models/mnist.blai", 0))) {
            printf("[failed] open model file\r\n");
            return;
        }

        int model_bin_len = 0;
        uint8_t *model_bin_buf = NULL;

        model_bin_len = aos_lseek(fd, 0, SEEK_END);
        if (0 >= model_bin_len) {
            aos_close(fd);
            printf("[failed] the length of model file is less then zero\r\n");
            return;
        }

        if (NULL == (model_bin_buf = malloc(model_bin_len))) {
            aos_close(fd);
            printf("[failed] malloc buffer to read model\r\n");
            return;
        }

        aos_lseek(fd, 0, SEEK_SET);
        if (model_bin_len != aos_read(fd, model_bin_buf, model_bin_len)) {
            aos_close(fd);
            printf("[failed] err occur while reading model\r\n");
            return;
        }
        aos_close(fd);

        if (BLAI_STATUS_NO_ERROR != blai_load_model_from_buffer(s_blai_model, (uint8_t *)model_bin_buf)) {
            blai_free(s_blai_model);
            s_blai_model = NULL;
            free(model_bin_buf);
            printf("[failed] load model to npu\r\n");
            return;
        }
        free(model_bin_buf);
    }

    { /* show inout shape */
        struct blai_net_info_t *net = s_blai_model->net;
        uint32_t total_layer_cnt = net->layer_cnt;
        uint32_t h, w, c;
        h = net->h;
        w = net->w;
        c = net->c;
        printf("raw input shape %ux%ux%u\r\n", h, w, c);
        printf("real input shape %ux%ux%u\r\n", h, w, (c) == 1 ? (c) : ALIGNUP((c), 4));
        h = net->layers[total_layer_cnt - 1].out_h;
        w = net->layers[total_layer_cnt - 1].out_w;
        c = net->layers[total_layer_cnt - 1].out_c;
        printf("raw output shape %ux%ux%u\r\n", h, w, c);
        printf("real output shape %ux%ux%u\r\n", h, w, (c) == 1 ? (c) : ALIGNUP((c), 4));
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

#define CROP_W (4 * IMG_W)
#define CROP_H (4 * IMG_H)
#define CROP_X ((TARGET_WH - CROP_W) / 2)
#define CROP_Y ((TARGET_WH - CROP_H) / 2)
            { /* crop */
                for (uint32_t y = 0; y < CROP_H; y++) {
                    memcpy(picture + sizeof(uint32_t) * (TARGET_WH * TARGET_WH + y * CROP_W),
                           picture + sizeof(uint32_t) * (TARGET_WH * (CROP_Y + y) + CROP_X), sizeof(uint32_t) * CROP_W);
                }

                BilinearInterpolation_RGBA8888(picture + sizeof(uint32_t) * TARGET_WH * TARGET_WH, CROP_W, CROP_H,
                                               input_buf, IMG_W, IMG_H);
                RGBA88882GRAY(input_buf, input_buf, IMG_W, IMG_H);

                uint8_t *input_buf_u8 = (uint8_t *)input_buf;
                for (uint32_t i = 0; i < IMG_H * IMG_W; i++) {
                    input_buf_u8[i] = ~input_buf_u8[i];
                    if (input_buf_u8[i] < 128) input_buf_u8[i] = 0;
                }

                DBG_PRINTF("\r\n");
                for (uint32_t ih = 0; ih < IMG_H; ih++) {
                    DBG_PRINTF("\t[%3u", input_buf_u8[ih * IMG_W]);
                    for (uint32_t iw = 1; iw < IMG_W; iw++) {
                        DBG_PRINTF(",%3u", input_buf_u8[ih * IMG_W + iw]);
                    }
                    DBG_PRINTF("],\r\n");
                }
            }

            { /* feed model */
                uint8_t *p_input = s_blai_model->buffer;
                struct blai_net_info_t *net = s_blai_model->net;
                uint32_t feed_size = net->w * net->h * ((net->c) == 1 ? (net->c) : ALIGNUP((net->c), 4));
                memcpy(p_input, input_buf, feed_size);
                csi_dcache_clean_range((uint8_t *)p_input, feed_size);
            }

            { /* draw cropped edge */
                for (uint32_t x = 0; x < CROP_W + 2; x++) {
                    ((uint32_t *)picture)[TARGET_WH * (CROP_Y - 1) + CROP_X - 1 + x] = 0xff;
                    ((uint32_t *)picture)[TARGET_WH * (CROP_Y + CROP_H) + CROP_X - 1 + x] = 0xff;
                }
                for (uint32_t y = 0; y < CROP_H; y++) {
                    ((uint32_t *)picture)[TARGET_WH * (CROP_Y + y) + CROP_X - 1] = 0xff;
                    ((uint32_t *)picture)[TARGET_WH * (CROP_Y + y) + CROP_X + CROP_W] = 0xff;
                }
            }

            BilinearInterpolation_RGBA88882RGB565(picture, TARGET_WH, TARGET_WH, s_lcd_fb, DISP_W, DISP_H);
            bl_cam_mipi_frame_pop();
            DBG_PRINTF("[done] fetch camera picture..\r\n");
        }

        rgb565_frame_t f = {
            .w = DISP_W,
            .h = DISP_H,
            .raw = s_lcd_fb,
        };

        { /* forward and post process */
            blai_inst_inference(s_blai_model);

            struct blai_net_info_t *net = s_blai_model->net;
            uint32_t total_layer_cnt = net->layer_cnt;
            uint32_t offset = net->layers[total_layer_cnt - 1].out_layer_mem * net->patch_size;
            uint8_t *output = s_blai_model->buffer + offset;

            float output_scale = net->layers[total_layer_cnt - 1].output_scale;
            int output_zero_point = net->layers[total_layer_cnt - 1].tf_output_offset;
            uint32_t h = net->layers[total_layer_cnt - 1].out_h;
            uint32_t w = net->layers[total_layer_cnt - 1].out_w;
            uint32_t c = net->layers[total_layer_cnt - 1].out_c;
            uint32_t output_size = h * w * c;

            uint32_t idx = ARGMAX(output, output_size);
            printf("[%u/%u]output %d, %f", idx, output_size, output_zero_point, output_scale);

            rgb565_frame_t *pf = &f;
            draw_char(pf, pf->w / 2, 16, idx + '0', 0x07e0, 0x0000);

            DBG_PRINTF(":\t[%3u", output[0]);
            for (uint32_t i = 1; i < output_size; i++) {
                DBG_PRINTF(",%3u", output[i]);
                if ((i & 0xf) == 0xf) DBG_PRINTF(",\r\n\t");
            }
            DBG_PRINTF("]");

            printf("\r\n");
        }

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
    blai_free(s_blai_model);
    s_blai_model = NULL;
}
