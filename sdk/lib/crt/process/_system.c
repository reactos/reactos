/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/process/_system.c
 * PURPOSE:     Excutes a shell command
 * PROGRAMER:   Ariadne
 *              Katayama Hirofumi MZ
 * UPDATE HISTORY:
 *              04/03/99: Created
 */

#include <precomp.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

/*
 * @implemented
 */
int system(const char *command)
{
  char *szCmdLine = NULL;
  char *szComSpec = NULL;
  PROCESS_INFORMATION ProcessInformation;
  STARTUPINFOA StartupInfo;
  BOOL result;
  int exit_code;
  char cmd_exe[MAX_PATH];

  szComSpec = getenv("COMSPEC");

// system should return 0 if command is null and the shell is found

  if (command == NULL) {
    return (szComSpec == NULL) ? 0 : 1;
  }

  if (!szComSpec || GetFileAttributesA(szComSpec) == INVALID_FILE_ATTRIBUTES)
  {
    GetSystemDirectoryA(cmd_exe, _countof(cmd_exe));
    strcat(cmd_exe, "\\cmd.exe");
    szComSpec = cmd_exe;
  }

  szCmdLine = malloc(1 + strlen(szComSpec) + 5 + strlen(command) + 1);
  if (szCmdLine == NULL)
  {
    _set_errno(ENOMEM);
    return -1;
  }

  strcpy(szCmdLine, "\"");
  strcat(szCmdLine, szComSpec);
  strcat(szCmdLine, "\" /C ");
  strcat(szCmdLine, command);

//command file has invalid format ENOEXEC

  memset(&StartupInfo, 0, sizeof(StartupInfo));
  StartupInfo.cb = sizeof(StartupInfo);
  StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
  StartupInfo.wShowWindow = SW_SHOWDEFAULT;

// In order to disable Ctrl+C, the process is created with CREATE_NEW_PROCESS_GROUP.
// Thus, SetConsoleCtrlHandler(NULL, TRUE) is made on behalf of the new process.

//SIGCHILD should be blocked as well

  result = CreateProcessA(szComSpec,
	                  szCmdLine,
			  NULL,
			  NULL,
			  TRUE,
			  CREATE_NEW_PROCESS_GROUP,
			  NULL,
			  NULL,
			  &StartupInfo,
			  &ProcessInformation);
  free(szCmdLine);

  if (result == FALSE)
  {
	_dosmaperr(GetLastError());
     return -1;
  }

  CloseHandle(ProcessInformation.hThread);

  /* Wait for the process to exit */
  _cwait(&exit_code, (intptr_t)ProcessInformation.hProcess, 0);

  CloseHandle(ProcessInformation.hProcess);

  _set_errno(0);
  return exit_code;
}

int CDECL _wsystem(const wchar_t *cmd)
{
    wchar_t *cmdline = NULL;
    wchar_t *comspec = NULL;
    PROCESS_INFORMATION process_info;
    STARTUPINFOW startup_info;
    BOOL result;
    int exit_code;
    wchar_t cmd_exe[MAX_PATH];

    comspec = _wgetenv(L"COMSPEC");

    /* _wsystem should return 0 if cmd is null and the shell is found */
    if (cmd == NULL)
    {
        return (comspec == NULL) ? 0 : 1;
    }

    if (comspec == NULL || GetFileAttributesW(comspec) == INVALID_FILE_ATTRIBUTES)
    {
        GetSystemDirectoryW(cmd_exe, _countof(cmd_exe));
        wcscat(cmd_exe, L"\\cmd.exe");
        comspec = cmd_exe;
    }

    cmdline = malloc((1 + wcslen(comspec) + 5 + wcslen(cmd) + 1) * sizeof(wchar_t));
    if (cmdline == NULL)
    {
        _set_errno(ENOMEM);
        return -1;
    }

    wcscpy(cmdline, L"\"");
    wcscat(cmdline, comspec);
    wcscat(cmdline, L"\" /C ");
    wcscat(cmdline, cmd);

    /* command file has invalid format ENOEXEC */

    memset(&startup_info, 0, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    startup_info.dwFlags = STARTF_USESHOWWINDOW;
    startup_info.wShowWindow = SW_SHOWDEFAULT;

    /* In order to disable Ctrl+C, the process is created with CREATE_NEW_PROCESS_GROUP.
       Thus, SetConsoleCtrlHandler(NULL, TRUE) is made on behalf of the new process. */

    /* SIGCHILD should be blocked as well */

    /* Create the process to execute the command */
    result = CreateProcessW(comspec,
                            cmdline,
                            NULL,
                            NULL,
                            TRUE,
                            CREATE_NEW_PROCESS_GROUP,
                            NULL,
                            NULL,
                            &startup_info,
                            &process_info);
    free(cmdline);

    if (!result)
    {
        _dosmaperr(GetLastError());
        return -1;
    }

    CloseHandle(process_info.hThread);

    /* Wait for the process to exit */
    _cwait(&exit_code, (intptr_t)process_info.hProcess, 0);

    CloseHandle(process_info.hProcess);

    _set_errno(0);
    return exit_code;
}
