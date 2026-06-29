/*
* PROJECT:     ReactOS Device Manager
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        dll/win32/devmgr/devmgmt/ResourceTypeNode.cpp
* PURPOSE:     Class object for
* COPYRIGHT:   Copyright 2025 Eric Kohl <ekohl@reactos.org>
*
*/

#include "precomp.h"
#include "devmgmt.h"
#include "ResourceTypeNode.h"


CResourceTypeNode::CResourceTypeNode(
    _In_ UINT ResId,
    _In_ PSP_CLASSIMAGELIST_DATA ImageListData
    ) :
    CNode(ResourceTypeNode, ImageListData)
{
    CAtlStringW str;
    if (str.LoadStringW(g_hThisInstance, ResId))
        StringCchCopyW(m_DisplayName, MAX_PATH, str.GetBuffer());
}


CResourceTypeNode::~CResourceTypeNode()
{
}


bool
CResourceTypeNode::SetupNode()
{
    return true;
}
