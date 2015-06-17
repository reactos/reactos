/*
* PROJECT:     ReactOS Device Manager
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        dll/win32/devmgr/devmgr/ClassNode.cpp
* PURPOSE:     Class object for
* COPYRIGHT:   Copyright 2015 Ged Murphy <gedmurphy@reactos.org>
*
*/

#include "stdafx.h"
#include "devmgmt.h"
#include "DeviceNode.h"


CDeviceNode::CDeviceNode(
    _In_opt_ DEVINST Device,
    _In_ PSP_CLASSIMAGELIST_DATA ImageListData
    ) :
    CNode(ImageListData),
    m_DevInst(Device),
    m_Status(0),
    m_ProblemNumber(0),
    m_OverlayImage(0)
{
}

CDeviceNode::~CDeviceNode()
{
}

bool
CDeviceNode::SetupNode()
{
    WCHAR ClassGuidString[MAX_GUID_STRING_LEN];
    ULONG ulLength;
    CONFIGRET cr;

    //    ATLASSERT(m_DeviceId == NULL);

    // Get the length of the device id string
    cr = CM_Get_Device_ID_Size(&ulLength, m_DevInst, 0);
    if (cr == CR_SUCCESS)
    {
        // We alloc heap here because this will be stored in the lParam of the TV
        m_DeviceId = (LPWSTR)HeapAlloc(GetProcessHeap(),
                                       0,
                                       (ulLength + 1) * sizeof(WCHAR));
        if (m_DeviceId)
        {
            // Now get the actual device id
            cr = CM_Get_Device_IDW(m_DevInst,
                                   m_DeviceId,
                                   ulLength + 1,
                                   0);
            if (cr != CR_SUCCESS)
            {
                HeapFree(GetProcessHeap(), 0, m_DeviceId);
                m_DeviceId = NULL;
            }
        }
    }

    // Make sure we got the string
    if (m_DeviceId == NULL)
        return false;

    // Get the current status of the device
    cr = CM_Get_DevNode_Status_Ex(&m_Status,
                                  &m_ProblemNumber,
                                  m_DevInst,
                                  0,
                                  NULL);
    if (cr != CR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, m_DeviceId);
        m_DeviceId = NULL;
        return false;
    }

    // Check if the device has a problem
    if (m_Status & DN_HAS_PROBLEM)
    {
        m_OverlayImage = 1;
    }

    // The disabled overlay takes precidence over the problem overlay
    if (m_ProblemNumber & (CM_PROB_DISABLED | CM_PROB_HARDWARE_DISABLED))
    {
        m_OverlayImage = 2;
    }


    // Get the class guid for this device
    ulLength = MAX_GUID_STRING_LEN * sizeof(WCHAR);
    cr = CM_Get_DevNode_Registry_PropertyW(m_DevInst,
                                           CM_DRP_CLASSGUID,
                                           NULL,
                                           ClassGuidString,
                                           &ulLength,
                                           0);
    if (cr == CR_SUCCESS)
    {
        // Convert the string to a proper guid
        CLSIDFromString(ClassGuidString, &m_ClassGuid);
    }
    else
    {
        // It's a device with no driver
        m_ClassGuid = GUID_DEVCLASS_UNKNOWN;
    }


    // Get the image for the class this device is in
    SetupDiGetClassImageIndex(m_ImageListData,
                              &m_ClassGuid,
                              &m_ClassImage);

    // Get the description for the device
    ulLength = DISPLAY_NAME_LEN * sizeof(WCHAR);
    cr = CM_Get_DevNode_Registry_PropertyW(m_DevInst,
                                           CM_DRP_FRIENDLYNAME,
                                           NULL,
                                           m_DisplayName,
                                           &ulLength,
                                           0);
    if (cr != CR_SUCCESS)
    {
        ulLength = DISPLAY_NAME_LEN * sizeof(WCHAR);
        cr = CM_Get_DevNode_Registry_PropertyW(m_DevInst,
                                               CM_DRP_DEVICEDESC,
                                               NULL,
                                               m_DisplayName,
                                               &ulLength,
                                               0);

    }

    // Cleanup if something failed
    if (cr != CR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, m_DeviceId);
        m_DeviceId = NULL;
    }

    return (cr == CR_SUCCESS ? true : false);
}


bool
CDeviceNode::IsHidden()
{
    CONFIGRET cr;
    cr = CM_Get_DevNode_Status_Ex(&m_Status,
                                  &m_ProblemNumber,
                                  m_DevInst,
                                  0,
                                  NULL);
    if (cr == CR_SUCCESS)
    {
        return ((m_Status & DN_NO_SHOW_IN_DM) != 0);
    }

    return false;
}

bool
CDeviceNode::CanDisable()
{
    CONFIGRET cr;
    cr = CM_Get_DevNode_Status_Ex(&m_Status,
                                  &m_ProblemNumber,
                                  m_DevInst,
                                  0,
                                  NULL);
    if (cr == CR_SUCCESS)
    {
        return ((m_Status & DN_DISABLEABLE) != 0);
    }

    return false;
}

bool
CDeviceNode::IsDisabled()
{
    CONFIGRET cr;
    cr = CM_Get_DevNode_Status_Ex(&m_Status,
                                  &m_ProblemNumber,
                                  m_DevInst,
                                  0,
                                  NULL);
    if (cr == CR_SUCCESS)
    {
        return ((m_ProblemNumber & (CM_PROB_DISABLED | CM_PROB_HARDWARE_DISABLED)) != 0);
    }

    return false;
}

bool
CDeviceNode::IsStarted()
{
    CONFIGRET cr;
    cr = CM_Get_DevNode_Status_Ex(&m_Status,
                                  &m_ProblemNumber,
                                  m_DevInst,
                                  0,
                                  NULL);
    if (cr == CR_SUCCESS)
    {
        return ((m_Status & DN_STARTED) != 0);
    }

    return false;
}

bool
CDeviceNode::IsInstalled()
{
    CONFIGRET cr;
    cr = CM_Get_DevNode_Status_Ex(&m_Status,
                                  &m_ProblemNumber,
                                  m_DevInst,
                                  0,
                                  NULL);
    if (cr == CR_SUCCESS)
    {
        return ((m_Status & DN_HAS_PROBLEM) != 0 ||
                (m_Status & (DN_DRIVER_LOADED | DN_STARTED)) != 0);
    }

    return false;
}