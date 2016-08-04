/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2010. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
* AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
* NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
* SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
* SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
* THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
* THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
* CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
* SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
* CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
* AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
* OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
* MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
* The following software/firmware and/or related documentation ("MediaTek Software")
* have been modified by MediaTek Inc. All revisions are subject to any receiver's
* applicable license agreements with MediaTek Inc.
*/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/xlog.h>

#include "kd_camera_hw.h"

#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_camera_feature.h"

/******************************************************************************
* Debug configuration
******************************************************************************/
#define PFX "[kd_camera_hw]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC printk

#define DEBUG_CAMERA_HW_K

#ifdef  DEBUG_CAMERA_HW_K
#define PK_DBG PK_DBG_FUNC
#define PK_ERR(fmt, arg...)         printk(KERN_ERR PFX "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_XLOG_INFO(fmt, args...) \
	do {    \
	xlog_printk(ANDROID_LOG_INFO, "kd_camera_hw", fmt, ##args); \
	} while(0)
#else
#define PK_DBG(a,...)
#define PK_ERR(a,...)
#define PK_XLOG_INFO(fmt, args...)
#endif

#define CIS_CAMERA_DVDD_ON(powerVolt , mode_name) \
	if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, powerVolt,mode_name)){ \
	PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n"); \
	goto _kdCISModulePowerOn_exit_; \
	}

#define CIS_CAMERA_AVDD_ON(powerVolt , mode_name)\
	if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, powerVolt,mode_name)){ \
	PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");  \
	goto _kdCISModulePowerOn_exit_;\
	}

#define CIS_CAMERA_VIO_ON(powerVolt , mode_name)\
	if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, powerVolt,mode_name)){\
	PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");\
	goto _kdCISModulePowerOn_exit_;\
	}

#define CIS_CAMERA_DVDD_OFF(mode_name) \
	if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D,mode_name)){\
	PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");\
	goto _kdCISModulePowerOn_exit_;\
	}

#define CIS_CAMERA_AVDD_OFF(mode_name)\
	if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A,mode_name)) {\
	PK_DBG("[CAMERA SENSOR] Fail to OFF analog power\n");\
	goto _kdCISModulePowerOn_exit_;\
	}

#define CIS_CAMERA_VIO_OFF(mode_name)\
	if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2,mode_name)){\
	PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");\
	goto _kdCISModulePowerOn_exit_;\
	}

#define CIS_MAIN_CAMERA_RST_OUTPUT(level) \
	if(mt_set_gpio_mode(GPIO_CAMERA_CMRST_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){\
	PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");\
	}\
	if(mt_set_gpio_dir(GPIO_CAMERA_CMRST_PIN,GPIO_DIR_OUT)){\
	PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");\
	}\
	if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,level)){\
	PK_DBG("[CAMERA LENS] set gpio failed!! \n");\
	}

#define CIS_MAIN_CAMERA_PDN_OUTPUT(level) \
	if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN_PIN,GPIO_CAMERA_CMPDN_PIN_M_GPIO)){\
	PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");\
	}\
	if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN_PIN,GPIO_DIR_OUT)){\
	PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");\
	}\
	if(mt_set_gpio_out(GPIO_CAMERA_CMPDN_PIN,level)){\
	PK_DBG("[CAMERA LENS] set gpio failed!! \n");\
	}

#define CIS_MAIN_CAMERA_LDO_OUTPUT(level) \
	if(mt_set_gpio_mode(GPIO_CAMERA_LDO_EN_PIN,GPIO_MODE_00)){\
	PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");\
	}\
	if(mt_set_gpio_dir(GPIO_CAMERA_LDO_EN_PIN,GPIO_DIR_OUT)){\
	PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");\
	}\
	if(mt_set_gpio_out(GPIO_CAMERA_LDO_EN_PIN,level)){\
	PK_DBG("[CAMERA LENS] set gpio failed!! \n");\
	}

