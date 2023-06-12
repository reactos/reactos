/*
 * PROJECT:     ReactOS Console IME
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Console IME
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "conime.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(conime);

#undef DEBUGOPTIONS
/* #define DEBUGOPTIONS */

PUSER_DATA              g_pUserData = NULL;
HIMC                    g_hOldIMC = NULL;
BOOL                    g_bIsLogOnSession = TRUE;
RTL_CRITICAL_SECTION    g_csLock;
DWORD                   g_dwAttachToThreadId = 0;
UINT                    g_cEntries = 0;
HANDLE                  g_ConsoleHandle = NULL;
BOOL                    g_bInActive = FALSE;

BOOL IntIsDoubleWidthChar(WCHAR wch)
{
    ULONG ByteSize;

    if (0x20 <= wch && wch <= 0x7E)
        return FALSE;

    if ((0x3041 <= wch && wch <= 0x3094) || (0x30A1 <= wch && wch <= 0x30F6) ||
        (0x3105 <= wch && wch <= 0x312C) || (0x3131 <= wch && wch <= 0x318E) ||
        (0xAC00 <= wch && wch <= 0xD7A3) || (0xFF01 <= wch && wch <= 0xFF5E))
    {
        return TRUE;
    }

    if ((0xFF61 <= wch && wch <= 0xFF9F) || (0xFFA0 <= wch && wch <= 0xFFBE) ||
        (0xFFC2 <= wch && wch <= 0xFFC7) || (0xFFCA <= wch && wch <= 0xFFCF) ||
        (0xFFD2 <= wch && wch <= 0xFFD7) || (0xFFDA <= wch && wch <= 0xFFDC))
    {
        return FALSE;
    }

    if ((0xFFE0 <= wch && wch <= 0xFFE6) || (0x4E00 <= wch && wch <= 0x9FA5) ||
        (0xF900 <= wch && wch <= 0xFA2D))
    {
        return TRUE;
    }

    RtlUnicodeToMultiByteSize(&ByteSize, &wch, sizeof(wch));
    return ByteSize == 2;
}

size_t IntGetCharInfoWidth(PCHAR_INFO pCharInfo, size_t cch)
{
    size_t ich, ret = 0;
    for (ich = 0; ich < cch; ++ich)
        ret += IntIsDoubleWidthChar(pCharInfo[ich].Char.UnicodeChar) + 1;
    return ret;
}

BOOL IntIsLogOnSession(VOID)
{
    ULONG cbReturn;
    LUID luid;
    HANDLE hToken;
    TOKEN_STATISTICS ts;
    static const LUID SystemLuid = SYSTEM_LUID;

    ZeroMemory(&luid, sizeof(luid));
    if (NT_SUCCESS(NtOpenProcessToken(INVALID_HANDLE_VALUE, TOKEN_QUERY, &hToken)))
    {
        NtQueryInformationToken(hToken, TokenStatistics, &ts, sizeof(ts), &cbReturn);
        NtClose(hToken);

        RtlCopyLuid(&luid, &ts.AuthenticationId);

        /* The LUID for the System account's logon session is always 999 */
        if (!RtlEqualLuid(&luid, &SystemLuid))
            g_bIsLogOnSession = FALSE;
    }

    return g_bIsLogOnSession;
}

/* Communicate with console window by WM_COPYDATA */
BOOL IntSendCopyDataToConsole(HWND hwndConsole, HWND hwndSender, PCOPYDATASTRUCT pcds)
{
    LRESULT ret;
    DWORD_PTR result;

    if (!hwndConsole)
        return FALSE;

    ret = SendMessageTimeoutW(hwndConsole, WM_COPYDATA, (WPARAM)hwndSender, (LPARAM)pcds,
                              SMTO_ABORTIFHUNG, 3000, (PDWORD_PTR)&result);
    return (ret != 0);
}

PCONSOLE_ENTRY IntFindConsoleEntry(HANDLE ConsoleHandle)
{
    INT iEntry;
    PCONSOLE_ENTRY pEntry;

    RtlEnterCriticalSection(&g_csLock);
    if (!g_ConsoleHandle)
        g_ConsoleHandle = ConsoleHandle;

    if (g_cEntries <= 1)
    {
        RtlLeaveCriticalSection(&g_csLock);
        return NULL;
    }

    for (iEntry = 1; iEntry < g_cEntries; ++iEntry)
    {
        pEntry = g_pUserData->apEntries[iEntry];
        if (pEntry && pEntry->ConsoleHandle == ConsoleHandle && !pEntry->bWndEnabled)
        {
            RtlLeaveCriticalSection(&g_csLock);
            return pEntry;
        }
    }

    RtlLeaveCriticalSection(&g_csLock);
    return NULL;
}

BOOL IntGetLayoutText(PCONSOLE_ENTRY pEntry)
{
    static const WCHAR aszMode[][7] =
    {
        { 0x5009, 0x9821, 0x8ACB, 0x8F38, 0x5165, 0x5B57, 0x6839 }, /* "倉頡請輸入字根" */
        { 0x5167, 0x78BC, 0x8ACB, 0x8F38, 0x5165, 0x5167, 0x78BC }, /* "內碼請輸入內碼" */
        { 0x55AE, 0x78BC, 0x8ACB, 0x8F38, 0x5165, 0x55AE, 0x78BC }, /* "單碼請輸入單碼" */
        { 0x901F, 0x6210, 0x8ACB, 0x8F38, 0x5165, 0x5B57, 0x6839 }, /* "速成請輸入字根" */
        { 0x5927, 0x6613, 0x8ACB, 0x8F38, 0x5165, 0x5B57, 0x6839 }, /* "大易請輸入字根" */
        { 0x82F1, 0x6570, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000 }, /* "英数　　　　　" */
        { 0xFF55, 0xFF53, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000 }, /* "ｕｓ　　　　　" */
        { 0x6CE8, 0x97F3, 0x8ACB, 0x8F38, 0x5165, 0x7B26, 0x865F }, /* "注音請輸入符號" */
    };
    HIMC hOldIMC = pEntry->hOldIMC;
    HKL hKL = pEntry->hKL;
    WCHAR szLayoutName[256], szKey[MAX_PATH];
    HKEY phkResult;
    DWORD cbData;
    size_t i;

    pEntry->szLayoutText[0] = UNICODE_NULL;
    pEntry->szMode[0] = UNICODE_NULL;

    /* Try to get layout name by using ImmEscapeW or ImmGetIMEFileNameW */
    if (ImmEscapeW(hKL, hOldIMC, IME_ESC_IME_NAME, szLayoutName) ||
        ImmGetIMEFileNameW(pEntry->hKL, szLayoutName, _countof(szLayoutName)))
    {
        StringCchCopyW(pEntry->szLayoutText, _countof(pEntry->szLayoutText), szLayoutName);
        return TRUE; /* Success */
    }

    /* The IME HKL must begin with 'E' or 'e' */
    if (!GetKeyboardLayoutNameW(szLayoutName) || (szLayoutName[0] != 'E' && szLayoutName[0] != 'e'))
        return FALSE; /* Failure */

    /* Try to get layout text from registry */
    StringCchCopyW(szKey, _countof(szKey), L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\");
    StringCchCatW(szKey, _countof(szKey), szLayoutName);
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                      &phkResult) != ERROR_SUCCESS)
    {
        return FALSE; /* Failure */
    }

    cbData = sizeof(pEntry->szLayoutText);
    RegQueryValueExW(phkResult, L"Layout Text", 0, 0, (LPBYTE)pEntry->szLayoutText, &cbData);
    RegCloseKey(phkResult);

    /* Get the mode text */
    if (pEntry->szLayoutText[0])
    {
        LPCWSTR pszText = pEntry->szLayoutText;
        for (i = 0; i < _countof(aszMode); ++i)
        {
            if (pszText[0] == aszMode[i][0] && pszText[1] == aszMode[i][1])
            {
                StringCchCopyW(pEntry->szMode, 5 + 1, &aszMode[i][2]);
                break;
            }
        }
    }

    return TRUE; /* Success */
}

BOOL IntFillImeStatusCHT(PCONSOLE_ENTRY pEntry, PIME_STATUS pImeStatus)
{
    FIXME("\n");
    return FALSE;
}

