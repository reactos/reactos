/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sep.h

Abstract:

    This module contains the internal (private) declarations needed by the
    Kernel mode security routines.

Author:

    Gary Kimura (GaryKi) 31-Mar-1989
    Jim Kelly (JimK) 2-Mar-1990

Revision History:


--*/

#ifndef _SEP_
#define _SEP_

#include "ntos.h"
#include <ntrmlsa.h>
#include "seopaque.h"



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                SE Diagnostics                                       //
//                                                                     //
/////////////////////////////////////////////////////////////////////////



#if DBG
#define SE_DIAGNOSTICS_ENABLED 1
#endif // DBG


//
// These definitions are useful diagnostics aids
//

#if SE_DIAGNOSTICS_ENABLED

//
// Test for enabled diagnostic
//

#define IF_SE_GLOBAL( FlagName ) \
    if (SeGlobalFlag & (SE_DIAG_##FlagName))

//
// Diagnostics print statement
//

#define SeDiagPrint( FlagName, _Text_ )                               \
    IF_SE_GLOBAL( FlagName )                                          \
        DbgPrint _Text_


#else

//
// diagnostics not enabled - No diagnostics included in build
//


//
// Test for diagnostics enabled
//

#define IF_SE_GLOBAL( FlagName ) if (FALSE)

//
// Diagnostics print statement (expands to no-op)
//

#define SeDiagPrint( FlagName, _Text_ )     ;

#endif // SE_DIAGNOSTICS_ENABLED




//
// The following flags enable or disable various diagnostic
// capabilities within SE code.  These flags are set in
// SeGlobalFlag (only available within a DBG system).
//
//      SD_TRACKING - Display information about create/deletion of
//          shared security descriptors
//
//

#define SE_DIAG_SD_TRACKING          ((ULONG) 0x00000001L)





//
// Control flag manipulation macros
//

//
//  Macro to query whether or not control flags ALL on
//  or not (ie, returns FALSE if any of the flags are not set)
//

#define SepAreFlagsSet( Mask, Bits )                                           \
            (                                                                  \
            ((Mask) & ( Bits )) == ( Bits )                                    \
            )

//
//  Macro to set the specified control bits in the given Security Descriptor
//

#define SepSetFlags( Mask, Bits )                                              \
            (                                                                  \
            ( Mask ) |= ( Bits )                                               \
            )

//
//  Macro to clear the passed control bits in the given Security Descriptor
//

#define SepClearFlags( Mask, Bits )                                            \
            (                                                                  \
            ( Mask ) &= ~( Bits )                                              \
            )




//
// Macro to determine the size of a PRIVILEGE_SET
//

#define SepPrivilegeSetSize( PrivilegeSet )                                    \
        ( ( PrivilegeSet ) == NULL ? 0 :                                       \
        ((( PrivilegeSet )->PrivilegeCount > 0)                                \
         ?                                                                     \
         ((ULONG)sizeof(PRIVILEGE_SET) +                                       \
           (                                                                   \
             (( PrivilegeSet )->PrivilegeCount  -  ANYSIZE_ARRAY) *            \
             (ULONG)sizeof(LUID_AND_ATTRIBUTES)                                \
           )                                                                   \
         )                                                                     \
         : ((ULONG)sizeof(PRIVILEGE_SET) - (ULONG)sizeof(LUID_AND_ATTRIBUTES)) \
        ))


//
//      Return the effective token from a SecurityContext
//

#define EffectiveToken( SubjectSecurityContext ) (                            \
                 (SubjectSecurityContext)->ClientToken ?                      \
                 (SubjectSecurityContext)->ClientToken :                      \
                 (SubjectSecurityContext)->PrimaryToken                       \
                 )                                                            \


//
//      Return a pointer to the Sid of the User of a given token
//

#define SepTokenUserSid( Token )   ((PTOKEN)(Token))->UserAndGroups->Sid


//
//      Return the AuthenticationId from a given token
//

#define SepTokenAuthenticationId( Token )   (((PTOKEN)(Token))->AuthenticationId)



//
//
// BOOLEAN
// SepBadImpersonationLevel(
//     IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
//     IN BOOLEAN ServerIsRemote
//     )
//
// Routine Description:
//
//     Determine whether a client is trying to impersonate innappropriately
//     This routine should only be called if a thread requesting impersonation
//     is itself already impersonating a client of its own.  This routine
//     indicates whether the client is attempting to violate the level of
//     impersonation granted to it by its client.
//
// Arguments:
//
//     ImpersonationLevel - Is the impersonation level of the client's
//         effective token.
//
//     ServerIsRemote - Is a boolean flag indicating whether the client
//         is requesting impersonation services to a remote system.  TRUE
//         indicates the session is a remote session, FALSE indicates the
//         session is a local session.  Delegation level is necessary to
//         achieve a remote session.
//
// Return Value:
//
//     TRUE - Indicates that the impersonation level of the client's client
//         token is innapropriate for the attempted impersonation.
//         An error (STATUS_BAD_IMPERSONATION_LEVEL) should be generated.
//
//     FALSE - Indicates the impersonation attempt is not bad, and should
//         be allowed.
//
//

#define SepBadImpersonationLevel(IL,SIR)  ((                                   \
            ((IL) == SecurityAnonymous) || ((IL) == SecurityIdentification) || \
            ( (SIR) && ((IL) != SecurityDelegation) )                          \
            ) ? TRUE : FALSE )



