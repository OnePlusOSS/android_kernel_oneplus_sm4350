// SPDX-License-Identifier: GPL-2.0-only
/* BQ2560x battery charging driver
 *
 * Copyright (C) 2013 Texas Instruments
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#define pr_fmt(fmt) "BQ25600 %s: " fmt, __func__

#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/debugfs.h>
#include <linux/errno.h>
#include <linux/iio/consumer.h>
#include <linux/iio/iio.h>
#include <dt-bindings/iio/qti_power_supply_iio.h>
#include <linux/qti_power_supply.h>

#ifndef DEBUG
#define DEBUG
#endif

#include "bq25600_reg.h"

#define MIN_PARALLEL_ICL_UA	250000
#define SUSPEND_CURRENT_MA	2

enum reason {
	USER	= BIT(0),
	THERMAL = BIT(1),
	CURRENT = BIT(2),
	SOC	= BIT(3),
};


enum bq2560x_part_no {
	BQ25600 = 0x00,
	BQ25600D = 0x01,
};

enum {
	MASTER_DEBUG_FCC = 0,
	SLAVE_DEBUG_FCC,
};

struct bq2560x_config {
	int		charge_voltage;
	int		charge_current;

	int		iindpm_threshold;
	int		vindpm_threshold;

	int		gpio_ce;
};


struct bq2560x {
	struct	device	*dev;
	struct	i2c_client *client;
	enum	bq2560x_part_no	part_no;
	int	revision;

	struct	bq2560x_config	cfg;
	struct	delayed_work monitor_work;

	bool	usb_present;
	bool	charge_enabled;
	bool	parallel_charger_suspended;

	bool	parallel_charger;
	bool	parallel_charger_present;

	bool	power_good;
	bool	iindpm;
	bool	vindpm;

	bool	in_therm_regulation;
	bool	in_vsys_regulation;


	int	vfloat_mv;
	int	usb_psy_ma;
	int	fast_cc_ma;

	int	usb_suspended_status;

	int	charge_state;
	int	fault_status;

	struct	mutex i2c_rw_lock;
	struct	mutex current_change_lock;
	struct	mutex data_lock;

	struct	power_supply *parallel_psy;
	struct	power_supply_desc parallel_psy_d;

	struct  power_supply *usb_psy;

	int	skip_reads;
	int	skip_writes;
	struct	dentry	*debug_root;
	struct	iio_dev	*indio_dev;
	struct	iio_chan_spec	*iio_chan;
	struct	class	parallel_class;
};

struct bq2560x_iio_channel {
	const char	*datasheet_name;
	int		channel_num;
	enum		iio_chan_type type;
	long		info_mask;
};

#define PL_IIO_CHAN(_name, _num, _type, _mask)		\
	{						\
		.datasheet_name = _name,		\
		.channel_num = _num,			\
		.type = _type,				\
		.info_mask = _mask,			\
	},

#define PL_CHAN_ENERGY(_name, _num)			\
	PL_IIO_CHAN(_name, _num, IIO_ENERGY,		\
		BIT(IIO_CHAN_INFO_PROCESSED))

#define PL_CHAN_INDEX(_name, _num)			\
	PL_IIO_CHAN(_name, _num, IIO_INDEX,		\
		BIT(IIO_CHAN_INFO_PROCESSED))

#define PL_CHAN_TEMP(_name, _num)			\
	PL_IIO_CHAN(_name, _num, IIO_TEMP,		\
		BIT(IIO_CHAN_INFO_PROCESSED))

#define PL_CHAN_VOLT(_name, _num)			\
	PL_IIO_CHAN(_name, _num, IIO_VOLTAGE,		\
		BIT(IIO_CHAN_INFO_PROCESSED))

#define PL_CHAN_CURR(_name, _num)			\
	PL_IIO_CHAN(_name, _num, IIO_CURRENT,		\
		BIT(IIO_CHAN_INFO_PROCESSED))

static const struct bq2560x_iio_channel bq2560x_iio_channels[] = {
	PL_CHAN_INDEX("pl_input_suspend", PSY_IIO_INPUT_SUSPEND)
	PL_CHAN_INDEX("pl_mode", PSY_IIO_PARALLEL_MODE)
	PL_CHAN_INDEX("pl_batfet_mode", PSY_IIO_PARALLEL_BATFET_MODE)
	PL_CHAN_CURR("pl_min_icl", PSY_IIO_MIN_ICL)
	PL_CHAN_CURR("pl_fcc_max", PSY_IIO_PARALLEL_FCC_MAX)
	PL_CHAN_CURR("pl_current_max", PSY_IIO_CURRENT_MAX)
	PL_CHAN_CURR("pl_constant_charge_current_max",
			PSY_IIO_CONSTANT_CHARGE_CURRENT_MAX)
	PL_CHAN_VOLT("pl_voltage_max", PSY_IIO_VOLTAGE_MAX)
	PL_CHAN_INDEX("pl_charge_type", PSY_IIO_CHARGE_TYPE)
};

static void bq2560x_dump_register(struct bq2560x *bq);

static int __bq2560x_read_reg(struct bq2560x *bq, u8 reg, u8 *data)
{
	s32 ret;

	pm_stay_awake(bq->dev);
	ret = i2c_smbus_read_byte_data(bq->client, reg);
	if (ret < 0) {
		pr_err("i2c read fail: can't read from reg 0x%02X\n", reg);
		pm_relax(bq->dev);
		return ret;
	}

	*data = (u8)ret;
	pm_relax(bq->dev);
	pr_debug("Reading 0x%02X=0x%02X\n", reg, *data);
	return 0;
}

static int __bq2560x_write_reg(struct bq2560x *bq, int reg, u8 val)
{
	s32 ret;

	pm_stay_awake(bq->dev);
	ret = i2c_smbus_write_byte_data(bq->client, reg, val);
	if (ret < 0) {
		pr_err("i2c write fail: can't write 0x%02X to reg 0x%02X: %d\n",
				val, reg, ret);
		pm_relax(bq->dev);
		return ret;
	}
	pm_relax(bq->dev);
	pr_debug("Writing 0x%02X=0x%02X\n", reg, val);
	return 0;
}

static int bq2560x_read_byte(struct bq2560x *bq, u8 *data, u8 reg)
{
	int ret;

	if (bq->skip_reads) {
		*data = 0;
		return 0;
	}

	mutex_lock(&bq->i2c_rw_lock);
	ret = __bq2560x_read_reg(bq, reg, data);
	mutex_unlock(&bq->i2c_rw_lock);

	return ret;
}

static int bq2560x_update_bits(struct bq2560x *bq, u8 reg,
		u8 mask, u8 data)
{
	int ret;
	u8 tmp;

	if (bq->skip_reads || bq->skip_writes)
		return 0;

	mutex_lock(&bq->i2c_rw_lock);
	ret = __bq2560x_read_reg(bq, reg, &tmp);
	if (ret) {
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);
		goto out;
	}

	tmp &= ~mask;
	tmp |= data & mask;

	ret = __bq2560x_write_reg(bq, reg, tmp);
	if (ret)
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);

out:
	mutex_unlock(&bq->i2c_rw_lock);
	return ret;
}

static int bq2560x_enable_hiz(struct bq2560x *bq)
{
	int ret;
	u8 val = REG00_HIZ_ENABLE << REG00_ENHIZ_SHIFT;

	ret = bq2560x_update_bits(bq, BQ2560X_REG_00, REG00_ENHIZ_MASK, val);
    pr_debug("[OPLUS_CHG][PL] bq2560x_enable_hiz\n");
	return ret;

}
EXPORT_SYMBOL_GPL(bq2560x_enable_hiz);


static int bq2560x_disable_hiz(struct bq2560x *bq)
{
	int ret;
	u8 val = REG00_HIZ_DISABLE << REG00_ENHIZ_SHIFT;

	ret = bq2560x_update_bits(bq, BQ2560X_REG_00, REG00_ENHIZ_MASK, val);
	pr_debug("[OPLUS_CHG][PL] bq2560x_disable_hiz\n");

	return ret;

}
EXPORT_SYMBOL_GPL(bq2560x_disable_hiz);

static int bq2560x_enable_charger(struct bq2560x *bq)
{
	int ret;
	u8 val = REG01_CHG_ENABLE << REG01_CHG_CONFIG_SHIFT;

	ret = bq2560x_update_bits(bq, BQ2560X_REG_01, REG01_CHG_CONFIG_MASK, val);
	pr_debug("[OPLUS_CHG][PL] bq2560x_enable_charger\n");
	return ret;
}
EXPORT_SYMBOL_GPL(bq2560x_enable_charger);

static int bq2560x_disable_charger(struct bq2560x *bq)
{
	int ret;
	u8 val = REG01_CHG_DISABLE << REG01_CHG_CONFIG_SHIFT;

	ret = bq2560x_update_bits(bq, BQ2560X_REG_01, REG01_CHG_CONFIG_MASK, val);
	pr_debug("[OPLUS_CHG][PL] bq2560x_disable_charger\n");
	return ret;
}
EXPORT_SYMBOL_GPL(bq2560x_disable_charger);

static int bq2560x_enable_term(struct bq2560x *bq, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = REG05_TERM_ENABLE << REG05_EN_TERM_SHIFT;
	else
		val = REG05_TERM_DISABLE << REG05_EN_TERM_SHIFT;

	ret = bq2560x_update_bits(bq, BQ2560X_REG_05, REG05_EN_TERM_MASK, val);

	return ret;
}
EXPORT_SYMBOL_GPL(bq2560x_enable_term);

int bq2560x_set_chargecurrent(struct bq2560x *bq, int curr)
{
	u8 ichg;

	ichg = (curr - REG02_ICHG_BASE)/REG02_ICHG_LSB;
	ichg <<= REG02_ICHG_SHIFT;
	return bq2560x_update_bits(bq, BQ2560X_REG_02, REG02_ICHG_MASK, ichg);

}
EXPORT_SYMBOL_GPL(bq2560x_set_chargecurrent);


static int bq2560x_get_chargecurrent(struct bq2560x *bq, int *curr)
{
	u8 val;
	int ret;

	ret = bq2560x_read_byte(bq, &val, BQ2560X_REG_02);
	if (ret < 0) {
		pr_err("failed to read REG04, ret = %d\n", ret);
		return ret;
	}
	*curr = (val & REG02_ICHG_MASK) >> REG02_ICHG_SHIFT;
	*curr *= REG02_ICHG_LSB;
	*curr += REG02_ICHG_BASE;

	return ret;
}
EXPORT_SYMBOL_GPL(bq2560x_get_chargecurrent);


int bq2560x_set_term_current(struct bq2560x *bq, int curr)
{
	u8 iterm;

	iterm = (curr - REG03_ITERM_BASE) / REG03_ITERM_LSB;
	iterm <<= REG03_ITERM_SHIFT;

	return bq2560x_update_bits(bq, BQ2560X_REG_03, REG03_ITERM_MASK, iterm);
}
EXPORT_SYMBOL_GPL(bq2560x_set_term_current);


int bq2560x_set_prechg_current(struct bq2560x *bq, int curr)
{
	u8 iprechg;

	iprechg = (curr - REG03_IPRECHG_BASE) / REG03_IPRECHG_LSB;
	iprechg <<= REG03_IPRECHG_SHIFT;

	return bq2560x_update_bits(bq, BQ2560X_REG_03, REG03_IPRECHG_MASK, iprechg);
}
EXPORT_SYMBOL_GPL(bq2560x_set_prechg_current);

int bq2560x_set_chargevoltage(struct bq2560x *bq, int volt)
{
	u8 val;

	val = (volt - REG04_VREG_BASE)/REG04_VREG_LSB;
	val <<= REG04_VREG_SHIFT;

	return bq2560x_update_bits(bq, BQ2560X_REG_04, REG04_VREG_MASK, val);
}
EXPORT_SYMBOL_GPL(bq2560x_set_chargevoltage);


int bq2560x_set_input_volt_limit(struct bq2560x *bq, int volt)
{
	u8 val;

	val = (volt - REG06_VINDPM_BASE) / REG06_VINDPM_LSB;
	val <<= REG06_VINDPM_SHIFT;

	return bq2560x_update_bits(bq, BQ2560X_REG_06, REG06_VINDPM_MASK, val);
}
EXPORT_SYMBOL_GPL(bq2560x_set_input_volt_limit);

int bq2560x_set_input_volt_ovp(struct bq2560x *bq, int ovp)
{
	u8 val;

	if (ovp == 5500)
		val = REG06_OVP_5P5V;
	else if (ovp == 6500)
		val = REG06_OVP_6P5V;
	else if (ovp == 10500)
		val = REG06_OVP_10P5V;
	else if (ovp == 14000)
		val = REG06_OVP_14V;
	else
		val = REG06_OVP_6P5V;

	val <<= REG06_OVP_SHIFT;

	return bq2560x_update_bits(bq, BQ2560X_REG_06, REG06_OVP_MASK, val);
}
EXPORT_SYMBOL_GPL(bq2560x_set_input_volt_ovp);

int bq2560x_set_input_current_limit(struct bq2560x *bq, int curr)
{
	u8 val;

	curr += REG00_IINLIM_LSB / 2;
	val = (curr - REG00_IINLIM_BASE) / REG00_IINLIM_LSB;
	val <<= REG00_IINLIM_SHIFT;

	return bq2560x_update_bits(bq, BQ2560X_REG_00, REG00_IINLIM_MASK, val);
}
EXPORT_SYMBOL_GPL(bq2560x_set_input_current_limit);

static int bq2560x_get_input_current_limit(struct bq2560x *bq, int *curr)
{
	u8 val;
	int ret;

	ret = bq2560x_read_byte(bq, &val, BQ2560X_REG_00);
	if (ret < 0) {
		pr_err("failed to read REG00, ret = %d\n", ret);
		return ret;
	}
	*curr = (val & REG00_IINLIM_MASK) >> REG00_IINLIM_SHIFT;
	*curr *= REG00_IINLIM_LSB;
	*curr += REG00_IINLIM_BASE;

	return ret;
}
EXPORT_SYMBOL_GPL(bq2560x_get_input_current_limit);

int bq2560x_set_watchdog_timer(struct bq2560x *bq, u8 timeout)
{
	u8 val;

	val = (timeout - REG05_WDT_BASE) / REG05_WDT_LSB;
	val <<= REG05_WDT_SHIFT;

	return bq2560x_update_bits(bq, BQ2560X_REG_05, REG05_WDT_MASK, val);
}
EXPORT_SYMBOL_GPL(bq2560x_set_watchdog_timer);

int bq2560x_disable_watchdog_timer(struct bq2560x *bq)
{
	u8 val = REG05_WDT_DISABLE << REG05_WDT_SHIFT;

	return bq2560x_update_bits(bq, BQ2560X_REG_05, REG05_WDT_MASK, val);
}
EXPORT_SYMBOL_GPL(bq2560x_disable_watchdog_timer);

int bq2560x_reset_watchdog_timer(struct bq2560x *bq)
{
	u8 val = REG01_WDT_RESET << REG01_WDT_RESET_SHIFT;

	return bq2560x_update_bits(bq, BQ2560X_REG_01, REG01_WDT_RESET_MASK, val);
}
EXPORT_SYMBOL_GPL(bq2560x_reset_watchdog_timer);

int bq2560x_reset_chip(struct bq2560x *bq)
{
	int ret;
	u8 val = REG0B_REG_RESET << REG0B_REG_RESET_SHIFT;

	ret = bq2560x_update_bits(bq, BQ2560X_REG_0B, REG0B_REG_RESET_MASK, val);
	return ret;
}
EXPORT_SYMBOL_GPL(bq2560x_reset_chip);

static int show_registers(struct seq_file *m, void *data)
{
	struct bq2560x *bq = m->private;
	int addr;
	int ret;
	u8 val;

	for (addr = 0x0; addr <= 0x0B; addr++) {
		ret = bq2560x_read_byte(bq, &val, addr);
		if (!ret)
			seq_printf(m, "Reg[%02X] = 0x%02X\n", addr, val);
	}
	return 0;
}


static int reg_debugfs_open(struct inode *inode, struct file *file)
{
	struct bq2560x *bq = inode->i_private;

	return single_open(file, show_registers, bq);
}


static const struct file_operations reg_debugfs_ops = {
	.owner		= THIS_MODULE,
	.open		= reg_debugfs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static void create_debugfs_entries(struct bq2560x *bq)
{
	bq->debug_root = debugfs_create_dir("bq2560x", NULL);
	if (!bq->debug_root)
		pr_err("Failed to create debug dir\n");

	if (bq->debug_root) {

		debugfs_create_file("registers", S_IFREG | S_IRUGO,
				bq->debug_root, bq, &reg_debugfs_ops);

		debugfs_create_x32("usb_psy_ma", S_IFREG | S_IRUGO,
				bq->debug_root, &(bq->usb_psy_ma));

		debugfs_create_x32("fault_status", S_IFREG | S_IRUGO,
				bq->debug_root, &(bq->fault_status));

		debugfs_create_x32("charge_state", S_IFREG | S_IRUGO,
				bq->debug_root, &(bq->charge_state));
	}
}

static int bq2560x_usb_suspend(struct bq2560x *bq, int reason, bool suspend)
{
	int rc = 0;
	int suspended;

	suspended = bq->usb_suspended_status;

	pr_debug("reason = %d requested_suspend = %d suspended_status = %d\n",
			reason, suspend, suspended);

	if (suspend == false)
		suspended &= ~reason;
	else
		suspended |= reason;

	pr_debug("new suspended_status = %d\n", suspended);

	if (suspended) {
		rc = bq2560x_disable_charger(bq);
		bq2560x_enable_hiz(bq);
		gpio_direction_output(bq->cfg.gpio_ce, 1);
		pr_debug("[OPLUS_CHG][PL] bq2560x_usb_suspend  DISABLE charge, suspend = %d\n", suspend);
	}
	else {
		rc = bq2560x_enable_charger(bq);
		bq2560x_disable_hiz(bq);
		gpio_direction_output(bq->cfg.gpio_ce, 0);
		pr_debug("[OPLUS_CHG][PL] bq2560x_usb_suspend  ENABLE charge, suspend = %d\n", suspend);
	}

	if (rc)
		pr_err("Couldn't suspend rc = %d\n", rc);
	else
		bq->usb_suspended_status = suspended;

	return rc;
}


static int bq2560x_set_charge_profile(struct bq2560x *bq);

static int bq2560x_init_device(struct bq2560x *bq)
{
	int ret;

	ret = bq2560x_disable_watchdog_timer(bq);
	if (ret < 0) {
		pr_err("Failed to disable watchdog timer:%d\n", ret);
		return ret;
	}
	bq2560x_disable_charger(bq);

	bq2560x_set_charge_profile(bq);

	ret = bq2560x_enable_term(bq, false);
	if (ret < 0) {
		pr_err("Failed to disable termination:%d\n", ret);
		return ret;
	}


	ret = bq2560x_set_input_volt_ovp(bq, 10500);
	if (ret < 0)
		pr_err("Failed to set ovp threshold to %d\n", 10500);

	return ret;
}

static int bq2560x_get_prop_charge_type(struct bq2560x *bq)
{
	u8 status;
	int ret;

	ret = bq2560x_read_byte(bq, &status, BQ2560X_REG_08);
	if (ret) {
		pr_err("failed to read status register, ret:%d\n", ret);
		return POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;
	}


	status = (status & REG08_CHRG_STAT_MASK) >> REG08_CHRG_STAT_SHIFT;

	if (status == REG08_CHRG_STAT_PRECHG)
		return POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
	else if (status == REG08_CHRG_STAT_FASTCHG)
		return POWER_SUPPLY_CHARGE_TYPE_FAST;
	else if (status == REG08_CHRG_STAT_CHGDONE)
		return POWER_SUPPLY_CHARGE_TYPE_NONE;
	else
		return POWER_SUPPLY_CHARGE_TYPE_NONE;

}

int bq2560x_parallel_set_chg_suspend(struct bq2560x *bq, int suspend)
{
	int rc;

	if (bq->parallel_charger_suspended == suspend) {
		pr_debug("Skip same state request suspended = %d suspend=%d\n",
				bq->parallel_charger_suspended, !suspend);
		//return 0;
	}

	pr_debug("[OPLUS_CHG][PL]  bq2560x_parallel_set_chg_suspend, suspend [%d]\n", suspend);
	if (!suspend) {
		bq->parallel_charger_suspended = false;
		rc = bq2560x_usb_suspend(bq, CURRENT, false);
		if (rc)
			pr_debug("failed to suspend rc=%d\n", rc);

		schedule_delayed_work(&bq->monitor_work, 1 * HZ);
	} else {
		rc = bq2560x_usb_suspend(bq, CURRENT, true);
		if (rc)
			pr_debug("failed to suspend rc=%d\n", rc);

		bq->usb_psy_ma = SUSPEND_CURRENT_MA;
		bq->parallel_charger_suspended = true;
		cancel_delayed_work(&bq->monitor_work);
	}

	return rc;
}

static int bq2560x_set_usb_chg_current(struct bq2560x *bq, int current_ma)
{
	int rc = 0;

	if (current_ma <= SUSPEND_CURRENT_MA) {
		bq2560x_usb_suspend(bq, CURRENT, true);
		pr_err("USB suspend\n");
		return 0;
	}

	rc = bq2560x_set_input_current_limit(bq, current_ma);
	if (rc) {
		pr_err("failed to set input current limit:%d\n", rc);
		return rc;
	}

	rc = bq2560x_set_chargecurrent(bq, current_ma);
	if (rc) {
		pr_err("Couldn't set fast chg current :%d\n", rc);
		return rc;
	}
	bq2560x_usb_suspend(bq, CURRENT, false);

	return rc;
}
static enum power_supply_property bq2560x_parallel_properties[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
};

static int bq2560x_parallel_set_property(struct power_supply *psy,
		enum power_supply_property prop,
		const union power_supply_propval *val)
{
	int rc = 0;

	switch (prop) {
	default:
		pr_err("unsupported prop:%d", prop);
		return -EINVAL;
	}

	return rc;
}

static int bq2560x_parallel_is_writeable(struct power_supply *psy,
		enum power_supply_property prop)
{
	switch (prop) {
	default:
		return 0;
	}
}

static int bq2560x_parallel_get_property(struct power_supply *psy,
		enum power_supply_property prop,
		union power_supply_propval *val)
{
	val->intval = 0;
	switch (prop) {
	default:
		pr_err_ratelimited("parallel psy get prop %d not supported\n",
			prop);
		return -EINVAL;
	}

	return 0;
}

static int bq2560x_parse_dt(struct device *dev, struct bq2560x *bq)
{
	int ret;
	struct device_node *np = dev->of_node;

	bq->usb_suspended_status = of_property_read_bool(np,
			"ti,bq2560x,charging-disabled");

	ret = of_property_read_u32(np, "ti,bq2560x,charge-voltage",
			&bq->cfg.charge_voltage);
	if (ret) {
		pr_err("Failed to read node of ti,bq2560x,charge-voltage\n");
		return ret;
	}
	bq->vfloat_mv = bq->cfg.charge_voltage;

	ret = of_property_read_u32(np, "ti,bq2560x,charge-current",
			&bq->cfg.charge_current);
	if (ret) {
		pr_err("Failed to read node of ti,bq2560x,charge-current\n");
		return ret;
	}

	bq->fast_cc_ma = bq->cfg.charge_current;

	ret = of_property_read_u32(np, "ti,bq2560x,input-current-limit",
			&bq->cfg.iindpm_threshold);
	if (ret) {
		pr_err("Failed to read node of ti,bq2560x,charge-current\n");
		return ret;
	}

	ret = of_property_read_u32(np, "ti,bq2560x,input-voltage-limit",
			&bq->cfg.vindpm_threshold);
	if (ret) {
		pr_err("Failed to read node of ti,bq2560x,charge-current\n");
		return ret;
	}

	bq->cfg.gpio_ce = of_get_named_gpio_flags(np, "ti,ce", 0, NULL);
	if (bq->cfg.gpio_ce) {
		gpio_request(bq->cfg.gpio_ce, "bq-ce");
		bq2560x_enable_hiz(bq);
		gpio_direction_output(bq->cfg.gpio_ce, 1);
		pr_err("[OPLUS_CHG][PL]: %s gpio_ce is %d, then set it high,default disable!\n",
			__func__, bq->cfg.gpio_ce);
	} else {
		pr_err("[OPLUS_CHG][PL]: %s can't get gpio_ce pin!\n", __func__);
	}
	return 0;
}

static int bq2560x_set_charge_profile(struct bq2560x *bq)
{
	int ret;

	ret = bq2560x_set_chargevoltage(bq, bq->cfg.charge_voltage);
	if (ret < 0) {
		pr_err("Failed to set charge voltage:%d\n", ret);
		return ret;
	}

	ret = bq2560x_set_chargecurrent(bq, bq->cfg.charge_current);
	if (ret < 0) {
		pr_err("Failed to set charge current:%d\n", ret);
		return ret;
	}

	ret = bq2560x_set_input_current_limit(bq, bq->cfg.iindpm_threshold);
	if (ret < 0) {
		pr_err("Failed to set input current limit:%d\n", ret);
		return ret;
	}

	ret = bq2560x_set_input_volt_limit(bq, bq->cfg.vindpm_threshold);
	if (ret < 0) {
		pr_err("Failed to set input current limit:%d\n", ret);
		return ret;
	}

	return ret;

}

static const unsigned char *charge_stat_str[] = {
	"Not Charging",
	"Precharging",
	"Fast Charging",
	"Charge Done",
};

static void bq2560x_update_status(struct bq2560x *bq)
{
	int ret;
	u8 status;

	ret = bq2560x_read_byte(bq, &status, BQ2560X_REG_08);
	if (ret == 0) {
		mutex_lock(&bq->data_lock);
		bq->in_therm_regulation = !!(status & REG08_THERM_STAT_MASK);
		bq->in_vsys_regulation = !!(status & REG08_VSYS_STAT_MASK);
		bq->charge_state = (status & REG08_CHRG_STAT_MASK) >> REG08_CHRG_STAT_SHIFT;
		mutex_unlock(&bq->data_lock);
	}

	ret = bq2560x_read_byte(bq, &status, BQ2560X_REG_0A);
	if (ret == 0) {
		mutex_lock(&bq->data_lock);
		bq->vindpm = !!(status & REG0A_VINDPM_STAT_MASK);
		bq->iindpm = !!(status & REG0A_IINDPM_STAT_MASK);
		mutex_unlock(&bq->data_lock);
	}
}

static void bq2560x_dump_register(struct bq2560x *bq)
{
	int ret;
	u8 addr;
	u8 val;

	for (addr = 0x0; addr <= 0x0B; addr++) {
		msleep(20);
		ret = bq2560x_read_byte(bq, &val, addr);
		if (ret)
			pr_err("[%s]:bq2560x_read_byte error!", __func__);
		//	pr_err("Reg[%02X] = 0x%02X\n", addr, val);
	}
}

static void bq2560x_dump_status(struct bq2560x *bq)
{
	u8 status;

	if (bq->in_therm_regulation)
		pr_err("therm regulation triggered\n");

	if (bq->in_vsys_regulation)
		pr_err("vsys regulation triggered\n");

	if (bq->fault_status & REG09_FAULT_WDT_MASK)
		pr_err("Watchdog timer expired!\n");

	if (bq->vindpm)
		pr_err("VINDPM triggered\n");

	if (bq->iindpm)
		pr_err("IINDPM triggered\n");

	status = (bq->fault_status & REG09_FAULT_CHRG_MASK) >> REG09_FAULT_CHRG_SHIFT;
	if (status == REG09_FAULT_CHRG_INPUT)
		pr_err("input fault!\n");
	else if (status == REG09_FAULT_CHRG_THERMAL)
		pr_err("charge thermal shutdown fault!\n");
	else if (status == REG09_FAULT_CHRG_TIMER)
		pr_err("charge timer expired fault!\n");

	if (bq->fault_status & REG09_FAULT_BAT_MASK)
		pr_err("battery ovp fault!\n");

	pr_err("%s\n", charge_stat_str[bq->charge_state]);
}

static void bq2560x_monitor_workfunc(struct work_struct *work)
{
	struct bq2560x *bq = container_of(work, struct bq2560x, monitor_work.work);

	bq2560x_update_status(bq);
	bq2560x_dump_status(bq);
	bq2560x_dump_register(bq);

	schedule_delayed_work(&bq->monitor_work, 5 * HZ);
}

static int bq2560x_iio_read_raw(struct iio_dev *indio_dev,
				     struct iio_chan_spec const *chan,
				     int *val1, int *val2, long mask)
{
	struct bq2560x *bq = iio_priv(indio_dev);
	int rc = 0;

	*val1 = 0;

	switch (chan->channel) {
	case PSY_IIO_CHARGE_TYPE:
		*val1 = bq2560x_get_prop_charge_type(bq);
		break;
//	case PSY_IIO_CHARGING_ENABLED:
//	case PSY_IIO_ONLINE:
//		*val1 = !bq->parallel_charger_suspended;
//		break;
//	case PSY_IIO_PIN_ENABLED:
//		*val1 = 1;//rc = bq2560x_get_prop_pin_enabled(chip, val1);
//		break;
//	case PSY_IIO_CHARGER_TEMP:
//		break;
//	case PSY_IIO_CHARGER_TEMP_MAX:
//		break;
	case PSY_IIO_INPUT_SUSPEND:
		*val1 = 0;//chip->disabled;
		break;
	case PSY_IIO_VOLTAGE_MAX:
		if (!bq->parallel_charger_suspended)
			*val1 = bq->vfloat_mv;
		break;
	case PSY_IIO_CONSTANT_CHARGE_CURRENT_MAX:
		if (!bq->parallel_charger_suspended)
			*val1 = bq->fast_cc_ma * 1000;
		break;
	case PSY_IIO_PARALLEL_MODE:
		*val1 = QTI_POWER_SUPPLY_PL_USBMID_USBMID;
		break;
//	case PSY_IIO_CONNECTOR_HEALTH:
//		break;
//	case PSY_IIO_DIE_HEALTH:
//		break;
	case PSY_IIO_PARALLEL_BATFET_MODE:
		*val1 = QTI_POWER_SUPPLY_PL_NON_STACKED_BATFET;
		break;
//	case PSY_IIO_INPUT_CURRENT_LIMITED:
//		if (!bq->parallel_charger_suspended)
//			*val1 = bq2560x_is_input_current_limited(bq) ? 1 : 0;
//		break;
	case PSY_IIO_CURRENT_MAX:
		if (!bq->parallel_charger_suspended)
			*val1 = bq->usb_psy_ma * 1000;
		break;
	case PSY_IIO_MIN_ICL:
		*val1 = MIN_PARALLEL_ICL_UA;
		break;
//	case PSY_IIO_PARALLEL_FCC_MAX:
//		if (!bq->parallel_charger_suspended)
//                      *val1 = bq->fast_cc_ma * 1000;
//		break;
//	case PSY_IIO_SET_SHIP_MODE:
//		/* Not in ship mode as long as device is active */
//		*val1 = 0;
//		break;
	default:
		pr_err_ratelimited("bq2560x IIO channel %x not supported\n",
			chan->channel);
		return -EINVAL;
	}

	if (rc < 0) {
		pr_debug("Couldn't read channel %x rc = %d\n",
				chan->channel, rc);
		return -ENODATA;
	}

	return IIO_VAL_INT;
}