BOOL IntFillImeStatusJPN(PCONSOLE_ENTRY pEntry, PIME_STATUS pImeStatus)
{
    static const WCHAR szZenKatakana[] = { 0x5168, 0x30AB };               /* "全カ" */
    static const WCHAR szZenHiragana[] = { 0x5168, 0x3042 };               /* "全あ" */
    static const WCHAR szZenAscii[] =    { 0x5168, 0xFF21 };               /* "全Ａ" */
    static const WCHAR szHanKanakana[] = { 0x534A, 0xFF76, L' ' };         /* "半ｶ " */
    static const WCHAR szHanHiragana[] = { 0x534A, 0xFF71, L' ' };         /* "半ｱ " */
    static const WCHAR szHanAscii[] =    { 0x534A, L'A', L' ' };           /* "半A " */
    static const WCHAR szMulti[] =       { 0x8907 };                       /* "複"   */
    static const WCHAR szSingle[] =      { 0x5358 };                       /* "単"   */
    static const WCHAR szAuto[] =        { 0x81EA };                       /* "自"   */
    static const WCHAR szRen[] =         { 0x9023 };                       /* "連"   */
    static const WCHAR szNone[] =        { L' ', L' ' };                   /* "  "   */
    static const WCHAR szKana[] =        { 0xFF76, 0xFF85, L' ', L' ' };   /* "ｶﾅ  " */
    static const WCHAR szRoma[] =        { 0xFF9B, 0xFF70, 0xFF8F, L' ' }; /* "ﾛｰﾏ " */
    PCHAR_INFO pCharInfo = pImeStatus->CharInfo;
    DWORD dwConversion = pEntry->dwConversion, dwSentence = pEntry->dwSentence;
    size_t i, ich = 0;

    if (!pEntry->bOpened)
        goto Finish;

#define ADD_DATA(szData) do { \
    size_t cchData = _countof(szData); \
    for (i = 0; i < cchData; ++i) { \
        pCharInfo->Char.UnicodeChar = szData[i]; \
        ++pCharInfo; \
        ++ich; \
    } \
} while (0)

    if (dwConversion & IME_CMODE_FULLSHAPE)
    {
        if (dwConversion & IME_CMODE_NATIVE)
        {
            if (dwConversion & IME_CMODE_KATAKANA)
                ADD_DATA(szZenKatakana);
            else
                ADD_DATA(szZenHiragana);
        }
        else
        {
            ADD_DATA(szZenAscii);
        }
    }
    else
    {
        if (dwConversion & IME_CMODE_NATIVE)
        {
            if (dwConversion & IME_CMODE_KATAKANA)
                ADD_DATA(szHanKanakana);
            else
                ADD_DATA(szHanHiragana);
        }
        else
        {
            ADD_DATA(szHanAscii);
        }
    }

    if (dwSentence & IME_SMODE_PLAURALCLAUSE)
        ADD_DATA(szMulti);
    else if (dwSentence & IME_SMODE_SINGLECONVERT)
        ADD_DATA(szSingle);
    else if (dwSentence & IME_SMODE_AUTOMATIC)
        ADD_DATA(szAuto);
    else if (dwSentence & IME_SMODE_PHRASEPREDICT)
        ADD_DATA(szRen);
    else
        ADD_DATA(szNone);

    if (GetKeyState(VK_KANA) & 1)
        ADD_DATA(szKana);
    else if (dwConversion & IME_CMODE_ROMAN)
        ADD_DATA(szRoma);

#undef ADD_DATA

Finish:
    pCharInfo = pImeStatus->CharInfo;
    for (i = 0; i < ich; ++i)
    {
        pCharInfo->Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        ++pCharInfo;
    }

    pImeStatus->cch = (DWORD)ich;
    pImeStatus->unknown0 = 1;
    return TRUE;
}

BOOL IntFillImeStatusKOR(PCONSOLE_ENTRY pEntry, PIME_STATUS pImeStatus)
{
    pImeStatus->CharInfo[0].Char.UnicodeChar = L' ';
    pImeStatus->CharInfo[0].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    pImeStatus->cch = 1;
    pImeStatus->unknown0 = 1;
    return TRUE;
}

BOOL IntFillImeStatusCHS(PCONSOLE_ENTRY pEntry, PIME_STATUS pImeStatus)
{
    FIXME("\n");
    return FALSE;
}

BOOL IntFillImeStatus(PCONSOLE_ENTRY pEntry, PIME_STATUS pImeStatus)
{
    switch (LOWORD(pEntry->hKL))
    {
        case LANGID_CHINESE_TRADITIONAL: return IntFillImeStatusCHT(pEntry, pImeStatus);
        case LANGID_JAPANESE:            return IntFillImeStatusJPN(pEntry, pImeStatus);
        case LANGID_KOREAN:              return IntFillImeStatusKOR(pEntry, pImeStatus);
        case LANGID_CHINESE_SIMPLIFIED:  return IntFillImeStatusCHS(pEntry, pImeStatus);
        default:                         return FALSE;
    }
}

/* WM_CREATE */
INT ConIme_OnCreate(HWND hWnd)
{
    g_hOldIMC = ImmGetContext(hWnd);
    return 0;
}

/* WM_ENDSESSION / WM_DESTROY */
VOID ConIme_OnEnd(HWND hWnd, UINT uMsg)
{
    HIMC hIMC;
    LPCANDIDATELIST *ppCandList;
    INT i;
    DWORD dwIndex;
    PCONSOLE_ENTRY pEntry;

    ImmAssociateContext(hWnd, g_hOldIMC);

    RtlEnterCriticalSection(&g_csLock);

    /* Free the entries */
    for (dwIndex = 1; dwIndex < g_cEntries; ++dwIndex)
    {
        pEntry = g_pUserData->apEntries[dwIndex];
        if (!pEntry)
            continue;

        hIMC = pEntry->hIMC;
        if (pEntry->bOpened)
        {
            pEntry->bOpened = FALSE;
            ImmSetOpenStatus(hIMC, FALSE);
        }
        ImmDestroyContext(hIMC);

        pEntry->pCompStr = LocalFree(pEntry->pCompStr);

        ppCandList = pEntry->apCandList;
        for (i = 0; i < _countof(pEntry->apCandList); ++i)
        {
            if (ppCandList[i])
            {
                ppCandList[i] = LocalFree(ppCandList[i]);
                pEntry->acbCandList[i] = 0;
            }
        }

        pEntry->pSystemLine = LocalFree(pEntry->pSystemLine);

        if (pEntry->pLocal2)
        {
            LocalFree(pEntry->pLocal2);
            pEntry->unknown3_5_0 = 0;
        }

        if (pEntry->pKLInfo)
            pEntry->pKLInfo = LocalFree(pEntry->pKLInfo);

        LocalFree(pEntry);
    }

    g_cEntries = 0;
    g_pUserData = LocalFree(g_pUserData);

    RtlLeaveCriticalSection(&g_csLock);

    if (g_dwAttachToThreadId && (uMsg != WM_ENDSESSION || !g_bIsLogOnSession))
    {
        /* Detach the thread input */
        AttachThreadInput(GetCurrentThreadId(), g_dwAttachToThreadId, FALSE);

        /* Unregister the Console IME */
        UnregisterConsoleIME();

        g_dwAttachToThreadId = 0;
    }
}

/* WM_ENABLE */
LRESULT ConIme_OnEnable(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    UINT i;
    PCONSOLE_ENTRY pEntry;

    if (wParam)
    {
        RtlEnterCriticalSection(&g_csLock);
        for (i = 1; i < g_cEntries; ++i)
        {
            pEntry = g_pUserData->apEntries[i];
            if (pEntry && pEntry->ConsoleHandle)
            {
                if (!pEntry->bConsoleEnabled && !IsWindowEnabled(pEntry->hwndConsole))
                {
                    EnableWindow(pEntry->hwndConsole, TRUE);
                    pEntry->bConsoleEnabled = TRUE;
                    if (!pEntry->bWndEnabled)
                        SetForegroundWindow(pEntry->hwndConsole);
                }
            }
        }
        RtlLeaveCriticalSection(&g_csLock);
    }
    else
    {
        pEntry = IntFindConsoleEntry(g_ConsoleHandle);
        if (pEntry && pEntry->ConsoleHandle)
        {
            pEntry->bConsoleEnabled = FALSE;
            EnableWindow(pEntry->hwndConsole, FALSE);
            g_bInActive = TRUE;
        }
    }

    return DefWindowProcW(hWnd, WM_ENABLE, wParam, lParam);
}

/* WM_KEYFIRST ... WM_KEYLAST */
BOOL ConIme_OnKeyChar(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PCONSOLE_ENTRY pEntry = IntFindConsoleEntry(g_ConsoleHandle);
    if (!pEntry)
        return FALSE;
    return PostMessageW(pEntry->hwndConsole, uMsg + 0x800, wParam, lParam); /* ==> 0x900... */
}

/* IMN_OPENSTATUSWINDOW */
BOOL ConIme_SendImeStatus(HWND hWnd)
{
    PCONSOLE_ENTRY pEntry;
    HIMC hIMC;
    PIME_STATUS pImeStatus;
    COPYDATASTRUCT CopyData;

    pEntry = IntFindConsoleEntry(g_ConsoleHandle);
    if (!pEntry)
        return FALSE;

    hIMC = ImmGetContext(hWnd);
    if (!hIMC)
        return FALSE;

    pImeStatus = LocalAlloc(LPTR, sizeof(IME_STATUS));
    if (!pImeStatus)
    {
        ImmReleaseContext(hWnd, hIMC);
        return FALSE;
    }

    ImmGetConversionStatus(hIMC, &pEntry->dwConversion, &pEntry->dwSentence);

    CopyData.dwData = CONIME_COPYDATA_SEND_IME_STATUS;
    CopyData.cbData = sizeof(IME_STATUS);
    CopyData.lpData = pImeStatus;

    if (IntFillImeStatus(pEntry, pImeStatus))
        IntSendCopyDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);

    LocalFree(pImeStatus);
    ImmReleaseContext(hWnd, hIMC);
    return TRUE;
}

