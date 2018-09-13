//-----------------------------------------------------------------------------
//  cvwin32.h
//
//  Copyright (C) 1993, Microsoft Corporation
//
//  Purpose:
//      api for the 4 functions in cvwin32.c
//
//  Functions/Methods present:
//
//  Revision History:
//
//  []      05-Mar-1993 Dans    Created
//
//-----------------------------------------------------------------------------

#if !defined(_cvwin32_h)
#define _cvwin32_h 1

#if ( defined ( TARGWIN32 ) || defined ( TARGWIN32S ) ) && !defined ( NO_CRITSEC )

typedef enum ICS {  // index to critical section
    icsBm,          // handle-based memory allocation routines
    icsWmalloc,     // pointer-based memory allocation routines
    icsMax
} ICS;

typedef void *  PCS;

void _fastcall      CVInitCritSection(ICS);
void _fastcall      CVLeaveCritSection(ICS);
void _fastcall      CVEnterCritSection(ICS);
void _fastcall      CVDeleteCritSection(ICS);
PCS  _fastcall      PcsAllocInit();
void _fastcall      FreePcs(PCS);
void _fastcall      AcquireLockPcs(PCS);
void _fastcall      ReleaseLockPcs(PCS);

#else

#define CVInitCritSection(ICS)
#define CVLeaveCritSection(ICS)
#define CVEnterCritSection(ICS)
#define CVDeleteCritSection(ICS)
#define PcsAllocInit()              NULL
#define FreePcs(PCS)
#define AcquireLockPcs(PCS)
#define ReleaseLockPcs(PCS)

#endif
#endif
