/*
 * MIME OLE International interface
 *
 * Copyright 2008 Huw Davies for CodeWeavers
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "objbase.h"
#include "ole2.h"
#include "mimeole.h"
#include "mlang.h"

#include "wine/list.h"
#include "wine/debug.h"

#include "inetcomm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(inetcomm);

typedef struct
{
    struct list entry;
    INETCSETINFO cs_info;
} charset_entry;

typedef struct
{
    IMimeInternational IMimeInternational_iface;
    LONG refs;
    CRITICAL_SECTION cs;

    struct list charsets;
    LONG next_charset_handle;
    HCHARSET default_charset;
} internat_impl;

static inline internat_impl *impl_from_IMimeInternational(IMimeInternational *iface)
{
    return CONTAINING_RECORD(iface, internat_impl, IMimeInternational_iface);
}

static inline HRESULT get_mlang(IMultiLanguage **ml)
{
    return CoCreateInstance(&CLSID_CMultiLanguage, NULL,  CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                            &IID_IMultiLanguage, (void **)ml);
}

static HRESULT WINAPI MimeInternat_QueryInterface( IMimeInternational *iface, REFIID riid, LPVOID *ppobj )
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IMimeInternational))
    {
        IMimeInternational_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }

    FIXME("interface %s not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI MimeInternat_AddRef( IMimeInternational *iface )
{
    internat_impl *This = impl_from_IMimeInternational( iface );
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI MimeInternat_Release( IMimeInternational *iface )
{
    internat_impl *This = impl_from_IMimeInternational( iface );
    ULONG refs;

    refs = InterlockedDecrement(&This->refs);
    if (!refs)
    {
        charset_entry *charset, *cursor2;

        LIST_FOR_EACH_ENTRY_SAFE(charset, cursor2, &This->charsets, charset_entry, entry)
        {
            list_remove(&charset->entry);
            HeapFree(GetProcessHeap(), 0, charset);
        }
        This->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->cs);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refs;
}

static HRESULT WINAPI MimeInternat_SetDefaultCharset(IMimeInternational *iface, HCHARSET hCharset)
{
    internat_impl *This = impl_from_IMimeInternational( iface );

    TRACE("(%p)->(%p)\n", iface, hCharset);

    if(hCharset == NULL) return E_INVALIDARG;
    /* FIXME check hCharset is valid */

    InterlockedExchangePointer(&This->default_charset, hCharset);

    return S_OK;
}

static HRESULT WINAPI MimeInternat_GetDefaultCharset(IMimeInternational *iface, LPHCHARSET phCharset)
{
    internat_impl *This = impl_from_IMimeInternational( iface );
    HRESULT hr = S_OK;

    TRACE("(%p)->(%p)\n", iface, phCharset);

    if(This->default_charset == NULL)
    {
        HCHARSET hcs;
        hr = IMimeInternational_GetCodePageCharset(iface, GetACP(), CHARSET_BODY, &hcs);
        if(SUCCEEDED(hr))
            InterlockedCompareExchangePointer(&This->default_charset, hcs, NULL);
    }
    *phCharset = This->default_charset;

    return hr;
}

static HRESULT mlang_getcodepageinfo(UINT cp, MIMECPINFO *mlang_cp_info)
{
    HRESULT hr;
    IMultiLanguage *ml;

    hr = get_mlang(&ml);

    if(SUCCEEDED(hr))
    {
        hr = IMultiLanguage_GetCodePageInfo(ml, cp, mlang_cp_info);
        IMultiLanguage_Release(ml);
    }
    return hr;
}

static HRESULT WINAPI MimeInternat_GetCodePageCharset(IMimeInternational *iface, CODEPAGEID cpiCodePage,
                                                      CHARSETTYPE ctCsetType,
                                                      LPHCHARSET phCharset)
{
    HRESULT hr;
    MIMECPINFO mlang_cp_info;

    TRACE("(%p)->(%ld, %d, %p)\n", iface, cpiCodePage, ctCsetType, phCharset);

    *phCharset = NULL;

    hr = mlang_getcodepageinfo(cpiCodePage, &mlang_cp_info);
    if(SUCCEEDED(hr))
    {
        const WCHAR *charset_nameW = NULL;
        char *charset_name;
        DWORD len;

        switch(ctCsetType)
        {
        case CHARSET_BODY:
            charset_nameW = mlang_cp_info.wszBodyCharset;
            break;
        case CHARSET_HEADER:
            charset_nameW = mlang_cp_info.wszHeaderCharset;
            break;
        case CHARSET_WEB:
            charset_nameW = mlang_cp_info.wszWebCharset;
            break;
        default:
            return MIME_E_INVALID_CHARSET_TYPE;
        }
        len = WideCharToMultiByte(CP_ACP, 0, charset_nameW, -1, NULL, 0, NULL, NULL);
        charset_name = HeapAlloc(GetProcessHeap(), 0, len);
        WideCharToMultiByte(CP_ACP, 0, charset_nameW, -1, charset_name, len, NULL, NULL);
        hr = IMimeInternational_FindCharset(iface, charset_name, phCharset);
        HeapFree(GetProcessHeap(), 0, charset_name);
    }
    return hr;
}

