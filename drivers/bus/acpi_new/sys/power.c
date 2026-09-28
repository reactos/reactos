/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device power states, S to D capability map and wake arming
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <debug.h>

/* Most wake blocks one sleep or resume pass handles */
#define UACPINT_WAKE_SNAPSHOT_MAX 64

BOOLEAN
NTAPI
UacpiNtNodeHasChild(
    _In_ uacpi_namespace_node *Node,
    _In_z_ const char *Name)
{
    uacpi_namespace_node *Child = NULL;

    if (!Node)
        return FALSE;

    if (uacpi_unlikely_error(uacpi_namespace_node_find(Node, Name, &Child)))
        return FALSE;

    return (Child != NULL);
}

static
BOOLEAN
NTAPI
UacpiNtPowerEvalMethod(
    _In_ uacpi_namespace_node *Node,
    _In_z_ const char *Name)
{
    uacpi_status UacpiStatus;

    UacpiStatus = uacpi_eval(Node, Name, NULL, NULL);
    if (uacpi_likely_success(UacpiStatus))
        return TRUE;

    if (UacpiStatus != UACPI_STATUS_NOT_FOUND)
        DPRINT1("uACPI-NT: %s failed: %s\n", Name, uacpi_status_to_string(UacpiStatus));

    return FALSE;
}

/* S4 and S5 always exist, S1 to S3 only when the \_Sx_ package does */
static
BOOLEAN
NTAPI
UacpiNtSleepStateSupported(
    _In_ ULONG SleepState)
{
    char Name[5] = { '_', 'S', '0', '_', '\0' };

    if (SleepState >= 4)
        return TRUE;

    Name[2] = (char)('0' + SleepState);
    return UacpiNtNodeHasChild(uacpi_namespace_root(), Name);
}

static
BOOLEAN
NTAPI
UacpiNtQuerySleepDeviceState(
    _In_ uacpi_namespace_node *Node,
    _In_ ULONG SleepState,
    _Out_ PDEVICE_POWER_STATE DeviceState)
{
    char Name[5] = { '_', 'S', '0', 'D', '\0' };
    uacpi_u64 Value = 0;

    Name[2] = (char)('0' + SleepState);

    if (uacpi_unlikely_error(uacpi_eval_simple_integer(Node, Name, &Value)) || Value > 3)
    {
        *DeviceState = PowerDeviceUnspecified;
        return FALSE;
    }

    *DeviceState = (DEVICE_POWER_STATE)(PowerDeviceD0 + (ULONG)Value);
    return TRUE;
}

/* The wake D state: _SxD first, then the mapped state if valid, else D3 */
static
DEVICE_POWER_STATE
NTAPI
UacpiNtWakeDeviceState(
    _In_ uacpi_namespace_node *Node,
    _In_ ULONG SleepState,
    _In_opt_ PDEVICE_CAPABILITIES Capabilities)
{
    DEVICE_POWER_STATE DeviceState;
    SYSTEM_POWER_STATE SystemState = (SYSTEM_POWER_STATE)(PowerSystemWorking + SleepState);

    if (UacpiNtQuerySleepDeviceState(Node, SleepState, &DeviceState))
        return DeviceState;

    if (Capabilities &&
        Capabilities->DeviceState[SystemState] >= PowerDeviceD0 &&
        Capabilities->DeviceState[SystemState] <= PowerDeviceD3)
    {
        return Capabilities->DeviceState[SystemState];
    }

    return PowerDeviceD3;
}

/* _PRW as a package of at least two elements, NULL otherwise. Caller unrefs. */
static
uacpi_object *
NTAPI
UacpiNtEvalPrw(
    _In_ uacpi_namespace_node *Node,
    _Out_ uacpi_object_array *Package)
{
    uacpi_object *Object = NULL;

    Package->objects = NULL;
    Package->count = 0;

    if (uacpi_unlikely_error(uacpi_eval(Node, "_PRW", NULL, &Object)) || !Object)
        return NULL;

    if (uacpi_object_get_type(Object) != UACPI_OBJECT_PACKAGE ||
        uacpi_unlikely_error(uacpi_object_get_package(Object, Package)) ||
        Package->count < 2)
    {
        uacpi_object_unref(Object);
        return NULL;
    }

    return Object;
}

