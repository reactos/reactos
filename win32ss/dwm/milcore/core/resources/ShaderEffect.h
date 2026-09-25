// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      ShaderEffect resource header.
//
//-----------------------------------------------------------------------------

MtExtern(CMilShaderEffectDuce);


// Limits the number of texture stages that can be used for 
// effects. Currently to 16 which is the max for PS 2.0
#define SHADEREFFECT_MAX_TEXTURE_STAGE_CONFIGURATIONS 16 

extern WarpPlatform::LockHandle g_LockJitterAccess;
// SamplingModes matching those defined in managed code.
#define EFFECT_SAMPLING_MODE_NEAREST_NEIGHBOR 0
#define EFFECT_SAMPLING_MODE_BILINEAR 1
#define EFFECT_SAMPLING_MODE_AUTO 2

// Class: CMilShaderEffectDuce
class CMilShaderEffectDuce : 
    public CMilEffectDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilShaderEffectDuce));

    CMilShaderEffectDuce(__in_ecount(1) CComposition* pComposition) : 
        CMilEffectDuce(pComposition)
    {
    }

    ~CMilShaderEffectDuce();

     override virtual BOOL OnChanged(
        CMilSlaveResource *pSender, 
        NotificationEventArgs::Flags e
        );

public:

    //
    // Interface IMILResourceCache inherited via CMILResourceCache requires us 
    // to implement the com base again. Note that this will map to the proper implementation
    // in the base class. 
    //

    DECLARE_COM_BASE

    //
    // Composition Resource Methods
    //

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_SHADEREFFECT || CMilEffectDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_SHADEREFFECT* pCmd,
        __in_bcount(cbPayload) LPCVOID pPayload,
        UINT cbPayload
        );

    HRESULT GeneratedProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_SHADEREFFECT* pCmd,
        __in_bcount(cbPayload) LPCVOID pPayload,
        UINT cbPayload
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    override ShaderEffectShaderRenderMode::Enum GetShaderRenderMode();

    override HRESULT TransformBoundsForInflation(__inout CMilRectF *bounds);
    
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
        );

    override bool UsesImplicitInput();

    override byte GetShaderMajorVersion();

    static HRESULT InitializeJitterLock()
    {
        HRESULT hr = S_OK;

        //    
        // DLL Globals are initialized to NULL, and this should
        // only be called from DLL_PROCESS_ATTACH in DllMain
        //
        Assert(g_LockJitterAccess == NULL);
        if ((g_LockJitterAccess = WarpPlatform::CreateLock()) == NULL)
        {
            IFC(E_FAIL);
        }        
    Cleanup:
        RRETURN(S_OK);
    }

    static void DeInitializeJitterLock()
    {
        WarpPlatform::DeleteLock(g_LockJitterAccess);
    }

