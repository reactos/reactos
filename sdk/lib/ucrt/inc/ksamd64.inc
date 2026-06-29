
include kxamd64.inc

;
;  Pointer size in bytes
;

SizeofPointer equ 00008H

;
;  Process State Enumerated Type Values
;

ProcessInMemory equ 00000H
ProcessOutOfMemory equ 00001H
ProcessInTransition equ 00002H

;
;  Thread State Enumerated Type Values
;

Initialized equ 00000H
Ready equ 00001H
Running equ 00002H
Standby equ 00003H
Terminated equ 00004H
Waiting equ 00005H

;
;  Processor Mode Definitions
;

KernelMode equ 00000H
UserMode equ 00001H

;
;  Wait Reason and Wait Type Enumerated Type Values
;

WrExecutive equ 00000H
WrDispatchInt equ 0001FH
WaitAny equ 00001H
WaitAll equ 00000H

;
;  Apc State Structure Offset Definitions
;

AsApcListHead equ 00000H
AsProcess equ 00020H
AsKernelApcPending equ 00029H
AsUserApcPendingAll equ 0002AH
KAPC_STATE_ANY_USER_APC_PENDING_MASK equ 00003H


;
;  Apc Record Structure Offset Definitions
;

ArNormalRoutine equ 00000H
ArNormalContext equ 00008H
ArSystemArgument1 equ 00010H
ArSystemArgument2 equ 00018H
ArFlags equ 00020H
KAPC_RECORD_LENGTH equ 00030H
KAPC_RECORD_FLAGS_CHECK_ALERT equ 00001H


;
;  Apc Callback Data Structure Offset Definitions
;

AcdContextRecord equ 00008H
KAPC_CALLBACK_DATA_LENGTH equ 00020H

;
;  Bug Check Code Definitions
;

APC_INDEX_MISMATCH equ 00001H
KERNEL_APC_PENDING_DURING_EXIT equ 00020H
ATTEMPTED_SWITCH_FROM_DPC equ 000B8H
DATA_BUS_ERROR equ 0002EH
DATA_COHERENCY_EXCEPTION equ 00055H
HAL1_INITIALIZATION_FAILED equ 00061H
INSTRUCTION_BUS_ERROR equ 0002FH
INSTRUCTION_COHERENCY_EXCEPTION equ 00056H
INTERRUPT_EXCEPTION_NOT_HANDLED equ 0003DH
INTERRUPT_UNWIND_ATTEMPTED equ 0003CH
INVALID_AFFINITY_SET equ 00003H
INVALID_DATA_ACCESS_TRAP equ 00004H
IRQL_GT_ZERO_AT_SYSTEM_SERVICE equ 0004AH
IRQL_NOT_LESS_OR_EQUAL equ 0000AH
KMODE_EXCEPTION_NOT_HANDLED equ 0001EH
NMI_HARDWARE_FAILURE equ 00080H
NO_USER_MODE_CONTEXT equ 0000EH
PAGE_FAULT_WITH_INTERRUPTS_OFF equ 00049H
PANIC_STACK_SWITCH equ 0002BH
SPIN_LOCK_INIT_FAILURE equ 00081H
SYSTEM_EXIT_OWNED_MUTEX equ 00039H
SYSTEM_SERVICE_EXCEPTION equ 0003BH
SYSTEM_UNWIND_PREVIOUS_USER equ 0003AH
TRAP_CAUSE_UNKNOWN equ 00012H
UNEXPECTED_KERNEL_MODE_TRAP equ 0007FH
HARDWARE_INTERRUPT_STORM equ 000F2H
RECURSIVE_MACHINE_CHECK equ 000FBH
RECURSIVE_NMI equ 00111H
KERNEL_SECURITY_CHECK_FAILURE equ 00139H
UNSUPPORTED_INSTRUCTION_MODE equ 00151H
BUGCHECK_CONTEXT_MODIFIER equ 080000000H
INVALID_CALLBACK_STACK_ADDRESS equ 001CDH
INVALID_KERNEL_STACK_ADDRESS equ 001CEH

;
;  Breakpoint type definitions
;

DBG_STATUS_CONTROL_C equ 00001H
STATUS_ASSERTION_FAILURE equ 0C0000420H

;
;  Client Id Structure Offset Definitions
;

CidUniqueProcess equ 00000H
CidUniqueThread equ 00008H

;
;  Exception Record Offset, Flag, and Enumerated Type Definitions
;

EXCEPTION_NONCONTINUABLE equ 00001H
EXCEPTION_UNWINDING equ 00002H
EXCEPTION_EXIT_UNWIND equ 00004H
EXCEPTION_STACK_INVALID equ 00008H
EXCEPTION_NESTED_CALL equ 00010H
EXCEPTION_TARGET_UNWIND equ 00020H
EXCEPTION_COLLIDED_UNWIND equ 00040H
EXCEPTION_UNWIND equ 00066H
EXCEPTION_EXECUTE_HANDLER equ 00001H
EXCEPTION_CONTINUE_SEARCH equ 00000H
EXCEPTION_CONTINUE_EXECUTION equ 0FFFFFFFFH

ExceptionContinueExecution equ 00000H
ExceptionContinueSearch equ 00001H
ExceptionNestedException equ 00002H
ExceptionCollidedUnwind equ 00003H

ErExceptionCode equ 00000H
ErExceptionFlags equ 00004H
ErExceptionRecord equ 00008H
ErExceptionAddress equ 00010H
ErNumberParameters equ 00018H
ErExceptionInformation equ 00020H
ExceptionRecordLength equ 000A0H

;
;  Fast Mutex Structure Offset Definitions
;

FmCount equ 00000H
FmOwner equ 00008H
FmContention equ 00010H
FmOldIrql equ 00030H

;
;  Interrupt Priority Request Level Definitions
;

PASSIVE_LEVEL equ 00000H
APC_LEVEL equ 00001H
DISPATCH_LEVEL equ 00002H
CLOCK_LEVEL equ 0000DH
IPI_LEVEL equ 0000EH
POWER_LEVEL equ 0000EH
PROFILE_LEVEL equ 0000FH
HIGH_LEVEL equ 0000FH
ifdef NT_UP
SYNCH_LEVEL equ 00002H
else
SYNCH_LEVEL equ 0000CH
endif

;
;  Large Integer Structure Offset Definitions
;

LiLowPart equ 00000H
LiHighPart equ 00004H

;
;  List Entry Structure Offset Definitions
;

LsFlink equ 00000H
LsBlink equ 00008H

;
;  String Structure Offset Definitions
;

StrLength equ 00000H
StrMaximumLength equ 00002H
StrBuffer equ 00008H

;
;  System DLL Init Block Structure Offset Definitions
;

IbCfgBitMap equ 000B8H
IbWow64CfgBitMap equ 000C8H
IbMitigationOptionsMap equ 000A0H
PS_MITIGATION_OPTION_BITS_PER_OPTION equ 00004H
PS_MITIGATION_OPTION_ALWAYS_ON equ 00001H
PS_MITIGATION_OPTION_ALWAYS_OFF equ 00002H
PS_MITIGATION_OPTION_MASK equ 00003H
PS_MITIGATION_OPTION_RETURN_FLOW_GUARD equ 00010H
PS_MITIGATION_OPTION_RESTRICT_SET_THREAD_CONTEXT equ 00013H
PS_MITIGATION_OPTION_CET_USER_SHADOW_STACKS equ 0001FH
PsneKiUserCallbackDispatcher equ 00030H

;
;  User Shared Data Structure Offset Definitions
;

UsTickCountMultiplier equ 00004H
UsInterruptTime equ 00008H
UsSystemTime equ 00014H
UsTimeZoneBias equ 00020H
UsImageNumberLow equ 0002CH
UsImageNumberHigh equ 0002EH
UsNtSystemRoot equ 00030H
UsMaxStackTraceDepth equ 00238H
UsCryptoExponent equ 0023CH
UsTimeZoneId equ 00240H
UsLargePageMinimum equ 00244H
UsNtBuildNumber equ 00260H
UsNtProductType equ 00264H
UsProductTypeIsValid equ 00268H
UsNtMajorVersion equ 0026CH
UsNtMinorVersion equ 00270H
UsProcessorFeatures equ 00274H
UsReserved1 equ 002B4H
UsReserved3 equ 002B8H
UsTimeSlip equ 002BCH
UsAlternativeArchitecture equ 002C0H
UsSystemExpirationDate equ 002C8H
UsSuiteMask equ 002D0H
UsKdDebuggerEnabled equ 002D4H
UsActiveConsoleId equ 002D8H
UsDismountCount equ 002DCH
UsComPlusPackage equ 002E0H
UsLastSystemRITEventTickCount equ 002E4H
UsNumberOfPhysicalPages equ 002E8H
UsSafeBootMode equ 002ECH
UsTestRetInstruction equ 002F8H
UsSystemCallPad equ 00310H
UsTickCount equ 00320H
UsTickCountQuad equ 00320H
UsXState equ 003D8H

