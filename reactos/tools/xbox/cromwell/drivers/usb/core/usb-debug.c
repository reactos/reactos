/*
 * debug.c - USB debug helper routines.
 *
 * I just want these out of the way where they aren't in your
 * face, but so that you can still use them..
 */
#define CONFIG_USB_DEBUG
#if 0
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>
#ifdef CONFIG_USB_DEBUG
	#define DEBUG
#else
	#undef DEBUG
#endif
#include <linux/usb.h>
#else
#include "../usb_wrapper.h"
#endif

static void usb_show_endpoint(struct usb_host_endpoint *endpoint)
{
	usb_show_endpoint_descriptor(&endpoint->desc);
}

static void usb_show_interface(struct usb_host_interface *altsetting)
{
	int i;

	usb_show_interface_descriptor(&altsetting->desc);

	for (i = 0; i < altsetting->desc.bNumEndpoints; i++)
		usb_show_endpoint(altsetting->endpoint + i);
}

static void usb_show_config(struct usb_host_config *config)
{
	int i, j;
	struct usb_interface *ifp;

	usb_show_config_descriptor(&config->desc);
	for (i = 0; i < config->desc.bNumInterfaces; i++) {
		ifp = config->interface + i;

		if (!ifp)
			break;

		printk("\n  Interface: %d\n", i);
		for (j = 0; j < ifp->num_altsetting; j++)
			usb_show_interface(ifp->altsetting + j);
	}
}

void usb_show_device(struct usb_device *dev)
{
	int i;

	usb_show_device_descriptor(&dev->descriptor);
	for (i = 0; i < dev->descriptor.bNumConfigurations; i++)
		usb_show_config(dev->config + i);
}

/*
 * Parse and show the different USB descriptors.
 */
void usb_show_device_descriptor(struct usb_device_descriptor *desc)
{
	if (!desc)
	{
		printk("Invalid USB device descriptor (NULL POINTER)\n");
		return;
	}
	printk("  Length              = %2d%s\n", desc->bLength,
		desc->bLength == USB_DT_DEVICE_SIZE ? "" : " (!!!)");
	printk("  DescriptorType      = %02x\n", desc->bDescriptorType);

	printk("  USB version         = %x.%02x\n",
		desc->bcdUSB >> 8, desc->bcdUSB & 0xff);
	printk("  Vendor:Product      = %04x:%04x\n",
		desc->idVendor, desc->idProduct);
	printk("  MaxPacketSize0      = %d\n", desc->bMaxPacketSize0);
	printk("  NumConfigurations   = %d\n", desc->bNumConfigurations);
	printk("  Device version      = %x.%02x\n",
		desc->bcdDevice >> 8, desc->bcdDevice & 0xff);

	printk("  Device Class:SubClass:Protocol = %02x:%02x:%02x\n",
		desc->bDeviceClass, desc->bDeviceSubClass, desc->bDeviceProtocol);
	switch (desc->bDeviceClass) {
	case 0:
		printk("    Per-interface classes\n");
		break;
	case USB_CLASS_AUDIO:
		printk("    Audio device class\n");
		break;
	case USB_CLASS_COMM:
		printk("    Communications class\n");
		break;
	case USB_CLASS_HID:
		printk("    Human Interface Devices class\n");
		break;
	case USB_CLASS_PRINTER:
		printk("    Printer device class\n");
		break;
	case USB_CLASS_MASS_STORAGE:
		printk("    Mass Storage device class\n");
		break;
	case USB_CLASS_HUB:
		printk("    Hub device class\n");
		break;
	case USB_CLASS_VENDOR_SPEC:
		printk("    Vendor class\n");
		break;
	default:
		printk("    Unknown class\n");
	}
}

void usb_show_config_descriptor(struct usb_config_descriptor *desc)
{
	printk("Configuration:\n");
	printk("  bLength             = %4d%s\n", desc->bLength,
		desc->bLength == USB_DT_CONFIG_SIZE ? "" : " (!!!)");
	printk("  bDescriptorType     =   %02x\n", desc->bDescriptorType);
	printk("  wTotalLength        = %04x\n", desc->wTotalLength);
	printk("  bNumInterfaces      =   %02x\n", desc->bNumInterfaces);
	printk("  bConfigurationValue =   %02x\n", desc->bConfigurationValue);
	printk("  iConfiguration      =   %02x\n", desc->iConfiguration);
	printk("  bmAttributes        =   %02x\n", desc->bmAttributes);
	printk("  bMaxPower            = %4dmA\n", desc->bMaxPower * 2);
}

