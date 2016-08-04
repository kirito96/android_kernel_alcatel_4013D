#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/ctype.h>
#include <asm/atomic.h>

#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/workqueue.h>
#include <linux/switch.h>
#include <linux/delay.h>

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/kthread.h>
#include <linux/input.h>
#include <linux/wakelock.h>
#include <linux/time.h>

#include <linux/string.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_reg_base.h>
#include <mach/irqs.h>

#include <cust_eint.h>
#include <cust_gpio_usage.h>
#include <mach/mt_gpio.h>
#include <mach/eint.h>

//GPIO Define must(GPIOX | 0x80000000 ) for skip mediatek code of checking weather direct using GPIO number. DCT will define the GPIO to (GPIOX |  0x80000000)
#define HALL_GPIO GPIO99
#define HALL_EINT 7
//Delay ms
#define DELAY 200

typedef enum {
	TYPE_UNKNOWN,
	TYPE_LED_COVER,
	TYPE_AUDIO_OUTPUT,
	TYPE_KEYPAD,
	TYPE_WIRELESS_CHARGER,
	TYPE_EINK
	} SCOVER_TYPE;
	
typedef enum{
	SWITCH_STATE_UNKNOWN,
	SWITCH_STATE_CLOSE,
	SWITCH_STATE_OPEN
} SCOVER_STATE;

static DECLARE_WAIT_QUEUE_HEAD(hall_thread_wq);
atomic_t gpio_value = ATOMIC_INIT(1);  //default setting -- open the cover and the gpio is high
atomic_t eint_triger = ATOMIC_INIT(0);

struct device *uevent_dev;
	
SCOVER_TYPE scover_type = TYPE_UNKNOWN;
SCOVER_STATE scover_state = SWITCH_STATE_OPEN;

SCOVER_TYPE scover_get_type(void)
{
	return scover_type; 
}

SCOVER_STATE scover_get_switch_state(void)
{
	if(1 == atomic_read(&gpio_value))
		scover_state = SWITCH_STATE_OPEN;
	else
		scover_state = SWITCH_STATE_CLOSE;
	
	return scover_state;
}

static ssize_t scover_show_state(struct device_driver *drv, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "%d\n", scover_get_switch_state());
}
static DRIVER_ATTR(switch_state, S_IWUSR | S_IRUGO, scover_show_state, NULL);

static ssize_t scover_show_type(struct device_driver *drv, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "%d\n", scover_get_type());
}
static DRIVER_ATTR(type, S_IWUSR | S_IRUGO, scover_show_type, NULL);

int hall_thread_kthread(void *data)
{
	while(1)
	{
		wait_event(hall_thread_wq,(1 == atomic_read(&eint_triger)));
		atomic_set(&eint_triger,0);
		msleep(DELAY);
		printk("[hall_driver] value = %d \n",atomic_read(&gpio_value));
		kobject_uevent(&(uevent_dev->kobj), KOBJ_CHANGE);
	}
	return 0;
}

void hal_eint_handle(void)
{
	if(1 == atomic_read(&gpio_value))
	{
		atomic_set(&gpio_value,0);
		mt_eint_set_polarity(HALL_EINT, MT_EINT_POL_POS);
	}
	else
	{
		atomic_set(&gpio_value,1);
		mt_eint_set_polarity(HALL_EINT, MT_EINT_POL_NEG);
	}
	atomic_set(&eint_triger,1);
	wake_up(&hall_thread_wq);
}

static int hall_probe(struct platform_device *dev)
{
	int ret;
    printk("[hall_driver] hall probe --- \n");
	kthread_run(hall_thread_kthread, NULL, "hall_thread_kthread");

	mt_set_gpio_mode(HALL_GPIO,7);
	mt_set_gpio_dir(HALL_GPIO,0);	

  ret = driver_create_file((dev->dev).driver, &driver_attr_type);
    if (ret) {
        printk("[hall driver] can not create driver attribute file type!\n");
    }
  ret = driver_create_file((dev->dev).driver, &driver_attr_switch_state);
    if (ret) {
        printk("[hall driver] can not create driver attribute file state!\n");
    }
	//EINT Intial
	uevent_dev = &(dev->dev);
	scover_type = TYPE_LED_COVER;
	mt_eint_set_hw_debounce(HALL_EINT,1); //EINT4 Debounce Enable,1ms
  	atomic_set(&gpio_value,1);
	mt_eint_registration(HALL_EINT,EINTF_TRIGGER_LOW,hal_eint_handle,1);
	//EINT Intial End 
	return 0;
}

struct platform_device hall_device = {
    .name   = "scover",
    .id        = -1,
};

static struct platform_driver hall_driver = {
	.probe         = hall_probe,
//    .remove        = hall_remove,
//    .shutdown      = hall_shutdown,
//    .suspend       = hall_suspend,
//    .resume        = hall_resume,
	.driver        = {
		.name = "scover",
	},
};

static int __init hall_init(void)
{
    int ret;

    ret = platform_device_register(&hall_device);
    if (ret) {
        printk("****[hal_driver] Unable to device register(%d)\n", ret);
    return ret;
    }
    
    ret = platform_driver_register(&hall_driver);
    if (ret) {
        printk("****[hall_driver] Unable to register driver (%d)\n", ret);
    return ret;
    }
    
    printk("****[hall_driver] Hall Sensor inital done \n");
    return 0;
}

static int __exit hall_exit(void)
{

}

module_init(hall_init);
module_exit(hall_exit);

MODULE_AUTHOR("Fei Chen");
MODULE_DESCRIPTION("HALL Sensor Driver");
MODULE_LICENSE("GPL");