;
;  Time Structure Offset Definitions
;

TmLowTime equ 00000H
TmHighTime equ 00004H

;
;  Thread Switch Counter Offset Definitions
;

TwFindAny equ 00000H
TwFindIdeal equ 00004H
TwFindLast equ 00008H
TwIdleAny equ 0000CH
TwIdleCurrent equ 00010H
TwIdleIdeal equ 00014H
TwIdleLast equ 00018H
TwPreemptAny equ 0001CH
TwPreemptCurrent equ 00020H
TwPreemptLast equ 00024H
TwSwitchToIdle equ 00028H

;
;  Status Code Definitions
;

STATUS_ACCESS_VIOLATION equ 0C0000005H
STATUS_ARRAY_BOUNDS_EXCEEDED equ 0C000008CH
STATUS_BAD_COMPRESSION_BUFFER equ 0C0000242H
STATUS_BREAKPOINT equ 080000003H
STATUS_CALLBACK_POP_STACK equ 0C0000423H
STATUS_DATATYPE_MISALIGNMENT equ 080000002H
STATUS_FLOAT_DENORMAL_OPERAND equ 0C000008DH
STATUS_FLOAT_DIVIDE_BY_ZERO equ 0C000008EH
STATUS_FLOAT_INEXACT_RESULT equ 0C000008FH
STATUS_FLOAT_INVALID_OPERATION equ 0C0000090H
STATUS_FLOAT_OVERFLOW equ 0C0000091H
STATUS_FLOAT_STACK_CHECK equ 0C0000092H
STATUS_FLOAT_UNDERFLOW equ 0C0000093H
STATUS_FLOAT_MULTIPLE_FAULTS equ 0C00002B4H
STATUS_FLOAT_MULTIPLE_TRAPS equ 0C00002B5H
STATUS_GUARD_PAGE_VIOLATION equ 080000001H
STATUS_ILLEGAL_FLOAT_CONTEXT equ 0C000014AH
STATUS_ILLEGAL_INSTRUCTION equ 0C000001DH
STATUS_INSTRUCTION_MISALIGNMENT equ 0C00000AAH
STATUS_INVALID_HANDLE equ 0C0000008H
STATUS_INVALID_LOCK_SEQUENCE equ 0C000001EH
STATUS_INVALID_OWNER equ 0C000005AH
STATUS_INVALID_PARAMETER equ 0C000000DH
STATUS_INVALID_PARAMETER_1 equ 0C00000EFH
STATUS_INVALID_SYSTEM_SERVICE equ 0C000001CH
STATUS_INVALID_THREAD equ 0C000071CH
STATUS_INTEGER_DIVIDE_BY_ZERO equ 0C0000094H
STATUS_INTEGER_OVERFLOW equ 0C0000095H
STATUS_IN_PAGE_ERROR equ 0C0000006H
STATUS_KERNEL_APC equ 00100H
STATUS_LONGJUMP equ 080000026H
STATUS_NO_CALLBACK_ACTIVE equ 0C0000258H
STATUS_PRIVILEGED_INSTRUCTION equ 0C0000096H
STATUS_SINGLE_STEP equ 080000004H
STATUS_STACK_BUFFER_OVERRUN equ 0C0000409H
STATUS_STACK_OVERFLOW equ 0C00000FDH
STATUS_SUCCESS equ 00000H
STATUS_THREAD_IS_TERMINATING equ 0C000004BH
STATUS_TIMEOUT equ 00102H
STATUS_UNWIND equ 0C0000027H
STATUS_UNWIND_CONSOLIDATE equ 080000029H
STATUS_USER_APC equ 000C0H
STATUS_WAKE_SYSTEM equ 040000294H
STATUS_WAKE_SYSTEM_DEBUGGER equ 080000007H
STATUS_SET_CONTEXT_DENIED equ 0C000060AH
STATUS_RETURN_ADDRESS_HIJACK_ATTEMPT equ 080000033H

;
;  Fast Fail Code Definitions
;

FAST_FAIL_INVALID_BUFFER_ACCESS equ 0001CH
FAST_FAIL_INVALID_NEXT_THREAD equ 0001EH
FAST_FAIL_INVALID_CONTROL_STACK equ 0002FH
FAST_FAIL_SET_CONTEXT_DENIED equ 00030H
FAST_FAIL_ENCLAVE_CALL_FAILURE equ 00035H
FAST_FAIL_FLAGS_CORRUPTION equ 0003BH

;
;  Bounds Callback Status Code Definitions
;

BoundExceptionContinueSearch equ 00000H
BoundExceptionHandled equ 00001H
BoundExceptionError equ 00002H

;
;  APC Object Structure Offset Definitions
;

ApType equ 00000H
ApSize equ 00002H
ApThread equ 00008H
ApApcListEntry equ 00010H
ApKernelRoutine equ 00020H
ApRundownRoutine equ 00028H
ApNormalRoutine equ 00030H
ApNormalContext equ 00038H
ApSystemArgument1 equ 00040H
ApSystemArgument2 equ 00048H
ApApcStateIndex equ 00050H
ApApcMode equ 00051H
ApInserted equ 00052H
ApcObjectLength equ 00058H

;
;  DPC object Structure Offset Definitions
;

DpType equ 00000H
DpNumber equ 00002H
DpImportance equ 00001H
DpDpcListEntry equ 00008H
DpDeferredRoutine equ 00018H
DpDeferredContext equ 00020H
DpSystemArgument1 equ 00028H
DpSystemArgument2 equ 00030H
DpDpcData equ 00038H
DpcObjectLength equ 00040H

KI_DPC_ALL_FLAGS equ 000BFH
KI_DPC_ANY_DPC_ACTIVE equ 010001H

;
;  Device Queue Object Structure Offset Definitions
;

DvType equ 00000H
DvSize equ 00002H
DvDeviceListHead equ 00008H
DvSpinLock equ 00018H
DvBusy equ 00020H
DeviceQueueObjectLength equ 00028H

;
;  Device Queue Entry Structure Offset Definitions
;

DeDeviceListEntry equ 00000H
DeSortKey equ 00010H
DeInserted equ 00014H
DeviceQueueEntryLength equ 00018H

;
;  Event Object Structure Offset Definitions
;

EvType equ 00000H
EvSize equ 00002H
EvSignalState equ 00004H
EvWaitListHead equ 00008H
EventObjectLength equ 00018H

;
;  Interrupt Object Structure Offset Definitions
;

KINTERRUPT_STATE_DISABLED_BIT equ 00000H
KINTERRUPT_STATE_DISABLED equ 00001H
InLevelSensitive equ 00000H
InLatched equ 00001H

InType equ 00000H
InSize equ 00002H
InInterruptListEntry equ 00008H
InServiceRoutine equ 00018H
InServiceContext equ 00030H
InSpinLock equ 00038H
InTickCount equ 00040H
InActualLock equ 00048H
InDispatchAddress equ 00050H
InVector equ 00058H
InIrql equ 0005CH
InSynchronizeIrql equ 0005DH
InFloatingSave equ 0005EH
InConnected equ 0005FH
InNumber equ 00060H
InShareVector equ 00064H
InInternalState equ 00068H
InMode equ 0006CH
InServiceCount equ 00074H
InDispatchCount equ 00078H
InTrapFrame equ 00088H
InterruptObjectLength equ 00120H

;
;  Process Object Structure Offset Definitions
;

PrType equ 00000H
PrSize equ 00002H
PrSignalState equ 00004H
PrProfileListHead equ 00018H
PrDirectoryTableBase equ 00028H
PrThreadListHead equ 00030H
PrAffinity equ 00050H
PrReadyListHead equ 00158H
PrSwapListEntry equ 00168H
PrActiveProcessors equ 00170H
PrProcessFlags equ 00278H
PrBasePriority equ 00280H
PrQuantumReset equ 00281H
PrCycleTime equ 00360H
PrKernelTime equ 0037CH
PrUserTime equ 00380H
PrInstrumentationCallback equ 003D8H
PrMitigationFlags2 equ 009D4H
KernelProcessObjectLength equ 00438H
ExecutiveProcessObjectLength equ 00B80H
Win32BatchFlushCallout equ 00007H

;
;  Profile Object Structure Offset Definitions
;

PfType equ 00000H
PfSize equ 00002H
PfProfileListEntry equ 00008H
PfProcess equ 00018H
PfRangeBase equ 00020H
PfRangeLimit equ 00028H
PfBucketShift equ 00030H
PfBuffer equ 00038H
PfSegment equ 00040H
PfAffinity equ 00048H
PfSource equ 00260H
PfStarted equ 00262H
ProfileObjectLength equ 00268H

;
;  Queue Object Structure Offset Definitions
;

