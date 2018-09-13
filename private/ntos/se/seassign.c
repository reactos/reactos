/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Seassign.c

Abstract:

    This Module implements the SeAssignSecurity procedure.  For a description
    of the pool allocation strategy please see the comments in semethod.c

Author:

    Gary Kimura     (GaryKi)    9-Nov-1989

Environment:

    Kernel Mode

Revision History:

    Richard Ward     (RichardW)  14-April-92
    Robert Reichel   (RobertRe)  28-February-95
        Added Compound ACEs

--*/


#include "sep.h"
#include "tokenp.h"
#include "sertlp.h"
#include "zwapi.h"
#include "nturtl.h"


//
//  Local macros and procedures
//


NTSTATUS
SepInheritAcl (
    IN PACL Acl,
    IN BOOLEAN IsDirectoryObject,
    IN PSID OwnerSid,
    IN PSID GroupSid,
    IN PSID ServerSid OPTIONAL,
    IN PSID ClientSid OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping,
    IN POOL_TYPE PoolType,
    OUT PACL *NewAcl
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SeAssignSecurity)
#pragma alloc_text(PAGE,SeAssignSecurityEx)
#pragma alloc_text(PAGE,SeDeassignSecurity)
#pragma alloc_text(PAGE,SepInheritAcl)
#pragma alloc_text(PAGE,SeAssignWorldSecurityDescriptor)
#pragma alloc_text(PAGE,SepDumpSecurityDescriptor)
#pragma alloc_text(PAGE,SepPrintAcl)
#pragma alloc_text(PAGE,SepPrintSid)
#pragma alloc_text(PAGE,SepDumpTokenInfo)
#pragma alloc_text(PAGE,SepSidTranslation)
#endif


//
// These variables control whether security descriptors and token
// information are dumped by their dump routines.  This allows
// selective turning on and off of debugging output by both program
// control and via the kernel debugger.
//

#if DBG

BOOLEAN SepDumpSD = FALSE;
BOOLEAN SepDumpToken = FALSE;

#endif




