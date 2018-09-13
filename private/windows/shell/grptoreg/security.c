/****************************** Module Header ******************************\
* Module Name: security.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Handles security aspects of winlogon operation.
*
* History:
* 12-05-91 Davidc       Created - mostly taken from old winlogon.c
\***************************************************************************/

#include "sec.h"
#include <winuserp.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>


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
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
BOOL
SetWorldSecurity(
    PSID    UserSid,
    PSECURITY_DESCRIPTOR *pSecDesc,
    BOOL bCommonGroupAccess
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

    if (bCommonGroupAccess) {
        AccessMask = KEY_READ;
    }
    else {
        AccessMask = KEY_READ | KEY_WRITE | DELETE;
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
* 04-14092 JohanneC       Created
\***************************************************************************/
BOOL InitializeSecurityAttributes
    (
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    BOOL bCommonGroupAccess
    )
{
    PSECURITY_DESCRIPTOR pSecDesc;

    if (!SetWorldSecurity(NULL, &pSecDesc, bCommonGroupAccess)) {
        return(FALSE);
    }

    pSecurityAttributes->nLength = sizeof(SECURITY_ATTRIBUTES);
    pSecurityAttributes->lpSecurityDescriptor = pSecDesc;
    pSecurityAttributes->bInheritHandle = TRUE;

    return(TRUE);
}

