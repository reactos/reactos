/*
 This driver is based on Cromwell's usbkey driver
 and also includes stuff from Linux 2.5 usbkey driver by Vojtech Pavlik
*/

#define NDEBUG
#include "../../usb_wrapper.h"

#define keyboarddebug  0

#if keyboarddebug
//extern int printk(const char *szFormat, ...);
#endif

unsigned int current_keyboard_key;

extern USBPORT_INTERFACE UsbPortInterface;

static unsigned char usb_kbd_keycode[256] = {
	  0,  0,  0,  0, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38,
	 50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44,  2,  3,
	  4,  5,  6,  7,  8,  9, 10, 11, 28,  1, 14, 15, 57, 12, 13, 26,
	 27, 43, 84, 39, 40, 41, 51, 52, 53, 58, 59, 60, 61, 62, 63, 64,
	 65, 66, 67, 68, 87, 88, 99, 70,119,110,102,104,111,107,109,106,
	105,108,103, 69, 98, 55, 74, 78, 96, 79, 80, 81, 75, 76, 77, 71,
	 72, 73, 82, 83, 86,127,116,117, 85, 89, 90, 91, 92, 93, 94, 95,
	120,121,122,123,134,138,130,132,128,129,131,137,133,135,136,113,
	115,114,  0,  0,  0,124,  0,181,182,183,184,185,186,187,188,189,
	190,191,192,193,194,195,196,197,198,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 29, 42, 56,125, 97, 54,100,126,164,166,165,163,161,115,114,113,
	150,158,159,128,136,177,178,176,142,152,173,140
};

struct usb_kbd_info {
	struct urb *urb;
	unsigned char kbd_pkt[8];
	unsigned char old[8];

	/*
	struct input_dev dev;
	struct usb_device *usbdev;
	struct urb irq, led;
	struct usb_ctrlrequest dr;
	unsigned char leds, newleds;
	char name[128];
	int open;
	*/
};

/**
 * memscan - Find a character in an area of memory.
 * @addr: The memory area
 * @c: The byte to search for
 * @size: The size of the area.
 *
 * returns the address of the first occurrence of @c, or 1 byte past
 * the area if @c is not found
 */
void * memscan(void * addr, int c, size_t size)
{
	unsigned char * p = (unsigned char *) addr;

	while (size) {
		if (*p == c)
			return (void *) p;
		p++;
		size--;
	}
  	return (void *) p;
}

void input_report_key(unsigned int code, int value)
{
	KEYBOARD_INPUT_DATA KeyboardInputData;
	ULONG InputDataConsumed;

	KeyboardInputData.MakeCode = code;
	KeyboardInputData.Flags = (value == 1) ? KEY_MAKE : KEY_BREAK;

	if (UsbPortInterface.KbdConnectData->ClassService)
	{
		KIRQL OldIrql;

		KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
		(*(PSERVICE_CALLBACK_ROUTINE)UsbPortInterface.KbdConnectData->ClassService)(
			UsbPortInterface.KbdConnectData->ClassDeviceObject,
			&KeyboardInputData,
			(&KeyboardInputData)+1,
			&InputDataConsumed);
		KeLowerIrql(OldIrql);
	}
}

