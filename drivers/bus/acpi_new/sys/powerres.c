/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Reference counted ACPI power resources
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <debug.h>

/* Built once at init, the mutex serializes _ON, _OFF and the refcounts */
static LIST_ENTRY PowerResourceList;
static KMUTEX PowerResourceMutex;
static BOOLEAN PowerResourcesReady;

static
VOID
NTAPI
UacpiNtPowerResTrace(
    _In_z_ const char *What,
    _In_ uacpi_namespace_node *Node)
{
    const uacpi_char *Path;

    Path = uacpi_namespace_node_generate_absolute_path(Node);
    DPRINT("uACPI-NT: power resource %s %s\n", What, Path ? Path : "?");

    if (Path)
        uacpi_free_absolute_path(Path);
}

static
uacpi_iteration_decision
UacpiNtPowerResCollect(
    _In_opt_ void *User,
    _In_ uacpi_namespace_node *Node,
    _In_ uacpi_u32 Depth)
{
    uacpi_power_resource_info Info;
    PUACPINT_POWER_RESOURCE Resource;
    uacpi_object *Object = NULL;
    uacpi_u64 Sta = 0;

    UNREFERENCED_PARAMETER(User);
    UNREFERENCED_PARAMETER(Depth);

    /* Evaluating the node hands back a copy of the PowerResource object */
    if (uacpi_unlikely_error(uacpi_eval_simple_typed(Node, NULL, UACPI_OBJECT_POWER_RESOURCE_BIT, &Object)) ||
        !Object)
    {
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    /* Best effort, a missing entry only means that resource is never switched */
    Resource = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Resource), UACPINT_POOL_TAG);
    if (!Resource)
    {
        uacpi_object_unref(Object);
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    RtlZeroMemory(Resource, sizeof(*Resource));
    Resource->Node = Node;

    if (uacpi_likely_success(uacpi_object_get_power_resource_info(Object, &Info)))
    {
        Resource->SleepLevel = Info.system_level;
        Resource->PowerOnOrder = Info.resource_order;
    }

    uacpi_object_unref(Object);

    /* Without _STA assume off, running _ON again is harmless */
    if (uacpi_likely_success(uacpi_eval_simple_integer(Node, "_STA", &Sta)))
        Resource->On = (Sta != 0);

    InsertTailList(&PowerResourceList, &Resource->Link);
    UacpiNtPowerResTrace(Resource->On ? "found on" : "found off", Node);

    return UACPI_ITERATION_DECISION_CONTINUE;
}

VOID
NTAPI
UacpiNtPowerResInit(VOID)
{
    if (PowerResourcesReady)
        return;

    InitializeListHead(&PowerResourceList);
    KeInitializeMutex(&PowerResourceMutex, 0);

    uacpi_namespace_for_each_child(uacpi_namespace_root(),
                                   UacpiNtPowerResCollect,
                                   UACPI_NULL,
                                   UACPI_OBJECT_POWER_RESOURCE_BIT,
                                   UACPI_MAX_DEPTH_ANY,
                                   UACPI_NULL);

    PowerResourcesReady = TRUE;
}

static
VOID
NTAPI
UacpiNtPowerResLock(VOID)
{
    KeWaitForSingleObject(&PowerResourceMutex, Executive, KernelMode, FALSE, NULL);
}

static
VOID
NTAPI
UacpiNtPowerResUnlock(VOID)
{
    KeReleaseMutex(&PowerResourceMutex, FALSE);
}

static
PUACPINT_POWER_RESOURCE
NTAPI
UacpiNtPowerResFind(
    _In_ uacpi_namespace_node *Node)
{
    PUACPINT_POWER_RESOURCE Resource;
    PLIST_ENTRY Entry;

    for (Entry = PowerResourceList.Flink; Entry != &PowerResourceList; Entry = Entry->Flink)
    {
        Resource = CONTAINING_RECORD(Entry, UACPINT_POWER_RESOURCE, Link);
        if (Resource->Node == Node)
            return Resource;
    }

    return NULL;
}

static
BOOLEAN
NTAPI
UacpiNtPowerResInList(
    _In_ PUACPINT_POWER_RESOURCE Resource,
    _In_reads_(Count) PUACPINT_POWER_RESOURCE *List,
    _In_ ULONG Count)
{
    ULONG i;

    for (i = 0; i < Count; i++)
    {
        if (List[i] == Resource)
            return TRUE;
    }

    return FALSE;
}

