
PROJECT_NAME ?= main
PROJECT_PATH := $(abspath .)
PROJECT_BOARD := evb
export PROJECT_PATH PROJECT_BOARD

-include ./proj_config.mk

ifeq ($(origin BL_SDK_PATH), undefined)
$(error   BL_SDK_PATH not found, please enter: export BL_SDK_PATH={sdk_path})
endif

COMPONENTS_BLSYS   := blog
COMPONENTS_VFS     := 
COMPONENTS_DEMO_CLI := bl808_wlan_cli_demo
COMPONENTS_AMP := bl808_xram bl808_ring bl808_net remote_net_manager bl808_user_xram bl808_peri_e907

# wifi module
COMPONENTS_NETWORK := lwip lwip_dhcpd dns_server netutils mbedtls_lts
COMPONENTS_NETWORK += wifi wifi_manager blcrypto_suite wpa_supplicant wifi_hosal wifi_bt_coex
COMPONENTS_NETWORK += bl606p_phyrf
COMPONENTS_NETWORK += bl_os_adapter

INCLUDE_COMPONENTS += freertos_e907 hal_drv vfs yloop cli utils newlibc blmtd easyflash4 freertos_e907_helper coredump
INCLUDE_COMPONENTS += startup_bl808
INCLUDE_COMPONENTS += bl808_e907_std
INCLUDE_COMPONENTS += hosal blfdt bl808_audio
INCLUDE_COMPONENTS += romfs usb_stack
INCLUDE_COMPONENTS += fatfs sdh_helper

COMPONENTS_SIPEEED := 
COMPONENTS_SIPEEED += m1s_boot_d0
COMPONENTS_SIPEEED += lfs m1s_lfs_e907
COMPONENTS_SIPEEED += m1s_e907_xram m1s_common_xram m1s_msc
INCLUDE_COMPONENTS += $(COMPONENTS_SIPEEED)

ifeq ($(CONFIG_COMPONENT_BUGKILLER_ENABLE),1)
INCLUDE_COMPONENTS += bugkiller_freertos
endif

INCLUDE_COMPONENTS += $(COMPONENTS_BLSYS)
INCLUDE_COMPONENTS += $(COMPONENTS_VFS)
INCLUDE_COMPONENTS += $(COMPONENTS_DEMO_CLI)
INCLUDE_COMPONENTS += $(COMPONENTS_AMP)
INCLUDE_COMPONENTS += $(COMPONENTS_NETWORK)
INCLUDE_COMPONENTS += $(PROJECT_NAME)

include $(BL_SDK_PATH)/make_scripts_riscv/project_common.mk
