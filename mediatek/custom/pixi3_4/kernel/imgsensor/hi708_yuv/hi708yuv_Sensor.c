#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
//#include <mach/mt6516_pll.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "hi708yuv_Sensor.h"
#include "hi708yuv_Camera_Sensor_para.h"
#include "hi708yuv_CameraCustomized.h"

#define HI708YUV_DEBUG
#ifdef HI708YUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

static UINT32 first_constant_focus = 0;
static UINT32 single_focus_mode = 0;

#if 0
extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
static int sensor_id_fail = 0; 
#define HI708_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para ,1,HI708_WRITE_ID)
#define HI708_write_cmos_sensor_2(addr, para, bytes) iWriteReg((u16) addr , (u32) para ,bytes,HI708_WRITE_ID)
kal_uint16 HI708_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
	iReadReg((u16) addr ,(u8*)&get_byte,HI708_WRITE_ID);
	return get_byte;
}

#endif
static DEFINE_SPINLOCK(hi708_yuv_drv_lock);
static int CAPTURE_FLAG = 0;
static int CAPTUREB_FLAG = 0;//Add By Paul

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
kal_uint16 HI708_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
	char puSendCmd[2] = {(char)(addr & 0xFF) , (char)(para & 0xFF)};
	iWriteRegI2C(puSendCmd , 2,HI708_WRITE_ID);
	return 0;
}
kal_uint16 HI708_read_cmos_sensor(kal_uint8 addr)
{
	kal_uint16 get_byte=0;
	char puSendCmd = { (char)(addr & 0xFF) };
	iReadRegI2C(&puSendCmd , 1, (u8*)&get_byte,1,HI708_WRITE_ID);
	return get_byte;
}


/*******************************************************************************
* // Adapter for Winmo typedef 
********************************************************************************/
#define WINMO_USE 0

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT


/*******************************************************************************
* follow is define by jun
********************************************************************************/
MSDK_SENSOR_CONFIG_STRUCT HI708SensorConfigData;

static struct HI708_sensor_STRUCT HI708_sensor;
static kal_uint32 HI708_zoom_factor = 0; 
static int sensor_id_fail = 0;	
const HI708_SENSOR_INIT_INFO HI708_Initial_Setting_Info[] =
{

	//PAGE 0
	//Image Size/Windowing/HSYNC/VSYNC[Type1]
	{0x03, 0x00},   //PAGEMODE(0x03)
	{0x01, 0xf1},
	{0x01, 0xf3},   //PWRCTL(0x01[P0])Bit[1]:Software Reset.
	{0x01, 0xf1},

	{0x11, 0x90},   //For No Fixed Framerate Bit[2]
	{0x12, 0x05},  //PCLK INV

	{0x20, 0x00},
	{0x21, 0x04},
	{0x22, 0x00},
	{0x23, 0x04},

	{0x24, 0x01},
	{0x25, 0xe0},
	{0x26, 0x02},
	{0x27, 0x80},

	{0x40, 0x01},   //HBLANK: 0x70 = 112
	{0x41, 0x58},
	{0x42, 0x00},   //VBLANK: 0x40 = 64
	{0x43, 0x14},   //0x04 -> 0x40: For Max Framerate = 30fps

	//BLC
	{0x80, 0x2e},
	{0x81, 0x7e},
	{0x82, 0x90},
	{0x83, 0x30},
	{0x84, 0x2c},
	{0x85, 0x4b},
	{0x89, 0x48},

	{0x90, 0x0c},
	{0x91, 0x0c},    
	{0x92, 0x48},
	{0x93, 0x40},
	{0x98, 0x38},
	{0x99, 0x40},
	{0xa0, 0x00},
	{0xa8, 0x40},

	//PAGE 2
	//Analog Circuit
	{0x03, 0x02},      
	{0x13, 0x40},
	{0x14, 0x04},
	{0x1a, 0x00},
	{0x1b, 0x08},

	{0x20, 0x33},
	{0x21, 0xaa},
	{0x22, 0xa7},
	{0x23, 0xb1},       //For Sun Pot

	{0x3b, 0x48},

	{0x50, 0x21},
	{0x52, 0xa2},
	{0x53, 0x0a},
	{0x54, 0x30},
	{0x55, 0x10},
	{0x56, 0x0c},
	{0x59, 0x0F},

	{0x60, 0x54},
	{0x61, 0x5d},
	{0x62, 0x56},
	{0x63, 0x5c},
	{0x64, 0x56},
	{0x65, 0x5c},
	{0x72, 0x57},
	{0x73, 0x5b},
	{0x74, 0x57},
	{0x75, 0x5b},
	{0x80, 0x02},
	{0x81, 0x46},
	{0x82, 0x07},
	{0x83, 0x10},
	{0x84, 0x07},
	{0x85, 0x10},
	{0x92, 0x24},
	{0x93, 0x30},
	{0x94, 0x24},
	{0x95, 0x30},
	{0xa0, 0x03},
	{0xa1, 0x45},
	{0xa4, 0x45},
	{0xa5, 0x03},
	{0xa8, 0x12},
	{0xa9, 0x20},
	{0xaa, 0x34},
	{0xab, 0x40},
	{0xb8, 0x55},
	{0xb9, 0x59},
	{0xbc, 0x05},
	{0xbd, 0x09},
	{0xc0, 0x5f},
	{0xc1, 0x67},
	{0xc2, 0x5f},
	{0xc3, 0x67},
	{0xc4, 0x60},
	{0xc5, 0x66},
	{0xc6, 0x60},
	{0xc7, 0x66},
	{0xc8, 0x61},
	{0xc9, 0x65},
	{0xca, 0x61},
	{0xcb, 0x65},
	{0xcc, 0x62},
	{0xcd, 0x64},
	{0xce, 0x62},
	{0xcf, 0x64},
	{0xd0, 0x53},
	{0xd1, 0x68},

	//PAGE 10
	//Image Format, Image Effect
	{0x03, 0x10},
	{0x10, 0x00},
	{0x11, 0x43},
	{0x12, 0xf0},

	{0x40, 0x80},
	{0x41, 0x02},
	{0x42, 0x00},
	{0x43, 0x00},
	{0x48, 0x80},

	{0x50, 0x48},

	{0x60, 0x67},
	{0x61, 0x00},
	{0x62, 0x80},//90
	{0x63, 0x80},//88
	{0x64, 0x48},
	{0x66, 0x90},
	{0x67, 0x67},

	//PAGE 11
	//Z-LPF
	{0x03, 0x11},
	{0x10, 0x25},   
	{0x11, 0x1f},   

	{0x20, 0x00},   
	{0x21, 0x38},   
	{0x23, 0x0a},

	{0x60, 0x10},   
	{0x61, 0x82},
	{0x62, 0x00},   
	{0x63, 0x83},   
	{0x64, 0x83},      
	{0x67, 0xF0},   
	{0x68, 0x40},   
	{0x69, 0x10},   

	//PAGE 12
	//2D
	{0x03, 0x12},

	{0x40, 0xe9},
	{0x41, 0x09},

	{0x50, 0x18},
	{0x51, 0x24},

	{0x70, 0x1f},
	{0x71, 0x00},
	{0x72, 0x00},
	{0x73, 0x00},
	{0x74, 0x10},
	{0x75, 0x10},
	{0x76, 0x20},
	{0x77, 0x80},
	{0x78, 0x88},
	{0x79, 0x18},

	{0xb0, 0x7d},

	//PAGE 13
	//Edge Enhancement
	{0x03, 0x13},
	{0x10, 0x01},   
	{0x11, 0x89},   
	{0x12, 0x14},   
	{0x13, 0x19},   
	{0x14, 0x08},

	{0x20, 0x07},//06
	{0x21, 0x05},//03
	{0x23, 0x30},
	{0x24, 0x33},
	{0x25, 0x08},
	{0x26, 0x18},
	{0x27, 0x00},
	{0x28, 0x08},
	{0x29, 0x50},
	{0x2a, 0xe0},
	{0x2b, 0x10},
	{0x2c, 0x28},
	{0x2d, 0x40},
	{0x2e, 0x00},
	{0x2f, 0x00},

	//PAGE 11
	{0x30, 0x11},

	{0x80, 0x03},
	{0x81, 0x07},

	{0x90, 0x07},//06
	{0x91, 0x05},	//SHARP2D_DIFF_CTL
	{0x92, 0x00},
	{0x93, 0x20},
	{0x94, 0x42},
	{0x95, 0x60},

	//PAGE 14
	//Lens Shading Correction
	{0x03, 0x14},
	{0x10, 0x01},

	{0x20, 0x80},   //For Y decay
	{0x21, 0x80},   //For Y decay
	{0x22, 0x60},
	{0x23, 0x4d},
	{0x24, 0x46},

	//PAGE 15 
	//Color Correction
	{0x03, 0x15}, 
	{0x10, 0x03},         
	{0x14, 0x3c},
	{0x16, 0x2c},
	{0x17, 0x2f},


	{0x30, 0xcb},
	{0x31, 0x61},
	{0x32, 0x16},
	{0x33, 0x23},
	{0x34, 0xce},
	{0x35, 0x2b},
	{0x36, 0x01},
	{0x37, 0x34},
	{0x38, 0x75},

	{0x40, 0x87},
	{0x41, 0x18},
	{0x42, 0x91},
	{0x43, 0x94},
	{0x44, 0x9f},
	{0x45, 0x33},
	{0x46, 0x00},
	{0x47, 0x94},
	{0x48, 0x14},

	//PAGE 16
	//Gamma Correction
	{0x03,  0x16},

	{0x30, 0x00},
	{0x31, 0x0a},
	{0x32, 0x1b},
	{0x33, 0x2e},
	{0x34, 0x5c},
	{0x35, 0x79},
	{0x36, 0x95},
	{0x37, 0xa4},
	{0x38, 0xb1},
	{0x39, 0xbd},
	{0x3a, 0xc8},
	{0x3b, 0xd9},
	{0x3c, 0xe8},
	{0x3d, 0xf5},
	{0x3e, 0xff},

	//PAGE 17 
	//Auto Flicker Cancellation 
	{0x03, 0x17},

	{0xc4, 0x3c},
	{0xc5, 0x32},

	//PAGE 20 
	//AE 
	{0x03, 0x20},

	{0x10, 0x0c},
	{0x11, 0x04},

	{0x20, 0x01},
	{0x28, 0x27},
	{0x29, 0xa1},   
	{0x2a, 0xf0},
	{0x2b, 0x34},
	{0x2c, 0x2b},

	{0x30, 0xf8},
	{0x39, 0x22},
	{0x3a, 0xde},
	{0x3b, 0x22},
	{0x3c, 0xde},

	{0x60, 0x95},
	{0x68, 0x3c},
	{0x69, 0x64},
	{0x6A, 0x28},
	{0x6B, 0xc8},

	{0x70, 0x42},   //For Y decay   
	{0x76, 0x22},
	{0x77, 0x02},   
	{0x78, 0x12},
	{0x79, 0x27},
	{0x7a, 0x23},  
	{0x7c, 0x1d},
	{0x7d, 0x22},

	{0x83, 0x01}, //EXP Normal 10.33 fps 
	{0x84, 0x7c}, 
	{0x85, 0xdc}, 
	{0x86, 0x00}, //EXPMin 3250.00 fps
	{0x87, 0xfa},
	{0x88, 0x01}, //EXP Max 10.33 fps 
	{0x89, 0x7c}, 
	{0x8a, 0xdc}, 
	{0x8B, 0x1f}, //EXP100 
	{0x8C, 0xbd}, 
	{0x8D, 0x1a}, //EXP120 
	{0x8E, 0x5e}, 
	{0x9c, 0x0b}, //EXP Limit 282.61 fps 
	{0x9d, 0x3b}, 
	{0x9e, 0x00}, //EXP Unit 
	{0x9f, 0xfa}, 
	{0xa3, 0x00}, //Outdoor Int 
	{0xa4, 0x1a}, 

	{0x91, 0x02},
	{0x92, 0xdc},
	{0x93, 0x6c},   
	{0x94, 0x01},
	{0x95, 0xb7},
	{0x96, 0x74},   
	{0x98, 0x8C},
	{0x99, 0x23},  


	{0xb1, 0x14},
	{0xb2, 0x50},
	{0xb4, 0x14},
	{0xb5, 0x38},
	{0xb6, 0x26},
	{0xb7, 0x20},
	{0xb8, 0x1d},
	{0xb9, 0x1b},
	{0xba, 0x1a},
	{0xbb, 0x19},
	{0xbc, 0x19},
	{0xbd, 0x18},

	{0xc0, 0x16},   //0x1a->0x16
	{0xc3, 0x48},
	{0xc4, 0x48}, 

	//PAGE 22 
	//AWB
	{0x03, 0x22},
	{0x10, 0xe2},
	{0x11, 0x26},

	{0x21, 0x40},

	{0x30, 0x80},
	{0x31, 0x80},
	{0x38, 0x12},
	{0x39, 0x33},
	{0x3a, 0x88},
	{0x3b, 0xc4},
	{0x40, 0xf0},
	{0x41, 0x33},
	{0x42, 0x33},
	{0x43, 0xf3},
	{0x44, 0x55},
	{0x45, 0x44},
	{0x46, 0x02},
	{0x60, 0x00},
	{0x61, 0x00},

	{0x80, 0x3a},
	{0x81, 0x20},
	{0x82, 0x40},
	{0x83, 0x52}, //RMAX Default : 50 -> 48 -> 52 
	{0x84, 0x1b}, //RMIN Default : 20
	{0x85, 0x55}, //BMAX Default : 50, 5a -> 58 -> 55
	{0x86, 0x2e}, //BMIN Default : 20
	{0x87, 0x40}, //RMAXB Default : 50, 4d
	{0x88, 0x2a}, //RMINB Default : 3e, 45 --> 42
	{0x89, 0x40}, //BMAXB Default : 2e, 2d --> 30//3e
	{0x8a, 0x30}, //BMINB Default : 20, 22 --> 26 --> 29
	{0x8b, 0x07}, //OUT TH//02
	{0x8d, 0x22},
	{0x8e, 0x71},  
	{0x8f, 0x63},

	{0x90, 0x60},
	{0x91, 0x5c},

    {0x92, 0x54},//0x56
    {0x93, 0x4b},//0x52
    {0x94, 0x41},//0x4c
    {0x95, 0x38},//0x36
    {0x96, 0x31},//0x31
    {0x97, 0x2d},//0x2e
    {0x98, 0x2a},//0x2a
	{0x99, 0x29},
	{0x9a, 0x26},
    {0x9b, 0x07},


	//PAGE 22
	{0x03, 0x22},
	{0x10, 0xfb},

	//PAGE 20
	{0x03, 0x20},
	{0x10, 0x9c},

	{0x01, 0xf0},

	//PAGE 0
	{0x03, 0x00},
	{0x01, 0x90},   //0xf1 ->0x41 : For Preview Green/Red Line.

	{0xff, 0xff}    //End of Initial Setting


};



