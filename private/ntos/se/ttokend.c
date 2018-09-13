/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tmachine.c

Abstract:

    This module tests token duplication.

Author:

    Jim Kelly    (JimK) 8-Feb-1994

Environment:

    User Mode - Win32

Revision History:


--*/







///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <nt.h>
#include <ntsam.h>
#include <ntsamp.h>
#include <ntlsa.h>
#include <ntrpcp.h>     // prototypes for MIDL user functions
#include <seopaque.h>
#include <string.h>



#ifdef NOT_PART_OF_PROGRAM


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Definitions                                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////





#define TMPP_USER_NAME_ADMIN           "Administrator"
#define TMPP_USER_NAME_GUEST           "Guest"
#define TMPP_GROUP_NAME_ADMINS         "Domain Admins"
#define TMPP_GROUP_NAME_USERS          "Domain Users"
#define TMPP_GROUP_NAME_NONE           "None"
#define TMPP_ALIAS_NAME_ADMINS         "Administrators"
#define TMPP_ALIAS_NAME_SYSTEM_OPS     "System Operators"
#define TMPP_ALIAS_NAME_POWER_USERS    "Power Users"
#define TMPP_ALIAS_NAME_USERS          "Users"
#define TMPP_ALIAS_NAME_GUESTS         "Guests"
#define TMPP_ALIAS_NAME_ACCOUNT_OPS    "Account Operators"
#define TMPP_ALIAS_NAME_PRINT_OPS      "Print Operators"
#define TMPP_ALIAS_NAME_BACKUP_OPS     "Backup Operators"



#define GROUP_NAME1             "GROUP1"
#define ALIAS_NAME1             "ALIAS1"
#define ALIAS_NAME2             "ALIAS2"
#define USER_NAME1              "USER1"
#define USER_NAME2              "USER2"
#define USER_NAME3              "USER3"

// Keep these names not longer than 8 char's until long registry names supported
#define DUMMY_NAME1             "DName1"
#define DUMMY_NAME2             "2emaNuD"

#define DUMMY_STRING1           "This is test string 1"
#define DUMMY_STRING2           "Test String2 - test string 2 - tEST sTRING 2"

#define ALL_NAMES_COUNT         (3)
#define SOME_NAMES_COUNT        (7)
#define NO_NAMES_COUNT          (2)

#define LOOKUP_KNOWN_NAME0      TMPP_USER_NAME_ADMIN
#define LOOKUP_KNOWN_NAME1_A    TMPP_GROUP_NAME_NONE
#define LOOKUP_KNOWN_NAME2_A    TMPP_GROUP_NAME_NONE
#define LOOKUP_KNOWN_NAME1_P    TMPP_GROUP_NAME_USERS
#define LOOKUP_KNOWN_NAME2_P    TMPP_GROUP_NAME_USERS

#define LOOKUP_KNOWN_NAME0_RID  DOMAIN_USER_RID_ADMIN
#define LOOKUP_KNOWN_NAME1_RID  DOMAIN_GROUP_RID_USERS
#define LOOKUP_KNOWN_NAME2_RID  DOMAIN_GROUP_RID_USERS

#define LOOKUP_UNKNOWN_NAME0    "JoeJoe"
#define LOOKUP_UNKNOWN_NAME1    "Tanya"
#define LOOKUP_UNKNOWN_NAME2    "Fred"
#define LOOKUP_UNKNOWN_NAME3    "Anyone"

#define LOOKUP_KNOWN_NAME0_USE  (SidTypeUser)
#define LOOKUP_KNOWN_NAME1_USE  (SidTypeGroup)
#define LOOKUP_KNOWN_NAME2_USE  (SidTypeGroup)


//
// This byte is expected to be different in the DummyLogonHours and
// NoRestrictionLogonHours.
//

#define LOGON_HOURS_DIFFERENT_OFFSET    (5)



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

LARGE_INTEGER LargeInteger1,
              LargeInteger2;

UNICODE_STRING DummyName1,
               DummyName2,
               DummyString1,
               DummyString2;

STRING         DummyAnsiString1,
               DummyAnsiString2;

LOGON_HOURS    NoLogonRestriction,
               DummyLogonHours;

CHAR           NoLogonRestrictionBitMask[21],
               DummyLogonHoursBitMask[21];


UNICODE_STRING  AllNames[ALL_NAMES_COUNT],
                SomeNames[SOME_NAMES_COUNT],
                NoNames[NO_NAMES_COUNT];


SID_NAME_USE    AllUses[ALL_NAMES_COUNT],
                SomeUses[SOME_NAMES_COUNT],
                NoUses[NO_NAMES_COUNT];

ULONG           AllRids[ALL_NAMES_COUNT],
                SomeRids[SOME_NAMES_COUNT],
                NoRids[NO_NAMES_COUNT];


PSID            BuiltinDomainSid,
                AccountDomainSid,
                PrimaryDomainSid,
                WorldSid,
                AdminsAliasSid,
                AccountAliasSid;


UNICODE_STRING  BuiltinDomainName,
                AccountDomainName,
                PrimaryDomainName;

BOOLEAN         AccountDomainIsNotPrimaryDomain;


//
// These are NOT mutually exclusive
//

BOOLEAN         BuiltinDomainTest,      // Test the builting domain
                SecurityOperatorTest,   // Test auditing accessibility
                AccountOpAliasTest,     // Test account operator functions
                AdminsAliasTest;        // Test domain admin functions



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private macros                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


//
// VOID
// TST_SUCCESS_ASSERT( IN NTSTATUS S );
//

#define TST_SUCCESS_ASSERT( S )                                             \
{                                                                           \
    if ( !NT_SUCCESS((S)) ) {                                               \
        printf("\n** SUCCESS STATUS ASSERTION FAILURE **\n");             \
        printf("   Status is:  0x%lx\n", (S) );                           \
        ASSERT(NT_SUCCESS((S)));                                            \
    }                                                                       \
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service prototypes                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////




BOOLEAN
TInitialize( VOID );

BOOLEAN
EnableSecurityPrivilege( VOID );

VOID
DetermineTestsToRun( VOID );

VOID
SeeIfSidIsSpecial(
    IN PSID Sid
    );

BOOLEAN
ServerTestSuite(
    PHANDLE ServerHandle,
    PHANDLE DomainHandle,
    PHANDLE BuiltinDomainHandle,
    PSID    *DomainSid
    );

BOOLEAN
SecurityTestSuite(
    HANDLE ServerHandle,
    HANDLE DomainHandle,
    ULONG Pass
    );

BOOLEAN
CheckReturnedSD(
    IN SECURITY_INFORMATION SI,
    IN PSECURITY_DESCRIPTOR SD,
    IN BOOLEAN              PrintTestSuccess
    );


BOOLEAN
DomainTestSuite(
    HANDLE DomainHandle
    );

BOOLEAN
GroupTestSuite(
    HANDLE DomainHandle,
    ULONG  Pass
    );

BOOLEAN
AliasTestSuite(
    HANDLE DomainHandle,
    HANDLE BuiltinDomainHandle,
    PSID DomainSid,
    ULONG  Pass
    );

BOOLEAN
UserTestSuite(
    HANDLE DomainHandle,
    ULONG Pass
    );


NTSTATUS
SampSetDomainPolicy( VOID );


NTSTATUS
SampGetLsaDomainInfo(
    PPOLICY_ACCOUNT_DOMAIN_INFO *PolicyAccountDomainInfo,
    PPOLICY_PRIMARY_DOMAIN_INFO *PolicyPrimaryDomainInfo
    );


//
// The following are in WRAPPERS.C, but are prototyped here since this
// test is the only thing that should ever call them.
//

NTSTATUS
SamTestPrivateFunctionsDomain(
    IN HANDLE DomainHandle
    );

NTSTATUS
SamTestPrivateFunctionsUser(
    IN HANDLE UserHandle
    );


#endif // NOT_PART_OF_PROGRAM

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



VOID
main (argc, argv)
int argc;
char **argv;

/*++

Routine Description:

    This is the main entry routine for this test.

Arguments:

    NONE


Return Value:




--*/
{
    NTSTATUS
        NtStatus;

    HANDLE
        h1, h2, h3;

    OBJECT_ATTRIBUTES
        ObjectAttributes;

    SECURITY_QUALITY_OF_SERVICE
        Qos;

    //
    // Duplicate our primary token to get an impersonation token.
    // (no security QOS causes duplicate to have Anonymous level)
    //

    NtStatus = NtOpenProcessToken( NtCurrentProcess(),
                                   TOKEN_DUPLICATE,
                                   &h1);
    printf("Test: Open Process Token: 0x%lx\n", NtStatus);

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, 0, NULL );
    NtStatus = NtDuplicateToken( h1,
                                 TOKEN_DUPLICATE,
                                 &ObjectAttributes,
                                 FALSE,         // EffectiveOnly
                                 TokenImpersonation,
                                 &h2);
    printf("Test: Duplicate Primary to anonymous Impersonation: 0x%lx\n", NtStatus);


    //
    // Now duplicate that to get a primary
    //
    NtStatus = NtDuplicateToken( h2,
                                 TOKEN_DUPLICATE,
                                 &ObjectAttributes,
                                 FALSE,         // EffectiveOnly
                                 TokenPrimary,
                                 &h3);
    printf("Test: Duplicate anonymous Impersonation to Primary: 0x%lx\n", NtStatus);

    //
    // Now try it again with Impersonate level.
    //

    Qos.Length = sizeof(Qos);
    Qos.ImpersonationLevel = SecurityImpersonation;
    Qos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    Qos.EffectiveOnly = FALSE;

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, 0, NULL );
    ObjectAttributes.SecurityQualityOfService = &Qos;

    NtStatus = NtDuplicateToken( h1,
                                 TOKEN_DUPLICATE,
                                 &ObjectAttributes,
                                 FALSE,         // EffectiveOnly
                                 TokenImpersonation,
                                 &h2);
    printf("Test: Duplicate Primary to IMPERSONATE Impersonation: 0x%lx\n", NtStatus);


    //
    // Now duplicate that to get a primary
    //
    NtStatus = NtDuplicateToken( h2,
                                 TOKEN_DUPLICATE,
                                 &ObjectAttributes,
                                 FALSE,         // EffectiveOnly
                                 TokenPrimary,
                                 &h3);
    printf("Test: Duplicate IMPERSONATE Impersonation to Primary: 0x%lx\n", NtStatus);

    return;
}

#ifdef NOT_PART_OF_PROGRAM

BOOLEAN
TInitialize (
    VOID
    )

