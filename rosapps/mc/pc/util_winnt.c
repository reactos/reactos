/* Utilities - Windows NT specific utilities (not in Win95)
   Copyright (C) 1994, 1995, 1996 the Free Software Foundation.

   Written 1996 by Juan Grigera<grigera@isis.unlp.edu.ar>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
*/
#include <config.h>
#include <windows.h>
#include "util_win32.h"
#include "trace_nt.h"


/* int winnt_IsAdministrator() - Determines whether user has Administrator (root)
                   priviledges.
   Return: 1 if administrator
   	   0 if not

   Note:  Code taken from MSKbase Number: Q118626.
   
   To determine whether or not a user is an administrator, you need to examine
   the user's access token with GetTokenInformation(). The access token
   represents the user's privileges and the groups to which the user belongs.
*/

int winnt_IsAdministrator()
{
   HANDLE hAccessToken;
   UCHAR InfoBuffer[1024];
   PTOKEN_GROUPS ptgGroups = (PTOKEN_GROUPS)InfoBuffer;
   DWORD dwInfoBufferSize;
   PSID psidAdministrators;
   SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
   UINT x;
   BOOL bSuccess;

   if(!OpenProcessToken(GetCurrentProcess(),TOKEN_READ,&hAccessToken))
      return 0;

   bSuccess = GetTokenInformation(hAccessToken,TokenGroups,InfoBuffer,
      1024, &dwInfoBufferSize);

   CloseHandle(hAccessToken);

   if( !bSuccess )
      return 0;

   if(!AllocateAndInitializeSid(&siaNtAuthority, 2,
      SECURITY_BUILTIN_DOMAIN_RID,
      DOMAIN_ALIAS_RID_ADMINS,
      0, 0, 0, 0, 0, 0,
      &psidAdministrators))
      return 0;

   bSuccess = 0;
   for(x=0;x<ptgGroups->GroupCount;x++) {
      if( EqualSid(psidAdministrators, ptgGroups->Groups[x].Sid) ) {
         bSuccess = 1;
         break;
      }
   }
   FreeSid(psidAdministrators);
   return bSuccess;
}


int geteuid ()	
{
    if (winnt_IsAdministrator())
	return 0;
    return 1;
}

