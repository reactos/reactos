/*++
  Copyright (c) 1990  Microsoft Corporation
  Copyright (c) 1992, 1993  Digital Equipment Corporation

Module Name:

    genalpha.c

Abstract:

    This module implements a program which generates ALPHA machine dependent
    structure offset definitions for kernel structures that are accessed in
    assembly code.

Author:

    David N. Cutler (davec) 27-Mar-1990
    Joe Notarangelo 26-Mar-1992

Revision History:

    Thomas Van Baak (tvb) 10-Jul-1992

        Modified CONTEXT, TRAP, and EXCEPTION frames according to the new
        Alpha calling standard.

    Forrest Foltz (forrestf) 24-Jan-1998

        Modified format to use new obj-based procedure.

--*/

#include "ki.h"
#pragma hdrstop
#define HEADER_FILE
#include "excpt.h"
#include "ntdef.h"
#include "ntkeapi.h"
#include "ntalpha.h"
#include "ntimage.h"
#include "ntseapi.h"
#include "ntobapi.h"
#include "ntlpcapi.h"
#include "ntioapi.h"
#include "ntmmapi.h"
#include "ntldr.h"
#include "ntpsapi.h"
#include "ntpoapi.h"
#include "ntexapi.h"
#include "ntnls.h"
#include "nturtl.h"
#include "ntcsrmsg.h"
#include "ntcsrsrv.h"
#include "ntxcapi.h"
#include "arc.h"
#include "ntstatus.h"
#include "kxalpha.h"
#include "stdarg.h"
#include "setjmp.h"
#include "alphaops.h"
#include "fwcallbk.h"

