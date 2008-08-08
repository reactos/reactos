/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/menu.c
 * PURPOSE:         Menu Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
APIENTRY
NtUserCheckMenuItem(HMENU hmenu,
                    UINT uIDCheckItem,
                    UINT uCheck)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserDeleteMenu(HMENU hMenu,
                 UINT uPosition,
                 UINT uFlags)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserDestroyMenu(HMENU hMenu)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserDrawMenuBarTemp(HWND hWnd,
                      HDC hDC,
                      PRECT hRect,
                      HMENU hMenu,
                      HFONT hFont)
{
    UNIMPLEMENTED;
    return 0;
}

UINT
APIENTRY
NtUserEnableMenuItem(HMENU hMenu,
                     UINT uIDEnableItem,
                     UINT uEnable)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserEndMenu(VOID)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserGetMenuBarInfo(HWND hwnd,
                     LONG idObject,
                     LONG idItem,
                     PMENUBARINFO pmbi)
{
    UNIMPLEMENTED;
    return FALSE;
}

UINT
APIENTRY
NtUserGetMenuIndex(HMENU hMenu,
                   UINT wID)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserGetMenuItemRect(HWND hWnd,
                      HMENU hMenu,
                      UINT uItem,
                      LPRECT lprcItem)
{
    UNIMPLEMENTED;
    return FALSE;
}

HMENU
APIENTRY
NtUserGetSystemMenu(HWND hWnd,
                    BOOL bRevert)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserSetSystemMenu(HWND hWnd,
                   HMENU hMenu)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserHiliteMenuItem(HWND hwnd,
                     HMENU hmenu,
                     UINT uItemHilite,
                     UINT uHilite)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtUserMenuItemFromPoint(HWND hWnd,
                        HMENU hMenu,
                        DWORD X,
                        DWORD Y)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserRemoveMenu(HMENU hMenu,
                 UINT uPosition,
                 UINT uFlags)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserSetMenu(HWND hWnd,
              HMENU hMenu,
              BOOL bRepaint)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserSetMenuContextHelpId(HMENU hmenu,
                           DWORD dwContextHelpId)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserSetMenuDefaultItem(HMENU hMenu,
                         UINT uItem,
                         UINT fByPos)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserSetMenuFlagRtoL(HMENU hMenu)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserThunkedMenuInfo(HMENU hMenu,
                      LPCMENUINFO lpcmi)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserThunkedMenuItemInfo(HMENU hMenu,
                          UINT uItem,
                          BOOL fByPosition,
                          BOOL bInsert,
                          LPMENUITEMINFOW lpmii,
                          PUNICODE_STRING lpszCaption)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserTrackPopupMenuEx(HMENU hmenu,
                       UINT fuFlags,
                       INT x,
                       INT y,
                       HWND hwnd,
                       LPTPMPARAMS lptpm)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserCalcMenuBar(DWORD dwUnknown1,
                  DWORD dwUnknown2,
                  DWORD dwUnknown3,
                  DWORD dwUnknown4,
                  DWORD dwUnknown5)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserPaintMenuBar(DWORD dwUnknown1,
                   DWORD dwUnknown2,
                   DWORD dwUnknown3,
                   DWORD dwUnknown4,
                   DWORD dwUnknown5,
                   DWORD dwUnknown6)
{
    UNIMPLEMENTED;
    return 0;
}
