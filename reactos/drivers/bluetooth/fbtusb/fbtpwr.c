// Copyright (c) 2004, Antony C. Roberts

// Use of this file is subject to the terms
// described in the LICENSE.TXT file that
// accompanies this file.
//
// Your use of this file indicates your
// acceptance of the terms described in
// LICENSE.TXT.
//
// http://www.freebt.net

#include "fbtusb.h"
#include "fbtpwr.h"
#include "fbtpnp.h"
#include "fbtdev.h"
#include "fbtrwr.h"
#include "fbtwmi.h"

#include "fbtusr.h"

// Handle power events
NTSTATUS NTAPI FreeBT_DispatchPower(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS           ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;
    //PUNICODE_STRING    tagString;
    PDEVICE_EXTENSION  deviceExtension;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    // We don't queue power Irps, we'll only check if the
    // device was removed, otherwise we'll take appropriate
    // action and send it to the next lower driver. In general
    // drivers should not cause long delays while handling power
    // IRPs. If a driver cannot handle a power IRP in a brief time,
    // it should return STATUS_PENDING and queue all incoming
    // IRPs until the IRP completes.
    if (Removed == deviceExtension->DeviceState)
    {

        // Even if a driver fails the IRP, it must nevertheless call
        // PoStartNextPowerIrp to inform the Power Manager that it
        // is ready to handle another power IRP.
        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = ntStatus = STATUS_DELETE_PENDING;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return ntStatus;

    }

    if (NotStarted == deviceExtension->DeviceState)
    {
        // if the device is not started yet, pass it down
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);

        return PoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

    }

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchPower::"));
    FreeBT_IoIncrement(deviceExtension);

    switch(irpStack->MinorFunction)
    {
    case IRP_MN_SET_POWER:
        // The Power Manager sends this IRP for one of the
        // following reasons:

        // 1) To notify drivers of a change to the system power state.
        // 2) To change the power state of a device for which
        //    the Power Manager is performing idle detection.

        // A driver sends IRP_MN_SET_POWER to change the power
        // state of its device if it's a power policy owner for the
        // device.
        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchPower: IRP_MN_SET_POWER\n"));
        IoMarkIrpPending(Irp);

        switch(irpStack->Parameters.Power.Type)
        {
        case SystemPowerState:
            HandleSystemSetPower(DeviceObject, Irp);
            ntStatus = STATUS_PENDING;
            break;

        case DevicePowerState:
            HandleDeviceSetPower(DeviceObject, Irp);
            ntStatus = STATUS_PENDING;
            break;

        }

        break;

    case IRP_MN_QUERY_POWER:
        // The Power Manager sends a power IRP with the minor
        // IRP code IRP_MN_QUERY_POWER to determine whether it
        // can safely change to the specified system power state
        // (S1-S5) and to allow drivers to prepare for such a change.
        // If a driver can put its device in the requested state,
        // it sets status to STATUS_SUCCESS and passes the IRP down.
        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchPower: IRP_MN_QUERY_POWER\n"));
        IoMarkIrpPending(Irp);

        switch(irpStack->Parameters.Power.Type)
        {
        case SystemPowerState:
            HandleSystemQueryPower(DeviceObject, Irp);
            ntStatus = STATUS_PENDING;
            break;

        case DevicePowerState:
            HandleDeviceQueryPower(DeviceObject, Irp);
            ntStatus = STATUS_PENDING;
            break;

        }

        break;

    case IRP_MN_WAIT_WAKE:
        // The minor power IRP code IRP_MN_WAIT_WAKE provides
        // for waking a device or waking the system. Drivers
        // of devices that can wake themselves or the system
        // send IRP_MN_WAIT_WAKE. The system sends IRP_MN_WAIT_WAKE
        // only to devices that always wake the system, such as
        // the power-on switch.
        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchPower: IRP_MN_WAIT_WAKE\n"));
        IoMarkIrpPending(Irp);
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(
                        Irp,
                        (PIO_COMPLETION_ROUTINE)WaitWakeCompletionRoutine,
                        deviceExtension,
                        TRUE,
                        TRUE,
                        TRUE);

        PoStartNextPowerIrp(Irp);
        ntStatus = PoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);
        if(!NT_SUCCESS(ntStatus))
        {
            FreeBT_DbgPrint(1, ("FBTUSB: Lower drivers failed the wait-wake Irp\n"));

        }

        ntStatus = STATUS_PENDING;

        // push back the count HERE and NOT in completion routine
        // a pending Wait Wake Irp should not impede stopping the device
        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchPower: IRP_MN_WAIT_WAKE::"));
        FreeBT_IoDecrement(deviceExtension);
        break;

    case IRP_MN_POWER_SEQUENCE:
        // A driver sends this IRP as an optimization to determine
        // whether its device actually entered a specific power state.
        // This IRP is optional. Power Manager cannot send this IRP.
        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchPower: IRP_MN_POWER_SEQUENCE\n"));

    default:
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        ntStatus = PoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);
        if(!NT_SUCCESS(ntStatus))
        {
            FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_DispatchPower: Lower drivers failed this Irp\n"));

        }

        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchPower::"));
        FreeBT_IoDecrement(deviceExtension);

        break;

    }

    return ntStatus;

}

