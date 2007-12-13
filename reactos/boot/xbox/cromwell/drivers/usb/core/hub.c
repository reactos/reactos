/*
 * USB hub driver.
 *
 * (C) Copyright 1999 Linus Torvalds
 * (C) Copyright 1999 Johannes Erdfelt
 * (C) Copyright 1999 Gregory P. Smith
 * (C) Copyright 2001 Brad Hards (bhards@bigpond.net.au)
 *
 */
#define DEBUG
#if 0
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/completion.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/ioctl.h>
#ifdef CONFIG_USB_DEBUG
	#define DEBUG
#else
	#undef DEBUG
#endif
#include <linux/usb.h>
#include <linux/usbdevice_fs.h>
#include <linux/suspend.h>

#include <asm/semaphore.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>

#include "hcd.h"
#include "hub.h"
#else
#include "../usb_wrapper.h"
#include "hcd.h"
#include "hub.h"
#define DEBUG
#endif

/* Wakes up khubd */
static spinlock_t hub_event_lock = SPIN_LOCK_UNLOCKED;
static DECLARE_MUTEX(usb_address0_sem);

static LIST_HEAD(hub_event_list);	/* List of hubs needing servicing */
static LIST_HEAD(hub_list);		/* List of all hubs (for cleanup) */

static DECLARE_WAIT_QUEUE_HEAD(khubd_wait);
static pid_t khubd_pid = 0;			/* PID of khubd */
static DECLARE_COMPLETION(khubd_exited);

#ifdef	DEBUG
static inline char *portspeed (int portstatus)
{
	if (portstatus & (1 << USB_PORT_FEAT_HIGHSPEED))
    		return "480 Mb/s";
	else if (portstatus & (1 << USB_PORT_FEAT_LOWSPEED))
		return "1.5 Mb/s";
	else
		return "12 Mb/s";
}
#endif

/* for dev_info, dev_dbg, etc */
static inline struct device *hubdev (struct usb_device *dev)
{
	return &dev->actconfig->interface [0].dev;
}

/* USB 2.0 spec Section 11.24.4.5 */
static int get_hub_descriptor(struct usb_device *dev, void *data, int size)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
		USB_REQ_GET_DESCRIPTOR, USB_DIR_IN | USB_RT_HUB,
		USB_DT_HUB << 8, 0, data, size, HZ * USB_CTRL_GET_TIMEOUT);
}

/*
 * USB 2.0 spec Section 11.24.2.1
 */
static int clear_hub_feature(struct usb_device *dev, int feature)
{
	return usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
		USB_REQ_CLEAR_FEATURE, USB_RT_HUB, feature, 0, NULL, 0, HZ);
}

/*
 * USB 2.0 spec Section 11.24.2.2
 * BUG: doesn't handle port indicator selector in high byte of wIndex
 */
static int clear_port_feature(struct usb_device *dev, int port, int feature)
{
	return usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
		USB_REQ_CLEAR_FEATURE, USB_RT_PORT, feature, port, NULL, 0, HZ);
}

/*
 * USB 2.0 spec Section 11.24.2.13
 * BUG: doesn't handle port indicator selector in high byte of wIndex
 */
static int set_port_feature(struct usb_device *dev, int port, int feature)
{
	return usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
		USB_REQ_SET_FEATURE, USB_RT_PORT, feature, port, NULL, 0, HZ);
}

/*
 * USB 2.0 spec Section 11.24.2.6
 */
static int get_hub_status(struct usb_device *dev,
		struct usb_hub_status *data)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
		USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_HUB, 0, 0,
		data, sizeof(*data), HZ * USB_CTRL_GET_TIMEOUT);
}

/*
 * USB 2.0 spec Section 11.24.2.7
 */
static int get_port_status(struct usb_device *dev, int port,
		struct usb_port_status *data)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
		USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_PORT, 0, port,
		data, sizeof(*data), HZ * USB_CTRL_GET_TIMEOUT);
}

/* completion function, fires on port status changes and various faults */
static void hub_irq(struct urb *urb, struct pt_regs *regs)
{
	struct usb_hub *hub = (struct usb_hub *)urb->context;
	unsigned long flags;
	int status;

	switch (urb->status) {
	case -ENOENT:		/* synchronous unlink */
	case -ECONNRESET:	/* async unlink */
	case -ESHUTDOWN:	/* hardware going away */
		return;

	default:		/* presumably an error */
		/* Cause a hub reset after 10 consecutive errors */
		dev_dbg (&hub->intf->dev, "transfer --> %d\n", urb->status);
		if ((++hub->nerrors < 10) || hub->error)
			goto resubmit;
		hub->error = urb->status;
		/* FALL THROUGH */
	
	/* let khubd handle things */
	case 0:			/* we got data:  port status changed */
		break;
	}

	hub->nerrors = 0;

	/* Something happened, let khubd figure it out */
	spin_lock_irqsave(&hub_event_lock, flags);
	if (list_empty(&hub->event_list)) {
		list_add(&hub->event_list, &hub_event_list);
		wake_up(&khubd_wait);
	}
	spin_unlock_irqrestore(&hub_event_lock, flags);

resubmit:
	if ((status = usb_submit_urb (hub->urb, GFP_ATOMIC)) != 0
			/* ENODEV means we raced disconnect() */
			&& status != -ENODEV)
		dev_err (&hub->intf->dev, "resubmit --> %d\n", urb->status);
}