static int bq2560x_iio_write_raw(struct iio_dev *indio_dev,
				     struct iio_chan_spec const *chan,
				     int val1, int val2, long mask)
{
	struct bq2560x *bq = iio_priv(indio_dev);
	int rc = 0;


	switch (chan->channel) {
	case PSY_IIO_INPUT_SUSPEND:
		rc = bq2560x_parallel_set_chg_suspend(bq, (bool)val1);
		break;
	case PSY_IIO_CURRENT_MAX:
		bq->usb_psy_ma = val1 / 1000;
		rc = bq2560x_set_usb_chg_current(bq, bq->usb_psy_ma);
		break;
	case PSY_IIO_VOLTAGE_MAX:
		bq->vfloat_mv = val1 / 1000;
		rc = bq2560x_set_chargevoltage(bq, bq->vfloat_mv);
		break;
	case PSY_IIO_CONSTANT_CHARGE_CURRENT_MAX:
		bq->usb_psy_ma = val1 / 1000;
		if (!bq->parallel_charger_suspended)
			rc = bq2560x_set_usb_chg_current(bq,
						bq->usb_psy_ma);
		break;
//	case PSY_IIO_CONNECTOR_HEALTH:
//		break;
//	case PSY_IIO_CHARGER_TEMP_MAX:
//		break;
//	case PSY_IIO_SET_SHIP_MODE:
//		break;
	default:
		rc = -EINVAL;
	}

	if (rc < 0)
		pr_debug("Couldn't write to channel %x rc = %d\n",
				chan->channel, rc);
//done:
	return rc;
}

