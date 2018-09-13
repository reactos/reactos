/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tokenp.h

Abstract:

    This module contains the internal (private) declarations needed by the
    TOKEN object routines.

    It also contains global variables needed by the TOKEN object routines.

Author:

    Jim Kelly (JimK) 18-May-1990

Revision History:

   v10: robertre
        Added SepAccessCheck and SepPrivilegeCheck prototypes
   v11: robertre
        Added parameter to SepAccessCheck

--*/

#ifndef _TOKENP_
#define _TOKENP_

//#define TOKEN_DEBUG

#include "ntos.h"
#include "sep.h"
#include "seopaque.h"



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                Token Diagnostics                                    //
//                                                                     //
/////////////////////////////////////////////////////////////////////////



#if DBG
#define TOKEN_DIAGNOSTICS_ENABLED 1
#endif // DBG


//
// These definitions are useful diagnostics aids
//

#if TOKEN_DIAGNOSTICS_ENABLED

//
// Test for enabled diagnostic
//

#define IF_TOKEN_GLOBAL( FlagName ) \
    if (TokenGlobalFlag & (TOKEN_DIAG_##FlagName))

//
// Diagnostics print statement
//

#define TokenDiagPrint( FlagName, _Text_ )                               \
    IF_TOKEN_GLOBAL( FlagName )                                          \
        DbgPrint _Text_



#else  // !TOKEN_DIAGNOSTICS_ENABLED

//
// No diagnostics included in build
//


//
// Test for diagnostics enabled
//

#define IF_TOKEN_GLOBAL( FlagName ) if (FALSE)

//
// Diagnostics print statement (expands to no-op)
//

#define TokenDiagPrint( FlagName, _Text_ )     ;

#endif // TOKEN_DIAGNOSTICS_ENABLED


//
// The following flags enable or disable various diagnostic
// capabilities within token code.  These flags are set in
// TokenGlobalFlag (only available within a DBG system).
//
//
//      TOKEN_LOCKS - Display information about acquisition and freeing
//          of token locks.
//

#define TOKEN_DIAG_TOKEN_LOCKS          ((ULONG) 0x00000001L)


/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                Token Related Constants                              //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

//
// By default, a token is charged the following for its dynamic component.
// The dynamic component houses the default ACL and primary group ID.
// If the size of these parameters passed upon token creation are larger
// than this default, then the larger value will be charged.
//

#define TOKEN_DEFAULT_DYNAMIC_CHARGE 500



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                Token Object Body                                    //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

//
// Tokens have three parts:
//
//               Fixed part of body,
//               Variable part of body,
//               Dynamic part (not in body).
//
// The fixed and variable parts are allocated in a single block of memory.
// The dynamic part is a separately allocated block of memory.
//
// The fixed part of the body contains the fixed length fields.  These
// are defined in the TOKEN data type.
//
// The variable part of the body is variable in length and contains
// privileges and user/group SIDs.  This part is variable in length
// between different token objects, but does not change once established
// for an individual token.
//
// The dynamic part is used to house default discretionary ACL information
// and the primary group ID.
//
// Pictorially, a token looks like:
//
//    ==============      +---------------+
//          ^             |               |
//          |             |               |
//          |             |               |
//          |             | DynamicPart   o-----------+
//          |             |- - - - - - - -|           |
//                  +-----o Privileges    |           |
//        Token     |     |- - - - - - - -|           |
//        Body      |  +--o UserAndGroups |           |
//          |       |  |  |- - - - - - - -|           |
//          |       |  +--o RestrictedSids|          \|/
//          |       |  |  |- - - - - - - -|        +---------------------+
//          |       |  |  | PrimaryGroup  o------->| [Primary Group SID] |
//          |       |  |  |- - - - - - - -|        |         o           |
//          |       |  |  | DefaultAcl    o---+    |         o           |
//          |       |  |  |- - - - - - - -|   |    |         o           |
//          |       |  |  |      o        |   |    |- - - - - - - - - - -|
//          |       |  |  |      o        |   +--->| [  Default Acl  ]   |
//          v       |  |  |      o        |        |         o           |
//    ==============|  |  |===============|        |         o           |
//          ^       |  +->| SIDs  Array   |        |         o           |
//          |       |     | [User  SID ]  |        +---------------------+
//          |       |     | [Group SID ]  |
//                  |     | [Group SID ]  |
//        Variable  |     | [Rest. Sid ]  |
//         Part     |     |     o         |
//                  |     |- - - - - - - -|
//          |       +---->| Privileges    |
//          |             | Array         |
//          v             |               |
//    ==============      +---------------+
//
//     WARNING: The positions of fields illustrated in this picture are not
//              intented to reflect their actual or even relative positions
//              within the real data structures.  The exception to this is
//              that THE USER SID IS THE FIRST SID IN THE UserAndGroups
//              ARRAY.
//


//
//             ! ! ! !     IMPORTANT     ! ! ! !
//
//      The access validation routines assume the SIDs are arranged
//      in a particular order within the variable part of the token.
//      Any changes to the order of the SIDs must be coordinated with
//      corresponding changes to the access validation routines.
//
//                   ! ! ! ! ! ! ! ! ! ! !



typedef struct _TOKEN {

    //
    // Fields arranged by size to preserve alignment.
    // Large fields before small fields.
    //


    //
    //  The following fields are either ReadOnly or ReadWrite.
    //  ReadOnly fields may be referenced any time a pointer to the
    //  token is still valid.  ReadWrite fields may only be referenced
    //  when the TokenLock is held.

    //  The dynamic part of the token (pointed to by the DynamicPart field)
    //  is also protected by the token lock.
    //
    //  ReadOnly  fields are marked Ro: in their comment.
    //  ReadWrite fields are marked Wr: in their comment.
    //

    TOKEN_SOURCE TokenSource;                           // Ro: 16-Bytes

    LUID TokenId;                                       // Ro: 8-Bytes
    LUID AuthenticationId;                              // Ro: 8-Bytes
    LUID ParentTokenId;                                 // Ro: 8-Bytes
    LARGE_INTEGER ExpirationTime;                       // Ro: 8-Bytes

    //
    // Each time the security information in a token is changed, the
    // following ID is changed.  Fields that cause this field to be
    // updated are marked as (Mod) in their comment field.
    //

    LUID ModifiedId;                                    // Wr: 8-Bytes

    ULONG SessionId;                                    // Wr: 4-bytes
    ULONG UserAndGroupCount;                            // Ro: 4-Bytes
    ULONG RestrictedSidCount;                           // Ro: 4-Bytes
    ULONG PrivilegeCount;                               // Ro: 4-Bytes
    ULONG VariableLength;                               // Ro: 4-Bytes
    ULONG DynamicCharged;                               // Ro: 4-Bytes

    ULONG DynamicAvailable;                             // Wr: 4-Bytes (Mod)
    ULONG DefaultOwnerIndex;                            // Wr: 4-Bytes (Mod)
    PSID_AND_ATTRIBUTES UserAndGroups;                  // Wr: 4-Bytes (Mod)
    PSID_AND_ATTRIBUTES RestrictedSids;                 // Ro: 4-Bytes
    PSID PrimaryGroup;                                  // Wr: 4-Bytes (Mod)
    PLUID_AND_ATTRIBUTES Privileges;                    // Wr: 4-Bytes (Mod)
    PULONG DynamicPart;                                 // Wr: 4-Bytes (Mod)
    PACL DefaultDacl;                                   // Wr: 4-Bytes (Mod)



    TOKEN_TYPE TokenType;                               // Ro: 1-Byte
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;    // Ro: 1-Byte

    UCHAR TokenFlags;                                   // Ro: 4-Bytes
    BOOLEAN TokenInUse;                                 // Wr: 1-Byte

    PSECURITY_TOKEN_PROXY_DATA ProxyData;               // Ro: 4-Bytes
    PSECURITY_TOKEN_AUDIT_DATA AuditData;               // Ro: 4-Bytes

    //
    // This marks the beginning of the variable part of the token.
    // It must follow all other fields in the token.
    //

    ULONG VariablePart;                                 // Wr: 4-Bytes (Mod)

    } TOKEN, * PTOKEN;

//
// Where:
//
//    TokenSource - Information provided by the executive component that
//                  requested the logon that the token represents.
//
//
//    TokenId - Is an LUID value.  Each token object has a uniquely
//        assigned LUID.
//
//
//    AuthenticationId - Is the LUID assigned by the domain controller for
//        the logon session.
//
//
//    ExpirationTime - Not yet supported in NT.
//
//
//    ModifiedId - Is an LUID which is changed each time a modification is
//        made to this token which changes the security semantics of the
//        token.  This includes enabling/disabling privileges and groups
//        and changing default ACLs, et cetera.  Any token which is a
//        duplicate of this token will have the same ModifiedId (until
//        one or the other is changed).  This does not cover changes to
//        non-security semantics fields, like TokenInUse.
//
//
//    UserAndGroupCount - Indicates the number of user/group IDs in this token.
//        This value must be at least 1.  A value of 1 indicates a user
//        ID with no supplemental group IDs.  A value of 5 indicates a
//        user ID and 4 supplemental group IDs.
//
//    PrivilegeCount - Indicates how many privileges are included in
//        this token.  May be zero or larger.
//
//    TokenType - Indicates which type of token this token object is.
//
//    ImpersonationLevel - For TokenImpersonation type tokens, this field
//        indicates the impersonation level.  For TokenPrimary type tokens,
//        this field is ignored.
//
//    DynamicCharged - Indicates how much pool quota has been charged
//        for the dynamic portion of this token.
//
//    DynamicAvailable - Indicates how much of the charged quota is still
//        available for use.  This is modified when  pool associated
//        with the dynamic portion of the token is allocated or freed,
//        such as when the default DACL or primary group is replaced.
//
//
//    DefaultOwnerIndex - If non-zero, identifies an ID that has explicitly
//        been established as the default owner for this token.  If it is zero,
//        the standard default (user ID) is used as the default owner.
//
//    UserAndGroups - Points to an array of SID_AND_ATTRIBUTES.  The first
//        element in this array is the token's user ID.  Any additional
//        elements are those of groups.  The number of entries in this
//        array is one greater than
//
//    PrimaryGroup - Points to an SID that is to be used as the primary
//        group of the token.  There are no value restrictions
//        placed upon what can be used as a primary group.  This
//        SID is not one of user or group IDs (although it may have the
//        same value as one of those IDs).
//
//    Privileges - Points to an array of privileges represented as
//        LUID_AND_ATTRIBUTES.  The number of elements in this array
//        is contained in the PrivilegesCount field.
//
//    TokenInUse - Is a boolean that indicates whether a primary token
//        is already in use by a process.  This field value is only
//        valid for primary tokens.
//
//    ProxyData - Optionally points to a Proxy data structure, containing
//        the information to be passed to AVR routines by file systems.
//        This field being non-null identifies the token as a proxy token.
//
//    AuditData - Optionally points to an Audit data structure, containing
//         global auditing data for this subject.
//
//        NOTE:  Access to this field is guarded by the global
//               PROCESS SECURITY FIELDS LOCK.
//    VariablePart - Is the beginning of the variable part of the token.
//


////////////////////////////////////////////////////////////////////////
//
// Internal version of Object Type list
//
////////////////////////////////////////////////////////////////////////

typedef struct _IOBJECT_TYPE_LIST {
    USHORT Level;
    USHORT Flags;
#define OBJECT_SUCCESS_AUDIT 0x1
#define OBJECT_FAILURE_AUDIT 0x2
    GUID ObjectType;
    LONG ParentIndex;
    ULONG Remaining;
    ULONG CurrentGranted;
    ULONG CurrentDenied;
} IOBJECT_TYPE_LIST, *PIOBJECT_TYPE_LIST;

NTSTATUS
SeCaptureObjectTypeList (
    IN POBJECT_TYPE_LIST ObjectTypeList OPTIONAL,
    IN ULONG ObjectTypeListLength,
    IN KPROCESSOR_MODE RequestorMode,
    OUT PIOBJECT_TYPE_LIST *CapturedObjectTypeList
    );

VOID
SeFreeCapturedObjectTypeList(
    IN PVOID ObjectTypeList
    );


/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                Token Specific Macros                                //
//                                                                     //
/////////////////////////////////////////////////////////////////////////







#ifndef TOKEN_DIAGNOSTICS_ENABLED

#define SepAcquireTokenReadLock(T)  KeEnterCriticalRegion();          \
                                    ExAcquireResourceShared(&SepTokenLock, TRUE)

#define SepAcquireTokenWriteLock(T) KeEnterCriticalRegion();          \
                                    ExAcquireResourceExclusive(&SepTokenLock, TRUE)

#define SepReleaseTokenReadLock(T)  ExReleaseResource(&SepTokenLock);  \
                                    KeLeaveCriticalRegion()

#else  // TOKEN_DIAGNOSTICS_ENABLED

#define SepAcquireTokenReadLock(T)  if (TokenGlobalFlag & TOKEN_DIAG_TOKEN_LOCKS) { \
                                        DbgPrint("SE (Token):  Acquiring Token READ Lock for access to token 0x%lx\n", (T)); \
                                    }                                 \
                                    KeEnterCriticalRegion();          \
                                    ExAcquireResourceShared(&SepTokenLock, TRUE)

#define SepAcquireTokenWriteLock(T) if (TokenGlobalFlag & TOKEN_DIAG_TOKEN_LOCKS) { \
                                        DbgPrint("SE (Token):  Acquiring Token WRITE Lock for access to token 0x%lx    ********************* EXCLUSIVE *****\n", (T)); \
                                    }                                 \
                                    KeEnterCriticalRegion();          \
                                    ExAcquireResourceExclusive(&SepTokenLock, TRUE)