QuSignalState equ 00004H
QuEntryListHead equ 00018H
QuCurrentCount equ 00028H
QuMaximumCount equ 0002CH
QuThreadListHead equ 00030H
QueueObjectLength equ 00040H

;
;  Thread Object Structure Offset Definitions
;

EtCid equ 004C8H
EtPicoContext equ 00630H

ThType equ 00000H
ThSize equ 00002H
ThLock equ 00000H
THREAD_FLAGS_CYCLE_PROFILING equ 00001H
THREAD_FLAGS_CYCLE_PROFILING_LOCK_BIT equ 00010H
THREAD_FLAGS_CYCLE_PROFILING_LOCK equ 010000H
THREAD_FLAGS_COUNTER_PROFILING equ 00002H
THREAD_FLAGS_COUNTER_PROFILING_LOCK_BIT equ 00011H
THREAD_FLAGS_COUNTER_PROFILING_LOCK equ 020000H
THREAD_FLAGS_GROUP_SCHEDULING equ 00004H
THREAD_FLAGS_AFFINITY_SET equ 00008H
THREAD_FLAGS_ACCOUNTING_CSWITCH equ 000B6H
THREAD_FLAGS_ACCOUNTING_INTERRUPT equ 00072H
THREAD_FLAGS_ACCOUNTING_ANY equ 000BEH
ThDebugActive equ 00003H
ThThreadControlFlags equ 00002H
ThSignalState equ 00004H
ThCycleTime equ 00048H
ThCurrentRunTime equ 00050H
ThStateSaveArea equ 00060H
ThInitialStack equ 00028H
ThStackLimit equ 00030H
ThKernelStack equ 00058H
ThThreadLock equ 00040H
ThApcState equ 00098H
ThProcess equ 00220H
ThNextProcessor equ 00218H
ThAdjustReason equ 00236H
ThAdjustIncrement equ 00237H
ThWaitReason equ 00283H
ThRunning equ 00071H
ThAlerted equ 00072H
ThWaitTime equ 001B4H
ThCombinedApcDisable equ 001E4H
ThKernelApcDisable equ 001E4H
ThSpecialApcDisable equ 001E6H
ThTeb equ 000F0H
ThTimer equ 00100H
thProcess equ 00220H

KTHREAD_AUTO_ALIGNMENT_BIT equ 00002H
KTHREAD_GUI_THREAD_MASK equ 00080H
KTHREAD_RESTRICTED_GUI_THREAD_MASK equ 0200000H
KTHREAD_SYSTEM_THREAD_BIT equ 0000AH
KTHREAD_QUEUE_DEFER_PREEMPTION_BIT equ 0000BH
KTHREAD_BAM_QOS_LEVEL_MASK equ 000FFH
KTHREAD_CET_USER_SHADOW_STACK_BIT equ 00014H
KTHREAD_CET_KERNEL_SHADOW_STACK_BIT equ 00016H

DEBUG_ACTIVE_ALT_SYSCALL_HANDLER equ 00024H
PsSystemCallMapToSystem equ 00001H

ThMiscFlags equ 00074H
ThThreadFlags equ 00078H
ThPreviousMode equ 00232H
ThContextSwitches equ 00154H
ThState equ 00184H
ThWaitIrql equ 00186H
ThTrapFrame equ 00090H
ThFirstArgument equ 00088H
ThApcStateIndex equ 0024AH
ThSystemCallNumber equ 00080H
ThWin32Thread equ 001C8H
ThStackBase equ 00038H
ThLegoData equ 002D0H
KernelThreadObjectLength equ 00480H
ExecutiveThreadObjectLength equ 00910H


;
;  Timer object Structure Offset Definitions
;

TiType equ 00000H
TiSize equ 00002H
TiSignalState equ 00004H
TiDueTime equ 00018H
TiTimerListEntry equ 00020H
TiDpc equ 00030H
TiPeriod equ 0003CH
TimerObjectLength equ 00040H


;
;  Wait Block Structure Offset Definitions
;

WbWaitListEntry equ 00000H
WbThread equ 00018H
WbObject equ 00020H
WbWaitKey equ 00012H
WbWaitType equ 00010H

;
;  Fiber Structure Offset Definitions
;

FbFiberData equ 00000H
FbExceptionList equ 00008H
FbStackBase equ 00010H
FbStackLimit equ 00018H
FbDeallocationStack equ 00020H
FbFiberContext equ 00030H
FbWx86Tib equ 00500H
FbActivationContextStackPointer equ 00508H
FbFlsData equ 00510H
FbGuaranteedStackBytes equ 00518H
FbTebFlags equ 0051CH
FbSsp equ 00528H

;
;  Process Environment Block Structure Offset Definitions
;

PeBeingDebugged equ 00002H
PeProcessParameters equ 00020H
PeKernelCallbackTable equ 00058H
ProcessEnvironmentBlockLength equ 007D0H

;
;  Process Parameters Block Structure Offset Definitions
;

PpFlags equ 00008H
USER_CALLBACK_FILTER equ 080000H

;
;  System Service Descriptor Table Structure Definitions
;

NUMBER_SERVICE_TABLES equ 00002H
SERVICE_NUMBER_MASK equ 00FFFH
SERVICE_TABLE_SHIFT equ 00007H
SERVICE_TABLE_MASK equ 00020H
SERVICE_TABLE_TEST equ 00020H

ThBase equ 00000H
ThLimit equ 00008H
SdBase equ 00000H
SdLimit equ 00010H
SdNumber equ 00018H
SdLength equ 00020H

;
;  Thread Environment Block Structure Offset Definitions
;

TeCmTeb equ 00000H
TeStackBase equ 00008H
TeStackLimit equ 00010H
TeFiberData equ 00020H
TeSelf equ 00030H
TeEnvironmentPointer equ 00038H
TeClientId equ 00040H
TeActiveRpcHandle equ 00050H
TeThreadLocalStoragePointer equ 00058H
TeCountOfOwnedCriticalSections equ 0006CH
TePeb equ 00060H
TeCsrClientThread equ 00070H
TeWOW32Reserved equ 00100H
TeSoftFpcr equ 0010CH
TeExceptionCode equ 002C0H
TeActivationContextStackPointer equ 002C8H
TeInstrumentationCallbackSp equ 002D0H
TeInstrumentationCallbackPreviousPc equ 002D8H
TeInstrumentationCallbackPreviousSp equ 002E0H
TeGdiClientPID equ 007F0H
TeGdiClientTID equ 007F4H
TeGdiThreadLocalInfo equ 007F8H
TeglDispatchTable equ 009F0H
TeglReserved1 equ 01138H
TeglReserved2 equ 01220H
TeglSectionInfo equ 01228H
TeglSection equ 01230H
TeglTable equ 01238H
TeglCurrentRC equ 01240H
TeglContext equ 01248H
TeDeallocationStack equ 01478H
TeTlsSlots equ 01480H
TeTlsExpansionSlots equ 01780H
TeLastErrorValue equ 00068H
TeVdm equ 01690H
TeInstrumentation equ 016B8H
TeGdiBatchCount equ 01740H
TeGuaranteedStackBytes equ 01748H
TeFlsData equ 017C8H
ThreadEnvironmentBlockLength equ 01850H
CmThreadEnvironmentBlockOffset equ 02000H
TLS_MINIMUM_AVAILABLE equ 00040H
TLS_EXPANSION_SLOTS equ 00400H

;
;  Lock Queue Structure Offset Definitions
;

LOCK_QUEUE_WAIT equ 00001H
LOCK_QUEUE_OWNER equ 00002H
LOCK_QUEUE_HEADER_SIZE equ 00010H


LqNext equ 00000H
LqLock equ 00008H

LqhLockQueue equ 00000H
LqhOldIrql equ 00010H
LqhNext equ 00000H
LqhLock equ 00008H

;
;  Kernel Stack Control Structure Offset (relative to initial stack pointer) Definitions
;

KSTACK_CONTROL_LENGTH equ 00050H
KcPreviousBase equ 00010H
KcPreviousLimit equ 00018H
KcPreviousKernel equ 00020H
KcPreviousInitial equ 00028H
KcPreviousKernelShadowStack equ 00040H
KcPreviousKernelShadowStackInitial equ 00048H
KcPreviousKernelShadowStackBase equ 00030H
KcPreviousKernelShadowStackLimit equ 00038H
KcCurrentBase equ 00000H
KcActualLimit equ 00008H
KSTACK_ACTUAL_LIMIT_EXPANDED equ 00001H


;
;  ETW Definitions
;

