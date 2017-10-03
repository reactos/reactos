/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/hardware.c
 * PURPOSE:     Hardware related routines.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 */

#include "hardware.h"
#define NDEBUG
#include <debug.h>

//FORCEINLINE
VOID
SetAsyncListQueueRegister(PEHCI_HOST_CONTROLLER hcd, ULONG PhysicalAddr)
{   
    ULONG OpRegisters = hcd->OpRegisters;
    WRITE_REGISTER_ULONG((PULONG)(OpRegisters + EHCI_ASYNCLISTBASE), PhysicalAddr);
}

//FORCEINLINE
ULONG
GetAsyncListQueueRegister(PEHCI_HOST_CONTROLLER hcd)
{
    ULONG OpRegisters = hcd->OpRegisters;
    return READ_REGISTER_ULONG((PULONG)(OpRegisters + EHCI_ASYNCLISTBASE));
}

//FORCEINLINE
VOID
SetPeriodicFrameListRegister(PEHCI_HOST_CONTROLLER hcd, ULONG PhysicalAddr)
{
    ULONG OpRegisters = hcd->OpRegisters;
    WRITE_REGISTER_ULONG((PULONG)(OpRegisters + EHCI_PERIODICLISTBASE), PhysicalAddr);
}

//FORCEINLINE
ULONG
GetPeriodicFrameListRegister(PEHCI_HOST_CONTROLLER hcd)
{
    ULONG OpRegisters = hcd->OpRegisters;
    return READ_REGISTER_ULONG((PULONG) (OpRegisters + EHCI_PERIODICLISTBASE));
}

//FORCEINLINE
ULONG
ReadControllerStatus(PEHCI_HOST_CONTROLLER hcd)
{
    ULONG OpRegisters = hcd->OpRegisters;
    return READ_REGISTER_ULONG ((PULONG) (OpRegisters + EHCI_USBSTS));
}

//FORCEINLINE
VOID
ClearControllerStatus(PEHCI_HOST_CONTROLLER hcd, ULONG Status)
{
    ULONG OpRegisters = hcd->OpRegisters;
    WRITE_REGISTER_ULONG((PULONG) (OpRegisters + EHCI_USBSTS), Status);
}

VOID
ResetPort(PEHCI_HOST_CONTROLLER hcd, UCHAR Port)
{
    ULONG tmp;
    ULONG OpRegisters = hcd->OpRegisters;
    DPRINT("Reset Port %x\n", Port);

    tmp = READ_REGISTER_ULONG((PULONG) ((OpRegisters + EHCI_PORTSC) + (4 * Port)));
    if (tmp & 0x400)
    {
        DPRINT1("Non HighSpeed device connected. Releasing ownership.\n");
        WRITE_REGISTER_ULONG((PULONG) ((OpRegisters + EHCI_PORTSC) + (4 * Port)), 0x2000);
    }

    /* Get current port state */
    tmp = READ_REGISTER_ULONG((PULONG) ((OpRegisters + EHCI_PORTSC) + (4 * Port)));

    /* Set reset and clear enable */
    tmp |= 0x100;
    tmp &= ~0x04;
    WRITE_REGISTER_ULONG((PULONG) ((OpRegisters + EHCI_PORTSC) + (4 * Port)), tmp);

    /* USB 2.0 Spec 10.2.8.1, more than 50ms */
    KeStallExecutionProcessor(100);

    /* Clear reset */
    tmp &= ~0x100;
    WRITE_REGISTER_ULONG((PULONG) ((OpRegisters + EHCI_PORTSC) + (4 * Port)), tmp);

    KeStallExecutionProcessor(100);

    tmp = READ_REGISTER_ULONG((PULONG) ((OpRegisters + EHCI_PORTSC) + (4 * Port)));

    if (tmp & 0x100)
    {
        DPRINT1("EHCI ERROR: Port Reset did not complete!\n");
        ASSERT(FALSE);
    }
    DPRINT("Reset complete\n");
}

