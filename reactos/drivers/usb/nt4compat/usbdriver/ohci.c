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

// shared with EHCI
NTSTATUS ehci_dispatch_irp(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp);
VOID ehci_set_dev_mgr(PHCD hcd, PUSB_DEV_MANAGER dev_mgr);
VOID ehci_set_id(PHCD hcd, UCHAR id);
BOOLEAN NTAPI ehci_cal_cpu_freq(PVOID context);

extern USB_DEV_MANAGER g_dev_mgr;

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
            KeSynchronizeExecution(pdev_ext->ohci_int, ehci_cal_cpu_freq, NULL);
        }
    }
    return NULL;
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
    //dev_desc.MaximumLength = EHCI_MAX_SIZE_TRANSFER;

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

    KeInitializeTimer(&pdev_ext->ehci->reset_timer);

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

VOID
ohci_init_hcd_interface(POHCI_DEV ohci)
{
    ohci->hcd_interf.hcd_set_dev_mgr = ehci_set_dev_mgr;
#if 0
    ohci->hcd_interf.hcd_get_dev_mgr = ehci_get_dev_mgr;
    ohci->hcd_interf.hcd_get_type = ehci_get_type;
#endif
    ohci->hcd_interf.hcd_set_id = ehci_set_id;
#if 0
    ohci->hcd_interf.hcd_get_id = ehci_get_id;
    ohci->hcd_interf.hcd_alloc_addr = ehci_alloc_addr;
    ohci->hcd_interf.hcd_free_addr = ehci_free_addr;
    ohci->hcd_interf.hcd_submit_urb = ehci_submit_urb2;
    ohci->hcd_interf.hcd_generic_urb_completion = ehci_generic_urb_completion;
    ohci->hcd_interf.hcd_get_root_hub = ehci_get_root_hub;
    ohci->hcd_interf.hcd_set_root_hub = ehci_set_root_hub;
    ohci->hcd_interf.hcd_remove_device = ehci_remove_device2;
    ohci->hcd_interf.hcd_rh_reset_port = ehci_rh_reset_port;
    ohci->hcd_interf.hcd_release = ehci_hcd_release;
    ohci->hcd_interf.hcd_cancel_urb = ehci_cancel_urb2;
    ohci->hcd_interf.hcd_start = ehci_start;
    ohci->hcd_interf.hcd_dispatch = ehci_hcd_dispatch;
#endif
    ohci->hcd_interf.flags = HCD_TYPE_OHCI;     //hcd types | hcd id
}