PERF_CONTEXTSWAP_OFFSET equ 00004H
PERF_CONTEXTSWAP_FLAG equ 00004H
PERF_INTERRUPT equ 020004000H
PERF_INTERRUPT_OFFSET equ 00004H
PERF_INTERRUPT_FLAG equ 04000H
PERF_SYSCALL_OFFSET equ 00008H
PERF_SYSCALL_FLAG equ 00040H
PERF_SPEC_CONTROL_OFFSET equ 00014H
PERF_SPEC_CONTROL_FLAG equ 00002H
NTOS_YIELD_MACRO equ 00001H
EtwTSLength equ 00020H
CONTEXT_SWAP_TRACING equ 00001H
LAST_BRANCH_RECORD_IN_USE equ 00002H
PROCESSOR_TRACE_ON_EACH_CORE_ENABLED equ 00004H

;
;  Enlightenment structure definitions
;

HeEnlightenments equ 00000H
HeHypervisorConnected equ 00004H
HeEndOfInterrupt equ 00008H
HeApicWriteIcr equ 00010H
HeSpinCountMask equ 0001CH
HeLongSpinWait equ 00020H

;
;  XState context structure offset definitions
;

XctxMask equ 00000H
XctxLength equ 00008H
XctxArea equ 00010H

;
;  XState configuration structure offset definitions
;

XcfgEnabledFeatures equ 00000H
XcfgEnabledVolatileFeatures equ 00008H
XcfgEnabledSupervisorFeatures equ 00218H

;
;  GETSETCONTEXT structure offset definitions
;

GetSetCtxContextPtr equ 00078H
XSAVE_ALIGN equ 00040H

;
;  XSAVE_AREA structure offset definitions
;

XsaHeader equ 00200H
XsaHeaderLength equ 00040H
XSTATE_MASK_LEGACY_FLOATING_POINT equ 00001H
XSTATE_MASK_LEGACY_SSE equ 00002H
XSTATE_MASK_LEGACY equ 00003H
XSTATE_MASK_GSSE equ 00004H
XSTATE_MASK_IPT equ 00100H
XSTATE_MASK_LARGE_FEATURES equ 040000H

;
;  KERNEL_CET_CONTEXT structure offset definitions
;

KccSsp equ 00000H
KccRip equ 00008H
KccSegCs equ 00010H

;
;  Extended context structure offset definitions
;

CxxAllOffset equ 00000H
CxxAllLength equ 00004H
CxxLegacyOffset equ 00008H
CxxLegacyLength equ 0000CH
CxxXStateOffset equ 00010H
CxxXStateLength equ 00014H
CxxKernelCetOffset equ 00018H
CxxKernelCetLength equ 0001CH

;
;  KAFFINITY_EX offsets
;

AfCount equ 00000H
AfBitmap equ 00008H
CFlushSize equ 000E8H
PbEntropyCount equ 089E0H
PbEntropyBuffer equ 089E4H
KENTROPY_TIMING_INTERRUPTS_PER_BUFFER equ 00400H
KENTROPY_TIMING_BUFFER_MASK equ 007FFH
KENTROPY_TIMING_ANALYSIS equ 00000H

;
;  Priority state definitions
;

KPRIORITY_STATE_PRIORITY_BITMASK equ 0007FH

;
;  Register Argument Home Address Offset Definitions
;

P1Home equ 00008H
P2Home equ 00010H
P3Home equ 00018H
P4Home equ 00020H

;
;  Special Register Structure Offset Definition
;

SrKernelDr0 equ 00020H
SrKernelDr1 equ 00028H
SrKernelDr2 equ 00030H
SrKernelDr3 equ 00038H
SrKernelDr6 equ 00040H
SrKernelDr7 equ 00048H
SrGdtr equ 00050H
SrIdtr equ 00060H
SrTr equ 00070H
SrMxCsr equ 00074H
SrMsrGsBase equ 000A8H
SrMsrGsSwap equ 000B0H
SrMsrStar equ 000B8H
SrMsrLStar equ 000C0H
SrMsrCStar equ 000C8H
SrMsrSyscallMask equ 000D0H
SrXcr0 equ 000D8H
SrMsrFsBase equ 000E0H

;
;  Processor Control Region Structure Offset Definitions
;

PcGdt equ 00000H
PcTss equ 00008H
PcUserRsp equ 00010H
PcUserRspShadow equ 0A010H
PcSelf equ 00018H
PcCurrentPrcb equ 00020H
PcLockArray equ 00028H
PcTeb equ 00030H
PcIdt equ 00038H
PcDbgMceNestingLevel equ 0A02CH
PcDbgMceFlags equ 0A030H
PcProcessorDescriptorArea equ 00038H
PcIrql equ 00050H
PcStallScaleFactor equ 00064H
PcHalReserved equ 000C0H
PcPrcb equ 00180H
PcMxCsr equ 00180H
PcInterruptRequest equ 00186H
PcIdleHalt equ 00187H
PcCurrentThread equ 00188H
PcNextThread equ 00190H
PcIdleThread equ 00198H
PcIpiFrozen equ 02F08H
PcNestingLevel equ 001A0H
PcNumber equ 001A4H
PcRspBase equ 001A8H
PcRspBaseShadow equ 0A008H
PcShadowFlags equ 0A018H
PcVerwSelector equ 0A02AH
PcPrcbLock equ 001B0H
PcCr0 equ 00280H
PcCr2 equ 00288H
PcCr3 equ 00290H
PcCr4 equ 00298H
PcKernelDr0 equ 002A0H
PcKernelDr1 equ 002A8H
PcKernelDr2 equ 002B0H
PcKernelDr3 equ 002B8H
PcKernelDr7 equ 002C8H
PcGdtrLimit equ 002D6H
PcGdtrBase equ 002D8H
PcIdtrLimit equ 002E6H
PcIdtrBase equ 002E8H
PcTr equ 002F0H
PcLdtr equ 002F2H
PcDebugControl equ 002F8H
PcLastBranchToRip equ 00300H
PcLastBranchFromRip equ 00308H
PcLastExceptionToRip equ 00310H
PcLastExceptionFromRip equ 00318H
PcCr8 equ 00320H
PcCpuType equ 001C0H
PcCpuID equ 001C1H
PcCpuStep equ 001C2H
PcCpuVendor equ 0020DH
PcVirtualApicAssist equ 08888H
PcFeatureBits equ 08B20H
PcNmiActive equ 08026H
PcMceActive equ 08027H
PcCombinedNmiMceActive equ 08026H
PcSfCode equ 08DE8H
PcSfVa equ 08DF0H
PcSfPa equ 08DF8H
PcKernelDirectoryTableBase equ 0A000H
PcExceptionStack equ 083E8H
PcExceptionStackActive equ 083E6H
PcDpcRequestSummary equ 0353CH
PcTransitionShadowStack equ 095A0H
PcKernelShadowStackInitial equ 095A8H
;
;
CPU_AMD equ 00001H
CPU_INTEL equ 00002H
CPU_VIA equ 00003H
PcCFlushSize equ 00268H
PcDeepSleep equ 082DAH
PcDeferredReadyListHead equ 02E88H
PcSystemCalls equ 02EB8H
PcDpcRoutineActive equ 0353AH
PcInterruptCount equ 082C0H
PcDebuggerSavedIRQL equ 082D8H
PcStartCycles equ 08300H
RTL_RETPOLINE_STUBS_UNWIND_INFO_MAX_SIZE equ 001F4H
ProcessorControlRegisterLength equ 02C040H

;
;  Defines kernel process structure
;

PrUserDirectoryTableBase equ 00388H
PrAddressPolicy equ 00390H

;
;  Defines for user shared data
;

ifdef RELOCATABLE_USER_SHARED_DATA
MM_SHARED_USER_DATA_VA_NOT_RELOCATABLE equ 0000000007FFE0000H
else
USER_SHARED_DATA equ 0FFFFF78000000000H
MM_SHARED_USER_DATA_VA equ 0000000007FFE0000H
endif
PF_COMPARE_EXCHANGE128 equ 0000000000000000EH
PF_RDWRFSGSBASE_AVAILABLE equ 00000000000000016H
PF_RDTSCP_INSTRUCTION_AVAILABLE equ 00000000000000020H
PF_RDPID_INSTRUCTION_AVAILABLE equ 00000000000000021H
SYSTEM_CALL_INT_2E equ 00001H
UsTickCountMultiplier equ 00004H
UsInterruptTime equ 00008H
UsSystemTime equ 00014H
UsTestRetInstruction equ 002F8H
UsSystemCall equ 00308H
UsProcessorFeatures equ 00274H

;
;  Tss Structure Offset Definitions
;

TssRsp0 equ 00004H
TssRsp1 equ 0000CH
TssRsp2 equ 00014H
TssPanicStack equ 00024H
TssMcaStack equ 0002CH
TssNmiStack equ 00034H
TssIoMapBase equ 00066H
TssLength equ 00068H

;
;  Gdt Descriptor Offset Definitions
;

KGDT64_NULL equ 00000H
KGDT64_R0_CODE equ 00010H
KGDT64_R0_DATA equ 00018H
KGDT64_R3_CMCODE equ 00020H
KGDT64_R3_DATA equ 00028H
KGDT64_R3_CODE equ 00030H
KGDT64_SYS_TSS equ 00040H
KGDT64_R3_CMTEB equ 00050H

