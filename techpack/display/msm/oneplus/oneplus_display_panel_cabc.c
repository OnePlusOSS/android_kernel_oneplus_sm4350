#include "oneplus_display_panel_cabc.h"
#include "oneplus_dsi_support.h"

int cabc_mode_backup = 1;
int cabc_lock_flag = 0;
int cabc_mode = 1;
EXPORT_SYMBOL(cabc_mode);
EXPORT_SYMBOL(cabc_mode_backup);

enum{
	CABC_LEVEL_0 = 0,
	CABC_LEVEL_1 = 1,
	CABC_LEVEL_2 = 3,
	CABC_EXIT_SPECIAL = 8,
	CABC_ENTER_SPECIAL = 9,
};
DEFINE_MUTEX(oplus_cabc_lock);

int oplus_display_get_cabc_mode(void)
{
	return cabc_mode_backup;
}
EXPORT_SYMBOL(oplus_display_get_cabc_mode);

int __oplus_display_set_cabc(int mode)
{
	mutex_lock(&oplus_cabc_lock);

	if (mode != cabc_mode) {
		cabc_mode = mode;
	}

	if (cabc_mode == CABC_ENTER_SPECIAL) {
		cabc_lock_flag = 1;
		cabc_mode = CABC_LEVEL_0;
	} else if (cabc_mode == CABC_EXIT_SPECIAL) {
		cabc_lock_flag = 0;
		cabc_mode = cabc_mode_backup;
	} else {
		cabc_mode_backup = cabc_mode;
	}

	mutex_unlock(&oplus_cabc_lock);
	printk("%s,cabc mode is %d, cabc_mode_backup is %d, lock_mode is %d\n", __func__, cabc_mode, cabc_mode_backup, cabc_lock_flag);
	if (cabc_mode == cabc_mode_backup && cabc_lock_flag) {
		printk("locked, nothing to do");
		return -1;
	}
	return 0;
}

int dsi_panel_cabc_mode_unlock(struct dsi_panel *panel, int mode)
{
	int rc = 0;

	if (!dsi_panel_initialized(panel)) {
		return -EINVAL;
	}

	switch (mode) {
	case 0:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_CABC_OFF);

		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_CABC_MODE0 cmds, rc=%d\n",
				   panel->name, rc);
		}

		break;

	case 1:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_CABC_MODE1);

		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_CABC_MODE1 cmds, rc=%d\n",
				   panel->name, rc);
		}

		break;

	case 2:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_CABC_MODE2);

		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_CABC_MODE2 cmds, rc=%d\n",
				   panel->name, rc);
		}

		break;

	case 3:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_CABC_MODE3);

		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_CABC_MODE3 cmds, rc=%d\n",
				   panel->name, rc);
		}

		break;

	default:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_CABC_MODE1);

		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_CABC_OFF cmds, rc=%d\n",
				   panel->name, rc);
		}

		pr_err("[%s] cabc mode Invalid %d\n",
			   panel->name, mode);
	}

	return rc;
}

int dsi_panel_cabc_mode(struct dsi_panel *panel, int mode)
{
	int rc = 0;

	if (!panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);

	rc = dsi_panel_cabc_mode_unlock(panel, mode);

	mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_display_cabc_mode(struct dsi_display *display, int mode)
{
	int rc = 0;

	if (!display || !display->panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	if (display->panel->panel_type == DSI_DISPLAY_PANEL_TYPE_OLED) {
		printk("oled cabc no need!\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
					 DSI_CORE_CLK, DSI_CLK_ON);
	}

	rc = dsi_panel_cabc_mode(display->panel, mode);

	if (rc) {
		pr_err("[%s] failed to dsi_panel_cabc_on, rc=%d\n",
			   display->name, rc);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
					  DSI_CORE_CLK, DSI_CLK_OFF);
	}

	mutex_unlock(&display->display_lock);
	return rc;
}

int oplus_dsi_update_cabc_mode(void)
{
	struct dsi_display *display = get_main_display();
	int ret = 0;

	if (!display) {
		pr_err("failed for: %s %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	ret = dsi_display_cabc_mode(display, cabc_mode);

	return ret;
}

int oplus_display_panel_get_cabc(void *data)
{
	uint32_t *temp = data;
	printk(KERN_INFO "oplus_display_get_cabc = %d\n", cabc_mode);

	(*temp) = cabc_mode;
	return 0;
}

int oplus_display_panel_set_cabc(void *data)
{
	uint32_t *temp_save = data;

	printk(KERN_INFO "%s oplus_display_set_cabc = %d\n", __func__, *temp_save);
	cabc_mode = *temp_save;

	__oplus_display_set_cabc(*temp_save);

	if (get_oplus_display_power_status() == OPLUS_DISPLAY_POWER_ON) {
		if (get_main_display() == NULL) {
			printk(KERN_INFO "oplus_display_set_cabc and main display is null");
			return -EINVAL;
		}

		dsi_display_cabc_mode(get_main_display(), cabc_mode);

	} else {
		printk(KERN_ERR
			   "%s oplus_display_set_cabc = %d, but now display panel status is not on\n",
			   __func__, *temp_save);
	}

	return 0;
}
