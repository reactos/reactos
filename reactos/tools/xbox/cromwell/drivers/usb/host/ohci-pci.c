/*
 * OHCI HCD (Host Controller Driver) for USB.
 *
 * (C) Copyright 1999 Roman Weissgaerber <weissg@vienna.at>
 * (C) Copyright 2000-2002 David Brownell <dbrownell@users.sourceforge.net>
 * 
 * [ Initialisation is based on Linus'  ]
 * [ uhci code and gregs ohci fragments ]
 * [ (C) Copyright 1999 Linus Torvalds  ]
 * [ (C) Copyright 1999 Gregory P. Smith]
 * 
 * PCI Bus Glue
 *
 * This file is licenced under the GPL.
 */
 
#ifdef CONFIG_PMAC_PBOOK
#include <asm/machdep.h>
#include <asm/pmac_feature.h>
#include <asm/pci-bridge.h>
#include <asm/prom.h>
#ifndef CONFIG_PM
#	define CONFIG_PM
#endif
#endif

#ifndef CONFIG_PCI
#error "This file is PCI bus glue.  CONFIG_PCI must be defined."
#endif

/*-------------------------------------------------------------------------*/

static int __devinit
ohci_pci_start (struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci (hcd);
	int		ret;

	if (hcd->pdev) {
		ohci->hcca = pci_alloc_consistent (hcd->pdev,
				sizeof *ohci->hcca, &ohci->hcca_dma);
		if (!ohci->hcca)
			return -ENOMEM;

		/* AMD 756, for most chips (early revs), corrupts register
		 * values on read ... so enable the vendor workaround.
		 */
		if (hcd->pdev->vendor == PCI_VENDOR_ID_AMD
				&& hcd->pdev->device == 0x740c) {
			ohci->flags = OHCI_QUIRK_AMD756;
			ohci_info (ohci, "AMD756 erratum 4 workaround\n");
		}

		/* FIXME for some of the early AMD 760 southbridges, OHCI
		 * won't work at all.  blacklist them.
		 */

		/* Apple's OHCI driver has a lot of bizarre workarounds
		 * for this chip.  Evidently control and bulk lists
		 * can get confused.  (B&W G3 models, and ...)
		 */
		else if (hcd->pdev->vendor == PCI_VENDOR_ID_OPTI
				&& hcd->pdev->device == 0xc861) {
			ohci_info (ohci,
				"WARNING: OPTi workarounds unavailable\n");
		}

		/* Check for NSC87560. We have to look at the bridge (fn1) to
		 * identify the USB (fn2). This quirk might apply to more or
		 * even all NSC stuff.
		 */
		else if (hcd->pdev->vendor == PCI_VENDOR_ID_NS) {
			struct pci_dev	*b, *hc;

			hc = hcd->pdev;
			b  = pci_find_slot (hc->bus->number,
					PCI_DEVFN (PCI_SLOT (hc->devfn), 1));
			if (b && b->device == PCI_DEVICE_ID_NS_87560_LIO
					&& b->vendor == PCI_VENDOR_ID_NS) {
				ohci->flags |= OHCI_QUIRK_SUPERIO;
				ohci_info (ohci, "Using NSC SuperIO setup\n");
			}
		}
	
	}

        memset (ohci->hcca, 0, sizeof (struct ohci_hcca));
	if ((ret = ohci_mem_init (ohci)) < 0) {
		ohci_stop (hcd);
		return ret;
	}
	ohci->regs = hcd->regs;

	if (hc_reset (ohci) < 0) {
		ohci_stop (hcd);
		return -ENODEV;
	}

	if (hc_start (ohci) < 0) {
		ohci_err (ohci, "can't start\n");
		ohci_stop (hcd);
		return -EBUSY;
	}

#ifdef	DEBUG
	ohci_dump (ohci, 1);
#endif
	return 0;
}

#ifdef	CONFIG_PM

