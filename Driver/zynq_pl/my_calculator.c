/*
 * my_calculator_driver.c
 * Copyright (C) 2019 Yuhei Horibe
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This is a device driver for my device on Zedboard
 *
 */

#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/slab.h>
//#include <linux/regulator/consumer.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/uaccess.h>


// ******************************************
// Address and register definitions
// ******************************************
// DWORD offset
#define MY_CAL_REG_A_OFFSET  0x0
#define MY_CAL_REG_B_OFFSET  0x4
#define MY_CAL_REG_C_OFFSET  0x8


// ******************************************
// i2c device definitions
// ******************************************
// Device name
#define MY_CALC_NAME "my_calculator"

// Device class and version numbers
//static int                my_calculator_major;  // Major number
//static struct class*      my_calculator_class = NULL;
static struct device*     my_calculator_dev  = NULL;

// My device on Zynq Zedboard
struct my_calculator_data
{
    //struct mutex  update_lock;
    int           my_calculator_major;
    void __iomem* addr_base;
    unsigned long size;
};

// Mutex for this device (file)
static DEFINE_MUTEX(my_calculator_mutex);


// ******************************************
// Define file operations
// ******************************************
static int my_calculator_open(struct inode* inode, struct file* file)
{
    // Check if the other process is using it or not
    if (!mutex_trylock(&my_calculator_mutex)) {
        printk("%s: Device currently in use!\n", __FUNCTION__);
        return -EBUSY;
    }
    printk("%s: Device opened\n", __FUNCTION__);

    return 0;
}

// Device release
static int my_calculator_release(struct inode* inode, struct file* file)
{
    printk("%s: Device closed\n", __FUNCTION__);
    mutex_unlock(&my_calculator_mutex);
    return 0;
}

// Read funcion
static ssize_t my_calculator_read(struct file *fp, char __user* buf, size_t count, loff_t* offset)
{
    struct my_calculator_data* data   = dev_get_drvdata(fp->private_data);
    const uint32_t               result = *(int*)(data->addr_base + MY_CAL_REG_C_OFFSET);
    return sprintf(buf, "%d", result);

    return 0;
}

// Write function
static ssize_t my_calculator_write(struct file* fp, const char* __user buff, size_t count, loff_t* offset)
{
    return 0;
}

// File operations
static const struct file_operations my_calculator_fops  = {
    .owner   = THIS_MODULE,
    .llseek  = no_llseek,
    .write   = my_calculator_write,
    .read    = my_calculator_read,
    .open    = my_calculator_open,
    .release = my_calculator_release
};

// ******************************************
// sysfs show/store
// ******************************************
static ssize_t my_calculator_oprd_a_show(struct device* dev, struct device_attribute* attr, char* buf)
{
    struct my_calculator_data* data       = dev_get_drvdata(dev);
    const unsigned long        oprd_a_val = *(int*)(data->addr_base + MY_CAL_REG_A_OFFSET);
    char                       out_buff[sizeof(unsigned long) + 1];

    printk("%s: Reading out register A: %lx", MY_CALC_NAME, oprd_a_val);
    sprintf(buf, "%04lx", oprd_a_val);
    out_buff[sizeof(unsigned long)] = '\0';
    if(copy_to_user(buf, out_buff, sizeof(unsigned long) + 1) != 0)
    {
        printk("%s: Failed to copy %s to user buffer", MY_CALC_NAME, out_buff);
        return -EFAULT;
    }
    return sizeof(unsigned long);
}

static ssize_t my_calculator_oprd_b_show(struct device* dev, struct device_attribute* attr, char* buf)
{
    struct my_calculator_data* data       = dev_get_drvdata(dev);
    const unsigned long        oprd_b_val = *(int*)(data->addr_base + MY_CAL_REG_B_OFFSET);
    char                       out_buff[sizeof(unsigned long) + 1];

    printk("%s: Reading out register B: %lx", MY_CALC_NAME, oprd_b_val);
    sprintf(out_buff, "%04lx", oprd_b_val);
    out_buff[sizeof(unsigned long)] = '\0';
    printk("%s: copying: %s to user buffer", MY_CALC_NAME, out_buff);
    if(copy_to_user(buf, out_buff, sizeof(unsigned long) + 1) != 0)
    {
        printk("%s: Failed to copy %s to user buffer", MY_CALC_NAME, out_buff);
        return -EFAULT;
    }
    return sizeof(unsigned long);
}

