//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       htmver.cxx
//
//  Contents:   HTML conditional expression evaluator
//
//              CVersions
//              CVerTok
//              CVerStack
//
//-------------------------------------------------------------------------

#include "headers.hxx"
 
#ifndef X_HTM_HXX
#define X_HTM_HXX
#include "htm.hxx"
#endif

#ifndef X_HTMVER_HXX_
#define X_HTMVER_HXX_
#include "htmver.hxx"
#endif

#ifndef X_VERVEC_H_
#define X_VERVEC_H_
#include "vervec.h"
#endif

MtDefine(CVersions, Dwn, "CVersions"); 
MtDefine(CVerStackAry, Dwn, "CVerStackAry");
MtDefine(CIVerVector, Dwn, "CIVerVector");

DeclareTag(tagHtmVer, "Dwn", "Trace conditional version evaluation");

//+------------------------------------------------------------------------
//
//  Overview:   Conditional HTML expressions
//
//  Synopsis:   This file parses and evaluations expressions inside
//              conditional HTML tags such as
//
//              <!#if IE 4 & (ge IE 4.0sr2 | gt JVM 3) #>
//
//              The main class is CVersions.
//
//              CVersions can create an IVersionVector object which can
//              accept component/version pairs such as "IE"/"4.0sr2"
//              via GetIVersionVector(). Trident's host will feed
//              version information to IVersionVector at SetClientSite
//              time.
//
//              It can then take a string such as
//
//              "if IE 4 & (ge IE 4.0sr2 | gt JVM 3)"
//              
//              and produce TRUE or FALSE (or ENDIF or syntax error)
//              via EvaluateConditional().
//
//              The implementation is a flattened recursive-descent
//              parser on top of a case-insensitive hashing tokenizer.
//
//-------------------------------------------------------------------------



//+------------------------------------------------------------------------
//
//  Overview:   Comparing version numbers
//
//  Synopsis:   Version ordering rules:
//
//              A version has releases separated by dots, with the most
//              significant release first:
//
//              4.010beta.3032 separates into ("4", "010beta", "3032")
//              4.999.a separates into ("4","999", "a")
//
//              Comparing from left to right, since "4" = "4" and
//              "010beta" < "999", 4.010beta.3032 precedes 4.999.a.
//
//              When comparing versions exactly, a version is assumed to
//              have infinite precision in the sense that an specified
//              version string of "4.2b" is interpreted to mean
//              "4.2b.0.0.0.0.0.0...".
//
//              Each release string is compared by splitting it into
//              parts consisting of just numbers or words. Again, the
//              the most significant part is on the left.
//
//              10alpha2 separates into  ("10", "alpha", "2")
//              010beta separates into   ("010", "beta")
//
//              Comparing from left to right, since "10" = "010" and
//              "alpha" < "beta", 10alpha2 precedes 010beta.
//
//              Individual parts of a release are compared as follows:
//
//              * Words precede numbers.
//              * Words are ordered in lexical order using unicode
//                values except for A-Z, which are lowercased
//                (best < bet = Bet < beta)
//              * Numbers are ordered in integer numerical order
//                (1 < 0024 = 24 < 100)
//
//              More examples comparing version strings:
//
//              4.a < 4.a0 < 4.aa < 4 = 4.0 < 4.0a < 4.1 < 4.5 < 4.010
//
//              CompareVersion reports:
//              
//              -1 if the actual version comes before the requested one
//              0 if the actual version is equal to the requested one
//              1 if the actual version comes after the requsted one
//
//
//  Synposis:   Version containment rules:
//
//              ContainVersion() compares a requested set of releases to
//              an actual version.
//
//              The "requested" version specifies a release set, so "4.2b"
//              specifies all versions in the release "4.2b", including
//              "4.2b.1" and "4.2b.alpha" but not including "4.2b1".
//
//              ContainVersion reports:
//              
//              -1 if the actual version comes before the requested set
//              0 if the actual version is in the requested set
//              1 if the actual version comes after the requsted set
//
//-------------------------------------------------------------------------


//+------------------------------------------------------------------------
//
//  Overview:   CompareNumber
//
//  Synopsis:   Compares two number strings
//
//-------------------------------------------------------------------------
LONG
CompareNumber(const TCHAR *pchAct, ULONG cchAct, const TCHAR *pchReq, ULONG cchReq)
{
    Assert(cchReq && ISDIGIT(*pchReq) && cchAct && ISDIGIT(*pchAct));
    
    while (cchReq && *pchReq == _T('0'))
    {
        pchReq++;
        cchReq--;
    }
    
    while (cchAct && *pchAct == _T('0'))
    {
        pchAct++;
        cchAct--;
    }
    
    if (cchReq != cchAct)
        return (cchReq > cchAct ? -1 : 1);

    while (cchReq)
    {
        Assert(ISDIGIT(*pchReq) && ISDIGIT(*pchAct));
        
        if (*pchReq != *pchAct)
            return (*pchReq > *pchAct ? -1 : 1);
        cchReq--;
        pchReq++;
        pchAct++;
    }

    return 0;
}

