/**
 * usb.c - USB driver stack project for Windows NT 4.0
 *
 * Copyright (c) 2002-2004 Zhiming  mypublic99@yahoo.com
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

LONG g_alloc_cnt = 0;
ULONG cpu_clock_freq = 0;

NTSTATUS usb_get_descriptor(PUSB_DEV pdev, PURB purb);
VOID usb_set_interface_completion(PURB purb, PVOID context);
NTSTATUS usb_set_interface(PURB purb);

PVOID
usb_alloc_mem(POOL_TYPE pool_type, LONG size)
{
    PVOID ret;
    g_alloc_cnt++;
    ret = ExAllocatePool(pool_type, size);
    usb_dbg_print(DBGLVL_ULTRA, ("usb_alloc_mem(): alloced=0x%x\n", g_alloc_cnt));
    return ret;
}

VOID
usb_free_mem(PVOID pbuf)
{
    g_alloc_cnt--;
    usb_dbg_print(DBGLVL_ULTRA, ("usb_free_mem(): alloced=0x%x\n", g_alloc_cnt));
    ExFreePool(pbuf);
}

VOID usb_config_dev_completion(PURB purb, PVOID context);

//shamelessly pasted from linux's usb.c
LONG
usb_calc_bus_time(LONG speed, LONG input_dir, LONG is_iso, LONG byte_count)
{
    LONG tmp;

    switch (speed & 0x3)        /* no isoc. here */
    {
        case USB_SPEED_LOW:
        {
            if (input_dir)
            {
                tmp = (67667L * (31L + 10L * bit_time(byte_count))) / 1000L;
                return (64060L + (2 * BW_HUB_LS_SETUP) + BW_HOST_DELAY + tmp);
            }
            else
            {
                tmp = (66700L * (31L + 10L * bit_time(byte_count))) / 1000L;
                return (64107L + (2 * BW_HUB_LS_SETUP) + BW_HOST_DELAY + tmp);
            }
            break;
        }
        /* for full-speed: */
        case USB_SPEED_FULL:
        {
            if (!is_iso)        /* Input or Output */
            {
                tmp = (8354L * (31L + 10L * bit_time(byte_count))) / 1000L;
                return (9107L + BW_HOST_DELAY + tmp);
            }                   /* end not Isoc */

            /* for isoc: */

            tmp = (8354L * (31L + 10L * bit_time(byte_count))) / 1000L;
            return (((input_dir) ? 7268L : 6265L) + BW_HOST_DELAY + tmp);
        }
        case USB_SPEED_HIGH:
        {
            if (!is_iso)
            {
                tmp = (999 + 926520 + 2083 * ((LONG) ((19 + 7 * 8 * byte_count) / 6))) / 1000;
            }
            else
            {
                tmp = (999 + 633232 + 2083 * ((LONG) ((19 + 7 * 8 * byte_count) / 6))) / 1000;
            }
            return tmp + USB2_HOST_DELAY;
        }
        default:
        {
            break;
        }
    }
    return 125001;
}

//
// if the dev is not in the list, return value is not success and the pointer is nulled
// if the dev is in the list but zomb, return value is error code and the pointer is the dev( no ref_count guarded )
// if the dev is alive and in the list, return is success and the pointer is the dev.
// one must be aware of what his doing before he uses the ppdev
//
NTSTATUS
usb_query_and_lock_dev(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle, PUSB_DEV * ppdev)
{
    int i;
    PLIST_ENTRY pthis, pnext;
    PUSB_DEV pdev = NULL;
    BOOLEAN valid_dev;

    USE_NON_PENDING_IRQL;

    *ppdev = NULL;

    if (dev_mgr == NULL || dev_handle == 0)
        return STATUS_INVALID_PARAMETER;

    i = dev_id_from_handle(dev_handle);

    KeAcquireSpinLock(&dev_mgr->dev_list_lock, &old_irql);
    ListFirst(&dev_mgr->dev_list, pthis);

    while (pthis)
    {
        pdev = (PUSB_DEV) pthis;
        if (pdev->dev_id != (ULONG) i)
        {
            ListNext(&dev_mgr->dev_list, pthis, pnext);
            pthis = pnext;
            continue;
        }
        else
            break;
    }
    if (pthis == NULL)
    {
        //no such device
        KeReleaseSpinLock(&dev_mgr->dev_list_lock, old_irql);
        return STATUS_INVALID_PARAMETER;
    }

    valid_dev = TRUE;

    lock_dev(pdev, TRUE);

    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        valid_dev = FALSE;
    }
    else
        pdev->ref_count++;      //guard the dev by increasing the ref count

    unlock_dev(pdev, TRUE);

    KeReleaseSpinLock(&dev_mgr->dev_list_lock, old_irql);

    *ppdev = pdev;

    if (!valid_dev)
        return STATUS_DEVICE_DOES_NOT_EXIST;

    return STATUS_SUCCESS;

}

NTSTATUS
usb_unlock_dev(PUSB_DEV dev)
{
    USE_BASIC_NON_PENDING_IRQL;

    if (dev == NULL)
        return STATUS_INVALID_PARAMETER;

    lock_dev(dev, FALSE);
    dev->ref_count--;
    if (dev->ref_count < 0)
        dev->ref_count = 0;
    unlock_dev(dev, FALSE);
    return STATUS_SUCCESS;
}

