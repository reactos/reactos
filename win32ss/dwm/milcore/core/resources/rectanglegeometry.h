// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_geometry
//      $Keywords:
//
//  $Description:
//      The RectangleGeometry CSlaveResource is responsible for maintaining the
//      current base values & animation resources for all RectangleGeometry
//      properties.  This class processes updates to those properties, and
//      obtains their current value when GetShapeDataCore is called.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilRectangleGeometryDuce);

// Class: CMilRectangleGeometryDuce
class CMilRectangleGeometryDuce : public CMilGeometryDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilRectangleGeometryDuce));

    CMilRectangleGeometryDuce(__in_ecount(1) CComposition* pComposition)
        : CMilGeometryDuce(pComposition)
    {
        SetDirty(TRUE);
        m_pShape = NULL;
    }

    virtual ~CMilRectangleGeometryDuce();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_RECTANGLEGEOMETRY || CMilGeometryDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_RECTANGLEGEOMETRY* pCmd
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();
private:
    IShapeData *m_pShape;

public:
    virtual HRESULT GetShapeDataCore(__deref_out_ecount(1) IShapeData **);

    CMilRectangleGeometryDuce_Data m_data;

};