/* WM_INPUTLANGCHANGEREQUEST */
LRESULT ConIme_OnInputLangChangeRequest(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    PCONSOLE_ENTRY pEntry = IntFindConsoleEntry(g_ConsoleHandle);
    if (!pEntry)
        return DefWindowProcW(hWnd, WM_INPUTLANGCHANGEREQUEST, wParam, lParam);

    PostMessageW(pEntry->hwndConsole, WM_INPUTLANGCHANGEREQUEST, wParam, lParam);
    return TRUE;
}

/* WM_INPUTLANGCHANGE */
VOID ConIme_OnInputLangChange(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HKL hKL = (HKL)lParam;
    PCONSOLE_ENTRY pEntry = IntFindConsoleEntry(g_ConsoleHandle);
    if (!pEntry)
        return;

    pEntry->hKL = hKL;
    ActivateKeyboardLayout(hKL, 0);
    IntGetLayoutText(pEntry);
    ConIme_SendImeStatus(hWnd);
}

/* WM_IME_SYSTEM */
VOID ConIme_OnImeSystem(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    COPYDATASTRUCT CopyData;
    PCONSOLE_ENTRY pEntry;

    pEntry = IntFindConsoleEntry(g_ConsoleHandle);
    if (!pEntry)
        return;

    CopyData.lpData = &wParam;
    CopyData.dwData = CONIME_COPYDATA_SEND_IME_SYSTEM;
    CopyData.cbData = sizeof(wParam);
    IntSendCopyDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);
}

VOID IntCloseCandidateCHS(HWND hWnd, HIMC hIMC, PCONSOLE_ENTRY pEntry, DWORD dwCandidates)
{
    PIME_STATUS pImeStatus;
    UINT iCand;
    LPCANDIDATELIST *ppCandList;
    COPYDATASTRUCT CopyData;

    pImeStatus = LocalAlloc(LPTR, sizeof(IME_STATUS));
    if (!pImeStatus)
        return;

    /* Clear candidate info */
    ppCandList = pEntry->apCandList;
    for (iCand = 0; iCand < _countof(pEntry->apCandList); ++iCand)
    {
        if ((dwCandidates & (1 << iCand)) && ppCandList[iCand])
        {
            ppCandList[iCand] = LocalFree(ppCandList[iCand]);
            pEntry->acbCandList[iCand] = 0;
        }
    }

    CopyData.dwData = CONIME_COPYDATA_SEND_IME_STATUS;
    CopyData.cbData = sizeof(IME_STATUS);
    CopyData.lpData = pImeStatus;
    if (IntFillImeStatusCHS(pEntry, pImeStatus))
        IntSendCopyDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);

    LocalFree(pImeStatus);
}

VOID IntCloseCandidateJpnOrKor(HWND hWnd, HIMC hIMC, PCONSOLE_ENTRY pEntry, DWORD dwCandidates)
{
    UINT iCand;
    LPCANDIDATELIST *ppCandList;
    COPYDATASTRUCT CopyData;

    /* Clear candidate info */
    ppCandList = pEntry->apCandList;
    for (iCand = 0; iCand < _countof(pEntry->apCandList); ++iCand)
    {
        if ((dwCandidates & (1 << iCand)) && ppCandList[iCand])
        {
            ppCandList[iCand] = LocalFree(ppCandList[iCand]);
            pEntry->acbCandList[iCand] = 0;
        }
    }

    CopyData.dwData = CONIME_COPYDATA_SEND_CAND_INFO;
    CopyData.cbData = 0;
    CopyData.lpData = NULL;
    IntSendCopyDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);
}

VOID IntCloseCandidateCHT(HWND hWnd, HIMC hIMC, PCONSOLE_ENTRY pEntry, DWORD dwCandidates)
{
    PIME_STATUS pImeStatus;
    UINT iCand;
    LPCANDIDATELIST *ppCandList;
    COPYDATASTRUCT CopyData;

    pImeStatus = LocalAlloc(LPTR, sizeof(IME_STATUS));
    if (!pImeStatus)
        return;

    /* Clear candidate info */
    ppCandList = pEntry->apCandList;
    for (iCand = 0; iCand < _countof(pEntry->apCandList); ++iCand)
    {
        if ((dwCandidates & (1 << iCand)) && ppCandList[iCand])
        {
            ppCandList[iCand] = LocalFree(ppCandList[iCand]);
            pEntry->acbCandList[iCand] = 0;
        }
    }

    CopyData.dwData = CONIME_COPYDATA_SEND_IME_STATUS;
    CopyData.cbData = sizeof(IME_STATUS);
    CopyData.lpData = pImeStatus;
    if (IntFillImeStatusCHT(pEntry, pImeStatus))
        IntSendCopyDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);

    LocalFree(pImeStatus);
}

/* IMN_CLOSECANDIDATE */
BOOL ConIme_OnNotifyCloseCandidate(HWND hWnd, DWORD dwCandidates)
{
    PCONSOLE_ENTRY pEntry;
    HIMC hIMC;
    LANGID wLang;

    pEntry = IntFindConsoleEntry(g_ConsoleHandle);
    if (!pEntry)
        return FALSE;

    hIMC = ImmGetContext(hWnd);
    if (!hIMC)
        return FALSE;

    wLang = LOWORD(pEntry->hKL);
    pEntry->bHasCands = FALSE;

    switch (wLang)
    {
        case LANGID_CHINESE_SIMPLIFIED:
            IntCloseCandidateCHS(hWnd, hIMC, pEntry, dwCandidates);
            break;
        case LANGID_JAPANESE:
        case LANGID_KOREAN:
            IntCloseCandidateJpnOrKor(hWnd, hIMC, pEntry, dwCandidates);
            break;
        case LANGID_CHINESE_TRADITIONAL:
            IntCloseCandidateCHT(hWnd, hIMC, pEntry, dwCandidates);
            break;
        default:
            break;
    }

    ImmReleaseContext(hWnd, hIMC);
    return TRUE;
}

LPCANDIDATELIST IntGetCandListFromEntry(PCONSOLE_ENTRY pEntry, HIMC hIMC, DWORD dwIndex)
{
    LPCANDIDATELIST pCandList;
    DWORD cbCandList = ImmGetCandidateListW(hIMC, dwIndex, NULL, 0);
    if (!cbCandList)
        return FALSE;

    if (pEntry->apCandList[dwIndex] && pEntry->acbCandList[dwIndex] != cbCandList)
    {
        pEntry->apCandList[dwIndex] = LocalFree(pEntry->apCandList[dwIndex]);
        pEntry->acbCandList[dwIndex] = 0;
    }

    if (!pEntry->apCandList[dwIndex])
    {
        pEntry->apCandList[dwIndex] = LocalAlloc(LPTR, cbCandList);
        if (pEntry->apCandList[dwIndex] == NULL)
            return NULL;

        pEntry->acbCandList[dwIndex] = cbCandList;
    }

    pCandList = pEntry->apCandList[dwIndex];
    ImmGetCandidateList(hIMC, dwIndex, pCandList, cbCandList);

    if (pCandList->dwCount > 1 &&
        (cbCandList <= pCandList->dwSelection ||
         cbCandList <= pCandList->dwPageStart ||
         cbCandList <= pCandList->dwOffset[pCandList->dwSelection] ||
         cbCandList <= pCandList->dwOffset[pCandList->dwPageStart]))
    {
        return NULL;
    }

    return pCandList;
}

PCONIME_SYSTEMLINE IntAllocSystemLine(PCONSOLE_ENTRY pEntry, LPDWORD pcchSysLine)
{
    PCONIME_SYSTEMLINE pSystemLine;
    DWORD cbSystemLine, cchSysLine;

    *pcchSysLine = 0;

    cchSysLine = pEntry->dwCoord.X;
    if (cchSysLine > 128)
        cchSysLine = 128;
    if (cchSysLine < 12)
        cchSysLine = 12;

    cbSystemLine = sizeof(DWORD) + (sizeof(WCHAR) + sizeof(BYTE)) * cchSysLine;
    if (pEntry->cbSystemLine < cbSystemLine)
    {
        if (pEntry->pSystemLine)
        {
            pEntry->pSystemLine = LocalFree(pEntry->pSystemLine);
            pEntry->cbSystemLine = 0;
        }

        pEntry->pSystemLine = LocalAlloc(LPTR, cbSystemLine);
        if (pEntry->pSystemLine == NULL)
            return FALSE;

        pEntry->cbSystemLine = cbSystemLine;
    }

    pSystemLine = pEntry->pSystemLine;
    pSystemLine->dwAttrOffset = sizeof(DWORD) + sizeof(WCHAR) * cchSysLine;

    *pcchSysLine = cchSysLine;

    return pSystemLine;
}

