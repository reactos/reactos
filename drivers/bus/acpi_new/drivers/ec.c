/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Embedded controller (PNP0C09) address space, queries and PDO
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <uacpi/opregion.h>
#include <uacpi/tables.h>
#include <uacpi/acpi.h>
#include <debug.h>

/* EC commands and status bits, ACPI specification chapter 12 */
#define UACPINT_EC_COMMAND_READ     0x80
#define UACPINT_EC_COMMAND_WRITE    0x81
#define UACPINT_EC_COMMAND_QUERY    0x84
#define UACPINT_EC_STATUS_OBF       0x01
#define UACPINT_EC_STATUS_IBF       0x02
#define UACPINT_EC_STATUS_SCI_EVT   0x20

#define UACPINT_EC_SPACE_SIZE       256
#define UACPINT_EC_TIMEOUT_US       10000
#define UACPINT_EC_POLL_US          10

/* Callers built for acpi.sys hand in 16 bytes on 32 bit and 32 on 64 bit */
C_ASSERT(sizeof(UACPINT_EC_QUERY_REGISTRATION) == (sizeof(PVOID) == 8 ? 32 : 16));

/* Grown at PASSIVE_LEVEL during bring up, walked by lookups */
static PUACPINT_EC UacpiNtEcList;
static KSPIN_LOCK UacpiNtEcListLock;

static
UCHAR
NTAPI
UacpiNtEcReadStatus(
    _In_ PUACPINT_EC Ec)
{
    return READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)Ec->ControlPort);
}

static
VOID
NTAPI
UacpiNtEcWriteCommand(
    _In_ PUACPINT_EC Ec,
    _In_ UCHAR Command)
{
    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)Ec->ControlPort, Command);
}

static
UCHAR
NTAPI
UacpiNtEcReadData(
    _In_ PUACPINT_EC Ec)
{
    return READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)Ec->DataPort);
}

static
VOID
NTAPI
UacpiNtEcWriteData(
    _In_ PUACPINT_EC Ec,
    _In_ UCHAR Data)
{
    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)Ec->DataPort, Data);
}

/* Polls the status port until the masked bits match or the step times out */
static
BOOLEAN
NTAPI
UacpiNtEcWaitStatus(
    _In_ PUACPINT_EC Ec,
    _In_ UCHAR Mask,
    _In_ UCHAR Expected)
{
    ULONG Elapsed = 0;

    while ((UacpiNtEcReadStatus(Ec) & Mask) != Expected)
    {
        if (Elapsed >= UACPINT_EC_TIMEOUT_US)
            return FALSE;

        KeStallExecutionProcessor(UACPINT_EC_POLL_US);
        Elapsed += UACPINT_EC_POLL_US;
    }

    return TRUE;
}

static
BOOLEAN
NTAPI
UacpiNtEcWaitInputEmpty(
    _In_ PUACPINT_EC Ec)
{
    return UacpiNtEcWaitStatus(Ec, UACPINT_EC_STATUS_IBF, 0);
}

static
BOOLEAN
NTAPI
UacpiNtEcWaitOutputFull(
    _In_ PUACPINT_EC Ec)
{
    return UacpiNtEcWaitStatus(Ec, UACPINT_EC_STATUS_OBF, UACPINT_EC_STATUS_OBF);
}

/*
 * _GLK for accesses that come from outside the interpreter. uACPI already
 * holds the lock around field accesses that ask for it, and it does not nest.
 */
static
BOOLEAN
NTAPI
UacpiNtEcLockGlobal(
    _In_ PUACPINT_EC Ec,
    _Out_ uacpi_u32 *Sequence)
{
    *Sequence = 0;

    if (!Ec->GlobalLock)
        return TRUE;

    return uacpi_acquire_global_lock(0xFFFF, Sequence) == UACPI_STATUS_OK;
}

static
VOID
NTAPI
UacpiNtEcUnlockGlobal(
    _In_ PUACPINT_EC Ec,
    _In_ uacpi_u32 Sequence)
{
    if (Ec->GlobalLock)
        uacpi_release_global_lock(Sequence);
}

