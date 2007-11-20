/**
 * ohci.c - USB driver stack project
 *
 * Copyright (c) 2007 Aleksey Bragin <aleksey@reactos.org>
 *                    based on some code by Zhiming <mypublic99@yahoo.com>
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (in the main directory of the distribution, the file
 * COPYING); if not, write to the Free Software Foundation,Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "usbdriver.h"
#include "ehci.h"
#include "ohci.h"

PDEVICE_OBJECT ohci_alloc(PDRIVER_OBJECT drvr_obj, PUNICODE_STRING reg_path,
                          ULONG bus_addr, PUSB_DEV_MANAGER dev_mgr);
//BOOLEAN ohci_release(PDEVICE_OBJECT pdev);
//static VOID ohci_stop(PEHCI_DEV ehci);
PDEVICE_OBJECT ohci_probe(PDRIVER_OBJECT drvr_obj, PUNICODE_STRING reg_path,
                          PUSB_DEV_MANAGER dev_mgr);
PDEVICE_OBJECT ohci_create_device(PDRIVER_OBJECT drvr_obj, PUSB_DEV_MANAGER dev_mgr);
//BOOLEAN ohci_delete_device(PDEVICE_OBJECT pdev);
//VOID ohci_get_capabilities(PEHCI_DEV ehci, PBYTE base);
//BOOLEAN NTAPI ohci_isr(PKINTERRUPT interrupt, PVOID context);
//BOOLEAN ohci_start(PHCD hcd);
VOID ohci_init_hcd_interface(POHCI_DEV ohci);
BOOLEAN ohci_rh_reset_port(PHCD hcd, UCHAR port_idx);

// shared with EHCI
NTSTATUS ehci_dispatch_irp(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp);
PUSB_DEV_MANAGER ehci_get_dev_mgr(PHCD hcd);
VOID ehci_set_dev_mgr(PHCD hcd, PUSB_DEV_MANAGER dev_mgr);
VOID ehci_set_id(PHCD hcd, UCHAR id);
UCHAR ehci_get_id(PHCD hcd);
UCHAR ehci_alloc_addr(PHCD hcd);
VOID ehci_free_addr(PHCD hcd, UCHAR addr);

BOOLEAN NTAPI ehci_cal_cpu_freq(PVOID context);

extern USB_DEV_MANAGER g_dev_mgr;

/* wrap-aware logic morphed from <linux/jiffies.h> */
#define tick_before(t1,t2) ((SHORT)(((SHORT)(t1))-((SHORT)(t2))) < 0)

#define OHCI_READ_PORT_ULONG( pul ) ( *pul )
#define OHCI_WRITE_PORT_ULONG( pul, src ) \
{\
   	*pul = ( ULONG )src;\
}

#define OHCI_READ_PORT_UCHAR( pch ) ( *pch )
#define OHCI_WRITE_PORT_UCHAR( pch, src ) ( *pch = ( UCHAR )src )
#define OHCI_READ_PORT_USHORT( psh ) ( *psh )
#define OHCI_WRITE_PORT_USHORT( psh, src ) ( *psh = ( USHORT )src )

/* AMD-756 (D2 rev) reports corrupt register contents in some cases.
 * The erratum (#4) description is incorrect.  AMD's workaround waits
 * till some bits (mostly reserved) are clear; ok for all revs.
 */
#define read_roothub(hc, register, mask) ({ \
	ULONG temp = OHCI_READ_PORT_ULONG(&((hc)->regs->roothub.register)); \
	if (temp == -1) \
		/*disable (hc)*/; \
	/*else if (hc->flags & OHCI_QUIRK_AMD756) \
		while (temp & mask) \
			temp = ohci_readl (hc, &hc->regs->roothub.register); */ \
	temp; })

static ULONG roothub_a (POHCI_DEV hc)
	{ return read_roothub (hc, a, 0xfc0fe000); }
/*
static inline u32 roothub_b (struct ohci_hcd *hc)
	{ return ohci_readl (hc, &hc->regs->roothub.b); }
static inline u32 roothub_status (struct ohci_hcd *hc)
	{ return ohci_readl (hc, &hc->regs->roothub.status); }
static u32 roothub_portstatus (struct ohci_hcd *hc, int i)
	{ return read_roothub (hc, portstatus [i], 0xffe0fce0); }
*/


/* For initializing controller (mask in an HCFS mode too) */
#define	OHCI_CONTROL_INIT 	OHCI_CTRL_CBSR
#define	OHCI_INTR_INIT \
	(OHCI_INTR_MIE | OHCI_INTR_UE | OHCI_INTR_RD | OHCI_INTR_WDH)

