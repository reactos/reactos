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
//      Contains CD3DRenderState implementation
//
//      There are 2 purposes to this class:
//
//      1.  Forward state setting calls to the CHwRenderStateManager
//      2.  Group states commonly set together into tables to make it easier to
//          specify rendering options.
//
//      For the second part there are several objects that contain a collection
//      of renderstates:
//
//          AlphaBlendMode FilterMode TextureStageOperation
//
//      We used to check to see if we had the same table set to minimize work,
//      but since the change to using the CHwRenderStateManager that
//      optimization was removed.  We will likely have to revisit it for
//      performance.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#define D3DCOLORWRITEENABLE_ALL \
    (D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA)

enum TextureBlendArgument
{
    TBA_None = 0,
    TBA_Diffuse = 1,
    TBA_Specular = 2
};

enum TextureBlendMode
{
    TBM_DEFAULT = 0,
    TBM_COPY = 1,
    TBM_APPLY_VECTOR_ALPHA = 2,
    TBM_ADD_COLORS = 3
};

enum PixelShader
{
    // Pixel shaders for text rendering.
    // Notation: PXS_<TextSmoothingType><ColorSource>, where:
    // TextSmoothingType: CT = clear type, GS = grey scale
    // ColorSource: SB = solid brush, TB = textured brush
    // Note that each mnemonic stays for one of two possible
    // pixel shaders, one for D3DFMT_L8 alpha texture and another
    // for D3DFMT_A8, D3DFMT_A8R8G8B8 and D3DFMT_P8. Proper choice
    // is made on Init().
    PXS_CTSB      = 0,
    PXS_GSSB      = 1,
    PXS_CTTB      = 2,
    PXS_GSTB      = 3,
    PXS_NUM
};


struct AlphaBlendMode;
struct TextureStageOperation;
struct FilterMode;

class CD3DSurface;

//+-----------------------------------------------------------------------------
//
//  Class:
//      CD3DRenderState
//
//  Synopsis:
//      The purpose of this class is to centralize all states used by our
//      rendering engine so that we can reduce state transitions for better
//      performance.  We also limit the states that can be set so that we can
//      understand the requirements for our engine.
//
//------------------------------------------------------------------------------
class CD3DRenderState
{
public:
    CD3DRenderState();
    ~CD3DRenderState();

    // Associate a d3d device with the render state object
    HRESULT Init(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __inout_ecount(1) IDirect3DDevice9 *pD3DDevice
        );

    HRESULT ResetState();

    //
    // Need to check how to annotate.
    //


    HRESULT SetVertexShaderConstantF(
        UINT StartRegister,
        __in_ecount(4*RegisterCount) const float *pConstantData,
        UINT RegisterCount
        )
        {
            return m_pStateManager->SetVertexShaderConstantF(
                StartRegister,
                pConstantData,
                RegisterCount
                );
        }

    HRESULT SetPixelShaderConstantF(
        UINT StartRegister,
        __in_ecount(4*RegisterCount) const float *pConstantData,
        UINT RegisterCount
        )
        {
            return m_pStateManager->SetPixelShaderConstantF(
                StartRegister,
                pConstantData,
                RegisterCount
                );
        }

    HRESULT SetPixelShaderConstantI(
        UINT StartRegister,
        __in_ecount(4) const int *pConstantData
        )
        {
            return m_pStateManager->SetPixelShaderConstantI(
                StartRegister,
                pConstantData
                );
        }

    HRESULT SetPixelShaderConstantB(
        UINT StartRegister,
        BOOL constantData
        )
        {
            return m_pStateManager->SetPixelShaderConstantB(
                StartRegister,
                constantData
                );
        }

    // Get alpha texture format that becomes known during Init() and then never changes
    D3DFORMAT GetAlphaTextureFormat() const { return m_alphaTextureFormat; }

    // Setting render state for text
    HRESULT SetRenderState_Text_ClearType_SolidBrush(
        DWORD dwForegroundColor,
        UINT gammaIndex
        );
    HRESULT SetRenderState_Text_ClearType_TextureBrush(
        UINT gammaIndex,
        float flEffectAlpha
        );
    HRESULT SetRenderState_Text_GreyScale_SolidBrush(
        DWORD dwForegroundColor,
        UINT gammaIndex
        );
    HRESULT SetRenderState_Text_GreyScale_TextureBrush(
        UINT gammaIndex,
        float flEffectAlpha
        );

