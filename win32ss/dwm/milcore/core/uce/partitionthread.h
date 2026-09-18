// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Name: partitionthread.h
//
//  Description: Partition thread definition
//
//------------------------------------------------------------------------
class Partition;
class CPartitionManager;
class CPartitionSchedule;
//+-----------------------------------------------------------------------
//
//  Class: CPartitionThread
//
//  Synopsis:  Worker thread for the partition manager
//
//------------------------------------------------------------------------
class CPartitionThread
{
public:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CPartitionManager));

    CPartitionThread(
        __in_ecount(1) CPartitionManager *pm, 
        int nPriority
        );
    
    virtual HRESULT Initialize()
    {
        return S_OK;
    }

    virtual ~CPartitionThread();    

    virtual HRESULT GetComposedEventId(
        __out_ecount(1) UINT *pcEventId
        )
    {
        return S_OK;
    }

    // starts the thread 
    HRESULT StartThread();
    
    inline HANDLE GetHandle()
    {
        return m_hThread;
    }

    DWORD GetThreadId() const
    {
        return m_dwTid;
    }

protected:
    virtual DWORD Run();

    // Compose and render everything required but not present
    void RenderPartition(
        __in_ecount(1) Partition * pPartition
        );

    // Present the last composed frame
    void PresentPartition(
        __in_ecount(1) Partition * pPartition
        );

    inline CPartitionManager *GetPartitionManager()
    {
        return m_pm;
    }

private:
    //
    // Prevent accidental assignment
    //
    
    CPartitionThread(CPartitionThread &);
    CPartitionThread &operator=(const CPartitionThread &);

    //
    // The worker thread entry function.
    //
    static DWORD WINAPI ThreadMain(
        // SAL anotation for this probably should be
        //    __in_bcount(sizeof(CPartitionThread))  but prefast
        // claims it doesn't know what CPartitionThread is
        VOID *pv
        );

    //
    // Data
    //

    HANDLE m_hThread;
    DWORD  m_dwTid;
    int m_nPriority;
    CPartitionManager *m_pm;
};


