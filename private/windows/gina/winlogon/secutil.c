//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       secutil.c
//
//  Contents:   Security Utilities
//
//  Classes:
//
//  Functions:
//
//  History:    8-25-94   RichardW   Created
//
//----------------------------------------------------------------------------

#include "precomp.h"
#pragma hdrstop

//
// 'Constants' used in this module only.
//
SID_IDENTIFIER_AUTHORITY gSystemSidAuthority = SECURITY_NT_AUTHORITY;
SID_IDENTIFIER_AUTHORITY gLocalSidAuthority = SECURITY_LOCAL_SID_AUTHORITY;
PSID gLocalSid;  // Initialized in 'InitializeSecurityGlobals'
PSID gAdminSid;  // Initialized in 'InitializeSecurityGlobals'
PSID gRestrictedSid;  // Initialized in 'InitializeSecurityGlobals'

typedef struct _MYACELIST {
    DWORD   Total;
    DWORD   Active;
    DWORD   WinlogonOnly;
    MYACE * Aces;
    DWORD   TotalSids;
    DWORD   ActiveSids;
    PSID *  Sids;
} MYACELIST, * PMYACELIST;


//
// Define all access to windows objects
//

#define DESKTOP_ALL (DESKTOP_READOBJECTS     | DESKTOP_CREATEWINDOW     | \
                     DESKTOP_CREATEMENU      | DESKTOP_HOOKCONTROL      | \
                     DESKTOP_JOURNALRECORD   | DESKTOP_JOURNALPLAYBACK  | \
                     DESKTOP_ENUMERATE       | DESKTOP_WRITEOBJECTS     | \
                     DESKTOP_SWITCHDESKTOP   | STANDARD_RIGHTS_REQUIRED)

#define WINSTA_ALL  (WINSTA_ENUMDESKTOPS     | WINSTA_READATTRIBUTES    | \
                     WINSTA_ACCESSCLIPBOARD  | WINSTA_CREATEDESKTOP     | \
                     WINSTA_WRITEATTRIBUTES  | WINSTA_ACCESSGLOBALATOMS | \
                     WINSTA_EXITWINDOWS      | WINSTA_ENUMERATE         | \
                     WINSTA_READSCREEN       | \
                     STANDARD_RIGHTS_REQUIRED)

#define WINSTA_ATOMS    (WINSTA_ACCESSGLOBALATOMS | \
                         WINSTA_ACCESSCLIPBOARD )

//
// Private prototypes
//

VOID
InitializeSecurityGlobals(
    VOID
    );

PVOID
CreateAccessAllowedAce(
    PSID  Sid,
    ACCESS_MASK AccessMask,
    UCHAR AceFlags,
    UCHAR InheritFlags
    );

VOID
DestroyAce(
    PVOID   Ace
    );

PMYACELIST
CreateAceList(
    DWORD   Count);

BOOL
InitializeWinstaSecurity(
    PWINDOWSTATION  pWS);

/***************************************************************************\
* InitializeSecurity
*
* Initializes the security module
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
BOOL
InitializeSecurity(
    VOID)
{
    //
    // Set up our module globals
    //
    InitializeSecurityGlobals();

    //
    // Initialize the removable medial module
    //

    RmvInitializeRemovableMediaSrvcs();

    return TRUE;
}


/***************************************************************************\
* SetMyAce
*
* Helper routine that fills in a MYACE structure.
*
* History:
* 02-06-92 Davidc       Created
\***************************************************************************/
VOID
SetMyAce(
    PMYACE MyAce,
    PSID Sid,
    ACCESS_MASK Mask,
    UCHAR InheritFlags
    )
{
    MyAce->Sid = Sid;
    MyAce->AccessMask= Mask;
    MyAce->InheritFlags = InheritFlags;
}

