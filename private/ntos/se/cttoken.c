/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    cttoken.c

Abstract:

    Common token object test routines.

    These routines are used in both kernel and user mode tests.

    This test assumes the security runtime library routines are
    functioning correctly.

    NOTE:  This test program allocates a lot of memory and frees
           none of it ! ! !



Author:

    Jim Kelly       (JimK)     27-June-1990

Environment:

    Test of token object.

Revision History:

--*/

#include "tsecomm.c"    // Mode dependent macros and routines.



////////////////////////////////////////////////////////////////
//                                                            //
// Module wide variables                                      //
//                                                            //
////////////////////////////////////////////////////////////////

#define DEFAULT_DACL_LENGTH (1024L)
#define GROUP_IDS_LENGTH (1024L)
#define NEW_GROUP_STATE_LENGTH (1024L)
#define PRIVILEGES_LENGTH (128L)
#define TOO_BIG_ACL_SIZE (2048L)
#define TOO_BIG_PRIMARY_GROUP_SIZE (39L)

//
// definitions related to TokenWithGroups
// (we also substitute SYSTEM for NEANDERTHOL in some tests)
//

#define FLINTSTONE_INDEX  (0L)
#define CHILD_INDEX       (1L)
#define NEANDERTHOL_INDEX (2L)
#define SYSTEM_INDEX      (2L)
#define WORLD_INDEX       (3L)
#define GROUP_COUNT       (4L)
#define RESTRICTED_SID_COUNT (2L)


//
// Definitions related to TokenWithPrivileges
//

#define UNSOLICITED_INDEX           (0L)
#define SECURITY_INDEX              (1L)
#define ASSIGN_PRIMARY_INDEX        (2L)
#define PRIVILEGE_COUNT             (3L)


    NTSTATUS Status;

    HANDLE SimpleToken;
    HANDLE TokenWithGroups;
    HANDLE TokenWithDefaultOwner;
    HANDLE TokenWithPrivileges;
    HANDLE TokenWithDefaultDacl;

    HANDLE TokenWithRestrictedGroups;
    HANDLE TokenWithRestrictedPrivileges;
    HANDLE TokenWithRestrictedSids;
    HANDLE TokenWithMoreRestrictedSids;


    HANDLE Token;
    HANDLE ProcessToken;
    HANDLE ImpersonationToken;
    HANDLE AnonymousToken;

    OBJECT_ATTRIBUTES PrimaryTokenAttributes;
    PSECURITY_DESCRIPTOR PrimarySecurityDescriptor;
    SECURITY_QUALITY_OF_SERVICE PrimarySecurityQos;

    OBJECT_ATTRIBUTES ImpersonationTokenAttributes;
    PSECURITY_DESCRIPTOR ImpersonationSecurityDescriptor;
    SECURITY_QUALITY_OF_SERVICE ImpersonationSecurityQos;

    OBJECT_ATTRIBUTES AnonymousTokenAttributes;
    PSECURITY_DESCRIPTOR AnonymousSecurityDescriptor;
    SECURITY_QUALITY_OF_SERVICE AnonymousSecurityQos;

    ULONG DisabledGroupAttributes;
    ULONG OptionalGroupAttributes;
    ULONG NormalGroupAttributes;
    ULONG OwnerGroupAttributes;

    ULONG LengthAvailable;
    ULONG CurrentLength;


    TIME_FIELDS TempTimeFields = {3000, 1, 1, 1, 1, 1, 1, 1};
    LARGE_INTEGER NoExpiration;

    LUID BadAuthenticationId;
    LUID SystemAuthenticationId = SYSTEM_LUID;
    LUID OriginalAuthenticationId;

    TOKEN_SOURCE TestSource = {"SE: TEST", 0};

    PSID Owner;
    PSID Group;
    PACL Dacl;

    PSID TempOwner;
    PSID TempGroup;
    PACL TempDacl;

    UQUAD ThreadStack[256];
    INITIAL_TEB InitialTeb;
    NTSTATUS Status;
    CLIENT_ID ThreadClientId;
    CONTEXT ThreadContext;
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES ThreadObja;






////////////////////////////////////////////////////////////////
//                                                            //
// Private Macros                                             //
//                                                            //
////////////////////////////////////////////////////////////////


#define TestpPrintLuid(G)                                                     \
            DbgPrint( "(0x%x, 0x%x)", \
                         (G).HighPart, (G).LowPart);                         \




////////////////////////////////////////////////////////////////
//                                                            //
// Initialization Routine                                     //
//                                                            //
////////////////////////////////////////////////////////////////

BOOLEAN
TestTokenInitialize()
{

    NTSTATUS Status;
    ULONG ReturnLength;
    HANDLE ProcessToken;
    TOKEN_STATISTICS ProcessTokenStatistics;
    PTOKEN_PRIVILEGES NewState;


    if (!TSeVariableInitialization()) {
        DbgPrint("Se:    Failed to initialize global test variables.\n");
        return FALSE;
    }


    DisabledGroupAttributes =  (SE_GROUP_ENABLED_BY_DEFAULT);

    OptionalGroupAttributes =  (SE_GROUP_ENABLED_BY_DEFAULT |
                                SE_GROUP_ENABLED
                                );
    NormalGroupAttributes =    (SE_GROUP_MANDATORY          |
                                SE_GROUP_ENABLED_BY_DEFAULT |
                                SE_GROUP_ENABLED
                                );
    OwnerGroupAttributes  =    (SE_GROUP_MANDATORY          |
                                SE_GROUP_ENABLED_BY_DEFAULT |
                                SE_GROUP_ENABLED            |
                                SE_GROUP_OWNER
                                );


    PrimarySecurityDescriptor =
        (PSECURITY_DESCRIPTOR)TstAllocatePool( PagedPool, 1024 );

    Status = RtlCreateSecurityDescriptor (
                 PrimarySecurityDescriptor,
                 SECURITY_DESCRIPTOR_REVISION1
                 ); ASSERT(NT_SUCCESS(Status));
    Status = RtlSetDaclSecurityDescriptor (
                 PrimarySecurityDescriptor,
                 TRUE,                  //DaclPresent,
                 NULL,                  //Dacl OPTIONAL,  // No protection
                 FALSE                  //DaclDefaulted OPTIONAL
                 ); ASSERT(NT_SUCCESS(Status));


    InitializeObjectAttributes(
        &PrimaryTokenAttributes,
        NULL,
        OBJ_INHERIT,
        NULL,
        PrimarySecurityDescriptor
        );


    ImpersonationSecurityDescriptor =
        (PSECURITY_DESCRIPTOR)TstAllocatePool( PagedPool, 1024 );

    ImpersonationSecurityQos.Length = (ULONG)sizeof(SECURITY_QUALITY_OF_SERVICE);
    ImpersonationSecurityQos.ImpersonationLevel = SecurityImpersonation;
    ImpersonationSecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    ImpersonationSecurityQos.EffectiveOnly = FALSE;

    InitializeObjectAttributes(
        &ImpersonationTokenAttributes,
        NULL,
        OBJ_INHERIT,
        NULL,
        NULL
        );
    ImpersonationTokenAttributes.SecurityQualityOfService =
        &ImpersonationSecurityQos;


    AnonymousSecurityDescriptor =
        (PSECURITY_DESCRIPTOR)TstAllocatePool( PagedPool, 1024 );

    AnonymousSecurityQos.Length = (ULONG)sizeof(SECURITY_QUALITY_OF_SERVICE);
    AnonymousSecurityQos.ImpersonationLevel = SecurityAnonymous;
    AnonymousSecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    AnonymousSecurityQos.EffectiveOnly = FALSE;

    InitializeObjectAttributes(
        &AnonymousTokenAttributes,
        NULL,
        OBJ_INHERIT,
        NULL,
        NULL
        );
    AnonymousTokenAttributes.SecurityQualityOfService =
        &AnonymousSecurityQos;


    //
    // Build an ACL for use.
    //

    Dacl        = (PACL)TstAllocatePool( PagedPool, 256 );

    Dacl->AclRevision=ACL_REVISION;
    Dacl->Sbz1=0;
    Dacl->Sbz2=0;
    Dacl->AclSize=256;
    Dacl->AceCount=0;


    //
    // Set up expiration times
    //

    TempTimeFields.Year = 3000;
    TempTimeFields.Month = 1;
    TempTimeFields.Day = 1;
    TempTimeFields.Hour = 1;
    TempTimeFields.Minute = 1;
    TempTimeFields.Second = 1;
    TempTimeFields.Milliseconds = 1;
    TempTimeFields.Weekday = 1;

    RtlTimeFieldsToTime( &TempTimeFields, &NoExpiration );

    //
    // Set up a bad authentication ID
    //

    BadAuthenticationId = RtlConvertLongToLuid(1);


    //
    // Use a token source specific to security test
    //

    NtAllocateLocallyUniqueId( &(TestSource.SourceIdentifier) );

    //
    // Create a new thread for impersonation tests
    //


    //
    // Initialize object attributes.
    // Note that the name of the thread is NULL so that we
    // can run multiple copies of the test at the same time
    // without collisions.
    //

    InitializeObjectAttributes(&ThreadObja, NULL, 0, NULL, NULL);

    //
    // Initialize thread context and initial TEB.
    //

    RtlInitializeContext(NtCurrentProcess(),
                         &ThreadContext,
                         NULL,
                         (PVOID)TestTokenInitialize,
                         &ThreadStack[254]);

    InitialTeb.StackBase = &ThreadStack[254];
    InitialTeb.StackLimit = &ThreadStack[0];

    //
    // Create a thread in a suspended state.
    //

    Status = NtCreateThread(&ThreadHandle,
                            THREAD_ALL_ACCESS,
                            &ThreadObja,
                            NtCurrentProcess(),
                            &ThreadClientId,
                            &ThreadContext,
                            &InitialTeb,
                            TRUE);

    ASSERT(NT_SUCCESS(Status));



    //
    // The following is sortof a horse-before-the-cart type of initialization.
    // Now that the system is enforcing things like "you can only create a
    // token with an AuthenticationId that the reference monitor has been told
    // about, it is necessary to obtain some information out of our current
    // token.
    //

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_ALL_ACCESS,
                 &ProcessToken
                 );
    ASSERT( NT_SUCCESS(Status) );
    Status = NtQueryInformationToken(
                 ProcessToken,                 // Handle
                 TokenStatistics,              // TokenInformationClass
                 &ProcessTokenStatistics,      // TokenInformation
                 sizeof(TOKEN_STATISTICS),     // TokenInformationLength
                 &ReturnLength                 // ReturnLength
                 );
    ASSERT(NT_SUCCESS(Status));
    OriginalAuthenticationId = ProcessTokenStatistics.AuthenticationId;


    DbgPrint("Se: enabling AssignPrimary & TCB privileges...\n");

    NewState = (PTOKEN_PRIVILEGES) TstAllocatePool(
                                    PagedPool,
                                    200
                                    );

    NewState->PrivilegeCount = 2;
    NewState->Privileges[0].Luid = CreateTokenPrivilege;
    NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    NewState->Privileges[1].Luid = AssignPrimaryTokenPrivilege;
    NewState->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;


    Status = NtAdjustPrivilegesToken(
                 ProcessToken,                     // TokenHandle
                 FALSE,                            // DisableAllPrivileges
                 NewState,                         // NewState (OPTIONAL)
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );

    if (Status != STATUS_SUCCESS) {

        DbgPrint("Failed to enable TCB and AssignPrimaryToken privilegs: 0x%x\n",Status);
        return FALSE;

    }

    DbgPrint("Done.\n");

    return TRUE;
}



////////////////////////////////////////////////////////////////
//                                                            //
// Test routines                                              //
//                                                            //
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//                                                            //
// Token Creation Test                                        //
//                                                            //
////////////////////////////////////////////////////////////////

BOOLEAN
TestTokenCreate()
{

    BOOLEAN CompletionStatus = TRUE;

    TOKEN_USER UserId;
    TOKEN_PRIMARY_GROUP PrimaryGroup;
    PTOKEN_GROUPS GroupIds;
    PTOKEN_PRIVILEGES Privileges;
    TOKEN_DEFAULT_DACL DefaultDacl;
    TOKEN_DEFAULT_DACL NullDefaultDacl;
    TOKEN_OWNER Owner;

    DbgPrint("\n");

    GroupIds = (PTOKEN_GROUPS)TstAllocatePool( PagedPool,
                                               GROUP_IDS_LENGTH
                                               );

    Privileges = (PTOKEN_PRIVILEGES)TstAllocatePool( PagedPool,
                                                     PRIVILEGES_LENGTH
                                                     );

    DefaultDacl.DefaultDacl = (PACL)TstAllocatePool( PagedPool,
                                                     DEFAULT_DACL_LENGTH
                                                     );


    //
    // Create the simplest token possible
    // (no Groups, explicit Owner, or DefaultDacl)
    //

    DbgPrint("Se:     Create Simple Token ...                                ");

    UserId.User.Sid = PebblesSid;
    UserId.User.Attributes = 0;
    GroupIds->GroupCount = 0;
    Privileges->PrivilegeCount = 0;
    PrimaryGroup.PrimaryGroup = FlintstoneSid;


    Status = NtCreateToken(
                 &Token,                   // Handle
                 (TOKEN_ALL_ACCESS),       // DesiredAccess
                 &PrimaryTokenAttributes,  // ObjectAttributes
                 TokenPrimary,             // TokenType
                 &SystemAuthenticationId,   // Authentication LUID
                 &NoExpiration,            // Expiration Time
                 &UserId,                  // Owner ID
                 GroupIds,                 // Group IDs
                 Privileges,               // Privileges
                 NULL,                     // Owner
                 &PrimaryGroup,            // Primary Group
                 NULL,                     // Default Dacl
                 &TestSource               // TokenSource
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
        Status = NtDuplicateObject(
                     NtCurrentProcess(),     // SourceProcessHandle
                     Token,                  // SourceHandle
                     NtCurrentProcess(),     // TargetProcessHandle
                     &SimpleToken,           // TargetHandle
                     0,                      // DesiredAccess (over-ridden by option)
                     0,                      // HandleAttributes
                     DUPLICATE_SAME_ACCESS   // Options
                     );
        ASSERT(NT_SUCCESS(Status));
        Status = NtClose(Token);
        ASSERT(NT_SUCCESS(Status));
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));



    //
    // Create a token with groups
    //

    DbgPrint("Se:     Create Token With Groups ...                           ");

    GroupIds->GroupCount = GROUP_COUNT;

    GroupIds->Groups[0].Sid  = FlintstoneSid;
    GroupIds->Groups[1].Sid       = ChildSid;
    GroupIds->Groups[2].Sid = NeandertholSid;
    GroupIds->Groups[3].Sid       = WorldSid;

    GroupIds->Groups[FLINTSTONE_INDEX].Attributes  = OwnerGroupAttributes;
    GroupIds->Groups[CHILD_INDEX].Attributes       = OptionalGroupAttributes;
    GroupIds->Groups[NEANDERTHOL_INDEX].Attributes = OptionalGroupAttributes;
    GroupIds->Groups[WORLD_INDEX].Attributes       = NormalGroupAttributes;


    UserId.User.Sid = PebblesSid;
    UserId.User.Attributes = 0;

    Privileges->PrivilegeCount = 0;

    PrimaryGroup.PrimaryGroup = FlintstoneSid;


    Status = NtCreateToken(
                 &Token,                   // Handle
                 (TOKEN_ALL_ACCESS),       // DesiredAccess
                 &PrimaryTokenAttributes,  // ObjectAttributes
                 TokenPrimary,             // TokenType
                 &OriginalAuthenticationId,   // Authentication LUID
                 &NoExpiration,            // Expiration Time
                 &UserId,                  // Owner ID
                 GroupIds,                 // Group IDs
                 Privileges,               // Privileges
                 NULL,                     // Owner
                 &PrimaryGroup,            // Primary Group
                 NULL,                     // Default Dacl
                 &TestSource               // TokenSource
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
        Status = NtDuplicateObject(
                     NtCurrentProcess(),     // SourceProcessHandle
                     Token,                  // SourceHandle
                     NtCurrentProcess(),     // TargetProcessHandle
                     &TokenWithGroups,       // TargetHandle
                     0,                      // DesiredAccess (over-ridden by option)
                     0,                      // HandleAttributes
                     DUPLICATE_SAME_ACCESS   // Options
                     );
        ASSERT(NT_SUCCESS(Status));
        Status = NtClose(Token);
        ASSERT(NT_SUCCESS(Status));
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));




    //
    // Create a token with default owner
    //

    DbgPrint("Se:     Create Token Default Owner ...                         ");

    GroupIds->GroupCount = GROUP_COUNT;

    GroupIds->Groups[FLINTSTONE_INDEX].Sid  = FlintstoneSid;
    GroupIds->Groups[CHILD_INDEX].Sid       = ChildSid;
    GroupIds->Groups[NEANDERTHOL_INDEX].Sid = NeandertholSid;
    GroupIds->Groups[WORLD_INDEX].Sid       = WorldSid;

    GroupIds->Groups[FLINTSTONE_INDEX].Attributes  = OwnerGroupAttributes;
    GroupIds->Groups[CHILD_INDEX].Attributes       = OptionalGroupAttributes;
    GroupIds->Groups[NEANDERTHOL_INDEX].Attributes = OptionalGroupAttributes;
    GroupIds->Groups[WORLD_INDEX].Attributes       = NormalGroupAttributes;


    UserId.User.Sid = PebblesSid;
    UserId.User.Attributes = 0;

    Owner.Owner = FlintstoneSid;

    Privileges->PrivilegeCount = 0;

    PrimaryGroup.PrimaryGroup = FlintstoneSid;


    Status = NtCreateToken(
                 &Token,                   // Handle
                 (TOKEN_ALL_ACCESS),       // DesiredAccess
                 &PrimaryTokenAttributes,  // ObjectAttributes
                 TokenPrimary,             // TokenType
                 &SystemAuthenticationId,   // Authentication LUID
                 &NoExpiration,            // Expiration Time
                 &UserId,                  // Owner ID
                 GroupIds,                 // Group IDs
                 Privileges,               // Privileges
                 &Owner,                   // Owner
                 &PrimaryGroup,            // Primary Group
                 NULL,                     // Default Dacl
                 &TestSource               // TokenSource
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
        Status = NtDuplicateObject(
                     NtCurrentProcess(),     // SourceProcessHandle
                     Token,                  // SourceHandle
                     NtCurrentProcess(),     // TargetProcessHandle
                     &TokenWithDefaultOwner, // TargetHandle
                     0,                      // DesiredAccess (over-ridden by option)
                     0,                      // HandleAttributes
                     DUPLICATE_SAME_ACCESS   // Options
                     );
        ASSERT(NT_SUCCESS(Status));
        Status = NtClose(Token);
        ASSERT(NT_SUCCESS(Status));
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));




    //
    // Create a token with default privileges
    //

    DbgPrint("Se:     Create Token privileges ...                            ");

    GroupIds->GroupCount = GROUP_COUNT;

    GroupIds->Groups[FLINTSTONE_INDEX].Sid  = FlintstoneSid;
    GroupIds->Groups[CHILD_INDEX].Sid       = ChildSid;
    GroupIds->Groups[NEANDERTHOL_INDEX].Sid = NeandertholSid;
    GroupIds->Groups[WORLD_INDEX].Sid       = WorldSid;

    GroupIds->Groups[FLINTSTONE_INDEX].Attributes  = OwnerGroupAttributes;
    GroupIds->Groups[CHILD_INDEX].Attributes       = OptionalGroupAttributes;
    GroupIds->Groups[NEANDERTHOL_INDEX].Attributes = OptionalGroupAttributes;
    GroupIds->Groups[WORLD_INDEX].Attributes       = NormalGroupAttributes;


    UserId.User.Sid = PebblesSid;
    UserId.User.Attributes = 0;

    Owner.Owner = FlintstoneSid;

    Privileges->PrivilegeCount = PRIVILEGE_COUNT;

    Privileges->Privileges[UNSOLICITED_INDEX].Luid = UnsolicitedInputPrivilege;
    Privileges->Privileges[SECURITY_INDEX].Luid = SecurityPrivilege;
    Privileges->Privileges[ASSIGN_PRIMARY_INDEX].Luid = AssignPrimaryTokenPrivilege;
    Privileges->Privileges[UNSOLICITED_INDEX].Attributes = 0;
    Privileges->Privileges[SECURITY_INDEX].Attributes = 0;
    Privileges->Privileges[ASSIGN_PRIMARY_INDEX].Attributes = SE_PRIVILEGE_ENABLED;

    PrimaryGroup.PrimaryGroup = FlintstoneSid;


    Status = NtCreateToken(
                 &Token,                   // Handle
                 (TOKEN_ALL_ACCESS),       // DesiredAccess
                 &PrimaryTokenAttributes,  // ObjectAttributes
                 TokenPrimary,             // TokenType
                 &OriginalAuthenticationId,   // Authentication LUID
                 &NoExpiration,            // Expiration Time
                 &UserId,                  // Owner ID
                 GroupIds,                 // Group IDs
                 Privileges,               // Privileges
                 &Owner,                   // Owner
                 &PrimaryGroup,            // Primary Group
                 NULL,                     // Default Dacl
                 &TestSource               // TokenSource
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
        Status = NtDuplicateObject(
                     NtCurrentProcess(),     // SourceProcessHandle
                     Token,                  // SourceHandle
                     NtCurrentProcess(),     // TargetProcessHandle
                     &TokenWithPrivileges,   // TargetHandle
                     0,                      // DesiredAccess (over-ridden by option)
                     0,                      // HandleAttributes
                     DUPLICATE_SAME_ACCESS   // Options
                     );
        ASSERT(NT_SUCCESS(Status));
        Status = NtClose(Token);
        ASSERT(NT_SUCCESS(Status));
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));




    //
    // Create a token with default DACL
    //

    DbgPrint("Se:     Create Token With Default Dacl ...                     ");

    GroupIds->GroupCount = GROUP_COUNT;

    GroupIds->Groups[FLINTSTONE_INDEX].Sid  = FlintstoneSid;
    GroupIds->Groups[CHILD_INDEX].Sid       = ChildSid;
    GroupIds->Groups[NEANDERTHOL_INDEX].Sid = NeandertholSid;
    GroupIds->Groups[WORLD_INDEX].Sid       = WorldSid;

    GroupIds->Groups[FLINTSTONE_INDEX].Attributes  = OwnerGroupAttributes;
    GroupIds->Groups[CHILD_INDEX].Attributes       = OptionalGroupAttributes;
    GroupIds->Groups[NEANDERTHOL_INDEX].Attributes = OptionalGroupAttributes;
    GroupIds->Groups[WORLD_INDEX].Attributes       = NormalGroupAttributes;

    UserId.User.Sid = PebblesSid;
    UserId.User.Attributes = 0;

    Owner.Owner = FlintstoneSid;

    Privileges->PrivilegeCount = PRIVILEGE_COUNT;

    Privileges->Privileges[UNSOLICITED_INDEX].Luid = UnsolicitedInputPrivilege;
    Privileges->Privileges[SECURITY_INDEX].Luid = SecurityPrivilege;
    Privileges->Privileges[ASSIGN_PRIMARY_INDEX].Luid = AssignPrimaryTokenPrivilege;
    Privileges->Privileges[UNSOLICITED_INDEX].Attributes = 0;
    Privileges->Privileges[SECURITY_INDEX].Attributes = 0;
    Privileges->Privileges[ASSIGN_PRIMARY_INDEX].Attributes = SE_PRIVILEGE_ENABLED;

    PrimaryGroup.PrimaryGroup = FlintstoneSid;

    Status = RtlCreateAcl( DefaultDacl.DefaultDacl, DEFAULT_DACL_LENGTH, ACL_REVISION);

    ASSERT(NT_SUCCESS(Status) );

    Status = NtCreateToken(
                 &Token,                   // Handle
                 (TOKEN_ALL_ACCESS),       // DesiredAccess
                 &PrimaryTokenAttributes,  // ObjectAttributes
                 TokenPrimary,             // TokenType
                 &SystemAuthenticationId,   // Authentication LUID
                 &NoExpiration,            // Expiration Time
                 &UserId,                  // Owner ID
                 GroupIds,                 // Group IDs
                 Privileges,               // Privileges
                 &Owner,                   // Owner
                 &PrimaryGroup,            // Primary Group
                 &DefaultDacl,             // Default Dacl
                 &TestSource               // TokenSource
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");

        //
        // Save a copy of this for later use...
        //

        Status = NtDuplicateObject(
                     NtCurrentProcess(),     // SourceProcessHandle
                     Token,                  // SourceHandle
                     NtCurrentProcess(),     // TargetProcessHandle
                     &TokenWithDefaultDacl,  // TargetHandle
                     0,                      // DesiredAccess (over-ridden by option)
                     0,                      // HandleAttributes
                     DUPLICATE_SAME_ACCESS   // Options
                     );
        ASSERT(NT_SUCCESS(Status));
        Status = NtClose(Token);
        ASSERT(NT_SUCCESS(Status));
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));




    //
    // Create a token with a null default DACL
    //

    DbgPrint("Se:     Create Token With a Null Default Dacl ...              ");

    GroupIds->GroupCount = GROUP_COUNT;

    GroupIds->Groups[FLINTSTONE_INDEX].Sid  = FlintstoneSid;
    GroupIds->Groups[CHILD_INDEX].Sid       = ChildSid;
    GroupIds->Groups[NEANDERTHOL_INDEX].Sid = NeandertholSid;
    GroupIds->Groups[WORLD_INDEX].Sid       = WorldSid;

    GroupIds->Groups[FLINTSTONE_INDEX].Attributes  = OwnerGroupAttributes;
    GroupIds->Groups[CHILD_INDEX].Attributes       = OptionalGroupAttributes;
    GroupIds->Groups[NEANDERTHOL_INDEX].Attributes = OptionalGroupAttributes;
    GroupIds->Groups[WORLD_INDEX].Attributes       = NormalGroupAttributes;

    UserId.User.Sid = PebblesSid;
    UserId.User.Attributes = 0;

    Owner.Owner = FlintstoneSid;

    Privileges->PrivilegeCount = PRIVILEGE_COUNT;

    Privileges->Privileges[UNSOLICITED_INDEX].Luid = UnsolicitedInputPrivilege;
    Privileges->Privileges[SECURITY_INDEX].Luid = SecurityPrivilege;
    Privileges->Privileges[ASSIGN_PRIMARY_INDEX].Luid = AssignPrimaryTokenPrivilege;
    Privileges->Privileges[UNSOLICITED_INDEX].Attributes = 0;
    Privileges->Privileges[SECURITY_INDEX].Attributes = 0;
    Privileges->Privileges[ASSIGN_PRIMARY_INDEX].Attributes = SE_PRIVILEGE_ENABLED;

    PrimaryGroup.PrimaryGroup = FlintstoneSid;

    NullDefaultDacl.DefaultDacl =  NULL;


    Status = NtCreateToken(
                 &Token,                   // Handle
                 (TOKEN_ALL_ACCESS),       // DesiredAccess
                 &PrimaryTokenAttributes,  // ObjectAttributes
                 TokenPrimary,             // TokenType
                 &OriginalAuthenticationId,   // Authentication LUID
                 &NoExpiration,            // Expiration Time
                 &UserId,                  // Owner ID
                 GroupIds,                 // Group IDs
                 Privileges,               // Privileges
                 &Owner,                   // Owner
                 &PrimaryGroup,            // Primary Group
                 &NullDefaultDacl,         // Default Dacl
                 &TestSource               // TokenSource
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
        Status = NtClose(Token);
        ASSERT(NT_SUCCESS(Status));
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));




    //
    // Create an impersonation token, Impersonation level = Impersonation
    //

    DbgPrint("Se:     Create an impersonation token ...                      ");

    GroupIds->GroupCount = GROUP_COUNT;

    GroupIds->Groups[FLINTSTONE_INDEX].Sid  = FlintstoneSid;
    GroupIds->Groups[CHILD_INDEX].Sid       = ChildSid;
    GroupIds->Groups[NEANDERTHOL_INDEX].Sid = NeandertholSid;
    GroupIds->Groups[WORLD_INDEX].Sid       = WorldSid;

    GroupIds->Groups[FLINTSTONE_INDEX].Attributes  = OwnerGroupAttributes;
    GroupIds->Groups[CHILD_INDEX].Attributes       = OptionalGroupAttributes;
    GroupIds->Groups[NEANDERTHOL_INDEX].Attributes = OptionalGroupAttributes;
    GroupIds->Groups[WORLD_INDEX].Attributes       = NormalGroupAttributes;

    UserId.User.Sid = PebblesSid;
    UserId.User.Attributes = 0;

    Owner.Owner = FlintstoneSid;

    Privileges->PrivilegeCount = PRIVILEGE_COUNT;

    Privileges->Privileges[UNSOLICITED_INDEX].Luid = UnsolicitedInputPrivilege;
    Privileges->Privileges[SECURITY_INDEX].Luid = SecurityPrivilege;
    Privileges->Privileges[ASSIGN_PRIMARY_INDEX].Luid = AssignPrimaryTokenPrivilege;
    Privileges->Privileges[UNSOLICITED_INDEX].Attributes = 0;
    Privileges->Privileges[SECURITY_INDEX].Attributes = 0;
    Privileges->Privileges[ASSIGN_PRIMARY_INDEX].Attributes = SE_PRIVILEGE_ENABLED;

    PrimaryGroup.PrimaryGroup = FlintstoneSid;

    Status = RtlCreateAcl( DefaultDacl.DefaultDacl, DEFAULT_DACL_LENGTH, ACL_REVISION);

    ASSERT(NT_SUCCESS(Status) );

    Status = NtCreateToken(
                 &Token,                   // Handle
                 (TOKEN_ALL_ACCESS),       // DesiredAccess
                 &ImpersonationTokenAttributes,  // ObjectAttributes
                 TokenImpersonation,       // TokenType
                 &SystemAuthenticationId,   // Authentication LUID
                 &NoExpiration,            // Expiration Time
                 &UserId,                  // Owner ID
                 GroupIds,                 // Group IDs
                 Privileges,               // Privileges
                 &Owner,                   // Owner
                 &PrimaryGroup,            // Primary Group
                 &DefaultDacl,             // Default Dacl
                 &TestSource               // TokenSource
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
        Status = NtDuplicateObject(
                     NtCurrentProcess(),     // SourceProcessHandle
                     Token,                  // SourceHandle
                     NtCurrentProcess(),     // TargetProcessHandle
                     &ImpersonationToken,    // TargetHandle
                     0,                      // DesiredAccess (over-ridden by option)
                     0,                      // HandleAttributes
                     DUPLICATE_SAME_ACCESS   // Options
                     );
        ASSERT(NT_SUCCESS(Status));
        Status = NtClose(Token);
        ASSERT(NT_SUCCESS(Status));
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));




    //
    // Create an impersonation token, Impersonation level = Anonymous
    //

    DbgPrint("Se:     Create an anonymous token ...                          ");

    GroupIds->GroupCount = GROUP_COUNT;

    GroupIds->Groups[FLINTSTONE_INDEX].Sid  = FlintstoneSid;
    GroupIds->Groups[CHILD_INDEX].Sid       = ChildSid;
    GroupIds->Groups[NEANDERTHOL_INDEX].Sid = NeandertholSid;
    GroupIds->Groups[WORLD_INDEX].Sid       = WorldSid;

    GroupIds->Groups[FLINTSTONE_INDEX].Attributes  = OwnerGroupAttributes;
    GroupIds->Groups[CHILD_INDEX].Attributes       = OptionalGroupAttributes;
    GroupIds->Groups[NEANDERTHOL_INDEX].Attributes = OptionalGroupAttributes;
    GroupIds->Groups[WORLD_INDEX].Attributes       = NormalGroupAttributes;

    UserId.User.Sid = PebblesSid;
    UserId.User.Attributes = 0;

    Owner.Owner = FlintstoneSid;

    Privileges->PrivilegeCount = PRIVILEGE_COUNT;

    Privileges->Privileges[UNSOLICITED_INDEX].Luid = UnsolicitedInputPrivilege;
    Privileges->Privileges[SECURITY_INDEX].Luid = SecurityPrivilege;
    Privileges->Privileges[ASSIGN_PRIMARY_INDEX].Luid = AssignPrimaryTokenPrivilege;
    Privileges->Privileges[UNSOLICITED_INDEX].Attributes = 0;
    Privileges->Privileges[SECURITY_INDEX].Attributes = 0;
    Privileges->Privileges[ASSIGN_PRIMARY_INDEX].Attributes = SE_PRIVILEGE_ENABLED;

    PrimaryGroup.PrimaryGroup = FlintstoneSid;

    Status = RtlCreateAcl( DefaultDacl.DefaultDacl, DEFAULT_DACL_LENGTH, ACL_REVISION);

    ASSERT(NT_SUCCESS(Status) );

    Status = NtCreateToken(
                 &Token,                     // Handle
                 (TOKEN_ALL_ACCESS),         // DesiredAccess
                 &AnonymousTokenAttributes,  // ObjectAttributes
                 TokenImpersonation,         // TokenType
                 &OriginalAuthenticationId,     // Authentication LUID
                 &NoExpiration,              // Expiration Time
                 &UserId,                    // Owner ID
                 GroupIds,                   // Group IDs
                 Privileges,                 // Privileges
                 &Owner,                     // Owner
                 &PrimaryGroup,              // Primary Group
                 &DefaultDacl,               // Default Dacl
                 &TestSource                 // TokenSource
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
        Status = NtDuplicateObject(
                     NtCurrentProcess(),     // SourceProcessHandle
                     Token,                  // SourceHandle
                     NtCurrentProcess(),     // TargetProcessHandle
                     &AnonymousToken,        // TargetHandle
                     0,                      // DesiredAccess (over-ridden by option)
                     0,                      // HandleAttributes
                     DUPLICATE_SAME_ACCESS   // Options
                     );
        ASSERT(NT_SUCCESS(Status));
        Status = NtClose(Token);
        ASSERT(NT_SUCCESS(Status));
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));



    //
    // Create the simplest token possible
    // (no Groups, explicit Owner, or DefaultDacl)
    //

    DbgPrint("Se:     Create Token With Bad Authentication Id ...            ");

    UserId.User.Sid = PebblesSid;
    UserId.User.Attributes = 0;
    GroupIds->GroupCount = 0;
    Privileges->PrivilegeCount = 0;
    PrimaryGroup.PrimaryGroup = FlintstoneSid;


    Status = NtCreateToken(
                 &Token,                   // Handle
                 (TOKEN_ALL_ACCESS),       // DesiredAccess
                 &PrimaryTokenAttributes,  // ObjectAttributes
                 TokenPrimary,             // TokenType
                 &BadAuthenticationId,     // Authentication LUID
                 &NoExpiration,            // Expiration Time
                 &UserId,                  // Owner ID
                 GroupIds,                 // Group IDs
                 Privileges,               // Privileges
                 NULL,                     // Owner
                 &PrimaryGroup,            // Primary Group
                 NULL,                     // Default Dacl
                 &TestSource               // TokenSource
                 );

    if (Status == STATUS_NO_SUCH_LOGON_SESSION) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Status should be: 0x%lx \n", STATUS_NO_SUCH_LOGON_SESSION);
        CompletionStatus = FALSE;
    }






    //
    // All done with this test
    //

    return CompletionStatus;
}

