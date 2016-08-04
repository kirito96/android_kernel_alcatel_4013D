/*****************************************************************************
*
* Filename:
* ---------
*   sensor.c
*
* Project:
* --------
*   ALPS
*
* Description:
* ------------
*   Source code of Sensor driver
*
*
* Author:
* -------
*   Anyuan Huang (MTK70663)
*
*============================================================================
*             HISTORY
* Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
*------------------------------------------------------------------------------
* $Revision:$
* $Modtime:$
* $Log:$
*
*
*
*------------------------------------------------------------------------------
* Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
*============================================================================
****************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <asm/io.h>
#include <asm/system.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "hi257yuv_Sensor.h"
#include "hi257yuv_Camera_Sensor_para.h"
#include "hi257yuv_CameraCustomized.h" 

#define HI257YUV_DEBUG
#ifdef HI257YUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

struct
{
	kal_bool    NightMode;
	kal_bool   VideoMode;
	kal_uint8   ZoomFactor; /* Zoom Index */
	kal_uint16  Banding;
	kal_uint32  PvShutter;
	kal_uint32  PvDummyPixels;
	kal_uint32  PvDummyLines;
	kal_uint32  CapDummyPixels;
	kal_uint32  CapDummyLines;
	kal_uint32  PvOpClk;
	kal_uint32  CapOpClk;

	/* Video frame rate 300 means 30.0fps. Unit Multiple 10. */
	kal_uint32  MaxFrameRate; 
	kal_uint32  MiniFrameRate; 
	/* Sensor Register backup. */
	kal_uint8   VDOCTL2; /* P0.0x11. */
	kal_uint8   ISPCTL3; /* P10.0x12. */
	kal_uint8   AECTL1;  /* P20.0x10. */
	kal_uint8   AWBCTL1; /* P22.0x10. */
} HI257Status;

static DEFINE_SPINLOCK(hi257_drv_lock);

static int CAPTURE_FLAG = 0;
static int CAPTURE_FLAGA = 0;
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

#define Sleep(ms) mdelay(ms)

kal_uint16 HI257WriteCmosSensor(kal_uint32 Addr, kal_uint32 Para)
{
	char pSendCmd[2] = {(char)(Addr & 0xFF) ,(char)(Para & 0xFF)};

	iWriteRegI2C(pSendCmd , 2,HI257_WRITE_ID);
}

kal_uint16 HI257ReadCmosSensor(kal_uint32 Addr)
{
	char pGetByte=0;
	char pSendCmd = (char)(Addr & 0xFF);

	iReadRegI2C(&pSendCmd , 1, &pGetByte,1,HI257_WRITE_ID);

	return pGetByte;
}

void HI257SetPage(kal_uint8 Page)
{
	HI257WriteCmosSensor(0x03, Page);
}
#define FLASH_BV_THRESHOLD 0x30
static void HI257_FlashTriggerCheck(unsigned int *pFeatureReturnPara32)
{
	unsigned int NormBr;

	HI257WriteCmosSensor(0x03,0x20);
	NormBr  = HI257ReadCmosSensor(0xd3);

	SENSORDB(" HI257_FlashTriggerCheck NormBr=0x%x\n",NormBr);

	if (NormBr > FLASH_BV_THRESHOLD)
	{
		*pFeatureReturnPara32 = FALSE;
		return;
	}

	*pFeatureReturnPara32 = TRUE;
	return;
}

//#define HI257_LOAD_FROM_T_FLASH
#ifdef HI257_LOAD_FROM_T_FLASH

static kal_uint8 fromsd = 0;
//kal_uint16 HI257_write_cmos_sensor(kal_uint8 addr, kal_uint8 para);

#define HI257_OP_CODE_INI		0x00		/* Initial value. */
#define HI257_OP_CODE_REG		0x01		/* Register */
#define HI257_OP_CODE_DLY		0x02		/* Delay */
#define HI257_OP_CODE_END		0x03		/* End of initial setting. */

typedef struct
{
	u16 init_reg;
	u16 init_val;	/* Save the register value and delay tick */
	u8 op_code;		/* 0 - Initial value, 1 - Register, 2 - Delay, 3 - End of setting. */
} HI257_initial_set_struct;

static	HI257_initial_set_struct HI257_Init_Reg[2000];

static u32 strtol(const char *nptr, u8 base)
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

static u8 Hi257_Initialize_from_T_Flash()
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
	static u8 data_buff[50*1024] ;
	fp = filp_open("/sdcard/hi257_sd.dat", O_RDONLY , 0); 
	if (IS_ERR(fp)) { 
		printk("create file error\n"); 
		return 2;//-1; 
	} 
	else
		printk("Hi256_Initialize_from_T_Flash Open File Success\n");

	fs = get_fs(); 
	set_fs(KERNEL_DS); 

	file_size = vfs_llseek(fp, 0, SEEK_END);
	vfs_read(fp, data_buff, file_size, &pos); 
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
			HI257_Init_Reg[i].op_code = HI257_OP_CODE_REG;

			HI257_Init_Reg[i].init_reg = strtol((const char *)curr_ptr, 16);
			curr_ptr += /*6*/5 ;	/* Skip "00, 0x" */

			HI257_Init_Reg[i].init_val = strtol((const char *)curr_ptr, 16);
			curr_ptr += 4;	/* Skip "00);" */
		}
		else									/* DLY */
		{
			/* Need add delay for this setting. */
			curr_ptr += 4;	
			HI257_Init_Reg[i].op_code = HI257_OP_CODE_DLY;

			HI257_Init_Reg[i].init_reg = 0xFF;
			HI257_Init_Reg[i].init_val = strtol((const char *)curr_ptr,  10);	/* Get the delay ticks, the delay should less then 50 */
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
	HI257_Init_Reg[i].op_code = HI257_OP_CODE_END;
	HI257_Init_Reg[i].init_reg = 0xFF;
	HI257_Init_Reg[i].init_val = 0xFF;
	i++;
	//for (j=0; j<i; j++)
	//printk(" %x  ==  %x\n",SP2528_Init_Reg[j].init_reg, SP2528_Init_Reg[j].init_val);

	/* Start apply the initial setting to sensor. */
#if 1
	for (j=0; j<i; j++)
	{
		if (HI257_Init_Reg[j].op_code == HI257_OP_CODE_END)	/* End of the setting. */
		{
			break ;
		}
		else if (HI257_Init_Reg[j].op_code == HI257_OP_CODE_DLY)
		{
			msleep(HI257_Init_Reg[j].init_val);		/* Delay */
		}
		else if (HI257_Init_Reg[j].op_code == HI257_OP_CODE_REG)
		{
			HI257WriteCmosSensor((kal_uint8)HI257_Init_Reg[j].init_reg, (kal_uint8)HI257_Init_Reg[j].init_val);
		}
		else
		{
			printk("REG ERROR!\n");
		}
	}
#endif
	printk("3\n");
	return 1;	
}
#endif

