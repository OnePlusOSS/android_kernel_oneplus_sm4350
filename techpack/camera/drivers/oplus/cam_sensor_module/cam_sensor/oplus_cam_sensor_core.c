// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
 */

#include <linux/module.h>
#include <cam_sensor_cmn_header.h>
#include "cam_sensor_util.h"
#include "cam_soc_util.h"
#include "cam_trace.h"
#include "cam_common_util.h"
#include "cam_packet_util.h"
#include "oplus_cam_sensor_core.h"
#include "cam_sensor_core.h"

#define MAX_LENGTH 128

/*add by hongbo.dai@camera 20190225, for fix current leak issue*/
/*static int RamWriteByte(struct camera_io_master *cci_master_info,
	uint32_t addr, uint32_t data, unsigned short mdelay)
{
	int32_t rc = 0;
	int retry = 1;
	int i = 0;
	struct cam_sensor_i2c_reg_array i2c_write_setting = {
		.reg_addr = addr,
		.reg_data = data,
		.delay = mdelay,
		.data_mask = 0x00,
	};
	struct cam_sensor_i2c_reg_setting i2c_write = {
		.reg_setting = &i2c_write_setting,
		.size = 1,
		.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD,
		.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE,
		.delay = mdelay,
	};
	if (cci_master_info == NULL) {
		CAM_ERR(CAM_SENSOR, "Invalid Args");
		return -EINVAL;
	}

	for( i = 0; i < retry; i++)
	{
		rc = camera_io_dev_write(cci_master_info, &i2c_write);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "write 0x%04x failed, retry:%d", addr, i+1);
		} else {
			return rc;
		}
	}
	return rc;
}


static int RamWriteWord(struct camera_io_master *cci_master_info,
	uint32_t addr, uint32_t data)
{
	int32_t rc = 0;
	int retry = 1;
	int i = 0;
	struct cam_sensor_i2c_reg_array i2c_write_setting = {
		.reg_addr = addr,
		.reg_data = data,
		.delay = 0x00,
		.data_mask = 0x00,
	};
	struct cam_sensor_i2c_reg_setting i2c_write = {
		.reg_setting = &i2c_write_setting,
		.size = 1,
		.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD,
		.data_type = CAMERA_SENSOR_I2C_TYPE_WORD,
		.delay = 0x00,
	};
	if (addr == 0x8c) {
		i2c_write .addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
		i2c_write .data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	}
	if (cci_master_info == NULL) {
		CAM_ERR(CAM_SENSOR, "Invalid Args");
		return -EINVAL;
	}

	for( i = 0; i < retry; i++)
	{
		rc = camera_io_dev_write(cci_master_info, &i2c_write);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "write 0x%04x failed, retry:%d", addr, i+1);
		} else {
			return rc;
		}
	}
	return rc;
}*/

struct sony_dfct_tbl_t imx471_dfct_tbl;

