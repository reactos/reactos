// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      Graphics Control dll exports.
//
//-----------------------------------------------------------------------------

#include "precomp.hpp"

HRESULT WINAPI MediaControl_CanAttach(
    __in PCWSTR lpName,
    __out_ecount(1) BOOL* pCanAttach)
{
    HRESULT hr = S_OK;
    
    if (lpName == NULL || pCanAttach == NULL)
    {
        IFC(E_INVALIDARG);
    }

    *pCanAttach = CMediaControl::CanAttach(lpName);

Cleanup:

    return hr;
}

HRESULT WINAPI MediaControl_Attach(
    __in PCWSTR lpName, 
    __deref_out void** ppMediaControl)
{
    HRESULT hr = S_OK;
    CMediaControl* pMediaControl = NULL;

    if (ppMediaControl == NULL)
    {
        IFC(E_INVALIDARG);
    }
    
    IFC(CMediaControl::Attach(lpName, &pMediaControl));

    *ppMediaControl = pMediaControl;

Cleanup:
    return hr;
}

void WINAPI MediaControl_Release(void* pMediaControl)
{
    delete ((CMediaControl*)pMediaControl);
}

HRESULT WINAPI MediaControl_GetDataPtr(
    __in void* pMediaControl,
    __deref_out void** pFile)
{
    HRESULT hr = S_OK;
    if ((pMediaControl == NULL) || 
        (pFile  == NULL))
    {
        IFC(E_INVALIDARG);
    }

    (*pFile) = ((CMediaControl*)pMediaControl)->GetDataPtr();

Cleanup:
    return hr;
}


