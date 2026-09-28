/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Namespace enumeration into PDOs and device IDs
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <debug.h>

ULONG UacpiNtEnumDiagEnabled = 1;
ULONG UacpiNtEnumDiagDelaySeconds = 2;

/* Processor instance IDs are the ACPI processor ID, hex from Windows 8 on */
#if (NTDDI_VERSION >= NTDDI_WIN8)
#define UACPINT_PROCESSOR_INSTANCE_FORMAT "%2x"
#else
#define UACPINT_PROCESSOR_INSTANCE_FORMAT "%2d"
#endif

/* Settled inventory states, the work item must never be queued twice */
#define UACPINT_ENUMDIAG_IDLE   0
#define UACPINT_ENUMDIAG_ARMED  1
#define UACPINT_ENUMDIAG_QUEUED 2
#define UACPINT_ENUMDIAG_DONE   3

static KTIMER UacpiNtEnumDiagTimer;
static KDPC UacpiNtEnumDiagDpc;
static WORK_QUEUE_ITEM UacpiNtEnumDiagWork;
static LONG UacpiNtEnumDiagState;
static PUACPINT_FDO UacpiNtEnumDiagFdo;

/* PnP frees ID buffers with ExFreePool */
static
PWSTR
NTAPI
UacpiNtAsciiToWide(
    _In_z_ PCSTR String)
{
    SIZE_T Length = strlen(String);
    PWSTR Wide;
    SIZE_T Index;

    Wide = ExAllocatePoolWithTag(PagedPool, (Length + 1) * sizeof(WCHAR), UACPINT_POOL_TAG);
    if (!Wide)
        return NULL;

    for (Index = 0; Index <= Length; Index++)
        Wide[Index] = (WCHAR)(UCHAR)String[Index];

    return Wide;
}

static
PWSTR
NTAPI
UacpiNtAsciiToMultiSz(
    _In_reads_(Count) PCSTR *Strings,
    _In_ ULONG Count)
{
    SIZE_T Chars = 1;
    PWSTR MultiSz;
    PWSTR Cursor;
    PCSTR Source;
    ULONG Index;

    for (Index = 0; Index < Count; Index++)
        Chars += strlen(Strings[Index]) + 1;

    MultiSz = ExAllocatePoolWithTag(PagedPool, Chars * sizeof(WCHAR), UACPINT_POOL_TAG);
    if (!MultiSz)
        return NULL;

    Cursor = MultiSz;
    for (Index = 0; Index < Count; Index++)
    {
        Source = Strings[Index];
        do
        {
            *Cursor++ = (WCHAR)(UCHAR)*Source;
        } while (*Source++ != ANSI_NULL);
    }

    *Cursor = UNICODE_NULL;
    return MultiSz;
}

static
BOOLEAN
NTAPI
UacpiNtNodeReadAdr(
    _In_ uacpi_namespace_node *Node,
    _Out_opt_ PULONG64 Adr)
{
    uacpi_u64 Value = 0;

    if (uacpi_unlikely_error(uacpi_eval_simple_integer(Node, "_ADR", &Value)))
        return FALSE;

    if (Adr)
        *Adr = Value;
    return TRUE;
}

/* Hid is left empty when the node has no _HID */
static
BOOLEAN
NTAPI
UacpiNtNodeReadHid(
    _In_ uacpi_namespace_node *Node,
    _Out_writes_z_(HidSize) PCHAR Hid,
    _In_ SIZE_T HidSize)
{
    uacpi_id_string *Id = NULL;

    Hid[0] = ANSI_NULL;

    if (uacpi_unlikely_error(uacpi_eval_hid(Node, &Id)) || !Id)
        return FALSE;

    RtlStringCbCopyA(Hid, HidSize, Id->value);
    uacpi_free_id_string(Id);
    return TRUE;
}

