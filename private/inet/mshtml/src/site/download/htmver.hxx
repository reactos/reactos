//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       htmver.hxx
//
//  Contents:   Declaration of CVersions
//
//-------------------------------------------------------------------------

#ifndef I_HTMVER_HXX_
#define I_HTMVER_HXX_
#pragma INCMSG("--- Beg 'htmver.hxx'")

// To enable client-side <![include "http://www.foo.com/foo.htm"]> feature,
// uncomment the following line

// Not enabled for IE 5.0 - no time to test it
// #define CLIENT_SIDE_INCLUDE_FEATURE


#ifndef X_ASSOC_HXX_
#define X_ASSOC_HXX_
#include "assoc.hxx"
#endif

MtExtern(CVersions);

interface IVersionVector;
class CAssoc;
class CIVersionVectorThunk;

CVersions *GetGlobalVersions();
BOOL SuggestGlobalVersions(CVersions *pVersions);
void DeinitGlobalVersions();

enum CONDVAL
{
    COND_NULL       = 0,
    COND_SYNTAX     = 1,
    COND_IF_TRUE    = 2,
    COND_IF_FALSE   = 3,
    COND_ENDIF      = 4,
#ifdef CLIENT_SIDE_INCLUDE_FEATURE
    COND_INCLUDE    = 5,
#endif
};

class CVersions
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CVersions));

    ULONG AddRef()  { _ulRefs++; return 0; }
    ULONG Release() { _ulRefs--; if (!_ulRefs) { delete this; return 0; } else return 1; }

    CVersions()     { _asaValues.Init(); _ulRefs = 1; }
    ~CVersions();
    
    // IVersionVector workers

    HRESULT SetVersion(const TCHAR *pch, ULONG cch, const TCHAR *pchVal, ULONG cchVal);
    HRESULT GetVersion(const TCHAR *pch, ULONG cch, TCHAR *pchVal, ULONG *pcchVal);

    // Internal

    HRESULT Init();
    HRESULT GetVersionVector(IVersionVector **ppVersion);
    void Commit();
    
    void RemoveVersionVector()   { _pThunk = NULL; }
    
    CAssoc *GetAssoc(const TCHAR *pch, ULONG cch)
                                 { return _asaValues.AssocFromStringCi(pch, cch, HashStringCi(pch, cch, 0)); }

    BOOL IsIf(CAssoc *pAssoc)      { return !!(pAssoc == _pAssocIf); }
    BOOL IsEndif(CAssoc *pAssoc)   { return !!(pAssoc == _pAssocEndif); }
    BOOL IsTrue(CAssoc *pAssoc)    { return !!(pAssoc == _pAssocTrue); }
    BOOL IsFalse(CAssoc *pAssoc)   { return !!(pAssoc == _pAssocFalse); }
    BOOL IsLt(CAssoc *pAssoc)      { return !!(pAssoc == _pAssocLt); }
    BOOL IsLte(CAssoc *pAssoc)     { return !!(pAssoc == _pAssocLte); }
    BOOL IsGt(CAssoc *pAssoc)      { return !!(pAssoc == _pAssocGt); }
    BOOL IsGte(CAssoc *pAssoc)     { return !!(pAssoc == _pAssocGte); }
#ifdef CLIENT_SIDE_INCLUDE_FEATURE
    BOOL IsInclude(CAssoc *pAssoc) { return !!(pAssoc == _pAssocInclude); }
#endif
    BOOL IsReserved(CAssoc *pAssoc);

    HRESULT EvaluateConditional(CONDVAL *pResult, const TCHAR *pch, ULONG cch);
    HRESULT Evaluate(LONG *pResult, const TCHAR *pch, ULONG cch);

private:
    HRESULT InitAssoc(CAssoc **ppAssoc, const TCHAR *pch, ULONG cch);
    
    ULONG _ulRefs;
    
    CAssocArray _asaValues;
    
    CAssoc *_pAssocIf;
    CAssoc *_pAssocEndif;
    CAssoc *_pAssocTrue;
    CAssoc *_pAssocFalse;
    CAssoc *_pAssocLt;
    CAssoc *_pAssocLte;
    CAssoc *_pAssocGt;
    CAssoc *_pAssocGte;
#ifdef CLIENT_SIDE_INCLUDE_FEATURE
    CAssoc *_pAssocInclude;
#endif

    CIVersionVectorThunk *_pThunk;
    BOOL _fCommitted;
};

#pragma INCMSG("--- End 'htmver.hxx'")
#else
#pragma INCMSG("*** Dup 'htmver.hxx'")
#endif

