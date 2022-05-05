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

// Win: StrToUInt
HRESULT APIENTRY
Imm32StrToUInt(LPCWSTR pszText, LPDWORD pdwValue, ULONG nBase)
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    RtlInitUnicodeString(&UnicodeString, pszText);
    Status = RtlUnicodeStringToInteger(&UnicodeString, nBase, pdwValue);
    if (!NT_SUCCESS(Status))
        return E_FAIL;
    return S_OK;
}

// Win: UIntToStr
HRESULT APIENTRY
Imm32UIntToStr(DWORD dwValue, ULONG nBase, LPWSTR pszBuff, USHORT cchBuff)
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

BOOL APIENTRY Imm32IsSystemJapaneseOrKorean(VOID)
{
    LCID lcid = GetSystemDefaultLCID();
    LANGID LangID = LANGIDFROMLCID(lcid);
    WORD wPrimary = PRIMARYLANGID(LangID);
    return (wPrimary == LANG_JAPANESE || wPrimary == LANG_KOREAN);
}

// Win: IsAnsiIMC
BOOL WINAPI Imm32IsImcAnsi(HIMC hIMC)
{
    BOOL ret;
    PCLIENTIMC pClientImc = ImmLockClientImc(hIMC);
    if (!pClientImc)
        return -1;
    ret = !(pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);
    return ret;
}

LPWSTR APIENTRY Imm32WideFromAnsi(LPCSTR pszA)
{
    INT cch = lstrlenA(pszA);
    LPWSTR pszW = ImmLocalAlloc(0, (cch + 1) * sizeof(WCHAR));
    if (pszW == NULL)
        return NULL;
    cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszA, cch, pszW, cch + 1);
    pszW[cch] = 0;
    return pszW;
}

LPSTR APIENTRY Imm32AnsiFromWide(LPCWSTR pszW)
{
    INT cchW = lstrlenW(pszW);
    INT cchA = (cchW + 1) * sizeof(WCHAR);
    LPSTR pszA = ImmLocalAlloc(0, cchA);
    if (!pszA)
        return NULL;
    cchA = WideCharToMultiByte(CP_ACP, 0, pszW, cchW, pszA, cchA, NULL, NULL);
    pszA[cchA] = 0;
    return pszA;
}

/* Converts the character index */
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
        return FALSE;
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
        return NULL;

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
    DWORD dwProcessID;
    if (pIMC->head.pti == Imm32CurrentPti())
        return TRUE;

    hIMC = pIMC->head.h;
    dwProcessID = (DWORD)NtUserQueryInputContext(hIMC, QIC_INPUTPROCESSID);
    return dwProcessID == (DWORD_PTR)NtCurrentTeb()->ClientId.UniqueProcess;
}

// Win: ImmLocalAlloc
LPVOID APIENTRY ImmLocalAlloc(DWORD dwFlags, DWORD dwBytes)
{
    if (!ghImmHeap)
    {
        ghImmHeap = RtlGetProcessHeap();
        if (ghImmHeap == NULL)
            return NULL;
    }
    return HeapAlloc(ghImmHeap, dwFlags, dwBytes);
}

// Win: MakeIMENotify
BOOL APIENTRY
Imm32MakeIMENotify(HIMC hIMC, HWND hwnd, DWORD dwAction, DWORD_PTR dwIndex, DWORD_PTR dwValue,
                   DWORD_PTR dwCommand, DWORD_PTR dwData)
{
    DWORD dwThreadId;
    HKL hKL;
    PIMEDPI pImeDpi;

    if (dwAction)
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
                pImeDpi->NotifyIME(hIMC, dwAction, dwIndex, dwValue);

                ImmUnlockImeDpi(pImeDpi); /* unlock */
            }
        }
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
    if (phNewList == NULL)
        return 0;

    Status = NtUserBuildHimcList(dwThreadId, dwCount, phNewList, &dwCount);
    while (Status == STATUS_BUFFER_TOO_SMALL)
    {
        ImmLocalFree(phNewList);
        if (cRetry++ >= MAX_RETRY)
            return 0;

        phNewList = ImmLocalAlloc(0, dwCount * sizeof(HIMC));
        if (phNewList == NULL)
            return 0;

        Status = NtUserBuildHimcList(dwThreadId, dwCount, phNewList, &dwCount);
    }

    if (NT_ERROR(Status) || !dwCount)
    {
        ImmLocalFree(phNewList);
        return 0;
    }

    *pphList = phNewList;
    return dwCount;