DWORD
IntGetSystemLineCHS(
    LPCANDIDATELIST pCandList,
    LPWSTR pszText,
    LPBYTE pbAttrs,
    DWORD cchSysLine,
    PCONSOLE_ENTRY pEntry,
    BOOL bOpen)
{
    return 0; /* FIXME */
}

DWORD
IntGetSystemLineJpnOrKor(
    LPCANDIDATELIST pCandList,
    LPWSTR pszText,
    LPBYTE pbAttrs,
    DWORD cchSysLine,
    PCONSOLE_ENTRY pEntry,
    BOOL bOpen)
{
    return 0; /* FIXME */
}

DWORD
IntGetSystemLineCHT(
    LPCANDIDATELIST pCandList,
    LPWSTR pszText,
    LPBYTE pbAttrs,
    DWORD cchSysLine,
    PCONSOLE_ENTRY pEntry,
    BOOL bOpen)
{
    return 0; /* FIXME */
}

VOID IntSendCandListCHS(HWND hWnd, HIMC hIMC, PCONSOLE_ENTRY pEntry, LPARAM lParam, BOOL bOpen)
{
    DWORD dwIndex, cchSysLine, dwPageStart, dwPageSize;
    LPCANDIDATELIST pCandList;
    PCONIME_SYSTEMLINE pSystemLine;

    for (dwIndex = 0; dwIndex < 32; ++dwIndex)
    {
        if (!(lParam & (1 << dwIndex)))
            continue;

        pCandList = IntGetCandListFromEntry(pEntry, hIMC, dwIndex);
        if (!pCandList)
            break;

        pSystemLine = IntAllocSystemLine(pEntry, &cchSysLine);
        if (!pSystemLine)
            break;

        IntGetSystemLineCHS(pCandList,
                            pSystemLine->szText,
                            (LPBYTE)pSystemLine + pSystemLine->dwAttrOffset,
                            cchSysLine,
                            pEntry,
                            bOpen);

        dwPageStart = 0; /* FIXME */
        dwPageSize = 1; /* FIXME */

        pEntry->bSettingCandInfo = TRUE;

        ImmNotifyIME(hIMC, NI_SETCANDIDATE_PAGESTART, dwIndex, dwPageStart);
        ImmNotifyIME(hIMC, NI_SETCANDIDATE_PAGESIZE, dwIndex, dwPageSize);

        pEntry->bSettingCandInfo = FALSE;

        ConIme_SendImeStatus(hWnd);
        break;
    }
}

VOID IntSendCandListJpnOrKor(HWND hWnd, HIMC hIMC, PCONSOLE_ENTRY pEntry, LPARAM lParam, BOOL bOpen)
{
    DWORD dwIndex, cchSysLine, dwPageStart, dwPageSize;
    LPCANDIDATELIST pCandList;
    PCONIME_SYSTEMLINE pSystemLine;
    COPYDATASTRUCT CopyData;

    for (dwIndex = 0; dwIndex < 32; ++dwIndex)
    {
        if (!(lParam & (1 << dwIndex)))
            continue;

        pCandList = IntGetCandListFromEntry(pEntry, hIMC, dwIndex);
        if (!pCandList)
            break;

        pSystemLine = IntAllocSystemLine(pEntry, &cchSysLine);
        if (!pSystemLine)
            break;

        IntGetSystemLineJpnOrKor(pCandList,
                                 pSystemLine->szText,
                                 (LPBYTE)pSystemLine + pSystemLine->dwAttrOffset,
                                 cchSysLine,
                                 pEntry,
                                 bOpen);

        dwPageStart = 0; /* FIXME */
        dwPageSize = 1; /* FIXME */

        pEntry->bSettingCandInfo = TRUE;

        ImmNotifyIME(hIMC, NI_SETCANDIDATE_PAGESTART, dwIndex, dwPageStart);
        ImmNotifyIME(hIMC, NI_SETCANDIDATE_PAGESIZE, dwIndex, dwPageSize);

        pEntry->bSettingCandInfo = FALSE;

        CopyData.dwData = CONIME_COPYDATA_SEND_CAND_INFO;
        CopyData.cbData = sizeof(DWORD) + cchSysLine * (sizeof(WCHAR) + sizeof(BYTE));
        CopyData.lpData = pSystemLine;
        IntSendCopyDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);
        break;
    }
}

VOID IntSendCandListCHT(HWND hWnd, HIMC hIMC, PCONSOLE_ENTRY pEntry, LPARAM lParam, BOOL bOpen)
{
    DWORD dwIndex, cchSysLine, dwPageStart, dwPageSize;
    LPCANDIDATELIST pCandList;
    PCONIME_SYSTEMLINE pSystemLine;

    for (dwIndex = 0; dwIndex < 32; ++dwIndex)
    {
        if (!(lParam & (1 << dwIndex)))
            continue;

        pCandList = IntGetCandListFromEntry(pEntry, hIMC, dwIndex);
        if (!pCandList)
            break;

        pSystemLine = IntAllocSystemLine(pEntry, &cchSysLine);
        if (!pSystemLine)
            break;

        IntGetSystemLineCHT(pCandList,
                            pSystemLine->szText,
                            (LPBYTE)pSystemLine + pSystemLine->dwAttrOffset,
                            cchSysLine,
                            pEntry,
                            bOpen);

        dwPageStart = 0; /* FIXME */
        dwPageSize = 1; /* FIXME */

        pEntry->bSettingCandInfo = TRUE;

        ImmNotifyIME(hIMC, NI_SETCANDIDATE_PAGESTART, dwIndex, dwPageStart);
        ImmNotifyIME(hIMC, NI_SETCANDIDATE_PAGESIZE, dwIndex, dwPageSize);

        pEntry->bSettingCandInfo = FALSE;

        ConIme_SendImeStatus(hWnd);
        break;
    }
}

/* IMN_OPENCANDIDATE / IMN_CHANGECANDIDATE */
BOOL ConIme_OnNotifyCandidate(HWND hWnd, LPARAM lParam, BOOL bOpen)
{
    PCONSOLE_ENTRY pEntry;
    HIMC hIMC;
    LANGID wLang;

    pEntry = IntFindConsoleEntry(g_ConsoleHandle);
    if (pEntry->bSettingCandInfo)
        return TRUE;

    hIMC = ImmGetContext(hWnd);
    if (!hIMC)
        return FALSE;

    wLang = LOWORD(pEntry->hKL);

    pEntry->bHasCands = TRUE;
    switch (wLang)
    {
        case LANGID_CHINESE_SIMPLIFIED:
            IntSendCandListCHS(hWnd, hIMC, pEntry, lParam, bOpen);
            break;
        case LANGID_JAPANESE:
        case LANGID_KOREAN:
            IntSendCandListJpnOrKor(hWnd, hIMC, pEntry, lParam, bOpen);
            break;
        case LANGID_CHINESE_TRADITIONAL:
            IntSendCandListCHT(hWnd, hIMC, pEntry, lParam, bOpen);
            break;
        default:
            break;
    }

    ImmReleaseContext(hWnd, hIMC);

    return TRUE;
}

BOOL IntIsImeOpen(HIMC hIMC, PCONSOLE_ENTRY pEntry)
{
    switch (LOWORD(pEntry->hKL))
    {
        case LANGID_CHINESE_SIMPLIFIED:
        case LANGID_CHINESE_TRADITIONAL:
        case LANGID_KOREAN:
            break;

        case LANGID_JAPANESE:
            return ImmGetOpenStatus(hIMC);

        default:
            return FALSE;
    }

    if (!ImmGetOpenStatus(hIMC))
        return FALSE;

    return ImmIsIME(pEntry->hKL);
}

/* IMN_SETOPENSTATUS */
BOOL ConIme_OnNotifySetOpenStatus(HWND hWnd)
{
    PCONSOLE_ENTRY pEntry;
    HIMC hIMC;
    BOOL ret;
    PIME_STATUS pImeStatus;
    COPYDATASTRUCT CopyData;

    pEntry = IntFindConsoleEntry(g_ConsoleHandle);
    if (!pEntry)
        return FALSE;

    hIMC = ImmGetContext(hWnd);
    if (!hIMC)
        return FALSE;

    ret = FALSE;
    pEntry->bOpened = IntIsImeOpen(hIMC, pEntry);
    ImmGetConversionStatus(hIMC, &pEntry->dwConversion, &pEntry->dwSentence);
    if (pEntry->dwCoord.X)
    {
        pImeStatus = LocalAlloc(LPTR, sizeof(IME_STATUS));
        if (pImeStatus)
        {
            CopyData.dwData = CONIME_COPYDATA_SEND_IME_STATUS;
            CopyData.cbData = sizeof(IME_STATUS);
            CopyData.lpData = pImeStatus;
            if (IntFillImeStatus(pEntry, pImeStatus))
                IntSendCopyDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);
            LocalFree(pImeStatus);
        }
        ret = TRUE;
    }

    ImmReleaseContext(hWnd, hIMC);
    return ret;
}