// 640 * 480
static void HI708_Set_VGA_mode(void)
{
	HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)|0x01);   //Sleep: For Write Reg

	HI708_write_cmos_sensor(0x03, 0x00);
	HI708_write_cmos_sensor(0x10, 0x00);        //VGA Size

	HI708_write_cmos_sensor(0x20, 0x00);
	HI708_write_cmos_sensor(0x21, 0x04);

	HI708_write_cmos_sensor(0x40, 0x01);        //HBLANK: 0x70 = 112
	HI708_write_cmos_sensor(0x41, 0x58);
	HI708_write_cmos_sensor(0x42, 0x00);        //VBLANK: 0x04 = 4
	HI708_write_cmos_sensor(0x43, 0x14);

	HI708_write_cmos_sensor(0x03, 0x11);
	HI708_write_cmos_sensor(0x10, 0x25);  

	HI708_write_cmos_sensor(0x03, 0x20);

	HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)&0x7f);   //Close AE
	HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)|0x08);   //Reset AE

	//HI708_write_cmos_sensor(0x83, 0x01);
	//HI708_write_cmos_sensor(0x84, 0x7c);
	//HI708_write_cmos_sensor(0x85, 0xdc);
	HI708_write_cmos_sensor(0x86, 0x00);
	HI708_write_cmos_sensor(0x87, 0xfa);

	HI708_write_cmos_sensor(0x8b, 0x1f);
	HI708_write_cmos_sensor(0x8c, 0xbd);
	HI708_write_cmos_sensor(0x8d, 0x1a);
	HI708_write_cmos_sensor(0x8e, 0x5e);

	HI708_write_cmos_sensor(0x9c, 0x0b);
	HI708_write_cmos_sensor(0x9d, 0x3b);
	HI708_write_cmos_sensor(0x9e, 0x00);
	HI708_write_cmos_sensor(0x9f, 0xfa);

	HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)&0xfe);   //Exit Sleep: For Write Reg

	HI708_write_cmos_sensor(0x03, 0x20);
	HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)|0x80);   //Open AE
	HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)&0xf7);   //Reset AE

}

static void HI708_Initial_Setting(void)
{
	kal_uint32 iEcount;
	for(iEcount=0;(!((0xff==(HI708_Initial_Setting_Info[iEcount].address))&&(0xff==(HI708_Initial_Setting_Info[iEcount].data))));iEcount++)
	{	
		HI708_write_cmos_sensor(HI708_Initial_Setting_Info[iEcount].address, HI708_Initial_Setting_Info[iEcount].data);
	}
	HI708_Set_VGA_mode();

}

static void HI708_Init_Parameter(void)
{
	spin_lock(&hi708_yuv_drv_lock);
	HI708_sensor.first_init = KAL_TRUE;
	HI708_sensor.pv_mode = KAL_TRUE;
	HI708_sensor.night_mode = KAL_FALSE;
	HI708_sensor.MPEG4_Video_mode = KAL_FALSE;

	HI708_sensor.cp_pclk = HI708_sensor.pv_pclk;

	HI708_sensor.pv_dummy_pixels = 0;
	HI708_sensor.pv_dummy_lines = 0;
	HI708_sensor.cp_dummy_pixels = 0;
	HI708_sensor.cp_dummy_lines = 0;

	HI708_sensor.wb = 0;
	HI708_sensor.exposure = 0;
	HI708_sensor.effect = 0;
	HI708_sensor.banding = AE_FLICKER_MODE_50HZ;

	HI708_sensor.pv_line_length = 640;
	HI708_sensor.pv_frame_height = 480;
	HI708_sensor.cp_line_length = 640;
	HI708_sensor.cp_frame_height = 480;
	spin_unlock(&hi708_yuv_drv_lock);    
}