NTSTATUS
usb_reset_pipe_ex(PUSB_DEV_MANAGER dev_mgr,
                  DEV_HANDLE endp_handle,          //endp handle to reset
                  PURBCOMPLETION reset_completion, //note: this reset completion has no right to delete the urb, that is only for reference
                  PVOID param)
{
    NTSTATUS status;
    PUSB_DEV pdev;
    LONG if_idx, endp_idx;
    PUSB_ENDPOINT pendp;
    USE_BASIC_NON_PENDING_IRQL;

    if (dev_mgr == NULL)
        return STATUS_INVALID_PARAMETER;

    status = usb_query_and_lock_dev(dev_mgr, (endp_handle & 0xffff0000), &pdev);
    if (status != STATUS_SUCCESS)
        return STATUS_UNSUCCESSFUL;

    lock_dev(pdev, FALSE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        status = STATUS_UNSUCCESSFUL;
        goto LBL_OUT;
    }

    if_idx = if_idx_from_handle(endp_handle);
    endp_idx = endp_idx_from_handle(endp_handle);

    if (default_endp_handle(endp_handle))
    {
        status = STATUS_UNSUCCESSFUL;
        goto LBL_OUT;
    }

    if (dev_state(pdev) < USB_DEV_STATE_CONFIGURED)
    {
        status = STATUS_DEVICE_NOT_READY;
        goto LBL_OUT;
    }

    pendp = &pdev->usb_config->interf[if_idx].endp[endp_idx];
    unlock_dev(pdev, FALSE) status = usb_reset_pipe(pdev, pendp, reset_completion, param);
    usb_unlock_dev(pdev);
    return status;

LBL_OUT:
    unlock_dev(pdev, FALSE);
    usb_unlock_dev(pdev);

    return status;
}

// caller must guarantee the pdev exist before the routine exit
NTSTATUS
usb_reset_pipe(PUSB_DEV pdev, PUSB_ENDPOINT pendp, PURBCOMPLETION client_reset_pipe_completion, PVOID param)
{

    PHCD hcd;
    PURB purb;
    BYTE endp_addr;
    NTSTATUS status;
    DEV_HANDLE dev_handle;

    USE_BASIC_NON_PENDING_IRQL;

    if (pdev == NULL || pendp == NULL)
        return STATUS_INVALID_PARAMETER;

    lock_dev(pdev, FALSE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, FALSE);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    hcd = pdev->hcd;
    endp_addr = pendp->pusb_endp_desc->bEndpointAddress;
    dev_handle = usb_make_handle(pdev->dev_id, 0, 0);
    unlock_dev(pdev, FALSE);

    purb = (PURB) usb_alloc_mem(NonPagedPool, sizeof(URB) + sizeof(PIRP));

    if (purb == NULL)
        return STATUS_NO_MEMORY;

    UsbBuildResetPipeRequest(purb,
                             dev_handle,
                             endp_addr,
                             usb_reset_pipe_completion,
                             pendp,
                             (LONG)client_reset_pipe_completion);

    *((PULONG)&purb[1]) = (ULONG)param;

    if ((status = hcd->hcd_submit_urb(hcd, pdev, &pdev->default_endp, purb)) != STATUS_PENDING)
    {
        usb_free_mem(purb);
        purb = NULL;
    }
    return status;
}

