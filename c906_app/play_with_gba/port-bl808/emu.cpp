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

uint16_t FB[240 * 256];
int frameDrawn = 0;
uint32_t frameCount = 0;

void systemDrawScreen(void)
{
    frameDrawn = 1;
    frameCount++;
    // return;
    if (frameCount % 2) {
        return;
    }

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
    // lcdSetWindow(0, 40, 240 - 1, 200 - 1);
    // lcdWriteFB((uint8_t*)FB, 240 * 160 * 2);
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

void emuHandleAssertFailed(const char *file, int line, const char *msg)
{
    printf("ASSERT FAILED: %s:%d: %s\n", file, line, msg);
    while (1) {
    }
}

#define EMU_ASSERT(x)                                  \
    if (!(x)) {                                        \
        emuHandleAssertFailed(__FILE__, __LINE__, #x); \
    }

int emuInit()
{
    CPUSetupBuffers();
    CPUInit(NULL, false);
    CPUReset();
    return 0;
}

void emuRunFrame()
{
    frameDrawn = 0;
    while (!frameDrawn) {
        CPULoop();
    }
}

extern uint64_t CPU_Get_MTimer_MS(void);
void emuMainLoop()
{
    memset(rom, 0, ROM_LEN);
    memcpy(rom, (void *)(0x58800000UL - 0x11000), 5 * 1024 * 1024);
    printf("Emuinit \r\n");
    emuInit();
    while (1) {
        if (frameCount % 100 == 0) {
            printf("Frame %d \r\n", frameCount);
        }
        static uint64_t last_ms = 0;
        printf("frame:%d fps\r\n", 1000 / (CPU_Get_MTimer_MS() - last_ms));
        last_ms = CPU_Get_MTimer_MS();
        extern uint32_t bl808_key_read();
        joy = bl808_key_read();
        UpdateJoypad();
        emuRunFrame();
    }
}
}