static int bq2560x_iio_of_xlate(struct iio_dev *indio_dev,
				const struct of_phandle_args *iiospec)
{
	struct bq2560x *bq = iio_priv(indio_dev);
	struct iio_chan_spec *iio_chan = bq->iio_chan;
	int i;

	for (i = 0; i < ARRAY_SIZE(bq2560x_iio_channels);
						i++, iio_chan++)
		if (iio_chan->channel == iiospec->args[0])
			return i;

	return -EINVAL;
}

static const struct iio_info bq2560x_iio_info = {
	.read_raw	= bq2560x_iio_read_raw,
	.write_raw	= bq2560x_iio_write_raw,
	.of_xlate	= bq2560x_iio_of_xlate,
};

static int bq2560x_init_iio_psy(struct bq2560x *chip)
{
	struct iio_dev *indio_dev = chip->indio_dev;
	struct iio_chan_spec *chan;
	int bq2560x_num_iio_channels = ARRAY_SIZE(bq2560x_iio_channels);
	int rc, i;

	chip->iio_chan = devm_kcalloc(chip->dev,
				bq2560x_num_iio_channels,
				sizeof(*chip->iio_chan),
				GFP_KERNEL);
	if (!chip->iio_chan)
		return -ENOMEM;

	indio_dev->info = &bq2560x_iio_info;
	indio_dev->dev.parent = chip->dev;
	indio_dev->dev.of_node = chip->dev->of_node;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = chip->iio_chan;
	indio_dev->num_channels = bq2560x_num_iio_channels;
	indio_dev->name = "bq2560x-charger";

	for (i = 0; i < bq2560x_num_iio_channels; i++) {
		chan = &chip->iio_chan[i];
		chan->address = i;
		chan->channel = bq2560x_iio_channels[i].channel_num;
		chan->type = bq2560x_iio_channels[i].type;
		chan->datasheet_name =
			bq2560x_iio_channels[i].datasheet_name;
		chan->extend_name =
			bq2560x_iio_channels[i].datasheet_name;
		chan->info_mask_separate =
			bq2560x_iio_channels[i].info_mask;
	}

	rc = devm_iio_device_register(chip->dev, indio_dev);
	if (rc) {
		pr_err("Failed to register bq2560x Parallel IIO device, rc=%d\n",
			rc);
		return rc;
	}

	return rc;

}

