// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Abstract:
//      Routines to write generated shader's binary resource.
//
//------------------------------------------------------------------------

#include "precomp.h"

//+----------------------------------------------------------------------------
//
//  Member:    CResourceGen::CResourceGen
//
//  Synopsis:  ctor
//
//-----------------------------------------------------------------------------
CResourceGen::CResourceGen()
{
    m_pFileRc = NULL;
    m_pFileHpp = NULL;
    m_pDevice = NULL;
    m_cbTotal = 0;
}

//+----------------------------------------------------------------------------
//
//  Member:    CResourceGen::~CResourceGen
//
//  Synopsis:  dtor
//
//-----------------------------------------------------------------------------
CResourceGen::~CResourceGen()
{
    if (m_pFileRc)
    {
        fprintf(m_pFileRc, "// Total data size = %d (0x%08x) bytes.\n", m_cbTotal, m_cbTotal);
        fclose(m_pFileRc);
    }
    if (m_pFileHpp)
    {
        fclose(m_pFileHpp);
    }
    ReleaseInterface(m_pDevice);
}


//+----------------------------------------------------------------------------
//
//  Member:    CResourceGen::Initialize
//
//  Synopsis:  Create an instance of CResourceGen with text file opened for writing.
//
//-----------------------------------------------------------------------------
HRESULT
CResourceGen::Initialize(
    __in char const *pszFileNameRc,
    __in char const *pszFileNameHpp,
    UINT uEnumStart
    )
{
    HRESULT hr = S_OK;

    m_pFileRc = fopen(pszFileNameRc, "wt");
    if (!m_pFileRc)
    {
        printf("Can't open %s for writing\n", pszFileNameRc);
        IFC(E_FAIL);
    }

    m_pFileHpp = fopen(pszFileNameHpp, "wt");
    if (!m_pFileHpp)
    {
        printf("Can't open %s for writing\n", pszFileNameHpp);
        IFC(E_FAIL);
    }

    IFC(CFakeDevice::Create(&m_pDevice));

    fprintf(m_pFileRc, CCodeGen::sc_szTitle);
    fprintf(m_pFileHpp, CCodeGen::sc_szTitle);

    m_uEnumCurrent = uEnumStart;

Cleanup:
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:    CResourceGen::CompileEffect
//
//  Synopsis:  Create an instance of CResourceGen with text file opened for writing.
//
//-----------------------------------------------------------------------------
HRESULT
CResourceGen::CompileEffect(
    __in WCHAR const *pszEffectFileName,
    __in char const *pszEffectName
    )
{
    HRESULT hr = S_OK;

    m_pszEffectName = pszEffectName;

    ID3DXBuffer *pCompilationErrors = NULL;

    IFC(D3DXCreateEffectFromFile(
        m_pDevice,
        pszEffectFileName,
        NULL,   //CONST D3DXMACRO* pDefines,
        NULL,   //LPD3DXINCLUDE pInclude,
        0,      //DWORD Flags: D3DXSHADER_DEBUG/SKIPVALIDATION/SKIPOPTIMIZATION 
        NULL,   //LPD3DXEFFECTPOOL pPool,
        &m_pEffect,
        &pCompilationErrors
        ));

    IFC(WriteEffect());

Cleanup:
    ReleaseInterface(m_pEffect);
    if (pCompilationErrors != NULL)
    {
        // Output compiler errors

        char *szErrors = (char *)pCompilationErrors->GetBufferPointer();
        printf("%s", szErrors);

        pCompilationErrors->Release();
    }
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:    CResourceGen::WriteEffect
//
//  Synopsis:
//      Traverse given ID3DXEffect, pointed by m_pEffect.
//      Generate C++ code for its components.
//
//-----------------------------------------------------------------------------
HRESULT
CResourceGen::WriteEffect()
{

    HRESULT hr = S_OK;
    D3DXEFFECT_DESC descEffect;

    IFC(m_pEffect->GetDesc(&descEffect));

    for (UINT i = 0; i < descEffect.Techniques; i++)
    {
        m_hTechnique = m_pEffect->GetTechnique(i);
        IFH(m_hTechnique);
        IFC(WriteTechnique());
    }

Cleanup:
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:    CResourceGen::WriteTechnique
//
//  Synopsis:
//      Traverse current technique, pointed by m_hTechnique.
//      Generate C++ code for its components.
//
//-----------------------------------------------------------------------------
HRESULT
CResourceGen::WriteTechnique()
{
    HRESULT hr = S_OK;

    IFC(m_pEffect->GetTechniqueDesc(m_hTechnique, &m_descTechnique));

    for (UINT i = 0; i < m_descTechnique.Passes; i++)
    {
        m_hPass = m_pEffect->GetPass(m_hTechnique, i);
        IFH(m_hPass);
        IFC(WritePass());
    }

Cleanup:
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:    CResourceGen::WritePass
//
//  Synopsis:
//      Traverse current pass, pointed by m_hPass.
//      Generate C++ code for its components.
//
//-----------------------------------------------------------------------------
HRESULT
CResourceGen::WritePass()
{
    HRESULT hr = S_OK;

    IFC(m_pEffect->GetPassDesc(m_hPass, &m_descPass));

    if (m_descPass.pPixelShaderFunction != NULL)
    {
        IFC(WritePixelShader());
    }

    if (m_descPass.pVertexShaderFunction != NULL)
    {
        IFC(WriteVertexShader());
    }

Cleanup:
    return hr;
}    

//+----------------------------------------------------------------------------
//
//  Member:    CResourceGen::WritePixelShader
//
//  Synopsis:
//      Generate code for pixel shader.
//
//-----------------------------------------------------------------------------
HRESULT
CResourceGen::WritePixelShader()
{
    HRESULT hr = S_OK;

    const DWORD* pFunction = m_descPass.pPixelShaderFunction;

    // form shader data array definition header, like
    // g_PixelShader_Foo_Tech_Pass RCDATA
    fprintf(
        m_pFileRc,
        "g_PixelShader_%s_%s_%s RCDATA\n",
        m_pszEffectName,
        m_descTechnique.Name,
        m_descPass.Name
        );

    fprintf(
        m_pFileHpp,
        "#define g_PixelShader_%s_%s_%s %d\n",
        m_pszEffectName,
        m_descTechnique.Name,
        m_descPass.Name,
        m_uEnumCurrent++
        );

    WriteDwordArray(pFunction);

//Cleanup:
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:    CResourceGen::WriteVertexShader
//
//  Synopsis:
//      Generate code for vertex shader.
//
//-----------------------------------------------------------------------------
HRESULT
CResourceGen::WriteVertexShader()
{
    HRESULT hr = S_OK;

    const DWORD* pFunction = m_descPass.pVertexShaderFunction;

    // form shader data array definition header, like
    //  g_VertexShader_Foo_Tech_Pass RCDATA
    fprintf(
        m_pFileRc,
        "g_VertexShader_%s_%s_%s RCDATA\n",
        m_pszEffectName,
        m_descTechnique.Name,
        m_descPass.Name
        );

    fprintf(
        m_pFileHpp,
        "#define g_VertexShader_%s_%s_%s %d\n",
        m_pszEffectName,
        m_descTechnique.Name,
        m_descPass.Name,
        m_uEnumCurrent++
        );

    WriteDwordArray(pFunction);

//Cleanup:
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:    CResourceGen::WriteDwordArray
//
//  Synopsis:
//      Helper for WritePixelShader() and WriteVertexShader().
//      Generate array data C++ code.
//
//-----------------------------------------------------------------------------
void
CResourceGen::WriteDwordArray(
    __in DWORD const *pFunction
    )
{
    // open array data
    fprintf(m_pFileRc, "{\n");

    // write data array, uRowSize DWORDs per line
    static const UINT uRowSize = 6;

    UINT cbSize = D3DXGetShaderSize(pFunction);

    for (UINT i = 0, n = cbSize/sizeof(DWORD); i < n; i++)
    {
        UINT j = i % uRowSize;
        if (j == 0)
        {
            fprintf(m_pFileRc, "    ");
        }
        fprintf(m_pFileRc, "0x%08xL", pFunction[i]);
        if (i+1 == n)
        {
            fprintf(m_pFileRc, "\n");
        }
        else
        {
            fprintf(m_pFileRc, ",");
            if (j+1 == uRowSize)
            {
                fprintf(m_pFileRc, "\n");
            }
            else
            {
                fprintf(m_pFileRc, " ");
            }
        }
    }

    // close array data
    fprintf(m_pFileRc, "};\n\n");

    // update total size of all arrays
    m_cbTotal += cbSize;
}

