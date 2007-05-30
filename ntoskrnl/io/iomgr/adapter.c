/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/adapter.c
 * PURPOSE:         I/O Wrappers for HAL Adapter APIs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Filip Navara (navaraf@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* DATA **********************************************************************/

POBJECT_TYPE IoAdapterObjectType;
POBJECT_TYPE IoDeviceHandlerObjectType;
ULONG IoDeviceHandlerObjectSize;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoAllocateAdapterChannel(IN PADAPTER_OBJECT AdapterObject,
                         IN PDEVICE_OBJECT DeviceObject,
                         IN ULONG NumberOfMapRegisters,
                         IN PDRIVER_CONTROL ExecutionRoutine,
                         IN PVOID Context)
{
    PWAIT_CONTEXT_BLOCK Wcb = &DeviceObject->Queue.Wcb;

    /* Initialize the WCB */
    Wcb->DeviceObject = DeviceObject;
    Wcb->DeviceContext = Context;
    Wcb->CurrentIrp = DeviceObject->CurrentIrp;

    /* Call HAL */
    return HalAllocateAdapterChannel(AdapterObject,
                                     Wcb,
                                     NumberOfMapRegisters,
                                     ExecutionRoutine);
}

/* EOF */
