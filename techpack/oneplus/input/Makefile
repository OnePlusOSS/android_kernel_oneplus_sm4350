# oem device deriver
#
ifeq ($(CONFIG_ARCH_LAHAINA), y)
obj-$(CONFIG_TOUCHSCREEN) += OneplusTouchScreen/
endif

obj-$(CONFIG_INPUT_FINGERPRINT) += fingerprint/

ifeq ($(CONFIG_ARCH_HOLI), y)
obj-$(CONFIG_TOUCHPANEL_OPLUS) += oplus_touchscreen/
obj-$(CONFIG_TOUCHPANEL_OPLUS) += touch.o
endif
