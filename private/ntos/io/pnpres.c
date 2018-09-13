/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pnpres.c

Abstract:

    This module contains the plug-and-play resource allocation and translation
    routines

Author:

    Shie-Lin Tzong (shielint) 1-Mar-1997

Environment:

    Kernel mode

Revision History:

    25-Sept-1998    SantoshJ    Made IopAssign non-recursive.
    01-Oct-1998     SantoshJ    Replaced "complex (broken)" hypercube code and replaced with
                                cascading counters. Simple, faster, smaller code.
                                Added timeouts to IopAssign.
                                Added more self-debugging capability by generating more
                                meaningful debug spew.
    03-Feb-1999     SantoshJ    Do allocation one device at a time.
                                Do devices with BOOT config before others.
                                Optimize IopFindBusDeviceNode.
--*/

#include "iop.h"
#pragma hdrstop

#define MYDBG 0

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'erpP')
#endif // POOL_TAGGING

#if MYDBG
#define ExAllocatePoolAT(a,b) ExAllocatePoolWithTag(a,b,'0rpP')
#define ExAllocatePoolRD(a,b) ExAllocatePoolWithTag(a,b,'1rpP')
#define ExAllocatePoolCMRL(a,b) ExAllocatePoolWithTag(a,b,'2rpP')
#define ExAllocatePoolCMRR(a,b) ExAllocatePoolWithTag(a,b,'3rpP')
#define ExAllocatePoolAE(a,b) ExAllocatePoolWithTag(a,b,'4rpP')
#define ExAllocatePoolTE(a,b) ExAllocatePoolWithTag(a,b,'5rpP')
#define ExAllocatePoolPRD(a,b) ExAllocatePoolWithTag(a,b,'6rpP')
#define ExAllocatePoolIORD(a,b) ExAllocatePoolWithTag(a,b,'7rpP')
#define ExAllocatePool1RD(a,b) ExAllocatePoolWithTag(a,b,'8rpP')
#define ExAllocatePoolPDO(a,b) ExAllocatePoolWithTag(a,b,'9rpP')
#define ExAllocatePoolIORR(a,b) ExAllocatePoolWithTag(a,b,'ArpP')
#define ExAllocatePoolIORL(a,b) ExAllocatePoolWithTag(a,b,'BrpP')
#define ExAllocatePoolIORRR(a,b) ExAllocatePoolWithTag(a,b,'CrpP')
#else  // MYDBG
#define ExAllocatePoolAT(a,b) ExAllocatePool(a,b)
#define ExAllocatePoolRD(a,b) ExAllocatePool(a,b)
#define ExAllocatePoolCMRL(a,b) ExAllocatePool(a,b)
#define ExAllocatePoolCMRR(a,b) ExAllocatePool(a,b)
#define ExAllocatePoolAE(a,b) ExAllocatePool(a,b)
#define ExAllocatePoolTE(a,b) ExAllocatePool(a,b)
#define ExAllocatePoolPRD(a,b) ExAllocatePool(a,b)
#define ExAllocatePoolIORD(a,b) ExAllocatePool(a,b)
#define ExAllocatePool1RD(a,b) ExAllocatePool(a,b)
#define ExAllocatePoolPDO(a,b) ExAllocatePool(a,b)
#define ExAllocatePoolIORR(a,b) ExAllocatePool(a,b)
#define ExAllocatePoolIORL(a,b) ExAllocatePool(a,b)
#define ExAllocatePoolIORRR(a,b) ExAllocatePool(a,b)
#endif // MYDBG

//
// Forward typedefs.
//

typedef struct _REQ_DESC
    REQ_DESC, *PREQ_DESC;
typedef struct _REQ_ALTERNATIVE
    REQ_ALTERNATIVE, *PREQ_ALTERNATIVE;
typedef struct _REQ_LIST
    REQ_LIST, *PREQ_LIST;

//
// An IO_RESOURCE_REQUIREMENTS_LIST is translated into a tree of these data strucures:
//     REQ_LIST
//     REQ_ALTERNATIVE
//     REQ_DESC
// which are easier to manipulate while exploring the solution space.
//

struct _REQ_DESC {
    BOOLEAN ArbitrationRequired;
    UCHAR Reserved[3];
    INTERFACE_TYPE InterfaceType;
    ULONG BusNumber;
    PREQ_ALTERNATIVE ReqAlternative;              // REQ_ALTERNATIVE back pointer
    ULONG ReqDescIndex;                           // REQ_ALTERNATIVE.ReqDescTable[] index
    ULONG DevicePrivateCount;                     // DevicePrivate info for the LogConf
    PIO_RESOURCE_DESCRIPTOR DevicePrivate;        // (DevicePrivate is per-LogConf)
    PREQ_DESC TranslatedReqDesc;                     // Stack pointer for translated REQ_DESC
    union {
        PPI_RESOURCE_ARBITER_ENTRY Arbiter;       // Used in Original REQ_DESC
        PPI_RESOURCE_TRANSLATOR_ENTRY Translator; // Used in translated/adjusted REQ_DESC
    } u;
    ARBITER_LIST_ENTRY AlternativeTable;
    CM_PARTIAL_RESOURCE_DESCRIPTOR Allocation;

    // This two fields are just a place holder
    // They have to be copied back to AlternativeTable
    // and Allocation to be useful.

    ARBITER_LIST_ENTRY BestAlternativeTable;
    CM_PARTIAL_RESOURCE_DESCRIPTOR BestAllocation;
};

struct _REQ_ALTERNATIVE {
    PREQ_LIST ReqList;                            // Containing REQ_LIST
    ULONG ReqAlternativeIndex;                    // Containing REQ_LIST.ReqAlternativeTable[] index
    ULONG Priority;
    ULONG ReqDescCount;
    PREQ_DESC *ReqDescTableEnd;
    PREQ_DESC ReqDescTable[1];                    // Variable length
};

struct _REQ_LIST {
    PIOP_RESOURCE_REQUEST AssignEntry;
    PDEVICE_OBJECT PhysicalDevice;
    INTERFACE_TYPE InterfaceType;
    ULONG BusNumber;
    PREQ_ALTERNATIVE *SelectedAlternative;         // Alternative under processed.
    ULONG ReqAlternativeCount;
    PREQ_ALTERNATIVE *ReqBestAlternative;          // Best alternative so far.
                                                   // (Initialize it to the end of the table)
    PREQ_ALTERNATIVE ReqAlternativeTable[1];       // Variable length
};

//
// Structure to represent a cascading counter.
//

typedef struct _Counter {
    ULONG   Count;          // Current counter value.
    ULONG   ResetValue;     // Reset value for this counter.
    ULONG   MaximumValue;   // Maximum value for this counter.
} COUNTER, *PCOUNTER;


#define IS_TRANSLATED_REQ_DESC(r)   ((r)->ReqAlternative ? FALSE : TRUE)

//
// IopReleaseBootResources can only be called for non ROOT enumerated devices
//

#define IopReleaseBootResources(DeviceNode)                          \
    ASSERT(((DeviceNode)->Flags & DNF_MADEUP) == 0);                 \
    IopReleaseResourcesInternal(DeviceNode);                         \
    (DeviceNode)->Flags &= ~DNF_HAS_BOOT_CONFIG;                     \
    (DeviceNode)->Flags &= ~DNF_BOOT_CONFIG_RESERVED;                \
    if ((DeviceNode)->BootResources) {                               \
        ExFreePool((DeviceNode)->BootResources);                     \
        (DeviceNode)->BootResources = NULL;                          \
    }

//
// Duplicate_detection_Context
//

typedef struct _DUPLICATE_DETECTION_CONTEXT {
    PCM_RESOURCE_LIST TranslatedResources;
    PDEVICE_NODE Duplicate;
} DUPLICATE_DETECTION_CONTEXT, *PDUPLICATE_DETECTION_CONTEXT;

//
// Reused device node fields
//

#define NextDeviceNode     Sibling
#define PreviousDeviceNode Child

//
// Macros and definitions
//

#define STRUCTURE_ALIGNMENT 1  // disable the structure alignment.

//
// Pool Management macros
//

typedef struct _IOP_POOL {
    PUCHAR PoolStart;
    PUCHAR PoolEnd;
    ULONG PoolSize;
} IOP_POOL, *PIOP_POOL;

#define IopInitPool(                                                   \
    /* IN PIOP_POOL */ Pool,                                           \
    /* IN PUCHAR */ Start,                                             \
    /* IN ULONG */ Size                                                \
    )                                                                  \
{                                                                      \
    (Pool)->PoolStart = (Start);                                       \
    (Pool)->PoolEnd = (Start) + (Size);                                \
    (Pool)->PoolSize = (Size);                                         \
    RtlZeroMemory(Start, Size);                                        \
}

#define IopAllocPoolAligned(                                           \
    /* OUT PUCHAR */ Memory,                                           \
    /* IN PIOP_POOL */ Pool,                                           \
    /* IN ULONG */ Size                                                \
    )                                                                  \
    (Pool)->PoolStart = (PUCHAR)                                       \
        (((ULONG_PTR) (Pool)->PoolStart + STRUCTURE_ALIGNMENT - 1)     \
        & ~(STRUCTURE_ALIGNMENT - 1));                                 \
    IopAllocPool(Memory, Pool, Size);

#define IopAllocPool(                                                  \
    /* OUT PUCHAR */ Memory,                                           \
    /* IN PIOP_POOL */ Pool,                                           \
    /* IN ULONG */ Size                                                \
    )                                                                  \
    *(Memory) = (PVOID) (Pool)->PoolStart;                             \
    (Pool)->PoolStart += (Size);                                       \
    ASSERT((Pool)->PoolStart <= (Pool)->PoolEnd);

//
// static variables
//

LIST_ENTRY PiActiveArbiterList;       // The List of arbiters being processed.
LIST_ENTRY PiBestArbiterList;         // The list of arbiters for the best alternative so far
ULONG PiBestPriority;                 // The best score so far
PIOP_RESOURCE_REQUEST PiAssignTable;
ULONG PiAssignTableCount;
PDEVICE_NODE IopLegacyDeviceNode;     // head pointer of the list of made up device node for
                                      // IoAssignResources and IoReportResourceUsage
BOOLEAN PiNoRetest;
#if DBG
BOOLEAN PiUseTimeout = FALSE;
#else
BOOLEAN PiUseTimeout = TRUE;
#endif

//
// Timeout value for IopAssign in milliseconds.
//

#define FIND_BEST_ASSIGNMENT_TIMEOUT    5000

//
// External references
//

extern WCHAR IopWstrTranslated[];
extern WCHAR IopWstrRaw[];

//
// Debug support
//

#if DBG_SCOPE

#define DUMP_ERROR  0x0001
#define DUMP_INFO   0x0002
#define DUMP_DETAIL 0x0004
#define STOP_ERROR  0x1000

//ULONG PnpResDebugLevel = DUMP_INFO + DUMP_ERROR;
ULONG PnpResDebugLevel = 0;

#define DebugMessage(Level,Message)         \
    if (PnpResDebugLevel & (Level)) {       \
        DbgPrint Message;                   \
    }

typedef struct {
    PDEVICE_NODE devnode;
    CM_PARTIAL_RESOURCE_DESCRIPTOR resource;
} PNPRESDEBUGTRANSLATIONFAILURE;

ULONG PnpResDebugTranslationFailureCount = 32;  // get count in both this line and the next.
PNPRESDEBUGTRANSLATIONFAILURE PnpResDebugTranslationFailureArray[32];
PNPRESDEBUGTRANSLATIONFAILURE *PnpResDebugTranslationFailure = PnpResDebugTranslationFailureArray;

#else

#define DebugMessage(Level,Message)

#endif

//
// Internal/Forward function references
//

PCM_RESOURCE_LIST
IopCreateCmResourceList(
    IN PCM_RESOURCE_LIST ResourceList,
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG   BusNumber,
    OUT PCM_RESOURCE_LIST *RemainingList
    );

PCM_RESOURCE_LIST
IopCombineCmResourceList(
    IN PCM_RESOURCE_LIST ResourceListA,
    IN PCM_RESOURCE_LIST ResourceListB
    );

VOID
IopRemoveLegacyDeviceNode (
    IN PDEVICE_OBJECT   DeviceObject OPTIONAL,
    IN PDEVICE_NODE     LegacyDeviceNode
    );

NTSTATUS
IopFindLegacyDeviceNode (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    OUT PDEVICE_NODE *LegacyDeviceNode,
    OUT PDEVICE_OBJECT *LegacyPDO
    );

PDEVICE_NODE
IopFindBusDeviceNodeInternal (
    IN PDEVICE_NODE DeviceNode,
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber
    );

NTSTATUS
IopGetResourceRequirementsForAssignTable(
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN PIOP_RESOURCE_REQUEST AssignTableEnd,
    OUT PULONG DeviceCount
    );

NTSTATUS
IopResourceRequirementsListToReqList(
    IN ARBITER_REQUEST_SOURCE AllocationType,
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoResources,
    IN PDEVICE_OBJECT PhysicalDevice,
    OUT PVOID *ResReqList
    );

VOID
IopRearrangeReqList (
    IN PREQ_LIST ReqList
    );

VOID
IopRearrangeAssignTable (
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN ULONG Count
    );

int
__cdecl
IopComparePriority(
    const void *arg1,
    const void *arg2
    );

VOID
IopFreeResourceRequirementsForAssignTable(
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN PIOP_RESOURCE_REQUEST AssignTableEnd
    );

VOID
IopFreeReqAlternative (
    IN PREQ_ALTERNATIVE ReqAlternative
    );

VOID
IopFreeReqList (
    IN PREQ_LIST ReqList
    );

NTSTATUS
IopAssign(
    IN ULONG AssignTableCount,
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN BOOLEAN Rebalance
    );

VOID
IopBuildCmResourceLists(
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN PIOP_RESOURCE_REQUEST AssignTableEnd
    );

VOID
IopBuildCmResourceList (
    IN PIOP_RESOURCE_REQUEST AssignEntry
    );

NTSTATUS
IopAssignInner(
    IN ULONG AssignTableCount,
    IN PIOP_RESOURCE_REQUEST  AssignTable,
    IN BOOLEAN Rebalance
    );

NTSTATUS
IopPlacement(
    IN ARBITER_ACTION ArbiterAction,
    IN BOOLEAN Rebalance
    );

VOID
IopAddReqDescsToArbiters (
    IN ULONG ReqDescCount,
    IN PREQ_DESC *ReqDescTable
    );

VOID
IopRemoveReqDescsFromArbiters (
    ULONG ReqDescCount,
    PREQ_DESC *ReqDescTable
    );

BOOLEAN
IopIsBestConfiguration(
    IN VOID
    );

VOID
IopSaveCurrentConfiguration (
    IN VOID
    );

VOID
IopRestoreBestConfiguration(
    IN VOID
    );

BOOLEAN
IopFindResourceHandlerInfo(
    IN RESOURCE_HANDLER_TYPE HandlerType,
    IN PDEVICE_NODE DeviceNode,
    IN UCHAR ResourceType,
    OUT PVOID *HandlerEntry
    );

NTSTATUS
IopSetupArbiterAndTranslators(
    IN PREQ_DESC ReqDesc
    );

NTSTATUS
IopParentToRawTranslation(
    IN OUT PREQ_DESC ReqDesc
    );

NTSTATUS
IopChildToRootTranslation(
    IN PDEVICE_NODE DeviceNode,  OPTIONAL
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ARBITER_REQUEST_SOURCE ArbiterRequestSource,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR *Target
    );

NTSTATUS
IopTranslateAndAdjustReqDesc(
    IN PREQ_DESC ReqDesc,
    IN PPI_RESOURCE_TRANSLATOR_ENTRY TranslatorEntry,
    OUT PREQ_DESC *TranslatedReqDesc
    );

NTSTATUS
IopCallArbiter(
    PPI_RESOURCE_ARBITER_ENTRY ArbiterEntry,
    ARBITER_ACTION Command,
    PVOID Input1,
    PVOID Input2,
    PVOID Input3
    );

VOID
IopQueryRebalance (
    IN PDEVICE_NODE DeviceNode,
    IN ULONG Phase,
    IN PULONG RebalanceCount,
    IN PDEVICE_OBJECT **DeviceTable
    );

VOID
IopQueryRebalanceWorker (
    IN PDEVICE_NODE DeviceNode,
    IN ULONG RebalancePhase,
    IN PULONG RebalanceCount,
    IN PDEVICE_OBJECT **DeviceTable
    );

VOID
IopTestForReconfiguration (
    IN PDEVICE_NODE DeviceNode,
    IN ULONG RebalancePhase,
    IN PULONG RebalanceCount,
    IN PDEVICE_OBJECT **DeviceTable
    );

NTSTATUS
IopPlacementForRebalance (
    IN PDEVICE_NODE DeviceNode,
    IN ARBITER_ACTION ArbiterAction
    );

NTSTATUS
IopArbitrateDeviceResources (
    IN PDEVICE_NODE DeviceNode,
    IN ARBITER_ACTION ArbiterAction
    );

NTSTATUS
IopRebalance (
    IN ULONG AssignTableCont,
    IN PIOP_RESOURCE_REQUEST AssignTable
    );

NTSTATUS
IopFindResourcesForArbiter (
    IN PDEVICE_NODE DeviceNode,
    IN UCHAR ResourceType,
    OUT ULONG *Count,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR *CmDesc
    );

VOID
IopReleaseResourcesInternal (
    IN PDEVICE_NODE DeviceNode
    );

VOID
IopReleaseResources (
    IN PDEVICE_NODE DeviceNode
    );

NTSTATUS
IopRestoreResourcesInternal (
    IN PDEVICE_NODE DeviceNode
    );

VOID
IopSetLegacyDeviceInstance (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_NODE DeviceNode
    );

PCM_RESOURCE_LIST
IopCombineLegacyResources (
    IN PDEVICE_NODE DeviceNode
    );

NTSTATUS
IopPlacementForReservation(
    VOID
    );

NTSTATUS
IopReserve(
    IN PREQ_LIST ReqList
    );

NTSTATUS
IopReserveBootResourcesInternal (
    IN ARBITER_REQUEST_SOURCE ArbiterRequestSource,
    IN PDEVICE_OBJECT DeviceObject,
    IN PCM_RESOURCE_LIST BootResources
    );

BOOLEAN
IopNeedToReleaseBootResources(
    IN PDEVICE_NODE DeviceNode,
    IN PCM_RESOURCE_LIST AllocatedResources
    );

VOID
IopReleaseFilteredBootResources(
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN PIOP_RESOURCE_REQUEST AssignTableEnd
    );

#if DBG_SCOPE
VOID
IopDumpResourceRequirementsList(
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoResources
    );

VOID
IopDumpResourceDescriptor (
    IN PUCHAR Indent,
    IN PIO_RESOURCE_DESCRIPTOR Desc
    );

#endif

#if DBG_SCOPE
VOID
IopCheckDataStructures (
    IN PDEVICE_NODE DeviceNode
    );

VOID
IopCheckDataStructuresWorker (
    IN PDEVICE_NODE Device
    );

#endif

NTSTATUS
IopQueryConflictListInternal(
    PDEVICE_OBJECT        PhysicalDeviceObject,
    IN PCM_RESOURCE_LIST  ResourceList,
    IN ULONG              ResourceListSize,
    OUT PPLUGPLAY_CONTROL_CONFLICT_LIST ConflictList,
    IN ULONG              ConflictListSize,
    IN ULONG              Flags
    );

NTSTATUS
IopQueryConflictFillConflicts(
    PDEVICE_OBJECT              PhysicalDeviceObject,
    IN ULONG                    ConflictCount,
    IN PARBITER_CONFLICT_INFO   ConflictInfoList,
    OUT PPLUGPLAY_CONTROL_CONFLICT_LIST ConflictList,
    IN ULONG                    ConflictListSize,
    IN ULONG                    Flags
    );

NTSTATUS
IopQueryConflictFillString(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PWSTR            Buffer,
    IN OUT PULONG       Length,
    IN OUT PULONG       Flags
    );

BOOLEAN
IopEliminateBogusConflict(
    IN PDEVICE_OBJECT   PhysicalDeviceObject,
    IN PDEVICE_OBJECT   ConflictDeviceObject
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopAllocateResources)
#pragma alloc_text(PAGE, IopGetResourceRequirementsForAssignTable)
#pragma alloc_text(PAGE, IopResourceRequirementsListToReqList)
#pragma alloc_text(PAGE, IopComparePriority)
#pragma alloc_text(PAGE, IopRearrangeReqList)
#pragma alloc_text(PAGE, IopRearrangeAssignTable)
#pragma alloc_text(PAGE, IopFreeResourceRequirementsForAssignTable)
#pragma alloc_text(PAGE, IopFreeReqList)
#pragma alloc_text(PAGE, IopFreeReqAlternative)
#pragma alloc_text(PAGE, IopAssign)
#pragma alloc_text(PAGE, IopBuildCmResourceLists)
#pragma alloc_text(PAGE, IopBuildCmResourceList)
#pragma alloc_text(PAGE, IopAssignInner)
#pragma alloc_text(PAGE, IopPlacement)
#pragma alloc_text(PAGE, IopAddReqDescsToArbiters)
#pragma alloc_text(PAGE, IopRemoveReqDescsFromArbiters)
#pragma alloc_text(PAGE, IopIsBestConfiguration)
#pragma alloc_text(PAGE, IopSaveCurrentConfiguration)
#pragma alloc_text(PAGE, IopRestoreBestConfiguration)
#pragma alloc_text(PAGE, IopFindResourceHandlerInfo)
#pragma alloc_text(PAGE, IopSetupArbiterAndTranslators)
#pragma alloc_text(PAGE, IopParentToRawTranslation)
#pragma alloc_text(PAGE, IopChildToRootTranslation)
#pragma alloc_text(PAGE, IopTranslateAndAdjustReqDesc)
#pragma alloc_text(PAGE, IopCallArbiter)
#pragma alloc_text(PAGE, IopQueryRebalance)
#pragma alloc_text(PAGE, IopTestForReconfiguration)
#pragma alloc_text(PAGE, IopPlacementForRebalance)
#pragma alloc_text(PAGE, IopArbitrateDeviceResources)
#pragma alloc_text(PAGE, IopRebalance)
#pragma alloc_text(PAGE, IopFindResourcesForArbiter)
//#pragma alloc_text(PAGE, IopLegacyResourceAllocation)
#pragma alloc_text(PAGE, IopFindLegacyDeviceNode)
#pragma alloc_text(PAGE, IopRemoveLegacyDeviceNode)
#pragma alloc_text(PAGE, IopFindBusDeviceNode)
#pragma alloc_text(PAGE, IopFindBusDeviceNodeInternal)
#pragma alloc_text(PAGE, IopDuplicateDetection)
#pragma alloc_text(PAGE, IopReleaseResourcesInternal)
#pragma alloc_text(PAGE, IopRestoreResourcesInternal)
#pragma alloc_text(PAGE, IopSetLegacyDeviceInstance)
#pragma alloc_text(PAGE, IopCombineLegacyResources)
#pragma alloc_text(PAGE, IopReserve)
#pragma alloc_text(PAGE, IopPlacementForReservation)
#pragma alloc_text(PAGE, IopAllocateBootResources)
#pragma alloc_text(PAGE, IopReserveBootResourcesInternal)
#pragma alloc_text(PAGE, IopReleaseResources)
#pragma alloc_text(PAGE, IopReallocateResources)
#pragma alloc_text(PAGE, IopReleaseFilteredBootResources)
#pragma alloc_text(PAGE, IopNeedToReleaseBootResources)
#pragma alloc_text(PAGE, IopQueryConflictList)
#pragma alloc_text(PAGE, IopQueryConflictListInternal)
#pragma alloc_text(PAGE, IopQueryConflictFillConflicts)
#pragma alloc_text(PAGE, IopQueryConflictFillString)
#pragma alloc_text(PAGE, IopEliminateBogusConflict)
#pragma alloc_text(PAGE, IopCreateCmResourceList)
#pragma alloc_text(PAGE, IopCombineCmResourceList)
#pragma alloc_text(INIT, IopReserveLegacyBootResources)
#pragma alloc_text(INIT, IopReserveBootResources)
#if DBG_SCOPE
#pragma alloc_text(PAGE, IopCheckDataStructures)
#pragma alloc_text(PAGE, IopCheckDataStructuresWorker)
#pragma alloc_text(PAGE, IopDumpResourceRequirementsList)
#pragma alloc_text(PAGE, IopDumpResourceDescriptor)
#endif
#endif

NTSTATUS
IopAllocateResources(
    IN PULONG DeviceCountP,
    IN OUT PIOP_RESOURCE_REQUEST *AssignTablePP,
    IN BOOLEAN Locked,
    IN BOOLEAN BootConfigsOK
    )

