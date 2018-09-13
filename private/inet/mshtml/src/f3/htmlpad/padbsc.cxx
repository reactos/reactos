//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       padbsc.cxx
//
//  Contents:   CPadBSC class.
//
//-------------------------------------------------------------------------

#include "padhead.hxx"

DeclareTag(tagPadBSC, "PadBSC", "Bind status callback")

IMPLEMENT_SUBOBJECT_IUNKNOWN(CPadBSC, CPadDoc, PadDoc, _BSC);

HRESULT
CPadBSC::QueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    if (iid == IID_IUnknown || iid == IID_IBindStatusCallback)
    {
        *ppv = (IBindStatusCallback *)this;
    }
    else
    {
        RRETURN(E_NOINTERFACE);
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}


HRESULT
CPadBSC::OnStartBinding(DWORD grfBSCOption, IBinding *pBinding)
{
    TraceTag((tagPadBSC, "OnStartBinding %08x", grfBSCOption));
    ReplaceInterface(&PadDoc()->_pBinding, pBinding);
    return S_OK;
}

HRESULT
CPadBSC::GetPriority(LONG *pnPriority)
{
    TraceTag((tagPadBSC, "GetPriority"));
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadBSC::OnLowResource(DWORD reserved)
{
    TraceTag((tagPadBSC, "OnLowResource"));
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadBSC::OnProgress(
    ULONG ulProgress, 
    ULONG ulProgressMax, 
    ULONG ulStatusCode, 
    LPCWSTR pchStatusText)
{
    TCHAR achBuf[MAX_PATH];
    static TCHAR *s_apch[] =
    {
        NULL,                               // 0
        TEXT("Finding resource <0s>"),      // 1
        TEXT("Connecting to <0s>"),         // 2
        TEXT("Redirecting <0s>"),           // 3
        TEXT("Downloading <0s>"),           // 4
        TEXT("Downloading <0s>"),           // 4
        NULL,                               // 6
        TEXT("Downloading components for <0s>"), // 7
        TEXT("Installing components for <0s>"),  // 8
        TEXT("Downloading <0s>"),                // 9
    };

    TraceTag((tagPadBSC, "OnProgress cur=%d, max=%d, status=%d", ulProgress, ulProgressMax, ulStatusCode));

    if (ulStatusCode < ARRAY_SIZE(s_apch) && s_apch[ulStatusCode])
    {
        Verify(!Format(0, achBuf, ARRAY_SIZE(achBuf),
                s_apch[ulStatusCode],
                pchStatusText));
        PadDoc()->SetStatusText(achBuf);
    }

    RRETURN(S_OK);
}

HRESULT
CPadBSC::OnStopBinding(HRESULT hresult, LPCWSTR szError)
{
    TraceTag((tagPadBSC, "OnStopBinding hr=%hr", hresult));

    if (PadDoc()->_pBCtx)
    {
        THR(RevokeBindStatusCallback(PadDoc()->_pBCtx, &PadDoc()->_BSC));
        ClearInterface(&PadDoc()->_pBCtx);
    }
    ClearInterface(&PadDoc()->_pBinding);
    return S_OK;
}

HRESULT
CPadBSC::GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    TraceTag((tagPadBSC, "GetBindInfo"));

    if ( !grfBINDF || !pbindinfo || !pbindinfo->cbSize )
        return E_INVALIDARG;
    
    *grfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;
    if (pbindinfo)
    {
        DWORD cbSize = pbindinfo->cbSize;
        memset(pbindinfo, 0, cbSize);
        pbindinfo->cbSize = cbSize;

        pbindinfo->dwBindVerb = BINDVERB_GET;
    }
    return S_OK;
}

HRESULT
CPadBSC::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM  *pstgmed)
{
    TraceTag((tagPadBSC, "OnDataAvailable grfBSCF=%08x, size=%d", grfBSCF, dwSize));
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadBSC::OnObjectAvailable(REFIID iid, IUnknown *pUnk)
{
    PerfLog(tagPerfWatchPad, this, "+CPadBSC::OnObjectAvailable");

    HRESULT         hr = S_OK;
    IOleObject *    pObject = NULL;

    TraceTag((tagPadBSC, "OnObjectAvailable"));

    if (PadDoc()->_pBinding)
    {
        hr = THR(pUnk->QueryInterface(IID_IOleObject, (void **)&pObject));
        if (hr)
            goto Cleanup;

        hr = THR(PadDoc()->Activate(pObject));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pObject);
    PerfLog(tagPerfWatchPad, this, "-CPadBSC::OnObjectAvailable");
    RRETURN(hr);
}
