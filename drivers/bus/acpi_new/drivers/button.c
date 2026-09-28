/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     SYS_BUTTON interface for the power, sleep and lid buttons
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <poclass.h>
#include <ntpoapi.h>
#include <uacpi/tables.h>
#include <uacpi/acpi.h>
#include <debug.h>

/* Lid reports as the power service expects them: changed bit plus open or closed */
#define UACPINT_LID_EVENT_OPEN      (SYS_BUTTON_LID_CHANGED | SYS_BUTTON_WAKE)
#define UACPINT_LID_EVENT_CLOSED    (SYS_BUTTON_LID_CHANGED | SYS_BUTTON_LID)

#define UACPINT_NOTIFY_BUTTON       0x80
#define UACPINT_NOTIFY_WAKE         0x02

/* The fixed button PDO is created once, a restarted FDO must not repeat it */
static BOOLEAN UacpiNtFixedButtonCreated;

BOOLEAN
NTAPI
UacpiNtButtonClassify(
    _Inout_ PUACPINT_PDO Pdo)
{
    /* Power and sleep buttons wake the system, the lid does not */
    if (_stricmp(Pdo->Hid, "PNP0C0C") == 0)
        Pdo->ButtonCaps = SYS_BUTTON_WAKE | SYS_BUTTON_POWER;
    else if (_stricmp(Pdo->Hid, "PNP0C0E") == 0)
        Pdo->ButtonCaps = SYS_BUTTON_WAKE | SYS_BUTTON_SLEEP;
    else if (_stricmp(Pdo->Hid, "PNP0C0D") == 0)
        Pdo->ButtonCaps = SYS_BUTTON_LID;
    else
        Pdo->ButtonCaps = 0;

    if (!Pdo->ButtonCaps)
        return FALSE;

    KeInitializeSpinLock(&Pdo->ButtonLock);
    InitializeListHead(&Pdo->ButtonIrpQueue);
    return TRUE;
}

/* \Callback\PowerState: remember whether closing the lid does anything */
static
VOID
NTAPI
UacpiNtLidPowerStateCallback(
    _In_opt_ PVOID CallbackContext,
    _In_opt_ PVOID Argument1,
    _In_opt_ PVOID Argument2)
{
    PUACPINT_PDO Pdo = CallbackContext;
    SYSTEM_POWER_POLICY Policy;
    POWER_ACTION Action;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Argument2);

    /* Only the policy change notification (code 0) is of interest */
    if (Argument1 || !Pdo)
        return;

    RtlZeroMemory(&Policy, sizeof(Policy));
    Status = ZwPowerInformation(SystemPowerPolicyCurrent, NULL, 0, &Policy, sizeof(Policy));
    if (!NT_SUCCESS(Status))
        return;

    Action = Policy.LidClose.Action;
    Pdo->LidCloseNoAction = (Action == PowerActionNone || Action == PowerActionReserved);
}

static
VOID
NTAPI
UacpiNtLidHookPolicy(
    _Inout_ PUACPINT_PDO Pdo)
{
    UNICODE_STRING CallbackName = RTL_CONSTANT_STRING(L"\\Callback\\PowerState");
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;

    InitializeObjectAttributes(&ObjectAttributes,
                               &CallbackName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ExCreateCallback(&Pdo->LidPowerCallback, &ObjectAttributes, FALSE, TRUE);
    if (!NT_SUCCESS(Status))
        return;

    Pdo->LidPowerCallbackHandle = ExRegisterCallback(Pdo->LidPowerCallback,
                                                     UacpiNtLidPowerStateCallback,
                                                     Pdo);

    /* Pick up the policy in effect right now */
    UacpiNtLidPowerStateCallback(Pdo, NULL, NULL);
}

VOID
NTAPI
UacpiNtButtonStart(
    _Inout_ PUACPINT_PDO Pdo)
{
    NTSTATUS Status;

    if (!Pdo->ButtonCaps || Pdo->ButtonIfRegistered)
        return;

    Status = IoRegisterDeviceInterface(Pdo->Shared.Self,
                                       &GUID_DEVICE_SYS_BUTTON,
                                       NULL,
                                       &Pdo->ButtonSymLink);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: SYS_BUTTON interface for %s failed 0x%lx\n", Pdo->Name, Status);
        return;
    }

    Pdo->ButtonIfRegistered = TRUE;
    IoSetDeviceInterfaceState(&Pdo->ButtonSymLink, TRUE);
    DPRINT("uACPI-NT: %s SYS_BUTTON up, caps 0x%lx\n", Pdo->Name, Pdo->ButtonCaps);

    /* The lid tracks the close policy and reports its state right away */
    if (Pdo->ButtonCaps & SYS_BUTTON_LID)
    {
        UacpiNtLidHookPolicy(Pdo);
        UacpiNtButtonNotify(Pdo, UACPINT_NOTIFY_BUTTON);
    }

    UacpiNtButtonArmWaitWake(Pdo);
}

