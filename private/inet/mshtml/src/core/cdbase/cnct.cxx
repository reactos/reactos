//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       cnct.cxx
//
//  Contents:   Connections interfaces and implementations (part 1 of 2).
//
//  History:    12-22-93   adams   Created
//              5-22-95    kfl     converted WCHAR to TCHAR
//              2-21-96    LyleC   Cleaned out unused stuff
//
//----------------------------------------------------------------------------

#include "headers.hxx"

MtDefine(CEnumConnections, ObjectModel, "CEnumConnections")
MtDefine(CEnumConnectionsCreate_pary, Locals, "CEnumConnections::Create pary")
MtDefine(CEnumConnectionsCreate_pary_pv, CEnumConnectionsCreate_pary, "CEnumConnections::Create pary->_pv")

//+------------------------------------------------------------------------
//
//  CEnumConnections Implementation
//
//-------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Member:     CEnumConnections::Create
//
//  Synopsis:   Creates a new CEnumGeneric object.
//
//  Arguments:  [pary]    -- Array to enumerate.
//              [ppenum]  -- Resulting CEnumGeneric object.
//
//  Returns:    HRESULT.
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

HRESULT
CEnumConnections::Create(
        int                 ccd,
        CONNECTDATA *       pcd,
        CEnumConnections ** ppenum)
{
    HRESULT                 hr      = S_OK;
    CEnumConnections *      penum   = NULL;
    CDataAry<CONNECTDATA> * pary = NULL;
    CONNECTDATA *           pcdT;
    int                     c;
    int                     cNonNull;

    Assert(ppenum);
    *ppenum = NULL;

    penum = new CEnumConnections;
    if (!penum)
        goto MemError;

    pary = new(Mt(CEnumConnectionsCreate_pary)) CDataAry<CONNECTDATA>(Mt(CEnumConnectionsCreate_pary_pv));
    if (!pary)
        goto MemError;

    Assert(!penum->_pary);

    //  Copy the AddRef'd array of pointers into a CFormsAry
    hr = THR(pary->EnsureSize(ccd));
    if (hr)
        goto Error;

    cNonNull = 0;
    for (c = ccd, pcdT = pcd; c > 0; c--, pcdT++)
    {
        if (pcdT->pUnk)
        {
            (*pary)[cNonNull] = *pcdT;
            pcdT->pUnk->AddRef();
            cNonNull++;
        }
    }

    pary->SetSize(cNonNull);
    penum->_pary = pary;

    *ppenum = penum;

Cleanup:
    RRETURN(hr);

MemError:
    hr = E_OUTOFMEMORY;
    // fall through

Error:
    ReleaseInterface(penum);
    delete pary;

    goto Cleanup;
}



//+---------------------------------------------------------------------------
//
//  Member:     CEnumConnections::CEnumConnections
//
//  Synopsis:   ctor.
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

CEnumConnections::CEnumConnections(void) :
        CBaseEnum(sizeof(IUnknown *), IID_IEnumConnections, TRUE, TRUE)
{
}



//+---------------------------------------------------------------------------
//
//  Member:     CEnumConnections::CEnumConnections
//
//  Synopsis:   ctor.
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

CEnumConnections::CEnumConnections(const CEnumConnections & enumc)
    : CBaseEnum(enumc)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CEnumConnections::~CEnumConnections
//
//  Synopsis:   dtor.
//
//  History:    10-12-99   alanau   Created
//
//----------------------------------------------------------------------------

CEnumConnections::~CEnumConnections()
{
    if (_pary && _fDelete)
    {
        if ( _fAddRef )
        {
            CONNECTDATA * pcdT;
            int         i;
            int         cSize = _pary->Size();
            
            for (i=0, pcdT = (CONNECTDATA *) Deref(0);
                 i < cSize;
                 i++, pcdT++)
            {
                pcdT->pUnk->Release();
            }
        }

        delete _pary;
        _pary = NULL;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CEnumConnections::Next
//
//  Synopsis:   Per IEnumConnections::Next.
//
//  History:    12-28-93   adams   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CEnumConnections::Next(
        ULONG           ulConnections,
        void *          reelt,
        ULONG *         lpcFetched)
{
    HRESULT         hr;
    CONNECTDATA *   pCD;               // CONNECTDATA's to enumerate
    CONNECTDATA *   pCDEnd;            // end of CONNECTDATA's
    CONNECTDATA *   pCDSrc;
    int             iActual;           // elems returned

    // Determine number of elements to enumerate.
    if (_i + (int) ulConnections <= _pary->Size())
    {
        hr = S_OK;
        iActual = (int) ulConnections;
    }
    else
    {
        hr = S_FALSE;
        iActual = _pary->Size() - _i;
    }

    if (iActual > 0 && !reelt)
        RRETURN(E_INVALIDARG);

    if (lpcFetched)
    {
        *lpcFetched = (ULONG) iActual;
    }

    // Return elements to enumerate.
    pCDEnd = (CONNECTDATA *) reelt + iActual;
    for (pCD = (CONNECTDATA *) reelt, pCDSrc = (CONNECTDATA *) Deref(_i);
        pCD < pCDEnd;
        pCD++, pCDSrc++)
    {
        *pCD = *pCDSrc;
        pCD->pUnk->AddRef();
    }

    _i += iActual;

    RRETURN1(hr, S_FALSE);
}



//+---------------------------------------------------------------------------
//
//  Member:     CEnumConnections::Clone
//
//  Synopsis:   Per IEnumConnections::Clone.
//
//  History:    12-28-93   adams   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CEnumConnections::Clone(CBaseEnum ** ppenum)
{
    HRESULT hr;

    if (!ppenum)
        RRETURN(E_INVALIDARG);

    *ppenum = NULL;
    hr = THR(Create(
            _pary->Size(),
            (CONNECTDATA *)(LPVOID)*_pary,
            (CEnumConnections **) ppenum));
    if (hr)
        RRETURN(hr);

    (* (CEnumConnections **) ppenum)->_i = _i;
    return S_OK;
}
