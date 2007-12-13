/*
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
#else
	#undef DEBUG
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/usb.h>
#include "hcd.h"
#else
#define DEBUG
#include "../usb_wrapper.h"
#include "hcd.h"
#endif


/* PCI-based HCs are normal, but custom bus glue should be ok */


/*-------------------------------------------------------------------------*/

/* configure so an HC device and id are always provided */
/* always called with process context; sleeping is OK */

/**
 * usb_hcd_pci_probe - initialize PCI-based HCDs
 * @dev: USB Host Controller being probed
 * @id: pci hotplug id connecting controller to HCD framework
 * Context: !in_interrupt()
 *
 * Allocates basic PCI resources for this USB host controller, and
 * then invokes the start() method for the HCD associated with it
 * through the hotplug entry's driver_data.
 *
 * Store this function in the HCD's struct pci_driver as probe().
 */
int usb_hcd_pci_probe (struct pci_dev *dev, const struct pci_device_id *id)
{
	struct hc_driver	*driver;
	unsigned long		resource, len;
	void			*base;
	struct usb_hcd		*hcd;
	int			retval, region;
	char			buf [8];
	//char			*bufp = buf;

	if (usb_disabled())
		return -ENODEV;

	if (!id || !(driver = (struct hc_driver *) id->driver_data))
		return -EINVAL;

	if (pci_enable_device (dev) < 0)
		return -ENODEV;

        if (!dev->irq) {
        	err ("Found HC with no IRQ.  Check BIOS/PCI %s setup!",
			dev->slot_name);
   	        return -ENODEV;
        }

	if (driver->flags & HCD_MEMORY) {	// EHCI, OHCI
		region = 0;
		resource = pci_resource_start (dev, 0);
		len = pci_resource_len (dev, 0);
		if (!request_mem_region (resource, len, driver->description)) {
			dbg ("controller already in use");
			return -EBUSY;
		}
		base = ioremap_nocache (resource, len);
		if (base == NULL) {
			dbg ("error mapping memory");
			retval = -EFAULT;
clean_1:
			release_mem_region (resource, len);
			err ("init %s fail, %d", dev->slot_name, retval);
			return retval;
		}

	} else { 				// UHCI
		resource = len = 0;
		for (region = 0; region < PCI_ROM_RESOURCE; region++) {
			if (!(pci_resource_flags (dev, region) & IORESOURCE_IO))
				continue;

			resource = pci_resource_start (dev, region);
			len = pci_resource_len (dev, region);
			if (request_region (resource, len,
					driver->description))
				break;
		}
		if (region == PCI_ROM_RESOURCE) {
			dbg ("no i/o regions available");
			return -EBUSY;
		}
		base = (void *) resource;
	}

	// driver->start(), later on, will transfer device from
	// control by SMM/BIOS to control by Linux (if needed)

	pci_set_master (dev);

	hcd = driver->hcd_alloc ();
	if (hcd == NULL){
		dbg ("hcd alloc fail");
		retval = -ENOMEM;
clean_2:
		if (driver->flags & HCD_MEMORY) {
			iounmap (base);
			goto clean_1;
		} else {
			release_region (resource, len);
			err ("init %s fail, %d", dev->slot_name, retval);
			return retval;
		}
	}
	pci_set_drvdata (dev, hcd);
	hcd->driver = driver;
	hcd->description = driver->description;
	hcd->pdev = dev;
	hcd->self.bus_name = dev->slot_name;
	hcd->product_desc = dev->dev.name;
	hcd->self.controller = &dev->dev;
	hcd->controller = hcd->self.controller;

	if ((retval = hcd_buffer_create (hcd)) != 0) {
clean_3:
		driver->hcd_free (hcd);
		goto clean_2;
	}

	dev_info (hcd->controller, "%s\n", hcd->product_desc);

#ifndef __sparc__
	sprintf (buf, "%d", dev->irq);
#else
	bufp = __irq_itoa(dev->irq);
#endif
	if (request_irq (dev->irq, usb_hcd_irq, SA_SHIRQ, hcd->description, hcd)
			!= 0) {
		dev_err (hcd->controller,
				"request interrupt %s failed\n", bufp);
		retval = -EBUSY;
		goto clean_3;
	}
	hcd->irq = dev->irq;

	hcd->regs = base;
	hcd->region = region;
	dev_info (hcd->controller, "irq %s, %s %p\n", bufp,
		(driver->flags & HCD_MEMORY) ? "pci mem" : "io base",
		base);

	usb_bus_init (&hcd->self);
	hcd->self.op = &usb_hcd_operations;
	hcd->self.hcpriv = (void *) hcd;

	INIT_LIST_HEAD (&hcd->dev_list);

	usb_register_bus (&hcd->self);

	if ((retval = driver->start (hcd)) < 0)
		usb_hcd_pci_remove (dev);

	return retval;
} 
EXPORT_SYMBOL (usb_hcd_pci_probe);


/* may be called without controller electrically present */
/* may be called with controller, bus, and devices active */

/**
 * usb_hcd_pci_remove - shutdown processing for PCI-based HCDs
 * @dev: USB Host Controller being removed
 * Context: !in_interrupt()
 *
 * Reverses the effect of usb_hcd_pci_probe(), first invoking
 * the HCD's stop() method.  It is always called from a thread
 * context, normally "rmmod", "apmd", or something similar.
 *
 * Store this function in the HCD's struct pci_driver as remove().
 */
