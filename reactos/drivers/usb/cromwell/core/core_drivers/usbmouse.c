/*
 This driver is based on Linux 2.5.75 usbmouse driver by Vojtech Pavlik
*/

#define NDEBUG
#include "../../usb_wrapper.h"

extern USBPORT_INTERFACE UsbPortInterface;

struct usb_mouse {
	char name[128];
	char phys[64];
	struct usb_device *usbdev;
	char btn_old;
	//struct input_dev dev;
	struct urb *irq;
	int open;

	signed char *data;
	dma_addr_t data_dma;
};

static void usb_mouse_irq(struct urb *urb, struct pt_regs *regs)
{
	struct usb_mouse *mouse = urb->context;
	signed char *data = mouse->data;
	int status;

	switch (urb->status) {
	case 0:			/* success */
		break;
	case -ECONNRESET:	/* unlink */
	case -ENOENT:
	case -ESHUTDOWN:
		return;
	/* -EPIPE:  should clear the halt */
	default:		/* error */
		goto resubmit;
	}
/*
	input_regs(dev, regs);

	input_report_key(dev, BTN_LEFT,   data[0] & 0x01);
	input_report_key(dev, BTN_RIGHT,  data[0] & 0x02);
	input_report_key(dev, BTN_MIDDLE, data[0] & 0x04);
	input_report_key(dev, BTN_SIDE,   data[0] & 0x08);
	input_report_key(dev, BTN_EXTRA,  data[0] & 0x10);

	input_report_rel(dev, REL_X,     data[1]);
	input_report_rel(dev, REL_Y,     data[2]);
	input_report_rel(dev, REL_WHEEL, data[3]);

	input_sync(dev);
*/

	{
		MOUSE_INPUT_DATA MouseInputData;
		ULONG InputDataConsumed;

		MouseInputData.Flags = MOUSE_MOVE_RELATIVE;
		MouseInputData.LastX = data[1];
		MouseInputData.LastY = data[2];

		MouseInputData.ButtonFlags = 0;
		MouseInputData.ButtonData = 0;

		if ((data[0] & 0x01) && ((mouse->btn_old & 0x01) != (data[0] & 0x01)))
			MouseInputData.ButtonFlags |= MOUSE_LEFT_BUTTON_DOWN;
		else if (!(data[0] & 0x01) && ((mouse->btn_old & 0x01) != (data[0] & 0x01)))
			MouseInputData.ButtonFlags |= MOUSE_LEFT_BUTTON_UP;

		if ((data[0] & 0x02) && ((mouse->btn_old & 0x02) != (data[0] & 0x02)))
			MouseInputData.ButtonFlags |= MOUSE_RIGHT_BUTTON_DOWN;
		else if (!(data[0] & 0x02) && ((mouse->btn_old & 0x02) != (data[0] & 0x02)))
			MouseInputData.ButtonFlags |= MOUSE_RIGHT_BUTTON_UP;

		if ((data[0] & 0x04) && ((mouse->btn_old & 0x04) != (data[0] & 0x04)))
			MouseInputData.ButtonFlags |= MOUSE_MIDDLE_BUTTON_DOWN;
		else if (!(data[0] & 0x04) && ((mouse->btn_old & 0x04) != (data[0] & 0x04)))
			MouseInputData.ButtonFlags |= MOUSE_MIDDLE_BUTTON_UP;

		if ((data[0] & 0x08) && ((mouse->btn_old & 0x08) != (data[0] & 0x08)))
			MouseInputData.ButtonFlags |= MOUSE_BUTTON_4_DOWN;
		else if (!(data[0] & 0x08) && ((mouse->btn_old & 0x08) != (data[0] & 0x08)))
			MouseInputData.ButtonFlags |= MOUSE_BUTTON_4_UP;

		if ((data[0] & 0x10) && ((mouse->btn_old & 0x10) != (data[0] & 0x10)))
			MouseInputData.ButtonFlags |= MOUSE_BUTTON_5_DOWN;
		else if (!(data[0] & 0x10) && ((mouse->btn_old & 0x10) != (data[0] & 0x10)))
			MouseInputData.ButtonFlags |= MOUSE_BUTTON_5_UP;

		if (data[3])
		{
			MouseInputData.ButtonFlags |= MOUSE_WHEEL;
			MouseInputData.ButtonData = data[3];
		}
        
		if (UsbPortInterface.MouseConnectData->ClassService)
		{
			KIRQL OldIrql;

			KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
			(*(PSERVICE_CALLBACK_ROUTINE)UsbPortInterface.MouseConnectData->ClassService)(
				UsbPortInterface.MouseConnectData->ClassDeviceObject,
				&MouseInputData,
				(&MouseInputData)+1,
				&InputDataConsumed);
			KeLowerIrql(OldIrql);
		}

		mouse->btn_old = data[0];

		// debug info
		printk("MouseInputData.Buttons=0x%03x\n", MouseInputData.Buttons);
	}

	printk("Mouse input: x %d, y %d, w %d, btn: 0x%02x\n", data[1], data[2], data[3], data[0]);

resubmit:
	status = usb_submit_urb (urb, SLAB_ATOMIC);
	if (status)
		err ("can't resubmit intr, %s-%s/input0, status %d",
				mouse->usbdev->bus->bus_name,
				mouse->usbdev->devpath, status);
}
/*
static int usb_mouse_open(struct input_dev *dev)
{
	struct usb_mouse *mouse = dev->private;

	if (mouse->open++)
		return 0;

	mouse->irq->dev = mouse->usbdev;
	if (usb_submit_urb(mouse->irq, GFP_KERNEL)) {
		mouse->open--;
		return -EIO;
	}

	return 0;
}

static void usb_mouse_close(struct input_dev *dev)
{
	struct usb_mouse *mouse = dev->private;

	if (!--mouse->open)
		usb_unlink_urb(mouse->irq);
}
*/