VOID
usb_reset_pipe_completion(PURB purb, PVOID context)
{
    PUSB_DEV pdev;
    PUSB_ENDPOINT pendp;

    USE_BASIC_NON_PENDING_IRQL;

    if (purb == NULL || context == NULL)
        return;

    pdev = purb->pdev;
    pendp = (PUSB_ENDPOINT) context;

    lock_dev(pdev, TRUE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        goto LBL_OUT;
    }

    if (usb_error(purb->status))
    {
        goto LBL_OUT;
    }
    //clear stall
    pendp->flags &= ~USB_ENDP_FLAG_STAT_MASK;

    //reset toggle endp_type
    if ((pendp->pusb_endp_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK ||
        (pendp->pusb_endp_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT)
    {
        pendp->flags &= ~USB_ENDP_FLAG_DATATOGGLE;
    }

LBL_OUT:
    unlock_dev(pdev, TRUE);

    if (purb->reference)
        ((PURBCOMPLETION) purb->reference) (purb, (PVOID) (*((PULONG) & purb[1])));

    usb_free_mem(purb);
    purb = NULL;
    return;

}

void
usb_reset_pipe_from_dispatch_completion(PURB purb, PVOID param)
{
    PURB pclient_urb;
    if (purb == NULL || param == NULL)
        TRAP();
    pclient_urb = (PURB) param;
    pclient_urb->status = purb->status;

    if (pclient_urb->completion)
    {
        pclient_urb->completion(pclient_urb, pclient_urb->context);
    }
    // the urb can not be freed here because it is owned by the reset
    // pipe completion
    return;
}

//used to check descriptor validity
BOOLEAN
is_header_match(PUCHAR pbuf, ULONG type)
{
    BOOLEAN ret;
    PUSB_DESC_HEADER phdr;
    phdr = (PUSB_DESC_HEADER) pbuf;

    switch (type)
    {
        case USB_DT_DEVICE:
        {
            ret = (phdr->bLength == sizeof(USB_DEVICE_DESC) && phdr->bDescriptorType == USB_DT_DEVICE);
            break;
        }
        case USB_DT_CONFIG:
        {
            ret = (phdr->bLength == sizeof(USB_CONFIGURATION_DESC) && phdr->bDescriptorType == USB_DT_CONFIG);
            break;
        }
        case USB_DT_INTERFACE:
        {
            ret = (phdr->bLength == sizeof(USB_INTERFACE_DESC) && phdr->bDescriptorType == USB_DT_INTERFACE);
            break;
        }
        case USB_DT_ENDPOINT:
        {
            ret = (phdr->bLength == sizeof(USB_ENDPOINT_DESC) && phdr->bDescriptorType == USB_DT_ENDPOINT);
            break;
        }
        default:
            ret = FALSE;
    }
    return ret;
}

BOOLEAN
usb_skip_endp_desc(PBYTE * pbUF, LONG n)
{
    if (is_header_match(*pbUF, USB_DT_ENDPOINT))
    {
        (*pbUF) += sizeof(USB_ENDPOINT_DESC) * n;
        return TRUE;
    }
    return FALSE;
}

BOOLEAN
usb_skip_if_desc(PBYTE * pBUF)
{
    BOOLEAN ret;
    PUSB_INTERFACE_DESC pif_desc = (PUSB_INTERFACE_DESC) * pBUF;
    LONG endp_count;
    ret = is_header_match((PBYTE) * pBUF, USB_DT_INTERFACE);
    if (ret == TRUE)
    {
        endp_count = pif_desc->bNumEndpoints;
        if (endp_count < MAX_ENDPS_PER_IF)
        {
            pif_desc++;
            ret = usb_skip_endp_desc((PBYTE *) & pif_desc, endp_count);
            if (ret)
                *(pBUF) = (PBYTE) pif_desc;
        }
        else
            ret = FALSE;
    }
    return ret;
}

BOOLEAN
usb_skip_if_and_altif(PUCHAR * pdesc_BUF)
{
    BOOLEAN ret;
    PUSB_INTERFACE_DESC pif_desc1 = (PUSB_INTERFACE_DESC) * pdesc_BUF;
    ret = is_header_match(*pdesc_BUF, USB_DT_INTERFACE);
    if (ret == TRUE)
    {
        if (pif_desc1->bAlternateSetting == 0)
            ret = usb_skip_if_desc((PUCHAR *) & pif_desc1);
        else
            //no default interface
            ret = FALSE;

        while (ret && pif_desc1->bAlternateSetting != 0)
            ret = usb_skip_if_desc((PUCHAR *) & pif_desc1);
    }
    if (ret)
        *pdesc_BUF = (PUCHAR) pif_desc1;

    return ret;
}

BOOLEAN
usb_skip_one_config(PUCHAR *pconfig_desc_BUF)
{
    LONG if_count, i;
    BOOLEAN ret;
    PUSB_CONFIGURATION_DESC pcfg_DESC = (PUSB_CONFIGURATION_DESC) * pconfig_desc_BUF;
    PUSB_INTERFACE_DESC pif_desc2 = (PUSB_INTERFACE_DESC) & pcfg_DESC[1];

    ret = is_header_match((PUCHAR) pcfg_DESC, USB_DT_CONFIG);
    if (ret)
        *pconfig_desc_BUF = &((BYTE *) pcfg_DESC)[pcfg_DESC->wTotalLength];
    return ret;

    ret = is_header_match((PUCHAR) pcfg_DESC, USB_DT_CONFIG)
        && is_header_match((PUCHAR) pif_desc2, USB_DT_INTERFACE);

    if (ret)
    {
        if_count = pcfg_DESC->bNumInterfaces;
        if (if_count < MAX_INTERFACES_PER_CONFIG)
        {
            for(i = 0; i < if_count; i++)
            {
                ret = usb_skip_if_and_altif((PUCHAR *) & pif_desc2);
                if (ret == FALSE)
                    break;
            }
            if (ret)
                *pconfig_desc_BUF = (PUCHAR) pif_desc2;
        }
    }
    return ret;
}

PUSB_CONFIGURATION_DESC
usb_find_config_desc_by_idx(PUCHAR pbuf, LONG idx, LONG cfg_count)
{
    LONG i;
    BOOLEAN ret;
    PUSB_CONFIGURATION_DESC pcfg_desc = (PUSB_CONFIGURATION_DESC) pbuf;
    if (pcfg_desc == NULL)
        return NULL;

    if (cfg_count > MAX_CONFIGS_PER_DEV)
        return NULL;

    if (idx > cfg_count)
        return NULL;

    if (idx == 0)
        return pcfg_desc;

    for(i = 0; i < idx - 1; i++)
    {
        ret = usb_skip_one_config((PBYTE *) & pcfg_desc);
        if (ret == FALSE)
            return NULL;
    }
    return pcfg_desc;
}

PUSB_CONFIGURATION_DESC
usb_find_config_desc_by_val(PBYTE pbuf, LONG val, LONG cfg_count)
{
    LONG i;
    BOOLEAN ret;
    PUSB_CONFIGURATION_DESC pcfg_desc = (PUSB_CONFIGURATION_DESC) pbuf;
    if (pcfg_desc == NULL)
        return NULL;

    if (cfg_count > MAX_CONFIGS_PER_DEV)
        return NULL;

    for(i = 0; i < cfg_count; i++)
    {
        if (pcfg_desc->bConfigurationValue == val)
            return pcfg_desc;

        ret = usb_skip_one_config((PBYTE *) & pcfg_desc);
        if (ret == FALSE)
            return NULL;
    }

    return NULL;
}

#define if_from_handle( handle ) ( ( handle & 0xff00 ) >> 8 )

NTSTATUS
usb_submit_config_urb(PURB purb)
{
    PUSB_DEV pdev;
    PUSB_DEV_MANAGER dev_mgr;
    PUSB_ENDPOINT pendp;
    PURB purb1;
    PUSB_CTRL_SETUP_PACKET psetup;
    NTSTATUS status;
    PHCD hcd;

    USE_BASIC_NON_PENDING_IRQL;

    if (purb == NULL)
        return STATUS_INVALID_PARAMETER;

    pdev = purb->pdev;
    pendp = purb->pendp;

    lock_dev(pdev, FALSE);

    dev_mgr = dev_mgr_from_dev(pdev);
    hcd = pdev->hcd;

    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        status = STATUS_DEVICE_DOES_NOT_EXIST;
        goto LBL_OUT;
    }

    if (dev_state(pdev) == USB_DEV_STATE_FIRST_CONFIG || dev_state(pdev) == USB_DEV_STATE_RECONFIG)
    {
        //outstanding request of set configuration exists in process
        status = STATUS_UNSUCCESSFUL;
        goto LBL_OUT;
    }

    psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;

    if (dev_state(pdev) == USB_DEV_STATE_CONFIGURED
        && pdev->usb_config->pusb_config_desc->bConfigurationValue == (BYTE) psetup->wValue)
    {
        //already the current config
        status = STATUS_SUCCESS;
        goto LBL_OUT;
    }


    if (dev_state(pdev) == USB_DEV_STATE_CONFIGURED)
    {
        // not support re-configuration yet
        status = STATUS_NOT_SUPPORTED;
        goto LBL_OUT;
    }

    psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;
    purb1 = usb_alloc_mem(NonPagedPool, sizeof(URB));
    if (purb1 == NULL)
    {
        status = STATUS_NO_MEMORY;
        goto LBL_OUT;
    }

    UsbBuildSelectConfigurationRequest(purb1,
                                       usb_make_handle(pdev->dev_id, 0, 0) | 0xffff,
                                       psetup->wValue, usb_config_dev_completion, 0, ((ULONG) purb));
    purb1->pdev = pdev;
    purb1->pendp = pendp;

    //change the dev state
    pdev->flags &= ~USB_DEV_STATE_MASK;
    pdev->flags |= USB_DEV_STATE_FIRST_CONFIG;

    unlock_dev(pdev, FALSE);

    status = hcd->hcd_submit_urb(hcd, pdev, pendp, purb1);
    if (status != STATUS_PENDING)
    {
        usb_free_mem(purb1);
        purb1 = NULL;
    }
    return status;

  LBL_OUT:
    unlock_dev(pdev, FALSE);
    return status;
}


NTSTATUS
usb_submit_urb(PUSB_DEV_MANAGER dev_mgr, PURB purb)
{
    NTSTATUS status;
    PUSB_DEV pdev;
    LONG if_idx, endp_idx;
    DEV_HANDLE endp_handle;
    PUSB_CTRL_SETUP_PACKET psetup;
    PUSB_ENDPOINT pendp;

    PHCD hcd;
    USE_BASIC_NON_PENDING_IRQL;

    if (purb == NULL || dev_mgr == NULL)
        return STATUS_INVALID_PARAMETER;

    endp_handle = purb->endp_handle;

    if (endp_handle == 0)
        return STATUS_INVALID_PARAMETER;

    status = usb_query_and_lock_dev(dev_mgr, endp_handle, &pdev);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    if_idx = if_idx_from_handle(endp_handle);
    endp_idx = endp_idx_from_handle(endp_handle);

    lock_dev(pdev, FALSE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, FALSE);
        status = STATUS_DEVICE_DOES_NOT_EXIST;
        goto LBL_OUT;
    }

    if (dev_state(pdev) < USB_DEV_STATE_ADDRESSED)
    {
        unlock_dev(pdev, FALSE);
        status = STATUS_DEVICE_NOT_READY;
        goto LBL_OUT;
    }

    dev_mgr = dev_mgr_from_dev(pdev);
    hcd = pdev->hcd;

    if (default_endp_handle(endp_handle))
    {
        //default endp
        pendp = &pdev->default_endp;
    }
    else if (if_idx >= MAX_INTERFACES_PER_CONFIG || endp_idx >= MAX_ENDPS_PER_IF)
    {
        status = STATUS_INVALID_PARAMETER;
        unlock_dev(pdev, FALSE);
        goto LBL_OUT;
    }
    else
    {
        if (dev_state(pdev) < USB_DEV_STATE_CONFIGURED)
        {
            status = STATUS_DEVICE_NOT_READY;
            unlock_dev(pdev, FALSE);
            goto LBL_OUT;
        }
        pendp = &pdev->usb_config->interf[if_idx].endp[endp_idx];

    }

    purb->pdev = pdev;
    purb->pendp = pendp;

    //for default endpoint we have some special process
    if (default_endp_handle(endp_handle))
    {
        psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;
        if (psetup->bmRequestType == 0 && psetup->bRequest == USB_REQ_SET_CONFIGURATION)
        {
            unlock_dev(pdev, FALSE);
            status = usb_submit_config_urb(purb);
            goto LBL_OUT;
        }
        else if (psetup->bmRequestType == 1 && psetup->bRequest == USB_REQ_SET_INTERFACE)
        {
            unlock_dev(pdev, FALSE);
            // status = STATUS_NOT_SUPPORTED;
            status = usb_set_interface(purb);
            goto LBL_OUT;
        }
        else if (psetup->bmRequestType == 0x80 && psetup->bRequest == USB_REQ_GET_DESCRIPTOR)
        {
            if ((psetup->wValue >> 8) == USB_DT_CONFIG || (psetup->wValue >> 8) == USB_DT_DEVICE)
            {
                unlock_dev(pdev, FALSE);
                status = usb_get_descriptor(pdev, purb);
                goto LBL_OUT;

                //get the descriptor directly
                //status = hcd->hcd_submit_urb( hcd, pdev, purb->pendp, purb );
                //goto LBL_OUT;
            }
        }
        else if (psetup->bmRequestType == 0x02 && psetup->bRequest == USB_REQ_CLEAR_FEATURE && psetup->wValue == 0)     //reset pipe
        {
            ULONG endp_addr;
            BOOLEAN found;
            endp_addr = psetup->wIndex;
            if ((endp_addr & 0xf) == 0)
            {
                unlock_dev(pdev, FALSE);
                status = STATUS_INVALID_PARAMETER;
                goto LBL_OUT;
            }

            // search for the endp by the endp addr in the wIndex
            found = FALSE;
            for(if_idx = 0; if_idx < pdev->usb_config->if_count; if_idx++)
            {
                for(endp_idx = 0; endp_idx < pdev->usb_config->interf[if_idx].endp_count; endp_idx++)
                {
                    pendp = &pdev->usb_config->interf[if_idx].endp[endp_idx];
                    if (pendp->pusb_endp_desc->bEndpointAddress == endp_addr)
                    {
                        found = TRUE;
                        break;
                    }
                }
                if (found == TRUE)
                    break;
            }
            if (found)
                endp_handle = usb_make_handle(pdev->dev_id, if_idx, endp_idx);
            else
            {
                unlock_dev(pdev, FALSE);
                status = STATUS_INVALID_PARAMETER;
                goto LBL_OUT;
            }
            unlock_dev(pdev, FALSE);
            status = usb_reset_pipe_ex(dev_mgr, endp_handle, usb_reset_pipe_from_dispatch_completion, purb);

            goto LBL_OUT;
        }
    }

    unlock_dev(pdev, FALSE);
    status = hcd->hcd_submit_urb(hcd, pdev, purb->pendp, purb);

LBL_OUT:
    usb_unlock_dev(pdev);
    return status;
}