static ssize_t my_calculator_output_show(struct device* dev, struct device_attribute* attr, char* buf)
{
    struct my_calculator_data* data       = dev_get_drvdata(dev);
    const unsigned long        output_val = *(int*)(data->addr_base + MY_CAL_REG_C_OFFSET);
    //char                       out_buff[sizeof(unsigned long) + 1];

    printk("%s: Reading out output register: %lx", MY_CALC_NAME, output_val);
    sprintf(buf, "%04lx", output_val);
    buf[sizeof(unsigned long)] = '\0';
    printk("%s: Copying %s to user buffer", MY_CALC_NAME, buf);
    return sizeof(unsigned long) + 1;
    //out_buff[sizeof(unsigned long)] = '\0';
    //if(copy_to_user(buf, out_buff, sizeof(unsigned long) + 1) != 0)
    //{
    //    printk("%s: Failed to copy %s to user buffer", MY_CALC_NAME, out_buff);
    //    return -EFAULT;
    //}
}

static ssize_t my_calculator_oprd_a_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
    struct my_calculator_data* data = dev_get_drvdata(dev);
    long unsigned int          oprd_a_val;
    if(kstrtoul(buf, 10, &oprd_a_val) != 0){
        return -1;
    }

    if(oprd_a_val > 0xF){
        return -1;
    }
    printk("%s: Writing to operand register A: %lx", MY_CALC_NAME, oprd_a_val);
    *(int*)(data->addr_base + MY_CAL_REG_A_OFFSET) = oprd_a_val;

    return count;
}

static ssize_t my_calculator_oprd_b_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
    struct my_calculator_data* data = dev_get_drvdata(dev);
    long unsigned int          oprd_b_val;
    if(kstrtoul(buf, 10, &oprd_b_val) != 0){
        return -1;
    }

    if(oprd_b_val > 0xF){
        return -1;
    }
    printk("%s: Writing to operand register B: %lx", MY_CALC_NAME, oprd_b_val);
    *(int*)(data->addr_base + MY_CAL_REG_B_OFFSET) = oprd_b_val;

    return count;
}

// sysfs attributes
static const struct device_attribute my_calculator_attr_oprd_a =
{
    .attr  = { .name = "oprd_a", .mode = S_IWUGO | S_IRUGO },
    .show  = my_calculator_oprd_a_show,
    .store = my_calculator_oprd_a_store
};

static const struct device_attribute my_calculator_attr_oprd_b =
{
    .attr  = { .name = "oprd_b", .mode = S_IWUGO | S_IRUGO },
    .show  = my_calculator_oprd_b_show,
    .store = my_calculator_oprd_b_store
};

static const struct device_attribute my_calculator_attr_output =
{
    .attr  = { .name = "output", .mode = S_IRUGO },
    .show  = my_calculator_output_show,
    .store = NULL
};

// ******************************************
// Probe and Remove
// ******************************************
static int my_calculator_probe(struct platform_device *pdev)
{
    int                          retval;
    //struct my_device_platform_data *my_device_pdata;
    struct my_calculator_data* data = NULL;
    struct resource*           res  = &pdev->resource[0];

    // Debug
    printk("my_calculator: %s\n", __FUNCTION__);

    // Allocate memory for device data
    data = devm_kzalloc(&pdev->dev, sizeof(struct my_calculator_data), GFP_KERNEL);

    if(data == NULL){
        printk("%s: Failed to allocate memory for device data.", MY_CALC_NAME);
        return -ENOMEM;
    }

    // Get base address
    if(res->start <= 0){
        printk("%s: Address from device tree is not correct. Set default address.", MY_CALC_NAME);
        data->size      = 0x1000;
        data->addr_base = (void __iomem*)ioremap(0x43c00000, data->size);
    }
    else{
        printk("%s: Register base address is loaded from device tree ... %lx", MY_CALC_NAME, (unsigned long)res->start);
        data->size      = (unsigned long)(res->end - res->start);
        data->addr_base = (void __iomem*)ioremap(res->start, data->size);
    }

    // Set driver data
    platform_set_drvdata(pdev, data);

    // Initialize the mutex
    //mutex_init(&data->update_lock);

    my_calculator_dev = &pdev->dev;
    // Register device
    //my_calculator_major   = register_chrdev(0, MY_CALC_NAME, &my_calculator_fops);
    //if(my_calculator_major < 0){
    //    printk("%s: Failed to register character device.\n", MY_CALC_NAME);
    //    return 0;
    //}

    // Create class
    //my_calculator_class   = class_create(THIS_MODULE, MY_CALC_NAME);
    //if(IS_ERR(my_calculator_class)){
    //    retval  = PTR_ERR(my_calculator_dev);
    //}

    // Create device
    //my_calculator_dev = device_create(my_calculator_class,
    //                                   NULL,
    //                                   MKDEV(my_calculator_major, 0),
    //                                   NULL,
    //                                   MY_CALC_NAME); 
    //if(IS_ERR(my_calculator_dev)) {
    //    retval = PTR_ERR(my_calculator_dev);
    //    printk("%s: Failed to create device\n", __FUNCTION__);
    //    goto unreg_class;
    //}

    // Create sysfs
    if(device_create_file(&pdev->dev, &my_calculator_attr_oprd_a)){
        goto unreg_class;
    }

    if(device_create_file(&pdev->dev, &my_calculator_attr_oprd_b)){
        goto unreg_class;
    }

    if(device_create_file(&pdev->dev, &my_calculator_attr_output)){
        goto unreg_class;
    }

    // Initialize the mutex for /dev fops clients
    mutex_init(&my_calculator_mutex);

    return 0;

// Cleanup for failed operation
unreg_class:
    //class_unregister(my_calculator_class);
    //class_destroy(my_calculator_class);
    //unregister_chrdev(my_calculator_major, MY_CALC_NAME);
    printk("%s: Driver initialization failed\n", __FUNCTION__);
    kfree(data);
    device_remove_file(&pdev->dev, &my_calculator_attr_oprd_a);
    device_remove_file(&pdev->dev, &my_calculator_attr_oprd_b);
    device_remove_file(&pdev->dev, &my_calculator_attr_output);
//out:
    return retval;
}

