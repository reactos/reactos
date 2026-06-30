// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwLinearGradientBrush declaration
//


MtExtern(CHwLinearGradientBrush);

//+----------------------------------------------------------------------------
//
//  Class:     CHwLinearGradientBrush
//
//  Synopsis:  This class implements the primary color source interface for a
//             linear gradient brush
//
//             This class uses a linear gradient color source. It is also a
//             cacheable resource and a poolable brush.  The caching is done on
//             the brush level so that we may cache multiple realizations if
//             needed.
//

class CHwLinearGradientBrush : public CHwCacheablePoolBrush
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwLinearGradientBrush));

    CHwLinearGradientBrush(
        __in_ecount(1) IMILPoolManager *pManager,
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        );

    ~CHwLinearGradientBrush();

    // IMILCacheableResource methods

    override bool IsValid() const;

    // CHwCacheablePoolBrush methods

    override virtual HRESULT SetBrushAndContext(
        __inout_ecount(1) CMILBrush *pBrush,
        __in_ecount(1) const CHwBrushContext &hwBrushContext
        );

    // IHwPrimaryColorSource methods

    override virtual HRESULT SendOperations(
        __inout_ecount(1) CHwPipelineBuilder *pBuilder
        );

    // CHwLinearGradientBrush methods

    HRESULT GetHwTexturedColorSource(
        __deref_out_ecount(1) CHwTexturedColorSource ** const ppColorSource
        );

protected:
    HRESULT SetBrushAndContextInternal(
        __inout_ecount(1) CMILBrush *pBrush,
        __in_ecount(1) const CHwBrushContext &hwBrushContext
        );

private:

    UINT m_uCachedUniquenessToken;  // uniqueness token for cached linear
                                    // gradient brush

protected:
    // Linear Gradient Color Source
    CHwLinearGradientColorSource *m_pLinGradSource;

};