/***************************************************************************\
* SetWinlogonDesktopSecurity
*
* Sets the security on the specified desktop so only winlogon can access it
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
BOOL
SetWinlogonDesktopSecurity(
    HDESK   hdesktop,
    PSID    WinlogonSid
    )
{
    MYACE   Ace[2];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    SECURITY_INFORMATION si;
    BOOL    Result;

    //
    // Define the Winlogon ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             WinlogonSid,
             DESKTOP_ALL,
             0
             );

    //
    // Add enumerate access for administrators
    //

    SetMyAce(&(Ace[AceCount++]),
             gAdminSid,
             DESKTOP_ENUMERATE | STANDARD_RIGHTS_REQUIRED,
             NO_PROPAGATE_INHERIT_ACE
             );

    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
        DebugLog((DEB_ERROR, "failed to create winlogon desktop security descriptor\n"));
        return(FALSE);
    }

    //
    // Set the DACL on the object
    //

    si = DACL_SECURITY_INFORMATION;
    Result = SetUserObjectSecurity(hdesktop, &si, SecurityDescriptor);

    //
    // Free up the security descriptor
    //

    DeleteSecurityDescriptor(SecurityDescriptor);

    //
    // Return success status
    //

    if (!Result) {
        DebugLog((DEB_ERROR, "failed to set winlogon desktop security\n"));
    }
    return(Result);
}


/***************************************************************************\
* SetUserDesktopSecurity
*
* Sets the security on the specified desktop given the logon sid passed.
*
* If UserSid = NULL, access is given only to winlogon
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
BOOL
SetUserDesktopSecurity(
    HDESK   hdesktop,
    PSID    UserSid,
    PSID    WinlogonSid
    )
{
    MYACE   Ace[4];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    SECURITY_INFORMATION si;
    BOOL    Result;
    ACCESS_MASK DesktopAccess;
    DWORD   MiserlyAccess;

    //
    // Define the Winlogon ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             WinlogonSid,
             DESKTOP_ALL,
             0
             );

    //
    // Define the Admin ACEs
    //

    MiserlyAccess = GetProfileInt( WINLOGON, RESTRICT_NONINTERACTIVE_ACCESS, 0 );

    if ( MiserlyAccess )
    {
        DesktopAccess = DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS |
                        DESKTOP_ENUMERATE |
                        DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU ;
    }
    else
    {

        DesktopAccess = DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS |
                        DESKTOP_ENUMERATE |
                        DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU |
                        GENERIC_EXECUTE ;
    }

    SetMyAce(&(Ace[AceCount++]),
             gAdminSid,
             DesktopAccess,
             0
             );

    SetMyAce(&(Ace[AceCount++]),
             gRestrictedSid,
             DESKTOP_ALL,
             0
             );
             
    //
    // Define the User ACEs
    //

    if (UserSid != NULL) {

        SetMyAce(&(Ace[AceCount++]),
                 UserSid,
                 DESKTOP_ALL,
                 0
                 );
    }

    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
        DebugLog((DEB_ERROR, "failed to create user desktop security descriptor\n"));
        return(FALSE);
    }

    //
    // Set the DACL on the object
    //

    si = DACL_SECURITY_INFORMATION;
    Result = SetUserObjectSecurity(hdesktop, &si, SecurityDescriptor);

    //
    // Free up the security descriptor
    //

    DeleteSecurityDescriptor(SecurityDescriptor);

    //
    // Return success status
    //

    if (!Result) {
        DebugLog((DEB_ERROR, "failed to set user desktop security\n"));
    }
    return(Result);
}


/***************************************************************************\
* CreateLogonSid
*
* Creates a logon sid for a new logon.
*
* If LogonId is non NULL, on return the LUID that is part of the logon
* sid is returned here.
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
PSID
CreateLogonSid(
    PLUID LogonId OPTIONAL
    )
{
    NTSTATUS Status;
    ULONG   Length;
    PSID    Sid;
    LUID    Luid;

    //
    // Generate a locally unique id to include in the logon sid
    //

    Status = NtAllocateLocallyUniqueId(&Luid);
    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "Failed to create LUID, status = 0x%lx", Status));
        return(NULL);
    }


    //
    // Allocate space for the sid and fill it in.
    //

    Length = RtlLengthRequiredSid(SECURITY_LOGON_IDS_RID_COUNT);

    Sid = (PSID)Alloc(Length);
    ASSERTMSG("Winlogon failed to allocate memory for logonsid", Sid != NULL);

    if (Sid != NULL) {

        RtlInitializeSid(Sid, &gSystemSidAuthority, SECURITY_LOGON_IDS_RID_COUNT);

        ASSERT(SECURITY_LOGON_IDS_RID_COUNT == 3);

        *(RtlSubAuthoritySid(Sid, 0)) = SECURITY_LOGON_IDS_RID;
        *(RtlSubAuthoritySid(Sid, 1 )) = Luid.HighPart;
        *(RtlSubAuthoritySid(Sid, 2 )) = Luid.LowPart;
    }


    //
    // Return the logon LUID if required.
    //

    if (LogonId != NULL) {
        *LogonId = Luid;
    }

    return(Sid);
}


/***************************************************************************\
* DeleteLogonSid
*
* Frees up memory allocated for logon sid
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
VOID
DeleteLogonSid(
    PSID Sid
    )
{
    Free(Sid);
}


/***************************************************************************\
* InitializeSecurityGlobals
*
* Initializes the various global constants (mainly Sids used in this module.
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
VOID
InitializeSecurityGlobals(
    VOID
    )
{
    NTSTATUS Status;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;


    //
    // Initialize the local sid for later
    //

    Status = RtlAllocateAndInitializeSid(
                    &gLocalSidAuthority,
                    1,
                    SECURITY_LOCAL_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &gLocalSid
                    );

    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "Failed to initialize local sid, status = 0x%lx", Status));
    }

    //
    // Initialize the admin sid for later
    //

    Status = RtlAllocateAndInitializeSid(
                    &gSystemSidAuthority,
                    2,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &gAdminSid
                    );
    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "Failed to initialize admin alias sid, status = 0x%lx", Status));
    }

    Status = RtlAllocateAndInitializeSid(
                                 & NtAuthority ,
                                 1,
                                 SECURITY_RESTRICTED_CODE_RID,
                                 0, 0, 0, 0, 0, 0, 0,
                                 &gRestrictedSid
                                 );

    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "Failed to initialize restricted sid, status = 0x%lx", Status));
    }


}


/***************************************************************************\
* EnablePrivilege
*
* Enables/disables the specified well-known privilege in the current thread
* token if there is one, otherwise the current process token.
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
BOOL
EnablePrivilege(
    ULONG Privilege,
    BOOL Enable
    )
{
    NTSTATUS Status;
    BOOLEAN WasEnabled;

    //
    // Try the thread token first
    //

    Status = RtlAdjustPrivilege(Privilege,
                                (BOOLEAN)Enable,
                                TRUE,
                                &WasEnabled);

    if (Status == STATUS_NO_TOKEN) {

        //
        // No thread token, use the process token
        //

        Status = RtlAdjustPrivilege(Privilege,
                                    (BOOLEAN)Enable,
                                    FALSE,
                                    &WasEnabled);
    }


    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "Failed to %ws privilege : 0x%lx, status = 0x%lx", Enable ? TEXT("enable") : TEXT("disable"), Privilege, Status));
        return(FALSE);
    }

    return(TRUE);
}



/***************************************************************************\
* SetUserProcessData
*
* Sets up the user process data structure for a new user.
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
BOOL
SetUserProcessData(
    PUSER_PROCESS_DATA UserProcessData,
    HANDLE  UserToken,
    PQUOTA_LIMITS Quotas OPTIONAL,
    PSID    UserSid,
    PSID    WinlogonSid
    )
{
    NTSTATUS    Status;

    //
    // Free an existing UserSid
    //
    if (UserProcessData->UserSid != NULL) {
        //
        // Don't free winlogon sid if this was a system logon (or no logon)
        //
        if (UserProcessData->UserSid != WinlogonSid) {
            DeleteLogonSid(UserProcessData->UserSid);
        }
        UserProcessData->UserSid = NULL;
    }

    //
    // Free up the logon token
    //

    if (UserProcessData->UserToken != NULL) {
        Status = NtClose(UserProcessData->UserToken);
        ASSERT(NT_SUCCESS(Status));
        UserProcessData->UserToken = NULL;
    }

    //
    // Free up any existing security descriptors
    //
    if (UserProcessData->NewProcessSD != NULL) {
        DeleteSecurityDescriptor(UserProcessData->NewProcessSD);
    }
    if (UserProcessData->NewProcessTokenSD != NULL) {
        DeleteSecurityDescriptor(UserProcessData->NewProcessTokenSD);
    }
    if (UserProcessData->NewThreadSD != NULL) {
        DeleteSecurityDescriptor(UserProcessData->NewThreadSD);
    }
    if (UserProcessData->NewThreadTokenSD != NULL) {
        DeleteSecurityDescriptor(UserProcessData->NewThreadTokenSD);
    }

    //
    // Store the new user's token and sid
    //

    ASSERT(UserSid != NULL); // should always have a non-NULL user sid

    UserProcessData->UserToken = UserToken;
    UserProcessData->UserSid = UserSid;

    //
    // Save the user's quota limits
    //

    if (ARGUMENT_PRESENT(Quotas)) {
        UserProcessData->Quotas = (*Quotas);
    }


    //
    // Set up new security descriptors
    //
#if 0
    UserProcessData->NewProcessSD = CreateUserProcessSD(
                                            UserSid,
                                            WinlogonSid);

    ASSERT(UserProcessData->NewProcessSD != NULL);

    UserProcessData->NewProcessTokenSD = CreateUserProcessTokenSD(
                                            UserSid,
                                            WinlogonSid);

    ASSERT(UserProcessData->NewProcessTokenSD != NULL);
#endif
    UserProcessData->NewThreadSD = CreateUserThreadSD(
                                            UserSid,
                                            WinlogonSid);

    ASSERT(UserProcessData->NewThreadSD != NULL);

    UserProcessData->NewThreadTokenSD = CreateUserThreadTokenSD(
                                            UserSid,
                                            WinlogonSid);

    ASSERT(UserProcessData->NewThreadTokenSD != NULL);

    return(TRUE);
}

/***************************************************************************\
* FUNCTION: SecurityChangeUser
*
* PURPOSE:  Sets up any security information for the new user.
*           This should be called whenever a user logs on or off.
*           UserLoggedOn should be set to indicate winlogon state, i.e.
*           TRUE if a real user is logged on, FALSE if this call is setting
*           our user back to system. (Note that UserToken and Sid may be
*           the winlogon token/sid on DBG machines where we allow system logon)
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
SecurityChangeUser(
    PTERMINAL pTerm,
    HANDLE Token,
    PQUOTA_LIMITS Quotas OPTIONAL,
    PSID LogonSid,
    BOOL UserLoggedOn
    )
{
    PWINDOWSTATION pWS = pTerm->pWinStaWinlogon;
    LUID luidNone = { 0, 0 };

    //
    // Save the token for this windowstation
    //

    pWS->hToken = Token;

    //
    // Set appropriate protection on windows objects
    //

    if (UserLoggedOn || g_fExecuteSetup)
    {
        AddUserToWinsta( pWS,
                         LogonSid,
                         Token );

    }
    else
    {
        RemoveUserFromWinsta( pWS,
                              pWS->UserProcessData.UserToken );
    }

    SetUserDesktopSecurity(pWS->hdeskApplication,
                           LogonSid,
                           g_WinlogonSid);

    //
    // Setup new-process data
    //

    SetUserProcessData(&pWS->UserProcessData,
                       Token,
                       Quotas,
                       LogonSid,
                       g_WinlogonSid);

    //
    // Setup the appropriate new environment
    //

    if (UserLoggedOn) {

        //
        // Do nothing to the profile or environment.  Environment and profiles
        // are all handled in wlx.c:LogonAttempt and DoStartShell
        //

        pTerm->LogoffFlags = 0;
        pTerm->TickCount = GetTickCount();

    } else {

        //
        // Restore the system environment
        //

        CloseIniFileUserMapping(pTerm);

        //
        // ResetEnvironment if we are the console or
        // we are not logging off. This prevents screen paints during logout
        //
        if ( g_Console || !pTerm->MuGlobals.fLogoffInProgress )
            ResetEnvironment(pTerm);

        SetWindowStationUser(pWS->hwinsta, &luidNone, NULL, 0);

    }


    //
    // Store whether there is a real user logged on or not
    //

    pTerm->UserLoggedOn = UserLoggedOn;

    return(TRUE);
}

/***************************************************************************\
* CreateSecurityDescriptor
*
* Creates a security descriptor containing an ACL containing the specified ACEs
*
* A SD created with this routine should be destroyed using
* DeleteSecurityDescriptor
*
* Returns a pointer to the security descriptor or NULL on failure.
*
* 02-06-92 Davidc       Created.
\***************************************************************************/