BOOLEAN
NTAPI
UacpiNtNodeIsPresent(
    _In_ uacpi_namespace_node *Node)
{
    /* A missing _STA reads back as all ones */
    uacpi_u32 Sta = UACPINT_STA_PRESENT | UACPINT_STA_FUNCTIONING;

    uacpi_eval_sta(Node, &Sta);
    return (Sta & UACPINT_STA_PRESENT) != 0;
}

/* PCI interrupt links never become devnodes, irqarb.c routes them */
static
BOOLEAN
NTAPI
UacpiNtHidIsExcluded(
    _In_z_ PCSTR Hid)
{
    return _stricmp(Hid, "PNP0C0F") == 0;
}

/* The processor container gets a PDO and its children are enumerated too */
static
BOOLEAN
NTAPI
UacpiNtHidIsContainer(
    _In_z_ PCSTR Hid)
{
    return _stricmp(Hid, "ACPI0010") == 0;
}

/* Evaluating a Processor() node hands back a copy of its object */
BOOLEAN
NTAPI
UacpiNtGetProcessorInfo(
    _In_ uacpi_namespace_node *Node,
    _Out_ uacpi_processor_info *Info)
{
    uacpi_object *Object = NULL;
    uacpi_status UacpiStatus;

    RtlZeroMemory(Info, sizeof(*Info));

    UacpiStatus = uacpi_eval_simple_typed(Node, NULL, UACPI_OBJECT_PROCESSOR_BIT, &Object);
    if (uacpi_unlikely_error(UacpiStatus) || !Object)
        return FALSE;

    UacpiStatus = uacpi_object_get_processor_info(Object, Info);
    uacpi_object_unref(Object);
    return uacpi_likely_success(UacpiStatus);
}

BOOLEAN
NTAPI
UacpiNtHidIsPciRoot(
    _In_z_ const char *Hid)
{
    return _stricmp(Hid, "PNP0A03") == 0 || _stricmp(Hid, "PNP0A08") == 0;
}

/* Must agree with UacpiNtBuildChildPdosForNode, resarb.c skips these nodes */
BOOLEAN
NTAPI
UacpiNtNodeWillBecomePdo(
    _In_ uacpi_namespace_node *Node)
{
    CHAR Hid[16];

    if (!UacpiNtNodeIsPresent(Node))
        return FALSE;

    if (!UacpiNtNodeReadHid(Node, Hid, sizeof(Hid)))
        return FALSE;

    return !UacpiNtHidIsExcluded(Hid);
}

/* Instance ID is the absolute path without separators */
static
VOID
NTAPI
UacpiNtPdoSetInstance(
    _Inout_ PUACPINT_PDO Pdo)
{
    uacpi_processor_info Info;
    const uacpi_char *Path;
    ULONG Out = 0;
    ULONG In;

    Path = uacpi_namespace_node_generate_absolute_path(Pdo->Node);
    if (Path)
    {
        for (In = 0; Path[In] != ANSI_NULL && Out < sizeof(Pdo->Instance) - 1; In++)
        {
            if (Path[In] != '\\' && Path[In] != '.')
                Pdo->Instance[Out++] = Path[In];
        }
        Pdo->Instance[Out] = ANSI_NULL;
        uacpi_free_absolute_path(Path);
    }
    else
    {
        RtlStringCbCopyA(Pdo->Instance, sizeof(Pdo->Instance), Pdo->Name);
    }

    if (!Pdo->IsProcessor)
        return;

    if (UacpiNtGetProcessorInfo(Pdo->Node, &Info))
    {
        RtlStringCbPrintfA(Pdo->Instance,
                           sizeof(Pdo->Instance),
                           UACPINT_PROCESSOR_INSTANCE_FORMAT,
                           Info.id);
    }
}

