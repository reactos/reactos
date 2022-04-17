/*
 * PROJECT:         avicap32
 * FILE:            dll\win32\avicap32\avicap32.c
 * PURPOSE:         Main file
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <stdio.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winver.h>
#include <winnls.h>
#include <wingdi.h>
#include <winternl.h>
#include <vfw.h>
#include <wine/debug.h>

#define CAP_DESC_MAX 32

WINE_DEFAULT_DEBUG_CHANNEL(avicap32);


HINSTANCE hInstance;


/* INTRENAL FUNCTIONS **************************************************/

LRESULT
CALLBACK
CaptureWindowProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        case WM_CREATE:
            break;

        case WM_PAINT:
            break;

        case WM_DESTROY:
            break;
    }

    return DefWindowProc(hwnd, Msg, wParam, lParam);
}


/* FUNCTIONS ***********************************************************/

/*
 * implemented
 */
HWND
VFWAPI
capCreateCaptureWindowW(LPCWSTR lpszWindowName,
                        DWORD dwStyle,
                        INT x,
                        INT y,
                        INT nWidth,
                        INT nHeight,
                        HWND hWnd,
                        INT nID)
{
    WCHAR szWindowClass[] = L"ClsCapWin";
    WNDCLASSEXW WndClass = {0};
    DWORD dwExStyle = 0;

    FIXME("capCreateCaptureWindowW() not fully implemented!\n");

    WndClass.cbSize        = sizeof(WNDCLASSEXW);
    WndClass.lpszClassName = szWindowClass;
    WndClass.lpfnWndProc   = CaptureWindowProc; /* TODO: Implement CaptureWindowProc */
    WndClass.hInstance     = hInstance;
    WndClass.style         = CS_HREDRAW | CS_VREDRAW;
    WndClass.hCursor       = LoadCursorW(0, IDC_ARROW);
    WndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

    if (RegisterClassExW(&WndClass) == (ATOM)0)
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
            return NULL;
    }

    return CreateWindowExW(dwExStyle,
                           szWindowClass,
                           lpszWindowName,
                           dwStyle,
                           x, y,
                           nWidth,
                           nHeight,
                           hWnd,
                           ULongToHandle(nID),
                           hInstance,
                           NULL);
}

/*
 * implemented
 */
HWND
VFWAPI
capCreateCaptureWindowA(LPCSTR lpszWindowName,
                        DWORD dwStyle,
                        INT x,
                        INT y,
                        INT nWidth,
                        INT nHeight,
                        HWND hWnd,
                        INT nID)
{
    UNICODE_STRING Name;
    HWND Wnd;

    if (lpszWindowName)
        RtlCreateUnicodeStringFromAsciiz(&Name, lpszWindowName);
    else
        Name.Buffer = NULL;

    Wnd = capCreateCaptureWindowW(Name.Buffer,
                                  dwStyle,
                                  x, y,
                                  nWidth,
                                  nHeight,
                                  hWnd,
                                  nID);

    RtlFreeUnicodeString(&Name);
    return Wnd;
}


/*
 * implemented
 */