static kal_uint8 HI708_power_on(void)
{
	kal_uint8 HI708_sensor_id = 0;
	spin_lock(&hi708_yuv_drv_lock);
	HI708_sensor.pv_pclk = 13000000;
	spin_unlock(&hi708_yuv_drv_lock);
	//Software Reset
	HI708_write_cmos_sensor(0x01,0xf1);
	HI708_write_cmos_sensor(0x01,0xf3);
	HI708_write_cmos_sensor(0x01,0xf1);

	/* Read Sensor ID  */
	HI708_sensor_id = HI708_read_cmos_sensor(0x04);
	SENSORDB("[HI708YUV]:read Sensor ID:%x\n",HI708_sensor_id);	
	return HI708_sensor_id;
}


/*************************************************************************
* FUNCTION
*	HI708Open
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
UINT32 HI708Open(void)
{
	spin_lock(&hi708_yuv_drv_lock);
	sensor_id_fail = 0; 
	spin_unlock(&hi708_yuv_drv_lock);
	SENSORDB("[Enter]:HI708 Open func:");

	if (HI708_power_on() != HI708_SENSOR_ID) 
	{
		SENSORDB("[HI708]Error:read sensor ID fail\n");
		spin_lock(&hi708_yuv_drv_lock);
		sensor_id_fail = 1;
		spin_unlock(&hi708_yuv_drv_lock);
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	/* Apply sensor initail setting*/
	HI708_Initial_Setting();
	HI708_Init_Parameter(); 

	first_constant_focus = 1;

	SENSORDB("[Exit]:HI708 Open func\n");     
	return ERROR_NONE;
}	/* HI708Open() */

/*************************************************************************
* FUNCTION
*	HI708_GetSensorID
*
* DESCRIPTION
*	This function get the sensor ID
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

//BEGIN: add by fangjie for show camera info in MINISW screen, FR803765
extern char Front_Camera_Name[256];  
//END: add by fangjie for show camera info in MINISW screen, FR803765
static kal_uint32 HI708_GetSensorID(kal_uint32 *sensorID)
{
	SENSORDB("[Enter]:HI708 Open func ");
	*sensorID = HI708_power_on() ;

	if (*sensorID != HI708_SENSOR_ID) 
	{
		SENSORDB("[HI708]Error:read sensor ID fail\n");
		spin_lock(&hi708_yuv_drv_lock);
		sensor_id_fail = 1;
		spin_unlock(&hi708_yuv_drv_lock);
		*sensorID = 0xFFFFFFFF; 
		return ERROR_SENSOR_CONNECT_FAIL;
	}	   
	//BEGIN: add by fangjie for show camera info in MINISW screen, FR803765
	sprintf(Front_Camera_Name,"Vender:  E-welly \nSensor ID:  0x%X \nSensor name:  HI708 \nResolution:  0.3M ",
		HI708_SENSOR_ID);
	//END: add by fangjie for show camera info in MINISW screen, FR803765
	return ERROR_NONE;    
}   /* HI708Open  */


/*************************************************************************
* FUNCTION
*	HI708Close
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
UINT32 HI708Close(void)
{

	return ERROR_NONE;
}	/* HI708Close() */


static void HI708_Set_Mirror_Flip(kal_uint8 image_mirror)
{
	/********************************************************
	* Page Mode 0: Reg 0x0011 bit[1:0] = [Y Flip : X Flip]
	* 0: Off; 1: On.
	*********************************************************/ 
	kal_uint8 temp_data;   
	SENSORDB("[Enter]:HI708 set Mirror_flip func:image_mirror=%d\n",image_mirror);	
	HI708_write_cmos_sensor(0x03,0x00);     //Page 0	
	temp_data = (HI708_read_cmos_sensor(0x11) & 0xfc);
	spin_lock(&hi708_yuv_drv_lock);
	//HI708_sensor.mirror = (HI708_read_cmos_sensor(0x11) & 0xfc); 
	switch (image_mirror) 
	{
	case IMAGE_NORMAL:
		//HI708_sensor.mirror |= 0x00;
		temp_data |= 0x00;
		break;
	case IMAGE_H_MIRROR:
		//HI708_sensor.mirror |= 0x01;
		temp_data |= 0x01;
		break;
	case IMAGE_V_MIRROR:
		//HI708_sensor.mirror |= 0x02;
		temp_data |= 0x02;
		break;
	case IMAGE_HV_MIRROR:
		//HI708_sensor.mirror |= 0x03;
		temp_data |= 0x03;
		break;
	default:
		//HI708_sensor.mirror |= 0x00;
		temp_data |= 0x00;
	}
	HI708_sensor.mirror = temp_data;
	spin_unlock(&hi708_yuv_drv_lock);
	HI708_write_cmos_sensor(0x11, HI708_sensor.mirror);
	SENSORDB("[Exit]:HI708 set Mirror_flip func\n");
}

#if 0
static void HI708_set_dummy(kal_uint16 dummy_pixels,kal_uint16 dummy_lines)
{	
	HI708_write_cmos_sensor(0x03, 0x00);                        //Page 0
	HI708_write_cmos_sensor(0x40,((dummy_pixels & 0x0F00))>>8);       //HBLANK
	HI708_write_cmos_sensor(0x41,(dummy_pixels & 0xFF));
	HI708_write_cmos_sensor(0x42,((dummy_lines & 0xFF00)>>8));       //VBLANK ( Vsync Type 1)
	HI708_write_cmos_sensor(0x43,(dummy_lines & 0xFF));
}  
#endif

static void HI708_Cal_Min_Frame_Rate(kal_uint16 min_framerate)
{
	kal_uint32 HI708_expmax = 0;
	kal_uint32 HI708_expbanding = 0;
	kal_uint32 temp_data;

	SENSORDB("[HI708] HI708_Cal_Min_Frame_Rate:min_fps=%d\n",min_framerate);

	//No Fixed Framerate
	HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)|0x01);   //Sleep: For Write Reg
	HI708_write_cmos_sensor(0x03, 0x00);
	HI708_write_cmos_sensor(0x11, HI708_read_cmos_sensor(0x11)&0xfb);

	HI708_write_cmos_sensor(0x03, 0x20);
	HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)&0x7f);   //Close AE

	HI708_write_cmos_sensor(0x11, 0x04);
	HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)|0x08);   //Reset AE
	HI708_write_cmos_sensor(0x2a, 0xf0);
	HI708_write_cmos_sensor(0x2b, 0x34);

	HI708_write_cmos_sensor(0x03, 0x00);
	temp_data = ((HI708_read_cmos_sensor(0x40)<<8)|HI708_read_cmos_sensor(0x41));
	spin_lock(&hi708_yuv_drv_lock);
	HI708_sensor.pv_dummy_pixels = temp_data;
	HI708_sensor.pv_line_length = HI708_VGA_DEFAULT_PIXEL_NUMS+ HI708_sensor.pv_dummy_pixels ;
	spin_unlock(&hi708_yuv_drv_lock);

	if(HI708_sensor.banding == AE_FLICKER_MODE_50HZ)
	{
		HI708_expbanding = (HI708_sensor.pv_pclk/HI708_sensor.pv_line_length/100)*HI708_sensor.pv_line_length/8;
		HI708_expmax = HI708_expbanding*100*10/min_framerate ;
	}
	else if(HI708_sensor.banding == AE_FLICKER_MODE_60HZ)
	{
		HI708_expbanding = (HI708_sensor.pv_pclk/HI708_sensor.pv_line_length/120)*HI708_sensor.pv_line_length/8;
		HI708_expmax = HI708_expbanding*120*10/min_framerate ;
	}
	else
	{
		SENSORDB("[HI708][Error] Wrong Banding Setting!!!...");
	}

	HI708_write_cmos_sensor(0x03, 0x20);
	HI708_write_cmos_sensor(0x88, (HI708_expmax>>16)&0xff);
	HI708_write_cmos_sensor(0x89, (HI708_expmax>>8)&0xff);
	HI708_write_cmos_sensor(0x8a, (HI708_expmax>>0)&0xff);

	HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)&0xfe);   //Exit Sleep: For Write Reg

	HI708_write_cmos_sensor(0x03, 0x20);
	HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)|0x80);   //Open AE
	HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)&0xf7);   //Reset AE
}


