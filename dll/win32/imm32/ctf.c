/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing the IMM32 Cicero-aware Text Framework (CTF)
 * COPYRIGHT:   Copyright 2022-2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

/*
 * TSF stands for "Text Services Framework". "Cicero" is the code name of TSF.
 * CTF stands for "Cicero-aware Text Framework".
 *
 * The CTF IME file is a DLL file. The export functions of the CTF IME file are
 * defined in "CtfImeTable.h" of this folder.
 */

/* The instance of the CTF IME file */
HINSTANCE g_hCtfIme = NULL;

/* Define the function types (FN_...) for CTF IME functions */
#undef DEFINE_CTF_IME_FN
#define DEFINE_CTF_IME_FN(func_name, ret_type, params) \
    typedef ret_type (WINAPI *FN_##func_name)params;
#include "CtfImeTable.h"

/* Define the global variables (g_pfn...) for CTF IME functions */
#undef DEFINE_CTF_IME_FN
#define DEFINE_CTF_IME_FN(func_name, ret_type, params) \
    FN_##func_name g_pfn##func_name = NULL;
#include "CtfImeTable.h"

/* The macro that gets the variable name from the CTF IME function name */
#define CTF_IME_FN(func_name) g_pfn##func_name

/* The type of ApphelpCheckIME function in apphelp.dll */
typedef BOOL (WINAPI *FN_ApphelpCheckIME)(_In_z_ LPCWSTR AppName);

/* FIXME: This is kernel32 function. We have to declare this in some header. */
BOOL WINAPI
BaseCheckAppcompatCache(_In_z_ LPCWSTR ApplicationName,
                        _In_ HANDLE FileHandle,
                        _In_opt_z_ LPCWSTR Environment,
                        _Out_ PULONG pdwReason);

/***********************************************************************
 * This function checks whether the app's IME is disabled by application
 * compatibility patcher.
 */
BOOL
Imm32CheckAndApplyAppCompat(
    _In_ ULONG dwReason,
    _In_z_ LPCWSTR pszAppName)
{
    HINSTANCE hinstApphelp;
    FN_ApphelpCheckIME pApphelpCheckIME;

    /* Query the application compatibility patcher */
    if (BaseCheckAppcompatCache(pszAppName, INVALID_HANDLE_VALUE, NULL, &dwReason))
        return TRUE; /* The app's IME is not disabled */

    /* Load apphelp.dll if necessary */
    hinstApphelp = GetModuleHandleW(L"apphelp.dll");
    if (!hinstApphelp)
    {
        hinstApphelp = LoadLibraryW(L"apphelp.dll");
        if (!hinstApphelp)
            return TRUE; /* There is no apphelp.dll. The app's IME is not disabled */
    }

    /* Is ApphelpCheckIME implemented? */
    pApphelpCheckIME = (FN_ApphelpCheckIME)GetProcAddress(hinstApphelp, "ApphelpCheckIME");
    if (!pApphelpCheckIME)
        return TRUE; /* Not implemented. The app's IME is not disabled */

    /* Is the app's IME disabled or not? */
    return pApphelpCheckIME(pszAppName);
}

/***********************************************************************
 * This function loads the CTF IME file if necessary and establishes
 * communication with the CTF IME.
 */
HINSTANCE
Imm32LoadCtfIme(VOID)
{
    BOOL bSuccess = FALSE;
    IMEINFOEX ImeInfoEx;
    WCHAR szImeFile[MAX_PATH];

    /* Lock the IME interface */
    RtlEnterCriticalSection(&gcsImeDpi);

    do
    {
        if (g_hCtfIme) /* Already loaded? */
        {
            bSuccess = TRUE;
            break;
        }

        /*
         * NOTE: (HKL)0x04090409 is English US keyboard (default).
         * The Cicero keyboard logically uses English US keyboard.
         */
        if (!ImmLoadLayout((HKL)ULongToHandle(0x04090409), &ImeInfoEx))
            break;

        /* Build a path string in system32. The installed IME file must be in system32. */
        Imm32GetSystemLibraryPath(szImeFile, _countof(szImeFile), ImeInfoEx.wszImeFile);

        /* Is the CTF IME disabled by app compatibility patcher? */
        if (!Imm32CheckAndApplyAppCompat(0, szImeFile))
            break; /* The app's IME is disabled */

        /* Load a CTF IME file */
        g_hCtfIme = LoadLibraryW(szImeFile);
        if (!g_hCtfIme)
            break;

        /* Assume success */
        bSuccess = TRUE;

        /* Retrieve the CTF IME functions */
#undef DEFINE_CTF_IME_FN
#define DEFINE_CTF_IME_FN(func_name, ret_type, params) \
        CTF_IME_FN(func_name) = (FN_##func_name)GetProcAddress(g_hCtfIme, #func_name); \
        if (!CTF_IME_FN(func_name)) \
        { \
            bSuccess = FALSE; /* Failed */ \
            break; \
        }
#include "CtfImeTable.h"
    } while (0);

    /* Unload the CTF IME if failed */
    if (!bSuccess)
    {
        /* Set NULL to the function pointers */
#undef DEFINE_CTF_IME_FN
#define DEFINE_CTF_IME_FN(func_name, ret_type, params) CTF_IME_FN(func_name) = NULL;
#include "CtfImeTable.h"

        if (g_hCtfIme)
        {
            FreeLibrary(g_hCtfIme);
            g_hCtfIme = NULL;
        }
    }

    /* Unlock the IME interface */
    RtlLeaveCriticalSection(&gcsImeDpi);

    return g_hCtfIme;
}

/***********************************************************************
 * This function calls CTF IME's CtfImeCreateThreadMgr function.
 */
HRESULT
CtfImeCreateThreadMgr(VOID)
{
    if (!Imm32LoadCtfIme())
        return E_FAIL;

    return CTF_IME_FN(CtfImeCreateThreadMgr)();
}

/***********************************************************************
 * This function calls CTF IME's CtfImeDestroyThreadMgr function.
 */
HRESULT
CtfImeDestroyThreadMgr(VOID)
{
    if (!Imm32LoadCtfIme())
        return E_FAIL;

    return CTF_IME_FN(CtfImeDestroyThreadMgr)();
}

/***********************************************************************
 * This function calls CTF IME's CtfImeCreateInputContext function.
 */
HRESULT
CtfImeCreateInputContext(
    _In_ HIMC hIMC)
{
    if (!Imm32LoadCtfIme())
        return E_FAIL;

    return CTF_IME_FN(CtfImeCreateInputContext)(hIMC);
}

/***********************************************************************
 * The callback function to activate CTF IMEs. Used in CtfAImmActivate.
 */
static BOOL CALLBACK
Imm32EnumCreateCtfICProc(
    _In_ HIMC hIMC,
    _In_ LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    CtfImeCreateInputContext(hIMC);
    return TRUE; /* Continue */
}

/***********************************************************************
 * This function calls CTF IME's CtfImeDestroyInputContext function.
 */
HRESULT
CtfImeDestroyInputContext(_In_ HIMC hIMC)
{
    if (!Imm32LoadCtfIme())
        return E_FAIL;

    return CTF_IME_FN(CtfImeDestroyInputContext)(hIMC);
}

/***********************************************************************
 * This function calls CTF IME's CtfImeDestroyInputContext function if necessary.
 */
HRESULT
CtfImmTIMDestroyInputContext(
    _In_ HIMC hIMC)
{
    if (!IS_CICERO_MODE() || (GetWin32ClientInfo()->dwCompatFlags2 & 2))
        return E_NOINTERFACE;

    return CtfImeDestroyInputContext(hIMC);
}

HRESULT
CtfImmTIMCreateInputContext(
    _In_ HIMC hIMC)
{
    TRACE("(%p)\n", hIMC);
    return E_NOTIMPL;
}

/***********************************************************************
 *      CtfAImmActivate (IMM32.@)
 *
 * This function activates "Active IMM" (AIMM).
 */
HRESULT WINAPI
CtfAImmActivate(
    _Out_opt_ HINSTANCE *phinstCtfIme)
{
    HRESULT hr;
    HINSTANCE hinstCtfIme;

    TRACE("(%p)\n", phinstCtfIme);

    /* Load a CTF IME file if necessary */
    hinstCtfIme = Imm32LoadCtfIme();

    /* Create a thread manager of the CTF IME */
    hr = CtfImeCreateThreadMgr();
    if (hr == S_OK)
    {
        /* Update CI_... flags of the thread client info */
        GetWin32ClientInfo()->CI_flags |= CI_AIMMACTIVATED; /* Activate AIMM */
        GetWin32ClientInfo()->CI_flags &= ~CI_TSFDISABLED;  /* Enable TSF */

        /* Create the CTF input contexts */
        ImmEnumInputContext(0, Imm32EnumCreateCtfICProc, 0);
    }

    if (phinstCtfIme)
        *phinstCtfIme = hinstCtfIme;

    return hr;
}

/***********************************************************************
 *      CtfAImmDeactivate (IMM32.@)
 *
 * This function de-activates "Active IMM" (AIMM).
 */
HRESULT WINAPI
CtfAImmDeactivate(
    _In_ BOOL bDestroy)
{
    HRESULT hr;

    if (!bDestroy)
        return E_FAIL;

    hr = CtfImeDestroyThreadMgr();
    if (hr == S_OK)
    {
        GetWin32ClientInfo()->CI_flags &= ~CI_AIMMACTIVATED; /* Deactivate AIMM */
        GetWin32ClientInfo()->CI_flags |= CI_TSFDISABLED;    /* Disable TSF */
    }

    return hr;
}

/***********************************************************************
 *		CtfImmIsCiceroEnabled (IMM32.@)
 */
BOOL WINAPI
CtfImmIsCiceroEnabled(VOID)
{
    return IS_CICERO_MODE();
}

/***********************************************************************
 *		CtfImmIsTextFrameServiceDisabled(IMM32.@)
 */
BOOL WINAPI
CtfImmIsTextFrameServiceDisabled(VOID)
{
    return !!(GetWin32ClientInfo()->CI_flags & CI_TSFDISABLED);
}

/***********************************************************************
 *		CtfImmTIMActivate(IMM32.@)
 */
HRESULT WINAPI
CtfImmTIMActivate(_In_ HKL hKL)
{
    FIXME("(%p)\n", hKL);
    return E_NOTIMPL;
}

/***********************************************************************
 *		CtfImmRestoreToolbarWnd(IMM32.@)
 */
VOID WINAPI
CtfImmRestoreToolbarWnd(_In_ DWORD dwStatus)
{
    FIXME("(0x%lx)\n", dwStatus);
}

/***********************************************************************
 *		CtfImmHideToolbarWnd(IMM32.@)
 */
DWORD WINAPI
CtfImmHideToolbarWnd(VOID)
{
    FIXME("()\n");
    return 0;
}

/***********************************************************************
 *		CtfImmDispatchDefImeMessage(IMM32.@)
 */
LRESULT WINAPI
CtfImmDispatchDefImeMessage(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    /* FIXME("(%p, %u, %p, %p)\n", hWnd, uMsg, wParam, lParam); */
    return 0;
}

/***********************************************************************
 *		CtfImmIsGuidMapEnable(IMM32.@)
 */
BOOL WINAPI
CtfImmIsGuidMapEnable(
    _In_ HIMC hIMC)
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
HRESULT WINAPI
CtfImmGetGuidAtom(
    _In_ HIMC hIMC,
    _In_ DWORD dwUnknown,
    _Out_ LPDWORD pdwGuidAtom)
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
