#include <precomp.h>
#include <direct.h>
#include <internal/debug.h>
#include <tchar.h>

/*
 * @implemented
 *
 *    _getdcwd (MSVCRT.@)
 *
 * Get the current working directory on a given disk.
 *
 * PARAMS
 *  drive [I] Drive letter to get the current working directory from.
 *  buf   [O] Destination for the current working directory.
 *  size  [I] Length of drive in characters.
 *
 * RETURNS
 *  Success: If drive is NULL, returns an allocated string containing the path.
 *           Otherwise populates drive with the path and returns it.
 *  Failure: NULL. errno indicates the error.
 */
_TCHAR* _tgetdcwd(int drive, _TCHAR * buf, int size)
{
  static _TCHAR* dummy;

  TRACE(":drive %d(%c), size %d\n",drive, drive + 'A' - 1, size);

  if (!drive || drive == _getdrive())
    return _tgetcwd(buf,size); /* current */
  else
  {
    _TCHAR dir[MAX_PATH];
    _TCHAR drivespec[] = _T("A:");
    int dir_len;

    drivespec[0] += drive - 1;
    if (GetDriveType(drivespec) < DRIVE_REMOVABLE)
    {
      __set_errno(EACCES);
      return NULL;
    }

    /* GetFullPathName for X: means "get working directory on drive X",
     * just like passing X: to SetCurrentDirectory means "switch to working
     * directory on drive X". -Gunnar */
    dir_len = GetFullPathName(drivespec,MAX_PATH,dir,&dummy);
    if (dir_len >= size || dir_len < 1)
    {
      __set_errno(ERANGE);
      return NULL; /* buf too small */
    }

    TRACE(":returning '%s'\n", dir);
    if (!buf)
      return _tcsdup(dir); /* allocate */

    _tcscpy(buf,dir);
  }
  return buf;
}