#undef INITIAL_COUNT
#undef MAX_RETRY
}

// Win: ConvertImeMenuItemInfoAtoW
INT APIENTRY
Imm32ImeMenuAnsiToWide(const IMEMENUITEMINFOA *pItemA, LPIMEMENUITEMINFOW pItemW,
                       UINT uCodePage, BOOL bBitmap)
{
    INT ret;
    pItemW->cbSize = pItemA->cbSize;
    pItemW->fType = pItemA->fType;
    pItemW->fState = pItemA->fState;
    pItemW->wID = pItemA->wID;
    if (bBitmap)
    {
        pItemW->hbmpChecked = pItemA->hbmpChecked;
        pItemW->hbmpUnchecked = pItemA->hbmpUnchecked;
        pItemW->hbmpItem = pItemA->hbmpItem;
    }
    pItemW->dwItemData = pItemA->dwItemData;
    ret = MultiByteToWideChar(uCodePage, 0, pItemA->szString, -1,
                              pItemW->szString, _countof(pItemW->szString));
    if (ret >= _countof(pItemW->szString))
    {
        ret = 0;
        pItemW->szString[0] = 0;
    }
    return ret;
}

// Win: ConvertImeMenuItemInfoWtoA
INT APIENTRY
Imm32ImeMenuWideToAnsi(const IMEMENUITEMINFOW *pItemW, LPIMEMENUITEMINFOA pItemA,
                       UINT uCodePage)
{
    INT ret;
    pItemA->cbSize = pItemW->cbSize;
    pItemA->fType = pItemW->fType;
    pItemA->fState = pItemW->fState;
    pItemA->wID = pItemW->wID;
    pItemA->hbmpChecked = pItemW->hbmpChecked;
    pItemA->hbmpUnchecked = pItemW->hbmpUnchecked;
    pItemA->dwItemData = pItemW->dwItemData;
    pItemA->hbmpItem = pItemW->hbmpItem;
    ret = WideCharToMultiByte(uCodePage, 0, pItemW->szString, -1,
                              pItemA->szString, _countof(pItemA->szString), NULL, NULL);
    if (ret >= _countof(pItemA->szString))
    {
        ret = 0;
        pItemA->szString[0] = 0;
    }
    return ret;
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
    if (pSubState)
    {
        pIC->fdwSentence |= pSubState->dwValue;
        return TRUE;
    }
    return FALSE;
}

// Win: SavePrivateMode
BOOL APIENTRY
Imm32SaveImeStateSentence(LPINPUTCONTEXTDX pIC, PIME_STATE pState, HKL hKL)
{
    PIME_SUBSTATE pSubState = Imm32FetchImeSubState(pState, hKL);
    if (pSubState)
    {
        pSubState->dwValue = (pIC->fdwSentence & 0xffff0000);
        return TRUE;
    }
    return FALSE;
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
        return 0;

    cchDest = MultiByteToWideChar(uCodePage, MB_PRECOMPOSED, pchSrc, pSrc->dwStrLen,
                                  NULL, 0);
    cbDest = sizeof(RECONVERTSTRING) + (cchDest + 1) * sizeof(WCHAR);
    if (!pDest)
        return cbDest;

    if (pDest->dwSize < cbDest)
        return 0;

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

    return cbDest;
}

