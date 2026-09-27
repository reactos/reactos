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
 *     - The NDIS_HANDLE Configuration context is no longer a registry handle.  An opaque struct
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

#include <ntifs.h>

#define PARAMETERS_KEY L"Parameters"     /* The parameters subkey under the device-specific key */

extern ULONG NdisEnableTxRxFlowControl;

static
PMINIPORT_CONFIGURATION_CONTEXT
AllocateConfigurationContext(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes)
{
    PMINIPORT_CONFIGURATION_CONTEXT ConfigurationContext;
    HANDLE KeyHandle;
    NTSTATUS Status;

    Status = ZwOpenKey(&KeyHandle, KEY_ALL_ACCESS, ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        NDIS_DbgPrint(MIN_TRACE, ("Failed to open registry configuration for this miniport "
                                  "'%wZ', status 0x%lx\n",
                                  ObjectAttributes->ObjectName, Status));
        return NULL;
    }

    ConfigurationContext = ExAllocatePoolUninitialized(NonPagedPool,
                                                       sizeof(*ConfigurationContext),
                                                       TAG_CONFIG);
    if (!ConfigurationContext)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        ZwClose(KeyHandle);
        return NULL;
    }

    ConfigurationContext->Handle = KeyHandle;

    KeInitializeSpinLock(&ConfigurationContext->ResourceLock);
    InitializeListHead(&ConfigurationContext->ResourceListHead);

    return ConfigurationContext;
}

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
    ULONG ParameterType;
    ULONG DataSize;
    PVOID Data;
    WCHAR Buff[11];

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    NDIS_DbgPrint(MID_TRACE, ("Parameter type: %d\n", ParameterValue->ParameterType));

    /* reset parameter type to standard reg types */
    switch(ParameterValue->ParameterType)
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
                      (ParameterValue->ParameterType == NdisParameterInteger) ? 10 : 16, &Str)))
                 {
                      NDIS_DbgPrint(MIN_TRACE, ("RtlIntegerToUnicodeString failed (%x)\n", *Status));
                      *Status = NDIS_STATUS_FAILURE;
                      return;
                 }
                 Data = Str.Buffer;
                 DataSize = Str.Length;
             }
             break;
        case NdisParameterString:
        case NdisParameterMultiString:
            ParameterType = (ParameterValue->ParameterType == NdisParameterString) ? REG_SZ : REG_MULTI_SZ;
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
            *Status = NDIS_STATUS_NOT_SUPPORTED;
            return;
    }

    *Status = ZwSetValueKey(((PMINIPORT_CONFIGURATION_CONTEXT)ConfigurationHandle)->Handle,
            Keyword, 0, ParameterType, Data, DataSize);

    if(*Status != STATUS_SUCCESS) {
        NDIS_DbgPrint(MIN_TRACE, ("ZwSetValueKey failed (%x)\n", *Status));
        *Status = NDIS_STATUS_FAILURE;
    } else
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
    PNDIS_CONFIGURATION_PARAMETER ParameterValue;
    PLIST_ENTRY CurrentEntry;

    while((CurrentEntry = ExInterlockedRemoveHeadList(&ConfigurationContext->ResourceListHead,
                                                      &ConfigurationContext->ResourceLock)) != NULL)
    {
        Resource = CONTAINING_RECORD(CurrentEntry, MINIPORT_RESOURCE, ListEntry);
        switch(Resource->ResourceType)
        {
          case MINIPORT_RESOURCE_TYPE_REGISTRY_DATA:
            ParameterValue = Resource->Resource;

            switch (ParameterValue->ParameterType)
            {
               case NdisParameterBinary:
                 ExFreePoolWithTag(ParameterValue->ParameterData.BinaryData.Buffer, TAG_CONFIG);
                 break;

               case NdisParameterString:
               case NdisParameterMultiString:
                 ExFreePoolWithTag(ParameterValue->ParameterData.StringData.Buffer, TAG_CONFIG);
                 break;

               default:
                 break;
            }

          /* Fall through to free NDIS_CONFIGURATION_PARAMETER struct */

          case MINIPORT_RESOURCE_TYPE_MEMORY:
            NDIS_DbgPrint(MAX_TRACE,("freeing 0x%x\n", Resource->Resource));
            ExFreePoolWithTag(Resource->Resource, TAG_CONFIG);
            break;

          default:
            NDIS_DbgPrint(MIN_TRACE,("Unknown resource type: %d\n", Resource->ResourceType));
            break;
        }

        ExFreePoolWithTag(Resource, TAG_CONFIG);
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
    PMINIPORT_CONFIGURATION_CONTEXT ConfigurationContext;
    PNDIS_WRAPPER_CONTEXT WrapperContext = (PNDIS_WRAPPER_CONTEXT)WrapperConfigurationContext;
    HANDLE RootKeyHandle = WrapperContext->RegistryHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING NoName = RTL_CONSTANT_STRING(L"");

    NDIS_DbgPrint(MAX_TRACE, ("Called\n"));

    *ConfigurationHandle = NULL;

    InitializeObjectAttributes(&ObjectAttributes,
                               &NoName,
                               OBJ_KERNEL_HANDLE,
                               RootKeyHandle,
                               NULL);

    ConfigurationContext = AllocateConfigurationContext(&ObjectAttributes);
    if(!ConfigurationContext)
    {
        NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

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
    PMINIPORT_CONFIGURATION_CONTEXT ConfigurationContext;

    KeyNameU.Length = 0;
    KeyNameU.MaximumLength = ProtocolSection->Length + sizeof(PARAMETERS_KEY) + sizeof(UNICODE_NULL);
    KeyNameU.Buffer = ExAllocatePoolUninitialized(PagedPool, KeyNameU.MaximumLength, TAG_CONFIG);
    if(!KeyNameU.Buffer)
    {
        NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
        *ConfigurationHandle = NULL;
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    RtlCopyUnicodeString(&KeyNameU, ProtocolSection);
    RtlAppendUnicodeToString(&KeyNameU, PARAMETERS_KEY);
    InitializeObjectAttributes(&KeyAttributes, &KeyNameU, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    ConfigurationContext = AllocateConfigurationContext(&KeyAttributes);
    ExFreePoolWithTag(KeyNameU.Buffer, TAG_CONFIG);
    if(!ConfigurationContext)
    {
        NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
        *ConfigurationHandle = NULL;
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    *ConfigurationHandle = (NDIS_HANDLE)ConfigurationContext;
    *Status = NDIS_STATUS_SUCCESS;
}

static
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
    return 0xFF;
}

static
BOOLEAN
IsValidNumericString(PNDIS_STRING String, ULONG Base)
/*
 * FUNCTION: Determines if a string is a valid number
 * ARGUMENTS:
 *     String: Unicode string to evaluate
 * RETURNS:
 *     TRUE if it is valid, FALSE if not
 */
{
    ULONG i;

    /* I don't think this will ever happen, but we warn it if it does */
    if (String->Length == 0)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Got an empty string; not sure what to do here\n"));
        return FALSE;
    }

    for (i = 0; i < String->Length / sizeof(WCHAR); i++)
    {
        /* Skip any NULL characters we find */
        if (String->Buffer[i] == UNICODE_NULL)
            continue;

        /* Make sure the character is valid for a numeric string of this base */
        if (UnicodeToHexByte(String->Buffer[i]) >= Base)
            return FALSE;
    }

    /* It's valid */
    return TRUE;
}

/*
 * @implemented
 */
VOID
EXPORT
NdisReadConfiguration(
    _At_(*Status, _Must_inspect_result_) _Out_ PNDIS_STATUS Status,
    _Out_ PNDIS_CONFIGURATION_PARAMETER* ParameterValue,
    _In_ NDIS_HANDLE ConfigurationHandle,
    _In_ PNDIS_STRING Keyword,
    _In_ NDIS_PARAMETER_TYPE ParameterType)
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
    PMINIPORT_CONFIGURATION_CONTEXT ConfigurationContext = ConfigurationHandle;
    PKEY_VALUE_PARTIAL_INFORMATION KeyInformation = NULL;
    PNDIS_CONFIGURATION_PARAMETER ConfigParameter = NULL;
    PMINIPORT_RESOURCE MiniportResource = NULL;
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;
    NTSTATUS NtStatus;
    ULONG LocalUlong;
    PVOID RegData;
    ULONG RegType;
    ULONG RegDataLength;
    static const UNICODE_STRING NdisVersionKey = RTL_CONSTANT_STRING(L"NdisVersion");
    static const UNICODE_STRING EnvironmentKey = RTL_CONSTANT_STRING(L"Environment");
    static const UNICODE_STRING ProcessorTypeKey = RTL_CONSTANT_STRING(L"ProcessorType");
    static const UNICODE_STRING FlowControlKey = RTL_CONSTANT_STRING(L"*FlowControl");

    NDIS_DbgPrint(MAX_TRACE, ("requested read type %d of '%wZ'\n", ParameterType, Keyword));

    ASSERT(ConfigurationContext);

    /* Check for predefined entry names */
    if (!NdisEnableTxRxFlowControl && RtlEqualUnicodeString(Keyword, &FlowControlKey, TRUE))
    {
        NDIS_DbgPrint(MID_TRACE, ("Disable TX/RX flow control for this miniport\n"));
        LocalUlong = 0;

        RegType = REG_DWORD;
        RegData = &LocalUlong;
        RegDataLength = sizeof(LocalUlong);
    }
    else if (RtlEqualUnicodeString(Keyword, &NdisVersionKey, TRUE))
    {
        LocalUlong = NDIS_VERSION;

        RegType = REG_DWORD;
        RegData = &LocalUlong;
        RegDataLength = sizeof(LocalUlong);
    }
    else if (RtlEqualUnicodeString(Keyword, &EnvironmentKey, TRUE))
    {
        LocalUlong = NdisEnvironmentWindowsNt;

        RegType = REG_DWORD;
        RegData = &LocalUlong;
        RegDataLength = sizeof(LocalUlong);
    }
    else if (RtlEqualUnicodeString(Keyword, &ProcessorTypeKey, TRUE))
    {
#if defined(_M_IX86)
        LocalUlong = NdisProcessorX86;
#elif defined(_M_AMD64)
        LocalUlong = NdisProcessorAmd64;
#elif defined(_M_ARM)
        LocalUlong = NdisProcessorArm;
#elif defined(_M_ARM64)
        LocalUlong = NdisProcessorArm64;
#elif defined(_M_MPPC)
        LocalUlong = NdisProcessorPpc;
#elif defined(_M_MIPS)
        LocalUlong = NdisProcessorMips;
#elif defined(_M_ALPHA)
        LocalUlong = NdisProcessorAlpha;
#elif defined(_M_IA64)
        LocalUlong = NdisProcessorIA64;
#else
#error Unknown architecture
#endif

        RegType = REG_DWORD;
        RegData = &LocalUlong;
        RegDataLength = sizeof(LocalUlong);
    }
    else
    {
        ULONG KeyDataLength, RetryCount;

        /* The most common case is NdisParameterInteger (REG_DWORD) */
        KeyDataLength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + sizeof(ULONG);

        for (RetryCount = 0; RetryCount < 2; ++RetryCount)
        {
            KeyInformation = ExAllocatePoolUninitialized(PagedPool, KeyDataLength, TAG_CONFIG);
            if (!KeyInformation)
            {
                NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
                *Status = NDIS_STATUS_RESOURCES;
                return;
            }

            /* Query the value */
            NtStatus = ZwQueryValueKey(ConfigurationContext->Handle,
                                       Keyword,
                                       KeyValuePartialInformation,
                                       KeyInformation,
                                       KeyDataLength,
                                       &KeyDataLength);

            /* Increase the allocation size if it's too small */
            if (NtStatus == STATUS_BUFFER_OVERFLOW || NtStatus == STATUS_BUFFER_TOO_SMALL)
            {
                ExFreePoolWithTag(KeyInformation, TAG_CONFIG);
                KeyInformation = NULL;
                continue;
            }

            break;
        }

        if (!NT_SUCCESS(NtStatus))
        {
            NDIS_DbgPrint(MIN_TRACE, ("ZwQueryValueKey failed for '%wZ', status 0x%lx\n",
                                      Keyword, NtStatus));
            NdisStatus = NDIS_STATUS_FAILURE;
            goto Exit;
        }

        RegType = KeyInformation->Type;
        RegData = &KeyInformation->Data[0];
        RegDataLength = KeyInformation->DataLength;
    }

    MiniportResource = ExAllocatePoolUninitialized(PagedPool,
                                                   sizeof(*MiniportResource),
                                                   TAG_CONFIG);
    if (!MiniportResource)
    {
       NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
       NdisStatus = NDIS_STATUS_RESOURCES;
       goto Exit;
    }

    ConfigParameter = ExAllocatePoolZero(PagedPool, sizeof(*ConfigParameter), TAG_CONFIG);
    if (!ConfigParameter)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        NdisStatus = NDIS_STATUS_RESOURCES;
        goto Exit;
    }

    switch (RegType)
    {
        case REG_DWORD:
        {
            ASSERT(RegDataLength == sizeof(ULONG));

            ConfigParameter->ParameterType = NdisParameterInteger;
            ConfigParameter->ParameterData.IntegerData = *(PULONG)RegData;

            NDIS_DbgPrint(MAX_TRACE, ("NdisParameterInteger 0x%lX (%lu)\n",
                                      ConfigParameter->ParameterData.IntegerData,
                                      ConfigParameter->ParameterData.IntegerData));
            break;
        }
        case REG_BINARY:
        {
            PVOID Buffer;

            NDIS_DbgPrint(MAX_TRACE, ("NdisParameterBinary\n"));

            Buffer = ExAllocatePoolUninitialized(PagedPool, RegDataLength, TAG_CONFIG);
            if (!Buffer)
            {
                NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
                NdisStatus = NDIS_STATUS_RESOURCES;
                goto Exit;
            }
            RtlCopyMemory(Buffer, RegData, RegDataLength);

            ConfigParameter->ParameterData.BinaryData.Buffer = Buffer;
            ConfigParameter->ParameterData.BinaryData.Length = RegDataLength;
            break;
        }
        case REG_MULTI_SZ:
        {
            PVOID Buffer;

            NDIS_DbgPrint(MAX_TRACE, ("NdisParameterMultiString\n"));

            Buffer = ExAllocatePoolUninitialized(PagedPool, RegDataLength, TAG_CONFIG);
            if (!Buffer)
            {
                NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
                NdisStatus = NDIS_STATUS_RESOURCES;
                goto Exit;
            }
            RtlCopyMemory(Buffer, RegData, RegDataLength);

            ConfigParameter->ParameterType = NdisParameterMultiString;
            ConfigParameter->ParameterData.StringData.Buffer = Buffer;
            ConfigParameter->ParameterData.StringData.Length = RegDataLength;
            break;
        }
        case REG_SZ:
        {
            UNICODE_STRING String;
            ULONG Base;
            PVOID Buffer;

            if (ParameterType == NdisParameterInteger)
                Base = 10;
            else if (ParameterType == NdisParameterHexInteger)
                Base = 16;
            else
                Base = 0;

            String.Length = String.MaximumLength = (USHORT)RegDataLength;
            String.Buffer = RegData;

            if (Base != 0 && IsValidNumericString(&String, Base))
            {
                NtStatus = RtlUnicodeStringToInteger(&String,
                                                     Base,
                                                     &ConfigParameter->ParameterData.IntegerData);
                ASSERT(NT_SUCCESS(NtStatus));

                NDIS_DbgPrint(MAX_TRACE, ("NdisParameter(Hex)Integer 0x%lX (%lu)\n",
                                          ConfigParameter->ParameterData.IntegerData,
                                          ConfigParameter->ParameterData.IntegerData));

                /*
                 * MSDN documents that this is returned for all integers,
                 * regardless of the ParameterType passed in.
                 */
                ConfigParameter->ParameterType = NdisParameterInteger;
            }
            else
            {
                NDIS_DbgPrint(MAX_TRACE, ("NdisParameterString\n"));

                ConfigParameter->ParameterType = NdisParameterString;

                Buffer = ExAllocatePoolUninitialized(PagedPool,
                                                     RegDataLength,
                                                     TAG_CONFIG);
                if (!Buffer)
                {
                    NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
                    NdisStatus = NDIS_STATUS_RESOURCES;
                    goto Exit;
                }
                RtlCopyMemory(Buffer, RegData, RegDataLength);

                ConfigParameter->ParameterData.StringData.Buffer = Buffer;
                ConfigParameter->ParameterData.StringData.Length = RegDataLength;
            }
            break;
        }
        default:
        {
            NDIS_DbgPrint(MIN_TRACE, ("Invalid type for NdisReadConfiguration (%d)\n", RegType));
            NDIS_DbgPrint(MIN_TRACE, ("Requested type: %lu\n", ParameterType));
            NDIS_DbgPrint(MIN_TRACE, ("Registry entry: '%wZ'\n", Keyword));

            NdisStatus = NDIS_STATUS_FAILURE;
            goto Exit;
        }
    }

    if ((ConfigParameter->ParameterType != ParameterType) &&
        !((ParameterType == NdisParameterHexInteger) &&
          (ConfigParameter->ParameterType == NdisParameterInteger)))
    {
        NDIS_DbgPrint(MIN_TRACE, ("Parameter type mismatch! (Requested: %d | Received: %d)\n",
                                  ParameterType, ConfigParameter->ParameterType));
        NDIS_DbgPrint(MIN_TRACE, ("Registry entry: '%wZ'\n", Keyword));
    }

    ASSERT(NdisStatus == NDIS_STATUS_SUCCESS);

    *ParameterValue = ConfigParameter;

    MiniportResource->ResourceType = MINIPORT_RESOURCE_TYPE_REGISTRY_DATA;
    MiniportResource->Resource = ConfigParameter;

    NDIS_DbgPrint(MID_TRACE, ("inserting %p into the resource list\n",
                              MiniportResource->Resource));

    ExInterlockedInsertTailList(&ConfigurationContext->ResourceListHead,
                                &MiniportResource->ListEntry,
                                &ConfigurationContext->ResourceLock);

Exit:
    *Status = NdisStatus;

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        if (ConfigParameter)
            ExFreePoolWithTag(ConfigParameter, TAG_CONFIG);
        if (MiniportResource)
            ExFreePoolWithTag(MiniportResource, TAG_CONFIG);
    }

    if (KeyInformation)
        ExFreePoolWithTag(KeyInformation, TAG_CONFIG);
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
    UINT i,j = 0;
    WCHAR Buff[11];
    NDIS_STRING str;

    NdisInitUnicodeString(&Keyword, L"NetworkAddress");
    NdisReadConfiguration(Status, &ParameterValue, ConfigurationHandle, &Keyword, NdisParameterString);
    if(*Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_DbgPrint(MID_TRACE, ("NdisReadConfiguration failed (%x)\n", *Status));
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    if (ParameterValue->ParameterType == NdisParameterInteger)
    {
        NDIS_DbgPrint(MAX_TRACE, ("Read integer data %lx\n",
                                  ParameterValue->ParameterData.IntegerData));

        str.Buffer = Buff;
        str.MaximumLength = (USHORT)sizeof(Buff);
        str.Length = 0;

        *Status = RtlIntegerToUnicodeString(ParameterValue->ParameterData.IntegerData,
                                            10,
                                            &str);

        if (*Status != NDIS_STATUS_SUCCESS)
        {
            NDIS_DbgPrint(MIN_TRACE, ("RtlIntegerToUnicodeString failed (%x)\n", *Status));
            *Status = NDIS_STATUS_FAILURE;
            return;
        }

        NDIS_DbgPrint(MAX_TRACE, ("Converted integer data into %wZ\n", &str));
    }
    else
    {
        ASSERT(ParameterValue->ParameterType == NdisParameterString);
        str = ParameterValue->ParameterData.StringData;
    }

    while (j < str.Length && str.Buffer[j] != '\0') j++;

    *NetworkAddressLength = (j+1) >> 1;

    if ((*NetworkAddressLength) == 0)
    {
        NDIS_DbgPrint(MIN_TRACE,("Empty NetworkAddress registry entry.\n"));
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    IntArray = ExAllocatePoolUninitialized(PagedPool,
                                           (*NetworkAddressLength) * sizeof(*IntArray),
                                           TAG_CONFIG);
    if(!IntArray)
    {
        NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

    MiniportResource = ExAllocatePoolUninitialized(PagedPool,
                                                   sizeof(*MiniportResource),
                                                   TAG_CONFIG);
    if(!MiniportResource)
    {
        NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
        ExFreePoolWithTag(IntArray, TAG_CONFIG);
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

    MiniportResource->ResourceType = MINIPORT_RESOURCE_TYPE_MEMORY;
    MiniportResource->Resource = IntArray;
    NDIS_DbgPrint(MID_TRACE,("inserting 0x%x into the resource list\n", MiniportResource->Resource));
    ExInterlockedInsertTailList(&ConfigurationContext->ResourceListHead, &MiniportResource->ListEntry, &ConfigurationContext->ResourceLock);

    /* convert from string to bytes */
    for(i=0; i<(*NetworkAddressLength); i++)
    {
        IntArray[i] = (UnicodeToHexByte((str.Buffer)[2*i]) << 4) +
                UnicodeToHexByte((str.Buffer)[2*i+1]);
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
    PMINIPORT_CONFIGURATION_CONTEXT ConfigurationContext;

    *KeyHandle = NULL;

    *Status = ZwEnumerateKey(ConfigurationHandle, Index, KeyBasicInformation, NULL, 0, &KeyInformationLength);
    if(*Status != STATUS_BUFFER_TOO_SMALL && *Status != STATUS_BUFFER_OVERFLOW && *Status != STATUS_SUCCESS)
    {
        NDIS_DbgPrint(MIN_TRACE, ("ZwEnumerateKey failed (%x)\n", *Status));
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    KeyInformation = ExAllocatePoolUninitialized(PagedPool,
                                                 KeyInformationLength + sizeof(*KeyInformation),
                                                 TAG_CONFIG);
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
        NDIS_DbgPrint(MIN_TRACE, ("ZwEnumerateKey failed (%x)\n", *Status));
        ExFreePoolWithTag(KeyInformation, TAG_CONFIG);
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    /* should i fail instead if the passed-in string isn't long enough? */
    wcsncpy(KeyName->Buffer, KeyInformation->Name, KeyName->MaximumLength/sizeof(WCHAR));
    KeyName->Length = (USHORT)min(KeyInformation->NameLength, KeyName->MaximumLength);

    InitializeObjectAttributes(&KeyAttributes, KeyName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, ConfigurationHandle, NULL);

    ConfigurationContext = AllocateConfigurationContext(&KeyAttributes);
    ExFreePoolWithTag(KeyInformation, TAG_CONFIG);
    if(!ConfigurationContext)
    {
        NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

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

    *KeyHandle = NULL;

    InitializeObjectAttributes(&KeyAttributes, KeyName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, ConfigurationHandle, 0);

    ConfigurationContext = AllocateConfigurationContext(&KeyAttributes);
    if(!ConfigurationContext)
    {
        NDIS_DbgPrint(MIN_TRACE,("Insufficient resources.\n"));
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    *KeyHandle = (NDIS_HANDLE)ConfigurationContext;
    *Status = NDIS_STATUS_SUCCESS;
}
