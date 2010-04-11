/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/pnpinit.c
 * PURPOSE:         PnP Initialization Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PUNICODE_STRING PiInitGroupOrderTable;
ULONG PiInitGroupOrderTableCount;
 
/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
PiInitCacheGroupInformation(VOID)
{
    HANDLE KeyHandle;
    NTSTATUS Status;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    PUNICODE_STRING GroupTable;
    ULONG Count;
    UNICODE_STRING GroupString =
        RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet"
                            L"\\Control\\ServiceGroupOrder");
    
    /* ReactOS HACK for SETUPLDR */
    if (KeLoaderBlock->SetupLdrBlock)
    {
        /* Bogus data */
        PiInitGroupOrderTableCount = 0;
        PiInitGroupOrderTable = (PVOID)0xBABEB00B;
        return STATUS_SUCCESS;
    }
    
    /* Open the registry key */
    Status = IopOpenRegistryKeyEx(&KeyHandle,
                                  NULL,
                                  &GroupString,
                                  KEY_READ);
    if (NT_SUCCESS(Status))
    {
        /* Get the list */
        Status = IopGetRegistryValue(KeyHandle, L"List", &KeyValueInformation);
        ZwClose(KeyHandle);
        
        /* Make sure we got it */
        if (NT_SUCCESS(Status))
        {
            /* Make sure it's valid */
            if ((KeyValueInformation->Type == REG_MULTI_SZ) &&
                (KeyValueInformation->DataLength))
            {
                /* Convert it to unicode strings */
                Status = PnpRegMultiSzToUnicodeStrings(KeyValueInformation,
                                                       &GroupTable,
                                                       &Count);
                
                /* Cache it for later */
                PiInitGroupOrderTable = GroupTable;
                PiInitGroupOrderTableCount = Count;
            }
            else
            {
                /* Fail */
                Status = STATUS_UNSUCCESSFUL;
            }
            
            /* Free the information */
            ExFreePool(KeyValueInformation);
        }
    }
    
    /* Return status */
    return Status;
}

USHORT
NTAPI
PpInitGetGroupOrderIndex(IN HANDLE ServiceHandle)
{
    NTSTATUS Status;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    ULONG i;
    PVOID Buffer;
    UNICODE_STRING Group;
    PAGED_CODE();
       
    /* Make sure we have a cache */
    if (!PiInitGroupOrderTable) return -1;
    
    /* If we don't have a handle, the rest is easy -- return the count */
    if (!ServiceHandle) return PiInitGroupOrderTableCount + 1;
    
    /* Otherwise, get the group value */
    Status = IopGetRegistryValue(ServiceHandle, L"Group", &KeyValueInformation);
    if (!NT_SUCCESS(Status)) return PiInitGroupOrderTableCount;

    /* Make sure we have a valid string */
    ASSERT(KeyValueInformation->Type == REG_SZ);
    ASSERT(KeyValueInformation->DataLength);
    
    /* Convert to unicode string */
    Buffer = (PVOID)((ULONG_PTR)KeyValueInformation + KeyValueInformation->DataOffset);
    PnpRegSzToString(Buffer, KeyValueInformation->DataLength, &Group.Length);
    Group.MaximumLength = KeyValueInformation->DataLength;
    Group.Buffer = Buffer;
    
    /* Loop the groups */
    for (i = 0; i < PiInitGroupOrderTableCount; i++)
    {
        /* Try to find a match */
        if (RtlEqualUnicodeString(&Group, &PiInitGroupOrderTable[i], TRUE)) break;
    }
    
    /* We're done */
    ExFreePool(KeyValueInformation);
    return i;
}

USHORT
NTAPI
PipGetDriverTagPriority(IN HANDLE ServiceHandle)
{
    NTSTATUS Status;
    HANDLE KeyHandle = NULL;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation = NULL;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformationTag;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformationGroupOrderList;
    PVOID Buffer;
    UNICODE_STRING Group;
    PULONG GroupOrder;
    ULONG i = -1, Count, Tag = 0;
    UNICODE_STRING GroupString =
    RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet"
                        L"\\Control\\ServiceGroupOrder");
    
    /* Open the key */
    Status = IopOpenRegistryKeyEx(&KeyHandle, NULL, &GroupString, KEY_READ);
    if (!NT_SUCCESS(Status)) goto Quickie;
    
    /* Read the group */
    Status = IopGetRegistryValue(ServiceHandle, L"Group", &KeyValueInformation);
    if (!NT_SUCCESS(Status)) goto Quickie;
    
    /* Make sure we have a group */
    if ((KeyValueInformation->Type == REG_SZ) &&
        (KeyValueInformation->DataLength))
    {
        /* Convert to unicode string */
        Buffer = (PVOID)((ULONG_PTR)KeyValueInformation + KeyValueInformation->DataOffset);
        PnpRegSzToString(Buffer, KeyValueInformation->DataLength, &Group.Length);
        Group.MaximumLength = KeyValueInformation->DataLength;
        Group.Buffer = Buffer;
    }

    /* Now read the tag */
    Status = IopGetRegistryValue(ServiceHandle, L"Tag", &KeyValueInformationTag);
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Make sure we have a tag */
    if ((KeyValueInformationTag->Type == REG_DWORD) &&
        (KeyValueInformationTag->DataLength))
    {
        /* Read it */
        Tag = *(PULONG)((ULONG_PTR)KeyValueInformationTag +
                        KeyValueInformationTag->DataOffset);
    }
    
    /* We can get rid of this now */
    ExFreePool(KeyValueInformationTag);

    /* Now let's read the group's tag order */
    Status = IopGetRegistryValue(KeyHandle,
                                 Group.Buffer,
                                 &KeyValueInformationGroupOrderList);
    
    /* We can get rid of this now */
