//V1.0.1 20140225 修正效果 for 绿色偏暗问题,20140307修正倍频和曝光参数Kavie   
//V1.0.2 20140305 Kavie for AWB 偏蓝，偏红问题  ,(20140306修改饱和度+0x08) 此版本Yaris3.5项目暂未使用,仅做保留参////量产版本2
//V1.0.3 20140415 Kavie for  Driver p0,0x1c=0x17(0x1f),0x1f=0xd0(0xf0)
//V1.0.5 20140509 Kavie for p0,0x6c,0x7a,0x25,0x28 for 行噪，shading ,indoor&F-AWB  Yaris3.5 (备用)
//V1.1.0 20140521 Kavie for shading Yaris3.5      
//V1.1.0 20140522 Kavie for Yaris M
//V1.1.1 20140530 Kavie for  Yaris M     shading,AWB test01
//V1.1.2 20140612 Kavie for  Yaris M     shading,gamma,saturation,contrast,AWB,other.. 

//V1.2.0 20141225 Kavie for  Pixi3-4   SP2519  修正暗处较暗,重新修正代码结构等问题
//V1.2.1 20141226 Kavie for  Pixi3-4   SP2519 add ISO set

/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2005
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

/*****************************************************************************
*
* Filename:
* ---------
*   sp2519yuv_Sensor.c
*
* Project:
* --------
*   MAUI
*
* Description:
* ------------
*   Image sensor driver function
*   V1.0.0
*
* Author:
* -------
*   Mormo
*
*=============================================================
*             HISTORY
* Below this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
*------------------------------------------------------------------------------
* $Log$
* 2011/11/03 Firsty Released By Mormo;
*   
*
*------------------------------------------------------------------------------
* Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
*=============================================================
******************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"
#include "sp2519yuv_Sensor.h"
#include "sp2519yuv_Camera_Sensor_para.h"
#include "sp2519yuv_CameraCustomized.h"

/*******************************************************************************
* // Adapter for Winmo typedef
********************************************************************************/
#define WINMO_USE 0

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT

//auto lum
#define SP2519_NORMAL_Y0ffset  0x18//0x10
#define SP2519_LOWLIGHT_Y0ffset  0x25//0x20

int 	   flash_mode_sp2519 = 0;
kal_bool   SP2519_MPEG4_encode_mode = KAL_FALSE;
kal_bool   SP2519_CAM_BANDING_50HZ = KAL_TRUE; //KAL_FALSE;
kal_bool   SP2519_CAM_Nightmode = 0;
kal_bool   setshutter = KAL_FALSE;
kal_uint32 SP2519_isp_master_clock;
kal_uint16 SP2519_dummy_pixels = 0, SP2519_dummy_lines = 0;
static kal_uint32 SP2519_g_fPV_PCLK = 24;

kal_uint16 G_shutter;
kal_uint8 G_Gain;

kal_uint8 SP2519_sensor_write_I2C_address = SP2519_WRITE_ID;
kal_uint8 SP2519_sensor_read_I2C_address = SP2519_READ_ID;

UINT8 SP2519PixelClockDivider=0;

MSDK_SENSOR_CONFIG_STRUCT SP2519SensorConfigData;


#define PFX "[sp2519]:"

#define SP2519YUV_DEBUG

#ifdef SP2519YUV_DEBUG
#define SENSORDB(fmt, arg...)  printk(PFX "%s: " fmt, __FUNCTION__ ,##arg)

#else
#define SENSORDB(x,...)
#endif

#define WINDOW_SIZE_UXGA	0
#define WINDOW_SIZE_720P 	1
#define WINDOW_SIZE_SVGA 	2
#define WINDOW_SIZE_VGA	 	3

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

#if 0 //for T-card
#define SP2519_DEBUG_SENSOR  //T-card
#define SP2519_Capture_debug //T-card for Capture
#define SP2519_Preview_debug //T-card for Preview
#endif

//#define AE_change  //for AE change
//#define Preview_SVGA

#if 1
#define SP2519_24M_60M
#endif

kal_uint16 SP2519_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
	char puSendCmd[2] = {(char)(addr & 0xFF) , (char)(para & 0xFF)};

	iWriteRegI2C(puSendCmd , 2, SP2519_WRITE_ID);

}
kal_uint16 SP2519_read_cmos_sensor(kal_uint8 addr)
{
	kal_uint16 get_byte=0;
	char puSendCmd = { (char)(addr & 0xFF) };
	iReadRegI2C(&puSendCmd , 1, (u8*)&get_byte, 1, SP2519_WRITE_ID);

	return get_byte;
}



#ifdef SP2519_DEBUG_SENSOR
static kal_uint8 SP2519_fromsd = 0;
kal_uint16 SP2519_write_cmos_sensor(kal_uint8 addr, kal_uint8 para);
#define SP2519_OP_CODE_INI		0x00		/* Initial value. */
#define SP2519_OP_CODE_REG		0x01		/* Register */
#define SP2519_OP_CODE_DLY		0x02		/* Delay */
#define SP2519_OP_CODE_END		0x03		/* End of initial setting. */

typedef struct
{
	u16 init_reg;
	u16 init_val;	/* Save the register value and delay tick */
	u8 op_code;		/* 0 - Initial value, 1 - Register, 2 - Delay, 3 - End of setting. */
} SP2519_initial_set_struct;

SP2519_initial_set_struct SP2519_Init_Reg[1000];
SP2519_initial_set_struct SP2519_Preview_Reg[200];
SP2519_initial_set_struct SP2519_Capture_Reg[200];

static u32 SP2519_strtol(const char *nptr, u8 base)
{
	u8 ret;
	if(!nptr || (base!=16 && base!=10 && base!=8))
	{
		printk("%s(): NULL pointer input\n", __FUNCTION__);
		return -1;
	}
	for(ret=0; *nptr; nptr++)
	{
		if((base==16 && *nptr>='A' && *nptr<='F') || 
			(base==16 && *nptr>='a' && *nptr<='f') || 
			(base>=10 && *nptr>='0' && *nptr<='9') ||
			(base>=8 && *nptr>='0' && *nptr<='7') )
		{
			ret *= base;
			if(base==16 && *nptr>='A' && *nptr<='F')
				ret += *nptr-'A'+10;
			else if(base==16 && *nptr>='a' && *nptr<='f')
				ret += *nptr-'a'+10;
			else if(base>=10 && *nptr>='0' && *nptr<='9')
				ret += *nptr-'0';
			else if(base>=8 && *nptr>='0' && *nptr<='7')
				ret += *nptr-'0';
		}
		else
			return ret;
	}
	return ret;
}

u8 SP2519_Initialize_from_T_Flash()
{
	//FS_HANDLE fp = -1;				/* Default, no file opened. */
	//u8 *data_buff = NULL;
	u8 *curr_ptr = NULL;
	u32 file_size = 0;
	//u32 bytes_read = 0;
	u32 i = 0, j = 0;
	u8 func_ind[4] = {0};	/* REG or DLY */


	struct file *fp; 
	mm_segment_t fs; 
	loff_t pos = 0; 
	static u8 data_buff[10*1024] ;

	fp = filp_open("/mnt/sdcard/SP2519_sd", O_RDONLY , 0); 
	if (IS_ERR(fp)) { 
		printk("create file error\n"); 
		return -1; 
	} 
	fs = get_fs(); 
	set_fs(KERNEL_DS); 

	file_size = vfs_llseek(fp, 0, SEEK_END);
	vfs_read(fp, data_buff, file_size, &pos); 
	//printk("%s %d %d\n", buf,iFileLen,pos); 
	filp_close(fp, NULL); 
	set_fs(fs);



	printk("1\n");

	/* Start parse the setting witch read from t-flash. */
	curr_ptr = data_buff;
	while (curr_ptr < (data_buff + file_size))
	{
		while ((*curr_ptr == ' ') || (*curr_ptr == '\t'))/* Skip the Space & TAB */
			curr_ptr++;				

		if (((*curr_ptr) == '/') && ((*(curr_ptr + 1)) == '*'))
		{
			while (!(((*curr_ptr) == '*') && ((*(curr_ptr + 1)) == '/')))
			{
				curr_ptr++;		/* Skip block comment code. */
			}

			while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
			{
				curr_ptr++;
			}

			curr_ptr += 2;						/* Skip the enter line */

			continue ;
		}

		if (((*curr_ptr) == '/') || ((*curr_ptr) == '{') || ((*curr_ptr) == '}'))		/* Comment line, skip it. */
		{
			while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
			{
				curr_ptr++;
			}

			curr_ptr += 2;						/* Skip the enter line */

			continue ;
		}
		/* This just content one enter line. */
		if (((*curr_ptr) == 0x0D) && ((*(curr_ptr + 1)) == 0x0A))
		{
			curr_ptr += 2;
			continue ;
		}
		//printk(" curr_ptr1 = %s\n",curr_ptr);
		memcpy(func_ind, curr_ptr, 3);


		if (strcmp((const char *)func_ind, "REG") == 0)		/* REG */
		{
			curr_ptr += 6;				/* Skip "REG(0x" or "DLY(" */
			SP2519_Init_Reg[i].op_code = SP2519_OP_CODE_REG;

			SP2519_Init_Reg[i].init_reg = SP2519_strtol((const char *)curr_ptr, 16);
			curr_ptr += 5;	/* Skip "00, 0x" */

			SP2519_Init_Reg[i].init_val = SP2519_strtol((const char *)curr_ptr, 16);
			curr_ptr += 4;	/* Skip "00);" */

		}
		else									/* DLY */
		{
			/* Need add delay for this setting. */
			curr_ptr += 4;	
			SP2519_Init_Reg[i].op_code = SP2519_OP_CODE_DLY;

			SP2519_Init_Reg[i].init_reg = 0xFF;
			SP2519_Init_Reg[i].init_val = SP2519_strtol((const char *)curr_ptr,  10);	/* Get the delay ticks, the delay should less then 50 */
		}
		i++;


		/* Skip to next line directly. */
		while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
		{
			curr_ptr++;
		}
		curr_ptr += 2;

	}
	printk("2\n");
	/* (0xFFFF, 0xFFFF) means the end of initial setting. */
	SP2519_Init_Reg[i].op_code = SP2519_OP_CODE_END;
	SP2519_Init_Reg[i].init_reg = 0xFF;
	SP2519_Init_Reg[i].init_val = 0xFF;
	i++;
	//for (j=0; j<i; j++)
	//printk(" %x  ==  %x\n",SP2519_Init_Reg[j].init_reg, SP2519_Init_Reg[j].init_val);

	/* Start apply the initial setting to sensor. */
#if 1
	for (j=0; j<i; j++)
	{
		if (SP2519_Init_Reg[j].op_code == SP2519_OP_CODE_END)	/* End of the setting. */
		{
			break ;
		}
		else if (SP2519_Init_Reg[j].op_code == SP2519_OP_CODE_DLY)
		{
			msleep(SP2519_Init_Reg[j].init_val);		/* Delay */
		}
		else if (SP2519_Init_Reg[j].op_code == SP2519_OP_CODE_REG)
		{

			SP2519_write_cmos_sensor((kal_uint8)SP2519_Init_Reg[j].init_reg, (kal_uint8)SP2519_Init_Reg[j].init_val);
		}
		else
		{
			printk("REG ERROR!\n");
		}
	}
#endif
	return 1;	
}



#ifdef SP2519_Preview_debug
u8 SP2519_Preview_from_T_Flash()
{
	//FS_HANDLE fp = -1;				/* Default, no file opened. */
	//u8 *data_buff = NULL;
	u8 *curr_ptr = NULL;
	u32 file_size = 0;
	//u32 bytes_read = 0;
	u32 i = 0, j = 0;
	u8 func_ind[4] = {0};	/* REG or DLY */


	struct file *fp; 
	mm_segment_t fs; 
	loff_t pos = 0; 
	static u8 data_buff[10*1024] ;

	fp = filp_open("/mnt/sdcard/sp2519_sd_preview", O_RDONLY , 0); 
	if (IS_ERR(fp)) { 
		printk("create file error\n"); 
		return -1; 
	} 
	fs = get_fs(); 
	set_fs(KERNEL_DS); 

	file_size = vfs_llseek(fp, 0, SEEK_END);
	vfs_read(fp, data_buff, file_size, &pos); 
	//printk("%s %d %d\n", buf,iFileLen,pos); 
	filp_close(fp, NULL); 
	set_fs(fs);

	/* Start parse the setting witch read from t-flash. */
	curr_ptr = data_buff;
	while (curr_ptr < (data_buff + file_size))
	{
		while ((*curr_ptr == ' ') || (*curr_ptr == '\t'))/* Skip the Space & TAB */
			curr_ptr++;				

		if (((*curr_ptr) == '/') && ((*(curr_ptr + 1)) == '*'))
		{
			while (!(((*curr_ptr) == '*') && ((*(curr_ptr + 1)) == '/')))
			{
				curr_ptr++;		/* Skip block comment code. */
			}

			while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
			{
				curr_ptr++;
			}

			curr_ptr += 2;						/* Skip the enter line */

			continue ;
		}

		if (((*curr_ptr) == '/') || ((*curr_ptr) == '{') || ((*curr_ptr) == '}'))		/* Comment line, skip it. */
		{
			while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
			{
				curr_ptr++;
			}

			curr_ptr += 2;						/* Skip the enter line */

			continue ;
		}
		/* This just content one enter line. */
		if (((*curr_ptr) == 0x0D) && ((*(curr_ptr + 1)) == 0x0A))
		{
			curr_ptr += 2;
			continue ;
		}
		//printk(" curr_ptr1 = %s\n",curr_ptr);
		memcpy(func_ind, curr_ptr, 3);


		if (strcmp((const char *)func_ind, "REG") == 0)		/* REG */
		{
			curr_ptr += 6;				/* Skip "REG(0x" or "DLY(" */
			SP2519_Preview_Reg[i].op_code = SP2519_OP_CODE_REG;

			SP2519_Preview_Reg[i].init_reg = strtol((const char *)curr_ptr, 16);
			curr_ptr += 5;	/* Skip "00, 0x" */

			SP2519_Preview_Reg[i].init_val = strtol((const char *)curr_ptr, 16);
			curr_ptr += 4;	/* Skip "00);" */

		}
		else									/* DLY */
		{
			/* Need add delay for this setting. */
			curr_ptr += 4;	
			SP2519_Preview_Reg[i].op_code = SP2519_OP_CODE_DLY;

			SP2519_Preview_Reg[i].init_reg = 0xFF;
			SP2519_Preview_Reg[i].init_val = strtol((const char *)curr_ptr,  10);	/* Get the delay ticks, the delay should less then 50 */
		}
		i++;


		/* Skip to next line directly. */
		while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
		{
			curr_ptr++;
		}
		curr_ptr += 2;
	}

	/* (0xFFFF, 0xFFFF) means the end of initial setting. */
	SP2519_Preview_Reg[i].op_code = SP2519_OP_CODE_END;
	SP2519_Preview_Reg[i].init_reg = 0xFF;
	SP2519_Preview_Reg[i].init_val = 0xFF;
	i++;
	//for (j=0; j<i; j++)
	//printk(" %x  ==  %x\n",SP2519_Init_Reg[j].init_reg, SP2519_Init_Reg[j].init_val);

	/* Start apply the initial setting to sensor. */
#if 1
	for (j=0; j<i; j++)
	{
		if (SP2519_Preview_Reg[j].op_code == SP2519_OP_CODE_END)	/* End of the setting. */
		{
			break ;
		}
		else if (SP2519_Preview_Reg[j].op_code == SP2519_OP_CODE_DLY)
		{
			msleep(SP2519_Preview_Reg[j].init_val);		/* Delay */
		}
		else if (SP2519_Preview_Reg[j].op_code == SP2519_OP_CODE_REG)
		{

			SP2519_write_cmos_sensor(SP2519_Preview_Reg[j].init_reg, SP2519_Preview_Reg[j].init_val);
		}
		else
		{
			printk("REG ERROR!\n");
		}
	}
#endif
	return 1;	
}
#endif



