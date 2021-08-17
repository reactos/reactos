/*
 * IMM32 library
 *
 * Copyright 1998 Patrik Stridvall
 * Copyright 2002, 2003, 2007 CodeWeavers, Aric Stewart
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winerror.h>
#include <wine/debug.h>
#include <imm.h>
#include <ddk/imm.h>
#include <winnls.h>
#include <winreg.h>
#include <wine/list.h>
#include <stdlib.h>
#include <ndk/umtypes.h>
#include <ndk/pstypes.h>
#include <ndk/rtlfuncs.h>
#include "../../../win32ss/include/ntuser.h"
#include "../../../win32ss/include/ntwin32.h"
#include <imm32_undoc.h>
#include <strsafe.h>

WINE_DEFAULT_DEBUG_CHANNEL(imm);

#define IMM_INIT_MAGIC 0x19650412
#define IMM_INVALID_CANDFORM ULONG_MAX

#define MAX_CANDIDATEFORM 4

#define LANGID_CHINESE_SIMPLIFIED MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)
#define LANGID_CHINESE_TRADITIONAL MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)
#define LANGID_JAPANESE MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT)

#define REGKEY_KEYBOARD_LAYOUTS \
    L"System\\CurrentControlSet\\Control\\Keyboard Layouts"
#define REGKEY_IMM \
    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\IMM"

#define ROUNDUP4(n) (((n) + 3) & ~3)  /* DWORD alignment */

HMODULE g_hImm32Inst = NULL;
RTL_CRITICAL_SECTION g_csImeDpi;
PIMEDPI g_pImeDpiList = NULL;
PSERVERINFO g_psi = NULL;
SHAREDINFO g_SharedInfo = { NULL };
BYTE g_bClientRegd = FALSE;
HANDLE g_hImm32Heap = NULL;

static BOOL APIENTRY Imm32InitInstance(HMODULE hMod)
{
    NTSTATUS status;

    if (hMod)
        g_hImm32Inst = hMod;

    if (g_bClientRegd)
        return TRUE;

    status = RtlInitializeCriticalSection(&g_csImeDpi);
    if (NT_ERROR(status))
        return FALSE;

    g_bClientRegd = TRUE;
    return TRUE;
}

LPVOID APIENTRY Imm32HeapAlloc(DWORD dwFlags, DWORD dwBytes)
{
    if (!g_hImm32Heap)
    {
        g_hImm32Heap = RtlGetProcessHeap();
        if (g_hImm32Heap == NULL)
            return NULL;
    }
    return HeapAlloc(g_hImm32Heap, dwFlags, dwBytes);
}

static LPWSTR APIENTRY Imm32WideFromAnsi(LPCSTR pszA)
{
    INT cch = lstrlenA(pszA);
    LPWSTR pszW = Imm32HeapAlloc(0, (cch + 1) * sizeof(WCHAR));
    if (pszW == NULL)
        return NULL;
    cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszA, cch, pszW, cch + 1);
    pszW[cch] = 0;
    return pszW;
}

static LPSTR APIENTRY Imm32AnsiFromWide(LPCWSTR pszW)
{
    INT cchW = lstrlenW(pszW);
    INT cchA = (cchW + 1) * sizeof(WCHAR);
    LPSTR pszA = Imm32HeapAlloc(0, cchA);
    if (!pszA)
        return NULL;
    cchA = WideCharToMultiByte(CP_ACP, 0, pszW, cchW, pszA, cchA, NULL, NULL);
    pszA[cchA] = 0;
    return pszA;
}

static inline BOOL Imm32IsCrossThreadAccess(HIMC hIMC)
{
    DWORD dwImeThreadId = NtUserQueryInputContext(hIMC, 1);
    DWORD dwThreadId = GetCurrentThreadId();
    return (dwImeThreadId != dwThreadId);
}

static VOID APIENTRY Imm32FreeImeDpi(PIMEDPI pImeDpi, BOOL bDestroy)
{
    if (pImeDpi->hInst == NULL)
        return;
    if (bDestroy)
        pImeDpi->ImeDestroy(0);
    FreeLibrary(pImeDpi->hInst);
    pImeDpi->hInst = NULL;
}

static BOOL APIENTRY
Imm32NotifyAction(HIMC hIMC, HWND hwnd, DWORD dwAction, DWORD_PTR dwIndex, DWORD_PTR dwValue,
                  DWORD_PTR dwCommand, DWORD_PTR dwData)
{
    DWORD dwLayout;
    HKL hKL;
    PIMEDPI pImeDpi;

    if (dwAction)
    {
        dwLayout = NtUserQueryInputContext(hIMC, 1);
        if (dwLayout)
        {
            /* find keyboard layout and lock it */
            hKL = GetKeyboardLayout(dwLayout);
            pImeDpi = ImmLockImeDpi(hKL);
            if (pImeDpi)
            {
                /* do notify */
                pImeDpi->NotifyIME(hIMC, dwAction, dwIndex, dwValue);

                ImmUnlockImeDpi(pImeDpi); /* unlock */
            }
        }
    }

    if (hwnd && dwCommand)
        SendMessageW(hwnd, WM_IME_NOTIFY, dwCommand, dwData);

    return TRUE;
}

static PIMEDPI APIENTRY Imm32FindImeDpi(HKL hKL)
{
    PIMEDPI pImeDpi;

    RtlEnterCriticalSection(&g_csImeDpi);
    for (pImeDpi = g_pImeDpiList; pImeDpi != NULL; pImeDpi = pImeDpi->pNext)
    {
        if (pImeDpi->hKL == hKL)
            break;
    }
    RtlLeaveCriticalSection(&g_csImeDpi);

    return pImeDpi;
}

static BOOL Imm32GetSystemLibraryPath(LPWSTR pszPath, DWORD cchPath, LPCWSTR pszFileName)
{
    if (!pszFileName[0] || !GetSystemDirectoryW(pszPath, cchPath))
        return FALSE;
    StringCchCatW(pszPath, cchPath, L"\\");
    StringCchCatW(pszPath, cchPath, pszFileName);
    return TRUE;
}

static BOOL APIENTRY Imm32InquireIme(PIMEDPI pImeDpi)
{
    WCHAR szUIClass[64];
    WNDCLASSW wcW;
    DWORD dwSysInfoFlags = 0; // TODO: ???
    LPIMEINFO pImeInfo = &pImeDpi->ImeInfo;

    // TODO: NtUserGetThreadState(16);

    if (!IS_IME_HKL(pImeDpi->hKL))
    {
        if (g_psi && (g_psi->dwSRVIFlags & SRVINFO_CICERO_ENABLED) &&
            pImeDpi->CtfImeInquireExW)
        {
            // TODO:
            return FALSE;
        }
    }

    if (!pImeDpi->ImeInquire(pImeInfo, szUIClass, dwSysInfoFlags))
        return FALSE;

    szUIClass[_countof(szUIClass) - 1] = 0;

    if (pImeInfo->dwPrivateDataSize == 0)
        pImeInfo->dwPrivateDataSize = 4;

#define VALID_IME_PROP (IME_PROP_AT_CARET              | \
                        IME_PROP_SPECIAL_UI            | \
                        IME_PROP_CANDLIST_START_FROM_1 | \
                        IME_PROP_UNICODE               | \
                        IME_PROP_COMPLETE_ON_UNSELECT  | \
                        IME_PROP_END_UNLOAD            | \
                        IME_PROP_KBD_CHAR_FIRST        | \
                        IME_PROP_IGNORE_UPKEYS         | \
                        IME_PROP_NEED_ALTKEY           | \
                        IME_PROP_NO_KEYS_ON_CLOSE      | \
                        IME_PROP_ACCEPT_WIDE_VKEY)
#define VALID_CMODE_CAPS (IME_CMODE_ALPHANUMERIC | \
                          IME_CMODE_NATIVE       | \
                          IME_CMODE_KATAKANA     | \
                          IME_CMODE_LANGUAGE     | \
                          IME_CMODE_FULLSHAPE    | \
                          IME_CMODE_ROMAN        | \
                          IME_CMODE_CHARCODE     | \
                          IME_CMODE_HANJACONVERT | \
                          IME_CMODE_SOFTKBD      | \
                          IME_CMODE_NOCONVERSION | \
                          IME_CMODE_EUDC         | \
                          IME_CMODE_SYMBOL       | \
                          IME_CMODE_FIXED)
#define VALID_SMODE_CAPS (IME_SMODE_NONE          | \
                          IME_SMODE_PLAURALCLAUSE | \
                          IME_SMODE_SINGLECONVERT | \
                          IME_SMODE_AUTOMATIC     | \
                          IME_SMODE_PHRASEPREDICT | \
                          IME_SMODE_CONVERSATION)
#define VALID_UI_CAPS (UI_CAP_2700    | \
                       UI_CAP_ROT90   | \
                       UI_CAP_ROTANY  | \
                       UI_CAP_SOFTKBD)
#define VALID_SCS_CAPS (SCS_CAP_COMPSTR            | \
                        SCS_CAP_MAKEREAD           | \
                        SCS_CAP_SETRECONVERTSTRING)
#define VALID_SELECT_CAPS (SELECT_CAP_CONVERSION | SELECT_CAP_SENTENCE)

    if (pImeInfo->fdwProperty & ~VALID_IME_PROP)
        return FALSE;
    if (pImeInfo->fdwConversionCaps & ~VALID_CMODE_CAPS)
        return FALSE;
    if (pImeInfo->fdwSentenceCaps & ~VALID_SMODE_CAPS)
        return FALSE;
    if (pImeInfo->fdwUICaps & ~VALID_UI_CAPS)
        return FALSE;
    if (pImeInfo->fdwSCSCaps & ~VALID_SCS_CAPS)
        return FALSE;
    if (pImeInfo->fdwSelectCaps & ~VALID_SELECT_CAPS)
        return FALSE;

#undef VALID_IME_PROP
#undef VALID_CMODE_CAPS
#undef VALID_SMODE_CAPS
#undef VALID_UI_CAPS
#undef VALID_SCS_CAPS
#undef VALID_SELECT_CAPS

    if (pImeInfo->fdwProperty & IME_PROP_UNICODE)
    {
        StringCchCopyW(pImeDpi->szUIClass, _countof(pImeDpi->szUIClass), szUIClass);
    }
    else
    {
        if (pImeDpi->uCodePage != GetACP() && pImeDpi->uCodePage)
            return FALSE;

        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPSTR)szUIClass, -1,
                            pImeDpi->szUIClass, _countof(pImeDpi->szUIClass));
    }

    return GetClassInfoW(pImeDpi->hInst, pImeDpi->szUIClass, &wcW);
}

static BOOL APIENTRY Imm32LoadImeInfo(PIMEINFOEX pImeInfoEx, PIMEDPI pImeDpi)
{
    WCHAR szPath[MAX_PATH];
    HINSTANCE hIME;
    FARPROC fn;

    if (!Imm32GetSystemLibraryPath(szPath, _countof(szPath), pImeInfoEx->wszImeFile))
        return FALSE;

    hIME = GetModuleHandleW(szPath);
    if (hIME == NULL)
    {
        hIME = LoadLibraryW(szPath);
        if (hIME == NULL)
        {
            ERR("Imm32LoadImeInfo: LoadLibraryW(%S) failed\n", szPath);
            return FALSE;
        }
    }
    pImeDpi->hInst = hIME;

#define DEFINE_IME_ENTRY(type, name, params, extended) \
    do { \
        fn = GetProcAddress(hIME, #name); \
        if (fn) pImeDpi->name = (FN_##name)fn; \
        else if (!extended) goto Failed; \
    } while (0);
#include "../../../win32ss/include/imetable.h"
#undef DEFINE_IME_ENTRY

    if (!Imm32InquireIme(pImeDpi))
    {
        ERR("Imm32LoadImeInfo: Imm32InquireIme failed\n");
        goto Failed;
    }

    if (pImeInfoEx->fLoadFlag)
        return TRUE;

    NtUserSetImeOwnerWindow(pImeInfoEx, TRUE);
    return TRUE;

Failed:
    FreeLibrary(pImeDpi->hInst);
    pImeDpi->hInst = NULL;
    return FALSE;
}

static DWORD APIENTRY
Imm32JTransCompA(LPINPUTCONTEXTDX pIC, LPCOMPOSITIONSTRING pCS,
                 const TRANSMSG *pSrc, LPTRANSMSG pDest)
{
    // FIXME
    *pDest = *pSrc;
    return 1;
}

static DWORD APIENTRY
Imm32JTransCompW(LPINPUTCONTEXTDX pIC, LPCOMPOSITIONSTRING pCS,
                 const TRANSMSG *pSrc, LPTRANSMSG pDest)
{
    // FIXME
    *pDest = *pSrc;
    return 1;
}

typedef LRESULT (WINAPI *FN_SendMessage)(HWND, UINT, WPARAM, LPARAM);

static DWORD APIENTRY
Imm32JTrans(DWORD dwCount, LPTRANSMSG pTrans, LPINPUTCONTEXTDX pIC,
            LPCOMPOSITIONSTRING pCS, BOOL bAnsi)
{
    DWORD ret = 0;
    HWND hWnd, hwndDefIME;
    LPTRANSMSG pSrcTrans, pEntry, pNext;
    DWORD dwIndex, iCandForm, dwNumber, cbNewTrans;
    HGLOBAL hGlobal;
    CANDIDATEFORM CandForm;
    FN_SendMessage pSendMessage;

    hWnd = pIC->hWnd;
    hwndDefIME = ImmGetDefaultIMEWnd(hWnd);
    pSendMessage = (IsWindowUnicode(hWnd) ? SendMessageW : SendMessageA);

    // clone the message list
    cbNewTrans = (dwCount + 1) * sizeof(TRANSMSG);
    pSrcTrans = Imm32HeapAlloc(HEAP_ZERO_MEMORY, cbNewTrans);
    if (pSrcTrans == NULL)
        return 0;
    RtlCopyMemory(pSrcTrans, pTrans, dwCount * sizeof(TRANSMSG));

    if (pIC->dwUIFlags & 0x2)
    {
        // find WM_IME_ENDCOMPOSITION
        for (dwIndex = 0; dwIndex < dwCount; ++dwIndex, ++pEntry)
        {
            if (pEntry->message == WM_IME_ENDCOMPOSITION)
                break;
        }

        if (pEntry->message == WM_IME_ENDCOMPOSITION) // if found
        {
            // move WM_IME_ENDCOMPOSITION to the end of the list
            for (pNext = pEntry + 1; pNext->message != 0; ++pEntry, ++pNext)
                *pEntry = *pNext;

            pEntry->message = WM_IME_ENDCOMPOSITION;
            pEntry->wParam = 0;
            pEntry->lParam = 0;
        }
    }

    for (pEntry = pSrcTrans; pEntry->message != 0; ++pEntry)
    {
        switch (pEntry->message)
        {
            case WM_IME_STARTCOMPOSITION:
                if (!(pIC->dwUIFlags & 0x2))
                {
                    // send IR_OPENCONVERT
                    if (pIC->cfCompForm.dwStyle != CFS_DEFAULT)
                        pSendMessage(hWnd, WM_IME_REPORT, IR_OPENCONVERT, 0);

                    goto DoDefault;
                }
                break;

            case WM_IME_ENDCOMPOSITION:
                if (pIC->dwUIFlags & 0x2)
                {
                    // send IR_UNDETERMINE
                    hGlobal = GlobalAlloc(GHND | GMEM_SHARE, 0x38);
                    if (hGlobal)
                    {
                        pSendMessage(hWnd, WM_IME_REPORT, IR_UNDETERMINE, (LPARAM)hGlobal);
                        GlobalFree(hGlobal);
                    }
                }
                else
                {
                    // send IR_CLOSECONVERT
                    if (pIC->cfCompForm.dwStyle != CFS_DEFAULT)
                        pSendMessage(hWnd, WM_IME_REPORT, IR_CLOSECONVERT, 0);

                    goto DoDefault;
                }
                break;

            case WM_IME_COMPOSITION:
                if (bAnsi)
                    dwNumber = Imm32JTransCompA(pIC, pCS, pEntry, pTrans);
                else
                    dwNumber = Imm32JTransCompW(pIC, pCS, pEntry, pTrans);

                ret += dwNumber;
                pTrans += dwNumber;

                if (!(pIC->dwUIFlags & 0x2))
                {
                    if (pIC->cfCompForm.dwStyle != CFS_DEFAULT)
                        pSendMessage(hWnd, WM_IME_REPORT, IR_CHANGECONVERT, 0);
                }
                break;

            case WM_IME_NOTIFY:
                if (pEntry->wParam == IMN_OPENCANDIDATE)
                {
                    if (IsWindow(hWnd) && (pIC->dwUIFlags & 0x2))
                    {
                        // send IMC_SETCANDIDATEPOS
                        for (iCandForm = 0; iCandForm < MAX_CANDIDATEFORM; ++iCandForm)
                        {
                            if (!(pEntry->lParam & (1 << iCandForm)))
                                continue;

                            CandForm.dwIndex = iCandForm;
                            CandForm.dwStyle = CFS_EXCLUDE;
                            CandForm.ptCurrentPos = pIC->cfCompForm.ptCurrentPos;
                            CandForm.rcArea = pIC->cfCompForm.rcArea;
                            pSendMessage(hwndDefIME, WM_IME_CONTROL, IMC_SETCANDIDATEPOS,
                                         (LPARAM)&CandForm);
                        }
                    }
                }

                if (!(pIC->dwUIFlags & 0x2))
                    goto DoDefault;

                // send a WM_IME_NOTIFY notification to the default ime window
                pSendMessage(hwndDefIME, pEntry->message, pEntry->wParam, pEntry->lParam);
                break;

DoDefault:
            default:
                // default processing
                *pTrans++ = *pEntry;
                ++ret;
                break;
        }
    }

    HeapFree(g_hImm32Heap, 0, pSrcTrans);
    return ret;
}

static DWORD APIENTRY
Imm32KTrans(DWORD dwCount, LPTRANSMSG pEntries, LPINPUTCONTEXTDX pIC,
            LPCOMPOSITIONSTRING pCS, BOOL bAnsi)
{
    return dwCount; // FIXME
}

static DWORD APIENTRY
Imm32Trans(DWORD dwCount, LPTRANSMSG pEntries, HIMC hIMC, BOOL bAnsi, WORD wLang)
{
    BOOL ret = FALSE;
    LPINPUTCONTEXTDX pIC;
    LPCOMPOSITIONSTRING pCS;

    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (pIC == NULL)
        return 0;

    pCS = ImmLockIMCC(pIC->hCompStr);
    if (pCS)
    {
        if (wLang == LANG_JAPANESE)
            ret = Imm32JTrans(dwCount, pEntries, pIC, pCS, bAnsi);
        else if (wLang == LANG_KOREAN)
            ret = Imm32KTrans(dwCount, pEntries, pIC, pCS, bAnsi);
        ImmUnlockIMCC(pIC->hCompStr);
    }

    ImmUnlockIMC(hIMC);
    return ret;
}

static PIMEDPI APIENTRY Ime32LoadImeDpi(HKL hKL, BOOL bLock)
{
    IMEINFOEX ImeInfoEx;
    CHARSETINFO ci;
    PIMEDPI pImeDpiNew, pImeDpiFound;
    UINT uCodePage;
    LCID lcid;

    if (!ImmGetImeInfoEx(&ImeInfoEx, ImeInfoExKeyboardLayout, &hKL) ||
        ImeInfoEx.fLoadFlag == 1)
    {
        return NULL;
    }

    pImeDpiNew = Imm32HeapAlloc(HEAP_ZERO_MEMORY, sizeof(IMEDPI));
    if (pImeDpiNew == NULL)
        return NULL;

    pImeDpiNew->hKL = hKL;

    lcid = LOWORD(hKL);
    if (TranslateCharsetInfo((LPDWORD)(DWORD_PTR)lcid, &ci, TCI_SRCLOCALE))
        uCodePage = ci.ciACP;
    else
        uCodePage = CP_ACP;
    pImeDpiNew->uCodePage = uCodePage;

    if (!Imm32LoadImeInfo(&ImeInfoEx, pImeDpiNew))
    {
        HeapFree(g_hImm32Heap, 0, pImeDpiNew);
        return FALSE;
    }

    RtlEnterCriticalSection(&g_csImeDpi);

    pImeDpiFound = Imm32FindImeDpi(hKL);
    if (pImeDpiFound)
    {
        if (!bLock)
            pImeDpiFound->dwFlags &= ~IMEDPI_FLAG_LOCKED;

        RtlLeaveCriticalSection(&g_csImeDpi);

        Imm32FreeImeDpi(pImeDpiNew, FALSE);
        HeapFree(g_hImm32Heap, 0, pImeDpiNew);
        return pImeDpiFound;
    }
    else
    {
        if (bLock)
        {
            pImeDpiNew->dwFlags |= IMEDPI_FLAG_LOCKED;
            pImeDpiNew->cLockObj = 1;
        }

        pImeDpiNew->pNext = g_pImeDpiList;
        g_pImeDpiList = pImeDpiNew;

        RtlLeaveCriticalSection(&g_csImeDpi);
        return pImeDpiNew;
    }
}

/***********************************************************************
 *		ImmLoadIME (IMM32.@)
 */
BOOL WINAPI ImmLoadIME(HKL hKL)
{
    PW32CLIENTINFO pInfo;
    PIMEDPI pImeDpi;

    if (!IS_IME_HKL(hKL))
    {
        if (!g_psi || !(g_psi->dwSRVIFlags & SRVINFO_CICERO_ENABLED))
            return FALSE;

        pInfo = (PW32CLIENTINFO)(NtCurrentTeb()->Win32ClientInfo);
        if ((pInfo->W32ClientInfo[0] & 2))
            return FALSE;
    }

    pImeDpi = Imm32FindImeDpi(hKL);
    if (pImeDpi == NULL)
        pImeDpi = Ime32LoadImeDpi(hKL, FALSE);
    return (pImeDpi != NULL);
}

PIMEDPI APIENTRY ImmLockOrLoadImeDpi(HKL hKL)
{
    PW32CLIENTINFO pInfo;
    PIMEDPI pImeDpi;

    if (!IS_IME_HKL(hKL))
    {
        if (!g_psi || !(g_psi->dwSRVIFlags & SRVINFO_CICERO_ENABLED))
            return NULL;

        pInfo = (PW32CLIENTINFO)(NtCurrentTeb()->Win32ClientInfo);
        if ((pInfo->W32ClientInfo[0] & 2))
            return NULL;
    }

    pImeDpi = ImmLockImeDpi(hKL);
    if (pImeDpi == NULL)
        pImeDpi = Ime32LoadImeDpi(hKL, TRUE);
    return pImeDpi;
}

/***********************************************************************
 *		ImmLoadLayout (IMM32.@)
 */
HKL WINAPI ImmLoadLayout(HKL hKL, PIMEINFOEX pImeInfoEx)
{
    DWORD cbData;
    UNICODE_STRING UnicodeString;
    HKEY hLayoutKey = NULL, hLayoutsKey = NULL;
    LONG error;
    NTSTATUS Status;
    WCHAR szLayout[MAX_PATH];

    TRACE("(%p, %p)\n", hKL, pImeInfoEx);

    if (IS_IME_HKL(hKL) ||
        !g_psi || !(g_psi->dwSRVIFlags & SRVINFO_CICERO_ENABLED) ||
        ((PW32CLIENTINFO)NtCurrentTeb()->Win32ClientInfo)->W32ClientInfo[0] & 2)
    {
        UnicodeString.Buffer = szLayout;
        UnicodeString.MaximumLength = sizeof(szLayout);
        Status = RtlIntegerToUnicodeString((DWORD_PTR)hKL, 16, &UnicodeString);
        if (!NT_SUCCESS(Status))
            return NULL;

        error = RegOpenKeyW(HKEY_LOCAL_MACHINE, REGKEY_KEYBOARD_LAYOUTS, &hLayoutsKey);
        if (error)
            return NULL;

        error = RegOpenKeyW(hLayoutsKey, szLayout, &hLayoutKey);
    }
    else
    {
        error = RegOpenKeyW(HKEY_LOCAL_MACHINE, REGKEY_IMM, &hLayoutKey);
    }

    if (error)
    {
        ERR("RegOpenKeyW error: 0x%08lX\n", error);
        hKL = NULL;
    }
    else
    {
        cbData = sizeof(pImeInfoEx->wszImeFile);
        error = RegQueryValueExW(hLayoutKey, L"Ime File", 0, 0,
                                 (LPBYTE)pImeInfoEx->wszImeFile, &cbData);
        if (error)
            hKL = NULL;
    }

    RegCloseKey(hLayoutKey);
    if (hLayoutsKey)
        RegCloseKey(hLayoutsKey);
    return hKL;
}

typedef struct _tagImmHkl{
    struct list entry;
    HKL         hkl;
    HMODULE     hIME;
    IMEINFO     imeInfo;
    WCHAR       imeClassName[17]; /* 16 character max */
    ULONG       uSelected;
    HWND        UIWnd;

    /* Function Pointers */
    BOOL (WINAPI *pImeInquire)(IMEINFO *, WCHAR *, const WCHAR *);
    BOOL (WINAPI *pImeConfigure)(HKL, HWND, DWORD, void *);
    BOOL (WINAPI *pImeDestroy)(UINT);
    LRESULT (WINAPI *pImeEscape)(HIMC, UINT, void *);
    BOOL (WINAPI *pImeSelect)(HIMC, BOOL);
    BOOL (WINAPI *pImeSetActiveContext)(HIMC, BOOL);
    UINT (WINAPI *pImeToAsciiEx)(UINT, UINT, const BYTE *, DWORD *, UINT, HIMC);
    BOOL (WINAPI *pNotifyIME)(HIMC, DWORD, DWORD, DWORD);
    BOOL (WINAPI *pImeRegisterWord)(const WCHAR *, DWORD, const WCHAR *);
    BOOL (WINAPI *pImeUnregisterWord)(const WCHAR *, DWORD, const WCHAR *);
    UINT (WINAPI *pImeEnumRegisterWord)(REGISTERWORDENUMPROCW, const WCHAR *, DWORD, const WCHAR *, void *);
    BOOL (WINAPI *pImeSetCompositionString)(HIMC, DWORD, const void *, DWORD, const void *, DWORD);
    DWORD (WINAPI *pImeConversionList)(HIMC, const WCHAR *, CANDIDATELIST *, DWORD, UINT);
    BOOL (WINAPI *pImeProcessKey)(HIMC, UINT, LPARAM, const BYTE *);
    UINT (WINAPI *pImeGetRegisterWordStyle)(UINT, STYLEBUFW *);
    DWORD (WINAPI *pImeGetImeMenuItems)(HIMC, DWORD, DWORD, IMEMENUITEMINFOW *, IMEMENUITEMINFOW *, DWORD);
} ImmHkl;

typedef struct tagInputContextData
{
        DWORD           dwLock;
        INPUTCONTEXT    IMC;
        DWORD           threadID;

        ImmHkl          *immKbd;
        UINT            lastVK;
        BOOL            threadDefault;
        DWORD           magic;
} InputContextData;

#define WINE_IMC_VALID_MAGIC 0x56434D49

typedef struct _tagIMMThreadData {
    struct list entry;
    DWORD threadID;
    HIMC defaultContext;
    HWND hwndDefault;
    BOOL disableIME;
    DWORD windowRefs;
} IMMThreadData;

static struct list ImmHklList = LIST_INIT(ImmHklList);
static struct list ImmThreadDataList = LIST_INIT(ImmThreadDataList);

static const WCHAR szwWineIMCProperty[] = {'W','i','n','e','I','m','m','H','I','M','C','P','r','o','p','e','r','t','y',0};

static const WCHAR szImeFileW[] = {'I','m','e',' ','F','i','l','e',0};
static const WCHAR szLayoutTextW[] = {'L','a','y','o','u','t',' ','T','e','x','t',0};
static const WCHAR szImeRegFmt[] = {'S','y','s','t','e','m','\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\','C','o','n','t','r','o','l','\\','K','e','y','b','o','a','r','d',' ','L','a','y','o','u','t','s','\\','%','0','8','l','x',0};

static const WCHAR szwIME[] = {'I','M','E',0};
static const WCHAR szwDefaultIME[] = {'D','e','f','a','u','l','t',' ','I','M','E',0};

static CRITICAL_SECTION threaddata_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &threaddata_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": threaddata_cs") }
};
static CRITICAL_SECTION threaddata_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

