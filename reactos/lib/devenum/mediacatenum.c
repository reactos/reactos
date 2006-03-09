/*
 *	IEnumMoniker implementation for DEVENUM.dll
 *
 * Copyright (C) 2002 Robert Shearman
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTES ON THIS FILE:
 * - Implements IEnumMoniker interface which enumerates through moniker
 *   objects created from HKEY_CLASSES/CLSID/{DEVICE_CLSID}/Instance
 */

#include "devenum_private.h"
#include "vfwmsgs.h"
#include "oleauto.h"

#include "wine/debug.h"


/* #define ICOM_THIS_From_IROTData(class, name) class* This = (class*)(((char*)name)-sizeof(void*)) */

WINE_DEFAULT_DEBUG_CHANNEL(devenum);

static ULONG WINAPI DEVENUM_IEnumMoniker_AddRef(LPENUMMONIKER iface);
static ULONG WINAPI DEVENUM_IMediaCatMoniker_AddRef(LPMONIKER iface);
static ULONG WINAPI DEVENUM_IPropertyBag_AddRef(LPPROPERTYBAG iface);

typedef struct
{
    IPropertyBagVtbl *lpVtbl;
    DWORD ref;
    HKEY hkey;
} RegPropBagImpl;


static HRESULT WINAPI DEVENUM_IPropertyBag_QueryInterface(
    LPPROPERTYBAG iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    RegPropBagImpl *This = (RegPropBagImpl *)iface;
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (This == NULL || ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IPropertyBag))
    {
        *ppvObj = (LPVOID)iface;
        DEVENUM_IPropertyBag_AddRef(iface);
        return S_OK;
    }

    FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

/**********************************************************************
 * DEVENUM_IPropertyBag_AddRef (also IUnknown)
 */
static ULONG WINAPI DEVENUM_IPropertyBag_AddRef(LPPROPERTYBAG iface)
{
    RegPropBagImpl *This = (RegPropBagImpl *)iface;
    TRACE("\n");

    return InterlockedIncrement(&This->ref);
}

/**********************************************************************
 * DEVENUM_IPropertyBag_Release (also IUnknown)
 */
static ULONG WINAPI DEVENUM_IPropertyBag_Release(LPPROPERTYBAG iface)
{
    RegPropBagImpl *This = (RegPropBagImpl *)iface;

    TRACE("\n");

    if (InterlockedDecrement(&This->ref) == 0) {
        RegCloseKey(This->hkey);
        CoTaskMemFree(This);
        DEVENUM_UnlockModule();
        return 0;
    }
    return This->ref;
}

