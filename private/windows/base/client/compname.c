/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    COMPNAME.C

Abstract:

    This module contains the GetComputerName and SetComputerName APIs.

Author:

    Dan Hinsley (DanHi)    2-Apr-1992


Revision History:


--*/

#include <basedll.h>

#include <dnsapi.h>

typedef DNS_STATUS
(WINAPI DNS_VALIDATE_NAME_FN)(
    IN LPCWSTR Name,
    IN DNS_NAME_FORMAT Format
    );
//
//


#define COMPUTERNAME_ROOT \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName"

#define NON_VOLATILE_COMPUTERNAME_NODE \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ComputerName"

#define VOLATILE_COMPUTERNAME_NODE \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName"

#define VOLATILE_COMPUTERNAME L"ActiveComputerName"
#define NON_VOLATILE_COMPUTERNAME L"ComputerName"
#define COMPUTERNAME_VALUE_NAME L"ComputerName"
#define CLASS_STRING L"Network ComputerName"

#define TCPIP_POLICY_ROOT \
        L"\\Registry\\Machine\\Software\\Policies\\Microsoft\\System\\DNSclient"

#define TCPIP_POLICY_DOMAINNAME \
        L"PrimaryDnsSuffix"

#define TCPIP_ROOT \
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters"

#define TCPIP_HOSTNAME \
        L"Hostname"

#define TCPIP_NV_HOSTNAME \
        L"NV Hostname"

#define TCPIP_DOMAINNAME \
        L"Domain"

#define TCPIP_NV_DOMAINNAME \
        L"NV Domain"


//
// Allow the cluster guys to override the returned
// names with their own virtual names
//

const PWSTR ClusterNameVars[] = {
                L"_CLUSTER_NETWORK_NAME_",
                L"_CLUSTER_NETWORK_HOSTNAME_",
                L"_CLUSTER_NETWORK_DOMAIN_",
                L"_CLUSTER_NETWORK_FQDN_"
                };

//
// Disallowed control characters (not including \0)
//

#define CTRL_CHARS_0       L"\001\002\003\004\005\006\007"
#define CTRL_CHARS_1   L"\010\011\012\013\014\015\016\017"
#define CTRL_CHARS_2   L"\020\021\022\023\024\025\026\027"
#define CTRL_CHARS_3   L"\030\031\032\033\034\035\036\037"

#define CTRL_CHARS_STR CTRL_CHARS_0 CTRL_CHARS_1 CTRL_CHARS_2 CTRL_CHARS_3

//
// Combinations of the above
//

#define ILLEGAL_NAME_CHARS_STR  L"\"/\\[]:|<>+=;,?" CTRL_CHARS_STR


//
// Worker routine
//

NTSTATUS
GetNameFromValue(
    HANDLE hKey,
    LPWSTR SubKeyName,
    LPWSTR ValueValue,
    LPDWORD nSize
    )

/*++

Routine Description:

  This returns the value of "ComputerName" value entry under the subkey
  SubKeyName relative to hKey.  This is used to get the value of the
  ActiveComputerName or ComputerName values.


Arguments:

    hKey       - handle to the Key the SubKey exists under

    SubKeyName - name of the subkey to look for the value under

    ValueValue - where the value of the value entry will be returned

    nSize      - pointer to the size (in characters) of the ValueValue buffer

Return Value:


--*/
{

#define VALUE_BUFFER_SIZE (sizeof(KEY_VALUE_FULL_INFORMATION) + \
    (sizeof( COMPUTERNAME_VALUE_NAME ) + MAX_COMPUTERNAME_LENGTH + 1) * sizeof(WCHAR))

    NTSTATUS NtStatus;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hSubKey;
    BYTE ValueBuffer[VALUE_BUFFER_SIZE];
    PKEY_VALUE_FULL_INFORMATION pKeyValueInformation = (PVOID) ValueBuffer;
    DWORD ValueLength;
    PWCHAR pTerminator;

    //
    // Open the node for the Subkey
    //

    RtlInitUnicodeString(&KeyName, SubKeyName);

    InitializeObjectAttributes(&ObjectAttributes,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE,
                              hKey,
                              NULL
                              );

    NtStatus = NtOpenKey(&hSubKey, KEY_READ, &ObjectAttributes);

    if (NT_SUCCESS(NtStatus)) {

        RtlInitUnicodeString(&ValueName, COMPUTERNAME_VALUE_NAME);

        NtStatus = NtQueryValueKey(hSubKey,
                                   &ValueName,
                                   KeyValueFullInformation,
                                   pKeyValueInformation,
                                   VALUE_BUFFER_SIZE,
                                   &ValueLength);

        NtClose(hSubKey);

        if (NT_SUCCESS(NtStatus)) {

            //
            // If the user's buffer is big enough, move it in
            // First see if it's null terminated.  If it is, pretend like
            // it's not.
            //

            pTerminator = (PWCHAR)((PBYTE) pKeyValueInformation +
                pKeyValueInformation->DataOffset +
                pKeyValueInformation->DataLength);
            pTerminator--;

            if (*pTerminator == L'\0') {
               pKeyValueInformation->DataLength -= sizeof(WCHAR);
            }

            if (*nSize >= pKeyValueInformation->DataLength/sizeof(WCHAR) + 1) {
               //
               // This isn't guaranteed to be NULL terminated, make it so
               //
                    RtlCopyMemory(ValueValue,
                        (LPWSTR)((PBYTE) pKeyValueInformation +
                        pKeyValueInformation->DataOffset),
                        pKeyValueInformation->DataLength);

                    pTerminator = (PWCHAR) ((PBYTE) ValueValue +
                        pKeyValueInformation->DataLength);
                    *pTerminator = L'\0';

                    //
                    // Return the number of characters to the caller
                    //

                    *nSize = wcslen(ValueValue);
            }
            else {
                NtStatus = STATUS_BUFFER_OVERFLOW;
                *nSize = pKeyValueInformation->DataLength/sizeof(WCHAR) + 1;
            }

        }
    }

    return(NtStatus);
}


//
// UNICODE APIs
//

BOOL
WINAPI
GetComputerNameW (
    LPWSTR lpBuffer,
    LPDWORD nSize
    )