    HRESULT SetFilterMode(
        DWORD dwSampler,
        __in_ecount(1) const FilterMode *pfmNew
        );

    HRESULT SetAlphaBlendMode(
        __in_ecount(1) const AlphaBlendMode *pabmNew
        );

    HRESULT GetFillMode(
        __out_ecount(1) D3DFILLMODE *d3dFillMode
        ) const;

    HRESULT GetDepthTestFunction(
        __out_ecount(1) D3DCMPFUNC *pd3dDepthTestFunction
        ) const;

    // Auxiliary render state switching for text
    // (to be called after proper SetRenderState_Text_*())
    HRESULT SetColorChannelRed();
    HRESULT SetColorChannelGreen();
    HRESULT SetColorChannelBlue();
    HRESULT RestoreColorChannels();


    /////////////////////////////////////////////////
    //
    //   High-Level Render State Setting Methods
    //
    //-----------------------------------------------

    HRESULT SetRenderState_AlphaSolidBrush();

    HRESULT SetRenderState_Texture(
        TextureBlendMode blendMode,
        TextureBlendArgument eBlendArgument,
        MilBitmapInterpolationMode::Enum interpolationMode,
        UINT cMasks
    );


    //
    // Forwarded calls to the RenderStateManager
    //

    MIL_FORCEINLINE HRESULT DisableTextureStage(
        DWORD dwStage
        )
    {
        return m_pStateManager->DisableTextureStage(
            dwStage
            );
    }

    MIL_FORCEINLINE HRESULT SetConvolutionMonoKernel(UINT width, UINT height)
    {
        return m_pStateManager->SetConvolutionMonoKernel(
            width,
            height
            );
    }

    MIL_FORCEINLINE HRESULT SetTransform(
        D3DTRANSFORMSTATETYPE state,
        __in_ecount(1) const CMILMatrix *pMatrix
        )
    {
        return m_pStateManager->SetTransform(
            state,
            pMatrix
            );
    }

    MIL_FORCEINLINE HRESULT SetWorldTransform(
        __in_ecount(1) const CBaseMatrix *pMatrix
        )
    {
        return m_pStateManager->SetWorldTransform(
            pMatrix
            );
    }

    MIL_FORCEINLINE HRESULT SetNonWorldTransform(
        D3DTRANSFORMSTATETYPE state,
        __in_ecount(1) const CMILMatrix *pMatrix
        )
    {
        return m_pStateManager->SetNonWorldTransform(
            state,
            pMatrix
            );
    }

    MIL_FORCEINLINE HRESULT GetTransform(
        D3DTRANSFORMSTATETYPE state,
        __out_ecount(1) CMILMatrix *pMatrix
        ) const
    {
        return m_pStateManager->GetTransform(
            state,
            pMatrix
            );
    }

    MIL_FORCEINLINE HRESULT SetRenderState(
        D3DRENDERSTATETYPE state,
        DWORD dwValue
        )
    {
        return m_pStateManager->SetRenderStateInline(
            state,
            dwValue
            );
    }

    MIL_FORCEINLINE HRESULT SetTextureStageState(
        DWORD dwStage,
        D3DTEXTURESTAGESTATETYPE state,
        DWORD dwValue
        )
    {
        return m_pStateManager->SetTextureStageStateInline(
            dwStage,
            state,
            dwValue
            );
    }

    MIL_FORCEINLINE HRESULT SetDefaultTexCoordIndices()
    {
        return m_pStateManager->SetDefaultTexCoordIndices();
    }

    MIL_FORCEINLINE HRESULT DisableTextureTransform(DWORD dwStage)
    {
        return m_pStateManager->SetTextureStageStateInline(
            dwStage,
            D3DTSS_TEXTURETRANSFORMFLAGS,
            D3DTTFF_DISABLE
            );
    }

    MIL_FORCEINLINE HRESULT SetSamplerState(
        DWORD dwSampler,
        D3DSAMPLERSTATETYPE state,
        DWORD dwValue
        )
    {
        //
        // Setting the ADDRESSU sampler state to NULL is not supported.
        //
        Assert(dwValue || state != D3DSAMP_ADDRESSU);

        //
        // Setting the ADDRESSV sampler state to NULL is not supported.
        //
        Assert(dwValue || state != D3DSAMP_ADDRESSV);

        return m_pStateManager->SetSamplerStateInline(
            dwSampler,
            state,
            dwValue
            );
    }

