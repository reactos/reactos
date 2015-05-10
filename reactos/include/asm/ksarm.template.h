
RAW(""),
RAW("#include <kxarm.h>"),
RAW(""),

HEADER("CPSR Values"),
CONSTANT(CPSRM_USER),
CONSTANT(CPSRM_FIQ),
CONSTANT(CPSRM_INT),
CONSTANT(CPSRM_SVC),
CONSTANT(CPSRM_ABT),
CONSTANT(CPSRM_UDF),
CONSTANT(CPSRM_SYS),
CONSTANT(CPSRM_MASK),
CONSTANT(SYSCALL_PSR),

CONSTANT(CPSRF_N), // 0x80000000
CONSTANT(CPSRF_Z), // 0x40000000
CONSTANT(CPSRF_C), // 0x20000000
CONSTANT(CPSRF_V), // 0x10000000
CONSTANT(CPSRF_Q), // 0x8000000
CONSTANT(CPSR_IT_MASK), // 0x600fc00

CONSTANT(FPSCRF_N), // 0x80000000
CONSTANT(FPSCRF_Z), // 0x40000000
CONSTANT(FPSCRF_C), // 0x20000000
CONSTANT(FPSCRF_V), // 0x10000000
CONSTANT(FPSCRF_QC), // 0x8000000

CONSTANT(FPSCRM_AHP), // 0x4000000
CONSTANT(FPSCRM_DN), // 0x2000000
CONSTANT(FPSCRM_FZ), // 0x1000000
CONSTANT(FPSCRM_RMODE_MASK), // 0xc00000
CONSTANT(FPSCRM_RMODE_RN), // 0x0
CONSTANT(FPSCRM_RMODE_RP), // 0x400000
CONSTANT(FPSCRM_RMODE_RM), // 0x800000
CONSTANT(FPSCRM_RMODE_RZ), // 0xc00000
CONSTANT(FPSCRM_DEPRECATED), // 0x370000

CONSTANT(FPSCR_IDE), // 0x8000
CONSTANT(FPSCR_IXE), // 0x1000
CONSTANT(FPSCR_UFE), // 0x800
CONSTANT(FPSCR_OFE), // 0x400
CONSTANT(FPSCR_DZE), // 0x200
CONSTANT(FPSCR_IOE), // 0x100
CONSTANT(FPSCR_IDC), // 0x80
CONSTANT(FPSCR_IXC), // 0x10
CONSTANT(FPSCR_UFC), // 0x8
CONSTANT(FPSCR_OFC), // 0x4
CONSTANT(FPSCR_DZC), // 0x2
CONSTANT(FPSCR_IOC), // 0x1

CONSTANT(CPSRC_INT), // 0x80
CONSTANT(CPSRC_ABORT), // 0x100
CONSTANT(CPSRC_THUMB), // 0x20

CONSTANT(SWFS_PAGE_FAULT), // 0x10
CONSTANT(SWFS_ALIGN_FAULT), // 0x20
CONSTANT(SWFS_HWERR_FAULT), // 0x40
CONSTANT(SWFS_DEBUG_FAULT), // 0x80
CONSTANT(SWFS_EXECUTE), // 0x8
CONSTANT(SWFS_WRITE), // 0x1

CONSTANT(CP14_DBGDSCR_MOE_MASK), // 0x3c
CONSTANT(CP14_DBGDSCR_MOE_SHIFT), // 0x2
CONSTANT(CP14_DBGDSCR_MOE_HALT), // 0x0
CONSTANT(CP14_DBGDSCR_MOE_BP), // 0x1
CONSTANT(CP14_DBGDSCR_MOE_WPASYNC), // 0x2
CONSTANT(CP14_DBGDSCR_MOE_BKPT), // 0x3
CONSTANT(CP14_DBGDSCR_MOE_EXTERNAL), // 0x4
CONSTANT(CP14_DBGDSCR_MOE_VECTOR), // 0x5
CONSTANT(CP14_DBGDSCR_MOE_WPSYNC), // 0xa

CONSTANT(CP15_PMCR_DP), // 0x20
CONSTANT(CP15_PMCR_X), // 0x10
CONSTANT(CP15_PMCR_CLKCNT_DIV), // 0x8
CONSTANT(CP15_PMCR_CLKCNT_RST), // 0x4
CONSTANT(CP15_PMCR_CNT_RST), // 0x2
CONSTANT(CP15_PMCR_ENABLE), // 0x1

HEADER("DebugService Control Types"),
//CONSTANT(BREAKPOINT_HW_SYNCH_WATCH), // 0x6
//CONSTANT(BREAKPOINT_HW_ASYNCH_WATCH), // 0x7
//CONSTANT(BREAKPOINT_HW_BREAK), // 0x8

// Handle table entry definition (FIXME: since win10, portable?)
#if (NTDDI_VERSION >= NTDDI_WIN10)
HEADER("Handle table entry definition"),
#define EXHANDLE_TABLE_ENTRY_LOCK 0x1
#define EXHANDLE_REFERENCE_SHIFT 0x1b
#define EXHANDLE_REF_ACQUIRE_LOCK 0xffffffff
#define EXHANDLE_REPLENISH_REFS 0x8
#define EXHANDLE_CACHED_REFS 0x1f
#endif

HEADER("Other constants"),
CONSTANT(PAGE_SHIFT),
//CONSTANT(PTE_TOP),
//CONSTANT(TRANSITION_ASID),
CONSTANT(KI_EXCEPTION_INTERNAL), // 0x10000000
//CONSTANT(KI_EXCEPTION_HARDWARE_ERROR), // 0x10000005

