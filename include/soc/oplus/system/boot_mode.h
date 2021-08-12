/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */
/************************************************************************************
**
** Description:
**     change define of boot_mode here for other place to use it
** Version: 1.0
************************************************************************************/
#ifndef _OPLUS_BOOT_H
#define _OPLUS_BOOT_H

extern int get_boot_mode(void);

/*add for charge*/
//extern bool qpnp_is_power_off_charging(void);
//EXPORT_SYMBOL(qpnp_is_power_off_charging);

/*add for detect charger when reboot */
//extern bool qpnp_is_charger_reboot(void);
//EXPORT_SYMBOL(qpnp_is_charger_reboot);
#endif  /*_OPLUS_BOOT_H*/


/*Add for kernel monitor whole bootup*/

extern bool op_is_monitorable_boot(void);



