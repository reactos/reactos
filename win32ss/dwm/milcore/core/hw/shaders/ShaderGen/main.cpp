// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Abstract:
//      Standalone executable to generate shader sources,
//      in a form of C++ arrays, from HLSL files.
//
//------------------------------------------------------------------------

#include "precomp.h"

class a {};

int __cdecl main()
{
    HRESULT hr = S_OK;

    // Variant 1: generate C++ codes
    //CCodeGen codeGen;
    //IFC(codeGen.Initialize("..\\Shaders.cpp", "..\\Shaders.h"));

    // Variant 2: generate resource codes
    CResourceGen codeGen;
    IFC(codeGen.Initialize("..\\Shaders.rc", "..\\Shaders.h", 100));

    IFC(codeGen.CompileEffect(L"hlslTextShaders11A.fx", "Text11A"));
    IFC(codeGen.CompileEffect(L"hlslTextShaders11L.fx", "Text11L"));
    IFC(codeGen.CompileEffect(L"hlslTextShaders20A.fx", "Text20A"));
    IFC(codeGen.CompileEffect(L"hlslTextShaders20L.fx", "Text20L"));


Cleanup:
    if (SUCCEEDED(hr))
    {
        printf("Generation succeeded\n");
        return 0;
    }
    else
    {
        printf("Generation failed\n");
        return 1;
    }
}

