/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sysinfo.c

Abstract:

    This module implements the NT set and query system information services.

Author:

    Steve Wood (stevewo) 21-Aug-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "exp.h"
#pragma hdrstop

#include "stdlib.h"
#include "string.h"
#include "vdmntos.h"
#include <nturtl.h>
#include "pool.h"
#include "stktrace.h"
#include "align.h"
#include "..\..\..\inc\alpha.h"

extern PVOID PspCidTable;       // BUGBUG - Copied from ps\psp.h

extern ULONG MmAvailablePages;
extern ULONG MmTotalCommittedPages;
extern ULONG MmTotalCommitLimit;
extern ULONG MmPeakCommitment;
extern ULONG MmLowestPhysicalPage;
extern ULONG MmHighestPhysicalPage;
extern ULONG MmTotalFreeSystemPtes[1];
extern ULONG MmSystemCodePage;
extern ULONG MmSystemCachePage;
extern ULONG MmPagedPoolPage;
extern ULONG MmSystemDriverPage;
extern ULONG MmTotalSystemCodePages;
extern ULONG MmTotalSystemDriverPages;
extern RTL_TIME_ZONE_INFORMATION ExpTimeZoneInformation;

//
// For SystemDpcBehaviorInformation
//
extern ULONG KiMaximumDpcQueueDepth;
extern ULONG KiMinimumDpcRate;
extern ULONG KiAdjustDpcThreshold;
extern ULONG KiIdealDpcRate;

extern LIST_ENTRY MmLoadedUserImageList;

extern MMSUPPORT MmSystemCacheWs;
extern ULONG MmTransitionSharedPages;
extern ULONG MmTransitionSharedPagesPeak;

#define ROUND_UP(VALUE,ROUND) ((ULONG)(((ULONG)VALUE + \
                               ((ULONG)ROUND - 1L)) & (~((ULONG)ROUND - 1L))))

//
// For referencing a user-supplied event handle
//
extern POBJECT_TYPE ExEventObjectType;


NTSTATUS
ExpValidateLocale(
    IN LCID LocaleId
    );

NTSTATUS
ExpGetCurrentUserUILanguage(
    IN WCHAR *ValueName,
    OUT LANGID *CurrentUserUILanguageId
    );

NTSTATUS
ExpSetCurrentUserUILanguage(
    IN WCHAR *ValueName,
    IN LANGID DefaultUILanguageId
    );

NTSTATUS
ExpGetUILanguagePolicy(
    IN HANDLE CurrentUserKey,
    OUT LANGID *PolicyUILanguageId
    );

NTSTATUS
ExpGetProcessInformation (
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length,
    IN PULONG SessionId OPTIONAL
    );

VOID
ExpCopyProcessInfo (
    IN PSYSTEM_PROCESS_INFORMATION ProcessInfo,
    IN PEPROCESS Process
    );

VOID
ExpCopyThreadInfo (
    IN PSYSTEM_THREAD_INFORMATION ThreadInfo,
    IN PETHREAD Thread
    );

#if i386 && !FPO
NTSTATUS
ExpGetStackTraceInformation (
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    );
#endif // i386 && !FPO

NTSTATUS
ExpGetLockInformation (
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    );

NTSTATUS
ExpGetLookasideInformation (
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG Length
    );

NTSTATUS
ExpGetPoolInformation(
    IN POOL_TYPE PoolType,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    );

NTSTATUS
ExpGetHandleInformation(
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    );

NTSTATUS
ExpGetObjectInformation(
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    );


NTSTATUS
ExpGetInstemulInformation(
    OUT PSYSTEM_VDM_INSTEMUL_INFO Info
    );

NTSTATUS
ExpGetPoolTagInfo (
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    IN OUT PULONG ReturnLength OPTIONAL
    );

NTSTATUS
ExpQueryModuleInformation(
    IN PLIST_ENTRY LoadOrderListHead,
    IN PLIST_ENTRY UserModeLoadOrderListHead,
    OUT PRTL_PROCESS_MODULES ModuleInformation,
    IN ULONG ModuleInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

NTSTATUS
ExpQueryLegacyDriverInformation(
    IN PSYSTEM_LEGACY_DRIVER_INFORMATION LegacyInfo,
    IN PULONG Length
    );

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, NtQueryDefaultLocale)
#pragma alloc_text(PAGE, NtSetDefaultLocale)
#pragma alloc_text(PAGE, NtQueryInstallUILanguage)
#pragma alloc_text(PAGE, NtQueryDefaultUILanguage)
#pragma alloc_text(PAGE, ExpGetCurrentUserUILanguage)
#pragma alloc_text(PAGE, NtSetDefaultUILanguage)
#pragma alloc_text(PAGE, ExpSetCurrentUserUILanguage)
#pragma alloc_text(PAGE, ExpValidateLocale)
#pragma alloc_text(PAGE, ExpGetUILanguagePolicy)
#pragma alloc_text(PAGE, NtQuerySystemInformation)
#pragma alloc_text(PAGE, NtSetSystemInformation)
#pragma alloc_text(PAGE, ExpGetHandleInformation)
#pragma alloc_text(PAGE, ExpGetObjectInformation)
#pragma alloc_text(PAGE, ExpQueryModuleInformation)
#pragma alloc_text(PAGE, ExpCopyProcessInfo)
#pragma alloc_text(PAGE, ExpQueryLegacyDriverInformation)
#pragma alloc_text(PAGELK, ExpGetProcessInformation)
#pragma alloc_text(PAGELK, ExpCopyThreadInfo)
#pragma alloc_text(PAGELK, ExpGetLockInformation)
#pragma alloc_text(PAGELK, ExpGetLookasideInformation)
#pragma alloc_text(PAGELK, ExpGetPoolInformation)
#endif



NTSTATUS
NtQueryDefaultLocale(
    IN BOOLEAN UserProfile,
    OUT PLCID DefaultLocaleId
    )
{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PAGED_CODE();

    Status = STATUS_SUCCESS;
    try {

        //
        // Get previous processor mode and probe output argument if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWriteUlong( (PULONG)DefaultLocaleId );
            }

        if (UserProfile) {
            *DefaultLocaleId = PsDefaultThreadLocaleId;
            }
        else {
            *DefaultLocaleId = PsDefaultSystemLocaleId;
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }

    return Status;
}


NTSTATUS
NtSetDefaultLocale(
    IN BOOLEAN UserProfile,
    IN LCID DefaultLocaleId
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyPath, KeyValueName;
    HANDLE CurrentUserKey, Key;
    WCHAR KeyValueBuffer[ 128 ];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    ULONG ResultLength;
    PWSTR s;
    ULONG n, i, Digit;
    WCHAR c;

    PAGED_CODE();

    if (DefaultLocaleId & 0xFFFF0000) {
        return STATUS_INVALID_PARAMETER;
        }

    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueBuffer;
    if (UserProfile) {
        Status = RtlOpenCurrentUser( MAXIMUM_ALLOWED, &CurrentUserKey );
        if (!NT_SUCCESS( Status )) {
            return Status;
            }

        RtlInitUnicodeString( &KeyValueName, L"Locale" );
        RtlInitUnicodeString( &KeyPath, L"Control Panel\\International" );
        }
    else {
        RtlInitUnicodeString( &KeyValueName, L"Default" );
        RtlInitUnicodeString( &KeyPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Nls\\Language" );
        CurrentUserKey = NULL;
        }

    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyPath,
                                (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                CurrentUserKey,
                                NULL
                              );
    if (DefaultLocaleId == 0) {
        Status = ZwOpenKey( &Key,
                            GENERIC_READ,
                            &ObjectAttributes
                          );
        if (NT_SUCCESS( Status )) {
            Status = ZwQueryValueKey( Key,
                                      &KeyValueName,
                                      KeyValuePartialInformation,
                                      KeyValueInformation,
                                      sizeof( KeyValueBuffer ),
                                      &ResultLength
                                    );
            if (NT_SUCCESS( Status )) {
                if (KeyValueInformation->Type == REG_SZ) {
                    s = (PWSTR)KeyValueInformation->Data;
                    for (i=0; i<KeyValueInformation->DataLength; i += sizeof( WCHAR )) {
                        c = *s++;
                        if (c >= L'0' && c <= L'9') {
                            Digit = c - L'0';
                            }
                        else
                        if (c >= L'A' && c <= L'F') {
                            Digit = c - L'A' + 10;
                            }
                        else
                        if (c >= L'a' && c <= L'f') {
                            Digit = c - L'a' + 10;
                            }
                        else {
                            break;
                            }

                        if (Digit >= 16) {
                            break;
                            }

                        DefaultLocaleId = (DefaultLocaleId << 4) | Digit;
                        }
                    }
                else
                if (KeyValueInformation->Type == REG_DWORD &&
                    KeyValueInformation->DataLength == sizeof( ULONG )
                   ) {
                    DefaultLocaleId = *(PLCID)KeyValueInformation->Data;
                    }
                else {
                    Status = STATUS_UNSUCCESSFUL;
                    }
                }

            ZwClose( Key );
            }
        }
    else {

        Status = ExpValidateLocale( DefaultLocaleId );

        if (NT_SUCCESS(Status)) {

            Status = ZwOpenKey( &Key,
                                GENERIC_WRITE,
                                &ObjectAttributes
                              );

            if (NT_SUCCESS( Status )) {
                if (UserProfile) {
                    n = 8;
                    }
                else {
                    n = 4;
                    }
                s = &KeyValueBuffer[ n ];
                *s-- = UNICODE_NULL;
                i = (ULONG)DefaultLocaleId;
                while (s >= KeyValueBuffer) {
                    Digit = i & 0x0000000F;
                    if (Digit <= 9) {
                        *s-- = (WCHAR)(Digit + L'0');
                        }
                    else {
                        *s-- = (WCHAR)((Digit - 10) + L'A');
                        }

                    i = i >> 4;
                    }

                Status = ZwSetValueKey( Key,
                                        &KeyValueName,
                                        0,
                                        REG_SZ,
                                        KeyValueBuffer,
                                        (n+1) * sizeof( WCHAR )
                                      );
                ZwClose( Key );
                }
            }
        }

    ZwClose( CurrentUserKey );

    if (NT_SUCCESS( Status )) {
        if (UserProfile) {
            PsDefaultThreadLocaleId = DefaultLocaleId;
            }
        else {
            PsDefaultSystemLocaleId = DefaultLocaleId;
            }
        }

    return Status;
}

NTSTATUS
NtQueryInstallUILanguage(
    OUT LANGID *InstallUILanguageId
    )
{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PAGED_CODE();

    Status = STATUS_SUCCESS;
    try {

        //
        // Get previous processor mode and probe output argument if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWriteUshort( (USHORT *)InstallUILanguageId );
            }

        *InstallUILanguageId = PsInstallUILanguageId;
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }

    return Status;
}

NTSTATUS
NtQueryDefaultUILanguage(
    OUT LANGID *DefaultUILanguageId
    )
{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PAGED_CODE();

    Status = STATUS_SUCCESS;
    try {

        //
        // Get previous processor mode and probe output argument if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWriteUshort( (USHORT *)DefaultUILanguageId );
            }

        //
        // Read the UI language from the current security context.
        //
        if (!NT_SUCCESS(ExpGetCurrentUserUILanguage( L"MultiUILanguageId", 
                                                     DefaultUILanguageId))) {
            *DefaultUILanguageId = PsInstallUILanguageId;
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }

    return Status;
}



