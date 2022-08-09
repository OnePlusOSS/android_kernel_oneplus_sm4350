# Copyright (C), 2008-2030, OPLUS Mobile Comm Corp., Ltd
### All rights reserved.
###
### File: - OplusKernelEnvConfig.mk
### Description:
###     you can get the oplus feature variables set in android side in this file
###     this file will add global macro for common oplus added feature
###     BSP team can do customzation by referring the feature variables
### Version: 1.0
### Date: 2020-03-18
### Author: Liang.Sun
###
### ------------------------------- Revision History: ----------------------------
### <author>                        <date>       <version>   <desc>
### ------------------------------------------------------------------------------
##################################################################################

-include oplus_native_features.mk
###ifdef OPLUS_ARCH_INJECT
OPLUS_CONNECTIVITY_NATIVE_FEATURE_SET :=


ifeq ($(OPLUS_FEATURE_WIFI_MTUDETECT), yes)
OPLUS_CONNECTIVITY_NATIVE_FEATURE_SET += OPLUS_FEATURE_WIFI_MTUDETECT
endif


$(foreach myfeature,$(OPLUS_CONNECTIVITY_NATIVE_FEATURE_SET),\
    $( \
        $(eval KBUILD_CFLAGS += -D$(myfeature)) \
        $(eval KBUILD_CPPFLAGS += -D$(myfeature)) \
        $(eval CFLAGS_KERNEL += -D$(myfeature)) \
        $(eval CFLAGS_MODULE += -D$(myfeature)) \
    ) \
)
###endif OPLUS_ARCH_INJECT

#bsp team should check and modify neccessary to make sure the following macro is allowed to declare
#can add or delete item for the top level macro
ALLOWED_MCROS := OPLUS_FEATURE_POWERINFO_FTM \
OPLUS_FEATURE_AGINGTEST \
OPLUS_FEATURE_WIFI_BDF  \
OPLUS_FEATURE_WIFI_MAC  \
OPLUS_FEATURE_WIFI_SLA  \
OPLUS_FEATURE_DHCP \
OPLUS_FEATURE_SELINUX_CONTROL_LOG  \
OPLUS_FEATURE_MODEM_MINIDUMP \
OPLUS_FEATURE_THEIA \
OPLUS_FEATURE_APP_MONITOR \
OPLUS_FEATURE_DATA_EVAL \
OPLUS_FEATURE_QCOM_WATCHDOG \
OPLUS_FEATURE_TASK_CPUSTATS \
OPLUS_FEATURE_IMPEDANCE_MATCH \
OPLUS_FEATURE_RT_INFO

KBUILD_CFLAGS += -DOPLUS_FEATURE_HEALTHINFO
KBUILD_CFLAGS += -DOPLUS_FEATURE_POWERINFO_FTM
KBUILD_CFLAGS += -DOPLUS_FEATURE_POWERINFO_STANDBY
KBUILD_CFLAGS += -DOPLUS_FEATURE_POWERINFO_STANDBY_DEBUG
KBUILD_CFLAGS += -DOPLUS_FEATURE_POWERINFO_RPMH

KBUILD_CFLAGS += -DOPLUS_FEATURE_SCHED_ASSIST
KBUILD_CFLAGS += -DOPLUS_FEATURE_WIFI_LIMMITBGSPEED
KBUILD_CFLAGS += -DOPLUS_FEATURE_WIFI_BDF
KBUILD_CFLAGS += -DOPLUS_FEATURE_WIFI_SLA
KBUILD_CFLAGS += -DOPLUS_FEATURE_DHCP
KBUILD_CFLAGS += -DOPLUS_FEATURE_PERFORMANCE
KBUILD_CFLAGS += -DOPLUS_FEATURE_HANS_FREEZE

KBUILD_CFLAGS += -DOPLUS_FEATURE_KTV
KBUILD_CFLAGS += -DOPLUS_ARCH_EXTENDS
KBUILD_CFLAGS += -DOPLUS_FEATURE_VIRTUAL_RESERVE_MEMORY
KBUILD_CFLAGS += -DOPLUS_FEATURE_MEMLEAK_DETECT
KBUILD_CFLAGS += -DOPLUS_AUDIO_PA_BOOST_VOLTAGE
KBUILD_CFLAGS += -DOPLUS_FEATURE_SPEAKER_MUTE

KBUILD_CFLAGS += -DOPLUS_FEATURE_TP_BASIC


#only declare a macro if nativefeature is define and also added in above ALLOWED_MCROS
$(foreach myfeature,$(ALLOWED_MCROS),\
    $(if $(strip $($(myfeature))),\
         $(warning make $(myfeature) to be a macro here) \
         $(eval KBUILD_CFLAGS += -D$(myfeature)) \
         $(eval KBUILD_CPPFLAGS += -D$(myfeature)) \
         $(eval CFLAGS_KERNEL += -D$(myfeature)) \
         $(eval CFLAGS_MODULE += -D$(myfeature)) \
))

# BSP team can do customzation by referring the feature variables

ifeq ($(OPLUS_FEATURE_SECURE_GUARD),yes)
export CONFIG_OPLUS_SECURE_GUARD=y
KBUILD_CFLAGS += -DCONFIG_OPLUS_SECURE_GUARD
endif

ifeq ($(OPLUS_FEATURE_SECURE_ROOTGUARD),yes)
export CONFIG_OPLUS_ROOT_CHECK=y
KBUILD_CFLAGS += -DCONFIG_OPLUS_ROOT_CHECK
endif

ifeq ($(OPLUS_FEATURE_SECURE_MOUNTGUARD),yes)
export CONFIG_OPLUS_MOUNT_BLOCK=y
KBUILD_CFLAGS += -DCONFIG_OPLUS_MOUNT_BLOCK
endif                                                                                               

ifeq ($(OPLUS_FEATURE_SECURE_EXECGUARD),yes)
export CONFIG_OPLUS_EXECVE_BLOCK=y
KBUILD_CFLAGS += -DCONFIG_OPLUS_EXECVE_BLOCK
KBUILD_CFLAGS += -DCONFIG_OPLUS_EXECVE_REPORT
endif

ifeq ($(OPLUS_FEATURE_SECURE_KEVENTUPLOAD),yes)
export CONFIG_OPLUS_KEVENT_UPLOAD=y
KBUILD_CFLAGS += -DCONFIG_OPLUS_KEVENT_UPLOAD
endif

ifeq ($(OPLUS_FEATURE_SECURE_KEYINTERFACESGUARD),yes)
KBUILD_CFLAGS += -DOPLUS_DISALLOW_KEY_INTERFACES
endif

KBUILD_CFLAGS += -DOPLUS_FEATURE_LOWMEM_DBG

