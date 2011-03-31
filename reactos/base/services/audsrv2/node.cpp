//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    node.cpp
//
// Abstract:    
//      This is the implementation file for the C++ class that exposes
//      functionality of KS nodes
//

#include "kssample.h"

// ============================================================================
//
//   CKsNode
//
//=============================================================================


////////////////////////////////////////////////////////////////////////////////
//
//  CKsNode::CKsNode()
//
//  Routine Description:
//      Copy constructor for CKsNode
//  
//  Arguments: 
//      A Node to copy from,
//      An HRESULT to indicate success or failure
//
//
//  Return Value:
//  
//     S_OK on success
//


CKsNode::CKsNode
(
    CKsNode*      pksnCopy,
    HRESULT*        phr
) : m_nId(0), 
    m_ulCpuResources(KSAUDIO_CPU_RESOURCES_UNINITIALIZED),
    m_guidType(GUID_NULL)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    if (pksnCopy)
    {
        m_nId = pksnCopy->m_nId;
        m_ulCpuResources = pksnCopy->m_ulCpuResources;
        m_guidType = pksnCopy->m_guidType;
    }
    
    TRACE_LEAVE_HRESULT(hr);
    *phr = hr;
    return;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsNode::CKsNode()
//
//  Routine Description:
//      Constructor for CKsNode
//  
//  Arguments:
//      A node ID,
//      A type,
//      An HRESULT to indicate success or failure
//
//
//  Return Value:
//  
//     S_OK on success
//


CKsNode::CKsNode
(
    ULONG           nID,
    REFGUID         guidType,
    HRESULT*        phr
) : m_nId(nID), 
    m_ulCpuResources(KSAUDIO_CPU_RESOURCES_UNINITIALIZED),
    m_guidType(guidType)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    
    TRACE_LEAVE_HRESULT(hr);
    *phr = hr;
    return;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsNode::GetType()
//
//  Routine Description:
//      Returns the node type
//  
//  Arguments: 
//     None  
//
//
//  Return Value:
//  
//     The GUID representint the node type
//


GUID CKsNode::GetType()
{
    TRACE_ENTER();
    TRACE_LEAVE();
    return m_guidType;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsNode::GetId()
//
//  Routine Description:
//      Returns the node ID
//  
//  Arguments: 
//     None  
//
//
//  Return Value:
//  
//     The Node ID
//


ULONG CKsNode::GetId()
{
    TRACE_ENTER();
    TRACE_LEAVE();
    return m_nId;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsNode::GetCpuResources()
//
//  Routine Description:
//      Returns the CPU Resources
//  
//  Arguments: 
//     None  
//
//
//  Return Value:
//  
//     The CPU Resources
//


ULONG CKsNode::GetCpuResources()
{
    TRACE_ENTER();
    TRACE_LEAVE();
    return m_ulCpuResources;
}