/*++

Routine Description:

    For each AssignTable entry, this routine queries device's IO resource requirements
    list and converts it to our internal REQ_LIST format; calls worker routine to perform
    the resources assignment.

Parameters:

    AssignTable - supplies a pointer to the first entry of a IOP_RESOURCE_REQUEST table.

    AssignTableEnd - supplies a pointer to the end of IOP_RESOURCE_REQUEST table.

    Locked - Indicates whether the IopRegistrySemaphore is acquired by the caller.

    BootConfigsOK - Indicates whether we should assign BOOT configs.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PIOP_RESOURCE_REQUEST   AssignTable;
    PIOP_RESOURCE_REQUEST   AssignTableEnd;
    PIOP_RESOURCE_REQUEST   AssignTableTail;
    PIOP_RESOURCE_REQUEST   AssignEntry;
    PIOP_RESOURCE_REQUEST   AssignEntryOriginal;
    ULONG                   AssignTableCount;
    ULONG                   DeviceCount;
    NTSTATUS                Status;
    BOOLEAN                 FreeAssignTable;
    BOOLEAN                 tryRebalance;

    PAGED_CODE();

    DeviceCount = *DeviceCountP;
    FreeAssignTable = FALSE;
    tryRebalance = TRUE;
    AssignTable = *AssignTablePP;
    AssignTableEnd = AssignTable + DeviceCount;

    //
    // If legacy device resource allocation, don't rebalance on failure,
    // if the resource they want is already assigned, what they are looking
    // for isn't there.
    //

    if ((DeviceCount == 1) && (AssignTable->Flags & IOP_ASSIGN_NO_REBALANCE)) {

        tryRebalance = FALSE;

    }

    //
    // Grab the IO registry semaphore to make sure no other device is
    // reporting it's resource usage while we are searching for conflicts.
    //

    if (!Locked) {

        KeEnterCriticalRegion();

        Status = KeWaitForSingleObject( &IopRegistrySemaphore,
                                        DelayExecution,
                                        KernelMode,
                                        FALSE,
                                        NULL );

        if (!NT_SUCCESS(Status)) {

            DebugMessage(DUMP_ERROR, ("IopAllocateResources: Get RegistrySemaphore failed. Status %x\n", Status));
            KeLeaveCriticalRegion();
            return Status;

        }
    }

    //
    // Get the resource requirements.
    //

    Status = IopGetResourceRequirementsForAssignTable(AssignTable, AssignTableEnd, &DeviceCount);
    if (DeviceCount == 0) {

        DebugMessage(DUMP_INFO, ("IopAllocateResources: Get Requirements for Assign Table found nothing. status %x\n", Status));

        if (!Locked) {

            KeReleaseSemaphore(&IopRegistrySemaphore, 0, 1, FALSE);
            KeLeaveCriticalRegion();

        }

        return Status;

    }

    //
    // Check if it is OK to assign resources to devices with BOOT configs.
    //

    if (BootConfigsOK) {

        if (!IopBootConfigsReserved) {

            //
            // Process devices with BOOT configs or no requirements first.
            //

            for (AssignEntry = AssignTable; AssignEntry < AssignTableEnd; AssignEntry++) {

                PDEVICE_NODE    deviceNode = (PDEVICE_NODE)AssignEntry->PhysicalDevice->DeviceObjectExtension->DeviceNode;

                if (deviceNode->Flags & DNF_HAS_BOOT_CONFIG) {

                    break;
                }
            }

            if (AssignEntry != AssignTableEnd) {

                //
                // There are devices with BOOT config.
                //

                for (AssignEntry = AssignTable; AssignEntry < AssignTableEnd; AssignEntry++) {

                    PDEVICE_NODE    deviceNode = (PDEVICE_NODE)AssignEntry->PhysicalDevice->DeviceObjectExtension->DeviceNode;

                    if (    !(AssignEntry->Flags & IOP_ASSIGN_IGNORE) &&
                            !(deviceNode->Flags & DNF_HAS_BOOT_CONFIG) &&
                            AssignEntry->ResourceRequirements &&
                            AssignEntry->AllocationType != ArbiterRequestPnpDetected) {

                        DeviceCount--;
                        DebugMessage(DUMP_INFO, ("Delaying non BOOT config device %ws...\n", deviceNode->InstancePath.Buffer));
                        AssignEntry->Status = STATUS_RETRY;
                        AssignEntry->Flags |= IOP_ASSIGN_IGNORE;
                        IopFreeResourceRequirementsForAssignTable(AssignEntry, AssignEntry + 1);

                    }
                }
            }

            if (DeviceCount == 0) {

                if (!Locked) {

                    KeReleaseSemaphore(&IopRegistrySemaphore, 0, 1, FALSE);
                    KeLeaveCriticalRegion();

                }

                return Status;
            }
        }

        //
        // Check if we need to allocate a new table.
        //

        if (DeviceCount != *DeviceCountP) {

            //
            // Allocate a new table.
            //

            AssignTable = (PIOP_RESOURCE_REQUEST) ExAllocatePoolAT( PagedPool,
                                                                    sizeof(IOP_RESOURCE_REQUEST) * DeviceCount);
            if (AssignTable == NULL) {

                IopFreeResourceRequirementsForAssignTable(*AssignTablePP, AssignTableEnd);

                if (!Locked) {

                    KeReleaseSemaphore(&IopRegistrySemaphore, 0, 1, FALSE);
                    KeLeaveCriticalRegion();

                }

                return STATUS_INSUFFICIENT_RESOURCES;
            }

            FreeAssignTable = TRUE;

            //
            // Process the AssignTable to remove any entry which is marked as IOP_ASSIGN_IGNORE.
            //

            AssignEntryOriginal = *AssignTablePP;
            AssignTableEnd = AssignTable + DeviceCount;
            for (AssignEntry = AssignTable; AssignEntry < AssignTableEnd;) {

                if (!(AssignEntryOriginal->Flags & IOP_ASSIGN_IGNORE)) {

                    *AssignEntry = *AssignEntryOriginal;
                    AssignEntry++;

                }
                AssignEntryOriginal++;
            }
        }

    } else {

        //
        // Only process devices with no resource requirements. Rest get STATUS_RETRY.
        //

        for (AssignEntry = AssignTable; AssignEntry < AssignTableEnd; AssignEntry++) {

            if (    !(AssignEntry->Flags & IOP_ASSIGN_IGNORE) &&
                    AssignEntry->ResourceRequirements) {

                PDEVICE_NODE    deviceNode = (PDEVICE_NODE)AssignEntry->PhysicalDevice->DeviceObjectExtension->DeviceNode;

                DebugMessage(DUMP_INFO, ("Delaying resources requiring device %ws...\n", deviceNode->InstancePath.Buffer));
                IopFreeResourceRequirementsForAssignTable(AssignEntry, AssignEntry + 1);
                AssignEntry->Status = STATUS_RETRY;

            }
        }

        //
        // Release the I/O Registry Semaphore
        //

        if (!Locked) {

            KeReleaseSemaphore(&IopRegistrySemaphore, 0, 1, FALSE);
            KeLeaveCriticalRegion();

        }

        return Status;
    }
    AssignTableCount = (ULONG)(AssignTableEnd - AssignTable);
    ASSERT(AssignTableCount == DeviceCount);

    //
    // Sort the AssignTable
    //

    IopRearrangeAssignTable(AssignTable, DeviceCount);

    for (AssignEntry = AssignTable; AssignEntry < AssignTableEnd; AssignEntry++) {

        PDEVICE_NODE    deviceNode = (PDEVICE_NODE)AssignEntry->PhysicalDevice->DeviceObjectExtension->DeviceNode;

        DebugMessage(DUMP_INFO, ("Pnpres: Trying to allocate resources for %ws.\n", deviceNode->InstancePath.Buffer));

        Status = IopAssignInner(1, AssignEntry, FALSE);
        if (NT_SUCCESS(Status)) {

            IopBuildCmResourceLists(AssignEntry, AssignEntry + 1);
            if (AssignEntry->AllocationType == ArbiterRequestPnpEnumerated) {

                IopReleaseFilteredBootResources(AssignEntry, AssignEntry + 1);
            }
        } else if (Status == STATUS_INSUFFICIENT_RESOURCES) {

            DebugMessage(DUMP_ERROR, ("IopAllocateResource: Failed to allocate Pool.\n"));
            break;

        } else if (tryRebalance) {

            DebugMessage(DUMP_INFO, ("IopAllocateResources: Initiating REBALANCE...\n"));

            deviceNode->Flags |= DNF_NEEDS_REBALANCE;
            Status = IopRebalance(1, AssignEntry);
            deviceNode->Flags &= ~DNF_NEEDS_REBALANCE;
            if (!NT_SUCCESS(Status)) {
                AssignEntry->Status = STATUS_CONFLICTING_ADDRESSES;
            }
        } else {
            AssignEntry->Status = STATUS_CONFLICTING_ADDRESSES;
        }
    }

    //
    // IF we did not go through the entire table without any error,
    // mark remaining entries as RETRY.
    //

    for (; AssignEntry < AssignTableEnd; AssignEntry++) {

        AssignEntry->Status = STATUS_RETRY;
    }

    IopFreeResourceRequirementsForAssignTable(AssignTable, AssignTableEnd);

    //
    // Release the I/O Registry Semaphore
    //

    if (!Locked) {
        KeReleaseSemaphore( &IopRegistrySemaphore, 0, 1, FALSE );
        KeLeaveCriticalRegion( );
    }

    //
    // Copy the information in our own AssignTable to caller's AssignTable
    //

    if (FreeAssignTable) {
        AssignEntryOriginal = *AssignTablePP;
        for (AssignEntry = AssignTable; AssignEntry < AssignTableEnd;) {
            if (AssignEntryOriginal->Flags & (IOP_ASSIGN_IGNORE | IOP_ASSIGN_RETRY)) {
                AssignEntryOriginal++;
                continue;
            }
            *AssignEntryOriginal = *AssignEntry;
            AssignEntry++;
            AssignEntryOriginal++;
        }
        ASSERT(AssignEntryOriginal <= *AssignTablePP + *DeviceCountP);
        ExFreePool(AssignTable);
    }

    AssignEntry = *AssignTablePP;
    while (AssignEntry < *AssignTablePP + *DeviceCountP) {
        if (AssignEntry->Flags & (IOP_ASSIGN_IGNORE | IOP_ASSIGN_RETRY)) {
            AssignEntry++;
            continue;
        }
        if ((AssignEntry->Flags & IOP_ASSIGN_EXCLUDE) || AssignEntry->ResourceAssignment == NULL) {

            AssignEntry->Status = STATUS_CONFLICTING_ADDRESSES;
        }
        AssignEntry++;
    }

    return Status;
}

NTSTATUS
IopGetResourceRequirementsForAssignTable(
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN PIOP_RESOURCE_REQUEST AssignTableEnd,
    OUT PULONG DeviceCount
    )

/*++

Routine Description:

    For each AssignTable entry, this routine queries device's IO resource requirements
    list and converts it to our internal REQ_LIST format.

Parameters:

    AssignTable - supplies a pointer to the first entry of a IOP_RESOURCE_REQUEST table.

    AssignTableEnd - supplies a pointer to the end of IOP_RESOURCE_REQUEST table.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS Status, FinalStatus = STATUS_UNSUCCESSFUL;
    PIOP_RESOURCE_REQUEST AssignEntry;
    PDEVICE_OBJECT PhysicalDevice;
    PDEVICE_NODE DeviceNode;
    ULONG Length, Count = 0;

    PAGED_CODE();

    //
    // Go thru each entry, if the io resource requirements is not established,
    // we will first see if the device node contains an io resreq, if yes, we will
    // use it.  Otherwise, we will query resource requirements from the driver and cache
    // it in our device node structure.
    //

    for (AssignEntry = AssignTable; AssignEntry < AssignTableEnd; ++AssignEntry) {

        AssignEntry->ReqList = NULL;
        if (AssignEntry->Flags & IOP_ASSIGN_IGNORE) {
            FinalStatus = STATUS_SUCCESS;
            continue;
        }

        AssignEntry->ResourceAssignment = NULL;
        AssignEntry->TranslatedResourceAssignment = NULL;
        PhysicalDevice = AssignEntry->PhysicalDevice;
        DeviceNode = (PDEVICE_NODE)PhysicalDevice->DeviceObjectExtension->DeviceNode;

        if (DeviceNode->Flags & DNF_RESOURCE_REQUIREMENTS_CHANGED) {
            if (DeviceNode->ResourceRequirements) {
                ExFreePool(DeviceNode->ResourceRequirements);
                DeviceNode->ResourceRequirements = NULL;

                //
                // Mark that we need to cleare the resource requirements changed flag when
                // succeed.
                //

                AssignEntry->Flags |= IOP_ASSIGN_CLEAR_RESOURCE_REQUIREMENTS_CHANGE_FLAG;
                DeviceNode->Flags &= ~DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED;
                //DeviceNode->Flags &= ~DNF_RESOURCE_REQUIREMENTS_CHANGED;
            }
        }

        if (!AssignEntry->ResourceRequirements) {
            if (DeviceNode->ResourceRequirements &&
                !(DeviceNode->Flags & DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED)) {
                DebugMessage(DUMP_DETAIL, ("Pnpres: Resource requirements list already exists for %ws\n", DeviceNode->InstancePath.Buffer));
                AssignEntry->ResourceRequirements = DeviceNode->ResourceRequirements;
                AssignEntry->AllocationType = ArbiterRequestPnpEnumerated;
            } else {
                DebugMessage(DUMP_INFO,("Pnpres: Query Resource requirements list for %ws...\n", DeviceNode->InstancePath.Buffer));
                Status = IopQueryDeviceResources (PhysicalDevice,
                                                  QUERY_RESOURCE_REQUIREMENTS,
                                                  &AssignEntry->ResourceRequirements,
                                                  &Length
                                                  );
                if (!NT_SUCCESS(Status)) {
                    AssignEntry->Flags |= IOP_ASSIGN_IGNORE;
                    AssignEntry->Status = Status;
                    continue;
                } else if (AssignEntry->ResourceRequirements == NULL) {

                    //
                    // Status Success with NULL ResourceRequirements means
                    // no resource required.
                    //

                    AssignEntry->Flags |= IOP_ASSIGN_IGNORE;
                    AssignEntry->Status = Status;
                    continue;
                }
                if (DeviceNode->ResourceRequirements) {
                    ExFreePool(DeviceNode->ResourceRequirements);
                    DeviceNode->Flags &= ~DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED;
                }
                DeviceNode->ResourceRequirements = AssignEntry->ResourceRequirements;
            }
        }

        //
        // For non-stop case, even though the res req list has changed, we need
        // to guarantee that it still get its current setting, if possible.
        // Note, if the new resreq list does not cover the old setting.  It's bus
        // drivers' responsibility to guarantee that non of their child would be
        // affected. Otherwise, the bus drivers should not ask for non-stop.
        //

        if (AssignEntry->Flags & IOP_ASSIGN_KEEP_CURRENT_CONFIG) {
            PIO_RESOURCE_REQUIREMENTS_LIST filteredList;
            BOOLEAN exactMatch;

            Status = IopFilterResourceRequirementsList (
                         AssignEntry->ResourceRequirements,
                         DeviceNode->ResourceList,
                         &filteredList,
                         &exactMatch
                         );
            if (NT_SUCCESS(Status)) {

                //
                // Do not free the original AssignEntry->ResourceRequirements.
                // It is used by deviceNode->ResourceRequirements.
                //

                AssignEntry->ResourceRequirements = filteredList;
            } else {
                AssignEntry->Flags &= ~IOP_ASSIGN_KEEP_CURRENT_CONFIG;
            }
        }

#if DBG_SCOPE
        if (PnpResDebugLevel & DUMP_INFO) {
            IopDumpResourceRequirementsList(AssignEntry->ResourceRequirements);
        }
#endif

        //
        // Convert Io resource requirements list to our internal representation.
        //

        Status = IopResourceRequirementsListToReqList(
                        AssignEntry->AllocationType,
                        AssignEntry->ResourceRequirements,
                        PhysicalDevice,
                        &AssignEntry->ReqList);

        if (!NT_SUCCESS(Status) || AssignEntry->ReqList == NULL) {
            AssignEntry->Flags |= IOP_ASSIGN_IGNORE;
            AssignEntry->Status = Status;
            continue;
        } else {
            PREQ_LIST ReqList;

            ReqList = (PREQ_LIST)AssignEntry->ReqList;
            ReqList->AssignEntry = AssignEntry;

            //
            // Sort the ReqList such that the higher priority Alternative list are
             // placed in the front of the list.
            //

            IopRearrangeReqList(ReqList);
            if (ReqList->ReqBestAlternative == NULL) {

                AssignEntry->Flags |= IOP_ASSIGN_IGNORE;
                AssignEntry->Status = STATUS_DEVICE_CONFIGURATION_ERROR;
                IopFreeResourceRequirementsForAssignTable(AssignEntry, AssignEntry + 1);
                continue;

            } else if (ReqList->ReqAlternativeCount < 3) {

                AssignEntry->Priority = 0;

            } else {

                AssignEntry->Priority = ReqList->ReqAlternativeCount;

            }
        }

        AssignEntry->Status = STATUS_SUCCESS;
        Count++;

        //
        // As long as there is one entry established, we will return success.
        //

        FinalStatus = STATUS_SUCCESS;
    }
    *DeviceCount = Count;
    return FinalStatus;
}

NTSTATUS
IopResourceRequirementsListToReqList(
    IN ARBITER_REQUEST_SOURCE AllocationType,
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoResources,
    IN PDEVICE_OBJECT PhysicalDevice,
    OUT PVOID *ResReqList
    )

/*++

Routine Description:

    This routine processes the input Io resource requirements list and
    generates an internal REQ_LIST and its related structures.

Parameters:

    IoResources - supplies a pointer to the Io resource requirements List.

    PhysicalDevice - supplies a pointer to the physical device object requesting
            the resources.

    ReqList - supplies a pointer to a variable to receive the returned REQ_LIST.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    PIO_RESOURCE_LIST IoResourceList = IoResources->List;
    PIO_RESOURCE_DESCRIPTOR IoResourceDescriptor, BaseDescriptor, firstDescriptor;
    PIO_RESOURCE_DESCRIPTOR IoResourceDescriptorEnd;
    LONG IoResourceListCount = (LONG) IoResources->AlternativeLists;
    PUCHAR CoreEnd = (PUCHAR) IoResources + IoResources->ListSize;
    ULONG ReqAlternativeCount = IoResourceListCount;
    ULONG ReqDescAlternativeCount = 0;
    ULONG AlternativeDescriptorCount = 0;  // count of descriptors with IO_RESOURCE_ALTERNATIVE flag set
    PREQ_LIST ReqList;
    BOOLEAN NoAlternativeDescriptor;
    INTERFACE_TYPE interfaceType;
    ULONG busNumber;
    NTSTATUS    failureStatus = STATUS_UNSUCCESSFUL;
    NTSTATUS    finalStatus = STATUS_SUCCESS;

    PAGED_CODE();

    *ResReqList = NULL;

    //
    // Make sure there is some resource requirements to be fulfilled.
    //

    if (IoResourceListCount == 0) {
        DebugMessage(DUMP_INFO, ("PnpRes: No ResReqList to convert to ReqList\n"));
        return STATUS_SUCCESS;
    }

    //
    // ***** Phase 1 *****
    //
    // Parse the IO ResReq list to make sure it is valid and to determine the sizes of
    // internal structures.
    //

    while (--IoResourceListCount >= 0) {

        IoResourceDescriptor = firstDescriptor = IoResourceList->Descriptors;
        IoResourceDescriptorEnd = IoResourceDescriptor + IoResourceList->Count;

        if (IoResourceDescriptor == IoResourceDescriptorEnd) {

            //
            // An alternative list with zero descriptor count
            //

            return STATUS_SUCCESS;
        }
        //
        // Perform sanity check.  We have not allocated any pool space.
        // If failed, simply return failure status.
        //

        if ((PUCHAR) IoResourceDescriptor > CoreEnd) {

            //
            // The structure header (excluding the variable length Descriptors array) is
            // invalid.
            //

            DebugMessage(DUMP_ERROR, ("PnpRes: Invalid ResReqList\n"));
            goto InvalidParameter;
        }

        if (IoResourceDescriptor > IoResourceDescriptorEnd ||
            (PUCHAR) IoResourceDescriptorEnd > CoreEnd) {

            //
            // IoResourceDescriptorEnd is the result of arithmetic overflow;
            // or, the descriptor array is outside of the valid memory.
            //

            DebugMessage(DUMP_ERROR, ("PnpRes: Invalid ResReqList\n"));
            goto InvalidParameter;
        }

        if (IoResourceDescriptor->Type == CmResourceTypeConfigData) {
            IoResourceDescriptor++;
            firstDescriptor++;
        }
        NoAlternativeDescriptor = TRUE;
        while (IoResourceDescriptor < IoResourceDescriptorEnd) {
            switch (IoResourceDescriptor->Type) {
            case CmResourceTypeConfigData:
#if DBG_SCOPE
                 if (PnpResDebugLevel & DUMP_ERROR) {
                     DbgPrint("PnPRes: Invalid ResReq list !!!\n");
                     DbgPrint("        ConfigData descriptors are per-LogConf and\n");
                     DbgPrint("        should be at the beginning of an AlternativeList\n");
                     ASSERT(0);
                 }
#endif
                 goto InvalidParameter;

            case CmResourceTypeDevicePrivate:
                 while (IoResourceDescriptor < IoResourceDescriptorEnd &&
                        IoResourceDescriptor->Type == CmResourceTypeDevicePrivate) {
                     if (IoResourceDescriptor == firstDescriptor) {
#if DBG_SCOPE
                        if (PnpResDebugLevel & DUMP_ERROR) {
                            DbgPrint("PnPRes: Invalid ResReq list !!!\n");
                            DbgPrint("        The first descriptor of a LogConf can not be\n");
                            DbgPrint("        a DevicePrivate descriptor.\n");
                            ASSERT(0);
                        }
#endif
                        goto InvalidParameter;
                     }
                     ReqDescAlternativeCount++;   // Count number of descriptors
                     IoResourceDescriptor++;
                 }
                 NoAlternativeDescriptor = TRUE;
                 break;

            default:

                ++ReqDescAlternativeCount;        // Count number of descriptors

                //
                // For non-arbitrated resource type, set its Option to preferred such
                // that we won't get confused.
                //

                if ((IoResourceDescriptor->Type & CmResourceTypeNonArbitrated) ||
                    (IoResourceDescriptor->Type == CmResourceTypeNull)) {

                    if (IoResourceDescriptor->Type == CmResourceTypeReserved) {
                        --ReqDescAlternativeCount;
                    }
                    IoResourceDescriptor->Option = IO_RESOURCE_PREFERRED;
                    IoResourceDescriptor++;
                    NoAlternativeDescriptor = TRUE;
                    break;
                }
                if (IoResourceDescriptor->Option & IO_RESOURCE_ALTERNATIVE) {
                    if (NoAlternativeDescriptor) {
#if DBG_SCOPE
                        if (PnpResDebugLevel & DUMP_ERROR) {
                            DbgPrint("PnPRes: Invalid ResReq list !!!\n");
                            DbgPrint("        Alternative descriptor without Default or Preferred descriptor.\n");
                            ASSERT(0);
                        }
#endif
                       goto InvalidParameter;
                    }
                    ++AlternativeDescriptorCount; // Count number of Alternative descriptors
                } else {
                    NoAlternativeDescriptor = FALSE;
                }
                IoResourceDescriptor++;
                break;
            }
        }
        ASSERT(IoResourceDescriptor == IoResourceDescriptorEnd);
        IoResourceList = (PIO_RESOURCE_LIST) IoResourceDescriptorEnd;
    }

    //
    // ***** Phase 2 *****
    //
    // Allocate structures and initialize them according to caller's Io ResReq list.
    //

    {
        ULONG ReqDescCount = ReqDescAlternativeCount - AlternativeDescriptorCount;
        PUCHAR PoolStart;
        ULONG PoolSize;
        IOP_POOL OuterPool;
        IOP_POOL ReqAlternativePool;
        IOP_POOL ReqDescPool;

        PREQ_ALTERNATIVE ReqAlternative;
        PREQ_ALTERNATIVE *ReqAlternativePP;
#if DBG
        PREQ_ALTERNATIVE *ReqAlternativeEndPP;
#endif
        PREQ_DESC ReqDesc;
        PREQ_DESC *ReqDescPP;
        ULONG ReqAlternativeIndex;
        ULONG ReqDescIndex;

        //
        // Each structure in the list has a variable length array.
        // Those arrays are declared in their typedefs as arrays with one element,
        // that extra element is not being substracted because it provides an
        // extra 4 bytes of memory that is 50% (statistically) wasted and 50%
        // (statistically) used to allow for 8 byte alignment of all the structures.
        //
        // Allocate the pool and fail if the allocation failed.
        //

        {
            ULONG ReqListPoolSize =
                (FIELD_OFFSET(REQ_LIST, ReqAlternativeTable) +
                sizeof(PVOID) * ReqAlternativeCount + STRUCTURE_ALIGNMENT - 1) &
                ~(STRUCTURE_ALIGNMENT - 1);

            ULONG ReqAlternativePoolSize = (ReqAlternativeCount *
                (FIELD_OFFSET(REQ_ALTERNATIVE, ReqDescTable) +
                + sizeof(PVOID) * ReqDescCount + STRUCTURE_ALIGNMENT - 1))
                & ~(STRUCTURE_ALIGNMENT - 1);

            ULONG ReqDescPoolSize = (ReqDescCount * (sizeof(REQ_DESC) + STRUCTURE_ALIGNMENT - 1))
                & ~(STRUCTURE_ALIGNMENT - 1);

            PoolSize = ReqListPoolSize + ReqAlternativePoolSize + ReqDescPoolSize;

            PoolStart = ExAllocatePoolRD(PagedPool, PoolSize);
            if (!PoolStart) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            IopInitPool(&OuterPool, PoolStart, PoolSize);

            //
            // Partition the OuterPool into inner pools.
            //

            IopAllocPoolAligned(&ReqList, &OuterPool, ReqListPoolSize);

            IopAllocPoolAligned(&PoolStart, &OuterPool, ReqAlternativePoolSize);
            IopInitPool(&ReqAlternativePool, PoolStart, ReqAlternativePoolSize);

            IopAllocPoolAligned(&PoolStart, &OuterPool, ReqDescPoolSize);
            IopInitPool(&ReqDescPool, PoolStart, ReqDescPoolSize);
        }

        //
        // Convert the IO_RESOURCE_REQUIREMENTS_LIST into the REQ_LIST.
        //

        ReqAlternativePP = ReqList->ReqAlternativeTable;
        RtlZeroMemory(ReqAlternativePP, ReqAlternativeCount * sizeof(PREQ_ALTERNATIVE));
#if DBG
        ReqAlternativeEndPP = ReqAlternativePP + ReqAlternativeCount;
#endif
        ReqList->ReqAlternativeCount = ReqAlternativeCount;
        ReqList->PhysicalDevice = PhysicalDevice;
        ReqList->InterfaceType = IoResources->InterfaceType;
        if (ReqList->InterfaceType == InterfaceTypeUndefined) {
            ReqList->InterfaceType = PnpDefaultInterfaceType;
        }

        ReqList->BusNumber = IoResources->BusNumber;
        ReqList->SelectedAlternative = NULL;
        ReqAlternativeIndex = 0;

        interfaceType = IoResources->InterfaceType;
        if (interfaceType == InterfaceTypeUndefined) {
            interfaceType = PnpDefaultInterfaceType;
        }
        busNumber = IoResources->BusNumber;
        IoResourceList = IoResources->List;
        IoResourceListCount = IoResources->AlternativeLists;

        while (--IoResourceListCount >= 0) {
            ULONG arbiterFlag;

            IoResourceDescriptor = IoResourceList->Descriptors;
            IoResourceDescriptorEnd = IoResourceDescriptor + IoResourceList->Count;

            //
            // For each Io alternate list, we create a REQ_ALTERNATE table
            //

            IopAllocPoolAligned(&ReqAlternative, &ReqAlternativePool, FIELD_OFFSET(REQ_ALTERNATIVE, ReqDescTable));
            ReqAlternative->ReqList = ReqList;
            ReqAlternative->ReqAlternativeIndex = ReqAlternativeIndex++;
            ASSERT(ReqAlternativePP < ReqAlternativeEndPP);
            *ReqAlternativePP++ = ReqAlternative;
            ReqAlternative->ReqDescCount = 0;
            ReqAlternative->ReqDescTableEnd = ReqAlternative->ReqDescTable;

            //
            // If the first descriptor of the alternative is a CmResourceTypeConfigData
            // it contains priority information.
            //

            if (IoResourceDescriptor->Type == CmResourceTypeConfigData) {
                ReqAlternative->Priority = IoResourceDescriptor->u.ConfigData.Priority;
                IoResourceDescriptor++;
            } else {
                ReqAlternative->Priority = LCPRI_NORMAL;
            }

            if (ReqAlternative->Priority == LCPRI_BOOTCONFIG) {
                arbiterFlag = ARBITER_FLAG_BOOT_CONFIG;
            } else {
                arbiterFlag = 0;
            }

            ReqDescPP = ReqAlternative->ReqDescTable;
            ReqDescIndex = 0;

            while (IoResourceDescriptor < IoResourceDescriptorEnd) {

                PARBITER_LIST_ENTRY ArbiterListEntry;

                if (IoResourceDescriptor->Type == CmResourceTypeReserved) {
                    interfaceType = IoResourceDescriptor->u.DevicePrivate.Data[0];
                    if (interfaceType == InterfaceTypeUndefined) {
                        interfaceType = PnpDefaultInterfaceType;
                    }
                    busNumber = IoResourceDescriptor->u.DevicePrivate.Data[1];
                    IoResourceDescriptor++;
                } else {
                    IopAllocPoolAligned(&ReqDesc, &ReqDescPool, sizeof(REQ_DESC));
                    ReqDesc->ArbitrationRequired =
                        (IoResourceDescriptor->Type & CmResourceTypeNonArbitrated ||
                         IoResourceDescriptor->Type == CmResourceTypeNull) ? FALSE : TRUE;
                    ReqDesc->ReqAlternative = ReqAlternative;
                    ReqDesc->ReqDescIndex = ReqDescIndex++;
                    ReqDesc->DevicePrivateCount = 0;
                    ReqDesc->DevicePrivate = NULL;
                    ReqDesc->InterfaceType = interfaceType;
                    ReqDesc->BusNumber = busNumber;

                    IopAllocPool(&PoolStart, &ReqAlternativePool, sizeof(PVOID));

                    ASSERT(PoolStart == (PUCHAR) ReqDescPP);

                    *ReqDescPP++ = ReqDesc;
                    ++ReqAlternative->ReqDescCount;
                    ++ReqAlternative->ReqDescTableEnd;

                    ReqDesc->TranslatedReqDesc = ReqDesc;  // TranslatedReqDesc points to itself.

                    ArbiterListEntry = &ReqDesc->AlternativeTable;
                    InitializeListHead(&ArbiterListEntry->ListEntry);
                    ArbiterListEntry->AlternativeCount = 0;
                    ArbiterListEntry->Alternatives = IoResourceDescriptor;
                    ArbiterListEntry->PhysicalDeviceObject = PhysicalDevice;
                    ArbiterListEntry->RequestSource = AllocationType;
                    ArbiterListEntry->Flags = arbiterFlag;
                    ArbiterListEntry->WorkSpace = 0;
                    ArbiterListEntry->InterfaceType = interfaceType;
                    ArbiterListEntry->SlotNumber = IoResources->SlotNumber;
                    ArbiterListEntry->BusNumber = IoResources->BusNumber;
                    ArbiterListEntry->Assignment = &ReqDesc->Allocation;
                    ArbiterListEntry->Result = ArbiterResultUndefined;

                    if (ReqDesc->ArbitrationRequired) {

                        //
                        // The BestAlternativeTable and BestAllocation are not initialized.
                        // They will be initialized when needed.

                        //
                        // Initialize the Cm partial resource descriptor to NOT_ALLOCATED.
                        //

                        ReqDesc->Allocation.Type = CmResourceTypeMaximum;

                        ASSERT((IoResourceDescriptor->Option & IO_RESOURCE_ALTERNATIVE) == 0);

                        ArbiterListEntry->AlternativeCount++;
                        IoResourceDescriptor++;
                        while (IoResourceDescriptor < IoResourceDescriptorEnd) {
                            if (IoResourceDescriptor->Type == CmResourceTypeDevicePrivate) {
                                ReqDesc->DevicePrivate = IoResourceDescriptor;
                                while (IoResourceDescriptor < IoResourceDescriptorEnd &&
                                       IoResourceDescriptor->Type == CmResourceTypeDevicePrivate) {
                                    ReqDesc->DevicePrivateCount++;
                                    ++IoResourceDescriptor;
                                }
                                break;
                            }
                            if (IoResourceDescriptor->Option & IO_RESOURCE_ALTERNATIVE) {
                                ArbiterListEntry->AlternativeCount++;
                                IoResourceDescriptor++;
                            } else {
                                break;
                            }
                        }

                        //
                        // Next query Arbiter and Translator interfaces for the resource desc.
                        //

                        status = IopSetupArbiterAndTranslators(ReqDesc);
                        if (!NT_SUCCESS(status)) {
                            DebugMessage(DUMP_ERROR, ("PnpRes: Unable to setup Arbiter and Translators\n"));
                            ReqAlternativeIndex--;
                            ReqAlternativePP--;
                            ReqList->ReqAlternativeCount--;
                            IopFreeReqAlternative(ReqAlternative);
                            failureStatus = status;
                            break;
                        }
                    } else {
                        ReqDesc->Allocation.Type = IoResourceDescriptor->Type;
                        ReqDesc->Allocation.ShareDisposition = IoResourceDescriptor->ShareDisposition;
                        ReqDesc->Allocation.Flags = IoResourceDescriptor->Flags;
                        ReqDesc->Allocation.u.DevicePrivate.Data[0] =
                            IoResourceDescriptor->u.DevicePrivate.Data[0];
                        ReqDesc->Allocation.u.DevicePrivate.Data[1] =
                            IoResourceDescriptor->u.DevicePrivate.Data[1];
                        ReqDesc->Allocation.u.DevicePrivate.Data[2] =
                            IoResourceDescriptor->u.DevicePrivate.Data[2];

                        IoResourceDescriptor++;
                    }
                }

                if (IoResourceDescriptor >= IoResourceDescriptorEnd) break;
            }
            IoResourceList = (PIO_RESOURCE_LIST) IoResourceDescriptorEnd;
        }
        if (ReqAlternativeIndex == 0) {
            finalStatus = failureStatus;
            IopFreeReqList(ReqList);
        }
    }

    if (finalStatus == STATUS_SUCCESS) {
        *ResReqList = ReqList;
    }

    return finalStatus;

InvalidParameter:
    return STATUS_INVALID_PARAMETER;
}

int
__cdecl
IopComparePriority(
    const void *arg1,
    const void *arg2
    )

/*++

Routine Description:

    This routine compares the priority of arg1 and arg1.  It is used in C run time sort.

Parameters:

    Arg1, arg2 - a pointer to PREQ_ALTERNATIVE

Return Value:

    < 0 if arg1 < arg2
    = 0 if arg1 = arg2
    > 0 if arg1 > arg2

--*/

{
    PREQ_ALTERNATIVE RA1 = *(PREQ_ALTERNATIVE *)arg1;
    PREQ_ALTERNATIVE RA2 = *(PREQ_ALTERNATIVE *)arg2;

    PAGED_CODE();

    if (RA1->Priority == RA2->Priority) {
        if ((ULONG_PTR)RA1 < (ULONG_PTR)RA2) {
            return -1;
        } else {
            return 1;
        }
    }
    if (RA1->Priority > RA2->Priority) {
        return 1;
    } else {
        return -1;
    }
}
int
__cdecl
IopCompareAlternativeCount(
    const void *arg1,
    const void *arg2
    )

/*++

Routine Description:

    This routine compares the priority of arg1 and arg1.  It is used in C run time sort.

Parameters:

    Arg1, arg2 - a pointer to PREQ_ALTERNATIVE

Return Value:

    < 0 if arg1 < arg2
    = 0 if arg1 = arg2
    > 0 if arg1 > arg2

--*/

{
    PIOP_RESOURCE_REQUEST RR1 = (PIOP_RESOURCE_REQUEST)arg1;
    PIOP_RESOURCE_REQUEST RR2 = (PIOP_RESOURCE_REQUEST)arg2;

    PAGED_CODE();

    if (RR1->Priority == RR2->Priority) {
        if ((ULONG_PTR)RR1 < (ULONG_PTR)RR2) {
            return -1;
        } else {
            return 1;
        }
    }
    if (RR1->Priority > RR2->Priority) {
        return 1;
    } else {
        return -1;
    }
}

VOID
IopRearrangeReqList (
    IN PREQ_LIST ReqList
    )

/*++

Routine Description:

    This routine sorts the REQ_ALTERNATIVE lists of REQ_LIST in increasing order (in terms of
    priority value.)  Priority 1 is considered better than priority 2.

    So, the better choice is placed in front of the list.

Parameters:

    ReqList - Supplies a pointer to a REQ_LIST.

Return Value:

    None.

--*/

{
    PREQ_ALTERNATIVE *alternative;
    PREQ_ALTERNATIVE *lastAlternative;

    PAGED_CODE();

    if (ReqList->ReqAlternativeCount > 1) {

        qsort(  (void *)ReqList->ReqAlternativeTable,
                ReqList->ReqAlternativeCount,
                sizeof(PREQ_ALTERNATIVE),
                IopComparePriority);

    }

    //
    // Set the BestAlternative so that we try alternatives with priority <= LCPRI_LASTSOFTCONFIG.
    //

    alternative = &ReqList->ReqAlternativeTable[0];
    for (lastAlternative = alternative + ReqList->ReqAlternativeCount; alternative < lastAlternative; alternative++) {

        if ((*alternative)->Priority > LCPRI_LASTSOFTCONFIG) {

            break;

        }
    }

    if (alternative == &ReqList->ReqAlternativeTable[0]) {

        PDEVICE_NODE deviceNode = (PDEVICE_NODE)ReqList->PhysicalDevice->DeviceObjectExtension->DeviceNode;

        DebugMessage(DUMP_ERROR, ("PNPRES: Invalid priorities in the logical configs for %ws\n", deviceNode->InstancePath.Buffer));
//        ASSERT(alternative != &ReqList->ReqAlternativeTable[0]);
        ReqList->ReqBestAlternative = NULL;

    } else {

        ReqList->ReqBestAlternative = alternative;
    }
}

VOID
IopRearrangeAssignTable (
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN ULONG Count
    )

/*++

Routine Description:

    This routine sorts the REQ_ALTERNATIVE lists of REQ_LIST in increasing order (in terms of
    priority value.)  Priority 1 is considered better than priority 2.

    So, the better choice is placed in front of the list.

Parameters:

    ReqList - Supplies a pointer to a REQ_LIST.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    if (Count == 1 || Count == 0) {

        //
        // Most ReqLists only have one alternative...
        //

        return;
    }

    qsort((void *)AssignTable,
          Count,
          sizeof(IOP_RESOURCE_REQUEST),
          IopCompareAlternativeCount
          );
}

VOID
IopFreeReqAlternative (
    IN PREQ_ALTERNATIVE ReqAlternative
    )
{
    PREQ_DESC   reqDesc, reqDescx;
    ULONG       i;

    PAGED_CODE();

    if (ReqAlternative) {
        for (i = 0; i < ReqAlternative->ReqDescCount; i++) {
            reqDesc = ReqAlternative->ReqDescTable[i];
            reqDescx = reqDesc->TranslatedReqDesc;
            while (reqDescx && IS_TRANSLATED_REQ_DESC(reqDescx)) {
                reqDesc = reqDescx;
                if (reqDescx->AlternativeTable.Alternatives) {
                    ExFreePool(reqDescx->AlternativeTable.Alternatives);
                }
                reqDescx = reqDescx->TranslatedReqDesc;
                ExFreePool(reqDesc);
            }
        }
    }
}

VOID
IopFreeReqList (
    IN PREQ_LIST ReqList
    )

/*++

Routine Description:

    This routine release the ReqList associated with a resource requirements list

Parameters:

    ReqList - supplies a pointer to the REQ_LIST.

Return Value:

    None.

--*/

{
    ULONG i;

    PAGED_CODE();

    if (ReqList) {

        //
        // First we need to free all the *extra* req descs for translators.
        // The default Req Desc will be freed when the ReqList is freed.
        //

        for (i = 0; i < ReqList->ReqAlternativeCount; i++) {
            IopFreeReqAlternative(ReqList->ReqAlternativeTable[i]);
        }
        ExFreePool(ReqList);
    }
}

VOID
IopFreeResourceRequirementsForAssignTable(
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN PIOP_RESOURCE_REQUEST AssignTableEnd
    )

