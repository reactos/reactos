/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/winlogon/winlogon.c
 * PURPOSE:         Logon 
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

//#include <ddk/ntddk.h>
#include <stdio.h>

/* FUNCTIONS *****************************************************************/

int main()
{
   char username[255];
   char password[255];
   
   printf("Winlogon\n");
   printf("login: ");
   fgets(username, 255, stdin);
   printf("Password: ");
   fgets(password, 255, stdin);
}