/* IMN_GUIDELINE */
BOOL ConIme_OnNotifyGuideLine(HWND hWnd)
{
    BOOL ret = FALSE;
    DWORD cbText;
    HIMC hIMC;
    PCONSOLE_ENTRY pEntry;
    COPYDATASTRUCT CopyData;
    LPWSTR pszText;

    pEntry = IntFindConsoleEntry(g_ConsoleHandle);
    if (!pEntry)
        return FALSE;

    hIMC = ImmGetContext(hWnd);
    if (!hIMC)
        return FALSE;

    ImmGetGuideLineW(hIMC, GGL_LEVEL, 0, 0);
    ImmGetGuideLineW(hIMC, GGL_INDEX, 0, 0);
    cbText = ImmGetGuideLineW(hIMC, GGL_STRING, 0, 0);
    if (!cbText) /* No guideline string */
    {
        CopyData.dwData = CONIME_COPYDATA_SEND_GUIDELINE;
        CopyData.cbData = 0;
        CopyData.lpData = NULL;
        IntSendCopyDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);
        ret = TRUE;
        goto Quit;
    }
    cbText += sizeof(WCHAR);

    pszText = LocalAlloc(LPTR, cbText);
    if (pszText) /* There is a guideline string */
    {
        CopyData.dwData = CONIME_COPYDATA_SEND_GUIDELINE;
        CopyData.cbData = cbText;
        CopyData.lpData = pszText;
        ImmGetGuideLineW(hIMC, GGL_STRING, pszText, cbText);
        IntSendCopyDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);
        LocalFree(pszText);
        ret = TRUE;
    }

Quit:
    ImmReleaseContext(hWnd, hIMC);
    return ret;
}

/* WM_IME_NOTIFY */
BOOL ConIme_OnImeNotify(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
        case IMN_OPENSTATUSWINDOW:
            ConIme_SendImeStatus(hWnd);
            break;
        case IMN_CHANGECANDIDATE:
            ConIme_OnNotifyCandidate(hWnd, lParam, FALSE);
            break;
        case IMN_CLOSECANDIDATE:
            ConIme_OnNotifyCloseCandidate(hWnd, (DWORD)lParam);
            break;
        case IMN_OPENCANDIDATE:
            ConIme_OnNotifyCandidate(hWnd, lParam, TRUE);
            break;
        case IMN_SETCONVERSIONMODE:
            ConIme_SendImeStatus(hWnd);
            return FALSE;
        case IMN_SETOPENSTATUS:
            ConIme_OnNotifySetOpenStatus(hWnd);
            return FALSE;
        case IMN_GUIDELINE:
            ConIme_OnNotifyGuideLine(hWnd);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

/* WM_IME_STARTCOMPOSITION */
VOID ConIme_OnImeStartComposition(HWND hWnd)
{
    PCONSOLE_ENTRY pEntry = IntFindConsoleEntry(g_ConsoleHandle);
    if (pEntry)
        pEntry->bInComposition = TRUE;
}

VOID IntDoImeCompJPN(HWND hWnd, PCONSOLE_ENTRY pEntry, DWORD dwFlags)
{
    FIXME("\n");
}

VOID IntDoImeCompCHS(HWND hWnd, PCONSOLE_ENTRY pEntry, DWORD dwFlags)
{
    FIXME("\n");
}

VOID IntDoImeCompKOR(HWND hWnd, PCONSOLE_ENTRY pEntry, DWORD dwFlags, WPARAM wParam)
{
    FIXME("\n");
}

VOID IntDoImeCompCHT(HWND hWnd, PCONSOLE_ENTRY pEntry, DWORD dwFlags)
{
    FIXME("\n");
}

VOID IntDoImeComp(HWND hWnd, DWORD dwFlags, WPARAM wParam)
{
    PCONSOLE_ENTRY pEntry = IntFindConsoleEntry(g_ConsoleHandle);
    if (!pEntry)
        return;

    switch (pEntry->wOutputCodePage)
    {
        case CP_JAPANESE:            IntDoImeCompJPN(hWnd, pEntry, dwFlags); break;
        case CP_CHINESE_SIMPLIFIED:  IntDoImeCompCHS(hWnd, pEntry, dwFlags); break;
        case CP_KOREAN:              IntDoImeCompKOR(hWnd, pEntry, dwFlags, wParam); break;
        case CP_CHINESE_TRADITIONAL: IntDoImeCompCHT(hWnd, pEntry, dwFlags); break;
        default: break;
    }
}

/* WM_IME_COMPOSITION */
VOID ConIme_OnImeComposition(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    if (!lParam)
        IntDoImeComp(hWnd, 0, wParam);
    if (lParam & GCS_RESULTSTR)
        IntDoImeComp(hWnd, lParam & GCS_RESULTSTR, wParam);
    if (lParam & GCS_COMPSTR)
        IntDoImeComp(hWnd, lParam & (GCS_COMPSTR | GCS_COMPATTR), wParam);
    if (lParam & 0x2000)
        IntDoImeComp(hWnd, lParam & 0x2010, wParam);
    if (lParam & 0x4000)
        IntDoImeComp(hWnd, lParam & 0x4010, wParam);
}

/* WM_IME_ENDCOMPOSITION */
VOID ConIme_OnImeEndComposition(HWND hWnd)
{
    PCONSOLE_ENTRY pEntry = IntFindConsoleEntry(g_ConsoleHandle);
    if (!pEntry)
        return;

    pEntry->bInComposition = FALSE;
    pEntry->pCompStr = LocalFree(pEntry->pCompStr);
}

/* (WM_KEYFIRST + 0x800 == 0x900) ... */
LRESULT ConIme_On0x900Plus(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret;
    UINT uVK;
    CHAR ach;
    WCHAR wch;
    WPARAM wParam2;
    UINT uMsg2;

    wParam2 = wParam;

    if (HIWORD(wParam))
    {
        switch (uMsg)
        {
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
                wParam2 = 0;
                break;

            default: /* WM_CHAR, WM_SYSCHAR, ... */
            {
                if (HIWORD(wParam) <= 0xFF)
                {
                    wParam2 = HIWORD(wParam);
                }
                else
                {
                    wch = HIWORD(wParam);
                    WideCharToMultiByte(CP_OEMCP, 0, &wch, 1, &ach, 1, NULL, NULL);
                    wParam2 = ach;
                }
                break;
            }
        }
    }
    else
    {
        wParam2 = wParam;
    }

    uMsg2 = (((LONG)lParam < 0) ? WM_KEYUP : WM_KEYDOWN);
    ret = ImmCallImeConsoleIME(hWnd, uMsg2, wParam, lParam, &uVK);

    if (!(ret & IPHK_HOTKEY))
    {
        if (ret & IPHK_PROCESSBYIME)
            return ImmTranslateMessage(hWnd, uMsg2, wParam, lParam);
        else if ((ret & IPHK_CHECKCTRL) || uMsg == (0x800 + WM_CHAR) || uMsg == (0x800 + WM_SYSCHAR))
            return ConIme_OnKeyChar(hWnd, uMsg - 0x800, wParam2, lParam);
        else
            return ConIme_OnKeyChar(hWnd, uMsg - 0x800, wParam, lParam);
    }

    return ret;
}

BOOL IntGrowEntries(VOID)
{
    PUSER_DATA pOldData;
    INT cNewCount = g_cEntries + 5;
    PUSER_DATA pUserData = LocalAlloc(LPTR, cNewCount * sizeof(PCONSOLE_ENTRY));
    if (!pUserData)
        return FALSE;

    CopyMemory(pUserData, g_pUserData, g_cEntries * sizeof(PCONSOLE_ENTRY));
    pOldData = g_pUserData;
    g_pUserData = pUserData;
    LocalFree(pOldData);

    g_cEntries = cNewCount;
    return TRUE;
}

BOOL ConIme_UnInit(HWND hWnd, PCONSOLE_ENTRY pEntry)
{
    HIMC hIMC;
    INT i;
    LPVOID *ppData;

    if (!pEntry->bConsoleEnabled)
    {
        pEntry->bWndEnabled = 1;
        return TRUE;
    }

    hIMC = pEntry->hIMC;
    pEntry->ConsoleHandle = NULL;
    pEntry->wInputCodePage = 0;
    pEntry->wOutputCodePage = 0;
    pEntry->hKL = NULL;
    ImmDestroyContext(hIMC);

    pEntry->pCompStr = LocalFree(pEntry->pCompStr);

    ppData = (LPVOID*)pEntry->apCandList;
    for (i = 0; i < _countof(pEntry->apCandList); ++i)
    {
        ppData[i] = LocalFree(ppData[i]);
    }

    pEntry->pLocal2 = LocalFree(pEntry->pLocal2);
    pEntry->pKLInfo = LocalFree(pEntry->pKLInfo);

    pEntry->bConsoleEnabled = FALSE;
    pEntry->bWndEnabled = 0;

    return TRUE;
}

BOOL ConIme_Init(HWND hWnd, HANDLE ConsoleHandle, HWND hwndConsole)
{
    PCONSOLE_ENTRY pEntry;
    PKLINFO pKLInfo;
    UINT i;
    HIMC hIMC;

    for (i = 1; ; ++i)
    {
        while (i >= g_cEntries)
        {
            if (!IntGrowEntries())
                return FALSE;
        }

        pEntry = g_pUserData->apEntries[i];
        if (!pEntry)
        {
            pEntry = (PCONSOLE_ENTRY)LocalAlloc(LPTR, sizeof(CONSOLE_ENTRY));
            if (!pEntry)
                return FALSE;
            g_pUserData->apEntries[i] = pEntry;
        }

        if (!pEntry->ConsoleHandle)
            break;

        if (pEntry->bWndEnabled)
        {
            if (pEntry->bConsoleEnabled)
                ConIme_UnInit(hWnd, pEntry);
        }

        if (!pEntry->ConsoleHandle)
            break;
    }

    ZeroMemory(pEntry, sizeof(CONSOLE_ENTRY));

    pKLInfo = LocalAlloc(LPTR, sizeof(KLINFO));
    pEntry->pKLInfo = pKLInfo;
    if (!pKLInfo)
        return FALSE;

    pKLInfo->hKL = NULL;
    pKLInfo->dwImeState = 0;

    pEntry->cKLs = 1;
    hIMC = ImmCreateContext();
    pEntry->hOldIMC = hIMC;
    if (!hIMC)
    {
        LocalFree(pEntry);
        return FALSE;
    }

    pEntry->hIMC = hIMC;
    pEntry->ConsoleHandle = ConsoleHandle;
    pEntry->hwndConsole = hwndConsole;
    pEntry->bConsoleEnabled = TRUE;
    pEntry->dwCoord.X = 80;
    pEntry->unknown2[0] = 0x8007;
    pEntry->unknown2[1] = 0x17;
    pEntry->unknown2[2] = 0x8007;
    pEntry->unknown2[3] = 0x8071;
    pEntry->unknown2[4] = 0x8004;
    pEntry->unknown2[5] = 0x8004;
    pEntry->unknown2[6] = 0x8004;
    pEntry->unknown2[7] = 0x8004;
    IntGetLayoutText(pEntry);
    return TRUE;
}

/* CONIMEM_INIT */
BOOL ConIme_OnInit(HWND hWnd, HANDLE ConsoleHandle, HWND hwndConsole)
{
    BOOL ret = FALSE;

    if (IntFindConsoleEntry(ConsoleHandle))
        return TRUE;

    RtlEnterCriticalSection(&g_csLock);
    if (ConIme_Init(hWnd, ConsoleHandle, hwndConsole))
    {
        ConIme_OnNotifySetOpenStatus(hwndConsole);
        ret = TRUE;
    }
    RtlLeaveCriticalSection(&g_csLock);
    return ret;
}

/* CONIMEM_UNINIT */
BOOL ConIme_OnUnInit(HWND hWnd, HANDLE ConsoleHandle)
{
    BOOL ret = FALSE;
    PCONSOLE_ENTRY pEntry;

    RtlEnterCriticalSection(&g_csLock);

    pEntry = IntFindConsoleEntry(ConsoleHandle);
    if (pEntry)
        ret = ConIme_UnInit(hWnd, pEntry);

    RtlLeaveCriticalSection(&g_csLock);
    return ret;
}

VOID IntSendConversionStatusCHT(HWND hWnd, PCONSOLE_ENTRY pEntry)
{
    COPYDATASTRUCT CopyData;
    PIME_STATUS pImeStatus = LocalAlloc(LPTR, sizeof(IME_STATUS));
    if (!pImeStatus)
        return;

    CopyData.cbData = sizeof(IME_STATUS);
    CopyData.dwData = CONIME_COPYDATA_SEND_IME_STATUS;
    CopyData.lpData = pImeStatus;
    if (IntFillImeStatusCHT(pEntry, pImeStatus))
        IntSendCopyDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);
    LocalFree(pImeStatus);
}

