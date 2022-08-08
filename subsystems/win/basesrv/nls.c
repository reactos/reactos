/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Base API Server DLL
 * FILE:            subsystems/win/basesrv/nls.c
 * PURPOSE:         National Language Support (NLS)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "basesrv.h"

#include <ndk/mmfuncs.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

RTL_CRITICAL_SECTION NlsCacheCriticalSection;
PNLS_USER_INFO pNlsRegUserInfo;

BOOLEAN BaseSrvKernel32DelayLoadComplete;
HANDLE BaseSrvKernel32DllHandle;
UNICODE_STRING BaseSrvKernel32DllPath;

POPEN_DATA_FILE pOpenDataFile;
PVOID /*PGET_DEFAULT_SORTKEY_SIZE */ pGetDefaultSortkeySize;
PVOID /*PGET_LINGUIST_LANG_SIZE*/ pGetLinguistLangSize;
PVOID /*PNLS_CONVERT_INTEGER_TO_STRING*/ pNlsConvertIntegerToString;
PVOID /*PVALIDATE_LCTYPE*/ pValidateLCType;
PVALIDATE_LOCALE pValidateLocale;
PGET_NLS_SECTION_NAME pGetNlsSectionName;
PVOID /*PGET_USER_DEFAULT_LANGID*/ pGetUserDefaultLangID;
PGET_CP_FILE_NAME_FROM_REGISTRY pGetCPFileNameFromRegistry;
PCREATE_NLS_SECURTY_DESCRIPTOR pCreateNlsSecurityDescriptor;

BASESRV_KERNEL_IMPORTS BaseSrvKernel32Imports[10] =
{
    { "OpenDataFile", (PVOID*) &pOpenDataFile },
    { "GetDefaultSortkeySize", (PVOID*) &pGetDefaultSortkeySize },
    { "GetLinguistLangSize", (PVOID*) &pGetLinguistLangSize },
    { "NlsConvertIntegerToString", (PVOID*) &pNlsConvertIntegerToString },
    { "ValidateLCType", (PVOID*) &pValidateLCType },
    { "ValidateLocale", (PVOID*) &pValidateLocale },
    { "GetNlsSectionName", (PVOID*) &pGetNlsSectionName },
    { "GetUserDefaultLangID", (PVOID*) &pGetUserDefaultLangID },
    { "GetCPFileNameFromRegistry", (PVOID*) &pGetCPFileNameFromRegistry },
    { "CreateNlsSecurityDescriptor", (PVOID*) &pCreateNlsSecurityDescriptor },
};

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
BaseSrvDelayLoadKernel32(VOID)
{
    NTSTATUS Status;
    ULONG i;
    ANSI_STRING ProcedureName;

    /* Only do this once */
    if (BaseSrvKernel32DelayLoadComplete) return STATUS_SUCCESS;

    /* Loop all imports */
    for (i = 0; i < RTL_NUMBER_OF(BaseSrvKernel32Imports); i++)
    {
        /* Only look them up once */
        if (!*BaseSrvKernel32Imports[i].FunctionPointer)
        {
            /* If we haven't loaded the DLL yet, do it now */
            if (!BaseSrvKernel32DllHandle)
            {
                Status = LdrLoadDll(0,
                                    0,
                                    &BaseSrvKernel32DllPath,
                                    &BaseSrvKernel32DllHandle);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Failed to load %wZ\n", &BaseSrvKernel32DllPath);
                    return Status;
                }
            }

            /* Get the address of the routine being looked up*/
            RtlInitAnsiString(&ProcedureName, BaseSrvKernel32Imports[i].FunctionName);
            Status = LdrGetProcedureAddress(BaseSrvKernel32DllHandle,
                                            &ProcedureName,
                                            0,
                                            BaseSrvKernel32Imports[i].FunctionPointer);
            DPRINT1("NLS: Found %Z at 0x%p\n",
                    &ProcedureName,
                    BaseSrvKernel32Imports[i].FunctionPointer);
            if (!NT_SUCCESS(Status)) break;
        }
    }

    /* Did we find them all? */
    if (i == RTL_NUMBER_OF(BaseSrvKernel32Imports))
    {
        /* Excellent */
        BaseSrvKernel32DelayLoadComplete = TRUE;
        return STATUS_SUCCESS;
    }

    /* Nope, fail */
    return Status;
}