/*++

Routine Description:

    Initialize test variables, et cetera.

Arguments:

    None.

Return Value:


    Note:


--*/
{
    NTSTATUS NtStatus;
    STRING Name;
    ULONG i;

    SID_IDENTIFIER_AUTHORITY    WorldSidAuthority        = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    DomainSidAuthority       = {0,0,0,0,0,0};
    SID_IDENTIFIER_AUTHORITY    BuiltinAuthority         = SECURITY_NT_AUTHORITY;


    //
    // Get the domain SIDs from the policy database...
    //

    NtStatus = SampSetDomainPolicy();
    ASSERT(NT_SUCCESS(NtStatus));


    //
    // A random large integer value..
    //

    LargeInteger1.LowPart = 1234;
    LargeInteger1.HighPart = 0;

    LargeInteger2.LowPart = 4321;
    LargeInteger2.HighPart = 0;


    RtlInitString( &Name, DUMMY_NAME1 );
    NtStatus = RtlAnsiStringToUnicodeString( &DummyName1, &Name, TRUE );
    TST_SUCCESS_ASSERT(NtStatus);

    RtlInitString( &Name, DUMMY_NAME2 );
    NtStatus = RtlAnsiStringToUnicodeString( &DummyName2, &Name, TRUE );
    TST_SUCCESS_ASSERT(NtStatus);


    RtlInitString( &DummyAnsiString1, DUMMY_STRING1 );
    NtStatus = RtlAnsiStringToUnicodeString( &DummyString1, &DummyAnsiString1, TRUE );
    TST_SUCCESS_ASSERT(NtStatus);

    RtlInitString( &DummyAnsiString2, DUMMY_STRING2 );
    NtStatus = RtlAnsiStringToUnicodeString( &DummyString2, &DummyAnsiString2, TRUE );
    TST_SUCCESS_ASSERT(NtStatus);


    DummyLogonHours.UnitsPerWeek = SAM_HOURS_PER_WEEK;
    DummyLogonHours.LogonHours   = &DummyLogonHoursBitMask[0];
    DummyLogonHoursBitMask[LOGON_HOURS_DIFFERENT_OFFSET] = 103; // Any non-zero value

    NoLogonRestriction.UnitsPerWeek = SAM_HOURS_PER_WEEK;
    NoLogonRestriction.LogonHours   = &NoLogonRestrictionBitMask[0];
    for ( i=0; i<(ULONG)((NoLogonRestriction.UnitsPerWeek+7)/8); i++) {
        NoLogonRestrictionBitMask[0] = 0;
    }



    //
    //  Initialize some SIDs
    //


    WorldSid = RtlAllocateHeap( RtlProcessHeap(), 0, RtlLengthRequiredSid(1) );
    ASSERT(WorldSid != NULL);
    RtlInitializeSid( WorldSid, &WorldSidAuthority, 1 );
    *(RtlSubAuthoritySid( WorldSid, 0 )) = SECURITY_WORLD_RID;

    AdminsAliasSid  = RtlAllocateHeap(RtlProcessHeap(), 0,RtlLengthRequiredSid( 2 ));
    ASSERT(AdminsAliasSid != NULL);
    RtlInitializeSid( AdminsAliasSid,   &BuiltinAuthority, 2 );
    *(RtlSubAuthoritySid( AdminsAliasSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( AdminsAliasSid,  1 )) = DOMAIN_ALIAS_RID_ADMINS;

    AccountAliasSid  = RtlAllocateHeap(RtlProcessHeap(), 0,RtlLengthRequiredSid( 2 ));
    ASSERT(AccountAliasSid != NULL);
    RtlInitializeSid( AccountAliasSid,   &BuiltinAuthority, 2 );
    *(RtlSubAuthoritySid( AccountAliasSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( AccountAliasSid,  1 )) = DOMAIN_ALIAS_RID_ACCOUNT_OPS;




    //
    // Initialize some stuff for SID and NAME lookup operations
    //

    RtlInitString( &Name, LOOKUP_KNOWN_NAME0 );

    AllUses[0] = LOOKUP_KNOWN_NAME0_USE;  AllRids[0] = LOOKUP_KNOWN_NAME0_RID;
    NtStatus = RtlAnsiStringToUnicodeString( &AllNames[0], &Name, TRUE ); TST_SUCCESS_ASSERT(NtStatus);
    SomeUses[0] = LOOKUP_KNOWN_NAME0_USE;  SomeRids[0] = LOOKUP_KNOWN_NAME0_RID;
    NtStatus = RtlAnsiStringToUnicodeString( &SomeNames[0], &Name, TRUE ); TST_SUCCESS_ASSERT(NtStatus);


    if (AccountDomainIsNotPrimaryDomain == TRUE) {
        RtlInitString( &Name, LOOKUP_KNOWN_NAME1_A );
    } else {
        RtlInitString( &Name, LOOKUP_KNOWN_NAME1_P );
    }
    AllUses[1] = LOOKUP_KNOWN_NAME1_USE;  AllRids[1] = LOOKUP_KNOWN_NAME1_RID;
    NtStatus = RtlAnsiStringToUnicodeString( &AllNames[1], &Name, TRUE ); TST_SUCCESS_ASSERT(NtStatus);
    SomeUses[1] = LOOKUP_KNOWN_NAME1_USE;  SomeRids[1] = LOOKUP_KNOWN_NAME1_RID;
    NtStatus = RtlAnsiStringToUnicodeString( &SomeNames[1], &Name, TRUE ); TST_SUCCESS_ASSERT(NtStatus);

    RtlInitString( &Name, LOOKUP_UNKNOWN_NAME0 );

    SomeUses[2] = SidTypeUnknown;
    NtStatus = RtlAnsiStringToUnicodeString( &SomeNames[2], &Name, TRUE ); TST_SUCCESS_ASSERT(NtStatus);
    NoUses[0] = SidTypeUnknown;
    NtStatus = RtlAnsiStringToUnicodeString( &NoNames[0], &Name, TRUE ); TST_SUCCESS_ASSERT(NtStatus);


    RtlInitString( &Name, LOOKUP_UNKNOWN_NAME1 );

    SomeUses[3] = SidTypeUnknown;
    NtStatus = RtlAnsiStringToUnicodeString( &SomeNames[3], &Name, TRUE ); TST_SUCCESS_ASSERT(NtStatus);
    NoUses[1] = SidTypeUnknown;
    NtStatus = RtlAnsiStringToUnicodeString( &NoNames[1], &Name, TRUE ); TST_SUCCESS_ASSERT(NtStatus);



    RtlInitString( &Name, LOOKUP_UNKNOWN_NAME2 );

    SomeUses[4] = SidTypeUnknown;
    NtStatus = RtlAnsiStringToUnicodeString( &SomeNames[4], &Name, TRUE ); TST_SUCCESS_ASSERT(NtStatus);


    if (AccountDomainIsNotPrimaryDomain == TRUE) {
        RtlInitString( &Name, LOOKUP_KNOWN_NAME2_A );
    } else {
        RtlInitString( &Name, LOOKUP_KNOWN_NAME2_P );
    }
    AllUses[2] = LOOKUP_KNOWN_NAME2_USE;  AllRids[2] = LOOKUP_KNOWN_NAME2_RID;
    NtStatus = RtlAnsiStringToUnicodeString( &AllNames[2], &Name, TRUE ); TST_SUCCESS_ASSERT(NtStatus);
    SomeUses[5] = LOOKUP_KNOWN_NAME2_USE;  SomeRids[5] = LOOKUP_KNOWN_NAME2_RID;
    NtStatus = RtlAnsiStringToUnicodeString( &SomeNames[5], &Name, TRUE ); TST_SUCCESS_ASSERT(NtStatus);



    RtlInitString( &Name, LOOKUP_UNKNOWN_NAME3 );

    SomeUses[6] = SidTypeUnknown;
    NtStatus = RtlAnsiStringToUnicodeString( &SomeNames[6], &Name, TRUE ); TST_SUCCESS_ASSERT(NtStatus);


    DetermineTestsToRun();

    return(TRUE);
}


NTSTATUS
SampSetDomainPolicy(
    )
/*++


Routine Description:

    This routine sets the names and SIDs for the builtin and account domains.
    The builtin account domain has a well known name and SID.
    The account domain has these stored in the Policy database.


    It places the information for these domains in:

            BuiltinDomainSid
            BuiltinDomainName
            AccountDomainSid
            AccountDomainName
            PrimaryDomainSid
            PrimaryDomainName

    It also sets the boolean:

            AccountDomainIsNotPrimaryDomain

    to TRUE if the account domain is found to be different from the
    Primary Domain.

Arguments:

    None.

Return Value:

--*/

{
    NTSTATUS NtStatus;
    PPOLICY_ACCOUNT_DOMAIN_INFO PolicyAccountDomainInfo;
    PPOLICY_PRIMARY_DOMAIN_INFO PolicyPrimaryDomainInfo;
    SID_IDENTIFIER_AUTHORITY BuiltinAuthority = SECURITY_NT_AUTHORITY;

    //
    // Builtin domain - well-known name and SID
    //

    RtlInitUnicodeString( &BuiltinDomainName, L"Builtin");

    BuiltinDomainSid  = RtlAllocateHeap(RtlProcessHeap(), 0,RtlLengthRequiredSid( 1 ));
    ASSERT( BuiltinDomainSid != NULL );
    RtlInitializeSid( BuiltinDomainSid,   &BuiltinAuthority, 1 );
    *(RtlSubAuthoritySid( BuiltinDomainSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;

    //
    // Account domain
    //

    NtStatus = SampGetLsaDomainInfo(
                   &PolicyAccountDomainInfo,
                   &PolicyPrimaryDomainInfo
                   );

    if (!NT_SUCCESS(NtStatus)) {

        return(NtStatus);
    }

    AccountDomainSid = PolicyAccountDomainInfo->DomainSid;
    AccountDomainName = PolicyAccountDomainInfo->DomainName;

    PrimaryDomainSid = PolicyPrimaryDomainInfo->Sid;
    PrimaryDomainName = PolicyPrimaryDomainInfo->Name;

    //
    // Determine whether the account domain is a primary domain.
    //

    AccountDomainIsNotPrimaryDomain =
        !RtlEqualUnicodeString( &PrimaryDomainName, &AccountDomainName, TRUE);

    return(NtStatus);;
}



NTSTATUS
SampGetLsaDomainInfo(
    PPOLICY_ACCOUNT_DOMAIN_INFO *PolicyAccountDomainInfo,
    PPOLICY_PRIMARY_DOMAIN_INFO *PolicyPrimaryDomainInfo
    )

/*++

Routine Description:

    This routine retrieves ACCOUNT domain information from the LSA
    policy database.


Arguments:

    PolicyAccountDomainInfo - Receives a pointer to a
        POLICY_ACCOUNT_DOMAIN_INFO structure containing the account
        domain info.

    PolicyPrimaryDomainInfo - Receives a pointer to a
        POLICY_PRIMARY_DOMAIN_INFO structure containing the Primary
        domain info.


Return Value:

    STATUS_SUCCESS - Succeeded.

    Other status values that may be returned from:

             LsaOpenPolicy()
             LsaQueryInformationPolicy()
--*/

{
    NTSTATUS Status, IgnoreStatus;

    LSA_HANDLE PolicyHandle;
    OBJECT_ATTRIBUTES PolicyObjectAttributes;

    //
    // Open the policy database
    //

    InitializeObjectAttributes( &PolicyObjectAttributes,
                                  NULL,             // Name
                                  0,                // Attributes
                                  NULL,             // Root
                                  NULL );           // Security Descriptor

    Status = LsaOpenPolicy( NULL,
                            &PolicyObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &PolicyHandle );
    if ( NT_SUCCESS(Status) ) {

        //
        // Query the account domain information
        //

        Status = LsaQueryInformationPolicy( PolicyHandle,
                                            PolicyAccountDomainInformation,
                                            (PVOID *)PolicyAccountDomainInfo );
#if DBG
        if ( NT_SUCCESS(Status) ) {
            ASSERT( (*PolicyAccountDomainInfo) != NULL );
            ASSERT( (*PolicyAccountDomainInfo)->DomainSid != NULL );
            ASSERT( (*PolicyAccountDomainInfo)->DomainName.Buffer != NULL );
        }
#endif \\DBG

        //
        // Query the Primary domain information
        //

        Status = LsaQueryInformationPolicy( PolicyHandle,
                                            PolicyPrimaryDomainInformation,
                                            (PVOID *)PolicyPrimaryDomainInfo );
#if DBG
        if ( NT_SUCCESS(Status) ) {
            ASSERT( (*PolicyPrimaryDomainInfo) != NULL );
            ASSERT( (*PolicyPrimaryDomainInfo)->Sid != NULL );
            ASSERT( (*PolicyPrimaryDomainInfo)->Name.Buffer != NULL );
        }
#endif \\DBG

        IgnoreStatus = LsaClose( PolicyHandle );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    return(Status);
}




PSID
CreateUserSid(
    PSID    DomainSid,
    ULONG   Rid
    )

/*++

Routine Description:

    This function creates a domain account sid given a domain sid and
    the relative id of the account within the domain.

Arguments:

    None.

Return Value:

    Pointer to Sid, or NULL on failure.
    The returned Sid must be freed with DeleteUserSid

--*/
{

    NTSTATUS IgnoreStatus;
    PSID AccountSid;
    UCHAR AccountSubAuthorityCount = *RtlSubAuthorityCountSid(DomainSid) + (UCHAR)1;
    ULONG AccountSidLength = RtlLengthRequiredSid(AccountSubAuthorityCount);
    PULONG  RidLocation;

    // Temp sanity check
    ASSERT(AccountSidLength == SeLengthSid(DomainSid) + sizeof(ULONG));

    //
    // Allocate space for the account sid
    //

    AccountSid = MIDL_user_allocate(AccountSidLength);

    if (AccountSid != NULL) {

        //
        // Copy the domain sid into the first part of the account sid
        //

        IgnoreStatus = RtlCopySid(AccountSidLength, AccountSid, DomainSid);
        ASSERT(NT_SUCCESS(IgnoreStatus));

        //
        // Increment the account sid sub-authority count
        //

        *RtlSubAuthorityCountSid(AccountSid) = AccountSubAuthorityCount;

        //
        // Add the rid as the final sub-authority
        //

        RidLocation = RtlSubAuthoritySid(AccountSid, AccountSubAuthorityCount - 1);
        *RidLocation = Rid;
    }

    return(AccountSid);
}



VOID
DeleteUserSid(
    PSID    UserSid
    )

/*++

Routine Description:

    Frees a sid returned by CreateUserSid.

Arguments:

    None.

Return Value:

    None.

--*/
{
    MIDL_user_free(UserSid);
}



BOOLEAN
EnableSecurityPrivilege(
    VOID
    )

/*++

Routine Description:

    This function enabled the SeSecurityPrivilege privilege.

Arguments:

    None.

Return Value:

    TRUE  if privilege successfully enabled.
    FALSE if not successfully enabled.

--*/
{

    NTSTATUS Status;
    HANDLE Token;
    LUID SecurityPrivilege;
    PTOKEN_PRIVILEGES NewState;
    ULONG ReturnLength;


    //
    // Open our own token
    //

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_ADJUST_PRIVILEGES,
                 &Token
                 );
    if (!NT_SUCCESS(Status)) {
        printf(" \n\n\n");
        printf("Tsamobj: Can't open process token to enable Security Privilege.\n");
        printf("         Completion status of NtOpenProcessToken() is: 0x%lx\n", Status);
        printf("\n");
        return(FALSE);
    }


    //
    // Initialize the adjustment structure
    //

    SecurityPrivilege =
        RtlConvertLongToLargeInteger(SE_SECURITY_PRIVILEGE);

    ASSERT( (sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES)) < 100);
    NewState = RtlAllocateHeap( RtlProcessHeap(), 0, 100 );

    NewState->PrivilegeCount = 1;
    NewState->Privileges[0].Luid = SecurityPrivilege;
    NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;


    //
    // Set the state of the privilege to ENABLED.
    //

    Status = NtAdjustPrivilegesToken(
                 Token,                            // TokenHandle
                 FALSE,                            // DisableAllPrivileges
                 NewState,                         // NewState
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );
    // don't use NT_SUCCESS here because STATUS_NOT_ALL_ASSIGNED is a success status
    if (Status != STATUS_SUCCESS) {
        return(FALSE);
    }


    //
    // Clean up some stuff before returning
    //

    RtlFreeHeap( RtlProcessHeap(), 0, NewState );
    Status = NtClose( Token );
    ASSERT(NT_SUCCESS(Status));


    return TRUE;

}



VOID
printfSid(
    PSID    Sid
    )

/*++

Routine Description:

    Prints a sid

Arguments:

    None.

Return Value:

    None.

--*/
{
    UCHAR   Buffer[128];
    UCHAR   String[128];
    UCHAR   i;
    ULONG   Tmp;
    PISID   iSid = (PISID)Sid;  // pointer to opaque structure
    PSID    NextSid = (PSID)Buffer;

    ASSERT(sizeof(Buffer) >= RtlLengthRequiredSid(1));

    {
        SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_WORLD_SID_AUTHORITY;
        RtlInitializeSid(NextSid, &SidAuthority, 1 );
        *(RtlSubAuthoritySid(NextSid, 0)) = SECURITY_WORLD_RID;
        if (RtlEqualSid(Sid, NextSid)) {
            printf("World");
            return;
        }
    }

    {
        SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_LOCAL_SID_AUTHORITY;
        RtlInitializeSid(NextSid, &SidAuthority, 1 );
        *(RtlSubAuthoritySid(NextSid, 0)) = SECURITY_LOCAL_RID;
        if (RtlEqualSid(Sid, NextSid)) {
            printf("Local");
            return;
        }
    }

    {
        SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_CREATOR_SID_AUTHORITY;
        RtlInitializeSid(NextSid, &SidAuthority, 1 );
        *(RtlSubAuthoritySid(NextSid, 0)) = SECURITY_CREATOR_OWNER_RID;
        if (RtlEqualSid(Sid, NextSid)) {
            printf("Creator");
            return;
        }
    }

    {
        SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;
        RtlInitializeSid(NextSid, &SidAuthority, 1 );
        *(RtlSubAuthoritySid(NextSid, 0)) = SECURITY_DIALUP_RID;
        if (RtlEqualSid(Sid, NextSid)) {
            printf("Dialup");
            return;
        }
    }

    {
        SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;
        RtlInitializeSid(NextSid, &SidAuthority, 1 );
        *(RtlSubAuthoritySid(NextSid, 0)) = SECURITY_NETWORK_RID;
        if (RtlEqualSid(Sid, NextSid)) {
            printf("Network");
            return;
        }
    }

    {
        SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;
        RtlInitializeSid(NextSid, &SidAuthority, 1 );
        *(RtlSubAuthoritySid(NextSid, 0)) = SECURITY_BATCH_RID;
        if (RtlEqualSid(Sid, NextSid)) {
            printf("Batch");
            return;
        }
    }

    {
        SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;
        RtlInitializeSid(NextSid, &SidAuthority, 1 );
        *(RtlSubAuthoritySid(NextSid, 0)) = SECURITY_INTERACTIVE_RID;
        if (RtlEqualSid(Sid, NextSid)) {
            printf("Interactive");
            return;
        }
    }


    {
        SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;
        RtlInitializeSid(NextSid, &SidAuthority, 1 );
        *(RtlSubAuthoritySid(NextSid, 0)) = SECURITY_LOCAL_SYSTEM_RID;
        if (RtlEqualSid(Sid, NextSid)) {
            printf("Local System");
            return;
        }
    }



    sprintf(Buffer, "S-%u-", (USHORT)iSid->Revision );
    strcpy(String, Buffer);

    if (  (iSid->IdentifierAuthority.Value[0] != 0)  ||
          (iSid->IdentifierAuthority.Value[1] != 0)     ){
        sprintf(Buffer, "0x%02hx%02hx%02hx%02hx%02hx%02hx",
                    (USHORT)iSid->IdentifierAuthority.Value[0],
                    (USHORT)iSid->IdentifierAuthority.Value[1],
                    (USHORT)iSid->IdentifierAuthority.Value[2],
                    (USHORT)iSid->IdentifierAuthority.Value[3],
                    (USHORT)iSid->IdentifierAuthority.Value[4],
                    (USHORT)iSid->IdentifierAuthority.Value[5] );
        strcat(String, Buffer);
    } else {
        Tmp = (ULONG)iSid->IdentifierAuthority.Value[5]          +
              (ULONG)(iSid->IdentifierAuthority.Value[4] <<  8)  +
              (ULONG)(iSid->IdentifierAuthority.Value[3] << 16)  +
              (ULONG)(iSid->IdentifierAuthority.Value[2] << 24);
        sprintf(Buffer, "%lu", Tmp);
        strcat(String, Buffer);
    }


    for (i=0;i<iSid->SubAuthorityCount ;i++ ) {
        sprintf(Buffer, "-%lu", iSid->SubAuthority[i]);
        strcat(String, Buffer);
    }

    printf(Buffer);

    return;
}


VOID
DetermineTestsToRun(
    VOID
    )

/*++

Routine Description:

    This function determines which tests are to be run.


Arguments:

    None.

Return Value:

    None.


--*/
{

    NTSTATUS            Status;
    HANDLE              Token;

    PTOKEN_USER         User;
    PTOKEN_GROUPS       Groups;

    ULONG               ReturnLength,
                        i;



    //
    // See if we can play with auditing information
    //

    SecurityOperatorTest = EnableSecurityPrivilege();


    //
    // Open our own token
    //

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_QUERY,
                 &Token
                 );
    if (!NT_SUCCESS(Status)) {
        printf(" \n\n\n");
        printf("Tsamobj: Can't open process token to query owner.\n");
        printf("         Completion status of NtOpenProcessToken() is: 0x%lx\n", Status);
        printf("\n");
        return;
    }


    //
    // Query the user id
    //

    User   = RtlAllocateHeap( RtlProcessHeap(), 0, 1000 ); // should be plenty big
    Status = NtQueryInformationToken( Token, TokenUser, User, 1000, &ReturnLength );
    ASSERT(NT_SUCCESS(Status));

    //
    // See if the ID is one of the special IDs (e.g., local admin,
    // domain account operator, or domain admin)
    //

    SeeIfSidIsSpecial( User->User.Sid );



    //
    // Query the group ids
    //

    Groups = RtlAllocateHeap( RtlProcessHeap(), 0, 1000 ); // should be plenty big
    Status = NtQueryInformationToken( Token, TokenGroups, Groups, 1000, &ReturnLength );
    ASSERT(NT_SUCCESS(Status));

    //
    // See if any of these IDs are special IDs
    //

    for (i=0; i<Groups->GroupCount; i++) {
        SeeIfSidIsSpecial( Groups->Groups[i].Sid );
    }





    //
    // Clean up some stuff before returning
    //

    RtlFreeHeap( RtlProcessHeap(), 0, User );
    RtlFreeHeap( RtlProcessHeap(), 0, Groups );
    Status = NtClose( Token );
    ASSERT(NT_SUCCESS(Status));



    printf("\n\n\n\nPerforming:\n\n");

    printf("        Administrator Alias Test. . . . . ");
    if (AdminsAliasTest) {
        printf("Yes\n\n");
    } else {
        printf("No\n\n");
    }

    printf("        Account Operator Alias  Test  . . ");
    if (AccountOpAliasTest) {
        printf("Yes\n\n");
    } else {
        printf("No\n\n");
    }

    printf("        Security Operator  Test . . . . . ");
    if (SecurityOperatorTest) {
        printf("Yes\n\n");
    } else {
        printf("No\n\n");
    }

    printf("\n\n\n");



    return;

}


VOID
SeeIfSidIsSpecial(
    IN PSID Sid
    )

/*++

Routine Description:

    This function determines whether the passed SID is one of the special
    SIDs, such as ADMINISTRATORS alias, or DomainAccountOperator, and
    sets test flags accordingly.


Arguments:

    Sid - Pointer to the SID to check.

Return Value:

    None.


--*/
{




    if (RtlEqualSid( Sid, AdminsAliasSid )){
        AdminsAliasTest = TRUE;
    }

    if (RtlEqualSid( Sid, AccountAliasSid )){
        AccountOpAliasTest = TRUE;
    }

    return;

}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Server Object Test Suite                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


BOOLEAN
ServerTestSuite(
    PHANDLE ServerHandle,
    PHANDLE DomainHandle,
    PHANDLE BuiltinDomainHandle,
    PSID    *DomainSid
    )

{
    NTSTATUS                        NtStatus;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    BOOLEAN                         TestStatus = TRUE;
    ULONG                           CountReturned;
    SAM_ENUMERATE_HANDLE            EnumerationContext;
    PSAM_RID_ENUMERATION            EnumerationBuffer;
    PSID                            BuiltinDomainSid;
    ACCESS_MASK                     ServerAccessMask, DomainAccessMask;





    printf("\n");
    printf("\n");
    printf("  Server Object                                           Test\n");

    ///////////////////////////////////////////////////////////////////////////
    //                                                                       //
    // Connect To Server                                                     //
    //                                                                       //
    ///////////////////////////////////////////////////////////////////////////

    printf("\n");
    printf("    Connect / Disconnect. . . . . . . . . . . . . . . . .   Suite\n");

    printf("      Connect . . . . . . . . . . . . . . . . . . . . . .     ");


    ServerAccessMask = SAM_SERVER_READ | SAM_SERVER_EXECUTE;
    if (AdminsAliasTest) {
        ServerAccessMask |= SAM_SERVER_ALL_ACCESS;
    }
    if (SecurityOperatorTest) {
        ServerAccessMask |= ACCESS_SYSTEM_SECURITY;
    }

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, 0, NULL );


    NtStatus = SamConnect(
                  NULL,                     // ServerName (Local machine)
                  ServerHandle,
                  ServerAccessMask,
                  &ObjectAttributes
                  );


    if (!NT_SUCCESS(NtStatus)) {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    } else {
        printf("Succeeded\n");
    }


    if (NT_SUCCESS(NtStatus)) {

        printf("      Disconnect  . . . . . . . . . . . . . . . . . . . .     ");

        NtStatus = SamCloseHandle( (*ServerHandle) );

        if (!NT_SUCCESS(NtStatus)) {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        } else {
            printf("Succeeded\n");
        }
    }



    printf("      Re-Connect  . . . . . . . . . . . . . . . . . . . .     ");


    NtStatus = SamConnect(
                  NULL,                     // ServerName (Local machine)
                  ServerHandle,
                  ServerAccessMask,
                  &ObjectAttributes
                  );


    if (!NT_SUCCESS(NtStatus)) {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    } else {
        printf("Succeeded\n");
    }


    ///////////////////////////////////////////////////////////////////////////
    //                                                                       //
    // Lookup/Enumerate Domains Suite                                        //
    //                                                                       //
    ///////////////////////////////////////////////////////////////////////////


    printf("\n");
    printf("    Domain Lookup/Enumerate/Open  . . . . . . . . . . . .   Suite\n");



    if (NT_SUCCESS(NtStatus)) {

        printf("      Lookup Account Domain . . . . . . . . . . . . . . .     ");


        NtStatus = SamLookupDomainInSamServer(
                       (*ServerHandle),
                       &AccountDomainName,
                       DomainSid
                       );

        if (!NT_SUCCESS(NtStatus)) {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        } else {
            if ( TRUE != RtlEqualSid((*DomainSid), AccountDomainSid)) {
                printf("Failed\n");
                printf("        The SID retrieved from the policy database did not\n");
                printf("        match the SID retrieved from SAM for the account\n");
                printf("        domain.\n");
                printf("        Sid from Policy Database is: ");
                printfSid(      AccountDomainSid ); printf("\n");
                printf("        Sid from SAM is: ");
                printfSid(      (*DomainSid) ); printf("\n");
                TestStatus = FALSE;
            } else {
                printf("Succeeded\n");
            }
        }

    }






    if (NT_SUCCESS(NtStatus)) {

        printf("      Enumerate Domain  . . . . . . . . . . . . . . . . .     ");


        EnumerationContext = 0;
        EnumerationBuffer = NULL;
        NtStatus = SamEnumerateDomainsInSamServer(
                       (*ServerHandle),
                       &EnumerationContext,
                       (PVOID *)&EnumerationBuffer,
                       1024,                        // PreferedMaximumLength
                       &CountReturned
                       );

        if (!NT_SUCCESS(NtStatus)) {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        } else {

            if (CountReturned == 0) {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                printf("        CountReturned is: 0x%lx\n", CountReturned);
                printf("        EnumerationContext is: 0x%lx\n", EnumerationContext);
                printf("        EnumerationBuffer Address is: 0x%lx\n", (ULONG)EnumerationBuffer);
                TestStatus = FALSE;

            } else {
                printf("Succeeded\n");
            }

            SamFreeMemory( EnumerationBuffer );
        }

    }






    if (NT_SUCCESS(NtStatus)) {

        printf("      Open Account Domain . . . . . . . . . . . . . . . .     ");

        if (NT_SUCCESS(NtStatus)) {

            DomainAccessMask = DOMAIN_READ | DOMAIN_EXECUTE;
            if (AccountOpAliasTest) {
                DomainAccessMask |= DOMAIN_READ | DOMAIN_WRITE | DOMAIN_EXECUTE;
            }
            if (AdminsAliasTest) {
                DomainAccessMask |= DOMAIN_ALL_ACCESS;
            }
            if (SecurityOperatorTest) {
                DomainAccessMask |= ACCESS_SYSTEM_SECURITY;
            }
            NtStatus = SamOpenDomain(
                           (*ServerHandle),
                           DomainAccessMask,
                           *DomainSid,
                           DomainHandle
                           );

            if (!NT_SUCCESS(NtStatus)) {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            } else {
                printf("Succeeded\n");
            }
        }

    }

    if (NT_SUCCESS(NtStatus)) {

        printf("      Open Builtin Domain . . . . . . . . . . . . . . . .     ");

        NtStatus = SamLookupDomainInSamServer(
                       (*ServerHandle),
                       &BuiltinDomainName,
                       &BuiltinDomainSid
                       );

        if (NT_SUCCESS(NtStatus)) {

            DomainAccessMask = DOMAIN_READ | DOMAIN_EXECUTE;
            if (AccountOpAliasTest) {
                DomainAccessMask |= DOMAIN_READ | DOMAIN_WRITE | DOMAIN_EXECUTE;
            }
            if (AdminsAliasTest) {
                DomainAccessMask |= (DOMAIN_EXECUTE | DOMAIN_READ |
                                     DOMAIN_READ_OTHER_PARAMETERS |
                                     DOMAIN_ADMINISTER_SERVER     |
                                     DOMAIN_CREATE_ALIAS);
            }
//            if (SecurityOperatorTest) {
//                DomainAccessMask |= ACCESS_SYSTEM_SECURITY;
//            }
            NtStatus = SamOpenDomain(
                           (*ServerHandle),
                           DomainAccessMask,
                           BuiltinDomainSid,
                           BuiltinDomainHandle
                           );

            if (!NT_SUCCESS(NtStatus)) {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            } else {
                printf("Succeeded\n");
            }
        }

    }

    return(TestStatus);


}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Security Manipulation Test Suite                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


BOOLEAN
SecurityTestSuite(
    HANDLE ServerHandle,
    HANDLE DomainHandle,
    ULONG Pass
    )
{

    BOOLEAN                 TestStatus = TRUE;
    NTSTATUS                NtStatus;

    PSECURITY_DESCRIPTOR    OriginalServerSD,
                            OriginalDomainSD,
                            OriginalUserSD,
                            OriginalGroupSD,
                            SD1;

    SECURITY_INFORMATION    SI1;
    PVOID TmpPointer1;

    SECURITY_DESCRIPTOR     SD1_Body;

    HANDLE                  GroupHandle,
                            UserHandle;




    printf("\n");
    printf("\n");
    printf("\n");

    if (Pass == 1) {

        printf("  Security Manipulation (Pass #1)                         Test\n");

        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Query Suite                                                           //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("\n");
        printf("    Query Security  . . . . . . . . . . . . . . . . . . .   Suite\n");


        //
        // Get Server's original SD
        //


        SI1 = 0;
        if (AdminsAliasTest) {
            SI1 |= OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                   DACL_SECURITY_INFORMATION;
        }
        if (SecurityOperatorTest) {
            SI1 |= SACL_SECURITY_INFORMATION;
        }
        if (SI1 != 0) {
            printf("      Query Server Security Descriptor  . . . . . . . . . .     ");
            SD1 = NULL;
            NtStatus = SamQuerySecurityObject(
                           ServerHandle,
                           SI1,
                           &SD1
                           );
            if (NT_SUCCESS(NtStatus)) {

                TestStatus = CheckReturnedSD( SI1, SD1, TRUE );

                //
                // Normally we would do a "SamFreeMemory( SD1 )" here.
                // However, we want to save this SD for future reference
                // and use.
                //

                OriginalServerSD = SD1;

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }
        }





        //
        // Get domain's original SD
        //


        SI1 = 0;
        if (AdminsAliasTest) {
            SI1 |= OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                   DACL_SECURITY_INFORMATION;
        }
        if (SecurityOperatorTest) {
            SI1 |= SACL_SECURITY_INFORMATION;
        }
        if (SI1 != 0) {
            printf("      Query Domain Security Descriptor  . . . . . . . . . .     ");
            SD1 = NULL;
            NtStatus = SamQuerySecurityObject(
                           DomainHandle,
                           SI1,
                           &SD1
                           );
            if (NT_SUCCESS(NtStatus)) {

                TestStatus = CheckReturnedSD( SI1, SD1, TRUE );

                //
                // Normally we would do a "SamFreeMemory( SD1 )" here.
                // However, we want to save this SD for future reference
                // and use.
                //

                OriginalDomainSD = SD1;

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }
        }






        //
        // Make sure the wrapper doesn't choke on a non-null pointer being passed
        // (assuming we have allocated memory).
        //


        SI1 = 0;
        if (AdminsAliasTest) {
            SI1 |= OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                   DACL_SECURITY_INFORMATION;
        }
        if (SecurityOperatorTest) {
            SI1 |= SACL_SECURITY_INFORMATION;
        }
        if (SI1 != 0) {
            printf("      Query Passing Non-null return buffer  . . . . . . . .     ");
            SD1 = RtlAllocateHeap( RtlProcessHeap(), 0, 1000 ); ASSERT(SD1 != NULL);
            TmpPointer1 = SD1;
            NtStatus = SamQuerySecurityObject(
                           DomainHandle,
                           SI1,
                           &SD1
                           );
            if (NT_SUCCESS(NtStatus)) {
                if (SD1 != TmpPointer1) {

                    TestStatus = CheckReturnedSD( SI1, SD1, TRUE );
                    if (TestStatus) {
                        SamFreeMemory( SD1 );
                    }


                } else {
                    printf("Failed\n");
                    printf("        Passed buffer address used on return.\n");
                    printf("        RPC should have allocated another buffer.\n");
                    TestStatus = FALSE;
                }
            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }


            RtlFreeHeap( RtlProcessHeap(), 0, TmpPointer1 );

        }






        //
        // Make sure we can query nothing
        //

        printf("      Query Nothing . . . . . . . . . . . . . . . . . . . .     ");

        SI1 = 0;
        SD1 = NULL;
        NtStatus = SamQuerySecurityObject(
                       DomainHandle,
                       SI1,
                       &SD1
                       );
        if (NT_SUCCESS(NtStatus)) {

            TestStatus = CheckReturnedSD( SI1, SD1, TRUE );
            if (TestStatus) {
                SamFreeMemory( SD1 );
            }

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }





        //
        // Query owner
        //


        if (AdminsAliasTest) {
            printf("      Query Owner (Server Object) . . . . . . . . . . . . .     ");
            SI1 = OWNER_SECURITY_INFORMATION;
            SD1 = NULL;
            NtStatus = SamQuerySecurityObject(
                           ServerHandle,
                           SI1,
                           &SD1
                           );
            if (NT_SUCCESS(NtStatus)) {

                TestStatus = CheckReturnedSD( SI1, SD1, TRUE );
                if (TestStatus) {
                    SamFreeMemory( SD1 );
                }

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }
        }




        if (AdminsAliasTest) {
            printf("      Query Owner (Domain Object) . . . . . . . . . . . . .     ");
            SI1 = OWNER_SECURITY_INFORMATION;
            SD1 = NULL;
            NtStatus = SamQuerySecurityObject(
                           DomainHandle,
                           SI1,
                           &SD1
                           );
            if (NT_SUCCESS(NtStatus)) {

                TestStatus = CheckReturnedSD( SI1, SD1, TRUE );
                if (TestStatus) {
                    SamFreeMemory( SD1 );
                }

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }
        }





        if (AdminsAliasTest) {

            //
            // Query Group
            //

            printf("      Query Group . . . . . . . . . . . . . . . . . . . . .     ");

            SI1 = GROUP_SECURITY_INFORMATION;
            SD1 = NULL;
            NtStatus = SamQuerySecurityObject(
                           DomainHandle,
                           SI1,
                           &SD1
                           );
            if (NT_SUCCESS(NtStatus)) {

                TestStatus = CheckReturnedSD( SI1, SD1, TRUE );
                if (TestStatus) {
                    SamFreeMemory( SD1 );
                }

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }





            //
            // Query Dacl
            //

            printf("      Query DACL  . . . . . . . . . . . . . . . . . . . . .     ");

            SI1 = DACL_SECURITY_INFORMATION;
            SD1 = NULL;
            NtStatus = SamQuerySecurityObject(
                           DomainHandle,
                           SI1,
                           &SD1
                           );
            if (NT_SUCCESS(NtStatus)) {

                TestStatus = CheckReturnedSD( SI1, SD1, TRUE );
                if (TestStatus) {
                    SamFreeMemory( SD1 );
                }

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }





            //
            // Query Sacl
            //

            printf("      Query SACL  . . . . . . . . . . . . . . . . . . . . .     ");

            SI1 = SACL_SECURITY_INFORMATION;
            SD1 = NULL;
            NtStatus = SamQuerySecurityObject(
                           DomainHandle,
                           SI1,
                           &SD1
                           );
            if (NT_SUCCESS(NtStatus)) {

                TestStatus = CheckReturnedSD( SI1, SD1, TRUE );
                if (TestStatus) {
                    SamFreeMemory( SD1 );
                }

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }

        }  // end_if (AdminsAliasTest)





        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Set Suite                                                             //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("\n");
        printf("    Set Security  . . . . . . . . . . . . . . . . . . . .   Suite\n");


        //
        // Make sure we can set nothing
        //

        printf("      Set Nothing . . . . . . . . . . . . . . . . . . . . .     ");

        SI1 = 0;
        SD1 = &SD1_Body;
        NtStatus = RtlCreateSecurityDescriptor( SD1, SECURITY_DESCRIPTOR_REVISION1 );
        ASSERT( NT_SUCCESS(NtStatus) );
        NtStatus = SamSetSecurityObject(
                       DomainHandle,
                       SI1,     // <------ This is invalid
                       SD1
                       );
        if (NtStatus == STATUS_INVALID_PARAMETER) {

            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }






        //
        // set something not passed
        //

        printf("      Set something not passed. . . . . . . . . . . . . . .     ");

        SI1 = GROUP_SECURITY_INFORMATION;
        SD1 = &SD1_Body;
        NtStatus = RtlCreateSecurityDescriptor( SD1, SECURITY_DESCRIPTOR_REVISION1 );
        ASSERT( NT_SUCCESS(NtStatus) );
        NtStatus = SamSetSecurityObject(
                       DomainHandle,
                       SI1,
                       SD1
                       );
        if (NtStatus == STATUS_BAD_DESCRIPTOR_FORMAT) {

            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }






        //
        // set a non-existant DACL
        //

        if (AdminsAliasTest) {
            printf("      Set non-existant DACL (Server object) . . . . . . . .     ");

            SI1 = DACL_SECURITY_INFORMATION;
            SD1 = &SD1_Body;
            NtStatus = RtlCreateSecurityDescriptor( SD1, SECURITY_DESCRIPTOR_REVISION1 );
            SD1_Body.Control = SE_DACL_PRESENT;
            ASSERT( NT_SUCCESS(NtStatus) );
            NtStatus = SamSetSecurityObject(
                           ServerHandle,
                           SI1,
                           SD1
                           );
            if (NT_SUCCESS(NtStatus)) {

                printf("Succeeded\n");

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }
        }



        if (AdminsAliasTest) {
            printf("      Set non-existant DACL (Domain Object) . . . . . . . .     ");

            SI1 = DACL_SECURITY_INFORMATION;
            SD1 = &SD1_Body;
            NtStatus = RtlCreateSecurityDescriptor( SD1, SECURITY_DESCRIPTOR_REVISION1 );
            SD1_Body.Control = SE_DACL_PRESENT;
            ASSERT( NT_SUCCESS(NtStatus) );
            NtStatus = SamSetSecurityObject(
                           DomainHandle,
                           SI1,
                           SD1
                           );
            if (NT_SUCCESS(NtStatus)) {

                printf("Succeeded\n");

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }
        }





        //
        // set original DACL (From original SD)
        //

        if (AdminsAliasTest) {

            printf("      Set original DACL (Server Object) . . . . . . . . . .     ");

            SI1 = DACL_SECURITY_INFORMATION;
            SD1 = OriginalServerSD;
            NtStatus = SamSetSecurityObject(
                           ServerHandle,
                           SI1,
                           SD1
                           );
            if (NT_SUCCESS(NtStatus)) {

                printf("Succeeded\n");

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }
        }



        if (AdminsAliasTest) {

            printf("      Set original DACL (Domain Object) . . . . . . . . . .     ");

            SI1 = DACL_SECURITY_INFORMATION;
            SD1 = OriginalDomainSD;
            NtStatus = SamSetSecurityObject(
                           DomainHandle,
                           SI1,
                           SD1
                           );
            if (NT_SUCCESS(NtStatus)) {

                printf("Succeeded\n");

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }
        }





        if (AdminsAliasTest) {

            //
            // set a non-existant SACL
            //

            printf("      Set non-existant SACL . . . . . . . . . . . . . . . .     ");

            SI1 = SACL_SECURITY_INFORMATION;
            SD1 = &SD1_Body;
            NtStatus = RtlCreateSecurityDescriptor( SD1, SECURITY_DESCRIPTOR_REVISION1 );
            SD1_Body.Control = SE_SACL_PRESENT;
            ASSERT( NT_SUCCESS(NtStatus) );
            NtStatus = SamSetSecurityObject(
                           DomainHandle,
                           SI1,
                           SD1
                           );
            if (NT_SUCCESS(NtStatus)) {

                printf("Succeeded\n");

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }






            //
            // set original SACL (From original SD)
            //

            printf("      Set original SACL . . . . . . . . . . . . . . . . . .     ");

            SI1 = SACL_SECURITY_INFORMATION;
            SD1 = OriginalDomainSD;
            NtStatus = SamSetSecurityObject(
                           DomainHandle,
                           SI1,
                           SD1
                           );
            if (NT_SUCCESS(NtStatus)) {

                printf("Succeeded\n");

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }





            //
            // set a owner to null
            //

            printf("      Set null Owner  . . . . . . . . . . . . . . . . . . .     ");

            SI1 = OWNER_SECURITY_INFORMATION;
            SD1 = &SD1_Body;
            NtStatus = RtlCreateSecurityDescriptor( SD1, SECURITY_DESCRIPTOR_REVISION1 );
            SD1_Body.Owner = NULL;
            ASSERT( NT_SUCCESS(NtStatus) );
            NtStatus = SamSetSecurityObject(
                           DomainHandle,
                           SI1,
                           SD1
                           );
            if (NtStatus = STATUS_BAD_DESCRIPTOR_FORMAT) {

                printf("Succeeded\n");

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }





            //
            // set owner to invalid value
            //

            printf("      Set owner to invalid value  . . . . . . . . . . . . .     ");

            SI1 = OWNER_SECURITY_INFORMATION;
            SD1 = &SD1_Body;
            NtStatus = RtlCreateSecurityDescriptor( SD1, SECURITY_DESCRIPTOR_REVISION1 );
            SD1_Body.Owner = WorldSid;
            ASSERT( NT_SUCCESS(NtStatus) );
            NtStatus = SamSetSecurityObject(
                           DomainHandle,
                           SI1,
                           SD1
                           );
            if (NtStatus = STATUS_INVALID_OWNER) {

                printf("Succeeded\n");

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }




            //
            // set a owner to valid value
            //

            printf("      Set owner to valid value  . . . . . . . . . . . . . .     ");

            printf("Untested\n");





            //
            // set group to null
            //

            printf("      Set null Group  . . . . . . . . . . . . . . . . . . .     ");

            SI1 = GROUP_SECURITY_INFORMATION;
            SD1 = &SD1_Body;
            NtStatus = RtlCreateSecurityDescriptor( SD1, SECURITY_DESCRIPTOR_REVISION1 );
            SD1_Body.Group = NULL;
            ASSERT( NT_SUCCESS(NtStatus) );
            NtStatus = SamSetSecurityObject(
                           DomainHandle,
                           SI1,
                           SD1
                           );
            if (NtStatus = STATUS_BAD_DESCRIPTOR_FORMAT) {

                printf("Succeeded\n");

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }






            //
            // set Group to valid value
            //

            printf("      Set Group to valid value  . . . . . . . . . . . . . .     ");

            SI1 = GROUP_SECURITY_INFORMATION;
            SD1 = &SD1_Body;
            NtStatus = RtlCreateSecurityDescriptor( SD1, SECURITY_DESCRIPTOR_REVISION1 );
            SD1_Body.Group = WorldSid;
            ASSERT( NT_SUCCESS(NtStatus) );
            NtStatus = SamSetSecurityObject(
                           DomainHandle,
                           SI1,
                           SD1
                           );
            if (NT_SUCCESS(NtStatus)) {

                printf("Succeeded\n");

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }





            //
            // set Group back to original value
            //

            printf("      Set Group to original value . . . . . . . . . . . . .     ");

            SI1 = GROUP_SECURITY_INFORMATION;
            SD1 = OriginalDomainSD;
            NtStatus = SamSetSecurityObject(
                           DomainHandle,
                           SI1,
                           SD1
                           );
            if (NT_SUCCESS(NtStatus)) {

                printf("Succeeded\n");

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }
        }




    }   // end Pass1


    if (Pass == 2) {

        ACCESS_MASK         AccessMask;
        PSID_NAME_USE       LookedUpUses;
        PULONG              LookedUpRids;
        UNICODE_STRING      AccountNames[10];
        STRING              AccountNameAnsi;


        //
        // This pass depends upon user and group accounts established in pass #1
        //





        if (AdminsAliasTest) {


            printf("  Security Manipulation (Pass #2)                         Test\n");

            ///////////////////////////////////////////////////////////////////////////
            //                                                                       //
            // Query Suite                                                           //
            //                                                                       //
            ///////////////////////////////////////////////////////////////////////////

            printf("\n");
            printf("    Query Security (User Object). . . . . . . . . . . . .   Suite\n");


            AccessMask = READ_CONTROL;
            if (SecurityOperatorTest) {
                AccessMask |= ACCESS_SYSTEM_SECURITY;
            }

            //
            // Open the user created in pass #1
            //

            RtlInitString( &AccountNameAnsi, USER_NAME1 );
            NtStatus = RtlAnsiStringToUnicodeString( &AccountNames[0], &AccountNameAnsi, TRUE );
            TST_SUCCESS_ASSERT(NtStatus);

            NtStatus = SamLookupNamesInDomain(
                           DomainHandle,
                           1,
                           &AccountNames[0],
                           &LookedUpRids,
                           &LookedUpUses
                           );
            RtlFreeUnicodeString( &AccountNames[0] );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(LookedUpUses[0] == SidTypeUser);
            NtStatus = SamOpenUser(
                           DomainHandle,
                           AccessMask,
                           LookedUpRids[0],
                           &UserHandle);
            SamFreeMemory( LookedUpUses ); SamFreeMemory( LookedUpRids );
            if (!NT_SUCCESS(NtStatus)) {
                printf("Failed to open user account created in pass #1\n");
            }
            TST_SUCCESS_ASSERT(NT_SUCCESS(NtStatus));



            //
            // Get user's original SD
            //

            SI1 |= OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                   DACL_SECURITY_INFORMATION;
            if (SecurityOperatorTest) {
                SI1 |= SACL_SECURITY_INFORMATION;
            }

            printf("      Query User Security Descriptor  . . . . . . . . . . .     ");
            SD1 = NULL;
            NtStatus = SamQuerySecurityObject(
                           UserHandle,
                           SI1,
                           &SD1
                           );
            if (NT_SUCCESS(NtStatus)) {

                TestStatus = CheckReturnedSD( SI1, SD1, TRUE );

                //
                // Normally we would do a "SamFreeMemory( SD1 )" here.
                // However, we want to save this SD for future reference
                // and use.
                //

                OriginalUserSD = SD1;

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }





            NtStatus = SamCloseHandle( UserHandle );
            TST_SUCCESS_ASSERT( UserHandle );



            ///////////////////////////////////////////////////////////////////////////
            //                                                                       //
            // Set Suite                                                             //
            //                                                                       //
            ///////////////////////////////////////////////////////////////////////////

            printf("\n");
            printf("    Set Security  (User Object) . . . . . . . . . . . . .   Suite\n");

            AccessMask = WRITE_DAC | WRITE_OWNER;
            if (SecurityOperatorTest) {
                AccessMask |= ACCESS_SYSTEM_SECURITY;
            }

            //
            // Open the user created in pass #1
            //

            RtlInitString( &AccountNameAnsi, USER_NAME1 );
            NtStatus = RtlAnsiStringToUnicodeString( &AccountNames[0], &AccountNameAnsi, TRUE );
            TST_SUCCESS_ASSERT(NtStatus);

            NtStatus = SamLookupNamesInDomain(
                           DomainHandle,
                           1,
                           &AccountNames[0],
                           &LookedUpRids,
                           &LookedUpUses
                           );
            RtlFreeUnicodeString( &AccountNames[0] );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(LookedUpUses[0] == SidTypeUser);
            NtStatus = SamOpenUser(
                           DomainHandle,
                           AccessMask,
                           LookedUpRids[0],
                           &UserHandle);
            SamFreeMemory( LookedUpUses ); SamFreeMemory( LookedUpRids );
            if (!NT_SUCCESS(NtStatus)) {
                printf("Failed to open user account created in pass #1\n");
            }
            TST_SUCCESS_ASSERT(NT_SUCCESS(NtStatus));


            //
            // Make sure we can set nothing
            //

            printf("      Set Nothing . . . . . . . . . . . . . . . . . . . . .     ");

            SI1 = 0;
            SD1 = &SD1_Body;
            NtStatus = RtlCreateSecurityDescriptor( SD1, SECURITY_DESCRIPTOR_REVISION1 );
            ASSERT( NT_SUCCESS(NtStatus) );
            NtStatus = SamSetSecurityObject(
                           UserHandle,
                           SI1,     // <------ This is invalid
                           SD1
                           );
            if (NtStatus == STATUS_INVALID_PARAMETER) {

                printf("Succeeded\n");

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }






            //
            // set something not passed
            //

            printf("      Set something not passed. . . . . . . . . . . . . . .     ");

            SI1 = GROUP_SECURITY_INFORMATION;
            SD1 = &SD1_Body;
            NtStatus = RtlCreateSecurityDescriptor( SD1, SECURITY_DESCRIPTOR_REVISION1 );
            ASSERT( NT_SUCCESS(NtStatus) );
            NtStatus = SamSetSecurityObject(
                           UserHandle,
                           SI1,
                           SD1
                           );
            if (NtStatus == STATUS_BAD_DESCRIPTOR_FORMAT) {

                printf("Succeeded\n");

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }








            printf("      Set non-existant DACL . . . . . . . . . . . . . . . .     ");

            SI1 = DACL_SECURITY_INFORMATION;
            SD1 = &SD1_Body;
            NtStatus = RtlCreateSecurityDescriptor( SD1, SECURITY_DESCRIPTOR_REVISION1 );
            SD1_Body.Control = SE_DACL_PRESENT;
            ASSERT( NT_SUCCESS(NtStatus) );
            NtStatus = SamSetSecurityObject(
                           UserHandle,
                           SI1,
                           SD1
                           );
            if (NT_SUCCESS(NtStatus)) {

                printf("Succeeded\n");

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }






            //
            // set original DACL (From original SD)
            //


            printf("      Set original DACL . . . . . . . . . . . . . . . . . .     ");

            SI1 = DACL_SECURITY_INFORMATION;
            SD1 = OriginalUserSD;
            NtStatus = SamSetSecurityObject(
                           UserHandle,
                           SI1,
                           SD1
                           );
            if (NT_SUCCESS(NtStatus)) {

                printf("Succeeded\n");

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }







            NtStatus = SamCloseHandle( UserHandle );
            TST_SUCCESS_ASSERT( UserHandle );



        }

        DBG_UNREFERENCED_LOCAL_VARIABLE( GroupHandle );
        DBG_UNREFERENCED_LOCAL_VARIABLE( OriginalGroupSD );
    }





    return TestStatus;
}


BOOLEAN
CheckReturnedSD(
    IN SECURITY_INFORMATION SI,
    IN PSECURITY_DESCRIPTOR SD,
    IN BOOLEAN              PrintTestSuccess
    )


{
    NTSTATUS NtStatus;

    BOOLEAN  Failed = FALSE,
             IgnoreBoolean,
             AclPresent,
             TestStatus = TRUE;

    PSID     SID;
    PACL     ACL;



    //
    // Check a returned security descriptor agains the information requested.
    //

    if (SD == NULL) {
        TestStatus = FALSE;
        if (PrintTestSuccess) {
            printf("Failed\n");
            Failed = TRUE;
            printf("        The SecurityDescriptor return address was not properly\n");
            printf("        set.\n");
        }
    }


    if (TestStatus) {

        //
        // Check owner
        //

        NtStatus = RtlGetOwnerSecurityDescriptor ( SD, &SID, &IgnoreBoolean);
        ASSERT(NT_SUCCESS(NtStatus));
        if (SI & OWNER_SECURITY_INFORMATION) {
            if (SID == NULL) {
                if (PrintTestSuccess) {
                    if (!Failed) {
                        printf("Failed\n");
                        printf("        Security descriptor address is 0x%lx\n", SD );
                        Failed = TRUE;
                    }
                    printf("        An owner was requested but the owner field of the\n");
                    printf("        security descriptor is not set.\n");
                    TestStatus = FALSE;

                }
            }
        } else {    // Owner not specified
            if (SID != NULL) {
                if (PrintTestSuccess) {
                    if (!Failed) {
                        printf("Failed\n");
                        printf("        Security descriptor address is 0x%lx\n", SD );
                        Failed = TRUE;
                    }
                    printf("        An owner was not requested but the owner field of the\n");
                    printf("        security descriptor is set.\n");
                    TestStatus = FALSE;
                }
            }
        }




        //
        // Check group
        //

        NtStatus = RtlGetGroupSecurityDescriptor ( SD, &SID, &IgnoreBoolean);
        ASSERT(NT_SUCCESS(NtStatus));
        if (SI & GROUP_SECURITY_INFORMATION) {
            if (SID == NULL) {
                if (PrintTestSuccess) {
                    if (!Failed) {
                        printf("Failed\n");
                        printf("        Security descriptor address is 0x%lx\n", SD );
                        Failed = TRUE;
                    }
                    printf("        A group was requested but the group field of the\n");
                    printf("        security descriptor is not set.\n");
                    TestStatus = FALSE;

                }
            }
        } else {    // Group not specified
            if (SID != NULL) {
                if (PrintTestSuccess) {
                    if (!Failed) {
                        printf("Failed\n");
                        printf("        Security descriptor address is 0x%lx\n", SD );
                        Failed = TRUE;
                    }
                    printf("        A group was not requested but the group field of the\n");
                    printf("        security descriptor is set.\n");
                    TestStatus = FALSE;
                }
            }
        }




        //
        // Check sacl
        //

        NtStatus = RtlGetSaclSecurityDescriptor ( SD, &AclPresent, &ACL, &IgnoreBoolean);
        ASSERT(NT_SUCCESS(NtStatus));
        if (SI & SACL_SECURITY_INFORMATION) {
            if (!AclPresent) {
                if (PrintTestSuccess) {
                    if (!Failed) {
                        printf("Failed\n");
                        printf("        Security descriptor address is 0x%lx\n", SD );
                        Failed = TRUE;
                    }
                    printf("        An SACL was requested but the SaclPresent flag\n");
                    printf("        of the security descriptor is not set.\n");
                    TestStatus = FALSE;

                }
            }
        } else {    // sacl not specified
            if (AclPresent) {
                if (PrintTestSuccess) {
                    if (!Failed) {
                        printf("Failed\n");
                        printf("        Security descriptor address is 0x%lx\n", SD );
                        Failed = TRUE;
                    }
                    printf("        An SACL was not requested but the SaclPresent flag\n");
                    printf("        of the security descriptor is set.\n");
                    TestStatus = FALSE;
                }
            }
        }





        //
        // Check Dacl
        //

        NtStatus = RtlGetDaclSecurityDescriptor ( SD, &AclPresent, &ACL, &IgnoreBoolean);
        ASSERT(NT_SUCCESS(NtStatus));
        if (SI & DACL_SECURITY_INFORMATION) {
            if (!AclPresent) {
                if (PrintTestSuccess) {
                    if (!Failed) {
                        printf("Failed\n");
                        printf("        Security descriptor address is 0x%lx\n", SD );
                        Failed = TRUE;
                    }
                    printf("        A DACL was requested but the DaclPresent flag\n");
                    printf("        of the security descriptor is not set.\n");
                    TestStatus = FALSE;

                }
            }
        } else {    // Dacl not specified
            if (AclPresent) {
                if (PrintTestSuccess) {
                    if (!Failed) {
                        printf("Failed\n");
                        printf("        Security descriptor address is 0x%lx\n", SD );
                        Failed = TRUE;
                    }
                    printf("        A DACL was not requested but the DaclPresent flag\n");
                    printf("        of the security descriptor is set.\n");
                    TestStatus = FALSE;
                }
            }
        }





    }




    if (PrintTestSuccess) {
        if (TestStatus) {
            printf("Succeeded\n");
        }
    }



    return(TestStatus);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Domain Object Test Suite                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


BOOLEAN
DomainTestSuite(
    HANDLE DomainHandle
    )
{

    BOOLEAN  TestStatus = TRUE;
    NTSTATUS NtStatus, IgnoreStatus;
    PVOID    Buffer, Buffer1, Buffer2;
    CHAR     UnusedBuffer[20];
    UNICODE_STRING      AccountName;
    STRING              AccountNameAnsi;
    HANDLE GroupHandle = NULL;
    HANDLE AliasHandle = NULL;
    HANDLE UserHandle = NULL;
    HANDLE ValidUserHandle = NULL;
    ULONG  GroupRid, AliasRid, UserRid, SavedGroupRid, SavedAliasRid, AccountCount, i;
    SAM_ENUMERATE_HANDLE EnumerationContext;
    ULONG   CountReturned;
    USHORT NameLength;
    PUNICODE_STRING  LookedUpNames;
    PSID_NAME_USE    LookedUpUses;
    PULONG           LookedUpRids;


    printf("\n");
    printf("\n");
    printf("\n");
    printf("  Domain                                                  Test\n");

    ///////////////////////////////////////////////////////////////////////////
    //                                                                       //
    // Query Suite                                                           //
    //                                                                       //
    ///////////////////////////////////////////////////////////////////////////

    printf("\n");
    printf("    Query Information . . . . . . . . . . . . . . . . . .   Suite\n");


    //
    // Make sure the wrapper doesn't choke on a non-null pointer being passed
    // (assuming we have allocated memory).
    //

    printf("      Query Buffer Allocation Test  . . . . . . . . . . . .     ");

    Buffer = &UnusedBuffer[0];
    NtStatus = SamQueryInformationDomain(
                   DomainHandle,
                   DomainStateInformation,
                   &Buffer
                   );
    if (NT_SUCCESS(NtStatus)) {
        if (Buffer != &UnusedBuffer[0]) {
            if (Buffer != NULL) {
                printf("Succeeded\n");
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Passed buffer address used on return.\n");
            printf("        RPC should have allocated another buffer.\n");
            TestStatus = FALSE;
        }
    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    }


    //
    // Query all the fixed length info levels
    //  Query - Password, Logoff, ServerRole, DomainState, ModifiedCount, LockoutInfo
    //

    printf("      Query DomainState . . . . . . . . . . . . . . . . . .     ");

    NtStatus = SamQueryInformationDomain(
                   DomainHandle,
                   DomainStateInformation,
                   &Buffer
                   );
    if (NT_SUCCESS(NtStatus)) {
        if (Buffer != NULL) {
                printf("Succeeded\n");
                SamFreeMemory( Buffer );
        } else {
            printf("Failed\n");
            printf("        Buffer address not set on return.\n");
            printf("        RPC should have allocated a buffer.\n");
            TestStatus = FALSE;
        }
    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;

    }


    printf("      Query ServerRole  . . . . . . . . . . . . . . . . . .     ");
    NtStatus = SamQueryInformationDomain(
                   DomainHandle,
                   DomainServerRoleInformation,
                   &Buffer
                   );
    if (NT_SUCCESS(NtStatus)) {
        if (Buffer != NULL) {
                printf("Succeeded\n");
                SamFreeMemory( Buffer );
        } else {
            printf("Failed\n");
            printf("        Buffer address not set on return.\n");
            printf("        RPC should have allocated a buffer.\n");
            TestStatus = FALSE;
        }
    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;

    }


    printf("      Query Password Information  . . . . . . . . . . . . .     ");
    NtStatus = SamQueryInformationDomain(
                   DomainHandle,
                   DomainPasswordInformation,
                   &Buffer
                   );
    if (NT_SUCCESS(NtStatus)) {
        if (Buffer != NULL) {
                printf("Succeeded\n");
                SamFreeMemory( Buffer );
        } else {
            printf("Failed\n");
            printf("        Buffer address not set on return.\n");
            printf("        RPC should have allocated a buffer.\n");
            TestStatus = FALSE;
        }
    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;

    }


    printf("      Query Logoff Information  . . . . . . . . . . . . . .     ");
    NtStatus = SamQueryInformationDomain(
                   DomainHandle,
                   DomainLogoffInformation,
                   &Buffer
                   );
    if (NT_SUCCESS(NtStatus)) {
        if (Buffer != NULL) {
                printf("Succeeded\n");
                SamFreeMemory( Buffer );
        } else {
            printf("Failed\n");
            printf("        Buffer address not set on return.\n");
            printf("        RPC should have allocated a buffer.\n");
            TestStatus = FALSE;
        }
    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;

    }


    printf("      Query Modified  . . . . . . . . . . . . . . . . . . .     ");
    NtStatus = SamQueryInformationDomain(
                   DomainHandle,
                   DomainModifiedInformation,
                   &Buffer
                   );
    if (NT_SUCCESS(NtStatus)) {
        if (Buffer != NULL) {
                printf("Succeeded\n");
                SamFreeMemory( Buffer );
        } else {
            printf("Failed\n");
            printf("        Buffer address not set on return.\n");
            printf("        RPC should have allocated a buffer.\n");
            TestStatus = FALSE;
        }
    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;

    }


    printf("      Query Lockout . . . . . . . . . . . . . . . . . . . .     ");
    NtStatus = SamQueryInformationDomain(
                   DomainHandle,
                   DomainLockoutInformation,
                   &Buffer
                   );
    if (NT_SUCCESS(NtStatus)) {
        if (Buffer != NULL) {
                printf("Succeeded\n");
                SamFreeMemory( Buffer );
        } else {
            printf("Failed\n");
            printf("        Buffer address not set on return.\n");
            printf("        RPC should have allocated a buffer.\n");
            TestStatus = FALSE;
        }
    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;

    }





    //
    // Query the name of the domain ...
    //

    printf("      Query Domain Name . . . . . . . . . . . . . . . . . .     ");

    Buffer = NULL;
    NtStatus = SamQueryInformationDomain(
                   DomainHandle,
                   DomainNameInformation,
                   &Buffer
                   );
    if (NT_SUCCESS(NtStatus)) {
        if (Buffer != NULL) {
            if ( (((DOMAIN_NAME_INFORMATION *)Buffer)->DomainName.MaximumLength > 0) &&
                 (((DOMAIN_NAME_INFORMATION *)Buffer)->DomainName.Buffer != NULL) ) {

                printf("Succeeded\n");

            } else {
                printf("Failed\n");
                printf("        String body returned and allocated,\n");
                printf("        but character buffer pointer is NULL.\n");
                TestStatus = FALSE;
            }
            SamFreeMemory( Buffer );
        } else {
            printf("Failed\n");
            printf("        Buffer address not set on return.\n");
            printf("        RPC should have allocated a buffer.\n");
            TestStatus = FALSE;
        }
    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    }



    //
    // Query whatever is in the OEM Information field ...
    //

    printf("      Query OEM Information . . . . . . . . . . . . . . . .     ");

    Buffer = NULL;
    NtStatus = SamQueryInformationDomain(
                   DomainHandle,
                   DomainOemInformation,
                   &Buffer
                   );
    if (NT_SUCCESS(NtStatus)) {
        if (Buffer != NULL) {
            if ( (((DOMAIN_OEM_INFORMATION *)Buffer)->OemInformation.MaximumLength >= 0) &&
                 (((DOMAIN_OEM_INFORMATION *)Buffer)->OemInformation.Buffer != NULL) ) {

                printf("Succeeded\n");

            } else {
                printf("Failed\n");
                printf("        String body returned and allocated,\n");
                printf("        but character buffer pointer is NULL.\n");
                TestStatus = FALSE;
            }
            SamFreeMemory( Buffer );
        } else {
            printf("Failed\n");
            printf("        Buffer address not set on return.\n");
            printf("        RPC should have allocated a buffer.\n");
            TestStatus = FALSE;
        }
    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    }



    //
    // Query whatever is in the Replication Information field ...
    //

    printf("      Query Replication Information . . . . . . . . . . . .     ");

    Buffer = NULL;
    NtStatus = SamQueryInformationDomain(
                   DomainHandle,
                   DomainReplicationInformation,
                   &Buffer
                   );
    if (NT_SUCCESS(NtStatus)) {
        if (Buffer != NULL) {
            if ( (((DOMAIN_REPLICATION_INFORMATION *)Buffer)->ReplicaSourceNodeName.MaximumLength >= 0) &&
                 (((DOMAIN_REPLICATION_INFORMATION *)Buffer)->ReplicaSourceNodeName.Buffer != NULL) ) {

                printf("Succeeded\n");

            } else {
                printf("Failed\n");
                printf("        String body returned and allocated,\n");
                printf("        but character buffer pointer is NULL.\n");
                TestStatus = FALSE;
            }
            SamFreeMemory( Buffer );
        } else {
            printf("Failed\n");
            printf("        Buffer address not set on return.\n");
            printf("        RPC should have allocated a buffer.\n");
            TestStatus = FALSE;
        }
    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    }



    //
    // Query domain general Information...
    //

    printf("      Query General Information . . . . . . . . . . . . . .     ");

    Buffer = NULL;
    NtStatus = SamQueryInformationDomain(
                   DomainHandle,
                   DomainGeneralInformation,
                   &Buffer
                   );
    if (NT_SUCCESS(NtStatus)) {
        if (Buffer != NULL) {

            printf("Succeeded\n");
            printf("          Number of Users  is: 0x%lx\n",
                 ((DOMAIN_GENERAL_INFORMATION *)Buffer)->UserCount );
            printf("          Number of groups is: 0x%lx\n",
                 ((DOMAIN_GENERAL_INFORMATION *)Buffer)->GroupCount);
            printf("          Number of aliases is: 0x%lx\n",
                 ((DOMAIN_GENERAL_INFORMATION *)Buffer)->AliasCount);


            SamFreeMemory( Buffer );

        } else {
            printf("Failed\n");
            printf("        Buffer address not set on return.\n");
            printf("        RPC should have allocated a buffer.\n");
            TestStatus = FALSE;
        }
    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    }



    //
    // Query domain general Information...
    //

    printf("      Query General Information 2 . . . . . . . . . . . . .     ");

    Buffer = NULL;
    NtStatus = SamQueryInformationDomain(
                   DomainHandle,
                   DomainGeneralInformation2,
                   &Buffer
                   );
    if (NT_SUCCESS(NtStatus)) {
        if (Buffer != NULL) {

            printf("Succeeded\n");
            printf("          Number of Users  is: 0x%lx\n",
                 ((DOMAIN_GENERAL_INFORMATION2 *)Buffer)->I1.UserCount );
            printf("          Number of groups is: 0x%lx\n",
                 ((DOMAIN_GENERAL_INFORMATION2 *)Buffer)->I1.GroupCount);
            printf("          Number of aliases is: 0x%lx\n",
                 ((DOMAIN_GENERAL_INFORMATION2 *)Buffer)->I1.AliasCount);


            SamFreeMemory( Buffer );

        } else {
            printf("Failed\n");
            printf("        Buffer address not set on return.\n");
            printf("        RPC should have allocated a buffer.\n");
            TestStatus = FALSE;
        }
    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    }








    ///////////////////////////////////////////////////////////////////////////
    //                                                                       //
    // Set Suite                                                             //
    //                                                                       //
    ///////////////////////////////////////////////////////////////////////////

    printf("    Set Information . . . . . . . . . . . . . . . . . . .   Suite\n");

    //
    // Set all the fixed length info levels
    //   - Password, Logoff, ServerRole, DomainState, ModifiedCount
    //

/*
 *  CANT TEST SERVER STATE SETTING WITHOUT BREAKING THE REST OF THE TEST.
 *  THE REASON IS, ONCE THE STATE IS CHANGED, NOTHING ELSE CAN BE DONE.
 *
 *    printf("      Set DomainState . . . . . . . . . . . . . . . . . . .     ");
 *
 *    //
 *    // Get the current value...
 *    //
 *
 *    NtStatus = SamQueryInformationDomain(
 *                   DomainHandle,
 *                   DomainStateInformation,
 *                   &Buffer1
 *                   );
 *    ASSERT( NT_SUCCESS(NtStatus) );
 *
 *    //
 *    // Change the field to a new value and write it out.
 *    //
 *
 *    if ( ((DOMAIN_STATE_INFORMATION *)Buffer1)->DomainServerState ==
 *         DomainServerEnabled ) {
 *        ((DOMAIN_STATE_INFORMATION *)Buffer1)->DomainServerState =
 *            DomainServerDisabled;
 *    } else {
 *        ((DOMAIN_STATE_INFORMATION *)Buffer1)->DomainServerState =
 *            DomainServerEnabled;
 *    }
 *
 *    NtStatus = SamSetInformationDomain(
 *                   DomainHandle,
 *                   DomainStateInformation,
 *                   Buffer1
 *                   );
 *    if ( NT_SUCCESS(NtStatus) ) {
 *
 *        //
 *        // Now check that the change was really made...
 *        //
 *
 *        NtStatus = SamQueryInformationDomain(
 *                       DomainHandle,
 *                       DomainStateInformation,
 *                       &Buffer2
 *                       );
 *        ASSERT(NT_SUCCESS( NtStatus ) );
 *        if (((DOMAIN_STATE_INFORMATION *)Buffer1)->DomainServerState ==
 *            ((DOMAIN_STATE_INFORMATION *)Buffer2)->DomainServerState    ) {
 *
 *                printf("Succeeded\n");
 *
 *        } else {
 *
 *            printf("Failed\n");
 *            printf("        Value queried doesn't match value written\n");
 *            printf("        Value Written is   0x%lx\n",
 *                (ULONG)((DOMAIN_STATE_INFORMATION *)Buffer1)->DomainServerState);
 *            printf("        Value Retrieved is 0x%lx\n",
 *                (ULONG)((DOMAIN_STATE_INFORMATION *)Buffer2)->DomainServerState);
 *
 *            TestStatus = FALSE;
 *
 *        }
 *
 *        SamFreeMemory( Buffer1 );
 *        SamFreeMemory( Buffer2 );
 *
 *    } else {
 *        printf("Failed\n");
 *        printf("        Completion status is 0x%lx\n", NtStatus);
 *        TestStatus = FALSE;
 *        SamFreeMemory( Buffer1 );
 *
 *    }
 */



/*
 *  CANT TEST SERVER ROLE SETTING WITHOUT BREAKING THE REST OF THE TEST.
 *  THE REASON IS, ONCE THE ROLE IS SET TO BACKUP, NOTHING ELSE CAN BE
 *  SET.
 *
 *   printf("      Set ServerRole  . . . . . . . . . . . . . . . . . . .     ");
 *
 *   //
 *   // Get the current value...
 *   //
 *
 *   NtStatus = SamQueryInformationDomain(
 *                  DomainHandle,
 *                  DomainServerRoleInformation,
 *                  &Buffer1
 *                  );
 *   ASSERT( NT_SUCCESS(NtStatus) );
 *
 *   //
 *   // Change the field to a new value and write it out.
 *   //
 *
 *   if ( ((DOMAIN_SERVER_ROLE_INFORMATION *)Buffer1)->DomainServerRole ==
 *        DomainServerRolePrimary ) {
 *       ((DOMAIN_SERVER_ROLE_INFORMATION *)Buffer1)->DomainServerRole =
 *           DomainServerRoleBackup;
 *   } else {
 *       ((DOMAIN_SERVER_ROLE_INFORMATION *)Buffer1)->DomainServerRole =
 *           DomainServerRolePrimary;
 *   }
 *
 *   NtStatus = SamSetInformationDomain(
 *                  DomainHandle,
 *                  DomainServerRoleInformation,
 *                  Buffer1
 *                  );
 *   if ( NT_SUCCESS(NtStatus) ) {
 *
 *       //
 *       // Now check that the change was really made...
 *       //
 *
 *       NtStatus = SamQueryInformationDomain(
 *                      DomainHandle,
 *                      DomainServerRoleInformation,
 *                      &Buffer2
 *                      );
 *       ASSERT(NT_SUCCESS( NtStatus ) );
 *       if (((DOMAIN_SERVER_ROLE_INFORMATION *)Buffer1)->DomainServerRole ==
 *           ((DOMAIN_SERVER_ROLE_INFORMATION *)Buffer2)->DomainServerRole    ) {
 *
 *               printf("Succeeded\n");
 *
 *       } else {
 *
 *           printf("Failed\n");
 *           printf("        Value queried doesn't match value written\n");
 *           printf("        Value Written is   0x%lx\n",
 *               (ULONG)((DOMAIN_SERVER_ROLE_INFORMATION *)Buffer1)->DomainServerRole);
 *           printf("        Value Retrieved is 0x%lx\n",
 *               (ULONG)((DOMAIN_SERVER_ROLE_INFORMATION *)Buffer2)->DomainServerRole);
 *
 *           TestStatus = FALSE;
 *
 *       }
 *
 *       SamFreeMemory( Buffer1 );
 *       SamFreeMemory( Buffer2 );
 *
 *   } else {
 *       printf("Failed\n");
 *       printf("        Completion status is 0x%lx\n", NtStatus);
 *       TestStatus = FALSE;
 *       SamFreeMemory( Buffer1 );
 *
 *   }
 */



    printf("      Set Password Information  . . . . . . . . . . . . . .     ");


    //
    // Get the current value...
    //

    NtStatus = SamQueryInformationDomain(
                   DomainHandle,
                   DomainPasswordInformation,
                   &Buffer1
                   );
    ASSERT( NT_SUCCESS(NtStatus) );

    //
    // Change a field to a new value and write it out.
    //

    if ( ((DOMAIN_PASSWORD_INFORMATION *)Buffer1)->MinPasswordLength == 0 ) {
         ((DOMAIN_PASSWORD_INFORMATION *)Buffer1)->MinPasswordLength =  6;
    } else {
        ((DOMAIN_PASSWORD_INFORMATION *)Buffer1)->MinPasswordLength =   0;
    }

    //
    // Set PasswordProperties to COMPLEX so that tests run after this one
    // are a little more interesting.
    //

    ((DOMAIN_PASSWORD_INFORMATION *)Buffer1)->PasswordProperties |= DOMAIN_PASSWORD_COMPLEX;

    NtStatus = SamSetInformationDomain(
                   DomainHandle,
                   DomainPasswordInformation,
                   Buffer1
                   );
    if ( NT_SUCCESS(NtStatus) ) {

        //
        // Now check that the change was really made...
        //

        NtStatus = SamQueryInformationDomain(
                       DomainHandle,
                       DomainPasswordInformation,
                       &Buffer2
                       );
        ASSERT(NT_SUCCESS( NtStatus ) );
        if (((DOMAIN_PASSWORD_INFORMATION *)Buffer1)->MinPasswordLength ==
            ((DOMAIN_PASSWORD_INFORMATION *)Buffer2)->MinPasswordLength    ) {

                printf("Succeeded\n");

        } else {

            printf("Failed\n");
            printf("        Value queried doesn't match value written\n");
            printf("        Value Written is   0x%lx\n",
                (ULONG)((DOMAIN_PASSWORD_INFORMATION *)Buffer1)->MinPasswordLength);
            printf("        Value Retrieved is 0x%lx\n",
                (ULONG)((DOMAIN_PASSWORD_INFORMATION *)Buffer2)->MinPasswordLength);

            TestStatus = FALSE;

        }

        SamFreeMemory( Buffer1 );
        SamFreeMemory( Buffer2 );

    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
        SamFreeMemory( Buffer1 );

    }



    printf("      Set Logoff Information  . . . . . . . . . . . . . . .     ");

    //
    // Get the current value...
    //

    NtStatus = SamQueryInformationDomain(
                   DomainHandle,
                   DomainLogoffInformation,
                   &Buffer1
                   );
    ASSERT( NT_SUCCESS(NtStatus) );

    //
    // Change the field to a new value and write it out.
    //

    if ( ((DOMAIN_LOGOFF_INFORMATION *)Buffer1)->ForceLogoff.LowPart == 0 ) {
         ((DOMAIN_LOGOFF_INFORMATION *)Buffer1)->ForceLogoff.LowPart = 1000;
    } else {
        ((DOMAIN_LOGOFF_INFORMATION *)Buffer1)->ForceLogoff.LowPart =   0;
    }

    NtStatus = SamSetInformationDomain(
                   DomainHandle,
                   DomainLogoffInformation,
                   Buffer1
                   );
    if ( NT_SUCCESS(NtStatus) ) {

        //
        // Now check that the change was really made...
        //

        NtStatus = SamQueryInformationDomain(
                       DomainHandle,
                       DomainLogoffInformation,
                       &Buffer2
                       );
        ASSERT(NT_SUCCESS( NtStatus ) );
        if (((DOMAIN_LOGOFF_INFORMATION *)Buffer1)->ForceLogoff.LowPart ==
            ((DOMAIN_LOGOFF_INFORMATION *)Buffer2)->ForceLogoff.LowPart    ) {

                printf("Succeeded\n");

        } else {

            printf("Failed\n");
            printf("        Value queried doesn't match value written\n");
            printf("        Value Written is   0x%lx\n",
                (ULONG)((DOMAIN_LOGOFF_INFORMATION *)Buffer1)->ForceLogoff.LowPart);
            printf("        Value Retrieved is 0x%lx\n",
                (ULONG)((DOMAIN_LOGOFF_INFORMATION *)Buffer2)->ForceLogoff.LowPart);

            TestStatus = FALSE;

        }

        SamFreeMemory( Buffer1 );
        SamFreeMemory( Buffer2 );

    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
        SamFreeMemory( Buffer1 );

    }



    printf("      Set Modified  . . . . . . . . . . . . . . . . . . . .     ");


    NtStatus = SamSetInformationDomain(
                   DomainHandle,
                   DomainModifiedInformation,
                   &LargeInteger1
                   );

    if (NtStatus != STATUS_INVALID_INFO_CLASS) {

        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    } else {
        printf("Succeeded\n");
    }


    printf("      Set Lockout Information . . . . . . . . . . . . . . .     ");

    //
    // Get the current value...
    //

    NtStatus = SamQueryInformationDomain(
                   DomainHandle,
                   DomainLockoutInformation,
                   &Buffer1
                   );
    ASSERT( NT_SUCCESS(NtStatus) );

    //
    // Change the field to a new value and write it out.
    //

    if ( ((DOMAIN_LOCKOUT_INFORMATION *)Buffer1)->LockoutDuration.LowPart == 0 ) {
         ((DOMAIN_LOCKOUT_INFORMATION *)Buffer1)->LockoutDuration.LowPart = 9000000;
    } else {
        ((DOMAIN_LOCKOUT_INFORMATION *)Buffer1)->LockoutDuration.LowPart =   0;
    }
    if ( ((DOMAIN_LOCKOUT_INFORMATION *)Buffer1)->LockoutObservationWindow.LowPart == 0 ) {
         ((DOMAIN_LOCKOUT_INFORMATION *)Buffer1)->LockoutObservationWindow.LowPart = 8000000;
    } else {
        ((DOMAIN_LOCKOUT_INFORMATION *)Buffer1)->LockoutObservationWindow.LowPart =   0;
    }
    if ( ((DOMAIN_LOCKOUT_INFORMATION *)Buffer1)->LockoutThreshold == 0 ) {
         ((DOMAIN_LOCKOUT_INFORMATION *)Buffer1)->LockoutThreshold =  2;
    } else {
        ((DOMAIN_LOCKOUT_INFORMATION *)Buffer1)->LockoutThreshold  =  0;
    }

    NtStatus = SamSetInformationDomain(
                   DomainHandle,
                   DomainLockoutInformation,
                   Buffer1
                   );
    if ( NT_SUCCESS(NtStatus) ) {

        //
        // Now check that the change was really made...
        //

        NtStatus = SamQueryInformationDomain(
                       DomainHandle,
                       DomainLockoutInformation,
                       &Buffer2
                       );
        ASSERT(NT_SUCCESS( NtStatus ) );
        if ( (((DOMAIN_LOCKOUT_INFORMATION *)Buffer1)->LockoutDuration.LowPart ==
             ((DOMAIN_LOCKOUT_INFORMATION *)Buffer2)->LockoutDuration.LowPart    ) &&
             (((DOMAIN_LOCKOUT_INFORMATION *)Buffer1)->LockoutObservationWindow.LowPart ==
             ((DOMAIN_LOCKOUT_INFORMATION *)Buffer2)->LockoutObservationWindow.LowPart    ) &&
             (((DOMAIN_LOCKOUT_INFORMATION *)Buffer1)->LockoutThreshold ==
             ((DOMAIN_LOCKOUT_INFORMATION *)Buffer2)->LockoutThreshold    ) ) {

                printf("Succeeded\n");

        } else {

            printf("Failed\n");
            printf("        Value queried doesn't match value written\n");
            printf("        Duration Written is   0x%lx\n",
                (ULONG)((DOMAIN_LOCKOUT_INFORMATION *)Buffer1)->LockoutDuration.LowPart);
            printf("        Duration  Retrieved is 0x%lx\n",
                (ULONG)((DOMAIN_LOCKOUT_INFORMATION *)Buffer2)->LockoutDuration.LowPart);
            printf("        Window Written is   0x%lx\n",
                (ULONG)((DOMAIN_LOCKOUT_INFORMATION *)Buffer1)->LockoutObservationWindow.LowPart);
            printf("        Window  Retrieved is 0x%lx\n",
                (ULONG)((DOMAIN_LOCKOUT_INFORMATION *)Buffer2)->LockoutObservationWindow.LowPart);
            printf("        Duration Written is   0x%lx\n",
                (ULONG)((DOMAIN_LOCKOUT_INFORMATION *)Buffer1)->LockoutThreshold);
            printf("        Duration  Retrieved is 0x%lx\n",
                (ULONG)((DOMAIN_LOCKOUT_INFORMATION *)Buffer2)->LockoutThreshold);

            TestStatus = FALSE;

        }

        SamFreeMemory( Buffer1 );
        SamFreeMemory( Buffer2 );

    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
        SamFreeMemory( Buffer1 );

    }




    printf("      Set Domain Name . . . . . . . . . . . . . . . . . . .     ");


    NtStatus = SamSetInformationDomain(
                   DomainHandle,
                   DomainNameInformation,
                   &DummyName1
                   );

    if (NtStatus != STATUS_INVALID_INFO_CLASS) {

        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    } else {
        printf("Succeeded\n");
    }


    printf("      Set OEM Information . . . . . . . . . . . . . . . . .     ");

    //
    // Get the current value...
    //

    NtStatus = SamQueryInformationDomain(
                   DomainHandle,
                   DomainOemInformation,
                   &Buffer1
                   );
    ASSERT( NT_SUCCESS(NtStatus) );

    //
    // Change the field to a new value and write it out.
    //

    NameLength = ((DOMAIN_OEM_INFORMATION *)Buffer1)->OemInformation.Length;
    if (  NameLength == DummyName1.Length ) {
        ((DOMAIN_OEM_INFORMATION *)Buffer1)->OemInformation = DummyName2;
    } else {
        ((DOMAIN_OEM_INFORMATION *)Buffer1)->OemInformation = DummyName1;
    }

    NtStatus = SamSetInformationDomain(
                   DomainHandle,
                   DomainOemInformation,
                   Buffer1
                   );
    if ( NT_SUCCESS(NtStatus) ) {

        //
        // Now check that the change was really made...
        //

        NtStatus = SamQueryInformationDomain(
                       DomainHandle,
                       DomainOemInformation,
                       &Buffer2
                       );
        ASSERT(NT_SUCCESS( NtStatus ) );
        if (((DOMAIN_OEM_INFORMATION *)Buffer1)->OemInformation.Length ==
            ((DOMAIN_OEM_INFORMATION *)Buffer2)->OemInformation.Length    ) {

            printf("Succeeded\n");

        } else {

            printf("Failed\n");
            printf("        Value queried doesn't match value written\n");
            printf("        Value Written is   0x%lx\n",
                (ULONG)((DOMAIN_OEM_INFORMATION *)Buffer1)->OemInformation.Length);
            printf("        Value Retrieved is 0x%lx\n",
                (ULONG)((DOMAIN_OEM_INFORMATION *)Buffer2)->OemInformation.Length);

            TestStatus = FALSE;

        }

        SamFreeMemory( Buffer1 );
        SamFreeMemory( Buffer2 );

    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
        SamFreeMemory( Buffer1 );

    }




    printf("      Set Replication Information . . . . . . . . . . . . .     ");

    //
    // Get the current value...
    //

    NtStatus = SamQueryInformationDomain(
                   DomainHandle,
                   DomainReplicationInformation,
                   &Buffer1
                   );
    ASSERT( NT_SUCCESS(NtStatus) );

    //
    // Change the field to a new value and write it out.
    //

    NameLength = ((DOMAIN_REPLICATION_INFORMATION *)Buffer1)->ReplicaSourceNodeName.Length;
    if (  NameLength == DummyName1.Length ) {
        ((DOMAIN_REPLICATION_INFORMATION *)Buffer1)->ReplicaSourceNodeName = DummyName2;
    } else {
        ((DOMAIN_REPLICATION_INFORMATION *)Buffer1)->ReplicaSourceNodeName = DummyName1;
    }

    NtStatus = SamSetInformationDomain(
                   DomainHandle,
                   DomainReplicationInformation,
                   Buffer1
                   );
    if ( NT_SUCCESS(NtStatus) ) {

        //
        // Now check that the change was really made...
        //

        NtStatus = SamQueryInformationDomain(
                       DomainHandle,
                       DomainReplicationInformation,
                       &Buffer2
                       );
        ASSERT(NT_SUCCESS( NtStatus ) );
        if (((DOMAIN_REPLICATION_INFORMATION *)Buffer1)->ReplicaSourceNodeName.Length ==
            ((DOMAIN_REPLICATION_INFORMATION *)Buffer2)->ReplicaSourceNodeName.Length    ) {

            printf("Succeeded\n");

        } else {

            printf("Failed\n");
            printf("        Value queried doesn't match value written\n");
            printf("        Value Written is   0x%lx\n",
                (ULONG)((DOMAIN_REPLICATION_INFORMATION *)Buffer1)->ReplicaSourceNodeName.Length);
            printf("        Value Retrieved is 0x%lx\n",
                (ULONG)((DOMAIN_REPLICATION_INFORMATION *)Buffer2)->ReplicaSourceNodeName.Length);

            TestStatus = FALSE;

        }

        SamFreeMemory( Buffer1 );
        SamFreeMemory( Buffer2 );

    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
        SamFreeMemory( Buffer1 );

    }




    ///////////////////////////////////////////////////////////////////////////
    //                                                                       //
    // Create User/Group/Alias Suite                                         //
    //                                                                       //
    ///////////////////////////////////////////////////////////////////////////

    printf("    Create User/Group/Alias . . . . . . . . . . . . . . . .   Suite\n");


    printf("      Create Group  . . . . . . . . . . . . . . . . . . . .     ");

    RtlInitString( &AccountNameAnsi, GROUP_NAME1 );
    NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
    TST_SUCCESS_ASSERT(NtStatus);


    //InitializeObjectAttributes( &ObjectAttributes, &AccountName, 0, 0, NULL );

    GroupRid = 0;
    GroupHandle = NULL;
    NtStatus = SamCreateGroupInDomain(
                   DomainHandle,
                   &AccountName,
                   GROUP_ALL_ACCESS,
                   &GroupHandle,
                   &GroupRid
                   );
    RtlFreeUnicodeString( &AccountName );

    if (NT_SUCCESS(NtStatus)) {
        if ( (GroupHandle == NULL) || (GroupRid == 0) ) {

        printf("Failed\n");
        printf("        Invalid GroupHandle or GroupRid returned.\n");
        printf("        Completion status is  0x%lx\n", NtStatus);
        printf("        GroupHandle value is: 0x%lx\n", (ULONG)GroupHandle);
        printf("        GroupRid value is:    0x%lx\n", GroupRid);
        TestStatus = FALSE;
        } else {

            printf("Succeeded\n");
            SavedGroupRid = GroupRid;
            NtStatus = SamCloseHandle( GroupHandle );
            if (!NT_SUCCESS(NtStatus)) {
                printf("        SamCloseHandle() completion status is: 0x%lx\n", NtStatus);
            }
            ASSERT( NT_SUCCESS(NtStatus) );

        }

    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    }




    printf("      Create Duplicate Group  . . . . . . . . . . . . . . .     ");
    RtlInitString( &AccountNameAnsi, GROUP_NAME1 );
    NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
    TST_SUCCESS_ASSERT(NtStatus);

    //InitializeObjectAttributes( &ObjectAttributes, &AccountName, 0, 0, NULL );


    GroupRid = 0;
    GroupHandle = NULL;
    NtStatus = SamCreateGroupInDomain(
                   DomainHandle,
                   &AccountName,
                   GROUP_ALL_ACCESS,
                   &GroupHandle,
                   &GroupRid
                   );
    RtlFreeUnicodeString( &AccountName );

    if (NtStatus != STATUS_GROUP_EXISTS) {

        printf("Failed\n");
        printf("        Completion status should be STATUS_GROUP_EXISTS\n");
        printf("        Completion status is  0x%lx\n", NtStatus);
        TestStatus = FALSE;

    } else {

        printf("Succeeded\n");

    }



    printf("      Create Alias  . . . . . . . . . . . . . . . . . . . .     ");

    RtlInitString( &AccountNameAnsi, ALIAS_NAME1 );
    NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
    TST_SUCCESS_ASSERT(NtStatus);


    AliasRid = 0;
    AliasHandle = NULL;
    NtStatus = SamCreateAliasInDomain(
                   DomainHandle,
                   &AccountName,
                   ALIAS_ALL_ACCESS,
                   &AliasHandle,
                   &AliasRid
                   );
    RtlFreeUnicodeString( &AccountName );

    if (NT_SUCCESS(NtStatus)) {
        if ( (AliasHandle == NULL) || (AliasRid == 0) ) {

        printf("Failed\n");
        printf("        Invalid AliasHandle or AliasRid returned.\n");
        printf("        Completion status is  0x%lx\n", NtStatus);
        printf("        AliasHandle value is: 0x%lx\n", (ULONG)AliasHandle);
        printf("        AliasRid value is:    0x%lx\n", AliasRid);
        TestStatus = FALSE;
        } else {

            printf("Succeeded\n");
            SavedAliasRid = AliasRid;
            NtStatus = SamCloseHandle( AliasHandle );
            if (!NT_SUCCESS(NtStatus)) {
                printf("        SamCloseHandle() completion status is: 0x%lx\n", NtStatus);
            }
            ASSERT( NT_SUCCESS(NtStatus) );


            if (AliasRid == SavedGroupRid) {
                printf("      Create Group/Alias Comparison. . . . . . . . . . . . .     Failed\n");

                printf("        Same RID assigned to new alias and group.\n");
                TestStatus = FALSE;
            }
        }

    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    }




    printf("      Create another Alias  . . . . . . . . . . . . . . . .     ");

    RtlInitString( &AccountNameAnsi, ALIAS_NAME2 );
    NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
    TST_SUCCESS_ASSERT(NtStatus);


    AliasRid = 0;
    AliasHandle = NULL;
    NtStatus = SamCreateAliasInDomain(
                   DomainHandle,
                   &AccountName,
                   ALIAS_ALL_ACCESS,
                   &AliasHandle,
                   &AliasRid
                   );
    RtlFreeUnicodeString( &AccountName );

    if (NT_SUCCESS(NtStatus)) {
        if ( (AliasHandle == NULL) || (AliasRid == 0) ) {

        printf("Failed\n");
        printf("        Invalid AliasHandle or AliasRid returned.\n");
        printf("        Completion status is  0x%lx\n", NtStatus);
        printf("        AliasHandle value is: 0x%lx\n", (ULONG)AliasHandle);
        printf("        AliasRid value is:    0x%lx\n", AliasRid);
        TestStatus = FALSE;
        } else {

            printf("Succeeded\n");
            SavedAliasRid = AliasRid;
            NtStatus = SamCloseHandle( AliasHandle );
            if (!NT_SUCCESS(NtStatus)) {
                printf("        SamCloseHandle() completion status is: 0x%lx\n", NtStatus);
            }
            ASSERT( NT_SUCCESS(NtStatus) );


            if (AliasRid == SavedGroupRid) {
                printf("      Create Group/Alias Comparison. . . . . . . . . . . . .     Failed\n");

                printf("        Same RID assigned to new alias and group.\n");
                TestStatus = FALSE;
            }
        }

    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    }




    printf("      Create Duplicate Alias  . . . . . . . . . . . . . . .     ");
    RtlInitString( &AccountNameAnsi, ALIAS_NAME1 );
    NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
    TST_SUCCESS_ASSERT(NtStatus);


    AliasRid = 0;
    AliasHandle = NULL;
    NtStatus = SamCreateAliasInDomain(
                   DomainHandle,
                   &AccountName,
                   ALIAS_ALL_ACCESS,
                   &AliasHandle,
                   &AliasRid
                   );
    RtlFreeUnicodeString( &AccountName );

    if (NtStatus != STATUS_ALIAS_EXISTS) {

        printf("Failed\n");
        printf("        Completion status should be STATUS_ALIAS_EXISTS\n");
        printf("        Completion status is  0x%lx\n", NtStatus);
        TestStatus = FALSE;

    } else {

        printf("Succeeded\n");

    }





    printf("      Create User . . . . . . . . . . . . . . . . . . . . .     ");

    RtlInitString( &AccountNameAnsi, USER_NAME1 );
    NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
    TST_SUCCESS_ASSERT(NtStatus);


    UserRid = 0;
    UserHandle = NULL;
    NtStatus = SamCreateUserInDomain(
                   DomainHandle,
                   &AccountName,
                   USER_ALL_ACCESS,
                   &UserHandle,
                   &UserRid
                   );
    RtlFreeUnicodeString( &AccountName );

    if (NT_SUCCESS(NtStatus)) {
        if ( (UserHandle == NULL) || (UserRid == 0) ) {

        printf("Failed\n");
        printf("        Invalid UserHandle or UserRid returned.\n");
        printf("        Completion status is  0x%lx\n", NtStatus);
        printf("        UserHandle value is: 0x%lx\n", (ULONG)UserHandle);
        printf("        UserRid value is:    0x%lx\n", UserRid);
        TestStatus = FALSE;
        } else {

            printf("Succeeded\n");
            ValidUserHandle = UserHandle;


            if (UserRid == SavedGroupRid) {
                printf("      Create Group/User Comparison. . . . . . . . . . . . .     Failed\n");

                printf("        Same RID assigned to new user and group.\n");
                TestStatus = FALSE;
            }

            if (UserRid == SavedAliasRid) {
                printf("      Create Alias/User Comparison. . . . . . . . . . . . .     Failed\n");

                printf("        Same RID assigned to new user and alias.\n");
                TestStatus = FALSE;
            }
        }

    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    }







    printf("      Create Duplicate User . . . . . . . . . . . . . . . .     ");

    RtlInitString( &AccountNameAnsi, USER_NAME1 );
    NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
    TST_SUCCESS_ASSERT(NtStatus);


    UserRid = 0;
    UserHandle = NULL;
    NtStatus = SamCreateUserInDomain(
                   DomainHandle,
                   &AccountName,
                   USER_ALL_ACCESS,
                   &UserHandle,
                   &UserRid
                   );
    RtlFreeUnicodeString( &AccountName );

    if (NtStatus != STATUS_USER_EXISTS) {

        printf("Failed\n");
        printf("        Completion status should be STATUS_USER_EXISTS\n");
        printf("        Completion status is  0x%lx\n", NtStatus);
        TestStatus = FALSE;

    } else {

        printf("Succeeded\n");

    }




    printf("      Create Group With Same Name As User . . . . . . . . .     ");

    RtlInitString( &AccountNameAnsi, USER_NAME1 );
    NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
    TST_SUCCESS_ASSERT(NtStatus);


    GroupRid = 0;
    GroupHandle = NULL;
    NtStatus = SamCreateGroupInDomain(
                   DomainHandle,
                   &AccountName,
                   GROUP_ALL_ACCESS,
                   &GroupHandle,
                   &GroupRid
                   );
    RtlFreeUnicodeString( &AccountName );

    if (NtStatus != STATUS_USER_EXISTS) {

        printf("Failed\n");
        printf("        Completion status should be STATUS_USER_EXISTS\n");
        printf("        Completion status is  0x%lx\n", NtStatus);
        TestStatus = FALSE;

    } else {

        printf("Succeeded\n");

    }




    printf("      Create Group With Same Name As Alias. . . . . . . . .     ");

    RtlInitString( &AccountNameAnsi, ALIAS_NAME1 );
    NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
    TST_SUCCESS_ASSERT(NtStatus);


    GroupRid = 0;
    GroupHandle = NULL;
    NtStatus = SamCreateGroupInDomain(
                   DomainHandle,
                   &AccountName,
                   GROUP_ALL_ACCESS,
                   &GroupHandle,
                   &GroupRid
                   );
    RtlFreeUnicodeString( &AccountName );

    if (NtStatus != STATUS_ALIAS_EXISTS) {

        printf("Failed\n");
        printf("        Completion status should be STATUS_ALIAS_EXISTS\n");
        printf("        Completion status is  0x%lx\n", NtStatus);
        TestStatus = FALSE;

    } else {

        printf("Succeeded\n");

    }



    printf("      Create Alias With Same Name As Group. . . . . . . . .     ");

    RtlInitString( &AccountNameAnsi, GROUP_NAME1 );
    NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
    TST_SUCCESS_ASSERT(NtStatus);


    AliasRid = 0;
    AliasHandle = NULL;
    NtStatus = SamCreateAliasInDomain(
                   DomainHandle,
                   &AccountName,
                   GROUP_ALL_ACCESS,
                   &AliasHandle,
                   &AliasRid
                   );
    RtlFreeUnicodeString( &AccountName );

    if (NtStatus != STATUS_GROUP_EXISTS) {

        printf("Failed\n");
        printf("        Completion status should be STATUS_GROUP_EXISTS\n");
        printf("        Completion status is  0x%lx\n", NtStatus);
        TestStatus = FALSE;

    } else {

        printf("Succeeded\n");

    }



    printf("      Create User With Same Name As Group . . . . . . . . .     ");

    RtlInitString( &AccountNameAnsi, GROUP_NAME1 );
    NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
    TST_SUCCESS_ASSERT(NtStatus);

    //InitializeObjectAttributes( &ObjectAttributes, &AccountName, 0, 0, NULL );


    UserRid = 0;
    UserHandle = NULL;
    NtStatus = SamCreateUserInDomain(
                   DomainHandle,
                   &AccountName,
                   USER_ALL_ACCESS,
                   &UserHandle,
                   &UserRid
                   );
    RtlFreeUnicodeString( &AccountName );

    if (NtStatus != STATUS_GROUP_EXISTS) {

        printf("Failed\n");
        printf("        Completion status should be STATUS_GROUP_EXISTS\n");
        printf("        Completion status is  0x%lx\n", NtStatus);
        TestStatus = FALSE;

    } else {

        printf("Succeeded\n");

    }



    printf("      Create User With Same Name As Alias . . . . . . . . .     ");

    RtlInitString( &AccountNameAnsi, ALIAS_NAME1 );
    NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
    TST_SUCCESS_ASSERT(NtStatus);


    UserRid = 0;
    UserHandle = NULL;
    NtStatus = SamCreateUserInDomain(
                   DomainHandle,
                   &AccountName,
                   USER_ALL_ACCESS,
                   &UserHandle,
                   &UserRid
                   );
    RtlFreeUnicodeString( &AccountName );

    if (NtStatus != STATUS_ALIAS_EXISTS) {

        printf("Failed\n");
        printf("        Completion status should be STATUS_ALIAS_EXISTS\n");
        printf("        Completion status is  0x%lx\n", NtStatus);
        TestStatus = FALSE;

    } else {

        printf("Succeeded\n");

    }



    ///////////////////////////////////////////////////////////////////////////
    //                                                                       //
    // Call server to test internal functions                                //
    //                                                                       //
    ///////////////////////////////////////////////////////////////////////////

    printf("\n");
    printf("    Test internal functions . . . . . . . . . . . . . . .   Suite\n");
    printf("      Test internal domain functions  . . . . . . . . . .       ");

    NtStatus = SamTestPrivateFunctionsDomain( DomainHandle );

    if ( NT_SUCCESS( NtStatus ) ) {

        printf("Succeeded.\n");

    } else {

        if ( NtStatus == STATUS_NOT_IMPLEMENTED ) {

            printf("Not Implemented\n");

        } else {

            printf("Failed.\n");
            printf("    Status = %lx\n", NtStatus );
            TestStatus = FALSE;
        }
    }

    printf("      Test internal user functions  . . . . . . . . . . .       ");

    if (ValidUserHandle == NULL) {

        printf("Test omitted - Valid User handle not available\n");
        TestStatus = FALSE;

    } else {

        NtStatus = SamTestPrivateFunctionsUser( ValidUserHandle );
        IgnoreStatus = SamCloseHandle( ValidUserHandle );
        ASSERT( NT_SUCCESS(IgnoreStatus) );

        if ( NT_SUCCESS( NtStatus ) ) {

            printf("Succeeded.\n");

        } else {

            if ( NtStatus == STATUS_NOT_IMPLEMENTED ) {

                printf("Not Implemented\n");

            } else {

                printf("Failed.\n");
                printf("    Status = %lx\n", NtStatus );
                TestStatus = FALSE;
            }
        }
    }


    ///////////////////////////////////////////////////////////////////////////
    //                                                                       //
    // Enumerate Users/Groups Suite                                          //
    //                                                                       //
    ///////////////////////////////////////////////////////////////////////////


    printf("    Enumerate Users/Groups/Aliases. . . . . . . . . . . .   Suite\n");

    printf("      Enumerate Groups - large prefered length  . . . . . .     ");


    EnumerationContext = 0;
    NtStatus = SamEnumerateGroupsInDomain(
                   DomainHandle,
                   &EnumerationContext,
                   &Buffer,
                   12000,                   // PreferedMaximumLength
                   &CountReturned
                   );
    AccountCount = CountReturned;       // Save for future test

    if (NT_SUCCESS(NtStatus)) {
        if (Buffer != NULL) {
            if (NtStatus == STATUS_SUCCESS) {

                if (CountReturned > 1) {
                    printf("Succeeded\n");
                    for (i=0; i<CountReturned; i++) {
                        printf("            Rid/Name(%ld): 0x%lx / %wZ\n",i,
                               ((PSAM_RID_ENUMERATION)(Buffer))[i].RelativeId,
                              &((PSAM_RID_ENUMERATION)(Buffer))[i].Name
                              );
                    }

                } else {
                    printf("Failed\n");
                    printf("        Completion status is 0x%lx\n", NtStatus);
                    printf("        Expected several entries to be returned.\n");
                    printf("        Received 0x%lx entries instead.\n", CountReturned);
                    TestStatus = FALSE;
                }

            } else {

                printf("Failed\n");
                printf("        Expected STATUS_MORE_ENTRIES to be returned.\n");
                printf("        Received 0x%lx instead.\n", NtStatus);
                printf("        Buffer        = 0x%lx\n", (ULONG)Buffer);
                printf("        CountReturned = 0x%lx\n", CountReturned);
                TestStatus = FALSE;
            }

            SamFreeMemory( Buffer );

        } else {
            printf("Failed\n");
            printf("        Buffer address not set on return.\n");
            printf("        RPC should have allocated a buffer.\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }

    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;

    }




    printf("      Enumerate Groups - small prefered length  . . . . . .     ");


    for ( i=0; i<AccountCount; i++) {
        EnumerationContext = i;
        NtStatus = SamEnumerateGroupsInDomain(
                       DomainHandle,
                       &EnumerationContext,
                       &Buffer,
                       0,                   // PreferedMaximumLength
                       &CountReturned
                       );

        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {
                if ( ((i >= AccountCount -1) && (NtStatus == STATUS_SUCCESS)) ||
                     ((i <= AccountCount -1) && (NtStatus == STATUS_MORE_ENTRIES))  ) {

                    if (CountReturned != 1) {
                        printf("Failed\n");
                        printf("        Completion status is 0x%lx\n", NtStatus);
                        printf("        Expected one entry to be returned.\n");
                        printf("        Received 0x%lx entries instead.\n", CountReturned);
                        TestStatus = FALSE;
                        i = AccountCount + 100;
                    }

                } else {

                    printf("Failed\n");
                    if (i < AccountCount -1 ) {
                        printf("        Expected STATUS_MORE_ENTRIES to be returned.\n");
                    } else {
                        printf("        Expected STATUS_SUCCESS to be returned.\n");
                    }
                    printf("        Received 0x%lx instead.\n", NtStatus);
                    printf("        Buffer        = 0x%lx\n", (ULONG)Buffer);
                    printf("        CountReturned = 0x%lx\n", CountReturned);
                    TestStatus = FALSE;
                    i = AccountCount + 100;
                }

                SamFreeMemory( Buffer );

            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
                i = AccountCount + 100;
            }

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
            i = AccountCount + 100;

        }
    }

    if ( i == AccountCount) {
        printf("Succeeded\n");
    }




    printf("      Enumerate Aliases - large prefered length . . . . . .     ");


    EnumerationContext = 0;
    NtStatus = SamEnumerateAliasesInDomain(
                   DomainHandle,
                   &EnumerationContext,
                   &Buffer,
                   12000,                   // PreferedMaximumLength
                   &CountReturned
                   );
    AccountCount = CountReturned;       // Save for future test

    if (NT_SUCCESS(NtStatus)) {
        if (Buffer != NULL) {
            if (NtStatus == STATUS_SUCCESS) {

                if (CountReturned > 1) {
                    printf("Succeeded\n");
                    for (i=0; i<CountReturned; i++) {
                        printf("            Rid/Name(%ld): 0x%lx / %wZ\n",i,
                               ((PSAM_RID_ENUMERATION)(Buffer))[i].RelativeId,
                              &((PSAM_RID_ENUMERATION)(Buffer))[i].Name
                              );
                    }

                } else {
                    printf("Failed\n");
                    printf("        Completion status is 0x%lx\n", NtStatus);
                    printf("        Expected several entries to be returned.\n");
                    printf("        Received 0x%lx entries instead.\n", CountReturned);
                    TestStatus = FALSE;
                }

            } else {

                printf("Failed\n");
                printf("        Expected STATUS_MORE_ENTRIES to be returned.\n");
                printf("        Received 0x%lx instead.\n", NtStatus);
                printf("        Buffer        = 0x%lx\n", (ULONG)Buffer);
                printf("        CountReturned = 0x%lx\n", CountReturned);
                TestStatus = FALSE;
            }

            SamFreeMemory( Buffer );

        } else {
            printf("Failed\n");
            printf("        Buffer address not set on return.\n");
            printf("        RPC should have allocated a buffer.\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }

    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;

    }




    printf("      Enumerate Aliases - small prefered length . . . . . .     ");


    for ( i=0; i<AccountCount; i++) {
        EnumerationContext = i;
        NtStatus = SamEnumerateAliasesInDomain(
                       DomainHandle,
                       &EnumerationContext,
                       &Buffer,
                       0,                   // PreferedMaximumLength
                       &CountReturned
                       );

        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {
                if ( ((i >= AccountCount -1) && (NtStatus == STATUS_SUCCESS)) ||
                     ((i <= AccountCount -1) && (NtStatus == STATUS_MORE_ENTRIES))  ) {

                    if (CountReturned != 1) {
                        printf("Failed\n");
                        printf("        Completion status is 0x%lx\n", NtStatus);
                        printf("        Expected one entry to be returned.\n");
                        printf("        Received 0x%lx entries instead.\n", CountReturned);
                        TestStatus = FALSE;
                        i = AccountCount + 100;
                    }

                } else {

                    printf("Failed\n");
                    if (i < AccountCount -1 ) {
                        printf("        Expected STATUS_MORE_ENTRIES to be returned.\n");
                    } else {
                        printf("        Expected STATUS_SUCCESS to be returned.\n");
                    }
                    printf("        Received 0x%lx instead.\n", NtStatus);
                    printf("        Buffer        = 0x%lx\n", (ULONG)Buffer);
                    printf("        CountReturned = 0x%lx\n", CountReturned);
                    TestStatus = FALSE;
                    i = AccountCount + 100;
                }

                SamFreeMemory( Buffer );

            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
                i = AccountCount + 100;
            }

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
            i = AccountCount + 100;

        }
    }

    if ( i == AccountCount) {
        printf("Succeeded\n");
    }





    printf("      Enumerate Users  - large prefered length  . . . . . .     ");


    EnumerationContext = 0;
    NtStatus = SamEnumerateUsersInDomain(
                   DomainHandle,
                   &EnumerationContext,
                   0,
                   &Buffer,
                   12000,                   // PreferedMaximumLength
                   &CountReturned
                   );
    AccountCount = CountReturned;       // Save for future test

    if (NT_SUCCESS(NtStatus)) {
        if (Buffer != NULL) {
            if (NtStatus == STATUS_SUCCESS) {

                if (CountReturned > 1) {
                    printf("Succeeded\n");
                    for (i=0; i<CountReturned; i++) {
                        printf("            Rid/Name(%ld): 0x%lx / %wZ\n",i,
                               ((PSAM_RID_ENUMERATION)(Buffer))[i].RelativeId,
                              &((PSAM_RID_ENUMERATION)(Buffer))[i].Name
                              );
                    }

                } else {
                    printf("Failed\n");
                    printf("        Completion status is 0x%lx\n", NtStatus);
                    printf("        Expected several entries to be returned.\n");
                    printf("        Received 0x%lx entries instead.\n", CountReturned);
                    TestStatus = FALSE;
                }

            } else {

                printf("Failed\n");
                printf("        Expected STATUS_MORE_ENTRIES to be returned.\n");
                printf("        Received 0x%lx instead.\n", NtStatus);
                printf("        Buffer        = 0x%lx\n", (ULONG)Buffer);
                printf("        CountReturned = 0x%lx\n", CountReturned);
                TestStatus = FALSE;
            }

            SamFreeMemory( Buffer );

        } else {
            printf("Failed\n");
            printf("        Buffer address not set on return.\n");
            printf("        RPC should have allocated a buffer.\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }

    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;

    }




    printf("      Enumerate Users  - small prefered length  . . . . . .     ");


    for ( i=0; i<AccountCount; i++) {
        EnumerationContext = i;
        NtStatus = SamEnumerateUsersInDomain(
                       DomainHandle,
                       &EnumerationContext,
                       0,
                       &Buffer,
                       0,                   // PreferedMaximumLength
                       &CountReturned
                       );

        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {
                if ( ((i >= AccountCount -1) && (NtStatus == STATUS_SUCCESS)) ||
                     ((i <= AccountCount -1) && (NtStatus == STATUS_MORE_ENTRIES))  ) {

                    if (CountReturned != 1) {
                        printf("Failed\n");
                        printf("        Completion status is 0x%lx\n", NtStatus);
                        printf("        Expected one entry to be returned.\n");
                        printf("        Received 0x%lx entries instead.\n", CountReturned);
                        TestStatus = FALSE;
                        i = AccountCount + 100;
                    }

                } else {

                    printf("Failed\n");
                    if (i < AccountCount -1 ) {
                        printf("        Expected STATUS_MORE_ENTRIES to be returned.\n");
                    } else {
                        printf("        Expected STATUS_SUCCESS to be returned.\n");
                    }
                    printf("        Received 0x%lx instead.\n", NtStatus);
                    printf("        Buffer        = 0x%lx\n", (ULONG)Buffer);
                    printf("        CountReturned = 0x%lx\n", CountReturned);
                    TestStatus = FALSE;
                    i = AccountCount + 100;
                }

                SamFreeMemory( Buffer );

            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
                i = AccountCount + 100;
            }

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
            i = AccountCount + 100;

        }
    }

    if ( i == AccountCount) {
        printf("Succeeded\n");
    }










    ///////////////////////////////////////////////////////////////////////////
    //                                                                       //
    // Lookup Names/IDs Suite                                                //
    //                                                                       //
    ///////////////////////////////////////////////////////////////////////////


    // LATER add alias search to lookup name suite.....


    printf("\n");
    printf("    Lookup Names/IDs  . . . . . . . . . . . . . . . . . .   Suite\n");


    printf("      Lookup Names (all existing) . . . . . . . . . . . . .     ");

    NtStatus = SamLookupNamesInDomain(
                   DomainHandle,
                   ALL_NAMES_COUNT,
                   &AllNames[0],
                   &LookedUpRids,
                   &LookedUpUses
                   );


    if (NT_SUCCESS(NtStatus)) {
        ASSERT( LookedUpRids != NULL );
        ASSERT( LookedUpUses != NULL );

        if (
            (LookedUpRids[0] == AllRids[0]) && (LookedUpUses[0] == AllUses[0])
                                            &&
            (LookedUpRids[1] == AllRids[1]) && (LookedUpUses[1] == AllUses[1])
                                            &&
            (LookedUpRids[2] == AllRids[2]) && (LookedUpUses[2] == AllUses[2])
            ) {


            printf("Succeeded\n");


        } else {
            printf("Failed\n");
            printf("        Rids or Uses dont match expected values.\n");
            printf("        Expected Rids:  0x%lx, 0x%lx, 0x%lx\n",
                AllRids[0], AllRids[1], AllRids[2]);
            printf("        Received Rids:  0x%lx, 0x%lx, 0x%lx\n",
                LookedUpRids[0], LookedUpRids[1], LookedUpRids[2]);
            printf("        Expected Uses:  0x%lx, 0x%lx, 0x%lx\n",
                AllUses[0], AllUses[1], AllUses[2]);
            printf("        Received Uses:  0x%lx, 0x%lx, 0x%lx\n",
                LookedUpUses[0], LookedUpUses[1], LookedUpUses[2]);
            TestStatus = FALSE;
        }


        SamFreeMemory( LookedUpRids );
        SamFreeMemory( LookedUpUses );

    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    }




    printf("      Lookup Names (Some existing)  . . . . . . . . . . . .     ");

    NtStatus = SamLookupNamesInDomain(
                   DomainHandle,
                   SOME_NAMES_COUNT,
                   &SomeNames[0],
                   &LookedUpRids,
                   &LookedUpUses
                   );


    if (NtStatus == STATUS_SOME_NOT_MAPPED) {
        ASSERT( LookedUpRids != NULL );
        ASSERT( LookedUpUses != NULL );

        if (
            (LookedUpRids[0] == SomeRids[0]) && (LookedUpUses[0] == SomeUses[0])
                                             &&
            (LookedUpRids[1] == SomeRids[1]) && (LookedUpUses[1] == SomeUses[1])
                                             &&
            (LookedUpRids[2] == SomeRids[2]) && (LookedUpUses[2] == SomeUses[2])
                                             &&
            (LookedUpRids[3] == SomeRids[3]) && (LookedUpUses[3] == SomeUses[3])
                                             &&
            (LookedUpRids[4] == SomeRids[4]) && (LookedUpUses[4] == SomeUses[4])
                                             &&
            (LookedUpRids[5] == SomeRids[5]) && (LookedUpUses[5] == SomeUses[5])
                                             &&
            (LookedUpRids[6] == SomeRids[6]) && (LookedUpUses[6] == SomeUses[6])
            ) {


            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Rids or Uses dont match expected values.\n");
            printf("        Expected Rids:  0x%lx, 0x%lx, 0x%lx, 0x%lx,  0x%lx, 0x%lx, 0x%lx\n",
                SomeRids[0], SomeRids[1], SomeRids[2], SomeRids[3], SomeRids[4], SomeRids[5], SomeRids[6]);
            printf("        Received Rids:  0x%lx, 0x%lx, 0x%lx, 0x%lx,  0x%lx, 0x%lx, 0x%lx\n",
                LookedUpRids[0], LookedUpRids[1], LookedUpRids[2], LookedUpRids[3], LookedUpRids[4], LookedUpRids[5], LookedUpRids[6]);
            printf("        Expected Uses:  0x%lx, 0x%lx, 0x%lx, 0x%lx,  0x%lx, 0x%lx, 0x%lx\n",
                SomeUses[0], SomeUses[1], SomeUses[2], SomeUses[3], SomeUses[4], SomeUses[5], SomeUses[6]);
            printf("        Received Uses:  0x%lx, 0x%lx, 0x%lx, 0x%lx,  0x%lx, 0x%lx, 0x%lx\n",
                LookedUpUses[0], LookedUpUses[1], LookedUpUses[2], LookedUpUses[3], LookedUpUses[4], LookedUpUses[5], LookedUpUses[2]);
            TestStatus = FALSE;
        }


        SamFreeMemory( LookedUpRids );
        SamFreeMemory( LookedUpUses );

    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    }



    printf("      Lookup Names (None existing)  . . . . . . . . . . . .     ");

    NtStatus = SamLookupNamesInDomain(
                   DomainHandle,
                   NO_NAMES_COUNT,
                   &NoNames[0],
                   &LookedUpRids,
                   &LookedUpUses
                   );


    if (NtStatus == STATUS_NONE_MAPPED) {
        ASSERT( LookedUpRids == NULL );
        ASSERT( LookedUpUses == NULL );

        printf("Succeeded\n");

    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    }


 
    printf("      Lookup SIDs (all existing)  . . . . . . . . . . . . .     ");

    NtStatus = SamLookupIdsInDomain(
                   DomainHandle,
                   ALL_NAMES_COUNT,
                   &AllRids[0],
                   &LookedUpNames,
                   &LookedUpUses
                   );


    if (NT_SUCCESS(NtStatus)) {
        ASSERT( LookedUpUses  != NULL );
        ASSERT( LookedUpNames != NULL );
        ASSERT( LookedUpNames[0].Buffer != NULL );
        ASSERT( LookedUpNames[1].Buffer != NULL );
        ASSERT( LookedUpNames[2].Buffer != NULL );

        if (
            (LookedUpUses[0] == AllUses[0]) &&
            (LookedUpUses[1] == AllUses[1]) &&
            (LookedUpUses[2] == AllUses[2]) &&
            !RtlCompareString( (PSTRING)&LookedUpNames[0], (PSTRING)&AllNames[0], TRUE ) &&
            !RtlCompareString( (PSTRING)&LookedUpNames[1], (PSTRING)&AllNames[1], TRUE ) &&
            !RtlCompareString( (PSTRING)&LookedUpNames[2], (PSTRING)&AllNames[2], TRUE )
            ) {


            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Names or Uses dont match expected values.\n");
            printf("        Expected Name[0]:  %wZ\n", &AllNames[0] );
            printf("        Received Name[0]:  %wZ\n", &LookedUpNames[0] );
            printf("        Expected Name[1]:  %wZ\n", &AllNames[1] );
            printf("        Received Name[1]:  %wZ\n", &LookedUpNames[1] );
            printf("        Expected Name[2]:  %wZ\n", &AllNames[2] );
            printf("        Received Name[2]:  %wZ\n", &LookedUpNames[2] );

            printf("        Expected Uses:  0x%lx, 0x%lx, 0x%lx\n",
                AllUses[0], AllUses[1], AllUses[2]);
            printf("        Received Uses:  0x%lx, 0x%lx, 0x%lx\n",
                LookedUpUses[0], LookedUpUses[1], LookedUpUses[2]);
            TestStatus = FALSE;
        }


        SamFreeMemory( LookedUpUses );
        SamFreeMemory( LookedUpNames );

    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    }




    printf("      Lookup SIDs (Some existing) . . . . . . . . . . . . .     ");

    NtStatus = SamLookupIdsInDomain(
                   DomainHandle,
                   SOME_NAMES_COUNT,
                   &SomeRids[0],
                   &LookedUpNames,
                   &LookedUpUses
                   );


    if (NtStatus == STATUS_SOME_NOT_MAPPED) {
        ASSERT( LookedUpUses  != NULL );
        ASSERT( LookedUpNames != NULL );
        ASSERT( LookedUpNames[0].Buffer != NULL );
        ASSERT( LookedUpNames[1].Buffer != NULL );
        ASSERT( LookedUpNames[2].Buffer == NULL );  // Unknown
        ASSERT( LookedUpNames[3].Buffer == NULL );  // Unknown
        ASSERT( LookedUpNames[4].Buffer == NULL );  // Unknown
        ASSERT( LookedUpNames[5].Buffer != NULL );
        ASSERT( LookedUpNames[6].Buffer == NULL );  // Unknown

        if (
            (LookedUpUses[0] == SomeUses[0]) &&
            (LookedUpUses[1] == SomeUses[1]) &&
            (LookedUpUses[2] == SomeUses[2]) &&
            !RtlCompareString( (PSTRING)&LookedUpNames[0], (PSTRING)&SomeNames[0], TRUE ) &&
            !RtlCompareString( (PSTRING)&LookedUpNames[1], (PSTRING)&SomeNames[1], TRUE ) &&
            !RtlCompareString( (PSTRING)&LookedUpNames[5], (PSTRING)&SomeNames[5], TRUE )
            ) {


            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Names or Uses dont match expected values.\n");
            printf("        Expected Name[0]:  %wZ\n", &SomeNames[0] );
            printf("        Received Name[0]:  %wZ\n", &LookedUpNames[0] );
            printf("        Expected Name[1]:  %wZ\n", &SomeNames[1] );
            printf("        Received Name[1]:  %wZ\n", &LookedUpNames[1] );
            printf("                 Name[2]:  (Unknown)\n");
            printf("                 Name[3]:  (Unknown)\n");
            printf("                 Name[4]:  (Unknown)\n");
            printf("        Expected Name[5]:  %wZ\n", &SomeNames[5] );
            printf("        Received Name[5]:  %wZ\n", &LookedUpNames[5] );
            printf("                 Name[6]:  (Unknown)\n");

            printf("        Expected Uses:  0x%lx, 0x%lx, 0x%lx, 0x%lx,  0x%lx, 0x%lx, 0x%lx\n",
                SomeUses[0], SomeUses[1], SomeUses[2], SomeUses[3], SomeUses[4], SomeUses[5], SomeUses[6]);
            printf("        Received Uses:  0x%lx, 0x%lx, 0x%lx, 0x%lx,  0x%lx, 0x%lx, 0x%lx\n",
                LookedUpUses[0], LookedUpUses[1], LookedUpUses[2], LookedUpUses[3], LookedUpUses[4], LookedUpUses[5], LookedUpUses[2]);
            TestStatus = FALSE;
        }


        SamFreeMemory( LookedUpUses );
        SamFreeMemory( LookedUpNames );

    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    }




    printf("      Lookup SIDs (None existing) . . . . . . . . . . . . .     ");

    NtStatus = SamLookupIdsInDomain(
                   DomainHandle,
                   NO_NAMES_COUNT,
                   &NoRids[0],
                   &LookedUpNames,
                   &LookedUpUses
                   );


    if (NtStatus == STATUS_NONE_MAPPED) {
        ASSERT( LookedUpUses  == NULL );
        ASSERT( LookedUpNames == NULL );

        printf("Succeeded\n");

    } else {
        printf("Failed\n");
        printf("        Completion status is 0x%lx\n", NtStatus);
        TestStatus = FALSE;
    }







    return TestStatus;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Group  Object Test Suite                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


BOOLEAN
GroupTestSuite(
    HANDLE DomainHandle,
    ULONG  Pass
    )

{
    NTSTATUS            NtStatus, IgnoreStatus;
    HANDLE              GroupHandle1, GroupHandle2, UserHandle1;
    ULONG               CountReturned, NameLength, i, MemberCount;
    ULONG               UserRid, GroupRid;
    PVOID               Buffer, Buffer1, Buffer2;
    SAM_ENUMERATE_HANDLE EnumerationContext;
    PULONG              Members, Attributes;
    PSID_NAME_USE       LookedUpUses;
    PULONG              LookedUpRids;
    UNICODE_STRING      AccountNames[10], AccountName;
    STRING              AccountNameAnsi;

    BOOLEAN             IndividualTestSucceeded, DeleteUser;
    BOOLEAN             TestStatus = TRUE;


    if (Pass == 1) {
        //
        // This test suite assumes that lookup and enumeration API funciton
        // properly.
        //

        printf("\n");
        printf("\n");
        printf("  Group (Pass #1) . . . . . . . . . . . . . . . . . . .   Test\n");

        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Open Group Suite                                                      //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("    Open Group  . . . . . . . . . . . . . . . . . . . . .   Suite\n");
        printf("      Open Groups . . . . . . . . . . . . . . . . . . . . .     ");
        IndividualTestSucceeded = TRUE;
        EnumerationContext = 0;
        NtStatus = SamEnumerateGroupsInDomain(
                       DomainHandle,
                       &EnumerationContext,
                       &Buffer,
                       12000,                   // PreferedMaximumLength
                       &CountReturned
                       );

        TST_SUCCESS_ASSERT(NtStatus);
        ASSERT(Buffer != NULL);
        ASSERT(CountReturned > 0);

        for (i=0; i<CountReturned; i++) {

            NtStatus = SamOpenGroup(
                           DomainHandle,
                           GROUP_ALL_ACCESS,
                           ((PSAM_RID_ENUMERATION)(Buffer))[i].RelativeId,
                           &GroupHandle1
                           );

            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SamOpenGroup(
                               DomainHandle,
                               GENERIC_READ,
                               ((PSAM_RID_ENUMERATION)(Buffer))[i].RelativeId,
                               &GroupHandle2
                               );

                if (NT_SUCCESS(NtStatus)) {
                    IgnoreStatus = SamCloseHandle( GroupHandle2 );
                    ASSERT( NT_SUCCESS(IgnoreStatus) );
                } else {
                    printf("Failed\n");
                    printf("        Completion status is 0x%lx\n", NtStatus);
                    printf("        Failed opening group second time.\n");
                    printf("        Rid of account is:   0x%lx\n",
                        ((PSAM_RID_ENUMERATION)(Buffer))[i].RelativeId);
                    printf("        Name of account is:  %wZ\n",
                        &((PSAM_RID_ENUMERATION)(Buffer))[i].Name );
                    TestStatus = FALSE;
                    IndividualTestSucceeded = FALSE;
                }

                IgnoreStatus = SamCloseHandle( GroupHandle1 );
                ASSERT( NT_SUCCESS(IgnoreStatus) );

            } else {

                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                printf("        Failed opening group for first time.\n");
                printf("        Rid of account is:   0x%lx\n",
                    ((PSAM_RID_ENUMERATION)(Buffer))[i].RelativeId);
                printf("        Name of account is:  %wZ\n",
                    &((PSAM_RID_ENUMERATION)(Buffer))[i].Name );
                TestStatus = FALSE;
                IndividualTestSucceeded = FALSE;
            }

            if (!IndividualTestSucceeded) {
                printf("                                                                ");
            }
        }


        SamFreeMemory( Buffer );
        if (IndividualTestSucceeded) {
            printf("Succeeded\n");
        }



        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Query     Group Suite                                                 //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("\n");
        printf("    Query Group . . . . . . . . . . . . . . . . . . . . .   Suite\n");

        printf("      Query Group General Information . . . . . . . . . . .     ");


        NtStatus = SamOpenGroup(
                       DomainHandle,
                       GROUP_READ_INFORMATION,
                       DOMAIN_GROUP_RID_USERS,
                       &GroupHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationGroup(
                       GroupHandle1,
                       GroupGeneralInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                if ( (((GROUP_GENERAL_INFORMATION *)Buffer)->Name.MaximumLength > 0) &&
                     (((GROUP_GENERAL_INFORMATION *)Buffer)->Name.Buffer != NULL)  ) {

                    printf("Succeeded\n");

                    printf("        Member Count is:  0x%lx\n",
                     (((GROUP_GENERAL_INFORMATION *)Buffer)->MemberCount) );
                    printf("        Attributes are:   0x%lx\n",
                     (((GROUP_GENERAL_INFORMATION *)Buffer)->Attributes) );
                    printf("        Group Name is:    %wZ\n",
                     &(((GROUP_GENERAL_INFORMATION *)Buffer)->Name) );



                } else {
                    printf("Failed\n");
                    printf("        Group Name not returned.\n");
                    TestStatus = FALSE;
                }
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( GroupHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );




        printf("      Query Group Name Information  . . . . . . . . . . . .     ");


        NtStatus = SamOpenGroup(
                       DomainHandle,
                       GROUP_READ_INFORMATION,
                       DOMAIN_GROUP_RID_USERS,
                       &GroupHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationGroup(
                       GroupHandle1,
                       GroupNameInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                if ( (((GROUP_NAME_INFORMATION *)Buffer)->Name.MaximumLength > 0) &&
                     (((GROUP_NAME_INFORMATION *)Buffer)->Name.Buffer != NULL)  ) {

                    printf("Succeeded\n");

                    printf("        Group Name is:    %wZ\n",
                     &(((GROUP_NAME_INFORMATION *)Buffer)->Name) );



                } else {
                    printf("Failed\n");
                    printf("        Group Name not returned.\n");
                    TestStatus = FALSE;
                }
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( GroupHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );




        printf("      Query Group Admin Comment Information . . . . . . . .     ");


        NtStatus = SamOpenGroup(
                       DomainHandle,
                       GROUP_READ_INFORMATION,
                       DOMAIN_GROUP_RID_USERS,
                       &GroupHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationGroup(
                       GroupHandle1,
                       GroupAdminCommentInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                if ( (((GROUP_ADM_COMMENT_INFORMATION *)Buffer)->AdminComment.MaximumLength >= 0) ) {

                    printf("Succeeded\n");

                    printf("        Group Admin Comment is:    %wZ\n",
                     &(((GROUP_ADM_COMMENT_INFORMATION *)Buffer)->AdminComment) );



                } else {
                    printf("Failed\n");
                    printf("        Group Admin Comment not returned.\n");
                    TestStatus = FALSE;
                }
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( GroupHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );




        printf("      Query Group Attribute Information . . . . . . . . . .     ");


        NtStatus = SamOpenGroup(
                       DomainHandle,
                       GROUP_READ_INFORMATION,
                       DOMAIN_GROUP_RID_USERS,
                       &GroupHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationGroup(
                       GroupHandle1,
                       GroupAttributeInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {


                printf("Succeeded\n");

                printf("        Attributes are:   0x%lx\n",
                 (((GROUP_ATTRIBUTE_INFORMATION *)Buffer)->Attributes) );


                SamFreeMemory( Buffer );

            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( GroupHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );




        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Get Members Of Group Suite                                            //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("\n");
        printf("    Get Members . . . . . . . . . . . . . . . . . . . . .   Suite\n");

        printf("      Get Members of Well-Known Account . . . . . . . . . .     ");

        NtStatus = SamOpenGroup(
                       DomainHandle,
                       GROUP_LIST_MEMBERS,
                       DOMAIN_GROUP_RID_USERS,
                       &GroupHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamGetMembersInGroup(
                       GroupHandle1,
                       &Members,
                       &Attributes,
                       &MemberCount
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Members != NULL || Attributes != NULL) {

                printf("Succeeded\n");


                printf("       Member Count:    %d Users\n", MemberCount);
                for ( i=0; i<MemberCount; i++) {

                    printf("       User[%d] Rid/Attributes:      0x%lx/0x%lx\n",
                        i, Members[i], Attributes[i]);


                }

                SamFreeMemory( Members );
                SamFreeMemory( Attributes );


            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( GroupHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );



        printf("      Get Members of Empty Group. . . . . . . . . . . . . .     ");

        //
        // This group was created earlier in the test
        //

        RtlInitString( &AccountNameAnsi, GROUP_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountNames[0], &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        NtStatus = SamLookupNamesInDomain(
                       DomainHandle,
                       1,
                       &AccountNames[0],
                       &LookedUpRids,
                       &LookedUpUses
                       );
        TST_SUCCESS_ASSERT(NtStatus);
        ASSERT(LookedUpUses[0] == SidTypeGroup);
        RtlFreeUnicodeString( &AccountNames[0] );



        GroupHandle1 = NULL;

        NtStatus = SamOpenGroup( DomainHandle, GROUP_LIST_MEMBERS, LookedUpRids[0], &GroupHandle1 );
        TST_SUCCESS_ASSERT(NtStatus);
        SamFreeMemory( LookedUpUses ); SamFreeMemory( LookedUpRids );

        NtStatus = SamGetMembersInGroup(
                       GroupHandle1,
                       &Members,
                       &Attributes,
                       &MemberCount
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (MemberCount == 0) {

                printf("Succeeded\n");




            } else {
                printf("Failed\n");
                printf("        Buffer addresses  set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                printf("       Member Count:    %d\n", MemberCount);
                for ( i=0; i<MemberCount; i++) {

                    printf("       User[%d] Rid/Attributes:      0x%lx/0x%lx\n",
                        i, Members[i], Attributes[i]);
                }

                SamFreeMemory( Members );
                SamFreeMemory( Attributes );
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( GroupHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );


        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Set Group Suite  (pass 1)                                             //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("\n");
        printf("    Set Group . . . . . . . . . . . . . . . . . . . . . .   Suite\n");


        printf("      Set Attribute . . . . . . . . . . . . . . . . . . . .     ");
        NtStatus = SamOpenGroup(
                       DomainHandle,
                       GROUP_WRITE_ACCOUNT | GROUP_READ_INFORMATION,
                       DOMAIN_GROUP_RID_USERS,
                       &GroupHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer1 = NULL;
        NtStatus = SamQueryInformationGroup(
                       GroupHandle1,
                       GroupAttributeInformation,
                       &Buffer1
                       );
        TST_SUCCESS_ASSERT(NtStatus);
        ASSERT(Buffer1 != NULL);

        //
        // Change the value and write it back
        //

        ((GROUP_ATTRIBUTE_INFORMATION *)Buffer1)->Attributes ^=
            SE_GROUP_ENABLED_BY_DEFAULT;


        NtStatus = SamSetInformationGroup(
                       GroupHandle1,
                       GroupAttributeInformation,
                       Buffer1
                       );
        if (NT_SUCCESS(NtStatus)) {

            //
            // Check the written value to make sure it stuck
            //

            Buffer2 = NULL;
            NtStatus = SamQueryInformationGroup(
                           GroupHandle1,
                           GroupAttributeInformation,
                           &Buffer2
                           );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(Buffer2 != NULL);

            if (((GROUP_ATTRIBUTE_INFORMATION *)Buffer1)->Attributes ==
                ((GROUP_ATTRIBUTE_INFORMATION *)Buffer2)->Attributes   ) {

                printf("Succeeded\n");

                SamFreeMemory( Buffer2 );

            } else {
                printf("Failed\n");
                printf("        Returned Value Doesn't Match Set Value.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        SamFreeMemory( Buffer1 );
        IgnoreStatus = SamCloseHandle( GroupHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );





        printf("      Set Admin Comment . . . . . . . . . . . . . . . . . .     ");

            NtStatus = SamOpenGroup(
                           DomainHandle,
                           GROUP_WRITE_ACCOUNT | GROUP_READ_INFORMATION,
                           DOMAIN_GROUP_RID_USERS,
                           &GroupHandle1
                           );
            ASSERT(NT_SUCCESS(NtStatus) );

            //
            // Get the current value...
            //

            Buffer1 = NULL;
            NtStatus = SamQueryInformationGroup(
                           GroupHandle1,
                           GroupAdminCommentInformation,
                           &Buffer1
                           );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(Buffer1 != NULL);


            //
            // Change the field to a new value and write it out.
            //

            NameLength = ((GROUP_ADM_COMMENT_INFORMATION *)Buffer1)->AdminComment.Length;
            if (  NameLength == DummyString1.Length ) {
                ((GROUP_ADM_COMMENT_INFORMATION *)Buffer1)->AdminComment = DummyString2;
            } else {
                ((GROUP_ADM_COMMENT_INFORMATION *)Buffer1)->AdminComment = DummyString1;
            }

            NtStatus = SamSetInformationGroup(
                           GroupHandle1,
                           GroupAdminCommentInformation,
                           Buffer1
                           );
            if ( NT_SUCCESS(NtStatus) ) {

                //
                // Now check that the change was really made...
                //

                Buffer2 = NULL;
                NtStatus = SamQueryInformationGroup(
                               GroupHandle1,
                               GroupAdminCommentInformation,
                               &Buffer2
                               );
                ASSERT(NT_SUCCESS( NtStatus ) );
                if (
                    !RtlCompareString(
                        (PSTRING)&((GROUP_ADM_COMMENT_INFORMATION *)Buffer1)->AdminComment,
                        (PSTRING)&((GROUP_ADM_COMMENT_INFORMATION *)Buffer2)->AdminComment,
                        TRUE)
                    ) {

                    printf("Succeeded\n");

                } else {

                    printf("Failed\n");
                    printf("        Value queried doesn't match value written\n");
                    printf("        Value Written is   %wZ\n",
                        (PUNICODE_STRING)&((GROUP_ADM_COMMENT_INFORMATION *)Buffer1)->AdminComment);
                    printf("        Value Retrieved is %wZ\n",
                        (PUNICODE_STRING)&((GROUP_ADM_COMMENT_INFORMATION *)Buffer2)->AdminComment);

                    TestStatus = FALSE;

                }

                SamFreeMemory( Buffer1 );
                SamFreeMemory( Buffer2 );

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
                SamFreeMemory( Buffer1 );

            }






    } // END PASS #1
    if (Pass == 2) {

        printf("\n");
        printf("\n");
        printf("  Group (Pass #2) . . . . . . . . . . . . . . . . . . .   Test\n");

        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Delete Group Suite                                                    //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("\n");
        printf("    Delete Group  . . . . . . . . . . . . . . . . . . . .   Suite\n");

        printf("      Delete Normal Group . . . . . . . . . . . . . . . . .     ");

        //
        // This group was created in pass #1
        //

        RtlInitString( &AccountNameAnsi, GROUP_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountNames[0], &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        NtStatus = SamLookupNamesInDomain(
                       DomainHandle,
                       1,
                       &AccountNames[0],
                       &LookedUpRids,
                       &LookedUpUses
                       );
        TST_SUCCESS_ASSERT(NtStatus);
        ASSERT(LookedUpUses[0] == SidTypeGroup);
        RtlFreeUnicodeString( &AccountNames[0] );



        GroupHandle1 = NULL;

        NtStatus = SamOpenGroup( DomainHandle, DELETE, LookedUpRids[0], &GroupHandle1 );
        TST_SUCCESS_ASSERT(NtStatus);
        SamFreeMemory( LookedUpUses ); SamFreeMemory( LookedUpRids );

        NtStatus = SamDeleteGroup( GroupHandle1 );
        if (NT_SUCCESS(NtStatus)) {
            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }




        printf("      Delete Well Known Group . . . . . . . . . . . . . . .     ");

        GroupHandle1 = NULL;

        NtStatus = SamOpenGroup( DomainHandle, DELETE, DOMAIN_GROUP_RID_USERS, &GroupHandle1 );
        TST_SUCCESS_ASSERT(NtStatus);

        NtStatus = SamDeleteGroup( GroupHandle1 );
        if (NtStatus == STATUS_SPECIAL_ACCOUNT) {

            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }

        NtStatus = SamCloseHandle( GroupHandle1 );
        ASSERT(NT_SUCCESS(NtStatus));








        printf("      Delete Primary Group Of A User. . . . . . . . . . . .     ");

        //
        // Make a user (might already exist)
        // Make a group
        // Make the group the user's primary group
        // Attempt to delete the group
        // Change the user so the group isn't the primary group
        // delete the group
        // If we created the user, delete it.

        //
        // The following user might already exist (from earlier in the test)
        //

        RtlInitString( &AccountNameAnsi, USER_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        //InitializeObjectAttributes( &ObjectAttributes, &AccountName, 0, 0, NULL );


        UserRid = 0;
        UserHandle1 = NULL;
        NtStatus = SamCreateUserInDomain(
                       DomainHandle,
                       &AccountName,
                       USER_ALL_ACCESS,
                       &UserHandle1,
                       &UserRid
                       );
        RtlFreeUnicodeString( &AccountName );
        DeleteUser = TRUE;
        if (NtStatus == STATUS_USER_EXISTS) {
            DeleteUser = FALSE;
            RtlInitString( &AccountNameAnsi, USER_NAME1 );
            NtStatus = RtlAnsiStringToUnicodeString( &AccountNames[0], &AccountNameAnsi, TRUE );
            TST_SUCCESS_ASSERT(NtStatus);

            NtStatus = SamLookupNamesInDomain(
                           DomainHandle,
                           1,
                           &AccountNames[0],
                           &LookedUpRids,
                           &LookedUpUses
                           );
            RtlFreeUnicodeString( &AccountNames[0] );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(LookedUpUses[0] == SidTypeUser);
            UserRid = LookedUpRids[0];
            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_ALL_ACCESS,
                           UserRid,
                           &UserHandle1);
            SamFreeMemory( LookedUpUses ); SamFreeMemory( LookedUpRids );
        }

        ASSERT(NT_SUCCESS(NtStatus));

        //
        // create the group
        //

        RtlInitString( &AccountNameAnsi, GROUP_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        //InitializeObjectAttributes( &ObjectAttributes, &AccountName, 0, 0, NULL );

        GroupRid = 0;
        GroupHandle1 = NULL;
        NtStatus = SamCreateGroupInDomain(
                       DomainHandle,
                       &AccountName,
                       GROUP_ALL_ACCESS,
                       &GroupHandle1,
                       &GroupRid
                       );
        RtlFreeUnicodeString( &AccountName );
        ASSERT(NT_SUCCESS(NtStatus));

        //
        // Make the user a member of this group
        //

        NtStatus = SamAddMemberToGroup(
                       GroupHandle1,
                       UserRid,
                       SE_GROUP_MANDATORY              |
                           SE_GROUP_ENABLED_BY_DEFAULT |
                           SE_GROUP_ENABLED
                       );
        ASSERT(NT_SUCCESS(NtStatus));




        //
        // Now try to delete the group
        //

        NtStatus = SamDeleteGroup( GroupHandle1 );
        if (NtStatus == STATUS_MEMBER_IN_GROUP) {

            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }


        //
        // Now get rid of the group and possibly the user account
        //

        NtStatus = SamRemoveMemberFromGroup(GroupHandle1, UserRid);
        ASSERT(NT_SUCCESS(NtStatus));


        NtStatus = SamDeleteGroup( GroupHandle1 );
        ASSERT(NT_SUCCESS(NtStatus));

        if (DeleteUser == TRUE) {
            NtStatus = SamDeleteUser( UserHandle1 );
            ASSERT(NT_SUCCESS(NtStatus));
        } else {
            NtStatus = SamCloseHandle( UserHandle1 );
            ASSERT(NT_SUCCESS(NtStatus));
        }



        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Add/Remove Member Suite                                               //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("\n");
        printf("    Add/Remove Member Suite . . . . . . . . . . . . . . .   Suite\n");

        printf("      Add Member  . . . . . . . . . . . . . . . . . . . . .     ");

        //
        // This test sets things up for the next test
        //

        //
        // The following user might already exist (from earlier in the test)
        //

        RtlInitString( &AccountNameAnsi, USER_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        //InitializeObjectAttributes( &ObjectAttributes, &AccountName, 0, 0, NULL );

        UserRid = 0;
        UserHandle1 = NULL;
        NtStatus = SamCreateUserInDomain(
                       DomainHandle,
                       &AccountName,
                       USER_ALL_ACCESS,
                       &UserHandle1,
                       &UserRid
                       );
        RtlFreeUnicodeString( &AccountName );
        DeleteUser = TRUE;
        if (NtStatus == STATUS_USER_EXISTS) {
            DeleteUser = FALSE;
            RtlInitString( &AccountNameAnsi, USER_NAME1 );
            NtStatus = RtlAnsiStringToUnicodeString( &AccountNames[0], &AccountNameAnsi, TRUE );
            TST_SUCCESS_ASSERT(NtStatus);

            NtStatus = SamLookupNamesInDomain(
                           DomainHandle,
                           1,
                           &AccountNames[0],
                           &LookedUpRids,
                           &LookedUpUses
                           );
            RtlFreeUnicodeString( &AccountNames[0] );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(LookedUpUses[0] == SidTypeUser);
            UserRid = LookedUpRids[0];
            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_ALL_ACCESS,
                           UserRid,
                           &UserHandle1);
            SamFreeMemory( LookedUpUses ); SamFreeMemory( LookedUpRids );
        }

        ASSERT(NT_SUCCESS(NtStatus));


        //
        // create the group
        //

        RtlInitString( &AccountNameAnsi, GROUP_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        //InitializeObjectAttributes( &ObjectAttributes, &AccountName, 0, 0, NULL );

        GroupRid = 0;
        GroupHandle1 = NULL;
        NtStatus = SamCreateGroupInDomain(
                       DomainHandle,
                       &AccountName,
                       GROUP_ALL_ACCESS,
                       &GroupHandle1,
                       &GroupRid
                       );
        RtlFreeUnicodeString( &AccountName );
        ASSERT(NT_SUCCESS(NtStatus));

        //
        // Make the user a member of this group
        //

        NtStatus = SamAddMemberToGroup(
                       GroupHandle1,
                       UserRid,
                       SE_GROUP_MANDATORY              |
                           SE_GROUP_ENABLED_BY_DEFAULT |
                           SE_GROUP_ENABLED
                       );

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SamGetMembersInGroup(
                           GroupHandle1,
                           &Members,
                           &Attributes,
                           &MemberCount
                           );
            ASSERT(NT_SUCCESS(NtStatus));

            NtStatus = STATUS_MEMBER_NOT_IN_GROUP;
            for ( i=0; i<MemberCount; i++) {
                if (Members[i] == UserRid) {
                    NtStatus = STATUS_SUCCESS;
                    break;
                }
            }

            if (NT_SUCCESS(NtStatus)) {
                if (Attributes[i] == SE_GROUP_MANDATORY          |
                                           SE_GROUP_ENABLED_BY_DEFAULT |
                                           SE_GROUP_ENABLED) {
                    printf("Succeeded\n");
                } else {
                    printf("Failed\n");
                    printf("Member Added but attributes don't match expected value.\n");
                    printf("Expected value:  0x%lx\n",(SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED));
                    printf("Retrieved value:  0x%lx\n",Attributes[i]);
                    TestStatus = FALSE;
                }
            } else {
                printf("Failed\n");
                printf("Service returned SUCCESS, but user not in member list for group.\n");
                TestStatus = FALSE;
            }


            if (Members != NULL) {
                SamFreeMemory( Members );
                SamFreeMemory( Attributes );
            }


        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }




        printf("      Remove Member . . . . . . . . . . . . . . . . . . . .     ");

        //
        // The previous test sets this one up.
        //

        //
        // Now try to remove the user from the group
        //

        NtStatus = SamRemoveMemberFromGroup(GroupHandle1, UserRid);
        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SamGetMembersInGroup(
                           GroupHandle1,
                           &Members,
                           &Attributes,
                           &MemberCount
                           );
            ASSERT(NT_SUCCESS(NtStatus));

            for ( i=0; i<MemberCount; i++) {
                if (Members[i] == UserRid) {
                    NtStatus = STATUS_MEMBER_IN_GROUP;
                    break;
                }
            }

            if (NT_SUCCESS(NtStatus)) {
                printf("Succeeded\n");
            } else {
                printf("Failed\n");
                printf("Service returned SUCCESS, but user still in member list for group.\n");
                TestStatus = FALSE;
            }


            SamFreeMemory( Members );
            SamFreeMemory( Attributes );

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }


        //
        // and clean up the user and group accounts
        //

        NtStatus = SamDeleteGroup( GroupHandle1 );
        ASSERT(NT_SUCCESS(NtStatus));

        if (DeleteUser == TRUE) {
            NtStatus = SamDeleteUser( UserHandle1 );
            ASSERT(NT_SUCCESS(NtStatus));
        } else {
            NtStatus = SamCloseHandle( UserHandle1 );
            ASSERT(NT_SUCCESS(NtStatus));
        }






        printf("      Add Non-Existant Member . . . . . . . . . . . . . . .     ");

        //
        // create the group
        //

        RtlInitString( &AccountNameAnsi, GROUP_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        //InitializeObjectAttributes( &ObjectAttributes, &AccountName, 0, 0, NULL );

        GroupRid = 0;
        GroupHandle1 = NULL;
        NtStatus = SamCreateGroupInDomain(
                       DomainHandle,
                       &AccountName,
                       GROUP_ALL_ACCESS,
                       &GroupHandle1,
                       &GroupRid
                       );
        RtlFreeUnicodeString( &AccountName );
        ASSERT(NT_SUCCESS(NtStatus));

        //
        // Specify a non-existant user be added to this group
        //

        UserRid = 30732579;             // Pretty sure this user doesn't exist.
        NtStatus = SamAddMemberToGroup(
                       GroupHandle1,
                       UserRid,
                       SE_GROUP_MANDATORY              |
                           SE_GROUP_ENABLED_BY_DEFAULT |
                           SE_GROUP_ENABLED
                       );

        if (NtStatus == STATUS_NO_SUCH_USER) {

            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }


        NtStatus = SamDeleteGroup( GroupHandle1 );
        ASSERT(NT_SUCCESS(NtStatus));




        printf("      Remove Non-existant Member  . . . . . . . . . . . . .     ");

        //
        // create the group
        //

        RtlInitString( &AccountNameAnsi, GROUP_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        //InitializeObjectAttributes( &ObjectAttributes, &AccountName, 0, 0, NULL );

        GroupRid = 0;
        GroupHandle1 = NULL;
        NtStatus = SamCreateGroupInDomain(
                       DomainHandle,
                       &AccountName,
                       GROUP_ALL_ACCESS,
                       &GroupHandle1,
                       &GroupRid
                       );
        RtlFreeUnicodeString( &AccountName );
        ASSERT(NT_SUCCESS(NtStatus));

        //
        // Specify a non-existant user be removed from this group
        //

        UserRid = 30732579;             // Pretty sure this user doesn't exist.
        NtStatus = SamRemoveMemberFromGroup( GroupHandle1, UserRid );

        if (NtStatus == STATUS_NO_SUCH_USER) {

            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }


        NtStatus = SamDeleteGroup( GroupHandle1 );
        ASSERT(NT_SUCCESS(NtStatus));




        printf("      Remove Primary Group Of Member  . . . . . . . . . . .     ");


        //
        // Make a user (might already exist)
        // Make a group
        // Make the group the user's primary group
        // Attempt to remove the group (should fail)
        // Change the user so the group isn't the primary group
        // remove the group
        // delete the group
        // If we created the user, delete it.

        //
        // The following user might already exist (from earlier in the test)
        //

        RtlInitString( &AccountNameAnsi, USER_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        //InitializeObjectAttributes( &ObjectAttributes, &AccountName, 0, 0, NULL );

        UserRid = 0;
        UserHandle1 = NULL;
        NtStatus = SamCreateUserInDomain(
                       DomainHandle,
                       &AccountName,
                       USER_ALL_ACCESS,
                       &UserHandle1,
                       &UserRid
                       );
        RtlFreeUnicodeString( &AccountName );
        DeleteUser = TRUE;
        if (NtStatus == STATUS_USER_EXISTS) {
            DeleteUser = FALSE;
            RtlInitString( &AccountNameAnsi, USER_NAME1 );
            NtStatus = RtlAnsiStringToUnicodeString( &AccountNames[0], &AccountNameAnsi, TRUE );
            TST_SUCCESS_ASSERT(NtStatus);

            NtStatus = SamLookupNamesInDomain(
                           DomainHandle,
                           1,
                           &AccountNames[0],
                           &LookedUpRids,
                           &LookedUpUses
                           );
            RtlFreeUnicodeString( &AccountNames[0] );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(LookedUpUses[0] == SidTypeUser);
            UserRid = LookedUpRids[0];
            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_ALL_ACCESS,
                           UserRid,
                           &UserHandle1);
            SamFreeMemory( LookedUpUses ); SamFreeMemory( LookedUpRids );
        }

        ASSERT(NT_SUCCESS(NtStatus));


        //
        // create the group
        //

        RtlInitString( &AccountNameAnsi, GROUP_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        //InitializeObjectAttributes( &ObjectAttributes, &AccountName, 0, 0, NULL );

        GroupRid = 0;
        GroupHandle1 = NULL;
        NtStatus = SamCreateGroupInDomain(
                       DomainHandle,
                       &AccountName,
                       GROUP_ALL_ACCESS,
                       &GroupHandle1,
                       &GroupRid
                       );
        RtlFreeUnicodeString( &AccountName );
        ASSERT(NT_SUCCESS(NtStatus));

        //
        // Make the user a member of this group
        //

        NtStatus = SamAddMemberToGroup(
                       GroupHandle1,
                       UserRid,
                       SE_GROUP_MANDATORY              |
                           SE_GROUP_ENABLED_BY_DEFAULT |
                           SE_GROUP_ENABLED
                       );
        ASSERT(NT_SUCCESS(NtStatus));


        //
        // Set the user's primary group Id to be this group
        //

        NtStatus = SamSetInformationUser(
                       UserHandle1,
                       UserPrimaryGroupInformation,
                       &GroupRid
                       );
        ASSERT(NT_SUCCESS(NtStatus));



        //
        // Now try to remove the user from the group
        //

        NtStatus = SamRemoveMemberFromGroup(GroupHandle1, UserRid);
        if (NtStatus == STATUS_MEMBERS_PRIMARY_GROUP) {

            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }


        //
        // Set the user's primary group Id back and remove the user
        // from the group
        //

        GroupRid = DOMAIN_GROUP_RID_USERS;
        NtStatus = SamSetInformationUser(
                       UserHandle1,
                       UserPrimaryGroupInformation,
                       &GroupRid
                       );
        ASSERT(NT_SUCCESS(NtStatus));
        NtStatus = SamRemoveMemberFromGroup(GroupHandle1, UserRid);
        ASSERT(NT_SUCCESS(NtStatus));



        //
        // Now get rid of the group and possibly the user account
        //


        NtStatus = SamDeleteGroup( GroupHandle1 );
        ASSERT(NT_SUCCESS(NtStatus));

        if (DeleteUser == TRUE) {
            NtStatus = SamDeleteUser( UserHandle1 );
            ASSERT(NT_SUCCESS(NtStatus));
        } else {
            NtStatus = SamCloseHandle( UserHandle1 );
            ASSERT(NT_SUCCESS(NtStatus));
        }






        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Set Group Suite  (pass 2)                                             //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("\n");
        printf("    Set Group . . . . . . . . . . . . . . . . . . . . . .   Suite\n");


        printf("      Set Name  . . . . . . . . . . . . . . . . . . . . . .     ");
        printf("(Untested)\n");


        printf("      Set Name Of Well-Known Account  . . . . . . . . . . .     ");
        printf("(Untested)\n");

    }

    return(TestStatus);

}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Alias  Object Test Suite                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


BOOLEAN
AliasTestSuite(
    HANDLE DomainHandle,
    HANDLE BuiltinDomainHandle,
    PSID   DomainSid,
    ULONG  Pass
    )

{
    NTSTATUS            NtStatus, IgnoreStatus;
    HANDLE              AdminAliasHandle, AliasHandle1, AliasHandle2, UserHandle1, UserHandle2, UserHandle3;
    ULONG               CountReturned, i, MemberCount;
    ULONG               UserRid, UserRid2, UserRid3, AliasRid, AliasRid2;
    PVOID               Buffer, Buffer1, Buffer2;
    ULONG               NameLength;
    SAM_ENUMERATE_HANDLE EnumerationContext;
    PULONG              Members;
    PSID                *AliasMembers;
    PSID_NAME_USE       LookedUpUses;
    PULONG              LookedUpRids;
    UNICODE_STRING      AccountNames[10], AccountName;
    STRING              AccountNameAnsi;
    PSID                UserSid1, UserSid2, GroupSid;

    BOOLEAN             IndividualTestSucceeded, DeleteUser;
    BOOLEAN             TestStatus = TRUE;


    if (Pass == 1) {
        //
        // This test suite assumes that lookup and enumeration API funciton
        // properly.
        //

        printf("\n");
        printf("\n");
        printf("  Alias (Pass #1) . . . . . . . . . . . . . . . . . . .   Test\n");

        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Open Alias Suite                                                      //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("    Open Alias  . . . . . . . . . . . . . . . . . . . . .   Suite\n");
        printf("      Open Aliases . . . . . . . . . . . . . . . . . . . . .     ");
        IndividualTestSucceeded = TRUE;
        EnumerationContext = 0;
        NtStatus = SamEnumerateAliasesInDomain(
                       DomainHandle,
                       &EnumerationContext,
                       &Buffer,
                       12000,                   // PreferedMaximumLength
                       &CountReturned
                       );

        TST_SUCCESS_ASSERT(NtStatus);
        ASSERT(Buffer != NULL);
        ASSERT(CountReturned > 0);

        for (i=0; i<CountReturned; i++) {

            NtStatus = SamOpenAlias(
                           DomainHandle,
                           ALIAS_ALL_ACCESS,
                           ((PSAM_RID_ENUMERATION)(Buffer))[i].RelativeId,
                           &AliasHandle1
                           );

            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SamOpenAlias(
                               DomainHandle,
                               GENERIC_READ,
                               ((PSAM_RID_ENUMERATION)(Buffer))[i].RelativeId,
                               &AliasHandle2
                               );

                if (NT_SUCCESS(NtStatus)) {
                    IgnoreStatus = SamCloseHandle( AliasHandle2 );
                    ASSERT( NT_SUCCESS(IgnoreStatus) );
                } else {
                    printf("Failed\n");
                    printf("        Completion status is 0x%lx\n", NtStatus);
                    printf("        Failed opening alias second time.\n");
                    printf("        Rid of account is:   0x%lx\n",
                        ((PSAM_RID_ENUMERATION)(Buffer))[i].RelativeId);
                    printf("        Name of account is:  %wZ\n",
                        &((PSAM_RID_ENUMERATION)(Buffer))[i].Name );
                    TestStatus = FALSE;
                    IndividualTestSucceeded = FALSE;
                }

                IgnoreStatus = SamCloseHandle( AliasHandle1 );
                ASSERT( NT_SUCCESS(IgnoreStatus) );

            } else {

                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                printf("        Failed opening alias for first time.\n");
                printf("        Rid of account is:   0x%lx\n",
                    ((PSAM_RID_ENUMERATION)(Buffer))[i].RelativeId);
                printf("        Name of account is:  %wZ\n",
                    &((PSAM_RID_ENUMERATION)(Buffer))[i].Name );
                TestStatus = FALSE;
                IndividualTestSucceeded = FALSE;
            }

            if (!IndividualTestSucceeded) {
                printf("                                                                ");
            }
        }


        SamFreeMemory( Buffer );
        if (IndividualTestSucceeded) {
            printf("Succeeded\n");
        }



        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Query     Alias Suite                                                 //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////


        //
        // Get the rid of an alias created earlier in the test
        //

        RtlInitString( &AccountNameAnsi, ALIAS_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountNames[0], &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        NtStatus = SamLookupNamesInDomain(
                       DomainHandle,
                       1,
                       &AccountNames[0],
                       &LookedUpRids,
                       &LookedUpUses
                       );
        TST_SUCCESS_ASSERT(NtStatus);
        ASSERT(LookedUpUses[0] == SidTypeAlias);
        RtlFreeUnicodeString( &AccountNames[0] );

        AliasRid = LookedUpRids[0];

        SamFreeMemory( LookedUpUses ); SamFreeMemory( LookedUpRids );


        printf("\n");
        printf("    Query Alias . . . . . . . . . . . . . . . . . . . . .   Suite\n");

        printf("      Query Alias General Information . . . . . . . . . . .     ");


        NtStatus = SamOpenAlias(
                       DomainHandle,
                       ALIAS_READ_INFORMATION,
                       AliasRid,
                       &AliasHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationAlias(
                       AliasHandle1,
                       AliasGeneralInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                if ( (((ALIAS_GENERAL_INFORMATION *)Buffer)->Name.MaximumLength > 0) &&
                     (((ALIAS_GENERAL_INFORMATION *)Buffer)->Name.Buffer != NULL)  ) {

                    printf("Succeeded\n");

                    printf("        Member Count is:  0x%lx\n",
                     (((ALIAS_GENERAL_INFORMATION *)Buffer)->MemberCount) );
                    printf("        Alias Name is:    %wZ\n",
                     &(((ALIAS_GENERAL_INFORMATION *)Buffer)->Name) );

                } else {
                    printf("Failed\n");
                    printf("        Alias Name not returned.\n");
                    TestStatus = FALSE;
                }
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( AliasHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );




        printf("      Query Alias Name Information  . . . . . . . . . . . .     ");


        NtStatus = SamOpenAlias(
                       DomainHandle,
                       ALIAS_READ_INFORMATION,
                       AliasRid,
                       &AliasHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationAlias(
                       AliasHandle1,
                       AliasNameInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                if ( (((ALIAS_NAME_INFORMATION *)Buffer)->Name.MaximumLength > 0) &&
                     (((ALIAS_NAME_INFORMATION *)Buffer)->Name.Buffer != NULL)  ) {

                    printf("Succeeded\n");

                    printf("        Alias Name is:    %wZ\n",
                     &(((ALIAS_NAME_INFORMATION *)Buffer)->Name) );
                } else {
                    printf("Failed\n");
                    printf("        Alias Name not returned.\n");
                    TestStatus = FALSE;
                }
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( AliasHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );




        printf("      Query Alias Admin Comment Information . . . . . . . .     ");


        NtStatus = SamOpenAlias(
                       DomainHandle,
                       ALIAS_READ_INFORMATION,
                       AliasRid,
                       &AliasHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationAlias(
                       AliasHandle1,
                       AliasAdminCommentInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                if ( (((ALIAS_ADM_COMMENT_INFORMATION *)Buffer)->AdminComment.MaximumLength >= 0) ) {

                    printf("Succeeded\n");

                    printf("        Alias Admin Comment is:    %wZ\n",
                     &(((ALIAS_ADM_COMMENT_INFORMATION *)Buffer)->AdminComment) );
                } else {
                    printf("Failed\n");
                    printf("        Alias Admin Comment not returned.\n");
                    TestStatus = FALSE;
                }
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( AliasHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );





        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Get Members Of Alias Suite                                            //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("\n");
        printf("    Get Members . . . . . . . . . . . . . . . . . . . . .   Suite\n");

#ifdef LATER // ALIAS_LATER - well-know aliases ?


        davidc/chads - this needs to access the builtin domain.

        printf("      Get Members of Well-Known Account . . . . . . . . . .     ");

        NtStatus = SamOpenAlias(
                       DomainHandle,
                       ALIAS_LIST_MEMBERS,
                       DOMAIN_ALIAS_RID_ADMINS,
                       &AliasHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamGetMembersInAlias(
                       AliasHandle1,
                       &AliasMembers,
                       &Attributes,
                       &MemberCount
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Members != NULL || Attributes != NULL) {

                printf("Succeeded\n");


                printf("       Member Count:    %d Users\n", MemberCount);
                for ( i=0; i<MemberCount; i++) {

                    // printf("       User[%d] Sid:      0x%lx\n",
                    //    i, Members[i]);


                }

                SamFreeMemory( AliasMembers );
                SamFreeMemory( Attributes );


            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( AliasHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );
#endif


        printf("      Get Members of Empty Alias. . . . . . . . . . . . . .     ");

        //
        // This alias was created earlier in the test
        //

        RtlInitString( &AccountNameAnsi, ALIAS_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountNames[0], &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        NtStatus = SamLookupNamesInDomain(
                       DomainHandle,
                       1,
                       &AccountNames[0],
                       &LookedUpRids,
                       &LookedUpUses
                       );
        TST_SUCCESS_ASSERT(NtStatus);
        ASSERT(LookedUpUses[0] == SidTypeAlias);
        RtlFreeUnicodeString( &AccountNames[0] );



        AliasHandle1 = NULL;

        NtStatus = SamOpenAlias( DomainHandle, ALIAS_LIST_MEMBERS, LookedUpRids[0], &AliasHandle1 );
        TST_SUCCESS_ASSERT(NtStatus);
        SamFreeMemory( LookedUpUses ); SamFreeMemory( LookedUpRids );

        NtStatus = SamGetMembersInAlias(
                       AliasHandle1,
                       &AliasMembers,
                       &MemberCount
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (MemberCount == 0) {

                printf("Succeeded\n");




            } else {
                printf("Failed\n");
                printf("       Member Count > 0 :    %d\n", MemberCount);
                for ( i=0; i<MemberCount; i++) {

                    // printf("       User[%d] Rid/Attributes:      0x%lx/0x%lx\n",
                    //    i, Members[i], Attributes[i]);
                }

                SamFreeMemory( AliasMembers );
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( AliasHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );



        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Set Alias Suite  (pass 1)                                             //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("\n");
        printf("    Set Alias . . . . . . . . . . . . . . . . . . . . . .   Suite\n");
        //


        // Get the rid of an alias created earlier in the test
        //

        RtlInitString( &AccountNameAnsi, ALIAS_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountNames[0], &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        NtStatus = SamLookupNamesInDomain(
                       DomainHandle,
                       1,
                       &AccountNames[0],
                       &LookedUpRids,
                       &LookedUpUses
                       );
        TST_SUCCESS_ASSERT(NtStatus);
        ASSERT(LookedUpUses[0] == SidTypeAlias);
        RtlFreeUnicodeString( &AccountNames[0] );

        AliasRid = LookedUpRids[0];

        SamFreeMemory( LookedUpUses ); SamFreeMemory( LookedUpRids );



        printf("      Set Admin Comment . . . . . . . . . . . . . . . . . .     ");

        NtStatus = SamOpenAlias(
                       DomainHandle,
                       ALIAS_WRITE_ACCOUNT | ALIAS_READ_INFORMATION,
                       AliasRid,
                       &AliasHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        //
        // Get the current value...
        //

        Buffer1 = NULL;
        NtStatus = SamQueryInformationAlias(
                       AliasHandle1,
                       AliasAdminCommentInformation,
                       &Buffer1
                       );
        TST_SUCCESS_ASSERT(NtStatus);
        ASSERT(Buffer1 != NULL);


        //
        // Change the field to a new value and write it out.
        //

        NameLength = ((ALIAS_ADM_COMMENT_INFORMATION *)Buffer1)->AdminComment.Length;
        if (  NameLength == DummyString1.Length ) {
            ((ALIAS_ADM_COMMENT_INFORMATION *)Buffer1)->AdminComment = DummyString2;
        } else {
            ((ALIAS_ADM_COMMENT_INFORMATION *)Buffer1)->AdminComment = DummyString1;
        }

        NtStatus = SamSetInformationAlias(
                       AliasHandle1,
                       AliasAdminCommentInformation,
                       Buffer1
                       );
        if ( NT_SUCCESS(NtStatus) ) {

            //
            // Now check that the change was really made...
            //

            Buffer2 = NULL;
            NtStatus = SamQueryInformationAlias(
                           AliasHandle1,
                           AliasAdminCommentInformation,
                           &Buffer2
                           );
            ASSERT(NT_SUCCESS( NtStatus ) );
            if (
                !RtlCompareString(
                    (PSTRING)&((ALIAS_ADM_COMMENT_INFORMATION *)Buffer1)->AdminComment,
                    (PSTRING)&((ALIAS_ADM_COMMENT_INFORMATION *)Buffer2)->AdminComment,
                    TRUE)
                ) {

                printf("Succeeded\n");

            } else {

                printf("Failed\n");
                printf("        Value queried doesn't match value written\n");
                printf("        Value Written is   %wZ\n",
                    (PUNICODE_STRING)&((ALIAS_ADM_COMMENT_INFORMATION *)Buffer1)->AdminComment);
                printf("        Value Retrieved is %wZ\n",
                    (PUNICODE_STRING)&((ALIAS_ADM_COMMENT_INFORMATION *)Buffer2)->AdminComment);

                TestStatus = FALSE;

            }

            SamFreeMemory( Buffer1 );
            SamFreeMemory( Buffer2 );

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
            SamFreeMemory( Buffer1 );

        }





    } // END PASS #1
    if (Pass == 2) {

        printf("\n");
        printf("\n");
        printf("  Alias (Pass #2) . . . . . . . . . . . . . . . . . . .   Test\n");

        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Delete Alias Suite                                                    //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("\n");
        printf("    Delete Alias  . . . . . . . . . . . . . . . . . . . .   Suite\n");

        printf("      Delete Normal Alias . . . . . . . . . . . . . . . . .     ");

        //
        // This alias was created in pass #1
        //

        RtlInitString( &AccountNameAnsi, ALIAS_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountNames[0], &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        NtStatus = SamLookupNamesInDomain(
                       DomainHandle,
                       1,
                       &AccountNames[0],
                       &LookedUpRids,
                       &LookedUpUses
                       );
        TST_SUCCESS_ASSERT(NtStatus);
        ASSERT(LookedUpUses[0] == SidTypeAlias);
        RtlFreeUnicodeString( &AccountNames[0] );



        AliasHandle1 = NULL;

        NtStatus = SamOpenAlias( DomainHandle, DELETE, LookedUpRids[0], &AliasHandle1 );
        TST_SUCCESS_ASSERT(NtStatus);
        SamFreeMemory( LookedUpUses ); SamFreeMemory( LookedUpRids );

        NtStatus = SamDeleteAlias( AliasHandle1 );
        if (NT_SUCCESS(NtStatus)) {
            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }


#ifdef LATER // ALIAS_LATER - well know aliases ?


        printf("      Delete Well Known Alias . . . . . . . . . . . . . . .     ");

        AliasHandle1 = NULL;

        NtStatus = SamOpenAlias( DomainHandle, DELETE, DOMAIN_GROUP_RID_USERS, &AliasHandle1 );
        TST_SUCCESS_ASSERT(NtStatus);

        NtStatus = SamDeleteAlias( AliasHandle1 );
        if (NtStatus == STATUS_SPECIAL_ACCOUNT) {

            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }

        NtStatus = SamCloseHandle( AliasHandle1 );
        ASSERT(NT_SUCCESS(NtStatus));




        printf("      Delete Admin Alias. . . . . . . . . . . . . . . . . .     ");
        AliasHandle1 = NULL;

        NtStatus = SamOpenAlias( DomainHandle, DELETE, DOMAIN_ALIAS_RID_ADMINS, &AliasHandle1 );
        TST_SUCCESS_ASSERT(NtStatus);

        NtStatus = SamDeleteAlias( AliasHandle1 );
        if (NtStatus == STATUS_SPECIAL_ACCOUNT) {

            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }

        NtStatus = SamCloseHandle( AliasHandle1 );
        ASSERT(NT_SUCCESS(NtStatus));


#endif



        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Add/Remove Member Suite                                               //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("\n");
        printf("    Add/Remove Member Suite . . . . . . . . . . . . . . .   Suite\n");

        printf("      Add Member  . . . . . . . . . . . . . . . . . . . . .     ");

        //
        // This test sets things up for the next test
        //

        //
        // The following user might already exist (from earlier in the test)
        //

        RtlInitString( &AccountNameAnsi, USER_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        UserRid = 0;
        UserHandle1 = NULL;
        NtStatus = SamCreateUserInDomain(
                       DomainHandle,
                       &AccountName,
                       USER_ALL_ACCESS,
                       &UserHandle1,
                       &UserRid
                       );
        RtlFreeUnicodeString( &AccountName );
        DeleteUser = TRUE;
        if (NtStatus == STATUS_USER_EXISTS) {
            DeleteUser = FALSE;
            RtlInitString( &AccountNameAnsi, USER_NAME1 );
            NtStatus = RtlAnsiStringToUnicodeString( &AccountNames[0], &AccountNameAnsi, TRUE );
            TST_SUCCESS_ASSERT(NtStatus);

            NtStatus = SamLookupNamesInDomain(
                           DomainHandle,
                           1,
                           &AccountNames[0],
                           &LookedUpRids,
                           &LookedUpUses
                           );
            RtlFreeUnicodeString( &AccountNames[0] );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(LookedUpUses[0] == SidTypeUser);
            UserRid = LookedUpRids[0];
            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_ALL_ACCESS,
                           UserRid,
                           &UserHandle1);
            SamFreeMemory( LookedUpUses ); SamFreeMemory( LookedUpRids );
        }

        ASSERT(NT_SUCCESS(NtStatus));

        //
        // This account won't exist yet
        //

        RtlInitString( &AccountNameAnsi, USER_NAME2 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        UserRid2 = 0;
        UserHandle2 = NULL;
        NtStatus = SamCreateUserInDomain(
                       DomainHandle,
                       &AccountName,
                       USER_ALL_ACCESS,
                       &UserHandle2,
                       &UserRid2
                       );
        RtlFreeUnicodeString( &AccountName );

        ASSERT(NT_SUCCESS(NtStatus));


        //
        // create the alias
        //

        RtlInitString( &AccountNameAnsi, ALIAS_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        AliasRid = 0;
        AliasHandle1 = NULL;
        NtStatus = SamCreateAliasInDomain(
                       DomainHandle,
                       &AccountName,
                       ALIAS_ALL_ACCESS,
                       &AliasHandle1,
                       &AliasRid
                       );
        RtlFreeUnicodeString( &AccountName );
        ASSERT(NT_SUCCESS(NtStatus));

        //
        // Make user1 a member of this alias
        //

        UserSid1 = CreateUserSid(DomainSid, UserRid);
        ASSERT(UserSid1 != NULL);

        UserSid2 = CreateUserSid(DomainSid, UserRid2);
        ASSERT(UserSid2 != NULL);



        NtStatus = SamAddMemberToAlias(
                       AliasHandle1,
                       UserSid1
                       );

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SamGetMembersInAlias(
                           AliasHandle1,
                           &AliasMembers,
                           &MemberCount
                           );
            ASSERT(NT_SUCCESS(NtStatus));

            NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;
            for ( i=0; i<MemberCount; i++) {
                if (RtlEqualSid(AliasMembers[i], UserSid1)) {
                    NtStatus = STATUS_SUCCESS;
                    break;
                }
            }

            if (!NT_SUCCESS(NtStatus)) {
                printf("Failed\n");
                printf("Service returned SUCCESS, but user not in member list for alias.\n");
                printf("Member list :\n");
                for (i=0; i<MemberCount; i++) {
                    printfSid(AliasMembers[i]);
                    printf("\n");
                }
                DebugBreak();
                TestStatus = FALSE;
            }


            if (AliasMembers != NULL) {
                SamFreeMemory( AliasMembers );
            }


            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SamGetAliasMembership(
                               DomainHandle,
                               1,
                               &UserSid1,
                               &MemberCount,
                               &Members
                               );
                ASSERT(NT_SUCCESS(NtStatus));

                NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;
                for ( i=0; i<MemberCount; i++) {
                    if (Members[i] == AliasRid) {
                        NtStatus = STATUS_SUCCESS;
                        break;
                    }
                }

                if (!NT_SUCCESS(NtStatus)) {
                    printf("Failed\n");
                    printf("Service returned SUCCESS, but alias not in account alias membership list.\n");
                    printf("Alias Membership :\n");
                    for (i=0; i<MemberCount; i++) {
                        printf("0x%lx\n", Members[i]);
                    }
                    DebugBreak();
                    TestStatus = FALSE;
                }

                if (Members != NULL) {
                    SamFreeMemory( Members );
                }


                //
                // Check for correct alias membership for multiple accounts
                // User1 should be in alias1
                // User2 should be no aliases.
                //

                if (NT_SUCCESS(NtStatus)) {

                    PSID    SidArray[2];
                    SidArray[0] = UserSid1;
                    SidArray[1] = UserSid2;

                    NtStatus = SamGetAliasMembership(
                                   DomainHandle,
                                   2,
                                   SidArray,
                                   &MemberCount,
                                   &Members
                                   );
                    ASSERT(NT_SUCCESS(NtStatus));

                    if (MemberCount != 1) {

                        printf("Failed\n");
                        printf("Service returned SUCCESS, but combined alias membership count for 2 accounts not correct.\n");
                        printf("Combined Alias Membership :\n");
                        for (i=0; i<MemberCount; i++) {
                            printf("0x%lx\n", Members[i]);
                        }
                        DebugBreak();
                        TestStatus = FALSE;

                    } else {

                        if (Members[0] != AliasRid) {
                            printf("Failed\n");
                            printf("Service returned SUCCESS, but combined alias membership for 2 accounts not correct.\n");
                            printf("Combined Alias Membership :\n");
                            for (i=0; i<MemberCount; i++) {
                                printf("0x%lx\n", Members[i]);
                            }
                            DebugBreak();
                            TestStatus = FALSE;

                        } else {
                            printf("Succeeded\n");
                        }
                    }

                    if (Members != NULL) {
                        SamFreeMemory( Members );
                    }
                }
            }


        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }







        printf("      Add another member to another alias . . . . . . . . .     ");






        //
        // Make user2 a member of alias2
        //

        //
        // This alias was created in pass #1
        //

        RtlInitString( &AccountNameAnsi, ALIAS_NAME2 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountNames[0], &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        NtStatus = SamLookupNamesInDomain(
                       DomainHandle,
                       1,
                       &AccountNames[0],
                       &LookedUpRids,
                       &LookedUpUses
                       );
        TST_SUCCESS_ASSERT(NtStatus);
        ASSERT(LookedUpUses[0] == SidTypeAlias);
        RtlFreeUnicodeString( &AccountNames[0] );

        AliasHandle2 = NULL;
        AliasRid2 = LookedUpRids[0];

        NtStatus = SamOpenAlias( DomainHandle, ALIAS_ALL_ACCESS, LookedUpRids[0], &AliasHandle2 );
        TST_SUCCESS_ASSERT(NtStatus);
        SamFreeMemory( LookedUpUses ); SamFreeMemory( LookedUpRids );


        NtStatus = SamAddMemberToAlias(
                       AliasHandle2,
                       UserSid2
                       );

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SamGetMembersInAlias(
                           AliasHandle2,
                           &AliasMembers,
                           &MemberCount
                           );
            ASSERT(NT_SUCCESS(NtStatus));

            NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;
            for ( i=0; i<MemberCount; i++) {
                if (RtlEqualSid(AliasMembers[i], UserSid2)) {
                    NtStatus = STATUS_SUCCESS;
                    break;
                }
            }

            if (!NT_SUCCESS(NtStatus)) {
                printf("Failed\n");
                printf("Service returned SUCCESS, but user not in member list for alias.\n");
                printf("Member list :\n");
                for (i=0; i<MemberCount; i++) {
                    printfSid(AliasMembers[i]);
                    printf("\n");
                }
                DebugBreak();
                TestStatus = FALSE;
            }


            if (AliasMembers != NULL) {
                SamFreeMemory( AliasMembers );
            }


            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SamGetAliasMembership(
                               DomainHandle,
                               1,
                               &UserSid2,
                               &MemberCount,
                               &Members
                               );
                ASSERT(NT_SUCCESS(NtStatus));

                NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;
                for ( i=0; i<MemberCount; i++) {
                    if (Members[i] == AliasRid2) {
                        NtStatus = STATUS_SUCCESS;
                        break;
                    }
                }

                if (!NT_SUCCESS(NtStatus)) {
                    printf("Failed\n");
                    printf("Service returned SUCCESS, but alias not in account alias membership list.\n");
                    printf("Alias Membership :\n");
                    for (i=0; i<MemberCount; i++) {
                        printf("0x%lx\n", Members[i]);
                    }
                    DebugBreak();
                    TestStatus = FALSE;
                }

                if (Members != NULL) {
                    SamFreeMemory( Members );
                }

                //
                // Check for correct alias membership for multiple accounts
                // User1 should be in alias1
                // User2 should be in alias2.
                //

                if (NT_SUCCESS(NtStatus)) {

                    PSID    SidArray[2];
                    SidArray[0] = UserSid1;
                    SidArray[1] = UserSid2;

                    NtStatus = SamGetAliasMembership(
                                   DomainHandle,
                                   2,
                                   SidArray,
                                   &MemberCount,
                                   &Members
                                   );
                    ASSERT(NT_SUCCESS(NtStatus));

                    if (MemberCount != 2) {

                        printf("Failed\n");
                        printf("Service returned SUCCESS, but combined alias membership count for 2 accounts not correct.\n");
                        printf("Combined Alias Membership :\n");
                        for (i=0; i<MemberCount; i++) {
                            printf("0x%lx\n", Members[i]);
                        }
                        DebugBreak();
                        TestStatus = FALSE;

                    } else {

                        if (((Members[0] == AliasRid) && (Members[1] == AliasRid2)) ||
                            ((Members[0] == AliasRid2) && (Members[1] == AliasRid)) ) {
                            printf("Succeeded\n");
                        } else {
                            printf("Failed\n");
                            printf("Service returned SUCCESS, but combined alias membership for 2 accounts not correct.\n");
                            printf("Combined Alias Membership :\n");
                            for (i=0; i<MemberCount; i++) {
                                printf("0x%lx\n", Members[i]);
                            }
                            DebugBreak();
                            TestStatus = FALSE;
                        }
                    }

                    if (Members != NULL) {
                        SamFreeMemory( Members );
                    }
                }
            }


        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }


        //
        // Remove user2 from alias2 again
        //

        NtStatus = SamRemoveMemberFromAlias(
                       AliasHandle2,
                       UserSid2
                       );

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SamGetMembersInAlias(
                           AliasHandle2,
                           &AliasMembers,
                           &MemberCount
                           );
            ASSERT(NT_SUCCESS(NtStatus));

            NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;
            for ( i=0; i<MemberCount; i++) {
                if (RtlEqualSid(AliasMembers[i], UserSid2)) {
                    NtStatus = STATUS_SUCCESS;
                    break;
                }
            }

            if (NtStatus != STATUS_MEMBER_NOT_IN_ALIAS) {
                printf("Failed\n");
                printf("Service returned SUCCESS, but user still in member list for alias.\n");
                printf("Member list :\n");
                for (i=0; i<MemberCount; i++) {
                    printfSid(AliasMembers[i]);
                    printf("\n");
                }
                DebugBreak();
                TestStatus = FALSE;
            }


            if (AliasMembers != NULL) {
                SamFreeMemory( AliasMembers );
            }


            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SamGetAliasMembership(
                               DomainHandle,
                               1,
                               &UserSid2,
                               &MemberCount,
                               &Members
                               );
                ASSERT(NT_SUCCESS(NtStatus));

                for ( i=0; i<MemberCount; i++) {
                    if (Members[i] == AliasRid2) {
                        NtStatus = STATUS_MEMBER_IN_ALIAS;
                        break;
                    }
                }

                if (!NT_SUCCESS(NtStatus)) {
                    printf("Failed\n");
                    printf("Service returned SUCCESS, but alias still in account alias membership list.\n");
                    printf("Alias Membership :\n");
                    for (i=0; i<MemberCount; i++) {
                        printf("0x%lx\n", Members[i]);
                    }
                    DebugBreak();
                    TestStatus = FALSE;
                }

                if (Members != NULL) {
                    SamFreeMemory( Members );
                }

                //
                // Check for correct alias membership for multiple accounts
                // User1 should be in alias1
                // User2 should be in no aliases.
                //

                if (NT_SUCCESS(NtStatus)) {

                    PSID    SidArray[2];
                    SidArray[0] = UserSid1;
                    SidArray[1] = UserSid2;

                    NtStatus = SamGetAliasMembership(
                                   DomainHandle,
                                   2,
                                   SidArray,
                                   &MemberCount,
                                   &Members
                                   );
                    ASSERT(NT_SUCCESS(NtStatus));

                    if (MemberCount != 1) {

                        printf("Failed\n");
                        printf("Service returned SUCCESS, but combined alias membership count for 2 accounts not correct.\n");
                        printf("Combined Alias Membership :\n");
                        for (i=0; i<MemberCount; i++) {
                            printf("0x%lx\n", Members[i]);
                        }
                        DebugBreak();
                        TestStatus = FALSE;

                    } else {

                        if (Members[0] == AliasRid) {
                            printf("Succeeded\n");
                        } else {
                            printf("Failed\n");
                            printf("Service returned SUCCESS, but combined alias membership for 2 accounts not correct.\n");
                            printf("Combined Alias Membership :\n");
                            for (i=0; i<MemberCount; i++) {
                                printf("0x%lx\n", Members[i]);
                            }
                            DebugBreak();
                            TestStatus = FALSE;
                        }
                    }

                    if (Members != NULL) {
                        SamFreeMemory( Members );
                    }
                }
            }


        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }


        NtStatus = SamCloseHandle( AliasHandle2 );
        ASSERT(NT_SUCCESS(NtStatus));





        printf("      Add Another Member  . . . . . . . . . . . . . . . . .     ");


        //
        // Make user2 a member of this alias
        //

        NtStatus = SamAddMemberToAlias(
                       AliasHandle1,
                       UserSid2
                       );

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SamGetMembersInAlias(
                           AliasHandle1,
                           &AliasMembers,
                           &MemberCount
                           );
            ASSERT(NT_SUCCESS(NtStatus));

            NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;
            for ( i=0; i<MemberCount; i++) {
                if (RtlEqualSid(AliasMembers[i], UserSid2)) {
                    NtStatus = STATUS_SUCCESS;
                    break;
                }
            }

            if (!NT_SUCCESS(NtStatus)) {
                printf("Failed\n");
                printf("Service returned SUCCESS, but user not in member list for alias.\n");
                printf("Member list :\n");
                for (i=0; i<MemberCount; i++) {
                    printfSid(AliasMembers[i]);
                    printf("\n");
                }
                DebugBreak();
                TestStatus = FALSE;
            }


            if (AliasMembers != NULL) {
                SamFreeMemory( AliasMembers );
            }


            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SamGetAliasMembership(
                               DomainHandle,
                               1,
                               &UserSid2,
                               &MemberCount,
                               &Members
                               );
                ASSERT(NT_SUCCESS(NtStatus));

                NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;
                for ( i=0; i<MemberCount; i++) {
                    if (Members[i] == AliasRid) {
                        NtStatus = STATUS_SUCCESS;
                        break;
                    }
                }

                if (!NT_SUCCESS(NtStatus)) {
                    printf("Failed\n");
                    printf("Service returned SUCCESS, but alias not in account alias membership list.\n");
                    printf("Alias Membership :\n");
                    for (i=0; i<MemberCount; i++) {
                        printf("0x%lx\n", Members[i]);
                    }
                    DebugBreak();
                    TestStatus = FALSE;
                }

                if (Members != NULL) {
                    SamFreeMemory( Members );
                }

                //
                // Check for correct alias membership for multiple accounts
                // User1 should be in alias1
                // User2 should be in alias1.
                //

                if (NT_SUCCESS(NtStatus)) {

                    PSID    SidArray[2];
                    SidArray[0] = UserSid1;
                    SidArray[1] = UserSid2;

                    NtStatus = SamGetAliasMembership(
                                   DomainHandle,
                                   2,
                                   SidArray,
                                   &MemberCount,
                                   &Members
                                   );
                    ASSERT(NT_SUCCESS(NtStatus));

                    if (MemberCount != 1) {

                        printf("Failed\n");
                        printf("Service returned SUCCESS, but combined alias membership count for 2 accounts not correct.\n");
                        printf("Combined Alias Membership :\n");
                        for (i=0; i<MemberCount; i++) {
                            printf("0x%lx\n", Members[i]);
                        }
                        DebugBreak();
                        TestStatus = FALSE;

                    } else {

                        if (Members[0] != AliasRid) {
                            printf("Failed\n");
                            printf("Service returned SUCCESS, but combined alias membership for 2 accounts not correct.\n");
                            printf("Combined Alias Membership :\n");
                            for (i=0; i<MemberCount; i++) {
                                printf("0x%lx\n", Members[i]);
                            }
                            DebugBreak();
                            TestStatus = FALSE;

                        } else {
                            printf("Succeeded\n");
                        }
                    }

                    if (Members != NULL) {
                        SamFreeMemory( Members );
                    }
                }
            }


        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }


        printf("      Remove Member . . . . . . . . . . . . . . . . . . . .     ");

        //
        // The previous test sets this one up.
        //

        //
        // Now try to remove the user from the alias
        //

        NtStatus = SamRemoveMemberFromAlias(AliasHandle1, UserSid1);
        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SamGetMembersInAlias(
                           AliasHandle1,
                           &AliasMembers,
                           &MemberCount
                           );
            ASSERT(NT_SUCCESS(NtStatus));

            for ( i=0; i<MemberCount; i++) {
                if (RtlEqualSid(AliasMembers[i], UserSid1)) {
                    NtStatus = STATUS_MEMBER_IN_ALIAS;
                    break;
                }
            }

            if (!NT_SUCCESS(NtStatus)) {
                printf("Failed\n");
                printf("Service returned SUCCESS, but user still in member list for alias.\n");
                printf("Member list :\n");
                for (i=0; i<MemberCount; i++) {
                    printfSid(AliasMembers[i]);
                    printf("\n");
                }
                DebugBreak();
                TestStatus = FALSE;
            }

            SamFreeMemory( AliasMembers );

            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SamGetAliasMembership(
                               DomainHandle,
                               1,
                               &UserSid1,
                               &MemberCount,
                               &Members
                               );
                ASSERT(NT_SUCCESS(NtStatus));

                for ( i=0; i<MemberCount; i++) {
                    if (Members[i] == AliasRid) {
                        NtStatus = STATUS_MEMBER_IN_ALIAS;
                        break;
                    }
                }

                if (!NT_SUCCESS(NtStatus)) {
                    printf("Failed\n");
                    printf("Service returned SUCCESS, but alias still in account alias membership list.\n");
                    printf("Alias Membership :\n");
                    for (i=0; i<MemberCount; i++) {
                        printf("0x%lx\n", Members[i]);
                    }
                    DebugBreak();
                    TestStatus = FALSE;
                }

                if (Members != NULL) {
                    SamFreeMemory( Members );
                }

                //
                // Check for correct alias membership for multiple accounts
                // User1 should be in no aliases
                // User2 should be in alias1.
                //

                if (NT_SUCCESS(NtStatus)) {

                    PSID    SidArray[2];
                    SidArray[0] = UserSid1;
                    SidArray[1] = UserSid2;

                    NtStatus = SamGetAliasMembership(
                                   DomainHandle,
                                   2,
                                   SidArray,
                                   &MemberCount,
                                   &Members
                                   );
                    ASSERT(NT_SUCCESS(NtStatus));

                    if (MemberCount != 1) {

                        printf("Failed\n");
                        printf("Service returned SUCCESS, but combined alias membership count for 2 accounts not correct.\n");
                        printf("Combined Alias Membership :\n");
                        for (i=0; i<MemberCount; i++) {
                            printf("0x%lx\n", Members[i]);
                        }
                        DebugBreak();
                        TestStatus = FALSE;

                    } else {

                        if (Members[0] != AliasRid) {
                            printf("Failed\n");
                            printf("Service returned SUCCESS, but combined alias membership for 2 accounts not correct.\n");
                            printf("Combined Alias Membership :\n");
                            for (i=0; i<MemberCount; i++) {
                                printf("0x%lx\n", Members[i]);
                            }
                            DebugBreak();
                            TestStatus = FALSE;

                        } else {
                            printf("Succeeded\n");
                        }
                    }

                    if (Members != NULL) {
                        SamFreeMemory( Members );
                    }
                }
            }

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }




        printf("      Add A User to ADMIN Alias . . . . . . . . . . . . . .     ");

        //
        // Make user2 a member of the ADMIN alias
        //

        NtStatus = SamOpenAlias(
                       BuiltinDomainHandle,
                       ALIAS_ALL_ACCESS,
                       DOMAIN_ALIAS_RID_ADMINS,
                       &AdminAliasHandle
                       );

        ASSERT( NT_SUCCESS( NtStatus ) );

        NtStatus = SamAddMemberToAlias(
                       AdminAliasHandle,
                       UserSid2
                       );

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SamGetMembersInAlias(
                           AdminAliasHandle,
                           &AliasMembers,
                           &MemberCount
                           );
            ASSERT(NT_SUCCESS(NtStatus));

            NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;
            for ( i=0; i<MemberCount; i++) {
                if (RtlEqualSid(AliasMembers[i], UserSid2)) {
                    NtStatus = STATUS_SUCCESS;
                    break;
                }
            }

            if (!NT_SUCCESS(NtStatus)) {
                printf("Failed\n");
                printf("Service returned SUCCESS, but user not in member list for alias.\n");
                printf("Member list :\n");
                for (i=0; i<MemberCount; i++) {
                    printfSid(AliasMembers[i]);
                    printf("\n");
                }
                DebugBreak();
                TestStatus = FALSE;
            }


            if (AliasMembers != NULL) {
                SamFreeMemory( AliasMembers );
            }


            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SamGetAliasMembership(
                               BuiltinDomainHandle,
                               1,
                               &UserSid2,
                               &MemberCount,
                               &Members
                               );
                ASSERT(NT_SUCCESS(NtStatus));

                NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;
                for ( i=0; i<MemberCount; i++) {
                    if (Members[i] == DOMAIN_ALIAS_RID_ADMINS) {
                        NtStatus = STATUS_SUCCESS;
                        break;
                    }
                }

                if (!NT_SUCCESS(NtStatus)) {
                    printf("Failed\n");
                    printf("Service returned SUCCESS, but alias not in account alias membership list.\n");
                    printf("Alias Membership :\n");
                    for (i=0; i<MemberCount; i++) {
                        printf("0x%lx\n", Members[i]);
                    }
                    DebugBreak();
                    TestStatus = FALSE;

                } else {

                    printf("Succeeded\n");
                }

                if (Members != NULL) {
                    SamFreeMemory( Members );
                }
            }


        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }


        printf("      Add A Group to ADMIN Alias . . . . . . . . . . . . . .     ");

        //
        // Make a group a member of the ADMIN alias
        //

        GroupSid = CreateUserSid(DomainSid, DOMAIN_GROUP_RID_USERS );
        ASSERT(GroupSid != NULL);

        NtStatus = SamAddMemberToAlias(
                       AdminAliasHandle,
                       GroupSid
                       );

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SamGetMembersInAlias(
                           AdminAliasHandle,
                           &AliasMembers,
                           &MemberCount
                           );
            ASSERT(NT_SUCCESS(NtStatus));

            NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;
            for ( i=0; i<MemberCount; i++) {
                if (RtlEqualSid(AliasMembers[i], GroupSid)) {
                    NtStatus = STATUS_SUCCESS;
                    break;
                }
            }

            if (!NT_SUCCESS(NtStatus)) {
                printf("Failed\n");
                printf("Service returned SUCCESS, but user not in member list for alias.\n");
                printf("Member list :\n");
                for (i=0; i<MemberCount; i++) {
                    printfSid(AliasMembers[i]);
                    printf("\n");
                }
                DebugBreak();
                TestStatus = FALSE;
            }


            if (AliasMembers != NULL) {
                SamFreeMemory( AliasMembers );
            }


            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SamGetAliasMembership(
                               BuiltinDomainHandle,
                               1,
                               &GroupSid,
                               &MemberCount,
                               &Members
                               );
                ASSERT(NT_SUCCESS(NtStatus));

                NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;
                for ( i=0; i<MemberCount; i++) {
                    if (Members[i] == DOMAIN_ALIAS_RID_ADMINS) {
                        NtStatus = STATUS_SUCCESS;
                        break;
                    }
                }

                if (!NT_SUCCESS(NtStatus)) {
                    printf("Failed\n");
                    printf("Service returned SUCCESS, but alias not in account alias membership list.\n");
                    printf("Alias Membership :\n");
                    for (i=0; i<MemberCount; i++) {
                        printf("0x%lx\n", Members[i]);
                    }
                    DebugBreak();
                    TestStatus = FALSE;

                } else {

                    printf("Succeeded\n");
                }

                if (Members != NULL) {
                    SamFreeMemory( Members );
                }
            }


        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }


// NOTE: user is already created in the group below.  Should keep this
// test, AND add another with an all-new group that's been added to the ADMIN
// alias (then ADD user to group, rather than create in it).
        printf("      Create user in ADMIN ALIAS'd Group. . . . . . . . . . .     ");

        RtlInitString( &AccountNameAnsi, USER_NAME3 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        UserRid3 = 0;
        UserHandle3 = NULL;
        NtStatus = SamCreateUserInDomain(
                       DomainHandle,
                       &AccountName,
                       USER_ALL_ACCESS,
                       &UserHandle3,
                       &UserRid3
                       );
        RtlFreeUnicodeString( &AccountName );

        if ( NT_SUCCESS( NtStatus ) ) {

            printf("Succeeded\n");

        } else {

            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }



//NOTE: doesn't work because this is primary group.
//put back in when all-new group is created, above
//        printf("      Remove user from ADMIN ALIAS'd Group. . . . . . . . . . .     ");
//
//        NtStatus = SamOpenGroup(
//                       DomainHandle,
//                       GROUP_ALL_ACCESS,
//                       DOMAIN_GROUP_RID_USERS,
//                       &GroupHandle
//                       );
//
//        ASSERT(NT_SUCCESS(NtStatus));
//
//        NtStatus = SamRemoveMemberFromGroup(
//                       GroupHandle,
//                       UserRid3
//                       );
//
//        if ( NT_SUCCESS( NtStatus ) ) {
//
//            printf("Succeeded\n");
//
//        } else {
//
//            printf("Failed\n");
//            printf("        Completion status is 0x%lx\n", NtStatus);
//            TestStatus = FALSE;
//        }
//
//        IgnoreStatus = SamCloseHandle( GroupHandle );
//        ASSERT(NT_SUCCESS(IgnoreStatus));
        IgnoreStatus = SamCloseHandle( UserHandle3 );
        ASSERT(NT_SUCCESS(IgnoreStatus));



        printf("      Remove User from ADMIN alias. . . . . . . . . . .     ");

        //
        // The previous test sets this one up.
        //
        // Now try to remove the user from the alias
        //

        NtStatus = SamRemoveMemberFromAlias(AdminAliasHandle, UserSid2);
        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SamGetMembersInAlias(
                           AdminAliasHandle,
                           &AliasMembers,
                           &MemberCount
                           );
            ASSERT(NT_SUCCESS(NtStatus));

            for ( i=0; i<MemberCount; i++) {
                if (RtlEqualSid(AliasMembers[i], UserSid2)) {
                    NtStatus = STATUS_MEMBER_IN_ALIAS;
                    break;
                }
            }

            if (!NT_SUCCESS(NtStatus)) {
                printf("Failed\n");
                printf("Service returned SUCCESS, but user still in member list for alias.\n");
                printf("Member list :\n");
                for (i=0; i<MemberCount; i++) {
                    printfSid(AliasMembers[i]);
                    printf("\n");
                }
                DebugBreak();
                TestStatus = FALSE;
            }

            SamFreeMemory( AliasMembers );

            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SamGetAliasMembership(
                               BuiltinDomainHandle,
                               1,
                               &UserSid2,
                               &MemberCount,
                               &Members
                               );
                ASSERT(NT_SUCCESS(NtStatus));

                for ( i=0; i<MemberCount; i++) {
                    if (Members[i] == DOMAIN_ALIAS_RID_ADMINS) {
                        NtStatus = STATUS_MEMBER_IN_ALIAS;
                        break;
                    }
                }

                if (!NT_SUCCESS(NtStatus)) {
                    printf("Failed\n");
                    printf("Service returned SUCCESS, but alias still in account alias membership list.\n");
                    printf("Alias Membership :\n");
                    for (i=0; i<MemberCount; i++) {
                        printf("0x%lx\n", Members[i]);
                    }
                    DebugBreak();
                    TestStatus = FALSE;

                } else {

                    printf("Succeeded\n");
                }

                if (Members != NULL) {
                    SamFreeMemory( Members );
                }
            }

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }




        //
        // Make user2 a member of the ADMIN alias again, so we can test
        // the new function SamRemoveMemberFromForeignDomain().
        // NOTE: we should make this a real test item.
        //

        NtStatus = SamAddMemberToAlias(
                       AdminAliasHandle,
                       UserSid2
                       );

        ASSERT (NT_SUCCESS(NtStatus));

        NtStatus = SamRemoveMemberFromForeignDomain(
                       BuiltinDomainHandle,
                       UserSid2 );

        ASSERT (NT_SUCCESS(NtStatus));



        printf("      Remove Group from ADMIN alias. . . . . . . . . . .     ");

        //
        // The previous test sets this one up.
        //
        // Now try to remove the group from the alias
        //

        NtStatus = SamRemoveMemberFromAlias(AdminAliasHandle, GroupSid);
        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SamGetMembersInAlias(
                           AdminAliasHandle,
                           &AliasMembers,
                           &MemberCount
                           );
            ASSERT(NT_SUCCESS(NtStatus));

            for ( i=0; i<MemberCount; i++) {
                if (RtlEqualSid(AliasMembers[i], GroupSid)) {
                    NtStatus = STATUS_MEMBER_IN_ALIAS;
                    break;
                }
            }

            if (!NT_SUCCESS(NtStatus)) {
                printf("Failed\n");
                printf("Service returned SUCCESS, but user still in member list for alias.\n");
                printf("Member list :\n");
                for (i=0; i<MemberCount; i++) {
                    printfSid(AliasMembers[i]);
                    printf("\n");
                }
                DebugBreak();
                TestStatus = FALSE;
            }

            SamFreeMemory( AliasMembers );

            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SamGetAliasMembership(
                               BuiltinDomainHandle,
                               1,
                               &GroupSid,
                               &MemberCount,
                               &Members
                               );
                ASSERT(NT_SUCCESS(NtStatus));

                for ( i=0; i<MemberCount; i++) {
                    if (Members[i] == DOMAIN_ALIAS_RID_ADMINS) {
                        NtStatus = STATUS_MEMBER_IN_ALIAS;
                        break;
                    }
                }

                if (!NT_SUCCESS(NtStatus)) {
                    printf("Failed\n");
                    printf("Service returned SUCCESS, but alias still in account alias membership list.\n");
                    printf("Alias Membership :\n");
                    for (i=0; i<MemberCount; i++) {
                        printf("0x%lx\n", Members[i]);
                    }
                    DebugBreak();
                    TestStatus = FALSE;

                } else {

                    printf("Succeeded\n");
                }

                if (Members != NULL) {
                    SamFreeMemory( Members );
                }
            }

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }

        IgnoreStatus = SamCloseHandle( AdminAliasHandle );
        ASSERT( NT_SUCCESS(IgnoreStatus) );



        printf("      Delete account while member of alias. . . . . . . . .     ");


        //
        // Now delete user2 and check the alias member list is updated
        //

        NtStatus = SamDeleteUser( UserHandle2 );
        ASSERT(NT_SUCCESS(NtStatus));

        NtStatus = SamGetMembersInAlias(
                       AliasHandle1,
                       &AliasMembers,
                       &MemberCount
                       );
        ASSERT(NT_SUCCESS(NtStatus));

        for ( i=0; i<MemberCount; i++) {
            if (RtlEqualSid(AliasMembers[i], UserSid2)) {
                NtStatus = STATUS_MEMBER_IN_ALIAS;
                break;
            }
        }

        if (!NT_SUCCESS(NtStatus)) {
            printf("Failed\n");
            printf("Service returned SUCCESS, but user still in member list for alias.\n");
            printf("Member list :\n");
            for (i=0; i<MemberCount; i++) {
                printfSid(AliasMembers[i]);
                printf("\n");
            }
            DebugBreak();
            TestStatus = FALSE;
        }

        SamFreeMemory( AliasMembers );

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SamGetAliasMembership(
                           DomainHandle,
                           1,
                           &UserSid2,
                           &MemberCount,
                           &Members
                           );
            ASSERT(NT_SUCCESS(NtStatus));

            if (MemberCount != 0) {
                printf("Failed\n");
                printf("Service returned SUCCESS, but alias still in alias membership list for account.\n");
                printf("Alias Membership :\n");
                for (i=0; i<MemberCount; i++) {
                    printf("0x%lx\n", Members[i]);
                }
                DebugBreak();
                TestStatus = FALSE;
            }

            if (Members != NULL) {
                SamFreeMemory( Members );
            }

            //
            // Check for correct alias membership for multiple accounts
            // User1 should be in no aliases
            // User2 should be in no aliases.
            //

            if (NT_SUCCESS(NtStatus)) {

                PSID    SidArray[2];
                SidArray[0] = UserSid1;
                SidArray[1] = UserSid2;

                NtStatus = SamGetAliasMembership(
                               DomainHandle,
                               2,
                               SidArray,
                               &MemberCount,
                               &Members
                               );
                ASSERT(NT_SUCCESS(NtStatus));

                if (MemberCount != 0) {

                    printf("Failed\n");
                    printf("Service returned SUCCESS, but combined alias membership count for 2 accounts not correct.\n");
                    printf("Combined Alias Membership :\n");
                    for (i=0; i<MemberCount; i++) {
                        printf("0x%lx\n", Members[i]);
                    }
                    DebugBreak();
                    TestStatus = FALSE;

                } else {
                    printf("Succeeded\n");
                }

                if (Members != NULL) {
                    SamFreeMemory( Members );
                }
            }
        }




        printf("      Delete alias with members . . . . . . . . . . . . . .     ");

        //
        // Make the user a member of this alias (again)
        //

        NtStatus = SamAddMemberToAlias(
                       AliasHandle1,
                       UserSid1
                       );
        ASSERT(NT_SUCCESS(NtStatus));

        //
        // Now delete the alias and check the membership list for user is updated
        //

        NtStatus = SamDeleteAlias( AliasHandle1 );
        ASSERT(NT_SUCCESS(NtStatus));

        NtStatus = SamGetAliasMembership(
                       DomainHandle,
                       1,
                       &UserSid1,
                       &MemberCount,
                       &Members
                       );
        ASSERT(NT_SUCCESS(NtStatus));

        for ( i=0; i<MemberCount; i++) {
            if (Members[i] == AliasRid) {
                NtStatus = STATUS_MEMBER_IN_ALIAS;
                break;
            }
        }

        if (NT_SUCCESS(NtStatus)) {
            printf("Succeeded\n");
        } else {
            printf("Failed\n");
            printf("Service returned SUCCESS, but alias still in account alias membership list.\n");
            printf("Alias Membership :\n");
            for (i=0; i<MemberCount; i++) {
                printf("0x%lx\n", Members[i]);
            }
            DebugBreak();
            TestStatus = FALSE;
        }

        if (Members != NULL) {
            SamFreeMemory( Members );
        }



        DeleteUserSid(UserSid1);
        DeleteUserSid(UserSid2);

        //
        // and clean up
        //

        if (DeleteUser == TRUE) {
            NtStatus = SamDeleteUser( UserHandle1 );
            ASSERT(NT_SUCCESS(NtStatus));
        } else {
            NtStatus = SamCloseHandle( UserHandle1 );
            ASSERT(NT_SUCCESS(NtStatus));
        }






        printf("      Add Foreign Domain Member . . . . . . . . . . . . . .     ");

        //
        // create the alias
        //

        RtlInitString( &AccountNameAnsi, ALIAS_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        AliasRid = 0;
        AliasHandle1 = NULL;
        NtStatus = SamCreateAliasInDomain(
                       DomainHandle,
                       &AccountName,
                       ALIAS_ALL_ACCESS,
                       &AliasHandle1,
                       &AliasRid
                       );
        RtlFreeUnicodeString( &AccountName );
        ASSERT(NT_SUCCESS(NtStatus));

        //
        // Specify a non-existant user be added to this alias
        //


        {
            PSID    ForeignDomainSid;

            ForeignDomainSid = CreateUserSid(DomainSid, 307333); // random domain sub-authority
            ASSERT(ForeignDomainSid != NULL);

            UserRid = 45728;    // Random user rid

            UserSid1 = CreateUserSid(ForeignDomainSid, UserRid);
            ASSERT(UserSid1 != NULL);

            DeleteUserSid(ForeignDomainSid);
        }


        NtStatus = SamAddMemberToAlias(
                       AliasHandle1,
                       UserSid1
                       );

        if (NtStatus == STATUS_SUCCESS) {

            NtStatus = SamGetMembersInAlias(
                           AliasHandle1,
                           &AliasMembers,
                           &MemberCount
                           );
            ASSERT(NT_SUCCESS(NtStatus));

            NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;
            for ( i=0; i<MemberCount; i++) {
                if (RtlEqualSid(AliasMembers[i], UserSid1)) {
                    NtStatus = STATUS_SUCCESS;
                    break;
                }
            }

            if (!NT_SUCCESS(NtStatus)) {
                printf("Failed\n");
                printf("Service returned SUCCESS, but user not in member list for alias.\n");
                printf("Member list :\n");
                for (i=0; i<MemberCount; i++) {
                    printfSid(AliasMembers[i]);
                    printf("\n");
                }
                DebugBreak();
                TestStatus = FALSE;
            }


            if (AliasMembers != NULL) {
                SamFreeMemory( AliasMembers );
            }


            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SamGetAliasMembership(
                               DomainHandle,
                               1,
                               &UserSid1,
                               &MemberCount,
                               &Members
                               );
                ASSERT(NT_SUCCESS(NtStatus));

                NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;
                for ( i=0; i<MemberCount; i++) {
                    if (Members[i] == AliasRid) {
                        NtStatus = STATUS_SUCCESS;
                        break;
                    }
                }

                if (NT_SUCCESS(NtStatus)) {
                    printf("Succeeded\n");
                } else {
                    printf("Failed\n");
                    printf("Service returned SUCCESS, but alias not in account alias membership list.\n");
                    printf("Alias Membership :\n");
                    for (i=0; i<MemberCount; i++) {
                        printf("0x%lx\n", Members[i]);
                    }
                    DebugBreak();
                    TestStatus = FALSE;
                }

                if (Members != NULL) {
                    SamFreeMemory( Members );
                }
            }

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }

        DeleteUserSid(UserSid1);




        printf("      Add alias as member . . . . . . . . . . . . . . . . .     ");

        //
        // Specify an alias in the current domain be added to this alias
        //


        UserSid1 = CreateUserSid(DomainSid, AliasRid2);
        ASSERT(UserSid1 != NULL);


        NtStatus = SamAddMemberToAlias(
                       AliasHandle1,
                       UserSid1
                       );

        if (NtStatus != STATUS_INVALID_MEMBER) {

                printf("Failed\n");
                printf("Expected service to return STATUS_INVALID_MEMBER, actually returned 0x%lx.\n", NtStatus);
                DebugBreak();
                TestStatus = FALSE;
        } else {
                printf("Succeeded\n");
        }

        DeleteUserSid(UserSid1);



        printf("      Add non-existant account in this domain as member . .     ");

        //
        // Specify a non-existant account in the current domain be added to this alias
        //


        UserSid1 = CreateUserSid(DomainSid, 32567); // Random rid
        ASSERT(UserSid1 != NULL);


        NtStatus = SamAddMemberToAlias(
                       AliasHandle1,
                       UserSid1
                       );

        if (NtStatus != STATUS_NO_SUCH_MEMBER) {

                printf("Failed\n");
                printf("Expected service to return STATUS_NO_SUCH_MEMBER, actually returned 0x%lx.\n", NtStatus);
                DebugBreak();
                TestStatus = FALSE;
        } else {
                printf("Succeeded\n");
        }

        DeleteUserSid(UserSid1);



        printf("      Remove Non-member . . . . . . . . . . . . . . . . . .      ");

        //
        // Specify a non-existant user be removed from this alias
        //

        {
            PSID    ForeignDomainSid;

            ForeignDomainSid = CreateUserSid(DomainSid, 35775); // random domain sub-authority
            ASSERT(ForeignDomainSid != NULL);

            UserRid = 623545;    // Random user rid

            UserSid1 = CreateUserSid(ForeignDomainSid, UserRid);
            ASSERT(UserSid1 != NULL);

            DeleteUserSid(ForeignDomainSid);
        }

        NtStatus = SamRemoveMemberFromAlias( AliasHandle1, UserSid1 );

        if (NtStatus == STATUS_MEMBER_NOT_IN_ALIAS) {

            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }

        DeleteUserSid(UserSid1);

        NtStatus = SamDeleteAlias( AliasHandle1 );
        ASSERT(NT_SUCCESS(NtStatus));




    }

    return(TestStatus);

}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// User Object Test Suite                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


BOOLEAN
UserTestSuite(
    HANDLE DomainHandle,
    ULONG Pass
    )


{

    PUSER_ALL_INFORMATION All, All2;
    NTSTATUS            NtStatus, IgnoreStatus, TmpStatus;
    HANDLE              UserHandle1, UserHandle2, GroupHandle1;
    ULONG               CountReturned, NameLength, MembershipCount, i;
    ULONG               UserRid, GroupRid;
    PVOID               Buffer, Buffer1, Buffer2;
    SAM_ENUMERATE_HANDLE EnumerationContext;
    USER_GENERAL_INFORMATION GeneralInformation;
    USER_LOGON_INFORMATION LogonInformation;
    USER_ACCOUNT_INFORMATION AccountInformation;
    PSID_NAME_USE       LookedUpUses;
    PULONG              LookedUpRids;
    UNICODE_STRING      AccountNames[10], AccountName;
    STRING              AccountNameAnsi, TmpAnsiString;




    BOOLEAN             IndividualTestSucceeded, DeleteUser;
    BOOLEAN             TestStatus = TRUE;




    if (Pass == 1) {
        // This test suite assumes that lookup and enumeration API funciton
        // properly.
        //

        printf("\n");
        printf("\n");
        printf("  User  (Pass #1) . . . . . . . . . . . . . . . . . . .   Test\n");

        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Open User  Suite                                                      //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("    Open User   . . . . . . . . . . . . . . . . . . . . .   Suite\n");
        printf("      Open Users  . . . . . . . . . . . . . . . . . . . . .     ");
        IndividualTestSucceeded = TRUE;
        EnumerationContext = 0;
        NtStatus = SamEnumerateUsersInDomain(
                       DomainHandle,
                       &EnumerationContext,
                       0,
                       &Buffer,
                       12000,                   // PreferedMaximumLength
                       &CountReturned
                       );

        TST_SUCCESS_ASSERT(NtStatus);
        ASSERT(Buffer != NULL);
        ASSERT(CountReturned > 0);

        for (i=0; i<CountReturned; i++) {

            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_ALL_ACCESS,
                           ((PSAM_RID_ENUMERATION)(Buffer))[i].RelativeId,
                           &UserHandle1
                           );

            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SamOpenUser(
                               DomainHandle,
                               GENERIC_READ,
                               ((PSAM_RID_ENUMERATION)(Buffer))[i].RelativeId,
                               &UserHandle2
                               );

                if (NT_SUCCESS(NtStatus)) {
                    IgnoreStatus = SamCloseHandle( UserHandle2 );
                    ASSERT( NT_SUCCESS(IgnoreStatus) );
                } else {
                    printf("Failed\n");
                    printf("        Completion status is 0x%lx\n", NtStatus);
                    printf("        Failed opening User  second time.\n");
                    printf("        Rid of account is:   0x%lx\n",
                        ((PSAM_RID_ENUMERATION)(Buffer))[i].RelativeId);
                    printf("        Name of account is:  %wZ\n",
                        &((PSAM_RID_ENUMERATION)(Buffer))[i].Name );
                    TestStatus = FALSE;
                    IndividualTestSucceeded = FALSE;
                }

                IgnoreStatus = SamCloseHandle( UserHandle1 );
                ASSERT( NT_SUCCESS(IgnoreStatus) );

            } else {

                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                printf("        Failed opening User  for first time.\n");
                printf("        Rid of account is:   0x%lx\n",
                    ((PSAM_RID_ENUMERATION)(Buffer))[i].RelativeId);
                printf("        Name of account is:  %wZ\n",
                    &((PSAM_RID_ENUMERATION)(Buffer))[i].Name );
                TestStatus = FALSE;
                IndividualTestSucceeded = FALSE;
            }

        }


        SamFreeMemory( Buffer );
        if (IndividualTestSucceeded) {
            printf("Succeeded\n");
        }



        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Query User Suite                                                      //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("\n");
        printf("    Query User  . . . . . . . . . . . . . . . . . . . . .   Suite\n");

        printf("      Query User  General Information . . . . . . . . . . .     ");


        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_GENERAL,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserGeneralInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                if ( (((USER_GENERAL_INFORMATION *)Buffer)->UserName.MaximumLength
                    >= 0) &&
                     (((USER_GENERAL_INFORMATION *)Buffer)->UserName.Buffer != NULL)
                         ) {

                    printf("Succeeded\n");

                    printf("      Primary Group is:   0x%lx\n",
                     (((USER_GENERAL_INFORMATION *)Buffer)->PrimaryGroupId) );
                    printf("        User  Name is:    *%wZ*\n",
                     &(((USER_GENERAL_INFORMATION *)Buffer)->UserName) );
                    printf("        Full  Name is:    *%wZ*\n",
                     &(((USER_GENERAL_INFORMATION *)Buffer)->FullName) );
                    printf("     Admin Comment is:    *%wZ*\n",
                     &(((USER_GENERAL_INFORMATION *)Buffer)->AdminComment) );
                    printf("      User Comment is:    *%wZ*\n",
                     &(((USER_GENERAL_INFORMATION *)Buffer)->UserComment) );


                } else {
                    printf("Failed\n");
                    printf("        One of the UNICODE_STRINGs  not returned.\n");
                    TestStatus = FALSE;
                }
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );




        printf("      Query User Name Information . . . . . . . . . . . . .     ");


        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_GENERAL,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserNameInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                if ( (((USER_NAME_INFORMATION *)Buffer)->UserName.MaximumLength > 0) &&
                     (((USER_NAME_INFORMATION *)Buffer)->UserName.Buffer != NULL)
                         ) {

                    printf("Succeeded\n");

                    printf("        User  Name is:    *%wZ*\n",
                     &(((USER_NAME_INFORMATION *)Buffer)->UserName) );
                    printf("        Full  Name is:    *%wZ*\n",
                     &(((USER_NAME_INFORMATION *)Buffer)->FullName) );



                } else {
                    printf("Failed\n");
                    printf("        One of the UNICODE_STRINGs not returned.\n");
                    TestStatus = FALSE;
                }
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );




        printf("      Query User Account Name Information . . . . . . . . .     ");


        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_GENERAL,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserAccountNameInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                if ( (((USER_ACCOUNT_NAME_INFORMATION *)Buffer)->UserName.MaximumLength > 0) &&
                     (((USER_ACCOUNT_NAME_INFORMATION *)Buffer)->UserName.Buffer != NULL)
                         ) {

                    printf("Succeeded\n");

                    printf("        User  Name is:    *%wZ*\n",
                     &(((USER_ACCOUNT_NAME_INFORMATION *)Buffer)->UserName) );



                } else {
                    printf("Failed\n");
                    printf("        UNICODE_STRING not returned.\n");
                    TestStatus = FALSE;
                }
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );




        printf("      Query User Full Name Information  . . . . . . . . . .     ");


        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_GENERAL,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserFullNameInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                if ( (((USER_FULL_NAME_INFORMATION *)Buffer)->FullName.MaximumLength
                    >= 0)
                         ) {

                    printf("Succeeded\n");

                    printf("        Full Name is:    *%wZ*\n",
                     &(((USER_FULL_NAME_INFORMATION *)Buffer)->FullName) );



                } else {
                    printf("Failed\n");
                    printf("        UNICODE_STRING not returned.\n");
                    TestStatus = FALSE;
                }
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );




        printf("      Query User  Admin Comment Information . . . . . . . .     ");


        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_GENERAL,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserAdminCommentInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                if ( (((USER_ADMIN_COMMENT_INFORMATION *)Buffer)->AdminComment.MaximumLength
                    >= 0)
                         ) {

                    printf("Succeeded\n");

                    printf("     Admin Comment is:    *%wZ*\n",
                     &(((USER_ADMIN_COMMENT_INFORMATION *)Buffer)->AdminComment) );

                } else {
                    printf("Failed\n");
                    printf("        User  Admin Comment not returned.\n");
                    TestStatus = FALSE;
                }
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );




        printf("      Query User  Primary Group Information . . . . . . . .     ");


        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_GENERAL,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserPrimaryGroupInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {


                printf("Succeeded\n");

                printf("     Primary Group  is:   0x%lx\n",
                 (((USER_PRIMARY_GROUP_INFORMATION *)Buffer)->PrimaryGroupId) );

                SamFreeMemory( Buffer );

            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );




        printf("      Query User Control Information  . . . . . . . . . . .     ");


        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_ACCOUNT,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserControlInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {


                printf("Succeeded\n");

                printf(" Account Control is:      0x%lx\n",
                 (((USER_CONTROL_INFORMATION *)Buffer)->UserAccountControl) );

                SamFreeMemory( Buffer );


            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );




        printf("      Query User Expiration Information . . . . . . . . . .     ");


        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_ACCOUNT,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserExpiresInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {


                printf("Succeeded\n");

                printf(" Account Expires on:      (0x%lx, 0x%lx)\n",
                 (((USER_EXPIRES_INFORMATION *)Buffer)->AccountExpires.HighPart),
                 (((USER_EXPIRES_INFORMATION *)Buffer)->AccountExpires.LowPart) );

                SamFreeMemory( Buffer );


            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );


        printf("      Query User Preferences Information  . . . . . . . . .     ");


        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_PREFERENCES | USER_READ_GENERAL,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserPreferencesInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                if ( (((USER_PREFERENCES_INFORMATION *)Buffer)->UserComment.MaximumLength
                    >= 0)
                         ) {

                    printf("Succeeded\n");

                    printf("     User Comment  is:    *%wZ*\n",
                     &(((USER_PREFERENCES_INFORMATION *)Buffer)->UserComment) );

                } else {
                    printf("Failed\n");
                    printf("        One of the UNICODE_STRINGs not returned.\n");
                    TestStatus = FALSE;
                }
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );





        printf("      Query User Home Directory Information . . . . . . . .     ");


        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_LOGON,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserHomeInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                if ( (((USER_HOME_INFORMATION *)Buffer)->HomeDirectory.MaximumLength
                    >= 0) &&
                     (((USER_HOME_INFORMATION *)Buffer)->HomeDirectoryDrive.MaximumLength
                     >= 0)
                         ) {

                    printf("Succeeded\n");

                    printf("    Home Directory is:    *%wZ*\n",
                     &(((USER_HOME_INFORMATION *)Buffer)->HomeDirectory) );
                    printf("    Home Directory Drive is:    *%wZ*\n",
                     &(((USER_HOME_INFORMATION *)Buffer)->HomeDirectoryDrive) );


                } else {
                    printf("Failed\n");
                    printf("        String not returned.\n");
                    TestStatus = FALSE;
                }
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );




        printf("      Query User Script Path Information  . . . . . . . . .     ");


        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_LOGON,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserScriptInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                if ( (((USER_SCRIPT_INFORMATION *)Buffer)->ScriptPath.MaximumLength
                    >= 0)
                         ) {

                    printf("Succeeded\n");

                    printf("      Script Path  is:    *%wZ*\n",
                     &(((USER_SCRIPT_INFORMATION *)Buffer)->ScriptPath) );


                } else {
                    printf("Failed\n");
                    printf("        String not returned.\n");
                    TestStatus = FALSE;
                }
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );



        printf("      Query User ProfilePath Information  . . . . . . . . .     ");


        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_LOGON,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserProfileInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                if ( (((USER_PROFILE_INFORMATION *)Buffer)->ProfilePath.MaximumLength
                    >= 0)
                         ) {

                    printf("Succeeded\n");

                    printf("      Profile Path  is:    *%wZ*\n",
                     &(((USER_PROFILE_INFORMATION *)Buffer)->ProfilePath) );


                } else {
                    printf("Failed\n");
                    printf("        String not returned.\n");
                    TestStatus = FALSE;
                }
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );



        printf("      Query User Logon Information  . . . . . . . . . . . .     ");


        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_ACCOUNT | USER_READ_GENERAL | USER_READ_PREFERENCES | USER_READ_LOGON,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserLogonInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                if ( (((USER_LOGON_INFORMATION *)Buffer)->UserName.MaximumLength > 0)       &&
                     (((USER_LOGON_INFORMATION *)Buffer)->UserName.Buffer != NULL)
                         ) {

                    printf("Succeeded\n");

                    printf("          User RID is:    0x%lx\n",
                     (((USER_LOGON_INFORMATION *)Buffer)->UserId) );
                    printf("     Primary Group is:    0x%lx\n",
                     (((USER_LOGON_INFORMATION *)Buffer)->PrimaryGroupId) );
                    printf("      Logon Units are:    0x%lx\n",
                     (((USER_LOGON_INFORMATION *)Buffer)->LogonHours.UnitsPerWeek) );
                    printf("     Bad PWD count is:    0x%lx\n",
                     (((USER_LOGON_INFORMATION *)Buffer)->BadPasswordCount) );
                    printf("       Logon count is:    0x%lx\n",
                     (((USER_LOGON_INFORMATION *)Buffer)->LogonCount) );

                    printf("        last Logon is:    (0x%lx, 0x%lx)\n",
                     (((USER_LOGON_INFORMATION *)Buffer)->LastLogon.HighPart),
                     (((USER_LOGON_INFORMATION *)Buffer)->LastLogon.LowPart)  );
                    printf("       last Logoff is:    (0x%lx, 0x%lx)\n",
                     (((USER_LOGON_INFORMATION *)Buffer)->LastLogoff.HighPart),
                     (((USER_LOGON_INFORMATION *)Buffer)->LastLogoff.LowPart)  );


                    printf("        User  Name is:    *%wZ*\n",
                     &(((USER_LOGON_INFORMATION *)Buffer)->UserName) );
                    printf("        Full  Name is:    *%wZ*\n",
                     &(((USER_LOGON_INFORMATION *)Buffer)->FullName) );
                    printf("          Home Dir is:    *%wZ*\n",
                     &(((USER_LOGON_INFORMATION *)Buffer)->HomeDirectory) );
                    printf("    Home Dir Drive is:    *%wZ*\n",
                     &(((USER_LOGON_INFORMATION *)Buffer)->HomeDirectoryDrive) );
                    printf("      Script Path  is:    *%wZ*\n",
                     &(((USER_LOGON_INFORMATION *)Buffer)->ScriptPath) );
                    printf("      Profile Path is:    *%wZ*\n",
                     &(((USER_LOGON_INFORMATION *)Buffer)->ProfilePath) );
                    printf("     WorkStations are:    *%wZ*\n",
                     &(((USER_LOGON_INFORMATION *)Buffer)->WorkStations) );




                } else {
                    printf("Failed\n");
                    printf("        One of the UNICODE_STRINGs  not returned.\n");
                    TestStatus = FALSE;
                }
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );




        printf("      Query User Logon Hours  . . . . . . . . . . . . . . .     ");


        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_LOGON,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserLogonHoursInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                printf("Succeeded\n");

                printf("      Logon Units are:    0x%lx\n",
                 (((USER_LOGON_HOURS_INFORMATION *)Buffer)->LogonHours.UnitsPerWeek) );


                SamFreeMemory( Buffer );

            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );




        printf("      Query Account Information . . . . . . . . . . . . . .     ");

        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_GENERAL | USER_READ_PREFERENCES |
                       USER_READ_LOGON   | USER_READ_ACCOUNT,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserAccountInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                if ( (((USER_ACCOUNT_INFORMATION *)Buffer)->UserName.MaximumLength > 0)       &&
                     (((USER_ACCOUNT_INFORMATION *)Buffer)->UserName.Buffer != NULL)
                         ) {

                    printf("Succeeded\n");

                    printf("          User RID is:    0x%lx\n",
                     (((USER_ACCOUNT_INFORMATION *)Buffer)->UserId) );
                    printf("     Primary Group is:    0x%lx\n",
                     (((USER_ACCOUNT_INFORMATION *)Buffer)->PrimaryGroupId) );
                    printf("      Logon Units are:    0x%lx\n",
                     (((USER_ACCOUNT_INFORMATION *)Buffer)->LogonHours.UnitsPerWeek) );
                    printf("     Bad PWD count is:    0x%lx\n",
                     (((USER_ACCOUNT_INFORMATION *)Buffer)->BadPasswordCount) );
                    printf("       Logon count is:    0x%lx\n",
                     (((USER_ACCOUNT_INFORMATION *)Buffer)->LogonCount) );
                    printf("      Account Ctrl is:    0x%lx\n",
                     (((USER_ACCOUNT_INFORMATION *)Buffer)->UserAccountControl) );

                    printf("        last Logon is:    (0x%lx, 0x%lx)\n",
                     (((USER_ACCOUNT_INFORMATION *)Buffer)->LastLogon.HighPart),
                     (((USER_ACCOUNT_INFORMATION *)Buffer)->LastLogon.LowPart)  );
                    printf("       last Logoff is:    (0x%lx, 0x%lx)\n",
                     (((USER_ACCOUNT_INFORMATION *)Buffer)->LastLogoff.HighPart),
                     (((USER_ACCOUNT_INFORMATION *)Buffer)->LastLogoff.LowPart)  );
                    printf("      Pwd Last Set is:    (0x%lx, 0x%lx)\n",
                     (((USER_ACCOUNT_INFORMATION *)Buffer)->PasswordLastSet.HighPart),
                     (((USER_ACCOUNT_INFORMATION *)Buffer)->PasswordLastSet.LowPart)  );
                    printf("   Account Expires is:    (0x%lx, 0x%lx)\n",
                     (((USER_ACCOUNT_INFORMATION *)Buffer)->AccountExpires.HighPart),
                     (((USER_ACCOUNT_INFORMATION *)Buffer)->AccountExpires.LowPart)  );


                    printf("        User  Name is:    *%wZ*\n",
                     &(((USER_ACCOUNT_INFORMATION *)Buffer)->UserName) );
                    printf("        Full  Name is:    *%wZ*\n",
                     &(((USER_ACCOUNT_INFORMATION *)Buffer)->FullName) );
                    printf("          Home Dir is:    *%wZ*\n",
                     &(((USER_ACCOUNT_INFORMATION *)Buffer)->HomeDirectory) );
                    printf("    Home Dir Drive is:    *%wZ*\n",
                     &(((USER_ACCOUNT_INFORMATION *)Buffer)->HomeDirectoryDrive) );
                    printf("      Script Path  is:    *%wZ*\n",
                     &(((USER_ACCOUNT_INFORMATION *)Buffer)->ScriptPath) );
                    printf("     Profile Path  is:    *%wZ*\n",
                     &(((USER_ACCOUNT_INFORMATION *)Buffer)->ProfilePath) );
                    printf("     Admin Comment is:    *%wZ*\n",
                     &(((USER_ACCOUNT_INFORMATION *)Buffer)->AdminComment) );
                    printf("     WorkStations are:    *%wZ*\n",
                     &(((USER_ACCOUNT_INFORMATION *)Buffer)->WorkStations) );



                } else {
                    printf("Failed\n");
                    printf("        One of the UNICODE_STRINGs  not returned.\n");
                    TestStatus = FALSE;
                }
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );








        printf("      Query Workstations Information  . . . . . . . . . . .     ");

        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_LOGON,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserWorkStationsInformation,
                       &Buffer
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                if ( (((USER_WORKSTATIONS_INFORMATION *)Buffer)->WorkStations.MaximumLength
                    >= 0)
                         ) {

                    printf("Succeeded\n");

                    printf("      Workstations is:    *%wZ*\n",
                     &(((USER_WORKSTATIONS_INFORMATION *)Buffer)->WorkStations) );


                } else {
                    printf("Failed\n");
                    printf("        String not returned.\n");
                    TestStatus = FALSE;
                }
                SamFreeMemory( Buffer );
            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );





        printf("      Query Internal1 Information  . . . . . . . . . . .     ");

        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_LOGON,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserInternal1Information,
                       &Buffer
                       );

        if ( NtStatus == STATUS_INVALID_INFO_CLASS ) {

            //
            // We're not a trusted client, so we expected this to fail.
            //

            printf("Succeeded\n");

        } else {

            printf("Failed\n");
            printf("        Status was %lx.\n", NtStatus );
            TestStatus = FALSE;
            if ( NT_SUCCESS( NtStatus ) ) {

                SamFreeMemory( Buffer );
            }
        }

// This is the code that USED to test this function, when it was allowed
// for non-trusted clients.
//
//        if (NT_SUCCESS(NtStatus)) {
//            if (Buffer != NULL) {
//
//                if ( (((USER_INTERNAL1_INFORMATION *)Buffer)->CaseInsensitiveDbcs.MaximumLength > 0) &&
//                     (((USER_INTERNAL1_INFORMATION *)Buffer)->CaseInsensitiveDbcs.Buffer != NULL) &&
//                     (((USER_INTERNAL1_INFORMATION *)Buffer)->CaseSensitiveUnicode.MaximumLength > 0) &&
//                     (((USER_INTERNAL1_INFORMATION *)Buffer)->CaseSensitiveUnicode.Buffer != NULL)
//                         ) {
//
//                     printf("Succeeded\n");
//
//                     //
//                     // Print them out as strings, even though they've been
//                     // through a OWF.
//                     //
//
//                     printf("      CaseInsensitiveDbcs is:    *%s*\n",
//                      &(((USER_INTERNAL1_INFORMATION *)Buffer)->CaseInsensitiveDbcs) );
//
//                     printf("      CaseSensitiveUnicode is:    *%s*\n",
//                      &(((USER_INTERNAL1_INFORMATION *)Buffer)->CaseSensitiveUnicode) );
//
//
//                } else {
//                    printf("Failed\n");
//                    printf("        One of the strings not returned.\n");
//                    TestStatus = FALSE;
//                }
//                SamFreeMemory( Buffer );
//            } else {
//                printf("Failed\n");
//                printf("        Buffer address not set on return.\n");
//                printf("        RPC should have allocated a buffer.\n");
//                TestStatus = FALSE;
//            }
//        } else {
//            printf("Failed\n");
//            printf("        Completion status is 0x%lx\n", NtStatus);
//            TestStatus = FALSE;
//        }

        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );





        printf("      Query Internal2 Information  . . . . . . . . . . .     ");

        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_LOGON,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserInternal2Information,
                       &Buffer
                       );

        if ( NtStatus == STATUS_INVALID_INFO_CLASS ) {

            //
            // We're not a trusted client, so we don't expect to be able
            // to do this.
            //

            printf("Succeeded.\n");

        } else {

            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
            SamFreeMemory( Buffer );
        }

// This is the code that USED to test this function, when non-trusted
// clients were allowed to do this...
//
//        if (NT_SUCCESS(NtStatus)) {
//            if (Buffer != NULL) {
//
//                printf("Succeeded\n");
//
//                printf("        last Logon is:    (0x%lx, 0x%lx)\n",
//                 (((USER_INTERNAL2_INFORMATION *)Buffer)->LastLogon.HighPart),
//                 (((USER_INTERNAL2_INFORMATION *)Buffer)->LastLogon.LowPart)  );
//                printf("       last Logoff is:    (0x%lx, 0x%lx)\n",
//                 (((USER_INTERNAL2_INFORMATION *)Buffer)->LastLogoff.HighPart),
//                 (((USER_INTERNAL2_INFORMATION *)Buffer)->LastLogoff.LowPart)  );
//                printf("       BadPwdCount is:    (0x%x)\n",
//                 ((USER_INTERNAL2_INFORMATION *)Buffer)->BadPasswordCount );
//                printf("       LogonCount  is:    (0x%x)\n",
//                 ((USER_INTERNAL2_INFORMATION *)Buffer)->LogonCount );
//
//                SamFreeMemory( Buffer );
//            } else {
//                printf("Failed\n");
//                printf("        Buffer address not set on return.\n");
//                printf("        RPC should have allocated a buffer.\n");
//                TestStatus = FALSE;
//            }
//        } else {
//            printf("Failed\n");
//            printf("        Completion status is 0x%lx\n", NtStatus);
//            TestStatus = FALSE;
//        }

        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );





        printf("      Query Set Password Information  . . . . . . . . . . .     ");




        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_READ_LOGON,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserSetPasswordInformation,
                       &Buffer
                       );
        if (NtStatus == STATUS_INVALID_INFO_CLASS ) {

            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            printf("        Expected 0x%lx (INVALID_INFO_CLASS)\n", STATUS_INVALID_INFO_CLASS);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );




        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Get Groups For User Suite                                             //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("\n");
        printf("    Get Groups For User . . . . . . . . . . . . . . . . .   Suite\n");

        printf("      Get Groups For Well-Known Account . . . . . . . . . .     ");

        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_LIST_GROUPS,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;
        NtStatus = SamGetGroupsForUser(
                       UserHandle1,
                       (PGROUP_MEMBERSHIP *)&Buffer,
                       &MembershipCount
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (Buffer != NULL) {

                printf("Succeeded\n");


                printf("          Member of:    %d groups\n", MembershipCount);
                for ( i=0; i<MembershipCount; i++) {

                    printf("      Group[%d] Rid/Attributes:      0x%lx/0x%lx\n",
                        i,
                        (((PGROUP_MEMBERSHIP)Buffer)[i].RelativeId),
                        (((PGROUP_MEMBERSHIP)Buffer)[i].Attributes)
                        );

                }

                SamFreeMemory( Buffer );


            } else {
                printf("Failed\n");
                printf("        Buffer address not set on return.\n");
                printf("        RPC should have allocated a buffer.\n");
                TestStatus = FALSE;
            }
        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );



        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Set User Suite                                                        //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("\n");
        printf("    Set User  . . . . . . . . . . . . . . . . . . . . . .   Suite\n");

        printf("      Set General Information . . . . . . . . . . . . . . .     ");
        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_ALL_ACCESS,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        //
        // Make the parameter marshallable, but don't worry about values.
        //

        GeneralInformation.UserName = DummyName1;
        GeneralInformation.FullName = DummyName1;
        GeneralInformation.AdminComment = DummyName1;
        GeneralInformation.UserComment  = DummyName1;

        Buffer = &GeneralInformation;
        NtStatus = SamSetInformationUser(
                       UserHandle1,
                       UserGeneralInformation,
                       Buffer
                       );
        if (NtStatus == STATUS_INVALID_INFO_CLASS ) {

            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            printf("        Expected 0x%lx (INVALID_INFO_CLASS)\n", STATUS_INVALID_INFO_CLASS);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );



        printf("      Set Preferences Information . . . . . . . . . . . . .     ");
            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_READ_GENERAL | USER_WRITE_PREFERENCES | USER_READ_PREFERENCES,
                           DOMAIN_USER_RID_ADMIN,
                           &UserHandle1
                           );
            ASSERT(NT_SUCCESS(NtStatus) );

            //
            // Get the current value...
            //

            Buffer1 = NULL;
            NtStatus = SamQueryInformationUser(
                           UserHandle1,
                           UserPreferencesInformation,
                           &Buffer1
                           );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(Buffer1 != NULL);


            //
            // Change the fields to  new values and write them out.
            //

            NameLength = ((USER_PREFERENCES_INFORMATION *)Buffer1)->UserComment.Length;
            if (  NameLength == DummyString1.Length ) {
                ((USER_PREFERENCES_INFORMATION *)Buffer1)->UserComment = DummyString2;
            } else {
                ((USER_PREFERENCES_INFORMATION *)Buffer1)->UserComment = DummyString1;
            }

            ((USER_PREFERENCES_INFORMATION *)Buffer1)->CountryCode += 1;
            ((USER_PREFERENCES_INFORMATION *)Buffer1)->CodePage += 1;

            NtStatus = SamSetInformationUser(
                           UserHandle1,
                           UserPreferencesInformation,
                           Buffer1
                           );
            if ( NT_SUCCESS(NtStatus) ) {

                //
                // Now check that the change was really made...
                //

                NtStatus = SamQueryInformationUser(
                               UserHandle1,
                               UserPreferencesInformation,
                               &Buffer2
                               );
                ASSERT(NT_SUCCESS( NtStatus ) );
                if (
                    !RtlCompareString(
                        (PSTRING)&((USER_PREFERENCES_INFORMATION *)Buffer1)->UserComment,
                        (PSTRING)&((USER_PREFERENCES_INFORMATION *)Buffer2)->UserComment,
                        TRUE)
                        &&
                        (((USER_PREFERENCES_INFORMATION *)Buffer1)->CountryCode ==
                         ((USER_PREFERENCES_INFORMATION *)Buffer2)->CountryCode)
                        &&
                        (((USER_PREFERENCES_INFORMATION *)Buffer1)->CodePage ==
                         ((USER_PREFERENCES_INFORMATION *)Buffer2)->CodePage)
                    ) {

                    printf("Succeeded\n");

                    //
                    // Change back some fields to keep from screwing up our database
                    //

                    ((USER_PREFERENCES_INFORMATION *)Buffer1)->CountryCode -= 1;
                    ((USER_PREFERENCES_INFORMATION *)Buffer1)->CodePage    -= 1;

                    IgnoreStatus = SamSetInformationUser(
                                       UserHandle1,
                                       UserPreferencesInformation,
                                       Buffer1
                                       );
                    ASSERT(NT_SUCCESS(IgnoreStatus));

                } else {

                    printf("Failed\n");
                    printf("        Values queried don't match values written\n");
                    printf("        UserComment Written is   %wZ\n",
                        (PUNICODE_STRING)&((USER_PREFERENCES_INFORMATION *)Buffer1)->UserComment);
                    printf("        UserComment Retrieved is %wZ\n",
                        (PUNICODE_STRING)&((USER_PREFERENCES_INFORMATION *)Buffer2)->UserComment);
                    printf("        CountryCode Written is   0x%lx\n",
                        (ULONG)((USER_PREFERENCES_INFORMATION *)Buffer1)->CountryCode);
                    printf("        CountryCode Retrieved is 0x%lx\n",
                        (ULONG)((USER_PREFERENCES_INFORMATION *)Buffer2)->CountryCode);
                    printf("        CodePage Written is   0x%lx\n",
                        (ULONG)((USER_PREFERENCES_INFORMATION *)Buffer1)->CodePage);
                    printf("        CodePage Retrieved is 0x%lx\n",
                        (ULONG)((USER_PREFERENCES_INFORMATION *)Buffer2)->CodePage);

                    TestStatus = FALSE;

                }

                SamFreeMemory( Buffer1 );
                SamFreeMemory( Buffer2 );

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
                SamFreeMemory( Buffer1 );

            }




        printf("      Set Logon Information . . . . . . . . . . . . . . . .     ");
        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_ALL_ACCESS,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        //
        // Make the parameter marshallable, but don't worry about values.
        //

        LogonInformation.UserName       = DummyName1;
        LogonInformation.FullName       = DummyName1;
        LogonInformation.HomeDirectory  = DummyName1;
        LogonInformation.HomeDirectoryDrive = DummyName1;
        LogonInformation.ScriptPath     = DummyName1;
        LogonInformation.ProfilePath    = DummyName1;
        LogonInformation.WorkStations   = DummyName1;

        LogonInformation.LogonHours     = DummyLogonHours;

        Buffer = &LogonInformation;
        NtStatus = SamSetInformationUser(
                       UserHandle1,
                       UserLogonInformation,
                       Buffer
                       );
        if (NtStatus == STATUS_INVALID_INFO_CLASS ) {

            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            printf("        Expected 0x%lx (INVALID_INFO_CLASS)\n", STATUS_INVALID_INFO_CLASS);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );



        printf("      Set Logon Hours Information . . . . . . . . . . . . .     ");
            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_WRITE_ACCOUNT | USER_READ_LOGON,
                           DOMAIN_USER_RID_ADMIN,
                           &UserHandle1
                           );
            ASSERT(NT_SUCCESS(NtStatus) );

            //
            // Get the current value...
            //

            Buffer1 = NULL;
            NtStatus = SamQueryInformationUser(
                           UserHandle1,
                           UserLogonHoursInformation,
                           &Buffer1
                           );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(Buffer1 != NULL);
            ASSERT( ((USER_LOGON_HOURS_INFORMATION *)Buffer1)->LogonHours.LogonHours
                    != NULL);  //Don't support zero length bit masks in this test yet.


            //
            // Change the field to a new value and write it out.
            // We have two choices for out test:
            //                                      NoLogonRestriction
            //                                      DummyLogonHours
            //
            // They are guaranteed to have different values in the
            // LOGON_HOURS_DIFFERENT_OFFSET byte of their respective bit masks.
            //

            if ( 0 == ((USER_LOGON_HOURS_INFORMATION *)Buffer1)->LogonHours.LogonHours[LOGON_HOURS_DIFFERENT_OFFSET]) {
                ((USER_LOGON_HOURS_INFORMATION *)Buffer1)->LogonHours = DummyLogonHours;
            } else {
                ((USER_LOGON_HOURS_INFORMATION *)Buffer1)->LogonHours = NoLogonRestriction;
            }

            NtStatus = SamSetInformationUser(
                           UserHandle1,
                           UserLogonHoursInformation,
                           Buffer1
                           );
            if ( NT_SUCCESS(NtStatus) ) {

                //
                // Now check that the change was really made...
                //

                NtStatus = SamQueryInformationUser(
                               UserHandle1,
                               UserLogonHoursInformation,
                               &Buffer2
                               );
                ASSERT(NT_SUCCESS( NtStatus ) );
                if (
                    ((USER_LOGON_HOURS_INFORMATION *)Buffer1)->LogonHours.LogonHours[LOGON_HOURS_DIFFERENT_OFFSET]
                    ==
                    ((USER_LOGON_HOURS_INFORMATION *)Buffer2)->LogonHours.LogonHours[LOGON_HOURS_DIFFERENT_OFFSET]
                    ) {

                    printf("Succeeded\n");

                } else {

                    printf("Failed\n");
                    printf("        Value queried doesn't match value written\n");
                    printf("        Units Written are   0x%lx\n",
                        ((USER_LOGON_HOURS_INFORMATION *)Buffer1)->LogonHours.UnitsPerWeek);
                    printf("        Units Retrieved are 0x%lx\n",
                        ((USER_LOGON_HOURS_INFORMATION *)Buffer2)->LogonHours.UnitsPerWeek);

                    printf("        Byte 0x%lx of the written bit mask is    0x%lx\n",
                        LOGON_HOURS_DIFFERENT_OFFSET,
                        (ULONG)((USER_LOGON_HOURS_INFORMATION *)Buffer1)->LogonHours.LogonHours[LOGON_HOURS_DIFFERENT_OFFSET]
                        );
                    printf("        Byte 0x%lx of the retrieved bit mask is  0x%lx\n",
                        LOGON_HOURS_DIFFERENT_OFFSET,
                        (ULONG)((USER_LOGON_HOURS_INFORMATION *)Buffer2)->LogonHours.LogonHours[LOGON_HOURS_DIFFERENT_OFFSET]
                        );

                    TestStatus = FALSE;

                }

                SamFreeMemory( Buffer1 );
                SamFreeMemory( Buffer2 );

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
                SamFreeMemory( Buffer1 );

            }





        printf("      Set Account Information . . . . . . . . . . . . . . .     ");
        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_WRITE_ACCOUNT        |
                           USER_READ_GENERAL     |
                           USER_READ_PREFERENCES |
                           USER_READ_LOGON,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        //
        // Make the parameter marshallable, but don't worry about values.
        //

        AccountInformation.UserName       = DummyName1;
        AccountInformation.FullName       = DummyName1;
        AccountInformation.HomeDirectory  = DummyName1;
        AccountInformation.HomeDirectoryDrive = DummyName1;
        AccountInformation.ScriptPath     = DummyName1;
        AccountInformation.ProfilePath    = DummyName1;
        AccountInformation.AdminComment   = DummyName1;
        AccountInformation.WorkStations   = DummyName1;

        AccountInformation.LogonHours     = DummyLogonHours;

        Buffer = &AccountInformation;
        NtStatus = SamSetInformationUser(
                       UserHandle1,
                       UserAccountInformation,
                       Buffer
                       );
        if (NtStatus == STATUS_INVALID_INFO_CLASS ) {

            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            printf("        Expected 0x%lx (INVALID_INFO_CLASS)\n", STATUS_INVALID_INFO_CLASS);
            TestStatus = FALSE;
        }
        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );


        printf("      Set Home  . . . . . . . . . . . . . . . . . . . . . .     ");
            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_WRITE_ACCOUNT | USER_READ_LOGON,
                           DOMAIN_USER_RID_ADMIN,
                           &UserHandle1
                           );
            ASSERT(NT_SUCCESS(NtStatus) );

            //
            // Get the current value...
            //

            Buffer1 = NULL;
            NtStatus = SamQueryInformationUser(
                           UserHandle1,
                           UserHomeInformation,
                           &Buffer1
                           );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(Buffer1 != NULL);


            //
            // Change the field to a new value and write it out.
            //

            NameLength = ((USER_HOME_INFORMATION *)Buffer1)->HomeDirectory.Length;
            if (  NameLength == DummyString1.Length ) {
                ((USER_HOME_INFORMATION *)Buffer1)->HomeDirectory = DummyString2;
            } else {
                ((USER_HOME_INFORMATION *)Buffer1)->HomeDirectory = DummyString1;
            }

            NameLength = ((USER_HOME_INFORMATION *)Buffer1)->HomeDirectoryDrive.Length;
            if (  NameLength == DummyString1.Length ) {
                ((USER_HOME_INFORMATION *)Buffer1)->HomeDirectoryDrive = DummyString2;
            } else {
                ((USER_HOME_INFORMATION *)Buffer1)->HomeDirectoryDrive = DummyString1;
            }

            NtStatus = SamSetInformationUser(
                           UserHandle1,
                           UserHomeInformation,
                           Buffer1
                           );
            if ( NT_SUCCESS(NtStatus) ) {

                //
                // Now check that the change was really made...
                //

                NtStatus = SamQueryInformationUser(
                               UserHandle1,
                               UserHomeInformation,
                               &Buffer2
                               );
                ASSERT(NT_SUCCESS( NtStatus ) );

                if (!RtlCompareString(
                        (PSTRING)&((USER_HOME_INFORMATION *)Buffer1)->HomeDirectory,
                        (PSTRING)&((USER_HOME_INFORMATION *)Buffer2)->HomeDirectory,
                        TRUE) ) {

                    if (!RtlCompareString(
                            (PSTRING)&((USER_HOME_INFORMATION *)Buffer1)->HomeDirectoryDrive,
                            (PSTRING)&((USER_HOME_INFORMATION *)Buffer2)->HomeDirectoryDrive,
                            TRUE)
                    ) {
                        printf("Succeeded\n");
                    } else {

                        printf("Failed\n");
                        printf("        Drive Value queried doesn't match value written\n");
                        printf("        Value Written is   %wZ\n",
                            (PUNICODE_STRING)&((USER_HOME_INFORMATION *)Buffer1)->HomeDirectoryDrive);
                        printf("        Value Retrieved is %wZ\n",
                            (PUNICODE_STRING)&((USER_HOME_INFORMATION *)Buffer2)->HomeDirectoryDrive);

                        TestStatus = FALSE;
                    }

                } else {

                    printf("Failed\n");
                    printf("        Directory Value queried doesn't match value written\n");
                    printf("        Value Written is   %wZ\n",
                        (PUNICODE_STRING)&((USER_HOME_INFORMATION *)Buffer1)->HomeDirectory);
                    printf("        Value Retrieved is %wZ\n",
                        (PUNICODE_STRING)&((USER_HOME_INFORMATION *)Buffer2)->HomeDirectory);

                    TestStatus = FALSE;

                }

                SamFreeMemory( Buffer1 );
                SamFreeMemory( Buffer2 );

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
                SamFreeMemory( Buffer1 );

            }




        printf("      Set Script  . . . . . . . . . . . . . . . . . . . . .     ");
            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_WRITE_ACCOUNT | USER_READ_LOGON,
                           DOMAIN_USER_RID_ADMIN,
                           &UserHandle1
                           );
            ASSERT(NT_SUCCESS(NtStatus) );

            //
            // Get the current value...
            //

            Buffer1 = NULL;
            NtStatus = SamQueryInformationUser(
                           UserHandle1,
                           UserScriptInformation,
                           &Buffer1
                           );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(Buffer1 != NULL);


            //
            // Change the field to a new value and write it out.
            //

            NameLength = ((USER_SCRIPT_INFORMATION *)Buffer1)->ScriptPath.Length;
            if (  NameLength == DummyString1.Length ) {
                ((USER_SCRIPT_INFORMATION *)Buffer1)->ScriptPath = DummyString2;
            } else {
                ((USER_SCRIPT_INFORMATION *)Buffer1)->ScriptPath = DummyString1;
            }

            NtStatus = SamSetInformationUser(
                           UserHandle1,
                           UserScriptInformation,
                           Buffer1
                           );
            if ( NT_SUCCESS(NtStatus) ) {

                //
                // Now check that the change was really made...
                //

                NtStatus = SamQueryInformationUser(
                               UserHandle1,
                               UserScriptInformation,
                               &Buffer2
                               );
                ASSERT(NT_SUCCESS( NtStatus ) );
                if (
                    !RtlCompareString(
                        (PSTRING)&((USER_SCRIPT_INFORMATION *)Buffer1)->ScriptPath,
                        (PSTRING)&((USER_SCRIPT_INFORMATION *)Buffer2)->ScriptPath,
                        TRUE)
                    ) {

                    printf("Succeeded\n");

                } else {

                    printf("Failed\n");
                    printf("        Value queried doesn't match value written\n");
                    printf("        Value Written is   %wZ\n",
                        (PUNICODE_STRING)&((USER_SCRIPT_INFORMATION *)Buffer1)->ScriptPath);
                    printf("        Value Retrieved is %wZ\n",
                        (PUNICODE_STRING)&((USER_SCRIPT_INFORMATION *)Buffer2)->ScriptPath);

                    TestStatus = FALSE;

                }

                SamFreeMemory( Buffer1 );
                SamFreeMemory( Buffer2 );

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
                SamFreeMemory( Buffer1 );

            }




        printf("      Set Profile . . . . . . . . . . . . . . . . . . . . .     ");
            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_WRITE_ACCOUNT | USER_READ_LOGON,
                           DOMAIN_USER_RID_ADMIN,
                           &UserHandle1
                           );
            ASSERT(NT_SUCCESS(NtStatus) );

            //
            // Get the current value...
            //

            Buffer1 = NULL;
            NtStatus = SamQueryInformationUser(
                           UserHandle1,
                           UserProfileInformation,
                           &Buffer1
                           );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(Buffer1 != NULL);


            //
            // Change the field to a new value and write it out.
            //

            NameLength = ((USER_PROFILE_INFORMATION *)Buffer1)->ProfilePath.Length;
            if (  NameLength == DummyString1.Length ) {
                ((USER_PROFILE_INFORMATION *)Buffer1)->ProfilePath = DummyString2;
            } else {
                ((USER_PROFILE_INFORMATION *)Buffer1)->ProfilePath = DummyString1;
            }

            NtStatus = SamSetInformationUser(
                           UserHandle1,
                           UserProfileInformation,
                           Buffer1
                           );
            if ( NT_SUCCESS(NtStatus) ) {

                //
                // Now check that the change was really made...
                //

                NtStatus = SamQueryInformationUser(
                               UserHandle1,
                               UserProfileInformation,
                               &Buffer2
                               );
                ASSERT(NT_SUCCESS( NtStatus ) );
                if (
                    !RtlCompareString(
                        (PSTRING)&((USER_PROFILE_INFORMATION *)Buffer1)->ProfilePath,
                        (PSTRING)&((USER_PROFILE_INFORMATION *)Buffer2)->ProfilePath,
                        TRUE)
                    ) {

                    printf("Succeeded\n");

                } else {

                    printf("Failed\n");
                    printf("        Value queried doesn't match value written\n");
                    printf("        Value Written is   %wZ\n",
                        (PUNICODE_STRING)&((USER_PROFILE_INFORMATION *)Buffer1)->ProfilePath);
                    printf("        Value Retrieved is %wZ\n",
                        (PUNICODE_STRING)&((USER_PROFILE_INFORMATION *)Buffer2)->ProfilePath);

                    TestStatus = FALSE;

                }

                SamFreeMemory( Buffer1 );
                SamFreeMemory( Buffer2 );

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
                SamFreeMemory( Buffer1 );

            }




        printf("      Set Admin Comment . . . . . . . . . . . . . . . . . .     ");

            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_WRITE_ACCOUNT | USER_READ_GENERAL,
                           DOMAIN_USER_RID_ADMIN,
                           &UserHandle1
                           );
            ASSERT(NT_SUCCESS(NtStatus) );

            //
            // Get the current value...
            //

            Buffer1 = NULL;
            NtStatus = SamQueryInformationUser(
                           UserHandle1,
                           UserAdminCommentInformation,
                           &Buffer1
                           );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(Buffer1 != NULL);


            //
            // Change the field to a new value and write it out.
            //

            NameLength = ((USER_ADMIN_COMMENT_INFORMATION *)Buffer1)->AdminComment.Length;
            if (  NameLength == DummyString1.Length ) {
                ((USER_ADMIN_COMMENT_INFORMATION *)Buffer1)->AdminComment = DummyString2;
            } else {
                ((USER_ADMIN_COMMENT_INFORMATION *)Buffer1)->AdminComment = DummyString1;
            }

            NtStatus = SamSetInformationUser(
                           UserHandle1,
                           UserAdminCommentInformation,
                           Buffer1
                           );
            if ( NT_SUCCESS(NtStatus) ) {

                //
                // Now check that the change was really made...
                //

                NtStatus = SamQueryInformationUser(
                               UserHandle1,
                               UserAdminCommentInformation,
                               &Buffer2
                               );
                ASSERT(NT_SUCCESS( NtStatus ) );
                if (
                    !RtlCompareString(
                        (PSTRING)&((USER_ADMIN_COMMENT_INFORMATION *)Buffer1)->AdminComment,
                        (PSTRING)&((USER_ADMIN_COMMENT_INFORMATION *)Buffer2)->AdminComment,
                        TRUE)
                    ) {

                    printf("Succeeded\n");

                } else {

                    printf("Failed\n");
                    printf("        Value queried doesn't match value written\n");
                    printf("        Value Written is   %wZ\n",
                        (PUNICODE_STRING)&((USER_ADMIN_COMMENT_INFORMATION *)Buffer1)->AdminComment);
                    printf("        Value Retrieved is %wZ\n",
                        (PUNICODE_STRING)&((USER_ADMIN_COMMENT_INFORMATION *)Buffer2)->AdminComment);

                    TestStatus = FALSE;

                }

                SamFreeMemory( Buffer1 );
                SamFreeMemory( Buffer2 );

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
                SamFreeMemory( Buffer1 );

            }


        printf("      Set Workstations  . . . . . . . . . . . . . . . . . .     ");
        printf("BROKEN TEST - NOT TESTED\n");
#ifdef BROKEN
            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_WRITE_ACCOUNT | USER_READ_LOGON,
                           DOMAIN_USER_RID_ADMIN,
                           &UserHandle1
                           );
            ASSERT(NT_SUCCESS(NtStatus) );

            //
            // Get the current value...
            //

            Buffer1 = NULL;
            NtStatus = SamQueryInformationUser(
                           UserHandle1,
                           UserWorkStationsInformation,
                           &Buffer1
                           );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(Buffer1 != NULL);


            //
            // Change the field to a new value and write it out.
            //

            NameLength = ((USER_WORKSTATIONS_INFORMATION *)Buffer1)->WorkStations.Length;
            if (  NameLength == DummyString1.Length ) {
                ((USER_WORKSTATIONS_INFORMATION *)Buffer1)->WorkStations = DummyString2;
            } else {
                ((USER_WORKSTATIONS_INFORMATION *)Buffer1)->WorkStations = DummyString1;
            }

            NtStatus = SamSetInformationUser(
                           UserHandle1,
                           UserWorkStationsInformation,
                           Buffer1
                           );
            if ( NT_SUCCESS(NtStatus) ) {

                //
                // Now check that the change was really made...
                //

                NtStatus = SamQueryInformationUser(
                               UserHandle1,
                               UserWorkStationsInformation,
                               &Buffer2
                               );
                ASSERT(NT_SUCCESS( NtStatus ) );
                if (
                    !RtlCompareString(
                        (PSTRING)&((USER_WORKSTATIONS_INFORMATION *)Buffer1)->WorkStations,
                        (PSTRING)&((USER_WORKSTATIONS_INFORMATION *)Buffer2)->WorkStations,
                        TRUE)
                    ) {

                    printf("Succeeded\n");

                } else {

                    printf("Failed\n");
                    printf("        Value queried doesn't match value written\n");
                    printf("        Value Written is   %wZ\n",
                        (PUNICODE_STRING)&((USER_WORKSTATIONS_INFORMATION *)Buffer1)->WorkStations);
                    printf("        Value Retrieved is %wZ\n",
                        (PUNICODE_STRING)&((USER_WORKSTATIONS_INFORMATION *)Buffer2)->WorkStations);

                    TestStatus = FALSE;

                }

                SamFreeMemory( Buffer1 );
                SamFreeMemory( Buffer2 );

            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
                SamFreeMemory( Buffer1 );

            }
#endif //BROKEN


        printf("      Set Internal1   . . . . . . . . . . . . . . . . . . .     ");

            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_WRITE_ACCOUNT | USER_READ_LOGON | USER_FORCE_PASSWORD_CHANGE,
                           DOMAIN_USER_RID_ADMIN,
                           &UserHandle1
                           );
            ASSERT(NT_SUCCESS(NtStatus) );

            //
            // We can't get the current values, since this level is only
            // queryable by trusted clients.  So just try setting a couple
            // of values and make sure that we don't get an error.
            //

            Buffer1 = RtlAllocateHeap( RtlProcessHeap(), 0, sizeof(USER_INTERNAL1_INFORMATION) );
            ASSERT( Buffer1 != NULL );

            ((PUSER_INTERNAL1_INFORMATION)Buffer1)->NtPasswordPresent = FALSE;
            ((PUSER_INTERNAL1_INFORMATION)Buffer1)->LmPasswordPresent = FALSE;

            NtStatus = SamSetInformationUser(
                           UserHandle1,
                           UserInternal1Information,
                           Buffer1
                           );

            if (NtStatus != STATUS_PASSWORD_RESTRICTION) {

                printf("Failed\n");
                printf("    Expected Status = 0x%lx\n", STATUS_PASSWORD_RESTRICTION);
                printf("    Received Status = 0x%lx\n", NtStatus );
                TestStatus = FALSE;

            } else {

                //
                // The NULL password worked, so let's try a real password.
                //

                NtStatus = RtlCalculateNtOwfPassword(
                    &DummyName1,
                    &((PUSER_INTERNAL1_INFORMATION)Buffer1)->NtOwfPassword
                    );
                ASSERT(NT_SUCCESS(NtStatus));

                ((PUSER_INTERNAL1_INFORMATION)Buffer1)->NtPasswordPresent = TRUE;

                NtStatus = RtlCalculateLmOwfPassword(
                    DUMMY_STRING1,
                    &((PUSER_INTERNAL1_INFORMATION)Buffer1)->LmOwfPassword
                    );
                ASSERT(NT_SUCCESS(NtStatus));

                ((PUSER_INTERNAL1_INFORMATION)Buffer1)->LmPasswordPresent = TRUE;

                NtStatus = SamSetInformationUser(
                               UserHandle1,
                               UserInternal1Information,
                               Buffer1
                               );

                if ( NT_SUCCESS(NtStatus) ) {

                    printf("Succeeded\n");

                } else {

                    printf("Failed\n");
                    printf("    Return status was %lx\n", NtStatus );
                    TestStatus = FALSE;
                }
            }

            RtlFreeHeap( RtlProcessHeap(), 0, Buffer1 );


// This is the code that used to be here, when UserInternal1Information was
// queryable by non-trusted clients...
//
//            Buffer1 = NULL;
//            NtStatus = SamQueryInformationUser(
//                           UserHandle1,
//                           UserInternal1Information,
//                           &Buffer1
//                           );
//            TST_SUCCESS_ASSERT(NtStatus);
//            ASSERT(Buffer1 != NULL);
//
//            //
//            // The passwords were initially empty.  Put in some random
//            // OWF passwords, and have them written out.
//            //
//
//            NtStatus = RtlCalculateNtOwfPassword(
//                (PNT_PASSWORD)&DummyName1,
//                &EncryptedPasswordBuffer
//                );
//
//            ((USER_INTERNAL1_INFORMATION *)Buffer1)->CaseSensitiveUnicode.Buffer = (PCHAR)&EncryptedPasswordBuffer;
//            ((USER_INTERNAL1_INFORMATION *)Buffer1)->CaseSensitiveUnicode.Length = 16;
//            ((USER_INTERNAL1_INFORMATION *)Buffer1)->CaseSensitiveUnicode.MaximumLength = 16;
//
//            NtStatus = RtlCalculateNtOwfPassword(
//                (PNT_PASSWORD)&DummyName2,
//                &EncryptedPasswordBuffer2
//                );
//
//            ((USER_INTERNAL1_INFORMATION *)Buffer1)->CaseInsensitiveDbcs.Buffer = (PCHAR)&EncryptedPasswordBuffer2;
//            ((USER_INTERNAL1_INFORMATION *)Buffer1)->CaseInsensitiveDbcs.Length = 16;
//            ((USER_INTERNAL1_INFORMATION *)Buffer1)->CaseInsensitiveDbcs.MaximumLength = 16;
//
//            NtStatus = SamSetInformationUser(
//                           UserHandle1,
//                           UserInternal1Information,
//                           Buffer1
//                           );
//            if ( NT_SUCCESS(NtStatus) ) {
//
//                //
//                // Now check that the change was really made...
//                //
//
//                NtStatus = SamQueryInformationUser(
//                               UserHandle1,
//                               UserInternal1Information,
//                               &Buffer2
//                               );
//                ASSERT(NT_SUCCESS( NtStatus ) );
//
//                if ( (
//                    !RtlCompareString(
//                        (PSTRING)&((USER_INTERNAL1_INFORMATION *)Buffer1)->CaseSensitiveUnicode,
//                        (PSTRING)&((USER_INTERNAL1_INFORMATION *)Buffer2)->CaseSensitiveUnicode,
//                        TRUE)
//                    ) || (
//                    !RtlCompareString(
//                        (PSTRING)&((USER_INTERNAL1_INFORMATION *)Buffer1)->CaseInsensitiveDbcs,
//                        (PSTRING)&((USER_INTERNAL1_INFORMATION *)Buffer2)->CaseInsensitiveDbcs,
//                        TRUE)
//                    ) ) {
//
//                    printf("Succeeded\n");
//
//                } else {
//
//                    printf("Failed\n");
//                    printf("        Value queried doesn't match value written\n");
//                    printf("        CaseInsensitiveDbcs Written is   %wZ\n",
//                        (PUNICODE_STRING)&((USER_INTERNAL1_INFORMATION *)Buffer1)->CaseInsensitiveDbcs);
//                    printf("        CaseInsensitiveDbcs Retrieved is %wZ\n",
//                        (PUNICODE_STRING)&((USER_INTERNAL1_INFORMATION *)Buffer2)->CaseInsensitiveDbcs);
//                    printf("        CaseSensitiveUnicode Written is   %wZ\n",
//                        (PUNICODE_STRING)&((USER_INTERNAL1_INFORMATION *)Buffer1)->CaseSensitiveUnicode);
//                    printf("        CaseSensitiveUnicode Retrieved is %wZ\n",
//                        (PUNICODE_STRING)&((USER_INTERNAL1_INFORMATION *)Buffer2)->CaseSensitiveUnicode);
//
//                    TestStatus = FALSE;
//
//                }
//
//                SamFreeMemory( Buffer1 );
//                SamFreeMemory( Buffer2 );
//
//            } else {
//                printf("Failed\n");
//                printf("        Completion status is 0x%lx\n", NtStatus);
//                TestStatus = FALSE;
//                SamFreeMemory( Buffer1 );
//
//            }



        printf("      Set Internal2   . . . . . . . . . . . . . . . . . . .     ");

            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_WRITE_ACCOUNT | USER_READ_LOGON,
                           DOMAIN_USER_RID_ADMIN,
                           &UserHandle1
                           );
            ASSERT(NT_SUCCESS(NtStatus) );

            //
            // We can't get the current values, since this level is only
            // queryable by trusted clients.  We can't set either, but
            // try it and make sure we get the correct error.
            //

            Buffer1 = RtlAllocateHeap( RtlProcessHeap(), 0, sizeof(USER_INTERNAL2_INFORMATION) );
            ASSERT( Buffer1 != NULL );

            ((USER_INTERNAL2_INFORMATION *)Buffer1)->LastLogon.HighPart = 1;
            ((USER_INTERNAL2_INFORMATION *)Buffer1)->LastLogoff.HighPart = 2;
            ((USER_INTERNAL2_INFORMATION *)Buffer1)->LastLogon.LowPart = 3;
            ((USER_INTERNAL2_INFORMATION *)Buffer1)->LastLogoff.LowPart = 4;
            ((USER_INTERNAL2_INFORMATION *)Buffer1)->BadPasswordCount = 5;
            ((USER_INTERNAL2_INFORMATION *)Buffer1)->LogonCount = 6;

            NtStatus = SamSetInformationUser(
                           UserHandle1,
                           UserInternal2Information,
                           Buffer1
                           );

            RtlFreeHeap( RtlProcessHeap(), 0, Buffer1 );

            if ( NtStatus == STATUS_INVALID_INFO_CLASS ) {

                printf("Succeeded\n");

            } else {

                printf("Failed\n");
                printf("    Expected Status = 0x%lx\n", STATUS_INVALID_INFO_CLASS);
                printf("    Received Status = 0x%lx\n", NtStatus );
                TestStatus = FALSE;
            }

