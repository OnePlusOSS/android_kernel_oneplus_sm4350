
#ifndef _oplus_BOOT_H
#define _oplus_BOOT_H
enum{
        MSM_BOOT_MODE__NORMAL,
        MSM_BOOT_MODE__FASTBOOT,
        MSM_BOOT_MODE__RECOVERY,
        MSM_BOOT_MODE__FACTORY,
        MSM_BOOT_MODE__RF,
        MSM_BOOT_MODE__WLAN,
        MSM_BOOT_MODE__MOS,
        MSM_BOOT_MODE__CHARGE,
        MSM_BOOT_MODE__SILENCE,
        MSM_BOOT_MODE__SAU,
        /*xiaofan.yang@PSW.TECH.AgingTest, 2019/01/07,Add for factory agingtest*/
        MSM_BOOT_MODE__AGING = 998,
        MSM_BOOT_MODE__SAFE = 999,
};

#ifdef CONFIG_QGKI
extern int get_boot_mode(void);
#else
__weak get_boot_mode(void)
{
  	return 0;
}
#endif


extern bool qpnp_is_power_off_charging(void);

extern bool qpnp_is_charger_reboot(void);

#endif  /*_oplus_BOOT_H*/


#ifdef PHOENIX_PROJECT
extern bool op_is_monitorable_boot(void);
#endif


