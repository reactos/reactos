/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing the IMM32 Cicero-aware Text Framework (CTF)
 * COPYRIGHT:   Copyright 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

/*
 * NOTE: Microsoft CTF protocol has vulnerability.
 *       If insecure, we don't follow the dangerous design.
 *
 * https://www.zdnet.com/article/vulnerability-in-microsoft-ctf-protocol-goes-back-to-windows-xp/
 * https://googleprojectzero.blogspot.com/2019/08/down-rabbit-hole.html
 */

/* Define the function types FN_... for CTF IME functions */
#undef DEFINE_CTF_IME_FN
#define DEFINE_CTF_IME_FN(func_name, ret_type, params) \
    typedef ret_type (WINAPI *FN_##func_name)params;
#include "CtfImeTable.h"

HINSTANCE g_hCtfIme = NULL;

#define CTF_IME_FN(func_name) g_p##func_name

/* Define the global variables for CTF IME functions */
#undef DEFINE_CTF_IME_FN
#define DEFINE_CTF_IME_FN(func_name, ret_type, params) \
    FN_##func_name g_p##func_name = NULL;
#include "CtfImeTable.h"

typedef BOOL (WINAPI *FN_ApphelpCheckIME)(LPCWSTR);

/* FIXME: This is kernel32.dll function */
BOOL
WINAPI
BaseCheckAppcompatCache(IN LPCWSTR ApplicationName,
                        IN HANDLE FileHandle,
                        IN LPCWSTR Environment,
                        OUT PULONG Reason);

BOOL Imm32CheckAndApplyAppCompat(ULONG dwReason, LPCWSTR pszAppName)
{
    HINSTANCE hinstApphelp;
    FN_ApphelpCheckIME pApphelpCheckIME;

    if (BaseCheckAppcompatCache(pszAppName, INVALID_HANDLE_VALUE, NULL, &dwReason))
        return TRUE;

    hinstApphelp = GetModuleHandleW(L"apphelp.dll");
    if (!hinstApphelp)
    {
        hinstApphelp = LoadLibraryW(L"apphelp.dll");
        if (!hinstApphelp)
            return TRUE;
    }

    pApphelpCheckIME = (FN_ApphelpCheckIME)GetProcAddress(hinstApphelp, "ApphelpCheckIME");
    if (!pApphelpCheckIME)
        return TRUE;

    return pApphelpCheckIME(pszAppName);
}

HMODULE APIENTRY Imm32LoadCtfIme(VOID)
{
    BOOL bSuccess = FALSE;
    IMEINFOEX ImeInfoEx;
    WCHAR szImeFile[MAX_PATH];

    RtlEnterCriticalSection(&gcsImeDpi);
    do
    {
        if (g_hCtfIme)
            break;

        /*
         * NOTE: (HKL)0x04090409 is English US keyboard (default).
         * The Cicero keyboard uses English US keyboard.
         */
        if (!ImmLoadLayout((HKL)ULongToHandle(0x04090409), &ImeInfoEx))
            break;

        Imm32GetSystemLibraryPath(szImeFile, _countof(szImeFile), ImeInfoEx.wszImeFile);

        if (!Imm32CheckAndApplyAppCompat(0, szImeFile))
            break;

        g_hCtfIme = LoadLibraryW(szImeFile);
        if (!g_hCtfIme)
            break;

        bSuccess = TRUE;

#undef DEFINE_CTF_IME_FN
#define DEFINE_CTF_IME_FN(func_name, ret_type, params) \
        CTF_IME_FN(func_name) = (FN_##func_name)GetProcAddress(g_hCtfIme, #func_name); \
        if (!CTF_IME_FN(func_name)) \
        { \
            bSuccess = FALSE; \
            break; \
        }
#include "CtfImeTable.h"
    } while (0);

    if (g_hCtfIme && !bSuccess)
    {
#undef DEFINE_CTF_IME_FN
#define DEFINE_CTF_IME_FN(func_name, ret_type, params) \
        CTF_IME_FN(func_name) = NULL;
#include "CtfImeTable.h"

        FreeLibrary(g_hCtfIme);
        g_hCtfIme = NULL;
    }

    RtlLeaveCriticalSection(&gcsImeDpi);
    return g_hCtfIme;
}

HRESULT Imm32CtfImeCreateThreadMgr(VOID)
{
    if (!Imm32LoadCtfIme() || CTF_IME_FN(CtfImeCreateThreadMgr) == NULL)
        return E_FAIL;
    return CTF_IME_FN(CtfImeCreateThreadMgr)();
}

HRESULT Imm32CtfImeCreateInputContext(HIMC hIMC)
{
    if (!Imm32LoadCtfIme() || CTF_IME_FN(CtfImeCreateInputContext) == NULL)
        return E_FAIL;
    return CTF_IME_FN(CtfImeCreateInputContext)(hIMC);
}

BOOL CALLBACK Imm32EnumIMC(HIMC hIMC, LPARAM lParam)
{
    Imm32CtfImeCreateInputContext(hIMC);
    return TRUE;
}

// Win: Internal_CtfImeDestroyInputContext
HRESULT APIENTRY Imm32CtfImeDestroyInputContext(HIMC hIMC)
{
    if (!Imm32LoadCtfIme())
        return E_FAIL;

#if 1
    FIXME("(%p)\n", hIMC);
    return E_NOTIMPL;
#else
    return g_pfnCtfImeDestroyInputContext(hIMC);
#endif
}

// Win: CtfImmTIMDestroyInputContext
HRESULT APIENTRY CtfImmTIMDestroyInputContext(HIMC hIMC)
{
    if (!IS_CICERO_MODE() || (GetWin32ClientInfo()->dwCompatFlags2 & 2))
        return E_NOINTERFACE;

    return Imm32CtfImeDestroyInputContext(hIMC);
}

// Win: CtfImmTIMCreateInputContext
HRESULT APIENTRY CtfImmTIMCreateInputContext(HIMC hIMC)
{
    TRACE("(%p)\n", hIMC);
    return E_NOTIMPL;
}

/***********************************************************************
 *      CtfAImmActivate (IMM32.1)
 */
HRESULT WINAPI CtfAImmActivate(HINSTANCE *phinstCtfIme)
{
    HINSTANCE hinstCtfIme;
    HRESULT hr;

    TRACE("(%p)\n", phinstCtfIme);

    hinstCtfIme = Imm32LoadCtfIme();
    hr = Imm32CtfImeCreateThreadMgr();
    if (hr == S_OK)
    {
        GetWin32ClientInfo()->CI_flags |= 0x800; /* FIXME */
        GetWin32ClientInfo()->CI_flags &= ~CI_TFSDISABLED;
        ImmEnumInputContext(0, Imm32EnumIMC, 0);
    }

    if (phinstCtfIme)
        *phinstCtfIme = hinstCtfIme;

    return hr;
}

/***********************************************************************
 *		CtfImmIsCiceroEnabled (IMM32.@)
 */
BOOL WINAPI CtfImmIsCiceroEnabled(VOID)
{
    return IS_CICERO_MODE();
}

/***********************************************************************
 *		CtfImmIsTextFrameServiceDisabled(IMM32.@)
 */
BOOL WINAPI CtfImmIsTextFrameServiceDisabled(VOID)
{
    return !!(GetWin32ClientInfo()->CI_flags & CI_TFSDISABLED);
}

/***********************************************************************
 *		CtfImmTIMActivate(IMM32.@)
 */
HRESULT WINAPI CtfImmTIMActivate(HKL hKL)
{
    FIXME("(%p)\n", hKL);
    return E_NOTIMPL;
}

/***********************************************************************
 *		CtfImmRestoreToolbarWnd(IMM32.@)
 */
VOID WINAPI CtfImmRestoreToolbarWnd(DWORD dwStatus)
{
    FIXME("(0x%lx)\n", dwStatus);
}

/***********************************************************************
 *		CtfImmHideToolbarWnd(IMM32.@)
 */
DWORD WINAPI CtfImmHideToolbarWnd(VOID)
{
    FIXME("()\n");
    return 0;
}

/***********************************************************************
 *		CtfImmDispatchDefImeMessage(IMM32.@)
 */
LRESULT WINAPI CtfImmDispatchDefImeMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    /* FIXME("(%p, %u, %p, %p)\n", hWnd, uMsg, wParam, lParam); */
    return 0;
}

/***********************************************************************
 *		CtfImmIsGuidMapEnable(IMM32.@)
 */
BOOL WINAPI CtfImmIsGuidMapEnable(HIMC hIMC)
{
    DWORD dwThreadId;
    HKL hKL;
    PIMEDPI pImeDpi;
    BOOL ret = FALSE;

    TRACE("(%p)\n", hIMC);

    if (!IS_CICERO_MODE() || IS_16BIT_MODE())
        return ret;

    dwThreadId = (DWORD)NtUserQueryInputContext(hIMC, QIC_INPUTTHREADID);
    hKL = GetKeyboardLayout(dwThreadId);

    if (IS_IME_HKL(hKL))
        return ret;

    pImeDpi = Imm32FindOrLoadImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return ret;

    ret = pImeDpi->CtfImeIsGuidMapEnable(hIMC);

    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		CtfImmGetGuidAtom(IMM32.@)
 */
HRESULT WINAPI CtfImmGetGuidAtom(HIMC hIMC, DWORD dwUnknown, LPDWORD pdwGuidAtom)
{
    HRESULT hr = E_FAIL;
    PIMEDPI pImeDpi;
    DWORD dwThreadId;
    HKL hKL;

    TRACE("(%p, 0xlX, %p)\n", hIMC, dwUnknown, pdwGuidAtom);

    *pdwGuidAtom = 0;

    if (!IS_CICERO_MODE() || IS_16BIT_MODE())
        return hr;

    dwThreadId = (DWORD)NtUserQueryInputContext(hIMC, QIC_INPUTTHREADID);
    hKL = GetKeyboardLayout(dwThreadId);
    if (IS_IME_HKL(hKL))
        return S_OK;

    pImeDpi = Imm32FindOrLoadImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return hr;

    hr = pImeDpi->CtfImeGetGuidAtom(hIMC, dwUnknown, pdwGuidAtom);

    ImmUnlockImeDpi(pImeDpi);
    return hr;
}