////////////////////////////////////////////////////////////////
//                                                            //
// Token Filtering Test                                       //
//                                                            //
////////////////////////////////////////////////////////////////

BOOLEAN
TestTokenFilter()
{

    BOOLEAN CompletionStatus = TRUE;

    PTOKEN_GROUPS GroupIds;
    PTOKEN_GROUPS RestrictedGroupIds;
    PTOKEN_PRIVILEGES Privileges;

    DbgPrint("\n");

    GroupIds = (PTOKEN_GROUPS)TstAllocatePool( PagedPool,
                                               GROUP_IDS_LENGTH
                                               );

    RestrictedGroupIds = (PTOKEN_GROUPS)TstAllocatePool( PagedPool,
                                                         GROUP_IDS_LENGTH
                                                         );

    Privileges = (PTOKEN_PRIVILEGES)TstAllocatePool( PagedPool,
                                                     PRIVILEGES_LENGTH
                                                     );




    //
    // Filter a token without doing anything
    //

    DbgPrint("Se:     Filter null Token ...                                  ");
    Status = NtFilterToken(
                TokenWithGroups,
                0,                      // no flags
                NULL,                   // no groups to disable
                NULL,                   // no privileges to disable
                NULL,                   // no restricted sids
                &Token
                );
    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
        NtClose(Token);
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }

    //
    // Filter a token and remove some groups
    //

    GroupIds->GroupCount = 2;

    GroupIds->Groups[0].Sid  = FlintstoneSid;
    GroupIds->Groups[1].Sid       = ChildSid;

    GroupIds->Groups[FLINTSTONE_INDEX].Attributes  = SE_GROUP_USE_FOR_DENY_ONLY;
    GroupIds->Groups[CHILD_INDEX].Attributes       = SE_GROUP_USE_FOR_DENY_ONLY;


    DbgPrint("Se:     Filter token with disabled groups ...                  ");
    Status = NtFilterToken(
                TokenWithGroups,
                0,                      // no flags
                GroupIds,               // no groups to disable
                NULL,                   // no privileges to disable
                NULL,                   // no restricted sids
                &TokenWithRestrictedGroups
                );
    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }


    //
    // Filter a token and remove some privileges
    //


    Privileges->PrivilegeCount = 2;

    Privileges->Privileges[UNSOLICITED_INDEX].Luid = UnsolicitedInputPrivilege;
    Privileges->Privileges[SECURITY_INDEX].Luid = SecurityPrivilege;
    Privileges->Privileges[UNSOLICITED_INDEX].Attributes = 0;
    Privileges->Privileges[SECURITY_INDEX].Attributes = 0;


    DbgPrint("Se:     Filter token with disabled privilegs ...               ");
    Status = NtFilterToken(
                TokenWithPrivileges,
                0,                      // no flags
                NULL,                   // no groups to disable
                Privileges,             // no privileges to disable
                NULL,                   // no restricted sids
                &TokenWithRestrictedPrivileges
                );
    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }


    //
    // Filter a restricted token and add some restricted sids
    //

    RestrictedGroupIds->GroupCount = RESTRICTED_SID_COUNT;

    RestrictedGroupIds->GroupCount = RESTRICTED_SID_COUNT;

    RestrictedGroupIds->Groups[0].Sid  = FlintstoneSid;
    RestrictedGroupIds->Groups[1].Sid       = ChildSid;

    RestrictedGroupIds->Groups[FLINTSTONE_INDEX].Attributes  = OwnerGroupAttributes;
    RestrictedGroupIds->Groups[CHILD_INDEX].Attributes       = OptionalGroupAttributes;



    DbgPrint("Se:     Filter token with restricted sids ...                 ");
    Status = NtFilterToken(
                TokenWithGroups,
                0,                      // no flags
                NULL,                   // no groups to disable
                NULL,                   // no privileges to disable
                RestrictedGroupIds,     // no restricted sids
                &TokenWithRestrictedSids
                );
    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }


    //
    // Filter a token and add some restricted sids
    //

    RestrictedGroupIds->GroupCount = RESTRICTED_SID_COUNT;

    RestrictedGroupIds->Groups[0].Sid       = NeandertholSid;
    RestrictedGroupIds->Groups[1].Sid       = WorldSid;

    RestrictedGroupIds->Groups[0].Attributes  = OwnerGroupAttributes;
    RestrictedGroupIds->Groups[1].Attributes       = OptionalGroupAttributes;


    DbgPrint("Se:     Filter token with more restricted sids ...             ");
    Status = NtFilterToken(
                TokenWithRestrictedSids,
                0,                      // no flags
                NULL,                   // no groups to disable
                NULL,                   // no privileges to disable
                RestrictedGroupIds,     // no restricted sids
                &TokenWithMoreRestrictedSids
                );
    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }


    //
    // All done with this test
    //

    return CompletionStatus;
}



////////////////////////////////////////////////////////////////
//                                                            //
// Open Primary Token Test                                    //
//                                                            //
////////////////////////////////////////////////////////////////

BOOLEAN
TestTokenOpenPrimary()
{
    NTSTATUS Status;
    BOOLEAN CompletionStatus = TRUE;

    HANDLE ProcessToken;
    HANDLE SubProcessToken;
    HANDLE SubProcess;

    TOKEN_STATISTICS ProcessTokenStatistics;
    TOKEN_STATISTICS SubProcessTokenStatistics;

    ULONG ReturnLength;

    DbgPrint("\n");

    //
    // Open the current process's token
    //

    DbgPrint("Se:     Open own process's token ...                           ");

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_ALL_ACCESS,
                 &ProcessToken
                 );
    if (NT_SUCCESS(Status)) {
        Status = NtQueryInformationToken(
                     ProcessToken,                 // Handle
                     TokenStatistics,              // TokenInformationClass
                     &ProcessTokenStatistics,      // TokenInformation
                     sizeof(TOKEN_STATISTICS),     // TokenInformationLength
                     &ReturnLength                 // ReturnLength
                     );
        ASSERT(NT_SUCCESS(Status));
        if ( ProcessTokenStatistics.TokenType == TokenPrimary) {
            if ( RtlEqualLuid( &ProcessTokenStatistics.AuthenticationId,
                               &OriginalAuthenticationId ) ) {
                DbgPrint("Succeeded.\n");
            } else {
                DbgPrint("********** Failed ************\n");
                DbgPrint("Unexpected authentication ID value.\n");
                DbgPrint("Authentication ID is: ");
                TestpPrintLuid(ProcessTokenStatistics.AuthenticationId);
                DbgPrint("\n");
                CompletionStatus = FALSE;
            }

        } else {
            DbgPrint("********** Failed ************\n");
            DbgPrint("Token type not TokenPrimary.\n");
            DbgPrint("Returned token type is: 0x%lx \n",
                    ProcessTokenStatistics.TokenType);
            DbgPrint("Authentication ID is: ");
            TestpPrintLuid(ProcessTokenStatistics.AuthenticationId);
            DbgPrint("\n");
            CompletionStatus = FALSE;
        }
        Status = NtClose(ProcessToken);
        ASSERT(NT_SUCCESS(Status));

    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }






    //
    // Open another process's token
    //

    DbgPrint("Se:     Open another process's token ...                       ");

    Status = NtCreateProcess(
                 &SubProcess,
                 (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE),
                 NULL,
                 NtCurrentProcess(),   // ParentProcess
                 FALSE,                // InheritObjectTable
                 NULL,                 // SectionHandle,
                 NULL,                 // DebugPort,
                 NULL                  // ExceptionPort
                 );

    Status = NtOpenProcessToken(
                 SubProcess,
                 TOKEN_ALL_ACCESS,
                 &SubProcessToken
                 );
    if (NT_SUCCESS(Status)) {
        Status = NtQueryInformationToken(
                     SubProcessToken,              // Handle
                     TokenStatistics,              // TokenInformationClass
                     &SubProcessTokenStatistics,   // TokenInformation
                     sizeof(TOKEN_STATISTICS),     // TokenInformationLength
                     &ReturnLength                 // ReturnLength
                     );
        ASSERT(NT_SUCCESS(Status));
        if ( SubProcessTokenStatistics.TokenType == TokenPrimary) {
            if ( RtlEqualLuid( &SubProcessTokenStatistics.AuthenticationId,
                               &OriginalAuthenticationId ) ) {
                if ( (ProcessTokenStatistics.TokenId.HighPart ==
                      SubProcessTokenStatistics.TokenId.HighPart)  &&
                     (ProcessTokenStatistics.TokenId.LowPart ==
                      SubProcessTokenStatistics.TokenId.LowPart) ) {
                    DbgPrint("********** Failed ************\n");
                    DbgPrint("Same token as parent process (token IDs match).\n");
                    DbgPrint("Authentication ID is: ");
                    TestpPrintLuid(SubProcessTokenStatistics.AuthenticationId);
                    DbgPrint("\n");
                    CompletionStatus = FALSE;

                } else {
                    DbgPrint("Succeeded.\n");
                }
            } else {
                DbgPrint("********** Failed ************\n");
                DbgPrint("Unexpected authentication ID value.\n");
                DbgPrint("Authentication ID is: ");
                TestpPrintLuid(SubProcessTokenStatistics.AuthenticationId);
                DbgPrint("\n");
                CompletionStatus = FALSE;
            }
        } else {
            DbgPrint("********** Failed ************\n");
            DbgPrint("Token type not TokenPrimary.\n");
            DbgPrint("Returned token type is: 0x%lx \n",
            SubProcessTokenStatistics.TokenType);
            DbgPrint("Authentication ID is: ");
            TestpPrintLuid(SubProcessTokenStatistics.AuthenticationId);
            DbgPrint("\n");
            CompletionStatus = FALSE;
        }
        Status = NtClose(SubProcessToken);
        ASSERT(NT_SUCCESS(Status));
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }


    return CompletionStatus;
}

////////////////////////////////////////////////////////////////
//                                                            //
// Query Token Test                                           //
//                                                            //
////////////////////////////////////////////////////////////////