int oplus_sensor_imx471_get_dpc_data(struct cam_sensor_ctrl_t *s_ctrl)
{
    int i = 0, j = 0;
    int rc = 0;
    int check_reg_val, dfct_data_h, dfct_data_l;
    int dfct_data = 0;
    int fd_dfct_num = 0, sg_dfct_num = 0;
    int retry_cnt = 5;
    int data_h = 0, data_v = 0;
    int fd_dfct_addr = FD_DFCT_ADDR;
    int sg_dfct_addr = SG_DFCT_ADDR;

    CAM_INFO(CAM_SENSOR, "sensor_imx471_get_dpc_data enter");
    if (s_ctrl == NULL) {
        CAM_ERR(CAM_SENSOR, "Invalid Args");
        return -EINVAL;
    }

    memset(&imx471_dfct_tbl, 0, sizeof(struct sony_dfct_tbl_t));

    for (i = 0; i < retry_cnt; i++) {
        check_reg_val = 0;
        rc = camera_io_dev_read(&(s_ctrl->io_master_info),
            FD_DFCT_NUM_ADDR, &check_reg_val,
            CAMERA_SENSOR_I2C_TYPE_WORD,
            CAMERA_SENSOR_I2C_TYPE_BYTE);

        if (0 == rc) {
            fd_dfct_num = check_reg_val & 0x07;
            if (fd_dfct_num > FD_DFCT_MAX_NUM)
                fd_dfct_num = FD_DFCT_MAX_NUM;
            break;
        }
    }

    for (i = 0; i < retry_cnt; i++) {
        check_reg_val = 0;
        rc = camera_io_dev_read(&(s_ctrl->io_master_info),
            SG_DFCT_NUM_ADDR, &check_reg_val,
            CAMERA_SENSOR_I2C_TYPE_WORD,
            CAMERA_SENSOR_I2C_TYPE_WORD);

        if (0 == rc) {
            sg_dfct_num = check_reg_val & 0x01FF;
            if (sg_dfct_num > SG_DFCT_MAX_NUM)
                sg_dfct_num = SG_DFCT_MAX_NUM;
            break;
        }
    }

    CAM_INFO(CAM_SENSOR, " fd_dfct_num = %d, sg_dfct_num = %d", fd_dfct_num, sg_dfct_num);
    imx471_dfct_tbl.fd_dfct_num = fd_dfct_num;
    imx471_dfct_tbl.sg_dfct_num = sg_dfct_num;

    if (fd_dfct_num > 0) {
        for (j = 0; j < fd_dfct_num; j++) {
            dfct_data = 0;
            for (i = 0; i < retry_cnt; i++) {
                dfct_data_h = 0;
                rc = camera_io_dev_read(&(s_ctrl->io_master_info),
                        fd_dfct_addr, &dfct_data_h,
                        CAMERA_SENSOR_I2C_TYPE_WORD,
                        CAMERA_SENSOR_I2C_TYPE_WORD);
                if (0 == rc) {
                    break;
                }
            }
            for (i = 0; i < retry_cnt; i++) {
                dfct_data_l = 0;
                rc = camera_io_dev_read(&(s_ctrl->io_master_info),
                        fd_dfct_addr+2, &dfct_data_l,
                        CAMERA_SENSOR_I2C_TYPE_WORD,
                        CAMERA_SENSOR_I2C_TYPE_WORD);
                if (0 == rc) {
                    break;
                }
            }
            CAM_DBG(CAM_SENSOR, " dfct_data_h = 0x%x, dfct_data_l = 0x%x", dfct_data_h, dfct_data_l);
            dfct_data = (dfct_data_h << 16) | dfct_data_l;
            data_h = 0;
            data_v = 0;
            data_h = (dfct_data & (H_DATA_MASK >> j%8)) >> (19 - j%8); //19 = 32 -13;
            data_v = (dfct_data & (V_DATA_MASK >> j%8)) >> (7 - j%8);  // 7 = 32 -13 -12;
            CAM_DBG(CAM_SENSOR, "j = %d, H = %d, V = %d", j, data_h, data_v);
            imx471_dfct_tbl.fd_dfct_addr[j] = ((data_h & 0x1FFF) << V_ADDR_SHIFT) | (data_v & 0x0FFF);
            CAM_DBG(CAM_SENSOR, "fd_dfct_data[%d] = 0x%08x", j, imx471_dfct_tbl.fd_dfct_addr[j]);
            fd_dfct_addr = fd_dfct_addr + 3 + ((j+1)%8 == 0);
        }
    }
    if (sg_dfct_num > 0) {
        for (j = 0; j < sg_dfct_num; j++) {
            dfct_data = 0;
            for (i = 0; i < retry_cnt; i++) {
                dfct_data_h = 0;
                rc = camera_io_dev_read(&(s_ctrl->io_master_info),
                        sg_dfct_addr, &dfct_data_h,
                        CAMERA_SENSOR_I2C_TYPE_WORD,
                        CAMERA_SENSOR_I2C_TYPE_WORD);
                if (0 == rc) {
                    break;
                }
            }
            for (i = 0; i < retry_cnt; i++) {
                dfct_data_l = 0;
                rc = camera_io_dev_read(&(s_ctrl->io_master_info),
                        sg_dfct_addr+2, &dfct_data_l,
                        CAMERA_SENSOR_I2C_TYPE_WORD,
                        CAMERA_SENSOR_I2C_TYPE_WORD);
                if (0 == rc) {
                    break;
                }
            }
            CAM_DBG(CAM_SENSOR, " dfct_data_h = 0x%x, dfct_data_l = 0x%x", dfct_data_h, dfct_data_l);
            dfct_data = (dfct_data_h << 16) | dfct_data_l;
            data_h = 0;
            data_v = 0;
            data_h = (dfct_data & (H_DATA_MASK >> j%8)) >> (19 - j%8); //19 = 32 -13;
            data_v = (dfct_data & (V_DATA_MASK >> j%8)) >> (7 - j%8);  // 7 = 32 -13 -12;
            CAM_DBG(CAM_SENSOR, "j = %d, H = %d, V = %d", j, data_h, data_v);
            imx471_dfct_tbl.sg_dfct_addr[j] = ((data_h & 0x1FFF) << V_ADDR_SHIFT) | (data_v & 0x0FFF);
            CAM_DBG(CAM_SENSOR, "sg_dfct_data[%d] = 0x%08x", j, imx471_dfct_tbl.sg_dfct_addr[j]);
            sg_dfct_addr = sg_dfct_addr + 3 + ((j+1)%8 == 0);
        }
    }

    CAM_INFO(CAM_SENSOR, "exit");
    return rc;
}

