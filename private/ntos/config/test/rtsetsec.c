/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    rtsetsec.c

Abstract:

    NT level registry security test program

    Assigns a world read-only security descriptor to an existing registry
    key object.

    rtsetsec <KeyPath>

    Example:

        rtsetsec \REGISTRY\MACHINE\TEST\read_only

Author:

    John Vert (jvert) 28-Jan-92

Revision History:

    Richard Ward (richardw) 14 April 92    Changed ACE_HEADER

--*/

#include "cmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PSID
GetMySid(
    VOID
    );

PSECURITY_DESCRIPTOR
GenerateDescriptor(
    VOID
    );

void
__cdecl main(
    int argc,
    char *argv[]
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING  KeyName;
    ANSI_STRING AnsiKeyName;
    HANDLE KeyHandle;
    PSECURITY_DESCRIPTOR NewSecurityDescriptor;

    //
    // Process args
    //

    if (argc != 2) {
        printf("Usage: %s <KeyPath>\n",argv[0]);
        exit(1);
    }

    RtlInitAnsiString(&AnsiKeyName, argv[1]);
    Status = RtlAnsiStringToUnicodeString(&KeyName, &AnsiKeyName, TRUE);
    if (!NT_SUCCESS(Status)) {
        printf("RtlAnsiStringToUnicodeString failed %lx\n",Status);
        exit(1);
    }

    printf("rtsetsec: starting\n");

    //
    // Open node that we want to change the security descriptor for.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        0,
        (HANDLE)NULL,
        NULL
        );
    ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

    Status = NtOpenKey(
                &KeyHandle,
                WRITE_DAC,
                &ObjectAttributes
                );
    if (!NT_SUCCESS(Status)) {
        printf("rtsetsec: NtOpenKey failed: %08lx\n", Status);
        exit(1);
    }

    NewSecurityDescriptor = GenerateDescriptor();

    Status = NtSetSecurityObject( KeyHandle,
                                  DACL_SECURITY_INFORMATION,
                                  NewSecurityDescriptor);
    if (!NT_SUCCESS(Status)) {
        printf("rtsetsec: NtSetSecurity failed: %08lx\n",Status);
        exit(1);
    }

    Status = NtClose(KeyHandle);
    if (!NT_SUCCESS(Status)) {
        printf("rtsetsec: NtClose failed: %08lx\n", Status);
        exit(1);
    }

    printf("rtsetsec: successful\n");

}

