/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ACPI thermal zone interface, thermal IOCTLs and WMI provider
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/*
 * What each release expects from a zone:
 *   Vista, Win7  THERMAL_INFORMATION with the _PSL processor mask
 *   Win8, 8.1    76 byte extended query with _HOT, passive limit, _NTT and _DTI
 *   Win10 and up 88 byte extended query with _CR3, _MTL, _DSM and a _STR description
 */

#include <uacpint.h>
#include <poclass.h>
#include <wmistr.h>
#include <wmilib.h>
#include <debug.h>

/* _STR description request, the caller frees the returned string */
#define IOCTL_UACPINT_THERMAL_DESCRIPTION \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x26, METHOD_BUFFERED, FILE_READ_ACCESS)

#define UACPINT_THERMAL_DESC_TAG    'TpcA'

#define UACPINT_NOTIFY_TEMPERATURE  0x80
#define UACPINT_NOTIFY_TRIP_POINTS  0x81

/* Room temperature fallback for WMI, 0 C in tenths of a Kelvin */
#define UACPINT_THERMAL_WMI_DEFAULT_TEMP 2732

/* Worker steps, run in this order */
#define UACPINT_THERMAL_WORK_INIT           0x01    ///< _DSM mask and _STR, once per start
#define UACPINT_THERMAL_WORK_POLICY         0x02    ///< _SCP with CoolingPolicy
#define UACPINT_THERMAL_WORK_TRIPS          0x04    ///< constants, trip points, _PSL
#define UACPINT_THERMAL_WORK_ACTIVE         0x08    ///< _ALx devices for FanLevel
#define UACPINT_THERMAL_WORK_TEMPERATURE    0x10    ///< _TMP, bumps ThermalStamp
#define UACPINT_THERMAL_WORK_COMPLETE       0x20    ///< complete the queued IRPs

#define UACPINT_THERMAL_WORK_ALL \
    (UACPINT_THERMAL_WORK_INIT | UACPINT_THERMAL_WORK_POLICY | UACPINT_THERMAL_WORK_TRIPS | \
     UACPINT_THERMAL_WORK_ACTIVE | UACPINT_THERMAL_WORK_TEMPERATURE | UACPINT_THERMAL_WORK_COMPLETE)

/* IOCTL_THERMAL_QUERY_INFORMATION output for the target release */
#if (NTDDI_VERSION >= NTDDI_WIN10)
typedef struct _UACPINT_THERMAL_QUERY
{
    ULONG ThermalStamp;
    ULONG ThermalConstant1;
    ULONG ThermalConstant2;
    ULONG SamplingPeriod;           ///< milliseconds
    ULONG CurrentTemperature;
    ULONG PassiveTripPoint;
    ULONG ThermalStandbyTripPoint;
    ULONG CriticalTripPoint;
    UCHAR ActiveTripPointCount;
    ULONG ActiveTripPoint[MAX_ACTIVE_COOLING_LEVELS];
    ULONG S4TransitionTripPoint;
    ULONG MinimumThrottle;
    ULONG OverThrottleThreshold;
} UACPINT_THERMAL_QUERY, *PUACPINT_THERMAL_QUERY;
C_ASSERT(sizeof(UACPINT_THERMAL_QUERY) == 88);
#elif (NTDDI_VERSION >= NTDDI_WIN8)
typedef struct _UACPINT_THERMAL_QUERY
{
    ULONG ThermalStamp;
    ULONG ThermalConstant1;
    ULONG ThermalConstant2;
    ULONG SamplingPeriod;
    ULONG CurrentTemperature;
    ULONG PassiveTripPoint;
    ULONG CriticalTripPoint;
    UCHAR ActiveTripPointCount;
    ULONG ActiveTripPoint[MAX_ACTIVE_COOLING_LEVELS];
    ULONG S4TransitionTripPoint;
} UACPINT_THERMAL_QUERY, *PUACPINT_THERMAL_QUERY;
C_ASSERT(sizeof(UACPINT_THERMAL_QUERY) == 76);
#else
typedef THERMAL_INFORMATION UACPINT_THERMAL_QUERY, *PUACPINT_THERMAL_QUERY;
C_ASSERT(sizeof(UACPINT_THERMAL_QUERY) == (sizeof(KAFFINITY) == 8 ? 88 : 76));
#endif

/* MSAcpi_ThermalZoneTemperature data block */
#include <pshpack1.h>
typedef struct _UACPINT_THERMAL_WMI
{
    ULONG ThermalStamp;
    ULONG ThermalConstant1;
    ULONG ThermalConstant2;
    ULONG Reserved;
    ULONG SamplingPeriod;
    ULONG CurrentTemperature;
    ULONG PassiveTripPoint;
    ULONG CriticalTripPoint;
    ULONG ActiveTripPointCount;
    ULONG ActiveTripPoint[MAX_ACTIVE_COOLING_LEVELS];
} UACPINT_THERMAL_WMI, *PUACPINT_THERMAL_WMI;
#include <poppack.h>

static WMIGUIDREGINFO UacpiNtThermalWmiGuids[] =
{
    { &MSAcpi_ThermalZoneTemperature_GUID, 1, 0 },
};

static LONG UacpiNtThermalWmiStamp;
static LONG UacpiNtThermalQuerySizeReported;

#if (NTDDI_VERSION >= NTDDI_WIN10)
/* Thermal extensions _DSM {14D399CD-7A27-4B18-8FB4-7CB7B9F4E500}, revision 0 */
static const GUID UacpiNtThermalDsmUuid =
    { 0x14d399cd, 0x7a27, 0x4b18, { 0x8f, 0xb4, 0x7c, 0xb7, 0xb9, 0xf4, 0xe5, 0x00 } };
#endif