/* Take a reference, the first one turns the resource on. Mutex held. */
static
VOID
NTAPI
UacpiNtPowerResReference(
    _Inout_ PUACPINT_POWER_RESOURCE Resource,
    _In_z_ const char *What)
{
    Resource->RefCount++;
    if (Resource->RefCount != 1 || Resource->On)
        return;

    if (uacpi_likely_success(uacpi_eval(Resource->Node, "_ON", UACPI_NULL, UACPI_NULL)))
    {
        Resource->On = TRUE;
        UacpiNtPowerResTrace(What, Resource->Node);
    }
}

/* Drop a reference, the last one turns the resource off. Mutex held. */
static
VOID
NTAPI
UacpiNtPowerResDereference(
    _Inout_ PUACPINT_POWER_RESOURCE Resource,
    _In_z_ const char *What)
{
    if (Resource->RefCount > 0)
        Resource->RefCount--;

    if (Resource->RefCount != 0 || !Resource->On)
        return;

    if (uacpi_likely_success(uacpi_eval(Resource->Node, "_OFF", UACPI_NULL, UACPI_NULL)))
    {
        Resource->On = FALSE;
        UacpiNtPowerResTrace(What, Resource->Node);
    }
}

/*
 * Map package elements from FirstIndex on to known power resources,
 * without duplicates and sorted by resource order.
 */
static
ULONG
NTAPI
UacpiNtPowerResResolve(
    _In_ uacpi_namespace_node *Scope,
    _In_ const uacpi_object_array *Package,
    _In_ ULONG FirstIndex,
    _In_z_ const char *What,
    _Out_writes_to_(MaxCount, return) PUACPINT_POWER_RESOURCE *List,
    _In_ ULONG MaxCount)
{
    PUACPINT_POWER_RESOURCE Resource;
    uacpi_namespace_node *Target;
    uacpi_data_view Text;
    ULONG Count = 0;
    ULONG i;
    ULONG j;

    for (i = FirstIndex; i < Package->count && Count < MaxCount; i++)
    {
        /* Names in a package stay unresolved strings until looked up from the device */
        Target = NULL;
        if (uacpi_object_is_aml_namepath(Package->objects[i]))
            uacpi_object_resolve_as_aml_namepath(Package->objects[i], Scope, &Target);
        else if (uacpi_likely_success(uacpi_object_get_string(Package->objects[i], &Text)))
            uacpi_namespace_node_resolve_from_aml_namepath(Scope, Text.text, &Target);

        Resource = Target ? UacpiNtPowerResFind(Target) : NULL;

        if (!Resource)
        {
            DPRINT("uACPI-NT: %s element %lu is not a power resource\n", What, i);
            continue;
        }

        if (!UacpiNtPowerResInList(Resource, List, Count))
            List[Count++] = Resource;
    }

    for (i = 1; i < Count; i++)
    {
        Resource = List[i];
        for (j = i; j > 0 && List[j - 1]->PowerOnOrder > Resource->PowerOnOrder; j--)
            List[j] = List[j - 1];

        List[j] = Resource;
    }

    return Count;
}

static
ULONG
NTAPI
UacpiNtPowerResEvalList(
    _In_opt_ uacpi_namespace_node *Device,
    _In_z_ const char *MethodName,
    _Out_writes_to_(MaxCount, return) PUACPINT_POWER_RESOURCE *List,
    _In_ ULONG MaxCount)
{
    uacpi_object_array Package;
    uacpi_object *Object = NULL;
    ULONG Count;

    if (!Device || !PowerResourcesReady)
        return 0;

    /* No _PRx means the device needs nothing for that state */
    if (uacpi_unlikely_error(uacpi_eval(Device, MethodName, UACPI_NULL, &Object)) || !Object)
        return 0;

    if (uacpi_unlikely_error(uacpi_object_get_package(Object, &Package)))
    {
        uacpi_object_unref(Object);
        return 0;
    }

    Count = UacpiNtPowerResResolve(Device, &Package, 0, MethodName, List, MaxCount);
    uacpi_object_unref(Object);
    return Count;
}

