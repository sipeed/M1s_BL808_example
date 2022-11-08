
#include <stdio.h>
#include <stdlib.h>

/* aos */
#include <cli.h>

/* utils */
#include <utils_getopt.h>
#include <utils_log.h>

/* bl808 c906 std driver */
#include <bl808_glb.h>

/* bl808 npu */
#include "blai_core.h"

/* fs */
#include <fs/vfs_romfs.h>
#include <vfs.h>

static void print_usage()
{
    printf("Usage: mdl -l<model_path> -o<output_size> <-i<input_bin_path>> <-u>\r\n");
    printf("\t-o must be specified if you want to see your result\r\n");
    printf("\twithout -i it will use random ram data as input\r\n");
    printf("\twith -u it will print output with hex format, otherwise float format\r\n");
    printf("\r\n");
}

static int get_file_from_romfs(char *name, char **xip_addr)
{
    int fd = -1;
    romfs_filebuf_t filebuf;

    if (0 > (fd = aos_open(name, 0))) {
        printf("%s not found!\r\n", name);
        return -1;
    }

    aos_ioctl(fd, IOCTL_ROMFS_GET_FILEBUF, (long unsigned int)&filebuf);
    *xip_addr = filebuf.buf;
    printf("Found file %s. XIP Addr %p, len %#x\r\n", name, filebuf.buf, filebuf.bufsize);
    return filebuf.bufsize;
}

void cmd_c906_mdl(char *buf, int len, int argc, char **argv)
{
    char *model_path = NULL;
    char *input_bin_path = NULL;
    uint32_t output_size = 0;
    bool printhex = false;

    int opt;
    getopt_env_t getopt_env;
    utils_getopt_init(&getopt_env, 0);
    // put ':' in the starting of the string so that program can distinguish
    // between '?' and ':'
    while ((opt = utils_getopt(&getopt_env, argc, argv, ":hul:i:o:")) != -1) {
        switch (opt) {
            case 'h':
                print_usage();
                break;
            case 'u':
                printhex = true;
                break;
            case 'l':
            case 'i':
            case 'o':
                // printf("option: %c\r\n", opt);
                // printf("optarg: %s\r\n", getopt_env.optarg);
                if ('l' == opt) {
                    /* load model path */
                    model_path = getopt_env.optarg;
                } else if ('i' == opt) {
                    /* input bin path */
                    input_bin_path = getopt_env.optarg;
                } else if ('o' == opt) {
                    /* ouput data size */
                    output_size = (uint32_t)atoi(getopt_env.optarg);
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

    if (NULL == model_path) {
        printf("model path unset\r\n");
        return;
    }
    blai_model_hdl_t model_hdl = blai_create();
    if (NULL == model_hdl) {
        printf("alloc model failed\r\n");
        return;
    }
    printf("alloc model success\r\n");

    const char *model_xip_addr;
    get_file_from_romfs(model_path, &model_xip_addr);

    blai_load_model_from_buffer(model_hdl, (uint8_t *)model_xip_addr);
    printf("load model success\r\n");

    BLAI_Model_t *model = (BLAI_Model_t *)model_hdl;
    struct blai_net_info_t *net = model->net;
    printf("model input %ux%ux%u\r\n", net->w, net->h, net->c);

    uint8_t *img_buf = blai_getInputBuffer(model_hdl);
    if (NULL == img_buf) {
        printf("img_buf unavailable\r\n");
        goto exit;
    }
    uint32_t img_size = net->w * net->h * net->c;

    const char *input_bin_xip_addr;
    if (input_bin_path) {
        uint32_t input_bin_size = get_file_from_romfs(input_bin_path, &input_bin_xip_addr);
        if (img_size > input_bin_size) img_size = input_bin_size;
        memcpy(img_buf, input_bin_xip_addr, img_size);
    }
    csi_dcache_clean_range((uint64_t *)img_buf, img_size);

    blai_npu_initCfg(model_hdl);
    blai_startCompute(model_hdl);

    int offset = net->layers[net->layer_cnt - 1].out_layer_mem * net->patch_size;
    uint8_t *output = (uint8_t *)model->buffer + offset;
    float output_scale = net->layers[net->layer_cnt - 1].output_scale;
    int output_zero_point = (int)net->layers[net->layer_cnt - 1].tf_output_offset;

    csi_dcache_clean_range((uint64_t *)output, output_size);
    printf("\r\n[%u]output %d, %f:\r\n", output_size, output_zero_point, output_scale);
    for (uint32_t i = 0; i < output_size; i++) {
        if (!printhex) {
            float model_output = (float)((uint8_t)output[i] - output_zero_point) * output_scale;
            switch (i & 0xf) {
                case 0x0:
                    printf("[%u]\t%f", i / 16, model_output);
                    break;
                case 0xf:
                    printf(" %f\r\n", model_output);
                    break;
                default:
                    printf(" %f", model_output);
                    break;
            }
        } else {
            switch (i & 0xf) {
                case 0x0:
                    printf("[%u]\t%02x", i / 16, output[i]);
                    break;
                case 0xf:
                    printf(" %02x\r\n", output[i]);
                    break;
                default:
                    printf(" %02x", output[i]);
                    break;
            }
        }
    }
    printf("\r\n");

exit:
    blai_free(model_hdl);
}