static
BOOLEAN
NTAPI
UacpiNtThermalEvalInteger(
    _In_opt_ uacpi_namespace_node *Node,
    _In_z_ const char *Method,
    _Out_ PULONG Value)
{
    uacpi_u64 Result = 0;

    if (!Node || uacpi_unlikely_error(uacpi_eval_simple_integer(Node, Method, &Result)))
        return FALSE;

    *Value = (ULONG)Result;
    return TRUE;
}

/* A missing or failing method reads as Default */
static
ULONG
NTAPI
UacpiNtThermalEvalDefault(
    _In_opt_ uacpi_namespace_node *Node,
    _In_z_ const char *Method,
    _In_ ULONG Default)
{
    ULONG Value;

    if (UacpiNtThermalEvalInteger(Node, Method, &Value))
        return Value;

    return Default;
}

/* For _SCP and _DTI, which take one Integer and return nothing useful */
static
VOID
NTAPI
UacpiNtThermalCallWithInteger(
    _In_opt_ uacpi_namespace_node *Node,
    _In_z_ const char *Method,
    _In_ ULONG Argument)
{
    uacpi_object_array Arguments;
    uacpi_object *Object;

    if (!Node || !UacpiNtNodeHasChild(Node, Method))
        return;

    Object = uacpi_object_create_integer(Argument);
    if (!Object)
        return;

    Arguments.objects = &Object;
    Arguments.count = 1;
    uacpi_eval(Node, Method, &Arguments, NULL);
    uacpi_object_unref(Object);
}

/* _ACx are contiguous from _AC0, returns how many were found */
static
ULONG
NTAPI
UacpiNtThermalReadActiveTrips(
    _In_opt_ uacpi_namespace_node *Node,
    _Out_writes_(MAX_ACTIVE_COOLING_LEVELS) PULONG TripPoints)
{
    CHAR Method[5] = "_AC0";
    ULONG Count;

    for (Count = 0; Count < MAX_ACTIVE_COOLING_LEVELS; Count++)
    {
        Method[3] = (CHAR)('0' + Count);
        if (!UacpiNtThermalEvalInteger(Node, Method, &TripPoints[Count]))
            break;
    }

    return Count;
}

/* _ALx and _PSL entries name devices relative to the zone */
static
uacpi_namespace_node *
NTAPI
UacpiNtThermalResolveReference(
    _In_ uacpi_object *Reference,
    _In_ uacpi_namespace_node *Scope)
{
    uacpi_namespace_node *Node = NULL;

    if (uacpi_unlikely_error(uacpi_object_resolve_as_aml_namepath(Reference, Scope, &Node)))
        return NULL;

    return Node;
}

#if (NTDDI_VERSION >= NTDDI_WIN10)
static
BOOLEAN
NTAPI
UacpiNtThermalEvalDsm(
    _In_ PUACPINT_PDO Pdo,
    _In_ ULONG Function,
    _Outptr_result_maybenull_ uacpi_object **Result)
{
    uacpi_object *Arguments[4];
    uacpi_object_array ArgumentArray;
    uacpi_object_array EmptyPackage = { 0 };
    uacpi_data_view Uuid;
    BOOLEAN Complete = TRUE;
    BOOLEAN Success = FALSE;
    ULONG Index;

    *Result = NULL;

    if (!UacpiNtNodeHasChild(Pdo->Node, "_DSM"))
        return FALSE;

    Uuid.const_bytes = (const uacpi_u8 *)&UacpiNtThermalDsmUuid;
    Uuid.length = sizeof(UacpiNtThermalDsmUuid);

    Arguments[0] = uacpi_object_create_buffer(Uuid);
    Arguments[1] = uacpi_object_create_integer(0);
    Arguments[2] = uacpi_object_create_integer(Function);
    Arguments[3] = uacpi_object_create_package(EmptyPackage);

    for (Index = 0; Index < RTL_NUMBER_OF(Arguments); Index++)
    {
        if (!Arguments[Index])
            Complete = FALSE;
    }

    if (Complete)
    {
        ArgumentArray.objects = Arguments;
        ArgumentArray.count = RTL_NUMBER_OF(Arguments);
        Success = uacpi_likely_success(uacpi_eval(Pdo->Node, "_DSM", &ArgumentArray, Result)) &&
                  *Result;
    }

    for (Index = 0; Index < RTL_NUMBER_OF(Arguments); Index++)
    {
        if (Arguments[Index])
            uacpi_object_unref(Arguments[Index]);
    }

    return Success;
}

/* An advertised _DSM function returning a percentage, capped at 100 */
static
BOOLEAN
NTAPI
UacpiNtThermalDsmPercent(
    _In_ PUACPINT_PDO Pdo,
    _In_ ULONG Function,
    _Out_ PULONG Percent)
{
    uacpi_object *Result;
    uacpi_u64 Value = 0;
    BOOLEAN Success;

    if (!(Pdo->Thermal.DsmFunctions & (1UL << Function)))
        return FALSE;

    if (!UacpiNtThermalEvalDsm(Pdo, Function, &Result))
        return FALSE;

    Success = uacpi_likely_success(uacpi_object_get_integer(Result, &Value));
    uacpi_object_unref(Result);

    if (Success)
        *Percent = (Value > 100) ? 100 : (ULONG)Value;

    return Success;
}

