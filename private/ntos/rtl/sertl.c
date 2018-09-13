/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sertl.c

Abstract:

    This Module implements many security rtl routines defined in ntseapi.h

Author:

    Jim Kelly       (JimK)     23-Mar-1990
    Robert Reichel  (RobertRe)  1-Mar-1991

Environment:

    Pure Runtime Library Routine

Revision History:


--*/


#include "ntrtlp.h"
#include <stdio.h>
#include "seopaque.h"
#include "sertlp.h"
#ifdef NTOS_KERNEL_RUNTIME
#include <..\se\sep.h>
#else // NTOS_KERNEL_RUNTIME
#include <..\dll\ldrp.h>
#endif // NTOS_KERNEL_RUNTIME

//
// BUG, BUG does anybody use this routine - no prototype in ntrtl.h
//

ULONG
RtlLengthUsedSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    );

#undef RtlEqualLuid

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualLuid (
    PLUID Luid1,
    PLUID Luid2
    );

NTSTATUS
RtlpConvertAclToAutoInherit (
    IN PACL ParentAcl OPTIONAL,
    IN PACL ChildAcl,
    IN GUID *ObjectType OPTIONAL,
    IN BOOLEAN IsDirectoryObject,
    IN PSID OwnerSid,
    IN PSID GroupSid,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PACL *NewAcl,
    OUT PULONG NewGenericControl
    );

BOOLEAN
RtlpCopyEffectiveAce (
    IN PACE_HEADER OldAce,
    IN BOOLEAN AutoInherit,
    IN BOOLEAN WillGenerateInheritAce,
    IN PSID ClientOwnerSid,
    IN PSID ClientGroupSid,
    IN PSID ServerOwnerSid OPTIONAL,
    IN PSID ServerGroupSid OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping,
    IN GUID *NewObjectType OPTIONAL,
    IN OUT PVOID *AcePosition,
    OUT PULONG NewAceLength,
    OUT PACL NewAcl,
    OUT PBOOLEAN ObjectAceInherited OPTIONAL,
    OUT PBOOLEAN EffectiveAceMapped,
    OUT PBOOLEAN AclOverflowed
    );

typedef enum {
     CopyInheritedAces,
     CopyNonInheritedAces,
     CopyAllAces } ACE_TYPE_TO_COPY;

NTSTATUS
RtlpCopyAces(
    IN PACL Acl,
    IN PGENERIC_MAPPING GenericMapping,
    IN ACE_TYPE_TO_COPY AceTypeToCopy,
    IN UCHAR AceFlagsToReset,
    IN BOOLEAN MapSids,
    IN PSID ClientOwnerSid,
    IN PSID ClientGroupSid,
    IN PSID ServerOwnerSid OPTIONAL,
    IN PSID ServerGroupSid OPTIONAL,
    OUT PULONG NewAclSizeParam,
    OUT PACL NewAcl
    );

NTSTATUS
RtlpGenerateInheritedAce (
    IN PACE_HEADER OldAce,
    IN BOOLEAN IsDirectoryObject,
    IN BOOLEAN AutoInherit,
    IN PSID ClientOwnerSid,
    IN PSID ClientGroupSid,
    IN PSID ServerOwnerSid OPTIONAL,
    IN PSID ServerGroupSid OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping,
    IN GUID *NewObjectType OPTIONAL,
    OUT PULONG NewAceLength,
    OUT PACL NewAcl,
    OUT PULONG NewAceExtraLength,
    OUT PBOOLEAN ObjectAceInherited
    );

NTSTATUS
RtlpGenerateInheritAcl(
    IN PACL Acl,
    IN BOOLEAN IsDirectoryObject,
    IN BOOLEAN AutoInherit,
    IN PSID ClientOwnerSid,
    IN PSID ClientGroupSid,
    IN PSID ServerOwnerSid OPTIONAL,
    IN PSID ServerGroupSid OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping,
    IN GUID *NewObjectType OPTIONAL,
    OUT PULONG NewAclSizeParam,
    OUT PACL NewAcl,
    OUT PBOOLEAN ObjectAceInherited
    );

NTSTATUS
RtlpInheritAcl2 (
    IN PACL DirectoryAcl,
    IN PACL ChildAcl,
    IN ULONG ChildGenericControl,
    IN BOOLEAN IsDirectoryObject,
    IN BOOLEAN AutoInherit,
    IN BOOLEAN DefaultDescriptorForObject,
    IN PSID OwnerSid,
    IN PSID GroupSid,
    IN PSID ServerOwnerSid OPTIONAL,
    IN PSID ServerGroupSid OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOLEAN IsSacl,
    IN GUID *NewObjectType OPTIONAL,
    IN PULONG AclBufferSize,
    IN OUT PUCHAR AclBuffer,
    OUT PBOOLEAN NewAclExplicitlyAssigned,
    OUT PULONG NewGenericControl
    );

NTSTATUS
RtlpComputeMergedAcl (
    IN PACL CurrentAcl,
    IN ULONG CurrentGenericControl,
    IN PACL ModificationAcl,
    IN ULONG ModificationGenericControl,
    IN PSID ClientOwnerSid,
    IN PSID ClientGroupSid,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOLEAN IsSacl,
    OUT PACL *NewAcl,
    OUT PULONG NewGenericControl
    );

NTSTATUS
RtlpComputeMergedAcl2 (
    IN PACL CurrentAcl,
    IN ULONG CurrentGenericControl,
    IN PACL ModificationAcl,
    IN ULONG ModificationGenericControl,
    IN PSID ClientOwnerSid,
    IN PSID ClientGroupSid,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOLEAN IsSacl,
    IN PULONG AclBufferSize,
    IN OUT PUCHAR AclBuffer,
    OUT PULONG NewGenericControl
    );

BOOLEAN
RtlpCompareAces(
    IN PKNOWN_ACE InheritedAce,
    IN PKNOWN_ACE ChildAce,
    IN PSID OwnerSid,
    IN PSID GroupSid
    );

BOOLEAN
RtlpCompareKnownObjectAces(
    IN PKNOWN_OBJECT_ACE InheritedAce,
    IN PKNOWN_OBJECT_ACE ChildAce,
    IN PSID OwnerSid OPTIONAL,
    IN PSID GroupSid OPTIONAL
    );

BOOLEAN
RtlpCompareKnownAces(
    IN PKNOWN_ACE InheritedAce,
    IN PKNOWN_ACE ChildAce,
    IN PSID OwnerSid OPTIONAL,
    IN PSID GroupSid OPTIONAL
    );

BOOLEAN
RtlpIsDuplicateAce(
    IN PACL Acl,
    IN PKNOWN_ACE NewAce,
    IN GUID *ObjectType OPTIONAL
    );

NTSTATUS
RtlpCreateServerAcl(
    IN PACL Acl,
    IN BOOLEAN AclUntrusted,
    IN PSID ServerSid,
    OUT PACL *ServerAcl,
    OUT BOOLEAN *ServerAclAllocated
    );

NTSTATUS
RtlpGetDefaultsSubjectContext(
    HANDLE ClientToken,
    OUT PTOKEN_OWNER *OwnerInfo,
    OUT PTOKEN_PRIMARY_GROUP *GroupInfo,
    OUT PTOKEN_DEFAULT_DACL *DefaultDaclInfo,
    OUT PTOKEN_OWNER *ServerOwner,
    OUT PTOKEN_PRIMARY_GROUP *ServerGroup
    );

BOOLEAN RtlpValidateSDOffsetAndSize (
    IN ULONG   Offset,
    IN ULONG   Length,
    IN ULONG   MinLength,
    OUT PULONG MaxLength
    );

BOOLEAN
RtlValidRelativeSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptorInput,
    IN ULONG SecurityDescriptorLength,
    IN SECURITY_INFORMATION RequiredInformation
    );

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlRunEncodeUnicodeString)
#pragma alloc_text(PAGE,RtlRunDecodeUnicodeString)
#pragma alloc_text(PAGE,RtlEraseUnicodeString)
#pragma alloc_text(PAGE,RtlAdjustPrivilege)
#pragma alloc_text(PAGE,RtlValidSid)
#pragma alloc_text(PAGE,RtlEqualSid)
#pragma alloc_text(PAGE,RtlEqualPrefixSid)
#pragma alloc_text(PAGE,RtlLengthRequiredSid)
#pragma alloc_text(PAGE,RtlInitializeSid)
#pragma alloc_text(PAGE,RtlFreeSid)
#pragma alloc_text(PAGE,RtlIdentifierAuthoritySid)
#pragma alloc_text(PAGE,RtlSubAuthoritySid)
#pragma alloc_text(PAGE,RtlSubAuthorityCountSid)
#pragma alloc_text(PAGE,RtlLengthSid)
#pragma alloc_text(PAGE,RtlCopySid)
#pragma alloc_text(PAGE,RtlCopySidAndAttributesArray)
#pragma alloc_text(PAGE,RtlConvertSidToUnicodeString)
#pragma alloc_text(PAGE,RtlEqualLuid)
#pragma alloc_text(PAGE,RtlCopyLuid)
#pragma alloc_text(PAGE,RtlCopyLuidAndAttributesArray)
#pragma alloc_text(PAGE,RtlCreateSecurityDescriptor)
#pragma alloc_text(PAGE,RtlCreateSecurityDescriptorRelative)
#pragma alloc_text(PAGE,RtlValidSecurityDescriptor)
#pragma alloc_text(PAGE,RtlLengthSecurityDescriptor)
#pragma alloc_text(PAGE,RtlLengthUsedSecurityDescriptor)
#pragma alloc_text(PAGE,RtlGetControlSecurityDescriptor)
#pragma alloc_text(PAGE,RtlSetControlSecurityDescriptor)
#pragma alloc_text(PAGE,RtlSetDaclSecurityDescriptor)
#pragma alloc_text(PAGE,RtlGetDaclSecurityDescriptor)
#pragma alloc_text(PAGE,RtlSetSaclSecurityDescriptor)
#pragma alloc_text(PAGE,RtlGetSaclSecurityDescriptor)
#pragma alloc_text(PAGE,RtlSetOwnerSecurityDescriptor)
#pragma alloc_text(PAGE,RtlGetOwnerSecurityDescriptor)
#pragma alloc_text(PAGE,RtlSetGroupSecurityDescriptor)
#pragma alloc_text(PAGE,RtlGetGroupSecurityDescriptor)
#pragma alloc_text(PAGE,RtlGetSecurityDescriptorRMControl)
#pragma alloc_text(PAGE,RtlSetSecurityDescriptorRMControl)
#pragma alloc_text(PAGE,RtlAreAllAccessesGranted)
#pragma alloc_text(PAGE,RtlAreAnyAccessesGranted)
#pragma alloc_text(PAGE,RtlMapGenericMask)
#pragma alloc_text(PAGE,RtlpApplyAclToObject)
#pragma alloc_text(PAGE,RtlpContainsCreatorGroupSid)
#pragma alloc_text(PAGE,RtlpContainsCreatorOwnerSid)
#pragma alloc_text(PAGE,RtlpCopyEffectiveAce)
#pragma alloc_text(PAGE,RtlpCopyAces)
#pragma alloc_text(PAGE,RtlpGenerateInheritAcl)
#pragma alloc_text(PAGE,RtlpGenerateInheritedAce)
#pragma alloc_text(PAGE,RtlpInheritAcl)
#pragma alloc_text(PAGE,RtlpInheritAcl2)
#pragma alloc_text(PAGE,RtlpComputeMergedAcl)
#pragma alloc_text(PAGE,RtlpComputeMergedAcl2)
#pragma alloc_text(PAGE,RtlpValidOwnerSubjectContext)
#pragma alloc_text(PAGE,RtlpConvertToAutoInheritSecurityObject)
#pragma alloc_text(PAGE,RtlpCompareAces)
#pragma alloc_text(PAGE,RtlpCompareKnownAces)
#pragma alloc_text(PAGE,RtlpCompareKnownObjectAces)
#pragma alloc_text(PAGE,RtlpIsDuplicateAce)
#pragma alloc_text(PAGE,RtlpConvertAclToAutoInherit)
#pragma alloc_text(PAGE,RtlSetSecurityObjectEx)
#pragma alloc_text(PAGE,RtlpCreateServerAcl)
#pragma alloc_text(PAGE,RtlValidRelativeSecurityDescriptor)
#pragma alloc_text(PAGE,RtlpValidateSDOffsetAndSize)
#endif



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//    Local Macros and Symbols                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


#define CREATOR_SID_SIZE 12

#define max(a,b)            (((a) > (b)) ? (a) : (b))

//
// Define an array mapping all ACE types to their base type.
//
// For instance, all allowed ACE types are similar.  As are all denied ACE types.
//

UCHAR RtlBaseAceType[] = {
    ACCESS_ALLOWED_ACE_TYPE,    // ACCESS_ALLOWED_ACE_TYPE (0x0)
    ACCESS_DENIED_ACE_TYPE,     // ACCESS_DENIED_ACE_TYPE (0x1)
    SYSTEM_AUDIT_ACE_TYPE,      // SYSTEM_AUDIT_ACE_TYPE (0x2)
    SYSTEM_ALARM_ACE_TYPE,      // SYSTEM_ALARM_ACE_TYPE (0x3)
    ACCESS_ALLOWED_ACE_TYPE,    // ACCESS_ALLOWED_COMPOUND_ACE_TYPE (0x4)
    ACCESS_ALLOWED_ACE_TYPE,    // ACCESS_ALLOWED_OBJECT_ACE_TYPE (0x5)
    ACCESS_DENIED_ACE_TYPE,     // ACCESS_DENIED_OBJECT_ACE_TYPE (0x6)
    SYSTEM_AUDIT_ACE_TYPE,      // SYSTEM_AUDIT_OBJECT_ACE_TYPE (0x7)
    SYSTEM_ALARM_ACE_TYPE       // SYSTEM_ALARM_OBJECT_ACE_TYPE (0x8)
};

//
// Define an array defining whether an ACE is a system ACE
//

UCHAR RtlIsSystemAceType[] = {
    FALSE,    // ACCESS_ALLOWED_ACE_TYPE (0x0)
    FALSE,    // ACCESS_DENIED_ACE_TYPE (0x1)
    TRUE,     // SYSTEM_AUDIT_ACE_TYPE (0x2)
    TRUE,     // SYSTEM_ALARM_ACE_TYPE (0x3)
    FALSE,    // ACCESS_ALLOWED_COMPOUND_ACE_TYPE (0x4)
    FALSE,    // ACCESS_ALLOWED_OBJECT_ACE_TYPE (0x5)
    FALSE,    // ACCESS_DENIED_OBJECT_ACE_TYPE (0x6)
    TRUE,     // SYSTEM_AUDIT_OBJECT_ACE_TYPE (0x7)
    TRUE      // SYSTEM_ALARM_OBJECT_ACE_TYPE (0x8)
};

BOOLEAN RtlpVerboseConvert = FALSE;

#define SE_VALID_CONTROL_BITS ( SE_DACL_UNTRUSTED | \
                                SE_SERVER_SECURITY | \
                                SE_DACL_AUTO_INHERIT_REQ | \
                                SE_SACL_AUTO_INHERIT_REQ | \
                                SE_DACL_AUTO_INHERITED | \
                                SE_SACL_AUTO_INHERITED | \
                                SE_DACL_PROTECTED | \
                                SE_SACL_PROTECTED )


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//    Exported Procedures                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


VOID
RtlRunEncodeUnicodeString(
    PUCHAR          Seed        OPTIONAL,
    PUNICODE_STRING String
    )

/*++

Routine Description:

    This function performs a trivial XOR run-encoding of a string.
    The purpose of this run-encoding is to change the character values
    to appear somewhat random and typically not printable.  This is
    useful for transforming passwords that you don't want to be easily
    distinguishable by visually scanning a paging file or memory dump.


Arguments:

    Seed - Points to a seed value to use in the encoding.  If the
        pointed to value is zero, then this routine will assign
        a value.

    String - The string to encode.  This string may be decode
        by passing it and the seed value to RtlRunDecodeUnicodeString().


Return Value:

    None - Nothing can really go wrong unless the caller passes bogus
        parameters.  In this case, the caller can catch the access
        violation.


--*/
{

    LARGE_INTEGER Time;
    PUCHAR        LocalSeed;
    NTSTATUS      Status;
    ULONG         i;
    PSTRING       S;


    RTL_PAGED_CODE();

    //
    // Typecast so we can work on bytes rather than WCHARs
    //

    S = (PSTRING)((PVOID)String);

    //
    // If a seed wasn't passed, use the 2nd byte of current time.
    // This byte seems to be sufficiently random (by observation).
    //

    if ((*Seed) == 0) {
        Status = NtQuerySystemTime ( &Time );
        ASSERT(NT_SUCCESS(Status));

        LocalSeed = (PUCHAR)((PVOID)&Time);

        i = 1;

        (*Seed) = LocalSeed[ i ];

        //
        // Occasionally, this byte could be zero.  That would cause the
        // string to become un-decodable, since 0 is the magic value that
        // causes us to re-gen the seed.  This loop makes sure that we
        // never end up with a zero byte (unless time is zero, as well).
        //

        while ( ((*Seed) == 0) && ( i < sizeof( Time ) ) )
        {
            (*Seed) |= LocalSeed[ i++ ] ;
        }

        if ( (*Seed) == 0 )
        {
            (*Seed) = 1;
        }
    }

    //
    // Transform the initial byte.
    // The funny constant just keeps the first byte from propagating
    // into the second byte in the next step.  Without a funny constant
    // this would happen for many languages (which typically have every
    // other byte zero.
    //
    //

    if (S->Length >= 1) {
        S->Buffer[0] ^= ((*Seed) | 0X43);
    }


    //
    // Now transform the rest of the string
    //

    for (i=1; i<S->Length; i++) {

        //
        //  There are export issues that cause us to want to
        //  keep this algorithm simple.  Please don't change it
        //  without checking with JimK first.  Thanks.
        //

        //
        // In order to be compatible with zero terminated unicode strings,
        //  this algorithm is designed to not produce a wide character of
        //  zero as long a the seed is not zero.
        //

        //
        // Simple running XOR with the previous byte and the
        // seed value.
        //

        S->Buffer[i] ^= (S->Buffer[i-1]^(*Seed));

    }


    return;

}


VOID
RtlRunDecodeUnicodeString(
    UCHAR           Seed,
    PUNICODE_STRING String
    )
/*++

Routine Description:

    This function performs the inverse of the function performed
    by RtlRunEncodeUnicodeString().  Please see RtlRunEncodeUnicodeString()
    for details.


Arguments:

    Seed - The seed value to use in RtlRunEncodeUnicodeString().

    String - The string to reveal.


Return Value:

    None - Nothing can really go wrong unless the caller passes bogus
        parameters.  In this case, the caller can catch the access
        violation.


--*/

{

    ULONG
        i;

    PSTRING
        S;

    RTL_PAGED_CODE();

    //
    // Typecast so we can work on bytes rather than WCHARs
    //

    S = (PSTRING)((PVOID)String);


    //
    // Transform the end of the string
    //

    for (i=S->Length; i>1; i--) {

        //
        // a simple running XOR with the previous byte and the
        // seed value.
        //

        S->Buffer[i-1] ^= (S->Buffer[i-2]^Seed);

    }

    //
    // Finally, transform the initial byte
    //

    if (S->Length >= 1) {
        S->Buffer[0] ^= (Seed | 0X43);
    }


    return;
}



VOID
RtlEraseUnicodeString(
    PUNICODE_STRING String
    )
/*++

Routine Description:

    This function scrubs the passed string by over-writing all
    characters in the string.  The entire string (i.e., MaximumLength)
    is erased, not just the current length.


Argumen ts:

    String - The string to be erased.


Return Value:

    None - Nothing can really go wrong unless the caller passes bogus
        parameters.  In this case, the caller can catch the access
        violation.


--*/

{
    RTL_PAGED_CODE();

    if ((String->Buffer == NULL) || (String->MaximumLength == 0)) {
        return;
    }

    RtlZeroMemory( (PVOID)String->Buffer, (ULONG)String->MaximumLength );

    String->Length = 0;

    return;
}



NTSTATUS
RtlAdjustPrivilege(
    ULONG Privilege,
    BOOLEAN Enable,
    BOOLEAN Client,
    PBOOLEAN WasEnabled
    )

/*++

Routine Description:

    This procedure enables or disables a privilege process-wide.

Arguments:

    Privilege - The lower 32-bits of the privilege ID to be enabled or
        disabled.  The upper 32-bits is assumed to be zero.

    Enable - A boolean indicating whether the privilege is to be enabled
        or disabled.  TRUE indicates the privilege is to be enabled.
        FALSE indicates the privilege is to be disabled.

    Client - A boolean indicating whether the privilege should be adjusted
        in a client token or the process's own token.   TRUE indicates
        the client's token should be used (and an error returned if there
        is no client token).  FALSE indicates the process's token should
        be used.

    WasEnabled - points to a boolean to receive an indication of whether
        the privilege was previously enabled or disabled.  TRUE indicates
        the privilege was previously enabled.  FALSE indicates the privilege
        was previoulsy disabled.  This value is useful for returning the
        privilege to its original state after using it.


Return Value:

    STATUS_SUCCESS - The privilege has been sucessfully enabled or disabled.

    STATUS_PRIVILEGE_NOT_HELD - The privilege is not held by the specified context.

    Other status values as may be returned by:

            NtOpenProcessToken()
            NtAdjustPrivilegesToken()


--*/

{
    NTSTATUS
        Status,
        TmpStatus;

    HANDLE
        Token;

    LUID
        LuidPrivilege;

    PTOKEN_PRIVILEGES
        NewPrivileges,
        OldPrivileges;

    ULONG
        Length;

    UCHAR
        Buffer1[sizeof(TOKEN_PRIVILEGES)+
                ((1-ANYSIZE_ARRAY)*sizeof(LUID_AND_ATTRIBUTES))],
        Buffer2[sizeof(TOKEN_PRIVILEGES)+
                ((1-ANYSIZE_ARRAY)*sizeof(LUID_AND_ATTRIBUTES))];


    RTL_PAGED_CODE();

    NewPrivileges = (PTOKEN_PRIVILEGES)Buffer1;
    OldPrivileges = (PTOKEN_PRIVILEGES)Buffer2;

    //
    // Open the appropriate token...
    //

    if (Client == TRUE) {
        Status = NtOpenThreadToken(
                     NtCurrentThread(),
                     TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                     FALSE,
                     &Token
                     );
    } else {

        Status = NtOpenProcessToken(
                     NtCurrentProcess(),
                     TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                     &Token
                    );
    }

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }



    //
    // Initialize the privilege adjustment structure
    //

    LuidPrivilege = RtlConvertUlongToLuid(Privilege);


    NewPrivileges->PrivilegeCount = 1;
    NewPrivileges->Privileges[0].Luid = LuidPrivilege;
    NewPrivileges->Privileges[0].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;



    //
    // Adjust the privilege
    //

    Status = NtAdjustPrivilegesToken(
                 Token,                     // TokenHandle
                 FALSE,                     // DisableAllPrivileges
                 NewPrivileges,             // NewPrivileges
                 sizeof(Buffer1),           // BufferLength
                 OldPrivileges,             // PreviousState (OPTIONAL)
                 &Length                    // ReturnLength
                 );


    TmpStatus = NtClose(Token);
    ASSERT(NT_SUCCESS(TmpStatus));


    //
    // Map the success code NOT_ALL_ASSIGNED to an appropriate error
    // since we're only trying to adjust the one privilege.
    //

    if (Status == STATUS_NOT_ALL_ASSIGNED) {
        Status = STATUS_PRIVILEGE_NOT_HELD;
    }


    if (NT_SUCCESS(Status)) {

        //
        // If there are no privileges in the previous state, there were
        // no changes made. The previous state of the privilege
        // is whatever we tried to change it to.
        //

        if (OldPrivileges->PrivilegeCount == 0) {

            (*WasEnabled) = Enable;

        } else {

            (*WasEnabled) =
                (OldPrivileges->Privileges[0].Attributes & SE_PRIVILEGE_ENABLED)
                ? TRUE : FALSE;
        }
    }

    return(Status);
}


BOOLEAN
RtlValidSid (
    IN PSID Sid
    )

/*++

Routine Description:

    This procedure validates an SID's structure.

Arguments:

    Sid - Pointer to the SID structure to validate.

Return Value:

    BOOLEAN - TRUE if the structure of Sid is valid.

--*/

{
    PISID Isid = (PISID) Sid;
    RTL_PAGED_CODE();
    //
    // Make sure revision is SID_REVISION and sub authority count is not
    // greater than maximum number of allowed sub-authorities.
    //

    try {

        if ( Isid != NULL && (Isid->Revision & 0x0f) == SID_REVISION) {
            if (Isid->SubAuthorityCount <= SID_MAX_SUB_AUTHORITIES) {

                //
                // Verify the memory actually contains the last subauthority
                //
#ifndef NTOS_KERNEL_RUNTIME
#define ProbeAndReadUlongUM(Address) \
        (*(volatile ULONG *)(Address))

                if (Isid->SubAuthorityCount > 0) {
                    ProbeAndReadUlongUM(
                        &Isid->SubAuthority[Isid->SubAuthorityCount-1]
                        );
                }
#endif // !NTOS_KERNEL_RUNTIME
                return TRUE;
          }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }

    return FALSE;

}



BOOLEAN
RtlEqualSid (
    IN PSID Sid1,
    IN PSID Sid2
    )

/*++

Routine Description:

    This procedure tests two SID values for equality.

Arguments:

    Sid1, Sid2 - Supply pointers to the two SID values to compare.
        The SID structures are assumed to be valid.

Return Value:

    BOOLEAN - TRUE if the value of Sid1 is equal to Sid2, and FALSE
        otherwise.

--*/

{
   ULONG SidLength;

   RTL_PAGED_CODE();

   //
   // Make sure they are the same revision
   //

   if ( ((SID *)Sid1)->Revision == ((SID *)Sid2)->Revision ) {

       //
       // Check the SubAuthorityCount first, because it's fast and
       // can help us exit faster.
       //

       if ( *RtlSubAuthorityCountSid( Sid1 ) == *RtlSubAuthorityCountSid( Sid2 )) {

           SidLength = SeLengthSid( Sid1 );
           return( (BOOLEAN)RtlEqualMemory( Sid1, Sid2, SidLength) );
       }
   }

   return( FALSE );

}



BOOLEAN
RtlEqualPrefixSid (
    IN PSID Sid1,
    IN PSID Sid2
    )

/*++

Routine Description:

    This procedure tests two SID prefix values for equality.

    An SID prefix is the entire SID except for the last sub-authority
    value.

Arguments:

    Sid1, Sid2 - Supply pointers to the two SID values to compare.
        The SID structures are assumed to be valid.

Return Value:

    BOOLEAN - TRUE if the prefix value of Sid1 is equal to Sid2, and FALSE
        otherwise.

--*/


{
    LONG Index;

    //
    // Typecast to the opaque SID structures.
    //

    SID *ISid1 = Sid1;
    SID *ISid2 = Sid2;

    RTL_PAGED_CODE();

    //
    // Make sure they are the same revision
    //

    if (ISid1->Revision == ISid2->Revision ) {

        //
        // Compare IdentifierAuthority values
        //

        if ( (ISid1->IdentifierAuthority.Value[0] ==
              ISid2->IdentifierAuthority.Value[0])  &&
             (ISid1->IdentifierAuthority.Value[1]==
              ISid2->IdentifierAuthority.Value[1])  &&
             (ISid1->IdentifierAuthority.Value[2] ==
              ISid2->IdentifierAuthority.Value[2])  &&
             (ISid1->IdentifierAuthority.Value[3] ==
              ISid2->IdentifierAuthority.Value[3])  &&
             (ISid1->IdentifierAuthority.Value[4] ==
              ISid2->IdentifierAuthority.Value[4])  &&
             (ISid1->IdentifierAuthority.Value[5] ==
              ISid2->IdentifierAuthority.Value[5])
            ) {

            //
            // Compare SubAuthorityCount values
            //

            if (ISid1->SubAuthorityCount == ISid2->SubAuthorityCount) {

                if (ISid1->SubAuthorityCount == 0) {
                    return TRUE;
                }

                Index = 0;
                while (Index < (ISid1->SubAuthorityCount - 1)) {
                    if ((ISid1->SubAuthority[Index]) != (ISid2->SubAuthority[Index])) {

                        //
                        // Found some SubAuthority values that weren't equal.
                        //

                        return FALSE;
                    }
                    Index += 1;
                }

                //
                // All SubAuthority values are equal.
                //

                return TRUE;
            }
        }
    }

    //
    // Either the Revision, SubAuthorityCount, or IdentifierAuthority values
    // weren't equal.
    //

    return FALSE;
}



ULONG
RtlLengthRequiredSid (
    IN ULONG SubAuthorityCount
    )

/*++

Routine Description:

    This routine returns the length, in bytes, required to store an SID
    with the specified number of Sub-Authorities.

Arguments:

    SubAuthorityCount - The number of sub-authorities to be stored in the SID.

Return Value:

    ULONG - The length, in bytes, required to store the SID.


--*/

{
    RTL_PAGED_CODE();

    return (8L + (4 * SubAuthorityCount));

}

#ifndef NTOS_KERNEL_RUNTIME

NTSTATUS
RtlAllocateAndInitializeSid(
    IN PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    IN UCHAR SubAuthorityCount,
    IN ULONG SubAuthority0,
    IN ULONG SubAuthority1,
    IN ULONG SubAuthority2,
    IN ULONG SubAuthority3,
    IN ULONG SubAuthority4,
    IN ULONG SubAuthority5,
    IN ULONG SubAuthority6,
    IN ULONG SubAuthority7,
    OUT PSID *Sid
    )

/*++

Routine Description:

    This function allocates and initializes a sid with the specified
    number of sub-authorities (up to 8).  A sid allocated with this
    routine must be freed using RtlFreeSid().

    THIS ROUTINE IS CURRENTLY NOT CALLABLE FROM KERNEL MODE.

Arguments:

    IdentifierAuthority - Pointer to the Identifier Authority value to
        set in the SID.

    SubAuthorityCount - The number of sub-authorities to place in the SID.
        This also identifies how many of the SubAuthorityN parameters
        have meaningful values.  This must contain a value from 0 through
        8.

    SubAuthority0-7 - Provides the corresponding sub-authority value to
        place in the SID.  For example, a SubAuthorityCount value of 3
        indicates that SubAuthority0, SubAuthority1, and SubAuthority0
        have meaningful values and the rest are to be ignored.

    Sid - Receives a pointer to the SID data structure to initialize.

Return Value:

    STATUS_SUCCESS - The SID has been allocated and initialized.

    STATUS_NO_MEMORY - The attempt to allocate memory for the SID
        failed.

    STATUS_INVALID_SID - The number of sub-authorities specified did
        not fall in the valid range for this api (0 through 8).


--*/
{
    PISID ISid;

    RTL_PAGED_CODE();

    if ( SubAuthorityCount > 8 ) {
        return( STATUS_INVALID_SID );
    }

    ISid = RtlAllocateHeap( RtlProcessHeap(), 0,
                            RtlLengthRequiredSid(SubAuthorityCount)
                            );
    if (ISid == NULL) {
        return(STATUS_NO_MEMORY);
    }

    ISid->SubAuthorityCount = (UCHAR)SubAuthorityCount;
    ISid->Revision = 1;
    ISid->IdentifierAuthority = *IdentifierAuthority;

    switch (SubAuthorityCount) {

    case 8:
        ISid->SubAuthority[7] = SubAuthority7;
    case 7:
        ISid->SubAuthority[6] = SubAuthority6;
    case 6:
        ISid->SubAuthority[5] = SubAuthority5;
    case 5:
        ISid->SubAuthority[4] = SubAuthority4;
    case 4:
        ISid->SubAuthority[3] = SubAuthority3;
    case 3:
        ISid->SubAuthority[2] = SubAuthority2;
    case 2:
        ISid->SubAuthority[1] = SubAuthority1;
    case 1:
        ISid->SubAuthority[0] = SubAuthority0;
    case 0:
        ;
    }

    (*Sid) = ISid;
    return( STATUS_SUCCESS );

}
#endif // NTOS_KERNEL_RUNTIME



NTSTATUS
RtlInitializeSid(
    IN PSID Sid,
    IN PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    IN UCHAR SubAuthorityCount
    )
/*++

Routine Description:

    This function initializes an SID data structure.  It does not, however,
    set the sub-authority values.  This must be done separately.

Arguments:

    Sid - Pointer to the SID data structure to initialize.

    IdentifierAuthority - Pointer to the Identifier Authority value to
        set in the SID.

    SubAuthorityCount - The number of sub-authorities that will be placed in
        the SID (a separate action).

Return Value:


--*/
{
    PISID ISid;

    RTL_PAGED_CODE();

    //
    //  Typecast to the opaque SID
    //

    ISid = (PISID)Sid;

    if ( SubAuthorityCount > SID_MAX_SUB_AUTHORITIES ) {
        return( STATUS_INVALID_PARAMETER );
    }

    ISid->SubAuthorityCount = (UCHAR)SubAuthorityCount;
    ISid->Revision = 1;
    ISid->IdentifierAuthority = *IdentifierAuthority;

    return( STATUS_SUCCESS );

}

#ifndef NTOS_KERNEL_RUNTIME

PVOID
RtlFreeSid(
    IN PSID Sid
    )

/*++

Routine Description:

    This function is used to free a SID previously allocated using
    RtlAllocateAndInitializeSid().

    THIS ROUTINE IS CURRENTLY NOT CALLABLE FROM KERNEL MODE.

Arguments:

    Sid - Pointer to the SID to free.

Return Value:

    None.


--*/
{
    RTL_PAGED_CODE();

    if (RtlFreeHeap( RtlProcessHeap(), 0, Sid ))
        return NULL;
    else
        return Sid;
}
#endif // NTOS_KERNEL_RUNTIME


PSID_IDENTIFIER_AUTHORITY
RtlIdentifierAuthoritySid(
    IN PSID Sid
    )
/*++

Routine Description:

    This function returns the address of an SID's IdentifierAuthority field.

Arguments:

    Sid - Pointer to the SID data structure.

Return Value:


--*/
{
    PISID ISid;

    RTL_PAGED_CODE();

    //
    //  Typecast to the opaque SID
    //

    ISid = (PISID)Sid;

    return &(ISid->IdentifierAuthority);

}

PULONG
RtlSubAuthoritySid(
    IN PSID Sid,
    IN ULONG SubAuthority
    )
/*++

Routine Description:

    This function returns the address of a sub-authority array element of
    an SID.

Arguments:

    Sid - Pointer to the SID data structure.

    SubAuthority - An index indicating which sub-authority is being specified.
        This value is not compared against the number of sub-authorities in the
        SID for validity.

Return Value:


--*/
{
    RTL_PAGED_CODE();

    return RtlpSubAuthoritySid( Sid, SubAuthority );
}

PUCHAR
RtlSubAuthorityCountSid(
    IN PSID Sid
    )
/*++

Routine Description:

    This function returns the address of the sub-authority count field of
    an SID.

Arguments:

    Sid - Pointer to the SID data structure.

Return Value:


--*/
{
    PISID ISid;

    RTL_PAGED_CODE();

    //
    //  Typecast to the opaque SID
    //

    ISid = (PISID)Sid;

    return &(ISid->SubAuthorityCount);

}

ULONG
RtlLengthSid (
    IN PSID Sid
    )

/*++

Routine Description:

    This routine returns the length, in bytes, of a structurally valid SID.

Arguments:

    Sid - Points to the SID whose length is to be returned.  The
        SID's structure is assumed to be valid.

Return Value:

    ULONG - The length, in bytes, of the SID.


--*/

{
    RTL_PAGED_CODE();

    return SeLengthSid(Sid);
}


NTSTATUS
RtlCopySid (
    IN ULONG DestinationSidLength,
    OUT PSID DestinationSid,
    IN PSID SourceSid
    )

/*++

Routine Description:

    This routine copies the value of the source SID to the destination
    SID.

Arguments:

    DestinationSidLength - Indicates the length, in bytes, of the
        destination SID buffer.

    DestinationSid - Pointer to a buffer to receive a copy of the
        source Sid value.

    SourceSid - Supplies the Sid value to be copied.

Return Value:

    STATUS_SUCCESS - Indicates the SID was successfully copied.

    STATUS_BUFFER_TOO_SMALL - Indicates the target buffer wasn't
        large enough to receive a copy of the SID.


--*/

{
    ULONG SidLength = SeLengthSid(SourceSid);

    RTL_PAGED_CODE();

    if (SidLength > DestinationSidLength) {

        return STATUS_BUFFER_TOO_SMALL;

    }

    //
    // Buffer is large enough
    //

    RtlMoveMemory( DestinationSid, SourceSid, SidLength );

    return STATUS_SUCCESS;

}


NTSTATUS
RtlCopySidAndAttributesArray (
    IN ULONG ArrayLength,
    IN PSID_AND_ATTRIBUTES Source,
    IN ULONG TargetSidBufferSize,
    OUT PSID_AND_ATTRIBUTES TargetArrayElement,
    OUT PSID TargetSid,
    OUT PSID *NextTargetSid,
    OUT PULONG RemainingTargetSidBufferSize
    )

/*++

Routine Description:

    This routine copies the value of the source SID_AND_ATTRIBUTES array
    to the target.  The actual SID values are placed according to a separate
    parameter.  This allows multiple arrays to be merged using this service
    to copy each.

Arguments:

    ArrayLength - Number of elements in the source array to copy.

    Source - Pointer to the source array.

    TargetSidBufferSize - Indicates the length, in bytes, of the buffer
        to receive the actual SID values.  If this value is less than
        the actual amount needed, then STATUS_BUFFER_TOO_SMALL is returned.

    TargetArrayElement - Indicates where the array elements are to be
        copied to (but not the SID values themselves).

    TargetSid - Indicates where the target SID values s are to be copied.  This
        is assumed to be ULONG aligned.  Each SID value will be copied
        into this buffer.  Each SID will be ULONG aligned.

    NextTargetSid - On completion, will be set to point to the ULONG
        aligned address following the last SID copied.

    RemainingTargetSidBufferSize - On completion, receives an indicatation
        of how much of the SID buffer is still unused.


Return Value:

    STATUS_SUCCESS - The call completed successfully.

    STATUS_BUFFER_TOO_SMALL - Indicates the buffer to receive the SID
        values wasn't large enough.



--*/

{

    ULONG Index = 0;
    PSID NextSid = TargetSid;
    ULONG NextSidLength;
    ULONG AlignedSidLength;
    ULONG RemainingLength = TargetSidBufferSize;

    RTL_PAGED_CODE();

    while (Index < ArrayLength) {

        NextSidLength = SeLengthSid( Source[Index].Sid );
        AlignedSidLength = PtrToUlong(LongAlign(NextSidLength));

        if (NextSidLength > RemainingLength) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        RemainingLength -= AlignedSidLength;

        TargetArrayElement[Index].Sid = NextSid;
        TargetArrayElement[Index].Attributes = Source[Index].Attributes;

        RtlCopySid( NextSidLength, NextSid, Source[Index].Sid );

        NextSid = (PSID)((PCHAR)NextSid + AlignedSidLength);

        Index += 1;

    } //end_while

    (*NextTargetSid) = NextSid;
    (*RemainingTargetSidBufferSize) = RemainingLength;

    return STATUS_SUCCESS;

}



NTSTATUS
RtlLengthSidAsUnicodeString(
    PSID Sid,
    PULONG StringLength
    )

/*++

Routine Description:


    This function returns the maximum length of the string needed
    to represent the SID supplied.  The actual string may be shorter,
    but this is intended to be a quick calculation.

Arguments:


    Sid - Supplies the SID that is to be converted to unicode.

    StringLength - Receives the max length required in bytes.

Return Value:

    SUCCESS - The conversion was successful

    STATUS_INVALID_SID - The sid provided does not have a valid structure,
        or has too many sub-authorities (more than SID_MAX_SUB_AUTHORITIES).

--*/

{
    ULONG   i ;

    PISID   iSid = (PISID)Sid;  // pointer to opaque structure


    RTL_PAGED_CODE();

    if ( RtlValidSid( Sid ) != TRUE)
    {
        return(STATUS_INVALID_SID);
    }

    //
    // if the SID's IA value has 5 or 6 significant bytes, the
    // representation will be in hex, with a 0x preceding.  Otherwise
    // it will be in decimal, with at most 10 characters.
    //

    if (  (iSid->IdentifierAuthority.Value[0] != 0)  ||
          (iSid->IdentifierAuthority.Value[1] != 0)  )
    {
        i = 14 ;    // 0x665544332211

    }
    else
    {
        i = 10 ;    // 4294967295 is the max ulong, at 10 chars
    }

    i += 4 ;        // room for the S-1-

    //
    // for each sub authority, it is a max of 10 chars (for a ulong),
    // plus the - separator
    //

    i += 11 * iSid->SubAuthorityCount ;

    *StringLength = i * sizeof( WCHAR );

    return STATUS_SUCCESS ;

}




NTSTATUS
RtlConvertSidToUnicodeString(
    PUNICODE_STRING UnicodeString,
    PSID Sid,
    BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:


    This function generates a printable unicode string representation
    of a SID.

    The resulting string will take one of two forms.  If the
    IdentifierAuthority value is not greater than 2^32, then
    the SID will be in the form:


        S-1-281736-12-72-9-110
              ^    ^^ ^^ ^ ^^^
              |     |  | |  |
              +-----+--+-+--+---- Decimal



    Otherwise it will take the form:


        S-1-0x173495281736-12-72-9-110
            ^^^^^^^^^^^^^^ ^^ ^^ ^ ^^^
             Hexidecimal    |  | |  |
                            +--+-+--+---- Decimal






Arguments:



    UnicodeString - Returns a unicode string that is equivalent to
        the SID. The maximum length field is only set if
        AllocateDestinationString is TRUE.

    Sid - Supplies the SID that is to be converted to unicode.

    AllocateDestinationString - Supplies a flag that controls whether or
        not this API allocates the buffer space for the destination
        string.  If it does, then the buffer must be deallocated using
        RtlFreeUnicodeString (note that only storage for
        DestinationString->Buffer is allocated by this API).

Return Value:

    SUCCESS - The conversion was successful

    STATUS_INVALID_SID - The sid provided does not have a valid structure,
        or has too many sub-authorities (more than SID_MAX_SUB_AUTHORITIES).

    STATUS_NO_MEMORY - There was not sufficient memory to allocate the
        target string.  This is returned only if AllocateDestinationString
        is specified as TRUE.

    STATUS_BUFFER_OVERFLOW - This is returned only if
        AllocateDestinationString is specified as FALSE.


--*/

{
    NTSTATUS Status;
    WCHAR UniBuffer[ 256 ];
    PWSTR Offset ;
    UNICODE_STRING LocalString ;

    UCHAR   i;
    ULONG   Tmp;
    LARGE_INTEGER Auth ;

    PISID   iSid = (PISID)Sid;  // pointer to opaque structure


    RTL_PAGED_CODE();

    if (RtlValidSid( Sid ) != TRUE) {
        return(STATUS_INVALID_SID);
    }

    if ( iSid->Revision != SID_REVISION )
    {
        return STATUS_INVALID_SID ;
    }

    wcscpy( UniBuffer, L"S-1-" );

    Offset = &UniBuffer[ 4 ];

    if (  (iSid->IdentifierAuthority.Value[0] != 0)  ||
          (iSid->IdentifierAuthority.Value[1] != 0)     ){

        //
        // Ugly hex dump.
        //

        Auth.HighPart = (LONG) (iSid->IdentifierAuthority.Value[ 0 ] << 8) +
                        (LONG) iSid->IdentifierAuthority.Value[ 1 ] ;

        Auth.LowPart = (ULONG)iSid->IdentifierAuthority.Value[5]          +
                       (ULONG)(iSid->IdentifierAuthority.Value[4] <<  8)  +
                       (ULONG)(iSid->IdentifierAuthority.Value[3] << 16)  +
                       (ULONG)(iSid->IdentifierAuthority.Value[2] << 24);

        Status = RtlLargeIntegerToUnicode(
                        &Auth,
                        16,
                        256 - (LONG) (Offset - UniBuffer),
                        Offset );


    } else {

        Tmp = (ULONG)iSid->IdentifierAuthority.Value[5]          +
              (ULONG)(iSid->IdentifierAuthority.Value[4] <<  8)  +
              (ULONG)(iSid->IdentifierAuthority.Value[3] << 16)  +
              (ULONG)(iSid->IdentifierAuthority.Value[2] << 24);

        Status = RtlIntegerToUnicode(
                        Tmp,
                        10,
                        256 - (LONG) (Offset - UniBuffer),
                        Offset );

    }

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }


    for (i=0;i<iSid->SubAuthorityCount ;i++ ) {

        while ( *Offset && ( Offset < &UniBuffer[ 255 ] ) )
        {
            Offset++ ;
        }

        *Offset++ = L'-' ;

        Status = RtlIntegerToUnicode(
                        iSid->SubAuthority[ i ],
                        10,
                        256 - (LONG) (Offset - UniBuffer),
                        Offset );

        if ( !NT_SUCCESS( Status ) )
        {
            return Status ;
        }
    }

    if ( AllocateDestinationString )
    {
        if ( RtlCreateUnicodeString( UnicodeString,
                                         UniBuffer ) )
        {
            Status = STATUS_SUCCESS ;
        }
        else
        {
            Status = STATUS_NO_MEMORY ;
        }

    }
    else
    {

        while ( *Offset && ( Offset < &UniBuffer[ 255 ] ) )
        {
            Offset++ ;
        }

        Tmp = (ULONG) (Offset - UniBuffer) * sizeof( WCHAR );

        if ( Tmp < UnicodeString->MaximumLength )
        {
            LocalString.Length = (USHORT) Tmp ;
            LocalString.MaximumLength = LocalString.Length + sizeof( WCHAR );
            LocalString.Buffer = UniBuffer ;

            RtlCopyUnicodeString(
                        UnicodeString,
                        &LocalString );

            Status = STATUS_SUCCESS ;
        }
        else
        {
            Status = STATUS_BUFFER_OVERFLOW ;
        }

    }

    return(Status);
}




BOOLEAN
RtlEqualLuid (
    IN PLUID Luid1,
    IN PLUID Luid2
    )

/*++

Routine Description:

    This procedure test two LUID values for equality.

    This routine is here for backwards compatibility only. New code
    should use the macro.

Arguments:

    Luid1, Luid2 - Supply pointers to the two LUID values to compare.

Return Value:

    BOOLEAN - TRUE if the value of Luid1 is equal to Luid2, and FALSE
        otherwise.


--*/

{
    LUID UNALIGNED * TempLuid1;
    LUID UNALIGNED * TempLuid2;

    RTL_PAGED_CODE();

    return((Luid1->HighPart == Luid2->HighPart) &&
           (Luid1->LowPart  == Luid2->LowPart));

}


VOID
RtlCopyLuid (
    OUT PLUID DestinationLuid,
    IN PLUID SourceLuid
    )

/*++

Routine Description:

    This routine copies the value of the source LUID to the
    destination LUID.

Arguments:

    DestinationLuid - Receives a copy of the source Luid value.

    SourceLuid - Supplies the Luid value to be copied.  This LUID is
                 assumed to be structurally valid.

Return Value:

    None.

--*/

{
    RTL_PAGED_CODE();

    (*DestinationLuid) = (*SourceLuid);
    return;
}

VOID
RtlCopyLuidAndAttributesArray (
    IN ULONG ArrayLength,
    IN PLUID_AND_ATTRIBUTES Source,
    OUT PLUID_AND_ATTRIBUTES Target
    )

/*++

Routine Description:

    This routine copies the value of the source LUID_AND_ATTRIBUTES array
    to the target.

Arguments:

    ArrayLength - Number of elements in the source array to copy.

    Source - The source array.

    Target - Indicates where the array elements are to be copied to.


Return Value:

    None.


--*/

{

    ULONG Index = 0;

    RTL_PAGED_CODE();

    while (Index < ArrayLength) {

        Target[Index] = Source[Index];

        Index += 1;

    } //end_while


    return;

}

NTSTATUS
RtlCreateSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG Revision
    )

