/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ctaccess.c

Abstract:

    Common access validation test routines

    These routines are used in both kernel and user mode tests.

    This test assumes the security runtime library routines are
    functioning correctly.


Author:

    Robert Reichel      (robertre)      12/14/90

Environment:

    Test of access validation routines

Revision History:

    v1: robertre
        Created

--*/

#include "tsecomm.c"    // Mode dependent macros and routines.



//
//  Define the local macros and procedure for this module
//

//
//  Return a pointer to the first Ace in an Acl (even if the Acl is empty).
//
//      PACE_HEADER
//      FirstAce (
//          IN PACL Acl
//          );
//

#define FirstAce(Acl) ((PVOID)((PUCHAR)(Acl) + sizeof(ACL)))

//
//  Return a pointer to the next Ace in a sequence (even if the input
//  Ace is the one in the sequence).
//
//      PACE_HEADER
//      NextAce (
//          IN PACE_HEADER Ace
//          );
//

#define NextAce(Ace) ((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize))

VOID
DumpAcl (
    IN PACL Acl
    );

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

//
// definitions related to TokenWithGroups
//

#define FLINTSTONE_INDEX  (0L)
#define CHILD_INDEX       (1L)
#define NEANDERTHOL_INDEX (2L)
#define WORLD_INDEX       (3L)
#define GROUP_COUNT       (4L)


//
// Definitions related to TokenWithPrivileges
//

#define UNSOLICITED_INDEX  (0L)
#define SECURITY_INDEX     (1L)
#define PRIVILEGE_COUNT    (2L)

//
//    Access types
//

#define SET_WIDGET_COLOR        0x00000001
#define SET_WIDGET_SIZE         0x00000002
#define GET_WIDGET_COLOR        0x00000004
#define GET_WIDGET_SIZE         0x00000008
#define START_WIDGET            0x00000010
#define STOP_WIDGET             0x00000020
#define GIVE_WIDGET             0x00000040
#define TAKE_WIDGET             0x00000080


    NTSTATUS Status;

    HANDLE SimpleToken;
    HANDLE TokenWithGroups;
    HANDLE TokenWithDefaultOwner;
    HANDLE TokenWithPrivileges;
    HANDLE TokenWithDefaultDacl;

    HANDLE Token;
    HANDLE ImpersonationToken;

    HANDLE PrimaryToken;

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

    LUID DummyAuthenticationId;
    LUID SystemAuthenticationId = SYSTEM_LUID;

    TOKEN_SOURCE TestSource = {"SE: TEST", 0};

    PSID Owner;
    PSID Group;
    PACL Dacl;

    PSID TempOwner;
    PSID TempGroup;
    PACL TempDacl;





////////////////////////////////////////////////////////////////
//                                                            //
// Initialization Routine                                     //
//                                                            //
////////////////////////////////////////////////////////////////

