// Shares.C -- Functions for manipulating NetDDE shares
#define    WINVER 0x0310
#define    NOAUTOUPDATE 1

#include <windows.h>
#include <windowsx.h>
#include "clipsrv.h"
#include "..\common\common.h"

BOOL
GetTokenHandle(
PHANDLE pTokenHandle )
{

if( !OpenThreadToken( GetCurrentThread(), TOKEN_READ, FALSE, pTokenHandle ) )
   {
   if( GetLastError() == ERROR_NO_TOKEN )
      {
      if( !OpenProcessToken( GetCurrentProcess(),
               TOKEN_READ,
               pTokenHandle ) )
         {
         return FALSE;
         }
      }
   else
      {
      return FALSE;
      }
   }
return TRUE;
}

// Purpose: Generate a self-relative SD whose ACL contains only an
//    entry for LocalSystem/GENERIC_ALL access. This SD will be used
//    in calls to CreateFile() for clipbook page files.
//
// Parameters: None
//
// Returns: Pointer to the security descriptor. This pointer may be freed.
//    Returns NULL on failure.
//
//////////////////////////////////////////////////////////////////////////
PSECURITY_DESCRIPTOR MakeLocalOnlySD(
void)
{
PSECURITY_DESCRIPTOR pSD;
PSECURITY_DESCRIPTOR pSDSelfRel = NULL;
SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
PSID sidLocal;
PACL Acl;
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

                  if (MakeSelfRelativeSD(pSD, pSDSelfRel, &dwSelfRelLen))
                     {
                     }
                  else
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

// Purpose: Create a security descriptor containing only a single
// DACL entry-- one to allow the user whose context we are running
// in GENERIC_ALL access.
//
// Parameters: None.
//
// Returns: A pointer to the security descriptor described above,
//    or NULL on failure.
PSECURITY_DESCRIPTOR CurrentUserOnlySD(
void)
{
SECURITY_DESCRIPTOR   aSD;
PSECURITY_DESCRIPTOR  pSD = NULL;
BOOL                  OK;
PACL                  TmpAcl;
PACCESS_ALLOWED_ACE   TmpAce;
DWORD                 lSD;
LONG                  DaclLength;
DWORD                 lTokenInfo;
HANDLE                hClientToken;
TOKEN_USER           *pUserTokenInfo;
SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

if (InitializeSecurityDescriptor(&aSD, SECURITY_DESCRIPTOR_REVISION)
    && !GetTokenHandle(&hClientToken))
   {
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
   if (pUserTokenInfo)
      {
      // Figure out how big a Dacl we'll need for just me to be on it.
      DaclLength = (DWORD)sizeof(ACL) +
            GetLengthSid( pUserTokenInfo->User.Sid ) +
            (DWORD)sizeof( ACCESS_ALLOWED_ACE );

      if (TmpAcl = (PACL)LocalAlloc(LMEM_FIXED, DaclLength ))
         {
         if (InitializeAcl( TmpAcl, DaclLength, ACL_REVISION ))
            {
            if (AddAccessAllowedAce( TmpAcl, ACL_REVISION,
                  GENERIC_ALL, pUserTokenInfo->User.Sid ))
               {
               if (GetAce( TmpAcl, 0, (LPVOID *)&TmpAce ))
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
               else
                  {
                  PERROR("GetAce error %d", GetLastError());
                  }
               }
            else
               {
               PERROR(TEXT("AddAccessAllowedAce fail\r\n"));
               }
            }
         else
            {
            PERROR(TEXT("InitializeAcl fail\r\n"));
            }
         LocalFree((HANDLE)TmpAcl);
         }
      else
         {
         PERROR(TEXT("LocalAllof for Acl fail\r\n"));
         }
      LocalFree((HANDLE)pUserTokenInfo);
      }
   else
      {
      PERROR(TEXT("Couldn't get usertokeninfo\r\n"));
      }
   CloseHandle(hClientToken);
   }
else
   {
   PERROR(TEXT("Couldn't get token handle or InitSD bad \r\n"));
   }
return pSD;
}