static void HI708_Fix_Video_Frame_Rate(kal_uint16 fix_framerate)
{
	kal_uint32 HI708_expfix;
	kal_uint32 HI708_expfix_temp;
	kal_uint32 HI708_expmax = 0;
	kal_uint32 HI708_expbanding = 0;
	kal_uint32 temp_data1,temp_data2;

	SENSORDB("[Enter]HI708 Fix_video_frame_rate func: fix_fps=%d\n",fix_framerate);

	spin_lock(&hi708_yuv_drv_lock);
	HI708_sensor.video_current_frame_rate = fix_framerate;
	spin_unlock(&hi708_yuv_drv_lock);
	// Fixed Framerate
	HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)|0x01);   //Sleep: For Write Reg

	HI708_write_cmos_sensor(0x03, 0x00);
	HI708_write_cmos_sensor(0x11, HI708_read_cmos_sensor(0x11)|0x04);

	HI708_write_cmos_sensor(0x03, 0x20);
	HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)&0x7f);   //Close AE

	HI708_write_cmos_sensor(0x11, 0x00);
	HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)|0x08);   //Reset AE
	HI708_write_cmos_sensor(0x2a, 0x00);
	HI708_write_cmos_sensor(0x2b, 0x35);

	HI708_write_cmos_sensor(0x03, 0x00);
	temp_data1 = ((HI708_read_cmos_sensor(0x40)<<8)|HI708_read_cmos_sensor(0x41));
	temp_data2 = ((HI708_read_cmos_sensor(0x42)<<8)|HI708_read_cmos_sensor(0x43));
	spin_lock(&hi708_yuv_drv_lock);
	HI708_sensor.pv_dummy_pixels = temp_data1; 
	HI708_sensor.pv_line_length = HI708_VGA_DEFAULT_PIXEL_NUMS + HI708_sensor.pv_dummy_pixels ;   
	HI708_sensor.pv_dummy_lines = temp_data2;
	spin_unlock(&hi708_yuv_drv_lock);

	HI708_expfix_temp = ((HI708_sensor.pv_pclk*10/fix_framerate)-(HI708_sensor.pv_line_length*HI708_sensor.pv_dummy_lines))/8;    
	HI708_expfix = ((HI708_expfix_temp*8/HI708_sensor.pv_line_length)*HI708_sensor.pv_line_length)/8;

	HI708_write_cmos_sensor(0x03, 0x20);    
	//HI708_write_cmos_sensor(0x83, (HI708_expfix>>16)&0xff);
	//HI708_write_cmos_sensor(0x84, (HI708_expfix>>8)&0xff);
	//HI708_write_cmos_sensor(0x85, (HI708_expfix>>0)&0xff);    
	HI708_write_cmos_sensor(0x91, (HI708_expfix>>16)&0xff);
	HI708_write_cmos_sensor(0x92, (HI708_expfix>>8)&0xff);
	HI708_write_cmos_sensor(0x93, (HI708_expfix>>0)&0xff);

	if(HI708_sensor.banding == AE_FLICKER_MODE_50HZ)
	{
		HI708_expbanding = ((HI708_read_cmos_sensor(0x8b)<<8)|HI708_read_cmos_sensor(0x8c));
	}
	else if(HI708_sensor.banding == AE_FLICKER_MODE_60HZ)
	{
		HI708_expbanding = ((HI708_read_cmos_sensor(0x8d)<<8)|HI708_read_cmos_sensor(0x8e));
	}
	else
	{
		SENSORDB("[HI708]Wrong Banding Setting!!!...");
	}
	HI708_expmax = ((HI708_expfix_temp-HI708_expbanding)/HI708_expbanding)*HI708_expbanding;    

	HI708_write_cmos_sensor(0x03, 0x20);
	HI708_write_cmos_sensor(0x88, (HI708_expmax>>16)&0xff);
	HI708_write_cmos_sensor(0x89, (HI708_expmax>>8)&0xff);
	HI708_write_cmos_sensor(0x8a, (HI708_expmax>>0)&0xff);

	HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)&0xfe);   //Exit Sleep: For Write Reg

	HI708_write_cmos_sensor(0x03, 0x20);
	HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)|0x80);   //Open AE
	HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)&0xf7);   //Reset AE
}

#if 0
// 320 * 240
static void HI708_Set_QVGA_mode(void)
{
	HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)|0x01);   //Sleep: For Write Reg

	HI708_write_cmos_sensor(0x03, 0x00);
	HI708_write_cmos_sensor(0x10, 0x01);        //QVGA Size: 0x10 -> 0x01

	HI708_write_cmos_sensor(0x20, 0x00);
	HI708_write_cmos_sensor(0x21, 0x02);

	HI708_write_cmos_sensor(0x40, 0x01);        //HBLANK:  0x0158 = 344
	HI708_write_cmos_sensor(0x41, 0x58);
	HI708_write_cmos_sensor(0x42, 0x00);        //VBLANK:  0x14 = 20
	HI708_write_cmos_sensor(0x43, 0x14);

	HI708_write_cmos_sensor(0x03, 0x11);        //QVGA Fixframerate
	HI708_write_cmos_sensor(0x10, 0x21);  

	HI708_write_cmos_sensor(0x03, 0x20);
	HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)&0x7f);   //Close AE
	HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)|0x08);   //Reset AE

	HI708_write_cmos_sensor(0x83, 0x00);
	HI708_write_cmos_sensor(0x84, 0xaf);
	HI708_write_cmos_sensor(0x85, 0xc8);
	HI708_write_cmos_sensor(0x86, 0x00);
	HI708_write_cmos_sensor(0x87, 0xfa);

	HI708_write_cmos_sensor(0x8b, 0x3a);
	HI708_write_cmos_sensor(0x8c, 0x98);
	HI708_write_cmos_sensor(0x8d, 0x30);
	HI708_write_cmos_sensor(0x8e, 0xd4);

	HI708_write_cmos_sensor(0x9c, 0x0b);
	HI708_write_cmos_sensor(0x9d, 0x3b);
	HI708_write_cmos_sensor(0x9e, 0x00);
	HI708_write_cmos_sensor(0x9f, 0xfa);

	HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)&0xfe);   //Exit Sleep: For Write Reg

	HI708_write_cmos_sensor(0x03, 0x20);
	HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)|0x80);   //Open AE
	HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)&0xf7);   //Reset AE

}
#endif
void HI708_night_mode(kal_bool enable)
{
	SENSORDB("[Enter]HI708 night mode func:enable = %d\n",enable);
	SENSORDB("HI708_sensor.video_mode = %d\n",HI708_sensor.MPEG4_Video_mode); 
	SENSORDB("HI708_sensor.night_mode = %d\n",HI708_sensor.night_mode);
	spin_lock(&hi708_yuv_drv_lock);
	HI708_sensor.night_mode = enable;
	spin_unlock(&hi708_yuv_drv_lock);

	if(HI708_sensor.MPEG4_Video_mode == KAL_TRUE)
		return;

	if(enable)
	{
		//HI708_set_param_wb(AWB_MODE_AUTO);//Lock auto AWB in night mode.
		//HI708_Cal_Min_Frame_Rate(HI708_MIN_FRAMERATE_5);                            
		if(HI708_sensor.banding == AE_FLICKER_MODE_50HZ)
		{
			HI708_write_cmos_sensor(0x03, 0x20);

			HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)&0x7f);   //Close AE
			HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)|0x08);   //Reset AE

			HI708_write_cmos_sensor(0x83, 0x02);
			HI708_write_cmos_sensor(0x84, 0x7a);
			HI708_write_cmos_sensor(0x85, 0xc4);
			HI708_write_cmos_sensor(0x86, 0x00);
			HI708_write_cmos_sensor(0x87, 0xfa);

			HI708_write_cmos_sensor(0x88, 0x02);
			HI708_write_cmos_sensor(0x89, 0x7a);
			HI708_write_cmos_sensor(0x8a, 0xc4);


			HI708_write_cmos_sensor(0x8b, 0x1f);
			HI708_write_cmos_sensor(0x8c, 0xbd);
			HI708_write_cmos_sensor(0x8d, 0x1a);
			HI708_write_cmos_sensor(0x8e, 0x5e);

			HI708_write_cmos_sensor(0x9c, 0x0b);
			HI708_write_cmos_sensor(0x9d, 0x3b);
			HI708_write_cmos_sensor(0x9e, 0x00);
			HI708_write_cmos_sensor(0x9f, 0xfa);

			HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)&0xfe);   //Exit Sleep: For Write Reg

			HI708_write_cmos_sensor(0x03, 0x20);
			HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)|0x80);   //Open AE
			HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)&0xf7);   //Reset AE
		}
		else 
		{
			HI708_write_cmos_sensor(0x03, 0x20);

			HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)&0x7f);   //Close AE
			HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)|0x08);   //Reset AE

			HI708_write_cmos_sensor(0x83, 0x02);
			HI708_write_cmos_sensor(0x84, 0x78);
			HI708_write_cmos_sensor(0x85, 0xd0);
			HI708_write_cmos_sensor(0x86, 0x00);
			HI708_write_cmos_sensor(0x87, 0xfa);

			HI708_write_cmos_sensor(0x88, 0x02);
			HI708_write_cmos_sensor(0x89, 0x78);
			HI708_write_cmos_sensor(0x8a, 0xd0);


			HI708_write_cmos_sensor(0x8b, 0x1f);
			HI708_write_cmos_sensor(0x8c, 0xbd);
			HI708_write_cmos_sensor(0x8d, 0x1a);
			HI708_write_cmos_sensor(0x8e, 0x5e);

			HI708_write_cmos_sensor(0x9c, 0x0b);
			HI708_write_cmos_sensor(0x9d, 0x3b);
			HI708_write_cmos_sensor(0x9e, 0x00);
			HI708_write_cmos_sensor(0x9f, 0xfa);

			HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)&0xfe);   //Exit Sleep: For Write Reg

			HI708_write_cmos_sensor(0x03, 0x20);
			HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)|0x80);   //Open AE
			HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)&0xf7);   //Reset AE
		}
	}
	else
	{
		//HI708_Cal_Min_Frame_Rate(HI708_MIN_FRAMERATE_10);
		if(HI708_sensor.banding == AE_FLICKER_MODE_50HZ)
		{
			HI708_write_cmos_sensor(0x03, 0x20);

			HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)&0x7f);   //Close AE
			HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)|0x08);   //Reset AE

			HI708_write_cmos_sensor(0x83, 0x01);
			HI708_write_cmos_sensor(0x84, 0x7c);
			HI708_write_cmos_sensor(0x85, 0xdc);
			HI708_write_cmos_sensor(0x86, 0x00);
			HI708_write_cmos_sensor(0x87, 0xfa);

			HI708_write_cmos_sensor(0x88, 0x01);
			HI708_write_cmos_sensor(0x89, 0x7c);
			HI708_write_cmos_sensor(0x8a, 0xdc);


			HI708_write_cmos_sensor(0x8b, 0x1f);
			HI708_write_cmos_sensor(0x8c, 0xbd);
			HI708_write_cmos_sensor(0x8d, 0x1a);
			HI708_write_cmos_sensor(0x8e, 0x5e);

			HI708_write_cmos_sensor(0x9c, 0x0b);
			HI708_write_cmos_sensor(0x9d, 0x3b);
			HI708_write_cmos_sensor(0x9e, 0x00);
			HI708_write_cmos_sensor(0x9f, 0xfa);

			HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)&0xfe);   //Exit Sleep: For Write Reg

			HI708_write_cmos_sensor(0x03, 0x20);
			HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)|0x80);   //Open AE
			HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)&0xf7);   //Reset AE
		}
		else 
		{
			HI708_write_cmos_sensor(0x03, 0x20);
			HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)&0x7f);	//Close AE
			HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)|0x08);	//Reset AE

			HI708_write_cmos_sensor(0x83, 0x01);
			HI708_write_cmos_sensor(0x84, 0x8b);
			HI708_write_cmos_sensor(0x85, 0x82);
			HI708_write_cmos_sensor(0x86, 0x00);
			HI708_write_cmos_sensor(0x87, 0xfa);

			HI708_write_cmos_sensor(0x88, 0x01);
			HI708_write_cmos_sensor(0x89, 0x8b);
			HI708_write_cmos_sensor(0x8a, 0x82);


			HI708_write_cmos_sensor(0x8b, 0x1f);
			HI708_write_cmos_sensor(0x8c, 0xbd);
			HI708_write_cmos_sensor(0x8d, 0x1a);
			HI708_write_cmos_sensor(0x8e, 0x5e);

			HI708_write_cmos_sensor(0x9c, 0x0b);
			HI708_write_cmos_sensor(0x9d, 0x3b);
			HI708_write_cmos_sensor(0x9e, 0x00);
			HI708_write_cmos_sensor(0x9f, 0xfa);

			HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)&0xfe);	//Exit Sleep: For Write Reg

			HI708_write_cmos_sensor(0x03, 0x20);
			HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)|0x80);	//Open AE
			HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)&0xf7);	//Reset AE
		}
	}
}

