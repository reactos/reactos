/* Copyright (C) 1997 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <crtdll/stdio.h>
#include <crtdll/sys/types.h>
#include <crtdll/stdlib.h>
#include <crtdll/string.h>
#include <crtdll/internal/file.h>
#include <crtdll/io.h>
#include <crtdll/wchar.h>

int _readcnv(int fn, void *buf, size_t siz  );

int
_filbuf(FILE *f)
{
  int size;
  char c;

 
//  if ( !READ_STREAM(f))
//    return EOF;


  if (f->_flag&(_IOSTRG|_IOEOF))
    return EOF;
  f->_flag &= ~_IOUNGETC;

  if (f->_base==NULL && (f->_flag&_IONBF)==0) {
    size = 4096;
    if ((f->_base = malloc(size+1)) == NULL)
    {
      f->_flag |= _IONBF;
      f->_flag &= ~(_IOFBF|_IOLBF);
    }
    else
    {
      f->_flag |= _IOMYBUF;
      f->_bufsiz = size;
    }
  }

  if (f->_flag&_IONBF)
    f->_base = &c;

  if (f == stdin) {
    if (stdout->_flag&_IOLBF)
      fflush(stdout);
    if (stderr->_flag&_IOLBF)
      fflush(stderr);
  }

// if(__is_text_file(f))
//	 f->_cnt = _readcnv(fileno(f), f->_base,
//		   f->_flag & _IONBF ? 1 : f->_bufsiz  );
//  else


  	f->_cnt = _read(fileno(f), f->_base,
		   f->_flag & _IONBF ? 1 : f->_bufsiz  );
	f->_flag |= _IOAHEAD;

  if(__is_text_file(f) && f->_cnt>0)
  {
    /* truncate text file at Ctrl-Z */
    char *cz=memchr(f->_base, 0x1A, f->_cnt);
    if(cz)
    {
      int newcnt = cz - f->_base;
      lseek(fileno(f), -(f->_cnt - newcnt), SEEK_CUR);
      f->_cnt = newcnt;
    }
  }
  f->_ptr = f->_base;
  if (f->_flag & _IONBF) 
     f->_base = NULL; // statically allocated buffer for sprintf
  
  if (--f->_cnt < 0) {
    if (f->_cnt == -1) {
      f->_flag |= _IOEOF;
      //if (f->_flag & _IORW)
	//f->_flag &= ~_IOREAD;
    } else
      f->_flag |= _IOERR;
    f->_cnt = 0;
    return EOF;
  }

  return *f->_ptr++ & 0377;
}

wint_t  _filwbuf(FILE *fp)
{
	return (wint_t )_filbuf(fp);
}

// convert the carriage return line feed pairs

int _readcnv(int fn, void *buf, size_t siz  )
{
	char *bufp = (char *)buf;
	int _bufsiz = siz;
	int cr = 0;
	int n;

 	n = _read(fn, buf, siz  );

	while (_bufsiz > 0) {
		if (*bufp == '\r') 
			cr++;
		else if ( cr != 0 ) 
			*bufp = *(bufp + cr);
		bufp++;
		_bufsiz--;
	}
	return n + cr;
}

