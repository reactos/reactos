/*** bm.c - Memory Manager routines
*
*   Copyright <C> 1990, Microsoft Corporation
*
* Purpose:  handle memory requests of dlls & linked list
*
*
*************************************************************************/

#include <windows.h>
#include <stdlib.h>
#include <assert.h>

#include "d.h"
#include "cvtypes.h"
#include "cvwin32.h"
#include "cvwmem.h"


#define cablMax    0x100
#define clpvMax    0x100
#define BLOCKMASK  0xFF00
#define ITEMMASK   0x00FF
#define BLOCKSHIFT 8

typedef LPV FAR *ABL; // Allocation BLock

static HMEM hvCur = (HMEM) 1;
static ABL FAR *rgabl = NULL;

#ifdef DEBUGVER
typedef BYTE FAR *LCKB; // LoCK Block

static LCKB FAR *rglckb = NULL;
#endif

#ifndef hmemNull
#define hmemNull ( (HMEM) NULL )
#endif // !hmemNull

#define IablFromHmem(hv) ( ((UINT)hv) >> BLOCKSHIFT)
#define IlpvFromHmem(hv) ( ((UINT)hv) &  ITEMMASK)
#define HmemFromIablIlpv(iabl,ilpv) ((HMEM) (( iabl << BLOCKSHIFT ) | ilpv ))
#define LpvFromHmem(hv) *( *( rgabl + IablFromHmem ( hv ) ) + IlpvFromHmem ( hv ))
#define LpvFromIablIlpv(iabl,ilpv) *( *( rgabl + iabl ) + ilpv )

#ifdef DEBUGVER
#define LckFromHmem(hv) *( *( rglckb + IablFromHmem ( hv ) ) + IlpvFromHmem ( hv ))
#endif

HMEM PASCAL BMFindAvail ( void );
BOOL PASCAL BMNewBlock ( int );

// Public functions

BOOL PASCAL BMInit ( VOID ) {

#ifdef BMHANDLES // {

    if ( ( rgabl = (ABL FAR *) _fmalloc ( sizeof ( ABL ) * cablMax ) ) == NULL ) {
        return FALSE;
    }
    _fmemset ( rgabl, 0, sizeof ( ABL ) * cablMax );

#ifdef DEBUGVER
    if ( ( rglckb = (LCKB FAR *) _fmalloc ( sizeof ( LCKB ) * cablMax ) ) == NULL ) {
        return FALSE;
    }
    _fmemset ( rglckb, 0, sizeof ( LCKB ) * cablMax );
#endif

    // CAV 6583 -- must reset hvCur each time we re-init
    // (the IDE calls BMInit() each time it starts a debug session, codeview
    // only calls BMInit() once so hvCur doesn't need to be reset...

    hvCur = (HMEM) 1;

    CVInitCritSection(icsBm);

    return BMNewBlock ( 0 );

#else // } else !BMHANDLES {

    // We're going to return pointers directly, rather than having our own
    // handles.  So we need to make sure that pointers and handles are the
    // same size.
    assert ( sizeof(HMEM) == sizeof(VOID FAR *) );

    return TRUE;

#endif // }

}

HMEM PASCAL  BMAlloc ( UINT cb ) {

#ifdef BMHANDLES // {

    HMEM hv;

    CVEnterCritSection(icsBm);

    hv  = BMFindAvail ( );

    if ( hv ) {
        LPV lpv = _fmalloc ( cb );

        if ( LPV == NULL ) {
            hv = hmemNull;
        }
        else {
            LpvFromHmem ( hv ) = lpv;
        }
    }

    CVLeaveCritSection(icsBm);

    return hv;

#else // } else !BMHANDLES {

    return (HMEM) _fmalloc(cb);

#endif // }

}

HMEM PASCAL  BMRealloc ( HMEM hv, UINT cb ) {

#ifdef BMHANDLES // {

    LPV  lpv;
    UINT cbOld;

    CVEnterCritSection(icsBm);

    lpv = LpvFromHmem ( hv );
    assert ( LPV );
    cbOld = _fmsize ( LPV );

    if ( cbOld != cb ) {
        LPV lpvNew = _fmalloc ( cb );

    if ( LPV && lpvNew) {
            _fmemcpy ( lpvNew, lpv, min ( cb, cbOld ) );
            _ffree ( LPV );
            LpvFromHmem ( hv ) = lpvNew;
        }
        else {
            hv = hmemNull;
        }
    }

    CVLeaveCritSection(icsBm);

    return hv;

#else // } else !BMHANDLES {

    return (HMEM) _frealloc((VOID FAR *)hv, cb);

#endif // }

}