void
usb_config_dev_completion(PURB purb, PVOID context)
{
    PURB puser_urb;
    PUSB_DEV pdev;
    PUSB_ENDPOINT pendp;
    PUSB_CTRL_SETUP_PACKET psetup;
    ULONG config_val;
    NTSTATUS status;

    USE_BASIC_NON_PENDING_IRQL;

    UNREFERENCED_PARAMETER(context);

    if (purb == NULL)
    {
        return;
    }
    pdev = purb->pdev;
    pendp = purb->pendp;

    if (pdev == NULL)
        return;

    if (purb->reference != 0)
        puser_urb = (PURB) purb->reference;
    else
        puser_urb = NULL;

    lock_dev(pdev, TRUE);

    if (puser_urb)
        puser_urb->status = purb->status;

    if (purb->status != STATUS_SUCCESS)
    {
        if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
        {
            goto LBL_OUT;
        }

        if (dev_state(pdev) == USB_DEV_STATE_FIRST_CONFIG)
        {
            pdev->flags &= ~USB_DEV_STATE_MASK;
            pdev->flags |= USB_DEV_STATE_ADDRESSED;
        }
        else if (dev_state(pdev) == USB_DEV_STATE_RECONFIG)
        {
            pdev->flags &= ~USB_DEV_STATE_MASK;
            pdev->flags |= USB_DEV_STATE_CONFIGURED;

        }
        goto LBL_OUT;
    }
    // now let's construct usb_config
    if (!pdev->usb_config)
    {
        psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;
        config_val = psetup->wValue;
        status = dev_mgr_build_usb_config(pdev,
                                          &pdev->desc_buf[sizeof(USB_DEVICE_DESC)],
                                          config_val, pdev->pusb_dev_desc->bNumConfigurations);
        if (status != STATUS_SUCCESS)
        {
            pdev->flags &= ~USB_DEV_STATE_MASK;
            pdev->flags |= USB_DEV_STATE_ADDRESSED;
            goto LBL_OUT;
        }
        pdev->flags &= ~USB_DEV_STATE_MASK;
        pdev->flags |= USB_DEV_STATE_CONFIGURED;
        //this usb dev represents physical dev
        if (pdev->pusb_dev_desc->bDeviceClass == USB_CLASS_HUB && pdev->pusb_dev_desc->bDeviceSubClass == 0)
        {
            pdev->flags &= ~USB_DEV_CLASS_MASK;
            pdev->flags |= USB_DEV_CLASS_HUB;
        }
        else if (pdev->pusb_dev_desc->bDeviceClass == USB_CLASS_MASS_STORAGE
                 && pdev->pusb_dev_desc->bDeviceSubClass == 0)
        {
            pdev->flags &= ~USB_DEV_CLASS_MASK;
            pdev->flags |= USB_DEV_CLASS_MASSSTOR;
        }
        else
        {
            pdev->flags &= ~USB_DEV_CLASS_MASK;
            pdev->flags |= USB_DEV_CLASS_SCANNER;
        }
    }
    else
    {
        //not supported
        puser_urb->status = STATUS_NOT_SUPPORTED;
        pdev->flags &= ~USB_DEV_STATE_MASK;
        pdev->flags |= USB_DEV_STATE_CONFIGURED;
    }

LBL_OUT:
    unlock_dev(pdev, TRUE);
    usb_free_mem(purb);
    if (puser_urb && puser_urb->completion)
        puser_urb->completion(puser_urb, puser_urb->context);

    return;
}

