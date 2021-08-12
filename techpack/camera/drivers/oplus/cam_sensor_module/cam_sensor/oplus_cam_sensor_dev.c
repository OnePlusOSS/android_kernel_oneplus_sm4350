// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
 */
#include "cam_sensor_dev.h"
#include "cam_req_mgr_dev.h"
#include "cam_sensor_soc.h"
#include "cam_sensor_core.h"
#include "oplus_cam_sensor_dev.h"

#define S5KGM1ST_SENSOR_ID		0xF8D1
#define HI846_SENSOR_ID			0x4608
#define OV8856_SENSOR_ID		0x885A
#define GC02K0_SENSOR_ID		0x2385
#define OV02B10_SENSOR_ID		0x002B
#define GC02M1B_SENSOR_ID		0x02e0
#define OV48B_SENSOR_ID			0x5648
#define IMX471_SENSOR_ID		0x0471
#define GC02M1B_N200_SENSOR_ID		0xe000
#define OV13B10_SENSOR_ID	0x0d42
struct cam_sensor_settings sensor_settings = {
#include "CAM_SENSOR_SETTINGS.h"
};

/* Add for AT camera test */
long oplus_cam_sensor_subdev_ioctl(struct v4l2_subdev *sd,
	unsigned int cmd, void *arg, unsigned int *is_ftm_current_test)
{
	int rc = 0;
	struct cam_sensor_ctrl_t *s_ctrl =
		v4l2_get_subdevdata(sd);

    struct cam_sensor_i2c_reg_setting sensor_setting;
	struct cam_sensor_i2c_reg_setting_array *ptr = NULL;
	switch (cmd) {
	case VIDIOC_CAM_FTM_POWNER_DOWN:
		CAM_ERR(CAM_SENSOR, "FTM stream off");
		rc = cam_sensor_power_down(s_ctrl);
        CAM_ERR(CAM_SENSOR, "FTM power down.rc=%d, sensorid is %x",rc,s_ctrl->sensordata->slave_info.sensor_id);
		break;
	case VIDIOC_CAM_FTM_POWNER_UP:
		rc = cam_sensor_power_up(s_ctrl);
		CAM_ERR(CAM_SENSOR, "FTM power up sensor id 0x%x,result %d",s_ctrl->sensordata->slave_info.sensor_id,rc);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "FTM power up failed!");
			break;
		}
        *is_ftm_current_test = 1;
		switch(s_ctrl->sensordata->slave_info.sensor_id){
		case S5KGM1ST_SENSOR_ID:
			ptr = &sensor_settings.s5kgm1st_setting;
			break;
		case HI846_SENSOR_ID:
			ptr = &sensor_settings.hi846_setting;
			break;
		case OV8856_SENSOR_ID:
			ptr = &sensor_settings.ov8856_setting;
			break;
		case GC02K0_SENSOR_ID:
			ptr = &sensor_settings.gc02k0_setting;
			break;
		case OV02B10_SENSOR_ID:
			ptr = &sensor_settings.ov02b10_setting;
			break;
		case GC02M1B_SENSOR_ID:
		case GC02M1B_N200_SENSOR_ID:
			ptr = &sensor_settings.gc02m1b_setting;
			break;
		case OV48B_SENSOR_ID:
			ptr = &sensor_settings.ov48b_setting;
			break;
		case IMX471_SENSOR_ID:
			ptr = &sensor_settings.imx471_setting;
			break;
		case OV13B10_SENSOR_ID:
			ptr = &sensor_settings.ov13b10_setting;
			break;
		default:
			break;
		}
		if (ptr != NULL){
			sensor_setting.reg_setting = ptr->reg_setting;
			sensor_setting.addr_type = ptr->addr_type;
			sensor_setting.data_type = ptr->data_type;
			sensor_setting.size = ptr->size;
			sensor_setting.delay = ptr->delay;
		}
		rc = camera_io_dev_write(&(s_ctrl->io_master_info), &sensor_setting);

		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "FTM Failed to write sensor setting");
		} else {
			CAM_ERR(CAM_SENSOR, "FTM successfully to write sensor setting");
		}
		break;
	default:
		CAM_ERR(CAM_SENSOR, "Invalid ioctl cmd: %d", cmd);
		break;
	}
	return rc;
}

