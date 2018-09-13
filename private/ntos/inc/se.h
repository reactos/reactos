/*++ BUILD Version: 0011    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    se.h

Abstract:

    This module contains the Security routines that are only callable
    from kernel mode.

    This file is included by including "ntos.h".

Author:

    Gary Kimura (GaryKi) 09-Mar-1989

Revision History:



--*/

#ifndef _SE_
#define _SE_



//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  Kernel mode only data structures                                        //
//  Opaque security data structures are defined in seopaque.h               //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

// begin_ntddk begin_nthal begin_ntifs
//
//  Security operation codes
//

typedef enum _SECURITY_OPERATION_CODE {
    SetSecurityDescriptor,
    QuerySecurityDescriptor,
    DeleteSecurityDescriptor,
    AssignSecurityDescriptor
    } SECURITY_OPERATION_CODE, *PSECURITY_OPERATION_CODE;

// end_ntddk end_nthal end_ntifs



//
//  Default security quota
//
//  This is the minimum amount of quota (in bytes) that will be
//  charged for security information for an object that has
//  security.
//

#define SE_DEFAULT_SECURITY_QUOTA   2048

// begin_ntifs
//
// Token Flags
//
// Flags that may be defined in the TokenFlags field of the token object,
// or in an ACCESS_STATE structure
//

#define TOKEN_HAS_TRAVERSE_PRIVILEGE    0x01
#define TOKEN_HAS_BACKUP_PRIVILEGE      0x02
#define TOKEN_HAS_RESTORE_PRIVILEGE     0x04
#define TOKEN_HAS_ADMIN_GROUP           0x08
#define TOKEN_IS_RESTRICTED             0x10

// end_ntifs


//
// General flag
//

#define SE_BACKUP_PRIVILEGES_CHECKED    0x00000010




// begin_ntddk begin_nthal begin_ntifs
//
//  Data structure used to capture subject security context
//  for access validations and auditing.
//
//  THE FIELDS OF THIS DATA STRUCTURE SHOULD BE CONSIDERED OPAQUE
//  BY ALL EXCEPT THE SECURITY ROUTINES.
//

typedef struct _SECURITY_SUBJECT_CONTEXT {
    PACCESS_TOKEN ClientToken;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    PACCESS_TOKEN PrimaryToken;
    PVOID ProcessAuditId;
    } SECURITY_SUBJECT_CONTEXT, *PSECURITY_SUBJECT_CONTEXT;

// end_ntddk end_nthal end_ntifs
//
// where
//
//    ClientToken - optionally points to a token object being used by the
//        subject's thread to impersonate a client.  If the subject's
//        thread is not impersonating a client, this field is set to null.
//        The token's reference count is incremented to count this field
//        as an outstanding reference.
//
//    ImpersonationLevel - Contains the impersonation level of the subject's
//        thread.  This field is only meaningful if the ClientToken field
//        is not null.  This field over-rides any higher impersonation
//        level value that might be in the client's token.
//
//    PrimaryToken - points the the subject's primary token.  The token's
//        reference count is incremented to count this field value as an
//        outstanding reference.
//
//    ProcessAuditId - Is an ID assigned to represent the subject's process.
//        As an implementation detail, this is the process object's address.
//        However, this field should not be treated as a pointer, and the
//        reference count of the process object is not incremented to
//        count it as an outstanding reference.
//


// begin_ntddk begin_nthal begin_ntifs
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                  ACCESS_STATE and related structures                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
//  Initial Privilege Set - Room for three privileges, which should
//  be enough for most applications.  This structure exists so that
//  it can be imbedded in an ACCESS_STATE structure.  Use PRIVILEGE_SET
//  for all other references to Privilege sets.
//

#define INITIAL_PRIVILEGE_COUNT         3

typedef struct _INITIAL_PRIVILEGE_SET {
    ULONG PrivilegeCount;
    ULONG Control;
    LUID_AND_ATTRIBUTES Privilege[INITIAL_PRIVILEGE_COUNT];
    } INITIAL_PRIVILEGE_SET, * PINITIAL_PRIVILEGE_SET;



//
// Combine the information that describes the state
// of an access-in-progress into a single structure
//


typedef struct _ACCESS_STATE {
   LUID OperationID;
   BOOLEAN SecurityEvaluated;
   BOOLEAN GenerateAudit;
   BOOLEAN GenerateOnClose;
   BOOLEAN PrivilegesAllocated;
   ULONG Flags;
   ACCESS_MASK RemainingDesiredAccess;
   ACCESS_MASK PreviouslyGrantedAccess;
   ACCESS_MASK OriginalDesiredAccess;
   SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
   PSECURITY_DESCRIPTOR SecurityDescriptor;
   PVOID AuxData;
   union {
      INITIAL_PRIVILEGE_SET InitialPrivilegeSet;
      PRIVILEGE_SET PrivilegeSet;
      } Privileges;

   BOOLEAN AuditPrivileges;
   UNICODE_STRING ObjectName;
   UNICODE_STRING ObjectTypeName;

   } ACCESS_STATE, *PACCESS_STATE;

// end_ntddk end_nthal end_ntifs