    MIL_FORCEINLINE HRESULT SetTexture(
        DWORD dwStage,
        __in_ecount_opt(1) IDirect3DBaseTexture9 *pTexture
        )
    {
        return m_pStateManager->SetTextureInline(
            dwStage,
            pTexture
            );
    }

    MIL_FORCEINLINE HRESULT SetVertexShader(
        __in_ecount_opt(1) IDirect3DVertexShader9 *pVertexShader
        )
    {
        return m_pStateManager->SetVertexShaderInline(
            pVertexShader
            );
    }

    MIL_FORCEINLINE HRESULT SetPixelShader(
        __in_ecount_opt(1) IDirect3DPixelShader9 *pPixelShader
        )
    {
        return m_pStateManager->SetPixelShaderInline(
            pPixelShader
            );
    }

    MIL_FORCEINLINE HRESULT Define2DTransforms(
        __in_ecount(1) const CMatrix<CoordinateSpace::DeviceHPC,CoordinateSpace::D3DHomogeneousClipIPC> *pProjection
        )
    {
        return m_pStateManager->Define2DTransforms(
            pProjection
            );
    }

    MIL_FORCEINLINE HRESULT SetStreamSource(
        __in_ecount_opt(1) IDirect3DVertexBuffer9 *pStream,
        UINT uVertexStride
        )
    {
        return m_pStateManager->SetStreamSource(
            pStream,
            uVertexStride
            );

    }

    MIL_FORCEINLINE HRESULT SetIndices(
        __in_ecount_opt(1) IDirect3DIndexBuffer9 *pStream
        )
    {
        return m_pStateManager->SetIndices(
            pStream
            );
    }

    MIL_FORCEINLINE BOOL IsFVFSet(
        DWORD dwFVF
        ) const
    {
        return m_pStateManager->IsFVFSet(dwFVF);
    }

    MIL_FORCEINLINE HRESULT Set2DTransformForFixedFunction()
    {
        return m_pStateManager->Set2DTransformForFixedFunction();
    }

    MIL_FORCEINLINE HRESULT Set2DTransformForVertexShader(
        UINT uStartRegister
        )
    {
        return m_pStateManager->Set2DTransformForVertexShader(
            uStartRegister
            );
    }

    MIL_FORCEINLINE HRESULT Set3DTransformForVertexShader(
        UINT uStartRegister
        )
    {
        return m_pStateManager->Set3DTransformForVertexShader(
            uStartRegister
            );
    }

    MIL_FORCEINLINE MilPointAndSizeL GetClip() const
    {
        return m_pStateManager->GetClip();
    }

    MIL_FORCEINLINE BOOL IsClipSet() const
    {
        return m_pStateManager->IsClipSet();
    }

    MIL_FORCEINLINE BOOL IsClipSet(const MilPointAndSizeL *prcClip) const
    {
        return m_pStateManager->IsClipSet(prcClip);
    }

    MIL_FORCEINLINE void SetClip(MilPointAndSizeL rcClip)
    {
        m_pStateManager->SetClip(rcClip);
    }

    MIL_FORCEINLINE HRESULT SetViewport(
        __in_ecount(1) const MilPointAndSizeL *prcViewport
        )
    {
        return m_pStateManager->SetViewport(prcViewport);
    }

    MIL_FORCEINLINE MilPointAndSizeL GetViewport() const
    {
        return m_pStateManager->GetViewport();
    }

    MIL_FORCEINLINE HRESULT SetFVF(DWORD dwFVF)
    {
        return m_pStateManager->SetFVFInline(dwFVF);
    }

    MIL_FORCEINLINE void SetClipSet(BOOL fSet)
    {
        return m_pStateManager->SetClipSet(fSet);
    }

    MIL_FORCEINLINE void ScissorRectChanged(
        __in_ecount(1) const MilPointAndSizeL *prc
        )
    {
        return m_pStateManager->ScissorRectChanged(prc);
    }

    MIL_FORCEINLINE HRESULT SetScissorRect(
        __in_ecount_opt(1) const MilPointAndSizeL *prc
        )
    {
        return m_pStateManager->SetScissorRect(prc);
    }

protected:
    //
    // Functions only for CD3DDeviceLevel1
    //