static
BOOLEAN
NTAPI
UacpiNtEcReadByte(
    _Inout_ PUACPINT_EC Ec,
    _In_ UCHAR Address,
    _Out_ PUCHAR Value)
{
    BOOLEAN Success;

    ExAcquireFastMutex(&Ec->Lock);

    Success = UacpiNtEcWaitInputEmpty(Ec);
    if (Success)
    {
        UacpiNtEcWriteCommand(Ec, UACPINT_EC_COMMAND_READ);
        Success = UacpiNtEcWaitInputEmpty(Ec);
    }

    if (Success)
    {
        UacpiNtEcWriteData(Ec, Address);
        Success = UacpiNtEcWaitOutputFull(Ec);
    }

    if (Success)
        *Value = UacpiNtEcReadData(Ec);

    ExReleaseFastMutex(&Ec->Lock);

    if (!Success)
        DPRINT("uACPI-NT: EC read of 0x%02x timed out\n", Address);

    return Success;
}

static
BOOLEAN
NTAPI
UacpiNtEcWriteByte(
    _Inout_ PUACPINT_EC Ec,
    _In_ UCHAR Address,
    _In_ UCHAR Value)
{
    BOOLEAN Success;

    ExAcquireFastMutex(&Ec->Lock);

    Success = UacpiNtEcWaitInputEmpty(Ec);
    if (Success)
    {
        UacpiNtEcWriteCommand(Ec, UACPINT_EC_COMMAND_WRITE);
        Success = UacpiNtEcWaitInputEmpty(Ec);
    }

    if (Success)
    {
        UacpiNtEcWriteData(Ec, Address);
        Success = UacpiNtEcWaitInputEmpty(Ec);
    }

    if (Success)
        UacpiNtEcWriteData(Ec, Value);

    ExReleaseFastMutex(&Ec->Lock);

    if (!Success)
        DPRINT("uACPI-NT: EC write of 0x%02x timed out\n", Address);

    return Success;
}

/* Runs one QUERY command, 0 means nothing was pending */
static
UCHAR
NTAPI
UacpiNtEcFetchQuery(
    _Inout_ PUACPINT_EC Ec)
{
    uacpi_u32 Sequence;
    UCHAR QueryCode = 0;

    /* Queries are our own transaction, nobody above holds _GLK */
    if (!UacpiNtEcLockGlobal(Ec, &Sequence))
        return 0;

    ExAcquireFastMutex(&Ec->Lock);

    if ((UacpiNtEcReadStatus(Ec) & UACPINT_EC_STATUS_SCI_EVT) &&
        UacpiNtEcWaitInputEmpty(Ec))
    {
        UacpiNtEcWriteCommand(Ec, UACPINT_EC_COMMAND_QUERY);
        if (UacpiNtEcWaitOutputFull(Ec))
            QueryCode = UacpiNtEcReadData(Ec);
    }

    ExReleaseFastMutex(&Ec->Lock);
    UacpiNtEcUnlockGlobal(Ec, Sequence);

    return QueryCode;
}

static
VOID
NTAPI
UacpiNtEcDispatchQuery(
    _In_ PUACPINT_EC Ec,
    _In_ UCHAR QueryCode)
{
    PUACPINT_EC_QUERY_ROUTINE Routine;
    PVOID Context;
    CHAR Method[8];
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ec->QueryLock, &OldIrql);
    Routine = Ec->QueryRoutine[QueryCode];
    Context = Ec->QueryContext[QueryCode];
    KeReleaseSpinLock(&Ec->QueryLock, OldIrql);

    /* A registered driver takes the code and _Qxx is skipped, like acpi.sys */
    if (Routine)
    {
        DPRINT("uACPI-NT: EC query 0x%02x goes to a driver\n", QueryCode);
        Routine(QueryCode, Context);
        return;
    }

    RtlStringCbPrintfA(Method, sizeof(Method), "_Q%02X", QueryCode);
    DPRINT("uACPI-NT: EC query 0x%02x runs %s\n", QueryCode, Method);

    if (Ec->Node)
        uacpi_eval(Ec->Node, Method, NULL, NULL);
}

static
VOID
NTAPI
UacpiNtEcDrainQueries(
    _Inout_ PUACPINT_EC Ec)
{
    ULONG Round;
    UCHAR QueryCode;

    /* Bounded so a stuck SCI_EVT cannot hold the worker forever */
    for (Round = 0; Round < UACPINT_EC_QUERY_CODES; Round++)
    {
        QueryCode = UacpiNtEcFetchQuery(Ec);
        if (!QueryCode)
            return;

        UacpiNtEcDispatchQuery(Ec, QueryCode);
    }

    DPRINT("uACPI-NT: EC query drain hit its limit\n");
}

