/* $Id: _system.c,v 1.7 2003/07/11 21:58:09 royce Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/process/system.c
 * PURPOSE:     Excutes a shell command
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              04/03/99: Created
 */
#include <windows.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>
#include <msvcrt/process.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>

/*
 * @implemented
 */
int system(const char *command)
{
  char *szCmdLine = NULL;
  char *szComSpec = NULL;

  PROCESS_INFORMATION ProcessInformation;
  STARTUPINFO StartupInfo;
  char *s;
  BOOL result;

  int nStatus;

  szComSpec = getenv("COMSPEC");

// system should return 0 if command is null and the shell is found

  if (command == NULL) {
      if (szComSpec == NULL)
	return 0;
      else
      {
	free(szComSpec);
	return -1;
      }
    }

// should return 127 or 0 ( MS ) if the shell is not found
// __set_errno(ENOENT);

  if (szComSpec == NULL)
  {
    szComSpec = strdup("cmd.exe");
    if (szComSpec == NULL)
    {
       __set_errno(ENOMEM);
       return -1;
    }
  }

  s = max(strchr(szComSpec, '\\'), strchr(szComSpec, '/'));
  if (s == NULL)
    s = szComSpec;
  else
    s++;

  szCmdLine = malloc(strlen(s) + 4 + strlen(command) + 1); 
  if (szCmdLine == NULL)
  {
     free (szComSpec);
     __set_errno(ENOMEM);
     return -1;
  }

  strcpy(szCmdLine, s);
  s = strrchr(szCmdLine, '.');
  if (s)
    *s = 0;
  strcat(szCmdLine, " /C ");
  strcat(szCmdLine, command);

//command file has invalid format ENOEXEC

  memset (&StartupInfo, 0, sizeof(STARTUPINFO));
  StartupInfo.cb = sizeof(STARTUPINFO);
  StartupInfo.lpReserved= NULL;
  StartupInfo.dwFlags = 0;
  StartupInfo.wShowWindow = SW_SHOWDEFAULT;
  StartupInfo.lpReserved2 = NULL;
  StartupInfo.cbReserved2 = 0;

// According to ansi standards the new process should ignore  SIGINT and SIGQUIT
// In order to disable ctr-c the process is created with CREATE_NEW_PROCESS_GROUP,
// thus SetConsoleCtrlHandler(NULL,TRUE) is made on behalf of the new process.


//SIGCHILD should be blocked aswell

  result = CreateProcessA(szComSpec,
	                  szCmdLine,
			  NULL,
			  NULL,
			  TRUE,
			  0,
			  NULL,
			  NULL,
			  &StartupInfo,
			  &ProcessInformation);
  free(szCmdLine);
  free(szComSpec);

  if (result == FALSE)
  {
     __set_errno(ENOEXEC);
     return -1;
  }
  
  CloseHandle(ProcessInformation.hThread);

// system should wait untill the calling process is finished
  _cwait(&nStatus,(int)ProcessInformation.hProcess,0);
  CloseHandle(ProcessInformation.hProcess);

  return nStatus;
}
