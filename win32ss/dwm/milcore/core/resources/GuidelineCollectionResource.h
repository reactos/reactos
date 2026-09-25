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
//      GuidelineCollection resource definitions
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilGuidelineSetDuce);

class CMilScheduleRecord;

class CMilGuidelineSetDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

private:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMilGuidelineSetDuce));

    //
    // Construction/Destruction
    //

    CMilGuidelineSetDuce(CComposition *pDevice);
    virtual ~CMilGuidelineSetDuce();

public:
    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_GUIDELINESET;
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_GUIDELINESET* pCmd,
        __in_bcount(cbPayload) LPCVOID pPayload,
        UINT cbPayload
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    HRESULT GetGuidelineCollection(
        __deref_out_ecount(1) CGuidelineCollection **ppGuidelineCollection
        );

    HRESULT ScheduleRender();

private:

    HRESULT UpdateGuidelineCollection(
        __deref_out_ecount(1) CGuidelineCollection **ppGuidelineCollection
        );

private:
    CComposition *m_pComposition;
    CMilScheduleRecord *m_pScheduleRecord;

    CMilGuidelineSetDuce_Data m_data;
    CGuidelineCollection *m_pGuidelineCollection;
};


//+-----------------------------------------------------------------------------
//
//  Member:
//      CGuidelineCollectionResource::GetGuidelineCollection
//
//  Synopsis:
//      Convert CMilGuidelineSetDuce_Data to CGuidelineCollection, if not yet
//      converted; return the pointer to CGuidelineCollection.
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE HRESULT
CMilGuidelineSetDuce::GetGuidelineCollection(
    __deref_out_ecount(1) CGuidelineCollection **ppGuidelineCollection
    )
{
    //
    // A tricky way to get to know that m_data has been updated
    // by generated routine ProcessUpdate().
    // The routine UpdateGuidelineCollection
    // consumes data supplied by ProcessUpdate, and sets m_data in some
    // special state that's impossible after ProcessUpdate().
    //
    // Likely we could do better. Say, generated code could call
    // CompleteProcessUpdate() that could be defined as costless stub
    // in CMilSlaveResource so that particular resource could override it.
    // NotifyOnChanged() could serve this way, but unfortunately
    // it is void.

    bool fAlreadyConverted = (m_data.m_pGuidelinesXData == NULL)
                          && (m_data.m_cbGuidelinesXSize != 0);
    if (fAlreadyConverted)
    {
        *ppGuidelineCollection = m_pGuidelineCollection;
        return S_OK;
    }
    else
    {
        return UpdateGuidelineCollection(ppGuidelineCollection);
    }
}