VOID
StopEhci(PEHCI_HOST_CONTROLLER hcd)
{
    ULONG OpRegisters = hcd->OpRegisters;
    PEHCI_USBCMD_CONTENT UsbCmd;
    PEHCI_USBSTS_CONTEXT UsbSts;
    LONG FailSafe;
    LONG tmp;

    DPRINT("Stopping Ehci controller\n");

    WRITE_REGISTER_ULONG((PULONG) (OpRegisters + EHCI_USBINTR), 0);

    tmp = READ_REGISTER_ULONG((PULONG) (OpRegisters + EHCI_USBCMD));
    UsbCmd = (PEHCI_USBCMD_CONTENT) & tmp;
    UsbCmd->Run = FALSE;
    WRITE_REGISTER_ULONG((PULONG) (OpRegisters + EHCI_USBCMD), tmp);

    /* Wait for the device to stop */
    for (FailSafe = 0; FailSafe < 1000; FailSafe++)
    {
        KeStallExecutionProcessor(10);
        tmp = READ_REGISTER_ULONG((PULONG)(OpRegisters + EHCI_USBSTS));
        UsbSts = (PEHCI_USBSTS_CONTEXT)&tmp;

        if (UsbSts->HCHalted)
        {
            break;
        }
    }
    if (!UsbSts->HCHalted)
    {
        DPRINT1("EHCI ERROR: Controller is not responding to Stop request!\n");
        ASSERT(FALSE);
    }
}

