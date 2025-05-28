/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IMM32 helper functions
 * COPYRIGHT:   Copyright 1998 Patrik Stridvall
 *              Copyright 2002, 2003, 2007 CodeWeavers, Aric Stewart
 *              Copyright 2017 James Tabor <james.tabor@reactos.org>
 *              Copyright 2018 Amine Khaldi <amine.khaldi@reactos.org>
 *              Copyright 2020-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

HANDLE ghImmHeap = NULL; // Win: pImmHeap

PTHREADINFO FASTCALL Imm32CurrentPti(VOID)
{
    if (NtCurrentTeb()->Win32ThreadInfo == NULL)
        NtUserGetThreadState(THREADSTATE_GETTHREADINFO);
    return NtCurrentTeb()->Win32ThreadInfo;
}

BOOL APIENTRY Imm32IsCrossThreadAccess(HIMC hIMC)
{
    DWORD_PTR dwImeThreadId = NtUserQueryInputContext(hIMC, QIC_INPUTTHREADID);
    DWORD_PTR dwCurrentThreadId = GetCurrentThreadId();
    return dwImeThreadId != dwCurrentThreadId;
}

// Win: TestWindowProcess
BOOL APIENTRY Imm32IsCrossProcessAccess(HWND hWnd)
{
    DWORD_PTR WndPID = NtUserQueryWindow(hWnd, QUERY_WINDOW_UNIQUE_PROCESS_ID);
    DWORD_PTR CurrentPID = (DWORD_PTR)NtCurrentTeb()->ClientId.UniqueProcess;
    return WndPID != CurrentPID;
}

HRESULT
Imm32StrToUInt(
    _In_ PCWSTR pszText,
    _Out_ PDWORD pdwValue,
    _In_ ULONG nBase)
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    *pdwValue = 0;
    RtlInitUnicodeString(&UnicodeString, pszText);
    Status = RtlUnicodeStringToInteger(&UnicodeString, nBase, pdwValue);
    if (!NT_SUCCESS(Status))
        return E_FAIL;
    return S_OK;
}

HRESULT
Imm32UIntToStr(
    _In_ DWORD dwValue,
    _In_ ULONG nBase,
    _Out_ PWSTR pszBuff,
    _In_ USHORT cchBuff)
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    UnicodeString.Buffer = pszBuff;
    UnicodeString.MaximumLength = cchBuff * sizeof(WCHAR);
    Status = RtlIntegerToUnicodeString(dwValue, nBase, &UnicodeString);
    if (!NT_SUCCESS(Status))
        return E_FAIL;
    return S_OK;
}

BOOL Imm32IsSystemJapaneseOrKorean(VOID)
{
    LCID lcid = GetSystemDefaultLCID();
    LANGID LangID = LANGIDFROMLCID(lcid);
    WORD wPrimary = PRIMARYLANGID(LangID);
    if (wPrimary != LANG_JAPANESE || wPrimary != LANG_KOREAN)
    {
        TRACE("The country has no special IME support\n");
        return FALSE;
    }
    return TRUE;
}

BOOL Imm32IsImcAnsi(HIMC hIMC)
{
    BOOL ret;
    PCLIENTIMC pClientImc = ImmLockClientImc(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pClientImc))
        return -1;
    ret = !(pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);
    return ret;
}

LPWSTR APIENTRY Imm32WideFromAnsi(UINT uCodePage, LPCSTR pszA)
{
    INT cch = lstrlenA(pszA);
    LPWSTR pszW = ImmLocalAlloc(0, (cch + 1) * sizeof(WCHAR));
    if (IS_NULL_UNEXPECTEDLY(pszW))
        return NULL;
    cch = MultiByteToWideChar(uCodePage, MB_PRECOMPOSED, pszA, cch, pszW, cch + 1);
    pszW[cch] = 0;
    return pszW;
}

LPSTR APIENTRY Imm32AnsiFromWide(UINT uCodePage, LPCWSTR pszW)
{
    INT cchW = lstrlenW(pszW);
    INT cchA = (cchW + 1) * sizeof(WCHAR);
    LPSTR pszA = ImmLocalAlloc(0, cchA);
    if (IS_NULL_UNEXPECTEDLY(pszA))
        return NULL;
    cchA = WideCharToMultiByte(uCodePage, 0, pszW, cchW, pszA, cchA, NULL, NULL);
    pszA[cchA] = 0;
    return pszA;
}