void HI257InitSetting(void)
{
	HI257WriteCmosSensor(0x01, 0x01); //sleep on
	HI257WriteCmosSensor(0x01, 0x01);//sleep on
	HI257WriteCmosSensor(0x01, 0x03);//sleep off
	HI257WriteCmosSensor(0x01, 0x01);//sleep on

	HI257WriteCmosSensor(0x03, 0x00);//Dummy 750us
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);

	HI257WriteCmosSensor(0x08, 0x00);

    /*Begin ersen.shang change drive strength form 0x44 to 0x33 for pr969579*/
	HI257WriteCmosSensor(0x09, 0x33);	// pclk drive confrol 
    /*End   ersen.shang change drive strength form 0x44 to 0x33 for pr969579*/

	HI257WriteCmosSensor(0x0a, 0x07);	// pad strength = max

	HI257WriteCmosSensor(0x0e, 0x03);//PLL On
	HI257WriteCmosSensor(0x0e, 0x73);//PLLx2

	HI257WriteCmosSensor(0x03, 0x00);//Dummy 750us
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);

	///// PAGE 20 /////
	HI257WriteCmosSensor(0x03, 0x20);//page 20
	HI257WriteCmosSensor(0x10, 0x1c);//AE off 50hz

	///// PAGE 22 /////
	HI257WriteCmosSensor(0x03, 0x22);//page 22
	HI257WriteCmosSensor(0x10, 0x69);//AWB off

	///// Initial Start /////
	///// PAGE 0 Start /////
	HI257WriteCmosSensor(0x03, 0x00);//page 0
	HI257WriteCmosSensor(0x10, 0x90);
	HI257WriteCmosSensor(0x11, 0x93); //Windowing On + 1Frame Skip
	HI257WriteCmosSensor(0x12, 0x04); //Rinsing edge 0x04 // Falling edge 0x00
	HI257WriteCmosSensor(0x14, 0x05);

	HI257WriteCmosSensor(0x20, 0x00); //Row H
	HI257WriteCmosSensor(0x21, 0x07); //Row L  
	HI257WriteCmosSensor(0x22, 0x00); //Col H
	HI257WriteCmosSensor(0x23, 0x07); //Col L  

	HI257WriteCmosSensor(0x24, 0x04); //Window height_H //= 1200
	HI257WriteCmosSensor(0x25, 0xb0); //Window height_L //
	HI257WriteCmosSensor(0x26, 0x06); //Window width_H  //= 1600
	HI257WriteCmosSensor(0x27, 0x40); //Window wight_L

	HI257WriteCmosSensor(0x40, 0x01); //Hblank_376
	HI257WriteCmosSensor(0x41, 0x04);
	HI257WriteCmosSensor(0x42, 0x00); //Vblank
	HI257WriteCmosSensor(0x43, 0x14); //Flick Stop

	HI257WriteCmosSensor(0x50, 0x00);//Test Pattern

	///// BLC /////
	HI257WriteCmosSensor(0x80, 0x2e);
	HI257WriteCmosSensor(0x81, 0x7e);
	HI257WriteCmosSensor(0x82, 0x90);
	HI257WriteCmosSensor(0x83, 0x00);
	HI257WriteCmosSensor(0x84, 0x0c);
	HI257WriteCmosSensor(0x85, 0x00);
	HI257WriteCmosSensor(0x86, 0x00);
	HI257WriteCmosSensor(0x87, 0x0f);
	HI257WriteCmosSensor(0x88, 0x34);
	HI257WriteCmosSensor(0x8a, 0x0b);
	HI257WriteCmosSensor(0x8e, 0x80);//Pga Blc Hold

	HI257WriteCmosSensor(0x90, 0x0a); //BLC_TIME_TH_ON
	HI257WriteCmosSensor(0x91, 0x0a); //BLC_TIME_TH_OFF
	HI257WriteCmosSensor(0x92, 0x58); //BLC_AG_TH_ON
	HI257WriteCmosSensor(0x93, 0x50); //BLC_AG_TH_OFF
	HI257WriteCmosSensor(0x96, 0xdc); //BLC Outdoor Th On
	HI257WriteCmosSensor(0x97, 0xfe); //BLC Outdoor Th Off
	HI257WriteCmosSensor(0x98, 0x38);

	//OutDoor  BLC
	HI257WriteCmosSensor(0x99, 0x43);//R,Gr,B,Gb Offset

	//Dark BLC
	HI257WriteCmosSensor(0xa0, 0x00);//R,Gr,B,Gb Offset

	//Normal BLC
	HI257WriteCmosSensor(0xa8, 0x43);//R,Gr,B,Gb Offset
	///// PAGE 0 END /////

	///// PAGE 2 START /////
	HI257WriteCmosSensor(0x03, 0x02);
	HI257WriteCmosSensor(0x10, 0x00);
	HI257WriteCmosSensor(0x13, 0x00);
	HI257WriteCmosSensor(0x14, 0x00);
	HI257WriteCmosSensor(0x18, 0xcc);
	HI257WriteCmosSensor(0x19, 0x01);// pmos switch on (for cfpn)
	HI257WriteCmosSensor(0x1A, 0x39);
	HI257WriteCmosSensor(0x1B, 0x00);
	HI257WriteCmosSensor(0x1C, 0x1a); // for ncp
	HI257WriteCmosSensor(0x1D, 0x14); // for ncp
	HI257WriteCmosSensor(0x1E, 0x30); // for ncp
	HI257WriteCmosSensor(0x1F, 0x10);

	HI257WriteCmosSensor(0x20, 0x77);
	HI257WriteCmosSensor(0x21, 0xde);
	HI257WriteCmosSensor(0x22, 0xa7);
	HI257WriteCmosSensor(0x23, 0x30);
	HI257WriteCmosSensor(0x24, 0x77);
	HI257WriteCmosSensor(0x25, 0x10);
	HI257WriteCmosSensor(0x26, 0x10);
	HI257WriteCmosSensor(0x27, 0x3c);
	HI257WriteCmosSensor(0x2b, 0x80);
	HI257WriteCmosSensor(0x2c, 0x02);
	HI257WriteCmosSensor(0x2d, 0x58);
	HI257WriteCmosSensor(0x2e, 0xde);
	HI257WriteCmosSensor(0x2f, 0xa7);

	HI257WriteCmosSensor(0x30, 0x00);
	HI257WriteCmosSensor(0x31, 0x99);
	HI257WriteCmosSensor(0x32, 0x00);
	HI257WriteCmosSensor(0x33, 0x00);
	HI257WriteCmosSensor(0x34, 0x22);
	HI257WriteCmosSensor(0x36, 0x75);
	HI257WriteCmosSensor(0x38, 0x88);
	HI257WriteCmosSensor(0x39, 0x88);
	HI257WriteCmosSensor(0x3d, 0x03);
	HI257WriteCmosSensor(0x3f, 0x02);

	HI257WriteCmosSensor(0x49, 0xd1);
	HI257WriteCmosSensor(0x4a, 0x10);

	HI257WriteCmosSensor(0x50, 0x21);
	HI257WriteCmosSensor(0x53, 0xb1);
	HI257WriteCmosSensor(0x54, 0x10);
	HI257WriteCmosSensor(0x55, 0x1c);// for ncp
	HI257WriteCmosSensor(0x56, 0x11);
	HI257WriteCmosSensor(0x5d, 0xa2);
	HI257WriteCmosSensor(0x5e, 0x5a);
	HI257WriteCmosSensor(0x5d, 0xa2);
	HI257WriteCmosSensor(0x5e, 0x5a);

	HI257WriteCmosSensor(0x60, 0x87);
	HI257WriteCmosSensor(0x61, 0x98);
	HI257WriteCmosSensor(0x62, 0x88);
	HI257WriteCmosSensor(0x63, 0x96);
	HI257WriteCmosSensor(0x64, 0x88);
	HI257WriteCmosSensor(0x65, 0x96);
	HI257WriteCmosSensor(0x67, 0x3f);
	HI257WriteCmosSensor(0x68, 0x3f);
	HI257WriteCmosSensor(0x69, 0x3f);

	HI257WriteCmosSensor(0x72, 0x89);
	HI257WriteCmosSensor(0x73, 0x95);
	HI257WriteCmosSensor(0x74, 0x89);
	HI257WriteCmosSensor(0x75, 0x95);
	HI257WriteCmosSensor(0x7C, 0x84);
	HI257WriteCmosSensor(0x7D, 0xaf);

	HI257WriteCmosSensor(0x80, 0x01);
	HI257WriteCmosSensor(0x81, 0x7a);
	HI257WriteCmosSensor(0x82, 0x13);
	HI257WriteCmosSensor(0x83, 0x24);
	HI257WriteCmosSensor(0x84, 0x78);
	HI257WriteCmosSensor(0x85, 0x7c);

	HI257WriteCmosSensor(0x92, 0x44);
	HI257WriteCmosSensor(0x93, 0x59);
	HI257WriteCmosSensor(0x94, 0x78);
	HI257WriteCmosSensor(0x95, 0x7c);

	HI257WriteCmosSensor(0xA0, 0x02);
	HI257WriteCmosSensor(0xA1, 0x74);
	HI257WriteCmosSensor(0xA4, 0x74);
	HI257WriteCmosSensor(0xA5, 0x02);
	HI257WriteCmosSensor(0xA8, 0x85);
	HI257WriteCmosSensor(0xA9, 0x8c);
	HI257WriteCmosSensor(0xAC, 0x10);
	HI257WriteCmosSensor(0xAD, 0x16);

	HI257WriteCmosSensor(0xB0, 0x99);
	HI257WriteCmosSensor(0xB1, 0xa3);
	HI257WriteCmosSensor(0xB4, 0x9b);
	HI257WriteCmosSensor(0xB5, 0xa2);
	HI257WriteCmosSensor(0xB8, 0x9b);
	HI257WriteCmosSensor(0xB9, 0x9f);
	HI257WriteCmosSensor(0xBC, 0x9b);
	HI257WriteCmosSensor(0xBD, 0x9f);

	HI257WriteCmosSensor(0xc4, 0x29);
	HI257WriteCmosSensor(0xc5, 0x40);
	HI257WriteCmosSensor(0xc6, 0x5c);
	HI257WriteCmosSensor(0xc7, 0x72);
	HI257WriteCmosSensor(0xc8, 0x2a);
	HI257WriteCmosSensor(0xc9, 0x3f);
	HI257WriteCmosSensor(0xcc, 0x5d);
	HI257WriteCmosSensor(0xcd, 0x71);

	HI257WriteCmosSensor(0xd0, 0x10);
	HI257WriteCmosSensor(0xd1, 0x14);
	HI257WriteCmosSensor(0xd2, 0x20);
	HI257WriteCmosSensor(0xd3, 0x00);
	HI257WriteCmosSensor(0xd4, 0x0a); //DCDC_TIME_TH_ON
	HI257WriteCmosSensor(0xd5, 0x0a); //DCDC_TIME_TH_OFF 
	HI257WriteCmosSensor(0xd6, 0x58); //DCDC_AG_TH_ON
	HI257WriteCmosSensor(0xd7, 0x50); //DCDC_AG_TH_OFF
	HI257WriteCmosSensor(0xdc, 0x00);
	HI257WriteCmosSensor(0xdd, 0xa3);
	HI257WriteCmosSensor(0xde, 0x00);
	HI257WriteCmosSensor(0xdf, 0x84);

	HI257WriteCmosSensor(0xe0, 0xa4);
	HI257WriteCmosSensor(0xe1, 0xa4);
	HI257WriteCmosSensor(0xe2, 0xa4);
	HI257WriteCmosSensor(0xe3, 0xa4);
	HI257WriteCmosSensor(0xe4, 0xa4);
	HI257WriteCmosSensor(0xe5, 0x01);
	HI257WriteCmosSensor(0xe8, 0x00);
	HI257WriteCmosSensor(0xe9, 0x00);
	HI257WriteCmosSensor(0xea, 0x77);

	HI257WriteCmosSensor(0xF0, 0x00);
	HI257WriteCmosSensor(0xF1, 0x00);
	HI257WriteCmosSensor(0xF2, 0x00);

	///// PAGE 2 END /////
	///// PAGE 10 START /////
	HI257WriteCmosSensor(0x03, 0x10);//page 10
	HI257WriteCmosSensor(0x10, 0x00); //S2D enable _ YUYV Order
	HI257WriteCmosSensor(0x11, 0x03);
	HI257WriteCmosSensor(0x12, 0xf0);
	HI257WriteCmosSensor(0x13, 0x01);

	HI257WriteCmosSensor(0x20, 0x00);
	HI257WriteCmosSensor(0x21, 0x40);
	HI257WriteCmosSensor(0x22, 0x0f);
	HI257WriteCmosSensor(0x24, 0x20);
	HI257WriteCmosSensor(0x25, 0x10);
	HI257WriteCmosSensor(0x26, 0x01);
	HI257WriteCmosSensor(0x27, 0x02);
	HI257WriteCmosSensor(0x28, 0x11);

	HI257WriteCmosSensor(0x40, 0x88);
	HI257WriteCmosSensor(0x41, 0x00);//D-YOffset Th
	HI257WriteCmosSensor(0x42, 0x04); //Cb Offset
	HI257WriteCmosSensor(0x43, 0x04);//Cr Offset
	HI257WriteCmosSensor(0x44, 0x80);
	HI257WriteCmosSensor(0x45, 0x80);
	HI257WriteCmosSensor(0x46, 0xf0);
	HI257WriteCmosSensor(0x48, 0x80);
	HI257WriteCmosSensor(0x4a, 0x80);

	HI257WriteCmosSensor(0x50, 0xa0);//D-YOffset AG

	HI257WriteCmosSensor(0x60, 0x4f);
	HI257WriteCmosSensor(0x61, 0x70); //Sat B
	HI257WriteCmosSensor(0x62, 0x72); //Sat R
	HI257WriteCmosSensor(0x63, 0x40); //Auto-De Color

	HI257WriteCmosSensor(0x66, 0x42);
	HI257WriteCmosSensor(0x67, 0x22);//23

	HI257WriteCmosSensor(0x6a, 0x18); //White Protection Offset Dark/Indoor
	HI257WriteCmosSensor(0x74, 0x08); //White Protection Offset Outdoor
	HI257WriteCmosSensor(0x76, 0x01); //White Protection Enable  01
	///// PAGE 10 END /////


	///// PAGE 11 START /////
	HI257WriteCmosSensor(0x03, 0x11);//page 11

	//LPF Auto Control
	HI257WriteCmosSensor(0x20, 0x00);
	HI257WriteCmosSensor(0x21, 0x00);
	HI257WriteCmosSensor(0x26, 0x8e);// Double_AG
	HI257WriteCmosSensor(0x27, 0x8d);// Double_AG
	HI257WriteCmosSensor(0x28, 0x0f);
	HI257WriteCmosSensor(0x29, 0x10);
	HI257WriteCmosSensor(0x2b, 0x30);
	HI257WriteCmosSensor(0x2c, 0x32);

	//GBGR
	HI257WriteCmosSensor(0x70, 0x2b);
	HI257WriteCmosSensor(0x74, 0x30);
	HI257WriteCmosSensor(0x75, 0x18);
	HI257WriteCmosSensor(0x76, 0x30);
	HI257WriteCmosSensor(0x77, 0xff);
	HI257WriteCmosSensor(0x78, 0xa0);
	HI257WriteCmosSensor(0x79, 0xff);//Dark GbGr Th
	HI257WriteCmosSensor(0x7a, 0x30);
	HI257WriteCmosSensor(0x7b, 0x20);
	HI257WriteCmosSensor(0x7c, 0xf4);//Dark Dy Th B[7:4]
	///// PAGE 11 END /////

	///// PAGE 12 START /////
	HI257WriteCmosSensor(0x03, 0x12);//page 11

	//YC2D
	HI257WriteCmosSensor(0x10, 0x03);//Y DPC Enable
	HI257WriteCmosSensor(0x11, 0x08);//
	HI257WriteCmosSensor(0x12, 0x10);//0x30 -> 0x10
	HI257WriteCmosSensor(0x20, 0x53);//Y_lpf_enable
	HI257WriteCmosSensor(0x21, 0x03);//C_lpf_enable_on
	HI257WriteCmosSensor(0x22, 0xe6);//YC2D_CrCbY_Dy

	HI257WriteCmosSensor(0x23, 0x14); //Outdoor Dy Th
	HI257WriteCmosSensor(0x24, 0x20); //Indoor Dy Th // For reso Limit 0x20
	HI257WriteCmosSensor(0x25, 0x28); //Dark Dy Th

	//Outdoor LPF Flat
	HI257WriteCmosSensor(0x30, 0xff);//Y Hi Th
	HI257WriteCmosSensor(0x31, 0x00);//Y Lo Th
	HI257WriteCmosSensor(0x32, 0xf0);//Std Hi Th //Reso Improve Th Low //50
	HI257WriteCmosSensor(0x33, 0x00);//Std Lo Th
	HI257WriteCmosSensor(0x34, 0x00);//Median ratio

	//Indoor LPF Flat
	HI257WriteCmosSensor(0x35, 0xff);//Y Hi Th
	HI257WriteCmosSensor(0x36, 0x00);//Y Lo Th
	HI257WriteCmosSensor(0x37, 0xff);//Std Hi Th //Reso Improve Th Low //50
	HI257WriteCmosSensor(0x38, 0x00);//Std Lo Th
	HI257WriteCmosSensor(0x39, 0x00);//Median ratio

	//Dark LPF Flat
	HI257WriteCmosSensor(0x3a, 0xff);//Y Hi Th
	HI257WriteCmosSensor(0x3b, 0x00);//Y Lo Th
	HI257WriteCmosSensor(0x3c, 0x93);//Std Hi Th //Reso Improve Th Low //50
	HI257WriteCmosSensor(0x3d, 0x00);//Std Lo Th
	HI257WriteCmosSensor(0x3e, 0x00);//Median ratio

	//Outdoor Cindition
	HI257WriteCmosSensor(0x46, 0xa0);//Out Lum Hi
	HI257WriteCmosSensor(0x47, 0x40);//Out Lum Lo

	//Indoor Cindition
	HI257WriteCmosSensor(0x4c, 0xb0);//Indoor Lum Hi
	HI257WriteCmosSensor(0x4d, 0x40);//Indoor Lum Lo

	//Dark Cindition
	HI257WriteCmosSensor(0x52, 0xb0);//Dark Lum Hi
	HI257WriteCmosSensor(0x53, 0x50);//Dark Lum Lo

	//C-Filter
	HI257WriteCmosSensor(0x70, 0x10);//Outdoor(2:1) AWM Th Horizontal
	HI257WriteCmosSensor(0x71, 0x0a);//Outdoor(2:1) Diff Th Vertical
	HI257WriteCmosSensor(0x72, 0x10);//Indoor,Dark1 AWM Th Horizontal
	HI257WriteCmosSensor(0x73, 0x0a);//Indoor,Dark1 Diff Th Vertical
	HI257WriteCmosSensor(0x74, 0x14);//Dark(2:3) AWM Th Horizontal
	HI257WriteCmosSensor(0x75, 0x0c);//Dark(2:3) Diff Th Vertical

	//DPC
	HI257WriteCmosSensor(0x90, 0x3d);
	HI257WriteCmosSensor(0x91, 0x34);
	HI257WriteCmosSensor(0x99, 0x28);
	HI257WriteCmosSensor(0x9c, 0x14);
	HI257WriteCmosSensor(0x9d, 0x15);
	HI257WriteCmosSensor(0x9e, 0x28);
	HI257WriteCmosSensor(0x9f, 0x28);
	HI257WriteCmosSensor(0xb0, 0x0e);//Zipper noise Detault change (0x75->0x0e)
	HI257WriteCmosSensor(0xb8, 0x44);
	HI257WriteCmosSensor(0xb9, 0x15);
	///// PAGE 12 END /////

	///// PAGE 13 START /////
	HI257WriteCmosSensor(0x03, 0x13);//page 13

	HI257WriteCmosSensor(0x80, 0xfd);//Sharp2D enable _ YUYV Order
	HI257WriteCmosSensor(0x81, 0x07); //Sharp2D Clip/Limit
	HI257WriteCmosSensor(0x82, 0x73); //Sharp2D Filter  //jacky 73
	HI257WriteCmosSensor(0x83, 0x00);//Sharp2D Low Clip
	HI257WriteCmosSensor(0x85, 0x00);

	HI257WriteCmosSensor(0x92, 0x33);//Sharp2D Slop n/p
	HI257WriteCmosSensor(0x93, 0x30);//Sharp2D LClip
	HI257WriteCmosSensor(0x94, 0x02);//Sharp2D HiClip1 Th
	HI257WriteCmosSensor(0x95, 0xf0);//Sharp2D HiClip2 Th
	HI257WriteCmosSensor(0x96, 0x1e);//Sharp2D HiClip2 Resolution
	HI257WriteCmosSensor(0x97, 0x40);
	HI257WriteCmosSensor(0x98, 0x80);
	HI257WriteCmosSensor(0x99, 0x40);

	//Sharp Lclp
	HI257WriteCmosSensor(0xa2, 0x00);//Outdoor Lclip_N
	HI257WriteCmosSensor(0xa3, 0x00);//Outdoor Lclip_P
	HI257WriteCmosSensor(0xa4, 0x05); //Indoor Lclip_N 0x03 For reso Limit 0x0e
	HI257WriteCmosSensor(0xa5, 0x06); //Indoor Lclip_P 0x0f For reso Limit 0x0f
	HI257WriteCmosSensor(0xa6, 0x80); //Dark Lclip_N
	HI257WriteCmosSensor(0xa7, 0x80); //Dark Lclip_P

	//Outdoor Slope
	HI257WriteCmosSensor(0xb6, 0x3a);//Lum negative Hi
	HI257WriteCmosSensor(0xb7, 0x3a);//Lum negative middle
	HI257WriteCmosSensor(0xb8, 0x3a);//Lum negative Low
	HI257WriteCmosSensor(0xb9, 0x3a);//Lum postive Hi
	HI257WriteCmosSensor(0xba, 0x37);//Lum postive middle
	HI257WriteCmosSensor(0xbb, 0x3a);//Lum postive Low

	//Indoor Slope
	HI257WriteCmosSensor(0xbc, 0x28);//Lum negative Hi
	HI257WriteCmosSensor(0xbd, 0x20);//Lum negative middle
	HI257WriteCmosSensor(0xbe, 0x24);//Lum negative Low
	HI257WriteCmosSensor(0xbf, 0x28);//Lum postive Hi
	HI257WriteCmosSensor(0xc0, 0x20);//Lum postive middle
	HI257WriteCmosSensor(0xc1, 0x24);//Lum postive Low

	//Dark Slope
	HI257WriteCmosSensor(0xc2, 0x18);//Lum negative Hi
	HI257WriteCmosSensor(0xc3, 0x18);//Lum negative middle
	HI257WriteCmosSensor(0xc4, 0x10);//Lum negative Low
	HI257WriteCmosSensor(0xc5, 0x18);//Lum postive Hi
	HI257WriteCmosSensor(0xc6, 0x18);//Lum postive middle
	HI257WriteCmosSensor(0xc7, 0x10);//Lum postive Low
	///// PAGE 13 END /////

	///// PAGE 14 START /////
	HI257WriteCmosSensor(0x03, 0x14);//page 14
	HI257WriteCmosSensor(0x10, 0x01);

	HI257WriteCmosSensor(0x20, 0x80); //X-Center
	HI257WriteCmosSensor(0x21, 0x80); //Y-Center//f0

	HI257WriteCmosSensor(0x22, 0x1e); //LSC R 1b->15 20130125
	HI257WriteCmosSensor(0x23, 0x1a); //LSC G
	HI257WriteCmosSensor(0x24, 0x1a); //LSC B

	HI257WriteCmosSensor(0x25, 0xf0); //LSC Off
	HI257WriteCmosSensor(0x26, 0xf0); //LSC On
	///// PAGE 14 END /////

	/////// PAGE 15 START ///////
	HI257WriteCmosSensor(0x03, 0x15);
	//Rstep H 20
	HI257WriteCmosSensor(0x10, 0x2f);
	//Rstep L 15
	HI257WriteCmosSensor(0x14, 0x43);	//CMCOFSGH 
	HI257WriteCmosSensor(0x15, 0x2f);	//CMCOFSGM
	HI257WriteCmosSensor(0x16, 0x20);	//CMCOFSGL
	HI257WriteCmosSensor(0x17, 0x2f);	//CMC SIGN
	//CMC
	HI257WriteCmosSensor(0x30, 0xf7);
	HI257WriteCmosSensor(0x31, 0xc8);
	HI257WriteCmosSensor(0x32, 0x4e);
	HI257WriteCmosSensor(0x33, 0x38);
	HI257WriteCmosSensor(0x34, 0xd8);
	HI257WriteCmosSensor(0x35, 0x21);
	HI257WriteCmosSensor(0x36, 0x13);
	HI257WriteCmosSensor(0x37, 0x5c);
	HI257WriteCmosSensor(0x38, 0xf0);
	//CMC OFS
	HI257WriteCmosSensor(0x40, 0x84);
	HI257WriteCmosSensor(0x41, 0x81);
	HI257WriteCmosSensor(0x42, 0xf8);
	HI257WriteCmosSensor(0x43, 0x0c);
	HI257WriteCmosSensor(0x44, 0x01);
	HI257WriteCmosSensor(0x45, 0x8c);
	HI257WriteCmosSensor(0x46, 0x00);
	HI257WriteCmosSensor(0x47, 0x00);
	HI257WriteCmosSensor(0x48, 0x00);
	//CMC POFS
	HI257WriteCmosSensor(0x50, 0x87);
	HI257WriteCmosSensor(0x51, 0x52);
	HI257WriteCmosSensor(0x52, 0xca);
	HI257WriteCmosSensor(0x53, 0x14);
	HI257WriteCmosSensor(0x54, 0x00);
	HI257WriteCmosSensor(0x55, 0x90);
	HI257WriteCmosSensor(0x56, 0x0d);
	HI257WriteCmosSensor(0x57, 0x05);
	HI257WriteCmosSensor(0x58, 0x94);
	///// PAGE 15 END /////

	///// PAGE 16 START /////
	HI257WriteCmosSensor(0x03, 0x16);
	HI257WriteCmosSensor(0x10, 0x31);//GMA_CTL
	HI257WriteCmosSensor(0x18, 0x8e);//AG_ON
	HI257WriteCmosSensor(0x19, 0x8d);//AG_OFF
	HI257WriteCmosSensor(0x1A, 0x0e);//TIME_ON
	HI257WriteCmosSensor(0x1B, 0x01);//TIME_OFF
	HI257WriteCmosSensor(0x1C, 0xdc);//OUT_ON
	HI257WriteCmosSensor(0x1D, 0xfe);//OUT_OFF
	//GMA
	HI257WriteCmosSensor(0x30, 0x00);
	HI257WriteCmosSensor(0x31, 0x06);
	HI257WriteCmosSensor(0x32, 0x12);
	HI257WriteCmosSensor(0x33, 0x29);
	HI257WriteCmosSensor(0x34, 0x4d);
	HI257WriteCmosSensor(0x35, 0x6c);
	HI257WriteCmosSensor(0x36, 0x81);
	HI257WriteCmosSensor(0x37, 0x94);
	HI257WriteCmosSensor(0x38, 0xa4);
	HI257WriteCmosSensor(0x39, 0xb3);
	HI257WriteCmosSensor(0x3a, 0xc0);
	HI257WriteCmosSensor(0x3b, 0xcb);
	HI257WriteCmosSensor(0x3c, 0xd5);
	HI257WriteCmosSensor(0x3d, 0xde);
	HI257WriteCmosSensor(0x3e, 0xe6);
	HI257WriteCmosSensor(0x3f, 0xee);
	HI257WriteCmosSensor(0x40, 0xf5);
	HI257WriteCmosSensor(0x41, 0xfc);
	HI257WriteCmosSensor(0x42, 0xff);
	//RGMA
	HI257WriteCmosSensor(0x50, 0x00);
	HI257WriteCmosSensor(0x51, 0x09);
	HI257WriteCmosSensor(0x52, 0x12);
	HI257WriteCmosSensor(0x53, 0x24);
	HI257WriteCmosSensor(0x54, 0x4a);
	HI257WriteCmosSensor(0x55, 0x73);
	HI257WriteCmosSensor(0x56, 0x96);
	HI257WriteCmosSensor(0x57, 0xad);
	HI257WriteCmosSensor(0x58, 0xbc);
	HI257WriteCmosSensor(0x59, 0xc9);
	HI257WriteCmosSensor(0x5a, 0xd3);
	HI257WriteCmosSensor(0x5b, 0xdb);
	HI257WriteCmosSensor(0x5c, 0xe3);
	HI257WriteCmosSensor(0x5d, 0xe9);
	HI257WriteCmosSensor(0x5e, 0xee);
	HI257WriteCmosSensor(0x5f, 0xf2);
	HI257WriteCmosSensor(0x60, 0xf6);
	HI257WriteCmosSensor(0x61, 0xf9);
	HI257WriteCmosSensor(0x62, 0xff);
	//BGMA
	HI257WriteCmosSensor(0x70, 0x00);
	HI257WriteCmosSensor(0x71, 0x10);
	HI257WriteCmosSensor(0x72, 0x19);
	HI257WriteCmosSensor(0x73, 0x2a);
	HI257WriteCmosSensor(0x74, 0x48);
	HI257WriteCmosSensor(0x75, 0x62);
	HI257WriteCmosSensor(0x76, 0x7e);
	HI257WriteCmosSensor(0x77, 0x97);
	HI257WriteCmosSensor(0x78, 0xa9);
	HI257WriteCmosSensor(0x79, 0xb8);
	HI257WriteCmosSensor(0x7a, 0xc6);
	HI257WriteCmosSensor(0x7b, 0xd0);
	HI257WriteCmosSensor(0x7c, 0xd9);
	HI257WriteCmosSensor(0x7d, 0xe3);
	HI257WriteCmosSensor(0x7e, 0xeb);
	HI257WriteCmosSensor(0x7f, 0xf1);
	HI257WriteCmosSensor(0x80, 0xf7);
	HI257WriteCmosSensor(0x81, 0xfc);
	HI257WriteCmosSensor(0x82, 0xff);
	///// PAGE 16 END /////

	///// PAGE 17 START /////
	HI257WriteCmosSensor(0x03, 0x17);//page 17
	HI257WriteCmosSensor(0xc1, 0x00);
	HI257WriteCmosSensor(0xc4, 0x4b);
	HI257WriteCmosSensor(0xc5, 0x3f);
	HI257WriteCmosSensor(0xc6, 0x02);
	HI257WriteCmosSensor(0xc7, 0x20);
	///// PAGE 17 END /////

	///// PAGE 18 START /////
	HI257WriteCmosSensor(0x03, 0x18); //page 18
	HI257WriteCmosSensor(0x10, 0x00); //Scaling
	HI257WriteCmosSensor(0x14, 0x00);
	///// PAGE 18 END /////

	///// PAGE 19 START /////
	HI257WriteCmosSensor(0x03, 0x19);//Page 0x18
	HI257WriteCmosSensor(0x10, 0x7f);//mcmc_ctl1 MCMC Enable B:[0]
	HI257WriteCmosSensor(0x11, 0x7f);//mcmc_ctl2
	HI257WriteCmosSensor(0x12, 0x1e);//mcmc_delta1
	HI257WriteCmosSensor(0x13, 0x32);//mcmc_center1
	HI257WriteCmosSensor(0x14, 0x1e);//mcmc_delta2
	HI257WriteCmosSensor(0x15, 0x6e);//mcmc_center2
	HI257WriteCmosSensor(0x16, 0x0a);//mcmc_delta3
	HI257WriteCmosSensor(0x17, 0xb8);//mcmc_center3
	HI257WriteCmosSensor(0x18, 0x1e);//mcmc_delta4
	HI257WriteCmosSensor(0x19, 0xe6);//mcmc_center4
	HI257WriteCmosSensor(0x1a, 0x9e);//mcmc_delta5
	HI257WriteCmosSensor(0x1b, 0x22);//mcmc_center5
	HI257WriteCmosSensor(0x1c, 0x9e);//mcmc_delta6
	HI257WriteCmosSensor(0x1d, 0x5e);//mcmc_center6
	HI257WriteCmosSensor(0x1e, 0x3b);//mcmc_sat_gain1
	HI257WriteCmosSensor(0x1f, 0x48); //mcmc_sat_gain2
	HI257WriteCmosSensor(0x20, 0x48); //mcmc_sat_gain3
	HI257WriteCmosSensor(0x21, 0x70); //mcmc_sat_gain4
	HI257WriteCmosSensor(0x22, 0x2f);//mcmc_sat_gain5
	HI257WriteCmosSensor(0x23, 0x37);//mcmc_sat_gain6
	HI257WriteCmosSensor(0x24, 0x00);//mcmc_hue_angle1
	HI257WriteCmosSensor(0x25, 0x07);//mcmc_hue_angle2
	HI257WriteCmosSensor(0x26, 0x0e);//mcmc_hue_angle3
	HI257WriteCmosSensor(0x27, 0x04);//mcmc_hue_angle4
	HI257WriteCmosSensor(0x28, 0x00);//mcmc_hue_angle5
	HI257WriteCmosSensor(0x29, 0x8c);//mcmc_hue_angle6

	HI257WriteCmosSensor(0x53, 0x10);//mcmc_ctl3

	HI257WriteCmosSensor(0x6c, 0xff);//mcmc_lum_ctl1 sat hue offset
	HI257WriteCmosSensor(0x6d, 0x3f);//mcmc_lum_ctl2 gain
	HI257WriteCmosSensor(0x6e, 0x00);//mcmc_lum_ctl3 hue
	HI257WriteCmosSensor(0x6f, 0x00);//mcmc_lum_ctl4 rgb offset
	HI257WriteCmosSensor(0x70, 0x00);//mcmc_lum_ctl5 rgb scale

	HI257WriteCmosSensor(0x71, 0x3f);//mcmc_lum_gain_wgt_th1 R1 magenta
	HI257WriteCmosSensor(0x72, 0x3f);//mcmc_lum_gain_wgt_th2 R1
	HI257WriteCmosSensor(0x73, 0x3f);//mcmc_lum_gain_wgt_th3 R1
	HI257WriteCmosSensor(0x74, 0x3f);//mcmc_lum_gain_wgt_th4 R1
	HI257WriteCmosSensor(0x75, 0x30);//mcmc_rg1_lum_sp1 	 R1
	HI257WriteCmosSensor(0x76, 0x50);//mcmc_rg1_lum_sp2 	 R1
	HI257WriteCmosSensor(0x77, 0x80);//mcmc_rg1_lum_sp3 	 R1
	HI257WriteCmosSensor(0x78, 0xb0);//mcmc_rg1_lum_sp4 	 R1

	HI257WriteCmosSensor(0x79, 0x3f);//mcmc_lum_gain_wgt_th1 R2 bright green
	HI257WriteCmosSensor(0x7a, 0x3f);//mcmc_lum_gain_wgt_th2 R2
	HI257WriteCmosSensor(0x7b, 0x3f);//mcmc_lum_gain_wgt_th3 R2
	HI257WriteCmosSensor(0x7c, 0x3f);//mcmc_lum_gain_wgt_th4 R2
	HI257WriteCmosSensor(0x7d, 0x28);//mcmc_rg2_lum_sp1 	 R2
	HI257WriteCmosSensor(0x7e, 0x50);//mcmc_rg2_lum_sp2 	 R2
	HI257WriteCmosSensor(0x7f, 0x80);//mcmc_rg2_lum_sp3 	 R2
	HI257WriteCmosSensor(0x80, 0xb0);//mcmc_rg2_lum_sp4 	 R2

	HI257WriteCmosSensor(0x81, 0x28);//mcmc_lum_gain_wgt_th1 R3 dark green
	HI257WriteCmosSensor(0x82, 0x3f);//mcmc_lum_gain_wgt_th2 R3
	HI257WriteCmosSensor(0x83, 0x3f);//mcmc_lum_gain_wgt_th3 R3
	HI257WriteCmosSensor(0x84, 0x3f);//mcmc_lum_gain_wgt_th4 R3
	HI257WriteCmosSensor(0x85, 0x28);//mcmc_rg3_lum_sp1 	 R3
	HI257WriteCmosSensor(0x86, 0x50);//mcmc_rg3_lum_sp2 	 R3
	HI257WriteCmosSensor(0x87, 0x80);//mcmc_rg3_lum_sp3 	 R3
	HI257WriteCmosSensor(0x88, 0xb0);//mcmc_rg3_lum_sp4 	 R3

	HI257WriteCmosSensor(0x89, 0x1a);//mcmc_lum_gain_wgt_th1 R4 skin
	HI257WriteCmosSensor(0x8a, 0x28);//mcmc_lum_gain_wgt_th2 R4
	HI257WriteCmosSensor(0x8b, 0x3f);//mcmc_lum_gain_wgt_th3 R4
	HI257WriteCmosSensor(0x8c, 0x3f);//mcmc_lum_gain_wgt_th4 R4
	HI257WriteCmosSensor(0x8d, 0x10);//mcmc_rg3_lum_sp1 	 R4
	HI257WriteCmosSensor(0x8e, 0x30);//mcmc_rg3_lum_sp2 	 R4
	HI257WriteCmosSensor(0x8f, 0x60);//mcmc_rg3_lum_sp3 	 R4
	HI257WriteCmosSensor(0x90, 0x90);//mcmc_rg3_lum_sp4 	 R4

	HI257WriteCmosSensor(0x91, 0x1a);//mcmc_rg5_gain_wgt_th1 R5 cyan
	HI257WriteCmosSensor(0x92, 0x28);//mcmc_rg5_gain_wgt_th2 R5
	HI257WriteCmosSensor(0x93, 0x3f);//mcmc_rg5_gain_wgt_th3 R5
	HI257WriteCmosSensor(0x94, 0x3f);//mcmc_rg5_gain_wgt_th4 R5
	HI257WriteCmosSensor(0x95, 0x28);//mcmc_rg5_lum_sp1 	 R5
	HI257WriteCmosSensor(0x96, 0x50);//mcmc_rg5_lum_sp2 	 R5
	HI257WriteCmosSensor(0x97, 0x80);//mcmc_rg5_lum_sp3 	 R5
	HI257WriteCmosSensor(0x98, 0xb0);//mcmc_rg5_lum_sp4 	 R5

	HI257WriteCmosSensor(0x99, 0x1a);//mcmc_rg6_gain_wgt_th1 R6 blue
	HI257WriteCmosSensor(0x9a, 0x28);//mcmc_rg6_gain_wgt_th2 R6
	HI257WriteCmosSensor(0x9b, 0x3f);//mcmc_rg6_gain_wgt_th3 R6
	HI257WriteCmosSensor(0x9c, 0x3f);//mcmc_rg6_gain_wgt_th4 R6
	HI257WriteCmosSensor(0x9d, 0x28);//mcmc_rg6_lum_sp1 	 R6
	HI257WriteCmosSensor(0x9e, 0x50);//mcmc_rg6_lum_sp2 	 R6
	HI257WriteCmosSensor(0x9f, 0x80);//mcmc_rg6_lum_sp3 	 R6
	HI257WriteCmosSensor(0xa0, 0xb0);//mcmc_rg6_lum_sp4 	 R6

	HI257WriteCmosSensor(0xa1, 0x00);
	HI257WriteCmosSensor(0xa2, 0x00);
	HI257WriteCmosSensor(0xe5, 0x80);//add 20120709 Bit[7] On MCMC --> YC2D_LPF


	/////// PAGE 20 START ///////
	HI257WriteCmosSensor(0x03, 0x20);
	HI257WriteCmosSensor(0x10, 0x1c);
	HI257WriteCmosSensor(0x11, 0x0c);
	HI257WriteCmosSensor(0x18, 0x30);
	HI257WriteCmosSensor(0x20, 0x25);//8x8 Ae weight 0~7 Outdoor / Weight Outdoor On B[5]
	HI257WriteCmosSensor(0x21, 0x30);
	HI257WriteCmosSensor(0x22, 0x10);
	HI257WriteCmosSensor(0x23, 0x00);

	HI257WriteCmosSensor(0x28, 0xa7);
	HI257WriteCmosSensor(0x29, 0x0d);
	HI257WriteCmosSensor(0x2a, 0xff);
	HI257WriteCmosSensor(0x2b, 0x34);//Adaptive Off,1/100 Flicker

	HI257WriteCmosSensor(0x2c, 0x83);//AE After CI
	HI257WriteCmosSensor(0x2d, 0x03);
	HI257WriteCmosSensor(0x2e, 0x13);
	HI257WriteCmosSensor(0x2f, 0x0b);

	HI257WriteCmosSensor(0x30, 0x78);
	HI257WriteCmosSensor(0x31, 0xd7);
	HI257WriteCmosSensor(0x32, 0x10);
	HI257WriteCmosSensor(0x33, 0x2e);
	HI257WriteCmosSensor(0x34, 0x20);
	HI257WriteCmosSensor(0x35, 0xd4);
	HI257WriteCmosSensor(0x36, 0xfe);
	HI257WriteCmosSensor(0x37, 0x32);
	HI257WriteCmosSensor(0x38, 0x04);
	HI257WriteCmosSensor(0x39, 0x22);
	HI257WriteCmosSensor(0x3a, 0xde);
	HI257WriteCmosSensor(0x3b, 0x22);
	HI257WriteCmosSensor(0x3c, 0xde);
	HI257WriteCmosSensor(0x3d, 0xe1);

	HI257WriteCmosSensor(0x50, 0x45);
	HI257WriteCmosSensor(0x51, 0x88);

	HI257WriteCmosSensor(0x56, 0x1f);// for tracking
	HI257WriteCmosSensor(0x57, 0xa6);// for tracking
	HI257WriteCmosSensor(0x58, 0x1a);// for tracking
	HI257WriteCmosSensor(0x59, 0x7a);// for tracking 

	HI257WriteCmosSensor(0x5a, 0x04);
	HI257WriteCmosSensor(0x5b, 0x04);

	HI257WriteCmosSensor(0x5e, 0xc7);
	HI257WriteCmosSensor(0x5f, 0x95);

	HI257WriteCmosSensor(0x62, 0x10);
	HI257WriteCmosSensor(0x63, 0xc0);
	HI257WriteCmosSensor(0x64, 0x10);
	HI257WriteCmosSensor(0x65, 0x8a);
	HI257WriteCmosSensor(0x66, 0x58);
	HI257WriteCmosSensor(0x67, 0x58);

	HI257WriteCmosSensor(0x70, 0x48); //4d//48
	HI257WriteCmosSensor(0x71, 0x84); //81(+4),89(-4)//85

	HI257WriteCmosSensor(0x76, 0x32);
	HI257WriteCmosSensor(0x77, 0xa1);
	HI257WriteCmosSensor(0x78, 0x22);//24
	HI257WriteCmosSensor(0x79, 0x30); // Y Target 70 => 25, 72 => 26 //30jacky
	HI257WriteCmosSensor(0x7a, 0x23);//23
	HI257WriteCmosSensor(0x7b, 0x22);//22
	HI257WriteCmosSensor(0x7d, 0x23);

	HI257WriteCmosSensor(0x83, 0x09); //EXP Normal 10.00 fps 
	HI257WriteCmosSensor(0x84, 0x20); 
	HI257WriteCmosSensor(0x85, 0x9a); 
	HI257WriteCmosSensor(0x86, 0x01); //EXPMin 12738.85 fps
	HI257WriteCmosSensor(0x87, 0xd7); 
	HI257WriteCmosSensor(0x88, 0x09); //EXP Max 60hz 10.00 fps 
	HI257WriteCmosSensor(0x89, 0x24); 
	HI257WriteCmosSensor(0x8a, 0x48); 
	HI257WriteCmosSensor(0xa5, 0x09); //EXP Max 50hz 10.00 fps 
	HI257WriteCmosSensor(0xa6, 0x20); 
	HI257WriteCmosSensor(0xa7, 0x9a); 
	HI257WriteCmosSensor(0x8B, 0xe9); //EXP100 
	HI257WriteCmosSensor(0x8C, 0xa9); 
	HI257WriteCmosSensor(0x8D, 0xc3);  //EXP120 
	HI257WriteCmosSensor(0x8E, 0x06); 
	HI257WriteCmosSensor(0x9c, 0x1b); //EXP Limit 849.26 fps 
	HI257WriteCmosSensor(0x9d, 0x99); 
	HI257WriteCmosSensor(0x9e, 0x01); //EXP Unit 
	HI257WriteCmosSensor(0x9f, 0xd7); 
	HI257WriteCmosSensor(0xa3, 0x00); //Outdoor Int 
	HI257WriteCmosSensor(0xa4, 0xc3);  

	HI257WriteCmosSensor(0x98, 0x9d); //9d
	HI257WriteCmosSensor(0x99, 0x45);
	HI257WriteCmosSensor(0x9a, 0x0d);
	HI257WriteCmosSensor(0x9b, 0xde);

	HI257WriteCmosSensor(0xb0, 0x15);
	HI257WriteCmosSensor(0xb1, 0x10);
	HI257WriteCmosSensor(0xb2, 0x60);
	HI257WriteCmosSensor(0xb3, 0x15);
	HI257WriteCmosSensor(0xb4, 0x16);
	HI257WriteCmosSensor(0xb5, 0x3c);
	HI257WriteCmosSensor(0xb6, 0x29);
	HI257WriteCmosSensor(0xb7, 0x23);
	HI257WriteCmosSensor(0xb8, 0x20);
	HI257WriteCmosSensor(0xb9, 0x1e);
	HI257WriteCmosSensor(0xba, 0x1c);
	HI257WriteCmosSensor(0xbb, 0x1b);
	HI257WriteCmosSensor(0xbc, 0x1b);
	HI257WriteCmosSensor(0xbd, 0x1a);

	HI257WriteCmosSensor(0xc0, 0x10);
	HI257WriteCmosSensor(0xc1, 0x30);
	HI257WriteCmosSensor(0xc2, 0x30);
	HI257WriteCmosSensor(0xc3, 0x30);
	HI257WriteCmosSensor(0xc4, 0x06);

	HI257WriteCmosSensor(0xc8, 0x80);
	HI257WriteCmosSensor(0xc9, 0x80);
	///// PAGE 20 END /////

	///// PAGE 21 START /////
	HI257WriteCmosSensor(0x03, 0x21);

	//Indoor Weight
	HI257WriteCmosSensor(0x20, 0x11);
	HI257WriteCmosSensor(0x21, 0x11);
	HI257WriteCmosSensor(0x22, 0x11);
	HI257WriteCmosSensor(0x23, 0x11);
	HI257WriteCmosSensor(0x24, 0x11);
	HI257WriteCmosSensor(0x25, 0x11);
	HI257WriteCmosSensor(0x26, 0x11);
	HI257WriteCmosSensor(0x27, 0x11);
	HI257WriteCmosSensor(0x28, 0x33);
	HI257WriteCmosSensor(0x29, 0x22);
	HI257WriteCmosSensor(0x2a, 0x22);
	HI257WriteCmosSensor(0x2b, 0x21);
	HI257WriteCmosSensor(0x2c, 0x33);
	HI257WriteCmosSensor(0x2d, 0x33);
	HI257WriteCmosSensor(0x2e, 0x32);
	HI257WriteCmosSensor(0x2f, 0x21);
	HI257WriteCmosSensor(0x30, 0x33);
	HI257WriteCmosSensor(0x31, 0x33);
	HI257WriteCmosSensor(0x32, 0x32);
	HI257WriteCmosSensor(0x33, 0x21);
	HI257WriteCmosSensor(0x34, 0x33);
	HI257WriteCmosSensor(0x35, 0x22);
	HI257WriteCmosSensor(0x36, 0x22);
	HI257WriteCmosSensor(0x37, 0x21);
	HI257WriteCmosSensor(0x38, 0x11);
	HI257WriteCmosSensor(0x39, 0x11);
	HI257WriteCmosSensor(0x3a, 0x11);
	HI257WriteCmosSensor(0x3b, 0x11);
	HI257WriteCmosSensor(0x3c, 0x11);
	HI257WriteCmosSensor(0x3d, 0x11);
	HI257WriteCmosSensor(0x3e, 0x11);
	HI257WriteCmosSensor(0x3f, 0x11);
	//OUT
	HI257WriteCmosSensor(0x40, 0x11);
	HI257WriteCmosSensor(0x41, 0x11);
	HI257WriteCmosSensor(0x42, 0x11);
	HI257WriteCmosSensor(0x43, 0x11);
	HI257WriteCmosSensor(0x44, 0x11);
	HI257WriteCmosSensor(0x45, 0x11);
	HI257WriteCmosSensor(0x46, 0x11);
	HI257WriteCmosSensor(0x47, 0x11);
	HI257WriteCmosSensor(0x48, 0x33);
	HI257WriteCmosSensor(0x49, 0x22);
	HI257WriteCmosSensor(0x4a, 0x22);
	HI257WriteCmosSensor(0x4b, 0x21);
	HI257WriteCmosSensor(0x4c, 0x33);
	HI257WriteCmosSensor(0x4d, 0x33);
	HI257WriteCmosSensor(0x4e, 0x32);
	HI257WriteCmosSensor(0x4f, 0x21);
	HI257WriteCmosSensor(0x50, 0x33);
	HI257WriteCmosSensor(0x51, 0x33);
	HI257WriteCmosSensor(0x52, 0x32);
	HI257WriteCmosSensor(0x53, 0x21);
	HI257WriteCmosSensor(0x54, 0x33);
	HI257WriteCmosSensor(0x55, 0x22);
	HI257WriteCmosSensor(0x56, 0x22);
	HI257WriteCmosSensor(0x57, 0x21);
	HI257WriteCmosSensor(0x58, 0x11);
	HI257WriteCmosSensor(0x59, 0x11);
	HI257WriteCmosSensor(0x5a, 0x11);
	HI257WriteCmosSensor(0x5b, 0x11);
	HI257WriteCmosSensor(0x5c, 0x11);
	HI257WriteCmosSensor(0x5d, 0x11);
	HI257WriteCmosSensor(0x5e, 0x11);
	HI257WriteCmosSensor(0x5f, 0x11);


	///// PAGE 22 START /////
	HI257WriteCmosSensor(0x03, 0x22); //page 22
	HI257WriteCmosSensor(0x10, 0xfd);
	HI257WriteCmosSensor(0x11, 0x2e);
	HI257WriteCmosSensor(0x19, 0x00); //Low On //
	HI257WriteCmosSensor(0x20, 0x30); //For AWB Speed
	HI257WriteCmosSensor(0x21, 0x80);
	HI257WriteCmosSensor(0x22, 0x00);
	HI257WriteCmosSensor(0x23, 0x00);
	HI257WriteCmosSensor(0x24, 0x01);
	HI257WriteCmosSensor(0x25, 0x7f); //2013-09-13 AWB Hunting 4f

	HI257WriteCmosSensor(0x30, 0x80);
	HI257WriteCmosSensor(0x31, 0x80);
	HI257WriteCmosSensor(0x38, 0x11);
	HI257WriteCmosSensor(0x39, 0x33);//34
	HI257WriteCmosSensor(0x40, 0xfa);//ersen.shang for pr860238 capture a photo,the color of the photo is blue 
	HI257WriteCmosSensor(0x41, 0x33); //Stb cdiff
	HI257WriteCmosSensor(0x42, 0x22); //Stb csum
	HI257WriteCmosSensor(0x43, 0xfa); //ersen.shang for pr860238 capture a photo,the color of the photo is blue 
	HI257WriteCmosSensor(0x44, 0x33); //Unstb cdiff
	HI257WriteCmosSensor(0x45, 0x33); //Unstb csum
	HI257WriteCmosSensor(0x46, 0x00);
	HI257WriteCmosSensor(0x47, 0x09); //2013-09-13 AWB Hunting
	HI257WriteCmosSensor(0x48, 0x00); //2013-09-13 AWB Hunting
	HI257WriteCmosSensor(0x49, 0x0a);

	HI257WriteCmosSensor(0x60, 0x04);
	HI257WriteCmosSensor(0x61, 0xc4);
	HI257WriteCmosSensor(0x62, 0x04);
	HI257WriteCmosSensor(0x63, 0x92);
	HI257WriteCmosSensor(0x66, 0x04);
	HI257WriteCmosSensor(0x67, 0xc4);
	HI257WriteCmosSensor(0x68, 0x04);
	HI257WriteCmosSensor(0x69, 0x92);

	HI257WriteCmosSensor(0x80, 0x38);
	HI257WriteCmosSensor(0x81, 0x20);
	HI257WriteCmosSensor(0x82, 0x38);

	HI257WriteCmosSensor(0x83, 0x45);
	HI257WriteCmosSensor(0x84, 0x16);
	HI257WriteCmosSensor(0x85, 0x4f);
	HI257WriteCmosSensor(0x86, 0x1a);

	HI257WriteCmosSensor(0x87, 0x40);
	HI257WriteCmosSensor(0x88, 0x2f);
	HI257WriteCmosSensor(0x89, 0x23);
	HI257WriteCmosSensor(0x8a, 0x18);

	HI257WriteCmosSensor(0x8b, 0x3d);
	HI257WriteCmosSensor(0x8c, 0x32);
	HI257WriteCmosSensor(0x8d, 0x24);
	HI257WriteCmosSensor(0x8e, 0x1d);

	HI257WriteCmosSensor(0x8f, 0x4f);
	HI257WriteCmosSensor(0x90, 0x46);
	HI257WriteCmosSensor(0x91, 0x41);
	HI257WriteCmosSensor(0x92, 0x3b);
	HI257WriteCmosSensor(0x93, 0x30);
	HI257WriteCmosSensor(0x94, 0x24);
	HI257WriteCmosSensor(0x95, 0x1c);
	HI257WriteCmosSensor(0x96, 0x18);
	HI257WriteCmosSensor(0x97, 0x16);
	HI257WriteCmosSensor(0x98, 0x14);
	HI257WriteCmosSensor(0x99, 0x11);
	HI257WriteCmosSensor(0x9a, 0x10);

	HI257WriteCmosSensor(0x9b, 0x77);
	HI257WriteCmosSensor(0x9c, 0x77);
	HI257WriteCmosSensor(0x9d, 0x48);
	HI257WriteCmosSensor(0x9e, 0x38);
	HI257WriteCmosSensor(0x9f, 0x30);

	HI257WriteCmosSensor(0xa0, 0xb0);
	HI257WriteCmosSensor(0xa1, 0x54);
	HI257WriteCmosSensor(0xa2, 0x4f);
	HI257WriteCmosSensor(0xa3, 0xff);

	HI257WriteCmosSensor(0xa4, 0x14); //1536fps
	HI257WriteCmosSensor(0xa5, 0x2c); //698fps
	HI257WriteCmosSensor(0xa6, 0xcf); //148fps

	HI257WriteCmosSensor(0xad, 0x40);
	HI257WriteCmosSensor(0xae, 0x4a);

	HI257WriteCmosSensor(0xaf, 0x28); //Low temp Rgain
	HI257WriteCmosSensor(0xb0, 0x26); //Low temp Rgain

	HI257WriteCmosSensor(0xb1, 0x00);
	HI257WriteCmosSensor(0xb4, 0xbf); //For Tracking AWB Weight
	HI257WriteCmosSensor(0xb8, 0x00);//(0+,1-)High Cb , (0+,1-)Low Cr  D2
	HI257WriteCmosSensor(0xb9, 0x00);

	/////// PAGE 20 ///////
	HI257WriteCmosSensor(0x03, 0x20);
	HI257WriteCmosSensor(0x10, 0x9c);//AE On 50hz

	/////// PAGE 22 ///////
	HI257WriteCmosSensor(0x03, 0x22);
	HI257WriteCmosSensor(0x10, 0xe9);//AWB On

	/////// PAGE 0 ///////
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x0e, 0x03);
	HI257WriteCmosSensor(0x0e, 0x73);

	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);

	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x01, 0x00);
	HI257WriteCmosSensor(0xff, 0xff);
	mdelay(100);
}

