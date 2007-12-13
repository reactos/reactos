/*
 * (C) Copyright Linus Torvalds 1999
 * (C) Copyright Johannes Erdfelt 1999-2001
 * (C) Copyright Andreas Gal 1999
 * (C) Copyright Gregory P. Smith 1999
 * (C) Copyright Deti Fliegl 1999
 * (C) Copyright Randy Dunlap 2000
 * (C) Copyright David Brownell 2000-2002
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if 0
#include <linux/config.h>

#ifdef CONFIG_USB_DEBUG
#define DEBUG
#endif

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include <linux/uts.h>			/* for UTS_SYSNAME */
#include <linux/pci.h>			/* for hcd->pdev and dma addressing */
#include <linux/dma-mapping.h>
#include <asm/byteorder.h>

#include <linux/usb.h>
#else
#include "../usb_wrapper.h"
//#define DEBUG
#endif

#include "hcd.h"

// #define USB_BANDWIDTH_MESSAGES

/*-------------------------------------------------------------------------*/

/*
 * USB Host Controller Driver framework
 *
 * Plugs into usbcore (usb_bus) and lets HCDs share code, minimizing
 * HCD-specific behaviors/bugs.
 *
 * This does error checks, tracks devices and urbs, and delegates to a
 * "hc_driver" only for code (and data) that really needs to know about
 * hardware differences.  That includes root hub registers, i/o queues,
 * and so on ... but as little else as possible.
 *
 * Shared code includes most of the "root hub" code (these are emulated,
 * though each HC's hardware works differently) and PCI glue, plus request
 * tracking overhead.  The HCD code should only block on spinlocks or on
 * hardware handshaking; blocking on software events (such as other kernel
 * threads releasing resources, or completing actions) is all generic.
 *
 * Happens the USB 2.0 spec says this would be invisible inside the "USBD",
 * and includes mostly a "HCDI" (HCD Interface) along with some APIs used
 * only by the hub driver ... and that neither should be seen or used by
 * usb client device drivers.
 *
 * Contributors of ideas or unattributed patches include: David Brownell,
 * Roman Weissgaerber, Rory Bolt, Greg Kroah-Hartman, ...
 *
 * HISTORY:
 * 2002-02-21	Pull in most of the usb_bus support from usb.c; some
 *		associated cleanup.  "usb_hcd" still != "usb_bus".
 * 2001-12-12	Initial patch version for Linux 2.5.1 kernel.
 */

/*-------------------------------------------------------------------------*/

/* host controllers we manage */
LIST_HEAD (usb_bus_list);
EXPORT_SYMBOL_GPL (usb_bus_list);

/* used when allocating bus numbers */
#define USB_MAXBUS		64
struct usb_busmap {
	unsigned long busmap [USB_MAXBUS / (8*sizeof (unsigned long))];
};
static struct usb_busmap busmap;

/* used when updating list of hcds */
DECLARE_MUTEX (usb_bus_list_lock);	/* exported only for usbfs */
EXPORT_SYMBOL_GPL (usb_bus_list_lock);

/* used when updating hcd data */
static spinlock_t hcd_data_lock = SPIN_LOCK_UNLOCKED;

/*-------------------------------------------------------------------------*/

/*
 * Sharable chunks of root hub code.
 */

/*-------------------------------------------------------------------------*/

#define KERNEL_REL	((LINUX_VERSION_CODE >> 16) & 0x0ff)
#define KERNEL_VER	((LINUX_VERSION_CODE >> 8) & 0x0ff)

/* usb 2.0 root hub device descriptor */
static const u8 usb2_rh_dev_descriptor [18] = {
	0x12,       /*  __u8  bLength; */
	0x01,       /*  __u8  bDescriptorType; Device */
	0x00, 0x02, /*  __u16 bcdUSB; v2.0 */

	0x09,	    /*  __u8  bDeviceClass; HUB_CLASSCODE */
	0x00,	    /*  __u8  bDeviceSubClass; */
	0x01,       /*  __u8  bDeviceProtocol; [ usb 2.0 single TT ]*/
	0x08,       /*  __u8  bMaxPacketSize0; 8 Bytes */

	0x00, 0x00, /*  __u16 idVendor; */
 	0x00, 0x00, /*  __u16 idProduct; */
	KERNEL_VER, KERNEL_REL, /*  __u16 bcdDevice */

	0x03,       /*  __u8  iManufacturer; */
	0x02,       /*  __u8  iProduct; */
	0x01,       /*  __u8  iSerialNumber; */
	0x01        /*  __u8  bNumConfigurations; */
};

/* no usb 2.0 root hub "device qualifier" descriptor: one speed only */

/* usb 1.1 root hub device descriptor */
static const u8 usb11_rh_dev_descriptor [18] = {
	0x12,       /*  __u8  bLength; */
	0x01,       /*  __u8  bDescriptorType; Device */
	0x10, 0x01, /*  __u16 bcdUSB; v1.1 */

	0x09,	    /*  __u8  bDeviceClass; HUB_CLASSCODE */
	0x00,	    /*  __u8  bDeviceSubClass; */
	0x00,       /*  __u8  bDeviceProtocol; [ low/full speeds only ] */
	0x08,       /*  __u8  bMaxPacketSize0; 8 Bytes */

	0x00, 0x00, /*  __u16 idVendor; */
 	0x00, 0x00, /*  __u16 idProduct; */
	KERNEL_VER, KERNEL_REL, /*  __u16 bcdDevice */

	0x03,       /*  __u8  iManufacturer; */
	0x02,       /*  __u8  iProduct; */
	0x01,       /*  __u8  iSerialNumber; */
	0x01        /*  __u8  bNumConfigurations; */
};


/*-------------------------------------------------------------------------*/

/* Configuration descriptors for our root hubs */