HEADER("CONTEXT Offsets"),
OFFSET(CxContextFlags, CONTEXT, ContextFlags), // 0x0
OFFSET(CxR0, CONTEXT, R0), // 0x4
OFFSET(CxR1, CONTEXT, R1), // 0x8
OFFSET(CxR2, CONTEXT, R2), // 0xc
OFFSET(CxR3, CONTEXT, R3), // 0x10
OFFSET(CxR4, CONTEXT, R4), // 0x14
OFFSET(CxR5, CONTEXT, R5), // 0x18
OFFSET(CxR6, CONTEXT, R6), // 0x1c
OFFSET(CxR7, CONTEXT, R7), // 0x20
OFFSET(CxR8, CONTEXT, R8), // 0x24
OFFSET(CxR9, CONTEXT, R9), // 0x28
OFFSET(CxR10, CONTEXT, R10), // 0x2c
OFFSET(CxR11, CONTEXT, R11), // 0x30
OFFSET(CxR12, CONTEXT, R12), // 0x34
OFFSET(CxSp, CONTEXT, Sp), // 0x38
OFFSET(CxLr, CONTEXT, Lr), // 0x3c
OFFSET(CxPc, CONTEXT, Pc), // 0x40
OFFSET(CxCpsr, CONTEXT, Cpsr), // 0x44
OFFSET(CxFpscr, CONTEXT, Fpscr), // 0x48
OFFSET(CxQ, CONTEXT, Q), // 0x50
OFFSET(CxD, CONTEXT, D), // 0x50
OFFSET(CxS, CONTEXT, S), // 0x50
OFFSET(CxD8, CONTEXT, D[8]), // 0x90
OFFSET(CxBvr, CONTEXT, Bvr), // 0x150
OFFSET(CxBcr, CONTEXT, Bcr), // 0x170
OFFSET(CxWvr, CONTEXT, Wvr), // 0x190
OFFSET(CxWcr, CONTEXT, Wcr), // 0x194
SIZE(CONTEXT_FRAME_LENGTH, CONTEXT), // 0x1a0
//CONSTANT(CONTEXT_ALIGN, __alignof(CONTEXT)),

HEADER("_JUMP_BUFFER offsets"),
OFFSET(JbFrame, _JUMP_BUFFER, Frame), // 0x0
OFFSET(JbR4, _JUMP_BUFFER, R4), // 0x4
OFFSET(JbR5, _JUMP_BUFFER, R5), // 0x8
OFFSET(JbR6, _JUMP_BUFFER, R6), // 0xc
OFFSET(JbR7, _JUMP_BUFFER, R7), // 0x10
OFFSET(JbR8, _JUMP_BUFFER, R8), // 0x14
OFFSET(JbR9, _JUMP_BUFFER, R9), // 0x18
OFFSET(JbR10, _JUMP_BUFFER, R10), // 0x1c
OFFSET(JbR11, _JUMP_BUFFER, R11), // 0x20
OFFSET(JbSp, _JUMP_BUFFER, Sp), // 0x24
OFFSET(JbPc, _JUMP_BUFFER, Pc), // 0x28
OFFSET(JbFpscr, _JUMP_BUFFER, Fpscr), // 0x2c
OFFSET(JbD, _JUMP_BUFFER, D), // 0x30

HEADER("DISPATCHER_CONTEXT offsets"),
OFFSET(DcControlPc, DISPATCHER_CONTEXT, ControlPc), // 0x0
OFFSET(DcImageBase, DISPATCHER_CONTEXT, ImageBase), // 0x4
OFFSET(DcFunctionEntry, DISPATCHER_CONTEXT, FunctionEntry), // 0x8
OFFSET(DcEstablisherFrame, DISPATCHER_CONTEXT, EstablisherFrame), // 0xc
OFFSET(DcTargetPc, DISPATCHER_CONTEXT, TargetPc), // 0x10
OFFSET(DcContextRecord, DISPATCHER_CONTEXT, ContextRecord), // 0x14
OFFSET(DcLanguageHandler, DISPATCHER_CONTEXT, LanguageHandler), // 0x18
OFFSET(DcHandlerData, DISPATCHER_CONTEXT, HandlerData), // 0x1c
OFFSET(DcHistoryTable, DISPATCHER_CONTEXT, HistoryTable), // 0x20
OFFSET(DcScopeIndex, DISPATCHER_CONTEXT, ScopeIndex), // 0x24
OFFSET(DcControlPcIsUnwound, DISPATCHER_CONTEXT, ControlPcIsUnwound), // 0x28
OFFSET(DcNonVolatileRegisters, DISPATCHER_CONTEXT, NonVolatileRegisters), // 0x2c
OFFSET(DcReserved, DISPATCHER_CONTEXT, Reserved), // 0x30

HEADER("Trap Frame offsets"),
OFFSET(TrArg3, KTRAP_FRAME, Arg3), // 0x0
OFFSET(TrFaultStatus, KTRAP_FRAME, FaultStatus), // 0x4
OFFSET(TrFaultAddress, KTRAP_FRAME, FaultAddress), // 0x8
OFFSET(TrTrapFrame, KTRAP_FRAME, TrapFrame), // 0x8
OFFSET(TrReserved, KTRAP_FRAME, Reserved), // 0xc
OFFSET(TrExceptionActive, KTRAP_FRAME, ExceptionActive), // 0x10
OFFSET(TrPreviousMode, KTRAP_FRAME, PreviousMode), // 0x13
OFFSET(TrDebugRegistersValid, KTRAP_FRAME, DebugRegistersValid), // 0x12
OFFSET(TrBvr, KTRAP_FRAME, Bvr), // 0x18
OFFSET(TrBcr, KTRAP_FRAME, Bcr), // 0x38
OFFSET(TrWvr, KTRAP_FRAME, Wvr), // 0x58
OFFSET(TrWcr, KTRAP_FRAME, Wcr), // 0x5c
OFFSET(TrVfpState, KTRAP_FRAME, VfpState), // 0x14
OFFSET(TrR0, KTRAP_FRAME, R0), // 0x60
OFFSET(TrR1, KTRAP_FRAME, R1), // 0x64
OFFSET(TrR2, KTRAP_FRAME, R2), // 0x68
OFFSET(TrR3, KTRAP_FRAME, R3), // 0x6c
OFFSET(TrR12, KTRAP_FRAME, R12), // 0x70
OFFSET(TrSp, KTRAP_FRAME, Sp), // 0x74
OFFSET(TrLr, KTRAP_FRAME, Lr), // 0x78
OFFSET(TrR11, KTRAP_FRAME, R11), // 0x7c
OFFSET(TrPc, KTRAP_FRAME, Pc), // 0x80
OFFSET(TrCpsr, KTRAP_FRAME, Cpsr), // 0x84
SIZE(KTRAP_FRAME_LENGTH, KTRAP_FRAME), // 0x88

HEADER("KEXCEPTION_FRAME offsets"),
OFFSET(ExParam5, KEXCEPTION_FRAME, Param5), // 0x0
OFFSET(ExTrapFrame, KEXCEPTION_FRAME, TrapFrame), // 0x4
OFFSET(ExR4, KEXCEPTION_FRAME, R4), // 0x14
OFFSET(ExR5, KEXCEPTION_FRAME, R5), // 0x18
OFFSET(ExR6, KEXCEPTION_FRAME, R6), // 0x1c
OFFSET(ExR7, KEXCEPTION_FRAME, R7), // 0x20
OFFSET(ExR8, KEXCEPTION_FRAME, R8), // 0x24
OFFSET(ExR9, KEXCEPTION_FRAME, R9), // 0x28
OFFSET(ExR10, KEXCEPTION_FRAME, R10), // 0x2c
OFFSET(ExR11, KEXCEPTION_FRAME, R11), // 0x30
OFFSET(ExReturn, KEXCEPTION_FRAME, Return), // 0x34
SIZE(KEXCEPTION_FRAME_LENGTH, KEXCEPTION_FRAME), // 0x38