/* Creating a second PDO for the same node bugchecks 0xCA */
static
PUACPINT_PDO
NTAPI
UacpiNtGetOrCreatePdo(
    _In_ PUACPINT_FDO Fdo,
    _In_ uacpi_namespace_node *Node,
    _In_opt_ uacpi_namespace_node *ScopeNode,
    _In_ BOOLEAN ThermalZone)
{
    PDEVICE_OBJECT DeviceObject;
    PUACPINT_PDO Pdo;
    uacpi_object_name Name;
    uacpi_object_type Type;
    ULONG64 Adr;
    BOOLEAN Processor = FALSE;
    NTSTATUS Status;

    Pdo = UacpiNtFindPdoByNode(Fdo, Node);
    if (Pdo)
    {
        Pdo->Present = TRUE;
        return Pdo;
    }

    /* acpi.sys builds no processor device without an ID string */
    if (!ThermalZone &&
        uacpi_likely_success(uacpi_namespace_node_type(Node, &Type)) &&
        Type == UACPI_OBJECT_PROCESSOR)
    {
        if (UacpiNtProcessorString[0] == ANSI_NULL)
            return NULL;
        Processor = TRUE;
    }

    Status = IoCreateDevice(Fdo->Shared.Self->DriverObject,
                            sizeof(UACPINT_PDO),
                            NULL,
                            FILE_DEVICE_ACPI,
                            FILE_AUTOGENERATED_DEVICE_NAME | FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: IoCreateDevice for a PDO failed 0x%lx\n", Status);
        return NULL;
    }

    Pdo = DeviceObject->DeviceExtension;
    RtlZeroMemory(Pdo, sizeof(*Pdo));
    Pdo->Shared.Type = UacpiNtDevObjPdo;
    Pdo->Shared.Self = DeviceObject;
    Pdo->Parent = Fdo;
    Pdo->Node = Node;
    Pdo->ScopeNode = ScopeNode;
    Pdo->Present = TRUE;
    Pdo->IsThermalZone = ThermalZone;
    Pdo->IsProcessor = Processor;
    UacpiNtWakeInit(&Pdo->Wake, Node);
    KeInitializeSpinLock(&Pdo->Thermal.Lock);
    InitializeListHead(&Pdo->Thermal.IrpQueue);

    Name = uacpi_namespace_node_name(Node);
    RtlCopyMemory(Pdo->Name, Name.text, sizeof(Name.text));
    Pdo->Name[sizeof(Name.text)] = ANSI_NULL;

    UacpiNtPdoSetInstance(Pdo);

    /* Thermal zones have no _HID, thermal.sys binds to this fixed ID */
    if (ThermalZone)
        RtlStringCbCopyA(Pdo->Hid, sizeof(Pdo->Hid), "ThermalZone");
    else
        UacpiNtNodeReadHid(Node, Pdo->Hid, sizeof(Pdo->Hid));

    if (UacpiNtNodeReadAdr(Node, &Adr))
    {
        Pdo->HasAdr = TRUE;
        Pdo->Adr = Adr;
    }

    UacpiNtButtonClassify(Pdo);

    DeviceObject->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    ExAcquireFastMutex(&Fdo->ChildLock);
    InsertTailList(&Fdo->ChildList, &Pdo->Link);
    ExReleaseFastMutex(&Fdo->ChildLock);

    if (Pdo->HasAdr)
    {
        DPRINT("uACPI-NT: PDO %s HID %s ADR 0x%I64X\n",
               Pdo->Name,
               Pdo->Hid[0] ? Pdo->Hid : "(none)",
               Pdo->Adr);
    }
    else
    {
        DPRINT("uACPI-NT: PDO %s HID %s\n", Pdo->Name, Pdo->Hid[0] ? Pdo->Hid : "(none)");
    }

    return Pdo;
}