#ifdef SP2519_Capture_debug
u8 SP2519_Capture_from_T_Flash()
{
	//FS_HANDLE fp = -1;				/* Default, no file opened. */
	//u8 *data_buff = NULL;
	u8 *curr_ptr = NULL;
	u32 file_size = 0;
	//u32 bytes_read = 0;
	u32 i = 0, j = 0;
	u8 func_ind[4] = {0};	/* REG or DLY */


	struct file *fp; 
	mm_segment_t fs; 
	loff_t pos = 0; 
	static u8 data_buff[10*1024] ;

	fp = filp_open("/mnt/sdcard/sp2519_sd_capture", O_RDONLY , 0); 
	if (IS_ERR(fp)) { 
		printk("create file error\n"); 
		return -1; 
	} 
	fs = get_fs(); 
	set_fs(KERNEL_DS); 

	file_size = vfs_llseek(fp, 0, SEEK_END);
	vfs_read(fp, data_buff, file_size, &pos); 
	//printk("%s %d %d\n", buf,iFileLen,pos); 
	filp_close(fp, NULL); 
	set_fs(fs);


	/* Start parse the setting witch read from t-flash. */
	curr_ptr = data_buff;
	while (curr_ptr < (data_buff + file_size))
	{
		while ((*curr_ptr == ' ') || (*curr_ptr == '\t'))/* Skip the Space & TAB */
			curr_ptr++;				

		if (((*curr_ptr) == '/') && ((*(curr_ptr + 1)) == '*'))
		{
			while (!(((*curr_ptr) == '*') && ((*(curr_ptr + 1)) == '/')))
			{
				curr_ptr++;		/* Skip block comment code. */
			}

			while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
			{
				curr_ptr++;
			}

			curr_ptr += 2;						/* Skip the enter line */

			continue ;
		}

		if (((*curr_ptr) == '/') || ((*curr_ptr) == '{') || ((*curr_ptr) == '}'))		/* Comment line, skip it. */
		{
			while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
			{
				curr_ptr++;
			}

			curr_ptr += 2;						/* Skip the enter line */

			continue ;
		}
		/* This just content one enter line. */
		if (((*curr_ptr) == 0x0D) && ((*(curr_ptr + 1)) == 0x0A))
		{
			curr_ptr += 2;
			continue ;
		}
		//printk(" curr_ptr1 = %s\n",curr_ptr);
		memcpy(func_ind, curr_ptr, 3);


		if (strcmp((const char *)func_ind, "REG") == 0)		/* REG */
		{
			curr_ptr += 6;				/* Skip "REG(0x" or "DLY(" */
			SP2519_Capture_Reg[i].op_code = SP2519_OP_CODE_REG;

			SP2519_Capture_Reg[i].init_reg = strtol((const char *)curr_ptr, 16);
			curr_ptr += 5;	/* Skip "00, 0x" */

			SP2519_Capture_Reg[i].init_val = strtol((const char *)curr_ptr, 16);
			curr_ptr += 4;	/* Skip "00);" */

		}
		else									/* DLY */
		{
			/* Need add delay for this setting. */
			curr_ptr += 4;	
			SP2519_Capture_Reg[i].op_code = SP2519_OP_CODE_DLY;

			SP2519_Capture_Reg[i].init_reg = 0xFF;
			SP2519_Capture_Reg[i].init_val = strtol((const char *)curr_ptr,  10);	/* Get the delay ticks, the delay should less then 50 */
		}
		i++;


		/* Skip to next line directly. */
		while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
		{
			curr_ptr++;
		}
		curr_ptr += 2;
	}

	/* (0xFFFF, 0xFFFF) means the end of initial setting. */
	SP2519_Capture_Reg[i].op_code = SP2519_OP_CODE_END;
	SP2519_Capture_Reg[i].init_reg = 0xFF;
	SP2519_Capture_Reg[i].init_val = 0xFF;
	i++;
	//for (j=0; j<i; j++)
	//printk(" %x  ==  %x\n",SP2519_Init_Reg[j].init_reg, SP2519_Init_Reg[j].init_val);

	/* Start apply the initial setting to sensor. */
#if 1
	for (j=0; j<i; j++)
	{
		if (SP2519_Capture_Reg[j].op_code == SP2519_OP_CODE_END)	/* End of the setting. */
		{
			break ;
		}
		else if (SP2519_Capture_Reg[j].op_code == SP2519_OP_CODE_DLY)
		{
			msleep(SP2519_Capture_Reg[j].init_val);		/* Delay */
		}
		else if (SP2519_Capture_Reg[j].op_code == SP2519_OP_CODE_REG)
		{

			SP2519_write_cmos_sensor(SP2519_Capture_Reg[j].init_reg, SP2519_Capture_Reg[j].init_val);
		}
		else
		{
			printk("REG ERROR!\n");
		}
	}
#endif
	return 1;	
}

#endif
#endif


/*************************************************************************
* FUNCTION
*	SP2519_SetShutter
*
* DESCRIPTION
*	This function set e-shutter of SP2519 to change exposure time.
*
* PARAMETERS
*   iShutter : exposured lines
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void SP2519_Set_Shutter(kal_uint16 iShutter)
{
	kal_uint8 temp_reg_L, temp_reg_H;
	temp_reg_L = iShutter & 0xff;
	temp_reg_H = (iShutter >>8) & 0xff;
	SP2519_write_cmos_sensor(0xfd,0x00); 
	SP2519_write_cmos_sensor(0x03,temp_reg_H);
	SP2519_write_cmos_sensor(0x04,temp_reg_L);
	SENSORDB(" SP2519_Set_Shutter\r\n");
} /* Set_SP2519_Shutter */


/*************************************************************************
* FUNCTION
*	SP2519_read_Shutter
*
* DESCRIPTION
*	This function read e-shutter of SP2519 .
*
* PARAMETERS
*  None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_uint16 SP2519_Read_Shutter(void)
{
	kal_uint8 temp_reg_L, temp_reg_H;
	kal_uint16 shutter;
	SP2519_write_cmos_sensor(0xfd,0x00); 
	temp_reg_L = SP2519_read_cmos_sensor(0x04);
	temp_reg_H = SP2519_read_cmos_sensor(0x03);

	shutter = (temp_reg_L & 0xFF) | (temp_reg_H << 8);
	SENSORDB(" SP2519_Read_Shutter %x \r\n",shutter);
	return shutter;
} /* SP2519_read_shutter */


/*************************************************************************
* FUNCTION
*	SP2519_ae_enable
*
* DESCRIPTION
*	This function enable or disable the awb (Auto White Balance).
*
* PARAMETERS
*	1. kal_bool : KAL_TRUE - enable awb, KAL_FALSE - disable awb.
*
* RETURNS
*	kal_bool : It means set awb right or not.
*
*************************************************************************/
static void SP2519_ae_enable(kal_bool enable)
{	

	kal_uint16 temp_AE_reg = 0;
	SENSORDB(" SP2519_ae_enable\r\n");

	SP2519_write_cmos_sensor(0xfd,0x01);
	temp_AE_reg = SP2519_read_cmos_sensor(0x32);
	if (enable == 1)
	{
		SP2519_write_cmos_sensor(0x32, (temp_AE_reg |0x05));
	}
	else if(enable == 0)
	{
		SP2519_write_cmos_sensor(0x32, (temp_AE_reg & (~0x05)));
	}
	SP2519_write_cmos_sensor(0xfd,0x00);

}

/*************************************************************************
* FUNCTION
*	SP2519_awb_enable
*
* DESCRIPTION
*	This function enable or disable the awb (Auto White Balance).
*
* PARAMETERS
*	1. kal_bool : KAL_TRUE - enable awb, KAL_FALSE - disable awb.
*
* RETURNS
*	kal_bool : It means set awb right or not.
*
*************************************************************************/
static void SP2519_awb_enable(kal_bool enable)
{	 

	kal_uint16 temp_AWB_reg = 0;
	SENSORDB(" SP2519_ae_enable\r\n");
	SP2519_write_cmos_sensor(0xfd,0x01);
	temp_AWB_reg = SP2519_read_cmos_sensor(0x32);
	if (enable == 1)
	{
		SP2519_write_cmos_sensor(0x32, (temp_AWB_reg |0x10));
	}
	else if(enable == 0)
	{
		SP2519_write_cmos_sensor(0x32, (temp_AWB_reg & (~0x10)));
	}
	SP2519_write_cmos_sensor(0xfd,0x00);

}


/*************************************************************************
* FUNCTION
*	SP2519_set_hb_shutter
*
* DESCRIPTION
*	This function set the dummy pixels(Horizontal Blanking) when capturing, it can be
*	used to adjust the frame rate  for back-end process.
*	
*	IMPORTANT NOTICE: the base shutter need re-calculate for some sensor, or else flicker may occur.
*
* PARAMETERS
*	1. kal_uint32 : Dummy Pixels (Horizontal Blanking)
*	2. kal_uint32 : shutter (Vertical Blanking)
*
* RETURNS
*	None
*
*************************************************************************/
static void SP2519_set_hb_shutter(kal_uint32 hb_add,  kal_uint32 shutter)
{
	kal_uint32 hb_ori, hb_total;
	kal_uint32 temp_reg, banding_step;
	SENSORDB(" SP2519_set_hb_shutter\r\n");
}    /* SP2519_set_dummy */

/*************************************************************************
* FUNCTION
*	SP2519_config_window
*
* DESCRIPTION
*	This function config the hardware window of SP2519 for getting specified
*  data of that window.
*
* PARAMETERS
*	start_x : start column of the interested window
*  start_y : start row of the interested window
*  width  : column widht of the itnerested window
*  height : row depth of the itnerested window
*
* RETURNS
*	the data that read from SP2519
*
* GLOBALS AFFECTED
*
*************************************************************************/
//void SP2519_config_window(kal_uint16 startx, kal_uint16 starty, kal_uint16 width, kal_uint16 height)//Ronlus
void SP2519_config_window(kal_uint8 index)
{
	SENSORDB(" SP2519_config_window,index = %d\r\n",index);
	switch(index)
	{
	case WINDOW_SIZE_UXGA:
		//window   //for uxga  20141224 Kavie	
		SP2519_write_cmos_sensor(0xfd, 0x01);
		SP2519_write_cmos_sensor(0x4a, 0x00);
		SP2519_write_cmos_sensor(0x4b, 0x04);
		SP2519_write_cmos_sensor(0x4c, 0xb0);
		SP2519_write_cmos_sensor(0x4d, 0x00);
		SP2519_write_cmos_sensor(0x4e, 0x06);
		SP2519_write_cmos_sensor(0x4f, 0x40);
		//close resize	
		SP2519_write_cmos_sensor(0xfd, 0x02);
		SP2519_write_cmos_sensor(0x40, 0x00);
		SP2519_write_cmos_sensor(0x41, 0x20);
		SP2519_write_cmos_sensor(0x42, 0x00);
		SP2519_write_cmos_sensor(0x43, 0x20);
		SP2519_write_cmos_sensor(0x44, 0x04);
		SP2519_write_cmos_sensor(0x45, 0xb0);
		SP2519_write_cmos_sensor(0x46, 0x06);
		SP2519_write_cmos_sensor(0x47, 0x40);
		SP2519_write_cmos_sensor(0x48, 0x00);

		SP2519_write_cmos_sensor(0xfd, 0x02);
		SP2519_write_cmos_sensor(0x0f, 0x00);
		SP2519_write_cmos_sensor(0x8f, 0x03);
		SP2519_write_cmos_sensor(0xfd, 0x00);	
		break;
	case WINDOW_SIZE_720P:
#ifdef AE_change
		SP2519_write_cmos_sensor(0xfd,0x00);
#endif
		break;
	case WINDOW_SIZE_SVGA:
		//window   //for uxga  20141224 Kavie	
		SP2519_write_cmos_sensor(0xfd, 0x01);
		SP2519_write_cmos_sensor(0x4a, 0x00);
		SP2519_write_cmos_sensor(0x4b, 0x04);
		SP2519_write_cmos_sensor(0x4c, 0xb0);
		SP2519_write_cmos_sensor(0x4d, 0x00);
		SP2519_write_cmos_sensor(0x4e, 0x06);
		SP2519_write_cmos_sensor(0x4f, 0x40);
		//open resize	
		SP2519_write_cmos_sensor(0xfd,0x02);
		SP2519_write_cmos_sensor(0x40,0x00);
		SP2519_write_cmos_sensor(0x41,0x40);
		SP2519_write_cmos_sensor(0x42,0x00);
		SP2519_write_cmos_sensor(0x43,0x40);
		SP2519_write_cmos_sensor(0x44,0x02);
		SP2519_write_cmos_sensor(0x45,0x58);
		SP2519_write_cmos_sensor(0x46,0x03);
		SP2519_write_cmos_sensor(0x47,0x20);

		SP2519_write_cmos_sensor(0xfd,0x02);			
		SP2519_write_cmos_sensor(0x0f,0x01);
		SP2519_write_cmos_sensor(0x8f,0x02);
		SP2519_write_cmos_sensor(0xfd,0x00);
		break;
	case WINDOW_SIZE_VGA:
		//window   //for uxga  20141224 Kavie	
		SP2519_write_cmos_sensor(0xfd, 0x01);
		SP2519_write_cmos_sensor(0x4a, 0x00);
		SP2519_write_cmos_sensor(0x4b, 0x04);
		SP2519_write_cmos_sensor(0x4c, 0xb0);
		SP2519_write_cmos_sensor(0x4d, 0x00);
		SP2519_write_cmos_sensor(0x4e, 0x06);
		SP2519_write_cmos_sensor(0x4f, 0x40);
		//open resize	
		SP2519_write_cmos_sensor(0xfd,0x02);
		SP2519_write_cmos_sensor(0x40,0x00);
		SP2519_write_cmos_sensor(0x41,0x50);
		SP2519_write_cmos_sensor(0x42,0x00);
		SP2519_write_cmos_sensor(0x43,0x50);
		SP2519_write_cmos_sensor(0x44,0x01);
		SP2519_write_cmos_sensor(0x45,0xe0);
		SP2519_write_cmos_sensor(0x46,0x02);
		SP2519_write_cmos_sensor(0x47,0x80);

		SP2519_write_cmos_sensor(0x0f,0x01);
		SP2519_write_cmos_sensor(0x8f,0x02);
		SP2519_write_cmos_sensor(0xfd,0x00);
		break;
	default:
		break;
	}
} /* SP2519_config_window */