VOID PASCAL  BMFree ( HMEM hv ) {

#ifdef BMHANDLES // {

    CVEnterCritSection(icsBm);

    if ( hv < hvCur ) {
        hvCur = hv;
    }

    _ffree ( LpvFromHmem ( hv ) );
    LpvFromHmem ( hv ) = NULL;

#ifdef DEBUGVER
    LckFromHmem ( hv ) = 0;
#endif

    CVLeaveCritSection(icsBm);

#else // } else !BMHANDLES {

    _ffree((VOID FAR *)hv);

#endif // }

}

LPV  PASCAL  BMLock ( HMEM hv ) {

#ifdef BMHANDLES // {

    LPV lpv;

    if ( hv ) {
#ifdef DEBUGVER
        LckFromHmem ( hv ) += 1;

        assert ( LckFromHmem ( hv ) > 0 );  // check for overflow
#endif
        LPV = LpvFromHmem( hv );
    }
    else {
        LPV = NULL;
    }

    return lpv;

#else // } else !BMHANDLES {

    return (VOID FAR *) hv;

#endif // }

}

VOID PASCAL  BMUnlock ( HMEM hv ) {

#if defined(BMHANDLES) && defined(DEBUGVER)
    if ( hv ) {
        assert ( LckFromHmem ( hv ) > 0 );

        LckFromHmem ( hv ) -= 1;
    }
#endif

}

BOOL PASCAL  BMIsLocked ( HMEM hv ) {

#if defined(BMHANDLES) && defined(DEBUGVER)
    return !!(LckFromHmem(hv));
#else
    return FALSE;
#endif
}

// Private functions

#ifdef BMHANDLES // {

HMEM PASCAL BMFindAvail ( void ) {
    WORD iabl = IablFromHmem ( hvCur );
    WORD ilpv = IlpvFromHmem ( hvCur );
    HMEM hv   = hmemNull;
    BOOL fFail= FALSE;

    // We were out of memory, and nobody has freed anything.

    if ( hvCur == hmemNull ) {
        return hmemNull;
    }

    while ( TRUE ) {
        // Loop through the current allocation block to find and
        //  empty slot - Note that hvCur always represents the
        //  least possible open block, so we don't have to backtrack

        while ( ilpv < clpvMax && LpvFromIablIlpv ( iabl, ilpv ) ) {
            ilpv += 1;
        }

        // If we ran off the end of the current block

        if ( ilpv == clpvMax ) {
            iabl += 1;

            if ( iabl == cablMax ) {
                fFail = TRUE;
                break;
            }
            else if ( *( rgabl + iabl ) == NULL ) {
                if ( !BMNewBlock ( iabl ) ) {
                    fFail = TRUE;
                    break;
                }
            }

            ilpv = 0;
        }
        else {
            break;
        }

    };

    if ( !fFail ) {
        hv = HmemFromIablIlpv ( iabl, ilpv );
    }

    hvCur = hv;
    return hv;
}

BOOL PASCAL BMNewBlock ( int iabl ) {
    assert ( iabl < cablMax );
    assert ( *( rgabl + iabl ) == NULL );

    if ( ( *( rgabl + iabl ) = _fmalloc ( clpvMax * sizeof ( LPV ) ) ) == NULL ) {
        return FALSE;
    }
    _fmemset ( *( rgabl + iabl ), 0, clpvMax * sizeof ( LPV ) );

#ifdef DEBUGVER
    if ( ( *( rglckb + iabl ) = _fmalloc ( clpvMax * sizeof ( BYTE ) ) ) == NULL ) {
        return FALSE;
    }
    _fmemset ( *( rglckb + iabl ), 0, clpvMax * sizeof ( BYTE ) );
#endif

    return TRUE;
}

#endif // } BMHANDLES