void usb_hcd_pci_remove (struct pci_dev *dev)
{
	struct usb_hcd		*hcd;
	struct usb_device	*hub;

	hcd = pci_get_drvdata(dev);
	if (!hcd)
		return;
	dev_info (hcd->controller, "remove, state %x\n", hcd->state);

	if (in_interrupt ())
		BUG ();

	hub = hcd->self.root_hub;
	hcd->state = USB_STATE_QUIESCING;

	dev_dbg (hcd->controller, "roothub graceful disconnect\n");
	usb_disconnect (&hub);

	hcd->driver->stop (hcd);
	hcd_buffer_destroy (hcd);
	hcd->state = USB_STATE_HALT;
	pci_set_drvdata (dev, 0);

	free_irq (hcd->irq, hcd);
	if (hcd->driver->flags & HCD_MEMORY) {
		iounmap (hcd->regs);
		release_mem_region (pci_resource_start (dev, 0),
			pci_resource_len (dev, 0));
	} else {
		release_region (pci_resource_start (dev, hcd->region),
			pci_resource_len (dev, hcd->region));
	}

	usb_deregister_bus (&hcd->self);
	if (atomic_read (&hcd->self.refcnt) != 1) {
		dev_warn (hcd->controller,
			"dangling refs (%d) to bus %d!\n",
			atomic_read (&hcd->self.refcnt) - 1,
			hcd->self.busnum);
	}
	hcd->driver->hcd_free (hcd);
}
EXPORT_SYMBOL (usb_hcd_pci_remove);


#ifdef	CONFIG_PM

/*
 * Some "sleep" power levels imply updating struct usb_driver
 * to include a callback asking hcds to do their bit by checking
 * if all the drivers can suspend.  Gets involved with remote wakeup.
 *
 * If there are pending urbs, then HCs will need to access memory,
 * causing extra power drain.  New sleep()/wakeup() PM calls might
 * be needed, beyond PCI suspend()/resume().  The root hub timer
 * still be accessing memory though ...
 *
 * FIXME:  USB should have some power budgeting support working with
 * all kinds of hubs.
 *
 * FIXME:  This assumes only D0->D3 suspend and D3->D0 resume.
 * D1 and D2 states should do something, yes?
 *
 * FIXME:  Should provide generic enable_wake(), calling pci_enable_wake()
 * for all supported states, so that USB remote wakeup can work for any
 * devices that support it (and are connected via powered hubs).
 *
 * FIXME:  resume doesn't seem to work right any more...
 */


// 2.4 kernels have issued concurrent resumes (w/APM)
// we defend against that error; PCI doesn't yet.

/**
 * usb_hcd_pci_suspend - power management suspend of a PCI-based HCD
 * @dev: USB Host Controller being suspended
 *
 * Store this function in the HCD's struct pci_driver as suspend().
 */
int usb_hcd_pci_suspend (struct pci_dev *dev, u32 state)
{
	struct usb_hcd		*hcd;
	int			retval;

	hcd = pci_get_drvdata(dev);
	dev_info (hcd->controller, "suspend to state %d\n", state);

	pci_save_state (dev, hcd->pci_state);

	// FIXME for all connected devices, leaf-to-root:
	// driver->suspend()
	// proposed "new 2.5 driver model" will automate that

	/* driver may want to disable DMA etc */
	retval = hcd->driver->suspend (hcd, state);
	hcd->state = USB_STATE_SUSPENDED;

 	pci_set_power_state (dev, state);
	return retval;
}
EXPORT_SYMBOL (usb_hcd_pci_suspend);

/**
 * usb_hcd_pci_resume - power management resume of a PCI-based HCD
 * @dev: USB Host Controller being resumed
 *
 * Store this function in the HCD's struct pci_driver as resume().
 */
int usb_hcd_pci_resume (struct pci_dev *dev)
{
	struct usb_hcd		*hcd;
	int			retval;

	hcd = pci_get_drvdata(dev);
	dev_info (hcd->controller, "resume\n");

	/* guard against multiple resumes (APM bug?) */
	atomic_inc (&hcd->resume_count);
	if (atomic_read (&hcd->resume_count) != 1) {
		dev_err (hcd->controller, "concurrent PCI resumes\n");
		retval = 0;
		goto done;
	}

	retval = -EBUSY;
	if (hcd->state != USB_STATE_SUSPENDED) {
		dev_dbg (hcd->controller, "can't resume, not suspended!\n");
		goto done;
	}
	hcd->state = USB_STATE_RESUMING;

	pci_set_power_state (dev, 0);
	pci_restore_state (dev, hcd->pci_state);

	retval = hcd->driver->resume (hcd);
	if (!HCD_IS_RUNNING (hcd->state)) {
		dev_dbg (hcd->controller, "resume fail, retval %d\n", retval);
		usb_hc_died (hcd);
// FIXME:  recover, reset etc.
	} else {
		// FIXME for all connected devices, root-to-leaf:
		// driver->resume ();
		// proposed "new 2.5 driver model" will automate that
	}

done:
	atomic_dec (&hcd->resume_count);
	return retval;
}
EXPORT_SYMBOL (usb_hcd_pci_resume);

#endif	/* CONFIG_PM */