DWORD APIENTRY
Imm32ReconvertAnsiFromWide(LPRECONVERTSTRING pDest, const RECONVERTSTRING *pSrc, UINT uCodePage)
{
    DWORD cch0, cch1, cchDest, cbDest;
    LPCWSTR pchSrc = (LPCWSTR)((LPCSTR)pSrc + pSrc->dwStrOffset);
    LPSTR pchDest;

    if (pSrc->dwVersion != 0)
        return 0;

    cchDest = WideCharToMultiByte(uCodePage, 0, pchSrc, pSrc->dwStrLen,
                                  NULL, 0, NULL, NULL);
    cbDest = sizeof(RECONVERTSTRING) + (cchDest + 1) * sizeof(CHAR);
    if (!pDest)
        return cbDest;

    if (pDest->dwSize < cbDest)
        return 0;

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

    return cbDest;
}

typedef BOOL (WINAPI *FN_GetFileVersionInfoW)(LPCWSTR, DWORD, DWORD, LPVOID);
typedef DWORD (WINAPI *FN_GetFileVersionInfoSizeW)(LPCWSTR, LPDWORD);
typedef BOOL (WINAPI *FN_VerQueryValueW)(LPCVOID, LPCWSTR, LPVOID*, PUINT);

static FN_GetFileVersionInfoW s_fnGetFileVersionInfoW = NULL;
static FN_GetFileVersionInfoSizeW s_fnGetFileVersionInfoSizeW = NULL;
static FN_VerQueryValueW s_fnVerQueryValueW = NULL;

// Win: LoadFixVersionInfo
static BOOL APIENTRY Imm32LoadImeFixedInfo(PIMEINFOEX pInfoEx, LPCVOID pVerInfo)
{
    UINT cbFixed = 0;
    VS_FIXEDFILEINFO *pFixed;
    if (!s_fnVerQueryValueW(pVerInfo, L"\\", (LPVOID*)&pFixed, &cbFixed) || !cbFixed)
        return FALSE;

    /* NOTE: The IME module must contain a version info of input method driver. */
    if (pFixed->dwFileType != VFT_DRV || pFixed->dwFileSubtype != VFT2_DRV_INPUTMETHOD)
        return FALSE;

    pInfoEx->dwProdVersion = pFixed->dwProductVersionMS;
    pInfoEx->dwImeWinVersion = 0x40000;
    return TRUE;
}

// Win: GetVersionDatum
static LPWSTR APIENTRY
Imm32GetVerInfoValue(LPCVOID pVerInfo, LPWSTR pszKey, DWORD cchKey, LPCWSTR pszName)
{
    size_t cchExtra;
    LPWSTR pszValue;
    UINT cbValue = 0;

    StringCchLengthW(pszKey, cchKey, &cchExtra);

    StringCchCatW(pszKey, cchKey, pszName);
    s_fnVerQueryValueW(pVerInfo, pszKey, (LPVOID*)&pszValue, &cbValue);
    pszKey[cchExtra] = 0;

    return (cbValue ? pszValue : NULL);
}

