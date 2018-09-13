//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       mshtmsvr.hxx
//
//  Contents:   Definition of the CDocSvr class plus other related stuff
//
//  Classes:    CDocSvr + more
//
//----------------------------------------------------------------------------

#ifndef _MSHTMSVR_HXX_
#define _MSHTMSVR_HXX_

MtExtern(CDocSvr)

class CDoc;

class CDocSvr : public CDoc
{
typedef CDoc super;
    
public:
    DECLARE_CLASS_TYPES(CDocSvr, CDoc);
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDocSvr))
    DECLARE_TEAROFF_TABLE(IServiceProvider)

    //
    // ctor
    //
    
    CDocSvr() : super(NULL, DOCTYPE_SERVER)
        {}

    HRESULT         Clone(CDocSvr *pDoc);
    
    //
    // CDoc overrides
    //
    
    virtual void    Passivate();
    STDMETHOD       (PrivateQueryInterface)(REFIID riid, void **ppv);
    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID guidService, REFIID iid, LPVOID * ppv));
    HRESULT         DoSave(void *pvSrvContext, PFN_SVR_WRITER_CALLBACK pfnWriter);
    
    //
    // Data
    //

    IDispatch * _pDispSvr;  // Dispatch to the server object.
};

#endif
