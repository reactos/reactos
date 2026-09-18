// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*=========================================================================*\



    File: api_mesh3d.cpp

    Module Name: MIL

\*=========================================================================*/

#include "precomp.hpp"
using namespace dxlayer;

MtDefine(CMILMesh3D, MILApi, "CMILMesh3D");

MtDefine(MMeshVertexPositionData,          CMILMesh3D, "MMeshVertexPositionData");
MtDefine(MMeshVertexNormalData,            CMILMesh3D, "MMeshVertexNormalData");
MtDefine(MMeshVertexTextureCoordinateData, CMILMesh3D, "MMeshVertexTextureCoordinateData");
MtDefine(MMeshIndexData,                   CMILMesh3D, "MMeshVertexIndexData");

/*=========================================================================*\
    CMILMesh - MIL Mesh3D Object
\*=========================================================================*/

//+------------------------------------------------------------------------
//
//  Function:  CMILMesh3D::ctor
//
//  Synopsis:  Initializes object.
//
//-------------------------------------------------------------------------
CMILMesh3D::CMILMesh3D(
    __in_ecount_opt(1) CMILFactory *pFactory,
    UINT cVertices,
    UINT cIndices,
    __out_ecount(1) HRESULT *result
    )
    : CMILObject(pFactory)
{   
    HRESULT hr = S_OK;
    
    m_fBoundsValid = FALSE;

#ifdef DBG
    m_fDBGPositionsSet = FALSE;
    m_fDBGIndicesSet = FALSE;
#endif

    m_cVertices = cVertices;
    m_cIndices = cIndices;

    m_pvec3Vertices = NULL;
    m_pvec3Normals = NULL;
    m_pvec2TextureCoordinates = NULL;
    m_pdwDiffuseColors = NULL;
    m_pdwSpecularColors = NULL;
    m_puIndices = NULL;
    
    m_fIsColorCacheValid = false;

    IFC(SizeTMult(sizeof(m_pvec3Vertices[0]), m_cVertices, &m_cbPositions));
    IFC(SizeTMult(sizeof(m_pvec3Normals[0]), m_cVertices, &m_cbNormals));
    IFC(SizeTMult(sizeof(m_pvec2TextureCoordinates[0]), m_cVertices, &m_cbTextureCoordinates));
    IFC(SizeTMult(sizeof(m_puIndices[0]), m_cIndices, &m_cbIndices));

    // The color buffers are allocated later
    m_cbDiffuseColors = 0;
    m_cbSpecularColors = 0;
    
    if (m_cVertices != 0)
    {
        //
        // Allocate Positions
        //
        m_pvec3Vertices = WPFAllocType(vector3*,
            ProcessHeap,
            Mt(MMeshVertexPositionData),
            m_cbPositions
            );
        IFCOOM(m_pvec3Vertices);

        //
        // Allocate Normals
        //
        m_pvec3Normals = WPFAllocType(vector3*,
            ProcessHeap,
            Mt(MMeshVertexNormalData),
            m_cbNormals
            );
        IFCOOM(m_pvec3Normals);
        
        //
        // Allocate Texture Coordinates
        //
        m_pvec2TextureCoordinates = WPFAllocType(vector2*,
            ProcessHeap,
            Mt(MMeshVertexTextureCoordinateData),
            m_cbTextureCoordinates
            );
        IFCOOM(m_pvec2TextureCoordinates);
    }

    if (m_cIndices != 0)
    {
        //
        // Allocate Indices
        //
        m_puIndices = WPFAllocType(UINT *,
            ProcessHeap,
            Mt(MMeshIndexData),
            m_cbIndices
            );
        IFCOOM(m_puIndices);
    } 

Cleanup:
    // CMILMesh3D::Create() will IFC(hr) and delete this on a failure
    *result = hr;
}