// Win: LoadVarVersionInfo
BOOL APIENTRY Imm32LoadImeLangAndDesc(PIMEINFOEX pInfoEx, LPCVOID pVerInfo)
{
    BOOL ret;
    WCHAR szKey[80];
    LPWSTR pszDesc;
    LPWORD pw;
    UINT cbData;
    LANGID LangID;

    /* Getting the version info. See VerQueryValue */
    ret = s_fnVerQueryValueW(pVerInfo, L"\\VarFileInfo\\Translation", (LPVOID*)&pw, &cbData);
    if (!ret || !cbData)
        return FALSE;

    if (pInfoEx->hkl == NULL)
        pInfoEx->hkl = (HKL)(DWORD_PTR)*pw; /* This is an invalid HKL */

    /* Try the current language and the Unicode codepage (0x04B0) */
    LangID = LANGIDFROMLCID(GetThreadLocale());
    StringCchPrintfW(szKey, _countof(szKey), L"\\StringFileInfo\\%04X04B0\\", LangID);
    pszDesc = Imm32GetVerInfoValue(pVerInfo, szKey, _countof(szKey), L"FileDescription");
    if (!pszDesc)
    {
        /* Retry the language and codepage of the IME module */
        StringCchPrintfW(szKey, _countof(szKey), L"\\StringFileInfo\\%04X%04X\\", pw[0], pw[1]);
        pszDesc = Imm32GetVerInfoValue(pVerInfo, szKey, _countof(szKey), L"FileDescription");
    }

    /* The description */
    if (pszDesc)
        StringCchCopyW(pInfoEx->wszImeDescription, _countof(pInfoEx->wszImeDescription), pszDesc);
    else
        pInfoEx->wszImeDescription[0] = 0;

    return TRUE;
}

// Win: LoadVersionInfo
BOOL APIENTRY Imm32LoadImeVerInfo(PIMEINFOEX pImeInfoEx)
{
    HINSTANCE hinstVersion;
    BOOL ret = FALSE, bLoaded = FALSE;
    WCHAR szPath[MAX_PATH];
    LPVOID pVerInfo;
    DWORD cbVerInfo, dwHandle;

    /* Load version.dll to use the version info API */
    Imm32GetSystemLibraryPath(szPath, _countof(szPath), L"version.dll");
    hinstVersion = GetModuleHandleW(szPath);
    if (!hinstVersion)
    {
        hinstVersion = LoadLibraryW(szPath);
        if (!hinstVersion)
            return FALSE;
        bLoaded = TRUE;
    }

#define GET_FN(name) do { \
    s_fn##name = (FN_##name)GetProcAddress(hinstVersion, #name); \
    if (!s_fn##name) goto Quit; \
} while (0)
    GET_FN(GetFileVersionInfoW);
    GET_FN(GetFileVersionInfoSizeW);
    GET_FN(VerQueryValueW);
#undef GET_FN

    /* The path of the IME module */
    Imm32GetSystemLibraryPath(szPath, _countof(szPath), pImeInfoEx->wszImeFile);

    cbVerInfo = s_fnGetFileVersionInfoSizeW(szPath, &dwHandle);
    if (!cbVerInfo)
        goto Quit;

    pVerInfo = ImmLocalAlloc(0, cbVerInfo);
    if (!pVerInfo)
        goto Quit;

    /* Load the version info of the IME module */
    if (s_fnGetFileVersionInfoW(szPath, dwHandle, cbVerInfo, pVerInfo) &&
        Imm32LoadImeFixedInfo(pImeInfoEx, pVerInfo))
    {
        ret = Imm32LoadImeLangAndDesc(pImeInfoEx, pVerInfo);
    }

    ImmLocalFree(pVerInfo);

Quit:
    if (bLoaded)
        FreeLibrary(hinstVersion);
    return ret;
}

// Win: AssignNewLayout
HKL APIENTRY Imm32AssignNewLayout(UINT cKLs, const REG_IME *pLayouts, WORD wLangID)
{
    UINT iKL, wID, wLow = 0xE0FF, wHigh = 0xE01F, wNextID = 0;

    for (iKL = 0; iKL < cKLs; ++iKL)
    {
        wHigh = max(wHigh, HIWORD(pLayouts[iKL].hKL));
        wLow = min(wLow, HIWORD(pLayouts[iKL].hKL));
    }

    if (wHigh < 0xE0FF)
    {
        wNextID = wHigh + 1;
    }
    else if (wLow > 0xE001)
    {
        wNextID = wLow - 1;
    }
    else
    {
        for (wID = 0xE020; wID <= 0xE0FF; ++wID)
        {
            for (iKL = 0; iKL < cKLs; ++iKL)
            {
                if (LOWORD(pLayouts[iKL].hKL) == wLangID &&
                    HIWORD(pLayouts[iKL].hKL) == wID)
                {
                    break;
                }
            }

            if (iKL >= cKLs)
                break;
        }

        if (wID <= 0xE0FF)
            wNextID = wID;
    }

    if (!wNextID)
        return NULL;

    return (HKL)(DWORD_PTR)MAKELONG(wLangID, wNextID);
}

