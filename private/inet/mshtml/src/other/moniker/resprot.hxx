//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       resprot.hxx
//
//  Contents:   The res: protocol
//
//  History:    02-12-97   AnandRa (Anand Ramakrishna)    Created
//
//----------------------------------------------------------------------------

#ifndef I_RESPROT_HXX_
#define I_RESPROT_HXX_
#pragma INCMSG("--- Beg 'resprot.hxx'")

#ifndef X_BASEPROT_HXX_
#define X_BASEPROT_HXX_
#include "baseprot.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

MtExtern(CResProtocol)
MtExtern(CViewSourceProtocol)

class CResProtocolCF : public CBaseProtocolCF
{
typedef CBaseProtocolCF super;

public:
    // constructor
    CResProtocolCF(FNCREATE *pfnCreate) : super(pfnCreate) {}

    // IOInetProtocolInfo overrides
    STDMETHODIMP ParseUrl(LPCWSTR     pwzUrl,
                          PARSEACTION ParseAction,
                          DWORD       dwFlags,
                          LPWSTR      pwzResult,
                          DWORD       cchResult,
                          DWORD *     pcchResult,
                          DWORD       dwReserved);
    STDMETHODIMP QueryInfo(LPCWSTR         pwzUrl,
                           QUERYOPTION     QueryOption,
                           DWORD           dwQueryFlags,
                           LPVOID          pBuffer,
                           DWORD           cbBuffer,
                           DWORD *         pcbBuf,
                           DWORD           dwReserved);
};


class CResProtocol : public CBaseProtocol
{
typedef CBaseProtocol super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CResProtocol))

    // ctor/dtor
    CResProtocol(IUnknown *pUnkOuter);
    ~CResProtocol();
    
    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}

    // Internet Helpers
    virtual HRESULT ParseAndBind();
    virtual void _ReportData(ULONG cb);
    static  HRESULT DoParseAndBind(
        TCHAR *pchUrl, 
        CStr &cstrResName,
        CStr &cstrResType,
        CStr &cstrRID,
        IStream **ppStm, 
        CResProtocol *pProt);
    
    // IInternetProtocol overrides
    STDMETHOD(Start)(
            LPCWSTR szUrl,
            IInternetProtocolSink *pProtSink,
            IInternetBindInfo *pOIBindInfo,
            DWORD grfSTI,
            HANDLE_PTR dwReserved);

    // Data members
    CStr                _cstrResName;   // Resource dll name
    CStr                _cstrResType;   // Resource type (empty if default type)
    CStr                _cstrRID;       // String resource id to use

    // static members
    static const CLASSDESC    s_classdesc;        // The class descriptor
};


class CViewSourceProtocolCF : public CBaseProtocolCF
{
typedef CBaseProtocolCF super;

public:
    // constructor
    CViewSourceProtocolCF(FNCREATE *pfnCreate) : super(pfnCreate) {}

    // IOInetProtocolInfo overrides
    STDMETHODIMP QueryInfo(LPCWSTR         pwzUrl,
                           QUERYOPTION     QueryOption,
                           DWORD           dwQueryFlags,
                           LPVOID          pBuffer,
                           DWORD           cbBuffer,
                           DWORD *         pcbBuf,
                           DWORD           dwReserved);
};


class CViewSourceProtocol : public CBaseProtocol
{
typedef CBaseProtocol super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CViewSourceProtocol))

    // ctor/dtor
    CViewSourceProtocol(IUnknown *pUnkOuter) : super(pUnkOuter) { _pBitsCtx = NULL; }
    ~CViewSourceProtocol() {}
    
    virtual void Passivate();
    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}

    // IInternetProtocol methods
    STDMETHOD(Abort)(HRESULT hrReason,DWORD dwOptions);

    // Internet Helpers
    virtual HRESULT ParseAndBind();
    void OnDwnChan();
    static void CALLBACK OnDwnChanCallback(void * pvObj, void * pvArg)
        { ((CViewSourceProtocol *)pvArg)->OnDwnChan(); }

    // Data members
    CBitsCtx *                _pBitsCtx;

    // static members
    static const CLASSDESC    s_classdesc;        // The class descriptor
};

#pragma INCMSG("--- End 'resprot.hxx'")
#else
#pragma INCMSG("*** Dup 'resprot.hxx'")
#endif