//+------------------------------------------------------------------------
//
//  Function:  CMILMesh3D::dtor
//
//  Synopsis:  Releases indices and vertex component arrays.
//
//-------------------------------------------------------------------------
CMILMesh3D::~CMILMesh3D()
{
    WPFFree(ProcessHeap, m_puIndices);
    WPFFree(ProcessHeap, m_pvec3Vertices);
    WPFFree(ProcessHeap, m_pvec3Normals);
    WPFFree(ProcessHeap, m_pdwDiffuseColors);
    WPFFree(ProcessHeap, m_pdwSpecularColors);
    WPFFree(ProcessHeap, m_pvec2TextureCoordinates);
}

//+------------------------------------------------------------------------
//
//  Function:  CMILMesh3D::Create
//
//  Synopsis:  Creates a mesh object.
//
//-------------------------------------------------------------------------
__checkReturn HRESULT 
CMILMesh3D::Create(
    __in_ecount_opt(1) CMILFactory *pFactory, 
    UINT cVertices,
    UINT cIndices,
    __deref_out_ecount(1) IMILMesh3D **ppIMesh3D
    )
{
    HRESULT hr = S_OK;

    Assert(ppIMesh3D);

    CMILMesh3D *pMesh3D = new CMILMesh3D(pFactory, cVertices, cIndices, &hr);
    IFCOOM(pMesh3D);

    // If there was an overflow or allocation error in the ctor, we'll error here 
    IFC(hr);

    pMesh3D->AddRef();

    // steals reference
    *ppIMesh3D = pMesh3D;
    pMesh3D = NULL;

Cleanup:
    delete pMesh3D;
    
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CalculateNormal
//
//  Synopsis:  Calculates the normal for a face that's specified by 
//             three vertices
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE void 
CalculateNormal(
    __in_ecount(1) const vector3 &vec3V0, 
    __in_ecount(1) const vector3 &vec3V1, 
    __in_ecount(1) const vector3 &vec3V2,
    __out_ecount(1) vector3 *pvec3Normal 
    )
{
    auto vec3Edge0 = vec3V0 - vec3V1;
    auto vec3Edge1 = vec3V0 - vec3V2;

    auto cross_product = vector3::cross_product(vec3Edge0, vec3Edge1);
    *pvec3Normal = cross_product.normalize();
}

//+------------------------------------------------------------------------
//
//  Function:  CMILMesh3D::CalculateNormals
//
//  Synopsis:  Calculates normals for the mesh, clobbering whatever alreay
//             existed in that array.
//
//-------------------------------------------------------------------------
void
CMILMesh3D::CalculateNormals()
{
#ifdef DBG
    Assert(m_pvec3Vertices == NULL || m_fDBGPositionsSet == TRUE);
    Assert(m_puIndices == NULL || m_fDBGIndicesSet == TRUE);
#endif

    if (m_pvec3Vertices && m_pvec3Normals)
    {
        vector3 vec3Normal;
            
        if (m_puIndices != NULL) // indexed mesh
        {
            // We'll be +='ing on m_pvec3Normals below so let's clear
            // the memory
            memset(
                m_pvec3Normals,
                0,
                m_cbNormals
                );
            
            Assert(m_cIndices % 3 == 0);
            UINT uNumFaces = m_cIndices / 3;

            for (UINT uFaceNum = 0; uFaceNum < uNumFaces; uFaceNum++)
            {
                UINT uVertex0 = m_puIndices[uFaceNum * 3 + 0];
                UINT uVertex1 = m_puIndices[uFaceNum * 3 + 1];
                UINT uVertex2 = m_puIndices[uFaceNum * 3 + 2];

                CalculateNormal(
                    m_pvec3Vertices[uVertex0], 
                    m_pvec3Vertices[uVertex1], 
                    m_pvec3Vertices[uVertex2], 
                    &vec3Normal
                    );

                m_pvec3Normals[uVertex0] += vec3Normal;
                m_pvec3Normals[uVertex1] += vec3Normal;
                m_pvec3Normals[uVertex2] += vec3Normal;
            }
            
            for (UINT i = 0; i < m_cVertices; i++)
            {
                m_pvec3Normals[i] = m_pvec3Normals[i].normalize();
            }
        }
        else // non-indexed mesh
        {
            // No memset needed because we aren't reading from m_pvec3Normals
            // and we'll be setting every value of it
            
            Assert(m_cVertices % 3 == 0);
            UINT uNumFaces = m_cVertices / 3;

            for (UINT uFaceNum = 0; uFaceNum < uNumFaces; uFaceNum++)
            {
                UINT uVertex0 = uFaceNum * 3 + 0;
                UINT uVertex1 = uFaceNum * 3 + 1;
                UINT uVertex2 = uFaceNum * 3 + 2;
                
                CalculateNormal(
                    m_pvec3Vertices[uVertex0], 
                    m_pvec3Vertices[uVertex1], 
                    m_pvec3Vertices[uVertex2], 
                    &vec3Normal
                    );

                m_pvec3Normals[uVertex0] = vec3Normal;
                m_pvec3Normals[uVertex1] = vec3Normal;
                m_pvec3Normals[uVertex2] = vec3Normal;
            }
        }   
    }
}

//+------------------------------------------------------------------------
//
//  Function:  CMILMesh3D::PrecomputeLighting
//
//  Synopsis:  Calculates the lighting values for the mesh
//
//-------------------------------------------------------------------------
HRESULT
CMILMesh3D::PrecomputeLighting(
    __in_ecount(1) const CMILMatrix *pmatWorldTransform,
    __in_ecount(1) const CMILMatrix *pmatViewTransform,
    __in_ecount(1) CMILLightData *pLightData
    )
{
    HRESULT hr = S_OK;

    Assert(pLightData != NULL);

    // Early exit if we have no vertices (i.e. the user didn't specify
    // a full triangle) or no lighting is required
    if (m_cVertices > 0 && (pLightData->IsDiffuseEnabled() || pLightData->IsSpecularEnabled()))
    {
        Assert(pmatWorldTransform!= NULL);
        Assert(pmatViewTransform!= NULL);
        Assert(m_pvec3Vertices != NULL);
        Assert(m_pvec3Normals != NULL);
        Assert(m_pvec2TextureCoordinates != NULL);

        if (!m_fIsColorCacheValid)
        {
            if (pLightData->IsDiffuseEnabled() && m_pdwDiffuseColors == NULL)
            {
                IFC(SizeTMult(sizeof(m_pdwDiffuseColors[0]), m_cVertices, &m_cbDiffuseColors));
                
                m_pdwDiffuseColors = WPFAllocType(DWORD *,
                    ProcessHeap,
                    Mt(MMeshIndexData),
                    m_cbDiffuseColors
                    );
                
                IFCOOM(m_pdwDiffuseColors);
            }

            if (pLightData->IsSpecularEnabled() && m_pdwSpecularColors == NULL)
            {
                IFC(SizeTMult(sizeof(m_pdwSpecularColors[0]), m_cVertices, &m_cbSpecularColors));
                
                m_pdwSpecularColors = WPFAllocType(DWORD *,
                    ProcessHeap,
                    Mt(MMeshIndexData),
                    m_cbSpecularColors
                    );
                
                IFCOOM(m_pdwSpecularColors);
            }
            
            // D3D does all lighting in camera space (a.k.a. worldview space). Right now we have
            // vertices in model space and lights in camera space. We would like to avoid transforming
            // all of the vertices and normals so let's transform the lights into model space when
            // possible. This is possible when worldview is a uniform SRT matrix.
            
            CMILMatrix matWorldViewTransform = *pmatWorldTransform * *pmatViewTransform;
            CMILMatrix matInvWorldViewTransform;

            vector3 vec3Scale, vec3Translation;
            quaternion quatRotation;

            auto canDecomposeMatWorldViewTransform = [&matWorldViewTransform]()->bool
            {
                try
                {
                    matWorldViewTransform.decompose();
                    return true;
                }
                catch (const dxlayer_exception&)
                {
                    return false;
                }
            };

            if (canDecomposeMatWorldViewTransform()
                && IsUniformNonZeroVec3(&vec3Scale))
            {
                // If the following call to inverse() throws, then we will crash. 
                // In the version of this code prior to the introduction of dxlayer::matrix<>::inverse(), 
                // this code would have failed to initialize matInvWorldViewTransform, resulting in 
                // potentially unpredictable behavior. Crashing is a better outcome if it turns out 
                // that inverse can not be calculated here. 
                matInvWorldViewTransform = matWorldViewTransform.inverse();

                // The previous call to IsUniformNonZeroVec3 ensures that vec3Scale.x != 0. 
                // Despite this, we see warning C4723 (potential divide by 0). 
                // Attempting to suppress this warning helps during compilation, but fails
                // during linking when whole-program optimization kicks in. 
                // The best solution seems to be to make it exceedingly clear that division by '0' 
                // is not likely. 
                auto flScale = (vec3Scale.x != 0) ? (1.0f / vec3Scale.x) : std::numeric_limits<float>::infinity();

                pLightData->Transform(CMILLight::TransformType_LightingSpace, &matInvWorldViewTransform, flScale);
                pLightData->SetCameraPosition(
                    matInvWorldViewTransform._41,
                    matInvWorldViewTransform._42,
                    matInvWorldViewTransform._43
                );

                for (UINT uVertexNum = 0; uVertexNum < m_cVertices; uVertexNum++)
                {
                    pLightData->GetLightContribution(
                        &m_pvec3Vertices[uVertexNum],
                        &m_pvec3Normals[uVertexNum],
                        pLightData->IsDiffuseEnabled() ? &m_pdwDiffuseColors[uVertexNum] : NULL,
                        pLightData->IsSpecularEnabled() ? &m_pdwSpecularColors[uVertexNum] : NULL
                    );
                }
            }
            else
            {          
                CMILMatrix matAdjTransWorldView;

                // Normals are transformed by the transpose of the adjoint (or inverse) of the matrix 
                // used to transform the vertices.  Since we define frontedness to do that flippy
                // thing it does we want the inverse.  Or actually since we don't care about the magnitude
                // the adjoint times the sign of the determinant is better.
                MILMatrixAdjoint(
                    &matAdjTransWorldView, 
                    &matWorldViewTransform);
                matAdjTransWorldView *=
                    (matWorldViewTransform.GetDeterminant3D() < 0.f) ? -1.f : 1.f;
                matAdjTransWorldView = matAdjTransWorldView.transpose();

                // the 2nd and 3rd arguments are ignored on a Copy so it doesn't matter
                // what we give them
                pLightData->Transform(CMILLight::TransformType_Copy, &matInvWorldViewTransform, 1.0);
                pLightData->SetCameraPosition(0, 0, 0);

                for (UINT uVertexNum = 0; uVertexNum < m_cVertices; uVertexNum++)
                {   
                    auto vec3PositionCameraSpace = 
                        math_extensions::transform_coord(
                            m_pvec3Vertices[uVertexNum], 
                            matWorldViewTransform
                        );
                    auto vec3NormalCameraSpace =
                        math_extensions::transform_normal(
                            m_pvec3Normals[uVertexNum], 
                            matAdjTransWorldView
                        ).normalize();

                    pLightData->GetLightContribution(
                        &vec3PositionCameraSpace,
                        &vec3NormalCameraSpace,
                        pLightData->IsDiffuseEnabled() ? &m_pdwDiffuseColors[uVertexNum] : NULL,
                        pLightData->IsSpecularEnabled() ? &m_pdwSpecularColors[uVertexNum] : NULL
                        );
                }
            }
        }
        else
        {
            // We've already done lighting for this model
            Assert(!pLightData->IsDiffuseEnabled() || m_pdwDiffuseColors != NULL);
            Assert(!pLightData->IsSpecularEnabled() || m_pdwSpecularColors != NULL);
        }
        
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CMILMesh3D::CopyPositionsFrom
//
//  Synopsis:  Copies new position data into the mesh, and invalidates the
//             cached bounds.
//
//             cbSize is the size of pVertexPositions' buffer in bytes
//
//-------------------------------------------------------------------------
HRESULT
CMILMesh3D::CopyPositionsFrom(
    __in_bcount(cbSize) const vector3* pVertexPositions,
    size_t cbSize
    )
{
    HRESULT hr = S_OK;

    if (m_cbPositions != 0)
    {
        if (cbSize == m_cbPositions && pVertexPositions)
        {
            m_fBoundsValid = false;

            Assert(m_pvec3Vertices);

            memcpy(
                m_pvec3Vertices,
                pVertexPositions,
                m_cbPositions
                );

#ifdef DBG
            m_fDBGPositionsSet = TRUE;
#endif
        }
        else
        {
            IFC(E_INVALIDARG);
        }
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CMILMesh3D::CopyNormalsFrom
//
//  Synopsis:  Copies as many normals from pVertexNormals as it can
//
//             If the user did not supply any normals or if the user
//             supplied too few, we'll generate the rest.
//
//             Assumes that pVertexNormals contains normalized normals. 
//             This method DOES NOT normalize anything.
//
//-------------------------------------------------------------------------
HRESULT
CMILMesh3D::CopyNormalsFrom(
    __in_bcount_opt(cbSize) const vector3* pVertexNormals,
    size_t cbSize
    )
{
    // If the user didn't specify any normals or they specified
    // too few normals, generate our own based on the position data
    if (!pVertexNormals || cbSize < m_cbNormals)
    {
        CalculateNormals();
    }
    
    // Copy as many normals as we can. If the user specified too many,
    // we'll copy cbNormalSize bytes. If the user specified too few
    // we'll take cbSize bytes and the rest of our buffer is correct
    // from CalculateNormals. 
    if (pVertexNormals && m_pvec3Normals)
    {
        size_t smallerSize = min(cbSize, m_cbNormals);
        memcpy(
            m_pvec3Normals,
            pVertexNormals,
            smallerSize
            );
    }

    RRETURN(S_OK);
}

//+------------------------------------------------------------------------
//
//  Function:  CMILMesh3D::CopyTextureCoordinatesFrom
//
//  Synopsis:  Copies new texture coordinate data into the mesh.
//
//             If pVertexTextureCoordinates is NULL, the mesh will zero out
//             its coordinates.
//
//             cbSize is the size of pVTexCoords in bytes
//
//-------------------------------------------------------------------------
HRESULT
CMILMesh3D::CopyTextureCoordinatesFrom(
    __in_bcount_opt(cbSize) const vector2* pVertexTextureCoordinates,
    size_t cbSize
    )
{
    HRESULT hr = S_OK;

    if (m_cbTextureCoordinates)
    {
        Assert(m_pvec2TextureCoordinates);
        
        if (pVertexTextureCoordinates)
        {
            if (m_cbTextureCoordinates == cbSize)
            {
                memcpy(
                    m_pvec2TextureCoordinates,
                    pVertexTextureCoordinates,
                    m_cbTextureCoordinates
                    );
            }
            else
            {
                IFC(E_INVALIDARG);
            }
        }
        else
        {
            memset(
                m_pvec2TextureCoordinates,
                0,
                m_cbTextureCoordinates
                );
        }
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:  CMILMesh3D::CopyTextureCoordinatesFromDoubles
//
//  Synopsis:  Copies new texture coordinate data into the mesh. If too
//             few coords are specified, the rest are filled in with (0,0).
//
//             if pVTexCoords is null, all coords are set to zero
//
//             cbSize is the size of pVTexCoords in bytes
//
//-------------------------------------------------------------------------
HRESULT 
CMILMesh3D::CopyTextureCoordinatesFromDoubles(
    __in_bcount_opt(cbSize) const MilPoint2D *pVertexTextureCoordinates,
    size_t cbSize
    )
{
    size_t numTexCoordsInUserBuffer = cbSize / sizeof(pVertexTextureCoordinates[0]);

    vector2 zeroCoord(0.0f, 0.0f);

    if (m_cbTextureCoordinates)
    {
        Assert(m_pvec2TextureCoordinates);
        
        if (pVertexTextureCoordinates)
        {
            for (UINT i = 0; i < GetNumVertices(); i++)
            {
                if (i < numTexCoordsInUserBuffer)
                {
                    m_pvec2TextureCoordinates[i].x = static_cast<float>(pVertexTextureCoordinates[i].X);
                    m_pvec2TextureCoordinates[i].y = static_cast<float>(pVertexTextureCoordinates[i].Y);
                }
                else
                {
                    m_pvec2TextureCoordinates[i] = zeroCoord;
                }
            }
        }
        else
        {
            memset(
                m_pvec2TextureCoordinates,
                0,
                m_cbTextureCoordinates
                );
        }
    }

    RRETURN(S_OK);
}

//+------------------------------------------------------------------------
//
//  Function:  CMILMesh3D::CopyIndicesFrom
//
//  Synopsis:  Copies new index data into the mesh.
//
//             cbSize is the size of rgIndices in bytes
//
//-------------------------------------------------------------------------
HRESULT
CMILMesh3D::CopyIndicesFrom(
    __in_bcount(cbSize) const UINT *rgIndices,
    size_t cbSize
    )
{
    HRESULT hr = S_OK;

    if (m_cbIndices)
    {
        Assert(m_puIndices);
        
        if (m_cbIndices == cbSize && rgIndices)
        {
            memcpy(
                m_puIndices,
                rgIndices,
                m_cbIndices
                );
#ifdef DBG
            m_fDBGIndicesSet = TRUE;
#endif
        }
        else
        {
            IFC(E_INVALIDARG);
        }
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CMILMesh3D::CloneMesh
//
//  Synopsis:  Makes a copy of the mesh.
//
//-------------------------------------------------------------------------
HRESULT
CMILMesh3D::CloneMesh(
    __deref_out_ecount(1) IMILMesh3D **ppIMesh3D
    )
{
    HRESULT hr = S_OK;
    IMILMesh3D *pNewMesh = NULL;

    IFC(CMILMesh3D::Create(
        m_pFactory, 
        m_cVertices,
        m_cIndices,
        &pNewMesh
        ));

    Assert(m_pvec3Vertices);
    Assert(m_pvec3Normals);
    Assert(m_pvec2TextureCoordinates);
    // No indices assert because it could be non-indexed

    IFC(pNewMesh->CopyPositionsFrom(m_pvec3Vertices, m_cbPositions));
    IFC(pNewMesh->CopyNormalsFrom(m_pvec3Normals, m_cbNormals));
    IFC(pNewMesh->CopyTextureCoordinatesFrom(m_pvec2TextureCoordinates, m_cbTextureCoordinates));
    IFC(pNewMesh->CopyIndicesFrom(m_puIndices, m_cbIndices));

    *ppIMesh3D = pNewMesh;
    pNewMesh = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pNewMesh);

    RRETURN(hr);
}
    

//+------------------------------------------------------------------------
//
//  Function:  CMILMesh3D::NotifyPositionChange
//
//  Synopsis:  Tells the mesh object that it's positions have been modified and
//             let's it know whether it should recalculate normals.
//
//-------------------------------------------------------------------------
void
CMILMesh3D::NotifyPositionChange(
    BOOL fCalculateNormals
    )
{
    m_fBoundsValid = FALSE;

#ifdef DBG
    m_fDBGPositionsSet = TRUE;
#endif

    if (fCalculateNormals)
    {
        CalculateNormals();
    }
}

//+------------------------------------------------------------------------
//
//  Function:  CMILMesh3D::NotifyIndicesChange
//
//  Synopsis:  Tells the mesh object that it's indices have been modified and
//             let's it know whether it should recalculate normals.
//
//-------------------------------------------------------------------------
void
CMILMesh3D::NotifyIndicesChange(
    BOOL fCalculateNormals
    )
{
#ifdef DBG
    m_fDBGIndicesSet = TRUE;
#endif

    if (fCalculateNormals)
    {
        CalculateNormals();
    }
}

//+------------------------------------------------------------------------
//
//  Function:  CMILMesh3D::GetBounds
//
//  Synopsis:  If the cached bounds aren't valid it calculates them.  Then it
//             returns the bounds.
//
//-------------------------------------------------------------------------
HRESULT 
CMILMesh3D::GetBounds(
    __out_ecount(1) MilPointAndSize3F *pboxBounds
    )
{
    HRESULT hr = S_OK;

    Assert(pboxBounds);

    if (!m_fBoundsValid)
    {
        IFC(CalculateBounds());
    }

    *pboxBounds = m_boxBounds;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CMILMesh3D::GetTextureCoordinateBounds
//
//  Synopsis:  If the cached bounds aren't valid it calculates them.  Then it
//             returns the bounds.
//
//-------------------------------------------------------------------------
HRESULT
CMILMesh3D::GetTextureCoordinateBounds(
    __out_ecount(1) CRectF<CoordinateSpace::BaseSampling> *pTextureCoordinateBounds
    )
{
    HRESULT hr = S_OK;

    if (!m_fBoundsValid)
    {
        IFC(CalculateBounds());
    }

    *pTextureCoordinateBounds = m_rcTextureCoordinateBounds;

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:  CMILMesh3D::CalculateBounds
//
//  Synopsis:  Non API function, used to calculate the bounds of the mesh.
//
//-------------------------------------------------------------------------
HRESULT 
CMILMesh3D::CalculateBounds()
{
    HRESULT hr = S_OK;

    m_fBoundsValid = FALSE;

    //
    // Calculate the bounds of the vertices
    //
    if (m_pvec3Vertices)
    {
        //
        // Calculate the bounding box of the vertices
        //
        try
        {
            auto bounding_box =
                vector3::compute_bounding_box(m_pvec3Vertices, m_cVertices, sizeof(m_pvec3Vertices[0]));

            m_boxBounds = CMilPointAndSize3F(bounding_box.first, bounding_box.second);
        }
        catch (dxlayer_exception& ex)
        {
            try
            {
                const hresult& err_hr = dynamic_cast<const hresult&>(ex.get_error());
                hr = err_hr.get_hr();
            }
            catch (std::bad_cast&)
            {
                // Couldn't find a specific HRESULT value, 
                // so set it to the generic failure code E_FAIL.
                hr = E_FAIL;
            }
        }
    }
    else
    {
        m_boxBounds = CMilPointAndSize3F::sc_boxEmpty;
    }

    if (SUCCEEDED(hr))
    {
        //
        // Calculate the bounds of the texture coordinates.
        //
        if (m_pvec2TextureCoordinates)
        {
            const vector2 *pvec2TextureCoordinate = m_pvec2TextureCoordinates;
            vector2
                ptMinUV = pvec2TextureCoordinate[0],
                ptMaxUV = pvec2TextureCoordinate[0];

            for (DWORD dwVertexNum = 1;
                dwVertexNum < m_cVertices;
                dwVertexNum++)
            {
                vector2 flCurrentUV = pvec2TextureCoordinate[dwVertexNum];

                ptMaxUV.x = max(flCurrentUV.x, ptMaxUV.x);
                ptMinUV.x = min(flCurrentUV.x, ptMinUV.x);

                ptMaxUV.y = max(flCurrentUV.y, ptMaxUV.y);
                ptMinUV.y = min(flCurrentUV.y, ptMinUV.y);
            }

            m_rcTextureCoordinateBounds.left = ptMinUV.x;
            m_rcTextureCoordinateBounds.top = ptMinUV.y;

            m_rcTextureCoordinateBounds.right = ptMaxUV.x;
            m_rcTextureCoordinateBounds.bottom = ptMaxUV.y;
        }
        else
        {
            m_rcTextureCoordinateBounds.SetEmpty();
        }

        m_fBoundsValid = TRUE;
    }

    RRETURN(hr);
}

/*=========================================================================*\
    Support methods
\*=========================================================================*/

HRESULT 
CMILMesh3D::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    HRESULT hr = S_OK;

    Assert(ppvObject);

    if (riid == IID_IMILMesh3D)
    {
        *ppvObject = static_cast<IMILMesh3D *>(this);
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    RRETURN(hr);
}


