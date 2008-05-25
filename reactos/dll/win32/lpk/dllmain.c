/* 
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS 
 * PURPOSE:              LPK Library
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */

#include "ros_lpk.h"

LPK_LPEDITCONTROL_LIST LpkEditControl = {EditCreate,       EditIchToXY,  EditMouseToIch, EditCchInWidth,
                                         EditGetLineWidth, EditDrawText, EditHScroll,    EditMoveSelection,
                                         EditVerifyText,   EditNextWord, EditSetMenu,    EditProcessMenu,
                                         EditCreateCaret, EditAdjustCaret};

BOOL
WINAPI
DllMain (
    HANDLE  hDll,
    DWORD   dwReason,
    LPVOID  lpReserved)
{

    return TRUE;
}

BOOL
WINAPI
LpkDllInitialize (
    HANDLE  hDll,
    DWORD   dwReason,
    LPVOID  lpReserved)
{
    return DllMain(hDll,dwReason,lpReserved);
}
