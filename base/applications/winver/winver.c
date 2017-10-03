/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Version Program
 * FILE:            base/applications/winver/winver.c
 */

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <shellapi.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    return ShellAboutW(NULL, L"ReactOS", NULL, NULL);
}