NTSTATUS
SeAssignSecurity (
    IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    IN PSECURITY_DESCRIPTOR ExplicitDescriptor OPTIONAL,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor,
    IN BOOLEAN IsDirectoryObject,
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
    IN PGENERIC_MAPPING GenericMapping,
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    This routine assumes privilege checking HAS NOT yet been performed
    and so will be performed by this routine.

    This procedure is used to build a security descriptor for a new object
    given the security descriptor of its parent directory and any originally
    requested security for the object.  The final security descriptor
    returned to the caller may contain a mix of information, some explicitly
    provided other from the new object's parent.


    See RtlpNewSecurityObject for a descriptor of how the NewDescriptor is
    built.


Arguments:

    ParentDescriptor - Optionally supplies the security descriptor of the
        parent directory under which this new object is being created.

    ExplicitDescriptor - Supplies the address of a pointer to the security
        descriptor as specified by the user that is to be applied to
        the new object.

    NewDescriptor - Returns the actual security descriptor for the new
        object that has been modified according to above rules.

    IsDirectoryObject - Specifies if the new object is itself a directory
        object.  A value of TRUE indicates the object is a container of other
        objects.

    SubjectContext - Supplies the security context of the subject creating the
        object. This is used to retrieve default security information for the
        new object, such as default owner, primary group, and discretionary
        access control.

    GenericMapping - Supplies a pointer to an array of access mask values
        denoting the mapping between each generic right to non-generic rights.

    PoolType - Specifies the pool type to use to when allocating a new
        security descriptor.

Return Value:

    STATUS_SUCCESS - indicates the operation was successful.

    STATUS_INVALID_OWNER - The owner SID provided as the owner of the
        target security descriptor is not one the caller is authorized
        to assign as the owner of an object.

    STATUS_PRIVILEGE_NOT_HELD - The caller does not have the privilege
        necessary to explicitly assign the specified system ACL.
        SeSecurityPrivilege privilege is needed to explicitly assign
        system ACLs to objects.
--*/

{
    NTSTATUS Status;
    ULONG AutoInherit = 0;
    PAGED_CODE();

#if DBG
    if ( ARGUMENT_PRESENT( ExplicitDescriptor) ) {
        SepDumpSecurityDescriptor( ExplicitDescriptor,
                                   "\nSeAssignSecurity: Input security descriptor = \n"
                                 );
    }

    if (ARGUMENT_PRESENT( ParentDescriptor )) {
        SepDumpSecurityDescriptor( ParentDescriptor,
                                   "\nSeAssignSecurity: Parent security descriptor = \n"
                                 );
    }
#endif // DBG

    //
    // If the Parent SD was created via AutoInheritance,
    //  and this object is being created with no explicit descriptor,
    //  then we can safely create this object as AutoInherit.
    //

    if ( ParentDescriptor != NULL ) {

        if ( (ExplicitDescriptor == NULL ||
              (((PISECURITY_DESCRIPTOR)ExplicitDescriptor)->Control & SE_DACL_PRESENT) == 0 ) &&
             (((PISECURITY_DESCRIPTOR)ParentDescriptor)->Control & SE_DACL_AUTO_INHERITED) != 0 ) {
            AutoInherit |= SEF_DACL_AUTO_INHERIT;
        }

        if ( (ExplicitDescriptor == NULL ||
             (((PISECURITY_DESCRIPTOR)ExplicitDescriptor)->Control & SE_SACL_PRESENT) == 0 ) &&
             (((PISECURITY_DESCRIPTOR)ParentDescriptor)->Control & SE_SACL_AUTO_INHERITED) != 0 ) {
            AutoInherit |= SEF_SACL_AUTO_INHERIT;
        }

    }


    Status = RtlpNewSecurityObject (
                    ParentDescriptor OPTIONAL,
                    ExplicitDescriptor OPTIONAL,
                    NewDescriptor,
                    NULL,   // No object type
                    IsDirectoryObject,
                    AutoInherit,
                    (HANDLE) SubjectContext,
                    GenericMapping );

#if DBG
    if ( NT_SUCCESS(Status)) {
        SepDumpSecurityDescriptor( *NewDescriptor,
                                   "SeAssignSecurity: Final security descriptor = \n"
                                 );
    }
#endif

    return Status;


    // RtlpNewSecurityObject always uses PagedPool.
    UNREFERENCED_PARAMETER( PagedPool );

}


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
    )

/*++

Routine Description:

    This routine assumes privilege checking HAS NOT yet been performed
    and so will be performed by this routine.

    This procedure is used to build a security descriptor for a new object
    given the security descriptor of its parent directory and any originally
    requested security for the object.  The final security descriptor
    returned to the caller may contain a mix of information, some explicitly
    provided other from the new object's parent.


    See RtlpNewSecurityObject for a descriptor of how the NewDescriptor is
    built.


Arguments:

    ParentDescriptor - Optionally supplies the security descriptor of the
        parent directory under which this new object is being created.

    ExplicitDescriptor - Supplies the address of a pointer to the security
        descriptor as specified by the user that is to be applied to
        the new object.

    NewDescriptor - Returns the actual security descriptor for the new
        object that has been modified according to above rules.

    ObjectType - GUID of the object type being created.  If the object being
        created has no GUID associated with it, then this argument is
        specified as NULL.

    IsDirectoryObject - Specifies if the new object is itself a directory
        object.  A value of TRUE indicates the object is a container of other
        objects.

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

    SubjectContext - Supplies the security context of the subject creating the
        object. This is used to retrieve default security information for the
        new object, such as default owner, primary group, and discretionary
        access control.

    GenericMapping - Supplies a pointer to an array of access mask values
        denoting the mapping between each generic right to non-generic rights.

    PoolType - Specifies the pool type to use to when allocating a new
        security descriptor.

Return Value:

    STATUS_SUCCESS - indicates the operation was successful.

    STATUS_INVALID_OWNER - The owner SID provided as the owner of the
        target security descriptor is not one the caller is authorized
        to assign as the owner of an object.

    STATUS_PRIVILEGE_NOT_HELD - The caller does not have the privilege
        necessary to explicitly assign the specified system ACL.
        SeSecurityPrivilege privilege is needed to explicitly assign
        system ACLs to objects.
--*/