/*++

Routine Description:

    This procedure initializes a new "absolute format" security descriptor.
    After the procedure call the security descriptor is initialized with no
    system ACL, no discretionary ACL, no owner, no primary group and
    all control flags set to false (null).

Arguments:


    SecurityDescriptor - Supplies the security descriptor to
        initialize.

    Revision - Provides the revision level to assign to the security
        descriptor.  This should be one (1) for this release.

Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.

    STATUS_UNKNOWN_REVISION - Indicates the revision level provided
        is not supported by this routine.

--*/

{
    RTL_PAGED_CODE();

    //
    // Check the requested revision
    //

    if (Revision == SECURITY_DESCRIPTOR_REVISION) {

        //
        // Typecast to the opaque SECURITY_DESCRIPTOR structure.
        //

        SECURITY_DESCRIPTOR *ISecurityDescriptor = SecurityDescriptor;

        RtlZeroMemory( ISecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));

        ISecurityDescriptor->Revision = SECURITY_DESCRIPTOR_REVISION;

        return STATUS_SUCCESS;
    }

    return STATUS_UNKNOWN_REVISION;
}


NTSTATUS
RtlCreateSecurityDescriptorRelative (
    IN PISECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG Revision
    )

/*++

Routine Description:

    This procedure initializes a new "relative format" security descriptor.
    After the procedure call the security descriptor is initialized with no
    system ACL, no discretionary ACL, no owner, no primary group and
    all control flags set to false (null).

Arguments:


    SecurityDescriptor - Supplies the security descriptor to
        initialize.

    Revision - Provides the revision level to assign to the security
        descriptor.  This should be one (1) for this release.

Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.

    STATUS_UNKNOWN_REVISION - Indicates the revision level provided
        is not supported by this routine.

Note:
    Warning, this code assume the caller allocated a relative security
    descriptor rather than a relative one.  Absolute is larger on systems
    with 64-bit pointers.

--*/

{
    RTL_PAGED_CODE();

    //
    // Check the requested revision
    //

    if (Revision == SECURITY_DESCRIPTOR_REVISION) {

        //
        // Typecast to the opaque SECURITY_DESCRIPTOR structure.
        //

        RtlZeroMemory( SecurityDescriptor, sizeof(SECURITY_DESCRIPTOR_RELATIVE));

        SecurityDescriptor->Revision = SECURITY_DESCRIPTOR_REVISION;

        return STATUS_SUCCESS;
    }

    return STATUS_UNKNOWN_REVISION;
}


BOOLEAN
RtlValidSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    This procedure validates a SecurityDescriptor's structure.  This
    involves validating the revision levels of each component of the
    security descriptor.

Arguments:

    SecurityDescriptor - Pointer to the SECURITY_DESCRIPTOR structure
        to validate.

Return Value:

    BOOLEAN - TRUE if the structure of SecurityDescriptor is valid.


--*/

{
    PSID Owner;
    PSID Group;
    PACL Dacl;
    PACL Sacl;

    //
    // Typecast to the opaque SECURITY_DESCRIPTOR structure.
    //

    SECURITY_DESCRIPTOR *ISecurityDescriptor = SecurityDescriptor;

    RTL_PAGED_CODE();

    try {

        //
        // known revision ?
        //

        if (ISecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION) {
            return FALSE;
        }


        //
        // Validate each element contained in the security descriptor
        //

        Owner = RtlpOwnerAddrSecurityDescriptor( ISecurityDescriptor );

        if (Owner != NULL) {
            if (!RtlValidSid( Owner )) {
                return FALSE;
            }
        }

        Group = RtlpGroupAddrSecurityDescriptor( ISecurityDescriptor );

        if (Group != NULL) {
            if (!RtlValidSid( Group )) {
                return FALSE;
            }
        }

        Dacl = RtlpDaclAddrSecurityDescriptor( ISecurityDescriptor );
        if (Dacl != NULL ) {

            if (!RtlValidAcl( Dacl )) {
                return FALSE;
            }
        }

        Sacl = RtlpSaclAddrSecurityDescriptor( ISecurityDescriptor );
        if ( Sacl != NULL ) {
            if (!RtlValidAcl( Sacl )) {
                return FALSE;
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }

    //
    // All components are valid
    //

    return TRUE;


}


ULONG
RtlLengthSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
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

    SecurityDescriptor - Points to the SECURITY_DESCRIPTOR whose
        length is to be returned.  The SECURITY_DESCRIPTOR's
        structure is assumed to be valid.

Return Value:

    ULONG - The length, in bytes, of the SECURITY_DESCRIPTOR.


--*/

{
    ULONG sum;
    PVOID Temp;


    //
    // Typecast to the opaque SECURITY_DESCRIPTOR structure.
    //

    SECURITY_DESCRIPTOR *ISecurityDescriptor = (SECURITY_DESCRIPTOR *)SecurityDescriptor;

    RTL_PAGED_CODE();

    //
    // The length is the sum of the following:
    //
    //       SECURITY_DESCRIPTOR_MIN_LENGTH (or sizeof(SECURITY_DESCRIPTOR))
    //       length of Owner SID (if present)
    //       length of Group SID (if present)
    //       length of Discretionary ACL (if present and non-null)
    //       length of System ACL (if present and non-null)
    //

    sum = ISecurityDescriptor->Control & SE_SELF_RELATIVE ?
                            sizeof(SECURITY_DESCRIPTOR_RELATIVE) :
                            sizeof(SECURITY_DESCRIPTOR);

    //
    // Add in length of Owner SID
    //

    Temp = RtlpOwnerAddrSecurityDescriptor(ISecurityDescriptor);
    if (Temp != NULL) {
        sum += LongAlignSize(SeLengthSid(Temp));
    }

    //
    // Add in length of Group SID
    //

    Temp = RtlpGroupAddrSecurityDescriptor(ISecurityDescriptor);
    if (Temp != NULL) {
        sum += LongAlignSize(SeLengthSid(Temp));
    }

    //
    // Add in used length of Discretionary ACL
    //

    Temp = RtlpDaclAddrSecurityDescriptor(ISecurityDescriptor);
    if ( Temp != NULL ) {

        sum += LongAlignSize(((PACL) Temp)->AclSize );
    }

    //
    // Add in used length of System Acl
    //

    Temp = RtlpSaclAddrSecurityDescriptor(ISecurityDescriptor);
    if ( Temp != NULL ) {

        sum += LongAlignSize(((PACL) Temp)->AclSize );
    }

    return sum;
}


ULONG
RtlLengthUsedSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    This routine returns the length, in bytes, in use in a structurally valid
    SECURITY_DESCRIPTOR.

    This is the number of bytes necessary to capture the security descriptor,
    which may be less the the current actual length of the security descriptor
    (RtlLengthSecurityDescriptor() is used to retrieve the actual length).

    Notice that the used length and actual length may differ if either the SACL
    or DACL include padding bytes.

    The length includes the length of all associated data structures (like SIDs
    and ACLs).  The length also takes into account the alignment requirements
    of each component.

    The minimum length of a security descriptor (one which has no associated
    SIDs or ACLs) is SECURITY_DESCRIPTOR_MIN_LENGTH.


Arguments:

    SecurityDescriptor - Points to the SECURITY_DESCRIPTOR whose used
        length is to be returned.  The SECURITY_DESCRIPTOR's
        structure is assumed to be valid.

Return Value:

    ULONG - Number of bytes of the SECURITY_DESCRIPTOR that are in use.


--*/

{
    ULONG sum;

    ACL_SIZE_INFORMATION AclSize;
    PVOID Temp;

    //
    // Typecast to the opaque SECURITY_DESCRIPTOR structure.
    //

    SECURITY_DESCRIPTOR *ISecurityDescriptor = (SECURITY_DESCRIPTOR *)SecurityDescriptor;

    RTL_PAGED_CODE();

    //
    // The length is the sum of the following:
    //
    //       SECURITY_DESCRIPTOR_MIN_LENGTH (or sizeof(SECURITY_DESCRIPTOR))
    //       length of Owner SID (if present)
    //       length of Group SID (if present)
    //       length of Discretionary ACL (if present and non-null)
    //       length of System ACL (if present and non-null)
    //

    sum = ISecurityDescriptor->Control & SE_SELF_RELATIVE ?
                            sizeof(SECURITY_DESCRIPTOR_RELATIVE) :
                            sizeof(SECURITY_DESCRIPTOR);

    //
    // Add in length of Owner SID
    //

    Temp = RtlpOwnerAddrSecurityDescriptor(ISecurityDescriptor);
    if (Temp != NULL) {
        sum += LongAlignSize(SeLengthSid(Temp));
    }

    //
    // Add in length of Group SID
    //

    Temp = RtlpGroupAddrSecurityDescriptor(ISecurityDescriptor);
    if (Temp != NULL) {
        sum += LongAlignSize(SeLengthSid(Temp));
    }

    //
    // Add in used length of Discretionary ACL
    //

    Temp = RtlpDaclAddrSecurityDescriptor(ISecurityDescriptor);
    if ( Temp != NULL ) {

        RtlQueryInformationAcl( Temp,
                                (PVOID)&AclSize,
                                sizeof(AclSize),
                                AclSizeInformation );

        sum += LongAlignSize(AclSize.AclBytesInUse);
    }

    //
    // Add in used length of System Acl
    //

    Temp = RtlpSaclAddrSecurityDescriptor(ISecurityDescriptor);
    if ( Temp != NULL ) {

        RtlQueryInformationAcl( Temp,
                                (PVOID)&AclSize,
                                sizeof(AclSize),
                                AclSizeInformation );

        sum += LongAlignSize(AclSize.AclBytesInUse);
    }

    return sum;
}



NTSTATUS
RtlSetAttributesSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL Control,
    OUT PULONG Revision
    )
{
    RTL_PAGED_CODE();

    //
    // Always return the revision value - even if this isn't a valid
    // security descriptor
    //

    *Revision = ((SECURITY_DESCRIPTOR *)SecurityDescriptor)->Revision;

    if ( ((SECURITY_DESCRIPTOR *)SecurityDescriptor)->Revision
         != SECURITY_DESCRIPTOR_REVISION ) {
        return STATUS_UNKNOWN_REVISION;
    }

    // BUGBUG: This is a worthless API.  There is no way to turn any of the bits off.
    // Use the newer RtlSetControlSecurityDescriptor.
    Control &= SE_VALID_CONTROL_BITS;
    return RtlSetControlSecurityDescriptor ( SecurityDescriptor, Control, Control );
}



NTSTATUS
RtlGetControlSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR_CONTROL Control,
    OUT PULONG Revision
    )

/*++

Routine Description:

    This procedure retrieves the control information from a security descriptor.

Arguments:

    SecurityDescriptor - Supplies the security descriptor.

    Control - Receives the control information.

    Revision - Receives the revision of the security descriptor.
               This value will always be returned, even if an error
               is returned by this routine.

Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.

    STATUS_UNKNOWN_REVISION - Indicates the revision of the security
        descriptor is not known to the routine.  It may be a newer
        revision than the routine knows about.


--*/

{
    RTL_PAGED_CODE();

    //
    // Always return the revision value - even if this isn't a valid
    // security descriptor
    //

    *Revision = ((SECURITY_DESCRIPTOR *)SecurityDescriptor)->Revision;


    if ( ((SECURITY_DESCRIPTOR *)SecurityDescriptor)->Revision
         != SECURITY_DESCRIPTOR_REVISION ) {
        return STATUS_UNKNOWN_REVISION;
    }


    *Control = ((SECURITY_DESCRIPTOR *)SecurityDescriptor)->Control;

    return STATUS_SUCCESS;

}

NTSTATUS
RtlSetControlSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
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
    //
    // Ensure the caller passed valid bits.
    //

    if ( (ControlBitsOfInterest & ~SE_VALID_CONTROL_BITS) != 0 ||
         (ControlBitsToSet & ~ControlBitsOfInterest) != 0 ) {
        return STATUS_INVALID_PARAMETER;
    }

    ((SECURITY_DESCRIPTOR *)pSecurityDescriptor)->Control &= ~ControlBitsOfInterest;
    ((SECURITY_DESCRIPTOR *)pSecurityDescriptor)->Control |= ControlBitsToSet;

    return STATUS_SUCCESS;
}


NTSTATUS
RtlSetDaclSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN BOOLEAN DaclPresent,
    IN PACL Dacl OPTIONAL,
    IN BOOLEAN DaclDefaulted OPTIONAL
    )

/*++

Routine Description:

    This procedure sets the discretionary ACL information of an absolute
    format security descriptor.  If there is already a discretionary ACL
    present in the security descriptor, it is superseded.

Arguments:

    SecurityDescriptor - Supplies the security descriptor to be which
        the discretionary ACL is to be added.

    DaclPresent - If FALSE, indicates the DaclPresent flag in the
        security descriptor should be set to FALSE.  In this case,
        the remaining optional parameters are ignored.  Otherwise,
        the DaclPresent control flag in the security descriptor is
        set to TRUE and the remaining optional parameters are not
        ignored.

    Dacl - Supplies the discretionary ACL for the security
        descriptor.  If this optional parameter is not passed, then a
        null ACL is assigned to the security descriptor.  A null
        discretionary ACL unconditionally grants access.  The ACL is
        referenced by, not copied into, by the security descriptor.

    DaclDefaulted - When set, indicates the discretionary ACL was
        picked up from some default mechanism (rather than explicitly
        specified by a user).  This value is set in the DaclDefaulted
        control flag in the security descriptor.  If this optional
        parameter is not passed, then the DaclDefaulted flag will be
        cleared.

Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.

    STATUS_UNKNOWN_REVISION - Indicates the revision of the security
        descriptor is not known to the routine.  It may be a newer
        revision than the routine knows about.

    STATUS_INVALID_SECURITY_DESCR - Indicates the security descriptor
        is not an absolute format security descriptor.


--*/

{

    //
    // Typecast to the opaque SECURITY_DESCRIPTOR structure.
    //

    SECURITY_DESCRIPTOR *ISecurityDescriptor = SecurityDescriptor;

    RTL_PAGED_CODE();

    //
    // Check the revision
    //

    if (ISecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION) {
       return STATUS_UNKNOWN_REVISION;
    }

    //
    // Make sure the descriptor is absolute format
    //

    if (ISecurityDescriptor->Control & SE_SELF_RELATIVE) {
        return STATUS_INVALID_SECURITY_DESCR;
    }

    //
    // Assign the DaclPresent flag value passed
    //


    if (DaclPresent) {

        ISecurityDescriptor->Control |= SE_DACL_PRESENT;

        //
        // Assign the ACL address if passed, otherwise set to null.
        //

        ISecurityDescriptor->Dacl = NULL;
        if (ARGUMENT_PRESENT(Dacl)) {
            ISecurityDescriptor->Dacl = Dacl;
        }




        //
        // Assign DaclDefaulted flag if passed, otherwise clear it.
        //

        ISecurityDescriptor->Control &= ~SE_DACL_DEFAULTED;
        if (DaclDefaulted == TRUE) {
            ISecurityDescriptor->Control |= SE_DACL_DEFAULTED;
        }
    } else {

        ISecurityDescriptor->Control &= ~SE_DACL_PRESENT;

    }


    return STATUS_SUCCESS;

}


NTSTATUS
RtlGetDaclSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PBOOLEAN DaclPresent,
    OUT PACL *Dacl,
    OUT PBOOLEAN DaclDefaulted
    )

/*++

Routine Description:

    This procedure retrieves the discretionary ACL information of a
    security descriptor.

Arguments:

    SecurityDescriptor - Supplies the security descriptor.

    DaclPresent - If TRUE, indicates that the security descriptor
        does contain a discretionary ACL.  In this case, the
        remaining OUT parameters will receive valid values.
        Otherwise, the security descriptor does not contain a
        discretionary ACL and the remaining OUT parameters will not
        receive valid values.

    Dacl - This value is returned only if the value returned for the
        DaclPresent flag is TRUE.  In this case, the Dacl parameter
        receives the address of the security descriptor's
        discretionary ACL.  If this value is returned as null, then
        the security descriptor has a null discretionary ACL.

    DaclDefaulted - This value is returned only if the value returned
        for the DaclPresent flag is TRUE.  In this case, the
        DaclDefaulted parameter receives the value of the security
        descriptor's DaclDefaulted control flag.

Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.

    STATUS_UNKNOWN_REVISION - Indicates the revision of the security
        descriptor is not known to the routine.  It may be a newer
        revision than the routine knows about.


--*/

{
    //
    // Typecast to the opaque SECURITY_DESCRIPTOR structure.
    //

    SECURITY_DESCRIPTOR *ISecurityDescriptor = SecurityDescriptor;

    RTL_PAGED_CODE();

    //
    // Check the revision
    //

    if (ISecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION) {
        return STATUS_UNKNOWN_REVISION;
    }

    //
    // Assign the DaclPresent flag value
    //

    *DaclPresent = RtlpAreControlBitsSet( ISecurityDescriptor, SE_DACL_PRESENT );

    if (*DaclPresent) {

        //
        // Assign the ACL address.
        //

        *Dacl = RtlpDaclAddrSecurityDescriptor(ISecurityDescriptor);

        //
        // Assign DaclDefaulted flag.
        //

        *DaclDefaulted = RtlpAreControlBitsSet( ISecurityDescriptor, SE_DACL_DEFAULTED );
    }

    return STATUS_SUCCESS;

}


NTSTATUS
RtlSetSaclSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN BOOLEAN SaclPresent,
    IN PACL Sacl OPTIONAL,
    IN BOOLEAN SaclDefaulted OPTIONAL
    )

/*++

Routine Description:

    This procedure sets the system ACL information of an absolute security
    descriptor.  If there is already a system ACL present in the
    security descriptor, it is superseded.

Arguments:

    SecurityDescriptor - Supplies the security descriptor to be which
        the system ACL is to be added.

    SaclPresent - If FALSE, indicates the SaclPresent flag in the
        security descriptor should be set to FALSE.  In this case,
        the remaining optional parameters are ignored.  Otherwise,
        the SaclPresent control flag in the security descriptor is
        set to TRUE and the remaining optional parameters are not
        ignored.

    Sacl - Supplies the system ACL for the security descriptor.  If
        this optional parameter is not passed, then a null ACL is
        assigned to the security descriptor.  The ACL is referenced
        by, not copied into, by the security descriptor.

    SaclDefaulted - When set, indicates the system ACL was picked up
        from some default mechanism (rather than explicitly specified
        by a user).  This value is set in the SaclDefaulted control
        flag in the security descriptor.  If this optional parameter
        is not passed, then the SaclDefaulted flag will be cleared.

Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.

    STATUS_UNKNOWN_REVISION - Indicates the revision of the security
        descriptor is not known to the routine.  It may be a newer
        revision than the routine knows about.

    STATUS_INVALID_SECURITY_DESCR - Indicates the security descriptor
        is not an absolute format security descriptor.


--*/

{

    //
    // Typecast to the opaque SECURITY_DESCRIPTOR structure.
    //

    SECURITY_DESCRIPTOR *ISecurityDescriptor = SecurityDescriptor;

    RTL_PAGED_CODE();

    //
    // Check the revision
    //

    if (ISecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION) {
        return STATUS_UNKNOWN_REVISION;
    }

    //
    // Make sure the descriptor is absolute format
    //

    if (ISecurityDescriptor->Control & SE_SELF_RELATIVE) {
        return STATUS_INVALID_SECURITY_DESCR;
    }

    //
    // Assign the SaclPresent flag value passed
    //


    if (SaclPresent) {

        ISecurityDescriptor->Control |= SE_SACL_PRESENT;

        //
        // Assign the ACL address if passed, otherwise set to null.
        //

        ISecurityDescriptor->Sacl = NULL;
        if (ARGUMENT_PRESENT(Sacl)) {
           ISecurityDescriptor->Sacl = Sacl;
        }

        //
        // Assign SaclDefaulted flag if passed, otherwise clear it.
        //

        ISecurityDescriptor->Control &= ~ SE_SACL_DEFAULTED;
        if (ARGUMENT_PRESENT(SaclDefaulted)) {
            ISecurityDescriptor->Control |= SE_SACL_DEFAULTED;
        }
    } else {

        ISecurityDescriptor->Control &= ~SE_SACL_PRESENT;
    }

    return STATUS_SUCCESS;

}


NTSTATUS
RtlGetSaclSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PBOOLEAN SaclPresent,
    OUT PACL *Sacl,
    OUT PBOOLEAN SaclDefaulted
    )

/*++

Routine Description:

    This procedure retrieves the system ACL information of a security
    descriptor.

Arguments:

    SecurityDescriptor - Supplies the security descriptor.

    SaclPresent - If TRUE, indicates that the security descriptor
        does contain a system ACL.  In this case, the remaining OUT
        parameters will receive valid values.  Otherwise, the
        security descriptor does not contain a system ACL and the
        remaining OUT parameters will not receive valid values.

    Sacl - This value is returned only if the value returned for the
        SaclPresent flag is TRUE.  In this case, the Sacl parameter
        receives the address of the security descriptor's system ACL.
        If this value is returned as null, then the security
        descriptor has a null system ACL.

    SaclDefaulted - This value is returned only if the value returned
        for the SaclPresent flag is TRUE.  In this case, the
        SaclDefaulted parameter receives the value of the security
        descriptor's SaclDefaulted control flag.

Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.

    STATUS_UNKNOWN_REVISION - Indicates the revision of the security
        descriptor is not known to the routine.  It may be a newer
        revision than the routine knows about.


--*/

{

    //
    // Typecast to the opaque SECURITY_DESCRIPTOR structure.
    //

    SECURITY_DESCRIPTOR *ISecurityDescriptor = SecurityDescriptor;

    RTL_PAGED_CODE();

    //
    // Check the revision
    //

    if (ISecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION) {
        return STATUS_UNKNOWN_REVISION;
    }

    //
    // Assign the SaclPresent flag value
    //

    *SaclPresent = RtlpAreControlBitsSet( ISecurityDescriptor, SE_SACL_PRESENT );

    if (*SaclPresent) {

        //
        // Assign the ACL address.
        //

        *Sacl = RtlpSaclAddrSecurityDescriptor(ISecurityDescriptor);

        //
        // Assign SaclDefaulted flag.
        //

        *SaclDefaulted = RtlpAreControlBitsSet( ISecurityDescriptor, SE_SACL_DEFAULTED );

    }

    return STATUS_SUCCESS;

}


NTSTATUS
RtlSetOwnerSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID Owner OPTIONAL,
    IN BOOLEAN OwnerDefaulted OPTIONAL
    )

/*++

Routine Description:

    This procedure sets the owner information of an absolute security
    descriptor.  If there is already an owner present in the security
    descriptor, it is superseded.

Arguments:

    SecurityDescriptor - Supplies the security descriptor in which
        the owner is to be set.  If the security descriptor already
        includes an owner, it will be superseded by the new owner.

    Owner - Supplies the owner SID for the security descriptor.  If
        this optional parameter is not passed, then the owner is
        cleared (indicating the security descriptor has no owner).
        The SID is referenced by, not copied into, the security
        descriptor.

    OwnerDefaulted - When set, indicates the owner was picked up from
        some default mechanism (rather than explicitly specified by a
        user).  This value is set in the OwnerDefaulted control flag
        in the security descriptor.  If this optional parameter is
        not passed, then the SaclDefaulted flag will be cleared.

Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.

    STATUS_UNKNOWN_REVISION - Indicates the revision of the security
        descriptor is not known to the routine.  It may be a newer
        revision than the routine knows about.

    STATUS_INVALID_SECURITY_DESCR - Indicates the security descriptor
        is not an absolute format security descriptor.


--*/

{

    //
    // Typecast to the opaque SECURITY_DESCRIPTOR structure.
    //

    SECURITY_DESCRIPTOR *ISecurityDescriptor = SecurityDescriptor;

    RTL_PAGED_CODE();

    //
    // Check the revision
    //

    if (ISecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION) {
        return STATUS_UNKNOWN_REVISION;
    }

    //
    // Make sure the descriptor is absolute format
    //

    if (ISecurityDescriptor->Control & SE_SELF_RELATIVE) {
        return STATUS_INVALID_SECURITY_DESCR;
    }

    //
    // Assign the Owner field if passed, otherwise clear it.
    //

    ISecurityDescriptor->Owner = NULL;
    if (ARGUMENT_PRESENT(Owner)) {
        ISecurityDescriptor->Owner = Owner;
    }

    //
    // Assign the OwnerDefaulted flag if passed, otherwise clear it.
    //

    ISecurityDescriptor->Control &= ~SE_OWNER_DEFAULTED;
    if (OwnerDefaulted == TRUE) {
        ISecurityDescriptor->Control |= SE_OWNER_DEFAULTED;
    }

    return STATUS_SUCCESS;

}


NTSTATUS
RtlGetOwnerSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSID *Owner,
    OUT PBOOLEAN OwnerDefaulted
    )

/*++

Routine Description:

    This procedure retrieves the owner information of a security
    descriptor.

Arguments:

    SecurityDescriptor - Supplies the security descriptor.

    Owner - Receives a pointer to the owner SID.  If the security
        descriptor does not currently contain an owner, then this
        value will be returned as null.  In this case, the remaining
        OUT parameters are not given valid return values.  Otherwise,
        this parameter points to an SID and the remaining OUT
        parameters are provided valid return values.

    OwnerDefaulted - This value is returned only if the value
        returned for the Owner parameter is not null.  In this case,
        the OwnerDefaulted parameter receives the value of the
        security descriptor's OwnerDefaulted control flag.

Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.

    STATUS_UNKNOWN_REVISION - Indicates the revision of the security
        descriptor is not known to the routine.  It may be a newer
        revision than the routine knows about.


--*/

{

    //
    // Typecast to the opaque SECURITY_DESCRIPTOR structure.
    //

    SECURITY_DESCRIPTOR *ISecurityDescriptor = SecurityDescriptor;

    RTL_PAGED_CODE();

    //
    // Check the revision
    //

    if (ISecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION) {
        return STATUS_UNKNOWN_REVISION;
    }

    //
    // Return the Owner field value.
    //

    *Owner = RtlpOwnerAddrSecurityDescriptor(ISecurityDescriptor);

    //
    // Return the OwnerDefaulted flag value.
    //

    *OwnerDefaulted = RtlpAreControlBitsSet( ISecurityDescriptor, SE_OWNER_DEFAULTED );

    return STATUS_SUCCESS;

}


NTSTATUS
RtlSetGroupSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID Group OPTIONAL,
    IN BOOLEAN GroupDefaulted OPTIONAL
    )

/*++

Routine Description:

    This procedure sets the primary group information of an absolute security
    descriptor.  If there is already an primary group present in the
    security descriptor, it is superseded.

Arguments:

    SecurityDescriptor - Supplies the security descriptor in which
        the primary group is to be set.  If the security descriptor
        already includes a primary group, it will be superseded by
        the new group.

    Group - Supplies the primary group SID for the security
        descriptor.  If this optional parameter is not passed, then
        the primary group is cleared (indicating the security
        descriptor has no primary group).  The SID is referenced by,
        not copied into, the security descriptor.

    GroupDefaulted - When set, indicates the owner was picked up from
        some default mechanism (rather than explicitly specified by a
        user).  This value is set in the OwnerDefaulted control flag
        in the security descriptor.  If this optional parameter is
        not passed, then the SaclDefaulted flag will be cleared.

Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.

    STATUS_UNKNOWN_REVISION - Indicates the revision of the security
        descriptor is not known to the routine.  It may be a newer
        revision than the routine knows about.

    STATUS_INVALID_SECURITY_DESCR - Indicates the security descriptor
        is not an absolute format security descriptor.


--*/

{

    //
    // Typecast to the opaque SECURITY_DESCRIPTOR structure.
    //

    SECURITY_DESCRIPTOR *ISecurityDescriptor = SecurityDescriptor;

    RTL_PAGED_CODE();

    //
    // Check the revision
    //

    if (ISecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION) {
        return STATUS_UNKNOWN_REVISION;
    }

    //
    // Make sure the descriptor is absolute format
    //

    if (ISecurityDescriptor->Control & SE_SELF_RELATIVE) {
        return STATUS_INVALID_SECURITY_DESCR;
    }

    //
    // Assign the Group field if passed, otherwise clear it.
    //

    ISecurityDescriptor->Group = NULL;
    if (ARGUMENT_PRESENT(Group)) {
        ISecurityDescriptor->Group = Group;
    }

    //
    // Assign the GroupDefaulted flag if passed, otherwise clear it.
    //

    ISecurityDescriptor->Control &= ~SE_GROUP_DEFAULTED;
    if (ARGUMENT_PRESENT(GroupDefaulted)) {
        ISecurityDescriptor->Control |= SE_GROUP_DEFAULTED;
    }

    return STATUS_SUCCESS;

}


