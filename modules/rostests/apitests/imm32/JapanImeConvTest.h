/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for Japanese IME conversion
 * COPYRIGHT:   Copyright 2022 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <windowsx.h>
#include <imm.h>
//#include <ddk/immdev.h>
#include <wine/test.h>
//#include <pseh/pseh2.h>
#include <stdio.h>

#define INTERVAL 200
#define SMALL_INTERVAL 80

static WNDPROC s_fnOldEditWndProc = NULL;

typedef struct tagTEST_ENTRY
{
    const UINT *pKeys;
    UINT cKeys;
    const void *pvResult;
    INT cWM_IME_COMPOSITION;
} TEST_ENTRY, *PTEST_ENTRY;

static const UINT s_keys1[] =
{
    'T', 'E', 'S', 'U', 'T', 'O', VK_SPACE, VK_RETURN
};
static const UINT s_keys2[] =
{
    'C', 'H', 'O', 'U', 'S', 'A', 'I', 'N', 'N', VK_SPACE, VK_RETURN
};

#ifdef UNICODE
    #define AorW(a, w) w
#else
    #define AorW(a, w) a
#endif

static const TEST_ENTRY s_entries[] =
{
    { s_keys1, _countof(s_keys1), AorW("\x83\x65\x83\x58\x83\x67", L"\x30C6\x30B9\x30C8"), 1 }, // "テスト"
    { s_keys2, _countof(s_keys2), AorW("\x92\xB2\x8D\xB8\x88\xF5", L"\x8ABF\x67FB\x54E1"), 1 }, // "調査員"
};

static INT s_iEntry = 0;
static INT s_cWM_IME_COMPOSITION = 0;

#ifdef UNICODE
static LPSTR WideToAnsi(INT nCodePage, LPCWSTR pszWide)
{
    static CHAR s_sz[512];
    WideCharToMultiByte(nCodePage, 0, pszWide, -1, s_sz, _countof(s_sz), NULL, NULL);
    return s_sz;
}
#endif

static LRESULT CALLBACK
EditWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_IME_COMPOSITION:
        if ((lParam & GCS_RESULTSTR) == GCS_RESULTSTR)
        {
            const TEST_ENTRY *entry = &s_entries[s_iEntry];
            HIMC hIMC;
            ++s_cWM_IME_COMPOSITION;
            hIMC = ImmGetContext(hwnd);
            if (hIMC)
            {
                LONG cchResult = ImmGetCompositionString(hIMC, GCS_RESULTSTR, NULL, 0);
                LPTSTR pszResult = (LPTSTR)calloc(cchResult + 1, sizeof(TCHAR));
                ok(pszResult != NULL, "pszResult was NULL\n");
                ImmGetCompositionString(hIMC, GCS_RESULTSTR, pszResult, cchResult);
#ifdef UNICODE
                printf("%s\n", WideToAnsi(CP_ACP, (LPTSTR)pszResult));
#else
                printf("%s\n", (LPTSTR)pszResult);
#endif
                ok(lstrcmp(pszResult, (LPTSTR)entry->pvResult) == 0, "pszResult differs\n");
                free(pszResult);

                ImmReleaseContext(hwnd, hIMC);
            }
        }
        // FALL THROUGH
    default:
        return CallWindowProc(s_fnOldEditWndProc, hwnd, uMsg, wParam, lParam);
    }
}

#define STAGE_1 10001
#define STAGE_2 10002
#define STAGE_3 10003
#define STAGE_4 10004
#define STAGE_5 10005

static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    HWND hEdt1 = GetDlgItem(hwnd, edt1);
    s_fnOldEditWndProc = (WNDPROC)SetWindowLongPtr(hEdt1, GWLP_WNDPROC, (LONG_PTR)EditWindowProc);
    SetTimer(hwnd, STAGE_1, INTERVAL, NULL);
    return TRUE;
}

static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case IDOK:
        case IDCANCEL:
            EndDialog(hwnd, id);
            break;
    }
}

static VOID PressKey(UINT vk)
{
    keybd_event(vk, 0, 0, 0);
    keybd_event(vk, 0, KEYEVENTF_KEYUP, 0);
    Sleep(SMALL_INTERVAL);
}

static void OnTimer(HWND hwnd, UINT id)
{
    HIMC hIMC;
    INT i;
    const TEST_ENTRY *entry = &s_entries[s_iEntry];
    static DWORD dwOldConversion, dwOldSentence;

    KillTimer(hwnd, id);

    switch (id)
    {
        case STAGE_1:
            s_cWM_IME_COMPOSITION = 0;
            hIMC = ImmGetContext(hwnd);
            ok(hIMC != NULL, "hIMC was NULL");
            if (hIMC)
            {
                ImmSetOpenStatus(hIMC, TRUE);
                ImmGetConversionStatus(hIMC, &dwOldConversion, &dwOldSentence);
                ImmSetConversionStatus(hIMC, IME_CMODE_FULLSHAPE | IME_CMODE_ROMAN | IME_CMODE_NATIVE, IME_SMODE_SINGLECONVERT);
                ImmReleaseContext(hwnd, hIMC);
            }
            SetTimer(hwnd, STAGE_2, INTERVAL, NULL);
            break;

        case STAGE_2:
            for (i = 0; i < entry->cKeys; ++i)
            {
                PressKey(entry->pKeys[i]);
            }
            SetTimer(hwnd, STAGE_3, INTERVAL, NULL);
            break;

        case STAGE_3:
            hIMC = ImmGetContext(hwnd);
            ok(hIMC != NULL, "hIMC was NULL");
            if (hIMC)
            {
                ImmSetConversionStatus(hIMC, dwOldConversion, dwOldSentence);
                ImmReleaseContext(hwnd, hIMC);
            }
            SetTimer(hwnd, STAGE_4, INTERVAL, NULL);
            break;

        case STAGE_4:
            ok_int(s_cWM_IME_COMPOSITION, entry->cWM_IME_COMPOSITION);
            for (i = s_cWM_IME_COMPOSITION; i < entry->cWM_IME_COMPOSITION; ++i)
                ok_int(0, 1);

            ++s_iEntry;
            if (s_iEntry == _countof(s_entries))
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            else
                SetTimer(hwnd, STAGE_1, INTERVAL, NULL);
            break;
    }
}

static INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
    }
    return 0;
}

#ifdef UNICODE
START_TEST(JapanImeConvTestW)
#else
START_TEST(JapanImeConvTestA)
#endif
{
    if (PRIMARYLANGID(GetSystemDefaultLangID()) != LANG_JAPANESE)
    {
        skip("Non-Japanese\n");
        return;
    }

    if (!GetSystemMetrics(SM_IMMENABLED))
    {
        skip("SM_IMMENABLED is OFF\n");
        return;
    }

    if (!ImmIsIME(GetKeyboardLayout(0)))
    {
        skip("The IME keyboard layout was not default\n");
        return;
    }

    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(1), NULL, DialogProc);

    if (s_iEntry < _countof(s_entries))
        skip("Skipped\n");
}
