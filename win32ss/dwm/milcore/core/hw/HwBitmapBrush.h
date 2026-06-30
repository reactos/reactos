// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_d3d
//      $Keywords:
//
//  $Description:
//      Contains CHwBitmapBrush declaration
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


//  This class is only used in scratch capacity with CHwBrushPool.  It could
//  very well just be a member of that class.
MtExtern(CHwBitmapBrush);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwBitmapBrush
//
//  Synopsis:
//      This class implements the primary color source interface for a bitmap
//      brush
//
//      This class uses a bitmap color source and sometimes a bumpmap source. It
//      is also a cacheable resource and a poolable brush. The caching is done
//      on the brush level so that we may cache multiple realizations if needed.
//

class CHwBitmapBrush : 
    public CHwBrush
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwBitmapBrush));

    CHwBitmapBrush(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        );
    virtual ~CHwBitmapBrush();

    // AddRef is not supported
    STDMETHOD_(ULONG, AddRef)();

    // Release is expected by the caller of CHwBrushPool::GetHwBrush
    STDMETHOD_(ULONG, Release)();

    HRESULT SetBrushAndContext(
        __inout_ecount(1) CMILBrush *pBrush,
        __in_ecount(1) const CHwBrushContext &hwBrushContext
        );

    // IHwPrimaryColorSource methods

    override HRESULT SendOperations(
        __inout_ecount(1) CHwPipelineBuilder *pBuilder
        );

private:

    // Bitmap Color Source
    CHwTexturedColorSource *m_pTexturedSource;

    // Texture Bump Map Source
    CHwTexturedColorSource *m_pBumpMapSource;
};





