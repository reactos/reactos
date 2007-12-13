/*
 * drivers/usb/usb.c
 *
 * (C) Copyright Linus Torvalds 1999
 * (C) Copyright Johannes Erdfelt 1999-2001
 * (C) Copyright Andreas Gal 1999
 * (C) Copyright Gregory P. Smith 1999
 * (C) Copyright Deti Fliegl 1999 (new USB architecture)
 * (C) Copyright Randy Dunlap 2000
 * (C) Copyright David Brownell 2000-2001 (kernel hotplug, usb_device_id,
 	more docs, etc)
 * (C) Copyright Yggdrasil Computing, Inc. 2000
 *     (usb_device_id matching changes by Adam J. Richter)
 * (C) Copyright Greg Kroah-Hartman 2002-2003
 *
 * NOTE! This is not actually a driver at all, rather this is
 * just a collection of helper routines that implement the
 * generic USB things that the real drivers can use..
 *
 * Think of this as a "USB library" rather than anything else.
 * It should be considered a slave, with no callbacks. Callbacks
 * are evil.
 */

#if 0
#include <linux/config.h>

#ifdef CONFIG_USB_DEBUG
	#define DEBUG
#else
	#undef DEBUG
#endif

#include <linux/module.h>
#include <linux/string.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include <linux/interrupt.h>  /* for in_interrupt() */
#include <linux/kmod.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/smp_lock.h>
#include <linux/usb.h>

#include <asm/io.h>
#include <asm/scatterlist.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include "hcd.h"
#include "usb.h"
#else
#include "../usb_wrapper.h"
#include "hcd.h"
#endif


extern int  usb_hub_init(void);
extern void usb_hub_cleanup(void);
extern int usb_major_init(void);
extern void usb_major_cleanup(void);


int nousb;		/* Disable USB when built into kernel image */
			/* Not honored on modular build */


static int generic_probe (struct device *dev)
{
	return 0;
}
static int generic_remove (struct device *dev)
{
	return 0;
}

static struct device_driver usb_generic_driver = {
	.name =	"usb",
	.bus = &usb_bus_type,
	.probe = generic_probe,
	.remove = generic_remove,
};

static int usb_generic_driver_data;

/* needs to be called with BKL held */
int usb_device_probe(struct device *dev)
{
	struct usb_interface * intf = to_usb_interface(dev);
	struct usb_driver * driver = to_usb_driver(dev->driver);
	const struct usb_device_id *id;
	int error = -ENODEV;

	dev_dbg(dev, "%s\n", __FUNCTION__);

	if (!driver->probe)
		return error;

	id = usb_match_id (intf, driver->id_table);
	if (id) {
		dev_dbg (dev, "%s - got id\n", __FUNCTION__);
		down (&driver->serialize);
		error = driver->probe (intf, id);
		up (&driver->serialize);
	}
	if (!error)
		intf->driver = driver;

	return error;
}

int usb_device_remove(struct device *dev)
{
	struct usb_interface *intf;
	struct usb_driver *driver;

	intf = list_entry(dev,struct usb_interface,dev);
	driver = to_usb_driver(dev->driver);

	down(&driver->serialize);

	if (intf->driver && intf->driver->disconnect)
		intf->driver->disconnect(intf);

	/* if driver->disconnect didn't release the interface */
	if (intf->driver)
		usb_driver_release_interface(driver, intf);

	up(&driver->serialize);

	return 0;
}

/**
 * usb_register - register a USB driver
 * @new_driver: USB operations for the driver
 *
 * Registers a USB driver with the USB core.  The list of unattached
 * interfaces will be rescanned whenever a new driver is added, allowing
 * the new driver to attach to any recognized devices.
 * Returns a negative error code on failure and 0 on success.
 * 
 * NOTE: if you want your driver to use the USB major number, you must call
 * usb_register_dev() to enable that functionality.  This function no longer
 * takes care of that.
 */
int usb_register(struct usb_driver *new_driver)
{
	int retval = 0;

	if (nousb)
		return -ENODEV;

	new_driver->driver.name = (char *)new_driver->name;
	new_driver->driver.bus = &usb_bus_type;
	new_driver->driver.probe = usb_device_probe;
	new_driver->driver.remove = usb_device_remove;

	init_MUTEX(&new_driver->serialize);

	retval = driver_register(&new_driver->driver);

	if (!retval) {
		info("registered new driver %s", new_driver->name);
		usbfs_update_special();
	} else {
		err("problem %d when registering driver %s",
			retval, new_driver->name);
	}

	return retval;
}

/**
 * usb_deregister - unregister a USB driver
 * @driver: USB operations of the driver to unregister
 * Context: !in_interrupt (), must be called with BKL held
 *
 * Unlinks the specified driver from the internal USB driver list.
 * 
 * NOTE: If you called usb_register_dev(), you still need to call
 * usb_deregister_dev() to clean up your driver's allocated minor numbers,
 * this * call will no longer do it for you.
 */
void usb_deregister(struct usb_driver *driver)
{
	info("deregistering driver %s", driver->name);

	driver_unregister (&driver->driver);

	usbfs_update_special();
}

/**
 * usb_ifnum_to_if - get the interface object with a given interface number (usbcore-internal)
 * @dev: the device whose current configuration is considered
 * @ifnum: the desired interface
 *
 * This walks the device descriptor for the currently active configuration
 * and returns a pointer to the interface with that particular interface
 * number, or null.
 *
 * Note that configuration descriptors are not required to assign interface
 * numbers sequentially, so that it would be incorrect to assume that
 * the first interface in that descriptor corresponds to interface zero.
 * This routine helps device drivers avoid such mistakes.
 * However, you should make sure that you do the right thing with any
 * alternate settings available for this interfaces.
 */
struct usb_interface *usb_ifnum_to_if(struct usb_device *dev, unsigned ifnum)
{
	int i;

	for (i = 0; i < dev->actconfig->desc.bNumInterfaces; i++)
		if (dev->actconfig->interface[i].altsetting[0]
				.desc.bInterfaceNumber == ifnum)
			return &dev->actconfig->interface[i];

	return NULL;
}

/**
 * usb_epnum_to_ep_desc - get the endpoint object with a given endpoint number
 * @dev: the device whose current configuration is considered
 * @epnum: the desired endpoint
 *
 * This walks the device descriptor for the currently active configuration,
 * and returns a pointer to the endpoint with that particular endpoint
 * number, or null.
 *
 * Note that interface descriptors are not required to assign endpont
 * numbers sequentially, so that it would be incorrect to assume that
 * the first endpoint in that descriptor corresponds to interface zero.
 * This routine helps device drivers avoid such mistakes.
 */
