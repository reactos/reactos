// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++



Module Name:

    memreader.cpp

Abstract:

    This class is responsible for reading the output of CMilDataStreamWriter.  It's data
    is organized into multiple allocations called 'blocks' linked together using SLISTs.  
    Each block contains an array of 'items', which are just contiguous memory chunks
    preceded by their size.  The fact that items are seperated into blocks is abstracted
    from the client by this class -- clients just retrieve the 'next' item.

Environment:

    User mode only.


--*/

#include "precomp.hpp"

/*++

Routine Description:

    CMilDataBlockReader::CMilDataBlockReader

--*/

CMilDataBlockReader::CMilDataBlockReader(
    __in_ecount(1) LIST_ENTRY *pDataListHead
    )    
{
    Assert (pDataListHead);
    
    m_pDataListHead = pDataListHead;
    m_pCurrentBlock = NULL;
}

/*++

Routine Description:

    CMilDataBlockReader::GetFirstItemSafe
        Read first item from first block.

--*/

HRESULT CMilDataBlockReader::GetFirstItemSafe(
    __out_ecount(1) UINT *pnItemID,
    __deref_out_bcount(*pcbItemSize) PVOID *ppItemData,
    __out_ecount(1) UINT *pcbItemSize
    )
{
    HRESULT hr = S_OK;
    
    // This is the only valid empty case -- the head points back to itself
    if (m_pDataListHead != m_pDataListHead->Flink)
    {    
        //
        // Set current block to beginning of the list.  The first block follows
        // the list head (i.e., m_pDataListHead->Flink).
        //

        SetCurrentBlock(m_pDataListHead->Flink);
        
        //
        // Now return the current item, if possible.
        //

        MIL_THR(m_streamReader.GetFirstItemSafe(pnItemID, ppItemData, pcbItemSize));

        // Fail if blocks exist after an empty block
        //
        // If S_FALSE is returned for the first item, that means EnsureSize()
        // was called, but then BeginItem/EndItem() wasn't.  To avoid having to loop
        // thru multiple empty blocks, only the last block in the list may be empty.
        // This is ensured by CMilDataStreamWriter::EnsureSize
        if ((S_FALSE == hr) &&
            (m_pCurrentBlock->Flink != m_pDataListHead)
            )
        {
            IFC(E_FAIL);
        }
    }
    else
    {
        hr = S_FALSE;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

/*++

Routine Description:

    CMilDataBlockReader::GetNextItemSafe
        Reads the next item from the current block.  If the current block is empty,
        we move to the next block.

--*/
HRESULT CMilDataBlockReader::GetNextItemSafe(
    __out_ecount(1) UINT *pnItemID,
    __deref_out_bcount(*pcbItemSize) PVOID *ppItemData,
    __out_ecount(1) UINT *pcbItemSize
    )
{
    HRESULT hr;

    // Read the next item in the current block
    MIL_THR(m_streamReader.GetNextItemSafe(pnItemID, ppItemData, pcbItemSize));

    // If we're at the end of the current block, move to the next block.
    if (S_FALSE == hr)
    {        
        LIST_ENTRY *pNextBlock = m_pCurrentBlock->Flink;

        // If we haven't reached the end of the data list, read from the next block
        if (pNextBlock != m_pDataListHead)
        {
            SetCurrentBlock(pNextBlock);

            MIL_THR(m_streamReader.GetNextItemSafe(pnItemID, ppItemData, pcbItemSize));

            // Fail if blocks exist after an empty block
            //
            // To avoid having to loop thru multiple empty blocks, only the last block 
            // in the list may be empty.  This is ensured by CMilDataStreamWriter::EnsureSize
            if ((S_FALSE == hr) &&
                (m_pCurrentBlock->Flink != m_pDataListHead)
                )
            {
                IFC(E_FAIL);
            }
        }
        // Else, we return S_FALSE
    }        

Cleanup:
    RRETURN1(hr, S_FALSE);    
}