static HRESULT WINAPI DEVENUM_IPropertyBag_Read(
    LPPROPERTYBAG iface,
    LPCOLESTR pszPropName,
    VARIANT* pVar,
    IErrorLog* pErrorLog)
{
    LPVOID pData = NULL;
    LONG received;
    DWORD type = 0;
    RegPropBagImpl *This = (RegPropBagImpl *)iface;
    HRESULT res = S_OK;
    LONG reswin32;

    TRACE("(%p)->(%s, %p, %p)\n", This, debugstr_w(pszPropName), pVar, pErrorLog);

    if (!pszPropName || !pVar)
        return E_POINTER;

    reswin32 = RegQueryValueExW(This->hkey, pszPropName, NULL, NULL, NULL, &received);
    res = HRESULT_FROM_WIN32(reswin32);

    if (SUCCEEDED(res))
    {
        pData = HeapAlloc(GetProcessHeap(), 0, received);

        /* work around a GCC bug that occurs here unless we use the reswin32 variable as well */
        reswin32 = RegQueryValueExW(This->hkey, pszPropName, NULL, &type, pData, &received);
        res = HRESULT_FROM_WIN32(reswin32);
    }

    if (SUCCEEDED(res))
    {
        res = E_INVALIDARG; /* assume we cannot coerce into right type */

        TRACE("Read %ld bytes (%s)\n", received, type == REG_SZ ? debugstr_w((LPWSTR)pData) : "binary data");

        switch (type)
        {
        case REG_SZ:
            switch (V_VT(pVar))
            {
            case VT_LPWSTR:
                V_UNION(pVar, bstrVal) = CoTaskMemAlloc(received);
                memcpy(V_UNION(pVar, bstrVal), (LPWSTR)pData, received);
                res = S_OK;
                break;
            case VT_EMPTY:
                V_VT(pVar) = VT_BSTR;
            /* fall through */
            case VT_BSTR:
                V_UNION(pVar, bstrVal) = SysAllocStringLen((LPWSTR)pData, received/sizeof(WCHAR) - 1);
                res = S_OK;
                break;
            }
            break;
        case REG_DWORD:
            TRACE("REG_DWORD: %lx\n", *(DWORD *)pData);
            switch (V_VT(pVar))
            {
            case VT_EMPTY:
                V_VT(pVar) = VT_I4;
                /* fall through */
            case VT_I4:
            case VT_UI4:
                V_UNION(pVar, ulVal) = *(DWORD *)pData;
                res = S_OK;
                break;
            }
            break;
        case REG_BINARY:
            {
                SAFEARRAYBOUND bound;
                void * pArrayElements;
                bound.lLbound = 0;
                bound.cElements = received;
                TRACE("REG_BINARY: len = %ld\n", received);
                switch (V_VT(pVar))
                {
                case VT_EMPTY:
                case VT_ARRAY | VT_UI1:
                    if (!(V_UNION(pVar, parray) = SafeArrayCreate(VT_UI1, 1, &bound)))
                        res = E_OUTOFMEMORY;
                    res = S_OK;
                    break;
                }

                if (res == E_INVALIDARG)
                    break;

                res = SafeArrayAccessData(V_UNION(pVar, parray), &pArrayElements);
                if (FAILED(res))
                    break;

                CopyMemory(pArrayElements, pData, received);
                res = SafeArrayUnaccessData(V_UNION(pVar, parray));
                break;
            }
        }
        if (res == E_INVALIDARG)
            FIXME("Variant type %x not supported for regtype %lx\n", V_VT(pVar), type);
    }

    if (pData)
        HeapFree(GetProcessHeap(), 0, pData);

    TRACE("<- %lx\n", res);
    return res;
}

static HRESULT WINAPI DEVENUM_IPropertyBag_Write(
    LPPROPERTYBAG iface,
    LPCOLESTR pszPropName,
    VARIANT* pVar)
{
    RegPropBagImpl *This = (RegPropBagImpl *)iface;
    LPVOID lpData = NULL;
    DWORD cbData = 0;
    DWORD dwType = 0;
    HRESULT res = S_OK;

    TRACE("(%p)->(%s, %p)\n", This, debugstr_w(pszPropName), pVar);

    switch (V_VT(pVar))
    {
    case VT_BSTR:
        TRACE("writing %s\n", debugstr_w(V_UNION(pVar, bstrVal)));
        lpData = (LPVOID)V_UNION(pVar, bstrVal);
        dwType = REG_SZ;
        cbData = (lstrlenW(V_UNION(pVar, bstrVal)) + 1) * sizeof(WCHAR);
        break;
    case VT_I4:
    case VT_UI4:
        TRACE("writing %lu\n", V_UNION(pVar, ulVal));
        lpData = (LPVOID)&V_UNION(pVar, ulVal);
        dwType = REG_DWORD;
        cbData = sizeof(DWORD);
        break;
    case VT_ARRAY | VT_UI1:
    {
        LONG lUbound = 0;
        LONG lLbound = 0;
        dwType = REG_BINARY;
        res = SafeArrayGetLBound(V_UNION(pVar, parray), 1, &lLbound);
        res = SafeArrayGetUBound(V_UNION(pVar, parray), 1, &lUbound);
        cbData = (lUbound - lLbound + 1) /* * sizeof(BYTE)*/;
        TRACE("cbData: %ld\n", cbData);
        res = SafeArrayAccessData(V_UNION(pVar, parray), &lpData);
        break;
    }
    default:
        FIXME("Variant type %d not handled\n", V_VT(pVar));
        return E_FAIL;
    }

    if (RegSetValueExW(This->hkey,
                       pszPropName, 0,
                       dwType, lpData, cbData) != ERROR_SUCCESS)
        res = E_FAIL;

    if (V_VT(pVar) & VT_ARRAY)
        res = SafeArrayUnaccessData(V_UNION(pVar, parray));

    return res;
}

static IPropertyBagVtbl IPropertyBag_Vtbl =
{
    DEVENUM_IPropertyBag_QueryInterface,
    DEVENUM_IPropertyBag_AddRef,
    DEVENUM_IPropertyBag_Release,
    DEVENUM_IPropertyBag_Read,
    DEVENUM_IPropertyBag_Write
};