struct usb_endpoint_descriptor *
usb_epnum_to_ep_desc(struct usb_device *dev, unsigned epnum)
{
	int i, j, k;

	for (i = 0; i < dev->actconfig->desc.bNumInterfaces; i++)
		for (j = 0; j < dev->actconfig->interface[i].num_altsetting; j++)
			for (k = 0; k < dev->actconfig->interface[i]
				.altsetting[j].desc.bNumEndpoints; k++)
				if (epnum == dev->actconfig->interface[i]
						.altsetting[j].endpoint[k]
						.desc.bEndpointAddress)
					return &dev->actconfig->interface[i]
						.altsetting[j].endpoint[k]
						.desc;

	return NULL;
}

/**
 * usb_driver_claim_interface - bind a driver to an interface
 * @driver: the driver to be bound
 * @iface: the interface to which it will be bound
 * @priv: driver data associated with that interface
 *
 * This is used by usb device drivers that need to claim more than one
 * interface on a device when probing (audio and acm are current examples).
 * No device driver should directly modify internal usb_interface or
 * usb_device structure members.
 *
 * Few drivers should need to use this routine, since the most natural
 * way to bind to an interface is to return the private data from
 * the driver's probe() method.  Any driver that does use this must
 * first be sure that no other driver has claimed the interface, by
 * checking with usb_interface_claimed().
 */
void usb_driver_claim_interface(struct usb_driver *driver, struct usb_interface *iface, void* priv)
{
	if (!iface || !driver)
		return;

	// FIXME change API to report an error in this case
	if (iface->driver)
	    err ("%s driver booted %s off interface %p",
	    	driver->name, iface->driver->name, iface);
	else
	    dbg("%s driver claimed interface %p", driver->name, iface);

	iface->driver = driver;
	usb_set_intfdata(iface, priv);
}

/**
 * usb_interface_claimed - returns true iff an interface is claimed
 * @iface: the interface being checked
 *
 * This should be used by drivers to check other interfaces to see if
 * they are available or not.  If another driver has claimed the interface,
 * they may not claim it.  Otherwise it's OK to claim it using
 * usb_driver_claim_interface().
 *
 * Returns true (nonzero) iff the interface is claimed, else false (zero).
 */
int usb_interface_claimed(struct usb_interface *iface)
{
	if (!iface)
		return 0;

	return (iface->driver != NULL);
} /* usb_interface_claimed() */

/**
 * usb_driver_release_interface - unbind a driver from an interface
 * @driver: the driver to be unbound
 * @iface: the interface from which it will be unbound
 * 
 * This should be used by drivers to release their claimed interfaces.
 * It is normally called in their disconnect() methods, and only for
 * drivers that bound to more than one interface in their probe().
 *
 * When the USB subsystem disconnect()s a driver from some interface,
 * it automatically invokes this method for that interface.  That
 * means that even drivers that used usb_driver_claim_interface()
 * usually won't need to call this.
 */
void usb_driver_release_interface(struct usb_driver *driver, struct usb_interface *iface)
{
	/* this should never happen, don't release something that's not ours */
	if (!iface || iface->driver != driver)
		return;

	iface->driver = NULL;
	usb_set_intfdata(iface, NULL);
}

/**
 * usb_match_id - find first usb_device_id matching device or interface
 * @interface: the interface of interest
 * @id: array of usb_device_id structures, terminated by zero entry
 *
 * usb_match_id searches an array of usb_device_id's and returns
 * the first one matching the device or interface, or null.
 * This is used when binding (or rebinding) a driver to an interface.
 * Most USB device drivers will use this indirectly, through the usb core,
 * but some layered driver frameworks use it directly.
 * These device tables are exported with MODULE_DEVICE_TABLE, through
 * modutils and "modules.usbmap", to support the driver loading
 * functionality of USB hotplugging.
 *
 * What Matches:
 *
 * The "match_flags" element in a usb_device_id controls which
 * members are used.  If the corresponding bit is set, the
 * value in the device_id must match its corresponding member
 * in the device or interface descriptor, or else the device_id
 * does not match.
 *
 * "driver_info" is normally used only by device drivers,
 * but you can create a wildcard "matches anything" usb_device_id
 * as a driver's "modules.usbmap" entry if you provide an id with
 * only a nonzero "driver_info" field.  If you do this, the USB device
 * driver's probe() routine should use additional intelligence to
 * decide whether to bind to the specified interface.
 * 
 * What Makes Good usb_device_id Tables:
 *
 * The match algorithm is very simple, so that intelligence in
 * driver selection must come from smart driver id records.
 * Unless you have good reasons to use another selection policy,
 * provide match elements only in related groups, and order match
 * specifiers from specific to general.  Use the macros provided
 * for that purpose if you can.
 *
 * The most specific match specifiers use device descriptor
 * data.  These are commonly used with product-specific matches;
 * the USB_DEVICE macro lets you provide vendor and product IDs,
 * and you can also match against ranges of product revisions.
 * These are widely used for devices with application or vendor
 * specific bDeviceClass values.
 *
 * Matches based on device class/subclass/protocol specifications
 * are slightly more general; use the USB_DEVICE_INFO macro, or
 * its siblings.  These are used with single-function devices
 * where bDeviceClass doesn't specify that each interface has
 * its own class. 
 *
 * Matches based on interface class/subclass/protocol are the
 * most general; they let drivers bind to any interface on a
 * multiple-function device.  Use the USB_INTERFACE_INFO
 * macro, or its siblings, to match class-per-interface style 
 * devices (as recorded in bDeviceClass).
 *  
 * Within those groups, remember that not all combinations are
 * meaningful.  For example, don't give a product version range
 * without vendor and product IDs; or specify a protocol without
 * its associated class and subclass.
 */   
