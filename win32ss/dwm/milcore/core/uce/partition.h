// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Abstract:
//      Definitions for the partition and partition state.
//
//------------------------------------------------------------------------------

#pragma once

//
// Forward declarations
//

class CPartitionManager;
class CMilCommandBatch;

enum PartitionState
{
    PartitionStateNull = 0x00,

    //
    // The partition has work queued to it and needs
    // composition pass.
    //
    PartitionNeedsBatchProcessing = 0x01,
    
    //
    // The partition has been requested for
    // composition pass.
    //
    PartitionNeedsCompositionPass = 0x02,
    
    //
    // This flag results from either
    // PartitionNeedsBatchProcessing or
    // PartitionNeedsCompositionPass.
    // Only partitions with PartitionNeedsRender flag are
    // really serviced with composition pass.
    //
    PartitionNeedsRender = 0x04,
    
    //
    // Partition has been rendered but the HWND targets have not 
    // yet been presented.
    //
    PartitionNeedsPresent = 0x08,
    
    //
    // Partition currently is included in manager's list.
    //
    PartitionIsEnqueued = 0x10,
    
    //
    // Partition currently is handled by worker thread.
    //
    PartitionIsBeingProcessed = 0x20,
    
    //
    // Partition is in zombie state. It will never process batches, render
    // or present again. This is a result of unhandled composition errors.
    //
    PartitionIsZombie = 0x40,

    //
    // Partition needs to report about getting in zombie state.
    // This flag cam only appear when PartitionIsZombie is set.
    //
    PartitionNeedsZombieNotification = 0x80,


    // ======================== Flags groups ===========================

    //
    // If any of bits of PartitionNeedsAttention is set then
    // the partitions requires CPartitionManager's attention
    // and should be referenced from CPartitionManager::m_PartitionList.
    //
    PartitionNeedsAttention
        = PartitionNeedsBatchProcessing
        | PartitionNeedsCompositionPass
        | PartitionNeedsRender
        | PartitionNeedsPresent
        | PartitionNeedsZombieNotification
        | PartitionIsBeingProcessed,


    //
    // These flags are to be cleared on composition pass.
    //
    PartitionRenderClearFlags
        = PartitionNeedsBatchProcessing
        | PartitionNeedsCompositionPass
        | PartitionNeedsRender,

    //
    // These flags are to be cleared when entering zombie state.
    //
    PartitionZombifyClearFlags
        = PartitionNeedsBatchProcessing
        | PartitionNeedsCompositionPass
        | PartitionNeedsRender
        | PartitionNeedsPresent
        | PartitionIsBeingProcessed,

    //
    // These flags are to be set when entering zombie state.
    //
    PartitionZombifySetFlags
        = PartitionIsZombie
        | PartitionNeedsZombieNotification,
};


//
// A class must inherit from this base class in order to have a thread assigned 
// from the partition manager. 
//

class __declspec(novtable) Partition : 
    public IMILRefCount,
    public LIST_ENTRY
{
public:

    Partition() 
    {
        Flink = NULL; 
        Blink = NULL; 
        m_state = PartitionStateNull; 
        m_hrZombieNotificationFailureReason = S_OK;
    }

    virtual ~Partition() {}

    virtual HRESULT Compose(
        __out_ecount(1) bool *pfNeedsPresent
        ) = 0;

    virtual HRESULT WaitForVBlank() = 0;
    
    virtual HRESULT Present(
        __in_ecount(1) CPartitionManager* pPartitionManager
        ) = 0;

    virtual void FlushChannels(
        bool fForceAllChannels = false
        ) = 0;

    // Notifies the UI thread that a partition has been zombied.
    virtual HRESULT NotifyPartitionIsZombie() = 0;

    // Enqueue the batch for processing by worker thread.
    virtual void EnqueueBatch(
        __inout_ecount(1) CMilCommandBatch *pBatch
        ) = 0;

    virtual MilCompositionDeviceState::Enum GetCompositionDeviceState() = 0;

    // Returns true if the partition is in zombie state.
    bool IsZombie() const
    {
        return (m_state & PartitionIsZombie) != 0;
    }

protected:
    // Returns true if the partition is currently handled by worker thread.
    bool IsBeingProcessed() const
    {
        return (m_state & PartitionIsBeingProcessed) != 0;
    }

    // Returns true if the partition has work queued to it.
    bool NeedsBatchProcessing() const
    {
        return (m_state & PartitionNeedsBatchProcessing) != 0;
    }

    // Returns true if the partition has deferred request for composition pass.
    bool NeedsCompositionPass() const
    {
        return (m_state & PartitionNeedsCompositionPass) != 0;
    }
    
    // Returns true if the partition has confirmed request for composition pass.
    bool NeedsRender() const
    {
        return (m_state & PartitionNeedsRender) != 0;
    }

    // Returns true if the partition has been rendered and needs to be presented.
    bool NeedsPresent() const
    {
        return (m_state & PartitionNeedsPresent) != 0;
    }
    
    // Returns true if the partition needs NotifyPartitionIsZombie().
    bool NeedsZombieNotification() const
    {
        return (m_state & PartitionNeedsZombieNotification) != 0;
    }

    // Returns true if the partition needs to be in partition manager list
    bool NeedsAttention() const
    {
        return (m_state & PartitionNeedsAttention) != 0;
    }

    // Returns true if the partition needs to be in partition manager list
    bool IsEnqueued() const 
    {
        return (m_state & PartitionIsEnqueued) != 0;
    }

    // Returns true if any of given flags are set
    bool HasAnyFlag(PartitionState flags) const
    {
        return (m_state & flags) != 0;
    }

    HRESULT m_hrZombieNotificationFailureReason;

private:
    friend class CPartitionManager;

    //
    // PartitionState m_state is controlled exclusuvely by CPartitionManager.
    //
    void SetStateFlags(PartitionState flags);

    void ClearStateFlags(PartitionState flags);

private:
    PartitionState m_state;
};