HEADER("KSPECIAL_REGISTERS offsets"),
OFFSET(KsCp15_Cr13_UsrRW, KSPECIAL_REGISTERS, Cp15_Cr13_UsrRW), // 0x1c
OFFSET(KsCp15_Cr13_UsrRO, KSPECIAL_REGISTERS, Cp15_Cr13_UsrRO), // 0x20
OFFSET(KsCp15_Cr13_SvcRW, KSPECIAL_REGISTERS, Cp15_Cr13_SvcRW), // 0x24
OFFSET(KsKernelBvr, KSPECIAL_REGISTERS, KernelBvr), // 0x28
OFFSET(KsKernelBcr, KSPECIAL_REGISTERS, KernelBcr), // 0x48
OFFSET(KsKernelWcr, KSPECIAL_REGISTERS, KernelWcr), // 0x6c
OFFSET(KsFpexc, KSPECIAL_REGISTERS, Fpexc), // 0x70
OFFSET(KsFpinst, KSPECIAL_REGISTERS, Fpinst), // 0x74
OFFSET(KsFpinst2, KSPECIAL_REGISTERS, Fpinst2), // 0x78
OFFSET(KsUserSp, KSPECIAL_REGISTERS, UserSp), // 0x7c
OFFSET(KsUserLr, KSPECIAL_REGISTERS, UserLr), // 0x80
OFFSET(KsAbortSp, KSPECIAL_REGISTERS, AbortSp), // 0x84
OFFSET(KsAbortLr, KSPECIAL_REGISTERS, AbortLr), // 0x88
OFFSET(KsAbortSpsr, KSPECIAL_REGISTERS, AbortSpsr), // 0x8c
OFFSET(KsUdfSp, KSPECIAL_REGISTERS, UdfSp), // 0x90
OFFSET(KsUdfLr, KSPECIAL_REGISTERS, UdfLr), // 0x94
OFFSET(KsUdfSpsr, KSPECIAL_REGISTERS, UdfSpsr), // 0x98
OFFSET(KsIrqSp, KSPECIAL_REGISTERS, IrqSp), // 0x9c
OFFSET(KsIrqLr, KSPECIAL_REGISTERS, IrqLr), // 0xa0
OFFSET(KsIrqSpsr, KSPECIAL_REGISTERS, IrqSpsr), // 0xa4

HEADER("KPROCESSOR_STATE offsets"),
OFFSET(PsSpecialRegisters, KPROCESSOR_STATE, SpecialRegisters), // 0x0
OFFSET(PsUsrRW, KPROCESSOR_STATE, SpecialRegisters.Cp15_Cr13_UsrRW), // 0x1c
OFFSET(PsUsrRO, KPROCESSOR_STATE, SpecialRegisters.Cp15_Cr13_UsrRO), // 0x20
OFFSET(PsSvcRW, KPROCESSOR_STATE, SpecialRegisters.Cp15_Cr13_SvcRW), // 0x24
OFFSET(PsArchState, KPROCESSOR_STATE, ArchState), // 0xa8
OFFSET(PsCpuid, KPROCESSOR_STATE, ArchState.Cp15_Cr0_CpuId), // 0xa8
OFFSET(PsControl, KPROCESSOR_STATE, ArchState.Cp15_Cr1_Control), // 0xac
OFFSET(PsAuxControl, KPROCESSOR_STATE, ArchState.Cp15_Cr1_AuxControl), // 0xb0
OFFSET(PsCpacr, KPROCESSOR_STATE, ArchState.Cp15_Cr1_Cpacr), // 0xb4
OFFSET(PsTtbControl, KPROCESSOR_STATE, ArchState.Cp15_Cr2_TtbControl), // 0xb8
OFFSET(PsTtb0, KPROCESSOR_STATE, ArchState.Cp15_Cr2_Ttb0), // 0xbc
OFFSET(PsTtb1, KPROCESSOR_STATE, ArchState.Cp15_Cr2_Ttb1), // 0xc0
OFFSET(PsDacr, KPROCESSOR_STATE, ArchState.Cp15_Cr3_Dacr), // 0xc4
OFFSET(PsPrimaryMemoryRemap, KPROCESSOR_STATE, ArchState.Cp15_Cr10_PrimaryMemoryRemap), // 0x1ec
OFFSET(PsNormalMemoryRemap, KPROCESSOR_STATE, ArchState.Cp15_Cr10_NormalMemoryRemap), // 0x1f0
OFFSET(PsVBARns, KPROCESSOR_STATE, ArchState.Cp15_Cr12_VBARns), // 0x1f4
OFFSET(PsAsid, KPROCESSOR_STATE, ArchState.Cp15_Cr13_ContextId), // 0x1f8
OFFSET(PsContextId, KPROCESSOR_STATE, ArchState.Cp15_Cr13_ContextId), // 0x1f8
OFFSET(PsContextFrame, KPROCESSOR_STATE, ContextFrame), // 0x200
SIZE(ProcessorStateLength, KPROCESSOR_STATE), // 0x3a0