/*++

Routine Description:

  This returns the active computername.  This is the computername when the
  system was last booted.  If this is changed (via SetComputerName) it does
  not take effect until the next system boot.


Arguments:

    lpBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the computer name.

    nSize - Specifies the maximum size (in characters) of the buffer.  This
        value should be set to at least MAX_COMPUTERNAME_LENGTH + 1 to allow
        sufficient room in the buffer for the computer name.  The length
        of the string is returned in nSize.

Return Value:

    TRUE on success, FALSE on failure.


--*/
{

    NTSTATUS NtStatus;
    UNICODE_STRING KeyName;
    UNICODE_STRING Class;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hKey = NULL;
    HANDLE hNewKey = NULL;
    ULONG Disposition;
    ULONG ValueLength;
    BOOL ReturnValue;
    DWORD Status;
    DWORD errcode;

    //
    // First check to see if the cluster computername variable is set.
    // If so, this overrides the actual computername to fool the application
    // into working when its network name and computer name are different.
    //

    ValueLength = GetEnvironmentVariableW(L"_CLUSTER_NETWORK_NAME_",
                                          lpBuffer,
                                          *nSize);
    if (ValueLength != 0) {
        //
        // The environment variable exists, return it directly
        //
        *nSize = ValueLength;
        return(TRUE);
    }


    if ( (gpTermsrvGetComputerName) &&
            ((errcode =  gpTermsrvGetComputerName(lpBuffer, nSize)) != ERROR_RETRY) ) {

        if (errcode == ERROR_BUFFER_OVERFLOW ) {
            ReturnValue = FALSE;
            goto Cleanup;

        } else {
            goto GoodReturn;
        }

    }

    //
    // Open the Computer node, both computername keys are relative
    // to this node.
    //

    RtlInitUnicodeString(&KeyName, COMPUTERNAME_ROOT);

    InitializeObjectAttributes(&ObjectAttributes,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL
                              );

    NtStatus = NtOpenKey(&hKey, KEY_READ, &ObjectAttributes);

    if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {

        //
        // This should never happen!  This key should have been created
        // at setup, and protected by an ACL so that only the ADMIN could
        // write to it.  Generate an event, and return a NULL computername.
        //

        //
        // BUGBUG - generate an alert/event/???
        //

        //
        // Return a NULL computername
        //

        if (ARGUMENT_PRESENT(lpBuffer))
        {
            lpBuffer[0] = L'\0';
        }
        *nSize = 0;
        goto GoodReturn;
    }

    if (!NT_SUCCESS(NtStatus)) {

        //
        // Some other error, return it to the caller
        //

        goto ErrorReturn;
    }

    //
    // Try to get the name from the volatile key
    //

    NtStatus = GetNameFromValue(hKey, VOLATILE_COMPUTERNAME, lpBuffer,
        nSize);

    //
    // The user's buffer wasn't big enough, just return the error.
    //

    if(NtStatus == STATUS_BUFFER_OVERFLOW) {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        ReturnValue = FALSE;
        goto Cleanup;
    }

    if (NT_SUCCESS(NtStatus)) {

        //
        // The volatile copy is already there, just return it
        //

        goto GoodReturn;
    }

    //
    // The volatile key isn't there, try for the non-volatile one
    //

    NtStatus = GetNameFromValue(hKey, NON_VOLATILE_COMPUTERNAME, lpBuffer,
        nSize);

    if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {

        //
        // This should never happen!  This value should have been created
        // at setup, and protected by an ACL so that only the ADMIN could
        // write to it.  Generate an event, and return an error to the
        // caller
        //

        //
        // BUGBUG - generate and alert/event/???
        //

        //
        // Return a NULL computername
        //

        lpBuffer[0] = L'\0';
        *nSize = 0;
        goto GoodReturn;
    }

    if (!NT_SUCCESS(NtStatus)) {

        //
        // Some other error, return it to the caller
        //

        goto ErrorReturn;
    }

    //
    // Now create the volatile key to "lock this in" until the next boot
    //

    RtlInitUnicodeString(&Class, CLASS_STRING);

    //
    // Turn KeyName into a UNICODE_STRING
    //

    RtlInitUnicodeString(&KeyName, VOLATILE_COMPUTERNAME);

    InitializeObjectAttributes(&ObjectAttributes,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE,
                              hKey,
                              NULL
                              );

    //
    // Now create the key
    //

    NtStatus = NtCreateKey(&hNewKey,
                         KEY_WRITE | KEY_READ,
                         &ObjectAttributes,
                         0,
                         &Class,
                         REG_OPTION_VOLATILE,
                         &Disposition);

    if (Disposition == REG_OPENED_EXISTING_KEY) {

        //
        // Someone beat us to this, just get the value they put there
        //

        NtStatus = GetNameFromValue(hKey, VOLATILE_COMPUTERNAME, lpBuffer,
           nSize);

        if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {

            //
            // This should never happen!  It just told me it existed
            //

            NtStatus = STATUS_UNSUCCESSFUL;
            goto ErrorReturn;
        }
    }

    //
    // Create the value under this key
    //

    RtlInitUnicodeString(&ValueName, COMPUTERNAME_VALUE_NAME);
    ValueLength = (wcslen(lpBuffer) + 1) * sizeof(WCHAR);
    NtStatus = NtSetValueKey(hNewKey,
                             &ValueName,
                             0,
                             REG_SZ,
                             lpBuffer,
                             ValueLength);

    if (!NT_SUCCESS(NtStatus)) {

        goto ErrorReturn;
    }

    goto GoodReturn;

ErrorReturn:

    //
    // An error was encountered, convert the status and return
    //

    BaseSetLastNTError(NtStatus);
    ReturnValue = FALSE;
    goto Cleanup;

GoodReturn:

    //
    // Everything went ok, update nSize with the length of the buffer and
    // return
    //

    *nSize = wcslen(lpBuffer);
    ReturnValue = TRUE;
    goto Cleanup;

Cleanup:

    if (hKey) {
        NtClose(hKey);
    }

    if (hNewKey) {
        NtClose(hNewKey);
    }

    return(ReturnValue);
}



BOOL
WINAPI
SetComputerNameW (
    LPCWSTR lpComputerName
    )