VOID IntSendConversionStatusJpnOrKor(HWND hWnd, PCONSOLE_ENTRY pEntry)
{
    COPYDATASTRUCT CopyData;
    CopyData.dwData = CONIME_COPYDATA_SEND_COMPSTR;
    CopyData.lpData = pEntry->pCompStr;
    CopyData.cbData = pEntry->pCompStr->cbSize;
    IntSendCopyDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);
}

VOID IntSendConversionStatusCHS(HWND hWnd, PCONSOLE_ENTRY pEntry)
{
    COPYDATASTRUCT CopyData;
    PIME_STATUS pImeStatus = LocalAlloc(LPTR, sizeof(IME_STATUS));
    if (!pImeStatus)
        return;

    CopyData.cbData = sizeof(IME_STATUS);
    CopyData.dwData = CONIME_COPYDATA_SEND_IME_STATUS;
    CopyData.lpData = pImeStatus;
    if (IntFillImeStatusCHS(pEntry, pImeStatus))
        IntSendCopyDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);
    LocalFree(pImeStatus);
}

VOID IntSendConversionStatus(HWND hWnd)
{
    PCONSOLE_ENTRY pEntry = IntFindConsoleEntry(g_ConsoleHandle);
    if (pEntry && pEntry->bInComposition)
    {
        switch (LOWORD(pEntry->hKL))
        {
            case LANGID_CHINESE_TRADITIONAL:
                IntSendConversionStatusCHT(hWnd, pEntry);
                break;
            case LANGID_JAPANESE:
            case LANGID_KOREAN:
                IntSendConversionStatusJpnOrKor(hWnd, pEntry);
                break;
            case LANGID_CHINESE_SIMPLIFIED:
                IntSendConversionStatusCHS(hWnd, pEntry);
                break;
        }
    }
}

/* CONIMEM_SET_FOCUS */
BOOL ConIme_OnSetFocus(HWND hWnd, HANDLE ConsoleHandle, HKL hKL)
{
    PCONSOLE_ENTRY pEntry;

    pEntry = IntFindConsoleEntry(ConsoleHandle);
    if (!pEntry)
        return FALSE;

    if (g_bInActive)
        g_bInActive = FALSE;

    pEntry->hKL = hKL;
    ActivateKeyboardLayout(hKL, 0);

    ImmAssociateContext(hWnd, pEntry->hOldIMC);

    if (!hKL)
    {
        IntGetLayoutText(pEntry);
        pEntry->dwImeProp = ImmGetProperty(pEntry->hKL, IGP_PROPERTY);
    }

    ImmSetActiveContextConsoleIME(hWnd, TRUE);
    g_ConsoleHandle = ConsoleHandle;
    ConIme_OnNotifySetOpenStatus(hWnd);

    if (pEntry->pCompStr)
        IntSendConversionStatus(hWnd);

    return TRUE;
}

/* CONIMEM_KILL_FOCUS */
BOOL ConIme_OnKillFocus(HWND hWnd, HANDLE ConsoleHandle)
{
    PCONSOLE_ENTRY pEntry = IntFindConsoleEntry(ConsoleHandle);
    if (!pEntry)
        return FALSE;

    if (g_bInActive)
    {
        g_bInActive = FALSE;
    }
    else
    {
        ImmSetActiveContextConsoleIME(hWnd, 0);
        ImmAssociateContext(hWnd, g_hOldIMC);
    }

    return TRUE;
}

/* CONIMEM_GET_IME_STATE */
DWORD IntGetImeState(HWND hWnd, HANDLE ConsoleHandle)
{
    HIMC hIMC;
    PCONSOLE_ENTRY pEntry;

    pEntry = IntFindConsoleEntry(ConsoleHandle);
    if (!pEntry)
        return 0;

    hIMC = ImmGetContext(hWnd);
    if (!hIMC)
        return IME_STATE_DISABLED;

    ImmGetConversionStatus(hIMC, &pEntry->dwConversion, &pEntry->dwSentence);
    pEntry->bOpened = IntIsImeOpen(hIMC, pEntry);
    ImmReleaseContext(hWnd, hIMC);

    return pEntry->dwConversion | (pEntry->bOpened ? IME_STATE_OPENED : 0);
}

