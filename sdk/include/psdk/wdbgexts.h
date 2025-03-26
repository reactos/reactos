#ifndef _WDBGEXTS_
#define _WDBGEXTS_

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    DBGKD_SIMULATION_NONE,
    DBGKD_SIMULATION_EXDI
};

#define KD_SECONDARY_VERSION_DEFAULT                    0
#define KD_SECONDARY_VERSION_AMD64_OBSOLETE_CONTEXT_1   0
#define KD_SECONDARY_VERSION_AMD64_OBSOLETE_CONTEXT_2   1
#define KD_SECONDARY_VERSION_AMD64_CONTEXT              2

#if defined(_AMD64_)
#define CURRENT_KD_SECONDARY_VERSION                    KD_SECONDARY_VERSION_AMD64_CONTEXT
#else
#define CURRENT_KD_SECONDARY_VERSION                    KD_SECONDARY_VERSION_DEFAULT
#endif

#define DBGKD_VERS_FLAG_MP                              0x0001
#define DBGKD_VERS_FLAG_DATA                            0x0002
#define DBGKD_VERS_FLAG_PTR64                           0x0004
#define DBGKD_VERS_FLAG_NOMM                            0x0008
#define DBGKD_VERS_FLAG_HSS                             0x0010
#define DBGKD_VERS_FLAG_PARTITIONS                      0x0020

#define KDBG_TAG                                        'GBDK'

typedef enum _DBGKD_MAJOR_TYPES
{
    DBGKD_MAJOR_NT,
    DBGKD_MAJOR_XBOX,
    DBGKD_MAJOR_BIG,
    DBGKD_MAJOR_EXDI,
    DBGKD_MAJOR_NTBD,
    DBGKD_MAJOR_EFI,
    DBGKD_MAJOR_TNT,
    DBGKD_MAJOR_SINGULARITY,
    DBGKD_MAJOR_HYPERVISOR,
    DBGKD_MAJOR_MIDORI,
    DBGKD_MAJOR_COUNT
} DBGKD_MAJOR_TYPES;

//
// The major type is in the high byte
//
#define DBGKD_MAJOR_TYPE(MajorVersion) \
    ((DBGKD_MAJOR_TYPES)((MajorVersion) >> 8))

typedef struct _DBGKD_GET_VERSION32
{
    USHORT MajorVersion;
    USHORT MinorVersion;
    USHORT ProtocolVersion;
    USHORT Flags;
    ULONG KernBase;
    ULONG PsLoadedModuleList;
    USHORT MachineType;
    USHORT ThCallbackStack;
    USHORT NextCallback;
    USHORT FramePointer;
    ULONG KiCallUserMode;
    ULONG KeUserCallbackDispatcher;
    ULONG BreakpointWithStatus;
    ULONG DebuggerDataList;
} DBGKD_GET_VERSION32, *PDBGKD_GET_VERSION32;

typedef struct _DBGKD_DEBUG_DATA_HEADER32
{
    LIST_ENTRY32 List;
    ULONG OwnerTag;
    ULONG Size;
} DBGKD_DEBUG_DATA_HEADER32, *PDBGKD_DEBUG_DATA_HEADER32;

