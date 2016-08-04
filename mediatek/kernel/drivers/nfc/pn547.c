/*
 * Copyright (C) 2010 Trusted Logic S.A.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <mach/mt_gpio.h>
#include <mach/eint.h>
#include <cust_gpio_usage.h>
#include <cust_eint.h>
#include <linux/hwmsen_dev.h>
#include <linux/switch.h>




#define VEN_PIN         	GPIO57
#ifdef NXP_NFC_SUPPORT_SOUL4
#define GPIO4_PIN       	GPIO97  //56 according to sch of soul4
#else
#define GPIO4_PIN		GPIO56
#endif
#define IRQ_PIN         	GPIO31
#define EINT_NUM                6
#define PN544_I2C_GROUP_ID	1
#define PN544_WRITE_ID		0x56//0x28
#define PN544_DRVNAME		"pn544"
static const struct i2c_device_id pn544_id[] = { { "pn544", 0 }, {} };

static struct i2c_board_info __initdata pn544_i2c_nfc = { I2C_BOARD_INFO("pn544", (PN544_WRITE_ID >> 1)) };

#define MAX_BUFFER_SIZE		255

#define PN544_MAGIC		0xE9
#define PN544_SET_PWR		_IOW(PN544_MAGIC, 0x01, unsigned int)
static struct switch_dev nfc_state;
struct pn544_dev	{
	wait_queue_head_t	read_wq;
	struct mutex		read_mutex;
	struct i2c_client	*client;
	struct miscdevice	pn544_device;
	bool			irq_enabled;
	spinlock_t		irq_enabled_lock;
};

static struct pn544_dev *p_pn544_dev = NULL;
//For DMA
static char *I2CDMAWriteBuf = NULL;
static unsigned int I2CDMAWriteBuf_pa;// = NULL;
static char *I2CDMAReadBuf = NULL;
static unsigned int I2CDMAReadBuf_pa;// = NULL;
static int nfc_exit=0;
/*----------------------------------------------------------------------------*/
static ssize_t nfc_show(struct device *dev, 
                                  struct device_attribute *attr, char *buf)
{	
    printk("nfc test: nfc show function!\n");

    if (!dev ) {
       
        return 0;
    }
    printk("The nfc is pn547 \n");
    return snprintf(buf, PAGE_SIZE, "%d\n", nfc_exit);    
   //Lightsensor flag,Let the APP know the Psensor is CTP type or physical type// add by llf
}
/*----------------------------------------------------------------------------*/
static ssize_t nfc_store(struct device* dev, 
                                   struct device_attribute *attr, const char *buf, size_t count)
{

	return count;
}   
/*----------------------------------------------------------------------------*/
DEVICE_ATTR(nfc,     S_IWUSR | S_IRUGO, nfc_show, nfc_store);
/*----------------------------------------------------------------------------*/
#if 0
static int pn544_compatible_test(struct pn544_dev *pn544_dev)
{
         int ret;
	  struct device * nfc_device;
         unsigned char send_reset[] = {0x20, 0x00, 0x01, 0x01};//PN547 RSET Frame

	  nfc_device=hwmsen_get_compatible_dev();
	  if (device_create_file(nfc_device, &dev_attr_nfc) < 0)

    			printk("Failed to create device file(%s)!\n", dev_attr_nfc.attr.name);

         // power on

         mt_set_gpio_out(GPIO4_PIN, 0);

         mt_set_gpio_out(VEN_PIN, 0);

         msleep(10);


         // power off

         mt_set_gpio_out(GPIO4_PIN, 0);

         mt_set_gpio_out(VEN_PIN, 1);

         msleep(10);

         // power on

         mt_set_gpio_out(GPIO4_PIN, 0);

         mt_set_gpio_out(VEN_PIN, 0);

         msleep(10);
 

         // write data, note use dma buffer to transfer, or change the i2c dma flag, if not, will report error
	memcpy(I2CDMAWriteBuf, send_reset, sizeof(send_reset));
         ret = i2c_master_send(pn544_dev->client,  (unsigned char *)I2CDMAWriteBuf_pa, sizeof(send_reset));

 
         // power off

         mt_set_gpio_out(GPIO4_PIN, 0);

         mt_set_gpio_out(VEN_PIN, 1);

         msleep(10);

         

         if (ret != sizeof(send_reset)) {

                   printk("pn544 %s : no pn544 or it is bad %d\n", __func__, ret);
			nfc_exit=0;
                   return -1;

         }

         else{
			nfc_exit=1;
                   return 0;

         }

}
#endif
static void pn544_disable_irq(struct pn544_dev *pn544_dev)
{
	unsigned long flags;


	spin_lock_irqsave(&pn544_dev->irq_enabled_lock, flags);
	if (pn544_dev->irq_enabled) {
        mt65xx_eint_mask(EINT_NUM);
		pn544_dev->irq_enabled = false;
	}
	spin_unlock_irqrestore(&pn544_dev->irq_enabled_lock, flags);


}

