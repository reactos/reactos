// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Abstract:
//      Routines to write generated C++ code.
//
//------------------------------------------------------------------------

#pragma once

//--------------------------------------------------------------------------------
//
// Class:   CCodeGen
//
// Synopsis:
//      Converts HLSL files to C++.
//
//  Usage pattern:
//
//      // Create the instance
//      CCodeGen codeGen;
//
//      // Initialize, giving output file names
//      IFC(codeGen.Initialize("myShaders.cpp", "myShader.h"));
//
//      // Compile as many files as needed
//      IFC(codeGen.CompileEffect(L"foo.fx", "Foo"));
//      IFC(codeGen.CompileEffect(L"bar.fx", "Zoo"));
//
//      // Destruct the instance
//--------------------------------------------------------------------------------

class CCodeGen
{
public:
    CCodeGen();
    HRESULT Initialize(
        __in char const *pszFileNameCpp,
        __in char const *pszFileNameHpp
        );
    HRESULT CompileEffect(
        __in WCHAR const *pszEffectFileName,
        __in char const *pszEffectName
        );
    ~CCodeGen();

    static char const sc_szTitle[];

private:
    HRESULT WriteEffect();
    HRESULT WriteTechnique();
    HRESULT WritePass();
    HRESULT WritePixelShader();
    HRESULT WriteVertexShader();
    void WriteDwordArray(
        __in DWORD const *pFunction
        );

private:
    IDirect3DDevice9 *m_pDevice;
    FILE *m_pFileCpp;
    FILE *m_pFileHpp;
    ID3DXEffect *m_pEffect;
    char const *m_pszEffectName;

    D3DXHANDLE m_hTechnique;
    D3DXTECHNIQUE_DESC m_descTechnique;

    D3DXHANDLE m_hPass;
    D3DXPASS_DESC m_descPass;

    UINT m_cbTotal;
};