/*++

Routine Description:

    For each AssignTable entry, this routine frees its attached REQ_LIST.

Parameters:

    AssignTable - supplies a pointer to the first entry of a IOP_RESOURCE_REQUEST table.

    AssignTableEnd - supplies a pointer to the end of IOP_RESOURCE_REQUEST table.

Return Value:

    None.

--*/

{
    PIOP_RESOURCE_REQUEST AssignEntry;

    PAGED_CODE();

    for (AssignEntry = AssignTable; AssignEntry < AssignTableEnd; ++AssignEntry) {
        IopFreeReqList(AssignEntry->ReqList);
        AssignEntry->ReqList = NULL;
        if (AssignEntry->Flags & IOP_ASSIGN_KEEP_CURRENT_CONFIG &&
            AssignEntry->ResourceRequirements) {

            //
            // The REAL resreq list is cached in DeviceNode->ResourceRequirements.
            // We need to free the filtered list.
            //

            ExFreePool(AssignEntry->ResourceRequirements);
            AssignEntry->ResourceRequirements = NULL;
        }
    }
}

VOID
IopBuildCmResourceList (
    IN PIOP_RESOURCE_REQUEST AssignEntry
    )
/*++

Routine Description:

    This routine walks REQ_LIST of the AssignEntry to build a corresponding
    Cm Resource lists.  It also reports the resources to ResourceMap.

Parameters:

    AssignEntry - Supplies a pointer to an IOP_ASSIGN_REQUEST structure

Return Value:

    None.  The ResourceAssignment in AssignEntry is initialized.

--*/

{
    NTSTATUS status;
    HANDLE resourceMapKey;
    PDEVICE_OBJECT physicalDevice;
    PREQ_LIST reqList = AssignEntry->ReqList;
    PREQ_ALTERNATIVE reqAlternative;
    PREQ_DESC reqDesc, reqDescx;
    PIO_RESOURCE_DESCRIPTOR privateData;
    ULONG count = 0, size, i;
    PCM_RESOURCE_LIST cmResources, cmResourcesRaw;
    PCM_FULL_RESOURCE_DESCRIPTOR cmFullResource, cmFullResourceRaw;
    PCM_PARTIAL_RESOURCE_LIST cmPartialList, cmPartialListRaw;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDescriptor, cmDescriptorRaw, assignment, tAssignment;
#if DBG
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDescriptorEnd, cmDescriptorEndRaw;
#endif

    PAGED_CODE();

    //
    // Determine the size of the CmResourceList
    //

    //
    // Determine the size of the CmResourceList
    //

    reqAlternative = *reqList->SelectedAlternative;
    for (i = 0; i < reqAlternative->ReqDescCount; i++) {
        reqDesc = reqAlternative->ReqDescTable[i];
        count += reqDesc->DevicePrivateCount + 1;
    }

    size = sizeof(CM_RESOURCE_LIST) + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * (count - 1);
    cmResources = (PCM_RESOURCE_LIST) ExAllocatePoolCMRL(PagedPool, size);
    if (!cmResources) {

        //
        // If we can not find memory, the resources will not be committed by arbiter.
        //

        DebugMessage(DUMP_ERROR, ("PnpRes: Not enough memory to build Translated CmResourceList\n"));
        AssignEntry->Status = STATUS_INSUFFICIENT_RESOURCES;
        AssignEntry->ResourceAssignment = NULL;
        AssignEntry->TranslatedResourceAssignment = NULL;
        return;
    }
    cmResourcesRaw = (PCM_RESOURCE_LIST) ExAllocatePoolCMRR(PagedPool, size);
    if (!cmResourcesRaw) {
        DebugMessage(DUMP_ERROR, ("PnpRes: Not enough memory to build Raw CmResourceList\n"));
        ExFreePool(cmResources);
        AssignEntry->Status = STATUS_INSUFFICIENT_RESOURCES;
        AssignEntry->ResourceAssignment = NULL;
        AssignEntry->TranslatedResourceAssignment = NULL;
        return;
    }
    cmResources->Count = 1;
    cmFullResource = cmResources->List;

    //
    // The CmResourceList we build here does not distinguish the
    // Interface Type on descriptor level.  This should be fine because
    // for IoReportResourceUsage we ignore the CmResourceList we build
    // here.
    //

    cmFullResource->InterfaceType = reqList->InterfaceType;
    cmFullResource->BusNumber = reqList->BusNumber;
    cmPartialList = &cmFullResource->PartialResourceList;
    cmPartialList->Version = 0;
    cmPartialList->Revision = 0;
    cmPartialList->Count = count;
    cmDescriptor = cmPartialList->PartialDescriptors;
#if DBG
    cmDescriptorEnd = cmDescriptor + count;
#endif
    cmResourcesRaw->Count = 1;
    cmFullResourceRaw = cmResourcesRaw->List;
    cmFullResourceRaw->InterfaceType = reqList->InterfaceType;
    cmFullResourceRaw->BusNumber = reqList->BusNumber;
    cmPartialListRaw = &cmFullResourceRaw->PartialResourceList;
    cmPartialListRaw->Version = 0;
    cmPartialListRaw->Revision = 0;
    cmPartialListRaw->Count = count;
    cmDescriptorRaw = cmPartialListRaw->PartialDescriptors;
#if DBG
    cmDescriptorEndRaw = cmDescriptorRaw + count;
#endif

    for (i = 0; i < reqAlternative->ReqDescCount; i++) {
        reqDesc = reqAlternative->ReqDescTable[i];

        if (reqDesc->ArbitrationRequired) {

            //
            // Get raw assignment and copy it to our raw resource list
            //

            reqDescx = reqDesc->TranslatedReqDesc;
            if (reqDescx->AlternativeTable.Result != ArbiterResultNullRequest) {
                status = IopParentToRawTranslation(reqDescx);
                if (!NT_SUCCESS(status)) {
                    DebugMessage(DUMP_ERROR, ("PnpRes: Parent To Raw translation failed\n"));
                    ExFreePool(cmResources);
                    ExFreePool(cmResourcesRaw);
                    AssignEntry->Status = STATUS_INSUFFICIENT_RESOURCES;
                    AssignEntry->ResourceAssignment = NULL;
                    return;
                }
                assignment = reqDesc->AlternativeTable.Assignment;
            } else {
                assignment = reqDescx->AlternativeTable.Assignment;
            }
            *cmDescriptorRaw = *assignment;
            cmDescriptorRaw++;

            //
            // Translate assignment and copy it to our translated resource list
            //
            if (reqDescx->AlternativeTable.Result != ArbiterResultNullRequest) {
                status = IopChildToRootTranslation(
                             (PDEVICE_NODE)reqDesc->AlternativeTable.PhysicalDeviceObject->DeviceObjectExtension->DeviceNode,
                             reqDesc->InterfaceType,
                             reqDesc->BusNumber,
                             reqDesc->AlternativeTable.RequestSource,
                             &reqDesc->Allocation,
                             &tAssignment
                             );
                if (!NT_SUCCESS(status)) {
                    DebugMessage(DUMP_ERROR, ("PnpRes: Child to Root translation failed\n"));
                    ExFreePool(cmResources);
                    ExFreePool(cmResourcesRaw);
                    AssignEntry->Status = STATUS_INSUFFICIENT_RESOURCES;
                    AssignEntry->ResourceAssignment = NULL;
                    return;
                }
                *cmDescriptor = *tAssignment;
                ExFreePool(tAssignment);
            } else {
                *cmDescriptor = *(reqDescx->AlternativeTable.Assignment);
            }
            cmDescriptor++;

        } else {
            *cmDescriptorRaw = reqDesc->Allocation;
            *cmDescriptor = reqDesc->Allocation;
            cmDescriptorRaw++;
            cmDescriptor++;
        }

        //
        // Next copy the device private descriptors to CmResourceLists
        //

        count = reqDesc->DevicePrivateCount;
        privateData = reqDesc->DevicePrivate;
        while (count != 0) {

            cmDescriptor->Type = cmDescriptorRaw->Type = CmResourceTypeDevicePrivate;
            cmDescriptor->ShareDisposition = cmDescriptorRaw->ShareDisposition =
                         CmResourceShareDeviceExclusive;
            cmDescriptor->Flags = cmDescriptorRaw->Flags = privateData->Flags;
            RtlMoveMemory(&cmDescriptorRaw->u.DevicePrivate,
                          &privateData->u.DevicePrivate,
                          sizeof(cmDescriptorRaw->u.DevicePrivate.Data)
                          );
            RtlMoveMemory(&cmDescriptor->u.DevicePrivate,
                          &privateData->u.DevicePrivate,
                          sizeof(cmDescriptor->u.DevicePrivate.Data)
                          );
            privateData++;
            cmDescriptorRaw++;
            cmDescriptor++;
            count--;
            ASSERT(cmDescriptorRaw <= cmDescriptorEndRaw);
            ASSERT(cmDescriptor <= cmDescriptorEnd);
        }
        ASSERT(cmDescriptor <= cmDescriptorEnd);
        ASSERT(cmDescriptorRaw <= cmDescriptorEndRaw);

    }

    //
    // report assigned resources to ResourceMap
    //

    physicalDevice = AssignEntry->PhysicalDevice;

    //
    // Open ResourceMap key
    //

    status = IopCreateRegistryKeyEx( &resourceMapKey,
                                     (HANDLE) NULL,
                                     &CmRegistryMachineHardwareResourceMapName,
                                     KEY_READ | KEY_WRITE,
                                     REG_OPTION_VOLATILE,
                                     NULL
                                     );
    if (NT_SUCCESS(status )) {
        WCHAR DeviceBuffer[256];
        POBJECT_NAME_INFORMATION NameInformation;
        ULONG NameLength;
        UNICODE_STRING UnicodeClassName;
        UNICODE_STRING UnicodeDriverName;
        UNICODE_STRING UnicodeDeviceName;

        RtlInitUnicodeString(&UnicodeClassName, PNPMGR_STR_PNP_MANAGER);

        RtlInitUnicodeString(&UnicodeDriverName, REGSTR_KEY_PNP_DRIVER);

        NameInformation = (POBJECT_NAME_INFORMATION) DeviceBuffer;
        status = ObQueryNameString( physicalDevice,
                                    NameInformation,
                                    sizeof( DeviceBuffer ),
                                    &NameLength );
        if (NT_SUCCESS(status)) {
            NameInformation->Name.MaximumLength = sizeof(DeviceBuffer) - sizeof(OBJECT_NAME_INFORMATION);
            if (NameInformation->Name.Length == 0) {
                NameInformation->Name.Buffer = (PVOID)((ULONG_PTR)DeviceBuffer + sizeof(OBJECT_NAME_INFORMATION));
            }

            UnicodeDeviceName = NameInformation->Name;
            RtlAppendUnicodeToString(&UnicodeDeviceName, IopWstrRaw);

            //
            // IopWriteResourceList should remove all the device private and device
            // specifiec descriptors.
            //

            status = IopWriteResourceList(
                         resourceMapKey,
                         &UnicodeClassName,
                         &UnicodeDriverName,
                         &UnicodeDeviceName,
                         cmResourcesRaw,
                         size
                         );
            if (NT_SUCCESS(status)) {
                UnicodeDeviceName = NameInformation->Name;
                RtlAppendUnicodeToString (&UnicodeDeviceName, IopWstrTranslated);
                status = IopWriteResourceList(
                             resourceMapKey,
                             &UnicodeClassName,
                             &UnicodeDriverName,
                             &UnicodeDeviceName,
                             cmResources,
                             size
                             );
            }
        }
        ZwClose(resourceMapKey);
    }
#if 0 // Ignore the registry writing status.
    if (!NT_SUCCESS(status)) {
        ExFreePool(cmResources);
        ExFreePool(cmResourcesRaw);
        cmResources = NULL;
        cmResourcesRaw = NULL;
    }
#endif
    AssignEntry->ResourceAssignment = cmResourcesRaw;
    AssignEntry->TranslatedResourceAssignment = cmResources;
}

VOID
IopBuildCmResourceLists(
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN PIOP_RESOURCE_REQUEST AssignTableEnd
    )

/*++

Routine Description:

    For each AssignTable entry, this routine queries device's IO resource requirements
    list and converts it to our internal REQ_LIST format.

Parameters:

    AssignTable - supplies a pointer to the first entry of a IOP_RESOURCE_REQUEST table.

    AssignTableEnd - supplies a pointer to the end of IOP_RESOURCE_REQUEST table.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    PIOP_RESOURCE_REQUEST assignEntry;
    PDEVICE_OBJECT physicalDevice;
    PDEVICE_NODE deviceNode;

    PAGED_CODE();

    //
    // Go thru each entry, for each Physical device object, we build a CmResourceList
    // from its ListOfAssignedResources.
    //

    for (assignEntry = AssignTable; assignEntry < AssignTableEnd; ++assignEntry) {

        assignEntry->ResourceAssignment = NULL;
        if (assignEntry->Flags & IOP_ASSIGN_IGNORE || assignEntry->Flags & IOP_ASSIGN_RETRY) {
            continue;
        }
        if (assignEntry->Flags & IOP_ASSIGN_EXCLUDE) {
            assignEntry->Status = STATUS_UNSUCCESSFUL;
            continue;
        }
        assignEntry->Status = STATUS_SUCCESS;
        IopBuildCmResourceList (assignEntry);
        if (assignEntry->ResourceAssignment) {
            physicalDevice = assignEntry->PhysicalDevice;
            deviceNode = (PDEVICE_NODE)physicalDevice->DeviceObjectExtension->DeviceNode;
            IopWriteAllocatedResourcesToRegistry(
                  deviceNode,
                  assignEntry->ResourceAssignment,
                  IopDetermineResourceListSize(assignEntry->ResourceAssignment)
                  );
#if DBG_SCOPE
            DebugMessage(DUMP_INFO,("Pnpres: Building CM resource lists for %ws...\n", deviceNode->InstancePath.Buffer));
            if (PnpResDebugLevel & DUMP_INFO) {
                DbgPrint("Raw resources ");
                IopDumpCmResourceList(assignEntry->ResourceAssignment);
                DbgPrint("Translated resources ");
                IopDumpCmResourceList(assignEntry->TranslatedResourceAssignment);
            }
#endif
        }
    }
}

BOOLEAN
IopNeedToReleaseBootResources(
    IN PDEVICE_NODE DeviceNode,
    IN PCM_RESOURCE_LIST AllocatedResources
    )

/*++

Routine Description:

    This routine checks the AllocatedResources against boot allocated resources.
    If the allocated resources do not cover all the resource types in boot resources,
    in another words some types of boot resources have not been released by arbiter,
    we will return TRUE to indicate we need to release the boot resources manually.

Parameters:

    DeviceNode -  A device node

    AllocatedResources - the resources assigned to the devicenode by arbiters.

Return Value:

    TRUE or FALSE.

--*/

{
    PCM_FULL_RESOURCE_DESCRIPTOR cmFullDesc_a, cmFullDesc_b;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDescriptor_a, cmDescriptor_b;
    ULONG size_a, size_b, i, j, k;
    BOOLEAN returnValue = FALSE, found;
    PCM_RESOURCE_LIST bootResources;

    PAGED_CODE();

    bootResources = DeviceNode->BootResources;
    if (AllocatedResources->Count == 1 && bootResources && bootResources->Count != 0) {

        cmFullDesc_a = &AllocatedResources->List[0];
        cmFullDesc_b = &bootResources->List[0];
        for (i = 0; i < bootResources->Count; i++) {
            cmDescriptor_b = &cmFullDesc_b->PartialResourceList.PartialDescriptors[0];
            for (j = 0; j < cmFullDesc_b->PartialResourceList.Count; j++) {
                size_b = 0;
                switch (cmDescriptor_b->Type) {
                case CmResourceTypeNull:
                    break;
                case CmResourceTypeDeviceSpecific:
                     size_b = cmDescriptor_b->u.DeviceSpecificData.DataSize;
                     break;
                default:
                     if (cmDescriptor_b->Type < CmResourceTypeMaximum) {
                         found = FALSE;
                         cmDescriptor_a = &cmFullDesc_a->PartialResourceList.PartialDescriptors[0];
                         for (k = 0; k < cmFullDesc_a->PartialResourceList.Count; k++) {
                             size_a = 0;
                             if (cmDescriptor_a->Type == CmResourceTypeDeviceSpecific) {
                                 size_a = cmDescriptor_a->u.DeviceSpecificData.DataSize;
                             } else if (cmDescriptor_b->Type == cmDescriptor_a->Type) {
                                 found = TRUE;
                                 break;
                             }
                             cmDescriptor_a++;
                             cmDescriptor_a = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmDescriptor_a + size_a);
                         }
                         if (found == FALSE) {
                             returnValue = TRUE;
                             goto exit;
                         }
                     }
                }
                cmDescriptor_b++;
                cmDescriptor_b = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmDescriptor_b + size_b);
            }
            cmFullDesc_b = (PCM_FULL_RESOURCE_DESCRIPTOR)cmDescriptor_b;
        }
    }
exit:
    return returnValue;
}


VOID
IopReleaseFilteredBootResources(
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN PIOP_RESOURCE_REQUEST AssignTableEnd
    )

/*++

Routine Description:

    For each AssignTable entry, this routine checks if we need to manually release the device's
    boot resources.

Parameters:

    AssignTable - supplies a pointer to the first entry of a IOP_RESOURCE_REQUEST table.

    AssignTableEnd - supplies a pointer to the end of IOP_RESOURCE_REQUEST table.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    PIOP_RESOURCE_REQUEST assignEntry;
    PDEVICE_OBJECT physicalDevice;
    PDEVICE_NODE deviceNode;

    PAGED_CODE();

    //
    // Go thru each entry, for each Physical device object, we build a CmResourceList
    // from its ListOfAssignedResources.
    //

    for (assignEntry = AssignTable; assignEntry < AssignTableEnd; ++assignEntry) {

        if (assignEntry->ResourceAssignment) {
            physicalDevice = assignEntry->PhysicalDevice;
            deviceNode = (PDEVICE_NODE)physicalDevice->DeviceObjectExtension->DeviceNode;

            //
            // Release the device's boot resources if desired
            // (If a driver filters its res req list and removes some boot resources, after arbiter satisfies
            // the new res req list, the filtered out boot resources do not get
            // released by arbiters.  Because they no longer passed to arbiters. )
            // I am not 100% sure we should release the filtered boot resources.  But that's what arbiters try
            // to achieve.  So, we will do it.
            //

            if (IopNeedToReleaseBootResources(deviceNode, assignEntry->ResourceAssignment)) {
                IopReleaseResourcesInternal(deviceNode);
                IopReserveBootResourcesInternal(
                        ArbiterRequestPnpEnumerated,
                        physicalDevice,
                        assignEntry->ResourceAssignment);
                deviceNode->Flags &= ~DNF_BOOT_CONFIG_RESERVED;  // Keep DeviceNode->BootResources
                deviceNode->ResourceList = assignEntry->ResourceAssignment;
                status = IopRestoreResourcesInternal(deviceNode);
                ASSERT(status == STATUS_SUCCESS);
                if (!NT_SUCCESS(status)) {

                    //
                    // BUGBUG - according to arbiter design, we should bugcheck.
                    //

                    assignEntry->Flags = IOP_ASSIGN_EXCLUDE;
                    assignEntry->Status = status;
                    ExFreePool(assignEntry->ResourceAssignment);
                    assignEntry->ResourceAssignment = NULL;
                }
                deviceNode->ResourceList = NULL;
            }
        }
    }
}

NTSTATUS
IopPlacement(
    IN ARBITER_ACTION ArbiterAction,
    IN BOOLEAN Rebalance
    )

/*++

Routine Description:

    This routine examines each arbiter if its resreq list changed its test function will be
    called.

Parameters:

    ArbiterAction - supplies an arbiter action code to perform TEST or RETEST.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PLIST_ENTRY listEntry;
    PPI_RESOURCE_ARBITER_ENTRY arbiterEntry;
    NTSTATUS status;

    ASSERT((ArbiterAction == ArbiterActionTestAllocation)   ||
           (ArbiterAction == ArbiterActionRetestAllocation) ||
           (ArbiterAction == ArbiterActionCommitAllocation));

    if (Rebalance) {

        //
        // For rebalance case, we always start from the root and work our way up.
        //

        status = IopPlacementForRebalance (IopRootDeviceNode, ArbiterAction);
    } else {
        listEntry = PiActiveArbiterList.Flink;
        while (listEntry != &PiActiveArbiterList) {
            arbiterEntry = CONTAINING_RECORD(listEntry, PI_RESOURCE_ARBITER_ENTRY, ActiveArbiterList);
            listEntry = listEntry->Flink;
            ASSERT(IsListEmpty(&arbiterEntry->ResourceList) == FALSE);
            if (arbiterEntry->ResourcesChanged == FALSE) {

                //
                // If the resource requirements are the same and it failed before, we know it
                // won't be able to succeed.  So, return failure.
                //

                if (arbiterEntry->State & PI_ARBITER_TEST_FAILED) {
                    return STATUS_UNSUCCESSFUL;
                }
            } else {

                //
                // If the resource requirements are changed, we need to call arbiter to test it.
                //

                status = IopCallArbiter(arbiterEntry,
                                        ArbiterAction,
                                        &arbiterEntry->ResourceList,
                                        0,
                                        NULL
                                        );
                if (!NT_SUCCESS(status)) {
                    ASSERT(ArbiterAction == ArbiterActionTestAllocation);
                    arbiterEntry->State |= PI_ARBITER_TEST_FAILED;
                    return status;
                } else {
                    arbiterEntry->State &= ~PI_ARBITER_TEST_FAILED;
                    arbiterEntry->ResourcesChanged = FALSE;
                    if (ArbiterAction == ArbiterActionTestAllocation) {
                        arbiterEntry->State |= PI_ARBITER_HAS_SOMETHING;
                    } else {
                        if (ArbiterAction == ArbiterActionRetestAllocation) {
                            status = IopCallArbiter(arbiterEntry, ArbiterActionCommitAllocation, NULL, NULL, NULL);
                            ASSERT(status == STATUS_SUCCESS);
                        }
                        arbiterEntry->State = 0;
                        InitializeListHead(&arbiterEntry->ActiveArbiterList);
                        InitializeListHead(&arbiterEntry->BestConfig);
                        InitializeListHead(&arbiterEntry->ResourceList);
                        InitializeListHead(&arbiterEntry->BestResourceList);
                    }
                }
            }
        }
        status = STATUS_SUCCESS;
    }
    return status;
}

NTSTATUS
IopPlacementForReservation(
    VOID
    )

/*++

Routine Description:

    This routine examines each arbiter if its resreq list changed its test function will be
    called.

Parameters:

    None.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PLIST_ENTRY listEntry;
    PPI_RESOURCE_ARBITER_ENTRY arbiterEntry;
    NTSTATUS status, returnStatus = STATUS_SUCCESS;

    listEntry = PiActiveArbiterList.Flink;
    while (listEntry != &PiActiveArbiterList) {
        arbiterEntry = CONTAINING_RECORD(listEntry, PI_RESOURCE_ARBITER_ENTRY, ActiveArbiterList);
        listEntry = listEntry->Flink;
        ASSERT(IsListEmpty(&arbiterEntry->ResourceList) == FALSE);
        if (arbiterEntry->ResourcesChanged) {

            //
            // If the resource requirements are changed, we need to call arbiter to reserve it.
            //

            status = IopCallArbiter(arbiterEntry,
                                    ArbiterActionBootAllocation,
                                    &arbiterEntry->ResourceList,
                                    0,
                                    NULL
                                    );
            if (!NT_SUCCESS(status)) {
#if MYDBG
                PARBITER_LIST_ENTRY arbiterListEntry = (PARBITER_LIST_ENTRY) arbiterEntry->ResourceList.Flink;
                DbgPrint("Allocate Boot Resources Failed ::\n");
                DbgPrint("        Count = %x, PDO = %x\n",
                         arbiterListEntry->AlternativeCount, arbiterListEntry->PhysicalDeviceObject);
                IopDumpResourceDescriptor(
                   "        ",
                   arbiterListEntry->Alternatives);
#endif
                returnStatus = status;
            }
            arbiterEntry->ResourcesChanged = FALSE;
            arbiterEntry->State = 0;
            InitializeListHead(&arbiterEntry->ActiveArbiterList);
            InitializeListHead(&arbiterEntry->BestConfig);
            InitializeListHead(&arbiterEntry->ResourceList);
            InitializeListHead(&arbiterEntry->BestResourceList);
        }
    }
    return returnStatus;
}

VOID
IopAddReqDescsToArbiters (
    IN ULONG ReqDescCount,
    IN PREQ_DESC *ReqDescTable
    )

/*++

Routine Description:

    This routine adds a list of a req descriptors to their arbiters.

Parameters:

    P1 -

Return Value:

    None.

--*/
{
    PREQ_DESC                   reqDesc;
    PREQ_DESC                   reqDescTranslated;
    PLIST_ENTRY                 listHead;
    PPI_RESOURCE_ARBITER_ENTRY  arbiterEntry;
    PREQ_DESC                   *reqDescTableEnd;

    for (reqDescTableEnd = ReqDescTable + ReqDescCount; ReqDescTable < reqDescTableEnd; ReqDescTable++) {

        //
        // For each req desc, find its arbiter, link the translated req desc to its arbiter.
        //

        reqDesc = *ReqDescTable;
        if (reqDesc->ArbitrationRequired) {

            reqDescTranslated = reqDesc->TranslatedReqDesc;  // Could be reqDesc itself

            InitializeListHead(&reqDescTranslated->AlternativeTable.ListEntry);
            arbiterEntry = reqDesc->u.Arbiter;
            ASSERT(arbiterEntry);
            listHead = &arbiterEntry->ResourceList;
            InsertTailList(listHead, &reqDescTranslated->AlternativeTable.ListEntry);

            arbiterEntry->ResourcesChanged = TRUE;
            if (arbiterEntry->State & PI_ARBITER_HAS_SOMETHING) {

                IopCallArbiter(arbiterEntry, ArbiterActionRollbackAllocation, NULL, NULL, NULL);
                arbiterEntry->State &= ~PI_ARBITER_HAS_SOMETHING;
            }

            //
            // Link the arbiter entry to our active arbiter list if it has not been
            //

            if (IsListEmpty(&arbiterEntry->ActiveArbiterList)) {

                InsertTailList(&PiActiveArbiterList, &arbiterEntry->ActiveArbiterList);
            }
        }
    }
}

VOID
IopRemoveReqDescsFromArbiters (
    ULONG ReqDescCount,
    PREQ_DESC *ReqDescTable
    )

/*++

Routine Description:

    This routine removes a list of a req descriptors from their arbiters.

Parameters:

    P1 -

Return Value:

    None.

--*/
{
    PREQ_DESC                   reqDesc;
    PREQ_DESC                   reqDescTranslated;
    PLIST_ENTRY                 listHead;
    PPI_RESOURCE_ARBITER_ENTRY  arbiterEntry;
    PREQ_DESC                   *reqDescTableEnd;

    for (reqDescTableEnd = ReqDescTable + ReqDescCount; ReqDescTable < reqDescTableEnd; ReqDescTable++) {

        //
        // For each req desc, find its arbiter, remove the req desc from its arbiter.
        //

        reqDesc = *ReqDescTable;
        if (reqDesc->ArbitrationRequired) {

            reqDescTranslated = reqDesc->TranslatedReqDesc;
            arbiterEntry = reqDesc->u.Arbiter;
            ASSERT(IsListEmpty(&arbiterEntry->ResourceList) == FALSE);

            arbiterEntry->ResourcesChanged = TRUE;
            if (arbiterEntry->State & PI_ARBITER_HAS_SOMETHING) {

                IopCallArbiter(arbiterEntry, ArbiterActionRollbackAllocation, NULL, NULL, NULL);
                arbiterEntry->State &= ~PI_ARBITER_HAS_SOMETHING;
            }

            RemoveEntryList(&reqDescTranslated->AlternativeTable.ListEntry);
            InitializeListHead(&reqDescTranslated->AlternativeTable.ListEntry);
            listHead = &arbiterEntry->ResourceList;
            if (IsListEmpty(listHead)) {

                //
                // Remove the arbiter entry from our active arbiter list if it has no resource
                // to arbitrate.
                //

                RemoveEntryList(&arbiterEntry->ActiveArbiterList);
                InitializeListHead(&arbiterEntry->ActiveArbiterList);
            }
        }
    }
}

BOOLEAN
IopIsBestConfiguration(
    IN VOID
    )

/*++

Routine Description:

    This routine checks if the current resource assignment yields the best score.
    If yes, this routine updates the static variable to reflect the best score.

Parameters:

    None.

Return Value:

    TRUE or FALSE.

--*/

{
    ULONG i, priority = 0;
    PREQ_ALTERNATIVE reqAlternative;
    PIOP_RESOURCE_REQUEST assignEntry;

    //
    // If PiNoRetest is set.  There is only one.  So, it is the best.
    //

    if (PiNoRetest) {
        return TRUE;
    }

    //
    // Go thru all the assign tables to collect priority
    //

    for (i = 0; i < PiAssignTableCount; i++) {
        assignEntry = PiAssignTable + i;

        if (!(assignEntry->Flags & IOP_ASSIGN_EXCLUDE)) {
            reqAlternative = *((PREQ_LIST)assignEntry->ReqList)->SelectedAlternative;
            priority += reqAlternative->Priority;
        }
    }

    //
    // If the new priority is better than current best priority.
    // Update the current best.
    //

    if (priority < PiBestPriority) {
        PiBestPriority = priority;
        return TRUE;
    } else {
        return FALSE;
    }
}

VOID
IopSaveCurrentConfiguration(
    IN VOID
    )

/*++

Routine Description:

    This routine goes thru every resource requirememts list to save the current
    best resource assignments.

Parameters:

    None.

Return Value:

    None.

--*/

{
    PREQ_ALTERNATIVE reqAlternative;
    PIOP_RESOURCE_REQUEST assignEntry;
    PREQ_DESC reqDesc, *reqDescpp;
    PREQ_LIST reqList;
    PLIST_ENTRY listEntry;
    PPI_RESOURCE_ARBITER_ENTRY arbiterEntry;
    ULONG i;

    //
    // If PiNoRetest is set, do nothing.  We don't need to save/restore
    //

    if (PiNoRetest) {
        PiBestArbiterList = PiActiveArbiterList;
        return;
    }

    //
    // Go thru all the assign tables to save the current assignment and mark
    // it as the current best selection.
    //

    for (i = 0; i < PiAssignTableCount; i++) {
        assignEntry = PiAssignTable + i;

        if (!(assignEntry->Flags & IOP_ASSIGN_EXCLUDE)) {
            reqList = assignEntry->ReqList;

            //
            // Save the current selected Alternative as the BestAlternative.
            //

            reqAlternative = *reqList->SelectedAlternative;
            reqList->ReqBestAlternative = reqList->SelectedAlternative;

            //
            // For each REQ_DESC in the best alternative list, save its arbiter list entry
            // and its assignments.
            //

            for (reqDescpp = reqAlternative->ReqDescTable;
                 reqDescpp < reqAlternative->ReqDescTableEnd;
                 reqDescpp++) {

                 if ((*reqDescpp)->ArbitrationRequired) {
                     reqDesc = (*reqDescpp)->TranslatedReqDesc;
                     reqDesc->BestAlternativeTable = reqDesc->AlternativeTable;
                     reqDesc->BestAllocation = reqDesc->Allocation;
                 }
            }
        }
    }

    //
    // Go throught the active arbiter list to save the current assignment
    // for retest.
    //

    listEntry = PiActiveArbiterList.Flink;
    while (listEntry != &PiActiveArbiterList) {
        arbiterEntry = CONTAINING_RECORD(listEntry, PI_RESOURCE_ARBITER_ENTRY, ActiveArbiterList);
        arbiterEntry->BestResourceList = arbiterEntry->ResourceList;
        arbiterEntry->BestConfig = arbiterEntry->ActiveArbiterList;
        listEntry = listEntry->Flink;
    }
    PiBestArbiterList = PiActiveArbiterList;
}

VOID
IopRestoreBestConfiguration(
    IN VOID
    )

/*++

Routine Description:

    This routine restores the arbiters info and all the resources requirements
    info to prepare arbiter restest.

Parameters:

    None.

Return Value:

    None.

--*/