const struct usb_device_id *
usb_match_id(struct usb_interface *interface, const struct usb_device_id *id)
{
	struct usb_host_interface *intf;
	struct usb_device *dev;

	/* proc_connectinfo in devio.c may call us with id == NULL. */
	if (id == NULL)
		return NULL;

	intf = &interface->altsetting [interface->act_altsetting];
	dev = interface_to_usbdev(interface);

	/* It is important to check that id->driver_info is nonzero,
	   since an entry that is all zeroes except for a nonzero
	   id->driver_info is the way to create an entry that
	   indicates that the driver want to examine every
	   device and interface. */
	for (; id->idVendor || id->bDeviceClass || id->bInterfaceClass ||
	       id->driver_info; id++) {

		if ((id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
		    id->idVendor != dev->descriptor.idVendor)
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
		    id->idProduct != dev->descriptor.idProduct)
			continue;

		/* No need to test id->bcdDevice_lo != 0, since 0 is never
		   greater than any unsigned number. */
		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_LO) &&
		    (id->bcdDevice_lo > dev->descriptor.bcdDevice))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_HI) &&
		    (id->bcdDevice_hi < dev->descriptor.bcdDevice))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_CLASS) &&
		    (id->bDeviceClass != dev->descriptor.bDeviceClass))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_SUBCLASS) &&
		    (id->bDeviceSubClass!= dev->descriptor.bDeviceSubClass))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_PROTOCOL) &&
		    (id->bDeviceProtocol != dev->descriptor.bDeviceProtocol))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_CLASS) &&
		    (id->bInterfaceClass != intf->desc.bInterfaceClass))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_SUBCLASS) &&
		    (id->bInterfaceSubClass != intf->desc.bInterfaceSubClass))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_PROTOCOL) &&
		    (id->bInterfaceProtocol != intf->desc.bInterfaceProtocol))
			continue;

		return id;
	}

	return NULL;
}

/**
 * usb_find_interface - find usb_interface pointer for driver and device
 * @drv: the driver whose current configuration is considered
 * @minor: the minor number of the desired device
 *
 * This walks the driver device list and returns a pointer to the interface 
 * with the matching minor.  Note, this only works for devices that share the
 * USB major number.
 */
struct usb_interface *usb_find_interface(struct usb_driver *drv, int minor)
{
	struct list_head *entry;
	struct device *dev;
	struct usb_interface *intf;

	list_for_each(entry, &drv->driver.devices) {
		dev = container_of(entry, struct device, driver_list);

		/* can't look at usb devices, only interfaces */
		if (dev->driver == &usb_generic_driver)
			continue;

		intf = to_usb_interface(dev);
		if (intf->minor == -1)
			continue;
		if (intf->minor == minor)
			return intf;
	}

	/* no device found that matches */
	return NULL;	
}

static int usb_device_match (struct device *dev, struct device_driver *drv)
{
	struct usb_interface *intf;
	struct usb_driver *usb_drv;
	const struct usb_device_id *id;

	/* check for generic driver, which we don't match any device with */
	if (drv == &usb_generic_driver)
		return 0;

	intf = to_usb_interface(dev);

	usb_drv = to_usb_driver(drv);
	id = usb_drv->id_table;
	
	id = usb_match_id (intf, usb_drv->id_table);
	if (id)
		return 1;

	return 0;
}


#ifdef	CONFIG_HOTPLUG

/*
 * USB hotplugging invokes what /proc/sys/kernel/hotplug says
 * (normally /sbin/hotplug) when USB devices get added or removed.
 *
 * This invokes a user mode policy agent, typically helping to load driver
 * or other modules, configure the device, and more.  Drivers can provide
 * a MODULE_DEVICE_TABLE to help with module loading subtasks.
 *
 * We're called either from khubd (the typical case) or from root hub
 * (init, kapmd, modprobe, rmmod, etc), but the agents need to handle
 * delays in event delivery.  Use sysfs (and DEVPATH) to make sure the
 * device (and this configuration!) are still present.
 */
static int usb_hotplug (struct device *dev, char **envp, int num_envp,
			char *buffer, int buffer_size)
{
	struct usb_interface *intf;
	struct usb_device *usb_dev;
	char *scratch;
	int i = 0;
	int length = 0;

	dbg ("%s", __FUNCTION__);

	if (!dev)
		return -ENODEV;

	/* Must check driver_data here, as on remove driver is always NULL */
	if ((dev->driver == &usb_generic_driver) || 
	    (dev->driver_data == &usb_generic_driver_data))
		return 0;

	intf = to_usb_interface(dev);
	usb_dev = interface_to_usbdev (intf);
	
	if (usb_dev->devnum < 0) {
		dbg ("device already deleted ??");
		return -ENODEV;
	}
	if (!usb_dev->bus) {
		dbg ("bus already removed?");
		return -ENODEV;
	}

	scratch = buffer;

#ifdef	CONFIG_USB_DEVICEFS
	/* If this is available, userspace programs can directly read
	 * all the device descriptors we don't tell them about.  Or
	 * even act as usermode drivers.
	 *
	 * FIXME reduce hardwired intelligence here
	 */
	envp [i++] = scratch;
	length += snprintf (scratch, buffer_size - length,
			    "DEVICE=/proc/bus/usb/%03d/%03d",
			    usb_dev->bus->busnum, usb_dev->devnum);
	if ((buffer_size - length <= 0) || (i >= num_envp))
		return -ENOMEM;
	++length;
	scratch += length;
#endif

	/* per-device configurations are common */
	envp [i++] = scratch;
	length += snprintf (scratch, buffer_size - length, "PRODUCT=%x/%x/%x",
			    usb_dev->descriptor.idVendor,
			    usb_dev->descriptor.idProduct,
			    usb_dev->descriptor.bcdDevice);
	if ((buffer_size - length <= 0) || (i >= num_envp))
		return -ENOMEM;
	++length;
	scratch += length;

	/* class-based driver binding models */
	envp [i++] = scratch;
	length += snprintf (scratch, buffer_size - length, "TYPE=%d/%d/%d",
			    usb_dev->descriptor.bDeviceClass,
			    usb_dev->descriptor.bDeviceSubClass,
			    usb_dev->descriptor.bDeviceProtocol);
	if ((buffer_size - length <= 0) || (i >= num_envp))
		return -ENOMEM;
	++length;
	scratch += length;

	if (usb_dev->descriptor.bDeviceClass == 0) {
		int alt = intf->act_altsetting;

		/* 2.4 only exposed interface zero.  in 2.5, hotplug
		 * agents are called for all interfaces, and can use
		 * $DEVPATH/bInterfaceNumber if necessary.
		 */
		envp [i++] = scratch;
		length += snprintf (scratch, buffer_size - length,
			    "INTERFACE=%d/%d/%d",
			    intf->altsetting[alt].desc.bInterfaceClass,
			    intf->altsetting[alt].desc.bInterfaceSubClass,
			    intf->altsetting[alt].desc.bInterfaceProtocol);
		if ((buffer_size - length <= 0) || (i >= num_envp))
			return -ENOMEM;
		++length;
		scratch += length;

	}
	envp [i++] = 0;

	return 0;
}