typedef struct _KDDEBUGGER_DATA32
{
    DBGKD_DEBUG_DATA_HEADER32 Header;
    ULONG KernBase;
    ULONG BreakpointWithStatus;
    ULONG SavedContext;
    USHORT ThCallbackStack;
    USHORT NextCallback;
    USHORT FramePointer;
    USHORT PaeEnabled:1;
    ULONG KiCallUserMode;
    ULONG KeUserCallbackDispatcher;
    ULONG PsLoadedModuleList;
    ULONG PsActiveProcessHead;
    ULONG PspCidTable;
    ULONG ExpSystemResourcesList;
    ULONG ExpPagedPoolDescriptor;
    ULONG ExpNumberOfPagedPools;
    ULONG KeTimeIncrement;
    ULONG KeBugCheckCallbackListHead;
    ULONG KiBugcheckData;
    ULONG IopErrorLogListHead;
    ULONG ObpRootDirectoryObject;
    ULONG ObpTypeObjectType;
    ULONG MmSystemCacheStart;
    ULONG MmSystemCacheEnd;
    ULONG MmSystemCacheWs;
    ULONG MmPfnDatabase;
    ULONG MmSystemPtesStart;
    ULONG MmSystemPtesEnd;
    ULONG MmSubsectionBase;
    ULONG MmNumberOfPagingFiles;
    ULONG MmLowestPhysicalPage;
    ULONG MmHighestPhysicalPage;
    ULONG MmNumberOfPhysicalPages;
    ULONG MmMaximumNonPagedPoolInBytes;
    ULONG MmNonPagedSystemStart;
    ULONG MmNonPagedPoolStart;
    ULONG MmNonPagedPoolEnd;
    ULONG MmPagedPoolStart;
    ULONG MmPagedPoolEnd;
    ULONG MmPagedPoolInformation;
    ULONG MmPageSize;
    ULONG MmSizeOfPagedPoolInBytes;
    ULONG MmTotalCommitLimit;
    ULONG MmTotalCommittedPages;
    ULONG MmSharedCommit;
    ULONG MmDriverCommit;
    ULONG MmProcessCommit;
    ULONG MmPagedPoolCommit;
    ULONG MmExtendedCommit;
    ULONG MmZeroedPageListHead;
    ULONG MmFreePageListHead;
    ULONG MmStandbyPageListHead;
    ULONG MmModifiedPageListHead;
    ULONG MmModifiedNoWritePageListHead;
    ULONG MmAvailablePages;
    ULONG MmResidentAvailablePages;
    ULONG PoolTrackTable;
    ULONG NonPagedPoolDescriptor;
    ULONG MmHighestUserAddress;
    ULONG MmSystemRangeStart;
    ULONG MmUserProbeAddress;
    ULONG KdPrintCircularBuffer;
    ULONG KdPrintCircularBufferEnd;
    ULONG KdPrintWritePointer;
    ULONG KdPrintRolloverCount;
    ULONG MmLoadedUserImageList;
} KDDEBUGGER_DATA32, *PKDDEBUGGER_DATA32;

typedef struct _DBGKD_GET_VERSION64
{
    USHORT MajorVersion;
    USHORT MinorVersion;
    UCHAR ProtocolVersion;
    UCHAR KdSecondaryVersion;
    USHORT Flags;
    USHORT MachineType;
    UCHAR MaxPacketType;
    UCHAR MaxStateChange;
    UCHAR MaxManipulate;
    UCHAR Simulation;
    USHORT Unused[1];
    ULONG64 KernBase;
    ULONG64 PsLoadedModuleList;
    ULONG64 DebuggerDataList;
} DBGKD_GET_VERSION64, *PDBGKD_GET_VERSION64;

typedef struct _DBGKD_DEBUG_DATA_HEADER64
{
    LIST_ENTRY64 List;
    ULONG OwnerTag;
    ULONG Size;
} DBGKD_DEBUG_DATA_HEADER64, *PDBGKD_DEBUG_DATA_HEADER64;

/* Self-documenting type: stores a pointer as a 64-bit quantity */
#if !defined(_WIN64) && (defined(__GNUC__) || defined(__clang__))
/* Minimal hackery for GCC/Clang, see commit b9cd3f2d9 (r25845) and de81021ba */
typedef union _ULPTR64
{
    ULONG_PTR ptr;
    ULONG64 ptr64;
} ULPTR64;
#else
// #define ULPTR64 PVOID64
#define ULPTR64 ULONG64
#endif

