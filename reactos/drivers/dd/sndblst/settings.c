/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 drivers/dd/sndblst/settings.c
 * PURPOSE:              MPU-401 MIDI device driver setting management
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Sept 28, 2003: Created
 */

#include <ntddk.h>
#include <windows.h>

#include "sndblst.h"

// #define NDEBUG
#include <debug.h>
#include "sbdebug.h"  // our own debug helper


NTSTATUS
OpenDevicesKey(
    IN PWSTR RegistryPath,
    OUT PHANDLE Key)
/*
    Description:
        Create a volatile key under this driver's Services node to contain
        the device name list.

    Parameters:
        RegistryPath    The location of the registry entry
        Key             The key in the registry

    Return Value:
        NT status STATUS_SUCCESS if successful (duh...)
*/
{
    NTSTATUS s;
    HANDLE hKey;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING uStr;

    // Attempt to open the key

    RtlInitUnicodeString(&uStr, RegistryPath);

    InitializeObjectAttributes(&oa, &uStr, OBJ_CASE_INSENSITIVE, NULL,
                                (PSECURITY_DESCRIPTOR)NULL);

    s = ZwOpenKey(&hKey, KEY_CREATE_SUB_KEY, &oa);

    if (! NT_SUCCESS(s))
        return s;   // Problem


    // Now create sub key

    RtlInitUnicodeString(&uStr, (PWSTR) DEVICE_SUBKEY);

    InitializeObjectAttributes(&oa, &uStr, OBJ_CASE_INSENSITIVE, hKey,
                                (PSECURITY_DESCRIPTOR)NULL);

    s = ZwCreateKey(Key, KEY_ALL_ACCESS, &oa, 0, NULL, REG_OPTION_VOLATILE,
                    NULL);

    ZwClose(hKey);

    return s;
}



NTSTATUS STDCALL EnumDeviceKeys(
    IN PUNICODE_STRING RegistryPath,
    IN PWSTR SubKey,
    IN PREGISTRY_CALLBACK_ROUTINE Callback,
    IN PVOID Context)
