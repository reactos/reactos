// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//-----------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(EnhancedContrastTable, MILRender, "EnhancedContrastTable");

//
// In the alpha blending variant to enhance contrast, proposed by John Platt, alpha is replaced by alpha', defined as follows:
//
//               alpha*(k+1)
//     alpha' := -----------
//               alpha*k + 1
//
// for a parameter k that varies continuously between 0 and 1 (or lower and higher).
// This alpha' "behaves" quite similar to the CRTs' non-linear luminosity response to input voltage (gamma curve).
// E.g. for k = 1, the increase of alpha' is "faster than linear", as illustrated in this small table:
//
//     alpha | 0.00 | 0.10 | 0.20 | 0.30 | 0.40 | 0.50 | 0.60 | 0.70 | 0.80 | 0.90 | 1.00
//     ----------------------------------------------------------------------------------
//     alpha'| 0.00 | 0.18 | 0.33 | 0.46 | 0.57 | 0.67 | 0.75 | 0.82 | 0.89 | 0.95 | 1.00
//
// Note that if k=0 then alpha' is equal to alpha.
//
//


//+----------------------------------------------------------------------------
//
//  Member:
//      EnhancedContrastTable::EnhancedContrastTable
//
//-----------------------------------------------------------------------------

EnhancedContrastTable::EnhancedContrastTable()
{
    // Set this variable to a bogus value. This class must be initialized
    // before use.
    m_k = FLOAT_QNAN;
}

//+----------------------------------------------------------------------------
//
//  Member:
//      EnhancedContrastTable::ReInit
//
//  Synopsis:
//      Reinitializes the table given a new contrast level
//
//-----------------------------------------------------------------------------

void
EnhancedContrastTable::ReInit(
    float k
    )
{
    Assert(k >= 0.0f);

    m_k = k;

    // Since we cast to and index into the table using BYTEs, ensure our max alpha
    // is 255.
    C_ASSERT(s_MaxAlpha == 255);

    //
    // Zero always maps to zero.
    // 
    m_table[0] = 0;

    //
    // Calculate the values in between.
    //

    for (UINT alpha = 1; alpha < s_MaxAlpha; ++alpha)
    {
        // Convert alpha to a real number in the range [0,1].
        float const realAlpha = alpha * (1.0f / s_MaxAlpha);

        // Compute enhanced contrast-adjusted alpha
        float const alphaWithContrast = (realAlpha * (k + 1)) / (realAlpha * k + 1);

        // Convert from a [0,1] ranged float to a [0, s_MaxAlpha] ranged BYTE,
        // rounding to the nearest integer
        m_table[alpha] = static_cast<BYTE>(alphaWithContrast * s_MaxAlpha + 0.5f);
    }

    //
    // MaxSubpixelWeight always maps to s_MaxAlpha.
    // 
    m_table[s_MaxAlpha] = s_MaxAlpha;
}


//+----------------------------------------------------------------------------
//
//  Member:
//      EnhancedContrastTable::RenormalizeAndApplyContrast
//
//  Synopsis:
//      Applies contrast enhancement to a buffer of alpha values.
//
//-----------------------------------------------------------------------------

void
EnhancedContrastTable::RenormalizeAndApplyContrast(
    __inout_ecount(stride * height) BYTE *buffer,
    UINT width,
    UINT height,
    UINT stride,
    UINT bufferSize
    ) const
{
    Assert(m_k >= 0.0f);

    BYTE *pRow = buffer;

    //
    // The width must be smaller than the input stride.
    // 
    __analysis_assume(width <= stride);

    for (UINT row = 0; row < height; row++)
    {
        for (UINT i = 0; i < width; i++)
        {
            pRow[i] = m_table[pRow[i]];
        }

        pRow += stride;
    }
}

