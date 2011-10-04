#include <precomp.h>
#include <direct.h>
#include <stdlib.h>
#include <tchar.h>

/*
 * @implemented
 */
_TCHAR* _tgetcwd(_TCHAR* buf, int size)
{
  _TCHAR dir[MAX_PATH];
  DWORD dir_len = GetCurrentDirectory(MAX_PATH,dir);

  if (dir_len == 0)
  {
    _dosmaperr(GetLastError());
    return NULL; /* FIXME: Real return value untested */
  }

  if (!buf)
  {
    return _tcsdup(dir);
  }

  if (dir_len >= (DWORD)size)
  {
    __set_errno(ERANGE);
    return NULL; /* buf too small */
  }

  _tcscpy(buf,dir);
  return buf;
}