/* Converts the character index */
/* Win: CalcCharacterPositionAtoW */
LONG APIENTRY IchWideFromAnsi(LONG cchAnsi, LPCSTR pchAnsi, UINT uCodePage)
{
    LONG cchWide;
    for (cchWide = 0; cchAnsi > 0; ++cchWide)
    {
        if (IsDBCSLeadByteEx(uCodePage, *pchAnsi) && pchAnsi[1])
        {
            cchAnsi -= 2;
            pchAnsi += 2;
        }
        else
        {
            --cchAnsi;
            ++pchAnsi;
        }
    }
    return cchWide;
}

/* Converts the character index */
/* Win: CalcCharacterPositionWtoA */
LONG APIENTRY IchAnsiFromWide(LONG cchWide, LPCWSTR pchWide, UINT uCodePage)
{
    LONG cb, cchAnsi;
    for (cchAnsi = 0; cchWide > 0; ++cchAnsi, ++pchWide, --cchWide)
    {
        cb = WideCharToMultiByte(uCodePage, 0, pchWide, 1, NULL, 0, NULL, NULL);
        if (cb > 1)
            ++cchAnsi;
    }
    return cchAnsi;
}

// Win: InternalGetSystemPathName
BOOL Imm32GetSystemLibraryPath(LPWSTR pszPath, DWORD cchPath, LPCWSTR pszFileName)
{
    if (!pszFileName[0] || !GetSystemDirectoryW(pszPath, cchPath))
    {
        ERR("Invalid filename\n");
        return FALSE;
    }
    StringCchCatW(pszPath, cchPath, L"\\");
    StringCchCatW(pszPath, cchPath, pszFileName);
    return TRUE;
}

// Win: LFontAtoLFontW
VOID APIENTRY LogFontAnsiToWide(const LOGFONTA *plfA, LPLOGFONTW plfW)
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

// Win: LFontWtoLFontA
VOID APIENTRY LogFontWideToAnsi(const LOGFONTW *plfW, LPLOGFONTA plfA)
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

static PVOID FASTCALL DesktopPtrToUser(PVOID ptr)
{
    PCLIENTINFO pci = GetWin32ClientInfo();
    PDESKTOPINFO pdi = pci->pDeskInfo;

    ASSERT(ptr != NULL);
    ASSERT(pdi != NULL);
    if (pdi->pvDesktopBase <= ptr && ptr < pdi->pvDesktopLimit)
        return (PVOID)((ULONG_PTR)ptr - pci->ulClientDelta);
    else
        return (PVOID)NtUserCallOneParam((DWORD_PTR)ptr, ONEPARAM_ROUTINE_GETDESKTOPMAPPING);
}

// Win: HMValidateHandleNoRip
LPVOID FASTCALL ValidateHandleNoErr(HANDLE hObject, UINT uType)
{
    UINT index;
    PUSER_HANDLE_TABLE ht;
    PUSER_HANDLE_ENTRY he;
    WORD generation;
    LPVOID ptr;

    if (!NtUserValidateHandleSecure(hObject))
    {
        WARN("Not a handle\n");
        return NULL;
    }

    ht = gSharedInfo.aheList; /* handle table */
    ASSERT(ht);
    /* ReactOS-Specific! */
    ASSERT(gSharedInfo.ulSharedDelta != 0);
    he = (PUSER_HANDLE_ENTRY)((ULONG_PTR)ht->handles - gSharedInfo.ulSharedDelta);

    index = (LOWORD(hObject) - FIRST_USER_HANDLE) >> 1;
    if ((INT)index < 0 || ht->nb_handles <= index || he[index].type != uType)
        return NULL;

    if (he[index].flags & HANDLEENTRY_DESTROY)
        return NULL;

    generation = HIWORD(hObject);
    if (generation != he[index].generation && generation && generation != 0xFFFF)
        return NULL;

    ptr = he[index].ptr;
    if (ptr)
        ptr = DesktopPtrToUser(ptr);

    return ptr;
}

// Win: HMValidateHandle
LPVOID FASTCALL ValidateHandle(HANDLE hObject, UINT uType)
{
    LPVOID pvObj = ValidateHandleNoErr(hObject, uType);
    if (pvObj)
        return pvObj;

    if (uType == TYPE_WINDOW)
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
    else
        SetLastError(ERROR_INVALID_HANDLE);
    return NULL;
}

