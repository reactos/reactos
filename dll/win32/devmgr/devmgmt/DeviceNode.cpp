/*
* PROJECT:     ReactOS Device Manager
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        dll/win32/devmgr/devmgmt/ClassNode.cpp
* PURPOSE:     Class object for
* COPYRIGHT:   Copyright 2015 Ged Murphy <gedmurphy@reactos.org>
*
*/

#include "precomp.h"
#include "devmgmt.h"
#include "DeviceNode.h"


CDeviceNode::CDeviceNode(
    _In_opt_ DEVINST Device,
    _In_ PSP_CLASSIMAGELIST_DATA ImageListData
    ) :
    CNode(DeviceNode, ImageListData),
    m_DevInst(Device),
    m_hDevInfo(NULL),
    m_Status(0),
    m_ProblemNumber(0),
    m_OverlayImage(0)
{
    ZeroMemory(&m_DevinfoData, sizeof(SP_DEVINFO_DATA));
}

CDeviceNode::CDeviceNode(
    _In_ const CDeviceNode &Node
    ) :
    CNode(Node)
{
    m_DevInst = Node.m_DevInst;
    m_hDevInfo = Node.m_hDevInfo;
    m_Status = Node.m_Status;
    m_ProblemNumber = Node.m_ProblemNumber;
    m_OverlayImage = Node.m_OverlayImage;
    CopyMemory(&m_DevinfoData, &Node.m_DevinfoData, sizeof(SP_DEVINFO_DATA));

    size_t size = wcslen(Node.m_DeviceId) + 1;
    m_DeviceId = new WCHAR[size];
    StringCbCopyW(m_DeviceId, size * sizeof(WCHAR), Node.m_DeviceId);
}

CDeviceNode::~CDeviceNode()
{
    Cleanup();
}

bool
CDeviceNode::SetupNode()
{
    WCHAR ClassGuidString[MAX_GUID_STRING_LEN];
    ULONG ulLength;
    CONFIGRET cr;

    // Get the length of the device id string
    cr = CM_Get_Device_ID_Size(&ulLength, m_DevInst, 0);
    if (cr == CR_SUCCESS)
    {
        // We alloc heap here because this will be stored in the lParam of the TV
        m_DeviceId = new WCHAR[ulLength + 1];

        // Now get the actual device id
        cr = CM_Get_Device_IDW(m_DevInst,
                               m_DeviceId,
                               ulLength + 1,
                               0);
        if (cr != CR_SUCCESS || wcscmp(m_DeviceId, L"HTREE\\ROOT\\0") == 0)
        {
            delete[] m_DeviceId;
            m_DeviceId = NULL;
        }
    }

    // Make sure we got the string
    if (m_DeviceId == NULL)
        return false;

    // Build up a handle a and devinfodata struct
    m_hDevInfo = SetupDiCreateDeviceInfoListExW(NULL,
                                                NULL,
                                                NULL,
                                                NULL);
    if (m_hDevInfo != INVALID_HANDLE_VALUE)
    {
        m_DevinfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        SetupDiOpenDeviceInfoW(m_hDevInfo,
                               m_DeviceId,
                               NULL,
                               0,
                               &m_DevinfoData);
    }

    // Check if the device has a problem
    if (HasProblem())
    {
        if (IsDisabled())
        {
            m_OverlayImage = OverlayDisabled;
        }
        else
        {
            m_OverlayImage = OverlayProblem;
        }
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
    ulLength = sizeof(m_DisplayName);
    cr = CM_Get_DevNode_Registry_PropertyW(m_DevInst,
                                           CM_DRP_FRIENDLYNAME,
                                           NULL,
                                           m_DisplayName,
                                           &ulLength,
                                           0);
    if (cr != CR_SUCCESS)
    {
        ulLength = sizeof(m_DisplayName);
        cr = CM_Get_DevNode_Registry_PropertyW(m_DevInst,
                                               CM_DRP_DEVICEDESC,
                                               NULL,
                                               m_DisplayName,
                                               &ulLength,
                                               0);
    }

    if (cr != CR_SUCCESS)
    {
        CAtlStringW str;
        if (str.LoadStringW(g_hThisInstance, IDS_UNKNOWNDEVICE))
            StringCchCopyW(m_DisplayName, MAX_PATH, str.GetBuffer());
    }

    return true;
}

bool
CDeviceNode::HasProblem()
{
    CONFIGRET cr;
    cr = CM_Get_DevNode_Status_Ex(&m_Status,
                                  &m_ProblemNumber,
                                  m_DevInst,
                                  0,
                                  NULL);
    if (cr == CR_SUCCESS)
    {
        return ((m_Status & (DN_HAS_PROBLEM | DN_PRIVATE_PROBLEM)) != 0);
    }

    return false;
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
        if (m_Status & DN_NO_SHOW_IN_DM)
            return true;
    }

    if (IsEqualGUID(*GetClassGuid(), GUID_DEVCLASS_LEGACYDRIVER) ||
        IsEqualGUID(*GetClassGuid(), GUID_DEVCLASS_VOLUME))
        return true;

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
        return ((m_ProblemNumber == CM_PROB_DISABLED) || (m_ProblemNumber == CM_PROB_HARDWARE_DISABLED));
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

bool
CDeviceNode::CanUninstall()
{
    CONFIGRET cr;
    cr = CM_Get_DevNode_Status_Ex(&m_Status,
                                  &m_ProblemNumber,
                                  m_DevInst,
                                  0,
                                  NULL);
    if (cr == CR_SUCCESS)
    {
        if ((m_Status & DN_ROOT_ENUMERATED) != 0 &&
            (m_Status & DN_DISABLEABLE) == 0)
                return false;
    }

    return true;
}

bool
CDeviceNode::EnableDevice(
    _In_ bool Enable,
    _Out_ bool &NeedsReboot
    )
{
    bool Canceled = false;

    SetFlags(DI_NODI_DEFAULTACTION, 0);

    SP_PROPCHANGE_PARAMS pcp;
    pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    pcp.StateChange = (Enable ? DICS_ENABLE : DICS_DISABLE);
    pcp.HwProfile = 0;

    // check both scopes to make sure we can make the change
    for (int i = 0; i < 2; i++)
    {
        // Check globally first, then check config specific
        pcp.Scope = (i == 0) ? DICS_FLAG_CONFIGGENERAL : DICS_FLAG_CONFIGSPECIFIC;

        if (SetupDiSetClassInstallParamsW(m_hDevInfo,
                                          &m_DevinfoData,
                                          &pcp.ClassInstallHeader,
                                          sizeof(SP_PROPCHANGE_PARAMS)))
        {
            SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
                                      m_hDevInfo,
                                      &m_DevinfoData);
        }

        if (GetLastError() == ERROR_CANCELLED)
        {
            Canceled = true;
            break;
        }
    }

    if (Canceled == false)
    {
        pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
        if (SetupDiSetClassInstallParamsW(m_hDevInfo,
                                          &m_DevinfoData,
                                          &pcp.ClassInstallHeader,
                                          sizeof(SP_PROPCHANGE_PARAMS)))
        {
            SetupDiChangeState(m_hDevInfo, &m_DevinfoData);
        }


        if (Enable)
        {
            // config specific enabling first, then global enabling.
            // The global appears to be the one that starts the device
            pcp.Scope = DICS_FLAG_GLOBAL;
            if (SetupDiSetClassInstallParamsW(m_hDevInfo,
                                              &m_DevinfoData,
                                              &pcp.ClassInstallHeader,
                                              sizeof(SP_PROPCHANGE_PARAMS)))
            {
                SetupDiChangeState(m_hDevInfo, &m_DevinfoData);
            }
        }

        SetFlags(DI_PROPERTIES_CHANGE, 0);

        NeedsReboot = ((GetFlags() & (DI_NEEDRESTART | DI_NEEDREBOOT)) != 0);
    }

    RemoveFlags(DI_NODI_DEFAULTACTION, 0);

    return true;
}

