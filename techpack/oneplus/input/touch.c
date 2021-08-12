#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/serio.h>
#include <linux/regulator/consumer.h>
#include "oplus_touchscreen/tp_devices.h"
#include "oplus_touchscreen/touchpanel_common.h"
#include <soc/oplus/oplus_project.h>
#include <soc/oplus/device_info.h>
#if IS_MODULE(CONFIG_TOUCHPANEL_OPLUS)

#define MAX_CMDLINE_PARAM_LEN 512
char tp_dsi_display_primary[MAX_CMDLINE_PARAM_LEN];
EXPORT_SYMBOL(tp_dsi_display_primary);
#endif
#define MAX_LIMIT_DATA_LENGTH         100
#if IS_BUILTIN(CONFIG_TOUCHPANEL_OPLUS)
extern char *saved_command_line;
#endif
int g_tp_prj_id = 0;
int g_tp_dev_vendor = TP_UNKNOWN;
int j = 0;
char *chip_name = NULL;


void primary_display_esd_check_enable(int enable)
{
    return;
}

EXPORT_SYMBOL(primary_display_esd_check_enable);
/*if can not compile success, please update vendor/oplus_touchsreen*/
struct tp_dev_name tp_dev_names[] = {
    {TP_OFILM, "OFILM"},
    {TP_BIEL, "BIEL"},
    {TP_TRULY, "TRULY"},
    {TP_BOE, "BOE"},
    {TP_G2Y, "G2Y"},
    {TP_TPK, "TPK"},
    {TP_JDI, "JDI"},
    {TP_TIANMA, "TIANMA"},
    {TP_SAMSUNG, "SAMSUNG"},
    {TP_DSJM, "DSJM"},
    {TP_BOE_B8, "BOEB8"},
    {TP_INNOLUX, "INNOLUX"},
    {TP_HIMAX_DPT, "DPT"},
    {TP_AUO, "AUO"},
    {TP_DEPUTE, "DEPUTE"},
    {TP_HUAXING, "HUAXING"},
    {TP_HLT, "HLT"},
    {TP_DJN, "DJN"},
    {TP_UNKNOWN, "UNKNOWN"},
};

typedef enum {
    TP_INDEX_NULL,
    himax_83112a,
    himax_83112f,
    ili9881_auo,
    ili9881_tm,
    nt36525b_boe,
    nt36525b_hlt,
    nt36672c,
    ili9881_inx,
	goodix_gt9886,
	focal_ft3518,
	ili7807s_tm
} TP_USED_INDEX;
TP_USED_INDEX tp_used_index  = TP_INDEX_NULL;

#define GET_TP_DEV_NAME(tp_type) ((tp_dev_names[tp_type].type == (tp_type))?tp_dev_names[tp_type].name:"UNMATCH")
#if IS_BUILTIN(CONFIG_TOUCHPANEL_OPLUS)
bool tp_judge_ic_match(char *tp_ic_name)
{

#ifdef CONFIG_TOUCHPANEL_MULTI_NOFLASH
    if (strstr(saved_command_line, tp_ic_name)) {
        return true;
    }

    pr_err("Lcd module not found\n");
    return false;
#else
    return true;
#endif

}
EXPORT_SYMBOL(tp_judge_ic_match);
#endif
int tp_judge_ic_match_commandline(struct panel_info *panel_data)
{
	int prj_id = 0;
	int i = 0;

	prj_id = panel_data->project_id;
#if IS_MODULE(CONFIG_TOUCHPANEL_OPLUS)
	memcpy(tp_dsi_display_primary,"qcom,mdss_dsi_nt36672c_tm_video",32);
	pr_err("[TP] tp_dsi_display_primary = %s \n", tp_dsi_display_primary);
#elif IS_BUILTIN(CONFIG_TOUCHPANEL_OPLUS)
	pr_err("[TP] boot_command_line = %s \n", saved_command_line);
#endif
	for (i = 0; i < panel_data->project_num; i++) {
		if (prj_id == panel_data->platform_support_project[i]) {
			g_tp_prj_id = panel_data->platform_support_project_dir[i];
			pr_err("[TP] Driver match support project [%d]\n", panel_data->platform_support_project[i]);
			for(j = 0; j < panel_data->panel_num; j++) {
#if IS_MODULE(CONFIG_TOUCHPANEL_OPLUS)
				if(strstr(tp_dsi_display_primary, panel_data->platform_support_commandline[j])||
					strstr("default_commandline", panel_data->platform_support_commandline[j]))
#elif IS_BUILTIN(CONFIG_TOUCHPANEL_OPLUS)
				if(strstr(saved_command_line, panel_data->platform_support_commandline[j]) ||
					strstr("default_commandline", panel_data->platform_support_commandline[j]))
#endif
				{
					panel_data->tp_type = panel_data->panel_type[j];
					if(panel_data->chip_num > 1) {
						chip_name = panel_data->chip_name[j];
						pr_err("[TP] WGL--1 chip_name = %s, panel_data->chip_name = %s", chip_name, panel_data->chip_name[j]);
					}
					pr_err("[TP] match panel type OK , panel type is [%d]\n", panel_data->tp_type);
					return j;
				}
				pr_err("[TP] Panel not found\n");
			}
		}
	}
	pr_err("[TP] Driver does not match the project\n");
	return -1;
}
EXPORT_SYMBOL(tp_judge_ic_match_commandline);

int tp_util_get_vendor(struct hw_resource *hw_res, struct panel_info *panel_data)
{
    char *vendor;

    panel_data->test_limit_name = kzalloc(MAX_LIMIT_DATA_LENGTH, GFP_KERNEL);
    if (panel_data->test_limit_name == NULL) {
        pr_err("[TP]panel_data.test_limit_name kzalloc error\n");
    }

    if (panel_data->tp_type == TP_UNKNOWN) {
        pr_err("[TP]%s type is unknown\n", __func__);
        return 0;
    }
    memcpy(panel_data->manufacture_info.version, panel_data->firmware_name[j], strlen(panel_data->firmware_name[j]));

    vendor = GET_TP_DEV_NAME(panel_data->tp_type);
    if(panel_data->chip_num == 1) {
        chip_name = panel_data->chip_name[0];
    }
    strcpy(panel_data->manufacture_info.manufacture, vendor);
    snprintf(panel_data->fw_name, MAX_FW_NAME_LENGTH,
            "tp/%d/FW_%s_%s.img",
            g_tp_prj_id, chip_name, vendor);

    if (panel_data->test_limit_name) {
        snprintf(panel_data->test_limit_name, MAX_LIMIT_DATA_LENGTH,
            "tp/%d/LIMIT_%s_%s.img",
            g_tp_prj_id, chip_name, vendor);
    }

    panel_data->manufacture_info.fw_path = panel_data->fw_name;

    pr_info("[TP]vendor:%s fw:%s limit:%s\n",
        vendor,
        panel_data->fw_name,
        panel_data->test_limit_name==NULL?"NO Limit":panel_data->test_limit_name);
    return 0;
}
EXPORT_SYMBOL(tp_util_get_vendor);

MODULE_DESCRIPTION("Touchscreen Common Driver");
MODULE_LICENSE("GPL");
