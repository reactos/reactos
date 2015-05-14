/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/kd64/kddata.c
 * PURPOSE:         Contains all global variables and settings for KD64
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include <mm/ARM3/miarm.h>
#undef MmSystemRangeStart

VOID NTAPI RtlpBreakWithStatusInstruction(VOID);

//
// Apply the KIPCR WDK workaround for x86 and AMD64
//
#if defined(_X86_) || defined(_AMD64_)
#define KPCR KIPCR
#endif

#if defined(_X86_)

#define KPCR_SELF_PCR_OFFSET           FIELD_OFFSET(KPCR, Self)
#define KPCR_CURRENT_PRCB_OFFSET       FIELD_OFFSET(KPCR, Prcb)
#define KPCR_CONTAINED_PRCB_OFFSET     FIELD_OFFSET(KPCR, PrcbData)
#define KPCR_INITIAL_STACK_OFFSET      0
#define KPCR_STACK_LIMIT_OFFSET        0
#define KPRCB_PCR_PAGE_OFFSET          0
#define CBSTACK_FRAME_POINTER          Ebp

#elif defined(_AMD64_)

#define KPCR_SELF_PCR_OFFSET           FIELD_OFFSET(KPCR, Self)
#define KPCR_CURRENT_PRCB_OFFSET       FIELD_OFFSET(KPCR, CurrentPrcb)
#define KPCR_CONTAINED_PRCB_OFFSET     FIELD_OFFSET(KPCR, Prcb)
#define KPCR_INITIAL_STACK_OFFSET      0
#define KPCR_STACK_LIMIT_OFFSET        0
#define KPRCB_PCR_PAGE_OFFSET          0
#define CBSTACK_FRAME_POINTER          Rbp

#elif defined(_ARM_)

#define KPCR_SELF_PCR_OFFSET           0
#define KPCR_CURRENT_PRCB_OFFSET       FIELD_OFFSET(KIPCR, Prcb)
#define KPCR_CONTAINED_PRCB_OFFSET     0
#define KPCR_INITIAL_STACK_OFFSET      FIELD_OFFSET(KPCR, InitialStack)
#define KPCR_STACK_LIMIT_OFFSET        FIELD_OFFSET(KPCR, StackLimit)
#define KPRCB_PCR_PAGE_OFFSET          FIELD_OFFSET(KPRCB, PcrPage)
#define CBSTACK_FRAME_POINTER          DummyFramePointer

#else
#error Unsupported Architecture
#endif

/* GLOBALS *******************************************************************/

//
// Debugger State
//
KD_CONTEXT KdpContext;
BOOLEAN KdpPortLocked;
KSPIN_LOCK KdpDebuggerLock;
BOOLEAN KdpControlCPressed;
BOOLEAN KdpContextSent;

//
// Debug Trap Handlers
//
PKDEBUG_ROUTINE KiDebugRoutine = KdpStub;
PKDEBUG_SWITCH_ROUTINE KiDebugSwitchRoutine;

//
// Debugger Configuration Settings
//
BOOLEAN KdBreakAfterSymbolLoad;
BOOLEAN KdPitchDebugger;
BOOLEAN KdDebuggerNotPresent;
BOOLEAN KdDebuggerEnabled;
BOOLEAN KdAutoEnableOnEvent;
BOOLEAN KdBlockEnable;
BOOLEAN KdIgnoreUmExceptions;
BOOLEAN KdPreviouslyEnabled;
BOOLEAN KdpDebuggerStructuresInitialized;
BOOLEAN KdEnteredDebugger;
ULONG KdDisableCount;
LARGE_INTEGER KdPerformanceCounterRate;

//
// Breakpoint Data
//
BREAKPOINT_ENTRY KdpBreakpointTable[KD_BREAKPOINT_MAX];
KD_BREAKPOINT_TYPE KdpBreakpointInstruction = KD_BREAKPOINT_VALUE;
BOOLEAN KdpOweBreakpoint;
BOOLEAN BreakpointsSuspended;
ULONG KdpNumInternalBreakpoints;

//
// Symbol Data
//
ULONG_PTR KdpCurrentSymbolStart, KdpCurrentSymbolEnd;

