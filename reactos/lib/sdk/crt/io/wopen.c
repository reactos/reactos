/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/io/open.c
 * PURPOSE:     Opens a file and translates handles to fileno
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

// rember to interlock the allocation of fileno when making this thread safe
// possibly store extra information at the handle

#include <precomp.h>
#if !defined(NDEBUG) && defined(DBG)
#include <stdarg.h>
#endif
#include <sys/stat.h>
#include <share.h>


/*********************************************************************
 *              _wopen (MSVCRT.@)
 */
int CDECL _wopen(const wchar_t *path,int flags,...)
{
  const unsigned int len = strlenW(path);
  char *patha = calloc(len + 1,1);
  va_list ap;
  int pmode;

  va_start(ap, flags);
  pmode = va_arg(ap, int);
  va_end(ap);

  if (patha && WideCharToMultiByte(CP_ACP,0,path,len,patha,len,NULL,NULL))
  {
    int retval = _open(patha,flags,pmode);
    free(patha);
    return retval;
  }

  _dosmaperr(GetLastError());
  return -1;
}

/*********************************************************************
 *              _wsopen (MSVCRT.@)
 */
int CDECL _wsopen( const wchar_t* path, int oflags, int shflags, ... )
{
  const unsigned int len = strlenW(path);
  char *patha = calloc(len + 1,1);
  va_list ap;
  int pmode;

  va_start(ap, shflags);
  pmode = va_arg(ap, int);
  va_end(ap);

  if (patha && WideCharToMultiByte(CP_ACP,0,path,len,patha,len,NULL,NULL))
  {
    int retval = sopen(patha,oflags,shflags,pmode);
    free(patha);
    return retval;
  }

  _dosmaperr(GetLastError());
  return -1;
}
