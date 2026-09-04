// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Abstract:
//      This class is responsible for reading the output of CMilDataStreamWriter.  
//      It's data is organized into multiple allocations called 'blocks' linked 
//      together using SLISTs. Each block contains an array of 'items', which 
//      are just contiguous memory chunks preceded by their size.  The fact that 
//      items are seperated into blocks is abstracted from the client by this 
//      class -- clients just retrieve the 'next' item.
//
//------------------------------------------------------------------------------

#pragma once

class CMilDataBlockReader
{
public:

    CMilDataBlockReader(
        __in_ecount(1) LIST_ENTRY *pDataList
        );

public:

    HRESULT GetFirstItemSafe(
        __out_ecount(1) UINT *pnItemID,
        __deref_out_bcount(*pcbItemSize) PVOID *ppItemData,
        __out_ecount(1) UINT *pcbItemSize
        );

    HRESULT GetNextItemSafe(
        __out_ecount(1) UINT *pnItemID,
        __deref_out_bcount(*pcbItemSize) PVOID *ppItemData,
        __out_ecount(1) UINT *pcbItemSize
        );


protected:

    inline VOID SetCurrentBlock(
        __in_xcount(static_cast<DataStreamBlock*>(pBlock)->cbWritten + sizeof(DataStreamBlock) - 4) LIST_ENTRY *pBlock
        )
    {
        //
        // Set the data pointers to point to the new block
        //
        
        m_pCurrentBlock = static_cast<DataStreamBlock*>(pBlock);

        #pragma prefast (push)
        #pragma prefast ( suppress: __WARNING_POTENTIAL_READ_OVERRUN, "Size of data is not actually 4 bytes" )
        m_streamReader.SetDataAndInitializeFirstItem(m_pCurrentBlock->data, m_pCurrentBlock->cbWritten);
        #pragma prefast (pop)
    }

    LIST_ENTRY *m_pDataListHead;

    DataStreamBlock *m_pCurrentBlock;

    CMilDataStreamReader m_streamReader;
};


