#include <stdbool.h>
#include <stdio.h>

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>

/* lcd */
#include <lcd.h>

/* bl808 c906 std driver */
#include <bl808_glb.h>

#include "m1s_c906_xram_pwm.h"

void main()
{
    st7789v_spi_init();

#define PWM_PORT (0)
#define PWM_PIN (11)
    m1s_xram_pwm_init(PWM_PORT, PWM_PIN, 2000, 25);
    m1s_xram_pwm_start(PWM_PORT, PWM_PIN);

    while (1)
    {
        /* 测试：循环切换多个颜色 */
        #define WHITE       0xFFFF
        #define BLACK       0x0000
        #define BLUE        0x001F
        #define RED         0xF800
        #define GREEN       0x07E0
        #define YELLOW      0xFFE0
        const uint16_t rgb565_list[] = 
        {WHITE, BLACK, BLUE, RED, GREEN, YELLOW};

        for (int i = 0; i < sizeof(rgb565_list) / sizeof(rgb565_list[0]); i ++)
        {
            uint64_t curr_cnt = CPU_Get_MTimer_US();
            st7789v_spi_clear(rgb565_list[i]);
            printf("lcd:%f fps\r\n", (double)1000000 / (CPU_Get_MTimer_US() - curr_cnt));
            vTaskDelay(1000);
        }
    }
}