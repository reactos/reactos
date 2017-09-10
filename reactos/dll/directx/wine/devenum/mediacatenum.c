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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES ON THIS FILE:
 * - Implements IEnumMoniker interface which enumerates through moniker
 *   objects created from HKEY_CLASSES/CLSID/{DEVICE_CLSID}/Instance
 */

#include "devenum_private.h"

#include <ocidl.h>
#include <oleauto.h>

typedef struct
{
    IEnumMoniker IEnumMoniker_iface;
    LONG ref;
    DWORD index;
    DWORD subkey_cnt;
    HKEY hkey;
    HKEY special_hkey;
} EnumMonikerImpl;

typedef struct
{
    IPropertyBag IPropertyBag_iface;
    LONG ref;
    HKEY hkey;
} RegPropBagImpl;


static inline RegPropBagImpl *impl_from_IPropertyBag(IPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, RegPropBagImpl, IPropertyBag_iface);
}

static HRESULT WINAPI DEVENUM_IPropertyBag_QueryInterface(
    LPPROPERTYBAG iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    RegPropBagImpl *This = impl_from_IPropertyBag(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppvObj);

    if (This == NULL || ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IPropertyBag))
    {
        *ppvObj = iface;
        IPropertyBag_AddRef(iface);
        return S_OK;
    }

    FIXME("- no interface IID: %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

/**********************************************************************
 * DEVENUM_IPropertyBag_AddRef (also IUnknown)
 */
static ULONG WINAPI DEVENUM_IPropertyBag_AddRef(LPPROPERTYBAG iface)
{
    RegPropBagImpl *This = impl_from_IPropertyBag(iface);

    TRACE("(%p)->() AddRef from %d\n", iface, This->ref);

    return InterlockedIncrement(&This->ref);
}

/**********************************************************************
 * DEVENUM_IPropertyBag_Release (also IUnknown)
 */
static ULONG WINAPI DEVENUM_IPropertyBag_Release(LPPROPERTYBAG iface)
{
    RegPropBagImpl *This = impl_from_IPropertyBag(iface);
    ULONG ref;

    TRACE("(%p)->() ReleaseThis->ref from %d\n", iface, This->ref);

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
        RegCloseKey(This->hkey);
        CoTaskMemFree(This);
        DEVENUM_UnlockModule();
    }
    return ref;
}

static HRESULT WINAPI DEVENUM_IPropertyBag_Read(
    LPPROPERTYBAG iface,
    LPCOLESTR pszPropName,
    VARIANT* pVar,
    IErrorLog* pErrorLog)
{
    LPVOID pData = NULL;
    DWORD received;
    DWORD type = 0;
    RegPropBagImpl *This = impl_from_IPropertyBag(iface);
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

        TRACE("Read %d bytes (%s)\n", received, type == REG_SZ ? debugstr_w(pData) : "binary data");

        switch (type)
        {
        case REG_SZ:
            switch (V_VT(pVar))
            {
            case VT_LPWSTR:
                V_BSTR(pVar) = CoTaskMemAlloc(received);
                memcpy(V_BSTR(pVar), pData, received);
                res = S_OK;
                break;
            case VT_EMPTY:
                V_VT(pVar) = VT_BSTR;
            /* fall through */
            case VT_BSTR:
                V_BSTR(pVar) = SysAllocStringLen(pData, received/sizeof(WCHAR) - 1);
                res = S_OK;
                break;
            }
            break;
        case REG_DWORD:
            TRACE("REG_DWORD: %x\n", *(DWORD *)pData);
            switch (V_VT(pVar))
            {
            case VT_EMPTY:
                V_VT(pVar) = VT_I4;
                /* fall through */
            case VT_I4:
            case VT_UI4:
                V_I4(pVar) = *(DWORD *)pData;
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
                TRACE("REG_BINARY: len = %d\n", received);
                switch (V_VT(pVar))
                {
                case VT_EMPTY:
                    V_VT(pVar) = VT_ARRAY | VT_UI1;
                    /* fall through */
                case VT_ARRAY | VT_UI1:
                    if (!(V_ARRAY(pVar) = SafeArrayCreate(VT_UI1, 1, &bound)))
                        res = E_OUTOFMEMORY;
                    else
                        res = S_OK;
                    break;
                }

                if (res == E_INVALIDARG)
                    break;

                res = SafeArrayAccessData(V_ARRAY(pVar), &pArrayElements);
                if (FAILED(res))
                    break;

                CopyMemory(pArrayElements, pData, received);
                res = SafeArrayUnaccessData(V_ARRAY(pVar));
                break;
            }
        }
        if (res == E_INVALIDARG)
            FIXME("Variant type %x not supported for regtype %x\n", V_VT(pVar), type);
    }

    HeapFree(GetProcessHeap(), 0, pData);

    TRACE("<- %x\n", res);
    return res;
}