void HI257InitPara(void)
{
	SENSORDB("enter the initPara function");
	spin_lock(&hi257_drv_lock);
	HI257Status.NightMode = KAL_FALSE;
	HI257Status.ZoomFactor = 0;
	HI257Status.Banding = AE_FLICKER_MODE_50HZ;
	HI257Status.PvShutter = 0x17c40;
	HI257Status.MaxFrameRate = HI257_MAX_FPS;
	HI257Status.MiniFrameRate = HI257_FPS(10);
	HI257Status.PvDummyPixels = 376;
	HI257Status.PvDummyLines = 20;
	HI257Status.CapDummyPixels = 376;
	HI257Status.CapDummyLines = 20; /* 10 FPS, 104 for 9.6 FPS*/
	HI257Status.PvOpClk = 24;
	HI257Status.CapOpClk = 24;  
	HI257Status.VDOCTL2 = 0x90;
	HI257Status.ISPCTL3 = 0x30;
	HI257Status.AECTL1 = 0x9c;
	HI257Status.AWBCTL1 = 0xe9;
	spin_unlock(&hi257_drv_lock);
}

/*************************************************************************
* FUNCTION
*  HI257SetMirror
*
* DESCRIPTION
*  This function mirror, flip or mirror & flip the sensor output image.
*
*  IMPORTANT NOTICE: For some sensor, it need re-set the output order Y1CbY2Cr after
*  mirror or flip.
*
* PARAMETERS
*  1. kal_uint16 : horizontal mirror or vertical flip direction.
*
* RETURNS
*  None
*
*************************************************************************/
static void HI257SetMirror(kal_uint16 ImageMirror)
{
	spin_lock(&hi257_drv_lock);
	HI257Status.VDOCTL2 &= 0xfc;   
	spin_unlock(&hi257_drv_lock);
	switch (ImageMirror)
	{
	case IMAGE_H_MIRROR:
		spin_lock(&hi257_drv_lock);
		HI257Status.VDOCTL2 |= 0x01;
		spin_unlock(&hi257_drv_lock);
		break;
	case IMAGE_V_MIRROR:
		spin_lock(&hi257_drv_lock);
		HI257Status.VDOCTL2 |= 0x02; 
		spin_unlock(&hi257_drv_lock);
		break;
	case IMAGE_HV_MIRROR:
		spin_lock(&hi257_drv_lock);
		HI257Status.VDOCTL2 |= 0x03;
		spin_unlock(&hi257_drv_lock);
		break;
	case IMAGE_NORMAL:
	default:
		spin_lock(&hi257_drv_lock);
		HI257Status.VDOCTL2 |= 0x00; 
		spin_unlock(&hi257_drv_lock);
	}
	HI257SetPage(0x00);
	HI257WriteCmosSensor(0x11, HI257Status.VDOCTL2); //zhao.li@tcl 0x80 
}

