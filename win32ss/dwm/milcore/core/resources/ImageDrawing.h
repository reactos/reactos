// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//       ImageDrawing Duce resource definition.
//

MtExtern(CMilImageDrawingDuce);

//+----------------------------------------------------------------------------
//
//  Class:     CMilImageDrawingDuce
//
//  Synopsis:  CMilDrawingDuce implementation that draws an image.  This
//             class is the resource that backs the managed ImageDrawing
//             class.
//
//-----------------------------------------------------------------------------
class CMilImageDrawingDuce : public CMilDrawingDuce
{
    friend class CResourceFactory;

protected:
    
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilImageDrawingDuce));

    CMilImageDrawingDuce(__in_ecount(1) CComposition* pComposition)
        : CMilDrawingDuce(pComposition)
    {
    }

    virtual ~CMilImageDrawingDuce() { UnRegisterNotifiers(); }

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_IMAGEDRAWING || CMilDrawingDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_IMAGEDRAWING* pCmd
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    
    override void UnRegisterNotifiers();

    virtual HRESULT Draw(
        __in_ecount(1) CDrawingContext *pDrawingContext
        );    

private:
    
    CMilImageDrawingDuce_Data m_data;    
};