bool
CDeviceNode::UninstallDevice()
{
    if (CanUninstall() == false)
        return false;

    SP_REMOVEDEVICE_PARAMS RemoveDevParams;
    RemoveDevParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    RemoveDevParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
    RemoveDevParams.Scope = DI_REMOVEDEVICE_GLOBAL;
    RemoveDevParams.HwProfile = 0;

    //
    // We probably need to walk all the siblings of this
    // device and ask if they're happy with the uninstall
    //

    // Remove it
    SetupDiSetClassInstallParamsW(m_hDevInfo,
                                  &m_DevinfoData,
                                  &RemoveDevParams.ClassInstallHeader,
                                  sizeof(SP_REMOVEDEVICE_PARAMS));
    SetupDiCallClassInstaller(DIF_REMOVE, m_hDevInfo, &m_DevinfoData);

    // Clear the install params
    SetupDiSetClassInstallParamsW(m_hDevInfo,
                                  &m_DevinfoData,
                                  NULL,
                                  0);

    return true;
}

/* PRIVATE METHODS ******************************************************/

void
CDeviceNode::Cleanup()
{
    if (m_DeviceId)
    {
        delete[] m_DeviceId;
        m_DeviceId = NULL;
    }
    if (m_hDevInfo)
    {
        SetupDiDestroyDeviceInfoList(m_hDevInfo);
        m_hDevInfo = NULL;
    }
}

DWORD
CDeviceNode::GetFlags()
{
    SP_DEVINSTALL_PARAMS DevInstallParams;
    DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParamsW(m_hDevInfo,
                                       &m_DevinfoData,
                                       &DevInstallParams))
    {
        return DevInstallParams.Flags;
    }
    return 0;
}

bool
CDeviceNode::SetFlags(
    _In_ DWORD Flags,
    _In_ DWORD FlagsEx
    )
{
    SP_DEVINSTALL_PARAMS DevInstallParams;
    DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParamsW(m_hDevInfo,
                                       &m_DevinfoData,
                                       &DevInstallParams))
    {
        DevInstallParams.Flags |= Flags;
        DevInstallParams.FlagsEx |= FlagsEx;
        return (SetupDiSetDeviceInstallParamsW(m_hDevInfo,
                                               &m_DevinfoData,
                                               &DevInstallParams) != 0);
    }
    return false;
}

bool
CDeviceNode::RemoveFlags(
    _In_ DWORD Flags,
    _In_ DWORD FlagsEx
    )
{
    SP_DEVINSTALL_PARAMS DevInstallParams;
    DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParamsW(m_hDevInfo,
                                       &m_DevinfoData,
                                       &DevInstallParams))
    {
        DevInstallParams.Flags &= ~Flags;
        DevInstallParams.FlagsEx &= ~FlagsEx;
        return (SetupDiSetDeviceInstallParamsW(m_hDevInfo,
                                               &m_DevinfoData,
                                               &DevInstallParams) != 0);
    }
    return false;
}
