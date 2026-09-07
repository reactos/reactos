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
//      GuidelineCollection resource implemenmtation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CMilGuidelineSetDuce, MILRender, "CMilGuidelineSetDuce");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilGuidelineSetDuce::CMilGuidelineSetDuce
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CMilGuidelineSetDuce::CMilGuidelineSetDuce(CComposition *pComposition)
{
    m_pComposition = pComposition;
    m_pGuidelineCollection = NULL;
    m_pScheduleRecord = NULL;

    m_data.m_pGuidelinesXData = NULL;
    m_data.m_cbGuidelinesXSize = 0;
    m_data.m_pGuidelinesYData = NULL;
    m_data.m_cbGuidelinesYSize = 0;
    m_data.m_IsDynamic = FALSE;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilGuidelineSetDuce::~CMilGuidelineSetDuce
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CMilGuidelineSetDuce::~CMilGuidelineSetDuce()
{
    UnRegisterNotifiers();

    if (m_pScheduleRecord)
    {
        CMilScheduleManager* pScheduleManager = m_pComposition->GetScheduleManager();
        Assert(pScheduleManager);
        pScheduleManager->Unschedule(&m_pScheduleRecord);
    }

    delete m_pGuidelineCollection;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilGuidelineSetDuce::ScheduleRender
//
//  Synopsis:
//
//------------------------------------------------------------------------------

HRESULT
CMilGuidelineSetDuce::ScheduleRender()
{
    CMilScheduleManager* pScheduleManager = m_pComposition->GetScheduleManager();
    Assert(pScheduleManager);
    return pScheduleManager->ScheduleRelative(
        this,
        &m_pScheduleRecord,
        CDynamicGuideline::sc_uTimeDelta
        );
}


//+-----------------------------------------------------------------------------
//
//  Structure:
//      DoublePair
//
//  Synopsis:
//      Contains two double values that define one dynamic guideline. The first
//      value is coordinate and the second is shift.
//      CGuidelineCollection::CreateFromDoubles requires given pairs to be
//      sorted in increasing order by (coordinate+shift).
//
//      The sum above, after converting to device space, defines the final
//      position of guideline. When applying guideline set to the geometry, for
//      each point we need to figure out which guideline is closest to this
//      point. Keeping array in sorted order helps to improve performance using
//      binary search algorithm.
//
//      The "operator >" allows to use template ArrayInsertionSort for such
//      sorting.
//
//------------------------------------------------------------------------------
struct DoublePair
{
    double value[2];
    bool operator > (DoublePair const & other) const
    {
        return value[0] + value[1] > other.value[0] + other.value[1];
    }
};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilGuidelineSetDuce::UpdateGuidelineCollection
//
//  Synopsis:
//      Convert CMilGuidelineSetDuce_Data to CGuidelineCollection, return the
//      pointer to CGuidelineCollection.
//
//------------------------------------------------------------------------------
HRESULT
CMilGuidelineSetDuce::UpdateGuidelineCollection(
    __deref_out_ecount(1) CGuidelineCollection **ppGuidelineCollection
    )
{
    HRESULT hr = S_OK;

    //
    // This private method should not be called twice after
    // single call to generated ProcessUpdate().
    // See comments for GetGuidelineCollection().
    //
#if DBG
    bool fAlreadyConverted = (m_data.m_pGuidelinesXData == NULL)
                          && (m_data.m_cbGuidelinesXSize != 0);
    Assert(!fAlreadyConverted);
#endif //DBG


    delete m_pGuidelineCollection;
    m_pGuidelineCollection = NULL;

    double* prgDataX = reinterpret_cast<double*>(m_data.m_pGuidelinesXData);
    double* prgDataY = reinterpret_cast<double*>(m_data.m_pGuidelinesYData);

    UINT16 usCountX = static_cast<UINT16>(m_data.m_cbGuidelinesXSize/sizeof(double));
    UINT16 usCountY = static_cast<UINT16>(m_data.m_cbGuidelinesYSize/sizeof(double));
    if (usCountX != m_data.m_cbGuidelinesXSize/sizeof(double) || 
        usCountY != m_data.m_cbGuidelinesYSize/sizeof(double))
    {
        // The update packet is malformed.
        // Don't treat it as a fatal failure.
        // Just allow rendering to run without pixel snapping.
        goto Cleanup;
    }

    //
    // Don't create CGuidelineCollection if there is no data given.
    // Keep m_pGuidelineCollection = NULL.
    //
    if (usCountX != 0 || usCountY != 0)
    {
        if (prgDataX)
        {
            if (m_data.m_IsDynamic)
            {
                // see comments above for struct DoublePair
                ArrayInsertionSort(reinterpret_cast<DoublePair*>(prgDataX), usCountX/2);
            }
            else
            {
                ArrayInsertionSort(prgDataX, usCountX);
            }
        }
        if (prgDataY)
        {
            if (m_data.m_IsDynamic)
            {
                // see comments above for struct DoublePair
                ArrayInsertionSort(reinterpret_cast<DoublePair*>(prgDataY), usCountY/2);
            }
            else
            {
                ArrayInsertionSort(prgDataY, usCountY);
            }
        }

        MIL_THR(CGuidelineCollection::CreateFromDoubles(
            usCountX,
            usCountY,
            prgDataX,
            prgDataY,
            m_data.m_IsDynamic,
            &m_pGuidelineCollection
            ));

        if (hr == WGXERR_MALFORMED_GUIDELINE_DATA)
        {
            // Something is wrong in given data.
            // Don't treat it as a fatal failure.
            // Just allow rendering to run without pixel snapping.
            Assert(m_pGuidelineCollection == NULL);
            hr = S_OK;
        }
    }

Cleanup:
    // Following call will free memory occupied by
    // arrays held in m_data.
    UnRegisterNotifiers();

    // change m_data so that next call to GetGuidelineCollection()
    // will find that m_pGuidelineCollection is already updated.
    Assert(m_data.m_cbGuidelinesXSize == 0);
    m_data.m_cbGuidelinesXSize = 1;

    *ppGuidelineCollection = m_pGuidelineCollection;

    RRETURN(hr);
}


