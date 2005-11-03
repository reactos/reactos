/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <stdio.h>
#include <stdarg.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <internal/file.h>


// changed check for writable stream


/*
 * @implemented
 */
int
fclose(FILE *f)
{
  int r = 0;

  if (f == NULL) {
    __set_errno (EINVAL);
    return EOF;
  }



// flush only if stream was opened for writing
  if ( !(f->_flag&_IOSTRG) ) {
  	if ( OPEN4WRITING(f) )
    		r = fflush(f);

  	if (_close(_fileno(f)) < 0)
      		r = EOF;
  	if (f->_flag&_IOMYBUF)
      		free(f->_base);

// Kernel might do this later
   if (f->_flag & _IORMONCL && f->_tmpfname)
  	{
         remove(f->_tmpfname);
         free(f->_tmpfname);
         f->_tmpfname = 0;
  	}
  }
  f->_cnt = 0;
  f->_base = 0;
  f->_ptr = 0;
  f->_bufsiz = 0;
  f->_flag = 0;
  f->_file = -1;
  return r;
}
