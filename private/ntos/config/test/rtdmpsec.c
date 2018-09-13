/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rtdmpsec.c

Abstract:

    NT level registry security test program #1, basic non-error paths.

    Dump out the security descriptors of a sub-tree of the registry.

    rtdmpsec <KeyPath>

    Will ennumerate and dump out the subkeys and values of KeyPath,
    and then apply itself recursively to each subkey it finds.

    It assumes data values are null terminated strings.

    Example:

        rtdmpsec \REGISTRY\MACHINE\TEST\bigkey

Author:

    John Vert (jvert) 24-Jan-92

        based on rtdmp.c by

    Bryan Willman (bryanwi)  10-Dec-91

        and getdacl.c by RobertRe

Revision History:

    Richard Ward (richardw)  14 April 1992   Changed ACE_HEADER

--*/

#include "cmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORK_SIZE   1024

//
//  Get a pointer to the first ace in an acl
//

#define FirstAce(Acl) ((PVOID)((PUCHAR)(Acl) + sizeof(ACL)))

//
//  Get a pointer to the following ace
//

#define NextAce(Ace) ((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize))

//
// Generic ACE structure, to be used for casting ACE's of known types
//

typedef struct _KNOWN_ACE {
   ACE_HEADER Header;
   ACCESS_MASK Mask;
   ULONG SidStart;
   } KNOWN_ACE, *PKNOWN_ACE;



VOID
InitVars();

VOID
PrintAcl (
    IN PACL Acl
    );

VOID
PrintAccessMask(
    IN ACCESS_MASK AccessMask
    );

void __cdecl main(int, char *);
void processargs();

void print(PUNICODE_STRING);

void
DumpSecurity(
    HANDLE  Handle
    );

void
Dump(
    HANDLE  Handle
    );

UNICODE_STRING  WorkName;
WCHAR           workbuffer[WORK_SIZE];

//
// Universal well known SIDs
//

PSID  NullSid;
PSID  WorldSid;
PSID  LocalSid;
PSID  CreatorOwnerSid;

//
// Sids defined by NT
//

PSID NtAuthoritySid;

PSID DialupSid;
PSID NetworkSid;
PSID BatchSid;
PSID InteractiveSid;
PSID LocalSystemSid;

void
__cdecl main(
    int argc,
    char *argv[]
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE          BaseHandle;

    InitVars();

    //
    // Process args
    //

    WorkName.MaximumLength = WORK_SIZE;
    WorkName.Length = 0L;
    WorkName.Buffer = &(workbuffer[0]);

    processargs(argc, argv);


    //
    // Set up and open KeyPath
    //

    printf("rtdmpsec: starting\n");

    InitializeObjectAttributes(
        &ObjectAttributes,
        &WorkName,
        0,
        (HANDLE)NULL,
        NULL
        );
    ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

    status = NtOpenKey(
                &BaseHandle,
                MAXIMUM_ALLOWED,
                &ObjectAttributes
                );
    if (!NT_SUCCESS(status)) {
        printf("rtdmpsec: t0: %08lx\n", status);
        exit(1);
    }

    Dump(BaseHandle);
}


