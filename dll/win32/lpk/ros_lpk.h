/* 
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS 
 * PURPOSE:              LPK Library
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */
#define WIN32_NO_STATUS
#define NTOS_MODE_USER

#include <windows.h>
#include <ndk/ntndk.h>

/* FIXME USP10 api that does not have prototype in any include file */
VOID WINAPI LpkPresent();

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
DWORD WINAPI LpkExtTextOut(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6,DWORD x7,DWORD x8,DWORD x9);
DWORD WINAPI LpkGetCharacterPlacement(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6, DWORD x7);
DWORD WINAPI LpkGetTextExtentExPoint(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6,DWORD x7,DWORD x8,DWORD x9);
DWORD WINAPI LpkPSMTextOut(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6);
DWORD WINAPI LpkUseGDIWidthCache(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5);
DWORD WINAPI ftsWordBreak(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5);

