// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Drop shadow  effect resource header.
//
//-----------------------------------------------------------------------------

MtExtern(CMilDropShadowEffectDuce);

// Class: CMilDropShadowEffectDuce
class CMilDropShadowEffectDuce : public CMilEffectDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilBlurEffectDuce));

    CMilDropShadowEffectDuce(__inout_ecount(1) CComposition* pComposition) : CMilEffectDuce(pComposition)
    {
        m_pComposition = pComposition;
    }

    ~CMilDropShadowEffectDuce();
    
    override HRESULT Initialize();    

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_DROPSHADOWEFFECT || CMilEffectDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_DROPSHADOWEFFECT* pCmd
        );

    override HRESULT ApplyEffect(
        __in CContextState *pContextState, 
        __in CHwSurfaceRenderTarget *pDestRT,
        __in CMILMatrix *pScaleTransform,
        __in CD3DDeviceLevel1 *pDevice, 
        UINT uIntermediateWidth,
        UINT uIntermediateHeight,
        __in_opt CHwTextureRenderTarget *pImplicitInput
        );

    override HRESULT ApplyEffectSw(
        __in CContextState *pContextState,
        __in CSwRenderTargetSurface *pDestRT,
        __in CMILMatrix *pScaleTransform, 
        UINT uIntermediateWidth,
        UINT uIntermediateHeight,
        __in_opt IWGXBitmap *pImplicitInput
        );

    override HRESULT PrepareSoftwarePass(
        __in const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::DeviceHPC> *pRealizationSamplingToDevice,
        __inout CPixelShaderState *pPixelShaderState, 
        __deref_out CPixelShaderCompiler **ppPixelShaderCompiler
        )
    {
        RRETURN(E_UNEXPECTED);
    }

    override HRESULT TransformBoundsForInflation(__inout CMilRectF *bounds);

    override HRESULT GetLocalSpaceClipBounds(
        __in CRectF<CoordinateSpace::LocalRendering> unclippedBoundsLocalSpace,
        __in CRectF<CoordinateSpace::PageInPixels> clip,
        __in const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *pWorldTransform,
        __out CRectF<CoordinateSpace::LocalRendering> *pClippedBoundsLocalSpace
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

private:

    static inline bool IsOpaque(UINT pixel)
    {
        return ((pixel & MIL_ALPHA_MASK) == MIL_ALPHA_MASK);
    }

    static inline bool IsTranslucent(UINT pixel)
    {
        return ((pixel & MIL_ALPHA_MASK) != MIL_ALPHA_MASK);
    }

    static inline UINT ConvertColor(const MilColorF &pixel)
    {
        return MIL_COLOR(static_cast<UINT>(pixel.a * 255.0),
                         static_cast<UINT>(pixel.r * 255.0),
                         static_cast<UINT>(pixel.g * 255.0),
                         static_cast<UINT>(pixel.b * 255.0)
                         );
    }

    static UINT * AdjustSourcePointer(UINT *pBuffer, INT_PTR offsetX, INT_PTR offsetY, UINT width);

    static HRESULT GaussianBlurLineOfPixels(
        __in_ecount(sourceWidth * sourceHeight) UINT *pInputBuffer,
        __in_ecount(nPixels + 2 * radius) UINT *pBlurIntermediateBuffer,
        __inout_ecount(nPixels) UINT *pOutputBuffer,
        UINT sourceWidth,
        UINT sourceHeight,
        UINT radius,
        UINT nPixels,
        __in_ecount(2 * radius + 1) float *pGaussianWeights
        );

    HRESULT SetupShader(
        __in CD3DDeviceLevel1 *pDevice,
        __in const CMILMatrix *pScaleTransform,
        __in float destinationWidth,
        __in float destinationHeight
        );

    void CalculateOffset(
        __in bool isScaled, 
        __in_opt const CMILMatrix *pScaleTransform,
        __out float *offsetX,
        __out float *offsetY
        );

    double GetBlurRadius();

    double GetDirection();

    double GetShadowDepth();

    double GetOpacity();

    MilColorF GetColor();

    HRESULT TransformBoundsInternal(
        __in bool isForClipping,
        __inout CMilRectF *pBounds);

private:
    CMilDropShadowEffectDuce_Data m_data;

    //
    // pointer to the pixel shader being used
    static CMilPixelShaderDuce* s_pPixelShader;

    // Holds the compiled SIMD code for the blur and color function
    static GenerateColorsBlur s_pfnBlurGaussianAndColor;

    CComposition* m_pComposition;
};