static
VOID
NTAPI
UacpiNtEcQueryWorker(
    _In_ PVOID Parameter)
{
    PUACPINT_EC Ec = Parameter;

    /* Cleared first so a GPE arriving mid drain schedules another pass */
    InterlockedExchange(&Ec->WorkQueued, 0);
    UacpiNtEcDrainQueries(Ec);
}

/* The SCI cannot queue a work item itself, so it goes through a DPC */
static
VOID
NTAPI
UacpiNtEcQueryDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PUACPINT_EC Ec = DeferredContext;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (!Ec)
        return;

    /* Deprecated, but the driver never unloads */
    ExQueueWorkItem(&Ec->Work, DelayedWorkQueue);
}

static
uacpi_interrupt_ret
UacpiNtEcGpeHandler(
    _In_ uacpi_handle Context,
    _In_opt_ uacpi_namespace_node *GpeDevice,
    _In_ uacpi_u16 GpeLine)
{
    PUACPINT_EC Ec = Context;

    UNREFERENCED_PARAMETER(GpeDevice);
    UNREFERENCED_PARAMETER(GpeLine);

    if (!InterlockedExchange(&Ec->WorkQueued, 1))
        KeInsertQueueDpc(&Ec->Dpc, NULL, NULL);

    return UACPI_GPE_REENABLE;
}

/* The EC is byte addressed, wider fields are assembled little endian */
static
uacpi_status
UacpiNtEcRegionRead(
    _Inout_ uacpi_region_rw_data *Data)
{
    PUACPINT_EC Ec = Data->handler_context;
    uacpi_u8 Width = Data->byte_width ? Data->byte_width : 1;
    uacpi_u8 Index;
    UCHAR Value;

    Data->value = 0;
    for (Index = 0; Index < Width; Index++)
    {
        if (Data->offset + Index >= UACPINT_EC_SPACE_SIZE)
            return UACPI_STATUS_AML_BAD_ENCODING;

        if (!UacpiNtEcReadByte(Ec, (UCHAR)(Data->offset + Index), &Value))
            return UACPI_STATUS_AML_BAD_ENCODING;

        Data->value |= (uacpi_u64)Value << (Index * 8);
    }

    return UACPI_STATUS_OK;
}

static
uacpi_status
UacpiNtEcRegionWrite(
    _In_ uacpi_region_rw_data *Data)
{
    PUACPINT_EC Ec = Data->handler_context;
    uacpi_u8 Width = Data->byte_width ? Data->byte_width : 1;
    uacpi_u8 Index;

    for (Index = 0; Index < Width; Index++)
    {
        if (Data->offset + Index >= UACPINT_EC_SPACE_SIZE)
            return UACPI_STATUS_AML_BAD_ENCODING;

        if (!UacpiNtEcWriteByte(Ec,
                                (UCHAR)(Data->offset + Index),
                                (UCHAR)(Data->value >> (Index * 8))))
        {
            return UACPI_STATUS_AML_BAD_ENCODING;
        }
    }

    return UACPI_STATUS_OK;
}

static
uacpi_status
UacpiNtEcRegionHandler(
    _In_ uacpi_region_op Operation,
    _Inout_ uacpi_handle OperationData)
{
    uacpi_region_attach_data *Attach;

    switch (Operation)
    {
        case UACPI_REGION_OP_ATTACH:
            Attach = OperationData;
            Attach->out_region_context = Attach->handler_context;
            return UACPI_STATUS_OK;

        case UACPI_REGION_OP_DETACH:
            return UACPI_STATUS_OK;

        case UACPI_REGION_OP_READ:
            return UacpiNtEcRegionRead(OperationData);

        case UACPI_REGION_OP_WRITE:
            return UacpiNtEcRegionWrite(OperationData);

        default:
            return UACPI_STATUS_UNIMPLEMENTED;
    }
}