NTSTATUS
usb_get_descriptor(PUSB_DEV pdev, PURB purb)
{
    PUSB_CTRL_SETUP_PACKET psetup;
    LONG idx, size, count, i;
    PBYTE buf;
    PUSB_CONFIGURATION_DESC pcfg_desc1;

    USE_BASIC_NON_PENDING_IRQL;

    if (pdev == NULL || purb == NULL)
        return STATUS_INVALID_PARAMETER;

    if (purb->data_buffer == NULL || purb->data_length == 0)
    {
        return purb->status = STATUS_INVALID_PARAMETER;
    }

    lock_dev(pdev, FALSE);
    psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;
    if (pdev->desc_buf == NULL)
    {
        purb->status = STATUS_DEVICE_NOT_READY;
        goto LBL_OUT;
    }

    if ((psetup->wValue >> 8) == USB_DT_CONFIG)
    {
        idx = (psetup->wValue & 0xff);

        count = pdev->pusb_dev_desc->bNumConfigurations;

        if (idx >= count)
        {
            purb->status = STATUS_INVALID_PARAMETER;
            goto LBL_OUT;
        }
        buf = &pdev->desc_buf[sizeof(USB_DEVICE_DESC)];
        pcfg_desc1 = usb_find_config_desc_by_idx(buf, idx, count);
        if (pcfg_desc1 == NULL)
        {
            purb->status = STATUS_UNSUCCESSFUL;
            goto LBL_OUT;
        }

        size = pcfg_desc1->wTotalLength;
        size = size > purb->data_length ? purb->data_length : size;
        for(i = 0; i < size; i++)
        {
            purb->data_buffer[i] = ((PBYTE) pcfg_desc1)[i];
        }
        purb->status = STATUS_SUCCESS;
        goto LBL_OUT;

    }
    else if ((psetup->wValue >> 8) == USB_DT_DEVICE)
    {
        size = purb->data_length > sizeof(USB_DEVICE_DESC) ? sizeof(USB_DEVICE_DESC) : purb->data_length;

        for(i = 0; i < size; i++)
        {
            purb->data_buffer[i] = ((PBYTE) pdev->pusb_dev_desc)[i];
        }
        purb->status = STATUS_SUCCESS;
    }

LBL_OUT:
    unlock_dev(pdev, FALSE);
    return purb->status;
}