static void HI257SetAeMode(kal_bool AeEnable)
{
	SENSORDB("[HI257]HI257SetAeMode AeEnable:%d;\n",AeEnable);

	if (AeEnable == KAL_TRUE)
	{
		spin_lock(&hi257_drv_lock);
		HI257Status.AECTL1 |= 0x80;
		spin_unlock(&hi257_drv_lock);
	}
	else
	{
		spin_lock(&hi257_drv_lock);
		HI257Status.AECTL1 &= (~0x80);
		spin_unlock(&hi257_drv_lock);
	}
	HI257SetPage(0x20);
	HI257WriteCmosSensor(0x10,HI257Status.AECTL1);  
}

static void HI257SetAwbMode(kal_bool AwbEnable)
{
	SENSORDB("[HI257]HI257SetAwbMode AwbEnable:%d;\n",AwbEnable);
	if (AwbEnable == KAL_TRUE)
	{
		spin_lock(&hi257_drv_lock);
		HI257Status.AWBCTL1 |= 0x80;
		spin_unlock(&hi257_drv_lock);
	}
	else
	{
		spin_lock(&hi257_drv_lock);
		HI257Status.AWBCTL1 &= (~0x80);
		spin_unlock(&hi257_drv_lock);
	}
	HI257SetPage(0x22);
	HI257WriteCmosSensor(0x10,HI257Status.AWBCTL1);  
}