{
    NTSTATUS Status;
    PAGED_CODE();

#if DBG
    if ( ARGUMENT_PRESENT( ExplicitDescriptor) ) {
        SepDumpSecurityDescriptor( ExplicitDescriptor,
                                   "\nSeAssignSecurityEx: Input security descriptor = \n"
                                 );
    }

    if (ARGUMENT_PRESENT( ParentDescriptor )) {
        SepDumpSecurityDescriptor( ParentDescriptor,
                                   "\nSeAssignSecurityEx: Parent security descriptor = \n"
                                 );
    }
#endif // DBG


    Status = RtlpNewSecurityObject (
                    ParentDescriptor OPTIONAL,
                    ExplicitDescriptor OPTIONAL,
                    NewDescriptor,
                    ObjectType,
                    IsDirectoryObject,
                    AutoInheritFlags,
                    (HANDLE) SubjectContext,
                    GenericMapping );

#if DBG
    if ( NT_SUCCESS(Status)) {
        SepDumpSecurityDescriptor( *NewDescriptor,
                                   "SeAssignSecurityEx: Final security descriptor = \n"
                                 );
    }
#endif

    return Status;


    // RtlpNewSecurityObject always uses PagedPool.
    UNREFERENCED_PARAMETER( PagedPool );

}


NTSTATUS
SeDeassignSecurity (
    IN OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    )

/*++

Routine Description:

    This routine deallocates the memory associated with a security descriptor
    that was assigned using SeAssignSecurity.


Arguments:

    SecurityDescriptor - Supplies the address of a pointer to the security
        descriptor  being deleted.

Return Value:

    STATUS_SUCCESS - The deallocation was successful.

--*/

{
    PAGED_CODE();

    if ((*SecurityDescriptor) != NULL) {
        ExFreePool( (*SecurityDescriptor) );
    }

    //
    //  And zero out the pointer to it for safety sake
    //

    (*SecurityDescriptor) = NULL;

    return( STATUS_SUCCESS );

}



NTSTATUS
SepInheritAcl (
    IN PACL Acl,
    IN BOOLEAN IsDirectoryObject,
    IN PSID ClientOwnerSid,
    IN PSID ClientGroupSid,
    IN PSID ServerOwnerSid OPTIONAL,
    IN PSID ServerGroupSid OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping,
    IN POOL_TYPE PoolType,
    OUT PACL *NewAcl
    )