/*
where:

    OperationID - an LUID to identify the operation being performed.  This
        ID will be put in the audit log to allow non-contiguous operations
        on the same object to be associated with one another.

    SecurityEvaluated - a marker to be set by Parse Methods to indicate
        that security access checking and audit logging has been performed.

    Flags - Holds misc flags for reference during the access attempt.

    AuditHandleCreation - a flag set by SeOpenObjectAuditAlarm to indicate
        that auditing is to take place when the handle for the object
        is allocated.

    RemainingDesiredAccess - Access mask containing the access types that
        have not yet been granted.

    PreviouslyGrantedAccess - Access mask containing the access types that
        have been granted, one way or another (for example, a given access
        may be granted as a result of owning a privilege rather than being
        in an ACL.  A routine can check the privilege and mark the access
        as granted without doing a formal access check).

    SubjectSecurityContext - The subject's captured security context

    PrivilegesAllocated - Flag to indicate whether we have allocated
        space for the privilege set from pool memory, so it can be
        freed.

    SecurityDescriptor - Temporarily contains the security descriptor
       for the object being created between the time the user's
       security descriptor is captured and the time the security
       descriptor is passed to SeAssignSecurity.  NO ONE BUT
       SEASSIGNSECURITY SHOULD EVER LOOK IN THIS FIELD FOR AN
       OBJECT'S SECURITY DESCRIPTOR.

    AuxData - points to an auxillary data structure to be used for future
        expansion of the access state in an upwardly compatible way.  This
        field replaces the PrivilegesUsed pointer, which was for internal
        use only.

    Privileges - A set of privileges, some of which may have the
        UsedForAccess bit set.  If the pre-allocated number of privileges
        is not enough, we will allocate space from pool memory to allow
        for growth.

*/



//*******************************************************************************
//                                                                              *
//  Since the AccessState structure is publically exposed to driver             *
//  writers, this structure contains additional data added after NT 3.51.       *
//                                                                              *
//  Its contents must be accessed only through Se level interfaces,             *
//  never directly by name.                                                     *
//                                                                              *
//  This structure is pointed to by the AuxData field of the AccessState.       *
//  It is allocated by SeCreateAccessState and freed by SeDeleteAccessState.    *
//                                                                              *
//  DO NOT EXPOSE THIS STRUCTURE TO THE PUBLIC.                                 *
//                                                                              *
//*******************************************************************************


typedef struct _AUX_ACCESS_DATA {
    PPRIVILEGE_SET PrivilegesUsed;
    GENERIC_MAPPING GenericMapping;
    ACCESS_MASK AccessesToAudit;
} AUX_ACCESS_DATA, *PAUX_ACCESS_DATA;

/*
where:

    PrivilegesUsed - Points to the set of privileges used during the access
        validation.

    GenericMapping - Points to the generic mapping for the object being accessed.
        Normally this would be filled in with the generic mapping passed to
        SeCreateAccessState, but in the case of the IO system (which does not
        know the type of object being accessed until it parses the name),
        it must be filled in later.  See the discussion of the GenericMapping
        parameter in SeCreateAccessState for more details.

    AccessToAudit - Used as a temporary holding area for the access mask
        to put into the audit record.  This field is necessary because the
        access being put into the newly created handle may not be the ones
        we want to audit.  This occurs when a file is opened for read-only
        transacted mode, where a read only file is opened for write access.
        We don't want to audit the fact that we granted write access, since
        we really didn't, and customers would be confused to see the extra
        bit in the audit record.


*/



//
//  Structure describing whether or not a particular type of event
//  is being audited
//

typedef struct _SE_AUDITING_STATE {
    BOOLEAN AuditOnSuccess;
    BOOLEAN AuditOnFailure;
} SE_AUDITING_STATE, *PSE_AUDITING_STATE;




typedef struct _SE_PROCESS_AUDIT_INFO {
    PEPROCESS Process;
    PEPROCESS Parent;
} SE_PROCESS_AUDIT_INFO, *PSE_PROCESS_AUDIT_INFO;




/************************************************************

                 WARNING WARNING WARNING


    Only add new fields to the end of this structure.


*************************************************************/

// begin_ntifs

typedef struct _SE_EXPORTS {

    //
    // Privilege values
    //

    LUID    SeCreateTokenPrivilege;
    LUID    SeAssignPrimaryTokenPrivilege;
    LUID    SeLockMemoryPrivilege;
    LUID    SeIncreaseQuotaPrivilege;
    LUID    SeUnsolicitedInputPrivilege;
    LUID    SeTcbPrivilege;
    LUID    SeSecurityPrivilege;
    LUID    SeTakeOwnershipPrivilege;
    LUID    SeLoadDriverPrivilege;
    LUID    SeCreatePagefilePrivilege;
    LUID    SeIncreaseBasePriorityPrivilege;
    LUID    SeSystemProfilePrivilege;
    LUID    SeSystemtimePrivilege;
    LUID    SeProfileSingleProcessPrivilege;
    LUID    SeCreatePermanentPrivilege;
    LUID    SeBackupPrivilege;
    LUID    SeRestorePrivilege;
    LUID    SeShutdownPrivilege;
    LUID    SeDebugPrivilege;
    LUID    SeAuditPrivilege;
    LUID    SeSystemEnvironmentPrivilege;
    LUID    SeChangeNotifyPrivilege;
    LUID    SeRemoteShutdownPrivilege;


    //
    // Universally defined Sids
    //


    PSID  SeNullSid;
    PSID  SeWorldSid;
    PSID  SeLocalSid;
    PSID  SeCreatorOwnerSid;
    PSID  SeCreatorGroupSid;


    //
    // Nt defined Sids
    //


    PSID  SeNtAuthoritySid;
    PSID  SeDialupSid;
    PSID  SeNetworkSid;
    PSID  SeBatchSid;
    PSID  SeInteractiveSid;
    PSID  SeLocalSystemSid;
    PSID  SeAliasAdminsSid;
    PSID  SeAliasUsersSid;
    PSID  SeAliasGuestsSid;
    PSID  SeAliasPowerUsersSid;
    PSID  SeAliasAccountOpsSid;
    PSID  SeAliasSystemOpsSid;
    PSID  SeAliasPrintOpsSid;
    PSID  SeAliasBackupOpsSid;

    //
    // New Sids defined for NT5
    //

    PSID  SeAuthenticatedUsersSid;

    PSID  SeRestrictedSid;
    PSID  SeAnonymousLogonSid;

    //
    // New Privileges defined for NT5
    //

    LUID  SeUndockPrivilege;
    LUID  SeSyncAgentPrivilege;
    LUID  SeEnableDelegationPrivilege;

} SE_EXPORTS, *PSE_EXPORTS;

