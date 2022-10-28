/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for Japanese IME conversion
 * COPYRIGHT:   Copyright 2022 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <windows.h>
#include <windowsx.h>
#include <imm.h>
#include <wine/test.h>

/*
 * We emulate some keyboard typing on dialog box and watch the conversion of Japanese IME.
 * This program needs Japanese environment and Japanese IME.
 * Tested on Japanese WinXP and Japanese Win10.
 */

#define INTERVAL 200
#define WM_PRESS_KEY_COMPLETE (WM_USER + 100)

/* The test entry structure */
typedef struct tagTEST_ENTRY
{
    const UINT *pKeys;
    UINT cKeys;
    const void *pvResult;
    INT cWM_IME_ENDCOMPOSITION;
} TEST_ENTRY, *PTEST_ENTRY;

// The Japanese word "テスト" conversion in Romaji
static const UINT s_keys1[] =
{
    'T', 'E', 'S', 'U', 'T', 'O', VK_SPACE, VK_RETURN
};
// The Japanese word "調査員" conversion in Romaji
static const UINT s_keys2[] =
{
    'C', 'H', 'O', 'U', 'S', 'A', 'I', 'N', 'N', VK_SPACE, VK_RETURN
};

#ifdef UNICODE
    #define AorW(a, w) w
#else
    #define AorW(a, w) a
#endif

/* The test entries */
static const TEST_ENTRY s_entries[] =
{
    // "テスト"
    { s_keys1, _countof(s_keys1), AorW("\x83\x65\x83\x58\x83\x67", L"\x30C6\x30B9\x30C8"), 1 },
    // "調査員"
    { s_keys2, _countof(s_keys2), AorW("\x92\xB2\x8D\xB8\x88\xF5", L"\x8ABF\x67FB\x54E1"), 1 },
};

static INT s_iEntry = 0;
static INT s_cWM_IME_ENDCOMPOSITION = 0;
static WNDPROC s_fnOldEditWndProc = NULL;

#ifdef UNICODE
static LPSTR WideToAnsi(INT nCodePage, LPCWSTR pszWide)
{
    static CHAR s_sz[512];
    WideCharToMultiByte(nCodePage, 0, pszWide, -1, s_sz, _countof(s_sz), NULL, NULL);
    return s_sz;
}
#endif

/* The window procedure for textbox to watch the IME conversion */
static LRESULT CALLBACK
EditWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_IME_ENDCOMPOSITION:
        {
            const TEST_ENTRY *entry = &s_entries[s_iEntry];
            HIMC hIMC;
            LONG cbResult;
            LPTSTR pszResult;

            /* Check conversion results of composition string */
            hIMC = ImmGetContext(hwnd);
            cbResult = ImmGetCompositionString(hIMC, GCS_RESULTSTR, NULL, 0);
            trace("cbResult: %ld\n", cbResult);
            if (cbResult > 0) /* Ignore zero string */
            {
                ok(hIMC != NULL, "hIMC was NULL\n");
                ++s_cWM_IME_ENDCOMPOSITION;

                pszResult = (LPTSTR)calloc(cbResult + sizeof(WCHAR), sizeof(BYTE));
                ok(pszResult != NULL, "pszResult was NULL\n");
                ImmGetCompositionString(hIMC, GCS_RESULTSTR, pszResult, cbResult);
#ifdef UNICODE
                trace("%s\n", WideToAnsi(CP_ACP, (LPTSTR)pszResult));
#else
                trace("%s\n", (LPTSTR)pszResult);
#endif
                ok(lstrcmp(pszResult, (LPTSTR)entry->pvResult) == 0, "pszResult differs\n");
                free(pszResult);
            }

            ImmReleaseContext(hwnd, hIMC);
        }
        break;
    }

    return CallWindowProc(s_fnOldEditWndProc, hwnd, uMsg, wParam, lParam);
}

/* Timer IDs */
#define STAGE_1 10001
#define STAGE_2 10002
#define STAGE_3 10003
#define STAGE_4 10004
#define STAGE_5 10005

/* WM_INITDIALOG */
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    /* Subclass the textbox to watch the IME conversion */
    HWND hEdt1 = GetDlgItem(hwnd, edt1);
    s_fnOldEditWndProc = (WNDPROC)SetWindowLongPtr(hEdt1, GWLP_WNDPROC, (LONG_PTR)EditWindowProc);

    /* Go to first stage */
    SetTimer(hwnd, STAGE_1, INTERVAL, 0);
    return TRUE;
}

/* WM_COMMAND */
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