/* _CRS lists the data port first and the command/status port second */
static
BOOLEAN
NTAPI
UacpiNtEcReadPorts(
    _In_ uacpi_namespace_node *Node,
    _Inout_ PUACPINT_EC Ec)
{
    uacpi_resources *Resources = NULL;
    uacpi_resource *Resource;
    USHORT Ports[2];
    ULONG Found = 0;

    if (uacpi_unlikely_error(uacpi_get_current_resources(Node, &Resources)) || !Resources)
        return FALSE;

    Resource = Resources->entries;
    while (Resource->type != UACPI_RESOURCE_TYPE_END_TAG && Found < RTL_NUMBER_OF(Ports))
    {
        if (Resource->type == UACPI_RESOURCE_TYPE_IO)
            Ports[Found++] = Resource->io.minimum;
        else if (Resource->type == UACPI_RESOURCE_TYPE_FIXED_IO)
            Ports[Found++] = Resource->fixed_io.address;

        Resource = UACPI_NEXT_RESOURCE(Resource);
    }

    uacpi_free_resources(Resources);

    if (Found < RTL_NUMBER_OF(Ports))
        return FALSE;

    Ec->DataPort = Ports[0];
    Ec->ControlPort = Ports[1];
    return TRUE;
}

/* _GPE is either a \_GPE index or a {block device, index} package */
static
BOOLEAN
NTAPI
UacpiNtEcReadGpe(
    _In_ uacpi_namespace_node *Node,
    _Inout_ PUACPINT_EC Ec)
{
    uacpi_object *Result = NULL;
    uacpi_object_array Package;
    uacpi_object *IndexObject = NULL;
    uacpi_u64 Index = 0;
    BOOLEAN Success = FALSE;

    if (uacpi_unlikely_error(uacpi_eval(Node, "_GPE", NULL, &Result)) || !Result)
    {
        DPRINT1("uACPI-NT: EC has no _GPE, queries will not be delivered\n");
        return FALSE;
    }

    switch (uacpi_object_get_type(Result))
    {
        case UACPI_OBJECT_INTEGER:
            IndexObject = Result;
            break;

        /* The block device in element 0 is ignored, the index is used as a \_GPE bit */
        case UACPI_OBJECT_PACKAGE:
            if (uacpi_likely_success(uacpi_object_get_package(Result, &Package)) &&
                Package.count >= 2)
            {
                IndexObject = Package.objects[1];
            }
            break;

        default:
            break;
    }

    if (IndexObject && uacpi_likely_success(uacpi_object_get_integer(IndexObject, &Index)))
    {
        Ec->GpeDevice = NULL;
        Ec->GpeLine = (uacpi_u16)Index;
        Success = TRUE;
    }

    uacpi_object_unref(Result);
    return Success;
}

static
VOID
NTAPI
UacpiNtEcReadGlk(
    _In_ uacpi_namespace_node *Node,
    _Inout_ PUACPINT_EC Ec)
{
    uacpi_u64 Glk = 0;

    if (uacpi_likely_success(uacpi_eval_simple_integer(Node, "_GLK", &Glk)) && Glk)
    {
        Ec->GlobalLock = TRUE;
        DPRINT("uACPI-NT: EC requires the global lock\n");
    }
}

static
PUACPINT_EC
NTAPI
UacpiNtEcAllocate(
    _In_ uacpi_namespace_node *Node)
{
    PUACPINT_EC Ec;

    Ec = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Ec), UACPINT_POOL_TAG);
    if (!Ec)
        return NULL;

    RtlZeroMemory(Ec, sizeof(*Ec));
    Ec->Node = Node;
    ExInitializeFastMutex(&Ec->Lock);
    KeInitializeSpinLock(&Ec->QueryLock);
    ExInitializeWorkItem(&Ec->Work, UacpiNtEcQueryWorker, Ec);
    KeInitializeDpc(&Ec->Dpc, UacpiNtEcQueryDpc, Ec);

    return Ec;
}

static
PUACPINT_EC
NTAPI
UacpiNtEcFindByNode(
    _In_ uacpi_namespace_node *Node)
{
    PUACPINT_EC Ec;
    KIRQL OldIrql;

    KeAcquireSpinLock(&UacpiNtEcListLock, &OldIrql);

    Ec = UacpiNtEcList;
    while (Ec && Ec->Node != Node)
        Ec = Ec->Next;

    KeReleaseSpinLock(&UacpiNtEcListLock, OldIrql);
    return Ec;
}

static
VOID
NTAPI
UacpiNtEcConnectGpe(
    _Inout_ PUACPINT_EC Ec)
{
    uacpi_status UacpiStatus;

    UacpiStatus = uacpi_install_gpe_handler(Ec->GpeDevice,
                                            Ec->GpeLine,
                                            UACPI_GPE_TRIGGERING_EDGE,
                                            UacpiNtEcGpeHandler,
                                            Ec);
    if (uacpi_unlikely_error(UacpiStatus))
    {
        DPRINT1("uACPI-NT: EC GPE 0x%02x handler install failed\n", Ec->GpeLine);
        return;
    }

    uacpi_enable_gpe(Ec->GpeDevice, Ec->GpeLine);
    DPRINT("uACPI-NT: EC query GPE 0x%02x connected\n", Ec->GpeLine);

    /* Something may have been pending since boot */
    UacpiNtEcDrainQueries(Ec);
}

