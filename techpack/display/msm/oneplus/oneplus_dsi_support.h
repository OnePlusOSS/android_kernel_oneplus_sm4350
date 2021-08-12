#ifndef _OPLUS_DSI_SUPPORT_H_
#define _OPLUS_DSI_SUPPORT_H_

enum oplus_display_power_status {
	OPLUS_DISPLAY_POWER_OFF = 0,
	OPLUS_DISPLAY_POWER_DOZE,
	OPLUS_DISPLAY_POWER_ON,
	OPLUS_DISPLAY_POWER_DOZE_SUSPEND,
	OPLUS_DISPLAY_POWER_ON_UNKNOW,
};

void set_oplus_display_power_status(enum oplus_display_power_status power_status);

enum oplus_display_power_status get_oplus_display_power_status(void);

#endif /* _OPLUS_DSI_SUPPORT_H_ */

