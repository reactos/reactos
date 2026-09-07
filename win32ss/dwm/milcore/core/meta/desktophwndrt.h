// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_targets
//      $Keywords:
//
//  $Description:
//          Contains CDesktopHWNDRenderTarget which adds hwnd support
//      to CDesktopRenderTarget. This includes the ability to resize a window and
//      enable/disable rendering on adapters that the window does not cover
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


//+-----------------------------------------------------------------------------
//
//  Class:
//      CDesktopHWNDRenderTarget
//
//  Synopsis:
//      CDesktopHWNDRenderTarget implementation
//
//      This is a multiple or meta Render Target for rendering a resizeable hwnd
//      on multiple desktop devices.  It handles enumerating the devices and
//      managing an array of sub-targets.
//
//------------------------------------------------------------------------------

MtExtern(CDesktopHWNDRenderTarget);

class CDesktopHWNDRenderTarget:
    public CDesktopRenderTarget
{

protected:

    void ComputeRenderAndAdjustPresentBounds(
        __inout_ecount(1) MetaData &oDevData,
        __in_range(0,INT_MAX) UINT uMaxRight,
        __in_range(0,INT_MAX) UINT uMaxBottom
        ) const;

    override HRESULT EditMetaData();

    HRESULT ResizeSubRT(
        __in_range(<=, (this->m_cRT)) UINT i,
        __in_range(>=, 1) UINT uWidthNew,
        __in_range(>=, 1) UINT uHeightNew
        );

public:

    void * __cdecl operator new(size_t cb, size_t cRTs);
    inline void __cdecl operator delete(void * pv) { WPFFree(ProcessHeap, pv); }
    inline void __cdecl operator delete(void * pv, size_t) { WPFFree(ProcessHeap, pv); }

    CDesktopHWNDRenderTarget(
        UINT cMaxRTs,
        __inout_ecount(1) CDisplaySet const *pDisplaySet,
        MilWindowLayerType::Enum eWindowLayerType
        );

    // IMILRenderTargetHWND.

    override STDMETHODIMP SetPosition(
        __in_ecount(1) MilRectF const *prc
        );

    override STDMETHODIMP GetInvalidRegions(
        __deref_outro_ecount(*pNumRegions) MilRectF const ** const prgRegions,
        __out_ecount(1) UINT *pNumRegions,
        __out bool *fWholeTargetInvalid        
        );

    override STDMETHODIMP_(VOID) GetIntersectionWithDisplay(
        UINT iDisplay,
        __out_ecount(1) MilRectL &rcIntersection
        );

    override STDMETHODIMP UpdatePresentProperties(
        MilTransparency::Flags transparencyFlags,
        FLOAT constantAlpha,
        __in_ecount(1) MilColorF const &colorKey
        );

    override STDMETHOD(Present)(
        );

#if DBG
protected:
    override bool DbgIsValidTransition(enum State eNewState);

#endif

protected:

    const MilWindowLayerType::Enum m_eWindowLayerType;  // Type of Win32 window
                                                        // layer we expect to
                                                        // deal with.  Any
                                                        // change requires the
                                                        // render target to be
                                                        // recreated.

    __field_ecount_part(4*m_cRT,0) // 4* = MAX_INVALID_REGIONS_PER_DEVICE*
    CMilRectF * const m_rgInvalidRegions;    // Scratch buffer for returning
                                                // invalid areas via
                                                // GetInvalidRegions.  This
                                                // allocated upfront to avoid
                                                // an allocation every frame.

    MilTransparency::Flags m_ePresentTransparency;
    BYTE m_bPresentAlpha;
    COLORREF m_crPresentColorKey;
};