// This is the code that was here when UserInternal2Information could be
// queried and set by non-trusted clients...
//
//            //
//            // Get the current values...
//            //
//
//            Buffer1 = NULL;
//            NtStatus = SamQueryInformationUser(
//                           UserHandle1,
//                           UserInternal2Information,
//                           &Buffer1
//                           );
//            TST_SUCCESS_ASSERT(NtStatus);
//            ASSERT(Buffer1 != NULL);
//
//            //
//            // Now change the fields and write them out.
//            //
//
//            ((USER_INTERNAL2_INFORMATION *)Buffer1)->LastLogon.HighPart += 1;
//            ((USER_INTERNAL2_INFORMATION *)Buffer1)->LastLogoff.HighPart += 1;
//            ((USER_INTERNAL2_INFORMATION *)Buffer1)->LastLogon.LowPart += 2;
//            ((USER_INTERNAL2_INFORMATION *)Buffer1)->LastLogoff.LowPart += 2;
//            ((USER_INTERNAL2_INFORMATION *)Buffer1)->BadPasswordCount += 1;
//            ((USER_INTERNAL2_INFORMATION *)Buffer1)->LogonCount += 1;
//
//            NtStatus = SamSetInformationUser(
//                           UserHandle1,
//                           UserInternal2Information,
//                           Buffer1
//                           );
//            if ( NT_SUCCESS(NtStatus) ) {
//
//                //
//                // Now check that the change was really made...
//                //
//
//                NtStatus = SamQueryInformationUser(
//                               UserHandle1,
//                               UserInternal2Information,
//                               &Buffer2
//                               );
//                ASSERT(NT_SUCCESS( NtStatus ) );
//                if (
//                    (((USER_INTERNAL2_INFORMATION *)Buffer1)->LastLogon.HighPart ==
//                    ((USER_INTERNAL2_INFORMATION *)Buffer2)->LastLogon.HighPart) &&
//                    (((USER_INTERNAL2_INFORMATION *)Buffer1)->LastLogon.LowPart ==
//                    ((USER_INTERNAL2_INFORMATION *)Buffer2)->LastLogon.LowPart) &&
//                    (((USER_INTERNAL2_INFORMATION *)Buffer1)->LastLogoff.HighPart ==
//                    ((USER_INTERNAL2_INFORMATION *)Buffer2)->LastLogoff.HighPart) &&
//                    (((USER_INTERNAL2_INFORMATION *)Buffer1)->LastLogoff.LowPart ==
//                    ((USER_INTERNAL2_INFORMATION *)Buffer2)->LastLogoff.LowPart) &&
//                    (((USER_INTERNAL2_INFORMATION *)Buffer1)->BadPasswordCount ==
//                    ((USER_INTERNAL2_INFORMATION *)Buffer2)->BadPasswordCount) &&
//                    (((USER_INTERNAL2_INFORMATION *)Buffer1)->LogonCount ==
//                    ((USER_INTERNAL2_INFORMATION *)Buffer2)->LogonCount)
//                     ) {
//
//                    printf("Succeeded\n");
//
//                } else {
//
//                    printf("Failed\n");
//                    printf("        Value queried doesn't match value written\n");
//
//                    TestStatus = FALSE;
//
//                }
//
//                SamFreeMemory( Buffer1 );
//                SamFreeMemory( Buffer2 );
//
//            } else {
//                printf("Failed\n");
//                printf("        Completion status is 0x%lx\n", NtStatus);
//                TestStatus = FALSE;
//                SamFreeMemory( Buffer1 );
//
//            }



        printf("      Set Password  . . . . . . . . . . . . . . . . . . . .     ");

            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_FORCE_PASSWORD_CHANGE,
                           DOMAIN_USER_RID_ADMIN,
                           &UserHandle1
                           );
            ASSERT(NT_SUCCESS(NtStatus) );

            //
            // Create a fake cleartext UNICODE password and write it out.
            //

            NtStatus = SamSetInformationUser(
                           UserHandle1,
                           UserSetPasswordInformation,
                           &DummyName2
                           );
            if ( NT_SUCCESS(NtStatus) ) {

                //
                // We can't verify that it really worked, so we just have
                // to trust the return code.
                //

                printf("Succeeded\n");

            } else {

                printf("Failed\n");
                printf("    Return code was %lx\n", NtStatus );
                TestStatus = FALSE;
            }



        printf("      Set Control . . . . . . . . . . . . . . . . . . . . .     ");
            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_WRITE_ACCOUNT | USER_READ_ACCOUNT,
                           DOMAIN_USER_RID_ADMIN,
                           &UserHandle1
                           );
            ASSERT(NT_SUCCESS(NtStatus) );

            Buffer1 = NULL;
            NtStatus = SamQueryInformationUser(
                           UserHandle1,
                           UserControlInformation,
                           &Buffer1
                           );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(Buffer1 != NULL);

            //
            // Change the value and write it back
            //

            ((USER_CONTROL_INFORMATION *)Buffer1)->UserAccountControl ^= USER_HOME_DIRECTORY_REQUIRED;


            NtStatus = SamSetInformationUser(
                           UserHandle1,
                           UserControlInformation,
                           Buffer1
                           );
            if (NT_SUCCESS(NtStatus)) {

                //
                // Check the written value to make sure it stuck
                //

                Buffer2 = NULL;
                NtStatus = SamQueryInformationUser(
                               UserHandle1,
                               UserControlInformation,
                               &Buffer2
                               );
                TST_SUCCESS_ASSERT(NtStatus);
                ASSERT(Buffer2 != NULL);

                if ( ((USER_CONTROL_INFORMATION *)Buffer1)->UserAccountControl  ==
                     ((USER_CONTROL_INFORMATION *)Buffer2)->UserAccountControl ) {

                    printf("Succeeded\n");

                    SamFreeMemory( Buffer2 );

                    //
                    // Make sure the account is left enabled to prevent problems.
                    //

                    ((USER_CONTROL_INFORMATION *)Buffer1)->UserAccountControl &= ~USER_ACCOUNT_DISABLED;

                    IgnoreStatus = SamSetInformationUser(
                                       UserHandle1,
                                       UserControlInformation,
                                       Buffer1
                                       );
                    ASSERT(NT_SUCCESS(IgnoreStatus));

                } else {
                    printf("Failed\n");
                    printf("        Returned Value Doesn't Match Set Value.\n");
                    TestStatus = FALSE;
                }
            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }
            SamFreeMemory( Buffer1 );
            IgnoreStatus = SamCloseHandle( UserHandle1 );
            ASSERT( NT_SUCCESS(IgnoreStatus) );


        printf("      Set Expires . . . . . . . . . . . . . . . . . . . . .     ");
        printf("BROKEN TEST - NOT TESTED\n");
