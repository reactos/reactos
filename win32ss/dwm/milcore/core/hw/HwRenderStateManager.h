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
//      Contains CHwRenderStateManager implementation
//
//      The purpose of this class is to centralize all states used by our
//      rendering engine so that we can reduce state transitions for better
//      performance.  We also limit the states that can be set so that we can
//      understand and restrict the requirements for our engine.
//
//      This class also maintains a table of render states and will only call
//      D3D on the states that change.  We've measured noticeable performance
//      improvements by avoiding the D3D API overhead.
//
//      NOTE-2004/05/21-chrisra State blocks are not a win.
//
//      Removing the stateblocks to go to setting the states and restoring them
//      for 3D saved about 20% on our scenarios.  If we have to manage more
//      states that may change, but for the time it looks like a big win to keep
//      from using stateblocks.
//
//  $ENDTAG
//
//  State Table Caches:
//      The recent changes remove the Table caches from the D3DRenderState class
//      In order to do the tables again changes would have to be made to the
//      HwStateTable class so all the states would have a pointer to a pointer
//      to a table that represents values for multiple states.
//
//      The HwRenderStateManager can keep a pointer to a table.  That pointer
//      would be referenced by all the states in the HwStateTable so if any of
//      them were set, they could NULL out the table pointer here in the
//      HwRenderStateManager.  That way if any appropriate state values changed,
//      when the HwRenderStateManager checked it's table pointer would be NULL.
//
//      class CHwRenderStateManager {
//
//      private:
//          AlphaBlendMode *m_pCurrentAlphaBlendModeTable;
//
//          CHwStateTable<DWORD> m_renderStates;
//      };
//
//      Then each entry in the CHwStateTable would have:
//          [StateKnown | StateValue | StateTable]
//
//      And each of the renderstates that were tracked by the AlphaBlendMode
//      table would have their StateTable entry set to
//      &m_pCurrentAlphaBlendModeTable, so if it's set they can set the
//      m_pCurrentAlphaBlendModeTable in CHwRenderStateManager to NULL.
//
//------------------------------------------------------------------------------

// MIL_MAX_TEXTURE_STAGE: The maximum number of texture stages that the class CD3DRenderState
// can use. The actual D3DDevice might not support this many, or it might support more.
#define MIL_TEXTURE_STAGE_COUNT  8
#define MIL_SAMPLER_COUNT        MIL_TEXTURE_STAGE_COUNT

#define NUM_D3DRS 210
#define NUM_D3DTSS 33
#define NUM_D3DTSAMPLERSTATES 14

#define NUM_D3DNONWORLDTRANSFORMS 24

ExternTag(tagWireframe);

MtExtern(CHwRenderStateManager);

class CD3DDeviceLevel1;

//+-----------------------------------------------------------------------------
//
//  Enumeration:
//      PixelShader
//
//  Synopsis:
//      The class CD3DRenderState handles a number of pixel shaders.
//
//------------------------------------------------------------------------------

//
// CalcTextureStateStatePos is a helper function to allow us to use a single
// dimensional state array to keep track of the 2 dimensional TextureStageState
// table.
//
MIL_FORCEINLINE DWORD
CalcTextureStageStatePos(
    DWORD dwStage,
    D3DTEXTURESTAGESTATETYPE stateType
    )
{
    return dwStage*NUM_D3DTSS + stateType;
}

