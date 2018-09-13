/*++

Copyright (c) 1987-1994  Microsoft Corporation

Module Name:

    subauth.c

Abstract:

    Sample SubAuthentication Package.

Author:

    Cliff Van Dyke (cliffv) 23-May-1994

Revisions:

    Andy Herron (andyhe)    21-Jun-1994  Added code to read domain/user info

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/


#if ( _MSC_VER >= 800 )
#pragma warning ( 3 : 4100 ) // enable "Unreferenced formal parameter"
#pragma warning ( 3 : 4219 ) // enable "trailing ',' used for variable argument list"
#endif

#define WIN32_NO_STATUS
#include <windef.h>
#undef WIN32_NO_STATUS
#include <windows.h>
#include <ntddk.h>
#include <ntdef.h>
#include <ntsam.h>
#include <ntstatus.h>

#include <crypt.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <logonmsv.h>


BOOLEAN
RtlEqualComputerName(
    IN PUNICODE_STRING String1,
    IN PUNICODE_STRING String2
    );

NTSTATUS
QuerySystemTime (
    OUT PLARGE_INTEGER SystemTime
    );


BOOL
GetPasswordExpired(
    IN LARGE_INTEGER PasswordLastSet,
    IN LARGE_INTEGER MaxPasswordAge
    );

NTSTATUS
AccountRestrictions(
    IN ULONG UserRid,
    IN PUNICODE_STRING LogonWorkStation,
    IN PUNICODE_STRING WorkStations,
    IN PLOGON_HOURS LogonHours,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
    );

LARGE_INTEGER
NetpSecondsToDeltaTime(
    IN ULONG Seconds
    );



NTSTATUS
Msv1_0SubAuthenticationRoutine (
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

    The subauthentication routine does cient/server specific authentication
    of a user.  The credentials of the user are passed in addition to all the
    information from SAM defining the user.  This routine decides whether to
    let the user logon.


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
    ULONG UserAccountControl;
    LARGE_INTEGER LogonTime;
    LARGE_INTEGER PasswordDateSet;

    PNETLOGON_NETWORK_INFO LogonNetworkInfo;


    //
    // Check whether the SubAuthentication package supports this type
    //  of logon.
    //

    *Authoritative = TRUE;
    *UserFlags = 0;
    *WhichFields = 0;

    (VOID) QuerySystemTime( &LogonTime );

    switch ( LogonLevel ) {
    case NetlogonInteractiveInformation:
    case NetlogonServiceInformation:

        //
        // This SubAuthentication package only supports network logons.
        //

        return STATUS_INVALID_INFO_CLASS;

    case NetlogonNetworkInformation:

        //
        // This SubAuthentication package doesn't support access via machine
        // accounts.
        //

        UserAccountControl = USER_NORMAL_ACCOUNT;

        //
        // Local user (Temp Duplicate) accounts are only used on the machine
        // being directly logged onto.
        // (Nor are interactive or service logons allowed to them.)
        //

        if ( (Flags & MSV1_0_PASSTHRU) == 0 ) {
            UserAccountControl |= USER_TEMP_DUPLICATE_ACCOUNT;
        }

        LogonNetworkInfo = (PNETLOGON_NETWORK_INFO) LogonInformation;

        break;

    default:
        *Authoritative = TRUE;
        return STATUS_INVALID_INFO_CLASS;
    }




    //
    // If the account type isn't allowed,
    //  Treat this as though the User Account doesn't exist.
    //

    if ( (UserAccountControl & UserAll->UserAccountControl) == 0 ) {
        *Authoritative = FALSE;
        Status = STATUS_NO_SUCH_USER;
        goto Cleanup;
    }

    //
    // This SubAuthentication package doesn't allow guest logons.
    //
    if ( Flags & MSV1_0_GUEST_LOGON ) {
        *Authoritative = FALSE;
        Status = STATUS_NO_SUCH_USER;
        goto Cleanup;
    }



    //
    // Ensure the account isn't locked out.
    //

    if ( UserAll->UserId != DOMAIN_USER_RID_ADMIN &&
         (UserAll->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED) ) {

        //
        // Since the UI strongly encourages admins to disable user
        // accounts rather than delete them.  Treat disabled acccount as
        // non-authoritative allowing the search to continue for other
        // accounts by the same name.
        //
        if ( UserAll->UserAccountControl & USER_ACCOUNT_DISABLED ) {
            *Authoritative = FALSE;
        } else {
            *Authoritative = TRUE;
        }
        Status = STATUS_ACCOUNT_LOCKED_OUT;
        goto Cleanup;
    }


    //
    // Check the password.
    //

    if ( FALSE /* VALIDATE THE USER'S PASSWORD HERE */ ) {

        Status = STATUS_WRONG_PASSWORD;

        //
        // Since the UI strongly encourages admins to disable user
        // accounts rather than delete them.  Treat disabled acccount as
        // non-authoritative allowing the search to continue for other
        // accounts by the same name.
        //
        if ( UserAll->UserAccountControl & USER_ACCOUNT_DISABLED ) {
            *Authoritative = FALSE;
        } else {
            *Authoritative = TRUE;
        }

        goto Cleanup;
    }

    //
    // Prevent some things from effecting the Administrator user
    //

    if (UserAll->UserId == DOMAIN_USER_RID_ADMIN) {

        //
        //  The administrator account doesn't have a forced logoff time.
        //

        LogoffTime->HighPart = 0x7FFFFFFF;
        LogoffTime->LowPart = 0xFFFFFFFF;

        KickoffTime->HighPart = 0x7FFFFFFF;
        KickoffTime->LowPart = 0xFFFFFFFF;

    } else {

        //
        // Check if the account is disabled.
        //

        if ( UserAll->UserAccountControl & USER_ACCOUNT_DISABLED ) {
            //
            // Since the UI strongly encourages admins to disable user
            // accounts rather than delete them.  Treat disabled acccount as
            // non-authoritative allowing the search to continue for other
            // accounts by the same name.
            //
            *Authoritative = FALSE;
            Status = STATUS_ACCOUNT_DISABLED;
            goto Cleanup;
        }

        //
        // Check if the account has expired.
        //

        if ( UserAll->AccountExpires.QuadPart != 0 &&
             LogonTime.QuadPart >= UserAll->AccountExpires.QuadPart ) {
            *Authoritative = TRUE;
            Status = STATUS_ACCOUNT_EXPIRED;
            goto Cleanup;
        }

#if 0

    //
    //  If your using SAM's password expiration date, use this code, otherwise
    //  use the code below and supply your own password set date...
    //

        //
        // The password is valid, check to see if the password is expired.
        //  (SAM will have appropriately set PasswordMustChange to reflect
        //  USER_DONT_EXPIRE_PASSWORD)
        //
        // If the password checked above is not the SAM password, you may
        // want to consider not checking the SAM password expiration times here.
        //

        if ( LogonTime.QuadPart >= UserAll->PasswordMustChange.QuadPart ) {

            if ( UserAll->PasswordLastSet.QuadPart == 0 ) {
                Status = STATUS_PASSWORD_MUST_CHANGE;
            } else {
                Status = STATUS_PASSWORD_EXPIRED;
            }
            *Authoritative = TRUE;
            goto Cleanup;
        }

#else

        //
        // Response is correct. So, check if the password has expired or not
        //

        if (! (UserAll->UserAccountControl & USER_DONT_EXPIRE_PASSWORD)) {
            LARGE_INTEGER MaxPasswordAge;
            MaxPasswordAge.HighPart = 0x7FFFFFFF;
            MaxPasswordAge.LowPart = 0xFFFFFFFF;

            //
            // PasswordDateSet should be modified to hold the last date the
            // user's password was set.
            //

            PasswordDateSet.LowPart = 0;
            PasswordDateSet.HighPart = 0;

            if ( GetPasswordExpired( PasswordDateSet,
                        MaxPasswordAge )) {

                Status = STATUS_PASSWORD_EXPIRED;
                goto Cleanup;
            }
        }

#endif


#if 0

    //
    // Validate the workstation the user logged on from.
    //
    // Ditch leading \\ on workstation name before passing it to SAM.
    //

    LocalWorkstation = LogonInfo->Workstation;
    if ( LocalWorkstation.Length > 0 &&
         LocalWorkstation.Buffer[0] == L'\\' &&
         LocalWorkstation.Buffer[1] == L'\\' ) {
        LocalWorkstation.Buffer += 2;
        LocalWorkstation.Length -= 2*sizeof(WCHAR);
        LocalWorkstation.MaximumLength -= 2*sizeof(WCHAR);
    }


    //
    //  To validate the user's logon hours as SAM does it, use this code,
    //  otherwise, supply your own checks below this code.
    //

    Status = AccountRestrictions( UserAll->UserId,
                                  &LocalWorkstation,
                                  (PUNICODE_STRING) &UserAll->WorkStations,
                                  &UserAll->LogonHours,
                                  LogoffTime,
                                  KickoffTime );

    if ( !NT_SUCCESS( Status )) {
        goto Cleanup;
    }

#else

        //
        // Validate the user's logon hours.
        //

        if ( TRUE /* VALIDATE THE LOGON HOURS */ ) {


            //
            // All times are allowed, so there's no logoff
            // time.  Return forever for both logofftime and
            // kickofftime.
            //

            LogoffTime->HighPart = 0x7FFFFFFF;
            LogoffTime->LowPart = 0xFFFFFFFF;

            KickoffTime->HighPart = 0x7FFFFFFF;
            KickoffTime->LowPart = 0xFFFFFFFF;
        } else {
            Status = STATUS_INVALID_LOGON_HOURS;
            *Authoritative = TRUE;
            goto Cleanup;
        }
#endif

        //
        // Validate if the user can logon from this workstation.
        //  (Supply subauthentication package specific code here.)

        if ( LogonNetworkInfo->Identity.Workstation.Buffer == NULL ) {
            Status = STATUS_INVALID_WORKSTATION;
            *Authoritative = TRUE;
            goto Cleanup;
        }
    }


    //
    // The user is valid.
    //

    *Authoritative = TRUE;
    Status = STATUS_SUCCESS;

    //
    // Cleanup up before returning.
    //