#else

static int usb_hotplug (struct device *dev, char **envp,
			int num_envp, char *buffer, int buffer_size)
{
	return -ENODEV;
}

#endif	/* CONFIG_HOTPLUG */

/**
 * usb_alloc_dev - allocate a usb device structure (usbcore-internal)
 * @parent: hub to which device is connected
 * @bus: bus used to access the device
 * Context: !in_interrupt ()
 *
 * Only hub drivers (including virtual root hub drivers for host
 * controllers) should ever call this.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 */
struct usb_device *usb_alloc_dev(struct usb_device *parent, struct usb_bus *bus)
{
	struct usb_device *dev;

	dev = kmalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return NULL;

	memset(dev, 0, sizeof(*dev));

	device_initialize(&dev->dev);
	dev->state = USB_STATE_ATTACHED;

	usb_bus_get(bus);

	if (!parent)
		dev->devpath [0] = '0';
	dev->bus = bus;
	dev->parent = parent;
	INIT_LIST_HEAD(&dev->filelist);

	init_MUTEX(&dev->serialize);

	if (dev->bus->op->allocate)
		dev->bus->op->allocate(dev);

	return dev;
}

/**
 * usb_get_dev - increments the reference count of the usb device structure
 * @dev: the device being referenced
 *
 * Each live reference to a device should be refcounted.
 *
 * Drivers for USB interfaces should normally record such references in
 * their probe() methods, when they bind to an interface, and release
 * them by calling usb_put_dev(), in their disconnect() methods.
 *
 * A pointer to the device with the incremented reference counter is returned.
 */
struct usb_device *usb_get_dev (struct usb_device *dev)
{
	struct device *tmp;

	if (!dev)
		return NULL;

	tmp = get_device(&dev->dev);
	if (tmp)        
		return to_usb_device(tmp);
	else
		return NULL;
}

/**
 * usb_put_dev - release a use of the usb device structure
 * @dev: device that's been disconnected
 *
 * Must be called when a user of a device is finished with it.  When the last
 * user of the device calls this function, the memory of the device is freed.
 */
void usb_put_dev(struct usb_device *dev)
{
	if (dev)
		put_device(&dev->dev);
}

/**
 * usb_release_dev - free a usb device structure when all users of it are finished.
 * @dev: device that's been disconnected
 *
 * Will be called only by the device core when all users of this usb device are
 * done.
 */
static void usb_release_dev(struct device *dev)
{
	struct usb_device *udev;

	udev = to_usb_device(dev);

	if (udev->bus && udev->bus->op && udev->bus->op->deallocate)
		udev->bus->op->deallocate(udev);
	usb_destroy_configuration (udev);
	usb_bus_put (udev->bus);
	kfree (udev);
}


static struct usb_device *match_device(struct usb_device *dev,
				       u16 vendor_id, u16 product_id)
{
	struct usb_device *ret_dev = NULL;
	int child;

	dbg("looking at vendor %d, product %d",
	    dev->descriptor.idVendor,
	    dev->descriptor.idProduct);

	/* see if this device matches */
	if ((dev->descriptor.idVendor == vendor_id) &&
	    (dev->descriptor.idProduct == product_id)) {
		dbg ("found the device!");
		ret_dev = usb_get_dev(dev);
		goto exit;
	}

	/* look through all of the children of this device */
	for (child = 0; child < dev->maxchild; ++child) {
		if (dev->children[child]) {
			ret_dev = match_device(dev->children[child],
					       vendor_id, product_id);
			if (ret_dev)
				goto exit;
		}
	}
exit:
	return ret_dev;
}

/**
 * usb_find_device - find a specific usb device in the system
 * @vendor_id: the vendor id of the device to find
 * @product_id: the product id of the device to find
 *
 * Returns a pointer to a struct usb_device if such a specified usb
 * device is present in the system currently.  The usage count of the
 * device will be incremented if a device is found.  Make sure to call
 * usb_put_dev() when the caller is finished with the device.
 *
 * If a device with the specified vendor and product id is not found,
 * NULL is returned.
 */
struct usb_device *usb_find_device(u16 vendor_id, u16 product_id)
{
	struct list_head *buslist;
	struct usb_bus *bus;
	struct usb_device *dev = NULL;
	
	down(&usb_bus_list_lock);
	for (buslist = usb_bus_list.next;
	     buslist != &usb_bus_list; 
	     buslist = buslist->next) {
		bus = container_of(buslist, struct usb_bus, bus_list);
		dev = match_device(bus->root_hub, vendor_id, product_id);
		if (dev)
			goto exit;
	}
exit:
	up(&usb_bus_list_lock);
	return dev;
}

/**
 * usb_get_current_frame_number - return current bus frame number
 * @dev: the device whose bus is being queried
 *
 * Returns the current frame number for the USB host controller
 * used with the given USB device.  This can be used when scheduling
 * isochronous requests.
 *
 * Note that different kinds of host controller have different
 * "scheduling horizons".  While one type might support scheduling only
 * 32 frames into the future, others could support scheduling up to
 * 1024 frames into the future.
 */
int usb_get_current_frame_number(struct usb_device *dev)
{
	return dev->bus->op->get_frame_number (dev);
}

/*-------------------------------------------------------------------*/
/*
 * __usb_get_extra_descriptor() finds a descriptor of specific type in the
 * extra field of the interface and endpoint descriptor structs.
 */

int __usb_get_extra_descriptor(char *buffer, unsigned size, unsigned char type, void **ptr)
{
	struct usb_descriptor_header *header;

	while (size >= sizeof(struct usb_descriptor_header)) {
		header = (struct usb_descriptor_header *)buffer;

		if (header->bLength < 2) {
			err("invalid descriptor length of %d", header->bLength);
			return -1;
		}

		if (header->bDescriptorType == type) {
			*ptr = header;
			return 0;
		}

		buffer += header->bLength;
		size -= header->bLength;
	}
	return -1;
}

/**
 * usb_disconnect - disconnect a device (usbcore-internal)
 * @pdev: pointer to device being disconnected
 * Context: !in_interrupt ()
 *
 * Something got disconnected. Get rid of it, and all of its children.
 *
 * Only hub drivers (including virtual root hub drivers for host
 * controllers) should ever call this.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 */
