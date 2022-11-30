#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gba.h"
#include "globals.h"
#include "memory.h"
#include "sound.h"

extern "C" {
#include "lcd.h"
}

static int frameDrawn = 0;
static uint32_t frameCount = 0;

void systemDrawScreen(void)
{
    static uint16_t FB[240 * 256];
    frameDrawn = 1;
    frameCount++;
    // return;
    // if (frameCount % 2) {
    //     return;
    // }

    uint16_t *src = pix;  // Stride is 256
    uint16_t *dst = FB;   // Stride is 240

    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 256; x++) {
            *dst++ = __builtin_bswap16(*src++);
        }
    }

    const int x = 25;
    const int y = 30;
    st7789v_spi_draw_picture_blocking(x, y, x + 256 - 1, y + 210 - 1, FB);
}

void systemOnWriteDataToSoundBuffer(int16_t *finalWave, int length) {}

void systemMessage(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    printf("GBA: %s", buf);
}

extern "C" {
static void emuRunFrame()
{
    frameDrawn = 0;
    while (!frameDrawn) {
        CPULoop();
    }
}

static void emuMain(void *arg)
{
    printf("Emuinit \r\n");
    CPUSetupBuffers();
    CPUInit(NULL, false);
    CPUReset();

    while (1) {
        // if (frameCount % 100 == 0) {
        //     printf("Frame %d \r\n", frameCount);
        // }
        // static uint64_t last_ms = 0;
        // extern uint64_t CPU_Get_MTimer_MS(void);
        // printf("frame:%d fps\r\n", 1000 / (CPU_Get_MTimer_MS() - last_ms));
        // last_ms = CPU_Get_MTimer_MS();

        extern uint32_t bl808_key_read();
        joy = bl808_key_read();
        UpdateJoypad();
        emuRunFrame();
    }
}

/* aos */
#include <cli.h>

/* utils */
#include <utils_getopt.h>
#include <utils_log.h>
/* vfs */
#include <vfs.h>

static void print_usage()
{
    printf("Usage: gba < -l<gba_path> | -e >\r\n");
    printf("\t-l<gba_path> to load gba game\r\n");
    printf("\t-e to exit current gba game\r\n");
    printf("\r\n");
}

static void cmd_c906_gba(char *buf, int len, int argc, char **argv)
{
    static TaskHandle_t task_handle = NULL;
    bool to_be_exit = false;
    char *gba_path = NULL;

    int opt;
    getopt_env_t getopt_env;
    utils_getopt_init(&getopt_env, 0);
    // put ':' in the starting of the string so that program can distinguish
    // between '?' and ':'
    while ((opt = utils_getopt(&getopt_env, argc, argv, ":hel:")) != -1) {
        switch (opt) {
            case 'h':
                print_usage();
                break;
            case 'e':
                to_be_exit = true;
                break;
            case 'l':
                // printf("option: %c\r\n", opt);
                // printf("optarg: %s\r\n", getopt_env.optarg);
                gba_path = getopt_env.optarg;
                break;
            case ':':
                // printf("%s: %c requires an argument\r\n", *argv, getopt_env.optopt);
                break;
            case '?':
                // printf("unknow option: %c\r\n", getopt_env.optopt);
                break;
        }
    }
    // optind is for the extra arguments which are not parsed
    for (; getopt_env.optind < argc; getopt_env.optind++) {
        printf("extra arguments: %s\r\n", argv[getopt_env.optind]);
    }

    if (to_be_exit) {
        if (task_handle) vTaskDelete(task_handle);
        task_handle = NULL;
        memset(rom, 0, ROM_LEN);
    } else if (gba_path) {
        int fd = -1;
        if (0 > (fd = aos_open(gba_path, 0))) {
            printf("[failed] open file %s\r\n", gba_path);
            return;
        }

        int gba_bin_len = 0;

        gba_bin_len = aos_lseek(fd, 0, SEEK_END);
        if (0 >= gba_bin_len) {
            aos_close(fd);
            printf("[failed] the length of gba file is less then zero\r\n");
            return;
        }

        aos_lseek(fd, 0, SEEK_SET);
        if (gba_bin_len != aos_read(fd, rom, gba_bin_len)) {
            aos_close(fd);
            printf("[failed] err occur while reading model\r\n");
            return;
        }
        aos_close(fd);

        puts("[OS] Starting emu task...\r\n");
        xTaskCreate(emuMain, (char *)"emuMainLoop", 8192, NULL, 14, NULL);
    }
}

const static struct cli_command cmds_user[] STATIC_CLI_CMD_ATTRIBUTE = {
    {"gba", "c906 gba command", cmd_c906_gba},
};

int cmd_c906_gba_cli_init(void)
{
    // static command(s) do NOT need to call aos_cli_register_command(s) to register.
    // However, calling aos_cli_register_command(s) here is OK but is of no effect as cmds_user are included in cmds
    // list.
    // XXX NOTE: Calling this *empty* function is necessary to make cmds_user in this file to be kept in the final link.
    // return aos_cli_register_commands(cmds_user, sizeof(cmds_user)/sizeof(cmds_user[0]));
    return 0;
}
}
