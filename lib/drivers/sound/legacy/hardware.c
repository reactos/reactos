/*
    ReactOS Sound System
    Hardware interaction helper

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        25 May 2008 - Created

    Notes:
        This uses some obsolete calls (eg: HalGetInterruptVector).
        Might be worth updating this in future to use some of the
        recommended functions like IoReportDetectedDevice and
        IoReportResourceForDetection...
*/

#include <ntddk.h>
#include <ntddsnd.h>
#include <debug.h>

/* NOTE: Disconnect using IoDisconnectInterrupt */

NTSTATUS
LegacyAttachInterrupt(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  UCHAR Irq,
    IN  PKSERVICE_ROUTINE ServiceRoutine,
    OUT PKINTERRUPT* InterruptObject)
{
    NTSTATUS Status;
    ULONG Vector;
    KIRQL IrqLevel;
    KAFFINITY Affinity;

    DPRINT("Obtaining interrupt vector");

    Vector = HalGetInterruptVector(Isa,
                                   0,
                                   Irq,
                                   Irq,
                                   &IrqLevel,
                                   &Affinity);

    DPRINT("Vector %d", Vector);
    DPRINT("Connecting IRQ %d", Irq);

    Status = IoConnectInterrupt(InterruptObject,
                                ServiceRoutine,
                                DeviceObject,
                                NULL,
                                Vector,
                                IrqLevel,
                                IrqLevel,
                                Latched,
                                FALSE,
                                Affinity,
                                FALSE);

    if ( Status == STATUS_INVALID_PARAMETER )
    {
        Status = STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    return Status;
}