static const u8 fs_rh_config_descriptor [] = {

	/* one configuration */
	0x09,       /*  __u8  bLength; */
	0x02,       /*  __u8  bDescriptorType; Configuration */
	0x19, 0x00, /*  __u16 wTotalLength; */
	0x01,       /*  __u8  bNumInterfaces; (1) */
	0x01,       /*  __u8  bConfigurationValue; */
	0x00,       /*  __u8  iConfiguration; */
	0x40,       /*  __u8  bmAttributes; 
				 Bit 7: Bus-powered,
				     6: Self-powered,
				     5 Remote-wakwup,
				     4..0: resvd */
	0x00,       /*  __u8  MaxPower; */
      
	/* USB 1.1:
	 * USB 2.0, single TT organization (mandatory):
	 *	one interface, protocol 0
	 *
	 * USB 2.0, multiple TT organization (optional):
	 *	two interfaces, protocols 1 (like single TT)
	 *	and 2 (multiple TT mode) ... config is
	 *	sometimes settable
	 *	NOT IMPLEMENTED
	 */

	/* one interface */
	0x09,       /*  __u8  if_bLength; */
	0x04,       /*  __u8  if_bDescriptorType; Interface */
	0x00,       /*  __u8  if_bInterfaceNumber; */
	0x00,       /*  __u8  if_bAlternateSetting; */
	0x01,       /*  __u8  if_bNumEndpoints; */
	0x09,       /*  __u8  if_bInterfaceClass; HUB_CLASSCODE */
	0x00,       /*  __u8  if_bInterfaceSubClass; */
	0x00,       /*  __u8  if_bInterfaceProtocol; [usb1.1 or single tt] */
	0x00,       /*  __u8  if_iInterface; */
     
	/* one endpoint (status change endpoint) */
	0x07,       /*  __u8  ep_bLength; */
	0x05,       /*  __u8  ep_bDescriptorType; Endpoint */
	0x81,       /*  __u8  ep_bEndpointAddress; IN Endpoint 1 */
 	0x03,       /*  __u8  ep_bmAttributes; Interrupt */
 	0x02, 0x00, /*  __u16 ep_wMaxPacketSize; 1 + (MAX_ROOT_PORTS / 8) */
	0xff        /*  __u8  ep_bInterval; (255ms -- usb 2.0 spec) */
};

static const u8 hs_rh_config_descriptor [] = {

	/* one configuration */
	0x09,       /*  __u8  bLength; */
	0x02,       /*  __u8  bDescriptorType; Configuration */
	0x19, 0x00, /*  __u16 wTotalLength; */
	0x01,       /*  __u8  bNumInterfaces; (1) */
	0x01,       /*  __u8  bConfigurationValue; */
	0x00,       /*  __u8  iConfiguration; */
	0x40,       /*  __u8  bmAttributes; 
				 Bit 7: Bus-powered,
				     6: Self-powered,
				     5 Remote-wakwup,
				     4..0: resvd */
	0x00,       /*  __u8  MaxPower; */
      
	/* USB 1.1:
	 * USB 2.0, single TT organization (mandatory):
	 *	one interface, protocol 0
	 *
	 * USB 2.0, multiple TT organization (optional):
	 *	two interfaces, protocols 1 (like single TT)
	 *	and 2 (multiple TT mode) ... config is
	 *	sometimes settable
	 *	NOT IMPLEMENTED
	 */

	/* one interface */
	0x09,       /*  __u8  if_bLength; */
	0x04,       /*  __u8  if_bDescriptorType; Interface */
	0x00,       /*  __u8  if_bInterfaceNumber; */
	0x00,       /*  __u8  if_bAlternateSetting; */
	0x01,       /*  __u8  if_bNumEndpoints; */
	0x09,       /*  __u8  if_bInterfaceClass; HUB_CLASSCODE */
	0x00,       /*  __u8  if_bInterfaceSubClass; */
	0x00,       /*  __u8  if_bInterfaceProtocol; [usb1.1 or single tt] */
	0x00,       /*  __u8  if_iInterface; */
     
	/* one endpoint (status change endpoint) */
	0x07,       /*  __u8  ep_bLength; */
	0x05,       /*  __u8  ep_bDescriptorType; Endpoint */
	0x81,       /*  __u8  ep_bEndpointAddress; IN Endpoint 1 */
 	0x03,       /*  __u8  ep_bmAttributes; Interrupt */
 	0x02, 0x00, /*  __u16 ep_wMaxPacketSize; 1 + (MAX_ROOT_PORTS / 8) */
	0x0c        /*  __u8  ep_bInterval; (256ms -- usb 2.0 spec) */
};

/*-------------------------------------------------------------------------*/

/*
 * helper routine for returning string descriptors in UTF-16LE
 * input can actually be ISO-8859-1; ASCII is its 7-bit subset
 */
static int ascii2utf (char *s, u8 *utf, int utfmax)
{
	int retval;

	for (retval = 0; *s && utfmax > 1; utfmax -= 2, retval += 2) {
		*utf++ = *s++;
		*utf++ = 0;
	}
	return retval;
}

/*
 * rh_string - provides manufacturer, product and serial strings for root hub
 * @id: the string ID number (1: serial number, 2: product, 3: vendor)
 * @hcd: the host controller for this root hub
 * @type: string describing our driver 
 * @data: return packet in UTF-16 LE
 * @len: length of the return packet
 *
 * Produces either a manufacturer, product or serial number string for the
 * virtual root hub device.
 */
static int rh_string (
	int		id,
	struct usb_hcd	*hcd,
	u8		*data,
	int		len
) {
	char buf [100];

	// language ids
	if (id == 0) {
		*data++ = 4; *data++ = 3;	/* 4 bytes string data */
		*data++ = 0x09; *data++ = 0x04;	/* MSFT-speak for "en-us" */
		return 4;

	// serial number
	} else if (id == 1) {
		strcpy (buf, hcd->self.bus_name);

	// product description
	} else if (id == 2) {
                strcpy (buf, hcd->product_desc);

 	// id 3 == vendor description
	} else if (id == 3) {
                sprintf (buf, "%s %s %s", UTS_SYSNAME, UTS_RELEASE,
			hcd->description);

	// unsupported IDs --> "protocol stall"
	} else
	    return 0;

	data [0] = 2 * (strlen (buf) + 1);
	data [1] = 3;	/* type == string */
	return 2 + ascii2utf (buf, data + 2, len - 2);
}


