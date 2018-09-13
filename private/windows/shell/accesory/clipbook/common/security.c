

// Shares.C -- Functions for manipulating NetDDE shares


#define    NOAUTOUPDATE 1

#include <windows.h>
#include <windowsx.h>
#include "common.h"
#include "security.h"
#include "debugout.h"





/*
 *      GetTokenHandle
 */

BOOL GetTokenHandle(
    PHANDLE pTokenHandle )
{

    if (OpenThreadToken( GetCurrentThread(), TOKEN_READ, FALSE, pTokenHandle))
        return TRUE;



    if (GetLastError() != ERROR_NO_TOKEN)
        return FALSE;


    if (OpenProcessToken( GetCurrentProcess(), TOKEN_READ, pTokenHandle))
        return TRUE;


    return FALSE;

}







/*
 *      MakeLocalOnlySD
 *
 *  Purpose: Generate a self-relative SD whose ACL contains only an
 *     entry for LocalSystem/GENERIC_ALL access. This SD will be used
 *     in calls to CreateFile() for clipbook page files.
 *
 *  Parameters: None
 *
 *  Returns: Pointer to the security descriptor. This pointer may be freed.
 *     Returns NULL on failure.
 */

PSECURITY_DESCRIPTOR MakeLocalOnlySD (void)
{
PSECURITY_DESCRIPTOR        pSD;
PSECURITY_DESCRIPTOR        pSDSelfRel = NULL;
SID_IDENTIFIER_AUTHORITY    authNT     = SECURITY_NT_AUTHORITY;

PSID  sidLocal;
PACL  Acl;
DWORD dwAclSize;



    if (AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
             0, 0, 0, 0, 0, 0, 0, &sidLocal))
        {
        if (InitializeSecurityDescriptor(&pSD, SECURITY_DESCRIPTOR_REVISION))
            {
            // Allocate space for DACL with "System Full Control" access
            dwAclSize = sizeof(ACL)+ GetLengthSid(sidLocal) +
                  sizeof(ACCESS_ALLOWED_ACE) + 42; // 42==fudge factor
            if (Acl = (PACL)GlobalAlloc(GPTR, dwAclSize))
                {
                if (InitializeAcl(Acl, dwAclSize, ACL_REVISION))
                    {
                    // LocalSystem gets all access, nobody else gets any.
                    if (AddAccessAllowedAce(Acl, ACL_REVISION,
                          GENERIC_ALL, sidLocal))
                        {
                        if (SetSecurityDescriptorDacl(pSD, TRUE, Acl, TRUE))
                            {
                            DWORD dwSelfRelLen;

                            dwSelfRelLen = GetSecurityDescriptorLength(pSD);
                            pSDSelfRel = GlobalAlloc(GPTR, dwSelfRelLen);

                            if (!MakeSelfRelativeSD(pSD, pSDSelfRel, &dwSelfRelLen))
                                {
                                GlobalFree((HANDLE)pSDSelfRel);
                                pSDSelfRel = NULL;
                                }
                            }
                        }
                    }
                GlobalFree((HANDLE)Acl);
                }
            }
        FreeSid(sidLocal);
        }
    return(pSDSelfRel);
}

















/*
 *      CurrentUserOnlySD
 *
 *  Purpose: Create a security descriptor containing only a single
 *  DACL entry-- one to allow the user whose context we are running
 *  in GENERIC_ALL access.
 *
 *  Parameters: None.
 *
 *  Returns: A pointer to the security descriptor described above,
 *     or NULL on failure.
 */