typedef struct _KDDEBUGGER_DATA64
{
    DBGKD_DEBUG_DATA_HEADER64 Header;
    ULONG64 KernBase;
    ULPTR64 BreakpointWithStatus;
    ULONG64 SavedContext;
    USHORT ThCallbackStack;
    USHORT NextCallback;
    USHORT FramePointer;
    USHORT PaeEnabled:1;
    ULPTR64 KiCallUserMode;
    ULONG64 KeUserCallbackDispatcher;
    ULPTR64 PsLoadedModuleList;
    ULPTR64 PsActiveProcessHead;
    ULPTR64 PspCidTable;
    ULPTR64 ExpSystemResourcesList;
    ULPTR64 ExpPagedPoolDescriptor;
    ULPTR64 ExpNumberOfPagedPools;
    ULPTR64 KeTimeIncrement;
    ULPTR64 KeBugCheckCallbackListHead;
    ULPTR64 KiBugcheckData;
    ULPTR64 IopErrorLogListHead;
    ULPTR64 ObpRootDirectoryObject;
    ULPTR64 ObpTypeObjectType;
    ULPTR64 MmSystemCacheStart;
    ULPTR64 MmSystemCacheEnd;
    ULPTR64 MmSystemCacheWs;
    ULPTR64 MmPfnDatabase;
    ULPTR64 MmSystemPtesStart;
    ULPTR64 MmSystemPtesEnd;
    ULPTR64 MmSubsectionBase;
    ULPTR64 MmNumberOfPagingFiles;
    ULPTR64 MmLowestPhysicalPage;
    ULPTR64 MmHighestPhysicalPage;
    ULPTR64 MmNumberOfPhysicalPages;
    ULPTR64 MmMaximumNonPagedPoolInBytes;
    ULPTR64 MmNonPagedSystemStart;
    ULPTR64 MmNonPagedPoolStart;
    ULPTR64 MmNonPagedPoolEnd;
    ULPTR64 MmPagedPoolStart;
    ULPTR64 MmPagedPoolEnd;
    ULPTR64 MmPagedPoolInformation;
    ULONG64 MmPageSize;
    ULPTR64 MmSizeOfPagedPoolInBytes;
    ULPTR64 MmTotalCommitLimit;
    ULPTR64 MmTotalCommittedPages;
    ULPTR64 MmSharedCommit;
    ULPTR64 MmDriverCommit;
    ULPTR64 MmProcessCommit;
    ULPTR64 MmPagedPoolCommit;
    ULPTR64 MmExtendedCommit;
    ULPTR64 MmZeroedPageListHead;
    ULPTR64 MmFreePageListHead;
    ULPTR64 MmStandbyPageListHead;
    ULPTR64 MmModifiedPageListHead;
    ULPTR64 MmModifiedNoWritePageListHead;
    ULPTR64 MmAvailablePages;
    ULPTR64 MmResidentAvailablePages;
    ULPTR64 PoolTrackTable;
    ULPTR64 NonPagedPoolDescriptor;
    ULPTR64 MmHighestUserAddress;
    ULPTR64 MmSystemRangeStart;
    ULPTR64 MmUserProbeAddress;
    ULPTR64 KdPrintCircularBuffer;
    ULPTR64 KdPrintCircularBufferEnd;
    ULPTR64 KdPrintWritePointer;
    ULPTR64 KdPrintRolloverCount;
    ULPTR64 MmLoadedUserImageList;

#if (NTDDI_VERSION >= NTDDI_WINXP)
    ULPTR64 NtBuildLab;
    ULPTR64 KiNormalSystemCall;
#endif

/* NOTE: Documented as "NT 5.0 hotfix (QFE) addition" */
#if (NTDDI_VERSION >= NTDDI_WIN2KSP4)
    ULPTR64 KiProcessorBlock;
    ULPTR64 MmUnloadedDrivers;
    ULPTR64 MmLastUnloadedDriver;
    ULPTR64 MmTriageActionTaken;
    ULPTR64 MmSpecialPoolTag;
    ULPTR64 KernelVerifier;
    ULPTR64 MmVerifierData;
    ULPTR64 MmAllocatedNonPagedPool;
    ULPTR64 MmPeakCommitment;
    ULPTR64 MmTotalCommitLimitMaximum;
    ULPTR64 CmNtCSDVersion;
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
    ULPTR64 MmPhysicalMemoryBlock;
    ULPTR64 MmSessionBase;
    ULPTR64 MmSessionSize;
    ULPTR64 MmSystemParentTablePage;
#endif

#if (NTDDI_VERSION >= NTDDI_WS03)
    ULPTR64 MmVirtualTranslationBase;
    USHORT OffsetKThreadNextProcessor;
    USHORT OffsetKThreadTeb;
    USHORT OffsetKThreadKernelStack;
    USHORT OffsetKThreadInitialStack;
    USHORT OffsetKThreadApcProcess;
    USHORT OffsetKThreadState;
    USHORT OffsetKThreadBStore;
    USHORT OffsetKThreadBStoreLimit;
    USHORT SizeEProcess;
    USHORT OffsetEprocessPeb;
    USHORT OffsetEprocessParentCID;
    USHORT OffsetEprocessDirectoryTableBase;
    USHORT SizePrcb;
    USHORT OffsetPrcbDpcRoutine;
    USHORT OffsetPrcbCurrentThread;
    USHORT OffsetPrcbMhz;
    USHORT OffsetPrcbCpuType;
    USHORT OffsetPrcbVendorString;
    USHORT OffsetPrcbProcStateContext;
    USHORT OffsetPrcbNumber;
    USHORT SizeEThread;
    ULPTR64 KdPrintCircularBufferPtr;
    ULPTR64 KdPrintBufferSize;
    ULPTR64 KeLoaderBlock;
    USHORT SizePcr;
    USHORT OffsetPcrSelfPcr;
    USHORT OffsetPcrCurrentPrcb;
    USHORT OffsetPcrContainedPrcb;
    USHORT OffsetPcrInitialBStore;
    USHORT OffsetPcrBStoreLimit;
    USHORT OffsetPcrInitialStack;
    USHORT OffsetPcrStackLimit;
    USHORT OffsetPrcbPcrPage;
    USHORT OffsetPrcbProcStateSpecialReg;
    USHORT GdtR0Code;
    USHORT GdtR0Data;
    USHORT GdtR0Pcr;
    USHORT GdtR3Code;
    USHORT GdtR3Data;
    USHORT GdtR3Teb;
    USHORT GdtLdt;
    USHORT GdtTss;
    USHORT Gdt64R3CmCode;
    USHORT Gdt64R3CmTeb;
    ULPTR64 IopNumTriageDumpDataBlocks;
    ULPTR64 IopTriageDumpDataBlocks;
#endif

#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULPTR64 VfCrashDataBlock;
    ULPTR64 MmBadPagesDetected;
    ULPTR64 MmZeroedPageSingleBitErrorsDetected;
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
    ULPTR64 EtwpDebuggerData;
    USHORT OffsetPrcbContext;
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
    USHORT OffsetPrcbMaxBreakpoints;
    USHORT OffsetPrcbMaxWatchpoints;
    ULONG OffsetKThreadStackLimit;
    ULONG OffsetKThreadStackBase;
    ULONG OffsetKThreadQueueListEntry;
    ULONG OffsetEThreadIrpList;
    USHORT OffsetPrcbIdleThread;
    USHORT OffsetPrcbNormalDpcState;
    USHORT OffsetPrcbDpcStack;
    USHORT OffsetPrcbIsrStack;
    USHORT SizeKDPC_STACK_FRAME;
#endif

#if (NTDDI_VERSION >= NTDDI_WINBLUE) // NTDDI_WIN81
    USHORT OffsetKPriQueueThreadListHead;
    USHORT OffsetKThreadWaitReason;
#endif

#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
    USHORT Padding;
    ULPTR64 PteBase;
#endif

#if (NTDDI_VERSION >= NTDDI_WIN10_RS5)
    ULPTR64 RetpolineStubFunctionTable;
    ULONG RetpolineStubFunctionTableSize;
    ULONG RetpolineStubOffset;
    ULONG RetpolineStubSize;
#endif
} KDDEBUGGER_DATA64, *PKDDEBUGGER_DATA64;

#ifdef __cplusplus
}
#endif

#endif // _WDBGEXTS_
