/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/* $Id: rpcss.c,v 1.3 2002/09/08 10:23:44 chorns Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/rpcss/rpcss.c
 * PURPOSE:          RPC server
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#define UNICODE

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>

//#include "services.h"

#define NDEBUG
#include <debug.h>



/* GLOBALS ******************************************************************/



/* FUNCTIONS *****************************************************************/

void
PrintString(char* fmt,...)
{
  char buffer[512];
  va_list ap;

  va_start(ap, fmt);
  vsprintf(buffer, fmt, ap);
  va_end(ap);

  OutputDebugStringA(buffer);
}



VOID CALLBACK
ServiceMain(DWORD argc, LPTSTR *argv)
{

  PrintString("ServiceMain() called\n");


  StartEndpointMapper();


  PrintString("ServiceMain() done\n");

}


int main(int argc, char *argv[])
{
#if 0
  SERVICE_TABLE_ENTRY ServiceTable[] = {{"RpcSs", ServiceMain},{NULL, NULL}};
#endif

  HANDLE hEvent;
  NTSTATUS Status;

  PrintString("RpcSs service\n");




#if 0
  StartServiceCtrlDispatcher(ServiceTable);
#endif


  ServiceMain(0, NULL);


  hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  WaitForSingleObject(hEvent, INFINITE);

  PrintString("EventLog: done\n");

  ExitThread(0);
  return(0);
}

/* EOF */