/* _PRW[1] as a sleep state number, 0 when out of range */
static
ULONG
NTAPI
UacpiNtPrwSleepState(
    _In_ uacpi_object_array *Package)
{
    uacpi_u64 Deepest = 0;

    if (uacpi_unlikely_error(uacpi_object_get_integer(Package->objects[1], &Deepest)))
        return 0;

    if (Deepest < 1 || Deepest > 5)
        return 0;

    return (ULONG)Deepest;
}

NTSTATUS
NTAPI
UacpiNtPowerSetDeviceState(
    _In_ PUACPINT_PDO Pdo,
    _In_ DEVICE_POWER_STATE DeviceState)
{
    static const char *MethodNames[] = { "_PS0", "_PS1", "_PS2", "_PS3" };
    PUACPINT_POWER_RESOURCE NewList[UACPINT_MAX_POWER_RESOURCES];
    ULONG NewCount = 0;
    ULONG Index;

    if (!Pdo->Node)
        return STATUS_SUCCESS;

    if (DeviceState < PowerDeviceD0 || DeviceState > PowerDeviceD3)
        return STATUS_SUCCESS;

    Index = DeviceState - PowerDeviceD0;

    /* Resources for the new state go on before _PSx, unused ones go off after */
    UacpiNtPowerResAcquireForState(Pdo, DeviceState, NewList, &NewCount);
    UacpiNtPowerEvalMethod(Pdo->Node, MethodNames[Index]);
    UacpiNtPowerResReleaseDelta(Pdo, NewList, NewCount);

    Pdo->CurrentDState = DeviceState;
    Pdo->DStateKnown = TRUE;

    DPRINT("uACPI-NT: %s now in D%lu\n", Pdo->Name, Index);
    return STATUS_SUCCESS;
}

static
VOID
NTAPI
UacpiNtCopyPowerCaps(
    _Out_ PDEVICE_CAPABILITIES Destination,
    _In_ PDEVICE_CAPABILITIES Source)
{
    RtlCopyMemory(Destination->DeviceState,
                  Source->DeviceState,
                  sizeof(Destination->DeviceState));

    Destination->DeviceWake = Source->DeviceWake;
    Destination->SystemWake = Source->SystemWake;
    Destination->DeviceD1 = Source->DeviceD1;
    Destination->DeviceD2 = Source->DeviceD2;
    Destination->WakeFromD0 = Source->WakeFromD0;
    Destination->WakeFromD1 = Source->WakeFromD1;
    Destination->WakeFromD2 = Source->WakeFromD2;
    Destination->WakeFromD3 = Source->WakeFromD3;
}

/* _PRW gives SystemWake, _SxW is not consulted */
static
VOID
NTAPI
UacpiNtPowerMapWake(
    _In_ PUACPINT_PDO Pdo,
    _Inout_ PDEVICE_CAPABILITIES Capabilities)
{
    uacpi_object_array Package;
    uacpi_object *Object;
    DEVICE_POWER_STATE DeviceWake;
    ULONG SleepState;

    Object = UacpiNtEvalPrw(Pdo->Node, &Package);
    if (!Object)
        return;

    SleepState = UacpiNtPrwSleepState(&Package);
    if (SleepState != 0)
    {
        if (UacpiNtSleepStateSupported(SleepState))
        {
            Capabilities->SystemWake = (SYSTEM_POWER_STATE)(PowerSystemWorking + SleepState);

            DeviceWake = UacpiNtWakeDeviceState(Pdo->Node, SleepState, Capabilities);
            Capabilities->DeviceWake = DeviceWake;

            /* Only the bit for DeviceWake itself, like acpi.sys */
            Capabilities->WakeFromD0 = (DeviceWake == PowerDeviceD0);
            Capabilities->WakeFromD1 = (DeviceWake == PowerDeviceD1);
            Capabilities->WakeFromD2 = (DeviceWake == PowerDeviceD2);
            Capabilities->WakeFromD3 = (DeviceWake == PowerDeviceD3);

            DPRINT("uACPI-NT: %s wakes from S%lu in D%lu\n",
                   Pdo->Name,
                   SleepState,
                   (ULONG)(DeviceWake - PowerDeviceD0));
        }
        else
        {
            DPRINT("uACPI-NT: %s _PRW wake state S%lu is not supported\n",
                   Pdo->Name,
                   SleepState);
        }
    }

    uacpi_object_unref(Object);
}

