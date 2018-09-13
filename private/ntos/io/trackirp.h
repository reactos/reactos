/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    trackirp.h

Abstract:

    The module associated with the header asserts Irps are handled correctly
    by drivers. No IRP-major specific testing is done; such code is done in
    flunkirp.*

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Revision History:


--*/

#ifndef _TRACKIRP_H_
#define _TRACKIRP_H_

#define ASSERTFLAG_TRACKIRPS           0x00000001
#define ASSERTFLAG_MONITOR_ALLOCS      0x00000002
#define ASSERTFLAG_POLICEIRPS          0x00000004
#define ASSERTFLAG_MONITORMAJORS       0x00000008
#define ASSERTFLAG_SURROGATE           0x00000010
#define ASSERTFLAG_SMASH_SRBS          0x00000020
#define ASSERTFLAG_CONSUME_ALWAYS      0x00000040
#define ASSERTFLAG_FORCEPENDING        0x00000080
#define ASSERTFLAG_COMPLETEATDPC       0x00000100
#define ASSERTFLAG_COMPLETEATPASSIVE   0x00000200
#define ASSERTFLAG_DEFERCOMPLETION     0x00000800
#define ASSERTFLAG_ROTATE_STATUS       0x00001000
//                                     ----------
#define ASSERTMASK_COMPLETESTYLE       0x00000F80
#define ASSERTFLAG_SEEDSTACK           0x00010000

//
// Disabling HACKHACKS_ENABLED will remove support for all hack code. The
// hack code allows the machine to fully boot in checked builds. Note that
// those hacks can be individually disabled by setting the IovpHackFlags
// variable at boot time.
//
#define HACKHACKS_ENABLED
#define HACKFLAG_FOR_MUP               0x00000001
#define HACKFLAG_FOR_SCSIPORT          0x00000002
#define HACKFLAG_FOR_ACPI              0x00000004
#define HACKFLAG_FOR_BOGUSIRPS         0x00000008

extern ULONG IovpHackFlags ;
extern ULONG IovpTrackingFlags ;

//
// Currently, ntddk.h uses up to 0x2000 for Irp->Flags
//
#define IRPFLAG_EXAMINE_MASK           0xC0000000
#define IRPFLAG_EXAMINE_NOT_TRACKED    0x80000000
#define IRPFLAG_EXAMINE_TRACKED        0x40000000
#define IRPFLAG_EXAMINE_UNMARKED       0x00000000

#define IRP_DIAG_HAS_SURROGATE         0x02000000
#define IRP_DIAG_IS_SURROGATE          0x01000000


#define TRACKFLAG_ACTIVE               0x00000001
#define TRACKFLAG_SURROGATE            0x00000002
#define TRACKFLAG_HAS_SURROGATE        0x00000004
#define TRACKFLAG_PROTECTEDIRP         0x00000008

#define TRACKFLAG_QUEUED_INTERNALLY    0x00000010
#define TRACKFLAG_BOGUS                0x00000020
#define TRACKFLAG_RELEASED             0x00000040
#define TRACKFLAG_SRB_MUNGED           0x00000080
#define TRACKFLAG_SWAPPED_BACK         0x00000100
#define TRACKFLAG_WATERMARKED          0x00100000
#define TRACKFLAG_IO_ALLOCATED         0x00200000
#define TRACKFLAG_UNWOUND_BADLY        0x00400000
#define TRACKFLAG_PASSED_FAILURE       0x01000000
#define TRACKFLAG_PASSED_AT_BAD_IRQL   0x02000000
#define TRACKFLAG_IN_TRANSIT           0x40000000
#define TRACKFLAG_REMOVED_FROM_TABLE   0x80000000

#define DOE_DESIGNATED_FDO             0x80000000
#define DOE_BOTTOM_OF_FDO_STACK        0x40000000
#define DOE_RAW_FDO                    0x20000000
#define DOE_EXAMINED                   0x10000000
#define DOE_TRACKED                    0x08000000

#define STACKFLAG_NO_HANDLER           0x80000000
#define STACKFLAG_REQUEST_COMPLETED    0x40000000
#define STACKFLAG_CHECK_FOR_REFERENCE  0x20000000
#define STACKFLAG_REACHED_PDO          0x10000000
#define STACKFLAG_FIRST_REQUEST        0x08000000