// end_ntifs

/************************************************************


                 WARNING WARNING WARNING


    Only add new fields to the end of this structure.


*************************************************************/



// begin_ntifs
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//              Logon session notification callback routines                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
//  These callback routines are used to notify file systems that have
//  registered of logon sessions being terminated, so they can cleanup state
//  associated with this logon session
//

typedef NTSTATUS
(*PSE_LOGON_SESSION_TERMINATED_ROUTINE)(
    IN PLUID LogonId);

// end_ntifs





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                  Exported Security Macro Definitions                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//++
//
//  ACCESS_MASK
//  SeComputeDeniedAccesses(
//      IN ACCESS_MASK GrantedAccess,
//      IN ACCESS_MASK DesiredAccess
//      );
//
//  Routine Description:
//
//      This routine generates an access mask containing those accesses
//      requested by DesiredAccess that aren't granted by GrantedAccess.
//      The result of this routine may be compared to 0 to determine
//      if a DesiredAccess mask contains any accesses that have not
//      been granted.
//
//      If the result IS ZERO, then all desired accesses have been granted.
//
//  Arguments:
//
//      GrantedAccess - Specifies the granted access mask.
//
//      DesiredAccess - Specifies the desired access mask.
//
//  Return Value:
//
//      An ACCESS_MASK containing the desired accesses that have
//      not been granted.
//
//--

#define SeComputeDeniedAccesses( GrantedAccess, DesiredAccess ) \
    ((~(GrantedAccess)) & (DesiredAccess) )


//++
//
//  BOOLEAN
//  SeComputeGrantedAccesses(
//      IN ACCESS_MASK GrantedAccess,
//      IN ACCESS_MASK DesiredAccess
//      );
//
//  Routine Description:
//
//      This routine generates an access mask containing acccesses
//      requested by DesiredAccess that are granted by GrantedAccess.
//      The result of this routine may be compared to 0 to determine
//      if any desired accesses have been granted.
//
//      If the result IS NON-ZERO, then at least one desired accesses
//      has been granted.
//
//  Arguments:
//
//      GrantedAccess - Specifies the granted access mask.
//
//      DesiredAccess - Specifies the desired access mask.
//
//  Return Value:
//
//      This routine returns TRUE if the DesiredAccess mask does specifies
//      any bits that are set in the GrantedAccess mask.
//
//--

#define SeComputeGrantedAccesses( GrantedAccess, DesiredAccess ) \
    ((GrantedAccess) & (DesiredAccess) )


//++
//
//  ULONG
//  SeLengthSid(
//      IN PSID Sid
//      );
//
//  Routine Description:
//
//      This routine computes the length of a SID.
//
//  Arguments:
//
//      Sid - Points to the SID whose length is to be returned.
//
//  Return Value:
//
//      The length, in bytes of the SID.
//
//--

#define SeLengthSid( Sid ) \
    (8 + (4 * ((SID *)Sid)->SubAuthorityCount))


//++
//  BOOLEAN
//  SeSameToken (
//      IN PTOKEN_CONTROL TokenControl1,
//      IN PTOKEN_CONTROL TokenControl2
//      )
//
//
//  Routine Description:
//
//      This routine returns a boolean value indicating whether the two
//      token control values represent the same token.  The token may
//      have changed over time, but must have the same authentication ID
//      and token ID.  A value of TRUE indicates they
//      are equal.  A value of FALSE indicates they are not equal.
//
//
//
//  Arguments:
//
//      TokenControl1 - Points to a token control to compare.
//
//      TokenControl2 - Points to the other token control to compare.
//
//  Return Value:
//
//      TRUE => The token control values represent the same token.
//
//      FALSE => The token control values do not represent the same token.
//
//
//--

#define SeSameToken(TC1,TC2)  (                                               \
        ((TC1)->TokenId.HighPart == (TC2)->TokenId.HighPart)               && \
        ((TC1)->TokenId.LowPart  == (TC2)->TokenId.LowPart)                && \
        (RtlEqualLuid(&(TC1)->AuthenticationId,&(TC2)->AuthenticationId))     \
        )


// begin_ntifs
//
//VOID
//SeDeleteClientSecurity(
//    IN PSECURITY_CLIENT_CONTEXT ClientContext
//    )
//
///*++
//
//Routine Description:
//
//    This service deletes a client security context block,
//    performing whatever cleanup might be necessary to do so.  In
//    particular, reference to any client token is removed.
//
//Arguments:
//
//    ClientContext - Points to the client security context block to be
//        deleted.
//
//
//Return Value:
//
//
//
//--*/
//--

#define SeDeleteClientSecurity(C)  {                                           \
            if (SeTokenType((C)->ClientToken) == TokenPrimary) {               \
                PsDereferencePrimaryToken( (C)->ClientToken );                 \
            } else {                                                           \
                PsDereferenceImpersonationToken( (C)->ClientToken );           \
            }                                                                  \
        }

// end_ntifs

//++
//VOID
//SeStopImpersonatingClient()
//
///*++
//
//Routine Description:
//
//    This service is used to stop impersonating a client using an
//    impersonation token.  This service must be called in the context
//    of the server thread which wishes to stop impersonating its
//    client.
//
//
//Arguments:
//
//    None.
//
//Return Value:
//
//    None.
//
//--*/
//--