NTSTATUS
RtlGetGroupSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSID *Group,
    OUT PBOOLEAN GroupDefaulted
    )

/*++

Routine Description:

    This procedure retrieves the primary group information of a
    security descriptor.

Arguments:

    SecurityDescriptor - Supplies the security descriptor.

    Group - Receives a pointer to the primary group SID.  If the
        security descriptor does not currently contain a primary
        group, then this value will be returned as null.  In this
        case, the remaining OUT parameters are not given valid return
        values.  Otherwise, this parameter points to an SID and the
        remaining OUT parameters are provided valid return values.

    GroupDefaulted - This value is returned only if the value
        returned for the Group parameter is not null.  In this case,
        the GroupDefaulted parameter receives the value of the
        security descriptor's GroupDefaulted control flag.

Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.

    STATUS_UNKNOWN_REVISION - Indicates the revision of the security
        descriptor is not known to the routine.  It may be a newer
        revision than the routine knows about.


--*/

{

    //
    // Typecast to the opaque SECURITY_DESCRIPTOR structure.
    //

    SECURITY_DESCRIPTOR *ISecurityDescriptor =
        (SECURITY_DESCRIPTOR *)SecurityDescriptor;

    RTL_PAGED_CODE();

    //
    // Check the revision
    //

    if (ISecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION) {
        return STATUS_UNKNOWN_REVISION;
    }

    //
    // Return the Group field value.
    //

    *Group = RtlpGroupAddrSecurityDescriptor(ISecurityDescriptor);

    //
    // Return the GroupDefaulted flag value.
    //

    *GroupDefaulted = RtlpAreControlBitsSet( ISecurityDescriptor, SE_GROUP_DEFAULTED );

    return STATUS_SUCCESS;

}