#define SepReleaseTokenReadLock(T)  if (TokenGlobalFlag & TOKEN_DIAG_TOKEN_LOCKS) { \
                                        DbgPrint("SE (Token):  Releasing Token Lock for access to token 0x%lx\n", (T)); \
                                    }                                 \
                                    ExReleaseResource(&SepTokenLock); \
                                    KeLeaveCriticalRegion()

#endif // TOKEN_DIAGNOSTICS_ENABLED

#define SepReleaseTokenWriteLock(T,M)                                    \
    {                                                                    \
      if ((M)) {                                                         \
          ExAllocateLocallyUniqueId( &((PTOKEN)(T))->ModifiedId  );      \
      }                                                                  \
      SepReleaseTokenReadLock( T );                                      \
    }

//
// Reference individual privilege attribute flags of any privilege array
//
//  P - is a pointer to an array of privileges (PLUID_AND_ATTRIBUTES)
//  I - is the index of the privilege
//  A - is the name of the attribute desired (e.g., Enabled, EnabledByDefault, etc. )
//

#define SepArrayPrivilegeAttributes(P,I) ( (P)[I].Attributes )

//
// Reference individual privilege attribute flags of token privileges
//
//  T - is a pointer to a token
//  I - is the index of the privilege
//  A - is the name of the attribute desired (e.g., Enabled, EnabledByDefault, etc. )
//