void pn544_dev_irq_handler(void)
{
	struct pn544_dev *pn544_dev = p_pn544_dev;

	if (!mt_get_gpio_in(IRQ_PIN)) {

		printk(KERN_DEBUG "***************\n");
		
		return;
	}

	pn544_disable_irq(pn544_dev);

	/* Wake up waiting readers */
	wake_up(&pn544_dev->read_wq);

}

static ssize_t pn544_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *offset)
{
	struct pn544_dev *pn544_dev = filp->private_data;
	int ret,i;
	char tmp[MAX_BUFFER_SIZE];

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	printk("pn544 %s : reading %zu bytes.\n", __func__, count);

	mutex_lock(&pn544_dev->read_mutex);

	if (!mt_get_gpio_in(IRQ_PIN)) {
        
		printk("pn544 read no event\n");
		
		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			goto fail;
		}
		
		printk("pn544 read wait event\n");
		
		pn544_dev->irq_enabled = true;
		mt65xx_eint_unmask(EINT_NUM);
	
		ret = wait_event_interruptible(pn544_dev->read_wq, mt_get_gpio_in(IRQ_PIN));

		pn544_disable_irq(pn544_dev);

		if (ret) {
			printk("pn544 read wait event error\n");
			goto fail;
		}
	}

	/* Read data */
	
    ret = i2c_master_recv(pn544_dev->client, (unsigned char *)I2CDMAReadBuf_pa, count);
	   
	mutex_unlock(&pn544_dev->read_mutex);

	if (ret < 0) {
		pr_err("pn544 %s: i2c_master_recv returned %d\n", __func__, ret);
		return ret;
	}
	if (ret > count) {
		pr_err("pn544 %s: received too many bytes from i2c (%d)\n", __func__, ret);
		return -EIO;
	}
	
   if (copy_to_user(buf, I2CDMAReadBuf, ret)) 
   {
      printk(KERN_DEBUG "%s : failed to copy to user space\n", __func__);
      return -EFAULT;
   }

	printk("pn544 IFD->PC:");
	for(i = 0; i < ret; i++) {
		printk(" %02X", I2CDMAReadBuf[i]);
	}
	printk("\n");

	return ret;

fail:
	mutex_unlock(&pn544_dev->read_mutex);
	return ret;
}

static ssize_t pn544_dev_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset)
{
	struct pn544_dev *pn544_dev;
	int ret, i,idx = 0;
	char tmp[MAX_BUFFER_SIZE];

	pn544_dev = filp->private_data;

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	if (copy_from_user(I2CDMAWriteBuf, &buf[(idx*255)], count)) 
	{
		printk(KERN_DEBUG "%s : failed to copy from user space\n", __func__);
		return -EFAULT;
	}
	printk("pn544 %s : writing %zu bytes.\n", __func__, count);
	/* Write data */

      ret = i2c_master_send(pn544_dev->client, (unsigned char *)I2CDMAWriteBuf_pa, count);
	if (ret != count) {
		pr_err("pn544 %s : i2c_master_send returned %d\n", __func__, ret);
		ret = -EIO;
	}
	printk("pn544 PC->IFD:");
	for(i = 0; i < count; i++) {
		printk(" %02X\n", I2CDMAWriteBuf[i]);
	}
	printk("\n");
	

	return ret;
}