static inline BOOL is_himc_ime_unicode(const InputContextData *data)
{
    return !!(data->immKbd->imeInfo.fdwProperty & IME_PROP_UNICODE);
}

static inline BOOL is_kbd_ime_unicode(const ImmHkl *hkl)
{
    return !!(hkl->imeInfo.fdwProperty & IME_PROP_UNICODE);
}

static InputContextData* get_imc_data(HIMC hIMC);

static inline WCHAR *strdupAtoW( const char *str )
{
    WCHAR *ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

static inline CHAR *strdupWtoA( const WCHAR *str )
{
    CHAR *ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static HMODULE load_graphics_driver(void)
{
    static const WCHAR display_device_guid_propW[] = {
        '_','_','w','i','n','e','_','d','i','s','p','l','a','y','_',
        'd','e','v','i','c','e','_','g','u','i','d',0 };
    static const WCHAR key_pathW[] = {
        'S','y','s','t','e','m','\\',
        'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
        'C','o','n','t','r','o','l','\\',
        'V','i','d','e','o','\\','{',0};
    static const WCHAR displayW[] = {'}','\\','0','0','0','0',0};
    static const WCHAR driverW[] = {'G','r','a','p','h','i','c','s','D','r','i','v','e','r',0};

    HMODULE ret = 0;
    HKEY hkey;
    DWORD size;
    WCHAR path[MAX_PATH];
    WCHAR key[ARRAY_SIZE( key_pathW ) + ARRAY_SIZE( displayW ) + 40];
    UINT guid_atom = HandleToULong( GetPropW( GetDesktopWindow(), display_device_guid_propW ));

    if (!guid_atom) return 0;
    memcpy( key, key_pathW, sizeof(key_pathW) );
    if (!GlobalGetAtomNameW( guid_atom, key + lstrlenW(key), 40 )) return 0;
    lstrcatW( key, displayW );
    if (RegOpenKeyW( HKEY_LOCAL_MACHINE, key, &hkey )) return 0;
    size = sizeof(path);
    if (!RegQueryValueExW( hkey, driverW, NULL, NULL, (BYTE *)path, &size )) ret = LoadLibraryW( path );
    RegCloseKey( hkey );
    TRACE( "%s %p\n", debugstr_w(path), ret );
    return ret;
}

/* ImmHkl loading and freeing */
#define LOAD_FUNCPTR(f) if((ptr->p##f = (LPVOID)GetProcAddress(ptr->hIME, #f)) == NULL){WARN("Can't find function %s in ime\n", #f);}
static ImmHkl *IMM_GetImmHkl(HKL hkl)
{
    ImmHkl *ptr;
    WCHAR filename[MAX_PATH];

    TRACE("Seeking ime for keyboard %p\n",hkl);

    LIST_FOR_EACH_ENTRY(ptr, &ImmHklList, ImmHkl, entry)
    {
        if (ptr->hkl == hkl)
            return ptr;
    }
    /* not found... create it */

    ptr = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ImmHkl));

    ptr->hkl = hkl;
    if (ImmGetIMEFileNameW(hkl, filename, MAX_PATH)) ptr->hIME = LoadLibraryW(filename);
    if (!ptr->hIME) ptr->hIME = load_graphics_driver();
    if (ptr->hIME)
    {
        LOAD_FUNCPTR(ImeInquire);
        if (!ptr->pImeInquire || !ptr->pImeInquire(&ptr->imeInfo, ptr->imeClassName, NULL))
        {
            FreeLibrary(ptr->hIME);
            ptr->hIME = NULL;
        }
        else
        {
            LOAD_FUNCPTR(ImeDestroy);
            LOAD_FUNCPTR(ImeSelect);
            if (!ptr->pImeSelect || !ptr->pImeDestroy)
            {
                FreeLibrary(ptr->hIME);
                ptr->hIME = NULL;
            }
            else
            {
                LOAD_FUNCPTR(ImeConfigure);
                LOAD_FUNCPTR(ImeEscape);
                LOAD_FUNCPTR(ImeSetActiveContext);
                LOAD_FUNCPTR(ImeToAsciiEx);
                LOAD_FUNCPTR(NotifyIME);
                LOAD_FUNCPTR(ImeRegisterWord);
                LOAD_FUNCPTR(ImeUnregisterWord);
                LOAD_FUNCPTR(ImeEnumRegisterWord);
                LOAD_FUNCPTR(ImeSetCompositionString);
                LOAD_FUNCPTR(ImeConversionList);
                LOAD_FUNCPTR(ImeProcessKey);
                LOAD_FUNCPTR(ImeGetRegisterWordStyle);
                LOAD_FUNCPTR(ImeGetImeMenuItems);
                /* make sure our classname is WCHAR */
                if (!is_kbd_ime_unicode(ptr))
                {
                    WCHAR bufW[17];
                    MultiByteToWideChar(CP_ACP, 0, (LPSTR)ptr->imeClassName,
                                        -1, bufW, 17);
                    lstrcpyW(ptr->imeClassName, bufW);
                }
            }
        }
    }
    list_add_head(&ImmHklList,&ptr->entry);

    return ptr;
}
#undef LOAD_FUNCPTR

static InputContextData* get_imc_data(HIMC hIMC)
{
    InputContextData *data = (InputContextData *)hIMC;

    if (hIMC == NULL)
        return NULL;

    if(IsBadReadPtr(data, sizeof(InputContextData)) || data->magic != WINE_IMC_VALID_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }
    return data;
}

static HIMC get_default_context( HWND hwnd )
{
    FIXME("Don't use this function\n");
    return FALSE;
}

static BOOL IMM_IsCrossThreadAccess(HWND hWnd,  HIMC hIMC)
{
    InputContextData *data;

    if (hWnd)
    {
        DWORD thread = GetWindowThreadProcessId(hWnd, NULL);
        if (thread != GetCurrentThreadId()) return TRUE;
    }
    data = get_imc_data(hIMC);
    if (data && data->threadID != GetCurrentThreadId())
        return TRUE;

    return FALSE;
}

/***********************************************************************
 *		ImmAssociateContext (IMM32.@)
 */
HIMC WINAPI ImmAssociateContext(HWND hWnd, HIMC hIMC)
{
    HIMC old = NULL;
    InputContextData *data = get_imc_data(hIMC);

    TRACE("(%p, %p):\n", hWnd, hIMC);

    if(hIMC && !data)
        return NULL;

    /*
     * If already associated just return
     */
    if (hIMC && data->IMC.hWnd == hWnd)
        return hIMC;

    if (hIMC && IMM_IsCrossThreadAccess(hWnd, hIMC))
        return NULL;

    if (hWnd)
    {
        HIMC defaultContext = get_default_context( hWnd );
        old = RemovePropW(hWnd,szwWineIMCProperty);

        if (old == NULL)
            old = defaultContext;
        else if (old == (HIMC)-1)
            old = NULL;

        if (hIMC != defaultContext)
        {
            if (hIMC == NULL) /* Meaning disable imm for that window*/
                SetPropW(hWnd,szwWineIMCProperty,(HANDLE)-1);
            else
                SetPropW(hWnd,szwWineIMCProperty,hIMC);
        }

        if (old)
        {
            InputContextData *old_data = (InputContextData *)old;
            if (old_data->IMC.hWnd == hWnd)
                old_data->IMC.hWnd = NULL;
        }
    }

    if (!hIMC)
        return old;

    if(GetActiveWindow() == data->IMC.hWnd)
    {
        SendMessageW(data->IMC.hWnd, WM_IME_SETCONTEXT, FALSE, ISC_SHOWUIALL);
        data->IMC.hWnd = hWnd;
        SendMessageW(data->IMC.hWnd, WM_IME_SETCONTEXT, TRUE, ISC_SHOWUIALL);
    }

    return old;
}


/*
 * Helper function for ImmAssociateContextEx
 */
static BOOL CALLBACK _ImmAssociateContextExEnumProc(HWND hwnd, LPARAM lParam)
{
    HIMC hImc = (HIMC)lParam;
    ImmAssociateContext(hwnd,hImc);
    return TRUE;
}

/***********************************************************************
 *              ImmAssociateContextEx (IMM32.@)
 */
BOOL WINAPI ImmAssociateContextEx(HWND hWnd, HIMC hIMC, DWORD dwFlags)
{
    TRACE("(%p, %p, 0x%x):\n", hWnd, hIMC, dwFlags);

    if (!hWnd)
        return FALSE;

    switch (dwFlags)
    {
    case 0:
        ImmAssociateContext(hWnd,hIMC);
        return TRUE;
    case IACE_DEFAULT:
    {
        HIMC defaultContext = get_default_context( hWnd );
        if (!defaultContext) return FALSE;
        ImmAssociateContext(hWnd,defaultContext);
        return TRUE;
    }
    case IACE_IGNORENOCONTEXT:
        if (GetPropW(hWnd,szwWineIMCProperty))
            ImmAssociateContext(hWnd,hIMC);
        return TRUE;
    case IACE_CHILDREN:
        EnumChildWindows(hWnd,_ImmAssociateContextExEnumProc,(LPARAM)hIMC);
        return TRUE;
    default:
        FIXME("Unknown dwFlags 0x%x\n",dwFlags);
        return FALSE;
    }
}

/***********************************************************************
 *		ImmConfigureIMEA (IMM32.@)
 */
BOOL WINAPI ImmConfigureIMEA(
  HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);

    TRACE("(%p, %p, %d, %p):\n", hKL, hWnd, dwMode, lpData);

    if (dwMode == IME_CONFIG_REGISTERWORD && !lpData)
        return FALSE;

    if (immHkl->hIME && immHkl->pImeConfigure)
    {
        if (dwMode != IME_CONFIG_REGISTERWORD || !is_kbd_ime_unicode(immHkl))
            return immHkl->pImeConfigure(hKL,hWnd,dwMode,lpData);
        else
        {
            REGISTERWORDW rww;
            REGISTERWORDA *rwa = lpData;
            BOOL rc;

            rww.lpReading = strdupAtoW(rwa->lpReading);
            rww.lpWord = strdupAtoW(rwa->lpWord);
            rc = immHkl->pImeConfigure(hKL,hWnd,dwMode,&rww);
            HeapFree(GetProcessHeap(),0,rww.lpReading);
            HeapFree(GetProcessHeap(),0,rww.lpWord);
            return rc;
        }
    }
    else
        return FALSE;
}

/***********************************************************************
 *		ImmConfigureIMEW (IMM32.@)
 */
BOOL WINAPI ImmConfigureIMEW(
  HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);

    TRACE("(%p, %p, %d, %p):\n", hKL, hWnd, dwMode, lpData);

    if (dwMode == IME_CONFIG_REGISTERWORD && !lpData)
        return FALSE;

    if (immHkl->hIME && immHkl->pImeConfigure)
    {
        if (dwMode != IME_CONFIG_REGISTERWORD || is_kbd_ime_unicode(immHkl))
            return immHkl->pImeConfigure(hKL,hWnd,dwMode,lpData);
        else
        {
            REGISTERWORDW *rww = lpData;
            REGISTERWORDA rwa;
            BOOL rc;

            rwa.lpReading = strdupWtoA(rww->lpReading);
            rwa.lpWord = strdupWtoA(rww->lpWord);
            rc = immHkl->pImeConfigure(hKL,hWnd,dwMode,&rwa);
            HeapFree(GetProcessHeap(),0,rwa.lpReading);
            HeapFree(GetProcessHeap(),0,rwa.lpWord);
            return rc;
        }
    }
    else
        return FALSE;
}

/***********************************************************************
 *		ImmCreateContext (IMM32.@)
 */
HIMC WINAPI ImmCreateContext(void)
{
    PCLIENTIMC pClientImc;
    HIMC hIMC;

    TRACE("()\n");

    if (g_psi == NULL || !(g_psi->dwSRVIFlags & SRVINFO_IMM32))
        return NULL;

    pClientImc = Imm32HeapAlloc(HEAP_ZERO_MEMORY, sizeof(CLIENTIMC));
    if (pClientImc == NULL)
        return NULL;

    hIMC = NtUserCreateInputContext(pClientImc);
    if (hIMC == NULL)
    {
        HeapFree(g_hImm32Heap, 0, pClientImc);
        return NULL;
    }

    RtlInitializeCriticalSection(&pClientImc->cs);

    // FIXME: NtUserGetThreadState and enum ThreadStateRoutines are broken.
    pClientImc->unknown = NtUserGetThreadState(13);

    return hIMC;
}