HEADER("KARM_ARCH_STATE offsets"),
OFFSET(AaCp15_Cr0_CpuId, KARM_ARCH_STATE, Cp15_Cr0_CpuId), // 0x0
OFFSET(AaCp15_Cr1_Control, KARM_ARCH_STATE, Cp15_Cr1_Control), // 0x4
OFFSET(AaCp15_Cr1_AuxControl, KARM_ARCH_STATE, Cp15_Cr1_AuxControl), // 0x8
OFFSET(AaCp15_Cr1_Cpacr, KARM_ARCH_STATE, Cp15_Cr1_Cpacr), // 0xc
OFFSET(AaCp15_Cr2_TtbControl, KARM_ARCH_STATE, Cp15_Cr2_TtbControl), // 0x10
OFFSET(AaCp15_Cr2_Ttb0, KARM_ARCH_STATE, Cp15_Cr2_Ttb0), // 0x14
OFFSET(AaCp15_Cr2_Ttb1, KARM_ARCH_STATE, Cp15_Cr2_Ttb1), // 0x18
OFFSET(AaCp15_Cr3_Dacr, KARM_ARCH_STATE, Cp15_Cr3_Dacr), // 0x1c
OFFSET(AaCp15_Cr5_Dfsr, KARM_ARCH_STATE, Cp15_Cr5_Dfsr), // 0x20
OFFSET(AaCp15_Cr5_Ifsr, KARM_ARCH_STATE, Cp15_Cr5_Ifsr), // 0x24
OFFSET(AaCp15_Cr6_Dfar, KARM_ARCH_STATE, Cp15_Cr6_Dfar), // 0x28
OFFSET(AaCp15_Cr6_Ifar, KARM_ARCH_STATE, Cp15_Cr6_Ifar), // 0x2c
OFFSET(AaCp15_Cr9_PmControl, KARM_ARCH_STATE, Cp15_Cr9_PmControl), // 0x30
OFFSET(AaCp15_Cr9_PmCountEnableSet, KARM_ARCH_STATE, Cp15_Cr9_PmCountEnableSet), // 0x34
OFFSET(AaCp15_Cr9_PmCycleCounter, KARM_ARCH_STATE, Cp15_Cr9_PmCycleCounter), // 0x38
OFFSET(AaCp15_Cr9_PmEventCounter, KARM_ARCH_STATE, Cp15_Cr9_PmEventCounter), // 0x3c
OFFSET(AaCp15_Cr9_PmEventType, KARM_ARCH_STATE, Cp15_Cr9_PmEventType), // 0xb8
OFFSET(AaCp15_Cr9_PmInterruptSelect, KARM_ARCH_STATE, Cp15_Cr9_PmInterruptSelect), // 0x134
OFFSET(AaCp15_Cr9_PmOverflowStatus, KARM_ARCH_STATE, Cp15_Cr9_PmOverflowStatus), // 0x138
OFFSET(AaCp15_Cr9_PmSelect, KARM_ARCH_STATE, Cp15_Cr9_PmSelect), // 0x13c
OFFSET(AaCp15_Cr9_PmUserEnable, KARM_ARCH_STATE, Cp15_Cr9_PmUserEnable), // 0x140
OFFSET(AaCp15_Cr10_PrimaryMemoryRemap, KARM_ARCH_STATE, Cp15_Cr10_PrimaryMemoryRemap), // 0x144
OFFSET(AaCp15_Cr10_NormalMemoryRemap, KARM_ARCH_STATE, Cp15_Cr10_NormalMemoryRemap), // 0x148
OFFSET(AaCp15_Cr12_VBARns, KARM_ARCH_STATE, Cp15_Cr12_VBARns), // 0x14c
OFFSET(AaCp15_Cr13_ContextId, KARM_ARCH_STATE, Cp15_Cr13_ContextId), // 0x150

HEADER("KSTART_FRAME offsets"),
OFFSET(SfR0, KSTART_FRAME, R0), // 0x0
OFFSET(SfR1, KSTART_FRAME, R1), // 0x4
OFFSET(SfR2, KSTART_FRAME, R2), // 0x8
OFFSET(SfReturn, KSTART_FRAME, Return), // 0xc
SIZE(KSTART_FRAME_LENGTH, KSTART_FRAME), // 0x10

HEADER("KSWITCH_FRAME offsets"),
OFFSET(SwApcBypass, KSWITCH_FRAME, ApcBypass), // 0x0
OFFSET(SwR11, KSWITCH_FRAME, R11), // 0x8
OFFSET(SwReturn, KSWITCH_FRAME, Return), // 0xc
SIZE(KSWITCH_FRAME_LENGTH, KSWITCH_FRAME), // 0x10

HEADER("MACHINE_FRAME offsets"),
OFFSET(MfSp, MACHINE_FRAME, Sp), // 0x0
OFFSET(MfPc, MACHINE_FRAME, Pc), // 0x4
SIZE(MachineFrameLength, MACHINE_FRAME), // 0x8

HEADER("KARM_VFP_STATE offsets"),
OFFSET(VsLink, KARM_VFP_STATE, Link), // 0x0
OFFSET(VsFpscr, KARM_VFP_STATE, Fpscr), // 0x4
OFFSET(VsVfpD, KARM_VFP_STATE, VfpD), // 0x10
OFFSET(VsVfpD8, KARM_VFP_STATE, VfpD[8]), // 0x50
SIZE(VFP_STATE_LENGTH, KARM_VFP_STATE), // 0x110

HEADER("KARM_MINI_STACK offsets"),
OFFSET(MsPc, KARM_MINI_STACK, Pc), // 0x0
OFFSET(MsCpsr, KARM_MINI_STACK, Cpsr), // 0x4
OFFSET(MsR4, KARM_MINI_STACK, R4), // 0x8
OFFSET(MsR5, KARM_MINI_STACK, R5), // 0xc
OFFSET(MsR6, KARM_MINI_STACK, R6), // 0x10
OFFSET(MsR7, KARM_MINI_STACK, R7), // 0x14
OFFSET(MsReserved, KARM_MINI_STACK, Reserved), // 0x18
SIZE(MiniStackLength, KARM_MINI_STACK), // 0x20

HEADER("KPCR offsets"),
OFFSET(PcSelf, KIPCR, Self), //  0xc
OFFSET(PcCurrentPrcb, KIPCR, CurrentPrcb), // 0x10
OFFSET(PcLockArray, KIPCR, LockArray), // 0x14
OFFSET(PcTeb, KIPCR, Used_Self), // 0x18
OFFSET(PcStallScaleFactor, KIPCR, StallScaleFactor), // 0x30
OFFSET(PcHalReserved, KIPCR, HalReserved), // 0x84
OFFSET(PcPrcb, KIPCR, Prcb), // 0x580
OFFSET(PcIdleHalt, KIPCR, Prcb.IdleHalt), // 0x582
OFFSET(PcCurrentThread, KIPCR, Prcb.CurrentThread), // 0x584
OFFSET(PcNextThread, KIPCR, Prcb.NextThread), // 0x588
OFFSET(PcIdleThread, KIPCR, Prcb.IdleThread), // 0x58c
OFFSET(PcNestingLevel, KIPCR, Prcb.NestingLevel), // 0x590
OFFSET(PcNumber, KIPCR, Prcb.Number), // 0x594
OFFSET(PcPrcbLock, KIPCR, Prcb.PrcbLock), // 0x598
OFFSET(PcGroupSetMember, KIPCR, Prcb.GroupSetMember), // 0x998
OFFSET(PcFeatureBits, KIPCR, Prcb.FeatureBits), // 0xa8c
OFFSET(PcDeferredReadyListHead, KIPCR, Prcb.DeferredReadyListHead), // 0xb84
OFFSET(PcSystemCalls, KIPCR, Prcb.KeSystemCalls), // 0xbb0
OFFSET(PcSpBase, KIPCR, Prcb.SpBase), // 0xc44
OFFSET(PcDpcRoutineActive, KIPCR, Prcb.DpcRoutineActive), // 0xc5a
OFFSET(PcInterruptCount, KIPCR, Prcb.InterruptCount), // 0xe80
OFFSET(PcSkipTick, KIPCR, Prcb.SkipTick), // 0xe98
OFFSET(PcDebuggerSavedIRQL, KIPCR, Prcb.DebuggerSavedIRQL), // 0xe99
OFFSET(PcStartCycles, KIPCR, Prcb.StartCycles), // 0xec8
OFFSET(PcCycleCounterHigh, KIPCR, Prcb.CycleCounterHigh), // 0xed8
SIZE(ProcessorControlRegisterLength, KIPCR), // 0x5b80