VOID
NTAPI
UacpiNtPowerBuildStateMap(
    _In_ PUACPINT_PDO Pdo,
    _Inout_ PDEVICE_CAPABILITIES Capabilities)
{
    DEVICE_POWER_STATE DeviceState;
    BOOLEAN HasD1;
    BOOLEAN HasD2;
    ULONG SystemState;

    if (Pdo->CapsMapDone)
    {
        UacpiNtCopyPowerCaps(Capabilities, &Pdo->CapsMap);
        return;
    }

    for (SystemState = 0; SystemState < PowerSystemMaximum; SystemState++)
    {
        Capabilities->DeviceState[SystemState] =
            (SystemState == PowerSystemWorking) ? PowerDeviceD0 : PowerDeviceD3;
    }

    Capabilities->DeviceWake = PowerDeviceUnspecified;
    Capabilities->SystemWake = PowerSystemUnspecified;

    if (!Pdo->Node)
        return;

    /* D0 and D3 always exist, D1 and D2 need a _PSx or _PRx */
    HasD1 = UacpiNtNodeHasChild(Pdo->Node, "_PS1") || UacpiNtNodeHasChild(Pdo->Node, "_PR1");
    HasD2 = UacpiNtNodeHasChild(Pdo->Node, "_PS2") || UacpiNtNodeHasChild(Pdo->Node, "_PR2");
    Capabilities->DeviceD1 = HasD1;
    Capabilities->DeviceD2 = HasD2;

    /* _S1D to _S4D, moved deeper onto a state the device has */
    for (SystemState = PowerSystemSleeping1; SystemState <= PowerSystemHibernate; SystemState++)
    {
        if (!UacpiNtQuerySleepDeviceState(Pdo->Node, SystemState - PowerSystemWorking, &DeviceState))
            continue;

        if (DeviceState == PowerDeviceD1 && !HasD1)
            DeviceState = HasD2 ? PowerDeviceD2 : PowerDeviceD3;

        if (DeviceState == PowerDeviceD2 && !HasD2)
            DeviceState = PowerDeviceD3;

        Capabilities->DeviceState[SystemState] = DeviceState;
    }

    UacpiNtPowerMapWake(Pdo, Capabilities);

    /* The AML answers do not change for the life of the PDO */
    UacpiNtCopyPowerCaps(&Pdo->CapsMap, Capabilities);
    Pdo->CapsMapDone = TRUE;
}

/*
 * Filtered foreign PDOs: the bus driver filled the D states and knows
 * nothing about _PRW, so only the wake fields are merged in.
 */
VOID
NTAPI
UacpiNtPowerMergeWakeCaps(
    _In_ uacpi_namespace_node *Node,
    _Inout_ PDEVICE_CAPABILITIES Capabilities)
{
    uacpi_object_array Package;
    uacpi_object *Object;
    SYSTEM_POWER_STATE SystemWake;
    DEVICE_POWER_STATE DeviceWake;
    ULONG SleepState;

    if (!Node || !Capabilities)
        return;

    Object = UacpiNtEvalPrw(Node, &Package);
    if (!Object)
        return;

    SleepState = UacpiNtPrwSleepState(&Package);
    if (SleepState != 0 && UacpiNtSleepStateSupported(SleepState))
    {
        SystemWake = (SYSTEM_POWER_STATE)(PowerSystemWorking + SleepState);

        /* Both the bus and the GPE limit how deep the device can wake from */
        if (Capabilities->SystemWake == PowerSystemUnspecified ||
            Capabilities->SystemWake > SystemWake)
        {
            Capabilities->SystemWake = SystemWake;
        }

        DeviceWake = UacpiNtWakeDeviceState(Node, SleepState, Capabilities);

        if (Capabilities->DeviceWake < DeviceWake)
            Capabilities->DeviceWake = DeviceWake;

        switch (DeviceWake)
        {
            case PowerDeviceD0:
                Capabilities->WakeFromD0 = TRUE;
                break;

            case PowerDeviceD1:
                Capabilities->WakeFromD1 = TRUE;
                break;

            case PowerDeviceD2:
                Capabilities->WakeFromD2 = TRUE;
                break;

            default:
                Capabilities->WakeFromD3 = TRUE;
                break;
        }

        DPRINT("uACPI-NT: filter wake caps S%lu D%lu\n",
               SleepState,
               (ULONG)(DeviceWake - PowerDeviceD0));
    }

    uacpi_object_unref(Object);
}