/*************************************************************************
* FUNCTION
*	SP2519_SetGain
*
* DESCRIPTION
*	This function is to set global gain to sensor.
*
* PARAMETERS
*   iGain : sensor global gain(base: 0x40)
*
* RETURNS
*	the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_uint16 SP2519_SetGain(kal_uint16 iGain)
{
	SENSORDB(" SP2519_SetGain\r\n");
	return iGain;
}

#define SP2519_FLASH_BV_THRESHOLD 0x60
void SP2519YUV_FlashTriggerCheck(unsigned int *pFeatureReturnPara32)
{
	unsigned int NormBr;

	SP2519_write_cmos_sensor(0xfd,0x01);
	NormBr = SP2519_read_cmos_sensor(0xf1);
	SENSORDB("%s flashlight value %d\n", __func__, NormBr);

	if (NormBr > SP2519_FLASH_BV_THRESHOLD)
	{
		*pFeatureReturnPara32 = 0;
		return;
	}
	*pFeatureReturnPara32 = 1;

	return;
}

/*************************************************************************
* FUNCTION
*	SP2519_NightMode
*
* DESCRIPTION
*	This function night mode of SP2519.
*
* PARAMETERS
*	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void SP2519_night_mode(kal_bool bEnable)
{
	//kal_uint16 night = SP2519_read_cmos_sensor(0x20);
	SENSORDB(" SP2519_night_mode\r\n");
	//sensorlist.cpp kd_imagesensor.h add related files  
	if (bEnable)/*night mode settings*/
	{
		SENSORDB(" night mode\r\n");
		SP2519_CAM_Nightmode = 1;
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0xcd,SP2519_LOWLIGHT_Y0ffset);
		SP2519_write_cmos_sensor(0xce,0x18);
		if(SP2519_MPEG4_encode_mode == KAL_TRUE) /*video night mode*/
		{
			SENSORDB(" video night mode\r\n");

			if(SP2519_CAM_BANDING_50HZ == KAL_TRUE)/*video night mode 50hz*/
			{	
#ifdef SP2519_24M_60M
				//24M fix 8fps 2.5pll 50hz
				SP2519_write_cmos_sensor(0xfd,0x00);
				SP2519_write_cmos_sensor(0x03,0x02);
				SP2519_write_cmos_sensor(0x04,0x52);
				SP2519_write_cmos_sensor(0x05,0x00);
				SP2519_write_cmos_sensor(0x06,0x00);
				SP2519_write_cmos_sensor(0x07,0x00);
				SP2519_write_cmos_sensor(0x08,0x00);
				SP2519_write_cmos_sensor(0x09,0x02);
				SP2519_write_cmos_sensor(0x0a,0x51);
				SP2519_write_cmos_sensor(0xfd,0x01);
				SP2519_write_cmos_sensor(0xf0,0x00);
				SP2519_write_cmos_sensor(0xf7,0x63);
				SP2519_write_cmos_sensor(0xf8,0x53);
				SP2519_write_cmos_sensor(0x02,0x0c);
				SP2519_write_cmos_sensor(0x03,0x01);
				SP2519_write_cmos_sensor(0x06,0x63);
				SP2519_write_cmos_sensor(0x07,0x00);
				SP2519_write_cmos_sensor(0x08,0x01);
				SP2519_write_cmos_sensor(0x09,0x00);
				SP2519_write_cmos_sensor(0xfd,0x02);
				SP2519_write_cmos_sensor(0x3d,0x0f);
				SP2519_write_cmos_sensor(0x3e,0x53);
				SP2519_write_cmos_sensor(0x3f,0x00);
				SP2519_write_cmos_sensor(0x88,0x2b);
				SP2519_write_cmos_sensor(0x89,0x2b);
				SP2519_write_cmos_sensor(0x8a,0x65);
				SP2519_write_cmos_sensor(0xfd,0x02);
				SP2519_write_cmos_sensor(0xbe,0xa4);
				SP2519_write_cmos_sensor(0xbf,0x04);
				SP2519_write_cmos_sensor(0xd0,0xa4);
				SP2519_write_cmos_sensor(0xd1,0x04);
				SP2519_write_cmos_sensor(0xc9,0xa4);
				SP2519_write_cmos_sensor(0xca,0x04);

#endif
				SENSORDB("video night mode 50hz\r\n");
			}
			else/*video night mode 60hz*/
			{ 
				SENSORDB(" video night mode 60hz\r\n");                                                   
#ifdef SP2519_24M_60M
				//24M fix 8fps 2.5pll 60hz 
				SP2519_write_cmos_sensor(0xfd,0x00);
				SP2519_write_cmos_sensor(0x03,0x01);
				SP2519_write_cmos_sensor(0x04,0xf2);
				SP2519_write_cmos_sensor(0x05,0x00);
				SP2519_write_cmos_sensor(0x06,0x00);
				SP2519_write_cmos_sensor(0x07,0x00);
				SP2519_write_cmos_sensor(0x08,0x00);
				SP2519_write_cmos_sensor(0x09,0x02);
				SP2519_write_cmos_sensor(0x0a,0x48);
				SP2519_write_cmos_sensor(0xfd,0x01);
				SP2519_write_cmos_sensor(0xf0,0x00);
				SP2519_write_cmos_sensor(0xf7,0x53);
				SP2519_write_cmos_sensor(0xf8,0x53);
				SP2519_write_cmos_sensor(0x02,0x0f);
				SP2519_write_cmos_sensor(0x03,0x01);
				SP2519_write_cmos_sensor(0x06,0x53);
				SP2519_write_cmos_sensor(0x07,0x00);
				SP2519_write_cmos_sensor(0x08,0x01);
				SP2519_write_cmos_sensor(0x09,0x00);
				SP2519_write_cmos_sensor(0xfd,0x02);
				SP2519_write_cmos_sensor(0x3d,0x0f);
				SP2519_write_cmos_sensor(0x3e,0x53);
				SP2519_write_cmos_sensor(0x3f,0x00);
				SP2519_write_cmos_sensor(0x88,0x2b);
				SP2519_write_cmos_sensor(0x89,0x2b);
				SP2519_write_cmos_sensor(0x8a,0x66);
				SP2519_write_cmos_sensor(0xfd,0x02);
				SP2519_write_cmos_sensor(0xbe,0xdd);
				SP2519_write_cmos_sensor(0xbf,0x04);
				SP2519_write_cmos_sensor(0xd0,0xdd);
				SP2519_write_cmos_sensor(0xd1,0x04);
				SP2519_write_cmos_sensor(0xc9,0xdd);
				SP2519_write_cmos_sensor(0xca,0x04);
#endif
			}
		}/*capture night mode*/
		else 
		{   
			SENSORDB(" capture night mode\r\n");
			if(SP2519_CAM_BANDING_50HZ == KAL_TRUE)/*capture night mode 50hz*/
			{	
				SENSORDB(" capture night mode 50hz\r\n");//3                            
#ifdef SP2519_24M_60M
				//24M 6-11.2fps 50HZ 2.5pll
				SP2519_write_cmos_sensor(0xfd,0x00);
				SP2519_write_cmos_sensor(0x03,0x03);
				SP2519_write_cmos_sensor(0x04,0x42);
				SP2519_write_cmos_sensor(0x05,0x00);
				SP2519_write_cmos_sensor(0x06,0x00);
				SP2519_write_cmos_sensor(0x07,0x00);
				SP2519_write_cmos_sensor(0x08,0x00);
				SP2519_write_cmos_sensor(0x09,0x00);
				SP2519_write_cmos_sensor(0x0a,0x9d);
				SP2519_write_cmos_sensor(0xfd,0x01);
				SP2519_write_cmos_sensor(0xf0,0x00);
				SP2519_write_cmos_sensor(0xf7,0x8b);
				SP2519_write_cmos_sensor(0xf8,0x74);
				SP2519_write_cmos_sensor(0x02,0x10);
				SP2519_write_cmos_sensor(0x03,0x01);
				SP2519_write_cmos_sensor(0x06,0x8b);
				SP2519_write_cmos_sensor(0x07,0x00);
				SP2519_write_cmos_sensor(0x08,0x01);
				SP2519_write_cmos_sensor(0x09,0x00);
				SP2519_write_cmos_sensor(0xfd,0x02);
				SP2519_write_cmos_sensor(0x3d,0x14);
				SP2519_write_cmos_sensor(0x3e,0x74);
				SP2519_write_cmos_sensor(0x3f,0x00);
				SP2519_write_cmos_sensor(0x88,0xae);
				SP2519_write_cmos_sensor(0x89,0x69);
				SP2519_write_cmos_sensor(0x8a,0x43);
				SP2519_write_cmos_sensor(0xfd,0x02);
				SP2519_write_cmos_sensor(0xbe,0xb0);
				SP2519_write_cmos_sensor(0xbf,0x08);
				SP2519_write_cmos_sensor(0xd0,0xb0);
				SP2519_write_cmos_sensor(0xd1,0x08);
				SP2519_write_cmos_sensor(0xc9,0xb0);
				SP2519_write_cmos_sensor(0xca,0x08);
#endif
			}
			else/*capture night mode 60hz*/
			{ 
				SENSORDB(" capture night mode 60hz\r\n");  
#ifdef SP2519_24M_60M
				//24M 6-11.2fps 2.5pll 60hz
				SP2519_write_cmos_sensor(0xfd,0x00);
				SP2519_write_cmos_sensor(0x03,0x02);
				SP2519_write_cmos_sensor(0x04,0xb8);
				SP2519_write_cmos_sensor(0x05,0x00);
				SP2519_write_cmos_sensor(0x06,0x00);
				SP2519_write_cmos_sensor(0x07,0x00);
				SP2519_write_cmos_sensor(0x08,0x00);
				SP2519_write_cmos_sensor(0x09,0x00);
				SP2519_write_cmos_sensor(0x0a,0x9c);
				SP2519_write_cmos_sensor(0xfd,0x01);
				SP2519_write_cmos_sensor(0xf0,0x00);
				SP2519_write_cmos_sensor(0xf7,0x74);
				SP2519_write_cmos_sensor(0xf8,0x74);
				SP2519_write_cmos_sensor(0x02,0x14);
				SP2519_write_cmos_sensor(0x03,0x01);
				SP2519_write_cmos_sensor(0x06,0x74);
				SP2519_write_cmos_sensor(0x07,0x00);
				SP2519_write_cmos_sensor(0x08,0x01);
				SP2519_write_cmos_sensor(0x09,0x00);
				SP2519_write_cmos_sensor(0xfd,0x02);
				SP2519_write_cmos_sensor(0x3d,0x14);
				SP2519_write_cmos_sensor(0x3e,0x74);
				SP2519_write_cmos_sensor(0x3f,0x00);
				SP2519_write_cmos_sensor(0x88,0x69);
				SP2519_write_cmos_sensor(0x89,0x69);
				SP2519_write_cmos_sensor(0x8a,0x44);
				SP2519_write_cmos_sensor(0xfd,0x02);
				SP2519_write_cmos_sensor(0xbe,0x10);
				SP2519_write_cmos_sensor(0xbf,0x09);
				SP2519_write_cmos_sensor(0xd0,0x10);
				SP2519_write_cmos_sensor(0xd1,0x09);
				SP2519_write_cmos_sensor(0xc9,0x10);
				SP2519_write_cmos_sensor(0xca,0x09);
#endif
			}
		}
	}
	else /*normal mode settings*/
	{
		SENSORDB(" normal mode\r\n");
		SP2519_CAM_Nightmode = 0;
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0xcd,SP2519_NORMAL_Y0ffset);
		SP2519_write_cmos_sensor(0xce,0x18);

		if (SP2519_MPEG4_encode_mode == KAL_TRUE) 
		{
			SENSORDB(" video normal mode\r\n");
			if(SP2519_CAM_BANDING_50HZ == KAL_TRUE)/*video normal mode 50hz*/
			{
				SENSORDB(" video normal mode 50hz\r\n");

#ifdef SP2519_24M_60M
				//24M fix 10fps 2.5PLL 50HZ
				SP2519_write_cmos_sensor(0xfd,0x00);
				SP2519_write_cmos_sensor(0x03,0x02);
				SP2519_write_cmos_sensor(0x04,0xe8);
				SP2519_write_cmos_sensor(0x05,0x00);
				SP2519_write_cmos_sensor(0x06,0x00);
				SP2519_write_cmos_sensor(0x07,0x00);
				SP2519_write_cmos_sensor(0x08,0x00);
				SP2519_write_cmos_sensor(0x09,0x01);
				SP2519_write_cmos_sensor(0x0a,0x20);
				SP2519_write_cmos_sensor(0xfd,0x01);
				SP2519_write_cmos_sensor(0xf0,0x00);
				SP2519_write_cmos_sensor(0xf7,0x7c);
				SP2519_write_cmos_sensor(0xf8,0x67);
				SP2519_write_cmos_sensor(0x02,0x0a);
				SP2519_write_cmos_sensor(0x03,0x01);
				SP2519_write_cmos_sensor(0x06,0x7c);
				SP2519_write_cmos_sensor(0x07,0x00);
				SP2519_write_cmos_sensor(0x08,0x01);
				SP2519_write_cmos_sensor(0x09,0x00);
				SP2519_write_cmos_sensor(0xfd,0x02);
				SP2519_write_cmos_sensor(0x3d,0x0c);
				SP2519_write_cmos_sensor(0x3e,0x67);
				SP2519_write_cmos_sensor(0x3f,0x00);
				SP2519_write_cmos_sensor(0x88,0x21);
				SP2519_write_cmos_sensor(0x89,0xf8);
				SP2519_write_cmos_sensor(0x8a,0x44);
				SP2519_write_cmos_sensor(0xfd,0x02);
				SP2519_write_cmos_sensor(0xbe,0xd8);
				SP2519_write_cmos_sensor(0xbf,0x04);
				SP2519_write_cmos_sensor(0xd0,0xd8);
				SP2519_write_cmos_sensor(0xd1,0x04);
				SP2519_write_cmos_sensor(0xc9,0xd8);
				SP2519_write_cmos_sensor(0xca,0x04);
#endif
			}
			else/*video normal mode 60hz*/
			{
				SENSORDB(" video normal mode 60hz\r\n");  

#ifdef SP2519_24M_60M
				//24M fix 10fps 2.5pll 60HZ
				SP2519_write_cmos_sensor(0xfd,0x00);
				SP2519_write_cmos_sensor(0x03,0x02);
				SP2519_write_cmos_sensor(0x04,0x6a);
				SP2519_write_cmos_sensor(0x05,0x00);
				SP2519_write_cmos_sensor(0x06,0x00);
				SP2519_write_cmos_sensor(0x07,0x00);
				SP2519_write_cmos_sensor(0x08,0x00);
				SP2519_write_cmos_sensor(0x09,0x01);
				SP2519_write_cmos_sensor(0x0a,0x24);
				SP2519_write_cmos_sensor(0xfd,0x01);
				SP2519_write_cmos_sensor(0xf0,0x00);
				SP2519_write_cmos_sensor(0xf7,0x67);
				SP2519_write_cmos_sensor(0xf8,0x67);
				SP2519_write_cmos_sensor(0x02,0x0c);
				SP2519_write_cmos_sensor(0x03,0x01);
				SP2519_write_cmos_sensor(0x06,0x67);
				SP2519_write_cmos_sensor(0x07,0x00);
				SP2519_write_cmos_sensor(0x08,0x01);
				SP2519_write_cmos_sensor(0x09,0x00);
				SP2519_write_cmos_sensor(0xfd,0x02);
				SP2519_write_cmos_sensor(0x3d,0x0c);
				SP2519_write_cmos_sensor(0x3e,0x67);
				SP2519_write_cmos_sensor(0x3f,0x00);
				SP2519_write_cmos_sensor(0x88,0xf8);
				SP2519_write_cmos_sensor(0x89,0xf8);
				SP2519_write_cmos_sensor(0x8a,0x44);
				SP2519_write_cmos_sensor(0xfd,0x02);
				SP2519_write_cmos_sensor(0xbe,0xd4);
				SP2519_write_cmos_sensor(0xbf,0x04);
				SP2519_write_cmos_sensor(0xd0,0xd4);
				SP2519_write_cmos_sensor(0xd1,0x04);
				SP2519_write_cmos_sensor(0xc9,0xd4);
				SP2519_write_cmos_sensor(0xca,0x04);
#endif
			}
		}
		else/*capture normal mode*/
		{
			SENSORDB(" capture normal mode\r\n");
			if(SP2519_CAM_BANDING_50HZ == KAL_TRUE)/*capture normal mode 50hz*/
			{
				SENSORDB(" capture normal mode 50hz\r\n");

#ifdef SP2519_24M_60M
				//24M 2.5pll 8-11.2fps 50HZ Maxgain=0xf8
				SP2519_write_cmos_sensor(0xfd,0x00);
				SP2519_write_cmos_sensor(0x03,0x03);
				SP2519_write_cmos_sensor(0x04,0x3c);
				SP2519_write_cmos_sensor(0x05,0x00);
				SP2519_write_cmos_sensor(0x06,0x00);
				SP2519_write_cmos_sensor(0x07,0x00);
				SP2519_write_cmos_sensor(0x08,0x00);
				SP2519_write_cmos_sensor(0x09,0x00);
				SP2519_write_cmos_sensor(0x0a,0xa3);
				SP2519_write_cmos_sensor(0xfd,0x01);
				SP2519_write_cmos_sensor(0xf0,0x00);
				SP2519_write_cmos_sensor(0xf7,0x8a);
				SP2519_write_cmos_sensor(0xf8,0x73);
				SP2519_write_cmos_sensor(0x02,0x0c);
				SP2519_write_cmos_sensor(0x03,0x01);
				SP2519_write_cmos_sensor(0x06,0x8a);
				SP2519_write_cmos_sensor(0x07,0x00);
				SP2519_write_cmos_sensor(0x08,0x01);
				SP2519_write_cmos_sensor(0x09,0x00);
				SP2519_write_cmos_sensor(0xfd,0x02);
				SP2519_write_cmos_sensor(0x3d,0x0f);
				SP2519_write_cmos_sensor(0x3e,0x73);
				SP2519_write_cmos_sensor(0x3f,0x00);
				SP2519_write_cmos_sensor(0x88,0xb5);
				SP2519_write_cmos_sensor(0x89,0x73);
				SP2519_write_cmos_sensor(0x8a,0x43);
				SP2519_write_cmos_sensor(0xfd,0x02);
				SP2519_write_cmos_sensor(0xbe,0x78);
				SP2519_write_cmos_sensor(0xbf,0x06);
				SP2519_write_cmos_sensor(0xd0,0x78);
				SP2519_write_cmos_sensor(0xd1,0x06);
				SP2519_write_cmos_sensor(0xc9,0x78);
				SP2519_write_cmos_sensor(0xca,0x06);
#endif
			}
			else/*capture normal mode 60hz*/
			{
				SENSORDB(" capture normal mode 60hz\r\n");

#ifdef SP2519_24M_60M
				//24M 8-11.2fps 2.5PLL 60HZ 
				SP2519_write_cmos_sensor(0xfd,0x00);
				SP2519_write_cmos_sensor(0x03,0x02);
				SP2519_write_cmos_sensor(0x04,0xb8);
				SP2519_write_cmos_sensor(0x05,0x00);
				SP2519_write_cmos_sensor(0x06,0x00);
				SP2519_write_cmos_sensor(0x07,0x00);
				SP2519_write_cmos_sensor(0x08,0x00);
				SP2519_write_cmos_sensor(0x09,0x00);
				SP2519_write_cmos_sensor(0x0a,0x9c);
				SP2519_write_cmos_sensor(0xfd,0x01);
				SP2519_write_cmos_sensor(0xf0,0x00);
				SP2519_write_cmos_sensor(0xf7,0x74);
				SP2519_write_cmos_sensor(0xf8,0x74);
				SP2519_write_cmos_sensor(0x02,0x0f);
				SP2519_write_cmos_sensor(0x03,0x01);
				SP2519_write_cmos_sensor(0x06,0x74);
				SP2519_write_cmos_sensor(0x07,0x00);
				SP2519_write_cmos_sensor(0x08,0x01);
				SP2519_write_cmos_sensor(0x09,0x00);
				SP2519_write_cmos_sensor(0xfd,0x02);
				SP2519_write_cmos_sensor(0x3d,0x0f);
				SP2519_write_cmos_sensor(0x3e,0x74);
				SP2519_write_cmos_sensor(0x3f,0x00);
				SP2519_write_cmos_sensor(0x88,0x69);
				SP2519_write_cmos_sensor(0x89,0x69);
				SP2519_write_cmos_sensor(0x8a,0x44);
				SP2519_write_cmos_sensor(0xfd,0x02);
				SP2519_write_cmos_sensor(0xbe,0xcc);
				SP2519_write_cmos_sensor(0xbf,0x06);
				SP2519_write_cmos_sensor(0xd0,0xcc);
				SP2519_write_cmos_sensor(0xd1,0x06);
				SP2519_write_cmos_sensor(0xc9,0xcc);
				SP2519_write_cmos_sensor(0xca,0x06);
#endif
			}
		}
	}
} /* SP2519_NightMode */