PSECURITY_DESCRIPTOR
CreateSecurityDescriptor(
    PMYACE  MyAce,
    ACEINDEX AceCount
    )
{
    NTSTATUS Status;
    ACEINDEX AceIndex;
    PACCESS_ALLOWED_ACE *Ace;
    PACL    Acl = NULL;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    ULONG   LengthAces;
    ULONG   LengthAcl;
    ULONG   LengthSd;

    //
    // Allocate space for the ACE pointer array
    //

    Ace = (PACCESS_ALLOWED_ACE *)Alloc(sizeof(PACCESS_ALLOWED_ACE) * AceCount);
    if (Ace == NULL) {
        DebugLog((DEB_ERROR, "Failed to allocated ACE array\n"));
        return(NULL);
    }

    //
    // Create the ACEs and calculate total ACE size
    //

    LengthAces = 0;
    for (AceIndex=0; AceIndex < AceCount; AceIndex ++) {
        Ace[AceIndex] = CreateAccessAllowedAce(MyAce[AceIndex].Sid,
                                               MyAce[AceIndex].AccessMask,
                                               0,
                                               MyAce[AceIndex].InheritFlags);
        if (Ace[AceIndex] == NULL) {
            DebugLog((DEB_ERROR, "Failed to allocate ace\n"));
        } else {
            LengthAces += Ace[AceIndex]->Header.AceSize;
        }
    }

    //
    // Calculate ACL and SD sizes
    //

    LengthAcl = sizeof(ACL) + LengthAces;
    LengthSd  = SECURITY_DESCRIPTOR_MIN_LENGTH;

    //
    // Create the ACL
    //

    Acl = Alloc(LengthAcl);

    if (Acl != NULL) {

        Status = RtlCreateAcl(Acl, LengthAcl, ACL_REVISION);
        ASSERT(NT_SUCCESS(Status));

        //
        // Add the ACES to the ACL and destroy the ACEs
        //

        for (AceIndex = 0; AceIndex < AceCount; AceIndex ++) {

            if (Ace[AceIndex] != NULL) {

                Status = RtlAddAce(Acl, ACL_REVISION, 0, Ace[AceIndex],
                                   Ace[AceIndex]->Header.AceSize);

                if (!NT_SUCCESS(Status)) {
                    DebugLog((DEB_ERROR, "AddAce failed, status = 0x%lx\n", Status));
                }

                DestroyAce(Ace[AceIndex]);
            }
        }

    } else {
        DebugLog((DEB_ERROR, "Failed to allocate ACL\n"));
    }

    //
    // Free the ACE pointer array
    //
    Free(Ace);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = Alloc(LengthSd);

    if (SecurityDescriptor != NULL) {

        Status = RtlCreateSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
        ASSERT(NT_SUCCESS(Status));

        //
        // Set the DACL on the security descriptor
        //
        Status = RtlSetDaclSecurityDescriptor(SecurityDescriptor, TRUE, Acl, FALSE);
        if (!NT_SUCCESS(Status)) {
            DebugLog((DEB_ERROR, "SetDACLSD failed, status = 0x%lx", Status));
        }
    } else {
        DebugLog((DEB_ERROR, "Failed to allocate security descriptor\n"));
    }

    //
    // Return with our spoils
    //
    return(SecurityDescriptor);
}