void usb_disconnect(struct usb_device **pdev)
{
	struct usb_device	*dev = *pdev;
	struct usb_bus		*bus;
	struct usb_operations	*ops;
	int			i;

	might_sleep ();

	if (!dev) {
		pr_debug ("%s nodev\n", __FUNCTION__);
		return;
	}
	bus = dev->bus;
	if (!bus) {
		pr_debug ("%s nobus\n", __FUNCTION__);
		return;
	}
	ops = bus->op;

	*pdev = NULL;

	/* mark the device as inactive, so any further urb submissions for
	 * this device will fail.
	 */
	dev->state = USB_STATE_NOTATTACHED;

	dev_info (&dev->dev, "USB disconnect, address %d\n", dev->devnum);

	/* Free up all the children before we remove this device */
	for (i = 0; i < USB_MAXCHILDREN; i++) {
		struct usb_device **child = dev->children + i;
		if (*child)
			usb_disconnect(child);
	}

	/* disconnect() drivers from interfaces (a key side effect) */
	dev_dbg (&dev->dev, "unregistering interfaces\n");
	if (dev->actconfig) {
		for (i = 0; i < dev->actconfig->desc.bNumInterfaces; i++) {
			struct usb_interface	*interface;

			/* remove this interface */
			interface = &dev->actconfig->interface[i];
			device_unregister(&interface->dev);
		}
	}

	/* deallocate hcd/hardware state */
	if (ops->disable) {
		void	(*disable)(struct usb_device *, int) = ops->disable;

		for (i = 0; i < 15; i++) {
			disable (dev, i);
			disable (dev, USB_DIR_IN | i);
		}
	}

	dev_dbg (&dev->dev, "unregistering device\n");
	/* Free the device number and remove the /proc/bus/usb entry */
	if (dev->devnum > 0) {
		clear_bit(dev->devnum, dev->bus->devmap.devicemap);
		usbfs_remove_device(dev);
	}
	device_unregister(&dev->dev);

	/* Decrement the reference count, it'll auto free everything when */
	/* it hits 0 which could very well be now */
	usb_put_dev(dev);
}

/**
 * usb_connect - pick device address (usbcore-internal)
 * @dev: newly detected device (in DEFAULT state)
 *
 * Picks a device address.  It's up to the hub (or root hub) driver
 * to handle and manage enumeration, starting from the DEFAULT state.
 * Only hub drivers (including virtual root hub drivers for host
 * controllers) should ever call this.
 */
void usb_connect(struct usb_device *dev)
{
	int devnum;
	// FIXME needs locking for SMP!!
	/* why? this is called only from the hub thread, 
	 * which hopefully doesn't run on multiple CPU's simultaneously 8-)
	 * ... it's also called from modprobe/rmmod/apmd threads as part
	 * of virtual root hub init/reinit.  In the init case, the hub code 
	 * won't have seen this, but not so for reinit ... 
	 */
	dev->descriptor.bMaxPacketSize0 = 8;  /* Start off at 8 bytes  */

	/* Try to allocate the next devnum beginning at bus->devnum_next. */
	devnum = find_next_zero_bit(dev->bus->devmap.devicemap, 128, dev->bus->devnum_next);
	if (devnum >= 128)
		devnum = find_next_zero_bit(dev->bus->devmap.devicemap, 128, 1);

	dev->bus->devnum_next = ( devnum >= 127 ? 1 : devnum + 1);

	if (devnum < 128) {
		set_bit(devnum, dev->bus->devmap.devicemap);
		dev->devnum = devnum;
	}
}


// hub-only!! ... and only exported for reset/reinit path.
// otherwise used internally, for usb_new_device()
int usb_set_address(struct usb_device *dev)
{
	int retval;

	if (dev->devnum == 0)
		return -EINVAL;
	if (dev->state != USB_STATE_DEFAULT && dev->state != USB_STATE_ADDRESS)
		return -EINVAL;
	retval = usb_control_msg(dev, usb_snddefctrl(dev), USB_REQ_SET_ADDRESS,
		0, dev->devnum, 0, NULL, 0, HZ * USB_CTRL_SET_TIMEOUT);
	if (retval == 0)
		dev->state = USB_STATE_ADDRESS;
	return retval;
}


/* improve on the default device description, if we can ... and
 * while we're at it, maybe show the vendor and product strings.
 */
static void set_device_description (struct usb_device *dev)
{
	void    *buf;
	int	mfgr = dev->descriptor.iManufacturer;
	int	prod = dev->descriptor.iProduct;
	int	vendor_id = dev->descriptor.idVendor;
	int	product_id = dev->descriptor.idProduct;
	char	*mfgr_str, *prod_str;

	/* set default; keep it if there are no strings, or kmalloc fails */
	sprintf (dev->dev.name, "USB device %04x:%04x",
		 vendor_id, product_id);

	if (!(buf = kmalloc(256 * 2, GFP_KERNEL)))
		return;
	
	prod_str = (char *) buf;
	mfgr_str = (char *) buf + 256;

	if (prod && usb_string (dev, prod, prod_str, 256) > 0) {
#ifdef DEBUG
		dev_printk (KERN_INFO, &dev->dev, "Product: %s\n", prod_str);
#endif
	} else {
		prod_str = 0;
	}

	if (mfgr && usb_string (dev, mfgr, mfgr_str, 256) > 0) {
#ifdef DEBUG
		dev_printk (KERN_INFO, &dev->dev, "Manufacturer: %s\n", mfgr_str);
#endif
	} else {
		mfgr_str = 0;
	}

	/* much like pci ... describe as either:
	 * - both strings:   'product descr (vendor descr)'
	 * - product only:   'product descr (USB device vvvv:pppp)'
	 * - vendor only:    'USB device vvvv:pppp (vendor descr)'
	 * - neither string: 'USB device vvvv:pppp'
	 */

	if (prod_str && mfgr_str) {

		snprintf(dev->dev.name, sizeof dev->dev.name,
			 "%s (%s)", prod_str, mfgr_str);
	} else if (prod_str) {
		snprintf(dev->dev.name, sizeof dev->dev.name,
			 "%s (USB device %04x:%04x)",
			 prod_str, vendor_id, product_id);

	} else if (mfgr_str) {
		snprintf(dev->dev.name, sizeof dev->dev.name,
			 "USB device %04x:%04x (%s)",
			 vendor_id, product_id, mfgr_str);
	}
	//usbprintk("USB connected: %s\n",dev->dev.name);
	kfree(buf);
}

