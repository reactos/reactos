//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       specpage.cxx
//
//  Contents:   AddPages()
//
//  Classes:    None.
//
//  Functions:  AddPages -- adds property pages to those provided by an
//                  implementation of ISpecifyPropertyPages::GetPages.
//
//  History:    5-02-94   adams   Created
//              27-Jul-94 doncl   MemAlloc->CoTaskMemAlloc
//
//----------------------------------------------------------------------------

#include <headers.hxx>


//+---------------------------------------------------------------------------
//
//  Function:   AddPages
//
//  Synopsis:   Adds property pages to those provided by an implementation of
//              ISpecifyPropertyPages::GetPages.
//
//  Arguments:  [pUnk]     -- Implementation of GetPages.  May be NULL.
//              [apUUID]   -- Array of pages to add.
//              [pCAUUID]  -- Result.
//
//  Returns:    HRESULT.
//
//  Modifies:   [pcauuid].
//
//----------------------------------------------------------------------------

STDAPI
AddPages(
        IUnknown *      pUnk,
        const UUID *    apuuid[],
        CAUUID *        pcauuid)
{
    HRESULT                 hr      = S_OK;
    ISpecifyPropertyPages * pSPP;           // spp iface of agg
    CAUUID                  cauuid;         // pSPP's property pages
    ULONG                   ul;             // counter
    UUID *                  puuidDest;      // copy destination
    const UUID **           ppuuidSrc;      // copy source
    UUID *                  puuid;          // search
    ULONG                   c;
    ULONG                   ulcPages;

    if (!apuuid)
    {
        ulcPages = 0;
    }
    else
    {
        for (ulcPages = 0; apuuid[ulcPages]; ulcPages++)
            ;
    }

    //  NOTE that this routine accepts requests to add zero
    //    pages, in which case ulcPages == 0 and apuuid == NULL

    pcauuid->cElems = 0;
    pcauuid->pElems = NULL;

    // Get source propery pages.
    cauuid.cElems = 0;
    cauuid.pElems = NULL;
    if (pUnk)
    {
        if (!THR(pUnk->QueryInterface(
                IID_ISpecifyPropertyPages,
                (void **) &pSPP)))
        {
            hr = THR(pSPP->GetPages(&cauuid));
            pSPP->Release();

            if (hr)
                goto Cleanup;
        }
    }

    // Alloc space for combined array of pages.
    pcauuid->pElems =
        (UUID *) CoTaskMemAlloc((cauuid.cElems + ulcPages) * sizeof(UUID));
    if (!pcauuid->pElems)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Copy pages to new array.
    memcpy(
            pcauuid->pElems,
            cauuid.pElems,
            (UINT) (cauuid.cElems * sizeof(UUID)));

    ppuuidSrc = apuuid;
    puuidDest = pcauuid->pElems + cauuid.cElems;
    for (ul = 0; ul < ulcPages; ul++)
    {
        //  Make sure we don't add any duplicate pages
        for (c = cauuid.cElems, puuid = cauuid.pElems;
             c > 0;
             c--, puuid++)
        {
            if (**ppuuidSrc == *puuid)
                goto NextUUID;
        }

        *puuidDest++ = **ppuuidSrc;

NextUUID:
        ppuuidSrc++;
    }

    pcauuid->cElems = puuidDest - pcauuid->pElems;

Cleanup:
    if (cauuid.pElems)
        CoTaskMemFree(cauuid.pElems);

    RRETURN(hr);
}
