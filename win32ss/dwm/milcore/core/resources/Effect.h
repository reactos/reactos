// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Effect resource header.
//
//-----------------------------------------------------------------------------

MtExtern(CMilEffectDuce);

// Class: CMilEffectDuce
class CMilEffectDuce : public CMilSlaveResource
{
    friend class CResourceFactory;
    friend class CMilPixelShaderDuce;

private:
    CComposition* m_pCompositionNoRef;


protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilEffectDuce));

    CMilEffectDuce(__in_ecount(1) CComposition* pComposition)
    {
        m_pCompositionNoRef = pComposition;
    }
    
    CMilEffectDuce() 
    {
    }

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_EFFECT;
    }

    virtual HRESULT ApplyEffect(
        __in CContextState *pContextState,
        __in CHwSurfaceRenderTarget *pDestRT,
        __in_ecount(1) CMILMatrix *pScaleTransform,
        __in CD3DDeviceLevel1 *pDevice, 
        UINT uIntermediateWidth,
        UINT uIntermediateHeight,
        __in_opt CHwTextureRenderTarget *pImplicitInputNoRef
        ) = 0;
    
    virtual HRESULT TransformBoundsForInflation(__inout CMilRectF *bounds) = 0;
    
    virtual HRESULT GetLocalSpaceClipBounds(
        __in CRectF<CoordinateSpace::LocalRendering> unclippedBoundsLocalSpace,
        __in CRectF<CoordinateSpace::PageInPixels> clip,
        __in const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *pWorldTransform,
        __out CRectF<CoordinateSpace::LocalRendering> *pClippedBoundsLocalSpace
        );

    virtual ShaderEffectShaderRenderMode::Enum GetShaderRenderMode();

    virtual HRESULT ApplyEffectSw(
        __in CContextState *pContextState,
        __in CSwRenderTargetSurface *pDestRT,
        __in CMILMatrix *pScaleTransform,
        UINT uIntermediateWidth,
        UINT uIntermediateHeight,
        __in_opt IWGXBitmap *pImplicitInput
        ) = 0;

    virtual HRESULT PrepareSoftwarePass(
        __in const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::DeviceHPC> *pRealizationSamplingToDevice,        
        __inout CPixelShaderState *pPixelShaderState, 
        __deref_out CPixelShaderCompiler **ppPixelShaderCompiler
        ) = 0;

    virtual bool UsesImplicitInput() { return true; }

    virtual byte GetShaderMajorVersion()
    {
        //
        // Used when checking for ps_3_0 support when running a ps_3_0 pixel shader.
        // By default, a shader is not a ps_3_0 pixel shader.
        //
        return 2;
    }

protected:

    __out CComposition *GetCompositionDeviceNoRef() 
    {
        Assert(m_pCompositionNoRef != NULL);
        return m_pCompositionNoRef;
    }


    static HRESULT CreateIntermediateRT(
        __in CD3DDeviceLevel1 *pD3DDevice, 
        __in UINT uWidth, 
        __in UINT uHeight, 
        __in D3DFORMAT d3dfmtTarget,
        __out CD3DVidMemOnlyTexture **ppVidMemOnlyTexture
        );

    static HRESULT SetupVertexTransform(    
        __in const CContextState *pContextState, 
        __in CD3DDeviceLevel1 *pDevice, 
        float destinationWidth, 
        float destinationHeight,
        bool passToFinalDestination
        );

    static HRESULT SetSamplerState(
        __in CD3DDeviceLevel1 *pDevice,
        UINT uSamplerRegister,
        bool setAddressMode,
        bool useBilinear
        );

    //
    // Hw Pixel Shader Cache
    //
    HRESULT GetHwPixelShaderEffectFromCache(
        __in CD3DDeviceLevel1 *pDevice,   
        __in UINT cacheIndex,
        __in bool forceRecreation,
        __in_bcount(sizeInBytes) BYTE *pPixelShaderByteCode,
        __in UINT sizeInBytes,        
        __out_opt CHwPixelShaderEffect **ppPixelShaderEffect);

    void ReleasePixelShaderEffectFromCache(__in UINT cacheIndex);
   
    HRESULT SetPixelShaderCacheCapacity(__in UINT cacheSize);
    

    static HRESULT LockResource(
        __in UINT resourceId, 
        __deref_out_bcount(*pSizeInBytes) BYTE **ppResource,
        __out UINT *pSizeInBytes
        );

};