/* _STR is a NUL terminated UTF-16 buffer */
static
VOID
NTAPI
UacpiNtThermalReadDescription(
    _Inout_ PUACPINT_PDO Pdo)
{
    uacpi_object *Object = NULL;
    uacpi_data_view View;
    const WCHAR *Text;
    PWCHAR Copy;
    BOOLEAN Valid;

    if (uacpi_unlikely_error(uacpi_eval_simple_buffer(Pdo->Node, "_STR", &Object)) || !Object)
        return;

    Valid = uacpi_likely_success(uacpi_object_get_buffer(Object, &View)) &&
            View.length > sizeof(WCHAR) &&
            (View.length % sizeof(WCHAR)) == 0 &&
            View.length <= MAXUSHORT;
    if (Valid)
    {
        Text = (const WCHAR *)View.const_bytes;
        Valid = (Text[View.length / sizeof(WCHAR) - 1] == UNICODE_NULL);
    }

    if (Valid)
    {
        Copy = ExAllocatePoolWithTag(NonPagedPool, View.length, UACPINT_POOL_TAG);
        if (Copy)
        {
            RtlCopyMemory(Copy, View.const_bytes, View.length);
            Pdo->Thermal.Description.Buffer = Copy;
            Pdo->Thermal.Description.Length = (USHORT)(wcslen(Copy) * sizeof(WCHAR));
            Pdo->Thermal.Description.MaximumLength =
                Pdo->Thermal.Description.Length + sizeof(WCHAR);
        }
    }

    uacpi_object_unref(Object);
}

/* Handed out in a fresh pool buffer that the requester frees */
static
VOID
NTAPI
UacpiNtThermalCopyDescription(
    _In_ PUACPINT_THERMAL_ZONE Zone,
    _Out_ PUNICODE_STRING Output)
{
    RtlZeroMemory(Output, sizeof(*Output));

    if (!Zone->Description.Buffer)
        return;

    Output->Buffer = ExAllocatePoolWithTag(NonPagedPoolNx,
                                           Zone->Description.MaximumLength,
                                           UACPINT_THERMAL_DESC_TAG);
    if (!Output->Buffer)
        return;

    RtlCopyMemory(Output->Buffer, Zone->Description.Buffer, Zone->Description.MaximumLength);
    Output->Length = Zone->Description.Length;
    Output->MaximumLength = Zone->Description.MaximumLength;
}

static
VOID
NTAPI
UacpiNtThermalReadThrottleLimits(
    _Inout_ PUACPINT_PDO Pdo)
{
    PUACPINT_THERMAL_INFO Info = &Pdo->Thermal.Info;
    ULONG Value;

    if (UacpiNtThermalEvalInteger(Pdo->Node, "_MTL", &Value))
        Info->MinimumThrottle = (Value > 100) ? 100 : Value;
    else if (!UacpiNtThermalDsmPercent(Pdo, 1, &Info->MinimumThrottle))
        Info->MinimumThrottle = 0;

    if (!UacpiNtThermalDsmPercent(Pdo, 3, &Info->OverThrottleThreshold))
        Info->OverThrottleThreshold = 0;
}
#endif

static
VOID
NTAPI
UacpiNtThermalInit(
    _Inout_ PUACPINT_PDO Pdo)
{
#if (NTDDI_VERSION >= NTDDI_WIN10)
    uacpi_object *Result;
    uacpi_data_view View;

    /* Byte 0 of function 0 is the supported function mask */
    Pdo->Thermal.DsmFunctions = 0;
    if (UacpiNtThermalEvalDsm(Pdo, 0, &Result))
    {
        if (uacpi_likely_success(uacpi_object_get_buffer(Result, &View)) && View.length >= 1)
            Pdo->Thermal.DsmFunctions = View.const_bytes[0];

        uacpi_object_unref(Result);
    }

    if (!Pdo->Thermal.Description.Buffer)
        UacpiNtThermalReadDescription(Pdo);
#endif

    Pdo->Thermal.DtiArmed = FALSE;
}

#if (NTDDI_VERSION < NTDDI_WIN8)
/* No ACPI to NT processor map exists, so any listed processor means all of them */
static
KAFFINITY
NTAPI
UacpiNtThermalPassiveProcessors(
    _In_ PUACPINT_PDO Pdo)
{
    uacpi_object *Package = NULL;
    uacpi_object_array Elements;
    KAFFINITY Processors = 0;
    uacpi_size Index;

    if (uacpi_unlikely_error(uacpi_eval_simple_package(Pdo->Node, "_PSL", &Package)) || !Package)
        return 0;

    if (uacpi_likely_success(uacpi_object_get_package(Package, &Elements)))
    {
        for (Index = 0; Index < Elements.count; Index++)
        {
            if (UacpiNtThermalResolveReference(Elements.objects[Index], Pdo->Node))
            {
                Processors = KeQueryActiveProcessors();
                break;
            }
        }
    }

    uacpi_object_unref(Package);
    return Processors;
}
#endif

/* Constants and trip points, anything missing reads as 0 */
static
VOID
NTAPI
UacpiNtThermalReadTrips(
    _Inout_ PUACPINT_PDO Pdo)
{
    PUACPINT_THERMAL_INFO Info = &Pdo->Thermal.Info;
    uacpi_namespace_node *Node = Pdo->Node;

    Info->ThermalConstant1 = UacpiNtThermalEvalDefault(Node, "_TC1", 0);
    Info->ThermalConstant2 = UacpiNtThermalEvalDefault(Node, "_TC2", 0);
    Info->PassiveTripPoint = UacpiNtThermalEvalDefault(Node, "_PSV", 0);
    Info->CriticalTripPoint = UacpiNtThermalEvalDefault(Node, "_CRT", 0);

#if (NTDDI_VERSION >= NTDDI_WIN10)
    /* Reported in milliseconds, _TSP is in tenths of a second */
    if (!UacpiNtThermalEvalInteger(Node, "_TFP", &Info->SamplingPeriod))
        Info->SamplingPeriod = UacpiNtThermalEvalDefault(Node, "_TSP", 0) * 100;

    Info->StandbyTripPoint = UacpiNtThermalEvalDefault(Node, "_CR3", 0);
#else
    Info->SamplingPeriod = UacpiNtThermalEvalDefault(Node, "_TSP", 0);
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
    Info->S4TripPoint = UacpiNtThermalEvalDefault(Node, "_HOT", 0);
    Pdo->Thermal.TripDelta = UacpiNtThermalEvalDefault(Node, "_NTT", 0);
#endif

    RtlZeroMemory(Info->ActiveTripPoint, sizeof(Info->ActiveTripPoint));
    Info->ActiveTripPointCount = UacpiNtThermalReadActiveTrips(Node, Info->ActiveTripPoint);

#if (NTDDI_VERSION >= NTDDI_WIN10)
    UacpiNtThermalReadThrottleLimits(Pdo);
#elif (NTDDI_VERSION < NTDDI_WIN8)
    Info->Processors = UacpiNtThermalPassiveProcessors(Pdo);
#endif

    DPRINT("uACPI-NT: %s trips PSV %lu CRT %lu, %lu active\n",
           Pdo->Name,
           Info->PassiveTripPoint,
           Info->CriticalTripPoint,
           Info->ActiveTripPointCount);
}