static VOID APIENTRY Imm32CleanupContextExtra(LPINPUTCONTEXT pIC)
{
    FIXME("We have to do something do here");
}

static PCLIENTIMC APIENTRY Imm32FindClientImc(HIMC hIMC)
{
    // FIXME
    return NULL;
}

BOOL APIENTRY Imm32CleanupContext(HIMC hIMC, HKL hKL, BOOL bKeep)
{
    PIMEDPI pImeDpi;
    LPINPUTCONTEXT pIC;
    PCLIENTIMC pClientImc;

    if (g_psi == NULL || !(g_psi->dwSRVIFlags & SRVINFO_IMM32) || hIMC == NULL)
        return FALSE;

    FIXME("We have do something to do here\n");
    pClientImc = Imm32FindClientImc(hIMC);
    if (!pClientImc)
        return FALSE;

    if (pClientImc->hImc == NULL)
    {
        pClientImc->dwFlags |= CLIENTIMC_UNKNOWN1;
        ImmUnlockClientImc(pClientImc);
        if (!bKeep)
            return NtUserDestroyInputContext(hIMC);
        return TRUE;
    }

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
    {
        ImmUnlockClientImc(pClientImc);
        return FALSE;
    }

    FIXME("We have do something to do here\n");

    if (pClientImc->hKL == hKL)
    {
        pImeDpi = ImmLockImeDpi(hKL);
        if (pImeDpi != NULL)
        {
            if (IS_IME_HKL(hKL))
            {
                pImeDpi->ImeSelect(hIMC, FALSE);
            }
            else if (g_psi && (g_psi->dwSRVIFlags & SRVINFO_CICERO_ENABLED))
            {
                FIXME("We have do something to do here\n");
            }
            ImmUnlockImeDpi(pImeDpi);
        }
        pClientImc->hKL = NULL;
    }

    ImmDestroyIMCC(pIC->hPrivate);
    ImmDestroyIMCC(pIC->hMsgBuf);
    ImmDestroyIMCC(pIC->hGuideLine);
    ImmDestroyIMCC(pIC->hCandInfo);
    ImmDestroyIMCC(pIC->hCompStr);

    Imm32CleanupContextExtra(pIC);

    ImmUnlockIMC(hIMC);

    pClientImc->dwFlags |= CLIENTIMC_UNKNOWN1;
    ImmUnlockClientImc(pClientImc);

    if (!bKeep)
        return NtUserDestroyInputContext(hIMC);

    return TRUE;
}

/***********************************************************************
 *		ImmDestroyContext (IMM32.@)
 */
BOOL WINAPI ImmDestroyContext(HIMC hIMC)
{
    HKL hKL;

    TRACE("(%p)\n", hIMC);

    if (g_psi == NULL || !(g_psi->dwSRVIFlags & SRVINFO_IMM32))
        return FALSE;

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    hKL = GetKeyboardLayout(0);
    return Imm32CleanupContext(hIMC, hKL, FALSE);
}

/***********************************************************************
 *		ImmDisableIME (IMM32.@)
 */
BOOL WINAPI ImmDisableIME(DWORD dwThreadId)
{
    return NtUserDisableThreadIme(dwThreadId);
}

/*
 * These functions absorb the difference between Ansi and Wide.
 */
typedef struct ENUM_WORD_A2W
{
    REGISTERWORDENUMPROCW lpfnEnumProc;
    LPVOID lpData;
    UINT ret;
} ENUM_WORD_A2W, *LPENUM_WORD_A2W;

typedef struct ENUM_WORD_W2A
{
    REGISTERWORDENUMPROCA lpfnEnumProc;
    LPVOID lpData;
    UINT ret;
} ENUM_WORD_W2A, *LPENUM_WORD_W2A;

static INT CALLBACK
Imm32EnumWordProcA2W(LPCSTR pszReadingA, DWORD dwStyle, LPCSTR pszRegisterA, LPVOID lpData)
{
    INT ret = 0;
    LPENUM_WORD_A2W lpEnumData = lpData;
    LPWSTR pszReadingW = NULL, pszRegisterW = NULL;

    if (pszReadingA)
    {
        pszReadingW = Imm32WideFromAnsi(pszReadingA);
        if (pszReadingW == NULL)
            goto Quit;
    }

    if (pszRegisterA)
    {
        pszRegisterW = Imm32WideFromAnsi(pszRegisterA);
        if (pszRegisterW == NULL)
            goto Quit;
    }

    ret = lpEnumData->lpfnEnumProc(pszReadingW, dwStyle, pszRegisterW, lpEnumData->lpData);
    lpEnumData->ret = ret;

Quit:
    if (pszReadingW)
        HeapFree(g_hImm32Heap, 0, pszReadingW);
    if (pszRegisterW)
        HeapFree(g_hImm32Heap, 0, pszRegisterW);
    return ret;
}

static INT CALLBACK
Imm32EnumWordProcW2A(LPCWSTR pszReadingW, DWORD dwStyle, LPCWSTR pszRegisterW, LPVOID lpData)
{
    INT ret = 0;
    LPENUM_WORD_W2A lpEnumData = lpData;
    LPSTR pszReadingA = NULL, pszRegisterA = NULL;

    if (pszReadingW)
    {
        pszReadingA = Imm32AnsiFromWide(pszReadingW);
        if (pszReadingW == NULL)
            goto Quit;
    }

    if (pszRegisterW)
    {
        pszRegisterA = Imm32AnsiFromWide(pszRegisterW);
        if (pszRegisterA == NULL)
            goto Quit;
    }

    ret = lpEnumData->lpfnEnumProc(pszReadingA, dwStyle, pszRegisterA, lpEnumData->lpData);
    lpEnumData->ret = ret;

Quit:
    if (pszReadingA)
        HeapFree(g_hImm32Heap, 0, pszReadingA);
    if (pszRegisterA)
        HeapFree(g_hImm32Heap, 0, pszRegisterA);
    return ret;
}

/***********************************************************************
 *		ImmEnumRegisterWordA (IMM32.@)
 */