/* Ports must be set. Without a GPE the EC still serves AML, it just raises no events */
static
BOOLEAN
NTAPI
UacpiNtEcInstall(
    _Inout_ PUACPINT_EC Ec,
    _In_ BOOLEAN HasGpe)
{
    uacpi_status UacpiStatus;
    KIRQL OldIrql;

    UacpiStatus = uacpi_install_address_space_handler(Ec->Node,
                                                      UACPI_ADDRESS_SPACE_EMBEDDED_CONTROLLER,
                                                      UacpiNtEcRegionHandler,
                                                      Ec);
    if (uacpi_unlikely_error(UacpiStatus))
    {
        DPRINT1("uACPI-NT: EC address space handler install failed\n");
        return FALSE;
    }

    Ec->RegionInstalled = TRUE;

    KeAcquireSpinLock(&UacpiNtEcListLock, &OldIrql);
    Ec->Next = UacpiNtEcList;
    UacpiNtEcList = Ec;
    KeReleaseSpinLock(&UacpiNtEcListLock, OldIrql);

    DPRINT("uACPI-NT: EC up, data 0x%x command 0x%x%s\n",
           Ec->DataPort,
           Ec->ControlPort,
           Ec->FromEcdt ? " from ECDT" : "");

    if (HasGpe)
        UacpiNtEcConnectGpe(Ec);

    return TRUE;
}

BOOLEAN
NTAPI
UacpiNtEcIsEcNode(
    _In_ uacpi_namespace_node *Node)
{
    static const uacpi_char *const EcIds[] = { "PNP0C09", UACPI_NULL };

    return Node && uacpi_device_matches_pnp_id(Node, EcIds);
}

/* Brings up one namespace EC from its own _CRS, _GPE and _GLK */
static
PUACPINT_EC
NTAPI
UacpiNtEcBringUpNode(
    _In_ uacpi_namespace_node *Node)
{
    PUACPINT_EC Ec;
    BOOLEAN HasGpe;

    /* Already running from the ECDT or an earlier pass */
    Ec = UacpiNtEcFindByNode(Node);
    if (Ec)
        return Ec;

    Ec = UacpiNtEcAllocate(Node);
    if (!Ec)
        return NULL;

    if (!UacpiNtEcReadPorts(Node, Ec))
    {
        DPRINT1("uACPI-NT: EC _CRS has no usable ports\n");
        ExFreePoolWithTag(Ec, UACPINT_POOL_TAG);
        return NULL;
    }

    UacpiNtEcReadGlk(Node, Ec);
    HasGpe = UacpiNtEcReadGpe(Node, Ec);

    if (!UacpiNtEcInstall(Ec, HasGpe))
    {
        ExFreePoolWithTag(Ec, UACPINT_POOL_TAG);
        return NULL;
    }

    return Ec;
}

static
BOOLEAN
NTAPI
UacpiNtEcBringUpEcdt(
    _In_ struct acpi_ecdt *Ecdt)
{
    uacpi_namespace_node *Node = NULL;
    uacpi_status UacpiStatus;
    PUACPINT_EC Ec;

    UacpiStatus = uacpi_namespace_node_find(uacpi_namespace_root(), Ecdt->ec_id, &Node);
    if (uacpi_unlikely_error(UacpiStatus) || !Node)
    {
        DPRINT1("uACPI-NT: ECDT names %s which is not in the namespace\n", Ecdt->ec_id);
        return FALSE;
    }

    if (UacpiNtEcFindByNode(Node))
        return TRUE;

    Ec = UacpiNtEcAllocate(Node);
    if (!Ec)
        return FALSE;

    Ec->ControlPort = (USHORT)Ecdt->ec_control.address;
    Ec->DataPort = (USHORT)Ecdt->ec_data.address;
    Ec->GpeDevice = NULL;
    Ec->GpeLine = Ecdt->gpe_bit;
    Ec->FromEcdt = TRUE;

    if (!UacpiNtEcInstall(Ec, TRUE))
    {
        ExFreePoolWithTag(Ec, UACPINT_POOL_TAG);
        return FALSE;
    }

    DPRINT("uACPI-NT: ECDT EC %s on GPE 0x%02x\n", Ecdt->ec_id, Ec->GpeLine);
    return TRUE;
}

