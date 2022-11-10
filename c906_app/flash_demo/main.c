#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>

/* aos */
#include <cli.h>

#include "m1s_c906_xram_flash.h"
// partion media
#define MEDIA_OFFSET_IN_FLASH (0x300000)
#define MEDIA_OFFSET_IN_XIP (0x582f0000)

extern void cmd_c906_flash(char *buf, int len, int argc, char **argv);
const static struct cli_command cmds_user[] STATIC_CLI_CMD_ATTRIBUTE = {
    {"flash", "c906 flash command", cmd_c906_flash},
};

static inline void log_hex(uint32_t *a, uint32_t l)
{
    l /= 4;
    for (uint32_t i = 0; i < l; i++) {
        if ((i & 0x3) == 0) printf("%08x:", (uint32_t)&a[i]);
        printf(" %08x", a[i]);
        if ((i & 0x3) == 0x3) printf("\r\n");
    }
    if (l & 0x3) printf("\r\n");
}

#define LOGDATA()              \
    do {                       \
        printf("ram:\r\n");    \
        log_hex(addr, length); \
        printf("\r\n");        \
    } while (0)

#define MODRAM()                                                    \
    do {                                                            \
        for (uint32_t i = 0; i < length / 4; i++) addr[i] = rand(); \
    } while (0)

#define CLRRAM()                                               \
    do {                                                       \
        for (uint32_t i = 0; i < length / 4; i++) addr[i] = 0; \
    } while (0)

void main()
{
    vTaskDelay(500);
    srand(CPU_Get_MTimer_Counter());
    uint32_t offset = MEDIA_OFFSET_IN_FLASH;
    uint32_t addr[32];
    uint32_t length = sizeof(addr);
    printf("xip:\r\n");
    log_hex(MEDIA_OFFSET_IN_XIP, length);

    printf("[flash] read\r\n");
    CLRRAM();
    m1s_xram_flash_read(offset, addr, length);
    LOGDATA();

    printf("[flash] erase\r\n");
    m1s_xram_flash_erase(offset, length);

    printf("[flash] read\r\n");
    CLRRAM();
    m1s_xram_flash_read(offset, addr, length);
    LOGDATA();

    printf("[ram] modified\r\n");
    MODRAM();
    LOGDATA();

    printf("[flash] write\r\n");
    m1s_xram_flash_write(offset, addr, length);

    printf("[flash] read\r\n");
    CLRRAM();
    m1s_xram_flash_read(offset, addr, length);
    LOGDATA();
}