#define CALLFLAG_COMPLETED             0x80000000
#define CALLFLAG_IS_REMOVE_IRP         0x40000000
#define CALLFLAG_REMOVING_FDO_STACK_DO 0x20000000
#define CALLFLAG_OVERRIDE_STATUS       0x10000000
#define CALLFLAG_TOPMOST_IN_SLOT       0x08000000

#define ALLOCFLAG_PROTECTEDIRP         0x00000001

#define SESSIONFLAG_UNWOUND_INCONSISTANT    0x00000001

#define IRP_SYSTEM_RESTRICTED          0x00000001
#define IRP_BOGUS                      0x00000002

#define SL_NOTCOPIED                   0x10

#define IRP_ALLOCATION_MONITORED       0x80

#define STARTED_TOP_OF_STACK        1
#define FORWARDED_TO_NEXT_DO        2
#define SKIPPED_A_DO                3
#define STARTED_INSIDE_STACK        4
#define CHANGED_STACKS_AT_BOTTOM    5
#define CHANGED_STACKS_MID_STACK    6

#define IRP_ALLOC_COUNT             5

typedef enum {

    DEFERACTION_QUEUE_WORKITEM,
    DEFERACTION_QUEUE_PASSIVE_TIMER,
    DEFERACTION_QUEUE_DISPATCH_TIMER,
    DEFERACTION_NORMAL

} DEFER_ACTION;

struct _IOFCALLDRIVER_STACKDATA;
struct _IOV_STACK_LOCATION;
struct _IOV_REQUEST_PACKET;

typedef struct _IOFCALLDRIVER_STACKDATA *PIOFCALLDRIVER_STACKDATA;
typedef struct _IOV_STACK_LOCATION      *PIOV_STACK_LOCATION;
typedef struct _IOV_REQUEST_PACKET      *PIOV_REQUEST_PACKET;
typedef struct _IOV_SESSION_DATA        *PIOV_SESSION_DATA;

typedef struct _IOFCALLDRIVER_STACKDATA {

    PIOV_SESSION_DATA       IovSessionData;
    PIOV_STACK_LOCATION     IovStackLocation; // For internal consistency checks only...
    ULONG                   Flags;
    LIST_ENTRY              SharedLocationList;
    NTSTATUS                ExpectedStatus;
    NTSTATUS                NewStatus;
    PDEVICE_OBJECT          RemovePdo;

} IOFCALLDRIVER_STACKDATA;

typedef struct _IOV_STACK_LOCATION {

    BOOLEAN                 InUse;
    ULONG                   Flags;
    PIOV_STACK_LOCATION     RequestsFirstStackLocation;
    LIST_ENTRY              CallStackData;
    PIO_STACK_LOCATION      IrpSp;
    PVOID                   LastDispatch;
    LARGE_INTEGER           PerfDispatchStart;
    LARGE_INTEGER           PerfStackLocationStart;
    PDEVICE_OBJECT          ReferencingObject;
    LONG                    ReferencingCount;
    IO_STATUS_BLOCK         InitialStatusBlock;
    IO_STATUS_BLOCK         LastStatusBlock;
    PETHREAD                ThreadDispatchedTo;

} IOV_STACK_LOCATION;

typedef struct _IOV_SESSION_DATA {

   PIOV_REQUEST_PACKET     IovRequestPacket;
   LONG                    SessionRefCount;
   LIST_ENTRY              SessionLink;
   ULONG                   SessionFlags;
   ULONG                   AssertFlags;

   PETHREAD                OriginatorThread;
   PDEVICE_OBJECT          DeviceLastCalled; // Last device called
   ULONG                   ForwardMethod;
   PIRP                    BestVisibleIrp;
   IOV_STACK_LOCATION      StackData[ANYSIZE_ARRAY];

} IOV_SESSION_DATA;

