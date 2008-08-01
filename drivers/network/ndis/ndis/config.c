/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/config.c
 * PURPOSE:     NDIS Configuration Services
 * PROGRAMMERS: Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *     Vizzini 07-28-2003 Created
 * NOTES:
 *     - Resource tracking has to be implemented here because of the design of the NDIS API.
 *       Whenever a read operation is performed, the NDIS library allocates space and returns
 *       it.  A linked list is kept associated with every handle of the memory allocated to
 *       it.  When the handle is closed, the resources are systematically released.
 *     - The NDIS_HANDLE Configuraiton context is no longer a registry handle.  An opaque struct
 *       had to be created to allow for resource tracking.  This means that Miniports cannot just
 *       pass this NDIS_HANDLE to things like ZwQueryValueKey().  I don't thknk they do (they
 *       certainly should not), but it should be kept in mind.
 *         UPDATE:  I just found this in the NTDDK:
 *         NdisOpenProtocolConfiguration returns a handle for the
 *         HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\NICDriverInstance\Parameters\ProtocolName
 *         registry key.  XXX This is a problem.  Following that, the DDK instructs programmers
 *         to use NdisReadConfiguration and NdisWriteConfiguration.  No telling what the world's idiots
 *         have done with this.
 *     - I have tried to stick to the DDK's definition of what return values are possible, which
 *       has resulted in stupid return values in some cases.  I do use STATUS_RESOURCES in a few
 *       places that the DDK doesn't explicitly mention it, though.
 *     - There's a general reliance on the fact that UNICODE_STRING.Length doesn't include a trailing
 *       0, which it shouldn't
 *     - I added support for NdisParameterBinary.  It's at the end of the struct.  I wonder if
 *       it'll break things.
 *     - All the routines in this file are PASSIVE_LEVEL only, and all memory is PagedPool
 */

#include "ndissys.h"

#define NDIS_VERSION 0x00040000          /* the version of NDIS we claim to be to miniport drivers */
#define PARAMETERS_KEY L"Parameters"     /* The parameters subkey under the device-specific key */

/*
 * @implemented
 */
VOID
EXPORT
NdisWriteConfiguration(
    OUT PNDIS_STATUS                    Status,
    IN  NDIS_HANDLE                     ConfigurationHandle,
    IN  PNDIS_STRING                    Keyword,
    IN  PNDIS_CONFIGURATION_PARAMETER   ParameterValue)