include(`..\genxx.h')

#define KSALPHA  SEF_KERNEL
#define HALALPHA SEF_HAL

//
// Helper macro for call pal functions
//

#define genPal(CALLPAL) \
    genStr("#define "CALLPAL##_STR" 0x%lx\n", CALLPAL)

//
// Bitmask definitions here
//

#if defined(_AXP64_)

    startBitStruc( HARDWARE_PTE,    KSALPHA )
    genBitField( Valid,             PTE_VALID )
    genBitField( Reserved1 )
    genBitField( FaultOnWrite,      PTE_FOW )
    genBitField( Reserved2 )
    genBitField( Global,            PTE_GLOBAL )
    genBitField( GranularityHint,   PTE_GH )
    genBitField( Reserved3 )
    genBitField( KernelReadAccess,  PTE_KRE )
    genBitField( UserReadAccess,    PTE_URE )
    genBitField( Reserved4 )
    genBitField( KernelWriteAccess, PTE_KWE )
    genBitField( UserWriteAccess,   PTE_UWE )
    genBitField( Reserved5 )
    genBitField( Write ,            PTE_WRITE )
    genBitField( CopyOnWrite,       PTE_COPY_ON_WRITE )
    genBitField( Software,          PTE_SOFTWARE )
    genBitField( PageFrameNumber,   PTE_PFN )

#else


    startBitStruc(HARDWARE_PTE, KSALPHA)
    genBitField(Valid, PTE_VALID)
    genBitField(Owner, PTE_OWNER)
    genBitField(Dirty, PTE_DIRTY)
    genBitField(reserved)
    genBitField(Global, PTE_GLOBAL)
    genBitField(GranularityHint)
    genBitField(Write, PTE_WRITE)
    genBitField(CopyOnWrite, PTE_COPYONWRITE)
    genBitField(PageFrameNumber, PTE_PFN)

#endif

    startBitStruc(PSR, KSALPHA)
    genBitField(MODE, PSR_MODE)
    genBitAlias(PSR_USER_MODE)
    genBitField(INTERRUPT_ENABLE, PSR_IE)
    genBitField(IRQL, PSR_IRQL)

    startBitStruc(IE, KSALPHA)
    genBitField(SoftwareInterruptEnables, IE_SFW)
    genBitField(HardwareInterruptEnables, IE_HDW)

    startBitStruc(MCHK_STATUS, KSALPHA | HALALPHA)
    genBitField(Correctable, MCHK_CORRECTABLE)
    genBitField(Retryable, MCHK_RETRYABLE)

    startBitStruc(MCES, KSALPHA | HALALPHA)
    genBitField(MachineCheck, MCES_MCK)
    genBitField(SystemCorrectable, MCES_SCE)
    genBitField(ProcessorCorrectable, MCES_PCE)
    genBitField(DisableProcessorCorrectable, MCES_DPC)
    genBitField(DisableSystemCorrectable, MCES_DSC)
    genBitField(DisableMachineChecks, MCES_DMCK)

    startBitStruc(EXC_SUM, KSALPHA)
    genBitField(SoftwareCompletion, EXCSUM_SWC)
    genBitField(InvalidOperation, EXCSUM_INV)
    genBitField(DivisionByZero, EXCSUM_DZE)
    genBitField(Overflow, EXCSUM_OVF)
    genBitField(Underflow, EXCSUM_UNF)
    genBitField(InexactResult, EXCSUM_INE)
    genBitField(IntegerOverflow, EXCSUM_IOV)

//
// Element description array is here
//

STRUC_ELEMENT ElementList[] = {

    START_LIST

    //
    // Output include statement for ALPHA architecture static definitions.
    //

  EnableInc(KSALPHA | HALALPHA)

    genTxt("#include \"kxalpha.h\"\n")
  DisableInc(HALALPHA)

    //
    // Include architecture independent definitions.
    //

#include "..\genxx.inc"


    //
    // Generate architecture dependent definitions.
    //
    // Processor control register structure definitions.
    //

  EnableInc(HAL)

    genCom("Processor Control Registers Structure Offset Definitions")
    genNam(PCR_MINOR_VERSION)
    genNam(PCR_MAJOR_VERSION)
    genDef(Pc, KPCR, MinorVersion)
    genDef(Pc, KPCR, MajorVersion)
    genDef(Pc, KPCR, PalBaseAddress)
    genDef(Pc, KPCR, PalMajorVersion)
    genDef(Pc, KPCR, PalMinorVersion)
    genDef(Pc, KPCR, PalSequenceVersion)
    genDef(Pc, KPCR, PalMajorSpecification)
    genDef(Pc, KPCR, PalMinorSpecification)
    genDef(Pc, KPCR, FirmwareRestartAddress)
    genDef(Pc, KPCR, RestartBlock)
    genDef(Pc, KPCR, PalReserved)
    genDef(Pc, KPCR, PalAlignmentFixupCount)
    genDef(Pc, KPCR, PanicStack)
    genDef(Pc, KPCR, ProcessorType)
    genDef(Pc, KPCR, ProcessorRevision)
    genDef(Pc, KPCR, PhysicalAddressBits)
    genDef(Pc, KPCR, MaximumAddressSpaceNumber)
    genDef(Pc, KPCR, PageSize)
    genDef(Pc, KPCR, FirstLevelDcacheSize)
    genDef(Pc, KPCR, FirstLevelDcacheFillSize)
    genDef(Pc, KPCR, FirstLevelIcacheSize)
    genDef(Pc, KPCR, FirstLevelIcacheFillSize)
    genDef(Pc, KPCR, FirmwareRevisionId)
    genDef(Pc, KPCR, SystemType)
    genDef(Pc, KPCR, SystemVariant)
    genDef(Pc, KPCR, SystemRevision)
    genDef(Pc, KPCR, SystemSerialNumber)
    genDef(Pc, KPCR, CycleClockPeriod)
    genDef(Pc, KPCR, SecondLevelCacheSize)
    genDef(Pc, KPCR, SecondLevelCacheFillSize)
    genDef(Pc, KPCR, ThirdLevelCacheSize)
    genDef(Pc, KPCR, ThirdLevelCacheFillSize)
    genDef(Pc, KPCR, FourthLevelCacheSize)
    genDef(Pc, KPCR, FourthLevelCacheFillSize)
    genDef(Pc, KPCR, Prcb)
    genDef(Pc, KPCR, Number)
    genDef(Pc, KPCR, SetMember)
    genDef(Pc, KPCR, HalReserved)
    genDef(Pc, KPCR, IrqlTable)
    genDef(Pc, KPCR, IrqlMask)
    genDef(Pc, KPCR, InterruptRoutine)
    genDef(Pc, KPCR, ReservedVectors)
    genDef(Pc, KPCR, MachineCheckError)
    genDef(Pc, KPCR, DpcStack)
    genDef(Pc, KPCR, NotMember)
    genDef(Pc, KPCR, CurrentPid)
    genDef(Pc, KPCR, SystemServiceDispatchStart)
    genDef(Pc, KPCR, SystemServiceDispatchEnd)
    genDef(Pc, KPCR, IdleThread)
    genVal(ProcessorControlRegisterLength, ROUND_UP(sizeof(KPCR), 16))

    genNamUint(SharedUserData)
    genDef(Us, KUSER_SHARED_DATA, TickCountLow)
    genDef(Us, KUSER_SHARED_DATA, TickCountMultiplier)
    genDef(Us, KUSER_SHARED_DATA, InterruptTime)
#if defined(_AXP64_)
    genDef(Us, KUSER_SHARED_DATA, InterruptHigh2Time)
    genDef(Us, KUSER_SHARED_DATA, SystemLowTime)
    genDef(Us, KUSER_SHARED_DATA, SystemHigh1Time)
    genDef(Us, KUSER_SHARED_DATA, SystemHigh2Time)
#else
    genDef(Us, KUSER_SHARED_DATA, SystemTime)
#endif

    //
    // Processor block structure definitions.
    //

    genCom("Processor Block Structure Offset Definitions")

    genNam(PRCB_MINOR_VERSION)
    genNam(PRCB_MAJOR_VERSION)

    genDef(Pb, KPRCB, MinorVersion)
    genDef(Pb, KPRCB, MajorVersion)
    genDef(Pb, KPRCB, CurrentThread)
    genDef(Pb, KPRCB, NextThread)
    genDef(Pb, KPRCB, IdleThread)
    genDef(Pb, KPRCB, Number)
    genDef(Pb, KPRCB, BuildType)
    genDef(Pb, KPRCB, SetMember)
    genDef(Pb, KPRCB, RestartBlock)

  DisableInc(HALALPHA)

    genDef(Pb, KPRCB, InterruptCount)
    genDef(Pb, KPRCB, DpcTime)
    genDef(Pb, KPRCB, InterruptTime)
    genDef(Pb, KPRCB, KernelTime)
    genDef(Pb, KPRCB, UserTime)
    genDef(Pb, KPRCB, QuantumEndDpc)
    genDef(Pb, KPRCB, IpiFrozen)
    genDef(Pb, KPRCB, IpiCounts)
    genDef(Pb, KPRCB, ProcessorState)

    genAlt(PbAlignmentFixupCount, KPRCB, KeAlignmentFixupCount)
    genAlt(PbContextSwitches, KPRCB, KeContextSwitches)
    genAlt(PbDcacheFlushCount, KPRCB, KeDcacheFlushCount)
    genAlt(PbExceptionDispatchCount, KPRCB, KeExceptionDispatchCount)
    genAlt(PbFirstLevelTbFills, KPRCB, KeFirstLevelTbFills)
    genAlt(PbFloatingEmulationCount, KPRCB, KeFloatingEmulationCount)
    genAlt(PbIcacheFlushCount, KPRCB, KeIcacheFlushCount)
    genAlt(PbSecondLevelTbFills, KPRCB, KeSecondLevelTbFills)
    genAlt(PbSystemCalls, KPRCB, KeSystemCalls)

    genDef(Pb, KPRCB, LockQueue)
    genDef(Pb, KPRCB, CurrentPacket)
    genDef(Pb, KPRCB, TargetSet)
    genDef(Pb, KPRCB, WorkerRoutine)
    genDef(Pb, KPRCB, RequestSummary)
    genDef(Pb, KPRCB, DpcListHead)
    genDef(Pb, KPRCB, DpcLock)
    genDef(Pb, KPRCB, DpcCount)
    genDef(Pb, KPRCB, LastDpcCount)
    genDef(Pb, KPRCB, QuantumEnd)
    genDef(Pb, KPRCB, StartCount)
    genDef(Pb, KPRCB, SoftwareInterrupts)
    genDef(Pb, KPRCB, InterruptTrapFrame)
    genDef(Pb, KPRCB, DpcRoutineActive)
    genDef(Pb, KPRCB, DpcQueueDepth)
    genDef(Pb, KPRCB, DpcRequestRate)
    genDef(Pb, KPRCB, DpcBypassCount)
    genDef(Pb, KPRCB, ApcBypassCount)
    genDef(Pb, KPRCB, DispatchInterruptCount)
    genDef(Pb, KPRCB, DebugDpcTime)
    genDef(Pb, KPRCB, DpcInterruptRequested)
    genDef(Pb, KPRCB, MaximumDpcQueueDepth)
    genDef(Pb, KPRCB, MinimumDpcRate)
    genDef(Pb, KPRCB, AdjustDpcThreshold)

  EnableInc(HALALPHA)

    genDef(Pb, KPRCB, PowerState)
    genVal(ProcessorBlockLength, ROUND_UP(sizeof(KPRCB), 16))

    //
    // Prcb power state
    //

    genCom("Processor Power State Offset Definitions")
    genDef(Pp, PROCESSOR_POWER_STATE, IdleFunction)

  DisableInc(HALALPHA)

    //
    // Immediate interprocessor command definitions.
    //

    genCom("Immediate Interprocessor Command Definitions")

    genNam(IPI_APC)
    genNam(IPI_DPC)
    genNam(IPI_FREEZE)
    genNam(IPI_PACKET_READY)

    //
    // Interprocessor interrupt count structure offset definitions.
    //

    genCom("Interprocessor Interrupt Count Structure Offset Definitions")

    genDef(Ic, KIPI_COUNTS, Freeze)
    genDef(Ic, KIPI_COUNTS, Packet)
    genDef(Ic, KIPI_COUNTS, DPC)
    genDef(Ic, KIPI_COUNTS, APC)
    genDef(Ic, KIPI_COUNTS, FlushSingleTb)
    genDef(Ic, KIPI_COUNTS, FlushEntireTb)
    genDef(Ic, KIPI_COUNTS, ChangeColor)
    genDef(Ic, KIPI_COUNTS, SweepDcache)
    genDef(Ic, KIPI_COUNTS, SweepIcache)
    genDef(Ic, KIPI_COUNTS, SweepIcacheRange)
    genDef(Ic, KIPI_COUNTS, FlushIoBuffers)

    //
    // LPC structure offset definitions
    //

    genCom("LPC Structure Offset Definitions")

    genAlt(PmLength, PORT_MESSAGE, u1.Length)
    genAlt(PmClientId, PORT_MESSAGE, ClientId)
    genAlt(PmProcess, PORT_MESSAGE, ClientId.UniqueProcess)
    genAlt(PmThread, PORT_MESSAGE, ClientId.UniqueThread)
    genAlt(PmMessageId, PORT_MESSAGE, MessageId)
    genAlt(PmClientViewSize, PORT_MESSAGE, ClientViewSize)
    genVal(PortMessageLength, sizeof(PORT_MESSAGE))

    //
    // Client ID structure offset definitions
    //

    genCom("Client Id Structure Offset Definitions")

    genDef(Cid, CLIENT_ID, UniqueProcess)
    genDef(Cid, CLIENT_ID, UniqueThread)

    //
    // Context frame offset definitions and flag definitions.
    //

  EnableInc(HALALPHA)
    genCom("Context Frame Offset and Flag Definitions")

    genNam(CONTEXT_FULL)
    genNam(CONTEXT_CONTROL)
    genNam(CONTEXT_FLOATING_POINT)
    genNam(CONTEXT_INTEGER)
    genSpc()

    genDef(Cx, CONTEXT, FltF0)
    genDef(Cx, CONTEXT, FltF1)
    genDef(Cx, CONTEXT, FltF2)
    genDef(Cx, CONTEXT, FltF3)
    genDef(Cx, CONTEXT, FltF4)
    genDef(Cx, CONTEXT, FltF5)
    genDef(Cx, CONTEXT, FltF6)
    genDef(Cx, CONTEXT, FltF7)
    genDef(Cx, CONTEXT, FltF8)
    genDef(Cx, CONTEXT, FltF9)
    genDef(Cx, CONTEXT, FltF10)
    genDef(Cx, CONTEXT, FltF11)
    genDef(Cx, CONTEXT, FltF12)
    genDef(Cx, CONTEXT, FltF13)
    genDef(Cx, CONTEXT, FltF14)
    genDef(Cx, CONTEXT, FltF15)
    genDef(Cx, CONTEXT, FltF16)
    genDef(Cx, CONTEXT, FltF17)
    genDef(Cx, CONTEXT, FltF18)
    genDef(Cx, CONTEXT, FltF19)
    genDef(Cx, CONTEXT, FltF20)
    genDef(Cx, CONTEXT, FltF21)
    genDef(Cx, CONTEXT, FltF22)
    genDef(Cx, CONTEXT, FltF23)
    genDef(Cx, CONTEXT, FltF24)
    genDef(Cx, CONTEXT, FltF25)
    genDef(Cx, CONTEXT, FltF26)
    genDef(Cx, CONTEXT, FltF27)
    genDef(Cx, CONTEXT, FltF28)
    genDef(Cx, CONTEXT, FltF29)
    genDef(Cx, CONTEXT, FltF30)
    genDef(Cx, CONTEXT, FltF31)

    genDef(Cx, CONTEXT, IntV0)
    genDef(Cx, CONTEXT, IntT0)
    genDef(Cx, CONTEXT, IntT1)
    genDef(Cx, CONTEXT, IntT2)

    genDef(Cx, CONTEXT, IntT3)
    genDef(Cx, CONTEXT, IntT4)
    genDef(Cx, CONTEXT, IntT5)
    genDef(Cx, CONTEXT, IntT6)

    genDef(Cx, CONTEXT, IntT7)
    genDef(Cx, CONTEXT, IntS0)
    genDef(Cx, CONTEXT, IntS1)
    genDef(Cx, CONTEXT, IntS2)

    genDef(Cx, CONTEXT, IntS3)
    genDef(Cx, CONTEXT, IntS4)
    genDef(Cx, CONTEXT, IntS5)
    genDef(Cx, CONTEXT, IntFp)

    genDef(Cx, CONTEXT, IntA0)
    genDef(Cx, CONTEXT, IntA1)
    genDef(Cx, CONTEXT, IntA2)
    genDef(Cx, CONTEXT, IntA3)

    genDef(Cx, CONTEXT, IntA4)
    genDef(Cx, CONTEXT, IntA5)
    genDef(Cx, CONTEXT, IntT8)
    genDef(Cx, CONTEXT, IntT9)

    genDef(Cx, CONTEXT, IntT10)
    genDef(Cx, CONTEXT, IntT11)
    genDef(Cx, CONTEXT, IntRa)
    genDef(Cx, CONTEXT, IntT12)

    genDef(Cx, CONTEXT, IntAt)
    genDef(Cx, CONTEXT, IntGp)
    genDef(Cx, CONTEXT, IntSp)
    genDef(Cx, CONTEXT, IntZero)

    genDef(Cx, CONTEXT, Fpcr)
    genDef(Cx, CONTEXT, SoftFpcr)
    genDef(Cx, CONTEXT, Fir)
    genDef(Cx, CONTEXT, Psr)
    genDef(Cx, CONTEXT, ContextFlags)
    genVal(ContextFrameLength, ROUND_UP(sizeof(CONTEXT), 16))

    //
    // Exception frame offset definitions.
    //

    genCom("Exception Frame Offset Definitions and Length")

    genDef(Ex, KEXCEPTION_FRAME, FltF2)
    genDef(Ex, KEXCEPTION_FRAME, FltF3)
    genDef(Ex, KEXCEPTION_FRAME, FltF4)
    genDef(Ex, KEXCEPTION_FRAME, FltF5)
    genDef(Ex, KEXCEPTION_FRAME, FltF6)
    genDef(Ex, KEXCEPTION_FRAME, FltF7)
    genDef(Ex, KEXCEPTION_FRAME, FltF8)
    genDef(Ex, KEXCEPTION_FRAME, FltF9)

    genDef(Ex, KEXCEPTION_FRAME, IntS0)
    genDef(Ex, KEXCEPTION_FRAME, IntS1)
    genDef(Ex, KEXCEPTION_FRAME, IntS2)
    genDef(Ex, KEXCEPTION_FRAME, IntS3)
    genDef(Ex, KEXCEPTION_FRAME, IntS4)
    genDef(Ex, KEXCEPTION_FRAME, IntS5)
    genDef(Ex, KEXCEPTION_FRAME, IntFp)

    genDef(Ex, KEXCEPTION_FRAME, Psr)
    genDef(Ex, KEXCEPTION_FRAME, SwapReturn)
    genDef(Ex, KEXCEPTION_FRAME, IntRa)
    genVal(ExceptionFrameLength, ROUND_UP(sizeof(KEXCEPTION_FRAME), 16))

    //
    // Jump buffer offset definitions.
    //

    genCom("Jump Offset Definitions and Length")

    genDef(Jb, _JUMP_BUFFER, Fp)
    genDef(Jb, _JUMP_BUFFER, Pc)
    genDef(Jb, _JUMP_BUFFER, Seb)
    genDef(Jb, _JUMP_BUFFER, Type)
    genDef(Jb, _JUMP_BUFFER, FltF2)
    genDef(Jb, _JUMP_BUFFER, FltF3)
    genDef(Jb, _JUMP_BUFFER, FltF4)
    genDef(Jb, _JUMP_BUFFER, FltF5)
    genDef(Jb, _JUMP_BUFFER, FltF6)
    genDef(Jb, _JUMP_BUFFER, FltF7)
    genDef(Jb, _JUMP_BUFFER, FltF8)
    genDef(Jb, _JUMP_BUFFER, FltF9)
    genDef(Jb, _JUMP_BUFFER, IntS0)
    genDef(Jb, _JUMP_BUFFER, IntS1)
    genDef(Jb, _JUMP_BUFFER, IntS2)
    genDef(Jb, _JUMP_BUFFER, IntS3)
    genDef(Jb, _JUMP_BUFFER, IntS4)
    genDef(Jb, _JUMP_BUFFER, IntS5)
    genDef(Jb, _JUMP_BUFFER, IntS6)
    genDef(Jb, _JUMP_BUFFER, IntSp)
    genDef(Jb, _JUMP_BUFFER, Fir)

    //
    // Trap frame offset definitions.
    //

    genCom("Trap Frame Offset Definitions and Length")

    genDef(Tr, KTRAP_FRAME, FltF0)
    genDef(Tr, KTRAP_FRAME, FltF1)

    genDef(Tr, KTRAP_FRAME, FltF10)
    genDef(Tr, KTRAP_FRAME, FltF11)
    genDef(Tr, KTRAP_FRAME, FltF12)
    genDef(Tr, KTRAP_FRAME, FltF13)
    genDef(Tr, KTRAP_FRAME, FltF14)
    genDef(Tr, KTRAP_FRAME, FltF15)
    genDef(Tr, KTRAP_FRAME, FltF16)
    genDef(Tr, KTRAP_FRAME, FltF17)
    genDef(Tr, KTRAP_FRAME, FltF18)
    genDef(Tr, KTRAP_FRAME, FltF19)
    genDef(Tr, KTRAP_FRAME, FltF20)
    genDef(Tr, KTRAP_FRAME, FltF21)
    genDef(Tr, KTRAP_FRAME, FltF22)
    genDef(Tr, KTRAP_FRAME, FltF23)
    genDef(Tr, KTRAP_FRAME, FltF24)
    genDef(Tr, KTRAP_FRAME, FltF25)
    genDef(Tr, KTRAP_FRAME, FltF26)
    genDef(Tr, KTRAP_FRAME, FltF27)
    genDef(Tr, KTRAP_FRAME, FltF28)
    genDef(Tr, KTRAP_FRAME, FltF29)
    genDef(Tr, KTRAP_FRAME, FltF30)

    genDef(Tr, KTRAP_FRAME, IntV0)

    genDef(Tr, KTRAP_FRAME, IntT0)
    genDef(Tr, KTRAP_FRAME, IntT1)
    genDef(Tr, KTRAP_FRAME, IntT2)
    genDef(Tr, KTRAP_FRAME, IntT3)
    genDef(Tr, KTRAP_FRAME, IntT4)
    genDef(Tr, KTRAP_FRAME, IntT5)
    genDef(Tr, KTRAP_FRAME, IntT6)
    genDef(Tr, KTRAP_FRAME, IntT7)

    genDef(Tr, KTRAP_FRAME, IntFp)

    genDef(Tr, KTRAP_FRAME, IntA0)
    genDef(Tr, KTRAP_FRAME, IntA1)
    genDef(Tr, KTRAP_FRAME, IntA2)
    genDef(Tr, KTRAP_FRAME, IntA3)
    genDef(Tr, KTRAP_FRAME, IntA4)
    genDef(Tr, KTRAP_FRAME, IntA5)

    genDef(Tr, KTRAP_FRAME, IntT8)
    genDef(Tr, KTRAP_FRAME, IntT9)
    genDef(Tr, KTRAP_FRAME, IntT10)
    genDef(Tr, KTRAP_FRAME, IntT11)

    genDef(Tr, KTRAP_FRAME, IntT12)
    genDef(Tr, KTRAP_FRAME, IntAt)
    genDef(Tr, KTRAP_FRAME, IntGp)
    genDef(Tr, KTRAP_FRAME, IntSp)

    genDef(Tr, KTRAP_FRAME, Fpcr)
    genDef(Tr, KTRAP_FRAME, Psr)
    genDef(Tr, KTRAP_FRAME, Fir)
    genAlt(TrExceptionRecord, KTRAP_FRAME, ExceptionRecord[0])
    genDef(Tr, KTRAP_FRAME, OldIrql)
    genDef(Tr, KTRAP_FRAME, PreviousMode)
    genDef(Tr, KTRAP_FRAME, IntRa)
    genDef(Tr, KTRAP_FRAME, TrapFrame)
    genVal(TrapFrameLength, ROUND_UP(sizeof(KTRAP_FRAME), 16))

    //
    // Firmware frame offset defintions and length.
    //

  DisableInc(HALALPHA)

    genCom("Firmware frame offset defintions and length")

    genNam(FW_EXC_MCHK)
    genNam(FW_EXC_ARITH)
    genNam(FW_EXC_INTERRUPT)
    genNam(FW_EXC_DFAULT)
    genNam(FW_EXC_ITBMISS)
    genNam(FW_EXC_ITBACV)
    genNam(FW_EXC_NDTBMISS)
    genNam(FW_EXC_PDTBMISS)
    genNam(FW_EXC_UNALIGNED)
    genNam(FW_EXC_OPCDEC)
    genNam(FW_EXC_FEN)
    genNam(FW_EXC_HALT)
    genNam(FW_EXC_BPT)
    genNam(FW_EXC_GENTRAP)
    genNam(FW_EXC_HALT_INTERRUPT)

    genDef(Fw, FIRMWARE_FRAME, Type)
    genDef(Fw, FIRMWARE_FRAME, Param1)
    genDef(Fw, FIRMWARE_FRAME, Param2)
    genDef(Fw, FIRMWARE_FRAME, Param3)
    genDef(Fw, FIRMWARE_FRAME, Param4)
    genDef(Fw, FIRMWARE_FRAME, Param5)
    genDef(Fw, FIRMWARE_FRAME, Psr)
    genDef(Fw, FIRMWARE_FRAME, Mmcsr)
    genDef(Fw, FIRMWARE_FRAME, Va)
    genDef(Fw, FIRMWARE_FRAME, Fir)
    genDef(Fw, FIRMWARE_FRAME, IntV0)
    genDef(Fw, FIRMWARE_FRAME, IntT0)
    genDef(Fw, FIRMWARE_FRAME, IntT1)
    genDef(Fw, FIRMWARE_FRAME, IntT2)
    genDef(Fw, FIRMWARE_FRAME, IntT3)
    genDef(Fw, FIRMWARE_FRAME, IntT4)
    genDef(Fw, FIRMWARE_FRAME, IntT5)
    genDef(Fw, FIRMWARE_FRAME, IntT6)
    genDef(Fw, FIRMWARE_FRAME, IntT7)
    genDef(Fw, FIRMWARE_FRAME, IntS0)
    genDef(Fw, FIRMWARE_FRAME, IntS1)
    genDef(Fw, FIRMWARE_FRAME, IntS2)
    genDef(Fw, FIRMWARE_FRAME, IntS3)
    genDef(Fw, FIRMWARE_FRAME, IntS4)
    genDef(Fw, FIRMWARE_FRAME, IntS5)
    genDef(Fw, FIRMWARE_FRAME, IntFp)
    genDef(Fw, FIRMWARE_FRAME, IntA0)
    genDef(Fw, FIRMWARE_FRAME, IntA1)
    genDef(Fw, FIRMWARE_FRAME, IntA2)
    genDef(Fw, FIRMWARE_FRAME, IntA3)
    genDef(Fw, FIRMWARE_FRAME, IntA4)
    genDef(Fw, FIRMWARE_FRAME, IntA5)
    genDef(Fw, FIRMWARE_FRAME, IntT8)
    genDef(Fw, FIRMWARE_FRAME, IntT9)
    genDef(Fw, FIRMWARE_FRAME, IntT10)
    genDef(Fw, FIRMWARE_FRAME, IntT11)
    genDef(Fw, FIRMWARE_FRAME, IntRa)
    genDef(Fw, FIRMWARE_FRAME, IntT12)
    genDef(Fw, FIRMWARE_FRAME, IntAt)
    genDef(Fw, FIRMWARE_FRAME, IntGp)
    genDef(Fw, FIRMWARE_FRAME, IntSp)
    genDef(Fw, FIRMWARE_FRAME, IntZero)
    genDef(Fw, FIRMWARE_FRAME, FltF0)
    genDef(Fw, FIRMWARE_FRAME, FltF1)
    genDef(Fw, FIRMWARE_FRAME, FltF2)
    genDef(Fw, FIRMWARE_FRAME, FltF3)
    genDef(Fw, FIRMWARE_FRAME, FltF4)
    genDef(Fw, FIRMWARE_FRAME, FltF5)
    genDef(Fw, FIRMWARE_FRAME, FltF6)
    genDef(Fw, FIRMWARE_FRAME, FltF7)
    genDef(Fw, FIRMWARE_FRAME, FltF8)
    genDef(Fw, FIRMWARE_FRAME, FltF9)
    genDef(Fw, FIRMWARE_FRAME, FltF10)
    genDef(Fw, FIRMWARE_FRAME, FltF11)
    genDef(Fw, FIRMWARE_FRAME, FltF12)
    genDef(Fw, FIRMWARE_FRAME, FltF13)
    genDef(Fw, FIRMWARE_FRAME, FltF14)
    genDef(Fw, FIRMWARE_FRAME, FltF15)
    genDef(Fw, FIRMWARE_FRAME, FltF16)
    genDef(Fw, FIRMWARE_FRAME, FltF17)
    genDef(Fw, FIRMWARE_FRAME, FltF18)
    genDef(Fw, FIRMWARE_FRAME, FltF19)
    genDef(Fw, FIRMWARE_FRAME, FltF20)
    genDef(Fw, FIRMWARE_FRAME, FltF21)
    genDef(Fw, FIRMWARE_FRAME, FltF22)
    genDef(Fw, FIRMWARE_FRAME, FltF23)
    genDef(Fw, FIRMWARE_FRAME, FltF24)
    genDef(Fw, FIRMWARE_FRAME, FltF25)
    genDef(Fw, FIRMWARE_FRAME, FltF26)
    genDef(Fw, FIRMWARE_FRAME, FltF27)
    genDef(Fw, FIRMWARE_FRAME, FltF28)
    genDef(Fw, FIRMWARE_FRAME, FltF29)
    genDef(Fw, FIRMWARE_FRAME, FltF30)
    genDef(Fw, FIRMWARE_FRAME, FltF31)

    genVal(FirmwareFrameLength, FIRMWARE_FRAME_LENGTH)

    //
    // Usermode lout frame definitions
    //

    genCom("Usermode callout frame definitions")

    genDef(Cu, KCALLOUT_FRAME, F2)
    genDef(Cu, KCALLOUT_FRAME, F3)
    genDef(Cu, KCALLOUT_FRAME, F4)
    genDef(Cu, KCALLOUT_FRAME, F5)
    genDef(Cu, KCALLOUT_FRAME, F6)
    genDef(Cu, KCALLOUT_FRAME, F7)
    genDef(Cu, KCALLOUT_FRAME, F8)
    genDef(Cu, KCALLOUT_FRAME, F9)
    genDef(Cu, KCALLOUT_FRAME, S0)
    genDef(Cu, KCALLOUT_FRAME, S1)
    genDef(Cu, KCALLOUT_FRAME, S2)
    genDef(Cu, KCALLOUT_FRAME, S3)
    genDef(Cu, KCALLOUT_FRAME, S4)
    genDef(Cu, KCALLOUT_FRAME, S5)
    genDef(Cu, KCALLOUT_FRAME, FP)
    genDef(Cu, KCALLOUT_FRAME, CbStk)
    genDef(Cu, KCALLOUT_FRAME, InStk)
    genDef(Cu, KCALLOUT_FRAME, TrFr)
    genDef(Cu, KCALLOUT_FRAME, TrFir)
    genDef(Cu, KCALLOUT_FRAME, Ra)
    genDef(Cu, KCALLOUT_FRAME, A0)
    genDef(Cu, KCALLOUT_FRAME, A1)
    genVal(CuFrameLength, sizeof(KCALLOUT_FRAME))

    //
    // Usermode callout user frame definitions.
    //

    genCom("Usermode callout user frame definitions")

    genDef(Ck, UCALLOUT_FRAME, Buffer)
    genDef(Ck, UCALLOUT_FRAME, Length)
    genDef(Ck, UCALLOUT_FRAME, ApiNumber)
    genDef(Ck, UCALLOUT_FRAME, Sp)
    genDef(Ck, UCALLOUT_FRAME, Ra)

    //
    // KFLOATING_SAVE definition
    //

    genCom("KFLOATING_SAVE definitions")

    genDef(Kfs, KFLOATING_SAVE, Fpcr)
    genDef(Kfs, KFLOATING_SAVE, SoftFpcr)
    genDef(Kfs, KFLOATING_SAVE, Reserved1)
    genDef(Kfs, KFLOATING_SAVE, Reserved2)
    genDef(Kfs, KFLOATING_SAVE, Reserved3)
    genDef(Kfs, KFLOATING_SAVE, Reserved4)

  EnableInc(HALALPHA)

    //
    // Loader Paeter Block offset definitions.
    //

    genCom("Loader Parameter Block Offset Definitions")

    genDef(Lpb, LOADER_PARAMETER_BLOCK, LoadOrderListHead)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, MemoryDescriptorListHead)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, KernelStack)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, Prcb)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, Process)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, Thread)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, RegistryLength)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, RegistryBase)

    genAlt(LpbDpcStack,
           LOADER_PARAMETER_BLOCK, u.Alpha.DpcStack)

    genAlt(LpbFirstLevelDcacheSize,
           LOADER_PARAMETER_BLOCK, u.Alpha.FirstLevelDcacheSize)

    genAlt(LpbFirstLevelDcacheFillSize,
           LOADER_PARAMETER_BLOCK, u.Alpha.FirstLevelDcacheFillSize)

    genAlt(LpbFirstLevelIcacheSize,
           LOADER_PARAMETER_BLOCK, u.Alpha.FirstLevelIcacheSize)

    genAlt(LpbFirstLevelIcacheFillSize,
           LOADER_PARAMETER_BLOCK, u.Alpha.FirstLevelIcacheFillSize)

    genAlt(LpbGpBase,
           LOADER_PARAMETER_BLOCK, u.Alpha.GpBase)

    genAlt(LpbPanicStack,
           LOADER_PARAMETER_BLOCK, u.Alpha.PanicStack)

    genAlt(LpbPcrPage,
           LOADER_PARAMETER_BLOCK, u.Alpha.PcrPage)

    genAlt(LpbPdrPage,
           LOADER_PARAMETER_BLOCK, u.Alpha.PdrPage)

    genAlt(LpbSecondLevelDcacheSize,
           LOADER_PARAMETER_BLOCK, u.Alpha.SecondLevelDcacheSize)

    genAlt(LpbSecondLevelDcacheFillSize,
           LOADER_PARAMETER_BLOCK, u.Alpha.SecondLevelDcacheFillSize)

    genAlt(LpbSecondLevelIcacheSize,
           LOADER_PARAMETER_BLOCK, u.Alpha.SecondLevelIcacheSize)

    genAlt(LpbSecondLevelIcacheFillSize,
           LOADER_PARAMETER_BLOCK, u.Alpha.SecondLevelIcacheFillSize)

    genAlt(LpbPhysicalAddressBits,
           LOADER_PARAMETER_BLOCK, u.Alpha.PhysicalAddressBits)

    genAlt(LpbMaximumAddressSpaceNumber,
           LOADER_PARAMETER_BLOCK, u.Alpha.MaximumAddressSpaceNumber)

    genAlt(LpbSystemSerialNumber,
           LOADER_PARAMETER_BLOCK, u.Alpha.SystemSerialNumber[0])

    genAlt(LpbSystemType,
           LOADER_PARAMETER_BLOCK, u.Alpha.SystemType[0])

    genAlt(LpbSystemVariant,
           LOADER_PARAMETER_BLOCK, u.Alpha.SystemVariant)

    genAlt(LpbSystemRevision,
           LOADER_PARAMETER_BLOCK, u.Alpha.SystemRevision)

    genAlt(LpbProcessorType,
           LOADER_PARAMETER_BLOCK, u.Alpha.ProcessorType)

    genAlt(LpbProcessorRevision,
           LOADER_PARAMETER_BLOCK, u.Alpha.ProcessorRevision)

    genAlt(LpbCycleClockPeriod,
           LOADER_PARAMETER_BLOCK, u.Alpha.CycleClockPeriod)

    genAlt(LpbPageSize,
           LOADER_PARAMETER_BLOCK, u.Alpha.PageSize)

    genAlt(LpbRestartBlock,
           LOADER_PARAMETER_BLOCK, u.Alpha.RestartBlock)

    genAlt(LpbFirmwareRestartAddress,
           LOADER_PARAMETER_BLOCK, u.Alpha.FirmwareRestartAddress)

    genAlt(LpbFirmwareRevisionId,
           LOADER_PARAMETER_BLOCK, u.Alpha.FirmwareRevisionId)

    genAlt(LpbPalBaseAddress,
           LOADER_PARAMETER_BLOCK, u.Alpha.PalBaseAddress)

  DisableInc(HALALPHA)

    //
    // Restart Block Structure and Alpha Save Area Structure.
    //
    // N.B. - The Alpha Save Area Structure Offsets are written as though
    // they were offsets from the beginning of the Restart block.
    //

  EnableInc(HALALPHA)
    genCom("Restart Block Structure Definitions")

    genDef(Rb, RESTART_BLOCK, Signature)
    genDef(Rb, RESTART_BLOCK, Length)
    genDef(Rb, RESTART_BLOCK, Version)
    genDef(Rb, RESTART_BLOCK, Revision)
    genDef(Rb, RESTART_BLOCK, NextRestartBlock)
    genDef(Rb, RESTART_BLOCK, RestartAddress)
    genDef(Rb, RESTART_BLOCK, BootMasterId)
    genDef(Rb, RESTART_BLOCK, ProcessorId)
    genDef(Rb, RESTART_BLOCK, BootStatus)
    genDef(Rb, RESTART_BLOCK, CheckSum)
    genDef(Rb, RESTART_BLOCK, SaveAreaLength)
    genAlt(RbSaveArea, RESTART_BLOCK, u.SaveArea)

    genVal(RbHaltReason,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, HaltReason))

    genVal(RbLogoutFrame,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, LogoutFrame))

    genVal(RbPalBase,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, PalBase))

    genVal(RbIntV0,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntV0))

    genVal(RbIntT0,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntT0))

    genVal(RbIntT1,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntT1))

    genVal(RbIntT2,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntT2))

    genVal(RbIntT3,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntT3))

    genVal(RbIntT4,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntT4))

    genVal(RbIntT5,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntT5))

    genVal(RbIntT6,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntT6))

    genVal(RbIntT7,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntT7))

    genVal(RbIntS0,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntS0))

    genVal(RbIntS1,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntS1))

    genVal(RbIntS2,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntS2))

    genVal(RbIntS3,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntS3))

    genVal(RbIntS4,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntS4))

    genVal(RbIntS5,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntS5))

    genVal(RbIntFp,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntFp))

    genVal(RbIntA0,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntA0))

    genVal(RbIntA1,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntA1))

    genVal(RbIntA2,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntA2))

    genVal(RbIntA3,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntA3))

    genVal(RbIntA4,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntA4))

    genVal(RbIntA5,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntA5))

    genVal(RbIntT8,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntT8))

    genVal(RbIntT9,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntT9))

    genVal(RbIntT10,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntT10))

    genVal(RbIntT11,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntT11))

    genVal(RbIntRa,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntRa))

    genVal(RbIntT12,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntT12))

    genVal(RbIntAT,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntAT))

    genVal(RbIntGp,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntGp))

    genVal(RbIntSp,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntSp))

    genVal(RbIntZero,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, IntZero))

    genVal(RbFpcr,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, Fpcr))

    genVal(RbFltF0,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF0))

    genVal(RbFltF1,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF1))

    genVal(RbFltF2,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF2))

    genVal(RbFltF3,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF3))

    genVal(RbFltF4,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF4))

    genVal(RbFltF5,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF5))

    genVal(RbFltF6,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF6))

    genVal(RbFltF7,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF7))

    genVal(RbFltF8,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF8))

    genVal(RbFltF9,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF9))

    genVal(RbFltF10,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF10))

    genVal(RbFltF11,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF11))

    genVal(RbFltF12,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF12))

    genVal(RbFltF13,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF13))

    genVal(RbFltF14,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF14))

    genVal(RbFltF15,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF15))

    genVal(RbFltF16,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF16))

    genVal(RbFltF17,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF17))

    genVal(RbFltF18,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF18))

    genVal(RbFltF19,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF19))

    genVal(RbFltF20,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF20))

    genVal(RbFltF21,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF21))

    genVal(RbFltF22,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF22))

    genVal(RbFltF23,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF23))

    genVal(RbFltF24,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF24))

    genVal(RbFltF25,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF25))

    genVal(RbFltF26,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF26))

    genVal(RbFltF27,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF27))

    genVal(RbFltF28,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF28))

    genVal(RbFltF29,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF29))

    genVal(RbFltF30,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF30))

    genVal(RbFltF31,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, FltF31))

    genVal(RbAsn,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, Asn))

    genVal(RbGeneralEntry,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, GeneralEntry))

    genVal(RbIksp,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, Iksp))

    genVal(RbInterruptEntry,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, InterruptEntry))

    genVal(RbKgp,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, Kgp))

    genVal(RbMces,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, Mces))

    genVal(RbMemMgmtEntry,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, MemMgmtEntry))

    genVal(RbPanicEntry,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, PanicEntry))

    genVal(RbPcr,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, Pcr))

    genVal(RbPdr,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, Pdr))

    genVal(RbPsr,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, Psr))

    genVal(RbReiRestartAddress,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, ReiRestartAddress))

    genVal(RbSirr,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, Sirr))

    genVal(RbSyscallEntry,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, SyscallEntry))

    genVal(RbTeb,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, Teb))

    genVal(RbThread,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, Thread))

    genVal(RbPerProcessorState,
            OFFSET(RESTART_BLOCK, u.SaveArea) +
            OFFSET(ALPHA_RESTART_STATE, PerProcessorState))

    //
    // Address space layout definitions
    //

    genCom("Address Space Layout Definitions")

    genNamUint(KSEG0_BASE)
    genNamUint(KSEG2_BASE)
  DisableInc(HALALPHA)

    genNamUint(SYSTEM_BASE)
    genNamUint(PDE_BASE)
    genNamUint(PTE_BASE)
    genNamUint(PDE64_BASE)
    genNamUint(PTE64_BASE)

    //
    // Page table and page directory entry definitions
    //

  EnableInc(HALALPHA)
    genCom("Page Table and Directory Entry Definitions")

    genNam(PAGE_SIZE)
    genNam(PAGE_SHIFT)
    genNam(PDI_SHIFT)
    genNam(PTI_SHIFT)
  DisableInc(HALALPHA)

    //
    // Breakpoint instruction definitions
    //

  EnableInc(HALALPHA)
    genCom("Breakpoint Definitions")

    genNam(USER_BREAKPOINT)
    genNam(KERNEL_BREAKPOINT)
    genNam(BREAKIN_BREAKPOINT)

    genNam(DEBUG_PRINT_BREAKPOINT)
    genNam(DEBUG_PROMPT_BREAKPOINT)
    genNam(DEBUG_STOP_BREAKPOINT)
    genNam(DEBUG_LOAD_SYMBOLS_BREAKPOINT)
    genNam(DEBUG_UNLOAD_SYMBOLS_BREAKPOINT)

  DisableInc(HALALPHA)
    //
    //
    // Trap code definitions
    //

    genCom("Trap Code Definitions")

    genNam(GENTRAP_INTEGER_OVERFLOW)
    genNam(GENTRAP_INTEGER_DIVIDE_BY_ZERO)
    genNam(GENTRAP_FLOATING_OVERFLOW)
    genNam(GENTRAP_FLOATING_DIVIDE_BY_ZERO)
    genNam(GENTRAP_FLOATING_UNDERFLOW)
    genNam(GENTRAP_FLOATING_INVALID_OPERAND)
    genNam(GENTRAP_FLOATING_INEXACT_RESULT)

    //
    // Miscellaneous definitions
    //

  EnableInc(HALALPHA)
    genCom("Miscellaneous Definitions")

    genNam(Executive)
    genNam(KernelMode)
    genNam(FALSE)
    genNam(TRUE)
  DisableInc(HALALPHA)

    genNam(BASE_PRIORITY_THRESHOLD)
    genNam(EVENT_PAIR_INCREMENT)
    genNam(LOW_REALTIME_PRIORITY)
    genNamUint(MM_USER_PROBE_ADDRESS)
    genNam(KERNEL_STACK_SIZE)
    genNam(KERNEL_LARGE_STACK_COMMIT)
    genNam(SET_LOW_WAIT_HIGH)
    genNam(SET_HIGH_WAIT_LOW)
    genNam(CLOCK_QUANTUM_DECREMENT)
    genNam(READY_SKIP_QUANTUM)
    genNam(THREAD_QUANTUM)
    genNam(WAIT_QUANTUM_DECREMENT)
    genNam(ROUND_TRIP_DECREMENT_COUNT)

    //
    // Generate processor type definitions.
    //

  EnableInc(HALALPHA)
    genNam(PROCESSOR_ALPHA_21064)
    genNam(PROCESSOR_ALPHA_21164)
    genNam(PROCESSOR_ALPHA_21066)
    genNam(PROCESSOR_ALPHA_21068)
    genNam(PROCESSOR_ALPHA_21164PC)
    genNam(PROCESSOR_ALPHA_21264)

    //
    // Insert any bitfield definitions that have been generated
    //

    DUMP_BITFIELDS

    //
    // Generate the call pal mnemonic to opcode definitions.
    //

    genCom("Call PAL Mnemonics")

    genTxt("// begin callpal\n")
    genSpc()

//
// N.B. any new call pal functions must be added to both alphaops.h
// and to the call pal entry table below.
//

    // Unprivileged Call Pals
    genPal(BPT_FUNC)
    genPal(CALLSYS_FUNC)
    genPal(IMB_FUNC)
    genPal(GENTRAP_FUNC)
    genPal(RDTEB_FUNC)
    genPal(KBPT_FUNC)
    genPal(CALLKD_FUNC)
#if defined(_AXP64_)
    genPal(RDTEB64_FUNC)
#endif
    // Privileged Call Pals
    genPal(HALT_FUNC)
    genPal(RESTART_FUNC)
    genPal(DRAINA_FUNC)
    genPal(REBOOT_FUNC)
    genPal(INITPAL_FUNC)
    genPal(WRENTRY_FUNC)
    genPal(SWPIRQL_FUNC)
    genPal(RDIRQL_FUNC)
    genPal(DI_FUNC)
    genPal(EI_FUNC)
    genPal(SWPPAL_FUNC)
    genPal(SSIR_FUNC)
    genPal(CSIR_FUNC)
    genPal(RFE_FUNC)
    genPal(RETSYS_FUNC)
    genPal(SWPCTX_FUNC)
    genPal(SWPPROCESS_FUNC)
    genPal(RDMCES_FUNC)
    genPal(WRMCES_FUNC)
    genPal(TBIA_FUNC)
    genPal(TBIS_FUNC)
    genPal(TBISASN_FUNC)
    genPal(DTBIS_FUNC)
    genPal(RDKSP_FUNC)
    genPal(SWPKSP_FUNC)
    genPal(RDPSR_FUNC)
    genPal(RDPCR_FUNC)
    genPal(RDTHREAD_FUNC)
    genPal(TBIM_FUNC)
    genPal(TBIMASN_FUNC)
    genPal(TBIM64_FUNC)
    genPal(TBIS64_FUNC)
    genPal(EALNFIX_FUNC)
    genPal(DALNFIX_FUNC)
    genPal(RDCOUNTERS_FUNC)
    genPal(RDSTATE_FUNC)
    genPal(WRPERFMON_FUNC)
    genPal(CP_SLEEP_FUNC)
    // 21064 (EV4) - specific functions
    genPal(INITPCR_FUNC)

    genSpc()
    genTxt("// end callpal\n")
    genSpc()

    //
    // Define Bios Argument structure definitions.
    //

    genCom("Bios Argument Structure Definitions")

    genDef(Ba, X86_BIOS_ARGUMENTS, Eax)
    genDef(Ba, X86_BIOS_ARGUMENTS, Ebx)
    genDef(Ba, X86_BIOS_ARGUMENTS, Ecx)
    genDef(Ba, X86_BIOS_ARGUMENTS, Edx)
    genDef(Ba, X86_BIOS_ARGUMENTS, Esi)
    genDef(Ba, X86_BIOS_ARGUMENTS, Edi)
    genDef(Ba, X86_BIOS_ARGUMENTS, Ebp)
    genVal(BiosArgumentLength, sizeof(X86_BIOS_ARGUMENTS))

    genCom("Define Vendor Callback Read/Write Error Frame Operation Types")

    genVal(ReadFrame, ReadFrame)
    genVal(WriteFrame, WriteFrame)

    genCom("Define Vendor Callback Vector Base Address")

    genValUint(SYSTEM_VECTOR_BASE,
               (ULONG_PTR)SYSTEM_BLOCK + OFFSET(SYSTEM_PARAMETER_BLOCK, VendorVector))

    genCom("Define Vendor Callback Offsets")

    genVal(VnCallBiosRoutine, CallBiosRoutine * 4)
    genVal(VnReadWriteErrorFrameRoutine, ReadWriteErrorFrameRoutine * 4)
    genVal(VnVideoDisplayInitializeRoutine, VideoDisplayInitializeRoutine * 4)

    genCom("Define Firmware Callback Vector Base Address")

    genValUint(FIRMWARE_VECTOR_BASE,
               (ULONG_PTR)SYSTEM_BLOCK + OFFSET(SYSTEM_PARAMETER_BLOCK, FirmwareVector))

    genCom("Define Firmware Callback Offsets")

    genVal(FwGetEnvironmentRoutine, GetEnvironmentRoutine * 4)
    genVal(FwSetEnvironmentRoutine, SetEnvironmentRoutine * 4)

    //
    // Close out conditional statement.
    //

  EnableInc(KSALPHA | HALALPHA)

    END_LIST

};
