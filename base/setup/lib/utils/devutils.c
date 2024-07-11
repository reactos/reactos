/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Device utility functions
 * COPYRIGHT:   Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "precomp.h"
#include "devutils.h"

/* FUNCTIONS *****************************************************************/

/**
 * @brief
 * Open an existing device given by its NT-style path, which is assumed to be
 * for a disk device or a partition. The open is for synchronous I/O access.
 *
 * @param[in]   DevicePath
 * Supplies the NT-style path to the device to open.
 *
 * @param[out]  DeviceHandle
 * If successful, receives the NT handle of the opened device.
 * Once the handle is no longer in use, call NtClose() to close it.
 *
 * @param[in]   DesiredAccess
 * An ACCESS_MASK value combination that determines the requested access
 * to the device. Because the open is for synchronous access, SYNCHRONIZE
 * is automatically added to the access mask.
 *
 * @param[in]   ShareAccess
 * Specifies the type of share access for the device.
 *
 * @return  An NTSTATUS code indicating success or failure.
 *
 * @see pOpenDeviceEx()
 **/
NTSTATUS
pOpenDeviceEx_UStr(
    _In_ PCUNICODE_STRING DevicePath,
    _Out_ PHANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    InitializeObjectAttributes(&ObjectAttributes,
                               (PUNICODE_STRING)DevicePath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    return NtOpenFile(DeviceHandle,
                      DesiredAccess | SYNCHRONIZE,
                      &ObjectAttributes,
                      &IoStatusBlock,
                      ShareAccess,
                      /* FILE_NON_DIRECTORY_FILE | */
                      FILE_SYNCHRONOUS_IO_NONALERT);
}

/**
 * @brief
 * Open an existing device given by its NT-style path, which is assumed to be
 * for a disk device or a partition. The open is share read/write/delete, for
 * synchronous I/O and read access.
 *
 * @param[in]   DevicePath
 * @param[out]  DeviceHandle
 * See the DevicePath and DeviceHandle parameters of pOpenDeviceEx_UStr().
 *
 * @return  An NTSTATUS code indicating success or failure.
 *
 * @see pOpenDevice(), pOpenDeviceEx(), pOpenDeviceEx_UStr()
 **/
NTSTATUS
pOpenDevice_UStr(
    _In_ PCUNICODE_STRING DevicePath,
    _Out_ PHANDLE DeviceHandle)
{
    return pOpenDeviceEx_UStr(DevicePath,
                              DeviceHandle,
                              FILE_READ_DATA | FILE_READ_ATTRIBUTES,
                              FILE_SHARE_ALL);
}

/**
 * @brief
 * Open an existing device given by its NT-style path, which is assumed to be
 * for a disk device or a partition. The open is for synchronous I/O access.
 *
 * @param[in]   DevicePath
 * @param[out]  DeviceHandle
 * @param[in]   DesiredAccess
 * @param[in]   ShareAccess
 * See pOpenDeviceEx_UStr() parameters.
 *
 * @return  An NTSTATUS code indicating success or failure.
 *
 * @see pOpenDeviceEx_UStr()
 **/
NTSTATUS
pOpenDeviceEx(
    _In_ PCWSTR DevicePath,
    _Out_ PHANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess)
{
    UNICODE_STRING Name;
    RtlInitUnicodeString(&Name, DevicePath);
    return pOpenDeviceEx_UStr(&Name, DeviceHandle, DesiredAccess, ShareAccess);
}

/**
 * @brief
 * Open an existing device given by its NT-style path, which is assumed to be
 * for a disk device or a partition. The open is share read/write/delete, for
 * synchronous I/O and read access.
 *
 * @param[in]   DevicePath
 * @param[out]  DeviceHandle
 * See the DevicePath and DeviceHandle parameters of pOpenDeviceEx_UStr().
 *
 * @return  An NTSTATUS code indicating success or failure.
 *
 * @see pOpenDeviceEx(), pOpenDevice_UStr(), pOpenDeviceEx_UStr()
 **/
NTSTATUS
pOpenDevice(
    _In_ PCWSTR DevicePath,
    _Out_ PHANDLE DeviceHandle)
{
    UNICODE_STRING Name;
    RtlInitUnicodeString(&Name, DevicePath);
    return pOpenDevice_UStr(&Name, DeviceHandle);
}


/* PnP ENUMERATION SUPPORT HELPERS *******************************************/

#define _CFGMGR32_
#include <cfgmgr32.h>

/**
 * @brief
 * Enumerates devices using PnP support.
 * The type of devices to be enumerated is specified by an interface
 * class GUID. A user-provided callback is invoked for each device found.
 *
 * @param[in]   InterfaceClassGuid
 * The interface class GUID designating the devices to enumerate.
 *
 * @param[in]   Callback
 * A user-provided callback function of type PENUM_DEVICES_PROC.
 *
 * @param[in]   Context
 * An optional context for the callback function.
 *
 * @note
 * This function uses the lower-level user-mode CM_* PnP API,
 * that are more widely available than the more common Win32
 * SetupDi* functions.
 * See
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/install/porting-from-setupapi-to-cfgmgr32#get-a-list-of-interfaces-get-the-device-exposing-each-interface-and-get-a-property-from-the-device
 * for more details.
 **/
NTSTATUS
// pNtEnumDevicesByInterfaceClass
pNtEnumDevicesPnP(
    _In_ const GUID* InterfaceClassGuid,
    _In_ PENUM_DEVICES_PROC Callback,
    _In_opt_ PVOID Context)
{
    NTSTATUS Status;
    CONFIGRET cr;
    ULONG DevIFaceListLength = 0;
    PWSTR DevIFaceList = NULL;
    PWSTR CurrentIFace;

    /*
     * Retrieve a list of device interface instances belonging to the given interface class.
     * Equivalent to:
     * hDevInfo = SetupDiGetClassDevs(pGuid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
     * and SetupDiEnumDeviceInterfaces(hDevInfo, NULL, pGuid, i, &DevIFaceData);
     */
    do
    {
        cr = CM_Get_Device_Interface_List_SizeW(&DevIFaceListLength,
                                                (GUID*)InterfaceClassGuid,
                                                NULL,
                                                CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
        if (cr != CR_SUCCESS)
            break;

        if (DevIFaceList) RtlFreeHeap(ProcessHeap, 0, DevIFaceList);
        DevIFaceList = RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY,
                                       DevIFaceListLength * sizeof(WCHAR));
        if (!DevIFaceList)
        {
            cr = CR_OUT_OF_MEMORY;
            break;
        }

        cr = CM_Get_Device_Interface_ListW((GUID*)InterfaceClassGuid,
                                           NULL,
                                           DevIFaceList,
                                           DevIFaceListLength,
                                           CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    } while (cr == CR_BUFFER_SMALL);

    if (cr != CR_SUCCESS)
    {
        if (DevIFaceList) RtlFreeHeap(ProcessHeap, 0, DevIFaceList);
        return STATUS_UNSUCCESSFUL;
    }

    /* Enumerate each device for the given interface class.
     * NOTE: This gives the proper interface names with the correct casing,
     * contrary to SetupDiGetDeviceInterfaceDetailW(...) that gives them
     * in all lower-case letters. */
    for (CurrentIFace = DevIFaceList;
         *CurrentIFace;
         CurrentIFace += wcslen(CurrentIFace) + 1)
    {
        UNICODE_STRING Name;
        HANDLE DeviceHandle;

// TESTING
#if 0
        WCHAR DevInstPath[MAX_DEVICE_ID_LEN];
        PWSTR buffer = NULL;
        ULONG buffersize = 0;

        cr = CM_Locate_DevNodeW(&DevInst,
                                CurrentDevice, // ????
                                CM_LOCATE_DEVNODE_NORMAL);
        if (cr != CR_SUCCESS)
            break;

        cr = CM_Get_Device_IDW(DevInst,
                               DevInstPath,
                               _countof(DevInstPath),
                               0);

        for (;;)
        {
            // SetupDiGetDeviceRegistryPropertyW(..., SPDRP_PHYSICAL_DEVICE_OBJECT_NAME, ...);
            cr = CM_Get_DevNode_Registry_PropertyW(DevInst,
                                                   CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                                   NULL, // PULONG pulRegDataType
                                                   (PBYTE)buffer,
                                                   &buffersize,
                                                   0);
            if (cr != CR_BUFFER_SMALL)
                break;

            if (buffer) RtlFreeHeap(ProcessHeap, 0, buffer);
            buffer = RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, buffersize);
            if (!buffer)
            {
                cr = CR_OUT_OF_MEMORY;
                break;
            }
        }
        if (cr != CR_SUCCESS)
            continue;
#endif

        RtlInitUnicodeString(&Name, CurrentIFace);

        /* Normalize the interface path in case it is of Win32-style */
        if (Name.Length > 4 * sizeof(WCHAR) &&
            Name.Buffer[0] == '\\' && Name.Buffer[1] == '\\' &&
            Name.Buffer[2] == '?'  && Name.Buffer[3] == '\\')
        {
            Name.Buffer[1] = '?';
        }

        Status = pOpenDevice(/*Name*/CurrentIFace, &DeviceHandle);
        if (NT_SUCCESS(Status))
        {
            /* Do the callback */
            if (Callback)
            {
                (void)Callback(InterfaceClassGuid,
                               CurrentIFace, DeviceHandle, Context);
            }

            NtClose(DeviceHandle);
        }

#if 0
        if (buffer) RtlFreeHeap(ProcessHeap, 0, buffer);
#endif
    }

    if (DevIFaceList) RtlFreeHeap(ProcessHeap, 0, DevIFaceList);

    return STATUS_SUCCESS;
}

#if 0 // FIXME!

// FIXME: This is actually Win7+
#define CM_GETIDLIST_FILTER_PRESENT             (0x00000100)
#define CM_GETIDLIST_FILTER_CLASS               (0x00000200)

NTSTATUS
// pNtEnumDevicesBySetupClass
pNtEnumDevicesPnP_Alt(
    _In_ const GUID* SetupClassGuid,
    _In_ PENUM_DEVICES_PROC Callback,
    _In_opt_ PVOID Context)
{
    NTSTATUS Status;
    CONFIGRET cr;

    WCHAR guidString[MAX_GUID_STRING_LEN];

    ULONG DevIDListLength = 0;
    PWSTR DevIDList = NULL;
    PWSTR CurrentID;

    // GuidToString(), UuidToStringW(), RtlStringFromGUID()
    Status = RtlStringCchPrintfW(guidString, _countof(guidString),
                              // L"{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"
                                 L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                                 SetupClassGuid->Data1, SetupClassGuid->Data2, SetupClassGuid->Data3,
                                 SetupClassGuid->Data4[0], SetupClassGuid->Data4[1],
                                 SetupClassGuid->Data4[2], SetupClassGuid->Data4[3],
                                 SetupClassGuid->Data4[4], SetupClassGuid->Data4[5],
                                 SetupClassGuid->Data4[6], SetupClassGuid->Data4[7]);
    ASSERT(NT_SUCCESS(Status));

    /*
     * Retrieve a list of device interface instances belonging to the given interface class.
     * Equivalent to:
     * hDevInfo = SetupDiGetClassDevs(pGuid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
     * and SetupDiEnumDeviceInterfaces(hDevInfo, NULL, pGuid, i, &DevIFaceData);
     */
    do
    {
        cr = CM_Get_Device_ID_List_SizeW(&DevIDListLength,
                                         guidString,
                                         CM_GETIDLIST_FILTER_CLASS |
                                         CM_GETIDLIST_FILTER_PRESENT);
        if (cr != CR_SUCCESS)
            break;

        if (DevIDList) RtlFreeHeap(ProcessHeap, 0, DevIDList);
        DevIDList = RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY,
                                    DevIDListLength * sizeof(WCHAR));
        if (!DevIDList)
        {
            cr = CR_OUT_OF_MEMORY;
            break;
        }

        cr = CM_Get_Device_ID_ListW(guidString,
                                    DevIDList,
                                    DevIDListLength,
                                    CM_GETIDLIST_FILTER_CLASS |
                                    CM_GETIDLIST_FILTER_PRESENT);
    } while (cr == CR_BUFFER_SMALL);

    if (cr != CR_SUCCESS)
    {
        if (DevIDList) RtlFreeHeap(ProcessHeap, 0, DevIDList);
        return STATUS_UNSUCCESSFUL;
    }

    /* Enumerate each device for the given interface class.
     * NOTE: This gives the proper interface names with the correct casing,
     * contrary to SetupDiGetDeviceInterfaceDetailW(...) that gives them
     * in all lower-case letters. */
    for (CurrentID = DevIDList;
         *CurrentID;
         CurrentID += wcslen(CurrentID) + 1)
    {
        UNICODE_STRING Name;
        HANDLE DeviceHandle;

        WCHAR DevInstPath[MAX_DEVICE_ID_LEN];
        DEVINST DevInst;
        PWSTR buffer = NULL;
        ULONG buffersize = 0;

        cr = CM_Locate_DevNodeW(&DevInst,
                                CurrentID, // CurrentDevice,
                                CM_LOCATE_DEVNODE_NORMAL);
        if (cr != CR_SUCCESS)
            break;

#if 1 // TEST: DevInstPath should get back CurrentID
        cr = CM_Get_Device_IDW(DevInst,
                               DevInstPath,
                               _countof(DevInstPath),
                               0);
#endif

        for (;;)
        {
            // SetupDiGetDeviceRegistryPropertyW(..., SPDRP_PHYSICAL_DEVICE_OBJECT_NAME, ...);
            cr = CM_Get_DevNode_Registry_PropertyW(DevInst,
                                                   CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                                   NULL, // PULONG pulRegDataType
                                                   (PBYTE)buffer,
                                                   &buffersize,
                                                   0);
            if (cr != CR_BUFFER_SMALL)
                break;

            if (buffer) RtlFreeHeap(ProcessHeap, 0, buffer);
            buffer = RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, buffersize);
            if (!buffer)
            {
                cr = CR_OUT_OF_MEMORY;
                break;
            }
        }
        if (cr != CR_SUCCESS)
            continue;


        RtlInitUnicodeString(&Name, buffer);

        /* Normalize the interface path in case it is of Win32-style */
        if (Name.Length > 4 * sizeof(WCHAR) &&
            Name.Buffer[0] == '\\' && Name.Buffer[1] == '\\' &&
            Name.Buffer[2] == '?'  && Name.Buffer[3] == '\\')
        {
            Name.Buffer[1] = '?';
        }

        Status = pOpenDevice(/*Name*/buffer, &DeviceHandle);
        if (NT_SUCCESS(Status))
        {
            /* Do the callback */
            if (Callback)
            {
                (void)Callback(SetupClassGuid,
                               buffer, DeviceHandle, Context);
            }

            NtClose(DeviceHandle);
        }

        if (buffer) RtlFreeHeap(ProcessHeap, 0, buffer);
    }

    if (DevIDList) RtlFreeHeap(ProcessHeap, 0, DevIDList);

    return STATUS_SUCCESS;
}

#endif

/* EOF */
