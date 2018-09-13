#include "priv.h"
#include "stream.h"

#define _DBLNext(pdbList) ((LPDBLIST)(((LPBYTE)(pdbList)) + (pdbList)->cbSize ))
#define DBSIG_WRAP ((DWORD)-1)

extern "C" {

HRESULT SHWriteDataBlockList(IStream* pstm, LPDBLIST pdbList)
{
    HRESULT hr = S_OK;

    if (pdbList)
    {
        for ( ; pdbList->cbSize; pdbList = _DBLNext(pdbList))
        {
            LPDATABLOCK_HEADER pdb;
            ULONG cbBytes;

            pdb = pdbList;
            if (DBSIG_WRAP == pdb->dwSignature)
                pdb++;

            TraceMsg(TF_DBLIST, "Writing extra data block, size:%x sig:%x", pdb->cbSize, pdb->dwSignature);
    
            if (FAILED(hr = ((CMemStream*)pstm)->Write((LPBYTE)pdb, pdb->cbSize, &cbBytes)))
                break;
    
            if (cbBytes != pdb->cbSize)
            {
                hr = STG_E_MEDIUMFULL;
                break;
            }
        }
    }

    // NULL terminate the list
    if (SUCCEEDED(hr))
    {
        DWORD dwData = 0;
        DWORD cbBytes;
        hr = ((CMemStream*)pstm)->Write(&dwData, SIZEOF(dwData), &cbBytes);
    }

    return(hr);
}

HRESULT SHReadDataBlockList(IStream* pstm, LPDBLIST * ppdbList)
{
    HRESULT hres;
    BYTE buf[200]; // all blocks today fit in this size (tested at 5)
    LPDATABLOCK_HEADER lpBuf = (LPDATABLOCK_HEADER)buf;
    DWORD cbBuf = SIZEOF(buf);
    DWORD dwSizeToRead, cbBytes;

    if (*ppdbList)
    {
        LocalFree((HLOCAL)(*ppdbList));
        *ppdbList = NULL;
    }

    while (TRUE)
    {
        DWORD cbSize;
        dwSizeToRead = SIZEOF(cbSize);
        hres = ((CMemStream*)pstm)->Read(&cbSize, dwSizeToRead, &cbBytes);
        if (SUCCEEDED(hres) && (cbBytes == dwSizeToRead))
        {

            // Windows 95 and NT 4 shipped a CShellLink that did NOT
            // NULL terminate the data it wrote out to the stream.
            // If more data was persisted after the CShellLink then
            // we will read in garbage. No real harm comes of this (*)
            // (because it is unlikely we'll get a dwSignature match)
            // but if the first dword is huge, we'll allocate a ton
            // of memory and page it in. This can take MINUTES on Win95.
            // Assume anything over 64K is from one of these
            // bogus streams.
            //
            // (*) actually, real harm comes because we don't leave the
            // stream in the correct place. Forms^3 put a work-around
            // in for this bug.
            //
            if (cbSize > 0x0000FFFF)
            {
                ULARGE_INTEGER liStart;
                LARGE_INTEGER liMove;

                // We read a DWORD of data that wasn't ours, back up.
                // NOTE: all of our stream implementations assume
                //       HighPart == 0
                //
                liMove.HighPart = liMove.LowPart = 0;
                if (SUCCEEDED(((CMemStream*)pstm)->Seek(liMove, STREAM_SEEK_CUR, &liStart)))
                {
                    ASSERT(liStart.HighPart == 0);
                    ASSERT(liStart.LowPart >= SIZEOF(cbSize));
                    liMove.LowPart = liStart.LowPart - SIZEOF(cbSize);

                    ((CMemStream*)pstm)->Seek(liMove, STREAM_SEEK_SET, NULL);
                }

                TraceMsg(TF_DBLIST, "ASSUMING NO NULL TERMINATION (FOR SIZE 0x%x)", cbSize);
                cbSize = 0;
            }

            // If we hit the 0 terminator, we're done.
            //
            if (cbSize < SIZEOF(DATABLOCK_HEADER))
                break;

            // Make sure we can read this block in.
            //
            if (cbSize > cbBuf)
            {
                HLOCAL pTemp;

                if (lpBuf == (LPDATABLOCK_HEADER)buf)
                    pTemp = LocalAlloc(LPTR, cbSize);
                else
                    pTemp = LocalReAlloc((HLOCAL)lpBuf, cbSize, LMEM_ZEROINIT | LMEM_MOVEABLE);

                if (pTemp)
                {
                    lpBuf = (LPDATABLOCK_HEADER)pTemp;
                    cbBuf = cbSize;
                }
                else
                {
                    hres = E_OUTOFMEMORY;
                    break;
                }
            }

            // Read in data block
            //
            lpBuf->cbSize = cbSize;
            dwSizeToRead = cbSize - SIZEOF(cbSize);
            hres = ((CMemStream*)pstm)->Read((LPBYTE)&(lpBuf->dwSignature), dwSizeToRead, &cbBytes);
            if (SUCCEEDED(hres) && (cbBytes == dwSizeToRead))
            {
                TraceMsg(TF_DBLIST, "Reading extra data block, size:%x sig:%x", lpBuf->cbSize, lpBuf->dwSignature);

                SHAddDataBlock(ppdbList, lpBuf);
            }
            else
                break;
        }
        else
            break;
    }

    // Free any allocated buffer
    //
    if (lpBuf != (LPDATABLOCK_HEADER)buf)
    {
        LocalFree((HLOCAL)lpBuf);
    }

    return(hres);
}

void SHFreeDataBlockList(LPDBLIST pdbList)
{
    if (pdbList)
    {
        LocalFree((HLOCAL)pdbList);
    }
}

BOOL SHAddDataBlock(LPDBLIST * ppdbList, LPDATABLOCK_HEADER pdb)
{
    LPDBLIST pdbCopyTo = NULL;
    DWORD dwSize;

    // Don't let anyone use our special signature
    //
    if (DBSIG_WRAP == pdb->dwSignature ||
        pdb->cbSize < SIZEOF(*pdb))
    {
        TraceMsg(TF_DBLIST, "SHAddDataBlock invalid datablock! (sig:%x size:%x)", pdb->dwSignature, pdb->cbSize);
        return E_INVALIDARG;
    }

    // Figure out how much space we need to hold this block
    //
    dwSize = pdb->cbSize;
    if (pdb->cbSize & 0x3)
    {
        dwSize = ((dwSize + 3) & ~0x3) + SIZEOF(DATABLOCK_HEADER);

        TraceMsg(TF_DBLIST, "Adding non-DWORD data block, size:%x sig:%x", pdb->cbSize, pdb->dwSignature);
    }
    else
    {
        TraceMsg(TF_DBLIST, "Adding data block, size:%x sig:%x", pdb->cbSize, pdb->dwSignature);
    }

    // Allocate the space
    //
    if (!*ppdbList)
    {
        *ppdbList = (LPDBLIST)LocalAlloc(LPTR, dwSize + SIZEOF(DWORD)); // include NULL terminator
        pdbCopyTo = *ppdbList;
    }
    else
    {
        DWORD dwTotalSize = 0;
        LPDBLIST pdbList;
        HLOCAL lpTmp;

        for (pdbList = *ppdbList ; pdbList->cbSize ; pdbList = _DBLNext(pdbList))
            dwTotalSize += pdbList->cbSize;

        lpTmp = LocalReAlloc((HLOCAL)*ppdbList, dwTotalSize + dwSize + SIZEOF(DWORD), // include NULL terminator
                             LMEM_ZEROINIT | LMEM_MOVEABLE);
        if (lpTmp)
        {
            *ppdbList = (LPDBLIST)lpTmp;
            pdbCopyTo = (LPDBLIST)(((LPBYTE)lpTmp) + dwTotalSize);
        }
    }

    // Copy the data block
    //
    if (pdbCopyTo)
    {
        LPBYTE pTmp = (LPBYTE)pdbCopyTo;

        // This block would cause other blocks to be
        // unaligned, wrap it
        //
        ASSERT(0 == (dwSize & 0x3));
        if (dwSize != pdb->cbSize)
        {
            pdbCopyTo->cbSize = dwSize;
            pdbCopyTo->dwSignature = DBSIG_WRAP;
            pTmp = (LPBYTE)(pdbCopyTo + 1);
        }
        CopyMemory(pTmp, pdb, pdb->cbSize);

        // NULL terminate the list
        _DBLNext(pdbCopyTo)->cbSize = 0;

        return TRUE;
    }

    return FALSE;
}

BOOL SHRemoveDataBlock(LPDBLIST * ppdbList, DWORD dwSignature)
{
    LPDBLIST pdbRemove = NULL;

    // Can't call SHFindDataBlock because that returnes the
    // block that was wrapped, we want the block that wraps.
    //
    if (*ppdbList)
    {
        LPDBLIST pdbList = *ppdbList;

        for ( ; pdbList->cbSize ; pdbList = _DBLNext(pdbList))
        {
            if (dwSignature == pdbList->dwSignature)
            {
                TraceMsg(TF_DBLIST, "Removing data block, size:%x sig:%x ptr:%x", pdbList->cbSize, pdbList->dwSignature, pdbList);
                pdbRemove = pdbList;
                break;
            }
            else if (DBSIG_WRAP == pdbList->dwSignature)
            {
                LPDBLIST pdbWrap = pdbList + 1;
                if (dwSignature == pdbWrap->dwSignature)
                {
                    TraceMsg(TF_DBLIST, "Removing non-DWORD data block, size:%x sig:%x ptr:", pdbWrap->cbSize, pdbWrap->dwSignature, pdbWrap);
                    pdbRemove = pdbList;
                    break;
                }
            }
        }
    }

    if (pdbRemove)
    {
        LPDBLIST pdbNext = _DBLNext(pdbRemove);
        LPDBLIST pdbEnd;
        DWORD dwSizeOfBlockToRemove;
        LONG lNewSize;

        for (pdbEnd = pdbNext ; pdbEnd->cbSize ; pdbEnd = _DBLNext(pdbEnd))
            ;

        dwSizeOfBlockToRemove = pdbRemove->cbSize;

        // Move remaining memory down
        MoveMemory(pdbRemove, pdbNext, (DWORD_PTR)pdbEnd - (DWORD_PTR)pdbNext + SIZEOF(DWORD));

        // Shrink our buffer
        lNewSize = (LONG) LocalSize(*ppdbList ) - dwSizeOfBlockToRemove;
        if (lNewSize > SIZEOF(DWORD))
        {
            LPVOID lpVoid;
            if (NULL != (lpVoid = LocalReAlloc( (HLOCAL)*ppdbList, lNewSize, LMEM_ZEROINIT | LMEM_MOVEABLE )))
            {
                *ppdbList = (LPDBLIST)lpVoid;
            }
        }
        else
        {
            // We've removed the last section, delete the whole deal
            LocalFree( (HLOCAL)(*ppdbList) );
            *ppdbList = NULL;

        }

        return TRUE;
    }

    return FALSE;
}

LPVOID SHFindDataBlock(LPDBLIST pdbList, DWORD dwSignature)
{
    if (pdbList)
    {
        for ( ; pdbList->cbSize ; pdbList = _DBLNext(pdbList))
        {
            if (dwSignature == pdbList->dwSignature)
            {
                TraceMsg(TF_DBLIST, "Found data block, size:%x sig:%x ptr:%x", pdbList->cbSize, pdbList->dwSignature, pdbList);

                return (LPVOID)pdbList;
            }
            else if (DBSIG_WRAP == pdbList->dwSignature)
            {
                LPDBLIST pdbWrap = pdbList + 1;
                if (dwSignature == pdbWrap->dwSignature)
                {
                    TraceMsg(TF_DBLIST, "Found non-DWORD data block, size:%x sig:%x ptr:%x", pdbWrap->cbSize, pdbWrap->dwSignature, pdbWrap);

                    return (LPVOID)pdbWrap;
                }
            }
        }
    }
    return NULL;
}

} // extern "C"