PSECURITY_DESCRIPTOR CurrentUserOnlySD (void)
{
SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

SECURITY_DESCRIPTOR   aSD;
PSECURITY_DESCRIPTOR  pSD = NULL;
BOOL                  OK;
PACL                  TmpAcl;
PACCESS_ALLOWED_ACE   TmpAce;
DWORD                 lSD;
LONG                  DaclLength;
DWORD                 lTokenInfo;
HANDLE                hClientToken;
TOKEN_USER            *pUserTokenInfo;




    if (!InitializeSecurityDescriptor(&aSD, SECURITY_DESCRIPTOR_REVISION)
        || GetTokenHandle(&hClientToken))
        {
        PERROR(TEXT("Couldn't get token handle or InitSD bad \r\n"));
        return NULL;
        }



    // See if the token info fits in 50 bytes. If it does, fine.
    // If not, realloc to proper size and get the token info.
    pUserTokenInfo = (TOKEN_USER *)LocalAlloc( LMEM_FIXED, 50 );
    if (!GetTokenInformation( hClientToken, TokenUser,
                 (LPVOID) pUserTokenInfo, 50, &lTokenInfo ) )
       {
       LocalFree( pUserTokenInfo );
       pUserTokenInfo = (TOKEN_USER *)LocalAlloc( LMEM_FIXED, lTokenInfo );
       if (!GetTokenInformation( hClientToken, TokenUser,
               (LPVOID) pUserTokenInfo, lTokenInfo, &lTokenInfo ) )
          {
          LocalFree( pUserTokenInfo );
          pUserTokenInfo = NULL;
          }
       }


    if (!pUserTokenInfo)
        {
        PERROR(TEXT("Couldn't get usertokeninfo\r\n"));
        }
    else
        {
        // Figure out how big a Dacl we'll need for just me to be on it.
        DaclLength = (DWORD)sizeof(ACL) +
              GetLengthSid( pUserTokenInfo->User.Sid ) +
              (DWORD)sizeof( ACCESS_ALLOWED_ACE );

        if (!(TmpAcl = (PACL)LocalAlloc(LMEM_FIXED, DaclLength )))
            {
            PERROR(TEXT("LocalAllof for Acl fail\r\n"));
            }
        else
            {
            if (!InitializeAcl( TmpAcl, DaclLength, ACL_REVISION ))
                {
                PERROR(TEXT("InitializeAcl fail\r\n"));
                }
            else if (!AddAccessAllowedAce( TmpAcl, ACL_REVISION,
                   GENERIC_ALL, pUserTokenInfo->User.Sid ))
                {
                PERROR(TEXT("AddAccessAllowedAce fail\r\n"));
                }
            else if (!GetAce( TmpAcl, 0, (LPVOID *)&TmpAce))
                {
                PERROR("GetAce error %d", GetLastError());
                }
            else
                {
                TmpAce->Header.AceFlags = 0;
                OK   = SetSecurityDescriptorDacl(&aSD, TRUE, TmpAcl, FALSE);
                lSD  = GetSecurityDescriptorLength( &aSD);

                if (pSD  = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, lSD))
                    {
                    MakeSelfRelativeSD( &aSD, pSD, &lSD);

                    if( IsValidSecurityDescriptor( pSD ) )
                        {
                        LocalFree(pSD);
                        pSD = NULL;
                        }
                    else
                        {
                        PERROR(TEXT("Failed creating self-relative SD (%d)."),
                              GetLastError());
                        }
                    }
                else
                    {
                    PERROR(TEXT("LocalAlloc for pSD fail\r\n"));
                    }
                }


            LocalFree((HANDLE)TmpAcl);
            }

        LocalFree((HANDLE)pUserTokenInfo);
        }


    CloseHandle(hClientToken);



    return pSD;

}










#if 0



/*
 *      CurrentUserOnlySD
 *
 *  Purpose: Create an SD with a DACL saying "Current user- All". And
 *     nothing else. Note: This implementation is guaranteed to create
 *     a self-relative SD.
 *
 *  Returns: Pointer to the SD, or NULL on failure.
 */

