//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       call_as.c wrapper functions for urlmon
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    1-08-96   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <urlint.h>
#include "transmit.h"

//+---------------------------------------------------------------------------
//
//  Function:   IBindHost_MonikerBindToStorage_Proxy
//
//  Synopsis:
//
//  Arguments:  [This] --
//              [pMk] --
//              [pBC] --
//              [pBSC] --
//              [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    7-23-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IBindHost_MonikerBindToStorage_Proxy(
    IBindHost __RPC_FAR * This,
    /* [in] */ IMoniker __RPC_FAR *pMk,
    /* [in] */ IBindCtx __RPC_FAR *pBC,
    /* [in] */ IBindStatusCallback __RPC_FAR *pBSC,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObj)
{
    HRESULT hr;
    TransDebugOut((DEB_DATA,"%p _IN IBindHost_MonikerBindToStorage_Proxy\n",This));
    *ppvObj = 0;

    hr = IBindHost_RemoteMonikerBindToStorage_Proxy(This, pMk, pBC, pBSC, riid, (IUnknown **)ppvObj);

    TransDebugOut((DEB_DATA,"%p OUT IBindHost_MonikerBindToStorage_Proxy (hr:%lx)\n",This, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IBindHost_MonikerBindToStorage_Stub
//
//  Synopsis:
//
//  Arguments:  [This] --
//              [pMk] --
//              [pBC] --
//              [pBSC] --
//              [riid] --
//              [ppUnk] --
//
//  Returns:
//
//  History:    7-23-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IBindHost_MonikerBindToStorage_Stub(
    IBindHost __RPC_FAR * This,
    /* [unique][in] */ IMoniker __RPC_FAR *pMk,
    /* [unique][in] */ IBindCtx __RPC_FAR *pBC,
    /* [unique][in] */ IBindStatusCallback __RPC_FAR *pBSC,
    /* [in] */         REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk)
{
    HRESULT hr;
    TransDebugOut((DEB_DATA,"%p _IN IBindHost_MonikerBindToStorage_Stub\n",This));

    hr = This->MonikerBindToStorage(pMk, pBC, pBSC, riid, (void **)ppUnk);

    if (FAILED(hr))
    {
        TransAssert((*ppUnk == 0));
        *ppUnk = 0;
    }

    TransDebugOut((DEB_DATA,"%p OUT IBindHost_MonikerBindToStorage_Stub (hr:%lx)\n",This, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IBindHost_MonikerBindToObject_Proxy
//
//  Synopsis:
//
//  Arguments:  [This] --
//              [pMk] --
//              [pBC] --
//              [pBSC] --
//              [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    7-23-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IBindHost_MonikerBindToObject_Proxy(
    IBindHost __RPC_FAR * This,
    /* [in] */ IMoniker __RPC_FAR *pMk,
    /* [in] */ IBindCtx __RPC_FAR *pBC,
    /* [in] */ IBindStatusCallback __RPC_FAR *pBSC,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObj)
{
    HRESULT hr;
    TransDebugOut((DEB_DATA,"%p _IN IBindHost_MonikerBindToObject_Proxy\n",This));

    *ppvObj = 0;
    hr = IBindHost_RemoteMonikerBindToObject_Proxy(This, pMk, pBC, pBSC, riid, (IUnknown **)ppvObj);

    TransDebugOut((DEB_DATA,"%p OUT IBindHost_MonikerBindToObject_Proxy (hr:%lx)\n",This, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IBindHost_MonikerBindToObject_Stub
//
//  Synopsis:
//
//  Arguments:  [This] --
//              [pMk] --
//              [pBC] --
//              [pBSC] --
//              [riid] --
//              [ppUnk] --
//
//  Returns:
//
//  History:    7-23-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IBindHost_MonikerBindToObject_Stub(
    IBindHost __RPC_FAR * This,
    /* [unique][in] */ IMoniker __RPC_FAR *pMk,
    /* [unique][in] */ IBindCtx __RPC_FAR *pBC,
    /* [unique][in] */ IBindStatusCallback __RPC_FAR *pBSC,
    /* [in] */          REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk)
{
    HRESULT hr;
    TransDebugOut((DEB_DATA,"%p _IN IBindHost_MonikerBindToObject_Stub\n",This));

    hr = This->MonikerBindToObject(pMk, pBC, pBSC, riid, (void **)ppUnk);

    if (FAILED(hr))
    {
        TransAssert((*ppUnk == 0));
        *ppUnk = 0;
    }

    TransDebugOut((DEB_DATA,"%p OUT IBindHost_MonikerBindToObject_Stub (hr:%lx)\n",This, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IBindStatusCallback_GetBindInfo_Proxy
//
//  Synopsis:
//
//  Arguments:  [This] --
//              [grfBINDF] --
//              [pbindinfo] --
//
//  Returns:
//
//  History:    7-23-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IBindStatusCallback_GetBindInfo_Proxy(
    IBindStatusCallback __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *grfBINDF,
    /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo)
{
    TransDebugOut((DEB_DATA,"%p _IN IBindStatusCallback_GetBindInfo_Proxy\n",This));
    HRESULT hr;
    RemSTGMEDIUM RemoteMedium;
    RemSTGMEDIUM *pRemoteMedium = &RemoteMedium;
    RemBINDINFO RemoteBindInfo;
    RemBINDINFO *pRemoteBindInfo = &RemoteBindInfo;

    hr = NOERROR;
    memset(&RemoteBindInfo, 0, sizeof(RemoteBindInfo));

    __try
    {

        RemoteBindInfo.cbSize        = pbindinfo->cbSize      ;
        RemoteBindInfo.szExtraInfo   = pbindinfo->szExtraInfo ;
        RemoteBindInfo.grfBindInfoF  = pbindinfo->grfBindInfoF;
        RemoteBindInfo.dwBindVerb    = pbindinfo->dwBindVerb  ;
        RemoteBindInfo.szCustomVerb  = pbindinfo->szCustomVerb;
        RemoteBindInfo.cbstgmedData  = pbindinfo->cbstgmedData;
        RemoteBindInfo.iid  = IID_NULL;
        STGMEDIUM_to_xmit(&(pbindinfo->stgmedData), (RemSTGMEDIUM **) &pRemoteMedium);

        hr = IBindStatusCallback_RemoteGetBindInfo_Proxy(This, grfBINDF, pRemoteBindInfo,pRemoteMedium);

        if (hr == NOERROR)
        {
            pbindinfo->szExtraInfo =   pRemoteBindInfo->szExtraInfo   ;
            pbindinfo->grfBindInfoF=   pRemoteBindInfo->grfBindInfoF  ;
            pbindinfo->dwBindVerb  =   pRemoteBindInfo->dwBindVerb    ;
            pbindinfo->szCustomVerb=   pRemoteBindInfo->szCustomVerb  ;
            pbindinfo->cbstgmedData=   pRemoteBindInfo->cbstgmedData  ;

            if ( pbindinfo->cbSize > URLMONOFFSETOF(BINDINFO, dwReserved) )
            {
                pbindinfo->pUnk =  0;
                pbindinfo->dwReserved =   pRemoteBindInfo->dwReserved;
                pbindinfo->dwOptions =    pRemoteBindInfo->dwOptions ;
                pbindinfo->dwOptionsFlags = pRemoteBindInfo->dwOptionsFlags;
                pbindinfo->dwCodePage = pRemoteBindInfo->dwCodePage;
                pbindinfo->iid = IID_NULL;
            }           
            STGMEDIUM_from_xmit( (RemSTGMEDIUM *) pRemoteMedium, &(pbindinfo->stgmedData));
        }
        else
        {

        }

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //Just ignore the exception.
    }
#ifdef unix
    __endexcept
#endif /* unix */
    TransDebugOut((DEB_DATA,"%p OUT IBindStatusCallback_GetBindInfo_Proxy (hr:%lx)\n",This, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IBindStatusCallback_GetBindInfo_Stub
//
//  Synopsis:
//
//  Arguments:  [This] --
//              [grfBINDF] --
//              [pbindinfo] --
//              [pRemstgmed] --
//
//  Returns:
//
//  History:    7-23-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IBindStatusCallback_GetBindInfo_Stub(
    IBindStatusCallback __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *grfBINDF,
    /* [unique][out][in] */ RemBINDINFO __RPC_FAR *prembindinfo,
    /* [unique][out][in] */ RemSTGMEDIUM __RPC_FAR *pRemstgmed
    )
{
    TransDebugOut((DEB_DATA,"%p _IN IBindStatusCallback_GetBindInfo_Stub\n",This));
    HRESULT hr;
    BINDINFO BindInfo;
    STGMEDIUM *pstgmed = NULL;

    hr = NOERROR;
    memset(&BindInfo, 0, sizeof(BindInfo));

    __try
    {
        BindInfo.cbSize        = prembindinfo->cbSize      ;
        BindInfo.szExtraInfo   = prembindinfo->szExtraInfo ;
        BindInfo.grfBindInfoF  = prembindinfo->grfBindInfoF;
        BindInfo.dwBindVerb    = prembindinfo->dwBindVerb  ;
        BindInfo.szCustomVerb  = prembindinfo->szCustomVerb;
        BindInfo.cbstgmedData  = prembindinfo->cbstgmedData;
        BindInfo.iid           = IID_NULL;
        
        if ( prembindinfo->cbSize >= URLMONOFFSETOF(BINDINFO, dwReserved) )
        {
            BindInfo.dwOptions      = prembindinfo->dwOptions;
            BindInfo.dwOptionsFlags = prembindinfo->dwOptionsFlags;
        }

        memset(&(BindInfo.stgmedData), 0 , sizeof(BindInfo.stgmedData));
        BindInfo.stgmedData.tymed  = TYMED_NULL;

        hr = This->GetBindInfo(grfBINDF, &BindInfo);

        if (hr == NOERROR)
        {
            pstgmed = &BindInfo.stgmedData;
            TransAssert((   (pstgmed->tymed == TYMED_NULL &&  pstgmed->pUnkForRelease == NULL)
                         || (pstgmed->tymed != TYMED_NULL) ));

            if (pstgmed->tymed != TYMED_NULL)
            {
                //Convert an STGMEDIUM to a RemSTGMEDIUM
                // structure so it can be sent
                STGMEDIUM_to_xmit(pstgmed,&pRemstgmed);
            }

            prembindinfo->szExtraInfo  = BindInfo.szExtraInfo   ;
            prembindinfo->grfBindInfoF = BindInfo.grfBindInfoF  ;
            prembindinfo->dwBindVerb   = BindInfo.dwBindVerb    ;
            prembindinfo->szCustomVerb = BindInfo.szCustomVerb  ;
            prembindinfo->cbstgmedData = BindInfo.cbstgmedData  ;

            if ( prembindinfo->cbSize > URLMONOFFSETOF(BINDINFO, dwReserved) )
            {
                prembindinfo->dwReserved     = BindInfo.dwReserved;
                prembindinfo->dwOptions      = BindInfo.dwOptions;
                prembindinfo->dwOptionsFlags = BindInfo.dwOptionsFlags;
                prembindinfo->iid  = IID_NULL;
                prembindinfo->pUnk = 0;
                prembindinfo->dwCodePage     = BindInfo.dwCodePage;
            }
            
        }
    }
    __finally
    {
    }
#ifdef unix
    __endfinally
#endif /* unix */
    TransDebugOut((DEB_DATA,"%p OUT IBindStatusCallback_GetBindInfo_Stub (hr:%lx)\n",This, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IBindStatusCallback_OnDataAvailable_Proxy
//
//  Synopsis:
//
//  Arguments:  [This] --
//              [grfBSCF] --
//              [dwSize] --
//              [pformatetc] --
//              [pstgmed] --
//
//  Returns:
//
//  History:    7-23-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IBindStatusCallback_OnDataAvailable_Proxy(
    IBindStatusCallback __RPC_FAR * This,
    /* [in] */ DWORD grfBSCF,
    /* [in] */ DWORD dwSize,
    /* [in] */ FORMATETC __RPC_FAR *pformatetc,
    /* [in] */ STGMEDIUM __RPC_FAR *pstgmed)
{
    TransDebugOut((DEB_DATA,"%p _IN IBindStatusCallback_OnDataAvailable_Proxy\n",This));
    HRESULT hr;
    RemSTGMEDIUM *pRemoteMedium = 0;
    RemFORMATETC *pRemoteformatetc = 0;
    RemFORMATETC Remoteformatetc;

    hr = NOERROR;

    __try
    {

        Remoteformatetc.cfFormat = (DWORD)pformatetc->cfFormat;
        Remoteformatetc.ptd      = 0;
        Remoteformatetc.dwAspect = pformatetc->dwAspect;
        Remoteformatetc.lindex   = pformatetc->lindex;
        Remoteformatetc.tymed    = pformatetc->tymed;
        pRemoteformatetc = &Remoteformatetc;

        STGMEDIUM_to_xmit(pstgmed, (RemSTGMEDIUM **) &pRemoteMedium);
        hr = IBindStatusCallback_RemoteOnDataAvailable_Proxy(This, grfBSCF, dwSize,
                                    pRemoteformatetc, pRemoteMedium);

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //Just ignore the exception.
    }
#ifdef unix
    __endexcept
#endif /* unix */
    if(pRemoteMedium != 0)
    {
        CoTaskMemFree(pRemoteMedium);
        pRemoteMedium = 0;
    }

    TransDebugOut((DEB_DATA,"%p OUT IBindStatusCallback_OnDataAvailable_Proxy (hr:%lx)\n",This, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IBindStatusCallback_OnDataAvailable_Stub
//
//  Synopsis:
//
//  Arguments:  [This] --
//              [grfBSCF] --
//              [dwSize] --
//              [pformatetc] --
//              [pstgmed] --
//
//  Returns:
//
//  History:    7-23-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IBindStatusCallback_OnDataAvailable_Stub(
    IBindStatusCallback __RPC_FAR * This,
    /* [in] */ DWORD grfBSCF,
    /* [in] */ DWORD dwSize,
    /* [in] */ RemFORMATETC *pformatetc,
    /* [in] */ RemSTGMEDIUM __RPC_FAR *pstgmed)
{
    TransDebugOut((DEB_DATA,"%p _IN IBindStatusCallback_OnDataAvailable_Stub\n",This));
    HRESULT hr;
    STGMEDIUM medium;
    FORMATETC formatetc;

    hr = NOERROR;

    __try
    {
        formatetc.cfFormat = (CLIPFORMAT) pformatetc->cfFormat;
        formatetc.ptd      = NULL;
        formatetc.dwAspect = pformatetc->dwAspect;
        formatetc.lindex   = pformatetc->lindex  ;
        formatetc.tymed    = pformatetc->tymed   ;

        memset(&medium, 0, sizeof(medium));
        STGMEDIUM_from_xmit (pstgmed, &medium);
        This->OnDataAvailable(grfBSCF,  dwSize, &formatetc, &medium);
    }
    __finally
    {
        STGMEDIUM_free_inst(&medium);
    }
#ifdef unix
    __endfinally
#endif /* unix */
    TransDebugOut((DEB_DATA,"%p OUT IBindStatusCallback_OnDataAvailable_Stub (hr:%lx)\n",This, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IBinding_GetBindResult_Proxy
//
//  Synopsis:
//
//  Arguments:  [This] --
//              [pclsidProtocol] --
//              [pdwResult] --
//              [pszResult] --
//              [pdwReserved] --
//
//  Returns:
//
//  History:    7-25-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IBinding_GetBindResult_Proxy(
    IBinding __RPC_FAR * This,
    /* [out] */ CLSID __RPC_FAR *pclsidProtocol,
    /* [out] */ DWORD __RPC_FAR *pdwResult,
    /* [out] */ LPOLESTR __RPC_FAR *pszResult,
    /* [out][in] */ DWORD __RPC_FAR *pdwReserved)
{
    TransDebugOut((DEB_DATA,"%p _IN IBindStatusCallbackMsg_GetBindResult_Proxy\n",This));
    HRESULT hr;

    __try
    {
        hr = IBinding_RemoteGetBindResult_Proxy(
                        This,
                        pclsidProtocol,
                        pdwResult,
                        pszResult,
                        NULL);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //Just ignore the exception.
    }
#ifdef unix
    __endexcept
#endif /* unix */

    TransDebugOut((DEB_DATA,"%p OUT IBindStatusCallbackMsg_GetBindResult_Proxy (hr:%lx)\n",This, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   IBinding_GetBindResult_Stub
//
//  Synopsis:
//
//  Arguments:  [This] --
//              [pclsidProtocol] --
//              [pdwResult] --
//              [pszResult] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    7-25-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IBinding_GetBindResult_Stub(
    IBinding __RPC_FAR * This,
    /* [out] */ CLSID __RPC_FAR *pclsidProtocol,
    /* [out] */ DWORD __RPC_FAR *pdwResult,
    /* [out] */ LPOLESTR __RPC_FAR *pszResult,
    /* [in] */ DWORD dwReserved)
{
    TransDebugOut((DEB_DATA,"%p _IN IBindStatusCallbackMsg_RemoteGetBindResult_Stub\n",This));
    HRESULT hr;
    hr = NOERROR;
    MSG msg;

    __try
    {
        This->GetBindResult(
            pclsidProtocol,
            pdwResult,
            pszResult,
            NULL);
    }
    __finally
    {
    }
#ifdef unix
    __endfinally
#endif /* unix */
    TransDebugOut((DEB_DATA,"%p OUT IBindStatusCallbackMsg_RemoteGetBindResult_Stub (hr:%lx)\n",This, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IWinInetHttpInfo_QueryInfo_Proxy
//
//  Synopsis:
//
//  Arguments:  [This] --
//              [dwOption] --
//              [pBuffer] --
//              [pcbBuf] --
//              [pdwFlags] --
//              [pdwReserved] --
//
//  Returns:
//
//  History:    9-20-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IWinInetHttpInfo_QueryInfo_Proxy(
    IWinInetHttpInfo __RPC_FAR * This,
    /* [in] */ DWORD dwOption,
    /* [size_is][out] */ LPVOID pBuffer,
    /* [out][in] */ DWORD __RPC_FAR *pcbBuf,
    /* [out][in] */ DWORD __RPC_FAR *pdwFlags,
    /* [out][in] */ DWORD __RPC_FAR *pdwReserved)
{
    TransDebugOut((DEB_DATA,"%p _IN IWinInetHttpInfo_QueryInfo_Proxy\n",This));
    HRESULT hr;

    if (!pcbBuf || (!pBuffer &&  *pcbBuf != 0))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        DWORD dwBuffer = 0;
        DWORD dwFlags = 0;
        DWORD dwReserved = 0;

        if (pdwFlags == NULL)
        {
            pdwFlags = &dwFlags;
        }

        if (pdwReserved == NULL)
        {
            pdwReserved = &dwReserved;
        }

        if (pBuffer == NULL)
        {
            pBuffer = &dwBuffer;
        }

        hr = IWinInetHttpInfo_RemoteQueryInfo_Proxy(This, dwOption, (BYTE*)pBuffer,
                                                    pcbBuf,pdwFlags,pdwReserved);
    }

    TransDebugOut((DEB_DATA,"%p OUT IWinInetHttpInfo_QueryInfo_Proxy (hr:%lx)\n",This, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IWinInetHttpInfo_QueryInfo_Stub
//
//  Synopsis:
//
//  Arguments:  [This] --
//              [dwOption] --
//              [pBuffer] --
//              [pcbBuf] --
//              [pdwFlags] --
//              [pdwReserved] --
//
//  Returns:
//
//  History:    9-20-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IWinInetHttpInfo_QueryInfo_Stub(
    IWinInetHttpInfo __RPC_FAR * This,
    /* [in] */ DWORD dwOption,
    /* [size_is][out] */ BYTE __RPC_FAR *pBuffer,
    /* [out][in] */ DWORD __RPC_FAR *pcbBuf,
    /* [out][in] */ DWORD __RPC_FAR *pdwFlags,
    /* [out][in] */ DWORD __RPC_FAR *pdwReserved)
{
    TransDebugOut((DEB_DATA,"%p _IN IWinInetHttpInfo_QueryInfo_Stub\n",This));
    HRESULT hr;

    hr = This->QueryInfo(dwOption, pBuffer, pcbBuf, pdwFlags, pdwReserved);

    TransDebugOut((DEB_DATA,"%p OUT IWinInetHttpInfo_QueryInfo_Stub (hr:%lx)\n",This, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IWinInetInfo_QueryOption_Proxy
//
//  Synopsis:
//
//  Arguments:  [This] --
//              [dwOption] --
//              [pBuffer] --
//              [pcbBuf] --
//
//  Returns:
//
//  History:    9-20-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IWinInetInfo_QueryOption_Proxy(
    IWinInetInfo __RPC_FAR * This,
    /* [in] */ DWORD dwOption,
    /* [size_is][out] */ LPVOID pBuffer,
    /* [out][in] */ DWORD *pcbBuf)
{
    TransDebugOut((DEB_DATA,"%p _IN IWinInetInfo_QueryOption_Proxy\n",This));
    HRESULT hr;

    if (!pcbBuf || (!pBuffer &&  *pcbBuf != 0))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        DWORD dwBuffer = 0;

        if (pBuffer == NULL)
        {
            pBuffer = &dwBuffer;
        }

        hr = IWinInetInfo_RemoteQueryOption_Proxy(This,dwOption, (BYTE*)pBuffer,pcbBuf);
    }

    TransDebugOut((DEB_DATA,"%p OUT IWinInetInfo_QueryOption_Proxy (hr:%lx)\n",This, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IWinInetInfo_QueryOption_Stub
//
//  Synopsis:
//
//  Arguments:  [This] --
//              [dwOption] --
//              [pBuffer] --
//              [pcbBuf] --
//
//  Returns:
//
//  History:    9-20-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IWinInetInfo_QueryOption_Stub(
    IWinInetInfo __RPC_FAR * This,
    /* [in] */ DWORD dwOption,
    /* [size_is][out] */ BYTE __RPC_FAR *pBuffer,
    /* [out][in] */ DWORD __RPC_FAR *pcbBuf)
{
    TransDebugOut((DEB_DATA,"%p _IN IWinInetInfo_QueryOption_Stub\n",This));
    HRESULT hr;

    hr = This->QueryOption(dwOption,pBuffer,pcbBuf);

    TransDebugOut((DEB_DATA,"%p OUT IWinInetInfo_QueryOption_Stub (hr:%lx)\n",This, hr));
    return hr;
}










