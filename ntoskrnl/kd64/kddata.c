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
#if defined(_M_IX86) || defined(_M_AMD64)
#define KPCR KIPCR
#endif

#if defined(_M_IX86)

#define KPCR_SELF_PCR_OFFSET           FIELD_OFFSET(KPCR, SelfPcr)
#define KPCR_CURRENT_PRCB_OFFSET       FIELD_OFFSET(KPCR, Prcb)
#define KPCR_CONTAINED_PRCB_OFFSET     FIELD_OFFSET(KPCR, PrcbData)
#define KPCR_INITIAL_STACK_OFFSET      0
#define KPCR_STACK_LIMIT_OFFSET        0
#define KPRCB_PCR_PAGE_OFFSET          0
#define CBSTACK_FRAME_POINTER          Ebp

#elif defined(_M_AMD64)

#define KPCR_SELF_PCR_OFFSET           FIELD_OFFSET(KPCR, Self)
#define KPCR_CURRENT_PRCB_OFFSET       FIELD_OFFSET(KPCR, CurrentPrcb)
#define KPCR_CONTAINED_PRCB_OFFSET     FIELD_OFFSET(KPCR, Prcb)
#define KPCR_INITIAL_STACK_OFFSET      0
#define KPCR_STACK_LIMIT_OFFSET        0
#define KPRCB_PCR_PAGE_OFFSET          0
#define CBSTACK_FRAME_POINTER          Rbp

#elif defined(_M_ARM)

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
CHAR KdpMessageBuffer[KDP_MSG_BUFFER_SIZE];
CHAR KdpPathBuffer[KDP_MSG_BUFFER_SIZE];

//
// KdPrint Buffers
//
CHAR KdPrintDefaultCircularBuffer[KD_DEFAULT_LOG_BUFFER_SIZE];
PCHAR KdPrintWritePointer = KdPrintDefaultCircularBuffer;
ULONG KdPrintRolloverCount;
PCHAR KdPrintCircularBuffer = KdPrintDefaultCircularBuffer;
ULONG KdPrintBufferSize = sizeof(KdPrintDefaultCircularBuffer);
ULONG KdPrintBufferChanges = 0;
KSPIN_LOCK KdpPrintSpinLock;

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
ULONG Kd_VDSDYN_Mask;   // Specified in Vista+
ULONG Kd_VDSDYNDR_Mask;
ULONG Kd_VDSLDR_Mask;   // Specified in Vista+
ULONG Kd_VDSUTIL_Mask;
ULONG Kd_DFRGIFC_Mask;
ULONG Kd_DEFAULT_Mask;
ULONG Kd_MM_Mask;
ULONG Kd_DFSC_Mask;
ULONG Kd_WOW64_Mask;
//
// Components specified in Vista+, some of which we also use in ReactOS
//
ULONG Kd_ALPC_Mask;
ULONG Kd_WDI_Mask;
ULONG Kd_PERFLIB_Mask;
ULONG Kd_KTM_Mask;
ULONG Kd_IOSTRESS_Mask;
ULONG Kd_HEAP_Mask;
ULONG Kd_WHEA_Mask;
ULONG Kd_USERGDI_Mask;
ULONG Kd_MMCSS_Mask;
ULONG Kd_TPM_Mask;
ULONG Kd_THREADORDER_Mask;
ULONG Kd_ENVIRON_Mask;
ULONG Kd_EMS_Mask;
ULONG Kd_WDT_Mask;
ULONG Kd_FVEVOL_Mask;
ULONG Kd_NDIS_Mask;
ULONG Kd_NVCTRACE_Mask;
ULONG Kd_LUAFV_Mask;
ULONG Kd_APPCOMPAT_Mask;
ULONG Kd_USBSTOR_Mask;
ULONG Kd_SBP2PORT_Mask;
ULONG Kd_COVERAGE_Mask;
ULONG Kd_CACHEMGR_Mask;
ULONG Kd_MOUNTMGR_Mask;
ULONG Kd_CFR_Mask;
ULONG Kd_TXF_Mask;
ULONG Kd_KSECDD_Mask;
ULONG Kd_FLTREGRESS_Mask;
ULONG Kd_MPIO_Mask;
ULONG Kd_MSDSM_Mask;
ULONG Kd_UDFS_Mask;
ULONG Kd_PSHED_Mask;
ULONG Kd_STORVSP_Mask;
ULONG Kd_LSASS_Mask;
ULONG Kd_SSPICLI_Mask;
ULONG Kd_CNG_Mask;
ULONG Kd_EXFAT_Mask;
ULONG Kd_FILETRACE_Mask;
ULONG Kd_XSAVE_Mask;
ULONG Kd_SE_Mask;
ULONG Kd_DRIVEEXTENDER_Mask;
//
// Components specified in Windows 8
//
ULONG Kd_POWER_Mask;
ULONG Kd_CRASHDUMPXHCI_Mask;
ULONG Kd_GPIO_Mask;
ULONG Kd_REFS_Mask;
ULONG Kd_WER_Mask;
//
// Components specified in Windows 10
//
ULONG Kd_CAPIMG_Mask;
ULONG Kd_VPCI_Mask;
ULONG Kd_STORAGECLASSMEMORY_Mask;
ULONG Kd_FSLIB_Mask;
// End Mask
ULONG Kd_ENDOFTABLE_Mask;