//
// Tracepoint Data
//
ULONG TraceDataBuffer[40];
ULONG TraceDataBufferPosition = 1;

//
// Time Slip Support
//
KDPC KdpTimeSlipDpc;
KTIMER KdpTimeSlipTimer;
WORK_QUEUE_ITEM KdpTimeSlipWorkItem;
LONG KdpTimeSlipPending = 1;
PKEVENT KdpTimeSlipEvent;
KSPIN_LOCK KdpTimeSlipEventLock;
LARGE_INTEGER KdTimerStop, KdTimerStart, KdTimerDifference;

//
// Buffers
//
CHAR KdpMessageBuffer[0x1000];
CHAR KdpPathBuffer[0x1000];

//
// KdPrint Buffers
//
CHAR KdPrintDefaultCircularBuffer[KD_DEFAULT_LOG_BUFFER_SIZE];
PCHAR KdPrintWritePointer = KdPrintDefaultCircularBuffer;
ULONG KdPrintRolloverCount;
PCHAR KdPrintCircularBuffer = KdPrintDefaultCircularBuffer;
ULONG KdPrintBufferSize = sizeof(KdPrintDefaultCircularBuffer);
ULONG KdPrintBufferChanges = 0;

//
// Debug Filter Masks
//
ULONG Kd_WIN2000_Mask = 1;
ULONG Kd_SYSTEM_Mask;
ULONG Kd_SMSS_Mask;
ULONG Kd_SETUP_Mask;
ULONG Kd_NTFS_Mask;
ULONG Kd_FSTUB_Mask;
ULONG Kd_CRASHDUMP_Mask;
ULONG Kd_CDAUDIO_Mask;
ULONG Kd_CDROM_Mask;
ULONG Kd_CLASSPNP_Mask;
ULONG Kd_DISK_Mask;
ULONG Kd_REDBOOK_Mask;
ULONG Kd_STORPROP_Mask;
ULONG Kd_SCSIPORT_Mask;
ULONG Kd_SCSIMINIPORT_Mask;
ULONG Kd_CONFIG_Mask;
ULONG Kd_I8042PRT_Mask;
ULONG Kd_SERMOUSE_Mask;
ULONG Kd_LSERMOUS_Mask;
ULONG Kd_KBDHID_Mask;
ULONG Kd_MOUHID_Mask;
ULONG Kd_KBDCLASS_Mask;
ULONG Kd_MOUCLASS_Mask;
ULONG Kd_TWOTRACK_Mask;
ULONG Kd_WMILIB_Mask;
ULONG Kd_ACPI_Mask;
ULONG Kd_AMLI_Mask;
ULONG Kd_HALIA64_Mask;
ULONG Kd_VIDEO_Mask;
ULONG Kd_SVCHOST_Mask;
ULONG Kd_VIDEOPRT_Mask;
ULONG Kd_TCPIP_Mask;
ULONG Kd_DMSYNTH_Mask;
ULONG Kd_NTOSPNP_Mask;
ULONG Kd_FASTFAT_Mask;
ULONG Kd_SAMSS_Mask;
ULONG Kd_PNPMGR_Mask;
ULONG Kd_NETAPI_Mask;
ULONG Kd_SCSERVER_Mask;
ULONG Kd_SCCLIENT_Mask;
ULONG Kd_SERIAL_Mask;
ULONG Kd_SERENUM_Mask;
ULONG Kd_UHCD_Mask;
ULONG Kd_RPCPROXY_Mask;
ULONG Kd_AUTOCHK_Mask;
ULONG Kd_DCOMSS_Mask;
ULONG Kd_UNIMODEM_Mask;
ULONG Kd_SIS_Mask;
ULONG Kd_FLTMGR_Mask;
ULONG Kd_WMICORE_Mask;
ULONG Kd_BURNENG_Mask;
ULONG Kd_IMAPI_Mask;
ULONG Kd_SXS_Mask;
ULONG Kd_FUSION_Mask;
ULONG Kd_IDLETASK_Mask;
ULONG Kd_SOFTPCI_Mask;
ULONG Kd_TAPE_Mask;
ULONG Kd_MCHGR_Mask;
ULONG Kd_IDEP_Mask;
ULONG Kd_PCIIDE_Mask;
ULONG Kd_FLOPPY_Mask;
ULONG Kd_FDC_Mask;
ULONG Kd_TERMSRV_Mask;
ULONG Kd_W32TIME_Mask;
ULONG Kd_PREFETCHER_Mask;
ULONG Kd_RSFILTER_Mask;
ULONG Kd_FCPORT_Mask;
ULONG Kd_PCI_Mask;
ULONG Kd_DMIO_Mask;
ULONG Kd_DMCONFIG_Mask;
ULONG Kd_DMADMIN_Mask;
ULONG Kd_WSOCKTRANSPORT_Mask;
ULONG Kd_VSS_Mask;
ULONG Kd_PNPMEM_Mask;
ULONG Kd_PROCESSOR_Mask;
ULONG Kd_DMSERVER_Mask;
ULONG Kd_SR_Mask;
ULONG Kd_INFINIBAND_Mask;
ULONG Kd_IHVDRIVER_Mask;
ULONG Kd_IHVVIDEO_Mask;
ULONG Kd_IHVAUDIO_Mask;
ULONG Kd_IHVNETWORK_Mask;
ULONG Kd_IHVSTREAMING_Mask;
ULONG Kd_IHVBUS_Mask;
ULONG Kd_HPS_Mask;
ULONG Kd_RTLTHREADPOOL_Mask;
ULONG Kd_LDR_Mask;
ULONG Kd_TCPIP6_Mask;
ULONG Kd_ISAPNP_Mask;
ULONG Kd_SHPC_Mask;
ULONG Kd_STORPORT_Mask;
ULONG Kd_STORMINIPORT_Mask;
ULONG Kd_PRINTSPOOLER_Mask;
ULONG Kd_VSSDYNDISK_Mask;
ULONG Kd_VERIFIER_Mask;
ULONG Kd_VDS_Mask;
ULONG Kd_VDSBAS_Mask;
ULONG Kd_VDSDYNDR_Mask;
ULONG Kd_VDSUTIL_Mask;
ULONG Kd_DFRGIFC_Mask;
ULONG Kd_DEFAULT_Mask;
ULONG Kd_MM_Mask;
ULONG Kd_DFSC_Mask;
ULONG Kd_WOW64_Mask;
ULONG Kd_ENDOFTABLE_Mask;