/*
    Description:
        Enumerate the device subkeys in the driver's registry entry, and
        call the specified callback routine for each device.

    Parameters:
        RegistryPath    The location of the registry entry
        Subkey          The device's subkey
        Callback        A routine called for each device
        Context         ???

    Return Value:
        NT status STATUS_SUCCESS if successful
*/
{
    NTSTATUS s;
    OBJECT_ATTRIBUTES oa;
    HANDLE hKey, hSubKey;
    UNICODE_STRING SubkeyName;
    ULONG i;

    // Attempt to open the key

    InitializeObjectAttributes(&oa, RegistryPath, OBJ_CASE_INSENSITIVE,
                                NULL, (PSECURITY_DESCRIPTOR)NULL);

    s = ZwOpenKey(&hKey, KEY_READ, &oa);

        TEST_STATUS(s); // debugging

    if (! NT_SUCCESS(s))
        return s;   // Problem

    RtlInitUnicodeString(&SubkeyName, SubKey);

    DPRINT("Subkey: %wZ\n", &SubkeyName);

    InitializeObjectAttributes(&oa, &SubkeyName, OBJ_CASE_INSENSITIVE,
                                hKey, (PSECURITY_DESCRIPTOR)NULL);

    s = ZwOpenKey(&hSubKey, KEY_ENUMERATE_SUB_KEYS, &oa);

    ZwClose(hKey);

        TEST_STATUS(s); // debugging

    if (! NT_SUCCESS(s))
        return s;


    // And now, the enumeration

    for (i = 0;; i ++)
    {
        KEY_BASIC_INFORMATION Info;
        PKEY_BASIC_INFORMATION pInfo;
        ULONG ResultLength = 0;
        ULONG Size = 0;
        PWSTR Pos;
        PWSTR Name;

        // Find the length of the subkey data

//        Info.NameLength = 0;    // TEMPORARY!

        s = ZwEnumerateKey(hSubKey, i, KeyBasicInformation, &Info,
                            sizeof(Info), &ResultLength);

        if (s == STATUS_NO_MORE_ENTRIES)
            break;

        DPRINT("Found an entry, allocating memory...\n");

//        Size = Info.NameLength + FIELD_OFFSET(KEY_BASIC_INFORMATION, Name[0]);
        Size = ResultLength + FIELD_OFFSET(KEY_BASIC_INFORMATION, Name[0]);

        DPRINT("Size is %d\n", Size);

        pInfo = (PKEY_BASIC_INFORMATION) ExAllocatePool(PagedPool, Size);

        if (pInfo == NULL)
        {
            DPRINT("INSUFFICIENT RESOURCES!\n");
            s = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        DPRINT("Re-enumerating...\n");

        s = ZwEnumerateKey(hSubKey, i, KeyBasicInformation, pInfo, Size,
                            &ResultLength);

//        TEST_STATUS(s); // debugging

        if (! NT_SUCCESS(s))
        {
            ExFreePool((PVOID) pInfo);
            s = STATUS_INTERNAL_ERROR;
            break;
        }

        DPRINT("Allocating memory for name...\n");

        Name = ExAllocatePool(PagedPool,
                          RegistryPath->Length + sizeof(WCHAR) +
                          SubkeyName.Length + sizeof(WCHAR) +
                          pInfo->NameLength + sizeof(UNICODE_NULL));

        if (Name == NULL)
        {
            DPRINT("INSUFFICIENT RESOURCES!");
            ExFreePool((PVOID) pInfo);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // Copy the key name
        RtlCopyMemory((PVOID)Name, (PVOID)RegistryPath->Buffer, RegistryPath->Length);
        Pos = Name + (RegistryPath->Length / sizeof(WCHAR));
        Pos[0] = '\\';
        Pos++;

        // Copy the parameters sub key name
        RtlCopyMemory((PVOID)Pos, (PVOID)SubKey, SubkeyName.Length);    //SubkeyName?
        Pos += SubkeyName.Length / sizeof(WCHAR);
        Pos[0] = '\\';
        Pos ++;

        // Copy the device sub key name
        RtlCopyMemory((PVOID)Pos, (PVOID)pInfo->Name, pInfo->NameLength);
        Pos += pInfo->NameLength / sizeof(WCHAR);
        Pos[0] = UNICODE_NULL;

        ExFreePool((PVOID)pInfo);

        DPRINT("Calling callback...\n");

        s = (*Callback)(Name, Context);

        if (! NT_SUCCESS(s))
        {   DPRINT("Callback FAILED\n");
            break;}
    }

    ZwClose(hSubKey);

    DPRINT("%d device registry keys found\n", i);

    if ((i == 0) && (s == STATUS_NO_MORE_ENTRIES))
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    return s == STATUS_NO_MORE_ENTRIES ? STATUS_SUCCESS : s;
}



NTSTATUS STDCALL LoadSettings(
    IN  PWSTR ValueName,
    IN  ULONG ValueType,
    IN  PVOID ValueData,
    IN  ULONG ValueLength,
    IN  PVOID Context,
    IN  PVOID EntryContext)
/*
    Description:
        Read the settings for a particular device

    Parameters:
        ValueName       The value to read from the registry
        ValueType       ?
        ValueData       ?
        ValueLength     ?
        Context         The configuration structure to write to
        EntryContext    ?

    Return Value:
        NT status STATUS_SUCCESS if successful
*/
{
    PDEVICE_EXTENSION DeviceInfo = Context;

    if (ValueType == REG_DWORD)
    {
        if (! _wcsicmp(ValueName, REGISTRY_PORT))
        {
            DeviceInfo->Port = *(PULONG) ValueData;
            DPRINT("Registry port = 0x%x\n", DeviceInfo->Port);
        }

        // More to come... (config.c)
    }

    else
    {
        // ?
    }

    return STATUS_SUCCESS;
}



NTSTATUS SaveSettings(
    IN  PWSTR RegistryPath,
    IN  ULONG Port,
    IN  ULONG IRQ,
    IN  ULONG DMA)
/*
    Description:
        Saves the settings for a particular device

    Parameters:
        RegistryPath    Where to save the settings to
        Port            The device's port number
        IRQ             The device's interrupt number
        DMA             The device's DMA channel

    Return Value:
        NT status STATUS_SUCCESS if successful
*/
{
    NTSTATUS s;

    DPRINT("SaveSettings() unimplemented\n");

//    UNIMPLEMENTED;

    return STATUS_SUCCESS;
}