VOID
NTAPI
UacpiNtWakeInit(
    _Out_ PUACPINT_WAKE Wake,
    _In_ uacpi_namespace_node *Node)
{
    uacpi_object_name Name;

    Wake->Node = Node;
    Wake->SystemWake = PowerSystemUnspecified;
    Wake->DeviceWake = PowerDeviceUnspecified;
    Wake->RequestedSystemState = PowerSystemUnspecified;
    KeInitializeSpinLock(&Wake->Lock);

    if (!Node)
    {
        Wake->Name[0] = '?';
        Wake->Name[1] = ANSI_NULL;
        return;
    }

    Name = uacpi_namespace_node_name(Node);
    RtlCopyMemory(Wake->Name, Name.text, sizeof(Name.text));
    Wake->Name[sizeof(Name.text)] = ANSI_NULL;
}

/* Resolve the wake GPE from _PRW once, only the \_GPE index form is handled */
static
BOOLEAN
NTAPI
UacpiNtWakeParsePrw(
    _Inout_ PUACPINT_WAKE Wake)
{
    uacpi_object_array Package;
    uacpi_object *Object;
    uacpi_u64 GpeLine = 0;
    ULONG SleepState;

    if (Wake->GpeParsed)
        return Wake->GpeValid;

    Wake->GpeParsed = TRUE;

    if (!Wake->Node)
        return FALSE;

    Object = UacpiNtEvalPrw(Wake->Node, &Package);
    if (!Object)
        return FALSE;

    if (uacpi_object_get_type(Package.objects[0]) == UACPI_OBJECT_INTEGER &&
        uacpi_likely_success(uacpi_object_get_integer(Package.objects[0], &GpeLine)) &&
        GpeLine <= 0xFF)
    {
        Wake->GpeDevice = NULL;
        Wake->GpeLine = (UCHAR)GpeLine;
        Wake->GpeValid = TRUE;
        DPRINT("uACPI-NT: %s wake GPE 0x%02lX\n", Wake->Name, (ULONG)GpeLine);
    }
    else
    {
        DPRINT1("uACPI-NT: %s _PRW names a GPE block device, wake is not armed\n", Wake->Name);
    }

    /* Kept for WAIT_WAKE validation and the _DSW arguments */
    SleepState = UacpiNtPrwSleepState(&Package);
    if (SleepState != 0 && UacpiNtSleepStateSupported(SleepState))
    {
        Wake->SystemWake = (SYSTEM_POWER_STATE)(PowerSystemWorking + SleepState);
        Wake->DeviceWake = UacpiNtWakeDeviceState(Wake->Node, SleepState, NULL);
    }

    uacpi_object_unref(Object);
    return Wake->GpeValid;
}

BOOLEAN
NTAPI
UacpiNtWakeHasPrw(
    _Inout_ PUACPINT_WAKE Wake)
{
    return UacpiNtWakeParsePrw(Wake);
}

/*
 * Switch the device's own wake circuit. Windows 8 and later prefer _DSW so
 * the firmware knows the target sleep depth, earlier releases only use _PSW.
 */