// Win: GetImeLayout
UINT APIENTRY Imm32GetImeLayout(PREG_IME pLayouts, UINT cLayouts)
{
    HKEY hkeyLayouts, hkeyIME;
    WCHAR szImeFileName[80], szImeKey[20];
    UINT iKey, nCount;
    DWORD cbData;
    LONG lError;
    ULONG Value;
    HKL hKL;

    /* Open the registry keyboard layouts */
    lError = RegOpenKeyW(HKEY_LOCAL_MACHINE, REGKEY_KEYBOARD_LAYOUTS, &hkeyLayouts);
    if (lError != ERROR_SUCCESS)
        return 0;

    for (iKey = nCount = 0; ; ++iKey)
    {
        /* Get the key name */
        lError = RegEnumKeyW(hkeyLayouts, iKey, szImeKey, _countof(szImeKey));
        if (lError != ERROR_SUCCESS)
            break;

        if (szImeKey[0] != L'E' && szImeKey[0] != L'e')
            continue; /* Not an IME layout */

        if (pLayouts == NULL) /* for counting only */
        {
            ++nCount;
            continue;
        }

        if (cLayouts <= nCount)
            break;

        lError = RegOpenKeyW(hkeyLayouts, szImeKey, &hkeyIME); /* Open the IME key */
        if (lError != ERROR_SUCCESS)
            break;

        /* Load the "Ime File" value */
        szImeFileName[0] = 0;
        cbData = sizeof(szImeFileName);
        RegQueryValueExW(hkeyIME, L"Ime File", NULL, NULL, (LPBYTE)szImeFileName, &cbData);
        szImeFileName[_countof(szImeFileName) - 1] = 0;

        RegCloseKey(hkeyIME);

        if (!szImeFileName[0])
            break;

        Imm32StrToUInt(szImeKey, &Value, 16);
        hKL = (HKL)(DWORD_PTR)Value;
        if (!IS_IME_HKL(hKL))
            break;

        /* Store the IME key and the IME filename */
        pLayouts[nCount].hKL = hKL;
        StringCchCopyW(pLayouts[nCount].szImeKey, _countof(pLayouts[nCount].szImeKey), szImeKey);
        CharUpperW(szImeFileName);
        StringCchCopyW(pLayouts[nCount].szFileName, _countof(pLayouts[nCount].szFileName),
                       szImeFileName);
        ++nCount;
    }

    RegCloseKey(hkeyLayouts);
    return nCount;
}