BOOLEAN
TestTokenQuery()
{
    BOOLEAN CompletionStatus = TRUE;
    ULONG ReturnLength;
    BOOLEAN ValuesCompare;

    PTOKEN_USER UserId = NULL;
    PTOKEN_PRIMARY_GROUP PrimaryGroup = NULL;
    PTOKEN_GROUPS GroupIds = NULL;
    PTOKEN_GROUPS RestrictedSids = NULL;
    PTOKEN_PRIVILEGES Privileges = NULL;
    PTOKEN_OWNER Owner = NULL;
    PTOKEN_DEFAULT_DACL DefaultDacl = NULL;

    SECURITY_IMPERSONATION_LEVEL QueriedImpersonationLevel;
    TOKEN_SOURCE QueriedSource;
    TOKEN_TYPE QueriedType;
    TOKEN_STATISTICS QueriedStatistics;

    DbgPrint("\n");



#if 0

   //
   // Query invalid return buffer address
   //

    DbgPrint("Se:     Query with invalid buffer address ...                  ");

    UserId = (PTOKEN_USER)((PVOID)0x200L);
    Status = NtQueryInformationToken(
                 SimpleToken,              // Handle
                 TokenUser,                // TokenInformationClass
                 UserId,                   // TokenInformation
                 3000,                     // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_ACCESS_VIOLATION) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));

#endif  //0


////////////////////////////////////////////////////////////////////////
//                                                                    //
//        Query User ID                                               //
//                                                                    //
////////////////////////////////////////////////////////////////////////

   //
   // Query User ID with zero length buffer
   //

    DbgPrint("Se:     Query User ID with zero length buffer ...              ");

    Status = NtQueryInformationToken(
                 SimpleToken,              // Handle
                 TokenUser,                // TokenInformationClass
                 UserId,                   // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));




    UserId = (PTOKEN_USER)TstAllocatePool( PagedPool,
                                           ReturnLength
                                           );

    //
    // Query user SID
    // (This relies upon the ReturnLength returned from previous call)
    //

    DbgPrint("Se:     Query token user ...                                   ");

    Status = NtQueryInformationToken(
                 SimpleToken,              // Handle
                 TokenUser,                // TokenInformationClass
                 UserId,                   // TokenInformation
                 ReturnLength,             // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (NT_SUCCESS(Status)) {

        //
        // Check returned value
        //

        if (RtlEqualSid((UserId->User.Sid), PebblesSid) ) {
            DbgPrint("Succeeded.\n");
        } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Unexpected value returned by query.\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));


   //
   // Query with too little buffer
    // (This relies upon the ReturnLength returned from previous call)
   //

    DbgPrint("Se:     Query user with too small buffer ...                   ");

    Status = NtQueryInformationToken(
                 SimpleToken,              // Handle
                 TokenUser,                // TokenInformationClass
                 UserId,                   // TokenInformation
                 ReturnLength-1,           // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));


////////////////////////////////////////////////////////////////////////
//                                                                    //
//        Query Primary Group                                         //
//                                                                    //
////////////////////////////////////////////////////////////////////////

   //
   // Query primary group with zero length buffer
   //

    DbgPrint("Se:     Query primary group with zero length buffer ...        ");

    Status = NtQueryInformationToken(
                 SimpleToken,              // Handle
                 TokenPrimaryGroup,        // TokenInformationClass
                 PrimaryGroup,             // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));




    PrimaryGroup = (PTOKEN_PRIMARY_GROUP)TstAllocatePool( PagedPool,
                                                          ReturnLength
                                                          );

    //
    // Query primary group SID
    // (This relies upon the ReturnLength returned from previous call)
    //

    DbgPrint("Se:     Query primary group ...                                ");

    Status = NtQueryInformationToken(
                 SimpleToken,              // Handle
                 TokenPrimaryGroup,        // TokenInformationClass
                 PrimaryGroup,             // TokenInformation
                 ReturnLength,             // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (NT_SUCCESS(Status)) {

        //
        // Check returned value
        //

        if (RtlEqualSid( PrimaryGroup->PrimaryGroup, FlintstoneSid) ) {
            DbgPrint("Succeeded.\n");
        } else {
            DbgPrint("********** Failed ************\n");
            DbgPrint("Unexpected value returned by query.\n");
            DbgPrint("Status is: 0x%lx \n", Status);
            DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
            CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));


   //
   // Query with too little buffer
    // (This relies upon the ReturnLength returned from previous call)
   //

    DbgPrint("Se:     Query primary group with too small buffer ...          ");

    Status = NtQueryInformationToken(
                 SimpleToken,              // Handle
                 TokenPrimaryGroup,        // TokenInformationClass
                 PrimaryGroup,             // TokenInformation
                 ReturnLength-1,           // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));



////////////////////////////////////////////////////////////////////////
//                                                                    //
//        Query Groups                                                //
//                                                                    //
////////////////////////////////////////////////////////////////////////

   //
   // Query groups with zero length buffer
   //

    DbgPrint("Se:     Query groups with zero length buffer ...               ");

    Status = NtQueryInformationToken(
                 TokenWithGroups,          // Handle
                 TokenGroups,              // TokenInformationClass
                 GroupIds,                 // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));




    GroupIds = (PTOKEN_GROUPS)TstAllocatePool( PagedPool,
                                               ReturnLength
                                               );

    //
    // Query Group SIDs
    // (This relies upon the ReturnLength returned from previous call)
    //

    DbgPrint("Se:     Query groups ...                                       ");

    Status = NtQueryInformationToken(
                 TokenWithGroups,          // Handle
                 TokenGroups,              // TokenInformationClass
                 GroupIds,                 // TokenInformation
                 ReturnLength,             // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (NT_SUCCESS(Status)) {

        //
        // Check returned value
        //    Group count = 4
        //    SID 0 = Flintstone
        //    SID 1 = ChildSid
        //    SID 2 = NeandertholSid
        //    SID 3 = WorldSid
        //

        ValuesCompare = TRUE;

        if (GroupIds->GroupCount != GROUP_COUNT) {
            ValuesCompare = FALSE;
        }

        if ( (!RtlEqualSid((GroupIds->Groups[FLINTSTONE_INDEX].Sid),
                            FlintstoneSid)) ||
             (GroupIds->Groups[FLINTSTONE_INDEX].Attributes !=
              OwnerGroupAttributes) ) {
            ValuesCompare = FALSE;
        }

        if ( (!RtlEqualSid((GroupIds->Groups[CHILD_INDEX].Sid), ChildSid)) ||
             (GroupIds->Groups[CHILD_INDEX].Attributes !=
              OptionalGroupAttributes) ) {
            ValuesCompare = FALSE;
        }

        if ( (!RtlEqualSid((GroupIds->Groups[NEANDERTHOL_INDEX].Sid),
              NeandertholSid)) ||
             (GroupIds->Groups[NEANDERTHOL_INDEX].Attributes !=
              OptionalGroupAttributes) ) {
            ValuesCompare = FALSE;
        }

        if ( (!RtlEqualSid((GroupIds->Groups[WORLD_INDEX].Sid), WorldSid)) ||
             (GroupIds->Groups[WORLD_INDEX].Attributes != NormalGroupAttributes) ) {
            ValuesCompare = FALSE;
        }


        if ( ValuesCompare ) {
            DbgPrint("Succeeded.\n");
        } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Unexpected value returned by query.\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        DbgPrint("Returned group count is: 0x%lx \n", GroupIds->GroupCount);
        CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));


   //
   // Query with too little buffer
    // (This relies upon the ReturnLength returned from previous call)
   //

    DbgPrint("Se:     Query groups with too small buffer ...                 ");

    Status = NtQueryInformationToken(
                 TokenWithGroups,          // Handle
                 TokenGroups,              // TokenInformationClass
                 GroupIds,                 // TokenInformation
                 ReturnLength-1,           // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));

   //
   // Query groups with zero length buffer
   //

    DbgPrint("Se:     Query restgroups with zero length buffer ...           ");
    Status = NtQueryInformationToken(
                 TokenWithRestrictedGroups,// Handle
                 TokenGroups,              // TokenInformationClass
                 GroupIds,                 // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));




    GroupIds = (PTOKEN_GROUPS)TstAllocatePool( PagedPool,
                                               ReturnLength
                                               );

    //
    // Query Group SIDs
    // (This relies upon the ReturnLength returned from previous call)
    //

    DbgPrint("Se:     Query rest groups ...                                  ");

    Status = NtQueryInformationToken(
                 TokenWithRestrictedGroups,// Handle
                 TokenGroups,              // TokenInformationClass
                 GroupIds,                 // TokenInformation
                 ReturnLength,             // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (NT_SUCCESS(Status)) {

        //
        // Check returned value
        //    Group count = 4
        //    SID 0 = Flintstone
        //    SID 1 = ChildSid
        //    SID 2 = NeandertholSid
        //    SID 3 = WorldSid
        //

        ValuesCompare = TRUE;

        if (GroupIds->GroupCount != GROUP_COUNT) {
            ValuesCompare = FALSE;
        }

        if ( (!RtlEqualSid((GroupIds->Groups[FLINTSTONE_INDEX].Sid),
                            FlintstoneSid)) ||
             (GroupIds->Groups[FLINTSTONE_INDEX].Attributes !=
              ((OwnerGroupAttributes & ~(SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT)) | SE_GROUP_USE_FOR_DENY_ONLY) ) ) {
            ValuesCompare = FALSE;
        }

        if ( (!RtlEqualSid((GroupIds->Groups[CHILD_INDEX].Sid), ChildSid)) ||
             (GroupIds->Groups[CHILD_INDEX].Attributes !=
                            ((OptionalGroupAttributes & ~(SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT)) | SE_GROUP_USE_FOR_DENY_ONLY)) ) {
            ValuesCompare = FALSE;
        }

        if ( (!RtlEqualSid((GroupIds->Groups[NEANDERTHOL_INDEX].Sid),
              NeandertholSid)) ||
             (GroupIds->Groups[NEANDERTHOL_INDEX].Attributes !=
              OptionalGroupAttributes) ) {
            ValuesCompare = FALSE;
        }

        if ( (!RtlEqualSid((GroupIds->Groups[WORLD_INDEX].Sid), WorldSid)) ||
             (GroupIds->Groups[WORLD_INDEX].Attributes != NormalGroupAttributes) ) {
            ValuesCompare = FALSE;
        }


        if ( ValuesCompare ) {
            DbgPrint("Succeeded.\n");
        } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Unexpected value returned by query.\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        DbgPrint("Returned group count is: 0x%lx \n", GroupIds->GroupCount);
        CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));



////////////////////////////////////////////////////////////////////////
//                                                                    //
//        Query RestrictedSids                                        //
//                                                                    //
////////////////////////////////////////////////////////////////////////

   //
   // Query groups with zero length buffer
   //

    DbgPrint("Se:     Query null restricted with zero length buffer ...      ");

    Status = NtQueryInformationToken(
                 TokenWithGroups,          // Handle
                 TokenRestrictedSids,      // TokenInformationClass
                 RestrictedSids,           // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));

   //
   // Query groups with zero length buffer
   //

    DbgPrint("Se:     Query restricted sids with zero length buffer ...      ");

    Status = NtQueryInformationToken(
                 TokenWithRestrictedSids,  // Handle
                 TokenRestrictedSids,      // TokenInformationClass
                 RestrictedSids,           // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));




    RestrictedSids = (PTOKEN_GROUPS)TstAllocatePool( PagedPool,
                                                     ReturnLength
                                                     );

    //
    // Query Group SIDs
    // (This relies upon the ReturnLength returned from previous call)
    //

    DbgPrint("Se:     Query restricted sids ...                              ");

    Status = NtQueryInformationToken(
                 TokenWithRestrictedSids,  // Handle
                 TokenRestrictedSids,      // TokenInformationClass
                 RestrictedSids,           // TokenInformation
                 ReturnLength,             // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG


    if (NT_SUCCESS(Status)) {

        //
        // Check returned value
        //    Group count = 2
        //    SID 0 = Flintstone
        //    SID 1 = ChildSid
        //

        ValuesCompare = TRUE;

        if (RestrictedSids->GroupCount != RESTRICTED_SID_COUNT) {
            ValuesCompare = FALSE;
        }

        if ( (!RtlEqualSid((RestrictedSids->Groups[FLINTSTONE_INDEX].Sid),
                            FlintstoneSid)) ||
             (RestrictedSids->Groups[FLINTSTONE_INDEX].Attributes !=
              (SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY)) ) {
            ValuesCompare = FALSE;
        }

        if ( (!RtlEqualSid((RestrictedSids->Groups[CHILD_INDEX].Sid), ChildSid)) ||
             (RestrictedSids->Groups[CHILD_INDEX].Attributes !=
              (SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY)) ) {
            ValuesCompare = FALSE;
        }


        if ( ValuesCompare ) {
            DbgPrint("Succeeded.\n");
        } else {
            DbgPrint("********** Failed ************\n");
            DbgPrint("Unexpected value returned by query.\n");
            DbgPrint("Status is: 0x%lx \n", Status);
            DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
            DbgPrint("Returned group count is: 0x%lx \n", RestrictedSids->GroupCount);
            CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));

    //
   // Query restricted sids with zero length buffer
   //

    DbgPrint("Se:     Query more restricted sids with zero length buffer ... ");

    Status = NtQueryInformationToken(
                 TokenWithMoreRestrictedSids,  // Handle
                 TokenRestrictedSids,      // TokenInformationClass
                 RestrictedSids,           // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));




    RestrictedSids = (PTOKEN_GROUPS)TstAllocatePool( PagedPool,
                                                     ReturnLength
                                                     );

    //
    // Query Group SIDs
    // (This relies upon the ReturnLength returned from previous call)
    //

    DbgPrint("Se:     Query more restricted sids ...                         ");

    Status = NtQueryInformationToken(
                 TokenWithMoreRestrictedSids,  // Handle
                 TokenRestrictedSids,      // TokenInformationClass
                 RestrictedSids,           // TokenInformation
                 ReturnLength,             // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG


    if (NT_SUCCESS(Status)) {

        //
        // Check returned value
        //    Group count = 2
        //    SID 0 = Flintstone
        //    SID 1 = ChildSid
        //    SID 2 = Neaderthol
        //    SID 3 = World
        //

        ValuesCompare = TRUE;

        if (RestrictedSids->GroupCount != GROUP_COUNT) {
            ValuesCompare = FALSE;
        }

        if ( (!RtlEqualSid((RestrictedSids->Groups[FLINTSTONE_INDEX].Sid),
                            FlintstoneSid)) ||
             (RestrictedSids->Groups[FLINTSTONE_INDEX].Attributes !=
              (SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY)) ) {
            ValuesCompare = FALSE;
        }

        if ( (!RtlEqualSid((RestrictedSids->Groups[CHILD_INDEX].Sid), ChildSid)) ||
             (RestrictedSids->Groups[CHILD_INDEX].Attributes !=
              (SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY)) ) {
            ValuesCompare = FALSE;
        }


        if ( (!RtlEqualSid((RestrictedSids->Groups[NEANDERTHOL_INDEX].Sid),
                            NeandertholSid)) ||
             (RestrictedSids->Groups[NEANDERTHOL_INDEX].Attributes !=
              (SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY)) ) {
            ValuesCompare = FALSE;
        }

        if ( (!RtlEqualSid((RestrictedSids->Groups[WORLD_INDEX].Sid), WorldSid)) ||
             (RestrictedSids->Groups[WORLD_INDEX].Attributes !=
              (SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY)) ) {
            ValuesCompare = FALSE;
        }



        if ( ValuesCompare ) {
            DbgPrint("Succeeded.\n");
        } else {
            DbgPrint("********** Failed ************\n");
            DbgPrint("Unexpected value returned by query.\n");
            DbgPrint("Status is: 0x%lx \n", Status);
            DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
            DbgPrint("Returned group count is: 0x%lx \n", RestrictedSids->GroupCount);
            CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));


   //
   // Query with too little buffer
    // (This relies upon the ReturnLength returned from previous call)
   //

    DbgPrint("Se:     Query groups with too small buffer ...                 ");

    Status = NtQueryInformationToken(
                 TokenWithGroups,          // Handle
                 TokenGroups,              // TokenInformationClass
                 GroupIds,                 // TokenInformation
                 ReturnLength-1,           // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));


////////////////////////////////////////////////////////////////////////
//                                                                    //
//        Query Privileges                                            //
//                                                                    //
////////////////////////////////////////////////////////////////////////


   //
   // Query groups with zero length buffer
   //

    DbgPrint("Se:     Query privileges with zero length buffer ...           ");

    Status = NtQueryInformationToken(
                 TokenWithPrivileges,      // Handle
                 TokenPrivileges,          // TokenInformationClass
                 NULL,                     // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));




    Privileges = (PTOKEN_PRIVILEGES)TstAllocatePool( PagedPool,
                                                     ReturnLength
                                                     );

    //
    // Query privileges
    // (This relies upon the ReturnLength returned from previous call)
    //

    DbgPrint("Se:     Query privileges ...                                   ");

    Status = NtQueryInformationToken(
                 TokenWithPrivileges,      // Handle
                 TokenPrivileges,          // TokenInformationClass
                 Privileges,               // TokenInformation
                 ReturnLength,             // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (NT_SUCCESS(Status)) {

        //
        // Check returned value
        //    Privilege count = PRIVILEGE_COUNT
        //    Privilege UNSOLICITED_INDEX    = UnsolicitedInputPrivilege
        //    Privilege SECURITY_INDEX       = SecurityPrivilege
        //    Privilege ASSIGN_PRIMARY_INDEX = AssignPrimaryPrivilege
        //

        ValuesCompare = TRUE;

        if (Privileges->PrivilegeCount != PRIVILEGE_COUNT) {
            ValuesCompare = FALSE;
        }

        if ( !RtlEqualLuid(&Privileges->Privileges[UNSOLICITED_INDEX].Luid,
               &UnsolicitedInputPrivilege)      ||
             (Privileges->Privileges[UNSOLICITED_INDEX].Attributes != 0)             ) {
            ValuesCompare = FALSE;
        }

        if ( !RtlEqualLuid(&Privileges->Privileges[SECURITY_INDEX].Luid,
               &SecurityPrivilege)             ||
             (Privileges->Privileges[SECURITY_INDEX].Attributes != 0)             ) {
            ValuesCompare = FALSE;
        }

        if ( !RtlEqualLuid(&Privileges->Privileges[ASSIGN_PRIMARY_INDEX].Luid,
               &AssignPrimaryTokenPrivilege)             ||
             (Privileges->Privileges[ASSIGN_PRIMARY_INDEX].Attributes != SE_PRIVILEGE_ENABLED)             ) {
            ValuesCompare = FALSE;
        }


        if ( ValuesCompare ) {
            DbgPrint("Succeeded.\n");
        } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Unexpected value returned by query.\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));


   //
   // Query with too little buffer
    // (This relies upon the ReturnLength returned from previous call)
   //

    DbgPrint("Se:     Query privileges with too small buffer ...             ");

    Status = NtQueryInformationToken(
                 TokenWithPrivileges,      // Handle
                 TokenPrivileges,          // TokenInformationClass
                 Privileges,               // TokenInformation
                 ReturnLength-1,           // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));

   //
   // Query groups with zero length buffer
   //

    DbgPrint("Se:     Query rest privileges with zero length buffer ...      ");

    ReturnLength = 0;
    Status = NtQueryInformationToken(
                 TokenWithRestrictedPrivileges,      // Handle
                 TokenPrivileges,          // TokenInformationClass
                 NULL,                     // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));




    Privileges = (PTOKEN_PRIVILEGES)TstAllocatePool( PagedPool,
                                                     ReturnLength
                                                     );

    //
    // Query privileges
    // (This relies upon the ReturnLength returned from previous call)
    //

    DbgPrint("Se:     Query rest privileges ...                              ");

    Status = NtQueryInformationToken(
                 TokenWithRestrictedPrivileges,      // Handle
                 TokenPrivileges,          // TokenInformationClass
                 Privileges,               // TokenInformation
                 ReturnLength,             // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (NT_SUCCESS(Status)) {

        //
        // Check returned value
        //    Privilege count = PRIVILEGE_COUNT -2
        //    Privilege ASSIGN_PRIMARY_INDEX = AssignPrimaryPrivilege
        //

        ValuesCompare = TRUE;

        if (Privileges->PrivilegeCount != PRIVILEGE_COUNT - 2) {
            ValuesCompare = FALSE;
        }


        if ( !RtlEqualLuid(&Privileges->Privileges[0].Luid,
               &AssignPrimaryTokenPrivilege)             ||
             (Privileges->Privileges[0].Attributes != SE_PRIVILEGE_ENABLED)             ) {
            ValuesCompare = FALSE;
        }


        if ( ValuesCompare ) {
            DbgPrint("Succeeded.\n");
        } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Unexpected value returned by query.\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));


////////////////////////////////////////////////////////////////////////
//                                                                    //
//        Query Owner                                                 //
//                                                                    //
////////////////////////////////////////////////////////////////////////

   //
   // Query Owner of simple token with zero length buffer
   //

    DbgPrint("Se:     Query Owner of simple token with zero length buffer... ");

    Status = NtQueryInformationToken(
                 SimpleToken,              // Handle
                 TokenOwner,               // TokenInformationClass
                 Owner,                    // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));



    Owner = (PTOKEN_OWNER)TstAllocatePool( PagedPool,
                                           ReturnLength
                                           );

    //
    // Query Owner SID
    // (This relies upon the ReturnLength returned from previous call)
    //

    DbgPrint("Se:     Query owner of simple token ...                        ");

    Status = NtQueryInformationToken(
                 SimpleToken,              // Handle
                 TokenOwner,               // TokenInformationClass
                 Owner,                    // TokenInformation
                 ReturnLength,             // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (NT_SUCCESS(Status)) {

        //
        // Check returned value
        //

        if (RtlEqualSid((Owner->Owner), PebblesSid) ) {
            DbgPrint("Succeeded.\n");
        } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Unexpected value returned by query.\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));


    //
    // Query owner of simple token with too little buffer
    // (This relies upon the ReturnLength returned from previous call)
    //

    DbgPrint("Se:     Query owner of simple token with too small buffer ...  ");

    Status = NtQueryInformationToken(
                 SimpleToken,              // Handle
                 TokenOwner,               // TokenInformationClass
                 Owner,                    // TokenInformation
                 ReturnLength-1,           // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));


    //
    // Query default owner of token with zero length buffer
    //

    DbgPrint("Se:     Query Default Owner of token with zero length buffer...");

    Status = NtQueryInformationToken(
                 TokenWithDefaultOwner,    // Handle
                 TokenOwner,               // TokenInformationClass
                 Owner,                    // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));




    Owner = (PTOKEN_OWNER)TstAllocatePool( PagedPool,
                                           ReturnLength
                                           );

    //
    // Query default owner of token
    // (This relies upon the ReturnLength returned from previous call)
    //

    DbgPrint("Se:     Query default owner of token ...                       ");

    Status = NtQueryInformationToken(
                 TokenWithDefaultOwner,    // Handle
                 TokenOwner,               // TokenInformationClass
                 Owner,                    // TokenInformation
                 ReturnLength,             // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (NT_SUCCESS(Status)) {

        //
        // Check returned value
        //

        if (RtlEqualSid((Owner->Owner), FlintstoneSid) ) {
            DbgPrint("Succeeded.\n");
        } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Unexpected value returned by query.\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));


    //
    // Query default owner of token with too little buffer
    // (This relies upon the ReturnLength returned from previous call)
    //

    DbgPrint("Se:     Query default owner of token with too small buffer ... ");

    Status = NtQueryInformationToken(
                 TokenWithDefaultOwner,    // Handle
                 TokenOwner,               // TokenInformationClass
                 Owner,                    // TokenInformation
                 ReturnLength-1,           // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));


////////////////////////////////////////////////////////////////////////
//                                                                    //
//        Query Default Dacl                                          //
//                                                                    //
////////////////////////////////////////////////////////////////////////

   //
   // Query default dacl with zero length buffer
   //

    DbgPrint("Se:     Query default DACL with zero length buffer ...         ");

    Status = NtQueryInformationToken(
                 TokenWithDefaultDacl,     // Handle
                 TokenDefaultDacl,         // TokenInformationClass
                 DefaultDacl,              // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));




    DefaultDacl = (PTOKEN_DEFAULT_DACL)TstAllocatePool( PagedPool,
                                                        ReturnLength
                                                        );

    //
    // Query default dacl
    // (This relies upon the ReturnLength returned from previous call)
    //

    DbgPrint("Se:     Query default dacl ...                                 ");

    Status = NtQueryInformationToken(
                 TokenWithDefaultDacl,     // Handle
                 TokenDefaultDacl,         // TokenInformationClass
                 DefaultDacl,              // TokenInformation
                 ReturnLength,             // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (NT_SUCCESS(Status)) {

        //
        // Check returned value
        //

        if (RtlValidAcl(DefaultDacl->DefaultDacl)) {

            if (DefaultDacl->DefaultDacl->AceCount == 0) {

                DbgPrint("Succeeded.\n");
            } else {
                DbgPrint("********** Failed ************\n");
                DbgPrint("Unexpected value returned by query.\n");
                DbgPrint("Status is: 0x%lx \n", Status);
                DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
                CompletionStatus = FALSE;
            }
        } else {
            DbgPrint("********** Failed ************\n");
            DbgPrint("Unexpected value returned by query.\n");
            DbgPrint("Status is: 0x%lx \n", Status);
            DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
            CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));


   //
   // Query with too little buffer
   // (This relies upon the ReturnLength returned from previous call)
   //

    DbgPrint("Se:     Query default Dacl with too small buffer ...           ");

    Status = NtQueryInformationToken(
                 TokenWithDefaultDacl,     // Handle
                 TokenDefaultDacl,         // TokenInformationClass
                 DefaultDacl,              // TokenInformation
                 ReturnLength-1,           // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));


   //
   // Query token with no default dacl
   //

    DbgPrint("Se:     Query default dacl from token with none ...            ");

    Status = NtQueryInformationToken(
                 SimpleToken,                // Handle
                 TokenDefaultDacl,           // TokenInformationClass
                 DefaultDacl,                // TokenInformation
                 sizeof(TOKEN_DEFAULT_DACL), // TokenInformationLength
                 &ReturnLength               // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));


////////////////////////////////////////////////////////////////////////
//                                                                    //
//        Query Token Source                                          //
//                                                                    //
////////////////////////////////////////////////////////////////////////

   //
   // Query Token Source with zero length buffer
   //

    DbgPrint("Se:     Query Token Source with zero length buffer ...         ");

    Status = NtQueryInformationToken(
                 TokenWithPrivileges,      // Handle
                 TokenSource,              // TokenInformationClass
                 &QueriedSource,           // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        if (ReturnLength == sizeof(TOKEN_SOURCE)) {
            DbgPrint("Succeeded.\n");
        } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        DbgPrint("TOKEN_SOURCE data size is 0x%lx \n", sizeof(TOKEN_SOURCE));
        CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));



    //
    // Query token source
    //

    DbgPrint("Se:     Query token source ...                                 ");

    Status = NtQueryInformationToken(
                 TokenWithPrivileges,      // Handle
                 TokenSource,              // TokenInformationClass
                 &QueriedSource,           // TokenInformation
                 sizeof(TOKEN_SOURCE),     // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (NT_SUCCESS(Status)) {

        //
        // Check returned value against TestSource
        //

        ValuesCompare = TRUE;

        if ( (QueriedSource.SourceName[0] != TestSource.SourceName[0]) ||
             (QueriedSource.SourceName[1] != TestSource.SourceName[1]) ||
             (QueriedSource.SourceName[2] != TestSource.SourceName[2]) ||
             (QueriedSource.SourceName[3] != TestSource.SourceName[3]) ||
             (QueriedSource.SourceName[4] != TestSource.SourceName[4]) ||
             (QueriedSource.SourceName[5] != TestSource.SourceName[5]) ||
             (QueriedSource.SourceName[6] != TestSource.SourceName[6]) ||
             (QueriedSource.SourceName[7] != TestSource.SourceName[7]) ) {

            ValuesCompare = FALSE;

        }

        if ( !RtlEqualLuid(&QueriedSource.SourceIdentifier,
               &TestSource.SourceIdentifier)   ) {

            ValuesCompare = FALSE;

        }


        if ( ValuesCompare ) {
            DbgPrint("Succeeded.\n");
        } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Unexpected value returned by query.\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));


    //
    // Query with too little buffer
    // (This relies upon the ReturnLength returned from previous call)
    //

    DbgPrint("Se:     Query token source with too small buffer ...           ");

    Status = NtQueryInformationToken(
                 TokenWithPrivileges,      // Handle
                 TokenSource,              // TokenInformationClass
                 &QueriedSource,           // TokenInformation
                 ReturnLength - 1,         // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));


////////////////////////////////////////////////////////////////////////
//                                                                    //
//        Query Token Type                                            //
//                                                                    //
////////////////////////////////////////////////////////////////////////

   //
   // Query Token type with zero length buffer
   //

    DbgPrint("Se:     Query Token type with zero length buffer ...           ");

    Status = NtQueryInformationToken(
                 TokenWithPrivileges,      // Handle
                 TokenType,                // TokenInformationClass
                 &QueriedType,             // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        if (ReturnLength == sizeof(TOKEN_TYPE)) {
            DbgPrint("Succeeded.\n");
        } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        DbgPrint("TOKEN_TYPE data size is 0x%lx \n", sizeof(TOKEN_TYPE));
        CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));




    //
    // Query token type
    //

    DbgPrint("Se:     Query token type ...                                   ");

    Status = NtQueryInformationToken(
                 TokenWithPrivileges,      // Handle
                 TokenType,                // TokenInformationClass
                 &QueriedType,             // TokenInformation
                 sizeof(TOKEN_TYPE),       // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (NT_SUCCESS(Status)) {

        //
        // Check returned value against TestSource
        //


        if ( QueriedType == TokenPrimary ) {
            DbgPrint("Succeeded.\n");
        } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Unexpected value returned by query.\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        DbgPrint("Returned token type is: 0x%lx \n", QueriedType);
        CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));


    //
    // Query with too little buffer
    // (This relies upon the ReturnLength returned from previous call)
    //

    DbgPrint("Se:     Query token type with too small buffer ...             ");

    Status = NtQueryInformationToken(
                 TokenWithPrivileges,      // Handle
                 TokenType,                // TokenInformationClass
                 &QueriedType,             // TokenInformation
                 ReturnLength - 1,         // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));


////////////////////////////////////////////////////////////////////////
//                                                                    //
//        Query Impersonation Level                                   //
//                                                                    //
////////////////////////////////////////////////////////////////////////

   //
   // Query Impersonation Level of primary token
   //

    DbgPrint("Se:     Query Impersonation level of primary token ...         ");

    Status = NtQueryInformationToken(
                 TokenWithPrivileges,                  // Handle
                 TokenImpersonationLevel,              // TokenInformationClass
                 &QueriedImpersonationLevel,           // TokenInformation
                 sizeof(SECURITY_IMPERSONATION_LEVEL), // TokenInformationLength
                 &ReturnLength                         // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_INVALID_INFO_CLASS) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(Status == STATUS_INVALID_INFO_CLASS);


////////////////////////////////////////////////////////////////////////
//                                                                    //
//        Query Token Statistics                                      //
//                                                                    //
////////////////////////////////////////////////////////////////////////

   //
   // Query Token statistics with zero length buffer
   //

    DbgPrint("Se:     Query Token statistics with zero length buffer ...     ");

    Status = NtQueryInformationToken(
                 TokenWithPrivileges,      // Handle
                 TokenStatistics,          // TokenInformationClass
                 &QueriedStatistics,       // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        if (ReturnLength == sizeof(TOKEN_STATISTICS)) {
            DbgPrint("Succeeded.\n");
        } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        DbgPrint("TOKEN_STATISTICS data size is 0x%lx \n", sizeof(TOKEN_STATISTICS));
        CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));




    //
    // Query token statistics
    //

    DbgPrint("Se:     Query token statistics ...                             ");

    Status = NtQueryInformationToken(
                 TokenWithPrivileges,      // Handle
                 TokenStatistics,          // TokenInformationClass
                 &QueriedStatistics,       // TokenInformation
                 sizeof(TOKEN_STATISTICS), // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (NT_SUCCESS(Status)) {

        //
        // Check returned value against TestSource
        //

        if ( ( QueriedStatistics.TokenType == TokenPrimary) &&
             ( QueriedStatistics.GroupCount == 4 )          &&
             ( QueriedStatistics.PrivilegeCount == PRIVILEGE_COUNT) ) {
            DbgPrint("Succeeded.\n");
        } else {
            DbgPrint("********** Failed ************\n");
            DbgPrint("Unexpected value returned by query.\n");
            DbgPrint("Status is: 0x%lx \n", Status);
            DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
            DbgPrint("Returned token type is: 0x%lx \n", QueriedStatistics.TokenType);
            DbgPrint("Returned group count is: 0x%lx \n", QueriedStatistics.GroupCount);
            DbgPrint("Returned privilege count is: 0x%lx \n", QueriedStatistics.PrivilegeCount);
            CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));


    //
    // Query with too little buffer
    // (This relies upon the ReturnLength returned from previous call)
    //

    DbgPrint("Se:     Query token statistics with too small buffer ...       ");

    Status = NtQueryInformationToken(
                 TokenWithPrivileges,      // Handle
                 TokenStatistics,          // TokenInformationClass
                 &QueriedStatistics,       // TokenInformation
                 ReturnLength - 1,         // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));



    return CompletionStatus;
}

////////////////////////////////////////////////////////////////
//                                                            //
// Set Token Test                                             //
//                                                            //
////////////////////////////////////////////////////////////////

BOOLEAN
TestTokenSet()
{
    BOOLEAN CompletionStatus = TRUE;
    ULONG InformationLength;
    ULONG ReturnLength;

    TOKEN_STATISTICS QueriedStatistics;

    TOKEN_PRIMARY_GROUP AssignedPrimaryGroup;
    PTOKEN_PRIMARY_GROUP QueriedPrimaryGroup = NULL;

    TOKEN_OWNER AssignedOwner;
    PTOKEN_OWNER QueriedOwner = NULL;

    TOKEN_DEFAULT_DACL AssignedDefaultDacl;
    PTOKEN_DEFAULT_DACL QueriedDefaultDacl = NULL;

    PSID TooBigSid;

    SID_IDENTIFIER_AUTHORITY BedrockAuthority = BEDROCK_AUTHORITY;

    DbgPrint("\n");


   //
   // Set owner of a token to be an invalid group
   //

    DbgPrint("Se:     Set default owner to be an invalid group ...           ");

    AssignedOwner.Owner = NeandertholSid;
    InformationLength = (ULONG)sizeof(TOKEN_OWNER);

    Status = NtSetInformationToken(
                 TokenWithGroups,          // Handle
                 TokenOwner,               // TokenInformationClass
                 &AssignedOwner,           // TokenInformation
                 InformationLength         // TokenInformationLength
                 );

    if (Status == STATUS_INVALID_OWNER) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("InformationLength is: 0x%lx \n", InformationLength);
        CompletionStatus = FALSE;
    }

    ASSERT(Status == STATUS_INVALID_OWNER);


   //
   // Set owner of a token to be an ID not in the token
   //

    DbgPrint("Se:     Set default owner to be an ID not in the token ...     ");

    AssignedOwner.Owner = BarneySid;
    InformationLength = (ULONG)sizeof(TOKEN_OWNER);

    Status = NtSetInformationToken(
                 TokenWithGroups,          // Handle
                 TokenOwner,               // TokenInformationClass
                 &AssignedOwner,           // TokenInformation
                 InformationLength         // TokenInformationLength
                 );

    if (Status == STATUS_INVALID_OWNER) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("InformationLength is: 0x%lx \n", InformationLength);
        CompletionStatus = FALSE;
    }

    ASSERT(Status == STATUS_INVALID_OWNER);


   //
   // Set owner of a token to be a valid group
   //

    DbgPrint("Se:     Set default owner to be a valid group ...              ");

    AssignedOwner.Owner = FlintstoneSid;
    InformationLength = (ULONG)sizeof(TOKEN_OWNER);

    Status = NtSetInformationToken(
                 TokenWithGroups,          // Handle
                 TokenOwner,               // TokenInformationClass
                 &AssignedOwner,           // TokenInformation
                 InformationLength         // TokenInformationLength
                 );

    if (!NT_SUCCESS(Status)) {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("InformationLength is: 0x%lx \n", InformationLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));

    //
    // Query the Owner to see that it was set properly
    //

    Status = NtQueryInformationToken(
                 TokenWithGroups,          // Handle
                 TokenOwner,               // TokenInformationClass
                 QueriedOwner,             // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status != STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("********** Failed Query of length ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));

    QueriedOwner = (PTOKEN_OWNER)TstAllocatePool( PagedPool,
                                                  ReturnLength
                                                  );

    Status = NtQueryInformationToken(
                 TokenWithGroups,          // Handle
                 TokenOwner,               // TokenInformationClass
                 QueriedOwner,             // TokenInformation
                 ReturnLength,             // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (NT_SUCCESS(Status)) {

        //
        // Check returned value
        //

        if (RtlEqualSid((QueriedOwner->Owner), AssignedOwner.Owner) ) {
            DbgPrint("Succeeded.\n");
        } else {
        DbgPrint("********** Failed Comparison ************\n");
        DbgPrint("Unexpected value returned by query.\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed Query Of Value ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));


    //
    //  Set Default Dacl

    //
    // Get a buffer for use in all Default Dacl assignment tests.
    // This will be initialized to different sizes for each test.
    //

    AssignedDefaultDacl.DefaultDacl =
        (PACL)TstAllocatePool( PagedPool, TOO_BIG_ACL_SIZE );


    //
    // Assign a discretionary ACL to a token that doesn't yet have one
    //

    DbgPrint("Se:     Set original discretionary ACL in token ...            ");

    InformationLength = (ULONG)sizeof(TOKEN_DEFAULT_DACL);
    RtlCreateAcl( AssignedDefaultDacl.DefaultDacl, 200, ACL_REVISION );

    Status = NtQueryInformationToken(
                 TokenWithGroups,            // Handle
                 TokenDefaultDacl,           // TokenInformationClass
                 &QueriedDefaultDacl,        // TokenInformation
                 0,                          // TokenInformationLength
                 &ReturnLength               // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(Status == STATUS_BUFFER_TOO_SMALL);

    if (ReturnLength != sizeof(TOKEN_DEFAULT_DACL)) {

        //
        // Wait a minute, this token has a default Dacl
        //

            DbgPrint("******** Failed - token has default dacl *********\n");
            DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
            CompletionStatus = FALSE;

    } else {

        Status = NtSetInformationToken(
                     TokenWithGroups,          // Handle
                     TokenDefaultDacl,         // TokenInformationClass
                     &AssignedDefaultDacl,     // TokenInformation
                     InformationLength         // TokenInformationLength
                     );

        if (NT_SUCCESS(Status)) {
            DbgPrint("Succeeded.\n");
        } else {
            DbgPrint("********** Failed ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);
            CompletionStatus = FALSE;
        }

    }

    ASSERT(NT_SUCCESS(Status));

    //
    // Replace a discretionary ACL in a token that already has one
    // Make it big to help with future "too big" tests...
    //


    //
    // find out how much space is available
    //

    Status = NtQueryInformationToken(
                 TokenWithGroups,                 // Handle
                 TokenStatistics,                 // TokenInformationClass
                 &QueriedStatistics,              // TokenInformation
                 (ULONG)sizeof(TOKEN_STATISTICS), // TokenInformationLength
                 &ReturnLength                    // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(Status));

    Status = NtQueryInformationToken(
                 TokenWithGroups,          // Handle
                 TokenDefaultDacl,         // TokenInformationClass
                 &QueriedDefaultDacl,      // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(Status == STATUS_BUFFER_TOO_SMALL);


    if (ReturnLength > sizeof(TOKEN_STATISTICS)) {
        CurrentLength = ReturnLength - (ULONG)sizeof(TOKEN_STATISTICS);
    } else {
        CurrentLength = 0;
    }

    LengthAvailable = QueriedStatistics.DynamicAvailable + CurrentLength;

    DbgPrint("Se:     Replace discretionary ACL in token ...                 ");

    InformationLength = (ULONG)sizeof(TOKEN_DEFAULT_DACL);
    RtlCreateAcl( AssignedDefaultDacl.DefaultDacl,
                  (ULONG)(LengthAvailable - 50),
                  ACL_REVISION
                  );

    Status = NtQueryInformationToken(
                 TokenWithGroups,          // Handle
                 TokenDefaultDacl,         // TokenInformationClass
                 &QueriedDefaultDacl,      // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG


    if (!(ReturnLength > sizeof(TOKEN_DEFAULT_DACL))) {

        //
        // Wait a minute, this token doesn't have a default Dacl
        //

            DbgPrint("******** Failed - No default dacl *********\n");
            CompletionStatus = FALSE;

    } else {

        Status = NtSetInformationToken(
                     TokenWithGroups,          // Handle
                     TokenDefaultDacl,         // TokenInformationClass
                     &AssignedDefaultDacl,     // TokenInformation
                     InformationLength         // TokenInformationLength
                     );

        if (NT_SUCCESS(Status)) {
            DbgPrint("Succeeded.\n");
        } else {
            DbgPrint("********** Failed ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);
            CompletionStatus = FALSE;
        }

    }

    ASSERT(NT_SUCCESS(Status));


    //
    // Assign a discretionary ACL that doesn't fit into the dynamic part of the
    // token.
    //


    //
    // find out how much space is available
    //

    Status = NtQueryInformationToken(
                 TokenWithGroups,                 // Handle
                 TokenStatistics,                 // TokenInformationClass
                 &QueriedStatistics,              // TokenInformation
                 (ULONG)sizeof(TOKEN_STATISTICS), // TokenInformationLength
                 &ReturnLength                    // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(Status));

    Status = NtQueryInformationToken(
                 TokenWithGroups,          // Handle
                 TokenDefaultDacl,         // TokenInformationClass
                 &QueriedDefaultDacl,      // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(Status == STATUS_BUFFER_TOO_SMALL);


    if (ReturnLength > sizeof(TOKEN_STATISTICS)) {
        CurrentLength = ReturnLength - (ULONG)sizeof(TOKEN_STATISTICS);
    } else {
        CurrentLength = 0;
    }

    LengthAvailable = QueriedStatistics.DynamicAvailable + CurrentLength;

    DbgPrint("Se:     Set too big discretionary ACL ...                      ");


    //
    // Now make sure our ACL is large enough to exceed the available
    // space.
    //

    RtlCreateAcl( AssignedDefaultDacl.DefaultDacl,
                  TOO_BIG_ACL_SIZE,
                  ACL_REVISION
                  );

    if (TOO_BIG_ACL_SIZE < LengthAvailable) {

        DbgPrint("********** Failed - Dynamic too big ************\n");
        DbgPrint("Dynamic available is: 0x%lx \n",
            QueriedStatistics.DynamicAvailable);
        DbgPrint("Current default Dacl size is: 0x%lx \n", CurrentLength);
        DbgPrint("Big ACL size is: 0x%lx \n", TOO_BIG_ACL_SIZE);
        CompletionStatus = FALSE;
    }


    InformationLength = (ULONG)sizeof(TOKEN_DEFAULT_DACL);

    Status = NtSetInformationToken(
                 TokenWithGroups,          // Handle
                 TokenDefaultDacl,         // TokenInformationClass
                 &AssignedDefaultDacl,     // TokenInformation
                 InformationLength         // TokenInformationLength
                 );

    if (Status == STATUS_ALLOTTED_SPACE_EXCEEDED) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Dynamic available is: 0x%lx \n",
            QueriedStatistics.DynamicAvailable);
        DbgPrint("Current default Dacl size is: 0x%lx \n", CurrentLength);
        DbgPrint("Big ACL size is: 0x%lx \n", TOO_BIG_ACL_SIZE);
        CompletionStatus = FALSE;
    }

    ASSERT(Status == STATUS_ALLOTTED_SPACE_EXCEEDED);


   //
   // Set primary group
   //

    DbgPrint("Se:     Set primary group ...                                  ");

    AssignedPrimaryGroup.PrimaryGroup = RubbleSid;
    InformationLength = (ULONG)sizeof(TOKEN_PRIMARY_GROUP);

    Status = NtSetInformationToken(
                 TokenWithGroups,          // Handle
                 TokenPrimaryGroup,        // TokenInformationClass
                 &AssignedPrimaryGroup,    // TokenInformation
                 InformationLength         // TokenInformationLength
                 );

    if (!NT_SUCCESS(Status)) {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("InformationLength is: 0x%lx \n", InformationLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));


    //
    // Query the Primary Group to see that it was set properly
    //

    Status = NtQueryInformationToken(
                 TokenWithGroups,          // Handle
                 TokenPrimaryGroup,        // TokenInformationClass
                 QueriedPrimaryGroup,      // TokenInformation
                 0,                        // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status != STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("********** Failed Query of length ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(!NT_SUCCESS(Status));

    QueriedPrimaryGroup =
        (PTOKEN_PRIMARY_GROUP)TstAllocatePool( PagedPool,
                                               ReturnLength
                                               );

    Status = NtQueryInformationToken(
                 TokenWithGroups,          // Handle
                 TokenPrimaryGroup,        // TokenInformationClass
                 QueriedPrimaryGroup,      // TokenInformation
                 ReturnLength,             // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (NT_SUCCESS(Status)) {

        //
        // Check returned value
        //

        if (RtlEqualSid((QueriedPrimaryGroup->PrimaryGroup),
            AssignedPrimaryGroup.PrimaryGroup) ) {
            DbgPrint("Succeeded.\n");
        } else {
        DbgPrint("********** Failed Comparison ************\n");
        DbgPrint("Unexpected value returned by query.\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
        }
    } else {
        DbgPrint("********** Failed Query Of Value ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Required return length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));


    //
    // Assign a primary group that doesn't fit into the dynamic part of the
    // token.
    //


    DbgPrint("Se:     Set too big primary group ...                          ");

    //
    // First, find out how much space is available
    //

    Status = NtQueryInformationToken(
                 TokenWithGroups,                 // Handle
                 TokenStatistics,                 // TokenInformationClass
                 &QueriedStatistics,              // TokenInformation
                 (ULONG)sizeof(TOKEN_STATISTICS), // TokenInformationLength
                 &ReturnLength                    // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(Status));

    Status = NtQueryInformationToken(
                 TokenWithGroups,          // Handle
                 TokenPrimaryGroup,        // TokenInformationClass
                 QueriedPrimaryGroup,      // TokenInformation
                 ReturnLength,             // TokenInformationLength
                 &ReturnLength             // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(Status));

    CurrentLength = SeLengthSid(QueriedPrimaryGroup->PrimaryGroup);
    LengthAvailable = QueriedStatistics.DynamicAvailable + CurrentLength;

    //
    // Now make sure our fake group ID is large enough to exceed the available
    // space.
    //

    TooBigSid = (PSID)TstAllocatePool(
                          PagedPool,
                          RtlLengthRequiredSid( TOO_BIG_PRIMARY_GROUP_SIZE )
                          );

    RtlInitializeSid(
        TooBigSid,
        &BedrockAuthority,
        TOO_BIG_PRIMARY_GROUP_SIZE
        );

    if ((ULONG) SeLengthSid(TooBigSid) < LengthAvailable) {

        DbgPrint("********** Failed - Dynamic too big ************\n");
        DbgPrint("Dynamic available is: 0x%lx \n",
            QueriedStatistics.DynamicAvailable);
        DbgPrint("Existing primary group length is: 0x%lx \n", CurrentLength);
        DbgPrint("Big SID size is: 0x%lx \n", SeLengthSid(TooBigSid));
        CompletionStatus = FALSE;
    }


    AssignedPrimaryGroup.PrimaryGroup = TooBigSid;
    InformationLength = (ULONG)sizeof(TOKEN_PRIMARY_GROUP);

    Status = NtSetInformationToken(
                 TokenWithGroups,          // Handle
                 TokenPrimaryGroup,        // TokenInformationClass
                 &AssignedPrimaryGroup,    // TokenInformation
                 InformationLength         // TokenInformationLength
                 );

    if (Status == STATUS_ALLOTTED_SPACE_EXCEEDED) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Dynamic available is: 0x%lx \n",
            QueriedStatistics.DynamicAvailable);
        DbgPrint("Existing primary group length is: 0x%lx \n", CurrentLength);
        DbgPrint("Big SID size is: 0x%lx \n", SeLengthSid(TooBigSid));
        CompletionStatus = FALSE;
    }




    return CompletionStatus;
}

////////////////////////////////////////////////////////////////
//                                                            //
// Adjust Privileges Test                                     //
//                                                            //
////////////////////////////////////////////////////////////////

BOOLEAN
TestTokenAdjustPrivileges()
{

    BOOLEAN CompletionStatus = TRUE;
    NTSTATUS Status;
    NTSTATUS IgnoreStatus;

    PTOKEN_PRIVILEGES NewState;
    PTOKEN_PRIVILEGES PreviousState;
    PTOKEN_PRIVILEGES PrePrivileges;
    PTOKEN_PRIVILEGES PostPrivileges;

    ULONG NewStateBufferLength = 200;
    ULONG PreviousStateBufferLength = 200;
    ULONG PrePrivilegesLength = 200;
    ULONG PostPrivilegesLength = 200;

    ULONG ReturnLength;
    ULONG IgnoreReturnLength;

    DbgPrint("\n");

    PreviousState = (PTOKEN_PRIVILEGES)TstAllocatePool(
                        PagedPool,
                        PreviousStateBufferLength
                        );

    PrePrivileges = (PTOKEN_PRIVILEGES)TstAllocatePool(
                        PagedPool,
                        PrePrivilegesLength
                        );

    PostPrivileges = (PTOKEN_PRIVILEGES)TstAllocatePool(
                        PagedPool,
                        PostPrivilegesLength
                        );

    NewState = (PTOKEN_PRIVILEGES)TstAllocatePool(
                   PagedPool,
                   NewStateBufferLength
                   );





    //////////////////////////////////////////////////////////////////////
    //                                                                  //
    // Adjust privileges giving no instructions                         //
    //                                                                  //
    //////////////////////////////////////////////////////////////////////


    DbgPrint("Se:     Adjust privileges with no instructions ...             ");

    Status = NtAdjustPrivilegesToken(
                 SimpleToken,                      // TokenHandle
                 FALSE,                            // DisableAllPrivileges
                 NULL,                             // NewState (OPTIONAL)
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );

    if (Status == STATUS_INVALID_PARAMETER) {

        DbgPrint("Succeeded. \n");

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_INVALID_PARAMETER);


    //////////////////////////////////////////////////////////////////////
    //                                                                  //
    // Enable privileges in token with no privileges                    //
    //                                                                  //
    //////////////////////////////////////////////////////////////////////


    NewState->PrivilegeCount = 1;
    NewState->Privileges[0].Luid = SecurityPrivilege;
    NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    DbgPrint("Se:     Enable privilege in token with none ...                ");

    Status = NtAdjustPrivilegesToken(
                 SimpleToken,                      // TokenHandle
                 FALSE,                            // DisableAllPrivileges
                 NewState,                         // NewState (OPTIONAL)
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );

    if (Status == STATUS_NOT_ALL_ASSIGNED) {

        DbgPrint("Succeeded. \n");

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_NOT_ALL_ASSIGNED);


    //////////////////////////////////////////////////////////////////////
    //                                                                  //
    //  Enable a privilege that isn't assigned                          //
    //                                                                  //
    //////////////////////////////////////////////////////////////////////

    NewState->PrivilegeCount = 1;
    NewState->Privileges[0].Luid = CreateTokenPrivilege;
    NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    DbgPrint("Se:     Enable unassigned privilege in token with some ...     ");

    PrePrivileges->PrivilegeCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithPrivileges,        // TokenHandle
                       TokenPrivileges,            // TokenInformationClass
                       PrePrivileges,              // TokenInformation
                       PrePrivilegesLength,        // TokenInformationLength
                       &IgnoreReturnLength         // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT( PrePrivileges->PrivilegeCount == PRIVILEGE_COUNT );
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    Status = NtAdjustPrivilegesToken(
                 TokenWithPrivileges,              // TokenHandle
                 FALSE,                            // DisableAllPrivileges
                 NewState,                         // NewState (OPTIONAL)
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );

    PostPrivileges->PrivilegeCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithPrivileges,        // TokenHandle
                       TokenPrivileges,            // TokenInformationClass
                       PostPrivileges,             // TokenInformation
                       PostPrivilegesLength,       // TokenInformationLength
                       &IgnoreReturnLength         // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT( PostPrivileges->PrivilegeCount == PRIVILEGE_COUNT );
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_NOT_ALL_ASSIGNED) {

         //
         // Check the privilege values
         //

         if ( (PrePrivileges->Privileges[0].Attributes ==
               PostPrivileges->Privileges[0].Attributes)    &&
              (PrePrivileges->Privileges[1].Attributes ==
               PostPrivileges->Privileges[1].Attributes)    ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);
            DbgPrint("Before and after privilege 0 state: 0x%lx,  0x%lx \n",
                    PrePrivileges->Privileges[0].Attributes,
                    PostPrivileges->Privileges[0].Attributes);
            DbgPrint("Before and after privilege 1 state: 0x%lx,  0x%lx \n",
                    PrePrivileges->Privileges[1].Attributes,
                    PostPrivileges->Privileges[1].Attributes);
            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_NOT_ALL_ASSIGNED);




    //////////////////////////////////////////////////////////////////////
    //                                                                  //
    // Disable All Privileges (which they already are)                  //
    //                                                                  //
    //////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Disable already disabled privileges ...                ");

    PrePrivileges->PrivilegeCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithPrivileges,        // TokenHandle
                       TokenPrivileges,            // TokenInformationClass
                       PrePrivileges,              // TokenInformation
                       PrePrivilegesLength,        // TokenInformationLength
                       &IgnoreReturnLength         // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT( PrePrivileges->PrivilegeCount == PRIVILEGE_COUNT );
    ASSERT( PrePrivileges->Privileges[0].Attributes == 0 );
    ASSERT( PrePrivileges->Privileges[1].Attributes == 0 );
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    Status = NtAdjustPrivilegesToken(
                 TokenWithPrivileges,              // TokenHandle
                 TRUE,                             // DisableAllPrivileges
                 NULL,                             // NewState (OPTIONAL)
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );


    PostPrivileges->PrivilegeCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithPrivileges,        // TokenHandle
                       TokenPrivileges,            // TokenInformationClass
                       PostPrivileges,             // TokenInformation
                       PostPrivilegesLength,       // TokenInformationLength
                       &IgnoreReturnLength         // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT( PostPrivileges->PrivilegeCount == PRIVILEGE_COUNT );
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_SUCCESS) {

         //
         // Check the privilege values
         //

         if ( (PostPrivileges->Privileges[0].Attributes == 0) &&
              (PostPrivileges->Privileges[1].Attributes == 0)    ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);
            DbgPrint("Before and after privilege 0 state: 0x%lx,  0x%lx \n",
                    PrePrivileges->Privileges[0].Attributes,
                    PostPrivileges->Privileges[0].Attributes);
            DbgPrint("Before and after privilege 1 state: 0x%lx,  0x%lx \n",
                    PrePrivileges->Privileges[1].Attributes,
                    PostPrivileges->Privileges[1].Attributes);
            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_SUCCESS);



    //////////////////////////////////////////////////////////////////////
    //                                                                  //
    // Enable currently disabled privileges                             //
    //                                                                  //
    //////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Enable currently disabled privileges ...               ");

    PrePrivileges->PrivilegeCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithPrivileges,        // TokenHandle
                       TokenPrivileges,            // TokenInformationClass
                       PrePrivileges,              // TokenInformation
                       PrePrivilegesLength,        // TokenInformationLength
                       &IgnoreReturnLength         // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT( PrePrivileges->PrivilegeCount == PRIVILEGE_COUNT );
    ASSERT( PrePrivileges->Privileges[0].Attributes == 0 );
    ASSERT( PrePrivileges->Privileges[1].Attributes == 0 );
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    NewState->PrivilegeCount = 2;
    NewState->Privileges[0].Luid = SecurityPrivilege;
    NewState->Privileges[1].Luid = UnsolicitedInputPrivilege;
    NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    NewState->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

    Status = NtAdjustPrivilegesToken(
                 TokenWithPrivileges,              // TokenHandle
                 FALSE,                            // DisableAllPrivileges
                 NewState,                         // NewState (OPTIONAL)
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );

    PostPrivileges->PrivilegeCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithPrivileges,        // TokenHandle
                       TokenPrivileges,            // TokenInformationClass
                       PostPrivileges,             // TokenInformation
                       PostPrivilegesLength,       // TokenInformationLength
                       &IgnoreReturnLength         // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT( PostPrivileges->PrivilegeCount == PRIVILEGE_COUNT );
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_SUCCESS) {

         //
         // Check the privilege values
         //

         if ( (PostPrivileges->Privileges[0].Attributes == SE_PRIVILEGE_ENABLED)    &&
              (PostPrivileges->Privileges[1].Attributes == SE_PRIVILEGE_ENABLED)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);
            DbgPrint("Before and after privilege 0 state: 0x%lx,  0x%lx \n",
                    PrePrivileges->Privileges[0].Attributes,
                    PostPrivileges->Privileges[0].Attributes);
            DbgPrint("Before and after privilege 1 state: 0x%lx,  0x%lx \n",
                    PrePrivileges->Privileges[1].Attributes,
                    PostPrivileges->Privileges[1].Attributes);
            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_SUCCESS);



    //////////////////////////////////////////////////////////////////////
    //                                                                  //
    // Disable all enabled privileges                                   //
    //                                                                  //
    //////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Disable all enabled privileges ...                     ");

    PrePrivileges->PrivilegeCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithPrivileges,        // TokenHandle
                       TokenPrivileges,            // TokenInformationClass
                       PrePrivileges,              // TokenInformation
                       PrePrivilegesLength,        // TokenInformationLength
                       &IgnoreReturnLength         // ReturnLength
                       );

    ASSERT( PrePrivileges->PrivilegeCount == PRIVILEGE_COUNT );
    ASSERT( PrePrivileges->Privileges[0].Attributes == SE_PRIVILEGE_ENABLED );
    ASSERT( PrePrivileges->Privileges[1].Attributes == SE_PRIVILEGE_ENABLED );
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    Status = NtAdjustPrivilegesToken(
                 TokenWithPrivileges,              // TokenHandle
                 TRUE,                             // DisableAllPrivileges
                 NULL,                             // NewState (OPTIONAL)
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );


    PostPrivileges->PrivilegeCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithPrivileges,        // TokenHandle
                       TokenPrivileges,            // TokenInformationClass
                       PostPrivileges,             // TokenInformation
                       PostPrivilegesLength,       // TokenInformationLength
                       &IgnoreReturnLength         // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT( PostPrivileges->PrivilegeCount == PRIVILEGE_COUNT );
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_SUCCESS) {

         //
         // Check the privilege values
         //

         if ( (PostPrivileges->Privileges[0].Attributes == 0)    &&
              (PostPrivileges->Privileges[1].Attributes == 0)    ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);
            DbgPrint("Before and after privilege 0 state: 0x%lx,  0x%lx \n",
                    PrePrivileges->Privileges[0].Attributes,
                    PostPrivileges->Privileges[0].Attributes);
            DbgPrint("Before and after privilege 1 state: 0x%lx,  0x%lx \n",
                    PrePrivileges->Privileges[1].Attributes,
                    PostPrivileges->Privileges[1].Attributes);
            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_SUCCESS);



    //////////////////////////////////////////////////////////////////////
    //                                                                  //
    // Enable privileges requesting previous state with no return       //
    // length buffer                                                    //
    //                                                                  //
    //////////////////////////////////////////////////////////////////////


    DbgPrint("Se:     PreviousState not NULL, ReturnLength NULL...           ");

    NewState->PrivilegeCount = 2;
    NewState->Privileges[0].Luid = SecurityPrivilege;
    NewState->Privileges[1].Luid = UnsolicitedInputPrivilege;
    NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    NewState->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

    Status = NtAdjustPrivilegesToken(
                 TokenWithPrivileges,              // TokenHandle
                 FALSE,                            // DisableAllPrivileges
                 NewState,                         // NewState (OPTIONAL)
                 0,                                // BufferLength
                 PreviousState,                    // PreviousState (OPTIONAL)
                 NULL                              // ReturnLength
                 );

    if (Status == STATUS_ACCESS_VIOLATION) {

        DbgPrint("Succeeded. \n");

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_ACCESS_VIOLATION);




    //////////////////////////////////////////////////////////////////////
    //                                                                  //
    // Enable privileges without requesting previous state and          //
    // providing no return length buffer                                //
    //                                                                  //
    //////////////////////////////////////////////////////////////////////


    DbgPrint("Se:     PreviousState and ReturnLength both NULL...            ");

    NewState->PrivilegeCount = 2;
    NewState->Privileges[0].Luid = SecurityPrivilege;
    NewState->Privileges[1].Luid = UnsolicitedInputPrivilege;
    NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    NewState->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

    Status = NtAdjustPrivilegesToken(
                 TokenWithPrivileges,              // TokenHandle
                 FALSE,                            // DisableAllPrivileges
                 NewState,                         // NewState (OPTIONAL)
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 NULL                              // ReturnLength
                 );

    if (Status == STATUS_SUCCESS) {

        DbgPrint("Succeeded. \n");

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_SUCCESS);






    //////////////////////////////////////////////////////////////////////
    //                                                                  //
    // Enable privileges requesting previous state with insufficient    //
    // buffer                                                           //
    //                                                                  //
    //////////////////////////////////////////////////////////////////////


    DbgPrint("Se:     Too small buffer for previous state ...                ");

    //
    // Establish a known previous state first...
    //

    Status = NtAdjustPrivilegesToken(
                 TokenWithPrivileges,              // TokenHandle
                 TRUE,                             // DisableAllPrivileges
                 NULL,                             // NewState (OPTIONAL)
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );

    NewState->PrivilegeCount = 2;
    NewState->Privileges[0].Luid = SecurityPrivilege;
    NewState->Privileges[1].Luid = UnsolicitedInputPrivilege;
    NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    NewState->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

    Status = NtAdjustPrivilegesToken(
                 TokenWithPrivileges,              // TokenHandle
                 FALSE,                            // DisableAllPrivileges
                 NewState,                         // NewState (OPTIONAL)
                 0,                                // BufferLength
                 PreviousState,                    // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    if (Status == STATUS_BUFFER_TOO_SMALL) {

        DbgPrint("Succeeded. \n");

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_BUFFER_TOO_SMALL);





    //////////////////////////////////////////////////////////////////////
    //                                                                  //
    // Enable one of the privileges requesting previous state           //
    //                                                                  //
    //////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Enable one requesting previous state ...               ");

    PrePrivileges->PrivilegeCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithPrivileges,        // TokenHandle
                       TokenPrivileges,            // TokenInformationClass
                       PrePrivileges,              // TokenInformation
                       PrePrivilegesLength,        // TokenInformationLength
                       &IgnoreReturnLength         // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT( PrePrivileges->PrivilegeCount == PRIVILEGE_COUNT );
    ASSERT( PrePrivileges->Privileges[0].Attributes == 0 );
    ASSERT( PrePrivileges->Privileges[1].Attributes == 0 );
    ASSERT(NT_SUCCESS(IgnoreStatus) );


    NewState->PrivilegeCount = 1;
    NewState->Privileges[0].Luid = SecurityPrivilege;
    NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    Status = NtAdjustPrivilegesToken(
                 TokenWithPrivileges,              // TokenHandle
                 FALSE,                            // DisableAllPrivileges
                 NewState,                         // NewState (OPTIONAL)
                 PreviousStateBufferLength,        // BufferLength
                 PreviousState,                    // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(Status));
    ASSERT(PreviousState->PrivilegeCount == 1);


    PostPrivileges->PrivilegeCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithPrivileges,        // TokenHandle
                       TokenPrivileges,            // TokenInformationClass
                       PostPrivileges,             // TokenInformation
                       PostPrivilegesLength,       // TokenInformationLength
                       &IgnoreReturnLength         // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT( PostPrivileges->PrivilegeCount == PRIVILEGE_COUNT );
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_SUCCESS) {

         //
         // Check the privilege values
         //

         if ( (PostPrivileges->Privileges[SECURITY_INDEX].Attributes ==
               SE_PRIVILEGE_ENABLED)    &&
              (PostPrivileges->Privileges[UNSOLICITED_INDEX].Attributes == 0)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);
            DbgPrint("Before and after privilege 0 state: 0x%lx,  0x%lx \n",
                    PrePrivileges->Privileges[0].Attributes,
                    PostPrivileges->Privileges[0].Attributes);
            DbgPrint("Before and after privilege 1 state: 0x%lx,  0x%lx \n",
                    PrePrivileges->Privileges[1].Attributes,
                    PostPrivileges->Privileges[1].Attributes);
            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        DbgPrint("Change Count is: 0x%lx \n", PreviousState->PrivilegeCount);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_SUCCESS);




    //////////////////////////////////////////////////////////////////////
    //                                                                  //
    // Enable the other privilege requesting previous state             //
    //                                                                  //
    //////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Enable one requesting previous state ...               ");

    PrePrivileges->PrivilegeCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithPrivileges,        // TokenHandle
                       TokenPrivileges,            // TokenInformationClass
                       PrePrivileges,              // TokenInformation
                       PrePrivilegesLength,        // TokenInformationLength
                       &IgnoreReturnLength         // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT( PrePrivileges->PrivilegeCount == PRIVILEGE_COUNT );
    ASSERT( PrePrivileges->Privileges[SECURITY_INDEX].Attributes ==
            SE_PRIVILEGE_ENABLED );
    ASSERT( PrePrivileges->Privileges[UNSOLICITED_INDEX].Attributes == 0 );
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    NewState->PrivilegeCount = 1;
    NewState->Privileges[0].Luid = UnsolicitedInputPrivilege;
    NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    Status = NtAdjustPrivilegesToken(
                 TokenWithPrivileges,              // TokenHandle
                 FALSE,                            // DisableAllPrivileges
                 NewState,                         // NewState (OPTIONAL)
                 PreviousStateBufferLength,        // BufferLength
                 PreviousState,                    // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(Status));
    ASSERT(PreviousState->PrivilegeCount == 1);


    PostPrivileges->PrivilegeCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithPrivileges,        // TokenHandle
                       TokenPrivileges,            // TokenInformationClass
                       PostPrivileges,             // TokenInformation
                       PostPrivilegesLength,       // TokenInformationLength
                       &IgnoreReturnLength         // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT( PostPrivileges->PrivilegeCount == PRIVILEGE_COUNT );
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_SUCCESS) {

         //
         // Check the privilege values
         //

         if ( (PostPrivileges->Privileges[0].Attributes == SE_PRIVILEGE_ENABLED)    &&
              (PostPrivileges->Privileges[1].Attributes == SE_PRIVILEGE_ENABLED)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);
            DbgPrint("Before and after privilege 0 state: 0x%lx,  0x%lx \n",
                    PrePrivileges->Privileges[0].Attributes,
                    PostPrivileges->Privileges[0].Attributes);
            DbgPrint("Before and after privilege 1 state: 0x%lx,  0x%lx \n",
                    PrePrivileges->Privileges[1].Attributes,
                    PostPrivileges->Privileges[1].Attributes);
            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        DbgPrint("Change Count is: 0x%lx \n", PreviousState->PrivilegeCount);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_SUCCESS);





    //////////////////////////////////////////////////////////////////////
    //                                                                  //
    // Return privileges to their previous state                        //
    // Uses PreviousState from previous call                            //
    //                                                                  //
    //////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Return privileges to previous state ...                ");

    PrePrivileges->PrivilegeCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithPrivileges,        // TokenHandle
                       TokenPrivileges,            // TokenInformationClass
                       PrePrivileges,              // TokenInformation
                       PrePrivilegesLength,        // TokenInformationLength
                       &IgnoreReturnLength         // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT( PrePrivileges->PrivilegeCount == PRIVILEGE_COUNT );
    ASSERT( PrePrivileges->Privileges[0].Attributes == SE_PRIVILEGE_ENABLED );
    ASSERT( PrePrivileges->Privileges[1].Attributes == SE_PRIVILEGE_ENABLED );
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    Status = NtAdjustPrivilegesToken(
                 TokenWithPrivileges,              // TokenHandle
                 FALSE,                            // DisableAllPrivileges
                 PreviousState,                    // NewState (OPTIONAL)
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );

    ASSERT(NT_SUCCESS(Status));
    ASSERT(PreviousState->PrivilegeCount == 1);


    PostPrivileges->PrivilegeCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithPrivileges,        // TokenHandle
                       TokenPrivileges,            // TokenInformationClass
                       PostPrivileges,             // TokenInformation
                       PostPrivilegesLength,       // TokenInformationLength
                       &IgnoreReturnLength         // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT( PostPrivileges->PrivilegeCount == PRIVILEGE_COUNT );
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_SUCCESS) {

         //
         // Check the privilege values
         //

         if ( (PostPrivileges->Privileges[SECURITY_INDEX].Attributes ==
              SE_PRIVILEGE_ENABLED)    &&
              (PostPrivileges->Privileges[UNSOLICITED_INDEX].Attributes == 0)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);
            DbgPrint("Before and after privilege 0 state: 0x%lx,  0x%lx \n",
                    PrePrivileges->Privileges[0].Attributes,
                    PostPrivileges->Privileges[0].Attributes);
            DbgPrint("Before and after privilege 1 state: 0x%lx,  0x%lx \n",
                    PrePrivileges->Privileges[1].Attributes,
                    PostPrivileges->Privileges[1].Attributes);
            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }


    ASSERT(Status == STATUS_SUCCESS);




    ////////////////////////////////////////////////////////////////
    //                                                            //
    // Done with test                                             //
    //                                                            //
    ////////////////////////////////////////////////////////////////



    TstDeallocatePool( PreviousState, PreviousStateBufferLength );
    TstDeallocatePool( NewState, NewStateBufferLength );
    TstDeallocatePool( PrePrivileges, PrePrivilegesLength );
    TstDeallocatePool( PostPrivileges, PostPrivilegesLength );


    return CompletionStatus;
}

////////////////////////////////////////////////////////////////
//                                                            //
// Adjust Groups Test                                         //
//                                                            //
////////////////////////////////////////////////////////////////

BOOLEAN
TestTokenAdjustGroups()
{
    BOOLEAN CompletionStatus = TRUE;
    NTSTATUS Status;
    NTSTATUS IgnoreStatus;

    PTOKEN_GROUPS NewState;
    PTOKEN_GROUPS PreviousState;
    PTOKEN_GROUPS PreGroups;
    PTOKEN_GROUPS PostGroups;

    ULONG NewStateBufferLength = 600;
    ULONG PreviousStateBufferLength = 600;
    ULONG PreGroupsLength = 600;
    ULONG PostGroupsLength = 600;

    ULONG ReturnLength;
    ULONG IgnoreReturnLength;

    DbgPrint("\n");

    PreviousState = (PTOKEN_GROUPS)TstAllocatePool(
                        PagedPool,
                        PreviousStateBufferLength
                        );

    PreGroups = (PTOKEN_GROUPS)TstAllocatePool(
                        PagedPool,
                        PreGroupsLength
                        );

    PostGroups = (PTOKEN_GROUPS)TstAllocatePool(
                        PagedPool,
                        PostGroupsLength
                        );

    NewState = (PTOKEN_GROUPS)TstAllocatePool(
                   PagedPool,
                   NewStateBufferLength
                   );





    //////////////////////////////////////////////////////////////////////
    //                                                                  //
    // Adjust groups giving no instructions                             //
    //                                                                  //
    //////////////////////////////////////////////////////////////////////


    DbgPrint("Se:     Adjust groups with no instructions ...                 ");

    Status = NtAdjustGroupsToken(
                 SimpleToken,                      // TokenHandle
                 FALSE,                            // ResetToDefault
                 NULL,                             // NewState (OPTIONAL)
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );

    if (Status == STATUS_INVALID_PARAMETER) {

        DbgPrint("Succeeded. \n");

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_INVALID_PARAMETER);


/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  Disable unknown group                                              //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Disable unknown group ...                              ");

    PreGroups->GroupCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PreGroups,              // TokenInformation
                       PreGroupsLength,        // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    if (IgnoreStatus != STATUS_SUCCESS) {
        DbgPrint(" \n IgnoreStatus = 0x%lx \n", IgnoreStatus);
        DbgPrint(" \n IgnoreReturnLength = 0x%lx \n", IgnoreReturnLength);
    }

    ASSERT(PreGroups->GroupCount == GROUP_COUNT );
    ASSERT(PreGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes);
    ASSERT(PreGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes);
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    NewState->GroupCount = 1;
    NewState->Groups[0].Sid = RubbleSid;
    NewState->Groups[0].Attributes = DisabledGroupAttributes;

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 FALSE,                        // ResetToDefault
                 NewState,                     // NewState (OPTIONAL)
                 0,                            // BufferLength
                 NULL,                         // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );

    PostGroups->GroupCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PostGroups,             // TokenInformation
                       PostGroupsLength,       // TokenInformationLength
                       &IgnoreReturnLength         // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_NOT_ALL_ASSIGNED) {

         //
         // Check the group values
         //

         ASSERT( PostGroups->GroupCount == GROUP_COUNT );
         if ( (PostGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes)    &&
              (PostGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes) &&
              (PostGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes) &&
              (PostGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);

            DbgPrint("Before/after Flintstone state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[FLINTSTONE_INDEX].Attributes,
                    PostGroups->Groups[FLINTSTONE_INDEX].Attributes);

            DbgPrint("Before/after Child state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[CHILD_INDEX].Attributes,
                    PostGroups->Groups[CHILD_INDEX].Attributes);

            DbgPrint("Before/after Neanderthol state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[NEANDERTHOL_INDEX].Attributes,
                    PostGroups->Groups[NEANDERTHOL_INDEX].Attributes);

            DbgPrint("Before/after World state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[WORLD_INDEX].Attributes,
                    PostGroups->Groups[WORLD_INDEX].Attributes);

            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);

            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_NOT_ALL_ASSIGNED);



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  Enable unknown group                                               //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Enable unknown group ...                               ");

    PreGroups->GroupCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PreGroups,              // TokenInformation
                       PreGroupsLength,        // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(PreGroups->GroupCount == GROUP_COUNT );
    ASSERT(PreGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes);
    ASSERT(PreGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes);
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    NewState->GroupCount = 1;
    NewState->Groups[0].Sid = RubbleSid;
    NewState->Groups[0].Attributes = OptionalGroupAttributes;

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 FALSE,                        // ResetToDefault
                 NewState,                     // NewState (OPTIONAL)
                 0,                            // BufferLength
                 NULL,                         // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );

    PostGroups->GroupCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PostGroups,             // TokenInformation
                       PostGroupsLength,       // TokenInformationLength
                       &IgnoreReturnLength         // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_NOT_ALL_ASSIGNED) {

         //
         // Check the group values
         //

         ASSERT( PostGroups->GroupCount == GROUP_COUNT );
         if ( (PostGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes)    &&
              (PostGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes) &&
              (PostGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes) &&
              (PostGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);

            DbgPrint("Before/after Flintstone state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[FLINTSTONE_INDEX].Attributes,
                    PostGroups->Groups[FLINTSTONE_INDEX].Attributes);

            DbgPrint("Before/after Child state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[CHILD_INDEX].Attributes,
                    PostGroups->Groups[CHILD_INDEX].Attributes);

            DbgPrint("Before/after Neanderthol state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[NEANDERTHOL_INDEX].Attributes,
                    PostGroups->Groups[NEANDERTHOL_INDEX].Attributes);

            DbgPrint("Before/after World state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[WORLD_INDEX].Attributes,
                    PostGroups->Groups[WORLD_INDEX].Attributes);

            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);

            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_NOT_ALL_ASSIGNED);



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  Disable mandatory group                                            //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Disable mandatory group ...                            ");

    PreGroups->GroupCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PreGroups,              // TokenInformation
                       PreGroupsLength,        // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(PreGroups->GroupCount == GROUP_COUNT );
    ASSERT(PreGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes);
    ASSERT(PreGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes);
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    NewState->GroupCount = 1;
    NewState->Groups[0].Sid = WorldSid;
    NewState->Groups[0].Attributes = DisabledGroupAttributes;

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 FALSE,                        // ResetToDefault
                 NewState,                     // NewState (OPTIONAL)
                 0,                            // BufferLength
                 NULL,                         // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );

    PostGroups->GroupCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PostGroups,             // TokenInformation
                       PostGroupsLength,       // TokenInformationLength
                       &IgnoreReturnLength         // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_CANT_DISABLE_MANDATORY) {

         //
         // Check the group values
         //

         ASSERT( PostGroups->GroupCount == GROUP_COUNT );
         if ( (PostGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes)    &&
              (PostGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes) &&
              (PostGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes) &&
              (PostGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);

            DbgPrint("Before/after Flintstone state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[FLINTSTONE_INDEX].Attributes,
                    PostGroups->Groups[FLINTSTONE_INDEX].Attributes);

            DbgPrint("Before/after Child state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[CHILD_INDEX].Attributes,
                    PostGroups->Groups[CHILD_INDEX].Attributes);

            DbgPrint("Before/after Neanderthol state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[NEANDERTHOL_INDEX].Attributes,
                    PostGroups->Groups[NEANDERTHOL_INDEX].Attributes);

            DbgPrint("Before/after World state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[WORLD_INDEX].Attributes,
                    PostGroups->Groups[WORLD_INDEX].Attributes);

            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);

            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_CANT_DISABLE_MANDATORY);




/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  Enable mandatory group                                             //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Enable mandatory group ...                             ");

    PreGroups->GroupCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PreGroups,              // TokenInformation
                       PreGroupsLength,        // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(PreGroups->GroupCount == GROUP_COUNT );
    ASSERT(PreGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes);
    ASSERT(PreGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes);
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    NewState->GroupCount = 1;
    NewState->Groups[0].Sid = WorldSid;
    NewState->Groups[0].Attributes = OptionalGroupAttributes;

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 FALSE,                        // ResetToDefault
                 NewState,                     // NewState (OPTIONAL)
                 0,                            // BufferLength
                 NULL,                         // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );

    PostGroups->GroupCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PostGroups,             // TokenInformation
                       PostGroupsLength,       // TokenInformationLength
                       &IgnoreReturnLength         // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_SUCCESS) {

         //
         // Check the group values
         //

         ASSERT( PostGroups->GroupCount == GROUP_COUNT );
         if ( (PostGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes)    &&
              (PostGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes) &&
              (PostGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes) &&
              (PostGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);

            DbgPrint("Before/after Flintstone state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[FLINTSTONE_INDEX].Attributes,
                    PostGroups->Groups[FLINTSTONE_INDEX].Attributes);

            DbgPrint("Before/after Child state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[CHILD_INDEX].Attributes,
                    PostGroups->Groups[CHILD_INDEX].Attributes);

            DbgPrint("Before/after Neanderthol state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[NEANDERTHOL_INDEX].Attributes,
                    PostGroups->Groups[NEANDERTHOL_INDEX].Attributes);

            DbgPrint("Before/after World state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[WORLD_INDEX].Attributes,
                    PostGroups->Groups[WORLD_INDEX].Attributes);

            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);

            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_SUCCESS);



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  Disable optional group                                             //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Disable optional group ...                             ");

    PreGroups->GroupCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PreGroups,              // TokenInformation
                       PreGroupsLength,        // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(PreGroups->GroupCount == GROUP_COUNT );
    ASSERT(PreGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes);
    ASSERT(PreGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes);
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    NewState->GroupCount = 1;
    NewState->Groups[0].Sid = ChildSid;
    NewState->Groups[0].Attributes = DisabledGroupAttributes;

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 FALSE,                        // ResetToDefault
                 NewState,                     // NewState (OPTIONAL)
                 0,                            // BufferLength
                 NULL,                         // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );

    PostGroups->GroupCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PostGroups,             // TokenInformation
                       PostGroupsLength,       // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_SUCCESS) {

         //
         // Check the group values
         //

         ASSERT( PostGroups->GroupCount == GROUP_COUNT );
         if ( (PostGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes)    &&
              (PostGroups->Groups[CHILD_INDEX].Attributes       == DisabledGroupAttributes) &&
              (PostGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes) &&
              (PostGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);

            DbgPrint("Before/after Flintstone state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[FLINTSTONE_INDEX].Attributes,
                    PostGroups->Groups[FLINTSTONE_INDEX].Attributes);

            DbgPrint("Before/after Child state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[CHILD_INDEX].Attributes,
                    PostGroups->Groups[CHILD_INDEX].Attributes);

            DbgPrint("Before/after Neanderthol state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[NEANDERTHOL_INDEX].Attributes,
                    PostGroups->Groups[NEANDERTHOL_INDEX].Attributes);

            DbgPrint("Before/after World state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[WORLD_INDEX].Attributes,
                    PostGroups->Groups[WORLD_INDEX].Attributes);

            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);

            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_SUCCESS);



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  Disable already disabled group                                     //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Disable already disabled group ...                     ");

    PreGroups->GroupCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PreGroups,              // TokenInformation
                       PreGroupsLength,        // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(PreGroups->GroupCount == GROUP_COUNT );
    ASSERT(PreGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes);
    ASSERT(PreGroups->Groups[CHILD_INDEX].Attributes       == DisabledGroupAttributes);
    ASSERT(PreGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes);
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    NewState->GroupCount = 1;
    NewState->Groups[0].Sid = ChildSid;
    NewState->Groups[0].Attributes = 0;

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 FALSE,                        // ResetToDefault
                 NewState,                     // NewState (OPTIONAL)
                 0,                            // BufferLength
                 NULL,                         // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );

    PostGroups->GroupCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PostGroups,             // TokenInformation
                       PostGroupsLength,       // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_SUCCESS) {

         //
         // Check the group values
         //

         ASSERT( PostGroups->GroupCount == GROUP_COUNT );
         if ( (PostGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes)    &&
              (PostGroups->Groups[CHILD_INDEX].Attributes       == DisabledGroupAttributes) &&
              (PostGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes) &&
              (PostGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);

            DbgPrint("Before/after Flintstone state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[FLINTSTONE_INDEX].Attributes,
                    PostGroups->Groups[FLINTSTONE_INDEX].Attributes);

            DbgPrint("Before/after Child state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[CHILD_INDEX].Attributes,
                    PostGroups->Groups[CHILD_INDEX].Attributes);

            DbgPrint("Before/after Neanderthol state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[NEANDERTHOL_INDEX].Attributes,
                    PostGroups->Groups[NEANDERTHOL_INDEX].Attributes);

            DbgPrint("Before/after World state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[WORLD_INDEX].Attributes,
                    PostGroups->Groups[WORLD_INDEX].Attributes);

            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);

            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_SUCCESS);



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  Enable optional group                                              //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Enable optional group ...                              ");

    PreGroups->GroupCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PreGroups,              // TokenInformation
                       PreGroupsLength,        // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(PreGroups->GroupCount == GROUP_COUNT );
    ASSERT(PreGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes);
    ASSERT(PreGroups->Groups[CHILD_INDEX].Attributes       == DisabledGroupAttributes);
    ASSERT(PreGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes);
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    NewState->GroupCount = 1;
    NewState->Groups[0].Sid = ChildSid;
    NewState->Groups[0].Attributes = SE_GROUP_ENABLED;

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 FALSE,                        // ResetToDefault
                 NewState,                     // NewState (OPTIONAL)
                 0,                            // BufferLength
                 NULL,                         // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );

    PostGroups->GroupCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PostGroups,             // TokenInformation
                       PostGroupsLength,       // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_SUCCESS) {

         //
         // Check the group values
         //

         ASSERT( PostGroups->GroupCount == GROUP_COUNT );
         if ( (PostGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes)    &&
              (PostGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes) &&
              (PostGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes) &&
              (PostGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);

            DbgPrint("Before/after Flintstone state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[FLINTSTONE_INDEX].Attributes,
                    PostGroups->Groups[FLINTSTONE_INDEX].Attributes);

            DbgPrint("Before/after Child state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[CHILD_INDEX].Attributes,
                    PostGroups->Groups[CHILD_INDEX].Attributes);

            DbgPrint("Before/after Neanderthol state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[NEANDERTHOL_INDEX].Attributes,
                    PostGroups->Groups[NEANDERTHOL_INDEX].Attributes);

            DbgPrint("Before/after World state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[WORLD_INDEX].Attributes,
                    PostGroups->Groups[WORLD_INDEX].Attributes);

            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);

            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_SUCCESS);



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  Enable already enabled group                                       //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Enable already enabled group ...                       ");

    PreGroups->GroupCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PreGroups,              // TokenInformation
                       PreGroupsLength,        // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(PreGroups->GroupCount == GROUP_COUNT );
    ASSERT(PreGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes);
    ASSERT(PreGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes);
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    NewState->GroupCount = 1;
    NewState->Groups[0].Sid = ChildSid;
    NewState->Groups[0].Attributes = SE_GROUP_ENABLED;

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 FALSE,                        // ResetToDefault
                 NewState,                     // NewState (OPTIONAL)
                 0,                            // BufferLength
                 NULL,                         // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );

    PostGroups->GroupCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PostGroups,             // TokenInformation
                       PostGroupsLength,       // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_SUCCESS) {

         //
         // Check the group values
         //

         ASSERT( PostGroups->GroupCount == GROUP_COUNT );
         if ( (PostGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes)    &&
              (PostGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes) &&
              (PostGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes) &&
              (PostGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);

            DbgPrint("Before/after Flintstone state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[FLINTSTONE_INDEX].Attributes,
                    PostGroups->Groups[FLINTSTONE_INDEX].Attributes);

            DbgPrint("Before/after Child state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[CHILD_INDEX].Attributes,
                    PostGroups->Groups[CHILD_INDEX].Attributes);

            DbgPrint("Before/after Neanderthol state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[NEANDERTHOL_INDEX].Attributes,
                    PostGroups->Groups[NEANDERTHOL_INDEX].Attributes);

            DbgPrint("Before/after World state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[WORLD_INDEX].Attributes,
                    PostGroups->Groups[WORLD_INDEX].Attributes);

            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);

            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_SUCCESS);



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  Disable optional and unknown group                                 //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Disable optional and unknown group ...                 ");

    PreGroups->GroupCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PreGroups,              // TokenInformation
                       PreGroupsLength,        // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(PreGroups->GroupCount == GROUP_COUNT );
    ASSERT(PreGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes);
    ASSERT(PreGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes);
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    NewState->GroupCount = 2;
    NewState->Groups[0].Sid = ChildSid;
    NewState->Groups[1].Sid = RubbleSid;
    NewState->Groups[0].Attributes = DisabledGroupAttributes;
    NewState->Groups[1].Attributes = DisabledGroupAttributes;

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 FALSE,                        // ResetToDefault
                 NewState,                     // NewState (OPTIONAL)
                 0,                            // BufferLength
                 NULL,                         // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );

    PostGroups->GroupCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PostGroups,             // TokenInformation
                       PostGroupsLength,       // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_NOT_ALL_ASSIGNED) {

         //
         // Check the group values
         //

         ASSERT( PostGroups->GroupCount == GROUP_COUNT );
         if ( (PostGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes)    &&
              (PostGroups->Groups[CHILD_INDEX].Attributes       == DisabledGroupAttributes) &&
              (PostGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes) &&
              (PostGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);

            DbgPrint("Before/after Flintstone state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[FLINTSTONE_INDEX].Attributes,
                    PostGroups->Groups[FLINTSTONE_INDEX].Attributes);

            DbgPrint("Before/after Child state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[CHILD_INDEX].Attributes,
                    PostGroups->Groups[CHILD_INDEX].Attributes);

            DbgPrint("Before/after Neanderthol state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[NEANDERTHOL_INDEX].Attributes,
                    PostGroups->Groups[NEANDERTHOL_INDEX].Attributes);

            DbgPrint("Before/after World state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[WORLD_INDEX].Attributes,
                    PostGroups->Groups[WORLD_INDEX].Attributes);

            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);

            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_NOT_ALL_ASSIGNED);




/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  Enable optional and unknown group                                  //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Enable optional and unknown group ...                  ");

    PreGroups->GroupCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PreGroups,              // TokenInformation
                       PreGroupsLength,        // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(PreGroups->GroupCount == GROUP_COUNT );
    ASSERT(PreGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes);
    ASSERT(PreGroups->Groups[CHILD_INDEX].Attributes       == DisabledGroupAttributes);
    ASSERT(PreGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes);
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    NewState->GroupCount = 2;
    NewState->Groups[0].Sid = ChildSid;
    NewState->Groups[1].Sid = RubbleSid;
    NewState->Groups[0].Attributes = OptionalGroupAttributes;
    NewState->Groups[1].Attributes = OptionalGroupAttributes;

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 FALSE,                        // ResetToDefault
                 NewState,                     // NewState (OPTIONAL)
                 0,                            // BufferLength
                 NULL,                         // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );

    PostGroups->GroupCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PostGroups,             // TokenInformation
                       PostGroupsLength,       // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_NOT_ALL_ASSIGNED) {

         //
         // Check the group values
         //

         ASSERT( PostGroups->GroupCount == GROUP_COUNT );
         if ( (PostGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes)    &&
              (PostGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes) &&
              (PostGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes) &&
              (PostGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);

            DbgPrint("Before/after Flintstone state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[FLINTSTONE_INDEX].Attributes,
                    PostGroups->Groups[FLINTSTONE_INDEX].Attributes);

            DbgPrint("Before/after Child state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[CHILD_INDEX].Attributes,
                    PostGroups->Groups[CHILD_INDEX].Attributes);

            DbgPrint("Before/after Neanderthol state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[NEANDERTHOL_INDEX].Attributes,
                    PostGroups->Groups[NEANDERTHOL_INDEX].Attributes);

            DbgPrint("Before/after World state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[WORLD_INDEX].Attributes,
                    PostGroups->Groups[WORLD_INDEX].Attributes);

            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);

            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_NOT_ALL_ASSIGNED);




/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  Disable optional and mandatory group                               //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Disable optional and mandatory group ...               ");

    PreGroups->GroupCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PreGroups,              // TokenInformation
                       PreGroupsLength,        // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(PreGroups->GroupCount == GROUP_COUNT );
    ASSERT(PreGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes);
    ASSERT(PreGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes);
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    NewState->GroupCount = 2;
    NewState->Groups[0].Sid = ChildSid;
    NewState->Groups[1].Sid = WorldSid;
    NewState->Groups[0].Attributes = DisabledGroupAttributes;
    NewState->Groups[1].Attributes = DisabledGroupAttributes;

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 FALSE,                        // ResetToDefault
                 NewState,                     // NewState (OPTIONAL)
                 0,                            // BufferLength
                 NULL,                         // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );

    PostGroups->GroupCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PostGroups,             // TokenInformation
                       PostGroupsLength,       // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_CANT_DISABLE_MANDATORY) {

         //
         // Check the group values
         //

         ASSERT( PostGroups->GroupCount == GROUP_COUNT );
         if ( (PostGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes)    &&
              (PostGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes) &&
              (PostGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes) &&
              (PostGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);

            DbgPrint("Before/after Flintstone state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[FLINTSTONE_INDEX].Attributes,
                    PostGroups->Groups[FLINTSTONE_INDEX].Attributes);

            DbgPrint("Before/after Child state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[CHILD_INDEX].Attributes,
                    PostGroups->Groups[CHILD_INDEX].Attributes);

            DbgPrint("Before/after Neanderthol state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[NEANDERTHOL_INDEX].Attributes,
                    PostGroups->Groups[NEANDERTHOL_INDEX].Attributes);

            DbgPrint("Before/after World state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[WORLD_INDEX].Attributes,
                    PostGroups->Groups[WORLD_INDEX].Attributes);

            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);

            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_CANT_DISABLE_MANDATORY);



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  Enable optional and mandatory group                                //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Enable optional and mandatory group ...                ");

    NewState->GroupCount = 1;
    NewState->Groups[0].Sid = ChildSid;
    NewState->Groups[0].Attributes = DisabledGroupAttributes;

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 FALSE,                        // ResetToDefault
                 NewState,                     // NewState (OPTIONAL)
                 0,                            // BufferLength
                 NULL,                         // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );
    ASSERT(Status == STATUS_SUCCESS);

    PreGroups->GroupCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PreGroups,              // TokenInformation
                       PreGroupsLength,        // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(PreGroups->GroupCount == GROUP_COUNT );
    ASSERT(PreGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes);
    ASSERT(PreGroups->Groups[CHILD_INDEX].Attributes       == DisabledGroupAttributes);
    ASSERT(PreGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes);
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    NewState->GroupCount = 2;
    NewState->Groups[0].Sid = ChildSid;
    NewState->Groups[1].Sid = WorldSid;
    NewState->Groups[0].Attributes = OptionalGroupAttributes;
    NewState->Groups[1].Attributes = OptionalGroupAttributes;

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 FALSE,                        // ResetToDefault
                 NewState,                     // NewState (OPTIONAL)
                 0,                            // BufferLength
                 NULL,                         // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );

    PostGroups->GroupCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PostGroups,             // TokenInformation
                       PostGroupsLength,       // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_SUCCESS) {

         //
         // Check the group values
         //

         ASSERT( PostGroups->GroupCount == GROUP_COUNT );
         if ( (PostGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes)    &&
              (PostGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes) &&
              (PostGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes) &&
              (PostGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);

            DbgPrint("Before/after Flintstone state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[FLINTSTONE_INDEX].Attributes,
                    PostGroups->Groups[FLINTSTONE_INDEX].Attributes);

            DbgPrint("Before/after Child state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[CHILD_INDEX].Attributes,
                    PostGroups->Groups[CHILD_INDEX].Attributes);

            DbgPrint("Before/after Neanderthol state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[NEANDERTHOL_INDEX].Attributes,
                    PostGroups->Groups[NEANDERTHOL_INDEX].Attributes);

            DbgPrint("Before/after World state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[WORLD_INDEX].Attributes,
                    PostGroups->Groups[WORLD_INDEX].Attributes);

            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);

            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_SUCCESS);



//////////////////////////////////////////////////////////////////////
//                                                                  //
// Disable optional group requesting previous state with            //
// insufficient buffer                                              //
//                                                                  //
//////////////////////////////////////////////////////////////////////


    DbgPrint("Se:     Too small buffer for previous state ...                ");


    PreGroups->GroupCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PreGroups,              // TokenInformation
                       PreGroupsLength,        // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(PreGroups->GroupCount == GROUP_COUNT );
    ASSERT(PreGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes);
    ASSERT(PreGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes);
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    NewState->GroupCount = 1;
    NewState->Groups[0].Sid = ChildSid;
    NewState->Groups[0].Attributes = DisabledGroupAttributes;

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 FALSE,                        // ResetToDefault
                 NewState,                     // NewState (OPTIONAL)
                 0,                            // BufferLength
                 PreviousState,                // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    PostGroups->GroupCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PostGroups,             // TokenInformation
                       PostGroupsLength,       // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_BUFFER_TOO_SMALL) {

         //
         // Check the group values
         //

         ASSERT( PostGroups->GroupCount == GROUP_COUNT );
         if ( (PostGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes)    &&
              (PostGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes) &&
              (PostGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes) &&
              (PostGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);

            DbgPrint("Before/after Flintstone state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[FLINTSTONE_INDEX].Attributes,
                    PostGroups->Groups[FLINTSTONE_INDEX].Attributes);

            DbgPrint("Before/after Child state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[CHILD_INDEX].Attributes,
                    PostGroups->Groups[CHILD_INDEX].Attributes);

            DbgPrint("Before/after Neanderthol state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[NEANDERTHOL_INDEX].Attributes,
                    PostGroups->Groups[NEANDERTHOL_INDEX].Attributes);

            DbgPrint("Before/after World state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[WORLD_INDEX].Attributes,
                    PostGroups->Groups[WORLD_INDEX].Attributes);

            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);

            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_BUFFER_TOO_SMALL);




/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  Disable optional requesting previous state                         //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Disable optional, requesting previous state ...        ");

    PreGroups->GroupCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PreGroups,              // TokenInformation
                       PreGroupsLength,        // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(PreGroups->GroupCount == GROUP_COUNT );
    ASSERT(PreGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes);
    ASSERT(PreGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes);
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    NewState->GroupCount = 2;
    NewState->Groups[0].Sid = NeandertholSid;
    NewState->Groups[1].Sid = ChildSid;
    NewState->Groups[0].Attributes = DisabledGroupAttributes;
    NewState->Groups[1].Attributes = DisabledGroupAttributes;
    PreviousState->GroupCount = 99;

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 FALSE,                        // ResetToDefault
                 NewState,                     // NewState (OPTIONAL)
                 PreviousStateBufferLength,    // BufferLength
                 PreviousState,                // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    PostGroups->GroupCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PostGroups,             // TokenInformation
                       PostGroupsLength,       // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_SUCCESS) {

         //
         // Check the group values
         //

         ASSERT( PostGroups->GroupCount == GROUP_COUNT );
         ASSERT( PreviousState->GroupCount == 2 );
         if ( (PostGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes)    &&
              (PostGroups->Groups[CHILD_INDEX].Attributes       == DisabledGroupAttributes) &&
              (PostGroups->Groups[NEANDERTHOL_INDEX].Attributes == DisabledGroupAttributes) &&
              (PostGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes)   &&
              (PreviousState->Groups[0].Attributes == OptionalGroupAttributes) &&
              (PreviousState->Groups[1].Attributes == OptionalGroupAttributes)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);

            DbgPrint("Before/after Flintstone state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[FLINTSTONE_INDEX].Attributes,
                    PostGroups->Groups[FLINTSTONE_INDEX].Attributes);

            DbgPrint("Before/after Child state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[CHILD_INDEX].Attributes,
                    PostGroups->Groups[CHILD_INDEX].Attributes);

            DbgPrint("Before/after Neanderthol state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[NEANDERTHOL_INDEX].Attributes,
                    PostGroups->Groups[NEANDERTHOL_INDEX].Attributes);

            DbgPrint("Before/after World state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[WORLD_INDEX].Attributes,
                    PostGroups->Groups[WORLD_INDEX].Attributes);

            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);

            DbgPrint("Previous count is: 0x%lx \n", PreviousState->GroupCount);
            DbgPrint("Previous state of group 0 is: 0x%lx \n",
                    PreviousState->Groups[0].Attributes);
            DbgPrint("Previous state of group 1 is: 0x%lx \n",
                    PreviousState->Groups[1].Attributes);


            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_SUCCESS);



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  Return group to previous state                                     //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Return to previous state ...                           ");

    PreGroups->GroupCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PreGroups,              // TokenInformation
                       PreGroupsLength,        // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(PreGroups->GroupCount == GROUP_COUNT );
    ASSERT(PreGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes);
    ASSERT(PreGroups->Groups[CHILD_INDEX].Attributes       == DisabledGroupAttributes);
    ASSERT(PreGroups->Groups[NEANDERTHOL_INDEX].Attributes == DisabledGroupAttributes);
    ASSERT(PreGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes);
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 FALSE,                        // ResetToDefault
                 PreviousState,                // NewState (OPTIONAL)
                 PreviousStateBufferLength,    // BufferLength
                 PreviousState,                // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    PostGroups->GroupCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PostGroups,             // TokenInformation
                       PostGroupsLength,       // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_SUCCESS) {

         //
         // Check the group values
         //

         ASSERT( PostGroups->GroupCount == GROUP_COUNT );
         if ( (PostGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes)    &&
              (PostGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes) &&
              (PostGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes) &&
              (PostGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes)   &&
              (PreviousState->Groups[0].Attributes == DisabledGroupAttributes) &&
              (PreviousState->Groups[1].Attributes == DisabledGroupAttributes)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);

            DbgPrint("Before/after Flintstone state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[FLINTSTONE_INDEX].Attributes,
                    PostGroups->Groups[FLINTSTONE_INDEX].Attributes);

            DbgPrint("Before/after Child state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[CHILD_INDEX].Attributes,
                    PostGroups->Groups[CHILD_INDEX].Attributes);

            DbgPrint("Before/after Neanderthol state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[NEANDERTHOL_INDEX].Attributes,
                    PostGroups->Groups[NEANDERTHOL_INDEX].Attributes);

            DbgPrint("Before/after World state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[WORLD_INDEX].Attributes,
                    PostGroups->Groups[WORLD_INDEX].Attributes);

            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);

            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_SUCCESS);



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  Return to previous state again                                     //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Return to previous state again ...                     ");

    PreGroups->GroupCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PreGroups,              // TokenInformation
                       PreGroupsLength,        // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(PreGroups->GroupCount == GROUP_COUNT );
    ASSERT(PreGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes);
    ASSERT(PreGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes);
    ASSERT(PreGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes);
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 FALSE,                        // ResetToDefault
                 PreviousState,                // NewState (OPTIONAL)
                 PreviousStateBufferLength,    // BufferLength
                 PreviousState,                // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    PostGroups->GroupCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PostGroups,             // TokenInformation
                       PostGroupsLength,       // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_SUCCESS) {

         //
         // Check the group values
         //

         ASSERT( PostGroups->GroupCount == GROUP_COUNT );
         if ( (PostGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes)    &&
              (PostGroups->Groups[CHILD_INDEX].Attributes       == DisabledGroupAttributes) &&
              (PostGroups->Groups[NEANDERTHOL_INDEX].Attributes == DisabledGroupAttributes) &&
              (PostGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes)   &&
              (PreviousState->Groups[0].Attributes == OptionalGroupAttributes) &&
              (PreviousState->Groups[1].Attributes == OptionalGroupAttributes)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);

            DbgPrint("Before/after Flintstone state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[FLINTSTONE_INDEX].Attributes,
                    PostGroups->Groups[FLINTSTONE_INDEX].Attributes);

            DbgPrint("Before/after Child state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[CHILD_INDEX].Attributes,
                    PostGroups->Groups[CHILD_INDEX].Attributes);

            DbgPrint("Before/after Neanderthol state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[NEANDERTHOL_INDEX].Attributes,
                    PostGroups->Groups[NEANDERTHOL_INDEX].Attributes);

            DbgPrint("Before/after World state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[WORLD_INDEX].Attributes,
                    PostGroups->Groups[WORLD_INDEX].Attributes);

            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);

            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_SUCCESS);




