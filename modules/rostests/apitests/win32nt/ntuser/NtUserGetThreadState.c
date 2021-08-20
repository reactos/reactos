/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for NtUserGetThreadState
 * COPYRIGHT:   Copyright 2021 Mike Blablabla <katayama.hirofumi.mz@gmail.com>
 */

#include <win32nt.h>

#define MAX_COUNT 8
#define IGNORED 0xDEADFACE
#define RAISED 0xBADBEEF

#undef DO_PRINT

#ifdef DO_PRINT
static void PrintThreadState(HWND hWnd, int lineno)
{
    INT i;
    HIMC hIMC = ImmGetContext(hWnd);
    HWND hImeWnd = ImmGetDefaultIMEWnd(hWnd);

    trace("---\n");
    trace("__LINE__: %d\n", lineno);
    trace("hWnd: %p\n", (LPVOID)hWnd);
    trace("GetFocus(): %p\n", (LPVOID)GetFocus());
    trace("GetCapture(): %p\n", (LPVOID)GetCapture());
    trace("GetActiveWindow(): %p\n", (LPVOID)GetActiveWindow());
    trace("ImmGetContext(): %p\n", (LPVOID)hIMC);
    trace("ImmGetDefaultIMEWnd(): %p\n", (LPVOID)hImeWnd);

    for (i = 0; i < MAX_COUNT; ++i)
    {
        _SEH2_TRY
        {
            trace("ThreadState[%d]: %p\n", i, (LPVOID)NtUserGetThreadState(i));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            trace("ThreadState[%d]: exception\n", i);
        }
        _SEH2_END;
    }

    ImmReleaseContext(hWnd, hIMC);
}
#endif

static void CheckThreadState(INT i, DWORD_PTR dwState)
{
    DWORD_PTR dwValue;

    _SEH2_TRY
    {
        dwValue = (DWORD_PTR)NtUserGetThreadState(i);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        dwValue = RAISED;
    }
    _SEH2_END;

    if (dwState != IGNORED)
        ok_long((DWORD)dwValue, (DWORD)dwState);
}

START_TEST(NtUserGetThreadState)
{
    HWND hWnd, hImeWnd;
    HIMC hIMC;

    hWnd = CreateWindowA("EDIT", "Test", ES_LEFT | ES_MULTILINE | WS_VISIBLE,
                         0, 0, 50, 30,
                         NULL, NULL, GetModuleHandleW(NULL), NULL);
    hImeWnd = ImmGetDefaultIMEWnd(hWnd);
    hIMC = ImmGetContext(hWnd);

#ifdef DO_PRINT
    PrintThreadState(hWnd, __LINE__);
#endif
    CheckThreadState(0, (DWORD_PTR)hWnd);
    CheckThreadState(1, (DWORD_PTR)hWnd);
    CheckThreadState(2, (DWORD_PTR)0);
    CheckThreadState(3, (DWORD_PTR)hImeWnd);
    CheckThreadState(4, (DWORD_PTR)hIMC);

    SetCapture(hWnd);

#ifdef DO_PRINT
    PrintThreadState(hWnd, __LINE__);
#endif
    CheckThreadState(0, (DWORD_PTR)hWnd);
    CheckThreadState(1, (DWORD_PTR)hWnd);
    CheckThreadState(2, (DWORD_PTR)hWnd);
    CheckThreadState(3, (DWORD_PTR)hImeWnd);
    CheckThreadState(4, (DWORD_PTR)hIMC);

    ReleaseCapture();

#ifdef DO_PRINT
    PrintThreadState(hWnd, __LINE__);
#endif
    CheckThreadState(0, (DWORD_PTR)hWnd);
    CheckThreadState(1, (DWORD_PTR)hWnd);
    CheckThreadState(2, (DWORD_PTR)0);
    CheckThreadState(3, (DWORD_PTR)hImeWnd);
    CheckThreadState(4, (DWORD_PTR)hIMC);

    SetFocus(hWnd);

#ifdef DO_PRINT
    PrintThreadState(hWnd, __LINE__);
#endif
    CheckThreadState(0, (DWORD_PTR)hWnd);
    CheckThreadState(1, (DWORD_PTR)hWnd);
    CheckThreadState(2, (DWORD_PTR)0);
    CheckThreadState(3, (DWORD_PTR)hImeWnd);
    CheckThreadState(4, (DWORD_PTR)hIMC);

    SetActiveWindow(hWnd);

#ifdef DO_PRINT
    PrintThreadState(hWnd, __LINE__);
#endif
    CheckThreadState(0, (DWORD_PTR)hWnd);
    CheckThreadState(1, (DWORD_PTR)hWnd);
    CheckThreadState(2, (DWORD_PTR)0);
    CheckThreadState(3, (DWORD_PTR)hImeWnd);
    CheckThreadState(4, (DWORD_PTR)hIMC);

    SetActiveWindow(NULL);

#ifdef DO_PRINT
    PrintThreadState(hWnd, __LINE__);
#endif
    CheckThreadState(0, (DWORD_PTR)0);
    CheckThreadState(1, (DWORD_PTR)0);
    CheckThreadState(2, (DWORD_PTR)0);
    CheckThreadState(3, (DWORD_PTR)hImeWnd);
    CheckThreadState(4, (DWORD_PTR)hIMC);

    DestroyWindow(hWnd);
}