// Win: WriteImeLayout
BOOL APIENTRY Imm32WriteImeLayout(HKL hKL, LPCWSTR pchFilePart, LPCWSTR pszLayout)
{
    UINT iPreload;
    HKEY hkeyLayouts, hkeyIME, hkeyPreload;
    WCHAR szImeKey[20], szPreloadNumber[20], szPreloadKey[20], szImeFileName[80];
    DWORD cbData;
    LANGID LangID;
    LONG lError;
    LPCWSTR pszLayoutFile;

    /* Open the registry keyboard layouts */
    lError = RegOpenKeyW(HKEY_LOCAL_MACHINE, REGKEY_KEYBOARD_LAYOUTS, &hkeyLayouts);
    if (lError != ERROR_SUCCESS)
        return FALSE;

    /* Get the IME key from hKL */
    Imm32UIntToStr((DWORD)(DWORD_PTR)hKL, 16, szImeKey, _countof(szImeKey));

    /* Create a registry IME key */
    lError = RegCreateKeyW(hkeyLayouts, szImeKey, &hkeyIME);
    if (lError != ERROR_SUCCESS)
        goto Failure;

    /* Write "Ime File" */
    cbData = (wcslen(pchFilePart) + 1) * sizeof(WCHAR);
    lError = RegSetValueExW(hkeyIME, L"Ime File", 0, REG_SZ, (LPBYTE)pchFilePart, cbData);
    if (lError != ERROR_SUCCESS)
        goto Failure;

    /* Write "Layout Text" */
    cbData = (wcslen(pszLayout) + 1) * sizeof(WCHAR);
    lError = RegSetValueExW(hkeyIME, L"Layout Text", 0, REG_SZ, (LPBYTE)pszLayout, cbData);
    if (lError != ERROR_SUCCESS)
        goto Failure;

    /* Choose "Layout File" from hKL */
    LangID = LOWORD(hKL);
    switch (LOBYTE(LangID))
    {
        case LANG_JAPANESE: pszLayoutFile = L"kbdjpn.dll"; break;
        case LANG_KOREAN:   pszLayoutFile = L"kbdkor.dll"; break;
        default:            pszLayoutFile = L"kbdus.dll"; break;
    }
    StringCchCopyW(szImeFileName, _countof(szImeFileName), pszLayoutFile);

    /* Write "Layout File" */
    cbData = (wcslen(szImeFileName) + 1) * sizeof(WCHAR);
    lError = RegSetValueExW(hkeyIME, L"Layout File", 0, REG_SZ, (LPBYTE)szImeFileName, cbData);
    if (lError != ERROR_SUCCESS)
        goto Failure;

    RegCloseKey(hkeyIME);
    RegCloseKey(hkeyLayouts);

    /* Create "Preload" key */
    RegCreateKeyW(HKEY_CURRENT_USER, L"Keyboard Layout\\Preload", &hkeyPreload);

#define MAX_PRELOAD 0x400
    for (iPreload = 1; iPreload < MAX_PRELOAD; ++iPreload)
    {
        Imm32UIntToStr(iPreload, 10, szPreloadNumber, _countof(szPreloadNumber));

        /* Load the key of the preload number */
        cbData = sizeof(szPreloadKey);
        lError = RegQueryValueExW(hkeyPreload, szPreloadNumber, NULL, NULL,
                                  (LPBYTE)szPreloadKey, &cbData);
        szPreloadKey[_countof(szPreloadKey) - 1] = 0;

        if (lError != ERROR_SUCCESS || lstrcmpiW(szImeKey, szPreloadKey) == 0)
            break; /* Found an empty room or the same key */
    }

    if (iPreload >= MAX_PRELOAD) /* Not found */
    {
        RegCloseKey(hkeyPreload);
        return FALSE;
    }
#undef MAX_PRELOAD

    /* Write the IME key to the preload number */
    cbData = (wcslen(szImeKey) + 1) * sizeof(WCHAR);
    lError = RegSetValueExW(hkeyPreload, szPreloadNumber, 0, REG_SZ, (LPBYTE)szImeKey, cbData);
    RegCloseKey(hkeyPreload);
    return lError == ERROR_SUCCESS;

Failure:
    RegCloseKey(hkeyIME);
    RegDeleteKeyW(hkeyLayouts, szImeKey);
    RegCloseKey(hkeyLayouts);
    return FALSE;
}

typedef INT (WINAPI *FN_LZOpenFileW)(LPWSTR, LPOFSTRUCT, WORD);
typedef LONG (WINAPI *FN_LZCopy)(INT, INT);
typedef VOID (WINAPI *FN_LZClose)(INT);