/* The ECDT describes the boot EC completely, no namespace methods needed */
static
BOOLEAN
NTAPI
UacpiNtEcInitFromEcdt(VOID)
{
    uacpi_table Table;
    BOOLEAN Success;

    if (uacpi_unlikely_error(uacpi_table_find_by_signature(ACPI_ECDT_SIGNATURE, &Table)))
        return FALSE;

    if (!Table.ptr)
        return FALSE;

    Success = UacpiNtEcBringUpEcdt(Table.ptr);
    uacpi_table_unref(&Table);

    return Success;
}

static
uacpi_iteration_decision
UacpiNtEcScanCallback(
    _In_opt_ void *User,
    _In_ uacpi_namespace_node *Node,
    _In_ uacpi_u32 Depth)
{
    UNREFERENCED_PARAMETER(User);
    UNREFERENCED_PARAMETER(Depth);

    /* IDs only, the EC _STA may itself need the region handler */
    if (!UacpiNtEcIsEcNode(Node))
        return UACPI_ITERATION_DECISION_CONTINUE;

    DPRINT("uACPI-NT: found a PNP0C09, reading its _CRS\n");

    /* One boot EC is enough */
    if (UacpiNtEcBringUpNode(Node))
        return UACPI_ITERATION_DECISION_BREAK;

    return UACPI_ITERATION_DECISION_CONTINUE;
}

/*
 * Runs from glue.c before the namespace is initialized, since _INI and _REG
 * may already touch EC fields. Other ECs come up at their own START.
 */
VOID
NTAPI
UacpiNtEcInitialize(VOID)
{
    KeInitializeSpinLock(&UacpiNtEcListLock);

    if (!UacpiNtEcInitFromEcdt())
        uacpi_namespace_for_each_child_simple(uacpi_namespace_root(), UacpiNtEcScanCallback, NULL);

    if (!UacpiNtEcList)
        DPRINT("uACPI-NT: no embedded controller\n");
}

/* Adopts the EC already running on this node, or brings it up now */
VOID
NTAPI
UacpiNtEcStart(
    _Inout_ PUACPINT_PDO Pdo)
{
    PUACPINT_EC Ec;

    if (!Pdo->Node || !UacpiNtEcIsEcNode(Pdo->Node))
        return;

    Ec = UacpiNtEcBringUpNode(Pdo->Node);
    if (!Ec)
    {
        DPRINT1("uACPI-NT: EC %s could not be started\n", Pdo->Name);
        return;
    }

    Ec->Pdo = Pdo;
    Pdo->Ec = Ec;
    DPRINT("uACPI-NT: EC %s started from %s\n", Pdo->Name, Ec->FromEcdt ? "ECDT" : "_CRS");
}

/* AML keeps using the EC while the namespace exists, only the PDO link goes away */
VOID
NTAPI
UacpiNtEcRemove(
    _Inout_ PUACPINT_PDO Pdo)
{
    PUACPINT_EC Ec = Pdo->Ec;

    if (!Ec)
        return;

    Ec->Pdo = NULL;
    Pdo->Ec = NULL;
    DPRINT("uACPI-NT: EC %s removed\n", Pdo->Name);
}

/*
 * IRP_MJ_READ and IRP_MJ_WRITE, ByteOffset is the EC address. This polls
 * under a fast mutex, so unlike acpi.sys it only works at PASSIVE_LEVEL.
 */
