/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power Manager control switches (lid, power buttons, etc) mechanisms
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

LIST_ENTRY PopControlSwitches;

/* PRIVATE FUNCTIONS **********************************************************/

static
PLIST_ENTRY
PopGetControlSwitchEntryFromList(
    _In_ PPOP_CONTROL_SWITCH TargetControlSwitch)
{
    PLIST_ENTRY Entry;
    PPOP_CONTROL_SWITCH ControlSwitch;

    /* Passing a NULL control switch is illegal */
    ASSERT(TargetControlSwitch);

    /* Iterate over the control switches and look for what we want */
    for (Entry = PopControlSwitches.Flink;
         Entry != &PopControlSwitches;
         Entry = Entry->Flink)
    {
        ControlSwitch = CONTAINING_RECORD(Entry, POP_CONTROL_SWITCH, Link);
        if (TargetControlSwitch == ControlSwitch)
        {
            return Entry;
        }
    }

    return NULL;
}

static
VOID
PopControlSwitchCleanup(
    _In_ PPOP_CONTROL_SWITCH ControlSwitch)
{
    PIRP Irp;
    PLIST_ENTRY Entry;
    PDEVICE_OBJECT DeviceObject;

    /*
     * Ensure the control is already in the global switches list, otherwise
     * something is seriously wrong. Remove it from the list.
     */
    Entry = PopGetControlSwitchEntryFromList(ControlSwitch);
    NT_ASSERT(Entry != NULL);
    RemoveEntryList(Entry);

    /* If this control switch was processing an IRP, free it  */
    Irp = ControlSwitch->Irp;
    if (Irp)
    {
        IoFreeIrp(Irp);
    }

    /* Dereference the device policy object */
    DeviceObject = ControlSwitch->DeviceObject;
    ObDereferenceObject(DeviceObject);

    /* Free the control switch */
    PopFreePool(ControlSwitch, TAG_PO_CONTROL_SWITCH);
}

static
NTSTATUS
NTAPI
PopControlSwitchIrpCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PVOID Context)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_MORE_PROCESSING_REQUIRED;
}

/* PUBLIC FUNCTIONS ***********************************************************/