// Win: TestInputContextProcess
BOOL APIENTRY Imm32CheckImcProcess(PIMC pIMC)
{
    HIMC hIMC;
    DWORD_PTR dwPID1, dwPID2;

    if (IS_NULL_UNEXPECTEDLY(pIMC))
        return FALSE;

    if (pIMC->head.pti == Imm32CurrentPti())
        return TRUE;

    hIMC = pIMC->head.h;
    dwPID1 = (DWORD)NtUserQueryInputContext(hIMC, QIC_INPUTPROCESSID);
    dwPID2 = (DWORD_PTR)NtCurrentTeb()->ClientId.UniqueProcess;
    if (dwPID1 != dwPID2)
    {
        WARN("PID 0x%X != 0x%X\n", dwPID1, dwPID2);
        return FALSE;
    }

    return TRUE;
}

LPVOID ImmLocalAlloc(_In_ DWORD dwFlags, _In_ DWORD dwBytes)
{
    if (!ghImmHeap)
    {
        ghImmHeap = RtlGetProcessHeap();
        if (IS_NULL_UNEXPECTEDLY(ghImmHeap))
            return NULL;
    }
    return HeapAlloc(ghImmHeap, dwFlags, dwBytes);
}

BOOL
Imm32MakeIMENotify(
    _In_ HIMC hIMC,
    _In_ HWND hwnd,
    _In_ DWORD dwAction,
    _In_ DWORD dwIndex,
    _Inout_opt_ DWORD_PTR dwValue,
    _In_ DWORD dwCommand,
    _Inout_opt_ DWORD_PTR dwData)
{
    DWORD dwThreadId;
    HKL hKL;
    PIMEDPI pImeDpi;

    if (dwAction != 0)
    {
        dwThreadId = (DWORD)NtUserQueryInputContext(hIMC, QIC_INPUTTHREADID);
        if (dwThreadId)
        {
            /* find keyboard layout and lock it */
            hKL = GetKeyboardLayout(dwThreadId);
            pImeDpi = ImmLockImeDpi(hKL);
            if (pImeDpi)
            {
                /* do notify */
                TRACE("NotifyIME(%p, %lu, %lu, %p)\n", hIMC, dwAction, dwIndex, dwValue);
                if (!pImeDpi->NotifyIME(hIMC, dwAction, dwIndex, dwValue))
                    WARN("NotifyIME(%p, %lu, %lu, %p) failed\n", hIMC, dwAction, dwIndex, dwValue);

                ImmUnlockImeDpi(pImeDpi); /* unlock */
            }
            else
            {
                WARN("pImeDpi was NULL\n");
            }
        }
        else
        {
            WARN("dwThreadId was zero\n");
        }
    }
    else
    {
        WARN("dwAction was zero\n");
    }

    if (hwnd && dwCommand)
        SendMessageW(hwnd, WM_IME_NOTIFY, dwCommand, dwData);

    return TRUE;
}

// Win: BuildHimcList
DWORD APIENTRY Imm32BuildHimcList(DWORD dwThreadId, HIMC **pphList)
{
#define INITIAL_COUNT 0x40
#define MAX_RETRY 10
    NTSTATUS Status;
    DWORD dwCount = INITIAL_COUNT, cRetry = 0;
    HIMC *phNewList;

    phNewList = ImmLocalAlloc(0, dwCount * sizeof(HIMC));
    if (IS_NULL_UNEXPECTEDLY(phNewList))
        return 0;

    Status = NtUserBuildHimcList(dwThreadId, dwCount, phNewList, &dwCount);
    while (Status == STATUS_BUFFER_TOO_SMALL)
    {
        ImmLocalFree(phNewList);
        if (cRetry++ >= MAX_RETRY)
            return 0;

        phNewList = ImmLocalAlloc(0, dwCount * sizeof(HIMC));
        if (IS_NULL_UNEXPECTEDLY(phNewList))
            return 0;

        Status = NtUserBuildHimcList(dwThreadId, dwCount, phNewList, &dwCount);
    }

    if (NT_ERROR(Status) || !dwCount)
    {
        ERR("Abnormal status\n");
        ImmLocalFree(phNewList);
        return 0;
    }

    *pphList = phNewList;
    return dwCount;
#undef INITIAL_COUNT
#undef MAX_RETRY
}

