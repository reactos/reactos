// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//       VideoDrawing Duce resource definition.
//

MtExtern(CMilVideoDrawingDuce);

//+----------------------------------------------------------------------------
//
//  Class:     CMilVideoDrawingDuce
//
//  Synopsis:  CMilDrawingDuce implementation that renders video.  This
//             class is the resource that backs the managed VideoDrawing
//             class.
//
//-----------------------------------------------------------------------------
class CMilVideoDrawingDuce : public CMilDrawingDuce
{
    friend class CResourceFactory;

protected:
    
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilVideoDrawingDuce));

    CMilVideoDrawingDuce(__in_ecount(1) CComposition* pComposition)
        : CMilDrawingDuce(pComposition)
    {
    }

    virtual ~CMilVideoDrawingDuce() { UnRegisterNotifiers(); }

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_VIDEODRAWING || CMilDrawingDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VIDEODRAWING* pCmd
        );

    HRESULT RegisterNotifiers(__in_ecount(1) CMilSlaveHandleTable *pHandleTable);
    
    override void UnRegisterNotifiers();

    virtual HRESULT Draw(
        __in_ecount(1) CDrawingContext *pDrawingContext
        );    

private:
    
    CMilVideoDrawingDuce_Data m_data;    
};



