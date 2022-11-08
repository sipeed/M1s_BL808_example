
#include <stdio.h>
#include <stdlib.h>

/* aos */
#include <cli.h>

/* utils */
#include <utils_getopt.h>
#include <utils_log.h>

/* bl808 c906 std driver */
#include <bl808_glb.h>

void cmd_c906_gpio(char *buf, int len, int argc, char **argv) {
  int gpio_pin = -1;
  int gpio_state = -1;

  int opt;
  getopt_env_t getopt_env;
  utils_getopt_init(&getopt_env, 0);
  // put ':' in the starting of the string so that program can distinguish
  // between '?' and ':'
  while ((opt = utils_getopt(&getopt_env, argc, argv, ":p:s:")) != -1) {
    switch (opt) {
      case 'i':
        // printf("option: %c\r\n", opt);
        break;
      case 'p':
      case 's':
        // printf("optarg: %s\r\n", getopt_env.optarg);
        if ('p' == opt) {
          gpio_pin = atoi(getopt_env.optarg);
        } else if ('s' == opt) {
          gpio_state = atoi(getopt_env.optarg);
        }
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

  if (gpio_pin == -1) return;
  GLB_GPIO_Cfg_Type cfg;
  cfg.drive = 0;
  cfg.smtCtrl = 1;
  cfg.gpioFun = GPIO_FUN_GPIO;
  cfg.outputMode = 0;
  cfg.pullType = GPIO_PULL_NONE;
  cfg.gpioPin = gpio_pin;

  cfg.gpioMode = gpio_state == -1 ? GPIO_MODE_INPUT : GPIO_MODE_OUTPUT;
  GLB_GPIO_Init(&cfg);
  if (gpio_state == -1) {
    printf("gpio_%d=%d\r\n", gpio_pin, GLB_GPIO_Read(gpio_pin));
  } else {
    GLB_GPIO_Write(gpio_pin, !!gpio_state);
  }
}