/* Root hub control transfers execute synchronously */
static int rh_call_control (struct usb_hcd *hcd, struct urb *urb)
{
	struct usb_ctrlrequest *cmd = (struct usb_ctrlrequest *) urb->setup_packet;
 	u16		typeReq, wValue, wIndex, wLength;
	const u8	*bufp = 0;
	u8		*ubuf = urb->transfer_buffer;
	int		len = 0;
	//unsigned long	flags;

	typeReq  = (cmd->bRequestType << 8) | cmd->bRequest;
	wValue   = le16_to_cpu (cmd->wValue);
	wIndex   = le16_to_cpu (cmd->wIndex);
	wLength  = le16_to_cpu (cmd->wLength);

	if (wLength > urb->transfer_buffer_length)
		goto error;

	/* set up for success */
	urb->status = 0;
	urb->actual_length = wLength;
	switch (typeReq) {

	/* DEVICE REQUESTS */

	case DeviceRequest | USB_REQ_GET_STATUS:
		// DEVICE_REMOTE_WAKEUP
		ubuf [0] = 1; // selfpowered
		ubuf [1] = 0;
			/* FALLTHROUGH */
	case DeviceOutRequest | USB_REQ_CLEAR_FEATURE:
	case DeviceOutRequest | USB_REQ_SET_FEATURE:
		dev_dbg (hcd->controller, "no device features yet yet\n");
		break;
	case DeviceRequest | USB_REQ_GET_CONFIGURATION:
		ubuf [0] = 1;
			/* FALLTHROUGH */
	case DeviceOutRequest | USB_REQ_SET_CONFIGURATION:
		break;
	case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
		switch (wValue & 0xff00) {
		case USB_DT_DEVICE << 8:
			if (hcd->driver->flags & HCD_USB2)
				bufp = usb2_rh_dev_descriptor;
			else if (hcd->driver->flags & HCD_USB11)
				bufp = usb11_rh_dev_descriptor;
			else
				goto error;
			len = 18;
			break;
		case USB_DT_CONFIG << 8:
			if (hcd->driver->flags & HCD_USB2) {
				bufp = hs_rh_config_descriptor;
				len = sizeof hs_rh_config_descriptor;
			} else {
				bufp = fs_rh_config_descriptor;
				len = sizeof fs_rh_config_descriptor;
			}
			break;
		case USB_DT_STRING << 8:
			urb->actual_length = rh_string (
				wValue & 0xff, hcd,
				ubuf, wLength);
			break;
		default:
			goto error;
		}
		break;
	case DeviceRequest | USB_REQ_GET_INTERFACE:
		ubuf [0] = 0;
			/* FALLTHROUGH */
	case DeviceOutRequest | USB_REQ_SET_INTERFACE:
		break;
	case DeviceOutRequest | USB_REQ_SET_ADDRESS:
		// wValue == urb->dev->devaddr
		dev_dbg (hcd->controller, "root hub device address %d\n",
			wValue);
		break;

	/* INTERFACE REQUESTS (no defined feature/status flags) */

	/* ENDPOINT REQUESTS */

	case EndpointRequest | USB_REQ_GET_STATUS:
		// ENDPOINT_HALT flag
		ubuf [0] = 0;
		ubuf [1] = 0;
			/* FALLTHROUGH */
	case EndpointOutRequest | USB_REQ_CLEAR_FEATURE:
	case EndpointOutRequest | USB_REQ_SET_FEATURE:
		dev_dbg (hcd->controller, "no endpoint features yet\n");
		break;

	/* CLASS REQUESTS (and errors) */

	default:
		/* non-generic request */
		urb->status = hcd->driver->hub_control (hcd,
			typeReq, wValue, wIndex,
			ubuf, wLength);
		break;
error:
		/* "protocol stall" on error */
		urb->status = -EPIPE;
		dev_dbg (hcd->controller, "unsupported hub control message (maxchild %d)\n",
				urb->dev->maxchild);
	}
	if (urb->status) {
		urb->actual_length = 0;
		dev_dbg (hcd->controller, "CTRL: TypeReq=0x%x val=0x%x idx=0x%x len=%d ==> %d\n",
			typeReq, wValue, wIndex, wLength, urb->status);
	}
	if (bufp) {
		if (urb->transfer_buffer_length < len)
			len = urb->transfer_buffer_length;
		urb->actual_length = len;
		// always USB_DIR_IN, toward host
		memcpy (ubuf, bufp, len);
	}

	/* any errors get returned through the urb completion */
	local_irq_save (flags);
	usb_hcd_giveback_urb (hcd, urb, NULL);
	local_irq_restore (flags);
	return 0;
}

/*-------------------------------------------------------------------------*/

/*
 * Root Hub interrupt transfers are synthesized with a timer.
 * Completions are called in_interrupt() but not in_irq().
 */

static void rh_report_status (unsigned long ptr);

static int rh_status_urb (struct usb_hcd *hcd, struct urb *urb) 
{
	int	len = 1 + (urb->dev->maxchild / 8);

	/* rh_timer protected by hcd_data_lock */
	if (hcd->rh_timer.data
			|| urb->status != -EINPROGRESS
			|| urb->transfer_buffer_length < len) {
		dev_dbg (hcd->controller,
				"not queuing rh status urb, stat %d\n",
				urb->status);
		return -EINVAL;
	}

	init_timer (&hcd->rh_timer);

	hcd->rh_timer.function = rh_report_status;
	hcd->rh_timer.data = (unsigned long) urb;
	/* USB 2.0 spec says 256msec; this is close enough */
	hcd->rh_timer.expires = jiffies + HZ/4;
	add_timer (&hcd->rh_timer);
	urb->hcpriv = hcd;	/* nonzero to indicate it's queued */
	return 0;
}

/* timer callback */

static void rh_report_status (unsigned long ptr)
{
	struct urb	*urb;
	struct usb_hcd	*hcd;
	int		length;
	//unsigned long	flags;

	urb = (struct urb *) ptr;
	local_irq_save (flags);
	spin_lock (&urb->lock);

	/* do nothing if the hc is gone or the urb's been unlinked */
	if (!urb->dev
			|| urb->status != -EINPROGRESS
			|| (hcd = urb->dev->bus->hcpriv) == 0
			|| !HCD_IS_RUNNING (hcd->state)) {
		spin_unlock (&urb->lock);
		local_irq_restore (flags);
		return;
	}

	length = hcd->driver->hub_status_data (hcd, urb->transfer_buffer);

	/* complete the status urb, or retrigger the timer */
	spin_lock (&hcd_data_lock);
	if (length > 0) {
		hcd->rh_timer.data = 0;
		urb->actual_length = length;
		urb->status = 0;
		urb->hcpriv = 0;
	} else
		mod_timer (&hcd->rh_timer, jiffies + HZ/4);
	spin_unlock (&hcd_data_lock);
	spin_unlock (&urb->lock);

	/* local irqs are always blocked in completions */
	if (length > 0)
		usb_hcd_giveback_urb (hcd, urb, NULL);
	local_irq_restore (flags);
}

/*-------------------------------------------------------------------------*/

static int rh_urb_enqueue (struct usb_hcd *hcd, struct urb *urb)
{
	if (usb_pipeint (urb->pipe)) {
		int		retval;
		unsigned long	flags;

		spin_lock_irqsave (&hcd_data_lock, flags);
		retval = rh_status_urb (hcd, urb);
		spin_unlock_irqrestore (&hcd_data_lock, flags);
		return retval;
	}
	if (usb_pipecontrol (urb->pipe))
		return rh_call_control (hcd, urb);
	else
		return -EINVAL;
}

/*-------------------------------------------------------------------------*/