static HRESULT mlang_getcsetinfo(const char *charset, MIMECSETINFO *mlang_info)
{
    DWORD len = MultiByteToWideChar(CP_ACP, 0, charset, -1, NULL, 0);
    BSTR bstr = SysAllocStringLen(NULL, len - 1);
    HRESULT hr;
    IMultiLanguage *ml;

    MultiByteToWideChar(CP_ACP, 0, charset, -1, bstr, len);

    hr = get_mlang(&ml);

    if(SUCCEEDED(hr))
    {
        hr = IMultiLanguage_GetCharsetInfo(ml, bstr, mlang_info);
        IMultiLanguage_Release(ml);
    }
    SysFreeString(bstr);
    if(FAILED(hr)) hr = MIME_E_NOT_FOUND;
    return hr;
}

static HCHARSET add_charset(struct list *list, MIMECSETINFO *mlang_info, HCHARSET handle)
{
    charset_entry *charset = HeapAlloc(GetProcessHeap(), 0, sizeof(*charset));

    WideCharToMultiByte(CP_ACP, 0, mlang_info->wszCharset, -1,
                        charset->cs_info.szName, sizeof(charset->cs_info.szName), NULL, NULL);
    charset->cs_info.cpiWindows = mlang_info->uiCodePage;
    charset->cs_info.cpiInternet = mlang_info->uiInternetEncoding;
    charset->cs_info.hCharset = handle;
    charset->cs_info.dwReserved1 = 0;
    list_add_head(list, &charset->entry);

    return charset->cs_info.hCharset;
}

static HRESULT WINAPI MimeInternat_FindCharset(IMimeInternational *iface, LPCSTR pszCharset,
                                               LPHCHARSET phCharset)
{
    internat_impl *This = impl_from_IMimeInternational( iface );
    HRESULT hr = MIME_E_NOT_FOUND;
    charset_entry *charset;

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_a(pszCharset), phCharset);

    *phCharset = NULL;

    EnterCriticalSection(&This->cs);

    LIST_FOR_EACH_ENTRY(charset, &This->charsets, charset_entry, entry)
    {
        if(!lstrcmpiA(charset->cs_info.szName, pszCharset))
        {
            *phCharset = charset->cs_info.hCharset;
            hr = S_OK;
            break;
        }
    }

    if(hr == MIME_E_NOT_FOUND)
    {
        MIMECSETINFO mlang_info;

        LeaveCriticalSection(&This->cs);
        hr = mlang_getcsetinfo(pszCharset, &mlang_info);
        EnterCriticalSection(&This->cs);

        if(SUCCEEDED(hr))
            *phCharset = add_charset(&This->charsets, &mlang_info,
                                     UlongToHandle(InterlockedIncrement(&This->next_charset_handle)));
    }

    LeaveCriticalSection(&This->cs);
    return hr;
}

