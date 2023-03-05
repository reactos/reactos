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
  char *s;
  BOOL result;
  DWORD exit_code;
  char szCmdExe[MAX_PATH];

  szComSpec = getenv("COMSPEC");

// system should return 0 if command is null and the shell is found

  if (command == NULL) {
    if (szComSpec == NULL)
      return 0;
    else
      return 1;
  }

  if (!szComSpec || GetFileAttributesA(szComSpec) == INVALID_FILE_ATTRIBUTES)
  {
    GetSystemDirectoryA(szCmdExe, _countof(szCmdExe));
    lstrcatA(szCmdExe, "\\cmd.exe");
    szComSpec = szCmdExe;
  }

  /* split the path from shell command */
  s = max(strrchr(szComSpec, '\\'), strrchr(szComSpec, '/'));
  if (s == NULL)
    s = szComSpec;
  else
    s++;

  szCmdLine = malloc(strlen(s) + 4 + strlen(command) + 1);
  if (szCmdLine == NULL)
  {
     _set_errno(ENOMEM);
     return -1;
  }

  strcpy(szCmdLine, s);
  s = strrchr(szCmdLine, '.');
  if (s)
    *s = 0;
  strcat(szCmdLine, " /C ");
  strcat(szCmdLine, command);

//command file has invalid format ENOEXEC

  ZeroMemory(&StartupInfo, sizeof(StartupInfo));
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
  WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
  GetExitCodeProcess(ProcessInformation.hProcess, &exit_code);

  CloseHandle(ProcessInformation.hProcess);

  _set_errno(0);
  return (int)exit_code;
}

int CDECL _wsystem(const wchar_t* cmd)
{
    wchar_t *szCmdLine = NULL;
    wchar_t *szComSpec = NULL;
    PROCESS_INFORMATION process_info;
    STARTUPINFOW startup_info;
    wchar_t *s;
    BOOL result;
    DWORD exit_code;
    wchar_t szCmdExe[MAX_PATH];

    szComSpec = _wgetenv(L"COMSPEC");

    /* _wsystem should return 0 if cmd is null and the shell is found */
    if (cmd == NULL)
    {
        if (szComSpec == NULL)
            return 0;
        else
            return 1;
    }

    if (szComSpec == NULL || GetFileAttributesW(szComSpec) == INVALID_FILE_ATTRIBUTES)
    {
        GetSystemDirectoryW(szCmdExe, _countof(szCmdExe));
        lstrcatW(szCmdExe, L"\\cmd.exe");
        szComSpec = szCmdExe;
    }

    /* split the path from shell command */
    s = max(wcsrchr(szComSpec, L'\\'), wcsrchr(szComSpec, L'/'));
    if (s == NULL)
        s = szComSpec;
    else
        s++;

    szCmdLine = malloc((wcslen(s) + 4 + wcslen(cmd) + 1) * sizeof(wchar_t));
    if (szCmdLine == NULL)
    {
        _set_errno(ENOMEM);
        return -1;
    }

    wcscpy(szCmdLine, s);
    s = wcsrchr(szCmdLine, L'.');
    if (s)
        *s = 0;
    wcscat(szCmdLine, L" /C ");
    wcscat(szCmdLine, cmd);

    /* command file has invalid format ENOEXEC */

    ZeroMemory(&startup_info, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    startup_info.dwFlags = STARTF_USESHOWWINDOW;
    startup_info.wShowWindow = SW_SHOWDEFAULT;

    /* In order to disable Ctrl+C, the process is created with CREATE_NEW_PROCESS_GROUP.
       Thus, SetConsoleCtrlHandler(NULL, TRUE) is made on behalf of the new process. */

    /* SIGCHILD should be blocked as well */

    /* Create the process to execute the command */
    result = CreateProcessW(szComSpec,
                            szCmdLine,
                            NULL,
                            NULL,
                            TRUE,
                            CREATE_NEW_PROCESS_GROUP,
                            NULL,
                            NULL,
                            &startup_info,
                            &process_info);
    free(szCmdLine);

    if (!result)
    {
        _dosmaperr(GetLastError());
        return -1;
    }

    CloseHandle(process_info.hThread);

    /* Wait for the process to exit */
    WaitForSingleObject(process_info.hProcess, INFINITE);
    GetExitCodeProcess(process_info.hProcess, &exit_code);

    CloseHandle(process_info.hProcess);

    _set_errno(0);
    return (int)exit_code;
}