{
    PREQ_ALTERNATIVE reqAlternative;
    PIOP_RESOURCE_REQUEST assignEntry;
    PREQ_DESC reqDesc, *reqDescpp;
    PREQ_LIST reqList;
    PLIST_ENTRY listEntry;
    PPI_RESOURCE_ARBITER_ENTRY arbiterEntry;
    ULONG i;

    if (PiNoRetest == FALSE) {

        //
        // Go thru all the assign tables to save the current configuration.
        //

        for (i = 0; i < PiAssignTableCount; i++) {
            assignEntry = PiAssignTable + i;

            if (!(assignEntry->Flags & IOP_ASSIGN_EXCLUDE)) {
                reqList = assignEntry->ReqList;

                //
                // Set the BestAlternative as the current selection.
                //

                reqAlternative = *reqList->ReqBestAlternative;
                reqList->SelectedAlternative = reqList->ReqBestAlternative;

                //
                // For each REQ_DESC in the best alternative list, restore its arbiter list entry
                // and its assignments from saved area.
                //

                for (reqDescpp = reqAlternative->ReqDescTable;
                     reqDescpp < reqAlternative->ReqDescTableEnd;
                     reqDescpp++) {

                     if ((*reqDescpp)->ArbitrationRequired) {
                         reqDesc = (*reqDescpp)->TranslatedReqDesc;
                         reqDesc->AlternativeTable = reqDesc->BestAlternativeTable;
                         reqDesc->Allocation = reqDesc->BestAllocation;
                     }
                }
            }
        }
        PiActiveArbiterList = PiBestArbiterList;
    }

    //
    // Go throught the active arbiter list to restore the current configuration
    // for retest.
    //

    listEntry = PiActiveArbiterList.Flink;
    while (listEntry != &PiActiveArbiterList) {
        arbiterEntry = CONTAINING_RECORD(listEntry, PI_RESOURCE_ARBITER_ENTRY, ActiveArbiterList);
        if (PiNoRetest == FALSE) {
            arbiterEntry->ResourceList = arbiterEntry->BestResourceList;
            arbiterEntry->ActiveArbiterList = arbiterEntry->BestConfig;
        }
        arbiterEntry->ResourcesChanged = TRUE;
        listEntry = listEntry->Flink;
    }
}

NTSTATUS
IopAssign(
    IN ULONG AssignTableCount,
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN BOOLEAN Rebalance
    )

/*++

Routine Description:

    This routine performs the resource allocation for the passed in AssignTables.

Parameters:

    AssignTableCount - supplies the number of AssignTable

    AssignTable - supplies a pointer to the first AssignTable.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS            status = STATUS_INSUFFICIENT_RESOURCES;
    LONG                tableIndex;
    PREQ_LIST           reqList;
    PREQ_ALTERNATIVE    reqAlternative;
    PDEVICE_NODE        deviceNode;
    LARGE_INTEGER       startTime;
    BOOLEAN             timeoutExpired;
    ULONG               spewCount = 0;

    PAGED_CODE();

    timeoutExpired = FALSE;

    //
    // Initialize our starting time.
    //

    KeQuerySystemTime(&startTime);

    //
    // Initialize the selected alternative for each entry to the first
    // possible alternative.
    //

    for (tableIndex = 0; tableIndex < (LONG)AssignTableCount; tableIndex++) {

        if (!(AssignTable[tableIndex].Flags & IOP_ASSIGN_EXCLUDE)) {

            reqList = AssignTable[tableIndex].ReqList;
            reqList->SelectedAlternative = &reqList->ReqAlternativeTable[0];
        }
    }

    //
    // Go through all possible combinations of req alternatives for all devices.
    //

    while (tableIndex >= 0) {

        //
        // Add the currently selected alternative to the arbiters iff
        // it has changed since the last time.
        //

        for (tableIndex = AssignTableCount - 1; tableIndex >= 0;) {

            if (AssignTable[tableIndex].Flags & IOP_ASSIGN_EXCLUDE) {

                tableIndex--;
            } else {

                reqList = AssignTable[tableIndex].ReqList;
                reqAlternative = *reqList->SelectedAlternative;
                deviceNode = (PDEVICE_NODE)AssignTable[tableIndex].PhysicalDevice->DeviceObjectExtension->DeviceNode;
                DebugMessage(DUMP_DETAIL, ("PnpRes: Adding %d/%d req alt to the arbiters for %ws.\n", reqAlternative->ReqAlternativeIndex + 1, reqList->ReqAlternativeCount, deviceNode->InstancePath.Buffer));
                IopAddReqDescsToArbiters(   reqAlternative->ReqDescCount,
                                            reqAlternative->ReqDescTable);
                if (reqList->SelectedAlternative == &reqList->ReqAlternativeTable[0]) {

                    tableIndex--;
                } else {

                    break;
                }
            }
        }

        //
        // Test this configuration.
        //

        status = IopPlacement(ArbiterActionTestAllocation, Rebalance);
        if (NT_SUCCESS(status)) {
            //
            // Compute priority and update best configuration if needed.
            //

            if (IopIsBestConfiguration()) {

                //
                // This assignment gets the best score.  Update its ReqBestAlternative
                // and return.
                //

                DebugMessage(DUMP_DETAIL, ("PnpRes: Found a best assignment.  Save it\n"));
                IopSaveCurrentConfiguration();

                //
                // The reqList->ReqBestAlternative will be set to reqAlternative, i.e.
                //     reqList->ReqBestAlternative = reqAlternative;
                // in IopSaveCurrentConfiguration().
                // This will cause the control to exit the for-loop.
                //

            }

            //
            // Do we need to stop?
            //

            if (PiNoRetest) {

                break;
            }
        }

        //
        // Algorithm to select the next alternative.
        //
        // We go through the table in the reverse direction.
        // We first remove the currently selected alternative.
        // We update the selected alternative to the next possible
        // alternative. If we roll over, we go to the next entry in
        // the table.
        //

        for (tableIndex = AssignTableCount - 1; tableIndex >= 0; ) {

            if (AssignTable[tableIndex].Flags & IOP_ASSIGN_EXCLUDE) {

                tableIndex--;
            } else {

                reqList = AssignTable[tableIndex].ReqList;
                reqAlternative = *reqList->SelectedAlternative;
                deviceNode = (PDEVICE_NODE)AssignTable[tableIndex].PhysicalDevice->DeviceObjectExtension->DeviceNode;
                DebugMessage(DUMP_DETAIL, ("PnpRes: Removing %d/%d req alt from the arbiters for %ws.\n", reqAlternative->ReqAlternativeIndex + 1, reqList->ReqAlternativeCount, deviceNode->InstancePath.Buffer));
                IopRemoveReqDescsFromArbiters(  reqAlternative->ReqDescCount,
                                                reqAlternative->ReqDescTable);
                if (++reqList->SelectedAlternative < reqList->ReqBestAlternative && !timeoutExpired) {

                    break;
                } else {

                    reqList->SelectedAlternative = &reqList->ReqAlternativeTable[0];
                    tableIndex--;
                }
            }

        }

        //
        // Check if timeout has expired.
        //

        if (tableIndex >= 0) {

            LARGE_INTEGER   currentTime;
            ULONG           timeDiff;

            //
            // Compute time difference in milliseconds.
            //

            KeQuerySystemTime(&currentTime);
            timeDiff = (ULONG)((currentTime.QuadPart - startTime.QuadPart) / 10000);

            //
            // We are done if timeout has expired.
            //

            if (timeDiff >= FIND_BEST_ASSIGNMENT_TIMEOUT)
            {
                if (PiUseTimeout) {

                    DebugMessage(DUMP_ERROR, ("PnpRes: Timeout expired, bailing out!\n"));
                    timeoutExpired = TRUE;

                } else {

                    spewCount = (spewCount + 1) % 50;
                    if (spewCount == 0) {
                        DbgPrint("PnpRes: Timeout expired.\n");
                    }
                }
            }
        }
    }

    return (status);
}

NTSTATUS
IopAssignInner(
    IN ULONG AssignTableCount,
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN BOOLEAN Rebalance
    )

/*++

Routine Description:

    This routine sets up static variables and invokes the real resource allocation
    routine.

Parameters:

    AssignTableCount - supplies the number of AssignTable

    AssignTable - supplies a pointer to the first AssignTable.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    ARBITER_ACTION arbiterAction;

    //
    // Initialize static variable
    //

    InitializeListHead(&PiBestArbiterList);
    InitializeListHead(&PiActiveArbiterList);
    PiAssignTableCount = AssignTableCount;
    PiAssignTable = AssignTable;
    PiBestPriority = (ULONG) -1;
    if (AssignTableCount == 1) {
        PiNoRetest = TRUE;
        arbiterAction = ArbiterActionCommitAllocation;
    } else {
        PiNoRetest = FALSE;
        arbiterAction = ArbiterActionRetestAllocation;
    }

    //
    // Try to solve the inner NP-complete problem.
    //

    IopAssign(AssignTableCount, AssignTable, Rebalance);
#if DBG_SCOPE
    if ((PnpResDebugLevel & STOP_ERROR) && PiNoRetest == FALSE) {
        IopCheckDataStructures(IopRootDeviceNode);
    }
#endif
    if (IsListEmpty(&PiBestArbiterList)) {
        DebugMessage(DUMP_DETAIL, ("PnpRes: IoAssignInner failed to find an assignment.\n"));
        status = STATUS_UNSUCCESSFUL;
    } else {

        if (Rebalance) {
            DebugMessage(DUMP_DETAIL, ("PnpRes: Restore the best assignment.\n"));
        } else {
            DebugMessage(DUMP_DETAIL, ("PnpRes: Restore the best assignment and commit it.\n"));
        }

        IopRestoreBestConfiguration();
        status = IopPlacement(arbiterAction, Rebalance);
#if DBG_SCOPE
        if (!Rebalance) {
            if (PnpResDebugLevel & STOP_ERROR) {
                IopCheckDataStructures(IopRootDeviceNode);
            }
        }
#endif
        ASSERT(status == STATUS_SUCCESS);
    }
    PiNoRetest = FALSE;
    return status;
}


NTSTATUS
IopReserve(
    IN PREQ_LIST ReqList
    )

/*++

Routine Description:

    This routine performs the resource allocation for the passed in AssignTables.

Parameters:


Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    PREQ_ALTERNATIVE RA;
    PREQ_ALTERNATIVE *reqAlternative;

    PAGED_CODE();

    //
    // Initialize static variable
    //

    InitializeListHead(&PiBestArbiterList);
    InitializeListHead(&PiActiveArbiterList);

    reqAlternative = ReqList->ReqAlternativeTable;

    RA = *reqAlternative;
    ReqList->SelectedAlternative = reqAlternative;
    IopAddReqDescsToArbiters(RA->ReqDescCount, RA->ReqDescTable);
    status = IopPlacementForReservation();
#if DBG_SCOPE
    if (PnpResDebugLevel & STOP_ERROR) {
        IopCheckDataStructures(IopRootDeviceNode);
    }
#endif
    return status;
}

BOOLEAN
IopFindResourceHandlerInfo(
    IN RESOURCE_HANDLER_TYPE HandlerType,
    IN PDEVICE_NODE DeviceNode,
    IN UCHAR ResourceType,
    OUT PVOID *HandlerEntry
    )

/*++

Routine Description:

    This routine finds the desired resource handler interface for the specified
    resource type in the specified Device node.

Parameters:

    HandlerType - Specifies the type of handler needed.

    DeviceNode - specifies the device node from where to search for handler

    ResourceTYpe - specifies the type of resource.

    HandlerEntry - supplies a pointer to a variable to receive the handler.

Return Value:

    TRUE + non-null HandlerEntry : Find handler info and there is a handler
    TRUE + NULL HandlerEntry     : Find handler info and there is NO handler
    FALSE + NULL HandlerEntry    : No handler info found.

--*/
{
    USHORT resourceMask;
    USHORT noHandlerMask, queryHandlerMask;
    PLIST_ENTRY listHead, nextEntry;
    PPI_RESOURCE_ARBITER_ENTRY arbiterEntry;

    *HandlerEntry = NULL;

    switch (HandlerType) {
    case ResourceArbiter:
        noHandlerMask = DeviceNode->NoArbiterMask;
        queryHandlerMask = DeviceNode->QueryArbiterMask;
        listHead = &DeviceNode->DeviceArbiterList;
        break;

    case ResourceTranslator:
        noHandlerMask = DeviceNode->NoTranslatorMask;
        queryHandlerMask = DeviceNode->QueryTranslatorMask;
        listHead = &DeviceNode->DeviceTranslatorList;
        break;

    default:
        return FALSE;
    }

    resourceMask = 1 << ResourceType;
    if (noHandlerMask & resourceMask) {

        //
        // There is no desired handler for the resource type in this device node
        //

        return TRUE;
    }

    if (queryHandlerMask & resourceMask) {

        //
        // Has handler for the resource type in this device node.
        // look for it ...
        //

        nextEntry = listHead->Flink;
        for (; nextEntry != listHead; nextEntry = nextEntry->Flink) {
            arbiterEntry = CONTAINING_RECORD(nextEntry, PI_RESOURCE_ARBITER_ENTRY, DeviceArbiterList);
            if (arbiterEntry->ResourceType == ResourceType) {
                break;
            }
        }
        ASSERT(nextEntry != listHead);     // There must be one ...
        *HandlerEntry = arbiterEntry;
        return TRUE;

    } else {

        //
        // If we are here there are two cases:
        //   We have not query the desired interface yet  or
        //   the resource type is out of the range we are tracking using masks.
        //   In this case, we need to go thru the link list.
        //

        if (ResourceType > PI_MAXIMUM_RESOURCE_TYPE_TRACKED) {
            nextEntry = listHead->Flink;
            for (; nextEntry != listHead; nextEntry = nextEntry->Flink) {
                arbiterEntry = CONTAINING_RECORD(nextEntry, PI_RESOURCE_ARBITER_ENTRY, DeviceArbiterList);
                if (arbiterEntry->ResourceType == ResourceType) {
                    if (arbiterEntry->ArbiterInterface) {
                        *HandlerEntry = arbiterEntry;
                    }
                    return TRUE;
                }
            }
        }
        return FALSE;
    }
}

NTSTATUS
IopSetupArbiterAndTranslators(
    IN PREQ_DESC ReqDesc
    )

/*++

Routine Description:

    This routine searches the arbiter and translators which arbitrates and translate
    the resources for the specified device.  This routine tries to find all the
    translator on the path of current device node to root device node

Parameters:

    ReqDesc - supplies a pointer to REQ_DESC which contains all the required information

Return Value:

    NTSTATUS value to indicate success or failure.

--*/

{
    PLIST_ENTRY listHead, nextEntry;
    PPI_RESOURCE_ARBITER_ENTRY arbiterEntry;
    PDEVICE_OBJECT deviceObject = ReqDesc->AlternativeTable.PhysicalDeviceObject;
    PDEVICE_NODE deviceNode;
    PREQ_DESC reqDesc = ReqDesc, translatedReqDesc;
    BOOLEAN found, arbiterFound = FALSE, restartedAlready;
    BOOLEAN  searchTranslator = TRUE, translatorFound = FALSE;
    NTSTATUS status;
    PPI_RESOURCE_TRANSLATOR_ENTRY translatorEntry;
    UCHAR resourceType = ReqDesc->TranslatedReqDesc->AlternativeTable.Alternatives->Type;
    PINTERFACE interface;
    USHORT resourceMask;

    if ((ReqDesc->AlternativeTable.RequestSource == ArbiterRequestHalReported) &&
        (ReqDesc->InterfaceType == Internal)) {

        // Trust hal if it says internal bus.

        restartedAlready = TRUE;
    } else {
        restartedAlready = FALSE;
    }

    //
    // If ReqDesc contains DeviceObject, this is for regular resources allocation
    // or boot resources preallocation.  Otherwise, it is for resources reservation.
    //

    if (deviceObject && ReqDesc->AlternativeTable.RequestSource != ArbiterRequestHalReported) {
        deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;
        // We want to start with the deviceNode instead of its parent.  Because the
        // deviceNode may provide a translator interface.
        // deviceNode = deviceNode->Parent;
    } else {

        //
        // For resource reservation, we always need to find the arbiter and translators
        // so set the device node to Root.
        //

        deviceNode = IopRootDeviceNode;
    }
    while (deviceNode) {
        if ((deviceNode == IopRootDeviceNode) && (translatorFound == FALSE)) {

            //
            // If we reach the root and have not find any translator, the device is on the
            // wrong way.
            //

            if (restartedAlready == FALSE) {
                restartedAlready = TRUE;

                deviceNode = IopFindBusDeviceNode (
                                 IopRootDeviceNode,
                                 ReqDesc->InterfaceType,
                                 ReqDesc->BusNumber,
                                 0
                                 );

                //
                // If we did not find a PDO, try again with InterfaceType == Isa. This allows
                // drivers that request Internal to get resources even if there is no PDO
                // that is Internal. (but if there is an Internal PDO, they get that one)
                //

                if ((deviceNode == IopRootDeviceNode) &&
                    (ReqDesc->ReqAlternative->ReqList->InterfaceType == Internal)) {
                    deviceNode = IopFindBusDeviceNode(
                                 IopRootDeviceNode,
                                 Isa,
                                 0,
                                 0
                                 );
                }

                //if ((PVOID)deviceNode == deviceObject->DeviceObjectExtension->DeviceNode) {
                //    deviceNode = IopRootDeviceNode;
                //} else {
                    continue;
                //}
            }
        }

        //
        // Check is there an arbiter for the device node?
        //   if yes, set up ReqDesc->u.Arbiter and set ArbiterFound to true.
        //   else move up to the parent of current device node.
        //

        if ((arbiterFound == FALSE) && (deviceNode->PhysicalDeviceObject != deviceObject)) {
            found = IopFindResourceHandlerInfo(
                               ResourceArbiter,
                               deviceNode,
                               resourceType,
                               &arbiterEntry);
            if (found == FALSE) {

                //
                // no information found on arbiter.  Try to query translator interface ...
                //

                if (resourceType <= PI_MAXIMUM_RESOURCE_TYPE_TRACKED) {
                    resourceMask = 1 << resourceType;
                } else {
                    resourceMask = 0;
                }
                status = IopQueryResourceHandlerInterface(ResourceArbiter,
                                                          deviceNode->PhysicalDeviceObject,
                                                          resourceType,
                                                          &interface);
                deviceNode->QueryArbiterMask |= resourceMask;
                if (!NT_SUCCESS(status)) {
                    deviceNode->NoArbiterMask |= resourceMask;
                    if (resourceType <= PI_MAXIMUM_RESOURCE_TYPE_TRACKED) {
                        found = TRUE;
                    } else {
                        interface = NULL;
                    }
                }
                if (found == FALSE) {
                    arbiterEntry = (PPI_RESOURCE_ARBITER_ENTRY)ExAllocatePoolAE(
                                       PagedPool,
                                       sizeof(PI_RESOURCE_ARBITER_ENTRY));
                    if (!arbiterEntry) {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        return status;
                    }
                    InitializeListHead(&arbiterEntry->ActiveArbiterList);
                    InitializeListHead(&arbiterEntry->DeviceArbiterList);
                    InitializeListHead(&arbiterEntry->BestConfig);
                    InitializeListHead(&arbiterEntry->ResourceList);
                    InitializeListHead(&arbiterEntry->BestResourceList);
                    arbiterEntry->ResourceType =resourceType;
                    arbiterEntry->State = 0;
                    arbiterEntry->ResourcesChanged = FALSE;
                    listHead = &deviceNode->DeviceArbiterList;
                    InsertTailList(listHead, &arbiterEntry->DeviceArbiterList);
                    arbiterEntry->ArbiterInterface = (PARBITER_INTERFACE)interface;
                    if (!interface) {

                        //
                        // if interface is NULL we really don't have translator.
                        //

                        arbiterEntry = NULL;
                    }
                }
            }

            //
            // If there is an desired resourcetype arbiter in the device node, make sure
            // it handle this resource requriements.
            //

            if (arbiterEntry) {
                arbiterFound = TRUE;
                if (arbiterEntry->ArbiterInterface->Flags & ARBITER_PARTIAL) {

                    //
                    // If the arbiter is partial, ask if it handles the resources
                    // if not, goto its parent.
                    //

                    status = IopCallArbiter(
                                arbiterEntry,
                                ArbiterActionQueryArbitrate,
                                ReqDesc->TranslatedReqDesc,
                                NULL,
                                NULL
                                );
                    if (!NT_SUCCESS(status)) {
                        arbiterFound = FALSE;
                    }
                }
            }
            if (arbiterFound) {
                ReqDesc->u.Arbiter = arbiterEntry;

                //
                // Initialize the arbiter entry
                //

                arbiterEntry->State = 0;
                arbiterEntry->ResourcesChanged = FALSE;
            }

        }

        if (searchTranslator) {
            //
            // First, check if there is a translator for the device node?
            // If yes, translate the req desc and link it to the front of ReqDesc->TranslatedReqDesc
            // else do nothing.
            //

            found = IopFindResourceHandlerInfo(
                        ResourceTranslator,
                        deviceNode,
                        resourceType,
                        &translatorEntry);

            if (found == FALSE) {

                //
                // no information found on translator.  Try to query translator interface ...
                //

                if (resourceType <= PI_MAXIMUM_RESOURCE_TYPE_TRACKED) {
                    resourceMask = 1 << resourceType;
                } else {
                    resourceMask = 0;
                }
                status = IopQueryResourceHandlerInterface(ResourceTranslator,
                                                          deviceNode->PhysicalDeviceObject,
                                                          resourceType,
                                                          &interface);
                deviceNode->QueryTranslatorMask |= resourceMask;
                if (!NT_SUCCESS(status)) {
                    deviceNode->NoTranslatorMask |= resourceMask;
                    if (resourceType <= PI_MAXIMUM_RESOURCE_TYPE_TRACKED) {
                        found = TRUE;
                    } else {
                        interface = NULL;
                    }
                }
                if (found == FALSE) {
                    translatorEntry = (PPI_RESOURCE_TRANSLATOR_ENTRY)ExAllocatePoolTE(
                                       PagedPool,
                                       sizeof(PI_RESOURCE_TRANSLATOR_ENTRY));
                    if (!translatorEntry) {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        return status;
                    }
                    translatorEntry->ResourceType = resourceType;
                    InitializeListHead(&translatorEntry->DeviceTranslatorList);
                    translatorEntry->TranslatorInterface = (PTRANSLATOR_INTERFACE)interface;
                    translatorEntry->DeviceNode = deviceNode;
                    listHead = &deviceNode->DeviceTranslatorList;
                    InsertTailList(listHead, &translatorEntry->DeviceTranslatorList);
                    if (!interface) {

                        //
                        // if interface is NULL we really don't have translator.
                        //

                        translatorEntry = NULL;
                    }
                }
            }
            if (translatorEntry) {
                translatorFound = TRUE;
            }
            if ((arbiterFound == FALSE) && translatorEntry) {

                //
                // Find a translator to translate the req desc ... Translate it and link it to
                // the front of ReqDesc->TranslatedReqDesc such that the first in the list is for
                // the Arbiter to use.
                //

                reqDesc = ReqDesc->TranslatedReqDesc;
                status = IopTranslateAndAdjustReqDesc(
                              reqDesc,
                              translatorEntry,
                              &translatedReqDesc);
                if (NT_SUCCESS(status)) {
                    ASSERT(translatedReqDesc);
                    resourceType = translatedReqDesc->AlternativeTable.Alternatives->Type;
                    translatedReqDesc->TranslatedReqDesc = ReqDesc->TranslatedReqDesc;
                    ReqDesc->TranslatedReqDesc = translatedReqDesc;
                    //
                    // If the translator is non-hierarchial and performs a complete
                    // translation to root (eg ISA interrups for PCI devices) then
                    // don't pass translations to parent.
                    //

                    if (status == STATUS_TRANSLATION_COMPLETE) {
                        searchTranslator = FALSE;
                    }
                } else {
                    DebugMessage(DUMP_INFO, ("PnpResr: resreq list TranslationAndAdjusted failed\n"));
                    return status;
                }
            }

        }

        //
        // Move up to current device node's parent
        //

        deviceNode = deviceNode->Parent;
    }

    if (arbiterFound) {
        return STATUS_SUCCESS;
    } else {

        //
        // We should BugCheck in this case.
        //

        DebugMessage(DUMP_ERROR, ("PnpResr: can not find resource type %x arbiter\n", resourceType));
        ASSERT(0);
        return STATUS_UNSUCCESSFUL;
    }

}

NTSTATUS
IopParentToRawTranslation(
    IN OUT PREQ_DESC ReqDesc
    )