VOID
StartEhci(PEHCI_HOST_CONTROLLER hcd)
{
    ULONG OpRegisters = hcd->OpRegisters;
    PEHCI_USBCMD_CONTENT UsbCmd;
    PEHCI_USBSTS_CONTEXT UsbSts;
    LONG failSafe;
    LONG tmp;
    LONG tmp2;

    DPRINT("Starting Ehci controller\n");

    tmp = READ_REGISTER_ULONG((PULONG)(OpRegisters + EHCI_USBSTS));
    UsbSts = (PEHCI_USBSTS_CONTEXT)&tmp;

    if (!UsbSts->HCHalted)
    {
        StopEhci(hcd);
    }

    tmp = READ_REGISTER_ULONG ((PULONG)(OpRegisters + EHCI_USBCMD));

    /* Reset the device */
    UsbCmd = (PEHCI_USBCMD_CONTENT) &tmp;
    UsbCmd->HCReset = TRUE;
    WRITE_REGISTER_ULONG ((PULONG)(OpRegisters + EHCI_USBCMD), tmp);

    /* Wait for the device to reset */
    for (failSafe = 0; failSafe < 1000; failSafe++)
    {
        KeStallExecutionProcessor(10);
        tmp = READ_REGISTER_ULONG((PULONG)(OpRegisters + EHCI_USBCMD));
        UsbCmd = (PEHCI_USBCMD_CONTENT)&tmp;

        if (!UsbCmd->HCReset)
        {
            break;
        }
        DPRINT("Waiting for reset, USBCMD: %x\n", READ_REGISTER_ULONG ((PULONG)(OpRegisters + EHCI_USBCMD)));
    }

    if (UsbCmd->HCReset)
    {
        DPRINT1("EHCI ERROR: Controller failed to reset! Will attempt to continue.\n");
        ASSERT(FALSE);
    }

    UsbCmd = (PEHCI_USBCMD_CONTENT) &tmp;

    /* Disable Interrupts on the device */
    WRITE_REGISTER_ULONG((PULONG)(OpRegisters + EHCI_USBINTR), 0);
    /* Clear the Status */
    WRITE_REGISTER_ULONG((PULONG)(OpRegisters + EHCI_USBSTS), 0x0000001f);

    WRITE_REGISTER_ULONG((PULONG)(OpRegisters + EHCI_CTRLDSSEGMENT), 0);

    SetAsyncListQueueRegister(hcd, hcd->AsyncListQueue->PhysicalAddr | QH_TYPE_QH);
    /* Set the ansync and periodic to disable */
    UsbCmd->PeriodicEnable = FALSE;
    UsbCmd->AsyncEnable = TRUE;
    WRITE_REGISTER_ULONG((PULONG)(OpRegisters + EHCI_USBCMD), tmp);

    /* Set the threshold */
    UsbCmd->IntThreshold = 1;
    WRITE_REGISTER_ULONG((PULONG)(OpRegisters + EHCI_USBCMD), tmp);

    /* Turn back on interrupts */
    WRITE_REGISTER_ULONG((PULONG)(OpRegisters + EHCI_USBINTR),
                        EHCI_USBINTR_ERR | EHCI_USBINTR_ASYNC | EHCI_USBINTR_HSERR
                        /*| EHCI_USBINTR_FLROVR*/  | EHCI_USBINTR_PC);
    WRITE_REGISTER_ULONG((PULONG)(OpRegisters + EHCI_USBINTR),
                        EHCI_USBINTR_INTE | EHCI_USBINTR_ERR | EHCI_USBINTR_ASYNC | EHCI_USBINTR_HSERR
                        /*| EHCI_USBINTR_FLROVR*/  | EHCI_USBINTR_PC);

    UsbCmd->Run = TRUE;
    WRITE_REGISTER_ULONG((PULONG)(OpRegisters + EHCI_USBCMD), tmp);

    /* Wait for the device to start */
    for (failSafe = 0; failSafe < 1000; failSafe++)
    {
        KeStallExecutionProcessor(10);
        tmp2 = READ_REGISTER_ULONG((PULONG)(OpRegisters + EHCI_USBSTS));
        UsbSts = (PEHCI_USBSTS_CONTEXT)&tmp2;

        if (!UsbSts->HCHalted)
        {
            break;
        }
        DPRINT("Waiting for start, USBSTS: %x\n", READ_REGISTER_ULONG ((PULONG)(OpRegisters + EHCI_USBSTS)));
    }

    if (UsbSts->HCHalted)
    {
        DPRINT1("EHCI ERROR: Controller failed to start!!!\n");
        ASSERT(FALSE);
    }
    /* Set all port routing to ECHI controller */
    WRITE_REGISTER_ULONG((PULONG)(OpRegisters + EHCI_CONFIGFLAG), 1);
}