void
Dump(
    HANDLE  Handle
    )
{
    NTSTATUS    status;
    PKEY_BASIC_INFORMATION KeyInformation;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG   NamePos;
    ULONG   index;
    STRING  enumname;
    HANDLE  WorkHandle;
    ULONG   ResultLength;
    static  char buffer[WORK_SIZE];
    PUCHAR  p;

    KeyInformation = (PKEY_BASIC_INFORMATION)buffer;
    NamePos = WorkName.Length;

    //
    // Print name of node we are about to dump out
    //
    printf("\n");
    print(&WorkName);
    printf("::\n");

    //
    // Print out node's values
    //
    DumpSecurity(Handle);

    //
    // Enumerate node's children and apply ourselves to each one
    //

    for (index = 0; TRUE; index++) {

        RtlZeroMemory(KeyInformation, WORK_SIZE);
        status = NtEnumerateKey(
                    Handle,
                    index,
                    KeyBasicInformation,
                    KeyInformation,
                    WORK_SIZE,
                    &ResultLength
                    );

        if (status == STATUS_NO_MORE_ENTRIES) {

            WorkName.Length = NamePos;
            return;

        } else if (!NT_SUCCESS(status)) {

            printf("rtdmpsec: dump1: status = %08lx\n", status);
            exit(1);

        }

        enumname.Buffer = &(KeyInformation->Name[0]);
        enumname.Length = KeyInformation->NameLength;
        enumname.MaximumLength = KeyInformation->NameLength;

        p = WorkName.Buffer;
        p += WorkName.Length;
        *p = '\\';
        p++;
        *p = '\0';
        WorkName.Length += 2;

        RtlAppendStringToString((PSTRING)&WorkName, (PSTRING)&enumname);

        InitializeObjectAttributes(
            &ObjectAttributes,
            &enumname,
            0,
            Handle,
            NULL
            );
        ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

        status = NtOpenKey(
                    &WorkHandle,
                    MAXIMUM_ALLOWED,
                    &ObjectAttributes
                    );
        if (!NT_SUCCESS(status)) {
            if (status == STATUS_ACCESS_DENIED) {
                printf("\n");
                print(&WorkName);
                printf("::\n\tAccess denied!\n");
            } else {
                printf("rtdmpsec: dump2: %08lx\n", status);
                exit(1);
            }
        } else {
            Dump(WorkHandle);
            NtClose(WorkHandle);
        }

        WorkName.Length = NamePos;
    }
}


void
DumpSecurity(
    HANDLE  Handle
    )
{
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    NTSTATUS Status;
    ULONG Length;
    PACL Dacl;
    BOOLEAN DaclPresent;
    BOOLEAN DaclDefaulted;

    Status = NtQuerySecurityObject( Handle,
                                    DACL_SECURITY_INFORMATION,
                                    NULL,
                                    0,
                                    &Length );

    if (Status != STATUS_BUFFER_TOO_SMALL) {
        printf("DumpSecurity t0: NtQuerySecurityObject failed %lx\n",Status);
        exit(1);
    }

    SecurityDescriptor = malloc(Length);
    if (SecurityDescriptor == NULL) {
        printf("DumpSecurity: couldn't malloc buffer\n");
        exit(1);
    }

    Status = NtQuerySecurityObject( Handle,
                                    DACL_SECURITY_INFORMATION,
                                    SecurityDescriptor,
                                    Length,
                                    &Length );

    if (!NT_SUCCESS(Status)) {
        printf("DumpSecurity t1: NtQuerySecurityObject failed %lx\n",Status);
        exit(1);
    }

    Dacl = NULL;

    Status = RtlGetDaclSecurityDescriptor( SecurityDescriptor,
                                           &DaclPresent,
                                           &Dacl,
                                           &DaclDefaulted );
    if (!NT_SUCCESS(Status)) {
        printf("DumpSecurity t2: RtlGetDaclSecurityDescriptor failed %lx\n",Status);
    }

    if (DaclPresent) {
        PrintAcl(Dacl);
    } else {
        printf("\tAcl not present\n");
    }

}


void
print(
    PUNICODE_STRING  String
    )
{
    static  ANSI_STRING temp;
    static  char        tempbuffer[WORK_SIZE];

    temp.MaximumLength = WORK_SIZE;
    temp.Length = 0L;
    temp.Buffer = tempbuffer;

    RtlUnicodeStringToAnsiString(&temp, String, FALSE);
    printf("%s", temp.Buffer);
    return;
}


void
processargs(
    int argc,
    char *argv[]
    )
{
    ANSI_STRING temp;

    if ( (argc != 2) )
    {
        printf("Usage: %s <KeyPath>\n",
                argv[0]);
        exit(1);
    }

    RtlInitAnsiString(
        &temp,
        argv[1]
        );

    RtlAnsiStringToUnicodeString(
        &WorkName,
        &temp,
        FALSE
        );

    return;
}


BOOLEAN
SidTranslation(
    PSID Sid,
    PSTRING AccountName
    )
// AccountName is expected to have a large maximum length