/*
 * FUNCTION: Writes a configuration value to the registry
 * ARGUMENTS:
 *     Status: Pointer to a caller-supplied NDIS_STATUS where we return status
 *     ConfigurationHandle: The Configuration Handle passed back from the call to one of the Open functions
 *     Keyword: The registry value name to write
 *     ParameterValue: The value data to write
 * RETURNS:
 *     NDIS_STATUS_SUCCESS - the operation completed successfully
 *     NDIS_STATUS_NOT_SUPPORTED - The parameter type is not supported
 *     NDIS_STATUS_RESOURCES - out of memory, etc.
 *     NDIS_STATUS_FAILURE - any other failure
 * NOTES:
 *    There's a cryptic comment in the ddk implying that this function allocates and keeps memory.
 *    I don't know why tho so i free everything before return.  comments welcome.
 */
{
    ULONG ParameterType = ParameterValue->ParameterType;
    ULONG DataSize;
    PVOID Data;
    WCHAR Buff[25];

    if(ParameterType != NdisParameterInteger &&
        ParameterType != NdisParameterHexInteger &&
        ParameterType != NdisParameterString &&
        ParameterType != NdisParameterMultiString &&
        ParameterType != NdisParameterBinary
      )
    {
        *Status = NDIS_STATUS_NOT_SUPPORTED;
        return;
    }

    /* reset parameter type to standard reg types */
    switch(ParameterType)
    {
        case NdisParameterHexInteger:
        case NdisParameterInteger:
             {
                 UNICODE_STRING Str;

                 Str.Buffer = (PWSTR) &Buff;
                 Str.MaximumLength = (USHORT)sizeof(Buff);
                 Str.Length = 0;

                 ParameterType = REG_SZ;
                 if (!NT_SUCCESS(RtlIntegerToUnicodeString(
                      ParameterValue->ParameterData.IntegerData,
                      (ParameterType == NdisParameterInteger) ? 10 : 16, &Str)))
                 {
                      *Status = NDIS_STATUS_FAILURE;
                      return;
                 }
                 Data = Str.Buffer;
                 DataSize = Str.Length;
             }
             break;
        case NdisParameterString:
        case NdisParameterMultiString:
            ParameterType = REG_SZ;
            Data = ParameterValue->ParameterData.StringData.Buffer;
            DataSize = ParameterValue->ParameterData.StringData.Length;
            break;

        /* New (undocumented) addition to 2k ddk */
        case NdisParameterBinary:
            ParameterType = REG_BINARY;
            Data = ParameterValue->ParameterData.BinaryData.Buffer;
            DataSize = ParameterValue->ParameterData.BinaryData.Length;
            break;

        default:
            *Status = NDIS_STATUS_FAILURE;
            return;
    }

    *Status = ZwSetValueKey(((PMINIPORT_CONFIGURATION_CONTEXT)ConfigurationHandle)->Handle,
            Keyword, 0, ParameterType, Data, DataSize);

    if(*Status != STATUS_SUCCESS)
        *Status = NDIS_STATUS_FAILURE;
    else
        *Status = NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisCloseConfiguration(
    IN  NDIS_HANDLE ConfigurationHandle)
/*
 * FUNCTION: Closes handles and releases per-handle resources
 * ARGUMENTS:
 *     ConfigurationHandle - pointer to the context with the resources to free
 */
{
    PMINIPORT_CONFIGURATION_CONTEXT ConfigurationContext = (PMINIPORT_CONFIGURATION_CONTEXT)ConfigurationHandle;
    PMINIPORT_RESOURCE Resource;

    while(!IsListEmpty(&ConfigurationContext->ResourceListHead))
    {
        Resource = (PMINIPORT_RESOURCE)RemoveTailList(&ConfigurationContext->ResourceListHead);
        if(Resource->ResourceType == MINIPORT_RESOURCE_TYPE_MEMORY)
        {
            NDIS_DbgPrint(MAX_TRACE,("freeing 0x%x\n", Resource->Resource));
            ExFreePool(Resource->Resource);
        }

        ExFreePool(Resource);
    }

    ZwClose(ConfigurationContext->Handle);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisOpenConfiguration(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_HANDLE    ConfigurationHandle,
    IN  NDIS_HANDLE     WrapperConfigurationContext)
/*
 * FUNCTION: Opens the configuration key and sets up resource tracking for the returned handle
 * ARGUMENTS:
 *     Status: Pointer to a caller-supplied NDIS_STATUS that is filled in with a return value
 *     ConfigurationHandle: Pointer to an opaque configuration handle returned on success
 *     WrapperConfigurationContext: handle originally passed back from NdisInitializeWrapper
 * RETURNS:
 *     NDIS_STATUS_SUCCESS: the operation completed successfully
 *     NDIS_STATUS_FAILURE: the operation failed
 * NOTES:
 *     I think this is the parameters key; please verify.
 */
{
    HANDLE KeyHandle;
    PMINIPORT_CONFIGURATION_CONTEXT ConfigurationContext;
    PNDIS_WRAPPER_CONTEXT WrapperContext = (PNDIS_WRAPPER_CONTEXT)WrapperConfigurationContext;
    HANDLE RootKeyHandle = WrapperContext->RegistryHandle;

    NDIS_DbgPrint(MAX_TRACE, ("Called\n"));

    *Status = ZwDuplicateObject(NtCurrentProcess(), RootKeyHandle,
                                NtCurrentProcess(), &KeyHandle, 0, 0,
                                DUPLICATE_SAME_ACCESS);
    if(!NT_SUCCESS(*Status))
    {
        NDIS_DbgPrint(MID_TRACE, ("Failed to open registry configuration for this miniport\n"));
        *ConfigurationHandle = NULL;
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    ConfigurationContext = ExAllocatePool(PagedPool, sizeof(MINIPORT_CONFIGURATION_CONTEXT));
    if(!ConfigurationContext)
    {
        NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
        ZwClose(KeyHandle);
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

    KeInitializeSpinLock(&ConfigurationContext->ResourceLock);
    InitializeListHead(&ConfigurationContext->ResourceListHead);

    ConfigurationContext->Handle = KeyHandle;

    *ConfigurationHandle = (NDIS_HANDLE)ConfigurationContext;
    *Status = NDIS_STATUS_SUCCESS;

    NDIS_DbgPrint(MAX_TRACE,("returning success\n"));
}


/*
 * @implemented
 */
VOID
EXPORT
NdisOpenProtocolConfiguration(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_HANDLE    ConfigurationHandle,
    IN  PNDIS_STRING    ProtocolSection)
/*
 * FUNCTION: Open the configuration key and set up resource tracking for the protocol
 * ARGUMENTS:
 *     Status: caller-allocated buffer where status is returned
 *     ConfigurationHandle: spot to return the opaque configuration context
 *     ProtocolSection: configuration string originally passed in to ProtocolBindAdapter
 * RETURNS:
 *     NDIS_STATUS_SUCCESS: the operation was a success
 *     NDIS_STATUS_FAILURE: the operation was not a success
 * NOTES:
 *     I think this is the per-device (adapter) parameters\{ProtocolName} key; please verify.
 */
{
    OBJECT_ATTRIBUTES KeyAttributes;
    UNICODE_STRING KeyNameU;
    HANDLE KeyHandle;
    PMINIPORT_CONFIGURATION_CONTEXT ConfigurationContext;

    KeyNameU.Length = 0;
    KeyNameU.MaximumLength = ProtocolSection->Length + sizeof(PARAMETERS_KEY) + sizeof(UNICODE_NULL);
    KeyNameU.Buffer = ExAllocatePool(PagedPool, KeyNameU.MaximumLength);
    if(!KeyNameU.Buffer)
    {
        NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
        *ConfigurationHandle = NULL;
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    RtlCopyUnicodeString(&KeyNameU, ProtocolSection);
    RtlAppendUnicodeToString(&KeyNameU, PARAMETERS_KEY);
    InitializeObjectAttributes(&KeyAttributes, &KeyNameU, OBJ_CASE_INSENSITIVE, NULL, NULL);

    *Status = ZwOpenKey(&KeyHandle, KEY_ALL_ACCESS, &KeyAttributes);

    ExFreePool(KeyNameU.Buffer);

    if(*Status != NDIS_STATUS_SUCCESS)
    {
        *ConfigurationHandle = NULL;
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    ConfigurationContext = ExAllocatePool(PagedPool, sizeof(MINIPORT_CONFIGURATION_CONTEXT));
    if(!ConfigurationContext)
    {
        NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
        *ConfigurationHandle = NULL;
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    KeInitializeSpinLock(&ConfigurationContext->ResourceLock);
    InitializeListHead(&ConfigurationContext->ResourceListHead);

    ConfigurationContext->Handle = KeyHandle;

    *ConfigurationHandle = (NDIS_HANDLE)ConfigurationContext;
    *Status = NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisReadConfiguration(
    OUT PNDIS_STATUS                    Status,
    OUT PNDIS_CONFIGURATION_PARAMETER   * ParameterValue,
    IN  NDIS_HANDLE                     ConfigurationHandle,
    IN  PNDIS_STRING                    Keyword,
    IN  NDIS_PARAMETER_TYPE             ParameterType)
/*
 * FUNCTION: Read a configuration value from the registry, tracking its resources
 * ARGUMENTS:
 *     Status: points to a place to write status into
 *     ParameterValue: Pointer to receive a newly-allocated parameter structure
 *     ConfigurationHandle: handle originally returned by an open function
 *     Keyword: Value name to read, or one of the following constants:
 *       Environment - returns NdisEnvironmentWindowsNt
 *       ProcessorType - returns NdisProcessorX86 until more architectures are added
 *       NdisVersion - returns NDIS_VERSION
 *     ParameterType: the type of the value to be queried
 * RETURNS:
 *     - A status in Status
 *     - A parameter value in ParameterValue
 */
{
    KEY_VALUE_PARTIAL_INFORMATION *KeyInformation;
    ULONG KeyDataLength;
    PMINIPORT_RESOURCE MiniportResource;
    PMINIPORT_CONFIGURATION_CONTEXT ConfigurationContext = (PMINIPORT_CONFIGURATION_CONTEXT)ConfigurationHandle;

    *ParameterValue = NULL;
    *Status = NDIS_STATUS_FAILURE;

    if(ParameterType != NdisParameterInteger &&
        ParameterType != NdisParameterHexInteger &&
        ParameterType != NdisParameterString &&
        ParameterType != NdisParameterMultiString &&
        ParameterType != NdisParameterBinary
      )
    {
        NDIS_DbgPrint(MID_TRACE,("unsupported parameter type\n"));
        return;
    }

    NDIS_DbgPrint(MAX_TRACE,("requested read of %wZ\n", Keyword));

    if(
        !wcsncmp(Keyword->Buffer, L"Environment", Keyword->Length/sizeof(WCHAR)) &&
        wcslen(L"Environment") == Keyword->Length/sizeof(WCHAR)
    )
    {
        *ParameterValue = ExAllocatePool(PagedPool, sizeof(NDIS_CONFIGURATION_PARAMETER));
        if(!*ParameterValue)
        {
            NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
            *Status = NDIS_STATUS_RESOURCES;
            return;
        }

        MiniportResource = ExAllocatePool(PagedPool, sizeof(MINIPORT_RESOURCE));
        if(!MiniportResource)
        {
            NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
            ExFreePool(*ParameterValue);
            *ParameterValue = NULL;
            *Status = NDIS_STATUS_RESOURCES;
            return;
        }

        MiniportResource->ResourceType = 0;
        MiniportResource->Resource = *ParameterValue;

        NDIS_DbgPrint(MID_TRACE,("inserting 0x%x into the resource list\n",
            MiniportResource->Resource));

        ExInterlockedInsertTailList(&ConfigurationContext->ResourceListHead,
            &MiniportResource->ListEntry, &ConfigurationContext->ResourceLock);

        (*ParameterValue)->ParameterType = NdisParameterInteger;
        (*ParameterValue)->ParameterData.IntegerData = NdisEnvironmentWindowsNt;
        *Status = NDIS_STATUS_SUCCESS;

        return;
    }

    if(
        !wcsncmp(Keyword->Buffer, L"ProcessorType", Keyword->Length/sizeof(WCHAR)) &&
        wcslen(L"ProcessorType") == Keyword->Length/sizeof(WCHAR)
    )
    {
        *ParameterValue = ExAllocatePool(PagedPool, sizeof(NDIS_CONFIGURATION_PARAMETER));
        if(!*ParameterValue)
        {
            NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
            *Status = NDIS_STATUS_RESOURCES;
            return;
        }

        MiniportResource = ExAllocatePool(PagedPool, sizeof(MINIPORT_RESOURCE));
        if(!MiniportResource)
        {
            NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
            ExFreePool(*ParameterValue);
            *ParameterValue = NULL;
            *Status = NDIS_STATUS_RESOURCES;
            return;
        }

        MiniportResource->ResourceType = 0;
        MiniportResource->Resource = *ParameterValue;
        NDIS_DbgPrint(MID_TRACE,("inserting 0x%x into the resource list\n", MiniportResource->Resource));
        ExInterlockedInsertTailList(&ConfigurationContext->ResourceListHead,
            &MiniportResource->ListEntry, &ConfigurationContext->ResourceLock);

        (*ParameterValue)->ParameterType = NdisParameterInteger;
        (*ParameterValue)->ParameterData.IntegerData = NdisProcessorX86;    /* XXX non-portable */
        *Status = NDIS_STATUS_SUCCESS;

        return;
    }

    if(
        !wcsncmp(Keyword->Buffer, L"NdisVersion", Keyword->Length/sizeof(WCHAR)) &&
        wcslen(L"NdisVersion") == Keyword->Length/sizeof(WCHAR)
    )
    {
        *ParameterValue = ExAllocatePool(PagedPool, sizeof(NDIS_CONFIGURATION_PARAMETER));
        if(!*ParameterValue)
        {
            NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
            *Status = NDIS_STATUS_RESOURCES;
            return;
        }

        MiniportResource = ExAllocatePool(PagedPool, sizeof(MINIPORT_RESOURCE));
        if(!MiniportResource)
        {
            NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
            ExFreePool(*ParameterValue);
            *ParameterValue = NULL;
            *Status = NDIS_STATUS_RESOURCES;
            return;
        }

        MiniportResource->ResourceType = 0;
        MiniportResource->Resource = *ParameterValue;
        NDIS_DbgPrint(MID_TRACE,("inserting 0x%x into the resource list\n", MiniportResource->Resource));
        ExInterlockedInsertTailList(&ConfigurationContext->ResourceListHead,
            &MiniportResource->ListEntry, &ConfigurationContext->ResourceLock);

        (*ParameterValue)->ParameterType = NdisParameterInteger;
        (*ParameterValue)->ParameterData.IntegerData = NDIS_VERSION;
        *Status = NDIS_STATUS_SUCCESS;

        NDIS_DbgPrint(MAX_TRACE,("ParameterType = %0x%x, ParameterValue = 0x%x\n",
            (*ParameterValue)->ParameterType, (*ParameterValue)->ParameterData.IntegerData));
        return;
    }

    /* figure out how much buffer i should allocate */
    *Status = ZwQueryValueKey(ConfigurationContext->Handle, Keyword, KeyValuePartialInformation, NULL, 0, &KeyDataLength);
    if(*Status != STATUS_BUFFER_OVERFLOW && *Status != STATUS_BUFFER_TOO_SMALL && *Status != STATUS_SUCCESS)
    {
        NDIS_DbgPrint(MID_TRACE,("ZwQueryValueKey #1 failed for %wZ, status 0x%x\n", Keyword, *Status));
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    /* allocate it */
    KeyInformation = ExAllocatePool(PagedPool, KeyDataLength + sizeof(KEY_VALUE_PARTIAL_INFORMATION));
    if(!KeyInformation)
    {
        NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

    /* grab the value */
    *Status = ZwQueryValueKey(ConfigurationContext->Handle, Keyword, KeyValuePartialInformation,
        KeyInformation, KeyDataLength + sizeof(KEY_VALUE_PARTIAL_INFORMATION), &KeyDataLength);
    if(*Status != STATUS_SUCCESS)
    {
        ExFreePool(KeyInformation);
        NDIS_DbgPrint(MID_TRACE,("ZwQueryValueKey #2 failed for %wZ, status 0x%x\n", Keyword, *Status));
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    switch(ParameterType)
    {
        case NdisParameterInteger:
        case NdisParameterHexInteger:
        {
            UNICODE_STRING str;

            *ParameterValue = ExAllocatePool(PagedPool, sizeof(NDIS_CONFIGURATION_PARAMETER));
            if(!*ParameterValue)
            {
                NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
                ExFreePool(KeyInformation);
                *Status = NDIS_STATUS_RESOURCES;
                return;
            }

            str.Length = str.MaximumLength = (USHORT)KeyInformation->DataLength;
            str.Buffer = (PWCHAR)KeyInformation->Data;

            (*ParameterValue)->ParameterType = ParameterType;

            /*
                 If ParameterType is NdisParameterInteger then the base of str is decimal.
                 If ParameterType is NdisParameterHexInteger then the base of str is hexadecimal.
            */
            if (ParameterType == NdisParameterInteger)
               *Status = RtlUnicodeStringToInteger(&str, 10, &(*ParameterValue)->ParameterData.IntegerData);
            else if (ParameterType == NdisParameterHexInteger)
               *Status = RtlUnicodeStringToInteger(&str, 16, &(*ParameterValue)->ParameterData.IntegerData);


            ExFreePool(KeyInformation);

            if(*Status != STATUS_SUCCESS)
                *Status = NDIS_STATUS_FAILURE;
            else
                *Status = NDIS_STATUS_SUCCESS;

            return;
        }

        case NdisParameterString:
        case NdisParameterMultiString:
        {
            PWCHAR RegData = 0;

            if(KeyInformation->Type != REG_SZ && KeyInformation->Type != REG_MULTI_SZ)
            {
                NDIS_DbgPrint(MID_TRACE,("requested type does not match actual value type\n"));
                ExFreePool(KeyInformation);
                *ParameterValue = NULL;
                *Status = NDIS_STATUS_FAILURE;
                return;
            }

            *ParameterValue = ExAllocatePool(PagedPool, sizeof(NDIS_CONFIGURATION_PARAMETER));
            if(!*ParameterValue)
            {
                NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
                ExFreePool(KeyInformation);
                *Status = NDIS_STATUS_RESOURCES;
                return;
            }

            RegData = ExAllocatePool(PagedPool, KeyInformation->DataLength);
            if(!RegData)
            {
                NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
                ExFreePool(KeyInformation);
                ExFreePool(*ParameterValue);
                *ParameterValue = NULL;
                *Status = NDIS_STATUS_FAILURE;
                return;
            }

            MiniportResource = ExAllocatePool(PagedPool, sizeof(MINIPORT_RESOURCE));
            if(!MiniportResource)
            {
                NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
                ExFreePool(KeyInformation);
                ExFreePool(*ParameterValue);
                *ParameterValue = NULL;
                *Status = NDIS_STATUS_RESOURCES;
                return;
            }

            MiniportResource->ResourceType = 0;
            MiniportResource->Resource = *ParameterValue;
            NDIS_DbgPrint(MID_TRACE,("inserting 0x%x into the resource list\n", MiniportResource->Resource));
            ExInterlockedInsertTailList(&ConfigurationContext->ResourceListHead, &MiniportResource->ListEntry, &ConfigurationContext->ResourceLock);

            memcpy(RegData, KeyInformation->Data, KeyInformation->DataLength);

            (*ParameterValue)->ParameterType = ParameterType;
            (*ParameterValue)->ParameterData.StringData.Length = (USHORT)KeyInformation->DataLength;
            (*ParameterValue)->ParameterData.StringData.Buffer = RegData;

            ExFreePool(KeyInformation);

            *Status = NDIS_STATUS_SUCCESS;

            return;
        }

        case NdisParameterBinary:
        {
            if(KeyInformation->Type != REG_BINARY)
            {
                NDIS_DbgPrint(MIN_TRACE,("requested type does not match actual value type\n"));
                *Status = NDIS_STATUS_FAILURE;
                ExFreePool(KeyInformation);
                return;
            }

            *ParameterValue = ExAllocatePool(PagedPool, sizeof(NDIS_CONFIGURATION_PARAMETER) + KeyInformation->DataLength);
            if(!*ParameterValue)
            {
                NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
                ExFreePool(KeyInformation);
                *Status = NDIS_STATUS_RESOURCES;
                return;
            }

            MiniportResource = ExAllocatePool(PagedPool, sizeof(MINIPORT_RESOURCE));
            if(!MiniportResource)
            {
                NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
                ExFreePool(KeyInformation);
                ExFreePool(*ParameterValue);
                *ParameterValue = NULL;
                *Status = NDIS_STATUS_RESOURCES;
                return;
            }

            MiniportResource->ResourceType = 0;
            MiniportResource->Resource = *ParameterValue;
            NDIS_DbgPrint(MID_TRACE,("inserting 0x%x into the resource list\n", MiniportResource->Resource));
            ExInterlockedInsertTailList(&ConfigurationContext->ResourceListHead, &MiniportResource->ListEntry, &ConfigurationContext->ResourceLock);

            (*ParameterValue)->ParameterType = ParameterType;
            memcpy(&((*ParameterValue)->ParameterData.BinaryData), KeyInformation->Data, KeyInformation->DataLength);

            ExFreePool(KeyInformation);

            *Status = NDIS_STATUS_SUCCESS;

            return;
        }
    }
}


UCHAR UnicodeToHexByte(WCHAR chr)
/*
 * FUNCTION: Converts a unicode hex character to its numerical value
 * ARGUMENTS:
 *     chr: Unicode character to convert
 * RETURNS:
 *     The numerical value of chr
 */
{
    switch(chr)
    {
        case L'0': return 0;
        case L'1': return 1;
        case L'2': return 2;
        case L'3': return 3;
        case L'4': return 4;
        case L'5': return 5;
        case L'6': return 6;
        case L'7': return 7;
        case L'8': return 8;
        case L'9': return 9;
        case L'A':
        case L'a':
            return 10;
        case L'B':
        case L'b':
            return 11;
        case L'C':
        case L'c':
            return 12;
        case L'D':
        case L'd':
            return 13;
        case L'E':
        case L'e':
            return 14;
        case L'F':
        case L'f':
            return 15;
    }
    return -1;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisReadNetworkAddress(
    OUT PNDIS_STATUS    Status,
    OUT PVOID           * NetworkAddress,
    OUT PUINT           NetworkAddressLength,
    IN  NDIS_HANDLE     ConfigurationHandle)
/*
 * FUNCTION: Reads the network address from the registry
 * ARGUMENTS:
 *     Status - variable to receive status
 *     NetworkAddress - pointer to a buffered network address array
 *     NetworkAddressLength - length of the NetworkAddress array
 *     ConfigurationHandle: handle passed back from one of the open routines
 * RETURNS:
 *     NDIS_STATUS_SUCCESS on success
 *     NDIS_STATUS_FAILURE on failure
 *     The network address is placed in the NetworkAddress buffer
 */
{
    PMINIPORT_CONFIGURATION_CONTEXT ConfigurationContext = (PMINIPORT_CONFIGURATION_CONTEXT)ConfigurationHandle;
    PMINIPORT_RESOURCE MiniportResource = NULL;
    PNDIS_CONFIGURATION_PARAMETER ParameterValue = NULL;
    NDIS_STRING Keyword;
    UINT *IntArray = 0;
    int i;

    /* FIXME - We don't quite support this yet due to buggy code below */
      {
        *Status = NDIS_STATUS_FAILURE;
        return;
      }

    *NetworkAddress = NULL;
    *NetworkAddressLength = 6;/* XXX magic constant */

    NdisInitUnicodeString(&Keyword, L"NetworkAddress");
    NdisReadConfiguration(Status, &ParameterValue, ConfigurationHandle, &Keyword, NdisParameterString);
    if(*Status != NDIS_STATUS_SUCCESS)
    {
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    /* 6 bytes for ethernet, tokenring, fddi, everything else? */
    IntArray = ExAllocatePool(PagedPool, 6*sizeof(UINT));
    if(!IntArray)
    {
        NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

    MiniportResource = ExAllocatePool(PagedPool, sizeof(MINIPORT_RESOURCE));
    if(!MiniportResource)
    {
        NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    MiniportResource->ResourceType = 0;
    MiniportResource->Resource = IntArray;
    NDIS_DbgPrint(MID_TRACE,("inserting 0x%x into the resource list\n", MiniportResource->Resource));
    ExInterlockedInsertTailList(&ConfigurationContext->ResourceListHead, &MiniportResource->ListEntry, &ConfigurationContext->ResourceLock);

    /* convert from string to bytes */
    for(i=0; i<6; i++)
    {
        IntArray[i] = (UnicodeToHexByte((ParameterValue->ParameterData.StringData.Buffer)[2*i]) << 4) +
                UnicodeToHexByte((ParameterValue->ParameterData.StringData.Buffer)[2*i+1]);
    }

    *NetworkAddress = IntArray;

    *Status = NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisOpenConfigurationKeyByIndex(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     ConfigurationHandle,
    IN  ULONG           Index,
    OUT PNDIS_STRING    KeyName,
    OUT PNDIS_HANDLE    KeyHandle)
/*
 * FUNCTION: Opens a configuration subkey by index number
 * ARGUMENTS:
 *     Status: pointer to an NDIS_STATUS to receive status info
 *     ConfigurationHandle: the handle passed back from a previous open function
 *     Index: the zero-based index of the subkey to open
 *     KeyName: the name of the key that was opened
 *     KeyHandle: a handle to the key that was opened
 * RETURNS:
 *     NDIS_STATUS_SUCCESS on success
 *     NDIS_STATUS_FAILURE on failure
 *     KeyName holds the name of the opened key
 *     KeyHandle holds a handle to the new key
 */
{
    KEY_BASIC_INFORMATION *KeyInformation;
    ULONG KeyInformationLength;
    OBJECT_ATTRIBUTES KeyAttributes;
    NDIS_HANDLE RegKeyHandle;
    PMINIPORT_CONFIGURATION_CONTEXT ConfigurationContext;

    *Status = ZwEnumerateKey(ConfigurationHandle, Index, KeyBasicInformation, NULL, 0, &KeyInformationLength);
    if(*Status != STATUS_BUFFER_TOO_SMALL && *Status != STATUS_BUFFER_OVERFLOW && *Status != STATUS_SUCCESS)
    {
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    KeyInformation = ExAllocatePool(PagedPool, KeyInformationLength + sizeof(KEY_BASIC_INFORMATION));
    if(!KeyInformation)
    {
        NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    *Status = ZwEnumerateKey(ConfigurationHandle, Index, KeyBasicInformation, KeyInformation,
        KeyInformationLength + sizeof(KEY_BASIC_INFORMATION), &KeyInformationLength);

    if(*Status != STATUS_SUCCESS)
    {
        ExFreePool(KeyInformation);
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    /* should i fail instead if the passed-in string isn't long enough? */
    wcsncpy(KeyName->Buffer, KeyInformation->Name, KeyName->MaximumLength/sizeof(WCHAR));
    KeyName->Length = (USHORT)KeyInformation->NameLength;

    InitializeObjectAttributes(&KeyAttributes, KeyName, OBJ_CASE_INSENSITIVE, ConfigurationHandle, NULL);

    *Status = ZwOpenKey(&RegKeyHandle, KEY_ALL_ACCESS, &KeyAttributes);

    ExFreePool(KeyInformation);

    if(*Status != STATUS_SUCCESS)
    {
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    ConfigurationContext = ExAllocatePool(PagedPool, sizeof(MINIPORT_CONFIGURATION_CONTEXT));
    if(!ConfigurationContext)
    {
        NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    KeInitializeSpinLock(&ConfigurationContext->ResourceLock);
    InitializeListHead(&ConfigurationContext->ResourceListHead);

    ConfigurationContext->Handle = RegKeyHandle;

    *KeyHandle = (NDIS_HANDLE)ConfigurationContext;

    *Status = NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisOpenConfigurationKeyByName(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     ConfigurationHandle,
    IN  PNDIS_STRING    KeyName,
    OUT PNDIS_HANDLE    KeyHandle)
/*
 * FUNCTION: Opens a configuration subkey by name
 * ARGUMENTS:
 *     Status: points to an NDIS_STATUS where status is returned
 *     ConfigurationHandle: handle returned by a previous open call
 *     KeyName: the name of the key to open
 *     KeyHandle: a handle to the opened key
 * RETURNS:
 *     NDIS_STATUS_SUCCESS on success
 *     NDIS_STATUS_FAILURE on failure
 *     KeyHandle holds a handle to the newly-opened key
 * NOTES:
 */
{
    PMINIPORT_CONFIGURATION_CONTEXT ConfigurationContext;
    OBJECT_ATTRIBUTES KeyAttributes;
    NDIS_HANDLE RegKeyHandle;

    InitializeObjectAttributes(&KeyAttributes, KeyName, OBJ_CASE_INSENSITIVE, ConfigurationHandle, 0);
    *Status = ZwOpenKey(&RegKeyHandle, KEY_ALL_ACCESS, &KeyAttributes);

    if(*Status != STATUS_SUCCESS)
    {
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    ConfigurationContext = ExAllocatePool(PagedPool, sizeof(MINIPORT_CONFIGURATION_CONTEXT));
    if(!ConfigurationContext)
    {
        NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    KeInitializeSpinLock(&ConfigurationContext->ResourceLock);
    InitializeListHead(&ConfigurationContext->ResourceListHead);

    ConfigurationContext->Handle = RegKeyHandle;

    *KeyHandle = (NDIS_HANDLE)ConfigurationContext;

    *Status = NDIS_STATUS_SUCCESS;
}

