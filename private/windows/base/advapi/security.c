/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    security.c

Abstract:

    This module implements Object Security APIs for Win32

Author:

    Jim Anderson (JimA) 01-Jul-1991
    Robert Reichel (RobertRe) 01-Jan-92

Revision History:

--*/

#include "advapi.h"
#include <ntlsa.h>
#include <rpc.h>
#include <rpcndr.h>
#include <stdio.h>

#define LSADEFINED


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//               Private Routine Prototypes                                //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


VOID
SepFormatAccountSid(
    PSID iSid,
    LPWSTR OutputBuffer
    );



/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//               Exported Routines                                         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////





BOOL
APIENTRY
DuplicateToken(
    HANDLE ExistingTokenHandle,
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    PHANDLE DuplicateTokenHandle
    )

/*++

Routine Description:

    Create a new token that is a duplicate of an existing token.  The
    new token will be an impersonation token of the supplied level.

Arguments:

    ExistingTokenHandle - Is a handle to a token already open for
        TOKEN_DUPLICATE access.

    ImpersonationLevel - Supplies the impersonation level of the new
        token.

    DuplicateTokenHandle - Returns the handle to the new token.  The
        handle will have TOKEN_IMPERSONATE and TOKEN_QUERY access to
        the new token.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.


--*/

{
    return( DuplicateTokenEx( ExistingTokenHandle,
                              TOKEN_IMPERSONATE | TOKEN_QUERY,
                              NULL,
                              ImpersonationLevel,
                              TokenImpersonation,
                              DuplicateTokenHandle
                              ) );

}

BOOL
APIENTRY
DuplicateTokenEx(
    HANDLE hExistingToken,
    DWORD dwDesiredAccess,
    LPSECURITY_ATTRIBUTES lpTokenAttributes,
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    TOKEN_TYPE TokenType,
    PHANDLE phNewToken)
/*++

    Routine Description:

    Create a new token that is a duplicate of an existing token.  This API
    more fully exposes NtDuplicateToken .

    Arguments:

        hExistingToken - Is a handle to a token already open for
                                        TOKEN_DUPLICATE access.

        dwDesiredAccess - desired access rights to the new token, e.g.
                                       TOKEN_DUPLICATE, TOKEN_IMPERSONATE, etc.

        lpTokenAttributes - Desired security attributes for the new token.

        ImpersonationLevel - Supplies the impersonation level of the new token.

        TokenType - One of TokenImpersonation or TokenPrimary.

        phNewToken  - Returns the handle to the new token.

    Return Value:

        Returns TRUE for success, FALSE for failure.  Extended error status
        is available using GetLastError.

--*/


