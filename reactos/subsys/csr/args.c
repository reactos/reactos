/* $Id$
 *
 * args.c - Client/Server Runtime - command line parsing
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
 * --------------------------------------------------------------------
 */
#include "csr.h"

#define NDEBUG
#include <debug.h>

/**********************************************************************
 * NAME							PRIVATE
 * 	CsrParseCommandLine/2
 */
NTSTATUS FASTCALL CsrParseCommandLine (PPEB  Peb,
				      PCOMMAND_LINE_ARGUMENT  Argument)
{
   HANDLE                        ProcessHeap = Peb->ProcessHeap;
   PRTL_USER_PROCESS_PARAMETERS  RtlProcessParameters = RtlNormalizeProcessParams (Peb->ProcessParameters);
   INT                           i = 0;
   INT                           afterlastspace = 0;


   DPRINT("CSR: %s called\n", __FUNCTION__);

   RtlZeroMemory (Argument, sizeof (COMMAND_LINE_ARGUMENT));

   Argument->Vector = (PWSTR *) RtlAllocateHeap (ProcessHeap,
						 0,
						 (CSRP_MAX_ARGUMENT_COUNT * sizeof Argument->Vector[0]));
   if(NULL == Argument->Vector)
   {
	   DPRINT("CSR: %s: no memory for Argument->Vector\n", __FUNCTION__);
	   return STATUS_NO_MEMORY;
   }

   Argument->Buffer.Length =
   Argument->Buffer.MaximumLength =
   	RtlProcessParameters->CommandLine.Length
	+ sizeof Argument->Buffer.Buffer [0]; /* zero terminated */
   Argument->Buffer.Buffer =
	(PWSTR) RtlAllocateHeap (ProcessHeap,
				 0,
                                 Argument->Buffer.MaximumLength);
   if(NULL == Argument->Buffer.Buffer)
   {
	   DPRINT("CSR: %s: no memory for Argument->Buffer.Buffer\n", __FUNCTION__);
	   return STATUS_NO_MEMORY;
   }

   RtlCopyMemory (Argument->Buffer.Buffer,
		  RtlProcessParameters->CommandLine.Buffer,
		  RtlProcessParameters->CommandLine.Length);

   while (Argument->Buffer.Buffer [i])
     {
	if (Argument->Buffer.Buffer[i] == L' ')
	  {
	     Argument->Count ++;
	     Argument->Buffer.Buffer [i] = L'\0';
	     Argument->Vector [Argument->Count - 1] = & (Argument->Buffer.Buffer [afterlastspace]);
	     i++;
	     while (Argument->Buffer.Buffer [i] == L' ')
	     {
		i++;
	     }
	     afterlastspace = i;
	  }
	else
	  {
	     i++;
	  }
     }

   if (Argument->Buffer.Buffer [afterlastspace] != L'\0')
     {
	Argument->Count ++;
	Argument->Buffer.Buffer [i] = L'\0';
	Argument->Vector [Argument->Count - 1] = & (Argument->Buffer.Buffer [afterlastspace]);
     }
#if !defined(NDEBUG)
   for (i=0; i<Argument->Count; i++)
   {
	   DPRINT("CSR: Argument[%d] = '%S'\n", i, Argument->Vector [i]);
   }
#endif
  return STATUS_SUCCESS;
}
/**********************************************************************
 * NAME							PRIVATE
 * 	CsrFreeCommandLine/2
 */

VOID FASTCALL CsrFreeCommandLine (PPEB  Peb,
				  PCOMMAND_LINE_ARGUMENT  Argument)
{
	DPRINT("CSR: %s called\n", __FUNCTION__);

	RtlFreeHeap (Peb->ProcessHeap,
	             0,
		     Argument->Vector);
	RtlFreeHeap (Peb->ProcessHeap,
	             0,
	             Argument->Buffer.Buffer);
}
/* EOF */