static
VOID
NTAPI
UacpiNtWakeSetPsw(
    _Inout_ PUACPINT_WAKE Wake,
    _In_ BOOLEAN Enable)
{
    uacpi_object_array Arguments;
    uacpi_object *EnableArg;
#if (NTDDI_VERSION >= NTDDI_WIN8)
    uacpi_object *DswArgs[3];
    uacpi_u64 TargetSystem = 0;
    uacpi_u64 TargetDevice = 0;
    ULONG i;
#endif

    if (!Wake->Node)
        return;

    if (!Wake->PswParsed)
    {
        Wake->HasDsw = UacpiNtNodeHasChild(Wake->Node, "_DSW");
        Wake->HasPsw = Wake->HasDsw ? FALSE : UacpiNtNodeHasChild(Wake->Node, "_PSW");
        Wake->PswParsed = TRUE;
    }

    if (!Wake->HasDsw && !Wake->HasPsw)
        return;

#if (NTDDI_VERSION >= NTDDI_WIN8)
    if (Wake->HasDsw)
    {
        if (Enable && Wake->RequestedSystemState > PowerSystemWorking)
            TargetSystem = (uacpi_u64)(Wake->RequestedSystemState - PowerSystemWorking);

        if (Enable && Wake->DeviceWake >= PowerDeviceD0 && Wake->DeviceWake <= PowerDeviceD3)
            TargetDevice = (uacpi_u64)(Wake->DeviceWake - PowerDeviceD0);

        DswArgs[0] = uacpi_object_create_integer(Enable ? 1 : 0);
        DswArgs[1] = uacpi_object_create_integer(TargetSystem);
        DswArgs[2] = uacpi_object_create_integer(TargetDevice);

        if (DswArgs[0] && DswArgs[1] && DswArgs[2])
        {
            Arguments.objects = DswArgs;
            Arguments.count = RTL_NUMBER_OF(DswArgs);
            uacpi_eval(Wake->Node, "_DSW", &Arguments, NULL);
        }

        for (i = 0; i < RTL_NUMBER_OF(DswArgs); i++)
        {
            if (DswArgs[i])
                uacpi_object_unref(DswArgs[i]);
        }
        return;
    }
#endif

    EnableArg = uacpi_object_create_integer(Enable ? 1 : 0);
    if (!EnableArg)
        return;

    Arguments.objects = &EnableArg;
    Arguments.count = 1;
    uacpi_eval(Wake->Node, "_PSW", &Arguments, NULL);
    uacpi_object_unref(EnableArg);
}

/* Undo the arm in reverse order so the uACPI GPE refcounts balance */
static
VOID
NTAPI
UacpiNtWakeReleaseHardware(
    _Inout_ PUACPINT_WAKE Wake)
{
    uacpi_disable_gpe(Wake->GpeDevice, Wake->GpeLine);
    uacpi_disable_gpe_for_wake(Wake->GpeDevice, Wake->GpeLine);
    UacpiNtWakeSetPsw(Wake, FALSE);
    UacpiNtPowerResReleaseWake(Wake->WakeResources, Wake->WakeResourceCount);
    Wake->WakeResourceCount = 0;
}

static
VOID
NTAPI
UacpiNtWakeDisarm(
    _Inout_ PUACPINT_WAKE Wake)
{
    if (!Wake->Armed)
        return;

    Wake->Armed = FALSE;
    UacpiNtWakeReleaseHardware(Wake);
}

/* The cancel routine may run at DISPATCH_LEVEL, the GPE and AML work cannot */
static
VOID
NTAPI
UacpiNtWakeDisarmWorker(
    _In_ PVOID Context)
{
    PUACPINT_WAKE Wake = Context;
    BOOLEAN Disarm = FALSE;
    KIRQL OldIrql;

    InterlockedExchange(&Wake->DisarmQueued, 0);

    /* A new WAIT_WAKE may have armed the device again since the cancel */
    KeAcquireSpinLock(&Wake->Lock, &OldIrql);
    if (!Wake->WaitWakeIrp && Wake->Armed)
    {
        Wake->Armed = FALSE;
        Disarm = TRUE;
    }
    KeReleaseSpinLock(&Wake->Lock, OldIrql);

    if (!Disarm)
        return;

    UacpiNtWakeReleaseHardware(Wake);
    DPRINT("uACPI-NT: %s deferred wake disarm done\n", Wake->Name);
}

/*
 * Collect every armed or depth suspended wake block so AML can run
 * without the child lock held.
 */