/*++

Routine Description:

    This is a private routine that produces an inherited acl from
    a parent acl according to the rules of inheritance

Arguments:

    Acl - Supplies the acl being inherited.

    IsDirectoryObject - Specifies if the new acl is for a directory.

    OwnerSid - Specifies the owner Sid to use.

    GroupSid - Specifies the group SID to use.

    ServerSid - Specifies the Server SID to use.

    ClientSid - Specifies the Client SID to use.

    GenericMapping - Specifies the generic mapping to use.

    PoolType - Specifies the pool type for the new acl.

    NewAcl - Receives a pointer to the new (inherited) acl.

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
//   inheritance in the user mode runtime (in sertl.c). Do not make changes //
//   here without also making changes in that module.                       //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


    NTSTATUS Status;
    ULONG NewAclLength;
    BOOLEAN NewAclExplicitlyAssigned;
    ULONG NewGenericControl;

    PAGED_CODE();
    ASSERT( PoolType == PagedPool ); // RtlpInheritAcl assumes paged pool

    //
    //  First check if the acl is null
    //

    if (Acl == NULL) {

        return STATUS_NO_INHERITANCE;
    }

    //
    // Generating an inheritable ACL.
    //
    // Pass all parameters as though there is no auto inheritance.
    //

    Status = RtlpInheritAcl(
                 Acl,
                 NULL,  // No child ACL since no auto inheritance
                 0,     // No child control since no auto inheritance
                 IsDirectoryObject,
                 FALSE, // Not AutoInherit since no auto inheritance
                 FALSE, // Not DefaultDescriptor since no auto inheritance
                 ClientOwnerSid,
                 ClientGroupSid,
                 ServerOwnerSid,
                 ServerGroupSid,
                 GenericMapping,
                 FALSE, // Isn't a SACL
                 NULL,  // No object GUID
                 NewAcl,
                 &NewAclExplicitlyAssigned,
                 &NewGenericControl );

    return Status;
}



NTSTATUS
SeAssignWorldSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG Length,
    IN PSECURITY_INFORMATION SecurityInformation
    )

/*++

Routine Description:

    This routine is called by the I/O system to properly initialize a
    security descriptor for a FAT file.  It will take a pointer to a
    buffer containing an emptry security descriptor, and create in the
    buffer a self-relative security descriptor with

        Owner = WorldSid,

        Group = WorldSid.

    Thus, a FAT file is accessable to all.

Arguments:

    SecurityDescriptor - Supplies a pointer to a buffer in which will be
        created a self-relative security descriptor as described above.

    Length - The length in bytes of the buffer.  If the length is too
        small, it will contain the minimum size required upon exit.


Return Value:

    STATUS_BUFFER_TOO_SMALL - The buffer was not big enough to contain
        the requested information.


--*/

{

    PCHAR Field;
    PCHAR Base;
    ULONG WorldSidLength;
    PISECURITY_DESCRIPTOR_RELATIVE ISecurityDescriptor;
    ULONG MinSize;
    NTSTATUS Status;

    PAGED_CODE();

    if ( !ARGUMENT_PRESENT( SecurityInformation )) {

        return( STATUS_ACCESS_DENIED );
    }

    WorldSidLength = SeLengthSid( SeWorldSid );

    MinSize = sizeof( SECURITY_DESCRIPTOR_RELATIVE ) + 2 * WorldSidLength;

    if ( *Length < MinSize ) {

        *Length = MinSize;
        return( STATUS_BUFFER_TOO_SMALL );
    }

    *Length = MinSize;

    ISecurityDescriptor = (SECURITY_DESCRIPTOR_RELATIVE *)SecurityDescriptor;

    Status = RtlCreateSecurityDescriptorRelative( ISecurityDescriptor,
                                          SECURITY_DESCRIPTOR_REVISION );

    if (!NT_SUCCESS( Status )) {
        return( Status );
    }

    Base = (PCHAR)(ISecurityDescriptor);
    Field =  Base + sizeof(SECURITY_DESCRIPTOR_RELATIVE);

    if ( *SecurityInformation & OWNER_SECURITY_INFORMATION ) {

        RtlCopyMemory( Field, SeWorldSid, WorldSidLength );
        ISecurityDescriptor->Owner = RtlPointerToOffset(Base,Field);
        Field += WorldSidLength;
    }

    if ( *SecurityInformation & GROUP_SECURITY_INFORMATION ) {

        RtlCopyMemory( Field, SeWorldSid, WorldSidLength );
        ISecurityDescriptor->Group = RtlPointerToOffset(Base,Field);
    }

    if ( *SecurityInformation & DACL_SECURITY_INFORMATION ) {
        RtlpSetControlBits( ISecurityDescriptor, SE_DACL_PRESENT );
    }

    if ( *SecurityInformation & SACL_SECURITY_INFORMATION ) {
        RtlpSetControlBits( ISecurityDescriptor, SE_SACL_PRESENT );
    }

    RtlpSetControlBits( ISecurityDescriptor, SE_SELF_RELATIVE );

    return( STATUS_SUCCESS );

}








//
//  BUGWARNING The following routines should be in a debug only kernel, since
//  all they do is dump stuff to a debug terminal as appropriate.  The same
//  goes for the declarations of the variables SepDumpSD and SepDumpToken
//



VOID
SepDumpSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSZ TitleString
    )