void usb_rh_status_dequeue (struct usb_hcd *hcd, struct urb *urb)
{
	//unsigned long	flags;

	/* note:  always a synchronous unlink */
	del_timer_sync (&hcd->rh_timer);
	hcd->rh_timer.data = 0;

	local_irq_save (flags);
	urb->hcpriv = 0;
	usb_hcd_giveback_urb (hcd, urb, NULL);
	local_irq_restore (flags);
}

/*-------------------------------------------------------------------------*/

/* exported only within usbcore */
void usb_bus_get (struct usb_bus *bus)
{
	atomic_inc (&bus->refcnt);
}

/* exported only within usbcore */
void usb_bus_put (struct usb_bus *bus)
{
	if (atomic_dec_and_test (&bus->refcnt))
		kfree (bus);
}

/*-------------------------------------------------------------------------*/

/**
 * usb_bus_init - shared initialization code
 * @bus: the bus structure being initialized
 *
 * This code is used to initialize a usb_bus structure, memory for which is
 * separately managed.
 */
void usb_bus_init (struct usb_bus *bus)
{
	memset (&bus->devmap, 0, sizeof(struct usb_devmap));

	bus->devnum_next = 1;

	bus->root_hub = NULL;
	bus->hcpriv = NULL;
	bus->busnum = -1;
	bus->bandwidth_allocated = 0;
	bus->bandwidth_int_reqs  = 0;
	bus->bandwidth_isoc_reqs = 0;

	INIT_LIST_HEAD (&bus->bus_list);

	atomic_set (&bus->refcnt, 1);
}
EXPORT_SYMBOL (usb_bus_init);

/**
 * usb_alloc_bus - creates a new USB host controller structure
 * @op: pointer to a struct usb_operations that this bus structure should use
 * Context: !in_interrupt()
 *
 * Creates a USB host controller bus structure with the specified 
 * usb_operations and initializes all the necessary internal objects.
 *
 * If no memory is available, NULL is returned.
 *
 * The caller should call usb_free_bus() when it is finished with the structure.
 */
struct usb_bus *usb_alloc_bus (struct usb_operations *op)
{
	struct usb_bus *bus;

	bus = kmalloc (sizeof *bus, GFP_KERNEL);
	if (!bus)
		return NULL;
	usb_bus_init (bus);
	bus->op = op;
	return bus;
}
EXPORT_SYMBOL (usb_alloc_bus);

/**
 * usb_free_bus - frees the memory used by a bus structure
 * @bus: pointer to the bus to free
 *
 * To be invoked by a HCD, only as the last step of decoupling from
 * hardware.  It is an error to call this if the reference count is
 * anything but one.  That would indicate that some system component
 * did not correctly shut down, and thought the hardware was still
 * accessible.
 */
void usb_free_bus (struct usb_bus *bus)
{
	if (!bus)
		return;
	if (atomic_read (&bus->refcnt) != 1)
		err ("usb_free_bus #%d, count != 1", bus->busnum);
	usb_bus_put (bus);
}
EXPORT_SYMBOL (usb_free_bus);

/*-------------------------------------------------------------------------*/

/**
 * usb_register_bus - registers the USB host controller with the usb core
 * @bus: pointer to the bus to register
 * Context: !in_interrupt()
 *
 * Assigns a bus number, and links the controller into usbcore data
 * structures so that it can be seen by scanning the bus list.
 */
void usb_register_bus(struct usb_bus *bus)
{
	int busnum;

	down (&usb_bus_list_lock);
	busnum = find_next_zero_bit (busmap.busmap, USB_MAXBUS, 1);
	if (busnum < USB_MAXBUS) {
		set_bit (busnum, busmap.busmap);
		bus->busnum = busnum;
	} else
		warn ("too many buses");

	usb_bus_get (bus);

	/* Add it to the list of buses */
	list_add (&bus->bus_list, &usb_bus_list);
	up (&usb_bus_list_lock);

	usbfs_add_bus (bus);

	dev_info (bus->controller, "new USB bus registered, assigned bus number %d\n", bus->busnum);
}
EXPORT_SYMBOL (usb_register_bus);

/**
 * usb_deregister_bus - deregisters the USB host controller
 * @bus: pointer to the bus to deregister
 * Context: !in_interrupt()
 *
 * Recycles the bus number, and unlinks the controller from usbcore data
 * structures so that it won't be seen by scanning the bus list.
 */
void usb_deregister_bus (struct usb_bus *bus)
{
	dev_info (bus->controller, "USB bus %d deregistered\n", bus->busnum);

	/*
	 * NOTE: make sure that all the devices are removed by the
	 * controller code, as well as having it call this when cleaning
	 * itself up
	 */
	down (&usb_bus_list_lock);
	list_del (&bus->bus_list);
	up (&usb_bus_list_lock);

	usbfs_remove_bus (bus);

	clear_bit (bus->busnum, busmap.busmap);

	usb_bus_put (bus);
}
EXPORT_SYMBOL (usb_deregister_bus);

/**
 * usb_register_root_hub - called by HCD to register its root hub 
 * @usb_dev: the usb root hub device to be registered.
 * @parent_dev: the parent device of this root hub.
 *
 * The USB host controller calls this function to register the root hub
 * properly with the USB subsystem.  It sets up the device properly in
 * the driverfs tree, and then calls usb_new_device() to register the
 * usb device.
 */
int usb_register_root_hub (struct usb_device *usb_dev, struct device *parent_dev)
{
	int retval;

	sprintf (&usb_dev->dev.bus_id[0], "usb%d", usb_dev->bus->busnum);
	usb_dev->state = USB_STATE_DEFAULT;
	retval = usb_new_device (usb_dev, parent_dev);
	if (retval)
		dev_err (parent_dev, "can't register root hub for %s, %d\n",
				usb_dev->dev.bus_id, retval);
	return retval;
}
EXPORT_SYMBOL (usb_register_root_hub);


/*-------------------------------------------------------------------------*/

/**
 * usb_calc_bus_time - approximate periodic transaction time in nanoseconds
 * @speed: from dev->speed; USB_SPEED_{LOW,FULL,HIGH}
 * @is_input: true iff the transaction sends data to the host
 * @isoc: true for isochronous transactions, false for interrupt ones
 * @bytecount: how many bytes in the transaction.
 *
 * Returns approximate bus time in nanoseconds for a periodic transaction.
 * See USB 2.0 spec section 5.11.3; only periodic transfers need to be
 * scheduled in software, this function is only used for such scheduling.
 */
