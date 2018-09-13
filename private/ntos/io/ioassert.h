/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ioassert.h

Abstract:

    This module implements some useful assertion routines

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Revision History:


--*/

#ifndef _IOASSERT_H_
#define _IOASSERT_H_

#if defined(_WIN64)
#define NO_SPECIAL_IRP
#else
#undef NO_SPECIAL_IRP
#endif

#ifdef NO_SPECIAL_IRP

#define WDM_CHASTISE_CALLER3(x)
#define WDM_CHASTISE_CALLER5(x)
#define WDM_CHASTISE_ROUTINE(x)
#define WDM_FAIL_CALLER(x, depth)
#define WDM_FAIL_CALLER1(x)
#define WDM_FAIL_CALLER2(x)
#define WDM_FAIL_CALLER3(x)
#define WDM_FAIL_CALLER4(x)
#define WDM_FAIL_CALLER5(x)
#define WDM_FAIL_CALLER6(x)
#define WDM_FAIL_ROUTINE(x)
#define KDASSERT(x)
#define ASSERT_SPINLOCK_HELD(x)

#else // NO_SPECIAL_IRP

extern LONG         IovpInitCalled;
extern ULONG        IovpVerifierLevel;
extern ULONG        IovpEnforcementLevel;
extern ULONG        IopDcControlInitial;
extern ULONG        IopDcControlOverride;
extern KSPIN_LOCK   IopDcControlLock;

typedef enum _DCERROR_ID {

    DCERROR_UNSPECIFIED = 0x200,
    DCERROR_DELETE_WHILE_ATTACHED,
    DCERROR_DETACH_NOT_ATTACHED,
    DCERROR_CANCELROUTINE_FORWARDED,
    DCERROR_NULL_DEVOBJ_FORWARDED,
    DCERROR_QUEUED_IRP_FORWARDED,
    DCERROR_NEXTIRPSP_DIRTY,
    DCERROR_IRPSP_COPIED,
    DCERROR_INSUFFICIENT_STACK_LOCATIONS,
    DCERROR_QUEUED_IRP_COMPLETED,
    DCERROR_FREE_OF_INUSE_TRACKED_IRP,
    DCERROR_FREE_OF_INUSE_IRP,
    DCERROR_FREE_OF_THREADED_IRP,
    DCERROR_REINIT_OF_ALLOCATED_IRP_WITH_QUOTA,
    DCERROR_PNP_IRP_BAD_INITIAL_STATUS,
    DCERROR_POWER_IRP_BAD_INITIAL_STATUS,
    DCERROR_WMI_IRP_BAD_INITIAL_STATUS,
    DCERROR_SKIPPED_DEVICE_OBJECT,
    DCERROR_BOGUS_FUNC_TRASHED,
    DCERROR_BOGUS_STATUS_TRASHED,
    DCERROR_BOGUS_INFO_TRASHED,
    DCERROR_PNP_FAILURE_FORWARDED,
    DCERROR_PNP_IRP_STATUS_RESET,
    DCERROR_PNP_IRP_NEEDS_FDO_HANDLING,
    DCERROR_PNP_IRP_FDO_HANDS_OFF,
    DCERROR_POWER_FAILURE_FORWARDED,
    DCERROR_POWER_IRP_STATUS_RESET,
    DCERROR_INVALID_STATUS,
    DCERROR_UNNECCESSARY_COPY,
    DCERROR_SHOULDVE_DETACHED,
    DCERROR_SHOULDVE_DELETED,
    DCERROR_MISSING_DISPATCH_FUNCTION,
    DCERROR_WMI_IRP_NOT_FORWARDED,
    DCERROR_DELETED_PRESENT_PDO,
    DCERROR_BUS_FILTER_ERRONEOUSLY_DETACHED,
    DCERROR_BUS_FILTER_ERRONEOUSLY_DELETED,
    DCERROR_INCONSISTANT_STATUS,
    DCERROR_UNINITIALIZED_STATUS,
    DCERROR_IRP_RETURNED_WITHOUT_COMPLETION,
    DCERROR_COMPLETION_ROUTINE_PAGABLE,
    DCERROR_PENDING_BIT_NOT_MIGRATED,
    DCERROR_CANCELROUTINE_ON_FORWARDED_IRP,
    DCERROR_PNP_IRP_NEEDS_PDO_HANDLING,
    DCERROR_TARGET_RELATION_LIST_EMPTY,
    DCERROR_TARGET_RELATION_NEEDS_REF,
    DCERROR_BOGUS_PNP_IRP_COMPLETED,
    DCERROR_SUCCESSFUL_PNP_IRP_NOT_FORWARDED,
    DCERROR_UNTOUCHED_PNP_IRP_NOT_FORWARDED,
    DCERROR_BOGUS_POWER_IRP_COMPLETED,
    DCERROR_SUCCESSFUL_POWER_IRP_NOT_FORWARDED,
    DCERROR_UNTOUCHED_POWER_IRP_NOT_FORWARDED,
    DCERROR_PNP_QUERY_CAP_BAD_VERSION,
    DCERROR_PNP_QUERY_CAP_BAD_SIZE,
    DCERROR_PNP_QUERY_CAP_BAD_ADDRESS,
    DCERROR_PNP_QUERY_CAP_BAD_UI_NUM,
    DCERROR_RESTRICTED_IRP,
    DCERROR_REINIT_OF_ALLOCATED_IRP_WITHOUT_QUOTA,
    DCERROR_UNFORWARDED_IRP_COMPLETED,
    DCERROR_DISPATCH_CALLED_AT_BAD_IRQL,
    DCERROR_BOGUS_MINOR_STATUS_TRASHED,
    DCERROR_MAXIMUM

} DCERROR_ID ;

