/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for NtUserGetThreadState
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <win32nt.h>

#define MAX_COUNT 8
#define IGNORED 0xDEADFACE
#define RAISED 0xBADBEEF

#undef DO_PRINT

#ifdef DO_PRINT
static VOID PrintThreadState(INT lineno, HWND hWnd)
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

static VOID CheckThreadState(INT lineno, INT i, DWORD_PTR dwState)
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
    {
        ok(dwValue == dwState, "Line %d: Mismatch 0x%lX vs. 0x%lX\n",
           lineno, (DWORD)dwValue, (DWORD)dwState);
    }
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

#define CHECK_THIS(i, value) \
    CheckThreadState(__LINE__, (i), (DWORD_PTR)(value));

#ifdef DO_PRINT
    PrintThreadState(__LINE__, hWnd);
#endif
    CHECK_THIS(0, hWnd);
    CHECK_THIS(1, hWnd);
    CHECK_THIS(2, 0);
    CHECK_THIS(3, hImeWnd);
    CHECK_THIS(4, hIMC);

    SetCapture(hWnd);

#ifdef DO_PRINT
    PrintThreadState(__LINE__, hWnd);
#endif
    CHECK_THIS(0, hWnd);
    CHECK_THIS(1, hWnd);
    CHECK_THIS(2, hWnd);
    CHECK_THIS(3, hImeWnd);
    CHECK_THIS(4, hIMC);

    ReleaseCapture();

#ifdef DO_PRINT
    PrintThreadState(__LINE__, hWnd);
#endif
    CHECK_THIS(0, hWnd);
    CHECK_THIS(1, hWnd);
    CHECK_THIS(2, 0);
    CHECK_THIS(3, hImeWnd);
    CHECK_THIS(4, hIMC);

    SetFocus(hWnd);

#ifdef DO_PRINT
    PrintThreadState(__LINE__, hWnd);
#endif
    CHECK_THIS(0, hWnd);
    CHECK_THIS(1, hWnd);
    CHECK_THIS(2, 0);
    CHECK_THIS(3, hImeWnd);
    CHECK_THIS(4, hIMC);

    SetActiveWindow(hWnd);

#ifdef DO_PRINT
    PrintThreadState(__LINE__, hWnd);
#endif
    CHECK_THIS(0, hWnd);
    CHECK_THIS(1, hWnd);
    CHECK_THIS(2, 0);
    CHECK_THIS(3, hImeWnd);
    CHECK_THIS(4, hIMC);

    SetActiveWindow(NULL);

#ifdef DO_PRINT
    PrintThreadState(__LINE__, hWnd);
#endif
    CHECK_THIS(0, 0);
    CHECK_THIS(1, 0);
    CHECK_THIS(2, 0);
    CHECK_THIS(3, hImeWnd);
    CHECK_THIS(4, hIMC);

    DestroyWindow(hWnd);
}
