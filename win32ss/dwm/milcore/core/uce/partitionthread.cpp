// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Name: partitionthread.cpp
//
//  Description: Implementation of the Partition Thread class
//
//------------------------------------------------------------------------
#include "precomp.hpp"


//+-----------------------------------------------------------------------
//
//  Member: CPartitionThread::ThreadMain 
//
//  Synopsis:  Thread wrapper function for use by CreateThread
//
//  Returns:  Thread exit status
//
//------------------------------------------------------------------------
DWORD WINAPI CPartitionThread::ThreadMain(
    // SAL anotation for this probably should be
    //    __in_bcount(sizeof(CPartitionThread))  but prefast
    // claims it doesn't know what CPartitionThread is
    VOID *pv
    )
{
    //
    // Our rendering code is tested with single precision floating point
    // which is also the mode we like to run DX with.  So, we enforce
    // this mode here.
    //

    CFloatFPU oGuard;

    DWORD dwStatus = 0;
    CPartitionThread *pthread = (CPartitionThread *)pv;
    CPartitionManager *pm = pthread->m_pm;

    dwStatus = pthread->Run();

    // Notify the manager this thread has stopped.
    // Do it only after deleting CPartitionThread object,
    // otherwise we have a risk of memory leak report.
    pm->ThreadStopped(pthread);
    
    return dwStatus;
}

//+-----------------------------------------------------------------------
//
//  Member: CPartitionThread::~CPartitionThread 
//
//  Synopsis: Constructor
//
//------------------------------------------------------------------------
CPartitionThread::CPartitionThread(
    __in_ecount(1) CPartitionManager *pm, 
    int nPriority
    )
{
    m_hThread = 0;
    m_pm = pm;
    m_nPriority = nPriority;
}
//+-----------------------------------------------------------------------
//
//  Member: CPartitionThread::~CPartitionThread 
//
//  Synopsis: Destructor
//
//------------------------------------------------------------------------
CPartitionThread::~CPartitionThread()
{
    //
    // Thread handle is closed by PartitionManager::Shutdown 
    // method after thread exits.
    //
}

//+-----------------------------------------------------------------------
//
//  Member: CPartitionThread::StartThread
//
//  Synopsis:  Spawns the thread
//
//  Returns: S_OK if succeeds
//
//------------------------------------------------------------------------
HRESULT 
CPartitionThread::StartThread()
{
    HRESULT hr = S_OK;

    m_hThread = CreateThread(
        NULL,
        0,
        CPartitionThread::ThreadMain,
        this,
        0,
        &m_dwTid
        );
    
    IFCOOM(m_hThread);


    SetThreadPriority(m_hThread, m_nPriority);
  Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:
//      CPartitionThread::RenderPartition
//
//  Synopsis:
//      Perform composition pass on the given partition
//
//------------------------------------------------------------------------
void
CPartitionThread::RenderPartition(
    __in_ecount(1) Partition * pPartition
    )
{
    HRESULT hr = S_OK;
    bool presentThisPartition = false;
    MIL_THR(pPartition->Compose(&presentThisPartition));
    if (FAILED(hr))
    {
        //
        // Composition has failed. Put the partition into zombie state and
        // send out notification to channels that have registered within
        // this partition for receiving notifications.
        //

        m_pm->ZombifyPartitionAndCompleteProcessing(pPartition, hr);
    }
    else if (presentThisPartition)
    {
        //
        // If any partition requires present, we do one
        //

        m_pm->SchedulePresentAndCompleteProcessing(pPartition);
    }
    else
    {
        //
        // Not going to do a present for this frame, so we can flush the channels
        // notifying calling threads now (see comment below)
        //
        
        pPartition->FlushChannels();
        m_pm->CompleteProcessing(pPartition);
    }
}


//+-----------------------------------------------------------------------
//
//  Member:
//      CPartitionThread::PresentPartition
//
//  Synopsis:
//      Present any unpresented rendering
//
//------------------------------------------------------------------------
void
CPartitionThread::PresentPartition(
    __in_ecount(1) Partition * pPartition
    )
{
    HRESULT hr = S_OK;
    MIL_THR(pPartition->Present(m_pm));
    if (FAILED(hr))
    {
        //
        // Presentation has failed. Put the partition into zombie state and
        // send out notification to channels that have registered within
        // this partition for receiving notifications.
        //

        m_pm->ZombifyPartitionAndCompleteProcessing(pPartition, hr);
    }
    else
    {
        //
        // We're finished processing packets, rendering and presenting, so we can 
        // notify all the channels that asked for a sync flush that we are ready.
        // We aim to do this as early as possible because threads will block
        // waiting for this notification. 
        //
        pPartition->FlushChannels();        
    
        // Complete processing of this partition
        m_pm->CompleteProcessing(pPartition);
    }
}


//+-----------------------------------------------------------------------
//
//  Member: CPartitionThread::Run 
//
//  Synopsis:  Simple main function for a worker thread
//             derived class may override it with more 
//             elaborate scheduling
//
//  Returns: Thread exit status
//
//------------------------------------------------------------------------
DWORD CPartitionThread::Run()
{
    WorkType workType = WorkType_None;

    do
    {
        Partition *pPartition = NULL;

        workType = GetPartitionManager()->GetWork(&pPartition);

        Assert((pPartition != NULL) || (workType == WorkType_None));

        switch (workType) 
        {
        case WorkType_Render:
            RenderPartition(pPartition);
            break;

        case WorkType_Present:
            PresentPartition(pPartition);
            break;

        case WorkType_Zombie:
            m_pm->HandleZombiePartition(pPartition);
            break;
        }
    }
    while (workType != WorkType_None);

    return 0;
}