/*++

Routine Description:

  This sets what the computername will be when the system is next booted.  This
  does not effect the active computername for the remainder of this boot, nor
  what is returned by GetComputerName before the next system boot.


Arguments:

    lpComputerName - points to the buffer that is contains the
        null-terminated character string containing the computer name.

Return Value:

    Returns TRUE on success, FALSE on failure.


--*/
{

    NTSTATUS NtStatus;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hKey = NULL;
    ULONG ValueLength;
    ULONG ComputerNameLength;
    ULONG AnsiComputerNameLength;

    //
    // Validate that the supplied computername is valid (not too long,
    // no incorrect characters, no leading or trailing spaces)
    //

    ComputerNameLength = wcslen(lpComputerName);

    //
    // The name length limitation should be based on ANSI. (LanMan compatibility)
    //

    NtStatus = RtlUnicodeToMultiByteSize(&AnsiComputerNameLength,
                                         (LPWSTR)lpComputerName,
                                         ComputerNameLength * sizeof(WCHAR));

    if ((!NT_SUCCESS(NtStatus)) ||
        (AnsiComputerNameLength == 0 )||(AnsiComputerNameLength > MAX_COMPUTERNAME_LENGTH)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //
    // Check for illegal characters; return an error if one is found
    //

    if (wcscspn(lpComputerName, ILLEGAL_NAME_CHARS_STR) < ComputerNameLength) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //
    // Check for leading or trailing spaces
    //

    if (lpComputerName[0] == L' ' ||
        lpComputerName[ComputerNameLength-1] == L' ') {
            SetLastError(ERROR_INVALID_PARAMETER);
            return(FALSE);

    }
    //
    // Open the ComputerName\ComputerName node
    //

    RtlInitUnicodeString(&KeyName, NON_VOLATILE_COMPUTERNAME_NODE);

    InitializeObjectAttributes(&ObjectAttributes,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL
                              );

    NtStatus = NtOpenKey(&hKey, KEY_READ | KEY_WRITE, &ObjectAttributes);

    if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {

        //
        // This should never happen!  This key should have been created
        // at setup, and protected by an ACL so that only the ADMIN could
        // write to it.  Generate an event, and return a NULL computername.
        //

        //
        // BUGBUG - generate and alert/event/???
        //

        //
        // Return an error to the user.  BUGBUG - should I just create the
        // node here?  Does it inherit the correct ACL's?
        //

        SetLastError(ERROR_GEN_FAILURE);
        return(FALSE);
    }

    //
    // Update the value under this key
    //

    RtlInitUnicodeString(&ValueName, COMPUTERNAME_VALUE_NAME);
    ValueLength = (wcslen(lpComputerName) + 1) * sizeof(WCHAR);
    NtStatus = NtSetValueKey(hKey,
                             &ValueName,
                             0,
                             REG_SZ,
                             (LPWSTR)lpComputerName,
                             ValueLength);

    if (!NT_SUCCESS(NtStatus)) {

        BaseSetLastNTError(NtStatus);
        NtClose(hKey);
        return(FALSE);
    }

    NtFlushKey(hKey);
    NtClose(hKey);
    return(TRUE);

}

NTSTATUS
BasepGetNameFromReg(
    PWSTR Path,
    PWSTR Value,
    PWSTR Buffer,
    PDWORD Length
    )
/*++

Routine Description:

  This routine gets a string from the value at the specified registry key.


Arguments:

  Path - Path to the registry key

  Value - Name of the value to retrieve

  Buffer - Buffer to return the value

  Length - size of the buffer in characters

Return Value:

  STATUS_SUCCESS, or various failures

--*/

#define REASONABLE_LENGTH 128

{
    NTSTATUS Status ;
    HANDLE Key ;
    OBJECT_ATTRIBUTES ObjA ;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;


    BYTE ValueBuffer[ REASONABLE_LENGTH ];
    PKEY_VALUE_FULL_INFORMATION pKeyValueInformation = (PVOID) ValueBuffer;
    BOOLEAN FreeBuffer = FALSE ;
    DWORD ValueLength;
    PWCHAR pTerminator;

    //
    // Open the node for the Subkey
    //

    RtlInitUnicodeString(&KeyName, Path );

    InitializeObjectAttributes(&ObjA,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL
                              );

    Status = NtOpenKey(&Key, KEY_READ, &ObjA );

    if (NT_SUCCESS(Status)) {

        RtlInitUnicodeString( &ValueName, Value );

        Status = NtQueryValueKey(Key,
                                   &ValueName,
                                   KeyValueFullInformation,
                                   pKeyValueInformation,
                                   REASONABLE_LENGTH ,
                                   &ValueLength);

        if ( Status == STATUS_BUFFER_OVERFLOW )
        {
            pKeyValueInformation = RtlAllocateHeap( RtlProcessHeap(),
                                                    0,
                                                    ValueLength );

            if ( pKeyValueInformation )
            {
                FreeBuffer = TRUE ;

                Status = NtQueryValueKey( Key,
                                          &ValueName,
                                          KeyValueFullInformation,
                                          pKeyValueInformation,
                                          ValueLength,
                                          &ValueLength );

            }
        }

        if ( NT_SUCCESS(Status) ) {

            //
            // If the user's buffer is big enough, move it in
            // First see if it's null terminated.  If it is, pretend like
            // it's not.
            //

            pTerminator = (PWCHAR)((PBYTE) pKeyValueInformation +
                pKeyValueInformation->DataOffset +
                pKeyValueInformation->DataLength);
            pTerminator--;

            if (*pTerminator == L'\0') {
               pKeyValueInformation->DataLength -= sizeof(WCHAR);
            }

            if ( ( *Length >= pKeyValueInformation->DataLength/sizeof(WCHAR) + 1) &&
                 ( Buffer != NULL ) ) {
               //
               // This isn't guaranteed to be NULL terminated, make it so
               //
                    RtlCopyMemory(Buffer,
                        (LPWSTR)((PBYTE) pKeyValueInformation +
                        pKeyValueInformation->DataOffset),
                        pKeyValueInformation->DataLength);

                    pTerminator = (PWCHAR) ((PBYTE) Buffer +
                        pKeyValueInformation->DataLength);
                    *pTerminator = L'\0';

                    //
                    // Return the number of characters to the caller
                    //

                    *Length = pKeyValueInformation->DataLength / sizeof(WCHAR) ;

            }
            else {
                Status = STATUS_BUFFER_OVERFLOW;
                *Length = pKeyValueInformation->DataLength/sizeof(WCHAR) + 1;
            }

        }

        NtClose( Key );
    }

    if ( FreeBuffer )
    {
        RtlFreeHeap( RtlProcessHeap(), 0, pKeyValueInformation );
    }

    return Status ;

}

NTSTATUS
BaseSetNameInReg(
    PCWSTR Path,
    PCWSTR Value,
    PCWSTR Buffer
    )
{
    NTSTATUS NtStatus;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hKey = NULL;
    ULONG ValueLength;

    //
    // Open the ComputerName\ComputerName node
    //

    RtlInitUnicodeString(&KeyName, Path);

    InitializeObjectAttributes(&ObjectAttributes,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL
                              );

    NtStatus = NtOpenKey(&hKey, KEY_READ | KEY_WRITE, &ObjectAttributes);

    if ( !NT_SUCCESS( NtStatus ) )
    {
        return NtStatus ;
    }

    //
    // Update the value under this key
    //

    RtlInitUnicodeString(&ValueName, Value);

    ValueLength = (wcslen( Buffer ) + 1) * sizeof(WCHAR);

    NtStatus = NtSetValueKey(hKey,
                             &ValueName,
                             0,
                             REG_SZ,
                             (LPWSTR) Buffer,
                             ValueLength);

    if ( NT_SUCCESS( NtStatus ) )
    {
        NtFlushKey( hKey );
    }

    NtClose(hKey);

    return NtStatus ;
}


BOOL
WINAPI
GetComputerNameExW(
    IN COMPUTER_NAME_FORMAT NameType,
    OUT LPWSTR lpBuffer,
    IN OUT LPDWORD nSize
    )

/*++

Routine Description:

  This returns the active computername in a particular format.  This is the
  computername when the system was last booted.  If this is changed (via
  SetComputerName) it does not take effect until the next system boot.


Arguments:

    NameType - Possible name formats to return the computer name in:

        ComputerNameNetBIOS - netbios name (compatible with GetComputerName)
        ComputerNameDnsHostname - DNS host name
        ComputerNameDnsDomain - DNS Domain name
        ComputerNameDnsFullyQualified - DNS Fully Qualified (hostname.dnsdomain)

    lpBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the computer name.

    nSize - Specifies the maximum size (in characters) of the buffer.  This
        value should be set to at least MAX_COMPUTERNAME_LENGTH + 1 to allow
        sufficient room in the buffer for the computer name.  The length
        of the string is returned in nSize.

Return Value:

    TRUE on success, FALSE on failure.


--*/
{
    NTSTATUS Status ;
    DWORD ValueLength ;
    DWORD HostLength ;
    DWORD DomainLength ;
    BOOL DontSetReturn = FALSE ;
    NT_PRODUCT_TYPE ProductType ;
    COMPUTER_NAME_FORMAT HostNameFormat, DomainNameFormat ;


    if ( NameType >= ComputerNameMax )
    {
        BaseSetLastNTError( STATUS_INVALID_PARAMETER );
        return FALSE ;
    }

    //
    // For general names, allow clusters to override the physical name:
    //

    if ( (NameType >= ComputerNameNetBIOS) &&
         (NameType <= ComputerNameDnsFullyQualified ) )
    {
        ValueLength = GetEnvironmentVariableW(
                            ClusterNameVars[ NameType ],
                            lpBuffer,
                            *nSize );

        if ( ValueLength )
        {
            *nSize = ValueLength ;

            return TRUE ;
        }
    }

    if ( lpBuffer && (*nSize > 0) )
    {
        lpBuffer[0] = L'\0';
    }

    switch ( NameType )
    {
        case ComputerNameNetBIOS:
        case ComputerNamePhysicalNetBIOS:
            Status = BasepGetNameFromReg(
                        VOLATILE_COMPUTERNAME_NODE,
                        COMPUTERNAME_VALUE_NAME,
                        lpBuffer,
                        nSize );

            if ( !NT_SUCCESS( Status ) )
            {
                if ( Status != STATUS_BUFFER_OVERFLOW )
                {
                    //
                    // Hmm, the value (or key) is missing.  Try the non-volatile
                    // one.
                    //

                    Status = BasepGetNameFromReg(
                                NON_VOLATILE_COMPUTERNAME_NODE,
                                COMPUTERNAME_VALUE_NAME,
                                lpBuffer,
                                nSize );


                }
            }

            break;

        case ComputerNameDnsHostname:
        case ComputerNamePhysicalDnsHostname:
            Status = BasepGetNameFromReg(
                        TCPIP_ROOT,
                        TCPIP_HOSTNAME,
                        lpBuffer,
                        nSize );

            break;

        case ComputerNameDnsDomain:
        case ComputerNamePhysicalDnsDomain:

            if ( !RtlGetNtProductType( &ProductType ))
            {
                //
                // If this fails, assume wksta for purposes of policy.
                //

                ProductType = NtProductWinNt;
            }

            if ( ProductType != NtProductLanManNt )
            {
                //
                //  Allow policy to override the domain name from the
                //  tcpip key on non-domain controllers.
                //

                Status = BasepGetNameFromReg(
                                TCPIP_POLICY_ROOT,
                                TCPIP_POLICY_DOMAINNAME,
                                lpBuffer,
                                nSize );
            }
            else
            {
                Status = STATUS_OBJECT_NAME_NOT_FOUND ;
            }

            //
            // If no policy, or a DC, read from the tcpip key.
            //

            if ( !NT_SUCCESS( Status ) )
            {
                Status = BasepGetNameFromReg(
                            TCPIP_ROOT,
                            TCPIP_DOMAINNAME,
                            lpBuffer,
                            nSize );
            }

            break;

        case ComputerNameDnsFullyQualified:
        case ComputerNamePhysicalDnsFullyQualified:

            //
            // This is the tricky case.  We have to construct the name from
            // the two components for the caller.
            //

            //
            // In general, don't set the last status, since we'll end up using
            // the other calls to handle that for us.
            //

            DontSetReturn = TRUE ;

            Status = STATUS_UNSUCCESSFUL ;

            if ( lpBuffer == NULL )
            {
                //
                // If this is just the computation call, quickly do the
                // two components
                //

                HostLength = DomainLength = 0 ;

                GetComputerNameExW( ComputerNameDnsHostname, NULL, &HostLength );

                if ( GetLastError() == ERROR_MORE_DATA )
                {
                    GetComputerNameExW( ComputerNameDnsDomain, NULL, &DomainLength );

                    if ( GetLastError() == ERROR_MORE_DATA )
                    {
                        //
                        // Simply add.  Note that since both account for a
                        // null terminator, the '.' that goes between them is
                        // covered.
                        //

                        *nSize = HostLength + DomainLength ;

                        Status = STATUS_BUFFER_OVERFLOW ;

                        DontSetReturn = FALSE ;
                    }
                }
            }
            else
            {
                HostLength = *nSize ;

                if ( GetComputerNameExW( ComputerNameDnsHostname,
                                         lpBuffer,
                                         &HostLength ) )
                {
                    HostLength += 1; // Add in the zero character (or . depending on perspective)
                    lpBuffer[ HostLength - 1 ] = L'.';

                    DomainLength = *nSize - HostLength ;

                    if (GetComputerNameExW( ComputerNameDnsDomain,
                                            &lpBuffer[ HostLength ],
                                            &DomainLength ) )
                    {
                        Status = STATUS_SUCCESS ;

                        if ( DomainLength == 0 )
                        {
                            lpBuffer[ HostLength - 1 ] = L'\0';
                            HostLength-- ;
                        }

                        *nSize = HostLength + DomainLength ;

                        DontSetReturn = TRUE ;
                    }
                    else if ( GetLastError() == ERROR_MORE_DATA )
                    {
                        //
                        // Simply add.  Note that since both account for a
                        // null terminator, the '.' that goes between them is
                        // covered.
                        //

                        *nSize = HostLength + DomainLength ;

                        Status = STATUS_BUFFER_OVERFLOW ;

                        DontSetReturn = FALSE ;
                    }
                    else
                    {
                        //
                        // Other error from trying to get the DNS Domain name.
                        // Let the error from the call trickle back.
                        //

                        *nSize = 0 ;

                        Status = STATUS_UNSUCCESSFUL ;

                        DontSetReturn = TRUE ;
                    }

                }
                else if ( GetLastError() == ERROR_MORE_DATA )
                {
                    DomainLength = 0;
                    GetComputerNameExW( ComputerNameDnsDomain, NULL, &DomainLength );

                    if ( GetLastError() == ERROR_MORE_DATA )
                    {
                        //
                        // Simply add.  Note that since both account for a
                        // null terminator, the '.' that goes between them is
                        // covered.
                        //

                        *nSize = HostLength + DomainLength ;

                        Status = STATUS_BUFFER_OVERFLOW ;

                        DontSetReturn = FALSE ;
                    }
                }
                else
                {

                    //
                    // Other error from trying to get the DNS Hostname.
                    // Let the error from the call trickle back.
                    //

                    *nSize = 0 ;

                    Status = STATUS_UNSUCCESSFUL ;

                    DontSetReturn = TRUE ;
                }
            }


            break;



    }

    if ( !NT_SUCCESS( Status ) )
    {
        if ( !DontSetReturn )
        {
            BaseSetLastNTError( Status );
        }
        return FALSE ;
    }

    return TRUE ;
}



BOOL
BaseSetNetbiosName(
    IN LPCWSTR lpComputerName
    )

{
    NTSTATUS NtStatus ;
    ULONG ComputerNameLength;
    ULONG AnsiComputerNameLength;

    //
    // Validate that the supplied computername is valid (not too long,
    // no incorrect characters, no leading or trailing spaces)
    //

    ComputerNameLength = wcslen(lpComputerName);

    //
    // The name length limitation should be based on ANSI. (LanMan compatibility)
    //

    NtStatus = RtlUnicodeToMultiByteSize(&AnsiComputerNameLength,
                                         (LPWSTR)lpComputerName,
                                         ComputerNameLength * sizeof(WCHAR));

    if ((!NT_SUCCESS(NtStatus)) ||
        (AnsiComputerNameLength == 0 )||(AnsiComputerNameLength > MAX_COMPUTERNAME_LENGTH)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //
    // Check for illegal characters; return an error if one is found
    //

    if (wcscspn(lpComputerName, ILLEGAL_NAME_CHARS_STR) < ComputerNameLength) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //
    // Check for leading or trailing spaces
    //

    if (lpComputerName[0] == L' ' ||
        lpComputerName[ComputerNameLength-1] == L' ') {
            SetLastError(ERROR_INVALID_PARAMETER);
            return(FALSE);

    }
    //
    // Open the ComputerName\ComputerName node
    //

    NtStatus = BaseSetNameInReg( NON_VOLATILE_COMPUTERNAME_NODE,
                                 COMPUTERNAME_VALUE_NAME,
                                 lpComputerName );

    if ( !NT_SUCCESS( NtStatus ))
    {
        BaseSetLastNTError( NtStatus );

        return FALSE ;
    }

    return TRUE ;
}

BOOL
BaseSetDnsName(
    LPCWSTR lpComputerName
    )
{

    UNICODE_STRING NewComputerName ;
    UNICODE_STRING DnsName ;
    NTSTATUS Status ;
    BOOL Return ;
    HANDLE DnsApi ;
    DNS_VALIDATE_NAME_FN * DnsValidateNameFn ;
    DNS_STATUS DnsStatus ;

    DnsApi = LoadLibrary("DNSAPI.DLL");

    if ( !DnsApi )
    {
        return FALSE ;
    }

    DnsValidateNameFn = (DNS_VALIDATE_NAME_FN *) GetProcAddress( DnsApi, "DnsValidateName_W" );

    if ( !DnsValidateNameFn )
    {
        FreeLibrary( DnsApi );

        return FALSE ;
    }

    DnsStatus = DnsValidateNameFn( lpComputerName, DnsNameHostnameLabel );

    FreeLibrary( DnsApi );

    if ( ( DnsStatus == 0 ) ||
         ( DnsStatus == DNS_ERROR_NON_RFC_NAME ) )
    {
        Status = BaseSetNameInReg( TCPIP_ROOT,
                                   TCPIP_NV_HOSTNAME,
                                   lpComputerName );
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER ;
    }

    if ( NT_SUCCESS( Status ) )
    {
        RtlInitUnicodeString( &DnsName, lpComputerName );

        Status = RtlDnsHostNameToComputerName( &NewComputerName,
                                               &DnsName,
                                               TRUE );

        if ( NT_SUCCESS( Status ) )
        {
            Return = BaseSetNetbiosName( NewComputerName.Buffer );

            RtlFreeUnicodeString( &NewComputerName );

            if ( !Return )
            {
                //
                // What?  Rollback?
                //

                return FALSE ;
            }

            return TRUE ;
        }
    }

    BaseSetLastNTError( Status ) ;

    return FALSE ;
}

BOOL
BaseSetDnsDomain(
    LPCWSTR lpName
    )
{
    NTSTATUS Status ;
    HANDLE DnsApi ;
    DNS_VALIDATE_NAME_FN * DnsValidateNameFn ;
    DNS_STATUS DnsStatus ;

    //
    // Special case the empty string, which is legal, but not according to dnsapi
    //

    if ( *lpName )
    {
        DnsApi = LoadLibrary("DNSAPI.DLL");

        if ( !DnsApi )
        {
            return FALSE ;
        }

        DnsValidateNameFn = (DNS_VALIDATE_NAME_FN *) GetProcAddress( DnsApi, "DnsValidateName_W" );

        if ( !DnsValidateNameFn )
        {
            FreeLibrary( DnsApi );

            return FALSE ;
        }

        DnsStatus = DnsValidateNameFn( lpName, DnsNameDomain );

        FreeLibrary( DnsApi );
    }
    else
    {
        DnsStatus = 0 ;
    }

    //
    // If the name is good, then keep it.
    //


    if ( ( DnsStatus == 0 ) ||
         ( DnsStatus == DNS_ERROR_NON_RFC_NAME ) )
    {
        Status = BaseSetNameInReg(
                        TCPIP_ROOT,
                        TCPIP_NV_DOMAINNAME,
                        lpName );
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER ;
    }



    if ( !NT_SUCCESS( Status ) )
    {
        BaseSetLastNTError( Status );

        return FALSE ;
    }
    return TRUE ;

}


BOOL
WINAPI
SetComputerNameExW(
    IN COMPUTER_NAME_FORMAT NameType,
    IN LPCWSTR lpBuffer
    )

/*++

Routine Description:

  This sets what the computername will be when the system is next booted.  This
  does not effect the active computername for the remainder of this boot, nor
  what is returned by GetComputerName before the next system boot.


Arguments:

    NameType - Name to set for the system

    lpComputerName - points to the buffer that is contains the
        null-terminated character string containing the computer name.

Return Value:

    Returns TRUE on success, FALSE on failure.


--*/
{
    ULONG Length ;

    //
    // Validate name:
    //

    if ( !lpBuffer )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE ;
    }

    Length = wcslen( lpBuffer );

    if ( Length )
    {
        if ( ( lpBuffer[0] == L' ') ||
             ( lpBuffer[ Length - 1 ] == L' ' ) )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE ;
        }

    }

    if (wcscspn(lpBuffer, ILLEGAL_NAME_CHARS_STR) < Length) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    switch ( NameType )
    {
        case ComputerNamePhysicalNetBIOS:
            return BaseSetNetbiosName( lpBuffer );

        case ComputerNamePhysicalDnsHostname:
            return BaseSetDnsName( lpBuffer );

        case ComputerNamePhysicalDnsDomain:
            return BaseSetDnsDomain( lpBuffer );

        default:
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE ;

    }

}