private:

    HRESULT DrawIntoIntermediate(
        __in CContextState *pContextState,
        __in CMilBrushDuce *pBrush, 
        __in IRenderTargetInternal *pDestRT, 
        float implicitInputWidth,
        float implicitInputHeight,
        __in const CMILMatrix *pScaleTransform,
        __deref_out IMILRenderTargetBitmap** ppTexture
        );

    HRESULT SendShaderSamplersSw(
        __in CContextState *pContextState,
        __in CSwRenderTargetSurface *pDestRT,
        __in_opt IWGXBitmap *pImplicitInputTexture,
        float implicitInputWidth,
        float implicitInputHeight,
        __in const CMILMatrix *pScaleTransform
        );

    HRESULT PrepareCacheBrushSamplerSw(
        __in CMilBitmapCacheBrushDuce *pBrush,
        __in CSwRenderTargetSurface *pDestRT, 
        __deref_out_opt IWGXBitmap** ppBrushSwTexture
        );

    HRESULT PrepareTileBrushSamplerSw(
        __in CContextState *pContextState,
        __in CMilBrushDuce *pBrush, 
        __in CSwRenderTargetSurface *pDestRT,
        float implicitInputWidth,
        float implicitInputHeight,
        __in const CMILMatrix *pScaleTransform,
        __deref_out IWGXBitmap** ppBrushSwTexture
        );
    
    HRESULT SendShaderConstantsHw(__inout CD3DDeviceLevel1 *pDevice);
 
    HRESULT SendShaderSamplersHw(
        __in CContextState *pContextState,
        __in CD3DDeviceLevel1 *pDevice,
        __in CHwSurfaceRenderTarget *pDestRT,
        __in_opt CD3DVidMemOnlyTexture* pImplicitInputTexture,
        float implicitInputWidth,
        float implicitInputHeight,
        __in const CMILMatrix *pScaleTransform
        );

    HRESULT PrepareTileBrushSamplerHw(
        __in CContextState *pContextState,
        __in CMilBrushDuce *pBrush, 
        __in CHwSurfaceRenderTarget *pDestRT, 
        float implicitInputWidth,
        float implicitInputHeight,
        __in const CMILMatrix *pScaleTransform,
        __deref_out CD3DVidMemOnlyTexture** ppBrushTexture
        );

    HRESULT PrepareCacheBrushSamplerHw(
        __in CMilBitmapCacheBrushDuce *pBrush,
        __in CD3DDeviceLevel1 *pDevice,
        __in CHwSurfaceRenderTarget *pDestRT, 
        __deref_out CD3DVidMemOnlyTexture** ppBrushTexture
        );

    HRESULT ResetTextureStagesHw(
        __in CD3DDeviceLevel1 *pDevice
        );

    void FreeSamplerData();
    
private:

    struct SamplerData
    {
    private:
        DWORD m_samplerRegister;
        DWORD m_samplingMode;
        CD3DVidMemOnlyTexture *m_pD3DTexture;
        CMilBrushDuce *m_pBrush;
        IWGXBitmap *m_pSwTexture;
        IWGXBitmapLock *m_pSwTextureLock;

        SamplerData();
        
    public:        
        void Init(
            DWORD samplerRegister,
            DWORD samplingMode,
            __in_opt CMilBrushDuce* pBrush)
        {
            AssertMsg(m_pBrush == NULL, "Init should only be called once");
            
            ReplaceInterface(m_pBrush, pBrush)
            
            m_samplerRegister = samplerRegister;
            m_samplingMode = samplingMode;
            m_pD3DTexture = NULL;          
            m_pSwTexture = NULL;
            m_pSwTextureLock = NULL;
        }

        void Free()
        {
            ReleaseInterface(m_pBrush);
            ReleaseInterface(m_pD3DTexture);
            ReleaseInterface(m_pSwTexture);
            ReleaseInterface(m_pSwTextureLock);
        }        

        void SetD3DTexture(__in_opt CD3DVidMemOnlyTexture* pD3DTexture)
        {
            ReplaceInterface(m_pD3DTexture, pD3DTexture);
        }

        __out CD3DVidMemOnlyTexture* GetD3DTextureNoRef()
        {
            return m_pD3DTexture;
        }        

        void SetSwTexture(__in_opt IWGXBitmap* pSwTexture)
        {
            ReplaceInterface(m_pSwTexture, pSwTexture);
        }        

        __out_opt IWGXBitmap *GetSwTextureNoRef()
        {
            return m_pSwTexture;
        }

        void SetSwTextureLock(__in_opt IWGXBitmapLock* pSwTextureLock)
        {
            ReplaceInterface(m_pSwTextureLock, pSwTextureLock);
        }

        DWORD GetSamplerRegister() const { return m_samplerRegister; }
        DWORD GetSamplingMode() const { return m_samplingMode; }
        
        CMilBrushDuce* GetBrushNoRef() { return m_pBrush; }
    };


private:    
    CMilShaderEffectDuce_Data m_data;
    UINT32 m_samplerDataCount; // ProcessUpdate ensures that the input sampler count does not 
                               //exceed SHADEREFFECT_MAX_TEXTURE_STAGE_CONFIGURATIONS.
    SamplerData* m_pSamplerData;
        
    CMILBrushShaderEffect *m_pSwShaderEffectBrush;
    float m_destinationWidthSw;
    float m_destinationHeightSw;
};


