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


CShell::CShell()
{
}

CShell::~CShell()
{
}

HRESULT CShell::Initialize()
{
    return S_OK;
}

// *** IShellDispatch methods ***
HRESULT STDMETHODCALLTYPE CShell::get_Application(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::get_Parent(IDispatch **ppid)
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

HRESULT STDMETHODCALLTYPE CShell::NameSpace(VARIANT vDir, Folder **ppsdf)
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

HRESULT STDMETHODCALLTYPE CShell::BrowseForFolder(LONG Hwnd, BSTR Title, LONG Options, VARIANT RootFolder, Folder **ppsdf)
{
    TRACE("(%p, %lu, %ls, %lu, %s, %p)\n", this, Hwnd, Title, Options, debugstr_variant(&RootFolder), ppsdf);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::Windows(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::Open(VARIANT vDir)
{
    TRACE("(%p, %s)\n", this, debugstr_variant(&vDir));
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::Explore(VARIANT vDir)
{
    TRACE("(%p, %s)\n", this, debugstr_variant(&vDir));
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::MinimizeAll()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::UndoMinimizeALL()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::FileRun()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::CascadeWindows()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::TileVertically()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::TileHorizontally()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::ShutdownWindows()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::Suspend()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::EjectPC()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::SetTime()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::TrayProperties()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::Help()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::FindFiles()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::FindComputer()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::RefreshMenu()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::ControlPanelItem(BSTR szDir)
{
    TRACE("(%p, %ls)\n", this, szDir);
    return E_NOTIMPL;
}


// *** IShellDispatch2 methods ***
HRESULT STDMETHODCALLTYPE CShell::IsRestricted(BSTR group, BSTR restriction, LONG *value)
{
    TRACE("(%p, %ls, %ls, %p)\n", this, group, restriction, value);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::ShellExecute(BSTR file, VARIANT args, VARIANT dir, VARIANT op, VARIANT show)
{
    TRACE("(%p, %ls, %s, %s, %s, %s)\n", this, file, debugstr_variant(&args), debugstr_variant(&dir), debugstr_variant(&op), debugstr_variant(&show));
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::FindPrinter(BSTR name, BSTR location, BSTR model)
{
    TRACE("(%p, %ls, %ls, %ls)\n", this, name, location, model);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::GetSystemInformation(BSTR name, VARIANT *ret)
{
    TRACE("(%p, %ls, %p)\n", this, name, ret);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::ServiceStart(BSTR service, VARIANT persistent, VARIANT *ret)
{
    TRACE("(%p, %ls, %s, %p)\n", this, service, wine_dbgstr_variant(&persistent), ret);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::ServiceStop(BSTR service, VARIANT persistent, VARIANT *ret)
{
    TRACE("(%p, %ls, %s, %p)\n", this, service, wine_dbgstr_variant(&persistent), ret);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::IsServiceRunning(BSTR service, VARIANT *running)
{
    TRACE("(%p, %ls, %p)\n", this, service, running);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::CanStartStopService(BSTR service, VARIANT *ret)
{
    TRACE("(%p, %ls, %p)\n", this, service, ret);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::ShowBrowserBar(BSTR clsid, VARIANT show, VARIANT *ret)
{
    TRACE("(%p, %ls, %s, %p)\n", this, clsid, wine_dbgstr_variant(&show), ret);
    return E_NOTIMPL;
}


// *** IShellDispatch3 methods ***
HRESULT STDMETHODCALLTYPE CShell::AddToRecent(VARIANT file, BSTR category)
{
    TRACE("(%p, %s, %ls)\n", this, wine_dbgstr_variant(&file), category);
    return E_NOTIMPL;
}


// *** IShellDispatch4 methods ***
HRESULT STDMETHODCALLTYPE CShell::WindowsSecurity()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::ToggleDesktop()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::ExplorerPolicy(BSTR policy, VARIANT *value)
{
    TRACE("(%p, %ls, %p)\n", this, policy, value);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::GetSetting(LONG setting, VARIANT_BOOL *result)
{
    TRACE("(%p, %lu, %p)\n", this, setting, result);
    return E_NOTIMPL;
}


// *** IObjectSafety methods ***
HRESULT STDMETHODCALLTYPE CShell::GetInterfaceSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    TRACE("(%p, %s, %p, %p)\n", this, wine_dbgstr_guid(&riid), pdwSupportedOptions, pdwEnabledOptions);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    TRACE("(%p, %s, %lu, %lu)\n", this, wine_dbgstr_guid(&riid), dwOptionSetMask, dwEnabledOptions);
    return E_NOTIMPL;
}


// *** IObjectWithSite methods ***
HRESULT STDMETHODCALLTYPE CShell::SetSite(IUnknown *pUnkSite)
{
    TRACE("(%p, %p)\n", this, pUnkSite);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShell::GetSite(REFIID riid, PVOID *ppvSite)
{
    TRACE("(%p, %s, %p)\n", this, wine_dbgstr_guid(&riid), ppvSite);
    return E_NOTIMPL;
}

HRESULT WINAPI CShell_Constructor(REFIID riid, LPVOID * ppvOut)
{
    return ShellObjectCreatorInit<CShell>(riid, ppvOut);
}