/* Devices in _ALx for x at or above Level run, the rest are turned off */
static
VOID
NTAPI
UacpiNtThermalApplyActiveList(
    _In_ PUACPINT_PDO Pdo,
    _In_ uacpi_object *Package,
    _In_z_ const char *Method,
    _In_ DEVICE_POWER_STATE State)
{
    uacpi_object_array Elements;
    uacpi_namespace_node *Device;
    PUACPINT_PDO Cooler;
    uacpi_size Index;

    if (uacpi_unlikely_error(uacpi_object_get_package(Package, &Elements)))
        return;

    for (Index = 0; Index < Elements.count; Index++)
    {
        Device = UacpiNtThermalResolveReference(Elements.objects[Index], Pdo->Node);
        Cooler = Device ? UacpiNtFindPdoByNode(GlobalAcpiFdo, Device) : NULL;
        if (!Cooler)
        {
            DPRINT("uACPI-NT: %s %s[%lu] has no device PDO\n", Pdo->Name, Method, (ULONG)Index);
            continue;
        }

        if (!Cooler->DStateKnown || Cooler->CurrentDState != State)
            UacpiNtPowerSetDeviceState(Cooler, State);
    }
}

static
VOID
NTAPI
UacpiNtThermalRunActive(
    _In_ PUACPINT_PDO Pdo,
    _In_ UCHAR Level)
{
    CHAR Method[5] = "_AL0";
    uacpi_object *Package;
    ULONG Index;

    /* _ALx are contiguous from _AL0 */
    for (Index = 0; Index < MAX_ACTIVE_COOLING_LEVELS; Index++)
    {
        Method[3] = (CHAR)('0' + Index);

        Package = NULL;
        if (uacpi_unlikely_error(uacpi_eval_simple_package(Pdo->Node, Method, &Package)) || !Package)
            break;

        UacpiNtThermalApplyActiveList(Pdo,
                                      Package,
                                      Method,
                                      (Index >= Level) ? PowerDeviceD0 : PowerDeviceD3);
        uacpi_object_unref(Package);
    }
}

#if (NTDDI_VERSION >= NTDDI_WIN8)
/* TRUE when a trip point lies between the last notify temperature and now */
static
BOOLEAN
NTAPI
UacpiNtThermalCrossedTrip(
    _In_ PUACPINT_THERMAL_INFO Info,
    _In_ ULONG NotifiedTemperature)
{
    ULONG TripPoints[4 + MAX_ACTIVE_COOLING_LEVELS];
    ULONG Current = Info->CurrentTemperature;
    ULONG Count = 0;
    ULONG Trip;
    ULONG Index;

    TripPoints[Count++] = Info->PassiveTripPoint;
    TripPoints[Count++] = Info->CriticalTripPoint;
    TripPoints[Count++] = Info->S4TripPoint;
#if (NTDDI_VERSION >= NTDDI_WIN10)
    TripPoints[Count++] = Info->StandbyTripPoint;
#endif
    for (Index = 0; Index < Info->ActiveTripPointCount; Index++)
        TripPoints[Count++] = Info->ActiveTripPoint[Index];

    for (Index = 0; Index < Count; Index++)
    {
        Trip = TripPoints[Index];
        if (!Trip)
            continue;

        if ((NotifiedTemperature < Trip && Trip <= Current) || (Current <= Trip && Trip < NotifiedTemperature))
            return TRUE;
    }

    return FALSE;
}

/* _DTI rearms the platform notification after an _NTT sized move or a trip crossing */
static
VOID
NTAPI
UacpiNtThermalCheckDti(
    _Inout_ PUACPINT_PDO Pdo)
{
    PUACPINT_THERMAL_ZONE Zone = &Pdo->Thermal;
    ULONG Current = Zone->Info.CurrentTemperature;
    ULONG NotifiedTemperature = Zone->NotifiedTemperature;
    ULONG Delta = Zone->TripDelta;
    BOOLEAN Rearm;

    if (!UacpiNtNodeHasChild(Pdo->Node, "_DTI"))
        return;

    Rearm = !Zone->DtiArmed;

    if (!Rearm && Delta)
    {
        Rearm = (NotifiedTemperature > Delta && Current <= NotifiedTemperature - Delta) ||
                (NotifiedTemperature <= MAXULONG - Delta && Current >= NotifiedTemperature + Delta);
    }

    if (!Rearm)
        Rearm = UacpiNtThermalCrossedTrip(&Zone->Info, NotifiedTemperature);

    if (!Rearm)
        return;

    Zone->NotifiedTemperature = Current;
    Zone->DtiArmed = TRUE;
    UacpiNtThermalCallWithInteger(Pdo->Node, "_DTI", Current);
}
#endif