static int ohci_pci_suspend (struct usb_hcd *hcd, u32 state)
{
	struct ohci_hcd		*ohci = hcd_to_ohci (hcd);
	unsigned long		flags;
	u16			cmd;

	if ((ohci->hc_control & OHCI_CTRL_HCFS) != OHCI_USB_OPER) {
		ohci_dbg (ohci, "can't suspend (state is %s)\n",
			hcfs2string (ohci->hc_control & OHCI_CTRL_HCFS));
		return -EIO;
	}

	/* act as if usb suspend can always be used */
	ohci_dbg (ohci, "suspend to %d\n", state);
	ohci->sleeping = 1;

	/* First stop processing */
  	spin_lock_irqsave (&ohci->lock, flags);
	ohci->hc_control &=
		~(OHCI_CTRL_PLE|OHCI_CTRL_CLE|OHCI_CTRL_BLE|OHCI_CTRL_IE);
	writel (ohci->hc_control, &ohci->regs->control);
	writel (OHCI_INTR_SF, &ohci->regs->intrstatus);
	(void) readl (&ohci->regs->intrstatus);
  	spin_unlock_irqrestore (&ohci->lock, flags);

	/* Wait a frame or two */
	mdelay (1);
	if (!readl (&ohci->regs->intrstatus) & OHCI_INTR_SF)
		mdelay (1);
		
#ifdef CONFIG_PMAC_PBOOK
	if (_machine == _MACH_Pmac)
		disable_irq (hcd->pdev->irq);
 	/* else, 2.4 assumes shared irqs -- don't disable */
#endif

	/* Enable remote wakeup */
	writel (readl (&ohci->regs->intrenable) | OHCI_INTR_RD,
		&ohci->regs->intrenable);

	/* Suspend chip and let things settle down a bit */
 	ohci->hc_control = OHCI_USB_SUSPEND;
 	writel (ohci->hc_control, &ohci->regs->control);
	(void) readl (&ohci->regs->control);
	mdelay (500); /* No schedule here ! */

	switch (readl (&ohci->regs->control) & OHCI_CTRL_HCFS) {
		case OHCI_USB_RESET:
			ohci_dbg (ohci, "suspend->reset ?\n");
			break;
		case OHCI_USB_RESUME:
			ohci_dbg (ohci, "suspend->resume ?\n");
			break;
		case OHCI_USB_OPER:
			ohci_dbg (ohci, "suspend->operational ?\n");
			break;
		case OHCI_USB_SUSPEND:
			ohci_dbg (ohci, "suspended\n");
			break;
	}

	/* In some rare situations, Apple's OHCI have happily trashed
	 * memory during sleep. We disable its bus master bit during
	 * suspend
	 */
	pci_read_config_word (hcd->pdev, PCI_COMMAND, &cmd);
	cmd &= ~PCI_COMMAND_MASTER;
	pci_write_config_word (hcd->pdev, PCI_COMMAND, cmd);
#ifdef CONFIG_PMAC_PBOOK
	{
	   	struct device_node	*of_node;
 
		/* Disable USB PAD & cell clock */
		of_node = pci_device_to_OF_node (hcd->pdev);
		if (of_node)
			pmac_call_feature(PMAC_FTR_USB_ENABLE, of_node, 0, 0);
	}
#endif
	return 0;
}


static int ohci_pci_resume (struct usb_hcd *hcd)
{
	struct ohci_hcd		*ohci = hcd_to_ohci (hcd);
	int			temp;
	int			retval = 0;
	unsigned long		flags;

#ifdef CONFIG_PMAC_PBOOK
	{
		struct device_node *of_node;

		/* Re-enable USB PAD & cell clock */
		of_node = pci_device_to_OF_node (hcd->pdev);
		if (of_node)
			pmac_call_feature (PMAC_FTR_USB_ENABLE, of_node, 0, 1);
	}
#endif
	/* did we suspend, or were we powered off? */
	ohci->hc_control = readl (&ohci->regs->control);
	temp = ohci->hc_control & OHCI_CTRL_HCFS;

#ifdef DEBUG
	/* the registers may look crazy here */
	ohci_dump_status (ohci, 0, 0);
#endif

	/* Re-enable bus mastering */
	pci_set_master (ohci->hcd.pdev);
	
	switch (temp) {

	case OHCI_USB_RESET:	// lost power
		ohci_info (ohci, "USB restart\n");
		retval = hc_restart (ohci);
		break;

	case OHCI_USB_SUSPEND:	// host wakeup
	case OHCI_USB_RESUME:	// remote wakeup
		ohci_info (ohci, "USB continue from %s wakeup\n",
			 (temp == OHCI_USB_SUSPEND)
				? "host" : "remote");
		ohci->hc_control = OHCI_USB_RESUME;
		writel (ohci->hc_control, &ohci->regs->control);
		(void) readl (&ohci->regs->control);
		mdelay (20); /* no schedule here ! */
		/* Some controllers (lucent) need a longer delay here */
		mdelay (15);

		temp = readl (&ohci->regs->control);
		temp = ohci->hc_control & OHCI_CTRL_HCFS;
		if (temp != OHCI_USB_RESUME) {
			ohci_err (ohci, "controller won't resume\n");
			ohci->disabled = 1;
			retval = -EIO;
			break;
		}

		/* Some chips likes being resumed first */
		writel (OHCI_USB_OPER, &ohci->regs->control);
		(void) readl (&ohci->regs->control);
		mdelay (3);

		/* Then re-enable operations */
		spin_lock_irqsave (&ohci->lock, flags);
		ohci->disabled = 0;
		ohci->sleeping = 0;
		ohci->hc_control = OHCI_CONTROL_INIT | OHCI_USB_OPER;
		if (!ohci->ed_rm_list) {
			if (ohci->ed_controltail)
				ohci->hc_control |= OHCI_CTRL_CLE;
			if (ohci->ed_bulktail)
				ohci->hc_control |= OHCI_CTRL_BLE;
		}
		hcd->state = USB_STATE_READY;
		writel (ohci->hc_control, &ohci->regs->control);

		/* trigger a start-frame interrupt (why?) */
		writel (OHCI_INTR_SF, &ohci->regs->intrstatus);
		writel (OHCI_INTR_SF, &ohci->regs->intrenable);

		/* Check for a pending done list */
		writel (OHCI_INTR_WDH, &ohci->regs->intrdisable);	
		(void) readl (&ohci->regs->intrdisable);
		spin_unlock_irqrestore (&ohci->lock, flags);

#ifdef CONFIG_PMAC_PBOOK
		if (_machine == _MACH_Pmac)
			enable_irq (hcd->pdev->irq);
#endif
		if (ohci->hcca->done_head)
			dl_done_list (ohci, dl_reverse_done_list (ohci), NULL);
		writel (OHCI_INTR_WDH, &ohci->regs->intrenable); 

		/* assume there are TDs on the bulk and control lists */
		writel (OHCI_BLF | OHCI_CLF, &ohci->regs->cmdstatus);

// ohci_dump_status (ohci);
ohci_dbg (ohci, "sleeping = %d, disabled = %d\n",
		ohci->sleeping, ohci->disabled);
		break;

	default:
		ohci_warn (ohci, "odd PCI resume\n");
	}
	return retval;
}