static
VOID
NTAPI
UacpiNtButtonWaitWakeDone(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ UCHAR MinorFunction,
    _In_ POWER_STATE PowerState,
    _In_opt_ PVOID Context,
    _In_ PIO_STATUS_BLOCK IoStatus)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(MinorFunction);
    UNREFERENCED_PARAMETER(PowerState);

    /* Rearming after a failure would spin on a hard error */
    if (!IoStatus || !NT_SUCCESS(IoStatus->Status) || !Context)
        return;

    /* The event itself comes in through Notify(2) */
    UacpiNtButtonArmWaitWake(Context);
}

VOID
NTAPI
UacpiNtButtonArmWaitWake(
    _In_ PUACPINT_PDO Pdo)
{
    POWER_STATE WakeState;

    if (!Pdo->ButtonCaps)
        return;

    /* A lid whose close does nothing is no wake source */
    if ((Pdo->ButtonCaps & SYS_BUTTON_LID) && Pdo->LidCloseNoAction)
        return;

    /*
     * Control method buttons wake from their _PRW state and are skipped when
     * _PRW gives none. The fixed button has no node and wakes through PM1.
     */
    WakeState.SystemState = PowerSystemSleeping3;
    if (Pdo->Wake.Node && UacpiNtWakeHasPrw(&Pdo->Wake))
    {
        if (Pdo->Wake.SystemWake == PowerSystemUnspecified)
            return;

        WakeState.SystemState = Pdo->Wake.SystemWake;
    }

    PoRequestPowerIrp(Pdo->Shared.Self,
                      IRP_MN_WAIT_WAKE,
                      WakeState,
                      UacpiNtButtonWaitWakeDone,
                      Pdo,
                      NULL);
}

static
VOID
NTAPI
UacpiNtButtonCancel(
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PUACPINT_PDO Pdo = DeviceObject->DeviceExtension;
    KIRQL OldIrql;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    KeAcquireSpinLock(&Pdo->ButtonLock, &OldIrql);
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    KeReleaseSpinLock(&Pdo->ButtonLock, OldIrql);

    UacpiNtCompleteIrp(Irp, STATUS_CANCELLED, 0);
}

static
NTSTATUS
NTAPI
UacpiNtButtonWaitForEvent(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp)
{
    PULONG Output = Irp->AssociatedIrp.SystemBuffer;
    ULONG Events;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Pdo->ButtonLock, &OldIrql);

    /* A latched event is handed out immediately */
    Events = Pdo->ButtonEvents;
    if (Events)
    {
        Pdo->ButtonEvents = 0;
        KeReleaseSpinLock(&Pdo->ButtonLock, OldIrql);

        *Output = Events;
        return UacpiNtCompleteIrp(Irp, STATUS_SUCCESS, sizeof(*Output));
    }

    IoSetCancelRoutine(Irp, UacpiNtButtonCancel);
    if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL))
    {
        KeReleaseSpinLock(&Pdo->ButtonLock, OldIrql);
        return UacpiNtCompleteIrp(Irp, STATUS_CANCELLED, 0);
    }

    IoMarkIrpPending(Irp);
    InsertTailList(&Pdo->ButtonIrpQueue, &Irp->Tail.Overlay.ListEntry);
    KeReleaseSpinLock(&Pdo->ButtonLock, OldIrql);
    return STATUS_PENDING;
}