/*************************************************************************
* FUNCTION
*	HI708Preview
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
static UINT32 HI708Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
						   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	spin_lock(&hi708_yuv_drv_lock);
	sensor_config_data->SensorImageMirror = IMAGE_HV_MIRROR; 
	if(HI708_sensor.first_init == KAL_TRUE)
	{
		HI708_sensor.MPEG4_Video_mode = HI708_sensor.MPEG4_Video_mode;
	}
	else
	{
		HI708_sensor.MPEG4_Video_mode = !HI708_sensor.MPEG4_Video_mode;
	}
	spin_unlock(&hi708_yuv_drv_lock);

	SENSORDB("[Enter]:HI708 preview func:");		
	SENSORDB("HI708_sensor.video_mode = %d\n",HI708_sensor.MPEG4_Video_mode); 

	spin_lock(&hi708_yuv_drv_lock);
	HI708_sensor.first_init = KAL_FALSE;	
	HI708_sensor.pv_mode = KAL_TRUE;		
	spin_unlock(&hi708_yuv_drv_lock);

	{   
		SENSORDB("[HI708]preview set_VGA_mode\n");
		HI708_Set_VGA_mode();
	}

	HI708_Set_Mirror_Flip(sensor_config_data->SensorImageMirror);

	SENSORDB("[Exit]:HI708 preview func\n");
	return TRUE; 
}	/* HI708_Preview */


UINT32 HI708Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	SENSORDB("[HI708][Enter]HI708_capture_func\n");
	spin_lock(&hi708_yuv_drv_lock);
	HI708_sensor.pv_mode = KAL_FALSE;	
	spin_unlock(&hi708_yuv_drv_lock);
	CAPTURE_FLAG = 1;
	CAPTUREB_FLAG = 1;

	return ERROR_NONE;
}	/* HM3451Capture() */


UINT32 HI708GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	SENSORDB("[Enter]:HI708 get Resolution func\n");

	pSensorResolution->SensorFullWidth=HI708_IMAGE_SENSOR_FULL_WIDTH - 10;  
	pSensorResolution->SensorFullHeight=HI708_IMAGE_SENSOR_FULL_HEIGHT - 10-10;
	pSensorResolution->SensorPreviewWidth=HI708_IMAGE_SENSOR_PV_WIDTH - 16;
	pSensorResolution->SensorPreviewHeight=HI708_IMAGE_SENSOR_PV_HEIGHT - 12-10;
	pSensorResolution->SensorVideoWidth=HI708_IMAGE_SENSOR_PV_WIDTH - 16;
	pSensorResolution->SensorVideoHeight=HI708_IMAGE_SENSOR_PV_HEIGHT - 12-10;
	pSensorResolution->Sensor3DFullWidth=HI708_IMAGE_SENSOR_FULL_WIDTH - 10;  
	pSensorResolution->Sensor3DFullHeight=HI708_IMAGE_SENSOR_FULL_HEIGHT - 10-10;
	pSensorResolution->Sensor3DPreviewWidth=HI708_IMAGE_SENSOR_PV_WIDTH - 16;
	pSensorResolution->Sensor3DPreviewHeight=HI708_IMAGE_SENSOR_PV_HEIGHT - 12-10;
	pSensorResolution->Sensor3DVideoWidth=HI708_IMAGE_SENSOR_PV_WIDTH - 16;
	pSensorResolution->Sensor3DVideoHeight=HI708_IMAGE_SENSOR_PV_HEIGHT - 12-10;

	SENSORDB("[Exit]:HI708 get Resolution func\n");	
	return ERROR_NONE;
}	/* HI708GetResolution() */

UINT32 HI708GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	SENSORDB("[Enter]:HI708 getInfo func:ScenarioId = %d\n",ScenarioId);

	pSensorInfo->SensorPreviewResolutionX=HI708_IMAGE_SENSOR_PV_WIDTH;
	pSensorInfo->SensorPreviewResolutionY=HI708_IMAGE_SENSOR_PV_HEIGHT;
	pSensorInfo->SensorFullResolutionX=HI708_IMAGE_SENSOR_FULL_WIDTH;
	pSensorInfo->SensorFullResolutionY=HI708_IMAGE_SENSOR_FULL_HEIGHT;

	pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=30;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE;//low is to reset 
	pSensorInfo->SensorResetDelayCount=4;  //4ms 
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV; //SENSOR_OUTPUT_FORMAT_YVYU;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorInterruptDelayLines = 1; 
	pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;


	pSensorInfo->CaptureDelayFrame = 4; 
	pSensorInfo->PreviewDelayFrame = 4;//10; 
	pSensorInfo->VideoDelayFrame = 0; 
	pSensorInfo->SensorMasterClockSwitch = 0; 
	pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;   	

	switch (ScenarioId)
	{
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
	case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW:
	case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:		
		pSensorInfo->SensorClockFreq=26;
		pSensorInfo->SensorClockDividCount=	3;
		pSensorInfo->SensorClockRisingCount= 0;
		pSensorInfo->SensorClockFallingCount= 2;
		pSensorInfo->SensorPixelClockCount= 3;
		pSensorInfo->SensorDataLatchCount= 2;
		pSensorInfo->SensorGrabStartX = 1; 
		pSensorInfo->SensorGrabStartY = 10;  	
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE:
		pSensorInfo->SensorClockFreq=26;
		pSensorInfo->SensorClockDividCount=	3;
		pSensorInfo->SensorClockRisingCount= 0;
		pSensorInfo->SensorClockFallingCount= 2;
		pSensorInfo->SensorPixelClockCount= 3;
		pSensorInfo->SensorDataLatchCount= 2;
		pSensorInfo->SensorGrabStartX = 1; 
		pSensorInfo->SensorGrabStartY = 10;//1;     			
		break;
	default:
		pSensorInfo->SensorClockFreq=26;
		pSensorInfo->SensorClockDividCount=3;
		pSensorInfo->SensorClockRisingCount=0;
		pSensorInfo->SensorClockFallingCount=2;
		pSensorInfo->SensorPixelClockCount=3;
		pSensorInfo->SensorDataLatchCount=2;
		pSensorInfo->SensorGrabStartX = 1; 
		pSensorInfo->SensorGrabStartY = 10;//1;     			
		break;
	}
	//	HI708_PixelClockDivider=pSensorInfo->SensorPixelClockCount;
	memcpy(pSensorConfigData, &HI708SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	SENSORDB("[Exit]:HI708 getInfo func\n");	
	return ERROR_NONE;
}	/* HI708GetInfo() */