long usb_calc_bus_time (int speed, int is_input, int isoc, int bytecount)
{
	unsigned long	tmp;

	switch (speed) {
	case USB_SPEED_LOW: 	/* INTR only */
		if (is_input) {
			tmp = (67667L * (31L + 10L * BitTime (bytecount))) / 1000L;
			return (64060L + (2 * BW_HUB_LS_SETUP) + BW_HOST_DELAY + tmp);
		} else {
			tmp = (66700L * (31L + 10L * BitTime (bytecount))) / 1000L;
			return (64107L + (2 * BW_HUB_LS_SETUP) + BW_HOST_DELAY + tmp);
		}
	case USB_SPEED_FULL:	/* ISOC or INTR */
		if (isoc) {
			tmp = (8354L * (31L + 10L * BitTime (bytecount))) / 1000L;
			return (((is_input) ? 7268L : 6265L) + BW_HOST_DELAY + tmp);
		} else {
			tmp = (8354L * (31L + 10L * BitTime (bytecount))) / 1000L;
			return (9107L + BW_HOST_DELAY + tmp);
		}
	case USB_SPEED_HIGH:	/* ISOC or INTR */
		// FIXME adjust for input vs output
		if (isoc)
			tmp = HS_USECS (bytecount);
		else
			tmp = HS_USECS_ISO (bytecount);
		return tmp;
	default:
		dbg ("bogus device speed!");
		return -1;
	}
}
EXPORT_SYMBOL (usb_calc_bus_time);

/*
 * usb_check_bandwidth():
 *
 * old_alloc is from host_controller->bandwidth_allocated in microseconds;
 * bustime is from calc_bus_time(), but converted to microseconds.
 *
 * returns <bustime in us> if successful,
 * or -ENOSPC if bandwidth request fails.
 *
 * FIXME:
 * This initial implementation does not use Endpoint.bInterval
 * in managing bandwidth allocation.
 * It probably needs to be expanded to use Endpoint.bInterval.
 * This can be done as a later enhancement (correction).
 *
 * This will also probably require some kind of
 * frame allocation tracking...meaning, for example,
 * that if multiple drivers request interrupts every 10 USB frames,
 * they don't all have to be allocated at
 * frame numbers N, N+10, N+20, etc.  Some of them could be at
 * N+11, N+21, N+31, etc., and others at
 * N+12, N+22, N+32, etc.
 *
 * Similarly for isochronous transfers...
 *
 * Individual HCDs can schedule more directly ... this logic
 * is not correct for high speed transfers.
 */
int usb_check_bandwidth (struct usb_device *dev, struct urb *urb)
{
	unsigned int	pipe = urb->pipe;
	long		bustime;
	int		is_in = usb_pipein (pipe);
	int		is_iso = usb_pipeisoc (pipe);
	int		old_alloc = dev->bus->bandwidth_allocated;
	int		new_alloc;


	bustime = NS_TO_US (usb_calc_bus_time (dev->speed, is_in, is_iso,
			usb_maxpacket (dev, pipe, !is_in)));
	if (is_iso)
		bustime /= urb->number_of_packets;

	new_alloc = old_alloc + (int) bustime;
	if (new_alloc > FRAME_TIME_MAX_USECS_ALLOC) {
#ifdef	DEBUG
		char	*mode = 
#ifdef CONFIG_USB_BANDWIDTH
			"";
#else
			"would have ";
#endif
		dev_dbg (&dev->dev, "usb_check_bandwidth %sFAILED: %d + %ld = %d usec\n",
			mode, old_alloc, bustime, new_alloc);
#endif
#ifdef CONFIG_USB_BANDWIDTH
		bustime = -ENOSPC;	/* report error */
#endif
	}

	return bustime;
}
EXPORT_SYMBOL (usb_check_bandwidth);


/**
 * usb_claim_bandwidth - records bandwidth for a periodic transfer
 * @dev: source/target of request
 * @urb: request (urb->dev == dev)
 * @bustime: bandwidth consumed, in (average) microseconds per frame
 * @isoc: true iff the request is isochronous
 *
 * Bus bandwidth reservations are recorded purely for diagnostic purposes.
 * HCDs are expected not to overcommit periodic bandwidth, and to record such
 * reservations whenever endpoints are added to the periodic schedule.
 *
 * FIXME averaging per-frame is suboptimal.  Better to sum over the HCD's
 * entire periodic schedule ... 32 frames for OHCI, 1024 for UHCI, settable
 * for EHCI (256/512/1024 frames, default 1024) and have the bus expose how
 * large its periodic schedule is.
 */
void usb_claim_bandwidth (struct usb_device *dev, struct urb *urb, int bustime, int isoc)
{
	dev->bus->bandwidth_allocated += bustime;
	if (isoc)
		dev->bus->bandwidth_isoc_reqs++;
	else
		dev->bus->bandwidth_int_reqs++;
	urb->bandwidth = bustime;

#ifdef USB_BANDWIDTH_MESSAGES
	dev_dbg (&dev->dev, "bandwidth alloc increased by %d (%s) to %d for %d requesters\n",
		bustime,
		isoc ? "ISOC" : "INTR",
		dev->bus->bandwidth_allocated,
		dev->bus->bandwidth_int_reqs + dev->bus->bandwidth_isoc_reqs);
#endif
}
EXPORT_SYMBOL (usb_claim_bandwidth);


/**
 * usb_release_bandwidth - reverses effect of usb_claim_bandwidth()
 * @dev: source/target of request
 * @urb: request (urb->dev == dev)
 * @isoc: true iff the request is isochronous
 *
 * This records that previously allocated bandwidth has been released.
 * Bandwidth is released when endpoints are removed from the host controller's
 * periodic schedule.
 */
void usb_release_bandwidth (struct usb_device *dev, struct urb *urb, int isoc)
{
	dev->bus->bandwidth_allocated -= urb->bandwidth;
	if (isoc)
		dev->bus->bandwidth_isoc_reqs--;
	else
		dev->bus->bandwidth_int_reqs--;

#ifdef USB_BANDWIDTH_MESSAGES
	dev_dbg (&dev->dev, "bandwidth alloc reduced by %d (%s) to %d for %d requesters\n",
		urb->bandwidth,
		isoc ? "ISOC" : "INTR",
		dev->bus->bandwidth_allocated,
		dev->bus->bandwidth_int_reqs + dev->bus->bandwidth_isoc_reqs);
#endif
	urb->bandwidth = 0;
}
EXPORT_SYMBOL (usb_release_bandwidth);


/*-------------------------------------------------------------------------*/

/*
 * Generic HC operations.
 */

/*-------------------------------------------------------------------------*/