#ifdef BROKEN
            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_WRITE_ACCOUNT | USER_READ_ACCOUNT,
                           DOMAIN_USER_RID_ADMIN,
                           &UserHandle1
                           );
            ASSERT(NT_SUCCESS(NtStatus) );

            Buffer1 = NULL;
            NtStatus = SamQueryInformationUser(
                           UserHandle1,
                           UserExpiresInformation,
                           &Buffer1
                           );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(Buffer1 != NULL);

            //
            // Change the value and write it back
            //

            ((USER_EXPIRES_INFORMATION *)Buffer1)->AccountExpires.LowPart  += 1234;
            ((USER_EXPIRES_INFORMATION *)Buffer1)->AccountExpires.HighPart += 1234;


            NtStatus = SamSetInformationUser(
                           UserHandle1,
                           UserExpiresInformation,
                           Buffer1
                           );
            if (NT_SUCCESS(NtStatus)) {

                //
                // Check the written value to make sure it stuck
                //

                Buffer2 = NULL;
                NtStatus = SamQueryInformationUser(
                               UserHandle1,
                               UserExpiresInformation,
                               &Buffer2
                               );
                TST_SUCCESS_ASSERT(NtStatus);
                ASSERT(Buffer2 != NULL);

                if ( ( ((USER_EXPIRES_INFORMATION *)Buffer1)->AccountExpires.LowPart  ==
                       ((USER_EXPIRES_INFORMATION *)Buffer2)->AccountExpires.LowPart )  &&
                     ( ((USER_EXPIRES_INFORMATION *)Buffer1)->AccountExpires.HighPart  ==
                       ((USER_EXPIRES_INFORMATION *)Buffer2)->AccountExpires.HighPart ) ) {

                    printf("Succeeded\n");

                    SamFreeMemory( Buffer2 );

                    //
                    // Change the values back
                    //

                    ((USER_EXPIRES_INFORMATION *)Buffer1)->AccountExpires.LowPart  += 1234;
                    ((USER_EXPIRES_INFORMATION *)Buffer1)->AccountExpires.HighPart += 1234;

                    IgnoreStatus = SamSetInformationUser(
                                       UserHandle1,
                                       UserExpiresInformation,
                                       Buffer1
                                       );
                    ASSERT(NT_SUCCESS(IgnoreStatus));

                } else {
                    printf("Failed\n");
                    printf("        Returned Value Doesn't Match Set Value.\n");
                    TestStatus = FALSE;
                }
            } else {
                printf("Failed\n");
                printf("        Completion status is 0x%lx\n", NtStatus);
                TestStatus = FALSE;
            }
            SamFreeMemory( Buffer1 );
            IgnoreStatus = SamCloseHandle( UserHandle1 );
            ASSERT( NT_SUCCESS(IgnoreStatus) );
