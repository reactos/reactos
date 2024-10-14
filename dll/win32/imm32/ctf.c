/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IMM CTF (Collaborative Translation Framework)
 * COPYRIGHT:   Copyright 2022-2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <ndk/ldrfuncs.h> /* for RtlDllShutdownInProgress */
#include <msctf.h> /* for ITfLangBarMgr */
#include <objidl.h> /* for IInitializeSpy */
#include <compat_undoc.h> /* for BaseCheckAppcompatCache */

WINE_DEFAULT_DEBUG_CHANNEL(imm);

static BOOL Imm32InsideLoaderLock(VOID)
{
    return NtCurrentTeb()->ProcessEnvironmentBlock->LoaderLock->OwningThread ==
           NtCurrentTeb()->ClientId.UniqueThread;
}

static BOOL
Imm32IsInteractiveUserLogon(VOID)
{
    BOOL bOK, IsMember = FALSE;
    PSID pSid;
    SID_IDENTIFIER_AUTHORITY IdentAuth = { SECURITY_NT_AUTHORITY };

    if (!AllocateAndInitializeSid(&IdentAuth, 1, SECURITY_INTERACTIVE_RID,
                                  0, 0, 0, 0, 0, 0, 0, &pSid))
    {
        ERR("Error: %ld\n", GetLastError());
        return FALSE;
    }

    bOK = CheckTokenMembership(NULL, pSid, &IsMember);

    if (pSid)
        FreeSid(pSid);

    return bOK && IsMember;
}

static BOOL
Imm32IsRunningInMsoobe(VOID)
{
    LPWSTR pchFilePart = NULL;
    WCHAR Buffer[MAX_PATH], FileName[MAX_PATH];

    if (!GetModuleFileNameW(NULL, FileName, _countof(FileName)))
        return FALSE;

    GetFullPathNameW(FileName, _countof(Buffer), Buffer, &pchFilePart);
    if (!pchFilePart)
        return FALSE;

    return lstrcmpiW(pchFilePart, L"msoobe.exe") == 0;
}

static BOOL
Imm32IsCUASEnabledInRegistry(VOID)
{
    HKEY hKey;
    LSTATUS error;
    DWORD dwType, dwData, cbData;

    error = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\CTF\\SystemShared", &hKey);
    if (error != ERROR_SUCCESS)
        return FALSE;

    dwData = 0;
    cbData = sizeof(dwData);
    error = RegQueryValueExW(hKey, L"CUAS", NULL, &dwType, (LPBYTE)&dwData, &cbData);
    RegCloseKey(hKey);

    if (error != ERROR_SUCCESS || dwType != REG_DWORD)
        return FALSE;

    return !!dwData;
}

BOOL
Imm32GetFn(
    _Inout_opt_ FARPROC *ppfn,
    _Inout_ HINSTANCE *phinstDLL,
    _In_ LPCWSTR pszDllName,
    _In_ LPCSTR pszFuncName)
{
    WCHAR szPath[MAX_PATH];

    if (*ppfn)
        return TRUE;

    if (*phinstDLL == NULL)
    {
        Imm32GetSystemLibraryPath(szPath, _countof(szPath), pszDllName);
        *phinstDLL = LoadLibraryExW(szPath, NULL, 0);
        if (*phinstDLL == NULL)
            return FALSE;
    }

    *ppfn = (FARPROC)GetProcAddress(*phinstDLL, pszFuncName);
    return *ppfn != NULL;
}

