/* $Id: objdir.c,v 1.1 2000/03/26 22:00:06 dwelch Exp $
 *
 * DESCRIPTION: Simple LPC Server
 * PROGRAMMER:  David Welch
 */

#include <ddk/ntddk.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
   UNICODE_STRING DirectoryNameW;
   UNICODE_STRING DirectoryNameA:
   OBJECT_ATTRIBUTES ObjectAttributes;
   NTSTATUS Status;
   HANDLE DirectoryHandle;
   
   RtlInitAnsiString(&DirectoryNameA, argv[1]);
   RtlAnsiStringToUnicodeString(&DirectoryNameW,
				&DirectoryNameA,
				TRUE);
   InitializeObjectAttributes(&ObjectAttributes,
			      &DirectoryNameW,
			      0,
			      NULL,
			      NULL);
   Status = NtOpenDirectoryObject(&DirectoryHandle,
				  0,
				  &ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
	printf("Failed to open directory object (Status%x)\n", Status);
	return(EXIT_FAILURE);
     }
   
   NtClose(DirectoryHandle);

   return EXIT_SUCCESS;
}


/* EOF */