/* called from khubd, or root hub init threads for hcd-private init */
static int hcd_alloc_dev (struct usb_device *udev)
{
	struct hcd_dev		*dev;
	struct usb_hcd		*hcd;
	unsigned long		flags;

	if (!udev || udev->hcpriv)
		return -EINVAL;
	if (!udev->bus || !udev->bus->hcpriv)
		return -ENODEV;
	hcd = udev->bus->hcpriv;
	if (hcd->state == USB_STATE_QUIESCING)
		return -ENOLINK;

	dev = (struct hcd_dev *) kmalloc (sizeof *dev, GFP_KERNEL);
	if (dev == NULL)
		return -ENOMEM;
	memset (dev, 0, sizeof *dev);

	INIT_LIST_HEAD (&dev->dev_list);
	INIT_LIST_HEAD (&dev->urb_list);

	spin_lock_irqsave (&hcd_data_lock, flags);
	list_add (&dev->dev_list, &hcd->dev_list);
	// refcount is implicit
	udev->hcpriv = dev;
	spin_unlock_irqrestore (&hcd_data_lock, flags);

	return 0;
}

/*-------------------------------------------------------------------------*/

static void urb_unlink (struct urb *urb)
{
	unsigned long		flags;
	struct usb_device	*dev;

	/* Release any periodic transfer bandwidth */
	if (urb->bandwidth)
		usb_release_bandwidth (urb->dev, urb,
			usb_pipeisoc (urb->pipe));

	/* clear all state linking urb to this dev (and hcd) */

	spin_lock_irqsave (&hcd_data_lock, flags);
	list_del_init (&urb->urb_list);
	dev = urb->dev;
	spin_unlock_irqrestore (&hcd_data_lock, flags);
	usb_put_dev (dev);
}


/* may be called in any context with a valid urb->dev usecount
 * caller surrenders "ownership" of urb
 * expects usb_submit_urb() to have sanity checked and conditioned all
 * inputs in the urb
 */
static int hcd_submit_urb (struct urb *urb, int mem_flags)
{
	int			status;
	struct usb_hcd		*hcd = urb->dev->bus->hcpriv;
	struct hcd_dev		*dev = urb->dev->hcpriv;
	unsigned long		flags;
	

	if (!hcd || !dev)
		return -ENODEV;
//	printk("submit_urb %p, # %i, t %i\n",urb,urb->dev->devnum,usb_pipetype(urb->pipe));
	/*
	 * FIXME:  make urb timeouts be generic, keeping the HCD cores
	 * as simple as possible.
	 */

	// NOTE:  a generic device/urb monitoring hook would go here.
	// hcd_monitor_hook(MONITOR_URB_SUBMIT, urb)
	// It would catch submission paths for all urbs.

	/*
	 * Atomically queue the urb,  first to our records, then to the HCD.
	 * Access to urb->status is controlled by urb->lock ... changes on
	 * i/o completion (normal or fault) or unlinking.
	 */

	// FIXME:  verify that quiescing hc works right (RH cleans up)

	spin_lock_irqsave (&hcd_data_lock, flags);
	if (HCD_IS_RUNNING (hcd->state) && hcd->state != USB_STATE_QUIESCING) {
		usb_get_dev (urb->dev);
		list_add_tail (&urb->urb_list, &dev->urb_list);
		status = 0;
	} else {
		INIT_LIST_HEAD (&urb->urb_list);
		status = -ESHUTDOWN;
	}
	spin_unlock_irqrestore (&hcd_data_lock, flags);
	if (status)
		return status;

	/* increment urb's reference count as part of giving it to the HCD
	 * (which now controls it).  HCD guarantees that it either returns
	 * an error or calls giveback(), but not both.
	 */

	urb = usb_get_urb (urb);
	if (urb->dev == hcd->self.root_hub) {
		/* NOTE:  requirement on hub callers (usbfs and the hub
		 * driver, for now) that URBs' urb->transfer_buffer be
		 * valid and usb_buffer_{sync,unmap}() not be needed, since
		 * they could clobber root hub response data.
		 */
		urb->transfer_flags |= URB_NO_DMA_MAP;
		status = rh_urb_enqueue (hcd, urb);
		goto done;
	}

	/* lower level hcd code should use *_dma exclusively,
	 * unless it uses pio or talks to another transport.
	 */
	if (!(urb->transfer_flags & URB_NO_DMA_MAP)
			&& hcd->controller->dma_mask) {
		if (usb_pipecontrol (urb->pipe))
			urb->setup_dma = dma_map_single (
					hcd->controller,
					urb->setup_packet,
					sizeof (struct usb_ctrlrequest),
					DMA_TO_DEVICE);
		if (urb->transfer_buffer_length != 0)
			urb->transfer_dma = dma_map_single (
					hcd->controller,
					urb->transfer_buffer,
					urb->transfer_buffer_length,
					usb_pipein (urb->pipe)
					    ? DMA_FROM_DEVICE
					    : DMA_TO_DEVICE);
	}

	status = hcd->driver->urb_enqueue (hcd, urb, mem_flags);
done:
	if (status) {
		usb_put_urb (urb);
		urb_unlink (urb);
	}
	return status;
}

/*-------------------------------------------------------------------------*/

/* called in any context */
static int hcd_get_frame_number (struct usb_device *udev)
{
	struct usb_hcd	*hcd = (struct usb_hcd *)udev->bus->hcpriv;
	return hcd->driver->get_frame_number (hcd);
}

/*-------------------------------------------------------------------------*/

/* this makes the hcd giveback() the urb more quickly, by kicking it
 * off hardware queues (which may take a while) and returning it as
 * soon as practical.  we've already set up the urb's return status,
 * but we can't know if the callback completed already.
 */
static void
unlink1 (struct usb_hcd *hcd, struct urb *urb)
{
	if (urb == (struct urb *) hcd->rh_timer.data)
		usb_rh_status_dequeue (hcd, urb);
	else {
		int		value;

		/* failures "should" be harmless */
		value = hcd->driver->urb_dequeue (hcd, urb);
		if (value != 0)
			dev_dbg (hcd->controller,
				"dequeue %p --> %d\n",
				urb, value);
	}
}

struct completion_splice {		// modified urb context:
	/* did we complete? */
	struct completion	done;

	/* original urb data */
	usb_complete_t		complete;
	void			*context;
};

static void unlink_complete (struct urb *urb, struct pt_regs *regs)
{
	struct completion_splice	*splice;

	splice = (struct completion_splice *) urb->context;

	/* issue original completion call */
	urb->complete = splice->complete;
	urb->context = splice->context;
	urb->complete (urb, regs);

	/* then let the synchronous unlink call complete */
	complete (&splice->done);
}

/*
 * called in any context; note ASYNC_UNLINK restrictions
 *
 * caller guarantees urb won't be recycled till both unlink()
 * and the urb's completion function return
 */
