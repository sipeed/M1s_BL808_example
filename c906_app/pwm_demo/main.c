#include <stdbool.h>
#include <stdio.h>

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>

#include "m1s_c906_xram_pwm.h"

#define PWM_PORT (0)
#define PWM_PIN (8)

void main()
{
    m1s_xram_pwm_init(PWM_PORT, PWM_PIN, 2000, 50);
    m1s_xram_pwm_start(PWM_PORT, PWM_PIN);

    /* 频率不变，占空比循环递增 */
    uint8_t duty = 50;
    for (int i = 0; i < 10; i++) {
        duty = (duty >= 100) ? 0 : (duty + 10);
        m1s_xram_pwm_set_duty(PWM_PORT, PWM_PIN, 2000, duty);
        vTaskDelay(1000);
    }

    /* 占空比不变，频率循环递增 */
    uint32_t freq = 0;
    for (int i = 0; i < 10; i++) {
        freq = freq >= 1 * 1000 * 1000 ? 0 : freq + 1 * 1000 * 100;
        m1s_xram_pwm_deinit(PWM_PORT);
        m1s_xram_pwm_init(PWM_PORT, PWM_PIN, freq, 50);
        m1s_xram_pwm_start(PWM_PORT, PWM_PIN);
        vTaskDelay(1000);
    }

    /* 暂停/启动pwm */
    for (int i = 0; i < 2; i++) {
        m1s_xram_pwm_stop(PWM_PORT, PWM_PIN);
        vTaskDelay(1000);
        m1s_xram_pwm_start(PWM_PORT, PWM_PIN);
        vTaskDelay(1000);
    }
}