UINT32 HI708Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	SENSORDB("[Enter]:HI708 Control func:ScenarioId = %d\n",ScenarioId);

	switch (ScenarioId)
	{
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		//case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW:
		//case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:		
		HI708Preview(pImageWindow, pSensorConfigData); 
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		//case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE:
		HI708Capture(pImageWindow, pSensorConfigData); 
		break;
	default:
		break; 
	}

	SENSORDB("[Exit]:HI708 Control func\n");	
	return TRUE;
}	/* HI708Control() */


/*************************************************************************
* FUNCTION
*	HI708_set_param_wb
*
* DESCRIPTION
*	wb setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL HI708_set_param_wb(UINT16 para)
{
	//This sensor need more time to balance AWB, 
	//we suggest higher fps or drop some frame to avoid garbage color when preview initial
	SENSORDB("[Enter]HI708 set_param_wb func:para = %d\n",para);

	if(HI708_sensor.wb == para) return KAL_TRUE;	

	spin_lock(&hi708_yuv_drv_lock);
	HI708_sensor.wb = para;
	spin_unlock(&hi708_yuv_drv_lock);

	switch (para)
	{            
	case AWB_MODE_AUTO:
		{
			HI708_write_cmos_sensor(0x03, 0x22);			
			HI708_write_cmos_sensor(0x11, 0x2e);				
			//HI708_write_cmos_sensor(0x80, 0x38);
			//HI708_write_cmos_sensor(0x82, 0x38);				
        HI708_write_cmos_sensor(0x83, 0x55);//56
        HI708_write_cmos_sensor(0x84, 0x16);
        HI708_write_cmos_sensor(0x85, 0x59);
        HI708_write_cmos_sensor(0x86, 0x23);				
			HI708_write_cmos_sensor(0x10, 0xfb);				
		}                
		break;
	case AWB_MODE_CLOUDY_DAYLIGHT:
		{
			HI708_write_cmos_sensor(0x03, 0x22);
			HI708_write_cmos_sensor(0x11, 0x28);
			HI708_write_cmos_sensor(0x80, 0x71);
			HI708_write_cmos_sensor(0x82, 0x2b);
			HI708_write_cmos_sensor(0x83, 0x72);
			HI708_write_cmos_sensor(0x84, 0x70);
			HI708_write_cmos_sensor(0x85, 0x2b);
			HI708_write_cmos_sensor(0x86, 0x28);
			HI708_write_cmos_sensor(0x10, 0xfb);
		}			   
		break;
	case AWB_MODE_DAYLIGHT:
		{
			HI708_write_cmos_sensor(0x03, 0x22);
			HI708_write_cmos_sensor(0x11, 0x28);          
			HI708_write_cmos_sensor(0x80, 0x59);
			HI708_write_cmos_sensor(0x82, 0x29);
			HI708_write_cmos_sensor(0x83, 0x60);
			HI708_write_cmos_sensor(0x84, 0x50);
			HI708_write_cmos_sensor(0x85, 0x2f);
			HI708_write_cmos_sensor(0x86, 0x23);
			HI708_write_cmos_sensor(0x10, 0xfb);
		}      
		break;
	case AWB_MODE_INCANDESCENT:	
		{
			HI708_write_cmos_sensor(0x03, 0x22);
			HI708_write_cmos_sensor(0x11, 0x28);          
			HI708_write_cmos_sensor(0x80, 0x29);
			HI708_write_cmos_sensor(0x82, 0x54);
			HI708_write_cmos_sensor(0x83, 0x2e);
			HI708_write_cmos_sensor(0x84, 0x23);
			HI708_write_cmos_sensor(0x85, 0x58);
			HI708_write_cmos_sensor(0x86, 0x4f);
			HI708_write_cmos_sensor(0x10, 0xfb);
		}		
		break;  
	case AWB_MODE_FLUORESCENT:
		{
			HI708_write_cmos_sensor(0x03, 0x22);
			HI708_write_cmos_sensor(0x11, 0x28);
			HI708_write_cmos_sensor(0x80, 0x41);
			HI708_write_cmos_sensor(0x82, 0x42);
			HI708_write_cmos_sensor(0x83, 0x44);
			HI708_write_cmos_sensor(0x84, 0x34);
			HI708_write_cmos_sensor(0x85, 0x46);
			HI708_write_cmos_sensor(0x86, 0x3a);
			HI708_write_cmos_sensor(0x10, 0xfb);
		}	
		break;  
	case AWB_MODE_TUNGSTEN:
		{
			HI708_write_cmos_sensor(0x03, 0x22);
			HI708_write_cmos_sensor(0x80, 0x24);
			HI708_write_cmos_sensor(0x81, 0x20);
			HI708_write_cmos_sensor(0x82, 0x58);
			HI708_write_cmos_sensor(0x83, 0x27);
			HI708_write_cmos_sensor(0x84, 0x22);
			HI708_write_cmos_sensor(0x85, 0x58);
			HI708_write_cmos_sensor(0x86, 0x52);
			HI708_write_cmos_sensor(0x10, 0xfb);
		}
		break;
	case AWB_MODE_OFF:
		{
			SENSORDB("HI708 AWB OFF");
			HI708_write_cmos_sensor(0x03, 0x22);
			HI708_write_cmos_sensor(0x10, 0xe2);
		}
		break;
	default:
		return FALSE;
	}

	return TRUE;	
} /* HI708_set_param_wb */

/*************************************************************************
* FUNCTION
*	HI708_set_param_effect
*
* DESCRIPTION
*	effect setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL HI708_set_param_effect(UINT16 para)
{
	SENSORDB("[Enter]HI708 set_param_effect func:para = %d\n",para);

	if(HI708_sensor.effect == para) return KAL_TRUE;

	spin_lock(&hi708_yuv_drv_lock);
	HI708_sensor.effect = para;
	spin_unlock(&hi708_yuv_drv_lock);

	switch (para)
	{
	case MEFFECT_OFF:
		{
			HI708_write_cmos_sensor(0x03, 0x10);
			HI708_write_cmos_sensor(0x11, 0x03);
			HI708_write_cmos_sensor(0x12, 0xf0);
			HI708_write_cmos_sensor(0x13, 0x00);
			HI708_write_cmos_sensor(0x44, 0x80);
			HI708_write_cmos_sensor(0x45, 0x80);

			HI708_write_cmos_sensor(0x47, 0x7f);
			HI708_write_cmos_sensor(0x03, 0x13);
			HI708_write_cmos_sensor(0x20, 0x07);
			HI708_write_cmos_sensor(0x21, 0x07);
		}
		break;
	case MEFFECT_SEPIA:
		{
			HI708_write_cmos_sensor(0x03, 0x10);
			HI708_write_cmos_sensor(0x11, 0x03);
			HI708_write_cmos_sensor(0x12, 0x23);
			HI708_write_cmos_sensor(0x13, 0x00);
			HI708_write_cmos_sensor(0x44, 0x70);
			HI708_write_cmos_sensor(0x45, 0x98);

			HI708_write_cmos_sensor(0x47, 0x7f);
			HI708_write_cmos_sensor(0x03, 0x13);
			HI708_write_cmos_sensor(0x20, 0x07);
			HI708_write_cmos_sensor(0x21, 0x07);
		}	
		break;  
	case MEFFECT_NEGATIVE:
		{
			HI708_write_cmos_sensor(0x03, 0x10);
			HI708_write_cmos_sensor(0x11, 0x03);
			HI708_write_cmos_sensor(0x12, 0x08);
			HI708_write_cmos_sensor(0x13, 0x00);
			HI708_write_cmos_sensor(0x14, 0x00);
		}
		break; 
	case MEFFECT_SEPIAGREEN:		
		{
			HI708_write_cmos_sensor(0x03, 0x10);
			HI708_write_cmos_sensor(0x11, 0x03);
			HI708_write_cmos_sensor(0x12, 0x03);
			//HI708_write_cmos_sensor(0x40, 0x00);
			HI708_write_cmos_sensor(0x13, 0x00);
			HI708_write_cmos_sensor(0x44, 0x30);
			HI708_write_cmos_sensor(0x45, 0x50);
		}	
		break;
	case MEFFECT_SEPIABLUE:
		{
			HI708_write_cmos_sensor(0x03, 0x10);
			HI708_write_cmos_sensor(0x11, 0x03);
			HI708_write_cmos_sensor(0x12, 0x03);
			//HI708_write_cmos_sensor(0x40, 0x00);
			HI708_write_cmos_sensor(0x13, 0x00);
			HI708_write_cmos_sensor(0x44, 0xb0);
			HI708_write_cmos_sensor(0x45, 0x40);
		}     
		break;        
	case MEFFECT_MONO:			
		{
			HI708_write_cmos_sensor(0x03, 0x10);
			HI708_write_cmos_sensor(0x11, 0x03);
			HI708_write_cmos_sensor(0x12, 0x03);
			//HI708_write_cmos_sensor(0x40, 0x00);
			HI708_write_cmos_sensor(0x44, 0x80);
			HI708_write_cmos_sensor(0x45, 0x80);
		}
		break;
	default:
		return KAL_FALSE;
	}

	return KAL_TRUE;
} /* HI708_set_param_effect */