BOOLEAN
EnumControllerPorts(PEHCI_HOST_CONTROLLER hcd)
{
    ULONG tmp, i, OpRegisters, ChildDeviceCount = 0;
    BOOLEAN PortChange = FALSE;

    OpRegisters = (ULONG)hcd->OpRegisters;
    /* Loop through the ports */
    for (i = 0; i < hcd->ECHICaps.HCSParams.PortCount; i++)
    {
        tmp = READ_REGISTER_ULONG((PULONG) ((OpRegisters + EHCI_PORTSC) + (4 * i)));

        /* Check for port change on this port */
        if (tmp & 0x02)
        {
            DPRINT1("Port Change\n");

            /* Clear status change */
            tmp = READ_REGISTER_ULONG((PULONG)((OpRegisters + EHCI_PORTSC) + (4 * i)));
            tmp |= 0x02;
            WRITE_REGISTER_ULONG((PULONG) ((OpRegisters + EHCI_PORTSC) + (4 * i)), tmp);
            
            PortChange = TRUE;
            /* Connect or Disconnect? */
            if (tmp & 0x01)
            {
                DPRINT1("Device connected on port %d\n", i);

                /* Check if a companion host controller exists */
                if (hcd->ECHICaps.HCSParams.CHCCount)
                {
                    tmp = READ_REGISTER_ULONG((PULONG)((OpRegisters + EHCI_PORTSC) + (4 * i)));

                    /* Port should be in disabled state, as per USB 2.0 specs */
                    if (tmp & 0x04)
                    {
                        DPRINT1("Warning: The port the device has just connected to is not disabled!\n");
                    }

                    /* Is this non high speed device */
                    if (tmp & 0x400)
                    {
                        DPRINT1("Non HighSpeed device connected. Releasing ownership.\n");
                        /* Release ownership to companion host controller */
                        WRITE_REGISTER_ULONG((PULONG) ((OpRegisters + EHCI_PORTSC) + (4 * i)), 0x2000);
                        continue;
                    }
                }

                KeStallExecutionProcessor(30);

                /* As per USB 2.0 Specs, 9.1.2. Reset the port and clear the status change */
                //tmp |= 0x100 | 0x02;
                /* Sanity, Disable port */
                //tmp &= ~0x04;

                //WRITE_REGISTER_ULONG((PULONG) ((Base + EHCI_PORTSC) + (4 * i)), tmp);

                //KeStallExecutionProcessor(20);

                tmp = READ_REGISTER_ULONG((PULONG)((OpRegisters + EHCI_PORTSC) + (4 * i)));

                ChildDeviceCount++;
                hcd->Ports[i].PortStatus &= ~0x8000;
                DPRINT1("Removed 0x8000\n");
                ASSERT(FALSE);
                hcd->Ports[i].PortStatus |= USB_PORT_STATUS_HIGH_SPEED;
                hcd->Ports[i].PortStatus |= USB_PORT_STATUS_CONNECT;
                hcd->Ports[i].PortChange |= USB_PORT_STATUS_CONNECT;
            }
            else
            {
                DPRINT1("Device disconnected on port %d\n", i);
                ChildDeviceCount--;
            }
        }
    }
    return PortChange;
}

VOID
GetCapabilities(PEHCI_CAPS PCap, ULONG CapRegister)
{
    PEHCI_HCS_CONTENT PHCS;
    LONG i;

    if (!PCap)
        return;

    PCap->Length = READ_REGISTER_UCHAR((PUCHAR)CapRegister);
    PCap->Reserved = READ_REGISTER_UCHAR((PUCHAR)(CapRegister + 1));
    PCap->HCIVersion = READ_REGISTER_USHORT((PUSHORT)(CapRegister + 2));
    PCap->HCSParamsLong = READ_REGISTER_ULONG((PULONG)(CapRegister + 4));
    PCap->HCCParams = READ_REGISTER_ULONG((PULONG)(CapRegister + 8));

    DPRINT("Length %d\n", PCap->Length);
    DPRINT("Reserved %d\n", PCap->Reserved);
    DPRINT("HCIVersion %x\n", PCap->HCIVersion);
    DPRINT("HCSParams %x\n", PCap->HCSParamsLong);
    DPRINT("HCCParams %x\n", PCap->HCCParams);

    if (PCap->HCCParams & 0x02)
        DPRINT1("Frame list size is configurable\n");

    if (PCap->HCCParams & 0x01)
        DPRINT1("64bit address mode not supported!\n");

    DPRINT1("Number of Ports: %d\n", PCap->HCSParams.PortCount);

    if (PCap->HCSParams.PortPowerControl)
        DPRINT1("Port Power Control is enabled\n");

    if (!PCap->HCSParams.CHCCount)
    {
        DPRINT1("Number of Companion Host controllers %x\n", PCap->HCSParams.CHCCount);
        DPRINT1("Number of Ports Per CHC: %d\n", PCap->HCSParams.PortPerCHC);
    }

    PHCS = (PEHCI_HCS_CONTENT)&PCap->HCSParams;
    if (PHCS->PortRouteRules)
    {
        for (i = 0; i < PCap->HCSParams.PortCount; i++)
        {
            PCap->PortRoute[i] = READ_REGISTER_UCHAR((PUCHAR) (CapRegister + 12 + i));
        }
    }
}
