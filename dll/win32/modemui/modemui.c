/*
 * PROJECT:     	ReactOS Modem Properties
 * LICENSE:     	GPL - See COPYING in the top level directory
 * FILE:        	dll/win32/modemui/modemui.c
 * PURPOSE:     	Modem Properties
 * COPYRIGHT:   	Copyright Dmitry Chapyshev <lentind@yandex.ru>
 *
 */

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>

//#include "resource.h"

static HINSTANCE hDllInstance;

INT_PTR CALLBACK
ModemCplDlgProc(IN HWND hwndDlg,
		        IN UINT uMsg,
                IN WPARAM wParam,
                IN LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {

        }
		break;
    }

    return 0;
}

INT_PTR CALLBACK
ModemPropPagesProvider(IN HWND hwndDlg,
		               IN UINT uMsg,
                       IN WPARAM wParam,
                       IN LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {

        }
		break;
    }

    return 0;
}

BOOL WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hDllInstance = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}
