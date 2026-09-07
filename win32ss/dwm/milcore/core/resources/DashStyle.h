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
//      Definition of CMilDashStyleDuce
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilDashStyleDuce);

// Class: CMilDashStyleDuce
class CMilDashStyleDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilDashStyleDuce));

    CMilDashStyleDuce(__in_ecount(1) CComposition*)
    {
    }

    virtual ~CMilDashStyleDuce();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_DASHSTYLE;
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_DASHSTYLE* pCmd,
        __in_bcount(cbPayload) LPCVOID pPayload,
        UINT cbPayload
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    HRESULT SetDashes(
        __inout_ecount(1) CPlainPen *pPen);  // The pen to set dashes on

    CMilDashStyleDuce_Data m_data;
};


