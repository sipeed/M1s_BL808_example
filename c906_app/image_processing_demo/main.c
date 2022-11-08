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

void main()
{
    romfs_register();
    st7789v_spi_init();
    st7789v_spi_clear(0);

    {
#define PIN_BTN1 (22)
#define PIN_BTN2 (23)
        GLB_GPIO_Cfg_Type cfg;
        cfg.drive = 0;
        cfg.smtCtrl = 1;
        cfg.gpioFun = GPIO_FUN_GPIO;
        cfg.outputMode = 0;
        cfg.pullType = GPIO_PULL_NONE;

        cfg.gpioPin = PIN_BTN1;
        cfg.gpioMode = GPIO_MODE_INPUT;
        GLB_GPIO_Init(&cfg);

        cfg.gpioPin = PIN_BTN2;
        cfg.gpioMode = GPIO_MODE_INPUT;
        GLB_GPIO_Init(&cfg);
    }

    vTaskDelay(1);
    while (0 != bl_cam_mipi_yuv_init()) {
        printf("bl cam init fail!\r\n");
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    st7789v_spi_set_dir(1, 0);

    uint8_t *picture = NULL;
    uint32_t length = 0;
#define DISP_W (240)
#define DISP_H (240)
#define IMG_W (DISP_W + 2)
#define IMG_H (DISP_H + 2)
    static uint32_t image_buf[IMG_W * IMG_H];
    static uint16_t draw_buf[DISP_W * DISP_H];

    for (uint32_t i = 0;; i++) {
        uint64_t curr_cnt = CPU_Get_MTimer_US();
        while (0 != bl_cam_mipi_rgb_frame_get(&picture, &length)) {
        }
#define CAMERA_W (400)
#define CAMERA_H (300)
#define TARGET_WH (CAMERA_H)
        for (uint16_t ih = 0; ih < CAMERA_H; ih++) {
            memcpy(picture + sizeof(uint32_t) * ih * TARGET_WH,
                   picture + sizeof(uint32_t) * (ih * CAMERA_W + (CAMERA_W - CAMERA_H) / 2),
                   sizeof(uint32_t) * TARGET_WH);
        }
        BilinearInterpolation_RGBA8888(picture, TARGET_WH, TARGET_WH, image_buf, IMG_W, IMG_H);

        const int tans_mats[][3][3] = {
            {
                {0, 0, 0},
                {0, 1, 0},
                {0, 0, 0},
            },
            {
                {-1, -1, -1},
                {-1, 8, -1},
                {-1, -1, -1},
            },
            {
                {-1, -1, -1},
                {-1, 9, -1},
                {-1, -1, -1},
            },
            {
                {2, 0, 0},
                {0, -1, 0},
                {0, 0, -1},
            },
            {
                {-1, -1, 0},
                {-1, 0, 1},
                {0, 1, 1},
            },
        };
        static int k = 0;
        {
            {
                static bool btn1_last_pressed = false;
                static bool btn2_last_pressed = false;
                bool btn1_pressed = !GLB_GPIO_Read(PIN_BTN1);
                bool btn2_pressed = !GLB_GPIO_Read(PIN_BTN2);
                bool btn1_clicked = false;
                bool btn2_clicked = false;

                btn1_clicked = btn1_last_pressed && !btn1_pressed;
                btn2_clicked = btn2_last_pressed && !btn2_pressed;

                btn1_last_pressed = btn1_pressed;
                btn2_last_pressed = btn2_pressed;

                if (btn1_clicked | btn2_clicked) {
                    printf("k:%d, btn1: %u, btn2: %u\r\n", k, btn1_clicked, btn2_clicked);

                    k += (sizeof(tans_mats) / sizeof(tans_mats[0]));
                    k = (k + btn1_clicked - btn2_clicked) % (sizeof(tans_mats) / sizeof(tans_mats[0]));
                }
            }
            struct {
                uint32_t r : 8;
                uint32_t g : 8;
                uint32_t b : 8;
                uint32_t a : 8;
            } *prgba8888 = image_buf;
#define MAT(x, y) (prgba8888[(ih + 1 + (y)) * IMG_W + iw + 1 + (x)])

            for (uint32_t ih = 0; ih < DISP_W; ih++) {
                for (uint32_t iw = 0; iw < DISP_H; iw++) {
                    int32_t r = 0, g = 0, b = 0;

                    for (int i = 0; i < 3; i++) {
                        for (int j = 0; j < 3; j++) {
                            r += tans_mats[k][i][j] * MAT(i - 1, j - 1).r;
                            g += tans_mats[k][i][j] * MAT(i - 1, j - 1).g;
                            b += tans_mats[k][i][j] * MAT(i - 1, j - 1).b;
                        }
                    }

                    if (r < 0) r = 0;
                    if (g < 0) g = 0;
                    if (b < 0) b = 0;

                    prgba8888[ih * DISP_W + iw].r = r;
                    prgba8888[ih * DISP_W + iw].g = g;
                    prgba8888[ih * DISP_W + iw].b = b;
                    prgba8888[ih * DISP_W + iw].a = 0;
                }
            }

            csi_dcache_clean_range((uint64_t *)image_buf, DISP_W * DISP_H * sizeof(uint32_t));
#undef MAT
        }

        RGBA88882RGB565(image_buf, draw_buf, DISP_W, DISP_H);

        rgb565_frame_t f = {
            .w = DISP_W,
            .h = DISP_H,
            .raw = draw_buf,
        };

        draw_string(&f, 8, 0, "use btn to switch", 0xf800, 0x0000);
        char line_buffer[16];
        snprintf(line_buffer, sizeof(line_buffer), "%3d %3d %3d", tans_mats[k][0][0], tans_mats[k][0][1],
                 tans_mats[k][0][2]);
        draw_string(&f, 8, 1 * 16, line_buffer, 0x07e0, 0x0000);
        snprintf(line_buffer, sizeof(line_buffer), "%3d %3d %3d", tans_mats[k][1][0], tans_mats[k][1][1],
                 tans_mats[k][1][2]);
        draw_string(&f, 8, 2 * 16, line_buffer, 0x07e0, 0x0000);
        snprintf(line_buffer, sizeof(line_buffer), "%3d %3d %3d", tans_mats[k][2][0], tans_mats[k][2][1],
                 tans_mats[k][2][2]);
        draw_string(&f, 8, 3 * 16, line_buffer, 0x07e0, 0x0000);

        for (int i = 0; i < DISP_W * DISP_H; i++) {
            draw_buf[i] = __builtin_bswap16(draw_buf[i]);
        }
        uint16_t x1 = (280 - DISP_W) / 2;
        uint16_t y1 = (240 - DISP_H) / 2;
        uint16_t x2 = x1 + DISP_W - 1;
        uint16_t y2 = y1 + DISP_H - 1;
        st7789v_spi_draw_picture_nonblocking(x1, y1, x2, y2, draw_buf);

        bl_cam_mipi_frame_pop();
        vTaskDelay(5);
    }
    bl_cam_mipi_yuv_deinit();
}
