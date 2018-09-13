/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    uipers.c

Abstract:

    Temporary security context display command.


Author:

    Jim Kelly (JimK) 23-May-1991

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <string.h>

#define _TST_USER_  // User mode test


#include "tsevars.c"    // Common test variables
#include "tsecomm.c"    // Mode dependent macros and routines.


    GUID SystemAuthenticationId = SYSTEM_GUID;


VOID
DisplaySecurityContext(
    IN HANDLE TokenHandle
    );


VOID
DisplayAccountSid(
    PISID Sid
    );


BOOLEAN
SidTranslation(
    PSID Sid,
    PSTRING AccountName
    );




////////////////////////////////////////////////////////////////
//                                                            //
// Private Macros                                             //
//                                                            //
////////////////////////////////////////////////////////////////


#define PrintGuid(G)                                                     \
            printf( "(0x%lx-%hx-%hx-%hx-%hx-%hx-%hx-%hx-%hx-%hx-%hx)\n", \
                         (G)->Data1,    (G)->Data2,    (G)->Data3,               \
                         (G)->Data4[0], (G)->Data4[1], (G)->Data4[2],            \
                         (G)->Data4[3], (G)->Data4[4], (G)->Data4[5],            \
                         (G)->Data4[6], (G)->Data4[7]);                         \


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

    if (RtlEqualSid(Sid, LocalManagerSid)) {
        RtlInitString( AccountName, "LOCAL MANAGER");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, LocalAdminSid)) {
        RtlInitString( AccountName, "LOCAL ADMIN");
        return(TRUE);
    }

    return(FALSE);

}


VOID
DisplayAccountSid(
    PISID Sid
    )
{
    UCHAR Buffer[128];
    STRING AccountName;
    UCHAR i;
    ULONG Tmp;

    Buffer[0] = 0;

    AccountName.MaximumLength = 127;
    AccountName.Length = 0;
    AccountName.Buffer = (PVOID)&Buffer[0];



    if (SidTranslation( (PSID)Sid, &AccountName) ) {

        printf("%s\n", AccountName.Buffer );

    } else {
        printf("S-%lu-", (USHORT)Sid->Revision );
        if (  (Sid->IdentifierAuthority.Value[0] != 0)  ||
              (Sid->IdentifierAuthority.Value[1] != 0)     ){
            printf("0x%02hx%02hx%02hx%02hx%02hx%02hx",
                        (USHORT)Sid->IdentifierAuthority.Value[0],
                        (USHORT)Sid->IdentifierAuthority.Value[1],
                        (USHORT)Sid->IdentifierAuthority.Value[2],
                        (USHORT)Sid->IdentifierAuthority.Value[3],
                        (USHORT)Sid->IdentifierAuthority.Value[4],
                        (USHORT)Sid->IdentifierAuthority.Value[5] );
        } else {
            Tmp = (ULONG)Sid->IdentifierAuthority.Value[5]          +
                  (ULONG)(Sid->IdentifierAuthority.Value[4] <<  8)  +
                  (ULONG)(Sid->IdentifierAuthority.Value[3] << 16)  +
                  (ULONG)(Sid->IdentifierAuthority.Value[2] << 24);
            printf("%lu", Tmp);
        }


        for (i=0;i<Sid->SubAuthorityCount ;i++ ) {
            printf("-%lu", Sid->SubAuthority[i]);
        }
        printf("\n");

    }

}



