/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for NtUserBuildHwndList
 * COPYRIGHT:   Copyright 2026 Max Korostil <mrmks04@yandex.ru>
 */

#include <apitest.h>
#include <wincon.h>

#include "../win32nt.h"

#define LIST_TERMINATOR ((HWND)1)

LRESULT CALLBACK WindowProc(HWND Hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(Hwnd, Msg, wParam, lParam);
}

DWORD WINAPI GuiThreadFunction(LPVOID lpParam)
{
    DWORD error;
    MSG msg;
    HINSTANCE hInstance = GetModuleHandleW(NULL);
    PCWSTR CLASS_NAME = L"GuiThreadWindowClass";    

    // Register the Window Class
    WNDCLASSW wc;
    RtlZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursorW(NULL, (PCWSTR)IDC_ARROW);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME,
                                L"Threaded GUI Window", 
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                NULL, NULL,
                                hInstance, NULL);
    
    error = GetLastError();
    ok(hwnd != NULL, "Create window is NULL\n");
    ok(error == ERROR_SUCCESS, "Error %u\n", error);
    
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
   
    if (hwnd)
        DestroyWindow(hwnd);
    
    return 0;
}

DWORD WINAPI ThreadFunction(LPVOID lpParam)
{
    PEVENT event = (PEVENT)lpParam;
    WaitForSingleObject(event, INFINITE);

    return 0;
}