NTSTATUS
NTAPI
PopCreateControlSwitch(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PPOP_CONTROL_SWITCH *ControlSwitch)
{
    PPOP_CONTROL_SWITCH LocalControlSwitch;

    PAGED_CODE();

    /* Allocate memory for the control switch we are going to create */
    LocalControlSwitch = PopAllocatePool(sizeof(POP_CONTROL_SWITCH),
                                         FALSE,
                                         TAG_PO_CONTROL_SWITCH);
    if (LocalControlSwitch == NULL)
    {
        DPRINT1("Failed to allocate pool of memory for the control switch\n");
        *ControlSwitch = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /*
     * Fill in the necessary datum. Albeit the control switch is pretty much created
     * at this point the control switch handler is responsible to initialize it with
     * real datum through communication between the power manager and the associated
     * device driver of which it handles the control switch.
     */
    InitializeListHead(&LocalControlSwitch->Link);
    LocalControlSwitch->Irp = NULL;
    LocalControlSwitch->Flags |= POP_CS_INITIALIZING;
    LocalControlSwitch->Mode = POP_CS_NO_MODE;
    LocalControlSwitch->DeviceObject = DeviceObject;
    LocalControlSwitch->SwitchType = SwitchNone;

    /* Insert the newly created control switch to the global switches list */
    InsertTailList(&PopControlSwitches, &LocalControlSwitch->Link);
    *ControlSwitch = LocalControlSwitch;
    return STATUS_SUCCESS;
}

VOID
NTAPI
PopSetButtonPowerAction(
    _Inout_ PPOWER_ACTION_POLICY Button,
    _In_ POWER_ACTION Action)
{
    PAGED_CODE();

    /* Punish the caller for bogus power actions */
    ASSERT((Action >= PowerActionNone) && (Action <= PowerActionDisplayOff));

    /* Setup the actions for this button */
    Button->Action = Action;
    Button->Flags = 0;
    Button->EventCode = 0;
}

_Use_decl_annotations_
VOID
NTAPI
PopControlSwitchHandler(
    _In_ PVOID Parameter)
{
    ULONG IoControlCode;
    PIRP Irp;
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_OBJECT DeviceObject;
    PPOP_CONTROL_SWITCH ControlSwitch;
    PPOP_DEVICE_POLICY_WORKITEM_DATA WorkItemData = (PPOP_DEVICE_POLICY_WORKITEM_DATA)Parameter;

    PAGED_CODE();

    /*
     * Ensure we got the right policy device as this handler only processes
     * control switches and not anything else.
     */
    ASSERT(WorkItemData->PolicyType == PolicyDeviceSystemButton);
    ControlSwitch = WorkItemData->PolicyData;

    /*
     * This control switch asked for cleanup. This could be caused by the ACPI
     * driver disabling that switch or we got an unexpected error. Tear the
     * switch apart.
     */
    if (ControlSwitch->Flags & POP_CS_CLEANUP)
    {
        PopControlSwitchCleanup(ControlSwitch);
        PopFreePool(WorkItemData, TAG_PO_POLICY_DEVICE_WORKITEM_DATA);
        return;
    }

    /*
     * Ensure this control switch does not have an outstanding IRP that still needs
     * to be completed. The CS IRP completion handler is responsible to free it once
     * it has got data from the ACPI driver of which the IRP is no longer needed.
     */
    ASSERT(ControlSwitch->Irp == NULL);

    /* Allocate a new fresh IRP to satisfy the required request */
    DeviceObject = ControlSwitch->DeviceObject;
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        /*
         * Failing this request on our end is fatal. Most likely the I/O manager
         * tried so hard to allocate an IRP for us it failed on doing so, due to
         * a serious low memory condition. Not something that we can do to salvage
         * the system so kill it.
         */
        KeBugCheckEx(INTERNAL_POWER_ERROR,
                     0,
                     POP_DEVICE_POLICY_IRP_ALLOC_FAILED,
                     PolicyDeviceSystemButton,
                     (ULONG_PTR)DeviceObject);
    }

    /*
     * This is a newly fresh created control switch of which we do not know what are
     * its true capabilities as of yet. Insert it into the global list of control switches
     * and set a query capabilities command mode so that on IRP completion the Power Manager
     * knows what data it looked for.
     */
    if (ControlSwitch->Flags & POP_CS_INITIALIZING)
    {
        ControlSwitch->Mode = POP_CS_QUERY_CAPS_MODE;
        IoControlCode = IOCTL_GET_SYS_BUTTON_CAPS;
        ControlSwitch->Flags &= ~POP_CS_INITIALIZING;
        goto DispatchIrp;
    }

    /*
     * We already know the capabilities of this control switch. Query a button event
     * from this switch and wait on it until the event gets triggered. The IRP completion
     * routine will handle it.
     */
    ControlSwitch->Mode = POP_CS_QUERY_EVENT_MODE;
    IoControlCode = IOCTL_GET_SYS_BUTTON_EVENT;

DispatchIrp:
    /* Setup the IRP stack parameters based on the requested operation */
    IrpStack = IoGetNextIrpStackLocation(Irp);
    IrpStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    IrpStack->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
    IrpStack->Parameters.DeviceIoControl.InputBufferLength = sizeof(ULONG);
    IrpStack->Parameters.DeviceIoControl.OutputBufferLength = sizeof(ULONG);

    /*
     * Register the IRP completion CS handler and give the IRP to the control switch,
     * so that when the ACPI driver returns back our IRP the completion handler will
     * further process the CS operation event.
     */
    ControlSwitch->Irp = Irp;
    IoSetCompletionRoutine(Irp,
                           PopControlSwitchIrpCompletion,
                           ControlSwitch,
                           TRUE,
                           TRUE,
                           TRUE);

    /* Finally dispatch the IRP to the ACPI driver */
    IoCallDriver(DeviceObject, Irp);
    PopFreePool(WorkItemData, TAG_PO_POLICY_DEVICE_WORKITEM_DATA);
}

/* EOF */