/*++

Routine Description:

    Private routine to dump a security descriptor to the debug
    screen.

Arguments:

    SecurityDescriptor - Supplies the security descriptor to be dumped.

    TitleString - A null terminated string to print before dumping
        the security descriptor.


Return Value:

    None.


--*/
{
#if DBG
    PISECURITY_DESCRIPTOR ISecurityDescriptor;
    UCHAR Revision;
    SECURITY_DESCRIPTOR_CONTROL Control;
    PSID Owner;
    PSID Group;
    PACL Sacl;
    PACL Dacl;

    PAGED_CODE();


    if (!SepDumpSD) {
        return;
    }

    if (!ARGUMENT_PRESENT( SecurityDescriptor )) {
        return;
    }

    DbgPrint(TitleString);

    ISecurityDescriptor = ( PISECURITY_DESCRIPTOR )SecurityDescriptor;

    Revision = ISecurityDescriptor->Revision;
    Control  = ISecurityDescriptor->Control;

    Owner    = RtlpOwnerAddrSecurityDescriptor( ISecurityDescriptor );
    Group    = RtlpGroupAddrSecurityDescriptor( ISecurityDescriptor );
    Sacl     = RtlpSaclAddrSecurityDescriptor( ISecurityDescriptor );
    Dacl     = RtlpDaclAddrSecurityDescriptor( ISecurityDescriptor );

    DbgPrint("\nSECURITY DESCRIPTOR\n");

    DbgPrint("Revision = %d\n",Revision);

    //
    // Print control info
    //

    if (Control & SE_OWNER_DEFAULTED) {
        DbgPrint("Owner defaulted\n");
    }
    if (Control & SE_GROUP_DEFAULTED) {
        DbgPrint("Group defaulted\n");
    }
    if (Control & SE_DACL_PRESENT) {
        DbgPrint("Dacl present\n");
    }
    if (Control & SE_DACL_DEFAULTED) {
        DbgPrint("Dacl defaulted\n");
    }
    if (Control & SE_SACL_PRESENT) {
        DbgPrint("Sacl present\n");
    }
    if (Control & SE_SACL_DEFAULTED) {
        DbgPrint("Sacl defaulted\n");
    }
    if (Control & SE_SELF_RELATIVE) {
        DbgPrint("Self relative\n");
    }
    if (Control & SE_DACL_UNTRUSTED) {
        DbgPrint("Dacl untrusted\n");
    }
    if (Control & SE_SERVER_SECURITY) {
        DbgPrint("Server security\n");
    }

    DbgPrint("Owner ");
    SepPrintSid( Owner );

    DbgPrint("Group ");
    SepPrintSid( Group );

    DbgPrint("Sacl");
    SepPrintAcl( Sacl );

    DbgPrint("Dacl");
    SepPrintAcl( Dacl );
#endif
}



VOID
SepPrintAcl (
    IN PACL Acl
    )

/*++

Routine Description:

    This routine dumps via (DbgPrint) an Acl for debug purposes.  It is
    specialized to dump standard aces.

Arguments:

    Acl - Supplies the Acl to dump

Return Value:

    None

--*/