LONG
usb_count_list(PLIST_HEAD list_head)
{
    LONG count;
    PLIST_ENTRY pthis, pnext;

    if (list_head == NULL)
        return 0;

    count = 0;
    ListFirst(list_head, pthis);

    while (pthis)
    {
        ListNext(list_head, pthis, pnext);
        pthis = pnext;
        count++;
    }
    return count;
}

// checks if processor supports Time Stamp Counter
__inline BOOLEAN
usb_query_clicks(PLARGE_INTEGER clicks)
{
    BOOLEAN ret_val;
    //so we have to use intel's cpu???
    ret_val = FALSE;

#ifdef _MSC_VER
    __asm
    {
        push ebx;
        push eax;
        mov eax, 1;             //read version
        cpuid;
        test edx, 0x10;         //timer stamp
        jz LBL_OUT;
        // cpuid                                //serialization
        rdtsc;
        mov ebx, dword ptr[clicks];
        mov dword ptr[ebx], eax;
        mov dword ptr[ebx + 4], edx;
        mov dword ptr[ret_val], TRUE;
LBL_OUT:
        pop eax;
        pop ebx;
    }
#else
    ret_val = FALSE;
#endif
    return ret_val;
}

VOID
usb_wait_ms_dpc(ULONG ms)
{
    LARGE_INTEGER Interval;
    if (ms <= 0)
        return;

    Interval.QuadPart = -ms * 10000;
    KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}


VOID
usb_wait_us_dpc(ULONG us)
{
    LARGE_INTEGER Interval;
    if (us <= 0)
        return;

    Interval.QuadPart = -us;
    KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}