int master_debug_fcc;
static ssize_t master_debug_fcc_show(struct class *c,
				struct class_attribute *attr, char *ubuf)
{
	return scnprintf(ubuf, PAGE_SIZE, "%d\n", master_debug_fcc);
}

static ssize_t master_debug_fcc_store(struct class *c, struct class_attribute *attr,
				const char *ubuf, size_t count)
{
	unsigned long val;

	if (kstrtoul(ubuf, 10, &val))
		return -EINVAL;
	master_debug_fcc = val;
	return count;
}

static CLASS_ATTR_RW(master_debug_fcc);

int slave_debug_fcc;
static ssize_t slave_debug_fcc_show(struct class *c,
				struct class_attribute *attr, char *ubuf)
{
	return scnprintf(ubuf, PAGE_SIZE, "%d\n", slave_debug_fcc);
}

static ssize_t slave_debug_fcc_store(struct class *c, struct class_attribute *attr,
				const char *ubuf, size_t count)
{
	unsigned long val;

	if (kstrtoul(ubuf, 10, &val))
		return -EINVAL;
	slave_debug_fcc = val;
	return count;
}

static CLASS_ATTR_RW(slave_debug_fcc);

static struct attribute *parallel_class_attrs[] = {
	[MASTER_DEBUG_FCC] = &class_attr_master_debug_fcc.attr,
	[SLAVE_DEBUG_FCC] = &class_attr_slave_debug_fcc.attr,
	NULL,
};
ATTRIBUTE_GROUPS(parallel_class);