//
// ANSI APIs
//

BOOL
WINAPI
GetComputerNameA (
    LPSTR lpBuffer,
    LPDWORD nSize
    )

/*++

Routine Description:

  This returns the active computername.  This is the computername when the
  system was last booted.  If this is changed (via SetComputerName) it does
  not take effect until the next system boot.


Arguments:

    lpBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the computer name.

    nSize - Specifies the maximum size (in characters) of the buffer.  This
        value should be set to at least MAX_COMPUTERNAME_LENGTH to allow
        sufficient room in the buffer for the computer name.  The length of
        the string is returned in nSize.

Return Value:

    TRUE on success, FALSE on failure.


--*/
{

    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    LPWSTR UnicodeBuffer;
    ULONG AnsiSize;
    ULONG UnicodeSize;

    //
    // Work buffer needs to be twice the size of the user's buffer
    //

    UnicodeBuffer = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), *nSize * sizeof(WCHAR));
    if (!UnicodeBuffer) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    //
    // Set up an ANSI_STRING that points to the user's buffer
    //

    AnsiString.MaximumLength = (USHORT) *nSize;
    AnsiString.Length = 0;
    AnsiString.Buffer = lpBuffer;

    //
    // Call the UNICODE version to do the work
    //

    UnicodeSize = *nSize ;

    if (!GetComputerNameW(UnicodeBuffer, &UnicodeSize)) {
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);
        return(FALSE);
    }

    //
    // Find out the required size of the ANSI buffer and validate it against
    // the passed in buffer size
    //

    RtlInitUnicodeString(&UnicodeString, UnicodeBuffer);
    AnsiSize = RtlUnicodeStringToAnsiSize(&UnicodeString);
    if (AnsiSize > *nSize) {

        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);

        BaseSetLastNTError( STATUS_BUFFER_OVERFLOW );

        *nSize = AnsiSize + 1 ;

        return(FALSE);
    }


    //
    // Now convert back to ANSI for the caller
    //

    RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);

    *nSize = AnsiString.Length;
    RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);
    return(TRUE);

}



