/*** mm.c - Memory Manager routines and callback routines for cw
*
*   Copyright <C> 1990, Microsoft Corporation
*
* Purpose:  handle the near and far memory requests of dlls, cw and help systems.
*
* Notes:    Include files required for use of the memory manager are:
*               #include "cvdef.h"
*               #include "mm.h"
*               #include "mmproto.h"
*           For an example of Memory Manager usage please see testmm.c.
*
*           _bheap_reset is special and assumes Medium > code memory
*           model
*
*           There are some CW callback and overridden functions in this
*           system.  Becareful when making mods to them.  See Auxcow.c for
*           more information.
*
*************************************************************************/

#include <windows.h>
#include "cvtypes.h"

#include "mm.h"
#include "mm.hmd"
#include "mm.hpt"

#include "ms.h"

#include <malloc.h>     /* for malloc() / free() */
#include <dos.h>        // for FP_SEG and FP_OFF
#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
/* #include "osassert.h" */
#include <assert.h>
#include <memory.h>

#ifdef HOST32
#define _memmax() 100*64L
#endif

typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned long   ulong;

#ifdef TARGDOS16
unsigned long ulcbDLLBuffer;
#endif // DOS16

#ifndef TARGWIN
#define FHEAPCHK()    _fheapchk()
#else // !WIN
#define FHEAPCHK()    _HEAPOK
#endif // !WIN

//
// Handle based allocation table
//

static MB mbTableInit[ MB_ENTRY_MAX ];
static PMB pmbTable[ MB_TABLE_MAX ] = { mbTableInit };

//
//  Near memory buffer for CW
//

static unsigned char rgbNearWS[ cbNEARBUFFER ];
static unsigned char *   pbNextNearBlock = rgbNearWS;

//
// Far work space data
//
#if defined(DOS16)
static unsigned short  selBasedHeap[MAXHEAPSELS] = { _NULLSEG, _NULLSEG, _NULLSEG, _NULLSEG, _NULLSEG, _NULLSEG, _NULLSEG };
#endif

#define cbDEFAULTHLPBUF     (0x6000)                /* default size of help buf */
ulong       ucbHelpBuffer = cbDEFAULTHLPBUF;

//************************************************************************
// Installable Expression evaluator Call Backs only.
//

/*** MHMemAllocate
*
*   Purpose: To far memory for installable expression evaluators.
*
*   Input:
*   cb  The number of bytes to allocate
*
*   Output:
*   Returns:
*   A far pointer to the memory allocated
*
*   Exceptions:
*
*   Notes: This is a thunk to the CodeView Memory manager
*
*************************************************************************/
HDEP LOADDS PASCAL
MHMemAllocate ( size_t cb )
{
    return ( MMhAllocMb ( (USHORT) MMDLLHEAP, (UCHAR) FMALLOCED, (USHORT) cb ) );

}

/*** MHIsMemLocked
*
*   Purpose: To check to see if memory is locked
*
*   Input:
*   hMem    - A handle to the memory
*
*   Output:
*   Returns:
*       True if memory is locked
*
*   Exceptions:
*
*   Notes:
*
*************************************************************************/
SHFLAG  LOADDS PASCAL
MHIsMemLocked ( HDEP hmem )
{
    return ( MMcbGetLockFromMb ( hmem ) );
}

/*** MHMemFree
*
*   Purpose: To free memory previously allocated with MHAllocate
*
*   Input:
*   hMem    - A handle of the memory to free
*
*   Output:
*   Returns:
*
*   Exceptions:
*
*   Notes:
*   If an invalid memory pointer is passed to be freed, unexpected
*   results may occur
*
*************************************************************************/
void LOADDS PASCAL
MHMemFree ( HDEP hmem )
{
    MMDeallocMb ( hmem );
}