Quickie:
    if (KeyValueInformation) ExFreePool(KeyValueInformation);
    if (KeyHandle) NtClose(KeyHandle);
    if (!NT_SUCCESS(Status)) return -1;
    
    /* We're on the success path -- validate the tag order*/
    if ((KeyValueInformationGroupOrderList->Type == REG_BINARY) &&
        (KeyValueInformationGroupOrderList->DataLength))
    {
        /* Get the order array */
        GroupOrder = (PULONG)((ULONG_PTR)KeyValueInformationGroupOrderList +
                              KeyValueInformationGroupOrderList->DataOffset);
        
        /* Get the count */
        Count = *GroupOrder;
        ASSERT(((Count + 1) * sizeof(ULONG)) <=
               KeyValueInformationGroupOrderList->DataLength);
        
        /* Now loop each tag */
        GroupOrder++;
        for (i = 1; i <= Count; i++)
        {
            /* If we found it, we're out */
            if (Tag == *GroupOrder) break;

            /* Try the next one */
            GroupOrder++;
        }
    }
    
    /* Last buffer to free */
    ExFreePool(KeyValueInformationGroupOrderList);
    return i;
}

NTSTATUS
NTAPI
PipCallDriverAddDevice(IN PDEVICE_NODE DeviceNode,
                       IN BOOLEAN LoadDriver,     
                       IN PDRIVER_OBJECT DriverObject)
{ 
    NTSTATUS Status;
    HANDLE EnumRootKey, SubKey, ControlKey, ClassKey, PropertiesKey;
    UNICODE_STRING ClassGuid, Properties;
    UNICODE_STRING EnumRoot = RTL_CONSTANT_STRING(ENUM_ROOT);
    UNICODE_STRING ControlClass =
    RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class");
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation = NULL;
    PWCHAR Buffer;
    
    /* Open enumeration root key */
    Status = IopOpenRegistryKeyEx(&EnumRootKey,
                                  NULL,
                                  &EnumRoot,
                                  KEY_READ);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopOpenRegistryKeyEx() failed with Status %08X\n", Status);
        return Status;
    }
    
    /* Open instance subkey */
    Status = IopOpenRegistryKeyEx(&SubKey,
                                  EnumRootKey,
                                  &DeviceNode->InstancePath,
                                  KEY_READ);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopOpenRegistryKeyEx() failed with Status %08X\n", Status);
        ZwClose(EnumRootKey);
        return Status;
    }
    
    /* Get class GUID */
    Status = IopGetRegistryValue(SubKey,
                                 REGSTR_VAL_CLASSGUID,
                                 &KeyValueInformation);
    if (NT_SUCCESS(Status))
    {
        /* Convert to unicode string */
        Buffer = (PVOID)((ULONG_PTR)KeyValueInformation + KeyValueInformation->DataOffset);
        PnpRegSzToString(Buffer, KeyValueInformation->DataLength, &ClassGuid.Length);
        ClassGuid.MaximumLength = KeyValueInformation->DataLength;
        ClassGuid.Buffer = Buffer;
        
        /* Open the key */
        Status = IopOpenRegistryKeyEx(&ControlKey,
                                      NULL,
                                      &ControlClass,
                                      KEY_READ);
        if (!NT_SUCCESS(Status))
        {
            /* No class key */
            DPRINT1("IopOpenRegistryKeyEx() failed with Status %08X\n", Status);
            ClassKey = NULL;
        }
        else
        {
            /* Open the class key */
            Status = IopOpenRegistryKeyEx(&ClassKey,
                                          ControlKey,
                                          &ClassGuid,
                                          KEY_READ);
            ZwClose(ControlKey);
            if (!NT_SUCCESS(Status))
            {
                /* No class key */
                DPRINT1("IopOpenRegistryKeyEx() failed with Status %08X\n", Status);
                ClassKey = NULL;
            }
        }
        
        /* Check if we made it till here */
        if (ClassKey)
        {
            /* Get the device properties */
            RtlInitUnicodeString(&Properties, REGSTR_KEY_DEVICE_PROPERTIES);
            Status = IopOpenRegistryKeyEx(&PropertiesKey,
                                          ClassKey,
                                          &Properties,
                                          KEY_READ);
            if (!NT_SUCCESS(Status))
            {
                /* No properties */
                DPRINT("IopOpenRegistryKeyEx() failed with Status %08X\n", Status);
                PropertiesKey = NULL;
            }
        }
        
        /* Free the registry data */
        ExFreePool(KeyValueInformation);
    }
    
    /* Do ReactOS-style setup */
    IopAttachFilterDrivers(DeviceNode, TRUE);
    Status = IopInitializeDevice(DeviceNode, DriverObject);
    if (NT_SUCCESS(Status))
    {
        IopAttachFilterDrivers(DeviceNode, FALSE);
        Status = IopStartDevice(DeviceNode);
    }
    
    /* Return status */
    return Status;
}

/* EOF */