BOOL
WINAPI
SetComputerNameA (
    LPCSTR lpComputerName
    )

/*++

Routine Description:

  This sets what the computername will be when the system is next booted.  This
  does not effect the active computername for the remainder of this boot, nor
  what is returned by GetComputerName before the next system boot.


Arguments:

    lpComputerName - points to the buffer that is contains the
        null-terminated character string containing the computer name.

Return Value:

    Returns TRUE on success, FALSE on failure.


--*/
{

    NTSTATUS NtStatus;
    BOOL ReturnValue;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    ULONG ComputerNameLength;

    //
    // Validate that the supplied computername is valid (not too long,
    // no incorrect characters, no leading or trailing spaces)
    //

    ComputerNameLength = strlen(lpComputerName);
    if ((ComputerNameLength == 0 )||(ComputerNameLength > MAX_COMPUTERNAME_LENGTH)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    RtlInitAnsiString(&AnsiString, lpComputerName);
    NtStatus = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString,
        TRUE);
    if (!NT_SUCCESS(NtStatus)) {
        BaseSetLastNTError(NtStatus);
        return(FALSE);
    }

    ReturnValue = SetComputerNameW((LPCWSTR)UnicodeString.Buffer);
    RtlFreeUnicodeString(&UnicodeString);
    return(ReturnValue);
}