#endif //BROKEN



        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Change Password For User Suite                                        //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("\n");
        printf("    Change Password For User  . . . . . . . . . . . . . .   Suite\n");

        printf("      Change Password For Well-Known User . . . . . . . . .     ");

        NtStatus = SamOpenUser(
                       DomainHandle,
                       USER_CHANGE_PASSWORD,
                       DOMAIN_USER_RID_ADMIN,
                       &UserHandle1
                       );
        ASSERT(NT_SUCCESS(NtStatus) );

        Buffer = NULL;

        //
        // The current password is DummyName2.  Using DummyName2 as the
        // old password, change it to DummyName1 and make sure we get
        // STATUS_SUCCESS.
        //

        NtStatus = SamChangePasswordUser(
                       UserHandle1,
                       &DummyName2,
                       &DummyName1
                       );

        //
        // The current password is DummyName1.  Using something WRONG for
        // the old password, try to change it to DummyName2 and make sure
        // it doesn't succeed.
        //

        if ( NtStatus == STATUS_SUCCESS ) {

            NtStatus = SamChangePasswordUser(
                           UserHandle1,
                           &DummyName2,
                           &DummyName2
                           );

            if ( NtStatus == STATUS_SUCCESS ) {

                NtStatus = STATUS_UNSUCCESSFUL;

            } else {

                NtStatus = STATUS_SUCCESS;
            }
        }

        //
        // The current password is DummyName1.  Using DummyName1 as the
        // old password, change it to DummyName2 and make sure it works
        // since by default there is no password history.
        //

        if ( NtStatus == STATUS_SUCCESS ) {

            NtStatus = SamChangePasswordUser(
                           UserHandle1,
                           &DummyName1,
                           &DummyName2
                           );
        }

        if ( NT_SUCCESS( NtStatus ) ) {

            printf("Succeeded\n");

        } else {

            printf("Failed\n");
            printf("        Status is %lx\n", NtStatus);

            TestStatus = FALSE;
        }

        IgnoreStatus = SamCloseHandle( UserHandle1 );
        ASSERT( NT_SUCCESS(IgnoreStatus) );

    }

