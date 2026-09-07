// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_geometry
//      $Keywords:
//      $File name: GeometryGroup.h
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilGeometryGroupDuce);

// Class: CMilGeometryGroupDuce
class CMilGeometryGroupDuce : public CMilGeometryDuce, public CMilCyclicResourceListEntry
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilGeometryGroupDuce));

    CMilGeometryGroupDuce(
        __in_ecount(1) CComposition* pComposition,
        __in_ecount(1) CMilSlaveHandleTable *pHTable
        ) : CMilGeometryDuce(pComposition),
            CMilCyclicResourceListEntry(pHTable)
    {
        SetDirty(TRUE);
    }

    virtual ~CMilGeometryGroupDuce();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_GEOMETRYGROUP || CMilGeometryDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_GEOMETRYGROUP* pCmd,
        __in_bcount(cbPayload) LPCVOID pPayload,
        UINT cbPayload
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();
    override CMilSlaveResource* GetResource();

private:

    CShape m_shape;

public:

    virtual HRESULT GetShapeDataCore(IShapeData **);

    CMilGeometryGroupDuce_Data m_data;

};