BOOL
WINAPI
GetComputerNameExA(
    IN COMPUTER_NAME_FORMAT NameType,
    OUT LPSTR lpBuffer,
    IN OUT LPDWORD nSize
    )
/*++

Routine Description:

  This returns the active computername in a particular format.  This is the
  computername when the system was last booted.  If this is changed (via
  SetComputerName) it does not take effect until the next system boot.


Arguments:

    NameType - Possible name formats to return the computer name in:

        ComputerNameNetBIOS - netbios name (compatible with GetComputerName)
        ComputerNameDnsHostname - DNS host name
        ComputerNameDnsDomain - DNS Domain name
        ComputerNameDnsFullyQualified - DNS Fully Qualified (hostname.dnsdomain)

    lpBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the computer name.

    nSize - Specifies the maximum size (in characters) of the buffer.  This
        value should be set to at least MAX_COMPUTERNAME_LENGTH + 1 to allow
        sufficient room in the buffer for the computer name.  The length
        of the string is returned in nSize.

Return Value:

    TRUE on success, FALSE on failure.


--*/
{
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    LPWSTR UnicodeBuffer;
    ULONG AnsiSize;
    ULONG UnicodeSize ;

    //
    // Work buffer needs to be twice the size of the user's buffer
    //

    UnicodeBuffer = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), *nSize * sizeof(WCHAR));
    if (!UnicodeBuffer) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    //
    // Set up an ANSI_STRING that points to the user's buffer
    //

    AnsiString.MaximumLength = (USHORT) *nSize;
    AnsiString.Length = 0;
    AnsiString.Buffer = lpBuffer;

    //
    // Call the UNICODE version to do the work
    //

    UnicodeSize = *nSize ;

    if (!GetComputerNameExW(NameType, UnicodeBuffer, &UnicodeSize)) {
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);
        return(FALSE);
    }

    //
    // Find out the required size of the ANSI buffer and validate it against
    // the passed in buffer size
    //

    RtlInitUnicodeString(&UnicodeString, UnicodeBuffer);
    AnsiSize = RtlUnicodeStringToAnsiSize(&UnicodeString);
    if (AnsiSize > *nSize) {

        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);

        BaseSetLastNTError( STATUS_BUFFER_OVERFLOW );

        *nSize = AnsiSize + 1 ;

        return(FALSE);
    }

    //
    // Now convert back to ANSI for the caller
    //

    RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);

    *nSize = AnsiString.Length;
    RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);
    return(TRUE);


}

BOOL
WINAPI
SetComputerNameExA(
    IN COMPUTER_NAME_FORMAT NameType,
    IN LPCSTR lpBuffer
    )
/*++

Routine Description:

  This sets what the computername will be when the system is next booted.  This
  does not effect the active computername for the remainder of this boot, nor
  what is returned by GetComputerName before the next system boot.


Arguments:

    NameType - Name to set for the system

    lpComputerName - points to the buffer that is contains the
        null-terminated character string containing the computer name.

Return Value:

    Returns TRUE on success, FALSE on failure.


--*/
{
    NTSTATUS NtStatus;
    BOOL ReturnValue;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;


    RtlInitAnsiString(&AnsiString, lpBuffer);
    NtStatus = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString,
        TRUE);
    if (!NT_SUCCESS(NtStatus)) {
        BaseSetLastNTError(NtStatus);
        return(FALSE);
    }

    ReturnValue = SetComputerNameExW(NameType, (LPCWSTR)UnicodeString.Buffer );
    RtlFreeUnicodeString(&UnicodeString);
    return(ReturnValue);
}

BOOL
WINAPI
DnsHostnameToComputerNameW(
    IN LPWSTR Hostname,
    OUT LPWSTR ComputerName,
    IN OUT LPDWORD nSize)
/*++

Routine Description:

    This routine will convert a DNS Hostname to a Win32 Computer Name.

Arguments:

    Hostname - DNS Hostname (any length)

    ComputerName - Win32 Computer Name (max length of MAX_COMPUTERNAME_LENGTH)

    nSize - On input, size of the buffer pointed to by ComputerName.  On output,
            size of the Computer Name, in characters.

Return Value:

    Returns TRUE on success, FALSE on failure.


--*/

{
    WCHAR CompName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    DWORD Size = MAX_COMPUTERNAME_LENGTH + 1 ;
    UNICODE_STRING CompName_U ;
    UNICODE_STRING Hostname_U ;
    NTSTATUS Status ;
    BOOL Ret ;

    CompName[0] = L'\0';
    CompName_U.Buffer = CompName ;
    CompName_U.Length = 0 ;
    CompName_U.MaximumLength = (MAX_COMPUTERNAME_LENGTH + 1) * sizeof( WCHAR );

    RtlInitUnicodeString( &Hostname_U, Hostname );

    Status = RtlDnsHostNameToComputerName( &CompName_U,
                                           &Hostname_U,
                                           FALSE );

    if ( NT_SUCCESS( Status ) )
    {
        if ( *nSize >= CompName_U.Length / sizeof(WCHAR) + 1 )
        {
            RtlCopyMemory( ComputerName,
                           CompName_U.Buffer,
                           CompName_U.Length );

            ComputerName[ CompName_U.Length / sizeof( WCHAR ) ] = L'\0';

            Ret = TRUE ;
        }
        else
        {
            BaseSetLastNTError( STATUS_BUFFER_OVERFLOW );
            Ret = FALSE ;
        }

        //
        // returns the count of characters
        //

        *nSize = CompName_U.Length / sizeof( WCHAR );
    }
    else
    {
        BaseSetLastNTError( Status );

        Ret = FALSE ;
    }

    return Ret ;

}