static int pn544_dev_open(struct inode *inode, struct file *filp)
{
	struct pn544_dev *pn544_dev = container_of(filp->private_data,
						struct pn544_dev,
						pn544_device);

	filp->private_data = pn544_dev;
	
	pr_debug("pn544 %s : %d,%d\n", __func__, imajor(inode), iminor(inode));

	return 0;
}

static long pn544_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
		case PN544_SET_PWR:
			if (arg == 2) {
				/* power on with firmware download (requires hw reset)
				 */
				printk("pn544 %s power on with firmware\n", __func__);
				mt_set_gpio_out(VEN_PIN, 0);
			printk(" ppn544_dev_ioctl_VEN_PIN=%d\n",mt_get_gpio_out(VEN_PIN));
				mt_set_gpio_out(GPIO4_PIN, 1);
			printk(" ppn544_dev_ioctl_GPIO4_PIN=%d\n",mt_get_gpio_out(GPIO4_PIN));
				msleep(10);
				mt_set_gpio_out(VEN_PIN, 1);
			printk(" ppn544_dev_ioctl_VEN_PIN=%d\n",mt_get_gpio_out(VEN_PIN));
				msleep(50);
				mt_set_gpio_out(VEN_PIN, 0);
			printk(" ppn544_dev_ioctl_VEN_PIN=%d\n",mt_get_gpio_out(VEN_PIN));
				msleep(10);
			} else if (arg == 1) {
				/* power on */
				printk("pn544 %s power on\n", __func__);
				mt_set_gpio_out(GPIO4_PIN, 0);
				//mt_set_gpio_out(VEN_PIN, 1);
				mt_set_gpio_out(VEN_PIN, 0);
				msleep(10);
			} else  if (arg == 0) {
				/* power off */
				printk("pn544 %s power off\n", __func__);
				mt_set_gpio_out(GPIO4_PIN, 0);
				//mt_set_gpio_out(VEN_PIN, 0);
				mt_set_gpio_out(VEN_PIN, 1);
				msleep(10);
			} else {
				printk("pn544 %s bad arg %lu\n", __func__, arg);
				return -EINVAL;
			}
			break;
		default:
			printk("pn544 %s bad ioctl %u\n", __func__, cmd);
			return -EINVAL;
	}

	return 0;
}

static const struct file_operations pn544_dev_fops = {
	.owner	= THIS_MODULE,
	.llseek	= no_llseek,
	.read	= pn544_dev_read,
	.write	= pn544_dev_write,
	.open	= pn544_dev_open,
	.unlocked_ioctl  = pn544_dev_ioctl,
};