;
;  Constants for AMD64 compatibility mode switching
;

X86AMD64_R3_LONG_MODE_CODE equ 00033H

;
;  GDT Entry Offset Definitions
;

KgdtBaseLow equ 00002H
KgdtBaseMiddle equ 00004H
KgdtBaseHigh equ 00007H
KgdtBaseUpper equ 00008H
KgdtFlags1 equ 00005H
KgdtLimitHigh equ 00006H
KgdtLimitLow equ 00000H
KGDT_LIMIT_ENCODE_MASK equ 03C00H
KGDT_ENTRY_PRESENT equ 00080H


;
;  Processor Block Structure Offset Definitions
;

PbMxCsr equ 00000H
PbInterruptRequest equ 00006H
PbIdleHalt equ 00007H
PbCurrentThread equ 00008H
PbNextThread equ 00010H
PbIdleThread equ 00018H
PbSmtIsolationThread equ 08880H
PbNestingLevel equ 00020H
PbNumber equ 00024H
PbRspBase equ 00028H
PbRspBaseShadow equ 09E88H
PbPrcbLock equ 00030H
PbTscFrequency equ 00090H
PbKeSpinLockOrdering equ 07EB0H
KI_SPINLOCK_ORDER_PRCB_LOCK equ 00080H
PbGroup equ 000D0H
PbGroupIndex equ 000D1H
PbProcessorState equ 00100H
PbContext equ 089C0H
PbCpuType equ 00040H
PbCpuID equ 00041H
PbCpuStep equ 00042H
PbHalReserved equ 00048H
PbMinorVersion equ 00088H
PbMajorVersion equ 0008AH
PbBuildType equ 0008CH
PbCpuVendor equ 0008DH
PbCoresPerPhysicalProcessor equ 000A0H
PbLogicalProcessorsPerCore equ 000A4H
PbApicMask equ 000DCH
PbCFlushSize equ 000E8H
PbAcpiReserved equ 000E0H
PbInitialApicId equ 000D4H
PbPrcbFlags equ 000ECH
PbPriorityState equ 00038H
PbLockQueue equ 006F0H
PbPPLookasideList equ 00800H
PbPPNPagedLookasideList equ 01500H
PbPPPagedLookasideList equ 02100H
PbPacketBarrier equ 02D80H
PbDeferredReadyListHead equ 02D08H
PbLookasideIrpFloat equ 02D9CH
PbSystemCalls equ 02D38H
PbReadOperationCount equ 02D5CH
PbWriteOperationCount equ 02D60H
PbOtherOperationCount equ 02D64H
PbReadTransferCount equ 02D68H
PbWriteTransferCount equ 02D70H
PbOtherTransferCount equ 02D78H
PbContextSwitches equ 02D3CH
PbTargetCount equ 02D84H
PbIpiFrozen equ 02D88H
PbRequestMailbox equ 0BEC0H
PbMailbox equ 09340H
PbDpcList equ 03340H
PbDpcLock equ 03350H
PbDpcCount equ 0335CH
PbDpcStack equ 033A0H
PbIsrStack equ 089D8H
PbMaximumDpcQueueDepth equ 033A8H
PbDpcRequestRate equ 033ACH
PbMinimumDpcRate equ 033B0H
PbDpcRoutineActive equ 033BAH
PbDpcRequestSummary equ 033BCH
PbNormalDpcState equ 033BCH
PbDpcLastCount equ 033B4H
PbQuantumEnd equ 033B9H
PbIdleSchedule equ 033BBH
PbInterruptObject equ 03400H
PbReadySummary equ 07ED8H
PbDispatcherReadyListHead equ 07F40H
PbInterruptCount equ 08140H
PbKernelTime equ 08144H
PbUserTime equ 08148H
PbDpcTime equ 0814CH
PbInterruptTime equ 08150H
PbAdjustDpcThreshold equ 08154H
PbStartCycles equ 08180H
PbPageColor equ 08250H
PbNodeColor equ 08254H
PbNodeShiftedColor equ 08258H
PbSecondaryColorMask equ 0825CH
PbCycleTime equ 08278H
PbCpuCycleScalingFactor equ 081B8H
PPM_FRACTION_SHIFT equ 00010H
PbFastReadNoWait equ 02D44H
PbFastReadWait equ 02D48H
PbFastReadNotPossible equ 02D4CH
PbCopyReadNoWait equ 02D50H
PbCopyReadWait equ 02D54H
PbCopyReadNoWaitMiss equ 02D58H
PbAlignmentFixupCount equ 086B8H
PbExceptionDispatchCount equ 08170H
PbVendorString equ 08990H
ProcessorBlockLength equ 0BF00H
PbShadowFlags equ 09E98H
PbTrappedSecurityDomain equ 000F0H
PbKernelShadowStackInitial equ 09428H

;
;  Time Fields (TIME_FIELDS) Structure Offset Definitions
;

TfSecond equ 0000AH
TfMinute equ 00008H
TfHour equ 00006H
TfWeekday equ 0000EH
TfDay equ 00004H
TfMonth equ 00002H
TfYear equ 00000H
TfMilliseconds equ 0000CH


;
;  Define constants for system IRQL and IDT vector conversion
;

MAXIMUM_IDTVECTOR equ 000FFH
MAXIMUM_PRIMARY_VECTOR equ 000FFH
PRIMARY_VECTOR_BASE equ 00030H
RPL_MASK equ 00003H
MODE_BIT equ 00000H
MODE_MASK equ 00001H
LDT_MASK equ 00004H

;
;  Flags in the CR0 register
;

CR0_PG equ 080000000H
CR0_ET equ 00010H
CR0_TS equ 00008H
CR0_EM equ 00004H
CR0_MP equ 00002H
CR0_PE equ 00001H
CR0_CD equ 040000000H
CR0_NW equ 020000000H
CR0_AM equ 040000H
CR0_WP equ 010000H
CR0_NE equ 00020H

;
;  Flags in the CR4 register
;

CR4_VME equ 00001H
CR4_PVI equ 00002H
CR4_TSD equ 00004H
CR4_DE equ 00008H
CR4_PSE equ 00010H
CR4_PAE equ 00020H
CR4_MCE equ 00040H
CR4_PGE equ 00080H
CR4_FXSR equ 00200H
CR4_XMMEXCPT equ 00400H
CR4_LA57 equ 01000H
CR4_XSAVE equ 040000H
CR4_PCIDE equ 020000H
CR4_CET equ 0800000H

;
;  Legacy Floating Status Bit Masks
;

FSW_INVALID_OPERATION equ 00001H
FSW_DENORMAL equ 00002H
FSW_ZERO_DIVIDE equ 00004H
FSW_OVERFLOW equ 00008H
FSW_UNDERFLOW equ 00010H
FSW_PRECISION equ 00020H
FSW_STACK_FAULT equ 00040H
FSW_ERROR_SUMMARY equ 00080H
FSW_CONDITION_CODE_0 equ 00100H
FSW_CONDITION_CODE_1 equ 00200H
FSW_CONDITION_CODE_2 equ 00400H
FSW_CONDITION_CODE_3 equ 04000H
FSW_ERROR_MASK equ 0003FH

;
;  MXCSR Floating Control/Status Bit Masks
;

XSW_INVALID_OPERATION equ 00001H
XSW_DENORMAL equ 00002H
XSW_ZERO_DIVIDE equ 00004H
XSW_OVERFLOW equ 00008H
XSW_UNDERFLOW equ 00010H
XSW_PRECISION equ 00020H
XSW_ERROR_MASK equ 0003FH
XSW_ERROR_SHIFT equ 00007H
XCW_INVALID_OPERATION equ 00080H
XCW_DENORMAL equ 00100H
XCW_ZERO_DIVIDE equ 00200H
XCW_OVERFLOW equ 00400H
XCW_UNDERFLOW equ 00800H
XCW_PRECISION equ 01000H
XCW_ROUND_CONTROL equ 06000H
XCW_FLUSH_ZERO equ 08000H
INITIAL_FPCSR equ 0027FH
INITIAL_MXCSR equ 01F80H

;
;  Machine Specific Register Numbers
;

MSR_EFER equ 0C0000080H
MSR_STAR equ 0C0000081H
MSR_LSTAR equ 0C0000082H
MSR_CSTAR equ 0C0000083H
MSR_SYSCALL_MASK equ 0C0000084H
MSR_FS_BASE equ 0C0000100H
MSR_GS_BASE equ 0C0000101H
MSR_GS_SWAP equ 0C0000102H
MSR_MCG_STATUS equ 0017AH
MSR_AMD_ACCESS equ 09C5A203AH
MSR_IA32_MISC_ENABLE equ 001A0H
MSR_IA32_U_CET equ 006A0H
MSR_IA32_S_CET equ 006A2H
MSR_IA32_PL0_SSP equ 006A4H
MSR_IA32_PL3_SSP equ 006A7H
MSR_IA32_XFD equ 001C4H
MSR_IA32_XFD_ERR equ 001C5H

