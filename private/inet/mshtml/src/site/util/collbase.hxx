//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1996-1998
//
//  File:       collbase.hxx
//
//  Contents:   CCollectionBase
//
//              Used to handle IDispatchEx methods and other common things
//                  for some of the collections
//----------------------------------------------------------------------------

#ifndef I_COLLBASE_HXX_
#define I_COLLBASE_HXX_
#pragma INCMSG("--- Beg 'collbase.hxx'")

MtExtern(CCollectionBase)

class CCollectionBase : public CBase
{
public:

    DECLARE_CLASS_TYPES(CCollectionBase, CBase);
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CCollectionBase))

    // IDispatchEx overrides (overriding CBase impl)
    DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (BSTR bstrName, DWORD grfdex, DISPID *pid));

    DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (DISPID dispid,
                         LCID lcid,
                         WORD wFlags,
                         DISPPARAMS *pdispparams,
                         VARIANT *pvarResult,
                         EXCEPINFO *pexcepinfo,
                         IServiceProvider *pSrvProvider));
    NV_DECLARE_TEAROFF_METHOD(GetNextDispID, getnextdispid, (
                DWORD grfdex,
                DISPID id,
                DISPID *prgid));

    NV_DECLARE_TEAROFF_METHOD(GetMemberName, getmembername, (DISPID id,
                                            BSTR *pbstrName));

    NV_DECLARE_TEAROFF_METHOD(GetIDsOfNames, getidsofnames, (
            REFIID                riid,
            LPTSTR *              rgszNames,
            UINT                  cNames,
            LCID                  lcid,
            DISPID *              rgdispid));
    NV_DECLARE_TEAROFF_METHOD(Invoke, invoke, (
            DISPID dispidMember,
            REFIID riid,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS FAR* pdispparams,
            VARIANT FAR* pvarResult,
            EXCEPINFO FAR* pexcepinfo,
            UINT FAR* puArgErr));

    // Helpers for determining whether dispids fall into collection range.
    BOOL IsCollectionDispID (DISPID dispidMember)
    {
        return ((dispidMember >= DISPID_COLLECTION_MIN) && (dispidMember <= DISPID_COLLECTION_MAX));
    }

    BOOL IsLegalIndex(long lIdx);

protected:
    virtual long FindByName(LPCTSTR pszName, BOOL fCaseSensitive = TRUE ) {Assert(0);return -1;}
    virtual LPCTSTR GetName(long lIdx) {Assert(0);return _T("");}
    virtual HRESULT GetItem( long lIndex, VARIANT *pvar ){Assert(0);return E_FAIL;}
};


#pragma INCMSG("--- End 'collbase.hxx'")
#else
#pragma INCMSG("*** Dup 'collbase.hxx'")
#endif