/*************************************************************************
* FUNCTION
*	SP2519_Sensor_Init
*
* DESCRIPTION
*	This function apply all of the initial setting to sensor.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
*************************************************************************/

void SP2519_Sensor_Init(void)
{
	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0x36,0x02);//add Kavie 20141224	
	SP2519_write_cmos_sensor(0xfd,0x00);
	//SP2519_write_cmos_sensor(0x1c,0x17);//del Kavie 20141224
	SP2519_write_cmos_sensor(0x30,0x00);//uxga  svga
#ifdef SP2519_24M_60M
	SP2519_write_cmos_sensor(0x2f,0x11);
#endif
	SP2519_write_cmos_sensor(0x09,0x01);
	SP2519_write_cmos_sensor(0xfd,0x00);
	SP2519_write_cmos_sensor(0x0c,0x55);
	SP2519_write_cmos_sensor(0x27,0xa5);
	SP2519_write_cmos_sensor(0x1a,0x4b);
	SP2519_write_cmos_sensor(0x20,0x2f);
	SP2519_write_cmos_sensor(0x22,0x5a);
	SP2519_write_cmos_sensor(0x25,0xad);
	SP2519_write_cmos_sensor(0x21,0x0d);//0x10 Kavie 20141224 svn
	SP2519_write_cmos_sensor(0x28,0x08);
	SP2519_write_cmos_sensor(0x1d,0x01);
	//SP2519_write_cmos_sensor(0x6c,0x00);//del 20141224 svn
	SP2519_write_cmos_sensor(0x7a,0x5d);
	SP2519_write_cmos_sensor(0x70,0x41);
	SP2519_write_cmos_sensor(0x74,0x40);
	SP2519_write_cmos_sensor(0x75,0x40);
	SP2519_write_cmos_sensor(0x15,0x3e);
	SP2519_write_cmos_sensor(0x71,0x3f);
	SP2519_write_cmos_sensor(0x7c,0x3f);
	SP2519_write_cmos_sensor(0x76,0x3f);
	SP2519_write_cmos_sensor(0x7e,0x29);
	SP2519_write_cmos_sensor(0x72,0x29);
	SP2519_write_cmos_sensor(0x77,0x28);
	SP2519_write_cmos_sensor(0x1e,0x01);

	/*Begin ersen.shang modify 0x1c=0x17 for pr956347*/
	SP2519_write_cmos_sensor(0x1c,0x17);
	/*End   ersen.shang modify 0x1c=0x17 for pr956347*/	
	
	SP2519_write_cmos_sensor(0x2e,0xc5);//0xc0 20141224 svn

	/*Begin ersen.shang modify 0x1f=0xe0 for pr956347*/	
	SP2519_write_cmos_sensor(0x1f,0xe0);
	/*End   ersen.shang modify 0x1f=0xe0 for pr956347*/
	
	SP2519_write_cmos_sensor(0x6c,0x00);

	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0x32,0x00);//add 20141224 svn
	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0x85,0x00);

	//SP2519_write_cmos_sensor(0xfd,0x02);//del 20141224 svn
	//SP2519_write_cmos_sensor(0x26,0xa0);//del 20141224 svn
	//SP2519_write_cmos_sensor(0x27,0x96);//del 20141224 svn

	SP2519_write_cmos_sensor(0xfd,0x00);
	SP2519_write_cmos_sensor(0xe7,0x03);
	SP2519_write_cmos_sensor(0xe7,0x00);
	//ae_set
	SP2519_write_cmos_sensor(0xfd,0x00);
	SP2519_write_cmos_sensor(0x03,0x03);
	SP2519_write_cmos_sensor(0x04,0x3c);
	SP2519_write_cmos_sensor(0x05,0x00);
	SP2519_write_cmos_sensor(0x06,0x00);
	SP2519_write_cmos_sensor(0x07,0x00);
	SP2519_write_cmos_sensor(0x08,0x00);
	SP2519_write_cmos_sensor(0x09,0x00);
	SP2519_write_cmos_sensor(0x0a,0xa3);
	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0xf0,0x00);
	SP2519_write_cmos_sensor(0xf7,0x8a);
	SP2519_write_cmos_sensor(0xf8,0x73);
	SP2519_write_cmos_sensor(0x02,0x0c);
	SP2519_write_cmos_sensor(0x03,0x01);
	SP2519_write_cmos_sensor(0x06,0x8a);
	SP2519_write_cmos_sensor(0x07,0x00);
	SP2519_write_cmos_sensor(0x08,0x01);
	SP2519_write_cmos_sensor(0x09,0x00);
	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0x3d,0x0f);
	SP2519_write_cmos_sensor(0x3e,0x73);
	SP2519_write_cmos_sensor(0x3f,0x00);
	SP2519_write_cmos_sensor(0x88,0xb5);
	SP2519_write_cmos_sensor(0x89,0x73);
	SP2519_write_cmos_sensor(0x8a,0x43);
	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0xbe,0x78);
	SP2519_write_cmos_sensor(0xbf,0x06);
	SP2519_write_cmos_sensor(0xd0,0x78);
	SP2519_write_cmos_sensor(0xd1,0x06);
	SP2519_write_cmos_sensor(0xc9,0x78);
	SP2519_write_cmos_sensor(0xca,0x06);

	//SP2519_write_cmos_sensor(0xfd,0x01);//del 20141224 svn
	//SP2519_write_cmos_sensor(0x32,0x15);//del 20141224 svn
	//SP2519_write_cmos_sensor(0xfd,0x00);//del 20141224 svn
	//SP2519_write_cmos_sensor(0xe7,0x03);//del 20141224 svn
	//SP2519_write_cmos_sensor(0xe7,0x00);//del 20141224 svn
	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0xb8,0x70);//0x90 20141224 svn
	SP2519_write_cmos_sensor(0xb9,0x80);//0x85 20141224 svn
	SP2519_write_cmos_sensor(0xba,0x30);
	SP2519_write_cmos_sensor(0xbb,0x45);
	SP2519_write_cmos_sensor(0xbc,0x90);//0xc0 20141224 svn
	SP2519_write_cmos_sensor(0xbd,0x70);//0x60 20141224 svn
	SP2519_write_cmos_sensor(0xfd,0x03);
	SP2519_write_cmos_sensor(0x77,0x48);
	//RPC
	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0xe0,0x48);
	SP2519_write_cmos_sensor(0xe1,0x38);
	SP2519_write_cmos_sensor(0xe2,0x30);
	SP2519_write_cmos_sensor(0xe3,0x2c);
	SP2519_write_cmos_sensor(0xe4,0x2c);
	SP2519_write_cmos_sensor(0xe5,0x2a);
	SP2519_write_cmos_sensor(0xe6,0x2a);
	SP2519_write_cmos_sensor(0xe7,0x28);
	SP2519_write_cmos_sensor(0xe8,0x28);
	SP2519_write_cmos_sensor(0xe9,0x28);
	SP2519_write_cmos_sensor(0xea,0x26);
	SP2519_write_cmos_sensor(0xf3,0x26);
	SP2519_write_cmos_sensor(0xf4,0x26);
	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0x04,0xc0);
	SP2519_write_cmos_sensor(0x05,0x26);
	SP2519_write_cmos_sensor(0x0a,0x48);
	SP2519_write_cmos_sensor(0x0b,0x26);

	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0xf2,0x09);
	SP2519_write_cmos_sensor(0xeb,0x78);
	SP2519_write_cmos_sensor(0xec,0x78);
	SP2519_write_cmos_sensor(0xed,0x06);
	SP2519_write_cmos_sensor(0xee,0x0a);

	SP2519_write_cmos_sensor(0xfd,0x02);//add 20141224 svn
	SP2519_write_cmos_sensor(0x4f,0x46);//add 20141224 svn

	SP2519_write_cmos_sensor(0xfd,0x03);
	SP2519_write_cmos_sensor(0x52,0xff);
	SP2519_write_cmos_sensor(0x53,0x60);
	SP2519_write_cmos_sensor(0x94,0x00);
	SP2519_write_cmos_sensor(0x54,0x00);
	SP2519_write_cmos_sensor(0x55,0x00);
	SP2519_write_cmos_sensor(0x56,0x80);
	SP2519_write_cmos_sensor(0x57,0x80);
	SP2519_write_cmos_sensor(0x95,0x00);
	SP2519_write_cmos_sensor(0x58,0x00);
	SP2519_write_cmos_sensor(0x59,0x00);
	SP2519_write_cmos_sensor(0x5a,0xf6);
	SP2519_write_cmos_sensor(0x5b,0x00);
	SP2519_write_cmos_sensor(0x5c,0x88);
	SP2519_write_cmos_sensor(0x5d,0x00);
	SP2519_write_cmos_sensor(0x96,0x00);
	SP2519_write_cmos_sensor(0xfd,0x03);
	SP2519_write_cmos_sensor(0x8a,0x00);
	SP2519_write_cmos_sensor(0x8b,0x00);
	SP2519_write_cmos_sensor(0x8c,0xff);
	SP2519_write_cmos_sensor(0x22,0xff);
	SP2519_write_cmos_sensor(0x23,0xff);
	SP2519_write_cmos_sensor(0x24,0xff);
	SP2519_write_cmos_sensor(0x25,0xff);
	SP2519_write_cmos_sensor(0x5e,0xff);
	SP2519_write_cmos_sensor(0x5f,0xff);
	SP2519_write_cmos_sensor(0x60,0xff);
	SP2519_write_cmos_sensor(0x61,0xff);
	SP2519_write_cmos_sensor(0x62,0x00);
	SP2519_write_cmos_sensor(0x63,0x00);
	SP2519_write_cmos_sensor(0x64,0x00);
	SP2519_write_cmos_sensor(0x65,0x00);
	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0x21,0x00);
	SP2519_write_cmos_sensor(0x22,0x00);
	SP2519_write_cmos_sensor(0x26,0x60);
	SP2519_write_cmos_sensor(0x27,0x14);
	SP2519_write_cmos_sensor(0x28,0x05);
	SP2519_write_cmos_sensor(0x29,0x00);
	SP2519_write_cmos_sensor(0x2a,0x01);   //enable
	//shading  for pingchuang 2236

	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0xa1,0x16);
	SP2519_write_cmos_sensor(0xa2,0x18);
	SP2519_write_cmos_sensor(0xa3,0x16);
	SP2519_write_cmos_sensor(0xa4,0x16);
	SP2519_write_cmos_sensor(0xa5,0x11);
	SP2519_write_cmos_sensor(0xa6,0x14);
	SP2519_write_cmos_sensor(0xa7,0x11);
	SP2519_write_cmos_sensor(0xa8,0x11);
	SP2519_write_cmos_sensor(0xa9,0x0e);
	SP2519_write_cmos_sensor(0xaa,0x12);
	SP2519_write_cmos_sensor(0xab,0x0f);
	SP2519_write_cmos_sensor(0xac,0x10);
	SP2519_write_cmos_sensor(0xad,0x04);
	SP2519_write_cmos_sensor(0xae,0x07);
	SP2519_write_cmos_sensor(0xaf,0x04);
	SP2519_write_cmos_sensor(0xb0,0x03);
	SP2519_write_cmos_sensor(0xb1,0x03);
	SP2519_write_cmos_sensor(0xb2,0x05);
	SP2519_write_cmos_sensor(0xb3,0x02);
	SP2519_write_cmos_sensor(0xb4,0x02);
	SP2519_write_cmos_sensor(0xb5,0x02);
	SP2519_write_cmos_sensor(0xb6,0x03);
	SP2519_write_cmos_sensor(0xb7,0x02);
	SP2519_write_cmos_sensor(0xb8,0x03);

	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0x26,0xa0);  //Red channel gain  svn                               
	SP2519_write_cmos_sensor(0x27,0x96);  //Blue channel gain svn                               
	SP2519_write_cmos_sensor(0x28,0xcc);  //Y top value limit                                
	SP2519_write_cmos_sensor(0x29,0x01);
	SP2519_write_cmos_sensor(0x2a,0x00);
	SP2519_write_cmos_sensor(0x2b,0x00);
	SP2519_write_cmos_sensor(0x2c,0x20);
	SP2519_write_cmos_sensor(0x2d,0xdc);
	SP2519_write_cmos_sensor(0x2e,0x20);
	SP2519_write_cmos_sensor(0x2f,0x96);
	SP2519_write_cmos_sensor(0x1b,0x80);
	SP2519_write_cmos_sensor(0x1a,0x80);
	SP2519_write_cmos_sensor(0x18,0x16);
	SP2519_write_cmos_sensor(0x19,0x26);
	SP2519_write_cmos_sensor(0x1d,0x04);
	SP2519_write_cmos_sensor(0x1f,0x06);
	//  AWB
	SP2519_write_cmos_sensor(0x66,0x24);
	SP2519_write_cmos_sensor(0x67,0x60);
	SP2519_write_cmos_sensor(0x68,0xb0);
	SP2519_write_cmos_sensor(0x69,0xe4);
	SP2519_write_cmos_sensor(0x6a,0xa5);

	SP2519_write_cmos_sensor(0x7c,0x24);
	SP2519_write_cmos_sensor(0x7d,0x4c);

	SP2519_write_cmos_sensor(0x7e,0xda);
	SP2519_write_cmos_sensor(0x7f,0x00);
	SP2519_write_cmos_sensor(0x80,0xa6);

	SP2519_write_cmos_sensor(0x70,0x18);
	SP2519_write_cmos_sensor(0x71,0x38);
	SP2519_write_cmos_sensor(0x72,0x01);
	SP2519_write_cmos_sensor(0x73,0x20);
	SP2519_write_cmos_sensor(0x74,0xaa);

	SP2519_write_cmos_sensor(0x6b,0xff);
	SP2519_write_cmos_sensor(0x6c,0x23);
	SP2519_write_cmos_sensor(0x6d,0xff);
	SP2519_write_cmos_sensor(0x6e,0x18);
	SP2519_write_cmos_sensor(0x6f,0xaa);

	SP2519_write_cmos_sensor(0x61,0xfd);
	SP2519_write_cmos_sensor(0x62,0x1d);
	SP2519_write_cmos_sensor(0x63,0x1c);
	SP2519_write_cmos_sensor(0x64,0x3c);
	SP2519_write_cmos_sensor(0x65,0x6a);

	SP2519_write_cmos_sensor(0x75,0x00);
	SP2519_write_cmos_sensor(0x76,0x09);
	SP2519_write_cmos_sensor(0x77,0x02);
	SP2519_write_cmos_sensor(0x0e,0x16);
	SP2519_write_cmos_sensor(0x3b,0x09);

	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0x02,0x00);
	SP2519_write_cmos_sensor(0x03,0x10);

	SP2519_write_cmos_sensor(0x04,0xf0);

	SP2519_write_cmos_sensor(0xf5,0xb3);//0xa8 20141224 svn
	SP2519_write_cmos_sensor(0xf6,0x80);//0x7c 20141224 svn
	SP2519_write_cmos_sensor(0xf7,0xe0);//0xb4 20141224 svn
	SP2519_write_cmos_sensor(0xf8,0x89);//0x80 20141224 svn

	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0x08,0x00);
	SP2519_write_cmos_sensor(0x09,0x04);
	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0xdd,0x0f);
	SP2519_write_cmos_sensor(0xde,0x0f);
	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0x57,0x30);
	SP2519_write_cmos_sensor(0x58,0x10);
	SP2519_write_cmos_sensor(0x59,0xe0);
	SP2519_write_cmos_sensor(0x5a,0x00);
	SP2519_write_cmos_sensor(0x5b,0x12);

	SP2519_write_cmos_sensor(0xcb,0x08);
	SP2519_write_cmos_sensor(0xcc,0x0b);
	SP2519_write_cmos_sensor(0xcd,0x10);
	SP2519_write_cmos_sensor(0xce,0x1a);

	SP2519_write_cmos_sensor(0xfd,0x03);
	SP2519_write_cmos_sensor(0x87,0x04);
	SP2519_write_cmos_sensor(0x88,0x08);
	SP2519_write_cmos_sensor(0x89,0x10);
	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0xe8,0x54);// in outdoor     //0x58   
	SP2519_write_cmos_sensor(0xec,0x64); //in outdoor   //0x68     
	SP2519_write_cmos_sensor(0xe9,0x50); //in normal     //0x60    
	SP2519_write_cmos_sensor(0xed,0x60); //in normal    //0x68     
	SP2519_write_cmos_sensor(0xea,0x48);//in dummy      //0x58     
	SP2519_write_cmos_sensor(0xee,0x50); //in dummy   //0x60       
	SP2519_write_cmos_sensor(0xeb,0x40);//in lowlight     //0x48   
	SP2519_write_cmos_sensor(0xef,0x43); //in low light      

	SP2519_write_cmos_sensor(0xfd,0x02);       
	SP2519_write_cmos_sensor(0xdc,0x04);       
	SP2519_write_cmos_sensor(0x05,0x6f);
	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0xf4,0x30);
	SP2519_write_cmos_sensor(0xfd,0x03);
	SP2519_write_cmos_sensor(0x97,0x98);
	SP2519_write_cmos_sensor(0x98,0x88);
	SP2519_write_cmos_sensor(0x99,0x88);
	SP2519_write_cmos_sensor(0x9a,0x80);
	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0xe4,0xff);
	SP2519_write_cmos_sensor(0xe5,0xff);
	SP2519_write_cmos_sensor(0xe6,0xff);
	SP2519_write_cmos_sensor(0xe7,0xff);
	SP2519_write_cmos_sensor(0xfd,0x03);

	SP2519_write_cmos_sensor(0x72,0x18);//0x10 20141224 svn
	SP2519_write_cmos_sensor(0x73,0x28);//0x20 20141224 svn
	SP2519_write_cmos_sensor(0x74,0x28);//0x20 20141224 svn
	SP2519_write_cmos_sensor(0x75,0x30);//0x28 20141224 svn

	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0x78,0x20);
	SP2519_write_cmos_sensor(0x79,0x20);
	SP2519_write_cmos_sensor(0x7a,0x14);
	SP2519_write_cmos_sensor(0x7b,0x08);
	SP2519_write_cmos_sensor(0x81,0x02);
	SP2519_write_cmos_sensor(0x82,0x20);
	SP2519_write_cmos_sensor(0x83,0x20);
	SP2519_write_cmos_sensor(0x84,0x08);
	SP2519_write_cmos_sensor(0xfd,0x03);
	SP2519_write_cmos_sensor(0x7e,0x06);
	SP2519_write_cmos_sensor(0x7f,0x0d);
	SP2519_write_cmos_sensor(0x80,0x10);
	SP2519_write_cmos_sensor(0x81,0x16);
	SP2519_write_cmos_sensor(0x7c,0xff);
	SP2519_write_cmos_sensor(0x82,0x54);
	SP2519_write_cmos_sensor(0x83,0x43);  
	SP2519_write_cmos_sensor(0x84,0x00);
	SP2519_write_cmos_sensor(0x85,0x20);
	SP2519_write_cmos_sensor(0x86,0x40);
	SP2519_write_cmos_sensor(0xfd,0x03);
	SP2519_write_cmos_sensor(0x66,0x18);
	SP2519_write_cmos_sensor(0x67,0x28);
	SP2519_write_cmos_sensor(0x68,0x20);
	SP2519_write_cmos_sensor(0x69,0x88);
	SP2519_write_cmos_sensor(0x9b,0x18);
	SP2519_write_cmos_sensor(0x9c,0x28);
	SP2519_write_cmos_sensor(0x9d,0x20);

	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0x8b,0x00);
	SP2519_write_cmos_sensor(0x8c,0x0d);
	SP2519_write_cmos_sensor(0x8d,0x19);
	SP2519_write_cmos_sensor(0x8e,0x26);
	SP2519_write_cmos_sensor(0x8f,0x30);
	SP2519_write_cmos_sensor(0x90,0x43);
	SP2519_write_cmos_sensor(0x91,0x53);
	SP2519_write_cmos_sensor(0x92,0x5e);
	SP2519_write_cmos_sensor(0x93,0x6a);
	SP2519_write_cmos_sensor(0x94,0x7d);
	SP2519_write_cmos_sensor(0x95,0x8d);
	SP2519_write_cmos_sensor(0x96,0x9e);
	SP2519_write_cmos_sensor(0x97,0xac);
	SP2519_write_cmos_sensor(0x98,0xba);
	SP2519_write_cmos_sensor(0x99,0xc6);
	SP2519_write_cmos_sensor(0x9a,0xd1);
	SP2519_write_cmos_sensor(0x9b,0xda);
	SP2519_write_cmos_sensor(0x9c,0xe4);
	SP2519_write_cmos_sensor(0x9d,0xeb);
	SP2519_write_cmos_sensor(0x9e,0xf2);
	SP2519_write_cmos_sensor(0x9f,0xf9);
	SP2519_write_cmos_sensor(0xa0,0xff);

	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0x15,0xa9);//0x9f 20141224 svn
	SP2519_write_cmos_sensor(0x16,0x84);//0x92 20141224 svn
	//!f  ccm
	SP2519_write_cmos_sensor(0xa0,0x97);
	SP2519_write_cmos_sensor(0xa1,0xea);
	SP2519_write_cmos_sensor(0xa2,0xff);
	SP2519_write_cmos_sensor(0xa3,0x0e);
	SP2519_write_cmos_sensor(0xa4,0x77);
	SP2519_write_cmos_sensor(0xa5,0xfa);
	SP2519_write_cmos_sensor(0xa6,0x08);
	SP2519_write_cmos_sensor(0xa7,0xcb);
	SP2519_write_cmos_sensor(0xa8,0xad);
	SP2519_write_cmos_sensor(0xa9,0x3c);
	SP2519_write_cmos_sensor(0xaa,0x30);
	SP2519_write_cmos_sensor(0xab,0x0c);
	//f  ccm
	SP2519_write_cmos_sensor(0xac,0x7f);
	SP2519_write_cmos_sensor(0xad,0x0b);
	SP2519_write_cmos_sensor(0xae,0xf4);
	SP2519_write_cmos_sensor(0xaf,0xfd);
	SP2519_write_cmos_sensor(0xb0,0x72);
	SP2519_write_cmos_sensor(0xb1,0x11);
	SP2519_write_cmos_sensor(0xb2,0xe7);
	SP2519_write_cmos_sensor(0xb3,0x96);
	SP2519_write_cmos_sensor(0xb4,0x04);
	SP2519_write_cmos_sensor(0xb5,0x30);
	SP2519_write_cmos_sensor(0xb6,0x03);
	SP2519_write_cmos_sensor(0xb7,0x1f);

	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0xd2,0x2d);
	SP2519_write_cmos_sensor(0xd1,0x38);
	SP2519_write_cmos_sensor(0xdd,0x3f);
	SP2519_write_cmos_sensor(0xde,0x37);
	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0xc1,0x40);
	SP2519_write_cmos_sensor(0xc2,0x40);
	SP2519_write_cmos_sensor(0xc3,0x40);
	SP2519_write_cmos_sensor(0xc4,0x40);
	SP2519_write_cmos_sensor(0xc5,0x80);
	SP2519_write_cmos_sensor(0xc6,0x60);
	SP2519_write_cmos_sensor(0xc7,0x00);
	SP2519_write_cmos_sensor(0xc8,0x00);

	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0xd3,0x98);
	SP2519_write_cmos_sensor(0xd4,0x94);
	SP2519_write_cmos_sensor(0xd5,0x8c);//0x90 Kavie 20141225
	SP2519_write_cmos_sensor(0xd6,0x80);//0x80 Kavie 20141225

	SP2519_write_cmos_sensor(0xd7,0x98);
	SP2519_write_cmos_sensor(0xd8,0x94);
	SP2519_write_cmos_sensor(0xd9,0x8c);//0x90 Kavie 20141225
	SP2519_write_cmos_sensor(0xda,0x80);//0x80 Kavie 20141225

	SP2519_write_cmos_sensor(0xfd,0x03);
	SP2519_write_cmos_sensor(0x76,0x0a);
	SP2519_write_cmos_sensor(0x7a,0x40);
	SP2519_write_cmos_sensor(0x7b,0x40);
	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0xc2,0xaa);
	SP2519_write_cmos_sensor(0xc3,0xaa);
	SP2519_write_cmos_sensor(0xc4,0x66);
	SP2519_write_cmos_sensor(0xc5,0x66);

	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0xcd,0x18);
	SP2519_write_cmos_sensor(0xce,0x18);

	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0x32,0x60);
	SP2519_write_cmos_sensor(0x35,0x60);
	SP2519_write_cmos_sensor(0x37,0x13);
	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0xdb,0x00);

	SP2519_write_cmos_sensor(0x10,0x84);//0x88 Kavie 20141225
	SP2519_write_cmos_sensor(0x11,0x84);//0x88 Kavie 20141225
	SP2519_write_cmos_sensor(0x12,0x88);
	SP2519_write_cmos_sensor(0x13,0x8a);

	SP2519_write_cmos_sensor(0x14,0xb8);//0xba Kavie 20141225
	SP2519_write_cmos_sensor(0x15,0xb0);//0xba  Kavie 20141225
	SP2519_write_cmos_sensor(0x16,0xa4);//0xa8  Kavie 20141225
	SP2519_write_cmos_sensor(0x17,0xa0);//0xa0  Kavie 20141225


	SP2519_write_cmos_sensor(0xfd,0x03);
	SP2519_write_cmos_sensor(0x00,0x80);
	SP2519_write_cmos_sensor(0x03,0x68);
	SP2519_write_cmos_sensor(0x06,0xd8);
	SP2519_write_cmos_sensor(0x07,0x28);
	SP2519_write_cmos_sensor(0x0a,0xfd);
	SP2519_write_cmos_sensor(0x01,0x16);
	SP2519_write_cmos_sensor(0x02,0x16);
	SP2519_write_cmos_sensor(0x04,0x16);
	SP2519_write_cmos_sensor(0x05,0x16);
	SP2519_write_cmos_sensor(0x0b,0x40);
	SP2519_write_cmos_sensor(0x0c,0x40);
	SP2519_write_cmos_sensor(0x0d,0x40);
	SP2519_write_cmos_sensor(0x0e,0x40);
	SP2519_write_cmos_sensor(0x08,0x0c);
	SP2519_write_cmos_sensor(0x09,0x0c);
	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0x8e,0x0a);
	SP2519_write_cmos_sensor(0x90,0x40);
	SP2519_write_cmos_sensor(0x91,0x40);
	SP2519_write_cmos_sensor(0x92,0x60);
	SP2519_write_cmos_sensor(0x93,0x80);
	SP2519_write_cmos_sensor(0x9e,0x44);
	SP2519_write_cmos_sensor(0x9f,0x44);
	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0x85,0x00);
	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0x00,0x00);
	SP2519_write_cmos_sensor(0xfb,0x25);

	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0x32,0x15);
	SP2519_write_cmos_sensor(0xfd,0x00);
	SP2519_write_cmos_sensor(0xe7,0x03);
	SP2519_write_cmos_sensor(0xe7,0x00);

	SP2519_write_cmos_sensor(0xfd,0x01);	
	SP2519_write_cmos_sensor(0x33,0xef);
	SP2519_write_cmos_sensor(0x34,0xef);

	SP2519_write_cmos_sensor(0x35,0x40);
	SP2519_write_cmos_sensor(0xfd,0x00);
	SP2519_write_cmos_sensor(0x3f,0x03);
	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0x50,0x00);
	SP2519_write_cmos_sensor(0x66,0x00);
	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0xd6,0x0f);



	SP2519_write_cmos_sensor(0xfd,0x00);
	SP2519_write_cmos_sensor(0x1b,0x30);
	//SP2519_write_cmos_sensor(0x1d,0x00);//del 20141224 svn
	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0x36,0x00);
	SP2519_write_cmos_sensor(0xfd,0x00);