{
    if (RtlEqualSid(Sid, WorldSid)) {
        RtlInitString( AccountName, "WORLD");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, LocalSid)) {
        RtlInitString( AccountName, "LOCAL");

        return(TRUE);
    }

    if (RtlEqualSid(Sid, NetworkSid)) {
        RtlInitString( AccountName, "NETWORK");

        return(TRUE);
    }

    if (RtlEqualSid(Sid, BatchSid)) {
        RtlInitString( AccountName, "BATCH");

        return(TRUE);
    }

    if (RtlEqualSid(Sid, InteractiveSid)) {
        RtlInitString( AccountName, "INTERACTIVE");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, LocalSystemSid)) {
        RtlInitString( AccountName, "SYSTEM");
        return(TRUE);
    }

//
//    if (RtlEqualSid(Sid, LocalManagerSid)) {
//      RtlInitString( AccountName, "LOCAL MANAGER");
//      return(TRUE);
//  }

//  if (RtlEqualSid(Sid, LocalAdminSid)) {
//      RtlInitString( AccountName, "LOCAL ADMIN");
//      return(TRUE);
//  }

    return(FALSE);

}


VOID
DisplayAccountSid(
    PSID Sid
    )
{
    UCHAR Buffer[128];
    STRING AccountName;
    UCHAR i;
    ULONG Tmp;
    PSID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    UCHAR SubAuthorityCount;

    Buffer[0] = 0;

    AccountName.MaximumLength = 127;
    AccountName.Length = 0;
    AccountName.Buffer = (PVOID)&Buffer[0];



    if (SidTranslation( (PSID)Sid, &AccountName) ) {

        printf("%s\n", AccountName.Buffer );

    } else {
        IdentifierAuthority = RtlIdentifierAuthoritySid(Sid);

        //
        // HACK! HACK!
        // The next line prints the revision of the SID.  Since there is no
        // rtl routine which gives us the SID revision, we must make due.
        // luckily, the revision field is the first field in the SID, so we
        // can just cast the pointer.
        //

        printf("S-%u-", (USHORT) *((PUCHAR) Sid) );

        if (  (IdentifierAuthority->Value[0] != 0)  ||
              (IdentifierAuthority->Value[1] != 0)     ){
            printf("0x%02hx%02hx%02hx%02hx%02hx%02hx",
                        IdentifierAuthority->Value[0],
                        IdentifierAuthority->Value[1],
                        IdentifierAuthority->Value[2],
                        IdentifierAuthority->Value[3],
                        IdentifierAuthority->Value[4],
                        IdentifierAuthority->Value[5] );
        } else {
            Tmp = IdentifierAuthority->Value[5]          +
                  (IdentifierAuthority->Value[4] <<  8)  +
                  (IdentifierAuthority->Value[3] << 16)  +
                  (IdentifierAuthority->Value[2] << 24);
            printf("%lu", Tmp);
        }

        SubAuthorityCount = *RtlSubAuthorityCountSid(Sid);
        for (i=0;i<SubAuthorityCount ;i++ ) {
            printf("-%lu", (*RtlSubAuthoritySid(Sid, i)));
        }
        printf("\n");

    }

}

