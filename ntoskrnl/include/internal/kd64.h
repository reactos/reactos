/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/include/internal/kd64.h
 * PURPOSE:         Internal header for the KD64 Library
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

#pragma once

//
// Default size of the DbgPrint log buffer
//
#if DBG
#define KD_DEFAULT_LOG_BUFFER_SIZE  0x8000
#else
#define KD_DEFAULT_LOG_BUFFER_SIZE  0x1000
#endif

//
// Default size of the Message and Path buffers
//
#define KDP_MSG_BUFFER_SIZE 0x1000

//
// Maximum supported number of breakpoints
//
#define KD_BREAKPOINT_MAX   32

//
// Highest limit starting which we consider that breakpoint addresses
// are either in system space, or in user space but inside shared DLLs.
//
// I'm wondering whether this can be computed using MmHighestUserAddress
// or whether there is already some #define somewhere else...
// See https://www.drdobbs.com/windows/faster-dll-load-load/184416918
// and https://www.drdobbs.com/rebasing-win32-dlls/184416272
// for a tentative explanation.
//
#define KD_HIGHEST_USER_BREAKPOINT_ADDRESS  (PVOID)0x60000000  // MmHighestUserAddress

//
// Breakpoint Status Flags
//
#define KD_BREAKPOINT_ACTIVE    0x01
#define KD_BREAKPOINT_PENDING   0x02
#define KD_BREAKPOINT_SUSPENDED 0x04
#define KD_BREAKPOINT_EXPIRED   0x08

//
// Structure for Breakpoints
//
typedef struct _BREAKPOINT_ENTRY
{
    ULONG Flags;
    ULONG_PTR DirectoryTableBase;
    PVOID Address;
    KD_BREAKPOINT_TYPE Content;
} BREAKPOINT_ENTRY, *PBREAKPOINT_ENTRY;

//
// Debug and Multi-Processor Switch Routine Definitions
//
typedef
BOOLEAN
(NTAPI *PKDEBUG_ROUTINE)(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChance
);

//
// Initialization Routines
//
BOOLEAN
NTAPI
KdInitSystem(
    _In_ ULONG BootPhase,
    _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock
);

VOID
NTAPI
KdUpdateDataBlock(
    VOID
);

//
// Determines if the kernel debugger must handle a particular trap
//
BOOLEAN
NTAPI
KdIsThisAKdTrap(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context,
    IN KPROCESSOR_MODE PreviousMode
);

//
// Multi-Processor Switch Support
//
KCONTINUE_STATUS
NTAPI
KdReportProcessorChange(
    VOID);

//
// Time Slip Support
//
VOID
NTAPI
KdpTimeSlipWork(
    IN PVOID Context
);

VOID
NTAPI
KdpTimeSlipDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

//
// Debug Trap Handlers
//
BOOLEAN
NTAPI
KdpStub(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChanceException
);

BOOLEAN
NTAPI
KdpTrap(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChanceException
);

//
// Port Locking
//
VOID
NTAPI
KdpPortLock(
    VOID
);

VOID
NTAPI
KdpPortUnlock(
    VOID
);

BOOLEAN
NTAPI
KdpPollBreakInWithPortLock(
    VOID
);

//
// Debugger Enter, Exit, Enable and Disable
//
BOOLEAN
NTAPI
KdEnterDebugger(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
);

VOID
NTAPI
KdExitDebugger(
    IN BOOLEAN Enable
);

NTSTATUS
NTAPI
KdEnableDebuggerWithLock(
    IN BOOLEAN NeedLock
);

NTSTATUS
NTAPI
KdDisableDebuggerWithLock(
    IN BOOLEAN NeedLock
);

//
// Debug Event Handlers
//
NTSTATUS
NTAPI
KdpPrint(
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_reads_bytes_(Length) PCHAR String,
    _In_ USHORT Length,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ PKEXCEPTION_FRAME ExceptionFrame,
    _Out_ PBOOLEAN Handled
);

USHORT
NTAPI
KdpPrompt(
    _In_reads_bytes_(PromptLength) PCHAR PromptString,
    _In_ USHORT PromptLength,
    _Out_writes_bytes_(MaximumResponseLength) PCHAR ResponseString,
    _In_ USHORT MaximumResponseLength,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ PKEXCEPTION_FRAME ExceptionFrame
);

