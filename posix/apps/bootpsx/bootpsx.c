/* $Id$
 *
 * PROJECT: ReactOS Operating System / POSIX Environment Subsystem
 *
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define NTOS_MODE_USER
#include <ntos.h>
#include <sm/helper.h>

#define RETRY_COUNT 3

/**********************************************************************
 * PsxCheckSubSystem/1
 */
NTSTATUS STDCALL
PsxCheckSubSystem (LPCSTR argv0)
{
	NTSTATUS          Status = STATUS_SUCCESS;
	UNICODE_STRING    DirectoryName = {0, 0, NULL};
	OBJECT_ATTRIBUTES DirectoryAttributes = {0};
	HANDLE            hDir = (HANDLE) 0;

	RtlInitUnicodeString (& DirectoryName, L"\\POSIX");
	InitializeObjectAttributes (& DirectoryAttributes,
				    & DirectoryName,
				    0,0,0);
	Status = NtOpenDirectoryObject (& hDir,
					DIRECTORY_TRAVERSE,
					& DirectoryAttributes);
	if(NT_SUCCESS(Status))
	{
		NtClose (hDir);
	}

	return Status;
}

/**********************************************************************
 * PsxBootstrap/1
 */
NTSTATUS STDCALL
PsxBootstrap (LPCSTR argv0)
{
	NTSTATUS       Status = STATUS_SUCCESS;
	UNICODE_STRING Program = {0, 0, NULL};
	HANDLE         SmApiPort = (HANDLE) 0;


	printf("Connecting to the SM: ");
	Status = SmConnectApiPort (NULL,
				   (HANDLE) 0,
				   IMAGE_SUBSYSTEM_UNKNOWN,
				   & SmApiPort);
	if(!NT_SUCCESS(Status))
	{
		fprintf(stderr,"\n%s: SmConnectApiPort failed with 0x%08lx\n",
				argv0, Status);
		return Status;
	}
	RtlInitUnicodeString (& Program, L"POSIX");
	Status = SmExecuteProgram (SmApiPort, & Program);
	if(STATUS_SUCCESS != Status)
	{
		fprintf(stderr, "%s: SmExecuteProgram = %08lx\n", argv0, Status);
	}
	NtClose (SmApiPort);
	return Status;
}

/**********************************************************************
 *
 *	ENTRY POINT					PUBLIC
 *
 *********************************************************************/
int main (int argc, char * argv [])
{
	NTSTATUS Status = STATUS_SUCCESS;
	INT      RetryCount = RETRY_COUNT;

	while(RetryCount > 0)
	{
		Status = PsxCheckSubSystem (argv[0]);
		if(STATUS_SUCCESS == Status)
		{
			if (RETRY_COUNT == RetryCount)
			{
				fprintf(stderr,"POSIX already booted.\n");
			}else{
				fprintf(stderr,"POSIX booted.\n");
			}
			break;
		}else{
			Status = PsxBootstrap (argv[0]);
		}
		-- RetryCount;
	}
	return NT_SUCCESS(Status) ? EXIT_SUCCESS : EXIT_FAILURE;
}
/* EOF */