void usb_show_interface_descriptor(struct usb_interface_descriptor *desc)
{
	printk("  Alternate Setting: %2d\n", desc->bAlternateSetting);
	printk("    bLength             = %4d%s\n", desc->bLength,
		desc->bLength == USB_DT_INTERFACE_SIZE ? "" : " (!!!)");
	printk("    bDescriptorType     =   %02x\n", desc->bDescriptorType);
	printk("    bInterfaceNumber    =   %02x\n", desc->bInterfaceNumber);
	printk("    bAlternateSetting   =   %02x\n", desc->bAlternateSetting);
	printk("    bNumEndpoints       =   %02x\n", desc->bNumEndpoints);
	printk("    bInterface Class:SubClass:Protocol =   %02x:%02x:%02x\n",
		desc->bInterfaceClass, desc->bInterfaceSubClass, desc->bInterfaceProtocol);
	printk("    iInterface          =   %02x\n", desc->iInterface);
}

void usb_show_endpoint_descriptor(struct usb_endpoint_descriptor *desc)
{
	char *LengthCommentString = (desc->bLength ==
		USB_DT_ENDPOINT_AUDIO_SIZE) ? " (Audio)" : (desc->bLength ==
		USB_DT_ENDPOINT_SIZE) ? "" : " (!!!)";
	char *EndpointType[4] = { "Control", "Isochronous", "Bulk", "Interrupt" };

	printk("    Endpoint:\n");
	printk("      bLength             = %4d%s\n",
		desc->bLength, LengthCommentString);
	printk("      bDescriptorType     =   %02x\n", desc->bDescriptorType);
	printk("      bEndpointAddress    =   %02x (%s)\n", desc->bEndpointAddress,
		(desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
			USB_ENDPOINT_XFER_CONTROL ? "i/o" :
		(desc->bEndpointAddress & USB_ENDPOINT_DIR_MASK) ? "in" : "out");
	printk("      bmAttributes        =   %02x (%s)\n", desc->bmAttributes,
		EndpointType[USB_ENDPOINT_XFERTYPE_MASK & desc->bmAttributes]);
	printk("      wMaxPacketSize      = %04x\n", desc->wMaxPacketSize);
	printk("      bInterval           =   %02x\n", desc->bInterval);

	/* Audio extensions to the endpoint descriptor */
	if (desc->bLength == USB_DT_ENDPOINT_AUDIO_SIZE) {
		printk("      bRefresh            =   %02x\n", desc->bRefresh);
		printk("      bSynchAddress       =   %02x\n", desc->bSynchAddress);
	}
}

void usb_show_string(struct usb_device *dev, char *id, int index)
{
	char *buf;

	if (!index)
		return;
	if (!(buf = kmalloc(256, GFP_KERNEL)))
		return;
	if (usb_string(dev, index, buf, 256) > 0)
		dev_printk(KERN_INFO, &dev->dev, "%s: %s\n", id, buf);
	kfree(buf);
}

void usb_dump_urb (struct urb *urb)
{
	printk ("urb                   :%p\n", urb);
	printk ("dev                   :%p\n", urb->dev);
	printk ("pipe                  :%08X\n", urb->pipe);
	printk ("status                :%d\n", urb->status);
	printk ("transfer_flags        :%08X\n", urb->transfer_flags);
	printk ("transfer_buffer       :%p\n", urb->transfer_buffer);
	printk ("transfer_buffer_length:%d\n", urb->transfer_buffer_length);
	printk ("actual_length         :%d\n", urb->actual_length);
	printk ("setup_packet          :%p\n", urb->setup_packet);
	printk ("start_frame           :%d\n", urb->start_frame);
	printk ("number_of_packets     :%d\n", urb->number_of_packets);
	printk ("interval              :%d\n", urb->interval);
	printk ("error_count           :%d\n", urb->error_count);
	printk ("context               :%p\n", urb->context);
	printk ("complete              :%p\n", urb->complete);
}