/* Emulate keyboard typing */
static VOID PressKey(UINT vk)
{
    INPUT inputs[2];
    ZeroMemory(inputs, sizeof(inputs));
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vk;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = vk;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(_countof(inputs), inputs, sizeof(INPUT));
}

/* WM_TIMER */
static void OnTimer(HWND hwnd, UINT id)
{
    HIMC hIMC;
    INT i;
    HWND hImeWnd;
    const TEST_ENTRY *entry = &s_entries[s_iEntry];
    static DWORD dwOldConversion, dwOldSentence;

    KillTimer(hwnd, id);

    switch (id)
    {
        case STAGE_1:
            /* Check focus. See WM_INITDIALOG return code. */
            ok(GetFocus() == GetDlgItem(hwnd, edt1), "GetFocus() was %p\n", GetFocus());

            hIMC = ImmGetContext(hwnd);
            ok(hIMC != NULL, "hIMC was NULL");
            if (hIMC)
            {
                /* Open the IME */
                ImmSetOpenStatus(hIMC, TRUE);
                /* Save the IME conversion status */
                ImmGetConversionStatus(hIMC, &dwOldConversion, &dwOldSentence);
                /* Modify the IME conversion status */
                ImmSetConversionStatus(hIMC,
                                       IME_CMODE_FULLSHAPE | IME_CMODE_ROMAN | IME_CMODE_NATIVE,
                                       IME_SMODE_SINGLECONVERT);

                /* Don't show candidate list */
                hImeWnd = ImmGetDefaultIMEWnd(hwnd);
                ImmIsUIMessage(hImeWnd, WM_IME_SETCONTEXT, TRUE,
                               ISC_SHOWUICOMPOSITIONWINDOW /* | ISC_SHOWUICANDIDATEWINDOW*/);

                ImmReleaseContext(hwnd, hIMC);
            }
            /* Initialize the counter */
            s_cWM_IME_ENDCOMPOSITION = 0;
            /* Go to next stage */
            SetTimer(hwnd, STAGE_2, INTERVAL, NULL);
            break;

        case STAGE_2:
            /* Emulate keyboard typing */
            for (i = 0; i < entry->cKeys; ++i)
            {
                PressKey(entry->pKeys[i]);
            }
            /* Wait for message queue processed */
            PostMessage(hwnd, WM_PRESS_KEY_COMPLETE, 0, 0);
            break;

        case STAGE_3:
            /* Revert the IME conversion status */
            hIMC = ImmGetContext(hwnd);
            ok(hIMC != NULL, "hIMC was NULL");
            if (hIMC)
            {
                ImmSetConversionStatus(hIMC, dwOldConversion, dwOldSentence);
                ImmReleaseContext(hwnd, hIMC);
            }
            /* Go to next stage */
            SetTimer(hwnd, STAGE_4, INTERVAL, NULL);
            break;

        case STAGE_4:
            /* Check the counter */
            ok_int(s_cWM_IME_ENDCOMPOSITION, entry->cWM_IME_ENDCOMPOSITION);
            if (s_cWM_IME_ENDCOMPOSITION < entry->cWM_IME_ENDCOMPOSITION)
            {
                skip("Some tests were skipped.\n");
            }

            /* Go to next test entry */
            ++s_iEntry;
            if (s_iEntry == _countof(s_entries))
                PostMessage(hwnd, WM_CLOSE, 0, 0); /* No more entry */
            else
                SetTimer(hwnd, STAGE_1, INTERVAL, NULL);
            break;
    }
}

/* Dialog procedure */
static INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_TIMER, OnTimer);

    case WM_PRESS_KEY_COMPLETE:
        /* Message queue is processed. Go to next stage. */
        SetTimer(hwnd, STAGE_3, INTERVAL, NULL);
        break;
    }
    return 0;
}

#ifdef UNICODE
START_TEST(JapanImeConvTestW)
#else
START_TEST(JapanImeConvTestA)
#endif
{
    /* Is the system Japanese? */
    if (PRIMARYLANGID(GetSystemDefaultLangID()) != LANG_JAPANESE)
    {
        skip("This testcase is for Japanese only.\n");
        return;
    }

    /* Is IMM enabled? */
    if (!GetSystemMetrics(SM_IMMENABLED))
    {
        skip("SM_IMMENABLED is OFF.\n");
        return;
    }

    /* Check the current keyboard layout is IME */
    if (!ImmIsIME(GetKeyboardLayout(0)))
    {
        skip("The IME keyboard layout was not default\n");
        return;
    }

    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(1), NULL, DialogProc);

    if (s_iEntry < _countof(s_entries))
        skip("Some tests were skipped.\n");
}
