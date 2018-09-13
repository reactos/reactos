/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    genppc.c

Abstract:

    This module implements a program which generates PPC machine dependent
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
#include "ctype.h"
#include "stdio.h"
#include "stdarg.h"
#include "stdlib.h"

#if defined(_M_IX86)                               // IBMCDB

#define _RESTORE_P386_DEF _M_IX86
#define _M_PPC 1
#define R4000 1
#undef i386
#undef _X86_
#undef _M_IX86

#endif

#include "nt.h"
#include "ntdef.h"
#include "ntkeapi.h"
#include "ntppc.h"
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
#include "ntrtl.h"
#include "nturtl.h"
#include "ntcsrmsg.h"
#include "ntcsrsrv.h"
#include "ntosdef.h"
#include "ntxcapi.h"
#include "ppc.h"
#include "arc.h"
#include "ke.h"
#include "ex.h"
#include "ps.h"
#include "bugcodes.h"
#include "ntstatus.h"
#include "exboosts.h"
#include "ppcdef.h"
#include "setjmp.h"

#if defined(RESTORE_P386_DEF)                              // IBMCDB

#undef _MSC_VER 800
#define _M_IX86 RESTORE_P386_DEF

#endif

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

FILE *KsPpc;
FILE *HalPpc;

//
// EnableInc(a) - Enables output to goto specified include file
//

#define EnableInc(a)    OutputEnabled |= a;

//
// DisableInc(a) - Disables output to goto specified include file
//

#define DisableInc(a)   OutputEnabled &= ~a;

ULONG OutputEnabled;

#define KSPPC      0x01
#define HALPPC     0x02

#define KERNEL KSPPC
#define HAL HALPPC

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
// This program generates the PPC machine dependent assembler offset
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
        outName = "\\nt\\public\\sdk\\inc\\ksppc.h";
    }
    outName = argc >= 2 ? argv[1] : "\\nt\\public\\sdk\\inc\\ksppc.h";
    KsPpc = fopen( outName, "w" );

    if (KsPpc == NULL) {
        fprintf( stderr, "GENPPC: Cannot open %s for writing.\n", outName);
        perror("GENPPC");
        exit(1);
    }

    fprintf( stderr, "GENPPC: Writing %s header file.\n", outName );

    outName = argc >= 3 ? argv[2] : "\\nt\\private\\ntos\\inc\\halppc.h";

    HalPpc = fopen( outName, "w" );

    if (HalPpc == NULL) {
        fprintf( stderr, "GENPPC: Cannot open %s for writing.\n", outName);
        perror("GENPPC");
        exit(1);
    }

    fprintf( stderr, "GENPPC: Writing %s header file.\n", outName );

    //
    // Include statement for PPC architecture static definitions.
    //

  EnableInc (KSPPC | HALPPC);

    dumpf("#include \"kxppc.h\"\n");

  DisableInc (HALPPC);

    //
    // Include architecture independent definitions.
    //

