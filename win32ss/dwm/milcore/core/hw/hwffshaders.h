// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_lighting
//      $Keywords:
//
//  $Description:
//      Contains declarations for all fixed function shaders
//          - CHwFFShader (base class)
//          - CHwDiffuseShader
//          - CHwSpecularShader
//          - CHwEmissiveShader
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CHwFFShader);
MtExtern(CHwDiffuseShader);
MtExtern(CHwSpecularShader);
MtExtern(CHwEmissiveShader);

class CHwBrush;

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwFFShader
//
//  Synopsis:
//      Base class for most of the FF shaders
//

class CHwFFShader :
    public CHwShader,
    public CMILRefCountBase
{
protected:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwFFShader));

    CHwFFShader(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        );

    virtual ~CHwFFShader();

    HRESULT Init(
        __in_ecount(1) CHwBrush *pHwBrush,
        __inout_ecount_opt(1) IMILEffectList *pEffectList,
        __in_ecount(1) CHwBrushContext const &oEffectContext
        );

protected:
    CHwBrush *m_pSurfaceBrush;
    IMILEffectList *m_pEffectList;
    CHwBrushContext const *m_pEffectContextNoRef;

public:
    virtual override HRESULT CreateCompatibleVertexBufferBuilder(
        MilVertexFormat mvfGeometryOutput,
        MilVertexFormatAttribute mvfGeometryAALocation,
        __inout_ecount(1) CBufferDispenser *pBufferDispenser,
        __deref_out_ecount(1) CHwVertexBuffer::Builder **ppBufferBuilder
        );

protected:
    virtual override HRESULT Finish();
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwDiffuseShader
//
//  Synopsis:
//      Diffuse FF shader
//

class CHwDiffuseShader :
    public CHwFFShader
{
private:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwDiffuseShader));

    CHwDiffuseShader(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        ) : CHwFFShader(pDevice)
    {};

public:
    DEFINE_REF_COUNT_BASE;

    static __checkReturn HRESULT Create(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_ecount(1) CHwBrush         *pHwBrush,
        __inout_ecount_opt(1) IMILEffectList    *pEffectList,
        __in_ecount(1) CHwBrushContext const    &hwBrushContext,
        __deref_out_ecount(1) CHwDiffuseShader **ppDiffuseShader
        );

protected:
    override HRESULT SetupPassVirtual(
        __in_ecount(1) IGeometryGenerator *pGeometryGenerator,
        __inout_ecount(1) CHwPipeline *pHwPipeline,
        __range(0,INT_MAX) UINT uPassNum
        );

    override HRESULT Begin(
        __in_ecount(1) CHwSurfaceRenderTarget *pHwTargetSurface,
        __in_ecount(1) const CMilRectL &rcRenderingBounds,
        bool fZBufferEnabled
        );

    override LightingValues GetRequiredLightingValues() const;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwSpecularShader
//
//  Synopsis:
//      Diffuse FF shader
//

class CHwSpecularShader :
    public CHwFFShader
{
private:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwSpecularShader));

    CHwSpecularShader(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        ) : CHwFFShader(pDevice)
    {};

public:
    DEFINE_REF_COUNT_BASE;

    static __checkReturn HRESULT Create(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_ecount(1) CHwBrush         *pHwBrush,
        __inout_ecount_opt(1) IMILEffectList    *pEffectList,
        __in_ecount(1) CHwBrushContext const    &hwBrushContext,
        __deref_out_ecount(1) CHwSpecularShader **ppSpecularShader
        );

protected:
    override HRESULT SetupPassVirtual(
        __in_ecount(1) IGeometryGenerator *pGeometryGenerator,
        __inout_ecount(1) CHwPipeline *pHwPipeline,
        __range(0,INT_MAX) UINT uPassNum
        );

    override HRESULT Begin(
        __in_ecount(1) CHwSurfaceRenderTarget *pHwTargetSurface,
        __in_ecount(1) const CMilRectL &rcRenderingBounds,
        bool fZBufferEnabled
        );

    override LightingValues GetRequiredLightingValues() const;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwEmissiveShader
//
//  Synopsis:
//      Emissive FF shader
//
//------------------------------------------------------------------------------
class CHwEmissiveShader :
    public CHwFFShader
{
private:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwEmissiveShader));

    CHwEmissiveShader(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        ) : CHwFFShader(pDevice)
    {};

public:
    DEFINE_REF_COUNT_BASE;

    static __checkReturn HRESULT Create(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_ecount(1) CHwBrush         *pHwBrush,
        __inout_ecount_opt(1) IMILEffectList    *pEffectList,
        __in_ecount(1) CHwBrushContext const    &hwBrushContext,
        __deref_out_ecount(1) CHwEmissiveShader **ppEmissiveShader
        );

protected:
    override HRESULT SetupPassVirtual(
        __in_ecount(1) IGeometryGenerator *pGeometryGenerator,
        __inout_ecount(1) CHwPipeline *pHwPipeline,
        __range(0,INT_MAX) UINT uPassNum
        );

    override HRESULT Begin(
        __in_ecount(1) CHwSurfaceRenderTarget *pHwTargetSurface,
        __in_ecount(1) const CMilRectL &rcRenderingBounds,
        bool fZBufferEnabled
        );

    override LightingValues GetRequiredLightingValues() const;
};
    