/*++

Routine Description:

    This routine translates an CmPartialResourceDescriptors
    from their translated form to their raw counterparts..

Parameters:

    ReqDesc - supplies a translated ReqDesc to be translated back to its raw form

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    PTRANSLATOR_INTERFACE translator;
    NTSTATUS status = STATUS_SUCCESS;
    PREQ_DESC rawReqDesc;

    if (ReqDesc->AlternativeTable.AlternativeCount == 0 ||
        ReqDesc->Allocation.Type == CmResourceTypeMaximum) {
        DebugMessage(DUMP_ERROR, ("PnpRes : Invalid ReqDesc for parent-to-raw translation.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // If this ReqDesc is the raw reqDesc then we are done.
    // Else call its translator to translate the resource and leave the result
    // in its raw (next level) reqdesc.
    //

    if (IS_TRANSLATED_REQ_DESC(ReqDesc)) {
        rawReqDesc = ReqDesc->TranslatedReqDesc;
        translator = ReqDesc->u.Translator->TranslatorInterface;
        status = (translator->TranslateResources)(
                      translator->Context,
                      ReqDesc->AlternativeTable.Assignment,
                      TranslateParentToChild,
                      rawReqDesc->AlternativeTable.AlternativeCount,
                      rawReqDesc->AlternativeTable.Alternatives,
                      rawReqDesc->AlternativeTable.PhysicalDeviceObject,
                      rawReqDesc->AlternativeTable.Assignment
                      );
        if (NT_SUCCESS(status)) {

            //
            // If the translator is non-hierarchial and performs a complete
            // translation to root (eg ISA interrups for PCI devices) then
            // don't pass translations to parent.
            //

            ASSERT(status != STATUS_TRANSLATION_COMPLETE);
            status = IopParentToRawTranslation(rawReqDesc);
        }
    }
    return status;
}

NTSTATUS
IopChildToRootTranslation(
    IN PDEVICE_NODE DeviceNode, OPTIONAL
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ARBITER_REQUEST_SOURCE ArbiterRequestSource,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR *Target
    )

/*++

Routine Description:

    This routine translates a CmPartialResourceDescriptors from
    their intermediate translated form to their final translated form.
    The translated CM_PARTIAL_RESOURCE_DESCRIPTOR is returned via Target variable.

    The caller is responsible to release the translated descriptor.

Parameters:

    DeviceNode - Specified the device object.  If The DeviceNode is specified,
                 the InterfaceType and BusNumber are ignored and we will
                 use DeviceNode as a starting point to find various translators to
                 translate the Source descriptor.  If DeviceNode is not specified,
                 the InterfaceType and BusNumber must be specified.

    InterfaceType, BusNumber - must be supplied if DeviceNode is not specified.

    Source - A pointer to the resource descriptor to be translated.

    Target - Supplies an address to receive the translated resource descriptor.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    PDEVICE_NODE deviceNode;
    PLIST_ENTRY listHead, nextEntry;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR target, source, tmp;
    PPI_RESOURCE_TRANSLATOR_ENTRY translatorEntry;
    PTRANSLATOR_INTERFACE translator;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN done = FALSE, foundTranslator = FALSE, restartedAlready;

    if (ArbiterRequestSource == ArbiterRequestHalReported) {
       restartedAlready = TRUE;
    } else {
       restartedAlready = FALSE;
    }

    source = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ExAllocatePoolPRD(
                         PagedPool,
                         sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
                         );
    if (source == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    target = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ExAllocatePoolPRD(
                         PagedPool,
                         sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
                         );
    if (target == NULL) {
        ExFreePool(source);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    *source = *Source;

    //
    // Move up to current device node's parent to start translation
    //

    if (!ARGUMENT_PRESENT(DeviceNode)) {
        deviceNode = IopFindBusDeviceNode (IopRootDeviceNode, InterfaceType, BusNumber, 0);
    } else {
        // We want to start with the deviceNode instead of its parent.  Because the
        // deviceNode may provide a translator interface.
        deviceNode = DeviceNode;
    }
    while (deviceNode && !done) {

        if ((deviceNode == IopRootDeviceNode) && (foundTranslator == FALSE)) {
            if (restartedAlready == FALSE) {
                restartedAlready = TRUE;
                deviceNode = IopFindBusDeviceNode (IopRootDeviceNode, InterfaceType, BusNumber, 0);

                //
                // If we did not find a PDO, try again with InterfaceType == Isa. This allows
                // drivers that request Internal to get resources even if there is no PDO
                // that is Internal. (but if there is an Internal PDO, they get that one)
                //

                if ((deviceNode == IopRootDeviceNode) && (InterfaceType == Internal)) {
                    deviceNode = IopFindBusDeviceNode(IopRootDeviceNode, Isa, 0, 0);
                }

                continue;
            }
        }
        //
        // First, check if there is a translator for the device node?
        // If yes, translate the req desc and link it to the front of ReqDesc->TranslatedReqDesc
        // else do nothing.
        //

        listHead = &deviceNode->DeviceTranslatorList;
        nextEntry = listHead->Flink;
        for (; nextEntry != listHead; nextEntry = nextEntry->Flink) {
            translatorEntry = CONTAINING_RECORD(nextEntry, PI_RESOURCE_TRANSLATOR_ENTRY, DeviceTranslatorList);
            if (translatorEntry->ResourceType == Source->Type) {
                if (translator = translatorEntry->TranslatorInterface) {

                    //
                    // Find a translator to translate the req desc ... Translate it and link it to
                    // the front of ReqDesc->TranslatedReqDesc.
                    //

doitagain:
                    status = (translator->TranslateResources) (
                                  translator->Context,
                                  source,
                                  TranslateChildToParent,
                                  0,
                                  NULL,
                                  DeviceNode ? DeviceNode->PhysicalDeviceObject : NULL,
                                  target
                                  );
                    if (NT_SUCCESS(status)) {
                        tmp = source;
                        source = target;
                        target = tmp;

                        //
                        // If the translator is non-hierarchial and performs a complete
                        // translation to root (eg ISA interrups for PCI devices) then
                        // don't pass translations to parent.
                        //

                        if (status == STATUS_TRANSLATION_COMPLETE) {
                            done = TRUE;
                        }

                    } else {
#if DBG_SCOPE
                        // DebugMessage(DUMP_ERROR, ("PnpRes: Child to Root Translation failed\n"));
                        DbgPrint("PnpRes: Child to Root Translation failed\n");
                        if (DeviceNode) {
                            DbgPrint(
                                "        DeviceNode %08x (PDO %08x)\n",
                                DeviceNode,
                                DeviceNode->PhysicalDeviceObject
                                );
                        }
                        DbgPrint(
                            "        Resource Type %02x Data %08x %08x %08x\n",
                            source->Type,
                            source->u.DevicePrivate.Data[0],
                            source->u.DevicePrivate.Data[1],
                            source->u.DevicePrivate.Data[2]
                            );
                        if (PnpResDebugTranslationFailureCount) {
                            PnpResDebugTranslationFailureCount--;
                            PnpResDebugTranslationFailure->devnode = DeviceNode;
                            PnpResDebugTranslationFailure->resource = *source;
                            PnpResDebugTranslationFailure++;
                        }
                        if (PnpResDebugLevel & STOP_ERROR) {
                            DbgBreakPoint();
                            goto doitagain;
                        }
#endif
                        goto exit;
                    }
                }
                break;
            }
        }

        //
        // Move up to current device node's parent
        //

        deviceNode = deviceNode->Parent;
    }
    *Target = source;
    ExFreePool(target);
    return status;
exit:
    ExFreePool(source);
    ExFreePool(target);
    return status;
}

NTSTATUS
IopTranslateAndAdjustReqDesc(
    IN PREQ_DESC ReqDesc,
    IN PPI_RESOURCE_TRANSLATOR_ENTRY TranslatorEntry,
    OUT PREQ_DESC *TranslatedReqDesc
    )

/*++

Routine Description:

    This routine translates and adjusts ReqDesc IoResourceDescriptors to
    their translated and adjusted form.

Parameters:

    ReqDesc - supplies a pointer to the REQ_DESC to be translated.

    TranslatorEntry - supplies a pointer to the translator infor structure.

    TranslatedReqDesc - supplies a pointer to a variable to receive the
                        translated REQ_DESC.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    ULONG i, total = 0, *targetCount;
    PTRANSLATOR_INTERFACE translator = TranslatorEntry->TranslatorInterface;
    PIO_RESOURCE_DESCRIPTOR ioDesc, *target, tIoDesc;
    PREQ_DESC tReqDesc;
    PARBITER_LIST_ENTRY arbiterEntry;
    NTSTATUS status, returnStatus = STATUS_SUCCESS;
    BOOLEAN reqTranslated = FALSE;

    if (ReqDesc->AlternativeTable.AlternativeCount == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    *TranslatedReqDesc = NULL;

    target = (PIO_RESOURCE_DESCRIPTOR *) ExAllocatePoolIORD(
                           PagedPool,
                           sizeof(PIO_RESOURCE_DESCRIPTOR) * ReqDesc->AlternativeTable.AlternativeCount
                           );
    if (target == NULL) {
        DebugMessage(DUMP_ERROR, ("PnpRes: Not Enough memory to perform resreqlist adjustment\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(target, sizeof(PIO_RESOURCE_DESCRIPTOR) * ReqDesc->AlternativeTable.AlternativeCount);

    targetCount = (PULONG) ExAllocatePool(
                           PagedPool,
                           sizeof(ULONG) * ReqDesc->AlternativeTable.AlternativeCount
                           );
    if (targetCount == NULL) {
        DebugMessage(DUMP_ERROR, ("PnpRes: Not Enough memory to perform resreqlist adjustment\n"));
        ExFreePool(target);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(targetCount, sizeof(ULONG) * ReqDesc->AlternativeTable.AlternativeCount);

    //
    // Determine the number of IO_RESOURCE_DESCRIPTORs after translation.
    //

    ioDesc = ReqDesc->AlternativeTable.Alternatives;
    for (i = 0; i < ReqDesc->AlternativeTable.AlternativeCount; i++) {
        status = (translator->TranslateResourceRequirements)(
                           translator->Context,
                           ioDesc,
                           ReqDesc->AlternativeTable.PhysicalDeviceObject,
                           &targetCount[i],
                           &target[i]
                           );
        if (!NT_SUCCESS(status) || targetCount[i] == 0) {
            DebugMessage(DUMP_ERROR, ("PnpRes:Translator failed to adjust resreqlist\n"));
            target[i] = ioDesc;
            targetCount[i] = 0;
            total++;
        } else {
            total += targetCount[i];
            reqTranslated = TRUE;
        }
        ioDesc++;
        if (NT_SUCCESS(status) && (returnStatus != STATUS_TRANSLATION_COMPLETE)) {
            returnStatus = status;
        }
    }

    if (!reqTranslated) {
        DebugMessage(DUMP_ERROR, ("PnpRes:Failed to translate any requirement for %ws!\n", ((PDEVICE_NODE)(ReqDesc->AlternativeTable.PhysicalDeviceObject->DeviceObjectExtension->DeviceNode))->InstancePath.Buffer));
        returnStatus = status;
    }

    //
    // Allocate memory for the adjusted/translated resources descriptors
    //

    tIoDesc = (PIO_RESOURCE_DESCRIPTOR) ExAllocatePoolIORD(
                           PagedPool,
                           total * sizeof(IO_RESOURCE_DESCRIPTOR));
    if (!tIoDesc) {
        DebugMessage(DUMP_ERROR, ("PnpRes: Not Enough memory to perform resreqlist adjustment\n"));
        returnStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    tReqDesc = (PREQ_DESC) ExAllocatePool1RD (PagedPool, sizeof(REQ_DESC));
    if (tReqDesc == NULL) {
        DebugMessage(DUMP_ERROR, ("PnpRes: Not Enough memory to perform resreqlist adjustment\n"));
        ExFreePool(tIoDesc);
        returnStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    //
    // Create and initialize a new REQ_DESC for the translated/adjusted io resources
    //

    RtlMoveMemory(tReqDesc, ReqDesc, sizeof(REQ_DESC));

    //
    // Set the translated req desc's ReqAlternative to NULL to indicated this
    // is not the original req desc.
    //

    tReqDesc->ReqAlternative = NULL;

    tReqDesc->u.Translator = TranslatorEntry;
    tReqDesc->TranslatedReqDesc = NULL;
    arbiterEntry = &tReqDesc->AlternativeTable;
    InitializeListHead(&arbiterEntry->ListEntry);
    arbiterEntry->AlternativeCount = total;
    arbiterEntry->Alternatives = tIoDesc;
    arbiterEntry->Assignment = &tReqDesc->Allocation;

    ioDesc = ReqDesc->AlternativeTable.Alternatives;
    for (i = 0; i < ReqDesc->AlternativeTable.AlternativeCount; i++) {
        if (targetCount[i] != 0) {
            RtlMoveMemory(tIoDesc, target[i], targetCount[i] * sizeof(IO_RESOURCE_DESCRIPTOR));
            tIoDesc += targetCount[i];
        } else {

            //
            // Make it become impossible to satisfy.
            //

            RtlMoveMemory(tIoDesc, ioDesc, sizeof(IO_RESOURCE_DESCRIPTOR));
            switch (tIoDesc->Type) {
            case CmResourceTypePort:
            case CmResourceTypeMemory:
                tIoDesc->u.Port.MinimumAddress.LowPart = 2;
                tIoDesc->u.Port.MinimumAddress.HighPart = 0;
                tIoDesc->u.Port.MaximumAddress.LowPart = 1;
                tIoDesc->u.Port.MaximumAddress.HighPart = 0;
                break;
            case CmResourceTypeBusNumber:
                tIoDesc->u.BusNumber.MinBusNumber = 2;
                tIoDesc->u.BusNumber.MaxBusNumber = 1;
                break;

            case CmResourceTypeInterrupt:
                tIoDesc->u.Interrupt.MinimumVector = 2;
                tIoDesc->u.Interrupt.MaximumVector = 1;
                break;

            case CmResourceTypeDma:
                tIoDesc->u.Dma.MinimumChannel = 2;
                tIoDesc->u.Dma.MaximumChannel = 1;
                break;
            default:
                ASSERT(0);
                break;
            }
            tIoDesc += 1;
        }
        ioDesc++;

    }

#if DBG
    //
    // Verify the adjusted resource descriptors are valid
    //

    ioDesc = arbiterEntry->Alternatives;
    ASSERT((ioDesc->Option & IO_RESOURCE_ALTERNATIVE) == 0);
    ioDesc++;
    for (i = 1; i < total; i++) {
        ASSERT(ioDesc->Option & IO_RESOURCE_ALTERNATIVE);
        ioDesc++;
    }
#endif
    *TranslatedReqDesc = tReqDesc;
exit:
    for (i = 0; i < ReqDesc->AlternativeTable.AlternativeCount; i++) {
        if (targetCount[i] != 0) {
            ASSERT(target[i]);
            ExFreePool(target[i]);
        }
    }
    ExFreePool(target);
    ExFreePool(targetCount);
    return returnStatus;
}

NTSTATUS
IopCallArbiter(
    PPI_RESOURCE_ARBITER_ENTRY ArbiterEntry,
    ARBITER_ACTION Command,
    PVOID Input1,
    PVOID Input2,
    PVOID Input3
    )

/*++

Routine Description:

    This routine builds a Parameter block from Input structure and calls specified
    arbiter to carry out the Command.

Parameters:

    ArbiterEntry - Supplies a pointer to our PI_RESOURCE_ARBITER_ENTRY such that
                   we know everything about the arbiter.

    Command - Supplies the Action code for the arbiter.

    Input - Supplies a PVOID pointer to a structure.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    ARBITER_PARAMETERS parameters;
    PARBITER_INTERFACE arbiterInterface = ArbiterEntry->ArbiterInterface;
    NTSTATUS status;
    PARBITER_LIST_ENTRY arbiterListEntry;
    LIST_ENTRY listHead;
    PVOID *ExtParams;

    switch (Command) {
    case ArbiterActionTestAllocation:
    case ArbiterActionRetestAllocation:

        //
        // For ArbiterActionTestAllocation, the Input is a pointer to the doubly
        // linked list of ARBITER_LIST_ENTRY's.
        //

        parameters.Parameters.TestAllocation.ArbitrationList = (PLIST_ENTRY)Input1;
        parameters.Parameters.TestAllocation.AllocateFromCount = (ULONG)((ULONG_PTR)Input2);
        parameters.Parameters.TestAllocation.AllocateFrom =
                                            (PCM_PARTIAL_RESOURCE_DESCRIPTOR)Input3;
        status = (arbiterInterface->ArbiterHandler)(
                      arbiterInterface->Context,
                      Command,
                      &parameters
                      );
        break;

    case ArbiterActionBootAllocation:

        //
        // For ArbiterActionBootAllocation, the input is a pointer to the doubly
        // linked list of ARBITER_LIST_ENTRY'S.
        //

        parameters.Parameters.BootAllocation.ArbitrationList = (PLIST_ENTRY)Input1;

        status = (arbiterInterface->ArbiterHandler)(
                      arbiterInterface->Context,
                      Command,
                      &parameters
                      );
        break;

    case ArbiterActionQueryArbitrate:

        //
        // For QueryArbiter, the input is a pointer to REQ_DESC
        //

        arbiterListEntry = &((PREQ_DESC)Input1)->AlternativeTable;
        ASSERT(IsListEmpty(&arbiterListEntry->ListEntry));
        listHead = arbiterListEntry->ListEntry;
        arbiterListEntry->ListEntry.Flink = arbiterListEntry->ListEntry.Blink = &listHead;
        parameters.Parameters.QueryArbitrate.ArbitrationList = &listHead;
        status = (arbiterInterface->ArbiterHandler)(
                      arbiterInterface->Context,
                      Command,
                      &parameters
                      );
        arbiterListEntry->ListEntry = listHead;
        break;

    case ArbiterActionCommitAllocation:
    case ArbiterActionRollbackAllocation:
    case ArbiterActionWriteReservedResources:

        //
        // Commit, Rollback and WriteReserved do not have parmater.
        //

        status = (arbiterInterface->ArbiterHandler)(
                      arbiterInterface->Context,
                      Command,
                      NULL
                      );
        break;

    case ArbiterActionQueryAllocatedResources:
        status = STATUS_NOT_IMPLEMENTED;
        break;

    case ArbiterActionQueryConflict:
        //
        // For QueryConflict
        // Ex0 is PDO
        // Ex1 is PIO_RESOURCE_DESCRIPTOR
        // Ex2 is PULONG
        // Ex3 is PARBITER_CONFLICT_INFO *
        ExtParams = (PVOID*)Input1;

        parameters.Parameters.QueryConflict.PhysicalDeviceObject = (PDEVICE_OBJECT)ExtParams[0];
        parameters.Parameters.QueryConflict.ConflictingResource = (PIO_RESOURCE_DESCRIPTOR)ExtParams[1];
        parameters.Parameters.QueryConflict.ConflictCount = (PULONG)ExtParams[2];
        parameters.Parameters.QueryConflict.Conflicts = (PARBITER_CONFLICT_INFO *)ExtParams[3];
        status = (arbiterInterface->ArbiterHandler)(
                      arbiterInterface->Context,
                      Command,
                      &parameters
                      );
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    return status;
}

#if DBG_SCOPE
VOID
IopDumpResourceRequirementsList(
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoResources
    )

/*++

Routine Description:

    This routine dumps IoResources

Parameters:

    IoResources - supplies a pointer to the IO resource requirements list

Return Value:

    None.

--*/

{
    PIO_RESOURCE_LIST IoResourceList;
    PIO_RESOURCE_DESCRIPTOR IoResourceDescriptor;
    PIO_RESOURCE_DESCRIPTOR IoResourceDescriptorEnd;
    LONG IoResourceListCount;

    if (IoResources == NULL) {
        return;
    }
    IoResourceList = IoResources->List;
    IoResourceListCount = (LONG) IoResources->AlternativeLists;
    //
    // For old IO resource requirements list there is no assigned resources associated with
    // it.  We simply free the memory.
    //

    DbgPrint("** ResReqList: Interface: %x, Bus: %x, Slot: %x, AlternativeLists: %x\n",
             IoResources->InterfaceType,
             IoResources->BusNumber,
             IoResources->SlotNumber,
             IoResources->AlternativeLists);
    while (--IoResourceListCount >= 0) {
        DbgPrint ("  Alternative List: DescCount: %x\n", IoResourceList->Count);
        IoResourceDescriptor = IoResourceList->Descriptors;
        IoResourceDescriptorEnd = IoResourceDescriptor + IoResourceList->Count;
        for (; IoResourceDescriptor < IoResourceDescriptorEnd; ++IoResourceDescriptor) {
            IopDumpResourceDescriptor("    ", IoResourceDescriptor);
        }
        IoResourceList = (PIO_RESOURCE_LIST) IoResourceDescriptorEnd;
    }
    DbgPrint("\n");
}
VOID
IopDumpResourceDescriptor (
    IN PUCHAR Indent,
    IN PIO_RESOURCE_DESCRIPTOR  Desc
    )
{
    DbgPrint("%sOpt: %x, Share: %x\t", Indent, Desc->Option, Desc->ShareDisposition);
    switch (Desc->Type) {
        case CmResourceTypePort:
            DbgPrint ("IO  Min: %x:%08x, Max: %x:%08x, Algn: %x, Len %x\n",
                Desc->u.Port.MinimumAddress.HighPart, Desc->u.Port.MinimumAddress.LowPart,
                Desc->u.Port.MaximumAddress.HighPart, Desc->u.Port.MaximumAddress.LowPart,
                Desc->u.Port.Alignment,
                Desc->u.Port.Length
                );
            break;

        case CmResourceTypeMemory:
            DbgPrint ("MEM Min: %x:%08x, Max: %x:%08x, Algn: %x, Len %x\n",
                Desc->u.Memory.MinimumAddress.HighPart, Desc->u.Memory.MinimumAddress.LowPart,
                Desc->u.Memory.MaximumAddress.HighPart, Desc->u.Memory.MaximumAddress.LowPart,
                Desc->u.Memory.Alignment,
                Desc->u.Memory.Length
                );
            break;

        case CmResourceTypeInterrupt:
            DbgPrint ("INT Min: %x, Max: %x\n",
                Desc->u.Interrupt.MinimumVector,
                Desc->u.Interrupt.MaximumVector
                );
            break;

        case CmResourceTypeDma:
            DbgPrint ("DMA Min: %x, Max: %x\n",
                Desc->u.Dma.MinimumChannel,
                Desc->u.Dma.MaximumChannel
                );
            break;

        case CmResourceTypeDevicePrivate:
            DbgPrint ("DevicePrivate Data: %x, %x, %x\n",
                Desc->u.DevicePrivate.Data[0],
                Desc->u.DevicePrivate.Data[1],
                Desc->u.DevicePrivate.Data[2]
                );
            break;

        default:
            DbgPrint ("Unknown Descriptor type %x\n",
                Desc->Type
                );
            break;
    }
}
#endif
VOID
IopQueryRebalance (
    IN PDEVICE_NODE DeviceNode,
    IN ULONG Phase,
    IN PULONG RebalanceCount,
    IN PDEVICE_OBJECT **DeviceTable
    )

/*++

Routine Description:

    This routine walks hardware tree depth first.  For each device node it visits,
    it call IopQueryReconfigureDevice to query-stop device for resource
    reconfiguration.

    Note, Under rebalancing situation, all the participated devices will be asked to
    stop.  Even they support non-stopped rebalancing.

Parameters:

    DeviceNode - supplies a pionter a device node which is the root of the tree to
                 be tested.

    Phase - Supplies a value to specify the phase of the rebalance.

    RebalanceCount - supplies a pointer to a variable to receive the number of devices
                 participating the rebalance.

Return Value:

    None.

--*/

{
    LONG oldState;
    PDEVICE_OBJECT *deviceList, *deviceTable, *device;
    ULONG count;
    PDEVICE_NODE deviceNode;


    //
    // Call worker routine to get a list of devices to be rebalanced.
    //

    deviceTable = *DeviceTable;
    IopQueryRebalanceWorker (DeviceNode, Phase, RebalanceCount, DeviceTable);

    count = *RebalanceCount;
    if (count != 0 && Phase == 0) {

        //
        // At phase 0, we did not actually query-stop the device.
        // We need to do it now.
        //

        deviceList = (PDEVICE_OBJECT *)ExAllocatePoolPDO(PagedPool, count * sizeof(PDEVICE_OBJECT));
        if (deviceList == NULL) {
            *RebalanceCount = 0;
            return;
        }
        RtlMoveMemory(deviceList, deviceTable, sizeof(PDEVICE_OBJECT) * count);

        //
        // Rebuild the returned device list
        //

        *RebalanceCount = 0;
        *DeviceTable = deviceTable;
        for (device = deviceList; device < (deviceList + count); device++) {
            deviceNode = (PDEVICE_NODE)((*device)->DeviceObjectExtension->DeviceNode);
            IopQueryRebalanceWorker (deviceNode, 1, RebalanceCount, DeviceTable);
        }
        ExFreePool(deviceList);
    }
    return;
}
VOID
IopQueryRebalanceWorker (
    IN PDEVICE_NODE DeviceNode,
    IN ULONG Phase,
    IN PULONG RebalanceCount,
    IN PDEVICE_OBJECT **DeviceTable
    )

/*++

Routine Description:

    This routine walks hardware tree depth first.  For each device node it visits,
    it call IopQueryReconfigureDevice to query-stop and stop device for resource
    reconfiguration.

Parameters:

    DeviceNode - supplies a pionter a device node which is the root of the tree to
                 be tested.

    Phase - Supplies a value to specify the phase of the rebalance.

    RebalanceCount - supplies a pointer to a variable to receive the number of devices
                 participating the rebalance.

Return Value:

    None.

--*/

{
    PDEVICE_NODE node;

    if (DeviceNode == NULL)
    {
        ASSERT(DeviceNode);
        return;
    }

    //
    // Include Insufficient_resources checking.  THis is because a driver (scsiminiport) may call
    // IoReportResourceUsage to perform detection and cause the (Isapnp) enumerated device
    // insufficient_resources to start.  At this point, the enumerated device resources should be locked down
    // for the detected instance.
    //

    if (((DeviceNode->Flags & DNF_ASYNC_REQUEST_PENDING)  ||
        (DeviceNode->Flags & DNF_STOPPED)                ||
        (IopDoesDevNodeHaveProblem(DeviceNode))          ||
        (DeviceNode->Flags & DNF_LEGACY_DRIVER)          ||
        (DeviceNode->Flags & DNF_ASSIGNING_RESOURCES))   &&
        !(DeviceNode->Flags & DNF_NEEDS_REBALANCE)) {

        node = DeviceNode->Sibling;

    } else {

        node = DeviceNode;

    }

    for (; node; node = node->Sibling) {

        if (node->Child) {

            //
            // If this subtree is not being removed, wait for current enumeration to complete.
            //

            if (node->LockCount == 0) {

                KeWaitForSingleObject( &node->EnumerationMutex,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);

                IopQueryRebalanceWorker (node->Child, Phase, RebalanceCount, DeviceTable);

                KeSetEvent(&node->EnumerationMutex, 0, FALSE);
            }
        }
    }

    for (node = DeviceNode; node; node = node->Sibling) {

        if (node->LockCount == 0) {

            IopTestForReconfiguration (node, Phase, RebalanceCount, DeviceTable);

        } else {

            DebugMessage(DUMP_ERROR, ("PNPRES: %ws enum lock is taken, skipping during REBALANCE!\n", node->InstancePath.Buffer));

        }
    }
}

VOID
IopTestForReconfiguration (
    IN PDEVICE_NODE DeviceNode,
    IN ULONG Phase,
    IN PULONG RebalanceCount,
    IN PDEVICE_OBJECT **DeviceTable
    )