/* See usb 7.1.7.5:  root hubs must issue at least 50 msec reset signaling,
 * not necessarily continuous ... to guard against resume signaling.
 * The short timeout is safe for non-root hubs, and is backward-compatible
 * with earlier Linux hosts.
 */
#ifdef	CONFIG_USB_SUSPEND
#define	PORT_RESET_MSEC		50
#else
#define	PORT_RESET_MSEC		10
#endif

/* this timer value might be vendor-specific ... */
#define	PORT_RESET_HW_MSEC	10


VOID
ohci_wait_ms(POHCI_DEV ohci, LONG ms)
{
    LARGE_INTEGER lms;
    if (ms <= 0)
        return;

    lms.QuadPart = -10 * ms;
    KeSetTimer(&ohci->reset_timer, lms, NULL);

    KeWaitForSingleObject(&ohci->reset_timer, Executive, KernelMode, FALSE, NULL);

    return;
}

PDEVICE_OBJECT ohci_probe(PDRIVER_OBJECT drvr_obj, PUNICODE_STRING reg_path,
                          PUSB_DEV_MANAGER dev_mgr)
{
    LONG bus, i, j, ret = 0;
    PCI_SLOT_NUMBER slot_num;
    PPCI_COMMON_CONFIG pci_config;
    PDEVICE_OBJECT pdev;
    BYTE buffer[sizeof(PCI_COMMON_CONFIG)];
    POHCI_DEVICE_EXTENSION pdev_ext;

    slot_num.u.AsULONG = 0;
    pci_config = (PPCI_COMMON_CONFIG) buffer;
    pdev = NULL;

    //scan the bus to find ohci controller
    for(bus = 0; bus < 2; bus++)        /*enum only bus0 and bus1 */
    {
        for(i = 0; i < PCI_MAX_DEVICES; i++)
        {
            slot_num.u.bits.DeviceNumber = i;
            for(j = 0; j < PCI_MAX_FUNCTIONS; j++)
            {
                slot_num.u.bits.FunctionNumber = j;

                ret = HalGetBusData(PCIConfiguration,
                                    bus, slot_num.u.AsULONG, pci_config, PCI_COMMON_HDR_LENGTH);

                /* Don't look further on this device */
                if ((ret == 0) || (ret == 2))
                    break;

                if (pci_config->BaseClass == 0x0c && pci_config->SubClass == 0x03
                    && pci_config->ProgIf == 0x10)
                {
                    // we found our usb host controller( OHCI ), create device
                    pdev = ohci_alloc(drvr_obj, reg_path, ((bus << 8) | (i << 3) | j), dev_mgr);

                    if (!pdev)
                        continue;
                }
            }

            if (ret == 0)
                break;
        }
    }

    if (pdev)
    {
        pdev_ext = pdev->DeviceExtension;
        if (pdev_ext)
        {
            // acquire higher irql to eliminate pre-empty
            //KeSynchronizeExecution(pdev_ext->ohci_int, ehci_cal_cpu_freq, NULL);
        }
    }
    return NULL;
}

BOOLEAN ohci_mem_init (POHCI_DEVICE_EXTENSION dev_ext)
{
	dev_ext->ohci->td_cache = HalAllocateCommonBuffer(dev_ext->padapter,
        sizeof(OHCI_TD), &dev_ext->ohci->td_logic_addr, FALSE);

	if (!dev_ext->ohci->td_cache)
		return FALSE;

	dev_ext->ohci->ed_cache = HalAllocateCommonBuffer(dev_ext->padapter,
        sizeof(OHCI_ED), &dev_ext->ohci->ed_logic_addr, FALSE);

	if (!dev_ext->ohci->ed_cache)
    {
        HalFreeCommonBuffer(dev_ext->padapter, sizeof(OHCI_TD), dev_ext->ohci->td_logic_addr,
            dev_ext->ohci->td_cache, FALSE);
		return FALSE;
    }

	return TRUE;
}