;
;  Flags within MSR_EFER
;

MSR_LMA equ 00400H
MSR_LME equ 00100H
MSR_SCE equ 00001H
MSR_NXE equ 00800H
MSR_PAT equ 00277H
MSR_DEBUG_CTL equ 001D9H

;
;  Flags within MSR_IA32_MISC_ENABLE
;

MSR_XD_ENABLE_MASK equ 0FFFFFFFBH

;
;  Flags within MSR_IA32_U_CET
;

IA32_CET_SH_STK_EN equ 00001H
IA32_CET_WR_SHSTK_EN equ 00002H

;
;  Flags within MSR_DEGUG_CTL
;

MSR_DEBUG_CTL_LBR equ 00001H
MSR_DEBUG_CTL_BTF equ 00002H

;
;  Flags within MSR_IA32_MCG_STATUS
;

MSR_IA32_MCG_STATUS_MCIP equ 00004H

;
;  Miscellaneous Definitions
;

MAXIMUM_PROCESSORS equ 00040H
INITIAL_STALL_COUNT equ 00064H
IRQL_NOT_GREATER_OR_EQUAL equ 00009H
IRQL_NOT_LESS_OR_EQUAL equ 0000AH
MUTEX_ALREADY_OWNED equ 000BFH
THREAD_NOT_MUTEX_OWNER equ 00011H
SPIN_LOCK_ALREADY_OWNED equ 0000FH
SPIN_LOCK_NOT_OWNED equ 00010H
Executive equ 00000H
FALSE equ 00000H
TRUE equ 00001H
NMI_BLOCKING_MASK equ 00004H
KERNEL_MCA_EXCEPTION_STACK_SIZE equ 02000H
IST_STACK_SIZE equ 06000H
ISR_STACK_SIZE equ 06000H
EXCEPTION_STACK_SIZE equ 06000H
EXCEPTION_EXECUTE_FAULT equ 00008H
KCACHE_ERRATA_MONITOR_FLAGS equ 00003H

;
;  Machine check nesting simulation debug controls
;

DBG_MCE_NMI_ON_MCE equ 00000H
DBG_MCE_MCE_ON_NMI equ 00001H
DBG_MCE_MCE_ON_NMI_MCE_TAIL equ 00002H
DBG_MCE_MCE_ON_MCE_MCE_TAIL equ 00003H
DBG_MCE_NMI_ON_MCE_MCE_TAIL equ 00004H
DBG_MCE_NMI_PENDING equ 0001FH

;
;  Flags returned from KiProcessControlProtection
;

KI_CP_HANDLED equ 00001H
KI_CP_NOT_HANDLED equ 00002H
KI_CP_SUPPORTED_HIJACK equ 00003H

;
;  Fast Fail Definitions
;

FAST_FAIL_GUARD_ICALL_CHECK_FAILURE equ 0000AH
FAST_FAIL_INVALID_JUMP_BUFFER equ 00012H
FAST_FAIL_INVALID_SET_OF_CONTEXT equ 0000DH
FAST_FAIL_GUARD_SS_FAILURE equ 0002CH
FAST_FAIL_CONTROL_INVALID_RETURN_ADDRESS equ 00039H
BASE_PRIORITY_THRESHOLD equ 00008H
EVENT_INCREMENT equ 00001H
LOW_REALTIME_PRIORITY equ 00010H
KERNEL_LARGE_STACK_COMMIT equ 06000H
KERNEL_LARGE_STACK_SIZE equ 012000H
DOUBLE_FAULT_STACK_SIZE equ 02000H
KI_KERNEL_SHADOW_STACK_SIZE_WITH_GUARD_PAGES equ 03000H
BREAKPOINT_BREAK equ 00000H
BREAKPOINT_COMMAND_STRING equ 00005H
BREAKPOINT_PRINT equ 00001H
BREAKPOINT_PROMPT equ 00002H
BREAKPOINT_LOAD_SYMBOLS equ 00003H
BREAKPOINT_UNLOAD_SYMBOLS equ 00004H
CLOCK_QUANTUM_DECREMENT equ 00003H
READY_SKIP_QUANTUM equ 00002H
THREAD_QUANTUM equ 00006H
WAIT_QUANTUM_DECREMENT equ 00001H
ROUND_TRIP_DECREMENT_COUNT equ 00010H
KI_EXCEPTION_GP_FAULT equ 010000001H
KI_EXCEPTION_INVALID_OP equ 010000002H
KI_EXCEPTION_INTEGER_DIVIDE_BY_ZERO equ 010000003H
KI_EXCEPTION_ACCESS_VIOLATION equ 010000004H
KI_EXCEPTION_SECURE_FAULT equ 010000006H
KI_EXCEPTION_SEGMENT_NOT_PRESENT equ 010000007H
KI_EXCEPTION_XFD_ERROR equ 010000010H
TARGET_FREEZE equ 00005H
KI_DPC_INTERRUPT_FLAGS equ 000AFH
KI_DPC_THREAD_ACTIVE equ 010000H
KernelShadowStackTypeRstorssp equ 00002H

;
;  Equates for exceptions which cause system fatal error
;

EXCEPTION_DIVIDED_BY_ZERO equ 00000H
EXCEPTION_DEBUG equ 00001H
EXCEPTION_NMI equ 00002H
EXCEPTION_INT3 equ 00003H
EXCEPTION_BOUND_CHECK equ 00005H
EXCEPTION_INVALID_OPCODE equ 00006H
EXCEPTION_NPX_NOT_AVAILABLE equ 00007H
EXCEPTION_DOUBLE_FAULT equ 00008H
EXCEPTION_NPX_OVERRUN equ 00009H
EXCEPTION_INVALID_TSS equ 0000AH
EXCEPTION_SEGMENT_NOT_PRESENT equ 0000BH
EXCEPTION_STACK_FAULT equ 0000CH
EXCEPTION_GP_FAULT equ 0000DH
EXCEPTION_RESERVED_TRAP equ 0000FH
EXCEPTION_NPX_ERROR equ 00010H
EXCEPTION_ALIGNMENT_CHECK equ 00011H
EXCEPTION_CP_FAULT equ 00015H
EXCEPTION_SE_FAULT equ 00017H
EXCEPTION_VIRTUALIZATION_FAULT equ 00020H

;
;  Exception Frame Offset Definitions and Length
;

ExP1Home equ 00000H
ExP2Home equ 00008H
ExP3Home equ 00010H
ExP4Home equ 00018H
ExP5 equ 00020H
ExXmm6 equ 00030H
ExXmm7 equ 00040H
ExXmm8 equ 00050H
ExXmm9 equ 00060H
ExXmm10 equ 00070H
ExXmm11 equ 00080H
ExXmm12 equ 00090H
ExXmm13 equ 000A0H
ExXmm14 equ 000B0H
ExXmm15 equ 000C0H
ExMxCsr equ 000F0H
ExRbp equ 000F8H
ExRbx equ 00100H
ExRdi equ 00108H
ExRsi equ 00110H
ExR12 equ 00118H
ExR13 equ 00120H
ExR14 equ 00128H
ExR15 equ 00130H
ExReturn equ 00138H

KEXCEPTION_FRAME_LENGTH equ 00140H
EXCEPTION_RECORD_LENGTH equ 000A0H

Enable_Feature_DTrace equ 00001H

;
;  Kernel Dynamic Tracing flags
;

KI_DYNAMIC_TRACE_SYSCALL_FLAG equ 00001H


;
;  Processor Descriptor Area definitions
;

PdaGdt equ 02FB0H
PdaKernelGsBase equ 01070H
PdaIdleStack equ 01078H
PdaKernelTransitionStack equ 04200H
PdaMceTransitionStack equ 04600H
PdaNmiTransitionStack equ 04800H
KSHADOW_FLAG_DEFERRED_FLUSH equ 00001H
KSHADOW_FLAG_DEFERRED_FLUSH_BIT equ 00000H
KSHADOW_FLAG_KERNEL_DIRECTORY equ 00002H
KSHADOW_FLAG_KERNEL_DIRECTORY_BIT equ 00001H


;
;  IST Base Frame definitions
;

IbfKernelGsBase equ 00000H
IbfIstStack equ 00008H
IbfPreviousGsBase equ 00010H
IbfPreviousCr3 equ 00018H
IstBaseFrameLength equ 00030H
IstLinkFrameLength equ 00020H
IST_TRANSITION_STACK_SIZE equ 001D0H


;
;  IST Link Frame definitions
;

KIST_LINK_FRAME_SIGNATURE equ 0464C494BH
IlfIstBaseFrame equ 00000H
IlfSignature equ 00008H
IstLinkFrameLength equ 00020H


