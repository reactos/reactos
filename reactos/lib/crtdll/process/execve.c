/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
//#include <crtdll/stubs.h>
//#include <crtdll/unistd.h>
#include <crtdll/process.h>

int execve(const char *path,const  char * const argv[], char * const envp[])
{
  return spawnve(P_OVERLAY, path, argv, envp);
}