PDEVICE_OBJECT
ohci_alloc(PDRIVER_OBJECT drvr_obj, PUNICODE_STRING reg_path, ULONG bus_addr, PUSB_DEV_MANAGER dev_mgr)
{
    LONG frd_num, prd_num;
    PDEVICE_OBJECT pdev = NULL;
    POHCI_DEVICE_EXTENSION pdev_ext;
    ULONG addr_space;
    //ULONG vector;
    LONG bus;
    //KIRQL irql;
    //KAFFINITY affinity;

    DEVICE_DESCRIPTION dev_desc;
    CM_PARTIAL_RESOURCE_DESCRIPTOR *pprd;
    PCI_SLOT_NUMBER slot_num;
    NTSTATUS status;

    pdev = ohci_create_device(drvr_obj, dev_mgr);

    if (pdev == NULL)
        return NULL;

    pdev_ext = pdev->DeviceExtension;

    pdev_ext->pci_addr = bus_addr;
    bus = (bus_addr >> 8);

    slot_num.u.AsULONG = 0;
    slot_num.u.bits.DeviceNumber = ((bus_addr & 0xff) >> 3);
    slot_num.u.bits.FunctionNumber = (bus_addr & 0x07);

    //now create adapter object
    RtlZeroMemory(&dev_desc, sizeof(dev_desc));

    dev_desc.Version = DEVICE_DESCRIPTION_VERSION;
    dev_desc.Master = TRUE;
    dev_desc.ScatterGather = TRUE;
    dev_desc.Dma32BitAddresses = TRUE;
    dev_desc.BusNumber = bus;
    dev_desc.InterfaceType = PCIBus;
    dev_desc.MaximumLength = EHCI_MAX_SIZE_TRANSFER;
    pdev_ext->map_regs = 2;     // we do not use it seriously
    pdev_ext->padapter = HalGetAdapter(&dev_desc, &pdev_ext->map_regs);

    DbgPrint("ohci_alloc(): reg_path=0x%x, \n \
              ohci_alloc(): bus=0x%x, bus_addr=0x%x \n \
              ohci_alloc(): slot_num=0x%x \n \
            ", (DWORD) reg_path, (DWORD) bus, (DWORD) bus_addr, (DWORD) slot_num.u.AsULONG);

    //let's allocate resources for this device
    DbgPrint("ohci_alloc(): about to assign slot res\n");
    if ((status = HalAssignSlotResources(reg_path, NULL,        //no class name yet
                                         drvr_obj, NULL,        //no support of another ehci controller
                                         PCIBus,
                                         bus, slot_num.u.AsULONG, &pdev_ext->res_list)) != STATUS_SUCCESS)
    {
        DbgPrint("ohci_alloc(): error assign slot res, 0x%x\n", status);
#if 0
        release_adapter(pdev_ext->padapter);
        pdev_ext->padapter = NULL;
        ohci_delete_device(pdev);
#endif
        return NULL;
    }

    //parse the resource list
    for(frd_num = 0; frd_num < (LONG) pdev_ext->res_list->Count; frd_num++)
    {
        for(prd_num = 0; prd_num < (LONG) pdev_ext->res_list->List[frd_num].PartialResourceList.Count;
            prd_num++)
        {
            pprd = &pdev_ext->res_list->List[frd_num].PartialResourceList.PartialDescriptors[prd_num];
            if (pprd->Type == CmResourceTypePort)
            {
                RtlCopyMemory(&pdev_ext->res_port, &pprd->u.Port, sizeof(pprd->u.Port));

            }
            else if (pprd->Type == CmResourceTypeInterrupt)
            {
                RtlCopyMemory(&pdev_ext->res_interrupt, &pprd->u.Interrupt, sizeof(pprd->u.Interrupt));
            }
            else if (pprd->Type == CmResourceTypeMemory)
            {
                RtlCopyMemory(&pdev_ext->res_memory, &pprd->u.Memory, sizeof(pprd->u.Memory));
            }
        }
    }

    //for port, translate them to system address
    addr_space = 0;
    if (HalTranslateBusAddress(PCIBus, bus, pdev_ext->res_port.Start, &addr_space,      //io space
                               &pdev_ext->ohci->ohci_reg_base) != (BOOLEAN) TRUE)
    {
        DbgPrint("ohci_alloc(): error, can not translate bus address\n");
#if 0
        release_adapter(pdev_ext->padapter);
        pdev_ext->padapter = NULL;
        ehci_delete_device(pdev);
#endif
        return NULL;
    }

    DbgPrint("ohci_alloc(): address space=0x%x\n, reg_base=0x%x\n",
             addr_space, pdev_ext->ohci->ohci_reg_base.u.LowPart);

    if (addr_space == 0)
    {
        //port has been mapped to memory space
        pdev_ext->ohci->port_mapped = TRUE;
        pdev_ext->ohci->port_base = (PBYTE) MmMapIoSpace(pdev_ext->ohci->ohci_reg_base,
                                                         pdev_ext->res_port.Length, FALSE);

        //fatal error can not map the registers
        if (pdev_ext->ohci->port_base == NULL)
        {
#if 0
            release_adapter(pdev_ext->padapter);
            pdev_ext->padapter = NULL;
            ehci_delete_device(pdev);
#endif
            return NULL;
        }
    }
    else
    {
        //io space
        pdev_ext->ohci->port_mapped = FALSE;
        pdev_ext->ohci->port_base = (PBYTE) pdev_ext->ohci->ohci_reg_base.LowPart;
    }

    //before we connect the interrupt, we have to init ohci
    pdev_ext->ohci->pdev_ext = pdev_ext;
    pdev_ext->ohci->regs = (POHCI_REGS)pdev_ext->ohci->port_base;

    KeInitializeTimer(&pdev_ext->ohci->reset_timer);

    // take it over from SMM/BIOS/whoever has it
	if (OHCI_READ_PORT_ULONG((PULONG)(pdev_ext->ohci->port_base + OHCI_CONTROL)) & OHCI_CTRL_IR)
    {
		ULONG temp;

		DbgPrint("USB HC TakeOver from BIOS/SMM\n");

		/* this timeout is arbitrary.  we make it long, so systems
		 * depending on usb keyboards may be usable even if the
		 * BIOS/SMM code seems pretty broken.
		 */
		temp = 500;	/* arbitrary: five seconds */

        OHCI_WRITE_PORT_ULONG((PULONG)(pdev_ext->ohci->port_base + OHCI_INTRENABLE), OHCI_INTR_OC);
        OHCI_WRITE_PORT_ULONG((PULONG)(pdev_ext->ohci->port_base + OHCI_CMDSTATUS), OHCI_OCR);

        while (OHCI_READ_PORT_ULONG((PULONG)(pdev_ext->ohci->port_base + OHCI_CONTROL)) & OHCI_CTRL_IR)
        {
			ohci_wait_ms(pdev_ext->ohci, 10);
			if (--temp == 0) {
				DbgPrint("USB HC takeover failed!"
					"  (BIOS/SMM bug)\n");
				return NULL;
			}
		}
		//ohci_usb_reset (ohci);
	}

	/* Disable HC interrupts */
    OHCI_WRITE_PORT_ULONG((PULONG)(pdev_ext->ohci->port_base + OHCI_INTRDISABLE), OHCI_INTR_MIE);
	// flush the writes
    (VOID)OHCI_READ_PORT_ULONG((PULONG)(pdev_ext->ohci->port_base + OHCI_CONTROL));

	/* Read the number of ports unless overridden */
	pdev_ext->ohci->num_ports = roothub_a(pdev_ext->ohci) & RH_A_NDP;

    DbgPrint("OHCI: %d ports\n", pdev_ext->ohci->num_ports);

	pdev_ext->ohci->hcca = HalAllocateCommonBuffer(pdev_ext->padapter,
        sizeof(*pdev_ext->ohci->hcca), &pdev_ext->ohci->hcca_logic_addr, FALSE);

    if (!pdev_ext->ohci->hcca)
    {
        DbgPrint("OHCI: HCCA allocation failed!\n");
        return NULL;
    }

    if (!ohci_mem_init(pdev_ext))
    {
        DbgPrint("OHCI: Mem init failed!\n");
        return NULL;
    }

#if 0
    //init ehci_caps
    // i = ( ( PEHCI_HCS_CONTENT )( &pdev_ext->ehci->ehci_caps.hcs_params ) )->length;

    ehci_get_capabilities(pdev_ext->ehci, pdev_ext->ehci->port_base);
    i = pdev_ext->ehci->ehci_caps.length;
    pdev_ext->ehci->port_base += i;

    if (ehci_init_schedule(pdev_ext->ehci, pdev_ext->padapter) == FALSE)
    {
        release_adapter(pdev_ext->padapter);
        pdev_ext->padapter = NULL;
        ehci_delete_device(pdev);
        return NULL;
    }

    InitializeListHead(&pdev_ext->ehci->urb_list);
    KeInitializeSpinLock(&pdev_ext->ehci->pending_endp_list_lock);
    InitializeListHead(&pdev_ext->ehci->pending_endp_list);

    ehci_dbg_print(DBGLVL_MAXIMUM, ("ehci_alloc(): pending_endp_list=0x%x\n",
                                    &pdev_ext->ehci->pending_endp_list));

    init_pending_endp_pool(&pdev_ext->ehci->pending_endp_pool);

    vector = HalGetInterruptVector(PCIBus,
                                   bus,
                                   pdev_ext->res_interrupt.level,
                                   pdev_ext->res_interrupt.vector, &irql, &affinity);

    //connect the interrupt
    DbgPrint("ehci_alloc(): the int=0x%x\n", vector);
    if ((status = IoConnectInterrupt(&pdev_ext->ehci_int, ehci_isr, pdev_ext->ehci, NULL, //&pdev_ext->ehci->frame_list_lock,
                           vector, irql, irql, LevelSensitive, TRUE,    //share the vector
                           affinity, FALSE))     //No float save
        != STATUS_SUCCESS)
    {
        DbgPrint("ehci_alloc(): Failed to connect interrupt, status = 0x%x!\n", status);
        ehci_release(pdev);
        return NULL;
    }

    KeInitializeDpc(&pdev_ext->ehci_dpc, ehci_dpc_callback, (PVOID) pdev_ext->ehci);
#endif

    return pdev;
}

