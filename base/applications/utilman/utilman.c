/*
 * PROJECT:         ReactOS Utility Manager (Accessibility)
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Main dialog code file
 * COPYRIGHT:       Copyright 2019-2020 George Bișoc (george.bisoc@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

/* FUNCTIONS ******************************************************************/

/**
 * @wWinMain
 *
 * Application entry point.
 *
 * @param[in]   hInstance
 *     Application instance.
 *
 * @param[in]   hPrevInstance
 *     The previous instance of the application (not used).
 *
 * @param[in]   pCmdLine
 *     Pointer to a command line argument (in wide string -- not used).
 *
 * @param[in]   nCmdShow
 *     An integer served as a flag to note how the application will be shown (not used).
 *
 * @return
 *     Returns 0 to let the function terminating before it enters in the message loop.
 *
 */
INT WINAPI wWinMain(IN HINSTANCE hInstance,
                    IN HINSTANCE hPrevInstance,
                    IN LPWSTR pCmdLine,
                    IN INT nCmdShow)
{
    HMODULE hModule;
    WCHAR szFormat[MAX_BUFFER];
    WCHAR szFailLoad[MAX_BUFFER];
    WCHAR szTitle[MAX_BUFFER];
    EXECDLGROUTINE UManStartDlg;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    /* Load the main resources module of Utility Manager */
    hModule = LoadLibraryW(L"UManDlg.dll");
    if (!hModule)
    {
        LoadStringW(hInstance, IDS_FAIL_INIT, szFormat, _countof(szFormat));
        LoadStringW(hInstance, IDS_FAIL_INIT_TITLE, szTitle, _countof(szTitle));

        StringCchPrintfW(szFailLoad, _countof(szFailLoad), szFormat, GetLastError());
        MessageBoxW(GetDesktopWindow(), szFailLoad, szTitle, MB_ICONERROR | MB_OK);
        return -1;
    }

    /* Get the function address and launch Utility Manager */
    UManStartDlg = (EXECDLGROUTINE)GetProcAddress(hModule, "UManStartDlg");
    UManStartDlg();

    FreeLibrary(hModule);
    return 0;
}
