/*
 * Copyright 2009 Piotr Caban for CodeWeavers
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

#include "urlmon_main.h"

HRESULT CALLBACK IWinInetHttpInfo_QueryInfo_Proxy(IWinInetHttpInfo* This,
    DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf, DWORD *pdwFlags,
    DWORD *pdwReserved)
{
    TRACE("(%p %x %p %p %p %p)\n", This, dwOption, pBuffer, pcbBuf, pdwFlags, pdwReserved);
    return IWinInetHttpInfo_RemoteQueryInfo_Proxy(This, dwOption, pBuffer, pcbBuf, pdwFlags, pdwReserved);
}

HRESULT __RPC_STUB IWinInetHttpInfo_QueryInfo_Stub(IWinInetHttpInfo* This,
    DWORD dwOption, BYTE *pBuffer, DWORD *pcbBuf, DWORD *pdwFlags,
    DWORD *pdwReserved)
{
    TRACE("(%p %x %p %p %p %p)\n", This, dwOption, pBuffer, pcbBuf, pdwFlags, pdwReserved);
    return IWinInetHttpInfo_QueryInfo(This, dwOption, pBuffer, pcbBuf, pdwFlags, pdwReserved);
}

HRESULT CALLBACK IWinInetInfo_QueryOption_Proxy(IWinInetInfo* This,
        DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf)
{
    TRACE("(%p %x %p %p)\n", This, dwOption, pBuffer, pcbBuf);
    return IWinInetInfo_RemoteQueryOption_Proxy(This, dwOption, pBuffer, pcbBuf);
}

HRESULT __RPC_STUB IWinInetInfo_QueryOption_Stub(IWinInetInfo* This,
        DWORD dwOption, BYTE *pBuffer, DWORD *pcbBuf)
{
    TRACE("(%p %x %p %p)\n", This, dwOption, pBuffer, pcbBuf);
    return IWinInetInfo_QueryOption(This, dwOption, pBuffer, pcbBuf);
}

HRESULT CALLBACK IBindHost_MonikerBindToStorage_Proxy(IBindHost* This,
        IMoniker *pMk, IBindCtx *pBC, IBindStatusCallback *pBSC,
        REFIID riid, void **ppvObj)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IBindHost_MonikerBindToStorage_Stub(IBindHost* This,
        IMoniker *pMk, IBindCtx *pBC, IBindStatusCallback *pBSC,
        REFIID riid, IUnknown **ppvObj)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT CALLBACK IBindHost_MonikerBindToObject_Proxy(IBindHost* This,
        IMoniker *pMk, IBindCtx *pBC, IBindStatusCallback *pBSC,
        REFIID riid, void **ppvObj)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IBindHost_MonikerBindToObject_Stub(IBindHost* This,
        IMoniker *pMk, IBindCtx *pBC, IBindStatusCallback *pBSC,
        REFIID riid, IUnknown **ppvObj)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT CALLBACK IBindStatusCallbackEx_GetBindInfoEx_Proxy(
        IBindStatusCallbackEx* This, DWORD *grfBINDF, BINDINFO *pbindinfo,
        DWORD *grfBINDF2, DWORD *pdwReserved)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IBindStatusCallbackEx_GetBindInfoEx_Stub(
        IBindStatusCallbackEx* This, DWORD *grfBINDF, RemBINDINFO *pbindinfo,
        RemSTGMEDIUM *pstgmed, DWORD *grfBINDF2, DWORD *pdwReserved)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT CALLBACK IBindStatusCallback_GetBindInfo_Proxy(
        IBindStatusCallback* This, DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IBindStatusCallback_GetBindInfo_Stub(
        IBindStatusCallback* This, DWORD *grfBINDF,
        RemBINDINFO *pbindinfo, RemSTGMEDIUM *pstgmed)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT CALLBACK IBindStatusCallback_OnDataAvailable_Proxy(
        IBindStatusCallback* This, DWORD grfBSCF, DWORD dwSize,
        FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IBindStatusCallback_OnDataAvailable_Stub(
        IBindStatusCallback* This, DWORD grfBSCF, DWORD dwSize,
        RemFORMATETC *pformatetc, RemSTGMEDIUM *pstgmed)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT CALLBACK IBinding_GetBindResult_Proxy(IBinding* This,
        CLSID *pclsidProtocol, DWORD *pdwResult,
        LPOLESTR *pszResult, DWORD *pdwReserved)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IBinding_GetBindResult_Stub(IBinding* This,
        CLSID *pclsidProtocol, DWORD *pdwResult,
        LPOLESTR *pszResult, DWORD dwReserved)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}