BOOLEAN
DisplayPrivilegeName(
    PLUID Privilege
    )
{

    //
    // This should be rewritten to use RtlLookupPrivilegeName.
    //
    // First we should probably spec and write RtlLookupPrivilegeName.
    //

    if ( ((*Privilege)QuadPart == CreateTokenPrivilege.QuadPart))  {
        printf("SeCreateTokenPrivilege         ");
        return(TRUE);
    }

    if ( ((*Privilege).QuadPart == AssignPrimaryTokenPrivilege.QuadPart))  {
        printf("SeAssignPrimaryTokenPrivilege  ");
        return(TRUE);
    }

    if ( ((*Privilege).QuadPart == LockMemoryPrivilege.QuadPart))  {
        printf("SeLockMemoryPrivilege          ");
        return(TRUE);
    }

    if ( ((*Privilege).QuadPart == IncreaseQuotaPrivilege.QuadPart))  {
        printf("SeIncreaseQuotaPrivilege       ");
        return(TRUE);
    }

    if ( ((*Privilege).QuadPart == UnsolicitedInputPrivilege.QuadPart))  {
        printf("SeUnsolicitedInputPrivilege    ");
        return(TRUE);
    }

    if ( ((*Privilege).QuadPart == TcbPrivilege.QuadPart))  {
        printf("SeTcbPrivilege                 ");
        return(TRUE);
    }

    if ( ((*Privilege).QuadPart == SecurityPrivilege.QuadPart))  {
        printf("SeSecurityPrivilege (Security Operator)  ");
        return(TRUE);
    }


    if ( ((*Privilege).QuadPart == TakeOwnershipPrivilege.QuadPart)) {
        printf("SeTakeOwnershipPrivilege              ");
        return(TRUE);
    }

    if ( ((*Privilege).QuadPart == LpcReplyBoostPrivilege.QuadPart)) {
        printf("SeLpcReplyBoostPrivilege              ");
        return(TRUE);
    }

    if ( ((*Privilege).QuadPart == CreatePagefilePrivilege.QuadPart)) {
        printf("SeCreatePagefilePrivilege              ");
        return(TRUE);
    }

    if ( ((*Privilege).QuadPart == IncreaseBasePriorityPrivilege.QuadPart)) {
        printf("SeIncreaseBasePriorityPrivilege              ");
        return(TRUE);
    }

    if ( ((*Privilege).QuadPart == SystemProfilePrivilege.QuadPart)) {
        printf("SeSystemProfilePrivilege              ");
        return(TRUE);
    }

    if ( ((*Privilege).QuadPart == SystemtimePrivilege.QuadPart)) {
        printf("SeSystemtimePrivilege              ");
        return(TRUE);
    }

    if ( ((*Privilege).QuadPart == ProfileSingleProcessPrivilege.QuadPart)) {
        printf("SeProfileSingleProcessPrivilege              ");
        return(TRUE);
    }

    if ( ((*Privilege).QuadPart == CreatePermanentPrivilege.QuadPart)) {
        printf("SeCreatePermanentPrivilege              ");
        return(TRUE);
    }

    if ( ((*Privilege).QuadPart == BackupPrivilege.QuadPart)) {
        printf("SeBackupPrivilege              ");
        return(TRUE);
    }

    if ( ((*Privilege).QuadPart == RestorePrivilege.QuadPart)) {
        printf("SeRestorePrivilege              ");
        return(TRUE);
    }

    if ( ((*Privilege).QuadPart == ShutdownPrivilege.QuadPart)) {
        printf("SeShutdownPrivilege             ");
        return(TRUE);
    }

    if ( ((*Privilege).QuadPart == DebugPrivilege.QuadPart)) {
        printf("SeDebugPrivilege                ");
        return(TRUE);
    }

    if ( ((*Privilege).QuadPart == SystemEnvironmentPrivilege.QuadPart)) {
        printf("SeSystemEnvironmentPrivilege    ");
        return(TRUE);
    }

    return(FALSE);

}



VOID
DisplayPrivilege(
    PLUID_AND_ATTRIBUTES Privilege
    )
{


    if (!DisplayPrivilegeName(&Privilege->Luid)) {
        printf("(Unknown Privilege.  Value is: (0x%lx,0x%lx))",
               Privilege->Luid.HighPart,
               Privilege->Luid.LowPart
               );
    }



    //
    // Display the attributes assigned to the privilege.
    //

    printf("\n                           [");
    if (!(Privilege->Attributes & SE_PRIVILEGE_ENABLED)) {
        printf("Not ");
    }
    printf("Enabled");

    //printf(" / ");
    //if (!(Privilege->Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT)) {
    //    printf("Not ");
    //}
    //printf("Enabled By Default");


    printf("]\n");
    printf("                           ");


    return;

}


