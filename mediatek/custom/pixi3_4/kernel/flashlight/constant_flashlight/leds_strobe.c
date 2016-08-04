//###########################################
//##                                       ##
//##        MT6333 NOT support             ##
//## (MT6333 support code from L426)       ##
//## (key word MT63333)                    ##
//###########################################

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/time.h>
#include "kd_flashlight.h"
#include <asm/io.h>
#include <asm/uaccess.h>
#include "kd_camera_hw.h"
#include <cust_gpio_usage.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/xlog.h>
#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
#include <linux/mutex.h>
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif
#endif

/******************************************************************************
* Debug configuration
******************************************************************************/
// availible parameter
// ANDROID_LOG_ASSERT
// ANDROID_LOG_ERROR
// ANDROID_LOG_WARNING
// ANDROID_LOG_INFO
// ANDROID_LOG_DEBUG
// ANDROID_LOG_VERBOSE
#define TAG_NAME "leds_strobe.c"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    xlog_printk(ANDROID_LOG_DEBUG  , TAG_NAME, KERN_INFO  "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_WARN(fmt, arg...)        xlog_printk(ANDROID_LOG_WARNING, TAG_NAME, KERN_WARNING  "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_NOTICE(fmt, arg...)      xlog_printk(ANDROID_LOG_DEBUG  , TAG_NAME, KERN_NOTICE  "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_INFO(fmt, arg...)        xlog_printk(ANDROID_LOG_INFO   , TAG_NAME, KERN_INFO  "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_TRC_FUNC(f)              xlog_printk(ANDROID_LOG_DEBUG  , TAG_NAME,  "<%s>\n", __FUNCTION__);
#define PK_TRC_VERBOSE(fmt, arg...) xlog_printk(ANDROID_LOG_VERBOSE, TAG_NAME,  fmt, ##arg)
#define PK_ERROR(fmt, arg...)       xlog_printk(ANDROID_LOG_ERROR  , TAG_NAME, KERN_ERR "%s: " fmt, __FUNCTION__ ,##arg)


#define DEBUG_LEDS_STROBE

#ifdef  DEBUG_LEDS_STROBE
#define PK_DBG PK_DBG_FUNC
#define PK_VER PK_TRC_VERBOSE
#define PK_ERR PK_ERROR
#else
#define PK_DBG(a,...)
#define PK_VER(a,...)
#define PK_ERR(a,...)
#endif

/******************************************************************************
* local variables
******************************************************************************/


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
static DEFINE_MUTEX(g_strobeSem);
#else
static DECLARE_MUTEX(g_strobeSem);
#endif

//#define GPIO_CAMERA_FLASH_EN GPIO137
//@@ #define FLASH_GPIO_ENF GPIO12
//@@ #define FLASH_GPIO_ENT GPIO13

#define STROBE_DEVICE_ID 0xC6
#define GPIO_FLASH_TORCH_EN_PIN GPIO20 //fangjie modify for Pixi3-4
#define GPIO_FLASH_TORCH_SWITCH_PIN  GPIO16 //fangjie modify for Pixi3-4
#define FLASHLIGHT_YUV_TORCH_LEVEL 6
#define FLASHLIGHT_YUV_NORMAL_LEVEL 12

/*****************************************************************************
Functions
*****************************************************************************/
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
static DEFINE_SPINLOCK(g_strobeSMPLock); /* cotta-- SMP proection */
static void work_timeOutFunc(struct work_struct *data);
static void work_timeOutFunc_150ms(struct work_struct *data);

static struct hrtimer g_timeOutTimer;
static struct work_struct workTimeOut;
static struct hrtimer g_timeOutTimer_150ms;
static struct work_struct workTimeOut_150ms;
static u32 	strobe_Res = 0;
static u32 	strobe_Timeus = 0;
static BOOL g_strobe_On = 0;
static int 	g_duty=-1;
static int 	g_timeOutTimeMs=0;