START_TEST(NtUserBuildHwndList)
{
    HDESK hDesktop = NULL;
    HWND hWndparent = NULL;
    HANDLE hEvent;
    HANDLE hThread;
    DWORD newThreadId = 0;
    DWORD error = 0;
    DWORD dwCount = 0;
    HWND hwndArray[1024];
    NTSTATUS status;
    SYSTEM_INFO si;
             
    // 1 - Success
    SetLastError(0);
    status = NtUserBuildHwndList(hDesktop,
                                 hWndparent,
                                 FALSE,
                                 0,
                                 ARRAYSIZE(hwndArray),
                                 hwndArray,
                                 &dwCount);
    error = GetLastError();
    ok_int(status, STATUS_SUCCESS);
    ok_int(error, ERROR_SUCCESS);

    // 2 - HWND array NULL
    SetLastError(0);
    status = NtUserBuildHwndList(hDesktop,
                                 hWndparent,
                                 FALSE,
                                 0,
                                 ARRAYSIZE(hwndArray),
                                 NULL,
                                 &dwCount);
    error = GetLastError();
    ok_int(status, STATUS_INVALID_HANDLE);
    ok_int(error, ERROR_NOACCESS);

    // 3 - HWND array wrong address
    SetLastError(0);
    status = NtUserBuildHwndList(hDesktop,
                                 hWndparent,
                                 FALSE,
                                 0,
                                 ARRAYSIZE(hwndArray),
                                 (HWND*)(ULONG_PTR)0xDEADBEEF,
                                 &dwCount);
    error = GetLastError();
    ok_int(status, STATUS_INVALID_HANDLE);
    ok_int(error, ERROR_NOACCESS);

    // 4 - Count is NULL
    SetLastError(0);
    status = NtUserBuildHwndList(hDesktop,
                                 hWndparent,
                                 FALSE,
                                 0,
                                 ARRAYSIZE(hwndArray),
                                 hwndArray,
                                 NULL);
    error = GetLastError();
    ok_int(status, STATUS_INVALID_HANDLE);
    ok_int(error, ERROR_NOACCESS);

    // 5 - Count wrong address
    SetLastError(0);
    status = NtUserBuildHwndList(hDesktop,
                                 hWndparent,
                                 FALSE,
                                 0,
                                 ARRAYSIZE(hwndArray),
                                 hwndArray,
                                 (DWORD*)(ULONG_PTR)0xDEADBEEF);
    error = GetLastError();
    ok_int(status, STATUS_INVALID_HANDLE);
    ok_int(error, ERROR_NOACCESS);

    // 6 - Thread id invalid
    SetLastError(0);
    status = NtUserBuildHwndList(hDesktop,
                                 hWndparent,
                                 FALSE,
                                 0xFFFFFFFF,
                                 ARRAYSIZE(hwndArray),
                                 hwndArray,
                                 &dwCount);
    error = GetLastError();
    ok_int(status, STATUS_INVALID_HANDLE);
    ok_int(error, ERROR_INVALID_PARAMETER);

    // 7 - Invalid parent handle
    SetLastError(0);
    status = NtUserBuildHwndList(hDesktop,
                                 (HWND)(ULONG_PTR)1,
                                 FALSE,
                                 0,
                                 ARRAYSIZE(hwndArray),
                                 hwndArray,
                                 &dwCount);
    error = GetLastError();
    ok_int(status, STATUS_INVALID_HANDLE);
    ok_int(error, ERROR_INVALID_WINDOW_HANDLE);

    // 8 - Invalid desktop handle
    SetLastError(0);
    status = NtUserBuildHwndList((HDESK)(ULONG_PTR)1,
                                 hWndparent,
                                 FALSE,
                                 0,
                                 ARRAYSIZE(hwndArray),
                                 hwndArray,
                                 &dwCount);
    error = GetLastError();
    ok_int(status, STATUS_INVALID_HANDLE);
    ok_int(error, ERROR_INVALID_HANDLE);

    GetSystemInfo(&si);

    // 9 - HWND array wrong address
    SetLastError(0);
    status = NtUserBuildHwndList(hDesktop,
                                 hWndparent,
                                 FALSE,
                                 0,
                                 ARRAYSIZE(hwndArray),
                                 (HWND*)(ULONG_PTR)si.lpMaximumApplicationAddress + 1,
                                 &dwCount);
    error = GetLastError();
    ok_int(status, STATUS_INVALID_HANDLE);
    ok_int(error, ERROR_NOACCESS);

    // 10 - Buffer size too small
    SetLastError(0);
    status = NtUserBuildHwndList(hDesktop,
                                 hWndparent,
                                 FALSE,
                                 0,
                                 1,
                                 hwndArray,
                                 &dwCount);
    error = GetLastError();
    ok_int(status, STATUS_BUFFER_TOO_SMALL);
    ok_int(error, ERROR_SUCCESS);

    // 11 - Gui Thread with one window
    hThread = CreateThread(NULL,
                           0,
                           GuiThreadFunction,
                           NULL,
                           0,
                           &newThreadId);
    if (hThread)
    {
        Sleep(1000);

        SetLastError(0);
        dwCount = 0;
        status = NtUserBuildHwndList(hDesktop,
                                     hWndparent,
                                     FALSE,
                                     newThreadId,
                                     ARRAYSIZE(hwndArray),
                                     hwndArray,
                                     &dwCount);
        error = GetLastError();
        ok_int(status, STATUS_SUCCESS);
        ok_int(error, ERROR_SUCCESS);
        ok_int(dwCount, 2);
        ok(hwndArray[dwCount - 1] == LIST_TERMINATOR, "List should end with HWND = 1");

        PostThreadMessage(newThreadId, WM_QUIT, 0, 0);
        CloseHandle(hThread);
        hThread = NULL;
    }
    else
    {
        skip("Create gui thread failed\n");
    }

    // 12 - Thread without windows
    hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    hThread = CreateThread(NULL,
                           0,
                           ThreadFunction,
                           hEvent,
                           0,
                           &newThreadId);
    
    if (hThread && hEvent)
    {  
        Sleep(1000);

        SetLastError(0);
        status = NtUserBuildHwndList(hDesktop,
                                     hWndparent,
                                     FALSE,
                                     newThreadId,
                                     ARRAYSIZE(hwndArray),
                                     hwndArray,
                                     &dwCount);
        error = GetLastError();
        ok_int(status, STATUS_INVALID_HANDLE);
        ok_int(error, ERROR_INVALID_PARAMETER);
        SetEvent(hEvent);
    }
    else
    {
        skip("Create non gui thread failed\n");
    }
    
    if (hThread)
    {
        CloseHandle(hThread);        
        hThread = NULL;
    }
    if (hEvent)
        CloseHandle(hEvent);    
}
