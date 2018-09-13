//+---------------------------------------------------------------------
//
//  File:       stgutils.hxx
//
//  Contents:   IStorage and IStream Helper functions
//
//----------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

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
GetMonikerDisplayName(LPMONIKER pmk, LPWSTR FAR* ppstr)
{
    HRESULT r;
    LPBC pbc;
    if (OK(r = CreateBindCtx(0, &pbc)))
    {
        r = pmk->GetDisplayName(pbc, NULL, ppstr);
        pbc->Release();
    }
    return r;
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
    HRESULT r;
    LPLOCKBYTES pLockBytes;
    if (OK(r = CreateILockBytesOnHGlobal(hgbl, TRUE, &pLockBytes)))
    {
        //REVIEW:  should be use STGM_DELETEONRELEASE when hgbl == NULL?
        r = StgCreateDocfileOnILockBytes(pLockBytes,
                STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, 0, ppStg);
        pLockBytes->Release();
    }
    return r;
}

//+---------------------------------------------------------------
//
//  Function:   ConvertToMemoryStream
//
//  Synopsis:   Takes a stream and produces an equivalent stream
//              stored in memory for faster individual reads
//
//  Arguments:  [pStrmFrom] -- the stream to convert
//
//  Returns:    An equivalent stream
//
//  Notes:      Any failure in creating the memory stream will result in
//              the original stream, pStrmFrom, being returned.
//              Hence the caller should assume the stream passed in has
//              been released and should release the stream returned
//              after it has finished using it.
//
//----------------------------------------------------------------

LPSTREAM
ConvertToMemoryStream(LPSTREAM pStrmFrom)
{
    //REVIEW:  perhaps we should use the IStream::CopyTo function
    // instead of doing the read manually!
    HRESULT r;
    LPSTREAM pStrm = NULL;
    STATSTG statstg;

    if (OK(r = pStrmFrom->Stat(&statstg, STATFLAG_NONAME)))
    {
        if (statstg.cbSize.HighPart != 0)
        {
#if DBG
           DOUT(L"o2base/stdils/ConvertToMemoryStream E_FAIL\r\n");
#endif
            r = E_FAIL;
        }
        else
        {
            HGLOBAL hgbl = GlobalAlloc(GMEM_SHARE, statstg.cbSize.LowPart);
            if (hgbl == NULL)
            {
               DOUT(L"o2base/stgutils/ConvertToMemoryStream failed\r\n");
               r = E_OUTOFMEMORY;
            }
            else
            {
                LPVOID pv = GlobalLock(hgbl);
                if (pv == NULL)
                {
#if DBG
           DOUT(L"o2base/stdils/ConvertToMemoryStream E_FAIL(2)\r\n");
#endif
                    r = E_FAIL;
                }
                else
                {
                    r = pStrmFrom->Read(pv, statstg.cbSize.LowPart, NULL);
                    GlobalUnlock(hgbl);
                    if (OK(r))
                    {
                        if (OK(r = CreateStreamOnHGlobal(hgbl, TRUE, &pStrm)))
                        {
                            pStrm->SetSize(statstg.cbSize);
                            pStrmFrom->Release();
                        }
                        else
                        {
#if DBG
           DOUT(L"o2base/stdils/ConvertToMemoryStream CreateStreamOnHGlobal Failed!\r\n");
#endif
                        }
                    }
                }
                if (!OK(r))
                {
                    GlobalFree(hgbl);
                }
            }
        }
    }

    if (!OK(r))
    {
        pStrm = pStrmFrom;
    }

    return pStrm;
}


