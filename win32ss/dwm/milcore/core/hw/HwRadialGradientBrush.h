// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    wim_mil_graphics_brush
//      $Keywords:
//
//  $Description:
//      Contains CHwRadialGradientBrush declaration
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


MtExtern(CHwRadialGradientBrush);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwRadialGradientBrush
//
//  Synopsis:
//      This class implements the primary color source interface for a radial
//      gradient brush
//
//      This class is an extension off of linear gradient brush. It is also a
//      cacheable resource and a poolable brush.  The caching is done on the
//      brush level so that we may cache multiple realizations if needed.
//

class CHwRadialGradientBrush : public CHwLinearGradientBrush
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwRadialGradientBrush));

    CHwRadialGradientBrush(
        __in_ecount(1) IMILPoolManager *pManager,
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        );

    ~CHwRadialGradientBrush();

    // CHwCacheablePoolBrush methods

    override HRESULT SetBrushAndContext(
        __inout_ecount(1) CMILBrush *pBrush,
        __in_ecount(1) const CHwBrushContext &hwBrushContext
        );

    // IHwPrimaryColorSource methods

    override HRESULT SendOperations(
        __inout_ecount(1) CHwPipelineBuilder *pBuilder
        );
};





