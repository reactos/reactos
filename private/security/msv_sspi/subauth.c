/*++

Copyright (c) 1987-1994  Microsoft Corporation

Module Name:

    subauth.c

Abstract:

    Interface to SubAuthentication Package.

Author:

    Cliff Van Dyke (cliffv) 23-May-1994

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:
 Chandana Surlu         21-Jul-96      Stolen from \\kernel\razzle3\src\security\msv1_0\subauth.c

--*/

#include "msp.h"
#include "nlp.h"
#include <winreg.h>
#include <kerberos.h>

//
// Prototype for subauthentication routines.
//
// the pre NT 5.0 Subauth routine
typedef NTSTATUS
(*PSUBAUTHENTICATION_ROUTINE)(
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN ULONG Flags,
    IN PUSER_ALL_INFORMATION UserAll,
    OUT PULONG WhichFields,
    OUT PULONG UserFlags,
    OUT PBOOLEAN Authoritative,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
);

// the NT 5.0 Subauth routine
typedef NTSTATUS
(*PSUBAUTHENTICATION_ROUTINEEX)(
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN ULONG Flags,
    IN PUSER_ALL_INFORMATION UserAll,
    IN SAM_HANDLE UserHandle,
    IN OUT PMSV1_0_VALIDATION_INFO ValidationInfo,
    OUT PULONG ActionsPerfomed
);

// the NT 5.0 Generic Subauth routine
typedef NTSTATUS
(*PSUBAUTHENTICATION_ROUTINEGENERIC)(
    IN PVOID SubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PULONG ReturnBufferLength,
    OUT PVOID *ReturnBuffer
);
typedef enum _SUBAUTH_TYPE {
    SubAuth = 1,   // Pre NT 5.0 subAuth called using LogonUser
    SubAuthEx,     // NT 5.0 subAuth called during LogonUser
    SubAuthGeneric // NT 5.0 subAuth called during LaCallAuthenticationPackage
} SUBAUTH_TYPE;
//
// Structure describing a loaded SubAuthentication DLL.
//

typedef struct _SUBAUTHENTICATION_DLL {
    LIST_ENTRY Next;
    ULONG DllNumber;
    PSUBAUTHENTICATION_ROUTINE SubAuthenticationRoutine;
    PSUBAUTHENTICATION_ROUTINEEX SubAuthenticationRoutineEx;
    PSUBAUTHENTICATION_ROUTINEGENERIC SubAuthenticationRoutineGeneric;
} SUBAUTHENTICATION_DLL, *PSUBAUTHENTICATION_DLL;

//
// Global list of all loaded subauthentication DLLs
//

LIST_ENTRY SubAuthenticationDlls;
CRITICAL_SECTION SubAuthenticationCritSect;




VOID
Msv1_0SubAuthenticationInitialization(
    VOID
)
/*++

Routine Description:

    Initialization routine for this source file.

Arguments:

    None.

Return Value:

    None.

--*/
{
    InitializeCriticalSection( &SubAuthenticationCritSect );
    InitializeListHead( &SubAuthenticationDlls );
}