#include "..\genxx.inc"

    //
    // Generate architecture dependent definitions.
    //
    // Processor control register structure definitions.
    //

  EnableInc (HALPPC);

    genCom("Processor Control Registers Structure Offset Definitions");

    genVal(PCR_MINOR_VERSION, PCR_MINOR_VERSION);
    genVal(PCR_MAJOR_VERSION, PCR_MAJOR_VERSION);

    genSpc();

    genDef(Pc, KPCR, MinorVersion);
    genDef(Pc, KPCR, MajorVersion);
    genDef(Pc, KPCR, InterruptRoutine);
    genDef(Pc, KPCR, PcrPage2);
    genDef(Pc, KPCR, Kseg0Top);
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
    genDef(Pc, KPCR, DcacheAlignment);
    genDef(Pc, KPCR, DcacheFillSize);
    genDef(Pc, KPCR, IcacheAlignment);
    genDef(Pc, KPCR, IcacheFillSize);
    genDef(Pc, KPCR, ProcessorVersion);
    genDef(Pc, KPCR, ProcessorRevision);
    genDef(Pc, KPCR, ProfileInterval);
    genDef(Pc, KPCR, ProfileCount);
    genDef(Pc, KPCR, StallExecutionCount);
    genDef(Pc, KPCR, StallScaleFactor);
    genDef(Pc, KPCR, CachePolicy);
    genDef(Pc, KPCR, IcacheMode);
    genDef(Pc, KPCR, DcacheMode);
    genDef(Pc, KPCR, IrqlMask);
    genDef(Pc, KPCR, IrqlTable);
    genDef(Pc, KPCR, CurrentIrql);
    genDef(Pc, KPCR, Number);
    genDef(Pc, KPCR, SetMember);
    genDef(Pc, KPCR, CurrentThread);
    genDef(Pc, KPCR, AlignedCachePolicy);
    genDef(Pc, KPCR, SoftwareInterrupt);
    genDef(Pc, KPCR, ApcInterrupt);
    genDef(Pc, KPCR, DispatchInterrupt);
    genDef(Pc, KPCR, NotMember);
    genDef(Pc, KPCR, SystemReserved);
    genDef(Pc, KPCR, HalReserved);

  DisableInc (HALPPC);

    genDef(Pc, KPCR, FirstLevelActive);
    genDef(Pc, KPCR, SystemServiceDispatchStart);
    genDef(Pc, KPCR, SystemServiceDispatchEnd);
    genDef(Pc, KPCR, InterruptStack);
    genDef(Pc, KPCR, QuantumEnd);
    genDef(Pc, KPCR, InitialStack);
    genDef(Pc, KPCR, PanicStack);
    genDef(Pc, KPCR, BadVaddr);
    genDef(Pc, KPCR, StackLimit);
    genDef(Pc, KPCR, SavedStackLimit);
    genDef(Pc, KPCR, SavedV0);
    genDef(Pc, KPCR, SavedV1);
    genDef(Pc, KPCR, DebugActive);
    genDef(Pc, KPCR, GprSave);
    genDef(Pc, KPCR, SiR0);
    genDef(Pc, KPCR, SiR2);
    genDef(Pc, KPCR, SiR3);
    genDef(Pc, KPCR, SiR4);
    genDef(Pc, KPCR, SiR5);
    genDef(Pc, KPCR, PgDirRa);
    genDef(Pc, KPCR, OnInterruptStack);
    genDef(Pc, KPCR, SavedInitialStack);

    genVal(ProcessorControlRegisterLength, ((sizeof(KPCR) + 15) & ~15));

    genSpc();

    genDef(Pc2, KUSER_SHARED_DATA, TickCountLow);
    genDef(Pc2, KUSER_SHARED_DATA, TickCountMultiplier);
    genDef(Pc2, KUSER_SHARED_DATA, InterruptTime);
    genDef(Pc2, KUSER_SHARED_DATA, SystemTime);
    genDef(Pc2, KUSER_SHARED_DATA, ProcessorFeatures);
    genVal(PfPpcMovemem64BitOk, sizeof(BOOLEAN) * PF_PPC_MOVEMEM_64BIT_OK);

    //
    // Offsets to elements within the InterruptRoutine table.
    //

    genSpc();

    genVal(IrPmiVector, sizeof(unsigned) * PMI_VECTOR);
    genVal(IrMachineCheckVector, sizeof(unsigned) * MACHINE_CHECK_VECTOR);
    genVal(IrDeviceVector, sizeof(unsigned) * EXTERNAL_INTERRUPT_VECTOR);
    genVal(IrDecrementVector, sizeof(unsigned) * DECREMENT_VECTOR);

    //
    // Processor block structure definitions.
    //

  EnableInc (HALPPC);

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
    genDef(Pb, KPRCB, PcrPage);
    genDef(Pb, KPRCB, SystemReserved);
    genDef(Pb, KPRCB, HalReserved);

  DisableInc (HALPPC);

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
    // Immediate interprocessor command definitions.
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
    genDef(Ic, KIPI_COUNTS, FlushEntireTb);
    genDef(Ic, KIPI_COUNTS, ChangeColor);
    genDef(Ic, KIPI_COUNTS, SweepDcache);
    genDef(Ic, KIPI_COUNTS, SweepIcache);
    genDef(Ic, KIPI_COUNTS, SweepIcacheRange);
    genDef(Ic, KIPI_COUNTS, FlushIoBuffers);

    //
    // Context frame offset definitions and flag definitions.
    //

  EnableInc (HALPPC);

    genCom("Context Frame Offset and Flag Definitions");

    genVal(CONTEXT_FULL, CONTEXT_FULL);
    genVal(CONTEXT_CONTROL, CONTEXT_CONTROL);
    genVal(CONTEXT_FLOATING_POINT, CONTEXT_FLOATING_POINT);
    genVal(CONTEXT_INTEGER, CONTEXT_INTEGER);

    genSpc();

    genDef(Cx, CONTEXT, Fpr0);
    genDef(Cx, CONTEXT, Fpr1);
    genDef(Cx, CONTEXT, Fpr2);
    genDef(Cx, CONTEXT, Fpr3);
    genDef(Cx, CONTEXT, Fpr4);
    genDef(Cx, CONTEXT, Fpr5);
    genDef(Cx, CONTEXT, Fpr6);
    genDef(Cx, CONTEXT, Fpr7);
    genDef(Cx, CONTEXT, Fpr8);
    genDef(Cx, CONTEXT, Fpr9);
    genDef(Cx, CONTEXT, Fpr10);
    genDef(Cx, CONTEXT, Fpr11);
    genDef(Cx, CONTEXT, Fpr12);
    genDef(Cx, CONTEXT, Fpr13);
    genDef(Cx, CONTEXT, Fpr14);
    genDef(Cx, CONTEXT, Fpr15);
    genDef(Cx, CONTEXT, Fpr16);
    genDef(Cx, CONTEXT, Fpr17);
    genDef(Cx, CONTEXT, Fpr18);
    genDef(Cx, CONTEXT, Fpr19);
    genDef(Cx, CONTEXT, Fpr20);
    genDef(Cx, CONTEXT, Fpr21);
    genDef(Cx, CONTEXT, Fpr22);
    genDef(Cx, CONTEXT, Fpr23);
    genDef(Cx, CONTEXT, Fpr24);
    genDef(Cx, CONTEXT, Fpr25);
    genDef(Cx, CONTEXT, Fpr26);
    genDef(Cx, CONTEXT, Fpr27);
    genDef(Cx, CONTEXT, Fpr28);
    genDef(Cx, CONTEXT, Fpr29);
    genDef(Cx, CONTEXT, Fpr30);
    genDef(Cx, CONTEXT, Fpr31);
    genDef(Cx, CONTEXT, Fpscr);
    genDef(Cx, CONTEXT, Gpr0);
    genDef(Cx, CONTEXT, Gpr1);
    genDef(Cx, CONTEXT, Gpr2);
    genDef(Cx, CONTEXT, Gpr3);
    genDef(Cx, CONTEXT, Gpr4);
    genDef(Cx, CONTEXT, Gpr5);
    genDef(Cx, CONTEXT, Gpr6);
    genDef(Cx, CONTEXT, Gpr7);
    genDef(Cx, CONTEXT, Gpr8);
    genDef(Cx, CONTEXT, Gpr9);
    genDef(Cx, CONTEXT, Gpr10);
    genDef(Cx, CONTEXT, Gpr11);
    genDef(Cx, CONTEXT, Gpr12);
    genDef(Cx, CONTEXT, Gpr13);
    genDef(Cx, CONTEXT, Gpr14);
    genDef(Cx, CONTEXT, Gpr15);
    genDef(Cx, CONTEXT, Gpr16);
    genDef(Cx, CONTEXT, Gpr17);
    genDef(Cx, CONTEXT, Gpr18);
    genDef(Cx, CONTEXT, Gpr19);
    genDef(Cx, CONTEXT, Gpr20);
    genDef(Cx, CONTEXT, Gpr21);
    genDef(Cx, CONTEXT, Gpr22);
    genDef(Cx, CONTEXT, Gpr23);
    genDef(Cx, CONTEXT, Gpr24);
    genDef(Cx, CONTEXT, Gpr25);
    genDef(Cx, CONTEXT, Gpr26);
    genDef(Cx, CONTEXT, Gpr27);
    genDef(Cx, CONTEXT, Gpr28);
    genDef(Cx, CONTEXT, Gpr29);
    genDef(Cx, CONTEXT, Gpr30);
    genDef(Cx, CONTEXT, Gpr31);
    genDef(Cx, CONTEXT, Cr);
    genDef(Cx, CONTEXT, Xer);
    genDef(Cx, CONTEXT, Msr);
    genDef(Cx, CONTEXT, Iar);
    genDef(Cx, CONTEXT, Lr);
    genDef(Cx, CONTEXT, Ctr);
    genDef(Cx, CONTEXT, ContextFlags);
    genDef(Cx, CONTEXT, Dr0);
    genDef(Cx, CONTEXT, Dr1);
    genDef(Cx, CONTEXT, Dr2);
    genDef(Cx, CONTEXT, Dr3);
    genDef(Cx, CONTEXT, Dr4);
    genDef(Cx, CONTEXT, Dr5);
    genDef(Cx, CONTEXT, Dr6);
    genDef(Cx, CONTEXT, Dr7);

    genVal(ContextFrameLength, (sizeof(CONTEXT) + 15) & (~15));

    //
    // Call/return stack frame header offset definitions.
    //

    genCom("Call/Return Stack Frame Header Offset Definitions and Length");

    genDef(Cr, STACK_FRAME_HEADER, BackChain);
    genDef(Cr, STACK_FRAME_HEADER, GlueSaved1);
    genDef(Cr, STACK_FRAME_HEADER, GlueSaved2);
    genDef(Cr, STACK_FRAME_HEADER, Reserved1);
    genDef(Cr, STACK_FRAME_HEADER, Spare1);
    genDef(Cr, STACK_FRAME_HEADER, Spare2);

    genDef(Cr, STACK_FRAME_HEADER, Parameter0);
    genDef(Cr, STACK_FRAME_HEADER, Parameter1);
    genDef(Cr, STACK_FRAME_HEADER, Parameter2);
    genDef(Cr, STACK_FRAME_HEADER, Parameter3);
    genDef(Cr, STACK_FRAME_HEADER, Parameter4);
    genDef(Cr, STACK_FRAME_HEADER, Parameter5);
    genDef(Cr, STACK_FRAME_HEADER, Parameter6);
    genDef(Cr, STACK_FRAME_HEADER, Parameter7);

    genVal(StackFrameHeaderLength, (sizeof(STACK_FRAME_HEADER) + 7) & (~7));

    //
    // Exception frame offset definitions.
    //

    genCom("Exception Frame Offset Definitions and Length");

    genDef(Ex, KEXCEPTION_FRAME, Gpr13);
    genDef(Ex, KEXCEPTION_FRAME, Gpr14);
    genDef(Ex, KEXCEPTION_FRAME, Gpr15);
    genDef(Ex, KEXCEPTION_FRAME, Gpr16);
    genDef(Ex, KEXCEPTION_FRAME, Gpr17);
    genDef(Ex, KEXCEPTION_FRAME, Gpr18);
    genDef(Ex, KEXCEPTION_FRAME, Gpr19);
    genDef(Ex, KEXCEPTION_FRAME, Gpr20);
    genDef(Ex, KEXCEPTION_FRAME, Gpr21);
    genDef(Ex, KEXCEPTION_FRAME, Gpr22);
    genDef(Ex, KEXCEPTION_FRAME, Gpr23);
    genDef(Ex, KEXCEPTION_FRAME, Gpr24);
    genDef(Ex, KEXCEPTION_FRAME, Gpr25);
    genDef(Ex, KEXCEPTION_FRAME, Gpr26);
    genDef(Ex, KEXCEPTION_FRAME, Gpr27);
    genDef(Ex, KEXCEPTION_FRAME, Gpr28);
    genDef(Ex, KEXCEPTION_FRAME, Gpr29);
    genDef(Ex, KEXCEPTION_FRAME, Gpr30);
    genDef(Ex, KEXCEPTION_FRAME, Gpr31);

    genDef(Ex, KEXCEPTION_FRAME, Fpr14);
    genDef(Ex, KEXCEPTION_FRAME, Fpr15);
    genDef(Ex, KEXCEPTION_FRAME, Fpr16);
    genDef(Ex, KEXCEPTION_FRAME, Fpr17);
    genDef(Ex, KEXCEPTION_FRAME, Fpr18);
    genDef(Ex, KEXCEPTION_FRAME, Fpr19);
    genDef(Ex, KEXCEPTION_FRAME, Fpr20);
    genDef(Ex, KEXCEPTION_FRAME, Fpr21);
    genDef(Ex, KEXCEPTION_FRAME, Fpr22);
    genDef(Ex, KEXCEPTION_FRAME, Fpr23);
    genDef(Ex, KEXCEPTION_FRAME, Fpr24);
    genDef(Ex, KEXCEPTION_FRAME, Fpr25);
    genDef(Ex, KEXCEPTION_FRAME, Fpr26);
    genDef(Ex, KEXCEPTION_FRAME, Fpr27);
    genDef(Ex, KEXCEPTION_FRAME, Fpr28);
    genDef(Ex, KEXCEPTION_FRAME, Fpr29);
    genDef(Ex, KEXCEPTION_FRAME, Fpr30);
    genDef(Ex, KEXCEPTION_FRAME, Fpr31);

    genVal(ExceptionFrameLength, (sizeof(KEXCEPTION_FRAME) + 7) & (~7));

    //
    // Swap Frame offset definitions.
    //

  DisableInc (HALPPC);

    genCom("Swap Frame Definitions and Length");

    genDef(Sw, KSWAP_FRAME, ConditionRegister);
    genDef(Sw, KSWAP_FRAME, SwapReturn);

    genVal(SwapFrameLength, (sizeof(KSWAP_FRAME) + 7) & (~7));

  EnableInc (HALPPC);

    //
    // Jump buffer offset definitions.
    //

  DisableInc (HALPPC);

    genCom("Jump Offset Definitions and Length");

    genDef(Jb, _JUMP_BUFFER, Fpr14);
    genDef(Jb, _JUMP_BUFFER, Fpr15);
    genDef(Jb, _JUMP_BUFFER, Fpr16);
    genDef(Jb, _JUMP_BUFFER, Fpr17);
    genDef(Jb, _JUMP_BUFFER, Fpr18);
    genDef(Jb, _JUMP_BUFFER, Fpr19);
    genDef(Jb, _JUMP_BUFFER, Fpr20);
    genDef(Jb, _JUMP_BUFFER, Fpr21);
    genDef(Jb, _JUMP_BUFFER, Fpr22);
    genDef(Jb, _JUMP_BUFFER, Fpr23);
    genDef(Jb, _JUMP_BUFFER, Fpr24);
    genDef(Jb, _JUMP_BUFFER, Fpr25);
    genDef(Jb, _JUMP_BUFFER, Fpr26);
    genDef(Jb, _JUMP_BUFFER, Fpr27);
    genDef(Jb, _JUMP_BUFFER, Fpr28);
    genDef(Jb, _JUMP_BUFFER, Fpr29);
    genDef(Jb, _JUMP_BUFFER, Fpr30);
    genDef(Jb, _JUMP_BUFFER, Fpr31);

    genDef(Jb, _JUMP_BUFFER, Gpr1);
    genDef(Jb, _JUMP_BUFFER, Gpr2);
    genDef(Jb, _JUMP_BUFFER, Gpr13);
    genDef(Jb, _JUMP_BUFFER, Gpr14);
    genDef(Jb, _JUMP_BUFFER, Gpr15);
    genDef(Jb, _JUMP_BUFFER, Gpr16);
    genDef(Jb, _JUMP_BUFFER, Gpr17);
    genDef(Jb, _JUMP_BUFFER, Gpr18);
    genDef(Jb, _JUMP_BUFFER, Gpr19);
    genDef(Jb, _JUMP_BUFFER, Gpr20);
    genDef(Jb, _JUMP_BUFFER, Gpr21);
    genDef(Jb, _JUMP_BUFFER, Gpr22);
    genDef(Jb, _JUMP_BUFFER, Gpr23);
    genDef(Jb, _JUMP_BUFFER, Gpr24);
    genDef(Jb, _JUMP_BUFFER, Gpr25);
    genDef(Jb, _JUMP_BUFFER, Gpr26);
    genDef(Jb, _JUMP_BUFFER, Gpr27);
    genDef(Jb, _JUMP_BUFFER, Gpr28);
    genDef(Jb, _JUMP_BUFFER, Gpr29);
    genDef(Jb, _JUMP_BUFFER, Gpr30);
    genDef(Jb, _JUMP_BUFFER, Gpr31);

    genDef(Jb, _JUMP_BUFFER, Cr);
    genDef(Jb, _JUMP_BUFFER, Iar);
    genDef(Jb, _JUMP_BUFFER, Type);

    //
    // Trap frame offset definitions.
    //

  EnableInc (HALPPC);

    genCom("Trap Frame Offset Definitions and Length");

    genDef(Tr, KTRAP_FRAME, TrapFrame);
    genDef(Tr, KTRAP_FRAME, OldIrql);
    genDef(Tr, KTRAP_FRAME, PreviousMode);
    genDef(Tr, KTRAP_FRAME, SavedApcStateIndex);
    genDef(Tr, KTRAP_FRAME, SavedKernelApcDisable);
    genDef(Tr, KTRAP_FRAME, ExceptionRecord);

    genDef(Tr, KTRAP_FRAME, Gpr0);
    genDef(Tr, KTRAP_FRAME, Gpr1);
    genDef(Tr, KTRAP_FRAME, Gpr2);
    genDef(Tr, KTRAP_FRAME, Gpr3);
    genDef(Tr, KTRAP_FRAME, Gpr4);
    genDef(Tr, KTRAP_FRAME, Gpr5);
    genDef(Tr, KTRAP_FRAME, Gpr6);
    genDef(Tr, KTRAP_FRAME, Gpr7);
    genDef(Tr, KTRAP_FRAME, Gpr8);
    genDef(Tr, KTRAP_FRAME, Gpr9);
    genDef(Tr, KTRAP_FRAME, Gpr10);
    genDef(Tr, KTRAP_FRAME, Gpr11);
    genDef(Tr, KTRAP_FRAME, Gpr12);

    genDef(Tr, KTRAP_FRAME, Fpr0);
    genDef(Tr, KTRAP_FRAME, Fpr1);
    genDef(Tr, KTRAP_FRAME, Fpr2);
    genDef(Tr, KTRAP_FRAME, Fpr3);
    genDef(Tr, KTRAP_FRAME, Fpr4);
    genDef(Tr, KTRAP_FRAME, Fpr5);
    genDef(Tr, KTRAP_FRAME, Fpr6);
    genDef(Tr, KTRAP_FRAME, Fpr7);
    genDef(Tr, KTRAP_FRAME, Fpr8);
    genDef(Tr, KTRAP_FRAME, Fpr9);
    genDef(Tr, KTRAP_FRAME, Fpr10);
    genDef(Tr, KTRAP_FRAME, Fpr11);
    genDef(Tr, KTRAP_FRAME, Fpr12);
    genDef(Tr, KTRAP_FRAME, Fpr13);

    genDef(Tr, KTRAP_FRAME, Fpscr);
    genDef(Tr, KTRAP_FRAME, Cr);
    genDef(Tr, KTRAP_FRAME, Xer);
    genDef(Tr, KTRAP_FRAME, Msr);
    genDef(Tr, KTRAP_FRAME, Iar);
    genDef(Tr, KTRAP_FRAME, Lr);
    genDef(Tr, KTRAP_FRAME, Ctr);

    genDef(Tr, KTRAP_FRAME, Dr0);
    genDef(Tr, KTRAP_FRAME, Dr1);
    genDef(Tr, KTRAP_FRAME, Dr2);
    genDef(Tr, KTRAP_FRAME, Dr3);
    genDef(Tr, KTRAP_FRAME, Dr4);
    genDef(Tr, KTRAP_FRAME, Dr5);
    genDef(Tr, KTRAP_FRAME, Dr6);
    genDef(Tr, KTRAP_FRAME, Dr7);

    genVal(TrapFrameLength, (sizeof(KTRAP_FRAME) + 7) & (~7));

    //
    // Usermode callout frame definitions
    //

  DisableInc (HALPPC);

    genCom("Usermode callout frame definitions");

    genDef(Cu, KCALLOUT_FRAME, Frame);
    genDef(Cu, KCALLOUT_FRAME, CbStk);
    genDef(Cu, KCALLOUT_FRAME, TrFr);
    genDef(Cu, KCALLOUT_FRAME, InStk);
    genDef(Cu, KCALLOUT_FRAME, TrIar);
    genDef(Cu, KCALLOUT_FRAME, TrToc);
    genDef(Cu, KCALLOUT_FRAME, R3);
    genDef(Cu, KCALLOUT_FRAME, R4);
    genDef(Cu, KCALLOUT_FRAME, Lr);
    genDef(Cu, KCALLOUT_FRAME, Gpr);
    genDef(Cu, KCALLOUT_FRAME, Fpr);

    genVal(CuFrameLength, sizeof(KCALLOUT_FRAME));

    genCom("Usermode callout user frame definitions");

    genDef(Ck, UCALLOUT_FRAME, Frame);
    genDef(Ck, UCALLOUT_FRAME, Buffer);
    genDef(Ck, UCALLOUT_FRAME, Length);
    genDef(Ck, UCALLOUT_FRAME, ApiNumber);
    genDef(Ck, UCALLOUT_FRAME, Lr);
    genDef(Ck, UCALLOUT_FRAME, Toc);

    genVal(CkFrameLength, sizeof(UCALLOUT_FRAME));

    //
    // Exception stack frame definitions
    //

    genCom("Exception stack frame frame definitions");

    genVal(STK_SLACK_SPACE, STK_SLACK_SPACE);
    genAlt(TF_BASE, KEXCEPTION_STACK_FRAME, TrapFrame);
    genAlt(KERN_SYS_CALL_FRAME, KEXCEPTION_STACK_FRAME, ExceptionFrame);
    genAlt(EF_BASE, KEXCEPTION_STACK_FRAME, ExceptionFrame);
    genDef(Ef, KEXCEPTION_STACK_FRAME, Lr);
    genDef(Ef, KEXCEPTION_STACK_FRAME, Cr);
    genAlt(USER_SYS_CALL_FRAME, KEXCEPTION_STACK_FRAME, SlackSpace);
    genAlt(STACK_DELTA_NEWSTK, KEXCEPTION_STACK_FRAME, SlackSpace);
    genVal(STACK_DELTA, sizeof(KEXCEPTION_STACK_FRAME));

  EnableInc (HALPPC);

    //
    // Processor State Frame offsets relative to base
    //

    genCom("Processor State Frame Offset Definitions");

    genDef(Ps, KPROCESSOR_STATE, ContextFrame);
    genDef(Ps, KPROCESSOR_STATE, SpecialRegisters);
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr0);
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr1);
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr2);
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr3);
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr4);
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr5);
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr6);
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr7);
    genDef(Sr, KSPECIAL_REGISTERS, Sprg0);
    genDef(Sr, KSPECIAL_REGISTERS, Sprg1);
    genDef(Sr, KSPECIAL_REGISTERS, Sr0);
    genDef(Sr, KSPECIAL_REGISTERS, Sr1);
    genDef(Sr, KSPECIAL_REGISTERS, Sr2);
    genDef(Sr, KSPECIAL_REGISTERS, Sr3);
    genDef(Sr, KSPECIAL_REGISTERS, Sr4);
    genDef(Sr, KSPECIAL_REGISTERS, Sr5);
    genDef(Sr, KSPECIAL_REGISTERS, Sr6);
    genDef(Sr, KSPECIAL_REGISTERS, Sr7);
    genDef(Sr, KSPECIAL_REGISTERS, Sr8);
    genDef(Sr, KSPECIAL_REGISTERS, Sr9);
    genDef(Sr, KSPECIAL_REGISTERS, Sr10);
    genDef(Sr, KSPECIAL_REGISTERS, Sr11);
    genDef(Sr, KSPECIAL_REGISTERS, Sr12);
    genDef(Sr, KSPECIAL_REGISTERS, Sr13);
    genDef(Sr, KSPECIAL_REGISTERS, Sr14);
    genDef(Sr, KSPECIAL_REGISTERS, Sr15);
    genDef(Sr, KSPECIAL_REGISTERS, DBAT0L);
    genDef(Sr, KSPECIAL_REGISTERS, DBAT0U);
    genDef(Sr, KSPECIAL_REGISTERS, DBAT1L);
    genDef(Sr, KSPECIAL_REGISTERS, DBAT1U);
    genDef(Sr, KSPECIAL_REGISTERS, DBAT2L);
    genDef(Sr, KSPECIAL_REGISTERS, DBAT2U);
    genDef(Sr, KSPECIAL_REGISTERS, DBAT3L);
    genDef(Sr, KSPECIAL_REGISTERS, DBAT3U);
    genDef(Sr, KSPECIAL_REGISTERS, IBAT0L);
    genDef(Sr, KSPECIAL_REGISTERS, IBAT0U);
    genDef(Sr, KSPECIAL_REGISTERS, IBAT1L);
    genDef(Sr, KSPECIAL_REGISTERS, IBAT1U);
    genDef(Sr, KSPECIAL_REGISTERS, IBAT2L);
    genDef(Sr, KSPECIAL_REGISTERS, IBAT2U);
    genDef(Sr, KSPECIAL_REGISTERS, IBAT3L);
    genDef(Sr, KSPECIAL_REGISTERS, IBAT3U);
    genDef(Sr, KSPECIAL_REGISTERS, Sdr1);

    genVal(ProcessorStateLength, ((sizeof(KPROCESSOR_STATE) + 15) & ~15));

    //
    // Loader Parameter Block offset definitions.
    //

    genCom("Loader Parameter Block Offset Definitions");

    genDef(Lpb, LOADER_PARAMETER_BLOCK, LoadOrderListHead);
    genDef(Lpb, LOADER_PARAMETER_BLOCK, MemoryDescriptorListHead);
    genDef(Lpb, LOADER_PARAMETER_BLOCK, KernelStack);
    genDef(Lpb, LOADER_PARAMETER_BLOCK, Prcb);
    genDef(Lpb, LOADER_PARAMETER_BLOCK, Process);
    genDef(Lpb, LOADER_PARAMETER_BLOCK, Thread);
    genDef(Lpb, LOADER_PARAMETER_BLOCK, RegistryLength);
    genDef(Lpb, LOADER_PARAMETER_BLOCK, RegistryBase);
    genAlt(LpbInterruptStack, LOADER_PARAMETER_BLOCK, u.Ppc.InterruptStack);
    genAlt(LpbFirstLevelDcacheSize, LOADER_PARAMETER_BLOCK, u.Ppc.FirstLevelDcacheSize);
    genAlt(LpbFirstLevelDcacheFillSize, LOADER_PARAMETER_BLOCK, u.Ppc.FirstLevelDcacheFillSize);
    genAlt(LpbFirstLevelIcacheSize, LOADER_PARAMETER_BLOCK, u.Ppc.FirstLevelIcacheSize);
    genAlt(LpbFirstLevelIcacheFillSize, LOADER_PARAMETER_BLOCK, u.Ppc.FirstLevelIcacheFillSize);
    genAlt(LpbHashedPageTable, LOADER_PARAMETER_BLOCK, u.Ppc.HashedPageTable);
    genAlt(LpbPanicStack, LOADER_PARAMETER_BLOCK, u.Ppc.PanicStack);
    genAlt(LpbPcrPage, LOADER_PARAMETER_BLOCK, u.Ppc.PcrPage);
    genAlt(LpbPdrPage, LOADER_PARAMETER_BLOCK, u.Ppc.PdrPage);
    genAlt(LpbSecondLevelDcacheSize, LOADER_PARAMETER_BLOCK, u.Ppc.SecondLevelDcacheSize);
    genAlt(LpbSecondLevelDcacheFillSize, LOADER_PARAMETER_BLOCK, u.Ppc.SecondLevelDcacheFillSize);
    genAlt(LpbSecondLevelIcacheSize, LOADER_PARAMETER_BLOCK, u.Ppc.SecondLevelIcacheSize);
    genAlt(LpbSecondLevelIcacheFillSize, LOADER_PARAMETER_BLOCK, u.Ppc.SecondLevelIcacheFillSize);
    genAlt(LpbPcrPage2, LOADER_PARAMETER_BLOCK, u.Ppc.PcrPage2);
    genAlt(LpbIcacheMode, LOADER_PARAMETER_BLOCK, u.Ppc.IcacheMode);
    genAlt(LpbDcacheMode, LOADER_PARAMETER_BLOCK, u.Ppc.DcacheMode);
    genAlt(LpbNumberCongruenceClasses, LOADER_PARAMETER_BLOCK, u.Ppc.NumberCongruenceClasses);
    genAlt(LpbKseg0Top, LOADER_PARAMETER_BLOCK, u.Ppc.Kseg0Top);
    genAlt(LpbMemoryManagementModel, LOADER_PARAMETER_BLOCK, u.Ppc.MemoryManagementModel);
    genAlt(LpbHashedPageTableSize, LOADER_PARAMETER_BLOCK, u.Ppc.HashedPageTableSize);
    genAlt(LpbKernelKseg0PagesDescriptor, LOADER_PARAMETER_BLOCK, u.Ppc.KernelKseg0PagesDescriptor);
    genAlt(LpbMinimumBlockLength, LOADER_PARAMETER_BLOCK, u.Ppc.MinimumBlockLength);
    genAlt(LpbMaximumBlockLength, LOADER_PARAMETER_BLOCK, u.Ppc.MaximumBlockLength);

    //
    // Memory Allocation Descriptor offset definitions.
    //

    genCom("Memory Allocation Descriptor Offset Definitions");

    genDef(Mad, MEMORY_ALLOCATION_DESCRIPTOR, ListEntry);
    genDef(Mad, MEMORY_ALLOCATION_DESCRIPTOR, MemoryType);
    genDef(Mad, MEMORY_ALLOCATION_DESCRIPTOR, BasePage);
    genDef(Mad, MEMORY_ALLOCATION_DESCRIPTOR, PageCount);

  DisableInc (HALPPC);

    //
    // Address space layout definitions
    //

  EnableInc (HALPPC);

    genCom("Address Space Layout Definitions");

    genVal(KUSEG_BASE, KUSEG_BASE);
    genVal(KSEG0_BASE, KSEG0_BASE);
    dumpf("#define KSEG1_BASE PCR->Kseg0Top\n");
    dumpf("#define KSEG2_BASE KSEG1_BASE\n");

  DisableInc (HALPPC);

    genVal(SYSTEM_BASE, SYSTEM_BASE);
    genVal(PDE_BASE, PDE_BASE);
    genVal(PTE_BASE, PTE_BASE);

    //
    // Page table and page directory entry definitions
    //

  EnableInc (HALPPC);

    genCom("Page Table and Directory Entry Definitions");

    genVal(PAGE_SIZE, PAGE_SIZE);
    genVal(PAGE_SHIFT, PAGE_SHIFT);
    genVal(PDI_SHIFT, PDI_SHIFT);
    genVal(PTI_SHIFT, PTI_SHIFT);

  DisableInc (HALPPC);

    //
    // Breakpoint instruction definitions
    //

  EnableInc (HALPPC);

    genCom("Breakpoint Definitions");

    genVal(USER_BREAKPOINT, USER_BREAKPOINT);
    genVal(KERNEL_BREAKPOINT, KERNEL_BREAKPOINT);
    genVal(BREAKIN_BREAKPOINT, BREAKIN_BREAKPOINT);

  DisableInc (HALPPC);

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

  EnableInc (HALPPC);

    genCom("Miscellaneous Definitions");

    genVal(Executive, Executive);
    genVal(KernelMode, KernelMode);
    genVal(FALSE, FALSE);
    genVal(TRUE, TRUE);
    genVal(UNCACHED_POLICY, UNCACHED_POLICY);
    genVal(KiPcr, KIPCR);
    genVal(KiPcr2, KIPCR2);

  DisableInc (HALPPC);

    genVal(BASE_PRIORITY_THRESHOLD, BASE_PRIORITY_THRESHOLD);
    genVal(EVENT_PAIR_INCREMENT, EVENT_PAIR_INCREMENT);
    genVal(LOW_REALTIME_PRIORITY, LOW_REALTIME_PRIORITY);
    genVal(KERNEL_STACK_SIZE, KERNEL_STACK_SIZE);
    genVal(KERNEL_LARGE_STACK_COMMIT, KERNEL_LARGE_STACK_COMMIT);
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
dumpf (const char *format, ...)
{
    va_list(arglist);

    va_start(arglist, format);

    if (OutputEnabled & KSPPC) {
        vfprintf (KsPpc, format, arglist);
    }

    if (OutputEnabled & HALPPC) {
        vfprintf (HalPpc, format, arglist);
    }

    va_end(arglist);
}
