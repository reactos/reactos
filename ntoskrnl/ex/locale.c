/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ex/locale.c
 * PURPOSE:         Locale (Language) Support for the Executive
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 *                  Thomas Weidenmueller (w3seek@reactos.org
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* System IDs: EN_US */
LCID PsDefaultSystemLocaleId = 0x00000409;
LANGID PsInstallUILanguageId = LANGIDFROMLCID(0x00000409);

/* UI/Thread IDs: Same as system */
LCID PsDefaultThreadLocaleId = 0x00000409;
LANGID PsDefaultUILanguageId = LANGIDFROMLCID(0x00000409);

/* DEFINES *******************************************************************/

#define BOGUS_LOCALE_ID 0xFFFF0000

/* PRIVATE FUNCTIONS *********************************************************/

/**
 * @brief
 * Validates the registry data of a NLS locale.
 *
 * @param[in] LocaleData
 * A pointer to partial information that contains
 * the NLS locale data.
 *
 * @return
 * Returns TRUE if the following conditions are met,
 * otherwise FALSE is returned.
 */
static
__inline
BOOLEAN
ExpValidateNlsLocaleData(
    _In_ PKEY_VALUE_PARTIAL_INFORMATION LocaleData)
{
    PWCHAR Data;

    /* Is this a null-terminated string type? */
    if (LocaleData->Type != REG_SZ)
    {
        return FALSE;
    }

    /* Does it have a consistent length? */
    if (LocaleData->DataLength < sizeof(WCHAR))
    {
        return FALSE;
    }

    /* Is the locale set and null-terminated? */
    Data = (PWSTR)LocaleData->Data;
    if (Data[0] != L'1' || Data[1] != UNICODE_NULL)
    {
        return FALSE;
    }

    /* All of the conditions above are met */
    return TRUE;
}

/**
 * @brief
 * Validates a NLS locale. Whether a locale is valid
 * or not depends on the following conditions:
 *
 * - The locale must exist in the Locale key, otherwise
 *   in the Alternate Sorts key;
 *
 * - The locale must exist in the Language Groups key, and
 *   the queried value must be readable;
 *
 * - The locale registry data must be of REG_SIZE type,
 *   has a consistent length and the locale belongs to
 *   a supported language group that is set.
 *
 * @param[in] LocaleId
 * A locale identifier that corresponds to a specific
 * locale to be validated.
 *
 * @return
 * Returns STATUS_SUCCESS if the function has successfully
 * validated the locale and it is valid. STATUS_OBJECT_NAME_NOT_FOUND
 * is returned if the following locale does not exist on the system.
 * A failure NTSTATUS code is returned otherwise.
 */
static
NTSTATUS
ExpValidateNlsLocaleId(
    _In_ LCID LocaleId)
{
    NTSTATUS Status;
    HANDLE NlsLocaleKey = NULL, AltSortKey = NULL, LangGroupKey = NULL;
    OBJECT_ATTRIBUTES NlsLocalKeyAttrs, AltSortKeyAttrs, LangGroupKeyAttrs;
    PKEY_VALUE_PARTIAL_INFORMATION BufferKey;
    WCHAR ValueBuffer[20], LocaleIdBuffer[20];
    ULONG ReturnedLength;
    UNICODE_STRING LocaleIdString;
    static UNICODE_STRING NlsLocaleKeyPath = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\Nls\\Locale");
    static UNICODE_STRING AltSortKeyPath = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\Nls\\Locale\\Alternate Sorts");
    static UNICODE_STRING LangGroupPath = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\Nls\\Language Groups");

    /* Initialize the registry path attributes */
    InitializeObjectAttributes(&NlsLocalKeyAttrs,
                               &NlsLocaleKeyPath,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    InitializeObjectAttributes(&AltSortKeyAttrs,
                               &AltSortKeyPath,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    InitializeObjectAttributes(&LangGroupKeyAttrs,
                               &LangGroupPath,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Copy the locale ID into a buffer */
    _swprintf(LocaleIdBuffer, L"%08lx", (ULONG)LocaleId);

    /* And build the LCID string */
    RtlInitUnicodeString(&LocaleIdString, LocaleIdBuffer);

    /* Open the NLS locale key */
    Status = ZwOpenKey(&NlsLocaleKey,
                       KEY_QUERY_VALUE,
                       &NlsLocalKeyAttrs);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open %wZ (Status 0x%lx)\n", NlsLocaleKeyPath, Status);
        return Status;
    }

    /* Open the NLS alternate sort locales key */
    Status = ZwOpenKey(&AltSortKey,
                       KEY_QUERY_VALUE,
                       &AltSortKeyAttrs);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open %wZ (Status 0x%lx)\n", AltSortKeyPath, Status);
        goto Quit;
    }

    /* Open the NLS language groups key */
    Status = ZwOpenKey(&LangGroupKey,
                       KEY_QUERY_VALUE,
                       &LangGroupKeyAttrs);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open %wZ (Status 0x%lx)\n", LangGroupPath, Status);
        goto Quit;
    }

    /* Check if the captured locale ID exists in the list of other locales */
    BufferKey = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
    Status = ZwQueryValueKey(NlsLocaleKey,
                             &LocaleIdString,
                             KeyValuePartialInformation,
                             BufferKey,
                             sizeof(ValueBuffer),
                             &ReturnedLength);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, retry by looking at the alternate sorts locales */
        Status = ZwQueryValueKey(AltSortKey,
                                 &LocaleIdString,
                                 KeyValuePartialInformation,
                                 BufferKey,
                                 sizeof(ValueBuffer),
                                 &ReturnedLength);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to query value from Alternate Sorts key (Status 0x%lx)\n", Status);
            goto Quit;
        }
    }

    /* Ensure the queried locale is of the right key type with a sane length */
    if (BufferKey->Type != REG_SZ ||
        BufferKey->DataLength < sizeof(WCHAR))
    {
        DPRINT1("The queried locale is of bad value type or length (Type %lu, DataLength %lu)\n",
                BufferKey->Type, BufferKey->DataLength);
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Quit;
    }

    /* We got what we need, now query the locale from the language groups */
    RtlInitUnicodeString(&LocaleIdString, (PWSTR)BufferKey->Data);
    Status = ZwQueryValueKey(LangGroupKey,
                             &LocaleIdString,
                             KeyValuePartialInformation,
                             BufferKey,
                             sizeof(ValueBuffer),
                             &ReturnedLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to query value from Language Groups key (Status 0x%lx)\n", Status);
        goto Quit;
    }

    /*
     * We have queried the locale with its data. However we are not finished here yet,
     * because the locale data could be malformed or the locale itself was not set
     * so ensure all of these conditions are met.
     */
    if (!ExpValidateNlsLocaleData(BufferKey))
    {
        DPRINT1("The locale data is not valid!\n");
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }

Quit:
    if (LangGroupKey != NULL)
    {
        ZwClose(LangGroupKey);
    }

    if (AltSortKey != NULL)
    {
        ZwClose(AltSortKey);
    }

    if (NlsLocaleKey != NULL)
    {
        ZwClose(NlsLocaleKey);
    }

    return Status;
}

NTSTATUS
NTAPI
ExpGetCurrentUserUILanguage(IN PCWSTR MuiName,
                            OUT LANGID* LanguageId)
{
    UCHAR ValueBuffer[256];
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName =
        RTL_CONSTANT_STRING(L"Control Panel\\Desktop");
    UNICODE_STRING ValueName;
    UNICODE_STRING ValueString;
    ULONG ValueLength;
    ULONG Value;
    HANDLE UserKey;
    HANDLE KeyHandle;
    NTSTATUS Status;
    PAGED_CODE();

    /* Setup the key name */
    RtlInitUnicodeString(&ValueName, MuiName);

    /* Open the use key */
    Status = RtlOpenCurrentUser(KEY_READ, &UserKey);
    if (!NT_SUCCESS(Status)) return Status;

    /* Initialize the attributes and open the key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               UserKey,
                               NULL);
    Status = ZwOpenKey(&KeyHandle, KEY_QUERY_VALUE,&ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* Set buffer and query the current value */
        ValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValueName,
                                 KeyValuePartialInformation,
                                 ValueBuffer,
                                 sizeof(ValueBuffer),
                                 &ValueLength);
        if (NT_SUCCESS(Status))
        {
            /* Success, is the value the right type? */
            if (ValueInfo->Type == REG_SZ)
            {
                /* It is. Initialize the data and convert it */
                RtlInitUnicodeString(&ValueString, (PWSTR)ValueInfo->Data);
                Status = RtlUnicodeStringToInteger(&ValueString, 16, &Value);
                if (NT_SUCCESS(Status))
                {
                    /* Return the language */
                    *LanguageId = (USHORT)Value;
                }
            }
            else
            {
                /* Fail */
                Status = STATUS_UNSUCCESSFUL;
            }
        }

        /* Close the key */
        ZwClose(KeyHandle);
    }

    /* Close the user key and return */
    ZwClose(UserKey);
    return Status;
}

