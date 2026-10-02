// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//       GeometryDrawing Duce resource definition.
//

MtExtern(CMilGeometryDrawingDuce);

//+----------------------------------------------------------------------------
//
//  Class:     CMilGeometryDrawingDuce
//
//  Synopsis:  CMilDrawingDuce implementation that draws a geometry.  This
//             class is the resource that backs the managed GeometryDrawing
//             class.
//
//-----------------------------------------------------------------------------
class CMilGeometryDrawingDuce : public CMilDrawingDuce
{
    friend class CResourceFactory;

protected:
    
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilGeometryDrawingDuce));

    CMilGeometryDrawingDuce(__in_ecount(1) CComposition* pComposition)
        : CMilDrawingDuce(pComposition)
    {
    }

    virtual ~CMilGeometryDrawingDuce() { UnRegisterNotifiers(); }

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_GEOMETRYDRAWING || CMilDrawingDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_GEOMETRYDRAWING* pCmd
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    
    override void UnRegisterNotifiers();

    virtual HRESULT Draw(
        __in_ecount(1) CDrawingContext *pDrawingContext
        );    

private:
    
    CMilGeometryDrawingDuce_Data m_data;    
};



