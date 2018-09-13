/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kddata.c

Abstract:

    This module contains global data for the portable kernel debgger.

Author:

    Mark Lucovsky 1-Nov-1993

Revision History:

--*/

#include "kdp.h"
#include "ke.h"
#include "pool.h"
#include "stdio.h"


//
// Miscellaneous data from all over the kernel
//


extern ULONG KiBugCheckData[];

extern PHANDLE_TABLE PspCidTable;

extern LIST_ENTRY ExpSystemResourcesList;
extern PPOOL_DESCRIPTOR ExpPagedPoolDescriptor;
extern ULONG ExpNumberOfPagedPools;

extern ULONG KeTimeIncrement;
extern LIST_ENTRY KeBugCheckCallbackListHead;
extern ULONG KiBugcheckData[];

extern LIST_ENTRY IopErrorLogListHead;

extern POBJECT_DIRECTORY ObpRootDirectoryObject;
extern POBJECT_TYPE ObpTypeObjectType;

extern PVOID MmSystemCacheStart;
extern PVOID MmSystemCacheEnd;

extern PVOID MmPfnDatabase;
extern ULONG MmSystemPtesStart[];
extern ULONG MmSystemPtesEnd[];
extern ULONG MmSubsectionBase;
extern ULONG MmNumberOfPagingFiles;

extern ULONG MmLowestPhysicalPage;
extern ULONG MmHighestPhysicalPage;
extern PFN_COUNT MmNumberOfPhysicalPages;

extern ULONG MmMaximumNonPagedPoolInBytes;
extern PVOID MmNonPagedSystemStart;
extern PVOID MmNonPagedPoolStart;
extern PVOID MmNonPagedPoolEnd;

extern PVOID MmPagedPoolStart;
extern PVOID MmPagedPoolEnd;
extern ULONG MmPagedPoolInfo[];
extern ULONG MmSizeOfPagedPoolInBytes;

extern ULONG MmTotalCommitLimit;
extern ULONG MmTotalCommittedPages;
extern ULONG MmSharedCommit;
extern ULONG MmDriverCommit;
extern ULONG MmProcessCommit;
extern ULONG MmPagedPoolCommit;
extern ULONG MmExtendedCommit;

extern MMPFNLIST MmZeroedPageListHead;
extern MMPFNLIST MmFreePageListHead;
extern MMPFNLIST MmStandbyPageListHead;
extern MMPFNLIST MmModifiedPageListHead;
extern MMPFNLIST MmModifiedNoWritePageListHead;
extern ULONG MmAvailablePages;
extern LONG MmResidentAvailablePages;
extern LIST_ENTRY MmLoadedUserImageList;

extern PPOOL_TRACKER_TABLE PoolTrackTable;
extern POOL_DESCRIPTOR NonPagedPoolDescriptor;

extern PVOID MiUnloadedDrivers;
extern ULONG MiLastUnloadedDriver;
extern ULONG MiTriageActionTaken;
extern ULONG MmSpecialPoolTag;
extern LOGICAL KernelVerifier;
extern PVOID MmVerifierData;
extern PFN_NUMBER MmAllocatedNonPagedPool;
extern SIZE_T MmPeakCommitment;
extern SIZE_T MmTotalCommitLimitMaximum;

//
// This block of data needs to always be present because crashdumps
// need the information.  Otherwise, things like PAGE_SIZE are not available
// in crashdumps, and extensions like !pool fail.
//

