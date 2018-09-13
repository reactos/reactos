/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tsevars.c

Abstract:

    This Module contains variables used in security test routines.


Author:

    Jim Kelly       (JimK)     23-Mar-1990

Environment:

    Test.

Revision History:

--*/

#include "tsecomm.c"    // Mode dependent macros and routines.


#ifndef _TSEVARS_
#define _TSEVARS_




typedef enum _USERS {
    Fred,
    Wilma,
    Pebbles,
    Barney,
    Betty,
    Bambam,
    Dino
} USERS;



//
// Define the Bedrock domain and its inhabitants
//
//     Bedrock Domain      S-1-39824-21-3-17
//     Fred                S-1-39824-21-3-17-2
//     Wilma               S-1-39824-21-3-17-3
//     Pebbles             S-1-39824-21-3-17-4
//     Dino                S-1-39824-21-3-17-5
//     Barney              S-1-39824-21-3-17-6
//     Betty               S-1-39824-21-3-17-7
//     Bambam              S-1-39824-21-3-17-8
//     Flintstone          S-1-39824-21-3-17-9
//     Rubble              S-1-39824-21-3-17-10
//     Adult               S-1-39824-21-3-17-11
//     Child               S-1-39824-21-3-17-12
//     Neanderthol         S-1-39824-21-3-17-13
//

#define BEDROCK_AUTHORITY               {0,0,0,0,155,144}
#define BEDROCK_SUBAUTHORITY_0          0x00000015L
#define BEDROCK_SUBAUTHORITY_1          0x00000003L
#define BEDROCK_SUBAUTHORITY_2          0x00000011L

#define FRED_RID                        0x00000002L
#define WILMA_RID                       0x00000003L
#define PEBBLES_RID                     0x00000004L
#define DINO_RID                        0x00000005L

#define BARNEY_RID                      0x00000006L
#define BETTY_RID                       0x00000007L
#define BAMBAM_RID                      0x00000008L

#define FLINTSTONE_RID                  0x00000009L
#define RUBBLE_RID                      0x0000000AL

#define ADULT_RID                       0x0000000BL
#define CHILD_RID                       0x0000000CL

#define NEANDERTHOL_RID                 0x0000000DL


PSID BedrockDomainSid;


PSID  FredSid;
PSID  WilmaSid;
PSID  PebblesSid;
PSID  DinoSid;

PSID  BarneySid;
PSID  BettySid;
PSID  BambamSid;

PSID  FlintstoneSid;
PSID  RubbleSid;

PSID  AdultSid;
PSID  ChildSid;

PSID  NeandertholSid;


//
// Universal well known SIDs
//

PSID  NullSid;
PSID  WorldSid;
PSID  LocalSid;
PSID  CreatorSid;

//
// Sids defined by NT
//

PSID NtAuthoritySid;

PSID DialupSid;
PSID NetworkSid;
PSID BatchSid;
PSID InteractiveSid;
PSID LocalSystemSid;





////////////////////////////////////////////////////////////////////////
//                                                                    //
//         Define the well known privileges                           //
//                                                                    //
////////////////////////////////////////////////////////////////////////


LUID CreateTokenPrivilege;
LUID AssignPrimaryTokenPrivilege;
LUID LockMemoryPrivilege;
LUID IncreaseQuotaPrivilege;
LUID UnsolicitedInputPrivilege;
LUID TcbPrivilege;
LUID SecurityPrivilege;

LUID TakeOwnershipPrivilege;
LUID CreatePagefilePrivilege;
LUID IncreaseBasePriorityPrivilege;
LUID SystemProfilePrivilege;
LUID SystemtimePrivilege;
LUID ProfileSingleProcessPrivilege;

LUID RestorePrivilege;
LUID BackupPrivilege;
LUID CreatePermanentPrivilege;
LUID ShutdownPrivilege;
LUID DebugPrivilege;