//+------------------------------------------------------------------------
//
//  Overview:   CompareWord
//
//  Synopsis:   Compares two word strings
//
//-------------------------------------------------------------------------
LONG
CompareWord(const TCHAR *pchAct, ULONG cchAct, const TCHAR *pchReq, ULONG cchReq)
{
    TCHAR chReq, chAct;
    
    Assert(cchReq && !ISDIGIT(*pchReq) && cchAct && !ISDIGIT(*pchAct));
    
    if (cchReq != cchAct)
        return (cchReq > cchAct ? -1 : 1);

    while (cchReq)
    {
        Assert(!ISDIGIT(*pchReq) && !ISDIGIT(*pchAct));

        chReq = *pchReq;
        chAct = *pchAct;

        if (ISUPPER(chReq))
            chAct += _T('a') - _T('A');
            
        if (ISUPPER(chReq))
            chAct += _T('a') - _T('A');
        
        if (chReq != chAct)
            return (chReq > chAct ? -1 : 1);
            
        cchReq--;
        pchReq++;
        pchAct++;
    }

    return 0;
}

//+------------------------------------------------------------------------
//
//  Overview:   CompareRelease
//
//  Synopsis:   Breaks up release strings into words and numbers and
//              compares them from left to right.
//
//-------------------------------------------------------------------------
LONG
CompareRelease(const TCHAR *pchAct, ULONG cchAct, const TCHAR *pchReq, ULONG cchReq)
{
    const TCHAR *pchReqPart;
    const TCHAR *pchActPart;
    LONG lCmp;

    if (!pchAct)
    {
        if (!cchReq || !ISDIGIT(*pchReq))
            return 1;

        while (cchReq && *pchReq == _T('0'))
        {
            pchReq++;
            cchReq--;
        }

        return (cchReq ? -1 : 0);
    }

    if (!pchReq)
    {
        if (!cchAct || !ISDIGIT(*pchAct))
            return 1;

        while (cchAct && *pchAct == _T('0'))
        {
            pchAct++;
            cchAct--;
        }

        return (cchAct ? -1 : 0);
    }
    
    if (!cchReq || !cchAct)
        return (cchReq ? -1 : cchAct ? 1 : 0);

    if (ISDIGIT(*pchAct))
    {
        if (!ISDIGIT(*pchReq))
            return -1;
            
        goto Number;
    }
    else
    {
        if (ISDIGIT(*pchReq))
            return 1;
    }

    for (;;)
    {
        Assert(cchAct && !ISDIGIT(*pchAct) && cchReq && !ISDIGIT(*pchReq));
        
        pchReqPart = pchReq;
        do
        {
            pchReq++;
            cchReq--;
        } while (cchReq && !ISDIGIT(*pchReq));

        pchActPart = pchAct;
        do
        {
            pchAct++;
            cchAct--;
        } while (cchAct && !ISDIGIT(*pchAct));

        lCmp = CompareWord(pchReqPart, pchReq - pchReqPart, pchActPart, pchAct - pchActPart);
        if (lCmp)
            return lCmp;

        if (!cchReq || !cchAct)
            break;

    Number:
        Assert(cchAct && ISDIGIT(*pchAct) && cchReq && ISDIGIT(*pchReq));
        
        pchReqPart = pchReq;
        do
        {
            pchReq++;
            cchReq--;
        } while (cchReq && ISDIGIT(*pchReq));

        pchActPart = pchAct;
        do
        {
            pchAct++;
            cchAct--;
        } while (cchAct && ISDIGIT(*pchAct));

        lCmp = CompareNumber(pchReqPart, pchReq - pchReqPart, pchActPart, pchAct - pchActPart);
        if (lCmp)
            return lCmp;

        if (!cchReq || !cchAct)
            break;
    }
    return (cchReq ? -1 : cchAct ? 1 : 0);
}

//+------------------------------------------------------------------------
//
//  Overview:   CompareVersion
//
//  Synopsis:   Breaks up version strings into '.' delimited release
//              strings and compares them from left to right.
//
//-------------------------------------------------------------------------
LONG
CompareVersion(const TCHAR *pchAct, ULONG cchAct, const TCHAR *pchReq, ULONG cchReq)
{
    const TCHAR *pchReqRel;
    const TCHAR *pchActRel;
    LONG lCmp;

    for (;;)
    {
        pchReqRel = pchReq;
        pchActRel = pchAct;
            
        while (cchReq && *pchReq != _T('.'))
        {
            pchReq++;
            cchReq--;
        }

        while (cchAct && *pchAct != _T('.'))
        {
            pchAct++;
            cchAct--;
        }

        if (pchAct == pchActRel && !cchAct)
            pchAct = pchActRel = NULL;
            
        if (pchReq == pchReqRel && !cchReq)
            pchReq = pchReqRel = NULL;

        lCmp = CompareRelease(pchReqRel, pchReq - pchReqRel, pchActRel, pchAct - pchActRel);

        if (lCmp || (!cchReq && !cchAct))
            return(lCmp);

        if (cchReq)
        {
            Assert(*pchReq == _T('.'));
            pchReq++;
            cchReq--;
        }
        
        if (cchAct)
        {
            Assert(*pchAct == _T('.'));
            pchAct++;
            cchAct--;
        }
    }
}