// Win: GetImeModeSaver
PIME_STATE APIENTRY
Imm32FetchImeState(LPINPUTCONTEXTDX pIC, HKL hKL)
{
    PIME_STATE pState;
    WORD Lang = PRIMARYLANGID(LOWORD(hKL));
    for (pState = pIC->pState; pState; pState = pState->pNext)
    {
        if (pState->wLang == Lang)
            break;
    }
    if (!pState)
    {
        pState = ImmLocalAlloc(HEAP_ZERO_MEMORY, sizeof(IME_STATE));
        if (pState)
        {
            pState->wLang = Lang;
            pState->pNext = pIC->pState;
            pIC->pState = pState;
        }
    }
    return pState;
}

// Win: GetImePrivateModeSaver
PIME_SUBSTATE APIENTRY
Imm32FetchImeSubState(PIME_STATE pState, HKL hKL)
{
    PIME_SUBSTATE pSubState;
    for (pSubState = pState->pSubState; pSubState; pSubState = pSubState->pNext)
    {
        if (pSubState->hKL == hKL)
            return pSubState;
    }
    pSubState = ImmLocalAlloc(0, sizeof(IME_SUBSTATE));
    if (!pSubState)
        return NULL;
    pSubState->dwValue = 0;
    pSubState->hKL = hKL;
    pSubState->pNext = pState->pSubState;
    pState->pSubState = pSubState;
    return pSubState;
}

// Win: RestorePrivateMode
BOOL APIENTRY
Imm32LoadImeStateSentence(LPINPUTCONTEXTDX pIC, PIME_STATE pState, HKL hKL)
{
    PIME_SUBSTATE pSubState = Imm32FetchImeSubState(pState, hKL);
    if (IS_NULL_UNEXPECTEDLY(pSubState))
        return FALSE;

    pIC->fdwSentence |= pSubState->dwValue;
    return TRUE;
}

// Win: SavePrivateMode
BOOL APIENTRY
Imm32SaveImeStateSentence(LPINPUTCONTEXTDX pIC, PIME_STATE pState, HKL hKL)
{
    PIME_SUBSTATE pSubState = Imm32FetchImeSubState(pState, hKL);
    if (IS_NULL_UNEXPECTEDLY(pSubState))
        return FALSE;

    pSubState->dwValue = (pIC->fdwSentence & 0xffff0000);
    return TRUE;
}

/*
 * See RECONVERTSTRING structure:
 * https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/RECONVERTSTRING.html
 *
 * The dwCompStrOffset and dwTargetOffset members are the relative position of dwStrOffset.
 * dwStrLen, dwCompStrLen, and dwTargetStrLen are the TCHAR count. dwStrOffset,
 * dwCompStrOffset, and dwTargetStrOffset are the byte offset.
 */

DWORD APIENTRY
Imm32ReconvertWideFromAnsi(LPRECONVERTSTRING pDest, const RECONVERTSTRING *pSrc, UINT uCodePage)
{
    DWORD cch0, cchDest, cbDest;
    LPCSTR pchSrc = (LPCSTR)pSrc + pSrc->dwStrOffset;
    LPWSTR pchDest;

    if (pSrc->dwVersion != 0)
    {
        ERR("\n");
        return 0;
    }

    cchDest = MultiByteToWideChar(uCodePage, MB_PRECOMPOSED, pchSrc, pSrc->dwStrLen,
                                  NULL, 0);
    cbDest = sizeof(RECONVERTSTRING) + (cchDest + 1) * sizeof(WCHAR);
    if (!pDest)
        return cbDest;

    if (pDest->dwSize < cbDest)
    {
        ERR("Too small\n");
        return 0;
    }

    /* dwSize */
    pDest->dwSize = cbDest;

    /* dwVersion */
    pDest->dwVersion = 0;

    /* dwStrOffset */
    pDest->dwStrOffset = sizeof(RECONVERTSTRING);

    /* dwCompStrOffset */
    cch0 = IchWideFromAnsi(pSrc->dwCompStrOffset, pchSrc, uCodePage);
    pDest->dwCompStrOffset = cch0 * sizeof(WCHAR);

    /* dwCompStrLen */
    cch0 = IchWideFromAnsi(pSrc->dwCompStrOffset + pSrc->dwCompStrLen, pchSrc, uCodePage);
    pDest->dwCompStrLen = (cch0 * sizeof(WCHAR) - pDest->dwCompStrOffset) / sizeof(WCHAR);

    /* dwTargetStrOffset */
    cch0 = IchWideFromAnsi(pSrc->dwTargetStrOffset, pchSrc, uCodePage);
    pDest->dwTargetStrOffset = cch0 * sizeof(WCHAR);

    /* dwTargetStrLen */
    cch0 = IchWideFromAnsi(pSrc->dwTargetStrOffset + pSrc->dwTargetStrLen, pchSrc, uCodePage);
    pDest->dwTargetStrLen = (cch0 * sizeof(WCHAR) - pSrc->dwTargetStrOffset) / sizeof(WCHAR);

    /* dwStrLen */
    pDest->dwStrLen = cchDest;

    /* the string */
    pchDest = (LPWSTR)((LPBYTE)pDest + pDest->dwStrOffset);
    cchDest = MultiByteToWideChar(uCodePage, MB_PRECOMPOSED, pchSrc, pSrc->dwStrLen,
                                  pchDest, cchDest);
    pchDest[cchDest] = 0;

    TRACE("cbDest: 0x%X\n", cbDest);
    return cbDest;
}