/* CONIMEM_SET_IME_STATE */
BOOL IntSetImeState(HWND hWnd, HANDLE ConsoleHandle, DWORD dwImeState)
{
    HIMC hIMC;
    BOOL bOpen;
    PCONSOLE_ENTRY pEntry;
    DWORD dwConversion, dwSentence;

    pEntry = IntFindConsoleEntry(ConsoleHandle);
    if (!pEntry)
        return FALSE;

    g_ConsoleHandle = pEntry->ConsoleHandle;
    if (dwImeState & IME_STATE_DISABLED)
    {
        ImmSetActiveContextConsoleIME(hWnd, FALSE);
        ImmAssociateContext(hWnd, NULL);
        pEntry->hOldIMC = NULL;
    }
    else
    {
        ImmAssociateContext(hWnd, pEntry->hIMC);
        ImmSetActiveContextConsoleIME(hWnd, TRUE);
        pEntry->hOldIMC = pEntry->hIMC;
    }

    hIMC = ImmGetContext(hWnd);
    if (!hIMC)
        return TRUE;

    bOpen = !!(dwImeState & IME_STATE_OPENED);
    pEntry->bOpened = bOpen;
    ImmSetOpenStatus(hIMC, bOpen);

    dwConversion = (dwImeState & ~(IME_STATE_OPENED | IME_STATE_DISABLED));
    dwSentence = pEntry->dwSentence;
    if (pEntry->dwConversion != dwConversion)
    {
        pEntry->dwConversion = dwConversion;
        ImmSetConversionStatus(hIMC, dwConversion, dwSentence);
    }

    ImmReleaseContext(hWnd, hIMC);
    return TRUE;
}

/* CONIMEM_SET_SCREEN_SIZE */
BOOL ConIme_OnSetScreenSize(HWND hWnd, HANDLE ConsoleHandle, DWORD dwCoord)
{
    PCONSOLE_ENTRY pEntry = IntFindConsoleEntry(ConsoleHandle);
    if (!pEntry)
        return FALSE;
    pEntry->dwCoord = *(COORD*)&dwCoord;
    return TRUE;
}

/* CONIMEM_LANGUAGE_CHANGE */
VOID ConIme_OnLangChange(HWND hWnd, HANDLE ConsoleHandle, HKL hNewKL)
{
    PIME_STATUS pImeStatus;
    COPYDATASTRUCT CopyData;
    PCONSOLE_ENTRY pEntry;
    PKLINFO phKLs, pKLInfo, phNewKLs;
    HKL hKL;
    INT iKL, cKLs;
    DWORD dwImeState;

    pEntry = IntFindConsoleEntry(ConsoleHandle);
    if (!pEntry)
        pEntry = IntFindConsoleEntry(g_ConsoleHandle);
    if (!pEntry)
        return;

    phKLs = pEntry->pKLInfo;
    if (!phKLs)
        return;

    hKL = pEntry->hKL;
    if (IS_IME_HKL(hKL))
    {
        cKLs = pEntry->cKLs;
        for (iKL = 0; iKL < cKLs; ++iKL)
        {
            if (!phKLs->hKL)
                break;
            if (phKLs->hKL == hKL)
                break;
        }

        if (iKL >= cKLs)
        {
            phNewKLs = LocalAlloc(LPTR, (cKLs + 1) * sizeof(KLINFO));
            if (!phNewKLs)
                return;

            CopyMemory(phNewKLs, pEntry->pKLInfo, pEntry->cKLs * sizeof(KLINFO));

            ++pEntry->cKLs;
            LocalFree(pEntry->pKLInfo);
            pEntry->pKLInfo = phNewKLs;
        }

        pEntry->pKLInfo[iKL].hKL = pEntry->hKL;

        dwImeState = pEntry->dwConversion;
        if (pEntry->bOpened)
            dwImeState |= IME_STATE_OPENED;
        pEntry->pKLInfo[iKL].dwImeState = dwImeState;
    }

    ActivateKeyboardLayout(hNewKL, 0);
    pEntry->hKL = hNewKL;
    IntGetLayoutText(pEntry);

    ConIme_SendImeStatus(hWnd);
    pEntry->dwImeProp = ImmGetProperty(pEntry->hKL, IGP_PROPERTY);

    pImeStatus = LocalAlloc(LPTR, sizeof(IME_STATUS));
    if (!pImeStatus)
        return;

    CopyData.dwData = CONIME_COPYDATA_SEND_IME_STATUS;
    CopyData.cbData = sizeof(IME_STATUS);
    CopyData.lpData = pImeStatus;
    if (IS_IME_HKL(hNewKL))
    {
        for (iKL = 0; iKL < pEntry->cKLs; ++iKL)
        {
            pKLInfo = &pEntry->pKLInfo[iKL];
            if (pKLInfo->hKL != hNewKL)
                continue;

            IntSetImeState(hWnd, ConsoleHandle, pKLInfo->dwImeState);
            ConIme_SendImeStatus(hWnd);
            if (IntFillImeStatus(pEntry, pImeStatus))
                IntSendCopyDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);
            break;
        }
    }
    else
    {
        IntSetImeState(hWnd, ConsoleHandle, (pEntry->dwConversion & ~IME_STATE_OPENED));
        pImeStatus->cch = 0;
        pImeStatus->unknown0 = 1;
        IntSendCopyDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);
    }

    LocalFree(pImeStatus);
}

/* CONIMEM_SET_CODEPAGE */
BOOL Console_OnSetCodepage(HWND hWnd, HANDLE ConsoleHandle, BOOL bOutput, WORD wCodepage)
{
    PCONSOLE_ENTRY pEntry = IntFindConsoleEntry(ConsoleHandle);
    if (!pEntry)
        return FALSE;

    if (bOutput)
        pEntry->wOutputCodePage = wCodepage;
    else
        pEntry->wInputCodePage = wCodepage;

    return TRUE;
}

/* CONIMEM_GO, CONIMEM_GO_NEXT, CONIMEM_GO_PREV */
BOOL ConIme_OnGo(HWND hWnd, HANDLE ConsoleHandle, HKL hKL, INT iDirection)
{
    LANGID wLang;
    UINT iKL, cKLs, i;
    PCONSOLE_ENTRY pEntry;
    HKL *phKLs;

    pEntry = IntFindConsoleEntry(ConsoleHandle);
    if (!pEntry)
        return FALSE;

    /* Get the language ID from codepage */
    switch (pEntry->wOutputCodePage)
    {
        case CP_CHINESE_SIMPLIFIED:  wLang = LANGID_CHINESE_SIMPLIFIED; break;
        case CP_JAPANESE:            wLang = LANGID_JAPANESE; break;
        case CP_KOREAN:              wLang = LANGID_KOREAN; break;
        case CP_CHINESE_TRADITIONAL: wLang = LANGID_CHINESE_TRADITIONAL; break;
        default:                     wLang = 0; break;
    }

    if (!IS_IME_HKL(hKL) || LOWORD(hKL) == wLang)
        return TRUE; /* Non-IME or same language */

    if (iDirection == 0)
        return FALSE;

    /* Get the keyboard layout list */
    cKLs = GetKeyboardLayoutList(0, NULL);
    if (!cKLs)
        return FALSE;

    phKLs = LocalAlloc(LPTR, cKLs * sizeof(HKL));
    if (!phKLs)
        return FALSE;

    GetKeyboardLayoutList(cKLs, phKLs);

    /* Find the keyboard layout */
    for (iKL = 0; iKL < cKLs; ++iKL)
    {
        if (pEntry->hKL == phKLs[iKL])
            break;
    }

    /* Set the next keyboard layout */
    if (iKL < cKLs)
    {
        for (i = 0; i < cKLs; ++i)
        {
            iKL += iDirection;
            if (iKL >= 0)
            {
                if (iKL >= cKLs)
                    iKL = 0;
            }
            else
            {
                iKL = cKLs - 1;
            }

            if (!(HIWORD(phKLs[iKL]) & 0xF000) || LOWORD(phKLs[iKL]) == wLang)
            {
                PostMessageW(pEntry->hwndConsole, PM_SET_HKL, (WPARAM)phKLs[iKL], 0);
                break;
            }
        }
    }

    LocalFree(phKLs);
    return FALSE;
}