//++
//
// BOOL
// IsValidElementCount(
//      IN ULONG Count,
//      IN <STRUCTURE>
//      );
//
//--

#define IsValidElementCount( Count, STRUCTURE ) \
    ( Count < ( (ULONG_PTR) ( (PUCHAR) ( (PUCHAR) (LONG_PTR)(LONG)0xFFFFFFFF - (PUCHAR) MM_SYSTEM_RANGE_START ) + 1 ) \
        / sizeof( STRUCTURE ) ) )



///////////////////////////////////////////////////////////////////////////
//                                                                       //
//  Constants                                                            //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

#define SEP_MAX_GROUP_COUNT 4096
#define SEP_MAX_PRIVILEGE_COUNT 4096


///////////////////////////////////////////////////////////////////////////
//                                                                       //
//  Private Data types                                                   //
//                                                                       //
///////////////////////////////////////////////////////////////////////////


extern HANDLE SepLsaHandle;

extern BOOLEAN SepAuditShutdownEvents;

//
// Spinlock protecting the queue of work being passed to LSA
//

extern ERESOURCE SepLsaQueueLock;

extern ULONG SepLsaQueueLength;

//
// Doubly linked list of work items queued to worker threads.
//

extern LIST_ENTRY SepLsaQueue;


// #define SepAcquireTokenReadLock(T)  KeEnterCriticalRegion();          \
//                                     ExAcquireResourceShared(&SepTokenLock, TRUE)

#define SepLockLsaQueue()  KeEnterCriticalRegion();                           \
                           ExAcquireResourceExclusive(&SepLsaQueueLock, TRUE)

#define SepUnlockLsaQueue() ExReleaseResource(&SepLsaQueueLock);              \
                            KeLeaveCriticalRegion()

#define  SepWorkListHead()  ((PSEP_LSA_WORK_ITEM)(&SepLsaQueue)->Flink)

#ifndef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'  eS')
#endif
#ifndef ExAllocatePoolWithQuota
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,'  eS')
#endif

typedef
VOID
(*PSEP_LSA_WORKER_CLEANUP_ROUTINE)(
    IN PVOID Parameter
    );


typedef enum _SEP_LSA_WORK_ITEM_TAG {
    SepDeleteLogon,
    SepAuditRecord
} SEP_LSA_WORK_ITEM_TAG, *PSEP_LSA_WORK_ITEM_TAG;