UINT32 ISDarkMode()
{
	kal_uint32 exptime,expmax;
	HI257SetPage(0x20);
	HI257WriteCmosSensor(0x10,0x1c);
	exptime=(HI257ReadCmosSensor(0x80)<<16 | HI257ReadCmosSensor(0x81)<<8 | HI257ReadCmosSensor(0x82));  //current exp time
	expmax=(HI257ReadCmosSensor(0xa5)<<16 | HI257ReadCmosSensor(0xa6)<<8 | HI257ReadCmosSensor(0xa7));//max exp time in this mode
	SENSORDB("[HI257] exptime:%d;expmax:%d\n",exptime,expmax);

	if(exptime<expmax)
		return FALSE;
	else 
		return TRUE;

}

/*************************************************************************
* FUNCTION
* HI257NightMode
*
* DESCRIPTION
* This function night mode of HI257.
*
* PARAMETERS
* none
*
* RETURNS
* None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void HI257NightMode(kal_bool Enable)
{
	kal_uint32 EXPMAX, EXPTIME, BLC_TIME_TH_ONOFF;
	kal_uint32 LineLength,BandingValue;
	SENSORDB("[HI257]HI257NightMode Enable:%d;\n",Enable);
	/* Night mode only for camera preview */
	//if (HI257Status.MaxFrameRate == HI257Status.MiniFrameRate)  return ;
	//spin_lock(&hi257_drv_lock);
	//HI257Status.MiniFrameRate = Enable ? HI257_FPS(5) : HI257_FPS(10);
	//spin_unlock(&hi257_drv_lock);

	if (Enable)
	{
		HI257Status.NightMode=TRUE;
		if(ISDarkMode())//dark condition
		{
			HI257Status.VDOCTL2 &= 0xFB;

			HI257WriteCmosSensor(0x03, 0x00);
			HI257WriteCmosSensor(0x11, 0x93);  //zhao.li@tcl 0x80 // Fixed frame rate OFF bit2 off :0 on:1
			HI257WriteCmosSensor(0x03, 0x20);
			HI257WriteCmosSensor(0x10, 0x1c);
			HI257WriteCmosSensor(0x03, 0x00);
			HI257WriteCmosSensor(0x11, 0x93); //zhao.li@tcl 0x80 //Video Off / Variable On

			HI257WriteCmosSensor(0x40, 0x01); //Hblank_284
			HI257WriteCmosSensor(0x41, 0x04);
			HI257WriteCmosSensor(0x42, 0x00); //Vblank
			HI257WriteCmosSensor(0x43, 0x14); //Flick Stop

			HI257WriteCmosSensor(0x90, 0x14);
			HI257WriteCmosSensor(0x91, 0x14);

			HI257WriteCmosSensor(0x03, 0x02);
			HI257WriteCmosSensor(0xd4, 0x14);
			HI257WriteCmosSensor(0xd5, 0x14);

			HI257WriteCmosSensor(0x03, 0x10);
			HI257WriteCmosSensor(0x13, 0x01);
			HI257WriteCmosSensor(0x48, 0x78);

			HI257WriteCmosSensor(0x03, 0x20);
			HI257WriteCmosSensor(0x83, 0x12); //EXP Normal 5.00 fps
			HI257WriteCmosSensor(0x84, 0x41);
			HI257WriteCmosSensor(0x85, 0x34);

			HI257WriteCmosSensor(0x88, 0x12); //EXP Max(120Hz) 5.00 fps
			HI257WriteCmosSensor(0x89, 0x48);
			HI257WriteCmosSensor(0x8a, 0x90);

			HI257WriteCmosSensor(0xa5, 0x12); //EXP Max(100Hz) 5.00 fps
			HI257WriteCmosSensor(0xa6, 0x41);
			HI257WriteCmosSensor(0xa7, 0x34);

			HI257WriteCmosSensor(0x03, 0x20);
			HI257WriteCmosSensor(0x10, 0x9c);

			mdelay(200);
		}
		else
		{
			HI257Status.NightMode=TRUE;
			HI257Status.VDOCTL2 &= 0xFB;
			HI257WriteCmosSensor(0x03, 0x00);
			HI257WriteCmosSensor(0x11, 0x93);  //zhao.li@tcl 0x80 // Fixed frame rate OFF bit2 off :0 on:1
			HI257WriteCmosSensor(0x03, 0x20);
			HI257WriteCmosSensor(0x10, 0x1c);
			HI257WriteCmosSensor(0x03, 0x00);
			HI257WriteCmosSensor(0x11, 0x93); //zhao.li@tcl 0x80 //Video Off / Variable On

			HI257WriteCmosSensor(0x40, 0x01); //Hblank_284
			HI257WriteCmosSensor(0x41, 0x04);
			HI257WriteCmosSensor(0x42, 0x00); //Vblank
			HI257WriteCmosSensor(0x43, 0x14); //Flick Stop

			HI257WriteCmosSensor(0x90, 0x14);
			HI257WriteCmosSensor(0x91, 0x14);

			HI257WriteCmosSensor(0x03, 0x02);
			HI257WriteCmosSensor(0xd4, 0x14);
			HI257WriteCmosSensor(0xd5, 0x14);

			HI257WriteCmosSensor(0x03, 0x10);
			HI257WriteCmosSensor(0x13, 0x01);
			HI257WriteCmosSensor(0x48, 0x78);

			HI257WriteCmosSensor(0x03, 0x20);
			HI257WriteCmosSensor(0x83, 0x12); //EXP Normal 5.00 fps
			HI257WriteCmosSensor(0x84, 0x41);
			HI257WriteCmosSensor(0x85, 0x34);

			HI257WriteCmosSensor(0x88, 0x12); //EXP Max(120Hz) 5.00 fps
			HI257WriteCmosSensor(0x89, 0x48);
			HI257WriteCmosSensor(0x8a, 0x90);

			HI257WriteCmosSensor(0xa5, 0x12); //EXP Max(100Hz) 5.00 fps
			HI257WriteCmosSensor(0xa6, 0x41);
			HI257WriteCmosSensor(0xa7, 0x34);

			HI257WriteCmosSensor(0x03, 0x20);
			HI257WriteCmosSensor(0x10, 0x9c);

			mdelay(200);
		}
	}
	else
	{
		HI257Status.VDOCTL2 &= 0xFB;

		HI257Status.NightMode=FALSE;

		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x11, 0x93);//zhao.li@tcl 0x80 //Video Off / Variable On

		HI257WriteCmosSensor(0x40, 0x01); //Hblank_284
		HI257WriteCmosSensor(0x41, 0x04);
		HI257WriteCmosSensor(0x42, 0x00); //Vblank
		HI257WriteCmosSensor(0x43, 0x14); //Flick Stop

		HI257WriteCmosSensor(0x90, 0x0a);
		HI257WriteCmosSensor(0x91, 0x0a);

		HI257WriteCmosSensor(0x03, 0x02);
		HI257WriteCmosSensor(0xd4, 0x0a);
		HI257WriteCmosSensor(0xd5, 0x0a);

		HI257WriteCmosSensor(0x03, 0x10);
		HI257WriteCmosSensor(0x13, 0x01);
		HI257WriteCmosSensor(0x48, 0x80);


		HI257WriteCmosSensor(0x03, 0x20);
		HI257WriteCmosSensor(0x10, 0x1c);
		HI257WriteCmosSensor(0x18, 0x38);

		HI257WriteCmosSensor(0x83, 0x09);
		HI257WriteCmosSensor(0x84, 0x20);
		HI257WriteCmosSensor(0x85, 0x9a);  

		HI257WriteCmosSensor(0x88, 0x09); //EXP Max(120Hz) 10.00 fps
		HI257WriteCmosSensor(0x89, 0x24);
		HI257WriteCmosSensor(0x8a, 0x48);

		HI257WriteCmosSensor(0xa5, 0x09); //EXP Max(100Hz) 10.00 fps 
		HI257WriteCmosSensor(0xa6, 0x20); 
		HI257WriteCmosSensor(0xa7, 0x9a); 

		HI257WriteCmosSensor(0x03, 0x20);
		HI257WriteCmosSensor(0x10, 0x9c);

		HI257WriteCmosSensor(0x18, 0x30);

		mdelay(200);
	};

	LineLength = HI257_PV_PERIOD_PIXEL_NUMS + HI257Status.PvDummyPixels;

} /* HI257NightMode */
/* HI257NightMode */


/*************************************************************************
* FUNCTION
* HI257Open
*
* DESCRIPTION
* this function initialize the registers of CMOS sensor
*
* PARAMETERS
* none
*
* RETURNS
*  none
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HI257Open(void)
{
	kal_uint16 SensorId = 0;
	kal_uint8 i;
	
	////1 software reset sensor and wait (to sensor)
	HI257SetPage(0x00);
	HI257WriteCmosSensor(0x01,0x01);
	HI257WriteCmosSensor(0x01,0x03);
	HI257WriteCmosSensor(0x01,0x01);
	SENSORDB("enter the open function");

	//  SensorId = HI257ReadCmosSensor(0x04);
	//
	//  Sleep(3);
	// SENSORDB("[HI257]HI257Open: Sensor ID %x\n",SensorId);
	/* for(i=0;i<3;i++)
	{
	SensorId = HI257ReadCmosSensor(0x04);

	if(SensorId==HI257_SENSOR_ID)
	break;
	else
	return ERROR_SENSOR_CONNECT_FAIL;
	}

	return ERROR_NONE;
	//  if(SensorId != HI257_SENSOR_ID)
	//  {
	//    return ERROR_SENSOR_CONNECT_FAIL;
	//  }
	*/
	//HI257InitSetting();
	//HI257InitPara();


#ifdef HI257_LOAD_FROM_T_FLASH 
	struct file *fp; 
	mm_segment_t fs; 
	loff_t pos = 0; 
	//static char buf[10*1024] ;

	printk("HI257 Open File Start\n");
	fp = filp_open("/sdcard/hi257_sd.dat", O_RDONLY , 0); 

	if (IS_ERR(fp)) 
	{ 
		fromsd = 0;	 
		printk("open file error\n");
		////////return 0;
	} 
	else 
	{
		printk("open file success\n");
		fromsd = 1;
		//SP2528_Initialize_from_T_Flash();
		filp_close(fp, NULL); 
	}
	//set_fs(fs);
	if(fromsd == 1)//gepeiwei   120903
		Hi257_Initialize_from_T_Flash();//??��?SD??��?���顧��?|��????����ao?����oy
	else
	{
		HI257InitSetting();
	}
	HI257InitPara();
	//HI257_Init_Parameter(); 
#else
	HI257InitSetting();
	HI257InitPara();
#endif
	//HI257_set_mirror_flip_cmk(3);

	return ERROR_NONE;
}	

/* HI257Open() */