NTSTATUS NTAPI HandleSystemQueryPower(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS           ntStatus;
    PDEVICE_EXTENSION  deviceExtension;
    SYSTEM_POWER_STATE systemState;
    PIO_STACK_LOCATION irpStack;

    FreeBT_DbgPrint(3, ("FBTUSB: HandleSystemQueryPower: Entered\n"));

    // initialize variables
    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    systemState = irpStack->Parameters.Power.State.SystemState;

    FreeBT_DbgPrint(3, ("FBTUSB: HandleSystemQueryPower: Query for system power state S%X\n"
                        "FBTUSB: HandleSystemQueryPower: Current system power state S%X\n",
                         systemState - 1,
                         deviceExtension->SysPower - 1));

    // Fail a query for a power state incompatible with waking up the system
    if ((deviceExtension->WaitWakeEnable) && (systemState > deviceExtension->DeviceCapabilities.SystemWake))
    {
        FreeBT_DbgPrint(1, ("FBTUSB: HandleSystemQueryPower: Query for an incompatible system power state\n"));

        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = ntStatus = STATUS_INVALID_DEVICE_STATE;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        FreeBT_DbgPrint(3, ("FBTUSB: HandleSystemQueryPower::"));
        FreeBT_IoDecrement(deviceExtension);

        return ntStatus;

    }

    // if querying for a lower S-state, issue a wait-wake
    if((systemState > deviceExtension->SysPower) && (deviceExtension->WaitWakeEnable))
    {
        IssueWaitWake(deviceExtension);

    }

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(
            Irp,
            (PIO_COMPLETION_ROUTINE)SysPoCompletionRoutine,
            deviceExtension,
            TRUE,
            TRUE,
            TRUE);

    ntStatus = PoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);
    FreeBT_DbgPrint(3, ("FBTUSB: HandleSystemQueryPower: Leaving\n"));

    return STATUS_PENDING;

}