BOOLEAN
TSeVariableInitialization()
/*++

Routine Description:

    This function initializes the global variables used in security
    tests.

Arguments:

    None.

Return Value:

    TRUE if variables successfully initialized.
    FALSE if not successfully initialized.

--*/
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


    SID_IDENTIFIER_AUTHORITY BedrockAuthority = BEDROCK_AUTHORITY;


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

    NullSid    = (PSID)TstAllocatePool(PagedPool,SidWithOneSubAuthority);
    WorldSid   = (PSID)TstAllocatePool(PagedPool,SidWithOneSubAuthority);
    LocalSid   = (PSID)TstAllocatePool(PagedPool,SidWithOneSubAuthority);
    CreatorSid = (PSID)TstAllocatePool(PagedPool,SidWithOneSubAuthority);

    RtlInitializeSid( NullSid,    &NullSidAuthority, 1 );
    RtlInitializeSid( WorldSid,   &WorldSidAuthority, 1 );
    RtlInitializeSid( LocalSid,   &LocalSidAuthority, 1 );
    RtlInitializeSid( CreatorSid, &CreatorSidAuthority, 1 );

    *(RtlSubAuthoritySid( NullSid, 0 ))    = SECURITY_NULL_RID;
    *(RtlSubAuthoritySid( WorldSid, 0 ))   = SECURITY_WORLD_RID;
    *(RtlSubAuthoritySid( LocalSid, 0 ))   = SECURITY_LOCAL_RID;
    *(RtlSubAuthoritySid( CreatorSid, 0 )) = SECURITY_CREATOR_OWNER_RID;

    //
    // Allocate and initialize the NT defined SIDs
    //

    NtAuthoritySid  = (PSID)TstAllocatePool(PagedPool,SidWithZeroSubAuthorities);
    DialupSid       = (PSID)TstAllocatePool(PagedPool,SidWithOneSubAuthority);
    NetworkSid      = (PSID)TstAllocatePool(PagedPool,SidWithOneSubAuthority);
    BatchSid        = (PSID)TstAllocatePool(PagedPool,SidWithOneSubAuthority);
    InteractiveSid  = (PSID)TstAllocatePool(PagedPool,SidWithOneSubAuthority);
    LocalSystemSid  = (PSID)TstAllocatePool(PagedPool,SidWithOneSubAuthority);

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



    //
    // Allocate and initialize the Bedrock SIDs
    //

    BedrockDomainSid  = (PSID)TstAllocatePool(PagedPool,SidWithThreeSubAuthorities);

    FredSid           = (PSID)TstAllocatePool(PagedPool,SidWithFourSubAuthorities);
    WilmaSid          = (PSID)TstAllocatePool(PagedPool,SidWithFourSubAuthorities);
    PebblesSid        = (PSID)TstAllocatePool(PagedPool,SidWithFourSubAuthorities);
    DinoSid           = (PSID)TstAllocatePool(PagedPool,SidWithFourSubAuthorities);

    BarneySid         = (PSID)TstAllocatePool(PagedPool,SidWithFourSubAuthorities);
    BettySid          = (PSID)TstAllocatePool(PagedPool,SidWithFourSubAuthorities);
    BambamSid         = (PSID)TstAllocatePool(PagedPool,SidWithFourSubAuthorities);

    FlintstoneSid     = (PSID)TstAllocatePool(PagedPool,SidWithFourSubAuthorities);
    RubbleSid         = (PSID)TstAllocatePool(PagedPool,SidWithFourSubAuthorities);

    AdultSid          = (PSID)TstAllocatePool(PagedPool,SidWithFourSubAuthorities);
    ChildSid          = (PSID)TstAllocatePool(PagedPool,SidWithFourSubAuthorities);

    NeandertholSid    = (PSID)TstAllocatePool(PagedPool,SidWithFourSubAuthorities);

    RtlInitializeSid( BedrockDomainSid,   &BedrockAuthority, 3 );
    *(RtlSubAuthoritySid( BedrockDomainSid, 0)) = BEDROCK_SUBAUTHORITY_0;
    *(RtlSubAuthoritySid( BedrockDomainSid, 1)) = BEDROCK_SUBAUTHORITY_1;
    *(RtlSubAuthoritySid( BedrockDomainSid, 2)) = BEDROCK_SUBAUTHORITY_2;

    RtlCopySid( SidWithFourSubAuthorities, FredSid, BedrockDomainSid);
    *(RtlSubAuthorityCountSid( FredSid )) += 1;
    *(RtlSubAuthoritySid( FredSid, 3)) = FRED_RID;

    RtlCopySid( SidWithFourSubAuthorities, WilmaSid, BedrockDomainSid);
    *(RtlSubAuthorityCountSid( WilmaSid )) += 1;
    *(RtlSubAuthoritySid( WilmaSid, 3)) = WILMA_RID;

    RtlCopySid( SidWithFourSubAuthorities, PebblesSid, BedrockDomainSid);
    *(RtlSubAuthorityCountSid( PebblesSid )) += 1;
    *(RtlSubAuthoritySid( PebblesSid, 3)) = PEBBLES_RID;

    RtlCopySid( SidWithFourSubAuthorities, DinoSid, BedrockDomainSid);
    *(RtlSubAuthorityCountSid( DinoSid )) += 1;
    *(RtlSubAuthoritySid( DinoSid, 3)) = DINO_RID;

    RtlCopySid( SidWithFourSubAuthorities, BarneySid, BedrockDomainSid);
    *(RtlSubAuthorityCountSid( BarneySid )) += 1;
    *(RtlSubAuthoritySid( BarneySid, 3)) = BARNEY_RID;

    RtlCopySid( SidWithFourSubAuthorities, BettySid, BedrockDomainSid);
    *(RtlSubAuthorityCountSid( BettySid )) += 1;
    *(RtlSubAuthoritySid( BettySid, 3)) = BETTY_RID;

    RtlCopySid( SidWithFourSubAuthorities, BambamSid, BedrockDomainSid);
    *(RtlSubAuthorityCountSid( BambamSid )) += 1;
    *(RtlSubAuthoritySid( BambamSid, 3)) = BAMBAM_RID;

    RtlCopySid( SidWithFourSubAuthorities, FlintstoneSid, BedrockDomainSid);
    *(RtlSubAuthorityCountSid( FlintstoneSid )) += 1;
    *(RtlSubAuthoritySid( FlintstoneSid, 3)) = FLINTSTONE_RID;

    RtlCopySid( SidWithFourSubAuthorities, RubbleSid, BedrockDomainSid);
    *(RtlSubAuthorityCountSid( RubbleSid )) += 1;
    *(RtlSubAuthoritySid( RubbleSid, 3)) = RUBBLE_RID;

    RtlCopySid( SidWithFourSubAuthorities, AdultSid, BedrockDomainSid);
    *(RtlSubAuthorityCountSid( AdultSid )) += 1;
    *(RtlSubAuthoritySid( AdultSid, 3)) = ADULT_RID;

    RtlCopySid( SidWithFourSubAuthorities, ChildSid, BedrockDomainSid);
    *(RtlSubAuthorityCountSid( ChildSid )) += 1;
    *(RtlSubAuthoritySid( ChildSid, 3)) = CHILD_RID;

    RtlCopySid( SidWithFourSubAuthorities, NeandertholSid, BedrockDomainSid);
    *(RtlSubAuthorityCountSid( NeandertholSid )) += 1;
    *(RtlSubAuthoritySid( NeandertholSid, 3)) = NEANDERTHOL_RID;


    CreateTokenPrivilege =
        RtlConvertLongToLuid(SE_CREATE_TOKEN_PRIVILEGE);
    AssignPrimaryTokenPrivilege =
        RtlConvertLongToLuid(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE);
    LockMemoryPrivilege =
        RtlConvertLongToLuid(SE_LOCK_MEMORY_PRIVILEGE);
    IncreaseQuotaPrivilege =
        RtlConvertLongToLuid(SE_INCREASE_QUOTA_PRIVILEGE);
    UnsolicitedInputPrivilege =
        RtlConvertLongToLuid(SE_UNSOLICITED_INPUT_PRIVILEGE);
    TcbPrivilege =
        RtlConvertLongToLuid(SE_TCB_PRIVILEGE);
    SecurityPrivilege =
        RtlConvertLongToLuid(SE_SECURITY_PRIVILEGE);
    TakeOwnershipPrivilege =
        RtlConvertLongToLuid(SE_TAKE_OWNERSHIP_PRIVILEGE);
    CreatePagefilePrivilege =
        RtlConvertLongToLuid(SE_CREATE_PAGEFILE_PRIVILEGE);
    IncreaseBasePriorityPrivilege =
        RtlConvertLongToLuid(SE_INC_BASE_PRIORITY_PRIVILEGE);
    SystemProfilePrivilege =
        RtlConvertLongToLuid(SE_SYSTEM_PROFILE_PRIVILEGE);
    SystemtimePrivilege =
        RtlConvertLongToLuid(SE_SYSTEMTIME_PRIVILEGE);
    ProfileSingleProcessPrivilege =
        RtlConvertLongToLuid(SE_PROF_SINGLE_PROCESS_PRIVILEGE);
    CreatePermanentPrivilege =
        RtlConvertLongToLuid(SE_CREATE_PERMANENT_PRIVILEGE);
    BackupPrivilege =
        RtlConvertLongToLuid(SE_BACKUP_PRIVILEGE);
    RestorePrivilege =
        RtlConvertLongToLuid(SE_RESTORE_PRIVILEGE);
    ShutdownPrivilege =
        RtlConvertLongToLuid(SE_SHUTDOWN_PRIVILEGE);
    DebugPrivilege =
        RtlConvertLongToLuid(SE_DEBUG_PRIVILEGE);


    return TRUE;

}
#endif  // _TSEVARS_