/* Handled comes back FALSE for anything that is not a SYS_BUTTON request */
NTSTATUS
NTAPI
UacpiNtButtonDeviceControl(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp,
    _Out_ PBOOLEAN Handled)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG IoControlCode = IoStack->Parameters.DeviceIoControl.IoControlCode;
    ULONG OutputLength = IoStack->Parameters.DeviceIoControl.OutputBufferLength;
    PULONG Output = Irp->AssociatedIrp.SystemBuffer;

    *Handled = FALSE;

    if (!Pdo->ButtonCaps)
        return STATUS_NOT_SUPPORTED;

    if (IoControlCode != IOCTL_GET_SYS_BUTTON_CAPS &&
        IoControlCode != IOCTL_GET_SYS_BUTTON_EVENT)
    {
        return STATUS_NOT_SUPPORTED;
    }

    *Handled = TRUE;

    /* Only kernel mode talks SYS_BUTTON */
    if (Irp->RequestorMode != KernelMode)
        return UacpiNtCompleteIrp(Irp, STATUS_NOT_IMPLEMENTED, 0);

    if (OutputLength < sizeof(*Output) || !Output)
        return UacpiNtCompleteIrp(Irp, STATUS_BUFFER_TOO_SMALL, 0);

    if (IoControlCode == IOCTL_GET_SYS_BUTTON_EVENT)
        return UacpiNtButtonWaitForEvent(Pdo, Irp);

    *Output = Pdo->ButtonCaps;
    return UacpiNtCompleteIrp(Irp, STATUS_SUCCESS, sizeof(*Output));
}

/* Completes a waiting event IRP or latches the event, no AML so any IRQL up to DISPATCH */
VOID
NTAPI
UacpiNtButtonDeliverEvent(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ ULONG Event)
{
    PLIST_ENTRY Entry;
    PIRP Waiter = NULL;
    PIRP Head;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Pdo->ButtonLock, &OldIrql);

    if (!IsListEmpty(&Pdo->ButtonIrpQueue))
    {
        Entry = Pdo->ButtonIrpQueue.Flink;
        Head = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);

        /* Losing the race to the cancel routine means we latch instead */
        if (IoSetCancelRoutine(Head, NULL))
        {
            RemoveEntryList(Entry);
            Waiter = Head;
        }
    }

    if (!Waiter)
        Pdo->ButtonEvents |= Event;

    KeReleaseSpinLock(&Pdo->ButtonLock, OldIrql);

    if (!Waiter)
    {
        DPRINT("uACPI-NT: %s latched button event 0x%lx\n", Pdo->Name, Event);
        return;
    }

    *(PULONG)Waiter->AssociatedIrp.SystemBuffer = Event;
    UacpiNtCompleteIrp(Waiter, STATUS_SUCCESS, sizeof(Event));
    DPRINT("uACPI-NT: %s delivered button event 0x%lx\n", Pdo->Name, Event);
}

static
ULONG
NTAPI
UacpiNtLidReadEvent(
    _Inout_ PUACPINT_PDO Pdo)
{
    uacpi_u64 LidOpen = 1;
    ULONG Event;

    if (Pdo->Node)
        uacpi_eval_simple_integer(Pdo->Node, "_LID", &LidOpen);

    Event = LidOpen ? UACPINT_LID_EVENT_OPEN : UACPINT_LID_EVENT_CLOSED;

    /* The first report after start is flagged as the initial state */
    if (!Pdo->LidInitialReported)
    {
        Pdo->LidInitialReported = TRUE;
        Event |= SYS_BUTTON_LID_INITIAL;
    }

    return Event;
}