static HRESULT WINAPI DEVENUM_IPropertyBag_Write(
    LPPROPERTYBAG iface,
    LPCOLESTR pszPropName,
    VARIANT* pVar)
{
    RegPropBagImpl *This = impl_from_IPropertyBag(iface);
    LPVOID lpData = NULL;
    DWORD cbData = 0;
    DWORD dwType = 0;
    HRESULT res = S_OK;

    TRACE("(%p)->(%s, %p)\n", This, debugstr_w(pszPropName), pVar);

    switch (V_VT(pVar))
    {
    case VT_BSTR:
    case VT_LPWSTR:
        TRACE("writing %s\n", debugstr_w(V_BSTR(pVar)));
        lpData = V_BSTR(pVar);
        dwType = REG_SZ;
        cbData = (lstrlenW(V_BSTR(pVar)) + 1) * sizeof(WCHAR);
        break;
    case VT_I4:
    case VT_UI4:
        TRACE("writing %u\n", V_UI4(pVar));
        lpData = &V_UI4(pVar);
        dwType = REG_DWORD;
        cbData = sizeof(DWORD);
        break;
    case VT_ARRAY | VT_UI1:
    {
        LONG lUbound = 0;
        LONG lLbound = 0;
        dwType = REG_BINARY;
        res = SafeArrayGetLBound(V_ARRAY(pVar), 1, &lLbound);
        res = SafeArrayGetUBound(V_ARRAY(pVar), 1, &lUbound);
        cbData = (lUbound - lLbound + 1) /* * sizeof(BYTE)*/;
        TRACE("cbData: %d\n", cbData);
        res = SafeArrayAccessData(V_ARRAY(pVar), &lpData);
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
        res = SafeArrayUnaccessData(V_ARRAY(pVar));

    return res;
}

static const IPropertyBagVtbl IPropertyBag_Vtbl =
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
    rpb->IPropertyBag_iface.lpVtbl = &IPropertyBag_Vtbl;
    rpb->ref = 1;
    rpb->hkey = hkey;
    *ppBag = &rpb->IPropertyBag_iface;
    DEVENUM_LockModule();
    return S_OK;
}


static inline MediaCatMoniker *impl_from_IMoniker(IMoniker *iface)
{
    return CONTAINING_RECORD(iface, MediaCatMoniker, IMoniker_iface);
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_QueryInterface(IMoniker *iface, REFIID riid,
        void **ppv)
{
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (!ppv)
        return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IPersist) ||
        IsEqualGUID(riid, &IID_IPersistStream) ||
        IsEqualGUID(riid, &IID_IMoniker))
    {
        *ppv = iface;
        IMoniker_AddRef(iface);
        return S_OK;
    }

    FIXME("- no interface IID: %s\n", debugstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI DEVENUM_IMediaCatMoniker_AddRef(IMoniker *iface)
{
    MediaCatMoniker *This = impl_from_IMoniker(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI DEVENUM_IMediaCatMoniker_Release(IMoniker *iface)
{
    MediaCatMoniker *This = impl_from_IMoniker(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if (ref == 0) {
        RegCloseKey(This->hkey);
        CoTaskMemFree(This);
        DEVENUM_UnlockModule();
    }
    return ref;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_GetClassID(IMoniker *iface, CLSID *pClassID)
{
    MediaCatMoniker *This = impl_from_IMoniker(iface);

    TRACE("(%p)->(%p)\n", This, pClassID);

    if (pClassID == NULL)
        return E_INVALIDARG;

    *pClassID = CLSID_CDeviceMoniker;

    return S_OK;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_IsDirty(IMoniker *iface)
{
    FIXME("(%p)->(): stub\n", iface);

    return S_FALSE;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_Load(IMoniker *iface, IStream *pStm)
{
    FIXME("(%p)->(%p): stub\n", iface, pStm);

    return E_NOTIMPL;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_Save(IMoniker *iface, IStream *pStm, BOOL fClearDirty)
{
    FIXME("(%p)->(%p, %s): stub\n", iface, pStm, fClearDirty ? "true" : "false");

    return STG_E_CANTSAVE;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_GetSizeMax(IMoniker *iface, ULARGE_INTEGER *pcbSize)
{
    FIXME("(%p)->(%p): stub\n", iface, pcbSize);

    ZeroMemory(pcbSize, sizeof(*pcbSize));

    return S_OK;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_BindToObject(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, REFIID riidResult, void **ppvResult)
{
    MediaCatMoniker *This = impl_from_IMoniker(iface);
    IUnknown * pObj = NULL;
    IPropertyBag * pProp = NULL;
    CLSID clsID;
    VARIANT var;
    HRESULT res = E_FAIL;

    TRACE("(%p)->(%p, %p, %s, %p)\n", This, pbc, pmkToLeft, debugstr_guid(riidResult), ppvResult);

    VariantInit(&var);
    *ppvResult = NULL;

    if(pmkToLeft==NULL)
    {
            /* first activation of this class */
	    LPVOID pvptr;
            res=IMoniker_BindToStorage(iface, NULL, NULL, &IID_IPropertyBag, &pvptr);
            pProp = pvptr;
            if (SUCCEEDED(res))
            {
                V_VT(&var) = VT_LPWSTR;
                res = IPropertyBag_Read(pProp, clsid_keyname, &var, NULL);
            }
            if (SUCCEEDED(res))
            {
                res = CLSIDFromString(V_BSTR(&var), &clsID);
                CoTaskMemFree(V_BSTR(&var));
            }
            if (SUCCEEDED(res))
            {
                res=CoCreateInstance(&clsID,NULL,CLSCTX_ALL,&IID_IUnknown,&pvptr);
                pObj = pvptr;
            }
    }

    if (pObj!=NULL)
    {
        /* get the requested interface from the loaded class */
        res = S_OK;
        if (pProp) {
           HRESULT res2;
           LPVOID ppv = NULL;
           res2 = IUnknown_QueryInterface(pObj, &IID_IPersistPropertyBag, &ppv);
           if (SUCCEEDED(res2)) {
              res = IPersistPropertyBag_Load((IPersistPropertyBag *) ppv, pProp, NULL);
              IPersistPropertyBag_Release((IPersistPropertyBag *) ppv);
           }
        }
        if (SUCCEEDED(res))
           res= IUnknown_QueryInterface(pObj,riidResult,ppvResult);
        IUnknown_Release(pObj);
    }

    if (pProp)
    {
        IPropertyBag_Release(pProp);
    }

    TRACE("<- 0x%x\n", res);

    return res;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_BindToStorage(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, REFIID riid, void **ppvObj)
{
    MediaCatMoniker *This = impl_from_IMoniker(iface);

    TRACE("(%p)->(%p, %p, %s, %p)\n", This, pbc, pmkToLeft, debugstr_guid(riid), ppvObj);

    *ppvObj = NULL;

    if (pmkToLeft)
        return MK_E_NOSTORAGE;

    if (pbc != NULL)
    {
        static DWORD reported;
        if (!reported)
        {
            FIXME("ignoring IBindCtx %p\n", pbc);
            reported++;
        }
    }

    if (IsEqualGUID(riid, &IID_IPropertyBag))
    {
        HANDLE hkey;
        DuplicateHandle(GetCurrentProcess(), This->hkey, GetCurrentProcess(), &hkey, 0, 0, DUPLICATE_SAME_ACCESS);
        return DEVENUM_IPropertyBag_Construct(hkey, (IPropertyBag**)ppvObj);
    }

    return MK_E_NOSTORAGE;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_Reduce(IMoniker *iface, IBindCtx *pbc,
        DWORD dwReduceHowFar, IMoniker **ppmkToLeft, IMoniker **ppmkReduced)
{
    TRACE("(%p)->(%p, %d, %p, %p)\n", iface, pbc, dwReduceHowFar, ppmkToLeft, ppmkReduced);

    if (ppmkToLeft)
        *ppmkToLeft = NULL;
    *ppmkReduced = iface;

    return MK_S_REDUCED_TO_SELF;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_ComposeWith(IMoniker *iface, IMoniker *pmkRight,
        BOOL fOnlyIfNotGeneric, IMoniker **ppmkComposite)
{
    FIXME("(%p)->(%p, %s, %p): stub\n", iface, pmkRight, fOnlyIfNotGeneric ? "true" : "false", ppmkComposite);

    /* FIXME: use CreateGenericComposite? */
    *ppmkComposite = NULL;

    return E_NOTIMPL;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_Enum(IMoniker *iface, BOOL fForward,
        IEnumMoniker **ppenumMoniker)
{
    FIXME("(%p)->(%s, %p): stub\n", iface, fForward ? "true" : "false", ppenumMoniker);

    *ppenumMoniker = NULL;

    return S_OK;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_IsEqual(IMoniker *iface, IMoniker *pmkOtherMoniker)
{
    CLSID clsid;
    LPOLESTR this_name, other_name;
    IBindCtx *bind;
    HRESULT res;

    TRACE("(%p)->(%p)\n", iface, pmkOtherMoniker);

    if (!pmkOtherMoniker)
        return E_INVALIDARG;

    IMoniker_GetClassID(pmkOtherMoniker, &clsid);
    if (!IsEqualCLSID(&clsid, &CLSID_CDeviceMoniker))
        return S_FALSE;

    res = CreateBindCtx(0, &bind);
    if (FAILED(res))
       return res;

    res = S_FALSE;
    if (SUCCEEDED(IMoniker_GetDisplayName(iface, bind, NULL, &this_name)) &&
        SUCCEEDED(IMoniker_GetDisplayName(pmkOtherMoniker, bind, NULL, &other_name)))
    {
        int result = lstrcmpiW(this_name, other_name);
        CoTaskMemFree(this_name);
        CoTaskMemFree(other_name);
        if (!result)
            res = S_OK;
    }
    IBindCtx_Release(bind);
    return res;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_Hash(IMoniker *iface, DWORD *pdwHash)
{
    TRACE("(%p)->(%p)\n", iface, pdwHash);

    *pdwHash = 0;

    return S_OK;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_IsRunning(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, IMoniker *pmkNewlyRunning)
{
    FIXME("(%p)->(%p, %p, %p): stub\n", iface, pbc, pmkToLeft, pmkNewlyRunning);

    return S_FALSE;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_GetTimeOfLastChange(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, FILETIME *pFileTime)
{
    TRACE("(%p)->(%p, %p, %p)\n", iface, pbc, pmkToLeft, pFileTime);

    pFileTime->dwLowDateTime = 0xFFFFFFFF;
    pFileTime->dwHighDateTime = 0x7FFFFFFF;

    return MK_E_UNAVAILABLE;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_Inverse(IMoniker *iface, IMoniker **ppmk)
{
    TRACE("(%p)->(%p)\n", iface, ppmk);

    *ppmk = NULL;

    return MK_E_NOINVERSE;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_CommonPrefixWith(IMoniker *iface,
        IMoniker *pmkOtherMoniker, IMoniker **ppmkPrefix)
{
    TRACE("(%p)->(%p, %p)\n", iface, pmkOtherMoniker, ppmkPrefix);

    *ppmkPrefix = NULL;

    return MK_E_NOPREFIX;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_RelativePathTo(IMoniker *iface, IMoniker *pmkOther,
        IMoniker **ppmkRelPath)
{
    TRACE("(%p)->(%p, %p)\n", iface, pmkOther, ppmkRelPath);

    *ppmkRelPath = pmkOther;

    return MK_S_HIM;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_GetDisplayName(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, LPOLESTR *ppszDisplayName)
{
    MediaCatMoniker *This = impl_from_IMoniker(iface);
    WCHAR wszBuffer[MAX_PATH];
    static const WCHAR wszFriendlyName[] = {'F','r','i','e','n','d','l','y','N','a','m','e',0};
    DWORD received = sizeof(wszBuffer);

    TRACE("(%p)->(%p, %p, %p)\n", iface, pbc, pmkToLeft, ppszDisplayName);

    *ppszDisplayName = NULL;

    /* FIXME: should this be the weird stuff we have to parse in IParseDisplayName? */
    if (RegQueryValueExW(This->hkey, wszFriendlyName, NULL, NULL, (LPBYTE)wszBuffer, &received) == ERROR_SUCCESS)
    {
        *ppszDisplayName = CoTaskMemAlloc(received);
        strcpyW(*ppszDisplayName, wszBuffer);
        return S_OK;
    }

    return E_FAIL;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_ParseDisplayName(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, LPOLESTR pszDisplayName, ULONG *pchEaten, IMoniker **ppmkOut)
{
    FIXME("(%p)->(%p, %p, %s, %p, %p)\n", iface, pbc, pmkToLeft, debugstr_w(pszDisplayName), pchEaten, ppmkOut);

    *pchEaten = 0;
    *ppmkOut = NULL;

    return MK_E_SYNTAX;
}

static HRESULT WINAPI DEVENUM_IMediaCatMoniker_IsSystemMoniker(IMoniker *iface, DWORD *pdwMksys)
{
    TRACE("(%p)->(%p)\n", iface, pdwMksys);

    return S_FALSE;
}

static const IMonikerVtbl IMoniker_Vtbl =
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

MediaCatMoniker * DEVENUM_IMediaCatMoniker_Construct(void)
{
    MediaCatMoniker * pMoniker = NULL;
    pMoniker = CoTaskMemAlloc(sizeof(MediaCatMoniker));
    if (!pMoniker)
        return NULL;

    pMoniker->IMoniker_iface.lpVtbl = &IMoniker_Vtbl;
    pMoniker->ref = 0;
    pMoniker->hkey = NULL;

    DEVENUM_IMediaCatMoniker_AddRef(&pMoniker->IMoniker_iface);

    DEVENUM_LockModule();

    return pMoniker;
}

static inline EnumMonikerImpl *impl_from_IEnumMoniker(IEnumMoniker *iface)
{
    return CONTAINING_RECORD(iface, EnumMonikerImpl, IEnumMoniker_iface);
}

static HRESULT WINAPI DEVENUM_IEnumMoniker_QueryInterface(IEnumMoniker *iface, REFIID riid,
        void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IEnumMoniker))
    {
        *ppv = iface;
        IEnumMoniker_AddRef(iface);
        return S_OK;
    }

    FIXME("- no interface IID: %s\n", debugstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI DEVENUM_IEnumMoniker_AddRef(IEnumMoniker *iface)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI DEVENUM_IEnumMoniker_Release(IEnumMoniker *iface)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if (!ref)
    {
        if(This->special_hkey)
            RegCloseKey(This->special_hkey);
        RegCloseKey(This->hkey);
        CoTaskMemFree(This);
        DEVENUM_UnlockModule();
        return 0;
    }
    return ref;
}

static HRESULT WINAPI DEVENUM_IEnumMoniker_Next(IEnumMoniker *iface, ULONG celt, IMoniker **rgelt,
        ULONG *pceltFetched)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);
    WCHAR buffer[MAX_PATH + 1];
    LONG res;
    ULONG fetched = 0;
    MediaCatMoniker * pMoniker;

    TRACE("(%p)->(%d, %p, %p)\n", iface, celt, rgelt, pceltFetched);

    while (fetched < celt)
    {
        if(This->index+fetched < This->subkey_cnt)
            res = RegEnumKeyW(This->hkey, This->index+fetched, buffer, sizeof(buffer) / sizeof(WCHAR));
        else if(This->special_hkey)
            res = RegEnumKeyW(This->special_hkey, This->index+fetched-This->subkey_cnt, buffer, sizeof(buffer) / sizeof(WCHAR));
        else
            break;
        if (res != ERROR_SUCCESS)
        {
            break;
        }
        pMoniker = DEVENUM_IMediaCatMoniker_Construct();
        if (!pMoniker)
            return E_OUTOFMEMORY;

        if (RegOpenKeyW(This->index+fetched < This->subkey_cnt ? This->hkey : This->special_hkey,
                        buffer, &pMoniker->hkey) != ERROR_SUCCESS)
        {
            IMoniker_Release(&pMoniker->IMoniker_iface);
            break;
        }
        rgelt[fetched] = &pMoniker->IMoniker_iface;
        fetched++;
    }

    This->index += fetched;

    TRACE("-- fetched %d\n", fetched);

    if (pceltFetched)
        *pceltFetched = fetched;

    if (fetched != celt)
        return S_FALSE;
    else
        return S_OK;
}

static HRESULT WINAPI DEVENUM_IEnumMoniker_Skip(IEnumMoniker *iface, ULONG celt)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);
    DWORD special_subkeys = 0;

    TRACE("(%p)->(%d)\n", iface, celt);

    /* Before incrementing, check if there are any more values to run through.
       Some programs use the Skip() function to get the number of devices */
    if(This->special_hkey)
        RegQueryInfoKeyW(This->special_hkey, NULL, NULL, NULL, &special_subkeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    if((This->index + celt) >= This->subkey_cnt + special_subkeys)
    {
        return S_FALSE;
    }

    This->index += celt;

    return S_OK;
}

static HRESULT WINAPI DEVENUM_IEnumMoniker_Reset(IEnumMoniker *iface)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);

    TRACE("(%p)->()\n", iface);

    This->index = 0;

    return S_OK;
}

static HRESULT WINAPI DEVENUM_IEnumMoniker_Clone(IEnumMoniker *iface, IEnumMoniker **ppenum)
{
    FIXME("(%p)->(%p): stub\n", iface, ppenum);

    return E_NOTIMPL;
}

/**********************************************************************
 * IEnumMoniker_Vtbl
 */
static const IEnumMonikerVtbl IEnumMoniker_Vtbl =
{
    DEVENUM_IEnumMoniker_QueryInterface,
    DEVENUM_IEnumMoniker_AddRef,
    DEVENUM_IEnumMoniker_Release,
    DEVENUM_IEnumMoniker_Next,
    DEVENUM_IEnumMoniker_Skip,
    DEVENUM_IEnumMoniker_Reset,
    DEVENUM_IEnumMoniker_Clone
};

HRESULT DEVENUM_IEnumMoniker_Construct(HKEY hkey, HKEY special_hkey, IEnumMoniker ** ppEnumMoniker)
{
    EnumMonikerImpl * pEnumMoniker = CoTaskMemAlloc(sizeof(EnumMonikerImpl));
    if (!pEnumMoniker)
        return E_OUTOFMEMORY;

    pEnumMoniker->IEnumMoniker_iface.lpVtbl = &IEnumMoniker_Vtbl;
    pEnumMoniker->ref = 1;
    pEnumMoniker->index = 0;
    pEnumMoniker->hkey = hkey;
    pEnumMoniker->special_hkey = special_hkey;

    *ppEnumMoniker = &pEnumMoniker->IEnumMoniker_iface;

    if(RegQueryInfoKeyW(pEnumMoniker->hkey, NULL, NULL, NULL, &pEnumMoniker->subkey_cnt, NULL, NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
        pEnumMoniker->subkey_cnt = 0;


    DEVENUM_LockModule();

    return S_OK;
}