typedef struct _IOV_REQUEST_PACKET {

    PIRP                    TrackedIrp;     // Tracked IRP (could be surrogate)
    KSPIN_LOCK              IrpLock;        // Spinlock on data structure
    KIRQL                   CallerIrql;     // IRQL taken at.
    LONG                    ReferenceCount; // # of reasons to keep this packet
    LONG                    PointerCount;   // # of reasons to track by irp addr
    ULONG                   Flags;
    LIST_ENTRY              HashLink;       // Link in hash table.
    LIST_ENTRY              SurrogateLink;  // Head is at HeadPacket
    LIST_ENTRY              SessionHead;    // List of all sessions.
    PIOV_REQUEST_PACKET     HeadPacket;     // First non-surrogate packet.
    CCHAR                   StackCount;     // StackCount of tracked IRP.

    ULONG                   AssertFlags;

    PIO_COMPLETION_ROUTINE  RealIrpCompletionRoutine;
    UCHAR                   RealIrpControl;
    PVOID                   RealIrpContext;
    PVOID                   AllocatorStack[IRP_ALLOC_COUNT];

    //
    // The following information is for the assertion routines to read.
    //
    UCHAR                   TopStackLocation;

    CCHAR                   PriorityBoost;  // Boost from IofCompleteRequest
    UCHAR                   LastLocation;   // Last location from IofCallDriver
    ULONG                   RefTrackingCount;
    PVOID                   RestoreHandle;

    PIOV_SESSION_DATA       pIovSessionData;

} IOV_REQUEST_PACKET;

typedef struct _DEFERRAL_CONTEXT {

    PIOV_REQUEST_PACKET     IovRequestPacket;
    PIO_COMPLETION_ROUTINE  OriginalCompletionRoutine;
    PVOID                   OriginalContext;
    PIRP                    OriginalIrp;
    CCHAR                   OriginalPriorityBoost;
    PDEVICE_OBJECT          DeviceObject;
    PIO_STACK_LOCATION      IrpSpNext;
    WORK_QUEUE_ITEM         WorkQueueItem;
    KDPC                    DpcItem;
    KTIMER                  DeferralTimer;
    DEFER_ACTION            DeferAction;

} DEFERRAL_CONTEXT, *PDEFERRAL_CONTEXT;

typedef struct _IOFCOMPLETEREQUEST_STACKDATA {

    PIOV_SESSION_DATA       IovSessionData;
    PIOV_REQUEST_PACKET     IovRequestPacket;
    BOOLEAN                 IsRemoveIrp ;
    LONG                    LocationsAdvanced ;
    ULONG                   RaisedCount ;
    KIRQL                   PreviousIrql ;
    PVOID                   CompletionRoutine ;

} IOFCOMPLETEREQUEST_STACKDATA, *PIOFCOMPLETEREQUEST_STACKDATA ;

#ifdef NO_SPECIAL_IRP

#define SPECIALIRP_MARK_NON_TRACKABLE(Irp)
#define SPECIALIRP_IOF_CALL_1(pIrp, DeviceObject, st1)
#define SPECIALIRP_IOF_CALL_2(Irp, DeviceObject, Routine, FinalStatus, st1)
#define SPECIALIRP_IOF_COMPLETE_1(Irp, PriorityBoost, CompletionPacket)
#define SPECIALIRP_IOF_COMPLETE_2(Irp, CompletionPacket)
#define SPECIALIRP_IOF_COMPLETE_3(Irp, Routine, CompletionPacket)
#define SPECIALIRP_IOF_COMPLETE_4(Irp, ReturnedStatus, CompletionPacket)
#define SPECIALIRP_IOF_COMPLETE_5(Irp, CompletionPacket)
#define SPECIALIRP_IOP_COMPLETE_REQUEST(Irp, StackPointer)
#define SPECIALIRP_IO_CANCEL_IRP(Irp, CancelHandled, ReturnValue) DbgBreakPoint() ;
#define SPECIALIRP_IO_FREE_IRP(Irp, FreeHandled)                  DbgBreakPoint() ;
#define SPECIALIRP_IO_ALLOCATE_IRP_1(StackSize, Quota, pIrp)      DbgBreakPoint() ;
#define SPECIALIRP_IO_ALLOCATE_IRP_2(Irp)
#define SPECIALIRP_IO_INITIALIZE_IRP(Irp, PacketSize, StackSize, InitHandled) \
                                                                  DbgBreakPoint() ;
#define SPECIALIRP_IO_ATTACH_DEVICE_TO_DEVICE_STACK(NewDevice, ExistingDevice)
#define SPECIALIRP_IO_DETACH_DEVICE(TargetDevice)
#define SPECIALIRP_IO_DELETE_DEVICE(TargetDevice)
#define SPECIALIRP_WATERMARK_IRP(Irp, Flags)

