/*
* PROJECT:     ReactOS Device Manager
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        dll/win32/devmgr/devmgr/node.cpp
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

CNode::~CNode()
{
}