/*** MHMemReAlloc
*
*   Purpose:    To reallocate some memory
*
*   Input:
*   hMem    - A handle to the old memory to reallocate
*   cbNew   - The new size
*
*   Output:
*   Returns:
*   - A new handle to the newly allocated memory. Or a NULL on error.
*     If an error ocurred, the old handle is still valid, otherwise
*     the old handle is invalid.
*
*   Exceptions:
*
*   Notes:
*   For this to work, the memory must not be locked!!
*
*   If a larger block is requested, the extended memory has
*   uninitialized data in it.
*
*************************************************************************/
HDEP LOADDS PASCAL
MHMemReAlloc( HDEP hmem, size_t cbNew )
{
    return ( MMhReallocMb ( hmem, cbNew, 0 ) );
}

/*** MHMemLock
*
*   Purpose: To lock into physical memory allocated by MHMemAllocate
*
*   Input:
*   hMem    - A handle to the memory
*
*   Output:
*   Returns:
*       The physical memory address
*
*   Exceptions:
*
*   Notes:  This is a IEE callback function
*
*
*************************************************************************/
HVOID   LOADDS PASCAL
MHMemLock( HDEP hmem )
{
    return ( MMlpvLockMb ( hmem ) );
}

/*** MHMemUnLock
*
*   Purpose: To unlock memory previously lock by MHMemLock
*
*   Input:
*   hMem
*
*   Output:
*   Returns:
*
*   Exceptions:
*
*   Notes:
*   This is a IEE callback only
*
*************************************************************************/
void    LOADDS PASCAL
MHMemUnLock ( HDEP hmem )
{
    MMbUnlockMb ( hmem );
}



//************************************************************************
// General Exported Far Memory Manager
//

/*** MHwInit - Initialize a block of far memory to use during codeview.
*
* Purpose:  To set up a block of far memory for CW, HELP, IEE, OSDebug
*       systems to use as well (in dos).
*
* Input:    none.
*
* Output:   none.
*
* Notes:    This routine can cause codeview to exit if cbFWORKMIN bytes
*       of memory is not available.
*       This routine sets up the space (in dos3) for SBAlloc, etc.,
*       to use if necessary.  It also sets up the initial SB table
*       and the SB pointer table.
*
*************************************************************************/
unsigned short PASCAL
MMwInit () {

unsigned short  i;
unsigned short        retval = TRUE;

#if defined(DOS16)
long        cbHeap;
static unsigned short  cselAllocated;

    selBasedHeap[ MMEMERGENCYHEAP ] = bselAlloc ( &i, cbFWORKMIN );
    if ( i < cbFWORKMIN ) {
        retval = FALSE;
    }
    else {
        cselAllocated = MMCOWHEAP;
        selBasedHeap[ MMCOWHEAP ] = bselAlloc ( &i, cbFWORKSTART );
        if ( i < cbFWORKMIN ) {
            retval = FALSE;
        }
        else {
            cselAllocated++;

            cbHeap = ulcbDLLBuffer;
            for ( cselAllocated;
                  cselAllocated < cHEAPSEL && cbHeap;
                  cselAllocated++ ) {

                ushort  cbThisAlloc = 0;
                selBasedHeap[ cselAllocated ] = bselAlloc ( &cbThisAlloc, cbHeap );
                cbHeap -= cbThisAlloc;
            }

            cbHeap = ucbHelpBuffer;
            for ( cselAllocated;
                  cselAllocated < cHEAPSEL && cbHeap;
                  cselAllocated++ ) {

                ushort  cbThisAlloc = 0;
                selBasedHeap[ cselAllocated ] = bselAlloc ( &cbThisAlloc, cbHeap );
                cbHeap -= cbThisAlloc;
            }
        }
    }
#endif


    // initialize the sb manager table

    for ( i = 1; i < MB_TABLE_MAX; i++ ) {
        pmbTable[ i ] = NULL;
    }
    for ( i = 0; i < MB_ENTRY_MAX; i++ ) {
        mbTableInit[ i ].flags = MBUNDEFINED;
        mbTableInit[ i ].cb = 0;
        mbTableInit[ i ].lpvBlock = NULL;
    }
    return retval;
}