{

    OBJECT_ATTRIBUTES ObjA;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    NTSTATUS Status;
    ULONG Attributes;

    SecurityQualityOfService.Length = sizeof( SECURITY_QUALITY_OF_SERVICE  );
    SecurityQualityOfService.ImpersonationLevel = ImpersonationLevel;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    if (lpTokenAttributes)
    {
        SecurityDescriptor = lpTokenAttributes->lpSecurityDescriptor;
        if (lpTokenAttributes->bInheritHandle)
        {
            Attributes = OBJ_INHERIT;
        }
        else
        {
            Attributes = 0;
        }
    }
    else
    {
        SecurityDescriptor = NULL;
        Attributes = 0;
    }

    InitializeObjectAttributes(
        &ObjA,
        NULL,
        Attributes,
        NULL,
        SecurityDescriptor
        );

    ObjA.SecurityQualityOfService = &SecurityQualityOfService;

    Status = NtDuplicateToken(
                 hExistingToken,
                 dwDesiredAccess,
                 &ObjA,
                 FALSE,
                 TokenType,
                 phNewToken
                 );

    if ( !NT_SUCCESS( Status ) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return( TRUE );

}





BOOL
APIENTRY
AllocateLocallyUniqueId(
    PLUID Luid
    )
/*++

Routine Description:

    Allocates a locally unique ID (LUID).

Arguments:

    Luid - Supplies a pointer used to return the LUID.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/

{   NTSTATUS Status;

    Status = NtAllocateLocallyUniqueId( Luid );
    if ( !NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( FALSE );
    }

    return( TRUE );
}




BOOL
APIENTRY
AccessCheck (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    HANDLE ClientToken,
    DWORD DesiredAccess,
    PGENERIC_MAPPING GenericMapping,
    PPRIVILEGE_SET PrivilegeSet,
    LPDWORD PrivilegeSetLength,
    LPDWORD GrantedAccess,
    LPBOOL AccessStatus
    )
/*++

Routine Description:

    This routine compares the input Security Descriptor against the
    input token and indicates by its return value if access is granted
    or denied.  If access is granted then the desired access mask
    becomes the granted access mask for the object.

    The semantics of the access check routine is described in the DSA
    Security Architecture workbook.  Note that during an access check
    only the discretionary ACL is examined.

Arguments:

    SecurityDescriptor - Supplies the security descriptor protecting the object
        being accessed

    ClientToken - Supplies the handle of the user's token.

    DesiredAccess - Supplies the desired access mask.

    GenericMapping - Supplies the generic mapping associated with this
        object type.

    PrivilegeSet - A pointer to a buffer that upon return will contain
        any privileges that were used to perform the access validation.
        If no privileges were used, the buffer will contain a privilege
        set consisting of zero privileges.

    PrivilegeSetLength - The size of the PrivilegeSet buffer in bytes.

    GrantedAccess - Returns an access mask describing the granted access.

    AccessStatus - Status value that may be returned indicating the
         reason why access was denied.  Routines should avoid hardcoding a
         return value of STATUS_ACCESS_DENIED so that a different value can
         be returned when mandatory access control is implemented.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;
    NTSTATUS RealStatus;

    Status = NtAccessCheck (
                pSecurityDescriptor,
                ClientToken,
                DesiredAccess,
                GenericMapping,
                PrivilegeSet,
                PrivilegeSetLength,
                GrantedAccess,
                &RealStatus
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    if ( !NT_SUCCESS( RealStatus ) ) {
        BaseSetLastNTError( RealStatus );
        *AccessStatus = FALSE;
        return( TRUE );
    }

    *AccessStatus = TRUE;
    return TRUE;
}




BOOL
APIENTRY
AccessCheckByType (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    PSID PrincipalSelfSid,
    HANDLE ClientToken,
    DWORD DesiredAccess,
    POBJECT_TYPE_LIST ObjectTypeList,
    DWORD ObjectTypeListLength,
    PGENERIC_MAPPING GenericMapping,
    PPRIVILEGE_SET PrivilegeSet,
    LPDWORD PrivilegeSetLength,
    LPDWORD GrantedAccess,
    LPBOOL AccessStatus
    )
/*++

Routine Description:

    This routine compares the input Security Descriptor against the
    input token and indicates by its return value if access is granted
    or denied.  If access is granted then the desired access mask
    becomes the granted access mask for the object.

    The semantics of the access check routine is described in the DSA
    Security Architecture workbook.  Note that during an access check
    only the discretionary ACL is examined.

Arguments:

    SecurityDescriptor - Supplies the security descriptor protecting the object
        being accessed

    PrincipalSelfSid - If the object being access checked is an object which
        represents a principal (e.g., a user object), this parameter should
        be the SID of the object.  Any ACE containing the constant
        PRINCIPAL_SELF_SID is replaced by this SID.

        The parameter should be NULL if the object does not represent a principal.

    ClientToken - Supplies the handle of the user's token.

    DesiredAccess - Supplies the desired access mask.

    ObjectTypeList - Supplies a list of GUIDs representing the object (and
        sub-objects) being accessed.  If no list is present, AccessCheckByType
        behaves identically to AccessCheck.

    ObjectTypeListLength - Specifies the number of elements in the ObjectTypeList.

    GenericMapping - Supplies the generic mapping associated with this
        object type.

    PrivilegeSet - A pointer to a buffer that upon return will contain
        any privileges that were used to perform the access validation.
        If no privileges were used, the buffer will contain a privilege
        set consisting of zero privileges.

    PrivilegeSetLength - The size of the PrivilegeSet buffer in bytes.

    GrantedAccess - Returns an access mask describing the granted access.

    AccessStatus - Status value that may be returned indicating the
         reason why access was denied.  Routines should avoid hardcoding a
         return value of STATUS_ACCESS_DENIED so that a different value can
         be returned when mandatory access control is implemented.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;
    NTSTATUS RealStatus;

    Status = NtAccessCheckByType (
                pSecurityDescriptor,
                PrincipalSelfSid,
                ClientToken,
                DesiredAccess,
                ObjectTypeList,
                ObjectTypeListLength,
                GenericMapping,
                PrivilegeSet,
                PrivilegeSetLength,
                GrantedAccess,
                &RealStatus
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    if ( !NT_SUCCESS( RealStatus ) ) {
        BaseSetLastNTError( RealStatus );
        *AccessStatus = FALSE;
        return( TRUE );
    }

    *AccessStatus = TRUE;
    return TRUE;
}




BOOL
APIENTRY
AccessCheckByTypeResultList (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    PSID PrincipalSelfSid,
    HANDLE ClientToken,
    DWORD DesiredAccess,
    POBJECT_TYPE_LIST ObjectTypeList,
    DWORD ObjectTypeListLength,
    PGENERIC_MAPPING GenericMapping,
    PPRIVILEGE_SET PrivilegeSet,
    LPDWORD PrivilegeSetLength,
    LPDWORD GrantedAccessList,
    LPDWORD AccessStatusList
    )
/*++

Routine Description:

    This routine compares the input Security Descriptor against the
    input token and indicates by its return value if access is granted
    or denied.  If access is granted then the desired access mask
    becomes the granted access mask for the object.

    The semantics of the access check routine is described in the DSA
    Security Architecture workbook.  Note that during an access check
    only the discretionary ACL is examined.

Arguments:

    SecurityDescriptor - Supplies the security descriptor protecting the object
        being accessed

    PrincipalSelfSid - If the object being access checked is an object which
        represents a principal (e.g., a user object), this parameter should
        be the SID of the object.  Any ACE containing the constant
        PRINCIPAL_SELF_SID is replaced by this SID.

        The parameter should be NULL if the object does not represent a principal.

    ClientToken - Supplies the handle of the user's token.

    DesiredAccess - Supplies the desired access mask.

    ObjectTypeList - Supplies a list of GUIDs representing the object (and
        sub-objects) being accessed.  If no list is present, AccessCheckByType
        behaves identically to AccessCheck.

    ObjectTypeListLength - Specifies the number of elements in the ObjectTypeList.

    GenericMapping - Supplies the generic mapping associated with this
        object type.

    PrivilegeSet - A pointer to a buffer that upon return will contain
        any privileges that were used to perform the access validation.
        If no privileges were used, the buffer will contain a privilege
        set consisting of zero privileges.

    PrivilegeSetLength - The size of the PrivilegeSet buffer in bytes.

    GrantedAccessList - Returns an access mask describing the granted access.

    AccessStatusList - Status value that may be returned indicating the
         reason why access was denied.  Routines should avoid hardcoding a
         return value of STATUS_ACCESS_DENIED so that a different value can
         be returned when mandatory access control is implemented.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;
    NTSTATUS RealStatus;
    ULONG i;

    ASSERT (sizeof(NTSTATUS) == sizeof(DWORD) );

    Status = NtAccessCheckByTypeResultList (
                pSecurityDescriptor,
                PrincipalSelfSid,
                ClientToken,
                DesiredAccess,
                ObjectTypeList,
                ObjectTypeListLength,
                GenericMapping,
                PrivilegeSet,
                PrivilegeSetLength,
                GrantedAccessList,
                (PNTSTATUS)AccessStatusList
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    //
    // Loop converting the array of NT status codes to WIN status codes.
    //

    for ( i=0; i<ObjectTypeListLength; i++ ) {
        if ( AccessStatusList[i] == STATUS_SUCCESS ) {
            AccessStatusList[i] = NO_ERROR;
        } else {
            AccessStatusList[i] = RtlNtStatusToDosError( AccessStatusList[i] );
        }
    }

    return TRUE;
}




BOOL
APIENTRY
OpenProcessToken (
    HANDLE ProcessHandle,
    DWORD DesiredAccess,
    PHANDLE TokenHandle
    )
/*++

Routine Description:

    Open a token object associated with a process and return a handle
    that may be used to access that token.

Arguments:

    ProcessHandle - Specifies the process whose token is to be
        opened.

    DesiredAccess - Is an access mask indicating which access types
        are desired to the token.  These access types are reconciled
        with the Discretionary Access Control list of the token to
        determine whether the accesses will be granted or denied.

    TokenHandle - Receives the handle of the newly opened token.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = NtOpenProcessToken (
        ProcessHandle,
        DesiredAccess,
        TokenHandle
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
OpenThreadToken (
    HANDLE ThreadHandle,
    DWORD DesiredAccess,
    BOOL OpenAsSelf,
    PHANDLE TokenHandle
    )
/*++


Routine Description:

Open a token object associated with a thread and return a handle that
may be used to access that token.

Arguments:

    ThreadHandle - Specifies the thread whose token is to be opened.

    DesiredAccess - Is an access mask indicating which access types
        are desired to the token.  These access types are reconciled
        with the Discretionary Access Control list of the token to
        determine whether the accesses will be granted or denied.

    OpenAsSelf - Is a boolean value indicating whether the access should
        be made using the calling thread's current security context, which
        may be that of a client if impersonating, or using the caller's
        process-level security context.  A value of FALSE indicates the
        caller's current context should be used un-modified.  A value of
        TRUE indicates the request should be fulfilled using the process
        level security context.

        This parameter is necessary to allow a server process to open
        a client's token when the client specified IDENTIFICATION level
        impersonation.  In this case, the caller would not be able to
        open the client's token using the client's context (because you
        can't create executive level objects using IDENTIFICATION level
        impersonation).

    TokenHandle - Receives the handle of the newly opened token.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = NtOpenThreadToken (
        ThreadHandle,
        DesiredAccess,
        (BOOLEAN)OpenAsSelf,
        TokenHandle
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
GetTokenInformation (
    HANDLE TokenHandle,
    TOKEN_INFORMATION_CLASS TokenInformationClass,
    PVOID TokenInformation,
    DWORD TokenInformationLength,
    PDWORD ReturnLength
    )
/*++


Routine Description:

    Retrieve information about a specified token.

Arguments:

    TokenHandle - Provides a handle to the token to operate on.

    TokenInformationClass - The token information class about which
        to retrieve information.

    TokenInformation - The buffer to receive the requested class of
        information.  The buffer must be aligned on at least a
        longword boundary.  The actual structures returned are
        dependent upon the information class requested, as defined in
        the TokenInformationClass parameter description.

        TokenInformation Format By Information Class:

           TokenUser => TOKEN_USER data structure.  TOKEN_QUERY
           access is needed to retrieve this information about a
           token.

           TokenGroups => TOKEN_GROUPS data structure.  TOKEN_QUERY
           access is needed to retrieve this information about a
           token.

           TokenPrivileges => TOKEN_PRIVILEGES data structure.
           TOKEN_QUERY access is needed to retrieve this information
           about a token.

           TokenOwner => TOKEN_OWNER data structure.  TOKEN_QUERY
           access is needed to retrieve this information about a
           token.

           TokenPrimaryGroup => TOKEN_PRIMARY_GROUP data structure.
           TOKEN_QUERY access is needed to retrieve this information
           about a token.

           TokenDefaultDacl => TOKEN_DEFAULT_DACL data structure.
           TOKEN_QUERY access is needed to retrieve this information
           about a token.

           TokenSource => TOKEN_SOURCE data structure.
           TOKEN_QUERY_SOURCE access is needed to retrieve this
           information about a token.

           TokenType => TOKEN_TYPE data structure.
           TOKEN_QUERY access is needed to retrieve this information
           about a token.

           TokenStatistics => TOKEN_STATISTICS data structure.
           TOKEN_QUERY access is needed to retrieve this
           information about a token.

    TokenInformationLength - Indicates the length, in bytes, of the
        TokenInformation buffer.

    ReturnLength - This parameter receives the actual length of the
        requested information.  If this value is larger than that
        provided by the TokenInformationLength parameter, then the
        buffer provided to receive the requested information is not
        large enough to hold that data and no data is returned.

        If the queried class is TokenDefaultDacl and there is no
        default Dacl established for the token, then the return
        length will be returned as zero, and no data will be returned.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = NtQueryInformationToken (
        TokenHandle,
        TokenInformationClass,
        TokenInformation,
        TokenInformationLength,
        ReturnLength
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
SetTokenInformation (
    HANDLE TokenHandle,
    TOKEN_INFORMATION_CLASS TokenInformationClass,
    PVOID TokenInformation,
    DWORD TokenInformationLength
    )
/*++


Routine Description:

    Modify information in a specified token.

Arguments:

    TokenHandle - Provides a handle to the token to operate on.

    TokenInformationClass - The token information class being set.

    TokenInformation - The buffer containing the new values for the
        specified class of information.  The buffer must be aligned
        on at least a longword boundary.  The actual structures
        provided are dependent upon the information class specified,
        as defined in the TokenInformationClass parameter
        description.

        TokenInformation Format By Information Class:

           TokenUser => This value is not a valid value for this API.
           The User ID may not be replaced.

           TokenGroups => This value is not a valid value for this
           API.  The Group IDs may not be replaced.  However, groups
           may be enabled and disabled using NtAdjustGroupsToken().

           TokenPrivileges => This value is not a valid value for
           this API.  Privilege information may not be replaced.
           However, privileges may be explicitly enabled and disabled
           using the NtAdjustPrivilegesToken API.

           TokenOwner => TOKEN_OWNER data structure.
           TOKEN_ADJUST_DEFAULT access is needed to replace this
           information in a token.  The owner values that may be
           specified are restricted to the user and group IDs with an
           attribute indicating they may be assigned as the owner of
           objects.

           TokenPrimaryGroup => TOKEN_PRIMARY_GROUP data structure.
           TOKEN_ADJUST_DEFAULT access is needed to replace this
           information in a token.  The primary group values that may
           be specified are restricted to be one of the group IDs
           already in the token.

           TokenDefaultDacl => TOKEN_DEFAULT_DACL data structure.
           TOKEN_ADJUST_DEFAULT access is needed to replace this
           information in a token.  The ACL provided as a new default
           discretionary ACL is not validated for structural
           correctness or consistency.

           TokenSource => This value is not a valid value for this
           API.  The source name and context handle  may not be
           replaced.

           TokenStatistics => This value is not a valid value for this
           API.  The statistics of a token are read-only.

    TokenInformationLength - Indicates the length, in bytes, of the
        TokenInformation buffer.  This is only the length of the primary
        buffer.  All extensions of the primary buffer are self describing.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = NtSetInformationToken (
        TokenHandle,
        TokenInformationClass,
        TokenInformation,
        TokenInformationLength
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}



BOOL
APIENTRY
AdjustTokenPrivileges (
    HANDLE TokenHandle,
    BOOL DisableAllPrivileges,
    PTOKEN_PRIVILEGES NewState,
    DWORD BufferLength,
    PTOKEN_PRIVILEGES PreviousState,
    PDWORD ReturnLength
    )
/*++


Routine Description:

    This routine is used to disable or enable privileges in the
    specified token.  The absence of some of the privileges listed to
    be changed won't effect the successful modification of the
    privileges that are in the token.  The previous enabled/disabled
    state of changed privileges may optionally be capture (for
    resetting later).

    TOKEN_ADJUST_PRIVILEGES access is required to enable or disable
    privileges in a token.


Arguments:

    TokenHandle - Provides a handle to the token to operate on.

    DisableAllPrivileges - This boolean parameter may be
        used to disable all privileges assigned to the token.  If
        this parameter is specified as TRUE, then the NewState parameter is
        ignored.

    NewState - This (optional) parameter points to a TOKEN_PRIVILEGES
        data structure containing the privileges whose states are to
        be adjusted (disabled or enabled).  Only the Enabled flag of
        the attributes associated with each privilege is used.  It
        provides the new value that is to be assigned to the privilege
        in the token.

    BufferLength - This optional parameter indicates the length (in
        bytes) of the PreviousState buffer.  This value must be
        provided if the PreviousState parameter is provided.

    PreviousState - This (optional) parameter points to a buffer to
        receive the state of any privileges actually changed by this
        request.  This information is formated as a TOKEN_PRIVILEGES
        data structure which may be passed as the NewState parameter
        in a subsequent call to this routine to restore the original
        state of those privilges.  TOKEN_QUERY access is needed to use
        this parameter.

        If this buffer does not contain enough space to receive the
        complete list of modified privileges, then no privilege
        states are changed and STATUS_BUFFER_TOO_SMALL is returned.
        In this case, the ReturnLength OUT parameter will
        contain the actual number of bytes needed to hold the
        information.

    ReturnLength - Indicates the actual number of bytes needed to
        contain the previous privilege state information.  This
        parameter is ignored if the PreviousState argument is not
        passed.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = NtAdjustPrivilegesToken (
        TokenHandle,
        (BOOLEAN)DisableAllPrivileges,
        NewState,
        BufferLength,
        PreviousState,
        ReturnLength
        );

    //
    // We need to set last error even for success because that
    // is the only way to tell if the api successfully assigned
    // all privileges.  That is, STATUS_NOT_ALL_ASSIGNED is a
    // Success severity level.
    //

    BaseSetLastNTError(Status);


    if ( !NT_SUCCESS(Status) ) {
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
AdjustTokenGroups (
    HANDLE TokenHandle,
    BOOL ResetToDefault,
    PTOKEN_GROUPS NewState,
    DWORD BufferLength,
    PTOKEN_GROUPS PreviousState,
    PDWORD ReturnLength
    )
/*++


Routine Description:

    This routine is used to disable or enable groups in the specified
    token.  The absence of some of the groups listed to be changed
    won't effect the successful modification of the groups that are in
    the token.  The previous enabled/disabled state of changed groups
    may optionally be capture (for resetting later).

    TOKEN_ADJUST_GROUPS access is required to enable or disable groups
    in a token

    Note that mandatory groups can not be disabled.  An attempt
    disable any mandatory groups will cause the call to fail, leaving
    the state of all groups unchanged.


Arguments:

    TokenHandle - Provides a handle to the token to operate on.

    ResetToDefault - The parameter indicates whether all the groups
        in the token are to be reset to their default enabled/disabled
        state.

    NewState - This parameter points to a TOKEN_GROUPS data structure
        containing the groups whose states are to be adjusted
        (disabled or enabled).  Only the Enabled flag of the
        attributes associated with each group is used.  It provides
        the new value that is to be assigned to the group in the
        token.  If the ResetToDefault argument is specified as TRUE,
        then this argument is ignored.  Otherwise, it must be passed.

    BufferLength - This optional parameter indicates the length (in
        bytes) of the PreviousState buffer.  This value must be
        provided if the PreviousState parameter is provided.

    PreviousState - This (optional) parameter points to a buffer to
        receive the state of any groups actually changed by this
        request.  This information is formated as a TOKEN_GROUPS data
        structure which may be passed as the NewState parameter in a
        subsequent call to NtAdjustGroups to restore the original state
        of those groups.  TOKEN_QUERY access is needed to use this
        parameter.

        If this buffer does not contain enough space to receive the
        complete list of modified groups, then no group states are
        changed and STATUS_BUFFER_TOO_SMALL is returned.  In this
        case, the ReturnLength return parameter will contain the
        actual number of bytes needed to hold the information.

    ReturnLength - Indicates the actual number of bytes needed to
        contain the previous group state information.
        This parameter is ignored if the PreviousState argument is not
        passed.


Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = NtAdjustGroupsToken (
        TokenHandle,
        (BOOLEAN)ResetToDefault,
        NewState,
        BufferLength,
        PreviousState,
        ReturnLength
        );

    //
    // We need to set last error even for success because that
    // is the only way to tell if the api successfully assigned
    // all groups.  That is, STATUS_NOT_ALL_ASSIGNED is a
    // Success severity level.
    //

    BaseSetLastNTError(Status);


    if ( !NT_SUCCESS(Status) ) {
        return FALSE;
    }

    return TRUE;

}





BOOL
APIENTRY
PrivilegeCheck (
    HANDLE ClientToken,
    PPRIVILEGE_SET RequiredPrivileges,
    LPBOOL pfResult
    )
/*++

Routine Description:

    This routine tests the caller's client's security context to see if it
    contains the specified privileges.

    This API requires the caller have SeSecurityPrivilege privilege.
    The test for this privilege is always against the primary token of
    the calling process, not the impersonation token of the thread.


Arguments:

    ClientToken - A handle to a token object representing a client
        attempting access.  This handle must be obtained from a
        communication session layer, such as from an LPC Port or Local
        Named Pipe, to prevent possible security policy violations.

    RequiredPrivileges - Points to a set of privileges.  The client's
        security context is to be checked to see which of the specified
        privileges are present.  The results will be indicated in the
        attributes associated with each privilege.  Note that
        flags in this parameter indicate whether all the privileges listed
        are needed, or any of the privileges.

    pfResult - Receives a boolean flag indicating whether the client
        has all the specified privileges or not.  A value of TRUE
        indicates the client has all the specified privileges.
        Otherwise a value of FALSE is returned.


Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;
    BOOLEAN Result;

    Status = NtPrivilegeCheck (
                ClientToken,
                RequiredPrivileges,
                &Result
                );

    *pfResult = Result;

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}



BOOL
APIENTRY
AccessCheckAndAuditAlarmW(
    LPCWSTR SubsystemName,
    PVOID HandleId,
    LPWSTR ObjectTypeName,
    LPWSTR ObjectName,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    DWORD DesiredAccess,
    PGENERIC_MAPPING GenericMapping,
    BOOL ObjectCreation,
    LPDWORD GrantedAccess,
    LPBOOL AccessStatus,
    LPBOOL pfGenerateOnClose
    )
/*++

Routine Description:

    This routine compares the input Security Descriptor against the
    caller's impersonation token and indicates if access is granted or
    denied.  If access is granted then the desired access mask becomes
    the granted access mask for the object.  The semantics of the
    access check routine is described in the DSA Security Architecture
    workbook.

    This routine will also generate any necessary audit messages as a
    result of the access attempt.

Arguments:

    SubsystemName - Supplies a name string identifying the subsystem
        calling the routine.

    HandleId - A unique value that will be used to represent the client's
        handle to the object.  This value is ignored (and may be re-used)
        if the access is denied.

    ObjectTypeName - Supplies the name of the type of the object being
        created or accessed.

    ObjectName - Supplies the name of the object being created or accessed.

    SecurityDescriptor - A pointer to the Security Descriptor against which
        acccess is to be checked.

    DesiredAccess - The desired acccess mask.  This mask must have been
        previously mapped to contain no generic accesses.

    GenericMapping - Supplies a pointer to the generic mapping associated
        with this object type.

    ObjectCreation - A boolean flag indicated whether the access will
        result in a new object being created if granted.  A value of TRUE
        indicates an object will be created, FALSE indicates an existing
        object will be opened.

    GrantedAccess - Receives a masking indicating which accesses have been
        granted (only valid on success).

    AccessStatus - Receives an indication of the success or failure of the
        access check.  If access is granted, STATUS_SUCCESS is returned.
        If access is denied, a value appropriate for return to the client
        is returned.  This will be STATUS_ACCESS_DENIED or, when mandatory
        access controls are implemented, STATUS_OBJECT_NOT_FOUND.

    pfGenerateOnClose - Points to a boolean that is set by the audity
        generation routine and must be passed to ObjectCloseAuditAlarm
        when the object handle is closed.


Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;
    NTSTATUS RealAccessStatus;
    BOOLEAN GenerateOnClose;
    UNICODE_STRING Subsystem;
    UNICODE_STRING ObjectType;
    UNICODE_STRING Object;


    RtlInitUnicodeString(
        &Subsystem,
        SubsystemName
        );

    RtlInitUnicodeString(
        &ObjectType,
        ObjectTypeName
        );

    RtlInitUnicodeString(
        &Object,
        ObjectName
        );

    Status = NtAccessCheckAndAuditAlarm (
                &Subsystem,
                HandleId,
                &ObjectType,
                &Object,
                SecurityDescriptor,
                DesiredAccess,
                GenericMapping,
                (BOOLEAN)ObjectCreation,
                GrantedAccess,
                &RealAccessStatus,
                &GenerateOnClose
                );


    *pfGenerateOnClose = (BOOL)GenerateOnClose;

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    if ( !NT_SUCCESS( RealAccessStatus )) {
        *AccessStatus = FALSE;
        BaseSetLastNTError( RealAccessStatus );
        return( TRUE );
    }

    *AccessStatus = TRUE;
    return TRUE;
}

BOOL
APIENTRY
AccessCheckByTypeAndAuditAlarmW (
    LPCWSTR SubsystemName,
    LPVOID HandleId,
    LPCWSTR ObjectTypeName,
    LPCWSTR ObjectName,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    PSID PrincipalSelfSid,
    DWORD DesiredAccess,
    AUDIT_EVENT_TYPE AuditType,
    DWORD Flags,
    POBJECT_TYPE_LIST ObjectTypeList,
    DWORD ObjectTypeListLength,
    PGENERIC_MAPPING GenericMapping,
    BOOL ObjectCreation,
    LPDWORD GrantedAccess,
    LPBOOL AccessStatus,
    LPBOOL pfGenerateOnClose
    )
/*++

Routine Description:

    This routine compares the input Security Descriptor against the
    caller's impersonation token and indicates if access is granted or
    denied.  If access is granted then the desired access mask becomes
    the granted access mask for the object.  The semantics of the
    access check routine is described in the DSA Security Architecture
    workbook.

    This routine will also generate any necessary audit messages as a
    result of the access attempt.

Arguments:

    SubsystemName - Supplies a name string identifying the subsystem
        calling the routine.

    HandleId - A unique value that will be used to represent the client's
        handle to the object.  This value is ignored (and may be re-used)
        if the access is denied.

    ObjectTypeName - Supplies the name of the type of the object being
        created or accessed.

    ObjectName - Supplies the name of the object being created or accessed.

    SecurityDescriptor - A pointer to the Security Descriptor against which
        acccess is to be checked.

    PrincipalSelfSid - If the object being access checked is an object which
        represents a principal (e.g., a user object), this parameter should
        be the SID of the object.  Any ACE containing the constant
        PRINCIPAL_SELF_SID is replaced by this SID.

        The parameter should be NULL if the object does not represent a principal.

    DesiredAccess - The desired acccess mask.  This mask must have been
        previously mapped to contain no generic accesses.

    AuditType - Specifies the type of audit to be generated.  Valid values
        are: AuditEventObjectAccess and AuditEventDirectoryServiceAccess.

    Flags - Flags modifying the execution of the API:

        AUDIT_ALLOW_NO_PRIVILEGE - If the caller does not have AuditPrivilege,
            the call will silently continue to check access and will
            generate no audit.

    ObjectTypeList - Supplies a list of GUIDs representing the object (and
        sub-objects) being accessed.  If no list is present, AccessCheckByType
        behaves identically to AccessCheck.

    ObjectTypeListLength - Specifies the number of elements in the ObjectTypeList.

    GenericMapping - Supplies a pointer to the generic mapping associated
        with this object type.

    ObjectCreation - A boolean flag indicated whether the access will
        result in a new object being created if granted.  A value of TRUE
        indicates an object will be created, FALSE indicates an existing
        object will be opened.

    GrantedAccess - Receives a masking indicating which accesses have been
        granted (only valid on success).

    AccessStatus - Receives an indication of the success or failure of the
        access check.  If access is granted, STATUS_SUCCESS is returned.
        If access is denied, a value appropriate for return to the client
        is returned.  This will be STATUS_ACCESS_DENIED or, when mandatory
        access controls are implemented, STATUS_OBJECT_NOT_FOUND.

    pfGenerateOnClose - Points to a boolean that is set by the audity
        generation routine and must be passed to ObjectCloseAuditAlarm
        when the object handle is closed.


Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;
    NTSTATUS RealAccessStatus;
    BOOLEAN GenerateOnClose;
    UNICODE_STRING Subsystem;
    UNICODE_STRING ObjectType;
    UNICODE_STRING Object;

    RtlInitUnicodeString(
        &Subsystem,
        SubsystemName
        );

    RtlInitUnicodeString(
        &ObjectType,
        ObjectTypeName
        );

    RtlInitUnicodeString(
        &Object,
        ObjectName
        );

    Status = NtAccessCheckByTypeAndAuditAlarm (
                &Subsystem,
                HandleId,
                &ObjectType,
                &Object,
                SecurityDescriptor,
                PrincipalSelfSid,
                DesiredAccess,
                AuditType,
                Flags,
                ObjectTypeList,
                ObjectTypeListLength,
                GenericMapping,
                (BOOLEAN)ObjectCreation,
                GrantedAccess,
                &RealAccessStatus,
                &GenerateOnClose
                );


    *pfGenerateOnClose = (BOOL)GenerateOnClose;

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    if ( !NT_SUCCESS( RealAccessStatus )) {
        *AccessStatus = FALSE;
        BaseSetLastNTError( RealAccessStatus );
        return( TRUE );
    }

    *AccessStatus = TRUE;
    return TRUE;
}


BOOL
APIENTRY
AccessCheckByTypeResultListAndAuditAlarmW (
    LPCWSTR SubsystemName,
    LPVOID HandleId,
    LPCWSTR ObjectTypeName,
    LPCWSTR ObjectName,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    PSID PrincipalSelfSid,
    DWORD DesiredAccess,
    AUDIT_EVENT_TYPE AuditType,
    DWORD Flags,
    POBJECT_TYPE_LIST ObjectTypeList,
    DWORD ObjectTypeListLength,
    PGENERIC_MAPPING GenericMapping,
    BOOL ObjectCreation,
    LPDWORD GrantedAccessList,
    LPDWORD AccessStatusList,
    LPBOOL pfGenerateOnClose
    )
/*++

Routine Description:

    This routine compares the input Security Descriptor against the
    caller's impersonation token and indicates if access is granted or
    denied.  If access is granted then the desired access mask becomes
    the granted access mask for the object.  The semantics of the
    access check routine is described in the DSA Security Architecture
    workbook.

    This routine will also generate any necessary audit messages as a
    result of the access attempt.

Arguments:

    SubsystemName - Supplies a name string identifying the subsystem
        calling the routine.

    HandleId - A unique value that will be used to represent the client's
        handle to the object.  This value is ignored (and may be re-used)
        if the access is denied.

    ObjectTypeName - Supplies the name of the type of the object being
        created or accessed.

    ObjectName - Supplies the name of the object being created or accessed.

    SecurityDescriptor - A pointer to the Security Descriptor against which
        acccess is to be checked.

    PrincipalSelfSid - If the object being access checked is an object which
        represents a principal (e.g., a user object), this parameter should
        be the SID of the object.  Any ACE containing the constant
        PRINCIPAL_SELF_SID is replaced by this SID.

        The parameter should be NULL if the object does not represent a principal.

    DesiredAccess - The desired acccess mask.  This mask must have been
        previously mapped to contain no generic accesses.

    AuditType - Specifies the type of audit to be generated.  Valid values
        are: AuditEventObjectAccess and AuditEventDirectoryServiceAccess.

    Flags - Flags modifying the execution of the API:

        AUDIT_ALLOW_NO_PRIVILEGE - If the called does not have AuditPrivilege,
            the call will silently continue to check access and will
            generate no audit.

    ObjectTypeList - Supplies a list of GUIDs representing the object (and
        sub-objects) being accessed.  If no list is present, AccessCheckByType
        behaves identically to AccessCheck.

    ObjectTypeListLength - Specifies the number of elements in the ObjectTypeList.

    GenericMapping - Supplies a pointer to the generic mapping associated
        with this object type.

    ObjectCreation - A boolean flag indicated whether the access will
        result in a new object being created if granted.  A value of TRUE
        indicates an object will be created, FALSE indicates an existing
        object will be opened.

    GrantedAccessList - Returns an access mask describing the granted access.

    AccessStatusList - Status value that may be returned indicating the
         reason why access was denied.  Routines should avoid hardcoding a
         return value of STATUS_ACCESS_DENIED so that a different value can
         be returned when mandatory access control is implemented.

    pfGenerateOnClose - Points to a boolean that is set by the audity
        generation routine and must be passed to ObjectCloseAuditAlarm
        when the object handle is closed.


Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;
    NTSTATUS RealAccessStatus;
    BOOLEAN GenerateOnClose;
    UNICODE_STRING Subsystem;
    UNICODE_STRING ObjectType;
    UNICODE_STRING Object;
    ULONG i;


    RtlInitUnicodeString(
        &Subsystem,
        SubsystemName
        );

    RtlInitUnicodeString(
        &ObjectType,
        ObjectTypeName
        );

    RtlInitUnicodeString(
        &Object,
        ObjectName
        );

    Status = NtAccessCheckByTypeResultListAndAuditAlarm (
                &Subsystem,
                HandleId,
                &ObjectType,
                &Object,
                SecurityDescriptor,
                PrincipalSelfSid,
                DesiredAccess,
                AuditType,
                Flags,
                ObjectTypeList,
                ObjectTypeListLength,
                GenericMapping,
                (BOOLEAN)ObjectCreation,
                GrantedAccessList,
                AccessStatusList,
                &GenerateOnClose
                );


    *pfGenerateOnClose = (BOOL)GenerateOnClose;

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    //
    // Loop converting the array of NT status codes to WIN status codes.
    //

    for ( i=0; i<ObjectTypeListLength; i++ ) {
        if ( AccessStatusList[i] == STATUS_SUCCESS ) {
            AccessStatusList[i] = NO_ERROR;
        } else {
            AccessStatusList[i] = RtlNtStatusToDosError( AccessStatusList[i] );
        }
    }

    return TRUE;
}


BOOL
APIENTRY
AccessCheckByTypeResultListAndAuditAlarmByHandleW (
    LPCWSTR SubsystemName,
    LPVOID HandleId,
    HANDLE ClientToken,
    LPCWSTR ObjectTypeName,
    LPCWSTR ObjectName,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    PSID PrincipalSelfSid,
    DWORD DesiredAccess,
    AUDIT_EVENT_TYPE AuditType,
    DWORD Flags,
    POBJECT_TYPE_LIST ObjectTypeList,
    DWORD ObjectTypeListLength,
    PGENERIC_MAPPING GenericMapping,
    BOOL ObjectCreation,
    LPDWORD GrantedAccessList,
    LPDWORD AccessStatusList,
    LPBOOL pfGenerateOnClose
    )
/*++

Routine Description:

    This routine compares the input Security Descriptor against the
    caller's impersonation token and indicates if access is granted or
    denied.  If access is granted then the desired access mask becomes
    the granted access mask for the object.  The semantics of the
    access check routine is described in the DSA Security Architecture
    workbook.

    This routine will also generate any necessary audit messages as a
    result of the access attempt.

Arguments:

    SubsystemName - Supplies a name string identifying the subsystem
        calling the routine.

    HandleId - A unique value that will be used to represent the client's
        handle to the object.  This value is ignored (and may be re-used)
        if the access is denied.

    ClientToken - A handle to a token object representing the client that
        requested the operation.  This handle must be obtained from a
        communication session layer, such as from an LPC Port or Local
        Named Pipe, to prevent possible security policy violations.

    ObjectTypeName - Supplies the name of the type of the object being
        created or accessed.

    ObjectName - Supplies the name of the object being created or accessed.

    SecurityDescriptor - A pointer to the Security Descriptor against which
        acccess is to be checked.

    PrincipalSelfSid - If the object being access checked is an object which
        represents a principal (e.g., a user object), this parameter should
        be the SID of the object.  Any ACE containing the constant
        PRINCIPAL_SELF_SID is replaced by this SID.

        The parameter should be NULL if the object does not represent a principal.

    DesiredAccess - The desired acccess mask.  This mask must have been
        previously mapped to contain no generic accesses.

    AuditType - Specifies the type of audit to be generated.  Valid values
        are: AuditEventObjectAccess and AuditEventDirectoryServiceAccess.

    Flags - Flags modifying the execution of the API:

        AUDIT_ALLOW_NO_PRIVILEGE - If the called does not have AuditPrivilege,
            the call will silently continue to check access and will
            generate no audit.

    ObjectTypeList - Supplies a list of GUIDs representing the object (and
        sub-objects) being accessed.  If no list is present, AccessCheckByType
        behaves identically to AccessCheck.

    ObjectTypeListLength - Specifies the number of elements in the ObjectTypeList.

    GenericMapping - Supplies a pointer to the generic mapping associated
        with this object type.

    ObjectCreation - A boolean flag indicated whether the access will
        result in a new object being created if granted.  A value of TRUE
        indicates an object will be created, FALSE indicates an existing
        object will be opened.

    GrantedAccessList - Returns an access mask describing the granted access.

    AccessStatusList - Status value that may be returned indicating the
         reason why access was denied.  Routines should avoid hardcoding a
         return value of STATUS_ACCESS_DENIED so that a different value can
         be returned when mandatory access control is implemented.

    pfGenerateOnClose - Points to a boolean that is set by the audity
        generation routine and must be passed to ObjectCloseAuditAlarm
        when the object handle is closed.


Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;
    NTSTATUS RealAccessStatus;
    BOOLEAN GenerateOnClose;
    UNICODE_STRING Subsystem;
    UNICODE_STRING ObjectType;
    UNICODE_STRING Object;
    ULONG i;


    RtlInitUnicodeString(
        &Subsystem,
        SubsystemName
        );

    RtlInitUnicodeString(
        &ObjectType,
        ObjectTypeName
        );

    RtlInitUnicodeString(
        &Object,
        ObjectName
        );

    Status = NtAccessCheckByTypeResultListAndAuditAlarmByHandle (
                &Subsystem,
                HandleId,
                ClientToken,
                &ObjectType,
                &Object,
                SecurityDescriptor,
                PrincipalSelfSid,
                DesiredAccess,
                AuditType,
                Flags,
                ObjectTypeList,
                ObjectTypeListLength,
                GenericMapping,
                (BOOLEAN)ObjectCreation,
                GrantedAccessList,
                AccessStatusList,
                &GenerateOnClose
                );


    *pfGenerateOnClose = (BOOL)GenerateOnClose;

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    //
    // Loop converting the array of NT status codes to WIN status codes.
    //

    for ( i=0; i<ObjectTypeListLength; i++ ) {
        if ( AccessStatusList[i] == STATUS_SUCCESS ) {
            AccessStatusList[i] = NO_ERROR;
        } else {
            AccessStatusList[i] = RtlNtStatusToDosError( AccessStatusList[i] );
        }
    }

    return TRUE;
}


BOOL
APIENTRY
AccessCheckAndAuditAlarmA (
    LPCSTR SubsystemName,
    PVOID HandleId,
    LPSTR ObjectTypeName,
    LPSTR ObjectName,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    DWORD DesiredAccess,
    PGENERIC_MAPPING GenericMapping,
    BOOL ObjectCreation,
    LPDWORD GrantedAccess,
    LPBOOL AccessStatus,
    LPBOOL pfGenerateOnClose
    )
/*++

Routine Description:

    ANSI Thunk to AccessCheckAndAuditAlarmW

--*/
{
    PUNICODE_STRING ObjectNameW;
    ANSI_STRING AnsiString;
    UNICODE_STRING SubsystemNameW;
    UNICODE_STRING ObjectTypeNameW;
    NTSTATUS Status;
    BOOL RVal;


    ObjectNameW = &NtCurrentTeb()->StaticUnicodeString;


    RtlInitAnsiString(&AnsiString,SubsystemName);
    Status = RtlAnsiStringToUnicodeString(&SubsystemNameW,&AnsiString,TRUE);

    if ( !NT_SUCCESS(Status) ) {

        BaseSetLastNTError(Status);
        return FALSE;
    }



    RtlInitAnsiString(&AnsiString,ObjectTypeName);
    Status = RtlAnsiStringToUnicodeString(&ObjectTypeNameW,&AnsiString,TRUE);

    if ( !NT_SUCCESS(Status) ) {

        RtlFreeUnicodeString( &SubsystemNameW );

        BaseSetLastNTError(Status);
        return FALSE;
    }


    //
    // Convert the object name string, but don't allocate memory to
    // do it, since we've got the space in the TEB available.
    //

    RtlInitAnsiString(&AnsiString,ObjectName);
    Status = RtlAnsiStringToUnicodeString(ObjectNameW,&AnsiString,FALSE);

    if ( !NT_SUCCESS(Status) ) {

        RtlFreeUnicodeString( &SubsystemNameW );
        RtlFreeUnicodeString( &ObjectTypeNameW );

        BaseSetLastNTError(Status);
        return FALSE;
    }


    RVal =  AccessCheckAndAuditAlarmW (
                (LPCWSTR)SubsystemNameW.Buffer,
                HandleId,
                ObjectTypeNameW.Buffer,
                ObjectNameW->Buffer,
                SecurityDescriptor,
                DesiredAccess,
                GenericMapping,
                ObjectCreation,
                GrantedAccess,
                AccessStatus,
                pfGenerateOnClose
                );


    RtlFreeUnicodeString( &SubsystemNameW );
    RtlFreeUnicodeString( &ObjectTypeNameW );

    return( RVal );
}

BOOL
APIENTRY
AccessCheckByTypeAndAuditAlarmA (
    LPCSTR SubsystemName,
    PVOID HandleId,
    LPCSTR ObjectTypeName,
    LPCSTR ObjectName,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    PSID PrincipalSelfSid,
    DWORD DesiredAccess,
    AUDIT_EVENT_TYPE AuditType,
    DWORD Flags,
    POBJECT_TYPE_LIST ObjectTypeList,
    DWORD ObjectTypeListLength,
    PGENERIC_MAPPING GenericMapping,
    BOOL ObjectCreation,
    LPDWORD GrantedAccess,
    LPBOOL AccessStatus,
    LPBOOL pfGenerateOnClose
    )
/*++

Routine Description:

    ANSI Thunk to AccessCheckByTypeAndAuditAlarmW

--*/
{
    PUNICODE_STRING ObjectNameW;
    ANSI_STRING AnsiString;
    UNICODE_STRING SubsystemNameW;
    UNICODE_STRING ObjectTypeNameW;
    NTSTATUS Status;

    BOOL RVal;


    ObjectNameW = &NtCurrentTeb()->StaticUnicodeString;


    RtlInitAnsiString(&AnsiString,SubsystemName);
    Status = RtlAnsiStringToUnicodeString(&SubsystemNameW,&AnsiString,TRUE);

    if ( !NT_SUCCESS(Status) ) {

        BaseSetLastNTError(Status);
        return FALSE;
    }



    RtlInitAnsiString(&AnsiString,ObjectTypeName);
    Status = RtlAnsiStringToUnicodeString(&ObjectTypeNameW,&AnsiString,TRUE);

    if ( !NT_SUCCESS(Status) ) {

        RtlFreeUnicodeString( &SubsystemNameW );

        BaseSetLastNTError(Status);
        return FALSE;
    }


    //
    // Convert the object name string, but don't allocate memory to
    // do it, since we've got the space in the TEB available.
    //

    RtlInitAnsiString(&AnsiString,ObjectName);
    Status = RtlAnsiStringToUnicodeString(ObjectNameW,&AnsiString,FALSE);

    if ( !NT_SUCCESS(Status) ) {

        RtlFreeUnicodeString( &SubsystemNameW );
        RtlFreeUnicodeString( &ObjectTypeNameW );

        BaseSetLastNTError(Status);
        return FALSE;
    }


    RVal =  AccessCheckByTypeAndAuditAlarmW (
                (LPCWSTR)SubsystemNameW.Buffer,
                HandleId,
                ObjectTypeNameW.Buffer,
                ObjectNameW->Buffer,
                SecurityDescriptor,
                PrincipalSelfSid,
                DesiredAccess,
                AuditType,
                Flags,
                ObjectTypeList,
                ObjectTypeListLength,
                GenericMapping,
                ObjectCreation,
                GrantedAccess,
                AccessStatus,
                pfGenerateOnClose
                );


    RtlFreeUnicodeString( &SubsystemNameW );
    RtlFreeUnicodeString( &ObjectTypeNameW );

    return( RVal );
}

WINADVAPI
BOOL
WINAPI
AccessCheckByTypeResultListAndAuditAlarmA (
    LPCSTR SubsystemName,
    LPVOID HandleId,
    LPCSTR ObjectTypeName,
    LPCSTR ObjectName,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    PSID PrincipalSelfSid,
    DWORD DesiredAccess,
    AUDIT_EVENT_TYPE AuditType,
    DWORD Flags,
    POBJECT_TYPE_LIST ObjectTypeList,
    DWORD ObjectTypeListLength,
    PGENERIC_MAPPING GenericMapping,
    BOOL ObjectCreation,
    LPDWORD GrantedAccess,
    LPDWORD AccessStatusList,
    LPBOOL pfGenerateOnClose
    )
/*++

Routine Description:

    ANSI Thunk to AccessCheckByTypeResultListAndAuditAlarmW

--*/
{
    PUNICODE_STRING ObjectNameW;
    ANSI_STRING AnsiString;
    UNICODE_STRING SubsystemNameW;
    UNICODE_STRING ObjectTypeNameW;
    NTSTATUS Status;

    BOOL RVal;


    ObjectNameW = &NtCurrentTeb()->StaticUnicodeString;


    RtlInitAnsiString(&AnsiString,SubsystemName);
    Status = RtlAnsiStringToUnicodeString(&SubsystemNameW,&AnsiString,TRUE);

    if ( !NT_SUCCESS(Status) ) {

        BaseSetLastNTError(Status);
        return FALSE;
    }



    RtlInitAnsiString(&AnsiString,ObjectTypeName);
    Status = RtlAnsiStringToUnicodeString(&ObjectTypeNameW,&AnsiString,TRUE);

    if ( !NT_SUCCESS(Status) ) {

        RtlFreeUnicodeString( &SubsystemNameW );

        BaseSetLastNTError(Status);
        return FALSE;
    }


    //
    // Convert the object name string, but don't allocate memory to
    // do it, since we've got the space in the TEB available.
    //

    RtlInitAnsiString(&AnsiString,ObjectName);
    Status = RtlAnsiStringToUnicodeString(ObjectNameW,&AnsiString,FALSE);

    if ( !NT_SUCCESS(Status) ) {

        RtlFreeUnicodeString( &SubsystemNameW );
        RtlFreeUnicodeString( &ObjectTypeNameW );

        BaseSetLastNTError(Status);
        return FALSE;
    }


    RVal =  AccessCheckByTypeResultListAndAuditAlarmW (
                (LPCWSTR)SubsystemNameW.Buffer,
                HandleId,
                ObjectTypeNameW.Buffer,
                ObjectNameW->Buffer,
                SecurityDescriptor,
                PrincipalSelfSid,
                DesiredAccess,
                AuditType,
                Flags,
                ObjectTypeList,
                ObjectTypeListLength,
                GenericMapping,
                ObjectCreation,
                GrantedAccess,
                AccessStatusList,
                pfGenerateOnClose
                );

    RtlFreeUnicodeString( &SubsystemNameW );
    RtlFreeUnicodeString( &ObjectTypeNameW );

    return( RVal );
}


WINADVAPI
BOOL
WINAPI
AccessCheckByTypeResultListAndAuditAlarmByHandleA (
    LPCSTR SubsystemName,
    LPVOID HandleId,
    HANDLE ClientToken,
    LPCSTR ObjectTypeName,
    LPCSTR ObjectName,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    PSID PrincipalSelfSid,
    DWORD DesiredAccess,
    AUDIT_EVENT_TYPE AuditType,
    DWORD Flags,
    POBJECT_TYPE_LIST ObjectTypeList,
    DWORD ObjectTypeListLength,
    PGENERIC_MAPPING GenericMapping,
    BOOL ObjectCreation,
    LPDWORD GrantedAccess,
    LPDWORD AccessStatusList,
    LPBOOL pfGenerateOnClose
    )
/*++

Routine Description:

    ANSI Thunk to AccessCheckByTypeResultListAndAuditAlarmW

--*/
{
    PUNICODE_STRING ObjectNameW;
    ANSI_STRING AnsiString;
    UNICODE_STRING SubsystemNameW;
    UNICODE_STRING ObjectTypeNameW;
    NTSTATUS Status;

    BOOL RVal;


    ObjectNameW = &NtCurrentTeb()->StaticUnicodeString;


    RtlInitAnsiString(&AnsiString,SubsystemName);
    Status = RtlAnsiStringToUnicodeString(&SubsystemNameW,&AnsiString,TRUE);

    if ( !NT_SUCCESS(Status) ) {

        BaseSetLastNTError(Status);
        return FALSE;
    }



    RtlInitAnsiString(&AnsiString,ObjectTypeName);
    Status = RtlAnsiStringToUnicodeString(&ObjectTypeNameW,&AnsiString,TRUE);

    if ( !NT_SUCCESS(Status) ) {

        RtlFreeUnicodeString( &SubsystemNameW );

        BaseSetLastNTError(Status);
        return FALSE;
    }


    //
    // Convert the object name string, but don't allocate memory to
    // do it, since we've got the space in the TEB available.
    //

    RtlInitAnsiString(&AnsiString,ObjectName);
    Status = RtlAnsiStringToUnicodeString(ObjectNameW,&AnsiString,FALSE);

    if ( !NT_SUCCESS(Status) ) {

        RtlFreeUnicodeString( &SubsystemNameW );
        RtlFreeUnicodeString( &ObjectTypeNameW );

        BaseSetLastNTError(Status);
        return FALSE;
    }


    RVal =  AccessCheckByTypeResultListAndAuditAlarmByHandleW (
                (LPCWSTR)SubsystemNameW.Buffer,
                HandleId,
                ClientToken,
                ObjectTypeNameW.Buffer,
                ObjectNameW->Buffer,
                SecurityDescriptor,
                PrincipalSelfSid,
                DesiredAccess,
                AuditType,
                Flags,
                ObjectTypeList,
                ObjectTypeListLength,
                GenericMapping,
                ObjectCreation,
                GrantedAccess,
                AccessStatusList,
                pfGenerateOnClose
                );

    RtlFreeUnicodeString( &SubsystemNameW );
    RtlFreeUnicodeString( &ObjectTypeNameW );

    return( RVal );
}

BOOL
APIENTRY
ObjectOpenAuditAlarmA (
    LPCSTR SubsystemName,
    PVOID HandleId,
    LPSTR ObjectTypeName,
    LPSTR ObjectName,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    HANDLE ClientToken,
    DWORD DesiredAccess,
    DWORD GrantedAccess,
    PPRIVILEGE_SET Privileges OPTIONAL,
    BOOL ObjectCreation,
    BOOL AccessGranted,
    LPBOOL GenerateOnClose
    )
/*++

Routine Description:

    ANSI Thunk to ObjectOpenAuditAlarmW

--*/
{
    PUNICODE_STRING ObjectNameW;
    ANSI_STRING AnsiString;
    UNICODE_STRING SubsystemNameW;
    UNICODE_STRING ObjectTypeNameW;
    NTSTATUS Status;
    BOOL RVal;


    ObjectNameW = &NtCurrentTeb()->StaticUnicodeString;


    RtlInitAnsiString(&AnsiString,SubsystemName);
    Status = RtlAnsiStringToUnicodeString(&SubsystemNameW,&AnsiString,TRUE);

    if ( !NT_SUCCESS(Status) ) {

        BaseSetLastNTError(Status);
        return FALSE;
    }



    RtlInitAnsiString(&AnsiString,ObjectTypeName);
    Status = RtlAnsiStringToUnicodeString(&ObjectTypeNameW,&AnsiString,TRUE);

    if ( !NT_SUCCESS(Status) ) {

        RtlFreeUnicodeString( &SubsystemNameW );

        BaseSetLastNTError(Status);
        return FALSE;
    }


    //
    // Convert the object name string, but don't allocate memory to
    // do it, since we've got the space in the TEB available.
    //

    RtlInitAnsiString(&AnsiString,ObjectName);
    Status = RtlAnsiStringToUnicodeString(ObjectNameW,&AnsiString,FALSE);

    if ( !NT_SUCCESS(Status) ) {

        RtlFreeUnicodeString( &SubsystemNameW );
        RtlFreeUnicodeString( &ObjectTypeNameW );

        BaseSetLastNTError(Status);
        return FALSE;
    }

    RVal = ObjectOpenAuditAlarmW (
               (LPCWSTR)SubsystemNameW.Buffer,
               HandleId,
               ObjectTypeNameW.Buffer,
               ObjectNameW->Buffer,
               pSecurityDescriptor,
               ClientToken,
               DesiredAccess,
               GrantedAccess,
               Privileges,
               ObjectCreation,
               AccessGranted,
               GenerateOnClose
               );

    RtlFreeUnicodeString( &SubsystemNameW );
    RtlFreeUnicodeString( &ObjectTypeNameW );

    return( RVal );

}



BOOL
APIENTRY
ObjectOpenAuditAlarmW (
    LPCWSTR SubsystemName,
    PVOID HandleId,
    LPWSTR ObjectTypeName,
    LPWSTR ObjectName,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    HANDLE ClientToken,
    DWORD DesiredAccess,
    DWORD GrantedAccess,
    PPRIVILEGE_SET Privileges OPTIONAL,
    BOOL ObjectCreation,
    BOOL AccessGranted,
    LPBOOL GenerateOnClose
    )
/*++

    Routine Description:

    This routine is used to generate audit and alarm messages when an
    attempt is made to access an existing protected subsystem object or
    create a new one.  This routine may result in several messages being
    generated and sent to Port objects.  This may result in a significant
    latency before returning.  Design of routines that must call this
    routine must take this potential latency into account.  This may have
    an impact on the approach taken for data structure mutex locking, for
    example.

    This routine may not be able to generate a complete audit record
    due to memory restrictions.

    This API requires the caller have SeSecurityPrivilege privilege.
    The test for this privilege is always against the primary token of
    the calling process, not the impersonation token of the thread.

Arguments:

    SubsystemName - Supplies a name string identifying the
        subsystem calling the routine.

    HandleId - A unique value representing the client's handle to the
        object.  If the access attempt was not successful (AccessGranted is
        FALSE), then this parameter is ignored.

    ObjectTypeName - Supplies the name of the type of object being
        accessed.

    ObjectName - Supplies the name of the object the client
        accessed or attempted to access.

    pSecurityDescriptor - An optional pointer to the security
        descriptor of the object being accessed.

    ClientToken - A handle to a token object representing the client that
        requested the operation.  This handle must be obtained from a
        communication session layer, such as from an LPC Port or Local
        Named Pipe, to prevent possible security policy violations.

    DesiredAccess - The desired access mask.  This mask must have been
        previously mapped to contain no generic accesses.

    GrantedAccess - The mask of accesses that were actually granted.

    Privileges - Optionally points to a set of privileges that were
        required for the access attempt.  Those privileges that were held
        by the subject are marked using the UsedForAccess flag of the
        attributes associated with each privilege.

    ObjectCreation - A boolean flag indicating whether the access will
        result in a new object being created if granted.  A value of TRUE
        indicates an object will be created, FALSE indicates an existing
        object will be opened.

    AccessGranted - Indicates whether the requested access was granted or
        not.  A value of TRUE indicates the access was granted.  A value of
        FALSE indicates the access was not granted.

    GenerateOnClose - Points to a boolean that is set by the audit
        generation routine and must be passed to NtCloseObjectAuditAlarm()
        when the object handle is closed.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING Subsystem;
    UNICODE_STRING ObjectType;
    UNICODE_STRING Object;


    RtlInitUnicodeString(
        &Subsystem,
        SubsystemName
        );

    RtlInitUnicodeString(
        &ObjectType,
        ObjectTypeName
        );

    RtlInitUnicodeString(
        &Object,
        ObjectName
        );

    Status = NtOpenObjectAuditAlarm (
                &Subsystem,
                &HandleId,
                &ObjectType,
                &Object,
                pSecurityDescriptor,
                ClientToken,
                DesiredAccess,
                GrantedAccess,
                Privileges,
                (BOOLEAN)ObjectCreation,
                (BOOLEAN)AccessGranted,
                (PBOOLEAN)GenerateOnClose
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}



BOOL
APIENTRY
ObjectPrivilegeAuditAlarmA (
    LPCSTR SubsystemName,
    PVOID HandleId,
    HANDLE ClientToken,
    DWORD DesiredAccess,
    PPRIVILEGE_SET Privileges,
    BOOL AccessGranted
    )
/*++

Routine Description:

    ANSI Thunk to ObjectPrivilegeAuditAlarmW

--*/
{
    PUNICODE_STRING SubsystemNameW;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    BOOL RVal;

    SubsystemNameW = &NtCurrentTeb()->StaticUnicodeString;

    //
    // Convert the object name string, but don't allocate memory to
    // do it, since we've got the space in the TEB available.
    //

    RtlInitAnsiString(&AnsiString,SubsystemName);
    Status = RtlAnsiStringToUnicodeString(SubsystemNameW,&AnsiString,FALSE);

    if ( !NT_SUCCESS(Status) ) {

        BaseSetLastNTError(Status);
        return FALSE;
    }

    RVal = ObjectPrivilegeAuditAlarmW (
                (LPCWSTR)SubsystemNameW->Buffer,
                HandleId,
                ClientToken,
                DesiredAccess,
                Privileges,
                AccessGranted
                );

    return( RVal );
}




BOOL
APIENTRY
ObjectPrivilegeAuditAlarmW (
    LPCWSTR SubsystemName,
    PVOID HandleId,
    HANDLE ClientToken,
    DWORD DesiredAccess,
    PPRIVILEGE_SET Privileges,
    BOOL AccessGranted
    )
/*++

Routine Description:

    This routine is used to generate audit and alarm messages when an
    attempt is made to perform privileged operations on a protected
    subsystem object after the object is already opened.  This routine
    may result in several messages being generated and sent to Port
    objects.  This may result in a significant latency before
    returning.  Design of routines that must call this routine must
    take this potential latency into account.  This may have an impact
    on the approach taken for data structure mutex locking, for
    example.

    This API requires the caller have SeSecurityPrivilege privilege.
    The test for this privilege is always against the primary token of
    the calling process, allowing the caller to be impersonating a
    client during the call with no ill effects.

Arguments:

    SubsystemName - Supplies a name string identifying the subsystem
        calling the routine.

    HandleId - A unique value representing the client's handle to the
        object.

    ClientToken - A handle to a token object representing the client that
        requested the operation.  This handle must be obtained from a
        communication session layer, such as from an LPC Port or Local
        Named Pipe, to prevent possible security policy violations.

    DesiredAccess - The desired access mask.  This mask must have been
        previously mapped to contain no generic accesses.

    Privileges - The set of privileges required for the requested
        operation.  Those privileges that were held by the subject are
        marked using the UsedForAccess flag of the attributes
        associated with each privilege.

    AccessGranted - Indicates whether the requested access was granted or
        not.  A value of TRUE indicates the access was granted.  A value of
        FALSE indicates the access was not granted.

Return value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING Subsystem;

    RtlInitUnicodeString(
        &Subsystem,
        SubsystemName
        );

    Status = NtPrivilegeObjectAuditAlarm (
        &Subsystem,
        HandleId,
        ClientToken,
        DesiredAccess,
        Privileges,
        (BOOLEAN)AccessGranted
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


BOOL
APIENTRY
ObjectCloseAuditAlarmA (
    LPCSTR SubsystemName,
    PVOID HandleId,
    BOOL GenerateOnClose
    )
/*++

Routine Description:

    ANSI Thunk to ObjectCloseAuditAlarmW

--*/
{
    PUNICODE_STRING SubsystemNameW;
    NTSTATUS Status;
    ANSI_STRING AnsiString;

    SubsystemNameW = &NtCurrentTeb()->StaticUnicodeString;

    //
    // Convert the object name string, but don't allocate memory to
    // do it, since we've got the space in the TEB available.
    //

    RtlInitAnsiString(&AnsiString,SubsystemName);
    Status = RtlAnsiStringToUnicodeString(SubsystemNameW,&AnsiString,FALSE);

    if ( !NT_SUCCESS(Status) ) {

        BaseSetLastNTError(Status);
        return FALSE;
    }

    return ObjectCloseAuditAlarmW (
               (LPCWSTR)SubsystemNameW->Buffer,
               HandleId,
               GenerateOnClose
               );


}

BOOL
APIENTRY
ObjectCloseAuditAlarmW (
    LPCWSTR SubsystemName,
    PVOID HandleId,
    BOOL GenerateOnClose
    )
/*++

Routine Description:

    This routine is used to generate audit and alarm messages when a handle
    to a protected subsystem object is deleted.  This routine may result in
    several messages being generated and sent to Port objects.  This may
    result in a significant latency before returning.  Design of routines
    that must call this routine must take this potential latency into
    account.  This may have an impact on the approach taken for data
    structure mutex locking, for example.

    This API requires the caller have SeSecurityPrivilege privilege.  The test
    for this privilege is always against the primary token of the calling
    process, allowing the caller to be impersonating a client during the
    call with no ill effects.

Arguments:

    SubsystemName - Supplies a name string identifying the subsystem
        calling the routine.

    HandleId - A unique value representing the client's handle to the
        object.

    GenerateOnClose - Is a boolean value returned from a corresponding
        AccessCheckAndAuditAlarm() call or ObjectOpenAuditAlarm() call
        when the object handle was created.

Return value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.


--*/
{
    NTSTATUS Status;
    UNICODE_STRING Subsystem;

    RtlInitUnicodeString( &Subsystem, SubsystemName );

    Status = NtCloseObjectAuditAlarm (
        &Subsystem,
        HandleId,
        (BOOLEAN)GenerateOnClose
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


BOOL
APIENTRY
ObjectDeleteAuditAlarmA (
    LPCSTR SubsystemName,
    PVOID HandleId,
    BOOL GenerateOnClose
    )
/*++

Routine Description:

    ANSI Thunk to ObjectDeleteAuditAlarmW

--*/
{
    PUNICODE_STRING SubsystemNameW;
    NTSTATUS Status;
    ANSI_STRING AnsiString;

    SubsystemNameW = &NtCurrentTeb()->StaticUnicodeString;

    //
    // Convert the object name string, but don't allocate memory to
    // do it, since we've got the space in the TEB available.
    //

    RtlInitAnsiString(&AnsiString,SubsystemName);
    Status = RtlAnsiStringToUnicodeString(SubsystemNameW,&AnsiString,FALSE);

    if ( !NT_SUCCESS(Status) ) {

        BaseSetLastNTError(Status);
        return FALSE;
    }

    return ObjectDeleteAuditAlarmW (
               (LPCWSTR)SubsystemNameW->Buffer,
               HandleId,
               GenerateOnClose
               );


}

BOOL
APIENTRY
ObjectDeleteAuditAlarmW (
    LPCWSTR SubsystemName,
    PVOID HandleId,
    BOOL GenerateOnClose
    )
/*++

Routine Description:

    This routine is used to generate audit and alarm messages when an object
    in a protected subsystem is deleted.  This routine may result in
    several messages being generated and sent to Port objects.  This may
    result in a significant latency before returning.  Design of routines
    that must call this routine must take this potential latency into
    account.  This may have an impact on the approach taken for data
    structure mutex locking, for example.

    This API requires the caller have SeSecurityPrivilege privilege.  The test
    for this privilege is always against the primary token of the calling
    process, allowing the caller to be impersonating a client during the
    call with no ill effects.

Arguments:

    SubsystemName - Supplies a name string identifying the subsystem
        calling the routine.

    HandleId - A unique value representing the client's handle to the
        object.

    GenerateOnClose - Is a boolean value returned from a corresponding
        AccessCheckAndAuditAlarm() call or ObjectOpenAuditAlarm() call
        when the object handle was created.

Return value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.


--*/
{
    NTSTATUS Status;
    UNICODE_STRING Subsystem;

    RtlInitUnicodeString( &Subsystem, SubsystemName );

    Status = NtDeleteObjectAuditAlarm (
        &Subsystem,
        HandleId,
        (BOOLEAN)GenerateOnClose
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}



BOOL
APIENTRY
PrivilegedServiceAuditAlarmA (
    LPCSTR SubsystemName,
    LPCSTR ServiceName,
    HANDLE ClientToken,
    PPRIVILEGE_SET Privileges,
    BOOL AccessGranted
    )
/*++

Routine Description:

    ANSI Thunk to PrivilegedServiceAuditAlarmW

--*/
{
    PUNICODE_STRING ServiceNameW;
    UNICODE_STRING SubsystemNameW;
    ANSI_STRING  AnsiString;
    NTSTATUS Status;
    BOOL RVal;

    ServiceNameW = &NtCurrentTeb()->StaticUnicodeString;

    //
    // Convert the object name string, but don't allocate memory to
    // do it, since we've got the space in the TEB available.
    //

    RtlInitAnsiString(&AnsiString,ServiceName);
    Status = RtlAnsiStringToUnicodeString(ServiceNameW,&AnsiString,FALSE);

    if ( !NT_SUCCESS(Status) ) {

        BaseSetLastNTError(Status);
        return FALSE;
    }

    RtlInitAnsiString(&AnsiString,SubsystemName);
    Status = RtlAnsiStringToUnicodeString(&SubsystemNameW,&AnsiString,TRUE);

    if ( !NT_SUCCESS(Status) ) {

        BaseSetLastNTError(Status);
        return FALSE;
    }

    RVal =  PrivilegedServiceAuditAlarmW (
                (LPCWSTR)SubsystemNameW.Buffer,
                (LPCWSTR)ServiceNameW->Buffer,
                ClientToken,
                Privileges,
                AccessGranted
                );

    RtlFreeUnicodeString( &SubsystemNameW );

    return( RVal );

}

BOOL
APIENTRY
PrivilegedServiceAuditAlarmW (
    LPCWSTR SubsystemName,
    LPCWSTR ServiceName,
    HANDLE ClientToken,
    PPRIVILEGE_SET Privileges,
    BOOL AccessGranted
    )
/*++

Routine Description:

    This routine is used to generate audit and alarm messages when an
    attempt is made to perform privileged system service operations.  This
    routine may result in several messages being generated and sent to Port
    objects.  This may result in a significant latency before returning.
    Design of routines that must call this routine must take this potential
    latency into account.  This may have an impact on the approach taken
    for data structure mutex locking, for example.

    This API requires the caller have SeSecurityPrivilege privilege.  The test
    for this privilege is always against the primary token of the calling
    process, allowing the caller to be impersonating a client during the
    call with no ill effects

Arguments:

    SubsystemName - Supplies a name string identifying the subsystem
        calling the routine.

    ServiceName - Supplies a name of the privileged subsystem service.  For
        example, "RESET RUNTIME LOCAL SECURITY POLICY" might be specified
        by a Local Security Authority service used to update the local
        security policy database.

    ClientToken - A handle to a token object representing the client that
        requested the operation.  This handle must be obtained from a
        communication session layer, such as from an LPC Port or Local
        Named Pipe, to prevent possible security policy violations.

    Privileges - Points to a set of privileges required to perform the
        privileged operation.  Those privileges that were held by the
        subject are marked using the UsedForAccess flag of the
        attributes associated with each privilege.

    AccessGranted - Indicates whether the requested access was granted or
        not.  A value of TRUE indicates the access was granted.  A value of
        FALSE indicates the access was not granted.


Return value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.


--*/
{
    NTSTATUS Status;
    UNICODE_STRING Subsystem;
    UNICODE_STRING Service;

    RtlInitUnicodeString( &Subsystem, SubsystemName );

    RtlInitUnicodeString( &Service, ServiceName );

    Status = NtPrivilegedServiceAuditAlarm (
        &Subsystem,
        &Service,
        ClientToken,
        Privileges,
        (BOOLEAN)AccessGranted
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
IsValidSid (
    PSID pSid
    )
/*++

Routine Description:

    This procedure validates an SID's structure.

Arguments:

    pSid - Pointer to the SID structure to validate.

Return Value:

    BOOLEAN - TRUE if the structure of pSid is valid.

--*/
{
    if ( !RtlValidSid ( pSid ) ) {
        SetLastError(ERROR_INVALID_SID);
        return FALSE;
    }

    return TRUE;


}




BOOL
APIENTRY
EqualSid (
    PSID pSid1,
    PSID pSid2
    )
/*++

Routine Description:

    This procedure tests two SID values for equality.

Arguments:

    pSid1, pSid2 - Supply pointers to the two SID values to compare.
        The SID structures are assumed to be valid.

Return Value:

    BOOLEAN - TRUE if the value of pSid1 is equal to pSid2, and FALSE
        otherwise.

--*/
{
    SetLastError(0);
    return (BOOL) RtlEqualSid (
                    pSid1,
                    pSid2
                    );
}




BOOL
APIENTRY
EqualPrefixSid (
    PSID pSid1,
    PSID pSid2
    )
/*++

Routine Description:

    This procedure tests two SID prefix values for equality.

    An SID prefix is the entire SID except for the last sub-authority
    value.

Arguments:

    pSid1, pSid2 - Supply pointers to the two SID values to compare.
        The SID structures are assumed to be valid.

Return Value:

    BOOLEAN - TRUE if the prefix value of pSid1 is equal to pSid2, and
        FALSE otherwise.

--*/
{
    SetLastError(0);
    return (BOOL) RtlEqualPrefixSid (
                    pSid1,
                    pSid2
                    );
}




DWORD
APIENTRY
GetSidLengthRequired (
    UCHAR nSubAuthorityCount
    )
/*++

Routine Description:

    This routine returns the length, in bytes, required to store an SID
    with the specified number of Sub-Authorities.

Arguments:

    nSubAuthorityCount - The number of sub-authorities to be stored in
        the SID.

Return Value:

    DWORD - The length, in bytes, required to store the SID.

--*/
{
    return RtlLengthRequiredSid (
                nSubAuthorityCount
                );
}



BOOL
APIENTRY
InitializeSid (
    PSID Sid,
    PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
    BYTE nSubAuthorityCount
    )

/*++

Routine Description:

    This function initializes an SID data structure.  It does not,
    however, set the sub-authority values.  This must be done
    separately.

Arguments:

    Sid - Pointer to the SID data structure to initialize.

    pIdentifierAuthority - Pointer to the Identifier Authority value
        to set in the SID.

    nSubAuthorityCount - The number of sub-authorities that will be
        placed in the SID (a separate action).

Return Value:

    None

--*/
{
    NTSTATUS Status;

    Status = RtlInitializeSid (
                Sid,
                pIdentifierAuthority,
                nSubAuthorityCount
                );

    if ( !NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( FALSE );
    }

    return( TRUE );
}



PVOID
APIENTRY
FreeSid(
    PSID pSid
    )

/*++

Routine Description:

    This function is used to free a SID previously allocated using
    AllocateAndInitializeSid().


Arguments:

    Sid - Pointer to the SID to free.

Return Value:

    None.


--*/
{
    return(RtlFreeSid( pSid ));
}



BOOL
APIENTRY
AllocateAndInitializeSid (
    PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
    BYTE nSubAuthorityCount,
    DWORD nSubAuthority0,
    DWORD nSubAuthority1,
    DWORD nSubAuthority2,
    DWORD nSubAuthority3,
    DWORD nSubAuthority4,
    DWORD nSubAuthority5,
    DWORD nSubAuthority6,
    DWORD nSubAuthority7,
    PSID *pSid
    )

/*++

Routine Description:

    This function allocates and initializes a sid with the specified
    number of sub-authorities (up to 8).  A sid allocated with this
    routine must be freed using FreeSid().


Arguments:

    pIdentifierAuthority - Pointer to the Identifier Authority value to
        set in the SID.

    nSubAuthorityCount - The number of sub-authorities to place in the SID.
        This also identifies how many of the SubAuthorityN parameters
        have meaningful values.  This must contain a value from 0 through
        8.

    nSubAuthority0-7 - Provides the corresponding sub-authority value to
        place in the SID.  For example, a SubAuthorityCount value of 3
        indicates that SubAuthority0, SubAuthority1, and SubAuthority0
        have meaningful values and the rest are to be ignored.

    Sid - Receives a pointer to the allocated and initialized SID data
        structure.

Return Value:


    ERROR_NO_MEMORY - The attempt to allocate memory for the SID
        failed.

    ERROR_INVALID_SID - The number of sub-authorities specified did
        not fall in the valid range for this api (0 through 8).

--*/
{
    NTSTATUS Status;

    Status = RtlAllocateAndInitializeSid (
                 pIdentifierAuthority,
                 (UCHAR)nSubAuthorityCount,
                 (ULONG)nSubAuthority0,
                 (ULONG)nSubAuthority1,
                 (ULONG)nSubAuthority2,
                 (ULONG)nSubAuthority3,
                 (ULONG)nSubAuthority4,
                 (ULONG)nSubAuthority5,
                 (ULONG)nSubAuthority6,
                 (ULONG)nSubAuthority7,
                 pSid
                 );

    if ( !NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( FALSE );
    }

    return( TRUE );
}




PSID_IDENTIFIER_AUTHORITY
GetSidIdentifierAuthority (
    PSID pSid
    )
/*++

Routine Description:

    This function returns the address of an SID's IdentifierAuthority field.

Arguments:

    Sid - Pointer to the SID data structure.

Return Value:

    Address of an SID's Identifier Authority field.

--*/
{
    SetLastError(0);
    return RtlIdentifierAuthoritySid (
               pSid
               );
}




PDWORD
GetSidSubAuthority (
    PSID pSid,
    DWORD nSubAuthority
    )
/*++

Routine Description:

    This function returns the address of a sub-authority array element of
    an SID.

Arguments:

    pSid - Pointer to the SID data structure.

    nSubAuthority - An index indicating which sub-authority is being
        specified.  This value is not compared against the number of
        sub-authorities in the SID for validity.

Return Value:

    Address of a relative ID within the SID.

--*/
{
    SetLastError(0);
    return RtlSubAuthoritySid (
               pSid,
               nSubAuthority
               );
}

PUCHAR
GetSidSubAuthorityCount (
    PSID pSid
    )
/*++

Routine Description:

    This function returns the address of the sub-authority count field of
    an SID.

Arguments:

    pSid - Pointer to the SID data structure.

Return Value:

    Address of the sub-authority count field of an SID.


--*/
{
    SetLastError(0);
    return RtlSubAuthorityCountSid (
               pSid
               );
}



DWORD
APIENTRY
GetLengthSid (
    PSID pSid
    )
/*++

Routine Description:

    This routine returns the length, in bytes, of a structurally valid SID.

Arguments:

    pSid - Points to the SID whose length is to be returned.  The
        SID's structure is assumed to be valid.

Return Value:

    DWORD - The length, in bytes, of the SID.

--*/
{
    SetLastError(0);
    return RtlLengthSid (
                pSid
                );
}



BOOL
APIENTRY
CopySid (
    DWORD nDestinationSidLength,
    PSID pDestinationSid,
    PSID pSourceSid
    )
/*++

Routine Description:

    This routine copies the value of the source SID to the destination
    SID.

Arguments:

    nDestinationSidLength - Indicates the length, in bytes, of the
        destination SID buffer.

    pDestinationSid - Pointer to a buffer to receive a copy of the
        source Sid value.

    pSourceSid - Supplies the Sid value to be copied.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlCopySid (
                nDestinationSidLength,
                pDestinationSid,
                pSourceSid
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
AreAllAccessesGranted (
    DWORD GrantedAccess,
    DWORD DesiredAccess
    )
/*++

Routine Description:

    This routine is used to check a desired access mask against a
    granted access mask.

Arguments:

        GrantedAccess - Specifies the granted access mask.

        DesiredAccess - Specifies the desired access mask.

Return Value:

    BOOL - TRUE if the GrantedAccess mask has all the bits set that
        the DesiredAccess mask has set.  That is, TRUE is returned if
        all of the desired accesses have been granted.

--*/
{
    return (BOOL) RtlAreAllAccessesGranted (
        GrantedAccess,
        DesiredAccess
        );
}




BOOL
APIENTRY
AreAnyAccessesGranted (
    DWORD GrantedAccess,
    DWORD DesiredAccess
    )
/*++

Routine Description:

    This routine is used to test whether any of a set of desired
    accesses are granted by a granted access mask.

Arguments:

        GrantedAccess - Specifies the granted access mask.

        DesiredAccess - Specifies the desired access mask.

Return Value:

    BOOL - TRUE if the GrantedAccess mask contains any of the bits
        specified in the DesiredAccess mask.  That is, if any of the
        desired accesses have been granted, TRUE is returned.


--*/
{
    return (BOOL) RtlAreAnyAccessesGranted (
        GrantedAccess,
        DesiredAccess
        );
}




VOID
APIENTRY
MapGenericMask (
    PDWORD AccessMask,
    PGENERIC_MAPPING GenericMapping
    )
/*++

Routine Description:

    This routine maps all generic accesses in the provided access mask
    to specific and standard accesses according to the provided
    GenericMapping.  The resulting mask will not have any of the
    generic bits set (GenericRead, GenericWrite, GenericExecute, or
    GenericAll) or any undefined bits set, but may have any other bit
    set.  If bits other than the generic bits are provided on input,
    they will not be cleared bt the mapping.

Arguments:

    AccessMask - Points to the access mask to be mapped.

    GenericMapping - The mapping of generic to specific and standard
        access types.

Return Value:

    None.

--*/
{
    RtlMapGenericMask (
        AccessMask,
        GenericMapping
        );
}



BOOL
APIENTRY
IsValidAcl (
    PACL pAcl
    )
/*++

Routine Description:

    This procedure validates an ACL.

    This involves validating the revision level of the ACL and ensuring
    that the number of ACEs specified in the AceCount fit in the space
    specified by the AclSize field of the ACL header.

Arguments:

    pAcl - Pointer to the ACL structure to validate.

Return Value:

    BOOLEAN - TRUE if the structure of Acl is valid.


--*/
{
    if ( !RtlValidAcl( pAcl ) ) {
        SetLastError(ERROR_INVALID_ACL);
        return FALSE;
    }
    return TRUE;
}




BOOL
APIENTRY
InitializeAcl (
    PACL pAcl,
    DWORD nAclLength,
    DWORD dwAclRevision
    )
/*++

Routine Description:

    InitializeAcl creates a new ACL in the caller supplied memory
    buffer.  The ACL contains zero ACEs; therefore, it is an empty ACL
    as opposed to a nonexistent ACL.  That is, if the ACL is now set
    to an object it will implicitly deny access to everyone.

Arguments:

    pAcl - Supplies the buffer containing the ACL being initialized

    nAclLength - Supplies the length of the ace buffer in bytes

    dwAclRevision - Supplies the revision for this Acl

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlCreateAcl (
                pAcl,
                nAclLength,
                dwAclRevision
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}



BOOL
APIENTRY
GetAclInformation (
    PACL pAcl,
    PVOID pAclInformation,
    DWORD nAclInformationLength,
    ACL_INFORMATION_CLASS dwAclInformationClass
    )
/*++

Routine Description:

    This routine returns to the caller information about an ACL.  The requested
    information can be AclRevisionInformation, or AclSizeInformation.

Arguments:

    pAcl - Supplies the Acl being examined

    pAclInformation - Supplies the buffer to receive the information
        being requested

    nAclInformationLength - Supplies the length of the AclInformation
        buffer in bytes

    dwAclInformationClass - Supplies the type of information being
        requested

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlQueryInformationAcl (
                pAcl,
                pAclInformation,
                nAclInformationLength,
                dwAclInformationClass
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
SetAclInformation (
    PACL pAcl,
    PVOID pAclInformation,
    DWORD nAclInformationLength,
    ACL_INFORMATION_CLASS dwAclInformationClass
    )
/*++

Routine Description:

    This routine sets the state of an ACL.  For now only the revision
    level can be set and for now only a revision level of 1 is accepted
    so this procedure is rather simple

Arguments:

    pAcl - Supplies the Acl being altered

    pAclInformation - Supplies the buffer containing the information
        being set

    nAclInformationLength - Supplies the length of the Acl information
        buffer

    dwAclInformationClass - Supplies the type of information begin set

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlSetInformationAcl (
                pAcl,
                pAclInformation,
                nAclInformationLength,
                dwAclInformationClass
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
AddAce (
    PACL pAcl,
    DWORD dwAceRevision,
    DWORD dwStartingAceIndex,
    PVOID pAceList,
    DWORD nAceListLength
    )
/*++

Routine Description:

    This routine adds a string of ACEs to an ACL.

Arguments:

    pAcl - Supplies the Acl being modified

    dwAceRevision - Supplies the Acl/Ace revision of the ACE being
        added

    dwStartingAceIndex - Supplies the ACE index which will be the
        index of the first ace inserted in the acl.  0 for the
        beginning of the list and MAXULONG for the end of the list.

    pAceList - Supplies the list of Aces to be added to the Acl

    nAceListLength - Supplies the size, in bytes, of the AceList
        buffer

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.


--*/
{
    NTSTATUS Status;

    Status = RtlAddAce (
        pAcl,
        dwAceRevision,
        dwStartingAceIndex,
        pAceList,
        nAceListLength
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
DeleteAce (
    PACL pAcl,
    DWORD dwAceIndex
    )
/*++

Routine Description:

    This routine deletes one ACE from an ACL.

Arguments:

    pAcl - Supplies the Acl being modified

    dwAceIndex - Supplies the index of the Ace to delete.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlDeleteAce (
                pAcl,
                dwAceIndex
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
GetAce (
    PACL pAcl,
    DWORD dwAceIndex,
    PVOID *pAce
    )
/*++

Routine Description:

    This routine returns a pointer to an ACE in an ACl referenced by
    ACE index

Arguments:

    pAcl - Supplies the ACL being queried

    dwAceIndex - Supplies the Ace index to locate

    pAce - Receives the address of the ACE within the ACL

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlGetAce (
                pAcl,
                dwAceIndex,
                pAce
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
AddAccessAllowedAce (
    PACL pAcl,
    DWORD dwAceRevision,
    DWORD AccessMask,
    PSID pSid
    )
/*++

Routine Description:

    This routine adds an ACCESS_ALLOWED ACE to an ACL.  This is
    expected to be a common form of ACL modification.

    A very bland ACE header is placed in the ACE.  It provides no
    inheritance and no ACE flags.

Arguments:

    PAcl - Supplies the Acl being modified

    dwAceRevision - Supplies the Acl/Ace revision of the ACE being added

    AccessMask - The mask of accesses to be granted to the specified SID.

    pSid - Pointer to the SID being granted access.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlAddAccessAllowedAce (
                pAcl,
                dwAceRevision,
                AccessMask,
                pSid
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
AddAccessAllowedAceEx (
    PACL pAcl,
    DWORD dwAceRevision,
    DWORD AceFlags,
    DWORD AccessMask,
    PSID pSid
    )
/*++

Routine Description:

    This routine adds an ACCESS_ALLOWED ACE to an ACL.  This is
    expected to be a common form of ACL modification.

    A very bland ACE header is placed in the ACE.  The AceFlags and
    inheritance are specified by the AceFlags parameter.

Arguments:

    PAcl - Supplies the Acl being modified

    dwAceRevision - Supplies the Acl/Ace revision of the ACE being added

    AceFlags - Supplies the inherit flags for the ACE.

    AccessMask - The mask of accesses to be granted to the specified SID.

    pSid - Pointer to the SID being granted access.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlAddAccessAllowedAceEx (
                pAcl,
                dwAceRevision,
                AceFlags,
                AccessMask,
                pSid
                );

    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_INVALID_PARAMETER ) {
            SetLastError( ERROR_INVALID_FLAGS );
        } else {
            BaseSetLastNTError(Status);
        }
        return FALSE;
    }

    return TRUE;
}



BOOL
APIENTRY
AddAccessDeniedAce (
    PACL pAcl,
    DWORD dwAceRevision,
    DWORD AccessMask,
    PSID pSid
    )
/*++

Routine Description:

    This routine adds an ACCESS_DENIED ACE to an ACL.  This is
    expected to be a common form of ACL modification.

    A very bland ACE header is placed in the ACE.  It provides no
    inheritance and no ACE flags.


Arguments:

    pAcl - Supplies the Acl being modified

    dwAceRevision - Supplies the Acl/Ace revision of the ACE being added

    AccessMask - The mask of accesses to be denied to the specified SID.

    pSid - Pointer to the SID being denied access.


Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlAddAccessDeniedAce (
                pAcl,
                dwAceRevision,
                AccessMask,
                pSid
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}



BOOL
APIENTRY
AddAccessDeniedAceEx (
    PACL pAcl,
    DWORD dwAceRevision,
    DWORD AceFlags,
    DWORD AccessMask,
    PSID pSid
    )
/*++

Routine Description:

    This routine adds an ACCESS_DENIED ACE to an ACL.  This is
    expected to be a common form of ACL modification.

    A very bland ACE header is placed in the ACE.  The AceFlags and
    inheritance are specified by the AceFlags parameter.


Arguments:

    pAcl - Supplies the Acl being modified

    dwAceRevision - Supplies the Acl/Ace revision of the ACE being added

    AceFlags - Supplies the inherit flags for the ACE.

    AccessMask - The mask of accesses to be denied to the specified SID.

    pSid - Pointer to the SID being denied access.


Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlAddAccessDeniedAceEx (
                pAcl,
                dwAceRevision,
                AceFlags,
                AccessMask,
                pSid
                );

    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_INVALID_PARAMETER ) {
            SetLastError( ERROR_INVALID_FLAGS );
        } else {
            BaseSetLastNTError(Status);
        }
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
AddAuditAccessAce(
    PACL pAcl,
    DWORD dwAceRevision,
    DWORD dwAccessMask,
    PSID pSid,
    BOOL bAuditSuccess,
    BOOL bAuditFailure
    )
/*++

Routine Description:

    This routine adds a SYSTEM_AUDIT ACE to an ACL.  This is
    expected to be a common form of ACL modification.

    A very bland ACE header is placed in the ACE.  It provides no
    inheritance.

    Parameters are used to indicate whether auditing is to be performed
    on success, failure, or both.


Arguments:

    pAcl - Supplies the Acl being modified

    dwAceRevision - Supplies the Acl/Ace revision of the ACE being added

    dwAccessMask - The mask of accesses to be denied to the specified SID.

    pSid - Pointer to the SID to be audited.

    bAuditSuccess - If TRUE, indicates successful access attempts are to be
        audited.

    bAuditFailure - If TRUE, indicated failed access attempts are to be
        audited.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/

{
    NTSTATUS Status;

    Status =  RtlAddAuditAccessAce (
                pAcl,
                dwAceRevision,
                dwAccessMask,
                pSid,
                (BOOLEAN)bAuditSuccess,
                (BOOLEAN)bAuditFailure
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
AddAuditAccessAceEx(
    PACL pAcl,
    DWORD dwAceRevision,
    DWORD AceFlags,
    DWORD dwAccessMask,
    PSID pSid,
    BOOL bAuditSuccess,
    BOOL bAuditFailure
    )
/*++

Routine Description:

    This routine adds a SYSTEM_AUDIT ACE to an ACL.  This is
    expected to be a common form of ACL modification.

    A very bland ACE header is placed in the ACE.  The AceFlags and
    inheritance are specified by the AceFlags parameter.

    Parameters are used to indicate whether auditing is to be performed
    on success, failure, or both.


Arguments:

    pAcl - Supplies the Acl being modified

    dwAceRevision - Supplies the Acl/Ace revision of the ACE being added

    AceFlags - Supplies the inherit flags for the ACE.

    dwAccessMask - The mask of accesses to be denied to the specified SID.

    pSid - Pointer to the SID to be audited.

    bAuditSuccess - If TRUE, indicates successful access attempts are to be
        audited.

    bAuditFailure - If TRUE, indicated failed access attempts are to be
        audited.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/

{
    NTSTATUS Status;

    Status =  RtlAddAuditAccessAceEx (
                pAcl,
                dwAceRevision,
                AceFlags,
                dwAccessMask,
                pSid,
                (BOOLEAN)bAuditSuccess,
                (BOOLEAN)bAuditFailure
                );

    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_INVALID_PARAMETER ) {
            SetLastError( ERROR_INVALID_FLAGS );
        } else {
            BaseSetLastNTError(Status);
        }
        return FALSE;
    }

    return TRUE;
}


BOOL
APIENTRY
AddAccessAllowedObjectAce (
    PACL pAcl,
    DWORD dwAceRevision,
    DWORD AceFlags,
    DWORD AccessMask,
    GUID *ObjectTypeGuid,
    GUID *InheritedObjectTypeGuid,
    PSID pSid
    )
/*++

Routine Description:

    This routine adds an ACCESS_ALLOWED_OBJECT ACE to an ACL.  This is
    expected to be a common form of ACL modification.

    A very bland ACE header is placed in the ACE.

Arguments:

    PAcl - Supplies the Acl being modified

    dwAceRevision - Supplies the Acl/Ace revision of the ACE being added

    AceFlags - Supplies the inherit flags for the ACE.

    AccessMask - The mask of accesses to be granted to the specified SID.

    ObjectTypeGuid - Supplies the GUID of the object this ACE applies to.
        If NULL, no object type GUID is placed in the ACE.

    InheritedObjectTypeGuid - Supplies the GUID of the object type that will
        inherit this ACE.  If NULL, no inherited object type GUID is placed in
        the ACE.

    pSid - Pointer to the SID being granted access.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlAddAccessAllowedObjectAce (
                pAcl,
                dwAceRevision,
                AceFlags,
                AccessMask,
                ObjectTypeGuid,
                InheritedObjectTypeGuid,
                pSid
                );

    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_INVALID_PARAMETER ) {
            SetLastError( ERROR_INVALID_FLAGS );
        } else {
            BaseSetLastNTError(Status);
        }
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
AddAccessDeniedObjectAce (
    PACL pAcl,
    DWORD dwAceRevision,
    DWORD AceFlags,
    DWORD AccessMask,
    GUID *ObjectTypeGuid,
    GUID *InheritedObjectTypeGuid,
    PSID pSid
    )
/*++

Routine Description:

    This routine adds an ACCESS_DENIED_OBJECT ACE to an ACL.  This is
    expected to be a common form of ACL modification.

    A very bland ACE header is placed in the ACE.

Arguments:

    PAcl - Supplies the Acl being modified

    dwAceRevision - Supplies the Acl/Ace revision of the ACE being added

    AceFlags - Supplies the inherit flags for the ACE.

    AccessMask - The mask of accesses to be granted to the specified SID.

    ObjectTypeGuid - Supplies the GUID of the object this ACE applies to.
        If NULL, no object type GUID is placed in the ACE.

    InheritedObjectTypeGuid - Supplies the GUID of the object type that will
        inherit this ACE.  If NULL, no inherited object type GUID is placed in
        the ACE.

    pSid - Pointer to the SID being denied access.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlAddAccessDeniedObjectAce (
                pAcl,
                dwAceRevision,
                AceFlags,
                AccessMask,
                ObjectTypeGuid,
                InheritedObjectTypeGuid,
                pSid
                );

    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_INVALID_PARAMETER ) {
            SetLastError( ERROR_INVALID_FLAGS );
        } else {
            BaseSetLastNTError(Status);
        }
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
AddAuditAccessObjectAce(
    PACL pAcl,
    DWORD dwAceRevision,
    DWORD AceFlags,
    DWORD dwAccessMask,
    GUID *ObjectTypeGuid,
    GUID *InheritedObjectTypeGuid,
    PSID pSid,
    BOOL bAuditSuccess,
    BOOL bAuditFailure
    )
/*++

Routine Description:

    This routine adds a SYSTEM_AUDIT_OBJECT_ACE to an ACL.  This is
    expected to be a common form of ACL modification.

    A very bland ACE header is placed in the ACE.  The AceFlags and
    inheritance are specified by the AceFlags parameter.

    Parameters are used to indicate whether auditing is to be performed
    on success, failure, or both.


Arguments:

    pAcl - Supplies the Acl being modified

    dwAceRevision - Supplies the Acl/Ace revision of the ACE being added

    AceFlags - Supplies the inherit flags for the ACE.

    dwAccessMask - The mask of accesses to be denied to the specified SID.

    ObjectTypeGuid - Supplies the GUID of the object this ACE applies to.
        If NULL, no object type GUID is placed in the ACE.

    InheritedObjectTypeGuid - Supplies the GUID of the object type that will
        inherit this ACE.  If NULL, no inherited object type GUID is placed in
        the ACE.

    pSid - Pointer to the SID to be audited.

    bAuditSuccess - If TRUE, indicates successful access attempts are to be
        audited.

    bAuditFailure - If TRUE, indicated failed access attempts are to be
        audited.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/

{
    NTSTATUS Status;

    Status =  RtlAddAuditAccessObjectAce (
                pAcl,
                dwAceRevision,
                AceFlags,
                dwAccessMask,
                ObjectTypeGuid,
                InheritedObjectTypeGuid,
                pSid,
                (BOOLEAN)bAuditSuccess,
                (BOOLEAN)bAuditFailure
                );

    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_INVALID_PARAMETER ) {
            SetLastError( ERROR_INVALID_FLAGS );
        } else {
            BaseSetLastNTError(Status);
        }
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
FindFirstFreeAce (
    PACL pAcl,
    PVOID *pAce
    )
/*++

Routine Description:

    This routine returns a pointer to the first free byte in an Acl
    or NULL if the acl is ill-formed.  If the Acl is full then the
    return pointer is to the byte immediately following the acl, and
    TRUE will be returned.

Arguments:

    pAcl - Supplies a pointer to the Acl to examine

    pAce - Receives a pointer to the first free position in the Acl

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    if ( !RtlFirstFreeAce( pAcl, pAce ) ) {
        SetLastError(ERROR_INVALID_ACL);
        return FALSE;
    }
    return TRUE;
}

BOOL
APIENTRY
InitializeSecurityDescriptor (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD dwRevision
    )
/*++

Routine Description:

    This procedure initializes a new "absolute format" security descriptor.
    After the procedure call the security descriptor is initialized with no
    system ACL, no discretionary ACL, no owner, no primary group and
    all control flags set to false (null).

Arguments:


    pSecurityDescriptor - Supplies the security descriptor to
        initialize.

    dwRevision - Provides the revision level to assign to the security
        descriptor.  This should be one (1) for this release.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlCreateSecurityDescriptor (
                pSecurityDescriptor,
                dwRevision
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
IsValidSecurityDescriptor (
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
/*++

Routine Description:

    This procedure validates a SecurityDescriptor's structure.  This
    involves validating the revision levels of each component of the
    security descriptor.

Arguments:

    pSecurityDescriptor - Pointer to the SECURITY_DESCRIPTOR structure
        to validate.

Return Value:

    BOOL - TRUE if the structure of SecurityDescriptor is valid.


--*/
{
    if (!RtlValidSecurityDescriptor ( pSecurityDescriptor )) {
        BaseSetLastNTError( STATUS_INVALID_SECURITY_DESCR );
        return( FALSE );
    }

    return( TRUE );
}




DWORD
APIENTRY
GetSecurityDescriptorLength (
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
/*++

Routine Description:

    This routine returns the length, in bytes, necessary to capture a
    structurally valid SECURITY_DESCRIPTOR.  The length includes the length
    of all associated data structures (like SIDs and ACLs).  The length also
    takes into account the alignment requirements of each component.

    The minimum length of a security descriptor (one which has no associated
    SIDs or ACLs) is SECURITY_DESCRIPTOR_MIN_LENGTH.


Arguments:

    pSecurityDescriptor - Points to the SECURITY_DESCRIPTOR whose
        length is to be returned.  The SECURITY_DESCRIPTOR's structure
        is assumed to be valid.

Return Value:

    DWORD - The length, in bytes, of the SECURITY_DESCRIPTOR.


--*/
{
    return RtlLengthSecurityDescriptor (
        pSecurityDescriptor
        );
}





BOOL
APIENTRY
GetSecurityDescriptorControl (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    PSECURITY_DESCRIPTOR_CONTROL pControl,
    LPDWORD lpdwRevision
    )
/*++

Routine Description:

    This procedure retrieves the control information from a security descriptor.

Arguments:

    pSecurityDescriptor - Supplies the security descriptor.

    pControl - Receives the control information.

    lpdwRevision - Receives the revision of the security descriptor.
        This value will always be returned, even if an error is
        returned by this routine.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlGetControlSecurityDescriptor (
                pSecurityDescriptor,
                pControl,
                lpdwRevision
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
SetSecurityDescriptorControl (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
    SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
    )
/*++

Routine Description:

    This procedure sets the control information in a security descriptor.


    For instance,

        SetSecurityDescriptorControl( &SecDesc,
                                      SE_DACL_PROTECTED,
                                      SE_DACL_PROTECTED );

    marks the DACL on the security descriptor as protected. And

        SetSecurityDescriptorControl( &SecDesc,
                                      SE_DACL_PROTECTED,
                                      0 );


    marks the DACL as not protected.

Arguments:

    pSecurityDescriptor - Supplies the security descriptor.

    ControlBitsOfInterest - A mask of the control bits being changed, set,
        or reset by this call.  The mask is the logical OR of one or more of
        the following flags:

            SE_DACL_UNTRUSTED
            SE_SERVER_SECURITY
            SE_DACL_AUTO_INHERIT_REQ
            SE_SACL_AUTO_INHERIT_REQ
            SE_DACL_AUTO_INHERITED
            SE_SACL_AUTO_INHERITED
            SE_DACL_PROTECTED
            SE_SACL_PROTECTED

    ControlBitsToSet - A mask indicating what the bits specified by ControlBitsOfInterest
        should be set to.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlSetControlSecurityDescriptor (
                pSecurityDescriptor,
                ControlBitsOfInterest,
                ControlBitsToSet );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
SetSecurityDescriptorDacl (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    BOOL bDaclPresent,
    PACL pDacl OPTIONAL,
    BOOL bDaclDefaulted OPTIONAL
    )
/*++

Routine Description:

    This procedure sets the discretionary ACL information of an absolute
    format security descriptor.  If there is already a discretionary ACL
    present in the security descriptor, it is superseded.

Arguments:

    pSecurityDescriptor - Supplies the security descriptor to be which
        the discretionary ACL is to be added.

    bDaclPresent - If FALSE, indicates the DaclPresent flag in the
        security descriptor should be set to FALSE.  In this case, the
        remaining optional parameters are ignored.  Otherwise, the
        DaclPresent control flag in the security descriptor is set to
        TRUE and the remaining optional parameters are not ignored.

    pDacl - Supplies the discretionary ACL for the security
        descriptor.  If this optional parameter is not passed, then a
        null ACL is assigned to the security descriptor.  A null
        discretionary ACL unconditionally grants access.  The ACL is
        referenced by, not copied into, by the security descriptor.

    bDaclDefaulted - When set, indicates the discretionary ACL was
        picked up from some default mechanism (rather than explicitly
        specified by a user).  This value is set in the DaclDefaulted
        control flag in the security descriptor.  If this optional
        parameter is not passed, then the DaclDefaulted flag will be
        cleared.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlSetDaclSecurityDescriptor (
        pSecurityDescriptor,
        (BOOLEAN)bDaclPresent,
        pDacl,
        (BOOLEAN)bDaclDefaulted
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}



BOOL
APIENTRY
GetSecurityDescriptorDacl (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    LPBOOL lpbDaclPresent,
    PACL *pDacl,
    LPBOOL lpbDaclDefaulted
    )
/*++

Routine Description:

    This procedure retrieves the discretionary ACL information of a
    security descriptor.

Arguments:

    pSecurityDescriptor - Supplies the security descriptor.

    lpbDaclPresent - If TRUE, indicates that the security descriptor
        does contain a discretionary ACL.  In this case, the
        remaining OUT parameters will receive valid values.
        Otherwise, the security descriptor does not contain a
        discretionary ACL and the remaining OUT parameters will not
        receive valid values.

    pDacl - This value is returned only if the value returned for the
        DaclPresent flag is TRUE.  In this case, the Dacl parameter
        receives the address of the security descriptor's
        discretionary ACL.  If this value is returned as null, then
        the security descriptor has a null discretionary ACL.

    lpbDaclDefaulted - This value is returned only if the value
        returned for the DaclPresent flag is TRUE.  In this case, the
        DaclDefaulted parameter receives the value of the security
        descriptor's DaclDefaulted control flag.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;
    BOOLEAN DaclPresent, DaclDefaulted;

    Status = RtlGetDaclSecurityDescriptor (
        pSecurityDescriptor,
        &DaclPresent,
        pDacl,
        &DaclDefaulted
        );
    *lpbDaclPresent = (BOOL)DaclPresent;
    *lpbDaclDefaulted = (BOOL)DaclDefaulted;

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
SetSecurityDescriptorSacl (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    BOOL bSaclPresent,
    PACL pSacl OPTIONAL,
    BOOL bSaclDefaulted
    )
/*++

Routine Description:

    This procedure sets the system ACL information of an absolute security
    descriptor.  If there is already a system ACL present in the
    security descriptor, it is superseded.

Arguments:

    pSecurityDescriptor - Supplies the security descriptor to be which
        the system ACL is to be added.

    bSaclPresent - If FALSE, indicates the SaclPresent flag in the
        security descriptor should be set to FALSE.  In this case,
        the remaining optional parameters are ignored.  Otherwise,
        the SaclPresent control flag in the security descriptor is
        set to TRUE and the remaining optional parameters are not
        ignored.

    pSacl - Supplies the system ACL for the security descriptor.  If
        this optional parameter is not passed, then a null ACL is
        assigned to the security descriptor.  The ACL is referenced
        by, not copied into, by the security descriptor.

    bSaclDefaulted - When set, indicates the system ACL was picked up
        from some default mechanism (rather than explicitly specified
        by a user).  This value is set in the SaclDefaulted control
        flag in the security descriptor.  If this optional parameter
        is not passed, then the SaclDefaulted flag will be cleared.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlSetSaclSecurityDescriptor (
                pSecurityDescriptor,
                (BOOLEAN)bSaclPresent,
                pSacl,
                (BOOLEAN)bSaclDefaulted
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
GetSecurityDescriptorSacl (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    LPBOOL lpbSaclPresent,
    PACL *pSacl,
    LPBOOL lpbSaclDefaulted
    )
/*++

Routine Description:

    This procedure retrieves the system ACL information of a security
    descriptor.

Arguments:

    pSecurityDescriptor - Supplies the security descriptor.

    lpbSaclPresent - If TRUE, indicates that the security descriptor
        does contain a system ACL.  In this case, the remaining OUT
        parameters will receive valid values.  Otherwise, the
        security descriptor does not contain a system ACL and the
        remaining OUT parameters will not receive valid values.

    pSacl - This value is returned only if the value returned for the
        SaclPresent flag is TRUE.  In this case, the Sacl parameter
        receives the address of the security descriptor's system ACL.
        If this value is returned as null, then the security
        descriptor has a null system ACL.

    lpbSaclDefaulted - This value is returned only if the value
        returned for the SaclPresent flag is TRUE.  In this case, the
        SaclDefaulted parameter receives the value of the security
        descriptor's SaclDefaulted control flag.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;
    BOOLEAN SaclPresent, SaclDefaulted;

    Status = RtlGetSaclSecurityDescriptor (
        pSecurityDescriptor,
        &SaclPresent,
        pSacl,
        &SaclDefaulted
        );
    *lpbSaclPresent = (BOOL)SaclPresent;
    *lpbSaclDefaulted = (BOOL)SaclDefaulted;

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
SetSecurityDescriptorOwner (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    PSID pOwner OPTIONAL,
    BOOL bOwnerDefaulted OPTIONAL
    )
/*++

Routine Description:

    This procedure sets the owner information of an absolute security
    descriptor.  If there is already an owner present in the security
    descriptor, it is superseded.

Arguments:

    pSecurityDescriptor - Supplies the security descriptor in which
        the owner is to be set.  If the security descriptor already
        includes an owner, it will be superseded by the new owner.

    pOwner - Supplies the owner SID for the security descriptor.  If
        this optional parameter is not passed, then the owner is
        cleared (indicating the security descriptor has no owner).
        The SID is referenced by, not copied into, the security
        descriptor.

    bOwnerDefaulted - When set, indicates the owner was picked up from
        some default mechanism (rather than explicitly specified by a
        user).  This value is set in the OwnerDefaulted control flag
        in the security descriptor.  If this optional parameter is
        not passed, then the SaclDefaulted flag will be cleared.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.


--*/
{
    NTSTATUS Status;

    Status = RtlSetOwnerSecurityDescriptor (
        pSecurityDescriptor,
        pOwner,
        (BOOLEAN)bOwnerDefaulted
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
GetSecurityDescriptorOwner (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    PSID *pOwner,
    LPBOOL lpbOwnerDefaulted
    )
/*++

Routine Description:

    This procedure retrieves the owner information of a security
    descriptor.

Arguments:

    pSecurityDescriptor - Supplies the security descriptor.

    pOwner - Receives a pointer to the owner SID.  If the security
        descriptor does not currently contain an owner, then this
        value will be returned as null.  In this case, the remaining
        OUT parameters are not given valid return values.  Otherwise,
        this parameter points to an SID and the remaining OUT
        parameters are provided valid return values.

    lpbOwnerDefaulted - This value is returned only if the value
        returned for the Owner parameter is not null.  In this case,
        the OwnerDefaulted parameter receives the value of the
        security descriptor's OwnerDefaulted control flag.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;
    BOOLEAN OwnerDefaulted;

    Status = RtlGetOwnerSecurityDescriptor (
        pSecurityDescriptor,
        pOwner,
        &OwnerDefaulted
        );
    *lpbOwnerDefaulted = (BOOL)OwnerDefaulted;

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
SetSecurityDescriptorGroup (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    PSID pGroup OPTIONAL,
    BOOL bGroupDefaulted OPTIONAL
    )
/*++

Routine Description:

    This procedure sets the primary group information of an absolute security
    descriptor.  If there is already an primary group present in the
    security descriptor, it is superseded.

Arguments:

    pSecurityDescriptor - Supplies the security descriptor in which
        the primary group is to be set.  If the security descriptor
        already includes a primary group, it will be superseded by
        the new group.

    pGroup - Supplies the primary group SID for the security
        descriptor.  If this optional parameter is not passed, then
        the primary group is cleared (indicating the security
        descriptor has no primary group).  The SID is referenced by,
        not copied into, the security descriptor.

    bGroupDefaulted - When set, indicates the owner was picked up from
        some default mechanism (rather than explicitly specified by a
        user).  This value is set in the OwnerDefaulted control flag
        in the security descriptor.  If this optional parameter is
        not passed, then the SaclDefaulted flag will be cleared.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlSetGroupSecurityDescriptor (
        pSecurityDescriptor,
        pGroup,
        (BOOLEAN)bGroupDefaulted
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
GetSecurityDescriptorGroup (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    PSID *pGroup,
    LPBOOL lpbGroupDefaulted
    )
/*++

Routine Description:

    This procedure retrieves the primary group information of a
    security descriptor.

Arguments:

    pSecurityDescriptor - Supplies the security descriptor.

    pGroup - Receives a pointer to the primary group SID.  If the
        security descriptor does not currently contain a primary
        group, then this value will be returned as null.  In this
        case, the remaining OUT parameters are not given valid return
        values.  Otherwise, this parameter points to an SID and the
        remaining OUT parameters are provided valid return values.

    lpbGroupDefaulted - This value is returned only if the value
        returned for the Group parameter is not null.  In this case,
        the GroupDefaulted parameter receives the value of the
        security descriptor's GroupDefaulted control flag.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;
    BOOLEAN GroupDefaulted;

    Status = RtlGetGroupSecurityDescriptor (
        pSecurityDescriptor,
        pGroup,
        &GroupDefaulted
        );
    *lpbGroupDefaulted = GroupDefaulted;

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
CreatePrivateObjectSecurity (
    PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    PSECURITY_DESCRIPTOR CreatorDescriptor OPTIONAL,
    PSECURITY_DESCRIPTOR * NewDescriptor,
    BOOL IsDirectoryObject,
    HANDLE Token,
    PGENERIC_MAPPING GenericMapping
    )
/*++

Routine Description:

    The procedure is used to allocpate and initialize a self-relative
    Security Descriptor for a new protected server's object.  It is called
    when a new protected server object is being created.  The generated
    security descriptor will be in self-relative form.

    This procedure, called only from user mode, is used to establish a
    security descriptor for a new protected server's object.  When no
    longer needed, this descriptor must be freed using
    DestroyPrivateObjectSecurity().

Arguments:

    ParentDescriptor - Supplies the Security Descriptor for the parent
        directory under which a new object is being created.  If there is
        no parent directory, then this argument is specified as NULL.

    CreatorDescriptor - (Optionally) Points to a security descriptor
        presented by the creator of the object.  If the creator of the
        object did not explicitly pass security information for the new
        object, then a null pointer should be passed.

    NewDescriptor - Points to a pointer that is to be made to point to the
        newly allocated self-relative security descriptor.

    IsDirectoryObject - Specifies if the new object is going to be a
        directory object.  A value of TRUE indicates the object is a
        container of other objects.

    Token - Supplies the token for the client on whose behalf the
        object is being created.  If it is an impersonation token,
        then it must be at SecurityIdentification level or higher.  If
        it is not an impersonation token, the operation proceeds
        normally.

        A client token is used to retrieve default security
        information for the new object, such as default owner, primary
        group, and discretionary access control.  The token must be
        open for TOKEN_QUERY access.

    GenericMapping - Supplies a pointer to a generic mapping array denoting
        the mapping between each generic right to specific rights.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlNewSecurityObject (
        ParentDescriptor,
        CreatorDescriptor,
        NewDescriptor,
        (BOOLEAN)IsDirectoryObject,
        Token,
        GenericMapping
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}



BOOL
APIENTRY
ConvertToAutoInheritPrivateObjectSecurity(
    PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    PSECURITY_DESCRIPTOR CurrentSecurityDescriptor,
    PSECURITY_DESCRIPTOR *NewSecurityDescriptor,
    GUID *ObjectType,
    BOOLEAN IsDirectoryObject,
    PGENERIC_MAPPING GenericMapping
    )
/*++

Routine Description:

    This is a converts a security descriptor whose ACLs are not marked
    as AutoInherit to a security descriptor whose ACLs are marked as
    AutoInherit.

    The resultant security descriptor has appropriate ACEs marked as
    INHERITED_ACE if the ACE was apparently inherited from the ParentDescriptor.
    If the ACL is apparently not inherited from the ParentDescriptor, the
    ACL in the resultant security descriptor is marked as SE_xACL_PROTECTED.

    This routine takes into account the various mechanisms for creating an
    inherited ACL:

    1) It was inherited via NT 3.x or 4.x ACL inheritance when the
    object was created.

    2) The subsequent parent or child ACL was re-written by the ACL editor
    (which perversely modifies the ACL to a semantically equivalent but
    different form).

    3) It was inherited by asking the ACL editor (File Manager/Explorer) to
    "Replace permissions on existing files/directories".

    4) It was inherited via cacls.exe.

    If the ACLs in the resultant security descriptor are not marked as protected, the
    resultant ACL is composed of two sets of ACEs: the non-inherited ACEs followed by the
    inherited ACEs.  The inherited ACEs are computed by called CreatePrivateObjectSecurityEx
    using the ParentDescriptor.  The non-inherited ACEs are those ACEs (or parts of ACEs)
    from the original CurrentSecurityDescriptor that were not inherited from the parent.

    When building the resultant NewSecurityDescriptor, care is taken to not change the
    semantics of the security descriptor.  As such, allow and deny ACEs are never moved
    in relation to one another.  If such movement is needed (for instance to place all
    non-inherited ACEs at the front of an ACL), the ACL is marked as protected to prevent
    the semantic change.

    ACEs in the original CurrentSecurityDescriptor are matched with ACEs in a computed
    inherited security descriptor to determine which ACEs were inherited.  During the
    comparision there is no requirement of a one to one match.  For instance, one ACL
    might use separate ACEs to grant a user read and write access while the other ACL
    might use only one ACE to grant the same access.  Or one ACL might grant the user
    the same access twice and the other might grant the user that access only once.  Or
    one ACL might combine the container inherit and object inherit ACE into a single ACE.
    In all these case, equivalent ACE combinations are deemed equivalent.

    No security checks are made in this routine.  The resultant security descriptor
    is equivalent to the new security descriptor, so the caller needs no permission to
    update the security descriptor to the new form.

    The Owner and Group field of the CurrentSecurityDescriptor is maintained.

    This routine support revision 2 and revision 4 ACLs.  It does not support compound
    ACEs.

Arguments:

    ParentDescriptor - Supplies the Security Descriptor for the parent
        directory under which a object exists.  If there is
        no parent directory, then this argument is specified as NULL.

    CurrentSecurityDescriptor - Supplies a pointer to the objects security descriptor
        that is going to be altered by this procedure.

    NewSecurityDescriptor Points to a pointer that is to be made to point to the
        newly allocated self-relative security descriptor. When no
        longer needed, this descriptor must be freed using
        DestroyPrivateObjectSecurity().

    ObjectType - GUID of the object type being created.  If the object being
        created has no GUID associated with it, then this argument is
        specified as NULL.

    IsDirectoryObject - Specifies if the object is a
        directory object.  A value of TRUE indicates the object is a
        container of other objects.

    GenericMapping - Supplies a pointer to a generic mapping array denoting
        the mapping between each generic right to specific rights.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlConvertToAutoInheritSecurityObject(
                ParentDescriptor,
                CurrentSecurityDescriptor,
                NewSecurityDescriptor,
                ObjectType,
                IsDirectoryObject,
                GenericMapping ) ;

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
CreatePrivateObjectSecurityEx (
    PSECURITY_DESCRIPTOR ParentDescriptor,
    PSECURITY_DESCRIPTOR CreatorDescriptor,
    PSECURITY_DESCRIPTOR * NewDescriptor,
    GUID *ObjectType,
    BOOL IsContainerObject,
    ULONG AutoInheritFlags,
    HANDLE Token,
    PGENERIC_MAPPING GenericMapping
    )
/*++

Routine Description:

    The procedure is used to allocate and initialize a self-relative
    Security Descriptor for a new protected server's object.  It is called
    when a new protected server object is being created. The generated
    security descriptor will be in self-relative form.

    This procedure, called only from user mode, is used to establish a
    security descriptor for a new protected server's object.
Arguments:

    ParentDescriptor - Supplies the Security Descriptor for the parent
        directory under which a new object is being created.  If there is
        no parent directory, then this argument is specified as NULL.

    CreatorDescriptor - (Optionally) Points to a security descriptor
        presented by the creator of the object.  If the creator of the
        object did not explicitly pass security information for the new
        object, then a null pointer should be passed.

    NewDescriptor - Points to a pointer that is to be made to point to the
        newly allocated self-relative security descriptor. When no
        longer needed, this descriptor must be freed using
        DestroyPrivateObjectSecurity().

    ObjectType - GUID of the object type being created.  If the object being
        created has no GUID associated with it, then this argument is
        specified as NULL.

    IsContainerObject - Specifies if the new object is going to be a
        container object.  A value of TRUE indicates the object is a
        container of other objects.

    AutoInheritFlags - Controls automatic inheritance of ACES from the Parent
        Descriptor.  Valid values are a bits mask of the logical OR of
        one or more of the following bits:

        SEF_DACL_AUTO_INHERIT - If set, inherit ACEs from the
            DACL ParentDescriptor are inherited to NewDescriptor in addition
            to any explicit ACEs specified by the CreatorDescriptor.

        SEF_SACL_AUTO_INHERIT - If set, inherit ACEs from the
            SACL ParentDescriptor are inherited to NewDescriptor in addition
            to any explicit ACEs specified by the CreatorDescriptor.

        SEF_DEFAULT_DESCRIPTOR_FOR_OBJECT - If set, the CreatorDescriptor
            is the default descriptor for ObjectType.  As such, the
            CreatorDescriptor will be ignored if any ObjectType specific
            ACEs are inherited from the parent.  If not such ACEs are inherited,
            the CreatorDescriptor is handled as though this flag were not
            specified.

        SEF_AVOID_PRIVILEGE_CHECK - If set, no privilege checking is done by this
            routine.  This flag is useful while implementing automatic inheritance
            to avoid checking privileges on each child updated.

        SEF_AVOID_OWNER_CHECK - If set, no owner checking is done by this routine.

        SEF_DEFAULT_OWNER_FROM_PARENT - If set, the owner of NewDescriptor will
            default to the owner from ParentDescriptor.  If not set, the owner
            of NewDescriptor will default to the user specified in Token.

            In either case, the owner of NewDescriptor is set to the owner from
            the CreatorDescriptor if that field is specified.

        SEF_DEFAULT_GROUP_FROM_PARENT - If set, the group of NewDescriptor will
            default to the group from ParentDescriptor.  If not set, the group
            of NewDescriptor will default to the group specified in Token.

            In either case, the group of NewDescriptor is set to the group from
            the CreatorDescriptor if that field is specified.


    Token - Supplies the token for the client on whose behalf the
        object is being created.  If it is an impersonation token,
        then it must be at SecurityIdentification level or higher.  If
        it is not an impersonation token, the operation proceeds
        normally.

        A client token is used to retrieve default security
        information for the new object, such as default owner, primary
        group, and discretionary access control.  The token must be
        open for TOKEN_QUERY access.

    GenericMapping - Supplies a pointer to a generic mapping array denoting
        the mapping between each generic right to specific rights.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlNewSecurityObjectEx (
        ParentDescriptor,
        CreatorDescriptor,
        NewDescriptor,
        ObjectType,
        (BOOLEAN)IsContainerObject,
        AutoInheritFlags,
        Token,
        GenericMapping
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
SetPrivateObjectSecurity (
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR ModificationDescriptor,
    PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    PGENERIC_MAPPING GenericMapping,
    HANDLE Token OPTIONAL
    )
/*++

Routine Description:

    Modify an object's existing self-relative form security descriptor.

    This procedure, called only from user mode, is used to update a
    security descriptor on an existing protected server's object.  It
    applies changes requested by a new security descriptor to the existing
    security descriptor.  If necessary, this routine will allocate
    additional memory to produce a larger security descriptor.  All access
    checking is expected to be done before calling this routine.  This
    includes checking for WRITE_OWNER, WRITE_DAC, and privilege to assign a
    system ACL as appropriate.

    The caller of this routine must not be impersonating a client.

Arguments:

    SecurityInformation - Indicates which security information is
        to be applied to the object.  The value(s) to be assigned are
        passed in the ModificationDescriptor parameter.

    ModificationDescriptor - Supplies the input security descriptor to be
        applied to the object.  The caller of this routine is expected
        to probe and capture the passed security descriptor before calling
        and release it after calling.

    ObjectsSecurityDescriptor - Supplies the address of a pointer to
        the objects security descriptor that is going to be altered by
        this procedure.  This security descriptor must be in self-
        relative form or an error will be returned.

    GenericMapping - This argument provides the mapping of generic to
        specific/standard access types for the object being accessed.
        This mapping structure is expected to be safe to access
        (i.e., captured if necessary) prior to be passed to this routine.

    Token - (optionally) Supplies the token for the client on whose
        behalf the security is being modified.  This parameter is only
        required to ensure that the client has provided a legitimate
        value for a new owner SID.  The token must be open for
        TOKEN_QUERY access.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlSetSecurityObject (
        SecurityInformation,
        ModificationDescriptor,
        ObjectsSecurityDescriptor,
        GenericMapping,
        Token
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
SetPrivateObjectSecurityEx (
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR ModificationDescriptor,
    PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    ULONG AutoInheritFlags,
    PGENERIC_MAPPING GenericMapping,
    HANDLE Token OPTIONAL
    )
/*++

Routine Description:

    Modify an object's existing self-relative form security descriptor.

    This procedure, called only from user mode, is used to update a
    security descriptor on an existing protected server's object.  It
    applies changes requested by a new security descriptor to the existing
    security descriptor.  If necessary, this routine will allocate
    additional memory to produce a larger security descriptor.  All access
    checking is expected to be done before calling this routine.  This
    includes checking for WRITE_OWNER, WRITE_DAC, and privilege to assign a
    system ACL as appropriate.

    The caller of this routine must not be impersonating a client.

Arguments:

    SecurityInformation - Indicates which security information is
        to be applied to the object.  The value(s) to be assigned are
        passed in the ModificationDescriptor parameter.

    ModificationDescriptor - Supplies the input security descriptor to be
        applied to the object.  The caller of this routine is expected
        to probe and capture the passed security descriptor before calling
        and release it after calling.

    ObjectsSecurityDescriptor - Supplies the address of a pointer to
        the objects security descriptor that is going to be altered by
        this procedure.  This security descriptor must be in self-
        relative form or an error will be returned.

    AutoInheritFlags - Controls automatic inheritance of ACES.
        Valid values are a bits mask of the logical OR of
        one or more of the following bits:

        SEF_DACL_AUTO_INHERIT - If set, inherited ACEs from the
            DACL in the ObjectsSecurityDescriptor are preserved and inherited ACEs from
            the ModificationDescriptor are ignored. Inherited ACEs are not supposed
            to be modified; so preserving them across this call is appropriate.
            If a protected server does not itself implement auto inheritance, it should
            not set this bit.  The caller of the protected server may implement
            auto inheritance and my indeed be modifying inherited ACEs.

        SEF_SACL_AUTO_INHERIT - If set, inherited ACEs from the
            SACL in the ObjectsSecurityDescriptor are preserved and inherited ACEs from
            the ModificationDescriptor are ignored. Inherited ACEs are not supposed
            to be modified; so preserving them across this call is appropriate.
            If a protected server does not itself implement auto inheritance, it should
            not set this bit.  The caller of the protected server may implement
            auto inheritance and my indeed be modifying inherited ACEs.

    GenericMapping - This argument provides the mapping of generic to
        specific/standard access types for the object being accessed.
        This mapping structure is expected to be safe to access
        (i.e., captured if necessary) prior to be passed to this routine.

    Token - (optionally) Supplies the token for the client on whose
        behalf the security is being modified.  This parameter is only
        required to ensure that the client has provided a legitimate
        value for a new owner SID.  The token must be open for
        TOKEN_QUERY access.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlSetSecurityObjectEx (
        SecurityInformation,
        ModificationDescriptor,
        ObjectsSecurityDescriptor,
        AutoInheritFlags,
        GenericMapping,
        Token
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
GetPrivateObjectSecurity (
    PSECURITY_DESCRIPTOR ObjectDescriptor,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR ResultantDescriptor,
    DWORD DescriptorLength,
    PDWORD ReturnLength
    )
/*++

Routine Description:

    Query information from a protected server object's existing security
    descriptor.

    This procedure, called only from user mode, is used to retrieve
    information from a security descriptor on an existing protected
    server's object.  All access checking is expected to be done before
    calling this routine.  This includes checking for READ_CONTROL, and
    privilege to read a system ACL as appropriate.

Arguments:

    ObjectDescriptor - Points to a pointer to a security descriptor to be
        queried.

    SecurityInformation - Identifies the security information being
        requested.

    ResultantDescriptor - Points to buffer to receive the resultant
        security descriptor.  The resultant security descriptor will
        contain all information requested by the SecurityInformation
        parameter.

    DescriptorLength - Is an unsigned integer which indicates the length,
        in bytes, of the buffer provided to receive the resultant
        descriptor.

    ReturnLength - Receives an unsigned integer indicating the actual
        number of bytes needed in the ResultantDescriptor to store the
        requested information.  If the value returned is greater than the
        value passed via the DescriptorLength parameter, then
        STATUS_BUFFER_TOO_SMALL is returned and no information is returned.


Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlQuerySecurityObject (
         ObjectDescriptor,
         SecurityInformation,
         ResultantDescriptor,
         DescriptorLength,
         ReturnLength
         );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
DestroyPrivateObjectSecurity (
    PSECURITY_DESCRIPTOR * ObjectDescriptor
    )
/*++

Routine Description:

    Delete a protected server object's security descriptor.

    This procedure, called only from user mode, is used to delete a
    security descriptor associated with a protected server's object.  This
    routine will normally be called by a protected server during object
    deletion.  The input descriptor is expected to be one created via
    a call to CreatePrivateObjectSecurity.

Arguments:

    ObjectDescriptor - Points to a pointer to a security descriptor to be
        deleted.


Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlDeleteSecurityObject (
        ObjectDescriptor
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
MakeSelfRelativeSD (
    PSECURITY_DESCRIPTOR pAbsoluteSecurityDescriptor,
    PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
    LPDWORD lpdwBufferLength
    )
/*++

Routine Description:

    Converts a security descriptor in absolute form to one in self-relative
    form.

Arguments:

    pAbsoluteSecurityDescriptor - Pointer to an absolute format
        security descriptor.  This descriptor will not be modified.

    pSelfRelativeSecurityDescriptor - Pointer to a buffer that will
        contain the returned self-relative security descriptor.

    lpdwBufferLength - Supplies the length of the buffer.  If the
        supplied buffer is not large enough to hold the self-relative
        security descriptor, an error will be returned, and this field
        will return the minimum size required.


Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlAbsoluteToSelfRelativeSD (
                pAbsoluteSecurityDescriptor,
                pSelfRelativeSecurityDescriptor,
                lpdwBufferLength
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
MakeAbsoluteSD (
    PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
    PSECURITY_DESCRIPTOR pAbsoluteSecurityDescriptor,
    LPDWORD lpdwAbsoluteSecurityDescriptorSize,
    PACL pDacl,
    LPDWORD lpdwDaclSize,
    PACL pSacl,
    LPDWORD lpdwSaclSize,
    PSID pOwner,
    LPDWORD lpdwOwnerSize,
    PSID pPrimaryGroup,
    LPDWORD lpdwPrimaryGroupSize
    )
/*++

Routine Description:

    Converts a security descriptor from self-relative format to absolute
    format

Arguments:

    pSecurityDescriptor - Supplies a pointer to a security descriptor
        in Self-Relative format

    pAbsoluteSecurityDescriptor - A pointer to a buffer in which will
        be placed the main body of the Absolute format security
        descriptor.

    lpdwAbsoluteSecurityDescriptorSize - The size in bytes of the
        buffer pointed to by pAbsoluteSecurityDescriptor.

    pDacl - Supplies a pointer to a buffer that will contain the Dacl
        of the output descriptor.  This pointer will be referenced by,
        not copied into, the output descriptor.

    lpdwDaclSize - Supplies the size of the buffer pointed to by Dacl.
        In case of error, it will return the minimum size necessary to
        contain the Dacl.

    pSacl - Supplies a pointer to a buffer that will contain the Sacl
        of the output descriptor.  This pointer will be referenced by,
        not copied into, the output descriptor.

    lpdwSaclSize - Supplies the size of the buffer pointed to by Sacl.
        In case of error, it will return the minimum size necessary to
        contain the Sacl.

    pOwner - Supplies a pointer to a buffer that will contain the
        Owner of the output descriptor.  This pointer will be
        referenced by, not copied into, the output descriptor.

    lpdwOwnerSize - Supplies the size of the buffer pointed to by
        Owner.  In case of error, it will return the minimum size
        necessary to contain the Owner.

    pPrimaryGroup - Supplies a pointer to a buffer that will contain
        the PrimaryGroup of the output descriptor.  This pointer will
        be referenced by, not copied into, the output descriptor.

    lpdwPrimaryGroupSize - Supplies the size of the buffer pointed to
        by PrimaryGroup.  In case of error, it will return the minimum
        size necessary to contain the PrimaryGroup.


Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlSelfRelativeToAbsoluteSD (
                pSelfRelativeSecurityDescriptor,
                pAbsoluteSecurityDescriptor,
                lpdwAbsoluteSecurityDescriptorSize,
                pDacl,
                lpdwDaclSize,
                pSacl,
                lpdwSaclSize,
                pOwner,
                lpdwOwnerSize,
                pPrimaryGroup,
                lpdwPrimaryGroupSize
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


VOID
SetSecurityAccessMask(
    IN SECURITY_INFORMATION SecurityInformation,
    OUT LPDWORD DesiredAccess
    )

/*++

Routine Description:

    This routine builds an access mask representing the accesses necessary
    to set the object security information specified in the SecurityInformation
    parameter.  While it is not difficult to determine this information,
    the use of a single routine to generate it will ensure minimal impact
    when the security information associated with an object is extended in
    the future (to include mandatory access control information).

Arguments:

    SecurityInformation - Identifies the object's security information to be
        modified.

    DesiredAccess - Points to an access mask to be set to represent the
        accesses necessary to modify the information specified in the
        SecurityInformation parameter.

Return Value:

    None.

--*/

{

    //
    // Figure out accesses needed to perform the indicated operation(s).
    //

    (*DesiredAccess) = 0;

    if ((SecurityInformation & OWNER_SECURITY_INFORMATION) ||
        (SecurityInformation & GROUP_SECURITY_INFORMATION)   ) {
        (*DesiredAccess) |= WRITE_OWNER;
    }

    if (SecurityInformation & DACL_SECURITY_INFORMATION) {
        (*DesiredAccess) |= WRITE_DAC;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION) {
        (*DesiredAccess) |= ACCESS_SYSTEM_SECURITY;
    }

    return;

}


VOID
QuerySecurityAccessMask(
    IN SECURITY_INFORMATION SecurityInformation,
    OUT LPDWORD DesiredAccess
    )

/*++

Routine Description:

    This routine builds an access mask representing the accesses necessary
    to query the object security information specified in the
    SecurityInformation parameter.  While it is not difficult to determine
    this information, the use of a single routine to generate it will ensure
    minimal impact when the security information associated with an object is
    extended in the future (to include mandatory access control information).

Arguments:

    SecurityInformation - Identifies the object's security information to be
        queried.

    DesiredAccess - Points to an access mask to be set to represent the
        accesses necessary to query the information specified in the
        SecurityInformation parameter.

Return Value:

    None.

--*/

{

    //
    // Figure out accesses needed to perform the indicated operation(s).
    //

    (*DesiredAccess) = 0;

    if ((SecurityInformation & OWNER_SECURITY_INFORMATION) ||
        (SecurityInformation & GROUP_SECURITY_INFORMATION) ||
        (SecurityInformation & DACL_SECURITY_INFORMATION)) {
        (*DesiredAccess) |= READ_CONTROL;
    }

    if ((SecurityInformation & SACL_SECURITY_INFORMATION)) {
        (*DesiredAccess) |= ACCESS_SYSTEM_SECURITY;
    }

    return;

}

BOOL
APIENTRY
SetFileSecurityW(
    LPCWSTR lpFileName,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    )

/*++

Routine Description:

    This API can be used to set the security of a file or directory
    (process, file, event, etc.).  This call is only successful if the
    following conditions are met:

    o If the object's owner or group is to be set, the caller must
      have WRITE_OWNER permission or have SeTakeOwnershipPrivilege.

    o If the object's DACL is to be set, the caller must have
      WRITE_DAC permission or be the object's owner.

    o If the object's SACL is to be set, the caller must have
      SeSecurityPrivilege.

Arguments:

    lpFileName - Supplies the file name of the file to open.  Depending on
        the value of the FailIfExists parameter, this name may or may
        not already exist.

    SecurityInformation - A pointer to information describing the
        contents of the Security Descriptor.

    pSecurityDescriptor - A pointer to a well formed Security
        Descriptor.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/
{
    NTSTATUS Status;
    HANDLE FileHandle;
    ACCESS_MASK DesiredAccess;

    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileName;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    IO_STATUS_BLOCK IoStatusBlock;
    PVOID FreeBuffer;


    SetSecurityAccessMask(
        SecurityInformation,
        &DesiredAccess
        );

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFileName,
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        BaseSetLastNTError(STATUS_OBJECT_NAME_INVALID);
        return FALSE;
        }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    //
    // Notice that FILE_OPEN_REPARSE_POINT inhibits the reparse behavior. Thus, the
    // security will always be set, as before, in the file denoted by the name.
    //

    Status = NtOpenFile(
                 &FileHandle,
                 DesiredAccess,
                 &Obja,
                 &IoStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 FILE_OPEN_REPARSE_POINT
                 );

    //
    // Back-level file systems may not support the FILE_OPEN_REPARSE_POINT
    // flag. We treat this case explicitly.
    //

    if ( Status == STATUS_INVALID_PARAMETER ) {
        //
        // Open without inhibiting the reparse behavior.
        //

        Status = NtOpenFile(
                     &FileHandle,
                     DesiredAccess,
                     &Obja,
                     &IoStatusBlock,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     0
                     );
        }

    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

    if ( !NT_SUCCESS( Status ) ) {
        BaseSetLastNTError( Status );
        return FALSE;
        }

    Status = NtSetSecurityObject(
                FileHandle,
                SecurityInformation,
                pSecurityDescriptor
                );

    NtClose(FileHandle);

    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    return TRUE;
}

BOOL
APIENTRY
SetFileSecurityA(
    LPCSTR lpFileName,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    )

/*++

Routine Description:

    ANSI thunk to SetFileSecurityW

--*/

{

    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitAnsiString(&AnsiString,lpFileName);
    Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    return ( SetFileSecurityW( (LPCWSTR)Unicode->Buffer,
                               SecurityInformation,
                               pSecurityDescriptor
                        )
           );
}

BOOL
APIENTRY
GetFileSecurityW(
    LPCWSTR lpFileName,
    SECURITY_INFORMATION RequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD nLength,
    LPDWORD lpnLengthNeeded
    )

/*++

Routine Description:

    This API returns top the caller a copy of the security descriptor
    protecting a file or directory.  Based on the caller's access
    rights and privileges, this procedure will return a security
    descriptor containing the requested security descriptor fields.
    To read the handle's security descriptor the caller must be
    granted READ_CONTROL access or be the owner of the object.  In
    addition, the caller must have SeSecurityPrivilege privilege to
    read the system ACL.

Arguments:

    lpFileName - Represents the name of the file or directory whose
        security is being retrieved.

    RequestedInformation - A pointer to the security information being
        requested.

    pSecurityDescriptor - A pointer to the buffer to receive a copy of
        the secrity descriptor protecting the object that the caller
        has the rigth to view.  The security descriptor is returned in
        self-relative format.

    nLength - The size, in bytes, of the security descriptor buffer.

    lpnLengthNeeded - A pointer to the variable to receive the number
        of bytes needed to store the complete secruity descriptor.  If
        returned number of bytes is less than or equal to nLength then
        the entire security descriptor is returned in the output
        buffer, otherwise none of the descriptor is returned.

Return Value:

    TRUE is returned for success, FALSE if access is denied or if the
        buffer is too small to hold the security descriptor.


--*/
{
    NTSTATUS Status;
    HANDLE FileHandle;
    ACCESS_MASK DesiredAccess;

    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileName;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    IO_STATUS_BLOCK IoStatusBlock;
    PVOID FreeBuffer;

    QuerySecurityAccessMask(
        RequestedInformation,
        &DesiredAccess
        );

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFileName,
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        BaseSetLastNTError(STATUS_OBJECT_NAME_INVALID);
        return FALSE;
        }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    //
    // Notice that FILE_OPEN_REPARSE_POINT inhibits the reparse behavior. Thus, the
    // security will always be set, as before, in the file denoted by the name.
    //

    Status = NtOpenFile(
                 &FileHandle,
                 DesiredAccess,
                 &Obja,
                 &IoStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 FILE_OPEN_REPARSE_POINT
                 );

    //
    // Back-level file systems may not support the FILE_OPEN_REPARSE_POINT
    // flag. We treat this case explicitly.
    //

    if ( Status == STATUS_INVALID_PARAMETER ) {
        //
        // Open without inhibiting the reparse behavior.
        //

        Status = NtOpenFile(
                     &FileHandle,
                     DesiredAccess,
                     &Obja,
                     &IoStatusBlock,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     0
                     );
        }

    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

    if (NT_SUCCESS(Status)) {
        Status = NtQuerySecurityObject(
                     FileHandle,
                     RequestedInformation,
                     pSecurityDescriptor,
                     nLength,
                     lpnLengthNeeded
                     );
        NtClose(FileHandle);
    }


    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    return TRUE;
}

BOOL
APIENTRY
GetFileSecurityA(
    LPCSTR lpFileName,
    SECURITY_INFORMATION RequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD nLength,
    LPDWORD lpnLengthNeeded
    )

/*++

Routine Description:

    ANSI thunk to GetFileSecurityW

--*/

{

    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitAnsiString(&AnsiString,lpFileName);
    Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    return ( GetFileSecurityW( (LPCWSTR)Unicode->Buffer,
                               RequestedInformation,
                               pSecurityDescriptor,
                               nLength,
                               lpnLengthNeeded
                        )
           );
}




BOOL
APIENTRY
SetKernelObjectSecurity (
    HANDLE Handle,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR SecurityDescriptor
    )
/*++

Routine Description:

    This API can be used to set the security of a kernel object
    (process, file, event, etc.).  This call is only successful if the
    following conditions are met:

    o If the object's owner or group is to be set, the caller must
      have WRITE_OWNER permission or have SeTakeOwnershipPrivilege.

    o If the object's DACL is to be set, the caller must have
      WRITE_DAC permission or be the object's owner.

    o If the object's SACL is to be set, the caller must have
      SeSecurityPrivilege.

Arguments:

    Handle - Represents a handle of a kernel object.

    SecurityInformation - A pointer to information describing the
        contents of the Security Descriptor.

    pSecurityDescriptor - A pointer to a well formed Security
        Descriptor.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/

{
    NTSTATUS Status;

    Status = NtSetSecurityObject(
                 Handle,
                 SecurityInformation,
                 SecurityDescriptor
                 );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
APIENTRY
GetKernelObjectSecurity (
    HANDLE Handle,
    SECURITY_INFORMATION RequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD nLength,
    LPDWORD lpnLengthNeeded
    )
/*++

Routine Description:

    This API returns top the caller a copy of the security descriptor
    protecting a kernel object.  Based on the caller's access rights
    and privileges, this procedure will return a security descriptor
    containing the requested security descriptor fields.  To read the
    handle's security descriptor the caller must be granted
    READ_CONTROL access or be the owner of the object.  In addition,
    the caller must have SeSecurityPrivilege privilege to read the
    system ACL.


Arguments:

    Handle - Represents a handle of a kernel object.

    RequestedInformation - A pointer to the security information being
        requested.

    pSecurityDescriptor - A pointer to the buffer to receive a copy of
        the secrity descriptor protecting the object that the caller
        has the rigth to view.  The security descriptor is returned in
        self-relative format.

    nLength - The size, in bytes, of the security descriptor buffer.

    lpnLengthNeeded - A pointer to the variable to receive the number
        of bytes needed to store the complete secruity descriptor.  If
        returned number of bytes is less than or equal to nLength then
        the entire security descriptor is returned in the output
        buffer, otherwise none of the descriptor is returned.


Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    NTSTATUS Status;

    Status = NtQuerySecurityObject(
                 Handle,
                 RequestedInformation,
                 pSecurityDescriptor,
                 nLength,
                 lpnLengthNeeded
                 );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;

}


BOOL
APIENTRY
ImpersonateNamedPipeClient(
    IN HANDLE hNamedPipe
    )
/*++

Routine Description:

    Impersonate a named pipe client application.

Arguments:

    hNamedPipe - Handle to a named pipe.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.


--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    Status =  NtFsControlFile(
                  hNamedPipe,
                  NULL,
                  NULL,
                  NULL,
                  &IoStatusBlock,
                  FSCTL_PIPE_IMPERSONATE,
                  NULL,
                  0,
                  NULL,
                  0
                 );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
ImpersonateSelf(
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    )

/*++

Routine Description:

    This routine may be used to obtain an Impersonation token representing
    your own process's context.  This may be useful for enabling a privilege
    for a single thread rather than for the entire process; or changing
    the default DACL for a single thread.

    The token is assigned to the callers thread.



Arguments:

    ImpersonationLevel - The level to make the impersonation token.



Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{

    NTSTATUS Status;

    Status = RtlImpersonateSelf( ImpersonationLevel );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;


}



BOOL
APIENTRY
RevertToSelf (
    VOID
    )
/*++

Routine Description:

    Terminate impersonation of a named pipe client application.

Arguments:

    None.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/

{
    HANDLE NewToken;
    NTSTATUS Status;

    NewToken = NULL;
    Status = NtSetInformationThread(
                 NtCurrentThread(),
                 ThreadImpersonationToken,
                 (PVOID)&NewToken,
                 (ULONG)sizeof(HANDLE)
                 );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;

}



BOOL
APIENTRY
SetThreadToken (
    PHANDLE Thread,
    HANDLE Token
    )
/*++

Routine Description:

    Assigns the specified impersonation token to the specified
    thread.

Arguments:

    Thread - Specifies the thread whose token is to be assigned.
        If NULL is specified, then the caller's thread is assumed.

    Token - The token to assign.  Must be open for TOKEN_IMPERSONATE
        access.  If null, then causes the specified thread to stop
        impersonating.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/

{
    NTSTATUS Status;
    HANDLE TargetThread;

    if (ARGUMENT_PRESENT(Thread)) {
        TargetThread = (*Thread);
    } else {
        TargetThread = NtCurrentThread();
    }


    Status = NtSetInformationThread(
                 TargetThread,
                 ThreadImpersonationToken,
                 (PVOID)&Token,
                 (ULONG)sizeof(HANDLE)
                 );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;

}

BOOL
LookupAccountNameInternal(
    LPCWSTR lpSystemName,
    LPCWSTR lpAccountName,
    PSID Sid,
    LPDWORD cbSid,
    LPWSTR ReferencedDomainName,
    LPDWORD cbReferencedDomainName,
    PSID_NAME_USE peUse,
    BOOL fUnicode
    )

/*++

Routine Description:

    Translates a passed name into an account SID.  It will also return
    the name and SID of the first domain in which this name was found.

Arguments:

    lpSystemName - Supplies the name of the system at which the lookup
        is to be performed.  If the null string is provided, the local
        system is assumed.

    lpAccountName - Supplies the account name.

    Sid - Returns the SID corresponding to the passed account name.

    cbSid - Supplies the size of the buffer passed in for Sid.  If
        the buffer size is not big enough, this parameter will
        return the size necessary to hold the output Sid.

    ReferencedDomainName - Returns the name of the domain in which the
        name was found.

    cbReferencedDomainName - Supplies the size (in Wide characters) of the
        ReferencedDomainName buffer.  If the buffer size is not large
        enough, this parameter will return the size necessary to hold
        the null-terminated output domain name.  If the buffer size is
        large enough, tis parameter will return the size (in Ansi characters,
        excluding the terminating null) of the Referenced Domain name.

    peUse - Returns an enumerated type inidicating the type of the
        account.

    fUnicode - indicates whether the caller wants a count of unicode or
        ansi characters.

Return Value:

    BOOL - TRUE is returned if successful, else FALSE.

--*/

{
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE PolicyHandle;
    NTSTATUS Status;
    NTSTATUS TmpStatus;
    UNICODE_STRING Name;
    PLSA_REFERENCED_DOMAIN_LIST ReferencedDomains = NULL;
    PLSA_TRANSLATED_SID TranslatedSid = NULL;
    PSID ReturnedDomainSid;
    UCHAR nSubAuthorities;
    UNICODE_STRING TmpString;
    DWORD ReturnedDomainNameSize;
    DWORD SidLengthRequired;
    BOOL Rc;
    UNICODE_STRING SystemName;
    PUNICODE_STRING pSystemName = NULL;

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    //
    // Set up the object attributes prior to opening the LSA.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0L,
        NULL,
        NULL
        );

    //
    // The InitializeObjectAttributes macro presently stores NULL for
    // the SecurityQualityOfService field, so we must manually copy that
    // structure for now.
    //

    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

    if ( ARGUMENT_PRESENT( lpSystemName )) {
        RtlInitUnicodeString( &SystemName, lpSystemName );
        pSystemName = &SystemName;
    }

    //
    // Open the LSA Policy Database for the target system.  This is the
    // starting point for the Name Lookup operation.
    //

    Status = LsaOpenPolicy(
                 pSystemName,
                 &ObjectAttributes,
                 POLICY_LOOKUP_NAMES,
                 &PolicyHandle
                 );

    if ( !NT_SUCCESS( Status )) {

        BaseSetLastNTError( Status );
        return( FALSE );
    }

    RtlInitUnicodeString( &Name, lpAccountName );

    //
    // Attempt to translate the Name to a Sid.
    //

    Status = LsaLookupNames(
                 PolicyHandle,
                 1,
                 &Name,
                 &ReferencedDomains,
                 &TranslatedSid
                 );

#if DBG
//
// This code is useful for tracking down components that call Lookup code
// before the system is initialized
//
    // ASSERT( Status != STATUS_INVALID_SERVER_STATE );
    if ( Status == STATUS_INVALID_SERVER_STATE ) {

        DbgPrint( "Process: %lu, Thread: %lu\n", GetCurrentProcessId(), GetCurrentThreadId() );
    }
#endif

    //
    // Close the Policy Handle,  which is not needed after here.
    //

    TmpStatus = LsaClose( PolicyHandle );
//    ASSERT( NT_SUCCESS( TmpStatus ));

    //
    // If an error was returned, check specifically for STATUS_NONE_MAPPED.
    // In this case, we may need to dispose of the returned Referenced Domain
    // List and Translated Sid structures.  For all other errors,
    // LsaLookupNames() frees these structures prior to exit.
    //

    if ( !NT_SUCCESS( Status )) {

        if (Status == STATUS_NONE_MAPPED) {

            if (ReferencedDomains != NULL) {

                TmpStatus = LsaFreeMemory( ReferencedDomains );
                ASSERT( NT_SUCCESS( TmpStatus ));
            }

            if (TranslatedSid != NULL) {

                TmpStatus = LsaFreeMemory( TranslatedSid );
                ASSERT( NT_SUCCESS( TmpStatus ));
            }
        }

        BaseSetLastNTError( Status );
        return( FALSE );
    }

    //
    // The Name was successfully translated.  There should be exactly
    // one Referenced Domain and its DomainIndex should be zero.
    //

    ASSERT ( TranslatedSid->DomainIndex == 0 );
    ASSERT ( ReferencedDomains != NULL);
    ASSERT ( ReferencedDomains->Domains != NULL );

    //
    // Calculate the lengths of the returned Sid and Domain Name (in Wide
    // Characters, excluding null).
    //
    if ( !fUnicode ) {
        RtlUnicodeToMultiByteSize(&ReturnedDomainNameSize,
                                  ReferencedDomains->Domains->Name.Buffer,
                                  ReferencedDomains->Domains->Name.Length);
    } else {
        ReturnedDomainNameSize = (ReferencedDomains->Domains->Name.Length / sizeof(WCHAR));
    }
    ReturnedDomainSid = ReferencedDomains->Domains[ TranslatedSid->DomainIndex ].Sid;
    nSubAuthorities = (*GetSidSubAuthorityCount( ReturnedDomainSid ));

    //
    // If the sid is not a domain sid, add on space for the subathority
    //

    if (TranslatedSid->Use != SidTypeDomain) {
        nSubAuthorities += (UCHAR)1;
    }
    SidLengthRequired = GetSidLengthRequired( nSubAuthorities );

    //
    // Check if buffer sizes are too small.  For the returned domain,
    // the size in Wide characters provided must allow for the null
    // terminator that will be appended to the returned name.
    //

    if ( (SidLengthRequired > *cbSid) ||
         (ReturnedDomainNameSize + 1 > *cbReferencedDomainName)
       ) {

        //
        // One or both buffers are too small.  Return sizes required for
        // both buffers.
        //

        *cbSid = SidLengthRequired;
        *cbReferencedDomainName = ReturnedDomainNameSize + 1;
        BaseSetLastNTError( STATUS_BUFFER_TOO_SMALL );
        Rc = FALSE;

    } else {

        //
        // The provided buffers are large enough.  Build the SID in the
        // return buffer by combining the Domain Sid returned by
        // LsaLookupNames() with the Relative Id returned.  The final SID
        // will have one more SubAuthority than the Domain Sid.
        //

        CopySid( *cbSid, Sid, ReturnedDomainSid );
        *GetSidSubAuthorityCount( Sid ) = nSubAuthorities;

        if (TranslatedSid->Use != SidTypeDomain) {
            *GetSidSubAuthority( Sid, (DWORD)(nSubAuthorities - 1) ) = TranslatedSid->RelativeId;
        }

        //
        // Copy the Domain Name into the return buffer and NULL terminate it.
        //

        TmpString.Buffer = ReferencedDomainName;
        TmpString.Length = 0;

        //
        // Watch for overflow of 16-bit name length
        //

        if (*cbReferencedDomainName < (DWORD) 32767) {

            TmpString.MaximumLength = (USHORT)((*cbReferencedDomainName) * sizeof(WCHAR));

        } else {

            TmpString.MaximumLength = (USHORT) 65534;
        }

        RtlCopyUnicodeString( &TmpString, &ReferencedDomains->Domains->Name );

        TmpString.Buffer[TmpString.Length/sizeof(WCHAR)] = (WCHAR) 0;

        //
        // Copy the Sid Use field.
        //

        *peUse = TranslatedSid->Use;

        //
        // Return the size (in Wide Characters, excluding the terminating
        // null) of the returned Referenced Domain Name.
        //

        *cbReferencedDomainName = ReturnedDomainNameSize;

        Rc = TRUE;
    }

    //
    // If necessary, free the structures returned by the LsaLookupNames()
    // function.
    //

    if (ReferencedDomains !=  NULL) {

        Status = LsaFreeMemory( ReferencedDomains );
        ASSERT( NT_SUCCESS( Status ));
    }

    if (TranslatedSid != NULL) {

        Status = LsaFreeMemory( TranslatedSid );
        ASSERT( NT_SUCCESS( Status ));
    }

    return( Rc );
}



BOOL
APIENTRY
LookupAccountNameA(
    LPCSTR lpSystemName,
    LPCSTR lpAccountName,
    PSID Sid,
    LPDWORD cbSid,
    LPSTR ReferencedDomainName,
    LPDWORD cbReferencedDomainName,
    PSID_NAME_USE peUse
    )

/*++

Routine Description:

    ANSI Thunk to LookupAccountNameW

Arguments:

    lpSystemName - Supplies the name of the system at which the lookup
        is to be performed.  If the null string is provided, the local
        system is assumed.

    lpAccountName - Supplies the account name.

    Sid - Returns the SID corresponding to the passed account name.

    cbSid - Supplies the size of the buffer passed in for Sid.  If
        the buffer size is not big enough, this parameter will
        return the size necessary to hold the output Sid.

    ReferencedDomainName - Returns the name of the domain in which the
        name was found.

    cbReferencedDomainName - Supplies the size (in Ansi characters) of the
        ReferencedDomainName buffer.  If the buffer size is not large
        enough, this parameter will return the size necessary to hold
        the null-terminated output domain name.  If the buffer size is
        large enough, tis parameter will return the size (in Ansi characters,
        excluding the terminating null) of the Referenced Domain name.

    peUse - Returns an enumerated type indicating the type of the
        account.

Return Value:

    BOOL - TRUE is returned if successful, else FALSE.

--*/

{
    UNICODE_STRING Unicode;
    UNICODE_STRING TmpUnicode;
    ANSI_STRING  AnsiString;
    PWSTR WReferencedDomainName = NULL;
    UNICODE_STRING SystemName;
    PWSTR pSystemName = NULL;
    NTSTATUS Status;
    BOOL rc = TRUE;
    DWORD cbInitReferencedDomainName;

    Unicode.Buffer = NULL;
    SystemName.Buffer = NULL;

    //
    // Save the original buffer size
    //

    cbInitReferencedDomainName = *cbReferencedDomainName;

    //
    // Convert the passed lpAccountName to a WCHAR string to be
    // passed to the ..W routine.  Note that we cannot use the
    // StaticUnicodeString in the Thread Environment Block because
    // this is used by LdrpWalkImportDescriptor, called from the
    // client RPC stub code of the LsaOpenPolicy() call in
    // LookupAccountNameW.
    //

    RtlInitAnsiString( &AnsiString, lpAccountName );
    Status = RtlAnsiStringToUnicodeString( &Unicode, &AnsiString, TRUE );

    if (!NT_SUCCESS(Status)) {

        rc = FALSE;
    }

    //
    // Allocate a temporary buffer for ReferencedDomainName that
    // is twice as large as what was passed to adjust for the
    // intermediate conversion to a WCHAR string.
    //

    if (rc) {

        WReferencedDomainName = LocalAlloc(
                                    LMEM_FIXED,
                                    sizeof(WCHAR) * (*cbReferencedDomainName)
                                    );

        if (WReferencedDomainName == NULL) {

            Status = STATUS_NO_MEMORY;
            rc = FALSE;
        }
    }

    //
    // If the target system name is non NULL, convert it to Unicode
    //

    if (rc) {

        if ( ARGUMENT_PRESENT( lpSystemName ) ) {

            RtlInitAnsiString( &AnsiString, lpSystemName );
            Status = RtlAnsiStringToUnicodeString( &SystemName, &AnsiString, TRUE );

            if (!NT_SUCCESS(Status)) {

                rc = FALSE;
            }

            pSystemName = SystemName.Buffer;
        }
    }

    //
    // Lookup the Account Sid and obtain its Unicode Account Name.
    //

    if (rc) {

        rc = LookupAccountNameInternal(
                 (LPCWSTR)pSystemName,
                 (LPCWSTR)Unicode.Buffer,
                 Sid,
                 cbSid,
                 WReferencedDomainName,
                 cbReferencedDomainName,
                 peUse,
                 FALSE          // not unicode
                 );
    }

    if ( SystemName.Buffer != NULL ) {

        RtlFreeUnicodeString( &SystemName );
    }

    //
    // Convert the returned null-terminated WCHAR string
    // back to a null-terminated CHAR string.
    //

    if (rc) {

        RtlInitUnicodeString( &TmpUnicode, WReferencedDomainName );
        AnsiString.Buffer = ReferencedDomainName;

        //
        // Watch for 16-bit overflow of MaximumLength
        //

        if (cbInitReferencedDomainName <= (DWORD) 65535) {

            AnsiString.MaximumLength = (USHORT) cbInitReferencedDomainName;

        } else {

            AnsiString.MaximumLength = (USHORT) 65535;
        }

        Status = RtlUnicodeStringToAnsiString( &AnsiString, &TmpUnicode, FALSE );

        if ( NT_SUCCESS( Status )) {

            ReferencedDomainName[AnsiString.Length] = 0;

        } else {

            rc = FALSE;
        }
    }

    if ( WReferencedDomainName != NULL) {

        LocalFree( WReferencedDomainName );
    }

    if (Unicode.Buffer != NULL) {

        RtlFreeUnicodeString(&Unicode);
    }

    if (!NT_SUCCESS(Status)) {

        BaseSetLastNTError( Status );
    }

    return( rc );
}





BOOL
APIENTRY
LookupAccountNameW(
    LPCWSTR lpSystemName,
    LPCWSTR lpAccountName,
    PSID Sid,
    LPDWORD cbSid,
    LPWSTR ReferencedDomainName,
    LPDWORD cbReferencedDomainName,
    PSID_NAME_USE peUse
    )

/*++

Routine Description:

    Translates a passed name into an account SID.  It will also return
    the name and SID of the first domain in which this name was found.

Arguments:

    lpSystemName - Supplies the name of the system at which the lookup
        is to be performed.  If the null string is provided, the local
        system is assumed.

    lpAccountName - Supplies the account name.

    Sid - Returns the SID corresponding to the passed account name.

    cbSid - Supplies the size of the buffer passed in for Sid.  If
        the buffer size is not big enough, this parameter will
        return the size necessary to hold the output Sid.

    ReferencedDomainName - Returns the name of the domain in which the
        name was found.

    cbReferencedDomainName - Supplies the size (in Wide characters) of the
        ReferencedDomainName buffer.  If the buffer size is not large
        enough, this parameter will return the size necessary to hold
        the null-terminated output domain name.  If the buffer size is
        large enough, tis parameter will return the size (in Ansi characters,
        excluding the terminating null) of the Referenced Domain name.

    peUse - Returns an enumerated type inidicating the type of the
        account.

Return Value:

    BOOL - TRUE is returned if successful, else FALSE.

--*/

{
    return(LookupAccountNameInternal( lpSystemName,
                                      lpAccountName,
                                      Sid,
                                      cbSid,
                                      ReferencedDomainName,
                                      cbReferencedDomainName,
                                      peUse,
                                      TRUE              // Unicode
                                    ) );

}


BOOL
APIENTRY
LookupAccountSidInternal(
    LPCWSTR lpSystemName,
    PSID lpSid,
    LPWSTR lpName,
    LPDWORD cbName,
    LPWSTR lpReferencedDomainName,
    LPDWORD cbReferencedDomainName,
    PSID_NAME_USE peUse,
    BOOL fUnicode
    )

/*++

Routine Description:

    Translates a passed SID into an account name.  It will also return
    the name and SID of the first domain in which this SID was found.

Arguments:

    lpSystemName - Supplies the name of the system at which the lookup
        is to be performed.  If the null string is provided, the local
        system is assumed.

    lpSid - Supplies the account Sid.

    lpName - Returns the name corresponding to the passed account SID.

    cbName - Supplies the size (in Wide characters) of the buffer passed in for
        lpName.  This size must allow one character for the null terminator
        that will be appended to the returned name.  If the buffer size is not
        large enough, this parameter will return the size necessary to hold
        the null-terminated output name.  If the buffer size is large enough,
        this parameter will return the size (in Ansi characters, excluding
        the null terminator) of the name returned.

    lpReferencedDomainName - Returns the name of the domain in which the
        name was found.

    cbReferencedDomainName - Supplies the size (in Wide characters) of the
        ReferencedDomainName buffer.  This size must allow one charcter for the
        null terminator that will be appended to the returned name.  If the
        buffer size is not large enough, this parameter will return the size
        necessary to hold the output null-terminated domain name.  If the
        buffer size is large enough, the size of the returned name, excluding
        the terminating null will be returned.

    peUse - Returns an enumerated type inidicating the type of the
        account.

    fUnicode - indicates whether the caller wants a count of unicode or
        ansi characters.

Return Value:

    BOOL - TRUE if successful, else FALSE.

--*/

{

    PLSA_TRANSLATED_NAME Names;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE PolicyHandle;
    PLSA_REFERENCED_DOMAIN_LIST ReferencedDomains;
    DWORD ReturnedDomainNameSize;
    DWORD ReturnedNameSize;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    NTSTATUS Status;
    UNICODE_STRING TmpString;
    NTSTATUS TmpStatus;
    UNICODE_STRING SystemName;
    PUNICODE_STRING pSystemName = NULL;
    BOOLEAN Rc = FALSE;

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    //
    // Set up the object attributes prior to opening the LSA.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0L,
        NULL,
        NULL
        );

    //
    // The InitializeObjectAttributes macro presently stores NULL for
    // the SecurityQualityOfService field, so we must manually copy that
    // structure for now.
    //

    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

    if ( ARGUMENT_PRESENT( lpSystemName )) {
        RtlInitUnicodeString( &SystemName, lpSystemName );
        pSystemName = &SystemName;
    }

    Status = LsaOpenPolicy(
                 pSystemName,
                 &ObjectAttributes,
                 POLICY_LOOKUP_NAMES,
                 &PolicyHandle
                 );

    if ( !NT_SUCCESS( Status )) {

        BaseSetLastNTError( Status );
        return( FALSE );
    }

    Status = LsaLookupSids(
                 PolicyHandle,
                 1,
                 &lpSid,
                 &ReferencedDomains,
                 &Names
                 );
#if DBG
//
// This code is useful for tracking down components that call Lookup code
// before the system is initialized
//
    // ASSERT( Status != STATUS_INVALID_SERVER_STATE );
    if ( Status == STATUS_INVALID_SERVER_STATE ) {

        DbgPrint( "Process: %lu, Thread: %lu\n", GetCurrentProcessId(), GetCurrentThreadId() );
    }
#endif

    TmpStatus = LsaClose( PolicyHandle );


    //
    // If an error was returned, check specifically for STATUS_NONE_MAPPED.
    // In this case, we may need to dispose of the returned Referenced Domain
    // List and Names structures.  For all other errors, LsaLookupSids()
    // frees these structures prior to exit.
    //

    if ( !NT_SUCCESS( Status )) {

        if (Status == STATUS_NONE_MAPPED) {

            if (ReferencedDomains != NULL) {

                TmpStatus = LsaFreeMemory( ReferencedDomains );
                ASSERT( NT_SUCCESS( TmpStatus ));
            }

            if (Names != NULL) {

                TmpStatus = LsaFreeMemory( Names );
                ASSERT( NT_SUCCESS( TmpStatus ));
            }
        }

        BaseSetLastNTError( Status );
        return( FALSE );
    }

    //
    // The Sid was successfully translated.  There should be exactly
    // one Referenced Domain and its DomainIndex should be zero.
    //

    ASSERT(Names->DomainIndex == 0);
    ASSERT(ReferencedDomains != NULL);
    ASSERT(ReferencedDomains->Domains != NULL);

    if ( ! fUnicode ) {

        RtlUnicodeToMultiByteSize(&ReturnedNameSize,
                                  Names->Name.Buffer,
                                  Names->Name.Length);


        RtlUnicodeToMultiByteSize(&ReturnedDomainNameSize,
                                  ReferencedDomains->Domains->Name.Buffer,
                                  ReferencedDomains->Domains->Name.Length);

    } else {
        ReturnedNameSize = (Names->Name.Length / sizeof(WCHAR));
        ReturnedDomainNameSize = (ReferencedDomains->Domains->Name.Length / sizeof(WCHAR));
    }


    //
    // Check if buffer sizes  for the Name and Referenced Domain Name are too
    // small.  The sizes in Wide characters provided must allow for the null
    // terminator that will be appended to the returned names.
    //

    if ((ReturnedNameSize + 1 > *cbName) ||
        (ReturnedDomainNameSize + 1 > *cbReferencedDomainName)) {

        //
        // One or both buffers are too small.  Return sizes required for
        // both buffers, allowing one character for the null terminator.
        //

        *cbReferencedDomainName = ReturnedDomainNameSize + 1;
        *cbName = ReturnedNameSize + 1;
        BaseSetLastNTError( STATUS_BUFFER_TOO_SMALL );
        Rc = FALSE;

    } else {

        //
        // Both buffers are of sufficient size.  Copy in the Name
        // information and add NULL terminators.
        //

        TmpString.Buffer = lpName;
        TmpString.Length = 0;

        //
        // Watch for 16-bit overflow on buffer size.  Clamp size to
        // 16 bits if necessary.
        //

        if (*cbName <= 32766) {

            TmpString.MaximumLength = (USHORT)((*cbName) * sizeof(WCHAR));

        } else {

            TmpString.MaximumLength = (USHORT) 65532;
        }

        if ((*cbName) > 0) {

            RtlCopyUnicodeString( &TmpString, &Names->Name );
            TmpString.Buffer[TmpString.Length/sizeof(WCHAR)] = (WCHAR) 0;
        }

        //
        // Copy in the Referenced Domain information.
        //

        TmpString.Buffer = lpReferencedDomainName;
        TmpString.Length = 0;

        //
        // Watch for 16-bit overflow on buffer size.  Clamp size to
        // 16 bits if necessary.
        //

        if (*cbReferencedDomainName <= 32767) {

            TmpString.MaximumLength = (USHORT)((*cbReferencedDomainName) * sizeof(WCHAR));

        } else {

            TmpString.MaximumLength = (USHORT) 65534;
        }

        RtlCopyUnicodeString( &TmpString, &ReferencedDomains->Domains->Name );
        TmpString.Buffer[TmpString.Length/sizeof(WCHAR)] = (WCHAR) 0;

        //
        // Return the sizes (in Wide Characters, excluding the terminating
        // null) of the name and domain name.
        //

        *cbReferencedDomainName = ReturnedDomainNameSize;
        *cbName = ReturnedNameSize;

        // Copy in the Use field.
        //

        *peUse = Names->Use;
        Rc = TRUE;
    }

    //
    // If necessary, free output buffers returned by LsaLookupSids
    //

    if (Names != NULL) {

        Status = LsaFreeMemory( Names );
        ASSERT( NT_SUCCESS( Status ));
    }

    if (ReferencedDomains != NULL) {

        Status = LsaFreeMemory( ReferencedDomains );
        ASSERT( NT_SUCCESS( Status ));
    }

    return(Rc);
}



BOOL
APIENTRY
LookupAccountSidA(
    LPCSTR lpSystemName,
    PSID lpSid,
    LPSTR lpName,
    LPDWORD cbName,
    LPSTR lpReferencedDomainName,
    LPDWORD cbReferencedDomainName,
    PSID_NAME_USE peUse
    )
/*++

Routine Description:

    ANSI Thunk to LookupAccountSidW

Arguments:

    lpSystemName - Supplies the name of the system at which the lookup
        is to be performed.  If the null string is provided, the local
        system is assumed.

    lpSid - Supplies the account Sid.

    lpName - Returns the name corresponding to the passed account SID.

    cbName - Supplies the size (in Ansi characters) of the buffer passed in for
        lpName.  This size must allow one character for the null terminator
        that will be appended to the returned name.  If the buffer size is not
        large enough, this parameter will return the size necessary to hold
        the null-terminated output name.  If the buffer size is large enough,
        this parameter will return the size (in Ansi characters, excluding
        the null terminator) of the name returned.

    lpReferencedDomainName - Returns the name of the domain in which the
        name was found.

    cbReferencedDomainName - Supplies the size (in Ansi characters) of the
        ReferencedDomainName buffer.  This size must allow one charcter for the
        null terminator that will be appended to the returned name.  If the
        buffer size is not large enough, this parameter will return the size
        necessary to hold the output null-terminated domain name.  If the
        buffer size is large enough, the size of the returned name, excluding
        the terminating null will be returned.

    peUse - Returns an enumerated type indicating the type of the
        account.


Return Value:

    BOOL - TRUE if successful, else FALSE.

--*/

{
    NTSTATUS Status;
    LPWSTR WName = NULL;
    LPWSTR WReferencedDomainName = NULL;
    BOOL BoolStatus;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    UNICODE_STRING SystemName;
    PWSTR pSystemName = NULL;
    DWORD cbInitName, cbInitReferencedDomainName;

    //
    // Save the original buffer sizes specified for the returned account Name
    // and Referenced Domain Name.
    //

    cbInitName = *cbName;
    cbInitReferencedDomainName = *cbReferencedDomainName;

    //
    // Construct temporary buffers for the Name and Domain information
    // that are twice the size of those passed in to adjust for the
    // intermediate conversion to WCHAR strings.
    //

    if ( *cbName > 0 ) {
        WName = LocalAlloc( LMEM_FIXED, (*cbName) * sizeof(WCHAR));

        if ( !WName )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE ;
        }
    }

    if ( *cbReferencedDomainName > 0 ) {
        WReferencedDomainName =
            LocalAlloc( LMEM_FIXED, (*cbReferencedDomainName) * sizeof(WCHAR));

        if ( !WReferencedDomainName )
        {
            if ( WName )
            {
                LocalFree( WName );
            }

            SetLastError( ERROR_OUTOFMEMORY );

            return FALSE ;
        }
    }

    if ( ARGUMENT_PRESENT( lpSystemName ) ) {

        RtlInitAnsiString( &AnsiString, lpSystemName );
        RtlAnsiStringToUnicodeString( &SystemName, &AnsiString, TRUE );
        pSystemName = SystemName.Buffer;
    }

    BoolStatus = LookupAccountSidInternal(
                     (LPCWSTR)pSystemName,
                     lpSid,
                     WName,
                     cbName,
                     WReferencedDomainName,
                     cbReferencedDomainName,
                     peUse,
                     FALSE              // not unicode
                     );

    if ( ARGUMENT_PRESENT( lpSystemName ) ) {
        RtlFreeUnicodeString( &SystemName );
    }

    if ( BoolStatus ) {

        //
        // Copy the Name and DomainName information into the passed CHAR
        // buffers.
        //

        if ( ARGUMENT_PRESENT(lpName) ) {

            AnsiString.Buffer = lpName;

            //
            // Watch for 16-bit overflow on buffer size.  Clamp size to
            // 16 bits if necessary.
            //

            if (cbInitName <= (DWORD) 65535) {

                AnsiString.MaximumLength = (USHORT) cbInitName;

            } else {

                AnsiString.MaximumLength = (USHORT) 65535;
            }

            RtlInitUnicodeString( &UnicodeString, WName );
            Status = RtlUnicodeStringToAnsiString( &AnsiString,
                                                   &UnicodeString,
                                                   FALSE );
            ASSERT(NT_SUCCESS(Status));
            AnsiString.Buffer[AnsiString.Length] = 0;
        }

        if ( ARGUMENT_PRESENT(lpReferencedDomainName) ) {

            AnsiString.Buffer = lpReferencedDomainName;

            //
            // Watch for 16-bit overflow on buffer size.  Clamp size to
            // 16 bits if necessary.
            //

            if (cbInitReferencedDomainName <= (DWORD) 65535) {

                AnsiString.MaximumLength = (USHORT) cbInitReferencedDomainName;

            } else {

                AnsiString.MaximumLength = (USHORT) 65535;
            }

            RtlInitUnicodeString( &UnicodeString, WReferencedDomainName );
            Status = RtlUnicodeStringToAnsiString( &AnsiString,
                                                   &UnicodeString,
                                                   FALSE );
            ASSERT(NT_SUCCESS(Status));
            AnsiString.Buffer[AnsiString.Length] = 0;
        }

    }

    if (ARGUMENT_PRESENT(WName)) {
        LocalFree( WName );
    }
    if (ARGUMENT_PRESENT(WReferencedDomainName)) {
        LocalFree( WReferencedDomainName );
    }

    return( BoolStatus );
}




BOOL
APIENTRY
LookupAccountSidW(
    LPCWSTR lpSystemName,
    PSID lpSid,
    LPWSTR lpName,
    LPDWORD cbName,
    LPWSTR lpReferencedDomainName,
    LPDWORD cbReferencedDomainName,
    PSID_NAME_USE peUse
    )

/*++

Routine Description:

    Translates a passed SID into an account name.  It will also return
    the name and SID of the first domain in which this SID was found.

Arguments:

    lpSystemName - Supplies the name of the system at which the lookup
        is to be performed.  If the null string is provided, the local
        system is assumed.

    lpSid - Supplies the account Sid.

    lpName - Returns the name corresponding to the passed account SID.

    cbName - Supplies the size (in Wide characters) of the buffer passed in for
        lpName.  This size must allow one character for the null terminator
        that will be appended to the returned name.  If the buffer size is not
        large enough, this parameter will return the size necessary to hold
        the null-terminated output name.  If the buffer size is large enough,
        this parameter will return the size (in Ansi characters, excluding
        the null terminator) of the name returned.

    lpReferencedDomainName - Returns the name of the domain in which the
        name was found.

    cbReferencedDomainName - Supplies the size (in Wide characters) of the
        ReferencedDomainName buffer.  This size must allow one charcter for the
        null terminator that will be appended to the returned name.  If the
        buffer size is not large enough, this parameter will return the size
        necessary to hold the output null-terminated domain name.  If the
        buffer size is large enough, the size of the returned name, excluding
        the terminating null will be returned.

    peUse - Returns an enumerated type inidicating the type of the
        account.

Return Value:

    BOOL - TRUE if successful, else FALSE.

--*/

{
    return(LookupAccountSidInternal(
                lpSystemName,
                lpSid,
                lpName,
                cbName,
                lpReferencedDomainName,
                cbReferencedDomainName,
                peUse,
                TRUE                    // Unicode
                ));
}


BOOL
APIENTRY
LookupPrivilegeValueA(
    LPCSTR lpSystemName,
    LPCSTR lpName,
    PLUID lpLuid
    )
/*++

Routine Description:

    ANSI Thunk to LookupPrivilegeValueW().

Arguments:


Return Value:


--*/
{
    NTSTATUS Status;
    UNICODE_STRING USystemName, UName;
    ANSI_STRING ASystemName, AName;
    BOOL bool;

    RtlInitAnsiString( &ASystemName, lpSystemName );
    RtlInitAnsiString( &AName, lpName );

    USystemName.Buffer = NULL;
    UName.Buffer = NULL;

    Status = RtlAnsiStringToUnicodeString( &USystemName, &ASystemName, TRUE );
    if (NT_SUCCESS(Status)) {

        Status = RtlAnsiStringToUnicodeString( &UName, &AName, TRUE );
        if (NT_SUCCESS(Status)) {


            bool = LookupPrivilegeValueW( (LPCWSTR)USystemName.Buffer,
                                          (LPCWSTR)UName.Buffer,
                                          lpLuid
                                          );

            RtlFreeUnicodeString( &UName );
        }

        RtlFreeUnicodeString( &USystemName );
    }

    if (!NT_SUCCESS(Status)) {

        BaseSetLastNTError( Status );
        return( FALSE );

    }

    return(bool);


}

BOOL
APIENTRY
LookupPrivilegeValueW(
    LPCWSTR lpSystemName,
    LPCWSTR lpName,
    PLUID  lpLuid
    )

/*++

Routine Description:


    This function retrieves the value used on the target system
    to locally represent the specified privilege.  The privilege
    is specified by programmatic name.


Arguments:

    lpSystemName - Supplies the name of the system at which the lookup
        is to be performed.  If the null string is provided, the local
        system is assumed.

    lpName - provides the privilege's programmatic name.

    lpLuid - Receives the locally unique ID the privilege is known by on the
        target machine.


Return Value:



--*/
{
    NTSTATUS                    Status,
                                TmpStatus;

    LSA_HANDLE                  PolicyHandle;

    OBJECT_ATTRIBUTES           ObjectAttributes;

    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;

    UNICODE_STRING              USystemName,
                                UName;

    PUNICODE_STRING             SystemName = NULL;


    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    //
    // Set up the object attributes prior to opening the LSA.
    //

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0L, NULL, NULL );
    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;


    if ( ARGUMENT_PRESENT( lpSystemName )) {
        RtlInitUnicodeString( &USystemName, lpSystemName );
        SystemName = &USystemName;
    }

    Status = LsaOpenPolicy(
                 SystemName,
                 &ObjectAttributes,
                 POLICY_LOOKUP_NAMES,
                 &PolicyHandle
                 );

    if ( !NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( FALSE );
    }



    RtlInitUnicodeString( &UName, lpName );
    Status = LsaLookupPrivilegeValue( PolicyHandle, &UName, lpLuid );

    TmpStatus = LsaClose( PolicyHandle );
//    ASSERT( NT_SUCCESS( TmpStatus ));


    if ( !NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( FALSE );
    }


    return(TRUE);
}



BOOL
APIENTRY
LookupPrivilegeNameA(
    LPCSTR   lpSystemName,
    PLUID   lpLuid,
    LPSTR   lpName,
    LPDWORD cchName
    )
/*++

Routine Description:

    ANSI Thunk to LookupPrivilegeValueW().

Arguments:


Return Value:


--*/
{
    NTSTATUS       Status;

    ANSI_STRING    AnsiName;
    LPWSTR         UnicodeBuffer;
    UNICODE_STRING UnicodeString;

    ANSI_STRING    AnsiSystemName;
    UNICODE_STRING UnicodeSystemName;
    DWORD          LengthRequired;

    //
    // Convert the passed SystemName to Unicode.  Let the Rtl function
    // allocate the memory we need.
    //

    RtlInitAnsiString( &AnsiSystemName, lpSystemName );
    Status = RtlAnsiStringToUnicodeString( &UnicodeSystemName, &AnsiSystemName, TRUE );

    if (!NT_SUCCESS( Status )) {

        BaseSetLastNTError( Status );
        return( FALSE );
    }

    //
    // Make sure we don't exceed the limits of a unicode string.
    //

    if (*cchName > 0xFFFC) {
        *cchName = 0xFFFC;
    }

    UnicodeBuffer = RtlAllocateHeap( RtlProcessHeap(), 0, *cchName * sizeof(WCHAR) );

    if (UnicodeBuffer == NULL) {

        RtlFreeUnicodeString( &UnicodeSystemName );
        BaseSetLastNTError( STATUS_NO_MEMORY );
        return( FALSE );
    }

    //
    // Don't pass in cchName, since it will be overwritten by LookupPrivilegeNameW,
    // and we need it later.
    //

    LengthRequired = *cchName;

    if (!LookupPrivilegeNameW( (LPCWSTR)UnicodeSystemName.Buffer,
                               lpLuid,
                               UnicodeBuffer,
                               &LengthRequired
                               )) {

        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeBuffer );
        RtlFreeUnicodeString( &UnicodeSystemName );
        *cchName = LengthRequired;
        return(FALSE);
    }

    //
    // Now convert back to ANSI for the caller
    //

    RtlInitUnicodeString(&UnicodeString, UnicodeBuffer);

    AnsiName.Buffer = lpName;
    AnsiName.Length = 0;
    AnsiName.MaximumLength = (USHORT)*cchName;

    Status = RtlUnicodeStringToAnsiString(&AnsiName, &UnicodeString, FALSE);

    ASSERT( NT_SUCCESS( Status ));

    *cchName = AnsiName.Length;

    RtlFreeHeap( RtlProcessHeap(), 0, UnicodeBuffer );
    RtlFreeUnicodeString( &UnicodeSystemName );

    return(TRUE);
}



BOOL
APIENTRY
LookupPrivilegeNameW(
    LPCWSTR  lpSystemName,
    PLUID   lpLuid,
    LPWSTR  lpName,
    LPDWORD cchName
    )
/*++

Routine Description:


    This function returns the programmatic name corresponding to
    the privilege represented on the target system by the provided
    LUID.


Arguments:


    lpSystemName - Supplies the name of the system at which the lookup
        is to be performed.  If the null string is provided, the local
        system is assumed.


    lpLuid - is the locally unique ID the privilege is known by on the
        target machine.

    lpName - Receives the privilege's programmatic name.

    cchName - indicates how large the buffer is (in characters).  This
        count does not include the null-terminator that is added at the
        end of the string.



Return Value:



--*/
{
    NTSTATUS                    Status,
                                TmpStatus;
    LSA_HANDLE                  PolicyHandle;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    UNICODE_STRING              USystemName;
    PUNICODE_STRING             SystemName,
                                UName;


    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    //
    // Set up the object attributes prior to opening the LSA.
    //

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0L, NULL, NULL );
    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;


    SystemName = NULL;
    if ( ARGUMENT_PRESENT( lpSystemName )) {
        RtlInitUnicodeString( &USystemName, lpSystemName );
        SystemName = &USystemName;
    }

    Status = LsaOpenPolicy(
                 SystemName,
                 &ObjectAttributes,
                 POLICY_LOOKUP_NAMES,
                 &PolicyHandle
                 );

    if ( !NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( FALSE );
    }


    UName = NULL;
    Status = LsaLookupPrivilegeName( PolicyHandle,lpLuid, &UName );

    if (NT_SUCCESS(Status) ) {

        if ((DWORD)UName->Length + sizeof( WCHAR) > (*cchName) * sizeof( WCHAR )) {
            Status = STATUS_BUFFER_TOO_SMALL;
            (*cchName) = ( UName->Length + sizeof( WCHAR) ) / sizeof( WCHAR );

        } else {

            RtlMoveMemory( lpName, UName->Buffer, UName->Length );
            lpName[UName->Length/sizeof(WCHAR)] = 0;  // NULL terminate it
            (*cchName) = UName->Length / sizeof( WCHAR );
        }

        LsaFreeMemory( UName->Buffer );
        LsaFreeMemory( UName );
    }

    TmpStatus = LsaClose( PolicyHandle );
//    ASSERT( NT_SUCCESS( TmpStatus ));


    if ( !NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( FALSE );
    }


    return(TRUE);
}



BOOL
APIENTRY
LookupPrivilegeDisplayNameA(
    LPCSTR   lpSystemName,
    LPCSTR   lpName,
    LPSTR   lpDisplayName,
    LPDWORD cchDisplayName,
    LPDWORD lpLanguageId
    )

/*++

Routine Description:

    ANSI Thunk to LookupPrivilegeValueW().

Arguments:


Return Value:


--*/
{
    NTSTATUS                Status;

    UNICODE_STRING          UnicodeSystemName;
    UNICODE_STRING          UnicodeString;
    UNICODE_STRING          UnicodeName;

    ANSI_STRING             AnsiSystemName;
    ANSI_STRING             AnsiDisplayName;
    ANSI_STRING             AnsiName;

    LPWSTR                  UnicodeBuffer;
    DWORD                   RequiredLength;


    RtlInitAnsiString( &AnsiSystemName, lpSystemName );
    Status = RtlAnsiStringToUnicodeString( &UnicodeSystemName, &AnsiSystemName, TRUE );

    if (!NT_SUCCESS( Status )) {

        BaseSetLastNTError( Status );
        return( FALSE );
    }

    //
    // Make sure we don't exceed that limits of a unicode string.
    //

    if (*cchDisplayName > 0xFFFC) {
        *cchDisplayName = 0xFFFC;
    }

    UnicodeBuffer =  RtlAllocateHeap( RtlProcessHeap(), 0, *cchDisplayName * sizeof(WCHAR));

    if (UnicodeBuffer == NULL) {

        RtlFreeUnicodeString( &UnicodeSystemName );
        BaseSetLastNTError( STATUS_NO_MEMORY );
        return( FALSE );
    }

    RtlInitAnsiString( &AnsiName, lpName );
    Status = RtlAnsiStringToUnicodeString( &UnicodeName, &AnsiName, TRUE );

    if (!NT_SUCCESS( Status )) {

        RtlFreeUnicodeString( &UnicodeSystemName );
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeBuffer );
        BaseSetLastNTError( Status );
        return( FALSE );
    }

    RequiredLength = *cchDisplayName;

    if (! LookupPrivilegeDisplayNameW( (LPCWSTR)UnicodeSystemName.Buffer,
                                       (LPCWSTR)UnicodeName.Buffer,
                                       UnicodeBuffer,
                                       &RequiredLength,
                                       lpLanguageId
                                       )) {

        //
        // No need to set last error here, we can assume the W routine did so.
        //

        *cchDisplayName = RequiredLength;

        RtlFreeUnicodeString( &UnicodeSystemName );
        RtlFreeUnicodeString( &UnicodeName );
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeBuffer );
        return( FALSE );
    }

    //
    // Now convert back to ANSI for the caller
    //

    RtlInitUnicodeString( &UnicodeString, UnicodeBuffer );

    AnsiDisplayName.Buffer = lpDisplayName;
    AnsiDisplayName.Length = 0;
    AnsiDisplayName.MaximumLength = (USHORT)(*cchDisplayName);

    Status = RtlUnicodeStringToAnsiString( &AnsiDisplayName, &UnicodeString, FALSE );

    ASSERT( NT_SUCCESS( Status ));

    *cchDisplayName = AnsiDisplayName.Length;

    RtlFreeUnicodeString( &UnicodeSystemName );
    RtlFreeUnicodeString( &UnicodeName );
    RtlFreeHeap( RtlProcessHeap(), 0, UnicodeBuffer );

    return( TRUE );
}



BOOL
APIENTRY
LookupPrivilegeDisplayNameW(
    LPCWSTR  lpSystemName,
    LPCWSTR  lpName,
    LPWSTR  lpDisplayName,
    LPDWORD cchDisplayName,
    LPDWORD lpLanguageId
    )

/*++

Routine Description:

    This function retrieves a displayable name representing the
    specified privilege.


Arguments:

    lpSystemName - Supplies the name of the system at which the lookup
        is to be performed.  If the null string is provided, the local
        system is assumed.


    lpName - provides the privilege's programmatic name.


    lpDisplayName - Receives the privilege's displayable name.

    cchDisplayName - indicates how large the buffer is (in characters).  This
        count does not include the null-terminator that is added at the
        end of the string.

    lpLanguageId - Receives the language of the returned displayable
        name.


Return Value:


--*/

{
    NTSTATUS                    Status,
                                TmpStatus;

    LSA_HANDLE                  PolicyHandle;

    OBJECT_ATTRIBUTES           ObjectAttributes;

    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;

    UNICODE_STRING              USystemName,
                                UName;

    PUNICODE_STRING             SystemName,
                                UDisplayName;

    SHORT                       LanguageId;


    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    //
    // Set up the object attributes prior to opening the LSA.
    //

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0L, NULL, NULL );
    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;


    SystemName = NULL;
    if ( ARGUMENT_PRESENT( lpSystemName )) {
        RtlInitUnicodeString( &USystemName, lpSystemName );
        SystemName = &USystemName;
    }

    Status = LsaOpenPolicy(
                 SystemName,
                 &ObjectAttributes,
                 POLICY_LOOKUP_NAMES,
                 &PolicyHandle
                 );

    if ( !NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( FALSE );
    }

    RtlInitUnicodeString( &UName, lpName );


    UDisplayName = NULL;
    Status = LsaLookupPrivilegeDisplayName( PolicyHandle,
                                            &UName,
                                            &UDisplayName,
                                            &LanguageId
                                            );
    (*lpLanguageId) = LanguageId;

    if (NT_SUCCESS(Status)) {

        if (UDisplayName->Length + sizeof(WCHAR) > (*cchDisplayName) * sizeof(WCHAR)) {
            Status = STATUS_BUFFER_TOO_SMALL;
            (*cchDisplayName) = (UDisplayName->Length + sizeof( WCHAR )) / sizeof( WCHAR );

        } else {

            RtlMoveMemory( lpDisplayName,
                           UDisplayName->Buffer,
                           UDisplayName->Length
                           );
            lpDisplayName[UDisplayName->Length/sizeof(WCHAR)] = 0;  // Null terminate it.
            (*cchDisplayName) = UDisplayName->Length / sizeof( WCHAR );
        }

        LsaFreeMemory( UDisplayName->Buffer );
        LsaFreeMemory( UDisplayName );

    }
    TmpStatus = LsaClose( PolicyHandle );
//    ASSERT( NT_SUCCESS( TmpStatus ));


    if ( !NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( FALSE );
    }


    return(TRUE);
}


BOOL
APIENTRY
ImpersonateAnonymousToken(
    IN HANDLE ThreadHandle
    )
/*++

Routine Description:

    Win32 wrapper for NtImpersonateAnonymousToken();

    Impersonates the system's anonymous logon token on this thread.

Arguments:

    ThreadHandle - Handle to the thread to do the impersonation.

Return Value:

    TRUE for success, FALSE for failure.

    Call GetLastError() for more information.

--*/
{
    NTSTATUS Status;

    Status = NtImpersonateAnonymousToken(
                ThreadHandle
                );

    if ( !NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( FALSE );

    } else {
        return( TRUE );
    }
}





/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//               Private Routines                                          //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

VOID
SepFormatAccountSid(
    PSID Sid,
    LPWSTR OutputBuffer
    )
{
    UCHAR Buffer[128];
    UCHAR TmpBuffer[128];
    ANSI_STRING AccountName;
    UCHAR i;
    ULONG Tmp;
    UNICODE_STRING OutputString;
    PISID iSid;
    NTSTATUS Status;

    //
    // Do everything as ANSI for the time being, and then
    // convert to wide-char at the bottom.
    //
    // We need to do this until we have more complete c-runtime support
    // for w-char strings.
    //

    iSid = (PISID) Sid;

    OutputString.Buffer = OutputBuffer;
    OutputString.MaximumLength = 127;

    Buffer[0] = 0;
    TmpBuffer[0] = 0;

    AccountName.MaximumLength = 127;
    AccountName.Length = (USHORT)((GetLengthSid( Sid ) > MAXUSHORT) ? MAXUSHORT : GetLengthSid( Sid ));
    AccountName.Buffer = Buffer;

    sprintf(TmpBuffer, "S-%u-", (USHORT)iSid->Revision );
    lstrcpy(Buffer, TmpBuffer);

    if (  (iSid->IdentifierAuthority.Value[0] != 0)  ||
          (iSid->IdentifierAuthority.Value[1] != 0)     ){
        sprintf(TmpBuffer, "0x%02hx%02hx%02hx%02hx%02hx%02hx",
                    (USHORT)iSid->IdentifierAuthority.Value[0],
                    (USHORT)iSid->IdentifierAuthority.Value[1],
                    (USHORT)iSid->IdentifierAuthority.Value[2],
                    (USHORT)iSid->IdentifierAuthority.Value[3],
                    (USHORT)iSid->IdentifierAuthority.Value[4],
                    (USHORT)iSid->IdentifierAuthority.Value[5] );
        lstrcat(Buffer, TmpBuffer);
    } else {
        Tmp = (ULONG)iSid->IdentifierAuthority.Value[5]          +
              (ULONG)(iSid->IdentifierAuthority.Value[4] <<  8)  +
              (ULONG)(iSid->IdentifierAuthority.Value[3] << 16)  +
              (ULONG)(iSid->IdentifierAuthority.Value[2] << 24);
        sprintf(TmpBuffer, "%lu", Tmp);
        lstrcat(Buffer, TmpBuffer);
    }

    for (i=0;i<iSid->SubAuthorityCount ;i++ ) {
        sprintf(TmpBuffer, "-%lu", iSid->SubAuthority[i]);
        lstrcat(Buffer, TmpBuffer);
    }

    Status = RtlAnsiStringToUnicodeString( &OutputString, &AccountName, FALSE );

    ASSERT( NT_SUCCESS( Status ));

    return;
}

BOOL
APIENTRY
CreateRestrictedToken(
    IN HANDLE ExistingTokenHandle,
    IN DWORD Flags,
    IN DWORD DisableSidCount,
    IN PSID_AND_ATTRIBUTES SidsToDisable OPTIONAL,
    IN DWORD DeletePrivilegeCount,
    IN PLUID_AND_ATTRIBUTES PrivilegesToDelete OPTIONAL,
    IN DWORD RestrictedSidCount,
    IN PSID_AND_ATTRIBUTES SidsToRestrict OPTIONAL,
    OUT PHANDLE NewTokenHandle
    )
{
    NTSTATUS Status;
    PTOKEN_GROUPS DisabledSids = NULL;
    PTOKEN_PRIVILEGES DeletedPrivileges = NULL;
    PTOKEN_GROUPS RestrictedSids = NULL;

    //
    // Convert the input parameters into the native NT format
    //

    if (DisableSidCount != 0) {
        if (SidsToDisable == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
        DisabledSids = (PTOKEN_GROUPS) LocalAlloc(0,sizeof(TOKEN_GROUPS) +
                                        (DisableSidCount - 1) * sizeof(SID_AND_ATTRIBUTES) );
        if (DisabledSids == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        DisabledSids->GroupCount = DisableSidCount;
        RtlCopyMemory(
            DisabledSids->Groups,
            SidsToDisable,
            DisableSidCount * sizeof(SID_AND_ATTRIBUTES)
            );
    }

    if (DeletePrivilegeCount != 0) {
        if (PrivilegesToDelete == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
        DeletedPrivileges = (PTOKEN_PRIVILEGES) LocalAlloc(0,sizeof(TOKEN_PRIVILEGES) +
                                        (DeletePrivilegeCount - 1) * sizeof(LUID_AND_ATTRIBUTES) );
        if (DeletedPrivileges == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        DeletedPrivileges->PrivilegeCount = DeletePrivilegeCount;
        RtlCopyMemory(
            DeletedPrivileges->Privileges,
            PrivilegesToDelete,
            DeletePrivilegeCount * sizeof(LUID_AND_ATTRIBUTES)
            );
    }

    if (RestrictedSidCount != 0) {
        if (SidsToRestrict == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
        RestrictedSids = (PTOKEN_GROUPS) LocalAlloc(0,sizeof(TOKEN_GROUPS) +
                                        (RestrictedSidCount - 1) * sizeof(SID_AND_ATTRIBUTES) );
        if (RestrictedSids == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RestrictedSids->GroupCount = RestrictedSidCount;
        RtlCopyMemory(
            RestrictedSids->Groups,
            SidsToRestrict,
            RestrictedSidCount * sizeof(SID_AND_ATTRIBUTES)
            );
    }

    Status = NtFilterToken(
                ExistingTokenHandle,
                Flags,
                DisabledSids,
                DeletedPrivileges,
                RestrictedSids,
                NewTokenHandle
                );

Cleanup:
    if (DisabledSids != NULL) {
        LocalFree(DisabledSids);
    }
    if (DeletedPrivileges != NULL) {
        LocalFree(DeletedPrivileges);
    }
    if (RestrictedSids != NULL) {
        LocalFree(RestrictedSids);
    }
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError( Status );
        return(FALSE);
    }
    return(TRUE);
}

BOOL
APIENTRY
IsTokenRestricted(
    IN HANDLE TokenHandle
    )
{
    PTOKEN_GROUPS RestrictedSids = NULL;
    ULONG ReturnLength;
    NTSTATUS Status;
    BOOL Result = FALSE;


    Status = NtQueryInformationToken(
                TokenHandle,
                TokenRestrictedSids,
                NULL,
                0,
                &ReturnLength
                );
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        BaseSetLastNTError(Status);
        return(FALSE);
    }

    RestrictedSids = (PTOKEN_GROUPS) LocalAlloc(0, ReturnLength);
    if (RestrictedSids == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    Status = NtQueryInformationToken(
                TokenHandle,
                TokenRestrictedSids,
                RestrictedSids,
                ReturnLength,
                &ReturnLength
                );
    if (NT_SUCCESS(Status))
    {
        if (RestrictedSids->GroupCount != 0)
        {
            Result = TRUE;
        }
    }
    else
    {
        BaseSetLastNTError(Status);
    }
    LocalFree(RestrictedSids);
    return(Result);
}


BOOL
APIENTRY
CheckTokenMembership(
    IN HANDLE TokenHandle OPTIONAL,
    IN PSID SidToCheck,
    OUT PBOOL IsMember
    )
/*++

Routine Description:

    This function checks to see whether the specified sid is enabled in
    the specified token.

Arguments:

    TokenHandle - If present, this token is checked for the sid. If not
        present then the current effective token will be used. This must
        be an impersonation token.

    SidToCheck - The sid to check for presence in the token

    IsMember - If the sid is enabled in the token, contains TRUE otherwise
        false.

Return Value:

    TRUE - The API completed successfully. It does not indicate that the
        sid is a member of the token.

    FALSE - The API failed. A more detailed status code can be retrieved
        via GetLastError()


--*/
{
    HANDLE ProcessToken = NULL;
    HANDLE EffectiveToken = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    PISECURITY_DESCRIPTOR SecDesc = NULL;
    ULONG SecurityDescriptorSize;
    GENERIC_MAPPING GenericMapping = {
        STANDARD_RIGHTS_READ,
        STANDARD_RIGHTS_EXECUTE,
        STANDARD_RIGHTS_WRITE,
        STANDARD_RIGHTS_ALL };
    //
    // The size of the privilege set needs to contain the set itself plus
    // any privileges that may be used. The privileges that are used
    // are SeTakeOwnership and SeSecurity, plus one for good measure
    //

    BYTE PrivilegeSetBuffer[sizeof(PRIVILEGE_SET) + 3*sizeof(LUID_AND_ATTRIBUTES)];
    PPRIVILEGE_SET PrivilegeSet = (PPRIVILEGE_SET) PrivilegeSetBuffer;
    ULONG PrivilegeSetLength = sizeof(PrivilegeSetBuffer);
    ACCESS_MASK AccessGranted = 0;
    NTSTATUS AccessStatus = 0;
    PACL Dacl = NULL;

#define MEMBER_ACCESS 1

    *IsMember = FALSE;

    //
    // Get a handle to the token
    //

    if (ARGUMENT_PRESENT(TokenHandle))
    {
        EffectiveToken = TokenHandle;
    }
    else
    {
        Status = NtOpenThreadToken(
                    NtCurrentThread(),
                    TOKEN_QUERY,
                    FALSE,              // don't open as self
                    &EffectiveToken
                    );

        //
        // if there is no thread token, try the process token
        //

        if (Status == STATUS_NO_TOKEN)
        {
            Status = NtOpenProcessToken(
                        NtCurrentProcess(),
                        TOKEN_QUERY | TOKEN_DUPLICATE,
                        &ProcessToken
                        );
            //
            // If we have a process token, we need to convert it to an
            // impersonation token
            //

            if (NT_SUCCESS(Status))
            {
                BOOL Result;
                Result = DuplicateToken(
                            ProcessToken,
                            SecurityImpersonation,
                            &EffectiveToken
                            );

                CloseHandle(ProcessToken);
                if (!Result)
                {
                    return(FALSE);
                }
            }
        }

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

    }

    //
    // Construct a security descriptor to pass to access check
    //

    //
    // The size is equal to the size of an SD + twice the length of the SID
    // (for owner and group) + size of the DACL = sizeof ACL + size of the
    // ACE, which is an ACE + length of
    // ths SID.
    //

    SecurityDescriptorSize = sizeof(SECURITY_DESCRIPTOR) +
                                sizeof(ACCESS_ALLOWED_ACE) +
                                sizeof(ACL) +
                                3 * RtlLengthSid(SidToCheck);

    SecDesc = (PISECURITY_DESCRIPTOR) LocalAlloc(LMEM_ZEROINIT, SecurityDescriptorSize );
    if (SecDesc == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    Dacl = (PACL) (SecDesc + 1);

    RtlCreateSecurityDescriptor(
        SecDesc,
        SECURITY_DESCRIPTOR_REVISION
        );

    //
    // Fill in fields of security descriptor
    //

    RtlSetOwnerSecurityDescriptor(
        SecDesc,
        SidToCheck,
        FALSE
        );
    RtlSetGroupSecurityDescriptor(
        SecDesc,
        SidToCheck,
        FALSE
        );

    Status = RtlCreateAcl(
                Dacl,
                SecurityDescriptorSize - sizeof(SECURITY_DESCRIPTOR),
                ACL_REVISION
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Status = RtlAddAccessAllowedAce(
                Dacl,
                ACL_REVISION,
                MEMBER_ACCESS,
                SidToCheck
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Set the DACL on the security descriptor
    //

    Status = RtlSetDaclSecurityDescriptor(
                SecDesc,
                TRUE,   // DACL present
                Dacl,
                FALSE   // not defaulted
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = NtAccessCheck(
                SecDesc,
                EffectiveToken,
                MEMBER_ACCESS,
                &GenericMapping,
                PrivilegeSet,
                &PrivilegeSetLength,
                &AccessGranted,
                &AccessStatus
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // if the access check failed, then the sid is not a member of the
    // token
    //

    if ((AccessStatus == STATUS_SUCCESS) && (AccessGranted == MEMBER_ACCESS))
    {
        *IsMember = TRUE;
    }




Cleanup:
    if (!ARGUMENT_PRESENT(TokenHandle) && (EffectiveToken != NULL))
    {
        (VOID) NtClose(EffectiveToken);
    }

    if (SecDesc != NULL)
    {
        LocalFree(SecDesc);
    }

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return(FALSE);
    }
    else
    {
        return(TRUE);
    }
}

BOOL
APIENTRY
MakeAbsoluteSD2 (
    PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
    LPDWORD lpdwBufferSize
    )
/*++

Routine Description:

    Converts a security descriptor from self-relative format to absolute
    format

Arguments:

    pSelfRelativeSecurityDescriptor - Supplies a pointer to a security descriptor
        in Self-Relative format

    lpdwBufferSize - The size in bytes of the
        buffer pointed to by pSelfRelativeSecurityDescriptor.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = RtlSelfRelativeToAbsoluteSD2 (
                pSelfRelativeSecurityDescriptor,
                lpdwBufferSize
                );

    //
    // MakeAbsoluteSD2() has the same prototype as
    // RtlSelfRelativeToAbsoluteSD2() so the parameters check
    // returns the same parameter order if the caller passes invalid parameter.
    //

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;

} // MakeAbsoluteSD2()




DWORD
APIENTRY
GetSecurityDescriptorRMControl(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PUCHAR RMControl
    )
/*++

Routine Description:

    This procedure returns the RM Control flags from a SecurityDescriptor if
    SE_RM_CONTROL_VALID flags is present in the control field.

Arguments:

    SecurityDescriptor - Pointer to the SECURITY_DESCRIPTOR structure

    RMControl          - Returns the flags in the SecurityDescriptor if
                         SE_RM_CONTROL_VALID is set in the control bits of the
                         SecurityDescriptor.

Return Value:
    ERROR_INVALID_DATA      if the SE_RM_CONTROL_VALID flag is not present in
                            the security descriptor
    ERROR_SUCCESS           otherwise

--*/


{
    BOOLEAN Result;

    Result = RtlGetSecurityDescriptorRMControl(
                 SecurityDescriptor,
                 RMControl
                 );

    if (FALSE == Result)
    {
        return ERROR_INVALID_DATA;
    }

    return ERROR_SUCCESS;
}

DWORD
APIENTRY
SetSecurityDescriptorRMControl(
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PUCHAR RMControl OPTIONAL
    )


/*++

Routine Description:

    This procedure sets the RM Control flag in the control field of
    SecurityDescriptor and sets Sbz1 to the the byte to which RMContol point
    If RMControl is NULL then the bits are cleared.


Arguments:

    SecurityDescriptor - Pointer to the SECURITY_DESCRIPTOR structure

    RMControl          - Pointer to the flags to set. If NULL then the bits
                         are cleared.


Return Value: ERROR_SUCCESS

--*/

{
    RtlSetSecurityDescriptorRMControl(
        SecurityDescriptor,
        RMControl
        );

    return ERROR_SUCCESS;
}
