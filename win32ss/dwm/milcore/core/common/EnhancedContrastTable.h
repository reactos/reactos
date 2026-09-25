// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_render
//      $Keywords:  
//
//  $Description:
//      Contains header for the enhanced contrast table
//
//  $ENDTAG
//
//-----------------------------------------------------------------------------

#pragma once

MtExtern(EnhancedContrastTable);

//+----------------------------------------------------------------------------
//
//  Class:
//      EnhancedContrastTable
//
//  Synopsis:
//      Class which helps to renormalize and apply enhanced contrast to a
//      buffer.
//
//-----------------------------------------------------------------------------

class EnhancedContrastTable
{
public:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(EnhancedContrastTable));
    
    EnhancedContrastTable();

    void ReInit(
        float k
        );

    float GetContrastValue() const
    {
        return m_k;
    }

    void RenormalizeAndApplyContrast(
        __inout_ecount(stride * height) BYTE *buffer,
        UINT width,
        UINT height,
        UINT stride,
        UINT bufferSize
        ) const;

private:
    float m_k;
    BYTE m_table[256];

    static const UINT s_MaxAlpha = DWRITE_ALPHA_MAX;
};