int FL_Enable(void)
{
	PK_DBG(" FL_Enable line=%d , g_duty =%d\n",__LINE__ , g_duty );
	
	if(g_duty == FLASHLIGHT_YUV_TORCH_LEVEL)
	{
		mt_set_gpio_out(GPIO_FLASH_TORCH_EN_PIN,1);
		mt_set_gpio_out(GPIO_FLASH_TORCH_SWITCH_PIN,0);// 1: FLASH,  0: Torch			
	}
	else if (g_duty == FLASHLIGHT_YUV_NORMAL_LEVEL)
	{
		mt_set_gpio_out(GPIO_FLASH_TORCH_EN_PIN,1);	
		mt_set_gpio_out(GPIO_FLASH_TORCH_SWITCH_PIN,1);// 1: FLASH,  0: Torch	

                /*Begin ersen.shang 20150113 for increase flash time for SGM*/
		ktime_t ktime;
		ktime = ktime_set( 0, 150*1000000 );
		hrtimer_start( &g_timeOutTimer_150ms , ktime, HRTIMER_MODE_REL );		
                /*End ersen.shang 20150113 for increase flash time for SGM*/
	}
	
	return 0;
}

int FL_Disable(void)
{
	PK_DBG(" FL_Disable line=%d\n",__LINE__);
	
	mt_set_gpio_out(GPIO_FLASH_TORCH_EN_PIN,0);
	mt_set_gpio_out(GPIO_FLASH_TORCH_SWITCH_PIN,0); // for power consumpation
	return 0;
}

int FL_dim_duty(kal_uint32 duty)
{
	PK_DBG(" FL_dim_duty line=%d duty =%d \n",__LINE__ , duty);
	g_duty =  duty;
	return 0;
}

int FL_Init(void)
{
	PK_DBG(" FL_Init line=%d\n",__LINE__);
	
	if(mt_set_gpio_mode(GPIO_FLASH_TORCH_EN_PIN,GPIO_MODE_00))
	{
		PK_DBG("[constant_flashlight] set gpio mode failed!! \n");
	}
	if(mt_set_gpio_dir(GPIO_FLASH_TORCH_EN_PIN,GPIO_DIR_OUT))
	{
		PK_DBG("[constant_flashlight] set gpio dir failed!! \n");
	}

	if(mt_set_gpio_mode(GPIO_FLASH_TORCH_SWITCH_PIN,GPIO_MODE_00))
	{
		PK_DBG("[constant_flashlight switch] set gpio mode failed!! \n");
	}
	if(mt_set_gpio_dir(GPIO_FLASH_TORCH_SWITCH_PIN,GPIO_DIR_OUT))
	{
		PK_DBG("[constant_flashlight switch] set gpio dir failed!! \n");
	}

	INIT_WORK(&workTimeOut, work_timeOutFunc);
	INIT_WORK(&workTimeOut_150ms, work_timeOutFunc_150ms);
	
	PK_DBG(" FL_Init line=%d\n",__LINE__);
	return 0;
}

int FL_Uninit(void)
{
	PK_DBG(" FL_Uninit line=%d\n",__LINE__);
	FL_Disable();
	return 0;
}

/*****************************************************************************
User interface
*****************************************************************************/
static void work_timeOutFunc(struct work_struct *data)
{
	PK_DBG("work_timeOutFunc\n");
	FL_Disable();
}

static void work_timeOutFunc_150ms(struct work_struct *data)
{
	PK_DBG("work_timeOutFunc_150ms\n");

        /*Begin ersen.shang 20150113 for increase flash time for SGM*/
	mt_set_gpio_out(GPIO_FLASH_TORCH_SWITCH_PIN,0); // for power consumpation
	udelay(10);
	mt_set_gpio_out(GPIO_FLASH_TORCH_SWITCH_PIN,1);// 1: FLASH,  0: Torch	
        /*End ersen.shang 20150113 for increase flash time for SGM*/
}