VOID
NTAPI
UacpiNtBuildChildPdosForNode(
    _In_ PUACPINT_FDO Fdo,
    _In_ uacpi_namespace_node *Parent)
{
    uacpi_namespace_node *Child = NULL;
    uacpi_object_type Type;
    PUACPINT_PDO Stale;
    CHAR Hid[16];

    if (!Parent)
        return;

    while (uacpi_likely_success(uacpi_namespace_node_next_typed(Parent,
                                                                &Child,
                                                                UACPI_OBJECT_DEVICE_BIT |
                                                                UACPI_OBJECT_THERMAL_ZONE_BIT |
                                                                UACPI_OBJECT_PROCESSOR_BIT)) &&
           Child)
    {
        if (uacpi_unlikely_error(uacpi_namespace_node_type(Child, &Type)))
            continue;

        if (Type == UACPI_OBJECT_THERMAL_ZONE)
        {
            UacpiNtGetOrCreatePdo(Fdo, Child, Parent, TRUE);
            continue;
        }

        if (!UacpiNtNodeIsPresent(Child))
        {
            Stale = UacpiNtFindPdoByNode(Fdo, Child);
            if (Stale)
                Stale->Present = FALSE;
            continue;
        }

        if (Type == UACPI_OBJECT_PROCESSOR)
        {
            UacpiNtGetOrCreatePdo(Fdo, Child, Parent, FALSE);
            continue;
        }

        /* _ADR only nodes are left for UacpiNtDetectFilterDevices */
        if (!UacpiNtNodeReadHid(Child, Hid, sizeof(Hid)) || UacpiNtHidIsExcluded(Hid))
            continue;

        UacpiNtGetOrCreatePdo(Fdo, Child, Parent, FALSE);

        if (UacpiNtHidIsContainer(Hid))
            UacpiNtBuildChildPdosForNode(Fdo, Child);
    }
}

static
BOOLEAN
NTAPI
UacpiNtPdoBelongsTo(
    _In_ PUACPINT_PDO Pdo,
    _In_opt_ uacpi_namespace_node *Parent,
    _In_opt_ uacpi_namespace_node *SecondParent)
{
    if (!Pdo->Present)
        return FALSE;

    return Pdo->ScopeNode == Parent || (SecondParent && Pdo->ScopeNode == SecondParent);
}

