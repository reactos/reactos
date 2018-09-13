//+---------------------------------------------------------------------
//
//  File:       stgutils.hxx
//
//  Contents:   IStorage and IStream Helper functions
//
//
//	History:	
//				5-22-95		kfl		converted WCHAR to TCHAR
//----------------------------------------------------------------------

#include "headers.hxx"


//+---------------------------------------------------------------
//
//  Function:   GetMonikerDisplayName
//
//  Synopsis:   Retrieves the display name of a moniker
//
//  Arguments:  [pmk] -- the moniker for which the display name is requested
//              [ppstr] -- the place where the display name is returned
//
//  Returns:    Success iff the display name could be retrieved
//
//  Notes:      The display name string is allocated using the task allocator
//              and should be freed by the same.
//
//----------------------------------------------------------------

HRESULT
GetMonikerDisplayName(LPMONIKER pmk, LPTSTR FAR* ppstr)
{
    HRESULT hr;
    LPBC    pbc;

    hr = THR(CreateBindCtx(0, &pbc));
    if (!hr)
    {
#ifndef _MACUNICODE
        hr = THR(pmk->GetDisplayName(pbc, NULL, ppstr));
#else
        LPOLESTR    szName;
        CStr        str;
        hr = THR(pmk->GetDisplayName(pbc, NULL, &szName));
        str.Set(szName);
        *ppstr = (LPTSTR)CoTaskMemAlloc (str.Length()*sizeof(TCHAR) + sizeof(TCHAR));
        if (*ppstr == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
            _tcscpy( *ppstr, str);
        CoTaskMemFree(szName);
#endif
        pbc->Release();
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Function:   CreateStorageOnHGlobal
//
//  Synopsis:   Creates an IStorage on a global memory handle
//
//  Arguments:  [hgbl] -- memory handle to create storage on
//              [ppStg] -- where the storage is returned
//
//  Returns:    Success iff the storage could be successfully created
//              on the memory handle.
//
//  Notes:      This helper function combines CreateILockBytesOnHGlobal
//              and StgCreateDocfileOnILockBytes.  hgbl may be NULL in
//              which case a global memory handle will be automatically
//              allocated.
//
//----------------------------------------------------------------

HRESULT
CreateStorageOnHGlobal(HGLOBAL hgbl, LPSTORAGE FAR* ppStg)
{
    HRESULT     hr;
    LPLOCKBYTES pLockBytes;

#ifndef _MAC
    hr = THR(CreateILockBytesOnHGlobal(hgbl, TRUE, &pLockBytes));
#else
    {
        Handle hv = NULL;

        if(!hgbl || GetWrapperHandle (hgbl,&hv))
        {

            hr = THR(CreateILockBytesOnHGlobal(hv, TRUE, &pLockBytes));
        }
        else
        {
            Assert ( 0&& "Failed to GetWrapperHandle");
            hr = E_INVALIDARG;
        }
    }
#endif

    if (!hr)
    {
#ifndef WIN16
        //REVIEW: should be use STGM_DELETEONRELEASE when hgbl == NULL?
        hr = THR(StgCreateDocfileOnILockBytes(
                pLockBytes,
                STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                ppStg));
#endif // ndef WIN16
        pLockBytes->Release();
    }

    RRETURN(hr);
}