PDEVICE_OBJECT
ohci_create_device(PDRIVER_OBJECT drvr_obj, PUSB_DEV_MANAGER dev_mgr)
{
    NTSTATUS status;
    PDEVICE_OBJECT pdev;
    POHCI_DEVICE_EXTENSION pdev_ext;

    UNICODE_STRING dev_name;
    UNICODE_STRING symb_name;

    STRING string, another_string;
    CHAR str_dev_name[64], str_symb_name[64];
    UCHAR hcd_id;

    if (drvr_obj == NULL)
        return NULL;

    //note: hcd count wont increment till the hcd is registered in dev_mgr
    sprintf(str_dev_name, "%s%d", OHCI_DEVICE_NAME, dev_mgr->hcd_count);
    sprintf(str_symb_name, "%s%d", OHCI_DOS_DEVICE_NAME, dev_mgr->hcd_count);

    RtlInitString(&string, str_dev_name);
    RtlAnsiStringToUnicodeString(&dev_name, &string, TRUE);

    pdev = NULL;
    status = IoCreateDevice(drvr_obj,
                            sizeof(OHCI_DEVICE_EXTENSION) + sizeof(OHCI_DEV),
                            &dev_name, FILE_OHCI_DEV_TYPE, 0, FALSE, &pdev);

    if (status != STATUS_SUCCESS || pdev == NULL)
    {
        RtlFreeUnicodeString(&dev_name);
        ehci_dbg_print(DBGLVL_MAXIMUM, ("ohci_create_device(): error create device 0x%x\n", status));
        return NULL;
    }

    pdev_ext = pdev->DeviceExtension;
    RtlZeroMemory(pdev_ext, sizeof(OHCI_DEVICE_EXTENSION) + sizeof(OHCI_DEV));

    pdev_ext->dev_ext_hdr.type = NTDEV_TYPE_HCD;
    pdev_ext->dev_ext_hdr.dispatch = ehci_dispatch_irp;
    pdev_ext->dev_ext_hdr.start_io = NULL;      //we do not support startio
    pdev_ext->dev_ext_hdr.dev_mgr = dev_mgr;

    pdev_ext->pdev_obj = pdev;
    pdev_ext->pdrvr_obj = drvr_obj;

    pdev_ext->ohci = (POHCI_DEV) & (pdev_ext[1]);

    RtlInitString(&another_string, str_symb_name);
    RtlAnsiStringToUnicodeString(&symb_name, &another_string, TRUE);
    //RtlInitUnicodeString( &symb_name, DOS_DEVICE_NAME );

    IoCreateSymbolicLink(&symb_name, &dev_name);

    ehci_dbg_print(DBGLVL_MAXIMUM,
                   ("ohci_create_device(): dev=0x%x\n, pdev_ext= 0x%x, ehci=0x%x, dev_mgr=0x%x\n", pdev,
                    pdev_ext, pdev_ext->ohci, dev_mgr));

    RtlFreeUnicodeString(&dev_name);
    RtlFreeUnicodeString(&symb_name);

    //register with dev_mgr though it is not initilized
    ohci_init_hcd_interface(pdev_ext->ohci);
    hcd_id = dev_mgr_register_hcd(dev_mgr, &pdev_ext->ohci->hcd_interf);

    pdev_ext->ohci->hcd_interf.hcd_set_id(&pdev_ext->ohci->hcd_interf, hcd_id);
    pdev_ext->ohci->hcd_interf.hcd_set_dev_mgr(&pdev_ext->ohci->hcd_interf, dev_mgr);

    return pdev;
}