//+------------------------------------------------------------------------
//
//  Overview:   ContainVersion
//
//  Synopsis:   Like compare version, but checks if pchAct is
//              contained in the set specified by pchReq.
//
//-------------------------------------------------------------------------
LONG
ContainVersion(const TCHAR *pchAct, ULONG cchAct, const TCHAR *pchReq, ULONG cchReq)
{
    const TCHAR *pchReqRel;
    const TCHAR *pchActRel;
    LONG lCmp;

    for (;;)
    {
        pchReqRel = pchReq;
        pchActRel = pchAct;
            
        while (cchReq && *pchReq != _T('.'))
        {
            pchReq++;
            cchReq--;
        }

        while (cchAct && *pchAct != _T('.'))
        {
            pchAct++;
            cchAct--;
        }

        if (pchAct == pchActRel && !cchAct)
            pchAct = pchActRel = NULL;

        lCmp = CompareRelease(pchReqRel, pchReq - pchReqRel, pchActRel, pchAct - pchActRel);

        if (lCmp || !cchReq)
            return(lCmp);

        Assert(*pchReq == _T('.'));
        pchReq++;
        cchReq--;
        
        if (cchAct)
        {
            Assert(*pchAct == _T('.'));
            pchAct++;
            cchAct--;
        }
    }
}


//+------------------------------------------------------------------------
//
//  Class:      CIVersionVectorThunk
//
//  Synopsis:   OLE object that serves as interface to CVersions
//
//-------------------------------------------------------------------------
class CIVersionVectorThunk : public IVersionVector
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CIVerVector));
    CIVersionVectorThunk()       { _ulRefs = 1; }
    ~CIVersionVectorThunk()      { Detach(); }
    
    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID iid, LPVOID * ppv);
    STDMETHOD_(ULONG, AddRef)()  { Assert(_ulRefs); _ulRefs++; return 0; }
    STDMETHOD_(ULONG, Release)() { _ulRefs--; if (!_ulRefs) { delete this; return 0; } return _ulRefs; }

    // IVersionVector methods

    STDMETHOD(SetVersion)(const TCHAR *pch, const TCHAR *pchVal);
    STDMETHOD(GetVersion)(const TCHAR *pch, TCHAR *pchVal, ULONG *pcchVal);

    // Internal
    CIVersionVectorThunk(CVersions *pValues)
                    { _pVersions = pValues; _fDetached = FALSE; _ulRefs = 1; }
    void Detach()   { if (_pVersions) _pVersions->RemoveVersionVector(); _pVersions = NULL; }

    ULONG _ulRefs;
    BOOL _fDetached;
    CVersions *_pVersions;
};


//+------------------------------------------------------------------------
//
//  Member:     CIVersionVectorThunk
//
//  Synopsis:   Simple QI Impl
//
//-------------------------------------------------------------------------
STDMETHODIMP
CIVersionVectorThunk::QueryInterface(REFIID iid, void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IVersionVector)
        *ppv = (IVersionVector*)this;
    else
    {
        *ppv = NULL;
        return(E_NOINTERFACE);
    }
    
    AddRef();
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CIVersionVectorThunk::SetVersion
//
//  Synopsis:   Passthrough, fail when detached
//
//-------------------------------------------------------------------------
HRESULT
CIVersionVectorThunk::SetVersion(const TCHAR *pch, const TCHAR *pchVal)
{
    if (!_pVersions)
        return E_FAIL;

    RRETURN(_pVersions->SetVersion(
        pch, pch ? _tcslen(pch) : 0, pchVal, pchVal ? _tcslen(pchVal) : 0));
}

//+------------------------------------------------------------------------
//
//  Member:     CIVersionVectorThunk::GetVersion
//
//  Synopsis:   Passthrough, fail when detached
//
//-------------------------------------------------------------------------
HRESULT
CIVersionVectorThunk::GetVersion(const TCHAR *pch, TCHAR *pchVal, ULONG *pcchVal)
{
    if (!_pVersions)
        return E_FAIL;

    RRETURN(_pVersions->GetVersion(pch, _tcslen(pch), pchVal, pcchVal));
}



