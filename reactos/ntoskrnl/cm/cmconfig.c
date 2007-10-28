/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmconfig.c
 * PURPOSE:         Configuration Manager - System Configuration Routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#include "cm.h"
#define NDEBUG
#include "debug.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
CmpInitializeRegistryNode(IN PCONFIGURATION_COMPONENT_DATA CurrentEntry,
                          IN HANDLE NodeHandle,
                          OUT PHANDLE NewHandle,
                          IN INTERFACE_TYPE InterfaceType,
                          IN ULONG BusNumber,
                          IN PUSHORT DeviceIndexTable)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName, ValueName, ValueData;
    HANDLE KeyHandle, ParentHandle;
    ANSI_STRING TempString;
    CHAR TempBuffer[12];
    WCHAR Buffer[12];
    PCONFIGURATION_COMPONENT Component;
    ULONG Disposition, Length = 0;

    /* Get the component */
    Component = &CurrentEntry->ComponentEntry;

    /* Set system class components to ARC system type */
    if (Component->Class == SystemClass) Component->Type = ArcSystem;

    /* Create a key for the component */
    InitializeObjectAttributes(&ObjectAttributes,
                               &CmTypeName[Component->Type],
                               OBJ_CASE_INSENSITIVE,
                               NodeHandle,
                               NULL);
    Status = NtCreateKey(&KeyHandle,
                         KEY_READ | KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         &Disposition);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if this is anything but a system class component */
    if (Component->Class != SystemClass)
    {
        /* Build the sub-component string */
        RtlIntegerToChar(DeviceIndexTable[Component->Type]++,
                         10,
                         12,
                         TempBuffer);
        RtlInitAnsiString(&TempString, TempBuffer);

        /* Convert it to Unicode */
        RtlInitEmptyUnicodeString(&KeyName, Buffer, sizeof(Buffer));
        RtlAnsiStringToUnicodeString(&KeyName, &TempString, FALSE);

        /* Create the key */
        ParentHandle = KeyHandle;
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   ParentHandle,
                                   NULL);
        Status = NtCreateKey(&KeyHandle,
                             KEY_READ | KEY_WRITE,
                             &ObjectAttributes,
                             0,
                             NULL,
                             0,
                             &Disposition);
        NtClose(ParentHandle);

        /* Fail if the key couldn't be created, and make sure it's a new key */
        if (!NT_SUCCESS(Status)) return Status;
        ASSERT(Disposition == REG_CREATED_NEW_KEY);
    }

    /* Sstup the compnent information key */
    RtlInitUnicodeString(&ValueName, L"Component Information");
    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_BINARY,
                           &Component->Flags,
                           FIELD_OFFSET(CONFIGURATION_COMPONENT,
                                        ConfigurationDataLength) -
                           FIELD_OFFSET(CONFIGURATION_COMPONENT, Flags));
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        NtClose(KeyHandle);
        return Status;
    }

    /* Check if we have an identifier */
    if (Component->IdentifierLength)
    {
        /* Build the string and convert it to Unicode */
        RtlInitUnicodeString(&ValueName, L"Identifier");
        RtlInitAnsiString(&TempString, Component->Identifier);
        Status = RtlAnsiStringToUnicodeString(&ValueData,
                                              &TempString,
                                              TRUE);
        if (NT_SUCCESS(Status))
        {
            /* Save the identifier in the registry */
            Status = NtSetValueKey(KeyHandle,
                                   &ValueName,
                                   0,
                                   REG_SZ,
                                   ValueData.Buffer,
                                   ValueData.Length + sizeof(UNICODE_NULL));
            RtlFreeUnicodeString(&ValueData);
        }

        /* Check for failure during conversion or registry write */
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            NtClose(KeyHandle);
            return Status;
        }
    }

    /* Setup the configuration data string */
    RtlInitUnicodeString(&ValueName, L"Configuration Data");

    /* Check if we got configuration data */
    if (CurrentEntry->ConfigurationData)
    {
        /* Calculate the total length and check if it fits into our buffer */
        Length = Component->ConfigurationDataLength +
                 FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList);
        if (Length > CmpConfigurationAreaSize)
        {
            ASSERTMSG("Component too large -- need reallocation!", FALSE);
        }
        else
        {
            /* Copy the data */
            RtlCopyMemory(&CmpConfigurationData->PartialResourceList.Version,
                          CurrentEntry->ConfigurationData,
                          Component->ConfigurationDataLength);
        }
    }
    else
    {
        /* No configuration data, setup defaults */
        CmpConfigurationData->PartialResourceList.Version = 0;
        CmpConfigurationData->PartialResourceList.Revision = 0;
        CmpConfigurationData->PartialResourceList.Count = 0;
        Length = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors) +
                 FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList);
    }

    /* Set the interface type and bus number */
    CmpConfigurationData->InterfaceType = InterfaceType;
    CmpConfigurationData->BusNumber = BusNumber;

    /* Save the actual data */
    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_FULL_RESOURCE_DESCRIPTOR,
                           CmpConfigurationData,
                           Length);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        NtClose(KeyHandle);
    }
    else
    {
        /* Return the new handle */
        *NewHandle = KeyHandle;
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
CmpInitializeHardwareConfiguration(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    ULONG Disposition;
    UNICODE_STRING KeyName;

    /* Setup the key name */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\Hardware\\DeviceMap");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Create the device map key */
    Status = NtCreateKey(&KeyHandle,
                         KEY_READ | KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         &Disposition);
    if (!NT_SUCCESS(Status)) return Status;
    NtClose(KeyHandle);

    /* Nobody should've created this key yet! */
    //ASSERT(Disposition == REG_CREATED_NEW_KEY);

    /* Setup the key name */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\Hardware\\Description");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Create the description key */
    Status = NtCreateKey(&KeyHandle,
                          KEY_READ | KEY_WRITE,
                          &ObjectAttributes,
                          0,
                          NULL,
                          0,
                          &Disposition);
    if (!NT_SUCCESS(Status)) return Status;

    /* Nobody should've created this key yet! */
    //ASSERT(Disposition == REG_CREATED_NEW_KEY);

    /* Allocate the configuration data buffer */
    CmpConfigurationData = ExAllocatePoolWithTag(PagedPool,
                                                 CmpConfigurationAreaSize,
                                                 TAG_CM);
    if (!CmpConfigurationData) return STATUS_INSUFFICIENT_RESOURCES;

    /* Check if we got anything from NTLDR */
    if (LoaderBlock->ConfigurationRoot)
    {
        ASSERTMSG("NTLDR ARC Hardware Tree Not Supported!\n", FALSE);
    }
    else
    {
        /* Nothing else to do */
        Status = STATUS_SUCCESS;
    }

    /* Close our handle, free the buffer and return status */
    ExFreePool(CmpConfigurationData);
    NtClose(KeyHandle);
    return Status;
}

