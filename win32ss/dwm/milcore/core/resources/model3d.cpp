// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(Model3DResource, MILRender, "Model3D Resource");
MtDefine(CMilModel3DDuce, Model3DResource, "CMilModel3DDuce");

HRESULT CMilModel3DDuce::PreRender(
    __in_ecount(1) CPrerenderWalker *pPrerenderer,
    __in_ecount(1) CMILMatrix *pTransform
    )
{
    //
    // This method is optional for derived classes. The default
    // implementation is empty.
    //

    return S_OK;
}

HRESULT CMilModel3DDuce::Render(
    __in_ecount(1) CModelRenderWalker *pRenderer
    )
{
    //
    // This method is optional for derived classes. The default
    // implementation is empty.
    //

    return S_OK;
}

void CMilModel3DDuce::PostRender(__in_ecount(1) CModelRenderWalker *pRenderer)
{
    //
    // This method is optional for derived classes. The default
    // implementation is empty.
    //
}

HRESULT CMilModel3DDuce::GetDepthSpan(
    __in_ecount(1) CMILMatrix *pTransform,
    __inout float &zmin,
    __inout float &zmax
    )
{
    //
    // This method is optional for derived classes. The default
    // implementation is empty.
    //

    return S_OK;
}