VOID
NTAPI
UacpiNtButtonNotify(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ ULONG NotifyValue)
{
    ULONG Event;

    if (!Pdo->ButtonCaps)
        return;

    switch (NotifyValue)
    {
        case UACPINT_NOTIFY_WAKE:
            Event = SYS_BUTTON_WAKE;
            break;

        case UACPINT_NOTIFY_BUTTON:
            if (Pdo->ButtonCaps & SYS_BUTTON_LID)
                Event = UacpiNtLidReadEvent(Pdo);
            else
                Event = Pdo->ButtonCaps & ~SYS_BUTTON_WAKE;
            break;

        default:
            return;
    }

    UacpiNtButtonDeliverEvent(Pdo, Event);
}

/* Fixed events are latched in the SCI handler and delivered from here */
static
VOID
NTAPI
UacpiNtFixedButtonDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PUACPINT_PDO Pdo = DeferredContext;
    ULONG Pending;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (!Pdo)
        return;

    Pending = (ULONG)InterlockedExchange(&Pdo->ButtonDeferred, 0);
    if (!Pending)
        return;

    /*
     * A press inside the resume window is what woke us. Reporting it as POWER
     * would let the policy put the machine straight back to sleep.
     */
    if (InterlockedCompareExchange(&UacpiNtSystemResuming, 0, 0))
    {
        UacpiNtButtonDeliverEvent(Pdo, SYS_BUTTON_WAKE);
        return;
    }

    if (Pending & SYS_BUTTON_POWER)
        UacpiNtButtonDeliverEvent(Pdo, SYS_BUTTON_POWER);

    if (Pending & SYS_BUTTON_SLEEP)
        UacpiNtButtonDeliverEvent(Pdo, SYS_BUTTON_SLEEP);
}

static
VOID
NTAPI
UacpiNtFixedButtonDefer(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ ULONG Event)
{
    InterlockedOr(&Pdo->ButtonDeferred, (LONG)Event);
    KeInsertQueueDpc(&Pdo->ButtonDpc, NULL, NULL);
}

static
uacpi_interrupt_ret
UacpiNtFixedPowerButton(
    _In_ uacpi_handle Context)
{
    UacpiNtFixedButtonDefer(Context, SYS_BUTTON_POWER);
    return UACPI_INTERRUPT_HANDLED;
}

static
uacpi_interrupt_ret
UacpiNtFixedSleepButton(
    _In_ uacpi_handle Context)
{
    UacpiNtFixedButtonDefer(Context, SYS_BUTTON_SLEEP);
    return UACPI_INTERRUPT_HANDLED;
}

/* A clear control method flag in the FADT means a fixed feature button */
static
ULONG
NTAPI
UacpiNtFixedButtonCaps(
    _In_ struct acpi_fadt *Fadt)
{
    ULONG Caps = 0;

    if (!(Fadt->flags & ACPI_PWR_BUTTON))
        Caps |= SYS_BUTTON_POWER;

    if (!(Fadt->flags & ACPI_SLP_BUTTON))
        Caps |= SYS_BUTTON_SLEEP;

    return Caps;
}

