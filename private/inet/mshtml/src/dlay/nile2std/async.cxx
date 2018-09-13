//--------------------------------------------------------------------
// Microsoft Nile to STD Mapping Layer
// (c) 1995 Microsoft Corporation.  All Rights Reserved.
//
//  File:       stdevent.cxx
//  Author:     Charles Frankston (cfranks)
//
//  Contents:   IRowsetAsynch methods
//

//
//
#include <dlaypch.hxx>

#ifndef X_ROWSET_HXX_
#define X_ROWSET_HXX_
#include "rowset.hxx"
#endif

//+-----------------------------------------------------------------------
//
//  Member:    GetStatus (IDBAsynchStatus public member)
//
//  Synopsis:  Returns the fraction of an async rowset currently populated.
//             Since we don't know what portion of an async rowset remains
//             to be populated, we use a gross approximation:
//                - if the rowset is still being populated, return 1/2
//                - if the rowset is done, return count/count, or 1
//
//  Returns:   S_OK             all OK.
//

STDMETHODIMP
CImpIRowset::GetStatus(HCHAPTER hChapter, DBASYNCHOP ulOperation,
                       DBCOUNTITEM *pulProgress, DBCOUNTITEM *pulProgressMax,
                       DBASYNCHPHASE *pulStatusCode, LPOLESTR *ppwszStatusText)
{
    HRESULT hr = S_OK;
    DBCOUNTITEM lProgressDummy, lProgressMaxDummy;
    DBASYNCHPHASE lStatusCodeDummy;
    COSPData *pOSPData = GetpOSPData(hChapter);

    if (pOSPData == NULL)
    {
        hr = DB_E_BADCHAPTER;
        goto Cleanup;
    }
    
    if (ppwszStatusText)
        *ppwszStatusText = NULL;            // We don't support status text

    if (!pulProgress) pulProgress = &lProgressDummy;
    if (!pulProgressMax) pulProgressMax = &lProgressMaxDummy;
    if (!pulStatusCode) pulStatusCode = &lStatusCodeDummy;

    if (!pOSPData->_fPopulationComplete)
    {
        *pulStatusCode = DBASYNCHPHASE_POPULATION;
        *pulProgress = pOSPData->_cSTDRows;
        hr = GetpOSP(hChapter)->getEstimatedRows((DBROWCOUNT *)pulProgressMax);
        if (FAILED(hr) || *pulProgressMax==0)
            *pulProgressMax = 2 * pOSPData->_cSTDRows;    // Arbitrary
    }
    else
    {
        *pulProgress = pOSPData->_cSTDRows;
        // pulProgressMax might be used as the denominator of a ratio.
        // Best it never be zero..
        *pulProgressMax = *pulProgress ? *pulProgress : 1;
        *pulStatusCode = DBASYNCHPHASE_COMPLETE;
    }

Cleanup:
    return hr;
}
        
//+-----------------------------------------------------------------------
//
//  Member:    Abort (IDBAsynchStatus public member)
//
//  Synopsis:  Stops any asynch data transfer that may be in progress
//
//  Returns:   S_OK if the STD returned success
//             E_FAIL if not.
//
STDMETHODIMP
CImpIRowset::Abort(HCHAPTER	hChapter,
                   ULONG     ulOperation)
{
    HRESULT hr;

    if (GetpOSPData(hChapter) == NULL)
        hr = DB_E_BADCHAPTER;
    else if (SUCCEEDED(GetpOSP(hChapter)->stopTransfer()))
        hr = S_OK;
    else
        hr = E_FAIL;

    return hr;
}