typedef struct _SEP_LSA_WORK_ITEM {

    //
    // This field must be the first field of this structure
    //

    LIST_ENTRY                      List;

    //
    // Command Params Memory type
    //

    SEP_RM_LSA_MEMORY_TYPE          CommandParamsMemoryType;

    //
    // Tag describing what kind of structure we've got
    //

    SEP_LSA_WORK_ITEM_TAG           Tag;

    //
    // The following union contains the data to be passed
    // to LSA.
    //

    union {

        PVOID                       BaseAddress;
        LUID                        LogonId;

    } CommandParams;

    //
    // These fields must be filled in by the caller of SepRmCallLsa
    //

    LSA_COMMAND_NUMBER              CommandNumber;
    ULONG                           CommandParamsLength;
    PVOID                           ReplyBuffer;
    ULONG                           ReplyBufferLength;

    //
    // CleanupFunction (if specified) will be called with CleanupParameter
    // as its argument before the SEP_LSA_WORK_ITEM is freed by SepRmCallLsa
    //

    PSEP_LSA_WORKER_CLEANUP_ROUTINE CleanupFunction;
    PVOID                           CleanupParameter;

} SEP_LSA_WORK_ITEM, *PSEP_LSA_WORK_ITEM;


typedef struct _SEP_WORK_ITEM {

    WORK_QUEUE_ITEM  WorkItem;

} SEP_WORK_ITEM, *PSEP_WORK_ITEM;

extern SEP_WORK_ITEM SepExWorkItem;







///////////////////////////////////////////////////////////////////////////
//                                                                       //
//  Private Routines                                                     //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

BOOLEAN
SepDevelopmentTest( VOID );      //Used only for development testing


BOOLEAN
SepInitializationPhase0( VOID );

BOOLEAN
SepInitializationPhase1( VOID );

BOOLEAN
SepVariableInitialization( VOID );

NTSTATUS
SepCreateToken(
    OUT PHANDLE TokenHandle,
    IN KPROCESSOR_MODE RequestorMode,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN TOKEN_TYPE TokenType,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel OPTIONAL,
    IN PLUID AuthenticationId,
    IN PLARGE_INTEGER ExpirationTime,
    IN PSID_AND_ATTRIBUTES User,
    IN ULONG GroupCount,
    IN PSID_AND_ATTRIBUTES Groups,
    IN ULONG GroupsLength,
    IN ULONG PrivilegeCount,
    IN PLUID_AND_ATTRIBUTES Privileges,
    IN ULONG PrivilegesLength,
    IN PSID Owner OPTIONAL,
    IN PSID PrimaryGroup,
    IN PACL DefaultDacl OPTIONAL,
    IN PTOKEN_SOURCE TokenSource,
    IN BOOLEAN SystemToken,
    IN PSECURITY_TOKEN_PROXY_DATA ProxyData OPTIONAL,
    IN PSECURITY_TOKEN_AUDIT_DATA AuditData OPTIONAL
    );

NTSTATUS
SepReferenceLogonSession(
    IN PLUID LogonId
    );

VOID
SepDeReferenceLogonSession(
    IN PLUID LogonId
    );

VOID
SepLockSubjectContext(
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext
    );

VOID
SepFreeSubjectContext(
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext
    );

VOID
SepGetDefaultsSubjectContext(
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
    OUT PSID *Owner,
    OUT PSID *Group,
    OUT PSID *ServerOwner,
    OUT PSID *ServerGroup,
    OUT PACL *Dacl
    );

BOOLEAN
SepValidOwnerSubjectContext(
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
    IN PSID Owner,
    IN BOOLEAN ServerObject
    );

BOOLEAN
SepIdAssignableAsGroup(
    IN PACCESS_TOKEN Token,
    IN PSID Group
    );

BOOLEAN
SepCheckAcl (
    IN PACL Acl,
    IN ULONG Length
    );

