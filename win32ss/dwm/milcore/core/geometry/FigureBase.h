// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_geometry
//      $Keywords:
//
//  $Description:
//      Defines the methods for processing a figure
//
//  $ENDTAG
//
//  Classes:
//      CFigureBase
//
//------------------------------------------------------------------------------

MtExtern(CFigureBase);

class CFigureBase
{
public:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CFigureBase));

public:
    CFigureBase(__in_ecount(1) const IFigureData &data)
    : m_refData(data)
    {
    }

    virtual ~CFigureBase()
    {
    }

    HRESULT AddToGpPath(
        __inout_ecount(1) DynArray<MilPoint2F> &rgPoints,
            // Path points
        __inout_ecount(1) DynArray<BYTE> &rgTypes,
            // Path types
        bool fSkipGaps
            // Skip no-stroke segments if true
        ) const;

    void Transform(
        __in_ecount(1) const CMILMatrix &matrix
            // In: Transformation matrix
        );

    HRESULT UpdateBounds(
        __inout_ecount(1) CBounds &bounds,
            // In/out: Bounds, updated here
        __in_ecount_opt(1) const CMILMatrix *pMatrix
            // In: Transformation (NULL OK)
        ) const;

    HRESULT Populate(
        __in_ecount(1) IPopulationSink *scanner,
            // The receiving scanner
        __in_ecount_opt(1) const CMILMatrix *pMatrix) const;
            // Transformation matrix (NULL OK)

#ifdef DBG
    void Dump() const;
#endif

     // Data
protected:
    const IFigureData &m_refData;     // The data
};



