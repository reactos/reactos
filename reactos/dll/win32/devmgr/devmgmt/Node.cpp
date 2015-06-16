/*
* PROJECT:     ReactOS Device Manager
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        dll/win32/devmgr/devmgr/node.cpp
* PURPOSE:     Object for each device in the tree
* COPYRIGHT:   Copyright 2015 Ged Murphy <gedmurphy@reactos.org>
*
*/

#include "stdafx.h"
#include "devmgmt.h"
#include "Node.h"


/* PUBLIC METHODS *******************************************/

CNode::CNode(_In_ LPGUID ClassGuid,
             _In_ PSP_CLASSIMAGELIST_DATA ImageListData) :
    m_ImageListData(ImageListData),
    m_NodeType(NodeClass),
    m_DevInst(0),
    m_DeviceId(NULL),
    m_ClassImage(0),
    m_Status(0),
    m_ProblemNumber(0),
    m_OverlayImage(0)
{
    m_DisplayName[0] = UNICODE_NULL;
    CopyMemory(&m_ClassGuid, ClassGuid, sizeof(GUID));
}

CNode::CNode(_In_opt_ DEVINST Device,
             _In_ PSP_CLASSIMAGELIST_DATA ImageListData) :
    m_ImageListData(ImageListData),
    m_NodeType(NodeDevice),
    m_DevInst(Device),
    m_DeviceId(NULL),
    m_ClassImage(0),
    m_Status(0),
    m_ProblemNumber(0),
    m_OverlayImage(0)
{
    m_DisplayName[0] = UNICODE_NULL;
    CopyMemory(&m_ClassGuid, &GUID_NULL, sizeof(GUID));
}

CNode::~CNode()
{
    Cleanup();
}

bool
CNode::Setup()
{
    // TODO: Make this polymorphic

    if (m_NodeType == NodeClass)
    {
        return SetupClassNode();
    }
    else if (m_NodeType == NodeDevice)
    {
        return SetupDeviceNode();
    }

    return FALSE;
}

bool
CNode::HasProperties()
{
    return (m_DeviceId != NULL);
}

bool
CNode::IsHidden()
{
    return ((m_Status & DN_NO_SHOW_IN_DM) != 0);
}

bool
CNode::CanDisable()
{
    return (m_NodeType == NodeDevice && ((m_Status & DN_DISABLEABLE) != 0));
}

bool
CNode::IsDisabled()
{
    return ((m_ProblemNumber & (CM_PROB_DISABLED | CM_PROB_HARDWARE_DISABLED)) != 0);
}

bool
CNode::IsStarted()
{
    return ((m_Status & DN_STARTED) != 0);
}

bool
CNode::IsInstalled()
{
    return ((m_Status & DN_HAS_PROBLEM) != 0 ||
            (m_Status & (DN_DRIVER_LOADED | DN_STARTED)) != 0);
}


/* PRIVATE METHODS ******************************************/

bool
CNode::SetupClassNode()
{
    DWORD RequiredSize, Type, Size;
    DWORD Success;
    HKEY hKey;

    // Open the registry key for this class
    hKey = SetupDiOpenClassRegKeyExW(&m_ClassGuid,
                                     MAXIMUM_ALLOWED,
                                     DIOCR_INSTALLER,
                                     NULL,
                                     0);
    if (hKey != INVALID_HANDLE_VALUE)
    {
        Size = DISPLAY_NAME_LEN;
        Type = REG_SZ;

        // Lookup the class description (win7+)
        Success = RegQueryValueExW(hKey,
                                   L"ClassDesc",
                                   NULL,
                                   &Type,
                                   (LPBYTE)m_DisplayName,
                                   &Size);
        if (Success == ERROR_SUCCESS)
        {
            // Check if the string starts with an @
            if (m_DisplayName[0] == L'@')
            {
                // The description is located in a module resource
                Success = ConvertResourceDescriptorToString(m_DisplayName, DISPLAY_NAME_LEN);
            }
        }
        else if (Success == ERROR_FILE_NOT_FOUND)
        {
            // WinXP stores the description in the default value
            Success = RegQueryValueExW(hKey,
                                       NULL,
                                       NULL,
                                       &Type,
                                       (LPBYTE)m_DisplayName,
                                       &Size);
        }

        // Close the registry key
        RegCloseKey(hKey);
    }
    else
    {
        Success = GetLastError();
    }

    // Check if we failed to get the class description
    if (Success != ERROR_SUCCESS)
    {
        // Use the class name as the description
        RequiredSize = DISPLAY_NAME_LEN;
        (VOID)SetupDiClassNameFromGuidW(&m_ClassGuid,
                                        m_DisplayName,
                                        RequiredSize,
                                        &RequiredSize);
    }

    // Get the image index for this class
    (VOID)SetupDiGetClassImageIndex(m_ImageListData,
                                    &m_ClassGuid,
                                    &m_ClassImage);

    return true;
}

bool
CNode::SetupDeviceNode()
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

void
CNode::Cleanup()
{
    if (m_DeviceId)
    {
        HeapFree(GetProcessHeap(), 0, m_DeviceId);
        m_DeviceId = NULL;
    }
}

DWORD
CNode::ConvertResourceDescriptorToString(
    _Inout_z_ LPWSTR ResourceDescriptor,
    _In_ DWORD ResourceDescriptorSize
)
{
    WCHAR ModulePath[MAX_PATH];
    WCHAR ResString[256];
    INT ResourceId;
    HMODULE hModule;
    LPWSTR ptr;
    DWORD Size;
    DWORD dwError;


    // First check for a semi colon */
    ptr = wcschr(ResourceDescriptor, L';');
    if (ptr)
    {
        // This must be an inf based descriptor, the desc is after the semi colon
        wcscpy_s(ResourceDescriptor, ResourceDescriptorSize, ++ptr);
        dwError = ERROR_SUCCESS;
    }
    else
    {
        // This must be a dll resource based descriptor. Find the comma
        ptr = wcschr(ResourceDescriptor, L',');
        if (ptr == NULL) return ERROR_INVALID_DATA;

        // Terminate the string where the comma was
        *ptr = UNICODE_NULL;

        // Expand any environment strings
        Size = ExpandEnvironmentStringsW(&ResourceDescriptor[1], ModulePath, MAX_PATH);
        if (Size > MAX_PATH) return ERROR_BUFFER_OVERFLOW;
        if (Size == 0) return GetLastError();

        // Put the comma back and move past it
        *ptr = L',';
        ptr++;

        // Load the dll
        hModule = LoadLibraryExW(ModulePath, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (hModule == NULL) return GetLastError();

        // Convert the resource id to a number
        ResourceId = _wtoi(ptr);

        // If the number is negative, make it positive
        if (ResourceId < 0) ResourceId = -ResourceId;

        // Load the string from the dll
        if (LoadStringW(hModule, ResourceId, ResString, 256))
        {
            wcscpy_s(ResourceDescriptor, ResourceDescriptorSize, ResString);
            dwError = ERROR_SUCCESS;
        }
        else
        {
            dwError = GetLastError();
        }

        // Free the library
        FreeLibrary(hModule);
    }

    return dwError;
}