/* Appends present children not already listed, each with a reference */
NTSTATUS
NTAPI
UacpiNtMergeChildRelations(
    _In_ PUACPINT_FDO Fdo,
    _In_ uacpi_namespace_node *Parent,
    _In_opt_ uacpi_namespace_node *SecondParent,
    _In_ PIRP Irp)
{
    PDEVICE_RELATIONS Existing = (PDEVICE_RELATIONS)Irp->IoStatus.Information;
    PDEVICE_RELATIONS Relations;
    PUACPINT_PDO Pdo;
    PLIST_ENTRY Entry;
    ULONG ExistingCount = Existing ? Existing->Count : 0;
    ULONG Added = 0;
    ULONG Index;
    SIZE_T Size;

    ExAcquireFastMutex(&Fdo->ChildLock);

    for (Entry = Fdo->ChildList.Flink; Entry != &Fdo->ChildList; Entry = Entry->Flink)
    {
        Pdo = CONTAINING_RECORD(Entry, UACPINT_PDO, Link);
        if (UacpiNtPdoBelongsTo(Pdo, Parent, SecondParent))
            Added++;
    }

    Size = FIELD_OFFSET(DEVICE_RELATIONS, Objects) +
           (SIZE_T)(ExistingCount + Added) * sizeof(PDEVICE_OBJECT);
    if (Size < sizeof(*Relations))
        Size = sizeof(*Relations);

    Relations = ExAllocatePoolWithTag(PagedPool, Size, UACPINT_POOL_TAG);
    if (!Relations)
    {
        ExReleaseFastMutex(&Fdo->ChildLock);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (ExistingCount)
        RtlCopyMemory(Relations->Objects, Existing->Objects, ExistingCount * sizeof(PDEVICE_OBJECT));
    Relations->Count = ExistingCount;

    for (Entry = Fdo->ChildList.Flink; Entry != &Fdo->ChildList; Entry = Entry->Flink)
    {
        Pdo = CONTAINING_RECORD(Entry, UACPINT_PDO, Link);
        if (!UacpiNtPdoBelongsTo(Pdo, Parent, SecondParent))
            continue;

        for (Index = 0; Index < ExistingCount; Index++)
        {
            if (Relations->Objects[Index] == Pdo->Shared.Self)
                break;
        }
        if (Index < ExistingCount)
            continue;

        ObReferenceObject(Pdo->Shared.Self);
        Relations->Objects[Relations->Count++] = Pdo->Shared.Self;
        Pdo->Reported = TRUE;
    }

    ExReleaseFastMutex(&Fdo->ChildLock);

    if (Existing)
        ExFreePool(Existing);

    Irp->IoStatus.Information = (ULONG_PTR)Relations;
    return STATUS_SUCCESS;
}

static
PCSTR
NTAPI
UacpiNtEnumDiagVerdict(
    _In_ PUACPINT_FDO Fdo,
    _In_ uacpi_namespace_node *Node,
    _In_ uacpi_object_type Type,
    _In_ uacpi_u32 Sta,
    _In_ BOOLEAN HasHid,
    _In_z_ PCSTR Hid,
    _In_ BOOLEAN HasAdr)
{
    if (Type == UACPI_OBJECT_THERMAL_ZONE)
        return "thermal zone";

    if (UacpiNtFindPdoByNode(Fdo, Node))
        return "PDO";

    if (!(Sta & UACPINT_STA_PRESENT))
        return "skipped, _STA reports not present";

    if (!HasHid)
        return HasAdr ? "skipped, no _HID, addressed by its parent bus" : "skipped, no _HID and no _ADR";

    if (UacpiNtHidIsExcluded(Hid))
        return "skipped, _HID excluded from enumeration";

    if (UacpiNtHidIsContainer(Hid))
        return "container, gets a PDO and is enumerated through";

    return "pending, below a PCI root until its filter runs";
}

static
uacpi_iteration_decision
UacpiNtEnumDiagCallback(
    _In_ void *User,
    _In_ uacpi_namespace_node *Node,
    _In_ uacpi_u32 Depth)
{
    uacpi_u32 Sta = UACPINT_STA_PRESENT | UACPINT_STA_FUNCTIONING;
    uacpi_object_type Type;
    uacpi_status StaStatus;
    const uacpi_char *Path;
    PCSTR Verdict;
    CHAR Hid[16];
    BOOLEAN HasHid;
    BOOLEAN HasAdr;

    if (uacpi_unlikely_error(uacpi_namespace_node_type(Node, &Type)))
        return UACPI_ITERATION_DECISION_CONTINUE;

    if (Type != UACPI_OBJECT_DEVICE &&
        Type != UACPI_OBJECT_THERMAL_ZONE &&
        Type != UACPI_OBJECT_PROCESSOR)
    {
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    StaStatus = uacpi_eval_sta(Node, &Sta);
    HasHid = UacpiNtNodeReadHid(Node, Hid, sizeof(Hid));
    HasAdr = UacpiNtNodeReadAdr(Node, NULL);
    Verdict = UacpiNtEnumDiagVerdict(User, Node, Type, Sta, HasHid, Hid, HasAdr);

    Path = uacpi_namespace_node_generate_absolute_path(Node);
    DPRINT("uACPI-NT: ns %-28s depth %u HID %-9s ADR %s _STA %s0x%X: %s\n",
           Path ? Path : "?",
           Depth,
           HasHid ? Hid : "-",
           HasAdr ? "yes" : "-",
           uacpi_unlikely_error(StaStatus) ? "(absent) " : "",
           Sta,
           Verdict);
    if (Path)
        uacpi_free_absolute_path(Path);

    return UACPI_ITERATION_DECISION_CONTINUE;
}

VOID
NTAPI
UacpiNtEnumDiagDump(
    _In_ PUACPINT_FDO Fdo,
    _In_ BOOLEAN Settled)
{
    uacpi_namespace_node *Root;

    PAGED_CODE();

    if (!UacpiNtEnumDiagEnabled)
        return;

    Root = uacpi_namespace_root();
    if (!Root)
        return;

    DPRINT("uACPI-NT: namespace inventory (%s)\n",
           Settled ? "settled, final" : "first pass, PCI subtree not walked yet");
    uacpi_namespace_for_each_child_simple(Root, UacpiNtEnumDiagCallback, Fdo);
    DPRINT("uACPI-NT: end of namespace inventory\n");
}

static
VOID
NTAPI
UacpiNtEnumDiagWorker(
    _In_opt_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);

    if (UacpiNtEnumDiagFdo)
        UacpiNtEnumDiagDump(UacpiNtEnumDiagFdo, TRUE);

    InterlockedExchange(&UacpiNtEnumDiagState, UACPINT_ENUMDIAG_DONE);
}

static
VOID
NTAPI
UacpiNtEnumDiagTimerDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (InterlockedCompareExchange(&UacpiNtEnumDiagState,
                                   UACPINT_ENUMDIAG_QUEUED,
                                   UACPINT_ENUMDIAG_ARMED) != UACPINT_ENUMDIAG_ARMED)
    {
        return;
    }

    /* _STA runs AML, that needs PASSIVE_LEVEL */
    ExQueueWorkItem(&UacpiNtEnumDiagWork, DelayedWorkQueue);
}

