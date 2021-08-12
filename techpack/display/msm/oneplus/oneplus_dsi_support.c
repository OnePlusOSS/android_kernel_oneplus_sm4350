#include "oneplus_dsi_support.h"
#include <linux/of.h>
#include <linux/err.h>

static enum oplus_display_power_status oplus_display_status =
	OPLUS_DISPLAY_POWER_OFF;

void set_oplus_display_power_status(enum oplus_display_power_status power_status)
{
    oplus_display_status = power_status;
}
EXPORT_SYMBOL(set_oplus_display_power_status);

enum oplus_display_power_status get_oplus_display_power_status(void)
{
	return oplus_display_status;
}
EXPORT_SYMBOL(get_oplus_display_power_status);