NTSTATUS NTAPI HandleSystemSetPower(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    NTSTATUS           ntStatus;
    PDEVICE_EXTENSION  deviceExtension;
    SYSTEM_POWER_STATE systemState;
    PIO_STACK_LOCATION irpStack;

    FreeBT_DbgPrint(3, ("FBTUSB: HandleSystemSetPower: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    systemState = irpStack->Parameters.Power.State.SystemState;

    FreeBT_DbgPrint(3, ("FBTUSB: HandleSystemSetPower: Set request for system power state S%X\n"
                         "FBTUSB: HandleSystemSetPower: Current system power state S%X\n",
                         systemState - 1,
                         deviceExtension->SysPower - 1));

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(
            Irp,
            (PIO_COMPLETION_ROUTINE)SysPoCompletionRoutine,
            deviceExtension,
            TRUE,
            TRUE,
            TRUE);

    ntStatus = PoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);
    FreeBT_DbgPrint(3, ("FBTUSB: HandleSystemSetPower: Leaving\n"));

    return STATUS_PENDING;

}

NTSTATUS NTAPI HandleDeviceQueryPower(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS           ntStatus;
    PDEVICE_EXTENSION  deviceExtension;
    PIO_STACK_LOCATION irpStack;
    DEVICE_POWER_STATE deviceState;

    FreeBT_DbgPrint(3, ("FBTUSB: HandleDeviceQueryPower: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceState = irpStack->Parameters.Power.State.DeviceState;

    FreeBT_DbgPrint(3, ("FBTUSB: HandleDeviceQueryPower: Query for device power state D%X\n"
                         "FBTUSB: HandleDeviceQueryPower: Current device power state D%X\n",
                         deviceState - 1,
                         deviceExtension->DevPower - 1));

    if (deviceExtension->WaitWakeEnable && deviceState > deviceExtension->DeviceCapabilities.DeviceWake)
    {
        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = ntStatus = STATUS_INVALID_DEVICE_STATE;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        FreeBT_DbgPrint(3, ("FBTUSB: HandleDeviceQueryPower::"));
        FreeBT_IoDecrement(deviceExtension);

        return ntStatus;

    }

    if (deviceState < deviceExtension->DevPower)
    {
        ntStatus = STATUS_SUCCESS;

    }

    else
    {
        ntStatus = HoldIoRequests(DeviceObject, Irp);
        if(STATUS_PENDING == ntStatus)
        {
            return ntStatus;

        }

    }

    // on error complete the Irp.
    // on success pass it to the lower layers
    PoStartNextPowerIrp(Irp);
    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;
    if(!NT_SUCCESS(ntStatus))
    {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

    }

    else
    {
        IoSkipCurrentIrpStackLocation(Irp);
        ntStatus=PoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

    }

    FreeBT_DbgPrint(3, ("FBTUSB: HandleDeviceQueryPower::"));
    FreeBT_IoDecrement(deviceExtension);

    FreeBT_DbgPrint(3, ("FBTUSB: HandleDeviceQueryPower: Leaving\n"));

    return ntStatus;

}


NTSTATUS NTAPI SysPoCompletionRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS           ntStatus;
    PIO_STACK_LOCATION irpStack;

    ntStatus = Irp->IoStatus.Status;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    FreeBT_DbgPrint(3, ("FBTUSB: SysPoCompletionRoutine: Entered\n"));

    // lower drivers failed this Irp
    if(!NT_SUCCESS(ntStatus))
    {
        PoStartNextPowerIrp(Irp);
        FreeBT_DbgPrint(3, ("FBTUSB: SysPoCompletionRoutine::"));
        FreeBT_IoDecrement(DeviceExtension);

        return STATUS_SUCCESS;

    }

    // ..otherwise update the cached system power state (IRP_MN_SET_POWER)
    if(irpStack->MinorFunction == IRP_MN_SET_POWER)
    {
        DeviceExtension->SysPower = irpStack->Parameters.Power.State.SystemState;

    }

    // queue device irp and return STATUS_MORE_PROCESSING_REQUIRED
    SendDeviceIrp(DeviceObject, Irp);

    FreeBT_DbgPrint(3, ("FBTUSB: SysPoCompletionRoutine: Leaving\n"));

    return STATUS_MORE_PROCESSING_REQUIRED;

}

VOID NTAPI SendDeviceIrp(IN PDEVICE_OBJECT DeviceObject, IN PIRP SIrp )
{
    NTSTATUS                  ntStatus;
    POWER_STATE               powState;
    PDEVICE_EXTENSION         deviceExtension;
    PIO_STACK_LOCATION        irpStack;
    SYSTEM_POWER_STATE        systemState;
    DEVICE_POWER_STATE        devState;
    PPOWER_COMPLETION_CONTEXT powerContext;

    irpStack = IoGetCurrentIrpStackLocation(SIrp);
    systemState = irpStack->Parameters.Power.State.SystemState;
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    FreeBT_DbgPrint(3, ("FBTUSB: SendDeviceIrp: Entered\n"));

    // Read out the D-IRP out of the S->D mapping array captured in QueryCap's.
    // we can choose deeper sleep states than our mapping but never choose
    // lighter ones.
    devState = deviceExtension->DeviceCapabilities.DeviceState[systemState];
    powState.DeviceState = devState;

    powerContext = (PPOWER_COMPLETION_CONTEXT) ExAllocatePool(NonPagedPool, sizeof(POWER_COMPLETION_CONTEXT));
    if (!powerContext)
    {
        FreeBT_DbgPrint(1, ("FBTUSB: SendDeviceIrp: Failed to alloc memory for powerContext\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;

    }

    else
    {
        powerContext->DeviceObject = DeviceObject;
        powerContext->SIrp = SIrp;

        // in win2k PoRequestPowerIrp can take fdo or pdo.
        ntStatus = PoRequestPowerIrp(
                            deviceExtension->PhysicalDeviceObject,
                            irpStack->MinorFunction,
                            powState,
                            (PREQUEST_POWER_COMPLETE)DevPoCompletionRoutine,
                            powerContext,
                            NULL);

    }

    if (!NT_SUCCESS(ntStatus))
    {
        if (powerContext)
        {
            ExFreePool(powerContext);

        }

        PoStartNextPowerIrp(SIrp);
        SIrp->IoStatus.Status = ntStatus;
        SIrp->IoStatus.Information = 0;
        IoCompleteRequest(SIrp, IO_NO_INCREMENT);

        FreeBT_DbgPrint(3, ("FBTUSB: SendDeviceIrp::"));
        FreeBT_IoDecrement(deviceExtension);

    }

    FreeBT_DbgPrint(3, ("FBTUSB: SendDeviceIrp: Leaving\n"));

}


VOID NTAPI DevPoCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PIRP                      sIrp;
    PDEVICE_EXTENSION         deviceExtension;
    PPOWER_COMPLETION_CONTEXT powerContext;

    powerContext = (PPOWER_COMPLETION_CONTEXT) Context;
    sIrp = powerContext->SIrp;
    deviceExtension = (PDEVICE_EXTENSION) powerContext->DeviceObject->DeviceExtension;

    FreeBT_DbgPrint(3, ("FBTUSB: DevPoCompletionRoutine: Entered\n"));

    sIrp->IoStatus.Status = IoStatus->Status;
    PoStartNextPowerIrp(sIrp);
    sIrp->IoStatus.Information = 0;
    IoCompleteRequest(sIrp, IO_NO_INCREMENT);

    FreeBT_DbgPrint(3, ("FBTUSB: DevPoCompletionRoutine::"));
    FreeBT_IoDecrement(deviceExtension);

    ExFreePool(powerContext);

    FreeBT_DbgPrint(3, ("FBTUSB: DevPoCompletionRoutine: Leaving\n"));

}

NTSTATUS NTAPI HandleDeviceSetPower(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    KIRQL              oldIrql;
    NTSTATUS           ntStatus;
    POWER_STATE        newState;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION  deviceExtension;
    DEVICE_POWER_STATE newDevState,
                       oldDevState;

    FreeBT_DbgPrint(3, ("FBTUSB: HandleDeviceSetPower: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    oldDevState = deviceExtension->DevPower;
    newState = irpStack->Parameters.Power.State;
    newDevState = newState.DeviceState;

    FreeBT_DbgPrint(3, ("FBTUSB: HandleDeviceSetPower: Set request for device power state D%X\n"
                         "FBTUSB: HandleDeviceSetPower: Current device power state D%X\n",
                         newDevState - 1,
                         deviceExtension->DevPower - 1));

    if (newDevState < oldDevState)
    {

        FreeBT_DbgPrint(3, ("FBTUSB: HandleDeviceSetPower: Adding power to the device\n"));

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(
                Irp,
                (PIO_COMPLETION_ROUTINE)FinishDevPoUpIrp,
                deviceExtension,
                TRUE,
                TRUE,
                TRUE);

        ntStatus = PoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

    }

    else
    {
        // newDevState >= oldDevState

        // hold I/O if transition from D0 -> DX (X = 1, 2, 3)
        // if transition from D1 or D2 to deeper sleep states,
        // I/O queue is already on hold.
        if(PowerDeviceD0 == oldDevState && newDevState > oldDevState)
        {
            // D0 -> DX transition
            FreeBT_DbgPrint(3, ("FBTUSB: HandleDeviceSetPower: Removing power from the device\n"));

            ntStatus = HoldIoRequests(DeviceObject, Irp);
            if (!NT_SUCCESS(ntStatus))
            {
                PoStartNextPowerIrp(Irp);
                Irp->IoStatus.Status = ntStatus;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);

                FreeBT_DbgPrint(3, ("FBTUSB: HandleDeviceSetPower::"));
                FreeBT_IoDecrement(deviceExtension);

                return ntStatus;

            }

            else
            {
                goto HandleDeviceSetPower_Exit;

            }

        }

        else if (PowerDeviceD0 == oldDevState && PowerDeviceD0 == newDevState)
        {
            // D0 -> D0
            // unblock the queue which may have been blocked processing
            // query irp
            FreeBT_DbgPrint(3, ("FBTUSB: HandleDeviceSetPower: A SetD0 request\n"));

            KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);
            deviceExtension->QueueState = AllowRequests;
            KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);

            ProcessQueuedRequests(deviceExtension);

        }

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(
                Irp,
                (PIO_COMPLETION_ROUTINE) FinishDevPoDnIrp,
                deviceExtension,
                TRUE,
                TRUE,
                TRUE);

        ntStatus = PoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);
        if(!NT_SUCCESS(ntStatus))
        {
            FreeBT_DbgPrint(1, ("FBTUSB: HandleDeviceSetPower: Lower drivers failed a power Irp\n"));

        }

    }

HandleDeviceSetPower_Exit:

    FreeBT_DbgPrint(3, ("FBTUSB: HandleDeviceSetPower: Leaving\n"));

    return STATUS_PENDING;

}

NTSTATUS NTAPI FinishDevPoUpIrp(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS           ntStatus;

    FreeBT_DbgPrint(3, ("FBTUSB: FinishDevPoUpIrp: Entered\n"));

    ntStatus = Irp->IoStatus.Status;
    if(Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);

    }

    if(!NT_SUCCESS(ntStatus))
    {
        PoStartNextPowerIrp(Irp);

        FreeBT_DbgPrint(3, ("FBTUSB: FinishDevPoUpIrp::"));
        FreeBT_IoDecrement(DeviceExtension);

        return STATUS_SUCCESS;

    }

    SetDeviceFunctional(DeviceObject, Irp, DeviceExtension);

    FreeBT_DbgPrint(3, ("FBTUSB: FinishDevPoUpIrp: Leaving\n"));

    return STATUS_MORE_PROCESSING_REQUIRED;

}