#ifdef Preview_SVGA//svga
	SP2519_config_window(WINDOW_SIZE_SVGA);
#else
	SP2519_config_window(WINDOW_SIZE_UXGA);
#endif
}


/*************************************************************************
* FUNCTION
*	SP2519_Write_More_Registers
*
* DESCRIPTION
*	This function is served for FAE to modify the necessary Init Regs. Do not modify the regs
*     in init_SP2519() directly.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void SP2519_Write_More_Registers(void)
{
	//Ronlus this function is used for FAE debug
}



/*************************************************************************
* FUNCTION
*	SP2519_PV_setting
*
* DESCRIPTION
*	This function apply the preview mode setting, normal the preview size is 1/4 of full size.
*	Ex. 2M (1600 x 1200)
*	Preview: 800 x 600 (Use sub-sample or binning to acheive it)
*	Full Size: 1600 x 1200 (Output every effective pixels.)
*
* PARAMETERS
*	1. image_sensor_exposure_window_struct : Set the grab start x,y and width,height.
*	2. image_sensor_config_struct : Current operation mode.
*
* RETURNS
*	None
*
*************************************************************************/
static void SP2519_PV_setting(void)
{
	//SP2519_write_cmos_sensor(0xfd,0x03);
	//SP2519_write_cmos_sensor(0x82,0x88);//raw_dns_fac_outdoor,raw_dns_fac_normal
	//SP2519_write_cmos_sensor(0x83,0x44);//raw_dns_fac_dummy,raw_dns_fac_low
} /* SP2519_PV_setting */


