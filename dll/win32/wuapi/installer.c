/*
 * IUpdateInstaller implementation
 *
 * Copyright 2008 Hans Leidekker
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "wuapi.h"
#include "wuapi_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wuapi);

typedef struct _update_installer
{
    IUpdateInstaller IUpdateInstaller_iface;
    LONG refs;
} update_installer;

static inline update_installer *impl_from_IUpdateInstaller( IUpdateInstaller *iface )
{
    return CONTAINING_RECORD(iface, update_installer, IUpdateInstaller_iface);
}

static ULONG WINAPI update_installer_AddRef(
    IUpdateInstaller *iface )
{
    update_installer *update_installer = impl_from_IUpdateInstaller( iface );
    return InterlockedIncrement( &update_installer->refs );
}

static ULONG WINAPI update_installer_Release(
    IUpdateInstaller *iface )
{
    update_installer *update_installer = impl_from_IUpdateInstaller( iface );
    LONG refs = InterlockedDecrement( &update_installer->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", update_installer);
        HeapFree( GetProcessHeap(), 0, update_installer );
    }
    return refs;
}

static HRESULT WINAPI update_installer_QueryInterface(
    IUpdateInstaller *iface,
    REFIID riid,
    void **ppvObject )
{
    update_installer *This = impl_from_IUpdateInstaller( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IUpdateInstaller ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IUpdateInstaller_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI update_installer_GetTypeInfoCount(
    IUpdateInstaller *iface,
    UINT *pctinfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_GetTypeInfo(
    IUpdateInstaller *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_GetIDsOfNames(
    IUpdateInstaller *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_Invoke(
    IUpdateInstaller *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_get_ClientApplicationID(
    IUpdateInstaller *This,
    BSTR *retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_put_ClientApplicationID(
    IUpdateInstaller *This,
    BSTR value )
{
    FIXME("%p, %s\n", This, debugstr_w(value));
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_get_IsForced(
    IUpdateInstaller *This,
    VARIANT_BOOL *retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_put_IsForced(
    IUpdateInstaller *This,
    VARIANT_BOOL value )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_get_ParentHwnd(
    IUpdateInstaller *This,
    HWND *retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_put_ParentHwnd(
    IUpdateInstaller *This,
    HWND value )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_put_ParentWindow(
    IUpdateInstaller *This,
    IUnknown *value )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_get_ParentWindow(
    IUpdateInstaller *This,
    IUnknown **retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_get_Updates(
    IUpdateInstaller *This,
    IUpdateCollection **retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_put_Updates(
    IUpdateInstaller *This,
    IUpdateCollection *value )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_BeginInstall(
    IUpdateInstaller *This,
    IUnknown *onProgressChanged,
    IUnknown *onCompleted,
    VARIANT state,
    IInstallationJob **retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_BeginUninstall(
    IUpdateInstaller *This,
    IUnknown *onProgressChanged,
    IUnknown *onCompleted,
    VARIANT state,
    IInstallationJob **retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_EndInstall(
    IUpdateInstaller *This,
    IInstallationJob *value,
    IInstallationResult **retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_EndUninstall(
    IUpdateInstaller *This,
    IInstallationJob *value,
    IInstallationResult **retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_Install(
    IUpdateInstaller *This,
    IInstallationResult **retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_RunWizard(
    IUpdateInstaller *This,
    BSTR dialogTitle,
    IInstallationResult **retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_get_IsBusy(
    IUpdateInstaller *This,
    VARIANT_BOOL *retval )
{
    FIXME("semi-stub, returning VARIANT_FALSE!\n");
    *retval = VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI update_installer_Uninstall(
    IUpdateInstaller *This,
    IInstallationResult **retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_get_AllowSourcePrompts(
    IUpdateInstaller *This,
    VARIANT_BOOL *retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_put_AllowSourcePrompts(
    IUpdateInstaller *This,
    VARIANT_BOOL value )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_installer_get_RebootRequiredBeforeInstallation(
    IUpdateInstaller *This,
    VARIANT_BOOL *retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const struct IUpdateInstallerVtbl update_installer_vtbl =
{
    update_installer_QueryInterface,
    update_installer_AddRef,
    update_installer_Release,
    update_installer_GetTypeInfoCount,
    update_installer_GetTypeInfo,
    update_installer_GetIDsOfNames,
    update_installer_Invoke,
    update_installer_get_ClientApplicationID,
    update_installer_put_ClientApplicationID,
    update_installer_get_IsForced,
    update_installer_put_IsForced,
    update_installer_get_ParentHwnd,
    update_installer_put_ParentHwnd,
    update_installer_put_ParentWindow,
    update_installer_get_ParentWindow,
    update_installer_get_Updates,
    update_installer_put_Updates,
    update_installer_BeginInstall,
    update_installer_BeginUninstall,
    update_installer_EndInstall,
    update_installer_EndUninstall,
    update_installer_Install,
    update_installer_RunWizard,
    update_installer_get_IsBusy,
    update_installer_Uninstall,
    update_installer_get_AllowSourcePrompts,
    update_installer_put_AllowSourcePrompts,
    update_installer_get_RebootRequiredBeforeInstallation
};

HRESULT UpdateInstaller_create( LPVOID *ppObj )
{
    update_installer *installer;

    TRACE("(%p)\n", ppObj);

    installer = HeapAlloc( GetProcessHeap(), 0, sizeof(*installer) );
    if (!installer) return E_OUTOFMEMORY;

    installer->IUpdateInstaller_iface.lpVtbl = &update_installer_vtbl;
    installer->refs = 1;

    *ppObj = &installer->IUpdateInstaller_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
