/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    genoff.c

Abstract:

    This module implements a program which generates structure offset
    definitions for kernel structures that are accessed in assembly code.

Author:

    Bryan M. Willman (bryanwi) 16-Oct-90


To build:

    in ke\up do a "nmake UMAPPL=geni386"
    watch out for precompiled headers


Revision History:

    Forrest Foltz (forrestf) 24-Jan-1998

        Modified format to use new obj-based procedure.

--*/


#include "ki.h"
#pragma hdrstop

#include "nturtl.h"
#include "vdmntos.h"
#include "abios.h"

include(`..\genxx.h')

#define KS386 KERNEL
#define HAL386 HAL

STRUC_ELEMENT ElementList[] = {

    START_LIST

  EnableInc(KS386)

    #include "genxx.inc"

    //
    // Generate architecture dependent definitions.
    //

    genCom("Apc Record Structure Offset Definitions")

    genDef(Ar, KAPC_RECORD, NormalRoutine)
    genDef(Ar, KAPC_RECORD, NormalContext)
    genDef(Ar, KAPC_RECORD, SystemArgument1)
    genDef(Ar, KAPC_RECORD, SystemArgument2)
    genVal(ApcRecordLength, sizeof(KAPC_RECORD))
    genSpc()

  EnableInc(HAL386)
    genCom("Processor Control Registers Structure Offset Definitions")

    genNam( KI_BEGIN_KERNEL_RESERVED )
    genTxt("ifdef NT_UP\n")
    genStr("    P0PCRADDRESS equ 0%lXH\n", KIP0PCRADDRESS)
    genStr("    PCR equ ds:[0%lXH]\n", KIP0PCRADDRESS)
    genTxt("else\n")
    genTxt("    PCR equ fs:\n")
    genTxt("endif\n\n")

    genAlt(PcExceptionList, KPCR,  NtTib.ExceptionList)
    genAlt(PcInitialStack, KPCR, NtTib.StackBase)
    genAlt(PcStackLimit, KPCR, NtTib.StackLimit)
    genDef(Pc, KPCR, SelfPcr)
    genDef(Pc, KPCR, Prcb)
    genAlt(PcTeb, KPCR, NtTib.Self)
    genDef(Pc, KPCR, Irql)
    genDef(Pc, KPCR, IRR)
    genDef(Pc, KPCR, IrrActive)
    genDef(Pc, KPCR, IDR)
    genAlt(PcIdt, KPCR, IDT)
    genAlt(PcGdt, KPCR, GDT)
    genAlt(PcTss, KPCR, TSS)
    genDef(Pc, KPCR, DebugActive)
    genDef(Pc, KPCR, Number)
    genDef(Pc, KPCR, VdmAlert)
    genDef(Pc, KPCR, SetMember)
    genDef(Pc, KPCR, StallScaleFactor)
    genAlt(PcHal, KPCR, HalReserved)
  DisableInc (HAL386)
    genDef(Pc, KPCR, PrcbData)
    genVal(ProcessorControlRegisterLength, sizeof(KPCR))
    genAlt(TebPeb, TEB, ProcessEnvironmentBlock)
    genDef(Peb, PEB, BeingDebugged)
    genDef(Peb, PEB, KernelCallbackTable)

  EnableInc (HAL386)
    genCom("Defines for user shared data")

    genVal(USER_SHARED_DATA, KI_USER_SHARED_DATA)
    genNam(MM_SHARED_USER_DATA_VA)
    genStr("USERDATA equ ds:[0%lXH]\n", KI_USER_SHARED_DATA)
    genDef(Us, KUSER_SHARED_DATA, TickCountLow)
    genDef(Us, KUSER_SHARED_DATA, TickCountMultiplier)
    genDef(Us, KUSER_SHARED_DATA, InterruptTime)
    genDef(Us, KUSER_SHARED_DATA, SystemTime)

    genCom("Tss Structure Offset Definitions")

    genDef(Tss, KTSS, Esp0)
    genDef(Tss, KTSS, CR3)
    genDef(Tss, KTSS, IoMapBase)
    genDef(Tss, KTSS, IoMaps)
    genVal(TssLength, sizeof(KTSS))
  DisableInc (HAL386)

  EnableInc (HAL386)
    genCom("Gdt Descriptor Offset Definitions")

    genNam(KGDT_R3_DATA)
    genNam(KGDT_R3_CODE)
    genNam(KGDT_R0_CODE)
    genNam(KGDT_R0_DATA)
    genNam(KGDT_R0_PCR)
    genNam(KGDT_STACK16)
    genNam(KGDT_CODE16)
    genNam(KGDT_TSS)
  DisableInc (HAL386)
    genNam(KGDT_R3_TEB)
    genNam(KGDT_DF_TSS)
    genNam(KGDT_NMI_TSS)
    genNam(KGDT_LDT)

  EnableInc (HAL386)
    genCom("GdtEntry Offset Definitions")

    genDef(Kgdt, KGDTENTRY, BaseLow)
    genAlt(KgdtBaseMid, KGDTENTRY, HighWord.Bytes.BaseMid)
    genAlt(KgdtBaseHi, KGDTENTRY, HighWord.Bytes.BaseHi)
    genAlt(KgdtLimitHi, KGDTENTRY, HighWord.Bytes.Flags2)
    genDef(Kgdt, KGDTENTRY, LimitLow)
    genSpc()

    //
    // Processor block structure definitions.
    //

    genCom("Processor Block Structure Offset Definitions")

    genDef(Pb, KPRCB, CurrentThread)
    genDef(Pb, KPRCB, NextThread)
    genDef(Pb, KPRCB, IdleThread)
    genDef(Pb, KPRCB, Number)
    genDef(Pb, KPRCB, SetMember)
    genDef(Pb, KPRCB, CpuID)
    genDef(Pb, KPRCB, CpuType)
    genDef(Pb, KPRCB, CpuStep)
    genDef(Pb, KPRCB, HalReserved)
    genDef(Pb, KPRCB, ProcessorState)
    genDef(Pb, KPRCB, LockQueue)

  DisableInc (HAL386)

    genDef(Pb, KPRCB, NpxThread)
    genDef(Pb, KPRCB, InterruptCount)
    genDef(Pb, KPRCB, KernelTime)
    genDef(Pb, KPRCB, UserTime)
    genDef(Pb, KPRCB, DpcTime)
    genDef(Pb, KPRCB, InterruptTime)
    genDef(Pb, KPRCB, ApcBypassCount)
    genDef(Pb, KPRCB, DpcBypassCount)
    genDef(Pb, KPRCB, AdjustDpcThreshold)
    genDef(Pb, KPRCB, DebugDpcTime)
    genDef(Pb, KPRCB, ThreadStartCount)
    genAlt(PbAlignmentFixupCount, KPRCB, KeAlignmentFixupCount)
    genAlt(PbContextSwitches, KPRCB, KeContextSwitches)
    genAlt(PbDcacheFlushCount, KPRCB, KeDcacheFlushCount)
    genAlt(PbExceptionDispatchCount, KPRCB, KeExceptionDispatchCount)
    genAlt(PbFirstLevelTbFills, KPRCB, KeFirstLevelTbFills)
    genAlt(PbFloatingEmulationCount, KPRCB, KeFloatingEmulationCount)
    genAlt(PbIcacheFlushCount, KPRCB, KeIcacheFlushCount)
    genAlt(PbSecondLevelTbFills, KPRCB, KeSecondLevelTbFills)
    genAlt(PbSystemCalls, KPRCB, KeSystemCalls)
    genDef(Pb, KPRCB, CurrentPacket)
    genDef(Pb, KPRCB, TargetSet)
    genDef(Pb, KPRCB, WorkerRoutine)
    genDef(Pb, KPRCB, IpiFrozen)
    genDef(Pb, KPRCB, RequestSummary)
    genDef(Pb, KPRCB, SignalDone)
    genDef(Pb, KPRCB, IpiFrame)
    genDef(Pb, KPRCB, DpcInterruptRequested)
    genDef(Pb, KPRCB, MaximumDpcQueueDepth)
    genDef(Pb, KPRCB, MinimumDpcRate)
    genDef(Pb, KPRCB, DpcListHead)
    genDef(Pb, KPRCB, DpcQueueDepth)
    genDef(Pb, KPRCB, DpcRoutineActive)
    genDef(Pb, KPRCB, DpcCount)
    genDef(Pb, KPRCB, DpcLastCount)
    genDef(Pb, KPRCB, DpcRequestRate)
    genDef(Pb, KPRCB, DpcStack)
    genDef(Pb, KPRCB, DpcLock)
    genDef(Pb, KPRCB, SkipTick)
    genDef(Pb, KPRCB, QuantumEnd)
    genDef(Pb, KPRCB, PowerState)
    genVal(ProcessorBlockLength, ((sizeof(KPRCB) + 15) & ~15))

    //
    // Prcb power state
    //

    genCom("Processor Power State Offset Definitions")
    genDef(Pp, PROCESSOR_POWER_STATE, IdleFunction)

    //
    // Interprocessor command definitions.
    //

    genCom("Immediate Interprocessor Command Definitions")

    genVal(IPI_APC, IPI_APC)
    genVal(IPI_DPC, IPI_DPC)
    genVal(IPI_FREEZE, IPI_FREEZE)
    genVal(IPI_PACKET_READY, IPI_PACKET_READY)
    genVal(IPI_SYNCH_REQUEST, IPI_SYNCH_REQUEST)

    genCom("Thread Environment Block Structure Offset Definitions")

    genAlt(TbExceptionList, TEB, NtTib.ExceptionList)
    genAlt(TbStackBase, TEB, NtTib.StackBase)
    genAlt(TbStackLimit, TEB, NtTib.StackLimit)
    genDef(Tb, TEB, EnvironmentPointer)
    genAlt(TbVersion, TEB, NtTib.Version)
    genAlt(TbFiberData, TEB, NtTib.FiberData)
    genAlt(TbArbitraryUserPointer, TEB, NtTib.ArbitraryUserPointer)
    genDef(Tb, TEB, ClientId)
    genDef(Tb, TEB, ThreadLocalStoragePointer)
    genDef(Tb, TEB, CountOfOwnedCriticalSections)
    genDef(Tb, TEB, SystemReserved1)
    genDef(Tb, TEB, Vdm)
    genDef(Tb, TEB, CsrClientThread)
    genDef(Tb, TEB, GdiThreadLocalInfo)
    genDef(Tb, TEB, glDispatchTable)
    genDef(Tb, TEB, glSectionInfo)
    genDef(Tb, TEB, glSection)
    genDef(Tb, TEB, glTable)
    genDef(Tb, TEB, glCurrentRC)
    genDef(Tb, TEB, glContext)
    genDef(Tb, TEB, WOW32Reserved)
    genDef(Tb, TEB, ExceptionCode)
    genDef(Tb, TEB, DeallocationStack)
    genDef(Tb, TEB, GdiBatchCount)

  EnableInc (HAL386)
    genCom("Time Fields (TIME_FIELDS) Structure Offset Definitions")
    genDef(Tf, TIME_FIELDS, Second)
    genDef(Tf, TIME_FIELDS, Minute)
    genDef(Tf, TIME_FIELDS, Hour)
    genDef(Tf, TIME_FIELDS, Weekday)
    genDef(Tf, TIME_FIELDS, Day)
    genDef(Tf, TIME_FIELDS, Month)
    genDef(Tf, TIME_FIELDS, Year)
    genDef(Tf, TIME_FIELDS, Milliseconds)
    genSpc()
  DisableInc (HAL386)

  EnableInc (HAL386)
    genCom("constants for system irql and IDT vector conversion")

    genNam(MAXIMUM_IDTVECTOR)
    genNam(MAXIMUM_PRIMARY_VECTOR)
    genNam(PRIMARY_VECTOR_BASE)
    genNam(RPL_MASK)
    genNam(MODE_MASK)

    genCom("Flags in the CR0 register")

    genNam(CR0_PG)
    genNam(CR0_ET)
    genNam(CR0_TS)
    genNam(CR0_EM)
    genNam(CR0_MP)
    genNam(CR0_PE)
    genNam(CR0_CD)
    genNam(CR0_NW)
    genNam(CR0_AM)
    genNam(CR0_WP)
    genNam(CR0_NE)

    genCom("Flags in the CR4 register")

    genNam(CR4_VME)
    genNam(CR4_PVI)
    genNam(CR4_TSD)
    genNam(CR4_DE)
    genNam(CR4_PSE)
    genNam(CR4_PAE)
    genNam(CR4_MCE)
    genNam(CR4_PGE)
    genNam(CR4_FXSR)
    genNam(CR4_XMMEXCPT)

    genCom("Miscellaneous Definitions")

    genNam(MAXIMUM_PROCESSORS)
    genNam(INITIAL_STALL_COUNT)
    genNam(IRQL_NOT_GREATER_OR_EQUAL)
    genNam(IRQL_NOT_LESS_OR_EQUAL)
    genNam(MUTEX_ALREADY_OWNED)
    genNam(THREAD_NOT_MUTEX_OWNER)
    genNam(SPIN_LOCK_ALREADY_OWNED)
    genNam(SPIN_LOCK_NOT_OWNED)
  DisableInc (HAL386)
    genNam(BASE_PRIORITY_THRESHOLD)
    genNam(EVENT_PAIR_INCREMENT)
    genNam(LOW_REALTIME_PRIORITY)
    genVal(BlackHole, 0xffffa000)
    genNam(KERNEL_LARGE_STACK_COMMIT)
    genNam(KERNEL_STACK_SIZE)
    genNam(DOUBLE_FAULT_STACK_SIZE)
    genNam(EFLAG_SELECT)
    genNam(BREAKPOINT_BREAK )
    genNam(IPI_FREEZE)
    genNam(CLOCK_QUANTUM_DECREMENT)
    genNam(READY_SKIP_QUANTUM)
    genNam(THREAD_QUANTUM)
    genNam(WAIT_QUANTUM_DECREMENT)
    genNam(ROUND_TRIP_DECREMENT_COUNT)

    //
    // Print trap frame offsets relative to sp.
    //

  EnableInc (HAL386)
    genCom("Trap Frame Offset Definitions and Length")

    genDef(Ts, KTRAP_FRAME, ExceptionList)
    genDef(Ts, KTRAP_FRAME, PreviousPreviousMode)
    genDef(Ts, KTRAP_FRAME, SegGs)
    genDef(Ts, KTRAP_FRAME, SegFs)
    genDef(Ts, KTRAP_FRAME, SegEs)
    genDef(Ts, KTRAP_FRAME, SegDs)
    genDef(Ts, KTRAP_FRAME, Edi)
    genDef(Ts, KTRAP_FRAME, Esi)
    genDef(Ts, KTRAP_FRAME, Ebp)
    genDef(Ts, KTRAP_FRAME, Ebx)
    genDef(Ts, KTRAP_FRAME, Edx)
    genDef(Ts, KTRAP_FRAME, Ecx)
    genDef(Ts, KTRAP_FRAME, Eax)
    genDef(Ts, KTRAP_FRAME, ErrCode)
    genDef(Ts, KTRAP_FRAME, Eip)
    genDef(Ts, KTRAP_FRAME, SegCs)
    genAlt(TsEflags, KTRAP_FRAME, EFlags)
    genDef(Ts, KTRAP_FRAME, HardwareEsp)
    genDef(Ts, KTRAP_FRAME, HardwareSegSs)
    genDef(Ts, KTRAP_FRAME, TempSegCs)
    genDef(Ts, KTRAP_FRAME, TempEsp)
    genDef(Ts, KTRAP_FRAME, DbgEbp)
    genDef(Ts, KTRAP_FRAME, DbgEip)
    genDef(Ts, KTRAP_FRAME, DbgArgMark)
    genDef(Ts, KTRAP_FRAME, DbgArgPointer)
    genDef(Ts, KTRAP_FRAME, Dr0)
    genDef(Ts, KTRAP_FRAME, Dr1)
    genDef(Ts, KTRAP_FRAME, Dr2)
    genDef(Ts, KTRAP_FRAME, Dr3)
    genDef(Ts, KTRAP_FRAME, Dr6)
    genDef(Ts, KTRAP_FRAME, Dr7)
    genDef(Ts, KTRAP_FRAME, V86Es)
    genDef(Ts, KTRAP_FRAME, V86Ds)
    genDef(Ts, KTRAP_FRAME, V86Fs)
    genDef(Ts, KTRAP_FRAME, V86Gs)
    genNam(KTRAP_FRAME_LENGTH)
    genNam(KTRAP_FRAME_ALIGN)
    genNam(FRAME_EDITED)
    genNam(EFLAGS_ALIGN_CHECK)
    genNam(EFLAGS_V86_MASK)
    genNam(EFLAGS_INTERRUPT_MASK)
    genNam(EFLAGS_VIF)
    genNam(EFLAGS_VIP)
    genNam(EFLAGS_USER_SANITIZE)

    genCom("Context Frame Offset and Flag Definitions")

    genNam(CONTEXT_FULL)
    genNam(CONTEXT_DEBUG_REGISTERS)
    genNam(CONTEXT_CONTROL)
    genNam(CONTEXT_FLOATING_POINT)
    genNam(CONTEXT_INTEGER)
    genNam(CONTEXT_SEGMENTS)
    genSpc()

    //
    // Print context frame offsets relative to sp.
    //

    genDef(Cs, CONTEXT, ContextFlags)
    genDef(Cs, CONTEXT, FloatSave)
    genDef(Cs, CONTEXT, SegGs)
    genDef(Cs, CONTEXT, SegFs)
    genDef(Cs, CONTEXT, SegEs)
    genDef(Cs, CONTEXT, SegDs)
    genDef(Cs, CONTEXT, Edi)
    genDef(Cs, CONTEXT, Esi)
    genDef(Cs, CONTEXT, Ebp)
    genDef(Cs, CONTEXT, Ebx)
    genDef(Cs, CONTEXT, Edx)
    genDef(Cs, CONTEXT, Ecx)
    genDef(Cs, CONTEXT, Eax)
    genDef(Cs, CONTEXT, Eip)
    genDef(Cs, CONTEXT, SegCs)
    genAlt(CsEflags, CONTEXT, EFlags)
    genDef(Cs, CONTEXT, Esp)
    genDef(Cs, CONTEXT, SegSs)
    genDef(Cs, CONTEXT, Dr0)
    genDef(Cs, CONTEXT, Dr1)
    genDef(Cs, CONTEXT, Dr2)
    genDef(Cs, CONTEXT, Dr3)
    genDef(Cs, CONTEXT, Dr6)
    genDef(Cs, CONTEXT, Dr7)
    genVal(ContextFrameLength, ROUND_UP(sizeof(CONTEXT), 16))
    genNam(DR6_LEGAL)
    genNam(DR7_LEGAL)
    genNam(DR7_ACTIVE)

    //
    // Print Registration Record Offsets relative to base
    //

    genDef(Err, EXCEPTION_REGISTRATION_RECORD, Handler)
    genDef(Err, EXCEPTION_REGISTRATION_RECORD, Next)

    //
    // Print floating point field offsets relative to Context.FloatSave
    //

    genCom("Floating save area field offset definitions")

    genDef(Fp, FLOATING_SAVE_AREA, ControlWord)
    genDef(Fp, FLOATING_SAVE_AREA, StatusWord)
    genDef(Fp, FLOATING_SAVE_AREA, TagWord)
    genDef(Fp, FLOATING_SAVE_AREA, ErrorOffset)
    genDef(Fp, FLOATING_SAVE_AREA, ErrorSelector)
    genDef(Fp, FLOATING_SAVE_AREA, DataOffset)
    genDef(Fp, FLOATING_SAVE_AREA, DataSelector)
    genDef(Fp, FLOATING_SAVE_AREA, RegisterArea)
    genDef(FpCtxt, FLOATING_SAVE_AREA, Cr0NpxState)

    //
    // Print floating point field offsets relative to Kernel Stack
    // The floating save area on kernel stack is bigger for FXSR mode
    //

    genCom("FX Floating save area field offset definitions")

    genDef(Fx, FXSAVE_FORMAT, ControlWord)
    genDef(Fx, FXSAVE_FORMAT, StatusWord)
    genDef(Fx, FXSAVE_FORMAT, TagWord)
    genDef(Fx, FXSAVE_FORMAT, ErrorOpcode)
    genDef(Fx, FXSAVE_FORMAT, ErrorOffset)
    genDef(Fx, FXSAVE_FORMAT, ErrorSelector)
    genDef(Fx, FXSAVE_FORMAT, DataOffset)
    genDef(Fx, FXSAVE_FORMAT, DataSelector)
    genDef(Fx, FXSAVE_FORMAT, MXCsr)
    genDef(FxFp, FXSAVE_FORMAT, RegisterArea)
    genDef(Fp, FX_SAVE_AREA,  NpxSavedCpu)
    genDef(Fp, FX_SAVE_AREA,  Cr0NpxState)

    genSpc()
    genVal(NPX_FRAME_LENGTH, sizeof(FX_SAVE_AREA))

    //
    // Processor State Frame offsets relative to base
    //

    genCom("Processor State Frame Offset Definitions\n")

    genDef(Ps, KPROCESSOR_STATE, ContextFrame)
    genDef(Ps, KPROCESSOR_STATE, SpecialRegisters)
    genDef(Sr, KSPECIAL_REGISTERS, Cr0)
    genDef(Sr, KSPECIAL_REGISTERS, Cr2)
    genDef(Sr, KSPECIAL_REGISTERS, Cr3)
    genDef(Sr, KSPECIAL_REGISTERS, Cr4)
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr0)
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr1)
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr2)
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr3)
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr6)
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr7)
    genAlt(SrGdtr, KSPECIAL_REGISTERS, Gdtr.Limit)

    genAlt(SrIdtr, KSPECIAL_REGISTERS, Idtr.Limit)
    genDef(Sr, KSPECIAL_REGISTERS, Tr)
    genDef(Sr, KSPECIAL_REGISTERS, Ldtr)
    genVal(ProcessorStateLength, ROUND_UP(sizeof(KPROCESSOR_STATE), 15))
  DisableInc (HAL386)

    //
    // E Process fields relative to base
    //

    genCom("EPROCESS")

    genDef(Ep, EPROCESS, DebugPort)
    genDef(Ep, EPROCESS, VdmObjects)

    //
    // E Resource fields relative to base
    //

    genCom("NTDDK Resource")

    genDef(Rs, NTDDK_ERESOURCE, OwnerThreads)
    genDef(Rs, NTDDK_ERESOURCE, OwnerCounts)
    genDef(Rs, NTDDK_ERESOURCE, TableSize)
    genDef(Rs, NTDDK_ERESOURCE, ActiveCount)
    genDef(Rs, NTDDK_ERESOURCE, Flag)
    genDef(Rs, NTDDK_ERESOURCE, InitialOwnerThreads)
    genVal(RsOwnedExclusive, ResourceOwnedExclusive)

    //
    // Define machine type (temporarily)
    //

  EnableInc (HAL386)

    genCom("Machine type definitions (Temporarily)")

    genNam(MACHINE_TYPE_ISA)
    genNam(MACHINE_TYPE_EISA)
    genNam(MACHINE_TYPE_MCA)

  DisableInc (HAL386)

    genCom("KeFeatureBits defines")

    genNam(KF_V86_VIS)
    genNam(KF_RDTSC)
    genNam(KF_CR4)
    genNam(KF_GLOBAL_PAGE)
    genNam(KF_LARGE_PAGE)
    genNam(KF_CMPXCHG8B)
    genNam(KF_FAST_SYSCALL)

  EnableInc (HAL386)
    genCom("LoaderParameterBlock offsets relative to base")

    genDef(Lpb, LOADER_PARAMETER_BLOCK, LoadOrderListHead)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, MemoryDescriptorListHead)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, KernelStack)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, Prcb)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, Process)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, Thread)
    genAlt(LpbI386, LOADER_PARAMETER_BLOCK, u.I386)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, RegistryLength)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, RegistryBase)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, ConfigurationRoot)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, ArcBootDeviceName)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, ArcHalDeviceName)
  DisableInc (HAL386)

    genNam(PAGE_SIZE)

    //
    // Define the VDM instruction emulation count indexes
    //

    genCom("VDM equates.")

    genNam(VDM_INDEX_Invalid)
    genNam(VDM_INDEX_0F)
    genNam(VDM_INDEX_ESPrefix)
    genNam(VDM_INDEX_CSPrefix)
    genNam(VDM_INDEX_SSPrefix)
    genNam(VDM_INDEX_DSPrefix)
    genNam(VDM_INDEX_FSPrefix)
    genNam(VDM_INDEX_GSPrefix)
    genNam(VDM_INDEX_OPER32Prefix)
    genNam(VDM_INDEX_ADDR32Prefix)
    genNam(VDM_INDEX_INSB)
    genNam(VDM_INDEX_INSW)
    genNam(VDM_INDEX_OUTSB)
    genNam(VDM_INDEX_OUTSW)
    genNam(VDM_INDEX_PUSHF)
    genNam(VDM_INDEX_POPF)
    genNam(VDM_INDEX_INTnn)
    genNam(VDM_INDEX_INTO)
    genNam(VDM_INDEX_IRET)
    genNam(VDM_INDEX_NPX)
    genNam(VDM_INDEX_INBimm)
    genNam(VDM_INDEX_INWimm)
    genNam(VDM_INDEX_OUTBimm)
    genNam(VDM_INDEX_OUTWimm)
    genNam(VDM_INDEX_INB)
    genNam(VDM_INDEX_INW)
    genNam(VDM_INDEX_OUTB)
    genNam(VDM_INDEX_OUTW)
    genNam(VDM_INDEX_LOCKPrefix)
    genNam(VDM_INDEX_REPNEPrefix)
    genNam(VDM_INDEX_REPPrefix)
    genNam(VDM_INDEX_CLI)
    genNam(VDM_INDEX_STI)
    genNam(VDM_INDEX_HLT)
    genNam(MAX_VDM_INDEX)

    //
    // Vdm feature bits
    //

    genCom("VDM feature bits.")

    genNam(V86_VIRTUAL_INT_EXTENSIONS)
    genNam(PM_VIRTUAL_INT_EXTENSIONS)

    //
    // Selector type
    //

    genCom("Selector types.")

    genNam(SEL_TYPE_NP)

    //
    // Usermode callout frame
    //
  DisableInc (HAL386)
    genCom("Usermode callout frame definitions")
    genDef(Cu, KCALLOUT_FRAME, InStk)
    genDef(Cu, KCALLOUT_FRAME, TrFr)
    genDef(Cu, KCALLOUT_FRAME, CbStk)
    genDef(Cu, KCALLOUT_FRAME, Edi)
    genDef(Cu, KCALLOUT_FRAME, Esi)
    genDef(Cu, KCALLOUT_FRAME, Ebx)
    genDef(Cu, KCALLOUT_FRAME, Ebp)
    genDef(Cu, KCALLOUT_FRAME, Ret)
    genDef(Cu, KCALLOUT_FRAME, OutBf)
    genDef(Cu, KCALLOUT_FRAME, OutLn)

    //
    // VDM_PROCESS_OBJECTS
    //

    genCom("VDM_PROCESS_OBJECTS")

    genDef(Vp, VDM_PROCESS_OBJECTS, VdmTib)

  EnableInc (HAL386)

    END_LIST
};
