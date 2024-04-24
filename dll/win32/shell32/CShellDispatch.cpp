/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     IShellDispatch implementation
 * COPYRIGHT:   Copyright 2015-2018 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2018 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 *              Copyright 2023 Whindmar Saksit (whindsaks@proton.me)
 */

#include "precomp.h"
#include "winsvc.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);


EXTERN_C DWORD WINAPI SHGetRestriction(LPCWSTR lpSubKey, LPCWSTR lpSubName, LPCWSTR lpValue);

static HRESULT PostTrayCommand(UINT cmd)
{
    HWND hTrayWnd = FindWindowW(L"Shell_TrayWnd", NULL);
    return hTrayWnd && PostMessageW(hTrayWnd, WM_COMMAND, cmd, 0) ? S_OK : S_FALSE;
}

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
    HRESULT hr = E_FAIL;
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
    if (!SUCCEEDED(hr))
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

    BROWSEINFOW bi = { 0 };
    bi.hwndOwner = reinterpret_cast<HWND>(LongToHandle(Hwnd));
    bi.lpszTitle = Title;
    bi.ulFlags = Options | BIF_NEWDIALOGSTYLE;

    CComHeapPtr<ITEMIDLIST> idlist;
    if (!is_optional_argument(&RootFolder) && VariantToIdlist(&RootFolder, &idlist) == S_OK)
        bi.pidlRoot = idlist;

    CComHeapPtr<ITEMIDLIST> selection;
    selection.Attach(SHBrowseForFolderW(&bi));
    if (!selection)
        return S_FALSE;

    return ShellObjectCreatorInit<CFolder>(static_cast<LPITEMIDLIST>(selection), IID_PPV_ARG(Folder, ppsdf));
}

HRESULT STDMETHODCALLTYPE CShellDispatch::Windows(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);
    return CoCreateInstance(CLSID_ShellWindows, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARG(IDispatch, ppid));
}

static HRESULT SHELL_OpenFolder(LPCITEMIDLIST pidl, LPCWSTR verb = NULL)
{
    SHELLEXECUTEINFOW sei;
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_IDLIST | SEE_MASK_FLAG_DDEWAIT;
    sei.hwnd = NULL;
    sei.lpVerb = verb;
    sei.lpFile = sei.lpParameters = sei.lpDirectory = NULL;
    sei.nShow = SW_SHOW;
    sei.lpIDList = const_cast<LPITEMIDLIST>(pidl);
    if (ShellExecuteExW(&sei))
        return S_OK;
    DWORD error = GetLastError();
    return HRESULT_FROM_WIN32(error);
}

