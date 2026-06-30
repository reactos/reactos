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
//      The LineGeometry CSlaveResource is responsible for maintaining the
//      current base values & animation resources for all LineGeometry
//      properties.  This class processes updates to those properties, and
//      obtains their current value when GetShapeDataCore is called.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilLineGeometryDuce);

// Class: CMilLineGeometryDuce
class CMilLineGeometryDuce : public CMilGeometryDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilLineGeometryDuce));

    CMilLineGeometryDuce(__in_ecount(1) CComposition* pComposition)
        : CMilGeometryDuce(pComposition)
    {
    }

    virtual ~CMilLineGeometryDuce();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_LINEGEOMETRY || CMilGeometryDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_LINEGEOMETRY* pCmd
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

private:
    CLine m_line;

public:
    virtual HRESULT GetShapeDataCore(__deref_out_ecount(1) IShapeData **);

    CMilLineGeometryDuce_Data m_data;

};