#endif	/* CONFIG_PM */


/*-------------------------------------------------------------------------*/

static const struct hc_driver ohci_pci_hc_driver = {
	.description =		hcd_name,

	/*
	 * generic hardware linkage
	 */
	.irq =			ohci_irq,
	.flags =		HCD_MEMORY | HCD_USB11,

	/*
	 * basic lifecycle operations
	 */
	.start =		ohci_pci_start,
#ifdef	CONFIG_PM
	.suspend =		ohci_pci_suspend,
	.resume =		ohci_pci_resume,
#endif
	.stop =			ohci_stop,

	/*
	 * memory lifecycle (except per-request)
	 */
	.hcd_alloc =		ohci_hcd_alloc,
	.hcd_free =		ohci_hcd_free,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ohci_urb_enqueue,
	.urb_dequeue =		ohci_urb_dequeue,
	.endpoint_disable =	ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ohci_hub_status_data,
	.hub_control =		ohci_hub_control,
};

/*-------------------------------------------------------------------------*/

static const struct pci_device_id __devinitdata pci_ids [] = { {

	/* handle any USB OHCI controller */
	.class =	(PCI_CLASS_SERIAL_USB << 8) | 0x10,
	.class_mask =	~0,
	.driver_data =	(unsigned long) &ohci_pci_hc_driver,

	/* no matter who makes it */
	.vendor =	PCI_ANY_ID,
	.device =	PCI_ANY_ID,
	.subvendor =	PCI_ANY_ID,
	.subdevice =	PCI_ANY_ID,

	}, { /* end: all zeroes */ }
};
MODULE_DEVICE_TABLE (pci, pci_ids);

/* pci driver glue; this is a "new style" PCI driver module */
static struct pci_driver ohci_pci_driver = {
	.name =		(char *) hcd_name,
	.id_table =	pci_ids,

	.probe =	usb_hcd_pci_probe,
	.remove =	usb_hcd_pci_remove,

#ifdef	CONFIG_PM
	.suspend =	usb_hcd_pci_suspend,
	.resume =	usb_hcd_pci_resume,
#endif
};

 
static int __init ohci_hcd_pci_init (void) 
{
	printk (KERN_DEBUG "%s: " DRIVER_INFO " (PCI)\n", hcd_name);
	if (usb_disabled())
		return -ENODEV;

	printk (KERN_DEBUG "%s: block sizes: ed %Zd td %Zd\n", hcd_name,
		sizeof (struct ed), sizeof (struct td));
	return pci_module_init (&ohci_pci_driver);
}
module_init (ohci_hcd_pci_init);

/*-------------------------------------------------------------------------*/

static void __exit ohci_hcd_pci_cleanup (void) 
{	
	pci_unregister_driver (&ohci_pci_driver);
}
module_exit (ohci_hcd_pci_cleanup);
