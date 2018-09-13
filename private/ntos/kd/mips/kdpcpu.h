/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdpcpu.h

Abstract:

    Machine specific kernel debugger data types and constants

Author:

    Mark Lucovsky (markl) 29-Aug-1990

Revision History:

--*/

#ifndef _KDPCPU_
#define _KDPCPU_

#define KDP_BREAKPOINT_TYPE  ULONG
#define KDP_BREAKPOINT_ALIGN 3
#define KDP_BREAKPOINT_VALUE 0x0016000d

#endif // _KDPCPU_