BOOLEAN
ohci_start(PHCD hcd)
{
    ULONG temp, mask;
    //PBYTE base;
    //PEHCI_USBCMD_CONTENT usbcmd;
    POHCI_DEV ohci;
    ULONG hc_control;

    if (hcd == NULL)
        return FALSE;

    ohci = struct_ptr(hcd, OHCI_DEV, hcd_interf);

  	/* Reset USB nearly "by the book".  RemoteWakeupConnected
	 * saved if boot firmware (BIOS/SMM/...) told us it's connected
	 * (for OHCI integrated on mainboard, it normally is)
	 */
	hc_control = OHCI_READ_PORT_ULONG((PULONG)&ohci->regs->control);
    DbgPrint("OHCI: resetting from state %x, control = 0x%x\n",
			(hc_control & OHCI_CTRL_HCFS),
			hc_control);

	//if (hc_control & OHCI_CTRL_RWC
	//		&& !(ohci->flags & OHCI_QUIRK_AMD756))
	//	ohci_to_hcd(ohci)->can_wakeup = 1;

	switch (hc_control & OHCI_CTRL_HCFS) {
	case OHCI_USB_OPER:
		temp = 0;
		break;
	case OHCI_USB_SUSPEND:
	case OHCI_USB_RESUME:
		hc_control &= OHCI_CTRL_RWC;
		hc_control |= OHCI_USB_RESUME;
		temp = 10 /* msec wait */;
		break;
	// case OHCI_USB_RESET:
	default:
		hc_control &= OHCI_CTRL_RWC;
		hc_control |= OHCI_USB_RESET;
		temp = 50 /* msec wait */;
		break;
	}
    OHCI_WRITE_PORT_ULONG((PULONG)&ohci->regs->control, hc_control);

	// flush the writes
    (VOID)OHCI_READ_PORT_ULONG((PULONG)&ohci->regs->control);

	ohci_wait_ms(ohci, temp);
	temp = roothub_a (ohci);
	if (!(temp & RH_A_NPS)) {
		/* power down each port */
		for (temp = 0; temp < ohci->num_ports; temp++)
        {
            OHCI_WRITE_PORT_ULONG((PULONG)&ohci->regs->roothub.portstatus [temp], RH_PS_LSDA);
        }
	}
	// flush those writes
	(VOID)OHCI_READ_PORT_ULONG((PULONG)&ohci->regs->control);
    RtlZeroMemory(ohci->hcca, sizeof(OHCI_HCCA));

	/* 2msec timelimit here means no irqs/preempt */
	//spin_lock_irq (&ohci->lock);

//retry:
	/* HC Reset requires max 10 us delay */
    OHCI_WRITE_PORT_ULONG((PULONG)&ohci->regs->cmdstatus, OHCI_HCR);
	temp = 30;	/* ... allow extra time */
	while ((OHCI_READ_PORT_ULONG ((PULONG)&ohci->regs->cmdstatus) & OHCI_HCR) != 0) {
		if (--temp == 0) {
			//spin_unlock_irq (&ohci->lock);
			//ohci_err (ohci, "USB HC reset timed out!\n");
            DbgPrint("OHCI: USB HC reset timed out!\n");
			return FALSE;
		}
        KeStallExecutionProcessor(1);
	}

	/* now we're in the SUSPEND state ... must go OPERATIONAL
	 * within 2msec else HC enters RESUME
	 *
	 * ... but some hardware won't init fmInterval "by the book"
	 * (SiS, OPTi ...), so reset again instead.  SiS doesn't need
	 * this if we write fmInterval after we're OPERATIONAL.
	 * Unclear about ALi, ServerWorks, and others ... this could
	 * easily be a longstanding bug in chip init on Linux.
	 */
#if 0
	if (ohci->flags & OHCI_QUIRK_INITRESET) {
		ohci_writel (ohci, ohci->hc_control, &ohci->regs->control);
		// flush those writes
		(void) ohci_readl (ohci, &ohci->regs->control);
	}
#endif
	/* Tell the controller where the control and bulk lists are
	 * The lists are empty now. */
    OHCI_WRITE_PORT_ULONG((PULONG)&ohci->regs->ed_controlhead, 0);
    OHCI_WRITE_PORT_ULONG((PULONG)&ohci->regs->ed_bulkhead, 0);

	/* a reset clears this */
    OHCI_WRITE_PORT_ULONG((PULONG)&ohci->regs->hcca, (ULONG)ohci->hcca_logic_addr.LowPart);

	//periodic_reinit (ohci);

 	/* start controller operations */
	hc_control &= OHCI_CTRL_RWC;
 	hc_control |= OHCI_CONTROL_INIT | OHCI_USB_OPER;
    OHCI_WRITE_PORT_ULONG((PULONG)&ohci->regs->control, hc_control);
	//ohci_to_hcd(ohci)->state = HC_STATE_RUNNING;

	/* wake on ConnectStatusChange, matching external hubs */
    OHCI_WRITE_PORT_ULONG((PULONG)&ohci->regs->roothub.status, RH_HS_DRWE);

	/* Choose the interrupts we care about now, others later on demand */
	mask = OHCI_INTR_INIT;
    //OHCI_WRITE_PORT_ULONG((PULONG)&ohci->regs->intrstatus, mask);
    //OHCI_WRITE_PORT_ULONG((PULONG)&ohci->regs->intrenable, mask);

	/* handle root hub init quirks ... */
	temp = roothub_a(ohci);
	temp &= ~(RH_A_PSM | RH_A_OCPM);
    OHCI_WRITE_PORT_ULONG((PULONG)&ohci->regs->roothub.status, RH_HS_LPSC);
    OHCI_WRITE_PORT_ULONG((PULONG)&ohci->regs->roothub.b, (temp & RH_A_NPS) ? 0 : RH_B_PPCM);
	// flush those writes
    (VOID)OHCI_READ_PORT_ULONG((PULONG)&ohci->regs->control);

	//spin_unlock_irq (&ohci->lock);

	// POTPGT delay is bits 24-31, in 2 ms units.
    ohci_wait_ms(ohci, (temp >> 23) & 0x1fe);
	//ohci_to_hcd(ohci)->state = HC_STATE_RUNNING;


    // Debug code follows!
    /*(VOID)ohci_rh_reset_port(hcd, 1);
    (VOID)ohci_rh_reset_port(hcd, 2);
    (VOID)ohci_rh_reset_port(hcd, 3);
    (VOID)ohci_rh_reset_port(hcd, 4);*/
    // Debug code ends!


    return TRUE;
}