#define SepTokenPrivilegeAttributes(T,I) ( (T)->Privileges[I].Attributes )

//
// Reference individual group attribute flags of any group array
//
//  G - is a pointer to the array of groups  (SID_AND_ATTRIBUTES[])
//  I - is the index of the group
//

#define SepArrayGroupAttributes(G,I)   ( (G)[I].Attributes )


//
// Reference individual group attribute flags of token groups
//
//  T - is a pointer to a token
//  I - is the index of the group
//

#define SepTokenGroupAttributes(T,I) ( (T)->UserAndGroups[I].Attributes )




////////////////////////////////////////////////////////////////////////
//                                                                    //
//           Private Routine Declarations                             //
//                                                                    //
////////////////////////////////////////////////////////////////////////

NTSTATUS
SepAdjustGroups(
    IN PTOKEN Token,
    IN BOOLEAN MakeChanges,
    IN BOOLEAN ResetToDefault,
    IN ULONG GroupCount OPTIONAL,
    IN PSID_AND_ATTRIBUTES NewState OPTIONAL,
    OUT PTOKEN_GROUPS PreviousState OPTIONAL,
    OUT PSID SidBuffer OPTIONAL,
    OUT PULONG ReturnLength,
    OUT PULONG ChangeCount,
    OUT PBOOLEAN ChangesMade
    );

