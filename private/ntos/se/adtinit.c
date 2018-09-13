/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adtinit.c

Abstract:

    Auditing - Initialization Routines

Author:

    Scott Birrell       (ScottBi)       November 12, 1991

Environment:

    Kernel Mode only

Revision History:

--*/


#include <nt.h>
#include "sep.h"
#include "adt.h"
#include "adtp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SepAdtInitializePhase1)
#pragma alloc_text(PAGE,SepAdtInitializeBounds)
#pragma alloc_text(PAGE,SepAdtValidateAuditBounds)
#endif


BOOLEAN
SepAdtValidateAuditBounds(
    ULONG Upper,
    ULONG Lower
    )

/*++

Routine Description:

    Examines the audit queue high and low water mark values and performs
    a general sanity check on them.

Arguments:

    Upper - High water mark.

    Lower - Low water mark.

Return Value:

    TRUE - values are acceptable.

    FALSE - values are unacceptable.


--*/

{
    PAGED_CODE();

    if ( Lower >= Upper ) {
        return( FALSE );
    }

    if ( Lower < 16 ) {
        return( FALSE );
    }

    if ( (Upper - Lower) < 16 ) {
        return( FALSE );
    }

    return( TRUE );
}

BOOLEAN
SepAdtInitializePhase1()

/*++

Routine Description:

    This function performs Phase 1 Initialization for the Auditing subcomponent
    of Security.  Global variables are initialized within the Nt Executive
    and Auditing is turned off.

Arguments:

    None

Return Value:

    BOOLEAN - TRUE if Auditing has been initialized correctly, else FALSE.

--*/

{
    PAGED_CODE();

    RtlInitUnicodeString( &SeSubsystemName, L"Security" );

    return( TRUE );
}




VOID
SepAdtInitializeBounds(
    VOID
    )

/*++

Routine Description:

    Queries the registry for the high and low water mark values for the
    audit log.  If they are not found or are unacceptable, returns without
    modifying the current values, which are statically initialized.

Arguments:

    None.

Return Value:

    None.

--*/

{

    HANDLE KeyHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    NTSTATUS Status;
    PSEP_AUDIT_BOUNDS AuditBounds;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    ULONG Length;

    PAGED_CODE();

    //
    // Get the high and low water marks out of the registry.
    //

    RtlInitUnicodeString( &KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa");

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenKey(
                 &KeyHandle,
                 KEY_QUERY_VALUE,
                 &ObjectAttributes
                 );

    if (!NT_SUCCESS( Status )) {

        //
        // Didn't work, take the defaults
        //

        return;
    }

    RtlInitUnicodeString( &ValueName, L"Bounds");

    Length = sizeof( KEY_VALUE_PARTIAL_INFORMATION ) - sizeof( UCHAR ) + sizeof( SEP_AUDIT_BOUNDS );

    KeyValueInformation = ExAllocatePool( PagedPool, Length );

    if ( KeyValueInformation == NULL ) {

        NtClose( KeyHandle );
        return;
    }

    Status = NtQueryValueKey(
                 KeyHandle,
                 &ValueName,
                 KeyValuePartialInformation,
                 (PVOID)KeyValueInformation,
                 Length,
                 &Length
                 );

    NtClose( KeyHandle );

    if (!NT_SUCCESS( Status )) {

        ExFreePool( KeyValueInformation );
        return;
    }


    AuditBounds = (PSEP_AUDIT_BOUNDS) &KeyValueInformation->Data;

    //
    // Sanity check what we got back
    //

    if(!SepAdtValidateAuditBounds( AuditBounds->UpperBound, AuditBounds->LowerBound )) {

        //
        // The values we got back are not to our liking.  Use the defaults.
        //

        ExFreePool( KeyValueInformation );
        return;
    }

    //
    // Take what we got from the registry.
    //

    SepAdtMaxListLength = AuditBounds->UpperBound;
    SepAdtMinListLength = AuditBounds->LowerBound;

    ExFreePool( KeyValueInformation );

    return;
}



NTSTATUS
SepAdtInitializeCrashOnFail(
    VOID
    )

/*++

Routine Description:

    Reads the registry to see if the user has told us to crash if an audit fails.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS

--*/

{
    HANDLE KeyHandle;
    NTSTATUS Status;
    NTSTATUS TmpStatus;
    OBJECT_ATTRIBUTES Obja;
    ULONG ResultLength;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    CHAR KeyInfo[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(BOOLEAN)];
    PKEY_VALUE_PARTIAL_INFORMATION pKeyInfo;

    SepCrashOnAuditFail = FALSE;

    //
    // Check the value of the CrashOnAudit flag in the registry.
    //

    RtlInitUnicodeString( &KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa");

    InitializeObjectAttributes( &Obja,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                                );

    Status = NtOpenKey(
                 &KeyHandle,
                 KEY_QUERY_VALUE | KEY_SET_VALUE,
                 &Obja
                 );

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        return( STATUS_SUCCESS );
    }

    RtlInitUnicodeString( &ValueName, CRASH_ON_AUDIT_FAIL_VALUE );

    Status = NtQueryValueKey(
                 KeyHandle,
                 &ValueName,
                 KeyValuePartialInformation,
                 KeyInfo,
                 sizeof(KeyInfo),
                 &ResultLength
                 );

    TmpStatus = NtClose(KeyHandle);
    ASSERT(NT_SUCCESS(TmpStatus));

    //
    // If the key isn't there, don't turn on CrashOnFail.
    //

    if (NT_SUCCESS( Status )) {

        pKeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION)KeyInfo;
        if ((UCHAR) *(pKeyInfo->Data) == LSAP_CRASH_ON_AUDIT_FAIL) {
            SepCrashOnAuditFail = TRUE;
        }
    }

    return( STATUS_SUCCESS );
}


