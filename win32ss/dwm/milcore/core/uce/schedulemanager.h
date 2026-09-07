// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//  

//
//  Description:
//      Contains CMilScheduleManager class declaration.
//
//      The instance of this class supposed to live in rendering thread
//      and serves slave resources. The resource can ask for additional
//      rendering cycle (at given time) in order to optimize the quality
//      of rendering, when it'll detect that animation has been stopped.
//

MtExtern(CMilScheduleManager);


class CMilScheduleManager;
class CMilScheduleRecord;
class CMilSlaveResource;

//+-----------------------------------------------------------------------------
//  
//  Class:      CMilScheduleManager
//
//  Synopsis:
//      The instance of this class supposed to live in rendering thread
//      and to serve slave resources. The resource can ask for additional
//      rendering cycle (at given time) in order to optimize the quality
//      of rendering, when it'll detect that animation has been stopped.
//
//      Usage pattern:
//
//      Resource instance should contain a pointer to CMilScheduleRecord,
//      say CMilScheduleRecord* m_pScheduleRecord that should be zeroed
//      in constructor.
//
//      Whenever the resource feels that it needs re-rasterization
//      at some time in the future, it should call
//      pScheduleManager->Schedule(this, &m_pScheduleRecord, uTimeToWake);
//
//      This will cause creating the schedule record that will be pointed
//      from m_pScheduleRecord. This in turn eventually will cause call to
//      NotifyOnChanged() at a given time.
//
//      To undo the effect of Scedule, call Unschedule(&m_pScheduleRecord).
//
//      It is allowed both to call Schedule() when record already exists, and
//      to call Unschedule() for already zeroed record pointer.
//
//      Resource destructor ultimately should call Unschedule() (for nonzero
//      pointer to record), otherwise we might get NotifyOnChanged() calls to
//      destroyed object.
//
//      The pointer to CMilScheduleManager is available by
//      ((CMilSlaveHandleTable*)GetHandleTable())->GetScheduleManager()
//------------------------------------------------------------------------------
class CMilScheduleManager
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMilScheduleManager));

    // Construction && destruction
    CMilScheduleManager();
    ~CMilScheduleManager();
    
    // scheduling && unscheduling
    HRESULT Schedule(CMilSlaveResource *pResource, CMilScheduleRecord **ppRecord, DWORD dwTimeToWake);

    HRESULT ScheduleRelative(CMilSlaveResource *pResource, CMilScheduleRecord **ppRecord, DWORD dwTimeDelta)
    {
        RRETURN(Schedule(pResource, ppRecord, m_dwCurrentTime + dwTimeDelta));
    }
    void Unschedule(CMilScheduleRecord **ppRecord);
    
    DWORD GetCurrentTime() const { return m_dwCurrentTime; }
    
    // rendering loop controls
    void Tick();
    DWORD GetNextActivityTimeout() const;

private:

    // record lists:
    // active records can be handled in arbitrary sequence,
    // so all the fields are valid in CMilScheduleRecord
    // instances held by m_pActiveRecords list.
    // In oppose, recycled lists are always handled sequentially
    // so only m_pNext is valid.
    CMilScheduleRecord* m_pActiveRecords;
    CMilScheduleRecord* m_pRecycledRecords;

    // times in milliseconds
    DWORD m_dwCurrentTime;
    DWORD m_dwTimeToWake;

private:
    void HookupRecord(CMilScheduleRecord* pRecord, CMilScheduleRecord** ppAnchor);
    static CMilScheduleRecord* UnhookRecord(CMilScheduleRecord* pRecord);

    CMilScheduleRecord* GetFreeRecord();
    void RecycleRecord(CMilScheduleRecord* pRecord);
    CMilScheduleRecord* FetchRecycledRecord();
};