#define DIAG_INITIALIZED            0x00000001
#define DIAG_BEEP                   0x00000002
#define DIAG_ZAPPED                 0x00000004
#define DIAG_CLEARED                0x00000008
#define DIAG_FATAL_ERROR            0x00000010
#define DIAG_WDM_ERROR              0x00000020
#define DIAG_IGNORE_DRIVER_LIST     0x00000040

#define DCPARAM_IRP             0x00000001
#define DCPARAM_ROUTINE         0x00000008
#define DCPARAM_DEVOBJ          0x00000040
#define DCPARAM_STATUS          0x00000200

#define _DC_ASSERT_OUT(ParenWrappedParamList, Frames) \
{ \
    static ULONG Flags=0 ; \
    if (IovpInitCalled) { \
        IopDriverCorrectnessTakeLock(&Flags, Frames) ;\
        IopDriverCorrectnessCheckUnderLock##ParenWrappedParamList;\
        IopDriverCorrectnessReleaseLock() ;\
    } \
}

//
// These macro's allow printf style debug output. They are invoked
// as follows (the excess parathesis's are required):
//
// WDM_FAIL_CALLER3(("Foo=%x",FooValue), irp) ;
//
#define WDM_CHASTISE_CALLER3(x)     _DC_ASSERT_OUT(x, 3)
#define WDM_CHASTISE_CALLER5(x)     _DC_ASSERT_OUT(x, 5)
#define WDM_FAIL_CALLER(x, depth)   _DC_ASSERT_OUT(x, depth)
#define WDM_FAIL_CALLER1(x)         _DC_ASSERT_OUT(x, 1)
#define WDM_FAIL_CALLER2(x)         _DC_ASSERT_OUT(x, 2)
#define WDM_FAIL_CALLER3(x)         _DC_ASSERT_OUT(x, 3)
#define WDM_FAIL_CALLER4(x)         _DC_ASSERT_OUT(x, 4)
#define WDM_FAIL_CALLER5(x)         _DC_ASSERT_OUT(x, 5)
#define WDM_FAIL_CALLER6(x)         _DC_ASSERT_OUT(x, 6)
#define WDM_FAIL_ROUTINE(x)         _DC_ASSERT_OUT(x, -1)
#define WDM_CHASTISE_ROUTINE(x)     _DC_ASSERT_OUT(x, -1)