PSECURITY_DESCRIPTOR CurrentUserOnlySD (void)
{
SECURITY_DESCRIPTOR     sd;
PSECURITY_DESCRIPTOR    pSD = NULL;
HANDLE                  htok;
PSID                    sid;
DWORD                   dwSidSize;
DWORD                   dwSDSize;
PACL                    pacl = NULL;
TOKEN_USER              *pusr = NULL;
PSID                    psidWorld;

SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;




    PINFO(TEXT("CUOnlySD "));

    AllocateAndInitializeSid (&WorldAuthority,
                              1,
                              SECURITY_WORLD_RID,
                              0,
                              0,
                              0,
                              0,
                              0,
                              0,
                              0,
                              &psidWorld);



    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
       {
       PERROR(TEXT("InitSD fail%ld\r\n"), GetLastError());
       goto done;
       }



    if (!GetTokenHandle(&htok))
       {
       PERROR(TEXT("GetTokenHandle fail%ld\r\n"), GetLastError());
       goto done;
       }



    // try 100 bytes, if not enough, alloc the required size.

    pusr = (TOKEN_USER *)GlobalAllocPtr(GHND, 100);

    if (!GetTokenInformation(htok, TokenUser, pusr, dwSidSize, &dwSidSize))
       {
       GlobalFree(pusr);
       pusr = (TOKEN_USER *)GlobalAlloc(GPTR, dwSidSize);
       }



    if (!pusr)
       {
       PERROR(TEXT("GlobalAllocPtr fail%ld\r\n"), GetLastError());
       goto done;
       }



    if (!GetTokenInformation (htok, TokenUser, pusr, dwSidSize, &dwSidSize))
       {
       PERROR(TEXT("GetTokinfo fail%ld\r\n"), GetLastError());
       goto done;
       }




    PrintSid (pusr->User.Sid);

    if (!SetSecurityDescriptorOwner(&sd, pusr->User.Sid, TRUE) ||
        !SetSecurityDescriptorGroup(&sd, pusr->User.Sid, TRUE))
       {
       PERROR(TEXT("Couldn't set owner/group %ld\r\n"), GetLastError());
       goto done;
       }



    pacl = (PACL) GlobalAllocPtr (GHND,
                                  sizeof(ACL)+
                                  (2 * sizeof (ACCESS_ALLOWED_ACE)) +
                                  GetLengthSid (pusr->User.Sid) +
                                  GetLengthSid (psidWorld));

    if (!pacl)
       {
       PERROR(TEXT("GlobalAllocPtr (PACL) fail%ld\r\n"), GetLastError());
       goto done;
       }



    if (!InitializeAcl(pacl,
                       sizeof(ACL) +
                       (2 * sizeof(ACCESS_ALLOWED_ACE)) +
                       GetLengthSid (pusr->User.Sid) +
                       GetLengthSid (psidWorld),
                       ACL_REVISION))
       {
       PERROR(TEXT("InitAcl fail%ld\r\n"), GetLastError());
       goto done;
       }




    if (!AddAccessAllowedAce (pacl,
                              ACL_REVISION,
                              NDDE_GUI_FULL_CONTROL,
                              pusr->User.Sid))
        // 6/25/93 - a-mgates Pages should have "userFC"
        //   access only when first created
        // &&
        //
        // AddAccessAllowedAce(pacl, ACL_REVISION,
        //    NDDE_GUI_READ,
        //    psidWorld))
       {
       PERROR(TEXT("AddAccAllAce fail %ld\r\n"), GetLastError());
       goto done;
       }



    if (!SetSecurityDescriptorDacl(&sd, TRUE, pacl, TRUE))
       {
       PERROR(TEXT("SetSDDacl fail%ld\r\n"), GetLastError());
       goto done;
       }



    dwSDSize = GetSecurityDescriptorLength(&sd);
    if (!(pSD = GlobalAllocPtr(GHND, dwSDSize)))
       {
       PERROR(TEXT("GAPtr for self-rel SD fail\r\n"));
       goto done;
       }




    if (MakeSelfRelativeSD(&sd, pSD, &dwSDSize))
       {
       PINFO(TEXT("CUOnlySD OK\r\n"));
       PrintSD(pSD);
       }
    else
       {
       PERROR(TEXT("MSelfRelSD fail %ld\r\n"),
             GetLastError());
       GlobalFreePtr(pSD);
       pSD = NULL;
       }





done:


    if (pacl) GlobalFreePtr(pacl);
    if (pusr) GlobalFreePtr(pusr);

    FreeSid (psidWorld);

    PERROR(TEXT("Returning %p\r\n"), pSD);


    return(pSD);

}

#endif // 0











#ifdef DEBUG


/*
 *      HexDumpBytes
 */

void HexDumpBytes(
    char        *pv,
    unsigned    cb)
{
char        achHex[]="0123456789ABCDEF";
char        achOut[80];
unsigned    iOut;



    iOut = 0;

    while (cb)
        {
        if (iOut >= 78)
            {
            PINFO(achOut);
            iOut = 0;
            }

        achOut[iOut++] = achHex[(*pv >> 4) & 0x0f];
        achOut[iOut++] = achHex[*pv++ & 0x0f];
        achOut[iOut]   = '\0';
        cb--;
        }


    if (iOut)
        {
        PINFO(achOut);
        }

}





/*
 *      PrintSid
 */