//
// Debug Filter Component Table
//
PULONG KdComponentTable[MAX_KD_COMPONENT_TABLE_ENTRIES] =
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
    &Kd_VDSDYN_Mask,    // Specified in Vista+
    &Kd_VDSDYNDR_Mask,
    &Kd_VDSLDR_Mask,    // Specified in Vista+
    &Kd_VDSUTIL_Mask,
    &Kd_DFRGIFC_Mask,
    &Kd_DEFAULT_Mask,
    &Kd_MM_Mask,
    &Kd_DFSC_Mask,
    &Kd_WOW64_Mask,
//
// Components specified in Vista+, some of which we also use in ReactOS
//
    &Kd_ALPC_Mask,
    &Kd_WDI_Mask,
    &Kd_PERFLIB_Mask,
    &Kd_KTM_Mask,
    &Kd_IOSTRESS_Mask,
    &Kd_HEAP_Mask,
    &Kd_WHEA_Mask,
    &Kd_USERGDI_Mask,
    &Kd_MMCSS_Mask,
    &Kd_TPM_Mask,
    &Kd_THREADORDER_Mask,
    &Kd_ENVIRON_Mask,
    &Kd_EMS_Mask,
    &Kd_WDT_Mask,
    &Kd_FVEVOL_Mask,
    &Kd_NDIS_Mask,
    &Kd_NVCTRACE_Mask,
    &Kd_LUAFV_Mask,
    &Kd_APPCOMPAT_Mask,
    &Kd_USBSTOR_Mask,
    &Kd_SBP2PORT_Mask,
    &Kd_COVERAGE_Mask,
    &Kd_CACHEMGR_Mask,
    &Kd_MOUNTMGR_Mask,
    &Kd_CFR_Mask,
    &Kd_TXF_Mask,
    &Kd_KSECDD_Mask,
    &Kd_FLTREGRESS_Mask,
    &Kd_MPIO_Mask,
    &Kd_MSDSM_Mask,
    &Kd_UDFS_Mask,
    &Kd_PSHED_Mask,
    &Kd_STORVSP_Mask,
    &Kd_LSASS_Mask,
    &Kd_SSPICLI_Mask,
    &Kd_CNG_Mask,
    &Kd_EXFAT_Mask,
    &Kd_FILETRACE_Mask,
    &Kd_XSAVE_Mask,
    &Kd_SE_Mask,
    &Kd_DRIVEEXTENDER_Mask,
//
// Components specified in Windows 8
//
    &Kd_POWER_Mask,
    &Kd_CRASHDUMPXHCI_Mask,
    &Kd_GPIO_Mask,
    &Kd_REFS_Mask,
    &Kd_WER_Mask,
//
// Components specified in Windows 10
//
    &Kd_CAPIMG_Mask,
    &Kd_VPCI_Mask,
    &Kd_STORAGECLASSMEMORY_Mask,
    &Kd_FSLIB_Mask,
// End Mask
    &Kd_ENDOFTABLE_Mask,
};

ULONG KdComponentTableSize = RTL_NUMBER_OF(KdComponentTable);

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
#if defined(_M_AMD64) || defined(_M_ARM64)
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

#if (NTDDI_VERSION >= NTDDI_WS03)
C_ASSERT(sizeof(KDDEBUGGER_DATA64) >= 0x318);
#endif