/***************************************************************************\
* DeleteSecurityDescriptor
*
* Deletes a security descriptor created using CreateSecurityDescriptor
*
* Returns TRUE on success, FALSE on failure
*
* 02-06-92 Davidc       Created.
\***************************************************************************/

BOOL
DeleteSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    NTSTATUS Status;
    PACL    Acl;
    BOOLEAN Present;
    BOOLEAN Defaulted;

    ASSERT(SecurityDescriptor != NULL);

    //
    // Get the ACL
    //
    Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
                                          &Present, &Acl, &Defaulted);
    if (NT_SUCCESS(Status)) {

        //
        // Destroy the ACL
        //
        if (Present && (Acl != NULL)) {
            Free(Acl);
        }
    } else {
        DebugLog((DEB_ERROR, "Failed to get DACL from security descriptor being destroyed, Status = 0x%lx", Status));
    }

    //
    // Destroy the Security Descriptor
    //
    Free(SecurityDescriptor);

    return(TRUE);
}


/***************************************************************************\
* CreateAccessAllowedAce
*
* Allocates memory for an ACCESS_ALLOWED_ACE and fills it in.
* The memory should be freed by calling DestroyACE.
*
* Returns pointer to ACE on success, NULL on failure
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
PVOID
CreateAccessAllowedAce(
    PSID  Sid,
    ACCESS_MASK AccessMask,
    UCHAR AceFlags,
    UCHAR InheritFlags
    )
{
    ULONG   LengthSid = RtlLengthSid(Sid);
    ULONG   LengthACE = sizeof(ACE_HEADER) + sizeof(ACCESS_MASK) + LengthSid;
    PACCESS_ALLOWED_ACE Ace;

    Ace = (PACCESS_ALLOWED_ACE)Alloc(LengthACE);
    if (Ace == NULL) {
        DebugLog((DEB_ERROR, "CreateAccessAllowedAce : Failed to allocate ace\n"));
        return NULL;
    }

    Ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    Ace->Header.AceSize = (UCHAR)LengthACE;
    Ace->Header.AceFlags = AceFlags | InheritFlags;
    Ace->Mask = AccessMask;
    RtlCopySid(LengthSid, (PSID)(&(Ace->SidStart)), Sid );

    return(Ace);
}


/***************************************************************************\
* DestroyAce
*
* Frees the memory allocate for an ACE
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
VOID
DestroyAce(
    PVOID   Ace
    )
{
    Free(Ace);
}


/***************************************************************************\
* FUNCTION: ImpersonateUser
*
* PURPOSE:  Impersonates the user by setting the users token
*           on the specified thread. If no thread is specified the token
*           is set on the current thread.
*
* RETURNS:  Handle to be used on call to StopImpersonating() or NULL on failure
*           If a non-null thread handle was passed in, the handle returned will
*           be the one passed in. (See note)
*
* NOTES:    Take care when passing in a thread handle and then calling
*           StopImpersonating() with the handle returned by this routine.
*           StopImpersonating() will close any thread handle passed to it -
*           even yours !
*
* HISTORY:
*
*   04-21-92 Davidc       Created.
*
\***************************************************************************/

HANDLE
ImpersonateUser(
    PUSER_PROCESS_DATA UserProcessData,
    HANDLE      ThreadHandle
    )
{
    NTSTATUS Status, IgnoreStatus;
    HANDLE  UserToken = UserProcessData->UserToken;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ImpersonationToken;
    BOOL ThreadHandleOpened = FALSE;

    if (ThreadHandle == NULL) {

        //
        // Get a handle to the current thread.
        // Once we have this handle, we can set the user's impersonation
        // token into the thread and remove it later even though we ARE
        // the user for the removal operation. This is because the handle
        // contains the access rights - the access is not re-evaluated
        // at token removal time.
        //

        Status = NtDuplicateObject( NtCurrentProcess(),     // Source process
                                    NtCurrentThread(),      // Source handle
                                    NtCurrentProcess(),     // Target process
                                    &ThreadHandle,          // Target handle
                                    THREAD_SET_THREAD_TOKEN,// Access
                                    0L,                     // Attributes
                                    DUPLICATE_SAME_ATTRIBUTES
                                  );
        if (!NT_SUCCESS(Status)) {
            DebugLog((DEB_ERROR, "ImpersonateUser : Failed to duplicate thread handle, status = 0x%lx", Status));
            return(NULL);
        }

        ThreadHandleOpened = TRUE;
    }


    //
    // If the usertoken is NULL, there's nothing to do
    //

    if (UserToken != NULL) {

        //
        // UserToken is a primary token - create an impersonation token version
        // of it so we can set it on our thread
        //

        InitializeObjectAttributes(
                            &ObjectAttributes,
                            NULL,
                            0L,
                            NULL,
                            UserProcessData->NewThreadTokenSD);

        SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
        SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        SecurityQualityOfService.EffectiveOnly = FALSE;

        ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;


        Status = NtDuplicateToken( UserToken,
                                   TOKEN_IMPERSONATE | TOKEN_READ,
                                   &ObjectAttributes,
                                   FALSE,
                                   TokenImpersonation,
                                   &ImpersonationToken
                                 );
        if (!NT_SUCCESS(Status)) {

            DebugLog((DEB_ERROR, "Failed to duplicate users token to create"
                     " impersonation thread, status = 0x%lx\n", Status));

            if (ThreadHandleOpened) {
                IgnoreStatus = NtClose(ThreadHandle);
                ASSERT(NT_SUCCESS(IgnoreStatus));
            }

            return(NULL);
        }



        //
        // Set the impersonation token on this thread so we 'are' the user
        //

        Status = NtSetInformationThread( ThreadHandle,
                                         ThreadImpersonationToken,
                                         (PVOID)&ImpersonationToken,
                                         sizeof(ImpersonationToken)
                                       );
        //
        // We're finished with our handle to the impersonation token
        //

        IgnoreStatus = NtClose(ImpersonationToken);
        ASSERT(NT_SUCCESS(IgnoreStatus));

        //
        // Check we set the token on our thread ok
        //

        if (!NT_SUCCESS(Status)) {

            DebugLog((DEB_ERROR, "Failed to set user impersonation token on winlogon thread, status = 0x%lx", Status));

            if (ThreadHandleOpened) {
                IgnoreStatus = NtClose(ThreadHandle);
                ASSERT(NT_SUCCESS(IgnoreStatus));
            }

            return(NULL);
        }

    }


    return(ThreadHandle);

}


