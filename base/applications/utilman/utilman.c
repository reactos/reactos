/*
 * PROJECT:         ReactOS Utility Manager (Accessibility)
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Main dialog code file
 * COPYRIGHT:       Copyright 2019-2020 Bișoc George (fraizeraust99 at gmail dot com)
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
    WCHAR wszFormat[MAX_BUFFER];
    WCHAR wszFailLoad[MAX_BUFFER];
    WCHAR wszTitle[MAX_BUFFER];
    EXECDLGROUTINE UManStartDlg;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    /* Load the main resources module of Utility Manager */
    hModule = LoadLibraryW(L"UManDlg.dll");
    if (!hModule)
    {
        LoadStringW(hInstance, IDS_FAIL_INIT, wszFormat, _countof(wszFormat));
        LoadStringW(hInstance, IDS_FAIL_INIT_TITLE, wszTitle, _countof(wszTitle));

        StringCchPrintfW(wszFailLoad, _countof(wszFailLoad), wszFormat, GetLastError());
        MessageBoxW(GetDesktopWindow(), wszFailLoad, wszTitle, MB_ICONERROR | MB_OK);
        return -1;
    }

    /* Get the function address and launch Utility Manager */
    UManStartDlg = (EXECDLGROUTINE)GetProcAddress(hModule, "UManStartDlg");
    UManStartDlg();

    FreeLibrary(hModule);
    return 0;
}
