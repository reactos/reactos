//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       simgprot.hxx
//
//  Contents:   The sysimage: protocol
//
//  History:    06-15-98   dli (Dan Li)    Created
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

class CSysimageProtocolCF : public CBaseProtocolCF
{
typedef CBaseProtocolCF super;

public:
    // constructor
    CSysimageProtocolCF(FNCREATE *pfnCreate) : super(pfnCreate) {}

    // IOInetProtocolInfo overrides
    STDMETHODIMP ParseUrl(LPCWSTR     pwzUrl,
                          PARSEACTION ParseAction,
                          DWORD       dwFlags,
                          LPWSTR      pwzResult,
                          DWORD       cchResult,
                          DWORD *     pcchResult,
                          DWORD       dwReserved);
};

MtExtern(CSysimageProtocol)
class CSysimageProtocol : public CBaseProtocol
{
typedef CBaseProtocol super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSysimageProtocol))

    // ctor/dtor
    CSysimageProtocol(IUnknown *pUnkOuter);
    ~CSysimageProtocol();
    
    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}

    // Internet Helpers
    virtual HRESULT ParseAndBind();
    virtual void _ReportData(ULONG cb);
    static  HRESULT DoParseAndBind(
        TCHAR *pchUrl, 
        CStr &cstrResName,
        CStr &cstrSize,
        CStr &cstrSelected,
        IStream **ppStm, 
        CSysimageProtocol *pProt);
    
    // IInternetProtocol overrides
    STDMETHOD(Start)(
            LPCWSTR szUrl,
            IInternetProtocolSink *pProtSink,
            IInternetBindInfo *pOIBindInfo,
            DWORD grfSTI,
            HANDLE_PTR dwReserved);

    // Data members
    CStr                _cstrModule; // Resource dll name
    CStr                _cstrSize;   // Resource type (empty if default type)
    CStr                _cstrSelected;

    // Handle to the shell image list
    HIMAGELIST _himglLarge;
    HIMAGELIST _himglSmall;

    // static members
    static const CLASSDESC    s_classdesc;        // The class descriptor
};

#pragma INCMSG("--- End 'resprot.hxx'")
#else
#pragma INCMSG("*** Dup 'resprot.hxx'")
#endif
