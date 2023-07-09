/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    genia64.c

Abstract:

    This module implements a program which generates IA64 machine dependent
    structure offset definitions for kernel structures that are accessed in
    assembly code.

Author:

    David N. Cutler (davec) 27-Mar-1990

Revision History:

    Allen M. Kay (akay) 25-Jan-1996          Modified this file for IA64.

    Forrest Foltz (forrestf) 24-Jan-1998

        Modified format to use new obj-based procedure.

--*/

#include "ki.h"
#pragma hdrstop
#define HEADER_FILE
#include "nt.h"
#include "excpt.h"
#include "ntdef.h"
#include "ntkeapi.h"
#include "ntia64.h"
#include "ntimage.h"
#include "ntseapi.h"
#include "ntobapi.h"
#include "ntlpcapi.h"
#include "ntioapi.h"
#include "ntmmapi.h"
#include "ntldr.h"
#include "ntpsapi.h"
#include "ntexapi.h"
#include "ntnls.h"
#include "nturtl.h"
#include "ntcsrmsg.h"
#include "ntcsrsrv.h"
#include "ntxcapi.h"
#include "ia64.h"
#include "arc.h"
#include "ntstatus.h"
#include "kxia64.h"
#include "stdarg.h"
#include "setjmp.h"

include(`..\genxx.h')

#define KSIA64 KERNEL
#define HALIA64 HAL

STRUC_ELEMENT ElementList[] = {

    START_LIST

    //
    // Include statement for IA64 architecture static definitions.
    //

  EnableInc (KSIA64 | HALIA64)
    genTxt("#include \"kxia64.h\"\n")
  DisableInc (HALIA64)

    //
    // Include IA64 register definition.
    //

  EnableInc (KSIA64 | HALIA64)
    genTxt("#include \"regia64.h\"\n")
  DisableInc (HALIA64)

    //
    // Include architecture independent definitions.
    //

#include "..\genxx.inc"

    //
    // Generate architecture dependent definitions.
    //

    //
    // Processor control register structure definitions.
    //

    genCom("Processor Control Registers Structure Offset Definitions")

    genNam(PCR_MINOR_VERSION)
    genNam(PCR_MAJOR_VERSION)

    genDef(Pc, KPCR, MinorVersion)
    genDef(Pc, KPCR, MajorVersion)
    genDef(Pc, KPCR, InterruptRoutine)
    genDef(Pc, KPCR, FirstLevelDcacheSize)
    genDef(Pc, KPCR, FirstLevelDcacheFillSize)
    genDef(Pc, KPCR, FirstLevelIcacheSize)
    genDef(Pc, KPCR, FirstLevelIcacheFillSize)
    genDef(Pc, KPCR, SecondLevelDcacheSize)
    genDef(Pc, KPCR, SecondLevelDcacheFillSize)
    genDef(Pc, KPCR, SecondLevelIcacheSize)
    genDef(Pc, KPCR, SecondLevelIcacheFillSize)
    genDef(Pc, KPCR, Prcb)
    genDef(Pc, KPCR, DcacheAlignment)
    genDef(Pc, KPCR, DcacheFillSize)
    genDef(Pc, KPCR, IcacheAlignment)
    genDef(Pc, KPCR, IcacheFillSize)
    genDef(Pc, KPCR, ProcessorId)
    genDef(Pc, KPCR, ProfileInterval)
    genDef(Pc, KPCR, ProfileCount)
    genDef(Pc, KPCR, StallExecutionCount)
    genDef(Pc, KPCR, StallScaleFactor)
    genDef(Pc, KPCR, Number)
    genDef(Pc, KPCR, DebugActive)
    genDef(Pc, KPCR, KernelDebugActive)
    genDef(Pc, KPCR, CurrentIrql)
    genDef(Pc, KPCR, SoftwareInterruptPending)
    genDef(Pc, KPCR, ApcInterrupt)
    genDef(Pc, KPCR, DispatchInterrupt)
    genDef(Pc, KPCR, IrqlMask)
    genDef(Pc, KPCR, IrqlTable)
    genDef(Pc, KPCR, SetMember)
    genDef(Pc, KPCR, CurrentThread)
    genDef(Pc, KPCR, NotMember)
    genDef(Pc, KPCR, SystemReserved)
    genDef(Pc, KPCR, HalReserved)
    genDef(Pc, KPCR, KernelGP)
    genDef(Pc, KPCR, InitialStack)
    genDef(Pc, KPCR, InitialBStore)
    genDef(Pc, KPCR, StackLimit)
    genDef(Pc, KPCR, BStoreLimit)
    genDef(Pc, KPCR, PanicStack)
    genDef(Pc, KPCR, SavedIIM)
    genDef(Pc, KPCR, SavedIFA)
    genDef(Pc, KPCR, ForwardProgressBuffer)
    genDef(Pc, KPCR, InterruptionCount)

    genNam(MAX_NUMBER_OF_IHISTORY_RECORDS)

  DisableInc (HALIA64)

    genVal(ProcessorControlRegisterLength, ROUND_UP(sizeof(KPCR), 16))
    genDef(Us, KUSER_SHARED_DATA, TickCountLow)
    genDef(Us, KUSER_SHARED_DATA, TickCountMultiplier)
    genDef(Us, KUSER_SHARED_DATA, InterruptTime)
    genDef(Us, KUSER_SHARED_DATA, InterruptHigh2Time)
    genDef(Us, KUSER_SHARED_DATA, SystemLowTime)
    genDef(Us, KUSER_SHARED_DATA, SystemHigh1Time)
    genDef(Us, KUSER_SHARED_DATA, SystemHigh2Time)

    //
    // Processor block structure definitions.
    //

  EnableInc (HALIA64)
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
    genDef(Pb, KPRCB, PcrPage)
    genDef(Pb, KPRCB, SystemReserved)
    genDef(Pb, KPRCB, HalReserved)

  DisableInc (HALIA64)

    genDef(Pb, KPRCB, DpcTime)
    genDef(Pb, KPRCB, InterruptTime)
    genDef(Pb, KPRCB, KernelTime)
    genDef(Pb, KPRCB, UserTime)
    genDef(Pb, KPRCB, AdjustDpcThreshold)
    genDef(Pb, KPRCB, InterruptCount)
    genDef(Pb, KPRCB, DispatchInterruptCount)
    genDef(Pb, KPRCB, ApcBypassCount)
    genDef(Pb, KPRCB, DpcBypassCount)
    genDef(Pb, KPRCB, IpiFrozen)
    genDef(Pb, KPRCB, ProcessorState)
    genDef(Pb, KPRCB, CcFastReadNoWait)
    genDef(Pb, KPRCB, CcFastReadWait)
    genDef(Pb, KPRCB, CcFastReadNotPossible)
    genDef(Pb, KPRCB, CcCopyReadNoWait)
    genDef(Pb, KPRCB, CcCopyReadWait)
    genDef(Pb, KPRCB, CcCopyReadNoWaitMiss)

    genAlt(PbAlignmentFixupCount, KPRCB, KeAlignmentFixupCount)
    genAlt(PbContextSwitches, KPRCB, KeContextSwitches)
    genAlt(PbDcacheFlushCount, KPRCB, KeDcacheFlushCount)
    genAlt(PbExceptionDispatchCount, KPRCB, KeExceptionDispatchCount)
    genAlt(PbFirstLevelTbFills, KPRCB, KeFirstLevelTbFills)
    genAlt(PbFloatingEmulationCount, KPRCB, KeFloatingEmulationCount)
    genAlt(PbIcacheFlushCount, KPRCB, KeIcacheFlushCount)
    genAlt(PbSecondLevelTbFills, KPRCB, KeSecondLevelTbFills)
    genAlt(PbSystemCalls, KPRCB, KeSystemCalls)

    genDef(Pb, KPRCB, ReservedCounter)
    genDef(Pb, KPRCB, CurrentPacket)
    genDef(Pb, KPRCB, TargetSet)
    genDef(Pb, KPRCB, WorkerRoutine)
    genDef(Pb, KPRCB, CachePad1)
    genDef(Pb, KPRCB, RequestSummary)
    genDef(Pb, KPRCB, SignalDone)
    genDef(Pb, KPRCB, DpcInterruptRequested)
    genDef(Pb, KPRCB, MaximumDpcQueueDepth)
    genDef(Pb, KPRCB, MinimumDpcRate)
    genDef(Pb, KPRCB, IpiCounts)
    genDef(Pb, KPRCB, StartCount)
    genDef(Pb, KPRCB, DpcLock)
    genDef(Pb, KPRCB, DpcListHead)
    genDef(Pb, KPRCB, DpcQueueDepth)
    genDef(Pb, KPRCB, DpcCount)
    genDef(Pb, KPRCB, DpcLastCount)
    genDef(Pb, KPRCB, DpcRequestRate)
    genDef(Pb, KPRCB, DpcRoutineActive)
    genDef(Pb, KPRCB, QuantumEnd)
    genDef(Pb, KPRCB, SkipTick)
    genVal(ProcessorBlockLength, ROUND_UP(sizeof(KPRCB), 16))

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
    genDef(Ic, KIPI_COUNTS, FlushMultipleTb)
    genDef(Ic, KIPI_COUNTS, FlushEntireTb)
    genDef(Ic, KIPI_COUNTS, GenericCall)
    genDef(Ic, KIPI_COUNTS, ChangeColor)
    genDef(Ic, KIPI_COUNTS, SweepDcache)
    genDef(Ic, KIPI_COUNTS, SweepIcache)
    genDef(Ic, KIPI_COUNTS, SweepIcacheRange)
    genDef(Ic, KIPI_COUNTS, FlushIoBuffers)
    genDef(Ic, KIPI_COUNTS, GratuitousDPC)

    //
    // Context frame offset definitions and flag definitions.
    //

  EnableInc (HALIA64)
    genCom("Context Frame Offset and Flag Definitions")

    genNam(CONTEXT_FULL)
    genNam(CONTEXT_CONTROL)
    genNam(CONTEXT_INTEGER)
    genNam(CONTEXT_LOWER_FLOATING_POINT)
    genNam(CONTEXT_HIGHER_FLOATING_POINT)
    genNam(CONTEXT_FLOATING_POINT)
    genNam(CONTEXT_DEBUG)
    genSpc()

    genDef(Cx, CONTEXT, ContextFlags)
    genSpc()

    genDef(Cx, CONTEXT, DbI0)
    genDef(Cx, CONTEXT, DbI1)
    genDef(Cx, CONTEXT, DbI2)
    genDef(Cx, CONTEXT, DbI3)
    genDef(Cx, CONTEXT, DbI4)
    genDef(Cx, CONTEXT, DbI5)
    genDef(Cx, CONTEXT, DbI6)
    genDef(Cx, CONTEXT, DbI7)
    genSpc()

    genDef(Cx, CONTEXT, DbD0)
    genDef(Cx, CONTEXT, DbD1)
    genDef(Cx, CONTEXT, DbD2)
    genDef(Cx, CONTEXT, DbD3)
    genDef(Cx, CONTEXT, DbD4)
    genDef(Cx, CONTEXT, DbD5)
    genDef(Cx, CONTEXT, DbD6)
    genDef(Cx, CONTEXT, DbD7)
    genSpc()

    genDef(Cx, CONTEXT, FltS0)
    genDef(Cx, CONTEXT, FltS1)
    genDef(Cx, CONTEXT, FltS2)
    genDef(Cx, CONTEXT, FltS3)
    genSpc()

    genDef(Cx, CONTEXT, FltT0)
    genDef(Cx, CONTEXT, FltT1)
    genDef(Cx, CONTEXT, FltT2)
    genDef(Cx, CONTEXT, FltT3)
    genDef(Cx, CONTEXT, FltT4)
    genDef(Cx, CONTEXT, FltT5)
    genDef(Cx, CONTEXT, FltT6)
    genDef(Cx, CONTEXT, FltT7)
    genDef(Cx, CONTEXT, FltT8)
    genDef(Cx, CONTEXT, FltT9)
    genSpc()

    genDef(Cx, CONTEXT, FltS4)
    genDef(Cx, CONTEXT, FltS5)
    genDef(Cx, CONTEXT, FltS6)
    genDef(Cx, CONTEXT, FltS7)
    genDef(Cx, CONTEXT, FltS8)
    genDef(Cx, CONTEXT, FltS9)
    genDef(Cx, CONTEXT, FltS10)
    genDef(Cx, CONTEXT, FltS11)
    genDef(Cx, CONTEXT, FltS12)
    genDef(Cx, CONTEXT, FltS13)
    genDef(Cx, CONTEXT, FltS14)
    genDef(Cx, CONTEXT, FltS15)
    genDef(Cx, CONTEXT, FltS16)
    genDef(Cx, CONTEXT, FltS17)
    genDef(Cx, CONTEXT, FltS18)
    genDef(Cx, CONTEXT, FltS19)
    genSpc()

    genDef(Cx, CONTEXT, FltF32)
    genDef(Cx, CONTEXT, FltF33)
    genDef(Cx, CONTEXT, FltF34)
    genDef(Cx, CONTEXT, FltF35)
    genDef(Cx, CONTEXT, FltF36)
    genDef(Cx, CONTEXT, FltF37)
    genDef(Cx, CONTEXT, FltF38)
    genDef(Cx, CONTEXT, FltF39)
    genSpc()

    genDef(Cx, CONTEXT, FltF40)
    genDef(Cx, CONTEXT, FltF41)
    genDef(Cx, CONTEXT, FltF42)
    genDef(Cx, CONTEXT, FltF43)
    genDef(Cx, CONTEXT, FltF44)
    genDef(Cx, CONTEXT, FltF45)
    genDef(Cx, CONTEXT, FltF46)
    genDef(Cx, CONTEXT, FltF47)
    genDef(Cx, CONTEXT, FltF48)
    genDef(Cx, CONTEXT, FltF49)
    genSpc()

    genDef(Cx, CONTEXT, FltF50)
    genDef(Cx, CONTEXT, FltF51)
    genDef(Cx, CONTEXT, FltF52)
    genDef(Cx, CONTEXT, FltF53)
    genDef(Cx, CONTEXT, FltF54)
    genDef(Cx, CONTEXT, FltF55)
    genDef(Cx, CONTEXT, FltF56)
    genDef(Cx, CONTEXT, FltF57)
    genDef(Cx, CONTEXT, FltF58)
    genDef(Cx, CONTEXT, FltF59)
    genSpc()

    genDef(Cx, CONTEXT, FltF60)
    genDef(Cx, CONTEXT, FltF61)
    genDef(Cx, CONTEXT, FltF62)
    genDef(Cx, CONTEXT, FltF63)
    genDef(Cx, CONTEXT, FltF64)
    genDef(Cx, CONTEXT, FltF65)
    genDef(Cx, CONTEXT, FltF66)
    genDef(Cx, CONTEXT, FltF67)
    genDef(Cx, CONTEXT, FltF68)
    genDef(Cx, CONTEXT, FltF69)
    genSpc()

    genDef(Cx, CONTEXT, FltF70)
    genDef(Cx, CONTEXT, FltF71)
    genDef(Cx, CONTEXT, FltF72)
    genDef(Cx, CONTEXT, FltF73)
    genDef(Cx, CONTEXT, FltF74)
    genDef(Cx, CONTEXT, FltF75)
    genDef(Cx, CONTEXT, FltF76)
    genDef(Cx, CONTEXT, FltF77)
    genDef(Cx, CONTEXT, FltF78)
    genDef(Cx, CONTEXT, FltF79)
    genSpc()

    genDef(Cx, CONTEXT, FltF80)
    genDef(Cx, CONTEXT, FltF81)
    genDef(Cx, CONTEXT, FltF82)
    genDef(Cx, CONTEXT, FltF83)
    genDef(Cx, CONTEXT, FltF84)
    genDef(Cx, CONTEXT, FltF85)
    genDef(Cx, CONTEXT, FltF86)
    genDef(Cx, CONTEXT, FltF87)
    genDef(Cx, CONTEXT, FltF88)
    genDef(Cx, CONTEXT, FltF89)
    genSpc()

    genDef(Cx, CONTEXT, FltF90)
    genDef(Cx, CONTEXT, FltF91)
    genDef(Cx, CONTEXT, FltF92)
    genDef(Cx, CONTEXT, FltF93)
    genDef(Cx, CONTEXT, FltF94)
    genDef(Cx, CONTEXT, FltF95)
    genDef(Cx, CONTEXT, FltF96)
    genDef(Cx, CONTEXT, FltF97)
    genDef(Cx, CONTEXT, FltF98)
    genDef(Cx, CONTEXT, FltF99)
    genSpc()

    genDef(Cx, CONTEXT, FltF100)
    genDef(Cx, CONTEXT, FltF101)
    genDef(Cx, CONTEXT, FltF102)
    genDef(Cx, CONTEXT, FltF103)
    genDef(Cx, CONTEXT, FltF104)
    genDef(Cx, CONTEXT, FltF105)
    genDef(Cx, CONTEXT, FltF106)
    genDef(Cx, CONTEXT, FltF107)
    genDef(Cx, CONTEXT, FltF108)
    genDef(Cx, CONTEXT, FltF109)
    genSpc()

    genDef(Cx, CONTEXT, FltF110)
    genDef(Cx, CONTEXT, FltF111)
    genDef(Cx, CONTEXT, FltF112)
    genDef(Cx, CONTEXT, FltF113)
    genDef(Cx, CONTEXT, FltF114)
    genDef(Cx, CONTEXT, FltF115)
    genDef(Cx, CONTEXT, FltF116)
    genDef(Cx, CONTEXT, FltF117)
    genDef(Cx, CONTEXT, FltF118)
    genDef(Cx, CONTEXT, FltF119)
    genSpc()

    genDef(Cx, CONTEXT, FltF120)
    genDef(Cx, CONTEXT, FltF121)
    genDef(Cx, CONTEXT, FltF122)
    genDef(Cx, CONTEXT, FltF123)
    genDef(Cx, CONTEXT, FltF124)
    genDef(Cx, CONTEXT, FltF125)
    genDef(Cx, CONTEXT, FltF126)
    genDef(Cx, CONTEXT, FltF127)
    genSpc()

    genDef(Cx, CONTEXT, StFPSR)
    genSpc()

    genDef(Cx, CONTEXT, IntGp)
    genDef(Cx, CONTEXT, IntT0)
    genDef(Cx, CONTEXT, IntT1)
    genDef(Cx, CONTEXT, IntS0)
    genDef(Cx, CONTEXT, IntS1)
    genDef(Cx, CONTEXT, IntS2)
    genDef(Cx, CONTEXT, IntS3)
    genDef(Cx, CONTEXT, IntV0)
    genDef(Cx, CONTEXT, IntT2)
    genDef(Cx, CONTEXT, IntT3)
    genDef(Cx, CONTEXT, IntT4)
    genDef(Cx, CONTEXT, IntSp)
    genDef(Cx, CONTEXT, IntTeb)
    genDef(Cx, CONTEXT, IntT5)
    genDef(Cx, CONTEXT, IntT6)
    genDef(Cx, CONTEXT, IntT7)
    genDef(Cx, CONTEXT, IntT8)
    genDef(Cx, CONTEXT, IntT9)
    genSpc()

    genDef(Cx, CONTEXT, IntT10)
    genDef(Cx, CONTEXT, IntT11)
    genDef(Cx, CONTEXT, IntT12)
    genDef(Cx, CONTEXT, IntT13)
    genDef(Cx, CONTEXT, IntT14)
    genDef(Cx, CONTEXT, IntT15)
    genDef(Cx, CONTEXT, IntT16)
    genDef(Cx, CONTEXT, IntT17)
    genDef(Cx, CONTEXT, IntT18)
    genDef(Cx, CONTEXT, IntT19)
    genDef(Cx, CONTEXT, IntT20)
    genDef(Cx, CONTEXT, IntT21)
    genDef(Cx, CONTEXT, IntT22)
    genSpc()

    genDef(Cx, CONTEXT, IntNats)
    genDef(Cx, CONTEXT, Preds)
    genSpc()

    genDef(Cx, CONTEXT, BrRp)
    genDef(Cx, CONTEXT, BrS0)
    genDef(Cx, CONTEXT, BrS1)
    genDef(Cx, CONTEXT, BrS2)
    genDef(Cx, CONTEXT, BrS3)
    genDef(Cx, CONTEXT, BrS4)
    genDef(Cx, CONTEXT, BrT0)
    genDef(Cx, CONTEXT, BrT1)
    genSpc()

    genDef(Cx, CONTEXT, ApUNAT)
    genDef(Cx, CONTEXT, ApLC)
    genDef(Cx, CONTEXT, ApEC)
    genDef(Cx, CONTEXT, ApCCV)
    genDef(Cx, CONTEXT, ApDCR)
    genDef(Cx, CONTEXT, RsPFS)
    genDef(Cx, CONTEXT, RsBSP)
    genDef(Cx, CONTEXT, RsBSPSTORE)
    genDef(Cx, CONTEXT, RsRSC)
    genDef(Cx, CONTEXT, RsRNAT)


    genDef(Cx, CONTEXT, StIPSR)
    genDef(Cx, CONTEXT, StIIP)
    genDef(Cx, CONTEXT, StIFS)
    genSpc()

    genDef(Cx, CONTEXT, StFCR)
    genDef(Cx, CONTEXT, Eflag)
    genDef(Cx, CONTEXT, SegCSD)
    genDef(Cx, CONTEXT, SegSSD)
    genDef(Cx, CONTEXT, Cflag)
    genDef(Cx, CONTEXT, StFSR)
    genDef(Cx, CONTEXT, StFIR)
    genDef(Cx, CONTEXT, StFDR)
    genSpc()

    genVal(ContextFrameLength, ROUND_UP(sizeof(CONTEXT), 16))
    genSpc()

    //
    // Application registers offset definitions
    //

    genCom("Debug Register Offset Definitions and Length")

    genDef(Ts, KAPPLICATION_REGISTERS, Ar21)
    genDef(Ts, KAPPLICATION_REGISTERS, Ar24)
    genDef(Ts, KAPPLICATION_REGISTERS, Ar25)
    genDef(Ts, KAPPLICATION_REGISTERS, Ar26)
    genDef(Ts, KAPPLICATION_REGISTERS, Ar27)
    genDef(Ts, KAPPLICATION_REGISTERS, Ar28)
    genDef(Ts, KAPPLICATION_REGISTERS, Ar29)
    genDef(Ts, KAPPLICATION_REGISTERS, Ar30)

    //
    // Higher FP volatile offset definitions.
    //

    genCom("Higher FP Volatile Offset Definitions and Length")

    genDef(Hi, KHIGHER_FP_VOLATILE, FltF32)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF33)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF34)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF35)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF36)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF37)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF38)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF39)
    genSpc()

    genDef(Hi, KHIGHER_FP_VOLATILE, FltF40)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF41)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF42)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF43)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF44)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF45)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF46)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF47)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF48)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF49)
    genSpc()

    genDef(Hi, KHIGHER_FP_VOLATILE, FltF50)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF51)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF52)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF53)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF54)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF55)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF56)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF57)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF58)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF59)
    genSpc()

    genDef(Hi, KHIGHER_FP_VOLATILE, FltF60)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF61)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF62)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF63)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF64)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF65)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF66)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF67)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF68)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF69)
    genSpc()

    genDef(Hi, KHIGHER_FP_VOLATILE, FltF70)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF71)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF72)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF73)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF74)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF75)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF76)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF77)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF78)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF79)
    genSpc()

    genDef(Hi, KHIGHER_FP_VOLATILE, FltF80)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF81)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF82)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF83)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF84)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF85)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF86)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF87)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF88)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF89)
    genSpc()

    genDef(Hi, KHIGHER_FP_VOLATILE, FltF90)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF91)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF92)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF93)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF94)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF95)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF96)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF97)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF98)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF99)
    genSpc()

    genDef(Hi, KHIGHER_FP_VOLATILE, FltF100)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF101)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF102)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF103)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF104)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF105)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF106)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF107)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF108)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF109)
    genSpc()

    genDef(Hi, KHIGHER_FP_VOLATILE, FltF110)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF111)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF112)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF113)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF114)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF115)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF116)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF117)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF118)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF119)
    genSpc()

    genDef(Hi, KHIGHER_FP_VOLATILE, FltF120)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF121)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF122)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF123)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF124)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF125)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF126)
    genDef(Hi, KHIGHER_FP_VOLATILE, FltF127)
    genSpc()

    //
    // Debug registers offset definitions
    //

    genCom("Debug Register Offset Definitions and Length")

    genDef(Dr, KDEBUG_REGISTERS, DbI0)
    genDef(Dr, KDEBUG_REGISTERS, DbI1)
    genDef(Dr, KDEBUG_REGISTERS, DbI2)
    genDef(Dr, KDEBUG_REGISTERS, DbI3)
    genDef(Dr, KDEBUG_REGISTERS, DbI4)
    genDef(Dr, KDEBUG_REGISTERS, DbI5)
    genDef(Dr, KDEBUG_REGISTERS, DbI6)
    genDef(Dr, KDEBUG_REGISTERS, DbI7)
    genSpc()
    
    genDef(Dr, KDEBUG_REGISTERS, DbD0)
    genDef(Dr, KDEBUG_REGISTERS, DbD1)
    genDef(Dr, KDEBUG_REGISTERS, DbD2)
    genDef(Dr, KDEBUG_REGISTERS, DbD3)
    genDef(Dr, KDEBUG_REGISTERS, DbD4)
    genDef(Dr, KDEBUG_REGISTERS, DbD5)
    genDef(Dr, KDEBUG_REGISTERS, DbD6)
    genDef(Dr, KDEBUG_REGISTERS, DbD7)
    genSpc()

    //
    // Thread State Save Area length
    //

    genDef(Ts, KTHREAD_STATE_SAVEAREA, AppRegisters)
    genDef(Ts, KTHREAD_STATE_SAVEAREA, PerfRegisters)
    genDef(Ts, KTHREAD_STATE_SAVEAREA, HigherFPVolatile)
    genDef(Ts, KTHREAD_STATE_SAVEAREA, DebugRegisters)
    genVal(ThreadStateSaveAreaLength, ROUND_UP(sizeof(KTHREAD_STATE_SAVEAREA), 16))
    
    //
    // Exception frame offset definitions.
    //

    genCom("Exception Frame Offset Definitions and Length")

    genDef(Ex, KEXCEPTION_FRAME, FltS0)
    genDef(Ex, KEXCEPTION_FRAME, FltS1)
    genDef(Ex, KEXCEPTION_FRAME, FltS2)
    genDef(Ex, KEXCEPTION_FRAME, FltS3)
    genDef(Ex, KEXCEPTION_FRAME, FltS4)
    genDef(Ex, KEXCEPTION_FRAME, FltS5)
    genDef(Ex, KEXCEPTION_FRAME, FltS6)
    genDef(Ex, KEXCEPTION_FRAME, FltS7)
    genDef(Ex, KEXCEPTION_FRAME, FltS8)
    genDef(Ex, KEXCEPTION_FRAME, FltS9)
    genDef(Ex, KEXCEPTION_FRAME, FltS10)
    genDef(Ex, KEXCEPTION_FRAME, FltS11)
    genDef(Ex, KEXCEPTION_FRAME, FltS12)
    genDef(Ex, KEXCEPTION_FRAME, FltS13)
    genDef(Ex, KEXCEPTION_FRAME, FltS14)
    genDef(Ex, KEXCEPTION_FRAME, FltS15)
    genDef(Ex, KEXCEPTION_FRAME, FltS16)
    genDef(Ex, KEXCEPTION_FRAME, FltS17)
    genDef(Ex, KEXCEPTION_FRAME, FltS18)
    genDef(Ex, KEXCEPTION_FRAME, FltS19)
    genSpc()

    genDef(Ex, KEXCEPTION_FRAME, IntS0)
    genDef(Ex, KEXCEPTION_FRAME, IntS1)
    genDef(Ex, KEXCEPTION_FRAME, IntS2)
    genDef(Ex, KEXCEPTION_FRAME, IntS3)
    genDef(Ex, KEXCEPTION_FRAME, IntNats)
    genSpc()

    genDef(Ex, KEXCEPTION_FRAME, BrS0)
    genDef(Ex, KEXCEPTION_FRAME, BrS1)
    genDef(Ex, KEXCEPTION_FRAME, BrS2)
    genDef(Ex, KEXCEPTION_FRAME, BrS3)
    genDef(Ex, KEXCEPTION_FRAME, BrS4)
    genSpc()

    genDef(Ex, KEXCEPTION_FRAME, ApEC)
    genDef(Ex, KEXCEPTION_FRAME, ApLC)
    genSpc()

    genVal(ExceptionFrameLength, ROUND_UP(sizeof(KEXCEPTION_FRAME), 16))

    //
    // Switch frame offset definitions.
    //

    genCom("Switch Frame Offset Definitions and Length")

    genAlt(SwExFrame, KSWITCH_FRAME, SwitchExceptionFrame)
    genAlt(SwPreds, KSWITCH_FRAME, SwitchPredicates)
    genAlt(SwRp, KSWITCH_FRAME, SwitchRp)
    genAlt(SwPFS, KSWITCH_FRAME, SwitchPFS)
    genAlt(SwFPSR, KSWITCH_FRAME, SwitchFPSR)
    genAlt(SwBsp, KSWITCH_FRAME, SwitchBsp)
    genAlt(SwRnat, KSWITCH_FRAME, SwitchRnat)
    genSpc()

    genVal(SwitchFrameLength, ROUND_UP(sizeof(KSWITCH_FRAME), 16))

    //
    // Plabel structure offset definitions.
    //

    genCom("Plabel structure offset definitions")

    genDef(Pl, PLABEL_DESCRIPTOR, EntryPoint)
    genDef(Pl, PLABEL_DESCRIPTOR, GlobalPointer)

  DisableInc (HALIA64)

    //
    // Jump buffer offset definitions.
    //

    genCom("Jump Offset Definitions and Length")

    genDef(Jb, _JUMP_BUFFER, Registration)
    genDef(Jb, _JUMP_BUFFER, TryLevel)
    genDef(Jb, _JUMP_BUFFER, Cookie)
    genDef(Jb, _JUMP_BUFFER, UnwindFunc)
    genDef(Jb, _JUMP_BUFFER, UnwindData)
    genSpc()

    genDef(Jb, _JUMP_BUFFER, FPSR)
    genDef(Jb, _JUMP_BUFFER, FltS0)
    genDef(Jb, _JUMP_BUFFER, FltS1)
    genDef(Jb, _JUMP_BUFFER, FltS2)
    genDef(Jb, _JUMP_BUFFER, FltS3)
    genDef(Jb, _JUMP_BUFFER, FltS4)
    genDef(Jb, _JUMP_BUFFER, FltS5)
    genDef(Jb, _JUMP_BUFFER, FltS6)
    genDef(Jb, _JUMP_BUFFER, FltS7)
    genDef(Jb, _JUMP_BUFFER, FltS8)
    genDef(Jb, _JUMP_BUFFER, FltS9)
    genSpc()

    genDef(Jb, _JUMP_BUFFER, FltS10)
    genDef(Jb, _JUMP_BUFFER, FltS11)
    genDef(Jb, _JUMP_BUFFER, FltS12)
    genDef(Jb, _JUMP_BUFFER, FltS13)
    genDef(Jb, _JUMP_BUFFER, FltS14)
    genDef(Jb, _JUMP_BUFFER, FltS15)
    genDef(Jb, _JUMP_BUFFER, FltS16)
    genDef(Jb, _JUMP_BUFFER, FltS17)
    genDef(Jb, _JUMP_BUFFER, FltS18)
    genDef(Jb, _JUMP_BUFFER, FltS19)
    genSpc()

    genDef(Jb, _JUMP_BUFFER, StIIP)
    genDef(Jb, _JUMP_BUFFER, BrS0)
    genDef(Jb, _JUMP_BUFFER, BrS1)
    genDef(Jb, _JUMP_BUFFER, BrS2)
    genDef(Jb, _JUMP_BUFFER, BrS3)
    genDef(Jb, _JUMP_BUFFER, BrS4)
    genSpc()

    genDef(Jb, _JUMP_BUFFER, RsBSP)
    genDef(Jb, _JUMP_BUFFER, RsPFS)
    genDef(Jb, _JUMP_BUFFER, ApUNAT)
    genDef(Jb, _JUMP_BUFFER, ApLC)
    genSpc()

    genDef(Jb, _JUMP_BUFFER, IntS0)
    genDef(Jb, _JUMP_BUFFER, IntS1)
    genDef(Jb, _JUMP_BUFFER, IntS2)
    genDef(Jb, _JUMP_BUFFER, IntS3)
    genDef(Jb, _JUMP_BUFFER, IntSp)
    genDef(Jb, _JUMP_BUFFER, IntNats)
    genDef(Jb, _JUMP_BUFFER, Preds)
    genSpc()

    genVal(JumpBufferLength, ROUND_UP(sizeof(_JUMP_BUFFER), 16))

    //
    // Trap frame offset definitions.
    //

  EnableInc (HALIA64)
    genCom("Trap Frame Offset Definitions and Length")

    genDef(Tr, KTRAP_FRAME, FltT0)
    genDef(Tr, KTRAP_FRAME, FltT1)
    genDef(Tr, KTRAP_FRAME, FltT2)
    genDef(Tr, KTRAP_FRAME, FltT3)
    genDef(Tr, KTRAP_FRAME, FltT4)
    genDef(Tr, KTRAP_FRAME, FltT5)
    genDef(Tr, KTRAP_FRAME, FltT6)
    genDef(Tr, KTRAP_FRAME, FltT7)
    genDef(Tr, KTRAP_FRAME, FltT8)
    genDef(Tr, KTRAP_FRAME, FltT9)
    genSpc()

    genDef(Tr, KTRAP_FRAME, IntGp)
    genDef(Tr, KTRAP_FRAME, IntT0)
    genDef(Tr, KTRAP_FRAME, IntT1)
    genSpc()

    genDef(Tr, KTRAP_FRAME, ApUNAT)
    genDef(Tr, KTRAP_FRAME, ApCCV)
    genDef(Tr, KTRAP_FRAME, ApDCR)
    genDef(Tr, KTRAP_FRAME, Preds)
    genSpc()

    genDef(Tr, KTRAP_FRAME, IntV0)
    genDef(Tr, KTRAP_FRAME, IntT2)
    genDef(Tr, KTRAP_FRAME, IntT3)
    genDef(Tr, KTRAP_FRAME, IntT4)
    genDef(Tr, KTRAP_FRAME, IntSp)
    genDef(Tr, KTRAP_FRAME, IntTeb)
    genDef(Tr, KTRAP_FRAME, IntT5)
    genDef(Tr, KTRAP_FRAME, IntT6)
    genDef(Tr, KTRAP_FRAME, IntT7)
    genDef(Tr, KTRAP_FRAME, IntT8)
    genDef(Tr, KTRAP_FRAME, IntT9)
    genSpc()

    genDef(Tr, KTRAP_FRAME, IntT10)
    genDef(Tr, KTRAP_FRAME, IntT11)
    genDef(Tr, KTRAP_FRAME, IntT12)
    genDef(Tr, KTRAP_FRAME, IntT13)
    genDef(Tr, KTRAP_FRAME, IntT14)
    genDef(Tr, KTRAP_FRAME, IntT15)
    genDef(Tr, KTRAP_FRAME, IntT16)
    genDef(Tr, KTRAP_FRAME, IntT17)
    genDef(Tr, KTRAP_FRAME, IntT18)
    genDef(Tr, KTRAP_FRAME, IntT19)
    genDef(Tr, KTRAP_FRAME, IntT20)
    genDef(Tr, KTRAP_FRAME, IntT21)
    genDef(Tr, KTRAP_FRAME, IntT22)
    genSpc()

    genDef(Tr, KTRAP_FRAME, IntNats)
    genSpc()

    genDef(Tr, KTRAP_FRAME, BrRp)
    genDef(Tr, KTRAP_FRAME, BrT0)
    genDef(Tr, KTRAP_FRAME, BrT1)
    genSpc()

    genDef(Tr, KTRAP_FRAME, RsPFS)
    genDef(Tr, KTRAP_FRAME, RsBSP)
    genDef(Tr, KTRAP_FRAME, RsRSC)
    genDef(Tr, KTRAP_FRAME, RsRNAT)
    genDef(Tr, KTRAP_FRAME, RsBSPSTORE)
    genSpc()

    genDef(Tr, KTRAP_FRAME, StIPSR)
    genDef(Tr, KTRAP_FRAME, StISR)
    genDef(Tr, KTRAP_FRAME, StIFA)
    genDef(Tr, KTRAP_FRAME, StIIP)
    genDef(Tr, KTRAP_FRAME, StIIPA)
    genDef(Tr, KTRAP_FRAME, StIFS)
    genDef(Tr, KTRAP_FRAME, StIIM)
    genDef(Tr, KTRAP_FRAME, StIHA)
    genDef(Tr, KTRAP_FRAME, StFPSR)
    genSpc()

    genDef(Tr, KTRAP_FRAME, OldIrql)
    genDef(Tr, KTRAP_FRAME, PreviousMode)
    genDef(Tr, KTRAP_FRAME, TrapFrame)
    genDef(Tr, KTRAP_FRAME, Handler)
    genDef(Tr, KTRAP_FRAME, EOFMarker)

    genDef(Tr, KTRAP_FRAME, ExceptionRecord)
    genSpc()

    genVal(TrapFrameLength, ROUND_UP(sizeof(KTRAP_FRAME),16))
    genVal(TrapFrameArguments, KTRAP_FRAME_ARGUMENTS)
    genNamUint(KTRAP_FRAME_EOF)

    //
    // Usermode callout kernel frame definitions
    //

    DisableInc(HALIA64)

    genCom("Usermode callout kernel frame definitions")
    
    genDef(Cu, KCALLOUT_FRAME, BrRp)
    genDef(Cu, KCALLOUT_FRAME, RsPFS)
    genDef(Cu, KCALLOUT_FRAME, Preds)
    genDef(Cu, KCALLOUT_FRAME, ApUNAT)
    genDef(Cu, KCALLOUT_FRAME, ApLC)
    genDef(Cu, KCALLOUT_FRAME, IntS0)
    genDef(Cu, KCALLOUT_FRAME, IntS1)
    genDef(Cu, KCALLOUT_FRAME, IntS2)
    genDef(Cu, KCALLOUT_FRAME, IntS3)
    genDef(Cu, KCALLOUT_FRAME, BrS0)
    genDef(Cu, KCALLOUT_FRAME, BrS1)
    genDef(Cu, KCALLOUT_FRAME, BrS2)
    genDef(Cu, KCALLOUT_FRAME, BrS3)
    genDef(Cu, KCALLOUT_FRAME, BrS4)
    genDef(Cu, KCALLOUT_FRAME, RsRNAT)
    genDef(Cu, KCALLOUT_FRAME, IntNats)
    genDef(Cu, KCALLOUT_FRAME, FltS0)
    genDef(Cu, KCALLOUT_FRAME, FltS1)
    genDef(Cu, KCALLOUT_FRAME, FltS2)
    genDef(Cu, KCALLOUT_FRAME, FltS3)
    genDef(Cu, KCALLOUT_FRAME, FltS4)
    genDef(Cu, KCALLOUT_FRAME, FltS5)
    genDef(Cu, KCALLOUT_FRAME, FltS6)
    genDef(Cu, KCALLOUT_FRAME, FltS7)
    genDef(Cu, KCALLOUT_FRAME, FltS8)
    genDef(Cu, KCALLOUT_FRAME, FltS9)
    genDef(Cu, KCALLOUT_FRAME, FltS10)
    genDef(Cu, KCALLOUT_FRAME, FltS11)
    genDef(Cu, KCALLOUT_FRAME, FltS12)
    genDef(Cu, KCALLOUT_FRAME, FltS13)
    genDef(Cu, KCALLOUT_FRAME, FltS14)
    genDef(Cu, KCALLOUT_FRAME, FltS15)
    genDef(Cu, KCALLOUT_FRAME, FltS16)
    genDef(Cu, KCALLOUT_FRAME, FltS17)
    genDef(Cu, KCALLOUT_FRAME, FltS18)
    genDef(Cu, KCALLOUT_FRAME, FltS19)
    genDef(Cu, KCALLOUT_FRAME, A0)
    genDef(Cu, KCALLOUT_FRAME, A1)
    genDef(Cu, KCALLOUT_FRAME, CbStk)
    genDef(Cu, KCALLOUT_FRAME, InStack)
    genDef(Cu, KCALLOUT_FRAME, CbBStore)
    genDef(Cu, KCALLOUT_FRAME, InBStore)
    genDef(Cu, KCALLOUT_FRAME, TrFrame)
    genDef(Cu, KCALLOUT_FRAME, TrStIIP)
    genVal(CuFrameLength, sizeof(KCALLOUT_FRAME))

    genCom("Usermode callout user frame definitions")

    genDef(Ck, UCALLOUT_FRAME, Buffer)
    genDef(Ck, UCALLOUT_FRAME, Length)
    genDef(Ck, UCALLOUT_FRAME, ApiNumber)
    genDef(Ck, UCALLOUT_FRAME, IntSp)
    genDef(Ck, UCALLOUT_FRAME, RsPFS)
    genDef(Ck, UCALLOUT_FRAME, BrRp)

    EnableInc(HALIA64)

    //
    // Loader Parameter Block offset definitions.
    //

    genCom("Loader Parameter Block Offset Definitions")

    genDef(Lpb, LOADER_PARAMETER_BLOCK, LoadOrderListHead)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, MemoryDescriptorListHead)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, KernelStack)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, Prcb)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, Process)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, Thread)

    genAlt(LpbSalSystemTable,
              LOADER_PARAMETER_BLOCK, u.Ia64.SalSystemTable)

    genAlt(LpbMPSConfigTable,
              LOADER_PARAMETER_BLOCK, u.Ia64.MPSConfigTable)

    genAlt(LpbAcpiRsdt,
              LOADER_PARAMETER_BLOCK, u.Ia64.AcpiRsdt)

    genAlt(LpbKernelPhysicalBase,
              LOADER_PARAMETER_BLOCK, u.Ia64.KernelPhysicalBase)

    genAlt(LpbKernelVirtualBase,
              LOADER_PARAMETER_BLOCK, u.Ia64.KernelVirtualBase)

    genAlt(LpbInterruptStack,
              LOADER_PARAMETER_BLOCK, u.Ia64.InterruptStack)

    genAlt(LpbPanicStack,
              LOADER_PARAMETER_BLOCK, u.Ia64.PanicStack)

    genAlt(LpbPcrPage,
              LOADER_PARAMETER_BLOCK, u.Ia64.PcrPage)

    genAlt(LpbPdrPage,
              LOADER_PARAMETER_BLOCK, u.Ia64.PdrPage)

    genAlt(LpbPcrPage2,
              LOADER_PARAMETER_BLOCK, u.Ia64.PcrPage2)

    genAlt(LpbMachineType,
              LOADER_PARAMETER_BLOCK, u.Ia64.MachineType)


  DisableInc (HALIA64)

    //
    // Address space layout definitions
    //

  EnableInc (HALIA64)

    genCom("Address Space Layout Definitions")

    genNam(UREGION_INDEX)
    genNamUint(KUSEG_BASE)
    genNamUint(KSEG0_BASE)
    genNamUint(KSEG2_BASE)
    genNamUint(KADDRESS_BASE)
    genNamUint(UADDRESS_BASE)

  DisableInc (HALIA64)
    genNamUint(SYSTEM_BASE)
    genNamUint(PDE_BASE)
    genNamUint(PTE_BASE)
    genNamUint(PDE_KBASE)
    genNamUint(PDE_UBASE)    
    genNamUint(PDE_UTBASE)    
    genNamUint(PDE_KTBASE)    
    genNamUint(KSEG3_BASE)    
    genNamUint(PTA_BASE)
    genNamUint(PDE_STBASE)    

    //
    // Page table and page directory entry definitions
    //

  EnableInc (HALIA64)
    genCom("Page Table and Directory Entry Definitions")

    genNam(PAGE_SIZE)
    genNam(PAGE_SHIFT)
    genNam(PDI_SHIFT)
    genNam(PTI_SHIFT)
    genNam(PTE_SHIFT)
    genNam(VHPT_PDE_BITS)

  DisableInc (HALIA64)

    //
    // Breakpoint instruction definitions
    //

  EnableInc (HALIA64)
    genCom("Breakpoint Definitions")

    genNam(USER_BREAKPOINT)
    genNam(KERNEL_BREAKPOINT)
    genNam(BREAKIN_BREAKPOINT)
  DisableInc (HALIA64)

    genNam(DIVIDE_OVERFLOW_BREAKPOINT)
    genNam(DIVIDE_BY_ZERO_BREAKPOINT)
    genNam(RANGE_CHECK_BREAKPOINT)
    genNam(STACK_OVERFLOW_BREAKPOINT)
    genNam(MULTIPLY_OVERFLOW_BREAKPOINT)
    genNam(DEBUG_PRINT_BREAKPOINT)
    genNam(DEBUG_PROMPT_BREAKPOINT)
    genNam(DEBUG_STOP_BREAKPOINT)
    genNam(DEBUG_LOAD_SYMBOLS_BREAKPOINT)
    genNam(DEBUG_UNLOAD_SYMBOLS_BREAKPOINT)

    //
    // Miscellaneous definitions
    //

    genCom("IA64 Specific Definitions")

    genNam(BREAK_SSI_BASE)
    genNam(BREAK_APP_BASE)
    genNam(BREAK_DEBUG_BASE)
    genNam(BREAK_SYSCALL_BASE)
    genNam(BREAK_SYSCALL)
    genNam(BREAK_FASTSYS_BASE)
    genNam(BREAK_SET_LOW_WAIT_HIGH)
    genNam(BREAK_SET_HIGH_WAIT_LOW)

    genNam(SYSCALL_FRAME)
    genNam(INTERRUPT_FRAME)
    genNam(EXCEPTION_FRAME)
    genNam(CONTEXT_FRAME)
    genSpc()

    //
    // Miscellaneous definitions
    //

  EnableInc (HALIA64)
    genCom("Miscellaneous Definitions")

    genNam(Executive)
    genNam(KernelMode)
    genNam(UserMode)
    genNam(FALSE)
    genNam(TRUE)

    genValUint(KiPcr, KIPCR)
    genValUint(KiPcr2, KI_USER_SHARED_DATA)

  DisableInc (HALIA64)

    genNam(BASE_PRIORITY_THRESHOLD)
    genNam(EVENT_PAIR_INCREMENT)
    genNam(LOW_REALTIME_PRIORITY)
    genNam(KERNEL_STACK_SIZE)
    genNam(KERNEL_BSTORE_SIZE)
    genNam(KERNEL_LARGE_STACK_COMMIT)
    genNam(KERNEL_LARGE_BSTORE_COMMIT)
    genNamUint(MM_USER_PROBE_ADDRESS)
    genNamUint(MM_EPC_VA)
    genNam(THREAD_QUANTUM)
    genNam(CLOCK_QUANTUM_DECREMENT)
    genNam(WAIT_QUANTUM_DECREMENT)
    genNam(READY_SKIP_QUANTUM)
    genNam(ROUND_TRIP_DECREMENT_COUNT)

    END_LIST
};

ASSERT_SAME(KTRAP_FRAME_LENGTH, KIA32_FRAME_LENGTH);
