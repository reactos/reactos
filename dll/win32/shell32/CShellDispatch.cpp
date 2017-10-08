/*
 * IShellDispatch implementation
 *
 * Copyright 2015 Mark Jansen
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

#include "precomp.h"

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
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::get_Parent(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);
    return E_NOTIMPL;
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
    LPITEMIDLIST idlist = NULL;
    HRESULT hr = VariantToIdlist(&vDir, &idlist);
    if (!SUCCEEDED(hr) || !idlist)
        return S_FALSE;
    CFolder* fld = new CComObject<CFolder>();
    fld->Init(idlist);
    *ppsdf = fld;
    fld->AddRef();
    return hr;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::BrowseForFolder(LONG Hwnd, BSTR Title, LONG Options, VARIANT RootFolder, Folder **ppsdf)
{
    TRACE("(%p, %lu, %ls, %lu, %s, %p)\n", this, Hwnd, Title, Options, debugstr_variant(&RootFolder), ppsdf);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellDispatch::Windows(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);
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

HRESULT STDMETHODCALLTYPE CShellDispatch::ShellExecute(BSTR file, VARIANT args, VARIANT dir, VARIANT op, VARIANT show)
{
    TRACE("(%p, %ls, %s, %s, %s, %s)\n", this, file, debugstr_variant(&args), debugstr_variant(&dir), debugstr_variant(&op), debugstr_variant(&show));
    return E_NOTIMPL;
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

HRESULT STDMETHODCALLTYPE CShellDispatch::IsServiceRunning(BSTR service, VARIANT *running)
{
    TRACE("(%p, %ls, %p)\n", this, service, running);
    return E_NOTIMPL;
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
    return E_NOTIMPL;
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