/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  Return to default state (capture previous state)                   //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Return to default state (w/previous state) ...         ");

    PreGroups->GroupCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PreGroups,              // TokenInformation
                       PreGroupsLength,        // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(PreGroups->GroupCount == GROUP_COUNT );
    ASSERT(PreGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes);
    ASSERT(PreGroups->Groups[CHILD_INDEX].Attributes       == DisabledGroupAttributes);
    ASSERT(PreGroups->Groups[NEANDERTHOL_INDEX].Attributes == DisabledGroupAttributes);
    ASSERT(PreGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes);
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 TRUE,                         // ResetToDefault
                 NULL,                         // NewState (OPTIONAL)
                 PreviousStateBufferLength,    // BufferLength
                 PreviousState,                // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) return length: 0x%lx \n", ReturnLength);
#endif //TOKEN_DEBUG

    PostGroups->GroupCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PostGroups,             // TokenInformation
                       PostGroupsLength,       // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_SUCCESS) {

         //
         // Check the group values
         //

         ASSERT( PostGroups->GroupCount == GROUP_COUNT );
         if ( (PostGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes)    &&
              (PostGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes) &&
              (PostGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes) &&
              (PostGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes)   &&
              (PreviousState->Groups[0].Attributes == DisabledGroupAttributes) &&
              (PreviousState->Groups[1].Attributes == DisabledGroupAttributes)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);

            DbgPrint("Before/after Flintstone state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[FLINTSTONE_INDEX].Attributes,
                    PostGroups->Groups[FLINTSTONE_INDEX].Attributes);

            DbgPrint("Before/after Child state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[CHILD_INDEX].Attributes,
                    PostGroups->Groups[CHILD_INDEX].Attributes);

            DbgPrint("Before/after Neanderthol state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[NEANDERTHOL_INDEX].Attributes,
                    PostGroups->Groups[NEANDERTHOL_INDEX].Attributes);

            DbgPrint("Before/after World state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[WORLD_INDEX].Attributes,
                    PostGroups->Groups[WORLD_INDEX].Attributes);

            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);

            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_SUCCESS);



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  Return to default state  (don't capture previous state)            //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:     Return to default state (no previous state) ...        ");

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 FALSE,                        // ResetToDefault
                 PreviousState,                // NewState (OPTIONAL)
                 0,                            // BufferLength
                 NULL,                         // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );

    ASSERT(Status == STATUS_SUCCESS);

    PreGroups->GroupCount = 77;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PreGroups,              // TokenInformation
                       PreGroupsLength,        // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(PreGroups->GroupCount == GROUP_COUNT );
    ASSERT(PreGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes);
    ASSERT(PreGroups->Groups[CHILD_INDEX].Attributes       == DisabledGroupAttributes);
    ASSERT(PreGroups->Groups[NEANDERTHOL_INDEX].Attributes == DisabledGroupAttributes);
    ASSERT(PreGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes);
    ASSERT(NT_SUCCESS(IgnoreStatus) );

    Status = NtAdjustGroupsToken(
                 TokenWithGroups,              // TokenHandle
                 TRUE,                         // ResetToDefault
                 NULL,                         // NewState (OPTIONAL)
                 0,                            // BufferLength
                 NULL,                         // PreviousState (OPTIONAL)
                 &ReturnLength                 // ReturnLength
                 );

    PostGroups->GroupCount = 88;
    IgnoreStatus = NtQueryInformationToken(
                       TokenWithGroups,        // TokenHandle
                       TokenGroups,            // TokenInformationClass
                       PostGroups,             // TokenInformation
                       PostGroupsLength,       // TokenInformationLength
                       &IgnoreReturnLength     // ReturnLength
                       );
#ifdef TOKEN_DEBUG
DbgPrint("\n (debug) ignore return length: 0x%lx \n", IgnoreReturnLength);
#endif //TOKEN_DEBUG

    ASSERT(NT_SUCCESS(IgnoreStatus) );

    if (Status == STATUS_SUCCESS) {

         //
         // Check the group values
         //

         ASSERT( PostGroups->GroupCount == GROUP_COUNT );
         if ( (PostGroups->Groups[FLINTSTONE_INDEX].Attributes  == OwnerGroupAttributes)    &&
              (PostGroups->Groups[CHILD_INDEX].Attributes       == OptionalGroupAttributes) &&
              (PostGroups->Groups[NEANDERTHOL_INDEX].Attributes == OptionalGroupAttributes) &&
              (PostGroups->Groups[WORLD_INDEX].Attributes       == NormalGroupAttributes)
         ) {

            DbgPrint("Succeeded. \n");

         } else {

            DbgPrint("********** Failed  Value Check ************\n");
            DbgPrint("Status is: 0x%lx \n", Status);

            DbgPrint("Before/after Flintstone state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[FLINTSTONE_INDEX].Attributes,
                    PostGroups->Groups[FLINTSTONE_INDEX].Attributes);

            DbgPrint("Before/after Child state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[CHILD_INDEX].Attributes,
                    PostGroups->Groups[CHILD_INDEX].Attributes);

            DbgPrint("Before/after Neanderthol state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[NEANDERTHOL_INDEX].Attributes,
                    PostGroups->Groups[NEANDERTHOL_INDEX].Attributes);

            DbgPrint("Before/after World state: 0x%lx / 0x%lx \n",
                    PreGroups->Groups[WORLD_INDEX].Attributes,
                    PostGroups->Groups[WORLD_INDEX].Attributes);

            DbgPrint("Return Length is: 0x%lx \n", ReturnLength);

            CompletionStatus = FALSE;

        }

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        DbgPrint("Return Length is: 0x%lx \n", ReturnLength);
        CompletionStatus = FALSE;

    }

    ASSERT(Status == STATUS_SUCCESS);




    ////////////////////////////////////////////////////////////////
    //                                                            //
    // Done with test                                             //
    //                                                            //
    ////////////////////////////////////////////////////////////////



    TstDeallocatePool( PreviousState, PreviousStateBufferLength );
    TstDeallocatePool( NewState, NewStateBufferLength );
    TstDeallocatePool( PreGroups, PreGroupsLength );
    TstDeallocatePool( PostGroups, PostGroupsLength );


    return CompletionStatus;
}