/* USB 2.0 spec Section 11.24.2.3 */
static inline int
hub_clear_tt_buffer (struct usb_device *hub, u16 devinfo, u16 tt)
{
	return usb_control_msg (hub, usb_rcvctrlpipe (hub, 0),
		HUB_CLEAR_TT_BUFFER, USB_DIR_IN | USB_RECIP_OTHER,
		devinfo, tt, 0, 0, HZ);
}

/*
 * enumeration blocks khubd for a long time. we use keventd instead, since
 * long blocking there is the exception, not the rule.  accordingly, HCDs
 * talking to TTs must queue control transfers (not just bulk and iso), so
 * both can talk to the same hub concurrently.
 */
static void hub_tt_kevent (void *arg)
{
	struct usb_hub		*hub = arg;
	unsigned long		flags;

	spin_lock_irqsave (&hub->tt.lock, flags);
	while (!list_empty (&hub->tt.clear_list)) {
		struct list_head	*temp;
		struct usb_tt_clear	*clear;
		struct usb_device	*dev;
		int			status;

		temp = hub->tt.clear_list.next;
		clear = list_entry (temp, struct usb_tt_clear, clear_list);
		list_del (&clear->clear_list);

		/* drop lock so HCD can concurrently report other TT errors */
		spin_unlock_irqrestore (&hub->tt.lock, flags);
		dev = interface_to_usbdev (hub->intf);
		status = hub_clear_tt_buffer (dev, clear->devinfo, clear->tt);
		spin_lock_irqsave (&hub->tt.lock, flags);

		if (status)
			err ("usb-%s-%s clear tt %d (%04x) error %d",
				dev->bus->bus_name, dev->devpath,
				clear->tt, clear->devinfo, status);
		kfree (clear);
	}
	spin_unlock_irqrestore (&hub->tt.lock, flags);
}

/**
 * usb_hub_tt_clear_buffer - clear control/bulk TT state in high speed hub
 * @dev: the device whose split transaction failed
 * @pipe: identifies the endpoint of the failed transaction
 *
 * High speed HCDs use this to tell the hub driver that some split control or
 * bulk transaction failed in a way that requires clearing internal state of
 * a transaction translator.  This is normally detected (and reported) from
 * interrupt context.
 *
 * It may not be possible for that hub to handle additional full (or low)
 * speed transactions until that state is fully cleared out.
 */
void usb_hub_tt_clear_buffer (struct usb_device *dev, int pipe)
{
	struct usb_tt		*tt = dev->tt;
	unsigned long		flags;
	struct usb_tt_clear	*clear;

	/* we've got to cope with an arbitrary number of pending TT clears,
	 * since each TT has "at least two" buffers that can need it (and
	 * there can be many TTs per hub).  even if they're uncommon.
	 */
	if ((clear = kmalloc (sizeof *clear, SLAB_ATOMIC)) == 0) {
		err ("can't save CLEAR_TT_BUFFER state for hub at usb-%s-%s",
			dev->bus->bus_name, tt->hub->devpath);
		/* FIXME recover somehow ... RESET_TT? */
		return;
	}

	/* info that CLEAR_TT_BUFFER needs */
	clear->tt = tt->multi ? dev->ttport : 1;
	clear->devinfo = usb_pipeendpoint (pipe);
	clear->devinfo |= dev->devnum << 4;
	clear->devinfo |= usb_pipecontrol (pipe)
			? (USB_ENDPOINT_XFER_CONTROL << 11)
			: (USB_ENDPOINT_XFER_BULK << 11);
	if (usb_pipein (pipe))
		clear->devinfo |= 1 << 15;
	
	/* tell keventd to clear state for this TT */
	spin_lock_irqsave (&tt->lock, flags);
	list_add_tail (&clear->clear_list, &tt->clear_list);
	schedule_work (&tt->kevent);
	spin_unlock_irqrestore (&tt->lock, flags);
}

static void hub_power_on(struct usb_hub *hub)
{
	struct usb_device *dev;
	int i;

	/* Enable power to the ports */
	dev_dbg(hubdev(interface_to_usbdev(hub->intf)),
		"enabling power on all ports\n");
	dev = interface_to_usbdev(hub->intf);

	for (i = 0; i < hub->descriptor->bNbrPorts; i++)
		set_port_feature(dev, i + 1, USB_PORT_FEAT_POWER);

	/* Wait for power to be enabled */
	wait_ms(hub->descriptor->bPwrOn2PwrGood * 2);
}

static int hub_hub_status(struct usb_hub *hub,
		u16 *status, u16 *change)
{
	struct usb_device *dev = interface_to_usbdev (hub->intf);
	int ret;

	ret = get_hub_status(dev, &hub->status->hub);
	if (ret < 0)
		dev_err (hubdev (dev),
			"%s failed (err = %d)\n", __FUNCTION__, ret);
	else {
		*status = le16_to_cpu(hub->status->hub.wHubStatus);
		*change = le16_to_cpu(hub->status->hub.wHubChange); 
		ret = 0;
	}
	return ret;
}

static int hub_configure(struct usb_hub *hub,
	struct usb_endpoint_descriptor *endpoint)
{
	struct usb_device *dev = interface_to_usbdev (hub->intf);
	struct device *hub_dev;
	u16 hubstatus, hubchange;
	unsigned int pipe;
	int maxp, ret;
	char *message;

	hub->buffer = usb_buffer_alloc(dev, sizeof(*hub->buffer), GFP_KERNEL,
			&hub->buffer_dma);
	if (!hub->buffer) {
		message = "can't allocate hub irq buffer";
		ret = -ENOMEM;
		goto fail;
	}

	hub->status = kmalloc(sizeof(*hub->status), GFP_KERNEL);
	if (!hub->status) {
		message = "can't kmalloc hub status buffer";
		ret = -ENOMEM;
		goto fail;
	}