//
// Debug Filter Component Table
//
PULONG KdComponentTable[104] =
{
    &Kd_SYSTEM_Mask,
    &Kd_SMSS_Mask,
    &Kd_SETUP_Mask,
    &Kd_NTFS_Mask,
    &Kd_FSTUB_Mask,
    &Kd_CRASHDUMP_Mask,
    &Kd_CDAUDIO_Mask,
    &Kd_CDROM_Mask,
    &Kd_CLASSPNP_Mask,
    &Kd_DISK_Mask,
    &Kd_REDBOOK_Mask,
    &Kd_STORPROP_Mask,
    &Kd_SCSIPORT_Mask,
    &Kd_SCSIMINIPORT_Mask,
    &Kd_CONFIG_Mask,
    &Kd_I8042PRT_Mask,
    &Kd_SERMOUSE_Mask,
    &Kd_LSERMOUS_Mask,
    &Kd_KBDHID_Mask,
    &Kd_MOUHID_Mask,
    &Kd_KBDCLASS_Mask,
    &Kd_MOUCLASS_Mask,
    &Kd_TWOTRACK_Mask,
    &Kd_WMILIB_Mask,
    &Kd_ACPI_Mask,
    &Kd_AMLI_Mask,
    &Kd_HALIA64_Mask,
    &Kd_VIDEO_Mask,
    &Kd_SVCHOST_Mask,
    &Kd_VIDEOPRT_Mask,
    &Kd_TCPIP_Mask,
    &Kd_DMSYNTH_Mask,
    &Kd_NTOSPNP_Mask,
    &Kd_FASTFAT_Mask,
    &Kd_SAMSS_Mask,
    &Kd_PNPMGR_Mask,
    &Kd_NETAPI_Mask,
    &Kd_SCSERVER_Mask,
    &Kd_SCCLIENT_Mask,
    &Kd_SERIAL_Mask,
    &Kd_SERENUM_Mask,
    &Kd_UHCD_Mask,
    &Kd_RPCPROXY_Mask,
    &Kd_AUTOCHK_Mask,
    &Kd_DCOMSS_Mask,
    &Kd_UNIMODEM_Mask,
    &Kd_SIS_Mask,
    &Kd_FLTMGR_Mask,
    &Kd_WMICORE_Mask,
    &Kd_BURNENG_Mask,
    &Kd_IMAPI_Mask,
    &Kd_SXS_Mask,
    &Kd_FUSION_Mask,
    &Kd_IDLETASK_Mask,
    &Kd_SOFTPCI_Mask,
    &Kd_TAPE_Mask,
    &Kd_MCHGR_Mask,
    &Kd_IDEP_Mask,
    &Kd_PCIIDE_Mask,
    &Kd_FLOPPY_Mask,
    &Kd_FDC_Mask,
    &Kd_TERMSRV_Mask,
    &Kd_W32TIME_Mask,
    &Kd_PREFETCHER_Mask,
    &Kd_RSFILTER_Mask,
    &Kd_FCPORT_Mask,
    &Kd_PCI_Mask,
    &Kd_DMIO_Mask,
    &Kd_DMCONFIG_Mask,
    &Kd_DMADMIN_Mask,
    &Kd_WSOCKTRANSPORT_Mask,
    &Kd_VSS_Mask,
    &Kd_PNPMEM_Mask,
    &Kd_PROCESSOR_Mask,
    &Kd_DMSERVER_Mask,
    &Kd_SR_Mask,
    &Kd_INFINIBAND_Mask,
    &Kd_IHVDRIVER_Mask,
    &Kd_IHVVIDEO_Mask,
    &Kd_IHVAUDIO_Mask,
    &Kd_IHVNETWORK_Mask,
    &Kd_IHVSTREAMING_Mask,
    &Kd_IHVBUS_Mask,
    &Kd_HPS_Mask,
    &Kd_RTLTHREADPOOL_Mask,
    &Kd_LDR_Mask,
    &Kd_TCPIP6_Mask,
    &Kd_ISAPNP_Mask,
    &Kd_SHPC_Mask,
    &Kd_STORPORT_Mask,
    &Kd_STORMINIPORT_Mask,
    &Kd_PRINTSPOOLER_Mask,
    &Kd_VSSDYNDISK_Mask,
    &Kd_VERIFIER_Mask,
    &Kd_VDS_Mask,
    &Kd_VDSBAS_Mask,
    &Kd_VDSDYNDR_Mask,
    &Kd_VDSUTIL_Mask,
    &Kd_DFRGIFC_Mask,
    &Kd_DEFAULT_Mask,
    &Kd_MM_Mask,
    &Kd_DFSC_Mask,
    &Kd_WOW64_Mask,
    &Kd_ENDOFTABLE_Mask,
};

