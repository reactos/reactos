/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            lib/cpl/desk/settings.c
 * PURPOSE:         Settings property page
 * 
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 */

#include <windows.h>
#include <commctrl.h>

#include "resource.h"

INT_PTR CALLBACK SettingsPageProc(HWND hwndDlg,
                                  UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            {
            } break;
        
        case WM_COMMAND:
            {
            } break;
    }
    
    return FALSE;
}

