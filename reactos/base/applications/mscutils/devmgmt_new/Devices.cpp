/*
* PROJECT:     ReactOS Device Manager
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        base/applications/mscutils/devmgmt/devices.cpp
* PURPOSE:     Wrapper around setupapi functions
* COPYRIGHT:   Copyright 2014 Ged Murphy <gedmurphy@gmail.com>
*
*/

#include "stdafx.h"
#include "devmgmt.h"
#include "Devices.h"


/* PUBLIC METHODS *****************************************/

CDevices::CDevices(void) :
    m_bInitialized(FALSE),
    m_RootImageIndex(-1)
{
    ZeroMemory(&m_ImageListData, sizeof(SP_CLASSIMAGELIST_DATA));

    m_RootName[0] = UNICODE_NULL;
}

CDevices::~CDevices(void)
{
    ATLASSERT(m_bInitialized == FALSE);
}

BOOL
CDevices::Initialize()
{
    BOOL bSuccess;

    ATLASSERT(m_bInitialized == FALSE);

    /* Get the device image list */
    m_ImageListData.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);
    bSuccess = SetupDiGetClassImageList(&m_ImageListData);
    if (bSuccess == FALSE) return FALSE;

    bSuccess = CreateRootDevice();
    if (bSuccess) m_bInitialized = TRUE;

    return m_bInitialized;
}

BOOL
CDevices::Uninitialize()
{
    if (m_ImageListData.ImageList != NULL)
    {
        SetupDiDestroyClassImageList(&m_ImageListData);
        ZeroMemory(&m_ImageListData, sizeof(SP_CLASSIMAGELIST_DATA));
    }

    m_bInitialized = FALSE;

    return TRUE;
}

BOOL
CDevices::GetDeviceTreeRoot(
    _Out_ LPWSTR RootName,
    _In_ DWORD RootNameSize,
    _Out_ PINT RootImageIndex
    )
{
    wcscpy_s(RootName, RootNameSize, m_RootName);
    *RootImageIndex = m_RootImageIndex;

    return TRUE;
}

BOOL
CDevices::GetChildDevice(
    _In_ DEVINST ParentDevInst,
    _Out_ PDEVINST DevInst
    )
{
    CONFIGRET cr;

    cr = CM_Get_Child(DevInst,
                      ParentDevInst,
                      0);
    return (cr == CR_SUCCESS);
}

BOOL
CDevices::GetSiblingDevice(
    _In_ DEVINST PrevDevice,
    _Out_ PDEVINST DevInst
    )
{
    CONFIGRET cr;

    cr = CM_Get_Sibling(DevInst,
                        PrevDevice,
                        0);
    return (cr == CR_SUCCESS);
}

BOOL
CDevices::GetDeviceStatus(
    _In_ LPWSTR DeviceId,
    _Out_ PULONG Status,
    _Out_ PULONG ProblemNumber
    )
{
    DEVINST DeviceInstance;
    CONFIGRET cr;

    *Status = 0;
    *ProblemNumber = 0;

    /* Use the device id string to lookup the instance */
    cr = CM_Locate_DevNodeW(&DeviceInstance,
                            DeviceId,
                            CM_LOCATE_DEVNODE_NORMAL);
    if (cr == CR_SUCCESS)
    {
        /* Get the status of this device */
        cr = CM_Get_DevNode_Status_Ex(Status,
                                      ProblemNumber,
                                      DeviceInstance,
                                      0,
                                      NULL);
    }

    return (cr == CR_SUCCESS) ? TRUE : FALSE;
}