ULONG KdComponentTableSize = sizeof(KdComponentTable) / sizeof(KdComponentTable[0]);

//
// Debugger Data
//
LIST_ENTRY KdpDebuggerDataListHead;
KSPIN_LOCK KdpDataSpinLock;

//
// Debugger Version and Data Block
//
DBGKD_GET_VERSION64 KdVersionBlock =
{
    0,
    0,
    DBGKD_64BIT_PROTOCOL_VERSION2,
    CURRENT_KD_SECONDARY_VERSION,
#if defined(_WIN64)
    DBGKD_VERS_FLAG_DATA | DBGKD_VERS_FLAG_PTR64,
#else
    DBGKD_VERS_FLAG_DATA,
#endif
    IMAGE_FILE_MACHINE_NATIVE,
    PACKET_TYPE_MAX,
    0,
    0,
    DBGKD_SIMULATION_NONE,
    {0},
    0,
    0,
    0
};
KDDEBUGGER_DATA64 KdDebuggerDataBlock =
{
    {{0}},
    0,
    {(ULONG_PTR)RtlpBreakWithStatusInstruction},
    0,
    FIELD_OFFSET(KTHREAD, CallbackStack),
#if defined(_M_ARM) || defined(_M_AMD64)
    0,
    0,
#else
    FIELD_OFFSET(KCALLOUT_FRAME, CallbackStack),
    FIELD_OFFSET(KCALLOUT_FRAME, CBSTACK_FRAME_POINTER),
#endif
    FALSE,
    {(ULONG_PTR)KiCallUserMode},
    0,
    {(ULONG_PTR)&PsLoadedModuleList},
    {(ULONG_PTR)&PsActiveProcessHead},
    {(ULONG_PTR)&PspCidTable},
    {(ULONG_PTR)&ExpSystemResourcesList},
    {(ULONG_PTR)ExpPagedPoolDescriptor},
    {(ULONG_PTR)&ExpNumberOfPagedPools},
    {(ULONG_PTR)&KeTimeIncrement},
    {(ULONG_PTR)&KeBugcheckCallbackListHead},
    {(ULONG_PTR)KiBugCheckData},
    {(ULONG_PTR)&IopErrorLogListHead},
    {(ULONG_PTR)&ObpRootDirectoryObject},
    {(ULONG_PTR)&ObpTypeObjectType},
    {(ULONG_PTR)&MmSystemCacheStart},
    {(ULONG_PTR)&MmSystemCacheEnd},
    {(ULONG_PTR)&MmSystemCacheWs},
    {(ULONG_PTR)&MmPfnDatabase},
    {(ULONG_PTR)MmSystemPtesStart},
    {(ULONG_PTR)MmSystemPtesEnd},
    {(ULONG_PTR)&MmSubsectionBase},
    {(ULONG_PTR)&MmNumberOfPagingFiles},
    {(ULONG_PTR)&MmLowestPhysicalPage},
    {(ULONG_PTR)&MmHighestPhysicalPage},
    {(ULONG_PTR)&MmNumberOfPhysicalPages},
    {(ULONG_PTR)&MmMaximumNonPagedPoolInBytes},
    {(ULONG_PTR)&MmNonPagedSystemStart},
    {(ULONG_PTR)&MmNonPagedPoolStart},
    {(ULONG_PTR)&MmNonPagedPoolEnd},
    {(ULONG_PTR)&MmPagedPoolStart},
    {(ULONG_PTR)&MmPagedPoolEnd},
    {(ULONG_PTR)&MmPagedPoolInfo},
    PAGE_SIZE,
    {(ULONG_PTR)&MmSizeOfPagedPoolInBytes},
    {(ULONG_PTR)&MmTotalCommitLimit},
    {(ULONG_PTR)&MmTotalCommittedPages},
    {(ULONG_PTR)&MmSharedCommit},
    {(ULONG_PTR)&MmDriverCommit},
    {(ULONG_PTR)&MmProcessCommit},
    {(ULONG_PTR)&MmPagedPoolCommit},
    {0},
    {(ULONG_PTR)&MmZeroedPageListHead},
    {(ULONG_PTR)&MmFreePageListHead},
    {(ULONG_PTR)&MmStandbyPageListHead},
    {(ULONG_PTR)&MmModifiedPageListHead},
    {(ULONG_PTR)&MmModifiedNoWritePageListHead},
    {(ULONG_PTR)&MmAvailablePages},
    {(ULONG_PTR)&MmResidentAvailablePages},
    {(ULONG_PTR)&PoolTrackTable},
    {(ULONG_PTR)&NonPagedPoolDescriptor},
    {(ULONG_PTR)&MmHighestUserAddress},
    {(ULONG_PTR)&MmSystemRangeStart},
    {(ULONG_PTR)&MmUserProbeAddress},
    {(ULONG_PTR)KdPrintDefaultCircularBuffer},
    {(ULONG_PTR)KdPrintDefaultCircularBuffer + 1},
    {(ULONG_PTR)&KdPrintWritePointer},
    {(ULONG_PTR)&KdPrintRolloverCount},
    {(ULONG_PTR)&MmLoadedUserImageList},
    {(ULONG_PTR)&NtBuildLab},
    {0},
    {(ULONG_PTR)KiProcessorBlock},
    {(ULONG_PTR)&MmUnloadedDrivers},
    {(ULONG_PTR)&MmLastUnloadedDrivers},
    {(ULONG_PTR)&MmTriageActionTaken},
    {(ULONG_PTR)&MmSpecialPoolTag},
    {(ULONG_PTR)&KernelVerifier},
    {(ULONG_PTR)&MmVerifierData},
    {(ULONG_PTR)&MmAllocatedNonPagedPool},
    {(ULONG_PTR)&MmPeakCommitment},
    {(ULONG_PTR)&MmtotalCommitLimitMaximum},
    {(ULONG_PTR)&CmNtCSDVersion},
    {(ULONG_PTR)&MmPhysicalMemoryBlock},
    {(ULONG_PTR)&MmSessionBase},
    {(ULONG_PTR)&MmSessionSize},
    {0},
    {0},
    FIELD_OFFSET(KTHREAD, NextProcessor),
    FIELD_OFFSET(KTHREAD, Teb),
    FIELD_OFFSET(KTHREAD, KernelStack),
    FIELD_OFFSET(KTHREAD, InitialStack),
    FIELD_OFFSET(KTHREAD, ApcState.Process),
    FIELD_OFFSET(KTHREAD, State),
    0,
    0,
    sizeof(EPROCESS),
    FIELD_OFFSET(EPROCESS, Peb),
    FIELD_OFFSET(EPROCESS, InheritedFromUniqueProcessId),
    FIELD_OFFSET(EPROCESS, Pcb.DirectoryTableBase),
    sizeof(KPRCB),
    FIELD_OFFSET(KPRCB, DpcRoutineActive),
    FIELD_OFFSET(KPRCB, CurrentThread),
    FIELD_OFFSET(KPRCB, MHz),
    FIELD_OFFSET(KPRCB, CpuType),
    FIELD_OFFSET(KPRCB, VendorString),
    FIELD_OFFSET(KPRCB, ProcessorState.ContextFrame),
    FIELD_OFFSET(KPRCB, Number),
    sizeof(ETHREAD),
    {(ULONG_PTR)KdPrintDefaultCircularBuffer},
    {(ULONG_PTR)&KdPrintBufferSize},
    {(ULONG_PTR)&KeLoaderBlock},
    sizeof(KPCR),
    KPCR_SELF_PCR_OFFSET,
    KPCR_CURRENT_PRCB_OFFSET,
    KPCR_CONTAINED_PRCB_OFFSET,
    0,
    0,
#if defined(_M_ARM)
    _WARN("KPCR_INITIAL_STACK_OFFSET, KPCR_STACK_LIMIT_OFFSET and KPRCB_PCR_PAGE_OFFSET not properly defined on ARM")
    0,
    0,
    0,
#else
    KPCR_INITIAL_STACK_OFFSET,
    KPCR_STACK_LIMIT_OFFSET,
    KPRCB_PCR_PAGE_OFFSET,
#endif
    FIELD_OFFSET(KPRCB, ProcessorState.SpecialRegisters),
#if defined(_X86_)
    //
    // x86 GDT/LDT/TSS constants
    //
    KGDT_R0_CODE,
    KGDT_R0_DATA,
    KGDT_R0_PCR,
    KGDT_R3_CODE,
    KGDT_R3_DATA,
    KGDT_R3_TEB,
    KGDT_LDT,
    KGDT_TSS,
    0,
    0,
#elif defined(_AMD64_)
    //
    // AMD64 GDT/LDT/TSS constants
    //
    KGDT64_R0_CODE,
    KGDT64_R3_DATA,
    KGDT64_R3_DATA,
    KGDT64_R3_CODE,
    KGDT64_R3_DATA,
    KGDT64_R3_DATA,
    0,
    KGDT64_SYS_TSS,
    0,
    0,
#else
    //
    // No GDT/LDT/TSS on other architectures
    //
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
#endif
    {(ULONG_PTR)&IopNumTriageDumpDataBlocks},
    {(ULONG_PTR)IopTriageDumpDataBlocks},
};
