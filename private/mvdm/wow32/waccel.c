//*****************************************************************************
//
// LoadAccelerator - compatibility support.
//     So much code for such a silly thing.
//
// 23-Jul-92  NanduriR   Created.
//*****************************************************************************

#include "precomp.h"
#pragma hdrstop

MODNAME(waccel.c);

extern ULONG SetCursorIconFlag(HAND16 hAccel16, BOOL bFlag);

LPACCELALIAS lpAccelAlias = NULL;


//*****************************************************************************
// WU32LoadAccelerators -
//   This gets called from WU32NotifyWow. I use the familiar name WU32...
//   because this gets called indirectly in response to LoadAccelerator
//
//   returs TRUE for success.
//*****************************************************************************


ULONG FASTCALL WU32LoadAccelerators(VPVOID vpData)
{
    PLOADACCEL16 ploadaccel16;
    HACCEL  hAccel;
    BOOL    fReturn = (BOOL)FALSE;


    GETVDMPTR(vpData, sizeof(LOADACCEL16), ploadaccel16);

    if (FindAccelAlias((HANDLE)FETCHWORD(ploadaccel16->hAccel),
                                                              HANDLE_16BIT)) {
        LOGDEBUG(0, ("AccelAlias already exists\n"));
        return (ULONG)TRUE;
    }

    if (hAccel = CreateAccel32(ploadaccel16->pAccel, ploadaccel16->cbAccel)) {
        fReturn =  (BOOL)SetupAccelAlias(FETCHWORD(ploadaccel16->hInst),
                                         FETCHWORD(ploadaccel16->hAccel),
                                         hAccel, TRUE);
    }


    FREEVDMPTR(ploadaccel16);
    return (ULONG)fReturn;
}


//*****************************************************************************
// SetupAccelAlias -
//    sets up the alias. the alias list is doubly linked. nothing fancy.
//
//    returns pointer to the alias.
//*****************************************************************************

LPACCELALIAS SetupAccelAlias(
    HAND16 hInstance,
    HAND16 hAccel16,
    HAND32 hAccel32,
    BOOL   f16
) {
    LPACCELALIAS lpT;
    WORD         hTask16;

    hTask16 = CURRENTPTD()->htask16;
    lpT = FindAccelAlias((HANDLE)hAccel16, HANDLE_16BIT);
    if (lpT == (LPACCELALIAS)NULL) {
        lpT = malloc_w_small(sizeof(ACCELALIAS));
        if (lpT) {
            lpT->lpNext = lpAccelAlias;
            lpT->lpPrev = (LPACCELALIAS)NULL;

            if (lpAccelAlias)
                lpAccelAlias->lpPrev = lpT;

            lpAccelAlias = lpT;
        }
    }
    else {
        LOGDEBUG(0, ("SetupAccelAlias: Alias Already exists. how & why?\n"));
        WOW32ASSERT(FALSE);
    }

    if (lpT) {
        lpT->hInst   = hInstance;
        lpT->hTask16 = CURRENTPTD()->htask16;
        lpT->h16     = hAccel16;
        lpT->h32     = hAccel32;
        lpT->f16     = (WORD)f16;

        // mark this so we can remove it from the alias list when
        // FreeResource() (in user.exe) calls GlobalFree() (in krnl386)
        SetCursorIconFlag(hAccel16, TRUE);
    }
    else {
        WOW32ASSERT(FALSE);
    }


    return (LPACCELALIAS)lpT;
}


//*****************************************************************************
// DestroyAccelAlias -
//    Deletes the 32bit table and Frees the memory
//
//    returns TRUE for success
//*****************************************************************************

BOOL  DestroyAccelAlias(WORD hTask16)
{
    WORD hCurTask16;
    LPACCELALIAS lpT;
    LPACCELALIAS lpTFree;

    hCurTask16 = CURRENTPTD()->htask16;
    lpT = lpAccelAlias;
    while (lpT) {
         if (lpT->hTask16 == hCurTask16) {
             if (lpT->lpPrev)
                 lpT->lpPrev->lpNext = lpT->lpNext;

             if (lpT->lpNext)
                 lpT->lpNext->lpPrev = lpT->lpPrev;

             if ( lpT->f16 ) {
                 DestroyAcceleratorTable(lpT->h32);
             } else {
                 // this function - DestroyAccelAlias- gets called during
                 // taskexit time and the 16bit task cleanup code has already
                 // freed this memory handle. so this callback is not needed.
                 //                                                - nanduri
                 // WOWGlobalFree16( lpT->h16 );
             }

             lpTFree = lpT;
             lpT = lpT->lpNext;
             if (lpTFree == lpAccelAlias)
                 lpAccelAlias = lpT;

             free_w_small(lpTFree);
         }
         else
             lpT = lpT->lpNext;
    }


    return TRUE;
}