int32_t oplus_cam_sensor_driver_cmd(struct cam_sensor_ctrl_t *s_ctrl,
	void *arg)
{
        int rc = 0;
        struct cam_control *cmd = (struct cam_control *)arg;
/*Zhixian.mai@Cam.Drv 20200329 add for oem ioctl for read /write register*/
	switch (cmd->op_code) {
	case CAM_OEM_IO_CMD:{
		struct cam_oem_rw_ctl oem_ctl;
		struct camera_io_master oem_io_master_info;
		struct cam_sensor_cci_client oem_cci_client;
              struct cam_oem_i2c_reg_array *cam_regs = NULL;
		if (copy_from_user(&oem_ctl, (void __user *)cmd->handle,
			sizeof(struct cam_oem_rw_ctl))) {
			CAM_ERR(CAM_SENSOR,
					"Fail in copy oem control infomation form user data");
                      rc = -ENOMEM;
                      return rc;
		}
		if (oem_ctl.num_bytes > 0) {
			cam_regs = (struct cam_oem_i2c_reg_array *)kzalloc(
				sizeof(struct cam_oem_i2c_reg_array)*oem_ctl.num_bytes, GFP_KERNEL);
			if (!cam_regs) {
				rc = -ENOMEM;
                             CAM_ERR(CAM_SENSOR,"failed alloc cam_regs");
				return rc;
			}

			if (copy_from_user(cam_regs, u64_to_user_ptr(oem_ctl.cam_regs_ptr),
				sizeof(struct cam_oem_i2c_reg_array)*oem_ctl.num_bytes)) {
				CAM_INFO(CAM_SENSOR, "copy_from_user error!!!", oem_ctl.num_bytes);
				rc = -EFAULT;
				goto free_cam_regs;
			}
		}
		memcpy(&oem_io_master_info, &(s_ctrl->io_master_info),sizeof(struct camera_io_master));
		memcpy(&oem_cci_client, s_ctrl->io_master_info.cci_client,sizeof(struct cam_sensor_cci_client));
		oem_io_master_info.cci_client = &oem_cci_client;
		if (oem_ctl.slave_addr != 0) {
			oem_io_master_info.cci_client->sid = (oem_ctl.slave_addr >> 1);
		}

		switch (oem_ctl.cmd_code) {
        	case CAM_OEM_CMD_READ_DEV: {
			int i = 0;
			for (; i < oem_ctl.num_bytes; i++)
			{
				rc |= cam_cci_i2c_read(
					 oem_io_master_info.cci_client,
					 cam_regs[i].reg_addr,
					 &(cam_regs[i].reg_data),
					 oem_ctl.reg_addr_type,
					 oem_ctl.reg_data_type);
				CAM_INFO(CAM_SENSOR,
					"read addr:0x%x  Data:0x%x ",
					cam_regs[i].reg_addr, cam_regs[i].reg_data);
			}

			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"Fail oem ctl data ,slave sensor id is 0x%x",s_ctrl->sensordata->slave_info.sensor_id);
				goto free_cam_regs;;
			}

			if (copy_to_user(u64_to_user_ptr(oem_ctl.cam_regs_ptr), cam_regs,
				sizeof(struct cam_oem_i2c_reg_array)*oem_ctl.num_bytes)) {
				CAM_ERR(CAM_SENSOR,
						"Fail oem ctl data ,slave sensor id is 0x%x",s_ctrl->sensordata->slave_info.sensor_id);
				goto free_cam_regs;

			}
			break;
		}
		case CAM_OEM_CMD_WRITE_DEV: {
			struct cam_sensor_i2c_reg_setting write_setting;
			int i = 0;
			for (;i < oem_ctl.num_bytes; i++)
			{
				CAM_DBG(CAM_SENSOR,"Get from OEM addr: 0x%x data: 0x%x ",
								cam_regs[i].reg_addr, cam_regs[i].reg_data);
			}

			write_setting.addr_type = oem_ctl.reg_addr_type;
			write_setting.data_type = oem_ctl.reg_data_type;
			write_setting.size = oem_ctl.num_bytes;
			write_setting.reg_setting = (struct cam_sensor_i2c_reg_array*)cam_regs;

			rc = cam_cci_i2c_write_table(&oem_io_master_info,&write_setting);

			if (rc < 0){
				CAM_ERR(CAM_SENSOR,
					"Fail oem write data ,slave sensor id is 0x%x",s_ctrl->sensordata->slave_info.sensor_id);
				goto free_cam_regs;
			}

			break;
		}

		case CAM_OEM_OIS_CALIB : {
			/*rc = cam_ois_sem1215s_calibration(&oem_io_master_info);
                      CAM_ERR(CAM_SENSOR, "ois calib failed rc:%d", rc);
			break;*/
		}

		default:
			CAM_ERR(CAM_SENSOR,
						"Unknow OEM cmd ,slave sensor id is 0x%x",s_ctrl->sensordata->slave_info.sensor_id);
			break ;
		}

free_cam_regs:
		if (cam_regs != NULL) {
			kfree(cam_regs);
			cam_regs = NULL;
		}
		mutex_unlock(&(s_ctrl->cam_sensor_mutex));
		return rc;
	}

	case CAM_OEM_GET_ID : {
		if (copy_to_user((void __user *)cmd->handle,&s_ctrl->soc_info.index,
						sizeof(uint32_t))) {
			CAM_ERR(CAM_SENSOR,
					"copy camera id to user fail ");
		}
		break;
	}

	/*add by hongbo.dai@camera 20190221, get DPC Data for IMX471*/
	case CAM_GET_DPC_DATA: {
		if (0x0471 != s_ctrl->sensordata->slave_info.sensor_id) {
			rc = -EFAULT;
			return rc;
		}
		CAM_INFO(CAM_SENSOR, "imx471_dfct_tbl: fd_dfct_num=%d, sg_dfct_num=%d",
			imx471_dfct_tbl.fd_dfct_num, imx471_dfct_tbl.sg_dfct_num);
		if (copy_to_user((void __user *) cmd->handle, &imx471_dfct_tbl,
			sizeof(struct  sony_dfct_tbl_t))) {
			CAM_ERR(CAM_SENSOR, "Failed Copy to User");
			rc = -EFAULT;
			return rc;
		}
	}
		break;

    default:
        CAM_ERR(CAM_SENSOR, "Invalid Opcode: %d", cmd->op_code);
		rc = -EINVAL;
    break;
    }

	return rc;
}