/* Every enumeration pass pushes the settled inventory out again */
VOID
NTAPI
UacpiNtEnumDiagArm(
    _In_ PUACPINT_FDO Fdo)
{
    LARGE_INTEGER DueTime;
    LONG Previous;

    if (!UacpiNtEnumDiagEnabled || !Fdo)
        return;

    Previous = InterlockedCompareExchange(&UacpiNtEnumDiagState,
                                          UACPINT_ENUMDIAG_ARMED,
                                          UACPINT_ENUMDIAG_IDLE);
    if (Previous == UACPINT_ENUMDIAG_QUEUED || Previous == UACPINT_ENUMDIAG_DONE)
        return;

    UacpiNtEnumDiagFdo = Fdo;
    DueTime.QuadPart = -((LONGLONG)UacpiNtEnumDiagDelaySeconds * 10 * 1000 * 1000);

    if (Previous == UACPINT_ENUMDIAG_IDLE)
    {
        ExInitializeWorkItem(&UacpiNtEnumDiagWork, UacpiNtEnumDiagWorker, NULL);
        KeInitializeDpc(&UacpiNtEnumDiagDpc, UacpiNtEnumDiagTimerDpc, NULL);
        KeInitializeTimer(&UacpiNtEnumDiagTimer);

        DPRINT("uACPI-NT: settled inventory follows %lu second(s) after the last pass\n",
               UacpiNtEnumDiagDelaySeconds);
    }

    KeSetTimer(&UacpiNtEnumDiagTimer, DueTime, &UacpiNtEnumDiagDpc);
}

