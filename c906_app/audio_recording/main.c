#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>

#include "m1s_c906_xram_audio.h"

#define AUDIO_BUFF_SIZE (4096)

void main()
{
    printf("Audio init..\r\n");
    uint16_t *buff = pvPortMalloc(AUDIO_BUFF_SIZE * 2);
    assert(buff);
    m1s_xram_audio_init(buff, AUDIO_BUFF_SIZE);

    uint16_t *rec_buff;
    uint32_t rec_buff_size;
    while (1) {
        if (0 == m1s_xram_audio_get(&rec_buff, &rec_buff_size) && rec_buff_size > 0) {
            for (int i = 0; i < rec_buff_size >> 1; i++) {
                if (i % 256 == 0) printf("\r\n");
                printf("%d, ", rec_buff[i]);
            }
            m1s_xram_audio_pop();
        }
        vTaskDelay(1);
    }
    m1s_xram_audio_deinit();
}
