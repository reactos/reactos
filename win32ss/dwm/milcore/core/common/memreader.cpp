// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Abstract:
//      Implementation of a general stream reader. The stream maintains 
//      a series of items of the following format:
//
//      [item size (UINT)]--[item id (UINT)]--[item data (item size - 4 bytes)]
//
//------------------------------------------------------------------------------

#include "precomp.hpp"


//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilDataStreamReader constructor
//
//------------------------------------------------------------------------------

CMilDataStreamReader::CMilDataStreamReader(
    __in_bcount(cbBuffer) LPCVOID pcvBuffer,
    UINT cbBuffer
    )
{
    m_pbData = reinterpret_cast<PBYTE>(const_cast<PVOID>(pcvBuffer));
    m_cbDataSize = cbBuffer;
    m_pbCurItemPos = NULL;
}


//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilDataStreamReader::GetFirstItemSafe
//
//    Synopsis:
//        Resets the reader to the first item in the buffer and returns it.
//        Performs all necessary validations to make sure that the memory
//        is valid.
//
//------------------------------------------------------------------------------

HRESULT 
CMilDataStreamReader::GetFirstItemSafe(
    __out_ecount(1) UINT *pnItemID,
    __deref_out_bcount(*pcbItemSize) PVOID *ppItemData,
    __out_ecount(1) UINT *pcbItemSize
    )
{
    //
    // Seek to the first item in the buffer
    //

    m_pbCurItemPos = m_pbData;


    //
    // Now return the current item, if possible.
    //

    RRETURN1(GetNextItemSafe(pnItemID, ppItemData, pcbItemSize), S_FALSE);
}


//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilDataStreamReader::GetNextItemSafe
//
//    Synopsis:
//        Reads the next item in the buffer and increments to the next one. 
//        Performs all necessary validations to make sure that the memory
//        is valid.
//
//        The IFC(E_FAIL)s are a little awkward, but they are like that to
//        maintain original behavior while providing accurate failure capture.
//
//------------------------------------------------------------------------------

HRESULT 
CMilDataStreamReader::GetNextItemSafe(
    __out_ecount(1) UINT *pnItemID,
    __deref_out_bcount(*pcbItemSize) PVOID *ppItemData,
    __out_ecount(1) UINT *pcbItemSize
    )
{
    HRESULT hr = S_OK;

#if DGB
    //
    // The m_pCurItemPos is first set by GetFirstItemSafe. If it's NULL, we
    // either haven't called GetFirstItemSafe before calling this method
    // or we have been passed an empty batch.
    //

    Assert(m_pbCurItemPos != NULL || m_nDataSize == 0);


    //
    // If m_pbCurItemPos is not NULL, it points to the buffer defined as 
    // [m_pbData, m_pbData+m_cbDataSize) (note: left inclusive) if the buffer 
    // has not been exhausted yet and to [m_pbData+m_cbDataSize] otherwise.
    //
    // Note that this arithmetic also works if the initial buffer is empty.
    //

    size_t cbCurrentItemOffset = 0;

    Assert(m_pCurItemPos >= m_pData);
    Assert(SUCCEEDED(PtrdiffTToSizeT(m_pCurItemPos - m_pData, &cbCurrentItemOffset)));
    Assert(cbCurrentItemOffset <= m_nDataSize);
#endif DBG

    //
    // Check if there's any data left in the buffer.
    //

    size_t cbRem = m_cbDataSize - (m_pbCurItemPos - m_pbData);

    if (cbRem == 0)
    {
        //
        // We reached the end of the data set.
        //

        *ppItemData = NULL;
        *pcbItemSize = 0;

        hr = S_FALSE;
    }
    else if (cbRem >= 2 * sizeof(UINT))
    {
        //
        // Read the current item size from the buffer.
        //

        UINT cbItemSize = *(reinterpret_cast<UINT *>(m_pbCurItemPos));

        //
        // Make sure that the item fits in the buffer. We expect to have at least
        // the item size and the item ID in the buffer, that makes two 32-bit
        // integers total. The item size must be a multiple of sizeof(UINT).
        //
        // The item size could still be wrong -- we have to verify it against
        // the item type (and, possibly, item contents) later.
        //

        if ((cbItemSize >= 2 * sizeof(UINT))
            && (cbItemSize % sizeof(UINT) == 0)
            && (cbItemSize <= cbRem))
        {

            *pnItemID = *(reinterpret_cast<UINT *>(m_pbCurItemPos + sizeof(UINT)));

            //
            // Return the item and its length
            //

            *ppItemData = m_pbCurItemPos + sizeof(UINT);  // skip size
            *pcbItemSize = cbItemSize - sizeof(UINT);


            //
            // Advance the current item pointer to the next item.
            //

            m_pbCurItemPos += cbItemSize;
        }
        else
        {
            IFC(E_FAIL);
        }
    }
    else
    {
        IFC(E_FAIL);
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

