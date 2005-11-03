/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <wchar.h>
#include <errno.h>
#include <internal/file.h>
#include <tchar.h>


/*
 * @implemented
 */
_TINT _ungettc(_TINT c, FILE *f)
{
  if (!__validfp (f) || !OPEN4READING(f)) {
      __set_errno (EINVAL);
      return _TEOF;
    }

  if (c == _TEOF)
    return _TEOF;

  if (f->_ptr == NULL || f->_base == NULL)
    return _TEOF;

  if (f->_ptr == f->_base)
    {
      if (f->_cnt == 0)
        f->_ptr+=sizeof(_TCHAR);
      else
        return _TEOF;
    }

  f->_cnt+=sizeof(_TCHAR);
  f->_ptr-=sizeof(_TCHAR);

#if 1
  if (*((_TCHAR*)(f->_ptr)) != c)
    {
      f->_flag |= _IOUNGETC;
      *((_TCHAR*)(f->_ptr)) = c;
    }
#else
  /* This is the old unicode version. Dunno what version is most correct. -Gunnar */
  f->_flag |= _IOUNGETC;
  *((_TCHAR*)(f->_ptr)) = c;
#endif

  return c;
}

