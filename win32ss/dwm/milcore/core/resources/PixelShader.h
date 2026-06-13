// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      PixelShader resource header.
//
//-----------------------------------------------------------------------------

MtExtern(CMilPixelShaderDuce);

// Class: CMilPixelShaderDuce
class CMilPixelShaderDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:

    CMilPixelShaderDuce(__in_ecount(1) CComposition* pComposition)
    {
        m_pComposition = pComposition;
    }

    ~CMilPixelShaderDuce();

public:

    //
    // Interface IMILResourceCache inherited via CMILResourceCache requires us 
    // to implement the com base again. Note that this will map to the proper implementation
    // in the base class. 
    //

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilPixelShaderDuce));

    DECLARE_COM_BASE

    static HRESULT Create(
        __in_ecount(1) CComposition *pComposition,
        __in ShaderEffectShaderRenderMode::Enum shaderEffectShaderRenderMode,
        __in UINT cbBytecodeSize,
        __in_bcount(cbBytecodeSize) BYTE* pBytecode, 
        __deref_out CMilPixelShaderDuce **ppOut);

    //
    // Composition Resource Methods
    //

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_PIXELSHADER;
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_PIXELSHADER* pCmd,
        __in_bcount(cbPayload) LPCVOID pPayload,
        UINT cbPayload);

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();
    override CMilSlaveResource* GetResource();

    // Gets the right effect for the device and sets it in.
    HRESULT SetupShader(__in CD3DDeviceLevel1* pDevice);

    HRESULT GetSwPixelShader(__deref_out CPixelShaderCompiler **ppPixelShaderCompiler);

    ShaderEffectShaderRenderMode::Enum GetShaderRenderMode() const;

    byte GetShaderMajorVersion();

protected:
    
     override virtual BOOL OnChanged(
        CMilSlaveResource *pSender, 
        NotificationEventArgs::Flags e
        );

private:
    CMilPixelShaderDuce();

    HRESULT Initialize(
        __inout_ecount(1) CComposition *pComposition,
        __in ShaderEffectShaderRenderMode::Enum shaderEffectShaderRenderMode,
        __in UINT cbBytecodeSize, 
        __in BYTE* pBytecode);
        
    HRESULT GetHwPixelShaderEffectFromCache(
        __in CD3DDeviceLevel1 *pDevice,   
        __out CHwPixelShaderEffect **ppPixelShaderEffect);

    static HRESULT EnsurePassThroughShaderResourceRead();

private:
    CComposition            *m_pComposition;
    CMilPixelShaderDuce_Data m_data;
    bool                     m_ignoreHwShader;
    
    // The hw cache is used for caching the device specific
    // pixel shader IDirect3DPixelShader9 object. 
    CMILSimpleResourceCache* m_hwPixelShaderEffectCache;

    CPixelShaderCompiler *m_pSwPixelShaderCompiler;
    static BYTE* m_pPassThroughShaderBytecodeData;
    static UINT  m_cbPassThroughShaderBytecodeSize;
};