static
ULONG
NTAPI
UacpiNtWakeSnapshotActive(
    _Out_writes_to_(MaxCount, return) PUACPINT_WAKE *Snapshot,
    _In_ ULONG MaxCount)
{
    PUACPINT_FDO Fdo = GlobalAcpiFdo;
    PLIST_ENTRY Entry;
    PUACPINT_PDO Pdo;
    PUACPINT_FLT Filter;
    ULONG Count = 0;

    if (!Fdo)
        return 0;

    ExAcquireFastMutex(&Fdo->ChildLock);

    for (Entry = Fdo->ChildList.Flink;
         Entry != &Fdo->ChildList && Count < MaxCount;
         Entry = Entry->Flink)
    {
        Pdo = CONTAINING_RECORD(Entry, UACPINT_PDO, Link);
        if ((Pdo->Wake.Armed || Pdo->Wake.DepthSuspended) && Pdo->Wake.Node)
            Snapshot[Count++] = &Pdo->Wake;
    }

    for (Entry = Fdo->FilterList.Flink;
         Entry != &Fdo->FilterList && Count < MaxCount;
         Entry = Entry->Flink)
    {
        Filter = CONTAINING_RECORD(Entry, UACPINT_FLT, Link);
        if ((Filter->Wake.Armed || Filter->Wake.DepthSuspended) && Filter->Wake.Node)
            Snapshot[Count++] = &Filter->Wake;
    }

    ExReleaseFastMutex(&Fdo->ChildLock);
    return Count;
}

/*
 * Mask armed devices whose WAIT_WAKE asked for a shallower state than the one
 * being entered. Runs between clearing all events and enabling the wake GPEs.
 */
VOID
NTAPI
UacpiNtWakeSuspendShallow(
    _In_ SYSTEM_POWER_STATE Target)
{
    PUACPINT_WAKE Snapshot[UACPINT_WAKE_SNAPSHOT_MAX];
    PUACPINT_WAKE Wake;
    ULONG Count;
    ULONG i;

    Count = UacpiNtWakeSnapshotActive(Snapshot, RTL_NUMBER_OF(Snapshot));
    for (i = 0; i < Count; i++)
    {
        Wake = Snapshot[i];

        if (!Wake->Armed || Wake->DepthSuspended)
            continue;

        if (Wake->RequestedSystemState == PowerSystemUnspecified ||
            Wake->RequestedSystemState >= Target)
        {
            continue;
        }

        uacpi_disable_gpe_for_wake(Wake->GpeDevice, Wake->GpeLine);
        Wake->DepthSuspended = TRUE;

        DPRINT("uACPI-NT: %s wake masked, wants S%lu, entering S%lu\n",
               Wake->Name,
               (ULONG)(Wake->RequestedSystemState - PowerSystemWorking),
               (ULONG)(Target - PowerSystemWorking));
    }
}

VOID
NTAPI
UacpiNtWakeRestoreSuspended(VOID)
{
    PUACPINT_WAKE Snapshot[UACPINT_WAKE_SNAPSHOT_MAX];
    ULONG Count;
    ULONG i;

    Count = UacpiNtWakeSnapshotActive(Snapshot, RTL_NUMBER_OF(Snapshot));
    for (i = 0; i < Count; i++)
    {
        if (!Snapshot[i]->DepthSuspended)
            continue;

        uacpi_enable_gpe_for_wake(Snapshot[i]->GpeDevice, Snapshot[i]->GpeLine);
        Snapshot[i]->DepthSuspended = FALSE;
    }
}

/* Hibernate cut all power, so the wake circuit of every armed device is set again */
VOID
NTAPI
UacpiNtWakeReArmAfterHibernate(VOID)
{
    PUACPINT_WAKE Snapshot[UACPINT_WAKE_SNAPSHOT_MAX];
    ULONG Count;
    ULONG i;

    Count = UacpiNtWakeSnapshotActive(Snapshot, RTL_NUMBER_OF(Snapshot));
    for (i = 0; i < Count; i++)
    {
        if (!Snapshot[i]->Armed)
            continue;

        UacpiNtWakeSetPsw(Snapshot[i], TRUE);
        DPRINT("uACPI-NT: %s wake circuit enabled after hibernate\n", Snapshot[i]->Name);
    }
}