BOOLEAN
TestTokenInitialize()
{

    TSeVariableInitialization();    // Initialize global variables


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

    InitializeObjectAttributes(
        &PrimaryTokenAttributes,
        NULL,
        OBJ_INHERIT,
        NULL,
        NULL
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
    // Use a dummy authentication ID for a while.
    //

    DummyAuthenticationId = FredLuid;


    //
    // Use a token source specific to security test
    //

    NtAllocateLocallyUniqueId( &(TestSource.SourceIdentifier) );

    DbgPrint("Done.\n");

    return TRUE;
}


BOOLEAN
CreateDAclToken()
{

    BOOLEAN CompletionStatus = TRUE;

    TOKEN_USER UserId;
    TOKEN_PRIMARY_GROUP PrimaryGroup;
    PTOKEN_GROUPS GroupIds;
    PTOKEN_PRIVILEGES Privileges;
    TOKEN_DEFAULT_DACL DefaultDacl;
    TOKEN_OWNER Owner;

    PSECURITY_DESCRIPTOR Widget1SecurityDescriptor;

    NTSTATUS AccessStatus;

    ACCESS_MASK GrantedAccess;

    PACCESS_ALLOWED_ACE AllowBarneySetColor;
    PACCESS_ALLOWED_ACE AllowFredSetColor;

    PACCESS_DENIED_ACE  DenyPebblesSetColor;

    PACCESS_ALLOWED_ACE AllowPebblesSetColor;
    PACCESS_DENIED_ACE  DenyFredSetColor;
    PACCESS_ALLOWED_ACE AllowBarneySetSize;
    PACCESS_ALLOWED_ACE AllowPebblesSetSize;

    PACCESS_ALLOWED_ACE AllowPebblesGetColor;
    PACCESS_ALLOWED_ACE AllowPebblesGetSize;

    USHORT AllowBarneySetColorLength;
    USHORT AllowFredSetColorLength;
    USHORT DenyPebblesSetColorLength;

    USHORT AllowPebblesSetColorLength;
    USHORT DenyFredSetColorLength;
    USHORT AllowBarneySetSizeLength;
    USHORT AllowPebblesSetSizeLength;

    USHORT AllowPebblesGetColorLength;
    USHORT AllowPebblesGetSizeLength;


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
    Privileges->Privileges[UNSOLICITED_INDEX].Attributes = 0;
    Privileges->Privileges[SECURITY_INDEX].Attributes = 0;

    PrimaryGroup.PrimaryGroup = FlintstoneSid;

    Status = RtlCreateAcl( DefaultDacl.DefaultDacl, DEFAULT_DACL_LENGTH, ACL_REVISION);

    ASSERT(NT_SUCCESS(Status) );

    Status = NtCreateToken(
                 &PrimaryToken,            // Handle
                 (TOKEN_ALL_ACCESS),       // DesiredAccess
                 &PrimaryTokenAttributes,  // ObjectAttributes
                 TokenPrimary,             // TokenType
                 &DummyAuthenticationId,   // Authentication LUID
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
    Privileges->Privileges[UNSOLICITED_INDEX].Attributes = 0;
    Privileges->Privileges[SECURITY_INDEX].Attributes = 0;

    PrimaryGroup.PrimaryGroup = FlintstoneSid;

    Status = RtlCreateAcl( DefaultDacl.DefaultDacl, DEFAULT_DACL_LENGTH, ACL_REVISION);

    ASSERT(NT_SUCCESS(Status) );

    Status = NtCreateToken(
                 &ImpersonationToken,      // Handle
                 (TOKEN_ALL_ACCESS),       // DesiredAccess
                 &ImpersonationTokenAttributes,  // ObjectAttributes
                 TokenImpersonation,       // TokenType
                 &DummyAuthenticationId,   // Authentication LUID
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
    } else {
        DbgPrint("********** Failed ************\n");
        DbgPrint("Status is: 0x%lx \n", Status);
        CompletionStatus = FALSE;
    }

    ASSERT(NT_SUCCESS(Status));

//
//    Attach tokens to process
//

    NtSetInformationProcess(
        NtCurrentProcess(),
        ProcessAccessToken,
        &PrimaryToken,
        sizeof( PHANDLE ));


    NtSetInformationThread(
        NtCurrentThread(),
        ThreadImpersonationToken,
        &ImpersonationToken,
        sizeof( PHANDLE ));



//  Create some ACEs

//    AllowBarneySetColor

    AllowBarneySetColorLength = (USHORT)(sizeof( ACCESS_ALLOWED_ACE ) - sizeof( ULONG ) +
                                SeLengthSid( BarneySid ));

    AllowBarneySetColor = (PVOID) TstAllocatePool ( PagedPool, AllowBarneySetColorLength );

    AllowBarneySetColor->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    AllowBarneySetColor->Header.AceSize = AllowBarneySetColorLength;
    AllowBarneySetColor->Header.AceFlags = 0;

    AllowBarneySetColor->Mask = SET_WIDGET_COLOR;

    RtlCopySid(
            SeLengthSid( BarneySid ),
            &(AllowBarneySetColor->SidStart),
            BarneySid );


//    DenyPebblesSetColor

    DenyPebblesSetColorLength = (USHORT)(sizeof( ACCESS_DENIED_ACE ) - sizeof( ULONG ) +
                                SeLengthSid( BarneySid ));

    DenyPebblesSetColor = (PVOID) TstAllocatePool ( PagedPool, DenyPebblesSetColorLength );

    DenyPebblesSetColor->Header.AceType = ACCESS_DENIED_ACE_TYPE;
    DenyPebblesSetColor->Header.AceSize = DenyPebblesSetColorLength;
    DenyPebblesSetColor->Header.AceFlags = 0;

    DenyPebblesSetColor->Mask = SET_WIDGET_COLOR;

    RtlCopySid(
            SeLengthSid( PebblesSid ),
            &(DenyPebblesSetColor->SidStart),
            PebblesSid );


//    AllowFredSetColor

    AllowFredSetColorLength = (USHORT)(sizeof( ACCESS_ALLOWED_ACE ) - sizeof( ULONG ) +
                                SeLengthSid( FredSid ));

    AllowFredSetColor = (PVOID) TstAllocatePool ( PagedPool, AllowFredSetColorLength );

    AllowFredSetColor->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    AllowFredSetColor->Header.AceSize = AllowFredSetColorLength;
    AllowFredSetColor->Header.AceFlags = 0;

    AllowFredSetColor->Mask = SET_WIDGET_COLOR;

    RtlCopySid(
            SeLengthSid( FredSid ),
            &(AllowFredSetColor->SidStart),
            FredSid );




//    AllowPebblesSetColor


    AllowPebblesSetColorLength = (USHORT)(sizeof( ACCESS_ALLOWED_ACE ) - sizeof( ULONG ) +
                                SeLengthSid( PebblesSid ));

    AllowPebblesSetColor = (PVOID) TstAllocatePool ( PagedPool, AllowPebblesSetColorLength );

    AllowPebblesSetColor->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    AllowPebblesSetColor->Header.AceSize = AllowPebblesSetColorLength;
    AllowPebblesSetColor->Header.AceFlags = 0;

    AllowPebblesSetColor->Mask = SET_WIDGET_COLOR;

    RtlCopySid(
            SeLengthSid( PebblesSid ),
            &(AllowPebblesSetColor->SidStart),
            PebblesSid );


//    DenyFredSetColor

    DenyFredSetColorLength = (USHORT)(sizeof( ACCESS_DENIED_ACE ) - sizeof( ULONG ) +
                                SeLengthSid( FredSid ));

    DenyFredSetColor = (PVOID) TstAllocatePool ( PagedPool, DenyFredSetColorLength );

    DenyFredSetColor->Header.AceType = ACCESS_DENIED_ACE_TYPE;
    DenyFredSetColor->Header.AceSize = DenyFredSetColorLength;
    DenyFredSetColor->Header.AceFlags = 0;

    DenyFredSetColor->Mask = SET_WIDGET_COLOR;

    RtlCopySid(
            SeLengthSid( FredSid ),
            &(DenyFredSetColor->SidStart),
            FredSid );

//    AllowBarneySetSize

    AllowBarneySetSizeLength = (USHORT)(sizeof( ACCESS_ALLOWED_ACE ) - sizeof( ULONG ) +
                                SeLengthSid( BarneySid ));

    AllowBarneySetSize = (PVOID) TstAllocatePool ( PagedPool, AllowBarneySetSizeLength );

    AllowBarneySetSize->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    AllowBarneySetSize->Header.AceSize = AllowBarneySetSizeLength;
    AllowBarneySetSize->Header.AceFlags = 0;

    AllowBarneySetSize->Mask = SET_WIDGET_SIZE;

    RtlCopySid(
            SeLengthSid( BarneySid ),
            &(AllowBarneySetSize->SidStart),
            BarneySid );

//    AllowPebblesSetSize

    AllowPebblesSetSizeLength = (USHORT)(sizeof( ACCESS_ALLOWED_ACE ) - sizeof( ULONG ) +
                                SeLengthSid( PebblesSid ));

    AllowPebblesSetSize = (PVOID) TstAllocatePool ( PagedPool, AllowPebblesSetSizeLength );

    AllowPebblesSetSize->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    AllowPebblesSetSize->Header.AceSize = AllowPebblesSetSizeLength;
    AllowPebblesSetSize->Header.AceFlags = 0;

    AllowPebblesSetSize->Mask = SET_WIDGET_SIZE;

    RtlCopySid(
            SeLengthSid( PebblesSid ),
            &(AllowPebblesSetSize->SidStart),
            PebblesSid );


//    AllowPebblesGetSize

    AllowPebblesGetSizeLength = (USHORT)(sizeof( ACCESS_ALLOWED_ACE ) - sizeof( ULONG ) +
                                SeLengthSid( PebblesSid ));

    AllowPebblesGetSize = (PVOID) TstAllocatePool ( PagedPool, AllowPebblesGetSizeLength );

    AllowPebblesGetSize->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    AllowPebblesGetSize->Header.AceSize = AllowPebblesGetSizeLength;
    AllowPebblesGetSize->Header.AceFlags = 0;

    AllowPebblesGetSize->Mask = SET_WIDGET_SIZE;

    RtlCopySid(
            SeLengthSid( PebblesSid ),
            &(AllowPebblesGetSize->SidStart),
            PebblesSid );


//    AllowPebblesGetColor

    AllowPebblesGetColorLength = (USHORT)(sizeof( ACCESS_ALLOWED_ACE ) - sizeof( ULONG ) +
                                SeLengthSid( PebblesSid ));

    AllowPebblesGetColor = (PVOID) TstAllocatePool ( PagedPool, AllowPebblesGetColorLength );

    AllowPebblesGetColor->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    AllowPebblesGetColor->Header.AceSize = AllowPebblesGetColorLength;
    AllowPebblesGetColor->Header.AceFlags = 0;

    AllowPebblesGetColor->Mask = SET_WIDGET_COLOR;

    RtlCopySid(
            SeLengthSid( PebblesSid ),
            &(AllowPebblesGetColor->SidStart),
            PebblesSid );

//
//    Create some ACLs that we can put into a Security Descriptor
//
    DbgBreakPoint();

//
//    Dacl
//
//  +----------------+    +----------------+   +----------------+
//  |  1st ACE       |    |  2nd ACE       |   |  3rd ACE       |
//  +----------------+    +----------------+   +----------------+
//  |  AccessAllowed |    |  AccessDenied  |   |  AccessAllowed |
//  +----------------+    +----------------+   +----------------+
//  |  BARNEY        |    |  PEBBLES       |   |  FRED          |
//  +----------------+    +----------------+   +----------------+
//  |  SetWidgeColor |    |  SetWidgeColor |   |  SetWidgeColor |
//  +----------------+    +----------------+   +----------------+
//

    Dacl = (PACL) TstAllocatePool ( PagedPool,  2048 );

    RtlCreateAcl( Dacl, 2048, ACL_REVISION);


    RtlAddAce ( Dacl,
                ACL_REVISION,
                0,
                AllowBarneySetColor,
                AllowBarneySetColorLength );

    RtlAddAce ( Dacl,
                ACL_REVISION,
                1,
                DenyPebblesSetColor,
                DenyPebblesSetColorLength );

    RtlAddAce ( Dacl,
                ACL_REVISION,
                2,
                DenyFredSetColor,
                AllowFredSetColorLength );

    DumpAcl (Dacl);





//  Create a security descriptor
//
//  Owner = Pebbles
//  Group = Flintstone
//  Dacl  = Dacl
//  Sacl  = NULL
//

    Widget1SecurityDescriptor =
        (PSECURITY_DESCRIPTOR)TstAllocatePool( PagedPool, 1024 );

    RtlCreateSecurityDescriptor( Widget1SecurityDescriptor,
                                 1 );


    RtlSetOwnerSecurityDescriptor( Widget1SecurityDescriptor,
                                   PebblesSid,
                                   FALSE );

    RtlSetGroupSecurityDescriptor( Widget1SecurityDescriptor,
                                   FlintstoneSid,
                                   FALSE );

    RtlSetDaclSecurityDescriptor( Widget1SecurityDescriptor,
                                  TRUE,
                                  Dacl,
                                  FALSE );

    RtlSetSaclSecurityDescriptor( Widget1SecurityDescriptor,
                                  FALSE,
                                  NULL,
                                  NULL );

//  See if Pebbles is allowed SET_WIDGET_COLOR (should be denied)

    Status = NtAccessCheck( Widget1SecurityDescriptor,
                   PrimaryToken,
                   (ACCESS_MASK) SET_WIDGET_COLOR,
                   &GrantedAccess,
                   &AccessStatus );

//    DbgBreakPoint();

    ASSERT(NT_SUCCESS(Status));

    ASSERT(!NT_SUCCESS(AccessStatus));

    ASSERT(GrantedAccess == NULL);


//  Update Dacl to be the following:
//
//    Dacl2
//
//  +----------------+    +----------------+   +----------------+
//  |  1st ACE       |    |  2nd ACE       |   |  3rd ACE       |
//  +----------------+    +----------------+   +----------------+
//  |  AccessAllowed |    |  AccessAllowed |   |  AccessDenied  |
//  +----------------+    +----------------+   +----------------+
//  |  BARNEY        |    |  PEBBLES       |   |  FRED          |
//  +----------------+    +----------------+   +----------------+
//  |  SetWidgeColor |    |  SetWidgeColor |   |  SetWidgeColor |
//  +----------------+    +----------------+   +----------------+
//

//  Delete 2nd Ace

    RtlDeleteAce (Dacl, 1);

    RtlAddAce ( Dacl,
                ACL_REVISION,
                1,
                AllowPebblesSetColor,
                AllowPebblesSetColorLength );

    RtlDeleteAce ( Dacl, 2 );

    RtlAddAce ( Dacl,
                ACL_REVISION,
                1,
                DenyFredSetColor,
                DenyFredSetColorLength );




//  Change the security descriptor to use updated Dacl
//
//  Owner = Pebbles
//  Group = Flintstone
//  Dacl  = Dacl2
//  Sacl  = NULL
//

    RtlSetDaclSecurityDescriptor( Widget1SecurityDescriptor,
                                  TRUE,
                                  Dacl,
                                  FALSE );

//  See if Pebbles is allowed SET_WIDGET_COLOR (should be permitted)

    Status = NtAccessCheck( Widget1SecurityDescriptor,
                            PrimaryToken,
                            (ACCESS_MASK) SET_WIDGET_COLOR,
                            &GrantedAccess,
                            &AccessStatus );


    ASSERT(NT_SUCCESS(Status));

    ASSERT(NT_SUCCESS(AccessStatus));

    ASSERT(GrantedAccess == (ACCESS_MASK)SET_WIDGET_COLOR);

//
//    Dacl3
//
//  +----------------+    +----------------+   +----------------+
//  |  1st ACE       |    |  2nd ACE       |   |  3rd ACE       |
//  +----------------+    +----------------+   +----------------+
//  |  AccessAllowed |    |  AccessAllowed |   |  AccessDenied  |
//  +----------------+    +----------------+   +----------------+
//  |  BARNEY        |    |  PEBBLES       |   |  FRED          |
//  +----------------+    +----------------+   +----------------+
//  |  SetWidgeColor |    |  SetWidgeColor |   |  SetWidgeColor |
//  +----------------+    +----------------+   +----------------+
//
//  +----------------+    +----------------+
//  |  4th ACE       |    |  5th ACE       |
//  +----------------+    +----------------+
//  |  AccessAllowed |    |  AccessAllowed |
//  +----------------+    +----------------+
//  |  BARNEY        |    |  PEBBLES       |
//  +----------------+    +----------------+
//  |  SetWidgeSize  |    |  SetWidgeSize  |
//  +----------------+    +----------------+
//


    RtlAddAce ( Dacl,
                ACL_REVISION,
                MAXULONG,
                AllowBarneySetSize,
                AllowBarneySetSizeLength );

    RtlAddAce ( Dacl,
                ACL_REVISION,
                MAXULONG,
                AllowPebblesSetSize,
                AllowPebblesSetSizeLength );

//  Change the security descriptor to use Dacl3
//
//  Owner = Pebbles
//  Group = Flintstone
//  Dacl  = Dacl3
//  Sacl  = NULL
//

    RtlSetDaclSecurityDescriptor( Widget1SecurityDescriptor,
                                  TRUE,
                                  Dacl,
                                  FALSE );

//  Request MAXIMUM_ACCESS for Pebbles.  Should get back SetWidgetSize
//  and SetWidgetColor

    Status = NtAccessCheck( Widget1SecurityDescriptor,
                            PrimaryToken,
                            (ACCESS_MASK) MAXIMUM_ALLOWED,
                            &GrantedAccess,
                            &AccessStatus );


    ASSERT(NT_SUCCESS(Status));

    ASSERT(NT_SUCCESS(AccessStatus));

    ASSERT(GrantedAccess == (ACCESS_MASK) (SET_WIDGET_COLOR | SET_WIDGET_SIZE));


//
//    Dacl4
//
//  +----------------+    +----------------+   +----------------+
//  |  1st ACE       |    |  2nd ACE       |   |  3rd ACE       |
//  +----------------+    +----------------+   +----------------+
//  |  AccessAllowed |    |  AccessAllowed |   |  AccessDenied  |
//  +----------------+    +----------------+   +----------------+
//  |  BARNEY        |    |  PEBBLES       |   |  FRED          |
//  +----------------+    +----------------+   +----------------+
//  |  SetWidgeColor |    |  SetWidgeColor |   |  SetWidgeColor |
//  +----------------+    +----------------+   +----------------+
//
//  +----------------+    +----------------+   +----------------+
//  |  4th ACE       |    |  5th ACE       |   |  6th ACE       |
//  +----------------+    +----------------+   +----------------+
//  |  AccessAllowed |    |  AccessAllowed |   |  AccessDenied  |
//  +----------------+    +----------------+   +----------------+
//  |  BARNEY        |    |  PEBBLES       |   |  PEBBLES       |
//  +----------------+    +----------------+   +----------------+
//  |  SetWidgeSize  |    |  SetWidgeSize  |   |  SetWidgeColor |
//  +----------------+    +----------------+   +----------------+
//

    RtlAddAce ( Dacl,
                ACL_REVISION,
                MAXULONG,
                DenyPebblesSetColor,
                DenyPebblesSetColorLength );

    RtlSetDaclSecurityDescriptor( Widget1SecurityDescriptor,
                                  TRUE,
                                  Dacl,
                                  FALSE );

//  Request MAXIMUM_ACCESS for Pebbles.  Should get back SetWidgetSize
//  and SetWidgetColor

    Status = NtAccessCheck( Widget1SecurityDescriptor,
                            PrimaryToken,
                            (ACCESS_MASK) MAXIMUM_ALLOWED,
                            &GrantedAccess,
                            &AccessStatus );


    ASSERT(NT_SUCCESS(Status));

    ASSERT(NT_SUCCESS(AccessStatus));

    ASSERT(GrantedAccess == (ACCESS_MASK) (SET_WIDGET_COLOR | SET_WIDGET_SIZE));


//
//    Dacl5
//
//  +----------------+    +----------------+   +----------------+
//  |  1st ACE       |    |  2nd ACE       |   |  3rd ACE       |
//  +----------------+    +----------------+   +----------------+
//  |  AccessAllowed |    |  AccessDenied  |   |  AccessDenied  |
//  +----------------+    +----------------+   +----------------+
//  |  BARNEY        |    |  PEBBLES       |   |  FRED          |
//  +----------------+    +----------------+   +----------------+
//  |  SetWidgeColor |    |  SetWidgeColor |   |  SetWidgeColor |
//  +----------------+    +----------------+   +----------------+
//
//  +----------------+    +----------------+   +----------------+
//  |  4th ACE       |    |  5th ACE       |   |  6th ACE       |
//  +----------------+    +----------------+   +----------------+
//  |  AccessAllowed |    |  AccessAllowed |   |  AccessAllowed |
//  +----------------+    +----------------+   +----------------+
//  |  BARNEY        |    |  PEBBLES       |   |  PEBBLES       |
//  +----------------+    +----------------+   +----------------+
//  |  SetWidgeSize  |    |  SetWidgeSize  |   |  SetWidgeColor |
//  +----------------+    +----------------+   +----------------+
//

    RtlDeleteAce (Dacl, 1);

    RtlAddAce ( Dacl,
                ACL_REVISION,
                1,
                DenyPebblesSetColor,
                DenyPebblesSetColorLength );

    RtlDeleteAce (Dacl, 5);

    RtlAddAce ( Dacl,
                ACL_REVISION,
                MAXULONG,
                AllowPebblesSetColor,
                AllowPebblesSetColorLength );


    DumpAcl ( Dacl );

    RtlSetDaclSecurityDescriptor( Widget1SecurityDescriptor,
                                  TRUE,
                                  Dacl,
                                  FALSE );

//  Request MAXIMUM_ACCESS for Pebbles.  Should get back SetWidgetSize

    Status = NtAccessCheck( Widget1SecurityDescriptor,
                            PrimaryToken,
                            (ACCESS_MASK) MAXIMUM_ALLOWED,
                            &GrantedAccess,
                            &AccessStatus );


    ASSERT(NT_SUCCESS(Status));

    ASSERT(NT_SUCCESS(AccessStatus));

    ASSERT(GrantedAccess == (ACCESS_MASK) SET_WIDGET_SIZE);


//
//    Dacl6
//
//  +----------------+    +----------------+   +----------------+
//  |  1st ACE       |    |  2nd ACE       |   |  3rd ACE       |
//  +----------------+    +----------------+   +----------------+
//  |  AccessAllowed |    |  AccessDenied  |   |  AccessDenied  |
//  +----------------+    +----------------+   +----------------+
//  |  BARNEY        |    |  PEBBLES       |   |  FRED          |
//  +----------------+    +----------------+   +----------------+
//  |  SetWidgeColor |    |  SetWidgeColor |   |  SetWidgeColor |
//  +----------------+    +----------------+   +----------------+
//
//  +----------------+    +----------------+   +----------------+
//  |  4th ACE       |    |  5th ACE       |   |  6th ACE       |
//  +----------------+    +----------------+   +----------------+
//  |  AccessAllowed |    |  AccessAllowed |   |  AccessAllowed |
//  +----------------+    +----------------+   +----------------+
//  |  BARNEY        |    |  PEBBLES       |   |  PEBBLES       |
//  +----------------+    +----------------+   +----------------+
//  |  SetWidgeSize  |    |  SetWidgeSize  |   |  SetWidgeColor |
//  +----------------+    +----------------+   +----------------+
//
//  +----------------+    +----------------+
//  |  7th ACE       |    |  8th ACE       |
//  +----------------+    +----------------+
//  |  AccessAllowed |    |  AccessAllowed |
//  +----------------+    +----------------+
//  |  PEBBLES       |    |  PEBBLES       |
//  +----------------+    +----------------+
//  |  GetWidgeSize  |    |  GetWidgeColor |
//  +----------------+    +----------------+
//

    RtlAddAce ( Dacl,
                ACL_REVISION,
                MAXULONG,
                AllowPebblesGetSize,
                AllowPebblesGetSizeLength );

    RtlAddAce ( Dacl,
                ACL_REVISION,
                MAXULONG,
                AllowPebblesGetColor,
                AllowPebblesGetColorLength );

    DumpAcl ( Dacl );

    RtlSetDaclSecurityDescriptor( Widget1SecurityDescriptor,
                                  TRUE,
                                  Dacl,
                                  FALSE );

//  Request MAXIMUM_ACCESS for Pebbles.  Should get back SetWidgetSize

    Status = NtAccessCheck( Widget1SecurityDescriptor,
                            PrimaryToken,
                            (ACCESS_MASK) MAXIMUM_ALLOWED,
                            &GrantedAccess,
                            &AccessStatus );


    ASSERT(NT_SUCCESS(Status));

    ASSERT(NT_SUCCESS(AccessStatus));

    ASSERT(GrantedAccess == (ACCESS_MASK) SET_WIDGET_SIZE);



    return(TRUE);


}


/////////////////////////////////////////////////////////////////////
//                                                                 //
//                                                                 //
//    Main test entry point                                        //
//                                                                 //
//                                                                 //
/////////////////////////////////////////////////////////////////////


BOOLEAN
CTAccess()
{

    BOOLEAN Result = TRUE;

    if (!TSeVariableInitialization()) {
        DbgPrint("Se:    Failed to initialize global test variables.\n");
        return FALSE;
    }

    DbgPrint("Se:   Initialization...");
    TestTokenInitialize();
    CreateDAclToken();

}


//
//  Debug support routine
//


typedef struct _STANDARD_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    PSID Sid;
} STANDARD_ACE;
typedef STANDARD_ACE *PSTANDARD_ACE;



VOID
DumpAcl (
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
    ULONG i;
    PSTANDARD_ACE Ace;

    DbgPrint("DumpAcl @ %8lx", Acl);

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

        DbgPrint(" AceHeader: %08lx ", *(PULONG)Ace);

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

            static PCHAR AceTypes[] = { "Access Allowed",
                                        "Access Denied ",
                                        "System Audit  ",
                                        "System Alarm  "
                                      };

            DbgPrint(AceTypes[Ace->Header.AceType]);
            DbgPrint("\nAccess Mask: %08lx ", Ace->Mask);

        } else {

            DbgPrint("Unknown Ace Type\n");

        }

        DbgPrint("\n");

        DbgPrint("AceSize = %d\n",Ace->Header.AceSize);
        DbgPrint("Ace Flags = ");
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


    }

}