HEADER("KPRCB offsets"),
OFFSET(PbIdleHalt, KPRCB, IdleHalt), // 0x2
OFFSET(PbCurrentThread, KPRCB, CurrentThread), // 0x4
OFFSET(PbNextThread, KPRCB, NextThread), // 0x8
OFFSET(PbIdleThread, KPRCB, IdleThread), //  0xc
OFFSET(PbNestingLevel, KPRCB, NestingLevel), // 0x10
OFFSET(PbNumber, KPRCB, Number), // 0x14
OFFSET(PbPrcbLock, KPRCB, PrcbLock), // 0x18
OFFSET(PbPriorityState, KPRCB, PriorityState), // 0x1c
OFFSET(PbProcessorState, KPRCB, ProcessorState), // 0x20
OFFSET(PbHalReserved, KPRCB, HalReserved), // 0x3d0
OFFSET(PbMinorVersion, KPRCB, MinorVersion), // 0x40c
OFFSET(PbMajorVersion, KPRCB, MajorVersion), // 0x40e
OFFSET(PbBuildType, KPRCB, BuildType), // 0x410
OFFSET(PbCoresPerPhysicalProcessor, KPRCB, CoresPerPhysicalProcessor), // 0x412
OFFSET(PbLogicalProcessorsPerCore, KPRCB, LogicalProcessorsPerCore), // 0x413
OFFSET(PbGroup, KPRCB, Group), // 0x41c
OFFSET(PbGroupIndex, KPRCB, GroupIndex), // 0x41d
OFFSET(PbLockQueue, KPRCB, LockQueue), // 0x480
OFFSET(PbProcessorVendorString, KPRCB, ProcessorVendorString), // 0x508
OFFSET(PbFeatureBits, KPRCB, FeatureBits), // 0x50c
OFFSET(PbPPLookasideList, KPRCB, PPLookasideList), // 0x580
OFFSET(PbPacketBarrier, KPRCB, PacketBarrier), // 0x600
OFFSET(PbDeferredReadyListHead, KPRCB, DeferredReadyListHead), // 0x604
OFFSET(PbSystemCalls, KPRCB, KeSystemCalls), // 0x630
OFFSET(PbContextSwitches, KPRCB, KeContextSwitches), // 0x634
OFFSET(PbFastReadNoWait, KPRCB, CcFastReadNoWait), // 0x638
OFFSET(PbFastReadWait, KPRCB, CcFastReadWait), // 0x63c
OFFSET(PbFastReadNotPossible, KPRCB, CcFastReadNotPossible), // 0x640
OFFSET(PbCopyReadNoWait, KPRCB, CcCopyReadNoWait), // 0x644
OFFSET(PbCopyReadWait, KPRCB, CcCopyReadWait), // 0x648
OFFSET(PbCopyReadNoWaitMiss, KPRCB, CcCopyReadNoWaitMiss), // 0x64c
OFFSET(PbLookasideIrpFloat, KPRCB, LookasideIrpFloat), // 0x650
OFFSET(PbReadOperationCount, KPRCB, IoReadOperationCount), // 0x654
OFFSET(PbWriteOperationCount, KPRCB, IoWriteOperationCount), // 0x658
OFFSET(PbOtherOperationCount, KPRCB, IoOtherOperationCount), // 0x65c
OFFSET(PbReadTransferCount, KPRCB, IoReadTransferCount), // 0x660
OFFSET(PbWriteTransferCount, KPRCB, IoWriteTransferCount), // 0x668
OFFSET(PbOtherTransferCount, KPRCB, IoOtherTransferCount), // 0x670
OFFSET(PbMailbox, KPRCB, Mailbox), // 0x680
OFFSET(PbIpiFrozen, KPRCB, IpiFrozen), // 0x688
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
OFFSET(PbDpcList, KPRCB, DpcData[0].DpcList), // 0x690
#else
OFFSET(PbDpcListHead, KPRCB, DpcData[0].DpcListHead), // 0x690
#endif
OFFSET(PbDpcLock, KPRCB, DpcData[0].DpcLock), // 0x698
OFFSET(PbDpcCount, KPRCB, DpcData[0].DpcCount), // 0x6a0
OFFSET(PbDpcStack, KPRCB, DpcStack), // 0x6c0
OFFSET(PbSpBase, KPRCB, SpBase), // 0x6c4
OFFSET(PbMaximumDpcQueueDepth, KPRCB, MaximumDpcQueueDepth), // 0x6c8
OFFSET(PbDpcRequestRate, KPRCB, DpcRequestRate), // 0x6cc
OFFSET(PbMinimumDpcRate, KPRCB, MinimumDpcRate), // 0x6d0
OFFSET(PbDpcLastCount, KPRCB, DpcLastCount), // 0x6d4
OFFSET(PbQuantumEnd, KPRCB, QuantumEnd), // 0x6d9
OFFSET(PbDpcRoutineActive, KPRCB, DpcRoutineActive), // 0x6da
OFFSET(PbIdleSchedule, KPRCB, IdleSchedule), // 0x6db
#if (NTDDI_VERSION >= NTDDI_WIN8)
OFFSET(PbDpcRequestSummary, KPRCB, DpcRequestSummary), // 0x6dc
OFFSET(PbNormalDpcState, KPRCB, NormalDpcState), // 0x6dc
OFFSET(PbDpcGate, KPRCB, DpcGate), // 0x700
#else
OFFSET(PbDpcSetEventRequest, KPRCB, DpcSetEventRequest), // 0x700
OFFSET(PbDpcEvent, KPRCB, DpcEvent), // 0x700
#endif
OFFSET(PbKeSpinLockOrdering, KPRCB, KeSpinLockOrdering), // 0x744
OFFSET(PbWaitListHead, KPRCB, WaitListHead), // 0x780
OFFSET(PbDispatcherReadyListHead, KPRCB, DispatcherReadyListHead), // 0x800
OFFSET(PbInterruptCount, KPRCB, InterruptCount), // 0x900
OFFSET(PbKernelTime, KPRCB, KernelTime), // 0x904
OFFSET(PbUserTime, KPRCB, UserTime), // 0x908
OFFSET(PbDpcTime, KPRCB, DpcTime), // 0x90c
OFFSET(PbInterruptTime, KPRCB, InterruptTime), // 0x910
OFFSET(PbAdjustDpcThreshold, KPRCB, AdjustDpcThreshold), // 0x914
OFFSET(PbExceptionDispatchCount, KPRCB, KeExceptionDispatchCount), // 0x934
OFFSET(PbParentNode, KPRCB, ParentNode), // 0x938
OFFSET(PbStartCycles, KPRCB, StartCycles), // 0x948
OFFSET(PbCycleCounterHigh, KPRCB, CycleCounterHigh), // 0x958
#if (NTDDI_VERSION >= NTDDI_WIN8)
OFFSET(PbEntropyCount, KPRCB, EntropyTimingState.EntropyCount), // 0x960
OFFSET(PbEntropyBuffer, KPRCB, EntropyTimingState.Buffer), // 0x964
#endif /* (NTDDI_VERSION >= NTDDI_WIN8) */
OFFSET(PbPageColor, KPRCB, PageColor), // 0xa8c
OFFSET(PbNodeColor, KPRCB, NodeColor), // 0xa90
OFFSET(PbNodeShiftedColor, KPRCB, NodeShiftedColor), // 0xa94
OFFSET(PbSecondaryColorMask, KPRCB, SecondaryColorMask), // 0xa98
OFFSET(PbCycleTime, KPRCB, CycleTime), // 0xaa0
OFFSET(PbCcFastMdlReadNoWait, KPRCB, CcFastMdlReadNoWait), // 0xb00
OFFSET(PbPowerState, KPRCB, PowerState), // 0xb80
OFFSET(PbAlignmentFixupCount, KPRCB, KeAlignmentFixupCount), // 0xd14
OFFSET(PbSpinLockAcquireCount, KPRCB, SynchCounters.SpinLockAcquireCount), // 0xe80
OFFSET(PbFiqMiniStack, KPRCB, FiqMiniStack), // 0xf50
OFFSET(PbIrqMiniStack, KPRCB, IrqMiniStack), // 0xf70
OFFSET(PbUdfMiniStack, KPRCB, UdfMiniStack), // 0xf90
OFFSET(PbAbtMiniStack, KPRCB, AbtMiniStack), // 0xfb0
OFFSET(PbPanicMiniStack, KPRCB, PanicMiniStack), // 0xfd0
OFFSET(PbPanicStackBase, KPRCB, PanicStackBase), // 0xff0
OFFSET(PbPPNPagedLookasideList, KPRCB, PPNPagedLookasideList), // 0x3200
OFFSET(PbPPPagedLookasideList, KPRCB, PPPagedLookasideList), // 0x3b00
//OFFSET(PbRequestMailbox, KPRCB, RequestMailbox), // 0x4600