/*************************************************************************
* FUNCTION
*	SP2519_CAP_setting
*
* DESCRIPTION
*	This function apply the full size mode setting.
*	Ex. 2M (1600 x 1200)
*	Preview: 800 x 600 (Use sub-sample or binning to acheive it)
*	Full Size: 1600 x 1200 (Output every effective pixels.)
*
* PARAMETERS
*	1. image_sensor_exposure_window_struct : Set the grab start x,y and width,height.
*	2. image_sensor_config_struct : Current operation mode.
*
* RETURNS
*	None
*
*************************************************************************/
static void SP2519_CAP_setting(void)
{
	//SP2519_write_cmos_sensor(0xfd,0x03);
	//SP2519_write_cmos_sensor(0x82,0x44);//raw_dns_fac_outdoor,raw_dns_fac_normal
	//SP2519_write_cmos_sensor(0x83,0x22);//raw_dns_fac_dummy,raw_dns_fac_low
} /* SP2519_CAP_setting */
/*************************************************************************
* FUNCTION
*	SP2519Open
*
* DESCRIPTION
*	This function initialize the registers of CMOS sensor
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 SP2519Open(void)
{
	kal_uint16 sensor_id=0;
	int i;
	printk(" SP2519Open\r\n");
	// check if sensor ID correct
	for(i = 0; i < 3; i++)
	{
		SP2519_write_cmos_sensor(0xfd,0x00);
		sensor_id = SP2519_read_cmos_sensor(0x02);
		printk(" SP2519 Sensor id = %x\n", sensor_id);
		if (sensor_id == SP2519_SENSOR_ID)
		{
			break;
		}
	}
	mdelay(50);
	if(sensor_id != SP2519_SENSOR_ID)
	{
		SENSORDB("SP2519 Sensor id read failed, ID = %x\n", sensor_id);
		return ERROR_SENSOR_CONNECT_FAIL;
	}

#ifdef SP2519_DEBUG_SENSOR //gepeiwei   120903
	//判断手机对应目录下是否有名为sp2528_sd 的文件,没有默认参数

	//介于各种原因，本版本初始化参数在_s_fmt中。
	struct file *fp; 
	mm_segment_t fs; 
	loff_t pos = 0; 
	static char buf[10*1024] ;

	fp = filp_open("/mnt/sdcard/sp2519_sd", O_RDONLY , 0); 
	if (IS_ERR(fp)) { 
		SP2519_fromsd = 0;   
		printk("open file error\n");

	} 
	else 
	{
		SP2519_fromsd = 1;
		printk("open file ok\n");

		//SP2519_Initialize_from_T_Flash();


		filp_close(fp, NULL); 
		set_fs(fs);
	}

	if(SP2519_fromsd == 1)//是否从SD读取//gepeiwei   120903
	{
		printk("________________from t!\n");
		SP2519_Initialize_from_T_Flash();//从SD卡读取的主要函数
	}
	else
	{
		SP2519_Sensor_Init();
		SP2519_Write_More_Registers();//added for FAE to debut
	}
#else  
	//RETAILMSG(1, (TEXT("Sensor Read ID OK \r\n")));
	// initail sequence write in
	SP2519_Sensor_Init();
	SP2519_Write_More_Registers();//added for FAE to debut
#endif
	return ERROR_NONE;
} /* SP2519Open */


/*************************************************************************
* FUNCTION
*	SP2519Close
*
* DESCRIPTION
*	This function is to turn off sensor module power.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 SP2519Close(void)
{
	SENSORDB(" SP2519Close\r\n");
	flash_mode_sp2519= 0;
	return ERROR_NONE;
} /* SP2519Close */

LOCAL void SP2519_AfterSnapshot(void)
{
	kal_uint8  	AE_tmp;
#if 1
	kal_uint8   tmp1,tmp2,tmp3;
#endif
	SP2519_write_cmos_sensor(0xfd,0x01);
	AE_tmp=SP2519_read_cmos_sensor(0x32);
	AE_tmp=AE_tmp&0xfa;
	SP2519_write_cmos_sensor(0x32,AE_tmp);
	SP2519_write_cmos_sensor(0xfd,0x00);
	SP2519_write_cmos_sensor(0xe7,0x03);
	SP2519_write_cmos_sensor(0xe7,0x00);
	SP2519_Set_Shutter(G_shutter);
	SP2519_write_cmos_sensor(0xfd,0x00); 
	SP2519_write_cmos_sensor(0x24,G_Gain);
	SP2519_write_cmos_sensor(0xfd,0x00);
	SP2519_write_cmos_sensor(0xe7,0x03);
	SP2519_write_cmos_sensor(0xe7,0x00);
	SP2519_write_cmos_sensor(0xfd,0x01);
	AE_tmp=SP2519_read_cmos_sensor(0x32);
	AE_tmp=AE_tmp|0x05;
	SP2519_write_cmos_sensor(0x32,AE_tmp);
	SP2519_write_cmos_sensor(0xfd,0x00);
	SP2519_write_cmos_sensor(0xe7,0x03);
	SP2519_write_cmos_sensor(0xe7,0x00);
	return 0;
}  