#define SeStopImpersonatingClient() PsRevertToSelf()


#define SeAssertMappedCanonicalAccess( AccessMask )                  \
    ASSERT(!( ( AccessMask ) &                                       \
            ( GENERIC_READ        |                                  \
              GENERIC_WRITE       |                                  \
              GENERIC_EXECUTE     |                                  \
              GENERIC_ALL ))                                         \
          )
/*++

Routine Description:

    This routine asserts that the given AccessMask does not contain
    any generic access types.

Arguments:

    AccessMask - The access mask to be checked.

Return Value:

    None, or doesn't return.

--*/



#define SeComputeSecurityQuota( Size )                                 \
    (                                                                  \
       ((( Size ) * 2 )  > SE_DEFAULT_SECURITY_QUOTA) ?                \
                    (( Size ) * 2 ) : SE_DEFAULT_SECURITY_QUOTA        \
    )

/*++

Routine Description:

    This macro computes the amount of quota to charge for
    security information.

    The current algorithm is to use the larger of twice the size
    of the Group + Dacl information being applied and the default as
    specified by SE_DEFAULT_SECURITY_QUOTA.

Arguments:

    Size - The size in bytes of the Group + Dacl information being applied
        to the object.

Return Value:

    The size in bytes to charge for security information on this object.

--*/

// begin_ntifs

//++
//
//  PACCESS_TOKEN
//  SeQuerySubjectContextToken(
//      IN PSECURITY_SUBJECT_CONTEXT SubjectContext
//      );
//
//  Routine Description:
//
//      This routine returns the effective token from the subject context,
//      either the client token, if present, or the process token.
//
//  Arguments:
//
//      SubjectContext - Context to query
//
//  Return Value:
//
//      This routine returns the PACCESS_TOKEN for the effective token.
//      The pointer may be passed to SeQueryInformationToken.  This routine
//      does not affect the lock status of the token, i.e. the token is not
//      locked.  If the SubjectContext has been locked, the token remains locked,
//      if not, the token remains unlocked.
//
//--

#define SeQuerySubjectContextToken( SubjectContext ) \
        ( ARGUMENT_PRESENT( ((PSECURITY_SUBJECT_CONTEXT) SubjectContext)->ClientToken) ? \
            ((PSECURITY_SUBJECT_CONTEXT) SubjectContext)->ClientToken : \
            ((PSECURITY_SUBJECT_CONTEXT) SubjectContext)->PrimaryToken )

// end_ntifs





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//  Define the exported procedures that are callable only from kernel mode   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOLEAN
SeInitSystem( VOID );

VOID
SeSetSecurityAccessMask(
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PACCESS_MASK DesiredAccess
    );

VOID
SeQuerySecurityAccessMask(
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PACCESS_MASK DesiredAccess
    );


NTSTATUS
SeDefaultObjectMethod (
    IN PVOID Object,
    IN SECURITY_OPERATION_CODE OperationCode,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG Length,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN POOL_TYPE PoolType,
    IN PGENERIC_MAPPING GenericMapping
    );


NTKERNELAPI
NTSTATUS
SeCaptureSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    IN KPROCESSOR_MODE RequestorMode,
    IN POOL_TYPE PoolType,
    IN BOOLEAN ForceCapture,
    OUT PSECURITY_DESCRIPTOR *OutputSecurityDescriptor
    );

NTKERNELAPI
VOID
SeReleaseSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR CapturedSecurityDescriptor,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN ForceCapture
    );

// begin_ntifs

NTKERNELAPI
VOID
SeCaptureSubjectContext (
    OUT PSECURITY_SUBJECT_CONTEXT SubjectContext
    );

NTKERNELAPI
VOID
SeLockSubjectContext(
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext
    );

NTKERNELAPI
VOID
SeUnlockSubjectContext(
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext
    );

NTKERNELAPI
VOID
SeReleaseSubjectContext (
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext
    );

// end_ntifs

NTSTATUS
SeCaptureSecurityQos (
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN KPROCESSOR_MODE RequestorMode,
    IN PBOOLEAN SecurityQosPresent,
    IN PSECURITY_ADVANCED_QUALITY_OF_SERVICE CapturedSecurityQos
    );

VOID
SeFreeCapturedSecurityQos(
    IN PVOID SecurityQos
    );

NTSTATUS
SeCaptureSid (
    IN PSID InputSid,
    IN KPROCESSOR_MODE RequestorMode,
    IN PVOID CaptureBuffer OPTIONAL,
    IN ULONG CaptureBufferLength,
    IN POOL_TYPE PoolType,
    IN BOOLEAN ForceCapture,
    OUT PSID *CapturedSid
    );


VOID
SeReleaseSid (
    IN PSID CapturedSid,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN ForceCapture
    );


NTSTATUS
SeCaptureAcl (
    IN PACL InputAcl,
    IN KPROCESSOR_MODE RequestorMode,
    IN PVOID CaptureBuffer OPTIONAL,
    IN ULONG CaptureBufferLength,
    IN POOL_TYPE PoolType,
    IN BOOLEAN ForceCapture,
    OUT PACL *CapturedAcl,
    OUT PULONG AlignedAclSize
    );


VOID
SeReleaseAcl (
    IN PACL CapturedAcl,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN ForceCapture
    );


