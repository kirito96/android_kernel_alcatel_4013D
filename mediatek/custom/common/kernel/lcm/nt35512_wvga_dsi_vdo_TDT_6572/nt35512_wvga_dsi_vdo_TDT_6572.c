#if defined(BUILD_LK)
#include <string.h>
#else
#include <linux/string.h>
#endif


#if defined(BUILD_LK)
#include "cust_gpio_usage.h"
#include <platform/mt_gpio.h>
#else
#include "cust_gpio_usage.h"
#include <mach/mt_gpio.h>
#endif

#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(480)
#define FRAME_HEIGHT 										(800)
#define REGFLAG_DELAY             							(0xFE)
#define REGFLAG_END_OF_TABLE      							(0x7F)   // END OF REGISTERS MARKER
#define LCM_DSI_CMD_MODE									0

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

//#define LCM_ESD_DEBUG

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))
#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)										lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

struct LCM_setting_table {
	unsigned char cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {

#if 1 /////Բ\C3\CE5.0\B4\E7IPS\C7\FD\B6\AF
	{0xFF,4, {0xAA,0x55,0xA5,0x80}},
	{0xF4,15,{0x80,0x00,0x55,0x00,0x04,0x31,0x20,0x10,0x45,0x32,
			  0x31,0x73,0xFF,0x75,0x0A}},
	{0xFF,4, {0xAA,0x55,0xA5,0x00}},

	{0xF0,5, {0x55,0xAA,0x52,0x08,0x01}},


	{0xBC,3, {0x00,0x58,0x00}},
	{0xBD,3, {0x00,0x58,0x00}},


	{0xBE,2, {0x00,0x56}},  
	{0xBF,1, {0x00}},
	{0xC3,1, {0x0B}},

	//# R+        
	{0xD1,52,{0x00,0x00,0x00,0x01,0x00,0x07,0x00,0x14,0x00,0x28,
			  0x00,0x57,0x00,0x8d,0x00,0xd4,0x01,0x11,0x01,0x5a,
			  0x01,0x88,0x01,0xc7,0x01,0xf8,0x01,0xf9,0x02,0x23,
			  0x02,0x4e,0x02,0x67,0x02,0x84,0x02,0x94,0x02,0xAd,
			  0x02,0xBa,0x02,0xd1,0x02,0xE1,0x02,0xFa,0x03,0x3F,
			  0x03,0xFF}},
	//#G +            
	{0xD2,52,{0x00,0x00,0x00,0x01,0x00,0x07,0x00,0x14,0x00,0x28,
			  0x00,0x57,0x00,0x8d,0x00,0xd4,0x01,0x11,0x01,0x5a,
			  0x01,0x88,0x01,0xc7,0x01,0xf8,0x01,0xf9,0x02,0x23,
			  0x02,0x4e,0x02,0x67,0x02,0x84,0x02,0x94,0x02,0xAd,
			  0x02,0xBa,0x02,0xd1,0x02,0xE1,0x02,0xFa,0x03,0x3F,
			  0x03,0xFF}},
	//#B +            
	{0xD3,52,{0x00,0x00,0x00,0x01,0x00,0x07,0x00,0x14,0x00,0x28,
			  0x00,0x57,0x00,0x8d,0x00,0xd4,0x01,0x11,0x01,0x5a,
			  0x01,0x88,0x01,0xc7,0x01,0xf8,0x01,0xf9,0x02,0x23,
			  0x02,0x4e,0x02,0x67,0x02,0x84,0x02,0x94,0x02,0xAd,
			  0x02,0xBa,0x02,0xd1,0x02,0xE1,0x02,0xFa,0x03,0x3F,
			  0x03,0xFF}},
	//# R-            
	{0xD4,52,{0x00,0x00,0x00,0x01,0x00,0x07,0x00,0x14,0x00,0x28,
			  0x00,0x57,0x00,0x8d,0x00,0xd4,0x01,0x11,0x01,0x5a,
			  0x01,0x88,0x01,0xc7,0x01,0xf8,0x01,0xf9,0x02,0x23,
			  0x02,0x4e,0x02,0x67,0x02,0x84,0x02,0x94,0x02,0xAd,
			  0x02,0xBa,0x02,0xd1,0x02,0xE1,0x02,0xFa,0x03,0x3F,
			  0x03,0xFF}},
	//#G -            
	{0xD5,52,{0x00,0x00,0x00,0x01,0x00,0x07,0x00,0x14,0x00,0x28,
			  0x00,0x57,0x00,0x8d,0x00,0xd4,0x01,0x11,0x01,0x5a,
			  0x01,0x88,0x01,0xc7,0x01,0xf8,0x01,0xf9,0x02,0x23,
			  0x02,0x4e,0x02,0x67,0x02,0x84,0x02,0x94,0x02,0xAd,
			  0x02,0xBa,0x02,0xd1,0x02,0xE1,0x02,0xFa,0x03,0x3F,
			  0x03,0xFF}},
	//#B -           
	{0xD6,52,{0x00,0x00,0x00,0x01,0x00,0x07,0x00,0x14,0x00,0x28,
			  0x00,0x57,0x00,0x8d,0x00,0xd4,0x01,0x11,0x01,0x5a,
			  0x01,0x88,0x01,0xc7,0x01,0xf8,0x01,0xf9,0x02,0x23,
			  0x02,0x4e,0x02,0x67,0x02,0x84,0x02,0x94,0x02,0xAd,
			  0x02,0xBa,0x02,0xd1,0x02,0xE1,0x02,0xFa,0x03,0x3F,
			  0x03,0xFF}},

	//Set AVDD Voltage//
	{0xB0,3, {0x0D,0x0D,0x0D}},

	//Set AVEE voltage//
	{0xB1,3, {0x0D,0x0D,0x0D}},

	//AVDD=2.5xVDCI//
	{0xB6,3, {0x34,0x44,0x44}},

	//AVEE= -2.5xVDCI//
	{0xB7,3, {0x45,0x34,0x34}},

	//Set VCL voltage//
	{0xB8,3, {0x24,0x24,0x24}},     

	//Set VGH//
	{0xB9,3, {0x34,0x34,0x34}},

	//Set VGL_REG=                         
	{0xBA,3, {0x16,0x16,0x16}},

	{0xF0,5, {0x55,0xAA,0x52,0x08,0x00}},
	{0xB1,1, {0xFC}},
	{0xB6,1, {0x04}},
	{0xB7,2, {0x74,0x74}},
	{0xB8,4, {0x01,0x07,0x07,0x07}},
	{0xBC,3, {0x00,0x00,0x00}},
	{0xBD,5, {0x01,0x84,0x07,0x31,0x00}},                                           
	{0xBE,5, {0x01,0x84,0x07,0x31,0x00}},                                     
	{0xBF,5, {0x01,0x84,0x07,0x31,0x00}},           
	{0x35,1, {0x00}},            
	{0x3A,1, {0x77}},
	{0xF0,5, {0x55,0xAA,0x52,0x08,0x00}},
	{0xC7,1,{0x02}},
	{0xC9,1,{0x11}},
	
	//Forward Scan// 
	{0xCA, 12,{0x01, 0xE4, 0xE4, 0xE4, 0xE4, 0xE4, 0xE4, 0xE4, 0x08, 0x08, 0x00, 0x00}},
	//CRL=0// 
	{0xB1,2,{0xFC, 0x00}},
	
	{0x11,1,{0x00}},
	{REGFLAG_DELAY, 150, {}},
	{0x29,1,{0x00}},
	{REGFLAG_DELAY, 50, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
#endif
};

static struct LCM_setting_table lcm_set_window[] = {
	{0x2A,	4,	{0x00, 0x00, (FRAME_WIDTH>>8), (FRAME_WIDTH&0xFF)}},
	{0x2B,	4,	{0x00, 0x00, (FRAME_HEIGHT>>8), (FRAME_HEIGHT&0xFF)}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_sleep_out_setting[] = {
	// Sleep Out
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},

	// Display ON
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 100, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 1, {0x00}},

	// Sleep Mode On
	{0x10, 1, {0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_backlight_level_setting[] = {
	{0x51, 1, {0xFF}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for(i = 0; i < count; i++) 
	{

		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) 
		{
			case REGFLAG_DELAY :
				MDELAY(table[i].count);
			break;

			case REGFLAG_END_OF_TABLE :
			break;				

			default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}

}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	//enable tearing-free
	params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
	//params->dbi.te_mode 				= LCM_DBI_TE_MODE_DISABLED;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
#else
	params->dsi.mode   = SYNC_EVENT_VDO_MODE;
#endif

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_TWO_LANE;

	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	// Not support in MT6573
	params->dsi.packet_size=256;

	// Video mode setting		
	params->dsi.intermediat_buffer_num = 2;

	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.word_count=480*3;

	params->dsi.vertical_sync_active				= 2; 
	params->dsi.vertical_backporch					= 10;
	params->dsi.vertical_frontporch					= 10;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 2;
	params->dsi.horizontal_backporch				= 80; //modify by fangjie for frame rate set to 60. 
	params->dsi.horizontal_frontporch				= 80; //modify by fangjie for frame rate set to 60. 
	params->dsi.horizontal_blanking_pixel			= 60;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	params->dsi.PLL_CLOCK=208; //modify by fangjie for frame rate set to 60. 
	params->dsi.ssc_disable = 0;
	params->dsi.ssc_range   = 5;

	//note: for Video mode
	//fps = data_rate * Lane_Number /( (Vsyn+VBP+VFP+Height)*(Hsyn+HBP+HFP+Width)*BPP)
	//data_rate = CLOCK_rate * 2;

}

static void lcm_init(void)
{
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(20);
	SET_RESET_PIN(1);
	MDELAY(120);
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);

}

static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);


	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
}

static void lcm_resume(void)
{
	lcm_init();
	//delete for PR904863
	//push_table(lcm_sleep_out_setting_hx8379c_tdt, sizeof(lcm_sleep_out_setting_hx8379c_tdt) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_update(unsigned int x, unsigned int y,
					   unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	data_array[3]= 0x00053902;
	data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[5]= (y1_LSB);
	data_array[6]= 0x002c3909;

	dsi_set_cmdq(data_array, 7, 0);

}

static void lcm_setbacklight(unsigned int level)
{
	unsigned int default_level = 145;
	unsigned int mapped_level = 0;

	//for LGE backlight IC mapping table
	if(level > 255) 
		level = 255;

	if(level >0) 
		mapped_level = default_level+(level)*(255-default_level)/(255);
	else
		mapped_level=0;

	// Refresh value of backlight level.
	lcm_backlight_level_setting[0].para_list[0] = mapped_level;

	push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_setpwm(unsigned int divider)
{
	// TBD
}

static unsigned int lcm_getpwm(unsigned int divider)
{
	// ref freq = 15MHz, B0h setting 0x80, so 80.6% * freq is pwm_clk;
	// pwm_clk / 255 / 2(lcm_setpwm() 6th params) = pwm_duration = 23706
	unsigned int pwm_clk = 23706 / (1<<divider);	
	return pwm_clk;
}

static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
	char  buffer[5]={0};
	int   array[4];

#if defined(LCM_ESD_DEBUG)
	printk("nt35512: lcm_esd_check enter\n");
#endif

	array[0] = 0x00043700;
	dsi_set_cmdq(array, 1, 1);

	//fangjie modify, because in DSI_dcs_read_lcm_reg_v2()
	//when the readsize>=3, the DI config is DSI_GERNERIC_READ_LONG_PACKET_ID. else DI is DSI_DCS_READ_PACKET_ID
	//old: read_reg_v2(0x09, buffer, 4); 
	read_reg_v2(0x0A, buffer, 2); 

#if defined(LCM_ESD_DEBUG)
	printk("lcm_esd_check buffer[0]=0x%x\n",buffer[0]); //0x80
	printk("lcm_esd_check buffer[1]=0x%x\n",buffer[1]); //0x73
	printk("lcm_esd_check buffer[2]=0x%x\n",buffer[2]); //0x3E
	printk("lcm_esd_check buffer[3]=0x%x\n",buffer[3]); //0xA6
#endif

	if((buffer[0]==0x9C))
	{
		return 0;
	}
	else
	{ 
		return 1;
	}
#endif
}

static unsigned int lcm_esd_recover(void)
{
#ifndef BUILD_LK 
	printk("lcm_esd_recover nt35512 enter");
	lcm_init();
	return 1;
#endif
}

// ---------------------------------------------------------------------------
//  Get LCM ID Information
// ---------------------------------------------------------------------------
static unsigned int lcm_compare_id()
{
	int id_type=0;	

	mt_set_gpio_mode(GPIO_LCM_ID1,GPIO_MODE_00);
	mt_set_gpio_pull_enable(GPIO_LCM_ID1, GPIO_PULL_DISABLE);
	mt_set_gpio_dir(GPIO_LCM_ID1, GPIO_DIR_IN);
	mt_set_gpio_mode(GPIO_LCM_ID2,GPIO_MODE_00);
	mt_set_gpio_pull_enable(GPIO_LCM_ID2, GPIO_PULL_DISABLE);
	mt_set_gpio_dir(GPIO_LCM_ID2, GPIO_DIR_IN);
	id_type = mt_get_gpio_in(GPIO_LCM_ID2)<<1 | mt_get_gpio_in(GPIO_LCM_ID1);

#if defined(BUILD_LK)
	printf("\t\t nt35512 [lcm_compare_id   id_type  %d ]\n" , id_type);		
#else
	printk("\t\t nt35512 [lcm_compare_id   id_type  %d ]\n" , id_type);	
#endif

#if defined(TARGET_RELOOK_SUPPORT)
	if (id_type == 1 ) //ili9806e_COE as 4th source ,and ID_tpye is 01bit. 
	{
		return 1 ;
	}
	else
	{
		return  0 ;
	}
#else
	return 0 ;
#endif
} 

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER nt35512_wvga_dsi_vdo_TDT_6572_lcm_drv = 
{
	.name			= "nt35512_wvga_dsi_vdo_TDT_6572",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
#if (LCM_DSI_CMD_MODE)
	.update         = lcm_update,
#endif

	//.set_backlight= lcm_setbacklight,
	//.set_pwm      = lcm_setpwm,
	//.get_pwm      = lcm_getpwm,

	.esd_check    = lcm_esd_check, //add by fangjie
	.esd_recover  = lcm_esd_recover,
	.compare_id    	= lcm_compare_id,
};