//+------------------------------------------------------------------------
//
//  Class:      CVersions
//
//-------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  Member:     CVersions::Init
//
//  Synopsis:   Set up reserved string values
//
//-------------------------------------------------------------------------
HRESULT
CVersions::Init()
{
    HRESULT hr;
    
    hr = THR(InitAssoc(&_pAssocIf, _T("if"), 2));
    if (hr)
        goto Cleanup;
        
    hr = THR(InitAssoc(&_pAssocEndif, _T("endif"), 5));
    if (hr)
        goto Cleanup;
        
    hr = THR(InitAssoc(&_pAssocTrue, _T("true"), 4));
    if (hr)
        goto Cleanup;
        
    hr = THR(InitAssoc(&_pAssocFalse, _T("false"), 5));
    if (hr)
        goto Cleanup;
        
    hr = THR(InitAssoc(&_pAssocLt, _T("lt"), 2));
    if (hr)
        goto Cleanup;
        
    hr = THR(InitAssoc(&_pAssocLte, _T("lte"), 3));
    if (hr)
        goto Cleanup;
        
    hr = THR(InitAssoc(&_pAssocGt, _T("gt"), 2));
    if (hr)
        goto Cleanup;
        
    hr = THR(InitAssoc(&_pAssocGte, _T("gte"), 3));
    if (hr)
        goto Cleanup;

#ifdef CLIENT_SIDE_INCLUDE_FEATURE
    hr = THR(InitAssoc(&_pAssocInclude, _T("include"), 7));
    if (hr)
        goto Cleanup;
#endif

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CVersions::~CVersions
//
//  Synopsis:   Free all the strings stored in the CAssocArray
//
//-------------------------------------------------------------------------
CVersions::~CVersions()
{
    ULONG c;
    CAssoc **ppAssoc;

    Commit(); // detach any IVersionVector thunk
    
    c = _asaValues._mHash;
    ppAssoc = _asaValues._pHashTable;

    for (; c; c--, ppAssoc++)
    {
        if (*ppAssoc && (*ppAssoc)->_number)
        {
            ((CStr *)&((*ppAssoc)->_number))->Free();
        }
    }

    _asaValues.Deinit();
}

//+------------------------------------------------------------------------
//
//  Member:     CVersions::InitAssoc
//
//  Synopsis:   Set up reserved string values
//
//-------------------------------------------------------------------------
HRESULT
CVersions::InitAssoc(CAssoc **ppAssoc, const TCHAR *pch, ULONG cch)
{
    *ppAssoc = _asaValues.AddAssoc(0, pch, cch, HashStringCi(pch, cch, 0));
    if (!*ppAssoc)
        RRETURN(E_OUTOFMEMORY);

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CVersions::GetIVersionVector
//
//  Synopsis:   Create or retrieve IVersionVector object
//
//-------------------------------------------------------------------------
HRESULT
CVersions::GetVersionVector(IVersionVector **ppIVersionVector)
{
    if (_fCommitted)
    {
        Assert(!_pThunk);
        RRETURN(E_FAIL);
    }
    
    if (_pThunk)
        RRETURN(_pThunk->QueryInterface(IID_IVersionVector, (void**)ppIVersionVector));
        
    CIVersionVectorThunk *pThunk = new CIVersionVectorThunk(this);
    if (!pThunk)
        RRETURN(E_OUTOFMEMORY);

    _pThunk = pThunk;

    *ppIVersionVector = (IVersionVector*)pThunk;
    
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CVersions::Commit
//
//  Synopsis:   Become read-only.
//
//              Detach IVersionVector object and prevent IVersionVector objects
//              from being created in the future.
//
//-------------------------------------------------------------------------
void
CVersions::Commit()
{
    if (_pThunk)
        _pThunk->Detach();

    Assert(!_pThunk);
    _fCommitted = TRUE;
}

//+------------------------------------------------------------------------
//
//  Member:     CVersions::IsReserved
//
//  Synopsis:   Returns TRUE for pAssocs that match a reserved value
//
//-------------------------------------------------------------------------
BOOL
CVersions::IsReserved(CAssoc *pAssoc)
{
    return (pAssoc && !pAssoc->_number &&
            (pAssoc == _pAssocIf ||
             pAssoc == _pAssocEndif ||
             pAssoc == _pAssocTrue ||
             pAssoc == _pAssocFalse ||
             pAssoc == _pAssocLt ||
             pAssoc == _pAssocLte ||
             pAssoc == _pAssocGt ||
             pAssoc == _pAssocGte
#ifdef CLIENT_SIDE_INCLUDE_FEATURE
             || pAssoc == _pAssocInclude
#endif             
            ));
}

//+------------------------------------------------------------------------
//
//  Member:     CVersions::SetVersion
//
//  Synopsis:   Sets the version value corresponding to the given string
//
//-------------------------------------------------------------------------
HRESULT
CVersions::SetVersion(const TCHAR *pch, ULONG cch, const TCHAR *pchVal, ULONG cchVal)
{
    HRESULT hr = S_OK;
    DWORD hash;
    CAssoc *pAssoc;
    DWORD_PTR val;

    Assert(sizeof(CStr) == sizeof(val));
    
    val = 0;
    
    if (pchVal)
    {
        hr = THR(((CStr *)&val)->Set(pchVal, cchVal));
        if (hr)
            goto Cleanup;
    }

    hash = HashStringCi(pch, cch, 0);
    
    pAssoc = _asaValues.AssocFromStringCi(pch, cch, hash);
    
    if (pAssoc)
    {
        // precaution: not allowed to set reserved values
        if (IsReserved(pAssoc))
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (pAssoc->_number)
        {
            ((CStr *)&(pAssoc->_number))->Free();
        }
            
        pAssoc->_number = val;
    }
    else if (val) // don't bother adding zero - equivalent to undefined
    {
        pAssoc = _asaValues.AddAssoc(val, pch, cch, hash);
        if (!pAssoc)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    val = 0;

Cleanup:

    if (val)
    {
        ((CStr *)&val)->Free();
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CVersions::GetVersion
//
//  Synopsis:   Gets the version value corresponding to the given string
//
//              If insufficient buffer size, *pcch returns required
//              buffer size.
//
//              If successful, *pcch returns string length of version
//              value, or (ULONG)-1 if there is no version value.
//
//-------------------------------------------------------------------------
HRESULT
CVersions::GetVersion(const TCHAR *pch, ULONG cch, TCHAR *pchVer, ULONG *pcchVer)
{
    CAssoc *pAssoc;
    DWORD_PTR val;
    ULONG cchVer;
    
    pAssoc = GetAssoc(pch, cch);
    val = pAssoc ? pAssoc->Number() : 0;

    cchVer = val ? ((CStr *)&val)->Length() + 1 : 0;

    if (!*pcchVer)
    {
        *pcchVer = cchVer;
        return S_OK;
    }

    if (*pcchVer < cchVer)
    {
        *pcchVer = cchVer;
        return E_FAIL;
    }

    if (cch)
    {
        memcpy(pchVer, (void*)val, cchVer * sizeof(TCHAR));
    }

    *pcchVer = cchVer - 1;

    return S_OK;
}



//+------------------------------------------------------------------------
//
//  Member:     CVerTok
//
//  Synopsis:   The tokenizer for version expressions
//
//              The tokenizer recognizes the following expressions:
//
//              Word    = ([a-z] | [A-Z]) ([0-9] | [a-z] | [A-Z])* (space)*
//              Version = [0-9] ([0-9] | [a-z] | [A-Z] | '.')* (space)*
//              Char    = (non-[a-z][A-Z][0-9]) (space)*
//
//-------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  Class:      CVerTok
//
//  Synopsis:   Tokenizer for the CVersions evaluator
//
//-------------------------------------------------------------------------
class CVerTok
{
public:
    enum CT
    {
        CT_NULL = 0,
        CT_WORD = 1,
        CT_VERS = 2,
        CT_CHAR = 3,
    };

    void Init(CVersions *pValues, const TCHAR *pch, ULONG cch);
    void Advance();

    BOOL IsIf()     { return(_ct == CT_WORD && _pVersions->IsIf(_pAssoc)); }
    BOOL IsEndif()  { return(_ct == CT_WORD && _pVersions->IsEndif(_pAssoc)); }
    BOOL IsTrue()   { return(_ct == CT_WORD && _pVersions->IsTrue(_pAssoc)); }
    BOOL IsFalse()  { return(_ct == CT_WORD && _pVersions->IsFalse(_pAssoc)); }
    BOOL IsLt()     { return(_ct == CT_WORD && _pVersions->IsLt(_pAssoc)); }
    BOOL IsLte()    { return(_ct == CT_WORD && _pVersions->IsLte(_pAssoc)); }
    BOOL IsGt()     { return(_ct == CT_WORD && _pVersions->IsGt(_pAssoc)); }
    BOOL IsGte()    { return(_ct == CT_WORD && _pVersions->IsGte(_pAssoc)); }
#ifdef CLIENT_SIDE_INCLUDE_FEATURE
    BOOL IsInclude(){ return(_ct == CT_WORD && _pVersions->IsInclude(_pAssoc)); }
#endif
    BOOL IsOr()     { return(_ct == CT_CHAR && _ch == _T('|')); }
    BOOL IsAnd()    { return(_ct == CT_CHAR && _ch == _T('&')); }
    BOOL IsNot()    { return(_ct == CT_CHAR && _ch == _T('!')); }
    BOOL IsLParen() { return(_ct == CT_CHAR && _ch == _T('(')); }
    BOOL IsRParen() { return(_ct == CT_CHAR && _ch == _T(')')); }
    BOOL IsWord()   { return(_ct == CT_WORD); }
    BOOL IsVersion(){ return(_ct == CT_VERS); }
    BOOL IsNull()   { return(_ct == CT_NULL); }

    LONG Ch()        { Assert(_ct == CT_CHAR); return _ch; }
    const TCHAR *Pch() { return _pch; }
    ULONG CchRemaining() { return _cch; }
    
    void ActVer(const TCHAR **ppch, ULONG *pcch);
    void ReqVer(const TCHAR **ppch, ULONG *pcch) { Assert(_ct == CT_VERS); *ppch = _pchVer; *pcch = _cchVer; }

private:
    CVersions *_pVersions;
    const TCHAR *_pch;
    ULONG _cch;
    
    CT _ct;
    
    CAssoc *_pAssoc;
    const TCHAR *_pchVer;
    ULONG _cchVer;
    TCHAR _ch;
};

//+------------------------------------------------------------------------
//
//  Member:     CVerTok::Init
//
//  Synopsis:   Attaches a string to be tokenized,
//              and an instance of CVersions for identifier lookups
//
//-------------------------------------------------------------------------
void
CVerTok::Init(CVersions *pValues, const TCHAR *pch, ULONG cch)
{
    _pVersions = pValues;
    _pch = pch;
    _cch = cch;
}

//+------------------------------------------------------------------------
//
//  Member:     CVerTok::Advance
//
//  Synopsis:   Scans characters and loads value of next token
//
//-------------------------------------------------------------------------
void
CVerTok::Advance()
{
    if (!_cch)
    {
        _ct = CT_NULL;
        return;
    }

    const TCHAR *pch = _pch;
    const TCHAR *pchStart = pch;
    ULONG cch = _cch;
    
    if (ISALPHA(*pch))
    {
        // Scan word
        do
        {
            cch--;
            pch++;
        } while (cch && (ISALPHA(*pch) || ISDIGIT(*pch) || *pch == _T('_')));
        
        _pAssoc = _pVersions->GetAssoc(pchStart, pch - pchStart);

        _ct = CT_WORD;
    }
    else if (ISDIGIT(*pch))
    {
        _ct = CT_VERS;
        _pchVer = pch;

        while (ISALPHA(*pch) || ISDIGIT(*pch) || *pch == _T('.'))
        {
            pch++;
            cch--;
        }
        
        _cchVer = pch - _pchVer;
    }
    else if (!ISSPACE(*pch))
    {
        // Scan one symbol
        _ct = CT_CHAR;
        _ch = *pch;
        cch--;
        pch++;
    }

    // skip space
    while (cch && ISSPACE(*pch))
    {
        cch--;
        pch++;
    }

    _cch = cch;
    _pch = pch;
}

//+------------------------------------------------------------------------
//
//  Member:     CVerTok::ActVer
//
//  Synopsis:   Looks up the actual version for the specified component
//
//-------------------------------------------------------------------------
void
CVerTok::ActVer(const TCHAR **ppchVer, ULONG *pcchVer)
{
    Assert(_ct == CT_WORD);

    if (_pAssoc)
    {
        *pcchVer = ((CStr *)&(_pAssoc->_number))->Length();
        *ppchVer = *((CStr *)&(_pAssoc->_number));
    }
    else
    {
        *pcchVer = 0;
        *ppchVer = NULL;
    }
}


//+------------------------------------------------------------------------
//
//  Class:      CVerState
//
//  Synopsis:   Unit of state for the parser for CVersions evaluator
//
//              Consists of a state and a single current value
//
//-------------------------------------------------------------------------
enum VSTATE
{
    VSTATE_NULL = 0,
    VSTATE_OR,          // OR
    VSTATE_ORLOOP,
    VSTATE_AND,         // AND
    VSTATE_ANDLOOP,
    VSTATE_NOT,         // NOT
    VSTATE_NOTNOT,
    VSTATE_TERM,        // TERM
    VSTATE_TERMNOT,
    VSTATE_TERMPAREN,
    VSTATE_CMP,         // CMP
};

class CVerState
{
public:
    VSTATE  _cs  : 7;
    ULONG   _val : 1;
};



//+------------------------------------------------------------------------
//
//  Class:      CVerStack
//
//  Synopsis:   Stack of state for the parser for CVersions evaluator
//
//              Allows recursive-descent on a compressed-storage stack.
//
//-------------------------------------------------------------------------
#define MAX_VER_STACK_DEPTH 128 // Each paren counts for two in depth

class CVerStack : public CVerState
{
public:
    CVerStack() : _ary(Mt(CVerStackAry))  { _cs = VSTATE_NULL; _val = _valRet = 0; }
    ULONG _valRet : 1;
    
    CStackDataAry<CVerState, MAX_VER_STACK_DEPTH> _ary;

    HRESULT Call(VSTATE csNew, VSTATE csRet);
    void Ret(ULONG val);
    
    HRESULT Call(VSTATE csNew, ULONG valNew, VSTATE csRet)
                                            { HRESULT hr = Call(csNew, csRet); _val = valNew; return(hr); }
    void Go(VSTATE csNew)                   { _cs = csNew; }
    void Go(VSTATE csNew, ULONG valNew)     { _cs = csNew; _val = valNew; }

    ULONG Depth()                           { return _ary.Size(); }
};

//+------------------------------------------------------------------------
//
//  Member:     CVerStack::Call
//
//  Synopsis:   Pushes a return state and current value on the stack,
//              then jumps to a specified new state.
//
//-------------------------------------------------------------------------
HRESULT
CVerStack::Call(VSTATE csNew, VSTATE csRet)
{
    HRESULT hr;
    
    _cs = csRet;
    
    hr = THR(_ary.AppendIndirect((CVerState *)this));
    
    _cs = csNew;
    
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CVerStack::Ret
//
//  Synopsis:   Pops a return state and value from the stack, and
//              sets _valRet as specified.
//
//-------------------------------------------------------------------------
void
CVerStack::Ret(ULONG val)
{
    Assert(_ary.Size());
    
    CVerState *pState = _ary + _ary.Size() - 1;
    _valRet = val;
    _cs = pState->_cs;
    _val = pState->_val;
    _ary.Delete(pState - _ary);
}




//+------------------------------------------------------------------------
//
//  Member:     CVersions::Evaluate
//
//  Synopsis:   The recursive-descent parser to evaluate version
//              expressions.
//
//              The grammar is implemented as follows
//
//              Expression -> OR
//
//              OR   -> AND | AND '|' OR
//                      returns TRUE/FALSE
//              AND  -> NOT | NOT '&' AND
//                      returns TRUE/FALSE
//              NOT  -> TERM | '!' TERM
//                      returns TRUE/FALSE
//              TERM -> CMP | '(' OR ')'
//                      returns TRUE/FALSE
//              CMP  -> 'lt' word version |
//                      'gt' word version |
//                      'lte' word version |
//                      'gte' word version |
//                      word version |
//                      word |
//                      'true' |
//                      'false'
//                      returns TRUE/FALSE
//
//-------------------------------------------------------------------------

HRESULT
CVersions::Evaluate(LONG *pRetval, const TCHAR *pch, ULONG cch)
{
    HRESULT hr;
    CVerTok tok;
    CVerStack s;

    tok.Init(this, pch, cch);
    tok.Advance();

    hr = THR(s.Call(VSTATE_OR, VSTATE_NULL));
    if (hr)
        goto Cleanup;
    
    for (;;)
    {
        if (s.Depth() >= MAX_VER_STACK_DEPTH)
            goto Syntax;
            
        switch (s._cs)
        {
        case VSTATE_NULL:
            goto Done;

        // OR   -> AND | AND '|' OR
        //          returns TRUE/FALSE
        
        case VSTATE_OR:
            hr = THR(s.Call(VSTATE_AND, 1, VSTATE_ORLOOP));
            if (hr)
                goto Cleanup;
            break;

        case VSTATE_ORLOOP:
            s._val |= s._valRet;
                
            if (tok.IsOr())
            {
                tok.Advance();
                s.Go(VSTATE_OR);
                break;
            }

            s.Ret(s._val);
            break;

        // AND  -> NOT | NOT '&' AND
        //          returns TRUE/FALSE
        
        case VSTATE_AND:
            hr = THR(s.Call(VSTATE_NOT, VSTATE_ANDLOOP));
            if (hr)
                goto Cleanup;
            break;

        case VSTATE_ANDLOOP:
            s._val &= s._valRet;
                
            if (tok.IsAnd())
            {
                tok.Advance();
                s.Go(VSTATE_AND);
                break;
            }

            s.Ret(s._val);
            break;

        // NOT  -> TERM | '!' NOT
        //          returns TRUE/FALSE

        case VSTATE_NOT:
            if (!tok.IsNot())
            {
                s.Go(VSTATE_TERM);
                break;
            }

            tok.Advance();

            // rather than calling, loop so as not to stack with (!!!!!foo)
            
            if (!tok.IsNot())
            {
                hr = THR(s.Call(VSTATE_TERM, VSTATE_NOTNOT));
                if (hr)
                    goto Cleanup;
                break;
            }

            tok.Advance();
            break;

        case VSTATE_NOTNOT:
        
            s.Ret(!s._valRet);
            break;

        // TERM -> CMP | '(' OR ')'
        //          returns TRUE/FALSE

        case VSTATE_TERM:
        
            if (tok.IsLParen())
            {
                tok.Advance();
                hr = THR(s.Call(VSTATE_OR, 0, VSTATE_TERMPAREN));
                if (hr)
                    goto Cleanup;
                break;
            }

            s.Go(VSTATE_CMP);
            break;
            
        case VSTATE_TERMPAREN:

            if (!tok.IsRParen())
                goto Syntax;
                
            tok.Advance();
            s.Ret(s._valRet);
            break;

        // CMP  -> word |
        //         word version |
        //         'lt' word version |
        //         'gt' word version |
        //         'lte' word version |
        //         'gte' word version |
        //         'true' |
        //         'false'
        //          returns TRUE/FALSE
        
        case VSTATE_CMP:

            {
                enum { LT, LTE, X, GTE, GT, T, F } comp;
                LONG lRes;
                const TCHAR *pchActVer;
                ULONG cchActVer;
                const TCHAR *pchReqVer;
                ULONG cchReqVer;
                
                if (tok.IsTrue())
                    comp = T;
                else if (tok.IsFalse())
                    comp = F;
                else if (tok.IsLt())
                    comp = LT;
                else if (tok.IsLte())
                    comp = LTE;
                else if (tok.IsGt())
                    comp = GT;
                else if (tok.IsGte())
                    comp = GTE;
                else
                    comp = X;

                if (comp != X)
                    tok.Advance();

                if (comp == T || comp == F)
                {
                    s.Ret(comp == T);
                    break;
                }
                
                if (!tok.IsWord())
                    goto Syntax;

                tok.ActVer(&pchActVer, &cchActVer);
                
                tok.Advance();

                if (!tok.IsVersion())
                {
                    if (comp != X)
                        goto Syntax;

                    s.Ret(!!pchActVer);
                    break;
                }

                if (!pchActVer)
                {
                    tok.Advance();
                    s.Ret(!!(comp == LT || comp == LTE));
                    break;
                }

                tok.ReqVer(&pchReqVer, &cchReqVer);

                Assert(cchReqVer && cchActVer);

                lRes = ContainVersion(pchActVer, cchActVer, pchReqVer, cchReqVer);

                tok.Advance();
                
                if (lRes < 0 && (comp == LT || comp == LTE) ||
                    !lRes && (comp == LTE || comp == X || comp == GTE) ||
                    lRes > 0 && (comp == GT || comp == GTE))
                    s.Ret(1);
                else
                    s.Ret(0);

                break;
            }
        }
    }

Syntax:

    // syntax error
    *pRetval = -1;
    RRETURN(hr);
    
Done:

    Assert( s._valRet == 1 ||
            s._valRet == 0);
    Assert(!hr);
            
    if (!tok.IsNull())
        *pRetval = -1;
    else
        *pRetval = (s._valRet == 0 ? 0 : 1);
    
    return(S_OK);

Cleanup:

    goto Syntax;
}

//+------------------------------------------------------------------------
//
//  Member:     CVersions::EvaluateConditional
//
//  Synopsis:   Parses "if (Expression)" or "endif" and returns
//
//              COND_NULL       - syntax error
//              COND_IF_TRUE    - if TRUE
//              COND_IF_FALSE   - if FALSE
//              COND_ENDIF      - endif
//
//-------------------------------------------------------------------------

HRESULT
CVersions::EvaluateConditional(CONDVAL *pRetval, const TCHAR *pch, ULONG cch)
{
    HRESULT hr = S_OK;
    CVerTok tok;
    LONG lBool;
    CONDVAL retval = COND_NULL;

    tok.Init(this, pch, cch);
    tok.Advance();

    if (tok.IsIf())
    {
        hr = THR(Evaluate(&lBool, tok.Pch(), tok.CchRemaining()));
        if (hr)
            goto Cleanup;

#if DBG == 1
        {
            TCHAR ach[64];
            ULONG cch = min(tok.CchRemaining(), (ULONG)63);
            memcpy(ach, tok.Pch(), cch * sizeof(TCHAR));
            ach[cch] = 0;
            
            TraceTag((tagHtmVer, "\"%ls\" evaluated to %d", ach, lBool));
        }
#endif

        retval = (lBool < 0 ? COND_SYNTAX : lBool ? COND_IF_TRUE : COND_IF_FALSE);
        goto Cleanup;
    }
    
    if (tok.IsEndif())
    {
        // BUGBUG: is it a syntax error to have tokens after "endif"?
        retval = COND_ENDIF;
        goto Cleanup;
    }

#ifdef CLIENT_SIDE_INCLUDE_FEATURE
    if (tok.IsInclude())
    {
        retval = COND_INCLUDE;
        goto Cleanup;
    }
#endif

    retval = COND_NULL;

Cleanup:
    *pRetval = retval;
    
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Global:     g_pVersions
//
//  Synposis:   Process cache for versions cache
//
//-------------------------------------------------------------------------
CVersions * g_pVersions = NULL;

CVersions *
GetGlobalVersions()
{   
    // Take it local to avoid a race
    CVersions *pVersions = g_pVersions;
    
    if (pVersions)
        pVersions->AddRef();
        
    return pVersions;
}

BOOL
SuggestGlobalVersions(CVersions *pVersions)
{
    LOCK_GLOBALS;
    Assert(pVersions);
    if (!g_pVersions)
    {
        // Force CVersions object to become read-only before dropping it in global
        pVersions->Commit();
        pVersions->AddRef();
        g_pVersions = pVersions;
        return TRUE;
    }

    return FALSE;
}

void
DeinitGlobalVersions()
{
    if (g_pVersions)
    {
        g_pVersions->Release();
        g_pVersions = NULL;
    }
}