VOID
usb_cal_cpu_freq()
{
    LARGE_INTEGER tick1, tick2;
    LONG i;
    // interval.QuadPart = -40 * 1000 * 1000;

    if (cpu_clock_freq >= 100 * 1000 * 1000)    // assume it is valid
        return;

    if (usb_query_clicks(&tick1))
    {
        for(i = 0; i < 25; i++)
        {
            usb_query_clicks(&tick1);
            KeStallExecutionProcessor(40 * 1000);
            usb_query_clicks(&tick2);
            cpu_clock_freq += (ULONG) (tick2.QuadPart - tick1.QuadPart);
        }
        // cpu_clock_freq *= 1000;
        usb_dbg_print(DBGLVL_MAXIMUM, ("usb_cal_cpu_freq(): cpu frequency = %d Hz\n", cpu_clock_freq));
    }
}

NTSTATUS
usb_set_interface(PURB purb)
{
    ULONG u;
    PURB purb1;
    PCTRL_REQ_STACK pstack;
    PUSB_DEV pdev;
    PUSB_CTRL_SETUP_PACKET psetup;
    PUSB_ENDPOINT pendp;
    NTSTATUS status;

    PHCD hcd;
    USE_BASIC_NON_PENDING_IRQL;

    purb1 = purb;
    pdev = purb->pdev;
    psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;

    lock_dev(pdev, FALSE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, FALSE);
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    if (dev_state(pdev) < USB_DEV_STATE_CONFIGURED)
    {
        unlock_dev(pdev, FALSE);
        return STATUS_DEVICE_NOT_READY;
    }

    hcd = pdev->hcd;

    if (psetup->wIndex >= pdev->usb_config->if_count)
    {
        unlock_dev(pdev, FALSE);
        return STATUS_INVALID_PARAMETER;
    }
    if (psetup->wValue >= pdev->usb_config->interf[psetup->wIndex].altif_count + 1)
    {
        unlock_dev(pdev, FALSE);
        return STATUS_INVALID_PARAMETER;
    }
    if (pdev->usb_config->interf[psetup->wIndex].pusb_if_desc->bAlternateSetting == psetup->wValue)
    {
        // already the current interface
        unlock_dev(pdev, FALSE);
        return STATUS_SUCCESS;
    }
    // check to see if the endp is busy
    for(u = 0; u < pdev->usb_config->interf[psetup->wIndex].endp_count; u++)
    {
        // This check is not adquate. Since we do not have mechanism to block the new coming
        // request during this request. the caller must guarantee no active or pending
        // usb request on these endpoint.
        pendp = &pdev->usb_config->interf[psetup->wIndex].endp[u];
        if (usb_endp_busy_count(pendp))
        {
            // active urb on that endp
            unlock_dev(pdev, FALSE);
            return STATUS_DEVICE_NOT_READY;
        }
        if (IsListEmpty(&pendp->urb_list))
        {
            // pending urb on that endp
            unlock_dev(pdev, FALSE);
            return STATUS_DEVICE_NOT_READY;
        }
    }
    unlock_dev(pdev, FALSE);

    if (purb1->ctrl_req_context.ctrl_stack_count == 0)
    {
        // ok, we have one stack cell for our use
        if (purb1->completion != NULL)
        {
            purb1->ctrl_req_context.ctrl_stack_count = 1;
            purb1->ctrl_req_context.ctrl_cur_stack = 0;
        }
        else
        {
            // use urb's completion and context
            purb1->completion = usb_set_interface_completion;
            purb1->context = pdev;
        }
    }
    else
    {
        if (purb->ctrl_req_context.ctrl_cur_stack + 1 >= purb->ctrl_req_context.ctrl_stack_count)
        {
            // stack full, let's allocate one new urb, we need stack size one
            purb1 = usb_alloc_mem(NonPagedPool, sizeof(URB));
            if (purb1 == NULL)
                return STATUS_NO_MEMORY;

            RtlCopyMemory(purb1, purb, sizeof(URB));

            // we do not use stack
            RtlZeroMemory(purb1->ctrl_req_stack, sizeof(CTRL_REQ_STACK));
            purb1->context = pdev;
            purb1->completion = usb_set_interface_completion;
            purb1->ctrl_parent_urb = purb;
            purb1->ctrl_req_context.ctrl_req_flags = CTRL_PARENT_URB_VALID;

            goto LBL_SEND_URB;
        }
        else
            purb->ctrl_req_context.ctrl_cur_stack++;
    }

    u = purb1->ctrl_req_context.ctrl_cur_stack;
    RtlZeroMemory(&purb1->ctrl_req_stack[u], sizeof(CTRL_REQ_STACK));
    pstack = &purb1->ctrl_req_stack[u];
    pstack->context = pdev;
    pstack->urb_completion = usb_set_interface_completion;

LBL_SEND_URB:
    if (hcd == NULL)
        return STATUS_INVALID_PARAMETER;

    status = hcd->hcd_submit_urb(hcd, purb->pdev, purb->pendp, purb);
    return status;
}

#define usb_complete_and_free_ctrl_urb( pURB ) \
{\
    UCHAR i, j;\
    i = pURB->ctrl_req_context.ctrl_cur_stack;\
    j = pURB->ctrl_req_context.ctrl_stack_count;\
    usb_call_ctrl_completion( pURB );\
    if( i == 0xff || j == 0 )\
        usb_free_mem( pURB );\
}