VOID
InitVars()
{
    ULONG SidWithZeroSubAuthorities;
    ULONG SidWithOneSubAuthority;
    ULONG SidWithThreeSubAuthorities;
    ULONG SidWithFourSubAuthorities;

    SID_IDENTIFIER_AUTHORITY NullSidAuthority    = SECURITY_NULL_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority   = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY LocalSidAuthority   = SECURITY_LOCAL_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY CreatorSidAuthority = SECURITY_CREATOR_SID_AUTHORITY;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;


    //
    //  The following SID sizes need to be allocated
    //

    SidWithZeroSubAuthorities  = RtlLengthRequiredSid( 0 );
    SidWithOneSubAuthority     = RtlLengthRequiredSid( 1 );
    SidWithThreeSubAuthorities = RtlLengthRequiredSid( 3 );
    SidWithFourSubAuthorities  = RtlLengthRequiredSid( 4 );

    //
    //  Allocate and initialize the universal SIDs
    //

    NullSid         = (PSID)malloc(SidWithOneSubAuthority);
    WorldSid        = (PSID)malloc(SidWithOneSubAuthority);
    LocalSid        = (PSID)malloc(SidWithOneSubAuthority);
    CreatorOwnerSid = (PSID)malloc(SidWithOneSubAuthority);

    RtlInitializeSid( NullSid,    &NullSidAuthority, 1 );
    RtlInitializeSid( WorldSid,   &WorldSidAuthority, 1 );
    RtlInitializeSid( LocalSid,   &LocalSidAuthority, 1 );
    RtlInitializeSid( CreatorOwnerSid, &CreatorSidAuthority, 1 );

    *(RtlSubAuthoritySid( NullSid, 0 ))         = SECURITY_NULL_RID;
    *(RtlSubAuthoritySid( WorldSid, 0 ))        = SECURITY_WORLD_RID;
    *(RtlSubAuthoritySid( LocalSid, 0 ))        = SECURITY_LOCAL_RID;
    *(RtlSubAuthoritySid( CreatorOwnerSid, 0 )) = SECURITY_CREATOR_OWNER_RID;

    //
    // Allocate and initialize the NT defined SIDs
    //

    NtAuthoritySid  = (PSID)malloc(SidWithZeroSubAuthorities);
    DialupSid       = (PSID)malloc(SidWithOneSubAuthority);
    NetworkSid      = (PSID)malloc(SidWithOneSubAuthority);
    BatchSid        = (PSID)malloc(SidWithOneSubAuthority);
    InteractiveSid  = (PSID)malloc(SidWithOneSubAuthority);
    LocalSystemSid  = (PSID)malloc(SidWithOneSubAuthority);

    RtlInitializeSid( NtAuthoritySid,   &NtAuthority, 0 );
    RtlInitializeSid( DialupSid,        &NtAuthority, 1 );
    RtlInitializeSid( NetworkSid,       &NtAuthority, 1 );
    RtlInitializeSid( BatchSid,         &NtAuthority, 1 );
    RtlInitializeSid( InteractiveSid,   &NtAuthority, 1 );
    RtlInitializeSid( LocalSystemSid,   &NtAuthority, 1 );

    *(RtlSubAuthoritySid( DialupSid,       0 )) = SECURITY_DIALUP_RID;
    *(RtlSubAuthoritySid( NetworkSid,      0 )) = SECURITY_NETWORK_RID;
    *(RtlSubAuthoritySid( BatchSid,        0 )) = SECURITY_BATCH_RID;
    *(RtlSubAuthoritySid( InteractiveSid,  0 )) = SECURITY_INTERACTIVE_RID;
    *(RtlSubAuthoritySid( LocalSystemSid,  0 )) = SECURITY_LOCAL_SYSTEM_RID;

    return;

}



VOID
PrintAcl (
    IN PACL Acl
    )

/*++

Routine Description:

    This routine dumps an Acl for debug purposes (via printf).  It is
    specialized to dump standard aces.

Arguments:

    Acl - Supplies the Acl to dump

Return Value:

    None

--*/


