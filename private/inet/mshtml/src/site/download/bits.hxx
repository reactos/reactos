//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       bits.hxx
//
//  Contents:   CBitsInfo
//              CBitsLoad
//
//-------------------------------------------------------------------------

#ifndef I_BITS_HXX_
#define I_BITS_HXX_
#pragma INCMSG("--- Beg 'bits.hxx'")

#ifndef X_DWN_HXX_
#define X_DWN_HXX_
#include "dwn.hxx"
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include "prgsnk.h"
#endif


// Debugging ------------------------------------------------------------------

MtExtern(CBitsCtx)
MtExtern(CBitsInfo)
MtExtern(CBitsLoad)

// CBitsInfo ------------------------------------------------------------------

class CBitsInfo : public CDwnInfo
{
    typedef CDwnInfo super;
    friend class CBitsCtx;
    friend class CBitsLoad;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBitsInfo))

                    CBitsInfo(UINT dt) : CDwnInfo() { _dt = dt; }
    virtual        ~CBitsInfo();
    virtual UINT    GetType()                    { return(DWNCTX_BITS); }
    virtual DWORD   GetProgSinkClass()           { return(_dwClass); }
    virtual HRESULT Init(DWNLOADINFO * pdli);
    HRESULT         SetFile(LPCTSTR pch)         { RRETURN(_cstrFile.Set(pch)); }
    virtual HRESULT GetFile(LPTSTR * ppch);
    HRESULT         NewDwnCtx(CDwnCtx ** ppDwnCtx);
    HRESULT         NewDwnLoad(CDwnLoad ** ppDwnLoad);
    HRESULT         OnLoadFile(LPCTSTR pszFile, HANDLE * phLock, BOOL fIsTemp);
    void            OnLoadDwnStm(CDwnStm * pDwnStm);
    virtual void    OnLoadDone(HRESULT hr);
    virtual BOOL    AttachEarly(UINT dt, DWORD dwRefresh, DWORD dwFlags, DWORD dwBindf);
    HRESULT         GetStream(IStream ** ppStream);

private:

    // Data members

    UINT            _dt;
    CStr            _cstrFile;
    HANDLE          _hLock;
    CDwnStm *       _pDwnStm;
    BOOL            _fIsTemp;
    DWORD           _dwClass;
};

// CBitsLoad ---------------------------------------------------------------

class CBitsLoad : public CDwnLoad
{
    typedef CDwnLoad super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBitsLoad))

private:

    virtual        ~CBitsLoad();
    CBitsInfo *     GetBitsInfo()   { return((CBitsInfo *)_pDwnInfo); }
    virtual HRESULT Init(DWNLOADINFO * pdli, CDwnInfo * pDwnInfo);        
    virtual HRESULT OnBindHeaders();
    virtual HRESULT OnBindData();

    // Data members

    BOOL            _fGotFile;
    BOOL            _fGotData;
    CDwnStm *       _pDwnStm;
    IStream *       _pStmFile;

};

// ----------------------------------------------------------------------------

#pragma INCMSG("--- End 'bits.hxx'")
#else
#pragma INCMSG("*** Dup 'bits.hxx'")
#endif