NTSTATUS
SepAdjustPrivileges(
    IN PTOKEN Token,
    IN BOOLEAN MakeChanges,
    IN BOOLEAN DisableAllPrivileges,
    IN ULONG PrivilegeCount OPTIONAL,
    IN PLUID_AND_ATTRIBUTES NewState OPTIONAL,
    OUT PTOKEN_PRIVILEGES PreviousState OPTIONAL,
    OUT PULONG ReturnLength,
    OUT PULONG ChangeCount,
    OUT PBOOLEAN ChangesMade
    );

VOID
SepAppendDefaultDacl(
    IN PTOKEN Token,
    IN PACL PAcl
    );

VOID
SepAppendPrimaryGroup(
    IN PTOKEN Token,
    IN PSID PSid
    );

NTSTATUS
SepDuplicateToken(
    IN PTOKEN ExistingToken,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN EffectiveOnly,
    IN TOKEN_TYPE TokenType,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel OPTIONAL,
    IN KPROCESSOR_MODE RequestorMode,
    OUT PTOKEN *DuplicateToken
    );

NTSTATUS
SepFilterToken(
    IN PTOKEN ExistingToken,
    IN KPROCESSOR_MODE RequestorMode,
    IN ULONG Flags,
    IN ULONG GroupCount,
    IN PSID_AND_ATTRIBUTES GroupsToDisable OPTIONAL,
    IN ULONG PrivilegeCount,
    IN PLUID_AND_ATTRIBUTES PrivilegesToDelete OPTIONAL,
    IN ULONG SidCount,
    IN PSID_AND_ATTRIBUTES RestrictedSids OPTIONAL,
    IN ULONG SidLength,
    OUT PTOKEN * FilteredToken
    );