NTSTATUS
NTAPI
UacpiNtEnumerateNamespace(
    _In_ PUACPINT_FDO Fdo)
{
    uacpi_namespace_node *Sb;
    uacpi_namespace_node *Tz;

    PAGED_CODE();

    if (!Fdo->InterpreterReady)
        return STATUS_UNSUCCESSFUL;

    Sb = uacpi_namespace_get_predefined(UACPI_PREDEFINED_NAMESPACE_SB);
    if (!Sb)
    {
        DPRINT1("uACPI-NT: \\_SB not found\n");
        return STATUS_UNSUCCESSFUL;
    }
    Tz = uacpi_namespace_get_predefined(UACPI_PREDEFINED_NAMESPACE_TZ);

    UacpiNtBuildChildPdosForNode(Fdo, Sb);
    UacpiNtBuildChildPdosForNode(Fdo, Tz);

    UacpiNtEnumDiagDump(Fdo, FALSE);
    UacpiNtEnumDiagArm(Fdo);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
UacpiNtBuildBusRelations(
    _In_ PUACPINT_FDO Fdo,
    _In_ PIRP Irp)
{
    uacpi_namespace_node *Sb = uacpi_namespace_get_predefined(UACPI_PREDEFINED_NAMESPACE_SB);
    uacpi_namespace_node *Tz = uacpi_namespace_get_predefined(UACPI_PREDEFINED_NAMESPACE_TZ);
    ULONG Count = 0;
    NTSTATUS Status;

    UacpiNtBuildChildPdosForNode(Fdo, Sb);
    UacpiNtBuildChildPdosForNode(Fdo, Tz);

    Status = UacpiNtMergeChildRelations(Fdo, Sb, Tz, Irp);
    if (NT_SUCCESS(Status) && Irp->IoStatus.Information)
        Count = ((PDEVICE_RELATIONS)Irp->IoStatus.Information)->Count;

    DPRINT("uACPI-NT: BusRelations has %lu PDO(s)\n", Count);
    return Status;
}

/* Nodes without _HID get fixed IDs, ACPI\<node name> would match no INF */
static
PCSTR
NTAPI
UacpiNtNoHidDeviceId(
    _In_ PUACPINT_PDO Pdo)
{
    uacpi_object_type Type;

    if (Pdo->IsThermalZone)
        return "ACPI\\ThermalZone";

    if (Pdo->Node &&
        uacpi_likely_success(uacpi_namespace_node_type(Pdo->Node, &Type)) &&
        Type == UACPI_OBJECT_PROCESSOR)
    {
        return "ACPI\\Processor";
    }

    return NULL;
}

/* Legacy Processor() IDs. Spaces are kept, PnP turns them into '_' */
static
NTSTATUS
NTAPI
UacpiNtProcessorQueryId(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    CHAR Trimmed[sizeof(UacpiNtProcessorString)];
    CHAR HardwareIds[6][sizeof(UacpiNtProcessorString) + 8];
    PCSTR IdList[6];
    PCHAR Model;
    PCHAR Family;
    PWSTR Result;
    ULONG Index;
#if (NTDDI_VERSION >= NTDDI_WIN10)
    CHAR DeviceId[sizeof(UacpiNtProcessorString) + sizeof(UacpiNtProcessorBrand) + 8];
#endif

    switch (IoStack->Parameters.QueryId.IdType)
    {
        case BusQueryDeviceID:
#if (NTDDI_VERSION >= NTDDI_WIN10)
            if (UacpiNtProcessorBrand[0] != ANSI_NULL)
            {
                RtlStringCbPrintfA(DeviceId,
                                   sizeof(DeviceId),
                                   "ACPI\\%s - %s",
                                   UacpiNtProcessorString,
                                   UacpiNtProcessorBrand);
                Result = UacpiNtAsciiToWide(DeviceId);
                break;
            }
#endif
            RtlStringCbPrintfA(HardwareIds[0], sizeof(HardwareIds[0]), "ACPI\\%s", UacpiNtProcessorString);
            Result = UacpiNtAsciiToWide(HardwareIds[0]);
            break;

        case BusQueryHardwareIDs:
            /* Full string, then cut before "Model", then before "Family" */
            RtlStringCbCopyA(Trimmed, sizeof(Trimmed), UacpiNtProcessorString);
            Model = strstr(Trimmed, "Model");
            Family = strstr(Trimmed, "Family");
            if (!Model || !Family || Model == Trimmed || Family == Trimmed)
                return STATUS_UNSUCCESSFUL;

            for (Index = 0; Index < RTL_NUMBER_OF(HardwareIds); Index += 2)
            {
                if (Index == 2)
                    Model[-1] = ANSI_NULL;
                else if (Index == 4)
                    Family[-1] = ANSI_NULL;

                RtlStringCbPrintfA(HardwareIds[Index], sizeof(HardwareIds[Index]), "ACPI\\%s", Trimmed);
                RtlStringCbPrintfA(HardwareIds[Index + 1], sizeof(HardwareIds[Index + 1]), "*%s", Trimmed);
                IdList[Index] = HardwareIds[Index];
                IdList[Index + 1] = HardwareIds[Index + 1];
            }
            Result = UacpiNtAsciiToMultiSz(IdList, RTL_NUMBER_OF(IdList));
            break;

        case BusQueryCompatibleIDs:
            IdList[0] = "ACPI\\Processor";
            Result = UacpiNtAsciiToMultiSz(IdList, 1);
            break;

        case BusQueryInstanceID:
            Result = UacpiNtAsciiToWide(Pdo->Instance);
            break;

        default:
            return Irp->IoStatus.Status;
    }

    if (!Result)
        return STATUS_INSUFFICIENT_RESOURCES;

    Irp->IoStatus.Information = (ULONG_PTR)Result;
    return STATUS_SUCCESS;
}

/* *<HID>, then ACPI\<CID> and *<CID> for every _CID entry */
static
PWSTR
NTAPI
UacpiNtBuildCompatibleIds(
    _In_ PUACPINT_PDO Pdo)
{
    CHAR Ids[16][40];
    PCSTR IdList[16];
    uacpi_pnp_id_list *Cids = NULL;
    ULONG Count = 0;
    ULONG Index;

    if (Pdo->Hid[0])
    {
        RtlStringCbPrintfA(Ids[Count], sizeof(Ids[Count]), "*%s", Pdo->Hid);
        IdList[Count] = Ids[Count];
        Count++;
    }

    if (Pdo->Node && uacpi_likely_success(uacpi_eval_cid(Pdo->Node, &Cids)) && Cids)
    {
        for (Index = 0; Index < Cids->num_ids && Count < RTL_NUMBER_OF(Ids) - 1; Index++)
        {
            RtlStringCbPrintfA(Ids[Count], sizeof(Ids[Count]), "ACPI\\%s", Cids->ids[Index].value);
            IdList[Count] = Ids[Count];
            Count++;

            RtlStringCbPrintfA(Ids[Count], sizeof(Ids[Count]), "*%s", Cids->ids[Index].value);
            IdList[Count] = Ids[Count];
            Count++;
        }
        uacpi_free_pnp_id_list(Cids);
    }

    if (!Count)
        return NULL;

    return UacpiNtAsciiToMultiSz(IdList, Count);
}

NTSTATUS
NTAPI
UacpiNtPdoQueryId(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    CHAR Id[64];
    CHAR StarId[64];
    PCSTR IdList[2];
    PCSTR Fixed;
    PWSTR Result;

    if (Pdo->IsProcessor)
        return UacpiNtProcessorQueryId(Pdo, Irp);

    switch (IoStack->Parameters.QueryId.IdType)
    {
        case BusQueryDeviceID:
        case BusQueryHardwareIDs:
            Fixed = Pdo->Hid[0] ? NULL : UacpiNtNoHidDeviceId(Pdo);
            if (Pdo->Hid[0])
                RtlStringCbPrintfA(Id, sizeof(Id), "ACPI\\%s", Pdo->Hid);
            else if (Fixed)
                RtlStringCbCopyA(Id, sizeof(Id), Fixed);
            else
                RtlStringCbPrintfA(Id, sizeof(Id), "ACPI\\%s", Pdo->Name);

            if (IoStack->Parameters.QueryId.IdType == BusQueryDeviceID)
            {
                Result = UacpiNtAsciiToWide(Id);
                break;
            }

            IdList[0] = Id;
            if (Pdo->Hid[0])
            {
                RtlStringCbPrintfA(StarId, sizeof(StarId), "*%s", Pdo->Hid);
                IdList[1] = StarId;
                Result = UacpiNtAsciiToMultiSz(IdList, 2);
            }
            else
            {
                Result = UacpiNtAsciiToMultiSz(IdList, 1);
            }
            break;

        /* No compatible IDs at all is a valid answer */
        case BusQueryCompatibleIDs:
            Irp->IoStatus.Information = (ULONG_PTR)UacpiNtBuildCompatibleIds(Pdo);
            return STATUS_SUCCESS;

        case BusQueryInstanceID:
            Result = UacpiNtAsciiToWide(Pdo->Instance);
            break;

        default:
            return Irp->IoStatus.Status;
    }

    if (!Result)
        return STATUS_INSUFFICIENT_RESOURCES;

    Irp->IoStatus.Information = (ULONG_PTR)Result;
    return STATUS_SUCCESS;
}