BOOLEAN
SepAdtInitializePrivilegeAuditing(
    VOID
    )

/*++

Routine Description:

    Checks to see if there is an entry in the registry telling us to do full privilege auditing
    (which currently means audit everything we normall audit, plus backup and restore privileges).

Arguments:

    None

Return Value:

    BOOLEAN - TRUE if Auditing has been initialized correctly, else FALSE.

--*/

{
    HANDLE KeyHandle;
    NTSTATUS Status;
    NTSTATUS TmpStatus;
    OBJECT_ATTRIBUTES Obja;
    ULONG ResultLength;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    CHAR KeyInfo[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(BOOLEAN)];
    PKEY_VALUE_PARTIAL_INFORMATION pKeyInfo;
    BOOLEAN Verbose;

    PAGED_CODE();

    //
    // Query the registry to set up the privilege auditing filter.
    //

    RtlInitUnicodeString( &KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa");

    InitializeObjectAttributes( &Obja,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                                );

    Status = NtOpenKey(
                 &KeyHandle,
                 KEY_QUERY_VALUE | KEY_SET_VALUE,
                 &Obja
                 );


    if (!NT_SUCCESS( Status )) {

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {

            return ( SepInitializePrivilegeFilter( FALSE ));

        } else {

            return( FALSE );
        }
    }

    RtlInitUnicodeString( &ValueName, FULL_PRIVILEGE_AUDITING );

    Status = NtQueryValueKey(
                 KeyHandle,
                 &ValueName,
                 KeyValuePartialInformation,
                 KeyInfo,
                 sizeof(KeyInfo),
                 &ResultLength
                 );

    TmpStatus = NtClose(KeyHandle);
    ASSERT(NT_SUCCESS(TmpStatus));

    if (!NT_SUCCESS( Status )) {

        Verbose = FALSE;

    } else {

        pKeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION)KeyInfo;
        Verbose = (BOOLEAN) *(pKeyInfo->Data);
    }

    return ( SepInitializePrivilegeFilter( Verbose ));
}


VOID
SepAdtInitializeAuditingOptions(
    VOID
    )

/*++

Routine Description:

    Initialize options that control auditing.
    (please refer to note in adtp.h near the def. of SEP_AUDIT_OPTIONS)

Arguments:

    None

Return Value:

    None

--*/

{
    HANDLE KeyHandle;
    NTSTATUS Status;
    NTSTATUS TmpStatus;
    OBJECT_ATTRIBUTES Obja;
    ULONG ResultLength;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    CHAR KeyInfo[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(BOOLEAN)];

    PAGED_CODE();

    //
    // Query the registry 
    //
    RtlInitUnicodeString( &KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa\\AuditingOptions");

    InitializeObjectAttributes( &Obja,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                                );

    Status = NtOpenKey(
                 &KeyHandle,
                 KEY_QUERY_VALUE,
                 &Obja
                 );


    if (!NT_SUCCESS( Status )) {

        goto Cleanup;
    }

    RtlInitUnicodeString( &ValueName, L"DoNotAuditCloseObjectEvents" );

    Status = NtQueryValueKey(
                 KeyHandle,
                 &ValueName,
                 KeyValuePartialInformation,
                 KeyInfo,
                 sizeof(KeyInfo),
                 &ResultLength
                 );

    TmpStatus = NtClose(KeyHandle);
    ASSERT(NT_SUCCESS(TmpStatus));

    if (NT_SUCCESS( Status )) {
        //
        // we check for the presence of this value, its value does not matter
        //
        SepAuditOptions.DoNotAuditCloseObjectEvents = TRUE;
    }

Cleanup:

    return;
}

