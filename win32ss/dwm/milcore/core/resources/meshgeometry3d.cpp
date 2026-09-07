// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_mesh
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"
using namespace dxlayer;

MtDefine(MeshGeometry3DResource, MILRender, "MeshGeometry3D Resource");
MtDefine(CMilMeshGeometry3DDuce, MeshGeometry3DResource, "CMilMeshGeometry3DDuce");
MtDefine(MMeshGeometryTextureCoordinateList, CMilMeshGeometry3DDuce, "MMeshGeometryTextureCoordinateList");

CMilMeshGeometry3DDuce::~CMilMeshGeometry3DDuce()
{
    UnRegisterNotifiers();
    ClearRealization();
}

void CMilMeshGeometry3DDuce::ClearRealization()
{
    if (m_realization)
    {
        ReleaseInterface(m_realization);
        m_realization = NULL;
    }
}

HRESULT CMilMeshGeometry3DDuce::GetRealization(
    __deref_out_ecount_opt(1) CMILMesh3D **ppRealization
    )
{
    HRESULT hr = S_OK;

    *ppRealization = NULL;

    if (!m_realization)
    {
        // Realize shouild return addref'd data
        // That ref belongs to our cached realization.
        IFC(Realize(&m_realization));
    }

    *ppRealization = m_realization;

    if (m_realization)
    {
        m_realization->AddRef();
    }

Cleanup:
    RRETURN(hr);
}

HRESULT CMilMeshGeometry3DDuce::Realize(
    __deref_out_ecount_opt(1) CMILMesh3D **ppMesh
    )
{
    HRESULT hr = S_OK;

    Assert(ppMesh);
    Assert((m_data.m_cbPositionsSize > 0) == (m_data.m_pPositionsData != NULL));
    Assert((m_data.m_cbNormalsSize > 0) == (m_data.m_pNormalsData != NULL));
    Assert((m_data.m_cbTextureCoordinatesSize > 0) == (m_data.m_pTextureCoordinatesData != NULL));
    Assert((m_data.m_cbTriangleIndicesSize > 0) == (m_data.m_pTriangleIndicesData != NULL));

    *ppMesh = NULL;
    IMILMesh3D *pIMesh = NULL;
    CMILMesh3D *pMesh = NULL;

    //
    //  Early exit with S_OK/NULL realization if mesh has no vertices.
    //  (We'll handle no VALID indices after we figure out how many valid indices we have.)
    //

    if (m_data.m_cbPositionsSize == 0)
    {
        goto Cleanup;
    }

    UINT cTriangleIndices = 0;
    UINT cVertices = 0;
    if (m_data.m_cbTriangleIndicesSize != 0)
    {
        //
        //  Determine how many triangle indices to actually use.  Let N be the number
        //  of vertices, and M be the number of user-given triangle indices.  We actually
        //  use M' indices where M' is the greatest positive integer such that
        //    1. All indices 0 .. M-1 are in [0,N-1]      and
        //    2. M % 3 = 0.
        //
        //  Note that if an index is negative, its UINT representation will be greater than
        //  GetPositionsCount() and it will be rejected.
        //

        Assert(GetPositionsCount() <= INT_MAX);

        const UINT *pTriangleIndicesData = reinterpret_cast<const UINT *>(m_data.m_pTriangleIndicesData);
        while (cTriangleIndices < GetTriangleIndicesCount() &&
               pTriangleIndicesData[cTriangleIndices] < GetPositionsCount())
        {
            ++cTriangleIndices;
        }
        cTriangleIndices -= cTriangleIndices % 3; 

        // If we have no valid indices early exit.
        if (cTriangleIndices == 0)
        {
            goto Cleanup;
        }

        cVertices = GetPositionsCount();  
    }
    else
    {
        Assert(m_data.m_cbPositionsSize != 0);

        // We have vertices but no indices. We'll treat this as a non-indexed mesh and
        // grab as many vertices as possible
        cVertices = GetPositionsCount();
        cVertices -= cVertices % 3;
        cTriangleIndices = 0;
    }

    IFC(CMILMesh3D::Create(NULL, cVertices, cTriangleIndices, &pIMesh));
    pMesh = static_cast<CMILMesh3D*>(pIMesh);

    IFC(pMesh->CopyTextureCoordinatesFromDoubles(
        m_data.m_pTextureCoordinatesData,
        m_data.m_cbTextureCoordinatesSize
        ));

    // cVertices is not necessarily equal to GetPositionsCount()
    size_t cbNewPositionBufferSize = cVertices * sizeof(m_data.m_pPositionsData[0]);
    Assert(cbNewPositionBufferSize <= m_data.m_cbPositionsSize);

    C_ASSERT(sizeof(vector3) == sizeof(m_data.m_pPositionsData[0]));
    IFC(pMesh->CopyPositionsFrom(
        reinterpret_cast<const vector3*>(m_data.m_pPositionsData),
        cbNewPositionBufferSize
        ));

    if (cTriangleIndices != 0)
    {
        // cTriangleIndices is not necessarily equal to GetTriangleIndicesCount()
        size_t cbNewIndexBufferSize = cTriangleIndices * sizeof(m_data.m_pTriangleIndicesData[0]);
        Assert(cbNewIndexBufferSize <= m_data.m_cbTriangleIndicesSize);

        // Earlier we made sure to throw out negative indices
        IFC(pMesh->CopyIndicesFrom(
            reinterpret_cast<UINT*>(m_data.m_pTriangleIndicesData),
            cbNewIndexBufferSize
            ));
    }

    // We don't trust the user to normalize all of the normals they provided, so
    // we will do so here.
    C_ASSERT(sizeof(vector3) == sizeof(m_data.m_pNormalsData[0]));
    auto pvec3NormalsData = reinterpret_cast<vector3*>(m_data.m_pNormalsData);
    for (UINT i = 0; i < GetNormalsCount(); i++)
    {     
        pvec3NormalsData[i] = pvec3NormalsData[i].normalize();
    }

    IFC(pMesh->CopyNormalsFrom(
        pvec3NormalsData,
        m_data.m_cbNormalsSize
        ));

    // Finally, "return" ppMesh
    *ppMesh = pMesh;

Cleanup:
    RRETURN(hr);
}

