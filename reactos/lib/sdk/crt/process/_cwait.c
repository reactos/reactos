/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/process/cwait.c
 * PURPOSE:     Waits for a process to exit
 */

#include <precomp.h>

/* Taken from Wine msvcrt/process.c */

/*
 * @implemented
 */
intptr_t CDECL _cwait(int *status, intptr_t pid, int action)
{
  HANDLE hPid = (HANDLE)pid;
  int doserrno;

  if (!WaitForSingleObject(hPid, INFINITE))
  {
    if (status)
    {
      DWORD stat;
      GetExitCodeProcess(hPid, &stat);
      *status = (int)stat;
    }
    return pid;
  }
  doserrno = GetLastError();

  if (doserrno == ERROR_INVALID_HANDLE)
  {
    *_errno() =  ECHILD;
    *__doserrno() = doserrno;
  }
  else
    _dosmaperr(doserrno);

  return status ? *status = -1 : -1;
}