/*** MMlpvAlloc - CV API callback for far memory buffer handling
*
* Purpose:  To allocate far memory for CW, CV, DLLs, IEE, and the Help
*           system (dos only)
*
* Input:    unsigned short wPriority level of priority required for memory request.
*           unsigned short cb, count of bytes requested.
*
* Output:
*  Returns  far pointer to block if successful, NULL if not.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void FAR * FAR PASCAL
MMlpvAlloc( unsigned short wPriority, unsigned short cb) {

void FAR *  lpv = NULL;

#if defined(DOS16)
unsigned short      i;
    for ( i = wPriority; i < cHEAPSEL; i++) {

        if ( selBasedHeap[ i ] != _NULLSEG ) {
            unsigned    uOff = (unsigned )_bmalloc ( selBasedHeap[ i] , cb );

            assert ( (_bheapset( selBasedHeap[i],0xff ) == _HEAPOK) || (_bheapset( selBasedHeap[i],0xff ) == _HEAPEMPTY) );

            if ( uOff != (unsigned) _NULLOFF) {
                FP_OFF ( lpv ) = uOff;
                FP_SEG ( lpv ) = selBasedHeap[ i ];
                assert ( (_bheapset( selBasedHeap[i],0xff ) == _HEAPOK) );
                break;
            }
        }
    }
#else   // DOS3
#ifdef HOST32
    assert ( _heapchk ( ) == _HEAPOK );
    lpv = malloc( cb );
#else
    assert ( FHEAPCHK() == _HEAPOK );
    lpv = _fmalloc( cb );
#endif // HOST32
#endif // DOS3
    return lpv;
}

/*** MMFreeLpv - CV API for far memory buffer handling free of lpv alloc
*
* Purpose:  To free a far work buffer
*
* Input:    void far * lpv, far pointer to block to deallocate
*
* Notes:    Since all blocks are allocated with only an offset, we just
*       have to match that offset value with the value in the
*       allocation table, fmb.fws.
*
*************************************************************************/
void FAR PASCAL
MMFreeLpv( void FAR * lpv)
{
#if defined(DOS16)
    int FAR _cdecl _bheap_reset ( unsigned );

    _bfree( FP_SEG(lpv), FP_OFF(lpv));
    //
    // _bheap_reset is NOT a part of the standard c6 runtime...
    //  supplied to reduce fragmentation, it resets the rover ptr to
    //  the first free block in the based heap.
    //
    _bheap_reset ( FP_SEG(lpv) );
    assert ( (_bheapset( FP_SEG(lpv),0xff ) == _HEAPOK) || (_bheapset( FP_SEG(lpv), 0xff ) == _HEAPEMPTY) );
#else   // dos5
#ifdef HOST32
    assert ( _heapchk ( ) == _HEAPOK );
    free ( lpv );
#else
    assert ( FHEAPCHK ( ) == _HEAPOK );
    _ffree( lpv );
#endif
#endif
}