static int hcd_unlink_urb (struct urb *urb)
{
	struct hcd_dev			*dev;
	struct usb_hcd			*hcd = 0;
	struct device			*sys = 0;
	unsigned long			flags;
	struct completion_splice	splice;
	int				retval;

	if (!urb)
		return -EINVAL;

	/*
	 * we contend for urb->status with the hcd core,
	 * which changes it while returning the urb.
	 *
	 * Caller guaranteed that the urb pointer hasn't been freed, and
	 * that it was submitted.  But as a rule it can't know whether or
	 * not it's already been unlinked ... so we respect the reversed
	 * lock sequence needed for the usb_hcd_giveback_urb() code paths
	 * (urb lock, then hcd_data_lock) in case some other CPU is now
	 * unlinking it.
	 */
	spin_lock_irqsave (&urb->lock, flags);
	spin_lock (&hcd_data_lock);

	if (!urb->dev || !urb->dev->bus) {
		retval = -ENODEV;
		goto done;
	}

	dev = urb->dev->hcpriv;
	sys = &urb->dev->dev;
	hcd = urb->dev->bus->hcpriv;
	if (!dev || !hcd) {
		retval = -ENODEV;
		goto done;
	}

	if (!urb->hcpriv) {
		retval = -EINVAL;
		goto done;
	}

	/* Any status except -EINPROGRESS means something already started to
	 * unlink this URB from the hardware.  So there's no more work to do.
	 *
	 * FIXME use better explicit urb state
	 */
	if (urb->status != -EINPROGRESS) {
		retval = -EBUSY;
		goto done;
	}

	/* maybe set up to block until the urb's completion fires.  the
	 * lower level hcd code is always async, locking on urb->status
	 * updates; an intercepted completion unblocks us.
	 */
	if (!(urb->transfer_flags & URB_ASYNC_UNLINK)) {
		if (in_interrupt ()) {
			dev_dbg (hcd->controller, "non-async unlink in_interrupt");
			retval = -EWOULDBLOCK;
			goto done;
		}
		/* synchronous unlink: block till we see the completion */
		init_completion (&splice.done);
		splice.complete = urb->complete;
		splice.context = urb->context;
		urb->complete = unlink_complete;
		urb->context = &splice;
		urb->status = -ENOENT;
	} else {
		/* asynchronous unlink */
		urb->status = -ECONNRESET;
	}
	spin_unlock (&hcd_data_lock);
	spin_unlock_irqrestore (&urb->lock, flags);

	// FIXME remove splicing, so this becomes unlink1 (hcd, urb);
	if (urb == (struct urb *) hcd->rh_timer.data) {
		usb_rh_status_dequeue (hcd, urb);
		retval = 0;
	} else {
		retval = hcd->driver->urb_dequeue (hcd, urb);

		/* hcds shouldn't really fail these calls, but... */
		if (retval) {
			dev_dbg (sys, "dequeue %p --> %d\n", urb, retval);
			if (!(urb->transfer_flags & URB_ASYNC_UNLINK)) {
				spin_lock_irqsave (&urb->lock, flags);
				urb->complete = splice.complete;
				urb->context = splice.context;
				spin_unlock_irqrestore (&urb->lock, flags);
			}
			goto bye;
		}
	}

    	/* block till giveback, if needed */
	if (urb->transfer_flags & URB_ASYNC_UNLINK)
		return -EINPROGRESS;

	wait_for_completion (&splice.done);
	return 0;

done:
	spin_unlock (&hcd_data_lock);
	spin_unlock_irqrestore (&urb->lock, flags);
bye:
	if (retval && sys && sys->driver)
		dev_dbg (sys, "hcd_unlink_urb %p fail %d\n", urb, retval);
	return retval;
}

/*-------------------------------------------------------------------------*/

/* disables the endpoint: cancels any pending urbs, then synchronizes with
 * the hcd to make sure all endpoint state is gone from hardware. use for
 * set_configuration, set_interface, driver removal, physical disconnect.
 *
 * example:  a qh stored in hcd_dev.ep[], holding state related to endpoint
 * type, maxpacket size, toggle, halt status, and scheduling.
 */
static void hcd_endpoint_disable (struct usb_device *udev, int endpoint)
{
	unsigned long	flags;
	struct hcd_dev	*dev;
	struct usb_hcd	*hcd;
	struct urb	*urb;
	unsigned	epnum = endpoint & USB_ENDPOINT_NUMBER_MASK;

	dev = udev->hcpriv;
	hcd = udev->bus->hcpriv;

rescan:
	/* (re)block new requests, as best we can */
	if (endpoint & USB_DIR_IN) {
		usb_endpoint_halt (udev, epnum, 0);
		udev->epmaxpacketin [epnum] = 0;
	} else {
		usb_endpoint_halt (udev, epnum, 1);
		udev->epmaxpacketout [epnum] = 0;
	}

	/* then kill any current requests */
	spin_lock_irqsave (&hcd_data_lock, flags);
	list_for_each_entry (urb, &dev->urb_list, urb_list) {
		int	tmp = urb->pipe;

		/* ignore urbs for other endpoints */
		if (usb_pipeendpoint (tmp) != epnum)
			continue;
		if ((tmp ^ endpoint) & USB_DIR_IN)
			continue;

		/* another cpu may be in hcd, spinning on hcd_data_lock
		 * to giveback() this urb.  the races here should be
		 * small, but a full fix needs a new "can't submit"
		 * urb state.
		 */
		if (urb->status != -EINPROGRESS)
			continue;
		usb_get_urb (urb);
		spin_unlock_irqrestore (&hcd_data_lock, flags);

		spin_lock_irqsave (&urb->lock, flags);
		tmp = urb->status;
		if (tmp == -EINPROGRESS)
			urb->status = -ESHUTDOWN;
		spin_unlock_irqrestore (&urb->lock, flags);

		/* kick hcd unless it's already returning this */
		if (tmp == -EINPROGRESS) {
			tmp = urb->pipe;
			unlink1 (hcd, urb);
			dev_dbg (hcd->controller,
				"shutdown urb %p pipe %08x ep%d%s%s\n",
				urb, tmp, usb_pipeendpoint (tmp),
				(tmp & USB_DIR_IN) ? "in" : "out",
				({ char *s; \
				 switch (usb_pipetype (tmp)) { \
				 case PIPE_CONTROL:	s = ""; break; \
				 case PIPE_BULK:	s = "-bulk"; break; \
				 case PIPE_INTERRUPT:	s = "-intr"; break; \
				 default: 		s = "-iso"; break; \
				}; s;}));
		}
		usb_put_urb (urb);

		/* list contents may have changed */
		goto rescan;
	}
	spin_unlock_irqrestore (&hcd_data_lock, flags);

	/* synchronize with the hardware, so old configuration state
	 * clears out immediately (and will be freed).
	 */
	might_sleep ();
	if (hcd->driver->endpoint_disable)
		hcd->driver->endpoint_disable (hcd, dev, endpoint);
}

