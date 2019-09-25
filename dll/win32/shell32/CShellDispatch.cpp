/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     IShellDispatch implementation
 * COPYRIGHT:   Copyright 2015-2018 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2018 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"
#include "winsvc.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);


CShellDispatch::CShellDispatch()
{
}

CShellDispatch::~CShellDispatch()
{
}

HRESULT CShellDispatch::Initialize()
{
    return S_OK;
}

// *** IShellDispatch methods ***
HRESULT STDMETHODCALLTYPE CShellDispatch::get_Application(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);

    if (!ppid)
        return E_INVALIDARG;

    *ppid = this;
    AddRef();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::get_Parent(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);

    if (ppid)
    {
        *ppid = static_cast<IDispatch*>(this);
        AddRef();
    }

    return S_OK;
}

HRESULT VariantToIdlist(VARIANT* var, LPITEMIDLIST* idlist)
{
    HRESULT hr = S_FALSE;
    if(V_VT(var) == VT_I4)
    {
        hr = SHGetSpecialFolderLocation(NULL, V_I4(var), idlist);
    }
    else if(V_VT(var) == VT_BSTR)
    {
        hr = SHILCreateFromPathW(V_BSTR(var), idlist, NULL);
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::NameSpace(VARIANT vDir, Folder **ppsdf)
{
    TRACE("(%p, %s, %p)\n", this, debugstr_variant(&vDir), ppsdf);
    if (!ppsdf)
        return E_POINTER;
    *ppsdf = NULL;
    HRESULT hr;

    if (V_VT(&vDir) == VT_I2)
    {
        hr = VariantChangeType(&vDir, &vDir, 0, VT_I4);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    CComHeapPtr<ITEMIDLIST> idlist;
    hr = VariantToIdlist(&vDir, &idlist);
    if (!SUCCEEDED(hr) || !idlist)
        return S_FALSE;

    return ShellObjectCreatorInit<CFolder>(static_cast<LPITEMIDLIST>(idlist), IID_PPV_ARG(Folder, ppsdf));
}

static BOOL is_optional_argument(const VARIANT *arg)
{
    return V_VT(arg) == VT_ERROR && V_ERROR(arg) == DISP_E_PARAMNOTFOUND;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::BrowseForFolder(LONG Hwnd, BSTR Title, LONG Options, VARIANT RootFolder, Folder **ppsdf)
{
    TRACE("(%p, %lu, %ls, %lu, %s, %p)\n", this, Hwnd, Title, Options, debugstr_variant(&RootFolder), ppsdf);

    *ppsdf = NULL;

    if (!is_optional_argument(&RootFolder))
        FIXME("root folder is ignored\n");

    BROWSEINFOW bi = { 0 };
    bi.hwndOwner = reinterpret_cast<HWND>(LongToHandle(Hwnd));
    bi.lpszTitle = Title;
    bi.ulFlags = Options;

    CComHeapPtr<ITEMIDLIST> selection;
    selection.Attach(SHBrowseForFolderW(&bi));
    if (!selection)
        return S_FALSE;

    return ShellObjectCreatorInit<CFolder>(static_cast<LPITEMIDLIST>(selection), IID_PPV_ARG(Folder, ppsdf));
}

HRESULT STDMETHODCALLTYPE CShellDispatch::Windows(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);

    *ppid = NULL;

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::Open(VARIANT vDir)
{
    TRACE("(%p, %s)\n", this, debugstr_variant(&vDir));
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::Explore(VARIANT vDir)
{
    TRACE("(%p, %s)\n", this, debugstr_variant(&vDir));
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::MinimizeAll()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::UndoMinimizeALL()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::FileRun()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::CascadeWindows()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::TileVertically()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::TileHorizontally()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::ShutdownWindows()
{
    ExitWindowsDialog(NULL);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::Suspend()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::EjectPC()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::SetTime()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::TrayProperties()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::Help()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::FindFiles()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::FindComputer()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::RefreshMenu()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::ControlPanelItem(BSTR szDir)
{
    TRACE("(%p, %ls)\n", this, szDir);
    return E_NOTIMPL;
}


// *** IShellDispatch2 methods ***
HRESULT STDMETHODCALLTYPE CShellDispatch::IsRestricted(BSTR group, BSTR restriction, LONG *value)
{
    TRACE("(%p, %ls, %ls, %p)\n", this, group, restriction, value);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::ShellExecute(BSTR file, VARIANT v_args, VARIANT v_dir, VARIANT v_op, VARIANT v_show)
{
    CComVariant args_str, dir_str, op_str, show_int;
    WCHAR *args = NULL, *dir = NULL, *op = NULL;
    INT show = 0;
    HINSTANCE ret;

    TRACE("(%s, %s, %s, %s, %s)\n", debugstr_w(file), debugstr_variant(&v_args),
            debugstr_variant(&v_dir), debugstr_variant(&v_op), debugstr_variant(&v_show));

    args_str.ChangeType(VT_BSTR, &v_args);
    if (V_VT(&args_str) == VT_BSTR)
        args = V_BSTR(&args_str);

    dir_str.ChangeType(VT_BSTR, &v_dir);
    if (V_VT(&dir_str) == VT_BSTR)
        dir = V_BSTR(&dir_str);

    op_str.ChangeType(VT_BSTR, &v_op);
    if (V_VT(&op_str) == VT_BSTR)
        op = V_BSTR(&op_str);

    show_int.ChangeType(VT_I4, &v_show);
    if (V_VT(&show_int) == VT_I4)
        show = V_I4(&show_int);

    ret = ShellExecuteW(NULL, op, file, args, dir, show);

    return (ULONG_PTR)ret > 32 ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::FindPrinter(BSTR name, BSTR location, BSTR model)
{
    TRACE("(%p, %ls, %ls, %ls)\n", this, name, location, model);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::GetSystemInformation(BSTR name, VARIANT *ret)
{
    TRACE("(%p, %ls, %p)\n", this, name, ret);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::ServiceStart(BSTR service, VARIANT persistent, VARIANT *ret)
{
    TRACE("(%p, %ls, %s, %p)\n", this, service, wine_dbgstr_variant(&persistent), ret);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::ServiceStop(BSTR service, VARIANT persistent, VARIANT *ret)
{
    TRACE("(%p, %ls, %s, %p)\n", this, service, wine_dbgstr_variant(&persistent), ret);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::IsServiceRunning(BSTR name, VARIANT *running)
{
    SERVICE_STATUS_PROCESS status;
    SC_HANDLE scm, service;
    DWORD dummy;

    TRACE("(%s, %p)\n", debugstr_w(name), running);

    V_VT(running) = VT_BOOL;
    V_BOOL(running) = VARIANT_FALSE;

    scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm)
    {
        ERR("failed to connect to service manager\n");
        return S_OK;
    }

    service = OpenServiceW(scm, name, SERVICE_QUERY_STATUS);
    if (!service)
    {
        ERR("Failed to open service %s (%u)\n", debugstr_w(name), GetLastError());
        CloseServiceHandle(scm);
        return S_OK;
    }

    if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (BYTE *)&status,
                              sizeof(SERVICE_STATUS_PROCESS), &dummy))
    {
        TRACE("failed to query service status (%u)\n", GetLastError());
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return S_OK;
    }

    if (status.dwCurrentState == SERVICE_RUNNING)
        V_BOOL(running) = VARIANT_TRUE;

    CloseServiceHandle(service);
    CloseServiceHandle(scm);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::CanStartStopService(BSTR service, VARIANT *ret)
{
    TRACE("(%p, %ls, %p)\n", this, service, ret);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::ShowBrowserBar(BSTR clsid, VARIANT show, VARIANT *ret)
{
    TRACE("(%p, %ls, %s, %p)\n", this, clsid, wine_dbgstr_variant(&show), ret);
    return E_NOTIMPL;
}


// *** IShellDispatch3 methods ***
HRESULT STDMETHODCALLTYPE CShellDispatch::AddToRecent(VARIANT file, BSTR category)
{
    TRACE("(%p, %s, %ls)\n", this, wine_dbgstr_variant(&file), category);
    return E_NOTIMPL;
}


// *** IShellDispatch4 methods ***
HRESULT STDMETHODCALLTYPE CShellDispatch::WindowsSecurity()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::ToggleDesktop()
{
    TRACE("(%p)\n", this);

    HWND hTrayWnd = FindWindowW(L"Shell_TrayWnd", NULL);
    PostMessageW(hTrayWnd, WM_COMMAND, TRAYCMD_TOGGLE_DESKTOP, 0);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::ExplorerPolicy(BSTR policy, VARIANT *value)
{
    TRACE("(%p, %ls, %p)\n", this, policy, value);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::GetSetting(LONG setting, VARIANT_BOOL *result)
{
    TRACE("(%p, %lu, %p)\n", this, setting, result);
    return E_NOTIMPL;
}


// *** IObjectSafety methods ***
HRESULT STDMETHODCALLTYPE CShellDispatch::GetInterfaceSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    TRACE("(%p, %s, %p, %p)\n", this, wine_dbgstr_guid(&riid), pdwSupportedOptions, pdwEnabledOptions);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    TRACE("(%p, %s, %lu, %lu)\n", this, wine_dbgstr_guid(&riid), dwOptionSetMask, dwEnabledOptions);
    return E_NOTIMPL;
}


// *** IObjectWithSite methods ***
HRESULT STDMETHODCALLTYPE CShellDispatch::SetSite(IUnknown *pUnkSite)
{
    TRACE("(%p, %p)\n", this, pUnkSite);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::GetSite(REFIID riid, PVOID *ppvSite)
{
    TRACE("(%p, %s, %p)\n", this, wine_dbgstr_guid(&riid), ppvSite);
    return E_NOTIMPL;
}

HRESULT WINAPI CShellDispatch_Constructor(REFIID riid, LPVOID * ppvOut)
{
    return ShellObjectCreatorInit<CShellDispatch>(riid, ppvOut);
}