BOOL
WINAPI
DnsHostnameToComputerNameA(
    IN LPSTR Hostname,
    OUT LPSTR ComputerName,
    IN OUT LPDWORD nSize)
/*++

Routine Description:

    This routine will convert a DNS Hostname to a Win32 Computer Name.

Arguments:

    Hostname - DNS Hostname (any length)

    ComputerName - Win32 Computer Name (max length of MAX_COMPUTERNAME_LENGTH)

    nSize - On input, size of the buffer pointed to by ComputerName.  On output,
            size of the Computer Name, in characters.

Return Value:

    Returns TRUE on success, FALSE on failure.


--*/
{
    WCHAR CompName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    DWORD Size = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL Ret ;
    UNICODE_STRING CompName_U ;
    UNICODE_STRING Hostname_U ;
    NTSTATUS Status ;
    ANSI_STRING CompName_A ;


    Status = RtlCreateUnicodeStringFromAsciiz( &Hostname_U,
                                               Hostname );

    if ( NT_SUCCESS( Status ) )
    {
        CompName[0] = L'\0';
        CompName_U.Buffer = CompName ;
        CompName_U.Length = 0 ;
        CompName_U.MaximumLength = (MAX_COMPUTERNAME_LENGTH + 1) * sizeof( WCHAR );

        Status = RtlDnsHostNameToComputerName( &CompName_U,
                                               &Hostname_U,
                                               FALSE );

        if ( NT_SUCCESS( Status ) )
        {
            CompName_A.Buffer = ComputerName ;
            CompName_A.Length = 0 ;
            CompName_A.MaximumLength = (USHORT) *nSize ;

            Status = RtlUnicodeStringToAnsiString( &CompName_A, &CompName_U, FALSE );

            if ( NT_SUCCESS( Status ) )
            {
                *nSize = CompName_A.Length ;
            }

        }

    }

    if ( !NT_SUCCESS( Status ) )
    {
        BaseSetLastNTError( Status );
        return FALSE ;
    }

    return TRUE ;

}





#include "dfsfsctl.h"
DWORD
BasepGetComputerNameFromNtPath (
    PUNICODE_STRING NtPathName,
    HANDLE hFile,
    LPWSTR lpBuffer,
    LPDWORD nSize
    )