	hub->descriptor = kmalloc(sizeof(*hub->descriptor), GFP_KERNEL);
	if (!hub->descriptor) {
		message = "can't kmalloc hub descriptor";
		ret = -ENOMEM;
		goto fail;
	}

	/* Request the entire hub descriptor.
	 * hub->descriptor can handle USB_MAXCHILDREN ports,
	 * but the hub can/will return fewer bytes here.
	 */
	ret = get_hub_descriptor(dev, hub->descriptor,
			sizeof(*hub->descriptor));
	if (ret < 0) {
		message = "can't read hub descriptor";
		goto fail;
	} else if (hub->descriptor->bNbrPorts > USB_MAXCHILDREN) {
		message = "hub has too many ports!";
		ret = -ENODEV;
		goto fail;
	}

	hub_dev = hubdev(dev);
	dev->maxchild = hub->descriptor->bNbrPorts;
	dev_info (hub_dev, "%d port%s detected\n", dev->maxchild,
		(dev->maxchild == 1) ? "" : "s");

	le16_to_cpus(&hub->descriptor->wHubCharacteristics);

	if (hub->descriptor->wHubCharacteristics & HUB_CHAR_COMPOUND) {
		int	i;
		char	portstr [USB_MAXCHILDREN + 1];

		for (i = 0; i < dev->maxchild; i++)
			portstr[i] = hub->descriptor->DeviceRemovable
				    [((i + 1) / 8)] & (1 << ((i + 1) % 8))
				? 'F' : 'R';
		portstr[dev->maxchild] = 0;
		dev_dbg(hub_dev, "compound device; port removable status: %s\n", portstr);
	} else
		dev_dbg(hub_dev, "standalone hub\n");

	switch (hub->descriptor->wHubCharacteristics & HUB_CHAR_LPSM) {
		case 0x00:
			dev_dbg(hub_dev, "ganged power switching\n");
			break;
		case 0x01:
			dev_dbg(hub_dev, "individual port power switching\n");
			break;
		case 0x02:
		case 0x03:
			dev_dbg(hub_dev, "unknown reserved power switching mode\n");
			break;
	}

	switch (hub->descriptor->wHubCharacteristics & HUB_CHAR_OCPM) {
		case 0x00:
			dev_dbg(hub_dev, "global over-current protection\n");
			break;
		case 0x08:
			dev_dbg(hub_dev, "individual port over-current protection\n");
			break;
		case 0x10:
		case 0x18:
			dev_dbg(hub_dev, "no over-current protection\n");
                        break;
	}

	spin_lock_init (&hub->tt.lock);
	INIT_LIST_HEAD (&hub->tt.clear_list);
	INIT_WORK (&hub->tt.kevent, hub_tt_kevent, hub);
	switch (dev->descriptor.bDeviceProtocol) {
		case 0:
			break;
		case 1:
			dev_dbg(hub_dev, "Single TT\n");
			hub->tt.hub = dev;
			break;
		case 2:
			dev_dbg(hub_dev, "TT per port\n");
			hub->tt.hub = dev;
			hub->tt.multi = 1;
			break;
		default:
			dev_dbg(hub_dev, "Unrecognized hub protocol %d\n",
				dev->descriptor.bDeviceProtocol);
			break;
	}

	switch (hub->descriptor->wHubCharacteristics & HUB_CHAR_TTTT) {
		case 0x00:
			if (dev->descriptor.bDeviceProtocol != 0)
				dev_dbg(hub_dev, "TT requires at most 8 FS bit times\n");
			break;
		case 0x20:
			dev_dbg(hub_dev, "TT requires at most 16 FS bit times\n");
			break;
		case 0x40:
			dev_dbg(hub_dev, "TT requires at most 24 FS bit times\n");
			break;
		case 0x60:
			dev_dbg(hub_dev, "TT requires at most 32 FS bit times\n");
			break;
	}

	dev_dbg(hub_dev, "Port indicators are %s supported\n", 
	    (hub->descriptor->wHubCharacteristics & HUB_CHAR_PORTIND)
	    	? "" : "not");

	dev_dbg(hub_dev, "power on to power good time: %dms\n",
		hub->descriptor->bPwrOn2PwrGood * 2);
	dev_dbg(hub_dev, "hub controller current requirement: %dmA\n",
		hub->descriptor->bHubContrCurrent);

	ret = hub_hub_status(hub, &hubstatus, &hubchange);
	if (ret < 0) {
		message = "can't get hub status";
		goto fail;
	}

	dev_dbg(hub_dev, "local power source is %s\n",
		(hubstatus & HUB_STATUS_LOCAL_POWER)
		? "lost (inactive)" : "good");

	dev_dbg(hub_dev, "%sover-current condition exists\n",
		(hubstatus & HUB_STATUS_OVERCURRENT) ? "" : "no ");

	/* Start the interrupt endpoint */
	pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);
	maxp = usb_maxpacket(dev, pipe, usb_pipeout(pipe));

	if (maxp > sizeof(*hub->buffer))
		maxp = sizeof(*hub->buffer);

	hub->urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!hub->urb) {
		message = "couldn't allocate interrupt urb";
		ret = -ENOMEM;
		goto fail;
	}

	usb_fill_int_urb(hub->urb, dev, pipe, *hub->buffer, maxp, hub_irq,
		hub, endpoint->bInterval);
	hub->urb->transfer_dma = hub->buffer_dma;
	hub->urb->transfer_flags |= URB_NO_DMA_MAP;
	ret = usb_submit_urb(hub->urb, GFP_KERNEL);
	if (ret) {
		message = "couldn't submit status urb";
		goto fail;
	}

	/* Wake up khubd */
	wake_up(&khubd_wait);

	hub_power_on(hub);

	return 0;