/*++

Routine Description:

    This routine query-stops a device which is started and owns resources.
    Note the resources for the device are not released at this point.

Parameters:

    DeviceNode - supplies a pointer to the device node to be tested for reconfiguration.

    Phase - Supplies a value to specify the phase of the rebalance.

    RebalanceCount - supplies a pointer to a variable to receive the number of devices
                 participating the rebalance.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PDEVICE_NODE nodex;
    NTSTATUS status;
    BOOLEAN addToList = FALSE;

    //
    // We still need to perform the test for sibling nodes.
    //

    if ((DeviceNode->Flags & DNF_ASYNC_REQUEST_PENDING)  ||
        (DeviceNode->Flags & DNF_STOPPED)                ||
        (IopDoesDevNodeHaveProblem(DeviceNode))          ||
        (DeviceNode->Flags & DNF_LEGACY_DRIVER)          ||
        (DeviceNode->Flags & DNF_ASSIGNING_RESOURCES)) {
        return;
    }

    if (Phase == 0) {

        //
        // At phase zero, this routine only wants to find out which devices's resource
        // requirements lists chagned.  No one actually gets stopped.
        //

        if (DeviceNode->Flags & DNF_RESOURCE_REQUIREMENTS_CHANGED &&
            !(DeviceNode->Flags & DNF_NON_STOPPED_REBALANCE) ) {

            //
            // It's too hard to handle non-stop rebalancing devices during rebalance.
            // So, We will skip it.
            //

            addToList = TRUE;
        } else {

            if (DeviceNode->Flags & DNF_STARTED) {
                status = IopQueryReconfiguration (IRP_MN_QUERY_STOP_DEVICE, DeviceNode->PhysicalDeviceObject);
                if (NT_SUCCESS(status)) {
                    if (status == STATUS_RESOURCE_REQUIREMENTS_CHANGED) {

                        //
                        // If we find out a device's resource requirements changed this way,
                        // it will be stopped and reassigned resources even if it supports
                        // non-stopped rebalance.
                        //

                        DeviceNode->Flags |= DNF_RESOURCE_REQUIREMENTS_CHANGED;
                        addToList = TRUE;
                    }
                }
                IopQueryReconfiguration (IRP_MN_CANCEL_STOP_DEVICE, DeviceNode->PhysicalDeviceObject);
            }
        }
        if (addToList) {
            *RebalanceCount = *RebalanceCount + 1;
            **DeviceTable = DeviceNode->PhysicalDeviceObject;
            *DeviceTable = *DeviceTable + 1;
        }
    } else {

        //
        // Phase 1
        //

        if (DeviceNode->Flags & DNF_STARTED) {

            //
            // Make sure all the resources required children of the DeviceNode are stopped.
            //

            nodex = DeviceNode->Child;
            while (nodex) {
                if (!(nodex->Flags & (DNF_STARTED | DNF_ASSIGNING_RESOURCES)) ||
                    (nodex->Flags & DNF_STOPPED) ||
                    (nodex->Flags & DNF_NEEDS_REBALANCE)) {
                    nodex = nodex->Sibling;
                } else {
                    break;
                }
            }
            if (nodex) {

                //
                // If any resource required child of the DeviceNode does not stopped,
                // we won't ask the DeviceNode to stop.
                // BUGBUG: We may want to restart the stopped subtrees.
                //

                DebugMessage(DUMP_INFO, ("Rebalance: Child %ws not stopped for %ws\n", nodex->InstancePath.Buffer, DeviceNode->InstancePath.Buffer));
                return;
            }
        } else if ((DeviceNode->Flags & DNF_HAS_BOOT_CONFIG) == 0 ||
                   !(DeviceNode->Flags & DNF_ADDED) ||
                   DeviceNode->Flags & DNF_MADEUP) {

            //
            // The device is not started and has no boot config.  There is no need to query-stop it.
            // Or if the device has BOOT config but there is no driver installed for it.  We don't query
            // stop it. (There may be legacy drivers are using the resources.)
            // We also don't want to query stop root enumerated devices (for performance reason.)
            // BUGBUG - We really want to query stop the root enumerated devices which do not have
            //          Boot Config and have resource requirement alternatives.  (But today some legacy
            //          driver installers do not create boot resources.)
            //

            return;
        }
        status = IopQueryReconfiguration (IRP_MN_QUERY_STOP_DEVICE, DeviceNode->PhysicalDeviceObject);
        if (NT_SUCCESS(status)) {
            DebugMessage(DUMP_INFO, ("Rebalance: %ws succeeded QueryStop\n", DeviceNode->InstancePath.Buffer));
            if (DeviceNode->Flags & DNF_STARTED) {

                //DeviceNode->Flags &= ~DNF_STARTED;
                DeviceNode->Flags |= DNF_STOPPED;
                *RebalanceCount = *RebalanceCount + 1;
                **DeviceTable = DeviceNode->PhysicalDeviceObject;

                //
                // Add a reference to the device object such that it won't disapear during rebalance.
                //

                ObReferenceObject(DeviceNode->PhysicalDeviceObject);
                *DeviceTable = *DeviceTable + 1;
            } else {

                //
                // We need to release the device's prealloc boot config.  This device will NOT
                // participate in resource rebalancing.
                //

                ASSERT(DeviceNode->Flags & DNF_HAS_BOOT_CONFIG);
                status = IopQueryReconfiguration (IRP_MN_STOP_DEVICE, DeviceNode->PhysicalDeviceObject);
                ASSERT(NT_SUCCESS(status));
                IopReleaseBootResources(DeviceNode);

                //
                // Reset BOOT CONFIG flags.  DO NOT set DNF_STOPPED.
                //

                DeviceNode->Flags &= ~(DNF_HAS_BOOT_CONFIG + DNF_BOOT_CONFIG_RESERVED);
            }
        } else {
            IopQueryReconfiguration (IRP_MN_CANCEL_STOP_DEVICE, DeviceNode->PhysicalDeviceObject);
        }
    }

}

NTSTATUS
IopPlacementForRebalance (
    IN PDEVICE_NODE DeviceNode,
    IN ARBITER_ACTION ArbiterAction
    )

/*++

Routine Description:

    This routine walks the device tree bredth-first to arbitrate resources for
    device node it visits.

Parameters:

    DeviceNode - supplies a pointer to a device node whoes subtree needs to be rebalanced.

    ArbiterAction - specifies TEST or RETEST.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_NODE node;

    //
    // Perform breadth-first.  Arbitrate resource for the current device node;
    // Place resources for its sibling subtree and then child subtree.
    //

    node = DeviceNode;
    while (node) {

        //
        // Arbitrate device resources iff its not locked for remove.
        //

        if (node->LockCount == 0) {

            status = IopArbitrateDeviceResources (node, ArbiterAction);
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }
        node = node->Sibling;
    }
    node = DeviceNode;
    while (node) {

        //
        // If this subtree is not being removed, then process it.
        //

        if (node->Child && node->LockCount == 0) {

            status = IopPlacementForRebalance (node->Child, ArbiterAction);
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }
        node = node->Sibling;
    }

    return status;
}

NTSTATUS
IopArbitrateDeviceResources (
    IN PDEVICE_NODE DeviceNode,
    IN ARBITER_ACTION ArbiterAction
    )

/*++

Routine Description:

    This routine

Parameters:

    P1 -

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PLIST_ENTRY listEntry, listHead;
    PPI_RESOURCE_ARBITER_ENTRY arbiterEntry;
    ULONG count = 0;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDesc = NULL;
    NTSTATUS status;

    ASSERT((ArbiterAction == ArbiterActionTestAllocation)   ||
           (ArbiterAction == ArbiterActionRetestAllocation) ||
           (ArbiterAction == ArbiterActionCommitAllocation));


    listHead = &DeviceNode->DeviceArbiterList;
    listEntry = listHead->Flink;
    while (listEntry != listHead) {
        arbiterEntry = CONTAINING_RECORD(listEntry, PI_RESOURCE_ARBITER_ENTRY, DeviceArbiterList);
        listEntry = listEntry->Flink;
        if (IsListEmpty(&arbiterEntry->ResourceList) == FALSE) {

            if (ArbiterAction == ArbiterActionCommitAllocation) {

                status = IopCallArbiter(arbiterEntry, ArbiterActionCommitAllocation, NULL, NULL, NULL);
                ASSERT(status == STATUS_SUCCESS);
                arbiterEntry->State = 0;
                InitializeListHead(&arbiterEntry->ActiveArbiterList);
                InitializeListHead(&arbiterEntry->BestConfig);
                InitializeListHead(&arbiterEntry->ResourceList);
                InitializeListHead(&arbiterEntry->BestResourceList);

            } else if (arbiterEntry->ResourcesChanged == FALSE) {

                //
                // If the resource requirements are the same and it failed before, we know it
                // won't be able to succeed.  So, return failure.
                //

                if (arbiterEntry->State & PI_ARBITER_TEST_FAILED) {
                    return STATUS_UNSUCCESSFUL;
                }
            } else {


                //
                // If the resource requirements are changed, we need to call arbiter to test it.
                // First find out what resources *could* be owned by the arbiter.
                // And then call the Arbiter to try to satisfy the request from the resources
                // it *could* have.

                status = IopFindResourcesForArbiter(
                                        DeviceNode,
                                        arbiterEntry->ResourceType,
                                        &count,
                                        &cmDesc
                                        );
                if (!NT_SUCCESS(status)) {
                    DebugMessage(DUMP_ERROR, ("Rebalance: Failed to find required resources for Arbiter\n"));
                    return status;
                }
                status = IopCallArbiter(arbiterEntry,
                                        ArbiterAction,
                                        &arbiterEntry->ResourceList,
                                        (PVOID)ULongToPtr(count),
                                        cmDesc
                                        );
                if (cmDesc) {
                    ExFreePool(cmDesc);
                }
                if (!NT_SUCCESS(status)) {
                    arbiterEntry->State |= PI_ARBITER_TEST_FAILED;
                    return status;
                } else {
                    arbiterEntry->State &= ~PI_ARBITER_TEST_FAILED;
                    arbiterEntry->ResourcesChanged = FALSE;
                    if (ArbiterAction == ArbiterActionTestAllocation) {
                        arbiterEntry->State |= PI_ARBITER_HAS_SOMETHING;
                    }
                }
            }
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
IopFindResourcesForArbiter (
    IN PDEVICE_NODE DeviceNode,
    IN UCHAR ResourceType,
    OUT ULONG *Count,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR *CmDesc
    )

/*++

Routine Description:

    This routine returns the resources required by the ResourceType arbiter in DeviceNode.

Parameters:

    DeviceNode -specifies the device node whose ResourceType arbiter is requesting for resources

    ResourceType - specifies the resource type

    Count - specifies a pointer to a varaible to receive the count of Cm descriptors returned

    CmDesc - specifies a pointer to a varibble to receive the returned cm descriptor.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PIOP_RESOURCE_REQUEST assignEntry;
    PREQ_ALTERNATIVE reqAlternative;
    PREQ_DESC reqDesc;
    ULONG i, count = 0;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDescriptor;

    *Count = 0;
    *CmDesc = NULL;
    if (DeviceNode->Flags & DNF_STARTED) {
        return STATUS_SUCCESS;
    }

    //
    // Find this device node's IOP_RESOURCE_REQUEST structure first
    //

    for (assignEntry = PiAssignTable + PiAssignTableCount - 1;
         assignEntry >= PiAssignTable;
         assignEntry--) {
        if (assignEntry->PhysicalDevice == DeviceNode->PhysicalDeviceObject) {
            break;
        }
    }
    if (assignEntry < PiAssignTable) {
        DebugMessage(DUMP_ERROR, ("Rebalance: No resreqlist for Arbiter? Can not find Arbiter assign table entry\n"));
        return STATUS_UNSUCCESSFUL;
    }

    reqAlternative = *((PREQ_LIST)assignEntry->ReqList)->SelectedAlternative;
    for (i = 0; i < reqAlternative->ReqDescCount; i++) {
        reqDesc = reqAlternative->ReqDescTable[i]->TranslatedReqDesc;
        if (reqDesc->Allocation.Type == ResourceType) {
            count++;
        }
    }

    cmDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ExAllocatePoolPRD(
                       PagedPool,
                       sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * count
                       );
    if (!cmDescriptor) {

        //
        // If we can not find memory, the resources will not be committed by arbiter.
        //

        DebugMessage(DUMP_ERROR, ("Rebalance: Not enough memory to perform rebalance\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *Count = count;
    *CmDesc = cmDescriptor;

    for (i = 0; i < reqAlternative->ReqDescCount; i++) {
        reqDesc = reqAlternative->ReqDescTable[i]->TranslatedReqDesc;
        if (reqDesc->Allocation.Type == ResourceType) {
            *cmDescriptor = reqDesc->Allocation;
            cmDescriptor++;
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
IopRebalance (
    IN ULONG AssignTableCount,
    IN PIOP_RESOURCE_REQUEST AssignTable
    )


/*++

Routine Description:

    This routine performs rebalancing operation.  There are two rebalance phases:
    In the phase 0, we only consider the devices whoes resource requirements changed
    and their children; in phase 1, we consider anyone who succeeds the query-stop.

Parameters:

    AssignTableCount,
    AssignTable - Supplies the number of origianl AssignTableCout and AssignTable which
                  triggers the rebalance operation.

        (if AssignTableCount == 0, we are processing device state change.)

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    ULONG count, i;
    PIOP_RESOURCE_REQUEST table = NULL, tableEnd, newEntry;
    PIOP_RESOURCE_REQUEST requestTable = NULL, requestTableEnd, entry1, entry2;
    ULONG phase0RebalanceCount = 0, rebalanceCount = 0, deviceCount;
    NTSTATUS status, statusx;
    PDEVICE_OBJECT *deviceTable, *deviceTablex;
    PDEVICE_NODE deviceNode;
    ULONG rebalancePhase = 0;

    //
    // Query all the device nodes to see who are willing to participate the rebalance
    // process.
    //

    deviceTable = (PDEVICE_OBJECT *) ExAllocatePoolPDO(
                      PagedPool,
                      sizeof(PDEVICE_OBJECT) * IopNumberDeviceNodes);
    if (deviceTable == NULL) {
        DebugMessage(DUMP_ERROR, ("Rebalance: Not enough memory to perform rebalance\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }


tryAgain:
    deviceTablex = deviceTable + phase0RebalanceCount;

    //
    // Walk device node tree depth-first to query-stop and stop devices.
    // At this point the resources of the stopped devices are not released yet.
    // Also, the leaf nodes are in the front of the device table and non leaf nodes
    // are at the end of the table.
    //

    IopQueryRebalance (IopRootDeviceNode, rebalancePhase, &rebalanceCount, &deviceTablex);
    if (rebalanceCount == 0) {

        //
        // If no one is interested and we are not processing resources req change,
        // move to next phase.
        //

        if (rebalancePhase == 0 && AssignTableCount != 0) {
            rebalancePhase = 1;
            goto tryAgain;
        }
        DebugMessage(DUMP_INFO, ("Rebalance: No device participates in rebalance phase %x\n", rebalancePhase));
        ExFreePool(deviceTable);
        deviceTable = NULL;
        status = STATUS_UNSUCCESSFUL;
        goto exit;
    }
    if (rebalanceCount == phase0RebalanceCount) {

        //
        // Phase 0 failed and no new device participates. failed the rebalance.
        //

        status = STATUS_UNSUCCESSFUL;
        goto exit;
    }
    if (rebalancePhase == 0) {
        phase0RebalanceCount = rebalanceCount;
    }

    //
    // Allocate pool for the new reconfiguration requests and the original requests.
    //

    table = (PIOP_RESOURCE_REQUEST) ExAllocatePoolIORR(
                 PagedPool,
                 sizeof(IOP_RESOURCE_REQUEST) * (AssignTableCount + rebalanceCount)
                 );
    if (table == NULL) {
        DebugMessage(DUMP_ERROR, ("Rebalance: Not enough memory to perform rebalance\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    tableEnd = table + AssignTableCount + rebalanceCount;

    //
    // Build a new resource request table.  The original requests will be at the beginning
    // of the table and new requests (reconfigured devices) are at the end.
    // After the new request table is built, the leaf nodes will be in front of the table,
    // and non leaf nodes will be close to the end of the table.  This is for optimization.
    //

    //
    // Copy the original request to the front of our new request table.
    //

    if (AssignTableCount != 0) {
        RtlMoveMemory(table, AssignTable, sizeof(IOP_RESOURCE_REQUEST) * AssignTableCount);
    }

    //
    // Initialize all the new entries of our new request table,
    //

    newEntry = table + AssignTableCount;
    RtlZeroMemory(newEntry, sizeof(IOP_RESOURCE_REQUEST) * rebalanceCount);
    for (i = 0, deviceTablex = deviceTable; i < rebalanceCount; i++, deviceTablex++) {
        newEntry[i].AllocationType = ArbiterRequestPnpEnumerated;
        newEntry[i].PhysicalDevice = *deviceTablex;
    }

    status = IopGetResourceRequirementsForAssignTable(
                 newEntry,
                 tableEnd ,
                 &deviceCount);
    if (!NT_SUCCESS(status) || deviceCount == 0) {
         DebugMessage(DUMP_ERROR, ("Rebalance: GetResourceRequirementsForAssignTable failed\n"));
         status = NT_SUCCESS(status)? STATUS_UNSUCCESSFUL : status;
         goto exit;
    }

    //
    // Process the AssignTable to remove any entry which is marked as IOP_ASSIGN_IGNORE
    //

    if (deviceCount != rebalanceCount) {

        deviceCount += AssignTableCount;
        requestTable = (PIOP_RESOURCE_REQUEST) ExAllocatePoolIORR(
                             PagedPool,
                             sizeof(IOP_RESOURCE_REQUEST) * deviceCount
                             );
        if (requestTable == NULL) {
            IopFreeResourceRequirementsForAssignTable(newEntry, tableEnd);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }
        for (entry1 = table, entry2 = requestTable; entry1 < tableEnd; entry1++) {
            if (!(entry1->Flags & IOP_ASSIGN_IGNORE)) {
                *entry2 = *entry1;
                entry2++;
            } else {
                //
                // BUGBUG!!! ??? (jamiehun)
                // if this assert fails, parts of this code are broken
                //
                ASSERT(entry1 >= newEntry);
            }
        }
        requestTableEnd = requestTable + deviceCount;
    } else {
        requestTable = table;
        requestTableEnd = tableEnd;
        deviceCount += AssignTableCount;
    }

    //
    // DO NOT Sort the AssignTable
    //

    //IopRearrangeAssignTable(requestTable, deviceCount);

#if 0

    //
    // We are about to perform rebalance.  Release the resources of the reconfiguration devices
    //

    for (entry1 = newEntry; entry1 < tableEnd; entry1++) {
        if (!(entry1->Flags & IOP_ASSIGN_IGNORE) &&
            !(entry1->Flags & IOP_ASSIGN_RESOURCES_RELEASED)) {
            deviceNode = (PDEVICE_NODE)entry1->PhysicalDevice->DeviceObjectExtension->DeviceNode;
            if (deviceNode->ResourceList) {

                //
                // Call IopReleaseResourcesInternal instead of IopReleaseResources such that
                // the pool for devicenode->ResourceList is not freed.  We need it to restart
                // the reconfigured devices in case rebalance failed.
                //

                IopReleaseResourcesInternal(deviceNode);
                entry1->Flags |= IOP_ASSIGN_RESOURCES_RELEASED;
            }
        }
    }

#endif

    //
    // Assign the resources. If we succeed, or if
    // there is a memory shortage return immediately.
    //

    status = IopAssignInner(deviceCount, requestTable, TRUE);
    if (NT_SUCCESS(status)) {

        //
        // If the rebalance succeeded, we need to restart all the reconfigured devices.
        // For the original devices, we will return and let IopAllocateResources to deal
        // with them.
        //

        IopBuildCmResourceLists(requestTable, requestTableEnd);

        //
        // Copy the new status back to the original AssignTable.
        //

        if (AssignTableCount != 0) {
            RtlMoveMemory(AssignTable, requestTable, sizeof(IOP_RESOURCE_REQUEST) * AssignTableCount);
        }
        //
        // free resource requirements we allocated while here
        //
        IopFreeResourceRequirementsForAssignTable(requestTable+AssignTableCount, requestTableEnd);

        if (table != requestTable) {

            //
            // If we switched request table ... copy the contents of new table back to
            // the old table.
            //

            for (entry1 = table, entry2 = requestTable; entry2 < requestTableEnd;) {

                if (entry1->Flags & IOP_ASSIGN_IGNORE) {
                    entry1++;
                    continue;
                }
                *entry1 = *entry2;
                if (entry2->Flags & IOP_ASSIGN_EXCLUDE) {
                    entry1->Status = STATUS_CONFLICTING_ADDRESSES;
                }
                entry2++;
                entry1++;
            }
        }

        //
        // Go thru the origianl request table to stop each query-stopped/reconfigured device.
        //

        DebugMessage(DUMP_DETAIL, ("PnpRes: STOP reconfigured devices during REBALANCE.\n"));

        for (entry1 = newEntry; entry1 < tableEnd; entry1++) {
            if (NT_SUCCESS(entry1->Status)) {
                IopQueryReconfiguration (IRP_MN_STOP_DEVICE, entry1->PhysicalDevice);
            } else {
                IopQueryReconfiguration (IRP_MN_CANCEL_STOP_DEVICE, entry1->PhysicalDevice);
                deviceNode = (PDEVICE_NODE)entry1->PhysicalDevice->DeviceObjectExtension->DeviceNode;
                deviceNode->Flags &= ~DNF_STOPPED;
            }
        }

        //
        // Commit the allocation AFTER stopping rebalance candidates.
        //

        DebugMessage(DUMP_DETAIL, ("PnpRes: Commit the new allocation during REBALANCE.\n"));

        IopPlacement(ArbiterActionCommitAllocation, TRUE);

#if DBG_SCOPE
        if (PnpResDebugLevel & STOP_ERROR) {
            IopCheckDataStructures(IopRootDeviceNode);
        }
#endif

        //
        // Go thru the origianl request table to start each stopped/reconfigured device.
        //

        for (entry1 = tableEnd - 1; entry1 >= newEntry; entry1--) {
            deviceNode = (PDEVICE_NODE)entry1->PhysicalDevice->DeviceObjectExtension->DeviceNode;

            if (NT_SUCCESS(entry1->Status)) {

                //
                // We need to release the pool space for ResourceList and ResourceListTranslated.
                // Because the earlier IopReleaseResourcesInternal does not release the pool.
                //

                if (deviceNode->ResourceList) {
                    ExFreePool(deviceNode->ResourceList);
                }
                deviceNode->ResourceList = entry1->ResourceAssignment;
                if (deviceNode->ResourceListTranslated) {
                    ExFreePool(deviceNode->ResourceListTranslated);
                }
                deviceNode->ResourceListTranslated = entry1->TranslatedResourceAssignment;
                if (deviceNode->ResourceList) {
                    deviceNode->Flags |= DNF_RESOURCE_ASSIGNED;
                    deviceNode->Flags &= ~DNF_RESOURCE_REPORTED;
                } else {
                    deviceNode->Flags |= DNF_NO_RESOURCE_REQUIRED;
                }
                if (entry1->Flags & IOP_ASSIGN_CLEAR_RESOURCE_REQUIREMENTS_CHANGE_FLAG) {

                    //
                    // If we are processing the resource requirements change request,
                    // clear its related flags.
                    //

                    deviceNode->Flags &= ~(DNF_RESOURCE_REQUIREMENTS_CHANGED | DNF_NON_STOPPED_REBALANCE);
                }

                //
                // Some drivers (like ndis) may want to do resource allocation during START.
                // So let go of the IopRegistrySemaphore during the start.
                //

                KeReleaseSemaphore(&IopRegistrySemaphore, 0, 1, FALSE);
                KeLeaveCriticalRegion();
                IopStartDevice(entry1->PhysicalDevice);
                KeEnterCriticalRegion();
                KeWaitForSingleObject(  &IopRegistrySemaphore,
                                        DelayExecution,
                                        KernelMode,
                                        FALSE,
                                        NULL );
            }
        }

        //
        // Finally release the references of the reconfigured device objects
        //

        for (deviceTablex = (deviceTable + rebalanceCount - 1);
             deviceTablex >= deviceTable;
             deviceTablex--) {
             ObDereferenceObject(*deviceTablex);
        }
        status = STATUS_SUCCESS;
    } else {

        //
        // Rebalance failed. Free our internal representation of the rebalance
        // candidates' resource requirements lists.
        // BugBug - should optimize the code.
        //

        IopFreeResourceRequirementsForAssignTable(requestTable + AssignTableCount, requestTableEnd);
        if (rebalancePhase == 0) {
            rebalancePhase++;
            if (requestTable) {
                ExFreePool(requestTable);
            }
            if (table && (table != requestTable)) {
                ExFreePool(table);
            }
            table = requestTable = NULL;
            goto tryAgain;
        }

        //
        // Rebalance failed.  Restore the resources for the reconfiguration participated devices.
        // Note, for some devices, their resource requirements may already changed.  In this
        // case, the resources we restore do not reflect the new requirements.
        //

        for (deviceTablex = (deviceTable + rebalanceCount - 1);
             deviceTablex >= deviceTable;
             deviceTablex--) {
             deviceNode = (PDEVICE_NODE)((*deviceTablex)->DeviceObjectExtension->DeviceNode);
#if 0
             statusx = IopRestoreResourcesInternal(deviceNode);
             if (NT_SUCCESS(statusx)) {
                 IopQueryReconfiguration (IRP_MN_CANCEL_STOP_DEVICE, *deviceTablex);
                 deviceNode->Flags &= ~DNF_STOPPED;
             } else {
                 ASSERT(0);
                 IopRequestDeviceRemoval(*deviceTablex);
             }
#else
             IopQueryReconfiguration (IRP_MN_CANCEL_STOP_DEVICE, *deviceTablex);
             deviceNode->Flags &= ~DNF_STOPPED;
#endif
             ObDereferenceObject(*deviceTablex);
        }
    }
    ExFreePool(deviceTable);
    deviceTable = NULL;

exit:

    if (!NT_SUCCESS(status) && deviceTable) {

        //
        // If we failed before trying to perform resource assignment,
        // we will end up here.
        //

        DebugMessage(DUMP_INFO, ("Rebalance: Rebalance failed\n"));

        //
        // Somehow we failed to start the rebalance operation.
        // We will cancel the query-stop request for the query-stopped devices bredth first.
        //

        for (deviceTablex = (deviceTable + rebalanceCount - 1);
             deviceTablex >= deviceTable;
             deviceTablex--) {

             deviceNode = (PDEVICE_NODE)((*deviceTablex)->DeviceObjectExtension->DeviceNode);
             IopQueryReconfiguration (IRP_MN_CANCEL_STOP_DEVICE, *deviceTablex);
             deviceNode->Flags &= ~DNF_STOPPED;
             ObDereferenceObject(*deviceTablex);
        }
    }
    if (deviceTable) {
        ExFreePool(deviceTable);
    }
    if (requestTable) {
        ExFreePool(requestTable);
    }
    if (table && (table != requestTable)) {
        ExFreePool(table);
    }
    return status;
}

NTSTATUS
IopRestoreResourcesInternal (
    IN PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    This routine reassigns the released resources for device specified by DeviceNode.

Parameters:

    DeviceNode - specifies the device node whose resources are goint to be released.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    IOP_RESOURCE_REQUEST requestTable;
    NTSTATUS status;

    if (DeviceNode->ResourceList == NULL) {
        return STATUS_SUCCESS;
    }
    requestTable.ResourceRequirements =
        IopCmResourcesToIoResources (0, DeviceNode->ResourceList, LCPRI_FORCECONFIG);
    if (requestTable.ResourceRequirements == NULL) {
        DebugMessage(DUMP_ERROR, ("PnpRes: Not enough memory to clean up rebalance failure\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    requestTable.Priority = 0;
    requestTable.Flags = 0;
    requestTable.AllocationType = ArbiterRequestPnpEnumerated;
    requestTable.PhysicalDevice = DeviceNode->PhysicalDeviceObject;
    requestTable.ReqList = NULL;
    requestTable.ResourceAssignment = NULL;
    requestTable.TranslatedResourceAssignment = NULL;
    requestTable.Status = 0;

    //
    // rebuild internal representation of the resource requirements list
    //

    status = IopResourceRequirementsListToReqList(
                    requestTable.AllocationType,
                    requestTable.ResourceRequirements,
                    requestTable.PhysicalDevice,
                    &requestTable.ReqList);

    if (!NT_SUCCESS(status) || requestTable.ReqList == NULL) {
        DebugMessage(DUMP_ERROR, ("PnpRes: Not enough memory to restore previous resources\n"));
        ExFreePool (requestTable.ResourceRequirements);
        return status;
    } else {
        PREQ_LIST reqList;

        reqList = (PREQ_LIST)requestTable.ReqList;
        reqList->AssignEntry = &requestTable;

        //
        // Sort the ReqList such that the higher priority Alternative list are
        // placed in the front of the list.
        //

        IopRearrangeReqList(reqList);
        if (reqList->ReqBestAlternative == NULL) {

            IopFreeResourceRequirementsForAssignTable(&requestTable, (&requestTable) + 1);
            return STATUS_DEVICE_CONFIGURATION_ERROR;

        }
    }

    status = IopAssignInner(1, &requestTable, FALSE);
    IopFreeResourceRequirementsForAssignTable(&requestTable, (&requestTable) + 1);
    if (!NT_SUCCESS(status)) {
        DbgPrint("IopRestoreResourcesInternal: BOOT conflict for %ws\n", DeviceNode->InstancePath.Buffer);
        ASSERT(NT_SUCCESS(status));
    }
    if (requestTable.ResourceAssignment) {
        ExFreePool(requestTable.ResourceAssignment);
    }
    if (requestTable.TranslatedResourceAssignment) {
        ExFreePool(requestTable.TranslatedResourceAssignment);
    }
    IopWriteAllocatedResourcesToRegistry (
        DeviceNode,
        DeviceNode->ResourceList,
        IopDetermineResourceListSize(DeviceNode->ResourceList)
        );
    return status;
}

VOID
IopReleaseResourcesInternal (
    IN PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    This routine releases the assigned resources for device specified by DeviceNode.
    Note, this routine does not reset the resource related fields in DeviceNode structure.

Parameters:

    DeviceNode - specifies the device node whose resources are goint to be released.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PDEVICE_NODE device;
    PLIST_ENTRY listHead, listEntry;
    PPI_RESOURCE_ARBITER_ENTRY arbiterEntry;
    ARBITER_LIST_ENTRY arbiterListEntry;
    INTERFACE_TYPE interfaceType;
    ULONG busNumber, listCount, i, j, size;
    PCM_RESOURCE_LIST resourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR cmFullDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmPartDesc;
    BOOLEAN search = TRUE;
#if DBG
    NTSTATUS status;
#endif

    InitializeListHead(&arbiterListEntry.ListEntry);
    arbiterListEntry.AlternativeCount = 0;
    arbiterListEntry.Alternatives = NULL;
    arbiterListEntry.PhysicalDeviceObject = DeviceNode->PhysicalDeviceObject;
    arbiterListEntry.Flags = 0;
    arbiterListEntry.WorkSpace = 0;
    arbiterListEntry.Assignment = NULL;
    arbiterListEntry.RequestSource = ArbiterRequestPnpEnumerated;

    resourceList = DeviceNode->ResourceList;
    if (resourceList == NULL) {
        resourceList = DeviceNode->BootResources;
    }
    if (resourceList && resourceList->Count > 0) {
        listCount = resourceList->Count;
        cmFullDesc = &resourceList->List[0];
    } else {
        listCount = 1;
        resourceList = NULL;
    }
    for (i = 0; i < listCount; i++) {

        if (resourceList) {
            interfaceType = cmFullDesc->InterfaceType;
            busNumber = cmFullDesc->BusNumber;
            if (interfaceType == InterfaceTypeUndefined) {
                interfaceType = PnpDefaultInterfaceType;
            }
        } else {
            interfaceType = PnpDefaultInterfaceType;
            busNumber = 0;
        }

        device = DeviceNode->Parent;
        while (device) {
            if ((device == IopRootDeviceNode) && search) {
                device = IopFindBusDeviceNode (
                                 IopRootDeviceNode,
                                 interfaceType,
                                 busNumber,
                                 0
                                 );

                //
                // If we did not find a PDO, try again with InterfaceType == Isa. This allows
                // drivers that request Internal to get resources even if there is no PDO
                // that is Internal. (but if there is an Internal PDO, they get that one)
                //

                if ((device == IopRootDeviceNode) && (interfaceType == Internal)) {
                    device = IopFindBusDeviceNode(IopRootDeviceNode, Isa, 0, 0);
                }
                search = FALSE;

            }
            listHead = &device->DeviceArbiterList;
            listEntry = listHead->Flink;
            while (listEntry != listHead) {
                arbiterEntry = CONTAINING_RECORD(listEntry, PI_RESOURCE_ARBITER_ENTRY, DeviceArbiterList);
                if (arbiterEntry->ArbiterInterface != NULL) {
                    search = FALSE;
                    ASSERT(IsListEmpty(&arbiterEntry->ResourceList));
                    InitializeListHead(&arbiterEntry->ResourceList);  // Recover from assert
                    InsertTailList(&arbiterEntry->ResourceList, &arbiterListEntry.ListEntry);
    #if DBG
                    status =
    #endif
                    IopCallArbiter(arbiterEntry,
                                   ArbiterActionTestAllocation,
                                   &arbiterEntry->ResourceList,
                                   NULL,
                                   NULL
                                   );
    #if DBG
                    ASSERT(status == STATUS_SUCCESS);
                    status =
    #endif
                    IopCallArbiter(arbiterEntry,
                                   ArbiterActionCommitAllocation,
                                   NULL,
                                   NULL,
                                   NULL
                                   );
    #if DBG
                    ASSERT(status == STATUS_SUCCESS);
    #endif
                    RemoveEntryList(&arbiterListEntry.ListEntry);
                    InitializeListHead(&arbiterListEntry.ListEntry);
                }
                listEntry = listEntry->Flink;
            }
            device = device->Parent;
        }

        //
        // If there are more than 1 list, move to next list
        //

        if (listCount > 1) {
            cmPartDesc = &cmFullDesc->PartialResourceList.PartialDescriptors[0];
            for (j = 0; j < cmFullDesc->PartialResourceList.Count; j++) {
                size = 0;
                switch (cmPartDesc->Type) {
                case CmResourceTypeDeviceSpecific:
                     size = cmPartDesc->u.DeviceSpecificData.DataSize;
                     break;
                }
                cmPartDesc++;
                cmPartDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmPartDesc + size);
            }
            cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmPartDesc;
        }
    }

    IopWriteAllocatedResourcesToRegistry (DeviceNode, NULL, 0);
}

NTSTATUS
IopFindLegacyDeviceNode (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    OUT PDEVICE_NODE *LegacyDeviceNode,
    OUT PDEVICE_OBJECT *LegacyPDO
    )

/*++

Routine Description:

    This routine searches for the device node and device object created for legacy resource
    allocation for the DriverObject and DeviceObject.

Parameters:

    DriverObject - specifies the driver object doing the legacy allocation.

    DeviceObject - specifies the device object.

    LegacyDeviceNode - receives the pointer to the legacy device node if found.

    LegacyDeviceObject - receives the pointer to the legacy device object if found.


Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS        status = STATUS_UNSUCCESSFUL;
    PDEVICE_NODE    deviceNode;

    ASSERT(LegacyDeviceNode && LegacyPDO);


    //
    // Use the device object if it exists.
    //

    if (DeviceObject) {

        deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
        if (deviceNode) {

            *LegacyPDO = DeviceObject;
            *LegacyDeviceNode = deviceNode;
            status = STATUS_SUCCESS;

        } else if (!(DeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE)) {

            deviceNode = IopAllocateDeviceNode(DeviceObject);
            if (deviceNode) {

                deviceNode->Flags |= DNF_LEGACY_RESOURCE_DEVICENODE;
                IopSetLegacyDeviceInstance (DriverObject, deviceNode);
                *LegacyPDO = DeviceObject;
                *LegacyDeviceNode = deviceNode;
                status = STATUS_SUCCESS;

            } else {

                DebugMessage(DUMP_ERROR, ("PNPRES: Failed to allocate device node for PDO %08X\n", DeviceObject));
                status = STATUS_INSUFFICIENT_RESOURCES;

            }

        } else {

            DebugMessage(DUMP_ERROR, ("PNPRES: %08X PDO without a device node!\n", DeviceObject));
            ASSERT(DeviceObject->DeviceObjectExtension->DeviceNode);

        }

    } else {

        //
        // Search our list of legacy device nodes.
        //

        for (   deviceNode = IopLegacyDeviceNode;
                deviceNode && deviceNode->DuplicatePDO != (PDEVICE_OBJECT)DriverObject;
                deviceNode = deviceNode->NextDeviceNode);

        if (deviceNode) {

            *LegacyPDO = deviceNode->PhysicalDeviceObject;
            *LegacyDeviceNode = deviceNode;
            status = STATUS_SUCCESS;

        } else {

            WCHAR           buffer[60];
            UNICODE_STRING  deviceName;
            LARGE_INTEGER   tickCount;
            ULONG           length;
            PDEVICE_OBJECT  pdo;

            //
            // We are seeing this for the first time.
            // Create a madeup device node.
            //

            KeQueryTickCount(&tickCount);
            length = _snwprintf(buffer, sizeof(buffer) / sizeof(WCHAR), L"\\Device\\Resource%04u%x", IopNumberDeviceNodes, tickCount.LowPart);
            deviceName.MaximumLength = sizeof(buffer);
            deviceName.Length = (USHORT)(length * sizeof(WCHAR));
            deviceName.Buffer = buffer;

            DebugMessage(DUMP_INFO, ("PNPRES: Creating dummy PDO %ws\n", deviceName.Buffer));

            status = IoCreateDevice( IoPnpDriverObject,
                                     sizeof(IOPNP_DEVICE_EXTENSION),
                                     &deviceName,
                                     FILE_DEVICE_CONTROLLER,
                                     0,
                                     FALSE,
                                     &pdo);

            if (NT_SUCCESS(status)) {

                pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;
                deviceNode = IopAllocateDeviceNode(pdo);
                if (deviceNode) {

                    //
                    // Change driver object to the caller even though the owner
                    // of the pdo is IoPnpDriverObject.  This is to support
                    // DriverExclusive for legacy interface.
                    //

                    pdo->DriverObject = DriverObject;
                    deviceNode->Flags = DNF_MADEUP | DNF_PROCESSED | DNF_LEGACY_RESOURCE_DEVICENODE;
                    deviceNode->DuplicatePDO = (PDEVICE_OBJECT)DriverObject;
                    IopSetLegacyDeviceInstance (DriverObject, deviceNode);

                    //
                    // Add it to our list of legacy device nodes rather than adding it to the HW tree.
                    //

                    deviceNode->NextDeviceNode = IopLegacyDeviceNode;
                    if (IopLegacyDeviceNode) {

                        IopLegacyDeviceNode->PreviousDeviceNode = deviceNode;

                    }
                    IopLegacyDeviceNode = deviceNode;
                    *LegacyPDO = pdo;
                    *LegacyDeviceNode = deviceNode;

                } else {

                    DebugMessage(DUMP_ERROR, ("PNPRES: Failed to allocate device node for PDO %08X\n", pdo));
                    IoDeleteDevice(pdo);
                    status = STATUS_INSUFFICIENT_RESOURCES;

                }

            } else {

                DebugMessage(DUMP_ERROR, ("PNPRES: IoCreateDevice failed for %ws with status %08X\n", deviceName.Buffer, status));

            }
        }
    }

    return status;
}

VOID
IopRemoveLegacyDeviceNode (
    IN PDEVICE_OBJECT   DeviceObject OPTIONAL,
    IN PDEVICE_NODE     LegacyDeviceNode
    )

/*++

Routine Description:

    This routine removes the device node and device object created for legacy resource
    allocation for the DeviceObject.

Parameters:

    DeviceObject - specifies the device object.

    LegacyDeviceNode - receives the pointer to the legacy device node if found.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    ASSERT(LegacyDeviceNode);


    if (!DeviceObject) {

        if (LegacyDeviceNode->DuplicatePDO) {

            LegacyDeviceNode->DuplicatePDO = NULL;
            if (LegacyDeviceNode->PreviousDeviceNode) {

                LegacyDeviceNode->PreviousDeviceNode->NextDeviceNode = LegacyDeviceNode->NextDeviceNode;

            }

            if (LegacyDeviceNode->NextDeviceNode) {

                LegacyDeviceNode->NextDeviceNode->PreviousDeviceNode = LegacyDeviceNode->PreviousDeviceNode;

            }

            if (IopLegacyDeviceNode == LegacyDeviceNode) {

                IopLegacyDeviceNode = LegacyDeviceNode->NextDeviceNode;

            }

        } else {

            DebugMessage(DUMP_ERROR, ("PNPRES: %ws does not have a duplicate PDO\n", LegacyDeviceNode->InstancePath.Buffer));
            ASSERT(LegacyDeviceNode->DuplicatePDO);
            return;

        }
    }

    if (!(DeviceObject && (DeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE))) {

        PDEVICE_NODE    resourceDeviceNode;
        PDEVICE_OBJECT  pdo;

        for (   resourceDeviceNode = (PDEVICE_NODE)LegacyDeviceNode->OverUsed1.LegacyDeviceNode;
                resourceDeviceNode;
                resourceDeviceNode = resourceDeviceNode->OverUsed2.NextResourceDeviceNode) {

                if (resourceDeviceNode->OverUsed2.NextResourceDeviceNode == LegacyDeviceNode) {

                    resourceDeviceNode->OverUsed2.NextResourceDeviceNode = LegacyDeviceNode->OverUsed2.NextResourceDeviceNode;
                    break;

                }
        }

        LegacyDeviceNode->Parent = LegacyDeviceNode->Sibling =
            LegacyDeviceNode->Child = LegacyDeviceNode->LastChild = NULL;

        //
        // Delete the dummy PDO and device node.
        //

        pdo = LegacyDeviceNode->PhysicalDeviceObject;
        IopDestroyDeviceNode(LegacyDeviceNode);

        if (!DeviceObject) {

            pdo->DriverObject = IoPnpDriverObject;
            IoDeleteDevice(pdo);
        }
    }
}


NTSTATUS
IopLegacyResourceAllocation (
    IN ARBITER_REQUEST_SOURCE AllocationType,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources OPTIONAL
    )

/*++

Routine Description:

    This routine handles legacy interface IoAssignResources and IoReportResourcesUsage,
    It converts the request to call IopAllocateResources.

Parameters:

    AllocationType - Allocation type for the legacy request.

    DriverObject - Driver object doing the legacy allocation.

    DeviceObject - Device object.

    ResourceRequirements - Legacy resource requirements. If NULL, caller want to free resources.

    AllocatedResources - Pointer to a variable that receives pointer to allocated resources.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PDEVICE_OBJECT      pdo;
    PDEVICE_NODE        deviceNode;
    PDEVICE_NODE        legacyDeviceNode;
    NTSTATUS            status;
    PCM_RESOURCE_LIST   combinedResources;
    KIRQL               irql;

    ASSERT(DriverObject);

    //
    // Grab the IO registry semaphore to make sure no other device is
    // reporting it's resource usage while we are searching for conflicts.
    //

    KeEnterCriticalRegion();

    status = KeWaitForSingleObject( &IopRegistrySemaphore,
                                    DelayExecution,
                                    KernelMode,
                                    FALSE,
                                    NULL);
    if (NT_SUCCESS(status)) {

        status = IopFindLegacyDeviceNode(DriverObject, DeviceObject, &deviceNode, &pdo);
        if (NT_SUCCESS(status)) {

            legacyDeviceNode = NULL;
            if (!deviceNode->Parent && ResourceRequirements) {

                //
                // Make IopRootDeviceNode the bus pdo so we will search the right bus pdo
                // on resource descriptor level.
                //

                if (ResourceRequirements->InterfaceType == InterfaceTypeUndefined) {

                    ResourceRequirements->InterfaceType = PnpDefaultInterfaceType;

                }
                deviceNode->Parent = IopRootDeviceNode;

            }

            //
            // Release resources for this device node.
            //

            if (    (!ResourceRequirements && deviceNode->Parent) ||
                    (deviceNode->Flags & (DNF_RESOURCE_REPORTED | DNF_RESOURCE_ASSIGNED))) {

                IopReleaseResources(deviceNode);

            }

            if (ResourceRequirements) {

                IOP_RESOURCE_REQUEST    requestTable;
                IOP_RESOURCE_REQUEST    *requestTablep;
                ULONG                   count;

                //
                // Try to allocate these resource requirements.
                //

                count = 1;
                RtlZeroMemory(&requestTable, sizeof(IOP_RESOURCE_REQUEST));
                requestTable.ResourceRequirements = ResourceRequirements;
                requestTable.PhysicalDevice = pdo;
                requestTable.Flags = IOP_ASSIGN_NO_REBALANCE;
                requestTable.AllocationType =  AllocationType;

                deviceNode->Flags |= DNF_ASSIGNING_RESOURCES;
                requestTablep = &requestTable;
                IopAllocateResources(&count, &requestTablep, TRUE, TRUE);
                deviceNode->Flags &= ~DNF_ASSIGNING_RESOURCES;
                status = requestTable.Status;
                if (NT_SUCCESS(status)) {

                    deviceNode->Flags |= DNF_RESOURCE_REPORTED;
                    //deviceNode->Flags &= ~DNF_INSUFFICIENT_RESOURCES;
                    deviceNode->ResourceListTranslated = requestTable.TranslatedResourceAssignment;
                    count = IopDetermineResourceListSize((*AllocatedResources) ? *AllocatedResources : requestTable.ResourceAssignment);
                    deviceNode->ResourceList = ExAllocatePoolIORL(PagedPool, count);
                    if (deviceNode->ResourceList) {

                        if (*AllocatedResources) {

                            //
                            // We got called from IoReportResourceUsage.
                            //

                            ASSERT(requestTable.ResourceAssignment);
                            ExFreePool(requestTable.ResourceAssignment);

                        } else {

                            //
                            // We got called from IoAssignResources.
                            //

                            *AllocatedResources = requestTable.ResourceAssignment;

                        }
                        RtlCopyMemory(deviceNode->ResourceList, *AllocatedResources, count);
                        legacyDeviceNode = (PDEVICE_NODE)deviceNode->OverUsed1.LegacyDeviceNode;

                    } else {

                        deviceNode->ResourceList = requestTable.ResourceAssignment;
                        IopReleaseResources(deviceNode);
                        status = STATUS_INSUFFICIENT_RESOURCES;

                    }
                }

                //
                // Remove the madeup PDO and device node if there was some error.
                //

                if (!NT_SUCCESS(status)) {

                    IopRemoveLegacyDeviceNode(DeviceObject, deviceNode);

                }

            } else {

                //
                // Caller wants to release resources.
                //

                legacyDeviceNode = (PDEVICE_NODE)deviceNode->OverUsed1.LegacyDeviceNode;
                IopRemoveLegacyDeviceNode(DeviceObject, deviceNode);

            }

            if (NT_SUCCESS(status)) {

                if (legacyDeviceNode) {

                    //
                    // After the resource is modified, update the allocated resource list
                    // for the Root\Legacy_xxxx\0000 device instance.
                    //

                    combinedResources = IopCombineLegacyResources(legacyDeviceNode);
                    if (combinedResources) {

                        IopWriteAllocatedResourcesToRegistry(   legacyDeviceNode,
                                                                combinedResources,
                                                                IopDetermineResourceListSize(combinedResources));
                        ExFreePool(combinedResources);
                    }
                }

                //
                // BUGBUG: Santoshj 10/01/99
                // Since IoReportDetectedDevice always uses IoPnpDriverObject instead of the one
                // passed in, we put in this hack so that we dont taint ourselves. Other bugs need
                // to be fixed before removing this hack (dont do resource allocation if this
                // was already reported).
                //

                if (AllocationType != ArbiterRequestPnpDetected && DriverObject != IoPnpDriverObject) {
                    //
                    // Modify the DRVOBJ flags.
                    //
                    ExAcquireFastLock(&IopDatabaseLock, &irql);
                    if (ResourceRequirements) {
                        //
                        // Once tainted, a driver can never lose it's legacy history
                        // (unless unloaded). This is because the device object
                        // field is optional, and we don't bother counting here...
                        //
                        DriverObject->Flags |= DRVO_LEGACY_RESOURCES;
                    }
                    ExReleaseFastLock(&IopDatabaseLock, irql);
                }
            }
        }

        KeReleaseSemaphore(&IopRegistrySemaphore, 0, 1, FALSE);

    } else {

        DebugMessage(DUMP_ERROR, ("PNPRES: IopLegacyResourceAllocation: Failed to acquire registry semaphore, status %08X\n", status));
    }

    KeLeaveCriticalRegion();

    return status;
}

PDEVICE_NODE
IopFindBusDeviceNode (
    IN PDEVICE_NODE DeviceNode,
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber
    )

/*++

Routine Description:

    This routine finds the bus PDO which performs translation and arbitration for the
    specified InterfaceType, BusNumber and SlotNumber.

Parameters:

    DeviceNode - the root of the PDO to start our search

    InterfaceType - Specifies the PDO's interface type.

    BusNumber - specifies the PDO's bus number.

    SlotNumber - specified the PDO's slot number (UNUSED).

Return Value:

    A pointer to the BUS PDO.

--*/