/***************************************************************************\
* FUNCTION: StopImpersonating
*
* PURPOSE:  Stops impersonating the client by removing the token on the
*           current thread.
*
* PARAMETERS: ThreadHandle - handle returned by ImpersonateUser() call.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* NOTES: If a thread handle was passed in to ImpersonateUser() then the
*        handle returned was one and the same. If this is passed to
*        StopImpersonating() the handle will be closed. Take care !
*
* HISTORY:
*
*   04-21-92 Davidc       Created.
*
\***************************************************************************/

BOOL
StopImpersonating(
    HANDLE  ThreadHandle
    )
{
    NTSTATUS Status, IgnoreStatus;
    HANDLE ImpersonationToken;


    //
    // Remove the user's token from our thread so we are 'ourself' again
    //

    ImpersonationToken = NULL;

    Status = NtSetInformationThread( ThreadHandle,
                                     ThreadImpersonationToken,
                                     (PVOID)&ImpersonationToken,
                                     sizeof(ImpersonationToken)
                                   );
    //
    // We're finished with the thread handle
    //

    IgnoreStatus = NtClose(ThreadHandle);
    ASSERT(NT_SUCCESS(IgnoreStatus));


    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "Failed to remove user impersonation token from winlogon thread, status = 0x%lx", Status));
    }

    return(NT_SUCCESS(Status));
}


/***************************************************************************\
* ExecUserThread
*
* Creates a thread of the winlogon process running in the logged on user's
* context.
*
* Returns thread handle on success, NULL on failure.
*
* Thread handle returned has all access to thread.
*
* 05-04-92 Davidc   Created.
\***************************************************************************/

HANDLE ExecUserThread(
    IN PTERMINAL pTerm,
    IN LPTHREAD_START_ROUTINE lpStartAddress,
    IN LPVOID Parameter,
    IN DWORD Flags,
    OUT LPDWORD ThreadId
    )
{
    SECURITY_ATTRIBUTES saThread;
    PUSER_PROCESS_DATA UserProcessData = &pTerm->pWinStaWinlogon->UserProcessData;
    HANDLE ThreadHandle, Handle;
    BOOL Result = FALSE;
    DWORD ResumeResult, IgnoreResult;

    //
    // Initialize thread security info
    //

    saThread.nLength = sizeof(SECURITY_ATTRIBUTES);
    saThread.lpSecurityDescriptor = UserProcessData->NewThreadSD;
    saThread.bInheritHandle = FALSE;

    //
    // Create the thread suspended
    //

    ThreadHandle = CreateThread(
                        &saThread,
                        0,                          // Default Stack size
                        lpStartAddress,
                        Parameter,
                        CREATE_SUSPENDED | Flags,
                        ThreadId);

    if (ThreadHandle == NULL) {
        DebugLog((DEB_ERROR, "User thread creation failed! Error = %d\n", GetLastError()));
        return(NULL);
    }


    //
    // Switch the thread to user context.
    //

    Handle = ImpersonateUser(UserProcessData, ThreadHandle);

    if (Handle == NULL) {

        DebugLog((DEB_ERROR, "Failed to set user context on thread!\n"));

    } else {

        //
        // Should have got back the handle we passed in
        //

        ASSERT(Handle == ThreadHandle);

        //
        // Let the thread run
        //

        ResumeResult = ResumeThread(ThreadHandle);

        if (ResumeResult == -1) {
            DebugLog((DEB_ERROR, "failed to resume thread, error = %d", GetLastError()));

        } else {

            //
            // Success
            //

            Result = TRUE;

        }
    }



    if (!Result) {

        //
        // Terminate the thread
        //

        IgnoreResult = TerminateThread(ThreadHandle, 0);
        ASSERT(IgnoreResult);

        //
        // Close the thread handle
        //

        IgnoreResult = CloseHandle(ThreadHandle);
        ASSERT(IgnoreResult);

        ThreadHandle = NULL;
    }


    return(ThreadHandle);
}



/***************************************************************************\
* CreateUserProfileKeySD
*
* Creates a security descriptor to protect registry keys in the user profile
*
* History:
* 22-Dec-92 Davidc       Created
* 04-May-93 Johannec     added 3rd parameter for locked groups set in upedit.exe
\***************************************************************************/
PSECURITY_DESCRIPTOR
CreateUserProfileKeySD(
    PSID    UserSid,
    PSID    WinlogonSid,
    BOOL    AllAccess
    )
{
    MYACE   Ace[3];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;


    ASSERT(UserSid != NULL);    // should always have a non-null user sid

    //
    // Define the User ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             UserSid,
             AllAccess ? KEY_ALL_ACCESS :
               KEY_ALL_ACCESS & ~(KEY_SET_VALUE | KEY_CREATE_SUB_KEY | DELETE),
             OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE
             );

    //
    // Define the Admin ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             gAdminSid,
             KEY_ALL_ACCESS,
             OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE
             );

    //
    // Define the Winlogon ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             WinlogonSid,
             KEY_ALL_ACCESS,
             OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE
             );


    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
        DebugLog((DEB_ERROR, "failed to create user process security descriptor\n\n"));
    }

    return(SecurityDescriptor);
}