KDDEBUGGER_DATA64 KdDebuggerDataBlock = {
    {0},                                    //  DBGKD_DEBUG_DATA_HEADER Header;
    (ULONG64)0,
    (ULONG64)RtlpBreakWithStatusInstruction,
    (ULONG64)0,
    (USHORT)FIELD_OFFSET(KTHREAD, CallbackStack),   //  USHORT  ThCallbackStack;
    (USHORT)FIELD_OFFSET(KCALLOUT_FRAME, CbStk),    //  USHORT  NextCallback;

    #if defined(_X86_)
    (USHORT)FIELD_OFFSET(KCALLOUT_FRAME, Ebp),
    #else
    (USHORT)0,                                      //  USHORT  FramePointer;
    #endif

    #if defined(_X86PAE_)
    (USHORT)1,
    #else
    (USHORT)0,                                      //  USHORT  PaeEnabled;
    #endif

    (ULONG64)KiCallUserMode,
    (ULONG64)0,

    (ULONG64)&PsLoadedModuleList,
    (ULONG64)&PsActiveProcessHead,
    (ULONG64)&PspCidTable,

    (ULONG64)&ExpSystemResourcesList,
    (ULONG64)&ExpPagedPoolDescriptor,
    (ULONG64)&ExpNumberOfPagedPools,

    (ULONG64)&KeTimeIncrement,
    (ULONG64)&KeBugCheckCallbackListHead,
    (ULONG64)KiBugCheckData,

    (ULONG64)&IopErrorLogListHead,

    (ULONG64)&ObpRootDirectoryObject,
    (ULONG64)&ObpTypeObjectType,

    (ULONG64)&MmSystemCacheStart,
    (ULONG64)&MmSystemCacheEnd,
    (ULONG64)&MmSystemCacheWs,

    (ULONG64)&MmPfnDatabase,
    (ULONG64)MmSystemPtesStart,
    (ULONG64)MmSystemPtesEnd,
    (ULONG64)&MmSubsectionBase,
    (ULONG64)&MmNumberOfPagingFiles,

    (ULONG64)&MmLowestPhysicalPage,
    (ULONG64)&MmHighestPhysicalPage,
    (ULONG64)&MmNumberOfPhysicalPages,

    (ULONG64)&MmMaximumNonPagedPoolInBytes,
    (ULONG64)&MmNonPagedSystemStart,
    (ULONG64)&MmNonPagedPoolStart,
    (ULONG64)&MmNonPagedPoolEnd,

    (ULONG64)&MmPagedPoolStart,
    (ULONG64)&MmPagedPoolEnd,
    (ULONG64)&MmPagedPoolInfo,
    (ULONG64)PAGE_SIZE,
    (ULONG64)&MmSizeOfPagedPoolInBytes,

    (ULONG64)&MmTotalCommitLimit,
    (ULONG64)&MmTotalCommittedPages,
    (ULONG64)&MmSharedCommit,
    (ULONG64)&MmDriverCommit,
    (ULONG64)&MmProcessCommit,
    (ULONG64)&MmPagedPoolCommit,
    (ULONG64)&MmExtendedCommit,

    (ULONG64)&MmZeroedPageListHead,
    (ULONG64)&MmFreePageListHead,
    (ULONG64)&MmStandbyPageListHead,
    (ULONG64)&MmModifiedPageListHead,
    (ULONG64)&MmModifiedNoWritePageListHead,
    (ULONG64)&MmAvailablePages,
    (ULONG64)&MmResidentAvailablePages,

    (ULONG64)&PoolTrackTable,
    (ULONG64)&NonPagedPoolDescriptor,

    (ULONG64)&MmHighestUserAddress,
    (ULONG64)&MmSystemRangeStart,
    (ULONG64)&MmUserProbeAddress,

    (ULONG64)KdPrintCircularBuffer,
    (ULONG64)KdPrintCircularBuffer+sizeof(KdPrintCircularBuffer),

    (ULONG64)&KdPrintWritePointer,
    (ULONG64)&KdPrintRolloverCount,
    (ULONG64)&MmLoadedUserImageList,

    (ULONG64)0,
    (ULONG64)0,
    (ULONG64)KiProcessorBlock,
    (ULONG64)&MiUnloadedDrivers,
    (ULONG64)&MiLastUnloadedDriver,
    (ULONG64)&MiTriageActionTaken,
    (ULONG64)&MmSpecialPoolTag,
    (ULONG64)&KernelVerifier,
    (ULONG64)&MmVerifierData,
    (ULONG64)&MmAllocatedNonPagedPool,
    (ULONG64)&MmPeakCommitment,
    (ULONG64)&MmTotalCommitLimitMaximum,
    (ULONG64)&CmNtCSDVersion

};


//
// All dta from here on will be paged out if the kernel debugger is
// not enabled.
//

#ifdef _X86_
#ifdef ALLOC_PRAGMA
#pragma data_seg("PAGEKD")
#endif
#endif // _X86_


UCHAR  KdPrintCircularBuffer[KDPRINTBUFFERSIZE] = {0};
PUCHAR KdPrintWritePointer = KdPrintCircularBuffer;
ULONG  KdPrintRolloverCount = 0;
KSPIN_LOCK KdpPrintSpinLock = 0;


BREAKPOINT_ENTRY KdpBreakpointTable[BREAKPOINT_TABLE_SIZE] = {0};
UCHAR KdpMessageBuffer[KDP_MESSAGE_BUFFER_SIZE] = {0};
UCHAR KdpPathBuffer[KDP_MESSAGE_BUFFER_SIZE] = {0};
DBGKD_INTERNAL_BREAKPOINT KdpInternalBPs[DBGKD_MAX_INTERNAL_BREAKPOINTS] = {0};

LARGE_INTEGER  KdPerformanceCounterRate = {0,0};
LARGE_INTEGER  KdTimerStart = {0,0} ;
LARGE_INTEGER  KdTimerStop = {0,0};
LARGE_INTEGER  KdTimerDifference = {0,0};

ULONG_PTR KdpCurrentSymbolStart = 0;
ULONG_PTR KdpCurrentSymbolEnd = 0;
LONG      KdpNextCallLevelChange = 0;   // used only over returns to the debugger.