////////////////////////////////////////////////////////////////
//                                                            //
// Compare duplicate to original token & display test results //
//                                                            //
////////////////////////////////////////////////////////////////

BOOLEAN
TestpCompareDuplicateToken(
    IN NTSTATUS Status,
    IN HANDLE OldToken,
    IN OBJECT_ATTRIBUTES NewAttributes,
    IN BOOLEAN EffectiveOnly,
    IN TOKEN_TYPE NewType,
    IN HANDLE NewToken
    )

{
    BOOLEAN CompletionStatus = TRUE;

    ULONG OldReturnLength;
    ULONG NewReturnLength;

    PTOKEN_USER OldUserId = NULL;
    PTOKEN_USER NewUserId = NULL;

    TOKEN_SOURCE OldSource;
    TOKEN_SOURCE NewSource;

    TOKEN_STATISTICS OldStatistics;
    TOKEN_STATISTICS NewStatistics;

    BOOLEAN SomeNotCompared = FALSE;


    //
    // Appease the compiler Gods
    //
    NewAttributes = NewAttributes;
    NewType = NewType;
    EffectiveOnly = EffectiveOnly;


    //
    // If the status isn't success, don't bother comparing the tokens
    //

    if (!NT_SUCCESS(Status)) {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        return FALSE;
    }

    //
    // Compare the user IDs
    //

    Status = NtQueryInformationToken(
                 OldToken,                 // Handle
                 TokenUser,                // TokenInformationClass
                 OldUserId,                // TokenInformation
                 0,                        // TokenInformationLength
                 &OldReturnLength          // ReturnLength
                 ); ASSERT(Status == STATUS_BUFFER_TOO_SMALL);
    OldUserId = (PTOKEN_USER)TstAllocatePool( PagedPool, OldReturnLength );

    Status = NtQueryInformationToken(
                 OldToken,                 // Handle
                 TokenUser,                // TokenInformationClass
                 OldUserId,                // TokenInformation
                 OldReturnLength,          // TokenInformationLength
                 &OldReturnLength          // ReturnLength
                 ); ASSERT(NT_SUCCESS(Status));


    Status = NtQueryInformationToken(
                 NewToken,                 // Handle
                 TokenUser,                // TokenInformationClass
                 NewUserId,                // TokenInformation
                 0,                        // TokenInformationLength
                 &NewReturnLength          // ReturnLength
                 ); ASSERT(Status == STATUS_BUFFER_TOO_SMALL);

    NewUserId = (PTOKEN_USER)TstAllocatePool( PagedPool, NewReturnLength );

    Status = NtQueryInformationToken(
                 NewToken,                 // Handle
                 TokenUser,                // TokenInformationClass
                 NewUserId,                // TokenInformation
                 NewReturnLength,          // TokenInformationLength
                 &NewReturnLength          // ReturnLength
                 ); ASSERT(NT_SUCCESS(Status));


    if ( !RtlEqualSid(OldUserId->User.Sid, NewUserId->User.Sid) ) {

        if (CompletionStatus) {
            DbgPrint("*** Failed Value Comparison ***\n");
        }
        DbgPrint("User IDs don't match.\n");
        CompletionStatus = FALSE;
    }

    TstDeallocatePool( OldUserId, OldReturnLength );
    TstDeallocatePool( NewUserId, NewReturnLength );


    //
    // Check the token statistics
    //

    if (CompletionStatus) {
        Status = NtQueryInformationToken(
                     OldToken,                        // Handle
                     TokenStatistics,                 // TokenInformationClass
                     &OldStatistics,                  // TokenInformation
                     (ULONG)sizeof(TOKEN_STATISTICS), // TokenInformationLength
                     &OldReturnLength                 // ReturnLength
                     ); ASSERT(NT_SUCCESS(Status));

        Status = NtQueryInformationToken(
                     NewToken,                        // Handle
                     TokenStatistics,                 // TokenInformationClass
                     &NewStatistics,                  // TokenInformation
                     (ULONG)sizeof(TOKEN_STATISTICS), // TokenInformationLength
                     &NewReturnLength                 // ReturnLength
                     ); ASSERT(NT_SUCCESS(Status));
        //
        // Must have:
        //             Different TokenId values
        //             Same authenticationId value
        //             Same ExpirationTime
        //             Same token type
        //             Same ImpersonationLevel (if correct token type)
        //             Same DynamicCharged & DynamicAvailable
        //
        // GroupCount and PrivilegeCount are deferred to the group and
        // privilege comparison due to the difficulty involved with
        // taking EffectiveOnly into account.
        //
        // The new token must have a ModifiedId that is the same as the
        // original.
        //

        //
        // Token ID
        //

        if ( (OldStatistics.TokenId.HighPart ==
              NewStatistics.TokenId.HighPart)    &&
             (OldStatistics.TokenId.LowPart ==
              NewStatistics.TokenId.LowPart)  ) {

            DbgPrint("*** Failed ***\n");
            DbgPrint("       TokenIds are equal.\n");
            DbgPrint("       Old TokenId is: (0x%xl, 0x%xl)\n",
                            OldStatistics.TokenId.HighPart,
                            OldStatistics.TokenId.LowPart);
            DbgPrint("       New TokenId is: (0x%xl, 0x%xl)\n",
                            NewStatistics.TokenId.HighPart,
                            NewStatistics.TokenId.LowPart);
            DbgPrint("       ");
            CompletionStatus = FALSE;
        }


        //
        // Authentication ID
        //

        if ( !RtlEqualLuid(&OldStatistics.AuthenticationId,
                           &NewStatistics.AuthenticationId) ) {

            DbgPrint("*** Failed ***\n");
            DbgPrint("       AuthenticationIds are not equal.\n");
            DbgPrint("Original Authentication ID is: ");
            TestpPrintLuid(OldStatistics.AuthenticationId);
            DbgPrint("\n");
            DbgPrint("New Authentication ID is: ");
            TestpPrintLuid(NewStatistics.AuthenticationId);
            DbgPrint("\n");
            DbgPrint("       ");
            CompletionStatus = FALSE;
        }

        //
        // ExpirationTime
        //

        if ( (OldStatistics.ExpirationTime.HighPart !=
              NewStatistics.ExpirationTime.HighPart)    ||
             (OldStatistics.ExpirationTime.LowPart !=
              NewStatistics.ExpirationTime.LowPart)  ) {

            DbgPrint("*** Failed ***\n");
            DbgPrint("       ExpirationTimes differ.\n");
            DbgPrint("       ");
            CompletionStatus = FALSE;
        }

        //
        // TokenType
        //

        if ( OldStatistics.TokenType != NewStatistics.TokenType ) {

            DbgPrint("*** Failed ***\n");
            DbgPrint("       Token types are different.\n");
            DbgPrint("       Old token type is:  0x%lx \n", OldStatistics.TokenType );
            DbgPrint("       New token type is:  0x%lx \n", NewStatistics.TokenType );
            DbgPrint("       ");
            CompletionStatus = FALSE;
        }

        //
        // ImpersonationLevel
        //

        if (NewStatistics.TokenType = TokenImpersonation) {
            if ( OldStatistics.ImpersonationLevel !=
                 NewStatistics.ImpersonationLevel ) {

                DbgPrint("*** Failed ***\n");
                DbgPrint("       Impersonation levels are different.\n");
                DbgPrint("       Old impersonation level  is:  0x%lx \n",
                                OldStatistics.ImpersonationLevel );
                DbgPrint("       New impersonation level is:  0x%lx \n",
                                NewStatistics.ImpersonationLevel );
                DbgPrint("       ");
                CompletionStatus = FALSE;
            }
        }

        //
        // DynamicCharged
        //

        if ( OldStatistics.DynamicCharged != NewStatistics.DynamicCharged ) {

            DbgPrint("*** Failed ***\n");
            DbgPrint("       DynamicCharges are different.\n");
            DbgPrint("       Old value is:  0x%lx \n", OldStatistics.DynamicCharged );
            DbgPrint("       New value is:  0x%lx \n", NewStatistics.DynamicCharged );
            DbgPrint("       ");
            CompletionStatus = FALSE;
        }

        //
        // DynamicAvailable
        //

        if ( OldStatistics.DynamicAvailable != NewStatistics.DynamicAvailable ) {

            DbgPrint("*** Failed ***\n");
            DbgPrint("       DynamicAvailable are different.\n");
            DbgPrint("       Old value is:  0x%lx \n", OldStatistics.DynamicAvailable );
            DbgPrint("       New value is:  0x%lx \n", NewStatistics.DynamicAvailable );
            DbgPrint("       ");
            CompletionStatus = FALSE;
        }


        //
        // ModifiedId
        //

        if ( (NewStatistics.ModifiedId.HighPart !=
              OldStatistics.ModifiedId.HighPart)   ||
             (NewStatistics.ModifiedId.LowPart  !=
              OldStatistics.ModifiedId.LowPart)     ) {

            DbgPrint("*** Failed ***\n");
            DbgPrint("       ModifiedIds different.\n");
            DbgPrint("       Old ModifiedId is: (0x%xl, 0x%xl)\n",
                            OldStatistics.ModifiedId.HighPart,
                            OldStatistics.ModifiedId.LowPart);
            DbgPrint("       New ModifiedId is: (0x%xl, 0x%xl)\n",
                            NewStatistics.ModifiedId.HighPart,
                            NewStatistics.ModifiedId.LowPart);
            DbgPrint("       ");
            CompletionStatus = FALSE;
        }

    }

    //
    // Compare the group IDs
    //

    SomeNotCompared = TRUE;

    //
    // Compare the privileges
    //

    SomeNotCompared = TRUE;

    //
    // Compare the owner IDs
    //

    SomeNotCompared = TRUE;

    //
    // Compare the primary group IDs
    //

    SomeNotCompared = TRUE;

    //
    // Compare the default dacls
    //

    SomeNotCompared = TRUE;

    //
    // Compare the token source
    //

    if (CompletionStatus) {
        Status = NtQueryInformationToken(
                     OldToken,                    // Handle
                     TokenSource,                 // TokenInformationClass
                     &OldSource,                  // TokenInformation
                     (ULONG)sizeof(TOKEN_SOURCE), // TokenInformationLength
                     &OldReturnLength             // ReturnLength
                     ); ASSERT(NT_SUCCESS(Status));

        Status = NtQueryInformationToken(
                     NewToken,                    // Handle
                     TokenSource,                 // TokenInformationClass
                     &NewSource,                  // TokenInformation
                     (ULONG)sizeof(TOKEN_SOURCE), // TokenInformationLength
                     &NewReturnLength             // ReturnLength
                     ); ASSERT(NT_SUCCESS(Status));

        if ( (OldSource.SourceIdentifier.HighPart ==
              NewSource.SourceIdentifier.HighPart)    &&
             (OldSource.SourceIdentifier.LowPart ==
              NewSource.SourceIdentifier.LowPart)  ) {
            if (  (OldSource.SourceName[0] != NewSource.SourceName[0])  ||
                  (OldSource.SourceName[1] != NewSource.SourceName[1])  ||
                  (OldSource.SourceName[2] != NewSource.SourceName[2])  ||
                  (OldSource.SourceName[3] != NewSource.SourceName[3])  ||
                  (OldSource.SourceName[4] != NewSource.SourceName[4])  ||
                  (OldSource.SourceName[5] != NewSource.SourceName[5])  ||
                  (OldSource.SourceName[6] != NewSource.SourceName[6])  ||
                  (OldSource.SourceName[7] != NewSource.SourceName[7])  ) {

                DbgPrint("*** Failed Value Comparison ***\n");
                DbgPrint("       SourceName changed.\n");
                CompletionStatus = FALSE;

            }
        } else {

            DbgPrint("*** Failed Value Comparison ***\n");
            DbgPrint("       SourceIdentifier changed.\n");
            DbgPrint("       Old SourceIdentifier is: (0x%xl, 0x%xl)\n",
                            OldSource.SourceIdentifier.HighPart,
                            OldSource.SourceIdentifier.LowPart);
            DbgPrint("       New SourceIdentifier is: (0x%xl, 0x%xl)\n",
                            NewSource.SourceIdentifier.HighPart,
                            NewSource.SourceIdentifier.LowPart);
            CompletionStatus = FALSE;

        }
    }

    ////////////////////////////////// Done /////////////////////////


    if (SomeNotCompared) {
        DbgPrint("Incomplete\n");
        DbgPrint("        Some fields not yet compared ...                       ");
    }

    if (CompletionStatus) {

        DbgPrint("Succeeded. \n");
    }

    return CompletionStatus;
}