static
VOID
NTAPI
UacpiNtWaitWakeCancel(
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PUACPINT_WAKE Wake = Irp->Tail.Overlay.DriverContext[0];
    BOOLEAN Owned = FALSE;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(DeviceObject);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    KeAcquireSpinLock(&Wake->Lock, &OldIrql);
    if (Wake->WaitWakeIrp == Irp)
    {
        /* Armed stays set, the worker clears it */
        Wake->WaitWakeIrp = NULL;
        Owned = TRUE;
    }
    KeReleaseSpinLock(&Wake->Lock, OldIrql);

    if (!Owned)
        return;

    UacpiNtCompleteIrp(Irp, STATUS_CANCELLED, 0);

    if (InterlockedExchange(&Wake->DisarmQueued, 1) == 0)
    {
        ExInitializeWorkItem(&Wake->DisarmWork, UacpiNtWakeDisarmWorker, Wake);
        ExQueueWorkItem(&Wake->DisarmWork, DelayedWorkQueue);
    }

    DPRINT("uACPI-NT: %s WAIT_WAKE canceled\n", Wake->Name);
}

/* Takes the pended WAIT_WAKE away from the cancel routine, NULL if it lost */
static
PIRP
NTAPI
UacpiNtWakeClaimIrp(
    _Inout_ PUACPINT_WAKE Wake)
{
    PIRP Irp;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Wake->Lock, &OldIrql);

    Irp = Wake->WaitWakeIrp;
    if (Irp && IoSetCancelRoutine(Irp, NULL))
        Wake->WaitWakeIrp = NULL;
    else
        Irp = NULL;

    KeReleaseSpinLock(&Wake->Lock, OldIrql);
    return Irp;
}

VOID
NTAPI
UacpiNtWakeComplete(
    _Inout_ PUACPINT_WAKE Wake)
{
    PIRP Irp;

    Irp = UacpiNtWakeClaimIrp(Wake);
    if (!Irp)
        return;

    UacpiNtWakeDisarm(Wake);
    DPRINT("uACPI-NT: %s signaled wake\n", Wake->Name);

    /* Only the first device done in a real resume is the system wake source */
    if (UacpiNtSystemResuming != 0 &&
        InterlockedCompareExchange(&UacpiNtWakeSourceClaimed, 1, 0) == 0)
    {
        PoSetSystemWake(Irp);
    }

    UacpiNtCompleteIrp(Irp, STATUS_SUCCESS, 0);
}

/* A removed device must not leave its wake GPE enabled */
VOID
NTAPI
UacpiNtWakeTeardown(
    _Inout_ PUACPINT_WAKE Wake)
{
    PIRP Irp;

    Irp = UacpiNtWakeClaimIrp(Wake);
    if (Irp)
        UacpiNtCompleteIrp(Irp, STATUS_NO_SUCH_DEVICE, 0);

    UacpiNtWakeDisarm(Wake);
}