/*************************************************************************
* FUNCTION
*	HI708_set_param_banding
*
* DESCRIPTION
*	banding setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL HI708_set_param_banding(UINT16 para)
{
	SENSORDB("[Enter]HI708 set_param_banding func:para = %d\n",para);

	if(HI708_sensor.banding == para) return KAL_TRUE;

	spin_lock(&hi708_yuv_drv_lock);
	HI708_sensor.banding = para;
	spin_unlock(&hi708_yuv_drv_lock);

	switch (para)
	{
	case AE_FLICKER_MODE_50HZ:
		{
			HI708_write_cmos_sensor(0x03,0x20);
			HI708_write_cmos_sensor(0x10,0x1c);
			HI708_write_cmos_sensor(0x18,0x38);
			HI708_write_cmos_sensor(0x83,0x02);
			HI708_write_cmos_sensor(0x84,0x7a);
			HI708_write_cmos_sensor(0x85,0xc4);
			HI708_write_cmos_sensor(0x88,0x02);
			HI708_write_cmos_sensor(0x89,0x7a);
			HI708_write_cmos_sensor(0x8a,0xc4);
			HI708_write_cmos_sensor(0x10,0x9c);
			HI708_write_cmos_sensor(0x18,0x30);
		}
		break;
	case AE_FLICKER_MODE_60HZ:
		{
			HI708_write_cmos_sensor(0x03,0x20);
			HI708_write_cmos_sensor(0x10,0x0c);
			HI708_write_cmos_sensor(0x18,0x38);
			HI708_write_cmos_sensor(0x83,0x02);
			HI708_write_cmos_sensor(0x84,0x78);
			HI708_write_cmos_sensor(0x85,0xd0);
			HI708_write_cmos_sensor(0x88,0x02);
			HI708_write_cmos_sensor(0x89,0x78);
			HI708_write_cmos_sensor(0x8a,0xd0);
			HI708_write_cmos_sensor(0x10,0x8c);
			HI708_write_cmos_sensor(0x18,0x30);
		}
		break;
	default:
		return KAL_FALSE;
	}

	return KAL_TRUE;
} /* HI708_set_param_banding */




/*************************************************************************
* FUNCTION
*	HI708_set_param_exposure
*
* DESCRIPTION
*	exposure setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL HI708_set_param_exposure(UINT16 para)
{
	SENSORDB("[Enter]HI708 set_param_exposure func:para = %d\n",para);

	if(HI708_sensor.exposure == para) return KAL_TRUE;

	spin_lock(&hi708_yuv_drv_lock);
	HI708_sensor.exposure = para;
	spin_unlock(&hi708_yuv_drv_lock);

	HI708_write_cmos_sensor(0x03,0x10);
	HI708_write_cmos_sensor(0x12,HI708_read_cmos_sensor(0x12)|0x10);
	switch (para)
	{
	case AE_EV_COMP_13:  //+4 EV
		HI708_write_cmos_sensor(0x40,0x60);
		break;  
	case AE_EV_COMP_10:  //+3 EV
		HI708_write_cmos_sensor(0x40,0x48);
		break;    
	case AE_EV_COMP_07:  //+2 EV
		HI708_write_cmos_sensor(0x40,0x30);
		break;    
	case AE_EV_COMP_03:	 //	+1 EV	
		HI708_write_cmos_sensor(0x40,0x18);	
		break;    
	case AE_EV_COMP_00:  // +0 EV
		HI708_write_cmos_sensor(0x40,0x00);
		break;    
	case AE_EV_COMP_n03:  // -1 EV
		HI708_write_cmos_sensor(0x40,0x98);
		break;    
	case AE_EV_COMP_n07:	// -2 EV		
		HI708_write_cmos_sensor(0x40,0xb0);	
		break;    
	case AE_EV_COMP_n10:   //-3 EV
		HI708_write_cmos_sensor(0x40,0xc8);
		break;
	case AE_EV_COMP_n13:  // -4 EV
		HI708_write_cmos_sensor(0x40,0xe0);
		break;
	default:
		return FALSE;
	}

	return TRUE;	
} /* HI708_set_param_exposure */

void HI708_set_AE_mode(UINT32 iPara)
{
	UINT8 temp_AE_reg = 0;
	SENSORDB("HI708_set_AE_mode = %d E \n",iPara);
	HI708_write_cmos_sensor(0x03,0x20);
	temp_AE_reg = HI708_read_cmos_sensor(0x10);

	if (AE_MODE_OFF == iPara)
	{
		// turn off AEC/AGC
		HI708_write_cmos_sensor(0x10,temp_AE_reg &~ 0x10);
	}	
	else
	{
		HI708_write_cmos_sensor(0x10,temp_AE_reg | 0x10);
	}
}
UINT32 HI708YUVSensorSetting(FEATURE_ID iCmd, UINT32 iPara)
{
	SENSORDB("[Enter]HI708YUVSensorSetting func:cmd = %d\n",iCmd);

	switch (iCmd) 
	{
	case FID_SCENE_MODE:	    //auto mode or night mode
		if (iPara == SCENE_MODE_OFF)//auto mode
		{
			if( CAPTURE_FLAG == 0)
				HI708_night_mode(FALSE); 
			else
				CAPTURE_FLAG = 0;
		}
		else if (iPara == SCENE_MODE_NIGHTSCENE)//night mode
		{
			if( CAPTURE_FLAG == 0)
				HI708_night_mode(TRUE); 
			else
				CAPTURE_FLAG = 0;
		}	
		break; 	    
	case FID_AWB_MODE:
		HI708_set_param_wb(iPara);
		break;
	case FID_COLOR_EFFECT:
		HI708_set_param_effect(iPara);
		break;
	case FID_AE_EV:	    	    
		HI708_set_param_exposure(iPara);
		break;
	case FID_AE_FLICKER:	  
		if( CAPTUREB_FLAG == 0)
			HI708_set_param_banding(iPara);
		else
			CAPTUREB_FLAG = 0;

		break;
	case FID_ZOOM_FACTOR:
		spin_lock(&hi708_yuv_drv_lock);
		HI708_zoom_factor = iPara; 
		spin_unlock(&hi708_yuv_drv_lock);
		break; 
	case FID_AE_SCENE_MODE: 
		HI708_set_AE_mode(iPara);
		break; 
	default:
		break;
	}
	return TRUE;
}   /* HI708YUVSensorSetting */

UINT32 HI708YUVSetVideoMode(UINT16 u2FrameRate)
{
	spin_lock(&hi708_yuv_drv_lock);
	HI708_sensor.MPEG4_Video_mode = KAL_TRUE;
	spin_unlock(&hi708_yuv_drv_lock);
	SENSORDB("[Enter]HI708 Set Video Mode:FrameRate= %d\n",u2FrameRate);
	SENSORDB("HI708_sensor.video_mode = %d\n",HI708_sensor.MPEG4_Video_mode);
	/*
	if(u2FrameRate == 30) u2FrameRate = 20;

	spin_lock(&hi708_yuv_drv_lock);
	HI708_sensor.fix_framerate = u2FrameRate * 10;
	spin_unlock(&hi708_yuv_drv_lock);

	if(HI708_sensor.fix_framerate <= 300 )
	{
	HI708_Fix_Video_Frame_Rate(HI708_sensor.fix_framerate); 
	}
	else 
	{
	SENSORDB("Wrong Frame Rate"); 
	}*/
	spin_lock(&hi708_yuv_drv_lock);
	spin_unlock(&hi708_yuv_drv_lock);
	return TRUE;
}

void HI708GetExifInfo(UINT32 exifAddr)
{
	SENSOR_EXIF_INFO_STRUCT* pExifInfo = (SENSOR_EXIF_INFO_STRUCT*)exifAddr;
	pExifInfo->FNumber = 28;
	pExifInfo->AEISOSpeed = AE_ISO_100;
	pExifInfo->AWBMode = HI708_sensor.wb;
	pExifInfo->CapExposureTime = 0;
	pExifInfo->FlashLightTimeus = 0;
	pExifInfo->RealISOValue = AE_ISO_100;
}