BOOLEAN
SepAuditAlarm (
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN PUNICODE_STRING ObjectTypeName,
    IN PUNICODE_STRING ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN ObjectCreation,
    IN ACCESS_MASK GrantedAccess,
    OUT PBOOLEAN GenerateOnClose
    );

BOOLEAN
SepSinglePrivilegeCheck (
   LUID DesiredPrivilege,
   IN PACCESS_TOKEN EffectiveToken,
   IN KPROCESSOR_MODE PreviousMode
   );

NTSTATUS
SepRmCallLsa(
    PSEP_WORK_ITEM SepWorkItem
    );

BOOLEAN
SepInitializeWorkList(
    VOID
    );

BOOLEAN
SepRmInitPhase0(
    );

VOID
SepConcatenatePrivileges(
    IN PPRIVILEGE_SET TargetPrivilegeSet,
    IN ULONG TargetBufferSize,
    IN PPRIVILEGE_SET SourcePrivilegeSet
    );

BOOLEAN
SepTokenIsOwner(
    IN PACCESS_TOKEN Token,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN BOOLEAN TokenLocked
    );

VOID
SepPrintAcl (
    IN PACL Acl
    );

VOID
SepPrintSid(
    IN PSID Sid
    );

VOID
SepDumpSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSZ TitleString
    );

BOOLEAN
SepSidTranslation(
    PSID Sid,
    PSTRING AccountName
    );

VOID
SepDumpTokenInfo(
    IN PACCESS_TOKEN Token
    );

VOID
SepDumpString(
    IN PUNICODE_STRING String
    );

BOOLEAN
SepSidInToken (
    IN PACCESS_TOKEN Token,
    IN PSID PrincipalSelfSid,
    IN PSID Sid,
    IN BOOLEAN DenyAce
    );


VOID
SepExamineSacl(
    IN PACL Sacl,
    IN PACCESS_TOKEN Token,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN AccessGranted,
    OUT PBOOLEAN GenerateAudit,
    OUT PBOOLEAN GenerateAlarm
    );


VOID
SepCopyString (
    IN PUNICODE_STRING SourceString,
    OUT PUNICODE_STRING *DestString
    );

VOID
SepAssemblePrivileges(
    IN ULONG PrivilegeCount,
    IN BOOLEAN SystemSecurity,
    IN BOOLEAN WriteOwner,
    OUT PPRIVILEGE_SET *Privileges
    );


PUNICODE_STRING
SepQueryTypeString(
    IN PVOID Object
    );


POBJECT_NAME_INFORMATION
SepQueryNameString(
    IN PVOID Object
    );

BOOLEAN
SepFilterPrivilegeAudits(
    IN PPRIVILEGE_SET PrivilegeSet
    );

BOOLEAN
SepQueueWorkItem(
    IN PSEP_LSA_WORK_ITEM LsaWorkItem,
    IN BOOLEAN ForceQueue
    );

PSEP_LSA_WORK_ITEM
SepDequeueWorkItem(
    VOID
    );

VOID
SepAdtGenerateDiscardAudit(
    VOID
    );

BOOLEAN
SepAdtValidateAuditBounds(
    ULONG Upper,
    ULONG Lower
    );

NTSTATUS
SepAdtInitializeCrashOnFail(
    VOID
    );

BOOLEAN
SepAdtInitializePrivilegeAuditing(
    VOID
    );

NTSTATUS
SepCopyProxyData (
    OUT PSECURITY_TOKEN_PROXY_DATA * DestProxyData,
    IN PSECURITY_TOKEN_PROXY_DATA SourceProxyData
    );

VOID
SepFreeProxyData (
    IN PSECURITY_TOKEN_PROXY_DATA ProxyData
    );

NTSTATUS
SepProbeAndCaptureQosData(
    IN PSECURITY_ADVANCED_QUALITY_OF_SERVICE CapturedSecurityQos
    );

PACCESS_TOKEN
SeMakeAnonymousToken ();

#endif // _SEP_
