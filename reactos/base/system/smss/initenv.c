/*
 * PROJECT:         ReactOS Session Manager
 * LICENSE:         GPL v2 or later - See COPYING in the top level directory
 * FILE:            base/system/smss/initenv.c
 * PURPOSE:         Environment initialization.
 * PROGRAMMERS:     ReactOS Development Team
 */

/* INCLUDES ******************************************************************/
#include "smss.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS */

PWSTR SmSystemEnvironment = NULL;


/* FUNCTIONS */

NTSTATUS
SmCreateEnvironment(VOID)
{
    return RtlCreateEnvironment(FALSE, &SmSystemEnvironment);
}


static NTSTATUS
SmpSetEnvironmentVariable(IN PVOID Context,
                          IN PWSTR ValueName,
                          IN PWSTR ValueData)
{
    UNICODE_STRING EnvVariable;
    UNICODE_STRING EnvValue;

    RtlInitUnicodeString(&EnvVariable,
                         ValueName);
    RtlInitUnicodeString(&EnvValue,
                         ValueData);
    return RtlSetEnvironmentVariable(Context,
                              &EnvVariable,
                              &EnvValue);
}


static NTSTATUS NTAPI
SmpEnvironmentQueryRoutine(IN PWSTR ValueName,
                           IN ULONG ValueType,
                           IN PVOID ValueData,
                           IN ULONG ValueLength,
                           IN PVOID Context,
                           IN PVOID EntryContext)
{
    DPRINT("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);

    if (ValueType != REG_SZ && ValueType != REG_EXPAND_SZ)
        return STATUS_SUCCESS;

    DPRINT("ValueData '%S'\n", (PWSTR)ValueData);
    return SmpSetEnvironmentVariable(Context,ValueName,(PWSTR)ValueData);
}


NTSTATUS
SmSetEnvironmentVariables(VOID)
{
    SYSTEM_BASIC_INFORMATION BasicInformation;

    RTL_QUERY_REGISTRY_TABLE QueryTable[3];
    UNICODE_STRING Identifier;
    UNICODE_STRING VendorIdentifier;
    WCHAR Buffer[256];

    UNICODE_STRING EnvironmentKeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE EnvironmentKey;
    UNICODE_STRING VariableName;
    PWSTR VariableData;

    NTSTATUS Status;


    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &BasicInformation,
                                      sizeof(SYSTEM_BASIC_INFORMATION),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SM: Failed to retrieve system basic information (Status %08lx)", Status);
        return Status;
    }

    RtlInitUnicodeString(&EnvironmentKeyName,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager\\Environment");
    InitializeObjectAttributes(&ObjectAttributes,
                               &EnvironmentKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open the system environment key */
    Status = NtOpenKey(&EnvironmentKey,
                       GENERIC_WRITE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SM: Failed to open the environment key (Status %08lx)", Status);
        return Status;
    }

    /* Set the 'NUMBER_OF_PROCESSORS' system environment variable */
    RtlInitUnicodeString(&VariableName,
                         L"NUMBER_OF_PROCESSORS");

    swprintf(Buffer, L"%lu", BasicInformation.NumberOfProcessors);

    Status = NtSetValueKey(EnvironmentKey,
                           &VariableName,
                           0,
                           REG_SZ,
                           Buffer,
                           (wcslen(Buffer) + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SM: Failed to set the NUMBER_OF_PROCESSORS environment variable (Status %08lx)", Status);
        goto done;
    }

    /* Set the 'OS' system environment variable */
    RtlInitUnicodeString(&VariableName,
                         L"OS");

    VariableData = L"ReactOS";

    Status = NtSetValueKey(EnvironmentKey,
                           &VariableName,
                           0,
                           REG_SZ,
                           VariableData,
                           (wcslen(VariableData) + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SM: Failed to set the OS environment variable (Status %08lx)", Status);
        goto done;
    }

    /* Set the 'PROCESSOR_ARCHITECTURE' system environment variable */
    RtlInitUnicodeString(&VariableName,
                         L"PROCESSOR_ARCHITECTURE");

#ifdef _M_IX86
    VariableData = L"x86";
#elif _M_MD64
    VariableData = L"AMD64";
#elif _M_ARM
    VariableData = L"ARM";
#elif _M_PPC
    VariableData = L"PPC";
#else
    #error "Unsupported Architecture!\n"
#endif

    Status = NtSetValueKey(EnvironmentKey,
                           &VariableName,
                           0,
                           REG_SZ,
                           VariableData,
                           (wcslen(VariableData) + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SM: Failed to set the PROCESSOR_ARCHITECTURE environment variable (Status %08lx)", Status);
        goto done;
    }

done:
    NtClose(EnvironmentKey);


    /* Set the 'PROCESSOR_IDENTIFIER' system environment variable */
    RtlInitUnicodeString(&Identifier, NULL);
    RtlInitUnicodeString(&VendorIdentifier, NULL);

    RtlZeroMemory(&QueryTable,
                  sizeof(QueryTable));

    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].Name = L"Identifier";
    QueryTable[0].EntryContext = &Identifier;

    QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[1].Name = L"VendorIdentifier";
    QueryTable[1].EntryContext = &VendorIdentifier;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    L"\\Registry\\Machine\\Hardware\\Description\\System\\CentralProcessor\\0",
                                    QueryTable,
                                    NULL,
                                    NULL);
    if (NT_SUCCESS(Status))
    {
        DPRINT("SM: szIdentifier: %wZ\n", &Identifier);
        DPRINT("SM: szVendorIdentifier: %wZ\n", &VendorIdentifier);

        swprintf(Buffer, L"%wZ, %wZ", &Identifier, &VendorIdentifier);

        RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                              L"Session Manager\\Environment",
                              L"PROCESSOR_IDENTIFIER",
                              REG_SZ,
                              Buffer,
                              (wcslen(Buffer) + 1) * sizeof(WCHAR));
    }

    RtlFreeUnicodeString(&Identifier);
    RtlFreeUnicodeString(&VendorIdentifier);

    return STATUS_SUCCESS;
}


/**********************************************************************
 *  Set environment variables from registry
 */
NTSTATUS
SmUpdateEnvironment(VOID)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    WCHAR ValueBuffer[MAX_PATH];
    NTSTATUS Status;

    /*
     * The following environment variables must be set prior to reading
     * other variables from the registry.
     *
     * Variables (example):
     *    SystemRoot = "C:\reactos"
     *    SystemDrive = "C:"
     */

    /* Copy system root into value buffer */
    wcscpy(ValueBuffer,
           SharedUserData->NtSystemRoot);

    /* Set SystemRoot = "C:\reactos" */
    SmpSetEnvironmentVariable(&SmSystemEnvironment, L"SystemRoot", ValueBuffer);

    /* Cut off trailing path */
    ValueBuffer[2] = 0;

    /* Set SystemDrive = "C:" */
    SmpSetEnvironmentVariable(&SmSystemEnvironment, L"SystemDrive", ValueBuffer);

    /* Read system environment from the registry. */
    RtlZeroMemory(&QueryTable,
                  sizeof(QueryTable));

    QueryTable[0].QueryRoutine = SmpEnvironmentQueryRoutine;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                                    L"Session Manager\\Environment",
                                    QueryTable,
                                    &SmSystemEnvironment,
                                    SmSystemEnvironment);

    return Status;
}

/* EOF */