NTSTATUS
NTAPI
UacpiNtWakeArm(
    _Inout_ PUACPINT_WAKE Wake,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    SYSTEM_POWER_STATE Requested = IoStack->Parameters.WaitWake.PowerState;
    uacpi_status UacpiStatus;
    BOOLEAN Busy;
    KIRQL OldIrql;

    if (!UacpiNtWakeParsePrw(Wake))
        return UacpiNtCompleteIrp(Irp, STATUS_NOT_SUPPORTED, 0);

    /* A failed _PRW[1] parse never blocks the request */
    if (Wake->SystemWake != PowerSystemUnspecified && Requested > Wake->SystemWake)
    {
        DPRINT1("uACPI-NT: %s WAIT_WAKE from S%lu is deeper than _PRW S%lu\n",
                Wake->Name,
                (ULONG)(Requested - PowerSystemWorking),
                (ULONG)(Wake->SystemWake - PowerSystemWorking));
        return UacpiNtCompleteIrp(Irp, STATUS_INVALID_DEVICE_STATE, 0);
    }

    Wake->RequestedSystemState = Requested;

    KeAcquireSpinLock(&Wake->Lock, &OldIrql);
    Busy = (Wake->WaitWakeIrp != NULL);
    KeReleaseSpinLock(&Wake->Lock, OldIrql);

    if (Busy)
        return UacpiNtCompleteIrp(Irp, STATUS_DEVICE_BUSY, 0);

    if (!Wake->SetupDone)
    {
        UacpiStatus = uacpi_setup_gpe_for_wake(Wake->GpeDevice, Wake->GpeLine, Wake->Node);
        if (uacpi_unlikely_error(UacpiStatus))
        {
            DPRINT1("uACPI-NT: %s wake GPE setup failed: %s\n",
                    Wake->Name,
                    uacpi_status_to_string(UacpiStatus));
            return UacpiNtCompleteIrp(Irp, STATUS_NOT_SUPPORTED, 0);
        }

        Wake->SetupDone = TRUE;
    }

    /* A stale latched status would complete the wait right away */
    uacpi_clear_gpe(Wake->GpeDevice, Wake->GpeLine);

    UacpiStatus = uacpi_enable_gpe_for_wake(Wake->GpeDevice, Wake->GpeLine);
    if (uacpi_unlikely_error(UacpiStatus))
    {
        DPRINT1("uACPI-NT: %s wake GPE mask failed: %s\n",
                Wake->Name,
                uacpi_status_to_string(UacpiStatus));
        return UacpiNtCompleteIrp(Irp, STATUS_NOT_SUPPORTED, 0);
    }

    /* The wake mask only matters at sleep, S0 wakes need the runtime enable too */
    UacpiStatus = uacpi_enable_gpe(Wake->GpeDevice, Wake->GpeLine);
    if (uacpi_unlikely_error(UacpiStatus))
    {
        uacpi_disable_gpe_for_wake(Wake->GpeDevice, Wake->GpeLine);
        DPRINT1("uACPI-NT: %s wake GPE enable failed: %s\n",
                Wake->Name,
                uacpi_status_to_string(UacpiStatus));
        return UacpiNtCompleteIrp(Irp, STATUS_NOT_SUPPORTED, 0);
    }

    Wake->Armed = TRUE;

    /* The _PRW power resources go on before the device wake circuit */
    Wake->WakeResourceCount = UacpiNtPowerResAcquireWake(Wake->Node,
                                                         Wake->WakeResources,
                                                         RTL_NUMBER_OF(Wake->WakeResources));
    UacpiNtWakeSetPsw(Wake, TRUE);

    /* The context has to be in place before the cancel routine can see the IRP */
    Irp->Tail.Overlay.DriverContext[0] = Wake;

    KeAcquireSpinLock(&Wake->Lock, &OldIrql);
    IoSetCancelRoutine(Irp, UacpiNtWaitWakeCancel);
    if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL))
    {
        KeReleaseSpinLock(&Wake->Lock, OldIrql);
        UacpiNtWakeDisarm(Wake);
        return UacpiNtCompleteIrp(Irp, STATUS_CANCELLED, 0);
    }

    IoMarkIrpPending(Irp);
    Wake->WaitWakeIrp = Irp;
    KeReleaseSpinLock(&Wake->Lock, OldIrql);

    DPRINT("uACPI-NT: %s WAIT_WAKE armed on GPE 0x%02X\n", Wake->Name, Wake->GpeLine);
    return STATUS_PENDING;
}

/* The PDO is the bottom of its stack, every power IRP completes here */
NTSTATUS
NTAPI
UacpiNtPdoSetPower(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status = STATUS_SUCCESS;

    PoStartNextPowerIrp(Irp);

    switch (IoStack->MinorFunction)
    {
        case IRP_MN_WAIT_WAKE:
            return UacpiNtWakeArm(&Pdo->Wake, Irp);

        /* System IRPs are only acknowledged, the device IRP follows */
        case IRP_MN_SET_POWER:
            if (IoStack->Parameters.Power.Type == DevicePowerState)
            {
                Status = UacpiNtPowerSetDeviceState(Pdo, IoStack->Parameters.Power.State.DeviceState);
                PoSetPowerState(Pdo->Shared.Self, DevicePowerState, IoStack->Parameters.Power.State);
            }
            break;

        case IRP_MN_QUERY_POWER:
            break;

        /* Leave minors we do not handle, such as POWER_SEQUENCE, as they came */
        default:
            Status = Irp->IoStatus.Status;
            break;
    }

    return UacpiNtCompleteIrp(Irp, Status, Irp->IoStatus.Information);
}
