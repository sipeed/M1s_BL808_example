#ifndef GLOBALS_H
#define GLOBALS_H

#include "types.h"
#include <stdint.h>


//performance boost tweaks.
#if USE_TWEAKS
    #define USE_TWEAK_SPEEDHACK 1
	#define USE_TWEAK_MEMFUNC 1
#endif

#define PIX_BUFFER_SCREEN_WIDTH 256

extern int saveType;
extern bool useBios;
extern bool skipBios;
extern bool cpuIsMultiBoot;
extern int cpuSaveType;
extern bool mirroringEnable;
extern bool enableRtc;
extern bool skipSaveGameBattery; // skip battery data when reading save states

extern int cpuDmaCount;

#define LIBRETRO_SAVE_BUF_LEN (0x22000)
#define OAM_LEN (0x400)
#define IOMEM_LEN (0x400)
#define PALETTERAM_LEN (0x400)
#define INTERNALRAM_LEN (0x8000)
#define VRAM_LEN (0x20000)
#define WORKRAM_LEN (0x40000)
#define BIOS_LEN (0x4000)
#define PIX_LEN (2 * 256 * 160)
#define ROM_LEN (32 * 1024 * 1024)

extern uint8_t oam[];
extern uint8_t ioMem[];
extern uint8_t internalRAM[];
extern uint8_t vram[];
extern uint8_t workRAM[];
extern uint8_t bios[];
extern uint16_t pix[];
extern uint8_t libretro_save_buf[];
extern uint8_t rom[];
extern uint8_t paletteRAM[];

#endif // GLOBALS_H