BOOLEAN
RtlAreAllAccessesGranted(
    IN ACCESS_MASK GrantedAccess,
    IN ACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This routine is used to check a desired access mask against a
    granted access mask.  It is used by the Object Management
    component when dereferencing a handle.

Arguments:

        GrantedAccess - Specifies the granted access mask.

        DesiredAccess - Specifies the desired access mask.

Return Value:

    BOOLEAN - TRUE if the GrantedAccess mask has all the bits set
        that the DesiredAccess mask has set.  That is, TRUE is
        returned if all of the desired accesses have been granted.

--*/

{
    RTL_PAGED_CODE();

    return ((BOOLEAN)((~(GrantedAccess) & (DesiredAccess)) == 0));
}


BOOLEAN
RtlAreAnyAccessesGranted(
    IN ACCESS_MASK GrantedAccess,
    IN ACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This routine is used to test whether any of a set of desired
    accesses are granted by a granted access mask.  It is used by
    components other than the the Object Management component for
    checking access mask subsets.

Arguments:

        GrantedAccess - Specifies the granted access mask.

        DesiredAccess - Specifies the desired access mask.

Return Value:

    BOOLEAN - TRUE if the GrantedAccess mask contains any of the bits
        specified in the DesiredAccess mask.  That is, if any of the
        desired accesses have been granted, TRUE is returned.


--*/

{
    RTL_PAGED_CODE();

    return ((BOOLEAN)(((GrantedAccess) & (DesiredAccess)) != 0));
}


VOID
RtlMapGenericMask(
    IN OUT PACCESS_MASK AccessMask,
    IN PGENERIC_MAPPING GenericMapping
    )

/*++

Routine Description:

    This routine maps all generic accesses in the provided access mask
    to specific and standard accesses according to the provided
    GenericMapping.

Arguments:

        AccessMask - Points to the access mask to be mapped.

        GenericMapping - The mapping of generic to specific and standard
                         access types.

Return Value:

    None.

--*/

{
    RTL_PAGED_CODE();

//    //
//    // Make sure the pointer is properly aligned
//    //
//
//    ASSERT( ((ULONG)AccessMask >> 2) << 2 == (ULONG)AccessMask );

    if (*AccessMask & GENERIC_READ) {

        *AccessMask |= GenericMapping->GenericRead;
    }

    if (*AccessMask & GENERIC_WRITE) {

        *AccessMask |= GenericMapping->GenericWrite;
    }

    if (*AccessMask & GENERIC_EXECUTE) {

        *AccessMask |= GenericMapping->GenericExecute;
    }

    if (*AccessMask & GENERIC_ALL) {

        *AccessMask |= GenericMapping->GenericAll;
    }

    //
    // Now clear the generic flags
    //

    *AccessMask &= ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);

    return;
}

NTSTATUS
RtlImpersonateSelf(
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel
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

    STATUS_SUCCESS -  The thread is now impersonating the calling process.

    Other - Status values returned by:

            NtOpenProcessToken()
            NtDuplicateToken()
            NtSetInformationThread()

--*/

{
    NTSTATUS
        Status,
        IgnoreStatus;

    HANDLE
        Token1,
        Token2;

    OBJECT_ATTRIBUTES
        ObjectAttributes;

    SECURITY_QUALITY_OF_SERVICE
        Qos;


    RTL_PAGED_CODE();

    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, 0, NULL);

    Qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    Qos.ImpersonationLevel = ImpersonationLevel;
    Qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    Qos.EffectiveOnly = FALSE;
    ObjectAttributes.SecurityQualityOfService = &Qos;

    Status = NtOpenProcessToken( NtCurrentProcess(), TOKEN_DUPLICATE, &Token1 );

    if (NT_SUCCESS(Status)) {
        Status = NtDuplicateToken(
                     Token1,
                     TOKEN_IMPERSONATE,
                     &ObjectAttributes,
                     FALSE,                 //EffectiveOnly
                     TokenImpersonation,
                     &Token2
                     );
        if (NT_SUCCESS(Status)) {
            Status = NtSetInformationThread(
                         NtCurrentThread(),
                         ThreadImpersonationToken,
                         &Token2,
                         sizeof(HANDLE)
                         );

            IgnoreStatus = NtClose( Token2 );
        }


        IgnoreStatus = NtClose( Token1 );
    }


    return(Status);

}

#ifndef WIN16


#ifndef NTOS_KERNEL_RUNTIME

BOOLEAN
RtlpValidOwnerSubjectContext(
    IN HANDLE Token,
    IN PSID Owner,
    IN BOOLEAN ServerObject,
    OUT PNTSTATUS ReturnStatus
    )
/*++

Routine Description:

    This routine checks to see whether the provided SID is one the subject
    is authorized to assign as the owner of objects.

Arguments:

    Token - Points to the subject's effective token

    Owner - Points to the SID to be checked.

    ServerObject - Boolean indicating whether or not this is a server
       object, meaning it is protected by a primary-client combination.

    ReturnStatus - Status to be passed back to the caller on failure.

Return Value:

    FALSE on failure.

--*/

{
    NTSTATUS Status;

    ULONG Index;
    BOOLEAN Found;
    ULONG ReturnLength;
    PTOKEN_GROUPS GroupIds = NULL;
    PTOKEN_USER UserId = NULL;
    PVOID HeapHandle;
    HANDLE TokenToUse;

    BOOLEAN HasPrivilege;
    PRIVILEGE_SET PrivilegeSet;

    RTL_PAGED_CODE();

    //
    // Get the handle to the current process heap
    //

    if ( Owner == NULL ) {
        *ReturnStatus = STATUS_INVALID_OWNER;
        return(FALSE);
    }

    //
    // If it's not a server object, check the owner against the contents of the
    // client token.  If it is a server object, the owner must be valid in the
    // primary token.
    //

    if (!ServerObject) {

        TokenToUse = Token;

    } else {

        *ReturnStatus = NtOpenProcessToken(
                            NtCurrentProcess(),
                            TOKEN_QUERY,
                            &TokenToUse
                            );

        if (!NT_SUCCESS( *ReturnStatus )) {
            return( FALSE );
        }
    }

    HeapHandle = RtlProcessHeap();

    //
    //  Get the User from the Token
    //

    *ReturnStatus = NtQueryInformationToken(
                         TokenToUse,
                         TokenUser,
                         UserId,
                         0,
                         &ReturnLength
                         );

    if (!NT_SUCCESS( *ReturnStatus ) && (STATUS_BUFFER_TOO_SMALL != *ReturnStatus)) {
        if (ServerObject) {
            NtClose( TokenToUse );
        }
        return( FALSE );

    }

    UserId = RtlAllocateHeap( HeapHandle, 0, ReturnLength );

    if (UserId == NULL) {

        *ReturnStatus = STATUS_NO_MEMORY;
        if (ServerObject) {
            NtClose( TokenToUse );
        }

        return( FALSE );
    }

    *ReturnStatus = NtQueryInformationToken(
                         TokenToUse,
                         TokenUser,
                         UserId,
                         ReturnLength,
                         &ReturnLength
                         );

    if (!NT_SUCCESS( *ReturnStatus )) {
        RtlFreeHeap( HeapHandle, 0, (PVOID)UserId );
        if (ServerObject) {
            NtClose( TokenToUse );
        }
        return( FALSE );
    }

    if ( RtlEqualSid( Owner, UserId->User.Sid ) ) {

        RtlFreeHeap( HeapHandle, 0, (PVOID)UserId );
        if (ServerObject) {
            NtClose( TokenToUse );
        }
        return( TRUE );
    }

    RtlFreeHeap( HeapHandle, 0, (PVOID)UserId );

    //
    // Get the groups from the Token
    //

    *ReturnStatus = NtQueryInformationToken(
                         TokenToUse,
                         TokenGroups,
                         GroupIds,
                         0,
                         &ReturnLength
                         );

    if (!NT_SUCCESS( *ReturnStatus ) && (STATUS_BUFFER_TOO_SMALL != *ReturnStatus)) {

        if (ServerObject) {
            NtClose( TokenToUse );
        }
        return( FALSE );
    }

    GroupIds = RtlAllocateHeap( HeapHandle, 0, ReturnLength );

    if (GroupIds == NULL) {

        *ReturnStatus = STATUS_NO_MEMORY;
        if (ServerObject) {
            NtClose( TokenToUse );
        }
        return( FALSE );
    }

    *ReturnStatus = NtQueryInformationToken(
                         TokenToUse,
                         TokenGroups,
                         GroupIds,
                         ReturnLength,
                         &ReturnLength
                         );

    if (ServerObject) {
        NtClose( TokenToUse );
    }

    if (!NT_SUCCESS( *ReturnStatus )) {
        RtlFreeHeap( HeapHandle, 0, GroupIds );
        return( FALSE );
    }

    //
    //  Walk through the list of group IDs looking for a match to
    //  the specified SID.  If one is found, make sure it may be
    //  assigned as an owner.
    //
    //  This code is similar to that performed to set the default
    //  owner of a token (NtSetInformationToken).
    //

    Index = 0;
    while (Index < GroupIds->GroupCount) {

        Found = RtlEqualSid(
                    Owner,
                    GroupIds->Groups[Index].Sid
                    );

        if ( Found ) {

            if ( RtlpIdAssignableAsOwner(GroupIds->Groups[Index])) {

                RtlFreeHeap( HeapHandle, 0, GroupIds );
                return TRUE;

            } else {

                break;

            } //endif assignable

        }  //endif Found

        Index++;

    } //endwhile

    RtlFreeHeap( HeapHandle, 0, GroupIds );

    //
    // If we are going to fail this call, check for Restore privilege,
    // and succeed if he has it.
    //

    //
    // Check for appropriate Privileges
    //
    // Audit/Alarm messages need to be generated due to the attempt
    // to perform a privileged operation.
    //

    PrivilegeSet.PrivilegeCount = 1;
    PrivilegeSet.Control = PRIVILEGE_SET_ALL_NECESSARY;
    PrivilegeSet.Privilege[0].Luid = RtlConvertLongToLuid(SE_RESTORE_PRIVILEGE);
    PrivilegeSet.Privilege[0].Attributes = 0;

    Status = NtPrivilegeCheck(
                Token,
                &PrivilegeSet,
                &HasPrivilege
                );

    if (!NT_SUCCESS( Status )) {
        HasPrivilege = FALSE;
    }

    if ( HasPrivilege ) {
        return TRUE;
    } else {
        *ReturnStatus = STATUS_INVALID_OWNER;
        return FALSE;
    }
}
#endif // NTOS_KERNEL_RUNTIME

#endif  // WIN16





VOID
RtlpApplyAclToObject (
    IN PACL Acl,
    IN PGENERIC_MAPPING GenericMapping
    )

/*++

Routine Description:

    This is a private routine that maps Access Masks of an ACL so that
    they are applicable to the object type the ACL is being applied to.

    Only known DSA ACEs are mapped.  Unknown ACE types are ignored.

    Only access types in the GenericAll mapping for the target object
    type will be non-zero upon return.

Arguments:

    Acl - Supplies the acl being applied.

    GenericMapping - Specifies the generic mapping to use.


Return Value:

    None.

--*/

{
    ULONG i;

    PACE_HEADER Ace;

    RTL_PAGED_CODE();

    //
    //  First check if the acl is null
    //

    if (Acl == NULL) {

        return;

    }


    //
    // Now walk the ACL, mapping each ACE as we go.
    //

    for (i = 0, Ace = FirstAce(Acl);
         i < Acl->AceCount;
         i += 1, Ace = NextAce(Ace)) {

        if (IsMSAceType( Ace )) {

            RtlApplyAceToObject( Ace, GenericMapping );
        }

    }

    return;
}


BOOLEAN
RtlpCopyEffectiveAce (
    IN PACE_HEADER OldAce,
    IN BOOLEAN AutoInherit,
    IN BOOLEAN WillGenerateInheritAce,
    IN PSID ClientOwnerSid,
    IN PSID ClientGroupSid,
    IN PSID ServerOwnerSid OPTIONAL,
    IN PSID ServerGroupSid OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping,
    IN GUID *NewObjectType OPTIONAL,
    IN OUT PVOID *AcePosition,
    OUT PULONG NewAceLength,
    OUT PACL NewAcl,
    OUT PBOOLEAN ObjectAceInherited OPTIONAL,
    OUT PBOOLEAN EffectiveAceMapped,
    OUT PBOOLEAN AclOverflowed
    )

/*++

Routine Description:

    This routine copy a specified ACE into an ACL as an effective ACE.
    The resultant ACE has all the inheritance bits turned of.
    The resultant ACE has the SID mapped from a generic SID to a specific SID
    (e.g., From "creator owner" to the passed in owner sid).

Arguments:

    OldAce - Supplies the ace being inherited

    AutoInherit - Specifies if the inheritance is an "automatic inheritance".
        As such, the inherited ACEs will be marked as such.

    WillGenerateInheritAce - Specifies if the caller intends to generate an
        inheritable ACE the corresponds to OldAce.  If TRUE, this routine will
        try to not map the effective ACE (increasing the likelyhood that
        EffectiveAceMapped will return FALSE),

    ClientOwnerSid - Specifies the owner Sid to use

    ClientGroupSid - Specifies the new Group Sid to use

    ServerSid - Optionally specifies the Server Sid to use in compound ACEs.

    ClientSid - Optionally specifies the Client Sid to use in compound ACEs.

    GenericMapping - Specifies the generic mapping to use

    NewObjectType - Type of object being inherited to.  If not specified,
        the object has no object type.

    AcePosition - On entry and exit, specifies location of the next available ACE
        position in NewAcl.
        A NULL ACE position means there is no room at all in NewAcl.

    NewAceLength - Returns the length (in bytes) needed in NewAcl to
        copy the specified ACE. This might be zero to indicate that the ACE
        need not be copied at all.

    NewAcl - Provides a pointer to the ACL into which the ACE is to be
        inherited.

    ObjectAceInherited - Returns true if one or more object ACEs were inherited
        based on NewObjectType
        If NULL, NewObjectType is ignored and the object ACE is always inherited

    EffectiveAceMapped - Return TRUE if the SID, guid, or access mask of Old Ace
        was modifed when copying the ACE.

    AclOverflowed - Returns TRUE if NewAcl wasn't long enough to contain NewAceLength.

Return Value:

    TRUE - No problem was detected.
    FALSE - Indicates something went wrong preventing
        the ACE from being compied.  This generally represents a bugcheck
        situation when returned from this call.

--*/
{
    ULONG LengthRequired;
    ACCESS_MASK LocalMask;

    PSID LocalServerOwner;
    PSID LocalServerGroup;

    ULONG CreatorSid[CREATOR_SID_SIZE];

    SID_IDENTIFIER_AUTHORITY  CreatorSidAuthority = SECURITY_CREATOR_SID_AUTHORITY;


    RTL_PAGED_CODE();

    //
    // Allocate and initialize the universal SIDs we're going to need
    // to look for inheritable ACEs.
    //

    ASSERT(RtlLengthRequiredSid( 1 ) == CREATOR_SID_SIZE);
    RtlInitializeSid( (PSID)CreatorSid, &CreatorSidAuthority, 1 );
    *(RtlpSubAuthoritySid( (PSID)CreatorSid, 0 )) = SECURITY_CREATOR_OWNER_RID;

    LocalServerOwner = ARGUMENT_PRESENT(ServerOwnerSid) ? ServerOwnerSid : ClientOwnerSid;
    LocalServerGroup = ARGUMENT_PRESENT(ServerGroupSid) ? ServerGroupSid : ClientGroupSid;


    //
    // Initialization
    //
    *EffectiveAceMapped = FALSE;
    if ( ARGUMENT_PRESENT(ObjectAceInherited)) {
        *ObjectAceInherited = FALSE;
    }
    *AclOverflowed = FALSE;
    LengthRequired = (ULONG)OldAce->AceSize;

    //
    // Process all MS ACE types specially
    //

    if ( IsMSAceType(OldAce) ) {
        ULONG Rid;
        PSID SidToCopy = NULL;
        ULONG AceHeaderToCopyLength;
        PACE_HEADER AceHeaderToCopy = OldAce;
        PSID ServerSidToCopy = NULL;

        UCHAR DummyAce[sizeof(KNOWN_OBJECT_ACE)+sizeof(GUID)];

        //
        // Grab the Sid pointer and access mask as a function of the ACE type
        //
        if (IsKnownAceType( OldAce ) ) {
            SidToCopy = &((PKNOWN_ACE)OldAce)->SidStart;
            AceHeaderToCopyLength = FIELD_OFFSET(KNOWN_ACE, SidStart);

        } else if (IsCompoundAceType(OldAce)) {

            SidToCopy = RtlCompoundAceClientSid( OldAce );
            AceHeaderToCopyLength = FIELD_OFFSET(KNOWN_COMPOUND_ACE, SidStart);
            ASSERT( FIELD_OFFSET(KNOWN_COMPOUND_ACE, Mask) ==
                    FIELD_OFFSET(KNOWN_ACE, Mask) );

            //
            // Compound ACEs have two SIDs (Map one now).
            //
            ServerSidToCopy = RtlCompoundAceServerSid( OldAce );

            if (RtlEqualPrefixSid ( ServerSidToCopy, CreatorSid )) {

                Rid = *RtlpSubAuthoritySid( ServerSidToCopy, 0 );
                switch (Rid) {
                case SECURITY_CREATOR_OWNER_RID:
                    ServerSidToCopy = ClientOwnerSid;
                    LengthRequired = LengthRequired - CREATOR_SID_SIZE + SeLengthSid(ClientOwnerSid);
                    *EffectiveAceMapped = TRUE;
                    break;
                case SECURITY_CREATOR_GROUP_RID:
                    if ( ClientGroupSid != NULL ) {
                        ServerSidToCopy = ClientGroupSid;
                        LengthRequired = LengthRequired - CREATOR_SID_SIZE + SeLengthSid(ClientGroupSid);
                        *EffectiveAceMapped = TRUE;
                    }
                    break;
                case SECURITY_CREATOR_OWNER_SERVER_RID:
                    ServerSidToCopy = LocalServerOwner;
                    LengthRequired = LengthRequired - CREATOR_SID_SIZE + SeLengthSid(LocalServerOwner);
                    *EffectiveAceMapped = TRUE;
                    break;
                case SECURITY_CREATOR_GROUP_SERVER_RID:
                    ServerSidToCopy = LocalServerGroup;
                    LengthRequired = LengthRequired - CREATOR_SID_SIZE + SeLengthSid(LocalServerGroup);
                    *EffectiveAceMapped = TRUE;
                    break;
                }

                //
                // If we don't know what this SID is, just copy the original.
                //
                if ( !*EffectiveAceMapped ) {
                    AceHeaderToCopyLength += SeLengthSid( ServerSidToCopy );
                    ServerSidToCopy = NULL;
                }

            } else {
                //
                // We don't know what this SID is, just copy the original.
                //
                AceHeaderToCopyLength += SeLengthSid( ServerSidToCopy );
                ServerSidToCopy = NULL;
            }

        //
        // Handle Object ACEs
        //
        } else {
            GUID *InheritedObjectType;

            SidToCopy = RtlObjectAceSid( OldAce );
            AceHeaderToCopyLength = (ULONG) ((PUCHAR)SidToCopy - (PUCHAR)OldAce);
            ASSERT( FIELD_OFFSET(KNOWN_OBJECT_ACE, Mask) ==
                    FIELD_OFFSET(KNOWN_ACE, Mask) );

            //
            // Handle ACEs that are only inherited for a specific object type,
            //
            InheritedObjectType = RtlObjectAceInheritedObjectType( OldAce );
            if ( ARGUMENT_PRESENT(ObjectAceInherited) && InheritedObjectType != NULL ) {

                //
                // If the object type doesn't match the inherited object type,
                //  don't inherit the ACE.
                //

                if ( NewObjectType == NULL ||
                     !RtlEqualMemory( InheritedObjectType,
                                      NewObjectType,
                                      sizeof(GUID) ) ) {

                    LengthRequired = 0;

                //
                // If the object type matches the inherited object type,
                //  Inherit an ACE with no inherited object type.
                //

                } else {

                    //
                    // Tell the caller we inherited an object type specific ACE.
                    //

                    *ObjectAceInherited = TRUE;

                    //
                    // If the caller is not going to generate an inheritable ACE,
                    //  deleting the inherited object type GUID for the effective ACE.
                    //
                    // Otherwise, leave it so the caller can merge the two ACEs.
                    //

                    if ( !WillGenerateInheritAce ) {
                        *EffectiveAceMapped = TRUE;

                        //
                        // If an object type GUID is present,
                        //  simply delete the inherited object type GUID.
                        //
                        if ( RtlObjectAceObjectTypePresent( OldAce )) {
                            LengthRequired -= sizeof(GUID);
                            AceHeaderToCopyLength -= sizeof(GUID);
                            RtlCopyMemory( DummyAce, OldAce, AceHeaderToCopyLength );

                            AceHeaderToCopy = (PACE_HEADER)DummyAce;
                            ((PKNOWN_OBJECT_ACE)AceHeaderToCopy)->Flags &= ~ACE_INHERITED_OBJECT_TYPE_PRESENT;


                        //
                        // If an object type GUID is not present,
                        //  convert the ACE to non-object type specific.
                        //
                        } else {
                            AceHeaderToCopyLength = AceHeaderToCopyLength -
                                             sizeof(GUID) +
                                             sizeof(KNOWN_ACE) -
                                             sizeof(KNOWN_OBJECT_ACE);
                            LengthRequired = LengthRequired -
                                             sizeof(GUID) +
                                             sizeof(KNOWN_ACE) -
                                             sizeof(KNOWN_OBJECT_ACE);

                            RtlCopyMemory( DummyAce, OldAce, AceHeaderToCopyLength );
                            AceHeaderToCopy = (PACE_HEADER)DummyAce;

                            AceHeaderToCopy->AceType = RtlBaseAceType[ OldAce->AceType ];

                        }
                    }
                }

            }
        }

        //
        // Only proceed if we've not already determined to drop the ACE.
        //

        if ( LengthRequired != 0 ) {

            //
            // If after mapping the access mask, the access mask
            // is empty, then drop the ACE.
            //
            // This is incompatible with NT 4.0 which simply mapped and left
            //  undefined access bits set.

            LocalMask = ((PKNOWN_ACE)(OldAce))->Mask;
            RtlApplyGenericMask( OldAce, &LocalMask, GenericMapping);

            if ( LocalMask != ((PKNOWN_ACE)(OldAce))->Mask ) {
                *EffectiveAceMapped = TRUE;
            }

            //
            // Mask off any bits that aren't meaningful
            //

            LocalMask &= ( STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL | ACCESS_SYSTEM_SECURITY );

            if (LocalMask == 0) {

                LengthRequired = 0;

            } else {

                //
                // See if the SID in the ACE is one of the various CREATOR_* SIDs by
                // comparing identifier authorities.
                //

                if (RtlEqualPrefixSid ( SidToCopy, CreatorSid )) {

                    Rid = *RtlpSubAuthoritySid( SidToCopy, 0 );

                    switch (Rid) {
                    case SECURITY_CREATOR_OWNER_RID:
                        SidToCopy = ClientOwnerSid;
                        LengthRequired = LengthRequired - CREATOR_SID_SIZE + SeLengthSid(ClientOwnerSid);
                        *EffectiveAceMapped = TRUE;
                        break;
                    case SECURITY_CREATOR_GROUP_RID:
                        if ( ClientGroupSid != NULL ) {
                            SidToCopy = ClientGroupSid;
                            LengthRequired = LengthRequired - CREATOR_SID_SIZE + SeLengthSid(ClientGroupSid);
                            *EffectiveAceMapped = TRUE;
                        }
                        break;
                    case SECURITY_CREATOR_OWNER_SERVER_RID:
                        SidToCopy = LocalServerOwner;
                        LengthRequired = LengthRequired - CREATOR_SID_SIZE + SeLengthSid(LocalServerOwner);
                        *EffectiveAceMapped = TRUE;
                        break;
                    case SECURITY_CREATOR_GROUP_SERVER_RID:
                        SidToCopy = LocalServerGroup;
                        LengthRequired = LengthRequired - CREATOR_SID_SIZE + SeLengthSid(LocalServerGroup);
                        *EffectiveAceMapped = TRUE;
                        break;
                    default :
                        //
                        // We don't know what this SID is, just copy the original.
                        //
                        break;
                    }
                }

                //
                // If the ACE doesn't fit,
                //  just note the fact and don't copy the ACE.
                //

                if ( *AcePosition == NULL ||
                     LengthRequired > (ULONG)NewAcl->AclSize - ((PUCHAR)(*AcePosition) - (PUCHAR)NewAcl) ) {
                    *AclOverflowed = TRUE;
                } else {

                    PUCHAR Target;

                    //
                    // Copy individual parts of the ACE separately.
                    //

                    Target = (PUCHAR)*AcePosition;

                    RtlCopyMemory(
                        Target,
                        AceHeaderToCopy,
                        AceHeaderToCopyLength );

                    Target += AceHeaderToCopyLength;

                    //
                    // Now copy the correct server SID
                    //

                    if ( ServerSidToCopy != NULL ) {
                        RtlCopyMemory(
                            Target,
                            ServerSidToCopy,
                            SeLengthSid(ServerSidToCopy)
                            );
                        Target += SeLengthSid(ServerSidToCopy);
                    }

                    //
                    // Now copy the correct SID
                    //

                    RtlCopyMemory(
                        Target,
                        SidToCopy,
                        SeLengthSid(SidToCopy)
                        );
                    Target += SeLengthSid(SidToCopy);

                    //
                    // Set the size of the ACE accordingly
                    //

                    if ( LengthRequired < (ULONG)(Target - (PUCHAR)*AcePosition) ) {
                        return FALSE;
                    }
                    LengthRequired = (ULONG)(Target - (PUCHAR)*AcePosition);
                    ((PKNOWN_ACE)*AcePosition)->Header.AceSize =
                        (USHORT)LengthRequired;


                    //
                    // Put the mapped access mask in the new ACE
                    //

                    ((PKNOWN_ACE)*AcePosition)->Mask = LocalMask;

                }
            }
        }

    } else {

        //
        // If the ACE doesn't fit,
        //  just note the fact and don't copy the ACE.
        //

        if ( LengthRequired > (ULONG)NewAcl->AclSize - ((PUCHAR)*AcePosition - (PUCHAR)NewAcl) ) {
            *AclOverflowed = TRUE;
        } else {

            //
            // Not a known ACE type, copy ACE as is
            //

            RtlCopyMemory(
                *AcePosition,
                OldAce,
                LengthRequired );
         }
    }

    //
    // If the ACE was actually kept, clear all the inherit flags
    // and update the ACE count of the ACL.
    //

    if ( !*AclOverflowed && LengthRequired != 0 ) {
        ((PACE_HEADER)*AcePosition)->AceFlags &= ~VALID_INHERIT_FLAGS;
        if ( AutoInherit ) {
            ((PACE_HEADER)*AcePosition)->AceFlags |= INHERITED_ACE;
        }
        NewAcl->AceCount += 1;
    }

    //
    // We have the length of the new ACE, but we've calculated
    // it with a ULONG.  It must fit into a USHORT.  See if it
    // does.
    //

    if (LengthRequired > 0xFFFF) {
        return FALSE;
    }

    //
    // Move the Ace Position to where the next ACE goes.
    //
    if ( !*AclOverflowed ) {
        *AcePosition = ((PUCHAR)*AcePosition) + LengthRequired;
    }

    //
    //  Now return to our caller
    //

    (*NewAceLength) = LengthRequired;

    return TRUE;
}

#ifndef WIN16

NTSTATUS
RtlpCopyAces(
    IN PACL Acl,
    IN PGENERIC_MAPPING GenericMapping,
    IN ACE_TYPE_TO_COPY AceTypeToCopy,
    IN UCHAR AceFlagsToReset,
    IN BOOLEAN MapSids,
    IN PSID ClientOwnerSid,
    IN PSID ClientGroupSid,
    IN PSID ServerOwnerSid OPTIONAL,
    IN PSID ServerGroupSid OPTIONAL,
    OUT PULONG NewAclSizeParam,
    OUT PACL NewAcl
    )

/*++

Routine Description:

    Copy ACEs from of an ACL and perform generic mapping.  Only ACEs specified
    by 'AceFilter' are copied.

Arguments:

    Acl - Supplies the ACL to copy from.

    GenericMapping - Specifies the generic mapping to use.

    AceTypeToCopy - Describes which aces to copy.

    AceFlagsToReset - Bit mask of ACE flags to reset (if set) on each ACE.

    MapSids - TRUE if the SID in the ACE is to be mapped to the corresponding
        actual SID.

    ClientOwnerSid - Specifies the owner Sid to use

    ClientGroupSid - Specifies the new Group Sid to use

    ServerOwnerSid - Optionally specifies the Server Sid to use in compound ACEs.

    ServerGroupSid - Optionally specifies the Server group Sid to use in compound ACEs.

    NewAclSizeParam - Receives the cumulatiave length of the copies ACEs

    NewAcl - Provides a pointer to the ACL to copy to.
        This ACL must already be initialized.


Return Value:

    STATUS_SUCCESS - An inheritable ACL has been generated.

    STATUS_BUFFER_TOO_SMALL - The ACL specified by NewAcl is too small for the
        copied ACEs.  The required size is returned in NewAceLength.


--*/

{

    NTSTATUS Status;
    ULONG i;

    PACE_HEADER OldAce;
    ULONG NewAclSize, NewAceSize;
    BOOLEAN AclOverflowed = FALSE;
    BOOLEAN CopyAce;
    PVOID AcePosition;


    RTL_PAGED_CODE();

    //
    // Validate the ACL.
    //

    if ( !ValidAclRevision(NewAcl) ) {
        return STATUS_UNKNOWN_REVISION;
    }

    //
    // Find where the first ACE goes.
    //

    if (!RtlFirstFreeAce( NewAcl, &AcePosition )) {
        return STATUS_BAD_INHERITANCE_ACL;
    }

    //
    // Walk through the original ACL copying ACEs.
    //

    NewAclSize = 0;
    for (i = 0, OldAce = FirstAce(Acl);
         i < Acl->AceCount;
         i += 1, OldAce = NextAce(OldAce)) {

        //
        // If the ACE wasn't inherited,
        //  copy it.
        //

        switch (AceTypeToCopy) {
        case CopyInheritedAces:
            CopyAce = AceInherited(OldAce);
            break;
        case CopyNonInheritedAces:
            CopyAce = !AceInherited(OldAce);
            break;
        case CopyAllAces:
            CopyAce = TRUE;
            break;
        default:
            CopyAce = FALSE;
            break;
        }

        if ( CopyAce ) {


            //
            // If SIDs are to be mapped,
            //  do so (and potentially create up to two ACEs).
            //

            if ( MapSids ) {
                PVOID TempAcePosition;
                ULONG EffectiveAceSize = 0;

                BOOLEAN EffectiveAceMapped;
                BOOLEAN GenerateInheritAce;

                //
                // Remember where the next ACE will be copied.
                //

                TempAcePosition = AcePosition;
                NewAceSize = 0;
                GenerateInheritAce =
                    (((PACE_HEADER)OldAce)->AceFlags & (OBJECT_INHERIT_ACE|CONTAINER_INHERIT_ACE)) != 0;


                //
                // If the orginal ACE is an effective ACE,
                //  create an effective ACE.
                //

                if ( !(((PACE_HEADER)OldAce)->AceFlags & INHERIT_ONLY_ACE)) {
                    BOOLEAN LocalAclOverflowed;

                    //
                    // Copy the effective ACE into the ACL.
                    //
                    if ( !RtlpCopyEffectiveAce (
                                    OldAce,
                                    FALSE,  // Don't set the auto inherit bit
                                    GenerateInheritAce,
                                    ClientOwnerSid,
                                    ClientGroupSid,
                                    ServerOwnerSid,
                                    ServerGroupSid,
                                    GenericMapping,
                                    NULL,   // Always copy object ACES
                                    &TempAcePosition,
                                    &EffectiveAceSize,
                                    NewAcl,
                                    NULL,   // Always copy object ACES
                                    &EffectiveAceMapped,
                                    &LocalAclOverflowed ) ) {

                        return STATUS_BAD_INHERITANCE_ACL;
                    }

                    if (LocalAclOverflowed) {
                        AclOverflowed = TRUE;
                    }
                    NewAceSize += EffectiveAceSize;

                    //
                    // Reset any undesirable AceFlags.
                    //

                    if ( !AclOverflowed ) {
                        ((PACE_HEADER)AcePosition)->AceFlags &= ~AceFlagsToReset;
                    }

                }

                //
                // If the original ACE is inheritable,
                //  create an inheritable ACE.
                //
                // ASSERT: AcePosition points to where the effective ACE was copied
                // ASSERT: TempAcePosition points to where the inheritable ACE should be copied
                //

                if ( GenerateInheritAce ) {

                    //
                    // If a effective ACE was created,
                    //  and it wasn't mapped,
                    //  avoid generating another ACE and simply merge the inheritance bits into
                    //      the effective ACE.
                    //

                    if ( EffectiveAceSize != 0 && !EffectiveAceMapped ) {

                       //
                       // Copy the inherit bits from the original ACE.
                       //
                       if ( !AclOverflowed ) {
                            ((PACE_HEADER)AcePosition)->AceFlags |=
                                ((PACE_HEADER)OldAce)->AceFlags & (VALID_INHERIT_FLAGS);
                            ((PACE_HEADER)AcePosition)->AceFlags &= ~AceFlagsToReset;
                       }


                    //
                    // Otherwise, generate an explicit inheritance ACE.
                    //
                    // But only if the access mask isn't zero.
                    //

                    } else if ( !IsMSAceType(OldAce) || ((PKNOWN_ACE)(OldAce))->Mask != 0 ) {

                        //
                        // Account for the new ACE being added to the ACL.
                        //
                        NewAceSize += (ULONG)(((PACE_HEADER)OldAce)->AceSize);

                        if (NewAceSize > 0xFFFF) {
                            return STATUS_BAD_INHERITANCE_ACL;
                        }

                        //
                        // If the ACE doesn't fit,
                        //  just note the fact and don't copy the ACE.
                        //

                        if ( ((PACE_HEADER)OldAce)->AceSize > NewAcl->AclSize - ((PUCHAR)TempAcePosition - (PUCHAR)NewAcl) ) {
                            AclOverflowed = TRUE;
                        } else {

                            //
                            // copy it as is, but make sure the InheritOnly bit is set.
                            //

                            if ( !AclOverflowed ) {
                                RtlCopyMemory(
                                    TempAcePosition,
                                    OldAce,
                                    ((PACE_HEADER)OldAce)->AceSize
                                    );

                                ((PACE_HEADER)TempAcePosition)->AceFlags |= INHERIT_ONLY_ACE;
                                ((PACE_HEADER)TempAcePosition)->AceFlags &= ~AceFlagsToReset;
                                NewAcl->AceCount += 1;
                            }
                        }
                    }

                }

            } else {
                NewAceSize = (ULONG)OldAce->AceSize;

                //
                // If the ACE doesn't fit,
                //  just note the fact and don't copy the ACE.
                //

                if ( AcePosition == NULL ||
                     NewAceSize > (ULONG)NewAcl->AclSize - ((PUCHAR)AcePosition - (PUCHAR)NewAcl) ) {
                    AclOverflowed = TRUE;
                } else if ( !AclOverflowed ) {


                    //
                    // Copy the ACE.
                    //

                    RtlCopyMemory(
                        AcePosition,
                        OldAce,
                        NewAceSize );

                    //
                    // Map the generic bits.
                    //
                    // Is it really right to map the generic bits on an ACE
                    // that's both effective and inheritable.  Shouldn't this
                    // be split into two ACEs in that case?  Or just skip the mapping?
                    //
                    if (IsMSAceType( AcePosition )) {
                        RtlApplyAceToObject( (PACE_HEADER)AcePosition, GenericMapping );
                    }

                    //
                    // Reset any undesirable AceFlags.
                    //

                    ((PACE_HEADER)AcePosition)->AceFlags &= ~AceFlagsToReset;

                    //
                    // Account for the new ACE.
                    //

                    NewAcl->AceCount += 1;
                }
            }


            //
            // Move the Ace Position to where the next ACE goes.
            //
            if ( !AclOverflowed ) {
                AcePosition = ((PUCHAR)AcePosition) + NewAceSize;
            } else {
                // On overflow, ensure no other ACEs are actually output to the buffer
                AcePosition = ((PUCHAR)NewAcl) + NewAcl->AclSize;
            }
            NewAclSize += NewAceSize;

        }
    }


    //
    // We have the length of the new ACE, but we've calculated
    // it with a ULONG.  It must fit into a USHORT.  See if it
    // does.
    //

    if (NewAclSize > 0xFFFF) {
        return STATUS_BAD_INHERITANCE_ACL;
    }

    (*NewAclSizeParam) = NewAclSize;

    return AclOverflowed ? STATUS_BUFFER_TOO_SMALL : STATUS_SUCCESS;

}


NTSTATUS
RtlpInheritAcl2 (
    IN PACL DirectoryAcl,
    IN PACL ChildAcl,
    IN ULONG ChildGenericControl,
    IN BOOLEAN IsDirectoryObject,
    IN BOOLEAN AutoInherit,
    IN BOOLEAN DefaultDescriptorForObject,
    IN PSID OwnerSid,
    IN PSID GroupSid,
    IN PSID ServerOwnerSid OPTIONAL,
    IN PSID ServerGroupSid OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOLEAN IsSacl,
    IN GUID *NewObjectType OPTIONAL,
    IN PULONG AclBufferSize,
    IN OUT PUCHAR AclBuffer,
    OUT PBOOLEAN NewAclExplicitlyAssigned,
    OUT PULONG NewGenericControl
    )

/*++

Routine Description:

    This is a private routine that produces an inherited acl from
    a parent acl according to the rules of inheritance

Arguments:

    DirectoryAcl - Supplies the acl being inherited.

    ChildAcl - Supplies the acl associated with the object.  This
        is either the current acl on the object or the acl being assigned
        to the object.

    ChildGenericControl - Specifies the control flags from the SecurityDescriptor
        describing the ChildAcl:

        SEP_ACL_PRESENT: Specifies that the child ACL is explictly supplied by
            the caller.

        SEP_ACL_DEFAULTED: Specifies that the child ACL was supplied by some
            defaulting mechanism.

        SEP_ACL_PROTECTED: Specifies that the child ACL is protected and
            should not inherit any ACE from the DirectoryACL

    IsDirectoryObject - Specifies if the new acl is for a directory.

    AutoInherit - Specifies if the inheritance is an "automatic inheritance".
        As such, the non-inherited ACEs from the ChildAcl will be preserved and
        the inherited ACEs from the DirectoryAcl will be marked as such.

    DefaultDescriptorForObject - If set, the CreatorDescriptor
        is the default descriptor for ObjectType.  As such, the
        CreatorDescriptor will be ignored if any ObjectType specific
        ACEs are inherited from the parent.  If not such ACEs are inherited,
        the CreatorDescriptor is handled as though this flag were not
        specified.

    OwnerSid - Specifies the owner Sid to use.

    GroupSid - Specifies the group SID to use.

    GenericMapping - Specifies the generic mapping to use.

    IsSacl - True if this is the SACL.  False if this is the DACL.

    NewObjectType - Type of object being inherited to.  If not specified,
        the object has no object type.

    AclBufferSize - On input, specifies the size of AclBuffer.
        On output, on success, returns the used size of AclBuffer.
        On output, if the buffer is too small, returns the required size of AclBuffer.

    AclBuffer - Receives a pointer to the new (inherited) acl.

    NewAclExplicitlyAssigned - Returns true to indicate that some portion of
        "NewAcl" was derived from an the explicit ChildAcl

    NewGenericControl - Specifies the control flags for the newly
        generated ACL.

        SEP_ACL_AUTO_INHERITED: Set if the ACL was generated using the
            Automatic Inheritance algorithm.

        SEP_ACL_PROTECTED: Specifies that the ACL is protected and
            was not inherited from the parent ACL.

Return Value:

    STATUS_SUCCESS - An inheritable ACL was successfully generated.

    STATUS_NO_INHERITANCE - An inheritable ACL was not successfully generated.
        This is a warning completion status.  The caller should use the default
        ACL.

    STATUS_BAD_INHERITANCE_ACL - Indicates the acl built was not a valid ACL.
        This can becaused by a number of things.  One of the more probable
        causes is the replacement of a CreatorId with an SID that didn't fit
        into the ACE or ACL.

    STATUS_UNKNOWN_REVISION - Indicates the source ACL is a revision that
        is unknown to this routine.

    STATUS_BUFFER_TOO_SMALL - The ACL specified by NewAcl is too small for the
        inheritance ACEs.  The required size is returned in AclBufferSize.

--*/

{
    NTSTATUS Status;
    ULONG ChildNewAclSize = 0;
    ULONG UsedChildNewAclSize = 0;
    ULONG DirectoryNewAclSize = 0;
    ULONG AclRevision;
    USHORT ChildAceCount;
    PVOID ChildAcePosition;
    PVOID DirectoryAcePosition;
    BOOLEAN AclOverflowed = FALSE;
    BOOLEAN AclProtected = FALSE;
    BOOLEAN NullAclOk = TRUE;
    BOOLEAN ObjectAceInherited;

    RTL_PAGED_CODE();


    //
    // Assume the ACL revision.
    //

    AclRevision = ACL_REVISION;
    RtlCreateAcl( (PACL)AclBuffer, *AclBufferSize, AclRevision );
    *NewAclExplicitlyAssigned = FALSE;
    *NewGenericControl = AutoInherit ? SEP_ACL_AUTO_INHERITED : 0;

    //
    // If the a current child ACL is not defaulted,
    //  the non-inherited ACEs from the current child ACL are to be preserved.
    //

    if ( (ChildGenericControl & SEP_ACL_DEFAULTED) == 0 ) {

        //
        // The resultant ACL should be protected if the input ACL is
        //  protected.
        //

        if ( ChildGenericControl & SEP_ACL_PROTECTED ) {
            AclProtected = TRUE;
            *NewGenericControl |= SEP_ACL_PROTECTED;
        }

        //
        // Only copy ACEs if the child ACL is actually present.
        //
        if ( (ChildGenericControl & (SEP_ACL_PRESENT|SEP_ACL_PROTECTED)) != 0 ) {


            if ( ChildAcl != NULL ) {
                ACE_TYPE_TO_COPY AceTypeToCopy;
                UCHAR AceFlagsToReset;
                BOOLEAN MapSids;


                AclRevision = max( AclRevision, ChildAcl->AclRevision );

                //
                // Since we're explicitly using the ACL specified by the caller,
                //  we never want to return a NULL ACL.
                //  Rather, if we have an ACL with no ACEs,
                //  we'll return exactly that.  For a DACL, that results
                //  in a DACL that grants no access rather than a DACL
                //  that grants all access.
                //

                NullAclOk = FALSE;

                //
                // If the caller doesn't understand auto inheritance,
                //  simply preserve the specified ACL 100% intact.
                //
                if ( !AutoInherit ) {

                    AceTypeToCopy = CopyAllAces;
                    AceFlagsToReset = 0;      // Don't turn off any ACE Flags
                    MapSids = FALSE;          // For backward compatibility

                //
                // If the child is protected,
                //  keep all of the ACEs turning off the INHERITED ACE flags.
                //
                } else if ( ChildGenericControl & SEP_ACL_PROTECTED ) {

                    AceTypeToCopy = CopyAllAces;
                    AceFlagsToReset = INHERITED_ACE; // Turn off all INHERITED_ACE flags
                    MapSids = TRUE;

                //
                // If the child is not protected,
                //  just copy the non-inherited ACEs.
                //
                // (The inherited ACEs will be recomputed from the parent.)
                //
                } else {

                    AceTypeToCopy = CopyNonInheritedAces;
                    AceFlagsToReset = 0;      // Don't turn off any ACE Flags
                    MapSids = TRUE;

                }

                //
                // Copy the requested ACEs.
                //

                Status = RtlpCopyAces(
                            ChildAcl,
                            GenericMapping,
                            AceTypeToCopy,
                            AceFlagsToReset,
                            MapSids,
                            OwnerSid,
                            GroupSid,
                            ServerOwnerSid,
                            ServerGroupSid,
                            &ChildNewAclSize,
                            (PACL)AclBuffer );

                UsedChildNewAclSize = ChildNewAclSize;
                if ( Status == STATUS_BUFFER_TOO_SMALL ) {
                    AclOverflowed = TRUE;
                    Status = STATUS_SUCCESS;
                }

                if ( !NT_SUCCESS(Status) ) {
                    return Status;
                }

                //
                // If this ACL might be ignored later,
                //  remember the current state of the ACL.
                //

                if ( DefaultDescriptorForObject && ChildNewAclSize != 0 ) {
                    ChildAceCount = ((PACL)AclBuffer)->AceCount;

                    if (!RtlFirstFreeAce( (PACL)AclBuffer, &ChildAcePosition ) ) {
                        return STATUS_BAD_INHERITANCE_ACL;
                    }
                }

            //
            // If the ACL isn't protected,
            //  don't allow NULL ACL semantics.
            //  (those semantics are ambiguous for auto inheritance)
            //
            } else if ( AutoInherit &&
                        !IsSacl &&
                        (ChildGenericControl & (SEP_ACL_PRESENT|SEP_ACL_PROTECTED)) == SEP_ACL_PRESENT ) {
                return STATUS_INVALID_ACL;

            }

            *NewAclExplicitlyAssigned = TRUE;

        }

    }

    //
    // Inherit ACEs from the Directory ACL in any of the following cases:
    //  If !AutoInheriting,
    //      Inherit if there is no explicit child ACL (ignoring a defaulted child).
    //  If AutoInheriting,
    //      observe the protected flag.
    //

    if ( (!AutoInherit &&
            (ChildGenericControl & SEP_ACL_PRESENT) == 0 ||
                (ChildGenericControl & SEP_ACL_DEFAULTED) != 0) ||
         (AutoInherit && !AclProtected) ) {

        //
        //  If there is no directory ACL,
        //      don't inherit from it.
        //

        if ( DirectoryAcl != NULL ) {

            //
            // If the DirectoryAcl is used,
            //  the revision of the Directory ACL is picked up.
            //

            if ( !ValidAclRevision(DirectoryAcl) ) {
                return STATUS_UNKNOWN_REVISION;
            }

            AclRevision = max( AclRevision, DirectoryAcl->AclRevision );

            //
            // Inherit the Parent's ACL.
            //

            Status = RtlpGenerateInheritAcl(
                         DirectoryAcl,
                         IsDirectoryObject,
                         AutoInherit,
                         OwnerSid,
                         GroupSid,
                         ServerOwnerSid,
                         ServerGroupSid,
                         GenericMapping,
                         NewObjectType,
                         &DirectoryNewAclSize,
                         (PACL)AclBuffer,
                         &ObjectAceInherited );

            if ( Status == STATUS_BUFFER_TOO_SMALL ) {
                AclOverflowed = TRUE;
                Status = STATUS_SUCCESS;
            }

            if ( !NT_SUCCESS(Status) ) {
                return Status;
            }

            //
            // If the default descriptor for the object should be ditched,
            //  because object specific ACEs were inherited from the directory,
            //  ditch them now.
            //

            if ( DefaultDescriptorForObject &&
                 ChildNewAclSize != 0 &&
                 ObjectAceInherited &&
                 !AclOverflowed ) {

                //
                // Compute the last used byte of the combined ACL
                //
                if (!RtlFirstFreeAce( (PACL)AclBuffer, &DirectoryAcePosition ) ) {
                    return STATUS_BAD_INHERITANCE_ACL;
                }
                if ( DirectoryAcePosition == NULL ) {
                    DirectoryAcePosition = AclBuffer + ((PACL)AclBuffer)->AclSize;
                }



                //
                // Move all the inherited ACEs to the front of the ACL.
                //

                RtlMoveMemory( FirstAce( AclBuffer ),
                               ChildAcePosition,
                               (ULONG)(((PUCHAR)DirectoryAcePosition) -
                                (PUCHAR)ChildAcePosition) );

                //
                // Adjust the ACE count to remove the deleted ACEs
                //

                ((PACL)AclBuffer)->AceCount -= ChildAceCount;

                //
                // Save the number of bytes of the Child ACL that were
                //  actually used.
                //

                UsedChildNewAclSize = 0;

            }
        }

    }

    //
    // If this routine didn't build the ACL,
    //  tell the caller.
    //

    if ( DirectoryNewAclSize + UsedChildNewAclSize == 0) {

        //
        // If the ACL was not explicitly assigned,
        //  tell the caller to default the ACL.
        //
        if ( !(*NewAclExplicitlyAssigned) ) {
            *AclBufferSize = 0;
            return STATUS_NO_INHERITANCE;

        //
        // If the Acl was explictly assigned,
        //  generate a NULL ACL based on the path taken above.
        //

        } else if ( NullAclOk ) {
            *AclBufferSize = 0;
            return STATUS_SUCCESS;
        }

        // DbgBreakPoint();
    }


    //
    // And make sure we don't exceed the length limitations of an ACL (WORD)
    //

    if ( DirectoryNewAclSize + UsedChildNewAclSize + sizeof(ACL) > 0xFFFF) {
        return(STATUS_BAD_INHERITANCE_ACL);
    }

    // BUGBUG: The caller has to allocate a buffer large enough for
    // ChildNewAclSize rather than UsedChildNewAclSize.  Due to the nature of
    // my algorithm above.
    (*AclBufferSize) = DirectoryNewAclSize + ChildNewAclSize + sizeof(ACL);

    if ( AclOverflowed ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Patch the real ACL size and revision into the ACL
    //

    ((PACL)AclBuffer)->AclSize = (USHORT)
        (DirectoryNewAclSize + UsedChildNewAclSize + sizeof(ACL));
    ((PACL)AclBuffer)->AclRevision = (UCHAR) AclRevision;

    return STATUS_SUCCESS;
}


NTSTATUS
RtlpInheritAcl (
    IN PACL DirectoryAcl,
    IN PACL ChildAcl,
    IN ULONG ChildGenericControl,
    IN BOOLEAN IsDirectoryObject,
    IN BOOLEAN AutoInherit,
    IN BOOLEAN DefaultDescriptorForObject,
    IN PSID OwnerSid,
    IN PSID GroupSid,
    IN PSID ServerOwnerSid OPTIONAL,
    IN PSID ServerGroupSid OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOLEAN IsSacl,
    IN GUID *NewObjectType OPTIONAL,
    OUT PACL *NewAcl,
    OUT PBOOLEAN NewAclExplicitlyAssigned,
    OUT PULONG NewGenericControl
    )

/*++

Routine Description:

    This is a private routine that produces an inherited acl from
    a parent acl according to the rules of inheritance

Arguments:

    DirectoryAcl - Supplies the acl being inherited.

    ChildAcl - Supplies the acl associated with the object.  This
        is either the current acl on the object or the acl being assigned
        to the object.

    ChildGenericControl - Specifies the control flags from the SecurityDescriptor
        describing the ChildAcl:

        SEP_ACL_PRESENT: Specifies that the child ACL is explictly supplied by
            the caller.

        SEP_ACL_DEFAULTED: Specifies that the child ACL was supplied by some
            defaulting mechanism.

        SEP_ACL_PROTECTED: Specifies that the child ACL is protected and
            should not inherit any ACE from the DirectoryACL

    IsDirectoryObject - Specifies if the new acl is for a directory.

    AutoInherit - Specifies if the inheritance is an "automatic inheritance".
        As such, the non-inherited ACEs from the ChildAcl will be preserved and
        the inherited ACEs from the DirectoryAcl will be marked as such.

    DefaultDescriptorForObject - If set, the CreatorDescriptor
        is the default descriptor for ObjectType.  As such, the
        CreatorDescriptor will be ignored if any ObjectType specific
        ACEs are inherited from the parent.  If not such ACEs are inherited,
        the CreatorDescriptor is handled as though this flag were not
        specified.

    OwnerSid - Specifies the owner Sid to use.

    GroupSid - Specifies the group SID to use.

    GenericMapping - Specifies the generic mapping to use.

    IsSacl - True if this is the SACL.  False if this is the DACL.

    NewObjectType - Type of object being inherited to.  If not specified,
        the object has no object type.

    NewAcl - Receives a pointer to the new (inherited) acl.

    NewAclExplicitlyAssigned - Returns true to indicate that some portion of
        "NewAcl" was derived from an the explicit ChildAcl

    NewGenericControl - Specifies the control flags for the newly
        generated ACL.

        SEP_ACL_AUTO_INHERITED: Set if the ACL was generated using the
            Automatic Inheritance algorithm.

        SEP_ACL_PROTECTED: Specifies that the ACL is protected and
            was not inherited from the parent ACL.

Return Value:

    STATUS_SUCCESS - An inheritable ACL was successfully generated.

    STATUS_NO_INHERITANCE - An inheritable ACL was not successfully generated.
        This is a warning completion status.

    STATUS_BAD_INHERITANCE_ACL - Indicates the acl built was not a valid ACL.
        This can becaused by a number of things.  One of the more probable
        causes is the replacement of a CreatorId with an SID that didn't fit
        into the ACE or ACL.

    STATUS_UNKNOWN_REVISION - Indicates the source ACL is a revision that
        is unknown to this routine.

--*/

{
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//   The logic in the ACL inheritance code must mirror the code for         //
//   inheritance in the executive (in seassign.c).  Do not make changes     //
//   here without also making changes in that module.                       //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////



    NTSTATUS Status;
    ULONG AclBufferSize;
    ULONG i;

#ifndef NTOS_KERNEL_RUNTIME
    PVOID HeapHandle;
#endif // NTOS_KERNEL_RUNTIME

    RTL_PAGED_CODE();

    //
    // Get the handle to the current process heap
    //

#ifndef NTOS_KERNEL_RUNTIME
    HeapHandle = RtlProcessHeap();
#endif // NTOS_KERNEL_RUNTIME



    //
    // Implement a two pass strategy.
    //
    // First try to create the ACL in a fixed length buffer.
    // If that is too small,
    //  then use the buffer size determined on the first pass
    //

    AclBufferSize = 1024;   // Typical maximum size of an ACL
    for ( i=0; i<2 ; i++ ) {

        //
        // Allocate heap for the new ACL.
        //

#ifdef NTOS_KERNEL_RUNTIME
        (*NewAcl) = ExAllocatePoolWithTag(
                        PagedPool,
                        AclBufferSize,
                        'cAeS' );
#else // NTOS_KERNEL_RUNTIME
        (*NewAcl) = RtlAllocateHeap(
                        HeapHandle,
                        MAKE_TAG(SE_TAG),
                        AclBufferSize );
#endif // NTOS_KERNEL_RUNTIME

        if ((*NewAcl) == NULL ) {
            return( STATUS_NO_MEMORY );
        }

        //
        // Actually build the inherited ACL.
        //

        Status = RtlpInheritAcl2 (
                    DirectoryAcl,
                    ChildAcl,
                    ChildGenericControl,
                    IsDirectoryObject,
                    AutoInherit,
                    DefaultDescriptorForObject,
                    OwnerSid,
                    GroupSid,
                    ServerOwnerSid,
                    ServerGroupSid,
                    GenericMapping,
                    IsSacl,
                    NewObjectType,
                    &AclBufferSize,
                    (PUCHAR) *NewAcl,
                    NewAclExplicitlyAssigned,
                    NewGenericControl );


        if ( NT_SUCCESS(Status) ) {

            //
            // If a NULL ACL should be used,
            //  tell the caller.
            //

            if ( AclBufferSize == 0 ) {

#ifdef NTOS_KERNEL_RUNTIME
                ExFreePool( *NewAcl );
#else // NTOS_KERNEL_RUNTIME
                RtlFreeHeap( HeapHandle, 0, *NewAcl );
#endif // NTOS_KERNEL_RUNTIME

                *NewAcl = NULL;
            }

            break;

        } else {
#ifdef NTOS_KERNEL_RUNTIME
            ExFreePool( *NewAcl );
#else // NTOS_KERNEL_RUNTIME
            RtlFreeHeap( HeapHandle, 0, *NewAcl );
#endif // NTOS_KERNEL_RUNTIME

            *NewAcl = NULL;

            if ( Status != STATUS_BUFFER_TOO_SMALL ) {
                break;
            }
        }
    }


    return Status;
}


NTSTATUS
RtlpGenerateInheritedAce (
    IN PACE_HEADER OldAce,
    IN BOOLEAN IsDirectoryObject,
    IN BOOLEAN AutoInherit,
    IN PSID ClientOwnerSid,
    IN PSID ClientGroupSid,
    IN PSID ServerOwnerSid OPTIONAL,
    IN PSID ServerGroupSid OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping,
    IN GUID *NewObjectType OPTIONAL,
    OUT PULONG NewAceLength,
    OUT PACL NewAcl,
    OUT PULONG NewAceExtraLength,
    OUT PBOOLEAN ObjectAceInherited
    )

/*++

Routine Description:

    This is a private routine that checks if the input ace is inheritable
    and produces 0, 1, or 2 inherited aces in the given buffer.

Arguments:

    OldAce - Supplies the ace being inherited

    IsDirectoryObject - Specifies if the new ACE is for a directory

    AutoInherit - Specifies if the inheritance is an "automatic inheritance".
        As such, the inherited ACEs will be marked as such.

    ClientOwnerSid - Specifies the owner Sid to use

    ClientGroupSid - Specifies the new Group Sid to use

    ServerSid - Optionally specifies the Server Sid to use in compound ACEs.

    ClientSid - Optionally specifies the Client Sid to use in compound ACEs.

    GenericMapping - Specifies the generic mapping to use

    NewObjectType - Type of object being inherited to.  If not specified,
        the object has no object type.

    NewAceLength - Receives the length (number of bytes) needed to allow for
        the inheritance of the specified ACE.  This might be zero.

    NewAcl - Provides a pointer to the ACL into which the ACE is to be
        inherited.

    NewAceExtraLength - Receives a length (number of bytes) temporarily used
        in the ACL for the inheritance ACE.  This might be zero

    ObjectAceInherited - Returns true if one or more object ACEs were inherited
        based on NewObjectType

Return Value:

    STATUS_SUCCESS - The ACE was inherited successfully.

    STATUS_BAD_INHERITANCE_ACL - Indicates something went wrong preventing
        the ACE from being inherited.  This generally represents a bugcheck
        situation when returned from this call.

    STATUS_BUFFER_TOO_SMALL - The ACL specified by NewAcl is too small for the
        inheritance ACEs.  The required size is returned in NewAceLength.


--*/

{
    ///////////////////////////////////////////////////////////////////////////
    //                                                                       //
    // !!!!!!!!!  This is tricky  !!!!!!!!!!                                 //
    //                                                                       //
    // The inheritence flags AND the sid of the ACE determine whether        //
    // we need 0, 1, or 2 ACEs.                                              //
    //                                                                       //
    // BE CAREFUL WHEN CHANGING THIS CODE.  READ THE DSA ACL ARCHITECTURE    //
    // SECTION COVERING INHERITENCE BEFORE ASSUMING YOU KNOW WHAT THE HELL   //
    // YOU ARE DOING!!!!                                                     //
    //                                                                       //
    // The general gist of the algorithm is:                                 //
    //                                                                       //
    //       if ( (container  && ContainerInherit) ||                        //
    //            (!container && ObjectInherit)      ) {                     //
    //               GenerateEffectiveAce;                                   //
    //       }                                                               //
    //                                                                       //
    //                                                                       //
    //       if (Container && Propagate) {                                   //
    //           Propogate copy of ACE and set InheritOnly;                  //
    //       }                                                               //
    //                                                                       //
    //                                                                       //
    // A slightly more accurate description of this algorithm is:            //
    //                                                                       //
    //   IO  === InheritOnly flag                                            //
    //   CI  === ContainerInherit flag                                       //
    //   OI  === ObjectInherit flag                                          //
    //   NPI === NoPropagateInherit flag                                     //
    //                                                                       //
    //   if ( (container  && CI) ||                                          //
    //        (!container && OI)   ) {                                       //
    //       Copy Header of ACE;                                             //
    //       Clear IO, NPI, CI, OI;                                          //
    //                                                                       //
    //       if (KnownAceType) {                                             //
    //           if (SID is a creator ID) {                                  //
    //               Copy appropriate creator SID;                           //
    //           } else {                                                    //
    //               Copy SID of original;                                   //
    //           }                                                           //
    //                                                                       //
    //           Copy AccessMask of original;                                //
    //           MapGenericAccesses;                                         //
    //           if (AccessMask == 0) {                                      //
    //               discard new ACE;                                        //
    //           }                                                           //
    //                                                                       //
    //       } else {                                                        //
    //           Copy body of ACE;                                           //
    //       }                                                               //
    //                                                                       //
    //   }                                                                   //
    //                                                                       //
    //   if (!NPI) {                                                         //
    //       Copy ACE as is;                                                 //
    //       Set IO;                                                         //
    //   }                                                                   //
    //                                                                       //
    //                                                                       //
    //                                                                       //
    ///////////////////////////////////////////////////////////////////////////



    ULONG LengthRequired = 0;
    ULONG ExtraLengthRequired = 0;
    PVOID AcePosition;
    PVOID EffectiveAcePosition;
    ULONG EffectiveAceSize = 0;

    BOOLEAN EffectiveAceMapped;
    BOOLEAN AclOverflowed = FALSE;
    BOOLEAN GenerateInheritAce;

    RTL_PAGED_CODE();

    //
    //  This is gross and ugly, but it's better than allocating
    //  virtual memory to hold the ClientSid, because that can
    //  fail, and propogating the error back is a tremendous pain
    //

    ASSERT(RtlLengthRequiredSid( 1 ) == CREATOR_SID_SIZE);
    *ObjectAceInherited = FALSE;
    GenerateInheritAce = IsDirectoryObject && Propagate(OldAce);

    //
    // Allocate and initialize the universal SIDs we're going to need
    // to look for inheritable ACEs.
    //

    if (!RtlFirstFreeAce( NewAcl, &AcePosition ) ) {
        return STATUS_BAD_INHERITANCE_ACL;
    }

    //
    //  check to see if we will have a effective ACE (one mapped to
    //  the target object type).
    //

    if ( (IsDirectoryObject  && ContainerInherit(OldAce)) ||
         (!IsDirectoryObject && ObjectInherit(OldAce))      ) {


        //
        // Remember where the effective ACE will be copied to.
        //
        EffectiveAcePosition = AcePosition;

        //
        // Copy the effective ACE into the ACL.
        //
        if ( !RtlpCopyEffectiveAce (
                        OldAce,
                        AutoInherit,
                        GenerateInheritAce,
                        ClientOwnerSid,
                        ClientGroupSid,
                        ServerOwnerSid,
                        ServerGroupSid,
                        GenericMapping,
                        NewObjectType,
                        &AcePosition,
                        &EffectiveAceSize,
                        NewAcl,
                        ObjectAceInherited,
                        &EffectiveAceMapped,
                        &AclOverflowed ) ) {

            return STATUS_BAD_INHERITANCE_ACL;
        }

        //
        // If the effective ACE is a duplicate of existing inherited ACEs,
        //  Don't really generate it.
        //

        if ( !AclOverflowed &&
             EffectiveAceSize > 0 &&
             EffectiveAcePosition != NULL &&
                RtlpIsDuplicateAce(
                    NewAcl,
                    EffectiveAcePosition,
                    NewObjectType ) ) {


            //
            // Truncate the ACE we just added.
            //

            NewAcl->AceCount--;
            AcePosition = EffectiveAcePosition;
            ExtraLengthRequired = max( ExtraLengthRequired, EffectiveAceSize );
            EffectiveAceSize = 0;
        }

        LengthRequired += EffectiveAceSize;

    }

    //
    // If we are inheriting onto a container, then we may need to
    // propagate the inheritance as well.
    //

    if ( GenerateInheritAce ) {

        //
        // If a effective ACE was created,
        //  and it wasn't mapped,
        //  avoid generating another ACE and simply merge the inheritance bits into
        //      the effective ACE.
        //

        if ( EffectiveAceSize != 0 && !EffectiveAceMapped ) {

           //
           // Copy the inherit bits from the original ACE.
           //
           if ( !AclOverflowed ) {
               ((PACE_HEADER)EffectiveAcePosition)->AceFlags |=
                    ((PACE_HEADER)OldAce)->AceFlags & (CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE);
               if ( AutoInherit ) {
                   ((PACE_HEADER)EffectiveAcePosition)->AceFlags |= INHERITED_ACE;
               }
           }


        //
        // Otherwise, generate an explicit inheritance ACE.
        //
        // But only if the access mask isn't zero.
        //

        } else if ( !IsMSAceType(OldAce) || ((PKNOWN_ACE)(OldAce))->Mask != 0 ) {

            //
            // Account for the new ACE being added to the ACL.
            //
            LengthRequired += (ULONG)(((PACE_HEADER)OldAce)->AceSize);

            if (LengthRequired > 0xFFFF) {
                return STATUS_BAD_INHERITANCE_ACL;
            }

            //
            // If the ACE doesn't fit,
            //  just note the fact and don't copy the ACE.
            //

            if ( ((PACE_HEADER)OldAce)->AceSize > NewAcl->AclSize - ((PUCHAR)AcePosition - (PUCHAR)NewAcl) ) {
                AclOverflowed = TRUE;
            } else if (!AclOverflowed){

                //
                // copy it as is, but make sure the InheritOnly bit is set.
                //

                RtlCopyMemory(
                    AcePosition,
                    OldAce,
                    ((PACE_HEADER)OldAce)->AceSize
                    );

                ((PACE_HEADER)AcePosition)->AceFlags |= INHERIT_ONLY_ACE;
                NewAcl->AceCount += 1;
                if ( AutoInherit ) {
                    ((PACE_HEADER)AcePosition)->AceFlags |= INHERITED_ACE;

                    //
                    // If the inheritance ACE is a duplicate of existing inherited ACEs,
                    //  Don't really generate it.
                    //

                    if ( RtlpIsDuplicateAce(
                                NewAcl,
                                AcePosition,
                                NewObjectType ) ) {


                        //
                        // Truncate the ACE we just added.
                        //

                        NewAcl->AceCount--;
                        ExtraLengthRequired = max( ExtraLengthRequired,
                                                   ((PACE_HEADER)OldAce)->AceSize );
                        LengthRequired -= (ULONG)(((PACE_HEADER)OldAce)->AceSize);
                    }
                }

            }
        }
    }

    //
    //  Now return to our caller
    //

    (*NewAceLength) = LengthRequired;
    (*NewAceExtraLength) = ExtraLengthRequired;

    return AclOverflowed ? STATUS_BUFFER_TOO_SMALL : STATUS_SUCCESS;
}


NTSTATUS
RtlpGenerateInheritAcl(
    IN PACL Acl,
    IN BOOLEAN IsDirectoryObject,
    IN BOOLEAN AutoInherit,
    IN PSID ClientOwnerSid,
    IN PSID ClientGroupSid,
    IN PSID ServerOwnerSid OPTIONAL,
    IN PSID ServerGroupSid OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping,
    IN GUID *NewObjectType OPTIONAL,
    OUT PULONG NewAclSizeParam,
    OUT PACL NewAcl,
    OUT PBOOLEAN ObjectAceInherited
    )

/*++

Routine Description:

    This is a private routine that produces an inheritable ACL.

    The buffer to contain the inherted ACL is passed in.  If the buffer is
    too small, the corect size is computed and STATUS_BUFFER_TOO_SMALL is
    returned.

Arguments:

    Acl - Supplies the acl being inherited.

    IsDirectoryObject - Specifies if the new acl is for a directory.

    AutoInherit - Specifies if the inheritance is an "automatic inheritance".
        As such, the inherited ACEs will be marked as such.

    OwnerSid - Specifies the owner Sid to use.

    GroupSid - Specifies the group SID to use.

    GenericMapping - Specifies the generic mapping to use.

    NewObjectType - Type of object being inherited to.  If not specified,
        the object has no object type.

    NewAclSizeParam - Receives the length of the inherited ACL.

    NewAcl - Provides a pointer to the buffer to receive the new
        (inherited) acl.  This ACL must already be initialized.

    ObjectAceInherited - Returns true if one or more object ACEs were inherited
        based on NewObjectType


Return Value:

    STATUS_SUCCESS - An inheritable ACL has been generated.

    STATUS_BAD_INHERITANCE_ACL - Indicates the acl built was not a valid ACL.
        This can becaused by a number of things.  One of the more probable
        causes is the replacement of a CreatorId with an SID that didn't fit
        into the ACE or ACL.

    STATUS_BUFFER_TOO_SMALL - The ACL specified by NewAcl is too small for the
        inheritance ACEs.  The required size is returned in NewAceLength.


--*/

{

    NTSTATUS Status;
    ULONG i;

    PACE_HEADER OldAce;
    ULONG NewAclSize, NewAceSize;
    ULONG NewAclExtraSize, NewAceExtraSize;
    BOOLEAN AclOverflowed = FALSE;
    BOOLEAN LocalObjectAceInherited;


    RTL_PAGED_CODE();

    //
    // Walk through the original ACL generating any necessary
    // inheritable ACEs.
    //

    NewAclSize = 0;
    NewAclExtraSize = 0;
    *ObjectAceInherited = FALSE;
    for (i = 0, OldAce = FirstAce(Acl);
         i < Acl->AceCount;
         i += 1, OldAce = NextAce(OldAce)) {

        //
        //  RtlpGenerateInheritedAce() will generate the ACE(s) necessary
        //  to inherit a single ACE.  This may be 0, 1, or more ACEs.
        //

        Status = RtlpGenerateInheritedAce(
                     OldAce,
                     IsDirectoryObject,
                     AutoInherit,
                     ClientOwnerSid,
                     ClientGroupSid,
                     ServerOwnerSid,
                     ServerGroupSid,
                     GenericMapping,
                     NewObjectType,
                     &NewAceSize,
                     NewAcl,
                     &NewAceExtraSize,
                     &LocalObjectAceInherited
                     );

        if ( Status == STATUS_BUFFER_TOO_SMALL ) {
            AclOverflowed = TRUE;
            Status = STATUS_SUCCESS;
        }

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        if ( LocalObjectAceInherited ) {
            *ObjectAceInherited = TRUE;
        }

        //
        // Make room in the ACL for the new ACE
        //
        NewAclSize += NewAceSize;

        //
        // If a previous ACE needed 'extra' space,
        //  reduce that requirement by the size of this ACE.
        //
        // The previous ACE can use this ACE's space temporarily
        //
        if ( NewAceSize > NewAclExtraSize ) {
            NewAclExtraSize = 0 ;
        } else {
            NewAclExtraSize -= NewAceSize;
        }

        //
        // The 'extra' space needed is the larger of that needed by any
        //  previous ACE and that need by this ACE
        //
        NewAclExtraSize = max( NewAclExtraSize, NewAceExtraSize );

    }

    //
    // We only need to include the "ExtraSize" if we've overflowed.
    //  In those cases, the caller will allocate the size we requested and
    //  try again.  Otherwise, the caller won't call back so we don't care
    //  if it knows about the extra size.
    //

    if ( AclOverflowed ) {
        (*NewAclSizeParam) = NewAclSize + NewAclExtraSize;
        return STATUS_BUFFER_TOO_SMALL;
    } else {
        (*NewAclSizeParam) = NewAclSize;
        return STATUS_SUCCESS;
    }

}


NTSTATUS
RtlpComputeMergedAcl2 (
    IN PACL CurrentAcl,
    IN ULONG CurrentGenericControl,
    IN PACL ModificationAcl,
    IN ULONG ModificationGenericControl,
    IN PSID ClientOwnerSid,
    IN PSID ClientGroupSid,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOLEAN IsSacl,
    IN PULONG AclBufferSize,
    IN OUT PUCHAR AclBuffer,
    OUT PULONG NewGenericControl
    )

/*++

Routine Description:

    This routine implements the 'set' semantics for auto inheritance.

    This routine builds the actual ACL that should be set on an object.
    The built ACL is a composite of the previous ACL on an object and
    the newly set ACL on the object.  The New ACL is built as follows:

    If SEP_ACL_PROTECTED is set in neither CurrentAcl nor ModificationAcl,
    the NewAcl is constructed from the inherited ACEs from the
    CurrentAcl and the non-inherited ACEs from the ModificationAcl.
    (That is, it is impossible to edit an inherited ACE by changing the
    ACL on an object.)

    If SEP_ACL_PROTECTED is set on ModificationAcl, CurrentAcl is ignored.
    NewAcl is built as a copy of ModificationAcl with any INHERITED_ACE
    bits turned off.

    If SEP_ACL_PROTECTED is set on CurrentAcl and not ModificationAcl, the
    CurrentAcl is ignored.  NewAcl is built as a copy of
    ModificationDescriptor.  It is the callers responsibility to ensure
    that the correct ACEs have the INHERITED_ACE bit turned on.

Arguments:

    CurrentAcl - The current ACL on the object.

    CurrentGenericControl - Specifies the control flags from the SecurityDescriptor
        describing the CurrentAcl.

    ModificationAcl - The ACL being applied to the object.

    ModificationGenericControl - Specifies the control flags from the SecurityDescriptor
        describing the CurrentAcl.

    ClientOwnerSid - Specifies the owner Sid to use

    ClientGroupSid - Specifies the new Group Sid to use

    GenericMapping - The mapping of generic to specific and standard
                     access types.

    IsSacl - True if this is the SACL.  False if this is the DACL.

    AclBufferSize - On input, specifies the size of AclBuffer.
        On output, on success, returns the used size of AclBuffer.
        On output, if the buffer is too small, returns the required size of AclBuffer.

    AclBuffer - Receives a pointer to the new (inherited) acl.

    NewGenericControl - Specifies the control flags for the newly
        generated ACL.

        Only the Protected and AutoInherited bits are returned.

Return Value:

    STATUS_SUCCESS - An ACL was successfully generated.

    STATUS_UNKNOWN_REVISION - Indicates the source ACL is a revision that
        is unknown to this routine.

--*/

{
    NTSTATUS Status;
    ULONG ModificationNewAclSize = 0;
    ULONG CurrentNewAclSize = 0;
    ULONG AclRevision;
    BOOLEAN AclOverflowed = FALSE;
    BOOLEAN NullAclOk = TRUE;

    RTL_PAGED_CODE();


    //
    // Assume the ACL revision.
    //

    AclRevision = ACL_REVISION;
    RtlCreateAcl( (PACL)AclBuffer, *AclBufferSize, AclRevision );

    //
    // This routine is only called for the AutoInheritance case.
    //

    *NewGenericControl = SEP_ACL_AUTO_INHERITED;

    //
    // If the new ACL is protected,
    //  simply use the new ACL with the INHERITED_ACE bits turned off.
    //

    if ( (ModificationGenericControl & SEP_ACL_PROTECTED) != 0 ) {

        //
        // Set the Control bits for the resultant descriptor.
        //

        *NewGenericControl |= SEP_ACL_PROTECTED;

        //
        // Only copy the ACL if it is actually present
        //

        if ( ModificationAcl != NULL ) {

            AclRevision = max( AclRevision, ModificationAcl->AclRevision );

            //
            // Copy all ACES, turn off the inherited bit, and generic map them.
            //

            Status = RtlpCopyAces(
                        ModificationAcl,
                        GenericMapping,
                        CopyAllAces,
                        INHERITED_ACE,  // Turn off all INHERITED_ACE flags
                        TRUE,           // Map sids as needed
                        ClientOwnerSid,
                        ClientGroupSid,
                        ClientOwnerSid, // Not technically correct. s.b. server sid
                        ClientGroupSid, // Not technically correct. s.b. server sid
                        &ModificationNewAclSize,
                        (PACL)AclBuffer );

            if ( Status == STATUS_BUFFER_TOO_SMALL ) {
                AclOverflowed = TRUE;
                Status = STATUS_SUCCESS;
            }

            if ( !NT_SUCCESS(Status) ) {
                return Status;
            }

            //
            // If the caller specified an ACL with no ACES,
            //  make sure we generate an ACL with no ACES.
            //

            NullAclOk = FALSE;
        }

    //
    // If the old ACL is protected but the new one isn't,
    //  simply use the new ACL as is.
    //
    // Rely on the caller to get the INHERITED_ACE bits right.
    //

    } else if ( (CurrentGenericControl & SEP_ACL_PROTECTED) != 0 ) {

        //
        // Only do the copy if the new ACL is specified.
        //

        if ( ModificationAcl != NULL ) {
            AclRevision = max( AclRevision, ModificationAcl->AclRevision );

            //
            // Copy all ACES, and generic map them.
            //

            Status = RtlpCopyAces(
                        ModificationAcl,
                        GenericMapping,
                        CopyAllAces,
                        0,
                        TRUE,           // Map sids as needed
                        ClientOwnerSid,
                        ClientGroupSid,
                        ClientOwnerSid, // Not technically correct. s.b. server sid
                        ClientGroupSid, // Not technically correct. s.b. server sid
                        &ModificationNewAclSize,
                        (PACL)AclBuffer );

            if ( Status == STATUS_BUFFER_TOO_SMALL ) {
                AclOverflowed = TRUE;
                Status = STATUS_SUCCESS;
            }

            if ( !NT_SUCCESS(Status) ) {
                return Status;
            }

            //
            // If the caller specified an ACL with no ACES,
            //  make sure we generate an ACL with no ACES.
            //

            NullAclOk = FALSE;

        //
        // Since the ACL isn't protected,
        //  don't allow NULL ACL semantics.
        //  (those semantics are ambiguous for auto inheritance)
        //
        } else if ( !IsSacl ) {
            return STATUS_INVALID_ACL;
        }


    //
    // If neither are protected,
    //  use the non-inherited ACEs from the new ACL, and
    //  preserve the inherited ACEs from the old ACL.
    //

    } else {

        //
        // NULL ACLs are always OK for a SACL.
        // NULL ACLs are never OK for a non-protected DACL.
        //

        NullAclOk = IsSacl;


        //
        // Only do the copy if the new ACL is specified.
        //

        if ( ModificationAcl != NULL ) {
            AclRevision = max( AclRevision, ModificationAcl->AclRevision );

            //
            // Copy the non-inherited ACES, and generic map them.
            //

            Status = RtlpCopyAces(
                        ModificationAcl,
                        GenericMapping,
                        CopyNonInheritedAces,
                        0,
                        TRUE,           // Map sids as needed
                        ClientOwnerSid,
                        ClientGroupSid,
                        ClientOwnerSid, // Not technically correct. s.b. server sid
                        ClientGroupSid, // Not technically correct. s.b. server sid
                        &ModificationNewAclSize,
                        (PACL)AclBuffer );

            if ( Status == STATUS_BUFFER_TOO_SMALL ) {
                AclOverflowed = TRUE;
                Status = STATUS_SUCCESS;
            }

            if ( !NT_SUCCESS(Status) ) {
                return Status;
            }

            //
            // If the caller specified an ACL with no ACES,
            //  make sure we generate an ACL with no ACES.
            //
            // If inherited aces were deleted, leave the flag alone allowing
            //  a NULL SACL to be generated.
            //

            if ( ModificationAcl->AceCount == 0 ) {
                NullAclOk = FALSE;
            }

        //
        // Since the ACL isn't protected,
        //  don't allow NULL ACL semantics.
        //  (those semantics are ambiguous for auto inheritance)
        //
        } else if ( !IsSacl ) {
            return STATUS_INVALID_ACL;
        }


        //
        // Only do the copy if the old ACL is specified.
        //

        if ( CurrentAcl != NULL ) {

            AclRevision = max( AclRevision, CurrentAcl->AclRevision );


            //
            // Copy the inherited ACES, and generic map them.
            //
            // Don't bother mapping the sids in these ACEs.  They got mapped
            // during inheritance.
            //

            Status = RtlpCopyAces(
                        CurrentAcl,
                        GenericMapping,
                        CopyInheritedAces,
                        0,
                        FALSE,          // Don't map the sids,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &CurrentNewAclSize,
                        (PACL)AclBuffer );

            if ( Status == STATUS_BUFFER_TOO_SMALL ) {
                AclOverflowed = TRUE;
                Status = STATUS_SUCCESS;
            }

            if ( !NT_SUCCESS(Status) ) {
                return Status;
            }
        }
    }

    //
    // If this routine didn't build the ACL,
    //  tell the caller to use an explict NULL ACL
    //

    if ( ModificationNewAclSize + CurrentNewAclSize == 0) {
        //
        // If the Acl was explictly assigned,
        //  generate a NULL ACL based on the path taken above.
        //

        if ( NullAclOk ) {
            *AclBufferSize = 0;
            return STATUS_SUCCESS;
        }
    }


    //
    // And make sure we don't exceed the length limitations of an ACL (WORD)
    //

    if ( ModificationNewAclSize + CurrentNewAclSize + sizeof(ACL) > 0xFFFF) {
        return(STATUS_BAD_INHERITANCE_ACL);
    }

    (*AclBufferSize) = ModificationNewAclSize + CurrentNewAclSize + sizeof(ACL);

    if ( AclOverflowed ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Patch the real ACL size and revision into the ACL
    //

    ((PACL)AclBuffer)->AclSize = (USHORT) *AclBufferSize;
    ((PACL)AclBuffer)->AclRevision = (UCHAR) AclRevision;

    return STATUS_SUCCESS;
}


NTSTATUS
RtlpComputeMergedAcl (
    IN PACL CurrentAcl,
    IN ULONG CurrentGenericControl,
    IN PACL ModificationAcl,
    IN ULONG ModificationGenericControl,
    IN PSID ClientOwnerSid,
    IN PSID ClientGroupSid,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOLEAN IsSacl,
    OUT PACL *NewAcl,
    OUT PULONG NewGenericControl
    )

/*++

Routine Description:

    This routine builds the actual ACL that should be set on an object.
    The built ACL is a composite of the previous ACL on an object and
    the newly set ACL on the object.  The New ACL is built as follows:

    If SEP_ACL_PROTECTED is set in neither CurrentAcl nor ModificationAcl,
    the NewAcl is constructed from the inherited ACEs from the
    CurrentAcl and the non-inherited ACEs from the ModificationAcl.
    (That is, it is impossible to edit an inherited ACE by changing the
    ACL on an object.)

    If SEP_ACL_PROTECTED is set on ModificationAcl, CurrentAcl is ignored.
    NewAcl is built as a copy of ModificationAcl with any INHERITED_ACE
    bits turned off.

    If SEP_ACL_PROTECTED is set on CurrentAcl and not ModificationAcl, the
    CurrentAcl is ignored.  NewAcl is built as a copy of
    ModificationDescriptor.  It is the callers responsibility to ensure
    that the correct ACEs have the INHERITED_ACE bit turned on.

Arguments:

    CurrentAcl - The current ACL on the object.

    CurrentGenericControl - Specifies the control flags from the SecurityDescriptor
        describing the CurrentAcl.

    ModificationAcl - The ACL being applied to the object.

    ModificationGenericControl - Specifies the control flags from the SecurityDescriptor
        describing the CurrentAcl.

    ClientOwnerSid - Specifies the owner Sid to use

    ClientGroupSid - Specifies the new Group Sid to use

    GenericMapping - The mapping of generic to specific and standard
                     access types.

    IsSacl - True if this is the SACL.  False if this is the DACL.

    NewAcl - Receives a pointer to the new resultant acl.

    NewGenericControl - Specifies the control flags for the newly
        generated ACL.

        Only the Protected and AutoInherited bits are returned.

Return Value:

    STATUS_SUCCESS - An ACL was successfully generated.

    STATUS_UNKNOWN_REVISION - Indicates the source ACL is a revision that
        is unknown to this routine.

--*/

{
    NTSTATUS Status;
    ULONG AclBufferSize;
    ULONG i;
#ifndef NTOS_KERNEL_RUNTIME
    PVOID HeapHandle;
#endif // NTOS_KERNEL_RUNTIME

    RTL_PAGED_CODE();

    //
    // Get the handle to the current process heap
    //

#ifndef NTOS_KERNEL_RUNTIME
    HeapHandle = RtlProcessHeap();
#endif // NTOS_KERNEL_RUNTIME


    //
    // Implement a two pass strategy.
    //
    // First try to create the ACL in a fixed length buffer.
    // If that is too small,
    //  then use the buffer size determined on the first pass
    //

    AclBufferSize = 1024;
    for ( i=0; i<2 ; i++ ) {

        //
        // Allocate heap for the new ACL.
        //

#ifdef NTOS_KERNEL_RUNTIME
        (*NewAcl) = ExAllocatePoolWithTag(
                        PagedPool,
                        AclBufferSize,
                        'cAeS' );
#else // NTOS_KERNEL_RUNTIME
        (*NewAcl) = RtlAllocateHeap( HeapHandle, 0, AclBufferSize );
#endif // NTOS_KERNEL_RUNTIME
        if ((*NewAcl) == NULL ) {
            return( STATUS_NO_MEMORY );
        }

        //
        // Merge the ACLs
        //

        Status = RtlpComputeMergedAcl2 (
                    CurrentAcl,
                    CurrentGenericControl,
                    ModificationAcl,
                    ModificationGenericControl,
                    ClientOwnerSid,
                    ClientGroupSid,
                    GenericMapping,
                    IsSacl,
                    &AclBufferSize,
                    (PUCHAR) *NewAcl,
                    NewGenericControl );


        if ( NT_SUCCESS(Status) ) {

            //
            // If a NULL ACL should be used,
            //  tell the caller.
            //

            if ( AclBufferSize == 0 ) {
#ifdef NTOS_KERNEL_RUNTIME
                ExFreePool( *NewAcl );
#else // NTOS_KERNEL_RUNTIME
                RtlFreeHeap( HeapHandle, 0, *NewAcl );
#endif // NTOS_KERNEL_RUNTIME
                *NewAcl = NULL;
            }

            break;

        } else {
#ifdef NTOS_KERNEL_RUNTIME
            ExFreePool( *NewAcl );
#else // NTOS_KERNEL_RUNTIME
            RtlFreeHeap( HeapHandle, 0, *NewAcl );
#endif // NTOS_KERNEL_RUNTIME
            *NewAcl = NULL;

            if ( Status != STATUS_BUFFER_TOO_SMALL ) {
                break;
            }
        }
    }


    return Status;
}

#endif // WIN16

#if DBG
NTSTATUS
RtlDumpUserSid(
    VOID
    )
{
    NTSTATUS Status;
    HANDLE   TokenHandle;
    CHAR     Buffer[200];
    ULONG    ReturnLength;
    PSID     pSid;
    UNICODE_STRING SidString;
    PTOKEN_USER  User;

    //
    // Attempt to open the impersonation token first
    //

    Status = NtOpenThreadToken(
                 NtCurrentThread(),
                 GENERIC_READ,
                 FALSE,
                 &TokenHandle
                 );

    if (!NT_SUCCESS( Status )) {

        DbgPrint("Not impersonating, status = %X, trying process token\n",Status);

        Status = NtOpenProcessToken(
                     NtCurrentProcess(),
                     GENERIC_READ,
                     &TokenHandle
                     );

        if (!NT_SUCCESS( Status )) {
            DbgPrint("Unable to open process token, status = %X\n",Status);
            return( Status );
        }
    }

    Status = NtQueryInformationToken (
                 TokenHandle,
                 TokenUser,
                 Buffer,
                 200,
                 &ReturnLength
                 );

    if (!NT_SUCCESS( Status )) {

        DbgPrint("Unable to query user sid, status = %X \n",Status);
        NtClose(TokenHandle);
        return( Status );
    }

    User = (PTOKEN_USER)Buffer;

    pSid = User->User.Sid;

    Status = RtlConvertSidToUnicodeString( &SidString, pSid, TRUE );

    if (!NT_SUCCESS( Status )) {
        DbgPrint("Unable to format sid string, status = %X \n",Status);
        NtClose(TokenHandle);
        return( Status );
    }

    DbgPrint("Current Sid = %wZ \n",&SidString);

    RtlFreeUnicodeString( &SidString );

    return( STATUS_SUCCESS );
}

#endif


NTSTATUS
RtlpConvertToAutoInheritSecurityObject(
    IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    IN PSECURITY_DESCRIPTOR CurrentSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR *NewSecurityDescriptor,
    IN GUID *ObjectType OPTIONAL,
    IN BOOLEAN IsDirectoryObject,
    IN PGENERIC_MAPPING GenericMapping
    )
/*++

Routine Description:

    This routine a converts a security descriptor whose ACLs are not marked
    as AutoInherit to a security descriptor whose ACLs are marked as
    AutoInherit.

    See comments for RtlConvertToAutoInheritSecurityObject.

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

    STATUS_SUCCESS - The operation was successful.

    See comments for RtlConvertToAutoInheritSecurityObject.


--*/
{
    NTSTATUS Status;
    PISECURITY_DESCRIPTOR CurrentDescriptor;
    PACL CurrentSacl;
    PACL CurrentDacl;

    PSID NewOwner;
    PSID NewGroup;

    PACL NewSacl = NULL;
    ULONG NewSaclControl = 0;
    BOOLEAN NewSaclAllocated = FALSE;

    PACL NewDacl = NULL;
    ULONG NewDaclControl = 0;
    BOOLEAN NewDaclAllocated = FALSE;
    PACL TemplateInheritedDacl = NULL;
    ULONG GenericControl;

    ULONG AllocationSize;
    ULONG NewOwnerSize;
    ULONG NewGroupSize;
    ULONG NewSaclSize;
    ULONG NewDaclSize;

    PCHAR Field;
    PCHAR Base;

    PISECURITY_DESCRIPTOR_RELATIVE INewDescriptor = NULL;
    ULONG ReturnLength;
    NTSTATUS PassedStatus;
    HANDLE PrimaryToken;

#ifndef NTOS_KERNEL_RUNTIME
    PVOID HeapHandle;
#endif // NTOS_KERNEL_RUNTIME

    RTL_PAGED_CODE();

    //
    // Get the handle to the current process heap
    //

#ifndef NTOS_KERNEL_RUNTIME
    HeapHandle = RtlProcessHeap();
#endif // NTOS_KERNEL_RUNTIME



    //
    //

    CurrentDescriptor = CurrentSecurityDescriptor;

    //
    // Validate the incoming security descriptor.
    //

    if (!RtlValidSecurityDescriptor ( CurrentDescriptor )) {
        Status = STATUS_INVALID_SECURITY_DESCR;
        goto Cleanup;
    }


    NewOwner = RtlpOwnerAddrSecurityDescriptor( CurrentDescriptor );
    if ( NewOwner == NULL ) {
        Status = STATUS_INVALID_SECURITY_DESCR;
        goto Cleanup;
    }

    NewGroup = RtlpGroupAddrSecurityDescriptor( CurrentDescriptor );




    //
    // Handle the SACL.
    //
    //
    // If the SACL isn't present,
    //  special case it.
    //

    CurrentSacl = RtlpSaclAddrSecurityDescriptor( CurrentDescriptor );

    if ( CurrentSacl == NULL ) {
        PACL ParentSacl;

        // Preserve the Acl Present bit and protected bit from the existing descriptor.
        NewSaclControl |= CurrentDescriptor->Control & (SE_SACL_PROTECTED|SE_SACL_PRESENT);

        // Always set the autoinherited bit.
        NewSaclControl |= SE_SACL_AUTO_INHERITED;


        //
        // If the Parent also has a NULL SACL,
        //  just consider this SACL as inherited.
        //  otherwise, this SACL is protected.
        //

        ParentSacl = ARGUMENT_PRESENT(ParentDescriptor) ?
                        RtlpSaclAddrSecurityDescriptor( ((SECURITY_DESCRIPTOR *)ParentDescriptor)) :
                        NULL;
        if ( ParentSacl != NULL) {
            NewSaclControl |= SE_SACL_PROTECTED;
        }


    //
    // If the SACL is already converted,
    //  or if this object is at the root of the tree,
    //  simply leave it alone.
    //
    // Don't force the Protect bit on at the root of the tree since it is semantically
    //  a no-op and gets in the way if the object is ever moved.
    //

    } else if ( RtlpAreControlBitsSet( CurrentDescriptor, SE_SACL_AUTO_INHERITED) ||
                RtlpAreControlBitsSet( CurrentDescriptor, SE_SACL_PROTECTED ) ||
                !ARGUMENT_PRESENT(ParentDescriptor) ) {

        // Preserve the Acl Present bit and protected bit from the existing descriptor.
        NewSaclControl |= CurrentDescriptor->Control & (SE_SACL_PROTECTED|SE_SACL_PRESENT);

        // Always set the autoinherited bit.
        NewSaclControl |= SE_SACL_AUTO_INHERITED;

        NewSacl = CurrentSacl;


    //
    // If the SACL is present,
    //  compute a new SACL with appropriate ACEs marked as inherited.
    //

    } else {


        Status = RtlpConvertAclToAutoInherit (
                    ARGUMENT_PRESENT(ParentDescriptor) ?
                        RtlpSaclAddrSecurityDescriptor(
                            ((SECURITY_DESCRIPTOR *)ParentDescriptor)) :
                        NULL,
                    RtlpSaclAddrSecurityDescriptor(CurrentDescriptor),
                    ObjectType,
                    IsDirectoryObject,
                    RtlpOwnerAddrSecurityDescriptor(CurrentDescriptor),
                    RtlpGroupAddrSecurityDescriptor(CurrentDescriptor),
                    GenericMapping,
                    &NewSacl,
                    &GenericControl );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

        NewSaclAllocated = TRUE;
        NewSaclControl |= SE_SACL_PRESENT | SeControlGenericToSacl( GenericControl );
    }


    //
    // Handle the DACL.
    //
    //
    // If the DACL isn't present,
    //  special case it.
    //

    CurrentDacl = RtlpDaclAddrSecurityDescriptor( CurrentDescriptor );

    if ( CurrentDacl == NULL ) {
        // Preserve the Dacl Present bit from the existing descriptor.
        NewDaclControl |= CurrentDescriptor->Control & SE_DACL_PRESENT;

        // Always set the autoinherited bit.
        // Force it protected.
        NewDaclControl |= SE_DACL_AUTO_INHERITED | SE_DACL_PROTECTED;



    //
    // If the DACL is already converted,
    //  or if this object is at the root of the tree,
    //  simply leave it alone.
    //
    // Don't force the Protect bit on at the root of the tree since it is semantically
    //  a no-op and gets in the way if the object is ever moved.
    //

    } else if ( RtlpAreControlBitsSet( CurrentDescriptor, SE_DACL_AUTO_INHERITED) ||
                RtlpAreControlBitsSet( CurrentDescriptor, SE_DACL_PROTECTED ) ||
                !ARGUMENT_PRESENT(ParentDescriptor) ) {

        // Preserve the Acl Present bit and protected bit from the existing descriptor.
        NewDaclControl |= CurrentDescriptor->Control & (SE_DACL_PROTECTED|SE_DACL_PRESENT);

        // Always set the autoinherited bit.
        NewDaclControl |= SE_DACL_AUTO_INHERITED;

        NewDacl = CurrentDacl;



    //
    // If the DACL is present,
    //  compute a new DACL with appropriate ACEs marked as inherited.
    //

    } else {


        Status = RtlpConvertAclToAutoInherit (
                    ARGUMENT_PRESENT(ParentDescriptor) ?
                        RtlpDaclAddrSecurityDescriptor(
                            ((SECURITY_DESCRIPTOR *)ParentDescriptor)) :
                        NULL,
                    RtlpDaclAddrSecurityDescriptor(CurrentDescriptor),
                    ObjectType,
                    IsDirectoryObject,
                    RtlpOwnerAddrSecurityDescriptor(CurrentDescriptor),
                    RtlpGroupAddrSecurityDescriptor(CurrentDescriptor),
                    GenericMapping,
                    &NewDacl,
                    &GenericControl );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

        NewDaclAllocated = TRUE;
        NewDaclControl |= SE_DACL_PRESENT | SeControlGenericToDacl( GenericControl );
    }



    //
    // Build the resultant security descriptor
    //
    // Also map the ACEs for application to the target object
    // type, if they haven't already been mapped.
    //

    NewOwnerSize = LongAlignSize(SeLengthSid(NewOwner));

    if ( NewGroup != NULL ) {
        NewGroupSize = LongAlignSize(SeLengthSid(NewGroup));
    } else {
        NewGroupSize = 0;
    }

    if (NewSacl != NULL) {
        NewSaclSize = LongAlignSize(NewSacl->AclSize);
    } else {
        NewSaclSize = 0;
    }

    if (NewDacl != NULL) {
        NewDaclSize = LongAlignSize(NewDacl->AclSize);
    } else {
        NewDaclSize = 0;
    }

    AllocationSize = LongAlignSize(sizeof(SECURITY_DESCRIPTOR_RELATIVE)) +
                     NewOwnerSize +
                     NewGroupSize +
                     NewSaclSize  +
                     NewDaclSize;

    //
    // Allocate and initialize the security descriptor as
    // self-relative form.
    //


#ifdef NTOS_KERNEL_RUNTIME
    INewDescriptor = ExAllocatePoolWithTag(
                        PagedPool,
                        AllocationSize,
                        'dSeS' );
#else // NTOS_KERNEL_RUNTIME
    INewDescriptor = RtlAllocateHeap(
                        HeapHandle,
                        MAKE_TAG(SE_TAG),
                        AllocationSize );
#endif // NTOS_KERNEL_RUNTIME

    if ( INewDescriptor == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }


    //
    // Initialize the security descriptor as self-relative form.
    //

    RtlCreateSecurityDescriptorRelative(
        INewDescriptor,
        SECURITY_DESCRIPTOR_REVISION
        );

    RtlpSetControlBits( INewDescriptor, SE_SELF_RELATIVE );

    Base = (PCHAR)(INewDescriptor);
    Field =  Base + sizeof(SECURITY_DESCRIPTOR_RELATIVE);

    //
    // Copy the Sacl
    //

    RtlpSetControlBits( INewDescriptor, NewSaclControl );
    if (NewSacl != NULL ) {

        RtlCopyMemory( Field, NewSacl, NewSacl->AclSize );
        INewDescriptor->Sacl = RtlPointerToOffset(Base,Field);
        Field += NewSaclSize;

    } else {

        INewDescriptor->Sacl = 0;
    }

    //
    // Copy the Dacl
    //

    RtlpSetControlBits( INewDescriptor, NewDaclControl );
    if (NewDacl != NULL ) {

        RtlCopyMemory( Field, NewDacl, NewDacl->AclSize );
        INewDescriptor->Dacl = RtlPointerToOffset(Base,Field);
        Field += NewDaclSize;

    } else {

        INewDescriptor->Dacl = 0;
    }

    //
    // Assign the owner
    //

    RtlCopyMemory( Field, NewOwner, SeLengthSid(NewOwner) );
    INewDescriptor->Owner = RtlPointerToOffset(Base,Field);
    Field += NewOwnerSize;

    if ( NewGroup != NULL ) {
        RtlCopyMemory( Field, NewGroup, SeLengthSid(NewGroup) );
        INewDescriptor->Group = RtlPointerToOffset(Base,Field);
    }

    Status = STATUS_SUCCESS;



    //
    // Cleanup any locally used resources.
    //
Cleanup:
    if (NewDaclAllocated) {
#ifdef NTOS_KERNEL_RUNTIME
            ExFreePool( NewDacl );
#else // NTOS_KERNEL_RUNTIME
            RtlFreeHeap( HeapHandle, 0, NewDacl );
#endif // NTOS_KERNEL_RUNTIME
    }
    if (NewSaclAllocated) {
#ifdef NTOS_KERNEL_RUNTIME
            ExFreePool( NewSacl );
#else // NTOS_KERNEL_RUNTIME
            RtlFreeHeap( HeapHandle, 0, NewSacl );
#endif // NTOS_KERNEL_RUNTIME
    }

    *NewSecurityDescriptor = (PSECURITY_DESCRIPTOR) INewDescriptor;

    return Status;


}

//
// Local macro to classify the ACE flags in an ACE.
//
// Returns one or more of the following ACE flags:
//
//  CONTAINER_INHERIT_ACE - ACE is inherited to child containers
//  OBJECT_INHERIT_ACE - ACE is inherited to child leaf objects
//  EFFECTIVE_ACE - ACE is used during access validation
//

#define MAX_CHILD_SID_GROUP_SIZE 3 // Number of bits in above list
#define EFFECTIVE_ACE INHERIT_ONLY_ACE
#define AceFlagsInAce( _Ace) \
            (((PACE_HEADER)(_Ace))->AceFlags & (OBJECT_INHERIT_ACE|CONTAINER_INHERIT_ACE) | \
             (((PACE_HEADER)(_Ace))->AceFlags & INHERIT_ONLY_ACE) ^ INHERIT_ONLY_ACE )


BOOLEAN
RtlpCompareAces(
    IN PKNOWN_ACE InheritedAce,
    IN PKNOWN_ACE ChildAce,
    IN PSID OwnerSid,
    IN PSID GroupSid
    )
/*++

Routine Description:

    Compare two aces to see if they are "substantially" the same.

Arguments:

    InheritedAce - Computed ACE as inherited from DirectoryAcl.

    ChildAce - The current acl on the object.  This ACL must be a revision 2 ACL.

    ObjectType - GUID of the object type being created.  If the object being
        created has no GUID associated with it, then this argument is
        specified as NULL.

    OwnerSid - Specifies the owner Sid to use.
        If not specified, the owner sid is not treated as special.

    GroupSid - Specifies the group SID to use.
        If not specified, the group sid is not treated as special.

Return Value:

    TRUE - The ACEs are substantially the same.
    FALSE - The ACEs are not substantially the same.

--*/
{
    BOOLEAN AcesCompare = FALSE;

    if (IsObjectAceType(InheritedAce) && IsObjectAceType(ChildAce)) {

        AcesCompare = RtlpCompareKnownObjectAces( (PKNOWN_OBJECT_ACE)InheritedAce,
                                                  (PKNOWN_OBJECT_ACE)ChildAce,
                                                  OwnerSid,
                                                  GroupSid
                                                  );
    } else {

        if (!IsObjectAceType(InheritedAce) && !IsObjectAceType(ChildAce)) {

            AcesCompare = RtlpCompareKnownAces( InheritedAce,
                                                ChildAce,
                                                OwnerSid,
                                                GroupSid
                                                );
        }
    }

    return( AcesCompare );
}


BOOLEAN
RtlpCompareKnownAces(
    IN PKNOWN_ACE InheritedAce,
    IN PKNOWN_ACE ChildAce,
    IN PSID OwnerSid OPTIONAL,
    IN PSID GroupSid OPTIONAL
    )

/*++

Routine Description:

    Compare two aces to see if they are "substantially" the same.

Arguments:

    InheritedAce - Computed ACE as inherited from DirectoryAcl.

    ChildAce - The current acl on the object.  This ACL must be a revision 2 ACL.

    OwnerSid - Specifies the owner Sid to use.
        If not specified, the owner sid is not treated as special.

    GroupSid - Specifies the group SID to use.
        If not specified, the group sid is not treated as special.

Return Value:

    TRUE - The ACEs are substantially the same.
    FALSE - The ACEs are not substantially the same.

--*/

{
    NTSTATUS Status;
    ACE_HEADER volatile *InheritedAceHdr = &InheritedAce->Header;

    RTL_PAGED_CODE();

    ASSERT(!IsObjectAceType(InheritedAce));
    ASSERT(!IsObjectAceType(ChildAce));

    //
    // If the Ace types are different,
    //  we don't match.
    //
    if ( RtlBaseAceType[ChildAce->Header.AceType] != RtlBaseAceType[InheritedAceHdr->AceType] ) {
#if DBG
        if ( RtlpVerboseConvert ) {
            KdPrint(("AceType mismatch"));
        }
#endif // DBG
        return FALSE;
    }

    //
    // If this is a system ACE,
    //  ensure the SUCCESS/FAILURE flags match.
    //

    if ( RtlIsSystemAceType[ChildAce->Header.AceType] ) {
        if ( (ChildAce->Header.AceFlags & (SUCCESSFUL_ACCESS_ACE_FLAG|FAILED_ACCESS_ACE_FLAG)) !=
             (InheritedAceHdr->AceFlags & (SUCCESSFUL_ACCESS_ACE_FLAG|FAILED_ACCESS_ACE_FLAG)) ) {
#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("System ace success/fail mismatch"));
            }
#endif // DBG
            return FALSE;
        }
    }

    //
    // If the SID of the inherited ACE doesn't match,
    //  we don't match.
    //

    if ( !RtlEqualSid( (PSID)&ChildAce->SidStart, (PSID)&InheritedAce->SidStart )) {

        //
        // The inheritance algorithm only does SID mapping when building the effective
        //  ace.  So, we only check for a mapped SID if the child ACE is an effective ACE.
        //

        if ( AceFlagsInAce(ChildAce) != EFFECTIVE_ACE ) {
#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("SID mismatch"));
            }
#endif // DBG
            return FALSE;
        }

        //
        // In the case of CreatorOwner and CreatorGroup, the SIDs don't have to
        //  exactly match.  When the InheritedAce was generated, care was taken
        //  to NOT map these sids.  The SID may (or may not) have been mapped in
        //  the ChildAce.  We want to compare equal in both cases.
        //
        // If the InheritedAce contains a CreatorOwner/Group SID,
        //  do the another comparison of the SID in the child ACE with the
        //  real owner/group from the child security descriptor.
        //

        if ( OwnerSid != NULL || GroupSid != NULL ) {
            SID_IDENTIFIER_AUTHORITY  CreatorSidAuthority = SECURITY_CREATOR_SID_AUTHORITY;
            ULONG CreatorSid[CREATOR_SID_SIZE];

            //
            // Allocate and initialize the universal SIDs we're going to need
            // to look for inheritable ACEs.
            //

            ASSERT(RtlLengthRequiredSid( 1 ) == CREATOR_SID_SIZE);
            RtlInitializeSid( (PSID)CreatorSid, &CreatorSidAuthority, 1 );
            *(RtlpSubAuthoritySid( (PSID)CreatorSid, 0 )) = SECURITY_CREATOR_OWNER_RID;

            if (RtlEqualPrefixSid ( (PSID)&InheritedAce->SidStart, CreatorSid )) {
                ULONG Rid;

                Rid = *RtlpSubAuthoritySid( (PSID)&InheritedAce->SidStart, 0 );
                switch (Rid) {
                case SECURITY_CREATOR_OWNER_RID:
                    if ( OwnerSid == NULL ||
                         !RtlEqualSid( (PSID)&ChildAce->SidStart, OwnerSid )) {
#if DBG
                        if ( RtlpVerboseConvert ) {
                            KdPrint(("SID mismatch (Creator Owner)"));
                        }
#endif // DBG
                        return FALSE;
                    }
                    break;
                case SECURITY_CREATOR_GROUP_RID:
                    if ( GroupSid == NULL ||
                         !RtlEqualSid( (PSID)&ChildAce->SidStart, GroupSid )) {
#if DBG
                        if ( RtlpVerboseConvert ) {
                            KdPrint(("SID mismatch (Creator Group)"));
                        }
#endif // DBG
                        return FALSE;
                    }
                    break;
                default:
#if DBG
                    if ( RtlpVerboseConvert ) {
                        KdPrint(("SID mismatch (Creator)"));
                    }
#endif // DBG
                    return FALSE;
                }

            } else {
#if DBG
                if ( RtlpVerboseConvert ) {
                    KdPrint(("SID mismatch"));
                }
#endif // DBG
                return FALSE;
            }
        } else {
#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("SID mismatch"));
            }
#endif // DBG
            return FALSE;
        }
    }

    return TRUE;

}