NTSTATUS
SeCaptureLuidAndAttributesArray (
    IN PLUID_AND_ATTRIBUTES InputArray,
    IN ULONG ArrayCount,
    IN KPROCESSOR_MODE RequestorMode,
    IN PVOID CaptureBuffer OPTIONAL,
    IN ULONG CaptureBufferLength,
    IN POOL_TYPE PoolType,
    IN BOOLEAN ForceCapture,
    OUT PLUID_AND_ATTRIBUTES *CapturedArray,
    OUT PULONG AlignedArraySize
    );



VOID
SeReleaseLuidAndAttributesArray (
    IN PLUID_AND_ATTRIBUTES CapturedArray,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN ForceCapture
    );



NTSTATUS
SeCaptureSidAndAttributesArray (
    IN PSID_AND_ATTRIBUTES InputArray,
    IN ULONG ArrayCount,
    IN KPROCESSOR_MODE RequestorMode,
    IN PVOID CaptureBuffer OPTIONAL,
    IN ULONG CaptureBufferLength,
    IN POOL_TYPE PoolType,
    IN BOOLEAN ForceCapture,
    OUT PSID_AND_ATTRIBUTES *CapturedArray,
    OUT PULONG AlignedArraySize
    );


VOID
SeReleaseSidAndAttributesArray (
    IN PSID_AND_ATTRIBUTES CapturedArray,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN ForceCapture
    );

// begin_ntddk begin_ntifs

NTKERNELAPI
NTSTATUS
SeAssignSecurity (
    IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    IN PSECURITY_DESCRIPTOR ExplicitDescriptor,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor,
    IN BOOLEAN IsDirectoryObject,
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
    IN PGENERIC_MAPPING GenericMapping,
    IN POOL_TYPE PoolType
    );

NTKERNELAPI
NTSTATUS
SeAssignSecurityEx (
    IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    IN PSECURITY_DESCRIPTOR ExplicitDescriptor OPTIONAL,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor,
    IN GUID *ObjectType OPTIONAL,
    IN BOOLEAN IsDirectoryObject,
    IN ULONG AutoInheritFlags,
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
    IN PGENERIC_MAPPING GenericMapping,
    IN POOL_TYPE PoolType
    );

NTKERNELAPI
NTSTATUS
SeDeassignSecurity (
    IN OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    );

NTKERNELAPI
BOOLEAN
SeAccessCheck (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN BOOLEAN SubjectContextLocked,
    IN ACCESS_MASK DesiredAccess,
    IN ACCESS_MASK PreviouslyGrantedAccess,
    OUT PPRIVILEGE_SET *Privileges OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping,
    IN KPROCESSOR_MODE AccessMode,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
    );

// end_ntddk end_ntifs

BOOLEAN
SeProxyAccessCheck (
    IN PUNICODE_STRING Volume,
    IN PUNICODE_STRING RelativePath,
    IN BOOLEAN ContainerObject,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN BOOLEAN SubjectContextLocked,
    IN ACCESS_MASK DesiredAccess,
    IN ACCESS_MASK PreviouslyGrantedAccess,
    OUT PPRIVILEGE_SET *Privileges OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping,
    IN KPROCESSOR_MODE AccessMode,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
    );

// begin_ntifs

NTKERNELAPI
BOOLEAN
SePrivilegeCheck(
    IN OUT PPRIVILEGE_SET RequiredPrivileges,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN KPROCESSOR_MODE AccessMode
    );

NTKERNELAPI
VOID
SeFreePrivileges(
    IN PPRIVILEGE_SET Privileges
    );

// end_ntifs

NTSTATUS
SePrivilegePolicyCheck(
    IN OUT PACCESS_MASK RemainingDesiredAccess,
    IN OUT PACCESS_MASK PreviouslyGrantedAccess,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext OPTIONAL,
    IN PACCESS_TOKEN Token OPTIONAL,
    OUT PPRIVILEGE_SET *PrivilegeSet,
    IN KPROCESSOR_MODE PreviousMode
    );

VOID
SeGenerateMessage (
    IN PSTRING ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PACCESS_TOKEN Token,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN AccessGranted,
    IN HANDLE AuditPort,
    IN HANDLE AlarmPort,
    IN KPROCESSOR_MODE AccessMode
    );

// begin_ntifs

NTKERNELAPI
VOID
SeOpenObjectAuditAlarm (
    IN PUNICODE_STRING ObjectTypeName,
    IN PVOID Object OPTIONAL,
    IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PACCESS_STATE AccessState,
    IN BOOLEAN ObjectCreated,
    IN BOOLEAN AccessGranted,
    IN KPROCESSOR_MODE AccessMode,
    OUT PBOOLEAN GenerateOnClose
    );

NTKERNELAPI
VOID
SeOpenObjectForDeleteAuditAlarm (
    IN PUNICODE_STRING ObjectTypeName,
    IN PVOID Object OPTIONAL,
    IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PACCESS_STATE AccessState,
    IN BOOLEAN ObjectCreated,
    IN BOOLEAN AccessGranted,
    IN KPROCESSOR_MODE AccessMode,
    OUT PBOOLEAN GenerateOnClose
    );

VOID
SeDeleteObjectAuditAlarm(
    IN PVOID Object,
    IN HANDLE Handle
    );


// end_ntifs

VOID
SeCloseObjectAuditAlarm(
    IN PVOID Object,
    IN HANDLE Handle,
    IN BOOLEAN GenerateOnClose
    );

VOID
SeTraverseAuditAlarm(
    IN PLUID OperationID,
    IN PVOID DirectoryObject,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN BOOLEAN SubjectContextLocked,
    IN ACCESS_MASK TraverseAccess,
    IN PPRIVILEGE_SET Privileges OPTIONAL,
    IN BOOLEAN AccessGranted,
    IN KPROCESSOR_MODE AccessMode
    );