//
// CalcSamplerStatePos is a helper function to allow us to use a single
// dimensional state array to keep track of the 2 dimensional SamplerState
// table.
//
MIL_FORCEINLINE DWORD
CalcSamplerStatePos(
    DWORD dwSampler,
    D3DSAMPLERSTATETYPE stateType
    )
{
    return dwSampler*NUM_D3DTSAMPLERSTATES + stateType;
}

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwRenderStateManager
//
//  Synopsis:
//      Acts as an intermediary device for setting state.
//
//  Responsibilites:
//      - Uses state tables to set state only when the value has changed
//
//------------------------------------------------------------------------------
class CHwRenderStateManager :
         public CMILRefCountBase
{
public:
    static __checkReturn HRESULT Create(
        __inout_ecount(1) IDirect3DDevice9 *pD3DDevice,
        DWORD dwMaxBlendStages,
        BOOL fCanHandleBlendFactor,
        BOOL fSupportsScissorRect,
        __range(1,UINT_MAX) DWORD dwMaxStream,
        __range(1,UINT_MAX) DWORD dwAnisotropicFilterLevel,
        __deref_out_ecount(1) CHwRenderStateManager **ppStateManager
        );

private:
    CHwRenderStateManager();
    ~CHwRenderStateManager();

public:

    DEFINE_REF_COUNT_BASE

    // error C4995: 'D3DMATRIX': name was marked as #pragma deprecated
    //
    // Ignore deprecation of D3DMATRIX for this prototype because
    // it is defined in the interface this class is implementing
#pragma warning (push)
#pragma warning (disable : 4995)
    HRESULT SetTransform(
        D3DTRANSFORMSTATETYPE state,
        __in_ecount(1) const D3DMATRIX *pMatrix
        );
#pragma warning (pop)

    HRESULT SetRenderState(
        D3DRENDERSTATETYPE state,
        DWORD dwValue
        )
    {
        return SetRenderStateInline(
            state,
            dwValue
            );
    }

    HRESULT SetTextureStageState(
        DWORD dwStage,
        D3DTEXTURESTAGESTATETYPE state,
        DWORD dwValue
        )
    {
        return SetTextureStageStateInline(
            dwStage,
            state,
            dwValue
            );
    }

    HRESULT SetVertexShaderConstantF(
        UINT nRegisterIndex,
        __in_ecount(4*nRegisterCount) const float *prConstantData,
        UINT nRegisterCount
        );

    HRESULT SetPixelShaderConstantF(
        UINT nRegisterIndex,
        __in_ecount(4*nRegisterCount) const float * prConstantData,
        UINT nRegisterCount
        );

    HRESULT SetPixelShaderConstantI(
        UINT nRegisterIndex,
        __in_ecount(4) const int * pConstantData
        );

    HRESULT SetPixelShaderConstantB(
        UINT nRegisterIndex,
        BOOL constantData
        );

    //
    // The inlined function calls.  We have to do this because the STDMETHODIMP
    // functions above will not be inlined.  By doing this we can have our dll
    // call the inlined functions by specifying them.
    //

    MIL_FORCEINLINE HRESULT SetRenderStateInline(
        D3DRENDERSTATETYPE state,
        DWORD dwValue
        )
    {
#if DBG
        if (state == D3DRS_FILLMODE)
        {
            if (IsTagEnabled(tagWireframe))
            {
                return S_OK;
            }
        }
#endif
        if (!m_renderStates.IsStateSet(
            state,
            dwValue
            ))
        {
            return ForceSetRenderState(
                state,
                dwValue
                );
        }

        return S_OK;
    }

    MIL_FORCEINLINE HRESULT SetTextureInline(
        DWORD dwStage,
        __in_ecount_opt(1) IDirect3DBaseTexture9 *pTexture
        )
    {
        //
        // Texture can be NULL
        //

        if (!m_textures.IsStateSet(dwStage,pTexture))
        {
            return ForceSetTexture(
                dwStage,
                pTexture
                );
        }

        return S_OK;
    }

    MIL_FORCEINLINE HRESULT SetDepthStencilSurfaceInline(
        __in_ecount_opt(1) IDirect3DSurface9 *pNewDepthSurface,
        UINT uWidth,
        UINT uHeight
        )
    {
        if (!m_depthStencilSurface.IsStateSet(
            0,
            pNewDepthSurface
            ))
        {
            return ForceSetDepthStencilSurface(
                pNewDepthSurface,
                uWidth,
                uHeight
                );
        }

        return S_OK;
    }

    bool
    IsDepthStencilSurfaceSmallerThan(
        UINT uWidth,
        UINT uHeight
        ) const;

    MIL_FORCEINLINE HRESULT ReleaseUseOfDepthStencilBuffer(
        __in_ecount(1) IDirect3DSurface9 *pNewDepthSurface
        )
    {
        if (m_depthStencilSurface.IsStateSet(0,pNewDepthSurface))
        {
            return ForceSetDepthStencilSurface(
                NULL,
                0,
                0
                );
        }

        return S_OK;
    }

    MIL_FORCEINLINE HRESULT SetTextureStageStateInline(
        DWORD dwStage,
        D3DTEXTURESTAGESTATETYPE state,
        DWORD dwValue
        )
    {
        Assert(dwStage <= m_nMaxTextureBlendStage); 

        // If dwStage == dwMaxStage, do nothing - if that stage exists, it
        // defaults to disabled.

        if ((dwStage < m_nMaxTextureBlendStage) &&
            !m_textureStageStates.IsStateSet(
                CalcTextureStageStatePos(dwStage, state),
                dwValue
                ))
        {
            return ForceSetTextureStageState(
                dwStage,
                state,
                dwValue
                );
        }

        return S_OK;
    }

    MIL_FORCEINLINE HRESULT SetSamplerStateInline(
        DWORD dwSampler,
        D3DSAMPLERSTATETYPE state,
        DWORD dwValue
        )
    {
        Assert(dwSampler <= m_nMaxTextureBlendStage);
 
        // If dwStage == dwMaxStage, do nothing - if that stage exists, it defaults
        // to disabled.

        if ((dwSampler < m_nMaxTextureBlendStage) &&
            !m_samplerStageStates.IsStateSet(
                CalcSamplerStatePos(dwSampler, state),
                dwValue
                ))
        {
            return ForceSetSamplerState(
                dwSampler,
                state,
                dwValue
                );
        }

        return S_OK;
    }

    HRESULT SetFVFInline(
        DWORD dwFVF
        )
    {
        if (!m_stateFVF.IsStateSet(
            0,
            dwFVF
            ))
        {
            return ForceSetFVF(
                dwFVF
                );
        }

        return S_OK;
    }

    HRESULT SetPixelShaderInline(
        __in_ecount_opt(1) IDirect3DPixelShader9 *pPixelShader
        )
    {
        //
        // Can have a NULL Pixel Shader
        //

        if (!m_statePixelShader.IsStateSet(0,pPixelShader))
        {
            return ForceSetPixelShader(
                pPixelShader
                );
        }

        return S_OK;
    }

    HRESULT SetVertexShaderInline(
        __in_ecount_opt(1) IDirect3DVertexShader9 *pVertexShader
        )
    {
        //
        // Can have a NULL Vertex Shader
        //

        if (!m_stateVertexShader.IsStateSet(0,pVertexShader))
        {
            return ForceSetVertexShader(
                pVertexShader
                );
        }

        return S_OK;
    }

    //
    // Transforms
    //

    HRESULT Define2DTransforms(
        __in_ecount(1) const CMatrix<CoordinateSpace::DeviceHPC,CoordinateSpace::D3DHomogeneousClipIPC> *pProjection
        );

    HRESULT Set2DTransformForFixedFunction();

    HRESULT Set2DTransformForVertexShader(UINT uStartRegister);
    HRESULT Set3DTransformForVertexShader(UINT uStartRegister);

    HRESULT GetTransform(
        D3DTRANSFORMSTATETYPE state,
        __out_ecount(1) CMILMatrix *pMatrix
        ) const;

    MIL_FORCEINLINE HRESULT SetNonWorldTransform(
        D3DTRANSFORMSTATETYPE state,
        __in_ecount(1) const CBaseMatrix *pMatrix
        )
    {
        HRESULT hr = S_OK;

        Assert(state < 256);

        if (!m_nonWorldTransforms.IsStateSet(
            state,
            *pMatrix
            ))
        {
            IFC(ForceSetNonWorldTransform(
                state,
                pMatrix
                ));

            m_f2DTransformsUsedForFixedFunction = FALSE;
            m_f2DTransformUsedForVertexShader = FALSE;
        }

    Cleanup:
        RRETURN(hr);
    }

    MIL_FORCEINLINE HRESULT SetWorldTransform(
        __in_ecount(1) const CBaseMatrix *pMatrix
        )
    {
        HRESULT hr = S_OK;

        if (!m_worldTransform.IsStateSet(
            0,
            *pMatrix
            ))
        {
            IFC(ForceSetWorldTransform(
                pMatrix
                ));

            m_f2DTransformsUsedForFixedFunction = FALSE;
            m_f2DTransformUsedForVertexShader = FALSE;
        }

    Cleanup:
        RRETURN(hr);
    }

    //
    // ScissorRect Functions
    //
    void InvalidateScissorRect();
    void ScissorRectChanged(
        __in_ecount(1) const MilPointAndSizeL *prc
        );

    HRESULT SetScissorRect(
        __in_ecount_opt(1) const MilPointAndSizeL *prc
        );

    MIL_FORCEINLINE BOOL IsFVFSet(
        DWORD dwFVF
        ) const
    {
        return m_stateFVF.IsStateSet(
            0,
            dwFVF
            );
    }


    MIL_FORCEINLINE HRESULT SetConvolutionMonoKernel(UINT width, UINT height)
    {
        return m_pID3DDeviceEx->SetConvolutionMonoKernel(width, height, NULL, NULL);
    }

    //
    // Clip Functions
    //
    void SetClipSet(BOOL fSet)
        { m_fClipSet = fSet; }

    BOOL IsClipSet(const MilPointAndSizeL *prcClip) const
    {
        return IsClipSet() && RtlEqualMemory(prcClip, &m_rcClip, sizeof(MilPointAndSizeL));
    }

    BOOL IsClipSet() const
        { return m_fClipSet; }

    MilPointAndSizeL GetClip() const
        { return m_rcClip; }

    void SetClip(MilPointAndSizeL rcClip)
        { m_rcClip = rcClip; }

    HRESULT SetViewport(
        __in_ecount(1) const MilPointAndSizeL *prcViewport
        );

    MilPointAndSizeL GetViewport() const
        { return m_rcViewport; }

    //
    // Misc
    //

    HRESULT DisableTextureStage(DWORD dwStage);

    HRESULT GetRenderState(
        D3DRENDERSTATETYPE state,
        __out_ecount(1) DWORD *pdwValue
        ) const
    {
        return m_renderStates.GetState(
            state,
            pdwValue
            );
    }

    MIL_FORCEINLINE HRESULT SetStreamSource(
        __in_ecount_opt(1) IDirect3DVertexBuffer9 *pStream,
        UINT uVertexStride
        )
    {
        if (!m_streamSourceVertexBuffer.IsStateSet(0,pStream) ||
            !m_streamSourceVertexStride.IsStateSet(0,uVertexStride)
           )
        {
            return ForceSetStreamSource(pStream,uVertexStride);
        }
        else
        {
            return S_OK;
        }
    }

    HRESULT SetIndices(
        __in_ecount_opt(1) IDirect3DIndexBuffer9 *pStream
        )
    {
        if (!m_indexStream.IsStateSet(0, pStream))
        {
            return ForceSetIndices(pStream);
        }
        else
        {
            return S_OK;
        }
    }

    HRESULT SetDefaultState(
        BOOL fCanHandleBlendFactor,
        BOOL fSupportsScissorRect,
        __range(1,UINT_MAX) DWORD dwMaxStream,
        __range(1,UINT_MAX) DWORD dwAnisotropicFilterLevel
        );

    HRESULT SetDefaultTexCoordIndices();

private:
    //
    // Force Setting functions.  These functions will set the value passed to
    // them without checking to see if it's changed and update the stored state.
    //
    HRESULT ForceSetTexture(
        DWORD dwStage,
        __in_ecount_opt(1) IDirect3DBaseTexture9 *pTexture
        );

    HRESULT ForceSetVertexShader(
        __in_ecount_opt(1) IDirect3DVertexShader9 *pVertexShader
        );

    HRESULT ForceSetPixelShader(
        __in_ecount_opt(1) IDirect3DPixelShader9 *pPixelShader
        );

    HRESULT ForceSetVertexShaderConstantF(
        UINT nRegisterIndex,
        __in_ecount(4*nRegisterCount) const float *prConstantData,
        UINT nRegisterCount
        );

    HRESULT ForceSetPixelShaderConstantF(
        UINT nRegisterIndex,
        __in_ecount(4*nRegisterCount) const float * prConstantData,
        UINT nRegisterCount
        );

    HRESULT ForceSetPixelShaderConstantI(
        UINT nRegisterIndex,
        __in_ecount(4) const int * pConstantData
        );

    HRESULT ForceSetPixelShaderConstantB(
        UINT nRegisterIndex,
        BOOL pConstantData
        );

    HRESULT ForceSetTextureStageState(
        DWORD dwStage,
        D3DTEXTURESTAGESTATETYPE state,
        DWORD dwValue
        );

    HRESULT ForceSetDepthStencilSurface(
        __in_ecount_opt(1) IDirect3DSurface9 *pDepthStencilSurface,
        UINT uWidth,
        UINT uHeight
        );

    HRESULT ForceSetSamplerState(
        DWORD dwSampler,
        D3DSAMPLERSTATETYPE state,
        DWORD dwValue
        );

    HRESULT ForceSetFVF(
        DWORD dwFVF
        );

    HRESULT ForceSetRenderState(
        D3DRENDERSTATETYPE state,
        DWORD dwValue
        );

    HRESULT ForceSetWorldTransform(
        __in_ecount(1) const CBaseMatrix *pMatrix
        );

    HRESULT ForceSetNonWorldTransform(
        D3DTRANSFORMSTATETYPE state,
        __in_ecount(1) const CBaseMatrix *pMatrix
        );

    HRESULT ForceSetIndices(
        __in_ecount_opt(1) IDirect3DIndexBuffer9 *pStream
        );

    HRESULT ForceSetStreamSource(
        __in_ecount_opt(1) IDirect3DVertexBuffer9 *pStream,
        UINT uVertexStride
        );

#if DBG
    void SetSupportedTable();
#endif

    HRESULT Init(
        __inout_ecount(1) IDirect3DDevice9 *pD3DDevice,
        DWORD dwMaxBlendStages,
        BOOL fCanHandleBlendFactor,
        BOOL fSupportsScissorRect,
        __range(1,UINT_MAX) DWORD dwMaxStream,
        __range(1,UINT_MAX) DWORD dwAnisotropicFilterLevel
        );

    void Check2DTransformInVertexShader(
        UINT nRegisterIndex,
        UINT nRegisterCount
        );

private:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwRenderStateManager));

    // This enum is used internally to remember the state of a render state
    // that can be enabled or disabled.
    enum EnableStatus
    {
        ES_DISABLED = 0,
        ES_ENABLED = 1,
        ES_UNKNOWN = 0xFFFFFFFF
    };

    //
    // D3D Device our state setting functions will be called on
    //
    IDirect3DDevice9* m_pID3DDevice;
    IDirect3DDevice9Ex* m_pID3DDeviceEx;

    //
    // RenderStates
    //
    CStateTable<DWORD> m_renderStates;

    //
    // Texture Stage State
    //
    CStateTable<DWORD> m_textureStageStates;

    //
    // Sampler Stage States
    //
    CStateTable<DWORD> m_samplerStageStates;

    //
    // Transforms
    //
    CStateTable<CMILMatrix> m_nonWorldTransforms;
    CStateTable<CMILMatrix> m_worldTransform;

    //
    // Shaders
    //
    CStateTable<IDirect3DVertexShader9 *> m_stateVertexShader;
    CStateTable<IDirect3DPixelShader9 *> m_statePixelShader;

    // Shader constants (float4 only)
    CStateTable<dxlayer::vector4> m_stateVertexShaderFloat4Constants;
    CStateTable<dxlayer::vector4> m_statePixelShaderFloat4Constants;

    //
    // The int register has a dimension of 4, but we only ever allow a user to set a single value
    // from managed code, which we then duplicate 3 times. So although we're passing 4 elements
    // to DX, we only have to cache the first element.
    // This works out because the MilColorI struct is generated and won't have "==", which
    // CStateTable needs.
    //
    CStateTable<int> m_statePixelShaderInt4Constants;
    CStateTable<BOOL> m_statePixelShaderBoolConstants;
    
    //
    // Textures
    //
    CStateTable<IDirect3DBaseTexture9 *> m_textures;

    CStateTable<IDirect3DSurface9 *> m_depthStencilSurface;

    UINT m_uDepthStencilSurfaceWidth;
    UINT m_uDepthStencilSurfaceHeight;

    CStateTable<DWORD> m_stateFVF;

    //
    // Streams
    //
    CStateTable<IDirect3DIndexBuffer9 *> m_indexStream;

    CStateTable<UINT> m_streamSourceVertexStride;
    CStateTable<IDirect3DVertexBuffer9 *> m_streamSourceVertexBuffer;

    //
    // Scissor rect
    //
    MilPointAndSizeL m_rcScissorRect;

    BOOL m_fClipSet;
    MilPointAndSizeL m_rcClip;

    MilPointAndSizeL m_rcViewport;

    UINT m_nMaxTextureBlendStage;

    //
    // Additional Caching
    //

    // True if D3DTSS_TEXCOORDINDEX is default for all stages.
    bool m_fTexCoordIndicesDefault;

    // Transforms

    CMatrix<CoordinateSpace::DeviceHPC,CoordinateSpace::D3DHomogeneousClipIPC>
        m_mat2DProjectionTransform;

    BOOL m_f2DTransformsUsedForFixedFunction;
    BOOL m_f2DTransformUsedForVertexShader;
    UINT m_u2DTransformVertexShaderStartRegister;
};



