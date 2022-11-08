#include <stdio.h>

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>

void main()
{
    while (1) {
        printf("hello, world!\r\n");
        vTaskDelay(1000);
    }
}