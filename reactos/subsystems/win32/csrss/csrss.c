/* $Id$
 *
 * csrss.c - Client/Server Runtime subsystem
 *
 * ReactOS Operating System
 *
 * --------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * --------------------------------------------------------------------
 */

#include <csrss.h>

#define NDEBUG
#include <debug.h>

int _cdecl _main(int argc,
		 char *argv[],
		 char *envp[],
		 int DebugFlag)
{
   NTSTATUS Status = STATUS_SUCCESS;

   //PrintString("ReactOS Client/Server Run-Time (Build %s)\n",
	     //KERNEL_VERSION_BUILD_STR);

   /*==================================================================
    *	Initialize the Win32 environment subsystem server.
    *================================================================*/
   if (CsrServerInitialization (argc, argv, envp) == TRUE)
     {
	/*
	 * Terminate the current thread only.
	 */
	Status = NtTerminateThread (NtCurrentThread(), 0);
     }
   else
     {
	DisplayString (L"CSR: CsrServerInitialization failed.\n");
	/*
	 * Tell the SM we failed.
	 */
	Status = NtTerminateProcess (NtCurrentProcess(), 0);
     }
   return (int) Status;
}

/* EOF */