NTSTATUS
NTAPI
ExpSetCurrentUserUILanguage(IN PCWSTR MuiName,
                            IN LANGID LanguageId)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"Control Panel\\Desktop");
    UNICODE_STRING ValueName;
    WCHAR ValueBuffer[8];
    ULONG ValueLength;
    HANDLE UserHandle;
    HANDLE KeyHandle;
    NTSTATUS Status;
    PAGED_CODE();

    /* Check that the passed language ID is not bogus */
    if (LanguageId & BOGUS_LOCALE_ID)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Setup the key name */
    RtlInitUnicodeString(&ValueName, MuiName);

    /* Open the use key */
    Status = RtlOpenCurrentUser(KEY_WRITE, &UserHandle);
    if (!NT_SUCCESS(Status)) return Status;

    /* Initialize the attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               UserHandle,
                               NULL);

    /* Validate the language ID */
    Status = ExpValidateNlsLocaleId(MAKELCID(LanguageId, SORT_DEFAULT));
    if (NT_SUCCESS(Status))
    {
        /* Open the key */
        Status = ZwOpenKey(&KeyHandle, KEY_SET_VALUE, &ObjectAttributes);
        if (NT_SUCCESS(Status))
        {
            /* Setup the value name */
            ValueLength = _swprintf(ValueBuffer, L"%04lX", (ULONG)LanguageId);

            /* Set the length for the call and set the value */
            ValueLength = (ValueLength + 1) * sizeof(WCHAR);
            Status = ZwSetValueKey(KeyHandle,
                                   &ValueName,
                                   0,
                                   REG_SZ,
                                   ValueBuffer,
                                   ValueLength);

            /* Close the handle for this key */
            ZwClose(KeyHandle);
        }
    }

    /* Close the user key and return status */
    ZwClose(UserHandle);
    return Status;
}

