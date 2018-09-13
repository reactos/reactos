/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    genmips.c

Abstract:

    This module implements a program which generates MIPS machine dependent
    structure offset definitions for kernel structures that are accessed in
    assembly code.

Author:

    David N. Cutler (davec) 27-Mar-1990

Revision History:

--*/

#include "ki.h"
#pragma hdrstop
#define HEADER_FILE
#include "excpt.h"
#include "ntdef.h"
#include "ntkeapi.h"
#include "ntmips.h"
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
#include "arc.h"
#include "ntstatus.h"
#include "kxmips.h"
#include "stdarg.h"
#include "setjmp.h"

//
// Define architecture specific generation macros.
//

#define genAlt(Name, Type, Member) \
    dumpf("#define " #Name " 0x%lx\n", OFFSET(Type, Member))

#define genCom(Comment)        \
    dumpf("\n");               \
    dumpf("//\n");             \
    dumpf("// " Comment "\n"); \
    dumpf("//\n");             \
    dumpf("\n")

#define genDef(Prefix, Type, Member) \
    dumpf("#define " #Prefix #Member " 0x%lx\n", OFFSET(Type, Member))

#define genVal(Name, Value)    \
    dumpf("#define " #Name " 0x%lx\n", Value)

#define genSpc() dumpf("\n");

//
// Define member offset computation macro.
//

#define OFFSET(type, field) ((LONG)(&((type *)0)->field))

FILE *KsMips;
FILE *HalMips;

//
// EnableInc(a) - Enables output to goto specified include file
//

#define EnableInc(a)    OutputEnabled |= a;

//
// DisableInc(a) - Disables output to goto specified include file
//

#define DisableInc(a)   OutputEnabled &= ~a;

ULONG OutputEnabled;

#define KSMIPS      0x01
#define HALMIPS     0x02

#define KERNEL KSMIPS
#define HAL HALMIPS

VOID dumpf (const char *format, ...);


//
// This routine returns the bit number right to left of a field.
//

LONG
t (
    IN ULONG z
    )

{
    LONG i;

    for (i = 0; i < 32; i += 1) {
        if ((z >> i) & 1) {
            break;
        }
    }
    return i;
}

//
// This program generates the MIPS machine dependent assembler offset
// definitions.
//

VOID
main (argc, argv)
    int argc;
    char *argv[];
{

    char *outName;
    LONG EventOffset;

    //
    // Create file for output.
    //

    if (argc == 2) {
        outName = argv[ 1 ];
    } else {
        outName = "\\nt\\public\\sdk\\inc\\ksmips.h";
    }

    outName = argc >= 2 ? argv[1] : "\\nt\\public\\sdk\\inc\\ksmips.h";
    KsMips = fopen( outName, "w" );
    if (KsMips == NULL) {
        fprintf( stderr, "GENMIPS: Cannot open %s for writing.\n", outName);

    } else {
        fprintf(stderr, "GENMIPS: Writing %s header file.\n", outName);
    }

    outName = argc >= 3 ? argv[2] : "\\nt\\private\\ntos\\inc\\halmips.h";
    HalMips = fopen( outName, "w" );
    if (HalMips == NULL) {
        fprintf( stderr, "GENMIPS: Cannot open %s for writing.\n", outName);

    } else {
        fprintf(stderr, "GENMIPS: Writing %s header file.\n", outName);
    }

    //
    // Include statement for MIPS architecture static definitions.
    //

    EnableInc (KSMIPS | HALMIPS);
    dumpf("#include \"kxmips.h\"\n");
    DisableInc (HALMIPS);

    //
    // Include architecture independent definitions.
    //

#include "..\genxx.inc"

    //
    // Generate architecture dependent definitions.
    //
    // Processor block structure definitions.
    //

    EnableInc(HALMIPS);

    genCom("Processor Block Structure Offset Definitions");

    genVal(PRCB_MINOR_VERSION, PRCB_MINOR_VERSION);
    genVal(PRCB_MAJOR_VERSION, PRCB_MAJOR_VERSION);

    genSpc();

    genDef(Pb, KPRCB, MinorVersion);
    genDef(Pb, KPRCB, MajorVersion);
    genDef(Pb, KPRCB, CurrentThread);
    genDef(Pb, KPRCB, NextThread);
    genDef(Pb, KPRCB, IdleThread);
    genDef(Pb, KPRCB, Number);
    genDef(Pb, KPRCB, SetMember);
    genDef(Pb, KPRCB, RestartBlock);
    genDef(Pb, KPRCB, SystemReserved);
    genDef(Pb, KPRCB, HalReserved);

    DisableInc(HALMIPS);

    genDef(Pb, KPRCB, DpcTime);
    genDef(Pb, KPRCB, InterruptTime);
    genDef(Pb, KPRCB, KernelTime);
    genDef(Pb, KPRCB, UserTime);
    genDef(Pb, KPRCB, AdjustDpcThreshold);
    genDef(Pb, KPRCB, InterruptCount);
    genDef(Pb, KPRCB, ApcBypassCount);
    genDef(Pb, KPRCB, DpcBypassCount);
    genDef(Pb, KPRCB, IpiFrozen);
    genDef(Pb, KPRCB, ProcessorState);
    genAlt(PbAlignmentFixupCount, KPRCB, KeAlignmentFixupCount);
    genAlt(PbContextSwitches, KPRCB, KeContextSwitches);
    genAlt(PbDcacheFlushCount, KPRCB, KeDcacheFlushCount);
    genAlt(PbExceptionDispatchCount, KPRCB, KeExceptionDispatchCount);
    genAlt(PbFirstLevelTbFills, KPRCB, KeFirstLevelTbFills);
    genAlt(PbFloatingEmulationCount, KPRCB, KeFloatingEmulationCount);
    genAlt(PbIcacheFlushCount, KPRCB, KeIcacheFlushCount);
    genAlt(PbSecondLevelTbFills, KPRCB, KeSecondLevelTbFills);
    genAlt(PbSystemCalls, KPRCB, KeSystemCalls);
    genDef(Pb, KPRCB, CurrentPacket);
    genDef(Pb, KPRCB, TargetSet);
    genDef(Pb, KPRCB, WorkerRoutine);
    genDef(Pb, KPRCB, RequestSummary);
    genDef(Pb, KPRCB, SignalDone);
    genDef(Pb, KPRCB, DpcInterruptRequested);
    genDef(Pb, KPRCB, MaximumDpcQueueDepth);
    genDef(Pb, KPRCB, MinimumDpcRate);
    genDef(Pb, KPRCB, IpiCounts);
    genDef(Pb, KPRCB, StartCount);
    genDef(Pb, KPRCB, DpcLock);
    genDef(Pb, KPRCB, DpcListHead);
    genDef(Pb, KPRCB, DpcQueueDepth);
    genDef(Pb, KPRCB, DpcCount);
    genDef(Pb, KPRCB, DpcLastCount);
    genDef(Pb, KPRCB, DpcRequestRate);
    genDef(Pb, KPRCB, DpcRoutineActive);
    genVal(ProcessorBlockLength, ((sizeof(KPRCB) + 15) & ~15));

    //
    // Processor control register structure definitions.
    //

#if defined(_MIPS_)

    EnableInc(HALMIPS);

    genCom("Processor Control Registers Structure Offset Definitions");

    genVal(PCR_MINOR_VERSION, PCR_MINOR_VERSION);
    genVal(PCR_MAJOR_VERSION, PCR_MAJOR_VERSION);

    genSpc();

    genDef(Pc, KPCR, MinorVersion);
    genDef(Pc, KPCR, MajorVersion);
    genDef(Pc, KPCR, InterruptRoutine);
    genDef(Pc, KPCR, XcodeDispatch);
    genDef(Pc, KPCR, FirstLevelDcacheSize);
    genDef(Pc, KPCR, FirstLevelDcacheFillSize);
    genDef(Pc, KPCR, FirstLevelIcacheSize);
    genDef(Pc, KPCR, FirstLevelIcacheFillSize);
    genDef(Pc, KPCR, SecondLevelDcacheSize);
    genDef(Pc, KPCR, SecondLevelDcacheFillSize);
    genDef(Pc, KPCR, SecondLevelIcacheSize);
    genDef(Pc, KPCR, SecondLevelIcacheFillSize);
    genDef(Pc, KPCR, Prcb);
    genDef(Pc, KPCR, Teb);
    genDef(Pc, KPCR, TlsArray);
    genDef(Pc, KPCR, DcacheFillSize);
    genDef(Pc, KPCR, IcacheAlignment);
    genDef(Pc, KPCR, IcacheFillSize);
    genDef(Pc, KPCR, ProcessorId);
    genDef(Pc, KPCR, ProfileInterval);
    genDef(Pc, KPCR, ProfileCount);
    genDef(Pc, KPCR, StallExecutionCount);
    genDef(Pc, KPCR, StallScaleFactor);
    genDef(Pc, KPCR, Number);
    genDef(Pc, KPCR, DataBusError);
    genDef(Pc, KPCR, InstructionBusError);
    genDef(Pc, KPCR, CachePolicy);
    genDef(Pc, KPCR, IrqlMask);
    genDef(Pc, KPCR, IrqlTable);
    genDef(Pc, KPCR, CurrentIrql);
    genDef(Pc, KPCR, SetMember);
    genDef(Pc, KPCR, CurrentThread);
    genDef(Pc, KPCR, AlignedCachePolicy);
    genDef(Pc, KPCR, NotMember);
    genDef(Pc, KPCR, SystemReserved);
    genDef(Pc, KPCR, DcacheAlignment);
    genDef(Pc, KPCR, HalReserved);

    DisableInc(HALMIPS);

    genDef(Pc, KPCR, FirstLevelActive);
    genDef(Pc, KPCR, DpcRoutineActive);
    genDef(Pc, KPCR, CurrentPid);
    genDef(Pc, KPCR, OnInterruptStack);
    genDef(Pc, KPCR, SavedInitialStack);
    genDef(Pc, KPCR, SavedStackLimit);
    genDef(Pc, KPCR, SystemServiceDispatchStart);
    genDef(Pc, KPCR, SystemServiceDispatchEnd);
    genDef(Pc, KPCR, InterruptStack);
    genDef(Pc, KPCR, PanicStack);
    genDef(Pc, KPCR, InitialStack);
    genDef(Pc, KPCR, StackLimit);
    genDef(Pc, KPCR, SavedEpc);
    genDef(Pc, KPCR, SavedT7);
    genDef(Pc, KPCR, SavedT8);
    genDef(Pc, KPCR, SavedT9);
    genDef(Pc, KPCR, SystemGp);
    genDef(Pc, KPCR, QuantumEnd);
    genDef(Pc, KPCR, BadVaddr);
    genDef(Pc, KPCR, TmpVaddr);
    genVal(ProcessorControlRegisterLength, ((sizeof(KPCR) + 15) & ~15));

    genSpc();

    genDef(Pc2, KUSER_SHARED_DATA, TickCountLow);
    genDef(Pc2, KUSER_SHARED_DATA, TickCountMultiplier);
    genDef(Pc2, KUSER_SHARED_DATA, InterruptTime);
    genDef(Pc2, KUSER_SHARED_DATA, SystemTime);

#endif

    //
    // TB entry structure offset definitions.
    //

#if defined(_MIPS_)

    genCom("TB Entry Structure Offset Definitions");

    genDef(Tb, TB_ENTRY, Entrylo0);
    genDef(Tb, TB_ENTRY, Entrylo1);
    genDef(Tb, TB_ENTRY, Entryhi);
    genDef(Tb, TB_ENTRY, Pagemask);

#endif

    //
    //
    // Interprocessor command definitions.
    //

    genCom("Immediate Interprocessor Command Definitions");

    genVal(IPI_APC, IPI_APC);
    genVal(IPI_DPC, IPI_DPC);
    genVal(IPI_FREEZE, IPI_FREEZE);
    genVal(IPI_PACKET_READY, IPI_PACKET_READY);

    //
    // Interprocessor interrupt count structure offset definitions.
    //

    genCom("Interprocessor Interrupt Count Structure Offset Definitions");

    genDef(Ic, KIPI_COUNTS, Freeze);
    genDef(Ic, KIPI_COUNTS, Packet);
    genDef(Ic, KIPI_COUNTS, DPC);
    genDef(Ic, KIPI_COUNTS, APC);
    genDef(Ic, KIPI_COUNTS, FlushSingleTb);
    genDef(Ic, KIPI_COUNTS, FlushMultipleTb);
    genDef(Ic, KIPI_COUNTS, FlushEntireTb);
    genDef(Ic, KIPI_COUNTS, GenericCall);
    genDef(Ic, KIPI_COUNTS, ChangeColor);
    genDef(Ic, KIPI_COUNTS, SweepDcache);
    genDef(Ic, KIPI_COUNTS, SweepIcache);
    genDef(Ic, KIPI_COUNTS, SweepIcacheRange);
    genDef(Ic, KIPI_COUNTS, FlushIoBuffers);
    genDef(Ic, KIPI_COUNTS, GratuitousDPC);

    //
    // Context frame offset definitions and flag definitions.
    //

    EnableInc (HALMIPS);

    genCom("Context Frame Offset and Flag Definitions");

    genVal(CONTEXT_FULL, CONTEXT_FULL);
    genVal(CONTEXT_CONTROL, CONTEXT_CONTROL);
    genVal(CONTEXT_FLOATING_POINT, CONTEXT_FLOATING_POINT);
    genVal(CONTEXT_INTEGER, CONTEXT_INTEGER);
    genVal(CONTEXT_EXTENDED_FLOAT, CONTEXT_EXTENDED_FLOAT);
    genVal(CONTEXT_EXTENDED_INTEGER, CONTEXT_EXTENDED_INTEGER);

    genCom("32-bit Context Frame Offset Definitions");

    genDef(Cx, CONTEXT, FltF0);
    genDef(Cx, CONTEXT, FltF1);
    genDef(Cx, CONTEXT, FltF2);
    genDef(Cx, CONTEXT, FltF3);
    genDef(Cx, CONTEXT, FltF4);
    genDef(Cx, CONTEXT, FltF5);
    genDef(Cx, CONTEXT, FltF6);
    genDef(Cx, CONTEXT, FltF7);
    genDef(Cx, CONTEXT, FltF8);
    genDef(Cx, CONTEXT, FltF9);
    genDef(Cx, CONTEXT, FltF10);
    genDef(Cx, CONTEXT, FltF11);
    genDef(Cx, CONTEXT, FltF12);
    genDef(Cx, CONTEXT, FltF13);
    genDef(Cx, CONTEXT, FltF14);
    genDef(Cx, CONTEXT, FltF15);
    genDef(Cx, CONTEXT, FltF16);
    genDef(Cx, CONTEXT, FltF17);
    genDef(Cx, CONTEXT, FltF18);
    genDef(Cx, CONTEXT, FltF19);
    genDef(Cx, CONTEXT, FltF20);
    genDef(Cx, CONTEXT, FltF21);
    genDef(Cx, CONTEXT, FltF22);
    genDef(Cx, CONTEXT, FltF23);
    genDef(Cx, CONTEXT, FltF24);
    genDef(Cx, CONTEXT, FltF25);
    genDef(Cx, CONTEXT, FltF26);
    genDef(Cx, CONTEXT, FltF27);
    genDef(Cx, CONTEXT, FltF28);
    genDef(Cx, CONTEXT, FltF29);
    genDef(Cx, CONTEXT, FltF30);
    genDef(Cx, CONTEXT, FltF31);
    genDef(Cx, CONTEXT, IntZero);
    genDef(Cx, CONTEXT, IntAt);
    genDef(Cx, CONTEXT, IntV0);
    genDef(Cx, CONTEXT, IntV1);
    genDef(Cx, CONTEXT, IntA0);
    genDef(Cx, CONTEXT, IntA1);
    genDef(Cx, CONTEXT, IntA2);
    genDef(Cx, CONTEXT, IntA3);
    genDef(Cx, CONTEXT, IntT0);
    genDef(Cx, CONTEXT, IntT1);
    genDef(Cx, CONTEXT, IntT2);
    genDef(Cx, CONTEXT, IntT3);
    genDef(Cx, CONTEXT, IntT4);
    genDef(Cx, CONTEXT, IntT5);
    genDef(Cx, CONTEXT, IntT6);
    genDef(Cx, CONTEXT, IntT7);
    genDef(Cx, CONTEXT, IntS0);
    genDef(Cx, CONTEXT, IntS1);
    genDef(Cx, CONTEXT, IntS2);
    genDef(Cx, CONTEXT, IntS3);
    genDef(Cx, CONTEXT, IntS4);
    genDef(Cx, CONTEXT, IntS5);
    genDef(Cx, CONTEXT, IntS6);
    genDef(Cx, CONTEXT, IntS7);
    genDef(Cx, CONTEXT, IntT8);
    genDef(Cx, CONTEXT, IntT9);
    genDef(Cx, CONTEXT, IntK0);
    genDef(Cx, CONTEXT, IntK1);
    genDef(Cx, CONTEXT, IntGp);
    genDef(Cx, CONTEXT, IntSp);
    genDef(Cx, CONTEXT, IntS8);
    genDef(Cx, CONTEXT, IntRa);
    genDef(Cx, CONTEXT, IntLo);
    genDef(Cx, CONTEXT, IntHi);
    genDef(Cx, CONTEXT, Fsr);
    genDef(Cx, CONTEXT, Fir);
    genDef(Cx, CONTEXT, Psr);
    genDef(Cx, CONTEXT, ContextFlags);

    genCom("64-bit Context Frame Offset Definitions");

    genDef(Cx, CONTEXT, XFltF0);
    genDef(Cx, CONTEXT, XFltF1);
    genDef(Cx, CONTEXT, XFltF2);
    genDef(Cx, CONTEXT, XFltF3);
    genDef(Cx, CONTEXT, XFltF4);
    genDef(Cx, CONTEXT, XFltF5);
    genDef(Cx, CONTEXT, XFltF6);
    genDef(Cx, CONTEXT, XFltF7);
    genDef(Cx, CONTEXT, XFltF8);
    genDef(Cx, CONTEXT, XFltF9);
    genDef(Cx, CONTEXT, XFltF10);
    genDef(Cx, CONTEXT, XFltF11);
    genDef(Cx, CONTEXT, XFltF12);
    genDef(Cx, CONTEXT, XFltF13);
    genDef(Cx, CONTEXT, XFltF14);
    genDef(Cx, CONTEXT, XFltF15);
    genDef(Cx, CONTEXT, XFltF16);
    genDef(Cx, CONTEXT, XFltF17);
    genDef(Cx, CONTEXT, XFltF18);
    genDef(Cx, CONTEXT, XFltF19);
    genDef(Cx, CONTEXT, XFltF20);
    genDef(Cx, CONTEXT, XFltF21);
    genDef(Cx, CONTEXT, XFltF22);
    genDef(Cx, CONTEXT, XFltF23);
    genDef(Cx, CONTEXT, XFltF24);
    genDef(Cx, CONTEXT, XFltF25);
    genDef(Cx, CONTEXT, XFltF26);
    genDef(Cx, CONTEXT, XFltF27);
    genDef(Cx, CONTEXT, XFltF28);
    genDef(Cx, CONTEXT, XFltF29);
    genDef(Cx, CONTEXT, XFltF30);
    genDef(Cx, CONTEXT, XFltF31);
    genDef(Cx, CONTEXT, XFsr);
    genDef(Cx, CONTEXT, XFir);
    genDef(Cx, CONTEXT, XPsr);
    genDef(Cx, CONTEXT, XContextFlags);
    genDef(Cx, CONTEXT, XIntZero);
    genDef(Cx, CONTEXT, XIntAt);
    genDef(Cx, CONTEXT, XIntV0);
    genDef(Cx, CONTEXT, XIntV1);
    genDef(Cx, CONTEXT, XIntA0);
    genDef(Cx, CONTEXT, XIntA1);
    genDef(Cx, CONTEXT, XIntA2);
    genDef(Cx, CONTEXT, XIntA3);
    genDef(Cx, CONTEXT, XIntT0);
    genDef(Cx, CONTEXT, XIntT1);
    genDef(Cx, CONTEXT, XIntT2);
    genDef(Cx, CONTEXT, XIntT3);
    genDef(Cx, CONTEXT, XIntT4);
    genDef(Cx, CONTEXT, XIntT5);
    genDef(Cx, CONTEXT, XIntT6);
    genDef(Cx, CONTEXT, XIntT7);
    genDef(Cx, CONTEXT, XIntS0);
    genDef(Cx, CONTEXT, XIntS1);
    genDef(Cx, CONTEXT, XIntS2);
    genDef(Cx, CONTEXT, XIntS3);
    genDef(Cx, CONTEXT, XIntS4);
    genDef(Cx, CONTEXT, XIntS5);
    genDef(Cx, CONTEXT, XIntS6);
    genDef(Cx, CONTEXT, XIntS7);
    genDef(Cx, CONTEXT, XIntT8);
    genDef(Cx, CONTEXT, XIntT9);
    genDef(Cx, CONTEXT, XIntK0);
    genDef(Cx, CONTEXT, XIntK1);
    genDef(Cx, CONTEXT, XIntGp);
    genDef(Cx, CONTEXT, XIntSp);
    genDef(Cx, CONTEXT, XIntS8);
    genDef(Cx, CONTEXT, XIntRa);
    genDef(Cx, CONTEXT, XIntLo);
    genDef(Cx, CONTEXT, XIntHi);
    genVal(ContextFrameLength, sizeof(CONTEXT));

    //
    // Exception frame offset definitions.
    //

    genCom("Exception Frame Offset Definitions and Length");

    genAlt(ExArgs, KEXCEPTION_FRAME, Argument);

    genCom("32-bit Nonvolatile Floating State");

    genDef(Ex, KEXCEPTION_FRAME, FltF20);
    genDef(Ex, KEXCEPTION_FRAME, FltF21);
    genDef(Ex, KEXCEPTION_FRAME, FltF22);
    genDef(Ex, KEXCEPTION_FRAME, FltF23);
    genDef(Ex, KEXCEPTION_FRAME, FltF24);
    genDef(Ex, KEXCEPTION_FRAME, FltF25);
    genDef(Ex, KEXCEPTION_FRAME, FltF26);
    genDef(Ex, KEXCEPTION_FRAME, FltF27);
    genDef(Ex, KEXCEPTION_FRAME, FltF28);
    genDef(Ex, KEXCEPTION_FRAME, FltF29);
    genDef(Ex, KEXCEPTION_FRAME, FltF30);
    genDef(Ex, KEXCEPTION_FRAME, FltF31);

    genCom("64-bit Nonvolatile Floating State");

    genDef(Ex, KEXCEPTION_FRAME, XFltF20);
    genDef(Ex, KEXCEPTION_FRAME, XFltF22);
    genDef(Ex, KEXCEPTION_FRAME, XFltF24);
    genDef(Ex, KEXCEPTION_FRAME, XFltF26);
    genDef(Ex, KEXCEPTION_FRAME, XFltF28);
    genDef(Ex, KEXCEPTION_FRAME, XFltF30);

    genCom("32-bit Nonvolatile Integer State");

    genDef(Ex, KEXCEPTION_FRAME, IntS0);
    genDef(Ex, KEXCEPTION_FRAME, IntS1);
    genDef(Ex, KEXCEPTION_FRAME, IntS2);
    genDef(Ex, KEXCEPTION_FRAME, IntS3);
    genDef(Ex, KEXCEPTION_FRAME, IntS4);
    genDef(Ex, KEXCEPTION_FRAME, IntS5);
    genDef(Ex, KEXCEPTION_FRAME, IntS6);
    genDef(Ex, KEXCEPTION_FRAME, IntS7);
    genDef(Ex, KEXCEPTION_FRAME, IntS8);
    genDef(Ex, KEXCEPTION_FRAME, SwapReturn);
    genDef(Ex, KEXCEPTION_FRAME, IntRa);
    genVal(ExceptionFrameLength, sizeof(KEXCEPTION_FRAME));

    //
    // Jump buffer offset definitions.
    //

  DisableInc (HALMIPS);

    genCom("Jump Offset Definitions and Length");

    genDef(Jb, _JUMP_BUFFER, FltF20);
    genDef(Jb, _JUMP_BUFFER, FltF21);
    genDef(Jb, _JUMP_BUFFER, FltF22);
    genDef(Jb, _JUMP_BUFFER, FltF23);
    genDef(Jb, _JUMP_BUFFER, FltF24);
    genDef(Jb, _JUMP_BUFFER, FltF25);
    genDef(Jb, _JUMP_BUFFER, FltF26);
    genDef(Jb, _JUMP_BUFFER, FltF27);
    genDef(Jb, _JUMP_BUFFER, FltF28);
    genDef(Jb, _JUMP_BUFFER, FltF29);
    genDef(Jb, _JUMP_BUFFER, FltF30);
    genDef(Jb, _JUMP_BUFFER, FltF31);
    genDef(Jb, _JUMP_BUFFER, IntS0);
    genDef(Jb, _JUMP_BUFFER, IntS1);
    genDef(Jb, _JUMP_BUFFER, IntS2);
    genDef(Jb, _JUMP_BUFFER, IntS3);
    genDef(Jb, _JUMP_BUFFER, IntS4);
    genDef(Jb, _JUMP_BUFFER, IntS5);
    genDef(Jb, _JUMP_BUFFER, IntS6);
    genDef(Jb, _JUMP_BUFFER, IntS7);
    genDef(Jb, _JUMP_BUFFER, IntS8);
    genDef(Jb, _JUMP_BUFFER, IntSp);
    genDef(Jb, _JUMP_BUFFER, Type);
    genDef(Jb, _JUMP_BUFFER, Fir);

    //
    // Trap frame offset definitions.
    //

  EnableInc (HALMIPS);

    genCom("Trap Frame Offset Definitions and Length");

    genAlt(TrArgs, KTRAP_FRAME, Argument);

    genCom("32-bit Volatile Floating State");

    genDef(Tr, KTRAP_FRAME, FltF0);
    genDef(Tr, KTRAP_FRAME, FltF1);
    genDef(Tr, KTRAP_FRAME, FltF2);
    genDef(Tr, KTRAP_FRAME, FltF3);
    genDef(Tr, KTRAP_FRAME, FltF4);
    genDef(Tr, KTRAP_FRAME, FltF5);
    genDef(Tr, KTRAP_FRAME, FltF6);
    genDef(Tr, KTRAP_FRAME, FltF7);
    genDef(Tr, KTRAP_FRAME, FltF8);
    genDef(Tr, KTRAP_FRAME, FltF9);
    genDef(Tr, KTRAP_FRAME, FltF10);
    genDef(Tr, KTRAP_FRAME, FltF11);
    genDef(Tr, KTRAP_FRAME, FltF12);
    genDef(Tr, KTRAP_FRAME, FltF13);
    genDef(Tr, KTRAP_FRAME, FltF14);
    genDef(Tr, KTRAP_FRAME, FltF15);
    genDef(Tr, KTRAP_FRAME, FltF16);
    genDef(Tr, KTRAP_FRAME, FltF17);
    genDef(Tr, KTRAP_FRAME, FltF18);
    genDef(Tr, KTRAP_FRAME, FltF19);

    genCom("64-bit Volatile Floating State");

    genDef(Tr, KTRAP_FRAME, XFltF0);
    genDef(Tr, KTRAP_FRAME, XFltF1);
    genDef(Tr, KTRAP_FRAME, XFltF2);
    genDef(Tr, KTRAP_FRAME, XFltF3);
    genDef(Tr, KTRAP_FRAME, XFltF4);
    genDef(Tr, KTRAP_FRAME, XFltF5);
    genDef(Tr, KTRAP_FRAME, XFltF6);
    genDef(Tr, KTRAP_FRAME, XFltF7);
    genDef(Tr, KTRAP_FRAME, XFltF8);
    genDef(Tr, KTRAP_FRAME, XFltF9);
    genDef(Tr, KTRAP_FRAME, XFltF10);
    genDef(Tr, KTRAP_FRAME, XFltF11);
    genDef(Tr, KTRAP_FRAME, XFltF12);
    genDef(Tr, KTRAP_FRAME, XFltF13);
    genDef(Tr, KTRAP_FRAME, XFltF14);
    genDef(Tr, KTRAP_FRAME, XFltF15);
    genDef(Tr, KTRAP_FRAME, XFltF16);
    genDef(Tr, KTRAP_FRAME, XFltF17);
    genDef(Tr, KTRAP_FRAME, XFltF18);
    genDef(Tr, KTRAP_FRAME, XFltF19);
    genDef(Tr, KTRAP_FRAME, XFltF21);
    genDef(Tr, KTRAP_FRAME, XFltF23);
    genDef(Tr, KTRAP_FRAME, XFltF25);
    genDef(Tr, KTRAP_FRAME, XFltF27);
    genDef(Tr, KTRAP_FRAME, XFltF29);
    genDef(Tr, KTRAP_FRAME, XFltF31);

    genCom("64-bit Volatile Integer State");

    genDef(Tr, KTRAP_FRAME, XIntZero);
    genDef(Tr, KTRAP_FRAME, XIntAt);
    genDef(Tr, KTRAP_FRAME, XIntV0);
    genDef(Tr, KTRAP_FRAME, XIntV1);
    genDef(Tr, KTRAP_FRAME, XIntA0);
    genDef(Tr, KTRAP_FRAME, XIntA1);
    genDef(Tr, KTRAP_FRAME, XIntA2);
    genDef(Tr, KTRAP_FRAME, XIntA3);
    genDef(Tr, KTRAP_FRAME, XIntT0);
    genDef(Tr, KTRAP_FRAME, XIntT1);
    genDef(Tr, KTRAP_FRAME, XIntT2);
    genDef(Tr, KTRAP_FRAME, XIntT3);
    genDef(Tr, KTRAP_FRAME, XIntT4);
    genDef(Tr, KTRAP_FRAME, XIntT5);
    genDef(Tr, KTRAP_FRAME, XIntT6);
    genDef(Tr, KTRAP_FRAME, XIntT7);
    genDef(Tr, KTRAP_FRAME, XIntS0);
    genDef(Tr, KTRAP_FRAME, XIntS1);
    genDef(Tr, KTRAP_FRAME, XIntS2);
    genDef(Tr, KTRAP_FRAME, XIntS3);
    genDef(Tr, KTRAP_FRAME, XIntS4);
    genDef(Tr, KTRAP_FRAME, XIntS5);
    genDef(Tr, KTRAP_FRAME, XIntS6);
    genDef(Tr, KTRAP_FRAME, XIntS7);
    genDef(Tr, KTRAP_FRAME, XIntT8);
    genDef(Tr, KTRAP_FRAME, XIntT9);
    genDef(Tr, KTRAP_FRAME, XIntGp);
    genDef(Tr, KTRAP_FRAME, XIntSp);
    genDef(Tr, KTRAP_FRAME, XIntS8);
    genDef(Tr, KTRAP_FRAME, XIntRa);
    genDef(Tr, KTRAP_FRAME, XIntLo);
    genDef(Tr, KTRAP_FRAME, XIntHi);

    genSpc();

    genDef(Tr, KTRAP_FRAME, Fir);
    genDef(Tr, KTRAP_FRAME, Fsr);
    genDef(Tr, KTRAP_FRAME, Psr);
    genDef(Tr, KTRAP_FRAME, ExceptionRecord);
    genDef(Tr, KTRAP_FRAME, OldIrql);
    genDef(Tr, KTRAP_FRAME, PreviousMode);
    genDef(Tr, KTRAP_FRAME, SavedFlag);
    genAlt(TrOnInterruptStack, KTRAP_FRAME, u.OnInterruptStack);
    genAlt(TrTrapFrame, KTRAP_FRAME, u.TrapFrame);

    genVal(TrapFrameLength, sizeof(KTRAP_FRAME));
    genVal(TrapFrameArguments, KTRAP_FRAME_ARGUMENTS);

    //
    // Usermode callout kernel frame definitions
    //

    DisableInc(HALMIPS);

    genCom("Usermode callout kernel frame definitions");

    genDef(Cu, KCALLOUT_FRAME, F20);
    genDef(Cu, KCALLOUT_FRAME, F21);
    genDef(Cu, KCALLOUT_FRAME, F22);
    genDef(Cu, KCALLOUT_FRAME, F23);
    genDef(Cu, KCALLOUT_FRAME, F24);
    genDef(Cu, KCALLOUT_FRAME, F25);
    genDef(Cu, KCALLOUT_FRAME, F26);
    genDef(Cu, KCALLOUT_FRAME, F27);
    genDef(Cu, KCALLOUT_FRAME, F28);
    genDef(Cu, KCALLOUT_FRAME, F29);
    genDef(Cu, KCALLOUT_FRAME, F30);
    genDef(Cu, KCALLOUT_FRAME, F31);
    genDef(Cu, KCALLOUT_FRAME, S0);
    genDef(Cu, KCALLOUT_FRAME, S1);
    genDef(Cu, KCALLOUT_FRAME, S2);
    genDef(Cu, KCALLOUT_FRAME, S3);
    genDef(Cu, KCALLOUT_FRAME, S4);
    genDef(Cu, KCALLOUT_FRAME, S5);
    genDef(Cu, KCALLOUT_FRAME, S6);
    genDef(Cu, KCALLOUT_FRAME, S7);
    genDef(Cu, KCALLOUT_FRAME, S8);
    genDef(Cu, KCALLOUT_FRAME, CbStk);
    genDef(Cu, KCALLOUT_FRAME, TrFr);
    genDef(Cu, KCALLOUT_FRAME, Fsr);
    genDef(Cu, KCALLOUT_FRAME, InStk);
    genDef(Cu, KCALLOUT_FRAME, Ra);
    genVal(CuFrameLength, OFFSET(KCALLOUT_FRAME, A0));
    genDef(Cu, KCALLOUT_FRAME, A0);
    genDef(Cu, KCALLOUT_FRAME, A1);

    //
    // Usermode callout user frame definitions.
    //

    genCom("Usermode callout user frame definitions");

    genDef(Ck, UCALLOUT_FRAME, Buffer);
    genDef(Ck, UCALLOUT_FRAME, Length);
    genDef(Ck, UCALLOUT_FRAME, ApiNumber);
    genDef(Ck, UCALLOUT_FRAME, Sp);
    genDef(Ck, UCALLOUT_FRAME, Ra);

    EnableInc(HALMIPS);

    //
    // Loader Parameter Block offset definitions.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Loader Parameter Block Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define LpbLoadOrderListHead 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, LoadOrderListHead));

    dumpf("#define LpbMemoryDescriptorListHead 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, MemoryDescriptorListHead));

    dumpf("#define LpbKernelStack 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, KernelStack));

    dumpf("#define LpbPrcb 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, Prcb));

    dumpf("#define LpbProcess 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, Process));

    dumpf("#define LpbThread 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, Thread));

    dumpf("#define LpbInterruptStack 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.InterruptStack));

    dumpf("#define LpbFirstLevelDcacheSize 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.FirstLevelDcacheSize));

    dumpf("#define LpbFirstLevelDcacheFillSize 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.FirstLevelDcacheFillSize));

    dumpf("#define LpbFirstLevelIcacheSize 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.FirstLevelIcacheSize));

    dumpf("#define LpbFirstLevelIcacheFillSize 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.FirstLevelIcacheFillSize));

    dumpf("#define LpbGpBase 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.GpBase));

    dumpf("#define LpbPanicStack 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.PanicStack));

    dumpf("#define LpbPcrPage 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.PcrPage));

    dumpf("#define LpbPdrPage 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.PdrPage));

    dumpf("#define LpbSecondLevelDcacheSize 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.SecondLevelDcacheSize));

    dumpf("#define LpbSecondLevelDcacheFillSize 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.SecondLevelDcacheFillSize));

    dumpf("#define LpbSecondLevelIcacheSize 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.SecondLevelIcacheSize));

    dumpf("#define LpbSecondLevelIcacheFillSize 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.SecondLevelIcacheFillSize));

    dumpf("#define LpbPcrPage2 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.PcrPage2));

    dumpf("#define LpbRegistryLength 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, RegistryLength));

    dumpf("#define LpbRegistryBase 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, RegistryBase));

    DisableInc (HALMIPS);

    //
    // Define Client/Server data structure definitions.
    //

    genCom("Client/Server Structure Definitions");

    genDef(Cid, CLIENT_ID, UniqueProcess);
    genDef(Cid, CLIENT_ID, UniqueThread);

    //
    // Address space layout definitions
    //

    EnableInc(HALMIPS);

    genCom("Address Space Layout Definitions");

    genVal(KUSEG_BASE, KUSEG_BASE);
    genVal(KSEG0_BASE, KSEG0_BASE);
    genVal(KSEG1_BASE, KSEG1_BASE);
    genVal(KSEG2_BASE, KSEG2_BASE);

    DisableInc(HALMIPS);

    genVal(CACHE_ERROR_VECTOR, CACHE_ERROR_VECTOR);
    genVal(SYSTEM_BASE, SYSTEM_BASE);
    genVal(PDE_BASE, PDE_BASE);
    genVal(PTE_BASE, PTE_BASE);
    genVal(PDE64_BASE, PDE64_BASE);
    genVal(PTE64_BASE, PTE64_BASE);

    //
    // Page table and page directory entry definitions
    //

    EnableInc(HALMIPS);

    genCom("Page Table and Directory Entry Definitions");

    genVal(PAGE_SIZE, PAGE_SIZE);
    genVal(PAGE_SHIFT, PAGE_SHIFT);
    genVal(PDI_SHIFT, PDI_SHIFT);
    genVal(PTI_SHIFT, PTI_SHIFT);

    //
    // Software interrupt request mask definitions
    //

    genCom("Software Interrupt Request Mask Definitions");

    genVal(APC_INTERRUPT, (1 << (APC_LEVEL + CAUSE_INTPEND - 1)));
    genVal(DISPATCH_INTERRUPT, (1 << (DISPATCH_LEVEL + CAUSE_INTPEND - 1)));

    DisableInc(HALMIPS);

    //
    // Breakpoint instruction definitions
    //

    EnableInc(HALMIPS);

    genCom("Breakpoint Definitions");

    genVal(USER_BREAKPOINT, USER_BREAKPOINT);
    genVal(KERNEL_BREAKPOINT, KERNEL_BREAKPOINT);
    genVal(BREAKIN_BREAKPOINT, BREAKIN_BREAKPOINT);

    DisableInc(HALMIPS);

    genVal(BRANCH_TAKEN_BREAKPOINT, BRANCH_TAKEN_BREAKPOINT);
    genVal(BRANCH_NOT_TAKEN_BREAKPOINT, BRANCH_NOT_TAKEN_BREAKPOINT);
    genVal(SINGLE_STEP_BREAKPOINT, SINGLE_STEP_BREAKPOINT);
    genVal(DIVIDE_OVERFLOW_BREAKPOINT, DIVIDE_OVERFLOW_BREAKPOINT);
    genVal(DIVIDE_BY_ZERO_BREAKPOINT, DIVIDE_BY_ZERO_BREAKPOINT);
    genVal(RANGE_CHECK_BREAKPOINT, RANGE_CHECK_BREAKPOINT);
    genVal(STACK_OVERFLOW_BREAKPOINT, STACK_OVERFLOW_BREAKPOINT);
    genVal(MULTIPLY_OVERFLOW_BREAKPOINT, MULTIPLY_OVERFLOW_BREAKPOINT);
    genVal(DEBUG_PRINT_BREAKPOINT, DEBUG_PRINT_BREAKPOINT);
    genVal(DEBUG_PROMPT_BREAKPOINT, DEBUG_PROMPT_BREAKPOINT);
    genVal(DEBUG_STOP_BREAKPOINT, DEBUG_STOP_BREAKPOINT);
    genVal(DEBUG_LOAD_SYMBOLS_BREAKPOINT, DEBUG_LOAD_SYMBOLS_BREAKPOINT);
    genVal(DEBUG_UNLOAD_SYMBOLS_BREAKPOINT, DEBUG_UNLOAD_SYMBOLS_BREAKPOINT);

    //
    // Miscellaneous definitions
    //

    EnableInc(HALMIPS);

    genCom("Miscellaneous Definitions");

    genVal(Executive, Executive);
    genVal(KernelMode, KernelMode);
    genVal(FALSE, FALSE);
    genVal(TRUE, TRUE);
    genVal(UNCACHED_POLICY, UNCACHED_POLICY);
    genVal(KiPcr, KIPCR);
    genVal(KiPcr2, KIPCR2);

    DisableInc(HALMIPS);

    genVal(UsPcr, USPCR);
    genVal(UsPcr2, USPCR2);
    genVal(BASE_PRIORITY_THRESHOLD, BASE_PRIORITY_THRESHOLD);
    genVal(EVENT_PAIR_INCREMENT, EVENT_PAIR_INCREMENT);
    genVal(LOW_REALTIME_PRIORITY, LOW_REALTIME_PRIORITY);
    genVal(KERNEL_STACK_SIZE, KERNEL_STACK_SIZE);
    genVal(KERNEL_LARGE_STACK_COMMIT, KERNEL_LARGE_STACK_COMMIT);
    genVal(XCODE_VECTOR_LENGTH, XCODE_VECTOR_LENGTH);
    genVal(MM_USER_PROBE_ADDRESS, MM_USER_PROBE_ADDRESS);
    genVal(ROUND_TO_NEAREST, ROUND_TO_NEAREST);
    genVal(ROUND_TO_ZERO, ROUND_TO_ZERO);
    genVal(ROUND_TO_PLUS_INFINITY, ROUND_TO_PLUS_INFINITY);
    genVal(ROUND_TO_MINUS_INFINITY, ROUND_TO_MINUS_INFINITY);
    genVal(CLOCK_QUANTUM_DECREMENT, CLOCK_QUANTUM_DECREMENT);
    genVal(READY_SKIP_QUANTUM, READY_SKIP_QUANTUM);
    genVal(THREAD_QUANTUM, THREAD_QUANTUM);
    genVal(WAIT_QUANTUM_DECREMENT, WAIT_QUANTUM_DECREMENT);
    genVal(ROUND_TRIP_DECREMENT_COUNT, ROUND_TRIP_DECREMENT_COUNT);

    //
    // Close header file.
    //

    fprintf(stderr, "         Finished\n");
    return;
}

VOID
dumpf(
    const char *format,
    ...
    )

{

    va_list(arglist);

    va_start(arglist, format);

    if (((OutputEnabled & KSMIPS) != 0) && (KsMips != NULL)) {
        vfprintf (KsMips, format, arglist);
    }

    if (((OutputEnabled & HALMIPS) != 0) && (HalMips != NULL)) {
        vfprintf (HalMips, format, arglist);
    }

    va_end(arglist);
}