NTSTATUS NTAPI SetDeviceFunctional(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension)
{
    KIRQL              oldIrql;
    NTSTATUS           ntStatus;
    POWER_STATE        newState;
    PIO_STACK_LOCATION irpStack;
    DEVICE_POWER_STATE newDevState, oldDevState;

    ntStatus = Irp->IoStatus.Status;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    newState = irpStack->Parameters.Power.State;
    newDevState = newState.DeviceState;
    oldDevState = DeviceExtension->DevPower;

    FreeBT_DbgPrint(3, ("FBTUSB: SetDeviceFunctional: Entered\n"));

    // update the cached state
    DeviceExtension->DevPower = newDevState;

    // restore appropriate amount of state to our h/w
    // this driver does not implement partial context
    // save/restore.
    PoSetPowerState(DeviceObject, DevicePowerState, newState);
    if(PowerDeviceD0 == newDevState)
    {
        KeAcquireSpinLock(&DeviceExtension->DevStateLock, &oldIrql);
        DeviceExtension->QueueState = AllowRequests;
        KeReleaseSpinLock(&DeviceExtension->DevStateLock, oldIrql);

        ProcessQueuedRequests(DeviceExtension);

    }

    PoStartNextPowerIrp(Irp);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    FreeBT_DbgPrint(3, ("FBTUSB: SetDeviceFunctional::"));
    FreeBT_IoDecrement(DeviceExtension);

    FreeBT_DbgPrint(3, ("FBTUSB: SetDeviceFunctional: Leaving\n"));

    return STATUS_SUCCESS;

}

