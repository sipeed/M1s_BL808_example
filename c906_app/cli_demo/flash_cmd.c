
#include <stdio.h>
#include <stdlib.h>

/* aos */
#include <cli.h>

/* utils */
#include <utils_getopt.h>
#include <utils_log.h>

/* bl808 c906 std driver */
#include <bl808_glb.h>

#include "m1s_c906_xram_flash.h"

void cmd_c906_flash(char *buf, int len, int argc, char **argv)
{
    uint32_t o = 0, a = 0, l = 0;
    uint8_t op = 0;
    int ret = -1;

    int opt;
    getopt_env_t getopt_env;
    utils_getopt_init(&getopt_env, 0);
    // put ':' in the starting of the string so that program can distinguish
    // between '?' and ':'
    while ((opt = utils_getopt(&getopt_env, argc, argv, ":rweo:a:l:")) != -1) {
        switch (opt) {
            case 'r':
            case 'w':
            case 'e':
                op = opt;
                break;
                // printf("option: %c\r\n", opt);
                break;
            case 'o':
            case 'a':
            case 'l':
                // printf("optarg: %s\r\n", getopt_env.optarg);
                if ('o' == opt) {
                    o = strtol(getopt_env.optarg, NULL, 0);
                } else if ('a' == opt) {
                    a = strtol(getopt_env.optarg, NULL, 0);
                } else if ('l' == opt) {
                    l = strtol(getopt_env.optarg, NULL, 0);
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

    switch (op) {
        case 'r':
            ret = m1s_xram_flash_read(o, a, l);
            break;
        case 'w':
            ret = m1s_xram_flash_write(o, a, l);
            break;
        case 'e':
            ret = m1s_xram_flash_erase(o, l);
            break;
        default:
            printf("no changed, select <r><w><e>\r\n");
            break;
    }
    if (0 != ret) {
        printf("flash operation error\r\n");
    }
}