/*-------------------------------------------------------------------------*/

/* called by khubd, rmmod, apmd, or other thread for hcd-private cleanup.
 * we're guaranteed that the device is fully quiesced.  also, that each
 * endpoint has been hcd_endpoint_disabled.
 */

static int hcd_free_dev (struct usb_device *udev)
{
	struct hcd_dev		*dev;
	struct usb_hcd		*hcd;
	unsigned long		flags;

	if (!udev || !udev->hcpriv)
		return -EINVAL;

	if (!udev->bus || !udev->bus->hcpriv)
		return -ENODEV;

	// should udev->devnum == -1 ??

	dev = udev->hcpriv;
	hcd = udev->bus->hcpriv;

	/* device driver problem with refcounts? */
	if (!list_empty (&dev->urb_list)) {
		dev_dbg (hcd->controller, "free busy dev, %s devnum %d (bug!)\n",
			hcd->self.bus_name, udev->devnum);
		return -EINVAL;
	}

	spin_lock_irqsave (&hcd_data_lock, flags);
	list_del (&dev->dev_list);
	udev->hcpriv = NULL;
	spin_unlock_irqrestore (&hcd_data_lock, flags);

	kfree (dev);
	return 0;
}

/*
 * usb_hcd_operations - adapts usb_bus framework to HCD framework (bus glue)
 *
 * When registering a USB bus through the HCD framework code, use this
 * usb_operations vector.  The PCI glue layer does so automatically; only
 * bus glue for non-PCI system busses will need to use this.
 */
struct usb_operations usb_hcd_operations = {
	.allocate =		hcd_alloc_dev,
	.get_frame_number =	hcd_get_frame_number,
	.submit_urb =		hcd_submit_urb,
	.unlink_urb =		hcd_unlink_urb,
	.deallocate =		hcd_free_dev,
	.buffer_alloc =		hcd_buffer_alloc,
	.buffer_free =		hcd_buffer_free,
	.disable =		hcd_endpoint_disable,
};
EXPORT_SYMBOL (usb_hcd_operations);

/*-------------------------------------------------------------------------*/

/**
 * usb_hcd_giveback_urb - return URB from HCD to device driver
 * @hcd: host controller returning the URB
 * @urb: urb being returned to the USB device driver.
 * @regs: pt_regs, passed down to the URB completion handler
 * Context: in_interrupt()
 *
 * This hands the URB from HCD to its USB device driver, using its
 * completion function.  The HCD has freed all per-urb resources
 * (and is done using urb->hcpriv).  It also released all HCD locks;
 * the device driver won't cause problems if it frees, modifies,
 * or resubmits this URB.
 */
void usb_hcd_giveback_urb (struct usb_hcd *hcd, struct urb *urb, struct pt_regs *regs)
{
	urb_unlink (urb);

	// NOTE:  a generic device/urb monitoring hook would go here.
	// hcd_monitor_hook(MONITOR_URB_FINISH, urb, dev)
	// It would catch exit/unlink paths for all urbs.

	/* lower level hcd code should use *_dma exclusively */
	if (!(urb->transfer_flags & URB_NO_DMA_MAP)) {
		if (usb_pipecontrol (urb->pipe))
			pci_unmap_single (hcd->pdev, urb->setup_dma,
					sizeof (struct usb_ctrlrequest),
					PCI_DMA_TODEVICE);
		if (urb->transfer_buffer_length != 0)
			pci_unmap_single (hcd->pdev, urb->transfer_dma,
					urb->transfer_buffer_length,
					usb_pipein (urb->pipe)
					    ? PCI_DMA_FROMDEVICE
					    : PCI_DMA_TODEVICE);
	}

	/* pass ownership to the completion handler */
	urb->complete (urb, regs);
	usb_put_urb (urb);
}
EXPORT_SYMBOL (usb_hcd_giveback_urb);

/*-------------------------------------------------------------------------*/

/**
 * usb_hcd_irq - hook IRQs to HCD framework (bus glue)
 * @irq: the IRQ being raised
 * @__hcd: pointer to the HCD whose IRQ is beinng signaled
 * @r: saved hardware registers
 *
 * When registering a USB bus through the HCD framework code, use this
 * to handle interrupts.  The PCI glue layer does so automatically; only
 * bus glue for non-PCI system busses will need to use this.
 */
irqreturn_t usb_hcd_irq (int irq, void *__hcd, struct pt_regs * r)
{
	struct usb_hcd		*hcd = __hcd;
	int			start = hcd->state;

	if (unlikely (hcd->state == USB_STATE_HALT))	/* irq sharing? */
		return IRQ_NONE;

	hcd->driver->irq (hcd, r);
	if (hcd->state != start && hcd->state == USB_STATE_HALT)
		usb_hc_died (hcd);
	return IRQ_HANDLED;
}
EXPORT_SYMBOL (usb_hcd_irq);

/*-------------------------------------------------------------------------*/

static void hcd_panic (void *_hcd)
{
	struct usb_hcd *hcd = _hcd;
	hcd->driver->stop (hcd);
}

/**
 * usb_hc_died - report abnormal shutdown of a host controller (bus glue)
 * @hcd: pointer to the HCD representing the controller
 *
 * This is called by bus glue to report a USB host controller that died
 * while operations may still have been pending.  It's called automatically
 * by the PCI glue, so only glue for non-PCI busses should need to call it. 
 */
void usb_hc_died (struct usb_hcd *hcd)
{
	struct list_head	*devlist, *urblist;
	struct hcd_dev		*dev;
	struct urb		*urb;
	unsigned long		flags;
	
	/* flag every pending urb as done */
	spin_lock_irqsave (&hcd_data_lock, flags);
	list_for_each (devlist, &hcd->dev_list) {
		dev = list_entry (devlist, struct hcd_dev, dev_list);
		list_for_each (urblist, &dev->urb_list) {
			urb = list_entry (urblist, struct urb, urb_list);
			dev_dbg (hcd->controller, "shutdown %s urb %p pipe %x, current status %d\n",
				hcd->self.bus_name, urb, urb->pipe, urb->status);
			if (urb->status == -EINPROGRESS)
				urb->status = -ESHUTDOWN;
		}
	}
	urb = (struct urb *) hcd->rh_timer.data;
	if (urb)
		urb->status = -ESHUTDOWN;
	spin_unlock_irqrestore (&hcd_data_lock, flags);

	/* hcd->stop() needs a task context */
	INIT_WORK (&hcd->work, hcd_panic, hcd);
	(void) schedule_work (&hcd->work);
}
EXPORT_SYMBOL (usb_hc_died);

