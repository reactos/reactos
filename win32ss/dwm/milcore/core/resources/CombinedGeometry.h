// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_geometry
//      $Keywords:
//      $File name: CombinedGeometry.h
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilCombinedGeometryDuce);

// Class: CMilCombinedGeometryDuce
class CMilCombinedGeometryDuce : public CMilGeometryDuce, public CMilCyclicResourceListEntry
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilCombinedGeometryDuce));

    CMilCombinedGeometryDuce(
        __in_ecount(1) CComposition* pComposition,
        __in_ecount(1) CMilSlaveHandleTable *pHTable
        ) : CMilGeometryDuce(pComposition),
            CMilCyclicResourceListEntry(pHTable)
    {
        SetDirty(TRUE);
    }

    virtual ~CMilCombinedGeometryDuce();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_COMBINEDGEOMETRY || CMilGeometryDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_COMBINEDGEOMETRY* pCmd
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();
    override CMilSlaveResource* GetResource();

private:

    CShape m_shape;

public:

    virtual HRESULT GetShapeDataCore(__deref_out_ecount(1) IShapeData **);

    CMilCombinedGeometryDuce_Data m_data;

};