VOID
NTAPI
BaseSrvNLSInit(IN PBASE_STATIC_SERVER_DATA StaticData)
{
    /* Initialize the lock */
    RtlInitializeCriticalSection(&NlsCacheCriticalSection);

    /* Initialize the data with all F's */
    pNlsRegUserInfo = &StaticData->NlsUserInfo;
    RtlFillMemory(&StaticData->NlsUserInfo, sizeof(StaticData->NlsUserInfo), 0xFF);

    /* Set empty LCID */
    pNlsRegUserInfo->UserLocaleId = 0;

    /* Reset the cache update counter */
    RtlEnterCriticalSection(&NlsCacheCriticalSection);
    pNlsRegUserInfo->ulCacheUpdateCount = 0;
    RtlLeaveCriticalSection(&NlsCacheCriticalSection);

    /* Get the LCID */
    NtQueryDefaultLocale(0, &pNlsRegUserInfo->UserLocaleId);
}

NTSTATUS
NTAPI
BaseSrvNlsConnect(IN PCSR_PROCESS CsrProcess,
                  IN OUT PVOID  ConnectionInfo,
                  IN OUT PULONG ConnectionInfoLength)
{
    /* Does nothing */
    return STATUS_SUCCESS;
}

/* PUBLIC SERVER APIS *********************************************************/

CSR_API(BaseSrvNlsSetUserInfo)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvNlsSetMultipleUserInfo)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvNlsCreateSection)
{
    NTSTATUS Status;
    HANDLE SectionHandle, ProcessHandle, FileHandle;
    ULONG LocaleId;
    UNICODE_STRING NlsSectionName;
    PWCHAR NlsFileName;
    PSECURITY_DESCRIPTOR NlsSd;
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR FileNameBuffer[32];
    WCHAR NlsSectionNameBuffer[32];
    PBASE_NLS_CREATE_SECTION NlsMsg = &((PBASE_API_MESSAGE)ApiMessage)->Data.NlsCreateSection;

    /* Load kernel32 first and import the NLS routines */
    Status = BaseSrvDelayLoadKernel32();
    if (!NT_SUCCESS(Status)) return Status;

    /* Assume failure */
    NlsMsg->SectionHandle = NULL;

    /* Check and validate the locale ID, if one is present */
    LocaleId = NlsMsg->LocaleId;
    DPRINT1("NLS: Create Section with LCID: %lx for Type: %d\n", LocaleId, NlsMsg->Type);
    if (LocaleId)
    {
        if (!pValidateLocale(LocaleId)) return STATUS_INVALID_PARAMETER;
    }

    /* Check which NLS section is being created */
    switch (NlsMsg->Type)
    {
        /* For each one, set the correct filename and object name */
        case 1:
            RtlInitUnicodeString(&NlsSectionName, L"\\NLS\\NlsSectionUnicode");
            NlsFileName = L"unicode.nls";
            break;
        case 2:
            RtlInitUnicodeString(&NlsSectionName, L"\\NLS\\NlsSectionLocale");
            NlsFileName =  L"locale.nls";
            break;
        case 3:
            RtlInitUnicodeString(&NlsSectionName, L"\\NLS\\NlsSectionCType");
            NlsFileName = L"ctype.nls";
            break;
        case 4:
            RtlInitUnicodeString(&NlsSectionName, L"\\NLS\\NlsSectionSortkey");
            NlsFileName = L"sortkey.nls";
            break;
        case 5:
            RtlInitUnicodeString(&NlsSectionName, L"\\NLS\\NlsSectionSortTbls");
            NlsFileName = L"sorttbls.nls";
            break;
        case 6:
            RtlInitUnicodeString(&NlsSectionName, L"\\NLS\\NlsSectionCP437");
            NlsFileName = L"c_437.nls";
            break;
        case 7:
            RtlInitUnicodeString(&NlsSectionName, L"\\NLS\\NlsSectionCP1252");
            NlsFileName = L"c_1252.nls";
            break;
        case 8:
            RtlInitUnicodeString(&NlsSectionName, L"\\NLS\\NlsSectionLANG_EXCEPT");
            NlsFileName = L"l_except.nls";
            break;
        case 9:
            DPRINT1("This type not yet supported\n");
            return STATUS_NOT_IMPLEMENTED;
        case 10:
            DPRINT1("This type not yet supported\n");
            return STATUS_NOT_IMPLEMENTED;
        case 11:
            /* Get the filename for this locale */
            if (!pGetCPFileNameFromRegistry(NlsMsg->LocaleId,
                                            FileNameBuffer,
                                            RTL_NUMBER_OF(FileNameBuffer)))
            {
                DPRINT1("File name query failed\n");
                return STATUS_INVALID_PARAMETER;
            }

            /* Get the name of the section for this locale */
            DPRINT1("File name: %S\n", FileNameBuffer);
            Status = pGetNlsSectionName(NlsMsg->LocaleId,
                                        10,
                                        0,
                                        L"\\NLS\\NlsSectionCP",
                                        NlsSectionNameBuffer,
                                        RTL_NUMBER_OF(NlsSectionNameBuffer));
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Section name query failed: %lx\n", Status);
                return Status;
            }

            /* Setup the name and go open it */
            NlsFileName = FileNameBuffer;
            DPRINT1("Section name: %S\n", NlsSectionNameBuffer);
            RtlInitUnicodeString(&NlsSectionName, NlsSectionNameBuffer);
            break;
        case 12:
            RtlInitUnicodeString(&NlsSectionName, L"\\NLS\\NlsSectionGeo");
            NlsFileName = L"geo.nls";
            break;
        default:
            DPRINT1("NLS: Invalid NLS type!\n");
            return STATUS_INVALID_PARAMETER;
    }

    /* Open the specified NLS file */
    Status = pOpenDataFile(&FileHandle, NlsFileName);
    if (Status != STATUS_SUCCESS)
    {
        DPRINT1("NLS: Failed to open file: %lx\n", Status);
        return Status;
    }

    /* Create an SD for the section object */
    Status = pCreateNlsSecurityDescriptor(&NlsSd,
                                          sizeof(SECURITY_DESCRIPTOR),
                                          SECTION_MAP_READ);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NLS: CreateNlsSecurityDescriptor FAILED!: %lx\n", Status);
        NtClose(FileHandle);
        return Status;
    }

    /* Create the section object proper */
    InitializeObjectAttributes(&ObjectAttributes,
                               &NlsSectionName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT | OBJ_OPENIF,
                               NULL,
                               NlsSd);
    Status = NtCreateSection(&SectionHandle,
                             SECTION_MAP_READ,
                             &ObjectAttributes,
                             0,
                             PAGE_READONLY,
                             SEC_COMMIT,
                             FileHandle);
    NtClose(FileHandle);
    RtlFreeHeap(RtlGetProcessHeap(), 0, NlsSd);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NLS: Failed to create section! %lx\n", Status);
        return Status;
    }

    /* Open a handle to the calling process */
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    Status = NtOpenProcess(&ProcessHandle,
                           PROCESS_DUP_HANDLE,
                           &ObjectAttributes,
                           &ApiMessage->Header.ClientId);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NLS: Failed to open process! %lx\n", Status);
        NtClose(SectionHandle);
        return Status;
    }

    /* Duplicate the handle to the section object into it */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               SectionHandle,
                               ProcessHandle,
                               &NlsMsg->SectionHandle,
                               0,
                               0,
                               3);
    NtClose(ProcessHandle);
    return Status;
}