static HRESULT DEVENUM_IPropertyBag_Construct(HANDLE hkey, IPropertyBag **ppBag)
{
    RegPropBagImpl * rpb = CoTaskMemAlloc(sizeof(RegPropBagImpl));
    if (!rpb)
        return E_OUTOFMEMORY;
    rpb->lpVtbl = &IPropertyBag_Vtbl;
    rpb->ref = 1;
    rpb->hkey = hkey;
    *ppBag = (IPropertyBag*)rpb;
    DEVENUM_LockModule();
    return S_OK;
}


static HRESULT WINAPI DEVENUM_IMediaCatMoniker_QueryInterface(
    LPMONIKER iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    MediaCatMoniker *This = (MediaCatMoniker *)iface;
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (This == NULL || ppvObj == NULL) return E_POINTER;

    *ppvObj = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IPersist) ||
        IsEqualGUID(riid, &IID_IPersistStream) ||
        IsEqualGUID(riid, &IID_IMoniker))
    {
        *ppvObj = (LPVOID)iface;
        DEVENUM_IMediaCatMoniker_AddRef(iface);
        return S_OK;
    }

    FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

/**********************************************************************
 * DEVENUM_IMediaCatMoniker_AddRef (also IUnknown)
 */
static ULONG WINAPI DEVENUM_IMediaCatMoniker_AddRef(LPMONIKER iface)
{
    MediaCatMoniker *This = (MediaCatMoniker *)iface;
    TRACE("\n");

    return InterlockedIncrement(&This->ref);
}

/**********************************************************************
 * DEVENUM_IMediaCatMoniker_Release (also IUnknown)
 */