VOID
NTAPI
KdpSymbol(
    IN PSTRING DllPath,
    IN PKD_SYMBOLS_INFO SymbolInfo,
    IN BOOLEAN Unload,
    IN KPROCESSOR_MODE PreviousMode,
    IN PCONTEXT ContextRecord,
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
);

VOID
NTAPI
KdpCommandString(
    IN PSTRING NameString,
    IN PSTRING CommandString,
    IN KPROCESSOR_MODE PreviousMode,
    IN PCONTEXT ContextRecord,
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
);

//
// State Change Notifications
//
VOID
NTAPI
KdpReportLoadSymbolsStateChange(
    IN PSTRING PathName,
    IN PKD_SYMBOLS_INFO SymbolInfo,
    IN BOOLEAN Unload,
    IN OUT PCONTEXT Context
);

VOID
NTAPI
KdpReportCommandStringStateChange(
    IN PSTRING NameString,
    IN PSTRING CommandString,
    IN OUT PCONTEXT Context
);

BOOLEAN
NTAPI
KdpReportExceptionStateChange(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT Context,
    IN BOOLEAN SecondChanceException
);

//
// Breakpoint Support
//
ULONG
NTAPI
KdpAddBreakpoint(
    IN PVOID Address
);

VOID
NTAPI
KdSetOwedBreakpoints(
    VOID
);

BOOLEAN
NTAPI
KdpDeleteBreakpoint(
    IN ULONG BpEntry
);

BOOLEAN
NTAPI
KdpDeleteBreakpointRange(
    IN PVOID Base,
    IN PVOID Limit
);

VOID
NTAPI
KdpSuspendBreakPoint(
    IN ULONG BpEntry
);

VOID
NTAPI
KdpRestoreAllBreakpoints(
    VOID
);

VOID
NTAPI
KdpSuspendAllBreakPoints(
    VOID
);

//
// Routine to determine if it is safe to disable the debugger
//
NTSTATUS
NTAPI
KdpAllowDisable(
    VOID
);

//
// Safe memory read & write Support
//
NTSTATUS
NTAPI
KdpCopyMemoryChunks(
    _In_ ULONG64 Address,
    _In_ PVOID Buffer,
    _In_ ULONG TotalSize,
    _In_ ULONG ChunkSize,
    _In_ ULONG Flags,
    _Out_opt_ PULONG ActualSize
);

//
// Internal memory handling routines for KD isolation
//
VOID
NTAPI
KdpMoveMemory(
    _In_ PVOID Destination,
    _In_ PVOID Source,
    _In_ SIZE_T Length
);

VOID
NTAPI
KdpZeroMemory(
    _In_ PVOID Destination,
    _In_ SIZE_T Length
);

//
// Low Level Support Routines for the KD API
//

//
// Version
//
VOID
NTAPI
KdpSysGetVersion(
    _Out_ PDBGKD_GET_VERSION64 Version);

//
// Context
//
VOID
NTAPI
KdpGetStateChange(
    IN PDBGKD_MANIPULATE_STATE64 State,
    IN PCONTEXT Context
);

VOID
NTAPI
KdpSetContextState(
    IN PDBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange,
    IN PCONTEXT Context
);

//
// MSR
//
NTSTATUS
NTAPI
KdpSysReadMsr(
    _In_ ULONG Msr,
    _Out_ PULONGLONG MsrValue);

NTSTATUS
NTAPI
KdpSysWriteMsr(
    _In_ ULONG Msr,
    _In_ PULONGLONG MsrValue);

//
// Bus
//
NTSTATUS
NTAPI
KdpSysReadBusData(
    _In_ BUS_DATA_TYPE BusDataType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _In_ ULONG Offset,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_ PULONG ActualLength);

NTSTATUS
NTAPI
KdpSysWriteBusData(
    _In_ BUS_DATA_TYPE BusDataType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _In_ ULONG Offset,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_ PULONG ActualLength);

//
// Control Space
//
NTSTATUS
NTAPI
KdpSysReadControlSpace(
    _In_ ULONG Processor,
    _In_ ULONG64 BaseAddress,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_ PULONG ActualLength);

NTSTATUS
NTAPI
KdpSysWriteControlSpace(
    _In_ ULONG Processor,
    _In_ ULONG64 BaseAddress,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_ PULONG ActualLength);