/* Each reading gets a new stamp, a query holding an older one wants it */
static
VOID
NTAPI
UacpiNtThermalReadTemperature(
    _Inout_ PUACPINT_PDO Pdo)
{
    PUACPINT_THERMAL_ZONE Zone = &Pdo->Thermal;
    ULONG Temperature;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Zone->Lock, &OldIrql);
    Zone->Info.ThermalStamp++;
    KeReleaseSpinLock(&Zone->Lock, OldIrql);

    if (UacpiNtThermalEvalInteger(Pdo->Node, "_TMP", &Temperature))
        Zone->Info.CurrentTemperature = Temperature;

#if (NTDDI_VERSION >= NTDDI_WIN8)
    UacpiNtThermalCheckDti(Pdo);
#endif
}

static
VOID
NTAPI
UacpiNtThermalFillQuery(
    _In_ PUACPINT_THERMAL_INFO Info,
    _Out_ PUACPINT_THERMAL_QUERY Query)
{
    ULONG Index;

    RtlZeroMemory(Query, sizeof(*Query));
    Query->ThermalStamp = Info->ThermalStamp;
    Query->ThermalConstant1 = Info->ThermalConstant1;
    Query->ThermalConstant2 = Info->ThermalConstant2;
    Query->SamplingPeriod = Info->SamplingPeriod;
    Query->CurrentTemperature = Info->CurrentTemperature;
    Query->PassiveTripPoint = Info->PassiveTripPoint;
    Query->CriticalTripPoint = Info->CriticalTripPoint;
    Query->ActiveTripPointCount = (UCHAR)Info->ActiveTripPointCount;

    for (Index = 0; Index < RTL_NUMBER_OF(Query->ActiveTripPoint); Index++)
        Query->ActiveTripPoint[Index] = Info->ActiveTripPoint[Index];

#if (NTDDI_VERSION >= NTDDI_WIN8)
    Query->S4TransitionTripPoint = Info->S4TripPoint;
#else
    Query->Processors = Info->Processors;
#endif

#if (NTDDI_VERSION >= NTDDI_WIN10)
    Query->ThermalStandbyTripPoint = Info->StandbyTripPoint;
    Query->MinimumThrottle = Info->MinimumThrottle;
    Query->OverThrottleThreshold = Info->OverThrottleThreshold;
#endif
}

static
ULONG
NTAPI
UacpiNtThermalIoControlCode(
    _In_ PIRP Irp)
{
    return IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode;
}

/* Completes every queued IRP with success, queries carry the zone data */
static
VOID
NTAPI
UacpiNtThermalCompleteIrps(
    _Inout_ PUACPINT_PDO Pdo)
{
    PUACPINT_THERMAL_ZONE Zone = &Pdo->Thermal;
    UACPINT_THERMAL_INFO Snapshot;
    LIST_ENTRY Completed;
    PLIST_ENTRY Entry;
    PLIST_ENTRY Next;
    BOOLEAN HadQuery = FALSE;
    ULONG_PTR Information;
    PIRP Irp;
    KIRQL OldIrql;

    InitializeListHead(&Completed);

    KeAcquireSpinLock(&Zone->Lock, &OldIrql);

    for (Entry = Zone->IrpQueue.Flink; Entry != &Zone->IrpQueue; Entry = Next)
    {
        Next = Entry->Flink;
        Irp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);

        if (UacpiNtThermalIoControlCode(Irp) == IOCTL_THERMAL_QUERY_INFORMATION)
        {
            /* Its cancel routine already owns it */
            if (!IoSetCancelRoutine(Irp, NULL))
                continue;

            HadQuery = TRUE;
        }

        RemoveEntryList(Entry);
        InsertTailList(&Completed, Entry);
    }

    if (HadQuery)
        Zone->Delivered = TRUE;

    Snapshot = Zone->Info;
    KeReleaseSpinLock(&Zone->Lock, OldIrql);

    while (!IsListEmpty(&Completed))
    {
        Entry = RemoveHeadList(&Completed);
        Irp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);
        Information = 0;

        switch (UacpiNtThermalIoControlCode(Irp))
        {
            case IOCTL_THERMAL_QUERY_INFORMATION:
                UacpiNtThermalFillQuery(&Snapshot, Irp->AssociatedIrp.SystemBuffer);
                Information = sizeof(UACPINT_THERMAL_QUERY);
                break;

#if (NTDDI_VERSION >= NTDDI_WIN10)
            case IOCTL_UACPINT_THERMAL_DESCRIPTION:
                UacpiNtThermalCopyDescription(Zone, Irp->AssociatedIrp.SystemBuffer);
                Information = sizeof(UNICODE_STRING);
                break;
#endif

            default:
                break;
        }

        UacpiNtCompleteIrp(Irp, STATUS_SUCCESS, Information);
    }
}

static
VOID
NTAPI
UacpiNtThermalWorker(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context)
{
    PUACPINT_PDO Pdo = DeviceObject->DeviceExtension;
    PUACPINT_THERMAL_ZONE Zone = &Pdo->Thermal;
    PIO_WORKITEM WorkItem = Context;
    UCHAR CoolingPolicy;
    UCHAR FanLevel;
    ULONG Work;
    KIRQL OldIrql;

    for (;;)
    {
        KeAcquireSpinLock(&Zone->Lock, &OldIrql);

        Work = Zone->Work;
        Zone->Work = 0;
        if (!Work)
        {
            Zone->Running = FALSE;
            KeReleaseSpinLock(&Zone->Lock, OldIrql);
            break;
        }

        CoolingPolicy = Zone->CoolingPolicy;
        FanLevel = Zone->FanLevel;
        KeReleaseSpinLock(&Zone->Lock, OldIrql);

#if (NTDDI_VERSION >= NTDDI_WIN8)
        /* Any step other than the reading itself is followed by a fresh reading */
        if (Work & (UACPINT_THERMAL_WORK_INIT | UACPINT_THERMAL_WORK_TRIPS | UACPINT_THERMAL_WORK_ACTIVE))
            Work |= UACPINT_THERMAL_WORK_TEMPERATURE;
#endif

        if (Work & UACPINT_THERMAL_WORK_INIT)
            UacpiNtThermalInit(Pdo);

        /* _SCP lets the platform reorder its trip points for the policy */
        if (Work & UACPINT_THERMAL_WORK_POLICY)
            UacpiNtThermalCallWithInteger(Pdo->Node, "_SCP", CoolingPolicy);

        if (Work & UACPINT_THERMAL_WORK_TRIPS)
            UacpiNtThermalReadTrips(Pdo);

        if (Work & UACPINT_THERMAL_WORK_ACTIVE)
            UacpiNtThermalRunActive(Pdo, FanLevel);

        if (Work & UACPINT_THERMAL_WORK_TEMPERATURE)
            UacpiNtThermalReadTemperature(Pdo);

        if (Work & UACPINT_THERMAL_WORK_COMPLETE)
            UacpiNtThermalCompleteIrps(Pdo);
    }

    if (WorkItem)
        IoFreeWorkItem(WorkItem);
}