/* Before _PSx: reference the new state's resources this PDO does not hold yet */
VOID
NTAPI
UacpiNtPowerResAcquireForState(
    _In_ PUACPINT_PDO Pdo,
    _In_ DEVICE_POWER_STATE DeviceState,
    _Out_writes_to_(UACPINT_MAX_POWER_RESOURCES, *NewCount) PUACPINT_POWER_RESOURCE *NewList,
    _Out_ PULONG NewCount)
{
    static const char *MethodNames[] = { "_PR0", "_PR1", "_PR2", "_PR3" };
    ULONG Count;
    ULONG i;

    *NewCount = 0;

    if (DeviceState < PowerDeviceD0 || DeviceState > PowerDeviceD3 || !PowerResourcesReady)
        return;

    Count = UacpiNtPowerResEvalList(Pdo->Node,
                                    MethodNames[DeviceState - PowerDeviceD0],
                                    NewList,
                                    UACPINT_MAX_POWER_RESOURCES);
    *NewCount = Count;
    if (Count == 0)
        return;

    UacpiNtPowerResLock();
    for (i = 0; i < Count; i++)
    {
        if (!UacpiNtPowerResInList(NewList[i], Pdo->HeldResources, Pdo->HeldCount))
            UacpiNtPowerResReference(NewList[i], "_ON");
    }
    UacpiNtPowerResUnlock();
}

/* After _PSx: release what the new state does not need and record the new set */
VOID
NTAPI
UacpiNtPowerResReleaseDelta(
    _In_ PUACPINT_PDO Pdo,
    _In_reads_(NewCount) PUACPINT_POWER_RESOURCE *NewList,
    _In_ ULONG NewCount)
{
    ULONG i;

    if (!PowerResourcesReady)
        return;

    if (NewCount > UACPINT_MAX_POWER_RESOURCES)
        NewCount = UACPINT_MAX_POWER_RESOURCES;

    UacpiNtPowerResLock();

    /* Reverse resource order for _OFF */
    for (i = Pdo->HeldCount; i > 0; i--)
    {
        if (!UacpiNtPowerResInList(Pdo->HeldResources[i - 1], NewList, NewCount))
            UacpiNtPowerResDereference(Pdo->HeldResources[i - 1], "_OFF");
    }

    for (i = 0; i < NewCount; i++)
        Pdo->HeldResources[i] = NewList[i];

    Pdo->HeldCount = NewCount;

    UacpiNtPowerResUnlock();
}

/*
 * The _PRW[2] and later resources stay on while the device is armed for
 * wake, apart from the D state set, since a D3 device can still wake.
 */
ULONG
NTAPI
UacpiNtPowerResAcquireWake(
    _In_ uacpi_namespace_node *Device,
    _Out_writes_to_(MaxCount, return) PUACPINT_POWER_RESOURCE *List,
    _In_ ULONG MaxCount)
{
    uacpi_object_array Package;
    uacpi_object *Object = NULL;
    ULONG Count;
    ULONG i;

    if (!Device || !PowerResourcesReady)
        return 0;

    if (uacpi_unlikely_error(uacpi_eval(Device, "_PRW", UACPI_NULL, &Object)) || !Object)
        return 0;

    if (uacpi_unlikely_error(uacpi_object_get_package(Object, &Package)) || Package.count < 2)
    {
        uacpi_object_unref(Object);
        return 0;
    }

    Count = UacpiNtPowerResResolve(Device, &Package, 2, "_PRW", List, MaxCount);
    uacpi_object_unref(Object);
    if (Count == 0)
        return 0;

    UacpiNtPowerResLock();
    for (i = 0; i < Count; i++)
        UacpiNtPowerResReference(List[i], "wake _ON");
    UacpiNtPowerResUnlock();

    return Count;
}

VOID
NTAPI
UacpiNtPowerResReleaseWake(
    _In_reads_(Count) PUACPINT_POWER_RESOURCE *List,
    _In_ ULONG Count)
{
    ULONG i;

    if (Count == 0 || !PowerResourcesReady)
        return;

    UacpiNtPowerResLock();
    for (i = Count; i > 0; i--)
        UacpiNtPowerResDereference(List[i - 1], "wake _OFF");
    UacpiNtPowerResUnlock();
}

/* S4 loses power resources without any D3 IRPs, so turn the on ones back on */
VOID
NTAPI
UacpiNtPowerResResume(VOID)
{
    PUACPINT_POWER_RESOURCE Resource;
    PLIST_ENTRY Entry;

    if (!PowerResourcesReady)
        return;

    UacpiNtPowerResLock();
    for (Entry = PowerResourceList.Flink; Entry != &PowerResourceList; Entry = Entry->Flink)
    {
        Resource = CONTAINING_RECORD(Entry, UACPINT_POWER_RESOURCE, Link);
        if (!Resource->On)
            continue;

        if (uacpi_likely_success(uacpi_eval(Resource->Node, "_ON", UACPI_NULL, UACPI_NULL)))
            UacpiNtPowerResTrace("resume _ON", Resource->Node);
    }
    UacpiNtPowerResUnlock();
}
