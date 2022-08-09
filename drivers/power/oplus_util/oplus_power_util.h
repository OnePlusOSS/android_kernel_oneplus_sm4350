/* SPDX-License-Identifier: GPL-2.0 */
#ifndef OPLUS_POWER_UTIL_H
#define OPLUS_POWER_UTIL_H

#define WAKELOCK_BUFFER_SIZE 4096
#define RPM_BUFFER_SIZE 128
#define VENDOR_EDIT
struct oplus_util {
	struct delayed_work oplus_work;
	int polling_time;
#ifndef CONFIG_OPLUS_WAKELOCK_PROFILER
	char *wakelock_buf;
#endif
	char *rpm_buf;
};

#ifndef ONFIG_OPLUS_WAKELOCK_PROFILER
extern void oplus_pm_get_active_wakeup_sources(char *pending_wakeup_source, size_t max);
#endif
#ifdef CONFIG_QTI_RPM_STATS_LOG
extern size_t oplus_get_rpm_master_stats(char *buf, size_t size);
#endif
void oplus_get_rpm_stats(void);
#endif //end of OPLUS_POWER_UTIL_H