/*
 * By the time we get here, we chose a new device address
 * and is in the default state. We need to identify the thing and
 * get the ball rolling..
 *
 * Returns 0 for success, != 0 for error.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 *
 * Only hub drivers (including virtual root hub drivers for host
 * controllers) should ever call this.
 */
#define NEW_DEVICE_RETRYS	2
#define SET_ADDRESS_RETRYS	2
int usb_new_device(struct usb_device *dev, struct device *parent)
{
	int err = 0;
	int i;
	int j;

	/*
	 * Set the driver for the usb device to point to the "generic" driver.
	 * This prevents the main usb device from being sent to the usb bus
	 * probe function.  Yes, it's a hack, but a nice one :)
	 *
	 * Do it asap, so more driver model stuff (like the device.h message
	 * utilities) can be used in hcd submit/unlink code paths.
	 */
	usb_generic_driver.bus = &usb_bus_type;
	dev->dev.parent = parent;
	dev->dev.driver = &usb_generic_driver;
	dev->dev.bus = &usb_bus_type;
	dev->dev.release = usb_release_dev;
	dev->dev.driver_data = &usb_generic_driver_data;
	usb_get_dev(dev);
	if (dev->dev.bus_id[0] == 0)
		sprintf (&dev->dev.bus_id[0], "%d-%s",
			 dev->bus->busnum, dev->devpath);

	/* dma masks come from the controller; readonly, except to hcd */
	dev->dev.dma_mask = parent->dma_mask;

	/* USB 2.0 section 5.5.3 talks about ep0 maxpacket ...
	 * it's fixed size except for full speed devices.
	 */
	switch (dev->speed) {
	case USB_SPEED_HIGH:		/* fixed at 64 */
		i = 64;
		break;
	case USB_SPEED_FULL:		/* 8, 16, 32, or 64 */
		/* to determine the ep0 maxpacket size, read the first 8
		 * bytes from the device descriptor to get bMaxPacketSize0;
		 * then correct our initial (small) guess.
		 */
		// FALLTHROUGH
	case USB_SPEED_LOW:		/* fixed at 8 */
		i = 8;
		break;
	default:
		return -EINVAL;
	}
	dev->epmaxpacketin [0] = i;
	dev->epmaxpacketout[0] = i;

	for (i = 0; i < NEW_DEVICE_RETRYS; ++i) {

		for (j = 0; j < SET_ADDRESS_RETRYS; ++j) {
			err = usb_set_address(dev);
			if (err >= 0)
				break;
			wait_ms(200);
		}
		if (err < 0) {
			dev_err(&dev->dev, "USB device not accepting new address=%d (error=%d)\n",
				dev->devnum, err);
			dev->state = USB_STATE_DEFAULT;
			clear_bit(dev->devnum, dev->bus->devmap.devicemap);
			dev->devnum = -1;
			return 1;
		}

		wait_ms(10);	/* Let the SET_ADDRESS settle */

		/* high and low speed devices don't need this... */
		err = usb_get_descriptor(dev, USB_DT_DEVICE, 0, &dev->descriptor, 8);
		if (err >= 8)
			break;
		wait_ms(100);
	}

	if (err < 8) {
		if (err < 0)
			dev_err(&dev->dev, "USB device not responding, giving up (error=%d)\n", err);
		else
			dev_err(&dev->dev, "USB device descriptor short read (expected %i, got %i)\n", 8, err);
		clear_bit(dev->devnum, dev->bus->devmap.devicemap);
		dev->devnum = -1;
		return 1;
	}
	if (dev->speed == USB_SPEED_FULL) {
		dev->epmaxpacketin [0] = dev->descriptor.bMaxPacketSize0;
		dev->epmaxpacketout[0] = dev->descriptor.bMaxPacketSize0;
	}

	/* USB device state == addressed ... still not usable */

	err = usb_get_device_descriptor(dev);
	if (err < (signed)sizeof(dev->descriptor)) {
		if (err < 0)
			dev_err(&dev->dev, "unable to get device descriptor (error=%d)\n", err);
		else
			dev_err(&dev->dev, "USB device descriptor short read (expected %Zi, got %i)\n",
				sizeof(dev->descriptor), err);
	
		clear_bit(dev->devnum, dev->bus->devmap.devicemap);
		dev->devnum = -1;
		return 1;
	}

	err = usb_get_configuration(dev);
	if (err < 0) {
		dev_err(&dev->dev, "unable to get device %d configuration (error=%d)\n",
			dev->devnum, err);
		clear_bit(dev->devnum, dev->bus->devmap.devicemap);
		dev->devnum = -1;
		return 1;
	}

	/* we set the default configuration here */
	err = usb_set_configuration(dev, dev->config[0].desc.bConfigurationValue);
	if (err) {
		dev_err(&dev->dev, "failed to set device %d default configuration (error=%d)\n",
			dev->devnum, err);
		clear_bit(dev->devnum, dev->bus->devmap.devicemap);
		dev->devnum = -1;
		return 1;
	}

	/* USB device state == configured ... tell the world! */

	dev_dbg(&dev->dev, "new device strings: Mfr=%d, Product=%d, SerialNumber=%d\n",
		dev->descriptor.iManufacturer, dev->descriptor.iProduct, dev->descriptor.iSerialNumber);
	set_device_description (dev);

#ifdef DEBUG
	if (dev->descriptor.iSerialNumber)
		usb_show_string(dev, "SerialNumber", dev->descriptor.iSerialNumber);
#endif
	/* put into sysfs, with device and config specific files */
	err = device_add (&dev->dev);
	if (err)
		return err;
	usb_create_driverfs_dev_files (dev);

	/* Register all of the interfaces for this device with the driver core.
	 * Remember, interfaces get bound to drivers, not devices. */
	for (i = 0; i < dev->actconfig->desc.bNumInterfaces; i++) {
		struct usb_interface *interface = &dev->actconfig->interface[i];
		struct usb_interface_descriptor *desc;

		desc = &interface->altsetting [interface->act_altsetting].desc;
		interface->dev.parent = &dev->dev;
		interface->dev.driver = NULL;
		interface->dev.bus = &usb_bus_type;
		interface->dev.dma_mask = parent->dma_mask;
		sprintf (&interface->dev.bus_id[0], "%d-%s:%d",
			 dev->bus->busnum, dev->devpath,
			 desc->bInterfaceNumber);
		if (!desc->iInterface
				|| usb_string (dev, desc->iInterface,
					interface->dev.name,
					sizeof interface->dev.name) <= 0) {
			/* typically devices won't bother with interface
			 * descriptions; this is the normal case.  an
			 * interface's driver might describe it better.
			 * (also: iInterface is per-altsetting ...)
			 */
			sprintf (&interface->dev.name[0],
				"usb-%s-%s interface %d",
				dev->bus->bus_name, dev->devpath,
				desc->bInterfaceNumber);
		}
		dev_dbg (&dev->dev, "%s - registering interface %s\n", __FUNCTION__, interface->dev.bus_id);
		device_add (&interface->dev);
		usb_create_driverfs_intf_files (interface);
	}
	/* add a /proc/bus/usb entry */
	usbfs_add_device(dev);

	return 0;
}