BOOL
VFWAPI
capGetDriverDescriptionW(WORD wDriverIndex,
                         LPWSTR lpszName,
                         INT cbName,
                         LPWSTR lpszVer,
                         INT cbVer)
{
    DWORD dwSize, dwIndex = 0;
    WCHAR szDriver[MAX_PATH];
    WCHAR szDriverName[MAX_PATH];
    WCHAR szFileName[MAX_PATH];
    WCHAR szVersion[MAX_PATH];
    HKEY hKey, hSubKey;

    /* TODO: Add support of data acquisition from system.ini */
    FIXME("capGetDriverDescriptionW() not fully implemented!\n");

    if (lpszName && cbName)
        lpszName[0] = L'\0';

    if (lpszVer && cbVer)
        lpszVer[0] = L'\0';

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SYSTEM\\CurrentControlSet\\Control\\MediaResources\\msvideo",
                      0,
                      KEY_READ,
                      &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    dwSize = sizeof(szDriver) / sizeof(WCHAR);

    while (RegEnumKeyExW(hKey, dwIndex,
                         szDriver, &dwSize,
                         NULL, NULL,
                         NULL, NULL) == ERROR_SUCCESS)
    {
        if (RegOpenKeyW(hKey, szDriver, &hSubKey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(szFileName);

            if (RegQueryValueExW(hSubKey,
                                 L"Driver",
                                 NULL,
                                 NULL,
                                 (LPBYTE)&szFileName,
                                 &dwSize) == ERROR_SUCCESS)
            {
                dwSize = sizeof(szDriverName);

                if (RegQueryValueExW(hSubKey,
                                     L"FriendlyName",
                                     NULL,
                                     NULL,
                                     (LPBYTE)&szDriverName,
                                     &dwSize) != ERROR_SUCCESS)
                {
                    wcscpy(szDriverName, L"Unknown Driver Name");
                }

                if (dwIndex == (DWORD)wDriverIndex)
                {
                    if (lpszName && cbName)
                    {
                        lstrcpynW(lpszName, szDriverName, cbName);
                    }

                    if (lpszVer && cbVer)
                    {
                        LPVOID Version, Ms;
                        DWORD dwInfoSize;
                        VS_FIXEDFILEINFO FileInfo;
                        UINT Ls;

                        dwInfoSize = GetFileVersionInfoSize(szFileName, NULL);
                        if (dwInfoSize)
                        {
                            Version = HeapAlloc(GetProcessHeap(), 0, dwInfoSize);

                            if (Version != NULL)
                            {
                                GetFileVersionInfo(szFileName, 0, dwInfoSize, Version);

                                if (VerQueryValueW(Version, L"\\", &Ms, &Ls))
                                {
                                    memmove(&FileInfo, Ms, Ls);
                                    swprintf(szVersion, L"Version: %d.%d.%d.%d",
                                             HIWORD(FileInfo.dwFileVersionMS),
                                             LOWORD(FileInfo.dwFileVersionMS),
                                             HIWORD(FileInfo.dwFileVersionLS),
                                             LOWORD(FileInfo.dwFileVersionLS));

                                    lstrcpynW(lpszVer, szVersion, cbVer);
                                }
                                HeapFree(GetProcessHeap(), 0, Version);
                            }
                        }
                    }

                    RegCloseKey(hSubKey);
                    RegCloseKey(hKey);
                    return TRUE;
                }
            }

            RegCloseKey(hSubKey);
        }

        dwSize = sizeof(szDriver) / sizeof(WCHAR);
        dwIndex++;
    }

    RegCloseKey(hKey);

    return FALSE;
}


/*
 * implemented
 */
BOOL
VFWAPI
capGetDriverDescriptionA(WORD wDriverIndex,
                         LPSTR lpszName,
                         INT cbName,
                         LPSTR lpszVer,
                         INT cbVer)
{
    WCHAR DevName[CAP_DESC_MAX], DevVer[CAP_DESC_MAX];
    BOOL Result;

    Result = capGetDriverDescriptionW(wDriverIndex, DevName, CAP_DESC_MAX, DevVer, CAP_DESC_MAX);
    if (Result)
    {
        WideCharToMultiByte(CP_ACP, 0, DevName, -1, lpszName, cbName, NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, DevVer, -1, lpszVer, cbVer, NULL, NULL);
    }

    return Result;
}


/*
 * unimplemented
 */
VOID
VFWAPI
AppCleanup(HINSTANCE hInst)
{
    UNIMPLEMENTED;
}


/*
 * unimplemented
 */
DWORD
VFWAPI
videoThunk32(DWORD dwUnknown1, DWORD dwUnknown2, DWORD dwUnknown3, DWORD dwUnknown4, DWORD dwUnknown5)
{
    UNIMPLEMENTED;
    return 0;
}


BOOL
WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            TRACE("avicap32 attached!\n");
            hInstance = hinstDLL;
            break;
    }

    return TRUE;
}