#define CIS_SUB_CAMERA_RST_OUTPUT(level) \
	if(mt_set_gpio_mode(GPIO_CAMERA_CMRST1_PIN,GPIO_CAMERA_CMRST1_PIN_M_GPIO)){\
	PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");\
	}\
	if(mt_set_gpio_dir(GPIO_CAMERA_CMRST1_PIN,GPIO_DIR_OUT)){\
	PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");\
	}\
	if(mt_set_gpio_out(GPIO_CAMERA_CMRST1_PIN,level)){\
	PK_DBG("[CAMERA LENS] set gpio failed!! \n");\
	}

#define CIS_SUB_CAMERA_PDN_OUTPUT(level) \
	if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN1_PIN,GPIO_CAMERA_CMPDN1_PIN_M_GPIO)){\
	PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");\
	}\
	if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN1_PIN,GPIO_DIR_OUT)){\
	PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");\
	}\
	if(mt_set_gpio_out(GPIO_CAMERA_CMPDN1_PIN,level)){\
	PK_DBG("[CAMERA LENS] set gpio failed!! \n");\
	}


int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name)
{
	u32 pinSetIdx = 0;//default main sensor
	u32 pinSetIdxTmp = 0;

	printk("hujl@debug:SensorIdx = %d\n", SensorIdx);

	//power ON
	if (On) 
	{
		PK_DBG("kdCISModulePowerOn -on:currSensorName=%s;\n",currSensorName);
		if(currSensorName && (0 == strcmp(SENSOR_DRVNAME_HI351_YUV,currSensorName))&& (SensorIdx == DUAL_CAMERA_MAIN_SENSOR)) 
		{
			CIS_CAMERA_VIO_ON (VOL_1800 , mode_name);
			msleep(2);		
			CIS_CAMERA_AVDD_ON(VOL_2800 , mode_name);
			msleep(2);
			CIS_CAMERA_DVDD_ON(VOL_1200 , mode_name);
			msleep(5);

			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);		

			//power on to normal state
			CIS_MAIN_CAMERA_PDN_OUTPUT(GPIO_OUT_ONE);

			//reset
			mdelay(30);
			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ONE);
			mdelay(2);      

			//disable inactive sensor
			CIS_SUB_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);
			CIS_SUB_CAMERA_PDN_OUTPUT(GPIO_OUT_ONE);
		}
		else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_S5K5EAYX_MIPI_YUV,currSensorName))&& (SensorIdx == DUAL_CAMERA_MAIN_SENSOR))
		{
			//disable inactive sensor
			CIS_SUB_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);
			CIS_SUB_CAMERA_PDN_OUTPUT(GPIO_OUT_ONE);

			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);
			CIS_MAIN_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);

			//vdd-reg vio avdd can rise in any order
			CIS_MAIN_CAMERA_LDO_OUTPUT(GPIO_OUT_ONE);			
			CIS_CAMERA_VIO_ON (VOL_1800, mode_name);
			mdelay(1);						
			CIS_CAMERA_AVDD_ON(VOL_2800, mode_name);
			mdelay(1);

			CIS_MAIN_CAMERA_PDN_OUTPUT(GPIO_OUT_ONE);
			mdelay(1);			
			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ONE);
			mdelay(1);			

		}
		else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_OV5645_MIPI_YUV,currSensorName))&& (SensorIdx == DUAL_CAMERA_MAIN_SENSOR))
		{
			//disable inactive sensor
			CIS_SUB_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);
			CIS_SUB_CAMERA_PDN_OUTPUT(GPIO_OUT_ONE);			   
		
			CIS_MAIN_CAMERA_LDO_OUTPUT(GPIO_OUT_ZERO);
			CIS_CAMERA_VIO_ON (VOL_1800,mode_name);			
			CIS_CAMERA_AVDD_ON(VOL_2800,mode_name);
			CIS_CAMERA_DVDD_ON(VOL_1500,mode_name);
			mdelay(10);

			CIS_MAIN_CAMERA_PDN_OUTPUT(GPIO_OUT_ONE);			
			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ONE);			
			mdelay(2);			
			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);
			mdelay(20);
			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ONE);
			mdelay(10);			
		}		
		else if(currSensorName && (0 == strcmp(SENSOR_DRVNAME_HI257_YUV,currSensorName))&& (SensorIdx == DUAL_CAMERA_MAIN_SENSOR)) 
		{
			//disable sub sensor
			CIS_SUB_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);
			CIS_SUB_CAMERA_PDN_OUTPUT(GPIO_OUT_ONE);

			CIS_MAIN_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);
			//active main sensor		
			CIS_MAIN_CAMERA_PDN_OUTPUT(GPIO_OUT_ONE);		
			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);

			CIS_CAMERA_VIO_ON (VOL_1800,mode_name);		
			mdelay(2);			
			CIS_CAMERA_AVDD_ON(VOL_2800,mode_name);
			mdelay(2);			
			CIS_CAMERA_DVDD_ON(VOL_1800,mode_name);

			msleep(5);

			CIS_MAIN_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);
			mdelay(20);
			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ONE);			

		} 
		else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_SP2519_YUV,currSensorName))&& (SensorIdx == DUAL_CAMERA_MAIN_SENSOR)) 
		{
			//disable inactive sensor
			CIS_SUB_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);
			CIS_SUB_CAMERA_PDN_OUTPUT(GPIO_OUT_ONE);


			CIS_CAMERA_AVDD_ON(VOL_2800,mode_name);
			CIS_CAMERA_VIO_ON (VOL_1800,mode_name);
			CIS_CAMERA_DVDD_ON(VOL_1800,mode_name);

			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ONE);						
			CIS_MAIN_CAMERA_PDN_OUTPUT(GPIO_OUT_ONE);						
			mdelay(10);

			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);			
			mdelay(20);			
			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ONE);			
			mdelay(10);			

			CIS_MAIN_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);			
			mdelay(10);						

		} 
		else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_SP0A19_YUV,currSensorName))&& (SensorIdx == DUAL_CAMERA_SUB_SENSOR)) 
		{
			//disable inactive sensor
			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);
			msleep(3);
			CIS_MAIN_CAMERA_PDN_OUTPUT(GPIO_OUT_ONE);
			msleep(10);
			CIS_SUB_CAMERA_PDN_OUTPUT(GPIO_OUT_ONE);
			msleep(3);

			CIS_CAMERA_AVDD_ON(VOL_2800,mode_name);
			msleep(3);
			CIS_CAMERA_VIO_ON (VOL_1800,mode_name);

			CIS_SUB_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);
			mdelay(30);
			CIS_SUB_CAMERA_PDN_OUTPUT(GPIO_OUT_ONE);		
			mdelay(30);
			CIS_SUB_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);								
			mdelay(50);						
		}
		else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_HI708_YUV,currSensorName))&& (SensorIdx == DUAL_CAMERA_SUB_SENSOR)) 
		{
			//disable inactive sensor
			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);
			CIS_MAIN_CAMERA_PDN_OUTPUT(GPIO_OUT_ONE);			
		
			CIS_SUB_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);
			CIS_SUB_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);		
			CIS_CAMERA_AVDD_ON(VOL_2800,mode_name);
			CIS_CAMERA_VIO_ON (VOL_1800,mode_name);

			mdelay(1);
			CIS_SUB_CAMERA_PDN_OUTPUT(GPIO_OUT_ONE);		
			mdelay(1);			
			CIS_SUB_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);								
			msleep(6);
		}
	} 
	else 
	{//power OFF		
		PK_DBG("kdCISModulePowerOn -off:currSensorName=%s\n",currSensorName);
		if(currSensorName && (0 == strcmp(SENSOR_DRVNAME_HI351_YUV,currSensorName))&& (SensorIdx == DUAL_CAMERA_MAIN_SENSOR))
		{
			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);
			CIS_MAIN_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);			

			CIS_CAMERA_DVDD_OFF(mode_name);
			CIS_CAMERA_AVDD_OFF(mode_name);
			CIS_CAMERA_VIO_OFF(mode_name);
		} 
		else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_SP2519_YUV,currSensorName))&& (SensorIdx == DUAL_CAMERA_MAIN_SENSOR)) 
		{
			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);
			CIS_MAIN_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);			

			CIS_CAMERA_DVDD_OFF(mode_name);
			CIS_CAMERA_AVDD_OFF(mode_name);
			CIS_CAMERA_VIO_OFF(mode_name);
		} 
		else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_HI257_YUV,currSensorName))&& (SensorIdx == DUAL_CAMERA_MAIN_SENSOR)) 
		{
			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);
			CIS_MAIN_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);			

			CIS_CAMERA_DVDD_OFF(mode_name);
			CIS_CAMERA_AVDD_OFF(mode_name);
			CIS_CAMERA_VIO_OFF(mode_name);
			//disable inactive sensor
			CIS_SUB_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);
			CIS_SUB_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);

		} 
		else if(currSensorName && (0 == strcmp(SENSOR_DRVNAME_OV5645_MIPI_YUV,currSensorName))&& (SensorIdx == DUAL_CAMERA_MAIN_SENSOR))
		{
			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);
			CIS_MAIN_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);			

			CIS_CAMERA_DVDD_OFF(mode_name);
			CIS_CAMERA_AVDD_OFF(mode_name);
			CIS_CAMERA_VIO_OFF(mode_name);
		}
		else if(currSensorName && (0 == strcmp(SENSOR_DRVNAME_S5K5EAYX_MIPI_YUV,currSensorName))&& (SensorIdx == DUAL_CAMERA_MAIN_SENSOR))
		{
			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);
			CIS_MAIN_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);			

			CIS_MAIN_CAMERA_LDO_OUTPUT(GPIO_OUT_ZERO);		
			CIS_CAMERA_AVDD_OFF(mode_name);
			CIS_CAMERA_VIO_OFF(mode_name);

			CIS_SUB_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);			

		}
		else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_SP0A19_YUV, currSensorName))&& (SensorIdx == DUAL_CAMERA_SUB_SENSOR)) 
		{
			//disable inactive sensor
			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);
			msleep(6);
			CIS_MAIN_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);			

			CIS_CAMERA_AVDD_OFF(mode_name);
			CIS_CAMERA_VIO_OFF(mode_name);	
			msleep(6);
			CIS_SUB_CAMERA_PDN_OUTPUT(GPIO_OUT_ONE);
			mdelay(6);
			CIS_SUB_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);
			mdelay(6);
		}
		else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_HI708_YUV, currSensorName))&& (SensorIdx == DUAL_CAMERA_SUB_SENSOR)) 
		{
			CIS_SUB_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);			

			CIS_CAMERA_AVDD_OFF(mode_name);
			CIS_CAMERA_VIO_OFF(mode_name);
		} 		
		else 
		{
			CIS_MAIN_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);
			CIS_MAIN_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);			
			CIS_SUB_CAMERA_PDN_OUTPUT(GPIO_OUT_ZERO);
			CIS_SUB_CAMERA_RST_OUTPUT(GPIO_OUT_ZERO);			

			CIS_CAMERA_DVDD_OFF(mode_name);
			CIS_CAMERA_AVDD_OFF(mode_name);
			CIS_CAMERA_VIO_OFF(mode_name);
		}

		/*yi.zheng.hz add for camera power consumption problem start FR437080*/
		mt_set_gpio_dir(GPIO_CAMERA_CMPDN_PIN, GPIO_DIR_IN);
		mt_set_gpio_pull_enable(GPIO_CAMERA_CMPDN_PIN, GPIO_PULL_DISABLE);

	}

	return 0;

_kdCISModulePowerOn_exit_:
	return -EIO;
}


EXPORT_SYMBOL(kdCISModulePowerOn);

//!--
//
