/* $Id: smss.c 12852 2005-01-06 13:58:04Z mf $
 *
 * debug.c - Session Manager debug messages switch and router
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
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#define NTOS_MODE_USER
#include <ntos.h>
#include <rosrtl/string.h>
#include <sm/api.h>
#include "smss.h"

/* GLOBALS ***********************************************************/

HANDLE DbgSsApiPort = INVALID_HANDLE_VALUE;
HANDLE DbgUiApiPort = INVALID_HANDLE_VALUE;

/* FUNCTIONS *********************************************************/

NTSTATUS
SmInitializeDbgSs (VOID)
{
  NTSTATUS Status;
  UNICODE_STRING UnicodeString;
  OBJECT_ATTRIBUTES ObjectAttributes;


  /* Create the \DbgSsApiPort object (LPC) */
  RtlRosInitUnicodeStringFromLiteral(&UnicodeString,
		       L"\\DbgSsApiPort");
  InitializeObjectAttributes(&ObjectAttributes,
			     &UnicodeString,
			     PORT_ALL_ACCESS,
			     NULL,
			     NULL);

  Status = NtCreatePort(&DbgSsApiPort,
			&ObjectAttributes,
			0,
			0,
			0);

  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  DbgPrint("SMSS: %s: \\DbgSsApiPort created\n",__FUNCTION__);

  /* Create the \DbgUiApiPort object (LPC) */
  RtlRosInitUnicodeStringFromLiteral(&UnicodeString,
		       L"\\DbgUiApiPort");
  InitializeObjectAttributes(&ObjectAttributes,
			     &UnicodeString,
			     PORT_ALL_ACCESS,
			     NULL,
			     NULL);

  Status = NtCreatePort(&DbgUiApiPort,
			&ObjectAttributes,
			0,
			0,
			0);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  DbgPrint("SMSS: %s: \\DbgUiApiPort created\n",__FUNCTION__);

  return STATUS_SUCCESS;
}

/* EOF */