VOID
DisplaySecurityContext(
    IN HANDLE TokenHandle
    )
{

#define BUFFER_SIZE (2048)

    NTSTATUS Status;
    ULONG i;
    ULONG ReturnLength;
    TOKEN_STATISTICS ProcessTokenStatistics;
    GUID AuthenticationId;
    UCHAR Buffer[BUFFER_SIZE];


    PTOKEN_USER UserId;
    PTOKEN_OWNER DefaultOwner;
    PTOKEN_PRIMARY_GROUP PrimaryGroup;
    PTOKEN_GROUPS GroupIds;
    PTOKEN_PRIVILEGES Privileges;




    /////////////////////////////////////////////////////////////////////////
    //                                                                     //
    // Logon ID                                                            //
    //                                                                     //
    /////////////////////////////////////////////////////////////////////////

    Status = NtQueryInformationToken(
                 TokenHandle,                  // Handle
                 TokenStatistics,              // TokenInformationClass
                 &ProcessTokenStatistics,      // TokenInformation
                 sizeof(TOKEN_STATISTICS),     // TokenInformationLength
                 &ReturnLength                 // ReturnLength
                 );
    ASSERT(NT_SUCCESS(Status));
    AuthenticationId = ProcessTokenStatistics.AuthenticationId;

    printf("     Logon Session:        ");
    if (RtlEqualGuid(&AuthenticationId, &SystemAuthenticationId )) {
        printf("(System Logon Session)\n");
    } else {
        PrintGuid( &AuthenticationId );
    }




    /////////////////////////////////////////////////////////////////////////
    //                                                                     //
    // User Id                                                             //
    //                                                                     //
    /////////////////////////////////////////////////////////////////////////

    UserId = (PTOKEN_USER)&Buffer[0];
    Status = NtQueryInformationToken(
                 TokenHandle,              // Handle
                 TokenUser,                // TokenInformationClass
                 UserId,                   // TokenInformation
                 BUFFER_SIZE,              // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );


    ASSERT(NT_SUCCESS(Status));

    printf("           User id:        ");
    DisplayAccountSid( (PISID)UserId->User.Sid );





    /////////////////////////////////////////////////////////////////////////
    //                                                                     //
    // Default Owner                                                       //
    //                                                                     //
    /////////////////////////////////////////////////////////////////////////

    DefaultOwner = (PTOKEN_OWNER)&Buffer[0];

    Status = NtQueryInformationToken(
                 TokenHandle,              // Handle
                 TokenOwner,               // TokenInformationClass
                 DefaultOwner,             // TokenInformation
                 BUFFER_SIZE,              // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );


    ASSERT(NT_SUCCESS(Status));

    printf("     Default Owner:        ");
    DisplayAccountSid( (PISID)DefaultOwner->Owner );






    /////////////////////////////////////////////////////////////////////////
    //                                                                     //
    // Primary Group                                                       //
    //                                                                     //
    /////////////////////////////////////////////////////////////////////////

    PrimaryGroup = (PTOKEN_PRIMARY_GROUP)&Buffer[0];

    Status = NtQueryInformationToken(
                 TokenHandle,              // Handle
                 TokenPrimaryGroup,        // TokenInformationClass
                 PrimaryGroup,             // TokenInformation
                 BUFFER_SIZE,              // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );


    ASSERT(NT_SUCCESS(Status));

    printf("     Primary Group:        ");
    DisplayAccountSid( (PISID)PrimaryGroup->PrimaryGroup );






    /////////////////////////////////////////////////////////////////////////
    //                                                                     //
    // Group Ids                                                           //
    //                                                                     //
    /////////////////////////////////////////////////////////////////////////

    printf("\n");
    GroupIds = (PTOKEN_GROUPS)&Buffer[0];
    Status   = NtQueryInformationToken(
                   TokenHandle,              // Handle
                   TokenGroups,              // TokenInformationClass
                   GroupIds,                 // TokenInformation
                   BUFFER_SIZE,              // TokenInformationLength
                   &ReturnLength             // ReturnLength
                   );


    ASSERT(NT_SUCCESS(Status));

    //printf("  Number of groups:        %ld\n", GroupIds->GroupCount);
    printf("            Groups:        ");

    for (i=0; i < GroupIds->GroupCount; i++ ) {
        //printf("                           Group %ld: ", i);
        DisplayAccountSid( (PISID)GroupIds->Groups[i].Sid );
        printf("                           ");
    }





    /////////////////////////////////////////////////////////////////////////
    //                                                                     //
    // Privileges                                                          //
    //                                                                     //
    /////////////////////////////////////////////////////////////////////////

    printf("\n");
    Privileges = (PTOKEN_PRIVILEGES)&Buffer[0];
    Status   = NtQueryInformationToken(
                   TokenHandle,              // Handle
                   TokenPrivileges,          // TokenInformationClass
                   Privileges,               // TokenInformation
                   BUFFER_SIZE,              // TokenInformationLength
                   &ReturnLength             // ReturnLength
                   );


    ASSERT(NT_SUCCESS(Status));

    printf("        Privileges:        ");
    if (Privileges->PrivilegeCount > 0) {

        for (i=0; i < Privileges->PrivilegeCount; i++ ) {
            DisplayPrivilege( &(Privileges->Privileges[i]) );
        }
    } else {
        printf("(none assigned)\n");
    }



    return;

}


BOOLEAN
main()
{

    NTSTATUS Status;
    HANDLE ProcessToken;


    TSeVariableInitialization();    // Initialize global variables

    printf("\n");


    //
    // Open our process token
    //

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_QUERY,
                 &ProcessToken
                 );
    if (!NT_SUCCESS(Status)) {
        printf("I'm terribly sorry, but you don't seem to have access to\n");
        printf("open your own process's token.\n");
        printf("\n");
        return(FALSE);
    }

    printf("Your process level security context is:\n");
    printf("\n");
    DisplaySecurityContext( ProcessToken );


    Status = NtClose( ProcessToken );

    return(TRUE);
}