////////////////////////////////////////////////////////////////
//                                                            //
// Duplicate Token Test                                       //
//                                                            //
////////////////////////////////////////////////////////////////

BOOLEAN
TestTokenDuplicate()
{
    BOOLEAN CompletionStatus = TRUE;

    BOOLEAN EffectiveOnly;
    TOKEN_TYPE NewType;
    HANDLE NewToken;

    OBJECT_ATTRIBUTES NewAttributes;

    SECURITY_QUALITY_OF_SERVICE ImpersonationLevel;
    SECURITY_QUALITY_OF_SERVICE IdentificationLevel;



    DbgPrint("\n");

    //
    // Initialize variables
    //

    ImpersonationLevel.Length = (ULONG)sizeof(SECURITY_QUALITY_OF_SERVICE);
    ImpersonationLevel.ImpersonationLevel = SecurityImpersonation;
    ImpersonationLevel.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    ImpersonationLevel.EffectiveOnly = FALSE;

    IdentificationLevel.Length = (ULONG)sizeof(SECURITY_QUALITY_OF_SERVICE);
    IdentificationLevel.ImpersonationLevel = SecurityImpersonation;
    IdentificationLevel.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    IdentificationLevel.EffectiveOnly = FALSE;


    InitializeObjectAttributes(
        &NewAttributes,
        NULL,
        OBJ_INHERIT,
        NULL,
        NULL
        );



    ////////////////////////////////////////////////////////////
    //                                                        //
    // Duplicate the simple token                             //
    //                                                        //
    ////////////////////////////////////////////////////////////

    DbgPrint("Se:     Duplicate primary token ...                            ");

    EffectiveOnly = FALSE;
    NewType = TokenImpersonation;
    NewAttributes.SecurityQualityOfService = &ImpersonationLevel;

    Status = NtDuplicateToken(
                 SimpleToken,             // ExistingTokenHandle
                 0,                       // DesiredAccess
                 &NewAttributes,          // ObjectAttributes
                 EffectiveOnly,           // EffectiveOnly
                 NewType,                 // TokenType
                 &NewToken                // NewTokenHandle
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
        Status = NtClose( NewToken ); ASSERT(NT_SUCCESS(NewToken));

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        return FALSE;
    }



    ////////////////////////////////////////////////////////////
    //                                                        //
    // Duplicate the restricted token                         //
    //                                                        //
    ////////////////////////////////////////////////////////////

    DbgPrint("Se:     Duplicate restricted sids ...                          ");

    EffectiveOnly = FALSE;
    NewType = TokenImpersonation;
    NewAttributes.SecurityQualityOfService = &ImpersonationLevel;

    Status = NtDuplicateToken(
                 TokenWithRestrictedSids, // ExistingTokenHandle
                 0,                       // DesiredAccess
                 &NewAttributes,          // ObjectAttributes
                 EffectiveOnly,           // EffectiveOnly
                 NewType,                 // TokenType
                 &NewToken                // NewTokenHandle
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
        Status = NtClose( NewToken ); ASSERT(NT_SUCCESS(NewToken));

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        return FALSE;
    }


    ////////////////////////////////////////////////////////////
    //                                                        //
    // Duplicate the token with restricted groups             //
    //                                                        //
    ////////////////////////////////////////////////////////////

    DbgPrint("Se:     Duplicate restricted groups ...                          ");

    EffectiveOnly = TRUE;
    NewType = TokenImpersonation;
    NewAttributes.SecurityQualityOfService = &ImpersonationLevel;

    Status = NtDuplicateToken(
                 TokenWithRestrictedSids, // ExistingTokenHandle
                 0,                       // DesiredAccess
                 &NewAttributes,          // ObjectAttributes
                 EffectiveOnly,           // EffectiveOnly
                 NewType,                 // TokenType
                 &NewToken                // NewTokenHandle
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
        Status = NtClose( NewToken ); ASSERT(NT_SUCCESS(NewToken));

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        return FALSE;
    }


    ////////////////////////////////////////////////////////////
    //                                                        //
    // Duplicate the full impersonation token                 //
    //                                                        //
    ////////////////////////////////////////////////////////////

    DbgPrint("Se:     Duplicate full impersonation token ...                      ");

    EffectiveOnly = FALSE;
    NewType = TokenImpersonation;
    NewAttributes.SecurityQualityOfService = &ImpersonationLevel;

    Status = NtDuplicateToken(
                 ImpersonationToken,       // ExistingTokenHandle
                 0,                        // DesiredAccess
                 &NewAttributes,           // ObjectAttributes
                 EffectiveOnly,            // EffectiveOnly
                 NewType,                  // TokenType
                 &NewToken                 // NewTokenHandle
                 );
    //
    // Check to see that the duplicate is really a duplicate of
    // the original and display the test results.
    //

    if (!TestpCompareDuplicateToken( Status,
                                     ImpersonationToken,
                                     NewAttributes,
                                     EffectiveOnly,
                                     NewType,
                                     NewToken ) ) {

        CompletionStatus = FALSE;
    }

    if (NT_SUCCESS(Status)) {

        Status = NtClose( NewToken );

        ASSERT(NT_SUCCESS(Status));
    }


    ////////////////////////////////////////////////////////////
    //                                                        //
    // Duplicate the full token, effective only               //
    //                                                        //
    ////////////////////////////////////////////////////////////

    DbgPrint("Se:     Duplicate full token, effective only ...                    ");

    EffectiveOnly = TRUE;
    NewType = TokenImpersonation;
    NewAttributes.SecurityQualityOfService = &ImpersonationLevel;

    Status = NtDuplicateToken(
                 ImpersonationToken,       // ExistingTokenHandle
                 0,                        // DesiredAccess
                 &NewAttributes,           // ObjectAttributes
                 EffectiveOnly,            // EffectiveOnly
                 NewType,                  // TokenType
                 &NewToken                 // NewTokenHandle
                 );
    //
    // Check to see that the duplicate is really a duplicate of
    // the original and display the test results.
    //

    if (!TestpCompareDuplicateToken( Status,
                                     ImpersonationToken,
                                     NewAttributes,
                                     EffectiveOnly,
                                     NewType,
                                     NewToken ) ) {

        CompletionStatus = FALSE;
    }

    if (NT_SUCCESS(Status)) {

        Status = NtClose( NewToken );

        ASSERT(NT_SUCCESS(Status));
    }










    return CompletionStatus;
}

