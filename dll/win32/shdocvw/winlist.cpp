/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     CLSID_ShellWindows and WinList_* functions
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "objects.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

static DWORD g_dwWinListCFCookie = 0;
static IShellWindows *g_pShellWindows = NULL;

static BOOL
IsExplorerProcess(VOID)
{
    static INT bValue = -1;
    if (bValue == -1)
        bValue = !!GetModuleHandleW(L"explorer.exe");
    return bValue;
}

//////////////////////////////////////////////////////////////////////////
//   OLE Automation for scripting

static VARIANT g_vaEmpty = { VT_EMPTY };

static HRESULT
InitVariantFromBuffer(
    _Out_ LPVARIANTARG pvarg,
    _In_ LPCVOID pv,
    _In_ SIZE_T cb)
{
    VariantInit(pvarg);

    LPSAFEARRAY pArray = SafeArrayCreateVector(VT_UI1, 0, cb);
    if (!pArray)
    {
        ERR("!pArray\n");
        return E_OUTOFMEMORY;
    }

    V_ARRAY(pvarg) = pArray;
    V_VT(pvarg) = VT_ARRAY | VT_UI1;
    CopyMemory(pArray->pvData, pv, cb);
    return S_OK;
}

static HRESULT
InitVariantFromIDList(
    _Out_ LPVARIANTARG pvarg,
    _In_ LPCITEMIDLIST pidl)
{
    return InitVariantFromBuffer(pvarg, pidl, ILGetSize(pidl));
}

//////////////////////////////////////////////////////////////////////////
//   Marshaling
//
// NOTE: GIT stands for "Global Interface Table"

static HRESULT
MarshalToGIT(
    _In_ IUnknown *pUnknown,
    _In_ REFIID riid,
    _Out_ PDWORD pdwCookie)
{
    *pdwCookie = 0;

    CComPtr<IGlobalInterfaceTable> pTable;
    HRESULT hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable, 0, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARG(IGlobalInterfaceTable, &pTable));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    return pTable->RegisterInterfaceInGlobal(pUnknown, riid, pdwCookie);
}

static VOID
RevokeFromGIT(_In_ DWORD dwCookie)
{
    if (dwCookie == MAXDWORD)
        return;

    CComPtr<IGlobalInterfaceTable> pTable;
    HRESULT hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable, 0, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARG(IGlobalInterfaceTable, &pTable));
    if (FAILED_UNEXPECTEDLY(hr))
        return;
    pTable->RevokeInterfaceFromGlobal(dwCookie);
}

static HRESULT
UnMarshalFromGIT(
    _In_ DWORD dwCookie,
    _In_ REFIID riid,
    _Out_ LPVOID *ppv)
{
    CComPtr<IGlobalInterfaceTable> pTable;
    HRESULT hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable, 0, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARG(IGlobalInterfaceTable, &pTable));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    return pTable->GetInterfaceFromGlobal(dwCookie, riid, ppv);
}

EXTERN_C HRESULT
ShellWindowsGetClassObject(
    _In_ REFIID rclsid,
    _In_ REFIID riid,
    _Out_ LPVOID *ppv)
{
    if (rclsid != CLSID_ShellWindows)
        return CLASS_E_CLASSNOTAVAILABLE;

    if (IsExplorerProcess())
        return CoGetClassObject(CLSID_ShellWindows, CLSCTX_LOCAL_SERVER, NULL, riid, ppv);

    return UnMarshalFromGIT(g_dwWinListCFCookie, riid, ppv);
}

//////////////////////////////////////////////////////////////////////////
//   CShellWindowListCF class --- Shell window list class factory

class CShellWindowListCF : public CComClassFactory
{
public:
    CShellWindowListCF();
    virtual ~CShellWindowListCF();

    HRESULT Init();
    HRESULT Register();
    VOID UnRegister();

    // IUnknown methods will be populated by ShellObjectCreator

    // *** IClassFactory methods ***
    STDMETHODIMP CreateInstance(
        _In_ IUnknown *pUnkOuter,
        _In_ REFIID riid,
        _Out_ void **ppvObject) override;
    STDMETHODIMP LockServer(_In_ BOOL fLock) override { return S_OK; }

    DECLARE_CLASSFACTORY_EX(CShellWindowListCF)
    DECLARE_NOT_AGGREGATABLE(CShellWindowListCF)

    BEGIN_COM_MAP(CComClassFactory)
        COM_INTERFACE_ENTRY_IID(IID_IClassFactory, IClassFactory)
    END_COM_MAP()

protected:
    CComPtr<IShellWindows> m_pShellWindows;
};

