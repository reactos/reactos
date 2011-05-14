/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS uxtheme.dll
 * FILE:            dll/win32/uxtheme/themehooks.c
 * PURPOSE:         uxtheme non client area management
 * PROGRAMMER:      Giannis Adamopoulos
 */
 


#include "config.h"

#include <stdlib.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "vfwmsgs.h"
#include "uxtheme.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);


LRESULT CALLBACK 
ThemeWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, WNDPROC DefWndProc)
{
    UNIMPLEMENTED;

    /* Some test */
    if(Msg == WM_NCPAINT || Msg == WM_NCACTIVATE)
        return FALSE;

    return DefWndProc(hWnd, Msg, wParam, lParam);
}
