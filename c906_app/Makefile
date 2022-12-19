#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME ?= c906_demo
PROJECT_PATH = $(abspath .)
PROJECT_BOARD := evb
export PROJECT_PATH PROJECT_BOARD

-include ./proj_config.mk

ifeq ($(origin BL_SDK_PATH), undefined)
$(error   BL_SDK_PATH not found, please enter: export BL_SDK_PATH={sdk_path})
endif

INCLUDE_COMPONENTS += bl808_c906_freertos bl808 bl808_std newlibc hosal freetype yloop cli utils
INCLUDE_COMPONENTS += blai_nn blai_npu_encoder
INCLUDE_COMPONENTS += bl_mm venc_device venc_framework 
INCLUDE_COMPONENTS += freertos_posix
INCLUDE_COMPONENTS += blog
INCLUDE_COMPONENTS += vfs lvgl lwip
INCLUDE_COMPONENTS += blfdt romfs fatfs sdh_helper 
INCLUDE_COMPONENTS += bl808_ring bl808_xram bl_os_adapter
INCLUDE_COMPONENTS += dsp2 bl_mm venc_device venc_framework dsp2_cli_demo mjpeg_sender_bl808 sensor rtsp_server

COMPONENTS_SIPEEED :=
COMPONENTS_SIPEEED += m1s_start
COMPONENTS_SIPEEED += m1s_model_runner
COMPONENTS_SIPEEED += m1s_tools
COMPONENTS_SIPEEED += lfs m1s_lfs_c906
COMPONENTS_SIPEEED += m1s_common_xram m1s_c906_xram
INCLUDE_COMPONENTS += $(COMPONENTS_SIPEEED)
INCLUDE_COMPONENTS += $(PROJECT_NAME)

CFLAGS += -DROMFS_STATIC_ROOTADDR=0x582f0000

include $(BL_SDK_PATH)/make_scripts_riscv/project_common.mk
