/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/process/system.c
 * PURPOSE:     Excutes a shell command
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              04/03/99: Created
 */
#include <windows.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>
#include <msvcrt/process.h>

int system(const char *command)
{
  char szCmdLine[MAX_PATH];
  char *szComSpec=NULL;

  PROCESS_INFORMATION ProcessInformation;
  STARTUPINFOA StartupInfo;

  int nStatus;

  szComSpec = getenv("COMSPEC");

// system should return 0 if command is null and the shell is found

  if (command == NULL)
    {
      if (szComSpec == NULL)
	return 0;
      else
	return -1;
    }

// should return 127 or 0 ( MS ) if the shell is not found
// __set_errno(ENOENT);

  if (szComSpec == NULL)
    szComSpec = "cmd.exe";

  strcpy(szCmdLine, " /C ");

  strncat(szCmdLine, command, MAX_PATH-5);

//check for a too long argument E2BIG

//command file has invalid format ENOEXEC


  StartupInfo.cb = sizeof(STARTUPINFOA);
  StartupInfo.lpReserved= NULL;
  StartupInfo.dwFlags = 0;
  StartupInfo.wShowWindow = SW_SHOWDEFAULT;
  StartupInfo.lpReserved2 = NULL;
  StartupInfo.cbReserved2 = 0;

// According to ansi standards the new process should ignore  SIGINT and SIGQUIT
// In order to disable ctr-c the process is created with CREATE_NEW_PROCESS_GROUP,
// thus SetConsoleCtrlHandler(NULL,TRUE) is made on behalf of the new process.


//SIGCHILD should be blocked aswell

  if (CreateProcessA(szComSpec,szCmdLine,NULL,NULL,TRUE,CREATE_NEW_PROCESS_GROUP,NULL,NULL,&StartupInfo,&ProcessInformation) == FALSE)
    {
      return -1;
    }

// system should wait untill the calling process is finished

  _cwait(&nStatus,(int)ProcessInformation.hProcess,0);

// free the comspec [ if the COMSPEC == NULL provision is removed
//  free(szComSpec);

  return nStatus;
}