ULONG
ohci_get_type(PHCD hcd)
{
    return HCD_TYPE_OHCI;       // ( hcd->flags & HCD_TYPE_MASK );
}

NTSTATUS
ohci_submit_urb2(PHCD hcd, PUSB_DEV pdev, PUSB_ENDPOINT pendp, PURB purb)
{
    DbgPrint("ohci_submit_urb2 caled, but not implemented!\n");
    return STATUS_UNSUCCESSFUL;
}

PUSB_DEV
ohci_get_root_hub(PHCD hcd)
{
    return ohci_from_hcd(hcd)->root_hub;
}

VOID
ohci_set_root_hub(PHCD hcd, PUSB_DEV root_hub)
{
    if (hcd == NULL || root_hub == NULL)
        return;
    ohci_from_hcd(hcd)->root_hub = root_hub;
    return;
}

BOOLEAN
ohci_remove_device2(PHCD hcd, PUSB_DEV pdev)
{
    if (hcd == NULL || pdev == NULL)
        return FALSE;

    return FALSE;
    //return ehci_remove_device(ehci_from_hcd(hcd), pdev);
}

BOOLEAN
ohci_hcd_release(PHCD hcd)
{
    POHCI_DEV ohci;
    POHCI_DEVICE_EXTENSION pdev_ext;

    if (hcd == NULL)
        return FALSE;

    ohci = ohci_from_hcd(hcd);
    pdev_ext = ohci->pdev_ext;
    return FALSE;//ehci_release(pdev_ext->pdev_obj);
}