UINT WINAPI ImmEnumRegisterWordA(
  HKL hKL, REGISTERWORDENUMPROCA lpfnEnumProc,
  LPCSTR lpszReading, DWORD dwStyle,
  LPCSTR lpszRegister, LPVOID lpData)
{
    UINT ret = 0;
    LPWSTR pszReadingW = NULL, pszRegisterW = NULL;
    ENUM_WORD_W2A EnumDataW2A;
    PIMEDPI pImeDpi;

    TRACE("(%p, %p, %s, 0x%lX, %s, %p)", hKL, lpfnEnumProc, debugstr_a(lpszReading),
          dwStyle, debugstr_a(lpszRegister), lpData);

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return 0;

    if (!(pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE))
    {
        ret = pImeDpi->ImeEnumRegisterWord(lpfnEnumProc, lpszReading, dwStyle,
                                           lpszRegister, lpData);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (lpszReading)
    {
        pszReadingW = Imm32WideFromAnsi(lpszReading);
        if (pszReadingW == NULL)
            goto Quit;
    }

    if (lpszRegister)
    {
        pszRegisterW = Imm32WideFromAnsi(lpszRegister);
        if (pszRegisterW == NULL)
            goto Quit;
    }

    EnumDataW2A.lpfnEnumProc = lpfnEnumProc;
    EnumDataW2A.lpData = lpData;
    EnumDataW2A.ret = 0;
    pImeDpi->ImeEnumRegisterWord(Imm32EnumWordProcW2A, pszReadingW, dwStyle,
                                 pszRegisterW, &EnumDataW2A);
    ret = EnumDataW2A.ret;

Quit:
    if (pszReadingW)
        HeapFree(g_hImm32Heap, 0, pszReadingW);
    if (pszRegisterW)
        HeapFree(g_hImm32Heap, 0, pszRegisterW);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmEnumRegisterWordW (IMM32.@)
 */
UINT WINAPI ImmEnumRegisterWordW(
  HKL hKL, REGISTERWORDENUMPROCW lpfnEnumProc,
  LPCWSTR lpszReading, DWORD dwStyle,
  LPCWSTR lpszRegister, LPVOID lpData)
{
    UINT ret = 0;
    LPSTR pszReadingA = NULL, pszRegisterA = NULL;
    ENUM_WORD_A2W EnumDataA2W;
    PIMEDPI pImeDpi;

    TRACE("(%p, %p, %s, 0x%lX, %s, %p)", hKL, lpfnEnumProc, debugstr_w(lpszReading),
          dwStyle, debugstr_w(lpszRegister), lpData);

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return 0;

    if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE)
    {
        ret = pImeDpi->ImeEnumRegisterWord(lpfnEnumProc, lpszReading, dwStyle,
                                           lpszRegister, lpData);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (lpszReading)
    {
        pszReadingA = Imm32AnsiFromWide(lpszReading);
        if (pszReadingA == NULL)
            goto Quit;
    }

    if (lpszRegister)
    {
        pszRegisterA = Imm32AnsiFromWide(lpszRegister);
        if (pszRegisterA == NULL)
            goto Quit;
    }

    EnumDataA2W.lpfnEnumProc = lpfnEnumProc;
    EnumDataA2W.lpData = lpData;
    EnumDataA2W.ret = 0;
    pImeDpi->ImeEnumRegisterWord(Imm32EnumWordProcA2W, pszReadingA, dwStyle,
                                 pszRegisterA, &EnumDataA2W);
    ret = EnumDataA2W.ret;

Quit:
    if (pszReadingA)
        HeapFree(g_hImm32Heap, 0, pszReadingA);
    if (pszRegisterA)
        HeapFree(g_hImm32Heap, 0, pszRegisterA);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

static inline BOOL EscapeRequiresWA(UINT uEscape)
{
        if (uEscape == IME_ESC_GET_EUDC_DICTIONARY ||
            uEscape == IME_ESC_SET_EUDC_DICTIONARY ||
            uEscape == IME_ESC_IME_NAME ||
            uEscape == IME_ESC_GETHELPFILENAME)
        return TRUE;
    return FALSE;
}

/***********************************************************************
 *		ImmEscapeA (IMM32.@)
 */
LRESULT WINAPI ImmEscapeA(
  HKL hKL, HIMC hIMC,
  UINT uEscape, LPVOID lpData)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %p, %d, %p):\n", hKL, hIMC, uEscape, lpData);

    if (immHkl->hIME && immHkl->pImeEscape)
    {
        if (!EscapeRequiresWA(uEscape) || !is_kbd_ime_unicode(immHkl))
            return immHkl->pImeEscape(hIMC,uEscape,lpData);
        else
        {
            WCHAR buffer[81]; /* largest required buffer should be 80 */
            LRESULT rc;
            if (uEscape == IME_ESC_SET_EUDC_DICTIONARY)
            {
                MultiByteToWideChar(CP_ACP,0,lpData,-1,buffer,81);
                rc = immHkl->pImeEscape(hIMC,uEscape,buffer);
            }
            else
            {
                rc = immHkl->pImeEscape(hIMC,uEscape,buffer);
                WideCharToMultiByte(CP_ACP,0,buffer,-1,lpData,80, NULL, NULL);
            }
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
 *		ImmEscapeW (IMM32.@)
 */
LRESULT WINAPI ImmEscapeW(
  HKL hKL, HIMC hIMC,
  UINT uEscape, LPVOID lpData)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %p, %d, %p):\n", hKL, hIMC, uEscape, lpData);

    if (immHkl->hIME && immHkl->pImeEscape)
    {
        if (!EscapeRequiresWA(uEscape) || is_kbd_ime_unicode(immHkl))
            return immHkl->pImeEscape(hIMC,uEscape,lpData);
        else
        {
            CHAR buffer[81]; /* largest required buffer should be 80 */
            LRESULT rc;
            if (uEscape == IME_ESC_SET_EUDC_DICTIONARY)
            {
                WideCharToMultiByte(CP_ACP,0,lpData,-1,buffer,81, NULL, NULL);
                rc = immHkl->pImeEscape(hIMC,uEscape,buffer);
            }
            else
            {
                rc = immHkl->pImeEscape(hIMC,uEscape,buffer);
                MultiByteToWideChar(CP_ACP,0,buffer,-1,lpData,80);
            }
            return rc;
        }
    }
    else
        return 0;
}

static PCLIENTIMC APIENTRY Imm32GetClientImcCache(void)
{
    // FIXME: Do something properly here
    return NULL;
}

static DWORD APIENTRY Imm32AllocAndBuildHimcList(DWORD dwThreadId, HIMC **pphList)
{
#define INITIAL_COUNT 0x40
#define MAX_RETRY 10
    NTSTATUS Status;
    DWORD dwCount = INITIAL_COUNT, cRetry = 0;
    HIMC *phNewList;

    phNewList = Imm32HeapAlloc(0, dwCount * sizeof(HIMC));
    if (phNewList == NULL)
        return 0;

    Status = NtUserBuildHimcList(dwThreadId, dwCount, phNewList, &dwCount);
    while (Status == STATUS_BUFFER_TOO_SMALL)
    {
        HeapFree(g_hImm32Heap, 0, phNewList);
        if (cRetry++ >= MAX_RETRY)
            return 0;

        phNewList = Imm32HeapAlloc(0, dwCount * sizeof(HIMC));
        if (phNewList == NULL)
            return 0;

        Status = NtUserBuildHimcList(dwThreadId, dwCount, phNewList, &dwCount);
    }

    if (NT_ERROR(Status) || !dwCount)
    {
        HeapFree(g_hImm32Heap, 0, phNewList);
        return 0;
    }

    *pphList = phNewList;
    return dwCount;
#undef INITIAL_COUNT
#undef MAX_RETRY
}

static BOOL APIENTRY Imm32ImeNonImeToggle(HIMC hIMC, HKL hKL, HWND hWnd, LANGID LangID)
{
    LPINPUTCONTEXT pIC;
    BOOL fOpen;

    if (hWnd != NULL)
        return FALSE;

    if (!IS_IME_HKL(hKL) || LOWORD(hKL) != LangID)
    {
        FIXME("We have to do something here\n");
        return TRUE;
    }

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return TRUE;

    fOpen = pIC->fOpen;
    ImmUnlockIMC(hIMC);

    if (!fOpen)
    {
        ImmSetOpenStatus(hIMC, TRUE);
        return TRUE;
    }

    FIXME("We have to do something here\n");
    return TRUE;
}

static BOOL APIENTRY Imm32CShapeToggle(HIMC hIMC, HKL hKL, HWND hWnd)
{
    LPINPUTCONTEXT pIC;
    BOOL fOpen;
    DWORD dwConversion, dwSentence;

    if (hWnd == NULL || !IS_IME_HKL(hKL))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return TRUE;

    fOpen = pIC->fOpen;
    if (fOpen)
    {
        dwConversion = (pIC->fdwConversion ^ IME_CMODE_FULLSHAPE);
        dwSentence = pIC->fdwSentence;
    }

    ImmUnlockIMC(hIMC);

    if (fOpen)
        ImmSetConversionStatus(hIMC, dwConversion, dwSentence);
    else
        ImmSetOpenStatus(hIMC, TRUE);

    return TRUE;
}

static BOOL APIENTRY Imm32CSymbolToggle(HIMC hIMC, HKL hKL, HWND hWnd)
{
    LPINPUTCONTEXT pIC;
    BOOL fOpen;
    DWORD dwConversion, dwSentence;

    if (hWnd == NULL || !IS_IME_HKL(hKL))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return TRUE;

    fOpen = pIC->fOpen;
    if (fOpen)
    {
        dwConversion = (pIC->fdwConversion ^ IME_CMODE_SYMBOL);
        dwSentence = pIC->fdwSentence;
    }

    ImmUnlockIMC(hIMC);

    if (fOpen)
        ImmSetConversionStatus(hIMC, dwConversion, dwSentence);
    else
        ImmSetOpenStatus(hIMC, TRUE);

    return TRUE;
}

static BOOL APIENTRY Imm32JCloseOpen(HIMC hIMC, HKL hKL, HWND hWnd)
{
    BOOL fOpen;

    if (ImmIsIME(hKL) && LOWORD(hKL) == LANGID_JAPANESE)
    {
        fOpen = ImmGetOpenStatus(hIMC);
        ImmSetOpenStatus(hIMC, !fOpen);
        return TRUE;
    }

    FIXME("We have to do something here\n");
    return TRUE;
}

static BOOL APIENTRY Imm32KShapeToggle(HIMC hIMC)
{
    LPINPUTCONTEXT pIC;
    DWORD dwConversion, dwSentence;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    dwConversion = (pIC->fdwConversion ^ IME_CMODE_FULLSHAPE);
    dwSentence = pIC->fdwSentence;
    ImmSetConversionStatus(hIMC, dwConversion, dwSentence);

    if (pIC->fdwConversion & (IME_CMODE_FULLSHAPE | IME_CMODE_NATIVE))
        ImmSetOpenStatus(hIMC, TRUE);
    else
        ImmSetOpenStatus(hIMC, FALSE);

    ImmUnlockIMC(hIMC);
    return TRUE;
}

static BOOL APIENTRY Imm32KHanjaConvert(HIMC hIMC)
{
    LPINPUTCONTEXT pIC;
    DWORD dwConversion, dwSentence;

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return FALSE;

    dwConversion = (pIC->fdwConversion ^ IME_CMODE_HANJACONVERT);
    dwSentence = pIC->fdwSentence;
    ImmUnlockIMC(hIMC);

    ImmSetConversionStatus(hIMC, dwConversion, dwSentence);
    return TRUE;
}

static BOOL APIENTRY Imm32KEnglish(HIMC hIMC)
{
    LPINPUTCONTEXT pIC;
    DWORD dwConversion, dwSentence;
    BOOL fOpen;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    dwConversion = (pIC->fdwConversion ^ IME_CMODE_NATIVE);
    dwSentence = pIC->fdwSentence;
    ImmSetConversionStatus(hIMC, dwConversion, dwSentence);

    fOpen = ((pIC->fdwConversion & (IME_CMODE_FULLSHAPE | IME_CMODE_NATIVE)) != 0);
    ImmSetOpenStatus(hIMC, fOpen);

    ImmUnlockIMC(hIMC);
    return TRUE;
}

static BOOL APIENTRY Imm32ProcessHotKey(HWND hWnd, HIMC hIMC, HKL hKL, DWORD dwHotKeyID)
{
    PIMEDPI pImeDpi;
    BOOL ret;

    if (hIMC && Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    switch (dwHotKeyID)
    {
        case IME_CHOTKEY_IME_NONIME_TOGGLE:
            return Imm32ImeNonImeToggle(hIMC, hKL, hWnd, LANGID_CHINESE_SIMPLIFIED);

        case IME_CHOTKEY_SHAPE_TOGGLE:
            return Imm32CShapeToggle(hIMC, hKL, hWnd);

        case IME_CHOTKEY_SYMBOL_TOGGLE:
            return Imm32CSymbolToggle(hIMC, hKL, hWnd);

        case IME_JHOTKEY_CLOSE_OPEN:
            return Imm32JCloseOpen(hIMC, hKL, hWnd);

        case IME_KHOTKEY_SHAPE_TOGGLE:
            return Imm32KShapeToggle(hIMC);

        case IME_KHOTKEY_HANJACONVERT:
            return Imm32KHanjaConvert(hIMC);

        case IME_KHOTKEY_ENGLISH:
            return Imm32KEnglish(hIMC);

        case IME_THOTKEY_IME_NONIME_TOGGLE:
            return Imm32ImeNonImeToggle(hIMC, hKL, hWnd, LANGID_CHINESE_TRADITIONAL);

        case IME_THOTKEY_SHAPE_TOGGLE:
            return Imm32CShapeToggle(hIMC, hKL, hWnd);

        case IME_THOTKEY_SYMBOL_TOGGLE:
            return Imm32CSymbolToggle(hIMC, hKL, hWnd);

        default:
            break;
    }

    if (dwHotKeyID < IME_HOTKEY_PRIVATE_FIRST || IME_HOTKEY_PRIVATE_LAST < dwHotKeyID)
        return FALSE;

    pImeDpi = ImmLockImeDpi(hKL);
    if (pImeDpi == NULL)
        return FALSE;

    ret = (BOOL)pImeDpi->ImeEscape(hIMC, IME_ESC_PRIVATE_HOTKEY, &dwHotKeyID);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmLockClientImc (IMM32.@)
 */
PCLIENTIMC WINAPI ImmLockClientImc(HIMC hImc)
{
    PCLIENTIMC pClientImc;

    TRACE("(%p)\n", hImc);

    if (hImc == NULL)
        return NULL;

    pClientImc = Imm32GetClientImcCache();
    if (!pClientImc)
    {
        pClientImc = Imm32HeapAlloc(HEAP_ZERO_MEMORY, sizeof(CLIENTIMC));
        if (!pClientImc)
            return NULL;

        RtlInitializeCriticalSection(&pClientImc->cs);

        // FIXME: NtUserGetThreadState and enum ThreadStateRoutines are broken.
        pClientImc->unknown = NtUserGetThreadState(13);

        if (!NtUserUpdateInputContext(hImc, 0, pClientImc))
        {
            HeapFree(g_hImm32Heap, 0, pClientImc);
            return NULL;
        }

        pClientImc->dwFlags |= CLIENTIMC_UNKNOWN2;
    }
    else
    {
        if (pClientImc->dwFlags & CLIENTIMC_UNKNOWN1)
            return NULL;
    }

    InterlockedIncrement(&pClientImc->cLockObj);
    return pClientImc;
}

/***********************************************************************
 *		ImmUnlockClientImc (IMM32.@)
 */
VOID WINAPI ImmUnlockClientImc(PCLIENTIMC pClientImc)
{
    LONG cLocks;
    HIMC hImc;

    TRACE("(%p)\n", pClientImc);

    cLocks = InterlockedDecrement(&pClientImc->cLockObj);
    if (cLocks != 0 || !(pClientImc->dwFlags & CLIENTIMC_UNKNOWN1))
        return;

    hImc = pClientImc->hImc;
    if (hImc)
        LocalFree(hImc);

    RtlDeleteCriticalSection(&pClientImc->cs);
    HeapFree(g_hImm32Heap, 0, pClientImc);
}

static DWORD APIENTRY
CandidateListWideToAnsi(const CANDIDATELIST *pWideCL, LPCANDIDATELIST pAnsiCL, DWORD dwBufLen,
                        UINT uCodePage)
{
    BOOL bUsedDefault;
    DWORD dwSize, dwIndex, cbGot, cbLeft;
    const BYTE *pbWide;
    LPBYTE pbAnsi;
    LPDWORD pibOffsets;

    /* calculate total ansi size */
    if (pWideCL->dwCount > 0)
    {
        dwSize = sizeof(CANDIDATELIST) + ((pWideCL->dwCount - 1) * sizeof(DWORD));
        for (dwIndex = 0; dwIndex < pWideCL->dwCount; ++dwIndex)
        {
            pbWide = (const BYTE *)pWideCL + pWideCL->dwOffset[dwIndex];
            cbGot = WideCharToMultiByte(uCodePage, 0, (LPCWSTR)pbWide, -1, NULL, 0,
                                        NULL, &bUsedDefault);
            dwSize += cbGot;
        }
    }
    else
    {
        dwSize = sizeof(CANDIDATELIST);
    }

    dwSize = ROUNDUP4(dwSize);
    if (dwBufLen == 0)
        return dwSize;
    if (dwBufLen < dwSize)
        return 0;

    /* store to ansi */
    pAnsiCL->dwSize = dwBufLen;
    pAnsiCL->dwStyle = pWideCL->dwStyle;
    pAnsiCL->dwCount = pWideCL->dwCount;
    pAnsiCL->dwSelection = pWideCL->dwSelection;
    pAnsiCL->dwPageStart = pWideCL->dwPageStart;
    pAnsiCL->dwPageSize = pWideCL->dwPageSize;

    pibOffsets = pAnsiCL->dwOffset;
    if (pWideCL->dwCount > 0)
    {
        pibOffsets[0] = sizeof(CANDIDATELIST) + ((pWideCL->dwCount - 1) * sizeof(DWORD));
        cbLeft = dwBufLen - pibOffsets[0];

        for (dwIndex = 0; dwIndex < pWideCL->dwCount; ++dwIndex)
        {
            pbWide = (const BYTE *)pWideCL + pWideCL->dwOffset[dwIndex];
            pbAnsi = (LPBYTE)pAnsiCL + pibOffsets[dwIndex];

            /* convert to ansi */
            cbGot = WideCharToMultiByte(uCodePage, 0, (LPCWSTR)pbWide, -1,
                                        (LPSTR)pbAnsi, cbLeft, NULL, &bUsedDefault);
            cbLeft -= cbGot;

            if (dwIndex < pWideCL->dwCount - 1)
                pibOffsets[dwIndex + 1] = pibOffsets[dwIndex] + cbGot;
        }
    }
    else
    {
        pibOffsets[0] = sizeof(CANDIDATELIST);
    }

    return dwBufLen;
}

static DWORD APIENTRY
CandidateListAnsiToWide(const CANDIDATELIST *pAnsiCL, LPCANDIDATELIST pWideCL, DWORD dwBufLen,
                        UINT uCodePage)
{
    DWORD dwSize, dwIndex, cchGot, cbGot, cbLeft;
    const BYTE *pbAnsi;
    LPBYTE pbWide;
    LPDWORD pibOffsets;

    /* calculate total wide size */
    if (pAnsiCL->dwCount > 0)
    {
        dwSize = sizeof(CANDIDATELIST) + ((pAnsiCL->dwCount - 1) * sizeof(DWORD));
        for (dwIndex = 0; dwIndex < pAnsiCL->dwCount; ++dwIndex)
        {
            pbAnsi = (const BYTE *)pAnsiCL + pAnsiCL->dwOffset[dwIndex];
            cchGot = MultiByteToWideChar(uCodePage, MB_PRECOMPOSED, (LPCSTR)pbAnsi, -1, NULL, 0);
            dwSize += cchGot * sizeof(WCHAR);
        }
    }
    else
    {
        dwSize = sizeof(CANDIDATELIST);
    }

    dwSize = ROUNDUP4(dwSize);
    if (dwBufLen == 0)
        return dwSize;
    if (dwBufLen < dwSize)
        return 0;

    /* store to wide */
    pWideCL->dwSize = dwBufLen;
    pWideCL->dwStyle = pAnsiCL->dwStyle;
    pWideCL->dwCount = pAnsiCL->dwCount;
    pWideCL->dwSelection = pAnsiCL->dwSelection;
    pWideCL->dwPageStart = pAnsiCL->dwPageStart;
    pWideCL->dwPageSize = pAnsiCL->dwPageSize;

    pibOffsets = pWideCL->dwOffset;
    if (pAnsiCL->dwCount > 0)
    {
        pibOffsets[0] = sizeof(CANDIDATELIST) + ((pWideCL->dwCount - 1) * sizeof(DWORD));
        cbLeft = dwBufLen - pibOffsets[0];

        for (dwIndex = 0; dwIndex < pAnsiCL->dwCount; ++dwIndex)
        {
            pbAnsi = (const BYTE *)pAnsiCL + pAnsiCL->dwOffset[dwIndex];
            pbWide = (LPBYTE)pWideCL + pibOffsets[dwIndex];

            /* convert to wide */
            cchGot = MultiByteToWideChar(uCodePage, MB_PRECOMPOSED, (LPCSTR)pbAnsi, -1,
                                         (LPWSTR)pbWide, cbLeft / sizeof(WCHAR));
            cbGot = cchGot * sizeof(WCHAR);
            cbLeft -= cbGot;

            if (dwIndex + 1 < pAnsiCL->dwCount)
                pibOffsets[dwIndex + 1] = pibOffsets[dwIndex] + cbGot;
        }
    }
    else
    {
        pibOffsets[0] = sizeof(CANDIDATELIST);
    }

    return dwBufLen;
}

static DWORD APIENTRY
ImmGetCandidateListAW(HIMC hIMC, DWORD dwIndex, LPCANDIDATELIST lpCandList, DWORD dwBufLen,
                      BOOL bAnsi)
{
    DWORD ret = 0;
    LPINPUTCONTEXT pIC;
    PCLIENTIMC pClientImc;
    LPCANDIDATEINFO pCI;
    LPCANDIDATELIST pCL;
    DWORD dwSize;

    pClientImc = ImmLockClientImc(hIMC);
    if (!pClientImc)
        return 0;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
    {
        ImmUnlockClientImc(pClientImc);
        return 0;
    }

    pCI = ImmLockIMCC(pIC->hCandInfo);
    if (pCI == NULL)
    {
        ImmUnlockIMC(hIMC);
        ImmUnlockClientImc(pClientImc);
        return 0;
    }

    if (pCI->dwSize < sizeof(CANDIDATEINFO) || pCI->dwCount <= dwIndex)
        goto Quit;

    /* get required size */
    pCL = (LPCANDIDATELIST)((LPBYTE)pCI + pCI->dwOffset[dwIndex]);
    if (bAnsi)
    {
        if (pClientImc->dwFlags & CLIENTIMC_WIDE)
            dwSize = CandidateListAnsiToWide(pCL, NULL, 0, CP_ACP);
        else
            dwSize = pCL->dwSize;
    }
    else
    {
        if (pClientImc->dwFlags & CLIENTIMC_WIDE)
            dwSize = pCL->dwSize;
        else
            dwSize = CandidateListWideToAnsi(pCL, NULL, 0, CP_ACP);
    }

    if (dwBufLen != 0 && dwSize != 0)
    {
        if (lpCandList == NULL || dwBufLen < dwSize)
            goto Quit;

        /* store */
        if (bAnsi)
        {
            if (pClientImc->dwFlags & CLIENTIMC_WIDE)
                CandidateListAnsiToWide(pCL, lpCandList, dwSize, CP_ACP);
            else
                RtlCopyMemory(lpCandList, pCL, dwSize);
        }
        else
        {
            if (pClientImc->dwFlags & CLIENTIMC_WIDE)
                RtlCopyMemory(lpCandList, pCL, dwSize);
            else
                CandidateListWideToAnsi(pCL, lpCandList, dwSize, CP_ACP);
        }
    }

    ret = dwSize;

Quit:
    ImmUnlockIMCC(pIC->hCandInfo);
    ImmUnlockIMC(hIMC);
    ImmUnlockClientImc(pClientImc);
    return ret;
}

DWORD APIENTRY ImmGetCandidateListCountAW(HIMC hIMC, LPDWORD lpdwListCount, BOOL bAnsi)
{
    DWORD ret = 0, cbGot, dwIndex;
    PCLIENTIMC pClientImc;
    LPINPUTCONTEXT pIC;
    const CANDIDATEINFO *pCI;
    const BYTE *pb;
    const CANDIDATELIST *pCL;
    const DWORD *pdwOffsets;

    if (lpdwListCount == NULL)
        return 0;

    *lpdwListCount = 0;

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return 0;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
    {
        ImmUnlockClientImc(pClientImc);
        return 0;
    }

    pCI = ImmLockIMCC(pIC->hCandInfo);
    if (pCI == NULL)
    {
        ImmUnlockIMC(hIMC);
        ImmUnlockClientImc(pClientImc);
        return 0;
    }

    if (pCI->dwSize < sizeof(CANDIDATEINFO))
        goto Quit;

    *lpdwListCount = pCI->dwCount; /* the number of candidate lists */

    /* calculate total size of candidate lists */
    if (bAnsi)
    {
        if (pClientImc->dwFlags & CLIENTIMC_WIDE)
        {
            ret = ROUNDUP4(pCI->dwPrivateSize);
            pdwOffsets = pCI->dwOffset;
            for (dwIndex = 0; dwIndex < pCI->dwCount; ++dwIndex)
            {
                pb = (const BYTE *)pCI + pdwOffsets[dwIndex];
                pCL = (const CANDIDATELIST *)pb;
                cbGot = CandidateListWideToAnsi(pCL, NULL, 0, CP_ACP);
                ret += cbGot;
            }
        }
        else
        {
            ret = pCI->dwSize;
        }
    }
    else
    {
        if (pClientImc->dwFlags & CLIENTIMC_WIDE)
        {
            ret = pCI->dwSize;
        }
        else
        {
            ret = ROUNDUP4(pCI->dwPrivateSize);
            pdwOffsets = pCI->dwOffset;
            for (dwIndex = 0; dwIndex < pCI->dwCount; ++dwIndex)
            {
                pb = (const BYTE *)pCI + pdwOffsets[dwIndex];
                pCL = (const CANDIDATELIST *)pb;
                cbGot = CandidateListAnsiToWide(pCL, NULL, 0, CP_ACP);
                ret += cbGot;
            }
        }
    }

Quit:
    ImmUnlockIMCC(pIC->hCandInfo);
    ImmUnlockIMC(hIMC);
    ImmUnlockClientImc(pClientImc);
    return ret;
}

/***********************************************************************
 *		ImmGetCandidateListA (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListA(
  HIMC hIMC, DWORD dwIndex,
  LPCANDIDATELIST lpCandList, DWORD dwBufLen)
{
    return ImmGetCandidateListAW(hIMC, dwIndex, lpCandList, dwBufLen, TRUE);
}

/***********************************************************************
 *		ImmGetCandidateListCountA (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListCountA(
  HIMC hIMC, LPDWORD lpdwListCount)
{
    return ImmGetCandidateListCountAW(hIMC, lpdwListCount, TRUE);
}

/***********************************************************************
 *		ImmGetCandidateListCountW (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListCountW(
  HIMC hIMC, LPDWORD lpdwListCount)
{
    return ImmGetCandidateListCountAW(hIMC, lpdwListCount, FALSE);
}

/***********************************************************************
 *		ImmGetCandidateListW (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListW(
  HIMC hIMC, DWORD dwIndex,
  LPCANDIDATELIST lpCandList, DWORD dwBufLen)
{
    return ImmGetCandidateListAW(hIMC, dwIndex, lpCandList, dwBufLen, FALSE);
}

/***********************************************************************
 *		ImmGetCandidateWindow (IMM32.@)
 */
BOOL WINAPI ImmGetCandidateWindow(
  HIMC hIMC, DWORD dwIndex, LPCANDIDATEFORM lpCandidate)
{
    BOOL ret = FALSE;
    LPINPUTCONTEXT pIC;
    LPCANDIDATEFORM pCF;

    TRACE("(%p, %lu, %p)\n", hIMC, dwIndex, lpCandidate);

    pIC = ImmLockIMC(hIMC);
    if (pIC  == NULL)
        return FALSE;

    pCF = &pIC->cfCandForm[dwIndex];
    if (pCF->dwIndex != IMM_INVALID_CANDFORM)
    {
        *lpCandidate = *pCF;
        ret = TRUE;
    }

    ImmUnlockIMC(hIMC);
    return ret;
}

static VOID APIENTRY LogFontAnsiToWide(const LOGFONTA *plfA, LPLOGFONTW plfW)
{
    size_t cch;
    RtlCopyMemory(plfW, plfA, offsetof(LOGFONTA, lfFaceName));
    StringCchLengthA(plfA->lfFaceName, _countof(plfA->lfFaceName), &cch);
    cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, plfA->lfFaceName, (INT)cch,
                              plfW->lfFaceName, _countof(plfW->lfFaceName));
    if (cch > _countof(plfW->lfFaceName) - 1)
        cch = _countof(plfW->lfFaceName) - 1;
    plfW->lfFaceName[cch] = 0;
}

static VOID APIENTRY LogFontWideToAnsi(const LOGFONTW *plfW, LPLOGFONTA plfA)
{
    size_t cch;
    RtlCopyMemory(plfA, plfW, offsetof(LOGFONTW, lfFaceName));
    StringCchLengthW(plfW->lfFaceName, _countof(plfW->lfFaceName), &cch);
    cch = WideCharToMultiByte(CP_ACP, 0, plfW->lfFaceName, (INT)cch,
                              plfA->lfFaceName, _countof(plfA->lfFaceName), NULL, NULL);
    if (cch > _countof(plfA->lfFaceName) - 1)
        cch = _countof(plfA->lfFaceName) - 1;
    plfA->lfFaceName[cch] = 0;
}

/***********************************************************************
 *		ImmGetCompositionFontA (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionFontA(HIMC hIMC, LPLOGFONTA lplf)
{
    PCLIENTIMC pClientImc;
    BOOL ret = FALSE, bWide;
    LPINPUTCONTEXT pIC;

    TRACE("(%p, %p)\n", hIMC, lplf);

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return FALSE;

    bWide = (pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    if (pIC->fdwInit & INIT_LOGFONT)
    {
        if (bWide)
            LogFontWideToAnsi(&pIC->lfFont.W, lplf);
        else
            *lplf = pIC->lfFont.A;

        ret = TRUE;
    }

    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmGetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
    PCLIENTIMC pClientImc;
    BOOL bWide;
    LPINPUTCONTEXT pIC;
    BOOL ret = FALSE;

    TRACE("(%p, %p)\n", hIMC, lplf);

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return FALSE;

    bWide = (pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    if (pIC->fdwInit & INIT_LOGFONT)
    {
        if (bWide)
            *lplf = pIC->lfFont.W;
        else
            LogFontAnsiToWide(&pIC->lfFont.A, lplf);

        ret = TRUE;
    }

    ImmUnlockIMC(hIMC);
    return ret;
}


/* Helpers for the GetCompositionString functions */

/* Source encoding is defined by context, source length is always given in respective characters. Destination buffer
   length is always in bytes. */
static INT CopyCompStringIMEtoClient(const InputContextData *data, const void *src, INT src_len, void *dst,
        INT dst_len, BOOL unicode)
{
    int char_size = unicode ? sizeof(WCHAR) : sizeof(char);
    INT ret;

    if (is_himc_ime_unicode(data) ^ unicode)
    {
        if (unicode)
            ret = MultiByteToWideChar(CP_ACP, 0, src, src_len, dst, dst_len / sizeof(WCHAR));
        else
            ret = WideCharToMultiByte(CP_ACP, 0, src, src_len, dst, dst_len, NULL, NULL);
        ret *= char_size;
    }
    else
    {
        if (dst_len)
        {
            ret = min(src_len * char_size, dst_len);
            memcpy(dst, src, ret);
        }
        else
            ret = src_len * char_size;
    }

    return ret;
}

/* Composition string encoding is defined by context, returned attributes correspond to string, converted according to
   passed mode. String length is in characters, attributes are in byte arrays. */
static INT CopyCompAttrIMEtoClient(const InputContextData *data, const BYTE *src, INT src_len, const void *comp_string,
        INT str_len, BYTE *dst, INT dst_len, BOOL unicode)
{
    union
    {
        const void *str;
        const WCHAR *strW;
        const char *strA;
    } string;
    INT rc;

    string.str = comp_string;

    if (is_himc_ime_unicode(data) && !unicode)
    {
        rc = WideCharToMultiByte(CP_ACP, 0, string.strW, str_len, NULL, 0, NULL, NULL);
        if (dst_len)
        {
            int i, j = 0, k = 0;

            if (rc < dst_len)
                dst_len = rc;
            for (i = 0; i < str_len; ++i)
            {
                int len;

                len = WideCharToMultiByte(CP_ACP, 0, string.strW + i, 1, NULL, 0, NULL, NULL);
                for (; len > 0; --len)
                {
                    dst[j++] = src[k];

                    if (j >= dst_len)
                        goto end;
                }
                ++k;
            }
        end:
            rc = j;
        }
    }
    else if (!is_himc_ime_unicode(data) && unicode)
    {
        rc = MultiByteToWideChar(CP_ACP, 0, string.strA, str_len, NULL, 0);
        if (dst_len)
        {
            int i, j = 0;

            if (rc < dst_len)
                dst_len = rc;
            for (i = 0; i < str_len; ++i)
            {
                if (IsDBCSLeadByte(string.strA[i]))
                    continue;

                dst[j++] = src[i];

                if (j >= dst_len)
                    break;
            }
            rc = j;
        }
    }
    else
    {
        memcpy(dst, src, min(src_len, dst_len));
        rc = src_len;
    }

    return rc;
}

static INT CopyCompClauseIMEtoClient(InputContextData *data, LPBYTE source, INT slen, LPBYTE ssource,
                                     LPBYTE target, INT tlen, BOOL unicode )
{
    INT rc;

    if (is_himc_ime_unicode(data) && !unicode)
    {
        if (tlen)
        {
            int i;

            if (slen < tlen)
                tlen = slen;
            tlen /= sizeof (DWORD);
            for (i = 0; i < tlen; ++i)
            {
                ((DWORD *)target)[i] = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)ssource,
                                                          ((DWORD *)source)[i],
                                                          NULL, 0,
                                                          NULL, NULL);
            }
            rc = sizeof (DWORD) * i;
        }
        else
            rc = slen;
    }
    else if (!is_himc_ime_unicode(data) && unicode)
    {
        if (tlen)
        {
            int i;

            if (slen < tlen)
                tlen = slen;
            tlen /= sizeof (DWORD);
            for (i = 0; i < tlen; ++i)
            {
                ((DWORD *)target)[i] = MultiByteToWideChar(CP_ACP, 0, (LPSTR)ssource,
                                                          ((DWORD *)source)[i],
                                                          NULL, 0);
            }
            rc = sizeof (DWORD) * i;
        }
        else
            rc = slen;
    }
    else
    {
        memcpy( target, source, min(slen,tlen));
        rc = slen;
    }

    return rc;
}

static INT CopyCompOffsetIMEtoClient(InputContextData *data, DWORD offset, LPBYTE ssource, BOOL unicode)
{
    int rc;

    if (is_himc_ime_unicode(data) && !unicode)
    {
        rc = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)ssource, offset, NULL, 0, NULL, NULL);
    }
    else if (!is_himc_ime_unicode(data) && unicode)
    {
        rc = MultiByteToWideChar(CP_ACP, 0, (LPSTR)ssource, offset, NULL, 0);
    }
    else
        rc = offset;

    return rc;
}

