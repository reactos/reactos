/****************************** Module Header ******************************\
* Module Name: security.c
*
* Copyright (c) 1992, Microsoft Corporation
*
* Handles security aspects of progman operation.
*
* History:
* 01-16-92 JohanneC       Created - mostly taken from old winlogon.c
\***************************************************************************/

#include "sec.h"
#include <winuserp.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <lm.h>


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
* SetWorldSecurity
*
* Sets the security given the logon sid passed.
*
* If the UserSid = NULL, no access is given to anyone other than world
* Users will have read access and if bWriteAccess is TRUE, they will also
* have write access.
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 04-16-91 Johannec       Created
\***************************************************************************/
BOOL
SetWorldSecurity(
    PSID    UserSid,
    PSECURITY_DESCRIPTOR *pSecDesc,
    BOOL bWriteAccess
    )
{
    MYACE   Ace[4];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PSID    WorldSid = NULL;
    PSID    AdminAliasSid = NULL;
    PSID    PowerUserAliasSid = NULL;
    PSID    SystemOpsAliasSid = NULL;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    NTSTATUS Status;
    ACCESS_MASK AccessMask;

    // Create the world Sid
    Status = RtlAllocateAndInitializeSid(
                         &WorldSidAuthority,
                         1,                   // Sub authority count
                         SECURITY_WORLD_RID,  // Sub authorities
                         0, 0, 0, 0, 0, 0, 0,
                         &WorldSid);

    if (!NT_SUCCESS(Status)) {
        DbgOnlyPrint("progman failed to allocate memory for world sid\n");
        return(FALSE);
    }

    Status = RtlAllocateAndInitializeSid(
                         &NtAuthority,
                         2,                            // Sub authority count
                         SECURITY_BUILTIN_DOMAIN_RID,  // Sub authority[0]
                         DOMAIN_ALIAS_RID_ADMINS,      // Sub authority[1]
                         0, 0, 0, 0, 0, 0,
                         &AdminAliasSid);

    Status = RtlAllocateAndInitializeSid(
                         &NtAuthority,
                         2,                            // Sub authority count
                         SECURITY_BUILTIN_DOMAIN_RID,  // Sub authority[0]
                         DOMAIN_ALIAS_RID_POWER_USERS, // Sub authority[1]
                         0, 0, 0, 0, 0, 0,
                         &PowerUserAliasSid);

    Status = RtlAllocateAndInitializeSid(
                         &NtAuthority,
                         2,                            // Sub authority count
                         SECURITY_BUILTIN_DOMAIN_RID,  // Sub authority[0]
                         DOMAIN_ALIAS_RID_SYSTEM_OPS,  // Sub authority[1]
                         0, 0, 0, 0, 0, 0,
                         &SystemOpsAliasSid);


    if (!NT_SUCCESS(Status)) {
        DbgOnlyPrint("progman failed to allocate memory for admin sid\n");
        return(FALSE);
    }



    //
    // Define the World ACEs
    //

    if (bWriteAccess) {
        AccessMask = KEY_READ | KEY_WRITE | DELETE;
    }
    else {
        AccessMask = KEY_READ;
    }

    SetMyAce(&(Ace[AceCount++]),
             WorldSid,
	          AccessMask,
             NO_PROPAGATE_INHERIT_ACE
             );

    //
    // Define the Admins ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             AdminAliasSid,
             GENERIC_ALL,
             NO_PROPAGATE_INHERIT_ACE
             );

    //
    // Define the Power Users ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             PowerUserAliasSid,
             GENERIC_ALL,
             NO_PROPAGATE_INHERIT_ACE
             );

    //
    // Define the System Operators ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             SystemOpsAliasSid,
             GENERIC_ALL,
             NO_PROPAGATE_INHERIT_ACE
             );

    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
        DbgOnlyPrint("Progman failed to create security descriptor\n\r");
        return(FALSE);
    }

#if 0
// Keep security descriptor global
// delete only when exiting the program

    //
    // Free up the security descriptor
    //

    DeleteSecurityDescriptor(SecurityDescriptor);
#endif

    //
    // Return success status
    //

    *pSecDesc = SecurityDescriptor;
    return(TRUE);
}

/***************************************************************************\
* InitializeSecurityAttributes
*
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 04-14-92 JohanneC       Created
\***************************************************************************/
BOOL InitializeSecurityAttributes(PSECURITY_ATTRIBUTES pSecurityAttributes,
                                  BOOL bWriteAccess)
{
    PSECURITY_DESCRIPTOR pSecDesc;

    if (!SetWorldSecurity(NULL, &pSecDesc, bWriteAccess)) {
        return(FALSE);
    }

    pSecurityAttributes->nLength = sizeof(SECURITY_ATTRIBUTES);
    pSecurityAttributes->lpSecurityDescriptor = pSecDesc;
    pSecurityAttributes->bInheritHandle = TRUE;

    return(TRUE);
}

BOOL
TestTokenForAdmin(
    HANDLE Token
    );