VOID
SeCreateInstanceAuditAlarm(
    IN PLUID OperationID OPTIONAL,
    IN PVOID Object,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN ACCESS_MASK DesiredAccess,
    IN PPRIVILEGE_SET Privileges OPTIONAL,
    IN BOOLEAN AccessGranted,
    IN KPROCESSOR_MODE AccessMode
    );

VOID
SeCreateObjectAuditAlarm(
    IN PLUID OperationID OPTIONAL,
    IN PVOID Object,
    IN PUNICODE_STRING ComponentName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN ACCESS_MASK DesiredAccess,
    IN PPRIVILEGE_SET Privileges OPTIONAL,
    IN BOOLEAN AccessGranted,
    OUT PBOOLEAN AuditPerformed,
    IN KPROCESSOR_MODE AccessMode
    );

VOID
SeObjectReferenceAuditAlarm(
    IN PLUID OperationID OPTIONAL,
    IN PVOID Object,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN ACCESS_MASK DesiredAccess,
    IN PPRIVILEGE_SET Privileges OPTIONAL,
    IN BOOLEAN AccessGranted,
    IN KPROCESSOR_MODE AccessMode
    );

NTKERNELAPI
VOID
SePrivilegeObjectAuditAlarm(
    IN HANDLE Handle,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN ACCESS_MASK DesiredAccess,
    IN PPRIVILEGE_SET Privileges,
    IN BOOLEAN AccessGranted,
    IN KPROCESSOR_MODE AccessMode
    );

BOOLEAN
SeCheckPrivilegedObject(
    LUID PrivilegeValue,
    HANDLE ObjectHandle,
    ACCESS_MASK DesiredAccess,
    KPROCESSOR_MODE PreviousMode
    );

// begin_ntddk begin_ntifs

NTKERNELAPI
BOOLEAN
SeValidSecurityDescriptor(
    IN ULONG Length,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    );

// end_ntddk end_ntifs



//VOID
//SeImplicitObjectAuditAlarm(
//    IN PLUID OperationID OPTIONAL,
//    IN PVOID Object,
//    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
//    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
//    IN ACCESS_MASK DesiredAccess,
//    IN PPRIVILEGE_SET Privileges OPTIONAL,
//    IN BOOLEAN AccessGranted,
//    IN KPROCESSOR_MODE AccessMode
//    );
//

VOID
SeAuditHandleCreation(
    IN PACCESS_STATE AccessState,
    IN HANDLE Handle
    );



PACCESS_TOKEN
SeMakeSystemToken ();

PACCESS_TOKEN
SeMakeAnonymousLogonToken ();

VOID
SeGetTokenControlInformation (
    IN PACCESS_TOKEN Token,
    OUT PTOKEN_CONTROL TokenControl
    );

//++
//
// PVOID
// SeTokenObjectType()
//
// Routine Description:
//
//    This function returns a pointer to the Token object type structure.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Pointer to the token object type structure.
//
//--

extern struct _OBJECT_TYPE *SepTokenObjectType;
#define SeTokenObjectType() (PVOID)SepTokenObjectType

NTKERNELAPI                                     // ntifs
TOKEN_TYPE                                      // ntifs
SeTokenType(                                    // ntifs
    IN PACCESS_TOKEN Token                      // ntifs
    );                                          // ntifs

SECURITY_IMPERSONATION_LEVEL
SeTokenImpersonationLevel(
    IN PACCESS_TOKEN Token
    );

NTKERNELAPI                                     // ntifs
BOOLEAN                                         // ntifs
SeTokenIsAdmin(                                 // ntifs
    IN PACCESS_TOKEN Token                      // ntifs
    );                                          // ntifs


NTKERNELAPI                                     // ntifs
BOOLEAN                                         // ntifs
SeTokenIsRestricted(                            // ntifs
    IN PACCESS_TOKEN Token                      // ntifs
    );                                          // ntifs

NTSTATUS
SeSubProcessToken (
    IN PEPROCESS ParentProcess,
    OUT PACCESS_TOKEN *ChildToken
    );

VOID
SeAssignPrimaryToken(
    IN PEPROCESS Process,
    IN PACCESS_TOKEN Token
    );

VOID
SeDeassignPrimaryToken(
    IN PEPROCESS Process
    );

NTSTATUS
SeExchangePrimaryToken(
    IN PEPROCESS Process,
    IN PACCESS_TOKEN NewAccessToken,
    OUT PACCESS_TOKEN *OldAccessToken
    );

NTSTATUS
SeCopyClientToken(
    IN PACCESS_TOKEN ClientToken,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    IN KPROCESSOR_MODE RequestorMode,
    OUT PACCESS_TOKEN *DuplicateToken
    );

NTSTATUS
SeFilterToken (
    IN PACCESS_TOKEN ExistingToken,
    IN ULONG Flags,
    IN PTOKEN_GROUPS SidsToDisable OPTIONAL,
    IN PTOKEN_PRIVILEGES PrivilegesToDelete OPTIONAL,
    IN PTOKEN_GROUPS RestrictedSids OPTIONAL,
    OUT PACCESS_TOKEN * FilteredToken
    );


// begin_ntifs

NTKERNELAPI
NTSTATUS
SeQueryAuthenticationIdToken(
    IN PACCESS_TOKEN Token,
    OUT PLUID AuthenticationId
    );

NTKERNELAPI
NTSTATUS
SeQuerySessionIdToken(
    IN PACCESS_TOKEN,
    IN PULONG pSessionId
    );

NTKERNELAPI
NTSTATUS
SeSetSessionIdToken(
    IN PACCESS_TOKEN,
    IN ULONG SessionId
    );

