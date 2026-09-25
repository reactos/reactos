// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*=========================================================================*\



    Module Name: MIL

\*=========================================================================*/

#include "precomp.hpp"

MtDefine(CMILShaderBrush,    MILApi, "CMILShaderBrush");
MtDefine(CMILShaderDiffuse,  MILApi, "CMILShaderDiffuse");
MtDefine(CMILShaderEmissive, MILApi, "CMILShaderEmissive");
MtDefine(CMILShaderSpecular, MILApi, "CMILShaderSpecular");

/*=========================================================================*\
    CMILShader - MIL Shader Object
\*=========================================================================*/

/*=========================================================================*\
    CMILShaderBrush - MIL Brush Shader Object

    Contains the common code for Shaders that require a brush

\*=========================================================================*/
CMILShaderBrush::CMILShaderBrush(
    __in_ecount_opt(1) CMILFactory *pFactory
    )
    : CMILShader(pFactory)
{
    m_pSurfaceSource = NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CMILShaderBrush::~CMILShaderBrush
//
//  Synopsis:
//      Releases the brush object.
//
//-----------------------------------------------------------------------------
CMILShaderBrush::~CMILShaderBrush()
{
    ReleaseInterfaceNoNULL(m_pSurfaceSource);
}

//+----------------------------------------------------------------------------
//
//  Function:  CMILShaderBrush::GetSurfaceSource
//
//  Synopsis:  Retrieves the Surface Brush Realizer
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CMILShaderBrush::GetSurfaceSource(
    __deref_out_ecount(1) CBrushRealizer **ppSurfaceSource
    )
{
    HRESULT hr = S_OK;

    API_ENTRY_NOFPU(CMILShaderBrush::GetSurfaceSource);

    if (!ppSurfaceSource)
    {
        IFC(E_INVALIDARG);
    }

    *ppSurfaceSource = m_pSurfaceSource;

    if(m_pSurfaceSource)
    {
        m_pSurfaceSource->AddRef();
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CMILShaderBrush::FreeBrushRealizations
//
//  Synopsis:
//      Frees the realization resources from the single brush in the class.
//
//-----------------------------------------------------------------------------

void
CMILShaderBrush::FreeBrushRealizations()
{
    m_pSurfaceSource->FreeRealizationResources();
}

//+----------------------------------------------------------------------------
//
//  Function:  CMILShaderBrush::SetSurfaceSource
//
//  Synopsis:  Sets the Surface Brush Realizer
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CMILShaderBrush::SetSurfaceSource(
    __in_ecount_opt(1) CBrushRealizer *pSurfaceSource
    )
{
    HRESULT hr = S_OK;

    API_ENTRY_NOFPU(CMILShaderBrush::SetSurfaceSource);

    if (!pSurfaceSource)
    {
        IFC(E_INVALIDARG);
    }

    ReplaceInterface(m_pSurfaceSource,pSurfaceSource);

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CMILShaderBrush::EnsureBrushRealizations
//
//  Synopsis:  Ensures realizations for the single brush realizer in this
//             shader.
//

HRESULT 
CMILShaderBrush::EnsureBrushRealizations(
    UINT uRealizationCacheIndex,
    DisplayId realizationDestination,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __in_ecount(1) const CContextState *pContextState,
    __inout_ecount(1) CIntermediateRTCreator *pRTCreator
    )
{
    HRESULT hr = S_OK;

    IFC(m_pSurfaceSource->EnsureRealization(
        uRealizationCacheIndex,
        realizationDestination,
        pBrushContext,
        pContextState,
        pRTCreator
        ));

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CMILShaderBrush::RestoreMetaIntermediates
//
//  Synopsis:
//      Restores meta intermediates within the single realized brush in this
//      shader.
//
//      This method should be called in the meta render target during cleanup,
//      after the drawing operations are complete.
//

void 
CMILShaderBrush::RestoreMetaIntermediates()
{
    m_pSurfaceSource->RestoreMetaIntermediates();
}

/*=========================================================================*\
    CMILShaderDiffuse - MIL Diffuse Shader Object
\*=========================================================================*/

//+----------------------------------------------------------------------------
//
//  Function:  CMILShaderDiffuse::ctor
//
//  Synopsis:  Initializes Diffuse Shader
//
//-----------------------------------------------------------------------------
CMILShaderDiffuse::CMILShaderDiffuse(
    __in_ecount_opt(1) CMILFactory *pFactory
    )
    : CMILShaderBrush(pFactory)
{
}

//+----------------------------------------------------------------------------
//
//  Function:  CMILShaderDiffuse::Create
//
//  Synopsis:  Create Diffuse Shader
//
//-----------------------------------------------------------------------------
HRESULT
CMILShaderDiffuse::Create(
    __in_ecount_opt(1) CMILFactory *pFactory,
    __in_ecount_opt(1) CBrushRealizer *pSurfaceBrush,
    __deref_out_ecount(1) CMILShaderDiffuse **ppDiffuseShader
    )
{
    HRESULT hr = S_OK;
    CMILShaderDiffuse *pNewDiffuseShader = NULL;

    //
    // We shouldn't have to check for invalid args on pSurfaceBrush because that
    // should be done by the factory create call.
    //
    *ppDiffuseShader = NULL;

    pNewDiffuseShader = new CMILShaderDiffuse(pFactory);
    IFCOOM(pNewDiffuseShader);

    pNewDiffuseShader->AddRef();

    IFC(pNewDiffuseShader->SetSurfaceSource(pSurfaceBrush));

    *ppDiffuseShader = pNewDiffuseShader; // steal ref
    pNewDiffuseShader = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pNewDiffuseShader);

    RRETURN(hr);
}

/*=========================================================================*\
    Support methods
\*=========================================================================*/

HRESULT
CMILShaderDiffuse::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    HRESULT hr = S_OK;

    if (ppvObject != NULL)
    {
        if (riid == IID_IMILShader)
        {
            *ppvObject = static_cast<IMILShader *>(this);
        }
        else if (riid == IID_IMILShaderDiffuse)
        {
            *ppvObject = static_cast<IMILShaderDiffuse *>(this);
        }
        else
        {
            // Couldn't find interface
            hr = E_NOINTERFACE;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

/*=========================================================================*\
    CMILShaderSpecular - MIL Specular Shader Object
\*=========================================================================*/

//+----------------------------------------------------------------------------
//
//  Function:  CMILShaderSpecular::ctor
//
//  Synopsis:  Initializes the Specular Shader
//
//-----------------------------------------------------------------------------
CMILShaderSpecular::CMILShaderSpecular(
    __in_ecount_opt(1) CMILFactory *pFactory
    )
    : CMILShaderBrush(pFactory)
{
}

//+----------------------------------------------------------------------------
//
//  Function:  CMILShaderSpecular::Create
//
//  Synopsis:  Creates the Specular Shader
//
//-----------------------------------------------------------------------------
HRESULT
CMILShaderSpecular::Create(
    __in_ecount_opt(1) CMILFactory *pFactory,
    __in_ecount_opt(1) CBrushRealizer *pSurfaceBrush,
    double rSpecularPower,
    __deref_out_ecount(1) CMILShaderSpecular **ppSpecularShader
    )
{
    HRESULT hr = S_OK;
    CMILShaderSpecular *pNewSpecularShader = NULL;

    //
    // Verify inputs.  We shouldn't have to check for invalid args on pSurfaceBrush and
    // ppIGlassShader because that should be done by the factory create call.
    //
    *ppSpecularShader = NULL;

    pNewSpecularShader = new CMILShaderSpecular(pFactory);
    IFCOOM(pNewSpecularShader);

    pNewSpecularShader->AddRef();

    IFC(pNewSpecularShader->SetSurfaceSource(
        pSurfaceBrush
        ));

    IFC(pNewSpecularShader->SetSpecularPower(static_cast<float>(rSpecularPower)));

    *ppSpecularShader = pNewSpecularShader; // steal ref
    pNewSpecularShader = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pNewSpecularShader);

    RRETURN(hr);
}

/*=========================================================================*\
    IMILShaderSpecular methods
\*=========================================================================*/

//+----------------------------------------------------------------------------
//
//  Function:  CMILShaderSpecular::SetSpecularPower
//
//  Synopsis:  Sets the specular power
//
//-----------------------------------------------------------------------------
HRESULT
CMILShaderSpecular::SetSpecularPower(
    float flSpecularPower
    )
{
    HRESULT hr = S_OK;

    API_ENTRY_NOFPU(CMILShaderSpecular::SetSpecularPower);

    m_flSpecularPower = flSpecularPower;

    API_CHECK(hr);
    RRETURN(hr);
}

/*=========================================================================*\
    Support methods
\*=========================================================================*/

HRESULT
CMILShaderSpecular::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    HRESULT hr = S_OK;

    if (ppvObject != NULL)
    {
        if (riid == IID_IMILShader)
        {
            *ppvObject = static_cast<IMILShader *>(this);
        }
        else if (riid == IID_IMILShaderSpecular)
        {
            *ppvObject = static_cast<IMILShaderSpecular *>(this);
        }
        else
        {
            // Couldn't find interface
            hr = E_NOINTERFACE;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

/*=========================================================================*\
    CMILShaderEmissive - MIL Emissive Shader Object
\*=========================================================================*/

//+----------------------------------------------------------------------------
//
//  Function:  CMILShaderEmissive::ctor
//
//  Synopsis:  Initializes the Emissive Shader
//
//-----------------------------------------------------------------------------
CMILShaderEmissive::CMILShaderEmissive(
    __in_ecount_opt(1) CMILFactory *pFactory
    )
    : CMILShaderBrush(pFactory)
{
}

//+----------------------------------------------------------------------------
//
//  Function:  CMILShaderEmissive::Create
//
//  Synopsis:  Creates the Emissive Shader
//
//-----------------------------------------------------------------------------
HRESULT
CMILShaderEmissive::Create(
    __in_ecount_opt(1) CMILFactory *pFactory,
    __in_ecount_opt(1) CBrushRealizer *pSurfaceBrush,
    __deref_out_ecount(1) CMILShaderEmissive **ppEmissiveShader
    )
{
    HRESULT hr = S_OK;
    CMILShaderEmissive *pNewEmissiveShader = NULL;

    //
    // Verify inputs.  We shouldn't have to check for invalid args on pSurfaceBrush 
    // because that should be done by the factory create call.
    //
    *ppEmissiveShader = NULL;

    pNewEmissiveShader = new CMILShaderEmissive(pFactory);
    IFCOOM(pNewEmissiveShader);

    pNewEmissiveShader->AddRef();

    IFC(pNewEmissiveShader->SetSurfaceSource(pSurfaceBrush));

    *ppEmissiveShader = pNewEmissiveShader; // steal ref
    pNewEmissiveShader = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pNewEmissiveShader);

    RRETURN(hr);
}

/*=========================================================================*\
    Support methods
\*=========================================================================*/

HRESULT
CMILShaderEmissive::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    HRESULT hr = S_OK;

    if (ppvObject != NULL)
    {
        if (riid == IID_IMILShader)
        {
            *ppvObject = static_cast<IMILShader *>(this);
        }
        else if (riid == IID_IMILShaderEmissive)
        {
            *ppvObject = static_cast<IMILShaderEmissive *>(this);
        }
        else
        {
            // Couldn't find interface
            hr = E_NOINTERFACE;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}