NTSTATUS
ohci_cancel_urb2(PHCD hcd, PUSB_DEV pdev, PUSB_ENDPOINT pendp, PURB purb)
{
    POHCI_DEV ohci;
    if (hcd == NULL)
        return STATUS_INVALID_PARAMETER;

    ohci = ohci_from_hcd(hcd);
    DbgPrint("ohci_cancel_urb2 called, but not implemented!\n");
    return STATUS_UNSUCCESSFUL;//ehci_cancel_urb(ehci, pdev, pendp, purb);
}

VOID
ohci_generic_urb_completion(PURB purb, PVOID context)
{
    DbgPrint("ohci_generic_urb_completion called, but not implemented!\n");
}

BOOLEAN
ohci_rh_reset_port(PHCD hcd, UCHAR port_idx)
{
    POHCI_DEV ohci;
    ULONG status, temp;
    PULONG PortStatus;
    USHORT Now, ResetDone;

    if (hcd == NULL)
        return FALSE;

    ohci = ohci_from_hcd(hcd);

    if (port_idx < 1 || port_idx > ohci->num_ports)
        return FALSE;

    port_idx--;

    PortStatus = &ohci->regs->roothub.portstatus[port_idx];

    Now = OHCI_READ_PORT_ULONG((PULONG)&ohci->regs->fmnumber);
	ResetDone = Now + PORT_RESET_MSEC;

	/* build a "continuous enough" reset signal, with up to
	 * 3msec gap between pulses.  scheduler HZ==100 must work;
	 * this might need to be deadline-scheduled.
	 */
	do {
		/* spin until any current reset finishes */
		for (;;) {
            temp = OHCI_READ_PORT_ULONG(PortStatus);
			if (!(temp & RH_PS_PRS))
				break;
            usb_wait_us_dpc(500);
		} 

		if (!(temp & RH_PS_CCS))
			break;
		if (temp & RH_PS_PRSC)
        {
            OHCI_WRITE_PORT_ULONG(PortStatus, RH_PS_PRSC);
        }

		/* start the next reset, sleep till it's probably done */
        OHCI_WRITE_PORT_ULONG(PortStatus, RH_PS_PRS);
        usb_wait_ms_dpc(PORT_RESET_HW_MSEC);
        Now = OHCI_READ_PORT_ULONG((PULONG)&ohci->regs->fmnumber);
	} while (tick_before(Now, ResetDone));

    status = OHCI_READ_PORT_ULONG((PULONG)(&ohci->regs->roothub.portstatus[port_idx]));
    usb_dbg_print(DBGLVL_MAXIMUM, ("ohci_rh_reset_port(): status after written=0x%x\n", status));
    return TRUE;
}

