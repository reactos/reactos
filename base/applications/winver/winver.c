/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Version Program
 * FILE:            base/applications/winver/winver.c
 */

#include "winver_p.h"

HINSTANCE Winver_hInstance;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    INITCOMMONCONTROLSEX iccx;
    WINVER_OS_INFO OSInfo;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    Winver_hInstance = hInstance;

    /* Initialize common controls */
    iccx.dwSize = sizeof(iccx);
    iccx.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&iccx);

    if (!Winver_GetOSInfo(&OSInfo))
    {
        /* OS info is not available, display the default contents */
        StringCchCopyW(OSInfo.szName, _countof(OSInfo.szName), L"ReactOS");
        OSInfo.szCompatInfo[0] = UNICODE_NULL;
    }

    return ShellAboutW(NULL, OSInfo.szName, OSInfo.szCompatInfo, NULL);
}