VOID
usb_set_interface_completion(PURB purb, PVOID context)
{
    PUSB_CTRL_SETUP_PACKET psetup;
    PUSB_INTERFACE pif, palt_if;
    USB_INTERFACE temp_if;
    UCHAR if_idx, if_alt_idx;
    PUSB_DEV pdev;
    PUSB_ENDPOINT pendp;
    ULONG i;
    PLIST_ENTRY pthis, pnext;

    USE_BASIC_NON_PENDING_IRQL;

    UNREFERENCED_PARAMETER(context);

    if (purb == NULL)
        return;

    if (purb->status == STATUS_SUCCESS)
    {
        psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;
        if_idx = (UCHAR) psetup->wIndex;
        if_alt_idx = (UCHAR) psetup->wValue;
        pdev = purb->pdev;
        RtlZeroMemory(&temp_if, sizeof(USB_INTERFACE));

        lock_dev(pdev, TRUE);
        if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
        {
            unlock_dev(pdev, TRUE);
            purb->status = STATUS_DEVICE_NOT_CONNECTED;
            purb->data_length = 0;
        }
        else
        {
            // let's swap the interface
            pif = &pdev->usb_config->interf[if_idx];
            ListFirst(&pif->altif_list, pthis);
            pnext = pthis;
            do
            {
                palt_if = struct_ptr(pthis, USB_INTERFACE, altif_list);
                if (palt_if->pusb_if_desc->bAlternateSetting == if_alt_idx)
                {
                    break;
                }
                palt_if = NULL;
                ListNext(&pif->altif_list, pthis, pnext);
                pthis = pnext;

            } while (pthis);

            if (palt_if != NULL)
            {
                RtlCopyMemory(&temp_if, palt_if, sizeof(USB_INTERFACE));

                palt_if->endp_count = pif->endp_count;
                RtlCopyMemory(palt_if->endp, pif->endp, sizeof(pif->endp));
                palt_if->pif_drv = pif->pif_drv;
                palt_if->pusb_if_desc = pif->pusb_if_desc;
                for(i = 0; i < palt_if->endp_count; i++)
                {
                    pendp = &palt_if->endp[i];
                    InitializeListHead(&pendp->urb_list);
                    pendp->flags = 0;
                }

                RtlCopyMemory(pif->endp, temp_if.endp, sizeof(temp_if.endp));
                pif->endp_count = temp_if.endp_count;
                pif->pusb_if_desc = temp_if.pusb_if_desc;
                for(i = 0; i < pif->endp_count; i++)
                {
                    pendp = &pif->endp[i];
                    InitializeListHead(&pendp->urb_list);
                    pendp->flags = 0;
                }
            }
            else
            {
                TRAP();
                purb->status = STATUS_UNSUCCESSFUL;
            }
        }
        unlock_dev(pdev, TRUE);
    }

    // for recursive reason, we have to store the parameter ahead
    usb_complete_and_free_ctrl_urb(purb);
}

// can only be called when current completion finished and called only in
// urb completion. And this func may be called recursively, if this routine
// is called, the urb must be treated as released.
VOID
usb_call_ctrl_completion(PURB purb)
{
    PURB parent_urb;
    PCTRL_REQ_STACK pstack;
    ULONG i;


    if (purb == NULL)
        return;

    if (purb->ctrl_req_context.ctrl_stack_count != 0)
    {
        i = purb->ctrl_req_context.ctrl_cur_stack;
        if (i > 0 && i < 0x10)
        {
            i--;
            purb->ctrl_req_context.ctrl_cur_stack = (UCHAR) i;
            pstack = &purb->ctrl_req_stack[i];
            if (pstack->urb_completion)
            {
                pstack->urb_completion(purb, pstack->context);
            }
            else
                TRAP();
        }
        else if (i == 0)
        {
            i = purb->ctrl_req_context.ctrl_cur_stack = 0xff;
            if (purb->completion)
            {
                purb->completion(purb, purb->context);
            }
            else
                TRAP();
        }
        else if (i == 0xff)
        {
            // only parent urb's completion, if parent urb exists, can be called
            if (purb->ctrl_req_context.ctrl_req_flags & CTRL_PARENT_URB_VALID)
            {
                parent_urb = purb->ctrl_parent_urb;
                if (parent_urb)
                {
                    pstack = &parent_urb->ctrl_req_stack[parent_urb->ctrl_req_context.ctrl_cur_stack];
                    pstack->urb_completion(parent_urb, pstack->context);
                }
                else
                    TRAP();
            }
        }
        else
            TRAP();
    }
    else if (purb->ctrl_req_context.ctrl_req_flags & CTRL_PARENT_URB_VALID)
    {
        // this is the case when the child urb won't use the stack
        parent_urb = purb->ctrl_parent_urb;
        if (parent_urb)
        {
            // pstack = &parent_urb->ctrl_req_stack[ parent_urb->ctrl_req_context.ctrl_cur_stack ];
            // pstack->urb_completion( parent_urb, pstack->context );
            usb_call_ctrl_completion(parent_urb);
        }
        else
            TRAP();
    }
    else
        return;
}
