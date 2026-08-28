/*
 *    ITSS Moniker implementation
 *
 * Copyright 2004 Mike McCormack
 *
 *  Implementation of the infamous mk:@MSITStore moniker
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


#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/itss.h"
#include "wine/debug.h"

#include "itsstor.h"

WINE_DEFAULT_DEBUG_CHANNEL(itss);

/*****************************************************************************/

typedef struct {
    IMoniker IMoniker_iface;
    LONG ref;
    LPWSTR szHtml;
    WCHAR szFile[1];
} ITS_IMonikerImpl;

static inline ITS_IMonikerImpl *impl_from_IMoniker(IMoniker *iface)
{
    return CONTAINING_RECORD(iface, ITS_IMonikerImpl, IMoniker_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI ITS_IMonikerImpl_QueryInterface(
    IMoniker* iface,
    REFIID riid,
    void** ppvObject)
{
    ITS_IMonikerImpl *This = impl_from_IMoniker(iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
	|| IsEqualGUID(riid, &IID_IParseDisplayName))
    {
	IMoniker_AddRef(iface);
	*ppvObject = iface;
	return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
    return E_NOINTERFACE;
}

static ULONG WINAPI ITS_IMonikerImpl_AddRef(
    IMoniker* iface)
{
    ITS_IMonikerImpl *This = impl_from_IMoniker(iface);
    TRACE("%p\n", This);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ITS_IMonikerImpl_Release(
    IMoniker* iface)
{
    ITS_IMonikerImpl *This = impl_from_IMoniker(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This);
        ITSS_UnlockModule();
    }

    return ref;
}

/*** IPersist methods ***/
static HRESULT WINAPI ITS_IMonikerImpl_GetClassID(
    IMoniker* iface,
    CLSID* pClassID)
{
    ITS_IMonikerImpl *This = impl_from_IMoniker(iface);

    TRACE("%p %p\n", This, pClassID);
    *pClassID = CLSID_ITStorage;
    return S_OK;
}

/*** IPersistStream methods ***/
static HRESULT WINAPI ITS_IMonikerImpl_IsDirty(
    IMoniker* iface)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITS_IMonikerImpl_Load(
    IMoniker* iface,
    IStream* pStm)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITS_IMonikerImpl_Save(
    IMoniker* iface,
    IStream* pStm,
    BOOL fClearDirty)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITS_IMonikerImpl_GetSizeMax(
    IMoniker* iface,
    ULARGE_INTEGER* pcbSize)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/*** IMoniker methods ***/
static HRESULT WINAPI ITS_IMonikerImpl_BindToObject(
    IMoniker* iface,
    IBindCtx* pbc,
    IMoniker* pmkToLeft,
    REFIID riidResult,
    void** ppvResult)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITS_IMonikerImpl_BindToStorage(
    IMoniker* iface,
    IBindCtx* pbc,
    IMoniker* pmkToLeft,
    REFIID riid,
    void** ppvObj)
{
    ITS_IMonikerImpl *This = impl_from_IMoniker(iface);
    DWORD grfMode = STGM_SIMPLE | STGM_READ | STGM_SHARE_EXCLUSIVE;
    HRESULT r;
    IStorage *stg = NULL;

    TRACE("%p %p %p %s %p\n", This,
           pbc, pmkToLeft, debugstr_guid(riid), ppvObj);

    r = ITSS_StgOpenStorage( This->szFile, NULL, grfMode, 0, 0, &stg );
    if( r == S_OK )
    {
        TRACE("Opened storage %s\n", debugstr_w( This->szFile ) );
        if (IsEqualGUID(riid, &IID_IStream))
            r = IStorage_OpenStream( stg, This->szHtml,
                       NULL, grfMode, 0, (IStream**)ppvObj );
        else if (IsEqualGUID(riid, &IID_IStorage))
            r = IStorage_OpenStorage( stg, This->szHtml,
                       NULL, grfMode, NULL, 0, (IStorage**)ppvObj );
        else
            r = STG_E_ACCESSDENIED;
        IStorage_Release( stg );
    }

    return r;
}

static HRESULT WINAPI ITS_IMonikerImpl_Reduce(
    IMoniker* iface,
    IBindCtx* pbc,
    DWORD dwReduceHowFar,
    IMoniker** ppmkToLeft,
    IMoniker** ppmkReduced)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITS_IMonikerImpl_ComposeWith(
    IMoniker* iface,
    IMoniker* pmkRight,
    BOOL fOnlyIfNotGeneric,
    IMoniker** ppmkComposite)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITS_IMonikerImpl_Enum(
    IMoniker* iface,
    BOOL fForward,
    IEnumMoniker** ppenumMoniker)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITS_IMonikerImpl_IsEqual(
    IMoniker* iface,
    IMoniker* pmkOtherMoniker)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITS_IMonikerImpl_Hash(
    IMoniker* iface,
    DWORD* pdwHash)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITS_IMonikerImpl_IsRunning(
    IMoniker* iface,
    IBindCtx* pbc,
    IMoniker* pmkToLeft,
    IMoniker* pmkNewlyRunning)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITS_IMonikerImpl_GetTimeOfLastChange(
    IMoniker* iface,
    IBindCtx* pbc,
    IMoniker* pmkToLeft,
    FILETIME* pFileTime)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITS_IMonikerImpl_Inverse(
    IMoniker* iface,
    IMoniker** ppmk)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITS_IMonikerImpl_CommonPrefixWith(
    IMoniker* iface,
    IMoniker* pmkOther,
    IMoniker** ppmkPrefix)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITS_IMonikerImpl_RelativePathTo(
    IMoniker* iface,
    IMoniker* pmkOther,
    IMoniker** ppmkRelPath)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITS_IMonikerImpl_GetDisplayName(
    IMoniker* iface,
    IBindCtx* pbc,
    IMoniker* pmkToLeft,
    LPOLESTR* ppszDisplayName)
{
    ITS_IMonikerImpl *This = impl_from_IMoniker(iface);
    DWORD len;
    LPWSTR str;

    TRACE("%p %p %p %p\n", iface, pbc, pmkToLeft, ppszDisplayName);

    len = lstrlenW( This->szFile ) + lstrlenW( This->szHtml );
    str = CoTaskMemAlloc( len*sizeof(WCHAR) );
    swprintf( str, len, L"ms-its:%s::%s", This->szFile, This->szHtml );

    *ppszDisplayName = str;
    
    return S_OK;
}

static HRESULT WINAPI ITS_IMonikerImpl_ParseDisplayName(
    IMoniker* iface,
    IBindCtx* pbc,
    IMoniker* pmkToLeft,
    LPOLESTR pszDisplayName,
    ULONG* pchEaten,
    IMoniker** ppmkOut)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITS_IMonikerImpl_IsSystemMoniker(
    IMoniker* iface,
    DWORD* pdwMksys)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const IMonikerVtbl ITS_IMonikerImpl_Vtbl =
{
    ITS_IMonikerImpl_QueryInterface,
    ITS_IMonikerImpl_AddRef,
    ITS_IMonikerImpl_Release,
    ITS_IMonikerImpl_GetClassID,
    ITS_IMonikerImpl_IsDirty,
    ITS_IMonikerImpl_Load,
    ITS_IMonikerImpl_Save,
    ITS_IMonikerImpl_GetSizeMax,
    ITS_IMonikerImpl_BindToObject,
    ITS_IMonikerImpl_BindToStorage,
    ITS_IMonikerImpl_Reduce,
    ITS_IMonikerImpl_ComposeWith,
    ITS_IMonikerImpl_Enum,
    ITS_IMonikerImpl_IsEqual,
    ITS_IMonikerImpl_Hash,
    ITS_IMonikerImpl_IsRunning,
    ITS_IMonikerImpl_GetTimeOfLastChange,
    ITS_IMonikerImpl_Inverse,
    ITS_IMonikerImpl_CommonPrefixWith,
    ITS_IMonikerImpl_RelativePathTo,
    ITS_IMonikerImpl_GetDisplayName,
    ITS_IMonikerImpl_ParseDisplayName,
    ITS_IMonikerImpl_IsSystemMoniker
};

static HRESULT ITS_IMoniker_create( IMoniker **ppObj, LPCWSTR name, DWORD n )
{
    ITS_IMonikerImpl *itsmon;
    DWORD sz;

    /* szFile[1] has space for one character already */
    sz = FIELD_OFFSET( ITS_IMonikerImpl, szFile[lstrlenW( name ) + 1] );

    itsmon = HeapAlloc( GetProcessHeap(), 0, sz );
    itsmon->IMoniker_iface.lpVtbl = &ITS_IMonikerImpl_Vtbl;
    itsmon->ref = 1;
    lstrcpyW( itsmon->szFile, name );
    itsmon->szHtml = &itsmon->szFile[n];

    while( *itsmon->szHtml == ':' )
        *itsmon->szHtml++ = 0;

    TRACE("-> %p %s %s\n", itsmon,
          debugstr_w(itsmon->szFile), debugstr_w(itsmon->szHtml) );
    *ppObj = &itsmon->IMoniker_iface;

    ITSS_LockModule();
    return S_OK;
}

/*****************************************************************************/

typedef struct {
    IParseDisplayName IParseDisplayName_iface;
    LONG ref;
} ITS_IParseDisplayNameImpl;

static inline ITS_IParseDisplayNameImpl *impl_from_IParseDisplayName(IParseDisplayName *iface)
{
    return CONTAINING_RECORD(iface, ITS_IParseDisplayNameImpl, IParseDisplayName_iface);
}

static HRESULT WINAPI ITS_IParseDisplayNameImpl_QueryInterface(
    IParseDisplayName* iface,
    REFIID riid,
    void** ppvObject)
{
    ITS_IParseDisplayNameImpl *This = impl_from_IParseDisplayName(iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
	|| IsEqualGUID(riid, &IID_IParseDisplayName))
    {
	IParseDisplayName_AddRef(iface);
	*ppvObject = iface;
	return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
    return E_NOINTERFACE;
}

static ULONG WINAPI ITS_IParseDisplayNameImpl_AddRef(
    IParseDisplayName* iface)
{
    ITS_IParseDisplayNameImpl *This = impl_from_IParseDisplayName(iface);
    TRACE("%p\n", This);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ITS_IParseDisplayNameImpl_Release(
    IParseDisplayName* iface)
{
    ITS_IParseDisplayNameImpl *This = impl_from_IParseDisplayName(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This);
        ITSS_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI ITS_IParseDisplayNameImpl_ParseDisplayName(
    IParseDisplayName *iface,
    IBindCtx * pbc,
    LPOLESTR pszDisplayName, 
    ULONG * pchEaten,
    IMoniker ** ppmkOut)
{
    static const WCHAR szPrefix[] = L"@MSITStore:";
    const DWORD prefix_len = ARRAY_SIZE(szPrefix)-1;
    DWORD n;

    ITS_IParseDisplayNameImpl *This = impl_from_IParseDisplayName(iface);

    TRACE("%p %s %p %p\n", This,
          debugstr_w( pszDisplayName ), pchEaten, ppmkOut );

    if( wcsnicmp( pszDisplayName, szPrefix, prefix_len ) )
        return MK_E_SYNTAX;

    /* search backwards for a double colon */
    for( n = lstrlenW( pszDisplayName ) - 3; prefix_len <= n; n-- )
        if( ( pszDisplayName[n] == ':' ) && ( pszDisplayName[n+1] == ':' ) )
            break;

    if( n < prefix_len )
        return MK_E_SYNTAX;

    if( !pszDisplayName[n+2] )
        return MK_E_SYNTAX;

    *pchEaten = lstrlenW( pszDisplayName ) - n - 3;

    return ITS_IMoniker_create( ppmkOut,
              &pszDisplayName[prefix_len], n-prefix_len );
}

static const IParseDisplayNameVtbl ITS_IParseDisplayNameImpl_Vtbl =
{
    ITS_IParseDisplayNameImpl_QueryInterface,
    ITS_IParseDisplayNameImpl_AddRef,
    ITS_IParseDisplayNameImpl_Release,
    ITS_IParseDisplayNameImpl_ParseDisplayName
};
 
HRESULT ITS_IParseDisplayName_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    ITS_IParseDisplayNameImpl *its;

    if( pUnkOuter )
        return CLASS_E_NOAGGREGATION;

    its = HeapAlloc( GetProcessHeap(), 0, sizeof(ITS_IParseDisplayNameImpl) );
    its->IParseDisplayName_iface.lpVtbl = &ITS_IParseDisplayNameImpl_Vtbl;
    its->ref = 1;

    TRACE("-> %p\n", its);
    *ppObj = its;

    ITSS_LockModule();
    return S_OK;
}