{
    PDEVICE_NODE busDeviceNode = NULL;

    UNREFERENCED_PARAMETER(SlotNumber);

    if (DeviceNode && InterfaceType != InterfaceTypeUndefined) {

        if (InterfaceType == PNPBus) {
            //
            // if we specified PnpBus as InterfaceType, we know nobody specifies such a bus
            // don't waste time looking for BusNumber because it doesn't exist
            //
            busDeviceNode = NULL;
        } else {
            //
            // search for bus (recursive)
            //
            busDeviceNode = IopFindBusDeviceNodeInternal(DeviceNode, (InterfaceType == Eisa) ? Isa : InterfaceType, BusNumber);
        }

        if (busDeviceNode == NULL && DeviceNode == IopRootDeviceNode) {

            DebugMessage(DUMP_DETAIL, ("IopFindBusDeviceNode: Found %ws with interface=%08X & bus=%08X\n", DeviceNode->InstancePath.Buffer, InterfaceType, BusNumber));
            return DeviceNode;
        }

        if (busDeviceNode) {

            DebugMessage(DUMP_DETAIL, ("IopFindBusDeviceNode: Found %ws with interface=%08X & bus=%08X\n", busDeviceNode->InstancePath.Buffer, InterfaceType, BusNumber));
        }
    }

    return busDeviceNode;
}

PDEVICE_NODE
IopFindBusDeviceNodeInternal (
    IN PDEVICE_NODE DeviceNode,
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber
    )

/*++

Routine Description:

    This worker routine finds the bus PDO which performs translation and arbitration for the
    specified InterfaceType and BusNumber.

Parameters:

    DeviceNode - the root of the PDO to start our search

    InterfaceType - Specifies the PDO's interface type.

    BusNumber - specifies the PDO's bus number.

Return Value:

    A pointer to the BUS PDO.

--*/

{
    PDEVICE_NODE current;

    for (current = DeviceNode; current; current = current->Sibling) {

        if (    BusNumber == current->BusNumber &&
                (InterfaceType == current->InterfaceType ||
                    (InterfaceType == Isa && current->InterfaceType == Eisa))) {

            return current;
        }

        if (current->Child) {

            PDEVICE_NODE busDeviceNode = IopFindBusDeviceNodeInternal(current->Child, InterfaceType, BusNumber);

            if (busDeviceNode) {

                return busDeviceNode;
            }
        }
    }

    return NULL;
}

NTSTATUS
IopDuplicateDetection (
    IN INTERFACE_TYPE LegacyBusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    OUT PDEVICE_NODE *DeviceNode
    )

/*++

Routine Description:

    This routine searches for the bus device driver for a given legacy device,
    sends a query interface IRP for legacy device detection, and if the driver
    implements this interface, requests the PDO for the given legacy device.

Parameters:

    LegacyBusType - The legacy device's interface type.

    BusNumber - The legacy device's bus number.

    SlotNumber - The legacy device's slot number.

    DeviceNode - specifies a pointer to a variable to receive the duplicated device node

Return Value:

    NTSTATUS code.

--*/

{
    PDEVICE_NODE deviceNode;
    PDEVICE_OBJECT busDeviceObject;
    PLEGACY_DEVICE_DETECTION_INTERFACE interface;
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;

    //
    // Initialize return parameter to "not found".
    //

    *DeviceNode = NULL;

    //
    // Search the device tree for the bus of the legacy device.
    //

    deviceNode = IopFindBusDeviceNode(
                     IopRootDeviceNode,
                     LegacyBusType,
                     BusNumber,
                     SlotNumber
                 );

    //
    // Either a bus driver does not exist (or more likely, the legacy bus
    // type and bus number were unspecified).  Either way, we can't make
    // any further progress.
    //

    if (deviceNode == NULL) {

        return STATUS_INVALID_DEVICE_REQUEST;

    }

    //
    // We found the legacy device's bus driver.  Query it to determine
    // whether it implements the LEGACY_DEVICE_DETECTION interface.
    //

    busDeviceObject = deviceNode->PhysicalDeviceObject;

    status = IopQueryResourceHandlerInterface(
                 ResourceLegacyDeviceDetection,
                 busDeviceObject,
                 0,
                 (PINTERFACE *)&interface
             );

    //
    // If it doesn't, we're stuck.
    //

    if (!NT_SUCCESS(status) || interface == NULL) {

        return STATUS_INVALID_DEVICE_REQUEST;

    }

    //
    // Invoke the bus driver's legacy device detection method.
    //

    status = (*interface->LegacyDeviceDetection)(
                 interface->Context,
                 LegacyBusType,
                 BusNumber,
                 SlotNumber,
                 &deviceObject
             );

    //
    // If it found a legacy device, update the return parameter.
    //

    if (NT_SUCCESS(status) && deviceObject != NULL) {

        *DeviceNode =
            (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;

        status = STATUS_SUCCESS;

    } else {

        status = STATUS_INVALID_DEVICE_REQUEST;

    }

    //
    // Free the interface.
    //

    (*interface->InterfaceDereference)(interface->Context);

    ExFreePool(interface);

    return status;
}

VOID
IopSetLegacyDeviceInstance (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    This routine sets the Root\Legacy_xxxx\0000 device instance path to the
    madeup PDO (i.e. DeviceNode) which is created only for legacy resource allocation.
    This routine also links the madeup PDO to the Root\Legacy_xxxx\0000 device node
    to keep track what resources are assigned to the driver which services the
    root\legacy_xxxx\0000 device.

Parameters:

    P1 -

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    UNICODE_STRING instancePath, rootString;
    HANDLE handle;
    PDEVICE_NODE legacyDeviceNode;
    PDEVICE_OBJECT legacyPdo;

    PAGED_CODE();

    DeviceNode->OverUsed1.LegacyDeviceNode = 0;
    instancePath.Length = 0;
    instancePath.Buffer = NULL;

    status = IopServiceInstanceToDeviceInstance (
                 NULL,
                 &DriverObject->DriverExtension->ServiceKeyName,
                 0,
                 &instancePath,
                 &handle,
                 KEY_READ
                 );
    if (NT_SUCCESS(status) && (instancePath.Length != 0)) {
        RtlInitUnicodeString(&rootString, L"ROOT\\LEGACY");
        if (RtlPrefixUnicodeString(&rootString, &instancePath, TRUE) == FALSE) {
            RtlFreeUnicodeString(&instancePath);
        } else {
            DeviceNode->InstancePath = instancePath;
            legacyPdo = IopDeviceObjectFromDeviceInstance (handle, NULL);
            if (legacyPdo) {
                legacyDeviceNode = (PDEVICE_NODE)legacyPdo->DeviceObjectExtension->DeviceNode;
                DeviceNode->OverUsed2.NextResourceDeviceNode =
                    legacyDeviceNode->OverUsed2.NextResourceDeviceNode;
                legacyDeviceNode->OverUsed2.NextResourceDeviceNode = DeviceNode;
                DeviceNode->OverUsed1.LegacyDeviceNode = legacyDeviceNode;
            }
        }
        ZwClose(handle);
    }
}

PCM_RESOURCE_LIST
IopCombineLegacyResources (
    IN PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    This routine sets the Root\Legacy_xxxx\0000 device instance path to the
    madeup PDO (i.e. DeviceNode) which is created only for legacy resource allocation.
    This routine also links the madeup PDO to the Root\Legacy_xxxx\0000 device node
    to keep track what resources are assigned to the driver which services the
    root\legacy_xxxx\0000 device.

Parameters:

    DeviceNode - The legacy device node whose resources need to be combined.

Return Value:

    Return the combined resource list.

--*/

{
    NTSTATUS status;
    PCM_RESOURCE_LIST combinedList = NULL;
    PDEVICE_NODE devNode = DeviceNode;
    ULONG size = 0;
    PUCHAR p;

    PAGED_CODE();

    if (DeviceNode) {

        //
        // First determine how much memory is needed for the new combined list.
        //

        while (devNode) {
            if (devNode->ResourceList) {
                size += IopDetermineResourceListSize(devNode->ResourceList);
            }
            devNode = (PDEVICE_NODE)devNode->OverUsed2.NextResourceDeviceNode;
        }
        if (size != 0) {
            combinedList = (PCM_RESOURCE_LIST) ExAllocatePoolCMRL(PagedPool, size);
            devNode = DeviceNode;
            if (combinedList) {
                combinedList->Count = 0;
                p = (PUCHAR)combinedList;
                p += sizeof(ULONG);  // Skip Count
                while (devNode) {
                    if (devNode->ResourceList) {
                        size = IopDetermineResourceListSize(devNode->ResourceList);
                        if (size != 0) {
                            size -= sizeof(ULONG);
                            RtlMoveMemory(
                                p,
                                devNode->ResourceList->List,
                                size
                                );
                            p += size;
                            combinedList->Count += devNode->ResourceList->Count;
                        }
                    }
                    devNode = (PDEVICE_NODE)devNode->OverUsed2.NextResourceDeviceNode;
                }
            }
        }
    }
    return combinedList;
}

PCM_RESOURCE_LIST
IopCreateCmResourceList(
    IN PCM_RESOURCE_LIST ResourceList,
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG   BusNumber,
    OUT PCM_RESOURCE_LIST  *RemainingList
    )
{
    PCM_RESOURCE_LIST               newList = NULL;
    ULONG                           i, j;
    ULONG                           totalSize, matchSize, remainingSize, listSize;
    PCM_FULL_RESOURCE_DESCRIPTOR    fullResourceDesc, newFullResourceDesc, remainingFullResourceDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptor;


    //
    // Determine the size of memory to be allocated for the matching resource list.
    //

    fullResourceDesc = &ResourceList->List[0];
    totalSize = FIELD_OFFSET(CM_RESOURCE_LIST, List);
    matchSize = 0;
    for (i = 0; i < ResourceList->Count; i++) {

        //
        // Add the size of this descriptor.
        //

        listSize = FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR,
                                PartialResourceList) +
                   FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST,
                                PartialDescriptors);

        partialDescriptor = &fullResourceDesc->PartialResourceList.PartialDescriptors[0];
        for (j = 0; j < fullResourceDesc->PartialResourceList.Count; j++) {

            ULONG descriptorSize = sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

            if (partialDescriptor->Type == CmResourceTypeDeviceSpecific) {

                descriptorSize += partialDescriptor->u.DeviceSpecificData.DataSize;
            }

            listSize += descriptorSize;
            partialDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)
                                    ((PUCHAR)partialDescriptor + descriptorSize);

        }

        if (    fullResourceDesc->InterfaceType == InterfaceType &&
                fullResourceDesc->BusNumber == BusNumber) {


            matchSize += listSize;
        }

        totalSize += listSize;
        fullResourceDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)
                                  ((PUCHAR)fullResourceDesc + listSize);
    }

    if (totalSize) {

        if (matchSize) {

            matchSize += FIELD_OFFSET(CM_RESOURCE_LIST, List);

        }
        if (matchSize == totalSize) {

            *RemainingList = NULL;
            newList = ResourceList;

        } else if (matchSize == 0) {

            *RemainingList = ResourceList;

        } else {

            //
            // Allocate memory for both lists.
            //

            newList = (PCM_RESOURCE_LIST)ExAllocatePoolIORRR(PagedPool, matchSize);

            if (newList) {

                *RemainingList = (PCM_RESOURCE_LIST)ExAllocatePoolIORRR(PagedPool, totalSize - matchSize + FIELD_OFFSET(CM_RESOURCE_LIST, List));

                if (*RemainingList) {

                    newList->Count = 0;
                    (*RemainingList)->Count = 0;
                    newFullResourceDesc = &newList->List[0];
                    remainingFullResourceDesc = &(*RemainingList)->List[0];
                    fullResourceDesc = &ResourceList->List[0];
                    for (i = 0; i < ResourceList->Count; i++) {

                        listSize = FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR,
                                                PartialResourceList) +
                                   FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST,
                                                PartialDescriptors);

                        partialDescriptor = &fullResourceDesc->PartialResourceList.PartialDescriptors[0];
                        for (j = 0; j < fullResourceDesc->PartialResourceList.Count; j++) {

                            ULONG descriptorSize = sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

                            if (partialDescriptor->Type == CmResourceTypeDeviceSpecific) {

                                descriptorSize += partialDescriptor->u.DeviceSpecificData.DataSize;
                            }

                            listSize += descriptorSize;
                            partialDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)
                                                    ((PUCHAR)partialDescriptor + descriptorSize);

                        }

                        if (    fullResourceDesc->InterfaceType == InterfaceType &&
                                fullResourceDesc->BusNumber == BusNumber) {

                            newList->Count++;
                            RtlMoveMemory(newFullResourceDesc, fullResourceDesc, listSize);
                            newFullResourceDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)
                                                          ((PUCHAR)newFullResourceDesc + listSize);
                        } else {

                            (*RemainingList)->Count++;
                            RtlMoveMemory(remainingFullResourceDesc, fullResourceDesc, listSize);
                            remainingFullResourceDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)
                                                          ((PUCHAR)remainingFullResourceDesc + listSize);
                        }

                        fullResourceDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)
                                                  ((PUCHAR)fullResourceDesc + listSize);
                    }

                } else {

                    ExFreePool(newList);
                    newList = NULL;

                }

            } else {

                *RemainingList = NULL;
            }
        }
    }
    return newList;
}

PCM_RESOURCE_LIST
IopCombineCmResourceList(
    IN PCM_RESOURCE_LIST ResourceListA,
    IN PCM_RESOURCE_LIST ResourceListB
    )
{
    PCM_RESOURCE_LIST newList = NULL;
    ULONG   sizeA, sizeB, size;

    if (ResourceListA == NULL) {

        return ResourceListB;

    }

    if (ResourceListB == NULL) {

        return ResourceListA;

    }

    sizeA = IopDetermineResourceListSize(ResourceListA);
    sizeB = IopDetermineResourceListSize(ResourceListB);

    if (sizeA && sizeB) {

        size = sizeA + sizeB - (sizeof(CM_RESOURCE_LIST) - sizeof(CM_FULL_RESOURCE_DESCRIPTOR));
        newList = (PCM_RESOURCE_LIST)ExAllocatePoolIORRR(PagedPool, size);
        if (newList) {

            RtlMoveMemory(newList, ResourceListA, sizeA);
            RtlMoveMemory(  (PUCHAR)newList + sizeA,
                            (PUCHAR)ResourceListB + (sizeof(CM_RESOURCE_LIST) - sizeof(CM_FULL_RESOURCE_DESCRIPTOR)),
                            sizeB - (sizeof(CM_RESOURCE_LIST) - sizeof(CM_FULL_RESOURCE_DESCRIPTOR)));
            newList->Count += ResourceListB->Count;
        }
    }

    return newList;
}

NTSTATUS
IopReserveLegacyBootResources(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG   BusNumber
    )

/*++

Routine Description:

    This routine is called to reserve legacy BOOT resources for the specified InterfaceType
    and BusNumber. This is done everytime a new bus with a legacy InterfaceType gets enumerated.

Arguments:

    InterfaceType - Legacy InterfaceType.

    BusNumber - Legacy BusNumber

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    NTSTATUS    status;
    PIOP_RESERVED_RESOURCES_RECORD  resourceRecord, prevRecord;
    PCM_RESOURCE_LIST   newList, remainingList;

    if (IopInitHalDeviceNode && IopInitHalResources) {

        remainingList = NULL;
        newList = IopCreateCmResourceList(IopInitHalResources, InterfaceType, BusNumber, &remainingList);
        if (newList) {

            if (remainingList == NULL) {

                //
                // Full match. Check for error.
                //

                ASSERT(newList == IopInitHalResources);

            } else {

                //
                // Partial match. Check for error.
                //

                ASSERT(IopInitHalResources != newList);
                ASSERT(IopInitHalResources != remainingList);

            }

            DebugMessage(DUMP_INFO, ("IopReserveLegacyBootResources: Allocating HAL BOOT config for interface %08X and bus %08X...\n", InterfaceType, BusNumber));
            if (PnpResDebugLevel & DUMP_INFO) {

                IopDumpCmResourceList(newList);

            }
            if (remainingList) {
                ExFreePool(IopInitHalResources);
            }
            IopInitHalResources = remainingList;
            remainingList = IopInitHalDeviceNode->BootResources;
            IopInitHalDeviceNode->Flags |= DNF_HAS_BOOT_CONFIG;
            status = IopAllocateBootResources(  ArbiterRequestHalReported,
                                                IopInitHalDeviceNode->PhysicalDeviceObject,
                                                newList);
            ASSERT(NT_SUCCESS(status));
            IopInitHalDeviceNode->BootResources = IopCombineCmResourceList(remainingList, newList);
            ASSERT(IopInitHalDeviceNode->BootResources);

            //
            // Free previous BOOT config if any.
            //

            if (remainingList) {

                ExFreePool(remainingList);

            }
        } else {

            //
            // No match. Check that there was no error.
            //

            ASSERT(remainingList && remainingList == IopInitHalResources);
        }
    }

    for (prevRecord = NULL, resourceRecord = IopInitReservedResourceList; resourceRecord;) {

        if (    resourceRecord->ReservedResources &&
                resourceRecord->ReservedResources->List[0].InterfaceType == InterfaceType &&
                resourceRecord->ReservedResources->List[0].BusNumber == BusNumber) {

            DebugMessage(DUMP_INFO, ("IopReserveLegacyBootResources: Allocating BOOT config...\n"));
            if (PnpResDebugLevel & DUMP_INFO) {

                IopDumpCmResourceList(resourceRecord->ReservedResources);

            }

            status = IopAllocateBootResources(  ArbiterRequestPnpEnumerated,
                                                resourceRecord->DeviceObject,
                                                resourceRecord->ReservedResources);
            if (!NT_SUCCESS(status)) {

                DbgPrint("IopReserveLegacyBootResources: IopAllocateBootResources failed with status = %08X\n", status);
                ASSERT(NT_SUCCESS(status));

            }

            if (resourceRecord->DeviceObject == NULL) {

                ExFreePool(resourceRecord->ReservedResources);

            }

            if (prevRecord) {

                prevRecord->Next = resourceRecord->Next;

            } else {

                IopInitReservedResourceList = resourceRecord->Next;

            }
            ExFreePool(resourceRecord);
            resourceRecord = (prevRecord)? prevRecord->Next : IopInitReservedResourceList;

        } else {

            prevRecord = resourceRecord;
            resourceRecord = resourceRecord->Next;

        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
IopAllocateBootResources (
    IN ARBITER_REQUEST_SOURCE ArbiterRequestSource,
    IN PDEVICE_OBJECT DeviceObject,
    IN PCM_RESOURCE_LIST BootResources
    )

/*++

Routine Description:

    This routine reports boot resources for the specified device to
    arbiters.

Arguments:

    DeviceObject - Supplies a pointer to the device object, could be NULL
        if DeviceObject is NULL, the BootResources are reserved and will not be given
            to a device unless there is no other choice. The caller must release the
            pool for BootResources.
        if DeviceObject is NON_NULL, BootResources are BOOT reserved (preallocated.)

    BootResources - Supplies a pointer to the Boot resources.

Return Value:

    None.

--*/
{
    NTSTATUS    status;

    PAGED_CODE();

    KeEnterCriticalRegion( );

    status = KeWaitForSingleObject( &IopRegistrySemaphore,
                                    DelayExecution,
                                    KernelMode,
                                    FALSE,
                                    NULL );

    if (NT_SUCCESS(status)) {

        status = IopReserveBootResourcesInternal(ArbiterRequestSource, DeviceObject, BootResources);
        KeReleaseSemaphore(&IopRegistrySemaphore, 0, 1, FALSE);

    } else {

        DebugMessage(DUMP_ERROR, ("IopReserveBootResources: Get RegustrySemaphore failed. Status %x\n", status));
    }

    KeLeaveCriticalRegion();

    return status;
}

NTSTATUS
IopReserveBootResourcesInternal (
    IN ARBITER_REQUEST_SOURCE ArbiterRequestSource,
    IN PDEVICE_OBJECT DeviceObject,
    IN PCM_RESOURCE_LIST BootResources
    )