LOCAL void SP2519_BeforeSnapshot(void)
{	
	kal_uint32 Shutter_temp;
	kal_uint16 Shutter, HB;
	kal_uint8  	HB_L,HB_H,Gain,AE_tmp;
	SP2519_write_cmos_sensor(0xfd,0x01);
	AE_tmp=SP2519_read_cmos_sensor(0x32);
	SP2519_write_cmos_sensor(0xfd,0x00); 
	Gain=SP2519_read_cmos_sensor(0x23);
	HB_H=SP2519_read_cmos_sensor(0x09);
	HB_L=SP2519_read_cmos_sensor(0x0a);

	G_Gain=Gain;
	HB = (HB_L & 0xFF) | (HB_H << 8);
	AE_tmp=AE_tmp&0xfa;
	Shutter = SP2519_Read_Shutter();
	G_shutter = Shutter;

	Shutter_temp = Shutter;	
	Shutter_temp = Shutter_temp*(517+HB)/2/(922+HB);	
	Shutter = Shutter_temp&0xffff;

	if(Shutter<1)
	{
		Shutter=1;
	}
	SP2519_write_cmos_sensor(0xfd,0x01);
	SP2519_write_cmos_sensor(0x32,AE_tmp);  
	SP2519_write_cmos_sensor(0xfd,0x00);
	SP2519_write_cmos_sensor(0xe7,0x03);
	SP2519_write_cmos_sensor(0xe7,0x00);

	SP2519_Set_Shutter(Shutter);	
	SP2519_write_cmos_sensor(0xfd,0x00); 
	SP2519_write_cmos_sensor(0x24,Gain);

	SP2519_write_cmos_sensor(0xfd,0x00);
	SP2519_write_cmos_sensor(0xe7,0x03);
	SP2519_write_cmos_sensor(0xe7,0x00);

	return 0;  
} 

/*************************************************************************
* FUNCTION
* SP2519Preview
*
* DESCRIPTION
*	This function start the sensor preview.
*
* PARAMETERS
*	*image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 SP2519Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint32 iTemp;
	kal_uint16 iStartX = 0, iStartY = 1;
	//kal_uint8  	AE_tmp;
	SENSORDB(" SP2519Preview fun start\r\n");
	SP2519_config_window(WINDOW_SIZE_UXGA); 
#ifdef AE_change
	if(setshutter)
	{
		//SP2519_config_window(WINDOW_SIZE_SVGA); 
		SP2519_AfterSnapshot(); 
		setshutter = KAL_FALSE;
	}
#endif

	if(sensor_config_data->SensorOperationMode == MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
	{
		SENSORDB(" video preview\r\n");
		SP2519_MPEG4_encode_mode = KAL_TRUE;
	}
	else
	{
		SENSORDB(" capture preview\r\n");
		SP2519_MPEG4_encode_mode = KAL_FALSE;
	}

#ifdef SP2519_Preview_debug
	SP2519_Preview_from_T_Flash();//add zch 2013-11-26
#else
	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0xcc,0x0b);
	SP2519_write_cmos_sensor(0xcd,0x10);
	SP2519_write_cmos_sensor(0xe8,0x58);//0x68 Kavie 20141225
	SP2519_write_cmos_sensor(0xec,0x68);//0x78 Kavie 20141225
	SP2519_write_cmos_sensor(0xe9,0x68);//0x78 Kavie 20141225
	SP2519_write_cmos_sensor(0xed,0x78);//0x88 Kavie 20141225
	SP2519_write_cmos_sensor(0xea,0x68);//0x78 Kavie 20141225
	SP2519_write_cmos_sensor(0xee,0x70);//0x80 Kavie 20141225
	SP2519_write_cmos_sensor(0xeb,0x58);//0x68 Kavie 20141225
	SP2519_write_cmos_sensor(0xef,0x50);//0x60 Kavie 20141225

	//SP2519_write_cmos_sensor(0xfd,0x02);
	//SP2519_write_cmos_sensor(0x0f,0x01);
	//SP2519_write_cmos_sensor(0x57,0x40);
#endif	

	image_window->GrabStartX= 1;
	image_window->GrabStartY= 1;
#ifdef Preview_SVGA
	SP2519_config_window(WINDOW_SIZE_SVGA);
	image_window->ExposureWindowWidth = 800-8; 
	image_window->ExposureWindowHeight = 600-6;
#else
	SP2519_config_window(WINDOW_SIZE_UXGA);
	image_window->ExposureWindowWidth = 1600-8; //800-8; 
	image_window->ExposureWindowHeight =1200-6; //600-6;
#endif
	// copy sensor_config_data
	memcpy(&SP2519SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));//rotation
	SENSORDB(" SP2519Preview fun end\r\n");
	return ERROR_NONE;
} /* SP2519Preview */


/*************************************************************************
* FUNCTION
*	SP2519Capture
*
* DESCRIPTION
*	This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 SP2519Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
	SENSORDB(" SP2519Capture start,%d,%d\r\n",image_window->ImageTargetWidth,image_window->ImageTargetHeight);
	//SP2519_MODE_CAPTURE=KAL_TRUE;
	flash_mode_sp2519= 1;

	//SP2519_CAP_setting();
	if ((image_window->ImageTargetWidth<=800)&&
		(image_window->ImageTargetHeight<=600))
	{    /* Less than PV Mode */
		image_window->GrabStartX = 0;
		image_window->GrabStartY = 1;
		image_window->ExposureWindowWidth= 800-8;
		image_window->ExposureWindowHeight = 600-6;
	}
	else

	{
		//kal_uint32 shutter, cap_dummy_pixels = 0; 
#ifdef AE_change
		if(!setshutter)
		{

			SP2519_config_window(WINDOW_SIZE_UXGA);
			setshutter = KAL_TRUE;
			SP2519_BeforeSnapshot(); 

			image_window->GrabStartX = 1;//iStartX;
			image_window->GrabStartY = 1;//iStartY;
			image_window->ExposureWindowWidth=1600-8 ;
			image_window->ExposureWindowHeight=1200-6;
		}	
		else
#endif
		{


			//SP2519_config_window(WINDOW_SIZE_UXGA);//add zch test(user this for SVGA)
			image_window->GrabStartX = 1;
			image_window->GrabStartY = 1;
			image_window->ExposureWindowWidth= 1600-8;
			image_window->ExposureWindowHeight = 1200-6;
		}

	}

#ifdef SP2519_Capture_debug
	SP2519_Capture_from_T_Flash();//add zch 2013-11-26
#else
	SP2519_write_cmos_sensor(0xfd,0x02);
	SP2519_write_cmos_sensor(0xcb,0x04);
	SP2519_write_cmos_sensor(0xcc,0x1a); 
	SP2519_write_cmos_sensor(0xcd,0x1a);
	SP2519_write_cmos_sensor(0xce,0x1a);

	SP2519_write_cmos_sensor(0xe8,0x54);// in outdoor    //0x58 Kavie 20141225   
	SP2519_write_cmos_sensor(0xec,0x64); //in outdoor   //0x68 Kavie 20141225     
	SP2519_write_cmos_sensor(0xe9,0x50); //in normal    //0x58 Kavie 20141225    
	SP2519_write_cmos_sensor(0xed,0x60); //in normal    //0x60 Kavie 20141225     
	SP2519_write_cmos_sensor(0xea,0x48);//in dummy     //0x50 Kavie 20141225     
	SP2519_write_cmos_sensor(0xee,0x50); //in dummy   //0x58 Kavie 20141225       
	SP2519_write_cmos_sensor(0xeb,0x40);//in lowlight    //0x48 Kavie 20141225   
	SP2519_write_cmos_sensor(0xef,0x40);
	//SP2519_write_cmos_sensor(0xfd,0x02);
	//SP2519_write_cmos_sensor(0x0f,0x00);
	//SP2519_write_cmos_sensor(0x57,0x40);
#endif
	// copy sensor_config_data
	memcpy(&SP2519SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	SENSORDB(" SP2519Capture fun end\r\n");
	return ERROR_NONE;
} /* SP2519_Capture() */



UINT32 SP2519GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	SENSORDB(" SP2519GetResolution\r\n");
	pSensorResolution->SensorFullWidth=1600-8;
	pSensorResolution->SensorFullHeight=1200-6;
#ifdef Preview_SVGA
	pSensorResolution->SensorPreviewWidth=800-8; //IMAGE_SENSOR_SVGA_WIDTH;(user this for SVGA)
	pSensorResolution->SensorPreviewHeight=600-6; //IMAGE_SIMAGE_SENSOR_SVGA_HEIGHT;(use this for SVGA)
	pSensorResolution->SensorVideoWidth=800-8; //800-8;
	pSensorResolution->SensorVideoHeight=600-6; //600-6;
#else
	pSensorResolution->SensorPreviewWidth=1600-8; //800-8;//IMAGE_SENSOR_SVGA_WIDTH;(user this for SVGA)
	pSensorResolution->SensorPreviewHeight=1200-6; //600-6;//IMAGE_SIMAGE_SENSOR_SVGA_HEIGHT;(use this for SVGA)
	pSensorResolution->SensorVideoWidth=1600-8; //800-8;
	pSensorResolution->SensorVideoHeight=1200-6; //600-6;
#endif
	return ERROR_NONE;
} /* SP2519GetResolution() */


UINT32 SP2519GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					 MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					 MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	SENSORDB(" SP2519GetInfo\r\n");
#ifdef Preview_SVGA
	pSensorInfo->SensorPreviewResolutionX=800-8; //800-8;//IMAGE_SENSOR_PV_WIDTH(use this for SVGA);
	pSensorInfo->SensorPreviewResolutionY=600-6; //600-6;//IMAGE_SENSOR_PV_HEIGHT(user this for SVGA);
#else
	pSensorInfo->SensorPreviewResolutionX=1600-8; //800-8;//IMAGE_SENSOR_PV_WIDTH(use this for SVGA);
	pSensorInfo->SensorPreviewResolutionY=1200-6; //600-6;//IMAGE_SENSOR_PV_HEIGHT(user this for SVGA);
#endif
	pSensorInfo->SensorFullResolutionX=1600-8;
	pSensorInfo->SensorFullResolutionY=1200-6;

	pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=10;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE;
	pSensorInfo->SensorResetDelayCount=1;
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_UYVY;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_HIGH;
	pSensorInfo->SensorInterruptDelayLines = 1;
	pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;

	pSensorInfo->CaptureDelayFrame = 2;
	pSensorInfo->PreviewDelayFrame = 4;
	pSensorInfo->VideoDelayFrame = 4;
	pSensorInfo->SensorMasterClockSwitch = 0;
	pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_4MA;

	switch (ScenarioId)
	{
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		//case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
		pSensorInfo->SensorClockFreq=24;
		pSensorInfo->SensorClockDividCount=	3;
		pSensorInfo->SensorClockRisingCount= 0;
		pSensorInfo->SensorClockFallingCount= 2;
		pSensorInfo->SensorPixelClockCount= 3;
		pSensorInfo->SensorDataLatchCount= 2;
		pSensorInfo->SensorGrabStartX = 1;
		pSensorInfo->SensorGrabStartY = 1;

		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	case MSDK_SCENARIO_ID_CAMERA_ZSD:
		//  case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
		pSensorInfo->SensorClockFreq=24;
		pSensorInfo->SensorClockDividCount= 3;
		pSensorInfo->SensorClockRisingCount=0;
		pSensorInfo->SensorClockFallingCount=2;
		pSensorInfo->SensorPixelClockCount=3;
		pSensorInfo->SensorDataLatchCount=2;
		pSensorInfo->SensorGrabStartX = 1;
		pSensorInfo->SensorGrabStartY = 1;
		break;
	default:
		pSensorInfo->SensorClockFreq=24;
		pSensorInfo->SensorClockDividCount= 3;
		pSensorInfo->SensorClockRisingCount=0;
		pSensorInfo->SensorClockFallingCount=2;
		pSensorInfo->SensorPixelClockCount=3;
		pSensorInfo->SensorDataLatchCount=2;
		pSensorInfo->SensorGrabStartX = 1;
		pSensorInfo->SensorGrabStartY = 1;
		break;
	}
	SP2519PixelClockDivider=pSensorInfo->SensorPixelClockCount;
	memcpy(pSensorConfigData, &SP2519SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;
} /* SP2519GetInfo() */


UINT32 SP2519Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					 MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	SENSORDB(" SP2519Control\r\n");
	switch (ScenarioId)
	{
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		//case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
		SP2519Preview(pImageWindow, pSensorConfigData);
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	case MSDK_SCENARIO_ID_CAMERA_ZSD:
		//case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
		SP2519Capture(pImageWindow, pSensorConfigData);
		break;
	}


	return TRUE;
}	/* SP2519Control() */

