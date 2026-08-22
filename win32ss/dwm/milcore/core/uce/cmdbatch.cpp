// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

// 
// Module Name:
//
//          cmdbatch.cpp
//
// Abstract:
//
//    Implementations of the batch recorder.
//    This is just an api layer hook to serialize all the calls
//   into memory, it is not synchronized and relies on the caller
//    for correct synchronization.
//
//------------------------------------------------------------------------


#include "precomp.hpp"

MtDefine(CMilCommandBatch, MILRender, "CMilCommandBatch");

//------------------------------------------------------------------------
//
// Routine Description:
// 
//    CMilCommandBatch ctor
//
//------------------------------------------------------------------------

CMilCommandBatch::CMilCommandBatch()
{
    m_commandType = PartitionCommandBatch;
}

//------------------------------------------------------------------------
//
// Routine Description:
// 
//    CMilCommandBatch destructor
//
//------------------------------------------------------------------------

CMilCommandBatch::~CMilCommandBatch()
{
}

//------------------------------------------------------------------------
//
// Routine Description:
// 
//    CMilCommandBatch::Create
//
//------------------------------------------------------------------------

HRESULT
CMilCommandBatch::Create(UINT cbSize, __out CMilCommandBatch **ppBatch)
{
    HRESULT hr = S_OK;

    CMilCommandBatch *pFrame = new CMilCommandBatch;
    IFCOOM(pFrame);

    IFC(pFrame->Initialize(cbSize));

    *ppBatch = pFrame;
    pFrame = NULL;

Cleanup:
    delete pFrame;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// Routine Description:
// 
//    CMilCommandBatch::Create
//
//------------------------------------------------------------------------

HRESULT
CMilCommandBatch::Create(__out CMilCommandBatch **ppBatch)
{
    RRETURN(Create(0, ppBatch));
}


//------------------------------------------------------------------------
//
// Routine Description:
// 
// CMilCommandBatch::SetChannelPtr
//     This method is used to set and remove the command buffer's
//     channel association. The association holds a reference to
//     the channel and participates in controling the channels lifetime.
//     This reference on the channel indicates that a channel has a command batch
//     in the change queue. The reference is set in proc when flushing the channel,
//     and cross proc when the command buffer is submitted. It is removed after the
//     command batch has been processed by the compositor.
//
//------------------------------------------------------------------------

void
CMilCommandBatch::SetChannelPtr(CMilServerChannel *pChannel)
{
    if (m_pChannel)
    {
        ReleaseInterface(m_pChannel);
    }
    m_pChannel = pChannel;
    if (m_pChannel)
    {
        m_pChannel->AddRef();
    }
}