#define IOP_DIAG_THROW_CHAFF_AT_STARTED_PDO_STACK(DeviceObject)

#else // NO_SPECIAL_IRP

//
// These are in trackirp.c
//

BOOLEAN
FASTCALL
IovpInitIrpTracking(
    IN ULONG   Level,
    IN ULONG   Flags
    );

VOID
FASTCALL
IovpCallDriver1(
    IN OUT PIRP           *IrpPointer,
    IN     PDEVICE_OBJECT DeviceObject,
    IN OUT PIOFCALLDRIVER_STACKDATA IofCallDriverStackData
    );

VOID
FASTCALL
IovpCallDriver2(
    IN     PIRP               Irp,
    IN     PDEVICE_OBJECT     DeviceObject,
    IN     PVOID              Routine,
    IN OUT NTSTATUS           *FinalStatus,
    IN     PIOFCALLDRIVER_STACKDATA IofCallDriverStackData
    );

VOID
FASTCALL
IovpCompleteRequest1(
    IN     PIRP               Irp,
    IN     CCHAR              PriorityBoost,
    IN OUT PIOFCOMPLETEREQUEST_STACKDATA CompletionPacket
    );

VOID
FASTCALL
IovpCompleteRequest2(
    IN     PIRP               Irp,
    IN OUT PIOFCOMPLETEREQUEST_STACKDATA CompletionPacket
    );

VOID
FASTCALL
IovpCompleteRequest3(
    IN     PIRP               Irp,
    IN     PVOID              Routine,
    IN OUT PIOFCOMPLETEREQUEST_STACKDATA CompletionPacket
    );

VOID
FASTCALL
IovpCompleteRequest4(
    IN     PIRP               Irp,
    IN     NTSTATUS           ReturnedStatus,
    IN OUT PIOFCOMPLETEREQUEST_STACKDATA CompletionPacket
    );

VOID
FASTCALL
IovpCompleteRequest5(
    IN     PIRP               Irp,
    IN OUT PIOFCOMPLETEREQUEST_STACKDATA CompletionPacket
    );

VOID
FASTCALL
IovpCompleteRequestApc(
    IN     PIRP               Irp,
    IN     PVOID              BestStackOffset
    );

VOID
FASTCALL
IovpCancelIrp(
    IN     PIRP               Irp,
    IN OUT PBOOLEAN           CancelHandled,
    IN OUT PBOOLEAN           ReturnValue
    );

VOID
FASTCALL
IovpFreeIrp(
    IN     PIRP               Irp,
    IN OUT PBOOLEAN           FreeHandled
    );

VOID
FASTCALL
IovpAllocateIrp1(
    IN     CCHAR              StackSize,
    IN     BOOLEAN            ChargeQuota,
    IN OUT PIRP               *IrpPointer
    );

VOID
FASTCALL
IovpAllocateIrp2(
    IN     PIRP               Irp
    );

VOID
FASTCALL
IovpInitializeIrp(
    IN OUT PIRP               Irp,
    IN     USHORT             PacketSize,
    IN     CCHAR              StackSize,
    IN OUT PBOOLEAN           InitializeHandled
    );

VOID
IovpExamineIrpStackForwarding(
    IN OUT  PIOV_REQUEST_PACKET  IovPacket,
    IN      BOOLEAN              IsNewSession,
    IN      ULONG                ForwardMethod,
    IN      PDEVICE_OBJECT       DeviceObject,
    IN      PIRP                 Irp,
    IN OUT  PIO_STACK_LOCATION  *IoCurrentStackLocation,
    OUT     PIO_STACK_LOCATION  *IoLastStackLocation,
    OUT     ULONG               *StackLocationsAdvanced
    );

NTSTATUS
IovpSwapSurrogateIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
FASTCALL
IovpExamineDevObjForwarding(
    IN  PDEVICE_OBJECT DeviceBeingCalled,
    IN  PDEVICE_OBJECT DeviceLastCalled,
    OUT PULONG         ForwardingTechnique
    );

VOID
FASTCALL
IovpFinalizeIrpSettings(
    IN OUT PIOV_REQUEST_PACKET   IrpTrackingData,
    IN BOOLEAN                   SurrogateIrpSwapped
    );