static LONG ImmGetCompositionStringT( HIMC hIMC, DWORD dwIndex, LPVOID lpBuf,
                                      DWORD dwBufLen, BOOL unicode)
{
    LONG rc = 0;
    InputContextData *data = get_imc_data(hIMC);
    LPCOMPOSITIONSTRING compstr;
    LPBYTE compdata;

    TRACE("(%p, 0x%x, %p, %d)\n", hIMC, dwIndex, lpBuf, dwBufLen);

    if (!data)
       return FALSE;

    if (!data->IMC.hCompStr)
       return FALSE;

    compdata = ImmLockIMCC(data->IMC.hCompStr);
    compstr = (LPCOMPOSITIONSTRING)compdata;

    switch (dwIndex)
    {
    case GCS_RESULTSTR:
        TRACE("GCS_RESULTSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwResultStrOffset, compstr->dwResultStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPSTR:
        TRACE("GCS_COMPSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwCompStrOffset, compstr->dwCompStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPATTR:
        TRACE("GCS_COMPATTR\n");
        rc = CopyCompAttrIMEtoClient(data, compdata + compstr->dwCompAttrOffset, compstr->dwCompAttrLen,
                                     compdata + compstr->dwCompStrOffset, compstr->dwCompStrLen,
                                     lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPCLAUSE:
        TRACE("GCS_COMPCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwCompClauseOffset,compstr->dwCompClauseLen,
                                       compdata + compstr->dwCompStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_RESULTCLAUSE:
        TRACE("GCS_RESULTCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwResultClauseOffset,compstr->dwResultClauseLen,
                                       compdata + compstr->dwResultStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_RESULTREADSTR:
        TRACE("GCS_RESULTREADSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwResultReadStrOffset, compstr->dwResultReadStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_RESULTREADCLAUSE:
        TRACE("GCS_RESULTREADCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwResultReadClauseOffset,compstr->dwResultReadClauseLen,
                                       compdata + compstr->dwResultStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPREADSTR:
        TRACE("GCS_COMPREADSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwCompReadStrOffset, compstr->dwCompReadStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPREADATTR:
        TRACE("GCS_COMPREADATTR\n");
        rc = CopyCompAttrIMEtoClient(data, compdata + compstr->dwCompReadAttrOffset, compstr->dwCompReadAttrLen,
                                     compdata + compstr->dwCompReadStrOffset, compstr->dwCompReadStrLen,
                                     lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPREADCLAUSE:
        TRACE("GCS_COMPREADCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwCompReadClauseOffset,compstr->dwCompReadClauseLen,
                                       compdata + compstr->dwCompStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_CURSORPOS:
        TRACE("GCS_CURSORPOS\n");
        rc = CopyCompOffsetIMEtoClient(data, compstr->dwCursorPos, compdata + compstr->dwCompStrOffset, unicode);
        break;
    case GCS_DELTASTART:
        TRACE("GCS_DELTASTART\n");
        rc = CopyCompOffsetIMEtoClient(data, compstr->dwDeltaStart, compdata + compstr->dwCompStrOffset, unicode);
        break;
    default:
        FIXME("Unhandled index 0x%x\n",dwIndex);
        break;
    }

    ImmUnlockIMCC(data->IMC.hCompStr);

    return rc;
}

/***********************************************************************
 *		ImmGetCompositionStringA (IMM32.@)
 */
LONG WINAPI ImmGetCompositionStringA(
  HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
    return ImmGetCompositionStringT(hIMC, dwIndex, lpBuf, dwBufLen, FALSE);
}


/***********************************************************************
 *		ImmGetCompositionStringW (IMM32.@)
 */
LONG WINAPI ImmGetCompositionStringW(
  HIMC hIMC, DWORD dwIndex,
  LPVOID lpBuf, DWORD dwBufLen)
{
    return ImmGetCompositionStringT(hIMC, dwIndex, lpBuf, dwBufLen, TRUE);
}

/***********************************************************************
 *		ImmGetCompositionWindow (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionWindow(HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
    LPINPUTCONTEXT pIC;
    BOOL ret = FALSE;

    TRACE("(%p, %p)\n", hIMC, lpCompForm);

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return FALSE;

    if (pIC->fdwInit & INIT_COMPFORM)
    {
        *lpCompForm = pIC->cfCompForm;
        ret = TRUE;
    }

    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmGetContext (IMM32.@)
 */
HIMC WINAPI ImmGetContext(HWND hWnd)
{
    HIMC rc;

    TRACE("%p\n", hWnd);

    if (!IsWindow(hWnd))
    {
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
        return NULL;
    }

    rc = GetPropW(hWnd,szwWineIMCProperty);
    if (rc == (HIMC)-1)
        rc = NULL;
    else if (rc == NULL)
        rc = get_default_context( hWnd );

    if (rc)
    {
        InputContextData *data = (InputContextData *)rc;
        data->IMC.hWnd = hWnd;
    }

    TRACE("returning %p\n", rc);

    return rc;
}

/***********************************************************************
 *		ImmGetConversionListA (IMM32.@)
 */
DWORD WINAPI ImmGetConversionListA(
  HKL hKL, HIMC hIMC,
  LPCSTR pSrc, LPCANDIDATELIST lpDst,
  DWORD dwBufLen, UINT uFlag)
{
    DWORD ret = 0;
    UINT cb;
    LPWSTR pszSrcW = NULL;
    LPCANDIDATELIST pCL = NULL;
    PIMEDPI pImeDpi;

    TRACE("(%p, %p, %s, %p, %lu, 0x%lX)\n", hKL, hIMC, debugstr_a(pSrc),
          lpDst, dwBufLen, uFlag);

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (pImeDpi == NULL)
        return 0;

    if (!(pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE))
    {
        ret = pImeDpi->ImeConversionList(hIMC, pSrc, lpDst, dwBufLen, uFlag);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (pSrc)
    {
        pszSrcW = Imm32WideFromAnsi(pSrc);
        if (pszSrcW == NULL)
            goto Quit;
    }

    cb = pImeDpi->ImeConversionList(hIMC, pszSrcW, NULL, 0, uFlag);
    if (cb == 0)
        goto Quit;

    pCL = Imm32HeapAlloc(0, cb);
    if (pCL == NULL)
        goto Quit;

    cb = pImeDpi->ImeConversionList(hIMC, pszSrcW, pCL, cb, uFlag);
    if (cb == 0)
        goto Quit;

    ret = CandidateListWideToAnsi(pCL, lpDst, dwBufLen, CP_ACP);

Quit:
    if (pszSrcW)
        HeapFree(g_hImm32Heap, 0, pszSrcW);
    if (pCL)
        HeapFree(g_hImm32Heap, 0, pCL);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmGetConversionListW (IMM32.@)
 */
DWORD WINAPI ImmGetConversionListW(
  HKL hKL, HIMC hIMC,
  LPCWSTR pSrc, LPCANDIDATELIST lpDst,
  DWORD dwBufLen, UINT uFlag)
{
    DWORD ret = 0;
    INT cb;
    PIMEDPI pImeDpi;
    LPCANDIDATELIST pCL = NULL;
    LPSTR pszSrcA = NULL;

    TRACE("(%p, %p, %s, %p, %lu, 0x%lX)\n", hKL, hIMC, debugstr_w(pSrc),
          lpDst, dwBufLen, uFlag);

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return 0;

    if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE)
    {
        ret = pImeDpi->ImeConversionList(hIMC, pSrc, lpDst, dwBufLen, uFlag);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (pSrc)
    {
        pszSrcA = Imm32AnsiFromWide(pSrc);
        if (pszSrcA == NULL)
            goto Quit;
    }

    cb = pImeDpi->ImeConversionList(hIMC, pszSrcA, NULL, 0, uFlag);
    if (cb == 0)
        goto Quit;

    pCL = Imm32HeapAlloc(0, cb);
    if (!pCL)
        goto Quit;

    cb = pImeDpi->ImeConversionList(hIMC, pszSrcA, pCL, cb, uFlag);
    if (!cb)
        goto Quit;

    ret = CandidateListAnsiToWide(pCL, lpDst, dwBufLen, CP_ACP);

Quit:
    if (pszSrcA)
        HeapFree(g_hImm32Heap, 0, pszSrcA);
    if (pCL)
        HeapFree(g_hImm32Heap, 0, pCL);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmGetConversionStatus (IMM32.@)
 */
BOOL WINAPI ImmGetConversionStatus(
  HIMC hIMC, LPDWORD lpfdwConversion, LPDWORD lpfdwSentence)
{
    LPINPUTCONTEXT pIC;

    TRACE("(%p %p %p)\n", hIMC, lpfdwConversion, lpfdwSentence);

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return FALSE;

    if (lpfdwConversion)
        *lpfdwConversion = pIC->fdwConversion;
    if (lpfdwSentence)
        *lpfdwSentence = pIC->fdwSentence;

    ImmUnlockIMC(hIMC);
    return TRUE;
}

/***********************************************************************
 *		ImmGetDefaultIMEWnd (IMM32.@)
 */
HWND WINAPI ImmGetDefaultIMEWnd(HWND hWnd)
{
    if (!g_psi || !(g_psi->dwSRVIFlags & SRVINFO_IMM32))
        return NULL;

    // FIXME: NtUserGetThreadState and enum ThreadStateRoutines are broken.
    if (hWnd == NULL)
        return (HWND)NtUserGetThreadState(3);

    return (HWND)NtUserQueryWindow(hWnd, QUERY_WINDOW_DEFAULT_IME);
}

/***********************************************************************
 *		CtfImmIsCiceroEnabled (IMM32.@)
 */
BOOL WINAPI CtfImmIsCiceroEnabled(VOID)
{
    return (g_psi && (g_psi->dwSRVIFlags & SRVINFO_CICERO_ENABLED));
}

/***********************************************************************
 *		ImmGetDescriptionA (IMM32.@)
 */
UINT WINAPI ImmGetDescriptionA(
  HKL hKL, LPSTR lpszDescription, UINT uBufLen)
{
    IMEINFOEX info;
    size_t cch;

    TRACE("(%p,%p,%d)\n", hKL, lpszDescription, uBufLen);

    if (!ImmGetImeInfoEx(&info, ImeInfoExKeyboardLayout, &hKL) || !IS_IME_HKL(hKL))
        return 0;

    StringCchLengthW(info.wszImeDescription, _countof(info.wszImeDescription), &cch);
    cch = WideCharToMultiByte(CP_ACP, 0, info.wszImeDescription, (INT)cch,
                              lpszDescription, uBufLen, NULL, NULL);
    if (uBufLen)
        lpszDescription[cch] = 0;
    return cch;
}

/***********************************************************************
 *		ImmGetDescriptionW (IMM32.@)
 */
UINT WINAPI ImmGetDescriptionW(HKL hKL, LPWSTR lpszDescription, UINT uBufLen)
{
    IMEINFOEX info;
    size_t cch;

    TRACE("(%p, %p, %d)\n", hKL, lpszDescription, uBufLen);

    if (!ImmGetImeInfoEx(&info, ImeInfoExKeyboardLayout, &hKL) || !IS_IME_HKL(hKL))
        return 0;

    if (uBufLen != 0)
        StringCchCopyW(lpszDescription, uBufLen, info.wszImeDescription);

    StringCchLengthW(info.wszImeDescription, _countof(info.wszImeDescription), &cch);
    return (UINT)cch;
}

static DWORD APIENTRY
ImmGetGuideLineAW(HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen, BOOL bAnsi)
{
    PCLIENTIMC pClientImc;
    LPINPUTCONTEXT pIC;
    LPGUIDELINE pGuideLine;
    DWORD cb, ret = 0;
    LPVOID pvStr, pvPrivate;
    BOOL bUsedDefault;

    pClientImc = ImmLockClientImc(hIMC);
    if (!pClientImc)
        return 0;

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
    {
        ImmUnlockClientImc(pClientImc);
        return 0;
    }

    pGuideLine = ImmLockIMCC(pIC->hGuideLine);
    if (!pGuideLine)
    {
        ImmUnlockIMC(hIMC);
        ImmUnlockClientImc(pClientImc);
        return 0;
    }

    if (dwIndex == GGL_LEVEL)
    {
        ret = pGuideLine->dwLevel;
        goto Quit;
    }

    if (dwIndex == GGL_INDEX)
    {
        ret = pGuideLine->dwIndex;
        goto Quit;
    }

    if (dwIndex == GGL_STRING)
    {
        pvStr = (LPBYTE)pGuideLine + pGuideLine->dwStrOffset;

        /* get size */
        if (bAnsi)
        {
            if (pClientImc->dwFlags & CLIENTIMC_WIDE)
            {
                cb = WideCharToMultiByte(CP_ACP, 0, pvStr, pGuideLine->dwStrLen,
                                         NULL, 0, NULL, &bUsedDefault);
            }
            else
            {
                cb = pGuideLine->dwStrLen * sizeof(CHAR);
            }
        }
        else
        {
            if (pClientImc->dwFlags & CLIENTIMC_WIDE)
            {
                cb = pGuideLine->dwStrLen * sizeof(WCHAR);
            }
            else
            {
                cb = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pvStr, pGuideLine->dwStrLen,
                                         NULL, 0) * sizeof(WCHAR);
            }
        }

        if (dwBufLen == 0 || cb == 0 || lpBuf == NULL || dwBufLen < cb)
        {
            ret = cb;
            goto Quit;
        }

        /* store to buffer */
        if (bAnsi)
        {
            if (pClientImc->dwFlags & CLIENTIMC_WIDE)
            {
                ret = WideCharToMultiByte(CP_ACP, 0, pvStr, pGuideLine->dwStrLen,
                                          lpBuf, dwBufLen, NULL, &bUsedDefault);
                goto Quit;
            }
        }
        else
        {
            if (!(pClientImc->dwFlags & CLIENTIMC_WIDE))
            {
                ret = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pvStr, pGuideLine->dwStrLen,
                                          lpBuf, dwBufLen) * sizeof(WCHAR);
                goto Quit;
            }
        }

        RtlCopyMemory(lpBuf, pvStr, cb);
        ret = cb;
        goto Quit;
    }

    if (dwIndex == GGL_PRIVATE)
    {
        pvPrivate = (LPBYTE)pGuideLine + pGuideLine->dwPrivateOffset;

        /* get size */
        if (bAnsi)
        {
            if ((pClientImc->dwFlags & CLIENTIMC_WIDE) &&
                pGuideLine->dwIndex == GL_ID_REVERSECONVERSION)
            {
                cb = CandidateListWideToAnsi(pvPrivate, NULL, 0, CP_ACP);
            }
            else
            {
                cb = pGuideLine->dwPrivateSize;
            }
        }
        else
        {
            if (!(pClientImc->dwFlags & CLIENTIMC_WIDE) &&
                pGuideLine->dwIndex == GL_ID_REVERSECONVERSION)
            {
                cb = CandidateListAnsiToWide(pvPrivate, NULL, 0, CP_ACP);
            }
            else
            {
                cb = pGuideLine->dwPrivateSize;
            }
        }

        if (dwBufLen == 0 || cb == 0 || lpBuf == NULL || dwBufLen < cb)
        {
            ret = cb;
            goto Quit;
        }

        /* store to buffer */
        if (bAnsi)
        {
            if ((pClientImc->dwFlags & CLIENTIMC_WIDE) &&
                pGuideLine->dwIndex == GL_ID_REVERSECONVERSION)
            {
                ret = CandidateListWideToAnsi(pvPrivate, lpBuf, cb, CP_ACP);
                goto Quit;
            }
        }
        else
        {
            if (!(pClientImc->dwFlags & CLIENTIMC_WIDE) &&
                pGuideLine->dwIndex == GL_ID_REVERSECONVERSION)
            {
                ret = CandidateListAnsiToWide(pvPrivate, lpBuf, cb, CP_ACP);
                goto Quit;
            }
        }

        RtlCopyMemory(lpBuf, pvPrivate, cb);
        ret = cb;
        goto Quit;
    }

Quit:
    ImmUnlockIMCC(pIC->hGuideLine);
    ImmUnlockIMC(hIMC);
    ImmUnlockClientImc(pClientImc);
    return ret;
}

/***********************************************************************
 *		ImmGetGuideLineA (IMM32.@)
 */
DWORD WINAPI ImmGetGuideLineA(
  HIMC hIMC, DWORD dwIndex, LPSTR lpBuf, DWORD dwBufLen)
{
    TRACE("(%p, %lu, %p, %lu)\n", hIMC, dwIndex, lpBuf, dwBufLen);
    return ImmGetGuideLineAW(hIMC, dwIndex, lpBuf, dwBufLen, TRUE);
}

/***********************************************************************
 *		ImmGetGuideLineW (IMM32.@)
 */
DWORD WINAPI ImmGetGuideLineW(HIMC hIMC, DWORD dwIndex, LPWSTR lpBuf, DWORD dwBufLen)
{
    TRACE("(%p, %lu, %p, %lu)\n", hIMC, dwIndex, lpBuf, dwBufLen);
    return ImmGetGuideLineAW(hIMC, dwIndex, lpBuf, dwBufLen, FALSE);
}

/***********************************************************************
 *		ImmGetIMEFileNameA (IMM32.@)
 */
UINT WINAPI ImmGetIMEFileNameA( HKL hKL, LPSTR lpszFileName, UINT uBufLen)
{
    BOOL bDefUsed;
    IMEINFOEX info;
    size_t cch;

    TRACE("(%p, %p, %u)\n", hKL, lpszFileName, uBufLen);

    if (!ImmGetImeInfoEx(&info, ImeInfoExKeyboardLayout, &hKL) || !IS_IME_HKL(hKL))
    {
        if (uBufLen > 0)
            lpszFileName[0] = 0;
        return 0;
    }

    StringCchLengthW(info.wszImeFile, _countof(info.wszImeFile), &cch);

    cch = WideCharToMultiByte(CP_ACP, 0, info.wszImeFile, (INT)cch,
                              lpszFileName, uBufLen, NULL, &bDefUsed);
    if (uBufLen == 0)
        return (UINT)cch;

    if (cch > uBufLen - 1)
        cch = uBufLen - 1;

    lpszFileName[cch] = 0;
    return (UINT)cch;
}

/***********************************************************************
 *		ImmGetIMEFileNameW (IMM32.@)
 */
UINT WINAPI ImmGetIMEFileNameW(HKL hKL, LPWSTR lpszFileName, UINT uBufLen)
{
    IMEINFOEX info;
    size_t cch;

    TRACE("(%p, %p, %u)\n", hKL, lpszFileName, uBufLen);

    if (!ImmGetImeInfoEx(&info, ImeInfoExKeyboardLayout, &hKL) || !IS_IME_HKL(hKL))
    {
        if (uBufLen > 0)
            lpszFileName[0] = 0;
        return 0;
    }

    StringCchLengthW(info.wszImeFile, _countof(info.wszImeFile), &cch);
    if (uBufLen == 0)
        return (UINT)cch;

    StringCchCopyNW(lpszFileName, uBufLen, info.wszImeFile, cch);

    if (cch > uBufLen - 1)
        cch = uBufLen - 1;

    lpszFileName[cch] = 0;
    return (UINT)cch;
}

/***********************************************************************
 *		ImmGetOpenStatus (IMM32.@)
 */
BOOL WINAPI ImmGetOpenStatus(HIMC hIMC)
{
    BOOL ret;
    LPINPUTCONTEXT pIC;

    TRACE("(%p)\n", hIMC);

    if (!hIMC)
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return FALSE;

    ret = pIC->fOpen;

    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmGetProperty (IMM32.@)
 */
DWORD WINAPI ImmGetProperty(HKL hKL, DWORD fdwIndex)
{
    IMEINFOEX ImeInfoEx;
    LPIMEINFO pImeInfo;
    DWORD dwValue;
    PIMEDPI pImeDpi = NULL;

    TRACE("(%p, %lu)\n", hKL, fdwIndex);

    if (!ImmGetImeInfoEx(&ImeInfoEx, ImeInfoExKeyboardLayout, &hKL))
        return FALSE;

    if (fdwIndex == IGP_GETIMEVERSION)
        return ImeInfoEx.dwImeWinVersion;

    if (ImeInfoEx.fLoadFlag != 2)
    {
        pImeDpi = ImmLockOrLoadImeDpi(hKL);
        if (pImeDpi == NULL)
            return FALSE;

        pImeInfo = &pImeDpi->ImeInfo;
    }
    else
    {
        pImeInfo = &ImeInfoEx.ImeInfo;
    }

    switch (fdwIndex)
    {
        case IGP_PROPERTY:      dwValue = pImeInfo->fdwProperty; break;
        case IGP_CONVERSION:    dwValue = pImeInfo->fdwConversionCaps; break;
        case IGP_SENTENCE:      dwValue = pImeInfo->fdwSentenceCaps; break;
        case IGP_UI:            dwValue = pImeInfo->fdwUICaps; break;
        case IGP_SETCOMPSTR:    dwValue = pImeInfo->fdwSCSCaps; break;
        case IGP_SELECT:        dwValue = pImeInfo->fdwSelectCaps; break;
        default:                dwValue = 0; break;
    }

    if (pImeDpi)
        ImmUnlockImeDpi(pImeDpi);
    return dwValue;
}

/***********************************************************************
 *		ImmGetRegisterWordStyleA (IMM32.@)
 */
UINT WINAPI ImmGetRegisterWordStyleA(
  HKL hKL, UINT nItem, LPSTYLEBUFA lpStyleBuf)
{
    UINT iItem, ret = 0;
    PIMEDPI pImeDpi;
    LPSTYLEBUFA pDestA;
    LPSTYLEBUFW pSrcW, pNewStylesW = NULL;
    size_t cchW;
    INT cchA;

    TRACE("(%p, %u, %p)\n", hKL, nItem, lpStyleBuf);

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return 0;

    if (!(pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE))
    {
        ret = pImeDpi->ImeGetRegisterWordStyle(nItem, lpStyleBuf);
        goto Quit;
    }

    if (nItem > 0)
    {
        pNewStylesW = Imm32HeapAlloc(0, nItem * sizeof(STYLEBUFW));
        if (!pNewStylesW)
            goto Quit;
    }

    ret = pImeDpi->ImeGetRegisterWordStyle(nItem, pNewStylesW);

    if (nItem > 0)
    {
        /* lpStyleBuf <-- pNewStylesW */
        for (iItem = 0; iItem < ret; ++iItem)
        {
            pSrcW = &pNewStylesW[iItem];
            pDestA = &lpStyleBuf[iItem];
            pDestA->dwStyle = pSrcW->dwStyle;
            StringCchLengthW(pSrcW->szDescription, _countof(pSrcW->szDescription), &cchW);
            cchA = WideCharToMultiByte(CP_ACP, MB_PRECOMPOSED,
                                       pSrcW->szDescription, (INT)cchW,
                                       pDestA->szDescription, _countof(pDestA->szDescription),
                                       NULL, NULL);
            if (cchA > _countof(pDestA->szDescription) - 1)
                cchA = _countof(pDestA->szDescription) - 1;
            pDestA->szDescription[cchA] = 0;
        }
    }

Quit:
    if (pNewStylesW)
        HeapFree(g_hImm32Heap, 0, pNewStylesW);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmGetRegisterWordStyleW (IMM32.@)
 */
UINT WINAPI ImmGetRegisterWordStyleW(
  HKL hKL, UINT nItem, LPSTYLEBUFW lpStyleBuf)
{
    UINT iItem, ret = 0;
    PIMEDPI pImeDpi;
    LPSTYLEBUFA pSrcA, pNewStylesA = NULL;
    LPSTYLEBUFW pDestW;
    size_t cchA;
    INT cchW;

    TRACE("(%p, %u, %p)\n", hKL, nItem, lpStyleBuf);

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return 0;

    if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE)
    {
        ret = pImeDpi->ImeGetRegisterWordStyle(nItem, lpStyleBuf);
        goto Quit;
    }

    if (nItem > 0)
    {
        pNewStylesA = Imm32HeapAlloc(0, nItem * sizeof(STYLEBUFA));
        if (!pNewStylesA)
            goto Quit;
    }

    ret = pImeDpi->ImeGetRegisterWordStyle(nItem, pNewStylesA);

    if (nItem > 0)
    {
        /* lpStyleBuf <-- pNewStylesA */
        for (iItem = 0; iItem < ret; ++iItem)
        {
            pSrcA = &pNewStylesA[iItem];
            pDestW = &lpStyleBuf[iItem];
            pDestW->dwStyle = pSrcA->dwStyle;
            StringCchLengthA(pSrcA->szDescription, _countof(pSrcA->szDescription), &cchA);
            cchW = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                                       pSrcA->szDescription, (INT)cchA,
                                       pDestW->szDescription, _countof(pDestW->szDescription));
            if (cchW > _countof(pDestW->szDescription) - 1)
                cchW = _countof(pDestW->szDescription) - 1;
            pDestW->szDescription[cchW] = 0;
        }
    }

Quit:
    if (pNewStylesA)
        HeapFree(g_hImm32Heap, 0, pNewStylesA);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmGetStatusWindowPos (IMM32.@)
 */
BOOL WINAPI ImmGetStatusWindowPos(HIMC hIMC, LPPOINT lpptPos)
{
    LPINPUTCONTEXT pIC;
    BOOL ret;

    TRACE("(%p, %p)\n", hIMC, lpptPos);

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    ret = !!(pIC->fdwInit & INIT_STATUSWNDPOS);
    if (ret)
        *lpptPos = pIC->ptStatusWndPos;

    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmGetVirtualKey (IMM32.@)
 */
UINT WINAPI ImmGetVirtualKey(HWND hWnd)
{
    HIMC hIMC;
    LPINPUTCONTEXTDX pIC;
    UINT ret = VK_PROCESSKEY;

    TRACE("(%p)\n", hWnd);

    hIMC = ImmGetContext(hWnd);
    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (!pIC)
        return ret;

    if (pIC->bNeedsTrans)
        ret = pIC->nVKey;

    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmInstallIMEA (IMM32.@)
 */
HKL WINAPI ImmInstallIMEA(
  LPCSTR lpszIMEFileName, LPCSTR lpszLayoutText)
{
    HKL hKL = NULL;
    LPWSTR pszFileNameW = NULL, pszLayoutTextW = NULL;

    TRACE("(%s, %s)\n", debugstr_a(lpszIMEFileName), debugstr_a(lpszLayoutText));

    pszFileNameW = Imm32WideFromAnsi(lpszIMEFileName);
    if (pszFileNameW == NULL)
        goto Quit;

    pszLayoutTextW = Imm32WideFromAnsi(lpszLayoutText);
    if (pszLayoutTextW == NULL)
        goto Quit;

    hKL = ImmInstallIMEW(pszFileNameW, pszLayoutTextW);

Quit:
    if (pszFileNameW)
        HeapFree(g_hImm32Heap, 0, pszFileNameW);
    if (pszLayoutTextW)
        HeapFree(g_hImm32Heap, 0, pszLayoutTextW);
    return hKL;
}

/***********************************************************************
 *		ImmInstallIMEW (IMM32.@)
 */
HKL WINAPI ImmInstallIMEW(
  LPCWSTR lpszIMEFileName, LPCWSTR lpszLayoutText)
{
    INT lcid = GetUserDefaultLCID();
    INT count;
    HKL hkl;
    DWORD rc;
    HKEY hkey;
    WCHAR regKey[ARRAY_SIZE(szImeRegFmt)+8];

    TRACE ("(%s, %s):\n", debugstr_w(lpszIMEFileName),
                          debugstr_w(lpszLayoutText));

    /* Start with 2.  e001 will be blank and so default to the wine internal IME */
    count = 2;

    while (count < 0xfff)
    {
        DWORD disposition = 0;

        hkl = (HKL)MAKELPARAM( lcid, 0xe000 | count );
        wsprintfW( regKey, szImeRegFmt, (ULONG_PTR)hkl);

        rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE, regKey, 0, NULL, 0, KEY_WRITE, NULL, &hkey, &disposition);
        if (rc == ERROR_SUCCESS && disposition == REG_CREATED_NEW_KEY)
            break;
        else if (rc == ERROR_SUCCESS)
            RegCloseKey(hkey);

        count++;
    }

    if (count == 0xfff)
    {
        WARN("Unable to find slot to install IME\n");
        return 0;
    }

    if (rc == ERROR_SUCCESS)
    {
        rc = RegSetValueExW(hkey, szImeFileW, 0, REG_SZ, (const BYTE*)lpszIMEFileName,
                            (lstrlenW(lpszIMEFileName) + 1) * sizeof(WCHAR));
        if (rc == ERROR_SUCCESS)
            rc = RegSetValueExW(hkey, szLayoutTextW, 0, REG_SZ, (const BYTE*)lpszLayoutText,
                                (lstrlenW(lpszLayoutText) + 1) * sizeof(WCHAR));
        RegCloseKey(hkey);
        return hkl;
    }
    else
    {
        WARN("Unable to set IME registry values\n");
        return 0;
    }
}

/***********************************************************************
 *		ImmIsIME (IMM32.@)
 */
BOOL WINAPI ImmIsIME(HKL hKL)
{
    IMEINFOEX info;
    TRACE("(%p)\n", hKL);
    return !!ImmGetImeInfoEx(&info, ImeInfoExImeWindow, &hKL);
}

static BOOL APIENTRY
ImmIsUIMessageAW(HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam, BOOL bAnsi)
{
    switch (msg)
    {
        case WM_IME_STARTCOMPOSITION: case WM_IME_ENDCOMPOSITION:
        case WM_IME_COMPOSITION: case WM_IME_SETCONTEXT: case WM_IME_NOTIFY:
        case WM_IME_COMPOSITIONFULL: case WM_IME_SELECT: case WM_IME_SYSTEM:
            break;
        default:
            return FALSE;
    }

    if (!hWndIME)
        return TRUE;

    if (bAnsi)
        SendMessageA(hWndIME, msg, wParam, lParam);
    else
        SendMessageW(hWndIME, msg, wParam, lParam);

    return TRUE;
}

/***********************************************************************
 *		ImmIsUIMessageA (IMM32.@)
 */
BOOL WINAPI ImmIsUIMessageA(
  HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%p, 0x%X, %p, %p)\n", hWndIME, msg, wParam, lParam);
    return ImmIsUIMessageAW(hWndIME, msg, wParam, lParam, TRUE);
}

/***********************************************************************
 *		ImmIsUIMessageW (IMM32.@)
 */
BOOL WINAPI ImmIsUIMessageW(
  HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%p, 0x%X, %p, %p)\n", hWndIME, msg, wParam, lParam);
    return ImmIsUIMessageAW(hWndIME, msg, wParam, lParam, FALSE);
}

/***********************************************************************
 *		ImmNotifyIME (IMM32.@)
 */
BOOL WINAPI ImmNotifyIME(
  HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
    HKL hKL;
    PIMEDPI pImeDpi;
    BOOL ret;

    TRACE("(%p, %lu, %lu, %lu)\n", hIMC, dwAction, dwIndex, dwValue);

    if (hIMC && Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    hKL = GetKeyboardLayout(0);
    pImeDpi = ImmLockImeDpi(hKL);
    if (pImeDpi == NULL)
        return FALSE;

    ret = pImeDpi->NotifyIME(hIMC, dwAction, dwIndex, dwValue);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmRegisterWordA (IMM32.@)
 */
BOOL WINAPI ImmRegisterWordA(
  HKL hKL, LPCSTR lpszReading, DWORD dwStyle, LPCSTR lpszRegister)
{
    BOOL ret = FALSE;
    PIMEDPI pImeDpi;
    LPWSTR pszReadingW = NULL, pszRegisterW = NULL;

    TRACE("(%p, %s, 0x%lX, %s)\n", hKL, debugstr_a(lpszReading), dwStyle,
          debugstr_a(lpszRegister));

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return FALSE;

    if (!(pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE))
    {
        ret = pImeDpi->ImeRegisterWord(lpszReading, dwStyle, lpszRegister);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (lpszReading)
    {
        pszReadingW = Imm32WideFromAnsi(lpszReading);
        if (pszReadingW == NULL)
            goto Quit;
    }

    if (lpszRegister)
    {
        pszRegisterW = Imm32WideFromAnsi(lpszRegister);
        if (pszRegisterW == NULL)
            goto Quit;
    }

    ret = pImeDpi->ImeRegisterWord(pszReadingW, dwStyle, pszRegisterW);

Quit:
    if (pszReadingW)
        HeapFree(g_hImm32Heap, 0, pszReadingW);
    if (pszRegisterW)
        HeapFree(g_hImm32Heap, 0, pszRegisterW);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmRegisterWordW (IMM32.@)
 */
BOOL WINAPI ImmRegisterWordW(
  HKL hKL, LPCWSTR lpszReading, DWORD dwStyle, LPCWSTR lpszRegister)
{
    BOOL ret = FALSE;
    PIMEDPI pImeDpi;
    LPSTR pszReadingA = NULL, pszRegisterA = NULL;

    TRACE("(%p, %s, 0x%lX, %s)\n", hKL, debugstr_w(lpszReading), dwStyle,
          debugstr_w(lpszRegister));

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return FALSE;

    if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE)
    {
        ret = pImeDpi->ImeRegisterWord(lpszReading, dwStyle, lpszRegister);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (lpszReading)
    {
        pszReadingA = Imm32AnsiFromWide(lpszReading);
        if (!pszReadingA)
            goto Quit;
    }

    if (lpszRegister)
    {
        pszRegisterA = Imm32AnsiFromWide(lpszRegister);
        if (!pszRegisterA)
            goto Quit;
    }

    ret = pImeDpi->ImeRegisterWord(pszReadingA, dwStyle, pszRegisterA);

Quit:
    if (pszReadingA)
        HeapFree(g_hImm32Heap, 0, pszReadingA);
    if (pszRegisterA)
        HeapFree(g_hImm32Heap, 0, pszRegisterA);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmReleaseContext (IMM32.@)
 */
BOOL WINAPI ImmReleaseContext(HWND hWnd, HIMC hIMC)
{
  static BOOL shown = FALSE;

  if (!shown) {
     FIXME("(%p, %p): stub\n", hWnd, hIMC);
     shown = TRUE;
  }
  return TRUE;
}

/***********************************************************************
*              ImmRequestMessageA(IMM32.@)
*/
LRESULT WINAPI ImmRequestMessageA(HIMC hIMC, WPARAM wParam, LPARAM lParam)
{
    InputContextData *data = get_imc_data(hIMC);

    TRACE("%p %ld %ld\n", hIMC, wParam, wParam);

    if (data) return SendMessageA(data->IMC.hWnd, WM_IME_REQUEST, wParam, lParam);

    SetLastError(ERROR_INVALID_HANDLE);
    return 0;
}

/***********************************************************************
*              ImmRequestMessageW(IMM32.@)
*/
LRESULT WINAPI ImmRequestMessageW(HIMC hIMC, WPARAM wParam, LPARAM lParam)
{
    InputContextData *data = get_imc_data(hIMC);

    TRACE("%p %ld %ld\n", hIMC, wParam, wParam);

    if (data) return SendMessageW(data->IMC.hWnd, WM_IME_REQUEST, wParam, lParam);

    SetLastError(ERROR_INVALID_HANDLE);
    return 0;
}

/***********************************************************************
 *		ImmSetCandidateWindow (IMM32.@)
 */
BOOL WINAPI ImmSetCandidateWindow(
  HIMC hIMC, LPCANDIDATEFORM lpCandidate)
{
    HWND hWnd;
    LPINPUTCONTEXT pIC;

    TRACE("(%p, %p)\n", hIMC, lpCandidate);

    if (lpCandidate->dwIndex >= MAX_CANDIDATEFORM)
        return FALSE;

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    hWnd = pIC->hWnd;
    pIC->cfCandForm[lpCandidate->dwIndex] = *lpCandidate;

    ImmUnlockIMC(hIMC);

    Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, 0, IMC_SETCANDIDATEPOS,
                      IMN_SETCANDIDATEPOS, (1 << lpCandidate->dwIndex));
    return TRUE;
}

static VOID APIENTRY WideToAnsiLogFont(const LOGFONTW *plfW, LPLOGFONTA plfA)
{
    BOOL bUsedDef;
    size_t cchW, cchA = _countof(plfA->lfFaceName);
    RtlCopyMemory(plfA, plfW, offsetof(LOGFONTA, lfFaceName));
    StringCchLengthW(plfW->lfFaceName, _countof(plfW->lfFaceName), &cchW);
    cchA = WideCharToMultiByte(CP_ACP, 0, plfW->lfFaceName, (INT)cchW,
                               plfA->lfFaceName, (INT)cchA, NULL, &bUsedDef);
    if (cchA > _countof(plfA->lfFaceName) - 1)
        cchA = _countof(plfA->lfFaceName) - 1;
    plfA->lfFaceName[cchA] = 0;
}

static VOID APIENTRY AnsiToWideLogFont(const LOGFONTA *plfA, LPLOGFONTW plfW)
{
    size_t cchA, cchW = _countof(plfW->lfFaceName);
    RtlCopyMemory(plfW, plfA, offsetof(LOGFONTW, lfFaceName));
    StringCchLengthA(plfA->lfFaceName, _countof(plfA->lfFaceName), &cchA);
    cchW = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, plfA->lfFaceName, (INT)cchA,
                               plfW->lfFaceName, cchW);
    if (cchW > _countof(plfW->lfFaceName) - 1)
        cchW = _countof(plfW->lfFaceName) - 1;
    plfW->lfFaceName[cchW] = 0;
}

/***********************************************************************
 *		ImmSetCompositionFontA (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontA(HIMC hIMC, LPLOGFONTA lplf)
{
    LOGFONTW lfW;
    PCLIENTIMC pClientImc;
    BOOL bWide;
    LPINPUTCONTEXTDX pIC;
    LCID lcid;
    HWND hWnd;
    PTEB pTeb;

    TRACE("(%p, %p)\n", hIMC, lplf);

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return FALSE;

    bWide = (pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    if (bWide)
    {
        AnsiToWideLogFont(lplf, &lfW);
        return ImmSetCompositionFontW(hIMC, &lfW);
    }

    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    pTeb = NtCurrentTeb();
    if (pTeb->Win32ClientInfo[2] < 0x400)
    {
        lcid = GetSystemDefaultLCID();
        if (PRIMARYLANGID(lcid) == LANG_JAPANESE && !(pIC->dwUIFlags & 2) &&
            pIC->cfCompForm.dwStyle != CFS_DEFAULT)
        {
            PostMessageA(pIC->hWnd, WM_IME_REPORT, IR_CHANGECONVERT, 0);
        }
    }

    pIC->lfFont.A = *lplf;
    pIC->fdwInit |= INIT_LOGFONT;
    hWnd = pIC->hWnd;

    ImmUnlockIMC(hIMC);

    Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, 0, IMC_SETCOMPOSITIONFONT,
                      IMN_SETCOMPOSITIONFONT, 0);
    return TRUE;
}

/***********************************************************************
 *		ImmSetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
    LOGFONTA lfA;
    PCLIENTIMC pClientImc;
    BOOL bWide;
    HWND hWnd;
    LPINPUTCONTEXTDX pIC;
    PTEB pTeb;
    LCID lcid;

    TRACE("(%p, %p)\n", hIMC, lplf);

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return FALSE;

    bWide = (pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    if (!bWide)
    {
        WideToAnsiLogFont(lplf, &lfA);
        return ImmSetCompositionFontA(hIMC, &lfA);
    }

    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    pTeb = NtCurrentTeb();
    if (pTeb->Win32ClientInfo[2] < 0x400)
    {
        lcid = GetSystemDefaultLCID();
        if (PRIMARYLANGID(lcid) == LANG_JAPANESE &&
            !(pIC->dwUIFlags & 2) &&
            pIC->cfCompForm.dwStyle != CFS_DEFAULT)
        {
            PostMessageW(pIC->hWnd, WM_IME_REPORT, IR_CHANGECONVERT, 0);
        }
    }

    pIC->lfFont.W = *lplf;
    pIC->fdwInit |= INIT_LOGFONT;
    hWnd = pIC->hWnd;

    ImmUnlockIMC(hIMC);

    Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, 0, IMC_SETCOMPOSITIONFONT,
                      IMN_SETCOMPOSITIONFONT, 0);
    return TRUE;
}

/***********************************************************************
 *		ImmSetCompositionStringA (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionStringA(
  HIMC hIMC, DWORD dwIndex,
  LPCVOID lpComp, DWORD dwCompLen,
  LPCVOID lpRead, DWORD dwReadLen)
{
    DWORD comp_len;
    DWORD read_len;
    WCHAR *CompBuffer = NULL;
    WCHAR *ReadBuffer = NULL;
    BOOL rc;
    InputContextData *data = get_imc_data(hIMC);

    TRACE("(%p, %d, %p, %d, %p, %d):\n",
            hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);

    if (!data)
        return FALSE;

    if (!(dwIndex == SCS_SETSTR ||
          dwIndex == SCS_CHANGEATTR ||
          dwIndex == SCS_CHANGECLAUSE ||
          dwIndex == SCS_SETRECONVERTSTRING ||
          dwIndex == SCS_QUERYRECONVERTSTRING))
        return FALSE;

    if (!is_himc_ime_unicode(data))
        return data->immKbd->pImeSetCompositionString(hIMC, dwIndex, lpComp,
                        dwCompLen, lpRead, dwReadLen);

    comp_len = MultiByteToWideChar(CP_ACP, 0, lpComp, dwCompLen, NULL, 0);
    if (comp_len)
    {
        CompBuffer = HeapAlloc(GetProcessHeap(),0,comp_len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpComp, dwCompLen, CompBuffer, comp_len);
    }

    read_len = MultiByteToWideChar(CP_ACP, 0, lpRead, dwReadLen, NULL, 0);
    if (read_len)
    {
        ReadBuffer = HeapAlloc(GetProcessHeap(),0,read_len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpRead, dwReadLen, ReadBuffer, read_len);
    }

    rc =  ImmSetCompositionStringW(hIMC, dwIndex, CompBuffer, comp_len,
                                   ReadBuffer, read_len);

    HeapFree(GetProcessHeap(), 0, CompBuffer);
    HeapFree(GetProcessHeap(), 0, ReadBuffer);

    return rc;
}

/***********************************************************************
 *		ImmSetCompositionStringW (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionStringW(
	HIMC hIMC, DWORD dwIndex,
	LPCVOID lpComp, DWORD dwCompLen,
	LPCVOID lpRead, DWORD dwReadLen)
{
    DWORD comp_len;
    DWORD read_len;
    CHAR *CompBuffer = NULL;
    CHAR *ReadBuffer = NULL;
    BOOL rc;
    InputContextData *data = get_imc_data(hIMC);

    TRACE("(%p, %d, %p, %d, %p, %d):\n",
            hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);

    if (!data)
        return FALSE;

    if (!(dwIndex == SCS_SETSTR ||
          dwIndex == SCS_CHANGEATTR ||
          dwIndex == SCS_CHANGECLAUSE ||
          dwIndex == SCS_SETRECONVERTSTRING ||
          dwIndex == SCS_QUERYRECONVERTSTRING))
        return FALSE;

    if (is_himc_ime_unicode(data))
        return data->immKbd->pImeSetCompositionString(hIMC, dwIndex, lpComp,
                        dwCompLen, lpRead, dwReadLen);

    comp_len = WideCharToMultiByte(CP_ACP, 0, lpComp, dwCompLen, NULL, 0, NULL,
                                   NULL);
    if (comp_len)
    {
        CompBuffer = HeapAlloc(GetProcessHeap(),0,comp_len);
        WideCharToMultiByte(CP_ACP, 0, lpComp, dwCompLen, CompBuffer, comp_len,
                            NULL, NULL);
    }

    read_len = WideCharToMultiByte(CP_ACP, 0, lpRead, dwReadLen, NULL, 0, NULL,
                                   NULL);
    if (read_len)
    {
        ReadBuffer = HeapAlloc(GetProcessHeap(),0,read_len);
        WideCharToMultiByte(CP_ACP, 0, lpRead, dwReadLen, ReadBuffer, read_len,
                            NULL, NULL);
    }

    rc =  ImmSetCompositionStringA(hIMC, dwIndex, CompBuffer, comp_len,
                                   ReadBuffer, read_len);

    HeapFree(GetProcessHeap(), 0, CompBuffer);
    HeapFree(GetProcessHeap(), 0, ReadBuffer);

    return rc;
}

/***********************************************************************
 *		ImmSetCompositionWindow (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionWindow(
  HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
    LPINPUTCONTEXT pIC;
    HWND hWnd;

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    pIC->cfCompForm = *lpCompForm;
    pIC->fdwInit |= INIT_COMPFORM;

    hWnd = pIC->hWnd;

    ImmUnlockIMC(hIMC);

    Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, 0,
                      IMC_SETCOMPOSITIONWINDOW, IMN_SETCOMPOSITIONWINDOW, 0);
    return TRUE;
}

/***********************************************************************
 *		ImmSetConversionStatus (IMM32.@)
 */
BOOL WINAPI ImmSetConversionStatus(
  HIMC hIMC, DWORD fdwConversion, DWORD fdwSentence)
{
    HKL hKL;
    LPINPUTCONTEXT pIC;
    DWORD dwOldConversion, dwOldSentence;
    BOOL fConversionChange = FALSE, fSentenceChange = FALSE;
    HWND hWnd;

    TRACE("(%p, 0x%lX, 0x%lX)\n", hIMC, fdwConversion, fdwSentence);

    hKL = GetKeyboardLayout(0);
    if (!IS_IME_HKL(hKL))
    {
        if (g_psi && (g_psi->dwSRVIFlags & SRVINFO_CICERO_ENABLED))
        {
            FIXME("Cicero\n");
            return FALSE;
        }
    }

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    if (pIC->fdwConversion != fdwConversion)
    {
        dwOldConversion = pIC->fdwConversion;
        pIC->fdwConversion = fdwConversion;
        fConversionChange = TRUE;
    }

    if (pIC->fdwSentence != fdwSentence)
    {
        dwOldSentence = pIC->fdwSentence;
        pIC->fdwSentence = fdwSentence;
        fSentenceChange = TRUE;
    }

    hWnd = pIC->hWnd;
    ImmUnlockIMC(hIMC);

    if (fConversionChange)
    {
        Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, dwOldConversion,
                          IMC_SETCONVERSIONMODE, IMN_SETCONVERSIONMODE, 0);
        NtUserNotifyIMEStatus(hWnd, hIMC, fdwConversion);
    }

    if (fSentenceChange)
    {
        Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, dwOldSentence,
                          IMC_SETSENTENCEMODE, IMN_SETSENTENCEMODE, 0);
    }

    return TRUE;
}

/***********************************************************************
 *		ImmLockImeDpi (IMM32.@)
 */
PIMEDPI WINAPI ImmLockImeDpi(HKL hKL)
{
    PIMEDPI pImeDpi = NULL;

    TRACE("(%p)\n", hKL);

    RtlEnterCriticalSection(&g_csImeDpi);

    /* Find by hKL */
    for (pImeDpi = g_pImeDpiList; pImeDpi; pImeDpi = pImeDpi->pNext)
    {
        if (pImeDpi->hKL == hKL) /* found */
        {
            /* lock if possible */
            if (pImeDpi->dwFlags & IMEDPI_FLAG_UNKNOWN)
                pImeDpi = NULL;
            else
                ++(pImeDpi->cLockObj);
            break;
        }
    }

    RtlLeaveCriticalSection(&g_csImeDpi);
    return pImeDpi;
}

/***********************************************************************
 *		ImmUnlockImeDpi (IMM32.@)
 */
VOID WINAPI ImmUnlockImeDpi(PIMEDPI pImeDpi)
{
    PIMEDPI *ppEntry;

    TRACE("(%p)\n", pImeDpi);

    if (pImeDpi == NULL)
        return;

    RtlEnterCriticalSection(&g_csImeDpi);

    /* unlock */
    --(pImeDpi->cLockObj);
    if (pImeDpi->cLockObj != 0)
    {
        RtlLeaveCriticalSection(&g_csImeDpi);
        return;
    }

    if ((pImeDpi->dwFlags & IMEDPI_FLAG_UNKNOWN) == 0)
    {
        if ((pImeDpi->dwFlags & IMEDPI_FLAG_LOCKED) == 0 ||
            (pImeDpi->ImeInfo.fdwProperty & IME_PROP_END_UNLOAD) == 0)
        {
            RtlLeaveCriticalSection(&g_csImeDpi);
            return;
        }
    }

    /* Remove from list */
    for (ppEntry = &g_pImeDpiList; *ppEntry; ppEntry = &((*ppEntry)->pNext))
    {
        if (*ppEntry == pImeDpi) /* found */
        {
            *ppEntry = pImeDpi->pNext;
            break;
        }
    }

    Imm32FreeImeDpi(pImeDpi, TRUE);
    HeapFree(g_hImm32Heap, 0, pImeDpi);

    RtlLeaveCriticalSection(&g_csImeDpi);
}

/***********************************************************************
 *		ImmSetOpenStatus (IMM32.@)
 */
BOOL WINAPI ImmSetOpenStatus(HIMC hIMC, BOOL fOpen)
{
    DWORD dwConversion;
    LPINPUTCONTEXT pIC;
    HWND hWnd;
    BOOL bHasChange = FALSE;

    TRACE("(%p, %d)\n", hIMC, fOpen);

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    if (pIC->fOpen != fOpen)
    {
        pIC->fOpen = fOpen;
        hWnd = pIC->hWnd;
        dwConversion = pIC->fdwConversion;
        bHasChange = TRUE;
    }

    ImmUnlockIMC(hIMC);

    if (bHasChange)
    {
        Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, 0,
                          IMC_SETOPENSTATUS, IMN_SETOPENSTATUS, 0);
        NtUserNotifyIMEStatus(hWnd, hIMC, dwConversion);
    }

    return TRUE;
}

/***********************************************************************
 *		ImmSetStatusWindowPos (IMM32.@)
 */
BOOL WINAPI ImmSetStatusWindowPos(HIMC hIMC, LPPOINT lpptPos)
{
    LPINPUTCONTEXT pIC;
    HWND hWnd;

    TRACE("(%p, {%ld, %ld})\n", hIMC, lpptPos->x, lpptPos->y);

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return FALSE;

    hWnd = pIC->hWnd;
    pIC->ptStatusWndPos = *lpptPos;
    pIC->fdwInit |= INIT_STATUSWNDPOS;

    ImmUnlockIMC(hIMC);

    Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, 0,
                      IMC_SETSTATUSWINDOWPOS, IMN_SETSTATUSWINDOWPOS, 0);
    return TRUE;
}

/***********************************************************************
 *              ImmCreateSoftKeyboard(IMM32.@)
 */
HWND WINAPI ImmCreateSoftKeyboard(UINT uType, UINT hOwner, int x, int y)
{
    FIXME("(%d, %d, %d, %d): stub\n", uType, hOwner, x, y);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/***********************************************************************
 *              ImmDestroySoftKeyboard(IMM32.@)
 */
BOOL WINAPI ImmDestroySoftKeyboard(HWND hSoftWnd)
{
    TRACE("(%p)\n", hSoftWnd);
    return DestroyWindow(hSoftWnd);
}

/***********************************************************************
 *              ImmShowSoftKeyboard(IMM32.@)
 */
BOOL WINAPI ImmShowSoftKeyboard(HWND hSoftWnd, int nCmdShow)
{
    TRACE("(%p, %d)\n", hSoftWnd, nCmdShow);
    if (hSoftWnd)
        return ShowWindow(hSoftWnd, nCmdShow);
    return FALSE;
}

/***********************************************************************
 *		ImmSimulateHotKey (IMM32.@)
 */
BOOL WINAPI ImmSimulateHotKey(HWND hWnd, DWORD dwHotKeyID)
{
    HIMC hIMC;
    DWORD dwThreadId;
    HKL hKL;
    BOOL ret;

    TRACE("(%p, 0x%lX)\n", hWnd, dwHotKeyID);

    hIMC = ImmGetContext(hWnd);
    dwThreadId = GetWindowThreadProcessId(hWnd, NULL);
    hKL = GetKeyboardLayout(dwThreadId);
    ret = Imm32ProcessHotKey(hWnd, hIMC, hKL, dwHotKeyID);
    ImmReleaseContext(hWnd, hIMC);
    return ret;
}

/***********************************************************************
 *		ImmUnregisterWordA (IMM32.@)
 */
BOOL WINAPI ImmUnregisterWordA(
  HKL hKL, LPCSTR lpszReading, DWORD dwStyle, LPCSTR lpszUnregister)
{
    BOOL ret = FALSE;
    PIMEDPI pImeDpi;
    LPWSTR pszReadingW = NULL, pszUnregisterW = NULL;

    TRACE("(%p, %s, 0x%lX, %s)\n", hKL, debugstr_a(lpszReading), dwStyle,
          debugstr_a(lpszUnregister));

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (pImeDpi == NULL)
        return FALSE;

    if (!(pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE))
    {
        ret = pImeDpi->ImeUnregisterWord(lpszReading, dwStyle, lpszUnregister);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (lpszReading)
    {
        pszReadingW = Imm32WideFromAnsi(lpszReading);
        if (pszReadingW == NULL)
            goto Quit;
    }

    if (lpszUnregister)
    {
        pszUnregisterW = Imm32WideFromAnsi(lpszUnregister);
        if (pszUnregisterW == NULL)
            goto Quit;
    }

    ret = pImeDpi->ImeUnregisterWord(pszReadingW, dwStyle, pszUnregisterW);

Quit:
    if (pszReadingW)
        HeapFree(g_hImm32Heap, 0, pszReadingW);
    if (pszUnregisterW)
        HeapFree(g_hImm32Heap, 0, pszUnregisterW);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmUnregisterWordW (IMM32.@)
 */
BOOL WINAPI ImmUnregisterWordW(
  HKL hKL, LPCWSTR lpszReading, DWORD dwStyle, LPCWSTR lpszUnregister)
{
    BOOL ret = FALSE;
    PIMEDPI pImeDpi;
    LPSTR pszReadingA = NULL, pszUnregisterA = NULL;

    TRACE("(%p, %s, 0x%lX, %s)\n", hKL, debugstr_w(lpszReading), dwStyle,
          debugstr_w(lpszUnregister));

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return FALSE;

    if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE)
    {
        ret = pImeDpi->ImeUnregisterWord(lpszReading, dwStyle, lpszUnregister);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (lpszReading)
    {
        pszReadingA = Imm32AnsiFromWide(lpszReading);
        if (!pszReadingA)
            goto Quit;
    }

    if (lpszUnregister)
    {
        pszUnregisterA = Imm32AnsiFromWide(lpszUnregister);
        if (!pszUnregisterA)
            goto Quit;
    }

    ret = pImeDpi->ImeUnregisterWord(pszReadingA, dwStyle, pszUnregisterA);

Quit:
    if (pszReadingA)
        HeapFree(g_hImm32Heap, 0, pszReadingA);
    if (pszUnregisterA)
        HeapFree(g_hImm32Heap, 0, pszUnregisterA);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmGetImeMenuItemsA (IMM32.@)
 */
DWORD WINAPI ImmGetImeMenuItemsA( HIMC hIMC, DWORD dwFlags, DWORD dwType,
   LPIMEMENUITEMINFOA lpImeParentMenu, LPIMEMENUITEMINFOA lpImeMenu,
    DWORD dwSize)
{
    InputContextData *data = get_imc_data(hIMC);
    TRACE("(%p, %i, %i, %p, %p, %i):\n", hIMC, dwFlags, dwType,
        lpImeParentMenu, lpImeMenu, dwSize);

    if (!data)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    if (data->immKbd->hIME && data->immKbd->pImeGetImeMenuItems)
    {
        if (!is_himc_ime_unicode(data) || (!lpImeParentMenu && !lpImeMenu))
            return data->immKbd->pImeGetImeMenuItems(hIMC, dwFlags, dwType,
                                (IMEMENUITEMINFOW*)lpImeParentMenu,
                                (IMEMENUITEMINFOW*)lpImeMenu, dwSize);
        else
        {
            IMEMENUITEMINFOW lpImeParentMenuW;
            IMEMENUITEMINFOW *lpImeMenuW, *parent = NULL;
            DWORD rc;

            if (lpImeParentMenu)
                parent = &lpImeParentMenuW;
            if (lpImeMenu)
            {
                int count = dwSize / sizeof(LPIMEMENUITEMINFOA);
                dwSize = count * sizeof(IMEMENUITEMINFOW);
                lpImeMenuW = HeapAlloc(GetProcessHeap(), 0, dwSize);
            }
            else
                lpImeMenuW = NULL;

            rc = data->immKbd->pImeGetImeMenuItems(hIMC, dwFlags, dwType,
                                parent, lpImeMenuW, dwSize);

            if (lpImeParentMenu)
            {
                memcpy(lpImeParentMenu,&lpImeParentMenuW,sizeof(IMEMENUITEMINFOA));
                lpImeParentMenu->hbmpItem = lpImeParentMenuW.hbmpItem;
                WideCharToMultiByte(CP_ACP, 0, lpImeParentMenuW.szString,
                    -1, lpImeParentMenu->szString, IMEMENUITEM_STRING_SIZE,
                    NULL, NULL);
            }
            if (lpImeMenu && rc)
            {
                unsigned int i;
                for (i = 0; i < rc; i++)
                {
                    memcpy(&lpImeMenu[i],&lpImeMenuW[1],sizeof(IMEMENUITEMINFOA));
                    lpImeMenu[i].hbmpItem = lpImeMenuW[i].hbmpItem;
                    WideCharToMultiByte(CP_ACP, 0, lpImeMenuW[i].szString,
                        -1, lpImeMenu[i].szString, IMEMENUITEM_STRING_SIZE,
                        NULL, NULL);
                }
            }
            HeapFree(GetProcessHeap(),0,lpImeMenuW);
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
*		ImmGetImeMenuItemsW (IMM32.@)
*/
DWORD WINAPI ImmGetImeMenuItemsW( HIMC hIMC, DWORD dwFlags, DWORD dwType,
   LPIMEMENUITEMINFOW lpImeParentMenu, LPIMEMENUITEMINFOW lpImeMenu,
   DWORD dwSize)
{
    InputContextData *data = get_imc_data(hIMC);
    TRACE("(%p, %i, %i, %p, %p, %i):\n", hIMC, dwFlags, dwType,
        lpImeParentMenu, lpImeMenu, dwSize);

    if (!data)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    if (data->immKbd->hIME && data->immKbd->pImeGetImeMenuItems)
    {
        if (is_himc_ime_unicode(data) || (!lpImeParentMenu && !lpImeMenu))
            return data->immKbd->pImeGetImeMenuItems(hIMC, dwFlags, dwType,
                                lpImeParentMenu, lpImeMenu, dwSize);
        else
        {
            IMEMENUITEMINFOA lpImeParentMenuA;
            IMEMENUITEMINFOA *lpImeMenuA, *parent = NULL;
            DWORD rc;

            if (lpImeParentMenu)
                parent = &lpImeParentMenuA;
            if (lpImeMenu)
            {
                int count = dwSize / sizeof(LPIMEMENUITEMINFOW);
                dwSize = count * sizeof(IMEMENUITEMINFOA);
                lpImeMenuA = HeapAlloc(GetProcessHeap(), 0, dwSize);
            }
            else
                lpImeMenuA = NULL;

            rc = data->immKbd->pImeGetImeMenuItems(hIMC, dwFlags, dwType,
                                (IMEMENUITEMINFOW*)parent,
                                (IMEMENUITEMINFOW*)lpImeMenuA, dwSize);

            if (lpImeParentMenu)
            {
                memcpy(lpImeParentMenu,&lpImeParentMenuA,sizeof(IMEMENUITEMINFOA));
                lpImeParentMenu->hbmpItem = lpImeParentMenuA.hbmpItem;
                MultiByteToWideChar(CP_ACP, 0, lpImeParentMenuA.szString,
                    -1, lpImeParentMenu->szString, IMEMENUITEM_STRING_SIZE);
            }
            if (lpImeMenu && rc)
            {
                unsigned int i;
                for (i = 0; i < rc; i++)
                {
                    memcpy(&lpImeMenu[i],&lpImeMenuA[1],sizeof(IMEMENUITEMINFOA));
                    lpImeMenu[i].hbmpItem = lpImeMenuA[i].hbmpItem;
                    MultiByteToWideChar(CP_ACP, 0, lpImeMenuA[i].szString,
                        -1, lpImeMenu[i].szString, IMEMENUITEM_STRING_SIZE);
                }
            }
            HeapFree(GetProcessHeap(),0,lpImeMenuA);
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
*		ImmLockIMC(IMM32.@)
*/
LPINPUTCONTEXT WINAPI ImmLockIMC(HIMC hIMC)
{
    InputContextData *data = get_imc_data(hIMC);

    if (!data)
        return NULL;
    data->dwLock++;
    return &data->IMC;
}

/***********************************************************************
*		ImmUnlockIMC(IMM32.@)
*/
BOOL WINAPI ImmUnlockIMC(HIMC hIMC)
{
    PCLIENTIMC pClientImc;
    HIMC hClientImc;

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return FALSE;

    hClientImc = pClientImc->hImc;
    if (hClientImc)
        LocalUnlock(hClientImc);

    InterlockedDecrement(&pClientImc->cLockObj);
    ImmUnlockClientImc(pClientImc);
    return TRUE;
}

/***********************************************************************
*		ImmGetIMCLockCount(IMM32.@)
*/
DWORD WINAPI ImmGetIMCLockCount(HIMC hIMC)
{
    DWORD ret;
    HIMC hClientImc;
    PCLIENTIMC pClientImc;

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return 0;

    ret = 0;
    hClientImc = pClientImc->hImc;
    if (hClientImc)
        ret = (LocalFlags(hClientImc) & LMEM_LOCKCOUNT);

    ImmUnlockClientImc(pClientImc);
    return ret;
}

/***********************************************************************
*		ImmCreateIMCC(IMM32.@)
*/
HIMCC  WINAPI ImmCreateIMCC(DWORD size)
{
    if (size < 4)
        size = 4;
    return LocalAlloc(LHND, size);
}

/***********************************************************************
*       ImmDestroyIMCC(IMM32.@)
*/
HIMCC WINAPI ImmDestroyIMCC(HIMCC block)
{
    if (block)
        return LocalFree(block);
    return NULL;
}

/***********************************************************************
*		ImmLockIMCC(IMM32.@)
*/
LPVOID WINAPI ImmLockIMCC(HIMCC imcc)
{
    if (imcc)
        return LocalLock(imcc);
    return NULL;
}

/***********************************************************************
*		ImmUnlockIMCC(IMM32.@)
*/
BOOL WINAPI ImmUnlockIMCC(HIMCC imcc)
{
    if (imcc)
        return LocalUnlock(imcc);
    return FALSE;
}

/***********************************************************************
*		ImmGetIMCCLockCount(IMM32.@)
*/
DWORD WINAPI ImmGetIMCCLockCount(HIMCC imcc)
{
    return LocalFlags(imcc) & LMEM_LOCKCOUNT;
}

/***********************************************************************
*		ImmReSizeIMCC(IMM32.@)
*/
HIMCC  WINAPI ImmReSizeIMCC(HIMCC imcc, DWORD size)
{
    if (!imcc)
        return NULL;
    return LocalReAlloc(imcc, size, LHND);
}

/***********************************************************************
*		ImmGetIMCCSize(IMM32.@)
*/
DWORD WINAPI ImmGetIMCCSize(HIMCC imcc)
{
    if (imcc)
        return LocalSize(imcc);
    return 0;
}

/***********************************************************************
*		ImmGenerateMessage(IMM32.@)
*/
BOOL WINAPI ImmGenerateMessage(HIMC hIMC)
{
    PCLIENTIMC pClientImc;
    LPINPUTCONTEXT pIC;
    LPTRANSMSG pMsgs, pTrans = NULL, pItem;
    HWND hWnd;
    DWORD dwIndex, dwCount, cbTrans;
    HIMCC hMsgBuf = NULL;
    LANGID LangID;
    WORD wLang;
    BOOL bAnsi;

    TRACE("(%p)\n", hIMC);

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return FALSE;

    bAnsi = !(pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    dwCount = pIC->dwNumMsgBuf;
    if (dwCount == 0)
        goto Quit;

    hMsgBuf = pIC->hMsgBuf;
    pMsgs = ImmLockIMCC(hMsgBuf);
    if (pMsgs == NULL)
        goto Quit;

    cbTrans = dwCount * sizeof(TRANSMSG);
    pTrans = Imm32HeapAlloc(0, cbTrans);
    if (pTrans == NULL)
        goto Quit;

    RtlCopyMemory(pTrans, pMsgs, cbTrans);

    if (GetWin32ClientInfo()->dwExpWinVer < 0x400) /* old version? */
    {
        LangID = LANGIDFROMLCID(GetSystemDefaultLCID());
        wLang = PRIMARYLANGID(LangID);

        /* translate the messages if Japanese or Korean */
        if (wLang == LANG_JAPANESE ||
            (wLang == LANG_KOREAN && NtUserGetAppImeLevel(pIC->hWnd) == 3))
        {
            dwCount = Imm32Trans(dwCount, pTrans, hIMC, bAnsi, wLang);
        }
    }

    /* send them */
    hWnd = pIC->hWnd;
    pItem = pTrans;
    for (dwIndex = 0; dwIndex < dwCount; ++dwIndex, ++pItem)
    {
        if (bAnsi)
            SendMessageA(hWnd, pItem->message, pItem->wParam, pItem->lParam);
        else
            SendMessageW(hWnd, pItem->message, pItem->wParam, pItem->lParam);
    }

Quit:
    if (pTrans)
        HeapFree(g_hImm32Heap, 0, pTrans);
    if (hMsgBuf)
        ImmUnlockIMCC(hMsgBuf);
    pIC->dwNumMsgBuf = 0; /* done */
    ImmUnlockIMC(hIMC);
    return TRUE;
}

static VOID APIENTRY
Imm32PostMessages(HWND hwnd, HIMC hIMC, DWORD dwCount, LPTRANSMSG lpTransMsg)
{
    DWORD dwIndex, cbTransMsg;
    PCLIENTIMC pClientImc;
    LPTRANSMSG pNewTransMsg = lpTransMsg, pItem;
    BOOL bAnsi;
    LANGID LangID;
    WORD Lang;

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return;

    bAnsi = !(pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    if (GetWin32ClientInfo()->dwExpWinVer < 0x400) /* old version? */
    {
        LangID = LANGIDFROMLCID(GetSystemDefaultLCID());
        Lang = PRIMARYLANGID(LangID);

        /* translate the messages if Japanese or Korean */
        if (Lang == LANG_JAPANESE ||
            (Lang == LANG_KOREAN && NtUserGetAppImeLevel(hwnd) == 3))
        {
            cbTransMsg = dwCount * sizeof(TRANSMSG);
            pNewTransMsg = Imm32HeapAlloc(0, cbTransMsg);
            if (pNewTransMsg)
            {
                RtlCopyMemory(pNewTransMsg, lpTransMsg, cbTransMsg);
                dwCount = Imm32Trans(dwCount, pNewTransMsg, hIMC, bAnsi, Lang);
            }
            else
            {
                pNewTransMsg = lpTransMsg;
            }
        }
    }

    /* post them */
    pItem = pNewTransMsg;
    for (dwIndex = 0; dwIndex < dwCount; ++dwIndex, ++pItem)
    {
        if (bAnsi)
            PostMessageA(hwnd, pItem->message, pItem->wParam, pItem->lParam);
        else
            PostMessageW(hwnd, pItem->message, pItem->wParam, pItem->lParam);
    }

    if (pNewTransMsg && pNewTransMsg != lpTransMsg)
        HeapFree(g_hImm32Heap, 0, pNewTransMsg);
}

/***********************************************************************
*       ImmTranslateMessage(IMM32.@)
*       ( Undocumented, call internally and from user32.dll )
*/
BOOL WINAPI ImmTranslateMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lKeyData)
{
#define MSG_COUNT 0x100
    BOOL ret = FALSE;
    INT kret;
    LPINPUTCONTEXTDX pIC;
    PIMEDPI pImeDpi = NULL;
    LPTRANSMSGLIST pList = NULL;
    LPTRANSMSG pTransMsg;
    BYTE abKeyState[256];
    HIMC hIMC;
    HKL hKL;
    UINT vk;
    DWORD dwThreadId, dwCount, cbList;
    WCHAR wch;
    WORD wChar;

    TRACE("(%p, 0x%X, %p, %p)\n", hwnd, msg, wParam, lKeyData);

    /* filter the message */
    switch (msg)
    {
        case WM_KEYDOWN: case WM_KEYUP: case WM_SYSKEYDOWN: case WM_SYSKEYUP:
            break;
        default:
            return FALSE;
    }

    hIMC = ImmGetContext(hwnd);
    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (pIC == NULL)
    {
        ImmReleaseContext(hwnd, hIMC);
        return FALSE;
    }

    if (!pIC->bNeedsTrans) /* is translation needed? */
    {
        /* directly post them */
        dwCount = pIC->dwNumMsgBuf;
        if (dwCount == 0)
            goto Quit;

        pTransMsg = ImmLockIMCC(pIC->hMsgBuf);
        if (pTransMsg)
        {
            Imm32PostMessages(hwnd, hIMC, dwCount, pTransMsg);
            ImmUnlockIMCC(pIC->hMsgBuf);
            ret = TRUE;
        }
        pIC->dwNumMsgBuf = 0; /* done */
        goto Quit;
    }
    pIC->bNeedsTrans = FALSE; /* clear the flag */

    dwThreadId = GetWindowThreadProcessId(hwnd, NULL);
    hKL = GetKeyboardLayout(dwThreadId);
    pImeDpi = ImmLockImeDpi(hKL);
    if (pImeDpi == NULL)
        goto Quit;

    if (!GetKeyboardState(abKeyState)) /* get keyboard ON/OFF status */
        goto Quit;

    /* convert a virtual key if IME_PROP_KBD_CHAR_FIRST */
    vk = pIC->nVKey;
    if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_KBD_CHAR_FIRST)
    {
        if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE)
        {
            wch = 0;
            kret = ToUnicode(vk, HIWORD(lKeyData), abKeyState, &wch, 1, 0);
            if (kret == 1)
                vk = MAKELONG(LOBYTE(vk), wch);
        }
        else
        {
            wChar = 0;
            kret = ToAsciiEx(vk, HIWORD(lKeyData), abKeyState, &wChar, 0, hKL);
            if (kret > 0)
                vk = MAKEWORD(vk, wChar);
        }
    }

    /* allocate a list */
    cbList = offsetof(TRANSMSGLIST, TransMsg) + MSG_COUNT * sizeof(TRANSMSG);
    pList = Imm32HeapAlloc(0, cbList);
    if (!pList)
        goto Quit;

    /* use IME conversion engine and convert the list */
    pList->uMsgCount = MSG_COUNT;
    kret = pImeDpi->ImeToAsciiEx(vk, HIWORD(lKeyData), abKeyState, pList, 0, hIMC);
    if (kret <= 0)
        goto Quit;

    /* post them */
    if (kret <= MSG_COUNT)
    {
        Imm32PostMessages(hwnd, hIMC, kret, pList->TransMsg);
        ret = TRUE;
    }
    else
    {
        pTransMsg = ImmLockIMCC(pIC->hMsgBuf);
        if (pTransMsg == NULL)
            goto Quit;
        Imm32PostMessages(hwnd, hIMC, kret, pTransMsg);
        ImmUnlockIMCC(pIC->hMsgBuf);
    }

Quit:
    if (pList)
        HeapFree(g_hImm32Heap, 0, pList);
    ImmUnlockImeDpi(pImeDpi);
    ImmUnlockIMC(hIMC);
    ImmReleaseContext(hwnd, hIMC);
    return ret;
#undef MSG_COUNT
}

/***********************************************************************
*		ImmProcessKey(IMM32.@)
*       ( Undocumented, called from user32.dll )
*/
BOOL WINAPI ImmProcessKey(HWND hwnd, HKL hKL, UINT vKey, LPARAM lKeyData, DWORD unknown)
{
    InputContextData *data;
    HIMC imc = ImmGetContext(hwnd);
    BYTE state[256];

    TRACE("%p %p %x %x %x\n",hwnd, hKL, vKey, (UINT)lKeyData, unknown);

    if (imc)
        data = (InputContextData *)imc;
    else
        return FALSE;

    /* Make sure we are inputting to the correct keyboard */
    if (data->immKbd->hkl != hKL)
    {
        ImmHkl *new_hkl = IMM_GetImmHkl(hKL);
        if (new_hkl)
        {
            data->immKbd->pImeSelect(imc, FALSE);
            data->immKbd->uSelected--;
            data->immKbd = new_hkl;
            data->immKbd->pImeSelect(imc, TRUE);
            data->immKbd->uSelected++;
        }
        else
            return FALSE;
    }

    if (!data->immKbd->hIME || !data->immKbd->pImeProcessKey)
        return FALSE;

    GetKeyboardState(state);
    if (data->immKbd->pImeProcessKey(imc, vKey, lKeyData, state))
    {
        data->lastVK = vKey;
        return TRUE;
    }

    data->lastVK = VK_PROCESSKEY;
    return FALSE;
}

/***********************************************************************
*		ImmDisableTextFrameService(IMM32.@)
*/
BOOL WINAPI ImmDisableTextFrameService(DWORD dwThreadId)
{
    FIXME("Stub\n");
    return FALSE;
}

/***********************************************************************
 *              ImmEnumInputContext(IMM32.@)
 */
BOOL WINAPI ImmEnumInputContext(DWORD dwThreadId, IMCENUMPROC lpfn, LPARAM lParam)
{
    HIMC *phList;
    DWORD dwIndex, dwCount;
    BOOL ret = TRUE;
    HIMC hIMC;

    TRACE("(%lu, %p, %p)\n", dwThreadId, lpfn, lParam);

    dwCount = Imm32AllocAndBuildHimcList(dwThreadId, &phList);
    if (!dwCount)
        return FALSE;

    for (dwIndex = 0; dwIndex < dwCount; ++dwIndex)
    {
        hIMC = phList[dwIndex];
        ret = (*lpfn)(hIMC, lParam);
        if (!ret)
            break;
    }

    HeapFree(g_hImm32Heap, 0, phList);
    return ret;
}

/***********************************************************************
 *              ImmGetHotKey(IMM32.@)
 */
BOOL WINAPI
ImmGetHotKey(IN DWORD dwHotKey,
             OUT LPUINT lpuModifiers,
             OUT LPUINT lpuVKey,
             OUT LPHKL lphKL)
{
    TRACE("%lx, %p, %p, %p\n", dwHotKey, lpuModifiers, lpuVKey, lphKL);
    if (lpuModifiers && lpuVKey)
        return NtUserGetImeHotKey(dwHotKey, lpuModifiers, lpuVKey, lphKL);
    return FALSE;
}

/***********************************************************************
 *              ImmDisableLegacyIME(IMM32.@)
 */
BOOL WINAPI ImmDisableLegacyIME(void)
{
    FIXME("stub\n");
    return TRUE;
}

/***********************************************************************
 *              ImmSetActiveContext(IMM32.@)
 */
BOOL WINAPI ImmSetActiveContext(HWND hwnd, HIMC hIMC, BOOL fFlag)
{
    FIXME("(%p, %p, %d): stub\n", hwnd, hIMC, fFlag);
    return FALSE;
}

/***********************************************************************
 *              ImmSetActiveContextConsoleIME(IMM32.@)
 */
BOOL WINAPI ImmSetActiveContextConsoleIME(HWND hwnd, BOOL fFlag)
{
    HIMC hIMC;
    TRACE("(%p, %d)\n", hwnd, fFlag);

    hIMC = ImmGetContext(hwnd);
    if (hIMC)
        return ImmSetActiveContext(hwnd, hIMC, fFlag);
    return FALSE;
}

/***********************************************************************
*		ImmRegisterClient(IMM32.@)
*       ( Undocumented, called from user32.dll )
*/
BOOL WINAPI ImmRegisterClient(PSHAREDINFO ptr, HINSTANCE hMod)
{
    g_SharedInfo = *ptr;
    g_psi = g_SharedInfo.psi;
    return Imm32InitInstance(hMod);
}

/***********************************************************************
 *		CtfImmIsTextFrameServiceDisabled(IMM32.@)
 */
BOOL WINAPI CtfImmIsTextFrameServiceDisabled(VOID)
{
    PTEB pTeb = NtCurrentTeb();
    if (((PW32CLIENTINFO)pTeb->Win32ClientInfo)->CI_flags & CI_TFSDISABLED)
        return TRUE;
    return FALSE;
}

/***********************************************************************
 *              ImmGetImeInfoEx (IMM32.@)
 */
BOOL WINAPI
ImmGetImeInfoEx(PIMEINFOEX pImeInfoEx,
                IMEINFOEXCLASS SearchType,
                PVOID pvSearchKey)
{
    BOOL bDisabled = FALSE;
    HKL hKL;
    PTEB pTeb;

    switch (SearchType)
    {
        case ImeInfoExKeyboardLayout:
            break;

        case ImeInfoExImeWindow:
            bDisabled = CtfImmIsTextFrameServiceDisabled();
            SearchType = ImeInfoExKeyboardLayout;
            break;

        case ImeInfoExImeFileName:
            StringCchCopyW(pImeInfoEx->wszImeFile, _countof(pImeInfoEx->wszImeFile),
                           pvSearchKey);
            goto Quit;
    }

    hKL = *(HKL*)pvSearchKey;
    pImeInfoEx->hkl = hKL;

    if (!IS_IME_HKL(hKL))
    {
        if (g_psi && (g_psi->dwSRVIFlags & SRVINFO_CICERO_ENABLED))
        {
            pTeb = NtCurrentTeb();
            if (((PW32CLIENTINFO)pTeb->Win32ClientInfo)->W32ClientInfo[0] & 2)
                return FALSE;
            if (!bDisabled)
                goto Quit;
        }
        return FALSE;
    }

Quit:
    return NtUserGetImeInfoEx(pImeInfoEx, SearchType);
}

BOOL WINAPI User32InitializeImmEntryTable(DWORD);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    HKL hKL;
    HIMC hIMC;
    PTEB pTeb;

    TRACE("(%p, 0x%X, %p)\n", hinstDLL, fdwReason, lpReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            //Imm32GenerateRandomSeed(hinstDLL, 1, lpReserved); // Non-sense
            if (!Imm32InitInstance(hinstDLL))
            {
                ERR("Imm32InitInstance failed\n");
                return FALSE;
            }
            if (!User32InitializeImmEntryTable(IMM_INIT_MAGIC))
            {
                ERR("User32InitializeImmEntryTable failed\n");
                return FALSE;
            }
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            if (g_psi == NULL || !(g_psi->dwSRVIFlags & SRVINFO_IMM32))
                return TRUE;

            pTeb = NtCurrentTeb();
            if (pTeb->Win32ThreadInfo == NULL)
                return TRUE;

            hKL = GetKeyboardLayout(0);
            // FIXME: NtUserGetThreadState and enum ThreadStateRoutines are broken.
            hIMC = (HIMC)NtUserGetThreadState(4);
            Imm32CleanupContext(hIMC, hKL, TRUE);
            break;

        case DLL_PROCESS_DETACH:
            RtlDeleteCriticalSection(&g_csImeDpi);
            break;
    }

    return TRUE;
}
