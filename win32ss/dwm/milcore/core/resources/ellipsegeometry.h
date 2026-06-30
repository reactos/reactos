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
//      The EllipseGeometry CSlaveResource is responsible for maintaining the
//      current base values & animation resources for all the EllipseGeometry
//      properties.  This class processes updates to those properties, and
//      obtains the current value of then when GetShapeDataCore is called.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilEllipseGeometryDuce);

// Class: CMilEllipseGeometryDuce
class CMilEllipseGeometryDuce : public CMilGeometryDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilEllipseGeometryDuce));

    CMilEllipseGeometryDuce(__in_ecount(1) CComposition* pComposition)
        : CMilGeometryDuce(pComposition)
    {
    }

    virtual ~CMilEllipseGeometryDuce();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_ELLIPSEGEOMETRY || CMilGeometryDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_ELLIPSEGEOMETRY* pCmd
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

private:

    CShape m_shape;

public:

    virtual HRESULT GetShapeDataCore(__deref_out_ecount(1) IShapeData **);

    CMilEllipseGeometryDuce_Data m_data;

};



