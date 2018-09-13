//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       jsprot.hxx
//
//  Contents:   The javascript: protocol
//
//  History:    01-13-1997   AnandRa (Anand Ramakrishna)    Created
//
//----------------------------------------------------------------------------

#ifndef I_JSPROT_HXX_
#define I_JSPROT_HXX_
#pragma INCMSG("--- Beg 'jsprot.hxx'")

#ifndef X_BASEPROT_HXX_
#define X_BASEPROT_HXX_
#include "baseprot.hxx"
#endif

MtExtern(CJSProtocol)

class CJSProtocolCF : public CBaseProtocolCF
{
typedef CBaseProtocolCF super;

public:
    // constructor
    CJSProtocolCF(FNCREATE *pfnCreate) : super(pfnCreate) {}

    // IOInetProtocolInfo overrides
    STDMETHODIMP ParseUrl(LPCWSTR     pwzUrl,
                          PARSEACTION ParseAction,
                          DWORD       dwFlags,
                          LPWSTR      pwzResult,
                          DWORD       cchResult,
                          DWORD *     pcchResult,
                          DWORD       dwReserved);
};


class CJSProtocol : public CBaseProtocol
{
typedef CBaseProtocol super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CJSProtocol))

    // ctor/dtor
    CJSProtocol(IUnknown *pUnkOuter);
    ~CJSProtocol();
    
    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}

    // Internet Helpers
    virtual HRESULT ParseAndBind();

    // static members
    static const CLASSDESC    s_classdesc;        // The class descriptor
};

#pragma INCMSG("--- End 'jsprot.hxx'")
#else
#pragma INCMSG("*** Dup 'jsprot.hxx'")
#endif