BOOLEAN
SepSidInSidAndAttributes (
    IN PSID_AND_ATTRIBUTES SidAndAttributes,
    IN ULONG SidCount,
    IN PSID PrincipalSelfSid,
    IN PSID Sid
    );

VOID
SepRemoveDisabledGroupsAndPrivileges(
    IN PTOKEN Token,
    IN ULONG Flags,
    IN ULONG GroupCount,
    IN PSID_AND_ATTRIBUTES GroupsToDisable,
    IN ULONG PrivilegeCount,
    IN PLUID_AND_ATTRIBUTES PrivilegesToDelete
    );


VOID
SepFreeDefaultDacl(
    IN PTOKEN Token
    );

VOID
SepFreePrimaryGroup(
    IN PTOKEN Token
    );


BOOLEAN
SepIdAssignableAsOwner(
    IN PTOKEN Token,
    IN ULONG Index
    );

VOID
SepMakeTokenEffectiveOnly(
    IN PTOKEN Token
    );

BOOLEAN
SepTokenInitialization( VOID );


VOID
SepTokenDeleteMethod (
    IN PVOID Token
    );

//
// These are here because if they are placed in sep.h, we don't
// have the PTOKEN datatype available.
//

BOOLEAN
SepPrivilegeCheck(
    IN PTOKEN Token,
    IN OUT PLUID_AND_ATTRIBUTES RequiredPrivileges,
    IN ULONG RequiredPrivilegeCount,
    IN ULONG PrivilegeSetControl,
    IN KPROCESSOR_MODE PreviousMode
    );

VOID
SepAccessCheck (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID PrincipalSelfSid,
    IN PTOKEN PrimaryToken,
    IN PTOKEN ClientToken OPTIONAL,
    IN ACCESS_MASK DesiredAccess,
    IN PIOBJECT_TYPE_LIST ObjectTypeList OPTIONAL,
    IN ULONG ObjectTypeListLength,
    IN PGENERIC_MAPPING GenericMapping,
    IN ACCESS_MASK PreviouslyGrantedAccess,
    IN KPROCESSOR_MODE PreviousMode,
    OUT PACCESS_MASK GrantedAccess,
    OUT PPRIVILEGE_SET *Privileges OPTIONAL,
    OUT PNTSTATUS AccessStatus,
    IN BOOLEAN ReturnResultList,
    OUT PBOOLEAN ReturnSomeAccessGranted,
    OUT PBOOLEAN ReturnSomeAccessDenied
    );

BOOLEAN
SepObjectInTypeList (
    IN GUID *ObjectType,
    IN PIOBJECT_TYPE_LIST ObjectTypeList,
    IN ULONG ObjectTypeListLength,
    OUT PULONG ReturnedIndex
    );

#ifdef TOKEN_DEBUG
VOID
SepDumpToken(
    IN PTOKEN T
    );
#endif //TOKEN_DEBUG

////////////////////////////////////////////////////////////////////////
//                                                                    //
//           Global Variables                                         //
//                                                                    //
////////////////////////////////////////////////////////////////////////


extern GENERIC_MAPPING  SepTokenMapping;
extern POBJECT_TYPE     SepTokenObjectType;

extern ERESOURCE        SepTokenLock;


#ifdef    TOKEN_DIAGNOSTICS_ENABLED
extern ULONG            TokenGlobalFlag;
#endif // TOKEN_DIAGNOSTICS_ENABLED


#endif // _TOKENP_
