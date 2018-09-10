/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests the undocumented user32.dll API SoftModalMessageBox()
 *              and the MB_SERVICE_NOTIFICATION flag of the MessageBox*() APIs.
 * COPYRIGHT:   Copyright 2018 Hermes Belusca-Maito
 */

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#include <windef.h>
#include <winbase.h>
#include <winuser.h>

#include "resource.h"

/* Adjust according to your platform! -- ReactOS is currently compatible with Windows Server 2003 */
#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WS03 // _WIN32_WINNT_WIN2K // _WIN32_WINNT_WS03 // _WIN32_WINNT_WIN7

typedef struct _MSGBOXDATA
{
    MSGBOXPARAMSW mbp;          // Size: 0x28 (x86), 0x50 (x64)
    HWND     hwndOwner;         // Will be converted to PWND
#if defined(_WIN32) && (_WIN32_WINNT >= _WIN32_WINNT_WIN7) /* (NTDDI_VERSION >= NTDDI_WIN7) */
    DWORD    dwPadding;
#endif
    WORD     wLanguageId;
    INT*     pidButton;         // Array of button IDs
    LPCWSTR* ppszButtonText;    // Array of button text strings
    DWORD    dwButtons;         // Number of buttons
    UINT     uDefButton;        // Default button ID
    UINT     uCancelId;         // Button ID corresponding to Cancel action
#if (_WIN32_WINNT >= _WIN32_WINNT_WINXP) /* (NTDDI_VERSION >= NTDDI_WINXP) */
    DWORD    dwTimeout;         // Message box timeout
#endif
    DWORD    dwReserved0;
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7) /* (NTDDI_VERSION >= NTDDI_WIN7) */
    DWORD    dwReserved[4];
#endif
} MSGBOXDATA, *PMSGBOXDATA, *LPMSGBOXDATA;


#if defined(_WIN64)

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7) /* (NTDDI_VERSION >= NTDDI_WIN7) */
C_ASSERT(sizeof(MSGBOXDATA) == 0x98);
#elif (_WIN32_WINNT <= _WIN32_WINNT_WS03) /* (NTDDI_VERSION == NTDDI_WS03) */
C_ASSERT(sizeof(MSGBOXDATA) == 0x88);
#endif

#else

#if (_WIN32_WINNT <= _WIN32_WINNT_WIN2K) /* (NTDDI_VERSION <= NTDDI_WIN2KSP4) */
C_ASSERT(sizeof(MSGBOXDATA) == 0x48);
#elif (_WIN32_WINNT >= _WIN32_WINNT_WIN7) /* (NTDDI_VERSION >= NTDDI_WIN7) */
C_ASSERT(sizeof(MSGBOXDATA) == 0x60);
#else // (_WIN32_WINNT == _WIN32_WINNT_WINXP || _WIN32_WINNT == _WIN32_WINNT_WS03) /* (NTDDI_VERSION == NTDDI_WS03) */
C_ASSERT(sizeof(MSGBOXDATA) == 0x4C);
#endif

#endif


//
// Example taken from http://rsdn.org/forum/winapi/3273168.1
// See also http://www.vbforums.com/showthread.php?840593-Message-Box-with-Four-Buttons
//
void TestSoftModalMsgBox(void)
{
    typedef int (WINAPI *SOFTMODALMESSAGEBOX)(LPMSGBOXDATA lpMsgBoxData);
    // int WINAPI SoftModalMessageBox(IN LPMSGBOXDATA lpMsgBoxData);
    SOFTMODALMESSAGEBOX SoftModalMessageBox = NULL;

    MSGBOXDATA data;
    int res = 0;

    INT pids[] =
    {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 /*, 2*/
    };
    /* NOTE: Buttons do NOT support resource IDs specifications! */
    LPCWSTR ppText[] =
    {
        L"Button 1", L"Button 2", L"Button 3", L"Button 4", L"Button 5", L"Button 6", L"Button 7", L"Button 8", L"Button 9", L"Button 10", L"Button 11", L"Button 12", L"Button 13"
    };

    ZeroMemory(&data, sizeof(data));
    data.mbp.cbSize = sizeof(data.mbp);
    data.mbp.hwndOwner = FindWindowW(L"Shell_TrayWnd", NULL);
    data.mbp.hInstance = GetModuleHandleW(NULL);
    data.mbp.lpszText  = L"This is a message box made using the undocumented SoftModalMessageBox() API.";
    data.mbp.lpszCaption = MAKEINTRESOURCEW(IDS_RES_CAPTION); // L"SoftModalMessageBox";
    data.mbp.lpfnMsgBoxCallback = NULL; // SoftModalMessageBoxCallback;
    data.mbp.dwStyle = MB_ICONINFORMATION | MB_RETRYCANCEL;

    data.wLanguageId = 0;
    data.pidButton      = pids;
    data.ppszButtonText = ppText;
    data.dwButtons  = _countof(pids);
    data.uDefButton = 2;
    data.uCancelId  = 0;
#if (_WIN32_WINNT >= _WIN32_WINNT_WINXP) /* (NTDDI_VERSION >= NTDDI_WINXP) */
    data.dwTimeout  = 3 * 1000;
#endif

    SoftModalMessageBox = (SOFTMODALMESSAGEBOX)GetProcAddress(GetModuleHandleW(L"user32.dll"), "SoftModalMessageBox");
    if (!SoftModalMessageBox)
    {
        printf("SoftModalMessageBoxW not found in user32.dll\n");
    }
    else
    {
        res = SoftModalMessageBox(&data);
        printf("Returned value = %i\n", res);
    }
}

void TestMsgBoxServiceNotification(void)
{
    int res;

    res = MessageBoxW(NULL, L"Hello World!", L"MB_SERVICE_NOTIFICATION",
                      MB_YESNOCANCEL | MB_DEFBUTTON3 | MB_ICONINFORMATION | /* MB_DEFAULT_DESKTOP_ONLY | */ MB_SERVICE_NOTIFICATION);
    printf("Returned value = %i\n", res);
}

int wmain(int argc, WCHAR* argv[])
{
    printf("Testing SoftModalMessageBox()...\n");
    TestSoftModalMsgBox();
    printf("\n");

    printf("Press any key to continue...\n");
    _getch();
    printf("\n");

    printf("Testing MB_SERVICE_NOTIFICATION...\n");
    TestMsgBoxServiceNotification();
    printf("\n");

    printf("Press any key to quit...\n");
    _getch();
    return 0;
}