/*** MMhAllocMb - Allocate handle based memory
*
*   Purpose:    To allocate cb bytes of memory and return a handle to that
*       memory.
*
*   Input:  cb, count of bytes
*
*   Returns:    handle to memory if successful, MBUNDEFINED if not.
*
*   Note:   This routine can also allocate near memory when the parameter
*       wWhere is set to MALLOCED.  Currently, GlobalAlloc and
*       GlobalRealloc use this method, so that the listbox.c will
*       grab near memory first.  This is not detrimental, since it
*       will not generally use too much and most dialogs have no
*       need for near memory.  Three exceptions come to mind:
*       Breakpoints, watches, and quickwatch dialogs.  Hence,
*       before a near malloc is done, the amount of free near
*       memory is checked so that some room is left.  See the
*       enum cbNEARLEAVEFREE.
*
*************************************************************************/
HDEP PASCAL FAR
MMhAllocMb(
    unsigned short      wPriority,
    unsigned char       bWhere,
    unsigned short      cb
    )
{
register unsigned short   i = 0;
register unsigned short   j = MB_ENTRY_MAX;
MBH             mbhRet;

//  first of all, find where a sb entry is, and then an entry in that one.

    mbhRet.hmem = MBUNDEFINED;
    while ( (j == MB_ENTRY_MAX) &&
        (i < MB_TABLE_MAX) &&
        (pmbTable[ i ] != NULL) ) {
        PMB pmb = pmbTable[ i ];
        for ( j = 0; j < MB_ENTRY_MAX; j++, pmb++ ) {
            if ( pmb->flags == MBUNDEFINED ) {
            // found a free handle
            break;
            }
        }

        if ( j == MB_ENTRY_MAX ) { // none found, go to next mb.
                i++;
        }
    }
//
//  at this point, i and j are VERY interesting.  if i == MB_TABLE_MAX,
//  then we are SOL, no sb room is available.  Otherwise,
//  everything is ok, i will have the sb table index, and j
//  will have the sb entry index.
//
//
    if ( i < MB_TABLE_MAX ) {

        if ( pmbTable[ i ] == NULL ) {  // allocate a new sb table
            pmbTable[ i ] = MMlpvAlloc ( wPriority, sizeof( MB ) * MB_ENTRY_MAX );
            j = 0;

            if ( pmbTable[ i ] != NULL ) {
                //
                // initialize the sb table
                //
                unsigned short    ind;
                PMB pmb = pmbTable[ i ];
                for ( ind = 0; ind < MB_ENTRY_MAX; ind++, pmb++ ) {
                    pmb->flags = MBUNDEFINED;
                    pmb->cb = 0;
                    pmb->lpvBlock = NULL;
                    pmb->swLocked = -1;
                }
            }
        }

        if ( pmbTable[ i ] != NULL ) {
            PMB pmb = pmbTable[ i ];
            pmb += j;
            // try in near if bWhere == MALLOCED and
            // the biggest block is at least as big as cb + cbNEARLEAVEFREE

#ifndef TARGWIN
            if ( bWhere == MALLOCED && cb + cbNEARLEAVEFREE < _memmax() ){
                void *  p = malloc ( cb );
                if ( p != NULL ) {
                    pmb->lpvBlock = (void FAR *) p;
                    pmb->cb = cb;
                    pmb->swLocked = 0;
                    mbhRet.mbhi.mbNum = (uchar) i;
                    mbhRet.mbhi.index = (uchar) j;
                    mbhRet.mbhi.flags = pmb->flags = bWhere;
                }
            }
#else // !TARGWIN
            if ( bWhere == MALLOCED ){
                void *  p = malloc ( cb );
                if ( p != NULL ) {
                    pmb->lpvBlock = (void FAR *) p;
                    pmb->cb = cb;
                    pmb->swLocked = 0;
                    mbhRet.mbhi.mbNum = (uchar) i;
                    mbhRet.mbhi.index = (uchar) j;
                    mbhRet.mbhi.flags = pmb->flags = bWhere;
                }
            }

#endif // !TARGWIN

            if ( pmb->lpvBlock == NULL ) {
                if ( (pmb->lpvBlock = MMlpvAlloc ( wPriority, cb )) != NULL ) {
                    pmb->cb = cb;
                    pmb->swLocked = 0;
                    mbhRet.mbhi.mbNum = (uchar) i;
                    mbhRet.mbhi.index = (uchar) j;
                    mbhRet.mbhi.flags = pmb->flags = FMALLOCED;
                }
                else {
                    pmb->flags = MBUNDEFINED;
                    pmb->swLocked = -1;
                    pmb->cb = 0;
                }
            }
        }
    }
    return ( mbhRet.hmem );
}

/*** MMDeallocMb - Deallocate handle based memory
*
*   Purpose:    To deallocate memory allocated using SBAlloc.
*
*   Input:  h,  handle to memory
*
*************************************************************************/
void PASCAL FAR
MMDeallocMb ( HDEP hmem )
{
PMB pmb = PmbHmemToPmb ( hmem );

    if ( pmb != NULL ) {
        if ( pmb->flags == MALLOCED ) {
            free ( (void *) (pmb->lpvBlock) );
        }
        else {
            MMFreeLpv ( pmb->lpvBlock );
        }
        pmb->flags = MBUNDEFINED;
        pmb->cb = 0;
        pmb->lpvBlock = NULL;
    }
}
/*** MMlpvHandleToLp - map SB handle to far *
*
*   Purpose:    To dereference an sb handle to a far *
*
*   Input:  h, handle
*
*   Returns:    far * if successful, NULL if not.
*
*************************************************************************/
void FAR * PASCAL FAR
MMlpvHandleToLp ( HDEP hmem )
{
PMB     pmb;
void FAR *  retval = NULL;

    if ( ( pmb = PmbHmemToPmb ( hmem ) ) != NULL ) {
        retval = pmb->lpvBlock;
    }
    return retval;
}