/***************************************************************************\
* TestUserForAdmin
*
*
* Returns TRUE if the current user is part of the ADMIN group,
* FALSE otherwise
*
* History:
* 07-15-92 JohanneC       Created
\***************************************************************************/
BOOL TestUserForAdmin()
{
    BOOL UserIsAdmin = FALSE;
    HANDLE Token;
#if 0
    ACCESS_MASK GrantedAccess;
    GENERIC_MAPPING GenericMapping;
    PPRIVILEGE_SET pPrivilegeSet;
    DWORD dwPrivilegeSetLength;
    MYACE   Ace[1];
    PSECURITY_DESCRIPTOR pSecDesc;
    NTSTATUS Status;
#endif
    PSID    AdminAliasSid = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    //
    // Get the token of the current process.
    //

    if (!OpenProcessToken(
                          GetCurrentProcess(),
                          TOKEN_QUERY,
                          &Token
                         ) ) {
        DbgOnlyPrint("Progman: Can't open own process token for token_query access\n\r");
        return(FALSE);
    }

#if 0

// not working because of error = STATUS_NO_IMPERSONATION_TOKEN
   so use the code from winlogon instead (see #else)

    Status = RtlAllocateAndInitializeSid(
                         &NtAuthority,
                         2,                            // Sub authority count
                         SECURITY_BUILTIN_DOMAIN_RID,  // Sub authority[0]
                         DOMAIN_ALIAS_RID_ADMINS,      // Sub authority[1]
                         0, 0, 0, 0, 0, 0,
                         &AdminAliasSid);

    if (!NT_SUCCESS(Status)) {
        DbgOnlyPrint(TEXT("progman failed to allocate memory for admin sid\n"));
        goto Exit;
    }

    //
    // Define the Admins ACEs
    //

    SetMyAce(&(Ace[0]),
             AdminAliasSid,
             GENERIC_ALL,
             NO_PROPAGATE_INHERIT_ACE
             );

    //
    // Create the security descriptor
    //

    pSecDesc = CreateSecurityDescriptor(Ace, 1);
    if (pSecDesc == NULL) {
        DbgOnlyPrint(TEXT("Progman failed to create security descriptor\n\r"));
        goto Exit;
    }

    //
    // Allocate memory for the PrivilegeSet buffer
    //

    dwPrivilegeSetLength = 256;
    pPrivilegeSet = Alloc(dwPrivilegeSetLength);
    if (!pPrivilegeSet) {
        DbgOnlyPrint(TEXT("Progman: Can't alloc memory for privilege set\n\r"));
        goto FreeSecDesc;
    }

    //
    // Test if user has admin privileges.
    //

    if(!AccessCheck(pSecDesc,
                Token,
                STANDARD_RIGHTS_ALL,
                &GenericMapping,
                pPrivilegeSet,
                &dwPrivilegeSetLength,
                &GrantedAccess,
                &UserIsAdmin) ){

        DbgOnlyPrint(TEXT("Progman: AccessCheck failed, error = %d\n\r"), GetLastError());
    }


    //
    // Free up the the PrivilegeSet
    //

    Free(pPrivilegeSet);

FreeSecDesc:

    //
    // Free up the security descriptor
    //

    DeleteSecurityDescriptor(pSecDesc);

#else

    UserIsAdmin = TestTokenForAdmin(Token);

#endif

//Exit:

    //
    // We are finished with the token.
    //

    CloseHandle(Token);

    return(UserIsAdmin);
}
/***************************************************************************\
* TestTokenForAdmin
*
* Returns TRUE if the token passed represents an admin user, otherwise FALSE
*
* The token handle passed must have TOKEN_QUERY access.
*
* History:
* 08-01-92 JohanneC     Extracted code from winlogon
* 05-06-92 Davidc       Created
\***************************************************************************/
BOOL
TestTokenForAdmin(
    HANDLE Token
    )
{
    NTSTATUS    Status;
    DWORD       InfoLength;
    PTOKEN_GROUPS TokenGroupList;
    DWORD       GroupIndex;
    PSID        AdminSid;
    BOOL        FoundAdmin;
SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

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

        DbgOnlyPrint("Winlogon failed to get group info for admin token, status = 0x%lx\n", Status);
        return(FALSE);
    }


    TokenGroupList = Alloc(InfoLength);

    if (TokenGroupList == NULL) {
        DbgOnlyPrint("Winlogon unable to allocate memory for token groups\n");
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
        DbgOnlyPrint("Winlogon failed to query groups for admin token, status = 0x%lx\n", Status);
        Free(TokenGroupList);
        return(FALSE);
    }



    //
    // Create the admin sid
    //

    Status = RtlAllocateAndInitializeSid(
                    &SystemSidAuthority, 2,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &AdminSid);

    if (!NT_SUCCESS(Status)) {
        DbgOnlyPrint("Winlogon failed to initialize admin alias sid\n");
        Free(TokenGroupList);
        return(FALSE);
    }


    //
    // Search group list for admin alias
    //

    FoundAdmin = FALSE;

    for (GroupIndex=0; GroupIndex < TokenGroupList->GroupCount; GroupIndex++ ) {

        if (RtlEqualSid(TokenGroupList->Groups[GroupIndex].Sid, AdminSid)) {
            FoundAdmin = TRUE;
            break;
        }
    }

    //
    // Tidy up
    //

    RtlFreeSid(AdminSid);
    Free(TokenGroupList);



    return(FoundAdmin);
}