// END PASS #1

    if (Pass == 2) {

        printf("\n");
        printf("\n");
        printf("  User  (Pass #2) . . . . . . . . . . . . . . . . . . .   Test\n");


        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Delete User  Suite                                                    //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("\n");
        printf("    Delete User   . . . . . . . . . . . . . . . . . . . .   Suite\n");



        printf("      Delete Normal User  . . . . . . . . . . . . . . . . .     ");

        //
        // This User was created in pass #1
        //

        RtlInitString( &AccountNameAnsi, USER_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountNames[0], &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        NtStatus = SamLookupNamesInDomain(
                       DomainHandle,
                       1,
                       &AccountNames[0],
                       &LookedUpRids,
                       &LookedUpUses
                       );
        TST_SUCCESS_ASSERT(NtStatus);
        ASSERT(LookedUpUses[0] == SidTypeUser);
        RtlFreeUnicodeString( &AccountNames[0] );



        UserHandle1 = NULL;

        NtStatus = SamOpenUser( DomainHandle, DELETE, LookedUpRids[0], &UserHandle1 );
        TST_SUCCESS_ASSERT(NtStatus);
        SamFreeMemory( LookedUpUses ); SamFreeMemory( LookedUpRids );

        NtStatus = SamDeleteUser( UserHandle1 );
        if (NT_SUCCESS(NtStatus)) {
            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }




        printf("      Delete Admin Group Member . . . . . . . . . . . . . .     ");
        printf("(Unimplemented)\n");

        printf("      Delete Last Admin Group Member  . . . . . . . . . . .     ");
        printf("(Unimplemented)\n");





        ///////////////////////////////////////////////////////////////////////////
        //                                                                       //
        // Set User Suite                                                        //
        //                                                                       //
        ///////////////////////////////////////////////////////////////////////////

        printf("\n");
        printf("    Set User (Pass 2) . . . . . . . . . . . . . . . . . .   Suite\n");

        printf("      Set ALL information. . . . . . . . . . . .     ");
        printf("BROKEN TEST - NOT TESTED\n");
#ifdef BROKEN

        RtlInitString( &AccountNameAnsi, "AllUser" );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        UserRid = 0;
        UserHandle1 = NULL;
        NtStatus = SamCreateUserInDomain(
                       DomainHandle,
                       &AccountName,
                       USER_ALL_ACCESS,
                       &UserHandle1,
                       &UserRid
                       );
        RtlFreeUnicodeString( &AccountName );

        ASSERT(NT_SUCCESS(NtStatus));

        All = NULL;

        NtStatus = SamQueryInformationUser(
                       UserHandle1,
                       UserAllInformation,
                       &All
                       );

        if ( NT_SUCCESS( NtStatus ) ) {

            //
            // Now change some of the data, and set it
            //

            RtlInitString( &TmpAnsiString, "FullName" );
            TmpStatus = RtlAnsiStringToUnicodeString(
                            (PUNICODE_STRING)(&All->FullName),
                            &TmpAnsiString,
                            TRUE );
            ASSERT( NT_SUCCESS( TmpStatus ) );

            RtlInitString( &TmpAnsiString, "HomeDirectory" );
            TmpStatus = RtlAnsiStringToUnicodeString(
                            (PUNICODE_STRING)(&All->HomeDirectory),
                            &TmpAnsiString,
                            TRUE );
            ASSERT( NT_SUCCESS( TmpStatus ) );

            RtlInitString( &TmpAnsiString, "HomeDirectoryDrive" );
            TmpStatus = RtlAnsiStringToUnicodeString(
                            (PUNICODE_STRING)(&All->HomeDirectoryDrive),
                            &TmpAnsiString,
                            TRUE );
            ASSERT( NT_SUCCESS( TmpStatus ) );

            RtlInitString( &TmpAnsiString, "ScriptPath" );
            TmpStatus = RtlAnsiStringToUnicodeString(
                            (PUNICODE_STRING)(&All->ScriptPath),
                            &TmpAnsiString,
                            TRUE );
            ASSERT( NT_SUCCESS( TmpStatus ) );

            RtlInitString( &TmpAnsiString, "ProfilePath" );
            TmpStatus = RtlAnsiStringToUnicodeString(
                            (PUNICODE_STRING)(&All->ProfilePath),
                            &TmpAnsiString,
                            TRUE );
            ASSERT( NT_SUCCESS( TmpStatus ) );

            RtlInitString( &TmpAnsiString, "AdminComment" );
            TmpStatus = RtlAnsiStringToUnicodeString(
                            (PUNICODE_STRING)(&All->AdminComment),
                            &TmpAnsiString,
                            TRUE );
            ASSERT( NT_SUCCESS( TmpStatus ) );

            RtlInitString( &TmpAnsiString, "WorkStations" );
            TmpStatus = RtlAnsiStringToUnicodeString(
                            (PUNICODE_STRING)(&All->WorkStations),
                            &TmpAnsiString,
                            TRUE );
            ASSERT( NT_SUCCESS( TmpStatus ) );

            RtlInitString( &TmpAnsiString, "UserComment" );
            TmpStatus = RtlAnsiStringToUnicodeString(
                            (PUNICODE_STRING)(&All->UserComment),
                            &TmpAnsiString,
                            TRUE );
            ASSERT( NT_SUCCESS( TmpStatus ) );

            RtlInitString( &TmpAnsiString, "Parameters" );
            TmpStatus = RtlAnsiStringToUnicodeString(
                            (PUNICODE_STRING)(&All->Parameters),
                            &TmpAnsiString,
                            TRUE );
            ASSERT( NT_SUCCESS( TmpStatus ) );

            All->CountryCode = 7;
            All->CodePage = 8;

            All->PasswordExpired = TRUE;
            All->NtPasswordPresent = TRUE;
            All->LmPasswordPresent = FALSE;

            RtlInitString( &TmpAnsiString, "NtPassword" );
            TmpStatus = RtlAnsiStringToUnicodeString(
                            (PUNICODE_STRING)(&All->NtPassword),
                            &TmpAnsiString,
                            TRUE );
            ASSERT( NT_SUCCESS( TmpStatus ) );

            All->LogonHours.UnitsPerWeek = 7;

            All->WhichFields =  ( USER_ALL_FULLNAME |
                                USER_ALL_HOMEDIRECTORY |
                                USER_ALL_HOMEDIRECTORYDRIVE |
                                USER_ALL_SCRIPTPATH |
                                USER_ALL_PROFILEPATH |
                                USER_ALL_ADMINCOMMENT |
                                USER_ALL_WORKSTATIONS |
                                USER_ALL_USERCOMMENT |
                                USER_ALL_PARAMETERS |
                                USER_ALL_COUNTRYCODE |
                                USER_ALL_CODEPAGE |
                                USER_ALL_PASSWORDEXPIRED |
                                USER_ALL_NTPASSWORDPRESENT |
                                USER_ALL_LOGONHOURS );

            NtStatus = SamSetInformationUser(
                           UserHandle1,
                           UserAllInformation,
                           All
                           );

            if ( NT_SUCCESS( NtStatus ) ) {

                NtStatus = SamQueryInformationUser(
                               UserHandle1,
                               UserAllInformation,
                               &All2
                               );

                if ( NT_SUCCESS( NtStatus ) ) {

                    //
                    // Verify that queried info is as we set it
                    //

                    if (

                        //
                        // Fields that we didn't touch.  Note that
                        // PasswordMustChange changed anyway, since we
                        // changed from a null to a non-null password.
                        //

                        ( All2->WhichFields != (USER_ALL_READ_GENERAL_MASK    |
                                               USER_ALL_READ_PREFERENCES_MASK |
                                               USER_ALL_READ_ACCOUNT_MASK     |
                                               USER_ALL_READ_LOGON_MASK) ) ||
                        ( !(All->LastLogon.QuadPart ==
                            All2->LastLogon.QuadPart ) ) ||
                        ( !(All->LastLogoff.QuadPart ==
                            All2->LastLogoff.QuadPart ) ) ||
                        ( !(All->PasswordLastSet.QuadPart ==
                            All2->PasswordLastSet.QuadPart ) ) ||
                        ( !(All->AccountExpires.QuadPart ==
                            All2->AccountExpires.QuadPart ) ) ||
                        ( !(All->PasswordCanChange.QuadPart ==
                            All2->PasswordCanChange.QuadPart ) ) ||
                        (  (All->PasswordMustChange.QuadPart ==
                            All2->PasswordMustChange.QuadPart ) ) ||
                        (RtlCompareUnicodeString(
                            &(All->UserName),
                            &(All2->UserName),
                            FALSE) != 0) ||
                        ( All->UserId != All2->UserId ) ||
                        ( All->PrimaryGroupId != All2->PrimaryGroupId ) ||
                        ( All->UserAccountControl != All2->UserAccountControl ) ||
                        ( All->PrivateDataSensitive !=
                            All2->PrivateDataSensitive ) ||

                        // Fields that we changed.  Note that we set
                        // NtPasswordSet, but it shouldn't be set on return.

                        (RtlCompareUnicodeString(
                            &(All->FullName),
                            &(All2->FullName),
                            FALSE) != 0) ||
                        (RtlCompareUnicodeString(
                            &(All->HomeDirectory),
                            &(All2->HomeDirectory),
                            FALSE) != 0) ||
                        (RtlCompareUnicodeString(
                            &(All->HomeDirectoryDrive),
                            &(All2->HomeDirectoryDrive),
                            FALSE) != 0) ||
                        (RtlCompareUnicodeString(
                            &(All->ScriptPath),
                            &(All2->ScriptPath),
                            FALSE) != 0) ||
                        (RtlCompareUnicodeString(
                            &(All->ProfilePath),
                            &(All2->ProfilePath),
                            FALSE) != 0) ||
                        (RtlCompareUnicodeString(
                            &(All->AdminComment),
                            &(All2->AdminComment),
                            FALSE) != 0) ||
                        (RtlCompareUnicodeString(
                            &(All->WorkStations),
                            &(All2->WorkStations),
                            FALSE) != 0) ||
                        (RtlCompareUnicodeString(
                            &(All->UserComment),
                            &(All2->UserComment),
                            FALSE) != 0) ||
                        (RtlCompareUnicodeString(
                            &(All->Parameters),
                            &(All2->Parameters),
                            FALSE) != 0) ||
                        ( All->CountryCode != All2->CountryCode ) ||
                        ( All->CodePage != All2->CodePage ) ||
                        ( All->LmPasswordPresent != All2->LmPasswordPresent ) ||
                        ( All->NtPasswordPresent == All2->NtPasswordPresent ) ||
                        ( All->LogonHours.UnitsPerWeek !=
                            All2->LogonHours.UnitsPerWeek )
                        ) {

                        NtStatus = STATUS_DATA_ERROR;
                    }

                    SamFreeMemory( All2 );
                }
            }

            SamFreeMemory( All );
        }

        if (NtStatus == STATUS_SUCCESS) {

            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }

        //
        // Now get rid of the user account if necessary
        //

        NtStatus = SamDeleteUser( UserHandle1 );
        ASSERT(NT_SUCCESS(NtStatus));
#endif //BROKEN


        printf("      Set Primary Group (non member). . . . . . . . . . . .     ");
        //
        // The following user might already exist (from earlier in the test)
        //

        RtlInitString( &AccountNameAnsi, USER_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        //InitializeObjectAttributes( &ObjectAttributes, &AccountName, 0, 0, NULL );


        UserRid = 0;
        UserHandle1 = NULL;
        NtStatus = SamCreateUserInDomain(
                       DomainHandle,
                       &AccountName,
                       USER_ALL_ACCESS,
                       &UserHandle1,
                       &UserRid
                       );
        RtlFreeUnicodeString( &AccountName );
        DeleteUser = TRUE;
        if (NtStatus == STATUS_USER_EXISTS) {
            DeleteUser = FALSE;
            RtlInitString( &AccountNameAnsi, USER_NAME1 );
            NtStatus = RtlAnsiStringToUnicodeString( &AccountNames[0], &AccountNameAnsi, TRUE );
            TST_SUCCESS_ASSERT(NtStatus);

            NtStatus = SamLookupNamesInDomain(
                           DomainHandle,
                           1,
                           &AccountNames[0],
                           &LookedUpRids,
                           &LookedUpUses
                           );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(LookedUpUses[0] == SidTypeUser);
            RtlFreeUnicodeString( &AccountNames[0] );
            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_ALL_ACCESS,
                           LookedUpRids[0],
                           &UserHandle1);
            SamFreeMemory( LookedUpUses ); SamFreeMemory( LookedUpRids );
        }

        ASSERT(NT_SUCCESS(NtStatus));


        //
        // The user is not a member of DOMAIN_GROUP_RID_ADMINS.
        // See if we can make this group the user's primary group
        //

        ASSERT(sizeof(GroupRid) == sizeof(USER_PRIMARY_GROUP_INFORMATION));
        GroupRid = DOMAIN_GROUP_RID_ADMINS;
        NtStatus = SamSetInformationUser(
                       UserHandle1,
                       UserPrimaryGroupInformation,
                       &GroupRid
                       );

        if (NtStatus == STATUS_MEMBER_NOT_IN_GROUP) {

            printf("Succeeded\n");

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }


        //
        // Now get rid of the user account if necessary
        //

        if (DeleteUser == TRUE) {
            NtStatus = SamDeleteUser( UserHandle1 );
            ASSERT(NT_SUCCESS(NtStatus));
        } else {
            NtStatus = SamCloseHandle( UserHandle1 );
            ASSERT(NT_SUCCESS(NtStatus));
        }



        printf("      Set Primary Group (member). . . . . . . . . . . . . .     ");

        //
        // Make a user (might already exist)
        // Make a group
        // Make the group the user's primary group
        // Change the user so the group isn't the primary group
        // remove the group
        // delete the group
        // If we created the user, delete it.

        //
        // The following user might already exist (from earlier in the test)
        //

        RtlInitString( &AccountNameAnsi, USER_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        //InitializeObjectAttributes( &ObjectAttributes, &AccountName, 0, 0, NULL );

        UserRid = 0;
        UserHandle1 = NULL;
        NtStatus = SamCreateUserInDomain(
                       DomainHandle,
                       &AccountName,
                       USER_ALL_ACCESS,
                       &UserHandle1,
                       &UserRid
                       );
        RtlFreeUnicodeString( &AccountName );
        DeleteUser = TRUE;
        if (NtStatus == STATUS_USER_EXISTS) {
            DeleteUser = FALSE;
            RtlInitString( &AccountNameAnsi, USER_NAME1 );
            NtStatus = RtlAnsiStringToUnicodeString( &AccountNames[0], &AccountNameAnsi, TRUE );
            TST_SUCCESS_ASSERT(NtStatus);

            NtStatus = SamLookupNamesInDomain(
                           DomainHandle,
                           1,
                           &AccountNames[0],
                           &LookedUpRids,
                           &LookedUpUses
                           );
            RtlFreeUnicodeString( &AccountNames[0] );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(LookedUpUses[0] == SidTypeUser);
            UserRid = LookedUpRids[0];
            NtStatus = SamOpenUser(
                           DomainHandle,
                           USER_ALL_ACCESS,
                           UserRid,
                           &UserHandle1);
            SamFreeMemory( LookedUpUses ); SamFreeMemory( LookedUpRids );
        }

        ASSERT(NT_SUCCESS(NtStatus));


        //
        // create the group
        //

        RtlInitString( &AccountNameAnsi, GROUP_NAME1 );
        NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi, TRUE );
        TST_SUCCESS_ASSERT(NtStatus);

        //InitializeObjectAttributes( &ObjectAttributes, &AccountName, 0, 0, NULL );

        GroupRid = 0;
        GroupHandle1 = NULL;
        NtStatus = SamCreateGroupInDomain(
                       DomainHandle,
                       &AccountName,
                       GROUP_ALL_ACCESS,
                       &GroupHandle1,
                       &GroupRid
                       );
        RtlFreeUnicodeString( &AccountName );
        ASSERT(NT_SUCCESS(NtStatus));

        //
        // Make the user a member of this group
        //

        NtStatus = SamAddMemberToGroup(
                       GroupHandle1,
                       UserRid,
                       SE_GROUP_MANDATORY              |
                           SE_GROUP_ENABLED_BY_DEFAULT |
                           SE_GROUP_ENABLED
                       );
        ASSERT(NT_SUCCESS(NtStatus));


        //
        // Set the user's primary group Id to be this group
        //

        NtStatus = SamSetInformationUser(
                       UserHandle1,
                       UserPrimaryGroupInformation,
                       &GroupRid
                       );
        if (NT_SUCCESS(NtStatus)) {

            Buffer1 = NULL;
            NtStatus = SamQueryInformationUser(
                           UserHandle1,
                           UserPrimaryGroupInformation,
                           &Buffer1
                           );
            TST_SUCCESS_ASSERT(NtStatus);
            ASSERT(Buffer1 != NULL);

            if ( ((USER_PRIMARY_GROUP_INFORMATION *)Buffer1)->PrimaryGroupId  ==
                 GroupRid ) {

                printf("Succeeded\n");

                SamFreeMemory( Buffer1 );
            } else {

                printf("Failed\n");
                printf("        Returned Value Doesn't Match Set Value.\n");
                printf("        Value written is: 0x%lx\n", GroupRid);
                printf("      Value retrieved is: 0x%lx\n",
                    ((USER_PRIMARY_GROUP_INFORMATION *)Buffer1)->PrimaryGroupId);
                TestStatus = FALSE;

            }

        } else {
            printf("Failed\n");
            printf("        Completion status is 0x%lx\n", NtStatus);
            TestStatus = FALSE;
        }


        //
        // Set the user's primary group Id back and remove the user
        // from the group
        //

        GroupRid = DOMAIN_GROUP_RID_USERS;
        NtStatus = SamSetInformationUser(
                       UserHandle1,
                       UserPrimaryGroupInformation,
                       &GroupRid
                       );
        ASSERT(NT_SUCCESS(NtStatus));
        NtStatus = SamRemoveMemberFromGroup(GroupHandle1, UserRid);
        ASSERT(NT_SUCCESS(NtStatus));



        //
        // Now get rid of the group and possibly the user account
        //


        NtStatus = SamDeleteGroup( GroupHandle1 );
        ASSERT(NT_SUCCESS(NtStatus));

        if (DeleteUser == TRUE) {
            NtStatus = SamDeleteUser( UserHandle1 );
            ASSERT(NT_SUCCESS(NtStatus));
        } else {
            NtStatus = SamCloseHandle( UserHandle1 );
            ASSERT(NT_SUCCESS(NtStatus));
        }









        printf("      Set Name Information  . . . . . . . . . . . . . . . .     ");
        printf("(Untested)\n");


    }

    return(TestStatus);

}
#endif // NOT_PART_OF_PROGRAM
