/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/stdlib/senv.c
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#ifdef _UNICODE
   #define sT "S"
#else
   #define sT "s"
#endif

#define MK_STR(s) #s

/*
 * @implemented
 */
void _tsearchenv(const _TCHAR* file,const _TCHAR* var,_TCHAR* path)
{
    _TCHAR* env = _tgetenv(var);
    _TCHAR* x;
    _TCHAR* y;
    _TCHAR* FilePart;

    TRACE(MK_STR(_tsearchenv)"()\n");

    x = _tcschr(env,'=');
    if ( x != NULL ) {
        *x = 0;
        x++;
    }
    y = _tcschr(env,';');
    while ( y != NULL ) {
        *y = 0;
        if ( SearchPath(x,file,NULL,MAX_PATH,path,&FilePart) > 0 ) {
            return;
        }
        x = y+1;
        y = _tcschr(env,';');
    }
    return;
}

/*********************************************************************
 *		_searchenv_s (MSVCRT.@)
 */
int _tsearchenv_s(const _TCHAR* file, const _TCHAR* env, _TCHAR *buf, size_t count)
{
  _TCHAR *envVal, *penv;
  _TCHAR curPath[MAX_PATH];

  if (!MSVCRT_CHECK_PMT(file != NULL) || !MSVCRT_CHECK_PMT(buf != NULL) ||
      !MSVCRT_CHECK_PMT(count > 0))
  {
      *_errno() = EINVAL;
      return EINVAL;
  }

  *buf = '\0';

  /* Try CWD first */
  if (GetFileAttributes( file ) != INVALID_FILE_ATTRIBUTES)
  {
    GetFullPathName( file, MAX_PATH, buf, NULL );
    _dosmaperr(GetLastError());
    return 0;
  }

  /* Search given environment variable */
  envVal = _tgetenv(env);
  if (!envVal)
  {
    _set_errno(ENOENT);
    return ENOENT;
  }

  penv = envVal;

  do
  {
    _TCHAR *end = penv;

    while(*end && *end != ';') end++; /* Find end of next path */
    if (penv == end || !*penv)
    {
      _set_errno(ENOENT);
      return ENOENT;
    }
    memcpy(curPath, penv, (end - penv) * sizeof(_TCHAR));
    if (curPath[end - penv] != '/' && curPath[end - penv] != '\\')
    {
      curPath[end - penv] = '\\';
      curPath[end - penv + 1] = '\0';
    }
    else
      curPath[end - penv] = '\0';

    _tcscat(curPath, file);
    if (GetFileAttributes( curPath ) != INVALID_FILE_ATTRIBUTES)
    {
      if (_tcslen(curPath) + 1 > count)
      {
          MSVCRT_INVALID_PMT("buf[count] is too small", ERANGE);
          return ERANGE;
      }
      _tcscpy(buf, curPath);
      return 0;
    }
    penv = *end ? end + 1 : end;
  } while(1);

}