//*****************************************************************************
// FindAccelAlias -
//    maps 16 bit handle to 32bit handle and vice versa
//
//    returns TRUE for success
//*****************************************************************************

LPACCELALIAS FindAccelAlias(HANDLE hAccel, UINT fSize)
{
    WORD hCurTask16;
    LPACCELALIAS lpT;

    hCurTask16 = CURRENTPTD()->htask16;
    lpT = lpAccelAlias;
    while (lpT) {
        if (lpT->hTask16 == hCurTask16) {
            if (fSize & HANDLE_16BIT) {
                if (lpT->h16 == (HAND16)hAccel)
                    return lpT;
            }
            else {
                if (lpT->h32 == (HAND32)hAccel)
                    return lpT;
            }
        }

        lpT = lpT->lpNext;
    }

    return NULL;
}


//*****************************************************************************
// GetAccelHandle32 -
//    Returns h32, given h16.
//
//*****************************************************************************

HAND32 GetAccelHandle32(HAND16 h16)
{
  LPACCELALIAS lpT;

  if (!(lpT = FindAccelAlias((HANDLE)(h16), HANDLE_16BIT))) {
      DWORD cbAccel16;
      VPVOID vpAccel16;
      HACCEL hAccel;

      if (vpAccel16 = RealLockResource16(h16, &cbAccel16)) {
          if (hAccel = CreateAccel32(vpAccel16, cbAccel16)) {
              lpT = SetupAccelAlias(CURRENTPTD()->hInst16,  h16,  hAccel, TRUE);
          }
          GlobalUnlock16(h16);
      }
  }
  return  (lpT) ? lpT->h32 : (HAND32)NULL;

}

//*****************************************************************************
// GetAccelHandle16 -
//    Returns h16, given h32.
//
//*****************************************************************************

HAND16 GetAccelHandle16(HAND32 h32)
{
    LPACCELALIAS lpT;
    HAND16  hAccel16;

    if (!(lpT = FindAccelAlias((HANDLE)(h32), HANDLE_32BIT))) {
        //
        // There isn't a corresponding 16-bit accelerator table handle already
        // so create one.
        //
        if ( (hAccel16 = CreateAccel16(h32)) != 0 ) {
            lpT = SetupAccelAlias(CURRENTPTD()->hInst16, hAccel16, h32, FALSE );
        }
    }

    return  (lpT) ? lpT->h16 : (HAND16)NULL;
}

//*****************************************************************************
// CreateAccel32 -
//   This gets called from WU32NotifyWow.
//
//   returs TRUE for success.
//*****************************************************************************


HACCEL CreateAccel32(VPVOID vpAccel16, DWORD cbAccel16)
{
    PSZ          pAccel16;
    DWORD        nElem16;

    LPACCEL lpAccel;
    HACCEL  hAccel = (HACCEL)NULL;
    UINT    i;
#if DBG
    UINT    LastKeyIndex = 0xffffffff;
#endif

    //
    // pAccel16 is pointer to an array of records of length:
    //    (BYTE+WORD+WORD)
    //

    GETVDMPTR(vpAccel16 , cbAccel16, pAccel16);
    if (pAccel16) {

        //
        // convert the 16bit accel table to 32bit format and create it.
        //

        nElem16 = cbAccel16 / (sizeof(BYTE) + 2 * sizeof(WORD));
        lpAccel = (LPACCEL)malloc_w(nElem16 * sizeof(ACCEL));
        if (lpAccel) {
            for (i=0; i<nElem16; ++i) {
                 lpAccel[i].fVirt = *(LPBYTE)(pAccel16);
#if DBG
                 if ((lpAccel[i].fVirt & 0x80) && i < LastKeyIndex) {
                    LastKeyIndex = i;
                 }
#endif
                 ((LPBYTE)pAccel16)++;
                 lpAccel[i].key   = FETCHWORD(*(LPWORD)pAccel16);
                 ((LPWORD)pAccel16)++;
                 lpAccel[i].cmd   = FETCHWORD(*(LPWORD)pAccel16);
                 ((LPWORD)pAccel16)++;
            }

#if DBG
            if (LastKeyIndex == 0xffffffff) {
                LOGDEBUG(LOG_ALWAYS, ("WOW::CreateAccel32 : no LastKey found in 16-bit haccel\n"));
            } else if (LastKeyIndex < nElem16-1) {
                LOGDEBUG(LOG_ALWAYS, ("WOW::CreateAccel32 : bogus LastKey flags ignored in 16-bit haccel\n"));
            }
#endif
            hAccel = CreateAcceleratorTable(lpAccel, i);
            free_w(lpAccel);
        }
        FREEVDMPTR(pAccel16);
    }

    return hAccel;
}