static int pn544_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	struct pn544_dev *pn544_dev;

	unsigned char send_reset[] = {0x20, 0x00, 0x01, 0x01};//Add for test PN547 RSET Frame 
	unsigned char recv_response[40] = {0};
         int test_ret, i,test_count = 3;
         		 

	printk("pn544 nfc probe step01 is ok\n");
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("pn544 %s : need I2C_FUNC_I2C\n", __func__);
		return  -ENODEV;
	}

	printk("pn544 nfc probe step02 is ok\n");

	pn544_dev = kzalloc(sizeof(*pn544_dev), GFP_KERNEL);
	if (pn544_dev == NULL) {
		dev_err(&client->dev, "pn544 failed to allocate memory for module data\n");
		return -ENOMEM;
	}
	memset(pn544_dev, 0, sizeof(struct pn544_dev));
	p_pn544_dev = pn544_dev;

	printk("pn544 nfc probe step03 is ok\n");
	
	client->addr = (client->addr & I2C_MASK_FLAG);
	client->addr = (client->addr | I2C_DMA_FLAG);
    client->addr = (client->addr | I2C_DIRECTION_FLAG);
    client->timing = 400;
	pn544_dev->client = client;

	/* init mutex and queues */
	init_waitqueue_head(&pn544_dev->read_wq);
	mutex_init(&pn544_dev->read_mutex);
	spin_lock_init(&pn544_dev->irq_enabled_lock);

	pn544_dev->pn544_device.minor = MISC_DYNAMIC_MINOR;
	pn544_dev->pn544_device.name = PN544_DRVNAME;
	pn544_dev->pn544_device.fops = &pn544_dev_fops;

	ret = misc_register(&pn544_dev->pn544_device);
	if (ret) {
		pr_err("pn544 %s : misc_register failed\n", __FILE__);
		goto err_misc_register;
	}
    
	printk("pn544 nfc probe step04 is ok\n");
    
	I2CDMAWriteBuf = (char *)dma_alloc_coherent(NULL, MAX_BUFFER_SIZE, &I2CDMAWriteBuf_pa, GFP_KERNEL);
	if (I2CDMAWriteBuf == NULL) 
	{
		pr_err("%s : failed to allocate dma buffer\n", __func__);
		goto err_request_irq_failed;
	}
	
	I2CDMAReadBuf = (char *)dma_alloc_coherent(NULL, MAX_BUFFER_SIZE, &I2CDMAReadBuf_pa, GFP_KERNEL);
	if (I2CDMAReadBuf == NULL) 
	{
		pr_err("%s : failed to allocate dma buffer\n", __func__);
		goto err_request_irq_failed;
	}
        printk(KERN_DEBUG "%s :I2CDMAWriteBuf_pa %d, I2CDMAReadBuf_pa,%d\n", __func__, I2CDMAWriteBuf_pa, I2CDMAReadBuf_pa);

	/* VEN */
	mt_set_gpio_mode(VEN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(VEN_PIN, GPIO_DIR_OUT);
    
	/* GPIO4 */
	mt_set_gpio_mode(GPIO4_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO4_PIN, GPIO_DIR_OUT);
    
	/* IRQ */
	mt_set_gpio_mode(IRQ_PIN, GPIO_MODE_02);
	mt_set_gpio_dir(IRQ_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(IRQ_PIN, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(IRQ_PIN, GPIO_PULL_DOWN);
    
	printk("pn544 nfc probe step05 is ok\n");
    
	pn544_dev->irq_enabled = true;
	mt65xx_eint_set_sens(EINT_NUM, CUST_EINT_LEVEL_SENSITIVE);
	mt65xx_eint_set_hw_debounce(EINT_NUM, 0);
	mt65xx_eint_registration(EINT_NUM, CUST_EINT_DEBOUNCE_DISABLE, CUST_EINT_POLARITY_HIGH, pn544_dev_irq_handler, 0);
	
	pn544_disable_irq(pn544_dev);
	mt65xx_eint_mask(EINT_NUM);

	client->irq = EINT_NUM;
	printk("%s : requesting IRQ %d\n", __func__, client->irq);
	i2c_set_clientdata(client, pn544_dev);
	
	printk("pn544 nfc probe step06 is ok\n");
	//pn544_compatible_test(pn544_dev);
	printk("pn544 nfc probe step07 is ok\n");

/******* DO for test wether NFC chip is exist start *******/
         nfc_state.name = "pn54x";
 	nfc_state.index = 0;
	nfc_state.state = 0;
	if(switch_dev_register(&nfc_state))
		printk("######pn547 switch device register failed!\n");
	/* power on */
	mt_set_gpio_out(GPIO4_PIN, 0);
	mt_set_gpio_out(VEN_PIN, 0);
	msleep(10);
	mt_set_gpio_out(GPIO4_PIN, 0);
	mt_set_gpio_out(VEN_PIN, 1);
	msleep(10);
	mt_set_gpio_out(GPIO4_PIN, 0);
	mt_set_gpio_out(VEN_PIN, 0);
	msleep(10);
  	memcpy(I2CDMAWriteBuf, send_reset, sizeof(send_reset));
        test_ret = i2c_master_send(pn544_dev->client, (unsigned char*)I2CDMAWriteBuf_pa, sizeof(send_reset));
        
        if(test_ret < 0){
             printk("######PN547 iic write failed and try again.\n");
	     msleep(50);	
             test_ret = i2c_master_send(pn544_dev->client, (unsigned char*)I2CDMAWriteBuf_pa, sizeof(send_reset));
		if(test_ret < 0)
			printk("######PN547 iic test failed!\n");
		}
         printk("######pn547 iic test write success!\n");
        //Send RESET Frame[0x20, 0x00, 0x01, 0x01] 
        //Expect to receive Response Frame from NFC Chip[0x40 0x00 0x03 0x00 0x10 0x01] six Byte
          while (test_count--){
		msleep(10);
		//test_ret = i2c_master_recv(pn544_dev->client, &recv_response[0], 6);
		test_ret = i2c_master_recv(pn544_dev->client, (unsigned char*)I2CDMAReadBuf_pa, 6);
		memcpy(recv_response, I2CDMAReadBuf, 6);
		if (test_ret < 0) {			
			printk("######pn547 read error code = %d", test_ret);
			switch_set_state((struct switch_dev*)&nfc_state, 0);		
				
		}
		else{
			if (recv_response[0] == 0x51) continue;
			
			printk("\n ######pn547 NFC->PC: ");
			
			for (i=0;i<6;i++){
				printk("%.2X ", recv_response[i]);}
			switch_set_state((struct switch_dev*)&nfc_state, 1);	
			printk("######pn547 is exist.\n");
			break;
		}
	}     
	/* power off */
	mt_set_gpio_out(GPIO4_PIN, 0);
	mt_set_gpio_out(VEN_PIN, 1);
 
/******* DO for test wether NFC chip is exist end *******/
	return 0;

err_dma_alloc:
	misc_deregister(&pn544_dev->pn544_device);
err_misc_register:
	mutex_destroy(&pn544_dev->read_mutex);
	kfree(pn544_dev);
	p_pn544_dev = NULL;
err_request_irq_failed:
	misc_deregister(&pn544_dev->pn544_device);   
	return ret;
}

static int pn544_remove(struct i2c_client *client)
{
	struct pn544_dev *pn544_dev;

	if (I2CDMAWriteBuf)
	{
		dma_free_coherent(NULL, MAX_BUFFER_SIZE, I2CDMAWriteBuf, I2CDMAWriteBuf_pa);
		I2CDMAWriteBuf = NULL;
		I2CDMAWriteBuf_pa = 0;
	}

	if (I2CDMAReadBuf)
	{
		dma_free_coherent(NULL, MAX_BUFFER_SIZE, I2CDMAReadBuf, I2CDMAReadBuf_pa);
		I2CDMAReadBuf = NULL;
		I2CDMAReadBuf_pa = 0;
	}

	pn544_dev = i2c_get_clientdata(client);
	misc_deregister(&pn544_dev->pn544_device);
	mutex_destroy(&pn544_dev->read_mutex);
	kfree(pn544_dev);
	p_pn544_dev = NULL;
	
	return 0;
}

static int pn544_detect(struct i2c_client *client, struct i2c_board_info *info) 
{         
	printk(" pn544_detect \n");
	strcpy(info->type, PN544_DRVNAME);
	return 0;
}

static struct i2c_driver pn544_driver = {
	
	.probe		= pn544_probe,
	.remove		= pn544_remove,
	.detect            = pn544_detect,
	.id_table	= pn544_id,
	.driver		= {
		//.owner	= THIS_MODULE,
		.name	= PN544_DRVNAME,
	},
};

/*
 * module load/unload record keeping
 */
/* static int pn544_platform_probe(struct platform_device *pdev) 
{
	int ret ;
	printk("add pn544_driver /n ");
	 ret = i2c_add_driver(&pn544_driver);
		if(ret !=0)
			{
				printk("add pn544_driver fail /n ");
				return -1;
			}
		
	 return 0;
}*/
/* static int pn544_platform_remove(struct platform_device *pdev) 
{
	 return 0;
}*/

/*static struct platform_driver pn544_platform_driver = {
	.probe      = pn544_platform_probe,
	.remove     = pn544_platform_remove,    
	.driver     = 
	{
		.name  = "pn547",
//		.owner = THIS_MODULE,
	}
};*/
static int __init pn544_dev_init(void)
{
	printk("pn544 Loading pn544 driver\n");
	
	i2c_register_board_info(PN544_I2C_GROUP_ID, &pn544_i2c_nfc, 1);
	 //platform_driver_register(&pn544_platform_driver);
	 return i2c_add_driver(&pn544_driver);

}
module_init(pn544_dev_init);

static void __exit pn544_dev_exit(void)
{
	printk("pn544 Unloading pn544 driver\n");
	i2c_del_driver(&pn544_driver);
}
module_exit(pn544_dev_exit);

MODULE_AUTHOR("XXX");
MODULE_DESCRIPTION("NFC PN544 driver");
MODULE_LICENSE("GPL");