;
;  PCID definitions
;

PCID_MASK equ 00003H
PCID_USER equ 00001H
PCID_USER_BIT equ 00000H
PCID_KERNEL equ 00002H
PCID_KERNEL_BIT equ 00001H
PCID_PRESERVE_TB_BIT equ 0003FH

KI_FLUSH_PCID equ 00001H
KI_FLUSH_INVPCID equ 00002H

KvaShadowEnabledUserGlobal equ 00002H

;
;  Jump Offset Definitions and Length
;

JbFrame equ 00000H
JbRbx equ 00008H
JbRsp equ 00010H
JbRbp equ 00018H
JbRsi equ 00020H
JbRdi equ 00028H
JbR12 equ 00030H
JbR13 equ 00038H
JbR14 equ 00040H
JbR15 equ 00048H
JbRip equ 00050H
JbMxCsr equ 00058H
JbFpCsr equ 0005CH
JbSpare equ 0005EH
JbXmm6 equ 00060H
JbXmm7 equ 00070H
JbXmm8 equ 00080H
JbXmm9 equ 00090H
JbXmm10 equ 000A0H
JbXmm11 equ 000B0H
JbXmm12 equ 000C0H
JbXmm13 equ 000D0H
JbXmm14 equ 000E0H
JbXmm15 equ 000F0H

;
;  Switch Frame Offset Definitions and Length
;

SwP5Home equ 00020H
SwApcBypass equ 00028H
SwRbp equ 00030H
SwReturn equ 00038H

KSWITCH_FRAME_LENGTH equ 00040H

;
;  Start System Thread Frame Offset Definitions and Length
;

SfP1Home equ 00000H
SfP2Home equ 00008H
SfP3Home equ 00010H
SfP4Home equ 00018H
SfReturn equ 00028H

KSTART_FRAME_LENGTH equ 00030H

;
;  Trap Frame Offset and EFLAG Definitions and Length
;

EFLAGS_TF_MASK equ 00100H
EFLAGS_TF_SHIFT equ 00008H
EFLAGS_ID_MASK equ 0200000H
EFLAGS_AC_MASK equ 040000H
EFLAGS_IF_MASK equ 00200H
EFLAGS_IF_SHIFT equ 00009H
EFLAGS_IF_BIT equ 00009H
KEXCEPTION_ACTIVE_INTERRUPT_FRAME equ 00000H
KEXCEPTION_ACTIVE_EXCEPTION_FRAME equ 00001H
KEXCEPTION_ACTIVE_SERVICE_FRAME equ 00002H

TrP1Home equ 0FFFFFF80H
TrP2Home equ 0FFFFFF88H
TrP3Home equ 0FFFFFF90H
TrP4Home equ 0FFFFFF98H
TrP5 equ 0FFFFFFA0H
TrPreviousMode equ 0FFFFFFA8H
TrPreviousIrql equ 0FFFFFFA9H
TrFaultIndicator equ 0FFFFFFAAH
TrExceptionActive equ 0FFFFFFABH
TrMxCsr equ 0FFFFFFACH
TrRax equ 0FFFFFFB0H
TrRcx equ 0FFFFFFB8H
TrRdx equ 0FFFFFFC0H
TrR8 equ 0FFFFFFC8H
TrR9 equ 0FFFFFFD0H
TrR10 equ 0FFFFFFD8H
TrR11 equ 0FFFFFFE0H
TrGsBase equ 0FFFFFFE8H
TrGsSwap equ 0FFFFFFE8H
TrXmm0 equ 0FFFFFFF0H
TrXmm1 equ 00000H
TrXmm2 equ 00010H
TrXmm3 equ 00020H
TrXmm4 equ 00030H
TrXmm5 equ 00040H
TrFaultAddress equ 00050H
TrDr0 equ 00058H
TrDr1 equ 00060H
TrDr2 equ 00068H
TrDr3 equ 00070H
TrDr6 equ 00078H
TrDr7 equ 00080H
TrShadowStackFrame equ 00058H
TrDebugControl equ 00088H
TrLastBranchToRip equ 00090H
TrLastBranchFromRip equ 00098H
TrLastExceptionToRip equ 000A0H
TrLastExceptionFromRip equ 000A8H
TrSegDs equ 000B0H
TrSegEs equ 000B2H
TrSegFs equ 000B4H
TrSegGs equ 000B6H
TrTrapFrame equ 000B8H
TrRbx equ 000C0H
TrRdi equ 000C8H
TrRsi equ 000D0H
TrRbp equ 000D8H
TrErrorCode equ 000E0H
TrRip equ 000E8H
TrSegCs equ 000F0H
TrLogging equ 000F3H
TrEFlags equ 000F8H
TrRsp equ 00100H
TrSegSs equ 00108H
;
;

KTRAP_FRAME_LENGTH equ 00190H

;
;  CPU information structure offset definitions
;

CpuEax equ 00000H
CpuEbx equ 00004H
CpuEcx equ 00008H
CpuEdx equ 0000CH

;
;  Usermode Callout User Frame Definitions
;

CkBuffer equ 00020H
CkLength equ 00028H
CkApiNumber equ 0002CH
CkRsp equ 00048H
CkRip equ 00030H
CalloutFrameLength equ 00058H

;
;  Machine Frame Offset Definitions
;

MfRip equ 00000H
MfSegCs equ 00008H
MfEFlags equ 00010H
MfRsp equ 00018H
MfSegSs equ 00020H
MachineFrameLength equ 00028H

;
;  Shadow Stack Machine Frame Offset Definitions
;

SmSsp equ 00000H
SmRip equ 00008H
SmSegCs equ 00010H

;
;  DPC Stack Frame Defintions
;

DpRsp equ 00040H
DpRip equ 00028H
DpP5 equ 00020H
DpcStackFrameLength equ 00050H

;
;  Context Frame Offset and Flag Definitions
;

CONTEXT_FULL equ 010000BH
CONTEXT_CONTROL equ 0100001H
CONTEXT_INTEGER equ 0100002H
CONTEXT_SEGMENTS equ 0100004H
CONTEXT_FLOATING_POINT equ 0100008H
CONTEXT_DEBUG_REGISTERS equ 0100010H
CONTEXT_XSTATE equ 0100040H
CONTEXT_KERNEL_CET equ 0100080H

CxP1Home equ 00000H
CxP2Home equ 00008H
CxP3Home equ 00010H
CxP4Home equ 00018H
CxP5Home equ 00020H
CxP6Home equ 00028H
CxContextFlags equ 00030H
CxMxCsr equ 00034H
CxSegCs equ 00038H
CxSegDs equ 0003AH
CxSegEs equ 0003CH
CxSegFs equ 0003EH
CxSegGs equ 00040H
CxSegSs equ 00042H
CxEFlags equ 00044H
CxDr0 equ 00048H
CxDr1 equ 00050H
CxDr2 equ 00058H
CxDr3 equ 00060H
CxDr6 equ 00068H
CxDr7 equ 00070H
CxRax equ 00078H
CxRcx equ 00080H
CxRdx equ 00088H
CxRbx equ 00090H
CxRsp equ 00098H
CxRbp equ 000A0H
CxRsi equ 000A8H
CxRdi equ 000B0H
CxR8 equ 000B8H
CxR9 equ 000C0H
CxR10 equ 000C8H
CxR11 equ 000D0H
CxR12 equ 000D8H
CxR13 equ 000E0H
CxR14 equ 000E8H
CxR15 equ 000F0H
CxRip equ 000F8H
CxFltSave equ 00100H
CxXmm0 equ 001A0H
CxXmm1 equ 001B0H
CxXmm2 equ 001C0H
CxXmm3 equ 001D0H
CxXmm4 equ 001E0H
CxXmm5 equ 001F0H
CxXmm6 equ 00200H
CxXmm7 equ 00210H
CxXmm8 equ 00220H
CxXmm9 equ 00230H
CxXmm10 equ 00240H
CxXmm11 equ 00250H
CxXmm12 equ 00260H
CxXmm13 equ 00270H
CxXmm14 equ 00280H
CxXmm15 equ 00290H
CxDebugControl equ 004A8H
CxLastBranchToRip equ 004B0H
CxLastBranchFromRip equ 004B8H
CxLastExceptionToRip equ 004C0H
CxLastExceptionFromRip equ 004C8H
CxVectorControl equ 004A0H
CxVectorRegister equ 00300H
CONTEXT_FRAME_LENGTH equ 004D0H
CONTEXT_EX_LENGTH equ 00020H

;
;  Debug Control Register Masks
;

