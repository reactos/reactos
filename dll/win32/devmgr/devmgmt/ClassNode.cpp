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
#include "ClassNode.h"


CClassNode::CClassNode(
    _In_ LPGUID ClassGuid,
    _In_ PSP_CLASSIMAGELIST_DATA ImageListData
    ) :
    CNode(ClassNode, ImageListData)
{
    CopyMemory(&m_ClassGuid, ClassGuid, sizeof(GUID));
}


CClassNode::~CClassNode()
{
}


bool
CClassNode::SetupNode()
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
        // Lookup the class description (win7+)
        Size = sizeof(m_DisplayName);
        Success = RegQueryValueExW(hKey,
                                   L"ClassDesc",
                                   NULL,
                                   &Type,
                                   (LPBYTE)m_DisplayName,
                                   &Size);
        if (Success == ERROR_SUCCESS)
        {
            if (Type != REG_SZ)
            {
                Success = ERROR_INVALID_DATA;
            }
            // Check if the string starts with an @
            else if (m_DisplayName[0] == L'@')
            {
                // The description is located in a module resource
                Success = ConvertResourceDescriptorToString(m_DisplayName, sizeof(m_DisplayName));
            }
        }
        else if (Success == ERROR_FILE_NOT_FOUND)
        {
            // WinXP stores the description in the default value
            Size = sizeof(m_DisplayName);
            Success = RegQueryValueExW(hKey,
                                       NULL,
                                       NULL,
                                       &Type,
                                       (LPBYTE)m_DisplayName,
                                       &Size);
            if (Success == ERROR_SUCCESS && Type != REG_SZ)
            {
                Success = ERROR_INVALID_DATA;
            }
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
        RequiredSize = _countof(m_DisplayName);
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


DWORD
CClassNode::ConvertResourceDescriptorToString(
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
        StringCbCopyW(ResourceDescriptor, ResourceDescriptorSize, ++ptr);
        dwError = ERROR_SUCCESS;
    }
    else
    {
        // This must be a dll resource based descriptor. Find the comma
        ptr = wcschr(ResourceDescriptor, L',');
        if (ptr == NULL)
            return ERROR_INVALID_DATA;

        // Terminate the string where the comma was
        *ptr = UNICODE_NULL;

        // Expand any environment strings
        Size = ExpandEnvironmentStringsW(&ResourceDescriptor[1], ModulePath, MAX_PATH);
        if (Size > MAX_PATH)
            return ERROR_BUFFER_OVERFLOW;
        if (Size == 0)
            return GetLastError();

        // Put the comma back and move past it
        *ptr = L',';
        ptr++;

        // Load the dll
        hModule = LoadLibraryExW(ModulePath, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (hModule == NULL)
            return GetLastError();

        // Convert the resource id to a number
        ResourceId = _wtoi(ptr);

        // If the number is negative, make it positive
        if (ResourceId < 0)
            ResourceId = -ResourceId;

        // Load the string from the dll
        if (LoadStringW(hModule, ResourceId, ResString, 256))
        {
            StringCbCopyW(ResourceDescriptor, ResourceDescriptorSize, ResString);
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
