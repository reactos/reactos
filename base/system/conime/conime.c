/*
 * PROJECT:     ReactOS Console IME
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing Console IME Input for Far-East Asian
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winnls.h>
#include <winreg.h>
#include <wincon.h>
#include <imm.h>
#include <immdev.h>
#include <stdlib.h>
#include <wchar.h>
#include <undocuser.h>
#include <imm32_undoc.h>
#include <wincon_undoc.h>
#include <cjkcode.h>
#include <strsafe.h>

#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include "conime.h"
#include "resource.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(conime);

// Special values for COPYDATASTRUCT.dwData
#define MAGIC_SEND_COMPSTR 0x4B425930 // 'KBY0'
#define MAGIC_SEND_IMEDISPLAY 0x4B425931 // 'KBY1'
#define MAGIC_SEND_GUIDELINE 0x4B425932 // 'KBY2'
#define MAGIC_SEND_CANDLIST 0x4B425935 // 'KBY5'
#define MAGIC_SEND_IMESYSTEM 0x4B425936 // 'KBY6'

#define _FOREGROUND_WHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define _BACKGROUND_WHITE (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE)

// Global variables
HANDLE g_hConsole = NULL;
PCONENTRY* g_ppEntries = NULL; // begins from &g_ppEntries[1] (g_ppEntries[0] is ignored)
UINT g_cEntries = 0;
HIMC g_hOldIMC = NULL;
DWORD g_dwAttachToThreadId = 0;
BOOL g_bIsLogOnSession = FALSE;
BOOL g_bDisabled = FALSE;
CRITICAL_SECTION g_csLock;

void IntFormatNumber(PWSTR pszBuffer, UINT value, UINT width)
{
    UINT divisor = 1;
    if (width > 1)
    {
        UINT tempWidth = width - 1;
        do
        {
            divisor *= 10;
            --tempWidth;
        } while (tempWidth);
    }

    for (UINT i = divisor, ich = 0; i > 0; i /= 10, ++ich)
    {
        UINT digit = value / i;
        value %= i;

        if (digit == 0 && ich > 0)
        {
            WCHAR prevChar = pszBuffer[ich - 1];
            if (prevChar == L' ' || prevChar == L'/')
            {
                pszBuffer[ich] = L' ';
                continue;
            }
        }

        pszBuffer[ich] = (WCHAR)(L'0' + digit);
    }
}

//! Determines if a Unicode character should be rendered as "Double Width" (2 columns).
BOOL IntIsDoubleWidthChar(WCHAR wch)
{
    // Characters from SPACE (0x20) to TILDE (0x7E) are always single-width
    if (0x0020 <= wch && wch <= 0x007E)
        return FALSE;

    // East Asian scripts (Full-width blocks)
    if ((0x3041 <= wch && wch <= 0x3094) || // Hiragana
        (0x30A1 <= wch && wch <= 0x30F6) || // Katakana
        (0x3105 <= wch && wch <= 0x312C) || // Bopomofo
        (0x3131 <= wch && wch <= 0x318E) || // Hangul Compatibility Jamo
        (0xAC00 <= wch && wch <= 0xD7A3) || // Hangul Syllables
        (0xFF01 <= wch && wch <= 0xFF5E))   // Full-width Forms (e.g., Ａ, Ｂ, ！）
    {
        return TRUE;
    }

    // Half-width East Asian blocks
    if ((0xFF61 <= wch && wch <= 0xFF9F) || // Half-width Katakana
        (0xFFA0 <= wch && wch <= 0xFFBE) || // Half-width Hangul
        (0xFFC2 <= wch && wch <= 0xFFC7) || // Specific Half-width Jamo variants
        (0xFFCA <= wch && wch <= 0xFFCF) ||
        (0xFFD2 <= wch && wch <= 0xFFD7) ||
        (0xFFDA <= wch && wch <= 0xFFDC))
    {
        return FALSE;
    }

    // CJK Ideographs and supplemental Full-width symbols
    if ((0xFFE0 <= wch && wch <= 0xFFE6) || // Full-width currency/symbols (￠, ￡, ￥, etc.)
        (0x4E00 <= wch && wch <= 0x9FA5) || // CJK Unified Ideographs (Common Kanji/Hanzi)
        (0xF900 <= wch && wch <= 0xFA2D))   // CJK Compatibility Ideographs
    {
        return TRUE;
    }

    // Fallback for undefined ranges
    ULONG cbMultiByte = 0;
    NTSTATUS Status = RtlUnicodeToMultiByteSize(&cbMultiByte, &wch, sizeof(WCHAR));
    if (NT_SUCCESS(Status))
        return cbMultiByte == 2;

    return FALSE;
}

INT IntGetStringWidth(PCWSTR pch)
{
    INT cch = 0;
    while (*pch)
        cch += IntIsDoubleWidthChar(*pch++) + 1;
    return cch;
}

BOOL IntSendDataToConsole(HWND hwndConsole, HWND hwndSender, PCOPYDATASTRUCT pCopyData)
{
    TRACE("IntSendDataToConsole(%p, %p): 0x%lX, 0x%lX\n", hwndConsole, hwndSender,
          pCopyData->dwData, pCopyData->cbData);

    if (!hwndConsole || !IsWindow(hwndConsole))
    {
        ERR("%p is null or invalid window\n", hwndConsole);
        return FALSE;
    }

    DWORD_PTR result;
    if (!SendMessageTimeoutW(hwndConsole, WM_COPYDATA, (WPARAM)hwndSender, (LPARAM)pCopyData,
                             SMTO_ABORTIFHUNG, 3000, &result))
    {
        ERR("SendMessageTimeoutW(%p) failed\n", hwndConsole);
        return FALSE;
    }

    TRACE("IntSendDataToConsole -> %p\n", result);
    return TRUE;
}

//! Determines whether the current process is an interactive logon session (such as Winlogon)
//  and stores the result in a global variable.
BOOL IntIsLogOnSession(void)
{
    NTSTATUS Status;
    HANDLE hToken;
    ULONG returnLength;
    TOKEN_STATISTICS tokenStats;
    static const LUID systemLuid = SYSTEM_LUID;

    Status = NtOpenProcessToken(INVALID_HANDLE_VALUE, TOKEN_QUERY, &hToken);
    if (!NT_SUCCESS(Status))
        return g_bIsLogOnSession;

    Status = NtQueryInformationToken(hToken, TokenStatistics, &tokenStats, sizeof(tokenStats),
                                     &returnLength);
    NtClose(hToken);

    if (!NT_SUCCESS(Status))
        return g_bIsLogOnSession;

    g_bIsLogOnSession = RtlEqualMemory(&tokenStats.AuthenticationId, &systemLuid,
                                       sizeof(systemLuid));
    return g_bIsLogOnSession;
}

void IntFreeConsoleEntries(void)
{
    EnterCriticalSection(&g_csLock);

    if (g_ppEntries)
    {
        for (UINT iEntry = 1; iEntry < g_cEntries; ++iEntry)
        {
            PCONENTRY pEntry = g_ppEntries[iEntry];
            if (!pEntry)
                continue;

            if (!pEntry->hConsole)
            {
                LocalFree(pEntry);
                continue;
            }

            if (pEntry->bOpened)
            {
                pEntry->bOpened = FALSE;
                ImmSetOpenStatus(pEntry->hNewIMC, FALSE);
            }

            if (pEntry->hNewIMC)
                ImmDestroyContext(pEntry->hNewIMC);

            if (pEntry->pCompStr)
            {
                LocalFree(pEntry->pCompStr);
                pEntry->pCompStr = NULL;
            }

            for (UINT iList = 0; iList < MAX_CANDLIST; ++iList)
            {
                if (!pEntry->apCandList[iList])
                    continue;
                LocalFree(pEntry->apCandList[iList]);
                pEntry->apCandList[iList] = NULL;
                pEntry->acbCandList[iList] = 0;
            }

            if (pEntry->pdwCandPageStart)
            {
                LocalFree(pEntry->pdwCandPageStart);
                pEntry->pdwCandPageStart = NULL;
                pEntry->cbCandPageData = 0;
            }

            if (pEntry->pKLInfo)
            {
                LocalFree(pEntry->pKLInfo);
                pEntry->pKLInfo = NULL;
            }

            LocalFree(pEntry);
        }

        LocalFree(g_ppEntries);
        g_ppEntries = NULL;
        g_cEntries = 0;
    }

    LeaveCriticalSection(&g_csLock);
}

static void IntSetCurrentConsole(HANDLE hConsole)
{
    TRACE("g_hConsole: %p\n", hConsole);
    g_hConsole = hConsole;
}

//! Finds the CONENTRY structure corresponding to the specified console handle.
PCONENTRY IntFindConsoleEntry(HANDLE hConsole)
{
    EnterCriticalSection(&g_csLock);

    if (!g_hConsole)
        IntSetCurrentConsole(hConsole);

    PCONENTRY pFound = NULL;
    for (UINT iEntry = 1; iEntry < g_cEntries; ++iEntry)
    {
        PCONENTRY pEntry = g_ppEntries[iEntry];
        if (pEntry && pEntry->hConsole == hConsole && !pEntry->bWndEnabled)
        {
            pFound = pEntry;
            break;
        }
    }

    LeaveCriticalSection(&g_csLock);
    TRACE("hConsole %p --> PCONENTRY %p\n", hConsole, pFound);
    return pFound;
}

//! Terminate Console IME.
void ConIme_OnEnd(HWND hwnd, UINT uMsg)
{
    ImmAssociateContext(hwnd, g_hOldIMC);

    IntFreeConsoleEntries();

    if (g_dwAttachToThreadId && (uMsg != WM_ENDSESSION || !g_bIsLogOnSession))
    {
        AttachThreadInput(GetCurrentThreadId(), g_dwAttachToThreadId, FALSE);
        UnregisterConsoleIME();
        g_dwAttachToThreadId = 0;
    }
}

BOOL IntGrowEntries(void)
{
    size_t cGrow = 5;
    size_t cNewCount = g_cEntries + cGrow;
    size_t cbOld = sizeof(PCONENTRY) * g_cEntries;
    size_t cbNew = sizeof(PCONENTRY) * cNewCount;
    PCONENTRY* ppEntries = LocalAlloc(LPTR, cbNew);
    if (!ppEntries)
        return FALSE;

    CopyMemory(ppEntries, g_ppEntries, cbOld);

    PCONENTRY* ppOldEntries = g_ppEntries;
    g_ppEntries = ppEntries;
    g_cEntries = cNewCount;
    LocalFree(ppOldEntries);

    return TRUE;
}

//! Frees the resources associated with the console entry.
BOOL ConIme_UnInitEntry(HWND hwnd, PCONENTRY pEntry)
{
    TRACE("ConIme_UnInitEntry: %p\n", pEntry);

    if (!pEntry->bConsoleEnabled)
    {
        pEntry->bWndEnabled = TRUE;
        return TRUE;
    }

    pEntry->hConsole = NULL;
    pEntry->ScreenSize.X = 0;
    pEntry->nCodePage = pEntry->nOutputCodePage = CP_ACP;
    pEntry->hKL = NULL;
    ImmDestroyContext(pEntry->hNewIMC);

    // Free composition string info
    if (pEntry->pCompStr)
    {
        LocalFree(pEntry->pCompStr);
        pEntry->pCompStr = NULL;
    }

    // Free candidate list
    for (UINT iList = 0; iList < _countof(pEntry->apCandList); ++iList)
    {
        if (!pEntry->apCandList[iList])
            continue;
        LocalFree(pEntry->apCandList[iList]);
        pEntry->apCandList[iList] = NULL;
    }

    // Free candidate separators
    if (pEntry->pdwCandPageStart)
    {
        LocalFree(pEntry->pdwCandPageStart);
        pEntry->pdwCandPageStart = NULL;
    }

    // Free keyboard list info
    if (pEntry->pKLInfo)
    {
        LocalFree(pEntry->pKLInfo);
        pEntry->pKLInfo = NULL;
    }

    pEntry->bConsoleEnabled = FALSE;
    pEntry->bWndEnabled = FALSE;
    return TRUE;
}

//! Gets the name of the current keyboard layout or IME and stores it in the entry.
//  If necessary, sets the input mode abbreviation (szMode) from an internal table.
BOOL IntGetImeLayoutText(PCONENTRY pEntry)
{
    pEntry->szMode[0] = pEntry->szLayoutText[0] = UNICODE_NULL;

    HIMC hIMC = pEntry->hOldIMC;
    HKL hKL = pEntry->hKL;

    // Attempt to get IME name
    WCHAR szLayoutName[MAX_PATH];
    if (ImmEscapeW(hKL, hIMC, IME_ESC_IME_NAME, szLayoutName) ||
        ImmGetIMEFileNameW(hKL, szLayoutName, _countof(szLayoutName)))
    {
        StringCchCopyW(pEntry->szLayoutText, _countof(pEntry->szLayoutText), szLayoutName);
        return TRUE;
    }

    // Non IME Keyboard?
    if (!GetKeyboardLayoutNameW(szLayoutName) ||
        (szLayoutName[0] != L'E' && szLayoutName[0] != L'e'))
    {
        return FALSE;
    }

    // Build registry path
    WCHAR szKeyPath[MAX_PATH];
    StringCchCopyW(szKeyPath, _countof(szKeyPath),
                   L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\");
    StringCchCatW(szKeyPath, _countof(szKeyPath), szLayoutName);

    HKEY hKey;
    LSTATUS error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szKeyPath, 0, KEY_QUERY_VALUE, &hKey);
    if (error == ERROR_SUCCESS)
    {
        DWORD cbData = sizeof(pEntry->szLayoutText);
        RegQueryValueExW(hKey, L"Layout Text", NULL, NULL, (PBYTE)pEntry->szLayoutText, &cbData);
        RegCloseKey(hKey);
    }

    PCWSTR pszText = pEntry->szLayoutText;
    if (pszText[0])
    {
#define PREFIX_LEN 2
#define COLUMN_COUNT 7
        static const WCHAR awchTable[] =
        {
            0x5009, 0x9821, 0x8ACB, 0x8F38, 0x5165, 0x5B57, 0x6839, // L"倉頡請輸入字根"
            0x5167, 0x78BC, 0x8ACB, 0x8F38, 0x5165, 0x5167, 0x78BC, // L"內碼請輸入內碼"
            0x55AE, 0x78BC, 0x8ACB, 0x8F38, 0x5165, 0x55AE, 0x78BC, // L"單碼請輸入單碼"
            0x901F, 0x6210, 0x8ACB, 0x8F38, 0x5165, 0x5B57, 0x6839, // L"速成請輸入字根"
            0x5927, 0x6613, 0x8ACB, 0x8F38, 0x5165, 0x5B57, 0x6839, // L"大易請輸入字根"
            0x82F1, 0x6570, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, // L"英数　　　　　"
            0xFF55, 0xFF53, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, // L"ｕｓ　　　　　"
            0x6CE8, 0x97F3, 0x8ACB, 0x8F38, 0x5165, 0x7B26, 0x865F, // L"注音請輸入符號"
            0x6CE8, 0x97F3, 0x8ACB, 0x3000, 0x9078, 0x3000, 0x5B57, // L"注音請　選　字"
        };

        INT ich;
        for (ich = 0; ich < _countof(awchTable); ich += COLUMN_COUNT)
        {
            if (RtlEqualMemory(pszText, &awchTable[ich], PREFIX_LEN * sizeof(WCHAR)))
                break;
        }

        if (ich < _countof(awchTable))
            StringCchCopyW(pEntry->szMode, (COLUMN_COUNT - PREFIX_LEN) + 1, &awchTable[ich + 2]);
    }

    return TRUE;
}

BOOL IntIsImeOpen(HIMC hIMC, PCONENTRY pEntry)
{
    LANGID wLangId = LOWORD(pEntry->hKL);
    switch (wLangId)
    {
        case LANGID_CHINESE_SIMPLIFIED:
        case LANGID_CHINESE_TRADITIONAL:
        case LANGID_KOREAN:
            if (!ImmGetOpenStatus(hIMC))
                return FALSE;
            return ImmIsIME(pEntry->hKL);
        case LANGID_JAPANESE:
            return ImmGetOpenStatus(hIMC);
        default:
            return FALSE;
    }
}

//! Fills the status buffer with characters and attributes from the
// Traditional Chinese IME candidate list
void IntFillImeCandidatesCHT(PCONENTRY pEntry, PIMEDISPLAY pDisplay, UINT iCand)
{
    PCOMPSTRINFO pCompStr = pEntry->pCompStr;
    PCANDINFO pCandInfo = pEntry->pCandInfo;
    if (!pCandInfo)
        return;

    PBYTE pbAttrIndex = (PBYTE)pCandInfo + pCandInfo->dwAttrsOffset;
    PCHAR_INFO pDest = &pDisplay->CharInfo[iCand];

    for (PWCHAR pchSrc = pCandInfo->szCandStr; *pchSrc; ++pchSrc, ++pbAttrIndex, ++pDest)
    {
        pDest->Char.UnicodeChar = *pchSrc;
        if (*pbAttrIndex < _countof(pCompStr->awAttrColor))
            pDest->Attributes = pCompStr->awAttrColor[*pbAttrIndex];
    }
}

//! Builds a buffer for displaying the Traditional Chinese IME mode.
UINT IntFillImeModeCHT(PCONENTRY pEntry, PIMEDISPLAY pDisplay, INT cch)
{
    PWCHAR pLayoutSrc = pEntry->szLayoutText;

    UINT width;
    for (width = 0; *pLayoutSrc && width < 5 - 1;)
    {
        WCHAR wch = *pLayoutSrc++;
        pDisplay->CharInfo[cch++].Char.UnicodeChar = wch;
        width += IntIsDoubleWidthChar(wch) + 1;
    }

    INT paddingCount = 5 - width;
    for (INT i = 0; i < paddingCount; ++i)
        pDisplay->CharInfo[cch++].Char.UnicodeChar = L' ';

    WCHAR modeChar;
    if (pEntry->dwConversion & IME_CMODE_FULLSHAPE)
        modeChar = 0x5168; // L'全' (full-width)
    else
        modeChar = 0x534A; // L'半' (half-width)

    pDisplay->CharInfo[cch++].Char.UnicodeChar = modeChar;
    pDisplay->CharInfo[cch++].Char.UnicodeChar = L' ';
    pDisplay->CharInfo[cch++].Char.UnicodeChar = L':';

    for (UINT i = 0; i < cch; ++i)
        pDisplay->CharInfo[i].Attributes = _FOREGROUND_WHITE;

    return cch;
}

void IntFillImeCompStrCHSorCHT(PCONENTRY pEntry, PIMEDISPLAY pDisplay, UINT cch)
{
    UINT maxX = max(pEntry->ScreenSize.X, IMEDISPLAY_MAX_X);

    PCOMPSTRINFO pCompStr = pEntry->pCompStr;
    if (!pCompStr || !pCompStr->dwCompStrLen)
        return;

    PWCHAR pchSrc = (PWCHAR)&pCompStr[1]; // bottom of COMPSTRINFO
    size_t cbCompStr = pCompStr->dwCompStrLen + sizeof(UNICODE_NULL);
    PBYTE pbAttrIndex = (PBYTE)pchSrc + cbCompStr;
    PCHAR_INFO pDest = &pDisplay->CharInfo[cch];

    DWORD dwCharCount = pCompStr->dwCompStrLen / sizeof(WCHAR);
    for (DWORD ich = 0; ich < dwCharCount && ich < maxX; ++ich, ++pDest)
    {
        pDest->Char.UnicodeChar = pchSrc[ich];

        BYTE ibColor = pbAttrIndex[ich];
        if (ibColor < _countof(pCompStr->awAttrColor))
            pDest->Attributes = pCompStr->awAttrColor[ibColor];
    }
}

UINT IntGetCharInfoWidth(PCHAR_INFO pCharInfo, UINT cch)
{
    UINT ret = 0;
    for (UINT ich = 0; ich < cch; ++ich)
        ret += IntIsDoubleWidthChar(pCharInfo[ich].Char.UnicodeChar) + 1;
    return ret;
}

inline UINT IntGetCharDisplayWidth(WCHAR wch)
{
    return IntIsDoubleWidthChar(wch) ? 2 : 1;
}

//! Adjusts the width of the status display and pads it with spaces
UINT IntFillImeSpaceCHSorCHT(PCONENTRY pEntry, PIMEDISPLAY pDisplay, UINT cch)
{
    const UINT maxX = max(pEntry->ScreenSize.X, IMEDISPLAY_MAX_X);

    UINT currentWidth = IntGetCharInfoWidth(pDisplay->CharInfo, cch);
    UINT index = cch;
    while (currentWidth > maxX && index > 0)
    {
        --index;
        WCHAR wch = pDisplay->CharInfo[index].Char.UnicodeChar;
        currentWidth -= IntGetCharDisplayWidth(wch);
    }

    const UINT remainingSpace = maxX - currentWidth;
    const UINT ichFinal = index + remainingSpace;

    if (remainingSpace > 0)
    {
        for (UINT ich = index; ich < ichFinal; ++ich)
        {
            PCHAR_INFO pChar = &pDisplay->CharInfo[ich];
            pChar->Char.UnicodeChar = L' ';

            if (ich < cch)
                pChar->Attributes = pDisplay->CharInfo[ich].Attributes;
            else
                pChar->Attributes = _FOREGROUND_WHITE;
        }
    }

    return ichFinal;
}

//! Converts the current Traditional Chinese IME status into a string for
// display (CHAR_INFO array).
BOOL IntFillImeDisplayCHT(PCONENTRY pEntry, PIMEDISPLAY pDisplay)
{
    UINT cch = 0;
    if (ImmIsIME(pEntry->hKL))
    {
        cch = IntFillImeModeCHT(pEntry, pDisplay, cch);
        if (pEntry->bInComposition)
        {
            if (pEntry->bHasAnyCand)
                IntFillImeCandidatesCHT(pEntry, pDisplay, cch);
            else
                IntFillImeCompStrCHSorCHT(pEntry, pDisplay, cch);
        }
        cch = IntFillImeSpaceCHSorCHT(pEntry, pDisplay, cch);
    }

    pDisplay->bFlag = FALSE;
    pDisplay->uCharInfoLen = cch;
    return TRUE;
}

inline void IntCopyUnicodeToCharInfo(PCHAR_INFO* ppDest, PCWSTR pszSrc)
{
    if (!ppDest || !*ppDest || !pszSrc)
        return;

    PCHAR_INFO pCurrent;
    for (pCurrent = *ppDest; *pszSrc; ++pCurrent)
    {
        pCurrent->Char.UnicodeChar = *pszSrc++;
        pCurrent->Attributes = 0;
    }

    *ppDest = pCurrent;
}

//! Converts the current Japanese IME status into a string for display (CHAR_INFO array).
BOOL IntFillImeDisplayJPN(PCONENTRY pEntry, PIMEDISPLAY pDisplay)
{
    PCHAR_INFO pChar = pDisplay->CharInfo;
    DWORD dwConversion = pEntry->dwConversion, dwSentence = pEntry->dwSentence;
    UINT cch = 0;
    WCHAR wchSentence = L' ';

    if (!pEntry->bOpened)
        goto DoSetAttributes;

    // Determines the input character type (full-width/half-width, hiragana/katakana/alphanumeric)
    if (dwConversion & IME_CMODE_FULLSHAPE) // Full-width (全角)
    {
        if (dwConversion & IME_CMODE_NATIVE)
        {
            if (dwConversion & IME_CMODE_KATAKANA)
                IntCopyUnicodeToCharInfo(&pChar, L"\x5168\x30AB"); // L"全カ"
            else
                IntCopyUnicodeToCharInfo(&pChar, L"\x5168\x3042"); // L"全あ"
        }
        else
        {
            IntCopyUnicodeToCharInfo(&pChar, L"\x5168\xFF21"); // L"全Ａ"
        }
        cch = 2;
    }
    else // Half-width (半角)
    {
        if (dwConversion & IME_CMODE_NATIVE)
        {
            if (dwConversion & IME_CMODE_KATAKANA)
                IntCopyUnicodeToCharInfo(&pChar, L"\x534A\xFF76 "); // L"半ｶ "
            else
                IntCopyUnicodeToCharInfo(&pChar, L"\x534A\xFF71 "); // L"半ｱ "
        }
        else
        {
            IntCopyUnicodeToCharInfo(&pChar, L"\x534A\x0041 "); // L"半A "
        }
        cch = 3;
    }

    // Determine phrase conversion mode (compound phrase, simple phrase, automatic, multi-phrase)
    if (dwSentence & IME_SMODE_PLAURALCLAUSE)
        wchSentence = L'\x8907'; // L'複'
    else if (dwSentence & IME_SMODE_SINGLECONVERT)
        wchSentence = L'\x5358'; // L'単'
    else if (dwSentence & IME_SMODE_AUTOMATIC)
        wchSentence = L'\x81EA'; // L'自'
    else if (dwSentence & IME_SMODE_PHRASEPREDICT)
        wchSentence = L'\x9023'; // L'連'

    pDisplay->CharInfo[cch++].Char.UnicodeChar = wchSentence;
    ++pChar;

    // Input method (kana input / romaji input) detection
    if (GetKeyState(VK_KANA) & 1)
    {
        IntCopyUnicodeToCharInfo(&pChar, L"\xFF76\xFF85  "); // L"ｶﾅ  "
        cch += 4;
    }
    else if (dwConversion & IME_CMODE_ROMAN)
    {
        IntCopyUnicodeToCharInfo(&pChar, L"\xFF9B\xFF70\xFF8F "); // L"ﾛｰﾏ "
        cch += 4;
    }

DoSetAttributes:
    for (UINT ich = 0; ich < cch; ++ich)
        pDisplay->CharInfo[ich].Attributes = _FOREGROUND_WHITE;

    pDisplay->uCharInfoLen = cch;
    pDisplay->bFlag = TRUE;
    return TRUE;
}

//! Converts the current Korean IME status into a string for display (CHAR_INFO array).
BOOL IntFillImeDisplayKOR(PCONENTRY pEntry, PIMEDISPLAY pDisplay)
{
    pDisplay->CharInfo[0].Char.UnicodeChar = L' ';
    pDisplay->CharInfo[0].Attributes = _FOREGROUND_WHITE;
    pDisplay->uCharInfoLen = 1;
    pDisplay->bFlag = TRUE;
    return TRUE;
}

//! Builds a buffer for displaying the Simplified Chinese IME mode.
INT IntFillImeModeCHS(PCONENTRY pEntry, PIMEDISPLAY pDisplay, UINT cch)
{
    DWORD dwConversion = pEntry->dwConversion;
    UINT width = 0;

    for (PWCHAR pLayoutSrc = pEntry->szLayoutText; *pLayoutSrc; ++pLayoutSrc)
    {
        WCHAR wch = *pLayoutSrc;
        if (wch == 0x8F93) // U+8F93 L'输'
            break;

        pDisplay->CharInfo[cch++].Char.UnicodeChar = wch;

        width += (IntIsDoubleWidthChar(wch) + 1);
        if (width >= 9)
        {
            cch++;
            pLayoutSrc++;
            break;
        }
    }

    INT paddingCount = 9 - width;
    for (INT i = 0; i < paddingCount; ++i)
        pDisplay->CharInfo[cch++].Char.UnicodeChar = L' ';

    WCHAR modeChar;
    if (dwConversion & IME_CMODE_FULLSHAPE)
        modeChar = 0x5168; // U+5168 L'全'
    else
        modeChar = 0x534A; // U+534A L'半'

    pDisplay->CharInfo[cch++].Char.UnicodeChar = modeChar;
    pDisplay->CharInfo[cch++].Char.UnicodeChar = L':';

    for (UINT ich = 0; ich < cch; ++ich)
        pDisplay->CharInfo[ich].Attributes = _FOREGROUND_WHITE;

    return cch;
}

//! Fills the status buffer with characters and attributes from the
// Simplified Chinese IME candidate list
UINT IntFillImeCandidatesCHS(PCONENTRY pEntry, PIMEDISPLAY pDisplay, UINT cch)
{
    PCOMPSTRINFO pCompStr = pEntry->pCompStr;

    if (!pEntry->pCandInfo)
        return cch;

    UINT displayCols = 0;
    DWORD strPosByte = 0;

    if (pCompStr->dwCompStrLen > 0)
    {
        PWCHAR pszComp = (PWCHAR)&pCompStr[1]; // bottom of COMPSTRINFO
        size_t cbCompStr = pCompStr->dwCompStrLen + sizeof(UNICODE_NULL);
        PBYTE pbAttrs = (PBYTE)pszComp + cbCompStr;

        while (displayCols < 10)
        {
            pDisplay->CharInfo[cch].Char.UnicodeChar = *pszComp;
            pDisplay->CharInfo[cch].Attributes = pCompStr->awAttrColor[*pbAttrs];
            ++cch;

            strPosByte += sizeof(WCHAR);
            displayCols += IntIsDoubleWidthChar(*pszComp) + 1;
            ++pszComp;
            ++pbAttrs;

            if (strPosByte >= pCompStr->dwCompStrLen)
                break;
        }
    }

    INT padCount = 10 - displayCols;
    for (INT i = 0; i < padCount; ++i)
        pDisplay->CharInfo[cch++].Char.UnicodeChar = L' ';

    pDisplay->CharInfo[cch++].Char.UnicodeChar = L':';

    PCANDINFO pCandInfo = pEntry->pCandInfo;
    PBYTE pbCandAttrs = (PBYTE)pCandInfo + pCandInfo->dwAttrsOffset;

    for (PWCHAR pszCand = pCandInfo->szCandStr; *pszCand; ++pszCand)
    {
        pDisplay->CharInfo[cch].Char.UnicodeChar = *pszCand;
        pDisplay->CharInfo[cch].Attributes = pCompStr->awAttrColor[*pbCandAttrs];
        ++cch;
        ++pbCandAttrs;
    }

    return cch;
}

//! Converts the current Simplified Chinese IME status into a string for display (CHAR_INFO array).
BOOL IntFillImeDisplayCHS(PCONENTRY pEntry, PIMEDISPLAY pDisplay)
{
    UINT cch = 0;
    if (ImmIsIME(pEntry->hKL))
    {
        cch = IntFillImeModeCHS(pEntry, pDisplay, cch);
        if (pEntry->bInComposition)
        {
            if (pEntry->bHasAnyCand)
                cch = IntFillImeCandidatesCHS(pEntry, pDisplay, cch);
            else
                IntFillImeCompStrCHSorCHT(pEntry, pDisplay, cch);
        }
        cch = IntFillImeSpaceCHSorCHT(pEntry, pDisplay, cch);
    }
    pDisplay->bFlag = FALSE;
    pDisplay->uCharInfoLen = cch;
    return TRUE;
}

//! Converts the current IME status into a string for display (CHAR_INFO array).
BOOL IntFillImeDisplay(PCONENTRY pEntry, PIMEDISPLAY pDisplay)
{
    LANGID wLangId = LOWORD(pEntry->hKL);
    switch (wLangId)
    {
        case LANGID_CHINESE_SIMPLIFIED:
            return IntFillImeDisplayCHS(pEntry, pDisplay);
        case LANGID_CHINESE_TRADITIONAL:
            return IntFillImeDisplayCHT(pEntry, pDisplay);
        case LANGID_JAPANESE:
            return IntFillImeDisplayJPN(pEntry, pDisplay);
        case LANGID_KOREAN:
            return IntFillImeDisplayKOR(pEntry, pDisplay);
        default:
            return FALSE;
    }
}

//! IMN_SETOPENSTATUS
BOOL ConIme_OnNotifySetOpenStatus(HWND hwndTarget)
{
    // Find active console entries
    PCONENTRY pEntry = IntFindConsoleEntry(g_hConsole);
    if (!pEntry)
        return FALSE;

    // Get IME context
    HIMC hIMC = ImmGetContext(hwndTarget);
    if (!hIMC)
        return FALSE;

    // Update IME state in an entry
    pEntry->bOpened = IntIsImeOpen(hIMC, pEntry);
    ImmGetConversionStatus(hIMC, &pEntry->dwConversion, &pEntry->dwSentence);

    // Send status to console according to conditions
    BOOL bSuccess = TRUE;
    if (pEntry->ScreenSize.X)
    {
        PIMEDISPLAY pDisplay = LocalAlloc(LPTR, sizeof(IMEDISPLAY));
        if (pDisplay)
        {
            COPYDATASTRUCT CopyData;
            CopyData.dwData = MAGIC_SEND_IMEDISPLAY;
            CopyData.cbData = sizeof(*pDisplay);
            CopyData.lpData = pDisplay;

            if (IntFillImeDisplay(pEntry, pDisplay))
                IntSendDataToConsole(pEntry->hwndConsole, hwndTarget, &CopyData);

            LocalFree(pDisplay);
        }
        else
        {
            bSuccess = FALSE;
        }
    }

    ImmReleaseContext(hwndTarget, hIMC);
    return bSuccess;
}

void IntSendConversionStatusCHT(HWND hwnd, PCONENTRY pEntry)
{
    PIMEDISPLAY pDisplay = LocalAlloc(LPTR, sizeof(IMEDISPLAY));
    if (!pDisplay)
        return;

    COPYDATASTRUCT CopyData;
    CopyData.cbData = sizeof(*pDisplay);
    CopyData.dwData = MAGIC_SEND_IMEDISPLAY;
    CopyData.lpData = pDisplay;
    if (IntFillImeDisplayCHT(pEntry, pDisplay))
        IntSendDataToConsole(pEntry->hwndConsole, hwnd, &CopyData);

    LocalFree(pDisplay);
}

void IntSendConversionStatusJPNorKOR(HWND hwnd, PCONENTRY pEntry)
{
    PCOMPSTRINFO pCompStr = pEntry->pCompStr;
    if (!pCompStr)
        return;

    COPYDATASTRUCT CopyData;
    CopyData.dwData = MAGIC_SEND_COMPSTR;
    CopyData.lpData = pCompStr;
    CopyData.cbData = pCompStr->dwSize;
    IntSendDataToConsole(pEntry->hwndConsole, hwnd, &CopyData);
}

void IntSendConversionStatusCHS(HWND hwnd, PCONENTRY pEntry)
{
    PIMEDISPLAY pDisplay = LocalAlloc(LPTR, sizeof(IMEDISPLAY));
    if (!pDisplay)
        return;

    COPYDATASTRUCT CopyData;
    CopyData.cbData = sizeof(*pDisplay);
    CopyData.dwData = MAGIC_SEND_IMEDISPLAY;
    CopyData.lpData = pDisplay;
    if (IntFillImeDisplayCHS(pEntry, pDisplay))
        IntSendDataToConsole(pEntry->hwndConsole, hwnd, &CopyData);

    LocalFree(pDisplay);
}

void IntSendConversionStatus(HWND hwnd)
{
    PCONENTRY pEntry = IntFindConsoleEntry(g_hConsole);
    if (!pEntry || !pEntry->bInComposition)
        return;

    LANGID wLangId = LOWORD(pEntry->hKL);
    switch (wLangId)
    {
        case LANGID_CHINESE_SIMPLIFIED:
            IntSendConversionStatusCHS(hwnd, pEntry);
            break;
        case LANGID_CHINESE_TRADITIONAL:
            IntSendConversionStatusCHT(hwnd, pEntry);
            break;
        case LANGID_JAPANESE:
        case LANGID_KOREAN:
            IntSendConversionStatusJPNorKOR(hwnd, pEntry);
            break;
    }
}

//! WM_USER_SWITCHIME
BOOL ConIme_OnSwitchIme(HWND hwnd, HANDLE hConsole, HKL hKL)
{
    PCONENTRY pEntry = IntFindConsoleEntry(hConsole);
    if (!pEntry)
        return FALSE;

    if (g_bDisabled)
        g_bDisabled = FALSE;

    HKL hOldKL = pEntry->hKL;
    pEntry->hKL = hKL;
    ActivateKeyboardLayout(hKL, 0);

    ImmAssociateContext(hwnd, pEntry->hOldIMC);

    if (!hOldKL)
    {
        IntGetImeLayoutText(pEntry);
        pEntry->dwImeProp = ImmGetProperty(pEntry->hKL, IGP_PROPERTY);
    }

    ImmSetActiveContextConsoleIME(hwnd, TRUE);
    IntSetCurrentConsole(hConsole);
    ConIme_OnNotifySetOpenStatus(hwnd);

    if (pEntry->pCompStr)
        IntSendConversionStatus(hwnd);

    return TRUE;
}

//! WM_USER_DEACTIVATE
BOOL ConIme_OnDeactivate(HWND hwnd, HANDLE hConsole)
{
    PCONENTRY pEntry = IntFindConsoleEntry(hConsole);
    if (!pEntry)
        return FALSE;

    if (g_bDisabled)
    {
        g_bDisabled = FALSE;
    }
    else
    {
        ImmSetActiveContextConsoleIME(hwnd, FALSE);
        ImmAssociateContext(hwnd, g_hOldIMC);
    }

    return TRUE;
}

//! WM_USER_SETSCREENSIZE
BOOL ConIme_SetScreenSize(HWND hwnd, HANDLE hConsole, COORD ScreenSize)
{
    PCONENTRY pEntry = IntFindConsoleEntry(hConsole);
    if (!pEntry)
        return FALSE;

    pEntry->ScreenSize = ScreenSize;
    return TRUE;
}

//! Handles keyboard layout switch requests.
BOOL ConIme_OnGo(HWND hwnd, HANDLE hConsole, HKL hKL, INT iDirection)
{
    PCONENTRY pEntry = IntFindConsoleEntry(hConsole);
    if (!pEntry)
        return FALSE; // Deny

    LANGID wTargetLang = 0;
    switch (pEntry->nOutputCodePage)
    {
        case CP_CHINESE_SIMPLIFIED:
            wTargetLang = LANGID_CHINESE_SIMPLIFIED;
            break;
        case CP_CHINESE_TRADITIONAL:
            wTargetLang = LANGID_CHINESE_TRADITIONAL;
            break;
        case CP_JAPANESE:
            wTargetLang = LANGID_JAPANESE;
            break;
        case CP_KOREAN:
            wTargetLang = LANGID_KOREAN;
            break;
        default:
            return TRUE; // Allow
    }

    if (!IS_IME_HKL(hKL) || LOWORD(hKL) == wTargetLang)
        return TRUE; // Allow

    if (iDirection == 0)
        return FALSE; // Deny

    // Get keyboard list
    INT cKLs = GetKeyboardLayoutList(0, NULL);
    if (cKLs <= 0)
        return FALSE; // Deny
    HKL* phKLs = LocalAlloc(LPTR, cKLs * sizeof(HKL));
    if (!phKLs)
        return FALSE; // Deny
    GetKeyboardLayoutList(cKLs, phKLs);

    // Get current keyboard
    INT iCurrent = 0;
    for (; iCurrent < cKLs; ++iCurrent)
    {
        if (pEntry->hKL == phKLs[iCurrent])
            break;
    }

    if (iCurrent < cKLs)
    {
        for (INT iAttempt = 0, iNext = iCurrent; iAttempt < cKLs; ++iAttempt)
        {
            iNext += iDirection;

            if (iNext < 0)
                iNext = cKLs - 1;
            else if (iNext >= cKLs)
                iNext = 0;

            HKL hKL = phKLs[iNext];
            LANGID wLangId = LOWORD(hKL);
            if (!IS_IME_HKL(hKL) || wLangId == wTargetLang)
            {
                PostMessageW(pEntry->hwndConsole, WM_USER + 0x0F, (WPARAM)hKL, 0);
                break;
            }
        }
    }

    LocalFree(phKLs);
    return FALSE; // Deny
}

//! WM_USER_SENDIMESTATUS
BOOL ConIme_SendImeStatus(HWND hWnd)
{
    PCONENTRY pEntry = IntFindConsoleEntry(g_hConsole);
    if (!pEntry)
        return FALSE;

    HIMC hIMC = ImmGetContext(hWnd);
    if (!hIMC)
        return FALSE;

    BOOL ret;
    PIMEDISPLAY pDisplay = LocalAlloc(LPTR, sizeof(IMEDISPLAY));
    if (pDisplay)
    {
        ImmGetConversionStatus(hIMC, &pEntry->dwConversion, &pEntry->dwSentence);

        COPYDATASTRUCT CopyData;
        CopyData.dwData = MAGIC_SEND_IMEDISPLAY;
        CopyData.cbData = sizeof(*pDisplay);
        CopyData.lpData = pDisplay;
        if (IntFillImeDisplay(pEntry, pDisplay))
            IntSendDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);

        LocalFree(pDisplay);
        ret = TRUE;
    }
    else
    {
        ret = FALSE;
    }

    ImmReleaseContext(hWnd, hIMC);
    return ret;
}

BOOL ConIme_OnInputLangChange(HWND hwnd, WPARAM wParam, HKL hKL)
{
    TRACE("(%p, %p, %p)\n", hwnd, wParam, hKL);

    PCONENTRY pEntry = IntFindConsoleEntry(g_hConsole);
    if (!pEntry)
        return FALSE;

    pEntry->hKL = hKL;
    ActivateKeyboardLayout(hKL, 0);
    IntGetImeLayoutText(pEntry);
    ConIme_SendImeStatus(hwnd);
    return TRUE;
}

//! Initializes and allocates a CONENTRY structure for the new console connection.
BOOL ConIme_InitEntry(HWND hwnd, HANDLE hConsole, HWND hwndConsole)
{
    TRACE("(%p, %p, %p)\n", hwnd, hConsole, hwndConsole);

    PCONENTRY pEntry = NULL;

    for (UINT iEntry = 1; ; ++iEntry)
    {
        while (iEntry >= g_cEntries)
        {
            if (!IntGrowEntries())
                return FALSE;
        }

        pEntry = g_ppEntries[iEntry];
        if (!pEntry)
        {
            pEntry = LocalAlloc(LPTR, sizeof(CONENTRY));
            if (!pEntry)
                return FALSE;

            g_ppEntries[iEntry] = pEntry;
        }

        if (!pEntry->hConsole)
            break;

        if (pEntry->bWndEnabled && pEntry->bConsoleEnabled)
            ConIme_UnInitEntry(hwnd, pEntry);

        if (!pEntry->hConsole)
            break;
    }

    ZeroMemory(pEntry, sizeof(*pEntry));
    PKLINFO pKLInfo = pEntry->pKLInfo = LocalAlloc(LPTR, sizeof(KLINFO));
    if (!pEntry->pKLInfo)
        return FALSE;

    pKLInfo->hKL = NULL;
    pKLInfo->dwConversion = 0;
    pEntry->cKLs = 1;

    HIMC hIMC = ImmCreateContext();
    if (!hIMC)
    {
        LocalFree(pKLInfo);
        pEntry->pKLInfo = NULL;
        return FALSE;
    }

    pEntry->hNewIMC = hIMC;
    pEntry->hConsole = hConsole;
    pEntry->hwndConsole = hwndConsole;
    pEntry->bConsoleEnabled = TRUE;
    pEntry->ScreenSize.X = 80;
    pEntry->awAttrColor[0] = COMMON_LVB_UNDERSCORE | 0x7;
    pEntry->awAttrColor[1] = BACKGROUND_BLUE | _FOREGROUND_WHITE;
    pEntry->awAttrColor[2] = COMMON_LVB_UNDERSCORE | _FOREGROUND_WHITE;
    pEntry->awAttrColor[3] = COMMON_LVB_UNDERSCORE | _BACKGROUND_WHITE | FOREGROUND_BLUE;
    pEntry->awAttrColor[4] = COMMON_LVB_UNDERSCORE | FOREGROUND_RED;
    pEntry->awAttrColor[5] = COMMON_LVB_UNDERSCORE | FOREGROUND_RED;
    pEntry->awAttrColor[6] = COMMON_LVB_UNDERSCORE | FOREGROUND_RED;
    pEntry->awAttrColor[7] = COMMON_LVB_UNDERSCORE | FOREGROUND_RED;
    IntGetImeLayoutText(pEntry);
    TRACE("ConIme_InitEntry --> %p\n", pEntry);
    return TRUE;
}

//! WM_USER_UNINIT
BOOL ConIme_OnUnInit(HWND hwnd, HANDLE hConsole)
{
    EnterCriticalSection(&g_csLock);
    PCONENTRY pEntry = IntFindConsoleEntry(hConsole);
    BOOL ret;
    if (pEntry)
        ret = ConIme_UnInitEntry(hwnd, pEntry);
    else
        ret = FALSE;
    LeaveCriticalSection(&g_csLock);
    return ret;
}

//! WM_USER_INIT
BOOL ConIme_OnInit(HWND hwnd, HANDLE hConsole, HWND hwndConsole)
{
    if (IntFindConsoleEntry(hConsole))
        return TRUE;

    BOOL ret;
    EnterCriticalSection(&g_csLock);
    if (ConIme_InitEntry(hwnd, hConsole, hwndConsole))
    {
        ConIme_OnNotifySetOpenStatus(hwndConsole);
        ret = TRUE;
    }
    else
    {
        ret = FALSE;
    }

    LeaveCriticalSection(&g_csLock);
    return ret;
}

//! WM_CREATE
inline BOOL ConIme_OnCreate(HWND hwnd)
{
    HIMC hIMC = ImmGetContext(hwnd);
    if (hIMC)
    {
        g_hOldIMC = hIMC;
        ImmReleaseContext(hwnd, hIMC);
    }
    return TRUE;
}

//! Processes Japanese IME Composition and Result strings.
void IntDoImeCompJPN(HWND hwnd, PCONENTRY pEntry, DWORD dwFlags)
{
    HIMC hIMC = ImmGetContext(hwnd);
    if (!hIMC)
        return;

    DWORD dwCompLen = 0; // composition string length (in bytes, excluding)
    DWORD dwAttrLen = 0; // attribute info length (in bytes)
    size_t cbCompStr, cbNeeded;
    COPYDATASTRUCT CopyData;

    if (dwFlags & GCS_COMPSTR)
    {
        dwCompLen = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, NULL, 0);
        if ((LONG)dwCompLen < 0)
            goto EXIT;

        if (dwFlags & GCS_COMPATTR)
        {
            dwAttrLen = ImmGetCompositionStringW(hIMC, GCS_COMPATTR, NULL, 0);
            if ((LONG)dwAttrLen < 0)
                goto EXIT;
        }
    }
    else if (dwFlags & GCS_RESULTSTR)
    {
        dwCompLen = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, NULL, 0);
        if ((LONG)dwCompLen < 0)
            goto EXIT;
    }
    else if (dwFlags == 0)
    {
        dwCompLen = 0;
    }
    else
    {
        goto EXIT;
    }

    cbCompStr = dwCompLen + sizeof(UNICODE_NULL);
    cbNeeded = sizeof(COMPSTRINFO) + cbCompStr + (dwAttrLen + 1);
    if (pEntry->pCompStr && pEntry->pCompStr->dwSize < cbNeeded)
    {
        LocalFree(pEntry->pCompStr);
        pEntry->pCompStr = NULL;
    }

    if (!pEntry->pCompStr)
    {
        pEntry->pCompStr = LocalAlloc(LPTR, cbNeeded);
        if (!pEntry->pCompStr)
            goto EXIT;
        pEntry->pCompStr->dwSize = (DWORD)cbNeeded;
    }

    PCOMPSTRINFO pCompStr = pEntry->pCompStr;
    ZeroMemory(&pCompStr->dwCompAttrLen, pCompStr->dwSize - offsetof(COMPSTRINFO, dwCompAttrLen));
    CopyMemory(pCompStr->awAttrColor, pEntry->awAttrColor, sizeof(pCompStr->awAttrColor));

    PWCHAR pszDest = (PWCHAR)&pCompStr[1]; // bottom of COMPSTRINFO
    PBYTE pAttrDest = (PBYTE)pszDest + cbCompStr;

    if (dwFlags & GCS_COMPSTR)
    {
        ImmGetCompositionStringW(hIMC, GCS_COMPSTR, pszDest, dwCompLen);
        pszDest[dwCompLen / sizeof(WCHAR)] = UNICODE_NULL;

        if (dwAttrLen)
        {
            ImmGetCompositionStringW(hIMC, GCS_COMPATTR, pAttrDest, dwAttrLen);
            pAttrDest[dwAttrLen] = 0;
        }

        LONG cursorPos = ImmGetCompositionStringW(hIMC, GCS_CURSORPOS, NULL, 0);
        if (cursorPos >= 0 && (DWORD)cursorPos < dwAttrLen)
            pAttrDest[cursorPos] |= BACKGROUND_BLUE;
        else
            pAttrDest[0] |= BACKGROUND_GREEN;

        pCompStr->dwCompStrLen = dwCompLen;
        pCompStr->dwCompStrOffset = sizeof(COMPSTRINFO);
        pCompStr->dwCompAttrLen = dwAttrLen;
        pCompStr->dwCompAttrOffset = sizeof(COMPSTRINFO) + cbCompStr;
    }
    else if (dwFlags & GCS_RESULTSTR)
    {
        ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, pszDest, dwCompLen);
        pszDest[dwCompLen / sizeof(WCHAR)] = UNICODE_NULL;

        pCompStr->dwResultStrLen = dwCompLen;
        pCompStr->dwResultStrOffset = sizeof(COMPSTRINFO);
    }

    CopyData.dwData = MAGIC_SEND_COMPSTR;
    CopyData.cbData = pCompStr->dwSize;
    CopyData.lpData = pCompStr;
    IntSendDataToConsole(pEntry->hwndConsole, hwnd, &CopyData);

EXIT:
    ImmReleaseContext(hwnd, hIMC);
}

//! Notifies the input composition status of the Chinese (Simplified) IME to the console
void IntDoImeCompCHS(HWND hwnd, PCONENTRY pEntry, DWORD dwFlags)
{
    HIMC hIMC = ImmGetContext(hwnd);
    if (!hIMC)
        return;

    PIMEDISPLAY pDisplay = LocalAlloc(LPTR, sizeof(IMEDISPLAY));
    if (!pDisplay)
    {
        ImmReleaseContext(hwnd, hIMC);
        return;
    }

    DWORD dwCompLen = 0; // composition string length (in bytes, excluding)
    DWORD dwAttrLen = 0; // attribute info length (in bytes)
    size_t cbCompStr, cbNeeded;
    COPYDATASTRUCT CopyData;

    if (dwFlags & GCS_COMPSTR)
    {
        dwCompLen = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, NULL, 0);
        if ((LONG)dwCompLen < 0)
            goto EXIT;

        if (dwFlags & GCS_COMPATTR)
        {
            dwAttrLen = ImmGetCompositionStringW(hIMC, GCS_COMPATTR, NULL, 0);
            if ((LONG)dwAttrLen < 0)
                goto EXIT;
        }
    }
    else if (dwFlags & GCS_RESULTSTR)
    {
        dwCompLen = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, NULL, 0);
        if ((LONG)dwCompLen < 0)
            goto EXIT;
    }
    else if (dwFlags == 0)
    {
        dwCompLen = 0;
    }
    else
    {
        goto EXIT;
    }

    cbCompStr = dwCompLen + sizeof(UNICODE_NULL);
    cbNeeded = sizeof(COMPSTRINFO) + cbCompStr + (dwAttrLen + 1);
    if (pEntry->pCompStr && cbNeeded > pEntry->pCompStr->dwSize)
    {
        LocalFree(pEntry->pCompStr);
        pEntry->pCompStr = NULL;
    }

    if (!pEntry->pCompStr)
    {
        pEntry->pCompStr = LocalAlloc(LPTR, cbNeeded);
        if (!pEntry->pCompStr)
            goto EXIT;
        pEntry->pCompStr->dwSize = (DWORD)cbNeeded;
    }

    PCOMPSTRINFO pCompStr = pEntry->pCompStr;
    ZeroMemory(&pCompStr->dwCompAttrLen, pCompStr->dwSize - offsetof(COMPSTRINFO, dwCompAttrLen));
    CopyMemory(pCompStr->awAttrColor, pEntry->awAttrColor, sizeof(pCompStr->awAttrColor));

    PWCHAR pszDest = (PWCHAR)&pCompStr[1]; // bottom of COMPSTRINFO
    PBYTE pAttrDest = (PBYTE)pszDest + cbCompStr;

    if (dwFlags & GCS_COMPSTR)
    {
        ImmGetCompositionStringW(hIMC, GCS_COMPSTR, pszDest, dwCompLen);
        pszDest[dwCompLen / sizeof(WCHAR)] = UNICODE_NULL;

        if (dwAttrLen)
        {
            ImmGetCompositionStringW(hIMC, GCS_COMPATTR, pAttrDest, dwAttrLen);
            pAttrDest[dwAttrLen] = 0;
        }

        pCompStr->dwCompStrLen = dwCompLen;
        pCompStr->dwCompStrOffset = sizeof(COMPSTRINFO);
        pCompStr->dwCompAttrLen = dwAttrLen;
        pCompStr->dwCompAttrOffset = sizeof(COMPSTRINFO) + cbCompStr;
    }
    else if (dwFlags & GCS_RESULTSTR)
    {
        ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, pszDest, dwCompLen);
        pszDest[dwCompLen / sizeof(WCHAR)] = UNICODE_NULL;

        pCompStr->dwResultStrLen = dwCompLen;
        pCompStr->dwResultStrOffset = sizeof(COMPSTRINFO);

        CopyData.dwData = MAGIC_SEND_COMPSTR;
        CopyData.cbData = pCompStr->dwSize;
        CopyData.lpData = pCompStr;
        IntSendDataToConsole(pEntry->hwndConsole, hwnd, &CopyData);
        goto EXIT;
    }

    CopyData.dwData = MAGIC_SEND_IMEDISPLAY;
    CopyData.cbData = sizeof(IMEDISPLAY);
    CopyData.lpData = pDisplay;
    if (IntFillImeDisplayCHS(pEntry, pDisplay))
        IntSendDataToConsole(pEntry->hwndConsole, hwnd, &CopyData);

EXIT:
    if (pDisplay)
        LocalFree(pDisplay);
    ImmReleaseContext(hwnd, hIMC);
}

//! Processes the input composition status of the Chinese (Traditional) IME and
// notifies the console
void IntDoImeCompCHT(HWND hWnd, PCONENTRY pEntry, DWORD dwFlags)
{
    HIMC hIMC = ImmGetContext(hWnd);
    if (!hIMC)
        return;

    PIMEDISPLAY pDisplay = LocalAlloc(LPTR, sizeof(IMEDISPLAY));
    if (!pDisplay)
    {
        ImmReleaseContext(hWnd, hIMC);
        return;
    }

    DWORD dwCompLen = 0; // composition string length (in bytes)
    DWORD dwAttrLen = 0; // attribute info length (in bytes)
    COPYDATASTRUCT CopyData;
    size_t cbCompStr, cbNeeded;
    PCOMPSTRINFO pCompStr;
    PWCHAR pszDest;
    PBYTE pAttrDest;

    if (dwFlags & GCS_COMPSTR)
    {
        dwCompLen = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, NULL, 0);
        if ((LONG)dwCompLen < 0)
            goto EXIT;

        if (dwFlags & GCS_COMPATTR)
        {
            dwAttrLen = ImmGetCompositionStringW(hIMC, GCS_COMPATTR, NULL, 0);
            if ((LONG)dwAttrLen < 0)
                goto EXIT;
        }
    }
    else if (dwFlags & GCS_RESULTSTR)
    {
        dwCompLen = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, NULL, 0);
        if ((LONG)dwCompLen < 0)
            goto EXIT;
    }
    else if (dwFlags != 0)
    {
        goto EXIT;
    }

    cbCompStr = dwCompLen + sizeof(UNICODE_NULL);
    cbNeeded = sizeof(COMPSTRINFO) + cbCompStr + (dwAttrLen + 1);
    if (pEntry->pCompStr && cbNeeded > pEntry->pCompStr->dwSize)
    {
        LocalFree(pEntry->pCompStr);
        pEntry->pCompStr = NULL;
    }

    if (!pEntry->pCompStr)
    {
        pEntry->pCompStr = LocalAlloc(LPTR, cbNeeded);
        if (!pEntry->pCompStr)
            goto EXIT;
        pEntry->pCompStr->dwSize = (DWORD)cbNeeded;
    }

    pCompStr = pEntry->pCompStr;
    ZeroMemory(&pCompStr->dwCompAttrLen, pCompStr->dwSize - sizeof(DWORD));
    CopyMemory(pCompStr->awAttrColor, pEntry->awAttrColor, sizeof(pCompStr->awAttrColor));

    pszDest = (PWCHAR)&pCompStr[1]; // bottom of COMPSTRINFO
    pAttrDest = (PBYTE)pszDest + cbCompStr;

    if (dwFlags & GCS_COMPSTR)
    {
        ImmGetCompositionStringW(hIMC, GCS_COMPSTR, pszDest, dwCompLen);
        pszDest[dwCompLen / sizeof(WCHAR)] = UNICODE_NULL;

        if (dwAttrLen)
        {
            ImmGetCompositionStringW(hIMC, GCS_COMPATTR, pAttrDest, dwAttrLen);
            pAttrDest[dwAttrLen] = 0;
        }

        pCompStr->dwCompStrLen = dwCompLen;
        if (dwCompLen)
            pCompStr->dwCompStrOffset = sizeof(COMPSTRINFO);
        else
            pCompStr->dwCompStrOffset = 0;

        pCompStr->dwCompAttrLen = dwAttrLen;
        if (dwAttrLen)
            pCompStr->dwCompAttrOffset = sizeof(COMPSTRINFO) + cbCompStr;
        else
            pCompStr->dwCompAttrOffset = 0;
    }
    else if (dwFlags & GCS_RESULTSTR)
    {
        ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, pszDest, dwCompLen);
        pszDest[dwCompLen / sizeof(WCHAR)] = UNICODE_NULL;

        pCompStr->dwResultStrLen = dwCompLen;
        if (dwCompLen)
            pCompStr->dwResultStrOffset = sizeof(COMPSTRINFO);
        else
            pCompStr->dwResultStrOffset = 0;

        CopyData.dwData = MAGIC_SEND_COMPSTR;
        CopyData.cbData = pCompStr->dwSize;
        CopyData.lpData = pCompStr;
        IntSendDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);
        goto EXIT;
    }

    CopyData.lpData = pDisplay;
    CopyData.cbData = sizeof(*pDisplay);
    CopyData.dwData = MAGIC_SEND_IMEDISPLAY;
    if (IntFillImeDisplayCHT(pEntry, pDisplay))
        IntSendDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);

EXIT:
    if (pDisplay)
        LocalFree(pDisplay);
    ImmReleaseContext(hWnd, hIMC);
}

//! Processes the Korean IME input composition state and notifies the console
void IntDoImeCompKOR(HWND hwnd, PCONENTRY pEntry, DWORD dwFlags, WCHAR wch)
{
    HIMC hIMC = ImmGetContext(hwnd);
    if (!hIMC)
        return;

    DWORD dwCompLen = 0; // composition string length (in bytes)
    DWORD dwAttrLen = 0; // attribute info length (in bytes)
    size_t cbCompStr, cbNeeded;
    PCOMPSTRINFO pCompStr;
    PWCHAR pszDest;
    PBYTE pAttrDest;
    COPYDATASTRUCT CopyData;

    if (dwFlags & GCS_COMPSTR)
    {
        dwCompLen = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, NULL, 0);
        if ((LONG)dwCompLen < 0)
            goto EXIT;

        if (dwFlags & GCS_COMPATTR)
        {
            dwAttrLen = ImmGetCompositionStringW(hIMC, GCS_COMPATTR, NULL, 0);
            if ((LONG)dwAttrLen < 0)
                goto EXIT;
        }
        else
        {
            dwAttrLen = dwCompLen;
        }
    }
    else if (dwFlags & GCS_RESULTSTR)
    {
        dwCompLen = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, NULL, 0);
        if ((LONG)dwCompLen < 0)
            goto EXIT;
        dwAttrLen = 0;
    }
    else if (dwFlags == 0)
    {
        dwCompLen = 0;
        dwAttrLen = 0;
    }
    else
    {
        goto EXIT;
    }

    cbCompStr = dwCompLen + sizeof(UNICODE_NULL);
    cbNeeded = sizeof(COMPSTRINFO) + cbCompStr + (dwAttrLen + 1);
    if (pEntry->pCompStr && pEntry->pCompStr->dwSize < cbNeeded)
    {
        LocalFree(pEntry->pCompStr);
        pEntry->pCompStr = NULL;
    }

    if (!pEntry->pCompStr)
    {
        pEntry->pCompStr = LocalAlloc(LPTR, cbNeeded);
        if (!pEntry->pCompStr)
            goto EXIT;
        pEntry->pCompStr->dwSize = cbNeeded;
    }

    pCompStr = pEntry->pCompStr;
    ZeroMemory(&pCompStr->dwCompAttrLen, pCompStr->dwSize - offsetof(COMPSTRINFO, dwCompAttrLen));
    CopyMemory(pCompStr->awAttrColor, pEntry->awAttrColor, sizeof(pCompStr->awAttrColor));

    pszDest = (PWCHAR)&pCompStr[1]; // bottom of COMPSTRINFO
    pAttrDest = (PBYTE)pszDest + cbCompStr;

    if (dwFlags & _GCS_SINGLECHAR)
    {
        *pszDest = wch;
        pszDest[dwCompLen / sizeof(WCHAR)] = UNICODE_NULL;
        *pAttrDest = 1;
        pAttrDest[dwAttrLen] = 0;
    }
    else if (dwFlags & GCS_COMPSTR)
    {
        ImmGetCompositionStringW(hIMC, GCS_COMPSTR, pszDest, dwCompLen);
        pszDest[dwCompLen / sizeof(WCHAR)] = UNICODE_NULL;

        if (dwFlags & GCS_COMPATTR)
        {
            ImmGetCompositionStringW(hIMC, GCS_COMPATTR, pAttrDest, dwAttrLen);
            pAttrDest[dwAttrLen] = 0;
        }
        else
        {
            ZeroMemory(pAttrDest, dwAttrLen);
            pAttrDest[dwAttrLen] = 0;
        }
        pCompStr->dwCompStrLen = dwCompLen;
        pCompStr->dwCompStrOffset = (DWORD)((PBYTE)pszDest - (PBYTE)pCompStr);
        pCompStr->dwCompAttrLen = dwAttrLen;
        pCompStr->dwCompAttrOffset = (DWORD)((PBYTE)pAttrDest - (PBYTE)pCompStr);
    }
    else if (dwFlags & GCS_RESULTSTR)
    {
        ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, pszDest, dwCompLen);
        pszDest[dwCompLen / sizeof(WCHAR)] = UNICODE_NULL;
        pCompStr->dwResultStrLen = dwCompLen;
        pCompStr->dwResultStrOffset = (DWORD)((PBYTE)pszDest - (PBYTE)pCompStr);
    }

    CopyData.dwData = MAGIC_SEND_COMPSTR;
    CopyData.cbData = pCompStr->dwSize;
    CopyData.lpData = pCompStr;
    IntSendDataToConsole(pEntry->hwndConsole, hwnd, &CopyData);

EXIT:
    ImmReleaseContext(hwnd, hIMC);
}

void IntDoImeComp(HWND hwnd, DWORD dwFlags, WCHAR wch)
{
    PCONENTRY pEntry = IntFindConsoleEntry(g_hConsole);
    if (!pEntry)
        return;

    switch (pEntry->nOutputCodePage)
    {
        case CP_CHINESE_SIMPLIFIED:
            IntDoImeCompCHS(hwnd, pEntry, dwFlags);
            break;
        case CP_CHINESE_TRADITIONAL:
            IntDoImeCompCHT(hwnd, pEntry, dwFlags);
            break;
        case CP_JAPANESE:
            IntDoImeCompJPN(hwnd, pEntry, dwFlags);
            break;
        case CP_KOREAN:
            IntDoImeCompKOR(hwnd, pEntry, dwFlags, wch);
            break;
    }
}

//! WM_IME_COMPOSITION
void ConIme_OnImeComposition(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACE("WM_IME_COMPOSITION\n");
    if (!lParam)
        IntDoImeComp(hwnd, 0, wParam);
    if (lParam & GCS_RESULTSTR)
        IntDoImeComp(hwnd, (DWORD)(lParam & GCS_RESULTSTR), wParam);
    if (lParam & GCS_COMPSTR)
        IntDoImeComp(hwnd, (DWORD)(lParam & (GCS_COMPATTR | GCS_COMPSTR)), wParam);
    if (lParam & _GCS_SINGLECHAR)
        IntDoImeComp(hwnd, (DWORD)(lParam & (_GCS_SINGLECHAR | GCS_COMPATTR)), wParam);
    if (lParam & 0x4000)
        IntDoImeComp(hwnd, (DWORD)(lParam & 0x4010), wParam);
}

UINT IntFormatCandLineJPNorKOR(
    PCANDIDATELIST pCandList,
    PWSTR pszCandStrDest,
    PBYTE pbAttrsDest,
    UINT width,
    UINT labelWidth,
    PCONENTRY pEntry,
    BOOL bIsCode)
{
    if (pCandList->dwSelection >= pCandList->dwCount)
        pCandList->dwSelection = 0;

    UINT pageIndex;
    for (pageIndex = pEntry->dwCandIndexMax; pageIndex > 0; --pageIndex)
    {
        if (pCandList->dwSelection >= pEntry->pdwCandPageStart[pageIndex])
            break;
    }

    ZeroMemory(pbAttrsDest, width);

    PWSTR pszCurrentPos = pszCandStrDest;
    PBYTE pbCurrentAttr = pbAttrsDest;
    UINT currentX = 0;
    UINT candNum = 1;

    if (bIsCode)
    {
        WCHAR szCharCode[8];
        CHAR asz[2];

        PCWCH pwchCand = (PCWCH)((PBYTE)pCandList + pCandList->dwOffset[pCandList->dwSelection]);
        WideCharToMultiByte(CP_OEMCP, 0, pwchCand, 1, asz, _countof(asz), NULL, NULL);

        StringCchPrintfW(szCharCode, _countof(szCharCode), L"[%04X] ", MAKEWORD(asz[0], asz[1]));

        size_t cchCode = wcslen(szCharCode);
        StringCchCopyW(pszCurrentPos, cchCode + 1, szCharCode);

        currentX = cchCode;
        pszCurrentPos += cchCode;
        pbCurrentAttr += cchCode;
    }

    DWORD iStart = pEntry->pdwCandPageStart[pageIndex];
    DWORD iEnd = pEntry->pdwCandPageStart[pageIndex + 1];
    for (DWORD i = iStart; i < iEnd; ++i)
    {
        const WCHAR* pszSrc = (const WCHAR*)((PBYTE)pCandList + pCandList->dwOffset[i]);
        size_t cchSrc = wcslen(pszSrc);
        UINT strWidth = IntGetStringWidth(pszSrc);
        BOOL bTruncated = FALSE;

        if (currentX + strWidth + 3 > width - labelWidth)
        {
            UINT tmpW = 0, ichSrc = 0;
            for (ichSrc = 0; ichSrc < cchSrc; ++ichSrc)
            {
                tmpW += IntIsDoubleWidthChar(pszSrc[ichSrc]) + 1;
                if (currentX + tmpW > width - labelWidth - 3)
                    break;
            }

            cchSrc = (ichSrc > 0) ? ichSrc - 1 : 0;
            strWidth = tmpW;
            bTruncated = TRUE;
        }

        if (currentX + strWidth + labelWidth + 3 > width)
            break;

        if (i == pCandList->dwSelection)
            FillMemory(pbCurrentAttr, cchSrc + 2, FOREGROUND_BLUE);

        *pszCurrentPos++ = (WCHAR)(L'0' + candNum);
        *pszCurrentPos++ = L':';
        CopyMemory(pszCurrentPos, pszSrc, cchSrc * sizeof(WCHAR));
        pszCurrentPos += cchSrc;
        *pszCurrentPos++ = L' ';

        currentX += strWidth + 3;
        pbCurrentAttr += cchSrc + 3;
        candNum++;

        if (bTruncated)
            break;
    }

    *pszCurrentPos = UNICODE_NULL;

    UINT totalWidth = IntGetStringWidth(pszCandStrDest);
    UINT usableWidth = width - labelWidth;

    if (totalWidth <= usableWidth)
    {
        if (bIsCode)
        {
            if (totalWidth < width)
            {
                UINT pad = width - totalWidth;
                wmemset(pszCurrentPos, L' ', pad);
                pszCurrentPos += pad;
            }
        }
        else
        {
            if (totalWidth < usableWidth)
            {
                UINT pad = usableWidth - totalWidth;
                wmemset(pszCurrentPos, L' ', pad);
                pszCurrentPos += pad;
            }

            UINT halfLabel = (labelWidth - 1) / 2;

            IntFormatNumber(pszCurrentPos, pCandList->dwSelection + 1, halfLabel);
            pszCurrentPos += halfLabel;

            *pszCurrentPos++ = L'/';

            IntFormatNumber(pszCurrentPos, pCandList->dwCount, halfLabel);
            pszCurrentPos += halfLabel;
        }

        *pszCurrentPos = UNICODE_NULL;
    }

    return pageIndex;
}

UINT IntFormatCandLineCHT(
    PCANDIDATELIST pCandList,
    PWSTR pszCandStrDest,
    PBYTE pbAttrsDest,
    UINT width,
    UINT labelWidth,
    PCONENTRY pEntry)
{
    if (pCandList->dwSelection >= pCandList->dwCount)
        pCandList->dwSelection = 0;

    UINT pageIndex;
    for (pageIndex = pEntry->dwCandIndexMax; pageIndex > 0; --pageIndex)
    {
        if (pCandList->dwSelection >= pEntry->pdwCandPageStart[pageIndex])
            break;
    }

    ZeroMemory(pbAttrsDest, width);

    PWSTR pszCurrentStr = pszCandStrDest;
    PBYTE pbCurrentAttr = pbAttrsDest;

    DWORD iStart = pEntry->pdwCandPageStart[pageIndex];
    DWORD iEnd = pEntry->pdwCandPageStart[pageIndex + 1];

    UINT currentX = 0;
    UINT usableWidth = width - labelWidth;
    WORD candidateNum = !!(pEntry->dwImeProp & IME_PROP_CANDLIST_START_FROM_1);

    for (DWORD i = iStart; i < iEnd; ++i)
    {
        const WCHAR* pszSrc = (const WCHAR*)((PBYTE)pCandList + pCandList->dwOffset[i]);
        size_t cchSrc = wcslen(pszSrc);
        UINT strWidth = IntGetStringWidth(pszSrc);
        BOOL bTruncated = FALSE;

        if (currentX + strWidth + 3 > usableWidth)
        {
            size_t tmpW = 0, ichSrc;
            for (ichSrc = 0; ichSrc < cchSrc; ++ichSrc)
            {
                tmpW += IntIsDoubleWidthChar(pszSrc[ichSrc]) + 1;
                if (currentX + tmpW > usableWidth - 3)
                    break;
            }

            cchSrc = (ichSrc > 0) ? ichSrc - 1 : 0;
            strWidth = tmpW;
            bTruncated = TRUE;
        }

        if (currentX + strWidth + labelWidth + 3 > width)
            break;

        if (i == pCandList->dwSelection)
            FillMemory(pbCurrentAttr, cchSrc + 2, FOREGROUND_BLUE);

        *pszCurrentStr++ = (WCHAR)(L'0' + candidateNum);
        *pszCurrentStr++ = L':';

        CopyMemory(pszCurrentStr, pszSrc, cchSrc * sizeof(WCHAR));
        pszCurrentStr += cchSrc;
        *pszCurrentStr++ = L' ';

        currentX += strWidth + 3;
        pbCurrentAttr += cchSrc + 3;
        candidateNum++;

        if (bTruncated)
            break;
    }

    *pszCurrentStr = UNICODE_NULL;

    if (IntGetStringWidth(pszCandStrDest) <= usableWidth)
    {
        *pszCurrentStr = L' ';
        PWSTR pszLabelPos = pszCurrentStr + 1;

        UINT halfLabel = (labelWidth - 1) / 2;

        IntFormatNumber(pszLabelPos, pCandList->dwSelection + 1, halfLabel);

        pszLabelPos[halfLabel] = L'/';

        IntFormatNumber(&pszLabelPos[halfLabel + 1], pCandList->dwCount, halfLabel);
        pszLabelPos[labelWidth] = UNICODE_NULL;
    }

    return pageIndex;
}

UINT IntFormatCandLineCHS(
    PCANDIDATELIST pCandList,
    PWSTR pszCandStrDest,
    PBYTE pbAttrsDest,
    UINT width,
    UINT labelWidth,
    PCONENTRY pEntry)
{
    if (pCandList->dwSelection >= pCandList->dwCount)
        pCandList->dwSelection = 0;

    UINT pageIndex;
    for (pageIndex = pEntry->dwCandIndexMax; pageIndex > 0; --pageIndex)
    {
        if (pCandList->dwSelection >= pEntry->pdwCandPageStart[pageIndex])
            break;
    }

    ZeroMemory(pbAttrsDest, width);

    PWSTR pszCurrentPos = pszCandStrDest;
    PBYTE pbCurrentAttr = pbAttrsDest;
    UINT currentX = 0;
    UINT usableWidth = width - labelWidth;
    WORD candidateNum = !!(pEntry->dwImeProp & IME_PROP_CANDLIST_START_FROM_1);

    DWORD iStart = pEntry->pdwCandPageStart[pageIndex];
    DWORD iEnd = pEntry->pdwCandPageStart[pageIndex + 1];
    for (DWORD i = iStart; i < iEnd; ++i)
    {
        const WCHAR* pszSrc = (const WCHAR*)((PBYTE)pCandList + pCandList->dwOffset[i]);
        size_t cchSrc = wcslen(pszSrc);
        UINT strWidth = IntGetStringWidth(pszSrc);
        BOOL bTruncated = FALSE;

        if (strWidth + currentX + 3 > usableWidth)
        {
            size_t tmpW = 0, ichSrc = 0;
            for (ichSrc = 0; ichSrc < cchSrc; ++ichSrc)
            {
                tmpW += IntIsDoubleWidthChar(pszSrc[ichSrc]) + 1;
                if (currentX + tmpW > usableWidth - 3)
                    break;
            }

            cchSrc = (ichSrc > 0) ? ichSrc - 1 : 0;
            strWidth = tmpW;
            bTruncated = TRUE;
        }

        if (currentX + strWidth + labelWidth + 3 > width)
            break;

        if (i == pCandList->dwSelection)
            FillMemory(pbCurrentAttr, cchSrc + 2, FOREGROUND_BLUE);

        *pszCurrentPos++ = (WCHAR)(L'0' + candidateNum);
        *pszCurrentPos++ = L':';
        CopyMemory(pszCurrentPos, pszSrc, cchSrc * sizeof(WCHAR));
        pszCurrentPos += cchSrc;
        *pszCurrentPos++ = L' ';

        currentX += strWidth + 3;
        pbCurrentAttr += cchSrc + 3;
        candidateNum++;

        if (bTruncated)
            break;
    }

    *pszCurrentPos = UNICODE_NULL;
    return pageIndex;
}

BOOL IntSendCandListCHT(HWND hwnd, HIMC hIMC, PCONENTRY pEntry, DWORD dwCandidates, BOOL bOpen)
{
    PIMEDISPLAY pDisplay = LocalAlloc(LPTR, sizeof(IMEDISPLAY));
    if (!pDisplay)
        return FALSE;

    for (DWORD dwIndex = 0; dwIndex < MAX_CANDLIST; ++dwIndex)
    {
        if (!(dwCandidates & (1 << dwIndex)))
            continue;

        DWORD cbList = ImmGetCandidateListW(hIMC, dwIndex, NULL, 0);
        if (cbList == 0)
            continue;

        if (pEntry->apCandList[dwIndex] && pEntry->acbCandList[dwIndex] != cbList)
        {
            LocalFree(pEntry->apCandList[dwIndex]);
            pEntry->apCandList[dwIndex] = NULL;
        }

        if (!pEntry->apCandList[dwIndex])
        {
            pEntry->apCandList[dwIndex] = LocalAlloc(LPTR, cbList);
            if (!pEntry->apCandList[dwIndex])
                continue;
            pEntry->acbCandList[dwIndex] = cbList;
        }

        PCANDIDATELIST pCandList = pEntry->apCandList[dwIndex];
        ImmGetCandidateListW(hIMC, dwIndex, pCandList, cbList);

        UINT screenX = max(min(pEntry->ScreenSize.X, 128), 12);
        UINT usableWidth = screenX - 8;
        if (usableWidth < 7)
            usableWidth = 7;

        UINT maxItemsPerPage = (usableWidth - 7) / 5;
        maxItemsPerPage = min(max(maxItemsPerPage, 1), 9);

        UINT numDigits = 0;
        for (UINT tmpCount = 1; tmpCount <= pCandList->dwCount; (tmpCount *= 10), ++numDigits)
        {
            if (tmpCount > MAXDWORD / 10)
                break;
        }

        UINT labelWidth = 2 * numDigits + 1;

        DWORD nPageCountNeeded;
        if ((pCandList->dwCount / maxItemsPerPage + 10) >= 100)
            nPageCountNeeded = 100;
        else
            nPageCountNeeded = (pCandList->dwCount / maxItemsPerPage + 10);

        if (pEntry->pdwCandPageStart &&
            pEntry->cbCandPageData != nPageCountNeeded * sizeof(DWORD))
        {
            LocalFree(pEntry->pdwCandPageStart);
            pEntry->pdwCandPageStart = NULL;
        }

        if (!pEntry->pdwCandPageStart)
        {
            pEntry->pdwCandPageStart = LocalAlloc(LPTR, nPageCountNeeded * sizeof(DWORD));
            if (!pEntry->pdwCandPageStart)
            {
                LocalFree(pDisplay);
                return FALSE;
            }
            pEntry->cbCandPageData = nPageCountNeeded * sizeof(DWORD);
        }

        if (bOpen)
            pEntry->dwCandOffset = 0;

        pEntry->pdwCandPageStart[0] = pEntry->dwCandOffset;

        PWCHAR firstStr = (PWCHAR)((PBYTE)pCandList + pCandList->dwOffset[0]);
        UINT currentX = IntGetStringWidth(firstStr) + 3;

        UINT iItem = 0, iCand = 1;
        for (iItem = 1; iItem < pCandList->dwCount && iCand < nPageCountNeeded - 1; ++iItem)
        {
            PWCHAR szText = (PWCHAR)((PBYTE)pCandList + pCandList->dwOffset[iItem]);
            UINT strWidth = IntGetStringWidth(szText);

            if (currentX + strWidth + 3 > (usableWidth - labelWidth))
            {
                pEntry->pdwCandPageStart[iCand++] = iItem;
                currentX = strWidth + 3;
            }
            else
            {
                currentX += strWidth + 3;
            }
        }

        pEntry->pdwCandPageStart[iCand] = pCandList->dwCount;
        pEntry->dwCandIndexMax = iCand;

        UINT cbCandInfo = 3 * usableWidth + 4;
        if (pEntry->dwSystemLineSize < cbCandInfo)
        {
            if (pEntry->pCandInfo)
                LocalFree(pEntry->pCandInfo);
            pEntry->pCandInfo = LocalAlloc(LPTR, cbCandInfo);
            if (!pEntry->pCandInfo)
            {
                LocalFree(pDisplay);
                return FALSE;
            }
            pEntry->dwSystemLineSize = cbCandInfo;
        }

        PCANDINFO pCI = pEntry->pCandInfo;
        pCI->dwAttrsOffset = 2 * usableWidth + 4;

        PBYTE pbAttrs = (PBYTE)pCI + pCI->dwAttrsOffset;
        UINT iPage = IntFormatCandLineCHT(pCandList, pCI->szCandStr, pbAttrs, usableWidth,
                                          labelWidth, pEntry);

        // Send page messages
        pEntry->bSkipPageMsg = TRUE;
        ImmNotifyIME(hIMC, NI_SETCANDIDATE_PAGESTART, dwIndex, pEntry->pdwCandPageStart[iPage]);
        ImmNotifyIME(hIMC, NI_SETCANDIDATE_PAGESIZE, dwIndex,
                     pEntry->pdwCandPageStart[iPage + 1] - pEntry->pdwCandPageStart[iPage]);
        pEntry->bSkipPageMsg = FALSE;

        COPYDATASTRUCT CopyData;
        CopyData.dwData = MAGIC_SEND_IMEDISPLAY;
        CopyData.cbData = sizeof(*pDisplay);
        CopyData.lpData = pDisplay;

        if (IntFillImeDisplayCHT(pEntry, pDisplay))
            IntSendDataToConsole(pEntry->hwndConsole, hwnd, &CopyData);
    }

    LocalFree(pDisplay);
    return TRUE;
}

BOOL IntSendCandListCHS(HWND hwnd, HIMC hIMC, PCONENTRY pEntry, DWORD dwCandidates, BOOL bOpen)
{
    PIMEDISPLAY pDisplay = LocalAlloc(LPTR, sizeof(IMEDISPLAY));
    if (!pDisplay)
        return FALSE;

    for (DWORD dwIndex = 0; dwIndex < MAX_CANDLIST; ++dwIndex)
    {
        if (!(dwCandidates & (1 << dwIndex)))
            continue;

        DWORD cbList = ImmGetCandidateListW(hIMC, dwIndex, NULL, 0);
        if (!cbList)
        {
            LocalFree(pDisplay);
            return FALSE;
        }

        if (pEntry->apCandList[dwIndex] && pEntry->acbCandList[dwIndex] != cbList)
        {
            LocalFree(pEntry->apCandList[dwIndex]);
            pEntry->apCandList[dwIndex] = NULL;
            pEntry->acbCandList[dwIndex] = 0;
        }

        if (!pEntry->apCandList[dwIndex])
        {
            pEntry->apCandList[dwIndex] = LocalAlloc(LPTR, cbList);
            if (!pEntry->apCandList[dwIndex])
            {
                LocalFree(pDisplay);
                return FALSE;
            }
            pEntry->acbCandList[dwIndex] = cbList;
        }

        PCANDIDATELIST pCandList = pEntry->apCandList[dwIndex];
        ImmGetCandidateListW(hIMC, dwIndex, pCandList, cbList);

        UINT screenX = max(min(pEntry->ScreenSize.X, 128), 12);

        INT usableWidth = (INT)screenX - 25;

        UINT maxItemsPerPage;
        if (usableWidth <= 7)
            maxItemsPerPage = 1;
        else
            maxItemsPerPage = (UINT)((usableWidth - 7) / 5);

        if (maxItemsPerPage > 9)
            maxItemsPerPage = 9;

        DWORD nPageCountNeeded = pCandList->dwCount / maxItemsPerPage + 10;
        if (nPageCountNeeded > 100)
            nPageCountNeeded = 100;

        if (pEntry->pdwCandPageStart &&
            pEntry->cbCandPageData != nPageCountNeeded * sizeof(DWORD))
        {
            LocalFree(pEntry->pdwCandPageStart);
            pEntry->pdwCandPageStart = NULL;
        }
        if (!pEntry->pdwCandPageStart)
        {
            pEntry->pdwCandPageStart = LocalAlloc(LPTR, nPageCountNeeded * sizeof(DWORD));
            if (!pEntry->pdwCandPageStart)
            {
                LocalFree(pDisplay);
                return FALSE;
            }
            pEntry->cbCandPageData = nPageCountNeeded * sizeof(DWORD);
        }

        if (bOpen)
            pEntry->dwCandOffset = 0;

        pEntry->pdwCandPageStart[0] = pEntry->dwCandOffset;

        PWCHAR szFirstStr = (PWCHAR)((PBYTE)pCandList + pCandList->dwOffset[0]);
        UINT currentX = IntGetStringWidth(szFirstStr) + 3;

        UINT iItem = 0, iCand = 1;
        for (iItem = 1; iItem < pCandList->dwCount && iCand < nPageCountNeeded - 1; ++iItem)
        {
            WCHAR* szText = (WCHAR*)((BYTE*)pCandList + pCandList->dwOffset[iItem]);
            UINT strWidth = IntGetStringWidth(szText);

            if (currentX + strWidth + 3 > usableWidth ||
                (iItem - pEntry->pdwCandPageStart[iCand-1]) >= 9)
            {
                pEntry->pdwCandPageStart[iCand++] = iItem;
                currentX = strWidth + 3;
            }
            else
            {
                currentX += strWidth + 3;
            }
        }

        pEntry->pdwCandPageStart[iCand] = pCandList->dwCount;
        pEntry->dwCandIndexMax = iCand;

        DWORD cbCandInfo = 3 * usableWidth + 4;
        if (pEntry->dwSystemLineSize < cbCandInfo)
        {
            if (pEntry->pCandInfo)
                LocalFree(pEntry->pCandInfo);
            pEntry->pCandInfo = LocalAlloc(LPTR, cbCandInfo);
            if (!pEntry->pCandInfo)
            {
                LocalFree(pDisplay);
                return FALSE;
            }
            pEntry->dwSystemLineSize = cbCandInfo;
        }

        PCANDINFO pCI = pEntry->pCandInfo;
        pCI->dwAttrsOffset = 2 * usableWidth + 4;

        PBYTE pbAttrs = (PBYTE)pCI + pCI->dwAttrsOffset;
        UINT iPage = IntFormatCandLineCHS(pCandList, pCI->szCandStr, pbAttrs, usableWidth, 0,
                                          pEntry);

        // Send page messages
        pEntry->bSkipPageMsg = TRUE;
        ImmNotifyIME(hIMC, NI_SETCANDIDATE_PAGESTART, dwIndex, pEntry->pdwCandPageStart[iPage]);
        ImmNotifyIME(hIMC, NI_SETCANDIDATE_PAGESIZE, dwIndex,
                     pEntry->pdwCandPageStart[iPage + 1] - pEntry->pdwCandPageStart[iPage]);
        pEntry->bSkipPageMsg = FALSE;

        COPYDATASTRUCT CopyData;
        CopyData.dwData = MAGIC_SEND_IMEDISPLAY;
        CopyData.cbData = sizeof(*pDisplay);
        CopyData.lpData = pDisplay;

        if (IntFillImeDisplayCHS(pEntry, pDisplay))
            IntSendDataToConsole(pEntry->hwndConsole, hwnd, &CopyData);
    }

    LocalFree(pDisplay);
    return TRUE;
}

BOOL
IntSendCandListJPNorKOR(HWND hwnd, HIMC hIMC, PCONENTRY pEntry, DWORD dwCandidates, BOOL bOpen)
{
    for (DWORD dwIndex = 0; dwIndex < MAX_CANDLIST; ++dwIndex)
    {
        if (!(dwCandidates & (1 << dwIndex)))
            continue;

        DWORD cbList = ImmGetCandidateListW(hIMC, dwIndex, NULL, 0);
        if (!cbList)
            return FALSE;

        if (pEntry->apCandList[dwIndex] && pEntry->acbCandList[dwIndex] != cbList)
        {
            LocalFree(pEntry->apCandList[dwIndex]);
            pEntry->apCandList[dwIndex] = NULL;
        }

        if (!pEntry->apCandList[dwIndex])
        {
            pEntry->apCandList[dwIndex] = LocalAlloc(LPTR, cbList);
            if (!pEntry->apCandList[dwIndex])
                return FALSE;
            pEntry->acbCandList[dwIndex] = cbList;
        }

        PCANDIDATELIST pCandList = pEntry->apCandList[dwIndex];
        ImmGetCandidateListW(hIMC, dwIndex, pCandList, cbList);

        UINT screenX = max(min(pEntry->ScreenSize.X, 128), 12);

        UINT maxItems = (screenX - 7) / 5;
        if (maxItems > 9) maxItems = 9;

        BOOL bIsCode = (pCandList->dwStyle == IME_CAND_CODE);
        UINT labelWidth;
        if (bIsCode)
        {
            labelWidth = 7;
        }
        else
        {
            UINT digits = 0;
            for (UINT tmp = 1; tmp <= pCandList->dwCount; tmp *= 10)
            {
                if (tmp > MAXDWORD / 10)
                    break;
                ++digits;
            }
            labelWidth = 2 * digits + 1;
        }

        DWORD nPageCountNeeded = (pCandList->dwCount / maxItems + 10);
        if (nPageCountNeeded > 100)
            nPageCountNeeded = 100;

        if (pEntry->pdwCandPageStart &&
            pEntry->cbCandPageData != nPageCountNeeded * sizeof(DWORD))
        {
            LocalFree(pEntry->pdwCandPageStart);
            pEntry->pdwCandPageStart = NULL;
        }

        if (!pEntry->pdwCandPageStart)
        {
            pEntry->pdwCandPageStart = LocalAlloc(LPTR, nPageCountNeeded * sizeof(DWORD));
            if (!pEntry->pdwCandPageStart)
                return FALSE;
            pEntry->cbCandPageData = nPageCountNeeded * sizeof(DWORD);
        }

        DWORD iItem, iCand = 1;

        if (bIsCode)
        {
            if (bOpen)
                pEntry->dwCandOffset = pCandList->dwSelection % 9;

            pEntry->pdwCandPageStart[0] = 0;
            for (iItem = pEntry->dwCandOffset; iItem < pCandList->dwCount; iItem += 9)
                pEntry->pdwCandPageStart[iCand++] = iItem;

            if (iItem > pCandList->dwCount)
                iItem = pCandList->dwCount;
        }
        else
        {
            if (bOpen)
                pEntry->dwCandOffset = 0;

            pEntry->pdwCandPageStart[0] = pEntry->dwCandOffset;

            UINT currentX =
                IntGetStringWidth((PWCHAR)((PBYTE)pCandList + pCandList->dwOffset[0])) + 3;
            UINT usableWidth = screenX - labelWidth;

            for (DWORD i = 1; i < pCandList->dwCount; ++i)
            {
                UINT strWidth =
                    IntGetStringWidth((PWCHAR)((PBYTE)pCandList + pCandList->dwOffset[i]));
                if (currentX + strWidth + 3 > usableWidth ||
                    (i - pEntry->pdwCandPageStart[iCand - 1]) >= 9)
                {
                    pEntry->pdwCandPageStart[iCand++] = i;
                    currentX = strWidth + 3;
                }
                else
                {
                    currentX += strWidth + 3;
                }
            }

            iItem = pCandList->dwCount;
        }

        pEntry->pdwCandPageStart[iCand] = iItem;
        pEntry->dwCandIndexMax = iCand;

        DWORD cbCandInfo = 3 * screenX + 4;
        if (pEntry->dwSystemLineSize < cbCandInfo)
        {
            if (pEntry->pCandInfo)
                LocalFree(pEntry->pCandInfo);
            pEntry->pCandInfo = LocalAlloc(LPTR, cbCandInfo);
            if (!pEntry->pCandInfo)
                return FALSE;
            pEntry->dwSystemLineSize = cbCandInfo;
        }

        PCANDINFO pCI = pEntry->pCandInfo;
        pCI->dwAttrsOffset = 2 * screenX + 4;

        PBYTE pbAttrs = (PBYTE)pCI + pCI->dwAttrsOffset;
        UINT iPage = IntFormatCandLineJPNorKOR(pCandList, pCI->szCandStr, pbAttrs,
                                               screenX, labelWidth, pEntry, bIsCode);

        // Send page messages
        pEntry->bSkipPageMsg = TRUE;
        ImmNotifyIME(hIMC, NI_SETCANDIDATE_PAGESTART, dwIndex, pEntry->pdwCandPageStart[iPage]);
        ImmNotifyIME(hIMC, NI_SETCANDIDATE_PAGESIZE, dwIndex,
                     pEntry->pdwCandPageStart[iPage + 1] - pEntry->pdwCandPageStart[iPage]);
        pEntry->bSkipPageMsg = FALSE;

        COPYDATASTRUCT CopyData;
        CopyData.dwData = MAGIC_SEND_CANDLIST;
        CopyData.cbData = cbCandInfo;
        CopyData.lpData = pCI;
        IntSendDataToConsole(pEntry->hwndConsole, hwnd, &CopyData);
    }

    return TRUE;
}

//! When the conversion candidate window opens, invoke the language-specific candidate
//  list process.
BOOL ConIme_OnNotifyOpenCandidate(HWND hwnd, LPARAM lParam, BOOL bOpen)
{
    PCONENTRY pEntry = IntFindConsoleEntry(g_hConsole);
    if (!pEntry)
        return FALSE;

    if (pEntry->bSkipPageMsg)
        return TRUE;

    HIMC hIMC = ImmGetContext(hwnd);
    if (!hIMC)
        return FALSE;

    pEntry->bHasAnyCand = TRUE;

    LANGID wLangId = LOWORD(pEntry->hKL);
    switch (wLangId)
    {
        case LANGID_CHINESE_SIMPLIFIED:
            IntSendCandListCHS(hwnd, hIMC, pEntry, (DWORD)lParam, bOpen);
            break;
        case LANGID_CHINESE_TRADITIONAL:
            IntSendCandListCHT(hwnd, hIMC, pEntry, (DWORD)lParam, bOpen);
            break;
        case LANGID_JAPANESE:
        case LANGID_KOREAN:
            IntSendCandListJPNorKOR(hwnd, hIMC, pEntry, (DWORD)lParam, bOpen);
            break;
        default:
            ImmReleaseContext(hwnd, hIMC);
            return FALSE;
    }

    ImmReleaseContext(hwnd, hIMC);
    return TRUE;
}

inline BOOL ConIme_OnNotifyChangeCandidate(HWND hwnd, LPARAM lParam)
{
    return ConIme_OnNotifyOpenCandidate(hwnd, lParam, FALSE);
}

//! Handles guideline notifications (error messages, etc.) from IME
BOOL ConIme_OnNotifyGuideLine(HWND hWnd)
{
    PCONENTRY pEntry = IntFindConsoleEntry(g_hConsole);
    if (!pEntry)
        return FALSE;

    HIMC hIMC = ImmGetContext(hWnd);
    if (!hIMC)
        return FALSE;

    COPYDATASTRUCT CopyData;
    CopyData.dwData = MAGIC_SEND_GUIDELINE;

    BOOL ret = FALSE;
    size_t cbGuideLine = ImmGetGuideLineW(hIMC, GGL_STRING, NULL, 0);
    if (!cbGuideLine)
    {
        CopyData.cbData = 0;
        CopyData.lpData = NULL;
        IntSendDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);
        ret = TRUE;
    }
    else
    {
        size_t cbAlloc = cbGuideLine + sizeof(UNICODE_NULL);
        PWSTR pszGuideLine = LocalAlloc(LPTR, cbAlloc);
        if (pszGuideLine)
        {
            ImmGetGuideLineW(hIMC, GGL_STRING, pszGuideLine, cbGuideLine);

            CopyData.cbData = cbAlloc;
            CopyData.lpData = pszGuideLine;
            IntSendDataToConsole(pEntry->hwndConsole, hWnd, &CopyData);

            LocalFree(pszGuideLine);
            ret = TRUE;
        }
    }

    ImmReleaseContext(hWnd, hIMC);
    return ret;
}

//! WM_USER_GETIMESTATE
DWORD IntGetImeState(HWND hWnd, HANDLE hConsole)
{
    PCONENTRY pEntry = IntFindConsoleEntry(hConsole);
    if (!pEntry)
        return 0;

    HIMC hIMC = ImmGetContext(hWnd);
    if (!hIMC)
        return _IME_CMODE_DEACTIVATE;

    ImmGetConversionStatus(hIMC, &pEntry->dwConversion, &pEntry->dwSentence);
    pEntry->bOpened = IntIsImeOpen(hIMC, pEntry);
    ImmReleaseContext(hWnd, hIMC);
    return pEntry->dwConversion + (pEntry->bOpened ? _IME_CMODE_OPEN : 0);
}

//! WM_USER_SETIMESTATE
BOOL IntSetImeState(HWND hwnd, HANDLE hConsole, DWORD dwConversion)
{
    PCONENTRY pEntry = IntFindConsoleEntry(hConsole);
    if (!pEntry)
        return FALSE;

    if (dwConversion & _IME_CMODE_DEACTIVATE)
    {
        ImmSetActiveContextConsoleIME(hwnd, FALSE);
        ImmAssociateContext(hwnd, NULL);
        pEntry->hOldIMC = NULL;
    }
    else
    {
        ImmAssociateContext(hwnd, pEntry->hNewIMC);
        ImmSetActiveContextConsoleIME(hwnd, TRUE);
        pEntry->hOldIMC = pEntry->hNewIMC;
    }

    HIMC hIMC = ImmGetContext(hwnd);
    if (!hIMC)
        return TRUE;

    BOOL bOpened = !!(dwConversion & _IME_CMODE_OPEN);
    pEntry->bOpened = bOpened;
    ImmSetOpenStatus(hIMC, bOpened);

    DWORD dwImeConversion = (dwConversion & ~_IME_CMODE_MASK);
    if (pEntry->dwConversion != dwImeConversion)
    {
        pEntry->dwConversion = dwImeConversion;
        ImmSetConversionStatus(hIMC, dwImeConversion, pEntry->dwSentence);
    }

    ImmReleaseContext(hwnd, hIMC);
    return TRUE;
}

//! WM_USER_SETCODEPAGE
BOOL ConIme_SetCodePage(HWND hwnd, HANDLE hConsole, BOOL bOutput, WORD wCodePage)
{
    PCONENTRY pEntry = IntFindConsoleEntry(hConsole);
    if (!pEntry)
        return FALSE;

    if (bOutput)
        pEntry->nOutputCodePage = wCodePage;
    else
        pEntry->nCodePage = wCodePage;

    return TRUE;
}

//! WM_IME_SYSTEM
BOOL ConIme_OnImeSystem(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PCONENTRY pEntry = IntFindConsoleEntry(g_hConsole);
    if (!pEntry)
        return FALSE;

    COPYDATASTRUCT CopyData;
    CopyData.lpData = &wParam;
    CopyData.dwData = MAGIC_SEND_IMESYSTEM;
    CopyData.cbData = sizeof(wParam);
    IntSendDataToConsole(pEntry->hwndConsole, hwnd, &CopyData);
    return TRUE;
}

//! Sends candidate list for Traditional Chinese (CHT).
BOOL IntCloseCandsCHT(HWND hwnd, HIMC hIMC, PCONENTRY pEntry, DWORD dwCandidates)
{
    PIMEDISPLAY pDisplay = LocalAlloc(LPTR, sizeof(IMEDISPLAY));
    if (!pDisplay)
        return FALSE;

    PCANDIDATELIST* apCandList = pEntry->apCandList;
    for (DWORD iCand = 0; iCand < MAX_CANDLIST; ++iCand)
    {
        if ((dwCandidates & (1 << iCand)) && apCandList[iCand])
        {
            LocalFree(apCandList[iCand]);
            apCandList[iCand] = NULL;
            pEntry->acbCandList[iCand] = 0;
        }
    }

    COPYDATASTRUCT CopyData;
    CopyData.dwData = MAGIC_SEND_IMEDISPLAY;
    CopyData.cbData = sizeof(*pDisplay);
    CopyData.lpData = pDisplay;
    if (IntFillImeDisplayCHT(pEntry, pDisplay))
        IntSendDataToConsole(pEntry->hwndConsole, hwnd, &CopyData);

    LocalFree(pDisplay);
    return TRUE;
}

//! Sends the IME candidate list for Japanese/Korean.
BOOL IntCloseCandsJPNorKOR(HWND hwnd, HIMC hIMC, PCONENTRY pEntry, DWORD dwCandidates)
{
    PCANDIDATELIST* apCandList = pEntry->apCandList;
    for (DWORD iCand = 0; iCand < MAX_CANDLIST; ++iCand)
    {
        if ((dwCandidates & (1 << iCand)) && apCandList[iCand])
        {
            LocalFree(apCandList[iCand]);
            apCandList[iCand] = NULL;
            pEntry->acbCandList[iCand] = 0;
        }
    }

    COPYDATASTRUCT CopyData = { MAGIC_SEND_CANDLIST };
    IntSendDataToConsole(pEntry->hwndConsole, hwnd, &CopyData);
    return TRUE;
}

//! Sends the IME candidate list for Simplified Chinese (CHS).
BOOL IntCloseCandsCHS(HWND hwnd, HIMC hIMC, PCONENTRY pEntry, DWORD dwCandidates)
{
    PIMEDISPLAY pDisplay = LocalAlloc(LPTR, sizeof(IMEDISPLAY));
    if (!pDisplay)
        return FALSE;

    PCANDIDATELIST* apCandList = pEntry->apCandList;
    for (DWORD iCand = 0; iCand < MAX_CANDLIST; ++iCand)
    {
        if ((dwCandidates & (1 << iCand)) && apCandList[iCand])
        {
            LocalFree(apCandList[iCand]);
            apCandList[iCand] = NULL;
            pEntry->acbCandList[iCand] = 0;
        }
    }

    COPYDATASTRUCT CopyData;
    CopyData.dwData = MAGIC_SEND_IMEDISPLAY;
    CopyData.cbData = sizeof(*pDisplay);
    CopyData.lpData = pDisplay;
    if (IntFillImeDisplayCHS(pEntry, pDisplay))
        IntSendDataToConsole(pEntry->hwndConsole, hwnd, &CopyData);

    LocalFree(pDisplay);
    return TRUE;
}

//! IMN_CLOSECANDIDATE
BOOL ConIme_OnNotifyCloseCandidate(HWND hwnd, DWORD dwCandidates)
{
    PCONENTRY pEntry = IntFindConsoleEntry(g_hConsole);
    if (!pEntry)
        return FALSE;

    HIMC hIMC = ImmGetContext(hwnd);
    if (!hIMC)
        return FALSE;

    pEntry->bHasAnyCand = FALSE;

    LANGID wLang = LOWORD(pEntry->hKL);
    switch (wLang)
    {
        case LANGID_CHINESE_SIMPLIFIED:
            IntCloseCandsCHS(hwnd, hIMC, pEntry, dwCandidates);
            break;
        case LANGID_CHINESE_TRADITIONAL:
            IntCloseCandsCHT(hwnd, hIMC, pEntry, dwCandidates);
            break;
        case LANGID_JAPANESE:
        case LANGID_KOREAN:
            IntCloseCandsJPNorKOR(hwnd, hIMC, pEntry, dwCandidates);
            break;
        default:
            ImmReleaseContext(hwnd, hIMC);
            return FALSE;
    }

    ImmReleaseContext(hwnd, hIMC);
    return TRUE;
}

//! WM_IME_NOTIFY
BOOL ConIme_OnImeNotify(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
        case IMN_OPENSTATUSWINDOW:
            TRACE("IMN_OPENSTATUSWINDOW\n");
            ConIme_SendImeStatus(hWnd);
            break;
        case IMN_OPENCANDIDATE:
            TRACE("IMN_OPENCANDIDATE\n");
            ConIme_OnNotifyOpenCandidate(hWnd, lParam, TRUE);
            break;
        case IMN_CHANGECANDIDATE:
            TRACE("IMN_CHANGECANDIDATE\n");
            ConIme_OnNotifyChangeCandidate(hWnd, lParam);
            break;
        case IMN_CLOSECANDIDATE:
            TRACE("IMN_CLOSECANDIDATE\n");
            ConIme_OnNotifyCloseCandidate(hWnd, (DWORD)lParam);
            break;
        case IMN_SETCONVERSIONMODE:
            TRACE("IMN_SETCONVERSIONMODE\n");
            ConIme_SendImeStatus(hWnd);
            return FALSE; // Return FALSE to allow default processing to continue
        case IMN_SETOPENSTATUS:
            TRACE("IMN_SETOPENSTATUS\n");
            ConIme_OnNotifySetOpenStatus(hWnd);
            return FALSE;
        case IMN_GUIDELINE:
            TRACE("IMN_GUIDELINE\n");
            ConIme_OnNotifyGuideLine(hWnd);
            break;
        default:
            return FALSE;
    }

    return TRUE;
}

BOOL ConIme_OnKeyChar(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%p, %u, %p, %p)\n", hwnd, uMsg, wParam, lParam);
    PCONENTRY pEntry = IntFindConsoleEntry(g_hConsole);
    if (pEntry)
        return PostMessageW(pEntry->hwndConsole, uMsg + WM_ROUTE, wParam, lParam);
    return FALSE;
}

//! WM_ROUTE_...
LRESULT ConIme_OnRoute(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%p, %u, %p, %p)\n", hwnd, uMsg, wParam, lParam);

    WPARAM wch = wParam;
    if (HIWORD(wParam))
    {
        if (uMsg == WM_ROUTE_KEYDOWN || uMsg == WM_ROUTE_KEYUP ||
            uMsg == WM_ROUTE_SYSKEYDOWN || uMsg == WM_ROUTE_SYSKEYUP)
        {
            wch = UNICODE_NULL;
        }
        else if (HIWORD(wParam) <= 0xFF)
        {
            wch = HIWORD(wParam);
        }
        else
        {
            WCHAR wsz[2] = { HIWORD(wParam), 0 };
            CHAR ach;
            WideCharToMultiByte(CP_ACP, 0, wsz, 1, &ach, 1, NULL, NULL);
            wch = ach;
        }
    }

    UINT vkey, uKeyMsg = ((HIWORD(lParam) & KF_UP) ? WM_KEYUP : WM_KEYDOWN);
    LRESULT ret = ImmCallImeConsoleIME(hwnd, uKeyMsg, wParam, lParam, &vkey);

    if (!(ret & IPHK_HOTKEY))
    {
        if (ret & IPHK_PROCESSBYIME)
            return ImmTranslateMessage(hwnd, uKeyMsg, wParam, lParam);
        else if ((ret & IPHK_CHECKCTRL) || uMsg == WM_ROUTE_CHAR || uMsg == WM_ROUTE_SYSCHAR)
            return ConIme_OnKeyChar(hwnd, uMsg - WM_ROUTE, wch, lParam);
        else
            return ConIme_OnKeyChar(hwnd, uMsg - WM_ROUTE, (UINT)wParam, lParam);
    }

    return ret;
}

//! WM_USER_SIMHOTKEY
BOOL ConIme_SimulateHotKey(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACE("WM_USER_SIMHOTKEY\n");
    return ImmSimulateHotKey(hwnd, (DWORD)lParam);
}

//! WM_ENABLE
void ConIme_OnEnable(void)
{
    EnterCriticalSection(&g_csLock);
    for (UINT iEntry = 1; iEntry < g_cEntries; ++iEntry)
    {
        PCONENTRY pEntry = g_ppEntries[iEntry];
        if (pEntry && pEntry->hConsole)
        {
            // Enable the console window and bring it to the front
            if (!pEntry->bConsoleEnabled && !IsWindowEnabled(pEntry->hwndConsole))
            {
                EnableWindow(pEntry->hwndConsole, TRUE);
                pEntry->bConsoleEnabled = TRUE;
                if (!pEntry->bWndEnabled)
                    SetForegroundWindow(pEntry->hwndConsole);
            }
        }
    }
    LeaveCriticalSection(&g_csLock);
}

//! WM_ENABLE
void ConIme_OnDisable(void)
{
    PCONENTRY pEntry = IntFindConsoleEntry(g_hConsole);
    if (pEntry && pEntry->hConsole)
    {
        pEntry->bConsoleEnabled = FALSE;
        EnableWindow(pEntry->hwndConsole, FALSE);
        g_bDisabled = TRUE;
    }
}

//! WM_IME_STARTCOMPOSITION
void ConIme_OnImeStartComposition(HWND hwnd)
{
    TRACE("WM_IME_STARTCOMPOSITION\n");
    PCONENTRY pEntry = IntFindConsoleEntry(g_hConsole);
    if (pEntry)
        pEntry->bInComposition = TRUE;
}

//! WM_IME_ENDCOMPOSITION
void ConIme_OnImeEndComposition(HWND hWnd)
{
    TRACE("WM_IME_ENDCOMPOSITION\n");

    // Find the currently active console entry
    PCONENTRY pEntry = IntFindConsoleEntry(g_hConsole);
    if (!pEntry)
        return;

    pEntry->bInComposition = FALSE;

    // Free composition string info
    if (pEntry->pCompStr)
    {
        LocalFree(pEntry->pCompStr);
        pEntry->pCompStr = NULL;
    }
}

//! WM_USER_CHANGEKEYBOARD
void ConIme_OnChangeKeyboard(HWND hwnd, HANDLE hConsole, HKL hNewKL)
{
    PCONENTRY pEntry = IntFindConsoleEntry(hConsole);
    if (!pEntry)
        pEntry = IntFindConsoleEntry(g_hConsole);
    if (!pEntry || !pEntry->pKLInfo)
        return;

    HKL hOldKL = pEntry->hKL;

    if (IS_IME_HKL(hOldKL)) // IME HKL?
    {
        INT iKL;
        PKLINFO pKLInfo = pEntry->pKLInfo;

        for (iKL = 0; iKL < pEntry->cKLs; ++iKL)
        {
            if (!pKLInfo[iKL].hKL || pKLInfo[iKL].hKL == hOldKL)
                break;
        }

        if (iKL >= pEntry->cKLs)
        {
            INT newCount = pEntry->cKLs + 1;
            PKLINFO phNewKLs = LocalAlloc(LPTR, newCount * sizeof(KLINFO));
            if (!phNewKLs)
                return;

            CopyMemory(phNewKLs, pEntry->pKLInfo, pEntry->cKLs * sizeof(KLINFO));
            LocalFree(pEntry->pKLInfo);
            pEntry->pKLInfo = phNewKLs;
            pEntry->cKLs = newCount;
        }

        pEntry->pKLInfo[iKL].hKL = hOldKL;

        DWORD dwConversion = pEntry->dwConversion | (pEntry->bOpened ? _IME_CMODE_OPEN : 0);
        pEntry->pKLInfo[iKL].dwConversion = dwConversion;
    }

    ActivateKeyboardLayout(hNewKL, 0);
    pEntry->hKL = hNewKL;

    IntGetImeLayoutText(pEntry);
    ConIme_SendImeStatus(hwnd);
    pEntry->dwImeProp = ImmGetProperty(pEntry->hKL, IGP_PROPERTY);

    PIMEDISPLAY pDisplay = LocalAlloc(LPTR, sizeof(IMEDISPLAY));
    if (!pDisplay)
        return;

    COPYDATASTRUCT CopyData;
    CopyData.dwData = MAGIC_SEND_IMEDISPLAY;
    CopyData.cbData = sizeof(*pDisplay);
    CopyData.lpData = pDisplay;

    if (IS_IME_HKL(hNewKL)) // IME HKL?
    {
        for (INT iKL = 0; iKL < pEntry->cKLs; ++iKL)
        {
            if (pEntry->pKLInfo[iKL].hKL != hNewKL)
                continue;

            IntSetImeState(hwnd, hConsole, pEntry->pKLInfo[iKL].dwConversion);
            ConIme_SendImeStatus(hwnd);

            if (IntFillImeDisplay(pEntry, pDisplay))
                IntSendDataToConsole(pEntry->hwndConsole, hwnd, &CopyData);

            break;
        }
    }
    else // Non-IME HKL?
    {
        IntSetImeState(hwnd, hConsole, pEntry->dwConversion & ~_IME_CMODE_OPEN);
        pDisplay->uCharInfoLen = 0;
        pDisplay->bFlag = TRUE;
        IntSendDataToConsole(pEntry->hwndConsole, hwnd, &CopyData);
    }

    LocalFree(pDisplay);
}

//! (WM_USER + ...)
LRESULT ConIme_OnUser(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%p, %u, %p, %p)\n", hWnd, uMsg, wParam, lParam);
    switch (uMsg)
    {
        case WM_USER_INIT:
            return ConIme_OnInit(hWnd, (HANDLE)wParam, (HWND)lParam);
        case WM_USER_UNINIT:
            return ConIme_OnUnInit(hWnd, (HANDLE)wParam);
        case WM_USER_SWITCHIME:
            return ConIme_OnSwitchIme(hWnd, (HANDLE)wParam, (HKL)lParam);
        case WM_USER_DEACTIVATE:
            return ConIme_OnDeactivate(hWnd, (HANDLE)wParam);
        case WM_USER_SIMHOTKEY:
            return ConIme_SimulateHotKey(hWnd, wParam, lParam);
        case WM_USER_GETIMESTATE:
            return IntGetImeState(hWnd, (HANDLE)wParam);
        case WM_USER_SETIMESTATE:
            return IntSetImeState(hWnd, (HANDLE)wParam, (DWORD)lParam);
        case WM_USER_SETSCREENSIZE:
        {
            COORD coord = { LOWORD(lParam), HIWORD(lParam) };
            return ConIme_SetScreenSize(hWnd, (HANDLE)wParam, coord);
        }
        case WM_USER_SENDIMESTATUS:
            return ConIme_SendImeStatus(hWnd);
        case WM_USER_CHANGEKEYBOARD:
            ConIme_OnChangeKeyboard(hWnd, (HANDLE)wParam, (HKL)lParam);
            return TRUE;
        case WM_USER_SETCODEPAGE:
            return ConIme_SetCodePage(hWnd, (HANDLE)wParam, LOWORD(lParam), HIWORD(lParam));
        case WM_USER_GO:
            return ConIme_OnGo(hWnd, (HANDLE)wParam, (HKL)lParam, 0);
        case WM_USER_GONEXT:
            return ConIme_OnGo(hWnd, (HANDLE)wParam, (HKL)lParam, +1);
        case WM_USER_GOBACK:
            return ConIme_OnGo(hWnd, (HANDLE)wParam, (HKL)lParam, -1);
        default:
            ERR("Unknown uMsg %u\n", uMsg);
            return FALSE;
    }
}

//! WM_INPUTLANGCHANGEREQUEST
LRESULT ConIme_OnInputLangChangeRequest(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%p, %p, %p)\n", hWnd, wParam, lParam);

    PCONENTRY pEntry = IntFindConsoleEntry(g_hConsole);
    if (!pEntry)
        return FALSE;

    // Forward messages to the console window
    PostMessageW(pEntry->hwndConsole, WM_INPUTLANGCHANGEREQUEST, wParam, lParam);
    return TRUE;
}

//! The main window procedure for the Console IME.
LRESULT CALLBACK
ConIme_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            return (ConIme_OnCreate(hWnd) ? 0 : -1);

        case WM_DESTROY:
            ConIme_OnEnd(hWnd, uMsg);
            PostQuitMessage(0);
            break;

        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;

        case WM_QUERYENDSESSION:
            return TRUE;

        case WM_ENDSESSION:
            ConIme_OnEnd(hWnd, uMsg);
            break;

        case WM_INPUTLANGCHANGE:
            ConIme_OnInputLangChange(hWnd, wParam, (HKL)lParam);
            return TRUE;

        case WM_INPUTLANGCHANGEREQUEST:
            if (ConIme_OnInputLangChangeRequest(hWnd, wParam, lParam))
                return TRUE;
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);

        case WM_ENABLE:
            if (wParam) // Enabled?
                ConIme_OnEnable();
            else
                ConIme_OnDisable();
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);

        case WM_IME_STARTCOMPOSITION:
            ConIme_OnImeStartComposition(hWnd);
            return TRUE;

        case WM_IME_ENDCOMPOSITION:
            ConIme_OnImeEndComposition(hWnd);
            return TRUE;

        case WM_IME_COMPOSITION:
            ConIme_OnImeComposition(hWnd, wParam, lParam);
            return TRUE;

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
        case WM_DEADCHAR:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_SYSCHAR:
        case WM_SYSDEADCHAR:
            ConIme_OnKeyChar(hWnd, uMsg, wParam, lParam);
            return TRUE;

        case WM_IME_SETCONTEXT:
            return DefWindowProcW(hWnd, uMsg, wParam, (lParam & ~ISC_SHOWUIALL));

        case WM_IME_NOTIFY:
            if (!ConIme_OnImeNotify(hWnd, wParam, lParam))
                return DefWindowProcW(hWnd, uMsg, wParam, lParam);
            return TRUE;

        case WM_IME_COMPOSITIONFULL:
            return TRUE;

        case WM_IME_SYSTEM:
            if (wParam == IMS_CONSOLEIME_1A || wParam == IMS_CONSOLEIME_1B)
            {
                ConIme_OnImeSystem(hWnd, wParam, lParam);
                return TRUE;
            }
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);

        default:
        {
            if (WM_USER_INIT <= uMsg && uMsg <= WM_USER_GOBACK)
                return ConIme_OnUser(hWnd, uMsg, wParam, lParam);

            if (WM_ROUTE_KEYDOWN <= uMsg && uMsg <= WM_ROUTE_SYSDEADCHAR)
            {
                ConIme_OnRoute(hWnd, uMsg, wParam, lParam);
                return TRUE;
            }

            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
        }
    }

    return 0;
}

//! Is Console IME on system process enabled?
BOOL IntIsConImeOnSystemProcessEnabled(VOID)
{
    BOOL bIsConImeOnSystemProcessEnabled = FALSE;

    HKEY hKey;
    LSTATUS error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                  L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Console",
                                  0, KEY_QUERY_VALUE, &hKey);
    if (error == ERROR_SUCCESS)
    {
        DWORD dwValue = FALSE, cbValue = sizeof(dwValue);
        error = RegQueryValueExW(hKey, L"EnableConImeOnSystemProcess", NULL, NULL,
                                 (PBYTE)&dwValue, &cbValue);
        if (error == ERROR_SUCCESS)
            bIsConImeOnSystemProcessEnabled = !!dwValue;
        RegCloseKey(hKey);
    }

    return bIsConImeOnSystemProcessEnabled;
}

//! Initialize Console IME instance
BOOL ConIme_InitInstance(HINSTANCE hInstance)
{
    BOOL bIsConImeOnSystemProcessEnabled = IntIsConImeOnSystemProcessEnabled();
    if (!bIsConImeOnSystemProcessEnabled && IntIsLogOnSession())
        return FALSE;

    const UINT cEntries = 10;
    g_ppEntries = LocalAlloc(LPTR, cEntries * sizeof(PCONENTRY));
    if (!g_ppEntries)
        return FALSE;

    InitializeCriticalSection(&g_csLock);

    g_cEntries = cEntries;

    // Open a startup synchronization event
    HANDLE hStartUpEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, L"ConsoleIME_StartUp_Event");
    if (!hStartUpEvent)
    {
        DeleteCriticalSection(&g_csLock);
        return FALSE;
    }

    INT x, y, cx, cy, cxScreen, cyMenu;
    HWND hWnd;

    // Register window class
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc   = ConIme_WndProc;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_MAINICON));
    wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"ConsoleIMEClass";
    ATOM atom = RegisterClassExW(&wc);
    if (!atom)
        goto Cleanup1;

    // Create main window
    cxScreen = GetSystemMetrics(SM_CXSCREEN);
    cyMenu = GetSystemMetrics(SM_CYMENU);
    x = cxScreen - (cxScreen / 3);
    y = cyMenu;
    cx = cxScreen / 3;
    cy = 10 * cyMenu;
    hWnd = CreateWindowW(L"ConsoleIMEClass", L"", WS_OVERLAPPEDWINDOW,
                         x, y, cx, cy, NULL, NULL, hInstance, NULL);
    if (!hWnd)
        goto Cleanup2;

    // Console IME registration and threaded input synchronization
    if (RegisterConsoleIME(hWnd, &g_dwAttachToThreadId) &&
        AttachThreadInput(GetCurrentThreadId(), g_dwAttachToThreadId, TRUE))
    {
        SetEvent(hStartUpEvent);
        CloseHandle(hStartUpEvent);
        return TRUE; // Success
    }

    if (g_dwAttachToThreadId)
        UnregisterConsoleIME();

    if (hWnd)
        DestroyWindow(hWnd);

Cleanup2:
    if (atom)
        UnregisterClassW(L"ConsoleIMEClass", hInstance);

Cleanup1:
    if (hStartUpEvent)
    {
        SetEvent(hStartUpEvent); // Set to release the waiting side even if it fails
        CloseHandle(hStartUpEvent);
    }

    DeleteCriticalSection(&g_csLock);
    return FALSE; // Failed
}

INT WINAPI
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, INT nCmdShow)
{
    WCHAR szSysDir[MAX_PATH];
    if (GetSystemDirectoryW(szSysDir, _countof(szSysDir)))
        SetCurrentDirectoryW(szSysDir);

    ImmDisableTextFrameService(0xFFFFFFFF);

    if (!ConIme_InitInstance(hInstance))
    {
        ERR("ConIme_InitInstance failed\n");
        IntFreeConsoleEntries();
        return 0;
    }

    MSG msg;
    _SEH2_TRY
    {
        // Main loop
        while (GetMessageW(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("Exception\n");
        msg.wParam = -1;
    }
    _SEH2_END;

    IntFreeConsoleEntries();
    DeleteCriticalSection(&g_csLock);

    return (INT)msg.wParam;
}
