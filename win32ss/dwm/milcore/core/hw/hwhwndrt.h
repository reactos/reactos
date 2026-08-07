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
//          Contains CHwHWNDRenderTarget which subclasses CHwDisplayRenderTarget
//      to render to a hwnd device.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

class CD3DSwapChain;
class CD3DDeviceLevel1;

MtExtern(CHwHWNDRenderTarget);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwHWNDRenderTarget
//
//  Synopsis:
//      This object creates the d3d device for the current hwnd and manages a
//      flipping chain.  Note that instead of using the primary flipping chain,
//      a secondary chain is created and used since secondary chains can be
//      resized without recreating the d3d device.
//
//      This object is also repsonsible for responding to resize and disabling
//      rendering when the hwnd is minimized.
//
//------------------------------------------------------------------------------

class CHwHWNDRenderTarget : 
    public CHwDisplayRenderTarget
{
protected:

    friend HRESULT CHwDisplayRenderTarget::Create(
        __in_ecount_opt(1) HWND hwnd,
        MilWindowLayerType::Enum eWindowLayerType,
        __in_ecount(1) CDisplay const *pDisplay,
        D3DDEVTYPE type,
        MilRTInitialization::Flags dwFlags,
        __deref_out_ecount(1) CHwDisplayRenderTarget **ppRenderTarget
        );

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwHWNDRenderTarget));

    CHwHWNDRenderTarget(
        __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
        __in_ecount(1) D3DPRESENT_PARAMETERS const &D3DPresentParams,
        UINT AdapterOrdinalInGroup,
        DisplayId associatedDisplay,
        MilWindowLayerType::Enum eWindowLayerType
        );

public:
    //
    // IRenderTargetHWNDInternal methods
    //

    override void SetPosition(POINT ptOrigin);

    override void UpdatePresentProperties(
        MilTransparency::Flags transparencyFlags,
        BYTE constantAlpha,
        COLORREF colorKey
        );

    override STDMETHODIMP Present(
        __in_ecount(1) const RECT *pRect
        );

    override STDMETHODIMP ScrollBlt (
        __in_ecount(1) const RECT *prcSource,
        __in_ecount(1) const RECT *prcDest
        );    

    override STDMETHODIMP Resize(
        UINT uWidth,
        UINT uHeight
        );

protected:

    override HRESULT Init(
        __in_ecount_opt(1) HWND hwnd, 
        __in_ecount(1) CDisplay const *pDisplay,
        D3DDEVTYPE type,
        MilRTInitialization::Flags dwFlags
        );

private:
    HRESULT UpdateFlippingChain(UINT uWidth, UINT uHeight);

private:

    const MilWindowLayerType::Enum m_eWindowLayerType;
};


