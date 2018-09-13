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


#ifdef _X86_
#ifdef ALLOC_PRAGMA
#pragma data_seg("PAGEKD")
#endif
#endif // _X86_



UCHAR  KdPrintCircularBuffer[KDPRINTBUFFERSIZE] = {0};
PUCHAR KdPrintWritePointer = KdPrintCircularBuffer;
ULONG  KdPrintRolloverCount = 0;
KSPIN_LOCK KdpPrintSpinLock = 0;

KDDEBUGGER_DATA KdDebuggerDataBlock = {
    {0},                                    //  DBGKD_DEBUG_DATA_HEADER Header;
    0,                                      //  ULONG   KernBase;
    (ULONG_PTR)RtlpBreakWithStatusInstruction, //  ULONG   BreakpointWithStatus;
    0,                                      //  ULONG   SavedContext;
    0,                                      //  USHORT  ThCallbackStack;
    FIELD_OFFSET(KCALLOUT_FRAME, CbStk),    //  USHORT  NextCallback;
    #if defined(_X86_)
    FIELD_OFFSET(KCALLOUT_FRAME, Ebp),
    #else
    0,                                      //  USHORT  FramePointer;
    #endif

    #if defined(_X86PAE_)
    1,
    #else
    0,                                      //  USHORT  PaeEnabled;
    #endif
    (ULONG_PTR)KiCallUserMode,              //  ULONG   KiCallUserMode;
    0,                                      //  ULONG   KeUserCallbackDispatcher;

    (ULONG_PTR)&PsLoadedModuleList,         //  ULONG_PTR   PsLoadedModuleList;
    (ULONG_PTR)&PsActiveProcessHead,        //  ULONG_PTR   PsActiveProcessHead;
    (ULONG_PTR)&PspCidTable,                //  ULONG_PTR   PspCidTable;

    (ULONG_PTR)&ExpSystemResourcesList,     //  ULONG_PTR   ExpSystemResourcesList;
    (ULONG_PTR)&ExpPagedPoolDescriptor,     //  ULONG_PTR   ExpPagedPoolDescriptor;
    (ULONG_PTR)&ExpNumberOfPagedPools,      //  ULONG_PTR   ExpNumberOfPagedPools;

    (ULONG_PTR)&KeTimeIncrement,            //  ULONG_PTR   KeTimeIncrement;
    (ULONG_PTR)&KeBugCheckCallbackListHead, //  ULONG_PTR   KeBugCheckCallbackListHead;
    (ULONG_PTR)KiBugCheckData,              //  ULONG_PTR   KiBugcheckData;

    (ULONG_PTR)&IopErrorLogListHead,        //  ULONG_PTR   IopErrorLogListHead;

    (ULONG_PTR)&ObpRootDirectoryObject,     //  ULONG_PTR   ObpRootDirectoryObject;
    (ULONG_PTR)&ObpTypeObjectType,          //  ULONG_PTR   ObpTypeObjectType;

    (ULONG_PTR)&MmSystemCacheStart,         //  ULONG_PTR   MmSystemCacheStart;
    (ULONG_PTR)&MmSystemCacheEnd,           //  ULONG_PTR   MmSystemCacheEnd;
    (ULONG_PTR)&MmSystemCacheWs,            //  ULONG_PTR   MmSystemCacheWs;

    (ULONG_PTR)&MmPfnDatabase,              //  ULONG_PTR   MmPfnDatabase;
    (ULONG_PTR)MmSystemPtesStart,           //  ULONG_PTR   MmSystemPtesStart;
    (ULONG_PTR)MmSystemPtesEnd,             //  ULONG_PTR   MmSystemPtesEnd;
    (ULONG_PTR)&MmSubsectionBase,           //  ULONG_PTR   MmSubsectionBase;
    (ULONG_PTR)&MmNumberOfPagingFiles,      //  ULONG_PTR   MmNumberOfPagingFiles;

    (ULONG_PTR)&MmLowestPhysicalPage,       //  ULONG_PTR   MmLowestPhysicalPage;
    (ULONG_PTR)&MmHighestPhysicalPage,      //  ULONG_PTR   MmHighestPhysicalPage;
    (ULONG_PTR)&MmNumberOfPhysicalPages,    //  ULONG_PTR   MmNumberOfPhysicalPages;

    (ULONG_PTR)&MmMaximumNonPagedPoolInBytes, //  ULONG_PTR   MmMaximumNonPagedPoolInBytes;
    (ULONG_PTR)&MmNonPagedSystemStart,      //  ULONG_PTR   MmNonPagedSystemStart;
    (ULONG_PTR)&MmNonPagedPoolStart,        //  ULONG_PTR   MmNonPagedPoolStart;
    (ULONG_PTR)&MmNonPagedPoolEnd,          //  ULONG_PTR   MmNonPagedPoolEnd;

    (ULONG_PTR)&MmPagedPoolStart,           //  ULONG_PTR   MmPagedPoolStart;
    (ULONG_PTR)&MmPagedPoolEnd,             //  ULONG_PTR   MmPagedPoolEnd;
    (ULONG_PTR)&MmPagedPoolInfo,            //  ULONG_PTR   MmPagedPoolInformation;
    0,                                      //  ULONG      Unused2
    (ULONG_PTR)&MmSizeOfPagedPoolInBytes,   //  ULONG_PTR   MmSizeOfPagedPoolInBytes;

    (ULONG_PTR)&MmTotalCommitLimit,         //  ULONG_PTR   MmTotalCommitLimit;
    (ULONG_PTR)&MmTotalCommittedPages,      //  ULONG_PTR   MmTotalCommittedPages;
    (ULONG_PTR)&MmSharedCommit,             //  ULONG_PTR   MmSharedCommit;
    (ULONG_PTR)&MmDriverCommit,             //  ULONG_PTR   MmDriverCommit;
    (ULONG_PTR)&MmProcessCommit,            //  ULONG_PTR   MmProcessCommit;
    (ULONG_PTR)&MmPagedPoolCommit,          //  ULONG_PTR   MmPagedPoolCommit;
    (ULONG_PTR)&MmExtendedCommit,           //  ULONG_PTR   MmExtendedCommit;

    (ULONG_PTR)&MmZeroedPageListHead,       //  ULONG_PTR   MmZeroedPageListHead;
    (ULONG_PTR)&MmFreePageListHead,         //  ULONG_PTR   MmFreePageListHead;
    (ULONG_PTR)&MmStandbyPageListHead,      //  ULONG_PTR   MmStandbyPageListHead;
    (ULONG_PTR)&MmModifiedPageListHead,     //  ULONG_PTR   MmModifiedPageListHead;
    (ULONG_PTR)&MmModifiedNoWritePageListHead, //  ULONG_PTR   MmModifiedNoWritePageListHead;
    (ULONG_PTR)&MmAvailablePages,           //  ULONG_PTR   MmAvailablePages;
    (ULONG_PTR)&MmResidentAvailablePages,   //  ULONG_PTR   MmResidentAvailablePages;

    (ULONG_PTR)&PoolTrackTable,             //  ULONG_PTR   PoolTrackTable;
    (ULONG_PTR)&NonPagedPoolDescriptor,     //  ULONG_PTR   NonPagedPoolDescriptor;

    (ULONG_PTR)&MmHighestUserAddress,       //  ULONG_PTR   MmHighestUserAddress;
    (ULONG_PTR)&MmSystemRangeStart,         //  ULONG_PTR   MmSystemRangeStart;
    (ULONG_PTR)&MmUserProbeAddress,         //  ULONG_PTR   MmUserProbeAddress;

    (ULONG_PTR) KdPrintCircularBuffer,      //  ULONG_PTR   KdPrintCircularBuffer;
    (ULONG_PTR) KdPrintCircularBuffer+sizeof(KdPrintCircularBuffer),
                                            //  ULONG_PTR   KdPrintCircularBufferEnd;
    (ULONG_PTR) &KdPrintWritePointer,       //  ULONG_PTR   KdPrintWritePointer;
    (ULONG_PTR) &KdPrintRolloverCount,      //  ULONG_PTR   KdPrintRolloverCount;
    (ULONG_PTR) &MmLoadedUserImageList,     //  ULONG_PTR   MmLoadedUserImageList;
};