//
// I/O Space
//
NTSTATUS
NTAPI
KdpSysReadIoSpace(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber,
    _In_ ULONG AddressSpace,
    _In_ ULONG64 IoAddress,
    _Out_writes_bytes_(DataSize) PVOID DataValue,
    _In_ ULONG DataSize,
    _Out_ PULONG ActualDataSize);

NTSTATUS
NTAPI
KdpSysWriteIoSpace(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber,
    _In_ ULONG AddressSpace,
    _In_ ULONG64 IoAddress,
    _In_reads_bytes_(DataSize) PVOID DataValue,
    _In_ ULONG DataSize,
    _Out_ PULONG ActualDataSize);

//
// Low Memory
//
NTSTATUS
NTAPI
KdpSysCheckLowMemory(
    IN ULONG Flags
);

//
// Internal routine for sending strings directly to the debugger
//
VOID
__cdecl
KdpDprintf(
    _In_ PCSTR Format,
    ...);

BOOLEAN
NTAPI
KdpPrintString(
    _In_ PSTRING Output);

VOID
NTAPI
KdLogDbgPrint(
    _In_ PSTRING String);

//
// Global KD Data
//
extern DBGKD_GET_VERSION64 KdVersionBlock;
extern KDDEBUGGER_DATA64 KdDebuggerDataBlock;
extern LIST_ENTRY KdpDebuggerDataListHead;
extern KSPIN_LOCK KdpDataSpinLock;
extern LARGE_INTEGER KdPerformanceCounterRate;
extern LARGE_INTEGER KdTimerStart;
extern ULONG KdDisableCount;
extern KD_CONTEXT KdpContext;
extern PKDEBUG_ROUTINE KiDebugRoutine;
extern BOOLEAN KdBreakAfterSymbolLoad;
extern BOOLEAN KdPitchDebugger;
extern BOOLEAN KdAutoEnableOnEvent;
extern BOOLEAN KdBlockEnable;
extern BOOLEAN KdIgnoreUmExceptions;
extern BOOLEAN KdPreviouslyEnabled;
extern BOOLEAN KdpDebuggerStructuresInitialized;
extern BOOLEAN KdEnteredDebugger;
extern KDPC KdpTimeSlipDpc;
extern KTIMER KdpTimeSlipTimer;
extern WORK_QUEUE_ITEM KdpTimeSlipWorkItem;
extern LONG KdpTimeSlipPending;
extern PKEVENT KdpTimeSlipEvent;
extern KSPIN_LOCK KdpTimeSlipEventLock;
extern BOOLEAN KdpPortLocked;
extern BOOLEAN KdpControlCPressed;
extern BOOLEAN KdpContextSent;
extern KSPIN_LOCK KdpDebuggerLock;
extern LARGE_INTEGER KdTimerStop, KdTimerStart, KdTimerDifference;

extern CHAR KdpMessageBuffer[KDP_MSG_BUFFER_SIZE];
extern CHAR KdpPathBuffer[KDP_MSG_BUFFER_SIZE];

extern CHAR KdPrintDefaultCircularBuffer[KD_DEFAULT_LOG_BUFFER_SIZE];
extern PCHAR KdPrintWritePointer;
extern ULONG KdPrintRolloverCount;
extern PCHAR KdPrintCircularBuffer;
extern ULONG KdPrintBufferSize;
extern ULONG KdPrintBufferChanges;
extern KSPIN_LOCK KdpPrintSpinLock;

extern BREAKPOINT_ENTRY KdpBreakpointTable[KD_BREAKPOINT_MAX];
extern KD_BREAKPOINT_TYPE KdpBreakpointInstruction;
extern BOOLEAN KdpOweBreakpoint;
extern BOOLEAN BreakpointsSuspended;
extern ULONG KdpNumInternalBreakpoints;
extern ULONG_PTR KdpCurrentSymbolStart, KdpCurrentSymbolEnd;
extern ULONG TraceDataBuffer[40];
extern ULONG TraceDataBufferPosition;

//
// Debug Filter Component Table
//
#define MAX_KD_COMPONENT_TABLE_ENTRIES  (DPFLTR_ENDOFTABLE_ID + 1)
extern ULONG KdComponentTableSize;
extern PULONG KdComponentTable[MAX_KD_COMPONENT_TABLE_ENTRIES];