#if !defined(_WIN64) && (defined(__GNUC__) || defined(__clang__))
/* Minimal hackery for GCC/Clang, see commit b9cd3f2d9 (r25845) and de81021ba */
#define PtrToUL64(x)    ((ULPTR64)(ULONG_PTR)(x))
#else
#define PtrToUL64(x)    ((ULPTR64)(x))
#endif
KDDEBUGGER_DATA64 KdDebuggerDataBlock =
{
    {{0}},
    0,
    PtrToUL64(RtlpBreakWithStatusInstruction),
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
    PtrToUL64(KiCallUserMode),
    0,
    PtrToUL64(&PsLoadedModuleList),
    PtrToUL64(&PsActiveProcessHead),
    PtrToUL64(&PspCidTable),
    PtrToUL64(&ExpSystemResourcesList),
    PtrToUL64(ExpPagedPoolDescriptor),
    PtrToUL64(&ExpNumberOfPagedPools),
    PtrToUL64(&KeTimeIncrement),
    PtrToUL64(&KeBugcheckCallbackListHead),
    PtrToUL64(KiBugCheckData),
    PtrToUL64(&IopErrorLogListHead),
    PtrToUL64(&ObpRootDirectoryObject),
    PtrToUL64(&ObpTypeObjectType),
    PtrToUL64(&MmSystemCacheStart),
    PtrToUL64(&MmSystemCacheEnd),
    PtrToUL64(&MmSystemCacheWs),
    PtrToUL64(&MmPfnDatabase),
    PtrToUL64(MmSystemPtesStart),
    PtrToUL64(MmSystemPtesEnd),
    PtrToUL64(&MmSubsectionBase),
    PtrToUL64(&MmNumberOfPagingFiles),
    PtrToUL64(&MmLowestPhysicalPage),
    PtrToUL64(&MmHighestPhysicalPage),
    PtrToUL64(&MmNumberOfPhysicalPages),
    PtrToUL64(&MmMaximumNonPagedPoolInBytes),
    PtrToUL64(&MmNonPagedSystemStart),
    PtrToUL64(&MmNonPagedPoolStart),
    PtrToUL64(&MmNonPagedPoolEnd),
    PtrToUL64(&MmPagedPoolStart),
    PtrToUL64(&MmPagedPoolEnd),
    PtrToUL64(&MmPagedPoolInfo),
    PAGE_SIZE,
    PtrToUL64(&MmSizeOfPagedPoolInBytes),
    PtrToUL64(&MmTotalCommitLimit),
    PtrToUL64(&MmTotalCommittedPages),
    PtrToUL64(&MmSharedCommit),
    PtrToUL64(&MmDriverCommit),
    PtrToUL64(&MmProcessCommit),
    PtrToUL64(&MmPagedPoolCommit),
    PtrToUL64(0),
    PtrToUL64(&MmZeroedPageListHead),
    PtrToUL64(&MmFreePageListHead),
    PtrToUL64(&MmStandbyPageListHead),
    PtrToUL64(&MmModifiedPageListHead),
    PtrToUL64(&MmModifiedNoWritePageListHead),
    PtrToUL64(&MmAvailablePages),
    PtrToUL64(&MmResidentAvailablePages),
    PtrToUL64(&PoolTrackTable),
    PtrToUL64(&NonPagedPoolDescriptor),
    PtrToUL64(&MmHighestUserAddress),
    PtrToUL64(&MmSystemRangeStart),
    PtrToUL64(&MmUserProbeAddress),
    PtrToUL64(KdPrintDefaultCircularBuffer),
    PtrToUL64(KdPrintDefaultCircularBuffer + sizeof(KdPrintDefaultCircularBuffer)),
    PtrToUL64(&KdPrintWritePointer),
    PtrToUL64(&KdPrintRolloverCount),
    PtrToUL64(&MmLoadedUserImageList),
    PtrToUL64(&NtBuildLab),
    PtrToUL64(0),
    PtrToUL64(KiProcessorBlock),
    PtrToUL64(&MmUnloadedDrivers),
    PtrToUL64(&MmLastUnloadedDrivers),
    PtrToUL64(&MmTriageActionTaken),
    PtrToUL64(&MmSpecialPoolTag),
    PtrToUL64(&KernelVerifier),
    PtrToUL64(&MmVerifierData),
    PtrToUL64(&MmAllocatedNonPagedPool),
    PtrToUL64(&MmPeakCommitment),
    PtrToUL64(&MmtotalCommitLimitMaximum),
    PtrToUL64(&CmNtCSDVersion),
    PtrToUL64(&MmPhysicalMemoryBlock),
    PtrToUL64(&MmSessionBase),
    PtrToUL64(&MmSessionSize),
    PtrToUL64(0),
    PtrToUL64(0),
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
    PtrToUL64(&KdPrintCircularBuffer),
    PtrToUL64(&KdPrintBufferSize),
    PtrToUL64(&KeLoaderBlock),
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
#if defined(_M_IX86)
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
#elif defined(_M_AMD64)
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
    PtrToUL64(&IopNumTriageDumpDataBlocks),
    PtrToUL64(IopTriageDumpDataBlocks),

#if (NTDDI_VERSION >= NTDDI_LONGHORN)
#error KdDebuggerDataBlock requires other fields for this NT version!
#endif
};