/*************************************************************************
* FUNCTION
*   HI257GetSensorID
*
* DESCRIPTION
*   This function get the sensor ID 
*
* PARAMETERS
*   *sensorID : return the sensor ID 
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
//BEGIN: add by fangjie for show camera info in MINISW screen, FR803765
extern char Back_Camera_Name[256]; 
//END: add by fangjie for show camera info in MINISW screen, FR803765
UINT32 HI257GetSensorID(UINT32 *sensorID) 

{
	int  retry = 3; 
	// check if sensor ID correct
	do 
	{		
		*sensorID =HI257ReadCmosSensor(0x04);
		if (*sensorID == HI257_SENSOR_ID)
			break;
		SENSORDB("HI257 Read Sensor ID Fail = 0x%04x\n", *sensorID); 
		retry--; 

	} while (retry > 0);

	if (*sensorID != HI257_SENSOR_ID) 
	{
		*sensorID = 0xFFFFFFFF; 
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	//BEGIN: add by fangjie for show camera info in MINISW screen, FR803765
	sprintf(Back_Camera_Name,"Vender:  E-welly \nSensor ID:  0x%X \nSensor name:  HI257  \nResolution:  2M ",
		HI257_SENSOR_ID);
	//END: add by fangjie for show camera info in MINISW screen, FR803765
	return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
* HI257Close
*
* DESCRIPTION
* This HI257SetMaxFramerateByScenario is to turn off sensor module power.
*
* PARAMETERS
* None
*
* RETURNS
* None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HI257SetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) 
{
	kal_uint32 pclk;
	kal_int16 dummyLine;
	kal_uint16 lineLength,frameHeight;

	SENSORDB("HI257SetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
	/*switch (scenarioId) 
	{
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	pclk = 134200000;
	lineLength = IMX111MIPI_PV_LINE_LENGTH_PIXELS;
	frameHeight = (10 * pclk)/frameRate/lineLength;
	dummyLine = frameHeight - IMX111MIPI_PV_FRAME_LENGTH_LINES;


	if(dummyLine<0)
	dummyLine = 0;
	//spin_lock(&imx111_drv_lock);	
	//IMX111MIPI_sensor.pv_mode=TRUE;
	//spin_unlock(&imx111_drv_lock);
	IMX111MIPI_SetDummy(0, dummyLine);			
	break;			
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
	pclk = 199200000;
	lineLength = IMX111MIPI_VIDEO_LINE_LENGTH_PIXELS;
	frameHeight = (10 * pclk)/frameRate/lineLength;
	dummyLine = frameHeight - IMX111MIPI_VIDEO_FRAME_LENGTH_LINES;
	if(dummyLine<0)
	dummyLine = 0;
	//	spin_lock(&imx111_drv_lock);	
	//	IMX111MIPI_sensor.pv_mode=TRUE;
	//	spin_unlock(&imx111_drv_lock);
	IMX111MIPI_SetDummy(0, dummyLine);			
	break;			
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	case MSDK_SCENARIO_ID_CAMERA_ZSD:			
	pclk = 158400000;
	lineLength = IMX111MIPI_FULL_LINE_LENGTH_PIXELS;
	frameHeight = (10 * pclk)/frameRate/lineLength;
	dummyLine = frameHeight - IMX111MIPI_FULL_FRAME_LENGTH_LINES;
	if(dummyLine<0)
	dummyLine = 0;

	//spin_lock(&imx111_drv_lock);	
	//IMX111MIPI_sensor.pv_mode=FALSE;
	//spin_unlock(&imx111_drv_lock);
	IMX111MIPI_SetDummy(0, dummyLine);			
	break;		//hhl 2-28
	case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
	break;
	case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
	break;
	case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
	break;		
	default:
	break;
	}	*/
	return ERROR_NONE;
}
/*************************************************************************
* FUNCTION
* HI257GetDefaultFramerateByScenario
*
* DESCRIPTION
* This function is to turn off sensor module power.
*
* PARAMETERS
* None
*
* RETURNS
* None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HI257GetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
{

	switch (scenarioId) 
	{
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		*pframeRate = 300;
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	case MSDK_SCENARIO_ID_CAMERA_ZSD:
		*pframeRate = 220;
		break;		//hhl 2-28
	/* case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
	case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
	case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
		*pframeRate = 300;
		break;		*/
	default:
		*pframeRate = 300;
		break;
	}
}

/*************************************************************************
* FUNCTION
* HI257Close
*
* DESCRIPTION
* This function is to turn off sensor module power.
*
* PARAMETERS
* None
*
* RETURNS
* None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HI257Close(void)
{
	return ERROR_NONE;
} /* HI257Close() */

/*************************************************************************
* FUNCTION
* HI257Preview
*
* DESCRIPTION
* This function start the sensor preview.
*
* PARAMETERS
* *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
* None
*
* GLOBALS AFFECTED
*
*************************************************************************/

UINT32 HI257Video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
				  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint32 LineLength, EXP100, EXP120, EXPMIN, EXPUNIT; 

	SENSORDB("\n\n\n\n\n\n");
	SENSORDB("[HI257]HI257Preview\n");

	/* For change max frame rate only need modify HI257Status.MaxFrameRate */
	spin_lock(&hi257_drv_lock);
	HI257Status.VideoMode=KAL_TRUE;
	spin_unlock(&hi257_drv_lock);

	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x00,0x01);
	HI257WriteCmosSensor(0x0e,0x03);

	HI257WriteCmosSensor(0x03,0x20);
	HI257WriteCmosSensor(0x10,0x1c);

	HI257WriteCmosSensor(0x03,0x22);
	HI257WriteCmosSensor(0x10,0x69);

	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x10,0x91);
	HI257WriteCmosSensor(0x11,0x93);//zhao.li@tcl 0x80

	HI257WriteCmosSensor(0x20,0x00);
	HI257WriteCmosSensor(0x21,0x02);
	HI257WriteCmosSensor(0x22,0x00);
	HI257WriteCmosSensor(0x23,0x04);

	HI257WriteCmosSensor(0x40,0x01); //Hblank_284
	HI257WriteCmosSensor(0x41,0x78);
	HI257WriteCmosSensor(0x42,0x00); //Vblank
	HI257WriteCmosSensor(0x43,0x14); //Flick Stop

	HI257WriteCmosSensor(0x03,0x18);
	HI257WriteCmosSensor(0x14,0x0b);

	HI257WriteCmosSensor(0x03, 0x20);
	HI257WriteCmosSensor(0x2b, 0x34); //Adaptive Off,1/100 Flicker
	HI257WriteCmosSensor(0x30, 0x78);
	//HI257WriteCmosSensor(0x88,0x09);//exp max10
	//HI257WriteCmosSensor(0x89,0x23);
	//HI257WriteCmosSensor(0x8a,0xce);

	HI257WriteCmosSensor(0x83, 0x05); //EXP Normal 16.67 fps 
	HI257WriteCmosSensor(0x84, 0x79); 
	HI257WriteCmosSensor(0x85, 0xf6); 
	HI257WriteCmosSensor(0x86, 0x01); //EXPMin 12738.85 fps
	HI257WriteCmosSensor(0x87, 0xd7); 
	HI257WriteCmosSensor(0x88, 0x06); //EXP Max 60hz 15.00 fps 
	HI257WriteCmosSensor(0x89, 0x18); 
	HI257WriteCmosSensor(0x8a, 0x30); 
	HI257WriteCmosSensor(0xa5, 0x05); //EXP Max 50hz 16.67 fps 
	HI257WriteCmosSensor(0xa6, 0x79); 
	HI257WriteCmosSensor(0xa7, 0xf6); 
	HI257WriteCmosSensor(0x8B, 0xe9); //EXP100 
	HI257WriteCmosSensor(0x8C, 0xa9); 
	HI257WriteCmosSensor(0x8D, 0xc3); //EXP120 
	HI257WriteCmosSensor(0x8E, 0x06); 
	HI257WriteCmosSensor(0x9c, 0x1b); //EXP Limit 849.26 fps 
	HI257WriteCmosSensor(0x9d, 0x99); 
	HI257WriteCmosSensor(0x9e, 0x01); //EXP Unit 
	HI257WriteCmosSensor(0x9f, 0xd7); 
	HI257WriteCmosSensor(0xa3, 0x00); //Outdoor Int 
	HI257WriteCmosSensor(0xa4, 0xc3); 

	HI257WriteCmosSensor(0x10,0x9c);//AE on

	HI257WriteCmosSensor(0x03,0x22);
	HI257WriteCmosSensor(0x10,0xe9);//awb on    
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x0e,0x03);
	HI257WriteCmosSensor(0x0e,0x73);
	HI257WriteCmosSensor(0x03,0x00); //Dummy
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x01,0x00);//Sleep off
	spin_lock(&hi257_drv_lock);
	HI257Status.PvDummyPixels = 376;
	spin_unlock(&hi257_drv_lock);
	LineLength = HI257_PV_PERIOD_PIXEL_NUMS + HI257Status.PvDummyPixels;
	spin_lock(&hi257_drv_lock);
	HI257Status.MiniFrameRate = HI257_FPS(10);  
	HI257Status.PvDummyLines = HI257Status.PvOpClk * 1000000 * HI257_FRAME_RATE_UNIT / LineLength / HI257Status.MaxFrameRate -  HI257_PV_PERIOD_LINE_NUMS; 
	spin_unlock(&hi257_drv_lock);
	HI257SetAeMode(KAL_TRUE);
	HI257SetAwbMode(KAL_TRUE);
	// HI257NightMode(HI257Status.NightMode);
	return ERROR_NONE;}

/*************************************************************************
* FUNCTION
* HI257Preview
*
* DESCRIPTION
* This function start the sensor preview.
*
* PARAMETERS
* *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
* None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HI257Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint32 LineLength, EXP100, EXP120, EXPMIN, EXPUNIT; 

	SENSORDB("\n\n\n\n\n\n");
	SENSORDB("[HI257]HI257Preview\n");

	/* For change max frame rate only need modify HI257Status.MaxFrameRate */
	spin_lock(&hi257_drv_lock);
	HI257Status.MaxFrameRate = HI257_MAX_FPS;
	spin_unlock(&hi257_drv_lock);

	if(HI257Status.VideoMode)
	{
		//HI257InitSetting();
		HI257Status.VideoMode=KAL_FALSE;
	}

	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x00,0x01);
	HI257WriteCmosSensor(0x0e,0x03);

	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x10,0x90);
	HI257WriteCmosSensor(0x11,0x93);//zhao.li@tcl 0x80 //Variable
	HI257WriteCmosSensor(0x20,0x00);
	HI257WriteCmosSensor(0x21,0x07);
	HI257WriteCmosSensor(0x22,0x00);
	HI257WriteCmosSensor(0x23,0x07);
	HI257WriteCmosSensor(0x40,0x01);//Hblank_284
	HI257WriteCmosSensor(0x41,0x04);
	HI257WriteCmosSensor(0x42,0x00);//Vblank
	HI257WriteCmosSensor(0x43,0x14);//Flick Stop

	HI257WriteCmosSensor(0x03,0x18);
	HI257WriteCmosSensor(0x14,0x00);

	HI257WriteCmosSensor(0x03,0x20);
	HI257WriteCmosSensor(0x10,0x1c);//AE Off

	HI257WriteCmosSensor(0x03,0x22);
	HI257WriteCmosSensor(0x10,0x69);//AWB Off

	HI257WriteCmosSensor(0x03,0x20);
	HI257WriteCmosSensor(0x2b,0x34);//Adaptive Off,1/100 Flicker
	HI257WriteCmosSensor(0x30,0x78);
	HI257WriteCmosSensor(0x10,0x9c);//AE on

	HI257WriteCmosSensor(0x03,0x22);
	HI257WriteCmosSensor(0x10,0xe9);//awb on

	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x0e,0x03);
	HI257WriteCmosSensor(0x0e,0x73);

	HI257WriteCmosSensor(0x03,0x00);//Dummy
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x03,0x00);

	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x01,0x00);//Sleep off

	spin_lock(&hi257_drv_lock);
	HI257Status.PvDummyPixels = 376;
	spin_unlock(&hi257_drv_lock);
	LineLength = HI257_PV_PERIOD_PIXEL_NUMS + HI257Status.PvDummyPixels;
	spin_lock(&hi257_drv_lock);
	HI257Status.MiniFrameRate = HI257_FPS(10);  
	HI257Status.PvDummyLines = HI257Status.PvOpClk * 1000000 * HI257_FRAME_RATE_UNIT / LineLength / HI257Status.MaxFrameRate -  HI257_PV_PERIOD_LINE_NUMS; 
	spin_unlock(&hi257_drv_lock);
	HI257SetAeMode(KAL_TRUE);
	HI257SetAwbMode(KAL_TRUE);
	//HI257NightMode(HI257Status.NightMode); //ersen.shang for pr860238 capture a photo,the color of the photo is blue 
	sensor_config_data->SensorImageMirror = IMAGE_HV_MIRROR;
	HI257SetMirror(sensor_config_data->SensorImageMirror);
	return ERROR_NONE;
}/* HI257Preview() */

UINT32 HI257Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{										 
	kal_uint32 LineLength, EXP100, EXP120, EXPMIN, EXPUNIT, CapShutter;
	kal_uint8 ClockDivider;
	kal_uint32 temp;
	
	SENSORDB("\n\n\n\n\n\n");
	SENSORDB("[HI257]HI257Capture!!!!!!!!!!!!!\n");
	SENSORDB("[HI257]Image Target Width: %d; Height: %d\n",image_window->ImageTargetWidth, image_window->ImageTargetHeight);

	if ((image_window->ImageTargetWidth<=HI257_PV_WIDTH)&&
		(image_window->ImageTargetHeight<=HI257_PV_HEIGHT))
		return ERROR_NONE;    /* Less than PV Mode */

	HI257WriteCmosSensor(0x03,0x00);
	HI257WriteCmosSensor(0x10,0x00);
	HI257SetPage(0x20);

	HI257Status.PvShutter = (HI257ReadCmosSensor(0x80) << 16)|(HI257ReadCmosSensor(0x81) << 8)|HI257ReadCmosSensor(0x82);

	CapShutter = HI257Status.PvShutter;
	HI257SetPage(0x20);
	HI257WriteCmosSensor(0x83, (CapShutter >> 16) & 0xFF);
	HI257WriteCmosSensor(0x84, (CapShutter >> 8) & 0xFF);
	HI257WriteCmosSensor(0x85, CapShutter & 0xFF);  
	HI257SetAeMode(KAL_FALSE);
	HI257SetAwbMode(KAL_FALSE);
	CAPTURE_FLAG = 1;
	CAPTURE_FLAGA = 1;
	return ERROR_NONE;
} /* HI257Capture() */
/* HI257Capture() */

UINT32 HI257GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	pSensorResolution->SensorFullWidth = HI257_FULL_WIDTH;
	pSensorResolution->SensorFullHeight = HI257_FULL_HEIGHT;
	pSensorResolution->SensorPreviewWidth = HI257_PV_WIDTH;
	pSensorResolution->SensorPreviewHeight = HI257_PV_HEIGHT;
	pSensorResolution->SensorVideoWidth=HI257_PV_WIDTH;
	pSensorResolution->SensorVideoHeight = HI257_PV_HEIGHT;
	return ERROR_NONE;
} /* HI257GetResolution() */

