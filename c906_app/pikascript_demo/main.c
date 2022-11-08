#include <FreeRTOS.h>
#include <aos/yloop.h>
#include <cli.h>
#include <lfs_c906.h>
#include <stdio.h>
#include <task.h>
#include <vfs.h>
#include "lv_conf.h"
#include "lvgl.h"
#include "pikaScript.h"

#include "lv_port_disp.h"
#include "lv_port_indev.h"

volatile PikaObj* root = NULL;
static volatile char rxbuff[1024 * 10];
static volatile int rxsize = 0;
static volatile int rxbusy = 0;
static void console_cb_read_pika(int fd, void* param) {
    while (rxbusy) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    rxbusy = 1;
    char buff[1024];
    int sz = aos_read(fd, buff, sizeof(buff));
    memcpy((void*)(rxbuff + rxsize), buff, sz);
    rxsize += sz;
    rxbusy = 0;
}

char __platform_getchar(void) {
    char c = 0;
    while (rxbusy || rxsize == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    rxbusy = 1;
    c = rxbuff[0];
    memmove(rxbuff, rxbuff + 1, rxsize - 1);
    rxsize--;
    rxbusy = 0;
    return c;
}

/* file API */
FILE* __platform_fopen(const char* filename, const char* modes) {
    return fopen(filename, modes);
}

int __platform_fclose(FILE* stream) {
    return fclose(stream);
}

size_t __platform_fwrite(const void* ptr, size_t size, size_t n, FILE* stream) {
    return fwrite(ptr, size, n, stream);
}

size_t __platform_fread(void* ptr, size_t size, size_t n, FILE* stream) {
    return fread(ptr, size, n, stream);
}

int __platform_fseek(FILE* stream, long offset, int whence) {
    return fseek(stream, offset, whence);
}

long __platform_ftell(FILE* stream) {
    return ftell(stream);
}

static void lvgl_task(void* param) {
    while (1) {
        lv_task_handler();
        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}

static void pika_REPL_task(void* param) {
    pikaScriptShell(root);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void main(void) {
    lv_init();
    lv_port_disp_init();
    printf("[ Info] In PikaSciprt Demo...\r\n");
    root = pikaScriptInit();
    int fd_console = 3;  // aos_open("/dev/ttyS0", 0);
    aos_cancel_poll_read_fd(fd_console, 0, NULL);
    aos_poll_read_fd(fd_console, console_cb_read_pika, (void*)0x12345678);
    lv_task_handler();
    xTaskCreate(lvgl_task, (char*)"lvdl task", 512, NULL, 15, NULL);
    xTaskCreate(pika_REPL_task, (char*)"pika REPL", 4096, NULL, 15, NULL);
}