BOOL
CDevices::GetDevice(
    _In_ DEVINST Device,
    _Out_writes_(DeviceNameSize) LPWSTR DeviceName,
    _In_ DWORD DeviceNameSize,
    _Outptr_ LPWSTR *DeviceId,
    _Out_ PINT ClassImage,
    _Out_ PULONG Status,
    _Out_ PULONG ProblemNumber
    )
{
    WCHAR ClassGuidString[MAX_GUID_STRING_LEN];
    GUID ClassGuid;
    ULONG ulLength;
    CONFIGRET cr;
    BOOL bSuccess;

    *DeviceId = NULL;

    /* Get the length of the device id string */
    cr = CM_Get_Device_ID_Size(&ulLength, Device, 0);
    if (cr == CR_SUCCESS)
    {
        /* We alloc heap here because this will be stored in the lParam of the TV */
        *DeviceId = (LPWSTR)HeapAlloc(GetProcessHeap(),
                                      0,
                                      (ulLength + 1) * sizeof(WCHAR));
        if (*DeviceId)
        {
            /* Now get the actual device id */
            cr = CM_Get_Device_IDW(Device,
                                   *DeviceId,
                                   ulLength + 1,
                                   0);
            if (cr != CR_SUCCESS)
            {
                HeapFree(GetProcessHeap(), 0, *DeviceId);
                *DeviceId = NULL;
            }
        }
    }

    /* Make sure we got the string */
    if (*DeviceId == NULL)
        return FALSE;


    /* Get the current status of the device */
    bSuccess = GetDeviceStatus(*DeviceId, Status, ProblemNumber);
    if (bSuccess == FALSE)
    {
        HeapFree(GetProcessHeap(), 0, *DeviceId);
        *DeviceId = NULL;
        return FALSE;
    }

    /* Get the class guid for this device */
    ulLength = MAX_GUID_STRING_LEN * sizeof(WCHAR);
    cr = CM_Get_DevNode_Registry_PropertyW(Device,
                                           CM_DRP_CLASSGUID,
                                           NULL,
                                           ClassGuidString,
                                           &ulLength,
                                           0);
    if (cr == CR_SUCCESS)
    {
        /* Convert the string to a proper guid */
        CLSIDFromString(ClassGuidString, &ClassGuid);
    }
    else
    {
        /* It's a device with no driver */
        ClassGuid = GUID_DEVCLASS_UNKNOWN;
    }


    /* Get the image for the class this device is in */
    SetupDiGetClassImageIndex(&m_ImageListData,
                              &ClassGuid,
                              ClassImage);

    /* Get the description for the device */
    ulLength = DeviceNameSize * sizeof(WCHAR);
    cr = CM_Get_DevNode_Registry_PropertyW(Device,
                                          CM_DRP_FRIENDLYNAME,
                                          NULL,
                                          DeviceName,
                                          &ulLength,
                                          0);
    if (cr != CR_SUCCESS)
    {
        ulLength = DeviceNameSize * sizeof(WCHAR);
        cr = CM_Get_DevNode_Registry_PropertyW(Device,
                                              CM_DRP_DEVICEDESC,
                                              NULL,
                                              DeviceName,
                                              &ulLength,
                                              0);

    }

    /* Cleanup if something failed */
    if (cr != CR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, *DeviceId);
        *DeviceId = NULL;
    }

    return (cr == CR_SUCCESS ? TRUE : FALSE);
}

BOOL
CDevices::EnumClasses(
    _In_ ULONG ClassIndex,
    _Out_writes_bytes_(sizeof(GUID)) LPGUID ClassGuid,
    _Out_writes_(ClassNameSize) LPWSTR ClassName,
    _In_ DWORD ClassNameSize,
    _Out_writes_(ClassDescSize) LPWSTR ClassDesc,
    _In_ DWORD ClassDescSize,
    _Out_ PINT ClassImage
    )
{
    DWORD RequiredSize, Type, Size;
    CONFIGRET cr;
    DWORD Success;
    HKEY hKey;

    ClassName[0] = UNICODE_NULL;
    ClassDesc[0] = UNICODE_NULL;
    *ClassImage = -1;

    /* Get the next class in the list */
    cr = CM_Enumerate_Classes(ClassIndex,
                              ClassGuid,
                              0);
    if (cr != CR_SUCCESS) return FALSE;

    /* Get the name of the device class */
    RequiredSize = MAX_CLASS_NAME_LEN;
    (VOID)SetupDiClassNameFromGuidW(ClassGuid,
                                    ClassName,
                                    RequiredSize,
                                    &RequiredSize);

    /* Open the registry key for this class */
    hKey = SetupDiOpenClassRegKeyExW(ClassGuid,
                                     MAXIMUM_ALLOWED,
                                     DIOCR_INSTALLER,
                                     NULL,
                                     0);
    if (hKey != INVALID_HANDLE_VALUE)
    {
        Size = ClassDescSize;
        Type = REG_SZ;

        /* Lookup the class description (win7+) */
        Success = RegQueryValueExW(hKey,
                                   L"ClassDesc",
                                   NULL,
                                   &Type,
                                   (LPBYTE)ClassDesc,
                                   &Size);
        if (Success == ERROR_SUCCESS)
        {
            /* Check if the string starts with an @ */
            if (ClassDesc[0] == L'@')
            {
                /* The description is located in a module resource */
                Success = ConvertResourceDescriptorToString(ClassDesc, ClassDescSize);
            }
        }
        else if (Success == ERROR_FILE_NOT_FOUND)
        {
            /* WinXP stores the description in the default value */
            Success = RegQueryValueExW(hKey,
                                       NULL,
                                       NULL,
                                       &Type,
                                       (LPBYTE)ClassDesc,
                                       &Size);
        }

        /* Close the registry key */
        RegCloseKey(hKey);
    }
    else
    {
        Success = GetLastError();
    }

    /* Check if we failed to get the class description */
    if (Success != ERROR_SUCCESS)
    {
        /* Use the class name as the description */
        wcscpy_s(ClassDesc, ClassDescSize, ClassName);
    }

    /* Get the image index for this class */
    (VOID)SetupDiGetClassImageIndex(&m_ImageListData,
                                    ClassGuid,
                                    ClassImage);

    return TRUE;
}