Cleanup:

    return Status;

}  // Msv1_0SubAuthenticationRoutine



BOOL
GetPasswordExpired (
    IN LARGE_INTEGER PasswordLastSet,
    IN LARGE_INTEGER MaxPasswordAge
    )

/*++

Routine Description:

    This routine returns true if the password is expired, false otherwise.

Arguments:

    PasswordLastSet - Time when the password was last set for this user.

    MaxPasswordAge - Maximum password age for any password in the domain.

Return Value:

    Returns true if password is expired.  False if not expired.

--*/
{
    LARGE_INTEGER PasswordMustChange;
    NTSTATUS Status;
    BOOLEAN rc;
    LARGE_INTEGER TimeNow;

    //
    // Compute the expiration time as the time the password was
    // last set plus the maximum age.
    //

    if ( PasswordLastSet.QuadPart < 0 || MaxPasswordAge.QuadPart > 0 ) {

        rc = TRUE;      // default for invalid times is that it is expired.

    } else {

        try {

            PasswordMustChange.QuadPart =
                PasswordLastSet.QuadPart - MaxPasswordAge.QuadPart;
            //
            // Limit the resultant time to the maximum valid absolute time
            //

            if ( PasswordMustChange.QuadPart < 0 ) {

                rc = FALSE;

            } else {

                Status = QuerySystemTime( &TimeNow );
                if (NT_SUCCESS(Status)) {

                    if ( TimeNow.QuadPart >= PasswordMustChange.QuadPart ) {
                        rc = TRUE;

                    } else {

                        rc = FALSE;
                    }
                } else {
                    rc = FALSE;     // won't fail if QuerySystemTime failed.
                }
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {

            rc = TRUE;
        }
    }

    return rc;

}  // GetPasswordExpired


NTSTATUS
QuerySystemTime (
    OUT PLARGE_INTEGER SystemTime
    )

/*++

Routine Description:

    This function returns the absolute system time. The time is in units of
    100nsec ticks since the base time which is midnight January 1, 1601.

Arguments:

    SystemTime - Supplies the address of a variable that will receive the
        current system time.

Return Value:

    STATUS_SUCCESS is returned if the service is successfully executed.

    STATUS_ACCESS_VIOLATION is returned if the output parameter for the
        system time cannot be written.

--*/

{
    SYSTEMTIME CurrentTime;

    GetSystemTime( &CurrentTime );

    if ( !SystemTimeToFileTime( &CurrentTime, (LPFILETIME) SystemTime ) ) {
        return STATUS_ACCESS_VIOLATION;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
SampMatchworkstation(
    IN PUNICODE_STRING LogonWorkStation,
    IN PUNICODE_STRING WorkStations
    )

/*++

Routine Description:

    Check if the given workstation is a member of the list of workstations
    given.


Arguments:

    LogonWorkStations - UNICODE name of the workstation that the user is
        trying to log into.

    WorkStations - API list of workstations that the user is allowed to
        log into.


Return Value:


    STATUS_SUCCESS - The user is allowed to log into the workstation.



--*/
{
    PWCHAR          WorkStationName;
    UNICODE_STRING  Unicode;
    NTSTATUS        NtStatus;
    WCHAR           Buffer[256];
    USHORT          LocalBufferLength = 256;
    UNICODE_STRING  WorkStationsListCopy;
    BOOLEAN         BufferAllocated = FALSE;
    PWCHAR          TmpBuffer;

    //
    // Local workstation is always allowed
    // If WorkStations field is 0 everybody is allowed
    //

    if ( ( LogonWorkStation == NULL ) ||
        ( LogonWorkStation->Length == 0 ) ||
        ( WorkStations->Length == 0 ) ) {

        return( STATUS_SUCCESS );
    }

    //
    // Assume failure; change status only if we find the string.
    //

    NtStatus = STATUS_INVALID_WORKSTATION;

    //
    // WorkStationApiList points to our current location in the list of
    // WorkStations.
    //

    if ( WorkStations->Length > LocalBufferLength ) {

        WorkStationsListCopy.Buffer = LocalAlloc( 0, WorkStations->Length );
        BufferAllocated = TRUE;

        if ( WorkStationsListCopy.Buffer == NULL ) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            return( NtStatus );
        }

        WorkStationsListCopy.MaximumLength = WorkStations->Length;

    } else {

        WorkStationsListCopy.Buffer = Buffer;
        WorkStationsListCopy.MaximumLength = LocalBufferLength;
    }

    RtlCopyUnicodeString( &WorkStationsListCopy, WorkStations );
    ASSERT( WorkStationsListCopy.Length == WorkStations->Length );

    //
    // wcstok requires a string the first time it's called, and NULL
    // for all subsequent calls.  Use a temporary variable so we
    // can do this.
    //

    TmpBuffer = WorkStationsListCopy.Buffer;

    while( WorkStationName = wcstok(TmpBuffer, L",") ) {

        TmpBuffer = NULL;
        RtlInitUnicodeString( &Unicode, WorkStationName );
        if (RtlEqualComputerName( &Unicode, LogonWorkStation )) {
            NtStatus = STATUS_SUCCESS;
            break;
        }
    }

    if ( BufferAllocated ) {
        LocalFree( WorkStationsListCopy.Buffer );
    }

    return( NtStatus );
}

NTSTATUS
AccountRestrictions(
    IN ULONG UserRid,
    IN PUNICODE_STRING LogonWorkStation,
    IN PUNICODE_STRING WorkStations,
    IN PLOGON_HOURS LogonHours,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
    )

/*++

Routine Description:

    Validate a user's ability to logon at this time and at the workstation
    being logged onto.


Arguments:

    UserRid - The user id of the user to operate on.

    LogonWorkStation - The name of the workstation the logon is being
        attempted at.

    WorkStations - The list of workstations the user may logon to.  This
        information comes from the user's account information.  It must
        be in API list format.

    LogonHours - The times the user may logon.  This information comes
        from the user's account information.

    LogoffTime - Receives the time at which the user should logoff the
        system.

    KickoffTime - Receives the time at which the user should be kicked
        off the system.


Return Value:


    STATUS_SUCCESS - Logon is permitted.

    STATUS_INVALID_LOGON_HOURS - The user is not authorized to logon at
        this time.

    STATUS_INVALID_WORKSTATION - The user is not authorized to logon to
        the specified workstation.


--*/
{

    static BOOLEAN GetForceLogoff = TRUE;
    static LARGE_INTEGER ForceLogoff = { 0x7fffffff, 0xFFFFFFF};

#define MILLISECONDS_PER_WEEK 7 * 24 * 60 * 60 * 1000

    SYSTEMTIME              CurrentTimeFields;
    LARGE_INTEGER           CurrentTime, CurrentUTCTime;
    LARGE_INTEGER           MillisecondsIntoWeekXUnitsPerWeek;
    LARGE_INTEGER           LargeUnitsIntoWeek;
    LARGE_INTEGER           Delta100Ns;
    NTSTATUS                NtStatus = STATUS_SUCCESS;
    ULONG                   CurrentMsIntoWeek;
    ULONG                   LogoffMsIntoWeek;
    ULONG                   DeltaMs;
    ULONG                   MillisecondsPerUnit;
    ULONG                   CurrentUnitsIntoWeek;
    ULONG                   LogoffUnitsIntoWeek;
    USHORT                  i;
    TIME_ZONE_INFORMATION   TimeZoneInformation;
    DWORD TimeZoneId;
    LARGE_INTEGER           BiasIn100NsUnits;
    LONG                    BiasInMinutes;



    //
    // Only check for users other than the builtin ADMIN
    //

    if ( UserRid != DOMAIN_USER_RID_ADMIN) {

        //
        // Scan to make sure the workstation being logged into is in the
        // list of valid workstations - or if the list of valid workstations
        // is null, which means that all are valid.
        //

        NtStatus = SampMatchworkstation( LogonWorkStation, WorkStations );

        if ( NT_SUCCESS( NtStatus ) ) {

            //
            // Check to make sure that the current time is a valid time to logon
            // in the LogonHours.
            //
            // We need to validate the time taking into account whether we are
            // in daylight savings time or standard time.  Thus, if the logon
            // hours specify that we are able to logon between 9am and 5pm,
            // this means 9am to 5pm standard time during the standard time
            // period, and 9am to 5pm daylight savings time when in the
            // daylight savings time.  Since the logon hours stored by SAM are
            // independent of daylight savings time, we need to add in the
            // difference between standard time and daylight savings time to
            // the current time before checking whether this time is a valid
            // time to logon.  Since this difference (or bias as it is called)
            // is actually held in the form
            //
            // Standard time = Daylight savings time + Bias
            //
            // the Bias is a negative number.  Thus we actually subtract the
            // signed Bias from the Current Time.

            //
            // First, get the Time Zone Information.
            //

            TimeZoneId = GetTimeZoneInformation(
                             (LPTIME_ZONE_INFORMATION) &TimeZoneInformation
                             );

            //
            // Next, get the appropriate bias (signed integer in minutes) to subtract from
            // the Universal Time Convention (UTC) time returned by NtQuerySystemTime
            // to get the local time.  The bias to be used depends whether we're
            // in Daylight Savings time or Standard Time as indicated by the
            // TimeZoneId parameter.
            //
            // local time  = UTC time - bias in 100Ns units
            //

            switch (TimeZoneId) {

            case TIME_ZONE_ID_UNKNOWN:

                //
                // There is no differentiation between standard and
                // daylight savings time.  Proceed as for Standard Time
                //

                BiasInMinutes = TimeZoneInformation.StandardBias;
                break;

            case TIME_ZONE_ID_STANDARD:

                BiasInMinutes = TimeZoneInformation.StandardBias;
                break;

            case TIME_ZONE_ID_DAYLIGHT:

                BiasInMinutes = TimeZoneInformation.DaylightBias;
                break;

            default:

                //
                // Something is wrong with the time zone information.  Fail
                // the logon request.
                //

                NtStatus = STATUS_INVALID_LOGON_HOURS;
                break;
            }

            if (NT_SUCCESS(NtStatus)) {

                //
                // Convert the Bias from minutes to 100ns units
                //

                BiasIn100NsUnits.QuadPart = ((LONGLONG)BiasInMinutes)
                                            * 60 * 10000000;

                //
                // Get the UTC time in 100Ns units used by Windows Nt.  This
                // time is GMT.
                //

                NtStatus = QuerySystemTime( &CurrentUTCTime );
            }

            if ( NT_SUCCESS( NtStatus ) ) {

                CurrentTime.QuadPart = CurrentUTCTime.QuadPart -
                              BiasIn100NsUnits.QuadPart;

                FileTimeToSystemTime( (PFILETIME)&CurrentTime, &CurrentTimeFields );

                CurrentMsIntoWeek = (((( CurrentTimeFields.wDayOfWeek * 24 ) +
                                       CurrentTimeFields.wHour ) * 60 +
                                       CurrentTimeFields.wMinute ) * 60 +
                                       CurrentTimeFields.wSecond ) * 1000 +
                                       CurrentTimeFields.wMilliseconds;

                MillisecondsIntoWeekXUnitsPerWeek.QuadPart =
                    ((LONGLONG)CurrentMsIntoWeek) *
                    ((LONGLONG)LogonHours->UnitsPerWeek);

                LargeUnitsIntoWeek = RtlExtendedLargeIntegerDivide(
                                         MillisecondsIntoWeekXUnitsPerWeek,
                                         MILLISECONDS_PER_WEEK,
                                         (PULONG)NULL );

                CurrentUnitsIntoWeek = LargeUnitsIntoWeek.LowPart;

                if ( !( LogonHours->LogonHours[ CurrentUnitsIntoWeek / 8] &
                    ( 0x01 << ( CurrentUnitsIntoWeek % 8 ) ) ) ) {

                    NtStatus = STATUS_INVALID_LOGON_HOURS;

                } else {

                    //
                    // Determine the next time that the user is NOT supposed to be logged
                    // in, and return that as LogoffTime.
                    //

                    i = 0;
                    LogoffUnitsIntoWeek = CurrentUnitsIntoWeek;

                    do {

                        i++;

                        LogoffUnitsIntoWeek = ( LogoffUnitsIntoWeek + 1 ) % LogonHours->UnitsPerWeek;

                    } while ( ( i <= LogonHours->UnitsPerWeek ) &&
                        ( LogonHours->LogonHours[ LogoffUnitsIntoWeek / 8 ] &
                        ( 0x01 << ( LogoffUnitsIntoWeek % 8 ) ) ) );

                    if ( i > LogonHours->UnitsPerWeek ) {

                        //
                        // All times are allowed, so there's no logoff
                        // time.  Return forever for both logofftime and
                        // kickofftime.
                        //

                        LogoffTime->HighPart = 0x7FFFFFFF;
                        LogoffTime->LowPart = 0xFFFFFFFF;

                        KickoffTime->HighPart = 0x7FFFFFFF;
                        KickoffTime->LowPart = 0xFFFFFFFF;

                    } else {

                        //
                        // LogoffUnitsIntoWeek points at which time unit the
                        // user is to log off.  Calculate actual time from
                        // the unit, and return it.
                        //
                        // CurrentTimeFields already holds the current
                        // time for some time during this week; just adjust
                        // to the logoff time during this week and convert
                        // to time format.
                        //

                        MillisecondsPerUnit = MILLISECONDS_PER_WEEK / LogonHours->UnitsPerWeek;

                        LogoffMsIntoWeek = MillisecondsPerUnit * LogoffUnitsIntoWeek;

                        if ( LogoffMsIntoWeek < CurrentMsIntoWeek ) {

                            DeltaMs = MILLISECONDS_PER_WEEK - ( CurrentMsIntoWeek - LogoffMsIntoWeek );

                        } else {

                            DeltaMs = LogoffMsIntoWeek - CurrentMsIntoWeek;
                        }

                        Delta100Ns = RtlExtendedIntegerMultiply(
                                         RtlConvertUlongToLargeInteger( DeltaMs ),
                                         10000
                                         );

                        LogoffTime->QuadPart = CurrentUTCTime.QuadPart +
                                      Delta100Ns.QuadPart;

                        //
                        // Grab the domain's ForceLogoff time.
                        //

                        if ( GetForceLogoff ) {
                            NET_API_STATUS NetStatus;
                            LPUSER_MODALS_INFO_0 UserModals0;

                            NetStatus = NetUserModalsGet( NULL,
                                                          0,
                                                          (LPBYTE *)&UserModals0 );

                            if ( NetStatus == 0 ) {
                                GetForceLogoff = FALSE;

                                ForceLogoff = NetpSecondsToDeltaTime( UserModals0->usrmod0_force_logoff );

                                NetApiBufferFree( UserModals0 );
                            }
                        }
                        //
                        // Subtract Domain->ForceLogoff from LogoffTime, and return
                        // that as KickoffTime.  Note that Domain->ForceLogoff is a
                        // negative delta.  If its magnitude is sufficiently large
                        // (in fact, larger than the difference between LogoffTime
                        // and the largest positive large integer), we'll get overflow
                        // resulting in a KickOffTime that is negative.  In this
                        // case, reset the KickOffTime to this largest positive
                        // large integer (i.e. "never") value.
                        //


                        KickoffTime->QuadPart = LogoffTime->QuadPart - ForceLogoff.QuadPart;

                        if (KickoffTime->QuadPart < 0) {

                            KickoffTime->HighPart = 0x7FFFFFFF;
                            KickoffTime->LowPart = 0xFFFFFFFF;
                        }
                    }
                }
            }
        }

    } else {

        //
        // Never kick administrators off
        //

        LogoffTime->HighPart  = 0x7FFFFFFF;
        LogoffTime->LowPart   = 0xFFFFFFFF;
        KickoffTime->HighPart = 0x7FFFFFFF;
        KickoffTime->LowPart  = 0xFFFFFFFF;
    }


    return( NtStatus );
}

LARGE_INTEGER
NetpSecondsToDeltaTime(
    IN ULONG Seconds
    )

/*++

Routine Description:

    Convert a number of seconds to an NT delta time specification

Arguments:

    Seconds - a positive number of seconds

Return Value:

    Returns the NT Delta time.  NT delta time is a negative number
        of 100ns units.

--*/

{
    LARGE_INTEGER DeltaTime;
    LARGE_INTEGER LargeSeconds;
    LARGE_INTEGER Answer;

    //
    // Special case TIMEQ_FOREVER (return a full scale negative)
    //

    if ( Seconds == TIMEQ_FOREVER ) {
        DeltaTime.LowPart = 0;
        DeltaTime.HighPart = (LONG) 0x80000000;

    //
    // Convert seconds to 100ns units simply by multiplying by 10000000.
    //
    // Convert to delta time by negating.
    //

    } else {

        LargeSeconds = RtlConvertUlongToLargeInteger( Seconds );

        Answer = RtlExtendedIntegerMultiply( LargeSeconds, 10000000 );

          if ( Answer.QuadPart < 0 ) {
            DeltaTime.LowPart = 0;
            DeltaTime.HighPart = (LONG) 0x80000000;
        } else {
            DeltaTime.QuadPart = -Answer.QuadPart;
        }

    }

    return DeltaTime;

} // NetpSecondsToDeltaTime

// subauth.c eof

