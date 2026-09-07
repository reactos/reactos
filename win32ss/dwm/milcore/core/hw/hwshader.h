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
//      Contains CHwShader declaration and definition
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Structure:
//      MILShaderData
//
//  Synopsis:
//      Data associated with Each Shader
//

struct MILShaderData
{
    int iNumPasses;
};

//+-----------------------------------------------------------------------------
//
//  Structure:
//      MILShaderPassData
//
//  Synopsis:
//      Data required for each pass in an effect
//

struct MILShaderPassData
{
    BOOL fPassUsesLighting;
    BOOL fLightingRequiredForPass;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwShader
//
//  Synopsis:
//      Provides base interface and implementation for HW shader implementations
//
//------------------------------------------------------------------------------

class CHwShader : public IMILRefCount
{
public:
    
    HRESULT DrawMesh3D(
        __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
        __in_ecount(1) CHwSurfaceRenderTarget *pHwTargetSurface,
        __inout_ecount(1) CMILMesh3D *pMesh3D,
        __in_ecount(1) const CMilRectL &rcRenderBoundsDeviceSpace,
        __inout_ecount(1) CContextState *pContextState,
        bool fZBufferEnabled
        );

    HRESULT DrawHwVertexBuffer(
        __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
        __in_ecount(1) CHwSurfaceRenderTarget *pHwTargetSurface,
        __inout_ecount(1) IGeometryGenerator *pIGeometryGenerator,
        __inout_ecount(1) CBufferDispenser *pBufferDispenser,
        __in_ecount(1) const CMilRectL &rcRenderBoundsDeviceSpace,
        BOOL fUse3DTransforms,
        bool fZBufferEnabled
        );

    virtual HRESULT CreateCompatibleVertexBufferBuilder(
        MilVertexFormat mvfGeometryOutput,
        MilVertexFormatAttribute mvfGeometryAALocation,
        __inout_ecount(1) CBufferDispenser *pBufferDispenser,
        __deref_out_ecount(1) CHwVertexBuffer::Builder **ppBufferBuilder
        ) PURE;


    BOOL ValidTechniqueFound();

    __range(1,INT_MAX) UINT GetNumPasses() const;

    // DO NOT | these flags together. There is only one vertex
    // color so trying to return "LV_Diffuse | LV_Specular" doesn't
    // make sense
    enum LightingValues
    {
        LV_None     = 0,
        LV_Diffuse  = 1, // includes Ambient since DiffuseMaterial includes Ambient
        LV_Specular = 2,
        LV_Emissive = 3
    };

private:

    HRESULT ShaderDrawMesh3D(
        __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
        __in_ecount(1) const CMILMesh3D *pMesh3D,
        __inout_ecount(1) CContextState *pContextState
        );

    HRESULT FixedFunctionDrawMesh3D(
        __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
        __inout_ecount(1) CMILMesh3D *pMesh3D,
        __inout_ecount(1) CContextState *pContextState
        );

protected:
    CHwShader(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        );


    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      CHwShader::Begin
    //
    //  Synopsis:
    //      Begins the shader by setting up the pass and lighting counters and
    //      calling begin on the inherited shader.
    //
    //--------------------------------------------------------------------------
    virtual HRESULT Begin(
        __in_ecount(1) CHwSurfaceRenderTarget *pHwTargetSurface,
        __in_ecount(1) const CMilRectL &rcRenderBoundsDeviceSpace,
        bool fZBufferEnabled
        ) PURE;

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      CHwShader::Finish
    //
    //  Synopsis:
    //      Calls the inherited FinishVirtual which should end the shader and do
    //      any cleanup necessary.
    //
    //--------------------------------------------------------------------------
    virtual HRESULT Finish() PURE;

    virtual HRESULT SetupPassVirtual(
        __in_ecount_opt(1) IGeometryGenerator *pGeometryGenerator,
        __inout_ecount_opt(1) CHwPipeline *pHwPipeline,
        __range(0,INT_MAX) UINT uPassNum
        ) PURE;

    virtual LightingValues GetRequiredLightingValues() const PURE;

protected:
    CD3DDeviceLevel1 *m_pDevice;

    MILShaderData m_oShaderData;
    DynArray<MILShaderPassData> m_rgPassData;
};



