ifeq ($(CONFIG_OPLUS_CHARGER_MTK),y)
subdir-ccflags-y += -Werror
subdir-ccflags-y += -Wno-unused-variable
ifeq ($(CONFIG_OPLUS_CHARGER_MTK6779Q),y)
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
endif
endif

#oplus_chg-y	+= wireless_ic/oplus_p922x.o
#oplus_chg-y	+= wireless_ic/oplus_nu1619.o
#oplus_chg-y	+= wireless_ic/oplus_chargepump.o

#oplus_chg-y	+= gauge_ic/oplus_bq27541.o

#oplus_chg-y	+= warp_ic/oplus_n76e.o
oplus_chg-y	+= warp_ic/oplus_warp_fw.o
#oplus_chg-y	+= warp_ic/oplus_stm8s.o
#oplus_chg-y	+= warp_ic/oplus_rk826.o
#oplus_chg-y	+= warp_ic/oplus_op10.o

#obj-y	+= adapter_ic/oplus_stm.o

oplus_chg-y	+= charger_ic/oplus_short_ic.o

#oplus_chg-$(CONFIG_OPLUS_HOLI_CHARGER) += ../supply/qcom/qpnp-smb5.o ../supply/qcom/smb5-lib.o

oplus_chg-y	+= oplus_charger.o
oplus_chg-y	+= oplus_gauge.o
oplus_chg-y	+= oplus_warp.o
oplus_chg-y	+= oplus_adapter.o
oplus_chg-y	+= oplus_wireless.o
oplus_chg-y	+= oplus_short.o
oplus_chg-y	+= oplus_configfs.o

obj-$(CONFIG_OPLUS_CHG) := oplus_chg.o