/*** MMlpvLockMb - map MB handle to far *
*
*   Purpose:    To dereference an sb handle to a far *
*
*   Input:  wHandle, handle
*
*   Returns:    far * if successful, NULL if not.
*
*************************************************************************/
void FAR * FAR PASCAL
MMlpvLockMb (HDEP hmem )
{
PMB pmb;
    pmb = PmbHmemToPmb ( hmem );
    assert ( pmb != NULL );
    assert ( pmb->swLocked >= 0 );
    pmb->swLocked++;
    return ( MMlpvHandleToLp ( hmem ) );
}

/*** MMbUnlockMb - Decrement Lock count
*
*   Purpose:    To decrement the lock count on MB block given a handle.
*               This function will decrement the internal counter
*
*   Input:  h, handle
*
*   Returns:    far * if successful, NULL if not.
*
*************************************************************************/
unsigned short FAR PASCAL
MMbUnlockMb ( HDEP hmem )
{
PMB pmb = PmbHmemToPmb ( hmem );
    assert ( pmb != NULL );
    assert ( pmb->swLocked > 0 );
    pmb->swLocked--;
    return ((pmb->swLocked > 0 ));
}
/*** MMwHandleSize - Get size of block of memory from handle.
*
*   Purpose:    To get the size of the block of memory referenced by a
*               MBH handle.
*
*   Input:  h,  handle
*
*   Returns:    cb of the block if successful, 0 if not.
*
*************************************************************************/
unsigned short PASCAL FAR
MMwHandleSize ( HDEP hmem )
{
PMB pmb;
unsigned short    retval = 0;

    if ( ( pmb = PmbHmemToPmb ( hmem    ) ) != NULL ) {
        retval = pmb->cb;
    }
    return retval;
}

/*** MMhReallocMb (wHandle, cb, flags)
*
*Purpose:
*   Realloc a handle to far memory.
*
*Entry:
*   wHandle     Handle to far memory.
*   cb      Number of bytes to reallocate to.
*   flags       Unused parameter.
*
*Exit:
*   Returns reallocated handle if successful, NULL if not.
*
*Exceptions:
*   None.
*******************************************************************************/
HDEP FAR PASCAL
MMhReallocMb (
HDEP    hmem,
unsigned long   cb,
unsigned short    flags )
{
unsigned short     cbMove;
unsigned char FAR *     lpDst = NULL;
unsigned char FAR *     lpSrc = NULL;

PMB             pmb = NULL;

    Unreferenced ( cbMove );
    Unreferenced ( flags );

    // Try near memory first.
    // only allow reallocations to be < 65520 bytes.


#if defined(DOS16)
    if ( cb <= 0xfff0 ) {

        if ( ( lpSrc = MMlpvHandleToLp ( hmem ) ) != NULL ) {
            lpDst = MMlpvAlloc ( MMDLLHEAP, (unsigned short) cb );

            if ( lpDst != NULL ) {

                pmb = PmbHmemToPmb ( hmem );
                assert ( pmb != NULL );
                cbMove = MMwHandleSize ( hmem ); // size of original handle

                _fmemcpy ( lpDst, lpSrc, min ( cbMove, (unsigned) cb ));

                MMFreeLpv ( lpSrc );

                pmb->lpvBlock = lpDst;
                pmb->cb = (unsigned short) cb;

            }
            else {
                pmb->lpvBlock = NULL;
                pmb->cb = 0;
                pmb->flags = MBUNDEFINED;
                hmem = MBUNDEFINED;
            }
        }
    }

#else
        pmb = PmbHmemToPmb ( hmem );
        assert ( pmb != NULL );
#ifdef HOST32
        assert ( _heapchk ( ) == _HEAPOK );
        lpSrc = realloc ( MMlpvHandleToLp ( hmem ), (unsigned short) cb );
#else
        assert ( FHEAPCHK( ) == _HEAPOK );
        lpSrc = _frealloc ( MMlpvHandleToLp ( hmem ), (unsigned short) cb );
#endif
        if ( lpSrc != NULL ) {
            pmb->lpvBlock = lpSrc;
            pmb->cb = (unsigned short) cb;
        }
        else {
            pmb->lpvBlock = NULL;
            pmb->cb = 0;
            pmb->flags = MBUNDEFINED;
        }
#endif
    return hmem;
}