/*++

Routine Description:

  Look at a path and determine the computer name of the host machine.
  In the future, we should remove this code, and add the capbility to query
  handles for their computer name.

  The name can only be obtained for NetBios paths - if the path is IP or DNS
  an error is returned.  (If the NetBios name has a "." in it, it will
  cause an error because it will be misinterpreted as a DNS path.  This case
  becomes less and less likely as the NT5 UI doesn't allow such computer names.)
  For DFS paths, the leaf server's name is returned, as long as it wasn't
  joined to its parent with an IP or DNS path name.

Arguments:

  NtPathName - points to a unicode string with the path to query.
  lpBuffer - points to buffer receives the computer name
  nSize - points to dword with the size of the input buffer, and the length
    (in characters, not including the null terminator) of the computer name
    on output.

Return Value:

    A Win32 error code.

--*/
{
    ULONG cbComputer = 0;
    DWORD dwError = ERROR_BAD_PATHNAME;
    ULONG AvailableLength = 0;
    PWCHAR PathCharacter = NULL;
    BOOL CheckForDfs = TRUE;
    NTSTATUS NtStatus = STATUS_SUCCESS;

    IO_STATUS_BLOCK IoStatusBlock;
    WCHAR FileNameInfoBuffer[MAX_PATH+sizeof(FILE_NAME_INFORMATION)];
    PFILE_NAME_INFORMATION FileNameInfo = (PFILE_NAME_INFORMATION)FileNameInfoBuffer;
    WCHAR DfsServerPathName[ MAX_PATH + 1 ];
    WCHAR DosDevice[3] = { L"A:" };
    WCHAR DosDeviceMapping[ MAX_PATH + 1 ];


    UNICODE_STRING UnicodeComputerName;

    const UNICODE_STRING NtUncPathNamePrefix = { 16, 18, L"\\??\\UNC\\"};
    const ULONG cchNtUncPathNamePrefix = 8;

    const UNICODE_STRING NtDrivePathNamePrefix = { 8, 10, L"\\??\\" };
    const ULONG cchNtDrivePathNamePrefix = 4;

    RtlInitUnicodeString( &UnicodeComputerName, NULL );

    // Is this a UNC path?

    if( RtlPrefixString( (PSTRING)&NtUncPathNamePrefix, (PSTRING)NtPathName, TRUE )) {

        // Make sure there's some more to this path than just the prefix
        if( NtPathName->Length <= NtUncPathNamePrefix.Length )
            goto Exit;

        // It appears to be a valid UNC path.  Point to the beginning of the computer
        // name, and calculate how much room is left in NtPathName after that.

        UnicodeComputerName.Buffer = &NtPathName->Buffer[ NtUncPathNamePrefix.Length/sizeof(WCHAR) ];
        AvailableLength = NtPathName->Length - NtUncPathNamePrefix.Length;

    }

    // If it's not a UNC path, then is it a drive-letter path?

    else if( RtlPrefixString( (PSTRING)&NtDrivePathNamePrefix, (PSTRING)NtPathName, TRUE )
             &&
             NtPathName->Buffer[ cchNtDrivePathNamePrefix + 1 ] == L':' ) {

        // It's a drive letter path, but it could still be local or remote

        static const WCHAR RedirectorMappingPrefix[] = { L"\\Device\\LanmanRedirector\\;" };
        static const WCHAR LocalVolumeMappingPrefix[] = { L"\\Device\\Harddisk" };
        static const WCHAR CDRomMappingPrefix[] = { L"\\Device\\CdRom" };
        static const WCHAR FloppyMappingPrefix[] = { L"\\Device\\Floppy" };
        static const WCHAR DfsMappingPrefix[] = { L"\\Device\\WinDfs\\" };

        // Get the correct, upper-cased, drive letter into DosDevice.

        DosDevice[0] = NtPathName->Buffer[ cchNtDrivePathNamePrefix ];
        if( L'a' <= DosDevice[0] && DosDevice[0] <= L'z' )
            DosDevice[0] = L'A' + (DosDevice[0] - L'a');

        // Map the drive letter to its symbolic link under \??.  E.g., say C:, D: & R:
        // are local/DFS/rdr drives, respectively.  You would then see something like:
        //
        //   C: => \Device\Volume1
        //   D: => \Device\WinDfs\G
        //   R: => \Device\LanmanRedirector\;R:0\scratch\scratch

        if( !QueryDosDeviceW( DosDevice, DosDeviceMapping, sizeof(DosDeviceMapping)/sizeof(DosDeviceMapping[0]) )) {
            dwError = GetLastError();
            goto Exit;
        }

        // Now that we have the DosDeviceMapping, we can check ... Is this a rdr drive?

        if( // Does it begin with "\Device\LanmanRedirector\;" ?
            DosDeviceMapping == wcsstr( DosDeviceMapping, RedirectorMappingPrefix )
            &&
            // Are the next letters the correct drive letter, a colon, and a whack?
            ( DosDevice[0] == DosDeviceMapping[ sizeof(RedirectorMappingPrefix)/sizeof(WCHAR) - 1 ]
              &&
              L':' == DosDeviceMapping[ sizeof(RedirectorMappingPrefix)/sizeof(WCHAR) ]
              &&
              (UnicodeComputerName.Buffer = wcschr(&DosDeviceMapping[ sizeof(RedirectorMappingPrefix)/sizeof(WCHAR) + 1 ], L'\\'))
            )) {

            // We have a valid rdr drive.  Point to the beginning of the computer
            // name, and calculate how much room is availble in DosDeviceMapping after that.

            UnicodeComputerName.Buffer += 1;
            AvailableLength = sizeof(DosDeviceMapping) - sizeof(DosDeviceMapping[0]) * (ULONG)(UnicodeComputerName.Buffer - DosDeviceMapping);

            // We know now that it's not a DFS path
            CheckForDfs = FALSE;

        }

        // If it's not a rdr drive, then maybe it's a local volume, floppy, or cdrom

        else if( DosDeviceMapping == wcsstr( DosDeviceMapping, LocalVolumeMappingPrefix )
                 ||
                 DosDeviceMapping == wcsstr( DosDeviceMapping, CDRomMappingPrefix )
                 ||
                 DosDeviceMapping == wcsstr( DosDeviceMapping, FloppyMappingPrefix ) ) {

            // We have a local drive, so just return the local computer name.

            CheckForDfs = FALSE;

            if( !GetComputerNameW( lpBuffer, nSize))
                dwError = GetLastError();
            else
                dwError = ERROR_SUCCESS;
            goto Exit;
        }

        // Finally, check to see if it's a DFS drive

        else if( DosDeviceMapping == wcsstr( DosDeviceMapping, DfsMappingPrefix )) {

            // Get the full UNC name of this DFS path.  Later, we'll call the DFS
            // driver to find out what the actual server name is.

            NtStatus = NtQueryInformationFile(
                        hFile,
                        &IoStatusBlock,
                        FileNameInfo,
                        sizeof(FileNameInfoBuffer),
                        FileNameInformation
                        );
            if( !NT_SUCCESS(NtStatus) ) {
                dwError = RtlNtStatusToDosError(NtStatus);
                goto Exit;
            }

            UnicodeComputerName.Buffer = FileNameInfo->FileName + 1;
            AvailableLength = FileNameInfo->FileNameLength;
        }

        // Otherwise, it's not a rdr, dfs, or local drive, so there's nothing we can do.

        else
            goto Exit;

    }   // else if( RtlPrefixString( (PSTRING)&NtDrivePathNamePrefix, (PSTRING)NtPathName, TRUE ) ...

    else {
        dwError = ERROR_BAD_PATHNAME;
        goto Exit;
    }


    // If we couldn't determine above if whether or not this is a DFS path, let the
    // DFS driver decide now.

    if( CheckForDfs && INVALID_HANDLE_VALUE != hFile ) {

        HANDLE hDFS = INVALID_HANDLE_VALUE;
        UNICODE_STRING DfsDriverName;
        OBJECT_ATTRIBUTES ObjectAttributes;

        WCHAR *DfsPathName = UnicodeComputerName.Buffer - 1;    // Back up to the whack
        ULONG DfsPathNameLength = AvailableLength + sizeof(WCHAR);

        // Open the DFS driver

        RtlInitUnicodeString( &DfsDriverName, DFS_DRIVER_NAME );
        InitializeObjectAttributes( &ObjectAttributes,
                                    &DfsDriverName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL
                                );

        NtStatus = NtCreateFile(
                        &hDFS,
                        SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_OPEN_IF,
                        FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL,
                        0
                    );

        if( !NT_SUCCESS(NtStatus) ) {
            dwError = RtlNtStatusToDosError(NtStatus);
            goto Exit;
        }

        // Query DFS's cache for the server name.  The name is guaranteed to
        // remain in the cache as long as the file is open.

        if( L'\\' != DfsPathName[0] ) {
            NtClose(hDFS);
            dwError = ERROR_BAD_PATHNAME;
            goto Exit;
        }

        NtStatus = NtFsControlFile(
                        hDFS,
                        NULL,       // Event,
                        NULL,       // ApcRoutine,
                        NULL,       // ApcContext,
                        &IoStatusBlock,
                        FSCTL_DFS_GET_SERVER_NAME,
                        DfsPathName,
                        DfsPathNameLength,
                        DfsServerPathName,
                        sizeof(DfsServerPathName)
                    );
        NtClose( hDFS );

        // STATUS_OBJECT_NAME_NOT_FOUND means that it's not a DFS path
        if( !NT_SUCCESS(NtStatus) ) {
            if( STATUS_OBJECT_NAME_NOT_FOUND != NtStatus  ) {
                dwError = RtlNtStatusToDosError(NtStatus);
                goto Exit;
            }
        }
        else if( L'\0' != DfsServerPathName[0] ) {

            // The previous DFS call returns the server-specific path to the file in UNC form.
            // Point UnicodeComputerName to just past the two whacks.

            AvailableLength = wcslen(DfsServerPathName) * sizeof(WCHAR);
            if( 3*sizeof(WCHAR) > AvailableLength
                ||
                L'\\' != DfsServerPathName[0]
                ||
                L'\\' != DfsServerPathName[1] )
            {
                dwError = ERROR_BAD_PATHNAME;
                goto Exit;
            }

            UnicodeComputerName.Buffer = DfsServerPathName + 2;
            AvailableLength -= 2 * sizeof(WCHAR);
        }
    }

    // If we get here, then the computer name\share is pointed to by UnicodeComputerName.Buffer.
    // But the Length is currently zero, so we search for the whack that separates
    // the computer name from the share, and set the Length to include just the computer name.

    PathCharacter = UnicodeComputerName.Buffer;

    while( ( (ULONG) ((PCHAR)PathCharacter - (PCHAR)UnicodeComputerName.Buffer) < AvailableLength)
           &&
           *PathCharacter != L'\\' ) {

        // If we found a '.', we fail because this is probably a DNS or IP name.
        if( L'.' == *PathCharacter ) {
            dwError = ERROR_BAD_PATHNAME;
            goto Exit;
        }

        PathCharacter++;
    }

    // Set the computer name length

    UnicodeComputerName.Length = UnicodeComputerName.MaximumLength
        = (USHORT) ((PCHAR)PathCharacter - (PCHAR)UnicodeComputerName.Buffer);

    // Fail if the computer name exceeded the length of the input NtPathName,
    // or if the length exceeds that allowed.

    if( UnicodeComputerName.Length >= AvailableLength
        ||
        UnicodeComputerName.Length > MAX_COMPUTERNAME_LENGTH*sizeof(WCHAR) ) {
        goto Exit;
    }

    // Copy the computer name into the caller's buffer, as long as there's enough
    // room for the name & a terminating '\0'.

    if( UnicodeComputerName.Length + sizeof(WCHAR) > *nSize * sizeof(WCHAR) ) {
        dwError = ERROR_BUFFER_OVERFLOW;
        goto Exit;
    }

    RtlCopyMemory( lpBuffer, UnicodeComputerName.Buffer, UnicodeComputerName.Length );
    *nSize = UnicodeComputerName.Length / sizeof(WCHAR);
    lpBuffer[ *nSize ] = L'\0';

    dwError = ERROR_SUCCESS;


Exit:

    return( dwError );

}
