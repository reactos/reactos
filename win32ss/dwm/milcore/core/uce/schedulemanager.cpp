// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//  

//
//  Clesses:    
//      CMilScheduleManager
//      CMilScheduleRecord;
//

#include "precomp.hpp"

MtDefine(CMilScheduleManager, Mem, "CMilScheduleManager");


//+-----------------------------------------------------------------------------
//  
//  Class:      CMilScheduleRecord
//
//  Synopsis:
//      The entity to store scheduled rendering request for one resource.
//------------------------------------------------------------------------------
class CMilScheduleRecord
{
    friend class CMilScheduleManager; // and nobody else
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMilScheduleManager));

    CMilSlaveResource* m_pResource;     // reference in client resource
    CMilScheduleRecord* m_pNext;        // next in list
    CMilScheduleRecord** m_pBack;       // previous in list
    CMilScheduleRecord** m_pAnchor;     // reference in client hook
    DWORD m_dwTimeToWake;               // requested wake up time, in milliseconds
};


//+-----------------------------------------------------------------------------
//  
//  Method:     CMilScheduleManager::CMilScheduleManager
//      
//  Synopsis:   Constructor
//
//------------------------------------------------------------------------------
CMilScheduleManager::CMilScheduleManager()
{
    m_pActiveRecords = NULL;
    m_pRecycledRecords = NULL;
    m_dwCurrentTime = 0;
    m_dwTimeToWake = INFINITE;
}

//+-----------------------------------------------------------------------------
//  
//  Method:     CMilScheduleManager::~CMilScheduleManager
//      
//  Synopsis:   Destructor
//
//------------------------------------------------------------------------------
CMilScheduleManager::~CMilScheduleManager()
{
    while (m_pActiveRecords)
    {
        delete UnhookRecord(m_pActiveRecords);
    }

    while (m_pRecycledRecords)
    {
        delete FetchRecycledRecord();
    }
}