fail:
	dev_err (&hub->intf->dev, "config failed, %s (err %d)\n",
			message, ret);
	/* hub_disconnect() frees urb and descriptor */
	return ret;
}

static void hub_disconnect(struct usb_interface *intf)
{
	struct usb_hub *hub = usb_get_intfdata (intf);
	unsigned long flags;

	if (!hub)
		return;

	usb_set_intfdata (intf, NULL);
	spin_lock_irqsave(&hub_event_lock, flags);

	/* Delete it and then reset it */
	list_del(&hub->event_list);
	INIT_LIST_HEAD(&hub->event_list);
	list_del(&hub->hub_list);
	INIT_LIST_HEAD(&hub->hub_list);

	spin_unlock_irqrestore(&hub_event_lock, flags);

	down(&hub->khubd_sem); /* Wait for khubd to leave this hub alone. */
	up(&hub->khubd_sem);

	/* assuming we used keventd, it must quiesce too */
	if (hub->tt.hub)
		flush_scheduled_work ();

	if (hub->urb) {
		usb_unlink_urb(hub->urb);
		usb_free_urb(hub->urb);
		hub->urb = NULL;
	}

	if (hub->descriptor) {
		kfree(hub->descriptor);
		hub->descriptor = NULL;
	}

	if (hub->status) {
		kfree(hub->status);
		hub->status = NULL;
	}

	if (hub->buffer) {
		usb_buffer_free(interface_to_usbdev(intf),
				sizeof(*hub->buffer), hub->buffer,
				hub->buffer_dma);
		hub->buffer = NULL;
	}

	/* Free the memory */
	kfree(hub);
}

static int hub_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_host_interface *desc;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_device *dev;
	struct usb_hub *hub;
	unsigned long flags;

	desc = intf->altsetting + intf->act_altsetting;
	dev = interface_to_usbdev(intf);

	/* Some hubs have a subclass of 1, which AFAICT according to the */
	/*  specs is not defined, but it works */
	if ((desc->desc.bInterfaceSubClass != 0) &&
	    (desc->desc.bInterfaceSubClass != 1)) {
descriptor_error:
		dev_err (&intf->dev, "bad descriptor, ignoring hub\n");
		return -EIO;
	}

	/* Multiple endpoints? What kind of mutant ninja-hub is this? */
	if (desc->desc.bNumEndpoints != 1) {
		goto descriptor_error;
	}

	endpoint = &desc->endpoint[0].desc;

	/* Output endpoint? Curiouser and curiouser.. */
	if (!(endpoint->bEndpointAddress & USB_DIR_IN)) {
		goto descriptor_error;
	}

	/* If it's not an interrupt endpoint, we'd better punt! */
	if ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
			!= USB_ENDPOINT_XFER_INT) {
		goto descriptor_error;
		return -EIO;
	}

	/* We found a hub */
	dev_info (hubdev (dev), "USB hub found\n");

	hub = kmalloc(sizeof(*hub), GFP_KERNEL);
	if (!hub) {
		err("couldn't kmalloc hub struct");
		return -ENOMEM;
	}

	memset(hub, 0, sizeof(*hub));

	INIT_LIST_HEAD(&hub->event_list);
	hub->intf = intf;
	init_MUTEX(&hub->khubd_sem);

	/* Record the new hub's existence */
	spin_lock_irqsave(&hub_event_lock, flags);
	INIT_LIST_HEAD(&hub->hub_list);
	list_add(&hub->hub_list, &hub_list);
	spin_unlock_irqrestore(&hub_event_lock, flags);

	usb_set_intfdata (intf, hub);

	if (hub_configure(hub, endpoint) >= 0) {
		strcpy (intf->dev.name, "Hub");
		return 0;
	}

	hub_disconnect (intf);
	return -ENODEV;
}

static int
hub_ioctl(struct usb_interface *intf, unsigned int code, void *user_data)
{
	struct usb_device *hub = interface_to_usbdev (intf);

	/* assert ifno == 0 (part of hub spec) */
	switch (code) {
	case USBDEVFS_HUB_PORTINFO: {
		struct usbdevfs_hub_portinfo *info = user_data;
		unsigned long flags;
		int i;

		spin_lock_irqsave(&hub_event_lock, flags);
		if (hub->devnum <= 0)
			info->nports = 0;
		else {
			info->nports = hub->maxchild;
			for (i = 0; i < info->nports; i++) {
				if (hub->children[i] == NULL)
					info->port[i] = 0;
				else
					info->port[i] =
						hub->children[i]->devnum;
			}
		}
		spin_unlock_irqrestore(&hub_event_lock, flags);

		return info->nports + 1;
		}

	default:
		return -ENOSYS;
	}
}

static int hub_reset(struct usb_hub *hub)
{
	struct usb_device *dev = interface_to_usbdev(hub->intf);
	int i;

	/* Disconnect any attached devices */
	for (i = 0; i < hub->descriptor->bNbrPorts; i++) {
		if (dev->children[i])
			usb_disconnect(&dev->children[i]);
	}

	/* Attempt to reset the hub */
	if (hub->urb)
		usb_unlink_urb(hub->urb);
	else
		return -1;

	if (usb_reset_device(dev))
		return -1;

	hub->urb->dev = dev;                                                    
	if (usb_submit_urb(hub->urb, GFP_KERNEL))
		return -1;

	hub_power_on(hub);

	return 0;
}

