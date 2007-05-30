/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS shutdown/logoff utility
 * FILE:            apps/utils/shutdown/shutdown.c
 * PURPOSE:         Initiate logoff, shutdown or reboot of the system
 */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tchar.h>

static void
PrintUsage(LPCTSTR Cmd)
{
  _ftprintf(stderr, _T("usage: %s [action] [flag]\n"), Cmd);
  _ftprintf(stderr, _T(" action = \"logoff\", \"reboot\", \"shutdown\" or \"poweroff\"\n"));
  _ftprintf(stderr, _T(" flag = \"force\"\n"));
}

int
_tmain(int argc, TCHAR *argv[])
{
  static struct
    {
      TCHAR *Name;
      UINT ExitType;
      UINT ExitFlags;
    }
  Options[] =
    {
      { _T("logoff"), EWX_LOGOFF, 0 },
      { _T("logout"), EWX_LOGOFF, 0 },
      { _T("poweroff"), EWX_POWEROFF, 0 },
      { _T("powerdown"), EWX_POWEROFF, 0 },
      { _T("reboot"), EWX_REBOOT, 0 },
      { _T("restart"), EWX_REBOOT, 0 },
      { _T("shutdown"), EWX_SHUTDOWN, 0 },
      { _T("force"), 0, EWX_FORCE },
      { _T("forceifhung"), 0, EWX_FORCEIFHUNG },
      { _T("ifhung"), 0, EWX_FORCEIFHUNG },
      { _T("hung"), 0, EWX_FORCEIFHUNG },
    };
  UINT ExitType, ExitFlags;
  HANDLE hToken;
  TOKEN_PRIVILEGES npr;
  TCHAR *Arg;
  TCHAR BaseName[_MAX_FNAME];
  unsigned i, j;
  BOOL HaveType, Matched;

  ExitType = 0;
  ExitFlags = 0;
  HaveType = FALSE;

  _tsplitpath(argv[0], NULL, NULL, BaseName, NULL);

  /* Process optional arguments */
  for (i = 1; i < (unsigned) argc; i++)
    {
      /* Allow e.g. "/s" or "-l" for shutdown resp. logoff */
      Arg = argv[i];
      if (_T('/') == *Arg || _T('-') == *Arg)
        {
          Arg++;
        }

      /* Search known options */
      Matched = FALSE;
      for (j = 0; j < sizeof(Options) / sizeof(Options[0]) && ! Matched; j++)
        {
          /* Match if arg starts the same as the option name */
          if (0 == _tcsnicmp(Options[j].Name, Arg, _tcslen(Arg)))
            {
              if (0 == Options[j].ExitFlags)
                {
                  /* Can have only 1 type */
                  if (HaveType)
                    {
                      PrintUsage(BaseName);
                      exit(1);
                    }
                  ExitType = Options[j].ExitType;
                  HaveType = TRUE;
                }
              else
                {
                  /* Can have only 1 flag */
                  if (0 != ExitFlags)
                    {
                      PrintUsage(BaseName);
                      exit(1);
                    }
                  ExitFlags |= Options[j].ExitFlags;
                }
              Matched = TRUE;
            }
        }

      /* Was the argument processed? */
      if (! Matched)
        {
          PrintUsage(BaseName);
          exit(1);
        }
    }

  /* Check command name if user didn't explicitly specify action */
  if (! HaveType)
    {
      for (j = 0; j < sizeof(Options) / sizeof(Options[0]); j++)
        {
          if (0 == _tcsicmp(Options[j].Name, BaseName) && 0 == Options[j].ExitFlags)
            {
              ExitType = Options[j].ExitType;
              HaveType = TRUE;
            }
        }
    }

  /* Still not sure what to do? */
  if (! HaveType)
    {
      PrintUsage(BaseName);
      exit(1);
    }

  /* Everyone can logoff, for the other actions you need the appropriate privilege */
  if (EWX_LOGOFF != ExitType)
    {
      /* enable shutdown privilege for current process */
      if (! OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
        {
          _ftprintf(stderr, _T("OpenProcessToken failed with error %d\n"), (int) GetLastError());
          exit(1);
        }
      npr.PrivilegeCount = 1;
      npr.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
      if (! LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &npr.Privileges[0].Luid))
        {
          CloseHandle(hToken);
          _ftprintf(stderr, _T("LookupPrivilegeValue failed with error %d\n"), (int) GetLastError());
          exit(1);
        }
      if (! AdjustTokenPrivileges(hToken, FALSE, &npr, 0, 0, 0)
          || ERROR_SUCCESS != GetLastError())
        {
          if (ERROR_NOT_ALL_ASSIGNED == GetLastError())
            {
              _ftprintf(stderr, _T("You are not authorized to shutdown the system\n"));
            }
          else
            {
              _ftprintf(stderr, _T("AdjustTokenPrivileges failed with error %d\n"), (int) GetLastError());
            }
          CloseHandle(hToken);
          exit(1);
        }
      CloseHandle(hToken);
    }

  /* Finally do it */
  if (! ExitWindowsEx(ExitType | ExitFlags, 0))
    {
      _ftprintf(stderr, _T("ExitWindowsEx failed with error %d\n"), (int) GetLastError());
      exit(1);
    }

  return 0;
}
