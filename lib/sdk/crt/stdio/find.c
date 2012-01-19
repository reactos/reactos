#include <precomp.h>
#include <tchar.h>
#include <io.h>

// Generate _findfirst and _findnext
#include "findgen.c"

/*
 * @implemented
 */
int _findclose(intptr_t handle)
{
  if (!FindClose((HANDLE)handle)) {
    _dosmaperr(GetLastError());
    return -1;
  }

  return 0;
}
