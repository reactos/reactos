/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <crtdll/internal/file.h>
#include <crtdll/wchar.h>
#include <crtdll/errno.h>

int
ungetc(int c, FILE *f)
{

  if (!__validfp (f) || !OPEN4READING(f)) {
      __set_errno (EINVAL);
      return EOF;
  }

  if (c == EOF )
	return EOF;

  
    
  if ( f->_ptr == NULL || f->_base == NULL)
   	 return EOF;

  if (f->_ptr == f->_base)
  {
    if (f->_cnt == 0)
      f->_ptr++;
    else
      return EOF;
  }

  f->_cnt++;
  f->_ptr--;
  if(*f->_ptr != c)
  {
    f->_flag |= _IOUNGETC;
    *f->_ptr = c;
  }

  return c;
}


wint_t
ungetwc(wchar_t c, FILE *f)
{
  if (!__validfp (f) || !OPEN4READING(f)) {
      __set_errno (EINVAL);
      return EOF;
  }

  if (c == (wchar_t)EOF )
	return EOF;

  
    
  if ( f->_ptr == NULL || f->_base == NULL)
   	 return EOF;

  if (f->_ptr == f->_base)
  {
    if (f->_cnt == 0)
      f->_ptr+=sizeof(wchar_t);
    else
      return EOF;
  }

  f->_cnt+=sizeof(wchar_t);
  f->_ptr-=sizeof(wchar_t);

  f->_flag |= _IOUNGETC;
  *((wchar_t *)(f->_ptr)) = c;

  return c;
}