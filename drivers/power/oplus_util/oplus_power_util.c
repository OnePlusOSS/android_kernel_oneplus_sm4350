// SPDX-License-Identifier: GPL-2.0
/*
 * kernel/power/oplus_util/oplus_power_util.c
 *
 * oplus power debug feature
 *
 * */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/suspend.h>
#include "oplus_power_util.h"
#include "../../../drivers/input/oplus_secure_drivers/include/oplus_secure_common.h"

struct oplus_util oplus_util;
static int polling_time = 60000;
module_param_named(polling_time, polling_time, int, S_IRUGO|S_IWUSR);

#ifndef CONFIG_OPLUS_WAKELOCK_PROFILER
ssize_t get_active_wakelocks(char *buf)
{
	oplus_pm_get_active_wakeup_sources(buf, WAKELOCK_BUFFER_SIZE);
	return 0;
}
#endif

void oplus_get_rpm_stats(void)
{
	extern secure_type_t get_secureType(void);

	pr_info("get_secureType(): %d", get_secureType());
	if (get_secureType() == 3)
		return;
#ifdef CONFIG_QTI_RPM_STATS_LOG
	oplus_get_rpm_master_stats(oplus_util.rpm_buf, RPM_BUFFER_SIZE);
	pr_info("[OPLUS_POWER]: %s", oplus_util.rpm_buf);
#endif
}

void oplus_power_polling_fn(struct work_struct *work)
{
#ifndef CONFIG_OPLUS_WAKELOCK_PROFILER
	get_active_wakelocks(oplus_util.wakelock_buf);
	pr_info("[OPLUS_POWER]: %s", oplus_util.wakelock_buf);
#endif
	oplus_get_rpm_stats();
	oplus_util.polling_time = polling_time;
	schedule_delayed_work(&(oplus_util.oplus_work),
					msecs_to_jiffies(oplus_util.polling_time));
}

static int __init oplus_util_init(void)
{
	oplus_util.polling_time = polling_time;
#ifndef CONFIG_OPLUS_WAKELOCK_PROFILER
	oplus_util.wakelock_buf = (char *)kzalloc(WAKELOCK_BUFFER_SIZE, GFP_KERNEL);
#endif
	oplus_util.rpm_buf = (char *)kzalloc(RPM_BUFFER_SIZE, GFP_KERNEL);
	INIT_DELAYED_WORK(&(oplus_util.oplus_work),
				oplus_power_polling_fn);
	schedule_delayed_work(&(oplus_util.oplus_work),
					msecs_to_jiffies(oplus_util.polling_time));
	return 0;
}

static void __exit oplus_util_exit(void)
{
#ifndef CONFIG_OPLUS_WAKELOCK_PROFILER
	kfree(oplus_util.wakelock_buf);
#endif
	kfree(oplus_util.rpm_buf);
}
module_init(oplus_util_init);
module_exit(oplus_util_exit);
