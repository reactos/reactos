/* $Id: services.c,v 1.1 2000/03/26 22:00:09 dwelch Exp $
 *
 * service control manager
 * 
 * ReactOS Operating System
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
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <services/services.h>

VOID ServicesInitialization(VOID)
{
   
}

/* Native process' entry point */

VOID NtProcessStartup(PPEB Peb)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE ServicesInitEvent;
   UNICODE_STRING UnicodeString;
   NTSTATUS Status;
   
   DisplayString(L"Service Control Manager\n");
   
   RtlInitUnicodeString(&UnicodeString,
			L"\\ServicesInitDone");
   InitializeObjectAttributes(&ObjectAttributes,
			      &UnicodeString,
			      EVENT_ALL_ACCESS,
			      0,
			      NULL);
   Status = NtOpenEvent(&ServicesInitEvent,
			EVENT_ALL_ACCESS,
			&ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("SERVICES: Failed to open notification event\n");
     }
   
   Status = ServicesInitialization ();
   
   if (!NT_SUCCESS(Status))
     {
	DisplayString( L"SERVICES: Subsystem failed to initialize.\n" );
	
	NtTerminateProcess(NtCurrentProcess(), Status);
     }

   
   DisplayString( L"CSR: Subsystem initialized.\n" );

   NtSetEvent(ServicesInitEvent, NULL);
   NtTerminateThread(NtCurrentThread(), STATUS_SUCCESS);
}

/* EOF */
