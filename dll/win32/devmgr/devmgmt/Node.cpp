/*
* PROJECT:     ReactOS Device Manager
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        dll/win32/devmgr/devmgmt/node.cpp
* PURPOSE:     Abstract base object for each node in the tree
* COPYRIGHT:   Copyright 2015 Ged Murphy <gedmurphy@reactos.org>
*
*/

#include "precomp.h"
#include "devmgmt.h"
#include "Node.h"


/* PUBLIC METHODS *******************************************/

CNode::CNode(_In_ NodeType Type,
             _In_ PSP_CLASSIMAGELIST_DATA ImageListData) :
    m_NodeType(Type),
    m_ImageListData(ImageListData),
    m_DeviceId(NULL),
    m_ClassImage(0)
{
    m_DisplayName[0] = UNICODE_NULL;
    m_ClassGuid = GUID_NULL;
}

CNode::CNode(const CNode &Node)
{
    m_NodeType = Node.m_NodeType;
    m_ImageListData = Node.m_ImageListData;
    m_DeviceId = Node.m_DeviceId;
    m_ClassImage = Node.m_ClassImage;

    StringCbCopyW(m_DisplayName, sizeof(m_DisplayName), Node.m_DisplayName);
    CopyMemory(&m_ClassGuid, &Node.m_ClassGuid, sizeof(GUID));
}

CNode::~CNode()
{
}