static CShellWindowListCF *g_pcfWinList = NULL;

//////////////////////////////////////////////////////////////////////////
//   CShellWindowListCF methods

CShellWindowListCF::CShellWindowListCF()
{
    SHDOCVW_LockModule();
}

CShellWindowListCF::~CShellWindowListCF()
{
    if (g_pShellWindows)
        g_pShellWindows = NULL;
    m_pShellWindows = NULL;
    SHDOCVW_UnlockModule();
}

HRESULT
CShellWindowListCF::Init()
{
    return CSDWindows_CreateInstance(&m_pShellWindows);
}

HRESULT
CShellWindowListCF::Register()
{
    if (IsExplorerProcess())
        return CoRegisterClassObject(CLSID_ShellWindows, this,
                                     CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
                                     REGCLS_MULTIPLEUSE, &g_dwWinListCFCookie);

    return MarshalToGIT(this, IID_IClassFactory, &g_dwWinListCFCookie);
}

VOID
CShellWindowListCF::UnRegister(VOID)
{
    if (!g_dwWinListCFCookie)
        return;

    if (IsExplorerProcess())
        CoRevokeClassObject(g_dwWinListCFCookie);
    else
        RevokeFromGIT(g_dwWinListCFCookie);

    g_dwWinListCFCookie = 0;
}

STDMETHODIMP
CShellWindowListCF::CreateInstance(
    _In_ IUnknown *pUnkOuter,
    _In_ REFIID riid,
    _Out_ void **ppvObject)
{
    if (!m_pShellWindows)
        return E_FAIL;
    return m_pShellWindows->QueryInterface(riid, ppvObject);
}

/*************************************************************************
 *    WinList_Init (SHDOCVW.110)
 *
 * Retired in NT 6.1.
 */
EXTERN_C
BOOL WINAPI
WinList_Init(VOID)
{
    TRACE("()\n");

    if (!IsExplorerProcess() && GetShellWindow())
        return FALSE;

    CComPtr<CShellWindowListCF> pWinListCF;
    HRESULT hr = ShellObjectCreator(pWinListCF);
    if (FAILED_UNEXPECTEDLY(hr))
        return FALSE;

    hr = pWinListCF->Init();
    if (FAILED_UNEXPECTEDLY(hr))
        return FALSE;

    pWinListCF->Register();
    g_pcfWinList = pWinListCF.Detach();
    g_pcfWinList->AddRef();

    return TRUE;
}

/*************************************************************************
 *    WinList_Terminate (SHDOCVW.111)
 *
 * NT 4.71 and higher. Retired in NT 6.1.
 */
EXTERN_C
VOID WINAPI
WinList_Terminate(VOID)
{
    TRACE("()\n");

    if (g_pcfWinList)
    {
        g_pcfWinList->UnRegister();
        g_pcfWinList->Release();
        g_pcfWinList = NULL;
    }

    if (g_pShellWindows)
    {
        g_pShellWindows->Release();
        g_pShellWindows = NULL;
    }
}

/*************************************************************************
 *    WinList_GetShellWindows (SHDOCVW.179)
 *
 * NT 5.0 and higher.
 * @see https://learn.microsoft.com/en-us/windows/win32/api/exdisp/nn-exdisp-ishellwindows
 */
EXTERN_C
IShellWindows* WINAPI
WinList_GetShellWindows(
    _In_ BOOL bCreate)
{
    TRACE("(%d)\n", bCreate);

    HRESULT hr;

    if (g_pShellWindows)
    {
        g_pShellWindows->AddRef();
        return g_pShellWindows;
    }

    if (!bCreate && g_pcfWinList)
    {
        hr = g_pcfWinList->CreateInstance(NULL, IID_PPV_ARG(IShellWindows, &g_pShellWindows));
        if (!FAILED_UNEXPECTEDLY(hr))
            return g_pShellWindows;
    }

    hr = CoCreateInstance(CLSID_ShellWindows, NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
                          IID_PPV_ARG(IShellWindows, &g_pShellWindows));
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

    return g_pShellWindows;
}

/*************************************************************************
 *    WinList_NotifyNewLocation (SHDOCVW.177)
 *
 * NT 5.0 and higher.
 * @see https://learn.microsoft.com/en-us/windows/win32/api/exdisp/nf-exdisp-ishellwindows-onnavigate
 */