HEADER("UCALLOUT_FRAME offsets (yes, Cu/Ck is confusing...)"),
OFFSET(CkBuffer, UCALLOUT_FRAME, Buffer),
OFFSET(CkLength, UCALLOUT_FRAME, Length),
OFFSET(CkApiNumber, UCALLOUT_FRAME, ApiNumber),
OFFSET(CkPc, UCALLOUT_FRAME, MachineFrame.Pc),
OFFSET(CkSp, UCALLOUT_FRAME, MachineFrame.Sp),
SIZE(CalloutFrameLength, UCALLOUT_FRAME),

HEADER("KCALLOUT_FRAME offsets (yes, Cu/Ck is confusing...)"),
OFFSET(CuTrapFrame, KCALLOUT_FRAME, TrapFrame),
OFFSET(CuOutputBuffer, KCALLOUT_FRAME, OutputBuffer),
OFFSET(CuOutputLength, KCALLOUT_FRAME, OutputLength),

// Processor Idle Times Offset Definitions
//#define PiStartTime 0x0
//#define PiEndTime 0x8

//#define DBGDSCR_MON_EN_BIT 0x8000
//#define KF_VFP_32REG 0x10
//#define KI_SPINLOCK_ORDER_PRCB_LOCK 0x40
//#define THREAD_FLAGS_CYCLE_PROFILING 0x1
//#define THREAD_FLAGS_CYCLE_PROFILING_LOCK_BIT 0x10
//#define THREAD_FLAGS_CYCLE_PROFILING_LOCK 0x10000
//#define THREAD_FLAGS_COUNTER_PROFILING 0x2
//#define THREAD_FLAGS_COUNTER_PROFILING_LOCK_BIT 0x11
//#define THREAD_FLAGS_COUNTER_PROFILING_LOCK 0x20000
//#define THREAD_FLAGS_GROUP_SCHEDULING 0x4
//#define THREAD_FLAGS_AFFINITY_SET 0x8
//#define THREAD_FLAGS_ACCOUNTING_CSWITCH 0x6
//#define THREAD_FLAGS_ACCOUNTING_ANY 0xe
//#define KTHREAD_AUTO_ALIGNMENT_BIT 0x0
//#define KTHREAD_GUI_THREAD_MASK 0x40
//#define KTHREAD_SYSTEM_THREAD_BIT 0xb
//#define KTHREAD_QUEUE_DEFER_PREEMPTION_BIT 0xa
//#define DEBUG_ACTIVE_DBG 0x1
//#define DEBUG_ACTIVE_DBG_INSTRUMENTED 0x3
//#define DEBUG_ACTIVE_INSTRUMENTED 0x2
//CONSTANT(DEBUG_ACTIVE_MINIMAL_THREAD),

