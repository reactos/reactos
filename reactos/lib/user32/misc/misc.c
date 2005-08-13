/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/misc.c
 * PURPOSE:         Misc
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * UPDATE HISTORY:
 *      19-11-2003  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
DWORD
STDCALL
GetGuiResources(
  HANDLE hProcess,
  DWORD uiFlags)
{
  return NtUserGetGuiResources(hProcess, uiFlags);
}


/*
 * Private calls for CSRSS
 */
VOID
STDCALL
PrivateCsrssManualGuiCheck(LONG Check)
{
  NtUserManualGuiCheck(Check);
}

VOID
STDCALL
PrivateCsrssInitialized(VOID)
{
  NtUserCallNoParam(NOPARAM_ROUTINE_CSRSS_INITIALIZED);
}


/*
 * @implemented
 */
BOOL
STDCALL
RegisterLogonProcess(DWORD dwProcessId, BOOL bRegister)
{
  return NtUserCallTwoParam(dwProcessId,
			    (DWORD)bRegister,
			    TWOPARAM_ROUTINE_REGISTERLOGONPROC);
}

/*
 * @implemented
 */
BOOL
STDCALL
SetLogonNotifyWindow (HWND Wnd, HWINSTA WinSta)
{
  /* Maybe we should call NtUserSetLogonNotifyWindow and let that one inform CSRSS??? */
  CSR_API_MESSAGE Request;
  ULONG CsrRequest;
  NTSTATUS Status;

  CsrRequest = MAKE_CSR_API(SET_LOGON_NOTIFY_WINDOW, CSR_GUI);
  Request.Data.SetLogonNotifyWindowRequest.LogonNotifyWindow = Wnd;

  Status = CsrClientCallServer(&Request,
			       NULL,
                   CsrRequest,
			       sizeof(CSR_API_MESSAGE));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return(FALSE);
    }

  return(TRUE);
}

/*
 * @implemented
 */
BOOL WINAPI
UpdatePerUserSystemParameters(
   DWORD dwReserved,
   BOOL bEnable)
{
   return NtUserUpdatePerUserSystemParameters(dwReserved, bEnable);
}