static HRESULT OpenFolder(VARIANT vDir, LPCWSTR verb = NULL)
{
    CComHeapPtr<ITEMIDLIST> idlist;
    HRESULT hr = VariantToIdlist(&vDir, &idlist);
    if (hr == S_OK && SHELL_OpenFolder(idlist, verb) == S_OK)
    {
        return S_OK;
    }
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::Open(VARIANT vDir)
{
    TRACE("(%p, %s)\n", this, debugstr_variant(&vDir));
    return OpenFolder(vDir);
}

HRESULT STDMETHODCALLTYPE CShellDispatch::Explore(VARIANT vDir)
{
    TRACE("(%p, %s)\n", this, debugstr_variant(&vDir));
    return OpenFolder(vDir, L"explore");
}

HRESULT STDMETHODCALLTYPE CShellDispatch::MinimizeAll()
{
    TRACE("(%p)\n", this);
    return PostTrayCommand(TRAYCMD_MINIMIZE_ALL);
}

HRESULT STDMETHODCALLTYPE CShellDispatch::UndoMinimizeALL()
{
    TRACE("(%p)\n", this);
    return PostTrayCommand(TRAYCMD_RESTORE_ALL);
}

HRESULT STDMETHODCALLTYPE CShellDispatch::FileRun()
{
    TRACE("(%p)\n", this);
    return PostTrayCommand(TRAYCMD_RUN_DIALOG);
}

HRESULT STDMETHODCALLTYPE CShellDispatch::CascadeWindows()
{
    TRACE("(%p)\n", this);
    return PostTrayCommand(TRAYCMD_CASCADE);
}

HRESULT STDMETHODCALLTYPE CShellDispatch::TileVertically()
{
    TRACE("(%p)\n", this);
    return PostTrayCommand(TRAYCMD_TILE_V);
}

HRESULT STDMETHODCALLTYPE CShellDispatch::TileHorizontally()
{
    TRACE("(%p)\n", this);
    return PostTrayCommand(TRAYCMD_TILE_H);
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
    return PostTrayCommand(TRAYCMD_DATE_AND_TIME);
}

HRESULT STDMETHODCALLTYPE CShellDispatch::TrayProperties()
{
    TRACE("(%p)\n", this);
    return PostTrayCommand(TRAYCMD_TASKBAR_PROPERTIES);
}

HRESULT STDMETHODCALLTYPE CShellDispatch::Help()
{
    TRACE("(%p)\n", this);
    return PostTrayCommand(TRAYCMD_HELP_AND_SUPPORT);
}

HRESULT STDMETHODCALLTYPE CShellDispatch::FindFiles()
{
    TRACE("(%p)\n", this);
    return PostTrayCommand(TRAYCMD_SEARCH_FILES);
}

HRESULT STDMETHODCALLTYPE CShellDispatch::FindComputer()
{
    TRACE("(%p)\n", this);
    return PostTrayCommand(TRAYCMD_SEARCH_COMPUTERS);
}

HRESULT STDMETHODCALLTYPE CShellDispatch::RefreshMenu()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::ControlPanelItem(BSTR szDir)
{
    TRACE("(%p, %ls)\n", this, szDir);
    return SHRunControlPanel(szDir, NULL) ? S_OK : S_FALSE;
}


// *** IShellDispatch2 methods ***
HRESULT STDMETHODCALLTYPE CShellDispatch::IsRestricted(BSTR group, BSTR restriction, LONG *value)
{
    TRACE("(%p, %ls, %ls, %p)\n", this, group, restriction, value);

    if (!value)
        return E_INVALIDARG;
    *value = SHGetRestriction(NULL, group, restriction);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::ShellExecute(BSTR file, VARIANT v_args, VARIANT v_dir, VARIANT v_op, VARIANT v_show)
{
    CComVariant args_str, dir_str, op_str, show_int;
    WCHAR *args = NULL, *dir = NULL, *op = NULL;
    INT show = SW_SHOW;
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

    if (!lstrcmpiW(name, L"ProcessorArchitecture"))
    {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        V_VT(ret) = VT_I4;
        V_UI4(ret) = si.wProcessorArchitecture;
        return S_OK;
    }

    UINT os = 0;
    if (!lstrcmpiW(name, L"IsOS_Professional"))
        os = OS_PROFESSIONAL;
    else if (!lstrcmpiW(name, L"IsOS_Personal"))
        os = OS_HOME;
    else if (!lstrcmpiW(name, L"IsOS_DomainMember"))
        os = OS_DOMAINMEMBER;
    if (os)
    {
        V_VT(ret) = VT_BOOL;
        V_BOOL(ret) = IsOS(os) ? VARIANT_TRUE : VARIANT_FALSE;
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT OpenServiceHelper(LPCWSTR name, DWORD access, SC_HANDLE &hSvc)
{
    hSvc = NULL;
    SC_HANDLE hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hScm)
        return HResultFromWin32(GetLastError());
    HRESULT hr = S_OK;
    hSvc = OpenServiceW(hScm, name, access);
    if (!hSvc)
        hr = HResultFromWin32(GetLastError());
    CloseServiceHandle(hScm);
    return hr;
}

static HRESULT SHELL32_ControlService(BSTR name, DWORD control, VARIANT &persistent)
{
    BOOL persist = V_VT(&persistent) == VT_BOOL && V_BOOL(&persistent);
    DWORD access = persist ? SERVICE_CHANGE_CONFIG : 0;
    switch (control)
    {
        case 0:
            access |= SERVICE_START;
            break;
        case SERVICE_CONTROL_STOP:
            access |= SERVICE_STOP;
            break;
    }
    SC_HANDLE hSvc;
    HRESULT hr = OpenServiceHelper(name, access, hSvc);
    if (SUCCEEDED(hr))
    {
        BOOL success;
        DWORD error, already;
        if (control)
        {
            SERVICE_STATUS ss;
            success = ControlService(hSvc, control, &ss);
            error = GetLastError();
            already = ERROR_SERVICE_NOT_ACTIVE;
        }
        else
        {
            success = StartService(hSvc, 0, NULL);
            error = GetLastError();
            already = ERROR_SERVICE_ALREADY_RUNNING;
        }
        hr = success ? S_OK : error == already ? S_FALSE : HRESULT_FROM_WIN32(error);
        if (SUCCEEDED(hr) && persist)
        {
            ChangeServiceConfigW(hSvc, SERVICE_NO_CHANGE,
                                 control ? SERVICE_DEMAND_START : SERVICE_AUTO_START,
                                 SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        }
        CloseServiceHandle(hSvc);
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::ServiceStart(BSTR service, VARIANT persistent, VARIANT *ret)
{
    TRACE("(%p, %ls, %s, %p)\n", this, service, wine_dbgstr_variant(&persistent), ret);

    HRESULT hr = SHELL32_ControlService(service, 0, persistent);
    V_VT(ret) = VT_BOOL;
    V_BOOL(ret) = (hr == S_OK ? VARIANT_TRUE : VARIANT_FALSE);
    return hr == S_OK ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::ServiceStop(BSTR service, VARIANT persistent, VARIANT *ret)
{
    TRACE("(%p, %ls, %s, %p)\n", this, service, wine_dbgstr_variant(&persistent), ret);

    HRESULT hr = SHELL32_ControlService(service, SERVICE_CONTROL_STOP, persistent);
    V_VT(ret) = VT_BOOL;
    V_BOOL(ret) = (hr == S_OK ? VARIANT_TRUE : VARIANT_FALSE);
    return hr == S_OK ? S_OK : S_FALSE;
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

    SC_HANDLE hSvc;
    HRESULT hr = OpenServiceHelper(service, SERVICE_START | SERVICE_STOP, hSvc);
    if (SUCCEEDED(hr))
        CloseServiceHandle(hSvc);
    V_VT(ret) = VT_BOOL;
    V_BOOL(ret) = (hr == S_OK ? VARIANT_TRUE : VARIANT_FALSE);
    return S_OK;
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

    CComHeapPtr<ITEMIDLIST> idlist;
    HRESULT hr = VariantToIdlist(&file, &idlist);
    if (hr == S_OK)
        SHAddToRecentDocs(SHARD_PIDL, (LPCITEMIDLIST)idlist);
    else
        hr = S_FALSE;
    return hr;
}


// *** IShellDispatch4 methods ***
#define IDM_SECURITY 5001 // From base/shell/explorer/resource.h
HRESULT STDMETHODCALLTYPE CShellDispatch::WindowsSecurity()
{
    TRACE("(%p)\n", this);
    return PostTrayCommand(IDM_SECURITY);
}

HRESULT STDMETHODCALLTYPE CShellDispatch::ToggleDesktop()
{
    TRACE("(%p)\n", this);
    return PostTrayCommand(TRAYCMD_TOGGLE_DESKTOP);
}

HRESULT STDMETHODCALLTYPE CShellDispatch::ExplorerPolicy(BSTR policy, VARIANT *value)
{
    TRACE("(%p, %ls, %p)\n", this, policy, value);
    return E_NOTIMPL;
}

#ifndef SSF_SERVERADMINUI
#define SSF_SERVERADMINUI 4
#endif
HRESULT STDMETHODCALLTYPE CShellDispatch::GetSetting(LONG setting, VARIANT_BOOL *result)
{
    TRACE("(%p, %lu, %p)\n", this, setting, result);

    int flag = -1;
    SHELLSTATE ss = { };
    SHGetSetSettings(&ss, setting, FALSE);
    switch (setting)
    {
        case SSF_SHOWALLOBJECTS:   flag = ss.fShowAllObjects;     break;
        case SSF_SHOWEXTENSIONS:   flag = ss.fShowExtensions;     break;
        case SSF_SHOWSYSFILES:     flag = ss.fShowSysFiles;       break;
        case SSF_DONTPRETTYPATH:   flag = ss.fDontPrettyPath;     break;
        case SSF_NOCONFIRMRECYCLE: flag = ss.fNoConfirmRecycle;   break;
        case SSF_SHOWSUPERHIDDEN:  flag = ss.fShowSuperHidden;    break;
        case SSF_SEPPROCESS:       flag = ss.fSepProcess;         break;
        case SSF_STARTPANELON:     flag = ss.fStartPanelOn;       break;
        case SSF_SERVERADMINUI:    flag = IsOS(OS_SERVERADMINUI); break;
    }
    if (flag >= 0)
    {
        *result = flag ? VARIANT_TRUE : VARIANT_FALSE;
        return S_OK;
    }

    return S_FALSE;
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

