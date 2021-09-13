/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * PURPOSE:              LPK Library
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */

#ifndef _LPK_H
#define _LPK_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winnls.h>
#include <usp10.h>
#include <strsafe.h>
#include "undocgdi.h"
#include "wine/unicode.h"
#include "wine/debug.h"

/* FIXME USP10 api that does not have prototype in any include file */
VOID WINAPI LpkPresent(VOID);

/* FIXME move _LPK_LPEDITCONTROL_LIST to global place so user32 can access it */
typedef struct _LPK_LPEDITCONTROL_LIST
{
    PVOID EditCreate;
    PVOID EditIchToXY;
    PVOID EditMouseToIch;
    PVOID EditCchInWidth;
    PVOID EditGetLineWidth;
    PVOID EditDrawText;
    PVOID EditHScroll;
    PVOID EditMoveSelection;
    PVOID EditVerifyText;
    PVOID EditNextWord;
    PVOID EditSetMenu;
    PVOID EditProcessMenu;
    PVOID EditCreateCaret;
    PVOID EditAdjustCaret;
} LPK_LPEDITCONTROL_LIST, *PLPK_LPEDITCONTROL_LIST;

/* This List are exported */


DWORD WINAPI EditCreate( DWORD x1, DWORD x2);
DWORD WINAPI EditIchToXY( DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5);
DWORD WINAPI EditMouseToIch( DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5);
DWORD WINAPI EditCchInWidth( DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5);

DWORD WINAPI EditGetLineWidth( DWORD x1, DWORD x2, DWORD x3, DWORD  x4);
DWORD WINAPI EditDrawText( DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5, DWORD x6, DWORD x7);
DWORD WINAPI EditHScroll( DWORD x1, DWORD x2, DWORD x3);
DWORD WINAPI EditMoveSelection( DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5);

DWORD WINAPI EditVerifyText( DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5, DWORD x6);
DWORD WINAPI EditNextWord(DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5, DWORD x6, DWORD x7);
DWORD WINAPI EditSetMenu(DWORD x1, DWORD x2);
DWORD WINAPI EditProcessMenu(DWORD x1, DWORD x2);
DWORD WINAPI EditCreateCaret(DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5);
DWORD WINAPI EditAdjustCaret(DWORD x1, DWORD x2, DWORD x3, DWORD x5);

DWORD WINAPI LpkInitialize(DWORD x1);
DWORD WINAPI LpkTabbedTextOut(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6,DWORD x7,DWORD x8,DWORD x9,DWORD x10,DWORD x11,DWORD x12);
BOOL WINAPI LpkDllInitialize (HANDLE  hDll, DWORD dwReason, LPVOID lpReserved);
DWORD WINAPI LpkDrawTextEx(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6,DWORD x7,DWORD x8,DWORD x9, DWORD x10);
DWORD WINAPI LpkUseGDIWidthCache(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5);
DWORD WINAPI ftsWordBreak(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5);

/* Implemented */

BOOL WINAPI LpkExtTextOut(HDC hdc, int x, int y, UINT fuOptions, const RECT *lprc,
                          LPCWSTR lpString, UINT uCount , const INT *lpDx, INT unknown);

DWORD WINAPI LpkGetCharacterPlacement(HDC hdc, LPCWSTR lpString, INT uCount, INT nMaxExtent,
                                      GCP_RESULTSW *lpResults, DWORD dwFlags, DWORD dwUnused);

INT WINAPI LpkPSMTextOut(HDC hdc, int x, int y, LPCWSTR lpString, int cString, DWORD dwFlags);

BOOL WINAPI LpkGetTextExtentExPoint(HDC hdc, LPCWSTR lpString, INT cString, INT nMaxExtent,
                                    LPINT lpnFit, LPINT lpnDx, LPSIZE lpSize, DWORD dwUnused,
                                    int unknown);
/* bidi.c */

#define WINE_GCPW_FORCE_LTR 0
#define WINE_GCPW_FORCE_RTL 1
#define WINE_GCPW_LOOSE_LTR 2
#define WINE_GCPW_LOOSE_RTL 3
#define WINE_GCPW_DIR_MASK 3
#define WINE_GCPW_LOOSE_MASK 2

BOOL BIDI_Reorder(
    _In_ HDC hDC,                /* [in] Display DC */
    _In_ LPCWSTR lpString,       /* [in] The string for which information is to be returned */
    _In_ INT uCount,             /* [in] Number of WCHARs in string. */
    _In_ DWORD dwFlags,          /* [in] GetCharacterPlacement compatible flags specifying how to process the string */
    _In_ DWORD dwWineGCP_Flags,  /* [in] Wine internal flags - Force paragraph direction */
    _Out_ LPWSTR lpOutString,    /* [out] Reordered string */
    _In_ INT uCountOut,          /* [in] Size of output buffer */
    _Out_ UINT *lpOrder,         /* [out] Logical -> Visual order map */
    _Out_ WORD **lpGlyphs,       /* [out] reordered, mirrored, shaped glyphs to display */
    _Out_ INT *cGlyphs           /* [out] number of glyphs generated */
    );

#endif /* _LPK_H */
