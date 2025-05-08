// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Abstract:
//      Declaration of a general stream reader. The stream maintains 
//      a series of items of the following format:
//
//      [item size (UINT)]--[item id (UINT)]--[item data (remainder)]
//
//------------------------------------------------------------------------------

#pragma once

class CMilDataStreamReader
{
public:
    CMilDataStreamReader(
        __in_bcount(cbBuffer) LPCVOID pcvBuffer,
        UINT cbBuffer
        );

    CMilDataStreamReader() { }

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

    VOID SetDataAndInitializeFirstItem(
        __in_bcount(cbBuffer) LPCVOID pcvBuffer,
        UINT cbBuffer
        )
    {
        m_pbData = reinterpret_cast<PBYTE>(const_cast<PVOID>(pcvBuffer));
        m_cbDataSize = cbBuffer;
        
        m_pbCurItemPos = m_pbData;
    }

protected:
    __field_bcount(m_cbDataSize) BYTE *m_pbData;
    BYTE *m_pbCurItemPos;

    UINT m_cbDataSize;
};


