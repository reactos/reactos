/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for USER handles
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "../win32nt.h"

ULONG gulWinVersion;

static
HANDLE
CreateWindowHandle()
{
    return CreateWindowExW(0, L"STATIC", L"Test", WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);
}

ULONG gDummyCursorBits;

static LRESULT CALLBACK
CallWndProcHook(
    _In_ int    nCode,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

static
HANDLE
CreateHandle(ULONG Type)
{
    switch (Type)
    {
    case TYPE_WINDOW:
        return CreateWindowHandle();
    case TYPE_MENU:
        return CreateMenu();
    case TYPE_CURSOR:
        return CreateCursor(NULL, 0, 0, 4, 4, &gDummyCursorBits, &gDummyCursorBits);
    case TYPE_SETWINDOWPOS:
        return BeginDeferWindowPos(1);
    case TYPE_HOOK:
        return SetWindowsHookExW(WH_CALLWNDPROC, CallWndProcHook, NULL, GetCurrentThreadId());
    case TYPE_CLIPDATA:
        return 0;
    case TYPE_CALLPROC:
        return 0;
    case TYPE_ACCELTABLE:
        return LoadAcceleratorsW(NULL, MAKEINTRESOURCEW(1));
    case TYPE_DDEACCESS:
        return 0;
    case TYPE_DDECONV:
        return 0;
    case TYPE_DDEXACT:
        return 0;
    case TYPE_MONITOR:
        return 0;
    case TYPE_KBDLAYOUT:
        return LoadKeyboardLayoutW(L"00000409", KLF_ACTIVATE);
    case TYPE_KBDFILE:
        return 0;
    case TYPE_WINEVENTHOOK:
        return SetWinEventHook(EVENT_MIN, EVENT_MAX, NULL, NULL, GetCurrentProcessId(), GetCurrentThreadId(), WINEVENT_OUTOFCONTEXT);
    case TYPE_TIMER:
        return (HANDLE)SetTimer(NULL, 0, 1000, NULL);
    case TYPE_INPUTCONTEXT:
        return 0;
    case TYPE_HIDDATA:
        return 0;
    case TYPE_DEVICEINFO:
        return 0;
    case TYPE_TOUCHINPUTINFO:
        return 0;
    case TYPE_GESTUREINFOOBJ:
        return 0;
    }

    return 0;
}

static
BOOL
DestroyObject(HANDLE h, ULONG Type)
{
    switch (Type)
    {
    case TYPE_WINDOW:
        return DestroyWindow(h);
    case TYPE_MENU:
        return DestroyMenu(h);
    case TYPE_CURSOR:
        return DestroyCursor(h);
    case TYPE_SETWINDOWPOS:
        return EndDeferWindowPos(h);
    case TYPE_HOOK:
        return UnhookWindowsHookEx(h);
    case TYPE_CLIPDATA:
        break;
    case TYPE_CALLPROC:
        break;
    case TYPE_ACCELTABLE:
        return DestroyAcceleratorTable(h);
    case TYPE_DDEACCESS:
        break;
    case TYPE_DDECONV:
        break;
    case TYPE_DDEXACT:
        break;
    case TYPE_MONITOR:
        break;
    case TYPE_KBDLAYOUT:
        return UnloadKeyboardLayout(h);
    case TYPE_KBDFILE:
        break;
    case TYPE_WINEVENTHOOK:
        return UnhookWinEvent(h);
    case TYPE_TIMER:
        return KillTimer(NULL, (UINT_PTR)h);
    case TYPE_INPUTCONTEXT:
        break;
    case TYPE_HIDDATA:
        break;
    case TYPE_DEVICEINFO:
        break;
    case TYPE_TOUCHINPUTINFO:
        break;
    case TYPE_GESTUREINFOOBJ:
        break;
    }

    return FALSE;
}

static
ULONG GetMaxHandleCount(ULONG Type)
{
    HANDLE Handles[0x10000];
    ULONG i, count;

    for (i = 0; i < 0x10000; i++)
    {
        Handles[i] = CreateHandle(Type);
        if (Handles[i] == NULL)
        {
            break;
        }
    }

    count = i;

    for (i = 0; i < 0x10000; i++)
    {
        if (Handles[i] == NULL)
        {
            break;
        }

        ok_eq_int(DestroyObject(Handles[i], Type), 1);
    }

    return count;
}

static
VOID
Test_MaxHandleCount()
{
    ULONG count;

#if 0 // Disabled, because it takes over 1 minute to run
    count = GetMaxHandleCount(TYPE_WINDOW);
    ok(count > 9990 && count < 10000, "Window handles: %lu\n", count);
#endif

    count = GetMaxHandleCount(TYPE_MENU);
    ok(count > 9990 && count < 10000, "Menu handles: %lu\n", count);

    count = GetMaxHandleCount(TYPE_CURSOR);
    ok(count > 4990 && count < 5000, "Cursor handles: %lu\n", count);

    count = GetMaxHandleCount(TYPE_SETWINDOWPOS);
    ok(count > 9990 && count < 10000, "SetWindowPos handles: %lu\n", count);

    count = GetMaxHandleCount(TYPE_HOOK);
    ok(count > 4990 && count < 10000, "Cursor handles: %lu\n", count);

    count = GetMaxHandleCount(TYPE_TIMER);
    ok(count > 9990 && count < 10000, "Timer handles: %lu\n", count);

}

static
VOID
Test_HandleOwners()
{
    HMODULE hmodUser32;
    PSHAREDINFO pSharedInfo;
    PUSER_HANDLE_ENTRY pHandleTable, phe;
    HANDLE h;
    PVOID ppiCurrent, ptiCurrent;

    hmodUser32 = GetModuleHandleA("user32.dll");
    pSharedInfo = (PSHAREDINFO)GetProcAddress(hmodUser32, "gSharedInfo");
    if (pSharedInfo == NULL)
    {
        skip("gSharedInfo not found\n");
        return;
    }

    pHandleTable = pSharedInfo->aheList;

    /* Create a Window - This is thread-owned */
    h = CreateHandle(TYPE_WINDOW);
    phe = &pHandleTable[HandleToULong(h) & 0xFFFF];
    ok_eq_char(phe->type, TYPE_WINDOW);
    ok(phe->ptr != NULL, "Window handle ptr not set\n");
    ok(phe->pti != NULL, "Window handle owner not set\n");
    ptiCurrent = phe->pti;
    DestroyObject(h, TYPE_WINDOW);

    /* Create a Menu - This is process-owned */
    h = CreateHandle(TYPE_MENU);
    phe = &pHandleTable[HandleToULong(h) & 0xFFFF];
    ok_eq_char(phe->type, TYPE_MENU);
    ok(phe->ptr != NULL, "Menu handle ptr not set\n");
    ok(phe->ppi != NULL, "Menu handle owner not set\n");
    ok(phe->ppi != ptiCurrent, "Menu handle owner is ptiCurrent\n");
    ppiCurrent = phe->ppi;

    /* Create a Cursor - This is process-owned */
    h = CreateHandle(TYPE_CURSOR);
    phe = &pHandleTable[HandleToULong(h) & 0xFFFF];
    ok_eq_char(phe->type, TYPE_CURSOR);
    ok(phe->ptr != NULL, "Cursor handle ptr not set\n");
    ok_eq_pointer(phe->ppi , ppiCurrent);
    DestroyObject(h, TYPE_CURSOR);

    /* Create a SWP - This is thread-owned */
    h = CreateHandle(TYPE_SETWINDOWPOS);
    phe = &pHandleTable[HandleToULong(h) & 0xFFFF];
    ok_eq_char(phe->type, TYPE_SETWINDOWPOS);
    ok(phe->ptr != NULL, "SetWindowPos handle ptr not set\n");
    ok_eq_pointer(phe->ppi , ptiCurrent);
    DestroyObject(h, TYPE_SETWINDOWPOS);

    /* Create a Hook - This is process-owned */
    h = CreateHandle(TYPE_HOOK);
    phe = &pHandleTable[HandleToULong(h) & 0xFFFF];
    ok_eq_char(phe->type, TYPE_HOOK);
    ok(phe->ptr != NULL, "Hook handle ptr not set\n");
    ok_eq_pointer(phe->ppi, ppiCurrent);
    DestroyObject(h, TYPE_HOOK);

    /* Create a Timer - This is process-owned */
    h = CreateHandle(TYPE_TIMER);
    phe = &pHandleTable[HandleToULong(h) & 0xFFFF];
    ok_eq_char(phe->type, TYPE_TIMER);
    ok(phe->ptr != NULL, "Timer handle ptr not set\n");
    ok_eq_pointer(phe->ppi, ppiCurrent);
    DestroyObject(h, TYPE_TIMER);

}

START_TEST(UserHandles)
{
    PPEB ppeb = NtCurrentTeb()->ProcessEnvironmentBlock;
    gulWinVersion = (ULONG)ppeb->OSMajorVersion << 8 | ppeb->OSMinorVersion;

    Test_MaxHandleCount();

    if (gulWinVersion < _WIN32_WINNT_WIN10)
    {
        Test_HandleOwners();
    }
    else
    {
        skip("Test_HandleOwners doesn't work on Win 10\n");
    }
}