/* Call with the zone lock held, TRUE means the caller must queue the worker */
static
BOOLEAN
NTAPI
UacpiNtThermalAddWork(
    _Inout_ PUACPINT_THERMAL_ZONE Zone,
    _In_ ULONG Work)
{
    /* The next completion pass has something new to report */
    if (Work & UACPINT_THERMAL_WORK_COMPLETE)
        Zone->Delivered = FALSE;

    Zone->Work |= Work;
    if (!Zone->Work || Zone->Running)
        return FALSE;

    Zone->Running = TRUE;
    return TRUE;
}

static
VOID
NTAPI
UacpiNtThermalQueueWorker(
    _Inout_ PUACPINT_PDO Pdo)
{
    PIO_WORKITEM WorkItem;
    KIRQL OldIrql;

    WorkItem = IoAllocateWorkItem(Pdo->Shared.Self);
    if (WorkItem)
    {
        IoQueueWorkItem(WorkItem, UacpiNtThermalWorker, DelayedWorkQueue, WorkItem);
        return;
    }

    /* The work stays pending and the next request queues the worker */
    KeAcquireSpinLock(&Pdo->Thermal.Lock, &OldIrql);
    Pdo->Thermal.Running = FALSE;
    KeReleaseSpinLock(&Pdo->Thermal.Lock, OldIrql);

    DPRINT1("uACPI-NT: %s thermal work item allocation failed\n", Pdo->Name);
}

static
VOID
NTAPI
UacpiNtThermalKick(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ ULONG Work)
{
    BOOLEAN QueueWorker = FALSE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Pdo->Thermal.Lock, &OldIrql);
    if (Pdo->Thermal.Active)
        QueueWorker = UacpiNtThermalAddWork(&Pdo->Thermal, Work);
    KeReleaseSpinLock(&Pdo->Thermal.Lock, OldIrql);

    if (QueueWorker)
        UacpiNtThermalQueueWorker(Pdo);
}

static
VOID
NTAPI
UacpiNtThermalCancel(
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PUACPINT_PDO Pdo = DeviceObject->DeviceExtension;
    KIRQL OldIrql;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    KeAcquireSpinLock(&Pdo->Thermal.Lock, &OldIrql);
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    KeReleaseSpinLock(&Pdo->Thermal.Lock, OldIrql);

    UacpiNtCompleteIrp(Irp, STATUS_CANCELLED, 0);
}

/* Input and output sizes per IOCTL, FALSE when the code is not ours on this release */
static
BOOLEAN
NTAPI
UacpiNtThermalRequestSizes(
    _In_ ULONG IoControlCode,
    _Out_ PULONG InputSize,
    _Out_ PULONG OutputSize)
{
    *InputSize = 0;
    *OutputSize = 0;

    switch (IoControlCode)
    {
        case IOCTL_THERMAL_QUERY_INFORMATION:
            *InputSize = sizeof(ULONG);
            *OutputSize = sizeof(UACPINT_THERMAL_QUERY);
            return TRUE;

        case IOCTL_THERMAL_SET_COOLING_POLICY:
        case IOCTL_RUN_ACTIVE_COOLING_METHOD:
#if (NTDDI_VERSION >= NTDDI_WIN8)
        case IOCTL_THERMAL_SET_PASSIVE_LIMIT:
#endif
            *InputSize = sizeof(UCHAR);
            return TRUE;

#if (NTDDI_VERSION >= NTDDI_WIN10)
        case IOCTL_UACPINT_THERMAL_DESCRIPTION:
            *OutputSize = sizeof(UNICODE_STRING);
            return TRUE;
#endif

        default:
            return FALSE;
    }
}

