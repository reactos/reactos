/* $Id: services.c,v 1.7 2002/09/07 15:13:09 chorns Exp $
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

/* NOTE:
 * - Services.exe is NOT a native application, it is a GUI app.
 */

/* INCLUDES *****************************************************************/

#define NTOS_USER_MODE
#include <ntos.h>
#include <windows.h>
#include <tchar.h>
#include "services.h"

#define DBG
#define NDEBUG
#include <debug.h>



/* GLOBALS ******************************************************************/

#define PIPE_BUFSIZE 1024
#define PIPE_TIMEOUT 1000


/* FUNCTIONS *****************************************************************/

void
PrintString(char* fmt,...)
{
#ifdef DBG
   char buffer[512];
   va_list ap;

   va_start(ap, fmt);
   vsprintf(buffer, fmt, ap);
   va_end(ap);

   OutputDebugStringA(buffer);
#endif
}


BOOL
ScmCreateStartEvent(PHANDLE StartEvent)
{
  HANDLE hEvent;

  hEvent = CreateEvent(NULL,
		       TRUE,
		       FALSE,
		       _T("SvcctrlStartEvent_A3725DX"));
  if (hEvent == NULL)
    {
      if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
	  hEvent = OpenEvent(EVENT_ALL_ACCESS,
			     FALSE,
			     _T("SvcctrlStartEvent_A3725DX"));
	  if (hEvent == NULL)
	    {
	      return(FALSE);
	    }
	}
      else
	{
	  return(FALSE);
	}
    }

  *StartEvent = hEvent;

  return(TRUE);
}


BOOL
ScmNamedPipeHandleRequest(
  PVOID Request,
  DWORD RequestSize,
	PVOID Reply,
	LPDWORD ReplySize)
{
  DbgPrint("SCM READ: %s\n", Request);

  *ReplySize = 0;

  return FALSE;
}


DWORD
WINAPI
ScmNamedPipeThread(
  LPVOID Context)
{
  CHAR chRequest[PIPE_BUFSIZE];
  CHAR chReply[PIPE_BUFSIZE];
  DWORD cbReplyBytes;
  DWORD cbBytesRead;
  DWORD cbWritten;
  BOOL fSuccess;
  HANDLE hPipe;

  DPRINT("Accepting SCM commands through named pipe\n");
   
  hPipe = (HANDLE)Context;

  for (;;)
  {
	  fSuccess = ReadFile(
      hPipe,
		  &chRequest,
			PIPE_BUFSIZE,
      &cbBytesRead,
			NULL);
	  if (!fSuccess || cbBytesRead == 0)
    {
	    break;
    }

	  if (ScmNamedPipeHandleRequest(&chRequest, cbBytesRead, &chReply, &cbReplyBytes))
    {
	    fSuccess = WriteFile(
        hPipe,
	    	&chReply,
	    	cbReplyBytes,
	    	&cbWritten,
	    	NULL);
	    if (!fSuccess || cbReplyBytes != cbWritten)
      {
	      break;
      }
    }
  }

  FlushFileBuffers(hPipe);
  DisconnectNamedPipe(hPipe);
  CloseHandle(hPipe);

  return ERROR_SUCCESS;
}


BOOL ScmCreateNamedPipe(VOID)
{
  DWORD dwThreadId;
  BOOL fConnected;
  HANDLE hThread;
  HANDLE hPipe;

  hPipe = CreateNamedPipe("\\\\.\\pipe\\Ntsvcs",
			  PIPE_ACCESS_DUPLEX,
			  PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
			  PIPE_UNLIMITED_INSTANCES,
			  PIPE_BUFSIZE,
			  PIPE_BUFSIZE,
			  PIPE_TIMEOUT,
			  NULL);
  if (hPipe == INVALID_HANDLE_VALUE)
    {
      DPRINT("CreateNamedPipe() failed (%d)\n", GetLastError());
      return(FALSE);
    }

  fConnected = ConnectNamedPipe(hPipe,
				NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
  if (fConnected)
    {
      DPRINT("Pipe connected\n");

      hThread = CreateThread(NULL,
			     0,
			     ScmNamedPipeThread,
			     (LPVOID)hPipe,
			     0,
			     &dwThreadId);
      if (!hThread)
	{
	  DPRINT("Could not create thread (%d)\n", GetLastError());

	  DisconnectNamedPipe(hPipe);
	  CloseHandle(hPipe);
	  return(FALSE);
	}
    }
  else
    {
      DPRINT("Pipe not connected\n");

      CloseHandle(hPipe);
      return FALSE;
    }

  return TRUE;
}


int STDCALL
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
{
  HANDLE hScmStartEvent;
  HANDLE hEvent;
  NTSTATUS Status;

  PrintString("Service Control Manager\n");

  /* Create start event */
  if (!ScmCreateStartEvent(&hScmStartEvent))
    {
      PrintString("SERVICES: Failed to create start event\n");
      ExitThread(0);
    }


  /* FIXME: more initialization */


  /* Create the service database */
  Status = ScmCreateServiceDataBase();
  if (!NT_SUCCESS(Status))
    {
      PrintString("ScmCreateServiceDataBase() failed (Status %lx)\n", Status);
      ExitThread(0);
    }

  /* Update service database */
  ScmGetBootAndSystemDriverState();

#if 0
   /* Create named pipe */
   if (!ScmCreateNamedPipe())
     {
	PrintString("SERVICES: Failed to create named pipe\n");
	ExitThread(0);
     }
#endif
   /* FIXME: create listener thread for pipe */


  /* FIXME: register process as service process */
//  RegisterServiceProcess();

  PrintString("SERVICES: Initialized.\n");

  /* Signal start event */
  SetEvent(hScmStartEvent);

  /* FIXME: register event handler (used for system shutdown) */
//  SetConsoleCtrlHandler(...);


  /* Start auto-start services */
  ScmAutoStartServices();

  /* FIXME: more to do ? */


  PrintString("SERVICES: Running.\n");

  hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  WaitForSingleObject(hEvent, INFINITE);
#if 0
    for (;;)
      {
	NtYieldExecution();
      }
#endif

  PrintString("SERVICES: Finished.\n");

  ExitThread(0);
  return(0);
}

/* EOF */
