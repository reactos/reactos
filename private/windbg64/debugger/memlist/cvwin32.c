/*
 *  CVWIN32.C 
 *
 *
 */

#if defined( TARGWIN32 ) || defined( TARGWIN32S )
#include <windows.h>
#endif
#include <stdlib.h>
#include <malloc.h>
#include "cvwin32.h"



//-----------------------------------------------------------------------------
//  cvwin32.c
//
//  Copyright (C) 1993, Microsoft Corporation
//
//  Purpose:
//      do stuff that can't be done in bm.c due to collisions
//      in types and such with windows.h
//
//  Functions/Methods present:
//
//  Revision History:
//
//  []      05-Mar-1993 Dans    Created
//
//-----------------------------------------------------------------------------

#if defined(TARGWIN32) || defined(TARGWIN32S)   /* { the whole file */
#define FASTCALL __fastcall

#if !defined(NO_CRITSEC)    /* { */

typedef CRITICAL_SECTION    CS, *_PCS;


CS  rgcsCv[icsMax];

void FASTCALL CVInitCritSection(ICS ics) {
    InitializeCriticalSection ( &rgcsCv[ics] );
    }

void FASTCALL CVEnterCritSection(ICS ics) {
    EnterCriticalSection ( &rgcsCv[ics] );
    }

void FASTCALL CVLeaveCritSection(ICS ics) {
    LeaveCriticalSection ( &rgcsCv[ics] );
    }

void FASTCALL CVDeleteCritSection(ICS ics) {
    DeleteCriticalSection ( &rgcsCv[ics] );
    }

PCS FASTCALL        PcsAllocInit() {
    _PCS pcs = (_PCS)calloc ( sizeof(CRITICAL_SECTION), 1 );
    if ( pcs )
        InitializeCriticalSection ( pcs );
    return pcs;
    }
void FASTCALL       FreePcs ( PCS pcs ) {
    DeleteCriticalSection ( (_PCS)pcs );
    free ( pcs );
    }
void FASTCALL       AcquireLockPcs ( PCS pcs ) {
    EnterCriticalSection ( (_PCS)pcs );
    }

void FASTCALL       ReleaseLockPcs ( PCS pcs ) {
    LeaveCriticalSection ( (_PCS)pcs );
    }


#endif  /* } NO_CRITSEC */

#endif  /* } the whole file */
