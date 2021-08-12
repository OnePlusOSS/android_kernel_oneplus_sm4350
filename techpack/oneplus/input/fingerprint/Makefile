obj-$(CONFIG_FINGERPRINT_DETECT)	+= fingerprint_detect/
ifeq ($(CONFIG_ARCH_HOLI), y)
#fingerprint_detect should before fpc1022
obj-$(CONFIG_FINGERPRINT_FPC)	        += fpc/
obj-$(CONFIG_FINGERPRINT_GOODIX)	+= goodix3626/
else
obj-$(CONFIG_FINGERPRINT_GOODIX)	+= goodix/
endif