static int bq2560x_parallel_charger_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct bq2560x *bq;
	int ret;
	struct power_supply_config parallel_psy_cfg = {};
	struct iio_dev *indio_dev;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*bq));
	if (!indio_dev)
		return -ENOMEM;

	bq = iio_priv(indio_dev);
	bq->indio_dev = indio_dev;
	bq->dev = &client->dev;
	bq->client = client;
	bq->parallel_charger = true;
	bq->parallel_charger_suspended = true;


	i2c_set_clientdata(client, bq);

	mutex_init(&bq->i2c_rw_lock);
	mutex_init(&bq->current_change_lock);
	mutex_init(&bq->data_lock);

	if (client->dev.of_node)
		bq2560x_parse_dt(&client->dev, bq);

	ret = bq2560x_init_device(bq);
	if (ret) {
		pr_err("device init failure: %d\n", ret);
		return ret;
	}

	INIT_DELAYED_WORK(&bq->monitor_work, bq2560x_monitor_workfunc);
	bq->parallel_psy_d.name	= "parallel";
	bq->parallel_psy_d.type	= 0;
	bq->parallel_psy_d.get_property = bq2560x_parallel_get_property;
	bq->parallel_psy_d.set_property = bq2560x_parallel_set_property;
	bq->parallel_psy_d.properties   = bq2560x_parallel_properties;
	bq->parallel_psy_d.property_is_writeable = bq2560x_parallel_is_writeable;
	bq->parallel_psy_d.num_properties = ARRAY_SIZE(bq2560x_parallel_properties);

	parallel_psy_cfg.drv_data = bq;
	parallel_psy_cfg.num_supplicants = POWER_SUPPLY_TYPE_UNKNOWN;
	bq->parallel_psy = devm_power_supply_register(bq->dev,
			&bq->parallel_psy_d,
			&parallel_psy_cfg);
	if (IS_ERR(bq->parallel_psy)) {
		pr_err("Couldn't register parallel psy rc=%ld\n",
				PTR_ERR(bq->parallel_psy));
		ret = PTR_ERR(bq->parallel_psy);
		return ret;
	}
	ret = bq2560x_init_iio_psy(bq);
	if (ret < 0) {
		pr_err("Couldn't initialize parallel IIO device rc=%d\n", ret);
		return ret;
	}
	bq->parallel_class.name = "parallel";
	bq->parallel_class.owner = THIS_MODULE;
	bq->parallel_class.class_groups = parallel_class_groups;

	class_register(&bq->parallel_class);
	create_debugfs_entries(bq);
	pr_err("REG03 USB PARALLEL CHARGER successfully probed\n");

	return 0;
}