NTSTATUS
ExpGetUILanguagePolicy(
    IN HANDLE CurrentUserKey,
    OUT LANGID *PolicyUILanguageId
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes, MuiObjectAttributes;
    UNICODE_STRING KeyPath, KeyValueName, MuiLanguagesKeyPath;
    HANDLE Key, KeyMui;
    WCHAR KeyValueBuffer[ 128 ];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    ULONG ResultLength;
    ULONG Language;

    PAGED_CODE();


    //
    // Let's verify that this is an MUI system first
    //
    RtlInitUnicodeString( &MuiLanguagesKeyPath, 
                          L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Nls\\MUILanguages" );
    InitializeObjectAttributes( &MuiObjectAttributes,
                                &MuiLanguagesKeyPath,
                                (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                NULL,
                                NULL
                              );

    Status = ZwOpenKey( &KeyMui,
                        GENERIC_READ,
                        &MuiObjectAttributes
                      );
    if (!NT_SUCCESS( Status )) {
        return Status;
        }

    ZwClose( KeyMui );



    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueBuffer;
    RtlInitUnicodeString( &KeyValueName, L"MultiUILanguageId" );
    RtlInitUnicodeString( &KeyPath, L"Software\\Policies\\Microsoft\\Control Panel\\Desktop" );

    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyPath,
                                (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                CurrentUserKey,
                                NULL
                              );

    //
    // Check if there is a Policy key
    //
    Status = ZwOpenKey( &Key,
                        GENERIC_READ,
                        &ObjectAttributes
                      );

    if (NT_SUCCESS( Status )) {

        Status = ZwQueryValueKey( Key,
                                  &KeyValueName,
                                  KeyValuePartialInformation,
                                  KeyValueInformation,
                                  sizeof( KeyValueBuffer ),
                                  &ResultLength
                                );

        if (NT_SUCCESS( Status )) {
            if ((KeyValueInformation->DataLength > 2) &&
                (KeyValueInformation->Type == REG_SZ)) {

                RtlInitUnicodeString( &KeyValueName, (PWSTR) KeyValueInformation->Data );
                Status = RtlUnicodeStringToInteger( &KeyValueName,
                                                    (ULONG)16,
                                                    &Language
                                                  );
                //
                // Final check to make sure this is an MUI system
                //
                if (NT_SUCCESS( Status )) {
                    *PolicyUILanguageId = (LANGID)Language;
                    }
                }
            else {
                Status = STATUS_UNSUCCESSFUL;
                }
            }
            ZwClose( Key );
        }

    return Status;
}



NTSTATUS
ExpSetCurrentUserUILanguage(
    IN WCHAR *ValueName,
    IN LANGID CurrentUserUILanguage
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyPath, KeyValueName, UILanguage;
    HANDLE CurrentUserKey, Key;       
    WCHAR KeyValueBuffer[ 128 ];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    PWSTR s;
    ULONG i, Digit;
    WCHAR c;

    PAGED_CODE();

    if (CurrentUserUILanguage & 0xFFFF0000) {
        return STATUS_INVALID_PARAMETER;
        }

    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueBuffer;
    Status = RtlOpenCurrentUser( MAXIMUM_ALLOWED, &CurrentUserKey );
    if (!NT_SUCCESS( Status )) {
        return Status;
        }

    RtlInitUnicodeString( &KeyValueName, ValueName );
    RtlInitUnicodeString( &KeyPath, L"Control Panel\\Desktop" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyPath,
                                (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                CurrentUserKey,
                                NULL
                              );


    Status = ExpValidateLocale( MAKELCID( CurrentUserUILanguage, SORT_DEFAULT ) );
    
    if (NT_SUCCESS(Status)) {

        Status = ZwOpenKey( &Key,
                            GENERIC_WRITE,
                            &ObjectAttributes
                          );
        if (NT_SUCCESS( Status )) {

            s = &KeyValueBuffer[ 8 ];
            *s-- = UNICODE_NULL;
            i = (ULONG)CurrentUserUILanguage;

            while (s >= KeyValueBuffer) {
                Digit = i & 0x0000000F;
                if (Digit <= 9) {
                    *s-- = (WCHAR)(Digit + L'0');
                    }
                else {
                    *s-- = (WCHAR)((Digit - 10) + L'A');
                    }

                i = i >> 4;
                }

            Status = ZwSetValueKey( Key,
                                    &KeyValueName,
                                    0,
                                    REG_SZ,
                                    KeyValueBuffer,
                                    9 * sizeof( WCHAR )
                                  );
            ZwClose( Key );
            }
        }

    ZwClose( CurrentUserKey );

    return Status;
}


NTSTATUS
ExpGetCurrentUserUILanguage(
    IN WCHAR *ValueName,
    OUT LANGID *CurrentUserUILanguageId
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyPath, KeyValueName, UILanguage;
    HANDLE CurrentUserKey, Key;       
    WCHAR KeyValueBuffer[ 128 ];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    ULONG ResultLength;
    ULONG Digit;

    PAGED_CODE();

    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueBuffer;
    Status = RtlOpenCurrentUser( MAXIMUM_ALLOWED, &CurrentUserKey );
    if (!NT_SUCCESS( Status )) {
        return Status;
        }

    RtlInitUnicodeString( &KeyValueName, ValueName );
    RtlInitUnicodeString( &KeyPath, L"Control Panel\\Desktop" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyPath,
                                (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                CurrentUserKey,
                                NULL
                              );

    //
    // Let's check if there is a policy installed for the UI language,
    // and if so, let's use it.
    //
    if (!NT_SUCCESS( ExpGetUILanguagePolicy( CurrentUserKey, CurrentUserUILanguageId ))) {
        Status = ZwOpenKey( &Key,
                            GENERIC_READ,
                            &ObjectAttributes
                          );
        if (NT_SUCCESS( Status )) {
            Status = ZwQueryValueKey( Key,
                                      &KeyValueName,
                                      KeyValuePartialInformation,
                                      KeyValueInformation,
                                      sizeof( KeyValueBuffer ),
                                      &ResultLength
                                    );
            if (NT_SUCCESS( Status )) {
                if (KeyValueInformation->Type == REG_SZ) {

                    RtlInitUnicodeString( &UILanguage, (PWSTR) KeyValueInformation->Data);
                    Status = RtlUnicodeStringToInteger( &UILanguage,
                                                        (ULONG) 16,
                                                        &Digit
                                                      );
                    if (NT_SUCCESS( Status )) {
                        *CurrentUserUILanguageId = (LANGID) Digit;
                        }
                    }
                else {
                    Status = STATUS_UNSUCCESSFUL;
                    }
                }
            ZwClose( Key );
            }
        }

    ZwClose( CurrentUserKey );

    return Status;
}


NTSTATUS
NtSetDefaultUILanguage(
    IN LANGID DefaultUILanguageId
    )
{
    NTSTATUS Status;
    LANGID LangId;

    //
    //  if this is called during user logon, then we need to update the user's registry.
    //
    if (DefaultUILanguageId == 0) {
        
          Status = ExpGetCurrentUserUILanguage( L"MUILanguagePending" , 
                                                &LangId 
                                              );
          if (NT_SUCCESS( Status )) {
            Status = ExpSetCurrentUserUILanguage( L"MultiUILanguageId" ,
                                                  LangId
                                                );
            }
          
          return Status;
        }
    
    return ExpSetCurrentUserUILanguage( L"MUILanguagePending", DefaultUILanguageId );
}

NTSTATUS
ExpValidateLocale(
    IN LCID LocaleId
    )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER, ReturnStatus;
    UNICODE_STRING LocaleName, KeyValueName;
    UNICODE_STRING NlsLocaleKeyPath, NlsSortKeyPath, NlsLangGroupKeyPath;
    WCHAR LocaleNameBuffer[ 32 ];
    WCHAR KeyValueNameBuffer[ 32 ];
    WCHAR KeyValueBuffer[ 128 ];
    WCHAR *Ptr;
    HANDLE LocaleKey, SortKey, LangGroupKey;
    OBJECT_ATTRIBUTES NlsLocaleObjA, NlsSortObjA, NlsLangGroupObjA;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    ULONG i, ResultLength;


    //
    //  Convert the LCID to the form %08x (e.g. 00000409)
    //
    LocaleName.Length = sizeof( LocaleNameBuffer ) / sizeof( WCHAR );
    LocaleName.MaximumLength = LocaleName.Length;
    LocaleName.Buffer = LocaleNameBuffer;

    //
    //  Convert LCID to a string
    //
    ReturnStatus = RtlIntegerToUnicodeString( LocaleId, 16, &LocaleName );
    if (!NT_SUCCESS(ReturnStatus))
        goto Failed1;

    Ptr = KeyValueNameBuffer;
    for (i = ((LocaleName.Length)/sizeof(WCHAR));
         i < 8;
         i++, Ptr++) {
        *Ptr = L'0';
        }
    *Ptr = UNICODE_NULL;

    RtlInitUnicodeString(&KeyValueName, KeyValueNameBuffer);
    KeyValueName.MaximumLength = sizeof( KeyValueNameBuffer ) / sizeof( WCHAR );
    RtlAppendUnicodeToString(&KeyValueName, LocaleName.Buffer);


    //
    // Open Registry Keys : Locale, Sort and LanguageGroup
    //
    RtlInitUnicodeString(&NlsLocaleKeyPath, 
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Nls\\Locale");

    InitializeObjectAttributes( &NlsLocaleObjA,
                                &NlsLocaleKeyPath,
                                (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                NULL,
                                NULL
                              );

    ReturnStatus = ZwOpenKey( &LocaleKey,
                              GENERIC_READ,
                              &NlsLocaleObjA 
                            );
    if (!NT_SUCCESS(ReturnStatus))
         goto Failed1;

    RtlInitUnicodeString(&NlsSortKeyPath, 
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Nls\\Locale\\Alternate Sorts");

    InitializeObjectAttributes( &NlsSortObjA,
                                &NlsSortKeyPath,
                                (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                NULL,
                                NULL
                              );

    ReturnStatus = ZwOpenKey( &SortKey,
                              GENERIC_READ,
                              &NlsSortObjA 
                            );
    if (!NT_SUCCESS(ReturnStatus))
         goto Failed2;

    RtlInitUnicodeString(&NlsLangGroupKeyPath, 
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Nls\\Language Groups");

    InitializeObjectAttributes( &NlsLangGroupObjA,
                                &NlsLangGroupKeyPath,
                                (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                NULL,
                                NULL
                              );


    ReturnStatus = ZwOpenKey( &LangGroupKey,
                              GENERIC_READ,
                              &NlsLangGroupObjA 
                            );
    if (!NT_SUCCESS(ReturnStatus))
         goto Failed3;

    //
    // Validate Locale : Lookup the Locale's Language group, and make sure it is there.
    //
    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) KeyValueBuffer;
    ReturnStatus = ZwQueryValueKey( LocaleKey,
                                    &KeyValueName,
                                    KeyValuePartialInformation,
                                    KeyValueInformation,
                                    sizeof( KeyValueBuffer ),
                                    &ResultLength
                                  );

    if (!NT_SUCCESS(ReturnStatus)) {
        ReturnStatus = ZwQueryValueKey( SortKey,
                                        &KeyValueName,
                                        KeyValuePartialInformation,
                                        KeyValueInformation,
                                        sizeof( KeyValueBuffer ),
                                        &ResultLength
                                      );
        }

    if ((NT_SUCCESS(ReturnStatus)) &&
        (KeyValueInformation->DataLength > 2)
       ) {

        RtlInitUnicodeString( &KeyValueName, (PWSTR) KeyValueInformation->Data );

        ReturnStatus = ZwQueryValueKey( LangGroupKey,
                                        &KeyValueName,
                                        KeyValuePartialInformation,
                                        KeyValueInformation,
                                        sizeof( KeyValueBuffer ),
                                        &ResultLength
                                      );
        if ((NT_SUCCESS(ReturnStatus)) &&
            (KeyValueInformation->Type == REG_SZ) &&
            (KeyValueInformation->DataLength > 2)
           ) {
            Ptr = (PWSTR) KeyValueInformation->Data;
            if (Ptr[0] == L'1' && Ptr[1] == UNICODE_NULL) {
                Status = STATUS_SUCCESS;
                }
            }
        }

    //
    // Close opened keys
    //

    ZwClose( LangGroupKey );

Failed3:
    ZwClose( SortKey );

Failed2:
    ZwClose( LocaleKey );

Failed1:

    //
    // If an error happens, let's record it.
    //
    if (!NT_SUCCESS(ReturnStatus)) {
        Status = ReturnStatus;
        }

    return Status;
}



NTSTATUS
NtQuerySystemInformation (
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )

/*++

Routine Description:

    This function queries information about the system.

Arguments:

    SystemInformationClass - The system information class about which
        to retrieve information.

    SystemInformation - A pointer to a buffer which receives the specified
        information.  The format and content of the buffer depend on the
        specified system information class.

        SystemInformation Format by Information Class:

        SystemBasicInformation - Data type is SYSTEM_BASIC_INFORMATION

            SYSTEM_BASIC_INFORMATION Structure

                ULONG Reserved - Always zero.

                ULONG TimerResolutionInMicroSeconds - The resolution of
                    the hardware time.  All time values in NT are
                    specified as 64-bit LARGE_INTEGER values in units of
                    100 nanoseconds.  This field allows an application to
                    understand how many of the low order bits of a system
                    time value are insignificant.

                ULONG PageSize - The physical page size for virtual memory
                    objects.  Physical memory is committed in PageSize
                    chunks.

                ULONG AllocationGranularity - The logical page size for
                    virtual memory objects.  Allocating 1 byte of virtual
                    memory will actually allocate AllocationGranularity
                    bytes of virtual memory.  Storing into that byte will
                    commit the first physical page of the virtual memory.

                ULONG MinimumUserModeAddress - The smallest valid user mode
                    address.  The first AllocationGranularity bytes of
                    the virtual address space are reserved.  This forces
                    access violations for code the dereferences a zero
                    pointer.

                ULONG MaximumUserModeAddress -  The largest valid user mode
                    address.  The next AllocationGranularity bytes of
                    the virtual address space are reserved.  This allows
                    system service routines to validate user mode pointer
                    parameters quickly.

                KAFFINITY ActiveProcessorsAffinityMask - The affinity mask
                    for the current hardware configuration.

                CCHAR NumberOfProcessors - The number of processors
                    in the current hardware configuration.

        SystemProcessorInformation - Data type is SYSTEM_PROCESSOR_INFORMATION

            SYSTEM_PROCESSOR_INFORMATION Structure

                USHORT ProcessorArchitecture - The processor architecture:
                    PROCESSOR_ARCHITECTURE_INTEL
                    PROCESSOR_ARCHITECTURE_MIPS
                    PROCESSOR_ARCHITECTURE_ALPHA
                    PROCESSOR_ARCHITECTURE_PPC

                USHORT ProcessorLevel - architecture dependent processor level.
                    This is the least common denominator for an MP system:

                    For PROCESSOR_ARCHITECTURE_INTEL:
                        3 - 386
                        4 - 486
                        5 - 586 or Pentium

                    For PROCESSOR_ARCHITECTURE_MIPS:
                        00xx - where xx is 8-bit implementation number (bits 8-15 of
                            PRId register.
                        0004 - R4000

                    For PROCESSOR_ARCHITECTURE_ALPHA:
                        xxxx - where xxxx is 16-bit processor version number (low
                            order 16 bits of processor version number from firmware)

                        21064 - 21064
                        21066 - 21066
                        21164 - 21164

                    For PROCESSOR_ARCHITECTURE_PPC:
                        xxxx - where xxxx is 16-bit processor version number (high
                            order 16 bits of Processor Version Register).
                        1 - 601
                        3 - 603
                        4 - 604
                        6 - 603+
                        9 - 604+
                        20 - 620

                USHORT ProcessorRevision - architecture dependent processor revision.
                    This is the least common denominator for an MP system:

                    For PROCESSOR_ARCHITECTURE_INTEL:
                        For Old Intel 386 or 486:
                            FFxx - where xx is displayed as a hexadecimal CPU stepping
                            (e.g. FFD0 is D0 stepping)

                        For Intel Pentium or Cyrix/NexGen 486
                            xxyy - where xx is model number and yy is stepping, so
                            0201 is Model 2, Stepping 1

                    For PROCESSOR_ARCHITECTURE_MIPS:
                        00xx is 8-bit revision number of processor (low order 8 bits
                            of PRId Register

                    For PROCESSOR_ARCHITECTURE_ALPHA:
                        xxyy - where xxyy is 16-bit processor revision number (low
                            order 16 bits of processor revision number from firmware).
                            Displayed as Model 'A'+xx, Pass yy

                    For PROCESSOR_ARCHITECTURE_PPC:
                        xxyy - where xxyy is 16-bit processor revision number (low
                            order 16 bits of Processor Version Register).  Displayed
                            as a fixed point number xx.yy

                USHORT Reserved - Always zero.

                ULONG ProcessorFeatureBits - architecture dependent processor feature bits.
                    This is the least common denominator for an MP system.

        SystemPerformanceInformation - Data type is SYSTEM_PERFORMANCE_INFORMATION

            SYSTEM_PERFORMANCE_INFORMATION Structure

                LARGE_INTEGER IdleProcessTime - Returns the kernel time of the idle
                    process.
        BUGBUG complete comment.
            LARGE_INTEGER IoReadTransferCount;
            LARGE_INTEGER IoWriteTransferCount;
            LARGE_INTEGER IoOtherTransferCount;
            LARGE_INTEGER KernelTime;
            LARGE_INTEGER UserTime;
            ULONG IoReadOperationCount;
            ULONG IoWriteOperationCount;
            ULONG IoOtherOperationCount;
            ULONG AvailablePages;
            ULONG CommittedPages;
            ULONG PageFaultCount;
            ULONG CopyOnWriteCount;
            ULONG TransitionCount;
            ULONG CacheTransitionCount;
            ULONG DemandZeroCount;
            ULONG PageReadCount;
            ULONG PageReadIoCount;
            ULONG CacheReadCount;
            ULONG CacheIoCount;
            ULONG DirtyPagesWriteCount;
            ULONG DirtyWriteIoCount;
            ULONG MappedPagesWriteCount;
            ULONG MappedWriteIoCount;
            ULONG PagedPoolPages;
            ULONG NonPagedPoolPages;
            ULONG PagedPoolAllocs;
            ULONG PagedPoolFrees;
            ULONG NonPagedPoolAllocs;
            ULONG NonPagedPoolFrees;
            ULONG LpcThreadsWaitingInReceive;
            ULONG LpcThreadsWaitingForReply;

        SystemProcessInformation - Data type is SYSTEM_PROCESS_INFORMATION

            SYSTEM_PROCESSOR_INFORMATION Structure
                BUGBUG - add here when done.

        SystemDockInformation - Data type is SYSTEM_DOCK_INFORMATION

             SYSTEM_DOCK_INFORMATION Structure

                 SYSTEM_DOCKED_STATE DockState - Ordinal specifying the current docking state. Possible values:
                     SystemDockStateUnknown - The docking state of the system could not be determined.
                     SystemUndocked - The system is undocked.
                     SystemDocked - The system is docked.

                 ULONG DockIdLength - Specifies the length in characters of the Dock ID string
                                      (not including terminating NULL).

                 ULONG SerialNumberOffset - Specifies the character offset of the Serial Number within
                                            the DockId buffer.

                 ULONG SerialNumberLength - Specifies the length in characters of the Serial Number
                                            string (not including terminating NULL).

                 WCHAR DockId - Character buffer containing two null-terminated strings.  The first
                                string is a character representation of the dock ID number, starting
                                at the beginning of the buffer.  The second string is a character
                                representation of the machine's serial number, starting at character
                                offset SerialNumberOffset in the buffer.


        SystemPowerSettings - Data type is SYSTEM_POWER_SETTINGS
            SYSTEM_POWER_INFORMATION Structure
                BOOLEAN SystemSuspendSupported - Supplies a BOOLEAN as to
                    whether the system suspend is enabled or not.
                BOOLEAN SystemHibernateSupported - Supplies a BOOLEAN as to
                    whether the system hibernate is enabled or not.
                BOOLEAN ResumeTimerSupportsSuspend - Supplies a BOOLEAN as to
                    whether the resuming from an external programmed timer
                    from within a system suspend is enabled or not.
                BOOLEAN ResumeTimerSupportsHibernate - Supplies a BOOLEAN as to
                    whether or resuming from an external programmed timer
                    from within a system hibernate is enabled or not.
                BOOLEAN LidSupported - Supplies a BOOLEAN as to whether or not
                    the suspending and resuming by Lid are enabled or not.
                BOOLEAN TurboSettingSupported - Supplies a BOOLEAN as to whether
                    or not the system supports a turbo mode setting.
                BOOLEAN TurboMode - Supplies a BOOLEAN as to whether or not
                    the system is in turbo mode.
                BOOLEAN SystemAcOrDc - Supplies a BOOLEAN as to whether or not
                    the system is in AC mode.
                BOOLEAN DisablePowerDown - If TRUE, signifies that all requests to
                    PoRequestPowerChange for a SET_POWER-PowerDown irp are to
                    be ignored.
                LARGE_INTEGER SpindownDrives - If non-zero, signifies to the
                    cache manager (or the IO subsystem) to optimize drive
                    accesses based upon power saves, are that drives are to
                    be spun down as appropriate. The value represents to user's
                    requested disk spin down timeout.

        SystemProcessorSpeedInformation - Data type is SYSTEM_PROCESSOR_SPEED_INFORMATION
            SYSTEM_PROCESSOR_SPEED_INFORMATION Structure (same as HalProcessorSpeedInformation)
                ULONG MaximumProcessorSpeed - The maximum hertz the processor is
                    capable of. This information is used by the UI to draw the
                    appropriate scale. This field is read-only and cannot be
                    set.
                ULONG CurrentAvailableSpeed - The hertz for which the processor
                    runs at when not idle. This field is read-only and cannot
                    be set.
                ULONG ConfiguredSpeedLimit - The hertz for which the processor
                    is limited to due to the current configuration.
                UCHAR PowerState
                    0 - Normal
                    1 - The processor speed is being limited due to available
                    power restrictions. This field id read-only by the system.
                UCHAR ThermalState
                    0 - Normal
                    1 - The processors speed is being limited due to thermal
                    restrictions. This field is read-only by the system.
                UCHAR TurboState
                    0 - Normal
                    1 - The processors speed is being limited by the fact that
                    the system turbo mode is currently disabled which is
                    requested to obtain more processor speed.

    SystemInformationLength - Specifies the length in bytes of the system
        information buffer.

    ReturnLength - An optional pointer which, if specified, receives the
        number of bytes placed in the system information buffer.

Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_INVALID_INFO_CLASS - The SystemInformationClass parameter
            did not specify a valid value.

        STATUS_INFO_LENGTH_MISMATCH - The value of the SystemInformationLength
            parameter did not match the length required for the information
            class requested by the SystemInformationClass parameter.

        STATUS_ACCESS_VIOLATION - Either the SystemInformation buffer pointer
            or the ReturnLength pointer value specified an invalid address.

        STATUS_WORKING_SET_QUOTA - The process does not have sufficient
            working set to lock the specified output structure in memory.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
            for this request to complete.

--*/

{

    KPROCESSOR_MODE PreviousMode;
    PSYSTEM_BASIC_INFORMATION BasicInfo;
    PSYSTEM_PROCESSOR_INFORMATION ProcessorInfo;
    SYSTEM_TIMEOFDAY_INFORMATION LocalTimeOfDayInfo;
    SYSTEM_PERFORMANCE_INFORMATION LocalPerformanceInfo;
    PSYSTEM_PERFORMANCE_INFORMATION PerformanceInfo;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION ProcessorPerformanceInfo;
    PSYSTEM_CALL_COUNT_INFORMATION CallCountInformation;
    PSYSTEM_DEVICE_INFORMATION DeviceInformation;
    PCONFIGURATION_INFORMATION ConfigInfo;
    PSYSTEM_EXCEPTION_INFORMATION ExceptionInformation;
    PSYSTEM_FILECACHE_INFORMATION FileCache;
    PSYSTEM_QUERY_TIME_ADJUST_INFORMATION TimeAdjustmentInformation;
    PSYSTEM_KERNEL_DEBUGGER_INFORMATION KernelDebuggerInformation;
    PSYSTEM_CONTEXT_SWITCH_INFORMATION ContextSwitchInformation;
    PSYSTEM_INTERRUPT_INFORMATION InterruptInformation;
    PSYSTEM_SESSION_PROCESS_INFORMATION SessionProcessInformation;
    PVOID ProcessInformation;
    ULONG ProcessInformationLength;

    NTSTATUS Status;
    BOOLEAN ReleaseModuleResoure = FALSE;
    PKPRCB Prcb;
    ULONG Length = 0;
    ULONG i;
    ULONG ContextSwitches;
    PULONG TableLimit, TableCounts;
    PKSERVICE_TABLE_DESCRIPTOR Table;
    ULONG SessionId;

    PAGED_CODE();

    //
    // Assume successful completion.
    //

    Status = STATUS_SUCCESS;
    try {

        //
        // Get previous processor mode and probe output argument if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWrite(SystemInformation,
                          SystemInformationLength,
                          SystemInformationClass == SystemKernelDebuggerInformation ? sizeof(BOOLEAN)
                                                                                    : sizeof(ULONG));

            if (ARGUMENT_PRESENT(ReturnLength)) {
                ProbeForWriteUlong(ReturnLength);
            }
        }

        if (ARGUMENT_PRESENT(ReturnLength)) {
            *ReturnLength = 0;
        }

        switch (SystemInformationClass) {

        case SystemBasicInformation:

            if (SystemInformationLength != sizeof( SYSTEM_BASIC_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            BasicInfo = (PSYSTEM_BASIC_INFORMATION)SystemInformation;
            BasicInfo->NumberOfProcessors = KeNumberProcessors;
            BasicInfo->ActiveProcessorsAffinityMask = (ULONG_PTR)KeActiveProcessors;
            BasicInfo->Reserved = 0;
            BasicInfo->TimerResolution = KeMaximumIncrement;
            BasicInfo->NumberOfPhysicalPages = MmNumberOfPhysicalPages;
            BasicInfo->LowestPhysicalPageNumber = MmLowestPhysicalPage;
            BasicInfo->HighestPhysicalPageNumber = MmHighestPhysicalPage;
            BasicInfo->PageSize = PAGE_SIZE;
            BasicInfo->AllocationGranularity = MM_ALLOCATION_GRANULARITY;
            BasicInfo->MinimumUserModeAddress = (ULONG_PTR)MM_LOWEST_USER_ADDRESS;
            BasicInfo->MaximumUserModeAddress = (ULONG_PTR)MM_HIGHEST_USER_ADDRESS;

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof( SYSTEM_BASIC_INFORMATION );
                }
            break;

        case SystemProcessorInformation:
            if (SystemInformationLength < sizeof( SYSTEM_PROCESSOR_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            ProcessorInfo = (PSYSTEM_PROCESSOR_INFORMATION)SystemInformation;

            ProcessorInfo->ProcessorArchitecture = KeProcessorArchitecture;
            ProcessorInfo->ProcessorLevel = KeProcessorLevel;
            ProcessorInfo->ProcessorRevision = KeProcessorRevision;
            ProcessorInfo->Reserved = 0;
            ProcessorInfo->ProcessorFeatureBits = KeFeatureBits;

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof( SYSTEM_PROCESSOR_INFORMATION );
                }

            break;

        case SystemPerformanceInformation:
            if (SystemInformationLength < sizeof( SYSTEM_PERFORMANCE_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            PerformanceInfo = (PSYSTEM_PERFORMANCE_INFORMATION)SystemInformation;

            //
            // Io information.
            //

            LocalPerformanceInfo.IoReadTransferCount = IoReadTransferCount;
            LocalPerformanceInfo.IoWriteTransferCount = IoWriteTransferCount;
            LocalPerformanceInfo.IoOtherTransferCount = IoOtherTransferCount;
            LocalPerformanceInfo.IoReadOperationCount = IoReadOperationCount;
            LocalPerformanceInfo.IoWriteOperationCount = IoWriteOperationCount;
            LocalPerformanceInfo.IoOtherOperationCount = IoOtherOperationCount;

            //
            // Ke information.
            //
            // These counters are kept on a per processor basis and must
            // be totaled.
            //

            {
                ULONG FirstLevelTbFills = 0;
                ULONG SecondLevelTbFills = 0;
                ULONG SystemCalls = 0;
//                ULONG InterruptCount = 0;

                ContextSwitches = 0;
                for (i = 0; i < (ULONG)KeNumberProcessors; i += 1) {
                    Prcb = KiProcessorBlock[i];
                    if (Prcb != NULL) {
                        ContextSwitches += Prcb->KeContextSwitches;
                        FirstLevelTbFills += Prcb->KeFirstLevelTbFills;
//                        InterruptCount += Prcb->KeInterruptCount;
                        SecondLevelTbFills += Prcb->KeSecondLevelTbFills;
                        SystemCalls += Prcb->KeSystemCalls;
                    }
                }

                LocalPerformanceInfo.ContextSwitches = ContextSwitches;
                LocalPerformanceInfo.FirstLevelTbFills = FirstLevelTbFills;
//                LocalPerformanceInfo.InterruptCount = KeInterruptCount;
                LocalPerformanceInfo.SecondLevelTbFills = SecondLevelTbFills;
                LocalPerformanceInfo.SystemCalls = SystemCalls;
            }

            //
            // Mm information.
            //

            LocalPerformanceInfo.AvailablePages = MmAvailablePages;
            LocalPerformanceInfo.CommittedPages = MmTotalCommittedPages;
            LocalPerformanceInfo.CommitLimit = MmTotalCommitLimit;
            LocalPerformanceInfo.PeakCommitment = MmPeakCommitment;
            LocalPerformanceInfo.PageFaultCount = MmInfoCounters.PageFaultCount;
            LocalPerformanceInfo.CopyOnWriteCount = MmInfoCounters.CopyOnWriteCount;
            LocalPerformanceInfo.TransitionCount = MmInfoCounters.TransitionCount;
            LocalPerformanceInfo.CacheTransitionCount = MmInfoCounters.CacheTransitionCount;
            LocalPerformanceInfo.DemandZeroCount = MmInfoCounters.DemandZeroCount;
            LocalPerformanceInfo.PageReadCount = MmInfoCounters.PageReadCount;
            LocalPerformanceInfo.PageReadIoCount = MmInfoCounters.PageReadIoCount;
            LocalPerformanceInfo.CacheReadCount = MmInfoCounters.CacheReadCount;
            LocalPerformanceInfo.CacheIoCount = MmInfoCounters.CacheIoCount;
            LocalPerformanceInfo.DirtyPagesWriteCount = MmInfoCounters.DirtyPagesWriteCount;
            LocalPerformanceInfo.DirtyWriteIoCount = MmInfoCounters.DirtyWriteIoCount;
            LocalPerformanceInfo.MappedPagesWriteCount = MmInfoCounters.MappedPagesWriteCount;
            LocalPerformanceInfo.MappedWriteIoCount = MmInfoCounters.MappedWriteIoCount;
            LocalPerformanceInfo.FreeSystemPtes = MmTotalFreeSystemPtes[0];

            LocalPerformanceInfo.ResidentSystemCodePage = MmSystemCodePage;
            LocalPerformanceInfo.ResidentSystemCachePage = MmSystemCachePage;
            LocalPerformanceInfo.ResidentPagedPoolPage = MmPagedPoolPage;
            LocalPerformanceInfo.ResidentSystemDriverPage = MmSystemDriverPage;
            LocalPerformanceInfo.TotalSystemCodePages = MmTotalSystemCodePages;
            LocalPerformanceInfo.TotalSystemDriverPages = MmTotalSystemDriverPages;

            //
            // Process information.
            //

            LocalPerformanceInfo.IdleProcessTime.QuadPart =
                                    UInt32x32To64(PsIdleProcess->Pcb.KernelTime,
                                                  KeMaximumIncrement);

            //
            // Pool information.
            //

            LocalPerformanceInfo.PagedPoolPages = 0;
            LocalPerformanceInfo.NonPagedPoolPages = 0;
            LocalPerformanceInfo.PagedPoolAllocs = 0;
            LocalPerformanceInfo.PagedPoolFrees = 0;
            LocalPerformanceInfo.PagedPoolLookasideHits = 0;
            LocalPerformanceInfo.NonPagedPoolAllocs = 0;
            LocalPerformanceInfo.NonPagedPoolFrees = 0;
            LocalPerformanceInfo.NonPagedPoolLookasideHits = 0;
            ExQueryPoolUsage( &LocalPerformanceInfo.PagedPoolPages,
                              &LocalPerformanceInfo.NonPagedPoolPages,
                              &LocalPerformanceInfo.PagedPoolAllocs,
                              &LocalPerformanceInfo.PagedPoolFrees,
                              &LocalPerformanceInfo.PagedPoolLookasideHits,
                              &LocalPerformanceInfo.NonPagedPoolAllocs,
                              &LocalPerformanceInfo.NonPagedPoolFrees,
                              &LocalPerformanceInfo.NonPagedPoolLookasideHits
                            );

            //
            // Cache Manager information.
            //

            LocalPerformanceInfo.CcFastReadNoWait = CcFastReadNoWait;
            LocalPerformanceInfo.CcFastReadWait = CcFastReadWait;
            LocalPerformanceInfo.CcFastReadResourceMiss = CcFastReadResourceMiss;
            LocalPerformanceInfo.CcFastReadNotPossible = CcFastReadNotPossible;
            LocalPerformanceInfo.CcFastMdlReadNoWait = CcFastMdlReadNoWait;
            LocalPerformanceInfo.CcFastMdlReadWait = CcFastMdlReadWait;
            LocalPerformanceInfo.CcFastMdlReadResourceMiss = CcFastMdlReadResourceMiss;
            LocalPerformanceInfo.CcFastMdlReadNotPossible = CcFastMdlReadNotPossible;
            LocalPerformanceInfo.CcMapDataNoWait = CcMapDataNoWait;
            LocalPerformanceInfo.CcMapDataWait = CcMapDataWait;
            LocalPerformanceInfo.CcMapDataNoWaitMiss = CcMapDataNoWaitMiss;
            LocalPerformanceInfo.CcMapDataWaitMiss = CcMapDataWaitMiss;
            LocalPerformanceInfo.CcPinMappedDataCount = CcPinMappedDataCount;
            LocalPerformanceInfo.CcPinReadNoWait = CcPinReadNoWait;
            LocalPerformanceInfo.CcPinReadWait = CcPinReadWait;
            LocalPerformanceInfo.CcPinReadNoWaitMiss = CcPinReadNoWaitMiss;
            LocalPerformanceInfo.CcPinReadWaitMiss = CcPinReadWaitMiss;
            LocalPerformanceInfo.CcCopyReadNoWait = CcCopyReadNoWait;
            LocalPerformanceInfo.CcCopyReadWait = CcCopyReadWait;
            LocalPerformanceInfo.CcCopyReadNoWaitMiss = CcCopyReadNoWaitMiss;
            LocalPerformanceInfo.CcCopyReadWaitMiss = CcCopyReadWaitMiss;
            LocalPerformanceInfo.CcMdlReadNoWait = CcMdlReadNoWait;
            LocalPerformanceInfo.CcMdlReadWait = CcMdlReadWait;
            LocalPerformanceInfo.CcMdlReadNoWaitMiss = CcMdlReadNoWaitMiss;
            LocalPerformanceInfo.CcMdlReadWaitMiss = CcMdlReadWaitMiss;
            LocalPerformanceInfo.CcReadAheadIos = CcReadAheadIos;
            LocalPerformanceInfo.CcLazyWriteIos = CcLazyWriteIos;
            LocalPerformanceInfo.CcLazyWritePages = CcLazyWritePages;
            LocalPerformanceInfo.CcDataFlushes = CcDataFlushes;
            LocalPerformanceInfo.CcDataPages = CcDataPages;

#if !defined(NT_UP)
            //
            // On an MP machines go sum up some other 'hot' cache manager
            // statistics.
            //

            for (i = 0; i < (ULONG)KeNumberProcessors; i++) {
                Prcb = KiProcessorBlock[i];

                LocalPerformanceInfo.CcFastReadNoWait += Prcb->CcFastReadNoWait;
                LocalPerformanceInfo.CcFastReadWait += Prcb->CcFastReadWait;
                LocalPerformanceInfo.CcFastReadNotPossible += Prcb->CcFastReadNotPossible;
                LocalPerformanceInfo.CcCopyReadNoWait += Prcb->CcCopyReadNoWait;
                LocalPerformanceInfo.CcCopyReadWait += Prcb->CcCopyReadWait;
                LocalPerformanceInfo.CcCopyReadNoWaitMiss += Prcb->CcCopyReadNoWaitMiss;
            }
#endif
            *PerformanceInfo = LocalPerformanceInfo;
            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof(LocalPerformanceInfo);
                }

            break;

        case SystemProcessorPerformanceInformation:
            if (SystemInformationLength <
                sizeof( SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            ProcessorPerformanceInfo =
                (PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) SystemInformation;

            Length = 0;
            for (i = 0; i < (ULONG)KeNumberProcessors; i++) {
                Prcb = KiProcessorBlock[i];
                if (Prcb != NULL) {
                    if (SystemInformationLength < Length + sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION))
                        break;

                    Length += sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);

                    ProcessorPerformanceInfo->UserTime.QuadPart =
                                                UInt32x32To64(Prcb->UserTime,
                                                              KeMaximumIncrement);

                    ProcessorPerformanceInfo->KernelTime.QuadPart =
                                                UInt32x32To64(Prcb->KernelTime,
                                                              KeMaximumIncrement);

                    ProcessorPerformanceInfo->DpcTime.QuadPart =
                                                UInt32x32To64(Prcb->DpcTime,
                                                              KeMaximumIncrement);

                    ProcessorPerformanceInfo->InterruptTime.QuadPart =
                                                UInt32x32To64(Prcb->InterruptTime,
                                                              KeMaximumIncrement);

                    ProcessorPerformanceInfo->IdleTime.QuadPart =
                                                UInt32x32To64(Prcb->IdleThread->KernelTime,
                                                              KeMaximumIncrement);

                    ProcessorPerformanceInfo->InterruptCount = Prcb->InterruptCount;

                    ProcessorPerformanceInfo++;
                }
            }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }

            break;

        case SystemTimeOfDayInformation:
            if (SystemInformationLength > sizeof (SYSTEM_TIMEOFDAY_INFORMATION)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            RtlZeroMemory (&LocalTimeOfDayInfo, sizeof(LocalTimeOfDayInfo));
            KeQuerySystemTime(&LocalTimeOfDayInfo.CurrentTime);
            LocalTimeOfDayInfo.BootTime = KeBootTime;
            LocalTimeOfDayInfo.TimeZoneBias = ExpTimeZoneBias;
            LocalTimeOfDayInfo.TimeZoneId = ExpCurrentTimeZoneId;
            LocalTimeOfDayInfo.BootTimeBias = KeBootTimeBias;
            LocalTimeOfDayInfo.SleepTimeBias = KeInterruptTimeBias;

            try {
                RtlCopyMemory (
                    SystemInformation,
                    &LocalTimeOfDayInfo,
                    SystemInformationLength
                    );

                if (ARGUMENT_PRESENT(ReturnLength) ) {
                    *ReturnLength = SystemInformationLength;
                }
            } except(EXCEPTION_EXECUTE_HANDLER) {
                return STATUS_SUCCESS;
            }

            break;

            //
            // Query system time adjustment information.
            //

        case SystemTimeAdjustmentInformation:
            if (SystemInformationLength != sizeof( SYSTEM_QUERY_TIME_ADJUST_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            TimeAdjustmentInformation =
                    (PSYSTEM_QUERY_TIME_ADJUST_INFORMATION)SystemInformation;

            TimeAdjustmentInformation->TimeAdjustment = KeTimeAdjustment;
            TimeAdjustmentInformation->TimeIncrement = KeMaximumIncrement;
            TimeAdjustmentInformation->Enable = KeTimeSynchronization;
            break;

        case SystemSummaryMemoryInformation:
        case SystemFullMemoryInformation:

            if (SystemInformationLength < sizeof( SYSTEM_MEMORY_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            Status = MmMemoryUsage (SystemInformation,
                                    SystemInformationLength,
             (SystemInformationClass == SystemFullMemoryInformation) ? 0 : 1,
                                    &Length);

            if (NT_SUCCESS(Status) && ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
            }
            break;

        case SystemPathInformation:
            DbgPrint( "EX: SystemPathInformation now available via SharedUserData\n" );
            DbgBreakPoint();
            return STATUS_NOT_IMPLEMENTED;
            break;

        case SystemProcessInformation:
            if (SystemInformationLength < sizeof( SYSTEM_PROCESS_INFORMATION)) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            Status = ExpGetProcessInformation (SystemInformation,
                                               SystemInformationLength,
                                               &Length,
                                               NULL);

            if (NT_SUCCESS(Status) && ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
            }

            break;

        case SystemSessionProcessInformation:

            if (SystemInformationLength < sizeof( SYSTEM_SESSION_PROCESS_INFORMATION)) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            SessionProcessInformation =
                        (PSYSTEM_SESSION_PROCESS_INFORMATION)SystemInformation;

            //
            // The lower level locks the buffer specified below into memory using MmProbeAndLockPages.
            // We don't need to probe the buffers here.
            //
            SessionId = SessionProcessInformation->SessionId;
            ProcessInformation = SessionProcessInformation->Buffer;
            ProcessInformationLength = SessionProcessInformation->SizeOfBuf;

            if (ProcessInformationLength < sizeof(SYSTEM_PROCESS_INFORMATION)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            if (!POINTER_IS_ALIGNED (ProcessInformation, sizeof (ULONG))) {
                return STATUS_DATATYPE_MISALIGNMENT;
            }

            Status = ExpGetProcessInformation (ProcessInformation,
                                               ProcessInformationLength,
                                               &Length,
                                               &SessionId);

            if (NT_SUCCESS(Status) && ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
            }

            break;

        case SystemCallCountInformation:

            Length = sizeof(SYSTEM_CALL_COUNT_INFORMATION) +
                        (NUMBER_SERVICE_TABLES * sizeof(ULONG));
            for ( i = 0, Table = KeServiceDescriptorTableShadow;
                  i < NUMBER_SERVICE_TABLES;
                  i++, Table++ ) {
                if ( (Table->Limit != 0) && (Table->Count != NULL) ) {
                    Length += Table->Limit * sizeof(ULONG);
                }
            }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
            }

            if (SystemInformationLength < Length) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            CallCountInformation = (PSYSTEM_CALL_COUNT_INFORMATION)SystemInformation;
            CallCountInformation->Length = Length;
            CallCountInformation->NumberOfTables = NUMBER_SERVICE_TABLES;

            TableLimit = (PULONG)(CallCountInformation + 1);
            TableCounts = TableLimit + NUMBER_SERVICE_TABLES;
            for ( i = 0, Table = KeServiceDescriptorTableShadow;
                  i < NUMBER_SERVICE_TABLES;
                  i++, Table++ ) {
                if ((Table->Limit == 0) || (Table->Count == NULL)) {
                    *TableLimit++ = 0;
                } else {
                    *TableLimit++ = Table->Limit;
                    RtlMoveMemory((PVOID)TableCounts,
                                  (PVOID)Table->Count,
                                  Table->Limit * sizeof(ULONG));
                    TableCounts += Table->Limit;
                }
            }

            break;

        case SystemDeviceInformation:
            if (SystemInformationLength != sizeof( SYSTEM_DEVICE_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            ConfigInfo = IoGetConfigurationInformation();
            DeviceInformation = (PSYSTEM_DEVICE_INFORMATION)SystemInformation;
            DeviceInformation->NumberOfDisks = ConfigInfo->DiskCount;
            DeviceInformation->NumberOfFloppies = ConfigInfo->FloppyCount;
            DeviceInformation->NumberOfCdRoms = ConfigInfo->CdRomCount;
            DeviceInformation->NumberOfTapes = ConfigInfo->TapeCount;
            DeviceInformation->NumberOfSerialPorts = ConfigInfo->SerialCount;
            DeviceInformation->NumberOfParallelPorts = ConfigInfo->ParallelCount;

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof( SYSTEM_DEVICE_INFORMATION );
                }
            break;

        case SystemFlagsInformation:
            if (SystemInformationLength != sizeof( SYSTEM_FLAGS_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            ((PSYSTEM_FLAGS_INFORMATION)SystemInformation)->Flags = NtGlobalFlag;

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof( SYSTEM_FLAGS_INFORMATION );
                }
            break;

        case SystemCallTimeInformation:
            return STATUS_NOT_IMPLEMENTED;

        case SystemModuleInformation:
            KeEnterCriticalRegion();
            ExAcquireResourceExclusive( &PsLoadedModuleResource, TRUE );
            ReleaseModuleResoure = TRUE;
            Status = ExpQueryModuleInformation( &PsLoadedModuleList,
                                                &MmLoadedUserImageList,
                                                (PRTL_PROCESS_MODULES)SystemInformation,
                                                SystemInformationLength,
                                                ReturnLength
                                              );
            ExReleaseResource (&PsLoadedModuleResource);
            ReleaseModuleResoure = FALSE;
            KeLeaveCriticalRegion();
            break;

        case SystemLocksInformation:
            if (SystemInformationLength < sizeof( RTL_PROCESS_LOCKS )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            Status = ExpGetLockInformation (SystemInformation,
                                            SystemInformationLength,
                                            &Length);

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }

            break;

        case SystemStackTraceInformation:
            if (SystemInformationLength < sizeof( RTL_PROCESS_BACKTRACES )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

#if i386 && !FPO
            Status = ExpGetStackTraceInformation (SystemInformation,
                                                  SystemInformationLength,
                                                  &Length);
#else
            Status = STATUS_NOT_IMPLEMENTED;
#endif // i386 && !FPO

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }

            break;

        case SystemPagedPoolInformation:
            if (SystemInformationLength < sizeof( SYSTEM_POOL_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            Status = ExpGetPoolInformation( PagedPool,
                                            SystemInformation,
                                            SystemInformationLength,
                                            &Length
                                          );

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }
            break;

        case SystemNonPagedPoolInformation:
            if (SystemInformationLength < sizeof( SYSTEM_POOL_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            Status = ExpGetPoolInformation( NonPagedPool,
                                            SystemInformation,
                                            SystemInformationLength,
                                            &Length
                                          );

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }
            break;

        case SystemHandleInformation:
            if (SystemInformationLength < sizeof( SYSTEM_HANDLE_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            Status = ExpGetHandleInformation( SystemInformation,
                                              SystemInformationLength,
                                              &Length
                                            );

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }
            break;

        case SystemObjectInformation:
            if (SystemInformationLength < sizeof( SYSTEM_OBJECTTYPE_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            Status = ExpGetObjectInformation( SystemInformation,
                                              SystemInformationLength,
                                              &Length
                                            );

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }
            break;

        case SystemPageFileInformation:

            if (SystemInformationLength < sizeof( SYSTEM_PAGEFILE_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            Status = MmGetPageFileInformation( SystemInformation,
                                               SystemInformationLength,
                                               &Length
                                              );

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }
            break;


        case SystemFileCacheInformation:

            //
            // This structure was extended in NT 4.0 from 12 bytes.
            // Use the previous size of 12 bytes for versioning info.
            //

            if (SystemInformationLength < 12) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            FileCache = (PSYSTEM_FILECACHE_INFORMATION)SystemInformation;
            FileCache->CurrentSize = MmSystemCacheWs.WorkingSetSize << PAGE_SHIFT;
            FileCache->PeakSize = MmSystemCacheWs.PeakWorkingSetSize << PAGE_SHIFT;
            FileCache->CurrentSizeIncludingTransitionInPages = MmSystemCacheWs.WorkingSetSize + MmTransitionSharedPages;
            FileCache->PeakSizeIncludingTransitionInPages = MmTransitionSharedPagesPeak;
            FileCache->PageFaultCount = MmSystemCacheWs.PageFaultCount;

            i = 12;
            if (SystemInformationLength >= sizeof( SYSTEM_FILECACHE_INFORMATION )) {
                i = sizeof (SYSTEM_FILECACHE_INFORMATION);
                FileCache->MinimumWorkingSet =
                                MmSystemCacheWs.MinimumWorkingSetSize;
                FileCache->MaximumWorkingSet =
                                MmSystemCacheWs.MaximumWorkingSetSize;
            }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = i;
            }
            break;

        case SystemPoolTagInformation:

#ifdef POOL_TAGGING
            if (SystemInformationLength < sizeof( SYSTEM_POOLTAG_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            Status = ExpGetPoolTagInfo (SystemInformation,
                                        SystemInformationLength,
                                        ReturnLength);
#else
            return STATUS_NOT_IMPLEMENTED;
#endif //POOL_TAGGING

            break;

        case SystemVdmInstemulInformation:
#ifdef i386
            if (SystemInformationLength < sizeof( SYSTEM_VDM_INSTEMUL_INFO )) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            Status = ExpGetInstemulInformation(
                                            (PSYSTEM_VDM_INSTEMUL_INFO)SystemInformation
                                            );

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof(SYSTEM_VDM_INSTEMUL_INFO);
            }
#else
            Status = STATUS_NOT_IMPLEMENTED;
#endif
            break;

        case SystemCrashDumpInformation:

            if (SystemInformationLength < sizeof( SYSTEM_CRASH_DUMP_INFORMATION)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            // Only allow callers that have create page file privilege
            // to access crash dump information
            //

            if (!SeSinglePrivilegeCheck(SeCreatePagefilePrivilege,PreviousMode)) {
                return STATUS_ACCESS_DENIED;
            }

            Status = MmGetCrashDumpInformation (
                           (PSYSTEM_CRASH_DUMP_INFORMATION)SystemInformation);


            if ( NT_SUCCESS( Status ) ) {
                Status = IoGetCrashDumpInformation (
                            (PSYSTEM_CRASH_DUMP_INFORMATION)SystemInformation);
            }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof(SYSTEM_CRASH_DUMP_INFORMATION);
            }

            break;

            //
            // Get system exception information which includes the number
            // of exceptions that have dispatched, the number of alignment
            // fixups, and the number of floating emulations that have been
            // performed.
            //

        case SystemExceptionInformation:
            if (SystemInformationLength < sizeof( SYSTEM_EXCEPTION_INFORMATION)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof(SYSTEM_EXCEPTION_INFORMATION);
            }

            ExceptionInformation = (PSYSTEM_EXCEPTION_INFORMATION)SystemInformation;

            //
            // Ke information.
            //
            // These counters are kept on a per processor basis and must
            // be totaled.
            //

            {
                ULONG AlignmentFixupCount = 0;
                ULONG ExceptionDispatchCount = 0;
                ULONG FloatingEmulationCount = 0;
                ULONG ByteWordEmulationCount = 0;

                for (i = 0; i < (ULONG)KeNumberProcessors; i += 1) {
                    Prcb = KiProcessorBlock[i];
                    if (Prcb != NULL) {
                        AlignmentFixupCount += Prcb->KeAlignmentFixupCount;
                        ExceptionDispatchCount += Prcb->KeExceptionDispatchCount;
                        FloatingEmulationCount += Prcb->KeFloatingEmulationCount;
#if defined(_ALPHA_)
                        AlignmentFixupCount +=
                            (ULONG)Prcb->Pcr->PalAlignmentFixupCount;

                        ByteWordEmulationCount += Prcb->KeByteWordEmulationCount;
#endif // defined(_ALPHA_)
                    }
                }

                ExceptionInformation->AlignmentFixupCount = AlignmentFixupCount;
                ExceptionInformation->ExceptionDispatchCount = ExceptionDispatchCount;
                ExceptionInformation->FloatingEmulationCount = FloatingEmulationCount;
                ExceptionInformation->ByteWordEmulationCount = ByteWordEmulationCount;
            }

            break;

        case SystemCrashDumpStateInformation:

            if (SystemInformationLength < sizeof( SYSTEM_CRASH_STATE_INFORMATION)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            Status = MmGetCrashDumpStateInformation (
                           (PSYSTEM_CRASH_STATE_INFORMATION)SystemInformation);

            if ( NT_SUCCESS( Status ) ) {
                if (SystemInformationLength >= sizeof (SYSTEM_CRASH_STATE_INFORMATION) ) {
                    Status = IoGetCrashDumpStateInformation (
                                   (PSYSTEM_CRASH_STATE_INFORMATION)SystemInformation);
                }
            }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof(SYSTEM_CRASH_STATE_INFORMATION);
            }

            break;

        case SystemKernelDebuggerInformation:

            if (SystemInformationLength < sizeof( SYSTEM_KERNEL_DEBUGGER_INFORMATION)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            KernelDebuggerInformation =
                (PSYSTEM_KERNEL_DEBUGGER_INFORMATION)SystemInformation;
            KernelDebuggerInformation->KernelDebuggerEnabled = KdDebuggerEnabled;
            KernelDebuggerInformation->KernelDebuggerNotPresent = KdDebuggerNotPresent;

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION);
            }

            break;

        case SystemContextSwitchInformation:

            if (SystemInformationLength < sizeof( SYSTEM_CONTEXT_SWITCH_INFORMATION)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            ContextSwitchInformation =
                (PSYSTEM_CONTEXT_SWITCH_INFORMATION)SystemInformation;

            //
            // Compute the total number of context switches and fill in the
            // remainder of the context switch information.
            //

            ContextSwitches = 0;
            for (i = 0; i < (ULONG)KeNumberProcessors; i += 1) {
                Prcb = KiProcessorBlock[i];
                if (Prcb != NULL) {
                    ContextSwitches += Prcb->KeContextSwitches;
                }

            }

            ContextSwitchInformation->ContextSwitches = ContextSwitches;
            ContextSwitchInformation->FindAny = KeThreadSwitchCounters.FindAny;
            ContextSwitchInformation->FindLast = KeThreadSwitchCounters.FindLast;
            ContextSwitchInformation->FindIdeal = KeThreadSwitchCounters.FindIdeal;
            ContextSwitchInformation->IdleAny = KeThreadSwitchCounters.IdleAny;
            ContextSwitchInformation->IdleCurrent = KeThreadSwitchCounters.IdleCurrent;
            ContextSwitchInformation->IdleLast = KeThreadSwitchCounters.IdleLast;
            ContextSwitchInformation->IdleIdeal = KeThreadSwitchCounters.IdleIdeal;
            ContextSwitchInformation->PreemptAny = KeThreadSwitchCounters.PreemptAny;
            ContextSwitchInformation->PreemptCurrent = KeThreadSwitchCounters.PreemptCurrent;
            ContextSwitchInformation->PreemptLast = KeThreadSwitchCounters.PreemptLast;
            ContextSwitchInformation->SwitchToIdle = KeThreadSwitchCounters.SwitchToIdle;

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof(SYSTEM_CONTEXT_SWITCH_INFORMATION);
            }

            break;

        case SystemRegistryQuotaInformation:

            if (SystemInformationLength < sizeof( SYSTEM_REGISTRY_QUOTA_INFORMATION)) {
                return(STATUS_INFO_LENGTH_MISMATCH);
            }
            CmQueryRegistryQuotaInformation((PSYSTEM_REGISTRY_QUOTA_INFORMATION)SystemInformation);

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof(SYSTEM_REGISTRY_QUOTA_INFORMATION);
            }
            break;

        case SystemDpcBehaviorInformation:
            {
                PSYSTEM_DPC_BEHAVIOR_INFORMATION DpcInfo;
                //
                // If the system information buffer is not the correct length,
                // then return an error.
                //
                if (SystemInformationLength != sizeof(SYSTEM_DPC_BEHAVIOR_INFORMATION)) {
                    return STATUS_INFO_LENGTH_MISMATCH;
                }

                DpcInfo = (PSYSTEM_DPC_BEHAVIOR_INFORMATION)SystemInformation;

                //
                // Exception handler for this routine will return the correct
                // error if any of these accesses fail.
                //
                //
                // Return the current DPC behavior variables
                //
                DpcInfo->DpcQueueDepth = KiMaximumDpcQueueDepth;
                DpcInfo->MinimumDpcRate = KiMinimumDpcRate;
                DpcInfo->AdjustDpcThreshold = KiAdjustDpcThreshold;
                DpcInfo->IdealDpcRate = KiIdealDpcRate;
            }
            break;

        case SystemInterruptInformation:

            if (SystemInformationLength < (sizeof(SYSTEM_INTERRUPT_INFORMATION) * KeNumberProcessors)) {
                return(STATUS_INFO_LENGTH_MISMATCH);
            }

            InterruptInformation = (PSYSTEM_INTERRUPT_INFORMATION)SystemInformation;
            for (i=0; i < (ULONG)KeNumberProcessors; i++) {
                Prcb = KiProcessorBlock[i];
                InterruptInformation->ContextSwitches = Prcb->KeContextSwitches;
                InterruptInformation->DpcCount = Prcb->DpcCount;
                InterruptInformation->DpcRate = Prcb->DpcRequestRate;
                InterruptInformation->TimeIncrement = KeTimeIncrement;
                InterruptInformation->DpcBypassCount = Prcb->DpcBypassCount;
                InterruptInformation->ApcBypassCount = Prcb->ApcBypassCount;

                ++InterruptInformation;
            }

            break;

        case SystemCurrentTimeZoneInformation:
            if (SystemInformationLength < sizeof( RTL_TIME_ZONE_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            RtlCopyMemory(SystemInformation,&ExpTimeZoneInformation,sizeof(ExpTimeZoneInformation));
            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof( RTL_TIME_ZONE_INFORMATION );
                }

            Status = STATUS_SUCCESS;
            break;

            //
            // Query pool lookaside list and general lookaside list
            // information.
            //

        case SystemLookasideInformation:
            Status = ExpGetLookasideInformation(SystemInformation,
                                                SystemInformationLength,
                                                &Length);

            if (ARGUMENT_PRESENT(ReturnLength)) {
                *ReturnLength = Length;
            }

            break;

        case SystemRangeStartInformation:

            if ( SystemInformationLength != sizeof(ULONG_PTR) ) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            *(PULONG_PTR)SystemInformation = (ULONG_PTR)MmSystemRangeStart;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(ULONG_PTR);
            }

            break;

        case SystemVerifierInformation:

            if (SystemInformationLength < sizeof( SYSTEM_VERIFIER_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            Status = MmGetVerifierInformation( SystemInformation,
                                               SystemInformationLength,
                                               &Length
                                              );

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }
            break;

        case SystemLegacyDriverInformation:
            if (SystemInformationLength < sizeof(SYSTEM_LEGACY_DRIVER_INFORMATION)) {
                return(STATUS_INFO_LENGTH_MISMATCH);
            }
            Length = SystemInformationLength;
            Status = ExpQueryLegacyDriverInformation((PSYSTEM_LEGACY_DRIVER_INFORMATION)SystemInformation, &Length);
            if (ARGUMENT_PRESENT(ReturnLength)) {
                *ReturnLength = Length;
            }
            break;

        default:

            //
            // Invalid argument.
            //

            return STATUS_INVALID_INFO_CLASS;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        if (ReleaseModuleResoure) {
            ExReleaseResource (&PsLoadedModuleResource);
            KeLeaveCriticalRegion();
        }

        Status = GetExceptionCode();
    }

    return Status;
}

NTSTATUS
NTAPI
NtSetSystemInformation (
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength
    )

/*++

Routine Description:

    This function set information about the system.

Arguments:

    SystemInformationClass - The system information class which is to
        be modified.

    SystemInformation - A pointer to a buffer which contains the specified
        information. The format and content of the buffer depend on the
        specified system information class.


    SystemInformationLength - Specifies the length in bytes of the system
        information buffer.

Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - Normal, successful completion.

        STATUS_ACCESS_VIOLATION - The specified system information buffer
            is not accessible.

        STATUS_INVALID_INFO_CLASS - The SystemInformationClass parameter
            did not specify a valid value.

        STATUS_INFO_LENGTH_MISMATCH - The value of the SystemInformationLength
            parameter did not match the length required for the information
            class requested by the SystemInformationClass parameter.

        STATUS_PRIVILEGE_NOT_HELD is returned if the caller does not have the
            privilege to set the system time.

--*/

{

    BOOLEAN Enable;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    ULONG TimeAdjustment;
    PSYSTEM_SET_TIME_ADJUST_INFORMATION TimeAdjustmentInformation;
    HANDLE EventHandle;
    PVOID Event;

    PAGED_CODE();

    //
    // Establish an exception handle in case the system information buffer
    // is not accessible.
    //

    Status = STATUS_SUCCESS;

    try {

        //
        // Get the previous processor mode and probe the input buffer for
        // read access if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForRead((PVOID)SystemInformation,
                         SystemInformationLength,
                         sizeof(ULONG));
        }

        //
        // Dispatch on the system information class.
        //

        switch (SystemInformationClass) {
        case SystemFlagsInformation:
            if (SystemInformationLength != sizeof( SYSTEM_FLAGS_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            if (!SeSinglePrivilegeCheck( SeDebugPrivilege, PreviousMode )) {
                return STATUS_ACCESS_DENIED;
                }
            else {
                NtGlobalFlag = ((PSYSTEM_FLAGS_INFORMATION)SystemInformation)->Flags & FLG_KERNELMODE_VALID_BITS;
                ((PSYSTEM_FLAGS_INFORMATION)SystemInformation)->Flags = NtGlobalFlag;
                }
            break;

            //
            // Set system time adjustment information.
            //
            // N.B. The caller must have the SeSystemTime privilege.
            //

        case SystemTimeAdjustmentInformation:

            //
            // If the system information buffer is not the correct length,
            // then return an error.
            //

            if (SystemInformationLength != sizeof( SYSTEM_SET_TIME_ADJUST_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            //
            // If the current thread does not have the privilege to set the
            // time adjustment variables, then return an error.
            //

            if ((PreviousMode != KernelMode) &&
                (SeSinglePrivilegeCheck(SeSystemtimePrivilege, PreviousMode) == FALSE)) {
                return STATUS_PRIVILEGE_NOT_HELD;
            }

            //
            // Set system time adjustment parameters.
            //

            TimeAdjustmentInformation =
                    (PSYSTEM_SET_TIME_ADJUST_INFORMATION)SystemInformation;

            Enable = TimeAdjustmentInformation->Enable;
            TimeAdjustment = TimeAdjustmentInformation->TimeAdjustment;

            if (Enable == TRUE) {
                KeTimeAdjustment = KeMaximumIncrement;
            } else {
                if (TimeAdjustment == 0) {
                    return STATUS_INVALID_PARAMETER_2;
                }
                KeTimeAdjustment = TimeAdjustment;
            }

            KeTimeSynchronization = Enable;
            break;

            //
            // Set an event to signal when the clock interrupt has been
            // masked for too long, causing the time to slip.
            // The event will be referenced to prevent it from being
            // deleted.  If the new event handle is valid or NULL, the
            // old event will be dereferenced and forgotten.  If the
            // event handle is non-NULL but invalid, the old event will
            // be remembered and a failure status will be returned.
            //
            // N.B. The caller must have the SeSystemTime privilege.
            //
        case SystemTimeSlipNotification:

            if (SystemInformationLength != sizeof(HANDLE)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            //
            // If the current thread does not have the privilege to set the
            // time adjustment variables, then return an error.
            //

            if ((PreviousMode != KernelMode) &&
                (SeSinglePrivilegeCheck(SeSystemtimePrivilege, PreviousMode) == FALSE)) {
                return STATUS_PRIVILEGE_NOT_HELD;
            }

            EventHandle = *(PHANDLE)SystemInformation;

            if (EventHandle == NULL) {

                //
                // Dereference the old event and don't signal anything
                // for time slips.
                //

                Event = NULL;
                Status = STATUS_SUCCESS;

            } else {

                Status = ObReferenceObjectByHandle(EventHandle,
                                                   EVENT_MODIFY_STATE,
                                                   ExEventObjectType,
                                                   PreviousMode,
                                                   &Event,
                                                   NULL);
            }

            if (NT_SUCCESS(Status)) {
                KdUpdateTimeSlipEvent(Event);
            }

            break;

            //
            // Set registry quota limit.
            //
            // N.B. The caller must have SeIncreaseQuotaPrivilege
            //
        case SystemRegistryQuotaInformation:

            //
            // If the system information buffer is not the correct length,
            // then return an error.
            //

            if (SystemInformationLength != sizeof( SYSTEM_REGISTRY_QUOTA_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            //
            // If the current thread does not have the privilege to create
            // a pagefile, then return an error.
            //

            if ((PreviousMode != KernelMode) &&
                (SeSinglePrivilegeCheck(SeIncreaseQuotaPrivilege, PreviousMode) == FALSE)) {
                return STATUS_PRIVILEGE_NOT_HELD;
            }

            //
            // Set registry quota parameters.
            //
            CmSetRegistryQuotaInformation((PSYSTEM_REGISTRY_QUOTA_INFORMATION)SystemInformation);

            break;

        case SystemPrioritySeperation:
            {
                ULONG PrioritySeparation;

                //
                // If the system information buffer is not the correct length,
                // then return an error.
                //

                if (SystemInformationLength != sizeof( ULONG )) {
                    return STATUS_INFO_LENGTH_MISMATCH;
                    }

                try {
                    PrioritySeparation = *(PULONG)SystemInformation;
                    }
                except(EXCEPTION_EXECUTE_HANDLER) {
                    return GetExceptionCode();
                    }

                PsChangeQuantumTable(TRUE,PrioritySeparation);
                Status = STATUS_SUCCESS;
            }
            break;

        case SystemExtendServiceTableInformation:
            {

                UNICODE_STRING Image;
                PWSTR  Buffer;
                PVOID ImageBaseAddress;
                ULONG_PTR EntryPoint;
                PVOID SectionPointer;
                PIMAGE_NT_HEADERS NtHeaders;
                PDRIVER_INITIALIZE InitRoutine;
                DRIVER_OBJECT Win32KDevice;

                //
                // If the system information buffer is not the correct length,
                // then return an error.
                //

                if (SystemInformationLength != sizeof( UNICODE_STRING ) ) {
                    return STATUS_INFO_LENGTH_MISMATCH;
                }

                if (PreviousMode != KernelMode) {

                    //
                    // The caller's access mode is not kernel so check to ensure that
                    // the caller has the privilege to load a driver.
                    //

                    if (!SeSinglePrivilegeCheck( SeLoadDriverPrivilege, PreviousMode )) {
                        return STATUS_PRIVILEGE_NOT_HELD;
                    }

                    try {
                        UNICODE_STRING tImage;
                        USHORT maxLength;

                        Buffer = NULL;
                        tImage = *(PUNICODE_STRING)SystemInformation;

                        //
                        // Leave room for the NUL, if possible.
                        // Guard against overflow.
                        //
                        maxLength = tImage.Length + sizeof(UNICODE_NULL);
                        if (maxLength < tImage.Length || maxLength > tImage.MaximumLength) {
                            maxLength = tImage.Length;
                        }

                        ProbeForRead(tImage.Buffer, maxLength, sizeof(UCHAR));

                        Buffer = ExAllocatePoolWithTag(PagedPool, maxLength, 'ofnI');
                        if ( !Buffer ) {
                            return STATUS_NO_MEMORY;
                        }

                        RtlCopyMemory(Buffer, tImage.Buffer, tImage.Length);
                        Image.Buffer = Buffer;
                        Image.Length = tImage.Length;
                        Image.MaximumLength = maxLength;
                    }
                    except(EXCEPTION_EXECUTE_HANDLER) {
                        if ( Buffer ) {
                            ExFreePool(Buffer);
                        }
                        return GetExceptionCode();
                    }

                    //
                    // Call MmLoadSystemImage with previous mode of kernel.
                    //

                    Status = ZwSetSystemInformation(
                                SystemExtendServiceTableInformation,
                                (PVOID)&Image,
                                sizeof(Image)
                                );

                    ExFreePool(Buffer);

                    return Status;

                }

                Image = *(PUNICODE_STRING)SystemInformation;

                //
                // Now in kernelmode, so load the driver.
                //

                Status = MmLoadSystemImage (&Image,
                                            NULL,
                                            NULL,
                                            TRUE,
                                            &SectionPointer,
                                            (PVOID *) &ImageBaseAddress);
                
                if (!NT_SUCCESS (Status)) {
                    return Status;
                }

                NtHeaders = RtlImageNtHeader( ImageBaseAddress );
                EntryPoint = NtHeaders->OptionalHeader.AddressOfEntryPoint;
                EntryPoint += (ULONG_PTR) ImageBaseAddress;
                InitRoutine = (PDRIVER_INITIALIZE) EntryPoint;

                RtlZeroMemory (&Win32KDevice, sizeof(Win32KDevice));
                ASSERT (KeGetCurrentIrql() == 0);

                Status = (InitRoutine)(&Win32KDevice,NULL);

                ASSERT (KeGetCurrentIrql() == 0);

                if (!NT_SUCCESS (Status)) {
                    MmUnloadSystemImage (SectionPointer);
                }
                else {

                    //
                    // Pass the driver object to memory management so the
                    // session can be unloaded cleanly.
                    //

                    MmSessionSetUnloadAddress (&Win32KDevice);
                }
            }
            break;


        case SystemUnloadGdiDriverInformation:
            {

                if (SystemInformationLength != sizeof( PVOID ) ) {
                    return STATUS_INFO_LENGTH_MISMATCH;
                    }

                if (PreviousMode != KernelMode) {

                    //
                    // The caller's access mode is not kernel so fail.
                    // Only GDI from the kernel can call this.
                    //

                    return STATUS_PRIVILEGE_NOT_HELD;

                    }

                MmUnloadSystemImage( *((PVOID *)SystemInformation) );

                Status = STATUS_SUCCESS;

                }
                break;


        case SystemLoadGdiDriverInformation:
            {

                UNICODE_STRING Image;
                PVOID ImageBaseAddress;
                ULONG_PTR EntryPoint;
                PVOID SectionPointer;

                PIMAGE_NT_HEADERS NtHeaders;

                //
                // If the system information buffer is not the correct length,
                // then return an error.
                //

                if (SystemInformationLength != sizeof( SYSTEM_GDI_DRIVER_INFORMATION ) ) {
                    return STATUS_INFO_LENGTH_MISMATCH;
                }

                if (PreviousMode != KernelMode) {

                    //
                    // The caller's access mode is not kernel so fail.
                    // Only GDI from the kernel can call this.
                    //

                    return STATUS_PRIVILEGE_NOT_HELD;
                }

                Image = ((PSYSTEM_GDI_DRIVER_INFORMATION)SystemInformation)->DriverName;
                Status = MmLoadSystemImage (&Image,
                                            NULL,
                                            NULL,
                                            TRUE,
                                            &SectionPointer,
                                            (PVOID *) &ImageBaseAddress);

                //
                // Some drivers (like dxapi.sys) may be already loaded by a
                // minidriver that links to it - allow these to re-succeed.
                //

                if ((NT_SUCCESS( Status )) ||
                    (Status == STATUS_IMAGE_ALREADY_LOADED)) {

                    PSYSTEM_GDI_DRIVER_INFORMATION GdiDriverInfo =
                        (PSYSTEM_GDI_DRIVER_INFORMATION) SystemInformation;

                    ULONG Size;
                    PVOID BaseAddress;

                    GdiDriverInfo->ExportSectionPointer =
                        RtlImageDirectoryEntryToData(ImageBaseAddress,
                                                     TRUE,
                                                     IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                     &Size);

                    //
                    // Capture the entry point.
                    //

                    NtHeaders = RtlImageNtHeader( ImageBaseAddress );
                    EntryPoint = NtHeaders->OptionalHeader.AddressOfEntryPoint;
                    EntryPoint += (ULONG_PTR) ImageBaseAddress;

                    GdiDriverInfo->ImageAddress = (PVOID) ImageBaseAddress;
                    GdiDriverInfo->SectionPointer = SectionPointer;
                    GdiDriverInfo->EntryPoint = (PVOID) EntryPoint;

                    //
                    // GDI drivers are always completely pagable.
                    //

                    if (NT_SUCCESS( Status )) {
                        BaseAddress = MmPageEntireDriver((PVOID)ImageBaseAddress);
                        ASSERT(BaseAddress == ImageBaseAddress);
                    }
                }
            }
            break;

        case SystemFileCacheInformation:

            if (SystemInformationLength < sizeof( SYSTEM_FILECACHE_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            if (!SeSinglePrivilegeCheck( SeIncreaseQuotaPrivilege, PreviousMode )) {
                return STATUS_ACCESS_DENIED;
            }

            return MmAdjustWorkingSetSize (
                        ((PSYSTEM_FILECACHE_INFORMATION)SystemInformation)->MinimumWorkingSet,
                        ((PSYSTEM_FILECACHE_INFORMATION)SystemInformation)->MaximumWorkingSet,
                        TRUE);

            break;

        case SystemDpcBehaviorInformation:
            {
                SYSTEM_DPC_BEHAVIOR_INFORMATION DpcInfo;
                //
                // If the system information buffer is not the correct length,
                // then return an error.
                //
                if (SystemInformationLength != sizeof(SYSTEM_DPC_BEHAVIOR_INFORMATION)) {
                    return STATUS_INFO_LENGTH_MISMATCH;
                }

                if (PreviousMode != KernelMode) {
                    //
                    // The caller's access mode is not kernel so check to ensure that
                    // the caller has the privilege to load a driver.
                    //

                    if (!SeSinglePrivilegeCheck( SeLoadDriverPrivilege, PreviousMode )) {
                        return STATUS_PRIVILEGE_NOT_HELD;
                    }
                }

                //
                // Exception handler for this routine will return the correct
                // error if this access fails.
                //
                DpcInfo = *(PSYSTEM_DPC_BEHAVIOR_INFORMATION)SystemInformation;

                //
                // Set the new DPC behavior variables
                //
                KiMaximumDpcQueueDepth = DpcInfo.DpcQueueDepth;
                KiMinimumDpcRate = DpcInfo.MinimumDpcRate;
                KiAdjustDpcThreshold = DpcInfo.AdjustDpcThreshold;
                KiIdealDpcRate = DpcInfo.IdealDpcRate;
            }
            break;

        case SystemSessionCreate:
            {

                //
                // Creation of a session space.
                //

                ULONG SessionId;

                //
                // If the system information buffer is not the correct length,
                // then return an error.
                //

                if (SystemInformationLength != sizeof(ULONG)) {
                    return STATUS_INFO_LENGTH_MISMATCH;
                }

                if (PreviousMode != KernelMode) {

                    //
                    // The caller's access mode is not kernel so check to
                    // ensure that the caller has the privilege to load
                    // a driver.
                    //

                    if (!SeSinglePrivilegeCheck (SeLoadDriverPrivilege, PreviousMode)) {
                        return STATUS_PRIVILEGE_NOT_HELD;
                    }

                    try {
                        ProbeForWriteUlong((PULONG)SystemInformation);
                    }
                    except (EXCEPTION_EXECUTE_HANDLER) {
                        return GetExceptionCode();
                    }
                }

                //
                // Create a session space in the current process.
                //

                Status = MmSessionCreate (&SessionId);

                if (NT_SUCCESS(Status)) {
                    if (PreviousMode != KernelMode) {
                        try {
                            *(PULONG)SystemInformation = SessionId;
                        }
                        except (EXCEPTION_EXECUTE_HANDLER) {
                            return GetExceptionCode();
                        }
                    }
                    else {
                        *(PULONG)SystemInformation = SessionId;
                    }
                }

                return Status;
            }
            break;

        case SystemSessionDetach:
            {
                ULONG SessionId;

                //
                // If the system information buffer is not the correct length,
                // then return an error.
                //

                if (SystemInformationLength != sizeof(ULONG)) {
                    return STATUS_INFO_LENGTH_MISMATCH;
                }

                if (PreviousMode != KernelMode) {

                    //
                    // The caller's access mode is not kernel so check to
                    // ensure that the caller has the privilege to load
                    // a driver.
                    //

                    if (!SeSinglePrivilegeCheck( SeLoadDriverPrivilege, PreviousMode )) {
                        return STATUS_PRIVILEGE_NOT_HELD;
                    }

                    try {
                        ProbeForRead ((PVOID)SystemInformation,
                                      sizeof(ULONG),
                                      sizeof(ULONG));

                        SessionId = *(PULONG)SystemInformation;
                    }
                    except (EXCEPTION_EXECUTE_HANDLER) {
                        return GetExceptionCode();
                    }
                }
                else {
                    SessionId = *(PULONG)SystemInformation;
                }

                //
                // Detach the current process from a session space
                // if it has one.
                //

                Status = MmSessionDelete (SessionId);

                return Status;
            }
            break;

        case SystemCrashDumpStateInformation:


            if (SystemInformationLength < sizeof( SYSTEM_CRASH_STATE_INFORMATION)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            if (!SeSinglePrivilegeCheck( SeCreatePagefilePrivilege, PreviousMode )) {
                return STATUS_ACCESS_DENIED;
            }

            Status = IoSetCrashDumpState( (SYSTEM_CRASH_STATE_INFORMATION *)SystemInformation);

            break;

        case SystemPerformanceTraceInformation:
#ifdef NTPERF
            Status = PerformanceTraceInformation(SystemInformationClass,
                                                 SystemInformation,
                                                 SystemInformationLength
                                                 );
#else
            Status = STATUS_INVALID_INFO_CLASS;
#endif
            break;
            
        case SystemVerifierThunkExtend:

            if (PreviousMode != KernelMode) {

                //
                // The caller's access mode is not kernel so fail.
                // Only device drivers can call this.
                //

                return STATUS_PRIVILEGE_NOT_HELD;
            }

            Status = MmAddVerifierThunks (SystemInformation,
                                          SystemInformationLength);

            break;

        case SystemVerifierInformation:

            if (!SeSinglePrivilegeCheck (SeDebugPrivilege, PreviousMode)) {
                return STATUS_ACCESS_DENIED;
            }

            Status = MmSetVerifierInformation (SystemInformation,
                                               SystemInformationLength);

            break;

        default:
            //KeBugCheckEx(SystemInformationClass,KdPitchDebugger,0,0,0);
            Status = STATUS_INVALID_INFO_CLASS;
            break;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    return Status;
}

PVOID
ExLockUserBuffer(
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PVOID *LockVariable
    )

{
    PMDL Mdl;
    PVOID Address;
    SIZE_T MdlSize;

    //
    // Allocate an MDL to map the request.
    //

    MdlSize = MmSizeOfMdl( Buffer, Length );
    Mdl = ExAllocatePoolWithQuotaTag (NonPagedPool,
                                      MdlSize,
                                      'ofnI');
    if (Mdl == NULL) {
        return NULL;
    }

    //
    // Initialize MDL for request.
    //

    MmInitializeMdl(Mdl, Buffer, Length);

    try {

        MmProbeAndLockPages (Mdl, KeGetPreviousMode(), IoWriteAccess);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        ExFreePool (Mdl);

        return( NULL );
    }

    Mdl->MdlFlags |= MDL_MAPPING_CAN_FAIL;
    Address =  MmGetSystemAddressForMdl (Mdl);
    *LockVariable = Mdl;
    if (Address == NULL) {
        ExUnlockUserBuffer (Mdl);
        *LockVariable = NULL;
    }

    return Address;
}


VOID
ExUnlockUserBuffer(
    IN PVOID LockVariable
    )

{
    MmUnlockPages ((PMDL)LockVariable);
    ExFreePool ((PMDL)LockVariable);
    return;
}

extern FAST_MUTEX PspActiveProcessMutex;
NTSTATUS
ExpGetProcessInformation (
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length,
    IN PULONG SessionId OPTIONAL
    )
/*++

Routine Description:

    This function returns information about all the processes and
    threads in the system.

Arguments:

    SystemInformation - A pointer to a buffer which receives the specified
        information.

    SystemInformationLength - Specifies the length in bytes of the system
        information buffer.

    Length - An optional pointer which, if specified, receives the
        number of bytes placed in the system information buffer.


Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_INVALID_INFO_CLASS - The SystemInformationClass parameter
            did not specify a valid value.

        STATUS_INFO_LENGTH_MISMATCH - The value of the SystemInformationLength
            parameter did not match the length required for the information
            class requested by the SystemInformationClass parameter.

        STATUS_ACCESS_VIOLATION - Either the SystemInformation buffer pointer
            or the Length pointer value specified an invalid address.

        STATUS_WORKING_SET_QUOTA - The process does not have sufficient
            working set to lock the specified output structure in memory.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
            for this request to complete.

--*/

{
    KEVENT Event;
    PEPROCESS Process;
    PETHREAD Thread;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    PLIST_ENTRY NextProcess;
    PLIST_ENTRY NextThread;
    PVOID MappedAddress;
    PVOID LockVariable;
    ULONG TotalSize = 0;
    ULONG NextEntryOffset = 0;
    PUCHAR Src;
    PWSTR Dst;
    ULONG n;
    NTSTATUS status = STATUS_SUCCESS;

    *Length = 0;

    MappedAddress = ExLockUserBuffer( SystemInformation,
                                      SystemInformationLength,
                                      &LockVariable
                                    );
    if (MappedAddress == NULL) {
        return( STATUS_ACCESS_VIOLATION );
    }
    MmLockPagableSectionByHandle (ExPageLockHandle);
    ExAcquireFastMutex(&PspActiveProcessMutex);

    
    //
    // Initialize an event object and then set the event with the wait
    // parameter TRUE. This causes the event to be set and control is
    // returned with the dispatcher database locked at dispatch IRQL.
    //

    KeInitializeEvent (&Event, NotificationEvent, FALSE);
    try {
      

        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)MappedAddress;
   

        if (!ARGUMENT_PRESENT(SessionId)) {
   
           NextEntryOffset = sizeof(SYSTEM_PROCESS_INFORMATION);

           TotalSize = sizeof(SYSTEM_PROCESS_INFORMATION);
   
           ExpCopyProcessInfo (ProcessInfo, PsIdleProcess);
   
           //
           // Since Idle process and system process share the same
           // object table, zero out idle processes handle count to
           // reduce confusion
           //
   
           ProcessInfo->HandleCount = 0;
   
           // Idle Process always has SessionId 0
           ProcessInfo->SessionId = 0;
           //
           // Set the event with the wait
           // parameter TRUE. This causes the event to be set and control is
           // returned with the dispatcher database locked at dispatch IRQL.
           //
           // WARNING - The following code assumes that the process structure
           //      uses kernel objects to synchronize access to the thread and
           //      process lists.
           //
   
           KeSetEvent (&Event, 0, TRUE);
   
           //
           // WARNING - The following code runs with the kernel dispatch database
           //      locked. EXTREME caution should be taken when modifying this
           //      code. Extended execution will ADVERSELY affect system operation
           //      and integrity.
           //
           // Get info for idle process's threads
           //
           //
           // Get information for each thread.
           //
   
           ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
           ProcessInfo->NumberOfThreads = 0;
           NextThread = PsIdleProcess->Pcb.ThreadListHead.Flink;
           while (NextThread != &PsIdleProcess->Pcb.ThreadListHead) {
               NextEntryOffset += sizeof(SYSTEM_THREAD_INFORMATION);
               TotalSize += sizeof(SYSTEM_THREAD_INFORMATION);
   
               if (TotalSize > SystemInformationLength) {
                   status = STATUS_INFO_LENGTH_MISMATCH;
                   KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL);
                   goto Failed;
               }
               Thread = (PETHREAD)(CONTAINING_RECORD(NextThread,
                                                     KTHREAD,
                                                     ThreadListEntry));
               ExpCopyThreadInfo (ThreadInfo,Thread);
   
               ProcessInfo->NumberOfThreads += 1;
               NextThread = NextThread->Flink;
               ThreadInfo += 1;
           }
           
           //
           // Unlock the dispatch database by waiting on the event that was
           // previously set with the wait parameter TRUE.
           //
   
           KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL);

           ProcessInfo->ImageName.Buffer = NULL;
           ProcessInfo->ImageName.Length = 0;
           ProcessInfo->NextEntryOffset = NextEntryOffset;
        }

        NextProcess = PsActiveProcessHead.Flink;

        while (NextProcess != &PsActiveProcessHead) {
            Process = CONTAINING_RECORD(NextProcess,
                                        EPROCESS,
                                        ActiveProcessLinks);

            if (ARGUMENT_PRESENT(SessionId) && (Process->SessionId != *SessionId)) {
               NextProcess = NextProcess->Flink;
               continue;
            }

            ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
                            ((PUCHAR)MappedAddress + TotalSize);

            NextEntryOffset = sizeof(SYSTEM_PROCESS_INFORMATION);
            TotalSize += sizeof(SYSTEM_PROCESS_INFORMATION);
            if (TotalSize > SystemInformationLength) {
                status = STATUS_INFO_LENGTH_MISMATCH;
                goto Failed;
            }

            //
            // Get information for each process.
            //

            ExpCopyProcessInfo (ProcessInfo, Process);


            //
            // Set the event with the wait
            // parameter TRUE. This causes the event to be set and control is
            // returned with the dispatcher database locked at dispatch IRQL.
            //
            // WARNING - The following code assumes that the process structure
            //      uses kernel objects to synchronize access to the thread and
            //      process lists.
            //
   
            KeSetEvent (&Event, 0, TRUE);
   
            //
            // WARNING - The following code runs with the kernel dispatch database
            //      locked. EXTREME caution should be taken when modifying this
            //      code. Extended execution will ADVERSELY affect system operation
            //      and integrity.
            //
   
            //
            // Get information for each thread.
            //
   
            ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
            ProcessInfo->NumberOfThreads = 0;
            NextThread = Process->Pcb.ThreadListHead.Flink;
            while (NextThread != &Process->Pcb.ThreadListHead) {
                NextEntryOffset += sizeof(SYSTEM_THREAD_INFORMATION);
                TotalSize += sizeof(SYSTEM_THREAD_INFORMATION);
   
                if (TotalSize > SystemInformationLength) {
                    status = STATUS_INFO_LENGTH_MISMATCH;
                    KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL);
                    goto Failed;
                }
                Thread = (PETHREAD)(CONTAINING_RECORD(NextThread,
                                                      KTHREAD,
                                                      ThreadListEntry));
                ExpCopyThreadInfo (ThreadInfo,Thread);
   
                ProcessInfo->NumberOfThreads += 1;
                NextThread = NextThread->Flink;
                ThreadInfo += 1;
            }
   
            //
            // Store the Remote Terminal SessionId
            //
            ProcessInfo->SessionId = Process->SessionId;

   
            //
            // Unlock the dispatch database by waiting on the event that was
            // previously set with the wait parameter TRUE.
            //
   
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL);
            //
            // Get the image name.
            //

            ProcessInfo->ImageName.Buffer = NULL;
            ProcessInfo->ImageName.Length = 0;
            ProcessInfo->ImageName.MaximumLength = 0;

            if ((n = strlen( Src = Process->ImageFileName ))) {
                n = ROUND_UP( ((n + 1) * sizeof( WCHAR )), sizeof(LARGE_INTEGER) );
                TotalSize += n;
                NextEntryOffset += n;
                if (TotalSize > SystemInformationLength) {
                    status = STATUS_INFO_LENGTH_MISMATCH;
                } else {
                    Dst = (PWSTR)(ThreadInfo);
                    while (*Dst++ = (WCHAR)*Src++) {
                        ;
                        }
                    ProcessInfo->ImageName.Length = (USHORT)((PCHAR)Dst - (PCHAR)ThreadInfo - sizeof( UNICODE_NULL ));
                    ProcessInfo->ImageName.MaximumLength = (USHORT)n;

                    //
                    // Set the image name to point into the user's memory.
                    //

                    ProcessInfo->ImageName.Buffer = (PWSTR)
                                ((PCHAR)SystemInformation +
                                 ((PCHAR)(ThreadInfo) - (PCHAR)MappedAddress));
                }

                if (!NT_SUCCESS( status )) {
                    goto Failed;
                }
            }

            //
            // Point to next process.
            //

            ProcessInfo->NextEntryOffset = NextEntryOffset;
            NextProcess = NextProcess->Flink;
        }

        ProcessInfo->NextEntryOffset = 0;
        status = STATUS_SUCCESS;
        *Length = TotalSize;

Failed:
        ;

    } finally {
        ExReleaseFastMutex(&PspActiveProcessMutex);
        MmUnlockPagableImageSection(ExPageLockHandle);
        ExUnlockUserBuffer( LockVariable );
    }

    return(status);
}

VOID
ExpCopyProcessInfo (
    IN PSYSTEM_PROCESS_INFORMATION ProcessInfo,
    IN PEPROCESS Process
    )

{
    PHANDLE_TABLE Ht;

    PAGED_CODE();

    Ht = (PHANDLE_TABLE)Process->ObjectTable;
    if ( Ht ) {
        ProcessInfo->HandleCount = Ht->HandleCount;
        }
    else {
        ProcessInfo->HandleCount = 0;
        }
    ProcessInfo->CreateTime = Process->CreateTime;
    ProcessInfo->UserTime.QuadPart = UInt32x32To64(Process->Pcb.UserTime,
                                                   KeMaximumIncrement);

    ProcessInfo->KernelTime.QuadPart = UInt32x32To64(Process->Pcb.KernelTime,
                                                     KeMaximumIncrement);

    ProcessInfo->BasePriority = Process->Pcb.BasePriority;
    ProcessInfo->UniqueProcessId = Process->UniqueProcessId;
    ProcessInfo->InheritedFromUniqueProcessId = Process->InheritedFromUniqueProcessId;
    ProcessInfo->PeakVirtualSize = Process->PeakVirtualSize;
    ProcessInfo->VirtualSize = Process->VirtualSize;
    ProcessInfo->PageFaultCount = Process->Vm.PageFaultCount;
    ProcessInfo->PeakWorkingSetSize = Process->Vm.PeakWorkingSetSize << PAGE_SHIFT;
    ProcessInfo->WorkingSetSize = Process->Vm.WorkingSetSize << PAGE_SHIFT;
    ProcessInfo->QuotaPeakPagedPoolUsage =
                            Process->QuotaPeakPoolUsage[PagedPool];
    ProcessInfo->QuotaPagedPoolUsage = Process->QuotaPoolUsage[PagedPool];
    ProcessInfo->QuotaPeakNonPagedPoolUsage =
                            Process->QuotaPeakPoolUsage[NonPagedPool];
    ProcessInfo->QuotaNonPagedPoolUsage =
                            Process->QuotaPoolUsage[NonPagedPool];
    ProcessInfo->PagefileUsage = Process->PagefileUsage << PAGE_SHIFT;
    ProcessInfo->PeakPagefileUsage = Process->PeakPagefileUsage << PAGE_SHIFT;
    ProcessInfo->PrivatePageCount = Process->CommitCharge << PAGE_SHIFT;

    ProcessInfo->ReadOperationCount = Process->ReadOperationCount;
    ProcessInfo->WriteOperationCount = Process->WriteOperationCount;
    ProcessInfo->OtherOperationCount = Process->OtherOperationCount;
    ProcessInfo->ReadTransferCount = Process->ReadTransferCount;
    ProcessInfo->WriteTransferCount = Process->WriteTransferCount;
    ProcessInfo->OtherTransferCount = Process->OtherTransferCount;
}

VOID
ExpCopyThreadInfo (
    IN PSYSTEM_THREAD_INFORMATION ThreadInfo,
    IN PETHREAD Thread
    )

{

    ThreadInfo->KernelTime.QuadPart = UInt32x32To64(Thread->Tcb.KernelTime,
                                                    KeMaximumIncrement);

    ThreadInfo->UserTime.QuadPart = UInt32x32To64(Thread->Tcb.UserTime,
                                                  KeMaximumIncrement);

    ThreadInfo->CreateTime.QuadPart = PS_GET_THREAD_CREATE_TIME (Thread);
    ThreadInfo->WaitTime = Thread->Tcb.WaitTime;
    ThreadInfo->ClientId = Thread->Cid;
    ThreadInfo->ThreadState = Thread->Tcb.State;
    ThreadInfo->WaitReason = Thread->Tcb.WaitReason;
    ThreadInfo->Priority = Thread->Tcb.Priority;
    ThreadInfo->BasePriority = Thread->Tcb.BasePriority;
    ThreadInfo->ContextSwitches = Thread->Tcb.ContextSwitches;
    ThreadInfo->StartAddress = Thread->StartAddress;
}

#ifdef i386
extern ULONG ExVdmOpcodeDispatchCounts[256];
extern ULONG VdmBopCount;
extern ULONG ExVdmSegmentNotPresent;

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, ExpGetInstemulInformation)
#endif


NTSTATUS
ExpGetInstemulInformation(
    OUT PSYSTEM_VDM_INSTEMUL_INFO Info
    )
{
    SYSTEM_VDM_INSTEMUL_INFO LocalInfo;

    LocalInfo.VdmOpcode0F       = ExVdmOpcodeDispatchCounts[VDM_INDEX_0F];
    LocalInfo.OpcodeESPrefix    = ExVdmOpcodeDispatchCounts[VDM_INDEX_ESPrefix];
    LocalInfo.OpcodeCSPrefix    = ExVdmOpcodeDispatchCounts[VDM_INDEX_CSPrefix];
    LocalInfo.OpcodeSSPrefix    = ExVdmOpcodeDispatchCounts[VDM_INDEX_SSPrefix];
    LocalInfo.OpcodeDSPrefix    = ExVdmOpcodeDispatchCounts[VDM_INDEX_DSPrefix];
    LocalInfo.OpcodeFSPrefix    = ExVdmOpcodeDispatchCounts[VDM_INDEX_FSPrefix];
    LocalInfo.OpcodeGSPrefix    = ExVdmOpcodeDispatchCounts[VDM_INDEX_GSPrefix];
    LocalInfo.OpcodeOPER32Prefix= ExVdmOpcodeDispatchCounts[VDM_INDEX_OPER32Prefix];
    LocalInfo.OpcodeADDR32Prefix= ExVdmOpcodeDispatchCounts[VDM_INDEX_ADDR32Prefix];
    LocalInfo.OpcodeINSB        = ExVdmOpcodeDispatchCounts[VDM_INDEX_INSB];
    LocalInfo.OpcodeINSW        = ExVdmOpcodeDispatchCounts[VDM_INDEX_INSW];
    LocalInfo.OpcodeOUTSB       = ExVdmOpcodeDispatchCounts[VDM_INDEX_OUTSB];
    LocalInfo.OpcodeOUTSW       = ExVdmOpcodeDispatchCounts[VDM_INDEX_OUTSW];
    LocalInfo.OpcodePUSHF       = ExVdmOpcodeDispatchCounts[VDM_INDEX_PUSHF];
    LocalInfo.OpcodePOPF        = ExVdmOpcodeDispatchCounts[VDM_INDEX_POPF];
    LocalInfo.OpcodeINTnn       = ExVdmOpcodeDispatchCounts[VDM_INDEX_INTnn];
    LocalInfo.OpcodeINTO        = ExVdmOpcodeDispatchCounts[VDM_INDEX_INTO];
    LocalInfo.OpcodeIRET        = ExVdmOpcodeDispatchCounts[VDM_INDEX_IRET];
    LocalInfo.OpcodeINBimm      = ExVdmOpcodeDispatchCounts[VDM_INDEX_INBimm];
    LocalInfo.OpcodeINWimm      = ExVdmOpcodeDispatchCounts[VDM_INDEX_INWimm];
    LocalInfo.OpcodeOUTBimm     = ExVdmOpcodeDispatchCounts[VDM_INDEX_OUTBimm];
    LocalInfo.OpcodeOUTWimm     = ExVdmOpcodeDispatchCounts[VDM_INDEX_OUTWimm];
    LocalInfo.OpcodeINB         = ExVdmOpcodeDispatchCounts[VDM_INDEX_INB];
    LocalInfo.OpcodeINW         = ExVdmOpcodeDispatchCounts[VDM_INDEX_INW];
    LocalInfo.OpcodeOUTB        = ExVdmOpcodeDispatchCounts[VDM_INDEX_OUTB];
    LocalInfo.OpcodeOUTW        = ExVdmOpcodeDispatchCounts[VDM_INDEX_OUTW];
    LocalInfo.OpcodeLOCKPrefix  = ExVdmOpcodeDispatchCounts[VDM_INDEX_LOCKPrefix];
    LocalInfo.OpcodeREPNEPrefix = ExVdmOpcodeDispatchCounts[VDM_INDEX_REPNEPrefix];
    LocalInfo.OpcodeREPPrefix   = ExVdmOpcodeDispatchCounts[VDM_INDEX_REPPrefix];
    LocalInfo.OpcodeHLT         = ExVdmOpcodeDispatchCounts[VDM_INDEX_HLT];
    LocalInfo.OpcodeCLI         = ExVdmOpcodeDispatchCounts[VDM_INDEX_CLI];
    LocalInfo.OpcodeSTI         = ExVdmOpcodeDispatchCounts[VDM_INDEX_STI];
    LocalInfo.BopCount          = VdmBopCount;
    LocalInfo.SegmentNotPresent = ExVdmSegmentNotPresent;

    RtlMoveMemory(Info,&LocalInfo,sizeof(LocalInfo));

    return STATUS_SUCCESS;
}
#endif

#if i386 && !FPO
NTSTATUS
ExpGetStackTraceInformation (
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    NTSTATUS Status;
    PRTL_PROCESS_BACKTRACES BackTraceInformation = (PRTL_PROCESS_BACKTRACES)SystemInformation;
    PRTL_PROCESS_BACKTRACE_INFORMATION BackTraceInfo;
    PSTACK_TRACE_DATABASE DataBase;
    PRTL_STACK_TRACE_ENTRY p, *pp;
    ULONG RequiredLength, n;

    DataBase = RtlpAcquireStackTraceDataBase();
    if (DataBase == NULL) {
        return STATUS_UNSUCCESSFUL;
        }
    DataBase->DumpInProgress = TRUE;
    RtlpReleaseStackTraceDataBase();
    try {
        RequiredLength = FIELD_OFFSET( RTL_PROCESS_BACKTRACES, BackTraces );
        if (SystemInformationLength < RequiredLength) {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            }
        else {
            BackTraceInformation->CommittedMemory =
                (ULONG)DataBase->CurrentUpperCommitLimit - (ULONG)DataBase->CommitBase;
            BackTraceInformation->ReservedMemory =
                (ULONG)DataBase->EntryIndexArray - (ULONG)DataBase->CommitBase;
            BackTraceInformation->NumberOfBackTraceLookups = DataBase->NumberOfEntriesLookedUp;
            n = DataBase->NumberOfEntriesAdded;
            BackTraceInformation->NumberOfBackTraces = n;
            }

        RequiredLength += (sizeof( *BackTraceInfo ) * n);
        if (SystemInformationLength < RequiredLength) {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            }
        else {
            Status = STATUS_SUCCESS;
            BackTraceInfo = &BackTraceInformation->BackTraces[ 0 ];
            pp = DataBase->EntryIndexArray;
            while (n--) {
                p = *--pp;
                BackTraceInfo->SymbolicBackTrace = NULL;
                BackTraceInfo->TraceCount = p->TraceCount;
                BackTraceInfo->Index = p->Index;
                BackTraceInfo->Depth = p->Depth;
                RtlMoveMemory( BackTraceInfo->BackTrace,
                               p->BackTrace,
                               p->Depth * sizeof( PVOID )
                             );
                BackTraceInfo++;
                }
            }
        }
    finally {
        DataBase->DumpInProgress = FALSE;
        }

    if (ARGUMENT_PRESENT(ReturnLength)) {
        *ReturnLength = RequiredLength;
    }
    return Status;
}
#endif // i386 && !FPO

NTSTATUS
ExpGetLockInformation (
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    )
/*++

Routine Description:

    This function returns information about all the ERESOURCE locks
    in the system.

Arguments:

    SystemInformation - A pointer to a buffer which receives the specified
        information.

    SystemInformationLength - Specifies the length in bytes of the system
        information buffer.

    Length - An optional pointer which, if specified, receives the
        number of bytes placed in the system information buffer.


Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_INVALID_INFO_CLASS - The SystemInformationClass parameter
            did not specify a valid value.

        STATUS_INFO_LENGTH_MISMATCH - The value of the SystemInformationLength
            parameter did not match the length required for the information
            class requested by the SystemInformationClass parameter.

        STATUS_ACCESS_VIOLATION - Either the SystemInformation buffer pointer
            or the Length pointer value specified an invalid address.

        STATUS_WORKING_SET_QUOTA - The process does not have sufficient
            working set to lock the specified output structure in memory.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
            for this request to complete.

--*/

{
    PRTL_PROCESS_LOCKS LockInfo;
    PVOID LockVariable;
    NTSTATUS Status;


    *Length = 0;

    LockInfo = (PRTL_PROCESS_LOCKS)
        ExLockUserBuffer( SystemInformation,
                          SystemInformationLength,
                          &LockVariable
                        );
    if (LockInfo == NULL) {
        return( STATUS_ACCESS_VIOLATION );
        }

    MmLockPagableSectionByHandle (ExPageLockHandle);
    try {

        Status = ExQuerySystemLockInformation( LockInfo,
                                               SystemInformationLength,
                                               Length
                                             );
        }
    finally {
        ExUnlockUserBuffer( LockVariable );
        MmUnlockPagableImageSection(ExPageLockHandle);
        }

    return( Status );
}

NTSTATUS
ExpGetLookasideInformation (
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG Length
    )

/*++

Routine Description:

    This function returns pool lookaside list and general lookaside
    list information.

Arguments:

    Buffer - Supplies a pointer to the buffer which receives the lookaside
        list information.

    BufferLength - Supplies the length of the information buffer in bytes.

    Length - Supplies a pointer to a variable that receives the length of
        lookaside information returned.

Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - Normal, successful completion.

        STATUS_ACCESS_VIOLATION - The buffer could not be locked in memory.

--*/

{

    PVOID BufferLock;
    PLIST_ENTRY Entry;
    ULONG Index;
    KIRQL OldIrql;
    ULONG Limit;
    PSYSTEM_LOOKASIDE_INFORMATION Lookaside;
    ULONG Number;
    PNPAGED_LOOKASIDE_LIST NPagedLookaside;
    PPAGED_LOOKASIDE_LIST PagedLookaside;
    PNPAGED_LOOKASIDE_LIST PoolLookaside;
    PKSPIN_LOCK SpinLock;
    NTSTATUS Status;

    //
    // Compute the number of lookaside entries and set the return status to
    // success.
    //

    Limit = BufferLength / sizeof(SYSTEM_LOOKASIDE_INFORMATION);
    Number = 0;
    Status = STATUS_SUCCESS;

    //
    // If the number of lookaside entries to return is not zero, then collect
    // the lookaside information.
    //

    if (Limit != 0) {
        if ((Lookaside =
                (PSYSTEM_LOOKASIDE_INFORMATION)ExLockUserBuffer(Buffer,
                                                                BufferLength,
                                                                &BufferLock)) == NULL) {
            Status = STATUS_ACCESS_VIOLATION;

        } else {
            MmLockPagableSectionByHandle(ExPageLockHandle);

            //
            // Copy nonpaged and paged pool lookaside information to
            // information buffer.
            //

            Entry = ExPoolLookasideListHead.Flink;
            while (Entry != &ExPoolLookasideListHead) {
                PoolLookaside = CONTAINING_RECORD(Entry,
                                                  NPAGED_LOOKASIDE_LIST,
                                                  L.ListEntry);

                Lookaside->CurrentDepth = (USHORT)PoolLookaside->L.ListHead.Depth;
                Lookaside->MaximumDepth = PoolLookaside->L.Depth;
                Lookaside->TotalAllocates = PoolLookaside->L.TotalAllocates;
                Lookaside->AllocateMisses =
                        PoolLookaside->L.TotalAllocates - PoolLookaside->L.AllocateHits;

                Lookaside->TotalFrees = PoolLookaside->L.TotalFrees;
                Lookaside->FreeMisses =
                        PoolLookaside->L.TotalFrees - PoolLookaside->L.FreeHits;

                Lookaside->Type = PoolLookaside->L.Type;
                Lookaside->Tag = PoolLookaside->L.Tag;
                Lookaside->Size = PoolLookaside->L.Size;
                Number += 1;
                if (Number == Limit) {
                    goto Finish2;
                }

                Entry = Entry->Flink;
                Lookaside += 1;
            }

            //
            // Copy nonpaged general lookaside information to buffer.
            //

            SpinLock = &ExNPagedLookasideLock;
            ExAcquireSpinLock(SpinLock, &OldIrql);
            Entry = ExNPagedLookasideListHead.Flink;
            while (Entry != &ExNPagedLookasideListHead) {
                NPagedLookaside = CONTAINING_RECORD(Entry,
                                                    NPAGED_LOOKASIDE_LIST,
                                                    L.ListEntry);

                Lookaside->CurrentDepth = (USHORT)NPagedLookaside->L.ListHead.Depth;
                Lookaside->MaximumDepth = NPagedLookaside->L.Depth;
                Lookaside->TotalAllocates = NPagedLookaside->L.TotalAllocates;
                Lookaside->AllocateMisses = NPagedLookaside->L.AllocateMisses;
                Lookaside->TotalFrees = NPagedLookaside->L.TotalFrees;
                Lookaside->FreeMisses = NPagedLookaside->L.FreeMisses;
                Lookaside->Type = 0;
                Lookaside->Tag = NPagedLookaside->L.Tag;
                Lookaside->Size = NPagedLookaside->L.Size;
                Number += 1;
                if (Number == Limit) {
                    goto Finish1;
                }

                Entry = Entry->Flink;
                Lookaside += 1;
            }

            ExReleaseSpinLock(SpinLock, OldIrql);

            //
            // Copy paged general lookaside information to buffer.
            //

            SpinLock = &ExPagedLookasideLock;
            ExAcquireSpinLock(SpinLock, &OldIrql);
            Entry = ExPagedLookasideListHead.Flink;
            while (Entry != &ExPagedLookasideListHead) {
                PagedLookaside = CONTAINING_RECORD(Entry,
                                                   PAGED_LOOKASIDE_LIST,
                                                   L.ListEntry);

                Lookaside->CurrentDepth = (USHORT)PagedLookaside->L.ListHead.Depth;
                Lookaside->MaximumDepth = PagedLookaside->L.Depth;
                Lookaside->TotalAllocates = PagedLookaside->L.TotalAllocates;
                Lookaside->AllocateMisses = PagedLookaside->L.AllocateMisses;
                Lookaside->TotalFrees = PagedLookaside->L.TotalFrees;
                Lookaside->FreeMisses = PagedLookaside->L.FreeMisses;
                Lookaside->Type = 1;
                Lookaside->Tag = PagedLookaside->L.Tag;
                Lookaside->Size = PagedLookaside->L.Size;
                Number += 1;
                if (Number == Limit) {
                    goto Finish1;
                }

                Entry = Entry->Flink;
                Lookaside += 1;
            }

        Finish1:
            ExReleaseSpinLock(SpinLock, OldIrql);

            //
            // Unlock user buffer and page lock image section.
            //

        Finish2:
            MmUnlockPagableImageSection(ExPageLockHandle);
            ExUnlockUserBuffer(BufferLock);
        }
    }

    *Length = Number * sizeof(SYSTEM_LOOKASIDE_INFORMATION);
    return Status;
}

NTSTATUS
ExpGetPoolInformation(
    IN POOL_TYPE PoolType,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    )
/*++

Routine Description:

    This function returns information about the specified type of pool memory.

Arguments:

    SystemInformation - A pointer to a buffer which receives the specified
        information.

    SystemInformationLength - Specifies the length in bytes of the system
        information buffer.

    Length - An optional pointer which, if specified, receives the
        number of bytes placed in the system information buffer.


Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_INVALID_INFO_CLASS - The SystemInformationClass parameter
            did not specify a valid value.

        STATUS_INFO_LENGTH_MISMATCH - The value of the SystemInformationLength
            parameter did not match the length required for the information
            class requested by the SystemInformationClass parameter.

        STATUS_ACCESS_VIOLATION - Either the SystemInformation buffer pointer
            or the Length pointer value specified an invalid address.

        STATUS_WORKING_SET_QUOTA - The process does not have sufficient
            working set to lock the specified output structure in memory.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
            for this request to complete.

--*/

{
#if DBG || (i386 && !FPO)

//
// Only works on checked builds or free x86 builds with FPO turned off
// See comment in mm\allocpag.c
//

    PSYSTEM_POOL_INFORMATION PoolInfo;
    PVOID LockVariable;
    NTSTATUS Status;


    *Length = 0;

    PoolInfo = (PSYSTEM_POOL_INFORMATION)
        ExLockUserBuffer( SystemInformation,
                          SystemInformationLength,
                          &LockVariable
                        );
    if (PoolInfo == NULL) {
        return( STATUS_ACCESS_VIOLATION );
        }

    MmLockPagableSectionByHandle (ExPageLockHandle);
    try {
        Status = ExSnapShotPool( PoolType,
                                 PoolInfo,
                                 SystemInformationLength,
                                 Length
                               );

        }
    finally {
        ExUnlockUserBuffer( LockVariable );
        MmUnlockPagableImageSection(ExPageLockHandle);
        }

    return( Status );
#else
    return STATUS_NOT_IMPLEMENTED;
#endif // DBG || (i386 && !FPO)
}

NTSTATUS
ExpGetHandleInformation(
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    )
/*++

Routine Description:

    This function returns information about the open handles in the system.

Arguments:

    SystemInformation - A pointer to a buffer which receives the specified
        information.

    SystemInformationLength - Specifies the length in bytes of the system
        information buffer.

    Length - An optional pointer which, if specified, receives the
        number of bytes placed in the system information buffer.


Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_INVALID_INFO_CLASS - The SystemInformationClass parameter
            did not specify a valid value.

        STATUS_INFO_LENGTH_MISMATCH - The value of the SystemInformationLength
            parameter did not match the length required for the information
            class requested by the SystemInformationClass parameter.

        STATUS_ACCESS_VIOLATION - Either the SystemInformation buffer pointer
            or the Length pointer value specified an invalid address.

        STATUS_WORKING_SET_QUOTA - The process does not have sufficient
            working set to lock the specified output structure in memory.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
            for this request to complete.

--*/

{
    PSYSTEM_HANDLE_INFORMATION HandleInfo;
    PVOID LockVariable;
    NTSTATUS Status;

    PAGED_CODE();

    *Length = 0;

    HandleInfo = (PSYSTEM_HANDLE_INFORMATION)
        ExLockUserBuffer( SystemInformation,
                          SystemInformationLength,
                          &LockVariable
                        );
    if (HandleInfo == NULL) {
        return( STATUS_ACCESS_VIOLATION );
        }

    try {
        Status = ObGetHandleInformation( HandleInfo,
                                         SystemInformationLength,
                                         Length
                                       );

        }
    finally {
        ExUnlockUserBuffer( LockVariable );
        }

    return( Status );
}

NTSTATUS
ExpGetObjectInformation(
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    )

/*++

Routine Description:

    This function returns information about the objects in the system.

Arguments:

    SystemInformation - A pointer to a buffer which receives the specified
        information.

    SystemInformationLength - Specifies the length in bytes of the system
        information buffer.

    Length - An optional pointer which, if specified, receives the
        number of bytes placed in the system information buffer.


Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_INVALID_INFO_CLASS - The SystemInformationClass parameter
            did not specify a valid value.

        STATUS_INFO_LENGTH_MISMATCH - The value of the SystemInformationLength
            parameter did not match the length required for the information
            class requested by the SystemInformationClass parameter.

        STATUS_ACCESS_VIOLATION - Either the SystemInformation buffer pointer
            or the Length pointer value specified an invalid address.

        STATUS_WORKING_SET_QUOTA - The process does not have sufficient
            working set to lock the specified output structure in memory.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
            for this request to complete.

--*/

{
    PSYSTEM_OBJECTTYPE_INFORMATION ObjectInfo;
    PVOID LockVariable;
    NTSTATUS Status;

    PAGED_CODE();

    *Length = 0;

    ObjectInfo = (PSYSTEM_OBJECTTYPE_INFORMATION)
        ExLockUserBuffer( SystemInformation,
                          SystemInformationLength,
                          &LockVariable
                        );
    if (ObjectInfo == NULL) {
        return( STATUS_ACCESS_VIOLATION );
        }

    try {
        Status = ObGetObjectInformation( SystemInformation,
                                         ObjectInfo,
                                         SystemInformationLength,
                                         Length
                                       );

        }
    finally {
        ExUnlockUserBuffer( LockVariable );
        }

    return( Status );
}

extern SIZE_T PoolTrackTableSize;
extern KSPIN_LOCK ExpTaggedPoolLock;

NTSTATUS
ExpGetPoolTagInfo (
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    IN OUT PULONG ReturnLength OPTIONAL
    )

{
    SIZE_T NumberOfBytes;
    ULONG totalBytes;
    ULONG i;
    KIRQL OldIrql;
    NTSTATUS status;
    PSYSTEM_POOLTAG_INFORMATION taginfo;
    PSYSTEM_POOLTAG poolTag;
    PPOOL_TRACKER_TABLE PoolTrackInfo;

    PAGED_CODE();
    if (!PoolTrackTable) {
        return STATUS_NOT_IMPLEMENTED;
    }

    totalBytes = 0;
    status = STATUS_SUCCESS;

    taginfo = (PSYSTEM_POOLTAG_INFORMATION)SystemInformation;
    poolTag = &taginfo->TagInfo[0];
    totalBytes = FIELD_OFFSET(SYSTEM_POOLTAG_INFORMATION, TagInfo);
    taginfo->Count = 0;

    //
    // Synchronize access to PoolTrackTable as it can move.
    //

    NumberOfBytes = PoolTrackTableSize * sizeof(POOL_TRACKER_TABLE);

    PoolTrackInfo = (PPOOL_TRACKER_TABLE) ExAllocatePoolWithTag (NonPagedPool,
                                                                 NumberOfBytes,
                                                                 'ofnI');

    if (PoolTrackInfo == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ExAcquireSpinLock(&ExpTaggedPoolLock, &OldIrql);

    RtlCopyMemory ((PVOID)PoolTrackInfo,
                   (PVOID)PoolTrackTable,
                   NumberOfBytes);

    ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);

    for (i = 0; i < NumberOfBytes / sizeof(POOL_TRACKER_TABLE); i += 1) {
        if (PoolTrackInfo[i].Key != 0) {
            taginfo->Count += 1;
            totalBytes += sizeof (SYSTEM_POOLTAG);
            if (SystemInformationLength < totalBytes) {
                status = STATUS_INFO_LENGTH_MISMATCH;
            } else {
                poolTag->TagUlong = PoolTrackInfo[i].Key;
                poolTag->PagedAllocs = PoolTrackInfo[i].PagedAllocs;
                poolTag->PagedFrees = PoolTrackInfo[i].PagedFrees;
                poolTag->PagedUsed = PoolTrackInfo[i].PagedBytes;
                poolTag->NonPagedAllocs = PoolTrackInfo[i].NonPagedAllocs;
                poolTag->NonPagedFrees = PoolTrackInfo[i].NonPagedFrees;
                poolTag->NonPagedUsed = PoolTrackInfo[i].NonPagedBytes;
                poolTag += 1;
            }
        }
    }

    ExFreePool (PoolTrackInfo);

    if (ARGUMENT_PRESENT(ReturnLength)) {
        *ReturnLength = totalBytes;
    }

    return status;
}


NTSTATUS
ExpQueryModuleInformation(
    IN PLIST_ENTRY LoadOrderListHead,
    IN PLIST_ENTRY UserModeLoadOrderListHead,
    OUT PRTL_PROCESS_MODULES ModuleInformation,
    IN ULONG ModuleInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    NTSTATUS Status;
    ULONG RequiredLength;
    PLIST_ENTRY Next;
    PLIST_ENTRY Next1;
    PRTL_PROCESS_MODULE_INFORMATION ModuleInfo;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry1;
    ANSI_STRING AnsiString;
    PUCHAR s;

    RequiredLength = FIELD_OFFSET( RTL_PROCESS_MODULES, Modules );
    if (ModuleInformationLength < RequiredLength) {
        Status = STATUS_INFO_LENGTH_MISMATCH;
        }
    else {
        ModuleInformation->NumberOfModules = 0;
        ModuleInfo = &ModuleInformation->Modules[ 0 ];
        Status = STATUS_SUCCESS;
        }

    Next = LoadOrderListHead->Flink;
    while ( Next != LoadOrderListHead ) {
        LdrDataTableEntry = CONTAINING_RECORD( Next,
                                               LDR_DATA_TABLE_ENTRY,
                                               InLoadOrderLinks
                                             );

        RequiredLength += sizeof( RTL_PROCESS_MODULE_INFORMATION );
        if (ModuleInformationLength < RequiredLength) {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            }
        else {

            ModuleInfo->MappedBase = NULL;
            ModuleInfo->ImageBase = LdrDataTableEntry->DllBase;
            ModuleInfo->ImageSize = LdrDataTableEntry->SizeOfImage;
            ModuleInfo->Flags = LdrDataTableEntry->Flags;
            ModuleInfo->LoadCount = LdrDataTableEntry->LoadCount;

            ModuleInfo->LoadOrderIndex = (USHORT)(ModuleInformation->NumberOfModules);
            ModuleInfo->InitOrderIndex = 0;
            AnsiString.Buffer = ModuleInfo->FullPathName;
            AnsiString.Length = 0;
            AnsiString.MaximumLength = sizeof( ModuleInfo->FullPathName );
            RtlUnicodeStringToAnsiString( &AnsiString,
                                          &LdrDataTableEntry->FullDllName,
                                          FALSE
                                        );
            s = AnsiString.Buffer + AnsiString.Length;
            while (s > AnsiString.Buffer && *--s) {
                if (*s == (UCHAR)OBJ_NAME_PATH_SEPARATOR) {
                    s++;
                    break;
                    }
                }
            ModuleInfo->OffsetToFileName = (USHORT)(s - AnsiString.Buffer);

            ModuleInfo++;
            }

        ModuleInformation->NumberOfModules++;
        Next = Next->Flink;
        }

    if (ARGUMENT_PRESENT( UserModeLoadOrderListHead )) {
        Next = UserModeLoadOrderListHead->Flink;
        while ( Next != UserModeLoadOrderListHead ) {
            LdrDataTableEntry = CONTAINING_RECORD( Next,
                                                   LDR_DATA_TABLE_ENTRY,
                                                   InLoadOrderLinks
                                                 );

            RequiredLength += sizeof( RTL_PROCESS_MODULE_INFORMATION );
            if (ModuleInformationLength < RequiredLength) {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                }
            else {
                ModuleInfo->MappedBase = NULL;
                ModuleInfo->ImageBase = LdrDataTableEntry->DllBase;
                ModuleInfo->ImageSize = LdrDataTableEntry->SizeOfImage;
                ModuleInfo->Flags = LdrDataTableEntry->Flags;
                ModuleInfo->LoadCount = LdrDataTableEntry->LoadCount;

                ModuleInfo->LoadOrderIndex = (USHORT)(ModuleInformation->NumberOfModules);

                ModuleInfo->InitOrderIndex = ModuleInfo->LoadOrderIndex;

                AnsiString.Buffer = ModuleInfo->FullPathName;
                AnsiString.Length = 0;
                AnsiString.MaximumLength = sizeof( ModuleInfo->FullPathName );
                RtlUnicodeStringToAnsiString( &AnsiString,
                                              &LdrDataTableEntry->FullDllName,
                                              FALSE
                                            );
                s = AnsiString.Buffer + AnsiString.Length;
                while (s > AnsiString.Buffer && *--s) {
                    if (*s == (UCHAR)OBJ_NAME_PATH_SEPARATOR) {
                        s++;
                        break;
                        }
                    }
                ModuleInfo->OffsetToFileName = (USHORT)(s - AnsiString.Buffer);

                ModuleInfo++;
                }

            ModuleInformation->NumberOfModules++;
            Next = Next->Flink;
            }
        }

    if (ARGUMENT_PRESENT(ReturnLength)) {
        *ReturnLength = RequiredLength;
    }
    return( Status );
}

BOOLEAN
ExIsProcessorFeaturePresent(
    ULONG ProcessorFeature
    )
{
    BOOLEAN rv;

    if ( ProcessorFeature < PROCESSOR_FEATURE_MAX ) {
        rv = SharedUserData->ProcessorFeatures[ProcessorFeature];
        }
    else {
        rv = FALSE;
        }
    return rv;
}


NTSTATUS
ExpQueryLegacyDriverInformation(
    IN PSYSTEM_LEGACY_DRIVER_INFORMATION LegacyInfo,
    IN PULONG Length
    )
/*++

Routine Description:

    Returns legacy driver information for figuring out why PNP/Power functionality
    is disabled.

Arguments:

    LegacyInfo - Returns the legacy driver information

    Length - Supplies the length of the LegacyInfo buffer
             Returns the amount of data written

Return Value:

    NTSTATUS

--*/

{
    PNP_VETO_TYPE VetoType;
    PWSTR VetoList = NULL;
    NTSTATUS Status;
    UNICODE_STRING String;
    ULONG ReturnLength;

    Status = IoGetLegacyVetoList(&VetoList, &VetoType);
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    RtlInitUnicodeString(&String, VetoList);
    ReturnLength = sizeof(SYSTEM_LEGACY_DRIVER_INFORMATION) + String.Length;
    if (ReturnLength > *Length) {
        Status = STATUS_BUFFER_OVERFLOW;
    } else {
        try {
            LegacyInfo->VetoType = VetoType;
            LegacyInfo->VetoList.Length = String.Length;
            LegacyInfo->VetoList.Buffer = (PWSTR)(LegacyInfo+1);
            RtlCopyMemory(LegacyInfo+1, String.Buffer, String.Length);
        } finally {
            if (VetoList) {
                ExFreePool(VetoList);
            }
        }
    }

    *Length = ReturnLength;
    return(Status);
}