BOOLEAN
RtlpCompareKnownObjectAces(
    IN PKNOWN_OBJECT_ACE InheritedAce,
    IN PKNOWN_OBJECT_ACE ChildAce,
    IN PSID OwnerSid OPTIONAL,
    IN PSID GroupSid OPTIONAL
    )

/*++

Routine Description:

    Compare two aces to see if they are "substantially" the same.

Arguments:

    InheritedAce - Computed ACE as inherited from DirectoryAcl.

    ChildAce - The current acl on the object.  This ACL must be a revision 2 ACL.

    ObjectType - GUID of the object type being created.  If the object being
        created has no GUID associated with it, then this argument is
        specified as NULL.

    OwnerSid - Specifies the owner Sid to use.
        If not specified, the owner sid is not treated as special.

    GroupSid - Specifies the group SID to use.
        If not specified, the group sid is not treated as special.

Return Value:

    TRUE - The ACEs are substantially the same.
    FALSE - The ACEs are not substantially the same.

--*/

{
    NTSTATUS Status;
    BOOLEAN DoingObjectAces;
    GUID *ChildObjectGuid;
    GUID *InhObjectGuid;
    GUID *ChildInheritedObjectGuid;
    GUID *InhInheritedObjectGuid;
    ACE_HEADER volatile *InheritedAceHdr = &InheritedAce->Header;

    RTL_PAGED_CODE();

    ASSERT(IsObjectAceType(InheritedAce));
    ASSERT(IsObjectAceType(ChildAce));
    //
    // If the Ace types are different,
    //  we don't match.
    //
    if ( RtlBaseAceType[ChildAce->Header.AceType] != RtlBaseAceType[InheritedAceHdr->AceType] ) {
#if DBG
        if ( RtlpVerboseConvert ) {
            KdPrint(("AceType mismatch"));
        }
#endif // DBG
        return FALSE;
    }

    //
    // If this is a system ACE,
    //  ensure the SUCCESS/FAILURE flags match.
    //

    if ( RtlIsSystemAceType[ChildAce->Header.AceType] ) {
        if ( (ChildAce->Header.AceFlags & (SUCCESSFUL_ACCESS_ACE_FLAG|FAILED_ACCESS_ACE_FLAG)) !=
             (InheritedAceHdr->AceFlags & (SUCCESSFUL_ACCESS_ACE_FLAG|FAILED_ACCESS_ACE_FLAG)) ) {
#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("System ace success/fail mismatch"));
            }
#endif // DBG
            return FALSE;
        }
    }

    //
    // Get the GUIDs from the Object Aces
    //

    ChildObjectGuid = RtlObjectAceObjectType(ChildAce);
    ChildInheritedObjectGuid = RtlObjectAceInheritedObjectType(ChildAce);

    InhObjectGuid = RtlObjectAceObjectType(InheritedAce);
    InhInheritedObjectGuid = RtlObjectAceInheritedObjectType(InheritedAce);

    //
    // If the InheritedObjectGuid is present in either ACE,
    //  they must be equal.
    //

    if ( ChildInheritedObjectGuid != NULL || InhInheritedObjectGuid != NULL ) {

        if ( ChildInheritedObjectGuid == NULL ||
             InhInheritedObjectGuid == NULL ||
             !RtlpIsEqualGuid( ChildInheritedObjectGuid, InhInheritedObjectGuid )) {
#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("InheritedObject GUID mismatch"));
            }
#endif // DBG
            return FALSE;
        }
    }

    //
    // If the ObjectGUID is present in either ACE,
    //  they must be equal.
    //
    // Any missing object GUID defaults to the passed in object GUID.
    //

    if ( (ChildObjectGuid != NULL) && (InhObjectGuid != NULL) ) {

        if (!RtlpIsEqualGuid( ChildObjectGuid, InhObjectGuid )) {
#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("Object GUID mismatch"));
            }
#endif // DBG

            return( FALSE );
        }
    } else {

        //
        // One or both is NULL, if it's only one, they don't match.
        //

        if ( !((ChildObjectGuid == NULL) && (InhObjectGuid == NULL)) ) {
#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("Object GUID mismatch"));
            }
#endif // DBG

            return( FALSE );
        }
    }

    //
    // If the SID of the inherited ACE doesn't match,
    //  we don't match.
    //

    if ( !RtlEqualSid( RtlObjectAceSid(ChildAce), RtlObjectAceSid(InheritedAce))) {

        //
        // The inheritance algorithm only does SID mapping when building the effective
        //  ace.  So, we only check for a mapped SID if the child ACE is an effective ACE.
        //

        if ( AceFlagsInAce(ChildAce) != EFFECTIVE_ACE ) {
#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("SID mismatch"));
            }
#endif // DBG
            return FALSE;
        }



        //
        // In the case of CreatorOwner and CreatorGroup, the SIDs don't have to
        //  exactly match.  When the InheritedAce was generated, care was taken
        //  to NOT map these sids.  The SID may (or may not) have been mapped in
        //  the ChildAce.  We want to compare equal in both cases.
        //
        // If the InheritedAce contains a CreatorOwner/Group SID,
        //  do the another comparison of the SID in the child ACE with the
        //  real owner/group from the child security descriptor.
        //

        if ( OwnerSid != NULL || GroupSid != NULL ) {
            SID_IDENTIFIER_AUTHORITY  CreatorSidAuthority = SECURITY_CREATOR_SID_AUTHORITY;
            ULONG CreatorSid[CREATOR_SID_SIZE];

            //
            // Allocate and initialize the universal SIDs we're going to need
            // to look for inheritable ACEs.
            //

            ASSERT(RtlLengthRequiredSid( 1 ) == CREATOR_SID_SIZE);
            RtlInitializeSid( (PSID)CreatorSid, &CreatorSidAuthority, 1 );
            *(RtlpSubAuthoritySid( (PSID)CreatorSid, 0 )) = SECURITY_CREATOR_OWNER_RID;

            if (RtlEqualPrefixSid ( RtlObjectAceSid(InheritedAce), CreatorSid )) {
                ULONG Rid;

                Rid = *RtlpSubAuthoritySid( RtlObjectAceSid(InheritedAce), 0 );
                switch (Rid) {
                case SECURITY_CREATOR_OWNER_RID:
                    if ( OwnerSid == NULL ||
                         !RtlEqualSid( RtlObjectAceSid(ChildAce), OwnerSid )) {
#if DBG
                        if ( RtlpVerboseConvert ) {
                            KdPrint(("SID mismatch (Creator Owner)"));
                        }
#endif // DBG
                        return FALSE;
                    }
                    break;
                case SECURITY_CREATOR_GROUP_RID:
                    if ( GroupSid == NULL ||
                         !RtlEqualSid( RtlObjectAceSid(ChildAce), GroupSid )) {
#if DBG
                        if ( RtlpVerboseConvert ) {
                            KdPrint(("SID mismatch (Creator Group)"));
                        }
#endif // DBG
                        return FALSE;
                    }
                    break;
                default:
#if DBG
                    if ( RtlpVerboseConvert ) {
                        KdPrint(("SID mismatch (Creator)"));
                    }
#endif // DBG
                    return FALSE;
                }

            } else {
#if DBG
                if ( RtlpVerboseConvert ) {
                    KdPrint(("SID mismatch"));
                }
#endif // DBG
                return FALSE;
            }
        } else {
#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("SID mismatch"));
            }