static void hub_start_disconnect(struct usb_device *dev)
{
	struct usb_device *parent = dev->parent;
	int i;

	/* Find the device pointer to disconnect */
	if (parent) {
		for (i = 0; i < parent->maxchild; i++) {
			if (parent->children[i] == dev) {
				usb_disconnect(&parent->children[i]);
				return;
			}
		}
	}

	err("cannot disconnect hub %s", dev->devpath);
}

static int hub_port_status(struct usb_device *dev, int port,
			       u16 *status, u16 *change)
{
	struct usb_hub *hub = usb_get_intfdata (dev->actconfig->interface);
	int ret;

	ret = get_port_status(dev, port + 1, &hub->status->port);
	if (ret < 0)
		dev_err (hubdev (dev),
			"%s failed (err = %d)\n", __FUNCTION__, ret);
	else {
		*status = le16_to_cpu(hub->status->port.wPortStatus);
		*change = le16_to_cpu(hub->status->port.wPortChange); 
		ret = 0;
	}
	return ret;
}

#define HUB_RESET_TRIES		5
#define HUB_PROBE_TRIES		2
#define HUB_SHORT_RESET_TIME	10
#define HUB_LONG_RESET_TIME	200
#define HUB_RESET_TIMEOUT	500

/* return: -1 on error, 0 on success, 1 on disconnect.  */
static int hub_port_wait_reset(struct usb_device *hub, int port,
				struct usb_device *dev, unsigned int delay)
{
	int delay_time, ret;
	u16 portstatus;
	u16 portchange;

	for (delay_time = 0;
			delay_time < HUB_RESET_TIMEOUT;
			delay_time += delay) {
		/* wait to give the device a chance to reset */
		wait_ms(delay);

		/* read and decode port status */
		ret = hub_port_status(hub, port, &portstatus, &portchange);
		if (ret < 0) {
			return -1;
		}

		/* Device went away? */
		if (!(portstatus & USB_PORT_STAT_CONNECTION))
			return 1;

		/* bomb out completely if something weird happened */
		if ((portchange & USB_PORT_STAT_C_CONNECTION))
			return -1;

		/* if we`ve finished resetting, then break out of the loop */
		if (!(portstatus & USB_PORT_STAT_RESET) &&
		    (portstatus & USB_PORT_STAT_ENABLE)) {
			if (portstatus & USB_PORT_STAT_HIGH_SPEED)
				dev->speed = USB_SPEED_HIGH;
			else if (portstatus & USB_PORT_STAT_LOW_SPEED)
				dev->speed = USB_SPEED_LOW;
			else
				dev->speed = USB_SPEED_FULL;
			return 0;
		}

		/* switch to the long delay after two short delay failures */
		if (delay_time >= 2 * HUB_SHORT_RESET_TIME)
			delay = HUB_LONG_RESET_TIME;

		dev_dbg (hubdev (hub),
			"port %d not reset yet, waiting %dms\n",
			port + 1, delay);
	}

	return -1;
}

/* return: -1 on error, 0 on success, 1 on disconnect.  */
static int hub_port_reset(struct usb_device *hub, int port,
				struct usb_device *dev, unsigned int delay)
{
	int i, status;

	/* Reset the port */
	for (i = 0; i < HUB_RESET_TRIES; i++) {
		set_port_feature(hub, port + 1, USB_PORT_FEAT_RESET);

		/* return on disconnect or reset */
		status = hub_port_wait_reset(hub, port, dev, delay);
		if (status != -1) {
			clear_port_feature(hub,
				port + 1, USB_PORT_FEAT_C_RESET);
			dev->state = status
					? USB_STATE_NOTATTACHED
					: USB_STATE_DEFAULT;
			return status;
		}

		dev_dbg (hubdev (hub),
			"port %d not enabled, trying reset again...\n",
			port + 1);
		delay = HUB_LONG_RESET_TIME;
	}

	dev_err (hubdev (hub),
		"Cannot enable port %i.  Maybe the USB cable is bad?\n",
		port + 1);

	return -1;
}

int hub_port_disable(struct usb_device *hub, int port)
{
	int ret;

	ret = clear_port_feature(hub, port + 1, USB_PORT_FEAT_ENABLE);
	if (ret)
		dev_err(hubdev(hub), "cannot disable port %d (err = %d)\n",
			port + 1, ret);

	return ret;
}

/* USB 2.0 spec, 7.1.7.3 / fig 7-29:
 *
 * Between connect detection and reset signaling there must be a delay
 * of 100ms at least for debounce and power-settling. The corresponding
 * timer shall restart whenever the downstream port detects a disconnect.
 * 
 * Apparently there are some bluetooth and irda-dongles and a number
 * of low-speed devices which require longer delays of about 200-400ms.
 * Not covered by the spec - but easy to deal with.
 *
 * This implementation uses 400ms minimum debounce timeout and checks
 * every 25ms for transient disconnects to restart the delay.
 */

#define HUB_DEBOUNCE_TIMEOUT	400
#define HUB_DEBOUNCE_STEP	 25
#define HUB_DEBOUNCE_STABLE	  4