enum hrtimer_restart ledTimeOutCallback(struct hrtimer *timer)
{
	PK_DBG("ledTimeOutCallback\n");

	schedule_work(&workTimeOut);
	return HRTIMER_NORESTART;
}

enum hrtimer_restart ledTimeOutCallback_150ms(struct hrtimer *timer)
{
	PK_DBG("ledTimeOutCallback_150ms\n");

	schedule_work(&workTimeOut_150ms);
	return HRTIMER_NORESTART;
}

void timerInit(void)
{
	PK_DBG("timerInit\n");

	g_timeOutTimeMs=1000;
	hrtimer_init( &g_timeOutTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
	g_timeOutTimer.function=ledTimeOutCallback;

        /*Begin ersen.shang 20150113 for increase flash time for SGM*/
	hrtimer_init( &g_timeOutTimer_150ms, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
	g_timeOutTimer_150ms.function=ledTimeOutCallback_150ms;
        /*End ersen.shang 20150113 for increase flash time for SGM*/
}

int TORCH_Enable(void)
{
	int ret;

	PK_DBG("TORCH_Enable\n");
	
	ret = mt_set_gpio_out(GPIO_FLASH_TORCH_EN_PIN, 1);
	mt_set_gpio_out(GPIO_FLASH_TORCH_SWITCH_PIN,0); // 1: FLASH,  0: Torch

	if (ret) 
	{
		PK_DBG("torch@debug:enable torch failed!\n");	
		return -1;
	} 
	else 
	{
		return 0;
	}
}

int TORCH_Disable(void)
{
	int ret;
	
	PK_DBG("TORCH_Disable\n");	
	
	ret = mt_set_gpio_out(GPIO_FLASH_TORCH_EN_PIN, 0);
	mt_set_gpio_out(GPIO_FLASH_TORCH_SWITCH_PIN,0);// 1: FLASH,  0: Torch

	if (ret) 
	{
		PK_DBG("torch@debug:disable torch failed!\n");
		return -1;
	} 
	else 
	{
		return 0;
	}
}

static int constant_flashlight_ioctl(MUINT32 cmd, MUINT32 arg)
{
	int i4RetValue = 0;
	int ior_shift;
	int iow_shift;
	int iowr_shift;
	int iFlashType;
	
	ior_shift = cmd - (_IOR(FLASHLIGHT_MAGIC,0, int));
	iow_shift = cmd - (_IOW(FLASHLIGHT_MAGIC,0, int));
	iowr_shift = cmd - (_IOWR(FLASHLIGHT_MAGIC,0, int));
	iFlashType = (int)FLASHLIGHT_NONE;
	
	PK_DBG("constant_flashlight_ioctl cmd = %d , arg = %d \n" , cmd , arg);		
	PK_DBG("constant_flashlight_ioctl() line=%d ior_shift=%d, iow_shift=%d iowr_shift=%d arg=%d\n",__LINE__, ior_shift, iow_shift, iowr_shift, arg);
	
	switch(cmd)
	{
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		PK_DBG("FLASH_IOC_SET_TIME_OUT_TIME_MS: %d\n",arg);
		g_timeOutTimeMs=arg;
		break;

	case FLASH_IOC_SET_DUTY :
		PK_DBG("FLASHLIGHT_DUTY: %d\n",arg);
		FL_dim_duty(arg);
		break;

	case FLASH_IOC_SET_STEP:
		PK_DBG("FLASH_IOC_SET_STEP: %d\n",arg);
		break;

	case FLASH_IOC_SET_ONOFF :
		PK_DBG("FLASHLIGHT_ONOFF: %d\n",arg);
		if(arg==1)
		{
			if(g_timeOutTimeMs!=0)
			{
				ktime_t ktime;
				ktime = ktime_set( 0, g_timeOutTimeMs*1000000 );
				hrtimer_start( &g_timeOutTimer, ktime, HRTIMER_MODE_REL );
			}
			FL_Enable();
		}
		else
		{
			hrtimer_cancel( &g_timeOutTimer );
                        
                        /*Begin ersen.shang 20150113 for increase flash time for SGM*/
                        /*cancel timer and flush work queue for avoid sync abnormal between APP and Driver*/
			hrtimer_cancel( &g_timeOutTimer_150ms);
			flush_work(&workTimeOut_150ms);		
                        /*End ersen.shang 20150113 for increase flash time for SGM*/		

			FL_Disable();
		}
		break;
		
	case FLASHLIGHTIOC_G_FLASHTYPE:
		spin_lock(&g_strobeSMPLock);    /* cotta-- SMP proection */
		iFlashType = FLASHLIGHT_LED_CONSTANT;
		spin_unlock(&g_strobeSMPLock);
		
		if(copy_to_user((void __user *) arg , (void*)&iFlashType , _IOC_SIZE(cmd)))
		{
			PK_DBG(" ioctl copy to user failed\n");
			return -EFAULT;
		}
		break;

	case FLASHLIGHTIOC_ENABLE_STATUS:
		PK_DBG("**********torch g_strobe_on = %d \n", g_strobe_On);
		copy_to_user((void __user *) arg , (void*)&g_strobe_On , sizeof(int));
		break;
		
	case FLASHLIGHT_TORCH_SELECT:
		PK_DBG("@@@@@@FLASHLIGHT_TORCH_SELECT@@@@@@\n");
		if (arg)
		{
			TORCH_Enable();
			g_strobe_On = TRUE;
		} 
		else 
		{
			TORCH_Disable();
			g_strobe_On = FALSE;
		}
		break;

	default :
		PK_DBG(" No such command \n");
		i4RetValue = -EPERM;
		break;
	}

	return i4RetValue;
}

static int constant_flashlight_open(void *pArg)
{
	int i4RetValue = 0;
	
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	mt_set_gpio_mode(GPIO_FLASH_TORCH_SWITCH_PIN,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_FLASH_TORCH_SWITCH_PIN,GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_FLASH_TORCH_SWITCH_PIN,0);   // 1: FLASH,  0: Torch
	mt_set_gpio_mode(GPIO_FLASH_TORCH_EN_PIN,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_FLASH_TORCH_EN_PIN,GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_FLASH_TORCH_EN_PIN,0);

	if (0 == strobe_Res)
	{
		FL_Init();
		timerInit();
	}
	
	spin_lock_irq(&g_strobeSMPLock);

	if(strobe_Res)
	{
		PK_ERR(" busy!\n");
		i4RetValue = -EBUSY;
	}
	else
	{
		strobe_Res += 1;
	}

	spin_unlock_irq(&g_strobeSMPLock);
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	return i4RetValue;
}

static int constant_flashlight_release(void *pArg)
{
	PK_DBG(" constant_flashlight_release\n");

	if (strobe_Res)
	{
		spin_lock_irq(&g_strobeSMPLock);

		strobe_Res = 0;
		strobe_Timeus = 0;

		/* LED On Status */
		g_strobe_On = FALSE;

		spin_unlock_irq(&g_strobeSMPLock);

		FL_Uninit();
	}

	PK_DBG(" Done\n");

	return 0;

}

FLASHLIGHT_FUNCTION_STRUCT	constantFlashlightFunc=
{
	constant_flashlight_open,
	constant_flashlight_release,
	constant_flashlight_ioctl
};

MUINT32 constantFlashlightInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{
	if (pfFunc != NULL)
	{
		*pfFunc = &constantFlashlightFunc;
	}
	return 0;
}


/* LED flash control for high current capture mode*/
ssize_t strobe_VDIrq(void)
{

	return 0;
}

EXPORT_SYMBOL(strobe_VDIrq);