/* PUBLIC FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
NtQueryDefaultLocale(IN BOOLEAN UserProfile,
                     OUT PLCID DefaultLocaleId)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Enter SEH for probing */
    _SEH2_TRY
    {
        /* Check if we came from user mode */
        if (KeGetPreviousMode() != KernelMode)
        {
            /* Probe the language ID */
            ProbeForWriteLangId(DefaultLocaleId);
        }

        /* Check if we have a user profile */
        if (UserProfile)
        {
            /* Return session wide thread locale */
            *DefaultLocaleId = MmGetSessionLocaleId();
        }
        else
        {
            /* Return system locale */
            *DefaultLocaleId = PsDefaultSystemLocaleId;
        }
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        /* Get exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtSetDefaultLocale(IN BOOLEAN UserProfile,
                   IN LCID DefaultLocaleId)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    UNICODE_STRING LocaleString;
    HANDLE KeyHandle = NULL;
    ULONG ValueLength;
    WCHAR ValueBuffer[20];
    HANDLE UserKey = NULL;
    NTSTATUS Status;
    UCHAR KeyValueBuffer[256];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    PAGED_CODE();

    /* Check that the passed locale ID is not bogus */
    if (DefaultLocaleId & BOGUS_LOCALE_ID)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if we have a profile */
    if (UserProfile)
    {
        /* Open the user's key */
        Status = RtlOpenCurrentUser(KEY_WRITE, &UserKey);
        if (!NT_SUCCESS(Status)) return Status;

        /* Initialize the registry location */
        RtlInitUnicodeString(&KeyName, L"Control Panel\\International");
        RtlInitUnicodeString(&ValueName, L"Locale");
    }
    else
    {
        /* Initialize the system registry location */
        RtlInitUnicodeString(&KeyName,
                             L"\\Registry\\Machine\\System\\CurrentControlSet"
                             L"\\Control\\Nls\\Language");
        RtlInitUnicodeString(&ValueName, L"Default");
    }

    /* Initialize the object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               UserKey,
                               NULL);

    /* Check if we don't have a default locale yet */
    if (!DefaultLocaleId)
    {
        /* Open the key for reading */
        Status = ZwOpenKey(&KeyHandle, KEY_QUERY_VALUE, &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        /* Query the key value */
        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueBuffer;
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValueName,
                                 KeyValuePartialInformation,
                                 KeyValueInformation,
                                 sizeof(KeyValueBuffer),
                                 &ValueLength);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        /* Check if this is a REG_DWORD */
        if ((KeyValueInformation->Type == REG_DWORD) &&
            (KeyValueInformation->DataLength == sizeof(ULONG)))
        {
            /* It contains the LCID as a DWORD */
            DefaultLocaleId = *((ULONG*)KeyValueInformation->Data);
        }
        /* Otherwise check for a REG_SZ */
        else if (KeyValueInformation->Type == REG_SZ)
        {
            /* Initialize a unicode string from the value data */
            LocaleString.Buffer = (PWCHAR)KeyValueInformation->Data;
            LocaleString.Length = (USHORT)KeyValueInformation->DataLength;
            LocaleString.MaximumLength = LocaleString.Length;

            /* Convert the hex string to a number */
            RtlUnicodeStringToInteger(&LocaleString, 16, &DefaultLocaleId);
        }
        else
        {
            Status = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        /* We have a locale, validate it */
        Status = ExpValidateNlsLocaleId(DefaultLocaleId);
        if (NT_SUCCESS(Status))
        {
            /* Open the key now */
            Status = ZwOpenKey(&KeyHandle, KEY_SET_VALUE, &ObjectAttributes);
            if (NT_SUCCESS(Status))
            {
                /* Check if we had a profile */
                if (UserProfile)
                {
                    /* Fill in the buffer */
                    ValueLength = _swprintf(ValueBuffer,
                                            L"%08lx",
                                            (ULONG)DefaultLocaleId);
                }
                else
                {
                    /* Fill in the buffer */
                    ValueLength = _swprintf(ValueBuffer,
                                            L"%04lx",
                                            (ULONG)DefaultLocaleId & 0xFFFF);
                }

                /* Set the length for the registry call */
                ValueLength = (ValueLength + 1) * sizeof(WCHAR);

                /* Now write the actual value */
                Status = ZwSetValueKey(KeyHandle,
                                       &ValueName,
                                       0,
                                       REG_SZ,
                                       ValueBuffer,
                                       ValueLength);
            }
        }
    }

Cleanup:

    /* Close the locale key */
    if (KeyHandle)
    {
        ObCloseHandle(KeyHandle, KernelMode);
    }

    /* Close the user key */
    if (UserKey)
    {
        ObCloseHandle(UserKey, KernelMode);
    }

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Check if it was for a user */
        if (UserProfile)
        {
            /* Set the session wide thread locale */
            MmSetSessionLocaleId(DefaultLocaleId);
        }
        else
        {
            /* Set system locale */
            PsDefaultSystemLocaleId = DefaultLocaleId;
        }
    }

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQueryInstallUILanguage(OUT LANGID* LanguageId)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Enter SEH for probing */
    _SEH2_TRY
    {
        /* Check if we came from user mode */
        if (KeGetPreviousMode() != KernelMode)
        {
            /* Probe the Language ID */
            ProbeForWriteLangId(LanguageId);
        }

        /* Return it */
        *LanguageId = PsInstallUILanguageId;
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        /* Get exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQueryDefaultUILanguage(OUT LANGID* LanguageId)
{
    NTSTATUS Status;
    LANGID SafeLanguageId;
    PAGED_CODE();

    /* Call the executive helper routine */
    Status = ExpGetCurrentUserUILanguage(L"MultiUILanguageId", &SafeLanguageId);

    /* Enter SEH for probing */
    _SEH2_TRY
    {
        /* Check if we came from user mode */
        if (KeGetPreviousMode() != KernelMode)
        {
            /* Probe the Language ID */
            ProbeForWriteLangId(LanguageId);
        }

        if (NT_SUCCESS(Status))
        {
            /* Success, return the language */
            *LanguageId = SafeLanguageId;
        }
        else
        {
            /* Failed, use fallback value */
            // NOTE: Windows doesn't use PsDefaultUILanguageId.
            *LanguageId = PsInstallUILanguageId;
        }
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        /* Return exception code */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Return success */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtSetDefaultUILanguage(IN LANGID LanguageId)
{
    NTSTATUS Status;
    PAGED_CODE();

    /* Check if the caller specified a language id */
    if (LanguageId)
    {
        /* Set the pending MUI language id */
        Status = ExpSetCurrentUserUILanguage(L"MUILanguagePending", LanguageId);
    }
    else
    {
        /* Otherwise get the pending MUI language id */
        Status = ExpGetCurrentUserUILanguage(L"MUILanguagePending", &LanguageId);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        /* And apply it as actual */
        Status = ExpSetCurrentUserUILanguage(L"MultiUILanguageId", LanguageId);
    }

    return Status;
}

static
NTSTATUS
ExpGetNlsSectionName(
    _In_ NLS_SELECTION_TYPE Type,
    _In_ UINT Id,
    _Out_ PUNICODE_STRING SectionName)
{
    NTSTATUS Status = STATUS_SUCCESS;

    switch (Type)
    {
    case NLS_SECTION_SORTKEYS:
        if (Id) return STATUS_INVALID_PARAMETER_1;
        RtlInitUnicodeString(SectionName, L"\\NLS\\NlsSectionSORTDEFAULT");
        break;
    case NLS_SECTION_CASEMAP:
        if (Id) return STATUS_UNSUCCESSFUL;
        RtlInitUnicodeString(SectionName, L"\\NLS\\NlsSectionLANG_INTL");
        break;
    case NLS_SECTION_CODEPAGE:
        Status = RtlUnicodeStringPrintf(SectionName, L"\\NLS\\NlsSectionCP%03u", Id);
        break;
    case NLS_SECTION_NORMALIZE:
        Status = RtlUnicodeStringPrintf(SectionName, L"\\NLS\\NlsSectionNORM%08x", Id);
        break;
    default:
        return STATUS_INVALID_PARAMETER_1;
    }
    return Status;
}

static
NTSTATUS
ExpGetNlsFilePath(
    _In_ UINT Type,
    _In_ UINT Id,
    _Out_ PUNICODE_STRING FileName)
{
    NTSTATUS Status = STATUS_SUCCESS;

    switch (Type)
    {
    case NLS_SECTION_SORTKEYS:
        RtlInitUnicodeString(FileName, L"\\SystemRoot\\Globalization\\Sorting\\sortdefault.nls");
        break;
    case NLS_SECTION_CASEMAP:
        RtlInitUnicodeString(FileName, L"\\SystemRoot\\System32\\l_intl.nls");
        break;
    case NLS_SECTION_CODEPAGE:
        Status = RtlUnicodeStringPrintf(FileName, L"\\SystemRoot\\System32\\c_%03u.nls", Id);
        break;
    case NLS_SECTION_NORMALIZE:
        switch (Id)
        {
        case NormalizationC:
            RtlInitUnicodeString(FileName, L"\\SystemRoot\\System32\\normnfc.nls");
            break;
        case NormalizationD:
            RtlInitUnicodeString(FileName, L"\\SystemRoot\\System32\\normnfd.nls");
            break;
        case NormalizationKC:
            RtlInitUnicodeString(FileName, L"\\SystemRoot\\System32\\normnfkc.nls");
            break;
        case NormalizationKD:
            RtlInitUnicodeString(FileName, L"\\SystemRoot\\System32\\normnfkd.nls");
            break;
        case 13:
            RtlInitUnicodeString(FileName, L"\\SystemRoot\\System32\\normidna.nls");
            break;
        default:
            return STATUS_INVALID_PARAMETER_1;
        }
        break;
    default:
        return STATUS_INVALID_PARAMETER_1;
    }
    return Status;
}

NTSTATUS
NTAPI
NtGetNlsSectionPtr(
    _In_ ULONG SectionType,
    _In_ ULONG SectionId,
    _In_ PVOID ContextData,
    _Out_ PVOID* SectionPointer,
    _Out_ SIZE_T* SectionSize)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES SectionAttributes;
    OBJECT_ATTRIBUTES FileAttributes;
    IO_STATUS_BLOCK Iosb;
    HANDLE FileHandle = NULL;
    HANDLE SectionHandle = NULL;
    PVOID BaseAddress = NULL;
    SIZE_T ViewSize = 0;
    DECLARE_UNICODE_STRING_SIZE(Name, 256);
    DECLARE_UNICODE_STRING_SIZE(FileName, 256);

    Status = ExpGetNlsSectionName(SectionType, SectionId, &Name);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ExpGetNlsFilePath(SectionType, SectionId, &FileName);
    if (!NT_SUCCESS(Status)) return Status;

    InitializeObjectAttributes(&SectionAttributes, &Name, OBJ_OPENIF | OBJ_KERNEL_HANDLE | OBJ_PERMANENT, NULL, NULL);

    /* Try to open the NLS section first, if that fails open the NLS file and create the section */
    if (!NT_SUCCESS(ZwOpenSection(&SectionHandle, SECTION_MAP_READ, &SectionAttributes)))
    {
        InitializeObjectAttributes(&FileAttributes, &FileName, OBJ_KERNEL_HANDLE, NULL, NULL);

        Status = ZwOpenFile(
            &FileHandle,
            GENERIC_READ,
            &FileAttributes,
            &Iosb,
            FILE_SHARE_READ,
            FILE_NON_DIRECTORY_FILE
        );

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ERROR: Failed to load NLS File: %ws\n", FileName.Buffer);
            return Status;
        }

        Status = ZwCreateSection(&SectionHandle, SECTION_MAP_READ, &SectionAttributes, NULL, PAGE_READONLY, SEC_COMMIT, FileHandle);
        ZwClose(FileHandle);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ERROR: Failed to create section %ws\n", Name.Buffer);
            return Status;
        }
    }

    /* Map the NLS file */
    Status = ZwMapViewOfSection(
        SectionHandle,
        ZwCurrentProcess(),
        &BaseAddress,
        0,
        0,
        NULL,
        &ViewSize,
        ViewUnmap,
        0,
        PAGE_READONLY
    );

    ZwClose(SectionHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to map NLS file %ws\n", FileName.Buffer);
        return Status;
    }

    _SEH2_TRY
    {
        /* Check if we came from user mode */
        if (KeGetPreviousMode() != KernelMode)
        {
            ProbeForWrite(SectionPointer, sizeof(*SectionPointer), sizeof(PVOID));
            ProbeForWrite(SectionSize, sizeof(*SectionSize), sizeof(SIZE_T));
        }

        *SectionPointer = BaseAddress;
        *SectionSize = ViewSize;
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        /* Return exception code */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    
    return Status;
}

NTSTATUS
NTAPI
NtInitializeNlsFiles(
    _Out_ PVOID* BaseAddress,
    _Out_ PLCID DefaultLocaleId,
    _Out_ PLARGE_INTEGER DefaultCasingTableSize,
    _Out_opt_ PULONG CurrentNLSVersion)
{
    NTSTATUS Status;
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES Attributes;
    IO_STATUS_BLOCK Iosb;
    HANDLE FileHandle = NULL;
    HANDLE SectionHandle = NULL;
    SIZE_T ViewSize = 0;
    PVOID SectionBaseAddress = NULL;

    RtlInitUnicodeString(&FileName, L"\\SystemRoot\\System32\\locale.nls");
    InitializeObjectAttributes(&Attributes, &FileName, OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = ZwOpenFile(
        &FileHandle,
        GENERIC_READ,
        &Attributes,
        &Iosb,
        FILE_SHARE_READ,
        FILE_NON_DIRECTORY_FILE
    );

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to load NLS File: %ws\n", FileName.Buffer);
        return Status;
    }

    Status = ZwCreateSection(&SectionHandle, SECTION_MAP_READ, NULL, NULL, PAGE_READONLY, SEC_COMMIT, FileHandle);
    ZwClose(FileHandle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to create section for NLS file %ws\n", FileName.Buffer);
        return Status;
    }

    Status = ZwMapViewOfSection(
        SectionHandle,
        ZwCurrentProcess(),
        &SectionBaseAddress,
        0,
        0,
        NULL,
        &ViewSize,
        ViewUnmap,
        0,
        PAGE_READONLY
    );

    ZwClose(SectionHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to map NLS file %ws\n", FileName.Buffer);
        return Status;
    }

     _SEH2_TRY
    {
        /* Check if we came from user mode */
        if (KeGetPreviousMode() != KernelMode)
        {
            ProbeForWriteLangId(DefaultLocaleId);
            ProbeForWrite(BaseAddress, sizeof(*BaseAddress), sizeof(PVOID));
            ProbeForWrite(DefaultCasingTableSize, sizeof(*DefaultCasingTableSize), sizeof(LARGE_INTEGER));
        }

        *DefaultLocaleId = PsDefaultSystemLocaleId;
        *BaseAddress = SectionBaseAddress;
        DefaultCasingTableSize->QuadPart = (LONGLONG)ViewSize;
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        /* Return exception code */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;
    
    return Status;
}

/* EOF */