BOOL
CDevices::EnumDevicesForClass(
    _Inout_opt_ LPHANDLE DeviceHandle,
    _In_ LPCGUID ClassGuid,
    _In_ DWORD Index,
    _Out_ LPBOOL MoreItems,
    _Out_ LPTSTR DeviceName,
    _In_ DWORD DeviceNameSize,
    _Outptr_ LPTSTR *DeviceId,
    _Out_ PULONG Status,
    _Out_ PULONG ProblemNumber
    )
{
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD DevIdSize;
    HDEVINFO hDevInfo;
    BOOL bUnknown, bSuccess;

    *MoreItems = FALSE;
    *DeviceName = NULL;
    *DeviceId = NULL;
    bUnknown = FALSE;

    /* The unknown class is a special case */
    if (IsEqualGUID(*ClassGuid, GUID_DEVCLASS_UNKNOWN))
        bUnknown = TRUE;


    /* Check if this is the first call for this class */
    if (*DeviceHandle == NULL)
    {
        ATLASSERT(Index == 0);

        /* Check for the special case */
        if (bUnknown == TRUE)
        {
            /* Get device info for all devices for all classes */
            hDevInfo = SetupDiGetClassDevsW(NULL,
                                            NULL,
                                            NULL,
                                            DIGCF_ALLCLASSES);
            if (hDevInfo == INVALID_HANDLE_VALUE)
                return FALSE;
        }
        else
        {
            /* We only want the devices for this class */
            hDevInfo = SetupDiGetClassDevsW(ClassGuid,
                                            NULL,
                                            NULL,
                                            DIGCF_PRESENT);
            if (hDevInfo == INVALID_HANDLE_VALUE)
                return FALSE;
        }

        /* Store the handle for the next call */
        *DeviceHandle = (HANDLE)hDevInfo;
    }
    else
    {
        hDevInfo = (HDEVINFO)*DeviceHandle;
    }


    /* Get the device info for this device in the class */
    ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    bSuccess = SetupDiEnumDeviceInfo(hDevInfo,
                                     Index,
                                     &DeviceInfoData);
    if (bSuccess == FALSE) goto Quit;



    /* We found a device, let the caller know they might be more */
    *MoreItems = TRUE;

    /* Check if this is the unknown class */
    if (bUnknown)
    {
        /* We're looking for devices without guids */
        if (IsEqualGUID(DeviceInfoData.ClassGuid, GUID_NULL) == FALSE)
        {
            /* This is a known device, we aren't interested in it */
            return FALSE;
        }
    }

    /* Get the size required to hold the device id */
    bSuccess = SetupDiGetDeviceInstanceIdW(hDevInfo,
                                           &DeviceInfoData,
                                           NULL,
                                           0,
                                           &DevIdSize);
    if (bSuccess == FALSE && (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
        goto Quit;

    /* Allocate some heap to hold the device string */
    *DeviceId = (LPTSTR)HeapAlloc(GetProcessHeap(),
                                  0,
                                  DevIdSize * sizeof(WCHAR));
    if (*DeviceId == NULL) goto Quit;

    /* Now get the device id string */
    bSuccess = SetupDiGetDeviceInstanceIdW(hDevInfo,
                                           &DeviceInfoData,
                                           *DeviceId,
                                           DevIdSize,
                                           NULL);
    if (bSuccess == FALSE) goto Quit;

    /* Skip the root device */
    if (*DeviceId != NULL &&
        wcscmp(*DeviceId, L"HTREE\\ROOT\\0") == 0)
    {
        bSuccess = FALSE;
        goto Quit;
    }


    /* Get the current status of the device */
    bSuccess = GetDeviceStatus(*DeviceId, Status, ProblemNumber);
    if (bSuccess == FALSE) goto Quit;


    /* Get the device's friendly name */
    bSuccess = SetupDiGetDeviceRegistryPropertyW(hDevInfo,
                                                 &DeviceInfoData,
                                                 SPDRP_FRIENDLYNAME,
                                                 0,
                                                 (BYTE*)DeviceName,
                                                 256,
                                                 NULL);
    if (bSuccess == FALSE)
    {
        /* if the friendly name fails, try the description instead */
        bSuccess = SetupDiGetDeviceRegistryPropertyW(hDevInfo,
                                                     &DeviceInfoData,
                                                     SPDRP_DEVICEDESC,
                                                     0,
                                                     (BYTE*)DeviceName,
                                                     256,
                                                     NULL);
    }

    /* If we didn't find a name, check if this is an unknown device */
    if (bSuccess == FALSE && bUnknown == TRUE)
    {
        /* We add in our own text */
        wcscpy_s(DeviceName, 256, L"Unknown device");
        bSuccess = TRUE;
    }

Quit:
    if (MoreItems == FALSE)
        SetupDiDestroyDeviceInfoList(hDevInfo);

    if (bSuccess == FALSE)
    {
        if (*DeviceId)
        {
            HeapFree(GetProcessHeap(), 0, *DeviceId);
            *DeviceId = NULL;
        }
    }

    return bSuccess;
}


/* PRIVATE METHODS ******************************************/

BOOL
CDevices::CreateRootDevice()
{
    HBITMAP hRootImage = NULL;
    DWORD Size = ROOT_NAME_SIZE;
    BOOL bSuccess = FALSE;
    CONFIGRET cr;

    /* The root name is the computer name */
    (VOID)GetComputerNameW(m_RootName, &Size);

    /* Load the bitmap we'll be using as the root image */
    hRootImage = LoadBitmapW(g_hInstance,
                             MAKEINTRESOURCEW(IDB_ROOT_IMAGE));
    if (hRootImage == NULL) goto Cleanup;

    /* Add this bitmap to the device image list. This is a bit hacky, but it's safe */
    m_RootImageIndex = ImageList_Add(m_ImageListData.ImageList,
                                    hRootImage,
                                    NULL);
    if (m_RootImageIndex == -1)
        goto Cleanup;

    /* Get the root instance */
    cr = CM_Locate_DevNodeW(&m_RootDevInst,
                            NULL,
                            CM_LOCATE_DEVNODE_NORMAL);
    if (cr == CR_SUCCESS)
        bSuccess = TRUE;

Cleanup:
    if (bSuccess == FALSE)
    {
        SetupDiDestroyClassImageList(&m_ImageListData);
        ZeroMemory(&m_ImageListData, sizeof(SP_CLASSIMAGELIST_DATA));
    }

    if (hRootImage) DeleteObject(hRootImage);

    return bSuccess;
}

DWORD
CDevices::ConvertResourceDescriptorToString(
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
    

    /* First check for a semi colon */
    ptr = wcschr(ResourceDescriptor, L';');
    if (ptr)
    {
        /* This must be an inf based descriptor, the desc is after the semi colon */
        wcscpy_s(ResourceDescriptor, ResourceDescriptorSize, ++ptr);
        dwError = ERROR_SUCCESS;
    }
    else
    {
        /* This must be a dll resource based descriptor. Find the comma */
        ptr = wcschr(ResourceDescriptor, L',');
        if (ptr == NULL) return ERROR_INVALID_DATA;

        /* Terminate the string where the comma was */
        *ptr = UNICODE_NULL;

        /* Expand any environment strings */
        Size = ExpandEnvironmentStringsW(&ResourceDescriptor[1], ModulePath, MAX_PATH);
        if (Size > MAX_PATH) return ERROR_BUFFER_OVERFLOW;
        if (Size == 0) return GetLastError();

        /* Put the comma back and move past it */
        *ptr = L',';
        ptr++;

        /* Load the dll */
        hModule = LoadLibraryExW(ModulePath, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (hModule == NULL) return GetLastError();

        /* Convert the resource id to a number */
        ResourceId = _wtoi(ptr);

        /* If the number is negative, make it positive */
        if (ResourceId < 0) ResourceId = -ResourceId;

        /* Load the string from the dll */
        if (LoadStringW(hModule, ResourceId, ResString, 256))
        {
            wcscpy_s(ResourceDescriptor, ResourceDescriptorSize, ResString);
            dwError = ERROR_SUCCESS;
        }
        else
        {
            dwError = GetLastError();
        }

        /* Free the library */
        FreeLibrary(hModule);
    }

    return dwError;
}