#define KDASSERT(x) { if (KdDebuggerEnabled) { ASSERT(x) ; } }

#define ASSERT_SPINLOCK_HELD(x)

//
//  From here to #else is for use of ioassert.c only
//

VOID IopDriverCorrectnessTakeLock(
    IN PULONG ControlNew,
    IN LONG   StackFramesToSkip
    );

NTSTATUS
IopDriverCorrectnessCheckUnderLock(
    IN DCERROR_ID    MessageIndex,
    IN ULONG         MessageParameterMask,
    ...
    );

VOID
IopDriverCorrectnessReleaseLock(
    VOID
    );

//
// This structure and the table using it define the types and ordering of
// IopDriverCorrectnessCheck (see this function for a more detailed explanation)
//
typedef struct _DCPARAM_TYPE_ENTRY {

    ULONG   DcParamMask;
    PSTR    DcParamName;

} DCPARAM_TYPE_ENTRY, *PDCPARAM_TYPE_ENTRY;

//
//  Internal structures used in the buildup of a correctness check.
//
typedef struct _DCERROR_CLASS {

    ULONG            ClassFlags;
    PSTR             MessageClassText;

} DCERROR_CLASS, *PDCERROR_CLASS;
typedef const PDCERROR_CLASS PCDCERROR_CLASS;

typedef struct _DC_CHECK_DATA {

    PULONG          Control;
    ULONG           AssertionControl;
    ULONG           TableIndex;
    DCERROR_ID      MessageID;
    PVOID           CulpritAddress;
    ULONG_PTR       OffsetIntoImage;
    UNICODE_STRING  DriverName;
    PDCERROR_CLASS  AssertionClass;
    PVOID           *DcParamArray;
    PSTR            ClassText;
    PSTR            AssertionText;
    BOOLEAN         InVerifierList;

} DC_CHECK_DATA, *PDC_CHECK_DATA;

NTSTATUS
IopDriverCorrectnessProcessMessageText(
   IN ULONG               MaxOutputBufferSize,
   OUT PSTR               OutputBuffer,
   IN OUT PDC_CHECK_DATA  DcCheckData
   );

VOID
IopDriverCorrectnessProcessParams(
    IN OUT PULONG          Control OPTIONAL,
    IN LONG                StackFramesToSkip,
    IN DCERROR_ID          MessageID,
    IN ULONG               MessageParameterMask,
    IN va_list *           MessageParameters,
    IN PVOID *             DcParamArray,
    OUT PDC_CHECK_DATA     DcCheckData
    );

BOOLEAN
IopDriverCorrectnessApplyControl(
    IN OUT PDC_CHECK_DATA  DcCheckData
    );

VOID
IopDriverCorrectnessThrowBugCheck(
    IN PDC_CHECK_DATA DcCheckData
    );

VOID
IopDriverCorrectnessPrintBuffer(
    IN PDC_CHECK_DATA DcCheckData
    );

VOID
IopDriverCorrectnessPrintParamData(
    IN PDC_CHECK_DATA DcCheckData
    );

VOID
IopDriverCorrectnessPrintIrp(
    IN PIRP IrpToFlag
    );

VOID
IopDriverCorrectnessPrintIrpStack(
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
IopDriverCorrectnessPrompt(
    IN  PDC_CHECK_DATA DcCheckData,
    OUT PBOOLEAN       ExitAssertion
    );

PVOID
IopDriverCorrectnessAddressToFileHeader(
    IN PVOID PcValue,
    OUT PLDR_DATA_TABLE_ENTRY *DataTableEntry
    );

BOOLEAN
IopIsMemoryRangeReadable(
    IN PVOID Location,
    IN size_t Length
    );

#endif // NO_SPECIAL_IRP

#endif // _IOASSERT_H_

