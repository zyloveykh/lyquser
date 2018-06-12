#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := H8SM

MODULES += app/mt_boot \
           dev/lcm

MTK_EMMC_SUPPORT = yes
DEFINES += MTK_NEW_COMBO_EMMC_SUPPORT
MTK_KERNEL_POWER_OFF_CHARGING = yes
#DEFINES += SWCHR_POWER_PATH
#DEFINES += MTK_BQ24261_SUPPORT
MTK_LCM_PHYSICAL_ROTATION = 270
CUSTOM_LK_LCM="kd070d5450nha6_rgb_dpi"
#nt35595_fhd_dsi_cmd_truly_nt50358 = yes

#FASTBOOT_USE_G_ORIGINAL_PROTOCOL = yes
MTK_SECURITY_SW_SUPPORT = yes
MTK_VERIFIED_BOOT_SUPPORT = yes
MTK_SEC_FASTBOOT_UNLOCK_SUPPORT = yes

BOOT_LOGO=xga

#DEFINES += WITH_DEBUG_DCC=1
DEFINES += WITH_DEBUG_UART=1
#DEFINES += WITH_DEBUG_FBCON=1

MTK_IN_HOUSE_TEE_SUPPORT=yes
MTK_SEC_VIDEO_PATH_SUPPORT=yes
MTK_WVDRM_L1_SUPPORT = no