/* return: -1 on error, 0 on success, 1 on disconnect.  */
static int hub_port_debounce(struct usb_device *hub, int port)
{
	int ret;
	int delay_time, stable_count;
	u16 portchange, portstatus;
	unsigned connection;

	connection = 0;
	stable_count = 0;
	for (delay_time = 0; delay_time < HUB_DEBOUNCE_TIMEOUT; delay_time += HUB_DEBOUNCE_STEP) {
		wait_ms(HUB_DEBOUNCE_STEP);

		ret = hub_port_status(hub, port, &portstatus, &portchange);
		if (ret < 0)
			return -1;

		if ((portstatus & USB_PORT_STAT_CONNECTION) == connection) {
			if (connection) {
				if (++stable_count == HUB_DEBOUNCE_STABLE)
					break;
			}
		} else {
			stable_count = 0;
		}
		connection = portstatus & USB_PORT_STAT_CONNECTION;

		if ((portchange & USB_PORT_STAT_C_CONNECTION)) {
			clear_port_feature(hub, port+1, USB_PORT_FEAT_C_CONNECTION);
		}
	}

	/* XXX Replace this with dbg() when 2.6 is about to ship. */
	dev_dbg (hubdev (hub),
		"debounce: port %d: delay %dms stable %d status 0x%x\n",
		port + 1, delay_time, stable_count, portstatus);

	return ((portstatus&USB_PORT_STAT_CONNECTION)) ? 0 : 1;
}

static void hub_port_connect_change(struct usb_hub *hubstate, int port,
					u16 portstatus, u16 portchange)
{
	struct usb_device *hub = interface_to_usbdev(hubstate->intf);
	struct usb_device *dev;
	unsigned int delay = HUB_SHORT_RESET_TIME;
	int i;

	dev_dbg (&hubstate->intf->dev,
		"port %d, status %x, change %x, %s\n",
		port + 1, portstatus, portchange, portspeed (portstatus));

	/* Clear the connection change status */
	clear_port_feature(hub, port + 1, USB_PORT_FEAT_C_CONNECTION);

	/* Disconnect any existing devices under this port */
	if (hub->children[port])
		usb_disconnect(&hub->children[port]);

	/* Return now if nothing is connected */
	if (!(portstatus & USB_PORT_STAT_CONNECTION)) {
		if (portstatus & USB_PORT_STAT_ENABLE)
			hub_port_disable(hub, port);

		return;
	}

	if (hub_port_debounce(hub, port)) {
		dev_err (&hubstate->intf->dev,
			"connect-debounce failed, port %d disabled\n",
			port+1);
		hub_port_disable(hub, port);
		return;
	}

	/* Some low speed devices have problems with the quick delay, so */
	/*  be a bit pessimistic with those devices. RHbug #23670 */
	if (portstatus & USB_PORT_STAT_LOW_SPEED)
		delay = HUB_LONG_RESET_TIME;

	down(&usb_address0_sem);

	for (i = 0; i < HUB_PROBE_TRIES; i++) {
		struct usb_device *pdev;
		int	len;

		/* Allocate a new device struct */
		dev = usb_alloc_dev(hub, hub->bus);
		if (!dev) {
			dev_err (&hubstate->intf->dev,
				"couldn't allocate usb_device\n");
			break;
		}

		hub->children[port] = dev;
		dev->state = USB_STATE_POWERED;

		/* Reset the device, and detect its speed */
		if (hub_port_reset(hub, port, dev, delay)) {
			usb_put_dev(dev);
			break;
		}

		/* Find a new address for it */
		usb_connect(dev);

		/* Set up TT records, if needed  */
		if (hub->tt) {
			dev->tt = hub->tt;
			dev->ttport = hub->ttport;
		} else if (dev->speed != USB_SPEED_HIGH
				&& hub->speed == USB_SPEED_HIGH) {
			dev->tt = &hubstate->tt;
			dev->ttport = port + 1;
		}

		/* Save readable and stable topology id, distinguishing
		 * devices by location for diagnostics, tools, etc.  The
		 * string is a path along hub ports, from the root.  Each
		 * device's id will be stable until USB is re-cabled, and
		 * hubs are often labeled with these port numbers.
		 *
		 * Initial size: ".NN" times five hubs + NUL = 16 bytes max
		 * (quite rare, since most hubs have 4-6 ports).
		 */
		pdev = dev->parent;
		if (pdev->devpath [0] != '0')	/* parent not root? */
			len = snprintf (dev->devpath, sizeof dev->devpath,
				"%s.%d", pdev->devpath, port + 1);
		/* root == "0", root port 2 == "2", port 3 that hub "2.3" */
		else
			len = snprintf (dev->devpath, sizeof dev->devpath,
				"%d", port + 1);
		if (len == sizeof dev->devpath)
			dev_err (&hubstate->intf->dev,
				"devpath size! usb/%03d/%03d path %s\n",
				dev->bus->busnum, dev->devnum, dev->devpath);
		dev_info (&hubstate->intf->dev,
			"new USB device on port %d, assigned address %d\n",
			port + 1, dev->devnum);

		/* put the device in the global device tree. the hub port
		 * is the "bus_id"; hubs show in hierarchy like bridges
		 */
		dev->dev.parent = dev->parent->dev.parent->parent;

		/* Run it through the hoops (find a driver, etc) */
		if (!usb_new_device(dev, &hub->dev))
			goto done;

		/* Free the configuration if there was an error */
		usb_put_dev(dev);

		/* Switch to a long reset time */
		delay = HUB_LONG_RESET_TIME;
	}

	hub->children[port] = NULL;
	hub_port_disable(hub, port);
done:
	up(&usb_address0_sem);
}