BOOLEAN
ohci_rh_get_dev_change(PHCD hcd, PBYTE buf)     //must have the rh dev_lock acquired
{
    POHCI_DEV ohci;
    LONG i;
    ULONG status;

    if (hcd == NULL)
        return FALSE;

    ohci = ohci_from_hcd(hcd);

    for(i = 0; i < ohci->num_ports; i++)
    {
        status = OHCI_READ_PORT_ULONG((PULONG)(&ohci->regs->roothub.portstatus[i]));

        if (status != 0)
        {
            ohci_dbg_print(DBGLVL_MAXIMUM, ("ohci_rh_get_dev_change(): erh port%d status=0x%x\n", i, status));
        }

        if (status & (RH_PS_PESC | RH_PS_CSC | RH_PS_OCIC))
        {
            buf[(i + 1) >> 3] |= (1 << ((i + 1) & 7));
        }
    }
    return TRUE;
}


NTSTATUS
ohci_hcd_dispatch(PHCD hcd, LONG disp_code, PVOID param)
{
    POHCI_DEV ohci;

    if (hcd == NULL)
        return STATUS_INVALID_PARAMETER;
    ohci = ohci_from_hcd(hcd);

    switch (disp_code)
    {
        case HCD_DISP_READ_PORT_COUNT:
        {
            if (param == NULL)
                return STATUS_INVALID_PARAMETER;
            *((PUCHAR) param) = ohci->num_ports;
            return STATUS_SUCCESS;
        }
        case HCD_DISP_READ_RH_DEV_CHANGE:
        {
            if (ohci_rh_get_dev_change(hcd, param) == FALSE)
                return STATUS_INVALID_PARAMETER;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_IMPLEMENTED;
}

VOID
ohci_init_hcd_interface(POHCI_DEV ohci)
{
    ohci->hcd_interf.hcd_set_dev_mgr = ehci_set_dev_mgr;
    ohci->hcd_interf.hcd_get_dev_mgr = ehci_get_dev_mgr;
    ohci->hcd_interf.hcd_get_type = ohci_get_type;
    ohci->hcd_interf.hcd_set_id = ehci_set_id;
    ohci->hcd_interf.hcd_get_id = ehci_get_id;
    ohci->hcd_interf.hcd_alloc_addr = ehci_alloc_addr;
    ohci->hcd_interf.hcd_free_addr = ehci_free_addr;
    ohci->hcd_interf.hcd_submit_urb = ohci_submit_urb2;
    ohci->hcd_interf.hcd_generic_urb_completion = ohci_generic_urb_completion;
    ohci->hcd_interf.hcd_get_root_hub = ohci_get_root_hub;
    ohci->hcd_interf.hcd_set_root_hub = ohci_set_root_hub;
    ohci->hcd_interf.hcd_remove_device = ohci_remove_device2;
    ohci->hcd_interf.hcd_rh_reset_port = ohci_rh_reset_port;
    ohci->hcd_interf.hcd_release = ohci_hcd_release;
    ohci->hcd_interf.hcd_cancel_urb = ohci_cancel_urb2;
    ohci->hcd_interf.hcd_start = ohci_start;
    ohci->hcd_interf.hcd_dispatch = ohci_hcd_dispatch;

    ohci->hcd_interf.flags = HCD_TYPE_OHCI;     //hcd types | hcd id
}

