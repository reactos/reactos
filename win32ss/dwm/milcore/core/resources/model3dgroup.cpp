// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(Model3DGroupResource, MILRender, "Model3DGroup Resource");
MtDefine(CMilModel3DGroupDuce, Model3DGroupResource, "CMilModel3DGroupDuce");

CMilModel3DGroupDuce::~CMilModel3DGroupDuce()
{
    UnRegisterNotifiers();
}

__out_ecount_opt(1) CMilTransform3DDuce *CMilModel3DGroupDuce::GetTransform()
{
    return m_data.m_pTransform;
}

HRESULT CMilModel3DGroupDuce::Render(__in_ecount(1) CModelRenderWalker *pRenderer)
{
    HRESULT hr = S_OK;

    if (m_data.m_pTransform)
    {
        CMILMatrix matrix;

        IFC(m_data.m_pTransform->GetRealization(&matrix));
        IFC(pRenderer->PushTransform(&matrix));
    }

Cleanup:
    RRETURN(hr);
}

void CMilModel3DGroupDuce::PostRender(__in_ecount(1) CModelRenderWalker *pRenderer)
{
    if (m_data.m_pTransform)
    {
        pRenderer->PopTransform();
    }
}