EXTERN_C
HRESULT WINAPI
WinList_NotifyNewLocation(
    _In_ IShellWindows *pShellWindows,
    _In_ LONG lCookie,
    _In_ LPCITEMIDLIST pidl)
{
    TRACE("(%p, %ld, %p)\n", pShellWindows, lCookie, pidl);

    if (!pidl)
    {
        ERR("!pidl\n");
        return E_UNEXPECTED;
    }

    VARIANTARG varg;
    HRESULT hr = InitVariantFromIDList(&varg, pidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pShellWindows->OnNavigate(lCookie, &varg);
    SafeArrayDestroy(V_ARRAY(&varg));
    return hr;
}

/*************************************************************************
 *    WinList_FindFolderWindow (SHDOCVW.178)
 *
 * NT 5.0 and higher.
 * @see https://learn.microsoft.com/en-us/windows/win32/api/exdisp/nf-exdisp-ishellwindows-findwindowsw
 */
EXTERN_C
HRESULT WINAPI
WinList_FindFolderWindow(
    _In_ LPCITEMIDLIST pidl,
    _In_ DWORD dwUnused,
    _Out_opt_ PLONG phwnd, // Stores a window handle but LONG type
    _Out_opt_ IWebBrowserApp **ppWebBrowserApp)
{
    UNREFERENCED_PARAMETER(dwUnused);

    TRACE("(%p, %ld, %p, %p)\n", pidl, dwUnused, phwnd, ppWebBrowserApp);

    if (ppWebBrowserApp)
        *ppWebBrowserApp = NULL;

    if (phwnd)
        *phwnd = 0;

    if (!pidl)
    {
        ERR("!pidl\n");
        return E_UNEXPECTED;
    }

    CComPtr<IShellWindows> pShellWindows(WinList_GetShellWindows(ppWebBrowserApp != NULL));
    if (!pShellWindows)
    {
        ERR("!pShellWindows\n");
        return E_UNEXPECTED;
    }

    VARIANTARG varg;
    HRESULT hr = InitVariantFromIDList(&varg, pidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IDispatch> pDispatch;
    const INT options = SWFO_INCLUDEPENDING | (ppWebBrowserApp ? SWFO_NEEDDISPATCH : 0);
    hr = pShellWindows->FindWindowSW(&varg, &g_vaEmpty, SWC_BROWSER, phwnd, options, &pDispatch);
    if (pDispatch && ppWebBrowserApp)
        hr = pDispatch->QueryInterface(IID_PPV_ARG(IWebBrowserApp, ppWebBrowserApp));

    SafeArrayDestroy(V_ARRAY(&varg));
    return hr;
}

/*************************************************************************
 *    WinList_RegisterPending (SHDOCVW.180)
 *
 * NT 5.0 and higher.
 * @see https://learn.microsoft.com/en-us/windows/win32/api/exdisp/nf-exdisp-ishellwindows-registerpending
 */
EXTERN_C
HRESULT WINAPI
WinList_RegisterPending(
    _In_ DWORD dwThreadId,
    _In_ LPCITEMIDLIST pidl,
    _In_ DWORD dwUnused,
    _Out_ PLONG plCookie)
{
    TRACE("(%ld, %p, %ld, %p)\n", dwThreadId, pidl, dwUnused, plCookie);

    if (!pidl)
    {
        ERR("!pidl\n");
        return E_UNEXPECTED;
    }

    CComPtr<IShellWindows> pShellWindows(WinList_GetShellWindows(FALSE));
    if (!pShellWindows)
    {
        ERR("!pShellWindows\n");
        return E_UNEXPECTED;
    }

    VARIANTARG varg;
    HRESULT hr = InitVariantFromIDList(&varg, pidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pShellWindows->RegisterPending(dwThreadId, &varg, &g_vaEmpty, SWC_BROWSER, plCookie);
    SafeArrayDestroy(V_ARRAY(&varg));
    return hr;
}

/*************************************************************************
 *    WinList_Revoke (SHDOCVW.181)
 *
 * NT 5.0 and higher.
 * @see https://learn.microsoft.com/en-us/windows/win32/api/exdisp/nf-exdisp-ishellwindows-revoke
 */
EXTERN_C
HRESULT WINAPI
WinList_Revoke(
    _In_ LONG lCookie)
{
    TRACE("(%ld)\n", lCookie);

    CComPtr<IShellWindows> pShellWindows(WinList_GetShellWindows(TRUE));
    if (!pShellWindows)
    {
        ERR("!pShellWindows\n");
        return E_FAIL;
    }

    return pShellWindows->Revoke(lCookie);
}