PDEVICE_OBJECT
FASTCALL
IovpGetDeviceAttachedTo(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
IovpInternalCompletionTrap(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
IovpInternalDeferredCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

PDEVICE_OBJECT
FASTCALL
IovpGetLowestDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
FASTCALL
IovpAssertNonLegacyDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG StackFramesToSkip,
    IN PUCHAR FailureTxt
    );

BOOLEAN
FASTCALL
IovpIsInFdoStack(
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
FASTCALL
IovpDoAssertIrps(
    VOID
    );

VOID
FASTCALL
IovpSeedStack(
    VOID
    );

VOID
FASTCALL
IovpSeedOnePage(
    VOID
    );

VOID
FASTCALL
IovpSeedTwoPages(
    VOID
    );

VOID
FASTCALL
IovpSeedThreePages(
    VOID
    );

VOID
IovpInternalCompleteAfterWait(
    IN PVOID Context
    );

VOID
IovpInternalCompleteAtDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
IovpAttachDeviceToDeviceStack(
    IN PDEVICE_OBJECT NewDevice,
    IN PDEVICE_OBJECT ExistingDevice
    );

VOID
IovpDetachDevice(
    IN PDEVICE_OBJECT LowerDevice
    );

VOID
IovpDeleteDevice(
    IN PDEVICE_OBJECT Device
    );

BOOLEAN
IovpIsInterestingStack(
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
IovpIsInterestingDriver(
    IN PDRIVER_OBJECT DriverObject
    );

VOID
IovpReexamineAllStacks(
    VOID
    );

BOOLEAN
IovpEnumDevObjCallback(
    IN PVOID Object,
    IN PUNICODE_STRING ObjectName,
    IN ULONG HandleCount,
    IN ULONG PointerCount,
    IN PVOID Context
    );

BOOLEAN
IovpAdvanceStackDownwards(
    IN  PIOV_STACK_LOCATION   StackDataArray,
    IN  CCHAR                 CurrentLocation,
    IN  PIO_STACK_LOCATION    IrpSp,
    IN  PIO_STACK_LOCATION    IrpLastSp OPTIONAL,
    IN  ULONG                 LocationsAdvanced,
    IN  BOOLEAN               IsNewRequest,
    IN  BOOLEAN               MarkAsTaken,
    OUT PIOV_STACK_LOCATION   *StackLocationInfo
    );

#define SPECIALIRP_MARK_NON_TRACKABLE(Irp) { \
    (Irp)->Flags |= IRPFLAG_EXAMINE_NOT_TRACKED; \
}

#define SPECIALIRP_IOF_CALL_1(pIrp, DeviceObject, st1) \
{\
    if (IovpTrackingFlags && IovpDoAssertIrps()) { \
        IovpCallDriver1((pIrp), (DeviceObject), (st1));\
        IovpSeedStack() ; \
    } else { \
        (st1)->IovSessionData = NULL ; \
        SPECIALIRP_MARK_NON_TRACKABLE(*pIrp); \
    } \
}

#define SPECIALIRP_IOF_CALL_2(Irp, DeviceObject, Routine, FinalStatus, st1) \
{\
    if (IovpTrackingFlags && IovpDoAssertIrps()) { \
        IovpCallDriver2((Irp), (DeviceObject), (Routine), (FinalStatus), (st1));\
    } \
}

#define SPECIALIRP_IOF_COMPLETE_1(Irp, PriorityBoost, CompletionPacket) \
{\
    if (IovpTrackingFlags && IovpDoAssertIrps()) { \
        IovpCompleteRequest1((Irp), (PriorityBoost), (CompletionPacket));\
    } else { \
        (CompletionPacket)->IovSessionData  = NULL ; \
    } \
}

#define SPECIALIRP_IOF_COMPLETE_2(Irp, CompletionPacket) \
{\
    if (IovpTrackingFlags && IovpDoAssertIrps()) { \
        IovpCompleteRequest2((Irp), (CompletionPacket));\
    } \
}

#define SPECIALIRP_IOF_COMPLETE_3(Irp, Routine, CompletionPacket) \
{\
    if (IovpTrackingFlags && IovpDoAssertIrps()) { \
        IovpCompleteRequest3((Irp), (Routine), (CompletionPacket));\
    } \
}

#define SPECIALIRP_IOF_COMPLETE_4(Irp, ReturnedStatus, CompletionPacket) \
{\
    if (IovpTrackingFlags && IovpDoAssertIrps()) { \
        IovpCompleteRequest4((Irp), (ReturnedStatus), (CompletionPacket));\
    } \
}

#define SPECIALIRP_IOF_COMPLETE_5(Irp, CompletionPacket) \
{\
    if (IovpTrackingFlags && IovpDoAssertIrps()) { \
        IovpCompleteRequest5((Irp), (CompletionPacket));\
    } \
}

#define SPECIALIRP_IO_CANCEL_IRP(Irp, CancelHandled, ReturnValue) \
{\
    if (IovpTrackingFlags && IovpDoAssertIrps()) { \
        IovpCancelIrp((Irp), (CancelHandled), (ReturnValue));\
    } else { \
        *(CancelHandled) = FALSE ; \
    } \
}

#define IOP_DIAG_THROW_CHAFF_AT_STARTED_PDO_STACK(DeviceObject) \
{\
    if (IovpTrackingFlags && IovpDoAssertIrps()) { \
        IovpThrowChaffAtStartedPdoStack(DeviceObject);\
    }\
}

#define SPECIALIRP_IO_FREE_IRP(Irp, FreeHandled) \
{\
    if (IovpTrackingFlags && IovpDoAssertIrps()) { \
        IovpFreeIrp((Irp), (FreeHandled));\
    } else { \
        *(FreeHandled) = FALSE ; \
    } \
}

#define SPECIALIRP_IO_ALLOCATE_IRP_1(StackSize, Quota, IrpPointer) \
{\
    if (IovpTrackingFlags && IovpDoAssertIrps()) { \
        IovpAllocateIrp1((StackSize), (Quota), (IrpPointer));\
    } else { \
        *(IrpPointer) = NULL ; \
    } \
}

#define SPECIALIRP_IO_ALLOCATE_IRP_2(Irp) \
{\
    if (IovpTrackingFlags && IovpDoAssertIrps()) { \
        IovpAllocateIrp2(Irp);\
    }\
}

#define SPECIALIRP_IO_INITIALIZE_IRP(Irp, PacketSize, StackSize, InitHandled) \
{\
    if (IovpTrackingFlags && IovpDoAssertIrps()) { \
        IovpInitializeIrp((Irp), (PacketSize), (StackSize), (InitHandled));\
    } else { \
        *(InitHandled) = FALSE ; \
    }\
}

#define SPECIALIRP_IO_DELETE_DEVICE(Device) \
{\
    if (IovpTrackingFlags && IovpDoAssertIrps()) { \
        IovpDeleteDevice(Device);\
    }\
}

#define SPECIALIRP_IO_ATTACH_DEVICE_TO_DEVICE_STACK(NewDevice, ExistingDevice) \
{\
    if (IovpTrackingFlags && IovpDoAssertIrps()) { \
        IovpAttachDeviceToDeviceStack(NewDevice, ExistingDevice);\
    }\
}

#define SPECIALIRP_IO_DETACH_DEVICE(TargetDevice) \
{\
    if (IovpTrackingFlags && IovpDoAssertIrps()) { \
        IovpDetachDevice(TargetDevice);\
    }\
}

#define SPECIALIRP_WATERMARK_IRP(Irp, Flags) \
{\
    if (IovpTrackingFlags && IovpDoAssertIrps()) { \
        IovpWatermarkIrp(Irp, Flags);\
    }\
}

#define SPECIALIRP_IOP_COMPLETE_REQUEST(Irp, StackPointer) \
{\
    if (IovpTrackingFlags && IovpDoAssertIrps()) { \
        IovpCompleteRequestApc(Irp, StackPointer);\
    }\
}

#if DBG
#define TRACKIRP_DBGPRINT(txt,level) \
{ \
    if (IovpIrpTrackingSpewLevel>(level)) { \
        DbgPrint##txt ; \
    }\
}
#else
#define TRACKIRP_DBGPRINT(txt,level)
#endif

#endif // NO_SPECIAL_IRP

#endif // _TRACKIRP_H_