void PrintSid(
    PSID    sid)
{
DWORD   cSubAuth;
DWORD   i;

    PINFO(TEXT("\r\nSID: "));

    if (sid)
        {
        HexDumpBytes((char *)GetSidIdentifierAuthority(sid), sizeof(SID_IDENTIFIER_AUTHORITY));

        SetLastError(0);
        cSubAuth = *GetSidSubAuthorityCount(sid);
        if (GetLastError())
            {
            PINFO(TEXT("Invalid SID\r\n"));
            }
        else
            {
            for (i = 0;i < cSubAuth; i++)
                {
                PINFO(TEXT("-"));
                HexDumpBytes((char *)GetSidSubAuthority(sid, i), sizeof(DWORD));
                }
            PINFO(TEXT("\r\n"));
            }
        }
    else
        {
        PINFO(TEXT("NULL SID\r\n"));
        }

}






/*
 *      PrintAcl
 *
 *  Purpose: Print out the entries in an access-control list.
 */

void PrintAcl(
    PACL    pacl)
{
ACL_SIZE_INFORMATION    aclsi;
ACCESS_ALLOWED_ACE      *pace;
unsigned                i;


    if (pacl)
        {
        if (GetAclInformation (pacl, &aclsi, sizeof(aclsi), AclSizeInformation))
            {
            for (i = 0;i < aclsi.AceCount;i++)
                {
                GetAce(pacl, i, &pace);

                PINFO(TEXT("Type(%x) Flags(%x) Access(%lx)\r\nSID:"),
                      (int)pace->Header.AceType,
                      (int)pace->Header.AceFlags,
                      pace->Mask);
                PrintSid((PSID)&(pace->SidStart));
                }
            }
        }
    else
        {
        PINFO(TEXT("NULL PACL\r\n"));
        }

}






/*
 *      PrintSD
 */

void PrintSD(
    PSECURITY_DESCRIPTOR    pSD)
{
DWORD   dwRev;
WORD    wSDC;
BOOL    fDefault, fAcl;
PACL    pacl;
PSID    sid;



    if (NULL == pSD)
        {
        PINFO(TEXT("NULL sd\r\n"));
        return;
        }

    if (!IsValidSecurityDescriptor(pSD))
        {
        PINFO(TEXT("Bad SD %p"), pSD);
        return;
        }

    // Drop control info and revision
    if (GetSecurityDescriptorControl(pSD, &wSDC, &dwRev))
        {
        PINFO(TEXT("SD - Length: [%ld] Control: [%x] [%lx]\r\nGroup:"),
              GetSecurityDescriptorLength(pSD), wSDC, dwRev);
        }
    else
        {
        PINFO(TEXT("Couldn't get control\r\nGroup"));
        }

    // Show group and owner
    if (GetSecurityDescriptorGroup(pSD, &sid, &fDefault) &&
        sid &&
        IsValidSid(sid))
        {
        PrintSid(sid);
        PINFO(TEXT(" %s default.\r\nOwner:"), fDefault ? TEXT(" ") : TEXT("Not"));
        }
    else
        {
        PINFO(TEXT("Couldn't get group\r\n"));
        }

    if (GetSecurityDescriptorOwner(pSD, &sid, &fDefault) &&
        sid &&
        IsValidSid(sid))
        {
        PrintSid(sid);
        PINFO(TEXT(" %s default.\r\n"), fDefault ? TEXT(" ") : TEXT("Not"));
        }
    else
        {
        PINFO(TEXT("Couldn't get owner\r\n"));
        }

    // Print DACL and SACL
    if (GetSecurityDescriptorDacl(pSD, &fAcl, &pacl, &fDefault))
        {
        PINFO(TEXT("DACL: %s %s\r\n"), fAcl ? "Yes" : "No",
              fDefault ? "Default" : " ");
        if (fAcl)
            {
            if (pacl && IsValidAcl(pacl))
                {
                PrintAcl(pacl);
                }
            else
                {
                PINFO(TEXT("Invalid Acl %p\r\n"), pacl);
                }
            }
        }
    else
        {
        PINFO(TEXT("Couldn't get DACL\r\n"));
        }

    if (GetSecurityDescriptorSacl(pSD, &fAcl, &pacl, &fDefault))
        {
        PINFO(TEXT("SACL: %s %s\r\n"), fAcl ? "Yes" : "No", fDefault ? "Default" : " ");
        if (fAcl)
            {
            if (pacl && IsValidAcl(pacl))
                {
                PrintAcl(pacl);
                }
            else
                {
                PINFO(TEXT("Invalid ACL %p\r\n"), pacl);
                }
            }
        }
    else
        {
        PINFO(TEXT("Couldn't get SACL\r\n"));
        }

}


#else
#define PrintSid(x)
#define PrintSD(x)
#endif