//
// Debug Filter Masks
//
extern ULONG Kd_WIN2000_Mask;
extern ULONG Kd_SYSTEM_Mask;
extern ULONG Kd_SMSS_Mask;
extern ULONG Kd_SETUP_Mask;
extern ULONG Kd_NTFS_Mask;
extern ULONG Kd_FSTUB_Mask;
extern ULONG Kd_CRASHDUMP_Mask;
extern ULONG Kd_CDAUDIO_Mask;
extern ULONG Kd_CDROM_Mask;
extern ULONG Kd_CLASSPNP_Mask;
extern ULONG Kd_DISK_Mask;
extern ULONG Kd_REDBOOK_Mask;
extern ULONG Kd_STORPROP_Mask;
extern ULONG Kd_SCSIPORT_Mask;
extern ULONG Kd_SCSIMINIPORT_Mask;
extern ULONG Kd_CONFIG_Mask;
extern ULONG Kd_I8042PRT_Mask;
extern ULONG Kd_SERMOUSE_Mask;
extern ULONG Kd_LSERMOUS_Mask;
extern ULONG Kd_KBDHID_Mask;
extern ULONG Kd_MOUHID_Mask;
extern ULONG Kd_KBDCLASS_Mask;
extern ULONG Kd_MOUCLASS_Mask;
extern ULONG Kd_TWOTRACK_Mask;
extern ULONG Kd_WMILIB_Mask;
extern ULONG Kd_ACPI_Mask;
extern ULONG Kd_AMLI_Mask;
extern ULONG Kd_HALIA64_Mask;
extern ULONG Kd_VIDEO_Mask;
extern ULONG Kd_SVCHOST_Mask;
extern ULONG Kd_VIDEOPRT_Mask;
extern ULONG Kd_TCPIP_Mask;
extern ULONG Kd_DMSYNTH_Mask;
extern ULONG Kd_NTOSPNP_Mask;
extern ULONG Kd_FASTFAT_Mask;
extern ULONG Kd_SAMSS_Mask;
extern ULONG Kd_PNPMGR_Mask;
extern ULONG Kd_NETAPI_Mask;
extern ULONG Kd_SCSERVER_Mask;
extern ULONG Kd_SCCLIENT_Mask;
extern ULONG Kd_SERIAL_Mask;
extern ULONG Kd_SERENUM_Mask;
extern ULONG Kd_UHCD_Mask;
extern ULONG Kd_RPCPROXY_Mask;
extern ULONG Kd_AUTOCHK_Mask;
extern ULONG Kd_DCOMSS_Mask;
extern ULONG Kd_UNIMODEM_Mask;
extern ULONG Kd_SIS_Mask;
extern ULONG Kd_FLTMGR_Mask;
extern ULONG Kd_WMICORE_Mask;
extern ULONG Kd_BURNENG_Mask;
extern ULONG Kd_IMAPI_Mask;
extern ULONG Kd_SXS_Mask;
extern ULONG Kd_FUSION_Mask;
extern ULONG Kd_IDLETASK_Mask;
extern ULONG Kd_SOFTPCI_Mask;
extern ULONG Kd_TAPE_Mask;
extern ULONG Kd_MCHGR_Mask;
extern ULONG Kd_IDEP_Mask;
extern ULONG Kd_PCIIDE_Mask;
extern ULONG Kd_FLOPPY_Mask;
extern ULONG Kd_FDC_Mask;
extern ULONG Kd_TERMSRV_Mask;
extern ULONG Kd_W32TIME_Mask;
extern ULONG Kd_PREFETCHER_Mask;
extern ULONG Kd_RSFILTER_Mask;
extern ULONG Kd_FCPORT_Mask;
extern ULONG Kd_PCI_Mask;
extern ULONG Kd_DMIO_Mask;
extern ULONG Kd_DMCONFIG_Mask;
extern ULONG Kd_DMADMIN_Mask;
extern ULONG Kd_WSOCKTRANSPORT_Mask;
extern ULONG Kd_VSS_Mask;
extern ULONG Kd_PNPMEM_Mask;
extern ULONG Kd_PROCESSOR_Mask;
extern ULONG Kd_DMSERVER_Mask;
extern ULONG Kd_SR_Mask;
extern ULONG Kd_INFINIBAND_Mask;
extern ULONG Kd_IHVDRIVER_Mask;
extern ULONG Kd_IHVVIDEO_Mask;
extern ULONG Kd_IHVAUDIO_Mask;
extern ULONG Kd_IHVNETWORK_Mask;
extern ULONG Kd_IHVSTREAMING_Mask;
extern ULONG Kd_IHVBUS_Mask;
extern ULONG Kd_HPS_Mask;
extern ULONG Kd_RTLTHREADPOOL_Mask;
extern ULONG Kd_LDR_Mask;
extern ULONG Kd_TCPIP6_Mask;
extern ULONG Kd_ISAPNP_Mask;
extern ULONG Kd_SHPC_Mask;
extern ULONG Kd_STORPORT_Mask;
extern ULONG Kd_STORMINIPORT_Mask;
extern ULONG Kd_PRINTSPOOLER_Mask;
extern ULONG Kd_VSSDYNDISK_Mask;
extern ULONG Kd_VERIFIER_Mask;
extern ULONG Kd_VDS_Mask;
extern ULONG Kd_VDSBAS_Mask;
extern ULONG Kd_VDSDYN_Mask;   // Specified in Vista+
extern ULONG Kd_VDSDYNDR_Mask;
extern ULONG Kd_VDSLDR_Mask;   // Specified in Vista+
extern ULONG Kd_VDSUTIL_Mask;
extern ULONG Kd_DFRGIFC_Mask;
extern ULONG Kd_DEFAULT_Mask;
extern ULONG Kd_MM_Mask;
extern ULONG Kd_DFSC_Mask;
extern ULONG Kd_WOW64_Mask;
//
// Components specified in Vista+, some of which we also use in ReactOS
//
extern ULONG Kd_ALPC_Mask;
extern ULONG Kd_WDI_Mask;
extern ULONG Kd_PERFLIB_Mask;
extern ULONG Kd_KTM_Mask;
extern ULONG Kd_IOSTRESS_Mask;
extern ULONG Kd_HEAP_Mask;
extern ULONG Kd_WHEA_Mask;
extern ULONG Kd_USERGDI_Mask;
extern ULONG Kd_MMCSS_Mask;
extern ULONG Kd_TPM_Mask;
extern ULONG Kd_THREADORDER_Mask;
extern ULONG Kd_ENVIRON_Mask;
extern ULONG Kd_EMS_Mask;
extern ULONG Kd_WDT_Mask;
extern ULONG Kd_FVEVOL_Mask;
extern ULONG Kd_NDIS_Mask;
extern ULONG Kd_NVCTRACE_Mask;
extern ULONG Kd_LUAFV_Mask;
extern ULONG Kd_APPCOMPAT_Mask;
extern ULONG Kd_USBSTOR_Mask;
extern ULONG Kd_SBP2PORT_Mask;
extern ULONG Kd_COVERAGE_Mask;
extern ULONG Kd_CACHEMGR_Mask;
extern ULONG Kd_MOUNTMGR_Mask;
extern ULONG Kd_CFR_Mask;
extern ULONG Kd_TXF_Mask;
extern ULONG Kd_KSECDD_Mask;
extern ULONG Kd_FLTREGRESS_Mask;
extern ULONG Kd_MPIO_Mask;
extern ULONG Kd_MSDSM_Mask;
extern ULONG Kd_UDFS_Mask;
extern ULONG Kd_PSHED_Mask;
extern ULONG Kd_STORVSP_Mask;
extern ULONG Kd_LSASS_Mask;
extern ULONG Kd_SSPICLI_Mask;
extern ULONG Kd_CNG_Mask;
extern ULONG Kd_EXFAT_Mask;
extern ULONG Kd_FILETRACE_Mask;
extern ULONG Kd_XSAVE_Mask;
extern ULONG Kd_SE_Mask;
extern ULONG Kd_DRIVEEXTENDER_Mask;
//
// Components specified in Windows 8
//
extern ULONG Kd_POWER_Mask;
extern ULONG Kd_CRASHDUMPXHCI_Mask;
extern ULONG Kd_GPIO_Mask;
extern ULONG Kd_REFS_Mask;
extern ULONG Kd_WER_Mask;
//
// Components specified in Windows 10
//
extern ULONG Kd_CAPIMG_Mask;
extern ULONG Kd_VPCI_Mask;
extern ULONG Kd_STORAGECLASSMEMORY_Mask;
extern ULONG Kd_FSLIB_Mask;
