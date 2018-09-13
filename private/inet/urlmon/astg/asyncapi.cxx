//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	asyncapi.cxx
//
//  Contents:	APIs for async docfiles
//
//  Classes:	
//
//  Functions:	
//
//  History:	19-Dec-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

#include "astghead.cxx"
#pragma hdrstop

#include "asyncapi.hxx"
#include "filllkb.hxx"
#include "filelkb.hxx"
#include "stgwrap.hxx"

#if DBG == 1
DECLARE_INFOLEVEL(astg);
#endif

HRESULT StgOpenAsyncDocfileOnIFillLockBytes( IFillLockBytes *pflb,
                                             DWORD grfMode,
                                             DWORD asyncFlags,
                                             IStorage **ppstgOpen)
{
    HRESULT hr;
    ILockBytes *pilb;
    IStorage *pstg;

    hr = pflb->QueryInterface(IID_ILockBytes, (void **)&pilb);
    if (FAILED(hr))
    {
        return hr;
    }
    
    hr = StgOpenStorageOnILockBytes(pilb,
                                    NULL,
                                    grfMode,
                                    NULL,
                                    0,
                                    &pstg);

    pilb->Release();
    if (FAILED(hr))
    {
        return hr;
    }

    *ppstgOpen = new CAsyncRootStorage(pstg,(CFillLockBytes *) pflb);
    if (*ppstgOpen == NULL)
    {
        return STG_E_INSUFFICIENTMEMORY;
    }
    
    return NOERROR;
}

HRESULT StgGetIFillLockBytesOnILockBytes( ILockBytes *pilb,
                                         IFillLockBytes **ppflb)
{
    SCODE sc = S_OK;
    CFillLockBytes *pflb = NULL;
    pflb = new CFillLockBytes(pilb);
    if (pflb == NULL)
    {
        return STG_E_INSUFFICIENTMEMORY;
    }
    sc = pflb->Init();
    if (FAILED(sc))
    {
        *ppflb = NULL;
        return sc;
    }
    
    *ppflb = pflb;
    return NOERROR;
}


HRESULT StgGetIFillLockBytesOnFile(OLECHAR const *pwcsName,
                                  IFillLockBytes **ppflb)
{
    SCODE sc;
    CFileLockBytes *pflb = NULL;
    pflb = new CFileLockBytes;
    if (pflb == NULL)
    {
        return STG_E_INSUFFICIENTMEMORY;
    }
    sc = pflb->Init(pwcsName);
    if (SUCCEEDED(sc))
    {
        sc = StgGetIFillLockBytesOnILockBytes(pflb, ppflb);
    }
    return sc;
}



#if DBG == 1
HRESULT StgGetDebugFileLockBytes(OLECHAR const *pwcsName, ILockBytes **ppilb)
{
    SCODE sc;
    CFileLockBytes *pflb;
    
    *ppilb = NULL;
    
    pflb = new CFileLockBytes;
    if (pflb == NULL)
    {
        return STG_E_INSUFFICIENTMEMORY;
    }

    sc = pflb->Init(pwcsName);
    if (FAILED(sc))
    {
        delete pflb;
        pflb = NULL;
    }
    
    *ppilb = pflb;
    
    return sc;
}
#endif
