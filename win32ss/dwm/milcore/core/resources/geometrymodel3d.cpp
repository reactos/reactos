// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//------------------------------------------------------------------------

#include "precomp.hpp"
using namespace dxlayer;

MtDefine(GeometryModel3DResource, MILRender, "GeometryModel3D Resource");
MtDefine(CMilGeometryModel3DDuce, GeometryModel3DResource, "CMilGeometryModel3DDuce");

CMilGeometryModel3DDuce::~CMilGeometryModel3DDuce()
{
    UnRegisterNotifiers();
}

__out_ecount_opt(1) CMilTransform3DDuce *CMilGeometryModel3DDuce::GetTransform()
{
    return m_data.m_pTransform;
}

HRESULT CMilGeometryModel3DDuce::Render(__in_ecount(1) CModelRenderWalker *pRenderer)
{
    RRETURN(pRenderer->RenderGeometryModel3D(this));
}

HRESULT CMilGeometryModel3DDuce::GetDepthSpan(
    __in_ecount(1) CMILMatrix *pTransform,
    __inout float &zmin,
    __inout float &zmax
    )
{
    HRESULT hr = S_OK;
    CMILMesh3D *pMesh = NULL;
    // The following line could be written simply as:
    //
    //    vector4 rgBoxVertices[8]; 
    //
    // There is a compiler bug in VS2013 that forces us to use the 
    // array initializer form seen here.
    vector4 rgBoxVertices[8]{ {}, {}, {}, {}, {}, {}, {}, {} };
    CMilPointAndSize3F boxBounds3D;

    std::vector<vector4> vertices;

    // Early exit with S_OK if the primitive has no mesh
    if (!m_data.m_pGeometry)
    {
        goto Cleanup;
    }

    //
    //  Retrieve the Mesh3D from the geometry
    //

    IFC(m_data.m_pGeometry->GetRealization(&pMesh));

    // Early exit with S_OK if the primitive has a mesh, but it is empty (i.e., has no vertices)
    if (!pMesh)
    {
        goto Cleanup;
    }

    IFC(pMesh->GetBounds(&boxBounds3D));
    boxBounds3D.ToVector4Array(rgBoxVertices);

    vertices 
        = math_extensions::transform_array(
            sizeof(rgBoxVertices[0]),                                                   // out_stride
            std::vector<vector4>(std::begin(rgBoxVertices), std::end(rgBoxVertices)),   // in
            sizeof(rgBoxVertices[0]),                                                   // in_stride
            *pTransform,                                                                // transformation
            ARRAY_SIZE(rgBoxVertices));
    std::copy(vertices.begin(), vertices.end(), rgBoxVertices);

    for (int i = 0; i < ARRAY_SIZE(rgBoxVertices); ++i)
    {
        float z = -rgBoxVertices[i][2]/rgBoxVertices[i][3];

        if (zmax < z)
        {
            zmax = z;
        }

        if (zmin > z)
        {
            zmin = z;
        }
    }

Cleanup:
    ReleaseInterfaceNoNULL(pMesh);
    RRETURN(hr);
}

