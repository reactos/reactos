/* $Id: services.c,v 1.1 2000/12/05 02:38:08 ekohl Exp $
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

HANDLE OutputHandle;


/* FUNCTIONS *****************************************************************/

void PrintString (char* fmt,...)
{
   char buffer[512];
   va_list ap;

   va_start(ap, fmt);
   vsprintf(buffer, fmt, ap);
   va_end(ap);

   WriteConsoleA(OutputHandle, buffer, strlen(buffer), NULL, NULL);
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
				_T("SvcctrlStartEvent_A3725DX"));
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


//int main (int argc, char *argv[])
int STDCALL
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
{
   HANDLE hScmStartEvent;
   
   AllocConsole();
   OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
   
   PrintString("Service Control Manager\n");
   
   /* Create start event */
   if (!ScmCreateStartEvent(&hScmStartEvent))
     {
	PrintString("SERVICES: Failed to create start event\n");
	ExitThread(0);
     }
   



   /* FIXME: more initialization */



   
   PrintString("SERVICES: Initialized.\n");

   /* Signal start event */
   SetEvent(hScmStartEvent);


   /* FIXME: more to do ? */


   PrintString("SERVICES: Running.\n");

   ExitThread (0);
   return 0;
}

/* EOF */