BOOL SP2519_set_param_wb(UINT16 para)
{
	SENSORDB(" SP2519_set_param_wb\r\n");
	switch (para)
	{
	case AWB_MODE_OFF:
		//SP2519_write_cmos_sensor(0xfd,0x00);
		// SP2519_write_cmos_sensor(0x32,0x05);
		break; 					
	case AWB_MODE_AUTO:
		SP2519_write_cmos_sensor(0xfd,0x02);
		SP2519_write_cmos_sensor(0x26,0xc1);
		SP2519_write_cmos_sensor(0x27,0x88);	   
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0x32,0x15);
		break;
	case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0x32,0x05);
		SP2519_write_cmos_sensor(0xfd,0x02);
		SP2519_write_cmos_sensor(0x26,0xe2);
		SP2519_write_cmos_sensor(0x27,0x82);
		SP2519_write_cmos_sensor(0xfd,0x00);
		break;
	case AWB_MODE_DAYLIGHT: //sunny
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0x32,0x05);
		SP2519_write_cmos_sensor(0xfd,0x02);
		SP2519_write_cmos_sensor(0x26,0xc1);
		SP2519_write_cmos_sensor(0x27,0x88);
		SP2519_write_cmos_sensor(0xfd,0x00);
		break;
	case AWB_MODE_INCANDESCENT: //office
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0x32,0x05);
		SP2519_write_cmos_sensor(0xfd,0x02);
		SP2519_write_cmos_sensor(0x26,0x7b);
		SP2519_write_cmos_sensor(0x27,0xd3);
		SP2519_write_cmos_sensor(0xfd,0x00);
		break;
	case AWB_MODE_TUNGSTEN: //home
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0x32,0x05);
		SP2519_write_cmos_sensor(0xfd,0x02);
		SP2519_write_cmos_sensor(0x26,0xae);
		SP2519_write_cmos_sensor(0x27,0xcc);
		SP2519_write_cmos_sensor(0xfd,0x00);
		break;
	case AWB_MODE_FLUORESCENT:
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0x32,0x05);
		SP2519_write_cmos_sensor(0xfd,0x02);
		SP2519_write_cmos_sensor(0x26,0xb4);
		SP2519_write_cmos_sensor(0x27,0xc4);
		SP2519_write_cmos_sensor(0xfd,0x00);
	default:
		return FALSE;
	}  
	return TRUE;
} /* SP2519_set_param_wb */


BOOL SP2519_set_param_effect(UINT16 para)
{
	SENSORDB(" SP2519_set_param_effect\r\n");
	switch (para)
	{
	case MEFFECT_OFF:
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0x66,0x00);
		SP2519_write_cmos_sensor(0x67,0x80);
		SP2519_write_cmos_sensor(0x68,0x80);
		SP2519_write_cmos_sensor(0xdb,0x00);
		SP2519_write_cmos_sensor(0x34,0xc7);
		SP2519_write_cmos_sensor(0xfd,0x02);
		SP2519_write_cmos_sensor(0x14,0x00);
		break;
	case MEFFECT_SEPIA:
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0x66,0x10);
		SP2519_write_cmos_sensor(0x67,0x98);
		SP2519_write_cmos_sensor(0x68,0x58);
		SP2519_write_cmos_sensor(0xdb,0x00);
		SP2519_write_cmos_sensor(0x34,0xc7);
		SP2519_write_cmos_sensor(0xfd,0x02);
		SP2519_write_cmos_sensor(0x14,0x00);

		break;
	case MEFFECT_NEGATIVE://----datasheet
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0x66,0x08);
		SP2519_write_cmos_sensor(0x67,0x80);
		SP2519_write_cmos_sensor(0x68,0x80);
		SP2519_write_cmos_sensor(0xdb,0x00);
		SP2519_write_cmos_sensor(0x34,0xc7);
		SP2519_write_cmos_sensor(0xfd,0x02);
		SP2519_write_cmos_sensor(0x14,0x00);
		break;
	case MEFFECT_SEPIAGREEN://----datasheet aqua
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0x66,0x10);
		SP2519_write_cmos_sensor(0x67,0x50);
		SP2519_write_cmos_sensor(0x68,0x50);
		SP2519_write_cmos_sensor(0xdb,0x00);
		SP2519_write_cmos_sensor(0x34,0xc7);
		SP2519_write_cmos_sensor(0xfd,0x02);
		SP2519_write_cmos_sensor(0x14,0x00);
		break;
	case MEFFECT_SEPIABLUE:
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0x66,0x10);
		SP2519_write_cmos_sensor(0x67,0x80);
		SP2519_write_cmos_sensor(0x68,0xb0);
		SP2519_write_cmos_sensor(0xdb,0x00);
		SP2519_write_cmos_sensor(0x34,0xc7);
		SP2519_write_cmos_sensor(0xfd,0x02);
		SP2519_write_cmos_sensor(0x14,0x00);

		break;
	case MEFFECT_MONO: //----datasheet black & white
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0x66,0x20);
		SP2519_write_cmos_sensor(0x67,0x80);
		SP2519_write_cmos_sensor(0x68,0x80);
		SP2519_write_cmos_sensor(0xdb,0x00);
		SP2519_write_cmos_sensor(0x34,0xc7);
		SP2519_write_cmos_sensor(0xfd,0x02);
		SP2519_write_cmos_sensor(0x14,0x00);
		break;
	default:
		return FALSE;
	}
	return TRUE;
} /* SP2519_set_param_effect */

UINT8 index = 1;
BOOL SP2519_set_param_banding(UINT16 para)
{
	//UINT16 buffer = 0;
	SENSORDB("SP2519_set_param_banding para = %d ---- index = %d\r\n",para,index); 
	SENSORDB("SP2519_set_param_banding ---- SP2519_MPEG4_encode_mode = %d\r\n",SP2519_MPEG4_encode_mode);
	switch (para)
	{
	case AE_FLICKER_MODE_50HZ:
		SP2519_CAM_BANDING_50HZ = KAL_TRUE;
		break;
	case AE_FLICKER_MODE_60HZ:
		SP2519_CAM_BANDING_50HZ = KAL_FALSE;
		break;
	default:
		SP2519_CAM_BANDING_50HZ = KAL_TRUE;
		break;//	return TRUE;
	}

	return TRUE;
} /* SP2519_set_param_banding */


BOOL SP2519_set_param_exposure(UINT16 para)
{
	SENSORDB(" SP2519_set_param_exposure\r\n");
	switch (para)
	{
	case AE_EV_COMP_n30:              /* EV -1.5 */
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0xdb,0xd0);
		break;
	case AE_EV_COMP_n20:              /* EV -1 */
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0xdb,0xe0);
		break;
	case AE_EV_COMP_n10:              /* EV -0.5 */
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0xdb,0xf0);
		break;
	case AE_EV_COMP_00:                /* EV 0 */
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0xdb,0x00);
		break;
	case AE_EV_COMP_10:              /* EV +0.5 */
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0xdb,0x10);
		break;
	case AE_EV_COMP_20:              /* EV +1 */
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0xdb,0x20);
		break;
	case AE_EV_COMP_30:              /* EV +1.5 */
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0xdb,0x30);
		break;
	default:
		return FALSE;
	}
	return TRUE;
} /* SP2519_set_param_exposure */

BOOL SP2519_set_iso(UINT16 para)
{

	switch (para)
	{
	case AE_ISO_100:
		//ISO100
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0x04,0x80);
		SP2519_write_cmos_sensor(0xfd,0x00);		
		break;
	case AE_ISO_200:
		//ISO200
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0x04,0xA0);				
		SP2519_write_cmos_sensor(0xfd,0x00);		
		break;
	case AE_ISO_400:
		//ISO400
		SP2519_write_cmos_sensor(0xfd,0x01);
		SP2519_write_cmos_sensor(0x04,0xb0);				
		SP2519_write_cmos_sensor(0xfd,0x00);				
		break;
	case AE_ISO_800:
		//ISO800
		SP2519_write_cmos_sensor(0xfd,0x01); 
		SP2519_write_cmos_sensor(0x04,0xd0);
		SP2519_write_cmos_sensor(0xfd,0x00);				
		break;
	case AE_ISO_1600:
		//ISO1600
		SP2519_write_cmos_sensor(0xfd,0x01); 
		SP2519_write_cmos_sensor(0x04,0xe0);
		SP2519_write_cmos_sensor(0xfd,0x00);				
		break;				
	default:
	case AE_ISO_AUTO://
		//Auto/
		SP2519_write_cmos_sensor(0xfd,0x01); 
		SP2519_write_cmos_sensor(0x04,0xc0);
		SP2519_write_cmos_sensor(0xfd,0x00);				
		break;
	}
	return;

}


UINT32 SP2519YUVSensorSetting(FEATURE_ID iCmd, UINT16 iPara)
{

#ifdef SP2519_DEBUG_SENSOR
	return TRUE;
#endif
	SENSORDB("SP2519YUVSensorSetting\r\n");
	switch (iCmd) 
	{
	case FID_SCENE_MODE:	    
		if (iPara == SCENE_MODE_OFF)
		{
			SP2519_night_mode(0); 
		}
		else if (iPara == SCENE_MODE_NIGHTSCENE)
		{
			SP2519_night_mode(1); 
		}	    
		break; 	    
	case FID_AWB_MODE:
		SP2519_set_param_wb(iPara);
		break;
	case FID_COLOR_EFFECT:
		SP2519_set_param_effect(iPara);
		break;
	case FID_AE_EV:
		SP2519_set_param_exposure(iPara);
		break;
	case FID_AE_ISO:
		SP2519_set_iso(iPara);
		break;		
	case FID_AE_FLICKER:
		SP2519_set_param_banding(iPara);
		SP2519_night_mode(SP2519_CAM_Nightmode); 
		break;
	default:
		break;
	}
	return TRUE;
}
/* SP2519YUVSensorSetting */

//BEGIN: add by fangjie for show camera info in MINISW screen, FR803765
extern char Back_Camera_Name[256];
//END: add by fangjie for show camera info in MINISW screen, FR803765
UINT32 sp2519_get_sensor_id(UINT32 *sensorID) 
{
	volatile signed char i;
	kal_uint16 sensor_id=0;
	SENSORDB(" SP2519GetSensorID ");
	//SENSORDB("xieyang in GPIO_CAMERA_CMPDN_PIN=%d,GPIO_CAMERA_CMPDN1_PIN=%d", 
	//	mt_get_gpio_out(GPIO_CAMERA_CMPDN_PIN),mt_get_gpio_out(GPIO_CAMERA_CMPDN1_PIN));

	for(i=0;i<3;i++)
	{
		SP2519_write_cmos_sensor(0xfd, 0x00); 
		sensor_id = SP2519_read_cmos_sensor(0x02);
		SENSORDB("%s sensor_id=%d\n", __func__, sensor_id);

		if (sensor_id == SP2519_SENSOR_ID)
		{
			break;
		}
	}

	if(sensor_id != SP2519_SENSOR_ID)
	{
		*sensorID = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	else
	{
		*sensorID = SP2519_SENSOR_ID;
	}

	//BEGIN: add by fangjie for show camera info in MINISW screen, FR803765
	sprintf(Back_Camera_Name,"Vender: Sunrise \nSensor ID:  0x%X \nSensor name:  SP2519 \nResolution:  2M ",SP2519_SENSOR_ID);
	//END: add by fangjie for show camera info in MINISW screen, FR803765

	return ERROR_NONE;
}


UINT32 SP2519FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
	UINT32 SP2519SensorRegNumber;
	UINT32 i;
	SENSORDB("SP2519FeatureControl.---FeatureId = %d\r\n",FeatureId);
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

	RETAILMSG(1, (_T("gaiyang SP2519FeatureControl FeatureId=%d\r\n"), FeatureId));

	switch (FeatureId)
	{
	case SENSOR_FEATURE_GET_RESOLUTION:
		*pFeatureReturnPara16++=IMAGE_SENSOR_UXGA_WIDTH;//UXGA_PERIOD_PIXEL_NUMS;
		*pFeatureReturnPara16=IMAGE_SENSOR_UXGA_HEIGHT;//UXGA_PERIOD_LINE_NUMS;
		*pFeatureParaLen=4;
		break;
	case SENSOR_FEATURE_GET_PERIOD:
		*pFeatureReturnPara16++=(SVGA_PERIOD_PIXEL_NUMS)+SP2519_dummy_pixels;
		*pFeatureReturnPara16=(SVGA_PERIOD_LINE_NUMS)+SP2519_dummy_lines;
		*pFeatureParaLen=4;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
		*pFeatureReturnPara32 = SP2519_g_fPV_PCLK;
		*pFeatureParaLen=4;
		break;
	case SENSOR_FEATURE_SET_ESHUTTER:
		break;
	case SENSOR_FEATURE_SET_NIGHTMODE:
#ifndef SP2519_DEBUG_SENSOR		
		SP2519_night_mode((BOOL) *pFeatureData16);
#endif
		break;
	case SENSOR_FEATURE_SET_GAIN:
	case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
	case SENSOR_FEATURE_GET_TRIGGER_FLASHLIGHT_INFO:
		SENSORDB("sp2519 GET_TRIGGER_FLASHLIGHT_INFO\n");
		SP2519YUV_FlashTriggerCheck(pFeatureData32);
		break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		SP2519_isp_master_clock=*pFeatureData32;
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		SP2519_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		pSensorRegData->RegData = SP2519_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
	case SENSOR_FEATURE_GET_CONFIG_PARA:
		memcpy(pSensorConfigData, &SP2519SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
		*pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
		break;
	case SENSOR_FEATURE_SET_CCT_REGISTER:
	case SENSOR_FEATURE_GET_CCT_REGISTER:
	case SENSOR_FEATURE_SET_ENG_REGISTER:
	case SENSOR_FEATURE_GET_ENG_REGISTER:
	case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
	case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
	case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
	case SENSOR_FEATURE_GET_GROUP_COUNT:
	case SENSOR_FEATURE_GET_GROUP_INFO:
	case SENSOR_FEATURE_GET_ITEM_INFO:
	case SENSOR_FEATURE_SET_ITEM_INFO:
	case SENSOR_FEATURE_GET_ENG_INFO:
		break;
	case SENSOR_FEATURE_CHECK_SENSOR_ID:
		sp2519_get_sensor_id(pFeatureReturnPara32); 
		break; 
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
		// if EEPROM does not exist in camera module.
		*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
		*pFeatureParaLen=4;
		break;
	case SENSOR_FEATURE_SET_YUV_CMD:
		SP2519YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
		break;
	default:
		break;
	}
	return ERROR_NONE;
}	/* SP2519FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncSP2519YUV=
{
	SP2519Open,
	SP2519GetInfo,
	SP2519GetResolution,
	SP2519FeatureControl,
	SP2519Control,
	SP2519Close
};


UINT32 SP2519_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	SENSORDB(" SP2519_YUV_SensorInit\r\n");
	if (pfFunc!=NULL)
	{
		SENSORDB(" SP2519_YUV_SensorInit fun_config success\r\n");
		*pfFunc=&SensorFuncSP2519YUV;
	}
	return ERROR_NONE;
} /* SensorInit() */

