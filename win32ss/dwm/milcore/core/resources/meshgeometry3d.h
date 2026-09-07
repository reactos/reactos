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

MtExtern(CMilMeshGeometry3DDuce);
MtExtern(pMeshGeometry3DVertexList);

// Class: CMilMeshGeometry3DDuce
class CMilMeshGeometry3DDuce : public CMilGeometry3DDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilMeshGeometry3DDuce));

    CMilMeshGeometry3DDuce(__in_ecount(1) CComposition* pComposition)
        : CMilGeometry3DDuce(pComposition)
    {
    }

    virtual ~CMilMeshGeometry3DDuce();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_MESHGEOMETRY3D || CMilGeometry3DDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_MESHGEOMETRY3D* pCmd,
        __in_bcount(cbPayload) LPCVOID pPayload,
        UINT cbPayload
        );

    HRESULT RegisterNotifiers(__in_ecount(1) CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    void ClearRealization();
    virtual HRESULT GetRealization(__deref_out_ecount_opt(1) CMILMesh3D **ppRealization);

private:
    HRESULT Realize(__deref_out_ecount_opt(1) CMILMesh3D **ppMesh);

    UINT32 GetPositionsCount() const
    {
        return m_data.m_cbPositionsSize / sizeof(m_data.m_pPositionsData[0]);
    }

    UINT32 GetNormalsCount() const
    {
        return m_data.m_cbNormalsSize / sizeof(m_data.m_pNormalsData[0]);
    }

    UINT32 GetTextureCoordinatesCount() const
    {
        return m_data.m_cbTextureCoordinatesSize / sizeof(m_data.m_pTextureCoordinatesData[0]);
    }

    UINT32 GetTriangleIndicesCount() const
    {
        return m_data.m_cbTriangleIndicesSize / sizeof(m_data.m_pTriangleIndicesData[0]);
    }

public:
    CMILMesh3D* m_realization;


    CMilMeshGeometry3DDuce_Data m_data;

};