////////////////////////////////////////////////////////////////
//                                                            //
// Assign Primary Token Test                                  //
//                                                            //
////////////////////////////////////////////////////////////////

BOOLEAN
TestTokenAssignPrimary()
{
    BOOLEAN CompletionStatus = TRUE;
    ULONG ReturnLength;

    TOKEN_STATISTICS OriginalTokenStatistics;
    TOKEN_STATISTICS NewTokenStatistics;
    TOKEN_STATISTICS AssignedTokenStatistics;


    TOKEN_USER UserId;
    TOKEN_PRIMARY_GROUP PrimaryGroup;
    PTOKEN_GROUPS GroupIds;
    PTOKEN_PRIVILEGES Privileges;
    TOKEN_DEFAULT_DACL DefaultDacl;
    TOKEN_OWNER Owner;

    PROCESS_ACCESS_TOKEN PrimaryTokenInfo;

    DbgPrint("\n");


    ////////////////////////////////////////////////////////////
    //                                                        //
    // Assign a valid primary token                           //
    //                                                        //
    ////////////////////////////////////////////////////////////

    DbgPrint("Se:     Assign new primary token ...                           ");

    //
    // Get information about the current token
    //

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_ALL_ACCESS,
                 &ProcessToken
                 );
    ASSERT (NT_SUCCESS(Status));

    Status = NtQueryInformationToken(
                 ProcessToken,                 // Handle
                 TokenStatistics,              // TokenInformationClass
                 &OriginalTokenStatistics,     // TokenInformation
                 sizeof(TOKEN_STATISTICS),     // TokenInformationLength
                 &ReturnLength                 // ReturnLength
                 );
    ASSERT(NT_SUCCESS(Status));




    //
    // Create a token with default DACL for use
    //

    GroupIds = (PTOKEN_GROUPS)TstAllocatePool( PagedPool,
                                               GROUP_IDS_LENGTH
                                               );

    Privileges = (PTOKEN_PRIVILEGES)TstAllocatePool( PagedPool,
                                                     PRIVILEGES_LENGTH
                                                     );

    DefaultDacl.DefaultDacl = (PACL)TstAllocatePool( PagedPool,
                                                     DEFAULT_DACL_LENGTH
                                                     );

    GroupIds->GroupCount = GROUP_COUNT;

    GroupIds->Groups[FLINTSTONE_INDEX].Sid  = FlintstoneSid;
    GroupIds->Groups[CHILD_INDEX].Sid       = ChildSid;
    GroupIds->Groups[SYSTEM_INDEX].Sid      = LocalSystemSid;
    GroupIds->Groups[WORLD_INDEX].Sid       = WorldSid;

    GroupIds->Groups[FLINTSTONE_INDEX].Attributes  = OwnerGroupAttributes;
    GroupIds->Groups[CHILD_INDEX].Attributes       = OptionalGroupAttributes;
    GroupIds->Groups[SYSTEM_INDEX].Attributes      = OptionalGroupAttributes;
    GroupIds->Groups[WORLD_INDEX].Attributes       = NormalGroupAttributes;

    UserId.User.Sid = PebblesSid;
    UserId.User.Attributes = 0;

    Owner.Owner = FlintstoneSid;

    Privileges->PrivilegeCount = PRIVILEGE_COUNT;

    Privileges->Privileges[UNSOLICITED_INDEX].Luid = UnsolicitedInputPrivilege;
    Privileges->Privileges[SECURITY_INDEX].Luid = SecurityPrivilege;
    Privileges->Privileges[ASSIGN_PRIMARY_INDEX].Luid = AssignPrimaryTokenPrivilege;
    Privileges->Privileges[UNSOLICITED_INDEX].Attributes = 0;
    Privileges->Privileges[SECURITY_INDEX].Attributes = 0;
    Privileges->Privileges[ASSIGN_PRIMARY_INDEX].Attributes = SE_PRIVILEGE_ENABLED;

    PrimaryGroup.PrimaryGroup = FlintstoneSid;

    Status = RtlCreateAcl( DefaultDacl.DefaultDacl, DEFAULT_DACL_LENGTH, ACL_REVISION);

    ASSERT(NT_SUCCESS(Status) );

    Status = NtCreateToken(
                 &Token,                   // Handle
                 (TOKEN_ALL_ACCESS),       // DesiredAccess
                 &PrimaryTokenAttributes,  // ObjectAttributes
                 TokenPrimary,             // TokenType
                 &SystemAuthenticationId,   // Authentication LUID
                 &NoExpiration,            // Expiration Time
                 &UserId,                  // Owner ID
                 GroupIds,                 // Group IDs
                 Privileges,               // Privileges
                 &Owner,                   // Owner
                 &PrimaryGroup,            // Primary Group
                 &DefaultDacl,             // Default Dacl
                 &TestSource               // TokenSource
                 );
    ASSERT(NT_SUCCESS(Status));

    //
    // Make sure key data is different than what is already on the process.
    //

    Status = NtQueryInformationToken(
                 Token,                        // Handle
                 TokenStatistics,              // TokenInformationClass
                 &NewTokenStatistics,          // TokenInformation
                 sizeof(TOKEN_STATISTICS),     // TokenInformationLength
                 &ReturnLength                 // ReturnLength
                 );
    ASSERT(NT_SUCCESS(Status));

    ASSERT( (OriginalTokenStatistics.TokenId.HighPart !=
             NewTokenStatistics.TokenId.HighPart)  ||
            (OriginalTokenStatistics.TokenId.LowPart !=
             NewTokenStatistics.TokenId.LowPart)        );



    //
    // Assign the new token
    //

    PrimaryTokenInfo.Token  = Token;
    PrimaryTokenInfo.Thread = NtCurrentThread();
    Status = NtSetInformationProcess(
                 NtCurrentProcess(),
                 ProcessAccessToken,
                 (PVOID)&PrimaryTokenInfo,
                 (ULONG)sizeof(PROCESS_ACCESS_TOKEN)
                 );

    if (!NT_SUCCESS(Status)) {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;

    } else {

        Status = NtClose( Token );
        ASSERT(NT_SUCCESS(Status));


        //
        // Get information about the assigned token
        //

        Status = NtOpenProcessToken(
                     NtCurrentProcess(),
                     TOKEN_QUERY | TOKEN_QUERY_SOURCE,
                     &Token
                     );
        ASSERT (NT_SUCCESS(Status));

        Status = NtQueryInformationToken(
                     Token,                        // Handle
                     TokenStatistics,              // TokenInformationClass
                     &AssignedTokenStatistics,     // TokenInformation
                     sizeof(TOKEN_STATISTICS),     // TokenInformationLength
                     &ReturnLength                 // ReturnLength
                     );
        ASSERT(NT_SUCCESS(Status));

        Status = NtClose( Token );
        ASSERT(NT_SUCCESS(Status));


        //
        // Information about assigned token and the new token
        // should be the same
        //

        ASSERT(AssignedTokenStatistics.TokenType == TokenPrimary);

        if ( (NewTokenStatistics.TokenId.HighPart ==
              AssignedTokenStatistics.TokenId.HighPart)  &&
             (NewTokenStatistics.TokenId.LowPart ==
              AssignedTokenStatistics.TokenId.LowPart) ) {

            DbgPrint("Succeeded.\n");

        } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Token ID mismatch.\n");
        DbgPrint("New token ID is:      (0x%lx, 0x%lx) \n",
                 NewTokenStatistics.TokenId.HighPart,
                 NewTokenStatistics.TokenId.LowPart);
        DbgPrint("Assigned token ID is: (0x%lx, 0x%lx) \n",
                 AssignedTokenStatistics.TokenId.HighPart,
                 AssignedTokenStatistics.TokenId.LowPart);
        CompletionStatus = FALSE;

        }
    }

    //
    // Change back to the original token
    //

    PrimaryTokenInfo.Token  = ProcessToken;
    PrimaryTokenInfo.Thread = NtCurrentThread();
    Status = NtSetInformationProcess(
                 NtCurrentProcess(),
                 ProcessAccessToken,
                 (PVOID)&PrimaryTokenInfo,
                 (ULONG)sizeof(PROCESS_ACCESS_TOKEN)
                 );

    ASSERT(NT_SUCCESS(Status));
    Status = NtClose( ProcessToken );
    ASSERT(NT_SUCCESS(Status));


    ////////////////////////////////////////////////////////////
    //                                                        //
    // Attempt to assign an impersonation token as primary    //
    //                                                        //
    ////////////////////////////////////////////////////////////

    DbgPrint("Se:     Assign impersonation token as primary ...              ");


    //
    // Create an impersonation token
    //
    GroupIds->GroupCount = GROUP_COUNT;

    GroupIds->Groups[FLINTSTONE_INDEX].Sid  = FlintstoneSid;
    GroupIds->Groups[CHILD_INDEX].Sid       = ChildSid;
    GroupIds->Groups[NEANDERTHOL_INDEX].Sid = NeandertholSid;
    GroupIds->Groups[WORLD_INDEX].Sid       = WorldSid;

    GroupIds->Groups[FLINTSTONE_INDEX].Attributes  = OwnerGroupAttributes;
    GroupIds->Groups[CHILD_INDEX].Attributes       = OptionalGroupAttributes;
    GroupIds->Groups[NEANDERTHOL_INDEX].Attributes = OptionalGroupAttributes;
    GroupIds->Groups[WORLD_INDEX].Attributes       = NormalGroupAttributes;

    UserId.User.Sid = PebblesSid;
    UserId.User.Attributes = 0;

    Owner.Owner = FlintstoneSid;

    Privileges->PrivilegeCount = PRIVILEGE_COUNT;

    Privileges->Privileges[UNSOLICITED_INDEX].Luid = UnsolicitedInputPrivilege;
    Privileges->Privileges[SECURITY_INDEX].Luid = SecurityPrivilege;
    Privileges->Privileges[ASSIGN_PRIMARY_INDEX].Luid = AssignPrimaryTokenPrivilege;
    Privileges->Privileges[UNSOLICITED_INDEX].Attributes = 0;
    Privileges->Privileges[SECURITY_INDEX].Attributes = 0;
    Privileges->Privileges[ASSIGN_PRIMARY_INDEX].Attributes = SE_PRIVILEGE_ENABLED;

    PrimaryGroup.PrimaryGroup = FlintstoneSid;

    Status = RtlCreateAcl( DefaultDacl.DefaultDacl, DEFAULT_DACL_LENGTH, ACL_REVISION);

    ASSERT(NT_SUCCESS(Status) );

    Status = NtCreateToken(
                 &Token,                   // Handle
                 (TOKEN_ALL_ACCESS),       // DesiredAccess
                 &ImpersonationTokenAttributes,  // ObjectAttributes
                 TokenImpersonation,       // TokenType
                 &OriginalAuthenticationId,   // Authentication LUID
                 &NoExpiration,            // Expiration Time
                 &UserId,                  // Owner ID
                 GroupIds,                 // Group IDs
                 Privileges,               // Privileges
                 &Owner,                   // Owner
                 &PrimaryGroup,            // Primary Group
                 &DefaultDacl,             // Default Dacl
                 &TestSource               // TokenSource
                 );
    ASSERT(NT_SUCCESS(Status));

    //
    // Assign the new token
    //

    PrimaryTokenInfo.Token  = Token;
    PrimaryTokenInfo.Thread = NtCurrentThread();
    Status = NtSetInformationProcess(
                 NtCurrentProcess(),
                 ProcessAccessToken,
                 (PVOID)&PrimaryTokenInfo,
                 (ULONG)sizeof(PROCESS_ACCESS_TOKEN)
                 );

    if (Status == STATUS_BAD_TOKEN_TYPE) {

        DbgPrint("Succeeded.\n");

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;

    }

    Status = NtClose( Token );
    ASSERT(NT_SUCCESS(Status));


    return CompletionStatus;
}