/**
 * usb_buffer_alloc - allocate dma-consistent buffer for URB_NO_DMA_MAP
 * @dev: device the buffer will be used with
 * @size: requested buffer size
 * @mem_flags: affect whether allocation may block
 * @dma: used to return DMA address of buffer
 *
 * Return value is either null (indicating no buffer could be allocated), or
 * the cpu-space pointer to a buffer that may be used to perform DMA to the
 * specified device.  Such cpu-space buffers are returned along with the DMA
 * address (through the pointer provided).
 *
 * These buffers are used with URB_NO_DMA_MAP set in urb->transfer_flags to
 * avoid behaviors like using "DMA bounce buffers", or tying down I/O mapping
 * hardware for long idle periods.  The implementation varies between
 * platforms, depending on details of how DMA will work to this device.
 * Using these buffers also helps prevent cacheline sharing problems on
 * architectures where CPU caches are not DMA-coherent.
 *
 * When the buffer is no longer used, free it with usb_buffer_free().
 */
void *usb_buffer_alloc (
	struct usb_device *dev,
	size_t size,
	int mem_flags,
	dma_addr_t *dma
)
{
	if (!dev || !dev->bus || !dev->bus->op || !dev->bus->op->buffer_alloc)
		return 0;
	return dev->bus->op->buffer_alloc (dev->bus, size, mem_flags, dma);
}

/**
 * usb_buffer_free - free memory allocated with usb_buffer_alloc()
 * @dev: device the buffer was used with
 * @size: requested buffer size
 * @addr: CPU address of buffer
 * @dma: DMA address of buffer
 *
 * This reclaims an I/O buffer, letting it be reused.  The memory must have
 * been allocated using usb_buffer_alloc(), and the parameters must match
 * those provided in that allocation request. 
 */
void usb_buffer_free (
	struct usb_device *dev,
	size_t size,
	void *addr,
	dma_addr_t dma
)
{
	if (!dev || !dev->bus || !dev->bus->op || !dev->bus->op->buffer_free)
	    	return;
	dev->bus->op->buffer_free (dev->bus, size, addr, dma);
}

/**
 * usb_buffer_map - create DMA mapping(s) for an urb
 * @urb: urb whose transfer_buffer will be mapped
 *
 * Return value is either null (indicating no buffer could be mapped), or
 * the parameter.  URB_NO_DMA_MAP is added to urb->transfer_flags if the
 * operation succeeds.  If the device is connected to this system through
 * a non-DMA controller, this operation always succeeds.
 *
 * This call would normally be used for an urb which is reused, perhaps
 * as the target of a large periodic transfer, with usb_buffer_dmasync()
 * calls to synchronize memory and dma state.  It may not be used for
 * control requests.
 *
 * Reverse the effect of this call with usb_buffer_unmap().
 */
struct urb *usb_buffer_map (struct urb *urb)
{
	struct usb_bus		*bus;
	struct device		*controller;

	if (!urb
			|| usb_pipecontrol (urb->pipe)
			|| !urb->dev
			|| !(bus = urb->dev->bus)
			|| !(controller = bus->controller))
		return 0;

	if (controller->dma_mask) {
		urb->transfer_dma = dma_map_single (controller,
			urb->transfer_buffer, urb->transfer_buffer_length,
			usb_pipein (urb->pipe)
				? DMA_FROM_DEVICE : DMA_TO_DEVICE);
	// FIXME generic api broken like pci, can't report errors
	// if (urb->transfer_dma == DMA_ADDR_INVALID) return 0;
	} else
		urb->transfer_dma = ~0;
	urb->transfer_flags |= URB_NO_DMA_MAP;
	return urb;
}

/**
 * usb_buffer_dmasync - synchronize DMA and CPU view of buffer(s)
 * @urb: urb whose transfer_buffer will be synchronized
 */
void usb_buffer_dmasync (struct urb *urb)
{
	struct usb_bus		*bus;
	struct device		*controller;

	if (!urb
			|| !(urb->transfer_flags & URB_NO_DMA_MAP)
			|| !urb->dev
			|| !(bus = urb->dev->bus)
			|| !(controller = bus->controller))
		return;

	if (controller->dma_mask)
		dma_sync_single (controller,
			urb->transfer_dma, urb->transfer_buffer_length,
			usb_pipein (urb->pipe)
				? DMA_FROM_DEVICE : DMA_TO_DEVICE);
}

/**
 * usb_buffer_unmap - free DMA mapping(s) for an urb
 * @urb: urb whose transfer_buffer will be unmapped
 *
 * Reverses the effect of usb_buffer_map().
 */
void usb_buffer_unmap (struct urb *urb)
{
	struct usb_bus		*bus;
	struct device		*controller;

	if (!urb
			|| !(urb->transfer_flags & URB_NO_DMA_MAP)
			|| !urb->dev
			|| !(bus = urb->dev->bus)
			|| !(controller = bus->controller))
		return;

	if (controller->dma_mask)
		dma_unmap_single (controller,
			urb->transfer_dma, urb->transfer_buffer_length,
			usb_pipein (urb->pipe)
				? DMA_FROM_DEVICE : DMA_TO_DEVICE);
	urb->transfer_flags &= ~URB_NO_DMA_MAP;
}