PSECURITY_DESCRIPTOR
GenerateDescriptor(
    VOID
    )
{
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PACL Acl;
    PSID WorldSid, CreatorSid;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    ULONG OwnerAceLength, WorldAceLength;
    ULONG AclLength;
    NTSTATUS Status;
    PACCESS_ALLOWED_ACE OwnerAce;
    PACCESS_ALLOWED_ACE WorldAce;

    WorldSid = malloc(RtlLengthRequiredSid(1));
    if (WorldSid == NULL) {
        printf("rtsetsec: GenerateDescriptor() couldn't malloc WorldSID\n");
        exit(1);
    }
    RtlInitializeSid(WorldSid, &WorldAuthority, 1);
    *(RtlSubAuthoritySid(WorldSid, 0)) = SECURITY_WORLD_RID;
    if (!RtlValidSid(WorldSid)) {
        printf("rtsetsec: GenerateDescriptor() created invalid World SID\n");
        exit(1);
    }

    CreatorSid = GetMySid();

    //
    // Since the ACCESS_DENIED_ACE already contains a ULONG for the
    // SID, we subtract this back out when calculating the size of the ACE
    //

    WorldAceLength = SeLengthSid(WorldSid) -
                     sizeof(ULONG)     +
                     sizeof(ACCESS_ALLOWED_ACE) ;
    WorldAce = malloc(WorldAceLength);
    if (WorldAce == NULL) {
        printf("rtsetsec: GenerateDescriptor() couldn't malloc WorldAce\n");
        exit(1);
    }

    OwnerAceLength = SeLengthSid(CreatorSid) -
                     sizeof(ULONG)     +
                     sizeof(ACCESS_ALLOWED_ACE);

    OwnerAce = malloc( OwnerAceLength );
    if (OwnerAce == NULL) {
        printf("rtsetsec: GenerateDescriptor() couldn't malloc OwnerAce\n");
        exit(1);
    }

    AclLength = OwnerAceLength + WorldAceLength + sizeof(ACL);
    Acl = malloc(AclLength);
    if (Acl == NULL) {
        printf("rtsetsec: GenerateDescriptor() couldn't malloc ACL\n");
        exit(1);
    }

    Status = RtlCreateAcl(Acl, AclLength, ACL_REVISION);
    if (!NT_SUCCESS(Status)) {
        printf("rtsetsec: RtlCreateAcl failed status %08lx\n", Status);
        exit(1);
    }

    //
    // Fill in ACE fields
    //

    WorldAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    WorldAce->Header.AceSize = (USHORT)WorldAceLength;
    WorldAce->Header.AceFlags = 0;  // clear audit and inherit flags
    WorldAce->Mask = KEY_READ;
    Status = RtlCopySid( SeLengthSid(WorldSid),
                         &WorldAce->SidStart,
                         WorldSid );
    if (!NT_SUCCESS(Status)) {
        printf("rtsetsec: RtlCopySid failed status %08lx\n", Status);
        exit(1);
    }

    OwnerAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    OwnerAce->Header.AceSize = (USHORT)OwnerAceLength;
    OwnerAce->Header.AceFlags = 0;  // clear audit and inherit flags
    OwnerAce->Mask = KEY_ALL_ACCESS;
    Status = RtlCopySid( SeLengthSid(CreatorSid),
                         &OwnerAce->SidStart,
                         CreatorSid );
    if (!NT_SUCCESS(Status)) {
        printf("rtsetsec: RtlCopySid failed status %08lx\n", Status);
        exit(1);
    }

    free(WorldSid);

    //
    // Now add the ACE to the beginning of the ACL.
    //

    Status = RtlAddAce( Acl,
                        ACL_REVISION,
                        0,
                        WorldAce,
                        WorldAceLength );
    if (!NT_SUCCESS(Status)) {
        printf("rtsetsec: RtlAddAce (world ace) failed status %08lx\n", Status);
        exit(1);
    }
    Status = RtlAddAce( Acl,
                        ACL_REVISION,
                        0,
                        OwnerAce,
                        OwnerAceLength );
    if (!NT_SUCCESS(Status)) {
        printf("rtsetsec: RtlAddAce (owner ace) failed status %08lx\n", Status);
        exit(1);
    }

    free(OwnerAce);
    free(WorldAce);

    //
    // Allocate and initialize the Security Descriptor
    //

    SecurityDescriptor = malloc(sizeof(SECURITY_DESCRIPTOR));
    Status = RtlCreateSecurityDescriptor( SecurityDescriptor,
                                          SECURITY_DESCRIPTOR_REVISION );
    if (!NT_SUCCESS(Status)) {
        printf("rtsetsec: RtlCreateSecurityDescriptor failed status %08lx\n",Status);
        exit(1);
    }

    Status = RtlSetDaclSecurityDescriptor( SecurityDescriptor,
                                           TRUE,
                                           Acl,
                                           FALSE );
    if (!NT_SUCCESS(Status)) {
        printf("rtsetsec: RtlSetDaclSecurityDescriptor failed status %08lx\n",Status);
        exit(1);
    }

    //
    // FINALLY we are finished!
    //

    return(SecurityDescriptor);

}

PSID
GetMySid(
    VOID
    )
{
    NTSTATUS Status;
    HANDLE Token;
    PTOKEN_OWNER Owner;
    ULONG Length;

    Status = NtOpenProcessToken( NtCurrentProcess(),
                                 TOKEN_QUERY,
                                 &Token );
    if (!NT_SUCCESS(Status)) {
        printf("rtsetsec: GetMySid() NtOpenProcessToken failed status %08lx\n",Status);
        exit(1);
    }

    Status = NtQueryInformationToken( Token,
                                      TokenOwner,
                                      Owner,
                                      0,
                                      &Length );
    if (Status != STATUS_BUFFER_TOO_SMALL) {
        printf("rtsetsec: GetMySid() NtQueryInformationToken failed status %08lx\n",Status);
        exit(1);
    }

    Owner = malloc(Length);
    if (Owner==NULL) {
        printf("rtsetsec: GetMySid() Couldn't malloc TOKEN_OWNER buffer\n");
        exit(1);
    }
    Status = NtQueryInformationToken( Token,
                                      TokenOwner,
                                      Owner,
                                      Length,
                                      &Length );
    if (!NT_SUCCESS(Status)) {
        printf("rtsetsec: GetMySid() NtQueryInformationToken failed status %08lx\n",Status);
        exit(1);
    }

    NtClose(Token);

    return(Owner->Owner);

}