static void usb_kbd_irq(struct urb *urb, struct pt_regs *regs)
{
	struct usb_kbd_info *kbd = urb->context;
	int i;

	if (urb->status) return;

	memcpy(kbd->kbd_pkt, urb->transfer_buffer, 8);

	//for (i = 0; i < 8; i++)
	//	input_report_key(usb_kbd_keycode[i + 224], (kbd->kbd_pkt[0] >> i) & 1);

	for (i = 2; i < 8; i++) {

		if (kbd->old[i] > 3 && memscan(kbd->kbd_pkt + 2, kbd->old[i], 6) == kbd->kbd_pkt + 8) {
			if (usb_kbd_keycode[kbd->old[i]])
				input_report_key(usb_kbd_keycode[kbd->old[i]], 0);
			else
				info("Unknown key (scancode %#x) released.", kbd->old[i]);
		}

		if (kbd->kbd_pkt[i] > 3 && memscan(kbd->old + 2, kbd->kbd_pkt[i], 6) == kbd->old + 8) {
			if (usb_kbd_keycode[kbd->kbd_pkt[i]])
				input_report_key(usb_kbd_keycode[kbd->kbd_pkt[i]], 1);
			else
				info("Unknown key (scancode %#x) pressed.", kbd->kbd_pkt[i]);
		}
	}

	memcpy(kbd->old, kbd->kbd_pkt, 8);

#if 0
	//memcpy(kbd->kbd_pkt, urb->transfer_buffer, 8);
	//current_keyboard_key = kbd->kbd_pkt[2];
	{
		KEYBOARD_INPUT_DATA KeyboardInputData;
		ULONG InputDataConsumed;

		KeyboardInputData.MakeCode = current_keyboard_key & ~0x80;
		KeyboardInputData.Flags = (current_keyboard_key & 0x80) ? KEY_MAKE : KEY_BREAK;

		if (UsbPortInterface.KbdConnectData->ClassService)
		{
			KIRQL OldIrql;

			KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
			(*(PSERVICE_CALLBACK_ROUTINE)UsbPortInterface.KbdConnectData->ClassService)(
				UsbPortInterface.KbdConnectData->ClassDeviceObject,
				&KeyboardInputData,
				(&KeyboardInputData)+1,
				&InputDataConsumed);
			KeLowerIrql(OldIrql);
		}
	}
#endif
	
	
	#if keyboarddebug
	printk(" -%02x %02x %02x %02x %02x %02x\n",kbd->kbd_pkt[0],kbd->kbd_pkt[1],kbd->kbd_pkt[2],kbd->kbd_pkt[3],kbd->kbd_pkt[4],kbd->kbd_pkt[5]);
	#endif
	
	usb_submit_urb(urb,GFP_ATOMIC);
		
}

static int usb_kbd_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct urb *urb;
	struct usb_device *udev = interface_to_usbdev (intf);
	struct usb_endpoint_descriptor *ep_irq_in;
	//struct usb_endpoint_descriptor *ep_irq_out;
	struct usb_kbd_info *usbk;

	//int i, pipe, maxp;
	//char *buf;

	usbk=(struct usb_kbd_info *)kmalloc(sizeof(struct usb_kbd_info),0);
	if (!usbk) return -1;
    memset(usbk, 0, sizeof(struct usb_kbd_info));

	urb=usb_alloc_urb(0,0);
	if (!urb) return -1;

	usbk->urb=urb;

	ep_irq_in = &intf->altsetting[0].endpoint[0].desc;
	usb_fill_int_urb(urb, udev,
                         usb_rcvintpipe(udev, ep_irq_in->bEndpointAddress),
                         usbk->kbd_pkt, 8, usb_kbd_irq,
                         usbk, 8);

	usb_submit_urb(urb,GFP_ATOMIC);
	usb_set_intfdata(intf,usbk);
	#if keyboarddebug
	printk("USB Keyboard Connected\n");	
	#endif

	return 0;
}


static void usb_kbd_disconnect(struct usb_interface *intf)
{
	struct usb_kbd_info *usbk = usb_get_intfdata (intf);
	usbprintk("Keyboard disconnected\n ");
	usb_unlink_urb(usbk->urb);
	usb_free_urb(usbk->urb);
	kfree(usbk);
}

static struct usb_device_id usb_kbd_id_table [] = {
	{ USB_INTERFACE_INFO(3, 1, 1) },
	{ }						/* Terminating entry */
};


static struct usb_driver usb_kbd_driver = {
	.owner =		THIS_MODULE,
	.name =			"keyboard",
	.probe =		usb_kbd_probe,
	.disconnect =		usb_kbd_disconnect,
	.id_table =		usb_kbd_id_table,
};

void UsbKeyBoardInit(void)
{
	//current_remote_key=0;
	//sbprintk("Keyboard probe %p ",xremote_probe);
	if (usb_register(&usb_kbd_driver) < 0) {
		#if keyboarddebug
		printk("Unable to register Keyboard driver");
		#endif
		return;
	}       
}

void UsbKeyBoardRemove(void) {
	usb_deregister(&usb_kbd_driver);
}
