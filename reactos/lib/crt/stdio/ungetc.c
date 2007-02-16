/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>

#include <wchar.h>
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
  
  if (f->_ptr == f->_base)
  {
      if (!f->_cnt == 0)        
        return _TEOF;
  }
            
   fseek(f, -sizeof(_TCHAR), SEEK_CUR);
    
   if(*(_TCHAR*)f->_ptr != c)
        *((_TCHAR*)(f->_ptr)) = c;
           
   return c;
}