UINT32 HI257GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	pSensorInfo->SensorPreviewResolutionX=HI257_PV_WIDTH;
	pSensorInfo->SensorPreviewResolutionY=HI257_PV_HEIGHT;
	pSensorInfo->SensorFullResolutionX=HI257_FULL_WIDTH;
	pSensorInfo->SensorFullResolutionY=HI257_FULL_HEIGHT;
	pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=20;
	pSensorInfo->SensorStillCaptureFrameRate=10;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE;
	pSensorInfo->SensorResetDelayCount=1;
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV; // back for 16 SENSOR_OUTPUT_FORMAT_UYVY;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorInterruptDelayLines = 1;
	pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;
	/*
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].BinningEnable=FALSE;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].BinningEnable=FALSE;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].BinningEnable=FALSE;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxWidth=CAM_SIZE_1M_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxHeight=CAM_SIZE_1M_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].BinningEnable=TRUE;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxWidth=CAM_SIZE_1M_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxHeight=CAM_SIZE_1M_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].BinningEnable=TRUE;
	*/
	pSensorInfo->CaptureDelayFrame = 3; 
	pSensorInfo->PreviewDelayFrame = 3; 
	pSensorInfo->VideoDelayFrame = 4; 
	pSensorInfo->SensorMasterClockSwitch = 0; 
	pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_2MA; 

	switch (ScenarioId)
	{
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		//  case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		// case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
	default:
		pSensorInfo->SensorClockFreq=24;
		pSensorInfo->SensorClockDividCount=3;
		pSensorInfo->SensorClockRisingCount=0;
		pSensorInfo->SensorClockFallingCount=2;
		pSensorInfo->SensorPixelClockCount=3;
		pSensorInfo->SensorDataLatchCount=2;
		pSensorInfo->SensorGrabStartX = HI257_GRAB_START_X; 
		pSensorInfo->SensorGrabStartY = HI257_GRAB_START_Y;
		break;
	}
	return ERROR_NONE;
} /* HI257GetInfo() */


UINT32 HI257Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	switch (ScenarioId)
	{
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		// case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
		HI257Preview(pImageWindow, pSensorConfigData);
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:

		HI257Video(pImageWindow, pSensorConfigData);
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	case MSDK_SCENARIO_ID_CAMERA_ZSD:
		//  case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
		HI257Capture(pImageWindow, pSensorConfigData);
		break;
	default:
		break; 
	}
	return TRUE;
} /* HI257Control() */


BOOL HI257SetWb(UINT16 Para)
{
	SENSORDB("[HI257]HI257SetWb Para:%d;\n",Para);
	
	switch (Para)
	{
	case AWB_MODE_OFF:
		HI257SetAwbMode(KAL_FALSE);
		break;                     
	case AWB_MODE_AUTO:
		HI257WriteCmosSensor(0x03, 0x22);
		//HI257WriteCmosSensor(0x80, 0x3d);
		//HI257WriteCmosSensor(0x81, 0x20);
		//HI257WriteCmosSensor(0x82, 0x32);
		HI257WriteCmosSensor(0x83, 0x50);
		HI257WriteCmosSensor(0x84, 0x15);
		HI257WriteCmosSensor(0x85, 0x50);
		HI257WriteCmosSensor(0x86, 0x20);      

		break;
	case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy
		HI257WriteCmosSensor(0x03, 0x22);
		HI257WriteCmosSensor(0x80, 0x49);
		HI257WriteCmosSensor(0x81, 0x20);
		HI257WriteCmosSensor(0x82, 0x24);
		HI257WriteCmosSensor(0x83, 0x50);
		HI257WriteCmosSensor(0x84, 0x45);
		HI257WriteCmosSensor(0x85, 0x24);
		HI257WriteCmosSensor(0x86, 0x1e);
		break;
	case AWB_MODE_DAYLIGHT: //sunny
		HI257WriteCmosSensor(0x03, 0x22);
		HI257WriteCmosSensor(0x80, 0x40);
		HI257WriteCmosSensor(0x81, 0x20);
		HI257WriteCmosSensor(0x82, 0x27);
		HI257WriteCmosSensor(0x83, 0x44);
		HI257WriteCmosSensor(0x84, 0x3f);
		HI257WriteCmosSensor(0x85, 0x29);
		HI257WriteCmosSensor(0x86, 0x23);
		break;
	case AWB_MODE_INCANDESCENT: //office
		HI257WriteCmosSensor(0x03, 0x22);
		HI257WriteCmosSensor(0x80, 0x20);
		HI257WriteCmosSensor(0x81, 0x20);
		HI257WriteCmosSensor(0x82, 0x47);
		HI257WriteCmosSensor(0x83, 0x22);
		HI257WriteCmosSensor(0x84, 0x1e);
		HI257WriteCmosSensor(0x85, 0x50);
		HI257WriteCmosSensor(0x86, 0x45);
		break;
	case AWB_MODE_TUNGSTEN: //home
		HI257WriteCmosSensor(0x03, 0x22);
		HI257WriteCmosSensor(0x80, 0x36);
		HI257WriteCmosSensor(0x81, 0x20);
		HI257WriteCmosSensor(0x82, 0x36);
		HI257WriteCmosSensor(0x83, 0x38);
		HI257WriteCmosSensor(0x84, 0x32);
		HI257WriteCmosSensor(0x85, 0x39);
		HI257WriteCmosSensor(0x86, 0x33);
		break;
	case AWB_MODE_FLUORESCENT:
		HI257WriteCmosSensor(0x03, 0x22);
		HI257WriteCmosSensor(0x80, 0x26);
		HI257WriteCmosSensor(0x81, 0x20);
		HI257WriteCmosSensor(0x82, 0x3f);
		HI257WriteCmosSensor(0x83, 0x2e);
		HI257WriteCmosSensor(0x84, 0x24);
		HI257WriteCmosSensor(0x85, 0x43);
		HI257WriteCmosSensor(0x86, 0x3d);
		break;
	default:
		return KAL_FALSE;
	}
	return KAL_TRUE;      
} /* HI257SetWb */

BOOL HI257SetEffect(UINT16 Para)
{
	SENSORDB("[HI253]HI257SetEffect Para:%d;\n",Para);
	
	switch (Para)
	{
	case MEFFECT_OFF:
		HI257WriteCmosSensor(0x03, 0x10);  
		HI257WriteCmosSensor(0x11, 0x03);
		HI257WriteCmosSensor(0x12, 0xf0);
		HI257WriteCmosSensor(0x13, 0x03);
		HI257WriteCmosSensor(0x42, 0x04);
		HI257WriteCmosSensor(0x43, 0x04);
		HI257WriteCmosSensor(0x44, 0x80);
		HI257WriteCmosSensor(0x45, 0x80);
		break;
	case MEFFECT_SEPIA:
		HI257WriteCmosSensor(0x03, 0x10);  
		HI257WriteCmosSensor(0x11, 0x03);
		HI257WriteCmosSensor(0x12, 0xd3);
		HI257WriteCmosSensor(0x13, 0x03);
		HI257WriteCmosSensor(0x42, 0x05);
		HI257WriteCmosSensor(0x43, 0x05);
		HI257WriteCmosSensor(0x44, 0x60);
		HI257WriteCmosSensor(0x45, 0xa3);
		break;
	case MEFFECT_NEGATIVE://----datasheet
		HI257WriteCmosSensor(0x03, 0x10);  
		HI257WriteCmosSensor(0x11, 0x03);
		HI257WriteCmosSensor(0x12, 0xd8);
		HI257WriteCmosSensor(0x13, 0x03);
		HI257WriteCmosSensor(0x42, 0x05);
		HI257WriteCmosSensor(0x43, 0x05);
		HI257WriteCmosSensor(0x44, 0x80);
		HI257WriteCmosSensor(0x45, 0x80);
		break;
	case MEFFECT_SEPIAGREEN://----datasheet aqua
		HI257WriteCmosSensor(0x03, 0x10);  
		HI257WriteCmosSensor(0x11, 0x03);
		HI257WriteCmosSensor(0x12, 0xd3);
		HI257WriteCmosSensor(0x13, 0x03);
		HI257WriteCmosSensor(0x42, 0x05);
		HI257WriteCmosSensor(0x43, 0x05);
		HI257WriteCmosSensor(0x44, 0x80);
		HI257WriteCmosSensor(0x45, 0x04);
		break;
	case MEFFECT_SEPIABLUE:
		HI257WriteCmosSensor(0x03, 0x10);  
		HI257WriteCmosSensor(0x11, 0x03);
		HI257WriteCmosSensor(0x12, 0xd3);
		HI257WriteCmosSensor(0x13, 0x03);
		HI257WriteCmosSensor(0x42, 0x05);
		HI257WriteCmosSensor(0x43, 0x05);
		HI257WriteCmosSensor(0x44, 0xb0);
		HI257WriteCmosSensor(0x45, 0x40);
		break;
	case MEFFECT_MONO: //----datasheet black & white
		HI257WriteCmosSensor(0x03, 0x10);  
		HI257WriteCmosSensor(0x11, 0x03);
		HI257WriteCmosSensor(0x12, 0xd3);
		HI257WriteCmosSensor(0x13, 0x03);
		HI257WriteCmosSensor(0x42, 0x05);
		HI257WriteCmosSensor(0x43, 0x05);
		HI257WriteCmosSensor(0x44, 0x80);
		HI257WriteCmosSensor(0x45, 0x80);
		break;
	default:
		return KAL_FALSE;
	}
	return KAL_TRUE;
} /* HI257SetEffect */


BOOL HI257setiso(UINT16 para)
{

	SENSORDB("[HI257]CONTROLFLOW HI257_set_iso Para:%d;\n",para);
	
	switch (para)
	{
	case AE_ISO_100:
		//ISO100
		HI257WriteCmosSensor(0x03, 0x20); 
		HI257WriteCmosSensor(0xb2, 0x20); 
		break;
	case AE_ISO_200:
		//ISO200
		HI257WriteCmosSensor(0x03, 0x20); 
		HI257WriteCmosSensor(0xb2, 0x40); 
		break;
	case AE_ISO_400:
		//ISO400
		HI257WriteCmosSensor(0x03, 0x20); 
		HI257WriteCmosSensor(0xb2, 0x60); 
		break;
	case AE_ISO_800:
		//ISO800
		HI257WriteCmosSensor(0x03, 0x20); 
		HI257WriteCmosSensor(0xb2, 0x90); 
		break;
	case AE_ISO_1600:
		//ISO1600
		HI257WriteCmosSensor(0x03, 0x20); 
		HI257WriteCmosSensor(0xb2, 0xb0); 
		break;
	default:
	case AE_ISO_AUTO://
		//Auto/
		HI257WriteCmosSensor(0x03, 0x20); 
		HI257WriteCmosSensor(0xb2, 0x60); 

		break;
	}
	return;

}

BOOL HI257SetBanding(UINT16 Para)
{
	SENSORDB("[HI257]HI257SetBanding Para:%d;\n",Para);

	spin_lock(&hi257_drv_lock);
	HI257Status.Banding = Para;
	spin_unlock(&hi257_drv_lock);

	if (HI257Status.Banding == AE_FLICKER_MODE_50HZ) 
	{
		spin_lock(&hi257_drv_lock);
		HI257Status.AECTL1 |= 0x10;
		HI257WriteCmosSensor(0x03, 0x20); //Page 20

		HI257WriteCmosSensor(0x10, 0x1c);
		HI257WriteCmosSensor(0x18, 0x38);

		HI257WriteCmosSensor(0x83, 0x0a);  //EXP Normal 10.33 fps 
		HI257WriteCmosSensor(0x84, 0x12); 
		HI257WriteCmosSensor(0x85, 0x20); 

		HI257WriteCmosSensor(0x10, 0x9c);
		HI257WriteCmosSensor(0x18, 0x30);
		spin_unlock(&hi257_drv_lock);
	}
	else
	{
		spin_lock(&hi257_drv_lock);
		HI257Status.AECTL1 &= (~0x10); 

		HI257WriteCmosSensor(0x03, 0x20); //Page 20
		HI257WriteCmosSensor(0x10, 0x0c);
		HI257WriteCmosSensor(0x18, 0x38);

		HI257WriteCmosSensor(0x83, 0x09);  //EXP Normal 8.33 fps 
		HI257WriteCmosSensor(0x84, 0xeb); 
		HI257WriteCmosSensor(0x85, 0x10); 

		HI257WriteCmosSensor(0x10, 0x8c);
		HI257WriteCmosSensor(0x18, 0x30);
		spin_unlock(&hi257_drv_lock);
	}
	HI257SetPage(0x20);  
	HI257WriteCmosSensor(0x10,HI257Status.AECTL1);  

	return TRUE;
} /* HI257SetBanding */

BOOL HI257SetExposure(UINT16 Para)
{
	SENSORDB("[HI257]HI257SetExposure Para:%d;\n",Para);

	HI257SetPage(0x10);  

	HI257Status.ISPCTL3 = HI257ReadCmosSensor(0x12);

	spin_lock(&hi257_drv_lock);
	HI257Status.ISPCTL3 |= 0x10;
	spin_unlock(&hi257_drv_lock);

	HI257WriteCmosSensor(0x12,HI257Status.ISPCTL3);//make sure the Yoffset control is opened.

	switch (Para)
	{
		//case AE_EV_COMP_n40:              /* EV -2 */
		//HI257WriteCmosSensor(0x40,0xe0);
		//break;
	case AE_EV_COMP_n30:              /* EV -1.5 */
		HI257WriteCmosSensor(0x40,0xc8);

		break;
	case AE_EV_COMP_n20:              /* EV -1 */
		HI257WriteCmosSensor(0x40,0xb0);
		break;
		
	case AE_EV_COMP_n10:              /* EV -0.5 */
		HI257WriteCmosSensor(0x40,0x98);
		break;
		
	case AE_EV_COMP_00:                /* EV 0 */
		HI257WriteCmosSensor(0x40, 0x88);
		break;
		
	case AE_EV_COMP_10:              /* EV +0.5 */
		HI257WriteCmosSensor(0x40,0x18);
		break;
		
	case AE_EV_COMP_20:              /* EV +1 */
		HI257WriteCmosSensor(0x40,0x30);
		break;
		
	case AE_EV_COMP_30:              /* EV +1.5 */
		HI257WriteCmosSensor(0x40,0x48);
		break;
		//case AE_EV_COMP_40:              /* EV +2 */
		//HI257WriteCmosSensor(0x40,0x60);
		//break;
	default:
		return KAL_FALSE;
	}
	return KAL_TRUE;
} /* HI257SetExposure */

UINT32 HI257YUVSensorSetting(FEATURE_ID Cmd, UINT32 Para)
{
	switch (Cmd) 
	{
	case FID_SCENE_MODE:
		if (Para == SCENE_MODE_OFF)
		{
			if( CAPTURE_FLAG == 0)
				HI257NightMode(KAL_FALSE); 
			else
				CAPTURE_FLAG = 0;
		}
		else if (Para == SCENE_MODE_NIGHTSCENE)
		{
			if( CAPTURE_FLAG == 0)
				HI257NightMode(KAL_TRUE); 
			else
				CAPTURE_FLAG = 0;
		}  
		break; 
	case FID_AWB_MODE:
		HI257SetWb(Para);
		break;
	case FID_COLOR_EFFECT:
		HI257SetEffect(Para);
		break;
	case FID_AE_EV:
		HI257SetExposure(Para);
		break;
	case FID_AE_FLICKER:
		if( CAPTURE_FLAGA == 0)
			HI257SetBanding(Para);
		else
			CAPTURE_FLAGA = 0;
		break;
	case FID_AE_ISO:
		SENSORDB("[HI257]HI257setiso:%d\n",Para);
		HI257setiso(Para);
		break;
	case FID_AE_SCENE_MODE: 
		if (Para == AE_MODE_OFF) 
		{
			HI257SetAeMode(KAL_FALSE);
		}
		else 
		{
			HI257SetAeMode(KAL_TRUE);
		}
		break; 
	case FID_ZOOM_FACTOR:
		SENSORDB("[HI257]ZoomFactor :%d;\n",Para);
		spin_lock(&hi257_drv_lock);
		HI257Status.ZoomFactor = Para;
		spin_unlock(&hi257_drv_lock);
		break;
	default:
		break;
	}
	return TRUE;
}   /* HI257YUVSensorSetting */