//#define ARM_VFP_MANAGEMENT 0x1
//#define ARM_VFP_ENABLE_STATISTICS 0x0
//#define ARM_VFP_ALWAYSON 0x0
//#define ARM_VFP_LAZY_ONEWAY 0x1
//#define ARM_VFP_LAZY_WITH_DEMOTION 0x2
#define TRAP_TYPE_INTERRUPT 0x1
#define TRAP_TYPE_SYSCALL 0x2
#define TRAP_TYPE_UNDEFINED 0x3
#define TRAP_TYPE_DATA_ABORT 0x4
#define TRAP_TYPE_PREFETCH_ABORT 0x5
#define TRAP_TYPE_RESET 0x6
#define TRAP_TYPE_FIQ 0x7
#define THUMB_BREAKPOINT 0xdefe
#define THUMB_DEBUG_SERVICE 0xdefd
#define THUMB_ASSERT 0xdefc
#define THUMB_FASTFAIL 0xdefb
#define THUMB_READ_CYCLES 0xdefa
#define THUMB_DIVIDE_BY_0 0xdef9
#define ARM_EXCEPTION_VECTOR 0xffff0000 // obsolete in win10
#define KI_DPC_INTERRUPT_FLAGS 0x2f // amd64 as well
#define KI_EXCEPTION_HARDWARE_ERROR 0x10000005
#define KTRAP_FRAME_ARGUMENTS 0x38
#define ARM_RED_ZONE_BYTES 0x8
CONSTANT(PF_ARM_EXTERNAL_CACHE_AVAILABLE),
#define FAST_FAIL_DEPRECATED_SERVICE_INVOKED 0x1b // since win10

#define CP14_DBGBCR_MISMATCH_BIT 0x400000
#define CP14_DBGBCR_ENABLE_BIT 0x1

#define CP15_CPACR_D32DIS 0x80000000
#define CP15_CPACR_ASEDIS 0x40000000
#define CP15_CPACR_VFP_MASK 0xf00000
#define CPVFP_FPEXC_EX 0x80000000
#define CPVFP_FPEXC_EN 0x40000000
#define CPVFP_FPEXC_DEX 0x20000000
#define CPVFP_FPEXC_FP2V 0x10000000

#define CP15_CR0_HARVARD_CACHE 0x1000000
#define CP15_xFSR_FS_HIGH 0x400
#define CP15_xFSR_FS_LOW 0xf
#define CP15_DFSR_WnR 0x800

#define CP15_SCTLR_I 0x1000
#define CP15_SCTLR_C 0x4
#define CP15_SCTLR_M 0x1
#define CP15_SCTLR_Z 0x800
#define CP15_SCTLR_TR 0x10000000 // obsolete in win10
#define CP15_THREAD_RESERVED_MASK 0x3f

// Processor Start Block Offset Definitions
#define PsbSelfMap 0x0
#define PsbTiledTtb0 0x4
#define ProcessorStartBlockLength 0x8

// Processor Parked Page Offset Definitions
#define PppArchitecturalStateVirtualAddress 0x10
#define PppArchitecturalState 0x18
#define PppDcacheFlushSavedRegisters 0x3b8 // obsolete in win10
#define ProcessorParkedPageLength 0x1000

#define TlThread 0x0
#define TlCpuNumber 0x4
#define TlTrapType 0x5
#define TlPadding 0x6
#define TlR0 0x8
#define TlR1 0xc
#define TlR2 0x10
#define TlR3 0x14
#define TlR12 0x18
#define TlSp 0x1c
#define TlLr 0x20
#define TlR11 0x24
#define TlPc 0x28
#define TlCpsr 0x2c

// DPC stack
#define DpSp 0x8
#define DpPc 0xc

// also amd64
#define KEXCEPTION_ACTIVE_INTERRUPT_FRAME 0x0
#define KEXCEPTION_ACTIVE_EXCEPTION_FRAME 0x1
#define KEXCEPTION_ACTIVE_SERVICE_FRAME 0x2



