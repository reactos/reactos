// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*=========================================================================*\



    Module Name: MIL

\*=========================================================================*/

#pragma once

// Future Consideration:   Remove the CMILShader classes
//
// These CMILShader classes are left over from the old ID3DXEffect interface.
// With that interface removed, we should be able to eliminate these classes
// entirely and pass the resource material classes directly down.  This has
// implications to our immediate mode milcoretest api though, so these changes
// should be made after our API decisions are made.
// 

enum ShaderTypes
{
    ShaderDiffuse,
    ShaderEmissive,
    ShaderSpecular,

    SHADERTYPES_FORCE_DWORD = MIL_FORCE_DWORD
};

/*=========================================================================*\
    CMILShader - MIL Shader Class
\*=========================================================================*/

class CMILShader :
    public CMILObject
{
public:
    virtual ShaderTypes GetType() const = 0;

    virtual HRESULT EnsureBrushRealizations(
        UINT uRealizationCacheIndex,
        DisplayId realizationDestination,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __in_ecount(1) const CContextState *pContextState,
        __inout_ecount(1) CIntermediateRTCreator *pRTCreator
        ) = 0;

    virtual void FreeBrushRealizations() = 0;

    virtual void RestoreMetaIntermediates() = 0;

protected:
    CMILShader(
        __in_ecount_opt(1) CMILFactory *pFactory
        )
        : CMILObject(pFactory)
    {}

    virtual ~CMILShader() {};
};

/*=========================================================================*\
    CMILShaderBrush - MIL Brush Shader Class
\*=========================================================================*/

MtExtern(CMILShaderBrush);

class CMILShaderBrush :
    public CMILShader
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILShaderBrush));

    STDMETHOD(GetSurfaceSource)(
        __deref_out_ecount(1) CBrushRealizer **ppSurfaceSource
        );

    void FreeBrushRealizations() override;
    HRESULT EnsureBrushRealizations(
        UINT uRealizationCacheIndex,
        DisplayId realizationDestination,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __in_ecount(1) const CContextState *pContextState,
        __inout_ecount(1) CIntermediateRTCreator *pRTCreator
        ) override;
    void RestoreMetaIntermediates() override;

protected:
    CMILShaderBrush(
        __in_ecount_opt(1) CMILFactory *pFactory
        );

    virtual ~CMILShaderBrush();

    STDMETHOD(SetSurfaceSource)(
        __in_ecount_opt(1) CBrushRealizer *pSurfaceSource
        );

    CBrushRealizer *m_pSurfaceSource;
};

/*=========================================================================*\
    CMILShaderDiffuse - MIL Diffuse Shader Class
\*=========================================================================*/

MtExtern(CMILShaderDiffuse);

class CMILShaderDiffuse :
    public CMILShaderBrush,
    public IMILShaderDiffuse
{
public:

    static HRESULT Create(
        __in_ecount_opt(1) CMILFactory *pFactory,
        __in_ecount_opt(1) CBrushRealizer *pSurfaceBrush,
        __deref_out_ecount(1) CMILShaderDiffuse **ppDiffuseShader
        );

protected:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILShaderDiffuse));

    CMILShaderDiffuse(__in_ecount_opt(1) CMILFactory *pFactory);

public:

    DECLARE_MIL_OBJECT

    // CMILShader Methods

    ShaderTypes GetType() const override
    {
        return ShaderDiffuse;
    }

    // IMILShader Methods

    STDMETHODIMP_(CMILShader *) GetClass() override { return this; }
};

/*=========================================================================*\
    CMILShaderSpecular - MIL Specular Shader Class
\*=========================================================================*/

MtExtern(CMILShaderSpecular);

class CMILShaderSpecular :
    public CMILShaderBrush,
    public IMILShaderSpecular
{
public:

    static HRESULT Create(
        __in_ecount_opt(1) CMILFactory *pFactory,
        __in_ecount_opt(1) CBrushRealizer *pSurfaceSource,
        double rSpecularPower,
        __deref_out_ecount(1) CMILShaderSpecular **ppSpecularShader
        );

protected:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILShaderSpecular));

    CMILShaderSpecular(
        __in_ecount_opt(1) CMILFactory *pFactory
        );

public:

    DECLARE_MIL_OBJECT

    // CMILShader Methods

    ShaderTypes GetType() const override
    {
        return ShaderSpecular;
    }

    // IMILShader Methods

    STDMETHODIMP_(CMILShader *) GetClass() override { return this; }

    HRESULT SetSpecularPower(
        float flSpecularPower
        );

private:
    float m_flSpecularPower;
};

/*=========================================================================*\
    CMILShaderEmissive - MIL Emissive Shader Class
\*=========================================================================*/

MtExtern(CMILShaderEmissive);

class CMILShaderEmissive :
    public CMILShaderBrush,
    public IMILShaderEmissive
{
public:

    static HRESULT Create(
        __in_ecount_opt(1) CMILFactory *pFactory,
        __in_ecount_opt(1) CBrushRealizer *pSurfaceSource,
        __deref_out_ecount(1) CMILShaderEmissive **ppEmissiveShader
        );

protected:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILShaderEmissive));

    CMILShaderEmissive(
        __in_ecount_opt(1) CMILFactory *pFactory
        );

public:

    DECLARE_MIL_OBJECT

    // CMILShader Methods

    ShaderTypes GetType() const override
    {
        return ShaderEmissive;
    }

    // IMILShader Methods

    STDMETHODIMP_(CMILShader *) GetClass() override { return this; }
};



