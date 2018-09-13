/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdpcpu.h

Abstract:

    Machine specific kernel debugger data types and constants

Author:

    Mark Lucovsky (markl) 29-Aug-1990
    Joe Notarangelo       24-June-1992  (ALPHA version)

Revision History:

--*/

#ifndef _KDPCPU_
#define _KDPCPU_

#include "alphaops.h"

//
// Define KD private PCR routines.
//
// Using the following private KD routines allows the kernel debugger to
// step over breakpoints in modules that call the standard PCR routines.
//

PKPCR KdpGetPcr();

ULONG KdpReadInternalProcessorState(PVOID, ULONG);
ULONG KdpReadInternalProcessorCounters(PVOID, ULONG);
VOID
KdpReadIoSpaceExtended (
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpWriteIoSpaceExtended (
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

struct _KPRCB *
KdpGetCurrentPrcb();

struct _KTHREAD *
KdpGetCurrentThread();

//
// Redefine the standard PCR routines
//
#undef KiPcr
#define KiPcr KdpGetPcr()

#undef KeGetPcr
#undef KeGetCurrentPrcb
#undef KeGetCurrentThread
#undef KeIsExecutingDpc
#define KeGetPcr() KdpGetPcr()
#define KeGetCurrentPrcb() KdpGetCurrentPrcb()
#define KeGetCurrentThread() KdpGetCurrentThread()

//
// Define TYPES
//

#define KDP_BREAKPOINT_TYPE  ULONG

// longword aligned
#define KDP_BREAKPOINT_ALIGN 3

// actual instruction is "call_pal kbpt"
#define KDP_BREAKPOINT_VALUE KBPT_FUNC

#endif // _KDPCPU_

