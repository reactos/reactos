/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/process/cwait.c
 * PURPOSE:     Waits for a process to exit 
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              04/03/99: Created
 */
#include <windows.h>
#include <msvcrt/process.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>

int _cwait(int* pnStatus, int hProc, int nAction)
{
  DWORD ExitCode;

  nAction = 0;
  if (WaitForSingleObject((void *)hProc,INFINITE) != WAIT_OBJECT_0)
    {
      __set_errno(ECHILD);
      return -1;
    }

  if (!GetExitCodeProcess((void *)hProc,&ExitCode))
    return -1;
  if (pnStatus != NULL)
    *pnStatus = (int)ExitCode;
  CloseHandle((HANDLE)hProc);
  return hProc;
}