BREAKPOINT_ENTRY KdpBreakpointTable[BREAKPOINT_TABLE_SIZE] = {0};
UCHAR KdpMessageBuffer[KDP_MESSAGE_BUFFER_SIZE] = {0};
UCHAR KdpPathBuffer[KDP_MESSAGE_BUFFER_SIZE] = {0};
DBGKD_INTERNAL_BREAKPOINT KdpInternalBPs[DBGKD_MAX_INTERNAL_BREAKPOINTS] = {0};

LARGE_INTEGER  KdPerformanceCounterRate = {0,0};
LARGE_INTEGER  KdTimerStart = {0,0} ;
LARGE_INTEGER  KdTimerStop = {0,0};
LARGE_INTEGER  KdTimerDifference = {0,0};

ULONG_PTR   KdpCurrentSymbolStart = 0;
ULONG_PTR   KdpCurrentSymbolEnd = 0;
LONG    KdpNextCallLevelChange = 0;   // used only over returns to the debugger.

ULONG_PTR   KdSpecialCalls[DBGKD_MAX_SPECIAL_CALLS];
ULONG   KdNumberOfSpecialCalls = 0;
ULONG_PTR   InitialSP = 0;
ULONG   KdpNumInternalBreakpoints = 0;
KTIMER  InternalBreakpointTimer = {0};
KDPC    InternalBreakpointCheckDpc = {0};

BOOLEAN KdpPortLocked = FALSE;


DBGKD_TRACE_DATA TraceDataBuffer[TRACE_DATA_BUFFER_MAX_SIZE] = {0};
ULONG            TraceDataBufferPosition = 1; // Element # to write next
                                   // Recall elt 0 is a length

TRACE_DATA_SYM   TraceDataSyms[256] = {0};
UCHAR NextTraceDataSym = 0;     // what's the next one to be replaced
UCHAR NumTraceDataSyms = 0;     // how many are valid?

ULONG IntBPsSkipping = 0;       // number of exceptions that are being skipped
                                // now

BOOLEAN WatchStepOver = FALSE;
PVOID WSOThread = NULL;         // thread doing stepover
ULONG WSOEsp = 0;               // stack pointer of thread doing stepover (yes, we need it)
ULONG WatchStepOverHandle = 0;
ULONG_PTR WatchStepOverBreakAddr = 0; // where the WatchStepOver break is set
BOOLEAN WatchStepOverSuspended = FALSE;
ULONG InstructionsTraced = 0;
BOOLEAN SymbolRecorded = FALSE;
LONG CallLevelChange = 0;
LONG oldpc = 0;
BOOLEAN InstrCountInternal = FALSE; // Processing a non-COUNTONLY?

BOOLEAN BreakpointsSuspended = FALSE;

//
// KdpRetryCount controls the number of retries before we give up and
//   assume kernel debugger is not present.
// KdpNumberRetries is the number of retries left.  Initially, it is set
//   to 5 such that booting NT without debugger won't be delayed to long.
//

ULONG KdpRetryCount = 5;
ULONG KdpNumberRetries = 5;

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
