//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       mailprot.hxx
//
//  Contents:   mailto: protocol
//
//  History:    04-26-97   Yin XIE    Created
//
//----------------------------------------------------------------------------

#ifndef I_MAILPROT_HXX_
#define I_MAILPROT_HXX_
#pragma INCMSG("--- Beg 'mailprot.hxx'")

#ifndef X_BASEPROT_HXX_
#define X_BASEPROT_HXX_
#include "baseprot.hxx"
#endif

MtExtern(CMailtoProtocol)
MtExtern(CMailtoFactory)

struct MAILTOURLSTRUCT
{
    TCHAR   *lptszAttr;     // Attribute name
    UINT    attrLen;        // attribute name length
    TCHAR   *lptszSep;      // seperator char
};

class CMailtoProtocol : public CBaseProtocol
{
typedef CBaseProtocol super;

    enum MAILTOURLATTR
    {
        mailtoAttrTO = 0,
        mailtoAttrCC,
        mailtoAttrBCC,
        mailtoAttrSUBJECT,
        mailtoAttrBODY,
        mailtoAttrNUM
    };

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMailtoProtocol))

    // ctor/dtor
    CMailtoProtocol(IUnknown *pUnkOuter);
    ~CMailtoProtocol();

    TCHAR * GetNextRecipient(TCHAR * lptszRecipients);
    HRESULT ParseMailToAttr();
	HRESULT LoadMailProvider(HMODULE *pHMod);
    HRESULT RunMailClient();
    void    ReleaseMAPIMessage(MapiMessage *pmm);
    HRESULT SetMAPIAttachement(MapiMessage *pmm);
    HRESULT SetMAPIRecipients(MapiMessage *pmm, LPTSTR lpszRecips, UINT uiNumRecips, int iClass);
    HRESULT MultiByteFromWideChar(WCHAR *lpwcsz, UINT cwc, LPSTR *lppsz, UINT *pcb);
    
    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}

    // Internet Helpers
    virtual HRESULT ParseAndBind();

#ifdef UNIX
    virtual HRESULT LaunchUnixClient(TCHAR *aRecips, UINT nRecips);
#endif

    // IInternetProtocol overrides
    STDMETHOD(Start)(
            LPCWSTR szUrl,
            IInternetProtocolSink *pProtSink,
            IInternetBindInfo *pOIBindInfo,
            DWORD grfSTI,
            HANDLE_PTR dwReserved);

    // Data members
    CStr                _aCStrAttr[mailtoAttrNUM];  // attributes
    UINT                _cp;                        // code page
    UINT                _cPostData;                 // post data size
    BYTE                *_postData;                 // post data
    TCHAR               _achTempFileName[MAX_PATH]; // temp file name for deletion
    LPTSTR              _pszMIMEType;               // MIME type of the post data

    // static members
    static const CLASSDESC    s_classdesc;        // The class descriptor
};

class CMailtoFactory : public CBaseProtocolCF
{
typedef CBaseProtocolCF super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMailtoFactory))

    // constructor
    CMailtoFactory(FNCREATE *pfnCreate) : super(pfnCreate) {}

    // IOInetProtocolInfo overrides
    STDMETHODIMP QueryInfo(LPCWSTR         pwzUrl,
                           QUERYOPTION     QueryOption,
                           DWORD           dwQueryFlags,
                           LPVOID          pBuffer,
                           DWORD           cbBuffer,
                           DWORD *         pcbBuf,
                           DWORD           dwReserved);
};

#pragma INCMSG("--- End 'mailprot.hxx'")
#else
#pragma INCMSG("*** Dup 'mailprot.hxx'")
#endif