{
#if DBG
    ULONG i;
    PKNOWN_ACE Ace;
    BOOLEAN KnownType;

    PAGED_CODE();

    DbgPrint("@ %8lx\n", Acl);

    //
    //  Check if the Acl is null
    //

    if (Acl == NULL) {

        return;

    }

    //
    //  Dump the Acl header
    //

    DbgPrint(" Revision: %02x", Acl->AclRevision);
    DbgPrint(" Size: %04x", Acl->AclSize);
    DbgPrint(" AceCount: %04x\n", Acl->AceCount);

    //
    //  Now for each Ace we want do dump it
    //

    for (i = 0, Ace = FirstAce(Acl);
         i < Acl->AceCount;
         i++, Ace = NextAce(Ace) ) {

        //
        //  print out the ace header
        //

        DbgPrint("\n AceHeader: %08lx ", *(PULONG)Ace);

        //
        //  special case on the standard ace types
        //

        if ((Ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) ||
            (Ace->Header.AceType == ACCESS_DENIED_ACE_TYPE) ||
            (Ace->Header.AceType == SYSTEM_AUDIT_ACE_TYPE) ||
            (Ace->Header.AceType == SYSTEM_ALARM_ACE_TYPE) ||
            (Ace->Header.AceType == ACCESS_ALLOWED_COMPOUND_ACE_TYPE)) {

            //
            //  The following array is indexed by ace types and must
            //  follow the allowed, denied, audit, alarm seqeuence
            //

            PCHAR AceTypes[] = { "Access Allowed",
                                 "Access Denied ",
                                 "System Audit  ",
                                 "System Alarm  ",
                                 "Compound Grant",
                               };

            DbgPrint(AceTypes[Ace->Header.AceType]);
            DbgPrint("\n Access Mask: %08lx ", Ace->Mask);
            KnownType = TRUE;

        } else {

            DbgPrint(" Unknown Ace Type\n");
            KnownType = FALSE;
        }

        DbgPrint("\n");

        DbgPrint(" AceSize = %d\n",Ace->Header.AceSize);

        DbgPrint(" Ace Flags = ");
        if (Ace->Header.AceFlags & OBJECT_INHERIT_ACE) {
            DbgPrint("OBJECT_INHERIT_ACE\n");
            DbgPrint("                   ");
        }

        if (Ace->Header.AceFlags & CONTAINER_INHERIT_ACE) {
            DbgPrint("CONTAINER_INHERIT_ACE\n");
            DbgPrint("                   ");
        }

        if (Ace->Header.AceFlags & NO_PROPAGATE_INHERIT_ACE) {
            DbgPrint("NO_PROPAGATE_INHERIT_ACE\n");
            DbgPrint("                   ");
        }

        if (Ace->Header.AceFlags & INHERIT_ONLY_ACE) {
            DbgPrint("INHERIT_ONLY_ACE\n");
            DbgPrint("                   ");
        }


        if (Ace->Header.AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG) {
            DbgPrint("SUCCESSFUL_ACCESS_ACE_FLAG\n");
            DbgPrint("            ");
        }

        if (Ace->Header.AceFlags & FAILED_ACCESS_ACE_FLAG) {
            DbgPrint("FAILED_ACCESS_ACE_FLAG\n");
            DbgPrint("            ");
        }

        DbgPrint("\n");

        if (KnownType != TRUE) {
            continue;
        }

        if (Ace->Header.AceType != ACCESS_ALLOWED_COMPOUND_ACE_TYPE) {
            DbgPrint(" Sid = ");
            SepPrintSid(&Ace->SidStart);
        } else {
            DbgPrint(" Server Sid = ");
            SepPrintSid(RtlCompoundAceServerSid(Ace));
            DbgPrint("\n Client Sid = ");
            SepPrintSid(RtlCompoundAceClientSid( Ace ));
        }
    }
#endif
}



VOID
SepPrintSid(
    IN PSID Sid
    )

/*++

Routine Description:

    Prints a formatted Sid

Arguments:

    Sid - Provides a pointer to the sid to be printed.


Return Value:

    None.

--*/

