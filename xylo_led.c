/*
 * Linux USB driver for Xylo
 *
 *   Copyright (C) 2012 CODORO TEAM
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/uaccess.h>

MODULE_AUTHOR("Codoro TEAM {COTARD, ZACCARIA, ROLLAND}");
MODULE_DESCRIPTION("Xylo USB driver");
MODULE_LICENSE("GPL");

#define BUF_SIZE    16

#define VENDOR_ID   0x04b4
#define PRODUCT_ID  0x8613

struct usb_xylo_led {
  struct usb_device *udev;
  int ledmask; // we can send up to 32 bits
};


// Forward declaration
static struct usb_driver xylo_led_driver;

/* Table of devices that work with this driver */
static struct usb_device_id id_table [] = {
  { USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
  {}
};
MODULE_DEVICE_TABLE (usb, id_table);


// Send Bulk msg to set led mask
int xylo_set_ledmask (struct usb_xylo_led *mydev, const char *buf)
{
  int ret = 0, l, n;
  char c;

  // Remove \n if any
  l = strlen(buf);
  l = (*(buf+l-1) == '\n' ? l-1 : l);

  // Get value from buf
  if (*buf == '\'') {
    // 'a => 0x61
    sscanf (buf+1, "%c", &c);
    mydev->ledmask = c;
    l--;
  }
  else
    sscanf (buf, "%x", &(mydev->ledmask));

  // Handle x instead of 0x
  n = (l >= 2 ? l/2 : l);


  //modulo 4 to handle bigger numbers
  mydev->ledmask %= 4;

  // Send bulk message
  if ((ret = usb_bulk_msg (mydev->udev, usb_sndbulkpipe(mydev->udev, 2), &(mydev->ledmask), 1, &l, 1000)))
    printk (KERN_WARNING "xylo_set_ledmask: usb_bulk_msg() error %d\n", ret);
  else
    printk (KERN_WARNING "xylo_set_ledmask: usb_bulk_msg() sent[%d], return[%d]\n", mydev->ledmask, ret);

  return ret;
}

static void xylo_trame_init(struct usb_device *udev)
{
  int ret = 0;
  char buf[11][16] = {{0x75, 0x81, 0x5f, 0x90, 0xe6, 0x00, 0x74, 0x0a, 0xf0, 0x90, 0xe6, 0x7a, 0x74, 0x01, 0xf0, 0x11},
                      {0x9b, 0x90, 0xe6, 0x18, 0x74, 0x10, 0xf0, 0x11, 0x9b, 0x90, 0xe6, 0x19, 0x74, 0x10, 0xf0, 0x11},
                      {0x9b, 0x90, 0xe6, 0x1a, 0x74, 0x0c, 0xf0, 0x11, 0x9b, 0x90, 0xe6, 0x1b, 0x74, 0x0c, 0xf0, 0x11},
                      {0x9b, 0x90, 0xe6, 0x02, 0x74, 0x98, 0xf0, 0x11, 0x9b, 0x90, 0xe6, 0x03, 0x74, 0xfe, 0xf0, 0x90},
                      {0xe6, 0x70, 0x74, 0x80, 0xf0, 0x11, 0x9b, 0x90, 0xe6, 0x01, 0x74, 0x03, 0xf0, 0x90, 0xe6, 0x8d},
                      {0xf0, 0xe5, 0xba, 0x20, 0xe1, 0xfb, 0x90, 0xe6, 0x8d, 0xe0, 0x60, 0x25, 0x90, 0xe7, 0x80, 0xb4},
                      {0x04, 0x27, 0xe0, 0xf5, 0xb2, 0xa3, 0xe0, 0xf5, 0xb5, 0xa3, 0xe0, 0xf5, 0xb0, 0xa3, 0xe0, 0x90},
                      {0xe6, 0x09, 0xf0, 0x90, 0xe7, 0xc0, 0xe5, 0xb0, 0xf0, 0x90, 0xe6, 0x8f, 0x74, 0x01, 0xf0, 0x80},
                      {0xcc, 0x90, 0xe7, 0xc0, 0xe5, 0xaa, 0xf0, 0x80, 0xf0, 0xff, 0xe0, 0xa3, 0x7e, 0x08, 0x13, 0x92},
                      {0x80, 0xc2, 0x81, 0xd2, 0x81, 0xde, 0xf7, 0xdf, 0xf1, 0x80, 0xb2, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22}};
  char init = 0x01;
  char end = 0x00;

  if ((ret = usb_control_msg(udev, usb_sndctrlpipe(udev,0), 0xa0,
                             0x40, 0xe600, 0x0000, &init, 0x0001, 1000)))
    printk("xylo_trame_init:  %i\n", ret);

  if ((ret = usb_control_msg(udev, usb_sndctrlpipe(udev,0), 0xa0,
                             0x40, 0x0000, 0x0000, buf[0], 0x0010, 1000)))
    printk("xylo_trame_init:  %i\n", ret);

  if ((ret = usb_control_msg(udev, usb_sndctrlpipe(udev,0), 0xa0,
                             0x40, 0x0010, 0x0000, buf[1], 0x0010, 1000)))
    printk("xylo_trame_init:  %i\n", ret);

  if ((ret = usb_control_msg(udev, usb_sndctrlpipe(udev,0), 0xa0,
                             0x40, 0x0020, 0x0000, buf[2], 0x0010, 1000)))
    printk("xylo_trame_init:  %i\n", ret);

  if ((ret = usb_control_msg(udev, usb_sndctrlpipe(udev,0), 0xa0,
                             0x40, 0x0030, 0x0000, buf[3], 0x0010, 1000)))
    printk("xylo_trame_init:  %i\n", ret);

  if ((ret = usb_control_msg(udev, usb_sndctrlpipe(udev,0), 0xa0,
                             0x40, 0x0040, 0x0000, buf[4], 0x0010, 1000)))
    printk("xylo_trame_init:  %i\n", ret);

  if ((ret = usb_control_msg(udev, usb_sndctrlpipe(udev,0), 0xa0,
                             0x40, 0x0050, 0x0000, buf[5], 0x0010, 1000)))
    printk("xylo_trame_init:  %i\n", ret);

  if ((ret = usb_control_msg(udev, usb_sndctrlpipe(udev,0), 0xa0,
                             0x40, 0x0060, 0x0000, buf[6], 0x0010, 1000)))
    printk("xylo_trame_init:  %i\n", ret);

  if ((ret = usb_control_msg(udev, usb_sndctrlpipe(udev,0), 0xa0,
                             0x40, 0x0070, 0x0000, buf[7], 0x0010, 1000)))
    printk("xylo_trame_init:  %i\n", ret);

  if ((ret = usb_control_msg(udev, usb_sndctrlpipe(udev,0), 0xa0,
                             0x40, 0x0080, 0x0000, buf[8], 0x0010, 1000)))
    printk("xylo_trame_init:  %i\n", ret);

  if ((ret = usb_control_msg(udev, usb_sndctrlpipe(udev,0), 0xa0,
                             0x40, 0x0090, 0x0000, buf[9], 0x0010, 1000)))
    printk("xylo_trame_init:  %i\n", ret);

  if ((ret = usb_control_msg(udev, usb_sndctrlpipe(udev,0), 0xa0,
                             0x40, 0x00a0, 0x0000, buf[10], 0x000b, 1000)))
    printk("xylo_trame_init:  %i\n", ret);

  if ((ret = usb_control_msg(udev, usb_sndctrlpipe(udev,0), 0xa0,
                             0x40, 0xe600, 0x0000, &end, 0x0001, 1000)))
    printk("xylo_trame_init:  %i\n", ret);
}

//
// char device functions
//
static int xylo_led_open (struct inode *inode, struct file *file)
{
  struct usb_xylo_led *dev;
  struct usb_interface *interface;
  int minor;

  minor = iminor(inode);

  // Get interface for device
  interface = usb_find_interface (&xylo_led_driver, minor);
  if (!interface)
    return -ENODEV;

  // Get private data from interface
  dev = usb_get_intfdata (interface);
  if (dev == NULL) {
      printk (KERN_WARNING "xylo_led_open(): can't find device for minor %d\n", minor);
      return -ENODEV;
  }

  // Set to file structure
  file->private_data = (void *)dev;

  return 0;
}

static int xylo_led_release (struct inode *inode, struct file *file)
{
  return 0;
}

static ssize_t xylo_led_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
  struct usb_xylo_led *mydev;
  char tmp[BUF_SIZE];
  int real, ret;