#endif // DBG
            return FALSE;
        }
    }

    return TRUE;

}




NTSTATUS
RtlpConvertAclToAutoInherit (
    IN PACL ParentAcl OPTIONAL,
    IN PACL ChildAcl,
    IN GUID *ObjectType OPTIONAL,
    IN BOOLEAN IsDirectoryObject,
    IN PSID OwnerSid,
    IN PSID GroupSid,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PACL *NewAcl,
    OUT PULONG NewGenericControl
    )

/*++

Routine Description:

    This is a private routine that produces an auto inherited acl from
    a ChildAcl that is not marked as auto inherited.  The passed in InheritedAcl
    is computed as the pure inherited ACL of Parent ACL of the object.

    See comments for RtlConvertToAutoInheritSecurityObject.

Arguments:

    ParentAcl - Supplies the ACL of the parent object.

    ChildAcl - Supplies the acl associated with the object.  This
        is the current acl on the object.

    ObjectType - GUID of the object type being created.  If the object being
        created has no GUID associated with it, then this argument is
        specified as NULL.

    IsDirectoryObject - Specifies if the object is a
        directory object.  A value of TRUE indicates the object is a
        container of other objects.

    OwnerSid - Specifies the owner Sid to use.

    GroupSid - Specifies the group SID to use.

    GenericMapping - Specifies the generic mapping to use.

    NewAcl - Receives a pointer to the new (auto inherited) acl.
        The ACL must be deallocated using the pool (kernel mode) or
            heap (user mode) deallocator.

    NewGenericControl - Specifies the control flags for the newly
        generated ACL.

        SEP_ACL_PRESENT: Specifies that the ACL is explictly supplied by
            the caller. ?? Ever set?

        SEP_ACL_DEFAULTED: Specifies that the ACL was supplied by some
            defaulting mechanism. ?? Ever set

        SEP_ACL_AUTO_INHERITED: Set if the ACL was generated using the
            Automatic Inheritance algorithm.

        SEP_ACL_PROTECTED: Specifies that the ACL is protected and
            was not inherited from the parent ACL.

Return Value:

    STATUS_SUCCESS - An inheritable ACL was successfully generated.

    STATUS_UNKNOWN_REVISION - Indicates the source ACL is a revision that
        is unknown to this routine.

    STATUS_INVALID_ACL - The structure of one of the ACLs in invalid.

--*/

{
    NTSTATUS Status;

    PACL InheritedAcl = NULL;
    PACL RealInheritedAcl = NULL;
    BOOLEAN AclExplicitlyAssigned;
    ULONG GenericControl;

    PKNOWN_ACE ChildAce = NULL;
    PKNOWN_ACE InheritedAce;

    LONG ChildAceIndex;
    LONG InheritedAceIndex;

    BOOLEAN InheritedAllowFound;
    BOOLEAN InheritedDenyFound;

    BOOLEAN AcesCompare;

    ACCESS_MASK InheritedContainerInheritMask;
    ACCESS_MASK InheritedObjectInheritMask;
    ACCESS_MASK InheritedEffectiveMask;
    ACCESS_MASK OriginalInheritedContainerInheritMask;
    ACCESS_MASK OriginalInheritedObjectInheritMask;
    ACCESS_MASK OriginalInheritedEffectiveMask;

    ULONG InheritedAceFlags;
    ULONG MatchedFlags;
    ULONG NonInheritedAclSize;


    PACE_HEADER AceHeader;
    PUCHAR Where;

    // ULONG i;

    //
    // This routine maintains an array of the structure below (one element per ACE in
    // the ChildAcl).
    //
    // The ACE is broken down into its component parts.  The access mask is triplicated.
    // That is, if the ACE is a ContainerInherit ACE, the access mask is remembered as
    // being a "ContainerInheritMask".  The same is true if the ACE is an ObjectInherit ACE
    // on an effective ACE.  This is done since each of the resultant 96 bits are
    // individually matched against corresponding bits in the computed inherited ACE.
    //
    // Each of the above mentioned masks are maintained in two forms.  The first is never
    // changed and represents the bits as the originally appeared in the child ACL.
    // This second is modified as the corresponding bits are matched in the inherited ACL.
    // When the algorithm is completed, bits that haven't been matched represent ACEs
    // that weren't inherited from the parent.
    //

    typedef struct {
        ACCESS_MASK OriginalContainerInheritMask;
        ACCESS_MASK OriginalObjectInheritMask;
        ACCESS_MASK OriginalEffectiveMask;

        ACCESS_MASK ContainerInheritMask;
        ACCESS_MASK ObjectInheritMask;
        ACCESS_MASK EffectiveMask;
    } ACE_INFO, *PACE_INFO;

    PACE_INFO ChildAceInfo = NULL;

    ULONG CreatorOwnerSid[CREATOR_SID_SIZE];
    ULONG CreatorGroupSid[CREATOR_SID_SIZE];

    SID_IDENTIFIER_AUTHORITY  CreatorSidAuthority = SECURITY_CREATOR_SID_AUTHORITY;

#ifndef NTOS_KERNEL_RUNTIME
    PVOID HeapHandle;
#endif // NTOS_KERNEL_RUNTIME

    RTL_PAGED_CODE();

    //
    // Get the handle to the current process heap
    //

#ifndef NTOS_KERNEL_RUNTIME
    HeapHandle = RtlProcessHeap();
#endif // NTOS_KERNEL_RUNTIME

    //
    // Allocate and initialize the universal SIDs we're going to need
    // to look for inheritable ACEs.
    //

    ASSERT(RtlLengthRequiredSid( 1 ) == CREATOR_SID_SIZE);
    RtlInitializeSid( (PSID)CreatorOwnerSid, &CreatorSidAuthority, 1 );
    *(RtlpSubAuthoritySid( (PSID)CreatorOwnerSid, 0 )) = SECURITY_CREATOR_OWNER_RID;

    RtlInitializeSid( (PSID)CreatorGroupSid, &CreatorSidAuthority, 1 );
    *(RtlpSubAuthoritySid( (PSID)CreatorGroupSid, 0 )) = SECURITY_CREATOR_GROUP_RID;

    //
    // Ensure the passed in ACLs are valid.
    //

    *NewGenericControl = SEP_ACL_AUTO_INHERITED;
    *NewAcl = NULL;

    if ( ParentAcl != NULL && !RtlValidAcl( ParentAcl ) ) {
        Status = STATUS_INVALID_ACL;
        goto Cleanup;
    }

    if (!RtlValidAcl( ChildAcl ) ) {
        Status = STATUS_INVALID_ACL;
        goto Cleanup;
    }


    //
    // Compute what the inherited ACL "should" look like.
    //
    // The inherited ACL is computed to NOT SID-map Creator Owner and Creator Group.
    // This allows use to later recognize the constant SIDs and special case them
    // rather than mistakenly confuse them with the mapped SID.
    //

    Status = RtlpInheritAcl (
                ParentAcl,
                NULL,   // No explicit child ACL
                0,      // No Child Generic Control
                IsDirectoryObject,
                TRUE,   // AutoInherit the DACL
                FALSE,  // Not default descriptor for object
                CreatorOwnerSid,   // Subsitute a constant SID
                CreatorGroupSid,   // Subsitute a constant SID
                CreatorOwnerSid,   // Server Owner (Technically incorrect, but OK since we don't support compound ACEs)
                CreatorGroupSid,   // Server Group
                GenericMapping,
                TRUE,   // Is a SACL
                ObjectType,
                &InheritedAcl,
                &AclExplicitlyAssigned,
                &GenericControl );

    if ( Status == STATUS_NO_INHERITANCE ) {
        *NewGenericControl |= SEP_ACL_PROTECTED;
#if DBG
        if ( RtlpVerboseConvert ) {
            KdPrint(("NO_INHERITANCE of the parent ACL\n" ));
        }
#endif // DBG
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    if ( !NT_SUCCESS(Status) ) {
#if DBG
        if ( RtlpVerboseConvert ) {
            KdPrint(("Can't build inherited ACL %lX\n", Status ));
        }
#endif // DBG
        goto Cleanup;
    }





    //
    // Allocate a work buffer describing the ChildAcl
    //

#ifdef NTOS_KERNEL_RUNTIME
    ChildAceInfo = ExAllocatePoolWithTag(
                        PagedPool,
                        ChildAcl->AceCount * sizeof(ACE_INFO),
                        'cAeS' );
#else // NTOS_KERNEL_RUNTIME
    ChildAceInfo = RtlAllocateHeap(
                        HeapHandle,
                        MAKE_TAG(SE_TAG),
                        ChildAcl->AceCount * sizeof(ACE_INFO) );
#endif // NTOS_KERNEL_RUNTIME

    if (ChildAceInfo == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    for (ChildAceIndex = 0, ChildAce = FirstAce(ChildAcl);
         ChildAceIndex < ChildAcl->AceCount;
         ChildAceIndex += 1, ChildAce = NextAce(ChildAce)) {
        ACCESS_MASK LocalMask;
        ULONG ChildAceFlags;

        if ( !IsV4AceType(ChildAce) || IsCompoundAceType(ChildAce)) {
             *NewGenericControl |= SEP_ACL_PROTECTED;
#if DBG
             if ( RtlpVerboseConvert ) {
                 KdPrint(("Inherited Ace type (%ld) not known\n", ChildAce->Header.AceType ));
             }
#endif // DBG
             Status = STATUS_SUCCESS;
             goto Cleanup;
        }

        //
        // Compute the generic mapped mask for use in all comparisons.  The
        //  generic mapping will be undone if needed later.
        //
        // All V4 aces have an access mask in the same location.
        //
        LocalMask = ((PKNOWN_ACE)(ChildAce))->Mask;
        RtlApplyGenericMask( ChildAce, &LocalMask, GenericMapping);


        //
        // Break the ACE into its component parts.
        //
        ChildAceFlags = AceFlagsInAce( ChildAce );
        if ( ChildAceFlags & CONTAINER_INHERIT_ACE ) {
            ChildAceInfo[ChildAceIndex].OriginalContainerInheritMask = LocalMask;
            ChildAceInfo[ChildAceIndex].ContainerInheritMask = LocalMask;
        } else {
            ChildAceInfo[ChildAceIndex].OriginalContainerInheritMask = 0;
            ChildAceInfo[ChildAceIndex].ContainerInheritMask = 0;
        }

        if ( ChildAceFlags & OBJECT_INHERIT_ACE ) {
            ChildAceInfo[ChildAceIndex].OriginalObjectInheritMask = LocalMask;
            ChildAceInfo[ChildAceIndex].ObjectInheritMask = LocalMask;
        } else {
            ChildAceInfo[ChildAceIndex].OriginalObjectInheritMask = 0;
            ChildAceInfo[ChildAceIndex].ObjectInheritMask = 0;
        }

        if ( ChildAceFlags & EFFECTIVE_ACE ) {
            ChildAceInfo[ChildAceIndex].OriginalEffectiveMask = LocalMask;
            ChildAceInfo[ChildAceIndex].EffectiveMask = LocalMask;
        } else {
            ChildAceInfo[ChildAceIndex].OriginalEffectiveMask = 0;
            ChildAceInfo[ChildAceIndex].EffectiveMask = 0;
        }

    }


    //
    // Walk through the computed inherited ACL one ACE at a time.
    //

    for (InheritedAceIndex = 0, InheritedAce = FirstAce(InheritedAcl);
         InheritedAceIndex < InheritedAcl->AceCount;
         InheritedAceIndex += 1, InheritedAce = NextAce(InheritedAce)) {

        ACCESS_MASK LocalMask;

        //
        // If the ACE isn't a valid version 4 ACE,
        //  this isn't an ACL we're interested in handling.
        //

        if ( !IsV4AceType(InheritedAce) || IsCompoundAceType(InheritedAce)) {
             *NewGenericControl |= SEP_ACL_PROTECTED;
#if DBG
             if ( RtlpVerboseConvert ) {
                 KdPrint(("Inherited Ace type (%ld) not known\n", InheritedAce->Header.AceType ));
             }
#endif // DBG
             Status = STATUS_SUCCESS;
             goto Cleanup;
        }

        //
        // Compute the generic mapped mask for use in all comparisons.  The
        //  generic mapping will be undone if needed later.
        //
        // All V4 aces have an access mask in the same location.
        //
        LocalMask = ((PKNOWN_ACE)(InheritedAce))->Mask;
        RtlApplyGenericMask( InheritedAce, &LocalMask, GenericMapping);

        if ( LocalMask == 0 ) {
#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("Worthless INH ACE: %ld 0x%8.8lx\n", InheritedAceIndex, LocalMask ));
            }
#endif // DBG
            continue;
        }

        //
        // This ACE is some combination of an effective ACE, a container
        //  inherit ACE and an object inherit ACE.  Process each of those
        //  attributes separately since they might be represented separately
        //  in the ChildAcl.
        //

        InheritedAceFlags = AceFlagsInAce( InheritedAce );

        if  ( InheritedAceFlags == 0 ) {
#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("Worthless INH ACE: %ld 0x%lx\n", InheritedAceIndex, InheritedAceFlags ));
            }
#endif // DBG
            continue;
        }

        if ( InheritedAceFlags & CONTAINER_INHERIT_ACE ) {
            OriginalInheritedContainerInheritMask = InheritedContainerInheritMask = LocalMask;
        } else {
            OriginalInheritedContainerInheritMask = InheritedContainerInheritMask = 0;
        }

        if ( InheritedAceFlags & OBJECT_INHERIT_ACE ) {
            OriginalInheritedObjectInheritMask = InheritedObjectInheritMask = LocalMask;
        } else {
            OriginalInheritedObjectInheritMask = InheritedObjectInheritMask = 0;
        }

        if ( InheritedAceFlags & EFFECTIVE_ACE ) {
            OriginalInheritedEffectiveMask = InheritedEffectiveMask = LocalMask;
        } else {
            OriginalInheritedEffectiveMask = InheritedEffectiveMask = 0;
        }

#if DBG
        if ( RtlpVerboseConvert ) {
            KdPrint(("Doing INH ACE:  %ld %8.8lX %8.8lX %8.8lX\n", InheritedAceIndex, InheritedEffectiveMask, InheritedContainerInheritMask, InheritedObjectInheritMask ));
        }
#endif // DBG


        //
        // Loop through the entire child ACL comparing each inherited ACE with
        //  each child ACE.  Don't stop simply because we've matched once.
        //  Multiple ACEs in the one ACL may have been condensed into a single ACE
        //  in the other ACL in any combination (by any of our friendly ACL editors).
        //  In all cases, it is better to compute a resultant auto inherited ACL
        //  than it is to compute a protected ACL.
        //

        for (ChildAceIndex = 0, ChildAce = FirstAce(ChildAcl);
             ChildAceIndex < ChildAcl->AceCount;
             ChildAceIndex += 1, ChildAce = NextAce(ChildAce)) {


            //
            // Ensure the ACE represents the same principal and object,
            //

#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("Compare Child Ace: %ld ", ChildAceIndex ));
            }
#endif // DBG

            if ( !RtlpCompareAces( InheritedAce,
                                   ChildAce,
                                   OwnerSid,
                                   GroupSid ) ) {
#if DBG
                if ( RtlpVerboseConvert ) {
                    KdPrint(("\n" ));
                }
#endif // DBG
                continue;
            }
#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("\n" ));
            }
#endif // DBG


            //
            // Match as many access bits in the INH ACE as possible.
            //
            // Don't pay any attention to whether the bits have been previously matched
            // in the CHILD ACE.  To do so, would imply that there is a one-to-one
            // correspondance between bits in the INH ACL and Child ACL.  Unfortunately,
            // ACL editors feel free to compress out duplicate bits in both
            // the CHILD ACL and PARENT ACL as they see fit.
            //

            InheritedEffectiveMask &= ~ChildAceInfo[ChildAceIndex].OriginalEffectiveMask;
            InheritedContainerInheritMask &= ~ChildAceInfo[ChildAceIndex].OriginalContainerInheritMask;
            InheritedObjectInheritMask &= ~ChildAceInfo[ChildAceIndex].OriginalObjectInheritMask;

#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("New   INH MASKs %ld %8.8lX %8.8lX %8.8lX\n", InheritedAceIndex, InheritedEffectiveMask, InheritedContainerInheritMask, InheritedObjectInheritMask ));
            }
#endif // DBG


            //
            // Match as many access bits in the child ACE as possible.
            //
            // Same reasoning as above.
            //

            ChildAceInfo[ChildAceIndex].EffectiveMask &= ~OriginalInheritedEffectiveMask;
            ChildAceInfo[ChildAceIndex].ContainerInheritMask &= ~OriginalInheritedContainerInheritMask;
            ChildAceInfo[ChildAceIndex].ObjectInheritMask &= ~OriginalInheritedObjectInheritMask;

#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("New Child MASKs %ld %8.8lX %8.8lX %8.8lX\n", ChildAceIndex, ChildAceInfo[ChildAceIndex].EffectiveMask, ChildAceInfo[ChildAceIndex].ContainerInheritMask, ChildAceInfo[ChildAceIndex].ObjectInheritMask ));
            }
#endif // DBG

        }


        //
        // If we couldn't process this inherited ACE,
        //  then the child ACL wasn't inherited.
        //

        if ( (InheritedEffectiveMask | InheritedContainerInheritMask | InheritedObjectInheritMask) != 0 ) {
            *NewGenericControl |= SEP_ACL_PROTECTED;
#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("INH ACE not completely matched: %ld %8.8lX %8.8lX %8.8lX\n", InheritedAceIndex, InheritedEffectiveMask, InheritedContainerInheritMask, InheritedObjectInheritMask ));
            }
#endif // DBG
            Status = STATUS_SUCCESS;
            goto Cleanup;
        }


    }

    //
    // ASSERT: All of the inherited ACEs have been processed.
    //

    //
    // Loop through the Child ACL ensuring we can build a valid auto inherited ACL
    //

    InheritedAllowFound = FALSE;
    InheritedDenyFound = FALSE;
    NonInheritedAclSize = 0;
    for (ChildAceIndex = 0, ChildAce = FirstAce(ChildAcl);
         ChildAceIndex < ChildAcl->AceCount;
         ChildAceIndex += 1, ChildAce = NextAce(ChildAce)) {

        ACCESS_MASK ResultantMask;

        //
        // Any Child ACE access bits not eliminated above required than an
        //  explicit non-inherited ACE by built.  That ACE will have an
        //  access mask that is the combined access mask of the unmatched bit
        //  in the effective, container inherit, and object inherit categories.
        //  Even though, the combined mask may include access bits not absolutely
        //  required (since they were already inherited), this strategy prevents
        //  us from having to build multiple ACEs (one for each category) for this
        //  single ACE.
        //

        ResultantMask =
            ChildAceInfo[ChildAceIndex].EffectiveMask |
            ChildAceInfo[ChildAceIndex].ContainerInheritMask |
            ChildAceInfo[ChildAceIndex].ObjectInheritMask;


        //
        // Handle an inherited ACE
        //

        if ( ResultantMask == 0 ) {

            //
            // Keep track of whether inherited "allow" and "deny" ACEs are found.
            //

            if ( RtlBaseAceType[ChildAce->Header.AceType] == ACCESS_ALLOWED_ACE_TYPE ) {
                InheritedAllowFound = TRUE;
            }

            if ( RtlBaseAceType[ChildAce->Header.AceType] == ACCESS_DENIED_ACE_TYPE ) {
                InheritedDenyFound = TRUE;
            }

        //
        // Handle a non-inherited ACE
        //

        } else {

            //
            // Keep a running tab of the size of the non-inherited ACEs.
            //

            NonInheritedAclSize += ChildAce->Header.AceSize;

            //
            // Since non-inherited ACEs will be moved to the front of the ACL,
            //  we have to be careful that we don't move a deny ACE in front of a
            //  previously found inherited allow ACE (and vice-versa).  To do so would
            //  change the semantics of the ACL.
            //

            if ( RtlBaseAceType[ChildAce->Header.AceType] == ACCESS_ALLOWED_ACE_TYPE && InheritedDenyFound ) {
                *NewGenericControl |= SEP_ACL_PROTECTED;
#if DBG
                if ( RtlpVerboseConvert ) {
                    KdPrint(("Previous deny found Child ACE: %ld\n", ChildAceIndex ));
                }
#endif // DBG
                Status = STATUS_SUCCESS;
                goto Cleanup;
            }

            if ( RtlBaseAceType[ChildAce->Header.AceType] == ACCESS_DENIED_ACE_TYPE && InheritedAllowFound ) {
                *NewGenericControl |= SEP_ACL_PROTECTED;
#if DBG
                if ( RtlpVerboseConvert ) {
                    KdPrint(("Previous allow found Child ACE: %ld\n", ChildAceIndex ));
                }
#endif // DBG
                Status = STATUS_SUCCESS;
                goto Cleanup;
            }

        }

    }

    //
    // The resultant ACL is composed of the non-inherited ACEs followed by
    // the inherited ACE. The inherited ACEs are built by running the
    // inheritance algorithm over the Parent ACL.
    //
    // The Inherited ACL computed below is almost identical to InhertedAcl.
    // However, InheritedAcl didn't properly substitute the correct owner and
    // group SID.
    //

    Status = RtlpInheritAcl (
                ParentAcl,
                NULL,   // No explicit child ACL
                0,      // No Child Generic Control
                IsDirectoryObject,
                TRUE,   // AutoInherit the DACL
                FALSE,  // Not default descriptor for object
                OwnerSid,   // Subsitute a constant SID
                GroupSid,   // Subsitute a constant SID
                OwnerSid,   // Server Owner (Technically incorrect, but OK since we don't support compound ACEs)
                GroupSid,   // Server Group
                GenericMapping,
                TRUE,   // Is a SACL
                ObjectType,
                &RealInheritedAcl,
                &AclExplicitlyAssigned,
                &GenericControl );

    if ( !NT_SUCCESS(Status) ) {
#if DBG
        if ( RtlpVerboseConvert ) {
            KdPrint(("Can't build real inherited ACL %lX\n", Status ));
        }
#endif // DBG
        goto Cleanup;
    }



    //
    // Allocate a buffer for the inherited ACL
    //

#ifdef NTOS_KERNEL_RUNTIME
    *NewAcl = ExAllocatePoolWithTag(
                        PagedPool,
                        RealInheritedAcl->AclSize + NonInheritedAclSize,
                        'cAeS' );
#else // NTOS_KERNEL_RUNTIME
    *NewAcl = RtlAllocateHeap(
                        HeapHandle,
                        MAKE_TAG(SE_TAG),
                        RealInheritedAcl->AclSize + NonInheritedAclSize );
#endif // NTOS_KERNEL_RUNTIME

    if ( *NewAcl == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // All non-inherited ACEs are copied first.
    // The inherited ACES are grabbed from real inherited ACL.
    //
    // Build an ACL Header.
    //

    Status = RtlCreateAcl( *NewAcl,
                           RealInheritedAcl->AclSize + NonInheritedAclSize,
                           max( RealInheritedAcl->AclRevision, ChildAcl->AclRevision ) );

    if ( !NT_SUCCESS(Status) ) {
#if DBG
        if ( RtlpVerboseConvert ) {
            KdPrint(("Can't create final ACL %lX\n", Status ));
        }
#endif // DBG
        //
        // The only reason for failure would be if the combined ACL is too large.
        // So just create a protected ACL (better than a failure).
        //
        *NewGenericControl |= SEP_ACL_PROTECTED;
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // Copy the non-inherited ACES.
    //

    Where = ((PUCHAR)(*NewAcl)) + sizeof(ACL);
    for (ChildAceIndex = 0, ChildAce = FirstAce(ChildAcl);
         ChildAceIndex < ChildAcl->AceCount;
         ChildAceIndex += 1, ChildAce = NextAce(ChildAce)) {

        ACCESS_MASK ResultantMask;

        //
        // Copy the non-inherited ACE from the Child only if there's a non-zero access mask.
        //

        ResultantMask =
            ChildAceInfo[ChildAceIndex].EffectiveMask |
            ChildAceInfo[ChildAceIndex].ContainerInheritMask |
            ChildAceInfo[ChildAceIndex].ObjectInheritMask;

        if ( ResultantMask != 0 ) {
            PKNOWN_ACE NewAce;
            ULONG GenericBitToTry;

            //
            // Use the original ChildAce as the template.
            //

            RtlCopyMemory( Where, ChildAce, ChildAce->Header.AceSize );
            NewAce = (PKNOWN_ACE)Where;
            NewAce->Header.AceFlags &= ~INHERITED_ACE;  // Clear stray bits
            Where += ChildAce->Header.AceSize;

            (*NewAcl)->AceCount ++;

            //
            // The AccessMask on the ACE are those access bits that didn't get matched
            //  by inherited ACEs.
            //

            NewAce->Mask = ChildAce->Mask & ResultantMask;
            ResultantMask &= ~ChildAce->Mask;
#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("Original non-inherited: %ld %8.8lX %8.8lX\n", ChildAceIndex, NewAce->Mask, ResultantMask ));
            }
#endif // DBG

            //
            // Map any remaining bits back to generic access bits.
            // Doing so might expand the ResultantMask to beyond what was computed above.
            // Doing so will never expand the computed ACE to beyond what the original
            //  ChildAce granted.
            //

            ASSERT( GENERIC_WRITE == (GENERIC_READ >> 1));
            ASSERT( GENERIC_EXECUTE == (GENERIC_WRITE >> 1));
            ASSERT( GENERIC_ALL == (GENERIC_EXECUTE >> 1));

            GenericBitToTry = GENERIC_READ;

            while ( ResultantMask && GenericBitToTry >= GENERIC_ALL ) {

                //
                // Only map generic bits that are in the ChildAce.
                //

                if ( GenericBitToTry & ChildAce->Mask ) {
                    ACCESS_MASK GenericMask;

                    //
                    // Compute the real access mask corresponding to the Generic bit.
                    //

                    GenericMask = GenericBitToTry;
                    RtlMapGenericMask( &GenericMask, GenericMapping );

                    //
                    // If the current generic bit matches any of the bits remaining,
                    //  set the generic bit in the current ACE.
                    //

                    if ( (ResultantMask & GenericMask) != 0 ) {
                        NewAce->Mask |= GenericBitToTry;
                        ResultantMask &= ~GenericMask;
                    }
#if DBG
                    if ( RtlpVerboseConvert ) {
                        KdPrint(("Generic  non-inherited: %ld %8.8lX %8.8lX\n", ChildAceIndex, NewAce->Mask, ResultantMask ));
                    }
#endif // DBG
                }

                //
                // Try the next Generic bit.
                //

                GenericBitToTry >>= 1;
            }


            //
            // This is really an internal error, but press on regardless.
            //

            ASSERT(ResultantMask == 0 );
            NewAce->Mask |= ResultantMask;
#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("Final    non-inherited: %ld %8.8lX %8.8lX\n", ChildAceIndex, NewAce->Mask, ResultantMask ));
            }
#endif // DBG

        }
    }

    //
    // Copy the inherited ACES.
    //  Simply copy computed Inherited ACL.
    //

    RtlCopyMemory( Where,
                   FirstAce(RealInheritedAcl),
                   RealInheritedAcl->AclSize - (ULONG)(((PUCHAR)FirstAce(RealInheritedAcl)) - (PUCHAR)RealInheritedAcl));
    Where += RealInheritedAcl->AclSize - (ULONG)(((PUCHAR)FirstAce(RealInheritedAcl)) - (PUCHAR)RealInheritedAcl);

    (*NewAcl)->AceCount += RealInheritedAcl->AceCount;
    ASSERT( (*NewAcl)->AclSize == Where - (PUCHAR)(*NewAcl) );


    Status = STATUS_SUCCESS;
Cleanup:

    //
    // If successful,
    //  build the resultant autoinherited ACL.
    //

    if ( NT_SUCCESS(Status) ) {

        //
        // If the Child ACL is protected,
        //  just build it as a copy of the original ACL
        //

        if ( *NewGenericControl & SEP_ACL_PROTECTED ) {

            //
            // If we've already allocated a new ACL (and couldn't finish it for some reason),
            //  free it.

            if ( *NewAcl != NULL) {
#ifdef NTOS_KERNEL_RUNTIME
                ExFreePool( *NewAcl );
#else // NTOS_KERNEL_RUNTIME
                RtlFreeHeap( HeapHandle, 0, *NewAcl );
#endif // NTOS_KERNEL_RUNTIME
                *NewAcl = NULL;
            }

            //
            // Allocate a buffer for the protected ACL.
            //

#ifdef NTOS_KERNEL_RUNTIME
            *NewAcl = ExAllocatePoolWithTag(
                                PagedPool,
                                ChildAcl->AclSize,
                                'cAeS' );
#else // NTOS_KERNEL_RUNTIME
            *NewAcl = RtlAllocateHeap(
                                HeapHandle,
                                MAKE_TAG(SE_TAG),
                                ChildAcl->AclSize );
#endif // NTOS_KERNEL_RUNTIME

            if ( *NewAcl == NULL ) {
                Status = STATUS_NO_MEMORY;
            } else {
                RtlCopyMemory( *NewAcl, ChildAcl, ChildAcl->AclSize );
            }
        }

    }

    if ( ChildAceInfo != NULL) {
#ifdef NTOS_KERNEL_RUNTIME
        ExFreePool( ChildAceInfo );
#else // NTOS_KERNEL_RUNTIME
        RtlFreeHeap( HeapHandle, 0, ChildAceInfo );
#endif // NTOS_KERNEL_RUNTIME
    }

    if ( InheritedAcl != NULL) {
#ifdef NTOS_KERNEL_RUNTIME
        ExFreePool( InheritedAcl );
#else // NTOS_KERNEL_RUNTIME
        RtlFreeHeap( HeapHandle, 0, InheritedAcl );
#endif // NTOS_KERNEL_RUNTIME
    }

    if ( RealInheritedAcl != NULL) {
#ifdef NTOS_KERNEL_RUNTIME
        ExFreePool( RealInheritedAcl );
#else // NTOS_KERNEL_RUNTIME
        RtlFreeHeap( HeapHandle, 0, RealInheritedAcl );
#endif // NTOS_KERNEL_RUNTIME
    }

    return Status;
}


BOOLEAN
RtlpIsDuplicateAce(
    IN PACL Acl,
    IN PKNOWN_ACE NewAce,
    IN GUID *ObjectType OPTIONAL
    )

/*++

Routine Description:

    This routine determine if an ACE is a duplicate of an ACE already in an
    ACL.  If so, the NewAce can be removed from the end of the ACL.

    This routine currently only detects duplicate version 4 ACEs.  If the
    ACE isn't version 4, the ACE will be declared to be a non-duplicate.

    This routine only detects duplicate INHERTED ACEs.

Arguments:

    Acl - Existing ACL

    NewAce - Ace to determine if it is already in Acl.
        NewAce is expected to be the last ACE in "Acl".

    ObjectType - GUID of the object type represented by Acl.  If the object
        has no GUID associated with it, then this argument is
        specified as NULL.

Return Value:

    TRUE - NewAce is a duplicate of another ACE on the Acl
    FALSE - NewAce is NOT a duplicate of another ACE on the Acl

--*/

{
    NTSTATUS Status;
    BOOLEAN RetVal = FALSE;

    LONG AceIndex;

    ACCESS_MASK NewAceContainerInheritMask;
    ACCESS_MASK NewAceObjectInheritMask;
    ACCESS_MASK NewAceEffectiveMask;

    ACCESS_MASK LocalMask;

    PKNOWN_ACE AceFromAcl;

    RTL_PAGED_CODE();


    //
    // Ensure the passed in ACE is one this routine understands
    //

    if ( !IsV4AceType(NewAce) || IsCompoundAceType(NewAce)) {
#if DBG
        if ( RtlpVerboseConvert ) {
            KdPrint(("New Ace type (%ld) not known\n", NewAce->Header.AceType ));
        }
#endif // DBG
        RetVal = FALSE;
        goto Cleanup;
    }

    //
    // This routine only works for ACEs marked as INHERITED.
    //

    if ( (NewAce->Header.AceFlags & INHERITED_ACE ) == 0 ) {
#if DBG
        if ( RtlpVerboseConvert ) {
            KdPrint(("New Ace type isn't inherited\n" ));
        }
#endif // DBG
        RetVal = FALSE;
        goto Cleanup;
    }


    //
    // Break the new ACE into its component parts.
    //
    // All V4 aces have an access mask in the same location.
    //
    LocalMask = ((PKNOWN_ACE)(NewAce))->Mask;

    if ( NewAce->Header.AceFlags & CONTAINER_INHERIT_ACE ) {
        NewAceContainerInheritMask = LocalMask;
    } else {
        NewAceContainerInheritMask = 0;
    }

    if ( NewAce->Header.AceFlags & OBJECT_INHERIT_ACE ) {
        NewAceObjectInheritMask = LocalMask;
    } else {
        NewAceObjectInheritMask = 0;
    }

    if ( (NewAce->Header.AceFlags & INHERIT_ONLY_ACE) == 0 ) {
        NewAceEffectiveMask = LocalMask;
    } else {
        NewAceEffectiveMask = 0;
    }
#if DBG
    if ( RtlpVerboseConvert ) {
        KdPrint(("Starting MASKs:  %8.8lX %8.8lX %8.8lX", NewAceEffectiveMask, NewAceContainerInheritMask, NewAceObjectInheritMask ));
    }
#endif // DBG




    //
    // Walk through the ACL one ACE at a time.
    //

    for (AceIndex = 0, AceFromAcl = FirstAce(Acl);
         AceIndex < Acl->AceCount-1;    // NewAce is the last ACE
         AceIndex += 1, AceFromAcl = NextAce(AceFromAcl)) {


        //
        // If the ACE isn't a valid version 4 ACE,
        //  this isn't an ACE we're interested in handling.
        //

        if ( !IsV4AceType(AceFromAcl) || IsCompoundAceType(AceFromAcl)) {
            continue;
        }

        //
        // This routine only works for ACEs marked as INHERITED.
        //

        if ( (AceFromAcl->Header.AceFlags & INHERITED_ACE ) == 0 ) {
            continue;
        }


        //
        // Compare the Ace from the ACL with the New ACE
        //
        //  Don't stop simply because we've matched once.
        //  Multiple ACEs in the one ACL may have been condensed into a single ACE
        //  in the other ACL in any combination (by any of our friendly ACL editors).
        //
#if DBG
        if ( RtlpVerboseConvert ) {
            KdPrint(("Compare Ace: %ld ", AceIndex ));
        }
#endif // DBG

        if ( RtlpCompareAces( AceFromAcl,
                              NewAce,
                              NULL,
                              NULL ) ) {


            //
            // Match the bits from the current ACE with bits from the New ACE.
            //
            // All V4 aces have an access mask in the same location.
            //

            LocalMask = ((PKNOWN_ACE)(AceFromAcl))->Mask;

            if ( AceFromAcl->Header.AceFlags & CONTAINER_INHERIT_ACE ) {
                NewAceContainerInheritMask &= ~LocalMask;
            }

            if ( AceFromAcl->Header.AceFlags & OBJECT_INHERIT_ACE ) {
                NewAceObjectInheritMask &= ~LocalMask;
            }

            if ( (AceFromAcl->Header.AceFlags & INHERIT_ONLY_ACE) == 0 ) {
                NewAceEffectiveMask &= ~LocalMask;
            }

#if DBG
            if ( RtlpVerboseConvert ) {
                KdPrint(("Remaining MASKs:  %8.8lX %8.8lX %8.8lX", NewAceEffectiveMask, NewAceContainerInheritMask, NewAceObjectInheritMask ));
            }
#endif // DBG

            //
            // If all bits have been matched in the New Ace,
            //  then this is a duplicate ACE.
            //

            if ( (NewAceEffectiveMask | NewAceContainerInheritMask | NewAceObjectInheritMask) == 0 ) {
#if DBG
                if ( RtlpVerboseConvert ) {
                    KdPrint(("\n"));
                }
#endif // DBG
                RetVal = TRUE;
                goto Cleanup;
            }
        }
#if DBG
        if ( RtlpVerboseConvert ) {
              KdPrint(("\n"));
        }
#endif // DBG


    }

    //
    // All of the ACEs of the ACL have been processed.
    //
    // We haven't matched all of the bits in the New Ace so this is not a duplicate ACE.
    //

    RetVal = FALSE;
Cleanup:

    return RetVal;

}


NTSTATUS
RtlpCreateServerAcl(
    IN PACL Acl,
    IN BOOLEAN AclUntrusted,
    IN PSID ServerSid,
    OUT PACL *ServerAcl,
    OUT BOOLEAN *ServerAclAllocated
    )

/*++

Routine Description:

    This routine takes an ACL and converts it into a server ACL.
    Currently, that means converting all of the GRANT ACEs into
    Compount Grants, and if necessary sanitizing any Compound
    Grants that are encountered.

Arguments:



Return Value:


--*/

