/* $Id: services.c,v 1.2 2001/01/20 18:39:35 ekohl Exp $
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

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>

#include <services/services.h>

/* GLOBALS ******************************************************************/



/* FUNCTIONS *****************************************************************/

void PrintString (char* fmt,...)
{
   char buffer[512];
   va_list ap;

   va_start(ap, fmt);
   vsprintf(buffer, fmt, ap);
   va_end(ap);

   OutputDebugString(buffer);
}


BOOL ScmCreateStartEvent(PHANDLE StartEvent)
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
				"SvcctrlStartEvent_A3725DX");
	     if (hEvent == NULL)
	       {
		  return FALSE;
	       }
	  }
	else
	  {
	     return FALSE;
	  }
     }

   *StartEvent = hEvent;

   return TRUE;
}


int STDCALL
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
{
   HANDLE hScmStartEvent;
   
   PrintString("Service Control Manager\n");
   
   /* Create start event */
   if (!ScmCreateStartEvent(&hScmStartEvent))
     {
	PrintString("SERVICES: Failed to create start event\n");
	ExitThread(0);
     }
   



   /* FIXME: more initialization */

   /* FIXME: create service database */
//   ScmCreateServiceDB();

   /* FIXME: update service database */
//   ScmGetBootAndSystemDriverState();

   /* FIXME: create pipe "\Pipe\Ntsvcs" */

   /* FIXME: create listener thread for pipe */

   /* FIXME: register process as service process */
//   RegisterServiceProcess();

   PrintString("SERVICES: Initialized.\n");

   /* Signal start event */
   SetEvent(hScmStartEvent);

   /* FIXME: register event handler (used for system shutdown) */
//   SetConsoleCtrlHandler(...);


   /* FIXME: start auto-start services */
//   ScmAutoStartServices();

   /* FIXME: more to do ? */


   PrintString("SERVICES: Running.\n");

    for (;;)
      {
	NtYieldExecution();
      }

   PrintString("SERVICES: Finished.\n");

   ExitThread (0);
   return 0;
}

/* EOF */
