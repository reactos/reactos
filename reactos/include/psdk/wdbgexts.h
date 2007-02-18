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

#define KDBG_TAG                                        TAG('G', 'B', 'D', 'K')

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

typedef struct _KDDEBUGGER_DATA64
{
    DBGKD_DEBUG_DATA_HEADER64 Header;
    ULONG64 KernBase;
    ULONG64 BreakpointWithStatus;
    ULONG64 SavedContext;
    USHORT ThCallbackStack;
    USHORT NextCallback;
    USHORT FramePointer;
    USHORT PaeEnabled:1;
    ULONG64 KiCallUserMode;
    ULONG64 KeUserCallbackDispatcher;
    ULONG64 PsLoadedModuleList;
    ULONG64 PsActiveProcessHead;
    ULONG64 PspCidTable;
    ULONG64 ExpSystemResourcesList;
    ULONG64 ExpPagedPoolDescriptor;
    ULONG64 ExpNumberOfPagedPools;
    ULONG64 KeTimeIncrement;
    ULONG64 KeBugCheckCallbackListHead;
    ULONG64 KiBugcheckData;
    ULONG64 IopErrorLogListHead;
    ULONG64 ObpRootDirectoryObject;
    ULONG64 ObpTypeObjectType;
    ULONG64 MmSystemCacheStart;
    ULONG64 MmSystemCacheEnd;
    ULONG64 MmSystemCacheWs;
    ULONG64 MmPfnDatabase;
    ULONG64 MmSystemPtesStart;
    ULONG64 MmSystemPtesEnd;
    ULONG64 MmSubsectionBase;
    ULONG64 MmNumberOfPagingFiles;
    ULONG64 MmLowestPhysicalPage;
    ULONG64 MmHighestPhysicalPage;
    ULONG64 MmNumberOfPhysicalPages;
    ULONG64 MmMaximumNonPagedPoolInBytes;
    ULONG64 MmNonPagedSystemStart;
    ULONG64 MmNonPagedPoolStart;
    ULONG64 MmNonPagedPoolEnd;
    ULONG64 MmPagedPoolStart;
    ULONG64 MmPagedPoolEnd;
    ULONG64 MmPagedPoolInformation;
    ULONG64 MmPageSize;
    ULONG64 MmSizeOfPagedPoolInBytes;
    ULONG64 MmTotalCommitLimit;
    ULONG64 MmTotalCommittedPages;
    ULONG64 MmSharedCommit;
    ULONG64 MmDriverCommit;
    ULONG64 MmProcessCommit;
    ULONG64 MmPagedPoolCommit;
    ULONG64 MmExtendedCommit;
    ULONG64 MmZeroedPageListHead;
    ULONG64 MmFreePageListHead;
    ULONG64 MmStandbyPageListHead;
    ULONG64 MmModifiedPageListHead;
    ULONG64 MmModifiedNoWritePageListHead;
    ULONG64 MmAvailablePages;
    ULONG64 MmResidentAvailablePages;
    ULONG64 PoolTrackTable;
    ULONG64 NonPagedPoolDescriptor;
    ULONG64 MmHighestUserAddress;
    ULONG64 MmSystemRangeStart;
    ULONG64 MmUserProbeAddress;
    ULONG64 KdPrintCircularBuffer;
    ULONG64 KdPrintCircularBufferEnd;
    ULONG64 KdPrintWritePointer;
    ULONG64 KdPrintRolloverCount;
    ULONG64 MmLoadedUserImageList;
    ULONG64 NtBuildLab;
    ULONG64 KiNormalSystemCall;
    ULONG64 KiProcessorBlock;
    ULONG64 MmUnloadedDrivers;
    ULONG64 MmLastUnloadedDriver;
    ULONG64 MmTriageActionTaken;
    ULONG64 MmSpecialPoolTag;
    ULONG64 KernelVerifier;
    ULONG64 MmVerifierData;
    ULONG64 MmAllocatedNonPagedPool;
    ULONG64 MmPeakCommitment;
    ULONG64 MmTotalCommitLimitMaximum;
    ULONG64 CmNtCSDVersion;
    ULONG64 MmPhysicalMemoryBlock;
    ULONG64 MmSessionBase;
    ULONG64 MmSessionSize;
    ULONG64 MmSystemParentTablePage;
    ULONG64 MmVirtualTranslationBase;
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
    ULONG64 KdPrintCircularBufferPtr;
    ULONG64 KdPrintBufferSize;
    ULONG64 KeLoaderBlock;
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
    ULONG64 IopNumTriageDumpDataBlocks;
    ULONG64 IopTriageDumpDataBlocks;
    ULONG64 VfCrashDataBlock;
} KDDEBUGGER_DATA64, *PKDDEBUGGER_DATA64;

#endif
