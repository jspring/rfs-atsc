/**\file
 *	Read USB digio. 
 *
 * (C) Copyright University of California 2006.  All rights reserved.
 *
 */

#include	<sys_os.h>
#include	"usb.h"

//This code is written to work with National Instruments USB-6501
//libusb 0.1.12 needs to be installed for this code to work

#define VENDOR_ID 0x3923
#define PRODUCT_ID 0x718a

#define USB_BUFFER_SIZE  500
static unsigned char buf[USB_BUFFER_SIZE];

// Buffers to hold USB data 
static unsigned char rcv_data[100];
static unsigned char snd_data[8];

struct usb_bus *usb_bus;
struct usb_device *dev;
struct usb_dev_handle *handle;	// later better style to have init return this

void find_digio()
{
	for(usb_bus = usb_busses; usb_bus; usb_bus = usb_bus->next)
	 for(dev = usb_bus->devices; dev; dev = dev->next)
	  if (VENDOR_ID == dev->descriptor.idVendor && PRODUCT_ID == dev->descriptor.idProduct) {
	   printf("found device\n");
	   return;    
	}
}

void init_usb_digio()
{
	int langid;
	int ret;
	int temp;

	//This data must be sent to the device before every read
	snd_data[0] = 0x00;
	snd_data[1] = 0x07;
	snd_data[2] = 0x14;
	snd_data[3] = 0x07;
	snd_data[4] = 0x00;
	snd_data[5] = 0x03;
	snd_data[6] = 0x02;  //this is the port that we want to read

	usb_init();
	usb_find_busses();
	usb_find_devices();
	find_digio();

	handle = usb_open(dev);	// setting global variable

	ret =  usb_get_string(handle, 0, 0, buf, USB_BUFFER_SIZE);
	if (ret > 0) langid = buf[2] | buf[3] >> 8;
	printf("langid = %d\n", langid);
	ret = usb_get_string(handle, 0, langid, buf, USB_BUFFER_SIZE);
//	printf("ret = %d\n", ret);
	ret = usb_set_configuration(handle, 0x01);
	printf("configuration set: ret = %df\n", ret);
}

// later better style to use "handle" as parameter
unsigned char read_digio_byte()
{
	int ret;
	ret = usb_bulk_write(handle, 0x01, snd_data, 7, USB_BUFFER_SIZE);
	ret = usb_bulk_read(handle, 0x81, rcv_data, 7, USB_BUFFER_SIZE);
	//temp = (int) rcv_data;
	fflush(stdout);
//	printf("read: status 0x%02hhx\n", rcv_data[6]);
	return (rcv_data[6]);
}
