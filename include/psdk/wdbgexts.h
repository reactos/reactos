#ifndef _WDBGEXTS_
#define _WDBGEXTS_

enum
{
    DBGKD_SIMULATION_NONE,
    DBGKD_SIMULATION_EXDI
};

#define KD_SECONDARY_VERSION_DEFAULT                    0
#define KD_SECONDARY_VERSION_AMD64_OBSOLETE_CONTEXT_1   0
#define KD_SECONDARY_VERSION_AMD64_OBSOLETE_CONTEXT_2   1
#define KD_SECONDARY_VERSION_AMD64_CONTEXT              2
#define CURRENT_KD_SECONDARY_VERSION                    KD_SECONDARY_VERSION_DEFAULT

#define DBGKD_VERS_FLAG_MP                              0x0001
#define DBGKD_VERS_FLAG_DATA                            0x0002
#define DBGKD_VERS_FLAG_PTR64                           0x0004
#define DBGKD_VERS_FLAG_NOMM                            0x0008
#define DBGKD_VERS_FLAG_HSS                             0x0010
#define DBGKD_VERS_FLAG_PARTITIONS                      0x0020

#define KDBG_TAG                                        TAG('K', 'D', 'B', 'G')

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

typedef union _GCC_ULONG64
{
    ULONG_PTR Pointer;
    ULONG64 RealPointer;
} GCC_ULONG64, *PGCC_ULONG64;

typedef struct _KDDEBUGGER_DATA64
{
    DBGKD_DEBUG_DATA_HEADER64 Header;
    ULONG64 KernBase;
    GCC_ULONG64 BreakpointWithStatus;
    ULONG64 SavedContext;
    USHORT ThCallbackStack;
    USHORT NextCallback;
    USHORT FramePointer;
    USHORT PaeEnabled:1;
    GCC_ULONG64 KiCallUserMode;
    GCC_ULONG64 KeUserCallbackDispatcher;
    GCC_ULONG64 PsLoadedModuleList;
    GCC_ULONG64 PsActiveProcessHead;
    GCC_ULONG64 PspCidTable;
    GCC_ULONG64 ExpSystemResourcesList;
    GCC_ULONG64 ExpPagedPoolDescriptor;
    GCC_ULONG64 ExpNumberOfPagedPools;
    GCC_ULONG64 KeTimeIncrement;
    GCC_ULONG64 KeBugCheckCallbackListHead;
    GCC_ULONG64 KiBugcheckData;
    GCC_ULONG64 IopErrorLogListHead;
    GCC_ULONG64 ObpRootDirectoryObject;
    GCC_ULONG64 ObpTypeObjectType;
    GCC_ULONG64 MmSystemCacheStart;
    GCC_ULONG64 MmSystemCacheEnd;
    GCC_ULONG64 MmSystemCacheWs;
    GCC_ULONG64 MmPfnDatabase;
    GCC_ULONG64 MmSystemPtesStart;
    GCC_ULONG64 MmSystemPtesEnd;
    GCC_ULONG64 MmSubsectionBase;
    GCC_ULONG64 MmNumberOfPagingFiles;
    GCC_ULONG64 MmLowestPhysicalPage;
    GCC_ULONG64 MmHighestPhysicalPage;
    GCC_ULONG64 MmNumberOfPhysicalPages;
    GCC_ULONG64 MmMaximumNonPagedPoolInBytes;
    GCC_ULONG64 MmNonPagedSystemStart;
    GCC_ULONG64 MmNonPagedPoolStart;
    GCC_ULONG64 MmNonPagedPoolEnd;
    GCC_ULONG64 MmPagedPoolStart;
    GCC_ULONG64 MmPagedPoolEnd;
    GCC_ULONG64 MmPagedPoolInformation;
    ULONG64 MmPageSize;
    GCC_ULONG64 MmSizeOfPagedPoolInBytes;
    GCC_ULONG64 MmTotalCommitLimit;
    GCC_ULONG64 MmTotalCommittedPages;
    GCC_ULONG64 MmSharedCommit;
    GCC_ULONG64 MmDriverCommit;
    GCC_ULONG64 MmProcessCommit;
    GCC_ULONG64 MmPagedPoolCommit;
    GCC_ULONG64 MmExtendedCommit;
    GCC_ULONG64 MmZeroedPageListHead;
    GCC_ULONG64 MmFreePageListHead;
    GCC_ULONG64 MmStandbyPageListHead;
    GCC_ULONG64 MmModifiedPageListHead;
    GCC_ULONG64 MmModifiedNoWritePageListHead;
    GCC_ULONG64 MmAvailablePages;
    GCC_ULONG64 MmResidentAvailablePages;
    GCC_ULONG64 PoolTrackTable;
    GCC_ULONG64 NonPagedPoolDescriptor;
    GCC_ULONG64 MmHighestUserAddress;
    GCC_ULONG64 MmSystemRangeStart;
    GCC_ULONG64 MmUserProbeAddress;
    GCC_ULONG64 KdPrintCircularBuffer;
    GCC_ULONG64 KdPrintCircularBufferEnd;
    GCC_ULONG64 KdPrintWritePointer;
    GCC_ULONG64 KdPrintRolloverCount;
    GCC_ULONG64 MmLoadedUserImageList;
    GCC_ULONG64 NtBuildLab;
    GCC_ULONG64 KiNormalSystemCall;
    GCC_ULONG64 KiProcessorBlock;
    GCC_ULONG64 MmUnloadedDrivers;
    GCC_ULONG64 MmLastUnloadedDriver;
    GCC_ULONG64 MmTriageActionTaken;
    GCC_ULONG64 MmSpecialPoolTag;
    GCC_ULONG64 KernelVerifier;
    GCC_ULONG64 MmVerifierData;
    GCC_ULONG64 MmAllocatedNonPagedPool;
    GCC_ULONG64 MmPeakCommitment;
    GCC_ULONG64 MmTotalCommitLimitMaximum;
    GCC_ULONG64 CmNtCSDVersion;
    GCC_ULONG64 MmPhysicalMemoryBlock;
    GCC_ULONG64 MmSessionBase;
    GCC_ULONG64 MmSessionSize;
    GCC_ULONG64 MmSystemParentTablePage;
    GCC_ULONG64 MmVirtualTranslationBase;
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
    GCC_ULONG64 KdPrintCircularBufferPtr;
    GCC_ULONG64 KdPrintBufferSize;
    GCC_ULONG64 KeLoaderBlock;
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
    GCC_ULONG64 IopNumTriageDumpDataBlocks;
    GCC_ULONG64 IopTriageDumpDataBlocks;
    GCC_ULONG64 VfCrashDataBlock;
} KDDEBUGGER_DATA64, *PKDDEBUGGER_DATA64;

#endif