NTSTATUS NTAPI FinishDevPoDnIrp(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS           ntStatus;
    POWER_STATE        newState;
    PIO_STACK_LOCATION irpStack;

    FreeBT_DbgPrint(3, ("FBTUSB: FinishDevPoDnIrp: Entered\n"));

    ntStatus = Irp->IoStatus.Status;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    newState = irpStack->Parameters.Power.State;

    if (NT_SUCCESS(ntStatus) && irpStack->MinorFunction == IRP_MN_SET_POWER)
    {
        FreeBT_DbgPrint(3, ("FBTUSB: updating cache..\n"));
        DeviceExtension->DevPower = newState.DeviceState;
        PoSetPowerState(DeviceObject, DevicePowerState, newState);

    }

    PoStartNextPowerIrp(Irp);

    FreeBT_DbgPrint(3, ("FBTUSB: FinishDevPoDnIrp::"));
    FreeBT_IoDecrement(DeviceExtension);

    FreeBT_DbgPrint(3, ("FBTUSB: FinishDevPoDnIrp: Leaving\n"));

    return STATUS_SUCCESS;

}

NTSTATUS NTAPI HoldIoRequests(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)

{
    NTSTATUS               ntStatus;
    PIO_WORKITEM           item;
    PDEVICE_EXTENSION      deviceExtension;
    PWORKER_THREAD_CONTEXT context;

    FreeBT_DbgPrint(3, ("FBTUSB: HoldIoRequests: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    deviceExtension->QueueState = HoldRequests;

    context = (PWORKER_THREAD_CONTEXT) ExAllocatePool(NonPagedPool, sizeof(WORKER_THREAD_CONTEXT));
    if(context)
    {
        item = IoAllocateWorkItem(DeviceObject);

        context->Irp = Irp;
        context->DeviceObject = DeviceObject;
        context->WorkItem = item;

        if (item)
        {
            IoMarkIrpPending(Irp);
            IoQueueWorkItem(item, HoldIoRequestsWorkerRoutine, DelayedWorkQueue, context);
            ntStatus = STATUS_PENDING;

        }

        else
        {
            FreeBT_DbgPrint(3, ("FBTUSB: HoldIoRequests: Failed to allocate memory for workitem\n"));
            ExFreePool(context);
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;

        }

    }

    else
    {
        FreeBT_DbgPrint(1, ("FBTUSB: HoldIoRequests: Failed to alloc memory for worker thread context\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;

    }

    FreeBT_DbgPrint(3, ("FBTUSB: HoldIoRequests: Leaving\n"));

    return ntStatus;

}

VOID NTAPI HoldIoRequestsWorkerRoutine(IN PDEVICE_OBJECT DeviceObject, IN PVOID Context)
{
    PIRP                   irp;
    NTSTATUS               ntStatus;
    PDEVICE_EXTENSION      deviceExtension;
    PWORKER_THREAD_CONTEXT context;

    FreeBT_DbgPrint(3, ("FBTUSB: HoldIoRequestsWorkerRoutine: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    context = (PWORKER_THREAD_CONTEXT) Context;
    irp = (PIRP) context->Irp;

    // wait for I/O in progress to finish.
    // the stop event is signalled when the counter drops to 1.
    // invoke FreeBT_IoDecrement twice: once each for the S-Irp and D-Irp.
    FreeBT_DbgPrint(3, ("FBTUSB: HoldIoRequestsWorkerRoutine::"));
    FreeBT_IoDecrement(deviceExtension);

    FreeBT_DbgPrint(3, ("FBTUSB: HoldIoRequestsWorkerRoutine::"));
    FreeBT_IoDecrement(deviceExtension);

    KeWaitForSingleObject(&deviceExtension->StopEvent, Executive, KernelMode, FALSE, NULL);

    // Increment twice to restore the count
    FreeBT_DbgPrint(3, ("FBTUSB: HoldIoRequestsWorkerRoutine::"));
    FreeBT_IoIncrement(deviceExtension);

    FreeBT_DbgPrint(3, ("FBTUSB: HoldIoRequestsWorkerRoutine::"));
    FreeBT_IoIncrement(deviceExtension);

    // now send the Irp down
    IoCopyCurrentIrpStackLocationToNext(irp);
    IoSetCompletionRoutine(
        irp,
        (PIO_COMPLETION_ROUTINE) FinishDevPoDnIrp,
        deviceExtension,
        TRUE,
        TRUE,
        TRUE);

    ntStatus = PoCallDriver(deviceExtension->TopOfStackDeviceObject, irp);
    if(!NT_SUCCESS(ntStatus))
    {
        FreeBT_DbgPrint(1, ("FBTUSB: HoldIoRequestsWorkerRoutine: Lower driver fail a power Irp\n"));

    }

    IoFreeWorkItem(context->WorkItem);
    ExFreePool((PVOID)context);

    FreeBT_DbgPrint(3, ("FBTUSB: HoldIoRequestsWorkerRoutine: Leaving\n"));

}

NTSTATUS NTAPI QueueRequest(IN OUT PDEVICE_EXTENSION DeviceExtension, IN PIRP Irp)
{
    KIRQL    oldIrql;
    NTSTATUS ntStatus;

    FreeBT_DbgPrint(3, ("FBTUSB: QueueRequests: Entered\n"));

    ntStatus = STATUS_PENDING;

    ASSERT(HoldRequests == DeviceExtension->QueueState);

    KeAcquireSpinLock(&DeviceExtension->QueueLock, &oldIrql);

    InsertTailList(&DeviceExtension->NewRequestsQueue, &Irp->Tail.Overlay.ListEntry);
    IoMarkIrpPending(Irp);
    IoSetCancelRoutine(Irp, CancelQueued);

    KeReleaseSpinLock(&DeviceExtension->QueueLock, oldIrql);

    FreeBT_DbgPrint(3, ("FBTUSB: QueueRequests: Leaving\n"));

    return ntStatus;

}

VOID NTAPI CancelQueued(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PDEVICE_EXTENSION deviceExtension;
    KIRQL             oldIrql;

    FreeBT_DbgPrint(3, ("FBTUSB: CancelQueued: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    oldIrql = Irp->CancelIrql;

    // Release the cancel spin lock
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    // Acquire the queue lock
    KeAcquireSpinLockAtDpcLevel(&deviceExtension->QueueLock);

    // Remove the cancelled Irp from queue and release the lock
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    KeReleaseSpinLock(&deviceExtension->QueueLock, oldIrql);

    // complete with STATUS_CANCELLED
    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    FreeBT_DbgPrint(3, ("FBTUSB: CancelQueued: Leaving\n"));

    return;

}

NTSTATUS NTAPI IssueWaitWake(IN PDEVICE_EXTENSION DeviceExtension)
{
    POWER_STATE poState;
    NTSTATUS    ntStatus;

    FreeBT_DbgPrint(3, ("FBTUSB: IssueWaitWake: Entered\n"));

    if(InterlockedExchange(&DeviceExtension->FlagWWOutstanding, 1))
    {
        return STATUS_DEVICE_BUSY;

    }

    InterlockedExchange(&DeviceExtension->FlagWWCancel, 0);

    // lowest state from which this Irp will wake the system
    poState.SystemState = DeviceExtension->DeviceCapabilities.SystemWake;
    ntStatus = PoRequestPowerIrp(DeviceExtension->PhysicalDeviceObject,
                                 IRP_MN_WAIT_WAKE,
                                 poState,
                                 (PREQUEST_POWER_COMPLETE) WaitWakeCallback,
                                 DeviceExtension,
                                 &DeviceExtension->WaitWakeIrp);

    if(!NT_SUCCESS(ntStatus))
    {
        InterlockedExchange(&DeviceExtension->FlagWWOutstanding, 0);

    }

    FreeBT_DbgPrint(3, ("FBTUSB: IssueWaitWake: Leaving\n"));

    return ntStatus;

}

VOID NTAPI CancelWaitWake(IN PDEVICE_EXTENSION DeviceExtension)
{
    PIRP Irp;

    FreeBT_DbgPrint(3, ("FBTUSB: CancelWaitWake: Entered\n"));

    Irp = (PIRP) InterlockedExchangePointer((PVOID*)&DeviceExtension->WaitWakeIrp,
                                            NULL);
    if(Irp)
    {
        IoCancelIrp(Irp);
        if(InterlockedExchange(&DeviceExtension->FlagWWCancel, 1))
        {
            PoStartNextPowerIrp(Irp);
            Irp->IoStatus.Status = STATUS_CANCELLED;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

        }

    }

    FreeBT_DbgPrint(3, ("FBTUSB: CancelWaitWake: Leaving\n"));

}

NTSTATUS NTAPI WaitWakeCompletionRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension)
{
    FreeBT_DbgPrint(3, ("FBTUSB: WaitWakeCompletionRoutine: Entered\n"));
    if(Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);

    }

    // Nullify the WaitWakeIrp pointer-the Irp is released
    // as part of the completion process. If it's already NULL,
    // avoid race with the CancelWaitWake routine.
    if(InterlockedExchangePointer((PVOID*)&DeviceExtension->WaitWakeIrp, NULL))
    {
        PoStartNextPowerIrp(Irp);

        return STATUS_SUCCESS;

    }

    // CancelWaitWake has run.
    // If FlagWWCancel != 0, complete the Irp.
    // If FlagWWCancel == 0, CancelWaitWake completes it.
    if(InterlockedExchange(&DeviceExtension->FlagWWCancel, 1))
    {
        PoStartNextPowerIrp(Irp);

        return STATUS_CANCELLED;

    }

    FreeBT_DbgPrint(3, ("FBTUSB: WaitWakeCompletionRoutine: Leaving\n"));

    return STATUS_MORE_PROCESSING_REQUIRED;

}

VOID NTAPI WaitWakeCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus)
{
    NTSTATUS               ntStatus;
    POWER_STATE            powerState;
    PDEVICE_EXTENSION      deviceExtension;

    FreeBT_DbgPrint(3, ("FBTUSB: WaitWakeCallback: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION) Context;

    InterlockedExchange(&deviceExtension->FlagWWOutstanding, 0);

    if(!NT_SUCCESS(IoStatus->Status))
    {
        return;

    }

    // wake up the device
    if(deviceExtension->DevPower == PowerDeviceD0)
    {
        FreeBT_DbgPrint(3, ("FBTUSB: WaitWakeCallback: Device already powered up...\n"));

        return;

    }

    FreeBT_DbgPrint(3, ("FBTUSB: WaitWakeCallback::"));
    FreeBT_IoIncrement(deviceExtension);

    powerState.DeviceState = PowerDeviceD0;
    ntStatus = PoRequestPowerIrp(deviceExtension->PhysicalDeviceObject,
                                 IRP_MN_SET_POWER,
                                 powerState,
                                 (PREQUEST_POWER_COMPLETE) WWIrpCompletionFunc,
                                 deviceExtension,
                                 NULL);

    if(deviceExtension->WaitWakeEnable)
    {
        IssueWaitWake(deviceExtension);

    }

    FreeBT_DbgPrint(3, ("FBTUSB: WaitWakeCallback: Leaving\n"));

    return;

}


PCHAR NTAPI PowerMinorFunctionString (IN UCHAR MinorFunction)
{
    switch (MinorFunction)
    {
        case IRP_MN_SET_POWER:
            return "IRP_MN_SET_POWER\n";

        case IRP_MN_QUERY_POWER:
            return "IRP_MN_QUERY_POWER\n";

        case IRP_MN_POWER_SEQUENCE:
            return "IRP_MN_POWER_SEQUENCE\n";

        case IRP_MN_WAIT_WAKE:
            return "IRP_MN_WAIT_WAKE\n";

        default:
            return "IRP_MN_?????\n";

    }

}
