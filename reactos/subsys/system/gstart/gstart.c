/* $Id: gstart.c,v 1.2 2002/09/07 15:13:09 chorns Exp $
 *
 * PROJECT    : ReactOS Operating System
 * DESCRIPTION: ReactOS GUI Startup
 * LICENSE    : See top level directory
 *
 */
#define NTOS_USER_MODE
#include <ntos.h>
#include <windows.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int ExecuteProcess(char* name, char* cmdline)
{
   PROCESS_INFORMATION	ProcessInformation;
   STARTUPINFO		StartupInfo;
   BOOL			ret;
   CHAR			fullname[260];
   PCHAR		p;
   
   /* append '.exe' if needed */
   strcpy (fullname, name);
   p = strrchr (fullname, '.');
   if ((p == NULL) || (_stricmp (p, ".exe") != 0))
     {
	strcat (fullname, ".exe");
     }

   memset(&StartupInfo, 0, sizeof(StartupInfo));
   StartupInfo.cb = sizeof (STARTUPINFO);
   StartupInfo.lpTitle = name;
   if( cmdline && *cmdline  )
     *(cmdline-1) = ' ';  
   ret = CreateProcessA(fullname,
			name,
			NULL,
			NULL,
			FALSE,
			0,
			NULL,
			NULL,
			&StartupInfo,
			&ProcessInformation);
   if (ret)
     {
       WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
       CloseHandle(ProcessInformation.hProcess);
       CloseHandle(ProcessInformation.hThread);
     }
   return(ret);
}

int main(int argc, char* argv[])
{
  HDC Desktop;
  HBRUSH Pen;

  GdiDllInitialize (NULL, DLL_PROCESS_ATTACH, NULL);

  /* Set up a DC called Desktop that accesses DISPLAY */
  Desktop = CreateDCA("DISPLAY", NULL, NULL, NULL);
  if (Desktop == NULL)
    {
      return 1;
    }

  Pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 255));
  SelectObject(Desktop, Pen);
  Rectangle(Desktop, 0, 0, 640, 480);

  if (argc > 1)
    {
      ExecuteProcess(argv[1], "");
    }
  else
    {
      Sleep(50);
    }

  DeleteDC(Desktop);

  return(0);
}

/* EOF */