/*++

Routine Description:

    This routine reports boot resources for the specified device to
    arbiters.

Arguments:

    DeviceObject - Supplies a pointer to the device object, could be NULL
        if DeviceObject is NULL, the BootResources are reserved and will not be given
            to a device unless there is no other choice. The caller must release the
            pool for BootResources.
        if DeviceObject is NON_NULL, BootResources are BOOT reserved (preallocated.)

    BootResources - Supplies a pointer to the Boot resources.

Return Value:

    None.

--*/
{
    PIO_RESOURCE_REQUIREMENTS_LIST ioResources;
    PREQ_LIST reqList;
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_NODE deviceNode;

    PAGED_CODE();

    if (DeviceObject) {
        deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    }
    ioResources = IopCmResourcesToIoResources(0, BootResources, LCPRI_BOOTCONFIG);
    if (ioResources) {
#if MYDBG
        if (deviceNode) {
            DbgPrint("\n===================================\n");
            DbgPrint("PreAllocate Resource List for %wZ :: \n", &deviceNode->InstancePath);
        } else {
            DbgPrint("Reserve Resource List :: ");
        }
        IopDumpResourceRequirementsList(ioResources);
        DbgPrint(" ++++++++++++++++++++++++++++++\n");
#endif

        status = IopResourceRequirementsListToReqList(
                        ArbiterRequestSource,
                        ioResources,
                        DeviceObject,
                        &reqList);

        if (NT_SUCCESS(status) && reqList) {
            IopReserve(reqList);
            if (DeviceObject) {
                deviceNode->Flags |= DNF_BOOT_CONFIG_RESERVED;
                if (deviceNode->BootResources == NULL) {
                    ULONG size;

                    //
                    // If we have not remember the root enumerated device's boot resources
                    // do it now.  We always need to pre-allocate boot config for root enumerated
                    // devices when their resources are released.
                    //

                    size = IopDetermineResourceListSize (BootResources);
                    deviceNode->BootResources = ExAllocatePoolIORL(PagedPool, size);
                    if (deviceNode->BootResources) {
                        RtlMoveMemory(deviceNode->BootResources, BootResources, size);
                    } else {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
            }
            IopFreeReqList(reqList);
        } else {
#if MYDBG
            ASSERT(0);                         // For now
#endif
        }
        ExFreePool(ioResources);
    }

    return status;
}

NTSTATUS
IopReserveBootResources (
    IN ARBITER_REQUEST_SOURCE ArbiterRequestSource,
    IN PDEVICE_OBJECT DeviceObject,
    IN PCM_RESOURCE_LIST BootResources
    )

/*++

Routine Description:

    This routine reports boot resources for the specified device to
    arbiters.  Since the real resource pre-alloc routine can only be called after
    all the arbiters and translators are present, all the calls to pre-alloc resources
    before the preallocation routine is available will be routed to this routine.

Arguments:

    DeviceObject - Supplies a pointer to the device object.  If present, caller wants to
        preallocate BOOT resources for a device object.  If NULL, caller wants to reserved
        resources for some unknown devices.

    BootResources - Supplies a pointer to the Boot resources.

Return Value:

    None.

--*/
{
    PIOP_RESERVED_RESOURCES_RECORD  resourceRecord;
    PDEVICE_NODE                    deviceNode;
    ULONG                           size;
    NTSTATUS                        status;

    status = STATUS_SUCCESS;;
    size = IopDetermineResourceListSize(BootResources);
    if (size != 0) {

        deviceNode = (PDEVICE_NODE)((DeviceObject)?  DeviceObject->DeviceObjectExtension->DeviceNode : NULL);

        //
        // Pre-allocate BOOT configs right away for non-madeup devices.
        //

        if (DeviceObject && !(deviceNode->Flags & DNF_MADEUP)) {

            return IopAllocateBootResources(ArbiterRequestSource, DeviceObject, BootResources);

        }


        if (DeviceObject) {

            ASSERT(deviceNode);
            deviceNode->BootResources = ExAllocatePoolIORL(PagedPool, size);
            if (deviceNode->BootResources) {

                RtlMoveMemory(deviceNode->BootResources, BootResources, size);

            } else {

                return STATUS_INSUFFICIENT_RESOURCES;

            }
        }

        resourceRecord = (PIOP_RESERVED_RESOURCES_RECORD) ExAllocatePoolIORRR(  PagedPool,
                                                                                sizeof(IOP_RESERVED_RESOURCES_RECORD));
        if (resourceRecord) {

            resourceRecord->ReservedResources = (DeviceObject)? deviceNode->BootResources : BootResources;
            resourceRecord->DeviceObject = DeviceObject;
            resourceRecord->Next = IopInitReservedResourceList;
            IopInitReservedResourceList = resourceRecord;

        } else {

            if (deviceNode && deviceNode->BootResources) {

                ExFreePool(deviceNode->BootResources);

            }

            return STATUS_INSUFFICIENT_RESOURCES;

        }
    }

    return status;
}

VOID
IopReleaseResources (
    IN PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    IopReleaseResources releases resources owned by the device and release
    the memory pool.  We also release the cached resource requirements list.
    If the device is a root enumerated device with BOOT config, we will preallocate
    boot config resources for this device.

    NOTE, this is a routine INTERNAL to this file.  NO one should call this function
    outside of this file.  Outside of this file, IopReleaseDeviceResources should be
    used.

Arguments:

    DeviceNode - Supplies a pointer to the device node.object.  If present, caller wants to

Return Value:

    None.

--*/
{

    //
    // Release the resources owned by the device
    //

    IopReleaseResourcesInternal(DeviceNode);
    DeviceNode->Flags &= ~DNF_RESOURCE_ASSIGNED;
    DeviceNode->Flags &= ~DNF_RESOURCE_REPORTED;

#if DBG_SCOPE

    if (DeviceNode->PreviousResourceList) {
        ExFreePool(DeviceNode->PreviousResourceList);
        DeviceNode->PreviousResourceList = NULL;
    }
    if (DeviceNode->PreviousResourceRequirements) {
        ExFreePool(DeviceNode->PreviousResourceRequirements);
        DeviceNode->PreviousResourceRequirements = NULL;
    }
#endif

    if (DeviceNode->ResourceList) {

#if DBG_SCOPE
        if (!NT_SUCCESS(DeviceNode->FailureStatus)) {
            DeviceNode->PreviousResourceList = DeviceNode->ResourceList;
        } else {
            ExFreePool(DeviceNode->ResourceList);
        }
#else
        ExFreePool(DeviceNode->ResourceList);
#endif

        DeviceNode->ResourceList = NULL;
    }
    if (DeviceNode->ResourceListTranslated) {
        ExFreePool(DeviceNode->ResourceListTranslated);
        DeviceNode->ResourceListTranslated = NULL;
    }

    //
    // If this device is a root enumerated device, preallocate its BOOT resources
    //

    if ((DeviceNode->Flags & (DNF_MADEUP | DNF_DEVICE_GONE)) == DNF_MADEUP) {
        if (DeviceNode->Flags & DNF_HAS_BOOT_CONFIG && DeviceNode->BootResources) {
            IopReserveBootResourcesInternal(ArbiterRequestPnpEnumerated,
                                            DeviceNode->PhysicalDeviceObject,
                                            DeviceNode->BootResources);
        }
    } else {
        DeviceNode->Flags &= ~(DNF_HAS_BOOT_CONFIG | DNF_BOOT_CONFIG_RESERVED);
        if (DeviceNode->BootResources) {
            ExFreePool(DeviceNode->BootResources);
            DeviceNode->BootResources = NULL;
        }
    }
}

VOID
IopReallocateResources (
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine performs the real work for IoInvalidateDeviceState - ResourceRequirementsChanged.

Arguments:

    DeviceObject - Supplies a pointer to the device object.

Return Value:

    None.

--*/
{
    PDEVICE_NODE deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    IOP_RESOURCE_REQUEST requestTable, *requestTablep;
    ULONG deviceCount, oldFlags;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Grab the IO registry semaphore to make sure no other device is
    // reporting it's resource usage while we are searching for conflicts.
    //

    KeEnterCriticalRegion();
    status = KeWaitForSingleObject( &IopRegistrySemaphore,
                                    DelayExecution,
                                    KernelMode,
                                    FALSE,
                                    NULL);
    if (NT_SUCCESS(status)) {

        //
        // Acquire the tree lock so we block remove for this device node.
        //

        ExAcquireResourceShared(&IopDeviceTreeLock, TRUE);

        //
        // Check the flags after acquiring the semaphore.
        //

        if (deviceNode->Flags & DNF_RESOURCE_REQUIREMENTS_CHANGED) {
            //
            // Save the flags which we may have to restore in case of failure.
            //

            oldFlags = deviceNode->Flags & DNF_HAS_RESOURCE;
            deviceNode->Flags &= ~(DNF_HAS_RESOURCE);

            if (deviceNode->Flags & DNF_NON_STOPPED_REBALANCE) {

                IopAcquireEnumerationLock(deviceNode);

                //
                // Set up parameters to call real routine
                //

                deviceNode->Flags |= DNF_ASSIGNING_RESOURCES;

                RtlZeroMemory(&requestTable, sizeof(IOP_RESOURCE_REQUEST));
                requestTable.PhysicalDevice = DeviceObject;
                requestTablep = &requestTable;
                requestTable.Flags |= IOP_ASSIGN_NO_REBALANCE + IOP_ASSIGN_KEEP_CURRENT_CONFIG;

                status = IopGetResourceRequirementsForAssignTable(  requestTablep,
                                                                    requestTablep + 1,
                                                                    &deviceCount);
                if (NT_SUCCESS(status) && deviceCount) {

                    //
                    // Release the current resources to the arbiters.
                    // Memory for ResourceList is not released.
                    //

                    if (deviceNode->ResourceList) {

                        IopReleaseResourcesInternal(deviceNode);
                    }

                    //
                    // Try to do the assignment.
                    //

                    status = IopAssignInner(deviceCount, requestTablep, FALSE);
                    if (NT_SUCCESS(status)) {

                        deviceNode->Flags &= ~(DNF_RESOURCE_REQUIREMENTS_CHANGED | DNF_NON_STOPPED_REBALANCE);

                        IopBuildCmResourceLists(requestTablep, requestTablep + 1);

                        //
                        // We need to release the pool space for ResourceList and ResourceListTranslated.
                        // Because the earlier IopReleaseResourcesInternal does not release the pool.
                        //

                        if (deviceNode->ResourceList) {

                            ExFreePool(deviceNode->ResourceList);

                        }
                        if (deviceNode->ResourceListTranslated) {

                            ExFreePool(deviceNode->ResourceListTranslated);

                        }

                        deviceNode->ResourceList = requestTablep->ResourceAssignment;
                        deviceNode->ResourceListTranslated = requestTablep->TranslatedResourceAssignment;
                        deviceNode->Flags |= DNF_RESOURCE_ASSIGNED;
                        deviceNode->Flags &= ~DNF_RESOURCE_REPORTED;

                        IopStartDevice(deviceNode->PhysicalDeviceObject);

                    } else {

                        NTSTATUS restoreResourcesStatus;

                        restoreResourcesStatus = IopRestoreResourcesInternal(deviceNode);
                        if (!NT_SUCCESS(restoreResourcesStatus)) {

                            ASSERT(NT_SUCCESS(restoreResourcesStatus));
                            IopRequestDeviceRemoval(DeviceObject, CM_PROB_NORMAL_CONFLICT);

                        }

                        //
                        // BUGBUG - once we failed, what will trigger us to try again?
                        //
                    }

                    IopFreeResourceRequirementsForAssignTable(requestTablep, requestTablep + 1);


                } else {

                    status = NT_SUCCESS(status)? STATUS_UNSUCCESSFUL : status;

                }

                deviceNode->Flags &= ~DNF_ASSIGNING_RESOURCES;
                IopReleaseEnumerationLock(deviceNode);

            } else {

                //
                // The device needs to be stopped to change resources.
                //

                status = IopRebalance(0, NULL);

            }

            //
            // Restore the flags in case of failure.
            //

            if (!NT_SUCCESS(status)) {

                deviceNode->Flags &= ~DNF_HAS_RESOURCE;
                deviceNode->Flags |= oldFlags;

            }

        } else {

            DebugMessage(DUMP_ERROR, ("PNPRES: Resource requirements not changed in IopReallocateResources, returning error!\n"));
        }

        ExReleaseResource(&IopDeviceTreeLock);
        KeReleaseSemaphore(&IopRegistrySemaphore, 0, 1, FALSE);

    } else {

        DebugMessage(DUMP_ERROR, ("PNPRES: IopReallocateResources failed to acquire Registry semaphore, status %08X\n", status));

    }

    KeLeaveCriticalRegion();
}

#if DBG_SCOPE

VOID
IopCheckDataStructures (
    IN PDEVICE_NODE DeviceNode
    )

{
    if (DeviceNode) {
        IopCheckDataStructuresWorker (DeviceNode);
        IopCheckDataStructures (DeviceNode->Sibling);
        IopCheckDataStructures (DeviceNode->Child);
    }
}
VOID
IopCheckDataStructuresWorker (
    IN PDEVICE_NODE Device
    )

/*++

Routine Description:

    This routine releases the assigned resources for device specified by DeviceNode.
    Note, this routine does not reset the resource related fields in DeviceNode structure.

Parameters:

    DeviceNode - specifies the device node whose resources are goint to be released.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PLIST_ENTRY listHead, listEntry;
    PPI_RESOURCE_ARBITER_ENTRY arbiterEntry;

    listHead = &Device->DeviceArbiterList;
    listEntry = listHead->Flink;
    while (listEntry != listHead) {
        arbiterEntry = CONTAINING_RECORD(listEntry, PI_RESOURCE_ARBITER_ENTRY, DeviceArbiterList);
        if (arbiterEntry->ArbiterInterface != NULL) {
            ASSERT(IsListEmpty(&arbiterEntry->ResourceList));
            ASSERT(IsListEmpty(&arbiterEntry->ActiveArbiterList));
            InitializeListHead(&arbiterEntry->ActiveArbiterList);
            InitializeListHead(&arbiterEntry->ResourceList);
        }
        listEntry = listEntry->Flink;
    }
}

#endif


NTSTATUS
IopQueryConflictList(
    PDEVICE_OBJECT        PhysicalDeviceObject,
    IN PCM_RESOURCE_LIST  ResourceList,
    IN ULONG              ResourceListSize,
    OUT PPLUGPLAY_CONTROL_CONFLICT_LIST ConflictList,
    IN ULONG              ConflictListSize,
    IN ULONG              Flags
    )
/*++

Routine Description:

    This routine performs the querying of device conflicts
    returning data in ConflictList

Arguments:

    PhysicalDeviceObject PDO of device to Query
    ResourceList      CM resource list containing single resource to query
    ResourceListSize  Size of ResourceList
    ConflictList      Conflict list to fill query details in
    ConflictListSize  Size of buffer that we can fill with Conflict information
    Flags             Currently unused (zero) for future passing of flags

Return Value:

    Should be success in most cases

--*/
{
    NTSTATUS status;

    PAGED_CODE();

    KeEnterCriticalRegion( );

    status = KeWaitForSingleObject( &IopRegistrySemaphore,
                                    DelayExecution,
                                    KernelMode,
                                    FALSE,
                                    NULL );

    if (!NT_SUCCESS( status )) {
        DebugMessage(DUMP_ERROR, ("IopQueryConflictList: Get RegustrySemaphore failed. Status %x\n", status));
        KeLeaveCriticalRegion( );
        return status;
    } else {
        status = IopQueryConflictListInternal(PhysicalDeviceObject, ResourceList, ResourceListSize, ConflictList, ConflictListSize, Flags);
        KeReleaseSemaphore( &IopRegistrySemaphore, 0, 1, FALSE );
        KeLeaveCriticalRegion( );
    }

    return status;
}



BOOLEAN
IopEliminateBogusConflict(
    IN PDEVICE_OBJECT   PhysicalDeviceObject,
    IN PDEVICE_OBJECT   ConflictDeviceObject
    )
/*++

Routine Description:

    Determine if we're really conflicting with ourselves
    if this is the case, we ignore it

Arguments:

    PhysicalDeviceObject  PDO we're performing the test for
    ConflictDeviceObject  The object we've determined is conflicting

Return Value:

    TRUE to eliminate the conflict

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_NODE deviceNode;
    PDRIVER_OBJECT driverObject;
    KIRQL           irql;
    PDEVICE_OBJECT  attachedDevice;

    PAGED_CODE();

    //
    // simple cases
    //
    if (PhysicalDeviceObject == NULL || ConflictDeviceObject == NULL) {
        return FALSE;
    }
    //
    // if ConflictDeviceObject is on PDO's stack, this is a non-conflict
    // nb at least PDO has to be checked
    //
    ExAcquireFastLock( &IopDatabaseLock, &irql );

    for (attachedDevice = PhysicalDeviceObject;
         attachedDevice;
         attachedDevice = attachedDevice->AttachedDevice) {

        if (attachedDevice == ConflictDeviceObject) {
            ExReleaseFastLock( &IopDatabaseLock, irql );
            return TRUE;
        }
    }

    ExReleaseFastLock( &IopDatabaseLock, irql );

    //
    // legacy case
    //
    deviceNode = PhysicalDeviceObject->DeviceObjectExtension->DeviceNode;
    ASSERT(deviceNode);
    if (deviceNode->Flags & DNF_LEGACY_DRIVER) {
        //
        // hmmm, let's see if our ConflictDeviceObject is resources associated with a legacy device
        //
        if (ConflictDeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE) {
            //
            // if not, we have a legacy conflicting with non-legacy, we're interested!
            //
            return FALSE;
        }
        //
        // FDO, report driver name
        //
        driverObject = ConflictDeviceObject->DriverObject;
        if(driverObject == NULL) {
            //
            // should not be NULL
            //
            ASSERT(driverObject);
            return FALSE;
        }
        //
        // compare deviceNode->Service with driverObject->Service
        //
        if (deviceNode->ServiceName.Length != 0 &&
            deviceNode->ServiceName.Length == driverObject->DriverExtension->ServiceKeyName.Length &&
            RtlCompareUnicodeString(&deviceNode->ServiceName,&driverObject->DriverExtension->ServiceKeyName,TRUE)==0) {
            //
            // the driver's service name is the same that this PDO is associated with
            // by ignoring it we could end up ignoring conflicts of simular types of legacy devices
            // but since these have to be hand-config'd anyhow, it's prob better than having false conflicts
            // (jamiehun)
            //
            return TRUE;
        }

    }
    return FALSE;
}


NTSTATUS
IopQueryConflictFillString(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PWSTR            Buffer,
    IN OUT PULONG       Length,
    IN OUT PULONG       Flags
    )
/*++

Routine Description:

    Obtain string or string-length for details of conflicting device

Arguments:

    DeviceObject        Device object we want Device-Instance-String or Service Name
    Buffer              Buffer to Fill, NULL if we just want length
    Length              Filled with length of Buffer, including terminated NULL (Words)
    Flags               Apropriate flags set describing what the string represents

Return Value:

    Should be success in most cases

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_NODE deviceNode;
    PDRIVER_OBJECT driverObject;
    PUNICODE_STRING infoString = NULL;
    ULONG MaxLength = 0;        // words
    ULONG ReqLength = 0;        // words
    ULONG flags = 0;

    PAGED_CODE();

    if (Length != NULL) {
        MaxLength = *Length;
    }

    if (Flags != NULL) {
        flags = *Flags;
    }

    if (DeviceObject == NULL) {
        //
        // unknown
        //
        goto final;

    }

    if ((DeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE) == 0 ) {
        //
        // FDO, report driver name
        //
        driverObject = DeviceObject->DriverObject;
        if(driverObject == NULL) {
            //
            // should not be NULL
            //
            ASSERT(driverObject);
            goto final;
        }
        infoString = & (driverObject->DriverName);
        flags |= PNP_CE_LEGACY_DRIVER;
        goto final;
    }

    //
    // we should in actual fact have a PDO
    //
    if (DeviceObject->DeviceObjectExtension == NULL) {
        //
        // should not be NULL
        //
        ASSERT(DeviceObject->DeviceObjectExtension);
        goto final;
    }

    deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    if (deviceNode == NULL) {
        //
        // should not be NULL
        //
        ASSERT(deviceNode);
        goto final;
    }

    if (deviceNode == IopRootDeviceNode) {
        //
        // owned by root device
        //
        flags |= PNP_CE_ROOT_OWNED;

    } else if (deviceNode -> Parent == NULL) {
        //
        // faked out PDO - must be legacy device
        //
        driverObject = (PDRIVER_OBJECT)(deviceNode->DuplicatePDO);
        if(driverObject == NULL) {
            //
            // should not be NULL
            //
            ASSERT(driverObject);
            goto final;
        }
        infoString = & (driverObject->DriverName);
        flags |= PNP_CE_LEGACY_DRIVER;
        goto final;
    }

    //
    // we should be happy with what we have
    //
    infoString = &deviceNode->InstancePath;

final:

    if (infoString != NULL) {
        //
        // we have a string to copy
        //
        if ((Buffer != NULL) && (MaxLength*sizeof(WCHAR) > infoString->Length)) {
            RtlCopyMemory(Buffer, infoString->Buffer, infoString->Length);
        }
        ReqLength += infoString->Length / sizeof(WCHAR);
    }

    if ((Buffer != NULL) && (MaxLength > ReqLength)) {
        Buffer[ReqLength] = 0;
    }

    ReqLength++;

    if (Length != NULL) {
        *Length = ReqLength;
    }
    if (Flags != NULL) {
        *Flags = flags;
    }

    return status;
}


NTSTATUS
IopQueryConflictFillConflicts(
    PDEVICE_OBJECT                  PhysicalDeviceObject,
    IN ULONG                        ConflictCount,
    IN PARBITER_CONFLICT_INFO       ConflictInfoList,
    OUT PPLUGPLAY_CONTROL_CONFLICT_LIST ConflictList,
    IN ULONG                        ConflictListSize,
    IN ULONG                        Flags
    )
/*++

Routine Description:

    Fill ConflictList with information on as many conflicts as possible

Arguments:

    PhysicalDeviceObject The PDO we're performing the test on
    ConflictCount       Number of Conflicts.
    ConflictInfoList    List of conflicting device info, can be NULL if ConflictCount is 0
    ConflictList        Structure to fill in with conflicts
    ConflictListSize    Size of Conflict List
    Flags               if non-zero, dummy conflict is created

Return Value:

    Should be success in most cases

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ConflictListIdealSize;
    ULONG ConflictListBaseSize;
    ULONG ConflictListCount;
    ULONG Index;
    ULONG ConflictIndex;
    ULONG EntrySize;
    ULONG ConflictStringsOffset;
    ULONG stringSize;
    ULONG stringTotalSize;
    ULONG DummyCount;
    PPLUGPLAY_CONTROL_CONFLICT_STRINGS ConfStrings;

    PAGED_CODE();

    //
    // determine how many conflicts we can
    //
    // for each conflict
    // translate to bus/resource/address in respect to conflicting device
    // add to conflict list
    //
    //

    //
    // preprocessing - given our ConflictInfoList and ConflictCount
    // remove any that appear to be bogus - ie, that are the same device that we are testing against
    // this stops mostly legacy issues
    //
    for(Index = 0;Index < ConflictCount; Index++) {
        if (IopEliminateBogusConflict(PhysicalDeviceObject,ConflictInfoList[Index].OwningObject)) {

            DebugMessage(DUMP_DETAIL, ("IopQueryConflictFillConflicts: eliminating \"identical\" PDO %08x conflicting with self (%08x)\n",
                                        ConflictInfoList[Index].OwningObject,PhysicalDeviceObject));
            //
            // move the last listed conflict into this space
            //
            if (Index+1 < ConflictCount) {
                RtlCopyMemory(&ConflictInfoList[Index],&ConflictInfoList[ConflictCount-1],sizeof(ARBITER_CONFLICT_INFO));
            }
            //
            // account for deleting this item
            //
            ConflictCount--;
            Index--;
        }
    }

    //
    // preprocessing - in our conflict list, we may have PDO's for legacy devices, and resource nodes for the same
    // or other duplicate entities (we only ever want to report a conflict once, even if there's multiple conflicting ranges)
    //

  RestartScan:

    for(Index = 0;Index < ConflictCount; Index++) {
        if (ConflictInfoList[Index].OwningObject != NULL) {

            ULONG Index2;

            for (Index2 = Index+1; Index2 < ConflictCount; Index2++) {
                if (IopEliminateBogusConflict(ConflictInfoList[Index].OwningObject,ConflictInfoList[Index2].OwningObject)) {
                    //
                    // Index2 is considered a dup of Index
                    //

                    DebugMessage(DUMP_DETAIL, ("IopQueryConflictFillConflicts: eliminating \"identical\" PDO %08x conflicting with PDO %08x\n",
                                                ConflictInfoList[Index2].OwningObject,ConflictInfoList[Index].OwningObject));
                    //
                    // move the last listed conflict into this space
                    //
                    if (Index2+1 < ConflictCount) {
                        RtlCopyMemory(&ConflictInfoList[Index2],&ConflictInfoList[ConflictCount-1],sizeof(ARBITER_CONFLICT_INFO));
                    }
                    //
                    // account for deleting this item
                    //
                    ConflictCount--;
                    Index2--;
                } else if (IopEliminateBogusConflict(ConflictInfoList[Index2].OwningObject,ConflictInfoList[Index].OwningObject)) {
                    //
                    // Index is considered a dup of Index2 (some legacy case)
                    //
                    DebugMessage(DUMP_DETAIL, ("IopQueryConflictFillConflicts: eliminating \"identical\" PDO %08x conflicting with PDO %08x\n",
                                                ConflictInfoList[Index2].OwningObject,ConflictInfoList[Index].OwningObject));
                    //
                    // move the one we want (Index2) into the space occupied by Index
                    //
                    RtlCopyMemory(&ConflictInfoList[Index],&ConflictInfoList[Index2],sizeof(ARBITER_CONFLICT_INFO));
                    //
                    // move the last listed conflict into the space we just created
                    //
                    if (Index2+1 < ConflictCount) {
                        RtlCopyMemory(&ConflictInfoList[Index2],&ConflictInfoList[ConflictCount-1],sizeof(ARBITER_CONFLICT_INFO));
                    }
                    //
                    // account for deleting this item
                    //
                    ConflictCount--;
                    //
                    // but as this is quirky, restart the scan
                    //
                    goto RestartScan;
                }
            }
        }
    }

    //
    // preprocessing - if we have any known reported conflicts, don't report back any unknown
    //

    for(Index = 0;Index < ConflictCount; Index++) {
        //
        // find first unknown
        //
        if (ConflictInfoList[Index].OwningObject == NULL) {
            //
            // eliminate all other unknowns
            //

            ULONG Index2;

            for (Index2 = Index+1; Index2 < ConflictCount; Index2++) {
                if (ConflictInfoList[Index2].OwningObject == NULL) {

                    DebugMessage(DUMP_DETAIL, ("IopQueryConflictFillConflicts: eliminating extra unknown\n"));
                    //
                    // move the last listed conflict into this space
                    //
                    if (Index2+1 < ConflictCount) {
                        RtlCopyMemory(&ConflictInfoList[Index2],&ConflictInfoList[ConflictCount-1],sizeof(ARBITER_CONFLICT_INFO));
                    }
                    //
                    // account for deleting this item
                    //
                    ConflictCount--;
                    Index2--;
                }
            }

            if(ConflictCount != 1) {

                DebugMessage(DUMP_DETAIL, ("IopQueryConflictFillConflicts: eliminating first unknown\n"));
                //
                // there were others, so ignore the unknown
                //
                if (Index+1 < ConflictCount) {
                    RtlCopyMemory(&ConflictInfoList[Index],&ConflictInfoList[ConflictCount-1],sizeof(ARBITER_CONFLICT_INFO));
                }
                ConflictCount --;
            }

            break;
        }
    }

    //
    // set number of actual and listed conflicts
    //

    ConflictListIdealSize = (sizeof(PLUGPLAY_CONTROL_CONFLICT_LIST) - sizeof(PLUGPLAY_CONTROL_CONFLICT_ENTRY)) + sizeof(PLUGPLAY_CONTROL_CONFLICT_STRINGS);
    ConflictListCount = 0;
    stringTotalSize = 0;
    DummyCount = 0;

    ASSERT(ConflictListSize >= ConflictListIdealSize); // we should have checked to see if buffer is at least this big

    DebugMessage(DUMP_DETAIL, ("IopQueryConflictFillConflicts: Detected %d conflicts\n", ConflictCount));

    //
    // estimate sizes
    //
    if (Flags) {
        //
        // flags entry required (ie resource not available for some specified reason)
        //
        stringSize = 1; // null-length string
        DummyCount ++;
        EntrySize = sizeof(PLUGPLAY_CONTROL_CONFLICT_ENTRY);
        EntrySize += sizeof(WCHAR) * stringSize;

        if((ConflictListIdealSize+EntrySize) <= ConflictListSize) {
            //
            // we can fit this one in
            //
            ConflictListCount++;
            stringTotalSize += stringSize;
        }
        ConflictListIdealSize += EntrySize;
    }
    //
    // report conflicts
    //
    for(Index = 0; Index < ConflictCount; Index ++) {

        stringSize = 0;
        IopQueryConflictFillString(ConflictInfoList[Index].OwningObject,NULL,&stringSize,NULL);

        //
        // account for entry
        //
        EntrySize = sizeof(PLUGPLAY_CONTROL_CONFLICT_ENTRY);
        EntrySize += sizeof(WCHAR) * stringSize;

        if((ConflictListIdealSize+EntrySize) <= ConflictListSize) {
            //
            // we can fit this one in
            //
            ConflictListCount++;
            stringTotalSize += stringSize;
        }
        ConflictListIdealSize += EntrySize;
    }

    ConflictList->ConflictsCounted = ConflictCount+DummyCount; // number of conflicts detected including any dummy conflict
    ConflictList->ConflictsListed = ConflictListCount;         // how many we could fit in
    ConflictList->RequiredBufferSize = ConflictListIdealSize;  // how much buffer space to supply on next call

    DebugMessage(DUMP_DETAIL, ("IopQueryConflictFillConflicts: Listing %d conflicts\n", ConflictListCount));
    DebugMessage(DUMP_DETAIL, ("IopQueryConflictFillConflicts: Need %08x bytes to list all conflicts\n", ConflictListIdealSize));

    ConfStrings = (PPLUGPLAY_CONTROL_CONFLICT_STRINGS)&(ConflictList->ConflictEntry[ConflictListCount]);
    ConfStrings->NullDeviceInstance = (ULONG)(-1);
    ConflictStringsOffset = 0;

    for(ConflictIndex = 0; ConflictIndex < DummyCount; ConflictIndex++) {
        //
        // flags entry required (ie resource not available for some specified reason)
        //
        if (Flags && ConflictIndex == 0) {
            ConflictList->ConflictEntry[ConflictIndex].DeviceInstance = ConflictStringsOffset;
            ConflictList->ConflictEntry[ConflictIndex].DeviceFlags = Flags;
            ConflictList->ConflictEntry[ConflictIndex].ResourceType = 0;
            ConflictList->ConflictEntry[ConflictIndex].ResourceStart = 0;
            ConflictList->ConflictEntry[ConflictIndex].ResourceEnd = 0;
            ConflictList->ConflictEntry[ConflictIndex].ResourceFlags = 0;

            ConfStrings->DeviceInstanceStrings[ConflictStringsOffset] = 0; // null string
            stringTotalSize --;
            ConflictStringsOffset ++;
            DebugMessage(DUMP_DETAIL, ("IopQueryConflictFillConflicts: Listing flags %08x\n", Flags));
        }
    }
    //
    // get/fill in details for all those we can fit into the buffer
    //
    for(Index = 0; ConflictIndex < ConflictListCount ; Index ++, ConflictIndex++) {

        ASSERT(Index < ConflictCount);
        //
        // assign conflict information
        //
        ConflictList->ConflictEntry[ConflictIndex].DeviceInstance = ConflictStringsOffset;
        ConflictList->ConflictEntry[ConflictIndex].DeviceFlags = 0;
        ConflictList->ConflictEntry[ConflictIndex].ResourceType = 0; // BUGBUG!!! (jamiehun) remember to do this (post NT5)!
        ConflictList->ConflictEntry[ConflictIndex].ResourceStart = (ULONGLONG)(1); // for now, return totally invalid range (1-0)
        ConflictList->ConflictEntry[ConflictIndex].ResourceEnd = 0;
        ConflictList->ConflictEntry[ConflictIndex].ResourceFlags = 0;

        //
        // fill string details
        //
        stringSize = stringTotalSize;
        IopQueryConflictFillString(ConflictInfoList[Index].OwningObject,
                                    &(ConfStrings->DeviceInstanceStrings[ConflictStringsOffset]),
                                    &stringSize,
                                    &(ConflictList->ConflictEntry[ConflictIndex].DeviceFlags));
        stringTotalSize -= stringSize;
        DebugMessage(DUMP_DETAIL, ("IopQueryConflictFillConflicts: Listing \"%S\"\n", &(ConfStrings->DeviceInstanceStrings[ConflictStringsOffset])));
        ConflictStringsOffset += stringSize;
    }

    //
    // another NULL at end of strings (this is accounted for in the PPLUGPLAY_CONTROL_CONFLICT_STRINGS structure)
    //
    ConfStrings->DeviceInstanceStrings[ConflictStringsOffset] = 0;

    //Clean0:
    ;
    return status;
}


NTSTATUS
IopQueryConflictListInternal(
    PDEVICE_OBJECT        PhysicalDeviceObject,
    IN PCM_RESOURCE_LIST  ResourceList,
    IN ULONG              ResourceListSize,
    OUT PPLUGPLAY_CONTROL_CONFLICT_LIST ConflictList,
    IN ULONG              ConflictListSize,
    IN ULONG              Flags
    )
/*++

Routine Description:

    Version of IopQueryConflictList without the locking

--*/
{

    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_NODE deviceNode = NULL;
    PIO_RESOURCE_REQUIREMENTS_LIST ioResources;
    PREQ_LIST reqList;
    PREQ_DESC reqDesc, reqDescTranslated;
    PLIST_ENTRY listHead;
    PPI_RESOURCE_ARBITER_ENTRY arbiterEntry;
    PREQ_ALTERNATIVE RA;
    PREQ_ALTERNATIVE *reqAlternative;
    ULONG ConflictCount = 0;
    PARBITER_CONFLICT_INFO ConflictInfoList = NULL;
    PIO_RESOURCE_DESCRIPTOR ConflictDesc = NULL;
    ULONG ReqDescCount = 0;
    PREQ_DESC *ReqDescTable = NULL;
    PIO_RESOURCE_REQUIREMENTS_LIST pIoReqList = NULL;
    PVOID ExtParams[4];

    PAGED_CODE();

    ASSERT(PhysicalDeviceObject);
    ASSERT(ResourceList);
    ASSERT(ResourceListSize);
    //
    // these parameters were generated by umpnpmgr
    // so should be correct - one resource, and one resource only
    //
    ASSERT(ResourceList->Count == 1);
    ASSERT(ResourceList->List[0].PartialResourceList.Count == 1);

    if (ConflictList == NULL || (ConflictListSize < (sizeof(PLUGPLAY_CONTROL_CONFLICT_LIST) - sizeof(PLUGPLAY_CONTROL_CONFLICT_ENTRY)) + sizeof(PLUGPLAY_CONTROL_CONFLICT_STRINGS))) {
        //
        // sanity check
        //
        status = STATUS_BUFFER_TOO_SMALL;
        goto Clean0;
    }
    //
    // whatever other error we return, ensure that ConflictList is interpretable
    //

    ConflictList->ConflictsCounted = 0;
    ConflictList->ConflictsListed = 0;
    ConflictList->RequiredBufferSize = (sizeof(PLUGPLAY_CONTROL_CONFLICT_LIST) - sizeof(PLUGPLAY_CONTROL_CONFLICT_ENTRY)) + sizeof(PLUGPLAY_CONTROL_CONFLICT_STRINGS);

    //
    // Retrieve the devnode from the PDO
    //
    deviceNode = (PDEVICE_NODE)PhysicalDeviceObject->DeviceObjectExtension->DeviceNode;
    if (!deviceNode) {
        status = STATUS_NO_SUCH_DEVICE;
        goto Clean0;
    }

    //
    // type-specific validation
    //
    switch(ResourceList->List[0].PartialResourceList.PartialDescriptors[0].Type) {
        case CmResourceTypePort:
        case CmResourceTypeMemory:
            if(ResourceList->List[0].PartialResourceList.PartialDescriptors[0].u.Generic.Length == 0) {
                //
                // zero-range resource can never conflict
                //
                status = STATUS_SUCCESS;
                goto Clean0;
            }
            break;
        case CmResourceTypeInterrupt:
        case CmResourceTypeDma:
            break;
        default:
            ASSERT(0);
            status = STATUS_INVALID_PARAMETER;
            goto Clean0;
    }

    //
    // apply bus details from node
    //
    if (deviceNode->ChildInterfaceType == InterfaceTypeUndefined) {
        //
        // we have to grovel around to find real Interface Type
        //
        pIoReqList = deviceNode->ResourceRequirements;
        if (pIoReqList != NULL && pIoReqList->InterfaceType != InterfaceTypeUndefined) {
            ResourceList->List[0].InterfaceType = pIoReqList->InterfaceType;
        } else {
            //
            // BUGBUG!!! (jamiehun)
            // we should never get here
            // if we do, I need to look at this more
            //
#if MYDBG
            ASSERT(0);
#endif
            ResourceList->List[0].InterfaceType = PnpDefaultInterfaceType;
        }

    } else {
        //
        // we trust the deviceNode to tell us Interface Type
        //
        ResourceList->List[0].InterfaceType = deviceNode->ChildInterfaceType;
    }
    //
    // HACKHACK!!! (jamiehun) some bus-types we are better off considered as default
    //
    switch(ResourceList->List[0].InterfaceType) {
        case InterfaceTypeUndefined:
        case PCMCIABus:
            ResourceList->List[0].InterfaceType = PnpDefaultInterfaceType;
    }
    if ((deviceNode->ChildBusNumber & 0x80000000) == 0x80000000) {
        //
        // we have to grovel around to find real Bus Number
        //
        pIoReqList = deviceNode->ResourceRequirements;
        if (pIoReqList != NULL && (pIoReqList->BusNumber & 0x80000000) != 0x80000000) {
            ResourceList->List[0].BusNumber = pIoReqList->BusNumber;
        } else {
            //
            // a resonable default, but assert is here so I remember to look at this more
            // BUGBUG!!! (jamiehun)
            //
#if MYDBG
            ASSERT(0);
#endif
            ResourceList->List[0].BusNumber = 0;
        }

    } else {
        //
        // we trust the deviceNode to tell us Bus Number
        //
        ResourceList->List[0].BusNumber = deviceNode->ChildBusNumber;
    }

    //
    // from our CM Resource List, obtain an IO Resource Requirements List
    //
    ioResources = IopCmResourcesToIoResources(0, ResourceList, LCPRI_FORCECONFIG);
    if (!ioResources) {
        status = STATUS_INVALID_PARAMETER;
        goto Clean0;
    }
    //
    // Convert ioResources to a Request list
    // and in the processess, determine any Arbiters/Translators to use
    //
    status = IopResourceRequirementsListToReqList(
                    ArbiterRequestUndefined,    // BUGBUG!!! (jamiehun) better alternative???
                    ioResources,
                    PhysicalDeviceObject,
                    &reqList);

    //
    // get arbitrator/translator for current device/bus
    //

    if (NT_SUCCESS(status) && reqList) {

        reqAlternative = reqList->ReqAlternativeTable;
        RA = *reqAlternative;
        reqList->SelectedAlternative = reqAlternative;

        ReqDescCount = RA->ReqDescCount;
        ReqDescTable = RA->ReqDescTable;

        //
        // we should have got only one descriptor, use only the first one
        //
        if (ReqDescCount>0) {

            //
            // get first descriptor & it's arbitor
            //

            reqDesc = *ReqDescTable;
            if (reqDesc->ArbitrationRequired) {
                reqDescTranslated = reqDesc->TranslatedReqDesc;  // Could be reqDesc itself

                arbiterEntry = reqDesc->u.Arbiter;
                ASSERT(arbiterEntry);
                //
                // the descriptor of interest - translated, first alternative in the table
                //
                ConflictDesc = reqDescTranslated->AlternativeTable.Alternatives;
                //
                // skip special descriptor
                // to get to the actual descriptor
                //
                if(ConflictDesc->Type == CmResourceTypeConfigData || ConflictDesc->Type == CmResourceTypeReserved)
                        ConflictDesc++;

                //
                // finally we can call the arbiter to get a conflict list (returning PDO's and Global Address Ranges)
                //
                ExtParams[0] = PhysicalDeviceObject;
                ExtParams[1] = ConflictDesc;
                ExtParams[2] = &ConflictCount;
                ExtParams[3] = &ConflictInfoList;
                status = IopCallArbiter(arbiterEntry, ArbiterActionQueryConflict , ExtParams, NULL , NULL);

                if (NT_SUCCESS(status)) {
                    //
                    // fill in user-memory buffer with conflict
                    //
                    status = IopQueryConflictFillConflicts(PhysicalDeviceObject,ConflictCount,ConflictInfoList,ConflictList,ConflictListSize,0);
                    if(ConflictInfoList != NULL) {
                        ExFreePool(ConflictInfoList);
                    }
                }
                else if(status == STATUS_RANGE_NOT_FOUND) {
                    //
                    // fill in with flag indicating bad range (this means range is not available)
                    // ConflictInfoList should not be allocated
                    //
                    status = IopQueryConflictFillConflicts(NULL,0,NULL,ConflictList,ConflictListSize,PNP_CE_TRANSLATE_FAILED);
                }

            } else {
#if MYDBG
                ASSERT(0);                         // For now
#endif
                status = STATUS_INVALID_PARAMETER;  // if we failed, it's prob because ResourceList was invalid
            }
        } else {
#if MYDBG
            ASSERT(0);                         // For now
#endif
            status = STATUS_INVALID_PARAMETER;  // if we failed, it's prob because ResourceList was invalid
        }

#if DBG_SCOPE
        if (PnpResDebugLevel & STOP_ERROR) {
            IopCheckDataStructures(IopRootDeviceNode);
        }
#endif

        IopFreeReqList(reqList);
    } else {
#if MYDBG
        ASSERT(0);                         // For now
#endif
        if(NT_SUCCESS(status)) {
            //
            // it was NULL because we had a zero resource count, must be invalid parameter
            //
            status = STATUS_INVALID_PARAMETER;
        }

    }
    ExFreePool(ioResources);

    Clean0:
    ;

    return status;
}
