/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdpcpu.h

Abstract:

    Machine specific kernel debugger data types and constants

Author:

    Chuck Bauman 14-Aug-1993

Revision History:

    Based on Mark Lucovsky (markl) MIPS version 29-Aug-1990
    Modified KDP_BREAKPOINT_VALUE to represent "twi 31,0,16" 14-Aug-1993

--*/

#ifndef _KDPCPU_
#define _KDPCPU_

#define KDP_BREAKPOINT_TYPE  ULONG
#define KDP_BREAKPOINT_ALIGN 3
#define KDP_BREAKPOINT_VALUE 0x0FE00016

#endif // _KDPCPU_