#define CP15_MIDR              15, 0,  0,  0, 0
#define CP15_CTR               15, 0,  0,  0, 1
#define CP15_TCMTR             15, 0,  0,  0, 2
#define CP15_TLBTR             15, 0,  0,  0, 3
#define CP15_MPIDR             15, 0,  0,  0, 5
#define CP15_PFR0              15, 0,  0,  1, 0
#define CP15_PFR1              15, 0,  0,  1, 1
#define CP15_DFR0              15, 0,  0,  1, 2
#define CP15_AFR0              15, 0,  0,  1, 3
#define CP15_MMFR0             15, 0,  0,  1, 4
#define CP15_MMFR1             15, 0,  0,  1, 5
#define CP15_MMFR2             15, 0,  0,  1, 6
#define CP15_MMFR3             15, 0,  0,  1, 7
#define CP15_ISAR0             15, 0,  0,  2, 0
#define CP15_ISAR1             15, 0,  0,  2, 1
#define CP15_ISAR2             15, 0,  0,  2, 2
#define CP15_ISAR3             15, 0,  0,  2, 3
#define CP15_ISAR4             15, 0,  0,  2, 4
#define CP15_ISAR5             15, 0,  0,  2, 5
#define CP15_ISAR6             15, 0,  0,  2, 6
#define CP15_ISAR7             15, 0,  0,  2, 7
#define CP15_SCTLR             15, 0,  1,  0, 0
#define CP15_ACTLR             15, 0,  1,  0, 1
#define CP15_CPACR             15, 0,  1,  0, 2
#define CP15_SCR               15, 0,  1,  1, 0
#define CP15_SDER              15, 0,  1,  1, 1
#define CP15_NSACR             15, 0,  1,  1, 2
#define CP15_TTBR0             15, 0,  2,  0, 0
#define CP15_TTBR1             15, 0,  2,  0, 1
#define CP15_TTBCR             15, 0,  2,  0, 2
#define CP15_DACR              15, 0,  3,  0, 0
#define CP15_DFSR              15, 0,  5,  0, 0
#define CP15_IFSR              15, 0,  5,  0, 1
#define CP15_DFAR              15, 0,  6,  0, 0
#define CP15_IFAR              15, 0,  6,  0, 2
#define CP15_ICIALLUIS         15, 0,  7,  1, 0
#define CP15_BPIALLIS          15, 0,  7,  1, 6
#define CP15_ICIALLU           15, 0,  7,  5, 0
#define CP15_ICIMVAU           15, 0,  7,  5, 1
#define CP15_BPIALL            15, 0,  7,  5, 6
#define CP15_BPIMVA            15, 0,  7,  5, 7
#define CP15_DCIMVAC           15, 0,  7,  6, 1
#define CP15_DCISW             15, 0,  7,  6, 2
#define CP15_DCCMVAC           15, 0,  7, 10, 1
#define CP15_DCCSW             15, 0,  7, 10, 2
#define CP15_DCCMVAU           15, 0,  7, 11, 1
#define CP15_DCCIMVAC          15, 0,  7, 14, 1
#define CP15_DCCISW            15, 0,  7, 14, 2
#define CP15_PAR               15, 0,  7,  4, 0
#define CP15_ATS1CPR           15, 0,  7,  8, 0
#define CP15_ATS1CPW           15, 0,  7,  8, 1
#define CP15_ATS1CUR           15, 0,  7,  8, 2
#define CP15_ATS1CUW           15, 0,  7,  8, 3
#define CP15_ISB               15, 0,  7,  5, 4
#define CP15_DSB               15, 0,  7, 10, 4
#define CP15_DMB               15, 0,  7, 10, 5
#define CP15_TLBIALLIS         15, 0,  8,  3, 0
#define CP15_TLBIMVAIS         15, 0,  8,  3, 1
#define CP15_TLBIASIDIS        15, 0,  8,  3, 2
#define CP15_TLBIMVAAIS        15, 0,  8,  3, 3
#define CP15_ITLBIALL          15, 0,  8,  5, 0
#define CP15_ITLBIMVA          15, 0,  8,  5, 1
#define CP15_ITLBIASID         15, 0,  8,  5, 2
#define CP15_DTLBIALL          15, 0,  8,  6, 0
#define CP15_DTLBIMVA          15, 0,  8,  6, 1
#define CP15_DTLBIASID         15, 0,  8,  6, 2
#define CP15_TLBIALL           15, 0,  8,  7, 0
#define CP15_TLBIMVA           15, 0,  8,  7, 1
#define CP15_TLBIASID          15, 0,  8,  7, 2
#define CP15_TLBIMVAA          15, 0,  8,  7, 3
#define CP15_PMCR              15, 0,  9, 12, 0
#define CP15_PMCNTENSET        15, 0,  9, 12, 1
#define CP15_PMCNTENCLR        15, 0,  9, 12, 2
#define CP15_PMOVSR            15, 0,  9, 12, 3
#define CP15_PSWINC            15, 0,  9, 12, 4
#define CP15_PMSELR            15, 0,  9, 12, 5
#define CP15_PMCCNTR           15, 0,  9, 13, 0
#define CP15_PMXEVTYPER        15, 0,  9, 13, 1
#define CP15_PMXEVCNTR         15, 0,  9, 13, 2
#define CP15_PMUSERENR         15, 0,  9, 14, 0
#define CP15_PMINTENSET        15, 0,  9, 14, 1
#define CP15_PMINTENCLR        15, 0,  9, 14, 2
#define CP15_PRRR              15, 0, 10,  2, 0
#define CP15_NMRR              15, 0, 10,  2, 1
#define CP15_VBAR              15, 0, 12,  0, 0
#define CP15_MVBAR             15, 0, 12,  0, 1
#define CP15_ISR               15, 0, 12,  1, 0
#define CP15_CONTEXTIDR        15, 0, 13,  0, 1
#define CP15_TPIDRURW          15, 0, 13,  0, 2
#define CP15_TPIDRURO          15, 0, 13,  0, 3
#define CP15_TPIDRPRW          15, 0, 13,  0, 4
#define CP15_CCSIDR            15, 1,  0,  0, 0
#define CP15_CLIDR             15, 1,  0,  0, 1
#define CP15_AIDR              15, 1,  0,  0, 7
#define CP15_CSSELR            15, 2,  0,  0, 0
#define CP14_DBGDIDR           14, 0,  0,  0, 0
#define CP14_DBGWFAR           14, 0,  0,  6, 0
#define CP14_DBGVCR            14, 0,  0,  7, 0
#define CP14_DBGECR            14, 0,  0,  9, 0
#define CP14_DBGDSCCR          14, 0,  0, 10, 0
#define CP14_DBGDSMCR          14, 0,  0, 11, 0
#define CP14_DBGDTRRX          14, 0,  0,  0, 2
#define CP14_DBGPCSR           14, 0,  0,  1, 2
#define CP14_DBGITR            14, 0,  0,  1, 2
#define CP14_DBGDSCR           14, 0,  0,  2, 2
#define CP14_DBGDTRTX          14, 0,  0,  3, 2
#define CP14_DBGDRCR           14, 0,  0,  4, 2
#define CP14_DBGCIDSR          14, 0,  0,  9, 2
#define CP14_DBGBVR0           14, 0,  0,  0, 4
#define CP14_DBGBVR1           14, 0,  0,  1, 4
#define CP14_DBGBVR2           14, 0,  0,  2, 4
#define CP14_DBGBVR3           14, 0,  0,  3, 4
#define CP14_DBGBVR4           14, 0,  0,  4, 4
#define CP14_DBGBVR5           14, 0,  0,  5, 4
#define CP14_DBGBVR6           14, 0,  0,  6, 4
#define CP14_DBGBVR7           14, 0,  0,  7, 4
#define CP14_DBGBCR0           14, 0,  0,  0, 5
#define CP14_DBGBCR1           14, 0,  0,  1, 5
#define CP14_DBGBCR2           14, 0,  0,  2, 5
#define CP14_DBGBCR3           14, 0,  0,  3, 5
#define CP14_DBGBCR4           14, 0,  0,  4, 5
#define CP14_DBGBCR5           14, 0,  0,  5, 5
#define CP14_DBGBCR6           14, 0,  0,  6, 5
#define CP14_DBGBCR7           14, 0,  0,  7, 5
#define CP14_DBGWVR0           14, 0,  0,  0, 6
#define CP14_DBGWVR1           14, 0,  0,  1, 6
#define CP14_DBGWVR2           14, 0,  0,  2, 6
#define CP14_DBGWVR3           14, 0,  0,  3, 6
#define CP14_DBGWCR0           14, 0,  0,  0, 7
#define CP14_DBGWCR1           14, 0,  0,  1, 7
#define CP14_DBGWCR2           14, 0,  0,  2, 7
#define CP14_DBGWCR3           14, 0,  0,  3, 7
#define CPVFP_FPSID            10, 7,  0,  0, 0
#define CPVFP_FPSCR            10, 7,  1,  0, 0
#define CPVFP_MVFR1            10, 7,  6,  0, 0
#define CPVFP_MVFR0            10, 7,  7,  0, 0
#define CPVFP_FPEXC            10, 7,  8,  0, 0
#define CP15_TTBRx_PD_MASK 0xffffc000



