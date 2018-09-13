//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       abtprot.hxx
//
//  Contents:   The res: protocol
//
//  History:    07-24-97   krisma (Kris Markel)    Created
//
//----------------------------------------------------------------------------

#ifndef I_ABTPROT_HXX_
#define I_ABTPROT_HXX_
#pragma INCMSG("--- Beg 'abtprot.hxx'")

#ifndef X_BASEPROT_HXX_
#define X_BASEPROT_HXX_
#include "baseprot.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

MtExtern(CAboutProtocol)

class CAboutProtocolCF : public CBaseProtocolCF
{
typedef CBaseProtocolCF super;

public:
    // constructor
    CAboutProtocolCF(FNCREATE *pfnCreate) : super(pfnCreate) {}

    // IOInetProtocolInfo overrides
    STDMETHODIMP ParseUrl(LPCWSTR     pwzUrl,
                          PARSEACTION ParseAction,
                          DWORD       dwFlags,
                          LPWSTR      pwzResult,
                          DWORD       cchResult,
                          DWORD *     pcchResult,
                          DWORD       dwReserved);

    STDMETHODIMP QueryInfo(
            LPCWSTR pwzUrl, 
            QUERYOPTION   OueryOption,
            DWORD         dwQueryFlags,
            LPVOID        pBuffer,
            DWORD         cbBuffer,
            DWORD  *      pcbBuf,
            DWORD         dwReserved);

};


class CAboutProtocol : public CBaseProtocol
{
typedef CBaseProtocol super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CAboutProtocol))

    // ctor/dtor
    CAboutProtocol(IUnknown *pUnkOuter);
    ~CAboutProtocol();
    
    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}
    // Internet Helpers
    virtual HRESULT ParseAndBind();

    // IInternetProtocol overrides
    STDMETHOD(Start)(
            LPCWSTR szUrl,
            IInternetProtocolSink *pProtSink,
            IInternetBindInfo *pOIBindInfo,
            DWORD grfSTI,
            HANDLE_PTR dwReserved);

    // static members
    static const CLASSDESC    s_classdesc;        // The class descriptor

protected:
    // helper functions
    virtual void _ReportData(ULONG cb);

};

#pragma INCMSG("--- End 'abtprot.hxx'")
#else
#pragma INCMSG("*** Dup 'abtprot.hxx'")
#endif