static int bq2560x_parallel_charger_remove(struct i2c_client *client)
{
	struct bq2560x *bq = i2c_get_clientdata(client);


	cancel_delayed_work_sync(&bq->monitor_work);

	//power_supply_unregister(bq->parallel_psy);

	mutex_destroy(&bq->current_change_lock);
	mutex_destroy(&bq->data_lock);
	mutex_destroy(&bq->i2c_rw_lock);

	debugfs_remove_recursive(bq->debug_root);

	return 0;
}

static void bq2560x_parallel_charger_shutdown(struct i2c_client *client)
{
	pr_err("%s: shutdown\n", __func__);

}

static const struct of_device_id bq2560x_charger_match_table[] = {
	{.compatible = "ti,bq25600-slave",},
	{},
};


static const struct i2c_device_id bq2560x_charger_id[] = {
	{ "bq25600", BQ25600 },
	{},
};

MODULE_DEVICE_TABLE(i2c, bq2560x_charger_id);

static struct i2c_driver bq2560x_charger_driver = {
	.driver		= {
		.name	= "bq25600",
		.of_match_table = bq2560x_charger_match_table,
	},
	.id_table	= bq2560x_charger_id,

	.probe		= bq2560x_parallel_charger_probe,
	.remove		= bq2560x_parallel_charger_remove,
	.shutdown   = bq2560x_parallel_charger_shutdown,
};

module_i2c_driver(bq2560x_charger_driver);

MODULE_DESCRIPTION("TI BQ2560x Charger Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Texas Instruments");
