/* Copyright (C) 1997 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <msvcrt/stdio.h>
#include <msvcrt/errno.h>
#include <msvcrt/fcntl.h>
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>


int fseek(FILE *f, long offset, int ptrname)
{
  long p = -1;			/* can't happen? */
  if ( f == NULL ) {
	__set_errno (EINVAL);
       return -1;
  }
  
  f->_flag &= ~_IOEOF;
  if (!OPEN4WRITING(f))
  {
    if (f->_base && !(f->_flag & _IONBF))
    {
      p = ftell(f);
      if (ptrname == SEEK_CUR)
      {
	offset += p;
	ptrname = SEEK_SET;
      }
      /* check if the target position is in the buffer and
        optimize seek by moving inside the buffer */
      if (ptrname == SEEK_SET && (f->_flag & (_IOUNGETC|_IOREAD|_IOWRT )) == 0
      && p-offset <= f->_ptr-f->_base && offset-p <= f->_cnt)
      {
        f->_ptr+=offset-p;
        f->_cnt+=p-offset;
        return 0;
      }
    }
 
    p = lseek(fileno(f), offset, ptrname);
    f->_cnt = 0;
    f->_ptr = f->_base;
    f->_flag &= ~_IOUNGETC;
  }
  else 
  {
    p = fflush(f);
    return lseek(fileno(f), offset, ptrname) == -1 || p == EOF ?
      -1 : 0;
  }
  return p==-1 ? -1 : 0;
}