/*** MHOutOfMemory - general purpose for CW out of memory conditions.
*
*
*************************************************************************/
void FAR PASCAL MMOutOfMemory()
{
    CVMessage ( ERRORMSG, ENOMEM, MSGBOX );
}

/*** MMcbGetLockFromMb
*
*   Purpose: To check to see if memory is locked
*
*   Input:
*   hMem    - A handle to the memory
*
*   Output:
*   Returns:
*       True if memory is locked
*
*   Exceptions:
*
*   Notes:
*
*************************************************************************/
unsigned short FAR PASCAL
MMcbGetLockFromMb (HDEP hmem )
{
PMB pmb;
    pmb = PmbHmemToPmb ( hmem );
    assert ( pmb != NULL );
    return (pmb->swLocked );
}



//
// Local Functions
//


#if defined(DOS16)
/*** bselAlloc - to allocate a block of memory via _bheapseg
*
*   Purpose:    to allocate memory blocks for dos3 static CW and help buffers.
*
*   Input:  cbAllocated, where to place the amount actually allocated.
*       cbHeapSize, amount to allocate
*
*   Output: cbAllocated, amount actually allocated
*   Returns:    selector of block allocated.
*
*   Exceptions:
*
*   Notes:
*
*************************************************************************/
LOCAL ushort PASCAL
bselAlloc ( ushort * cbAllocated, long cbHeapSize ) {

ushort  retval = _NULLSEG;
long    cbThisAlloc;

    if ( cbHeapSize > 0 ) {
        assert ( cbHeapSize <= ( (long)_HEAP_MAXREQ << 1 ) );
        if ( cbHeapSize > _HEAP_MAXREQ ) {
            cbHeapSize >= 1;
        }
        cbThisAlloc = cbHeapSize;
        while ( (retval = _bheapseg ( (unsigned short)cbThisAlloc )) == _NULLSEG ) {
            //
            // failed, decrease request down by cbFWORKGRAN until we
            //  hit 0
            //
            cbThisAlloc -= cbFWORKGRAN;
            if ( cbThisAlloc < 0 ) {
            cbThisAlloc = 0;
            break;
            }
        }
        *cbAllocated = (ushort) cbThisAlloc;
    }
    return retval;
}
#endif

/*** PmbHmemToPmb - return a pointer to the sb refered to by a handle
*
*   Input:  h,  sb handle
*
*   Returns:    pmb, pointer to the sb
*
*   Exceptions:
*
*   Notes:
*
*************************************************************************/
LOCAL PMB PASCAL NEAR
PmbHmemToPmb ( HDEP hmem )
{
register MBH            mbh;
register unsigned short j;
PMB                     pmbRet = NULL;

    if ( hmem != MBUNDEFINED ) {
        mbh.hmem = hmem;
        j = mbh.mbhi.mbNum;
        if ( j < MB_TABLE_MAX && pmbTable[ j ] != NULL ) {
            PMB pmb = pmbTable[ j ];
            j = mbh.mbhi.index;
            if ( pmb[ j ].flags != MBUNDEFINED && pmb[ j ].lpvBlock != NULL ) {
                pmbRet = &pmb[ j ];
            }
        }
    }
    return pmbRet;
}
