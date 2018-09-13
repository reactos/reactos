//+----------------------------------------------------------------------------
//
//  File:       peer.cxx
//
//  Contents:   peer holder
//
//  Classes:    CPeerHolder
//
//-----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_PEERFACT_HXX_
#define X_PEERFACT_HXX_
#include "peerfact.hxx"
#endif

#ifndef X_PEERURLMAP_HXX_
#define X_PEERURLMAP_HXX_
#include "peerurlmap.hxx"
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Misc
//
///////////////////////////////////////////////////////////////////////////////

MtDefine(CPeerFactoryUrlMap,                CDoc,               "CPeerFactoryUrlMap")
MtDefine(CPeerFactoryUrlMap_aryFactories,   CPeerFactoryUrlMap, "CDoc::_aryFactories")


///////////////////////////////////////////////////////////////////////////////
//
//  Class:      CPeerFactoryUrlMap
//
///////////////////////////////////////////////////////////////////////////////

//+----------------------------------------------------------------------------
//
//  Member:       CPeerFactoryUrlMap constructor
//
//-----------------------------------------------------------------------------

CPeerFactoryUrlMap::CPeerFactoryUrlMap(CDoc * pDoc) :
    _UrlMap(CStringTable::CASEINSENSITIVE)
{
    _pDoc = pDoc;
}

//+----------------------------------------------------------------------------
//
//  Member:       CPeerFactoryUrlMap destructor
//
//-----------------------------------------------------------------------------

CPeerFactoryUrlMap::~CPeerFactoryUrlMap()
{
    int                 c;
    CPeerFactoryUrl **  ppFactory;

    for (c = _aryFactories.Size(), ppFactory = _aryFactories;
         c;
         c--, ppFactory++)
    {
        (*ppFactory)->Release();
    }

    _aryFactories.DeleteAll();
}

//+----------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrlMap::StopDownloads
//
//-----------------------------------------------------------------------------

HRESULT
CPeerFactoryUrlMap::StopDownloads()
{
    HRESULT             hr = S_OK;
    int                 c;
    CPeerFactoryUrl **  ppFactory;

    for (c = _aryFactories.Size(), ppFactory = _aryFactories;
         c;
         c--, ppFactory++)
    {
        (*ppFactory)->StopBinding();
    }

    RRETURN (hr);
}

/*
//+----------------------------------------------------------------------------
//
//  Member:       CPeerFactoryUrlMap::EnsurePeerFactoryUrl
//
//-----------------------------------------------------------------------------

HRESULT
CPeerFactoryUrlMap::EnsurePeerFactoryUrl(LPTSTR pchUrl, CMarkup * pMarkup, CPeerFactoryUrl ** ppFactory)
{
    HRESULT             hr = S_OK;
    int                 idx, cnt;
    BOOL                fClone;
    LPTSTR              pchUrlSecondPound;
    int                 nUrlSecondPound;
    CPeerFactoryUrl *   pFactory;

    Assert (ppFactory);
    *ppFactory = NULL;

    pchUrlSecondPound = (_T('#') == pchUrl[0]) ? StrChr(pchUrl + 1, _T('#')) : NULL;
    nUrlSecondPound = pchUrlSecondPound ? pchUrlSecondPound - pchUrl : 0;

    fClone = FALSE;

    for (idx = 0, cnt = _aryFactories.Size(); idx < cnt; idx++)
    {
        pFactory = _aryFactories[idx];

        if (0 == StrCmpIC(pchUrl, pFactory->_cstrUrl))
        {
            break;
        }

        if (nUrlSecondPound &&
            0 == StrCmpNIC(pchUrl, pFactory->_cstrUrl, nUrlSecondPound) &&
            0 == pFactory->_cstrUrl[nUrlSecondPound])
        {
            // found the peer factory "#foo", but need to clone it into a more specific "#foo#cat"
            fClone = TRUE;
            break;
        }
    }

    //
    // create new CPeerFactoryUrl if necessary and/or clone existing if necessary
    //

    if (idx == cnt)
    {
        CStringNullTerminator     nullTerminator(pchUrlSecondPound); // convert url "#foo#bar" to "#foo" temporary

        hr = THR(CPeerFactoryUrl::Create(pchUrl, _pDoc, pMarkup, &pFactory));
        if (hr)
            goto Cleanup;

        hr = THR(_aryFactories.Append(pFactory));
        if (hr)
            goto Cleanup;

        idx = _aryFactories.Size() - 1;  // (the idx is also used in cloning codepath)
        fClone = (NULL != pchUrlSecondPound);   // request cloning only if necessary

    }

    if (fClone)
    {
        // clone existing peer factory into more specific factory and insert it *before* less specific

        hr = THR(pFactory->Clone(pchUrl, &pFactory));
        if (hr)
            goto Cleanup;

        hr = THR(_aryFactories.Insert(idx, pFactory));
        if (hr)
            goto Cleanup;
    }

    *ppFactory = pFactory;

Cleanup:
    RRETURN (hr);
}
*/

//+----------------------------------------------------------------------------
//
//  Member:       CPeerFactoryUrlMap::EnsurePeerFactoryUrl
//
// (for more details see also comments in the beginning of peerfact.cxx file)
// this function modifies _aryFactories in the following way:
//  - if there already is a factory for the url,
//          then no change happens;
//  - if the url is in form "http://...foo.bla" or "#foo" (but not "#foo#bla"),
//          then it creates a new factory for the url
//  - if the url is in form "#foo#bla",
//          then it creates 2 new factories: one for "#foo" and one for "#foo#bla"
//
//-----------------------------------------------------------------------------

HRESULT
CPeerFactoryUrlMap::EnsurePeerFactoryUrl(LPTSTR pchUrl, CMarkup * pMarkup, CPeerFactoryUrl ** ppFactory)
{
    HRESULT             hr = S_OK;
    CPeerFactoryUrl *   pFactory;
    LPTSTR              pchUrlSecondPound;

    Assert (ppFactory);

    pFactory = (CPeerFactoryUrl*) _UrlMap.Find(pchUrl);
    if (pFactory)
        goto Cleanup; // done

    // factory for the url not found

    pchUrlSecondPound = (_T('#') == pchUrl[0]) ? StrChr(pchUrl + 1, _T('#')) : NULL;

    if (!pchUrlSecondPound)
    {
        // url in form "http://...foo.bla"  or "#foo", but not in form "#foo#bar"

        hr = THR(CPeerFactoryUrl::Create(pchUrl, _pDoc, pMarkup, &pFactory));
        if (hr)
            goto Cleanup;
    }
    else
    {
        // url in form "#foo#bar"

        // ensure factory for "#foo"
        {
            CStringNullTerminator nullTerminator(pchUrlSecondPound); // convert url "#foo#bar" to "#foo" temporary

            hr = THR(EnsurePeerFactoryUrl(pchUrl, pMarkup, &pFactory));
            if (hr)
                goto Cleanup;
        }

        // clone factory for "#foo#bar" from factory for "#foo"

        hr = THR(pFactory->Clone(pchUrl, &pFactory));
        if (hr)
            goto Cleanup;
    }

    hr = THR(_aryFactories.Append(pFactory));
    if (hr)
        goto Cleanup;

    hr = THR(_UrlMap.Add(pFactory, pchUrl));

Cleanup:

    if (S_OK == hr)
        *ppFactory = pFactory;
    else
        *ppFactory = NULL;

    RRETURN (hr);
}
