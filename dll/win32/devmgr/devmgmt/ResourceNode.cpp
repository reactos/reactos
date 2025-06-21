/*
* PROJECT:     ReactOS Device Manager
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        dll/win32/devmgr/devmgmt/ResourceNode.cpp
* PURPOSE:     Class object for
* COPYRIGHT:   Copyright 2025 Eric Kohl <ekohl@reactos.org>
*
*/

#include "precomp.h"
#include "restypes.h"
#include "devmgmt.h"
#include "ResourceNode.h"


CResourceNode::CResourceNode(
    _In_ CDeviceNode *Node,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _In_ PSP_CLASSIMAGELIST_DATA ImageListData
    ) :
    CNode(ResourceNode, ImageListData)
{
    WCHAR szDetail[200];
    WCHAR szDescription[100];
    ULONG ulLength;

    m_DeviceId = Node->GetDeviceId();
    m_ClassImage = Node->GetClassImage();

    ulLength = sizeof(szDescription);
    CM_Get_DevNode_Registry_PropertyW(Node->GetDeviceInst(),
                                      CM_DRP_DEVICEDESC,
                                      NULL,
                                      szDescription,
                                      &ulLength,
                                      0);

    if (Descriptor->Type == CmResourceTypeInterrupt)
    {
        wsprintf(szDetail, L"(%s) 0x%08x (%d) %s",
                 (Descriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED) ? L"ISA" : L"PCI",
                 Descriptor->u.Interrupt.Level, Descriptor->u.Interrupt.Vector,
                 szDescription);
        StringCchCopyW(m_DisplayName, MAX_PATH, szDetail);
    }
    else if (Descriptor->Type == CmResourceTypePort)
    {
        wsprintf(szDetail, L"[%08lx - %08lx] %s",
                 Descriptor->u.Port.Start.LowPart, Descriptor->u.Port.Start.LowPart + Descriptor->u.Port.Length - 1,
                 szDescription);
        StringCchCopyW(m_DisplayName, MAX_PATH, szDetail);
    }
    else if (Descriptor->Type == CmResourceTypeMemory)
    {
        wsprintf(szDetail, L"[%08I64x - %08I64x] %s",
                 Descriptor->u.Memory.Start.QuadPart, Descriptor->u.Memory.Start.QuadPart + Descriptor->u.Memory.Length - 1,
                 szDescription);
        StringCchCopyW(m_DisplayName, MAX_PATH, szDetail);
    }
    else if (Descriptor->Type == CmResourceTypeDma)
    {
        wsprintf(szDetail, L"%08ld %s", Descriptor->u.Dma.Channel, szDescription);
        StringCchCopyW(m_DisplayName, MAX_PATH, szDetail);
    }
}


CResourceNode::~CResourceNode()
{
}


bool
CResourceNode::SetupNode()
{
    return true;
}