UINT32 HI257YUVSetVideoMode(UINT16 FrameRate)	
{
	return;
	kal_uint32 EXPFIX, BLC_TIME_TH_ONOFF;
	kal_uint32 LineLength,BandingValue;
	//  return TRUE;

	SENSORDB("[HI257]HI257YUVSetVideoMode FrameRate:%d;\n",FrameRate);

	if(FrameRate>=30)
	{
		FrameRate=30;
		//Video mode30
		SENSORDB("[HI257]HI257YUVSetVideoMode FrameRate:%d;\n",FrameRate);

		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x00, 0x01);
		HI257WriteCmosSensor(0x0e, 0x03);
		HI257WriteCmosSensor(0x10, 0x92); //Pre2
		HI257WriteCmosSensor(0x11, 0x93);//zhao.li@tcl 0x80

		HI257WriteCmosSensor(0x40, 0x01); //Hblank_284
		HI257WriteCmosSensor(0x41, 0x78);
		HI257WriteCmosSensor(0x42, 0x00); //Vblank
		HI257WriteCmosSensor(0x43, 0x14); //Flick Stop

		HI257WriteCmosSensor(0x90, 0x0b); //BLC_TIME_TH_ON
		HI257WriteCmosSensor(0x91, 0x0b); //BLC_TIME_TH_OFF
		HI257WriteCmosSensor(0x92, 0x58); //BLC_AG_TH_ON
		HI257WriteCmosSensor(0x93, 0x50); //BLC_AG_TH_OFF

		HI257WriteCmosSensor(0x03, 0x02);
		HI257WriteCmosSensor(0xd4, 0x0b); //DCDC_TIME_TH_ON
		HI257WriteCmosSensor(0xd5, 0x0b); //DCDC_TIME_TH_OFF 
		HI257WriteCmosSensor(0xd6, 0x58); //DCDC_AG_TH_ON
		HI257WriteCmosSensor(0xd7, 0x50); //DCDC_AG_TH_OFF

		HI257WriteCmosSensor(0x03, 0x20);
		HI257WriteCmosSensor(0x10, 0x1c);

		HI257WriteCmosSensor(0x03, 0x22);
		HI257WriteCmosSensor(0x10, 0x69);

		HI257WriteCmosSensor(0x03, 0x20);
		HI257WriteCmosSensor(0x2a, 0x34);
		HI257WriteCmosSensor(0x2b, 0x78);

		HI257WriteCmosSensor(0x83, 0x02); //EXP Normal 33.33 fps 
		HI257WriteCmosSensor(0x84, 0xbd); 
		HI257WriteCmosSensor(0x85, 0xee);

		HI257WriteCmosSensor(0x86, 0x01); 
		HI257WriteCmosSensor(0x87, 0x2a);

		//HI257WriteCmosSensor(0x88, 0x03); //EXP Max(120Hz) 30.00 fps 
		//HI257WriteCmosSensor(0x89, 0x09); 
		//HI257WriteCmosSensor(0x8a, 0xbc); 

		HI257WriteCmosSensor(0xa5, 0x02); //EXP Max(100Hz) 33.33 fps   
		HI257WriteCmosSensor(0xa6, 0xbd); 
		HI257WriteCmosSensor(0xa7, 0xee);  

		HI257WriteCmosSensor(0x91, 0x01); //EXP Fixed 30.00 fps   
		HI257WriteCmosSensor(0x92, 0x86);
		HI257WriteCmosSensor(0x93, 0x8b);

		HI257WriteCmosSensor(0x9c, 0x11);
		HI257WriteCmosSensor(0x9d, 0x76);

		HI257WriteCmosSensor(0x9e, 0x01);
		HI257WriteCmosSensor(0x9f, 0x2a);

		HI257WriteCmosSensor(0x03, 0x20);
		HI257WriteCmosSensor(0x10, 0x9c);

		HI257WriteCmosSensor(0x03, 0x22);
		HI257WriteCmosSensor(0x10, 0xe9);

		HI257WriteCmosSensor(0x03, 0x00);
		//HI257WriteCmosSensor(0x11, 0x84);

		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x0e, 0x03);
		HI257WriteCmosSensor(0x0e, 0x73);

		HI257WriteCmosSensor(0x03, 0x00); //Dummy
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x01, 0x00);
	}
	/*
	else if (FrameRate>15 &&(FrameRate<=24))
	{

	FrameRate=24;//Video mode 24
	SENSORDB("[HI257]HI257YUVSetVideoMode FrameRate:%d;\n",FrameRate);

	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x00, 0x01);
	HI257WriteCmosSensor(0x0e, 0x03);
	HI257WriteCmosSensor(0x10, 0x93); //Pre2
	HI257WriteCmosSensor(0x11, 0x80);

	HI257WriteCmosSensor(0x40, 0x01); //Hblank_284
	HI257WriteCmosSensor(0x41, 0x1c);
	HI257WriteCmosSensor(0x42, 0x00); //Vblank
	HI257WriteCmosSensor(0x43, 0x14); //Flick Stop

	HI257WriteCmosSensor(0x90, 0x04); //BLC_TIME_TH_ON
	HI257WriteCmosSensor(0x91, 0x04); //BLC_TIME_TH_OFF
	HI257WriteCmosSensor(0x92, 0xb8); //BLC_AG_TH_ON
	HI257WriteCmosSensor(0x93, 0xb0); //BLC_AG_TH_OFF

	HI257WriteCmosSensor(0x03, 0x02);
	HI257WriteCmosSensor(0xd4, 0x04); //DCDC_TIME_TH_ON
	HI257WriteCmosSensor(0xd5, 0x04); //DCDC_TIME_TH_OFF 
	HI257WriteCmosSensor(0xd6, 0xb8); //DCDC_AG_TH_ON
	HI257WriteCmosSensor(0xd7, 0xb0); //DCDC_AG_TH_OFF

	HI257WriteCmosSensor(0x03, 0x20);
	HI257WriteCmosSensor(0x10, 0x1c);

	HI257WriteCmosSensor(0x03, 0x22);
	HI257WriteCmosSensor(0x10, 0x69);

	HI257WriteCmosSensor(0x03, 0x20);
	HI257WriteCmosSensor(0x2a, 0x04);
	HI257WriteCmosSensor(0x2b, 0x78);

	HI257WriteCmosSensor(0x83, 0x02); //EXP Normal 33.33 fps 
	HI257WriteCmosSensor(0x84, 0xbe); 
	HI257WriteCmosSensor(0x85, 0x8a);

	HI257WriteCmosSensor(0x86, 0x01); 
	HI257WriteCmosSensor(0x87, 0x5c);

	//HI257WriteCmosSensor(0x88, 0x03); //EXP Max(120Hz) 30.00 fps 
	//HI257WriteCmosSensor(0x89, 0xa8); 
	//HI257WriteCmosSensor(0x8a, 0xb8); 

	HI257WriteCmosSensor(0xa5, 0x03); //EXP Max(100Hz) 25.00 fps   
	HI257WriteCmosSensor(0xa6, 0xa8); 
	HI257WriteCmosSensor(0xa7, 0xb8); 

	HI257WriteCmosSensor(0x91, 0x03); //EXP Fixed 30.00 fps   
	HI257WriteCmosSensor(0x92, 0xd0);
	HI257WriteCmosSensor(0x93, 0x77);

	HI257WriteCmosSensor(0x9c, 0x0a);
	HI257WriteCmosSensor(0x9d, 0xe0);

	HI257WriteCmosSensor(0x9e, 0x01);
	HI257WriteCmosSensor(0x9f, 0x5c);

	HI257WriteCmosSensor(0x03, 0x20);
	HI257WriteCmosSensor(0x10, 0x9c);

	HI257WriteCmosSensor(0x03, 0x22);
	HI257WriteCmosSensor(0x10, 0xe9);

	HI257WriteCmosSensor(0x03, 0x00);
	//HI257WriteCmosSensor(0x11, 0x84);

	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x0e, 0x03);
	HI257WriteCmosSensor(0x0e, 0x73);

	HI257WriteCmosSensor(0x03, 0x00); //Dummy
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x03, 0x00);
	HI257WriteCmosSensor(0x01, 0x00);
	}
	*/
	else
	{
		FrameRate=15;
		//Video mode 15

		SENSORDB("[HI257]HI257YUVSetVideoMode FrameRate:%d;\n",FrameRate);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x00, 0x01);
		HI257WriteCmosSensor(0x0e, 0x03);
		HI257WriteCmosSensor(0x10, 0x91); //Pre2
		HI257WriteCmosSensor(0x11, 0x83);//zhao.li@tcl 0x80

		HI257WriteCmosSensor(0x40, 0x01); //Hblank_284
		HI257WriteCmosSensor(0x41, 0x04);
		HI257WriteCmosSensor(0x42, 0x00); //Vblank
		HI257WriteCmosSensor(0x43, 0x14); //Flick Stop

		HI257WriteCmosSensor(0x90, 0x06); //BLC_TIME_TH_ON
		HI257WriteCmosSensor(0x91, 0x06); //BLC_TIME_TH_OFF
		HI257WriteCmosSensor(0x92, 0x58); //BLC_AG_TH_ON
		HI257WriteCmosSensor(0x93, 0x50); //BLC_AG_TH_OFF

		HI257WriteCmosSensor(0x03, 0x02);
		HI257WriteCmosSensor(0xd4, 0x06); //DCDC_TIME_TH_ON
		HI257WriteCmosSensor(0xd5, 0x06); //DCDC_TIME_TH_OFF 
		HI257WriteCmosSensor(0xd6, 0x58); //DCDC_AG_TH_ON
		HI257WriteCmosSensor(0xd7, 0x50); //DCDC_AG_TH_OFF

		HI257WriteCmosSensor(0x03, 0x20);
		HI257WriteCmosSensor(0x10, 0x1c);

		HI257WriteCmosSensor(0x03, 0x22);
		HI257WriteCmosSensor(0x10, 0x69);

		HI257WriteCmosSensor(0x03, 0x20);
		HI257WriteCmosSensor(0x2b, 0x34);
		HI257WriteCmosSensor(0x30, 0x78);

		HI257WriteCmosSensor(0x83, 0x05); //EXP Normal 16.67 fps 
		HI257WriteCmosSensor(0x84, 0x79); 
		HI257WriteCmosSensor(0x85, 0xf6); 
		HI257WriteCmosSensor(0x86, 0x01); //EXPMin 12738.85 fps
		HI257WriteCmosSensor(0x87, 0xd7); 
		HI257WriteCmosSensor(0x88, 0x06); //EXP Max 60hz 15.00 fps 
		HI257WriteCmosSensor(0x89, 0x18); 
		HI257WriteCmosSensor(0x8a, 0x30); 
		HI257WriteCmosSensor(0xa5, 0x05); //EXP Max 50hz 16.67 fps 
		HI257WriteCmosSensor(0xa6, 0x79); 
		HI257WriteCmosSensor(0xa7, 0xf6); 
		HI257WriteCmosSensor(0x8B, 0xe9); //EXP100 
		HI257WriteCmosSensor(0x8C, 0xa9); 
		HI257WriteCmosSensor(0x8D, 0xc3); //EXP120 
		HI257WriteCmosSensor(0x8E, 0x06); 
		HI257WriteCmosSensor(0x9c, 0x1b); //EXP Limit 849.26 fps 
		HI257WriteCmosSensor(0x9d, 0x99); 
		HI257WriteCmosSensor(0x9e, 0x01); //EXP Unit 
		HI257WriteCmosSensor(0x9f, 0xd7); 
		HI257WriteCmosSensor(0xa3, 0x00); //Outdoor Int 
		HI257WriteCmosSensor(0xa4, 0xc3); 

		HI257WriteCmosSensor(0x03, 0x20);
		HI257WriteCmosSensor(0x10, 0x9c);

		HI257WriteCmosSensor(0x03, 0x22);
		HI257WriteCmosSensor(0x10, 0xe9);

		HI257WriteCmosSensor(0x03, 0x00);
		//HI257WriteCmosSensor(0x11, 0x84);

		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x0e, 0x03);
		HI257WriteCmosSensor(0x0e, 0x73);

		HI257WriteCmosSensor(0x03, 0x00); //Dummy
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x03, 0x00);
		HI257WriteCmosSensor(0x01, 0x00);
	}
}

UINT32 HI257FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
						   UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

	switch (FeatureId)
	{
	case SENSOR_FEATURE_GET_RESOLUTION:
		*pFeatureReturnPara16++=HI257_FULL_WIDTH;
		*pFeatureReturnPara16=HI257_FULL_HEIGHT;
		*pFeatureParaLen=4;
		break;
	case SENSOR_FEATURE_GET_PERIOD:
		*pFeatureReturnPara16++=HI257_PV_PERIOD_PIXEL_NUMS+HI257Status.PvDummyPixels;
		*pFeatureReturnPara16=HI257_PV_PERIOD_LINE_NUMS+HI257Status.PvDummyLines;
		*pFeatureParaLen=4;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
		*pFeatureReturnPara32 = HI257Status.PvOpClk*2;
		*pFeatureParaLen=4;
		break;
	case SENSOR_FEATURE_SET_ESHUTTER:
		break;
	case SENSOR_FEATURE_SET_NIGHTMODE:
		if( CAPTURE_FLAG == 0)
		{
			HI257NightMode((BOOL) *pFeatureData16);
		}
		else
		{
			spin_lock(&hi257_drv_lock);
			CAPTURE_FLAG = 0;
			spin_unlock(&hi257_drv_lock);
		}
		break;
	case SENSOR_FEATURE_SET_GAIN:
	case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		HI257WriteCmosSensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		pSensorRegData->RegData = HI257ReadCmosSensor(pSensorRegData->RegAddr);
		break;
	case SENSOR_FEATURE_GET_CONFIG_PARA:
		*pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
		break;
	case SENSOR_FEATURE_SET_CCT_REGISTER:
	case SENSOR_FEATURE_GET_CCT_REGISTER:
	case SENSOR_FEATURE_SET_ENG_REGISTER:
	case SENSOR_FEATURE_GET_ENG_REGISTER:
	case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
	case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
	case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
	case SENSOR_FEATURE_GET_GROUP_INFO:
	case SENSOR_FEATURE_GET_ITEM_INFO:
	case SENSOR_FEATURE_SET_ITEM_INFO:
	case SENSOR_FEATURE_GET_ENG_INFO:
		break;
	case SENSOR_FEATURE_GET_GROUP_COUNT:
		*pFeatureReturnPara32++=0;
		*pFeatureParaLen=4;
		break; 
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
		// if EEPROM does not exist in camera module.
		*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
		*pFeatureParaLen=4;
		break;
	case SENSOR_FEATURE_SET_YUV_CMD:
		HI257YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
		break;
	case SENSOR_FEATURE_SET_VIDEO_MODE:
		HI257YUVSetVideoMode(*pFeatureData16);
		break; 
	case SENSOR_FEATURE_CHECK_SENSOR_ID:
		HI257GetSensorID(pFeatureReturnPara32); 
		break; 
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		HI257SetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		HI257GetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
		break;
	case SENSOR_FEATURE_GET_TRIGGER_FLASHLIGHT_INFO:
		HI257_FlashTriggerCheck(pFeatureData32);
		SENSORDB("[5EA] F_GET_TRIGGER_FLASHLIGHT_INFO: %d\n", pFeatureData32);
		break;

	default:
		break;
	}
	return ERROR_NONE;
} /* HI257FeatureControl() */

SENSOR_FUNCTION_STRUCT SensorFuncHI257=
{
	HI257Open,
	HI257GetInfo,
	HI257GetResolution,
	HI257FeatureControl,
	HI257Control,
	HI257Close
};

UINT32 HI257_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{   
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncHI257;
	return ERROR_NONE;
} /* SensorInit() */