{
    USHORT RequiredSize = sizeof(ACL);
    USHORT AceSizeAdjustment;
    USHORT ServerSidSize;
    PACE_HEADER Ace;
    ULONG i;
    PVOID Target;
    PVOID AcePosition;
    PSID UntrustedSid;
    PSID ClientSid;
    NTSTATUS Status;

    RTL_PAGED_CODE();

    if (Acl == NULL) {
        *ServerAclAllocated = FALSE;
        *ServerAcl = NULL;
        return( STATUS_SUCCESS );
    }

    AceSizeAdjustment = sizeof( KNOWN_COMPOUND_ACE ) - sizeof( KNOWN_ACE );
    ASSERT( sizeof( KNOWN_COMPOUND_ACE ) >= sizeof( KNOWN_ACE ) );

    ServerSidSize = (USHORT)SeLengthSid( ServerSid );

    //
    // Do this in two passes.  First, determine how big the final
    // result is going to be, and then allocate the space and make
    // the changes.
    //

    for (i = 0, Ace = FirstAce(Acl);
         i < Acl->AceCount;
         i += 1, Ace = NextAce(Ace)) {

        //
        // If it's an ACCESS_ALLOWED_ACE_TYPE, we'll need to add in the
        // size of the Server SID.
        //

        if (Ace->AceType == ACCESS_ALLOWED_ACE_TYPE) {

            //
            // Simply add the size of the new Server SID plus whatever
            // adjustment needs to be made to increase the size of the ACE.
            //

            RequiredSize += ( ServerSidSize + AceSizeAdjustment );

        } else {

            if (AclUntrusted && Ace->AceType == ACCESS_ALLOWED_COMPOUND_ACE_TYPE ) {

                //
                // Since the Acl is untrusted, we don't care what is in the
                // server SID, we're going to replace it.
                //

                UntrustedSid = RtlCompoundAceServerSid( Ace );
                if ((USHORT)SeLengthSid(UntrustedSid) > ServerSidSize) {
                    RequiredSize += ((USHORT)SeLengthSid(UntrustedSid) - ServerSidSize);
                } else {
                    RequiredSize += (ServerSidSize - (USHORT)SeLengthSid(UntrustedSid));

                }
            }
        }

        RequiredSize += Ace->AceSize;
    }

#ifdef NTOS_KERNEL_RUNTIME
    (*ServerAcl) = (PACL)ExAllocatePoolWithTag( PagedPool, RequiredSize, 'cAeS' );
#else // NTOS_KERNEL_RUNTIME
    (*ServerAcl) = (PACL)RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( SE_TAG ), RequiredSize );
#endif // NTOS_KERNEL_RUNTIME

    if ((*ServerAcl) == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Mark as allocated so caller knows to free it.
    //

    *ServerAclAllocated = TRUE;

    Status = RtlCreateAcl( (*ServerAcl), RequiredSize, ACL_REVISION3 );
    ASSERT( NT_SUCCESS( Status ));

    for (i = 0, Ace = FirstAce(Acl), Target=FirstAce( *ServerAcl );
         i < Acl->AceCount;
         i += 1, Ace = NextAce(Ace)) {

        //
        // If it's an ACCESS_ALLOWED_ACE_TYPE, convert to a Server ACE.
        //

        if (Ace->AceType == ACCESS_ALLOWED_ACE_TYPE ||
           (AclUntrusted && Ace->AceType == ACCESS_ALLOWED_COMPOUND_ACE_TYPE )) {

            AcePosition = Target;

            if (Ace->AceType == ACCESS_ALLOWED_ACE_TYPE) {
                ClientSid =  &((PKNOWN_ACE)Ace)->SidStart;
            } else {
                ClientSid = RtlCompoundAceClientSid( Ace );
            }

            //
            // Copy up to the access mask.
            //

            RtlCopyMemory(
                Target,
                Ace,
                FIELD_OFFSET(KNOWN_ACE, SidStart)
                );

            //
            // Now copy the correct Server SID
            //

            Target = ((PCHAR)Target + (UCHAR)(FIELD_OFFSET(KNOWN_COMPOUND_ACE, SidStart)));

            RtlCopyMemory(
                Target,
                ServerSid,
                SeLengthSid(ServerSid)
                );

            Target = ((PCHAR)Target + (UCHAR)SeLengthSid(ServerSid));

            //
            // Now copy in the correct client SID.  We can copy this right out of
            // the original ACE.
            //

            RtlCopyMemory(
                Target,
                ClientSid,
                SeLengthSid(ClientSid)
                );

            Target = ((PCHAR)Target + SeLengthSid(ClientSid));

            //
            // Set the size of the ACE accordingly
            //

            ((PKNOWN_COMPOUND_ACE)AcePosition)->Header.AceSize =
                (USHORT)FIELD_OFFSET(KNOWN_COMPOUND_ACE, SidStart) +
                (USHORT)SeLengthSid(ServerSid) +
                (USHORT)SeLengthSid(ClientSid);

            //
            // Set the type
            //

            ((PKNOWN_COMPOUND_ACE)AcePosition)->Header.AceType = ACCESS_ALLOWED_COMPOUND_ACE_TYPE;
            ((PKNOWN_COMPOUND_ACE)AcePosition)->CompoundAceType = COMPOUND_ACE_IMPERSONATION;

        } else {

            //
            // Just copy the ACE as is.
            //

            RtlCopyMemory( Target, Ace, Ace->AceSize );

            Target = ((PCHAR)Target + Ace->AceSize);
        }
    }

    (*ServerAcl)->AceCount = Acl->AceCount;

    return( STATUS_SUCCESS );
}