    bool IsDepthStencilSurfaceSmallerThan(
        UINT uWidth,
        UINT uHeight
        ) const
    {
        return m_pStateManager->IsDepthStencilSurfaceSmallerThan(uWidth, uHeight);
    }

    HRESULT SetDepthStencilSurfaceInternal(
        __in_ecount_opt(1) CD3DSurface *pDepthStencilBuffer
        );

    HRESULT ReleaseUseOfDepthStencilSurfaceInternal(
        __in_ecount(1) CD3DSurface *pDepthStencilBuffer
        );

private:

#if DBG
    VOID AssertFilterMode(DWORD dwSampler);
    VOID AssertAlphaBlendMode();
    VOID AssertPixelShader();
    VOID AssertTextureStageOperation(DWORD dwStage);
#else
    VOID AssertFilterMode(DWORD dwSampler) {}
    VOID AssertAlphaBlendMode() {}
    VOID AssertPixelShader() {}
    VOID AssertTextureStageOperation(DWORD dwStage) {}
#endif

public:

    /////////////////////////////////////////////////
    //
    //   Low-Level Render State Setting Methods
    //
    //-----------------------------------------------
    HRESULT SetTextureStageOperation(
        DWORD dwStage,
        __in_ecount(1) const TextureStageOperation *ptsoNew
        );

    BOOL CanDrawText() const { return m_fCanDrawText; }
    BOOL CanDrawTextUsingPS20() const { return m_fDrawTextUsingPS20; }

    HRESULT SetClearTypeOffsets(float ds, float dt);

public:

    // Render state variants
    static const AlphaBlendMode
        sc_abmUnknown,
        sc_abmSrcCopy,
        sc_abmDestCopy,
        sc_abmSrcOverPremultiplied,
        sc_abmSrcUnderPremultiplied,
        sc_abmSrcAlphaMultiply,
        sc_abmSrcInverseAlphaMultiply,
        sc_abmSrcOver_SrcNonPremultiplied,
        sc_abmSrcOver_InverseAlpha_SrcNonPremultiplied,
        sc_abmSrcVectorAlphaWithBlendFactor,
        sc_abmSrcVectorAlpha,
        sc_abmAddSourceColor,
        sc_abmSrcAlphaWithInvDestColor;

    static const FilterMode
        sc_fmUnknown,
        sc_fmNearest,
        sc_fmLinear,
        sc_fmTriLinear,
        sc_fmAnisotropic,
        sc_fmMinOnlyAnisotropic,
        sc_fmConvolution;

    static const TextureStageOperation
        sc_tsoUnknown,
        sc_tsoDiffuse,
        sc_tsoTextureXCurrentRGB,
        sc_tsoTextureXSpecularRGB,
        sc_tsoSelectTexture,
        sc_tsoPremulTextureXCurrent,
        sc_tsoPremulTextureXDiffuse,
        sc_tsoOpaqueTextureXCurrent,
        sc_tsoOpaqueTextureXDiffuse,
        sc_tsoMaskTextureXCurrent,
        sc_tsoBumpMapTexture,
        sc_tsoColorSelectTextureAlphaMultiplyDiffuse,
        sc_tsoColorSelectTextureAlphaMultiplyCurrent,
        sc_tsoColorSelectDiffuseAlphaMultiplyTexture,
        sc_tsoColorSelectCurrentAlphaMultiplyTexture;

private:
    CHwRenderStateManager *m_pStateManager;
    CD3DDeviceLevel1 *m_pDeviceNoRef;

    // format of the textures used to represent glyph run shapes
    D3DFORMAT m_alphaTextureFormat;
    HRESULT InitAlphaTextures();

    BOOL m_fCanDrawText;
    BOOL m_fDrawTextUsingPS20;

    IDirect3DPixelShader9* m_pPixelShaders[PXS_NUM];
    const FilterMode* m_pTextFilterMode;

    HRESULT InitPixelShaders();

    HRESULT SetConstantRegisters_SolidBrush_PS11(
        DWORD dwColor,
        UINT gammaIndex
        );

    HRESULT SetConstantRegisters_SolidBrush_PS20(
        DWORD dwColor,
        UINT gammaIndex
        );

    HRESULT SetConstantRegisters_TexturedBrush(
        UINT gammaIndex,
        float flEffectAlpha
        );

};