  memset ((void*)tmp, 0, sizeof(tmp));

  /* get the dev object */
  mydev = file->private_data;
  if (mydev == NULL)
    return -ENODEV;

  real = min((size_t)BUF_SIZE, count);

  if (real)
    if (copy_from_user(tmp, buf, real))
      return -EFAULT;

  // Now set the ledmask value
  if ((ret = xylo_set_ledmask (mydev, tmp)) < 0)
    return ret;
  else
    return real;
}

static struct file_operations xylo_led_file_operations = {
  .open = xylo_led_open,
  .release = xylo_led_release,
  .write = xylo_led_write
};

static struct usb_class_driver xylo_led_class_driver = {
  .name = "usb/xylo_led%d",
  .fops = &xylo_led_file_operations,
  .minor_base = 0
};


/*
** This function will be called when sys entry is read.
*/
static ssize_t
show_ledmask (struct device *dev, struct device_attribute *attr,
	    char *buf)
{
  struct usb_interface *interface;
  struct usb_xylo_led *mydev;

  interface = to_usb_interface (dev);
  mydev = usb_get_intfdata (interface);

  return sprintf (buf, "0x%x\n",  mydev->ledmask);
}


/*
** This function will be called when sys entry is written.
*/
static ssize_t
set_ledmask (struct device *dev, struct device_attribute *attr,
	   const char *buf, size_t count)
{
  struct usb_interface *interface;
  struct usb_xylo_led *mydev;
  int ret;

  interface = to_usb_interface (dev);
  mydev = usb_get_intfdata (interface);

  // Now set the ledmask value
  if ((ret = xylo_set_ledmask (mydev, buf)) < 0)
    return ret;
  else
    return count;
}