/***************************************************************************\
* CreateUserProcessTokenSD
*
* Creates a security descriptor to protect primary tokens on user processes
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
PSECURITY_DESCRIPTOR
CreateUserProcessTokenSD(
    PSID    UserSid,
    PSID    WinlogonSid
    )
{
    MYACE   Ace[2];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    ASSERT(UserSid != NULL);    // should always have a non-null user sid

    //
    // Define the User ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             UserSid,
             TOKEN_ADJUST_PRIVILEGES | TOKEN_ADJUST_GROUPS |
             TOKEN_ADJUST_DEFAULT | TOKEN_QUERY |
             TOKEN_DUPLICATE | TOKEN_IMPERSONATE | READ_CONTROL,
             0
             );

    //
    // Define the Winlogon ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             WinlogonSid,
             TOKEN_READ,
             0
             );

    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
        DebugLog((DEB_ERROR, "failed to create user process token security descriptor"));
    }

    return(SecurityDescriptor);

    DBG_UNREFERENCED_PARAMETER(WinlogonSid);
}

/***************************************************************************\
* CreateUserProcessSD
*
* Creates a security descriptor to protect user processes
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
PSECURITY_DESCRIPTOR
CreateUserProcessSD(
    PSID    UserSid,
    PSID    WinlogonSid
    )
{
    MYACE   Ace[2];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    ASSERT(UserSid != NULL);    // should always have a non-null user sid

    //
    // Define the Winlogon ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             WinlogonSid,
             PROCESS_SET_INFORMATION | // Allow primary token to be set
             PROCESS_TERMINATE | SYNCHRONIZE, // Allow screen-saver control
             0
             );

    //
    // Define the User ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             UserSid,
             PROCESS_ALL_ACCESS,
             0
             );

    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
        DebugLog((DEB_ERROR, "failed to create user process security descriptor"));
    }

    return(SecurityDescriptor);
}


/***************************************************************************\
* TestTokenForAdmin
*
* Returns TRUE if the token passed represents an admin user, otherwise FALSE
*
* The token handle passed must have TOKEN_QUERY access.
*
* History:
* 05-06-92 Davidc       Created
\***************************************************************************/
BOOL
TestTokenForAdmin(
    HANDLE Token
    )
{
    NTSTATUS    Status;
    ULONG       InfoLength;
    PTOKEN_GROUPS TokenGroupList;
    ULONG       GroupIndex;
    BOOL        FoundAdmin;

    //
    // Get a list of groups in the token
    //

    Status = NtQueryInformationToken(
                 Token,                    // Handle
                 TokenGroups,              // TokenInformationClass
                 NULL,                     // TokenInformation
                 0,                        // TokenInformationLength
                 &InfoLength               // ReturnLength
                 );

    if ((Status != STATUS_SUCCESS) && (Status != STATUS_BUFFER_TOO_SMALL)) {

        DebugLog((DEB_ERROR, "failed to get group info for admin token, status = 0x%lx\n", Status));
        return(FALSE);
    }


    TokenGroupList = Alloc(InfoLength);

    if (TokenGroupList == NULL) {
        DebugLog((DEB_ERROR, "unable to allocate memory for token groups\n"));
        return(FALSE);
    }

    Status = NtQueryInformationToken(
                 Token,                    // Handle
                 TokenGroups,              // TokenInformationClass
                 TokenGroupList,           // TokenInformation
                 InfoLength,               // TokenInformationLength
                 &InfoLength               // ReturnLength
                 );

    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "failed to query groups for admin token, status = 0x%lx\n", Status));
        Free(TokenGroupList);
        return(FALSE);
    }


    //
    // Search group list for admin alias
    //

    FoundAdmin = FALSE;

    for (GroupIndex=0; GroupIndex < TokenGroupList->GroupCount; GroupIndex++ ) {

        if (RtlEqualSid(TokenGroupList->Groups[GroupIndex].Sid, gAdminSid)) {
            FoundAdmin = TRUE;
            break;
        }
    }

    //
    // Tidy up
    //

    Free(TokenGroupList);



    return(FoundAdmin);
}


/***************************************************************************\
* SetProcessToken
*
* Set the primary token of the specified process
* If the specified token is NULL, this routine does nothing.
*
* It assumed that the handles in ProcessInformation are the handles returned
* on creation of the process and therefore have all access.
*
* Returns TRUE on success, FALSE on failure.
*
* 01-31-91 Davidc   Created.
\***************************************************************************/

BOOL
SetProcessToken(
    PTERMINAL    pTerm,
    HANDLE      hProcess,
    HANDLE      hThread,
    HANDLE      hToken
    )
{
    NTSTATUS Status, AdjustStatus;
    PROCESS_ACCESS_TOKEN PrimaryTokenInfo;
    HANDLE TokenToAssign;
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN WasEnabled;
    PSECURITY_DESCRIPTOR psd;


    //
    // Check for a NULL token. (No need to do anything)
    // The process will run in the parent process's context and inherit
    // the default ACL from the parent process's token.
    //
    if (hToken == NULL)
    {
        return(TRUE);
    }

    psd = CreateUserProcessTokenSD( pTerm->pWinStaWinlogon->UserProcessData.UserSid,
                                    g_WinlogonSid );

    if ( !psd )
    {
        return FALSE ;
    }

    //
    // A primary token can only be assigned to one process.
    // Duplicate the logon token so we can assign one to the new
    // process.
    //

    InitializeObjectAttributes(
                 &ObjectAttributes,
                 NULL,
                 0,
                 NULL,
                 psd
                 );

    Status = NtDuplicateToken(
                 hToken, // Duplicate this token
                 0,                 // Same desired access
                 &ObjectAttributes,
                 FALSE,             // EffectiveOnly
                 TokenPrimary,      // TokenType
                 &TokenToAssign     // Duplicate token handle stored here
                 );

    DeleteSecurityDescriptor(psd);

    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "SetProcessToken failed to duplicate primary token for new user process, status = 0x%lx\n", Status));
        return(FALSE);
    }

    //
    // Set the process's primary token
    //


    //
    // Enable the required privilege
    //

    Status = RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, TRUE,
                                FALSE, &WasEnabled);
    if (NT_SUCCESS(Status)) {

        PrimaryTokenInfo.Token  = TokenToAssign;
        PrimaryTokenInfo.Thread = hThread;

        Status = NtSetInformationProcess(
                    hProcess,
                    ProcessAccessToken,
                    (PVOID)&PrimaryTokenInfo,
                    (ULONG)sizeof(PROCESS_ACCESS_TOKEN)
                    );
        //
        // Restore the privilege to its previous state
        //

        AdjustStatus = RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE,
                                          WasEnabled, FALSE, &WasEnabled);
        if (!NT_SUCCESS(AdjustStatus)) {
            DebugLog((DEB_ERROR, "failed to restore assign-primary-token privilege to previous enabled state\n"));
        }

        if (NT_SUCCESS(Status)) {
            Status = AdjustStatus;
        }
    } else {
        DebugLog((DEB_ERROR, "failed to enable assign-primary-token privilege\n"));
    }

    //
    // We're finished with the token handle
    //

    CloseHandle(TokenToAssign);


    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "SetProcessToken failed to set primary token for new user process, Status = 0x%lx\n", Status));
        SetLastError(RtlNtStatusToDosError(Status));
    }

    return (NT_SUCCESS(Status));
}


/***************************************************************************\
* CreateUserThreadSD
*
* Creates a security descriptor to protect user threads in the winlogon process
*
* History:
* 05-04-92 Davidc       Created
\***************************************************************************/
PSECURITY_DESCRIPTOR
CreateUserThreadSD(
    PSID    UserSid,
    PSID    WinlogonSid
    )
{
    MYACE   Ace[2];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    ASSERT(UserSid != NULL);    // should always have a non-null user sid

    //
    // Define the Winlogon ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             WinlogonSid,
             THREAD_QUERY_INFORMATION |
             THREAD_SET_THREAD_TOKEN |
             THREAD_SUSPEND_RESUME |
             THREAD_TERMINATE | SYNCHRONIZE,
             0
             );

    //
    // Define the User ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             UserSid,
             THREAD_GET_CONTEXT |
             THREAD_QUERY_INFORMATION,
             0
             );

    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
        DebugLog((DEB_ERROR, "failed to create user process security descriptor\n"));
    }

    return(SecurityDescriptor);
}


