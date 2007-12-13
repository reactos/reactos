#include "../usb_wrapper.h"

#define keyboarddebug  0

#if keyboarddebug
extern int printe(const char *szFormat, ...);
int ycoffset = 0;
#endif

unsigned int current_keyboard_key;

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

static void usb_kbd_irq(struct urb *urb, struct pt_regs *regs)
{
	struct usb_kbd_info *kbd = urb->context;
	int i;

	if (urb->status) return;
	
	memcpy(kbd->kbd_pkt, urb->transfer_buffer, 8);
	
	current_keyboard_key = kbd->kbd_pkt[2];
	
	
	#if keyboarddebug
	ycoffset += 15;
	ycoffset = ycoffset % 600;
	VIDEO_CURSOR_POSX=20;
	VIDEO_CURSOR_POSY=ycoffset;	
	printe(" -%02x %02x %02x %02x %02x %02x\n",kbd->kbd_pkt[0],kbd->kbd_pkt[1],kbd->kbd_pkt[2],kbd->kbd_pkt[3],kbd->kbd_pkt[4],kbd->kbd_pkt[5]);
	#endif
	
	usb_submit_urb(urb,GFP_ATOMIC);
		
}

static int usb_kbd_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct urb *urb;
	struct usb_device *udev = interface_to_usbdev (intf);
	struct usb_endpoint_descriptor *ep_irq_in;
	struct usb_endpoint_descriptor *ep_irq_out;
	struct usb_kbd_info *usbk;

	int i, pipe, maxp;
	char *buf;

	usbk=(struct usb_kbd_info *)kmalloc(sizeof(struct usb_kbd_info),0);
	if (!usbk) return -1;

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
	printe("USB Keyboard Connected\n");	
	#endif
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
		printe("Unable to register Keyboard driver");
		#endif
		return;
	}       
}

void UsbKeyBoardRemove(void) {
	usb_deregister(&usb_kbd_driver);
}
