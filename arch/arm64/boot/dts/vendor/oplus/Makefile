ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)
dtbo-$(CONFIG_ARCH_HOLI) += \
	dre9-2083c-mtp-overlay.dtbo

dre9-2083c-mtp-overlay.dtbo-base := holi.dtb

endif


always		:= $(dtb-y)
subdir-y	:= $(dts-dirs)
clean-files	:= *.dtb *.dtbo