PSUBAUTHENTICATION_DLL ReferenceSubAuth (
    IN ULONG DllNumber,
    OUT PNTSTATUS SubStatus)
{
    LONG RegStatus;

    PSUBAUTHENTICATION_DLL SubAuthenticationDll = NULL;

    HKEY ParmHandle = NULL;
    HINSTANCE DllHandle = NULL;

    CHAR ValueName[sizeof(MSV1_0_SUBAUTHENTICATION_VALUE)+3];
    CHAR DllName[MAXIMUM_FILENAME_LENGTH+1];
    DWORD DllNameSize;
    DWORD DllNameType;
    PSUBAUTHENTICATION_ROUTINE SubAuthenticationRoutine = NULL;
    PSUBAUTHENTICATION_ROUTINEEX SubAuthenticationRoutineEx = NULL;
    PSUBAUTHENTICATION_ROUTINEGENERIC SubAuthenticationRoutineGeneric = NULL;

    PLIST_ENTRY ListEntry;

    *SubStatus = STATUS_SUCCESS;
    DllName[0] = 0;

    // See if the SubAuthentication Dll is already loaded.
    //

    for ( ListEntry = SubAuthenticationDlls.Flink ;
          ListEntry != &SubAuthenticationDlls ;
          ListEntry = ListEntry->Flink) {

        SubAuthenticationDll = CONTAINING_RECORD( ListEntry,
                                                  SUBAUTHENTICATION_DLL,
                                                  Next );

        if ( SubAuthenticationDll->DllNumber == DllNumber ) {
            break;
        }

        SubAuthenticationDll = NULL;

    }

    //
    // If the Dll is not already loaded,
    // load it.
    //

    if ( SubAuthenticationDll == NULL ) {

        //
        // Build the name of the registry value.
        //

        RtlCopyMemory( ValueName,
                       MSV1_0_SUBAUTHENTICATION_VALUE,
                       sizeof(MSV1_0_SUBAUTHENTICATION_VALUE) );

        *SubStatus = RtlIntegerToChar(
                    DllNumber & KERB_SUBAUTHENTICATION_MASK,
                    10,          // Base
                    4,           // Length of buffer
                    &ValueName[sizeof(MSV1_0_SUBAUTHENTICATION_VALUE)-1] );

        if ( !NT_SUCCESS(*SubStatus) ) {
            goto Cleanup;
        }


        //
        // Open the MSV1_0_SUBAUTHENTICATION_KEY registry key.
        //


        if ((DllNumber & KERB_SUBAUTHENTICATION_FLAG) == 0) {
            RegStatus = RegOpenKeyExA(
                            HKEY_LOCAL_MACHINE,
                            MSV1_0_SUBAUTHENTICATION_KEY,
                            0,      //Reserved
                            KEY_QUERY_VALUE,
                            &ParmHandle );
        } else {
            RegStatus = RegOpenKeyExA(
                            HKEY_LOCAL_MACHINE,
                            KERB_SUBAUTHENTICATION_KEY,
                            0,      //Reserved
                            KEY_QUERY_VALUE,
                            &ParmHandle );

        }

        if ( RegStatus != ERROR_SUCCESS ) {
            KdPrint(( "MSV1_0: Cannot open registry key %s %ld.\n",
                      MSV1_0_SUBAUTHENTICATION_KEY,
                      RegStatus ));
        }
        else
        {

            //
            // Get the registry value.
            //

            DllNameSize = sizeof(DllName);

            RegStatus = RegQueryValueExA(
                        ParmHandle,
                        ValueName,
                        NULL,     // Reserved
                        &DllNameType,
                        DllName,
                        &DllNameSize );

            if ( RegStatus != ERROR_SUCCESS ) {
                KdPrint(( "MSV1_0: Cannot query registry value %s %ld, don't panic.\n",
                      ValueName,
                      RegStatus ));
            }
            else
            {

                if ( DllNameType != REG_SZ ) {
                    KdPrint(( "MSV1_0: Registry value %s isn't REG_SZ.\n",
                          ValueName ));
                    *SubStatus = STATUS_DLL_NOT_FOUND;
                    goto Cleanup;
                }

                //
                // Load the DLL
                //

                DllHandle = LoadLibraryA( DllName );

                if ( DllHandle == NULL ) {
                    KdPrint(( "MSV1_0: Cannot load dll %s %ld.\n",
                      DllName,
                      GetLastError() ));
                    *SubStatus = STATUS_DLL_NOT_FOUND;
                    goto Cleanup;
                }

                //
                // Find the SubAuthenticationRoutine. For packages other than
                // zero, this will be Msv1_0SubauthenticationRoutine. For packge
                // zero it will be Msv1_0SubauthenticationFilter.
                //

                if ((DllNumber & KERB_SUBAUTHENTICATION_MASK) == 0)
                {
                    SubAuthenticationRoutine = (PSUBAUTHENTICATION_ROUTINE)
                    GetProcAddress(DllHandle, "Msv1_0SubAuthenticationFilter");
                }
                else
                {
                    SubAuthenticationRoutine = (PSUBAUTHENTICATION_ROUTINE)
                    GetProcAddress(DllHandle, "Msv1_0SubAuthenticationRoutine");
                }

                //
                // Find the SubAuthenticationRoutine
                //

                SubAuthenticationRoutineEx = (PSUBAUTHENTICATION_ROUTINEEX)
                GetProcAddress(DllHandle, "Msv1_0SubAuthenticationRoutineEx");

                //
                // Find the SubAuthenticationRoutineGeneric
                //

                SubAuthenticationRoutineGeneric = (PSUBAUTHENTICATION_ROUTINEGENERIC)
                GetProcAddress(DllHandle, "Msv1_0SubAuthenticationRoutineGeneric");

            }
        }

        //
        // If we didn't find the DLL or any routines, bail out now.
        //

        if ((DllHandle == NULL) ||
            ((SubAuthenticationRoutine == NULL) &&
            (SubAuthenticationRoutineEx == NULL) &&
            (SubAuthenticationRoutineGeneric == NULL))) {

           KdPrint(( "MSV1_0: Cannot find any of the subauth entry points in %s %ld.\n",
               DllName,
               GetLastError() ));
            *SubStatus = STATUS_PROCEDURE_NOT_FOUND;
            goto Cleanup;
        }

        //
        // Cache the address of the procedure.
        //

        SubAuthenticationDll =
            RtlAllocateHeap(MspHeap, 0, sizeof(SUBAUTHENTICATION_DLL));

        if ( SubAuthenticationDll == NULL ) {
            *SubStatus = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        SubAuthenticationDll->DllNumber = DllNumber;
        SubAuthenticationDll->SubAuthenticationRoutine = SubAuthenticationRoutine;
        SubAuthenticationDll->SubAuthenticationRoutineEx = SubAuthenticationRoutineEx;
        SubAuthenticationDll->SubAuthenticationRoutineGeneric = SubAuthenticationRoutineGeneric;
        InsertHeadList( &SubAuthenticationDlls, &SubAuthenticationDll->Next );

        DllHandle = NULL;

    }

    //
    // Cleanup up before returning.
    //

Cleanup:


    if ( ParmHandle != NULL ) {
        RegCloseKey( ParmHandle );
    }

    if ( !NT_SUCCESS( *SubStatus) ) {
        if ( DllHandle != NULL ) {
            FreeLibrary( DllHandle );
        }
    }

    return SubAuthenticationDll;
}

BOOLEAN
Msv1_0SubAuthenticationPresent(
    IN ULONG DllNumber
)
/*++

Routine Description:

    Returns TRUE if there is a subauthentication package with the given number

Arguments:

    DllNumber - the number of the DLL to check

Return Value:

    TRUE if there is a subauthentication DLL, otherwise FALSE.

--*/
{
    NTSTATUS SubStatus;

    return(ReferenceSubAuth ( DllNumber, &SubStatus) != NULL);
}


NTSTATUS
Msv1_0SubAuthenticationRoutineZero(
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN ULONG Flags,
    IN PUSER_ALL_INFORMATION UserAll,
    OUT PULONG WhichFields,
    OUT PULONG UserFlags,
    OUT PBOOLEAN Authoritative,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
    )
{
    PNETLOGON_LOGON_IDENTITY_INFO LogonInfo;
    ULONG DllNumber;

    LogonInfo = (PNETLOGON_LOGON_IDENTITY_INFO) LogonInformation;
    DllNumber = LogonInfo->ParameterControl >> MSV1_0_SUBAUTHENTICATION_DLL_SHIFT;

    if( DllNumber != 0 ) {
        return STATUS_SUCCESS;
    }

    return Msv1_0SubAuthenticationRoutine(
                    LogonLevel,
                    LogonInformation,
                    Flags,
                    UserAll,
                    WhichFields,
                    UserFlags,
                    Authoritative,
                    LogoffTime,
                    KickoffTime
                    );
}



NTSTATUS
Msv1_0SubAuthenticationRoutine(
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN ULONG Flags,
    IN PUSER_ALL_INFORMATION UserAll,
    OUT PULONG WhichFields,
    OUT PULONG UserFlags,
    OUT PBOOLEAN Authoritative,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
)
/*++

Routine Description:

    The subauthentication routine does client/server specific authentication
    of a user.  This stub routine loads the appropriate subauthentication
    package DLL and calls out to that DLL to do the actuall validation.

Arguments:

    LogonLevel -- Specifies the level of information given in
        LogonInformation.

    LogonInformation -- Specifies the description for the user
        logging on.  The LogonDomainName field should be ignored.

    Flags - Flags describing the circumstances of the logon.

        MSV1_0_PASSTHRU -- This is a PassThru authenication.  (i.e., the
            user isn't connecting to this machine.)
        MSV1_0_GUEST_LOGON -- This is a retry of the logon using the GUEST
            user account.

    UserAll -- The description of the user as returned from SAM.

    WhichFields -- Returns which fields from UserAllInfo are to be written
        back to SAM.  The fields will only be written if MSV returns success
        to it's caller.  Only the following bits are valid.

        USER_ALL_PARAMETERS - Write UserAllInfo->Parameters back to SAM.  If
            the size of the buffer is changed, Msv1_0SubAuthenticationRoutine
            must delete the old buffer using MIDL_user_free() and reallocate the
            buffer using MIDL_user_allocate().

    UserFlags -- Returns UserFlags to be returned from LsaLogonUser in the
        LogonProfile.  The following bits are currently defined:


            LOGON_GUEST -- This was a guest logon
            LOGON_NOENCRYPTION -- The caller didn't specify encrypted credentials
            LOGON_GRACE_LOGON -- The caller's password has expired but logon
                was allowed during a grace period following the expiration.

        SubAuthentication packages should restrict themselves to returning
        bits in the high order byte of UserFlags.  However, this convention
        isn't enforced giving the SubAuthentication package more flexibility.

    Authoritative -- Returns whether the status returned is an
        authoritative status which should be returned to the original
        caller.  If not, this logon request may be tried again on another
        domain controller.  This parameter is returned regardless of the
        status code.

    LogoffTime - Receives the time at which the user should logoff the
        system.  This time is specified as a GMT relative NT system time.

    KickoffTime - Receives the time at which the user should be kicked
        off the system. This time is specified as a GMT relative NT system
        time.  Specify, a full scale positive number if the user isn't to
        be kicked off.

Return Value:

    STATUS_SUCCESS: if there was no error.

    STATUS_NO_SUCH_USER: The specified user has no account.
    STATUS_WRONG_PASSWORD: The password was invalid.

    STATUS_INVALID_INFO_CLASS: LogonLevel is invalid.
    STATUS_ACCOUNT_LOCKED_OUT: The account is locked out
    STATUS_ACCOUNT_DISABLED: The account is disabled
    STATUS_ACCOUNT_EXPIRED: The account has expired.
    STATUS_PASSWORD_MUST_CHANGE: Account is marked as Password must change
        on next logon.
    STATUS_PASSWORD_EXPIRED: The Password is expired.
    STATUS_INVALID_LOGON_HOURS - The user is not authorized to logon at
        this time.
    STATUS_INVALID_WORKSTATION - The user is not authorized to logon to
        the specified workstation.

--*/
{
    NTSTATUS Status;
    NTSTATUS SubStatus;

    ULONG DllNumber;
    PSUBAUTHENTICATION_DLL SubAuthenticationDll;
    PSUBAUTHENTICATION_ROUTINE SubAuthenticationRoutine;

    PNETLOGON_LOGON_IDENTITY_INFO LogonInfo;
    BOOLEAN CritSectLocked = FALSE;


    //
    // Initialization
    //

    LogonInfo = (PNETLOGON_LOGON_IDENTITY_INFO) LogonInformation;

    DllNumber = LogonInfo->ParameterControl >> MSV1_0_SUBAUTHENTICATION_DLL_SHIFT;
    *Authoritative = TRUE;

    EnterCriticalSection( &SubAuthenticationCritSect );
    CritSectLocked = TRUE;

    //
    // Find the SubAuthentication Dll.
    //

    SubAuthenticationDll = ReferenceSubAuth ( DllNumber, &SubStatus);

    //
    // If this was package zero and we didn't find it, remember it for
    // next time.
    //

    if ( (DllNumber == 0) && (SubAuthenticationDll == NULL) ) {
        NlpSubAuthZeroExists = FALSE;
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }


    if (SubStatus != STATUS_SUCCESS)
    {
        KdPrint(( "MSV1_0: SubAuth Error value is %ld.\n", SubStatus));
        Status = SubStatus;
        goto Cleanup;
    }


    //
    // Leave the crit sect while calling the DLL
    //

    SubAuthenticationRoutine = SubAuthenticationDll->SubAuthenticationRoutine;
    LeaveCriticalSection( &SubAuthenticationCritSect );
    CritSectLocked = FALSE;

    if (SubAuthenticationRoutine == NULL)
    {
        if( DllNumber == 0 ) {

            //
            // If this was package zero and we didn't find it, remember it for
            // next time.
            //

            NlpSubAuthZeroExists = FALSE;
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_PROCEDURE_NOT_FOUND;
        }
        goto Cleanup;
    }

    //
    // Call the actual authentication routine.
    //

    Status = (*SubAuthenticationRoutine)(
                   LogonLevel,
                   LogonInformation,
                   Flags,
                   UserAll,
                   WhichFields,
                   UserFlags,
                   Authoritative,
                   LogoffTime,
                   KickoffTime );

    //
    // Cleanup up before returning.
    //

Cleanup:

    if ( CritSectLocked ) {
        LeaveCriticalSection( &SubAuthenticationCritSect );
    }

    return Status;
}


NTSTATUS
Msv1_0ExportSubAuthenticationRoutine(
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN ULONG Flags,
    IN ULONG DllNumber,
    IN PUSER_ALL_INFORMATION UserAll,
    OUT PULONG WhichFields,
    OUT PULONG UserFlags,
    OUT PBOOLEAN Authoritative,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
)
/*++

Routine Description:

    The subauthentication routine does client/server specific authentication
    of a user.  This stub routine loads the appropriate subauthentication
    package DLL and calls out to that DLL to do the actuall validation.

Arguments:

    LogonLevel -- Specifies the level of information given in
        LogonInformation.

    LogonInformation -- Specifies the description for the user
        logging on.  The LogonDomainName field should be ignored.

    Flags -- Flags describing the circumstances of the logon.

        MSV1_0_PASSTHRU -- This is a PassThru authenication.  (i.e., the
            user isn't connecting to this machine.)
        MSV1_0_GUEST_LOGON -- This is a retry of the logon using the GUEST
            user account.

    DllNumber - The number of the subauthentication DLL to call.

    UserAll -- The description of the user as returned from SAM.

    WhichFields -- Returns which fields from UserAllInfo are to be written
        back to SAM.  The fields will only be written if MSV returns success
        to it's caller.  Only the following bits are valid.

        USER_ALL_PARAMETERS - Write UserAllInfo->Parameters back to SAM.  If
            the size of the buffer is changed, Msv1_0SubAuthenticationRoutine
            must delete the old buffer using MIDL_user_free() and reallocate the
            buffer using MIDL_user_allocate().

    UserFlags -- Returns UserFlags to be returned from LsaLogonUser in the
        LogonProfile.  The following bits are currently defined:


            LOGON_GUEST -- This was a guest logon
            LOGON_NOENCRYPTION -- The caller didn't specify encrypted credentials
            LOGON_GRACE_LOGON -- The caller's password has expired but logon
                was allowed during a grace period following the expiration.

        SubAuthentication packages should restrict themselves to returning
        bits in the high order byte of UserFlags.  However, this convention
        isn't enforced giving the SubAuthentication package more flexibility.

    Authoritative -- Returns whether the status returned is an
        authoritative status which should be returned to the original
        caller.  If not, this logon request may be tried again on another
        domain controller.  This parameter is returned regardless of the
        status code.

    LogoffTime - Receives the time at which the user should logoff the
        system.  This time is specified as a GMT relative NT system time.

    KickoffTime - Receives the time at which the user should be kicked
        off the system. This time is specified as a GMT relative NT system
        time.  Specify, a full scale positive number if the user isn't to
        be kicked off.

Return Value:

    STATUS_SUCCESS: if there was no error.

    STATUS_NO_SUCH_USER: The specified user has no account.
    STATUS_WRONG_PASSWORD: The password was invalid.

    STATUS_INVALID_INFO_CLASS: LogonLevel is invalid.
    STATUS_ACCOUNT_LOCKED_OUT: The account is locked out
    STATUS_ACCOUNT_DISABLED: The account is disabled
    STATUS_ACCOUNT_EXPIRED: The account has expired.
    STATUS_PASSWORD_MUST_CHANGE: Account is marked as Password must change
        on next logon.
    STATUS_PASSWORD_EXPIRED: The Password is expired.
    STATUS_INVALID_LOGON_HOURS - The user is not authorized to logon at
        this time.
    STATUS_INVALID_WORKSTATION - The user is not authorized to logon to
        the specified workstation.

--*/
{
    NTSTATUS Status;
    NTSTATUS SubStatus;

    PSUBAUTHENTICATION_DLL SubAuthenticationDll;
    PSUBAUTHENTICATION_ROUTINE SubAuthenticationRoutine;

    PNETLOGON_LOGON_IDENTITY_INFO LogonInfo;
    BOOLEAN CritSectLocked = FALSE;


    //
    // Initialization
    //

    LogonInfo = (PNETLOGON_LOGON_IDENTITY_INFO) LogonInformation;

    *Authoritative = TRUE;

    EnterCriticalSection( &SubAuthenticationCritSect );
    CritSectLocked = TRUE;

    //
    // Find the SubAuthentication Dll.
    //

    SubAuthenticationDll = ReferenceSubAuth ( DllNumber, &SubStatus);

    if (SubStatus != STATUS_SUCCESS)
    {
        KdPrint(( "MSV1_0: SubAuth Error value is %ld.\n", SubStatus));
        Status = SubStatus;
        goto Cleanup;
    }

    //
    // Leave the crit sect while calling the DLL
    //

    SubAuthenticationRoutine = SubAuthenticationDll->SubAuthenticationRoutine;
    LeaveCriticalSection( &SubAuthenticationCritSect );
    CritSectLocked = FALSE;

    if (SubAuthenticationRoutine == NULL)
    {
        Status = STATUS_PROCEDURE_NOT_FOUND;
        goto Cleanup;
    }

    //
    // Call the actual authentication routine.
    //

    Status = (*SubAuthenticationRoutine)(
                   LogonLevel,
                   LogonInformation,
                   Flags,
                   UserAll,
                   WhichFields,
                   UserFlags,
                   Authoritative,
                   LogoffTime,
                   KickoffTime );

    //
    // Cleanup up before returning.
    //

Cleanup:

    if ( CritSectLocked ) {
        LeaveCriticalSection( &SubAuthenticationCritSect );
    }

    return Status;
}



NTSTATUS
Msv1_0SubAuthenticationRoutineEx(
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN ULONG Flags,
    IN PUSER_ALL_INFORMATION UserAll,
    IN SAM_HANDLE UserHandle,
    IN OUT PMSV1_0_VALIDATION_INFO ValidationInfo,
    OUT PULONG ActionsPerformed
)
/*++

Routine Description:

    The subauthentication routine does client/server specific authentication
    of a user.  This stub routine loads the appropriate subauthentication
    package DLL and calls out to that DLL to do the actuall validation.

Arguments:

    LogonLevel -- Specifies the level of information given in
        LogonInformation.

    LogonInformation -- Specifies the description for the user
        logging on.  The LogonDomainName field should be ignored.

    Flags - Flags describing the circumstances of the logon.

        MSV1_0_PASSTHRU -- This is a PassThru authenication.  (i.e., the
            user isn't connecting to this machine.)
        MSV1_0_GUEST_LOGON -- This is a retry of the logon using the GUEST
            user account.

    UserAll -- The description of the user as returned from SAM.

    WhichFields -- Returns which fields from UserAllInfo are to be written
        back to SAM.  The fields will only be written if MSV returns success
        to it's caller.  Only the following bits are valid.

        USER_ALL_PARAMETERS - Write UserAllInfo->Parameters back to SAM.  If
            the size of the buffer is changed, Msv1_0SubAuthenticationRoutine
            must delete the old buffer using MIDL_user_free() and reallocate the
            buffer using MIDL_user_allocate().

    UserFlags -- Returns UserFlags to be returned from LsaLogonUser in the
        LogonProfile.  The following bits are currently defined:


            LOGON_GUEST -- This was a guest logon
            LOGON_NOENCRYPTION -- The caller didn't specify encrypted credentials
            LOGON_GRACE_LOGON -- The caller's password has expired but logon
                was allowed during a grace period following the expiration.

        SubAuthentication packages should restrict themselves to returning
        bits in the high order byte of UserFlags.  However, this convention
        isn't enforced giving the SubAuthentication package more flexibility.

    Authoritative -- Returns whether the status returned is an
        authoritative status which should be returned to the original
        caller.  If not, this logon request may be tried again on another
        domain controller.  This parameter is returned regardless of the
        status code.

    LogoffTime - Receives the time at which the user should logoff the
        system.  This time is specified as a GMT relative NT system time.

    KickoffTime - Receives the time at which the user should be kicked
        off the system. This time is specified as a GMT relative NT system
        time.  Specify, a full scale positive number if the user isn't to
        be kicked off.

Return Value:

    STATUS_SUCCESS: if there was no error.

    STATUS_NO_SUCH_USER: The specified user has no account.
    STATUS_WRONG_PASSWORD: The password was invalid.

    STATUS_INVALID_INFO_CLASS: LogonLevel is invalid.
    STATUS_ACCOUNT_LOCKED_OUT: The account is locked out
    STATUS_ACCOUNT_DISABLED: The account is disabled
    STATUS_ACCOUNT_EXPIRED: The account has expired.
    STATUS_PASSWORD_MUST_CHANGE: Account is marked as Password must change
        on next logon.
    STATUS_PASSWORD_EXPIRED: The Password is expired.
    STATUS_INVALID_LOGON_HOURS - The user is not authorized to logon at
        this time.
    STATUS_INVALID_WORKSTATION - The user is not authorized to logon to
        the specified workstation.

--*/
{
    NTSTATUS Status;
    NTSTATUS SubStatus;

    ULONG DllNumber;
    PSUBAUTHENTICATION_DLL SubAuthenticationDll;
    PSUBAUTHENTICATION_ROUTINEEX SubAuthenticationRoutineEx;

    PNETLOGON_LOGON_IDENTITY_INFO LogonInfo;
    BOOLEAN CritSectLocked = FALSE;

    //
    // Initialization
    //

    LogonInfo = (PNETLOGON_LOGON_IDENTITY_INFO) LogonInformation;

    DllNumber = LogonInfo->ParameterControl >> MSV1_0_SUBAUTHENTICATION_DLL_SHIFT;

    EnterCriticalSection( &SubAuthenticationCritSect );
    CritSectLocked = TRUE;

    //
    // Find the SubAuthentication Dll.
    //

    SubAuthenticationDll = ReferenceSubAuth (DllNumber, &SubStatus);;


    if (SubStatus != STATUS_SUCCESS)
    {
        KdPrint(( "MSV1_0: SubAuth Error value is %ld.\n", SubStatus));
        Status = SubStatus;
        goto Cleanup;
    }

    //
    // Leave the crit sect while calling the DLL
    //

    SubAuthenticationRoutineEx = SubAuthenticationDll->SubAuthenticationRoutineEx;
    LeaveCriticalSection( &SubAuthenticationCritSect );
    CritSectLocked = FALSE;


    if (SubAuthenticationRoutineEx == NULL)
    {
        Status = STATUS_PROCEDURE_NOT_FOUND;
        goto Cleanup;
    }
    //
    // Call the actual authentication routine.
    //

    Status = (*SubAuthenticationRoutineEx)(
                   LogonLevel,
                   LogonInformation,
                   Flags,
                   UserAll,
                   UserHandle,
                   ValidationInfo,
                   ActionsPerformed
                   );

    //
    // Cleanup up before returning.
    //

Cleanup:

    if ( CritSectLocked ) {
        LeaveCriticalSection( &SubAuthenticationCritSect );
    }

    return Status;
}

NTSTATUS
MspNtSubAuth(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    )

/*++

Routine Description:

    This routine is the dispatch routine for LsaCallAuthenticationPackage()
    with a message type of MsV1_0SubAuthInfo.

Arguments:

    The arguments to this routine are identical to those of LsaApCallPackage.
    Only the special attributes of these parameters as they apply to
    this routine are mentioned here.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.


--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SubStatus = STATUS_SUCCESS;
    PMSV1_0_SUBAUTH_REQUEST SubAuthRequest;
    PMSV1_0_SUBAUTH_RESPONSE SubAuthResponse;
    CLIENT_BUFFER_DESC ClientBufferDesc;
    ULONG ReturnDataLength = 0;
    PVOID ReturnDataBuffer = NULL;
    BOOLEAN CritSectLocked = FALSE;

    PSUBAUTHENTICATION_DLL SubAuthenticationDll;
    PSUBAUTHENTICATION_ROUTINEGENERIC SubAuthenticationRoutineGeneric;

    NlpInitClientBuffer( &ClientBufferDesc, ClientRequest );
    *ProtocolStatus = STATUS_SUCCESS;

    //
    // Ensure the specified Submit Buffer is of reasonable size and
    // relocate all of the pointers to be relative to the LSA allocated
    // buffer.
    //

    if ( SubmitBufferSize < sizeof(MSV1_0_SUBAUTH_REQUEST) ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    SubAuthRequest = (PMSV1_0_SUBAUTH_REQUEST) ProtocolSubmitBuffer;

    //
    // Make sure the buffer fits in the supplied size
    //

    if (SubAuthRequest->SubAuthSubmitBuffer != NULL) {
        if (SubAuthRequest->SubAuthSubmitBuffer + SubAuthRequest->SubAuthInfoLength >
            (PUCHAR) ClientBufferBase + SubmitBufferSize) {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        // Reset the pointers for the validation data
        //

        SubAuthRequest->SubAuthSubmitBuffer =
                (PUCHAR) SubAuthRequest -
                (ULONG_PTR) ClientBufferBase +
                (ULONG_PTR) SubAuthRequest->SubAuthSubmitBuffer;

    }

    // If subauth package found, call the routine,


    EnterCriticalSection( &SubAuthenticationCritSect );
    CritSectLocked = TRUE;

    //
    // Find the SubAuthentication Dll.
    //

    SubAuthenticationDll = ReferenceSubAuth (SubAuthRequest->SubAuthPackageId, &SubStatus);;


    if (SubStatus != STATUS_SUCCESS)
    {
        KdPrint(( "MSV1_0: SubAuth Error value is %ld.\n", SubStatus));
        Status = SubStatus;
        goto Cleanup;
    }

    //
    // Leave the crit sect while calling the DLL
    //

    SubAuthenticationRoutineGeneric = SubAuthenticationDll->SubAuthenticationRoutineGeneric;
    LeaveCriticalSection( &SubAuthenticationCritSect );
    CritSectLocked = FALSE;

    if (SubAuthenticationRoutineGeneric == NULL)
    {
        Status = STATUS_PROCEDURE_NOT_FOUND;
        goto Cleanup;
    }
    Status = (*SubAuthenticationRoutineGeneric)(
                           (PVOID) SubAuthRequest->SubAuthSubmitBuffer,
                           SubAuthRequest->SubAuthInfoLength,
                           &ReturnDataLength,
                           &ReturnDataBuffer);

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Allocate a buffer to return to the caller.
    //

    *ReturnBufferSize = sizeof(MSV1_0_SUBAUTH_RESPONSE) +
                        ReturnDataLength;

    Status = NlpAllocateClientBuffer( &ClientBufferDesc,
                                      sizeof(MSV1_0_SUBAUTH_RESPONSE),
                                      *ReturnBufferSize );


    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }

    SubAuthResponse = (PMSV1_0_SUBAUTH_RESPONSE) ClientBufferDesc.MsvBuffer;

    //
    // Fill in the return buffer.
    //

    SubAuthResponse->MessageType = MsV1_0SubAuth;
    SubAuthResponse->SubAuthInfoLength = ReturnDataLength;

    if (ReturnDataLength > 0)
    {
        SubAuthResponse->SubAuthReturnBuffer = ClientBufferDesc.UserBuffer + sizeof(MSV1_0_SUBAUTH_RESPONSE);

        if (ReturnDataBuffer)
        {
            RtlCopyMemory(
                SubAuthResponse + 1,
                ReturnDataBuffer,
                ReturnDataLength
                );

            // Make relative pointers
            SubAuthResponse->SubAuthReturnBuffer = (PUCHAR) sizeof(MSV1_0_SUBAUTH_RESPONSE);
        }
        else
        {
            SubAuthResponse->SubAuthReturnBuffer = NULL;
            SubStatus = STATUS_NO_MEMORY;
        }
    }
    else
    {
        SubAuthResponse->SubAuthReturnBuffer = 0;
    }


    //
    // Flush the buffer to the client's address space.
    //

    Status = NlpFlushClientBuffer( &ClientBufferDesc,
                                   ProtocolReturnBuffer );


Cleanup:

    if (ReturnDataBuffer != NULL) {
        MIDL_user_free(ReturnDataBuffer);
    }

    if ( !NT_SUCCESS(Status)) {
        NlpFreeClientBuffer( &ClientBufferDesc );
    }

    if ( CritSectLocked ) {
        LeaveCriticalSection( &SubAuthenticationCritSect );
    }
    *ProtocolStatus = SubStatus;
    return(Status);

}
