/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for Keyboard Layouts DLL files
 * COPYRIGHT:   Copyright 2022 Stanislav Motylkov <x86corez@gmail.com>
 */

#include "precomp.h"
#include <ntuser.h>

static
HANDLE
CreateWindowHandle()
{
    return CreateWindowExW(0, L"STATIC", L"Test", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);
}

static
HANDLE
CreateHandle(HANDLE_TYPE Type)
{
    switch (Type)
    {
    case TYPE_WINDOW:
        return CreateWindowHandle();
    case TYPE_MENU:
        return CreateMenu();
    case TYPE_CURSOR:
        return LoadCursorW(NULL, IDC_ARROW);
    case TYPE_SETWINDOWPOS:
        return 0;
    case TYPE_HOOK:
        return SetWindowsHookExW(WH_KEYBOARD, NULL, NULL, GetCurrentThreadId());
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
        return SetTimer(NULL, 0, 1000, NULL);
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
}

static
VOID
DestroyObject(HANDLE h, HANDLE_TYPE Type)
{
    switch (Type)
    {
    case TYPE_WINDOW:
        DestroyWindow(h);
        break;
    case TYPE_MENU:
        DestroyMenu(h);
        break;
    case TYPE_CURSOR:
        DestroyCursor(h);
        break;
    case TYPE_SETWINDOWPOS:
        break;
    case TYPE_HOOK:
        UnhookWindowsHookEx(h);
        break;
    case TYPE_CLIPDATA:
        break;
    case TYPE_CALLPROC:
        break;
    case TYPE_ACCELTABLE:
        DestroyAcceleratorTable(h);
        break;
    case TYPE_DDEACCESS:
        break;
    case TYPE_DDECONV:
        break;
    case TYPE_DDEXACT:
        break;
    case TYPE_MONITOR:
        break;
    case TYPE_KBDLAYOUT:
        UnloadKeyboardLayout(h);
        break;
    case TYPE_KBDFILE:
        break;
    case TYPE_WINEVENTHOOK:
        UnhookWinEvent(h);
        break;
    case TYPE_TIMER:
        KillTimer(NULL, (UINT_PTR)h);
        break;
    case TYPE_INPUTCONTEXT:
        break;
    case TYPE_HIDDATA:
        break;
    case TYPE_DEVICEINFO:
        break;
    case TYPE_TOUCHINPUTINFO:
        break;
    case TYPE_GESTUREINFOOBJ:
    }
}

static
ULONG GetMaxHandleCount(HANDLE_TYPE Type)
{
    HANDLE Handles[0x10000];
    HANDLE Handle;
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
        Handle = Handles[i];
        if (Handle == NULL)
        {
            break;
        }

        DestroyObject(Handle, Type);
    }

    return count;
}

static
VOID
Test_MaxHandleCount()
{
    ULONG count;

    count = GetMaxHandleCount(TYPE_WINDOW);
    ok(count > 9900 && count < 10000, "Window handles: %lu\n", count);

}

START_TEST(Handles)
{
    Test_MaxHandleCount();
}