NTSTATUS
NTAPI
UacpiNtThermalDeviceControl(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp,
    _Out_ PBOOLEAN Handled)
{
    PUACPINT_THERMAL_ZONE Zone = &Pdo->Thermal;
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG IoControlCode = IoStack->Parameters.DeviceIoControl.IoControlCode;
    ULONG InputLength = IoStack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG OutputLength = IoStack->Parameters.DeviceIoControl.OutputBufferLength;
    PVOID Buffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG InputSize;
    ULONG OutputSize;
    ULONG Work = 0;
    BOOLEAN QueueWorker;
    KIRQL OldIrql;

    *Handled = UacpiNtThermalRequestSizes(IoControlCode, &InputSize, &OutputSize);
    if (!*Handled)
        return STATUS_NOT_SUPPORTED;

    /* The kernel thermal manager is the only client */
    if (Irp->RequestorMode != KernelMode)
        return UacpiNtCompleteIrp(Irp, STATUS_NOT_IMPLEMENTED, 0);

    if (IoControlCode == IOCTL_THERMAL_QUERY_INFORMATION &&
        OutputLength != OutputSize &&
        !InterlockedExchange(&UacpiNtThermalQuerySizeReported, 1))
    {
        DPRINT("uACPI-NT: thermal query buffer is %lu bytes, this build returns %lu\n",
               OutputLength,
               OutputSize);
    }

    if ((!Buffer && (InputSize || OutputSize)) || InputLength < InputSize || OutputLength < OutputSize)
        return UacpiNtCompleteIrp(Irp, STATUS_BUFFER_TOO_SMALL, 0);

    KeAcquireSpinLock(&Zone->Lock, &OldIrql);

    if (!Zone->Active)
    {
        KeReleaseSpinLock(&Zone->Lock, OldIrql);
        return UacpiNtCompleteIrp(Irp, STATUS_NO_SUCH_DEVICE, 0);
    }

    switch (IoControlCode)
    {
        case IOCTL_THERMAL_QUERY_INFORMATION:
            /* A stale stamp wants a new reading, a current one waits for news */
            if (*(PULONG)Buffer != Zone->Info.ThermalStamp)
                Work = UACPINT_THERMAL_WORK_TEMPERATURE | UACPINT_THERMAL_WORK_COMPLETE;
            else if (!Zone->Delivered)
                Work = UACPINT_THERMAL_WORK_COMPLETE;

            IoSetCancelRoutine(Irp, UacpiNtThermalCancel);
            if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL))
            {
                KeReleaseSpinLock(&Zone->Lock, OldIrql);
                return UacpiNtCompleteIrp(Irp, STATUS_CANCELLED, 0);
            }
            break;

        case IOCTL_THERMAL_SET_COOLING_POLICY:
            Zone->CoolingPolicy = *(PUCHAR)Buffer;
            Work = UACPINT_THERMAL_WORK_POLICY | UACPINT_THERMAL_WORK_TRIPS | UACPINT_THERMAL_WORK_COMPLETE;
            DPRINT("uACPI-NT: %s cooling policy %u\n", Pdo->Name, Zone->CoolingPolicy);
            break;

        case IOCTL_RUN_ACTIVE_COOLING_METHOD:
            Zone->FanLevel = *(PUCHAR)Buffer;
            Work = UACPINT_THERMAL_WORK_ACTIVE | UACPINT_THERMAL_WORK_COMPLETE;
            DPRINT("uACPI-NT: %s active cooling level %u\n", Pdo->Name, Zone->FanLevel);
            break;

#if (NTDDI_VERSION >= NTDDI_WIN8)
        /* Only recorded, nothing drives a thermal cooling interface yet */
        case IOCTL_THERMAL_SET_PASSIVE_LIMIT:
            Zone->ThrottleLimit = *(PUCHAR)Buffer;
            Work = UACPINT_THERMAL_WORK_TEMPERATURE | UACPINT_THERMAL_WORK_COMPLETE;
            DPRINT("uACPI-NT: %s passive limit %u%%\n", Pdo->Name, Zone->ThrottleLimit);
            break;
#endif

        /* The description */
        default:
            Work = UACPINT_THERMAL_WORK_COMPLETE;
            break;
    }

    IoMarkIrpPending(Irp);
    InsertTailList(&Zone->IrpQueue, &Irp->Tail.Overlay.ListEntry);
    QueueWorker = UacpiNtThermalAddWork(Zone, Work);
    KeReleaseSpinLock(&Zone->Lock, OldIrql);

    if (QueueWorker)
        UacpiNtThermalQueueWorker(Pdo);

    return STATUS_PENDING;
}

VOID
NTAPI
UacpiNtThermalNotify(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ ULONG NotifyValue)
{
    switch (NotifyValue)
    {
        case UACPINT_NOTIFY_TEMPERATURE:
            UacpiNtThermalKick(Pdo, UACPINT_THERMAL_WORK_TEMPERATURE | UACPINT_THERMAL_WORK_COMPLETE);
            break;

        case UACPINT_NOTIFY_TRIP_POINTS:
            UacpiNtThermalKick(Pdo,
                               UACPINT_THERMAL_WORK_TRIPS |
                               UACPINT_THERMAL_WORK_TEMPERATURE |
                               UACPINT_THERMAL_WORK_COMPLETE);
            break;

        default:
            break;
    }
}

/* Every _ALx device runs (level 0) until the kernel picks a level */
static
VOID
NTAPI
UacpiNtThermalZoneStart(
    _Inout_ PUACPINT_PDO Pdo)
{
    PUACPINT_THERMAL_ZONE Zone = &Pdo->Thermal;
    BOOLEAN QueueWorker = FALSE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Zone->Lock, &OldIrql);
    if (!Zone->Active)
    {
        Zone->Active = TRUE;
        Zone->CoolingPolicy = PASSIVE_COOLING;
        Zone->FanLevel = 0;
        Zone->ThrottleLimit = 100;
        QueueWorker = UacpiNtThermalAddWork(Zone, UACPINT_THERMAL_WORK_ALL);
    }
    KeReleaseSpinLock(&Zone->Lock, OldIrql);

    if (QueueWorker)
        UacpiNtThermalQueueWorker(Pdo);
}

/* A stopped zone takes no more IOCTLs and completes what it holds with success */
static
VOID
NTAPI
UacpiNtThermalZoneStop(
    _Inout_ PUACPINT_PDO Pdo)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&Pdo->Thermal.Lock, &OldIrql);
    Pdo->Thermal.Active = FALSE;
    Pdo->Thermal.Work = 0;
    KeReleaseSpinLock(&Pdo->Thermal.Lock, OldIrql);

    UacpiNtThermalCompleteIrps(Pdo);
}