{
#if DBG
    UCHAR i;
    ULONG Tmp;
    PISID ISid;
    STRING AccountName;
    UCHAR Buffer[128];

    PAGED_CODE();

    if (Sid == NULL) {
        DbgPrint("Sid is NULL\n");
        return;
    }

    Buffer[0] = 0;

    AccountName.MaximumLength = 127;
    AccountName.Length = 0;
    AccountName.Buffer = (PVOID)&Buffer[0];

    if (SepSidTranslation( Sid, &AccountName )) {

        DbgPrint("%s   ", AccountName.Buffer );
    }

    ISid = (PISID)Sid;

    DbgPrint("S-%lu-", (USHORT)ISid->Revision );
    if (  (ISid->IdentifierAuthority.Value[0] != 0)  ||
          (ISid->IdentifierAuthority.Value[1] != 0)     ){
        DbgPrint("0x%02hx%02hx%02hx%02hx%02hx%02hx",
                    (USHORT)ISid->IdentifierAuthority.Value[0],
                    (USHORT)ISid->IdentifierAuthority.Value[1],
                    (USHORT)ISid->IdentifierAuthority.Value[2],
                    (USHORT)ISid->IdentifierAuthority.Value[3],
                    (USHORT)ISid->IdentifierAuthority.Value[4],
                    (USHORT)ISid->IdentifierAuthority.Value[5] );
    } else {
        Tmp = (ULONG)ISid->IdentifierAuthority.Value[5]          +
              (ULONG)(ISid->IdentifierAuthority.Value[4] <<  8)  +
              (ULONG)(ISid->IdentifierAuthority.Value[3] << 16)  +
              (ULONG)(ISid->IdentifierAuthority.Value[2] << 24);
        DbgPrint("%lu", Tmp);
    }


    for (i=0;i<ISid->SubAuthorityCount ;i++ ) {
        DbgPrint("-%lu", ISid->SubAuthority[i]);
    }
    DbgPrint("\n");
#endif
}




VOID
SepDumpTokenInfo(
    IN PACCESS_TOKEN Token
    )

/*++

Routine Description:

    Prints interesting information in a token.

Arguments:

    Token - Provides the token to be examined.


Return Value:

    None.

--*/

{
#if DBG
    ULONG UserAndGroupCount;
    PSID_AND_ATTRIBUTES TokenSid;
    ULONG i;
    PTOKEN IToken;

    PAGED_CODE();

    if (!SepDumpToken) {
        return;
    }

    IToken = (TOKEN *)Token;

    UserAndGroupCount = IToken->UserAndGroupCount;

    DbgPrint("\n\nToken Address=%lx\n",IToken);
    DbgPrint("Token User and Groups Array:\n\n");

    for ( i = 0 , TokenSid = IToken->UserAndGroups;
          i < UserAndGroupCount ;
          i++, TokenSid++
        ) {

        SepPrintSid( TokenSid->Sid );

        }

    if ( IToken->RestrictedSids ) {
        UserAndGroupCount = IToken->RestrictedSidCount;

        DbgPrint("Restricted Sids Array:\n\n");

        for ( i = 0 , TokenSid = IToken->RestrictedSids;
              i < UserAndGroupCount ;
              i++, TokenSid++
            ) {

            SepPrintSid( TokenSid->Sid );

            }
    }
#endif
}



BOOLEAN
SepSidTranslation(
    PSID Sid,
    PSTRING AccountName
    )

/*++

Routine Description:

    This routine translates well-known SIDs into English names.

Arguments:

    Sid - Provides the sid to be examined.

    AccountName - Provides a string buffer in which to place the
        translated name.

Return Value:

    None

--*/

// AccountName is expected to have a large maximum length

{
    PAGED_CODE();

    if (RtlEqualSid(Sid, SeWorldSid)) {
        RtlInitString( AccountName, "WORLD         ");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, SeLocalSid)) {
        RtlInitString( AccountName, "LOCAL         ");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, SeNetworkSid)) {
        RtlInitString( AccountName, "NETWORK       ");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, SeBatchSid)) {
        RtlInitString( AccountName, "BATCH         ");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, SeInteractiveSid)) {
        RtlInitString( AccountName, "INTERACTIVE   ");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, SeLocalSystemSid)) {
        RtlInitString( AccountName, "SYSTEM        ");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, SeCreatorOwnerSid)) {
        RtlInitString( AccountName, "CREATOR_OWNER ");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, SeCreatorGroupSid)) {
        RtlInitString( AccountName, "CREATOR_GROUP ");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, SeCreatorOwnerServerSid)) {
        RtlInitString( AccountName, "CREATOR_OWNER_SERVER ");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, SeCreatorGroupServerSid)) {
        RtlInitString( AccountName, "CREATOR_GROUP_SERVER ");
        return(TRUE);
    }

    return(FALSE);
}

//
//  End debug only routines
//