{
    ULONG i;
    PKNOWN_ACE Ace;
    BOOLEAN KnownType;
    PCHAR AceTypes[] = { "Access Allowed",
                         "Access Denied ",
                         "System Audit  ",
                         "System Alarm  "
                       };

    if (Acl == NULL) {

        printf("\tAcl == ALL ACCESS GRANTED!\n");
        return;

    }

    //
    //  Dump the Acl header
    //

    printf("\tRevision: %02x", Acl->AclRevision);
    printf(" Size: %04x", Acl->AclSize);
    printf(" AceCount: %04x\n", Acl->AceCount);

    //
    //  Now for each Ace we want do dump it
    //

    for (i = 0, Ace = FirstAce(Acl);
         i < Acl->AceCount;
         i++, Ace = NextAce(Ace) ) {

        //
        //  print out the ace header
        //

        printf("\n\tAceHeader: %08lx ", *(PULONG)Ace);

        //
        //  special case on the standard ace types
        //

        if ((Ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) ||
            (Ace->Header.AceType == ACCESS_DENIED_ACE_TYPE) ||
            (Ace->Header.AceType == SYSTEM_AUDIT_ACE_TYPE) ||
            (Ace->Header.AceType == SYSTEM_ALARM_ACE_TYPE)) {

            //
            //  The following array is indexed by ace types and must
            //  follow the allowed, denied, audit, alarm seqeuence
            //

            PCHAR AceTypes[] = { "Access Allowed",
                                 "Access Denied ",
                                 "System Audit  ",
                                 "System Alarm  "
                               };

            printf(AceTypes[Ace->Header.AceType]);
            PrintAccessMask(Ace->Mask);
            KnownType = TRUE;

        } else {

            KnownType = FALSE;
            printf(" Unknown Ace Type\n");

        }

        printf("\n");

        printf("\tAceSize = %d\n",Ace->Header.AceSize);

        printf("\tAce Flags = ");
        if (Ace->Header.AceFlags & OBJECT_INHERIT_ACE) {
            printf("OBJECT_INHERIT_ACE\n");
            printf("                   ");
        }

        if (Ace->Header.AceFlags & CONTAINER_INHERIT_ACE) {
            printf("CONTAINER_INHERIT_ACE\n");
            printf("                   ");
        }

        if (Ace->Header.AceFlags & NO_PROPAGATE_INHERIT_ACE) {
            printf("NO_PROPAGATE_INHERIT_ACE\n");
            printf("                   ");
        }

        if (Ace->Header.AceFlags & INHERIT_ONLY_ACE) {
            printf("INHERIT_ONLY_ACE\n");
            printf("                   ");
        }

        if (Ace->Header.AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG) {
            printf("SUCCESSFUL_ACCESS_ACE_FLAG\n");
            printf("            ");
        }

        if (Ace->Header.AceFlags & FAILED_ACCESS_ACE_FLAG) {
            printf("FAILED_ACCESS_ACE_FLAG\n");
            printf("            ");
        }

        printf("\n");

        printf("\tSid = ");
        DisplayAccountSid(&Ace->SidStart);
    }

}


VOID
PrintAccessMask(
    IN ACCESS_MASK AccessMask
    )
{
    printf("\n\tAccess Mask: ");

    if (AccessMask == KEY_ALL_ACCESS) {
        printf("KEY_ALL_ACCESS\n\t             ");
        return;
    }
    if (AccessMask == KEY_READ) {
        printf("KEY_READ\n\t             ");
        return;
    }
    if (AccessMask == KEY_WRITE) {
        printf("KEY_WRITE\n\t             ");
        return;
    }

    if (AccessMask & KEY_QUERY_VALUE) {
        printf("KEY_QUERY_VALUE\n\t             ");
    }
    if (AccessMask & KEY_SET_VALUE) {
        printf("KEY_SET_VALUE\n\t             ");
    }
    if (AccessMask & KEY_CREATE_SUB_KEY) {
        printf("KEY_CREATE_SUB_KEY\n\t             ");
    }
    if (AccessMask & KEY_ENUMERATE_SUB_KEYS) {
        printf("KEY_ENUMERATE_SUB_KEYS\n\t             ");
    }
    if (AccessMask & KEY_NOTIFY) {
        printf("KEY_NOTIFY\n\t             ");
    }
    if (AccessMask & KEY_CREATE_LINK) {
        printf("KEY_CREATE_LINK\n\t             ");
    }
    if (AccessMask & GENERIC_ALL) {
        printf("GENERIC_ALL\n\t             ");
    }
    if (AccessMask & GENERIC_EXECUTE) {
        printf("GENERIC_EXECUTE\n\t             ");
    }
    if (AccessMask & GENERIC_WRITE) {
        printf("GENERIC_WRITE\n\t             ");
    }
    if (AccessMask & GENERIC_READ) {
        printf("GENERIC_READ\n\t             ");
    }
    if (AccessMask & GENERIC_READ) {
        printf("GENERIC_READ\n\t             ");
    }
    if (AccessMask & MAXIMUM_ALLOWED) {
        printf("MAXIMUM_ALLOWED\n\t             ");
    }
    if (AccessMask & ACCESS_SYSTEM_SECURITY) {
        printf("ACCESS_SYSTEM_SECURITY\n\t             ");
    }
    if (AccessMask & WRITE_OWNER) {
        printf("WRITE_OWNER\n\t             ");
    }
    if (AccessMask & WRITE_DAC) {
        printf("WRITE_DAC\n\t             ");
    }
    if (AccessMask & READ_CONTROL) {
        printf("READ_CONTROL\n\t             ");
    }
    if (AccessMask & DELETE) {
        printf("DELETE\n\t             ");
    }
}
