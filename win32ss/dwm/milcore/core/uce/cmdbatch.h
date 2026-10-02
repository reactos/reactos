// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

// 
//------------------------------------------------------------------------

MtExtern(CMilCommandBatch);

#define INITIAL_BATCH_SIZE 0x1000

class CMilSlaveHandleTable;
class CMilServerChannel;
interface IMilBatchDevice;

// MIL marshal type (related to the transport type)
enum PartitionCommandType
{
    PartitionCommandTypeInvalid = 0x0,
    PartitionCommandBatch,
    PartitionCommandOpenChannel,
    PartitionCommandCloseChannel
};

class CMilCommandBatch : public CMilDataStreamWriter
{

private :
    CMilCommandBatch();


   
public:
    static HRESULT Create(UINT cbSize, __out CMilCommandBatch **ppBatch);
    static HRESULT Create(__out CMilCommandBatch **ppBatch);

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilCommandBatch));

    ~CMilCommandBatch();



    // This method is used to set and remove the command buffer's
    // channel association. The association holds a reference to
    // the channel and participates in controling the channels lifetime.
    // This reference on the channel indicates that a channel has a command batch
    // in the change queue. The reference is set in proc when flushing the channel,
    // and cross proc when the command buffer is submitted. It is removed after the
    // command batch has been processed by the compositor.
    void SetChannelPtr(CMilServerChannel *pChannel);

    // sets the channel handle for use in the cross packet transport case.
    void SetChannel(HMIL_CHANNEL hChannel)
    {
        m_hChannel = hChannel;
    }

    CMilServerChannel *GetChannelPtr() { return m_pChannel;}
    
    HMIL_CHANNEL GetChannel() const
    {
        return m_hChannel;
    }

public:

    //
    // List entry used used when putting instances of this class in a list.
    // This field is used both for the device queue and for the free command
    // batch lookaside.
    //

    SLIST_ENTRY m_link;

    PartitionCommandType m_commandType;

    //
    // Free list head in the master handle table.
    // temporary methods until we spin of tables inside the channel
    // these methods are neded to control handle deletion.
    //

    UINT GetFreeIndex() const
    {
        return m_idxFree;
    }

    void SetFreeIndex(UINT idx)
    {
        m_idxFree = idx;
    }

private:

    // pointer to the channel this batch was sent on. During the composition pass, this channel
    // is used to retrieve corresponding handle tables and marshalling types.
    CMilServerChannel *m_pChannel;

    // when going over packet transports, this member holds the channel handle used
    // to route the command batch to its corresponding channel.
    HMIL_CHANNEL m_hChannel;

    UINT m_idxFree;
};

interface IMilBatchDevice :
    public IMILRefCount
{
    virtual HRESULT SubmitBatch(__in CMilCommandBatch *ppBatch) = 0;
};



