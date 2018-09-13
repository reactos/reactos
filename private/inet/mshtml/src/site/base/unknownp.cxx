//+---------------------------------------------------------------------
//
//   File:      unknownp.cxx
//
//  Contents:   Unknown Pair class
//
//  Classes:    CUnknownPairList
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_UNKNOWNP_HXX_
#define X_UNKNOWNP_HXX_
#include "unknownp.hxx"
#endif

CUnknownPair* 
CUnknownPairList::Get(size_t i)
{
    Assert(i>=0&&i<Length());
    return ((CUnknownPair *)*_paryUnknown)+i;
}

size_t
CUnknownPairList::Length() const
{
    return _paryUnknown?_paryUnknown->Size():0;
}

HRESULT 
CUnknownPairList::Duplicate(CUnknownPairList &upl) const
{
    HRESULT hr = S_OK;
    CUnknownPair *pupThis, *pupThat;
    int cUPairs;

    CDataAry<CUnknownPair> *paryOtherUnknown;

    // Both attr bags must have unknowns, or neither
    if (!_paryUnknown)
        goto Cleanup;

    cUPairs = _paryUnknown->Size();

    upl._paryUnknown = new CDataAry<CUnknownPair>;
    if (!upl._paryUnknown)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    paryOtherUnknown = upl._paryUnknown;
    hr = paryOtherUnknown->EnsureSize(cUPairs);
    if (hr)
        goto Cleanup;

    paryOtherUnknown->SetSize(cUPairs);

    pupThis = (CUnknownPair *)*_paryUnknown;
    pupThat = (CUnknownPair *)*paryOtherUnknown;
    for ( ; --cUPairs >= 0; pupThis++, pupThat++)
    {
        UINT cchA = pupThis->_cstrA.Length();
        UINT cchB = pupThis->_cstrB.Length();

        new (&pupThat->_cstrA) CStr();
        new (&pupThat->_cstrB) CStr();

        hr = pupThat->_cstrA.Set((LPTSTR)pupThis->_cstrA, cchA);
        if (hr)
            goto Cleanup;

        if (cchB)
        {
            hr = pupThat->_cstrB.Set((LPTSTR)pupThis->_cstrB, cchB);
            if (hr)
                goto Cleanup;
        }
    }

// BUGBUG: Should we free a partially copied array???
Cleanup:
    RRETURN(hr);
}

BOOL 
CUnknownPairList::Compare(const CUnknownPairList *pup) const
{
    CUnknownPair *pupThis, *pupThat;
    int cUPairs;
    CDataAry<CUnknownPair> *paryOtherUnknown = pup->_paryUnknown;

    // Both attr bags must have unknowns, or neither
    if (!_paryUnknown || !paryOtherUnknown)
    {
        return (!_paryUnknown && !paryOtherUnknown);
    }

    cUPairs = _paryUnknown->Size();
    if (cUPairs != paryOtherUnknown->Size())
        return FALSE;

    pupThis = (CUnknownPair *)*_paryUnknown;
    pupThat = (CUnknownPair *)*paryOtherUnknown;
    for ( ; --cUPairs >= 0; pupThis++, pupThat++)
    {
        UINT cchName = pupThis->_cstrA.Length();
        UINT cchValue = pupThis->_cstrB.Length();

        // Check lengths first
        if (   cchName != pupThat->_cstrA.Length()
            || cchValue != pupThat->_cstrB.Length())
            return FALSE;

        if (   _tcsicmp((LPTSTR)pupThis->_cstrA, (LPTSTR)pupThat->_cstrA)
            || (cchValue && StrCmpC((LPTSTR)pupThis->_cstrB, (LPTSTR)pupThat->_cstrB)))
            return FALSE;
    }
    return TRUE;
}

WORD 
CUnknownPairList::ComputeCrc() const
{
    WORD wHash = 0;

    if (_paryUnknown) {
        CUnknownPair *pUPair;
        int cUPairs;

        for (pUPair = (CUnknownPair *)*_paryUnknown, cUPairs = _paryUnknown->Size();
             cUPairs; cUPairs--, pUPair++)
        {
            wHash ^= pUPair->_cstrA.ComputeCrc();
            wHash ^= pUPair->_cstrB.ComputeCrc();
        }
    }
    return wHash;
}

HRESULT
CUnknownPairList::AddUnknownPair(const TCHAR *pchA, const size_t cchA,
                            const TCHAR *pchB, const size_t cchB)
{
    HRESULT hr = S_OK;
    CUnknownPair *pUPair;
    CUnknownPair upDummy;
    int iCount, iIndex, iDiff;


    Assert(pchA);

    if (!_paryUnknown)
    {
        _paryUnknown = new CDataAry<CUnknownPair>;
    }

    if (!_paryUnknown)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // BUGBUG: should bsearch

    iCount = _paryUnknown->Size();

    for (iIndex = 0; iIndex < iCount; iIndex++)
    {
        pUPair = Get(iIndex);
        Assert(pUPair);

        // Compare names (case-insensitive)
        iDiff = _tcsnicmp(pchA, cchA, (LPTSTR)pUPair->_cstrA, -1);
        if (iDiff < 0)
            break;
        else if (!iDiff)
        {
            // Compare values
            if (!cchB || !pchB)      // If there's no value, insert here
                break;
            else if (!(LPTSTR)pUPair->_cstrB)   // If this slot has no value, keep looking
                continue;
            else
            {
                // Compare value strings (case-sensitive)
                iDiff = _tcsncmp(pchB, cchB, (LPTSTR)pUPair->_cstrB, -1);
                if (iDiff <= 0)
                    break;
            }
        }
    }

    // BUGBUG: This is egregious. Should upDummy be initialized first?????
    hr = _paryUnknown->InsertIndirect(iIndex, &upDummy);
    if (hr)
        goto Cleanup;
    pUPair = Get(iIndex);
    Assert(pUPair);

    new (&pUPair->_cstrA) CStr();
    new (&pUPair->_cstrB) CStr();

    hr = pUPair->_cstrA.Set(pchA, cchA);
    if (hr)
        goto Cleanup;

    if (pchB && cchB)
    {
        hr = pUPair->_cstrB.Set(pchB, cchB);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);

}

void
CUnknownPairList::Free()
{
    CUnknownPair *pUPair;

    if (_paryUnknown)
    {
        int i = _paryUnknown->Size();
        for (pUPair = (CUnknownPair *)(*_paryUnknown); i; i--, pUPair++)
        {
            pUPair->_cstrA.Free();
            pUPair->_cstrB.Free();
        }

        delete _paryUnknown;
        _paryUnknown = NULL;
    }
}