////////////////////////////////////////////////////////////////
//                                                            //
// Impersonation Test  (with open test)                       //
//                                                            //
////////////////////////////////////////////////////////////////

BOOLEAN
TestTokenImpersonation()
{
    BOOLEAN CompletionStatus = TRUE;

    HANDLE OpenedToken;
    HANDLE NewToken;
    OBJECT_ATTRIBUTES NewAttributes;
    TOKEN_TYPE NewType;
    BOOLEAN EffectiveOnly = FALSE;

    SECURITY_QUALITY_OF_SERVICE ImpersonationLevel;



    DbgPrint("\n");


    ////////////////////////////////////////////////////////////
    //                                                        //
    // Terminate impersonation using NtSetInformationThread() //
    //                                                        //
    ////////////////////////////////////////////////////////////

    DbgPrint("Se:     Revert to self (specify NULL handle) ...               ");

    NewToken = NULL;
    Status = NtSetInformationThread(
                 NtCurrentThread(),
                 ThreadImpersonationToken,
                 (PVOID)&NewToken,
                 (ULONG)sizeof(HANDLE)
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;

    }


    ////////////////////////////////////////////////////////////
    //                                                        //
    // Attempt to assign a primary token as an impersonation  //
    // token.                                                 //
    //                                                        //
    ////////////////////////////////////////////////////////////

    DbgPrint("Se:     Assigning primary token as impersonation token ...     ");

    NewToken = TokenWithGroups;
    Status = NtSetInformationThread(
                 NtCurrentThread(),
                 ThreadImpersonationToken,
                 (PVOID)&NewToken,
                 (ULONG)sizeof(HANDLE)
                 );

    if (Status == STATUS_BAD_TOKEN_TYPE) {
        DbgPrint("Succeeded.\n");
    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;

    }


    ////////////////////////////////////////////////////////////
    //                                                        //
    // Assign a valid impersonation token                     //
    //                                                        //
    ////////////////////////////////////////////////////////////

    DbgPrint("Se:     Assign valid impersonation token ...                   ");

    NewToken = ImpersonationToken;
    Status = NtSetInformationThread(
                 NtCurrentThread(),
                 ThreadImpersonationToken,
                 (PVOID)&NewToken,
                 (ULONG)sizeof(HANDLE)
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;

    }


    ////////////////////////////////////////////////////////////
    //                                                        //
    // Open the impersonation token                           //
    //                                                        //
    ////////////////////////////////////////////////////////////


    DbgPrint("Se:     Open an impersonation token ...                        ");

    Status = NtOpenThreadToken(
                 NtCurrentThread(),
                 TOKEN_ALL_ACCESS,
                 TRUE,
                 &OpenedToken
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
        Status = NtClose( OpenedToken );
        ASSERT(NT_SUCCESS(Status));
    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;

    }



    ////////////////////////////////////////////////////////////
    //                                                        //
    // Open a non-existent impersonation token                //
    //                                                        //
    ////////////////////////////////////////////////////////////


    DbgPrint("Se:     Open a non-existent impersonation token ...            ");

    //
    // Clear any existing impersonation token.
    //

    NewToken = NULL;
    Status = NtSetInformationThread(
                 NtCurrentThread(),
                 ThreadImpersonationToken,
                 (PVOID)&NewToken,
                 (ULONG)sizeof(HANDLE)
                 );  ASSERT(NT_SUCCESS(Status));

    Status = NtOpenThreadToken(
                 NtCurrentThread(),
                 TOKEN_ALL_ACCESS,
                 TRUE,
                 &OpenedToken
                 );

    if (Status == STATUS_NO_TOKEN) {
        DbgPrint("Succeeded.\n");
    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;

    }


    ////////////////////////////////////////////////////////////
    //                                                        //
    // Open an anonymous   impersonation token                //
    //                                                        //
    ////////////////////////////////////////////////////////////


    DbgPrint("Se:     Open an anonymous impersonation token ...              ");

    //
    //  Assign an anonymous impersonation token
    //

    NewToken = AnonymousToken;
    Status = NtSetInformationThread(
                 ThreadHandle,
                 ThreadImpersonationToken,
                 (PVOID)&NewToken,
                 (ULONG)sizeof(HANDLE)
                 );  ASSERT(NT_SUCCESS(Status));


    Status = NtOpenThreadToken(
                 ThreadHandle,
                 TOKEN_ALL_ACCESS,
                 TRUE,
                 &OpenedToken
                 );

    if (Status == STATUS_CANT_OPEN_ANONYMOUS) {
        DbgPrint("Succeeded.\n");
    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;

    }


    ////////////////////////////////////////////////////////////
    //                                                        //
    // Change the impersonation of a thread                   //
    //                                                        //
    ////////////////////////////////////////////////////////////


    DbgPrint("Se:     Change the impersonation token ...                     ");

    NewToken = NULL;
    Status = NtSetInformationThread(
                 ThreadHandle,
                 ThreadImpersonationToken,
                 (PVOID)&NewToken,
                 (ULONG)sizeof(HANDLE)
                 );  ASSERT(NT_SUCCESS(Status));

    NewToken = AnonymousToken;
    Status = NtSetInformationThread(
                 ThreadHandle,
                 ThreadImpersonationToken,
                 (PVOID)&NewToken,
                 (ULONG)sizeof(HANDLE)
                 );  ASSERT(NT_SUCCESS(Status));

    NewToken = ImpersonationToken;
    Status = NtSetInformationThread(
                 ThreadHandle,
                 ThreadImpersonationToken,
                 (PVOID)&NewToken,
                 (ULONG)sizeof(HANDLE)
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;

    }

    ////////////////////////////////////////////////////////////
    //                                                        //
    // Impersonate a restricted token                         //
    //                                                        //
    ////////////////////////////////////////////////////////////


    DbgPrint("Se:     Impersonate restricted token ...                      ");

    NewToken = NULL;
    Status = NtSetInformationThread(
                 NtCurrentThread(),
                 ThreadImpersonationToken,
                 (PVOID)&NewToken,
                 (ULONG)sizeof(HANDLE)
                 );  ASSERT(NT_SUCCESS(Status));




    //
    // Initialize variables
    //

    InitializeObjectAttributes(
        &NewAttributes,
        NULL,
        OBJ_INHERIT,
        NULL,
        NULL
        );


    ImpersonationLevel.Length = (ULONG)sizeof(SECURITY_QUALITY_OF_SERVICE);
    ImpersonationLevel.ImpersonationLevel = SecurityImpersonation;
    ImpersonationLevel.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    ImpersonationLevel.EffectiveOnly = FALSE;
    NewType = TokenImpersonation;
    NewAttributes.SecurityQualityOfService = &ImpersonationLevel;


    Status = NtDuplicateToken(
                 TokenWithRestrictedSids, // ExistingTokenHandle
                 TOKEN_ALL_ACCESS,        // DesiredAccess
                 &NewAttributes,          // ObjectAttributes
                 EffectiveOnly,           // EffectiveOnly
                 NewType,                 // TokenType
                 &NewToken                // NewTokenHandle
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }

    Status = NtSetInformationThread(
                 NtCurrentThread(),
                 ThreadImpersonationToken,
                 (PVOID)&NewToken,
                 (ULONG)sizeof(HANDLE)
                 );  ASSERT(NT_SUCCESS(Status));

    //
    // Now try to open something, like the process, which should fail
    //

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_QUERY | TOKEN_QUERY_SOURCE,
                 &Token
                 );
    if (Status != STATUS_ACCESS_DENIED) {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 MAXIMUM_ALLOWED,
                 &Token
                 );
    if (Status != STATUS_ACCESS_DENIED) {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }

    Status = NtDuplicateToken(
                 TokenWithMoreRestrictedSids, // ExistingTokenHandle
                 TOKEN_ALL_ACCESS,        // DesiredAccess
                 &NewAttributes,          // ObjectAttributes
                 EffectiveOnly,           // EffectiveOnly
                 NewType,                 // TokenType
                 &NewToken                // NewTokenHandle
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");

    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }

    Status = NtSetInformationThread(
                 NtCurrentThread(),
                 ThreadImpersonationToken,
                 (PVOID)&NewToken,
                 (ULONG)sizeof(HANDLE)
                 );  ASSERT(NT_SUCCESS(Status));


    //
    // Now try to open something, like the process, which should succeed
    //

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_QUERY | TOKEN_QUERY_SOURCE,
                 &Token
                 );
    if (Status != STATUS_SUCCESS) {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 MAXIMUM_ALLOWED,
                 &Token
                 );
    if (Status != STATUS_SUCCESS) {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }

    NewToken = NULL;
    Status = NtSetInformationThread(
                 NtCurrentThread(),
                 ThreadImpersonationToken,
                 (PVOID)&NewToken,
                 (ULONG)sizeof(HANDLE)
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
    } else {

        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;

    }

    Status = NtTerminateThread(
                 ThreadHandle,
                 (NTSTATUS)0
                 );

    ASSERT(NT_SUCCESS(Status));

    return CompletionStatus;
}

////////////////////////////////////////////////////////////////
//                                                            //
// Main Program Entry                                         //
//                                                            //
////////////////////////////////////////////////////////////////

BOOLEAN
CTToken()      // Common Test for Token object
{
    BOOLEAN Result = TRUE;

    DbgPrint("Se:   Initialization...");
    TestTokenInitialize();

    DbgPrint("Se:   Token Creation Test...                                 Test");
    if (!TestTokenCreate()) { Result = FALSE; }

    DbgPrint("Se:   Token Filtering Test...                                Test");
    if (!TestTokenFilter()) { Result = FALSE; }

    DbgPrint("Se:   Token Open Test (with primary token)...                Test");
    if (!TestTokenOpenPrimary()) { Result = FALSE; }

    DbgPrint("Se:   Token Query Test...                                    Test");
    if (!TestTokenQuery()) { Result = FALSE; }

    DbgPrint("Se:   Token Set Test...                                      Test");
    if (!TestTokenSet()) { Result = FALSE; }

    DbgPrint("Se:   Token Adjust Privileges Test...                        Test");
    if (!TestTokenAdjustPrivileges()) {Result = FALSE; }

    DbgPrint("Se:   Token Adjust Group Test...                             Test");
    if (!TestTokenAdjustGroups()) { Result = FALSE; }

    DbgPrint("Se:   Token Duplication Test...                              Test");
    if (!TestTokenDuplicate()) { Result = FALSE; }

    DbgPrint("Se:   Primary Token Assignment Test...                       Test");
    if (!TestTokenAssignPrimary()) { Result = FALSE; }

    DbgPrint("Se:   Impersonation Test (and impersonation open)...         Test");
    if (!TestTokenImpersonation()) { Result = FALSE; }


    DbgPrint("\n");
    DbgPrint("\n");
    DbgPrint("    ********************\n");
    DbgPrint("    **                **\n");
    if (Result) {
        DbgPrint("Se: ** Test Succeeded **\n");
    } else {
        DbgPrint("Se: **  Test Failed   **\n");
    }

    DbgPrint("    **                **\n");
    DbgPrint("    ********************\n");
    DbgPrint("\n");
    DbgPrint("\n");

    return Result;
}