static ULONG WINAPI DEVENUM_IMediaCatMoniker_Release(LPMONIKER iface)
{
    MediaCatMoniker *This = (MediaCatMoniker *)iface;
    ULONG ref;
    TRACE("\n");

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
        RegCloseKey(This->hkey);
        CoTaskMemFree(This);
        DEVENUM_UnlockModule();
    }
    return ref;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_GetClassID(
    LPMONIKER iface,
    CLSID* pClassID)
{
    MediaCatMoniker *This = (MediaCatMoniker *)iface;
    FIXME("(%p)->(%p)\n", This, pClassID);

    if (pClassID == NULL)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_IsDirty(LPMONIKER iface)
{
    FIXME("()\n");

    return S_FALSE;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_Load(LPMONIKER iface, IStream* pStm)
{
    FIXME("(%p)\n", pStm);

    return E_NOTIMPL;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_Save(LPMONIKER iface, IStream* pStm, BOOL fClearDirty)
{
    FIXME("(%p, %s)\n", pStm, fClearDirty ? "true" : "false");

    return STG_E_CANTSAVE;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_GetSizeMax(
    LPMONIKER iface,
    ULARGE_INTEGER* pcbSize)
{
    FIXME("(%p)\n", pcbSize);

    ZeroMemory(pcbSize, sizeof(*pcbSize));

    return S_OK;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_BindToObject(
    LPMONIKER iface,
    IBindCtx* pbc,
    IMoniker* pmkToLeft,
    REFIID riidResult,
    void** ppvResult)
{
    IUnknown * pObj = NULL;
    IPropertyBag * pProp = NULL;
    CLSID clsID;
    VARIANT var;
    HRESULT res = E_FAIL;

    MediaCatMoniker *This = (MediaCatMoniker *)iface;

    VariantClear(&var);

    TRACE("(%p)->(%p, %p, %s, %p)\n", This, pbc, pmkToLeft, debugstr_guid(riidResult), ppvResult);

    *ppvResult = NULL;

    if(pmkToLeft==NULL)
    {
            /* first activation of this class */
	    LPVOID pvptr;
            res=IMoniker_BindToStorage(iface, NULL, NULL, &IID_IPropertyBag, &pvptr);
	    pProp = (IPropertyBag*)pvptr;
            if (SUCCEEDED(res))
            {
                V_VT(&var) = VT_LPWSTR;
                res = IPropertyBag_Read(pProp, clsid_keyname, &var, NULL);
            }
            if (SUCCEEDED(res))
            {
                res = CLSIDFromString(V_UNION(&var,bstrVal), &clsID);
                CoTaskMemFree(V_UNION(&var, bstrVal));
            }
            if (SUCCEEDED(res))
            {
                res=CoCreateInstance(&clsID,NULL,CLSCTX_ALL,&IID_IUnknown,&pvptr);
		pObj = (IUnknown*)pvptr;
            }
    }

    if (pObj!=NULL)
    {
        /* get the requested interface from the loaded class */
        res= IUnknown_QueryInterface(pObj,riidResult,ppvResult);
    }

    if (pProp)
    {
        IPropertyBag_Release(pProp);
    }

    TRACE("<- 0x%lx\n", res);

    return res;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_BindToStorage(
    LPMONIKER iface,
    IBindCtx* pbc,
    IMoniker* pmkToLeft,
    REFIID riid,
    void** ppvObj)
{
    MediaCatMoniker *This = (MediaCatMoniker *)iface;
    TRACE("(%p)->(%p, %p, %s, %p)\n", This, pbc, pmkToLeft, debugstr_guid(riid), ppvObj);

    *ppvObj = NULL;

    if (pbc || pmkToLeft)
        return MK_E_NOSTORAGE;

    if (IsEqualGUID(riid, &IID_IPropertyBag))
    {
        HANDLE hkey;
        DuplicateHandle(GetCurrentProcess(), This->hkey, GetCurrentProcess(), &hkey, 0, 0, DUPLICATE_SAME_ACCESS);
        return DEVENUM_IPropertyBag_Construct(hkey, (IPropertyBag**)ppvObj);
    }

    return MK_E_NOSTORAGE;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_Reduce(
    LPMONIKER iface,
    IBindCtx* pbc,
    DWORD dwReduceHowFar,
    IMoniker** ppmkToLeft,
    IMoniker** ppmkReduced)
{
    TRACE("(%p, %ld, %p, %p)\n", pbc, dwReduceHowFar, ppmkToLeft, ppmkReduced);

    if (ppmkToLeft)
        *ppmkToLeft = NULL;
    *ppmkReduced = iface;

    return MK_S_REDUCED_TO_SELF;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_ComposeWith(
    LPMONIKER iface,
    IMoniker* pmkRight,
    BOOL fOnlyIfNotGeneric,
    IMoniker** ppmkComposite)
{
    FIXME("(%p, %s, %p): stub\n", pmkRight, fOnlyIfNotGeneric ? "true" : "false", ppmkComposite);

    /* FIXME: use CreateGenericComposite? */
    *ppmkComposite = NULL;

    return E_NOTIMPL;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_Enum(
    LPMONIKER iface,
    BOOL fForward,
    IEnumMoniker** ppenumMoniker)
{
    FIXME("(%s, %p): stub\n", fForward ? "true" : "false", ppenumMoniker);

    *ppenumMoniker = NULL;

    return S_OK;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_IsEqual(
    LPMONIKER iface,
    IMoniker* pmkOtherMoniker)
{
    FIXME("(%p)\n", pmkOtherMoniker);

    return E_NOTIMPL;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_Hash(
    LPMONIKER iface,
    DWORD* pdwHash)
{
    TRACE("(%p)\n", pdwHash);

    *pdwHash = 0;

    return S_OK;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_IsRunning(
    LPMONIKER iface,
    IBindCtx* pbc,
    IMoniker* pmkToLeft,
    IMoniker* pmkNewlyRunning)
{
    FIXME("(%p, %p, %p)\n", pbc, pmkToLeft, pmkNewlyRunning);

    return S_FALSE;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_GetTimeOfLastChange(
    LPMONIKER iface,
    IBindCtx* pbc,
    IMoniker* pmkToLeft,
    FILETIME* pFileTime)
{
    TRACE("(%p, %p, %p)\n", pbc, pmkToLeft, pFileTime);

    pFileTime->dwLowDateTime = 0xFFFFFFFF;
    pFileTime->dwHighDateTime = 0x7FFFFFFF;

    return MK_E_UNAVAILABLE;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_Inverse(
    LPMONIKER iface,
    IMoniker** ppmk)
{
    TRACE("(%p)\n", ppmk);

    *ppmk = NULL;

    return MK_E_NOINVERSE;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_CommonPrefixWith(
    LPMONIKER iface,
    IMoniker* pmkOtherMoniker,
    IMoniker** ppmkPrefix)
{
    TRACE("(%p, %p)\n", pmkOtherMoniker, ppmkPrefix);

    *ppmkPrefix = NULL;

    return MK_E_NOPREFIX;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_RelativePathTo(
    LPMONIKER iface,
    IMoniker* pmkOther,
    IMoniker** ppmkRelPath)
{
    TRACE("(%p, %p)\n", pmkOther, ppmkRelPath);

    *ppmkRelPath = pmkOther;

    return MK_S_HIM;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_GetDisplayName(
    LPMONIKER iface,
    IBindCtx* pbc,
    IMoniker* pmkToLeft,
    LPOLESTR* ppszDisplayName)
{
    MediaCatMoniker *This = (MediaCatMoniker *)iface;
    WCHAR wszBuffer[MAX_PATH];
    static const WCHAR wszFriendlyName[] = {'F','r','i','e','n','d','l','y','N','a','m','e',0};
    LONG received = sizeof(wszFriendlyName);

    TRACE("(%p, %p, %p)\n", pbc, pmkToLeft, ppszDisplayName);

    *ppszDisplayName = NULL;

    /* FIXME: should this be the weird stuff we have to parse in IParseDisplayName? */
    if (RegQueryValueW(This->hkey, wszFriendlyName, wszBuffer, &received) == ERROR_SUCCESS)
    {
        *ppszDisplayName = CoTaskMemAlloc(received);
        strcpyW(*ppszDisplayName, wszBuffer);
        return S_OK;
    }

    return E_FAIL;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_ParseDisplayName(
    LPMONIKER iface,
    IBindCtx* pbc,
    IMoniker* pmkToLeft,
    LPOLESTR pszDisplayName,
    ULONG* pchEaten,
    IMoniker** ppmkOut)
{
    FIXME("(%p, %p, %s, %p, %p)\n", pbc, pmkToLeft, debugstr_w(pszDisplayName), pchEaten, ppmkOut);

    *pchEaten = 0;
    *ppmkOut = NULL;

    return MK_E_SYNTAX;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_IsSystemMoniker(
    LPMONIKER iface,
    DWORD* pdwMksys)
{
    TRACE("(%p)\n", pdwMksys);

    return S_FALSE;
}

static IMonikerVtbl IMoniker_Vtbl =
{
    DEVENUM_IMediaCatMoniker_QueryInterface,
    DEVENUM_IMediaCatMoniker_AddRef,
    DEVENUM_IMediaCatMoniker_Release,
    DEVENUM_IMediaCatMoniker_GetClassID,
    DEVENUM_IMediaCatMoniker_IsDirty,
    DEVENUM_IMediaCatMoniker_Load,
    DEVENUM_IMediaCatMoniker_Save,
    DEVENUM_IMediaCatMoniker_GetSizeMax,
    DEVENUM_IMediaCatMoniker_BindToObject,
    DEVENUM_IMediaCatMoniker_BindToStorage,
    DEVENUM_IMediaCatMoniker_Reduce,
    DEVENUM_IMediaCatMoniker_ComposeWith,
    DEVENUM_IMediaCatMoniker_Enum,
    DEVENUM_IMediaCatMoniker_IsEqual,
    DEVENUM_IMediaCatMoniker_Hash,
    DEVENUM_IMediaCatMoniker_IsRunning,
    DEVENUM_IMediaCatMoniker_GetTimeOfLastChange,
    DEVENUM_IMediaCatMoniker_Inverse,
    DEVENUM_IMediaCatMoniker_CommonPrefixWith,
    DEVENUM_IMediaCatMoniker_RelativePathTo,
    DEVENUM_IMediaCatMoniker_GetDisplayName,
    DEVENUM_IMediaCatMoniker_ParseDisplayName,
    DEVENUM_IMediaCatMoniker_IsSystemMoniker
};

MediaCatMoniker * DEVENUM_IMediaCatMoniker_Construct()
{
    MediaCatMoniker * pMoniker = NULL;
    pMoniker = CoTaskMemAlloc(sizeof(MediaCatMoniker));
    if (!pMoniker)
        return NULL;

    pMoniker->lpVtbl = &IMoniker_Vtbl;
    pMoniker->ref = 0;
    pMoniker->hkey = NULL;

    DEVENUM_IMediaCatMoniker_AddRef((LPMONIKER)pMoniker);

    DEVENUM_LockModule();

    return pMoniker;
}

/**********************************************************************
 * DEVENUM_IEnumMoniker_QueryInterface (also IUnknown)
 */
static HRESULT WINAPI DEVENUM_IEnumMoniker_QueryInterface(
    LPENUMMONIKER iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (This == NULL || ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IEnumMoniker))
    {
        *ppvObj = (LPVOID)iface;
        DEVENUM_IEnumMoniker_AddRef(iface);
        return S_OK;
    }

    FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

/**********************************************************************
 * DEVENUM_IEnumMoniker_AddRef (also IUnknown)
 */
static ULONG WINAPI DEVENUM_IEnumMoniker_AddRef(LPENUMMONIKER iface)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    TRACE("\n");

    return InterlockedIncrement(&This->ref);
}

/**********************************************************************
 * DEVENUM_IEnumMoniker_Release (also IUnknown)
 */
static ULONG WINAPI DEVENUM_IEnumMoniker_Release(LPENUMMONIKER iface)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    TRACE("\n");

    if (!InterlockedDecrement(&This->ref))
    {
        RegCloseKey(This->hkey);
        CoTaskMemFree(This);
        DEVENUM_UnlockModule();
        return 0;
    }
    return This->ref;
}

static HRESULT WINAPI DEVENUM_IEnumMoniker_Next(LPENUMMONIKER iface, ULONG celt, IMoniker ** rgelt, ULONG * pceltFetched)
{
    WCHAR buffer[MAX_PATH + 1];
    LONG res;
    ULONG fetched = 0;
    MediaCatMoniker * pMoniker;
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    TRACE("(%ld, %p, %p)\n", celt, rgelt, pceltFetched);

    while (fetched < celt)
    {
        res = RegEnumKeyW(This->hkey, This->index, buffer, sizeof(buffer) / sizeof(WCHAR));
        if (res != ERROR_SUCCESS)
        {
            break;
        }
        pMoniker = DEVENUM_IMediaCatMoniker_Construct();
        if (!pMoniker)
            return E_OUTOFMEMORY;

        if (RegOpenKeyW(This->hkey, buffer, &pMoniker->hkey) != ERROR_SUCCESS)
        {
            DEVENUM_IMediaCatMoniker_Release((LPMONIKER)pMoniker);
            break;
        }
        rgelt[fetched] = (LPMONIKER)pMoniker;
        fetched++;
    }

    This->index += fetched;

    TRACE("-- fetched %ld\n", fetched);

    if (pceltFetched)
        *pceltFetched = fetched;

    if (fetched != celt)
        return S_FALSE;
    else
        return S_OK;
}

static HRESULT WINAPI DEVENUM_IEnumMoniker_Skip(LPENUMMONIKER iface, ULONG celt)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    TRACE("(%ld)\n", celt);

    This->index += celt;

    return S_OK;
}

static HRESULT WINAPI DEVENUM_IEnumMoniker_Reset(LPENUMMONIKER iface)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    TRACE("()\n");

    This->index = 0;

    return S_OK;
}

static HRESULT WINAPI DEVENUM_IEnumMoniker_Clone(LPENUMMONIKER iface, IEnumMoniker ** ppenum)
{
    FIXME("(%p): stub\n", ppenum);

    return E_NOTIMPL;
}

/**********************************************************************
 * IEnumMoniker_Vtbl
 */
static IEnumMonikerVtbl IEnumMoniker_Vtbl =
{
    DEVENUM_IEnumMoniker_QueryInterface,
    DEVENUM_IEnumMoniker_AddRef,
    DEVENUM_IEnumMoniker_Release,
    DEVENUM_IEnumMoniker_Next,
    DEVENUM_IEnumMoniker_Skip,
    DEVENUM_IEnumMoniker_Reset,
    DEVENUM_IEnumMoniker_Clone
};

HRESULT DEVENUM_IEnumMoniker_Construct(HKEY hkey, IEnumMoniker ** ppEnumMoniker)
{
    EnumMonikerImpl * pEnumMoniker = CoTaskMemAlloc(sizeof(EnumMonikerImpl));
    if (!pEnumMoniker)
        return E_OUTOFMEMORY;

    pEnumMoniker->lpVtbl = &IEnumMoniker_Vtbl;
    pEnumMoniker->ref = 1;
    pEnumMoniker->index = 0;
    pEnumMoniker->hkey = hkey;

    *ppEnumMoniker = (IEnumMoniker *)pEnumMoniker;

    DEVENUM_LockModule();

    return S_OK;
}