//+-----------------------------------------------------------------------------
//  
//  Method:     CMilScheduleManager::Schedule
//      
//  Synopsis:   Accept reschedule request.
//              Time is measured in milliseconds.
//
//------------------------------------------------------------------------------
HRESULT
CMilScheduleManager::Schedule(
    CMilSlaveResource *pResource,
    CMilScheduleRecord **pAnchor,
    DWORD dwTimeToWake
    )
{
    HRESULT hr = S_OK;
    Assert(pResource);
    Assert(pAnchor);

    CMilScheduleRecord *pRecord = *pAnchor;
    if (pRecord == NULL)
    {
        pRecord = GetFreeRecord();
        IFCOOM(pRecord);
        HookupRecord(pRecord, pAnchor);
        pRecord->m_pResource = pResource;
    }
    else
    {
        Assert(pRecord->m_pResource == pResource);
    }

    pRecord->m_dwTimeToWake = dwTimeToWake;

    //
    // Update m_dwTimeToWake that keeps the time that's most close to current moment.
    // We can't compare times explicitely, due to overflow.
    // Need to subtract current time first.
    // Overflow (i.e. wrap around) on this subtraction is okay.
    //
    if ((pRecord->m_dwTimeToWake - m_dwCurrentTime) < (m_dwTimeToWake - m_dwCurrentTime))
    {
        m_dwTimeToWake = pRecord->m_dwTimeToWake;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//  
//  Method:     CMilScheduleManager::Unschedule
//      
//  Synopsis:   Cancel reschedule request
//
//------------------------------------------------------------------------------
void
CMilScheduleManager::Unschedule(CMilScheduleRecord **pAnchor)
{
    Assert(pAnchor);

    CMilScheduleRecord *pRecord = *pAnchor;
    if (pRecord)
    {
        RecycleRecord(UnhookRecord(pRecord));
    }

    Assert(*pAnchor == NULL);

    // We might wish the optimisation here, recalculating
    // m_dwTimeToWake as a minimum of m_dwTimeToWake in all active
    // records. However this doesn't seem reasonable
    // because resources would seldom call this method,
    // and because extra tick wouldn't make noticeable harm.
}

//+-----------------------------------------------------------------------------
//  
//  Method:     CMilScheduleManager::Tick
//      
//  Synopsis:   Execute notifications for resources that scheduled reactivations
//
//------------------------------------------------------------------------------
void
CMilScheduleManager::Tick()
{
    m_dwCurrentTime = ::GetTickCount();
    m_dwTimeToWake = m_dwCurrentTime + INFINITE;

    // Release unused recycled records
    while (m_pRecycledRecords)
    {
        delete FetchRecycledRecord();
    }

    for(CMilScheduleRecord *pRecord = m_pActiveRecords; pRecord;)
    {
        CMilScheduleRecord *pNextRecord = pRecord->m_pNext;

        //
        // Detect if the requiested awake moment is current one,
        // or it is in the past already.
        // Time values may wrap due to overflow (though we need 49.7 days
        // to wait before it might happen).
        //
        if (int(pRecord->m_dwTimeToWake - m_dwCurrentTime) <= 0)
        {
            //
            // The time moment of interest happened already;
            // notify resource and remove record from the list.
            //
            CMilSlaveResource *pResource = pRecord->m_pResource;
            Assert(pResource);
            pResource->NotifyOnChanged(pResource);
            RecycleRecord(UnhookRecord(pRecord));
        }
        else
        {
            //
            // The time moment of interest not yet happened;
            // record will remain in active list, so we need
            // to update m_uTimeToWake that is minimum of
            // wake times of all the records in the list.
            //
            if ((pRecord->m_dwTimeToWake - m_dwCurrentTime) < (m_dwTimeToWake - m_dwCurrentTime))
            {
                m_dwTimeToWake = pRecord->m_dwTimeToWake;
            }
        }

        pRecord = pNextRecord;
    }
}

//+-----------------------------------------------------------------------------
//  
//  Method:     CMilScheduleManager::GetNextActivityTimeout     
//      
//  Synopsis:   Calculates time in milliseconds between current time and the 
//              time when next tick is needed.
//              Return INFINITE if next tick is not required at all.
//
//------------------------------------------------------------------------------
DWORD
CMilScheduleManager::GetNextActivityTimeout() const
{
    return m_dwTimeToWake - m_dwCurrentTime;
}

//+-----------------------------------------------------------------------------
//  
//  Method:     CMilScheduleManager::HookupRecord
//      
//  Synopsis:   Attach the record to the list and to client reference point
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE void
CMilScheduleManager::HookupRecord(
    CMilScheduleRecord* pRecord,
    CMilScheduleRecord** pAnchor
    )
{
    Assert(pRecord);
    Assert(pAnchor && *pAnchor == NULL);
    pRecord->m_pAnchor = pAnchor;
    *pAnchor = pRecord;

    pRecord->m_pBack = &m_pActiveRecords;
    pRecord->m_pNext = m_pActiveRecords;

    m_pActiveRecords = pRecord;
    if (pRecord->m_pNext)
    {
        Assert(pRecord->m_pNext->m_pBack == &m_pActiveRecords);
        pRecord->m_pNext->m_pBack = &pRecord->m_pNext;
    }
}

//+-----------------------------------------------------------------------------
//  
//  Method:     CMilScheduleManager::UnhookRecord
//      
//  Synopsis:   Detach the record from the list and client reference point
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CMilScheduleRecord*
CMilScheduleManager::UnhookRecord(CMilScheduleRecord *pRecord)
{
    Assert(pRecord->m_pAnchor && *pRecord->m_pAnchor == pRecord);
    *pRecord->m_pAnchor = NULL;

    Assert(pRecord->m_pBack && *pRecord->m_pBack == pRecord);
    *pRecord->m_pBack = pRecord->m_pNext;

    if (pRecord->m_pNext)
    {
        Assert(pRecord->m_pNext->m_pBack == &pRecord->m_pNext);
        pRecord->m_pNext->m_pBack = pRecord->m_pBack;
    }

    return pRecord;
}

//+-----------------------------------------------------------------------------
//  
//  Method:     CMilScheduleManager::GetFreeRecord
//      
//  Synopsis:   Either get the record from list of recycled ones
//              or allocate new record
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CMilScheduleRecord*
CMilScheduleManager::GetFreeRecord()
{
    if (m_pRecycledRecords)
    {
        return FetchRecycledRecord();
    }
    else
    {
        return new CMilScheduleRecord;
    }
}

//+-----------------------------------------------------------------------------
//  
//  Method:     CMilScheduleManager::RecycleRecord
//      
//  Synopsis:   Attach the record to the recycled list
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE void
CMilScheduleManager::RecycleRecord(CMilScheduleRecord* pRecord)
{
    Assert(pRecord);

    pRecord->m_pNext = m_pRecycledRecords;
    m_pRecycledRecords = pRecord;
}

//+-----------------------------------------------------------------------------
//  
//  Method:     CMilScheduleManager::FetchRecycledRecord
//      
//  Synopsis:   Detach the record from the recycled list.
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CMilScheduleRecord*
CMilScheduleManager::FetchRecycledRecord()
{
    Assert(m_pRecycledRecords);
    CMilScheduleRecord *pRecord =  m_pRecycledRecords;
    m_pRecycledRecords = pRecord->m_pNext;
    return pRecord;
}



