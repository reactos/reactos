/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <crtdll/stdio.h>
#include <crtdll/errno.h>
#include <crtdll/sys/types.h>
#include <crtdll/sys/stat.h>
#include <crtdll/stdlib.h>
#include <crtdll/internal/file.h>
#include <crtdll/io.h>


int fflush(FILE *f)
{
  char *base;
  int n, rn;



  if (f == NULL)
  {
     int e = errno;

     __set_errno(0);
    _fwalk((void (*)(FILE *))fflush);
    if (_errno)
      return EOF;
    __set_errno(e);
    return 0;
  }


// nothing to do if stream can not be written to

  if ( !OPEN4WRITING(f) ) {
	__set_errno (EINVAL);
	return 0;
  }

// discard any unget characters

  f->_flag &= ~_IOUNGETC;


// check for buffered dirty block

  if ( (f->_flag&(_IODIRTY|_IONBF)) ==_IODIRTY && f->_base != NULL)
  {

    base = f->_base;
   

// if the buffer is read ahead and dirty we will flush it entirely
// else the buffer is appended to the file to the extend it has valid bytes

    if ( (f->_flag & _IOAHEAD) == _IOAHEAD )
    	rn = n = f->_ptr - base + f->_cnt;
    else
	rn = n = f->_ptr - base;

    f->_ptr = base;

    if ((f->_flag & _IOFBF) == _IOFBF) {
     	if ( (f->_flag & _IOAHEAD) == _IOAHEAD )
 	    _lseek(fileno(f),-rn, SEEK_CUR);
    }

    f->_flag &= ~_IOAHEAD;


    f->_cnt = (f->_flag&(_IOLBF|_IONBF)) ? 0 : f->_bufsiz;

// how can write return less than rn without being on error ???

// possibly commit the flushed data
// better open the file in write through mode

    do {
      n = _write(fileno(f), base, rn);
      if (n <= 0) {
	f->_flag |= _IOERR;
	return EOF;
      }
      rn -= n;
      base += n;
    } while (rn > 0);
    f->_flag &= ~_IODIRTY;

  }
  if (OPEN4READING(f) && OPEN4WRITING(f) )
  {
    f->_cnt = 0;
    f->_ptr = f->_base;
  }
  return 0;
}

int _flushall( void )
{
	return fflush(NULL);
}