DR7_LEGAL equ 0FFFF0355H
DR7_ACTIVE equ 00355H
DR7_TRACE_BRANCH equ 00200H
DR7_LAST_BRANCH equ 00100H
DEBUG_ACTIVE_DR7 equ 00001H
DEBUG_ACTIVE_INSTRUMENTED equ 00002H
DEBUG_ACTIVE_DBG_INSTRUMENTED equ 00003H
DEBUG_ACTIVE_MINIMAL_THREAD equ 00004H
DEBUG_ACTIVE_SET_CONTEXT_STATE_LOCK_BIT equ 0001BH
DEBUG_ACTIVE_SET_CONTEXT_STATE_LOCK equ 08000000H

;
;  Continue Argument Structure Definitions
;

KCONTINUE_INVALID equ 00004H
KCONTINUE_ARGUMENT_LENGTH equ 00020H

;
;  SK Shadow Stack Assist Operation Definitions
;

SkShadowStackAssistDebugTrapCopyFrame equ 00002H

;
;  Dispatcher Context Structure Offset Definitions
;

DcControlPc equ 00000H
DcImageBase equ 00008H
DcFunctionEntry equ 00010H
DcEstablisherFrame equ 00018H
DcTargetIp equ 00020H
DcContextRecord equ 00028H
DcLanguageHandler equ 00030H
DcHandlerData equ 00038H
DcHistoryTable equ 00040H
DcScopeIndex equ 00048H

;
;  Legacy Floating Save Area Structure Offset definitions
;

LfControlWord equ 00000H
LfStatusWord equ 00002H
LfTagWord equ 00004H
LfErrorOpcode equ 00006H
LfErrorOffset equ 00008H
LfErrorSelector equ 0000CH
LfDataOffset equ 00010H
LfDataSelector equ 00014H
LfMxCsr equ 00018H
LfMxCsr_Mask equ 0001CH
LfFloatRegisters equ 00020H
LfXmmRegisters equ 000A0H
LfFloatSaveLength equ 00200H
X87ErrorOffset equ 0000CH
X87FloatSaveLength equ 0006CH


;
;  Processor State Frame Offset Definitions
;

PsSpecialRegisters equ 00000H
PsCr0 equ 00000H
PsCr2 equ 00008H
PsCr3 equ 00010H
PsCr4 equ 00018H
PsKernelDr0 equ 00020H
PsKernelDr1 equ 00028H
PsKernelDr2 equ 00030H
PsKernelDr3 equ 00038H
PsKernelDr6 equ 00040H
PsKernelDr7 equ 00048H
PsGdtr equ 00056H
PsIdtr equ 00066H
PsTr equ 00070H
PsLdtr equ 00072H
PsMxCsr equ 00074H
PsMsrGsBase equ 000A8H
PsMsrGsSwap equ 000B0H
PsMsrStar equ 000B8H
PsMsrLStar equ 000C0H
PsMsrCStar equ 000C8H
PsMsrSyscallMask equ 000D0H
PsXcr0 equ 000D8H
PsMsrFsBase equ 000E0H
PsContextFrame equ 000F0H
PsDebugControl equ 00078H
PsLastBranchToRip equ 00080H
PsLastBranchFromRip equ 00088H
PsLastExceptionToRip equ 00090H
PsLastExceptionFromRip equ 00098H
PsCr8 equ 000A0H
ProcessorStateLength equ 005C0H

;
;  Processor Start Block Offset Definitions
;

PsbCompletionFlag equ 00004H
PsbGdt32 equ 0000CH
PsbIdt32 equ 00012H
PsbGdt equ 00018H
PsbTiledMemoryMap equ 00058H
PsbPmTarget equ 00060H
PsbLmTarget equ 00070H
PsbLmIdentityTarget equ 00066H
PsbSelfMap equ 00078H
PsbMsrPat equ 00080H
PsbMsrEFER equ 00088H
PsbProcessorState equ 00090H
PsbFlags equ 00008H
PROCESSOR_START_FLAG_FORCE_ENABLE_NX equ 00001H
PROCESSOR_START_FLAG_LA57 equ 00002H
PROCESSOR_START_FLAG_HIDDEN equ 00004H
ProcessorStartBlockLength equ 00650H
XHF_NOEXECUTE equ 0100000H

;
;  MCE Recovery Context Offset Definitions
;

MrcFlags equ 00000H
MrcPhysicalAddress equ 00008H
MrcRecord equ 00008H
MceRecoveryContextLength equ 00010H

;
;  MCE Recovery Context Flags Definitions
;

KMRC_ALTERNATE_CONTEXT equ 00001H
KMRC_WORK_ITEM equ 00002H
KMRC_OFFLINE_PAGE equ 00004H
KMRC_TERMINATE_PROCESS equ 00008H
KMRC_KERNEL_CONSUMER equ 00010H

;
;  KTHREAD
;

ThNpxState equ 00250H
ThThreadFlags2 equ 00200H
ThSpecCtrl equ 0007FH
KTHREAD_RUNNING_NON_RETPOLINE_CODE_BIT equ 00001H
KTHREAD_RUNNING_NON_RETPOLINE_CODE equ 00002H
ThKernelShadowStack equ 00408H
ThKernelShadowStackBase equ 00418H
ThKernelShadowStackInitial equ 00410H
ThKernelShadowStackLimit equ 00420H
ThExtendedFeatureDisableMask equ 00428H

;
;  ETHREAD
;

ThSetContextState equ 00668H
ThUserFsBase equ 00638H
ThUserGsBase equ 00640H
PICO_SYSTEM_CALL equ 00008H

;
;  EPROCESS
;

EpDebugPort equ 00578H
EpWoW64Process equ 00580H
EpFlags3 equ 0087CH
PS_PROCESS_FLAGS3_SECURITY_DOMAIN_CHANGED_V equ 00015H
PS_PROCESS_MITIGATION_FLAGS2_CET_RELATED equ 0C0F6C000H
;
;

;
;  Machine type definitions (Temporarily)
;

MACHINE_TYPE_ISA equ 00000H
MACHINE_TYPE_EISA equ 00001H
MACHINE_TYPE_MCA equ 00002H

;
;  KeFeatureBits defines
;

KF_RDTSC equ 00002H
KF_RDTSCP equ 00000000400000000H
KF_CR4 equ 00004H
KF_GLOBAL_PAGE equ 00010H
KF_LARGE_PAGE equ 00020H
KF_CMPXCHG8B equ 00080H
KF_FAST_SYSCALL equ 01000H
KF_BRANCH equ 020000H
KF_XSTATE equ 0800000H
KF_XSAVEOPT_BIT equ 0000FH
KF_XSTATE_BIT equ 00017H
KF_XSAVES_BIT equ 00026H
KF_RDWRFSGSBASE_BIT equ 0001CH
KF_CET_SS equ 00000400000000000H
KF_XFD equ 00080000000000000H
KF_FPU_LEAKAGE_BIT equ 00029H
KF_CAT_BIT equ 0002CH
KF_XFD_BIT equ 00037H

;
;  LoaderParameterBlock offsets relative to base
;

LpbKernelStack equ 00080H
LpbPrcb equ 00088H
LpbProcess equ 00090H
LpbThread equ 00098H
PAGE_SIZE equ 01000H

;
;  Kernel Mode Callout Frame Definitions
;

CuTrapFrame equ 000D0H
CuOutputBuffer equ 000D8H
CuOutputLength equ 000E0H

;
;  Enclave call dispatch frame
;

EcEnclaveNumber equ 00020H
EcParameterAddress equ 00028H
EcParameterValue equ 00030H
EcOriginalReturn equ 00038H
EcFramePointer equ 00040H
EcReturnAddress equ 00048H

;
;  Hypervisor Enlightenment Definitions
;

HV_MMU_USE_HYPERCALL_FOR_ADDRESS_SWITCH equ 00001H
HV_MMU_USE_HYPERCALL_FOR_LOCAL_FLUSH equ 00002H
HV_MMU_USE_HYPERCALL_FOR_REMOTE_FLUSH equ 00004H
HV_APIC_ENLIGHTENED equ 00010H
HV_KE_USE_HYPERCALL_FOR_LONG_SPIN_WAIT equ 00040H
HV_DEPRECATE_AUTO_EOI equ 01000H
HV_X64_MSR_EOI equ 040000070H
HV_X64_MSR_ICR equ 040000071H
HV_X64_MSR_TSC_FREQUENCY equ 040000022H
APIC_DELIVER_NMI equ 00400H
LAPIC_ICR_SELF equ 040000H
HV_VIRTUAL_APIC_NO_EOI_REQUIRED equ 00000H
HvApicFlags equ 00000H
HvVirtualFaultCode equ 00044H
HvVirtualFaultParam equ 00048H
HvExtVirtualizationFaultEpf equ 00001H

;
;  IO Status Block Offset Definitions
;

IoStatus equ 00000H
IoPointer equ 00000H
IoInformation equ 00008H

;
;  KAFFINITY_EX size
;

AffinityExLength equ 00108H

;
;  WoW Compatibility mode transition thunk
;

ServiceCpupReturnFromSimulatedCode equ 0001FH
HV_VECTOR equ 00030H
;