DWORD APIENTRY
Imm32ReconvertAnsiFromWide(LPRECONVERTSTRING pDest, const RECONVERTSTRING *pSrc, UINT uCodePage)
{
    DWORD cch0, cch1, cchDest, cbDest;
    LPCWSTR pchSrc = (LPCWSTR)((LPCSTR)pSrc + pSrc->dwStrOffset);
    LPSTR pchDest;

    if (pSrc->dwVersion != 0)
    {
        ERR("\n");
        return 0;
    }

    cchDest = WideCharToMultiByte(uCodePage, 0, pchSrc, pSrc->dwStrLen,
                                  NULL, 0, NULL, NULL);
    cbDest = sizeof(RECONVERTSTRING) + (cchDest + 1) * sizeof(CHAR);
    if (!pDest)
        return cbDest;

    if (pDest->dwSize < cbDest)
    {
        ERR("Too small\n");
        return 0;
    }

    /* dwSize */
    pDest->dwSize = cbDest;

    /* dwVersion */
    pDest->dwVersion = 0;

    /* dwStrOffset */
    pDest->dwStrOffset = sizeof(RECONVERTSTRING);

    /* dwCompStrOffset */
    cch1 = pSrc->dwCompStrOffset / sizeof(WCHAR);
    cch0 = IchAnsiFromWide(cch1, pchSrc, uCodePage);
    pDest->dwCompStrOffset = cch0 * sizeof(CHAR);

    /* dwCompStrLen */
    cch0 = IchAnsiFromWide(cch1 + pSrc->dwCompStrLen, pchSrc, uCodePage);
    pDest->dwCompStrLen = cch0 * sizeof(CHAR) - pDest->dwCompStrOffset;

    /* dwTargetStrOffset */
    cch1 = pSrc->dwTargetStrOffset / sizeof(WCHAR);
    cch0 = IchAnsiFromWide(cch1, pchSrc, uCodePage);
    pDest->dwTargetStrOffset = cch0 * sizeof(CHAR);

    /* dwTargetStrLen */
    cch0 = IchAnsiFromWide(cch1 + pSrc->dwTargetStrLen, pchSrc, uCodePage);
    pDest->dwTargetStrLen = cch0 * sizeof(CHAR) - pDest->dwTargetStrOffset;

    /* dwStrLen */
    pDest->dwStrLen = cchDest;

    /* the string */
    pchDest = (LPSTR)pDest + pDest->dwStrOffset;
    cchDest = WideCharToMultiByte(uCodePage, 0, pchSrc, pSrc->dwStrLen,
                                  pchDest, cchDest, NULL, NULL);
    pchDest[cchDest] = 0;

    TRACE("cchDest: 0x%X\n", cchDest);
    return cbDest;
}

/***********************************************************************
 *		ImmCreateIMCC(IMM32.@)
 */
HIMCC WINAPI ImmCreateIMCC(DWORD size)
{
    if (size < sizeof(DWORD))
        size = sizeof(DWORD);
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
HIMCC WINAPI ImmReSizeIMCC(HIMCC imcc, DWORD size)
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
 *		ImmGetIMCLockCount(IMM32.@)
 */
DWORD WINAPI ImmGetIMCLockCount(HIMC hIMC)
{
    DWORD ret;
    HANDLE hInputContext;
    PCLIENTIMC pClientImc;

    pClientImc = ImmLockClientImc(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pClientImc))
        return 0;

    ret = 0;
    hInputContext = pClientImc->hInputContext;
    if (hInputContext)
        ret = (LocalFlags(hInputContext) & LMEM_LOCKCOUNT);

    ImmUnlockClientImc(pClientImc);
    return ret;
}