/* Fixed power and sleep buttons share one node-less ACPI\FixedButton PDO */
VOID
NTAPI
UacpiNtFixedButtonInit(
    _In_ PUACPINT_FDO Fdo)
{
    PDEVICE_OBJECT DeviceObject;
    struct acpi_fadt *Fadt = NULL;
    PUACPINT_PDO Pdo;
    NTSTATUS Status;
    ULONG Caps;

    if (UacpiNtFixedButtonCreated)
        return;

    /* Without a FADT nothing is decided yet, a later start tries again */
    if (uacpi_unlikely_error(uacpi_table_fadt(&Fadt)) || !Fadt)
        return;

    UacpiNtFixedButtonCreated = TRUE;

    Caps = UacpiNtFixedButtonCaps(Fadt);
    if (!Caps)
        return;

    Status = IoCreateDevice(Fdo->Shared.Self->DriverObject,
                            sizeof(*Pdo),
                            NULL,
                            FILE_DEVICE_ACPI,
                            FILE_AUTOGENERATED_DEVICE_NAME | FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: fixed button PDO creation failed 0x%lx\n", Status);
        return;
    }

    Pdo = DeviceObject->DeviceExtension;
    RtlZeroMemory(Pdo, sizeof(*Pdo));
    Pdo->Shared.Type = UacpiNtDevObjPdo;
    Pdo->Shared.Self = DeviceObject;
    Pdo->Parent = Fdo;
    Pdo->Node = NULL;
    Pdo->ScopeNode = uacpi_namespace_get_predefined(UACPI_PREDEFINED_NAMESPACE_SB);
    Pdo->Present = TRUE;
    Pdo->ButtonCaps = Caps | SYS_BUTTON_WAKE;

    /* Fixed buttons wake through PM1, not a GPE */
    UacpiNtWakeInit(&Pdo->Wake, NULL);

    KeInitializeSpinLock(&Pdo->ButtonLock);
    InitializeListHead(&Pdo->ButtonIrpQueue);
    KeInitializeDpc(&Pdo->ButtonDpc, UacpiNtFixedButtonDpc, Pdo);

    /* Reported as ACPI\FixedButton */
    RtlStringCbCopyA(Pdo->Name, sizeof(Pdo->Name), "FXBT");
    RtlStringCbCopyA(Pdo->Hid, sizeof(Pdo->Hid), "FixedButton");
    RtlStringCbCopyA(Pdo->Instance, sizeof(Pdo->Instance), "0");

    DeviceObject->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    ExAcquireFastMutex(&Fdo->ChildLock);
    InsertTailList(&Fdo->ChildList, &Pdo->Link);
    ExReleaseFastMutex(&Fdo->ChildLock);

    if (Caps & SYS_BUTTON_POWER)
    {
        uacpi_install_fixed_event_handler(UACPI_FIXED_EVENT_POWER_BUTTON,
                                          UacpiNtFixedPowerButton,
                                          Pdo);
    }

    if (Caps & SYS_BUTTON_SLEEP)
    {
        uacpi_install_fixed_event_handler(UACPI_FIXED_EVENT_SLEEP_BUTTON,
                                          UacpiNtFixedSleepButton,
                                          Pdo);
    }

    DPRINT("uACPI-NT: fixed button PDO up, caps 0x%lx\n", Pdo->ButtonCaps);
}

VOID
NTAPI
UacpiNtButtonRemove(
    _Inout_ PUACPINT_PDO Pdo)
{
    LIST_ENTRY Orphans;
    PLIST_ENTRY Entry;
    PIRP Irp;
    KIRQL OldIrql;

    if (!Pdo->ButtonCaps)
        return;

    if (Pdo->ButtonIfRegistered)
    {
        IoSetDeviceInterfaceState(&Pdo->ButtonSymLink, FALSE);
        RtlFreeUnicodeString(&Pdo->ButtonSymLink);
        Pdo->ButtonIfRegistered = FALSE;
    }

    if (Pdo->LidPowerCallbackHandle)
    {
        ExUnregisterCallback(Pdo->LidPowerCallbackHandle);
        Pdo->LidPowerCallbackHandle = NULL;
    }

    if (Pdo->LidPowerCallback)
    {
        ObDereferenceObject(Pdo->LidPowerCallback);
        Pdo->LidPowerCallback = NULL;
    }

    InitializeListHead(&Orphans);

    KeAcquireSpinLock(&Pdo->ButtonLock, &OldIrql);
    while (!IsListEmpty(&Pdo->ButtonIrpQueue))
    {
        Entry = RemoveHeadList(&Pdo->ButtonIrpQueue);
        Irp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);

        /* A running cancel routine unlinks it again, so leave it self linked */
        if (IoSetCancelRoutine(Irp, NULL))
            InsertTailList(&Orphans, Entry);
        else
            InitializeListHead(Entry);
    }
    KeReleaseSpinLock(&Pdo->ButtonLock, OldIrql);

    while (!IsListEmpty(&Orphans))
    {
        Entry = RemoveHeadList(&Orphans);
        Irp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);
        UacpiNtCompleteIrp(Irp, STATUS_DELETE_PENDING, 0);
    }
}