/**
 * usb_buffer_map_sg - create scatterlist DMA mapping(s) for an endpoint
 * @dev: device to which the scatterlist will be mapped
 * @pipe: endpoint defining the mapping direction
 * @sg: the scatterlist to map
 * @nents: the number of entries in the scatterlist
 *
 * Return value is either < 0 (indicating no buffers could be mapped), or
 * the number of DMA mapping array entries in the scatterlist.
 *
 * The caller is responsible for placing the resulting DMA addresses from
 * the scatterlist into URB transfer buffer pointers, and for setting the
 * URB_NO_DMA_MAP transfer flag in each of those URBs.
 *
 * Top I/O rates come from queuing URBs, instead of waiting for each one
 * to complete before starting the next I/O.   This is particularly easy
 * to do with scatterlists.  Just allocate and submit one URB for each DMA
 * mapping entry returned, stopping on the first error or when all succeed.
 * Better yet, use the usb_sg_*() calls, which do that (and more) for you.
 *
 * This call would normally be used when translating scatterlist requests,
 * rather than usb_buffer_map(), since on some hardware (with IOMMUs) it
 * may be able to coalesce mappings for improved I/O efficiency.
 *
 * Reverse the effect of this call with usb_buffer_unmap_sg().
 */
int usb_buffer_map_sg (struct usb_device *dev, unsigned pipe,
		struct scatterlist *sg, int nents)
{
	struct usb_bus		*bus;
	struct device		*controller;

	if (!dev
			|| usb_pipecontrol (pipe)
			|| !(bus = dev->bus)
			|| !(controller = bus->controller)
			|| !controller->dma_mask)
		return -1;

	// FIXME generic api broken like pci, can't report errors
	return dma_map_sg (controller, sg, nents,
			usb_pipein (pipe) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
}

/**
 * usb_buffer_dmasync_sg - synchronize DMA and CPU view of scatterlist buffer(s)
 * @dev: device to which the scatterlist will be mapped
 * @pipe: endpoint defining the mapping direction
 * @sg: the scatterlist to synchronize
 * @n_hw_ents: the positive return value from usb_buffer_map_sg
 *
 * Use this when you are re-using a scatterlist's data buffers for
 * another USB request.
 */
void usb_buffer_dmasync_sg (struct usb_device *dev, unsigned pipe,
		struct scatterlist *sg, int n_hw_ents)
{
	struct usb_bus		*bus;
	struct device		*controller;

	if (!dev
			|| !(bus = dev->bus)
			|| !(controller = bus->controller)
			|| !controller->dma_mask)
		return;

	dma_sync_sg (controller, sg, n_hw_ents,
			usb_pipein (pipe) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
}

/**
 * usb_buffer_unmap_sg - free DMA mapping(s) for a scatterlist
 * @dev: device to which the scatterlist will be mapped
 * @pipe: endpoint defining the mapping direction
 * @sg: the scatterlist to unmap
 * @n_hw_ents: the positive return value from usb_buffer_map_sg
 *
 * Reverses the effect of usb_buffer_map_sg().
 */
void usb_buffer_unmap_sg (struct usb_device *dev, unsigned pipe,
		struct scatterlist *sg, int n_hw_ents)
{
	struct usb_bus		*bus;
	struct device		*controller;

	if (!dev
			|| !(bus = dev->bus)
			|| !(controller = bus->controller)
			|| !controller->dma_mask)
		return;

	dma_unmap_sg (controller, sg, n_hw_ents,
			usb_pipein (pipe) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
}


struct bus_type usb_bus_type = {
	.name =		"usb",
	.match =	usb_device_match,
	.hotplug =	usb_hotplug,
};

#ifndef MODULE

static int __init usb_setup_disable(char *str)
{
	nousb = 1;
	return 1;
}

/* format to disable USB on kernel command line is: nousb */
__setup("nousb", usb_setup_disable);

#endif

/*
 * for external read access to <nousb>
 */
int usb_disabled(void)
{
	return nousb;
}

/*
 * Init
 */
static int __init usb_init(void)
{
	if (nousb) {
		info("USB support disabled\n");
		return 0;
	}

	bus_register(&usb_bus_type);
	usb_major_init();
	usbfs_init();
	usb_hub_init();

	driver_register(&usb_generic_driver);

	return 0;
}

/*
 * Cleanup
 */
static void __exit usb_exit(void)
{
	/* This will matter if shutdown/reboot does exitcalls. */
	if (nousb)
		return;

	driver_unregister(&usb_generic_driver);
	usb_major_cleanup();
	usbfs_cleanup();
	usb_hub_cleanup();
	bus_unregister(&usb_bus_type);
}

subsys_initcall(usb_init);
module_exit(usb_exit);

/*
 * USB may be built into the kernel or be built as modules.
 * These symbols are exported for device (or host controller)
 * driver modules to use.
 */
EXPORT_SYMBOL(usb_epnum_to_ep_desc);

EXPORT_SYMBOL(usb_register);
EXPORT_SYMBOL(usb_deregister);
EXPORT_SYMBOL(usb_disabled);

EXPORT_SYMBOL(usb_device_probe);
EXPORT_SYMBOL(usb_device_remove);

EXPORT_SYMBOL(usb_alloc_dev);
EXPORT_SYMBOL(usb_put_dev);
EXPORT_SYMBOL(usb_get_dev);
EXPORT_SYMBOL(usb_hub_tt_clear_buffer);

EXPORT_SYMBOL(usb_driver_claim_interface);
EXPORT_SYMBOL(usb_interface_claimed);
EXPORT_SYMBOL(usb_driver_release_interface);
EXPORT_SYMBOL(usb_match_id);
EXPORT_SYMBOL(usb_find_interface);
EXPORT_SYMBOL(usb_ifnum_to_if);

EXPORT_SYMBOL(usb_new_device);
EXPORT_SYMBOL(usb_reset_device);
EXPORT_SYMBOL(usb_connect);
EXPORT_SYMBOL(usb_disconnect);

EXPORT_SYMBOL(__usb_get_extra_descriptor);

EXPORT_SYMBOL(usb_find_device);
EXPORT_SYMBOL(usb_get_current_frame_number);

EXPORT_SYMBOL (usb_buffer_alloc);
EXPORT_SYMBOL (usb_buffer_free);

EXPORT_SYMBOL (usb_buffer_map);
EXPORT_SYMBOL (usb_buffer_dmasync);
EXPORT_SYMBOL (usb_buffer_unmap);

EXPORT_SYMBOL (usb_buffer_map_sg);
EXPORT_SYMBOL (usb_buffer_dmasync_sg);
EXPORT_SYMBOL (usb_buffer_unmap_sg);

MODULE_LICENSE("GPL");