#define IMM32_GET_FN(ppfn, phinstDLL, dll_name, func_name) \
    Imm32GetFn((FARPROC*)(ppfn), (phinstDLL), (dll_name), #func_name)

/***********************************************************************
 * OLE32.DLL
 */

HINSTANCE g_hOle32 = NULL;

#define OLE32_FN(name) g_pfnOLE32_##name

typedef HRESULT (WINAPI *FN_CoInitializeEx)(LPVOID, DWORD);
typedef VOID    (WINAPI *FN_CoUninitialize)(VOID);
typedef HRESULT (WINAPI *FN_CoRegisterInitializeSpy)(IInitializeSpy*, ULARGE_INTEGER*);
typedef HRESULT (WINAPI *FN_CoRevokeInitializeSpy)(ULARGE_INTEGER);

FN_CoInitializeEx           OLE32_FN(CoInitializeEx)            = NULL;
FN_CoUninitialize           OLE32_FN(CoUninitialize)            = NULL;
FN_CoRegisterInitializeSpy  OLE32_FN(CoRegisterInitializeSpy)   = NULL;
FN_CoRevokeInitializeSpy    OLE32_FN(CoRevokeInitializeSpy)     = NULL;

#define Imm32GetOle32Fn(func_name) \
    IMM32_GET_FN(&OLE32_FN(func_name), &g_hOle32, L"ole32.dll", #func_name)

HRESULT Imm32CoInitializeEx(VOID)
{
    if (!Imm32GetOle32Fn(CoInitializeEx))
        return E_FAIL;

    return OLE32_FN(CoInitializeEx)(NULL, COINIT_APARTMENTTHREADED);
}

VOID Imm32CoUninitialize(VOID)
{
    if (!Imm32GetOle32Fn(CoUninitialize))
        return;

    OLE32_FN(CoUninitialize)();
}

HRESULT Imm32CoRegisterInitializeSpy(IInitializeSpy* spy, ULARGE_INTEGER* cookie)
{
    if (!Imm32GetOle32Fn(CoRegisterInitializeSpy))
        return E_FAIL;

    return OLE32_FN(CoRegisterInitializeSpy)(spy, cookie);
}

HRESULT Imm32CoRevokeInitializeSpy(ULARGE_INTEGER cookie)
{
    if (!Imm32GetOle32Fn(CoRevokeInitializeSpy))
        return E_FAIL;

    return OLE32_FN(CoRevokeInitializeSpy)(cookie);
}

/***********************************************************************
 * MSCTF.DLL
 */

HINSTANCE g_hMsctf = NULL;

#define MSCTF_FN(name) g_pfnMSCTF_##name

typedef HRESULT (WINAPI *FN_TF_CreateLangBarMgr)(ITfLangBarMgr**);
typedef VOID    (WINAPI *FN_TF_InvalidAssemblyListCacheIfExist)(VOID);

FN_TF_CreateLangBarMgr                MSCTF_FN(TF_CreateLangBarMgr)                = NULL;
FN_TF_InvalidAssemblyListCacheIfExist MSCTF_FN(TF_InvalidAssemblyListCacheIfExist) = NULL;

#define Imm32GetMsctfFn(func_name) \
    IMM32_GET_FN(&MSCTF_FN(func_name), &g_hMsctf, L"msctf.dll", #func_name)

HRESULT Imm32TF_CreateLangBarMgr(_Inout_ ITfLangBarMgr **ppBarMgr)
{
    TRACE("TF_CreateLangBarMgr(%p)\n", ppBarMgr);

    if (!Imm32GetMsctfFn(TF_CreateLangBarMgr))
        return E_FAIL;

    return MSCTF_FN(TF_CreateLangBarMgr)(ppBarMgr);
}

VOID Imm32TF_InvalidAssemblyListCacheIfExist(VOID)
{
    TRACE("TF_InvalidAssemblyListCacheIfExist()\n");

    if (!Imm32GetMsctfFn(TF_InvalidAssemblyListCacheIfExist))
        return;

    MSCTF_FN(TF_InvalidAssemblyListCacheIfExist)();
}

/***********************************************************************
 * CTF (Collaborative Translation Framework) IME support
 */

/* "Active IMM" compatibility flags */
DWORD g_aimm_compat_flags = 0;

/* Disable CUAS? */
BOOL g_disable_CUAS_flag = FALSE;

/* The instance of the CTF IME file */
HINSTANCE g_hCtfIme = NULL;

/* Define the function types (FN_...) for CTF IME functions */
#undef DEFINE_CTF_IME_FN
#define DEFINE_CTF_IME_FN(func_name, ret_type, params) \
    typedef ret_type (WINAPI *FN_##func_name)params;
#include <CtfImeTable.h>

/* Define the global variables (g_pfn...) for CTF IME functions */
#undef DEFINE_CTF_IME_FN
#define DEFINE_CTF_IME_FN(func_name, ret_type, params) \
    FN_##func_name g_pfn##func_name = NULL;
#include <CtfImeTable.h>

/* The macro that gets the variable name from the CTF IME function name */
#define CTF_IME_FN(func_name) g_pfn##func_name

/* The type of ApphelpCheckIME function in apphelp.dll */
typedef BOOL (WINAPI *FN_ApphelpCheckIME)(_In_z_ LPCWSTR AppName);

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
 * TLS (Thread-Local Storage)
 *
 * See: TlsAlloc
 */

DWORD g_dwTLSIndex = -1;

/* IMM Thread-Local Storage (TLS) data */
typedef struct IMMTLSDATA
{
    IInitializeSpy *pSpy;            /* CoInitialize Spy */
    DWORD           dwUnknown1;
    ULARGE_INTEGER  uliCookie;       /* Spy requires a cookie for revoking */
    BOOL            bDoCount;        /* Is it counting? */
    DWORD           dwSkipCount;     /* The skipped count */
    BOOL            bUninitializing; /* Is it uninitializing? */
    DWORD           dwUnknown2;
} IMMTLSDATA, *PIMMTLSDATA;

static VOID
Imm32InitTLS(VOID)
{
    RtlEnterCriticalSection(&gcsImeDpi);

    if (g_dwTLSIndex == -1)
        g_dwTLSIndex = TlsAlloc();

    RtlLeaveCriticalSection(&gcsImeDpi);
}

static IMMTLSDATA*
Imm32AllocateTLS(VOID)
{
    IMMTLSDATA *pData;

    if (g_dwTLSIndex == -1)
        return NULL;

    pData = (IMMTLSDATA*)TlsGetValue(g_dwTLSIndex);
    if (pData)
        return pData;

    pData = (IMMTLSDATA*)ImmLocalAlloc(HEAP_ZERO_MEMORY, sizeof(IMMTLSDATA));
    if (IS_NULL_UNEXPECTEDLY(pData))
        return NULL;

    if (IS_FALSE_UNEXPECTEDLY(TlsSetValue(g_dwTLSIndex, pData)))
    {
        ImmLocalFree(pData);
        return NULL;
    }

    return pData;
}

static IMMTLSDATA*
Imm32GetTLS(VOID)
{
    if (g_dwTLSIndex == -1)
        return NULL;

    return (IMMTLSDATA*)TlsGetValue(g_dwTLSIndex);
}

/* Get */
static DWORD
Imm32GetCoInitCountSkip(VOID)
{
    IMMTLSDATA *pData = Imm32GetTLS();
    if (!pData)
        return 0;
    return pData->dwSkipCount;
}

/* Increment */
static DWORD
Imm32IncCoInitCountSkip(VOID)
{
    IMMTLSDATA *pData;
    DWORD dwOldSkipCount;

    pData = Imm32GetTLS();
    if (!pData)
        return 0;

    dwOldSkipCount = pData->dwSkipCount;
    if (pData->bDoCount)
        pData->dwSkipCount = dwOldSkipCount + 1;

    return dwOldSkipCount;
}

/* Decrement */
static DWORD
Imm32DecCoInitCountSkip(VOID)
{
    DWORD dwSkipCount;
    IMMTLSDATA *pData;

    pData = Imm32GetTLS();;
    if (!pData)
        return 0;

    dwSkipCount = pData->dwSkipCount;
    if (pData->bDoCount)
    {
        if (dwSkipCount)
            pData->dwSkipCount = dwSkipCount - 1;
    }

    return dwSkipCount;
}

/***********************************************************************
 *		CtfImmEnterCoInitCountSkipMode (IMM32.@)
 */
VOID WINAPI CtfImmEnterCoInitCountSkipMode(VOID)
{
    IMMTLSDATA *pData;

    TRACE("()\n");

    pData = Imm32GetTLS();
    if (pData)
        ++(pData->bDoCount);
}

/***********************************************************************
 *		CtfImmLeaveCoInitCountSkipMode (IMM32.@)
 */
BOOL WINAPI CtfImmLeaveCoInitCountSkipMode(VOID)
{
    IMMTLSDATA *pData;

    TRACE("()\n");

    pData = Imm32GetTLS();
    if (!pData || !pData->bDoCount)
        return FALSE;

    --(pData->bDoCount);
    return TRUE;
}

/***********************************************************************
 * ISPY (I am not spy!)
 *
 * ISPY watches CoInitialize[Ex] / CoUninitialize to manage COM initialization status.
 */

typedef struct ISPY
{
    const IInitializeSpyVtbl *m_pSpyVtbl;
    LONG m_cRefs;
} ISPY, *PISPY;

static STDMETHODIMP
ISPY_QueryInterface(
    _Inout_ IInitializeSpy *pThis,
    _In_ REFIID riid,
    _Inout_ LPVOID *ppvObj)
{
    ISPY *pSpy = (ISPY*)pThis;

    if (!ppvObj)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (!IsEqualIID(riid, &IID_IUnknown) && !IsEqualIID(riid, &IID_IInitializeSpy))
        return E_NOINTERFACE;

    ++(pSpy->m_cRefs);
    *ppvObj = pSpy;
    return S_OK;
}

static STDMETHODIMP_(ULONG)
ISPY_AddRef(
    _Inout_ IInitializeSpy *pThis)
{
    ISPY *pSpy = (ISPY*)pThis;
    return ++pSpy->m_cRefs;
}

static STDMETHODIMP_(ULONG)
ISPY_Release(
    _Inout_ IInitializeSpy *pThis)
{
    ISPY *pSpy = (ISPY*)pThis;
    if (--pSpy->m_cRefs == 0)
    {
        ImmLocalFree(pSpy);
        return 0;
    }
    return pSpy->m_cRefs;
}

/*
 * (Pre/Post)(Initialize/Uninitialize) will be automatically called from OLE32
 * as the results of watching.
 */

static STDMETHODIMP
ISPY_PreInitialize(
    _Inout_ IInitializeSpy *pThis,
    _In_ DWORD dwCoInit,
    _In_ DWORD dwCurThreadAptRefs)
{
    DWORD cCount;

    UNREFERENCED_PARAMETER(pThis);

    cCount = Imm32IncCoInitCountSkip();
    if (!dwCoInit &&
        (dwCurThreadAptRefs == cCount + 1) &&
        (GetWin32ClientInfo()->CI_flags & CI_CTFCOINIT))
    {
        Imm32ActivateOrDeactivateTIM(FALSE);
        CtfImmCoUninitialize();
    }

    return S_OK;
}

static STDMETHODIMP
ISPY_PostInitialize(
    _Inout_ IInitializeSpy *pThis,
    _In_ HRESULT hrCoInit,
    _In_ DWORD dwCoInit,
    _In_ DWORD dwNewThreadAptRefs)
{
    DWORD CoInitCountSkip;

    UNREFERENCED_PARAMETER(pThis);
    UNREFERENCED_PARAMETER(dwCoInit);

    CoInitCountSkip = Imm32GetCoInitCountSkip();

    if ((hrCoInit != S_FALSE) ||
        (dwNewThreadAptRefs != CoInitCountSkip + 2) ||
        !(GetWin32ClientInfo()->CI_flags & CI_CTFCOINIT))
    {
        return hrCoInit;
    }

    return S_OK;
}

static STDMETHODIMP
ISPY_PreUninitialize(
    _Inout_ IInitializeSpy *pThis,
    _In_ DWORD dwCurThreadAptRefs)
{
    UNREFERENCED_PARAMETER(pThis);

    if (dwCurThreadAptRefs == 1 &&
        !RtlDllShutdownInProgress() &&
        !Imm32InsideLoaderLock() &&
        (GetWin32ClientInfo()->CI_flags & CI_CTFCOINIT))
    {
        IMMTLSDATA *pData = Imm32GetTLS();
        if (pData && !pData->bUninitializing)
            Imm32CoInitializeEx();
    }

    return S_OK;
}

static STDMETHODIMP
ISPY_PostUninitialize(
    _In_ IInitializeSpy *pThis,
    _In_ DWORD dwNewThreadAptRefs)
{
    UNREFERENCED_PARAMETER(pThis);
    UNREFERENCED_PARAMETER(dwNewThreadAptRefs);
    Imm32DecCoInitCountSkip();
    return S_OK;
}

static const IInitializeSpyVtbl g_vtblISPY =
{
    ISPY_QueryInterface,
    ISPY_AddRef,
    ISPY_Release,
    ISPY_PreInitialize,
    ISPY_PostInitialize,
    ISPY_PreUninitialize,
    ISPY_PostUninitialize,
};

static ISPY*
Imm32AllocIMMISPY(VOID)
{
    ISPY *pSpy = (ISPY*)ImmLocalAlloc(0, sizeof(ISPY));
    if (!pSpy)
        return NULL;

    pSpy->m_pSpyVtbl = &g_vtblISPY;
    pSpy->m_cRefs = 1;
    return pSpy;
}

#define Imm32DeleteIMMISPY(pSpy) ImmLocalFree(pSpy)

/***********************************************************************
 *		CtfImmCoInitialize (Not exported)
 */
HRESULT
CtfImmCoInitialize(VOID)
{
    HRESULT hr;
    IMMTLSDATA *pData;
    ISPY *pSpy;

    if (GetWin32ClientInfo()->CI_flags & CI_CTFCOINIT)
        return S_OK; /* Already initialized */

    hr = Imm32CoInitializeEx();
    if (FAILED_UNEXPECTEDLY(hr))
        return hr; /* CoInitializeEx failed */

    GetWin32ClientInfo()->CI_flags |= CI_CTFCOINIT;
    Imm32InitTLS();

    pData = Imm32AllocateTLS();
    if (!pData || pData->pSpy)
        return S_OK; /* Cannot allocate or already it has a spy */

    pSpy = Imm32AllocIMMISPY();
    pData->pSpy = (IInitializeSpy*)pSpy;
    if (IS_NULL_UNEXPECTEDLY(pSpy))
        return S_OK; /* Cannot allocate a spy */

    if (FAILED_UNEXPECTEDLY(Imm32CoRegisterInitializeSpy(pData->pSpy, &pData->uliCookie)))
    {
        /* Failed to register the spy */
        Imm32DeleteIMMISPY(pData->pSpy);
        pData->pSpy = NULL;
        pData->uliCookie.QuadPart = 0;
    }

    return S_OK;
}

/***********************************************************************
 *		CtfImmCoUninitialize (IMM32.@)
 */
VOID WINAPI
CtfImmCoUninitialize(VOID)
{
    IMMTLSDATA *pData;

    if (!(GetWin32ClientInfo()->CI_flags & CI_CTFCOINIT))
        return; /* Not CoInitialize'd */

    pData = Imm32GetTLS();
    if (pData)
    {
        pData->bUninitializing = TRUE;
        Imm32CoUninitialize(); /* Do CoUninitialize */
        pData->bUninitializing = FALSE;

        GetWin32ClientInfo()->CI_flags &= ~CI_CTFCOINIT;
    }

    pData = Imm32AllocateTLS();
    if (!pData || !pData->pSpy)
        return; /* There were no spy */

    /* Our work is done. We don't need spies like you anymore. */
    Imm32CoRevokeInitializeSpy(pData->uliCookie);
    ISPY_Release(pData->pSpy);
    pData->pSpy = NULL;
    pData->uliCookie.QuadPart = 0;
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
            break; /* This IME is disabled */

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
#include <CtfImeTable.h>
    } while (0);

    /* Unload the CTF IME if failed */
    if (!bSuccess)
    {
        /* Set NULL to the function pointers */
#undef DEFINE_CTF_IME_FN
#define DEFINE_CTF_IME_FN(func_name, ret_type, params) CTF_IME_FN(func_name) = NULL;
#include <CtfImeTable.h>

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
 * This function calls the same name function of the CTF IME side.
 */
HRESULT
CtfImeCreateThreadMgr(VOID)
{
    TRACE("()\n");

    if (!Imm32LoadCtfIme())
        return E_FAIL;

    return CTF_IME_FN(CtfImeCreateThreadMgr)();
}

/***********************************************************************
 * This function calls the same name function of the CTF IME side.
 */
BOOL
CtfImeProcessCicHotkey(_In_ HIMC hIMC, _In_ UINT vKey, _In_ LPARAM lParam)
{
    TRACE("(%p, %u, %p)\n", hIMC, vKey, lParam);

    if (!Imm32LoadCtfIme())
        return FALSE;

    return CTF_IME_FN(CtfImeProcessCicHotkey)(hIMC, vKey, lParam);
}

/***********************************************************************
 * This function calls the same name function of the CTF IME side.
 */
HRESULT
CtfImeDestroyThreadMgr(VOID)
{
    TRACE("()\n");

    if (!Imm32LoadCtfIme())
        return E_FAIL;

    return CTF_IME_FN(CtfImeDestroyThreadMgr)();
}

/***********************************************************************
 *		CtfAImmIsIME (IMM32.@)
 *
 * @return TRUE if CTF IME or IMM IME is enabled.
 */
BOOL WINAPI
CtfAImmIsIME(_In_ HKL hKL)
{
    TRACE("(%p)\n", hKL);
    if (!Imm32LoadCtfIme())
        return ImmIsIME(hKL);
    return CTF_IME_FN(CtfImeIsIME)(hKL);
}

/***********************************************************************
 *		CtfImmIsCiceroStartedInThread (IMM32.@)
 *
 * @return TRUE if Cicero is started in the current thread.
 */
BOOL WINAPI
CtfImmIsCiceroStartedInThread(VOID)
{
    TRACE("()\n");
    return !!(GetWin32ClientInfo()->CI_flags & CI_CICERO_STARTED);
}

/***********************************************************************
 *		CtfImmSetCiceroStartInThread (IMM32.@)
 */
VOID WINAPI CtfImmSetCiceroStartInThread(_In_ BOOL bStarted)
{
    TRACE("(%d)\n", bStarted);
    if (bStarted)
        GetWin32ClientInfo()->CI_flags |= CI_CICERO_STARTED;
    else
        GetWin32ClientInfo()->CI_flags &= ~CI_CICERO_STARTED;
}

/***********************************************************************
 *		CtfImmSetAppCompatFlags (IMM32.@)
 *
 * Sets the application compatibility flags.
 */
VOID WINAPI
CtfImmSetAppCompatFlags(_In_ DWORD dwFlags)
{
    TRACE("(0x%08X)\n", dwFlags);
    if (!(dwFlags & 0xF0FFFFFF))
        g_aimm_compat_flags = dwFlags;
}

/***********************************************************************
 * This function calls the same name function of the CTF IME side.
 */
HRESULT
CtfImeCreateInputContext(
    _In_ HIMC hIMC)
{
    TRACE("(%p)\n", hIMC);

    if (!Imm32LoadCtfIme())
        return E_FAIL;

    return CTF_IME_FN(CtfImeCreateInputContext)(hIMC);
}

/***********************************************************************
 * This function calls the same name function of the CTF IME side.
 */
HRESULT
CtfImeDestroyInputContext(_In_ HIMC hIMC)
{
    TRACE("(%p)\n", hIMC);

    if (!Imm32LoadCtfIme())
        return E_FAIL;

    return CTF_IME_FN(CtfImeDestroyInputContext)(hIMC);
}

/***********************************************************************
 * This function calls the same name function of the CTF IME side.
 */
HRESULT
CtfImeSetActiveContextAlways(
    _In_ HIMC hIMC,
    _In_ BOOL fActive,
    _In_ HWND hWnd,
    _In_ HKL hKL)
{
    TRACE("(%p, %d, %p, %p)\n", hIMC, fActive, hWnd, hKL);

    if (!Imm32LoadCtfIme())
        return E_FAIL;

    return CTF_IME_FN(CtfImeSetActiveContextAlways)(hIMC, fActive, hWnd, hKL);
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
 * Thread Input Manager (TIM)
 */

static BOOL
Imm32IsTIMDisabledInRegistry(VOID)
{
    DWORD dwData, cbData;
    HKEY hKey;
    LSTATUS error;

    error = RegOpenKeyW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\CTF", &hKey);
    if (error != ERROR_SUCCESS)
        return FALSE;

    dwData = 0;
    cbData = sizeof(dwData);
    RegQueryValueExW(hKey, L"Disable Thread Input Manager", NULL, NULL, (LPBYTE)&dwData, &cbData);
    RegCloseKey(hKey);
    return !!dwData;
}

HRESULT
Imm32ActivateOrDeactivateTIM(
    _In_ BOOL bCreate)
{
    HRESULT hr = S_OK;

    if (!IS_CICERO_MODE() || IS_16BIT_MODE() ||
        !(GetWin32ClientInfo()->CI_flags & CI_CTFCOINIT))
    {
        return S_OK; /* No need to activate/de-activate TIM */
    }

    if (bCreate)
    {
        if (!(GetWin32ClientInfo()->CI_flags & CI_CTFTIM))
        {
            hr = CtfImeCreateThreadMgr();
            if (SUCCEEDED(hr))
                GetWin32ClientInfo()->CI_flags |= CI_CTFTIM;
        }
    }
    else /* Destroy */
    {
        if (GetWin32ClientInfo()->CI_flags & CI_CTFTIM)
        {
            hr = CtfImeDestroyThreadMgr();
            if (SUCCEEDED(hr))
                GetWin32ClientInfo()->CI_flags &= ~CI_CTFTIM;
        }
    }

    return hr;
}

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
    PCLIENTIMC pClientImc;
    DWORD_PTR dwImeThreadId, dwCurrentThreadId;
    HRESULT hr = S_FALSE;

    TRACE("(%p)\n", hIMC);

    pClientImc = ImmLockClientImc(hIMC);
    if (!pClientImc)
        return E_FAIL;

    if (GetWin32ClientInfo()->CI_flags & CI_AIMMACTIVATED)
    {
        if (!pClientImc->bCtfIme)
        {
            dwImeThreadId = NtUserQueryInputContext(hIMC, QIC_INPUTTHREADID);
            dwCurrentThreadId = GetCurrentThreadId();
            if (dwImeThreadId == dwCurrentThreadId)
            {
                pClientImc->bCtfIme = TRUE;
                hr = CtfImeCreateInputContext(hIMC);
                if (FAILED_UNEXPECTEDLY(hr))
                    pClientImc->bCtfIme = FALSE;
            }
        }
    }
    else
    {
        if (!(GetWin32ClientInfo()->CI_flags & CI_CTFTIM))
            return S_OK;

        if (!pClientImc->bCtfIme)
        {
            dwImeThreadId = NtUserQueryInputContext(hIMC, QIC_INPUTTHREADID);
            dwCurrentThreadId = GetCurrentThreadId();
            if ((dwImeThreadId == dwCurrentThreadId) && IS_CICERO_MODE() && !IS_16BIT_MODE())
            {
                pClientImc->bCtfIme = TRUE;
                hr = CtfImeCreateInputContext(hIMC);
                if (FAILED_UNEXPECTEDLY(hr))
                    pClientImc->bCtfIme = FALSE;
            }
        }
    }

    ImmUnlockClientImc(pClientImc);
    return hr;
}

/***********************************************************************
 *      CtfImmLastEnabledWndDestroy (IMM32.@)
 *
 * Same as Imm32ActivateOrDeactivateTIM but its naming is improper.
 */
HRESULT WINAPI
CtfImmLastEnabledWndDestroy(
    _In_ BOOL bCreate)
{
    TRACE("(%d)\n", bCreate);
    return Imm32ActivateOrDeactivateTIM(bCreate);
}

/***********************************************************************
 *      CtfAImmActivate (IMM32.@)
 *
 * This function activates "Active IMM" (AIMM) and TSF.
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
 * This function de-activates "Active IMM" (AIMM) and TSF.
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
 *
 * @return TRUE if Cicero is enabled.
 */
BOOL WINAPI
CtfImmIsCiceroEnabled(VOID)
{
    return IS_CICERO_MODE();
}

/***********************************************************************
 *		CtfImmIsTextFrameServiceDisabled (IMM32.@)
 *
 * @return TRUE if TSF is disabled.
 */
BOOL WINAPI
CtfImmIsTextFrameServiceDisabled(VOID)
{
    return !!(GetWin32ClientInfo()->CI_flags & CI_TSFDISABLED);
}

/***********************************************************************
 *		ImmDisableTextFrameService (IMM32.@)
 */
BOOL WINAPI
ImmDisableTextFrameService(_In_ DWORD dwThreadId)
{
    HRESULT hr = S_OK;

    TRACE("(0x%lX)\n", dwThreadId);

    if (dwThreadId == -1)
        g_disable_CUAS_flag = TRUE;

    if ((dwThreadId && !g_disable_CUAS_flag) || (GetWin32ClientInfo()->CI_flags & CI_TSFDISABLED))
        return TRUE;

    GetWin32ClientInfo()->CI_flags |= CI_TSFDISABLED;

    if (IS_CICERO_MODE() && !IS_16BIT_MODE() &&
        (GetWin32ClientInfo()->CI_flags & CI_CTFCOINIT) &&
        (GetWin32ClientInfo()->CI_flags & CI_CTFTIM))
    {
        hr = CtfImeDestroyThreadMgr();
        if (SUCCEEDED(hr))
        {
            GetWin32ClientInfo()->CI_flags &= ~CI_CTFTIM;
            CtfImmCoUninitialize();
        }
    }

    return hr == S_OK;
}

/***********************************************************************
 *		CtfImmTIMActivate (IMM32.@)
 *
 * Activates Thread Input Manager (TIM) in the thread.
 */
HRESULT WINAPI
CtfImmTIMActivate(_In_ HKL hKL)
{
    HRESULT hr = S_OK;

    TRACE("(%p)\n", hKL);

    if (g_disable_CUAS_flag)
    {
        TRACE("g_disable_CUAS_flag\n");
        GetWin32ClientInfo()->CI_flags |= CI_TSFDISABLED;
        return FALSE;
    }

    if (GetWin32ClientInfo()->CI_flags & CI_TSFDISABLED)
    {
        TRACE("CI_TSFDISABLED\n");
        return FALSE;
    }

    if (Imm32IsTIMDisabledInRegistry())
    {
        TRACE("TIM is disabled in registry\n");
        GetWin32ClientInfo()->CI_flags |= CI_TSFDISABLED;
        return FALSE;
    }

    if (!Imm32IsInteractiveUserLogon() || Imm32IsRunningInMsoobe())
    {
        TRACE("TIM is disabled due to LOGON or MSOBE\n");
        return FALSE;
    }

    if (!Imm32IsCUASEnabledInRegistry())
    {
        TRACE("CUAS is disabled in registry\n");
        GetWin32ClientInfo()->CI_flags |= CI_TSFDISABLED;
        return FALSE;
    }

    if (NtCurrentTeb()->ProcessEnvironmentBlock->AppCompatFlags.LowPart & 0x100)
    {
        TRACE("CUAS is disabled by AppCompatFlags\n");
        GetWin32ClientInfo()->CI_flags |= CI_TSFDISABLED;
        return FALSE;
    }

    if (RtlIsThreadWithinLoaderCallout() || Imm32InsideLoaderLock())
    {
        TRACE("TIM is disabled by Loader\n");
        return FALSE;
    }

    if (!IS_CICERO_MODE() || IS_16BIT_MODE())
    {
        TRACE("TIM is disabled because CICERO mode is unset\n");
        return FALSE;
    }

    if (IS_IME_HKL(hKL))
        hKL = (HKL)UlongToHandle(MAKELONG(LOWORD(hKL), LOWORD(hKL)));

    if (!ImmLoadIME(hKL))
        Imm32TF_InvalidAssemblyListCacheIfExist();

    CtfImmCoInitialize();

    if ((GetWin32ClientInfo()->CI_flags & CI_CTFCOINIT) &&
        !(GetWin32ClientInfo()->CI_flags & CI_CTFTIM))
    {
        hr = CtfImeCreateThreadMgr();
        if (SUCCEEDED(hr))
            GetWin32ClientInfo()->CI_flags |= CI_CTFTIM;
    }

    return hr;
}

/***********************************************************************
 * Setting language band
 */

typedef struct IMM_DELAY_SET_LANG_BAND
{
    HWND hWnd;
    BOOL fSet;
} IMM_DELAY_SET_LANG_BAND, *PIMM_DELAY_SET_LANG_BAND;

/* Sends a message to set the language band with delay. */
static DWORD APIENTRY Imm32DelaySetLangBandProc(LPVOID arg)
{
    HWND hwndDefIME;
    WPARAM wParam;
    DWORD_PTR lResult;
    PIMM_DELAY_SET_LANG_BAND pSetBand = arg;

    Sleep(3000); /* Delay 3 seconds! */

    hwndDefIME = ImmGetDefaultIMEWnd(pSetBand->hWnd);
    if (hwndDefIME)
    {
        wParam = (pSetBand->fSet ? IMS_SETLANGBAND : IMS_UNSETLANGBAND);
        SendMessageTimeoutW(hwndDefIME, WM_IME_SYSTEM, wParam, (LPARAM)pSetBand->hWnd,
                            SMTO_BLOCK | SMTO_ABORTIFHUNG, 5000, &lResult);
    }
    ImmLocalFree(pSetBand);
    return FALSE;
}

/* Updates the language band. */
LRESULT
CtfImmSetLangBand(
    _In_ HWND hWnd,
    _In_ BOOL fSet)
{
    HANDLE hThread;
    PWND pWnd = NULL;
    PIMM_DELAY_SET_LANG_BAND pSetBand;
    DWORD_PTR lResult = 0;

    if (hWnd && gpsi)
        pWnd = ValidateHwndNoErr(hWnd);

    if (IS_NULL_UNEXPECTEDLY(pWnd))
        return 0;

    if (pWnd->state2 & WNDS2_WMCREATEMSGPROCESSED)
    {
        SendMessageTimeoutW(hWnd, WM_USER + 0x105, 0, fSet, SMTO_BLOCK | SMTO_ABORTIFHUNG,
                            5000, &lResult);
        return lResult;
    }

    pSetBand = ImmLocalAlloc(0, sizeof(IMM_DELAY_SET_LANG_BAND));
    if (IS_NULL_UNEXPECTEDLY(pSetBand))
        return 0;

    pSetBand->hWnd = hWnd;
    pSetBand->fSet = fSet;

    hThread = CreateThread(NULL, 0, Imm32DelaySetLangBandProc, pSetBand, 0, NULL);
    if (hThread)
        CloseHandle(hThread);
    return 0;
}

/***********************************************************************
 *		CtfImmGenerateMessage (IMM32.@)
 */
BOOL WINAPI
CtfImmGenerateMessage(
    _In_ HIMC hIMC,
    _In_ BOOL bSend)
{
    DWORD_PTR dwImeThreadId, dwCurrentThreadId;
    PCLIENTIMC pClientImc;
    BOOL bUnicode;
    LPINPUTCONTEXT pIC;
    DWORD iMsg, dwNumMsgBuf;
    LPTRANSMSG pOldTransMsg, pNewTransMsg;
    SIZE_T cbTransMsg;

    TRACE("(%p, %d)\n", hIMC, bSend);

    dwImeThreadId = NtUserQueryInputContext(hIMC, QIC_INPUTTHREADID);
    dwCurrentThreadId = GetCurrentThreadId();
    if (dwImeThreadId != dwCurrentThreadId)
    {
        ERR("%p vs %p\n", dwImeThreadId, dwCurrentThreadId);
        return FALSE;
    }

    pClientImc = ImmLockClientImc(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pClientImc))
        return FALSE;

    bUnicode = !!(pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    pIC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return FALSE;

    dwNumMsgBuf = pIC->dwNumMsgBuf;
    pOldTransMsg = (LPTRANSMSG)ImmLockIMCC(pIC->hMsgBuf);
    if (IS_NULL_UNEXPECTEDLY(pOldTransMsg))
    {
        pIC->dwNumMsgBuf = 0;
        ImmUnlockIMC(hIMC);
        return TRUE;
    }

    cbTransMsg = sizeof(TRANSMSG) * dwNumMsgBuf;
    pNewTransMsg = (PTRANSMSG)ImmLocalAlloc(0, cbTransMsg);
    if (IS_NULL_UNEXPECTEDLY(pNewTransMsg))
    {
        ImmUnlockIMCC(pIC->hMsgBuf);
        pIC->dwNumMsgBuf = 0;
        ImmUnlockIMC(hIMC);
        return TRUE;
    }

    RtlCopyMemory(pNewTransMsg, pOldTransMsg, cbTransMsg);

    for (iMsg = 0; iMsg < dwNumMsgBuf; ++iMsg)
    {
        HWND hWnd = pIC->hWnd;
        UINT uMsg = pNewTransMsg[iMsg].message;
        WPARAM wParam = pNewTransMsg[iMsg].wParam;
        LPARAM lParam = pNewTransMsg[iMsg].lParam;
        if (bSend)
        {
            if (bUnicode)
                SendMessageW(hWnd, uMsg, wParam, lParam);
            else
                SendMessageA(hWnd, uMsg, wParam, lParam);
        }
        else
        {
            if (bUnicode)
                PostMessageW(hWnd, uMsg, wParam, lParam);
            else
                PostMessageA(hWnd, uMsg, wParam, lParam);
        }
    }

    ImmLocalFree(pNewTransMsg);
    ImmUnlockIMCC(pIC->hMsgBuf);
    pIC->dwNumMsgBuf = 0; /* Processed */
    ImmUnlockIMC(hIMC);

    return TRUE;
}

/***********************************************************************
 *		CtfImmHideToolbarWnd (IMM32.@)
 *
 * Used with CtfImmRestoreToolbarWnd.
 */
DWORD WINAPI
CtfImmHideToolbarWnd(VOID)
{
    ITfLangBarMgr *pBarMgr;
    DWORD dwShowFlags = 0;
    BOOL bShown;

    TRACE("()\n");

    if (FAILED(Imm32TF_CreateLangBarMgr(&pBarMgr)))
        return dwShowFlags;

    if (SUCCEEDED(pBarMgr->lpVtbl->GetShowFloatingStatus(pBarMgr, &dwShowFlags)))
    {
        bShown = !(dwShowFlags & 0x800);
        dwShowFlags &= 0xF;
        if (bShown)
            pBarMgr->lpVtbl->ShowFloating(pBarMgr, 8);
    }

    pBarMgr->lpVtbl->Release(pBarMgr);
    return dwShowFlags;
}

/***********************************************************************
 *		CtfImmRestoreToolbarWnd (IMM32.@)
 *
 * Used with CtfImmHideToolbarWnd.
 */
VOID WINAPI
CtfImmRestoreToolbarWnd(
    _In_ LPVOID pUnused,
    _In_ DWORD dwShowFlags)
{
    HRESULT hr;
    ITfLangBarMgr *pBarMgr;

    UNREFERENCED_PARAMETER(pUnused);

    TRACE("(%p, 0x%X)\n", pUnused, dwShowFlags);

    hr = Imm32TF_CreateLangBarMgr(&pBarMgr);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    if (dwShowFlags)
        pBarMgr->lpVtbl->ShowFloating(pBarMgr, dwShowFlags);

    pBarMgr->lpVtbl->Release(pBarMgr);
}

/***********************************************************************
 *		CtfImmDispatchDefImeMessage (IMM32.@)
 */
LRESULT WINAPI
CtfImmDispatchDefImeMessage(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    TRACE("(%p, %u, %p, %p)\n", hWnd, uMsg, wParam, lParam);

    if (RtlDllShutdownInProgress() || Imm32InsideLoaderLock() || !Imm32LoadCtfIme())
        return 0;

    return CTF_IME_FN(CtfImeDispatchDefImeMessage)(hWnd, uMsg, wParam, lParam);
}

/***********************************************************************
 *		CtfImmIsGuidMapEnable (IMM32.@)
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
 *		CtfImmGetGuidAtom (IMM32.@)
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
