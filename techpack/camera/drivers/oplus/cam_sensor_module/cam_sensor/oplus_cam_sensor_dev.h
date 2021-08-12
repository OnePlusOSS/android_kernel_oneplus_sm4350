/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
 */

#ifndef _OPLUS_CAM_SENSOR_DEV_H_
#define _OPLUS_CAM_SENSOR_DEV_H_

#include "cam_sensor_dev.h"

struct cam_sensor_i2c_reg_setting_array {
	struct cam_sensor_i2c_reg_array reg_setting[4600];
	unsigned short size;
	enum camera_sensor_i2c_type addr_type;
	enum camera_sensor_i2c_type data_type;
	unsigned short delay;
};

struct cam_sensor_settings {
	struct cam_sensor_i2c_reg_setting_array s5kgm1st_setting;
	struct cam_sensor_i2c_reg_setting_array hi846_setting;
	struct cam_sensor_i2c_reg_setting_array ov8856_setting;
	struct cam_sensor_i2c_reg_setting_array ov02b10_setting;
	struct cam_sensor_i2c_reg_setting_array gc02k0_setting;
	struct cam_sensor_i2c_reg_setting_array gc02m1b_setting;
	struct cam_sensor_i2c_reg_setting_array ov48b_setting;
	struct cam_sensor_i2c_reg_setting_array imx471_setting;
	struct cam_sensor_i2c_reg_setting_array ov13b10_setting;
};

long oplus_cam_sensor_subdev_ioctl(struct v4l2_subdev *sd,
	unsigned int cmd, void *arg, unsigned int *is_ftm_current_test);

#endif /* _OPLUS_CAM_SENSOR_DEV_H_ */