// This macro generates a struct device_attribute
static DEVICE_ATTR (ledmask, S_IRUGO | S_IWUGO, show_ledmask, set_ledmask);


static int xylo_led_probe (struct usb_interface *interface, const struct usb_device_id *id)
{
  struct usb_device *udev = interface_to_usbdev (interface);
  struct usb_xylo_led *xylo_led_dev;
  int ret;

  ret = usb_register_dev(interface, &xylo_led_class_driver);
  if (ret < 0) {
    printk (KERN_WARNING "xylo_led_probe: usb_register_dev() error\n");
    return ret;
  }

  xylo_led_dev = kmalloc (sizeof(struct usb_xylo_led), GFP_KERNEL);
  if (xylo_led_dev == NULL) {
    dev_err (&interface->dev, "xylo_led_probe: Out of memory\n");
    return -ENOMEM;
  }

  // Fill private structure and save it
  memset (xylo_led_dev, 0, sizeof (*xylo_led_dev));
  xylo_led_dev->udev = usb_get_dev(udev);
  xylo_led_dev->ledmask = 0;
  usb_set_intfdata (interface, xylo_led_dev);

  // Create /sys entry
  ret = device_create_file (&interface->dev, &dev_attr_ledmask);
  if (ret < 0) {
    printk (KERN_WARNING "xylo_led_probe: device_create_file() error\n");
    return ret;
  }

  // Set interface to alternate 1
  ret = usb_set_interface  (udev, 0, 1);
  if (ret < 0) {
    printk (KERN_WARNING "xylo_led_probe: usb_set_interface() error\n");
    return ret;
  }

  xylo_trame_init(xylo_led_dev->udev);

  //dev_info(&interface->dev, "xylo now attached\n");

  return 0;
}


static void xylo_led_disconnect(struct usb_interface *interface)
{
  struct usb_xylo_led *dev;

  dev = usb_get_intfdata(interface);
  usb_deregister_dev(interface, &xylo_led_class_driver);
  device_remove_file (&interface->dev, &dev_attr_ledmask);

  usb_set_intfdata(interface, NULL);
  kfree(dev);

  dev_info(&interface->dev, "Xylo now disconnected\n");
}

static struct usb_driver xylo_led_driver = {
  .name = "xylo_led",
  .probe = xylo_led_probe,
  .disconnect = xylo_led_disconnect,
  .id_table = id_table,
};

static int __init usb_xylo_led_init(void)
{
  int retval = 0;

  printk(KERN_DEBUG "xylo_led: usb_xylo_led_init()\n");

  retval = usb_register(&xylo_led_driver);
  if (retval) {
    err("usb_register failed. Error number %d", retval);
    return retval;
  }

  printk(KERN_DEBUG "xylo_led: device successfully registered\n");

  return retval;
}

static void __exit usb_xylo_led_exit(void)
{
  printk(KERN_DEBUG "xylo_led: usb_xylo_led_exit()\n");

  usb_deregister(&xylo_led_driver);
}

module_init (usb_xylo_led_init);
module_exit (usb_xylo_led_exit);