//*****************************************************************************
// CreateAccel16 -
//   This gets called from WU32NotifyWow.
//
//   returns HACCEL16 for success.
//*****************************************************************************

HAND16 CreateAccel16(HACCEL hAccel32)
{
    UINT    iEntries;
    UINT    cbSize;
    LPACCEL lpAccel32;
    HAND16  hAccel16;
    VPVOID  vpAccel16;
    LPBYTE  lpAccel16;
    LPBYTE  lpAccel16Original;
    UINT    i;

    iEntries = CopyAcceleratorTable( hAccel32, NULL, 0 );

    if ( iEntries == 0 ) {      // Invalid hAccel32
        return( 0 );
    }

    lpAccel32 = (LPACCEL)malloc_w(iEntries * sizeof(ACCEL));
    if ( lpAccel32 == NULL ) {
        LOGDEBUG(LOG_ERROR, ("WOW::CreateAccel16 : Failed to alloc memory for 32-bit accel\n"));
        return( 0 );
    }

    iEntries = CopyAcceleratorTable( hAccel32, lpAccel32, iEntries );

    cbSize = iEntries * (sizeof(BYTE) + 2 * sizeof(WORD));

    vpAccel16 = GlobalAllocLock16( GMEM_MOVEABLE, cbSize, &hAccel16 );

    if ( vpAccel16 == 0 ) {     // Out of 16-bit memory
        LOGDEBUG(LOG_ERROR, ("WOW::CreateAccel16 : Failed to alloc memory for 16-bit haccel\n"));
        free_w( lpAccel32 );
        return( 0 );
    }

    GETVDMPTR(vpAccel16, cbSize, lpAccel16 );

    WOW32ASSERT( lpAccel16 != NULL );

    lpAccel16Original = lpAccel16;

    //
    // Now iterate through the entries changing them and moving them into
    // the 16-bit memory.
    //

    i = 0;

    while ( i < iEntries ) {
        if ( i == iEntries-1 ) {
            // Last one, set the last bit
            *lpAccel16++ = lpAccel32[i].fVirt | 0x80;
        } else {
            *lpAccel16++ = lpAccel32[i].fVirt;
        }
        *((PWORD16)lpAccel16) = lpAccel32[i].key;
        lpAccel16 += sizeof(WORD);
        *((PWORD16)lpAccel16) = lpAccel32[i].cmd;
        lpAccel16 += sizeof(WORD);

        i++;
    }

    FLUSHVDMPTR(vpAccel16, cbSize, lpAccel16Original);
    FREEVDMPTR(lpAccel16Original);

    GlobalUnlock16( hAccel16 );

    return( hAccel16 );
}



// this gets called indirectly from GlobalFree() in krnl386.exe
// via WK32WowCursorIconOp() in wcuricon.c
void FreeAccelAliasEntry(LPACCELALIAS lpT) {

    if (lpT == lpAccelAlias)
        lpAccelAlias = lpT->lpNext;

    if (lpT->lpPrev)
        lpT->lpPrev->lpNext = lpT->lpNext;

    if (lpT->lpNext)
        lpT->lpNext->lpPrev = lpT->lpPrev;

    if ( lpT->f16 ) {
        DestroyAcceleratorTable(lpT->h32);
    } else {
         // this function - FreeAccelAliasEntry -- is being called
         // indirectly from GlobalFree() in krnl386.  GlobalFree()
         // takes care of freeing h16 so this callback is not needed.
         //                                                - a-craigj
         // WOWGlobalFree16( lpT->h16 );
    }

    free_w_small(lpT);
}

