/* 
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/process/_system.c
 * PURPOSE:     Excutes a shell command
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              04/03/99: Created
 */

#include <precomp.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>


wchar_t *msvcrt_wstrdupa(const char *); //file.c
intptr_t do_spawnW(int mode, const wchar_t* cmdname, const wchar_t* args, const wchar_t* envp); //process.c

/* INTERNAL: retrieve COMSPEC environment variable */
static wchar_t *get_comspec(void)
{
  static const wchar_t cmd[] = {'c','m','d',0};
  static const wchar_t comspec[] = {'C','O','M','S','P','E','C',0};
  wchar_t *ret;
  unsigned int len;

  if (!(len = GetEnvironmentVariableW(comspec, NULL, 0))) len = sizeof(cmd)/sizeof(wchar_t);
  if ((ret = HeapAlloc(GetProcessHeap(), 0, len * sizeof(wchar_t))))
  {
    if (!GetEnvironmentVariableW(comspec, ret, len)) strcpyW(ret, cmd);
  }
  return ret;
}

int CDECL _wsystem(const wchar_t* cmd)
{
  int res;
  wchar_t *comspec, *fullcmd;
  unsigned int len, comspecLen, flagLen, cmdLen;
  static const wchar_t flag[] = {' ','/','c',' ',0};

  comspec = get_comspec();

  if (cmd == NULL)
  {
    if (comspec == NULL)
    {
        *_errno() = ENOENT;
        return 0;
    }
    HeapFree(GetProcessHeap(), 0, comspec);
    return 1;
  }

  if ( comspec == NULL)
    return -1;

  comspecLen = wcslen(comspec);
  flagLen  = wcslen(flag);
  cmdLen = wcslen(cmd);
  
  len = comspecLen + flagLen + cmdLen + 1;

  if (!(fullcmd = HeapAlloc(GetProcessHeap(), 0, len * sizeof(wchar_t))))
  {
    HeapFree(GetProcessHeap(), 0, comspec);
    return -1;
  }
  wcsncat(fullcmd, comspec, comspecLen);
  wcsncat(fullcmd, flag, flagLen);
  wcsncat(fullcmd, cmd, cmdLen);

  res = do_spawnW(_P_WAIT, comspec, fullcmd, NULL);

  HeapFree(GetProcessHeap(), 0, comspec);
  HeapFree(GetProcessHeap(), 0, fullcmd);
  return res;
}

/*
 * @implemented
 */
int CDECL system(const char* cmd)
{
  int res = -1;
  wchar_t *cmdW;

  if (cmd == NULL)
    return _wsystem(NULL);

  if ((cmdW = msvcrt_wstrdupa(cmd)))
  {
    res = _wsystem(cmdW);
    HeapFree(GetProcessHeap(), 0, cmdW);
  }
  return res;
}