CSR_API(BaseSrvNlsUpdateCacheCount)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvNlsGetUserInfo)
{
    NTSTATUS Status;
    PBASE_NLS_GET_USER_INFO NlsMsg = &((PBASE_API_MESSAGE)ApiMessage)->Data.NlsGetUserInfo;

    /* Make sure the buffer is valid and of the right size */
    if ((CsrValidateMessageBuffer(ApiMessage, &NlsMsg->NlsUserInfo, NlsMsg->Size, sizeof(BYTE))) &&
        (NlsMsg->Size == sizeof(NLS_USER_INFO)))
    {
        /* Acquire the lock to prevent updates while we copy */
        Status = RtlEnterCriticalSection(&NlsCacheCriticalSection);
        if (NT_SUCCESS(Status))
        {
            /* Do the copy now, then drop the lock */
            RtlCopyMemory(NlsMsg->NlsUserInfo, pNlsRegUserInfo, NlsMsg->Size);
            DPRINT1("NLS Data copy complete\n");
            RtlLeaveCriticalSection(&NlsCacheCriticalSection);
        }
    }
    else
    {
        /* The data was invalid, bail out */
        DPRINT1("NLS: Size of info is invalid: %lx vs %lx\n", NlsMsg->Size, sizeof(NLS_USER_INFO));
        Status = STATUS_INVALID_PARAMETER;
    }

    /* All done */
    return Status;
}

/* PUBLIC APIS ****************************************************************/

NTSTATUS
NTAPI
BaseSrvNlsLogon(DWORD Unknown)
{
    DPRINT1("%s(%lu) not yet implemented\n", __FUNCTION__, Unknown);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
BaseSrvNlsUpdateRegistryCache(DWORD Unknown1,
                              DWORD Unknown2)
{
    DPRINT1("%s(%lu, %lu) not yet implemented\n", __FUNCTION__, Unknown1, Unknown2);
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