static void hub_events(void)
{
	unsigned long flags;
	struct list_head *tmp;
	struct usb_device *dev;
	struct usb_hub *hub;
	u16 hubstatus;
	u16 hubchange;
	u16 portstatus;
	u16 portchange;
	int i, ret;
	int m=0;
	/*
	 *  We restart the list every time to avoid a deadlock with
	 * deleting hubs downstream from this one. This should be
	 * safe since we delete the hub from the event list.
	 * Not the most efficient, but avoids deadlocks.
	 */

	while (m<5) {
		m++;
		spin_lock_irqsave(&hub_event_lock, flags);

		if (list_empty(&hub_event_list))
			break;

		/* Grab the next entry from the beginning of the list */
		tmp = hub_event_list.next;

		hub = list_entry(tmp, struct usb_hub, event_list);
		dev = interface_to_usbdev(hub->intf);

		list_del_init(tmp);

		if (unlikely(down_trylock(&hub->khubd_sem)))
			BUG();	/* never blocks, we were on list */

		spin_unlock_irqrestore(&hub_event_lock, flags);

		if (hub->error) {
			dev_dbg (&hub->intf->dev, "resetting for error %d\n",
				hub->error);

			if (hub_reset(hub)) {
				dev_dbg (&hub->intf->dev,
					"can't reset; disconnecting\n");
				up(&hub->khubd_sem);
				hub_start_disconnect(dev);
				continue;
			}

			hub->nerrors = 0;
			hub->error = 0;
		}

		for (i = 0; i < hub->descriptor->bNbrPorts; i++) {
			ret = hub_port_status(dev, i, &portstatus, &portchange);
			if (ret < 0) {
				continue;
			}

			if (portchange & USB_PORT_STAT_C_CONNECTION) {
				hub_port_connect_change(hub, i, portstatus, portchange);
			} else if (portchange & USB_PORT_STAT_C_ENABLE) {
				dev_dbg (hubdev (dev),
					"port %d enable change, status %x\n",
					i + 1, portstatus);
				clear_port_feature(dev,
					i + 1, USB_PORT_FEAT_C_ENABLE);

				/*
				 * EM interference sometimes causes badly
				 * shielded USB devices to be shutdown by
				 * the hub, this hack enables them again.
				 * Works at least with mouse driver. 
				 */
				if (!(portstatus & USB_PORT_STAT_ENABLE)
				    && (portstatus & USB_PORT_STAT_CONNECTION)
				    && (dev->children[i])) {
					dev_err (&hub->intf->dev,
					    "port %i "
					    "disabled by hub (EMI?), "
					    "re-enabling...",
						i + 1);
					hub_port_connect_change(hub,
						i, portstatus, portchange);
				}
			}

			if (portchange & USB_PORT_STAT_C_SUSPEND) {
				dev_dbg (&hub->intf->dev,
					"suspend change on port %d\n",
					i + 1);
				clear_port_feature(dev,
					i + 1,  USB_PORT_FEAT_C_SUSPEND);
			}
			
			if (portchange & USB_PORT_STAT_C_OVERCURRENT) {
				dev_err (&hub->intf->dev,
					"over-current change on port %d\n",
					i + 1);
				clear_port_feature(dev,
					i + 1, USB_PORT_FEAT_C_OVER_CURRENT);
				hub_power_on(hub);
			}

			if (portchange & USB_PORT_STAT_C_RESET) {
				dev_dbg (&hub->intf->dev,
					"reset change on port %d\n",
					i + 1);
				clear_port_feature(dev,
					i + 1, USB_PORT_FEAT_C_RESET);
			}
		} /* end for i */

		/* deal with hub status changes */
		if (hub_hub_status(hub, &hubstatus, &hubchange) < 0)
			dev_err (&hub->intf->dev, "get_hub_status failed\n");
		else {
			if (hubchange & HUB_CHANGE_LOCAL_POWER) {
				dev_dbg (&hub->intf->dev, "power change\n");
				clear_hub_feature(dev, C_HUB_LOCAL_POWER);
			}
			if (hubchange & HUB_CHANGE_OVERCURRENT) {
				dev_dbg (&hub->intf->dev, "overcurrent change\n");
				wait_ms(500);	/* Cool down */
				clear_hub_feature(dev, C_HUB_OVER_CURRENT);
                        	hub_power_on(hub);
			}
		}
		up(&hub->khubd_sem);
        } /* end while (1) */

	spin_unlock_irqrestore(&hub_event_lock, flags);
}

static int hub_thread(void *__hub)
{
	/*
	 * This thread doesn't need any user-level access,
	 * so get rid of all our resources
	 */

	daemonize("khubd");
	allow_signal(SIGKILL);
	/* Send me a signal to get me die (for debugging) */
	do {
		
		hub_events();
		wait_event_interruptible(khubd_wait, !list_empty(&hub_event_list)); 

		if (current->flags & PF_FREEZE)
			refrigerator(PF_IOTHREAD);

	} while (!signal_pending(current));

//	dbg("hub_thread exiting");
	complete_and_exit(&khubd_exited, 0);
}

