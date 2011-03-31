//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    node.h
//
// Abstract:    
//      This is the header file for the C++ class that exposes
//      functionality of KS nodes
//

#pragma once
#ifndef __KSNODE_H
#define __KSNODE_H

// Reserved node identifiers
#define NODE_UNINITIALIZED  0xFFFFFFFF
#define NODE_WILDCARD       0xFFFFFFFE
#define KSAUDIO_CPU_RESOURCES_UNINITIALIZED 'ENON'

////////////////////////////////////////////////////////////////////////////////
//
//  class CKsNode
//
//  Class Description:
//      This is the base class for classes that proxy Ks filters from user mode.
//      This class wraps an irptarget and associated node id.  It simplifies property
//      calls on nodes.
//
//
//

class CKsNode
{
public:
    CKsNode(ULONG nID, REFGUID guidType, HRESULT *phr);
    CKsNode(CKsNode* pksnCopy, HRESULT *phr);
    GUID GetType();
    ULONG GetId();
    ULONG GetCpuResources();

protected:

private:
    ULONG           m_nId;
    ULONG           m_ulCpuResources;

    GUID            m_guidType;
};

typedef CKsNode *               PCKsNode;

#endif //__KSNODE_H
