/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User API Server DLL
 * FILE:            win32ss/user/winsrv/init.c
 * PURPOSE:         Initialization
 * PROGRAMMERS:     Dmitry Philippov (shedon@mail.ru)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

/* PSDK Headers */
#include <stdarg.h>
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <windef.h>
#include <winuser.h>

#define NDEBUG
#include <debug.h>


/* ENTRY-POINT ****************************************************************/

/*** HACK from win32csr... ***/
static HHOOK hhk = NULL;

LRESULT
CALLBACK
KeyboardHookProc(int nCode,
                 WPARAM wParam,
                 LPARAM lParam)
{
    DPRINT1("KeyboardHookProc Processing!\n");
    return CallNextHookEx(hhk, nCode, wParam, lParam);
}
/*** END - HACK from win32csr... ***/

BOOL
WINAPI
DllMain(IN HINSTANCE hInstanceDll,
        IN DWORD dwReason,
        IN LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(hInstanceDll);
    UNREFERENCED_PARAMETER(dwReason);
    UNREFERENCED_PARAMETER(lpReserved);

    if (DLL_PROCESS_ATTACH == dwReason)
    {
        DPRINT1("WINSRV - HACK: Use keyboard hook hack\n");
/*** HACK from win32csr... ***/
//
// HACK HACK HACK ReactOS to BOOT! Initialization BUG ALERT! See bug 5655.
//
        hhk = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, NULL, 0);
// BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT!
//  BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT!
//   BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT!

/*** END - HACK from win32csr... ***/
    }

    return TRUE;
}

/* EOF */
