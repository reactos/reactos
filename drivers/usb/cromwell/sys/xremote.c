/*
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * 
 */

/*
 *  $Id: xremote.c,v 1.5 2004/11/22 19:10:57 davidmpye Exp $
 *
 *  Copyright (c) 2002 Steven Toth <steve@toth.demon.co.uk>
 *
 *  XBOX DVD dongle infrared device driver for the input driver suite.
 *
 *  This work was derived from the usbkbd.c kernel module.
 *
 *  History:
 *
 *  2002_08_31 - 0.1 - Initial release
 *  2002_09_02 - 0.2 - Added IOCTL support enabling user space administration
 *                     of the translation matrix.
 *
 */

#include "../usb_wrapper.h"


u16 current_remote_key;
u8 remotekeyIsRepeat;

struct xremote_info 
{
	struct urb *urb;
	unsigned char irpkt[8];
};

/*  USB callback completion handler
 *  Code in transfer_buffer is received as six unsigned chars
 *  Example PLAY=00 06 ea 0a 40 00
 *  The command is located in byte[2], the rest are ignored.
 *  Key position is byte[4] bit0 (7-0 format) 0=down, 1=up
 *  All other bits are unknown / now required.
 */

static void xremote_irq(struct urb *urb, struct pt_regs *regs)
{
	struct xremote_info *xri = urb->context;
        
	if (urb->status) return;
	if (urb->actual_length < 6) return;

	/* Messy/unnecessary, fix this */
	memcpy(xri->irpkt, urb->transfer_buffer, 6);

	/* Set the key action based in the sent action */
	current_remote_key = ((xri->irpkt[2] & 0xff)<<8) | (xri->irpkt[3] & 0xff);

	if (((xri->irpkt[4] & 0xff) + ((xri->irpkt[5] & 0xff ) << 8))>0x41) {
		remotekeyIsRepeat=0;
	}
	else remotekeyIsRepeat=1;
		             
	usb_submit_urb(urb,GFP_ATOMIC);
}

static int xremote_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct urb *urb;
	struct usb_device *udev = interface_to_usbdev (intf);
	struct usb_endpoint_descriptor *ep_irq_in;
	struct usb_endpoint_descriptor *ep_irq_out;
	struct xremote_info *xri;

	xri=(struct xremote_info *)kmalloc(sizeof(struct xremote_info),0);
	if (!xri) return -1;

	urb=usb_alloc_urb(0,0);
	if (!urb) return -1;

	xri->urb=urb;

	ep_irq_in = &intf->altsetting[0].endpoint[0].desc;
	usb_fill_int_urb(urb, udev,
                         usb_rcvintpipe(udev, ep_irq_in->bEndpointAddress),
                         xri->irpkt, 8, xremote_irq,
                         xri, 8);

	usb_submit_urb(urb,GFP_ATOMIC);
	usb_set_intfdata(intf,xri);

	usbprintk("DVD Remote connected\n");
	return 0;
}

static void xremote_disconnect(struct usb_interface *intf)
{
	struct xremote_info *xri = usb_get_intfdata (intf);
	usbprintk("DVD Remote disconnected\n ");
	usb_unlink_urb(xri->urb);
	usb_free_urb(xri->urb);
	kfree(xri);
}

static struct usb_device_id xremote_id_table [] = {
	{ USB_DEVICE(0x040b, 0x6521) }, /* Gamester Xbox DVD Movie Playback Kit IR */
	{ USB_DEVICE(0x045e, 0x0284) }, /* Microsoft Xbox DVD Movie Playback Kit IR */
	{ USB_DEVICE(0x0000, 0x0000) }, // nothing detected - FAIL
	{ } /* Terminating entry */
};

static struct usb_driver xremote_driver = {
	.owner =		THIS_MODULE,
	.name =			"XRemote",
	.probe =		xremote_probe,
	.disconnect =		xremote_disconnect,
	.id_table =		xremote_id_table,
};

void XRemoteInit(void)
{

	current_remote_key=0;
	usbprintk("XRemote probe %p ",xremote_probe);
	if (usb_register(&xremote_driver) < 0) {
		err("Unable to register XRemote driver");
		return;
	}       
}

void XRemoteRemove(void) {
	usb_deregister(&xremote_driver);
}
