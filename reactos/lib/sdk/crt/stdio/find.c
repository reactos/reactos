#define _USE_FIND64 0
#include "find64.c"

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
