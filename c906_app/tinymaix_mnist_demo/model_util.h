#pragma once

#include <stdbool.h>
#include <stdio.h>

/* sipeed utils */
#include <fstool/romfs_util.h>

/* sipeed tinymaix */
#include "tinymaix.h"

#if TM_MDL_TYPE == TM_MDL_INT8
#include "mnist_valid_q.h"
#elif TM_MDL_TYPE == TM_MDL_FP32
#include "mnist_resnet_f.h"
#endif

typedef struct {
    uint8_t *output;
    float output_scale;
    int output_zero_point;
    uint32_t output_size;
} model_out_t;

typedef void (*model_out_cb_t)(model_out_t *out, void *arg);

static model_out_t out = {
    .output = NULL,
    .output_scale = 0,
    .output_zero_point = 0,
    .output_size = 0,
};

static tm_err_t layer_cb(tm_mdl_t *mdl, tml_head_t *lh)
{
    out.output_scale = lh->out_s;
    out.output_zero_point = lh->out_zp;

    // dump middle result
    int h = lh->out_dims[1];
    int w = lh->out_dims[2];
    int ch = lh->out_dims[3];
    mtype_t *output = TML_GET_OUTPUT(mdl, lh);

#if 0
    TM_PRINTF("Layer %d callback ========\n", mdl->layer_i);
    for (int y = 0; y < h; y++) {
        TM_PRINTF("[");
        for (int x = 0; x < w; x++) {
            TM_PRINTF("[");
            for (int c = 0; c < ch; c++) {
#if TM_MDL_TYPE == TM_MDL_FP32
                TM_PRINTF("%.3f,", output[(y * w + x) * ch + c]);
#else
                TM_PRINTF("%.3f,", TML_DEQUANT(lh, output[(y * w + x) * ch + c]));
#endif
            }
            TM_PRINTF("],");
        }
        TM_PRINTF("],\r\n");
    }
    TM_PRINTF("\r\n");
#endif
    return TM_OK;
}

static void *load_model(const char *model_path) { return NULL; }

static void model_forward(void *const model, void *input, uint32_t output_size, model_out_cb_t cb, void *cb_arg)
{
    TM_DBGT_INIT();
    TM_PRINTF("mnist demo\n");

    tm_mdl_t _model;
    tm_mdl_t *mdl = &_model;

    tm_mat_t in_uint8 = {3, 28, 28, 1, {(mtype_t *)input}};

    tm_mat_t in = {3, 28, 28, 1, {NULL}};
    tm_mat_t outs[1];
    tm_err_t res;

    tm_stat((tm_mdlbin_t *)mdl_data);

    res = tm_load(mdl, mdl_data, NULL, layer_cb, &in);
    if (res != TM_OK) {
        TM_PRINTF("tm model load err %d\n", res);
        return NULL;
    }

#if (TM_MDL_TYPE == TM_MDL_INT8) || (TM_MDL_TYPE == TM_MDL_INT16)
    res = tm_preprocess(mdl, TMPP_UINT2INT, &in_uint8, &in);
#else
    res = tm_preprocess(mdl, TMPP_UINT2FP01, &in_uint8, &in);
#endif
    TM_DBGT_START();
    res = tm_run(mdl, &in, outs);
    TM_DBGT("tm_run");
    if (res != TM_OK) {
        TM_PRINTF("tm run error: %d\n", res);
        return;
    }

    printf("dims:%u, h:%u, w:%u, c:%u\r\n", outs[0].dims, outs[0].h, outs[0].w, outs[0].c);
    out.output_size = outs->dims * outs->h * outs->w * outs->c;
    out.output = (uint8_t *)outs->data;
    csi_dcache_clean_range((uint64_t *)&out, sizeof(out));
    cb(&out, cb_arg);
}