ULONG_PTR KdSpecialCalls[DBGKD_MAX_SPECIAL_CALLS];
ULONG     KdNumberOfSpecialCalls = 0;
ULONG_PTR InitialSP = 0;
ULONG     KdpNumInternalBreakpoints = 0;
KTIMER    InternalBreakpointTimer = {0};
KDPC      InternalBreakpointCheckDpc = {0};

BOOLEAN   KdpPortLocked = FALSE;


DBGKD_TRACE_DATA TraceDataBuffer[TRACE_DATA_BUFFER_MAX_SIZE] = {0};
ULONG            TraceDataBufferPosition = 1; // Element # to write next
                                   // Recall elt 0 is a length

TRACE_DATA_SYM   TraceDataSyms[256] = {0};
UCHAR NextTraceDataSym = 0;     // what's the next one to be replaced
UCHAR NumTraceDataSyms = 0;     // how many are valid?

ULONG IntBPsSkipping = 0;       // number of exceptions that are being skipped
                                // now

BOOLEAN   WatchStepOver = FALSE;
PVOID     WSOThread = NULL;         // thread doing stepover
ULONG_PTR WSOEsp = 0;               // stack pointer of thread doing stepover (yes, we need it)
ULONG     WatchStepOverHandle = 0;
ULONG_PTR WatchStepOverBreakAddr = 0; // where the WatchStepOver break is set
BOOLEAN   WatchStepOverSuspended = FALSE;
ULONG     InstructionsTraced = 0;
BOOLEAN   SymbolRecorded = FALSE;
LONG      CallLevelChange = 0;
LONG_PTR  oldpc = 0;
BOOLEAN   InstrCountInternal = FALSE; // Processing a non-COUNTONLY?

BOOLEAN   BreakpointsSuspended = FALSE;

//
// KdpRetryCount controls the number of retries before we give up and
//   assume kernel debugger is not present.
// KdpNumberRetries is the number of retries left.  Initially, it is set
//   to 5 such that booting NT without debugger won't be delayed to long.
//

ULONG KdpRetryCount = 5;
ULONG KdpNumberRetries = 5;
ULONG KdpDefaultRetries = MAXIMUM_RETRIES ; // Retries are set to this after boot
BOOLEAN KdpControlCPending   = FALSE;
BOOLEAN KdpControlCPressed   = FALSE;

KDP_BREAKPOINT_TYPE KdpBreakpointInstruction = {0};
ULONG KdpNextPacketIdToSend = 0;
ULONG KdpPacketIdExpected = 0;
PVOID KdpNtosImageBase = NULL;

//
// KdDebugParameters contains the debug port address and baud rate
//     used to initialize kernel debugger port.
//
// (They both get initialized to zero to indicate using default settings.)
// If SYSTEM hive contains the parameters, i.e. port and baud rate, system
// init code will fill in these variable with the values stored in the hive.
//

DEBUG_PARAMETERS KdDebugParameters = {0, 0};

KSPIN_LOCK      KdpDataSpinLock = 0;
LIST_ENTRY      KdpDebuggerDataListHead = {NULL,NULL};

//
// !search support variables (page hit database)
//

PFN_NUMBER KdpSearchPageHits [SEARCH_PAGE_HIT_DATABASE_SIZE];
ULONG KdpSearchPageHitOffsets [SEARCH_PAGE_HIT_DATABASE_SIZE];
ULONG KdpSearchPageHitIndex;

LOGICAL KdpSearchInProgress = FALSE;

PFN_NUMBER KdpSearchStartPageFrame;
PFN_NUMBER KdpSearchEndPageFrame;

ULONG_PTR KdpSearchAddressRangeStart;
ULONG_PTR KdpSearchAddressRangeEnd;

ULONG KdpSearchCheckPoint = KDP_SEARCH_SYMBOL_CHECK;


#ifdef _X86_
#ifdef ALLOC_PRAGMA
#pragma data_seg()
#endif
#endif // _X86_

KSPIN_LOCK      KdpTimeSlipEventLock = 0;
PVOID           KdpTimeSlipEvent = NULL;
KDPC            KdpTimeSlipDpc = {0};
WORK_QUEUE_ITEM KdpTimeSlipWorkItem = {NULL};
KTIMER          KdpTimeSlipTimer = {0};
ULONG           KdpTimeSlipPending = 1;


BOOLEAN KdDebuggerNotPresent = FALSE;
BOOLEAN KdDebuggerEnabled = FALSE;
BOOLEAN KdPitchDebugger = TRUE;
BOOLEAN KdpDebuggerStructuresInitialized = FALSE ;
ULONG KdpOweBreakpoint;
ULONG KdEnteredDebugger = FALSE;