static void HI708_FOCUS_OVT_AFC_Get_AF_Status(UINT32 *pFeatureReturnPara32)
{
	static  UINT32 constant_focus_time_tap = 0;
	static  UINT32 constant_focus_count    = 0;
	static  UINT32 single_focus_count = 0;

	UINT32 state_3028=0;
	UINT32 state_3029=0;
	*pFeatureReturnPara32 = SENSOR_AF_IDLE;
	//fake constant focus always successfully 
	if( (first_constant_focus == 1) || (constant_focus_count == 50) )
	{
		if( constant_focus_time_tap == 10 )
		{
			state_3028 = 0x01  ;
			state_3029 = 0x10 ;  // SENSOR_AF_FOCUSED
			constant_focus_time_tap = 0;
			constant_focus_count    = 0;
			first_constant_focus    = 0;
		}else{
			state_3028 = 0x01  ;
			state_3029 = 0x00 ;  // SENSOR_AF_FOCUSING
			constant_focus_time_tap++;
		}
	}else{
		constant_focus_count++;
	}

	//fake single focus always successfully
	if ( single_focus_mode ){
		if( single_focus_count == 3 )
		{
			state_3028 = 0x01  ;
			state_3029 = 0x10 ;  // SENSOR_AF_FOCUSED
			single_focus_count = 0;
			single_focus_mode = 0;
		}else{
			state_3028 = 0x01  ;
			state_3029 = 0x00 ;  // SENSOR_AF_FOCUSING
			single_focus_count++;
		}
	}

	mDELAY(1);
	SENSORDB("[HI708MIPI]enter HI708_FOCUS_OVT_AFC_Get_AF_Status function:state_3028=%d,state_3029=%d\n",state_3028,state_3029);
	if (state_3028==0)
	{
		*pFeatureReturnPara32 = SENSOR_AF_ERROR;    
	}
	else if (state_3028==1)
	{
		switch (state_3029)
		{
		case 0x70:
			*pFeatureReturnPara32 = SENSOR_AF_IDLE;
			break;
		case 0x00:
			*pFeatureReturnPara32 = SENSOR_AF_FOCUSING;
			break;
		case 0x10:
			*pFeatureReturnPara32 = SENSOR_AF_FOCUSED;
			break;
		case 0x20:
			*pFeatureReturnPara32 = SENSOR_AF_FOCUSED;
			break;
		default:
			*pFeatureReturnPara32 = SENSOR_AF_SCENE_DETECTING; 
			break;
		}                                  
	}    
	SENSORDB("[HI708MIPI]exit HI708_FOCUS_OVT_AFC_Get_AF_Status function:state_3028=%d,state_3029=%d\n",state_3028,state_3029);
}

static void HI708_FOCUS_Get_AF_Max_Num_Focus_Areas(UINT32 *pFeatureReturnPara32)
{
	/*Begin ersen.shang 20150612 modify pFeatureReturnPara32 from 1 to 0 for pr1023494 cts focus areas test error*/
	*pFeatureReturnPara32 = 0;    
	/*End   ersen.shang 20150612 modify pFeatureReturnPara32 from 1 to 0 for pr1023494 cts focus areas test error*/
	
	SENSORDB(" *pFeatureReturnPara32 = %d\n",  *pFeatureReturnPara32);	
}

static void HI708_FOCUS_Get_AE_Max_Num_Metering_Areas(UINT32 *pFeatureReturnPara32)
{ 	
	SENSORDB("[HI708MIPI]enter HI708_FOCUS_Get_AE_Max_Num_Metering_Areas function:\n ");
	*pFeatureReturnPara32 = 1;    
	SENSORDB(" *pFeatureReturnPara32 = %d\n",  *pFeatureReturnPara32);
	SENSORDB("[HI708MIPI]exit HI708_FOCUS_Get_AE_Max_Num_Metering_Areas function:\n ");
}

static void HI708_FOCUS_Get_AF_Macro(UINT32 *pFeatureReturnPara32)
{
	*pFeatureReturnPara32 = 0;
}
static void HI708_FOCUS_Get_AF_Inf(UINT32 * pFeatureReturnPara32)
{
	*pFeatureReturnPara32 = 0;
}

UINT32 HI708FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
						   UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
	//UINT16 u2Temp = 0; 
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

	switch (FeatureId)
	{
	case SENSOR_FEATURE_GET_RESOLUTION:
		*pFeatureReturnPara16++=HI708_IMAGE_SENSOR_FULL_WIDTH;
		*pFeatureReturnPara16=HI708_IMAGE_SENSOR_FULL_HEIGHT;
		*pFeatureParaLen=4;
		break;
	case SENSOR_FEATURE_GET_PERIOD:
		*pFeatureReturnPara16++=HI708_IMAGE_SENSOR_PV_WIDTH;//+HI708_sensor.pv_dummy_pixels;
		*pFeatureReturnPara16=HI708_IMAGE_SENSOR_PV_HEIGHT;//+HI708_sensor.pv_dummy_lines;
		*pFeatureParaLen=4;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
		//*pFeatureReturnPara32 = HI708_sensor_pclk/10;
		*pFeatureParaLen=4;
		break;
	case SENSOR_FEATURE_SET_ESHUTTER:

		break;
	case SENSOR_FEATURE_SET_NIGHTMODE:
		HI708_night_mode((BOOL) *pFeatureData16);
		break;
	case SENSOR_FEATURE_SET_GAIN:
		break; 
	case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		HI708_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		pSensorRegData->RegData = HI708_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
	case SENSOR_FEATURE_GET_CONFIG_PARA:
		memcpy(pSensorConfigData, &HI708SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
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
		// *pFeatureReturnPara32++=0;
		//*pFeatureParaLen=4;
		break; 

	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
		// if EEPROM does not exist in camera module.
		*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
		*pFeatureParaLen=4;
		break;
	case SENSOR_FEATURE_SET_YUV_CMD:
		HI708YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
		break;	
	case SENSOR_FEATURE_SET_VIDEO_MODE:
		HI708YUVSetVideoMode(*pFeatureData16);
		break; 
	case SENSOR_FEATURE_CHECK_SENSOR_ID:
		HI708_GetSensorID(pFeatureData32); 
		break; 
	case SENSOR_FEATURE_GET_EXIF_INFO:
		SENSORDB("SENSOR_FEATURE_GET_EXIF_INFO\n");
		SENSORDB("EXIF addr = 0x%x\n",*pFeatureData32);          
		HI708GetExifInfo(*pFeatureData32);
		break;  

		/**********************below is FAKE AF control**********************/	     
	case SENSOR_FEATURE_GET_AF_STATUS:
		SENSORDB("[HI708MIPI]SENSOR_FEATURE_GET_AF_STATUS\n");
		HI708_FOCUS_OVT_AFC_Get_AF_Status(pFeatureReturnPara32);            
		*pFeatureParaLen=4;
		break;
	case SENSOR_FEATURE_SINGLE_FOCUS_MODE:
		SENSORDB("[HI708MIPI]SENSOR_FEATURE_SINGLE_FOCUS_MODE\n");
		single_focus_mode = 1;
		break;
	case SENSOR_FEATURE_CONSTANT_AF:
		SENSORDB("[HI708MIPI]SENSOR_FEATURE_CONSTANT_AF\n");
		break;
	case SENSOR_FEATURE_CANCEL_AF:
		SENSORDB("[HI708MIPI]SENSOR_FEATURE_CANCEL_AF\n");         
		break;
	case SENSOR_FEATURE_GET_AF_INF:
		SENSORDB("[HI708MIPI]SENSOR_FEATURE_GET_AF_INF\n");
		HI708_FOCUS_Get_AF_Inf(pFeatureReturnPara32);
		*pFeatureParaLen=4;            
		break;
	case SENSOR_FEATURE_GET_AF_MACRO:
		SENSORDB("[HI708MIPI]SENSOR_FEATURE_GET_AF_MACRO\n");
		HI708_FOCUS_Get_AF_Macro(pFeatureReturnPara32);
		*pFeatureParaLen=4;            
		break;
	case SENSOR_FEATURE_SET_AF_WINDOW: 
		SENSORDB("[HI708MIPI]SENSOR_FEATURE_SET_AF_WINDOW\n");
		//HI708_FOCUS_Set_AF_Window(*pFeatureData32);
		break;       					
	case SENSOR_FEATURE_GET_AF_MAX_NUM_FOCUS_AREAS:
		SENSORDB("[HI708MIPI]SENSOR_FEATURE_GET_AF_MAX_NUM_FOCUS_AREAS\n");
		HI708_FOCUS_Get_AF_Max_Num_Focus_Areas(pFeatureReturnPara32);            
		*pFeatureParaLen=4;
		break; 								                              	               
	case SENSOR_FEATURE_GET_AE_MAX_NUM_METERING_AREAS:
		SENSORDB("[HI708MIPI]AE zone addr = 0x%x\n",*pFeatureData32);
		HI708_FOCUS_Get_AE_Max_Num_Metering_Areas(pFeatureReturnPara32);            
		*pFeatureParaLen=4;
		break;        
	case SENSOR_FEATURE_SET_AE_WINDOW:
		SENSORDB("[HI708MIPI]AE zone addr = 0x%x\n",*pFeatureData32);			
		//HI708_FOCUS_Set_AE_Window(*pFeatureData32);
		break;           
	default:
		break;			
	}
	return ERROR_NONE;
}	/* HI708FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncHI708=
{
	HI708Open,
	HI708GetInfo,
	HI708GetResolution,
	HI708FeatureControl,
	HI708Control,
	HI708Close
};

UINT32 HI708_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncHI708;

	return ERROR_NONE;
}	/* SensorInit() */