static int usb_mouse_probe(struct usb_interface * intf, const struct usb_device_id * id)
{
	struct usb_device * dev = interface_to_usbdev(intf);
	struct usb_host_interface *interface;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_mouse *mouse;
	int pipe, maxp;
	char path[64];
	char *buf;

	interface = &intf->altsetting[intf->act_altsetting];

	if (interface->desc.bNumEndpoints != 1) 
		return -ENODEV;

	endpoint = &interface->endpoint[0].desc;
	if (!(endpoint->bEndpointAddress & 0x80)) 
		return -ENODEV;
	if ((endpoint->bmAttributes & 3) != 3) 
		return -ENODEV;

	pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);
	maxp = usb_maxpacket(dev, pipe, usb_pipeout(pipe));

	if (!(mouse = kmalloc(sizeof(struct usb_mouse), GFP_KERNEL))) 
		return -ENOMEM;
	memset(mouse, 0, sizeof(struct usb_mouse));

	mouse->data = usb_buffer_alloc(dev, 8, SLAB_ATOMIC, &mouse->data_dma);
	if (!mouse->data) {
		kfree(mouse);
		return -ENOMEM;
	}

	mouse->irq = usb_alloc_urb(0, GFP_KERNEL);
	if (!mouse->irq) {
		usb_buffer_free(dev, 8, mouse->data, mouse->data_dma);
		kfree(mouse);
		return -ENODEV;
	}

	mouse->usbdev = dev;

	usb_make_path(dev, path, 64);
	sprintf(mouse->phys, "%s/input0", path);

	if (!(buf = kmalloc(63, GFP_KERNEL))) {
		usb_buffer_free(dev, 8, mouse->data, mouse->data_dma);
		kfree(mouse);
		return -ENOMEM;
	}

	if (dev->descriptor.iManufacturer &&
		usb_string(dev, dev->descriptor.iManufacturer, buf, 63) > 0)
			strcat(mouse->name, buf);
	if (dev->descriptor.iProduct &&
		usb_string(dev, dev->descriptor.iProduct, buf, 63) > 0)
			sprintf(mouse->name, "%s %s", mouse->name, buf);

	if (!strlen(mouse->name))
		sprintf(mouse->name, "USB HIDBP Mouse %04x:%04x",
			dev->descriptor.idVendor, dev->descriptor.idProduct);

	kfree(buf);

	usb_fill_int_urb(mouse->irq, dev, pipe, mouse->data,
			 (maxp > 8 ? 8 : maxp),
			 usb_mouse_irq, mouse, endpoint->bInterval);
	//mouse->irq->transfer_dma = mouse->data_dma;
	//mouse->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	printk(KERN_INFO "input: %s on %s\n", mouse->name, path);

	usb_set_intfdata(intf, mouse);

	// Open device
	mouse->irq->dev = mouse->usbdev;
	if (usb_submit_urb(mouse->irq, GFP_KERNEL)) {
		return -EIO;
	}

	mouse->btn_old = 0;

	return 0;
}

static void usb_mouse_disconnect(struct usb_interface *intf)
{
	struct usb_mouse *mouse = usb_get_intfdata (intf);
	
	usb_set_intfdata(intf, NULL);
	usbprintk("Mouse disconnected\n ");
	if (mouse) {
		usb_unlink_urb(mouse->irq);
		usb_free_urb(mouse->irq);
		usb_buffer_free(interface_to_usbdev(intf), 8, mouse->data, mouse->data_dma);
		kfree(mouse);
	}
}

static struct usb_device_id usb_mouse_id_table [] = {
	{ USB_INTERFACE_INFO(3, 1, 2) },
    { }						/* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, usb_mouse_id_table);

static struct usb_driver usb_mouse_driver = {
	.owner		= THIS_MODULE,
	.name		= "usbmouse",
	.probe		= usb_mouse_probe,
	.disconnect	= usb_mouse_disconnect,
	.id_table	= usb_mouse_id_table,
};

void UsbMouseInit(void)
{
	if (usb_register(&usb_mouse_driver) < 0) {
		#if mousedebug
		printk("Unable to register Mouse driver");
		#endif
		return;
	}       
}

void UsbMouseRemove(void) {
	usb_deregister(&usb_mouse_driver);
}
