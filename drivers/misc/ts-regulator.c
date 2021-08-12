/* Copyright (c) 2019, Thundersoft.:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_runtime.h>

#include <linux/delay.h>

#define REG_DEV_NAME "reg_test"

struct regtest_dev {
    struct device *dev;
    struct regulator *l6e;
    struct regulator *l10a;
    struct regulator *l11e;
};

struct regtest_dev *regtest_dev_ptr;
const char *L10A_name = "L10A";
const char *L11E_name = "L11E";
const char *L6E_name = "L6E";

extern int get_hw_board_version(void);

static int regtest_probe(struct platform_device *pdev) {

    int ret = 0;
    int hw_version = get_hw_board_version();

    pr_err("%s hw_version=%d\n", __func__,hw_version);

    regtest_dev_ptr = devm_kzalloc(&pdev->dev, sizeof(struct regtest_dev), GFP_KERNEL);
    if (regtest_dev_ptr == NULL) {
        pr_err("Error: unable to allocate driver memory\n");
        return -EINVAL;
    }
    regtest_dev_ptr->dev = &pdev->dev;


    if(hw_version <= 4){
        regtest_dev_ptr->l11e = regulator_get(regtest_dev_ptr->dev, L11E_name);
        regulator_set_load(regtest_dev_ptr->l11e, 150000);
        regulator_set_voltage(regtest_dev_ptr->l11e, 2700000, 2700000);

        ret =  regulator_enable(regtest_dev_ptr->l11e);
        if (ret) {
            pr_err("%s enable L11E fail!\n", __func__);
        } else {
            pr_err("%s enable L11E  success!\n", __func__);
        }

        udelay(10);

        ret =  regulator_disable(regtest_dev_ptr->l11e);
        if (ret) {
            pr_err("%s disable L11E fail!\n", __func__);
        } else {
            pr_err("%s disable L11E  success!\n", __func__);
        }

        regtest_dev_ptr->l10a = regulator_get(regtest_dev_ptr->dev, L10A_name);
        regulator_set_load(regtest_dev_ptr->l10a, 150000);
        regulator_set_voltage(regtest_dev_ptr->l10a, 2700000, 2700000);

        ret =  regulator_enable(regtest_dev_ptr->l10a);
        if (ret) {
            pr_err("%s enable L10A fail!\n", __func__);
        } else {
            pr_err("%s enable L10A  success!\n", __func__);
        }
    }
    else{

    regtest_dev_ptr->l10a = regulator_get(regtest_dev_ptr->dev, L10A_name);
    regulator_set_load(regtest_dev_ptr->l10a, 150000);
    regulator_set_voltage(regtest_dev_ptr->l10a, 2700000, 2700000);

    ret =  regulator_enable(regtest_dev_ptr->l10a);
    if (ret) {
        pr_err("%s enable L10A fail!\n", __func__);
    } else {
        pr_err("%s enable L10A  success!\n", __func__);
    }

    udelay(10);
    
        ret =  regulator_disable(regtest_dev_ptr->l10a);
        if (ret) {
            pr_err("%s disable L10A fail!\n", __func__);
        } else {
            pr_err("%s disable L10A  success!\n", __func__);
        }
    }

    
    return 0;
}

static const struct of_device_id regtest_match_table[] = {
    {
        .compatible = REG_DEV_NAME,
    },
    {},
};

static struct platform_driver regtest_driver = {
    .probe = regtest_probe,
    .driver = {
        .name = REG_DEV_NAME,
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(regtest_match_table),
    },
};

static int __init regtest_init(void) {
    int ret = 0;

    pr_err("%s\n", __func__);
    ret = platform_driver_register(&regtest_driver);
    if (ret)
        return ret;

    return 0;
}

static void __exit regtest_exit(void) {
    platform_driver_unregister(&regtest_driver);
}

late_initcall(regtest_init);
module_exit(regtest_exit);

MODULE_AUTHOR("Oneplus sun daoyong");
MODULE_DESCRIPTION("Oneplus regulator test Driver");
MODULE_LICENSE("GPL");