NTKERNELAPI
NTSTATUS
SeCreateClientSecurity (
    IN PETHREAD ClientThread,
    IN PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
    IN BOOLEAN RemoteSession,
    OUT PSECURITY_CLIENT_CONTEXT ClientContext
    );

NTKERNELAPI
VOID
SeImpersonateClient(
    IN PSECURITY_CLIENT_CONTEXT ClientContext,
    IN PETHREAD ServerThread OPTIONAL
    );

NTKERNELAPI
NTSTATUS
SeImpersonateClientEx(
    IN PSECURITY_CLIENT_CONTEXT ClientContext,
    IN PETHREAD ServerThread OPTIONAL
    );

NTKERNELAPI
NTSTATUS
SeCreateClientSecurityFromSubjectContext (
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
    IN PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
    IN BOOLEAN ServerIsRemote,
    OUT PSECURITY_CLIENT_CONTEXT ClientContext
    );

// end_ntifs

//
// Do not export the following routines to drivers.
// If you need to do so, create a new routine that
// does not take the AuxData parameter and export
// that.
//

NTKERNELAPI
NTSTATUS
SeCreateAccessState(
   IN PACCESS_STATE AccessState,
   IN PAUX_ACCESS_DATA AuxData,
   IN ACCESS_MASK DesiredAccess,
   IN PGENERIC_MAPPING GenericMapping
   );

NTKERNELAPI
VOID
SeDeleteAccessState(
    IN PACCESS_STATE AccessState
    );

NTSTATUS
SeUpdateClientSecurity(
    IN PETHREAD ClientThread,
    IN OUT PSECURITY_CLIENT_CONTEXT ClientContext,
    OUT PBOOLEAN ChangesMade,
    OUT PBOOLEAN NewToken
    );

BOOLEAN
SeRmInitPhase1(
    );

// begin_ntifs

NTKERNELAPI
NTSTATUS
SeQuerySecurityDescriptorInfo (
    IN PSECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG Length,
    IN PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor
    );

NTKERNELAPI
NTSTATUS
SeSetSecurityDescriptorInfo (
    IN PVOID Object OPTIONAL,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN POOL_TYPE PoolType,
    IN PGENERIC_MAPPING GenericMapping
    );

NTKERNELAPI
NTSTATUS
SeSetSecurityDescriptorInfoEx (
    IN PVOID Object OPTIONAL,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR ModificationDescriptor,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN ULONG AutoInheritFlags,
    IN POOL_TYPE PoolType,
    IN PGENERIC_MAPPING GenericMapping
    );

NTKERNELAPI
NTSTATUS
SeAppendPrivileges(
    PACCESS_STATE AccessState,
    PPRIVILEGE_SET Privileges
    );

// end_ntifs

NTSTATUS
SeComputeQuotaInformationSize(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PULONG Size
    );

VOID
SePrivilegedServiceAuditAlarm (
    IN PUNICODE_STRING ServiceName,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN PPRIVILEGE_SET Privileges,
    IN BOOLEAN AccessGranted
    );

NTKERNELAPI                                                     // ntddk ntifs
BOOLEAN                                                         // ntddk ntifs
SeSinglePrivilegeCheck(                                         // ntddk ntifs
    LUID PrivilegeValue,                                        // ntddk ntifs
    KPROCESSOR_MODE PreviousMode                                // ntddk ntifs
    );                                                          // ntddk ntifs

BOOLEAN
SeCheckAuditPrivilege (
   IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
   IN KPROCESSOR_MODE PreviousMode
   );

NTSTATUS
SeAssignWorldSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG Length,
    IN PSECURITY_INFORMATION SecurityInformation
    );

BOOLEAN
SeFastTraverseCheck(
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    ACCESS_MASK TraverseAccess,
    KPROCESSOR_MODE AccessMode
    );

NTKERNELAPI                                                     // ntifs
BOOLEAN                                                         // ntifs
SeAuditingFileEvents(                                           // ntifs
    IN BOOLEAN AccessGranted,                                   // ntifs
    IN PSECURITY_DESCRIPTOR SecurityDescriptor                  // ntifs
    );                                                          // ntifs

NTKERNELAPI                                                     // ntifs
BOOLEAN                                                         // ntifs
SeAuditingFileOrGlobalEvents(                                   // ntifs
    IN BOOLEAN AccessGranted,                                   // ntifs
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,                 // ntifs
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext         // ntifs
    );                                                          // ntifs

VOID
SeAuditProcessCreation(
    PEPROCESS Process,
    PEPROCESS Parent,
    PUNICODE_STRING ImageFileName
    );

VOID
SeAuditProcessExit(
    PEPROCESS Process
    );

VOID
SeAuditHandleDuplication(
    PVOID SourceHandle,
    PVOID NewHandle,
    PEPROCESS SourceProcess,
    PEPROCESS TargetProcess
    );

// begin_ntifs

VOID
SeSetAccessStateGenericMapping (
    PACCESS_STATE AccessState,
    PGENERIC_MAPPING GenericMapping
    );

// end_ntifs

// begin_ntifs

NTKERNELAPI
NTSTATUS
SeRegisterLogonSessionTerminatedRoutine(
    IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine
    );

NTKERNELAPI
NTSTATUS
SeUnregisterLogonSessionTerminatedRoutine(
    IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine
    );

NTKERNELAPI
NTSTATUS
SeMarkLogonSessionForTerminationNotification(
    IN PLUID LogonId
    );

NTKERNELAPI
NTSTATUS
SeQueryInformationToken (
    IN PACCESS_TOKEN Token,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID *TokenInformation
    );


// end_ntifs

NTSTATUS
SeIsChildToken(
    IN HANDLE Token,
    OUT PBOOLEAN IsChild
    );