// Win: CopyImeFile
BOOL APIENTRY Imm32CopyImeFile(LPWSTR pszOldFile, LPCWSTR pszNewFile)
{
    BOOL ret = FALSE, bLoaded = FALSE;
    HMODULE hinstLZ32;
    WCHAR szLZ32Path[MAX_PATH];
    CHAR szDestA[MAX_PATH];
    OFSTRUCT OFStruct;
    FN_LZOpenFileW fnLZOpenFileW;
    FN_LZCopy fnLZCopy;
    FN_LZClose fnLZClose;
    HFILE hfDest, hfSrc;

    /* Load LZ32.dll for copying/decompressing file */
    Imm32GetSystemLibraryPath(szLZ32Path, _countof(szLZ32Path), L"LZ32");
    hinstLZ32 = GetModuleHandleW(szLZ32Path);
    if (!hinstLZ32)
    {
        hinstLZ32 = LoadLibraryW(szLZ32Path);
        if (!hinstLZ32)
            return FALSE;
        bLoaded = TRUE;
    }

#define GET_FN(name) do { \
    fn##name = (FN_##name)GetProcAddress(hinstLZ32, #name); \
    if (!fn##name) goto Quit; \
} while (0)
    GET_FN(LZOpenFileW);
    GET_FN(LZCopy);
    GET_FN(LZClose);
#undef GET_FN

    if (!WideCharToMultiByte(CP_ACP, 0, pszNewFile, -1, szDestA, _countof(szDestA), NULL, NULL))
        goto Quit;
    szDestA[_countof(szDestA) - 1] = 0;

    hfSrc = fnLZOpenFileW(pszOldFile, &OFStruct, OF_READ);
    if (hfSrc < 0)
        goto Quit;

    hfDest = OpenFile(szDestA, &OFStruct, OF_CREATE);
    if (hfDest != HFILE_ERROR)
    {
        ret = (fnLZCopy(hfSrc, hfDest) >= 0);
        _lclose(hfDest);
    }

    fnLZClose(hfSrc);

Quit:
    if (bLoaded)
        FreeLibrary(hinstLZ32);
    return ret;
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
    if (pClientImc == NULL)
        return 0;

    ret = 0;
    hInputContext = pClientImc->hInputContext;
    if (hInputContext)
        ret = (LocalFlags(hInputContext) & LMEM_LOCKCOUNT);

    ImmUnlockClientImc(pClientImc);
    return ret;
}

/***********************************************************************
 *		ImmIMPGetIMEA(IMM32.@)
 */
BOOL WINAPI ImmIMPGetIMEA(HWND hWnd, LPIMEPROA pImePro)
{
    FIXME("(%p, %p)\n", hWnd, pImePro);
    return FALSE;
}

/***********************************************************************
 *		ImmIMPGetIMEW(IMM32.@)
 */
BOOL WINAPI ImmIMPGetIMEW(HWND hWnd, LPIMEPROW pImePro)
{
    FIXME("(%p, %p)\n", hWnd, pImePro);
    return FALSE;
}

/***********************************************************************
 *		ImmIMPQueryIMEA(IMM32.@)
 */
BOOL WINAPI ImmIMPQueryIMEA(LPIMEPROA pImePro)
{
    FIXME("(%p)\n", pImePro);
    return FALSE;
}

/***********************************************************************
 *		ImmIMPQueryIMEW(IMM32.@)
 */
BOOL WINAPI ImmIMPQueryIMEW(LPIMEPROW pImePro)
{
    FIXME("(%p)\n", pImePro);
    return FALSE;
}

/***********************************************************************
 *		ImmIMPSetIMEA(IMM32.@)
 */
BOOL WINAPI ImmIMPSetIMEA(HWND hWnd, LPIMEPROA pImePro)
{
    FIXME("(%p, %p)\n", hWnd, pImePro);
    return FALSE;
}

/***********************************************************************
 *		ImmIMPSetIMEW(IMM32.@)
 */
BOOL WINAPI ImmIMPSetIMEW(HWND hWnd, LPIMEPROW pImePro)
{
    FIXME("(%p, %p)\n", hWnd, pImePro);
    return FALSE;
}