static
NTSTATUS
NTAPI
UacpiNtThermalQueryWmiRegInfo(
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PULONG RegFlags,
    _Inout_ PUNICODE_STRING InstanceName,
    _Outptr_result_maybenull_ PUNICODE_STRING *RegistryPath,
    _Inout_ PUNICODE_STRING MofResourceName,
    _Outptr_result_maybenull_ PDEVICE_OBJECT *Pdo)
{
    UNREFERENCED_PARAMETER(InstanceName);
    UNREFERENCED_PARAMETER(MofResourceName);

    /* WmiLib leaves RegistryPath untouched, it must be set either way */
    *RegistryPath = GlobalDriverRegPath.Buffer ? &GlobalDriverRegPath : NULL;

    /* Instance names come from the PDO instance path */
    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *Pdo = DeviceObject;
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
UacpiNtThermalQueryWmiDataBlock(
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _In_ ULONG GuidIndex,
    _In_ ULONG InstanceIndex,
    _In_ ULONG InstanceCount,
    _Out_writes_opt_(InstanceCount) PULONG InstanceLengthArray,
    _In_ ULONG BufferAvail,
    _Out_writes_bytes_opt_(BufferAvail) PUCHAR Buffer)
{
    PUACPINT_PDO Pdo = DeviceObject->DeviceExtension;
    uacpi_namespace_node *Node = Pdo->Node;
    PUACPINT_THERMAL_WMI Data;

    UNREFERENCED_PARAMETER(InstanceIndex);
    UNREFERENCED_PARAMETER(InstanceCount);

    if (GuidIndex != 0)
        return WmiCompleteRequest(DeviceObject, Irp, STATUS_WMI_GUID_NOT_FOUND, 0, IO_NO_INCREMENT);

    if (BufferAvail < sizeof(*Data) || !InstanceLengthArray || !Buffer)
        return WmiCompleteRequest(DeviceObject, Irp, STATUS_BUFFER_TOO_SMALL, 0, IO_NO_INCREMENT);

    /* A fresh evaluation for each query, independent of the IOCTL worker */
    Data = (PUACPINT_THERMAL_WMI)Buffer;
    RtlZeroMemory(Data, sizeof(*Data));
    Data->ThermalStamp = (ULONG)InterlockedIncrement(&UacpiNtThermalWmiStamp);
    Data->ThermalConstant1 = UacpiNtThermalEvalDefault(Node, "_TC1", 0);
    Data->ThermalConstant2 = UacpiNtThermalEvalDefault(Node, "_TC2", 0);
    Data->SamplingPeriod = UacpiNtThermalEvalDefault(Node, "_TSP", 0);
    Data->CurrentTemperature = UacpiNtThermalEvalDefault(Node, "_TMP", UACPINT_THERMAL_WMI_DEFAULT_TEMP);
    Data->PassiveTripPoint = UacpiNtThermalEvalDefault(Node, "_PSV", 0);
    Data->CriticalTripPoint = UacpiNtThermalEvalDefault(Node, "_CRT", 0);
    Data->ActiveTripPointCount = UacpiNtThermalReadActiveTrips(Node, Data->ActiveTripPoint);

    InstanceLengthArray[0] = sizeof(*Data);
    return WmiCompleteRequest(DeviceObject, Irp, STATUS_SUCCESS, sizeof(*Data), IO_NO_INCREMENT);
}

NTSTATUS
NTAPI
UacpiNtThermalSystemControl(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp,
    _Out_ PBOOLEAN Handled)
{
    SYSCTL_IRP_DISPOSITION Disposition;
    NTSTATUS Status;

    *Handled = FALSE;

    if (!Pdo->ThermalWmi)
        return STATUS_NOT_SUPPORTED;

    *Handled = TRUE;

    Status = WmiSystemControl(Pdo->ThermalWmi, Pdo->Shared.Self, Irp, &Disposition);
    switch (Disposition)
    {
        /* WmiLib completed it */
        case IrpProcessed:
            break;

        case IrpNotCompleted:
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        /* Forwarding is not possible, a PDO is the bottom of the stack */
        default:
            Status = Irp->IoStatus.Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
    }

    return Status;
}

static
VOID
NTAPI
UacpiNtThermalWmiRegister(
    _Inout_ PUACPINT_PDO Pdo)
{
    PWMILIB_CONTEXT WmiContext;

    if (Pdo->ThermalWmi)
        return;

    WmiContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(*WmiContext), UACPINT_POOL_TAG);
    if (!WmiContext)
        return;

    RtlZeroMemory(WmiContext, sizeof(*WmiContext));
    WmiContext->GuidCount = RTL_NUMBER_OF(UacpiNtThermalWmiGuids);
    WmiContext->GuidList = UacpiNtThermalWmiGuids;
    WmiContext->QueryWmiRegInfo = UacpiNtThermalQueryWmiRegInfo;
    WmiContext->QueryWmiDataBlock = UacpiNtThermalQueryWmiDataBlock;
    Pdo->ThermalWmi = WmiContext;

    IoWMIRegistrationControl(Pdo->Shared.Self, WMIREG_ACTION_REGISTER);
    DPRINT("uACPI-NT: %s WMI temperature provider up\n", Pdo->Name);
}

VOID
NTAPI
UacpiNtThermalStart(
    _Inout_ PUACPINT_PDO Pdo)
{
    if (!Pdo->IsThermalZone)
        return;

    UacpiNtDevIfRegister(Pdo, &GUID_DEVICE_THERMAL_ZONE, "thermal");
    UacpiNtThermalWmiRegister(Pdo);
    UacpiNtThermalZoneStart(Pdo);
}

VOID
NTAPI
UacpiNtThermalRemove(
    _Inout_ PUACPINT_PDO Pdo)
{
    if (!Pdo->IsThermalZone)
        return;

    UacpiNtThermalZoneStop(Pdo);

    if (Pdo->ThermalWmi)
    {
        IoWMIRegistrationControl(Pdo->Shared.Self, WMIREG_ACTION_DEREGISTER);
        ExFreePoolWithTag(Pdo->ThermalWmi, UACPINT_POOL_TAG);
        Pdo->ThermalWmi = NULL;
    }
}