NTSTATUS
SeIsChildTokenByPointer(
    IN PACCESS_TOKEN Token,
    OUT PBOOLEAN IsChild
    );


NTSTATUS
SeFastFilterToken(
    IN PACCESS_TOKEN ExistingToken,
    IN KPROCESSOR_MODE RequestorMode,
    IN ULONG Flags,
    IN ULONG GroupCount,
    IN PSID_AND_ATTRIBUTES GroupsToDisable OPTIONAL,
    IN ULONG PrivilegeCount,
    IN PLUID_AND_ATTRIBUTES PrivilegesToDelete OPTIONAL,
    IN ULONG SidCount,
    IN PSID_AND_ATTRIBUTES RestrictedSids OPTIONAL,
    IN ULONG SidLength,
    OUT PACCESS_TOKEN * FilteredToken
    );

////////////////////////////////////////////////////////////////////////
//                                                                    //
//           Global, READ ONLY, Security variables                    //
//                                                                    //
////////////////////////////////////////////////////////////////////////


// begin_ntifs
// **************************************************************
//
//              C A V E A T      P R O G R A M M E R
//
//
//  If you wish to include this file in an NT driver and use SeExports structure
//  defined above, you need to call:
//
//
//      SeEnableAccessToExports()
//
//  exactly once during initialization.
//
//              C A V E A T      P R O G R A M M E R
//
// **************************************************************
#if 0
#define SeEnableAccessToExports() SeExports = *(PSE_EXPORTS *)SeExports;
extern PSE_EXPORTS SeExports;                       // ntifs
#else
#define SeEnableAccessToExports()
extern NTKERNELAPI PSE_EXPORTS SeExports;
#endif

// end_ntifs

//
// Value used to represent the authentication ID of system processes
//

extern LUID SeSystemAuthenticationId;
extern LUID SeAnonymousAuthenticationId;

extern TOKEN_SOURCE SeSystemTokenSource;

//
// Universal well known SIDs
//

extern PSID  SeNullSid;
extern PSID  SeWorldSid;
extern PSID  SeLocalSid;
extern PSID  SeCreatorOwnerSid;
extern PSID  SeCreatorGroupSid;
extern PSID  SeCreatorOwnerServerSid;
extern PSID  SeCreatorGroupServerSid;
extern PSID  SePrincipalSelfSid;


//
// Sids defined by NT
//

extern PSID SeNtAuthoritySid;

extern PSID SeDialupSid;
extern PSID SeNetworkSid;
extern PSID SeBatchSid;
extern PSID SeInteractiveSid;
extern PSID SeLocalSystemSid;
extern PSID SeAuthenticatedUsersSid;
extern PSID SeAliasAdminsSid;
extern PSID SeRestrictedSid;
extern PSID SeAnonymousLogonSid;
extern PSID SeAliasUsersSid;
extern PSID SeAliasGuestsSid;
extern PSID SeAliasPowerUsersSid;
extern PSID SeAliasAccountOpsSid;
extern PSID SeAliasSystemOpsSid;
extern PSID SeAliasPrintOpsSid;
extern PSID SeAliasBackupOpsSid;

//
// Well known tokens
//

extern PACCESS_TOKEN SeAnonymousLogonToken;

//
// System default DACLs & Security Descriptors
//

extern PSECURITY_DESCRIPTOR SePublicDefaultSd;
extern PSECURITY_DESCRIPTOR SePublicDefaultUnrestrictedSd;
extern PSECURITY_DESCRIPTOR SePublicOpenSd;
extern PSECURITY_DESCRIPTOR SePublicOpenUnrestrictedSd;
extern PSECURITY_DESCRIPTOR SeSystemDefaultSd;
extern PSECURITY_DESCRIPTOR SeUnrestrictedSd;

extern PACL SePublicDefaultDacl;
extern PACL SePublicDefaultUnrestrictedDacl;
extern PACL SePublicOpenDacl;
extern PACL SePublicOpenUnrestrictedDacl;
extern PACL SeSystemDefaultDacl;
extern PACL SeUnrestrictedDacl;

//
//  Well known privilege values
//


extern LUID SeCreateTokenPrivilege;
extern LUID SeAssignPrimaryTokenPrivilege;
extern LUID SeLockMemoryPrivilege;
extern LUID SeIncreaseQuotaPrivilege;
extern LUID SeUnsolicitedInputPrivilege;
extern LUID SeTcbPrivilege;
extern LUID SeSecurityPrivilege;
extern LUID SeTakeOwnershipPrivilege;
extern LUID SeLoadDriverPrivilege;
extern LUID SeCreatePagefilePrivilege;
extern LUID SeIncreaseBasePriorityPrivilege;
extern LUID SeSystemProfilePrivilege;
extern LUID SeSystemtimePrivilege;
extern LUID SeProfileSingleProcessPrivilege;
extern LUID SeCreatePermanentPrivilege;
extern LUID SeBackupPrivilege;
extern LUID SeRestorePrivilege;
extern LUID SeShutdownPrivilege;
extern LUID SeDebugPrivilege;
extern LUID SeAuditPrivilege;
extern LUID SeSystemEnvironmentPrivilege;
extern LUID SeChangeNotifyPrivilege;
extern LUID SeRemoteShutdownPrivilege;
extern LUID SeUndockPrivilege;
extern LUID SeSyncAgentPrivilege;
extern LUID SeEnableDelegationPrivilege;


//
// Auditing information array
//

extern SE_AUDITING_STATE SeAuditingState[];

//
// Flag so that other components may quickly check for
// auditing.
//

extern BOOLEAN SeDetailedAuditing;

extern UNICODE_STRING SeSubsystemName;


#endif // _SE_