#ifndef NTOS_KERNEL_RUNTIME
NTSTATUS
RtlpGetDefaultsSubjectContext(
    HANDLE ClientToken,
    OUT PTOKEN_OWNER *OwnerInfo,
    OUT PTOKEN_PRIMARY_GROUP *GroupInfo,
    OUT PTOKEN_DEFAULT_DACL *DefaultDaclInfo,
    OUT PTOKEN_OWNER *ServerOwner,
    OUT PTOKEN_PRIMARY_GROUP *ServerGroup
    )
{
    HANDLE PrimaryToken;
    PVOID HeapHandle;
    NTSTATUS Status;
    ULONG ServerGroupInfoSize;
    ULONG ServerOwnerInfoSize;
    ULONG TokenDaclInfoSize;
    ULONG TokenGroupInfoSize;
    ULONG TokenOwnerInfoSize;

    BOOLEAN ClosePrimaryToken = FALSE;

    *OwnerInfo = NULL;
    *GroupInfo = NULL;
    *DefaultDaclInfo = NULL;
    *ServerOwner = NULL;
    *ServerGroup = NULL;

    HeapHandle = RtlProcessHeap();

    //
    // If the caller doesn't know the client token,
    //  simply don't return any information.
    //

    if ( ClientToken != NULL ) {
        //
        // Obtain the default owner from the client.
        //

        Status = NtQueryInformationToken(
                     ClientToken,                        // Handle
                     TokenOwner,                   // TokenInformationClass
                     NULL,                         // TokenInformation
                     0,                            // TokenInformationLength
                     &TokenOwnerInfoSize           // ReturnLength
                     );

        if ( STATUS_BUFFER_TOO_SMALL != Status ) {
            goto Cleanup;
        }

        *OwnerInfo = RtlAllocateHeap( HeapHandle, MAKE_TAG( SE_TAG ), TokenOwnerInfoSize );

        if ( *OwnerInfo == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        Status = NtQueryInformationToken(
                     ClientToken,                        // Handle
                     TokenOwner,                   // TokenInformationClass
                     *OwnerInfo,               // TokenInformation
                     TokenOwnerInfoSize,           // TokenInformationLength
                     &TokenOwnerInfoSize           // ReturnLength
                     );

        if (!NT_SUCCESS( Status )) {
            goto Cleanup;
        }

        //
        // Obtain the default group from the client token.
        //

        Status = NtQueryInformationToken(
                     ClientToken,                        // Handle
                     TokenPrimaryGroup,            // TokenInformationClass
                     *GroupInfo,                   // TokenInformation
                     0,                            // TokenInformationLength
                     &TokenGroupInfoSize           // ReturnLength
                     );

        if ( STATUS_BUFFER_TOO_SMALL != Status ) {
            goto Cleanup;
        }

        *GroupInfo = RtlAllocateHeap( HeapHandle, MAKE_TAG( SE_TAG ), TokenGroupInfoSize );

        if ( *GroupInfo == NULL ) {

            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        Status = NtQueryInformationToken(
                     ClientToken,                  // Handle
                     TokenPrimaryGroup,            // TokenInformationClass
                     *GroupInfo,                   // TokenInformation
                     TokenGroupInfoSize,           // TokenInformationLength
                     &TokenGroupInfoSize           // ReturnLength
                     );

        if (!NT_SUCCESS( Status )) {
            goto Cleanup;
        }

        Status = NtQueryInformationToken(
                     ClientToken,                        // Handle
                     TokenDefaultDacl,             // TokenInformationClass
                     *DefaultDaclInfo,             // TokenInformation
                     0,                            // TokenInformationLength
                     &TokenDaclInfoSize            // ReturnLength
                     );

        if ( STATUS_BUFFER_TOO_SMALL != Status ) {
            goto Cleanup;
        }

        *DefaultDaclInfo = RtlAllocateHeap( HeapHandle, MAKE_TAG( SE_TAG ), TokenDaclInfoSize );

        if ( *DefaultDaclInfo == NULL ) {

            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        Status = NtQueryInformationToken(
                     ClientToken,                        // Handle
                     TokenDefaultDacl,             // TokenInformationClass
                     *DefaultDaclInfo,             // TokenInformation
                     TokenDaclInfoSize,            // TokenInformationLength
                     &TokenDaclInfoSize            // ReturnLength
                     );

        if (!NT_SUCCESS( Status )) {
            goto Cleanup;
        }
    }

    //
    // Now open the primary token to determine how to substitute for
    // ServerOwner and ServerGroup.
    //

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_QUERY,
                 &PrimaryToken
                 );

    if (!NT_SUCCESS( Status )) {
        ClosePrimaryToken = FALSE;
        goto Cleanup;
    } else {
        ClosePrimaryToken = TRUE;
    }

    Status = NtQueryInformationToken(
                 PrimaryToken,                 // Handle
                 TokenOwner,                   // TokenInformationClass
                 NULL,                         // TokenInformation
                 0,                            // TokenInformationLength
                 &ServerOwnerInfoSize          // ReturnLength
                 );

    if ( STATUS_BUFFER_TOO_SMALL != Status ) {
        goto Cleanup;
    }

    *ServerOwner = RtlAllocateHeap( HeapHandle, MAKE_TAG( SE_TAG ), ServerOwnerInfoSize );

    if ( *ServerOwner == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    Status = NtQueryInformationToken(
                 PrimaryToken,                 // Handle
                 TokenOwner,                   // TokenInformationClass
                 *ServerOwner,                 // TokenInformation
                 ServerOwnerInfoSize,          // TokenInformationLength
                 &ServerOwnerInfoSize          // ReturnLength
                 );

    if (!NT_SUCCESS( Status )) {
        goto Cleanup;
    }

    //
    // Find the server group.
    //

    Status = NtQueryInformationToken(
                 PrimaryToken,                 // Handle
                 TokenPrimaryGroup,            // TokenInformationClass
                 *ServerGroup,                 // TokenInformation
                 0,                            // TokenInformationLength
                 &ServerGroupInfoSize          // ReturnLength
                 );

    if ( STATUS_BUFFER_TOO_SMALL != Status ) {
        goto Cleanup;
    }

    *ServerGroup = RtlAllocateHeap( HeapHandle, MAKE_TAG( SE_TAG ), ServerGroupInfoSize );

    if ( *ServerGroup == NULL ) {
        goto Cleanup;
    }

    Status = NtQueryInformationToken(
                 PrimaryToken,                 // Handle
                 TokenPrimaryGroup,            // TokenInformationClass
                 *ServerGroup,                 // TokenInformation
                 ServerGroupInfoSize,          // TokenInformationLength
                 &ServerGroupInfoSize          // ReturnLength
                 );

    if (!NT_SUCCESS( Status )) {
        goto Cleanup;
    }

    NtClose( PrimaryToken );

    return( STATUS_SUCCESS );

Cleanup:

    if (*OwnerInfo != NULL) {
        RtlFreeHeap( HeapHandle, 0, (PVOID)*OwnerInfo );
        *OwnerInfo = NULL;
    }

    if (*GroupInfo != NULL) {
        RtlFreeHeap( HeapHandle, 0, (PVOID)*GroupInfo );
        *GroupInfo = NULL;
    }

    if (*DefaultDaclInfo != NULL) {
        RtlFreeHeap( HeapHandle, 0, (PVOID)*DefaultDaclInfo );
        *DefaultDaclInfo = NULL;
    }

    if (*ServerOwner != NULL) {
        RtlFreeHeap( HeapHandle, 0, (PVOID)*ServerOwner );
        *ServerOwner = NULL;
    }

    if (*ServerGroup != NULL) {
        RtlFreeHeap( HeapHandle, 0, (PVOID)*ServerGroup );
        *ServerGroup = NULL;
    }

    if (ClosePrimaryToken  == TRUE) {
        NtClose( PrimaryToken );
    }

    return( Status );
}
#endif // NTOS_KERNEL_RUNTIME


NTSTATUS
RtlpNewSecurityObject (
    IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    IN PSECURITY_DESCRIPTOR CreatorDescriptor OPTIONAL,
    OUT PSECURITY_DESCRIPTOR * NewDescriptor,
    IN GUID *ObjectType OPTIONAL,
    IN BOOLEAN IsDirectoryObject,
    IN ULONG AutoInheritFlags,
    IN HANDLE Token OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping
    )
/*++

Routine Description:

    The procedure is used to allocate and initialize a self-relative
    Security Descriptor for a new protected server's object.  It is called
    when a new protected server object is being created.  The generated
    security descriptor will be in self-relative form.

    This procedure, called only from user mode, is used to establish a
    security descriptor for a new protected server's object.  Memory is
    allocated to hold each of the security descriptor's components (using
    NtAllocateVirtualMemory()).  The final security descriptor generated by
    this procedure is produced according to the rules stated in ???

    System and Discretionary ACL Assignment
    ---------------------------------------

    The assignment of system and discretionary ACLs is governed by the
    logic illustrated in the following table:

                 |  Explicit      |  Explicit     |
                 | (non-default)  |  Default      |   No
                 |  Acl           |  Acl          |   Acl
                 |  Specified     |  Specified    |   Specified
    -------------+----------------+---------------+--------------
                 |                |               |
    Inheritable  | Assign         |  Assign       | Assign
    Acl From     | Specified      |  Inherited    | Inherited
    Parent       | Acl(1)(2)      |  Acl          | Acl
                 |                |               |
    -------------+----------------+---------------+--------------
    No           |                |               |
    Inheritable  | Assign         |  Assign       | Assign
    Acl From     | Specified      |  Default      | No Acl
    Parent       | Acl(1)         |  Acl          |
                 |                |               |
    -------------+----------------+---------------+--------------

    (1) Any ACEs with the INHERITED_ACE bit set are NOT copied to the assigned
    security descriptor.

    (2) If the AutoInheritFlags is flagged to automatically inherit ACEs from
    parent (SEF_DACL_AUTO_INHERIT or SEF_SACL_AUTO_INHERIT), inherited
    ACEs from the parent will be appended after explicit ACEs from the
    CreatorDescriptor.


    Note that an explicitly specified ACL, whether a default ACL or
    not, may be empty or null.

    If the caller is explicitly assigning a system acl, default or
    non-default, the caller must either be a kernel mode client or
    must be appropriately privileged.


    Owner and Group Assignment
    --------------------------

    The assignment of the new object's owner and group is governed
    by the following logic:

       1)   If the passed security descriptor includes an owner, it
            is assigned as the new object's owner.  Otherwise, the
            caller's token is looked in for the owner.  Within the
            token, if there is a default owner, it is assigned.
            Otherwise, the caller's user ID is assigned.

       2)   If the passed security descriptor includes a group, it
            is assigned as the new object's group.  Otherwise, the
            caller's token is looked in for the group.  Within the
            token, if there is a default group, it is assigned.
            Otherwise, the caller's primary group ID is assigned.


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

    ObjectType - GUID of the object type being created.  If the object being
        created has no GUID associated with it, then this argument is
        specified as NULL.

    IsDirectoryObject - Specifies if the new object is going to be a
        directory object.  A value of TRUE indicates the object is a
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
            ACEs are inherited from the parent.  If no such ACEs are inherited,
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

        For calls from the kernel, Supplies the security context of the subject creating the
        object. This is used to retrieve default security information for the
        new object, such as default owner, primary group, and discretionary
        access control.

        If not specified, the Owner and Primary group must be specified in the
        CreatorDescriptor.

    GenericMapping - Supplies a pointer to a generic mapping array denoting
        the mapping between each generic right to specific rights.

Return Value:

    STATUS_SUCCESS - The operation was successful.

    STATUS_INVALID_OWNER - The owner SID provided as the owner of the
        target security descriptor is not one the subject is authorized to
        assign as the owner of an object.

    STATUS_NO_CLIENT_TOKEN - Indicates a client token was not explicitly
        provided and the caller is not currently impersonating a client.

    STATUS_PRIVILEGE_NOT_HELD - The caller does not have the privilege
        necessary to explicitly assign the specified system ACL.
        SeSecurityPrivilege privilege is needed to explicitly assign
        system ACLs to objects.


--*/
{


    SECURITY_DESCRIPTOR *CapturedDescriptor;
    SECURITY_DESCRIPTOR InCaseOneNotPassed;
    BOOLEAN SecurityDescriptorPassed;

    NTSTATUS Status;

    PACL NewSacl = NULL;
    BOOLEAN NewSaclInherited = FALSE;

    PACL NewDacl = NULL;
    PACL ServerDacl = NULL;
    BOOLEAN NewDaclInherited = FALSE;

    PSID NewOwner = NULL;
    PSID NewGroup = NULL;

    BOOLEAN SaclExplicitlyAssigned = FALSE;
    BOOLEAN OwnerExplicitlyAssigned = FALSE;
    BOOLEAN DaclExplicitlyAssigned = FALSE;

    BOOLEAN ServerDaclAllocated = FALSE;

    BOOLEAN ServerObject;
    BOOLEAN DaclUntrusted;

    BOOLEAN HasPrivilege;
    PRIVILEGE_SET PrivilegeSet;

    PSID SubjectContextOwner = NULL;
    PSID SubjectContextGroup = NULL;
    PSID ServerOwner = NULL;
    PSID ServerGroup = NULL;

    PACL SubjectContextDacl = NULL;

    ULONG AllocationSize;
    ULONG NewOwnerSize;
    ULONG NewGroupSize;
    ULONG NewSaclSize;
    ULONG NewDaclSize;

    PCHAR Field;
    PCHAR Base;



    PISECURITY_DESCRIPTOR_RELATIVE INewDescriptor = NULL;
    NTSTATUS PassedStatus;
    KPROCESSOR_MODE RequestorMode;

    ULONG GenericControl;
    ULONG NewControlBits = SE_SELF_RELATIVE;

#ifndef NTOS_KERNEL_RUNTIME
    PTOKEN_OWNER         TokenOwnerInfo = NULL;
    PTOKEN_PRIMARY_GROUP TokenPrimaryGroupInfo = NULL;
    PTOKEN_DEFAULT_DACL  TokenDefaultDaclInfo = NULL;

    PTOKEN_OWNER         ServerOwnerInfo = NULL;
    PTOKEN_PRIMARY_GROUP ServerGroupInfo = NULL;
    PVOID HeapHandle;

#else

    //
    // For kernel mode callers, the Token parameter is really
    // a pointer to a subject context structure.
    //

    PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
    PVOID SubjectContextInfo = NULL;

    SubjectSecurityContext = (PSECURITY_SUBJECT_CONTEXT)Token;

#endif // NTOS_KERNEL_RUNTIME


#ifdef NTOS_KERNEL_RUNTIME
    //
    //  Get the previous mode of the caller
    //

    RequestorMode = KeGetPreviousMode();
#else // NTOS_KERNEL_RUNTIME
    RequestorMode = UserMode;

    //
    // Get the handle to the current process heap
    //

    HeapHandle = RtlProcessHeap();

    //
    // Ensure the token is an impersonation token.
    //
    if ( Token != NULL ) {
        TOKEN_STATISTICS    ThreadTokenStatistics;
        ULONG ReturnLength;

        Status = NtQueryInformationToken(
                     Token,                        // Handle
                     TokenStatistics,              // TokenInformationClass
                     &ThreadTokenStatistics,       // TokenInformation
                     sizeof(TOKEN_STATISTICS),     // TokenInformationLength
                     &ReturnLength                 // ReturnLength
                     );

        if (!NT_SUCCESS( Status )) {
            return( Status );
        }

        //
        //  If it is an impersonation token, then make sure it is at a
        //  high enough level.
        //

        if (ThreadTokenStatistics.TokenType == TokenImpersonation) {

            if (ThreadTokenStatistics.ImpersonationLevel < SecurityIdentification ) {

                return( STATUS_BAD_IMPERSONATION_LEVEL );
            }
        }

    }
#endif // NTOS_KERNEL_RUNTIME


    //
    //  The desired end result is to build a self-relative security descriptor.
    //  This means that a single block of memory will be allocated and all
    //  security information copied into it.  To minimize work along the way,
    //  it is desirable to reference (rather than copy) each field as we
    //  determine its source.  This can not be done with inherited ACLs, however,
    //  since they must be built from another ACL.  So, explicitly assigned
    //  and defaulted SIDs and ACLs are just referenced until they are copied
    //  into the self-relative descriptor.  Inherited ACLs are built in a
    //  temporary buffer which must be deallocated after being copied to the
    //  self-relative descriptor.
    //



    //
    //  If a security descriptor has been passed, capture it, otherwise
    //  cobble up a fake one to simplify the code that follows.
    //

    if (ARGUMENT_PRESENT(CreatorDescriptor)) {

        CapturedDescriptor = CreatorDescriptor;
        SecurityDescriptorPassed = TRUE;

    } else {

        //
        //  No descriptor passed, make a fake one
        //

        SecurityDescriptorPassed = FALSE;

        RtlCreateSecurityDescriptor(&InCaseOneNotPassed,
                                    SECURITY_DESCRIPTOR_REVISION);
        CapturedDescriptor = &InCaseOneNotPassed;

    }


    if ( CapturedDescriptor->Control & SE_SERVER_SECURITY ) {
        ServerObject = TRUE;
    } else {
        ServerObject = FALSE;
    }

    if ( CapturedDescriptor->Control & SE_DACL_UNTRUSTED ) {
        DaclUntrusted = TRUE;
    } else {
        DaclUntrusted = FALSE;
    }



    //
    // Get the required information from the token.
    //
    //
    // Grab pointers to the default owner, primary group, and
    // discretionary ACL.
    //
    if ( Token != NULL || ServerObject ) {

#ifdef NTOS_KERNEL_RUNTIME

        PSID TmpSubjectContextOwner = NULL;
        PSID TmpSubjectContextGroup = NULL;
        PSID TmpServerOwner = NULL;
        PSID TmpServerGroup = NULL;

        PACL TmpSubjectContextDacl = NULL;

        SIZE_T SubjectContextInfoSize = 0;

        //
        // Lock the subject context for read access so that the pointers
        // we copy out of it don't disappear on us at random
        //

        SeLockSubjectContext( SubjectSecurityContext );

        SepGetDefaultsSubjectContext(
            SubjectSecurityContext,
            &TmpSubjectContextOwner,
            &TmpSubjectContextGroup,
            &TmpServerOwner,
            &TmpServerGroup,
            &TmpSubjectContextDacl
            );

        //
        // We can't keep the subject context locked, because
        // we may have to do a privilege check later, which calls
        // PsLockProcessSecurityFields, which can cause a deadlock
        // with PsImpersonateClient, which takes them in the reverse
        // order.
        //
        // Since we're giving up our read lock on the token, we
        // need to copy all the stuff that we just got back.  Since
        // it's not going to change, we can save some cycles and copy
        // it all into a single chunck of memory.
        //

        SubjectContextInfoSize = SeLengthSid( TmpSubjectContextOwner ) +
                                 SeLengthSid( TmpServerOwner )         +
                                 (TmpSubjectContextGroup != NULL ? SeLengthSid( TmpSubjectContextGroup ) : 0) +
                                 (TmpServerGroup         != NULL ? SeLengthSid( TmpServerGroup )         : 0) +
                                 (TmpSubjectContextDacl  != NULL ? TmpSubjectContextDacl->AclSize        : 0);

        SubjectContextInfo = ExAllocatePoolWithTag( PagedPool, SubjectContextInfoSize, 'dSeS');

        if (SubjectContextInfo) {

            //
            // Copy in the data
            //

            Base = SubjectContextInfo;

            //
            // There will always be an owner.
            //

            SubjectContextOwner = (PSID)Base;
            RtlCopySid( SeLengthSid( TmpSubjectContextOwner), Base, TmpSubjectContextOwner );
            Base += SeLengthSid( TmpSubjectContextOwner);

            //
            // Groups may be NULL
            //

            if (TmpSubjectContextGroup != NULL) {
                SubjectContextGroup = (PSID)Base;
                RtlCopySid( SeLengthSid( TmpSubjectContextGroup), Base, TmpSubjectContextGroup );
                Base += SeLengthSid( TmpSubjectContextGroup );
            } else {
                SubjectContextGroup = NULL;
            }

            ServerOwner = (PSID)Base;
            RtlCopySid( SeLengthSid( TmpServerOwner ), Base, TmpServerOwner );
            Base += SeLengthSid( TmpServerOwner );

            //
            // Groups may be NULL
            //

            if (TmpServerGroup != NULL) {
                ServerGroup = (PSID)Base;
                RtlCopySid( SeLengthSid( TmpServerGroup ), Base, TmpServerGroup );
                Base += SeLengthSid( TmpServerGroup );
            } else {
                ServerGroup = NULL;
            }

            if (TmpSubjectContextDacl != NULL) {
                SubjectContextDacl = (PACL)Base;
                RtlCopyMemory( Base, TmpSubjectContextDacl, TmpSubjectContextDacl->AclSize );
                // Base += TmpSubjectContextDacl->AclSize;
            } else {
                SubjectContextDacl = NULL;
            }

        } else {

            SeUnlockSubjectContext( SubjectSecurityContext );

            return( STATUS_INSUFFICIENT_RESOURCES );
        }

        SeUnlockSubjectContext( SubjectSecurityContext );


#else // NTOS_KERNEL_RUNTIME
        Status = RtlpGetDefaultsSubjectContext(
                     Token,
                     &TokenOwnerInfo,
                     &TokenPrimaryGroupInfo,
                     &TokenDefaultDaclInfo,
                     &ServerOwnerInfo,
                     &ServerGroupInfo
                     );

        if (!NT_SUCCESS( Status )) {
            return( Status );
        }

        SubjectContextOwner = TokenOwnerInfo->Owner;
        SubjectContextGroup = TokenPrimaryGroupInfo->PrimaryGroup;
        SubjectContextDacl  = TokenDefaultDaclInfo->DefaultDacl;
        ServerOwner         = ServerOwnerInfo->Owner;
        ServerGroup         = ServerGroupInfo->PrimaryGroup;
#endif // NTOS_KERNEL_RUNTIME
    }



    //
    // Establish an owner SID
    //

    NewOwner = RtlpOwnerAddrSecurityDescriptor(CapturedDescriptor);

    if ((NewOwner) != NULL) {

        //
        // Use the specified owner
        //

        OwnerExplicitlyAssigned = TRUE;

    } else {

        //
        // If the caller said to default the owner from the parent descriptor,
        //  grab it now.
        //

        if ( AutoInheritFlags & SEF_DEFAULT_OWNER_FROM_PARENT) {
            if ( !ARGUMENT_PRESENT(ParentDescriptor) ) {
                Status = STATUS_INVALID_OWNER;
                goto Cleanup;
            }
            NewOwner = RtlpOwnerAddrSecurityDescriptor((SECURITY_DESCRIPTOR *)ParentDescriptor);
            OwnerExplicitlyAssigned = TRUE;

            if ( NewOwner == NULL ) {
                Status = STATUS_INVALID_OWNER;
                goto Cleanup;
            }
        } else {

            //
            // Pick up the default from the subject's security context.
            //
            // This does NOT constitute explicit assignment of owner
            // and does not have to be checked as an ID that can be
            // assigned as owner.  This is because a default can not
            // be established in a token unless the user of the token
            // can assign it as an owner.
            //

            //
            // If we've been asked to create a ServerObject, we need to
            // make sure to pick up the new owner from the Primary token,
            // not the client token.  If we're not impersonating, they will
            // end up being the same.
            //

            NewOwner = ServerObject ? ServerOwner : SubjectContextOwner;

            //
            // Ensure an owner is now defined.
            //

            if ( NewOwner == NULL ) {
                Status = STATUS_NO_TOKEN;
                goto Cleanup;
            }
        }
    }


    //
    // Establish a Group SID
    //

    NewGroup = RtlpGroupAddrSecurityDescriptor(CapturedDescriptor);

    if (NewGroup == NULL) {

        //
        // If the caller said to default the group from the parent descriptor,
        //  grab it now.
        //

        if ( AutoInheritFlags & SEF_DEFAULT_GROUP_FROM_PARENT) {
            if ( !ARGUMENT_PRESENT(ParentDescriptor) ) {
                Status = STATUS_INVALID_PRIMARY_GROUP;
                goto Cleanup;
            }
            NewGroup = RtlpGroupAddrSecurityDescriptor((SECURITY_DESCRIPTOR *)ParentDescriptor);
        } else {
            //
            // Pick up the primary group from the subject's security context
            //
            // If we're creating a Server object, use the group from the server
            // context.
            //

            NewGroup = ServerObject ? ServerGroup : SubjectContextGroup;
        }

    }

    if (NewGroup != NULL) {
        if (!RtlValidSid( NewGroup )) {
            Status = STATUS_INVALID_PRIMARY_GROUP;
            goto Cleanup;
        }
    } else {
        Status = STATUS_INVALID_PRIMARY_GROUP;
        goto Cleanup;
    }




    //
    // Establish System Acl
    //

    Status = RtlpInheritAcl (
                ARGUMENT_PRESENT(ParentDescriptor) ?
                    RtlpSaclAddrSecurityDescriptor(
                        ((SECURITY_DESCRIPTOR *)ParentDescriptor)) :
                    NULL,
                RtlpSaclAddrSecurityDescriptor(CapturedDescriptor),
                SeControlSaclToGeneric( CapturedDescriptor->Control ),
                IsDirectoryObject,
                (BOOLEAN)((AutoInheritFlags & SEF_SACL_AUTO_INHERIT) != 0),
                (BOOLEAN)((AutoInheritFlags & SEF_DEFAULT_DESCRIPTOR_FOR_OBJECT) != 0),
                NewOwner,
                NewGroup,
                ServerOwner,
                ServerGroup,
                GenericMapping,
                TRUE,   // Is a SACL
                ObjectType,
                &NewSacl,
                &SaclExplicitlyAssigned,
                &GenericControl );

    if ( NT_SUCCESS(Status) ) {
        NewSaclInherited = TRUE;
        NewControlBits |= SE_SACL_PRESENT | SeControlGenericToSacl( GenericControl );

    } else if ( Status == STATUS_NO_INHERITANCE ) {

        //
        // Always set the auto inherit bit if the caller requested it.
        //

        if ( AutoInheritFlags & SEF_SACL_AUTO_INHERIT) {
            NewControlBits |= SE_SACL_AUTO_INHERITED;
        }

        //
        // No inheritable ACL - check for a defaulted one.
        //
        if ( RtlpAreControlBitsSet( CapturedDescriptor,
                                SE_SACL_PRESENT | SE_SACL_DEFAULTED ) ) {

            //
            // Reference the default ACL
            //

            NewSacl = RtlpSaclAddrSecurityDescriptor(CapturedDescriptor);
            NewControlBits |= SE_SACL_PRESENT;
            NewControlBits |= (CapturedDescriptor->Control & SE_SACL_PROTECTED);

            //
            // This counts as an explicit assignment.
            //
            SaclExplicitlyAssigned = TRUE;
        }

    } else {

        //
        // Some unusual error occured
        //

        goto Cleanup;
    }




    //
    // Establish Discretionary Acl
    //

    Status = RtlpInheritAcl (
                ARGUMENT_PRESENT(ParentDescriptor) ?
                    RtlpDaclAddrSecurityDescriptor(
                        ((SECURITY_DESCRIPTOR *)ParentDescriptor)) :
                    NULL,
                RtlpDaclAddrSecurityDescriptor(CapturedDescriptor),
                SeControlDaclToGeneric( CapturedDescriptor->Control ),
                IsDirectoryObject,
                (BOOLEAN)((AutoInheritFlags & SEF_DACL_AUTO_INHERIT) != 0),
                (BOOLEAN)((AutoInheritFlags & SEF_DEFAULT_DESCRIPTOR_FOR_OBJECT) != 0),
                NewOwner,
                NewGroup,
                ServerOwner,
                ServerGroup,
                GenericMapping,
                FALSE,   // Is a DACL
                ObjectType,
                &NewDacl,
                &DaclExplicitlyAssigned,
                &GenericControl );

    if ( NT_SUCCESS(Status) ) {
        NewDaclInherited = TRUE;
        NewControlBits |= SE_DACL_PRESENT | SeControlGenericToDacl( GenericControl );

    } else if ( Status == STATUS_NO_INHERITANCE ) {

        //
        // Always set the auto inherit bit if the caller requested it.
        //

        if ( AutoInheritFlags & SEF_DACL_AUTO_INHERIT) {
            NewControlBits |= SE_DACL_AUTO_INHERITED;
        }

        //
        // No inheritable ACL - check for a defaulted one.
        //
        if ( RtlpAreControlBitsSet( CapturedDescriptor,
                                SE_DACL_PRESENT | SE_DACL_DEFAULTED ) ) {

            //
            // Reference the default ACL
            //

            NewDacl = RtlpDaclAddrSecurityDescriptor(CapturedDescriptor);
            NewControlBits |= SE_DACL_PRESENT;
            NewControlBits |= (CapturedDescriptor->Control & SE_DACL_PROTECTED);

            //
            // This counts as an explicit assignment.
            //
            DaclExplicitlyAssigned = TRUE;

        //
        // Default to the DACL on the token.
        //
        } else if (ARGUMENT_PRESENT(SubjectContextDacl)) {

            NewDacl = SubjectContextDacl;
            NewControlBits |= SE_DACL_PRESENT;

        }


    } else {

        //
        // Some unusual error occured
        //

        goto Cleanup;
    }

    //
    // If auto inheriting and the computed child DACL is NULL,
    //  mark it as protected.
    //
    // NULL DACLs are problematic when ACEs are actually inherited from the
    // parent DACL.  It is better to mark them as protected NOW (even if we don't
    // end up inheriting any ACEs) to avoid confusion later.
    //

    if ( (AutoInheritFlags & SEF_DACL_AUTO_INHERIT) != 0 &&
         NewDacl == NULL ) {
        NewControlBits |= SE_DACL_PROTECTED;
    }



    //
    // Now make sure that the caller has the right to assign
    // everything in the descriptor.  The requestor is subjected
    // to privilege and restriction tests for some assignments.
    //
    if (RequestorMode == UserMode) {


        //
        // Anybody can assign any Discretionary ACL or group that they want to.
        //

        //
        //  See if the system ACL was explicitly specified
        //

        if ( SaclExplicitlyAssigned &&
             (AutoInheritFlags & SEF_AVOID_PRIVILEGE_CHECK) == 0 ) {

            //
            // Require a Token if we're to do the privilege check.
            //

            if ( Token == NULL ) {
                Status = STATUS_NO_TOKEN;
                goto Cleanup;
            }

#ifdef NTOS_KERNEL_RUNTIME

            //
            // Check for appropriate Privileges
            // Audit/Alarm messages need to be generated due to the attempt
            // to perform a privileged operation.
            //

            //
            // Note: be sure to do the privilege check against
            // the passed subject context!
            //

            PrivilegeSet.PrivilegeCount = 1;
            PrivilegeSet.Control = PRIVILEGE_SET_ALL_NECESSARY;
            PrivilegeSet.Privilege[0].Luid = SeSecurityPrivilege;
            PrivilegeSet.Privilege[0].Attributes = 0;

            HasPrivilege = SePrivilegeCheck(
                               &PrivilegeSet,
                               SubjectSecurityContext,
                               RequestorMode
                               );

            if ( RequestorMode != KernelMode ) {

                SePrivilegedServiceAuditAlarm (
                    NULL,                              // BUGWARNING need service name
                    SubjectSecurityContext,
                    &PrivilegeSet,
                    HasPrivilege
                    );
            }

#else // NTOS_KERNEL_RUNTIME
            //
            // Check for appropriate Privileges
            //
            // Audit/Alarm messages need to be generated due to the attempt
            // to perform a privileged operation.
            //

            PrivilegeSet.PrivilegeCount = 1;
            PrivilegeSet.Control = PRIVILEGE_SET_ALL_NECESSARY;
            PrivilegeSet.Privilege[0].Luid = RtlConvertLongToLuid(SE_SECURITY_PRIVILEGE);
            PrivilegeSet.Privilege[0].Attributes = 0;

            Status = NtPrivilegeCheck(
                        Token,
                        &PrivilegeSet,
                        &HasPrivilege
                        );

            if (!NT_SUCCESS( Status )) {
                goto Cleanup;
            }

#endif // NTOS_KERNEL_RUNTIME

            if ( !HasPrivilege ) {
                Status = STATUS_PRIVILEGE_NOT_HELD;
                goto Cleanup;
            }

        }

        //
        // See if the owner field is one the requestor can assign
        //

        if (OwnerExplicitlyAssigned &&
            (AutoInheritFlags & SEF_AVOID_OWNER_CHECK) == 0 ) {

#ifdef NTOS_KERNEL_RUNTIME


            if (!SepValidOwnerSubjectContext(
                    SubjectSecurityContext,
                    NewOwner,
                    ServerObject)
                    ) {

                Status = STATUS_INVALID_OWNER;
                goto Cleanup;
            }

#else // NTOS_KERNEL_RUNTIME

            //
            // Require a Token if we're to do the privilege check.
            //

            if ( Token == NULL ) {
                Status = STATUS_NO_TOKEN;
                goto Cleanup;
            }

            if (!RtlpValidOwnerSubjectContext(
                    Token,
                    NewOwner,
                    ServerObject,
                    &PassedStatus) ) {

                Status = PassedStatus;
                goto Cleanup;
            }
#endif // NTOS_KERNEL_RUNTIME
        }


        //
        // If the DACL was explictly assigned and this is a server object,
        //  convert the DACL to be a server DACL
        //

        if (DaclExplicitlyAssigned && ServerObject) {

            Status = RtlpCreateServerAcl(
                         NewDacl,
                         DaclUntrusted,
                         ServerOwner,
                         &ServerDacl,
                         &ServerDaclAllocated
                         );

            if (!NT_SUCCESS( Status )) {
                goto Cleanup;
            }

            NewDacl = ServerDacl;
        }
    }


    //
    // Everything is assignable by the requestor.
    // Calculate the memory needed to house all the information in
    // a self-relative security descriptor.
    //
    // Also map the ACEs for application to the target object
    // type, if they haven't already been mapped.
    //

    NewOwnerSize = LongAlignSize(SeLengthSid(NewOwner));
    if (NewGroup != NULL) {
        NewGroupSize = LongAlignSize(SeLengthSid(NewGroup));
    }

    if ((NewControlBits & SE_SACL_PRESENT) && (NewSacl != NULL)) {
        NewSaclSize = LongAlignSize(NewSacl->AclSize);
    } else {
        NewSaclSize = 0;
    }

    if ( (NewControlBits & SE_DACL_PRESENT) && (NewDacl != NULL)) {
        NewDaclSize = LongAlignSize(NewDacl->AclSize);
    } else {
        NewDaclSize = 0;
    }

    AllocationSize = LongAlignSize(sizeof(SECURITY_DESCRIPTOR_RELATIVE)) +
                     NewOwnerSize +
                     NewGroupSize +
                     NewSaclSize  +
                     NewDaclSize;

    //
    // Allocate and initialize the security descriptor as
    // self-relative form.
    //

#ifdef NTOS_KERNEL_RUNTIME
    INewDescriptor = (PSECURITY_DESCRIPTOR)ExAllocatePoolWithTag( PagedPool, AllocationSize, 'dSeS');
#else // NTOS_KERNEL_RUNTIME
    INewDescriptor = RtlAllocateHeap( HeapHandle, MAKE_TAG( SE_TAG ), AllocationSize );
#endif // NTOS_KERNEL_RUNTIME

    if ( INewDescriptor == NULL ) {
#ifdef NTOS_KERNEL_RUNTIME
        Status = STATUS_INSUFFICIENT_RESOURCES;
#else // NTOS_KERNEL_RUNTIME
        Status = STATUS_NO_MEMORY;
#endif // NTOS_KERNEL_RUNTIME
        goto Cleanup;
    }

    RtlCreateSecurityDescriptorRelative(
        INewDescriptor,
        SECURITY_DESCRIPTOR_REVISION
        );

    RtlpSetControlBits( INewDescriptor, NewControlBits );

    Base = (PCHAR)(INewDescriptor);
    Field =  Base + sizeof(SECURITY_DESCRIPTOR_RELATIVE);

    //
    // Map and Copy in the Sacl
    //

    if (NewControlBits & SE_SACL_PRESENT) {

        if (NewSacl != NULL) {

            RtlCopyMemory( Field, NewSacl, NewSacl->AclSize );

            if (!NewSaclInherited) {
                RtlpApplyAclToObject( (PACL)Field, GenericMapping );
            }

            INewDescriptor->Sacl = RtlPointerToOffset(Base,Field);
            Field += NewSaclSize;

        } else {

            INewDescriptor->Sacl = 0;
        }

    }

    //
    // Map and Copy in the Dacl
    //

    if (NewControlBits & SE_DACL_PRESENT) {

        if (NewDacl != NULL) {

            RtlCopyMemory( Field, NewDacl, NewDacl->AclSize );

            if (!NewDaclInherited) {
                RtlpApplyAclToObject( (PACL)Field, GenericMapping );
            }

            INewDescriptor->Dacl = RtlPointerToOffset(Base,Field);
            Field += NewDaclSize;

        } else {

            INewDescriptor->Dacl = 0;
        }

    }

    //
    // Assign the owner
    //

    RtlCopyMemory( Field, NewOwner, SeLengthSid(NewOwner) );
    INewDescriptor->Owner = RtlPointerToOffset(Base,Field);
    Field += NewOwnerSize;

    if (NewGroup != NULL) {
        RtlCopyMemory( Field, NewGroup, SeLengthSid(NewGroup) );
        INewDescriptor->Group = RtlPointerToOffset(Base,Field);
    }

    Status = STATUS_SUCCESS;



Cleanup:
    //
    // If we allocated memory for a Server DACL, free it now.
    //

    if (ServerDaclAllocated) {
#ifdef NTOS_KERNEL_RUNTIME
        ExFreePool( ServerDacl );
#else // NTOS_KERNEL_RUNTIME
        RtlFreeHeap(RtlProcessHeap(), 0, ServerDacl );
#endif // NTOS_KERNEL_RUNTIME
    }

    //
    // Either an error was encountered or the assignment has completed
    // successfully.  In either case, we have to clean up any memory.
    //

#ifdef NTOS_KERNEL_RUNTIME
//     if ( SubjectSecurityContext != NULL ) {
//         SeUnlockSubjectContext( SubjectSecurityContext );
//     }

    if (SubjectContextInfo != NULL) {
        ExFreePool( SubjectContextInfo );
    }

#else // NTOS_KERNEL_RUNTIME
    RtlFreeHeap( HeapHandle, 0, (PVOID)TokenOwnerInfo );
    RtlFreeHeap( HeapHandle, 0, (PVOID)TokenPrimaryGroupInfo );
    RtlFreeHeap( HeapHandle, 0, (PVOID)TokenDefaultDaclInfo );
    RtlFreeHeap( HeapHandle, 0, (PVOID)ServerOwnerInfo );
    RtlFreeHeap( HeapHandle, 0, (PVOID)ServerGroupInfo );
#endif // NTOS_KERNEL_RUNTIME

    if (NewSaclInherited && NewSacl != NULL ) {
#ifdef NTOS_KERNEL_RUNTIME
        ExFreePool( NewSacl );
#else // NTOS_KERNEL_RUNTIME
        RtlFreeHeap( HeapHandle, 0, (PVOID)NewSacl );
#endif // NTOS_KERNEL_RUNTIME
    }

    if (NewDaclInherited && NewDacl != NULL ) {
#ifdef NTOS_KERNEL_RUNTIME
        ExFreePool( NewDacl );
#else // NTOS_KERNEL_RUNTIME
        RtlFreeHeap( HeapHandle, 0, (PVOID)NewDacl );
#endif // NTOS_KERNEL_RUNTIME
    }

    *NewDescriptor = (PSECURITY_DESCRIPTOR) INewDescriptor;


    return Status;
}


NTSTATUS
RtlpSetSecurityObject (
    IN PVOID Object OPTIONAL,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR ModificationDescriptor,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN ULONG AutoInheritFlags,
    IN ULONG PoolType,
    IN PGENERIC_MAPPING GenericMapping,
    IN HANDLE Token OPTIONAL
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

                                  - - WARNING - -

    This service is for use by protected subsystems that project their own
    type of object.  This service is explicitly not for use by the
    executive for executive objects and must not be called from kernel
    mode.

Arguments:

    Object - Optionally supplies the object whose security is
        being adjusted.  This is used to update security quota
        information.

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

         SEF_AVOID_PRIVILEGE_CHECK - If set, the Token in not used to ensure the
            Owner passed in ModificationDescriptor is valid.

    PoolType - Specifies the type of pool to allocate for the objects
        security descriptor.

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

    STATUS_SUCCESS - The operation was successful.

    STATUS_INVALID_OWNER - The owner SID provided as the new owner of the
        target security descriptor is not one the caller is authorized to
        assign as the owner of an object, or the client did not pass
        a token at all.

    STATUS_NO_CLIENT_TOKEN - Indicates a client token was not explicitly
        provided and the caller is not currently impersonating a client.

    STATUS_BAD_DESCRIPTOR_FORMAT - Indicates the provided object's security
        descriptor was not in self-relative format.

--*/

{
    BOOLEAN NewGroupPresent = FALSE;
    BOOLEAN NewOwnerPresent = FALSE;

    BOOLEAN ServerAclAllocated = FALSE;
    BOOLEAN LocalDaclAllocated = FALSE;
    BOOLEAN LocalSaclAllocated = FALSE;
    BOOLEAN ServerObject;
    BOOLEAN DaclUntrusted;

    PCHAR Field;
    PCHAR Base;

    PISECURITY_DESCRIPTOR_RELATIVE NewDescriptor = NULL;

    NTSTATUS Status;

    TOKEN_STATISTICS ThreadTokenStatistics;

    ULONG ReturnLength;

    PSID NewGroup;
    PSID NewOwner;

    PACL NewDacl;
    PACL LocalDacl;
    PACL NewSacl;
    PACL LocalSacl;

    ULONG NewDaclSize;
    ULONG NewSaclSize;
    ULONG NewOwnerSize;
    ULONG NewGroupSize;
    ULONG AllocationSize;
    ULONG ServerOwnerInfoSize;

    HANDLE PrimaryToken;
    ULONG GenericControl;
    ULONG NewControlBits = SE_SELF_RELATIVE;

    PACL ServerDacl;

    SECURITY_SUBJECT_CONTEXT SubjectContext;


    //
    // Typecast to internal representation of security descriptor.
    // Note that the internal one is not a pointer to a pointer.
    // It is just a pointer to a security descriptor.
    //
    PISECURITY_DESCRIPTOR IModificationDescriptor =
       (PISECURITY_DESCRIPTOR)ModificationDescriptor;

    PISECURITY_DESCRIPTOR *IObjectsSecurityDescriptor =
       (PISECURITY_DESCRIPTOR *)(ObjectsSecurityDescriptor);

#ifndef NTOS_KERNEL_RUNTIME
    PVOID HeapHandle;
#endif // NTOS_KERNEL_RUNTIME

    RTL_PAGED_CODE();

    //
    // Get the handle to the current process heap
    //

#ifndef NTOS_KERNEL_RUNTIME
    HeapHandle = RtlProcessHeap();
#endif // NTOS_KERNEL_RUNTIME

    //
    //  Validate that the provided SD is in self-relative form
    //

    if ( !RtlpAreControlBitsSet(*IObjectsSecurityDescriptor, SE_SELF_RELATIVE) ) {
        Status = STATUS_BAD_DESCRIPTOR_FORMAT;
        goto Cleanup;
    }

    //
    // Check to see if we need to edit the passed acl
    // either because we're creating a server object, or because
    // we were passed an untrusted ACL.
    //

    if (ARGUMENT_PRESENT(ModificationDescriptor)) {

        if ( RtlpAreControlBitsSet(IModificationDescriptor, SE_SERVER_SECURITY)) {
            ServerObject = TRUE;
        } else {
            ServerObject = FALSE;
        }

        if ( RtlpAreControlBitsSet(IModificationDescriptor, SE_DACL_UNTRUSTED)) {
            DaclUntrusted = TRUE;
        } else {
            DaclUntrusted = FALSE;
        }

    } else {

        ServerObject = FALSE;
        DaclUntrusted = FALSE;

    }


    //
    // For each item specified in the SecurityInformation, extract it
    // and get it to the point where it can be copied into a new
    // descriptor.
    //

    //
    // if he's setting the owner field, make sure he's
    // allowed to set that value as an owner.
    //

    if (SecurityInformation & OWNER_SECURITY_INFORMATION) {

        NewOwner = RtlpOwnerAddrSecurityDescriptor( IModificationDescriptor );
        NewOwnerPresent = TRUE;

        if ((AutoInheritFlags & SEF_AVOID_PRIVILEGE_CHECK) == 0 ) {

#ifdef NTOS_KERNEL_RUNTIME

            SeCaptureSubjectContext( &SubjectContext );

            if (!SepValidOwnerSubjectContext( &SubjectContext, NewOwner, ServerObject ) ) {

                SeReleaseSubjectContext( &SubjectContext );
                return( STATUS_INVALID_OWNER );

            } else {

                SeReleaseSubjectContext( &SubjectContext );
            }
#else // NTOS_KERNEL_RUNTIME

            if ( ARGUMENT_PRESENT( Token )) {

                Status = NtQueryInformationToken(
                             Token,                        // Handle
                             TokenStatistics,              // TokenInformationClass
                             &ThreadTokenStatistics,       // TokenInformation
                             sizeof(TOKEN_STATISTICS),     // TokenInformationLength
                             &ReturnLength                 // ReturnLength
                             );

                if (!NT_SUCCESS( Status )) {
                    goto Cleanup;
                }

                //
                //  If it is an impersonation token, then make sure it is at a
                //  high enough level.
                //

                if (ThreadTokenStatistics.TokenType == TokenImpersonation) {

                    if (ThreadTokenStatistics.ImpersonationLevel < SecurityIdentification ) {
                        Status = STATUS_BAD_IMPERSONATION_LEVEL;
                        goto Cleanup;
                    }
                }

            } else {

                Status = STATUS_INVALID_OWNER;
                goto Cleanup;
            }

            if (!RtlpValidOwnerSubjectContext(
                    Token,
                    NewOwner,
                    ServerObject,
                    &Status) ) {

                    Status = STATUS_INVALID_OWNER;
                    goto Cleanup;
            }
#endif // NTOS_KERNEL_RUNTIME
        }

    } else {

        NewOwner = RtlpOwnerAddrSecurityDescriptor ( *IObjectsSecurityDescriptor );
        if (NewOwner == NULL) {
            Status = STATUS_INVALID_OWNER;
            goto Cleanup;
        }

    }
    ASSERT( NewOwner != NULL );
    if (!RtlValidSid( NewOwner )) {
        Status = STATUS_INVALID_OWNER;
        goto Cleanup;
    }


    if (SecurityInformation & GROUP_SECURITY_INFORMATION) {

        NewGroup = RtlpGroupAddrSecurityDescriptor(IModificationDescriptor);
        NewGroupPresent = TRUE;

    } else {

        NewGroup = RtlpGroupAddrSecurityDescriptor( *IObjectsSecurityDescriptor );
    }

    if (NewGroup != NULL) {
        if (!RtlValidSid( NewGroup )) {
            Status = STATUS_INVALID_PRIMARY_GROUP;
            goto Cleanup;
        }
    } else {
        Status = STATUS_INVALID_PRIMARY_GROUP;
        goto Cleanup;
    }


    if (SecurityInformation & DACL_SECURITY_INFORMATION) {

        //
        // If AutoInherit is requested,
        //  build a merged ACL.
        //

        if ( AutoInheritFlags & SEF_DACL_AUTO_INHERIT ) {
            Status = RtlpComputeMergedAcl(
                        RtlpDaclAddrSecurityDescriptor( *IObjectsSecurityDescriptor ),
                        SeControlDaclToGeneric( (*IObjectsSecurityDescriptor)->Control ),
                        RtlpDaclAddrSecurityDescriptor( IModificationDescriptor ),
                        SeControlDaclToGeneric( IModificationDescriptor->Control ),
                        NewOwner,
                        NewGroup,
                        GenericMapping,
                        FALSE,      // Not a SACL
                        &LocalDacl,
                        &GenericControl );

            if ( !NT_SUCCESS(Status)) {
                goto Cleanup;
            }

            LocalDaclAllocated = TRUE;
            NewDacl = LocalDacl;
            NewControlBits |= SE_DACL_PRESENT;
            NewControlBits |= SeControlGenericToDacl( GenericControl );

        //
        // If AutoInherit isn't requested,
        //  just grab a copy of the input DACL.
        //

        } else {
            NewDacl = RtlpDaclAddrSecurityDescriptor( IModificationDescriptor );
            NewControlBits |= SE_DACL_PRESENT;
            NewControlBits |= IModificationDescriptor->Control & SE_DACL_PROTECTED;

            //
            // If the original caller claims he understands auto inheritance,
            //  preserve the AutoInherited flag.
            //

            if ( RtlpAreControlBitsSet(IModificationDescriptor, SE_DACL_AUTO_INHERIT_REQ|SE_DACL_AUTO_INHERITED) ) {
                NewControlBits |= SE_DACL_AUTO_INHERITED;
            }
        }

        if (ServerObject) {

#ifdef NTOS_KERNEL_RUNTIME

            PSID SubjectContextOwner;
            PSID SubjectContextGroup;
            PSID SubjectContextServerOwner;
            PSID SubjectContextServerGroup;
            PACL SubjectContextDacl;

            SeCaptureSubjectContext( &SubjectContext );

            SepGetDefaultsSubjectContext(
                &SubjectContext,
                &SubjectContextOwner,
                &SubjectContextGroup,
                &SubjectContextServerOwner,
                &SubjectContextServerGroup,
                &SubjectContextDacl
                );

            Status = RtlpCreateServerAcl(
                         NewDacl,
                         DaclUntrusted,
                         SubjectContextServerOwner,
                         &ServerDacl,
                         &ServerAclAllocated
                         );

            SeReleaseSubjectContext( &SubjectContext );
#else // NTOS_KERNEL_RUNTIME
            PTOKEN_OWNER ServerSid;

            //
            // Obtain the default Server SID to substitute in the
            // ACL if necessary.
            //

            ServerOwnerInfoSize = RtlLengthRequiredSid( SID_MAX_SUB_AUTHORITIES );

            ServerSid = RtlAllocateHeap( HeapHandle, MAKE_TAG( SE_TAG ), ServerOwnerInfoSize );

            if (ServerSid == NULL) {
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            Status = NtOpenProcessToken(
                         NtCurrentProcess(),
                         TOKEN_QUERY,
                         &PrimaryToken
                         );

            if (!NT_SUCCESS( Status )) {
                RtlFreeHeap( HeapHandle, 0, ServerSid );
                goto Cleanup;
            }

            Status = NtQueryInformationToken(
                         PrimaryToken,                 // Handle
                         TokenOwner,                   // TokenInformationClass
                         ServerSid,                    // TokenInformation
                         ServerOwnerInfoSize,          // TokenInformationLength
                         &ServerOwnerInfoSize          // ReturnLength
                         );

            NtClose( PrimaryToken );

            if (!NT_SUCCESS( Status )) {
                RtlFreeHeap( HeapHandle, 0, ServerSid );
                goto Cleanup;
            }

            Status = RtlpCreateServerAcl(
                         NewDacl,
                         DaclUntrusted,
                         ServerSid->Owner,
                         &ServerDacl,
                         &ServerAclAllocated
                         );

            RtlFreeHeap( HeapHandle, 0, ServerSid );
#endif // NTOS_KERNEL_RUNTIME

            if (!NT_SUCCESS( Status )) {
                goto Cleanup;
            }

            NewDacl = ServerDacl;

        }

    } else {

        NewDacl = RtlpDaclAddrSecurityDescriptor( *IObjectsSecurityDescriptor );
    }



    if (SecurityInformation & SACL_SECURITY_INFORMATION) {


        //
        // If AutoInherit is requested,
        //  build a merged ACL.
        //

        if ( AutoInheritFlags & SEF_SACL_AUTO_INHERIT ) {
            Status = RtlpComputeMergedAcl(
                        RtlpSaclAddrSecurityDescriptor( *IObjectsSecurityDescriptor ),
                        SeControlSaclToGeneric( (*IObjectsSecurityDescriptor)->Control ),
                        RtlpSaclAddrSecurityDescriptor( IModificationDescriptor ),
                        SeControlSaclToGeneric( IModificationDescriptor->Control ),
                        NewOwner,
                        NewGroup,
                        GenericMapping,
                        TRUE,      // Is a SACL
                        &LocalSacl,
                        &GenericControl );

            if ( !NT_SUCCESS(Status)) {
                goto Cleanup;
            }
            LocalSaclAllocated = TRUE;
            NewSacl = LocalSacl;
            NewControlBits |= SE_SACL_PRESENT;
            NewControlBits |= SeControlGenericToSacl( GenericControl );
        } else {
            NewSacl = RtlpSaclAddrSecurityDescriptor( IModificationDescriptor );
            NewControlBits |= SE_SACL_PRESENT;
            NewControlBits |= IModificationDescriptor->Control & SE_SACL_PROTECTED;

            //
            // If the original caller claims he understands auto inheritance,
            //  preserve the AutoInherited flag.
            //

            if ( RtlpAreControlBitsSet(IModificationDescriptor, SE_SACL_AUTO_INHERIT_REQ|SE_SACL_AUTO_INHERITED) ) {
                NewControlBits |= SE_SACL_AUTO_INHERITED;
            }
        }

    } else {

        NewSacl = RtlpSaclAddrSecurityDescriptor( *IObjectsSecurityDescriptor );
    }


    //
    // Everything is assignable by the requestor.
    // Calculate the memory needed to house all the information in
    // a self-relative security descriptor.
    //
    // Also map the ACEs for application to the target object
    // type, if they haven't already been mapped.
    //

    NewOwnerSize = LongAlignSize(SeLengthSid(NewOwner));

    if (NewGroup != NULL) {
        NewGroupSize = LongAlignSize(SeLengthSid(NewGroup));
    } else {
        NewGroupSize = 0;
    }

    if (NewSacl != NULL) {
        NewSaclSize = LongAlignSize(NewSacl->AclSize);
    } else {
        NewSaclSize = 0;
    }

    if (NewDacl !=NULL) {
        NewDaclSize = LongAlignSize(NewDacl->AclSize);
    } else {
        NewDaclSize = 0;
    }

    AllocationSize = LongAlignSize(sizeof(SECURITY_DESCRIPTOR_RELATIVE)) +
                     NewOwnerSize +
                     NewGroupSize +
                     NewSaclSize  +
                     NewDaclSize;

    //
    // Allocate and initialize the security descriptor as
    // self-relative form.
    //

#ifdef NTOS_KERNEL_RUNTIME
    NewDescriptor = ExAllocatePoolWithTag(PoolType, AllocationSize, 'dSeS');
#else // NTOS_KERNEL_RUNTIME
    NewDescriptor = RtlAllocateHeap( HeapHandle, MAKE_TAG( SE_TAG ), AllocationSize );
#endif // NTOS_KERNEL_RUNTIME

    if ( NewDescriptor == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    Status = RtlCreateSecurityDescriptorRelative(
                 NewDescriptor,
                 SECURITY_DESCRIPTOR_REVISION
                 );

    ASSERT( NT_SUCCESS( Status ) );

#ifdef NTOS_KERNEL_RUNTIME
    //
    // We must check to make sure that the Group and Dacl size
    // do not exceed the quota preallocated for this object's
    // security when it was created.
    //
    // Update SeComputeSecurityQuota if this changes.
    //


    if (ARGUMENT_PRESENT( Object )) {

        Status = ObValidateSecurityQuota(
                     Object,
                     NewGroupSize + NewDaclSize
                     );

        if (!NT_SUCCESS( Status )) {

            //
            // The new information is too big.
            //

            ExFreePool( NewDescriptor );
            goto Cleanup;
        }

    }
#endif // NTOS_KERNEL_RUNTIME


    Base = (PCHAR)NewDescriptor;
    Field =  Base + sizeof(SECURITY_DESCRIPTOR_RELATIVE);

    //
    // Map and Copy in the Sacl
    //


    //         if new item {
    //             PRESENT=TRUE
    //             DEFAULTED=FALSE
    //             if (NULL) {
    //                 set new pointer to NULL
    //             } else {
    //                 copy into new SD
    //             }
    //         } else {
    //             copy PRESENT bit
    //             copy DEFAULTED bit
    //             if (NULL) {
    //                 set new pointer to NULL
    //             } else {
    //                 copy old one into new SD
    //             }
    //         }

    RtlpSetControlBits( NewDescriptor, NewControlBits );


    if (IModificationDescriptor->Control & SE_RM_CONTROL_VALID) {
        NewDescriptor->Sbz1 = IModificationDescriptor->Sbz1;
        NewDescriptor->Control |= SE_RM_CONTROL_VALID;
    }

    if (NewSacl == NULL) {
        NewDescriptor->Sacl = 0;

    } else {
        RtlCopyMemory( Field, NewSacl, NewSacl->AclSize );
        RtlpApplyAclToObject( (PACL)Field, GenericMapping );
        NewDescriptor->Sacl = RtlPointerToOffset(Base,Field);
        Field += NewSaclSize;
    }




    if ( (NewControlBits & SE_SACL_PRESENT) == 0 ) {

        //
        // Propagate the SE_SACL_DEFAULTED and SE_SACL_PRESENT
        // bits from the old security descriptor into the new
        // one.
        //

        RtlpPropagateControlBits(
            NewDescriptor,
            *IObjectsSecurityDescriptor,
            SE_SACL_DEFAULTED | SE_SACL_PRESENT | SE_SACL_PROTECTED
            );

    }



    //
    // Fill in Dacl field in new SD
    //

    if (NewDacl == NULL) {
        NewDescriptor->Dacl = 0;

    } else {
        RtlCopyMemory( Field, NewDacl, NewDacl->AclSize );
        RtlpApplyAclToObject( (PACL)Field, GenericMapping );
        NewDescriptor->Dacl = RtlPointerToOffset(Base,Field);
        Field += NewDaclSize;
    }


    if ( (NewControlBits & SE_DACL_PRESENT) == 0 ) {

        //
        // Propagate the SE_DACL_DEFAULTED and SE_DACL_PRESENT
        // bits from the old security descriptor into the new
        // one.
        //

        RtlpPropagateControlBits(
            NewDescriptor,
            *IObjectsSecurityDescriptor,
            SE_DACL_DEFAULTED | SE_DACL_PRESENT | SE_DACL_PROTECTED
            );

    }

//         if new item {
//             PRESENT=TRUE
//             DEFAULTED=FALSE
//             if (NULL) {
//                 set new pointer to NULL
//             } else {
//                 copy into new SD
//             }
//         } else {
//             copy PRESENT bit
//             copy DEFAULTED bit
//             if (NULL) {
//                 set new pointer to NULL
//             } else {
//                 copy old one into new SD
//             }
//         }


    //
    // Fill in Owner field in new SD
    //

    RtlCopyMemory( Field, NewOwner, SeLengthSid(NewOwner) );
    NewDescriptor->Owner = RtlPointerToOffset(Base,Field);
    Field += NewOwnerSize;

    if (!NewOwnerPresent) {

        //
        // Propagate the SE_OWNER_DEFAULTED bit from the old SD.
        // If a new owner is being assigned, we want to leave
        // SE_OWNER_DEFAULTED off, which means leave it alone.
        //

        RtlpPropagateControlBits(
            NewDescriptor,
            *IObjectsSecurityDescriptor,
            SE_OWNER_DEFAULTED
            );

    } else {
        ASSERT( !RtlpAreControlBitsSet( NewDescriptor, SE_OWNER_DEFAULTED ) );
    }


    //
    // Fill in Group field in new SD
    //

    if ( NewGroup != NULL) {
        RtlCopyMemory( Field, NewGroup, SeLengthSid(NewGroup) );
        NewDescriptor->Group = RtlPointerToOffset(Base,Field);
    }

    if (!NewGroupPresent) {

        //
        // Propagate the SE_GROUP_DEFAULTED bit from the old SD
        // If a new owner is being assigned, we want to leave
        // SE_GROUP_DEFAULTED off, which means leave it alone.
        //

        RtlpPropagateControlBits(
            NewDescriptor,
            *IObjectsSecurityDescriptor,
            SE_GROUP_DEFAULTED
            );
    } else {
        ASSERT( !RtlpAreControlBitsSet( NewDescriptor, SE_GROUP_DEFAULTED ) );

    }

    //
    // Free old descriptor
    //

    // Kernel version doesn't free the old descriptor
#ifndef NTOS_KERNEL_RUNTIME
    RtlFreeHeap( HeapHandle, 0, (PVOID) *IObjectsSecurityDescriptor );
#endif // NTOS_KERNEL_RUNTIME

    *ObjectsSecurityDescriptor = (PSECURITY_DESCRIPTOR)NewDescriptor;
    Status = STATUS_SUCCESS;

Cleanup:
    if ( LocalDaclAllocated ) {
#ifdef NTOS_KERNEL_RUNTIME
        ExFreePool( LocalDacl );
#else // NTOS_KERNEL_RUNTIME
        RtlFreeHeap( HeapHandle, 0, LocalDacl );
#endif // NTOS_KERNEL_RUNTIME
    }
    if ( LocalSaclAllocated ) {
#ifdef NTOS_KERNEL_RUNTIME
        ExFreePool( LocalSacl );
#else // NTOS_KERNEL_RUNTIME
        RtlFreeHeap( HeapHandle, 0, LocalSacl );
#endif // NTOS_KERNEL_RUNTIME
    }
    if (ServerAclAllocated) {
#ifdef NTOS_KERNEL_RUNTIME
        ExFreePool( ServerDacl );
#else // NTOS_KERNEL_RUNTIME
        RtlFreeHeap( HeapHandle, 0, ServerDacl );
#endif // NTOS_KERNEL_RUNTIME
    }

    return( Status );
}

BOOLEAN RtlpValidateSDOffsetAndSize (
    IN ULONG   Offset,
    IN ULONG   Length,
    IN ULONG   MinLength,
    OUT PULONG MaxLength
    )
/*++

Routine Description:

    This procedure validates offsets within a SecurityDescriptor.
    It checks that the structure can have the minimum length,
    not overlap with the fixed header and returns the maximum size
    of the item and longword alignment.

Arguments:

    Offset - Offset from start of SD of structure to validate
    Length - Total size of SD
    MinLength - Minimum size this structure can be
    MaxLength - Retuns the maximum length this item can be given by
                the enclosing structure.

Return Value:

    BOOLEAN - TRUE if the item is valid


--*/

{
    ULONG Left;

    *MaxLength = 0;
    //
    // Don't allow overlap with header just in case caller modifies control bits etc
    //
    if (Offset < sizeof (SECURITY_DESCRIPTOR_RELATIVE)) {
       return FALSE;
    }

    //
    // Don't allow offsets beyond the end of the buffer
    //
    if (Offset >= Length) {
       return FALSE;
    }

    //
    // Calculate maximim size of segment and check its limits
    //
    Left = Length - Offset;

    if (Left < MinLength) {
       return FALSE;
    }

    //
    // Reject unaligned offsets
    //
    if (Offset & (sizeof (ULONG) - 1)) {
       return FALSE;
    }
    *MaxLength = Left;
    return TRUE;
}


BOOLEAN
RtlValidRelativeSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptorInput,
    IN ULONG SecurityDescriptorLength,
    IN SECURITY_INFORMATION RequiredInformation
    )

/*++

Routine Description:

    This procedure validates a SecurityDescriptor's structure
    contained within a flat buffer.  This involves validating
    the revision levels of each component of the security
    descriptor.

Arguments:

    SecurityDescriptor - Pointer to the SECURITY_DESCRIPTOR structure
        to validate.
    SecurityDescriptorLength - Size of flat buffer containing the security
        descriptor.
    RequiredInformation - Which SD components must be present to be valid.
        OWNER_SECURITY_INFORMATION etc as a bit mask.
        OWNER_SECURITY_INFORMATION - There must be a valid owner SID
        GROUP_SECURITY_INFORMATION - There must be a valid group SID
        DACL_SECURITY_INFORMATION - Ignored
        SACL_SECURITY_INFORMATION - Ignored

Return Value:

    BOOLEAN - TRUE if the structure of SecurityDescriptor is valid.


--*/

{
    PISECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor;
    PISID OwnerSid;
    PISID GroupSid;
    PACE_HEADER Ace;
    PACL Dacl;
    PACL Sacl;
    ULONG MaxOwnerSidLength;
    ULONG MaxGroupSidLength;
    ULONG MaxDaclLength;
    ULONG MaxSaclLength;

    if (SecurityDescriptorLength < sizeof(SECURITY_DESCRIPTOR_RELATIVE)) {
        return FALSE;
    }

    //
    // Check the revision information.
    //

    if (((PISECURITY_DESCRIPTOR) SecurityDescriptorInput)->Revision !=
             SECURITY_DESCRIPTOR_REVISION) {
        return FALSE;
    }

    //
    // Make sure the passed SecurityDescriptor is in self-relative form
    //

    if (!(((PISECURITY_DESCRIPTOR) SecurityDescriptorInput)->Control & SE_SELF_RELATIVE)) {
        return FALSE;
    }

    SecurityDescriptor = (PISECURITY_DESCRIPTOR_RELATIVE) SecurityDescriptorInput;

    //
    // Validate the owner if it's there and see if its allowed to be missing
    //
    if (SecurityDescriptor->Owner == 0) {
        if (RequiredInformation & OWNER_SECURITY_INFORMATION) {
            return FALSE;
        }
    } else {
        if (!RtlpValidateSDOffsetAndSize (SecurityDescriptor->Owner,
                                          SecurityDescriptorLength,
                                          sizeof (SID),
                                          &MaxOwnerSidLength)) {
            return FALSE;
        }
        //
        // It is safe to reference the owner's SubAuthorityCount, compute the
        // expected length of the SID
        //

        OwnerSid = (PSID)RtlOffsetToPointer (SecurityDescriptor,
                                             SecurityDescriptor->Owner);

        if (OwnerSid->Revision != SID_REVISION) {
            return FALSE;
        }

        if (OwnerSid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
            return FALSE;
        }

        if (MaxOwnerSidLength < (ULONG) SeLengthSid (OwnerSid)) {
            return FALSE;
        }

    }

    //
    // The owner appears to be a structurally valid SID that lies within
    // the bounds of the security descriptor.  Do the same for the Group
    // if there is one.
    //
    //
    // Validate the group if it's there and see if its allowed to be missing
    //
    if (SecurityDescriptor->Group == 0) {
        if (RequiredInformation & GROUP_SECURITY_INFORMATION) {
            return FALSE;
        }
    } else {
        if (!RtlpValidateSDOffsetAndSize (SecurityDescriptor->Group,
                                          SecurityDescriptorLength,
                                          sizeof (SID),
                                          &MaxGroupSidLength)) {
            return FALSE;
        }
        //
        // It is safe to reference the group's SubAuthorityCount, compute the
        // expected length of the SID
        //

        GroupSid = (PSID)RtlOffsetToPointer (SecurityDescriptor,
                                             SecurityDescriptor->Group);

        if (GroupSid->Revision != SID_REVISION) {
            return FALSE;
        }

        if (GroupSid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
            return FALSE;
        }

        if (MaxGroupSidLength < (ULONG) SeLengthSid (GroupSid)) {
             return FALSE;
        }

    }

    //
    // Validate the DACL if it's there and check if its allowed to be missing.
    //

    if (!RtlpAreControlBitsSet (SecurityDescriptor, SE_DACL_PRESENT)) {
//
// Some code does this kind of thing:
//
// InitializeSecurityDescriptor (&sd, SECURITY_DESCRIPTOR_REVISION);
// RegSetKeySecurity(hKey, DACL_SECURITY_INFORMATION, &sd) )
//
// With the current system this works the same as passing in a NULL DACL but it looks
// almost by accident
//
//        if (RequiredInformation & DACL_SECURITY_INFORMATION) {
//            return FALSE;
//        }
    } else if (SecurityDescriptor->Dacl) {
        if (!RtlpValidateSDOffsetAndSize (SecurityDescriptor->Dacl,
                                          SecurityDescriptorLength,
                                          sizeof (ACL),
                                          &MaxDaclLength)) {
            return FALSE;
        }

        Dacl = (PACL) RtlOffsetToPointer (SecurityDescriptor,
                                          SecurityDescriptor->Dacl);

        //
        // Make sure the DACL length fits within the bounds of the security descriptor.
        //
        if (MaxDaclLength < Dacl->AclSize) {
            return FALSE;
        }

        //
        // Make sure the ACL is structurally valid.
        //
        if (!RtlValidAcl (Dacl)) {
            return FALSE;
        }
    }

    //
    // Validate the SACL if it's there and check if its allowed to be missing.
    //

    if (!RtlpAreControlBitsSet (SecurityDescriptor, SE_SACL_PRESENT)) {
//        if (RequiredInformation & SACL_SECURITY_INFORMATION) {
//            return FALSE;
//        }
    } else if (SecurityDescriptor->Sacl) {
        if (!RtlpValidateSDOffsetAndSize (SecurityDescriptor->Sacl,
                                          SecurityDescriptorLength,
                                          sizeof (ACL),
                                          &MaxSaclLength)) {
            return FALSE;
        }

        Sacl = (PACL) RtlOffsetToPointer (SecurityDescriptor,
                                          SecurityDescriptor->Sacl);

        //
        // Make sure the SACL length fits within the bounds of the security descriptor.
        //

        if (MaxSaclLength < Sacl->AclSize) {
            return FALSE;
        }

        //
        // Make sure the ACL is structurally valid.
        //

        if (!RtlValidAcl (Sacl)) {
            return FALSE;
        }
    }

    return TRUE;
}





BOOLEAN
RtlGetSecurityDescriptorRMControl(
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

    BOOLEAN - TRUE if SE_RM_CONTROL_VALID is set in the Control bits of the
              SecurityDescriptor.

Note:
    Parameter validation has already been done in Advapi.


--*/

{
    PISECURITY_DESCRIPTOR ISecurityDescriptor = (PISECURITY_DESCRIPTOR) SecurityDescriptor;

    if (!(ISecurityDescriptor->Control & SE_RM_CONTROL_VALID))
    {
        *RMControl = 0;
        return FALSE;
    }

    *RMControl = ISecurityDescriptor->Sbz1;

    return TRUE;
}


VOID
RtlSetSecurityDescriptorRMControl(
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PUCHAR RMControl OPTIONAL
    )

/*++

Routine Description:

    This procedure sets the RM Control flag in the control field of
    SecurityDescriptor and sets Sbz1 to the the byte to which RMContol points.
    If RMControl is NULL then the bits are cleared.

Arguments:

    SecurityDescriptor - Pointer to the SECURITY_DESCRIPTOR structure
    RMControl          - Pointer to the flags to set. If NULL then the bits
                         are cleared.

Note:
    Parameter validation has already been done in Advapi.


--*/

{
    PISECURITY_DESCRIPTOR ISecurityDescriptor = (PISECURITY_DESCRIPTOR) SecurityDescriptor;

    if (ARGUMENT_PRESENT(RMControl)) {
        ISecurityDescriptor->Control |= SE_RM_CONTROL_VALID;
        ISecurityDescriptor->Sbz1 = *RMControl;
    } else {
        ISecurityDescriptor->Control &= ~SE_RM_CONTROL_VALID;
        ISecurityDescriptor->Sbz1 = 0;
    }
}