/* The window procedure */
LRESULT CALLBACK
ConIme_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            return ConIme_OnCreate(hWnd);

        case WM_QUERYENDSESSION:
            return TRUE;

        case WM_ENDSESSION:
            ConIme_OnEnd(hWnd, uMsg);
            break;

        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;

        case WM_DESTROY:
            ConIme_OnEnd(hWnd, uMsg);
            PostQuitMessage(0);
            break;

        case WM_ENABLE:
            return ConIme_OnEnable(hWnd, wParam, lParam);

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
        case WM_DEADCHAR:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_SYSCHAR:
        case WM_SYSDEADCHAR:
            return ConIme_OnKeyChar(hWnd, uMsg, wParam, lParam);

        case WM_INPUTLANGCHANGEREQUEST:
            return ConIme_OnInputLangChangeRequest(hWnd, wParam, lParam);

        case WM_INPUTLANGCHANGE:
            ConIme_OnInputLangChange(hWnd, wParam, lParam);
            return TRUE;

        case WM_IME_SYSTEM:
            if (wParam == 0x1A || wParam == 0x1B)
            {
                ConIme_OnImeSystem(hWnd, wParam, lParam);
                return TRUE;
            }
            goto DoDefault;

        case WM_IME_SETCONTEXT:
            /* Don't display CompForm, GuideLine, CandForms */
            lParam &= ~(ISC_SHOWUICOMPOSITIONWINDOW | ISC_SHOWUIGUIDELINE |
                        ISC_SHOWUIALLCANDIDATEWINDOW);
            goto DoDefault;

        case WM_IME_NOTIFY:
            if (!ConIme_OnImeNotify(hWnd, wParam, lParam))
                goto DoDefault;
            /* Don't display CompForm, GuideLine, CandForms */
            lParam &= ~(ISC_SHOWUICOMPOSITIONWINDOW | ISC_SHOWUIGUIDELINE |
                        ISC_SHOWUIALLCANDIDATEWINDOW);
            goto DoDefault;

        case WM_IME_STARTCOMPOSITION:
            ConIme_OnImeStartComposition(hWnd);
            return TRUE;

        case WM_IME_COMPOSITION:
            ConIme_OnImeComposition(hWnd, wParam, lParam);
            return TRUE;

        case WM_IME_ENDCOMPOSITION:
            ConIme_OnImeEndComposition(hWnd);
            return TRUE;

        case WM_IME_COMPOSITIONFULL:
            return TRUE;

        case CONIMEM_INIT: /* == 0x400 */
            return ConIme_OnInit(hWnd, (HANDLE)wParam, (HWND)lParam);

        case CONIMEM_UNINIT:
            return ConIme_OnUnInit(hWnd, (HANDLE)wParam);

        case CONIMEM_SET_FOCUS:
            return ConIme_OnSetFocus(hWnd, (HANDLE)wParam, (HKL)lParam);

        case CONIMEM_KILL_FOCUS:
            return ConIme_OnKillFocus(hWnd, (HANDLE)wParam);

        case CONIMEM_SIMULATE_KEY:
            return ImmSimulateHotKey(hWnd, (DWORD)lParam);

        case CONIMEM_GET_IME_STATE:
            return IntGetImeState(hWnd, (HANDLE)wParam);

        case CONIMEM_SET_IME_STATE:
            return IntSetImeState(hWnd, (HANDLE)wParam, (DWORD)lParam);

        case CONIMEM_SET_SCREEN_SIZE:
            return ConIme_OnSetScreenSize(hWnd, (HANDLE)wParam, (DWORD)lParam);

        case CONIMEM_SEND_IME_STATUS:
            return ConIme_SendImeStatus(hWnd);

        case CONIMEM_LANGUAGE_CHANGE:
            ConIme_OnLangChange(hWnd, (HANDLE)wParam, (HKL)lParam);
            return TRUE;

        case CONIMEM_SET_CODEPAGE:
            return Console_OnSetCodepage(hWnd, (HANDLE)wParam, LOWORD(lParam), HIWORD(lParam));

        case CONIMEM_GO:
            return ConIme_OnGo(hWnd, (HANDLE)wParam, (HKL)lParam, 0);

        case CONIMEM_GO_NEXT:
            return ConIme_OnGo(hWnd, (HANDLE)wParam, (HKL)lParam, 1);

        case CONIMEM_GO_PREV:
            return ConIme_OnGo(hWnd, (HANDLE)wParam, (HKL)lParam, -1);

        case WM_KEYDOWN + 0x800: /* == 0x900 */
        case WM_KEYUP + 0x800:
        case WM_CHAR + 0x800:
        case WM_DEADCHAR + 0x800:
        case WM_SYSKEYDOWN + 0x800:
        case WM_SYSKEYUP + 0x800:
        case WM_SYSCHAR + 0x800:
            ConIme_On0x900Plus(hWnd, uMsg, wParam, lParam);
            return TRUE;

#ifdef DEBUGOPTIONS
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case ID_ABOUT_APP:
                    MessageBoxW(hWnd, L"Console IME", L"About this", MB_ICONINFORMATION);
                    break;

                case ID_EXIT_APP:
                    DestroyWindow(hWnd);
                    break;

                case ID_ACCESS_VIOLATION:
                    *(int*)NULL = 0;
                    break;

                default:
                    break;
            }
            break;
#endif

DoDefault:
        default:
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

/* Initialize the application */
BOOL ConIme_InitInstance(HINSTANCE hInstance)
{
    HKEY hKey;
    ATOM wAtom = 0;
    HWND hWnd = NULL;
    DWORD dwValue, cbValue;
    BOOL bConImeEnabledOnSystemProcess = FALSE;
    LONG error;
    HANDLE hEvent;
    WNDCLASSW wc;
    INT cxScreen, cyMenu;

    /* Open Console registry key */
    error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Console",
                          0, KEY_QUERY_VALUE, &hKey);
    if (!error)
    {
        /* Is Console IME enabled on system process? */
        dwValue = 0;
        cbValue = sizeof(dwValue);
        error = RegQueryValueExW(hKey, L"EnableConImeOnSystemProcess", NULL, NULL,
                                 (LPBYTE)&dwValue, &cbValue);
        if (!error)
            bConImeEnabledOnSystemProcess = (dwValue != 0);

        /* Close the key */
        RegCloseKey(hKey);
    }

    if (!bConImeEnabledOnSystemProcess && IntIsLogOnSession())
        return FALSE;

    RtlInitializeCriticalSection(&g_csLock);

    g_pUserData = LocalAlloc(LPTR, sizeof(USER_DATA));
    if (!g_pUserData)
        return FALSE;

    ZeroMemory(g_pUserData, sizeof(USER_DATA));
    g_cEntries = 10;

    /* Open the event object that already exists */
    hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, L"ConsoleIME_StartUp_Event");
    if (!hEvent)
        goto Failure;

    /* Register the main window class */
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = ConIme_WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_MAINICON));
    wc.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_ARROW));
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
#ifdef DEBUGOPTIONS
    wc.lpszMenuName = L"DEBUGMENU";
#endif
    wc.lpszClassName = L"ConsoleIMEClass";
    wAtom = RegisterClassW(&wc);
    if (!wAtom)
        goto Failure;

    /* Create the main window */
    cxScreen = GetSystemMetrics(SM_CXSCREEN);
    cyMenu = GetSystemMetrics(SM_CYMENU);
    hWnd = CreateWindowExW(0,
                           L"ConsoleIMEClass",
                           NULL,
                           WS_OVERLAPPEDWINDOW,
                           cxScreen - cxScreen / 3,
                           cyMenu,
                           cxScreen / 3,
                           10 * cyMenu,
                           NULL,
                           NULL,
                           hInstance,
                           NULL);
    if (!hWnd)
        goto Failure;

#ifdef DEBUGOPTIONS
    ShowWindow(hWnd, SW_SHOWNORMAL);
    UpdateWindow(hWnd);
#endif

    /* Register this window as a console IME. RegisterConsoleIME is a kernel32 function. */
    if (!RegisterConsoleIME(hWnd, &g_dwAttachToThreadId))
        goto Failure;

    /* Attach the thread input */
    if (AttachThreadInput(GetCurrentThreadId(), g_dwAttachToThreadId, TRUE))
    {
        /* Make the event signaled and close it */
        SetEvent(hEvent);
        CloseHandle(hEvent);
        return TRUE; /* Success */
    }

Failure:
    /* Unregister the console IME. UnregisterConsoleIME is a kernel32 function. */
    if (g_dwAttachToThreadId)
    {
        UnregisterConsoleIME();
        g_dwAttachToThreadId = 0;
    }

    if (hWnd)
        DestroyWindow(hWnd);

    if (wAtom)
        UnregisterClassW(L"ConsoleIMEClass", hInstance);

    if (hEvent)
    {
        SetEvent(hEvent);
        CloseHandle(hEvent);
    }

    return FALSE; /* Failure */
}

/* The Win32 main function */
INT WINAPI
wWinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPWSTR      lpCmdLine,
    INT         nCmdShow)
{
    WCHAR szDir[MAX_PATH];
    MSG msg;

    /* Make the current directory system32 */
    if (GetSystemDirectoryW(szDir, _countof(szDir)))
        SetCurrentDirectoryW(szDir);

    /* Disable TFS on this process */
    ImmDisableTextFrameService(-1);

    /* Initialize the application */
    if (!ConIme_InitInstance(hInstance))
        return 0;

    _SEH2_TRY
    {
        /* The message loop */
        while (GetMessageW(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        if (g_dwAttachToThreadId)
            UnregisterConsoleIME();

        msg.wParam = -1;
    }
    _SEH2_END;

    return (INT)msg.wParam;
}