static int my_calculator_remove(struct platform_device* pdev)
{
    //struct my_device_platform_data *my_device_pdata = dev_get_platdata(&pdev->dev);;
    struct my_calculator_data *data = platform_get_drvdata(pdev);

    printk("my_calculator: %s\n", __FUNCTION__);

    // Remove sysfs
    device_remove_file(&pdev->dev, &my_calculator_attr_oprd_a);
    device_remove_file(&pdev->dev, &my_calculator_attr_oprd_b);
    device_remove_file(&pdev->dev, &my_calculator_attr_output);
 
    // Destroy device
    //device_destroy(my_calculator_class, MKDEV(my_calculator_major, 0));

    // Release class
    //class_unregister(my_calculator_class);
    //class_destroy(my_calculator_class);

    // Release platform device data
    kfree(data);

    // Unregistering device
    //unregister_chrdev(my_calculator_major, MY_CALC_NAME);
 
    return 0;
}

// ******************************************
// Power management
// ******************************************
// Suspend the device.
static int my_calculator_pm_suspend(struct device *dev)
{
    //struct my_calculator_data* data = dev_get_drvdata(dev);

    printk("%s: PM SUSPEND\n", __FUNCTION__);

    // Suspend the device here

    return 0;
}

// Resume the device.
static int my_calculator_pm_resume(struct device *dev)
{
    //struct my_calculator_data* data = dev_get_drvdata(dev);

    printk("%s: PM RESUME\n", __FUNCTION__);

    // Resume the device here

    return 0;
}

static const struct dev_pm_ops my_calculator_pm_ops = 
{
    .suspend = my_calculator_pm_suspend,
    .resume  = my_calculator_pm_resume,
};


// ******************************************
// Platform device driver
// ******************************************
// Device match table
static const struct of_device_id my_calculator_of_ids[] = 
{
    { .compatible = "xlnx,my_calculator" },
    { }
};
MODULE_DEVICE_TABLE(of, my_calculator_of_ids);

// Platform device driver
static struct platform_driver my_calculator_driver = {
    .probe              = my_calculator_probe,
    .remove             = my_calculator_remove,
    .driver             = {
        .name	        = "my_calculator_drv",
        .of_match_table = my_calculator_of_ids,
        .owner	        = THIS_MODULE,
        .pm	            = &my_calculator_pm_ops,
    },
};

// ******************************************
// init and exit
// ******************************************
static int __init my_calculator_init_test(void)
{
    printk("%s: init\n", MY_CALC_NAME);
 
    // Probe this device in init
	//ret = platform_driver_register(&my_calculator_driver);
    return  platform_driver_probe(&my_calculator_driver, my_calculator_probe);
}
module_init(my_calculator_init_test);

static void __exit my_calculator_cleanup(void)
{
    printk("%s: exit.\n", MY_CALC_NAME);
    platform_driver_unregister(&my_calculator_driver);
}
module_exit(my_calculator_cleanup);


// ******************************************
// License
// ******************************************
MODULE_AUTHOR("Yuhei Horibe <yuhei1.horibe@gmail.com>");
MODULE_DESCRIPTION("Driver for my device on Zedboard");
MODULE_LICENSE("GPL v2");