NTSTATUS
NTAPI
UacpiNtEcReadWrite(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    PUACPINT_EC Ec = Pdo->Ec;
    PUCHAR Buffer = Irp->AssociatedIrp.SystemBuffer;
    BOOLEAN IsWrite = (IoStack->MajorFunction == IRP_MJ_WRITE);
    ULONG Length = IoStack->Parameters.Read.Length;
    ULONG Offset = IoStack->Parameters.Read.ByteOffset.LowPart;
    uacpi_u32 Sequence;
    BOOLEAN Success;
    ULONG Index;

    if (!Ec || !Ec->RegionInstalled)
        return UacpiNtCompleteIrp(Irp, STATUS_DEVICE_NOT_READY, 0);

    if (KeGetCurrentIrql() != PASSIVE_LEVEL)
        return UacpiNtCompleteIrp(Irp, STATUS_INVALID_DEVICE_STATE, 0);

    if (!Length)
        return UacpiNtCompleteIrp(Irp, STATUS_SUCCESS, 0);

    if (!Buffer ||
        IoStack->Parameters.Read.ByteOffset.HighPart != 0 ||
        Offset >= UACPINT_EC_SPACE_SIZE ||
        Length > UACPINT_EC_SPACE_SIZE - Offset)
    {
        return UacpiNtCompleteIrp(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    /* Held for the whole transfer so firmware never sees half of it */
    if (!UacpiNtEcLockGlobal(Ec, &Sequence))
        return UacpiNtCompleteIrp(Irp, STATUS_DEVICE_BUSY, 0);

    for (Index = 0; Index < Length; Index++)
    {
        if (IsWrite)
            Success = UacpiNtEcWriteByte(Ec, (UCHAR)(Offset + Index), Buffer[Index]);
        else
            Success = UacpiNtEcReadByte(Ec, (UCHAR)(Offset + Index), &Buffer[Index]);

        if (!Success)
            break;
    }

    UacpiNtEcUnlockGlobal(Ec, Sequence);

    /* A timeout reports how far the transfer got */
    if (Index < Length)
        return UacpiNtCompleteIrp(Irp, STATUS_IO_TIMEOUT, Index);

    return UacpiNtCompleteIrp(Irp, STATUS_SUCCESS, Length);
}

static
NTSTATUS
NTAPI
UacpiNtEcUpdateRegistration(
    _Inout_ PUACPINT_EC Ec,
    _Inout_ PUACPINT_EC_QUERY_REGISTRATION Registration,
    _In_ BOOLEAN Register)
{
    UCHAR QueryCode = Registration->QueryCode;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ec->QueryLock, &OldIrql);

    if (Register)
    {
        /* One owner per query code */
        if (Registration->Handler && !Ec->QueryRoutine[QueryCode])
        {
            Ec->QueryRoutine[QueryCode] = (PUACPINT_EC_QUERY_ROUTINE)Registration->Handler;
            Ec->QueryContext[QueryCode] = Registration->Context;
            Registration->Cookie = (ULONG_PTR)QueryCode + 1;
            Status = STATUS_SUCCESS;
        }
    }
    else if (Ec->QueryRoutine[QueryCode])
    {
        Ec->QueryRoutine[QueryCode] = NULL;
        Ec->QueryContext[QueryCode] = NULL;
        Status = STATUS_SUCCESS;
    }

    KeReleaseSpinLock(&Ec->QueryLock, OldIrql);
    return Status;
}

/* METHOD_NEITHER hands over the caller's own pointer, so kernel callers only */
NTSTATUS
NTAPI
UacpiNtEcDeviceControl(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp,
    _Out_ PBOOLEAN Handled)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG IoControlCode = IoStack->Parameters.DeviceIoControl.IoControlCode;
    PUACPINT_EC_QUERY_REGISTRATION Registration;
    PUACPINT_EC Ec = Pdo->Ec;
    BOOLEAN Register;
    NTSTATUS Status;

    *Handled = FALSE;

    if (IoControlCode == IOCTL_UACPINT_EC_REGISTER_QUERY_HANDLER)
        Register = TRUE;
    else if (IoControlCode == IOCTL_UACPINT_EC_UNREGISTER_QUERY_HANDLER)
        Register = FALSE;
    else
        return STATUS_NOT_SUPPORTED;

    *Handled = TRUE;

    if (!Ec)
        return UacpiNtCompleteIrp(Irp, STATUS_DEVICE_NOT_READY, 0);

    if (Irp->RequestorMode != KernelMode)
        return UacpiNtCompleteIrp(Irp, STATUS_INVALID_DEVICE_REQUEST, 0);

    Registration = IoStack->Parameters.DeviceIoControl.Type3InputBuffer;
    if (!Registration ||
        IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(*Registration))
    {
        return UacpiNtCompleteIrp(Irp, STATUS_BUFFER_TOO_SMALL, 0);
    }

    Status = UacpiNtEcUpdateRegistration(Ec, Registration, Register);
    DPRINT("uACPI-NT: EC %s handler for _Q%02X, status 0x%lx\n",
           Register ? "register" : "unregister",
           Registration->QueryCode,
           Status);

    return UacpiNtCompleteIrp(Irp, Status, 0);
}