static HRESULT WINAPI MimeInternat_GetCharsetInfo(IMimeInternational *iface, HCHARSET hCharset,
                                                  LPINETCSETINFO pCsetInfo)
{
    internat_impl *This = impl_from_IMimeInternational( iface );
    HRESULT hr = MIME_E_INVALID_HANDLE;
    charset_entry *charset;

    TRACE("(%p)->(%p, %p)\n", iface, hCharset, pCsetInfo);

    EnterCriticalSection(&This->cs);

    LIST_FOR_EACH_ENTRY(charset, &This->charsets, charset_entry, entry)
    {
        if(charset->cs_info.hCharset ==  hCharset)
        {
            *pCsetInfo = charset->cs_info;
            hr = S_OK;
            break;
        }
    }

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI MimeInternat_GetCodePageInfo(IMimeInternational *iface, CODEPAGEID cpiCodePage,
                                                   LPCODEPAGEINFO pCodePageInfo)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_CanConvertCodePages(IMimeInternational *iface, CODEPAGEID cpiSource,
                                                       CODEPAGEID cpiDest)
{
    HRESULT hr;
    IMultiLanguage *ml;

    TRACE("(%p)->(%ld, %ld)\n", iface, cpiSource, cpiDest);

    /* Could call mlang.IsConvertINetStringAvailable() to avoid the COM overhead if need be. */

    hr = get_mlang(&ml);
    if(SUCCEEDED(hr))
    {
        hr = IMultiLanguage_IsConvertible(ml, cpiSource, cpiDest);
        IMultiLanguage_Release(ml);
    }

    return hr;
}

static HRESULT WINAPI MimeInternat_DecodeHeader(IMimeInternational *iface, HCHARSET hCharset,
                                                LPCSTR pszData,
                                                LPPROPVARIANT pDecoded,
                                                LPRFC1522INFO pRfc1522Info)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_EncodeHeader(IMimeInternational *iface, HCHARSET hCharset,
                                                LPPROPVARIANT pData,
                                                LPSTR *ppszEncoded,
                                                LPRFC1522INFO pRfc1522Info)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_ConvertBuffer(IMimeInternational *iface, CODEPAGEID cpiSource,
                                                 CODEPAGEID cpiDest, LPBLOB pIn, LPBLOB pOut,
                                                 ULONG *pcbRead)
{
    HRESULT hr;
    IMultiLanguage *ml;

    TRACE("(%p)->(%ld, %ld, %p, %p, %p)\n", iface, cpiSource, cpiDest, pIn, pOut, pcbRead);

    *pcbRead = 0;
    pOut->cbSize = 0;

    /* Could call mlang.ConvertINetString() to avoid the COM overhead if need be. */

    hr = get_mlang(&ml);
    if(SUCCEEDED(hr))
    {
        DWORD mode = 0;
        UINT in_size = pIn->cbSize, out_size;

        hr = IMultiLanguage_ConvertString(ml, &mode, cpiSource, cpiDest, pIn->pBlobData, &in_size,
                                          NULL, &out_size);
        if(hr == S_OK) /* S_FALSE means the conversion could not be performed */
        {
            pOut->pBlobData = CoTaskMemAlloc(out_size);
            if(!pOut->pBlobData)
                hr = E_OUTOFMEMORY;
            else
            {
                mode = 0;
                in_size = pIn->cbSize;
                hr = IMultiLanguage_ConvertString(ml, &mode, cpiSource, cpiDest, pIn->pBlobData, &in_size,
                                                  pOut->pBlobData, &out_size);

                if(hr == S_OK)
                {
                    *pcbRead = in_size;
                    pOut->cbSize = out_size;
                }
                else
                    CoTaskMemFree(pOut->pBlobData);
            }
        }
        IMultiLanguage_Release(ml);
    }

    return hr;
}

static HRESULT WINAPI MimeInternat_ConvertString(IMimeInternational *iface, CODEPAGEID cpiSource,
                                                 CODEPAGEID cpiDest, LPPROPVARIANT pIn,
                                                 LPPROPVARIANT pOut)
{
    HRESULT hr;
    int src_len;
    IMultiLanguage *ml;

    TRACE("(%p)->(%ld, %ld, %p %p)\n", iface, cpiSource, cpiDest, pIn, pOut);

    switch(pIn->vt)
    {
    case VT_LPSTR:
        if(cpiSource == CP_UNICODE) cpiSource = GetACP();
        src_len = strlen(pIn->pszVal);
        break;
    case VT_LPWSTR:
        cpiSource = CP_UNICODE;
        src_len = lstrlenW(pIn->pwszVal) * sizeof(WCHAR);
        break;
    default:
        return E_INVALIDARG;
    }

    hr = get_mlang(&ml);
    if(SUCCEEDED(hr))
    {
        DWORD mode = 0;
        UINT in_size = src_len, out_size;

        hr = IMultiLanguage_ConvertString(ml, &mode, cpiSource, cpiDest, (BYTE*)pIn->pszVal, &in_size,
                                          NULL, &out_size);
        if(hr == S_OK) /* S_FALSE means the conversion could not be performed */
        {
            out_size += (cpiDest == CP_UNICODE) ? sizeof(WCHAR) : sizeof(char);

            pOut->pszVal = CoTaskMemAlloc(out_size);
            if(!pOut->pszVal)
                hr = E_OUTOFMEMORY;
            else
            {
                mode = 0;
                in_size = src_len;
                hr = IMultiLanguage_ConvertString(ml, &mode, cpiSource, cpiDest, (BYTE*)pIn->pszVal, &in_size,
                                                  (BYTE*)pOut->pszVal, &out_size);

                if(hr == S_OK)
                {
                    if(cpiDest == CP_UNICODE)
                    {
                        pOut->pwszVal[out_size / sizeof(WCHAR)] = 0;
                        pOut->vt = VT_LPWSTR;
                    }
                    else
                    {
                        pOut->pszVal[out_size] = '\0';
                        pOut->vt = VT_LPSTR;
                    }
                }
                else
                    CoTaskMemFree(pOut->pszVal);
            }
        }
        IMultiLanguage_Release(ml);
    }
    return hr;
}

static HRESULT WINAPI MimeInternat_MLANG_ConvertInetReset(IMimeInternational *iface)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_MLANG_ConvertInetString(IMimeInternational *iface, CODEPAGEID cpiSource,
                                                           CODEPAGEID cpiDest,
                                                           LPCSTR pSource,
                                                           int *pnSizeOfSource,
                                                           LPSTR pDestination,
                                                           int *pnDstSize)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_Rfc1522Decode(IMimeInternational *iface, LPCSTR pszValue,
                                                 LPSTR pszCharset,
                                                 ULONG cchmax,
                                                 LPSTR *ppszDecoded)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_Rfc1522Encode(IMimeInternational *iface, LPCSTR pszValue,
                                                 HCHARSET hCharset,
                                                 LPSTR *ppszEncoded)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static IMimeInternationalVtbl mime_internat_vtbl =
{
    MimeInternat_QueryInterface,
    MimeInternat_AddRef,
    MimeInternat_Release,
    MimeInternat_SetDefaultCharset,
    MimeInternat_GetDefaultCharset,
    MimeInternat_GetCodePageCharset,
    MimeInternat_FindCharset,
    MimeInternat_GetCharsetInfo,
    MimeInternat_GetCodePageInfo,
    MimeInternat_CanConvertCodePages,
    MimeInternat_DecodeHeader,
    MimeInternat_EncodeHeader,
    MimeInternat_ConvertBuffer,
    MimeInternat_ConvertString,
    MimeInternat_MLANG_ConvertInetReset,
    MimeInternat_MLANG_ConvertInetString,
    MimeInternat_Rfc1522Decode,
    MimeInternat_Rfc1522Encode
};

static internat_impl *global_internat;

HRESULT MimeInternational_Construct(IMimeInternational **internat)
{
    global_internat = HeapAlloc(GetProcessHeap(), 0, sizeof(*global_internat));
    global_internat->IMimeInternational_iface.lpVtbl = &mime_internat_vtbl;
    global_internat->refs = 0;
    InitializeCriticalSectionEx(&global_internat->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    global_internat->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": global_internat.cs");

    list_init(&global_internat->charsets);
    global_internat->next_charset_handle = 0;
    global_internat->default_charset = NULL;

    *internat = &global_internat->IMimeInternational_iface;

    IMimeInternational_AddRef(*internat);
    return S_OK;
}

HRESULT WINAPI MimeOleGetInternat(IMimeInternational **internat)
{
    TRACE("(%p)\n", internat);

    *internat = &global_internat->IMimeInternational_iface;
    IMimeInternational_AddRef(*internat);
    return S_OK;
}

HRESULT WINAPI MimeOleFindCharset(LPCSTR name, LPHCHARSET charset)
{
    IMimeInternational *internat;
    HRESULT hr;

    TRACE("(%s, %p)\n", debugstr_a(name), charset);

    hr = MimeOleGetInternat(&internat);
    if(SUCCEEDED(hr))
    {
        hr = IMimeInternational_FindCharset(internat, name, charset);
        IMimeInternational_Release(internat);
    }
    return hr;
}

HRESULT WINAPI MimeOleGetCharsetInfo(HCHARSET hCharset, LPINETCSETINFO pCsetInfo)
{
    IMimeInternational *internat;
    HRESULT hr;

    TRACE("(%p, %p)\n", hCharset, pCsetInfo);

    hr = MimeOleGetInternat(&internat);
    if(SUCCEEDED(hr))
    {
        hr = IMimeInternational_GetCharsetInfo(internat, hCharset, pCsetInfo);
        IMimeInternational_Release(internat);
    }
    return hr;
}

HRESULT WINAPI MimeOleGetDefaultCharset(LPHCHARSET charset)
{
    IMimeInternational *internat;
    HRESULT hr;

    TRACE("(%p)\n", charset);

    hr = MimeOleGetInternat(&internat);
    if(SUCCEEDED(hr))
    {
        hr = IMimeInternational_GetDefaultCharset(internat, charset);
        IMimeInternational_Release(internat);
    }
    return hr;
}
