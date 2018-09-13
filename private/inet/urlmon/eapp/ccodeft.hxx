//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CCodeFt.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    04-29-97   DanpoZ (Danpo Zhang)   Created
//
//----------------------------------------------------------------------------
#ifndef _CCODEFT_HXX_
#define _CCODEFT_HXX_

#include <urlmon.h>
#include <urlmki.h>

//+---------------------------------------------------------------------------
//
//  Class:       CStdZFilter
//
//  Purpose:     Standard MS Compressor/DeCompressor 
//               It support deflate and gzip schema
//
//  Interface:   [support all IDataFilter] 
//
//  History:     04-29-97   DanpoZ (Danpo Zhang)   Created
//
//----------------------------------------------------------------------------
class CStdZFilter : public IDataFilter
{ 
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP DoEncode(
        DWORD dwFlags,
        LONG  lInBufferSize,
        BYTE* pbInBuffer,
        LONG  lOutBufferSize,  
        BYTE* pbOutBuffer,
        LONG  lInBytesAvailable,
        LONG* plInBytesRead,
        LONG* plOutBytesWritten,
        DWORD dwReserved
    );

    STDMETHODIMP DoDecode(
        DWORD dwFlags,
        LONG  lInBufferSize,
        BYTE* pbInBuffer,
        LONG  lOutBufferSize,  
        BYTE* pbOutBuffer,
        LONG  lInBytesAvailable,
        LONG* plInBytesRead,
        LONG* plOutBytesWritten,
        DWORD dwReserved
    );

    STDMETHODIMP SetEncodingLevel(DWORD dwEncLevel);

    // ctor and dtor
    CStdZFilter(ULONG ulSchema);
    ~CStdZFilter();

    // init
    HRESULT     InitFilter();
private:
    CRefCount   _CRefs;
    void*       _pEncodeCtx;
    void*       _pDecodeCtx;    
    INT         _cEncLevel;
    ULONG       _ulSchema;
};


//+---------------------------------------------------------------------------
//
//  Class:       CEncodingFilterFactory
//
//  Purpose:     Factory class that creates DataFilter
//
//  Interface:   [support all IEncodingFilterFactory] 
//               [Creates IDataFilter object]
//
//  History:     04-29-97   DanpoZ (Danpo Zhang)   Created
//
//----------------------------------------------------------------------------
class CEncodingFilterFactory : public IEncodingFilterFactory
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP FindBestFilter(
        LPCWSTR         pwzCodeIn,
        LPCWSTR         pwzCodeOut,
        DATAINFO        info,
        IDataFilter**   ppDF 
    ); 

    STDMETHODIMP GetDefaultFilter(
        LPCWSTR         pwzCodeIn,
        LPCWSTR         pwzCodeOut,
        IDataFilter**   ppDF
    );

    // ctor and dtor
    CEncodingFilterFactory();
    ~CEncodingFilterFactory();
private:
    STDMETHODIMP LookupClsIDFromReg(
        LPCWSTR         pwzCodeIn, 
        LPCWSTR         pwzCodeOut, 
        CLSID*          pclsid,
        DWORD           dwFlags
    );
    
private:
    CRefCount           _CRefs;
};

#endif