static struct usb_device_id hub_id_table [] = {
    { .match_flags = USB_DEVICE_ID_MATCH_DEV_CLASS,
      .bDeviceClass = USB_CLASS_HUB},
    { .match_flags = USB_DEVICE_ID_MATCH_INT_CLASS,
      .bInterfaceClass = USB_CLASS_HUB},
    { }						/* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, hub_id_table);

static struct usb_driver hub_driver = {
	.owner =	THIS_MODULE,
	.name =		"hub",
	.probe =	hub_probe,
	.disconnect =	hub_disconnect,
	.ioctl =	hub_ioctl,
	.id_table =	hub_id_table,
};

/*
 * This should be a separate module.
 */
int usb_hub_init(void)
{
	pid_t pid;

	if (usb_register(&hub_driver) < 0) {
		err("Unable to register USB hub driver");
		return -1;
	}

	pid = kernel_thread(hub_thread, NULL,
		CLONE_FS | CLONE_FILES | CLONE_SIGHAND);
	if (pid >= 0) {
		khubd_pid = pid;
		return 0;
	}

	/* Fall through if kernel_thread failed */
	usb_deregister(&hub_driver);
	err("failed to start hub_thread");

	return -1;
}

void usb_hub_cleanup(void)
{
	int ret;

	/* Kill the thread */
	ret = kill_proc(khubd_pid, SIGKILL, 1);

	wait_for_completion(&khubd_exited);

	/*
	 * Hub resources are freed for us by usb_deregister. It calls
	 * usb_driver_purge on every device which in turn calls that
	 * devices disconnect function if it is using this driver.
	 * The hub_disconnect function takes care of releasing the
	 * individual hub resources. -greg
	 */
	usb_deregister(&hub_driver);
} /* usb_hub_cleanup() */

/*
 * WARNING - If a driver calls usb_reset_device, you should simulate a
 * disconnect() and probe() for other interfaces you doesn't claim. This
 * is left up to the driver writer right now. This insures other drivers
 * have a chance to re-setup their interface.
 *
 * Take a look at proc_resetdevice in devio.c for some sample code to
 * do this.
 * Use this only from within your probe function, otherwise use
 * usb_reset_device() below, which ensure proper locking
 */
int usb_physical_reset_device(struct usb_device *dev)
{
	struct usb_device *parent = dev->parent;
	struct usb_device_descriptor *descriptor;
	int i, ret, port = -1;

	if (!parent) {
		err("attempting to reset root hub!");
		return -EINVAL;
	}

	for (i = 0; i < parent->maxchild; i++)
		if (parent->children[i] == dev) {
			port = i;
			break;
		}

	if (port < 0)
		return -ENOENT;

	descriptor = kmalloc(sizeof *descriptor, GFP_NOIO);
	if (!descriptor) {
		return -ENOMEM;
	}

	down(&usb_address0_sem);

	/* Send a reset to the device */
	if (hub_port_reset(parent, port, dev, HUB_SHORT_RESET_TIME)) {
		hub_port_disable(parent, port);
		up(&usb_address0_sem);
		kfree(descriptor);
		return(-ENODEV);
	}

	/* Reprogram the Address */
	ret = usb_set_address(dev);
	if (ret < 0) {
		err("USB device not accepting new address (error=%d)", ret);
		hub_port_disable(parent, port);
		up(&usb_address0_sem);
		kfree(descriptor);
		return ret;
	}

	/* Let the SET_ADDRESS settle */
	wait_ms(10);

	up(&usb_address0_sem);

	/*
	 * Now we fetch the configuration descriptors for the device and
	 * see if anything has changed. If it has, we dump the current
	 * parsed descriptors and reparse from scratch. Then we leave
	 * the device alone for the caller to finish setting up.
	 *
	 * If nothing changed, we reprogram the configuration and then
	 * the alternate settings.
	 */

	ret = usb_get_descriptor(dev, USB_DT_DEVICE, 0, descriptor,
			sizeof(*descriptor));
	if (ret < 0) {
		kfree(descriptor);
		return ret;
	}

	le16_to_cpus(&descriptor->bcdUSB);
	le16_to_cpus(&descriptor->idVendor);
	le16_to_cpus(&descriptor->idProduct);
	le16_to_cpus(&descriptor->bcdDevice);

	if (memcmp(&dev->descriptor, descriptor, sizeof(*descriptor))) {
		kfree(descriptor);
		usb_destroy_configuration(dev);

		ret = usb_get_device_descriptor(dev);
		if (ret < sizeof(dev->descriptor)) {
			if (ret < 0)
				err("unable to get device %s descriptor "
					"(error=%d)", dev->devpath, ret);
			else
				err("USB device %s descriptor short read "
					"(expected %Zi, got %i)",
					dev->devpath,
					sizeof(dev->descriptor), ret);

			clear_bit(dev->devnum, dev->bus->devmap.devicemap);
			dev->devnum = -1;
			return -EIO;
		}

		ret = usb_get_configuration(dev);
		if (ret < 0) {
			err("unable to get configuration (error=%d)", ret);
			usb_destroy_configuration(dev);
			clear_bit(dev->devnum, dev->bus->devmap.devicemap);
			dev->devnum = -1;
			return 1;
		}

		dev->actconfig = dev->config;
		usb_set_maxpacket(dev);

		return 1;
	}

	kfree(descriptor);

	ret = usb_set_configuration(dev, dev->actconfig->desc.bConfigurationValue);
	if (ret < 0) {
		err("failed to set dev %s active configuration (error=%d)",
			dev->devpath, ret);
		return ret;
	}

	for (i = 0; i < dev->actconfig->desc.bNumInterfaces; i++) {
		struct usb_interface *intf = &dev->actconfig->interface[i];
		struct usb_interface_descriptor *as;

		as = &intf->altsetting[intf->act_altsetting].desc;
		ret = usb_set_interface(dev, as->bInterfaceNumber,
			as->bAlternateSetting);
		if (ret < 0) {
			err("failed to set active alternate setting "
				"for dev %s interface %d (error=%d)",
				dev->devpath, i, ret);
			return ret;
		}
	}

	return 0;
}

int usb_reset_device(struct usb_device *udev)
{
	//struct device *gdev = &udev->dev;
	int r;
	
	down_read(&gdev->bus->subsys.rwsem);
	r = usb_physical_reset_device(udev);
	up_read(&gdev->bus->subsys.rwsem);

	return r;
}