/***************************************************************************\
* CreateUserThreadTokenSD
*
* Creates a security descriptor to protect tokens on user threads
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
PSECURITY_DESCRIPTOR
CreateUserThreadTokenSD(
    PSID    UserSid,
    PSID    WinlogonSid
    )
{
    MYACE   Ace[2];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    ASSERT(UserSid != NULL);    // should always have a non-null user sid

    //
    // Define the User ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             UserSid,
             TOKEN_ADJUST_PRIVILEGES | TOKEN_ADJUST_GROUPS |
             TOKEN_ADJUST_DEFAULT | TOKEN_QUERY |
             TOKEN_DUPLICATE | TOKEN_IMPERSONATE | READ_CONTROL,
             0
             );

    //
    // Define the Winlogon ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             WinlogonSid,
             TOKEN_ALL_ACCESS,
             0
             );

    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
        DebugLog((DEB_ERROR, "failed to create user process token security descriptor\n"));
    }

    return(SecurityDescriptor);

    DBG_UNREFERENCED_PARAMETER(WinlogonSid);
}


//+---------------------------------------------------------------------------
//
//  Function:   CreateAceList
//
//  Synopsis:   Create and initialize the ACELIST to track access to the winsta
//
//  Arguments:  [Count] --
//
//  History:    6-24-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PMYACELIST
CreateAceList(
    DWORD   Count)
{
    PMYACELIST  pList;

    pList = LocalAlloc( LMEM_FIXED, sizeof( MYACELIST ) );

    if ( pList )
    {
        pList->Aces = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                    sizeof( MYACE ) * Count );

        if ( pList->Aces )
        {
            pList->Total = Count;
            pList->Active = 0;

            pList->Sids = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                        sizeof( PSID ) * Count );

            if ( pList->Sids )
            {
                pList->TotalSids = Count;
                pList->ActiveSids = 0;

                return( pList );
            }

            return( pList );
        }

        LocalFree( pList );
    }

    return( NULL );
}


//+---------------------------------------------------------------------------
//
//  Function:   AceListAddSid
//
//  Synopsis:   Adds a SID to the list where the ace list is maintained.
//
//  Arguments:  [pList] --
//              [Sid]   --
//
//  History:    6-24-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PSID
AceListAddSid(
    PMYACELIST  pList,
    PSID        Sid )
{
    PVOID   pCopy;
    PSID    SidCopy;
    DWORD   SidSize;

    if ( pList->ActiveSids == pList->TotalSids )
    {
        pCopy = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                            sizeof( PSID ) * (pList->TotalSids * 2 )  );

        if ( pCopy )
        {
            CopyMemory( pCopy, pList->Sids, sizeof(PSID) * pList->ActiveSids );
            LocalFree( pList->Sids );
            pList->Sids = pCopy;
            pList->TotalSids = pList->TotalSids * 2 ;
        }
        else
        {
            return( NULL );
        }

    }

    SidSize = RtlLengthSid( Sid );

    SidCopy = LocalAlloc( LMEM_FIXED, SidSize );

    if ( SidCopy )
    {
        CopyMemory( SidCopy, Sid, SidSize );

        pList->Sids[ pList->ActiveSids++ ] = SidCopy ;

        return( SidCopy );
    }

    return( NULL );

}


//+---------------------------------------------------------------------------
//
//  Function:   AceListRemoveSid
//
//  Synopsis:   Removes and frees a SID from the list
//
//  Arguments:  [pList]    -- List
//              [Sid]      -- Sid to remove
//              [Absolute] -- TRUE -> pointer match, FALSE -> SID match
//
//  History:    6-24-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
AceListRemoveSid(
    PMYACELIST      pList,
    PSID            Sid,
    BOOL            Absolute )
{
    DWORD   i;
    PSID    SidToDelete;

    for ( i = 0 ; i < pList->ActiveSids ; i++ )
    {
        if ( Absolute )
        {
            if ( pList->Sids[ i ] == Sid )
            {
                break;
            }
        }
        else
        {
            if ( RtlEqualSid( pList->Sids[i], Sid ) )
            {
                break;
            }
        }

    }

    if ( i == pList->ActiveSids )
    {
        return;
    }

    SidToDelete = pList->Sids[ i ];

    pList->ActiveSids --;

    pList->Sids[ i ] = pList->Sids[ pList->ActiveSids ];

    LocalFree( SidToDelete );

}
//+---------------------------------------------------------------------------
//
//  Function:   AceListSetWinstaSecurity
//
//  Synopsis:   Applies the ACL in pList to the window station
//
//  Arguments:  [pList]   --
//              [hWinsta] --
//
//  History:    6-24-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
AceListSetWinstaSecurity(
    PMYACELIST          pList,
    DWORD               Count,
    HWINSTA             hWinsta )
{
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    SECURITY_INFORMATION si;
    BOOL    Result;

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(pList->Aces, Count );
    if (SecurityDescriptor == NULL) {
        DebugLog((DEB_ERROR, "failed to create winsta security descriptor\n"));
        return(FALSE);
    }

    //
    // Set the DACL on the object
    //

    si = DACL_SECURITY_INFORMATION;
    Result = SetUserObjectSecurity(hWinsta, &si, SecurityDescriptor);

    //
    // Free up the security descriptor
    //

    DeleteSecurityDescriptor(SecurityDescriptor);

    //
    // Return success status
    //

    if (!Result) {
        DebugLog((DEB_ERROR, "failed to set windowstation security\n"));
    }
    return(Result);
}

//+---------------------------------------------------------------------------
//
//  Function:   InitializeWinstaSecurity
//
//  Synopsis:   Initializes the window station security to winlogon/admin only
//
//  Arguments:  [pWinsta] --
//
//  History:    6-24-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
InitializeWinstaSecurity(
    PWINDOWSTATION  pWS)
{
    PMYACELIST  pList;
    DWORD       MiserlyAccess;
    ACCESS_MASK WinstaAccess, DesktopAccess;

    pList = CreateAceList( 16 );

    if ( !pList )
    {
        return( FALSE );
    }

    pWS->Acl = pList;

    //
    // Define the Winlogon ACEs
    //

    SetMyAce(& ( pList->Aces[ pList->Active ++ ]),
             g_WinlogonSid,
             WINSTA_ALL,
             NO_PROPAGATE_INHERIT_ACE
             );

    SetMyAce(& ( pList->Aces[ pList->Active ++ ]),
             g_WinlogonSid,
             GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL,
             OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE
             );

    //
    // Define the Admin ACEs
    //

    MiserlyAccess = GetProfileInt( WINLOGON, RESTRICT_NONINTERACTIVE_ACCESS, 0 );

    if ( MiserlyAccess )
    {
        WinstaAccess = WINSTA_ENUMERATE | WINSTA_READATTRIBUTES ;

        DesktopAccess = DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS |
                        DESKTOP_ENUMERATE |
                        DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU ;
    }
    else
    {
        WinstaAccess = WINSTA_ENUMERATE | WINSTA_READATTRIBUTES |
                        WINSTA_ATOMS | STANDARD_RIGHTS_EXECUTE |
                        WINSTA_EXITWINDOWS ;

        DesktopAccess = DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS |
                        DESKTOP_ENUMERATE |
                        DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU |
                        GENERIC_EXECUTE ;
    }

    SetMyAce(& ( pList->Aces[ pList->Active ++ ]),
             gAdminSid,
             WinstaAccess,
             NO_PROPAGATE_INHERIT_ACE
             );

    SetMyAce(& ( pList->Aces[ pList->Active ++ ]),
             gAdminSid,
             DesktopAccess,
             OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE
             );

    SetMyAce( &pList->Aces[ pList->Active ++ ],
              gRestrictedSid,
              WINSTA_ALL,
              NO_PROPAGATE_INHERIT_ACE );

    SetMyAce( &pList->Aces[ pList->Active ++ ],
              gRestrictedSid,
              GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL,
              OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE );

    pList->WinlogonOnly = pList->Active ;

    return( AceListSetWinstaSecurity( pList, pList->Active, pWS->hwinsta ) );

}

//+---------------------------------------------------------------------------
//
//  Function:   AddUserToWinsta
//
//  Synopsis:   Adds the specified user to the window station
//
//  Arguments:  [pWinsta]  --
//              [LogonSid] --
//              [Token]    --
//
//  History:    6-24-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
AddUserToWinsta(
    PWINDOWSTATION  pWS,
    PSID            LogonSid,
    HANDLE          Token )
{
    PTOKEN_USER     pUser;
    UCHAR           Buffer[128];
    PMYACELIST      pList;
    NTSTATUS        Status;
    ULONG           Needed;
    PSID            LogonSidCopy;
    PSID            UserSidCopy;
    PMYACE          Copy ;

    pList = pWS->Acl;

    pUser = (PTOKEN_USER) Buffer;

    Status = NtQueryInformationToken(   Token,
                                        TokenUser,
                                        pUser,
                                        sizeof(Buffer),
                                        &Needed );

    if ( !NT_SUCCESS( Status ) )
    {
        return( FALSE );
    }


    //
    // Define the User ACEs
    //

    LogonSidCopy = AceListAddSid( pList, LogonSid );

    if ( !LogonSidCopy )
    {
        return( FALSE );
    }

    UserSidCopy = AceListAddSid( pList, pUser->User.Sid );

    if ( !UserSidCopy )
    {
        AceListRemoveSid( pList, LogonSidCopy, TRUE );

        return( FALSE );
    }

    if ( pList->Active + 3 > pList->Total )
    {
        Copy = LocalAlloc( LMEM_FIXED, sizeof( MYACE ) * pList->Total * 2 );

        if ( !Copy )
        {
            return FALSE ;
        }

        CopyMemory( Copy, pList->Aces, pList->Active * sizeof( MYACE ) );
        LocalFree( pList->Aces );
        pList->Aces = Copy ;
        pList->Total = pList->Total * 2 ;
    }

    SetMyAce( &pList->Aces[ pList->Active ++ ],
              LogonSidCopy,
              WINSTA_ALL,
              NO_PROPAGATE_INHERIT_ACE );

    SetMyAce( &pList->Aces[ pList->Active ++ ],
              LogonSidCopy,
              GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL,
              OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE );

    SetMyAce( &pList->Aces[ pList->Active ++ ],
              UserSidCopy,
              WINSTA_ATOMS,
              NO_PROPAGATE_INHERIT_ACE );


    return( AceListSetWinstaSecurity( pList, pList->Active, pWS->hwinsta ) );

}

//+---------------------------------------------------------------------------
//
//  Function:   RemoveUserFromWinsta
//
//  Synopsis:   Removes a user from the window station
//
//  Arguments:  [pWinsta] --
//              [Token]   --
//
//  History:    6-24-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
RemoveUserFromWinsta(
    PWINDOWSTATION  pWS,
    HANDLE          Token )
{
    DWORD       i;
    PTOKEN_USER     pUser;
    UCHAR           Buffer[128];
    PMYACELIST      pList;
    NTSTATUS        Status;
    ULONG           Needed;

    pList = pWS->Acl;

    if ( pList->Active == 0 )
    {
        return( FALSE );
    }

    pUser = (PTOKEN_USER) Buffer;

    Status = NtQueryInformationToken(   Token,
                                        TokenUser,
                                        pUser,
                                        sizeof(Buffer),
                                        &Needed );

    if ( !NT_SUCCESS( Status ) )
    {
        return( FALSE );
    }


    for ( i = pList->Active - 1 ; i >= pList->WinlogonOnly ; i-- )
    {
        if ( RtlEqualSid( pList->Aces[i].Sid, pUser->User.Sid ) )
        {
            break;
        }
    }

    if ( i < pList->WinlogonOnly )
    {
        return( FALSE );
    }

    //
    // We add users in blocks of three, usually LogonSid, LogonSid, UserSid.
    // Thus, we delete them in threes
    //

    if ( i < 2 )
    {
        return( FALSE );
    }

    //
    // Clean up SIDs
    //

    AceListRemoveSid( pList, pList->Aces[ i ].Sid, TRUE );

    AceListRemoveSid( pList, pList->Aces[ i - 1 ].Sid, TRUE );

    //
    // If there are still more entries after this one,
    // slide them down.
    //

    if ( pList->Active > i + 1 )
    {
        MoveMemory( &pList->Aces[ i - 2 ],
                    &pList->Aces[ i + 1 ],
                    (pList->Active - i - 1) * sizeof( MYACE ) );
    }

    pList->Active -= 3;

    return( AceListSetWinstaSecurity( pList, pList->Active, pWS->hwinsta ) );

}

//+---------------------------------------------------------------------------
//
//  Function:   FastSetWinstaSecurity
//
//  Synopsis:   Allows fast toggling between "normal" and winlogon only access
//
//  Arguments:  [pWinsta]    --
//              [FullAccess] --
//
//  History:    6-24-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
FastSetWinstaSecurity(
    PWINDOWSTATION  pWS,
    BOOL            FullAccess)
{
    PMYACELIST  pList;

    pList = (PMYACELIST) pWS->Acl;

    if ( FullAccess )
    {
        return( AceListSetWinstaSecurity( pList,
                                          pList->Active,
                                          pWS->hwinsta ) );

    }
    else
    {
        return( AceListSetWinstaSecurity( pList,
                                          pList->WinlogonOnly,
                                          pWS->hwinsta ) );
    }

}
