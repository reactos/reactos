/* Copyright (C) 1997 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


int _readcnv(int fn, void *buf, size_t siz  );

int
_filbuf(FILE *f)
{
  int size;
  char c;

 
  if ( !OPEN4READING(f)) {
	__set_errno (EINVAL);
	return EOF;
  }


  if (f->_flag&(_IOSTRG|_IOEOF))
    return EOF;
  f->_flag &= ~_IOUNGETC;

  if (f->_base==NULL && (f->_flag&_IONBF)==0) {
    size = 4096;
    if ((f->_base = malloc(size+1)) == NULL)
    {
	// error ENOMEM
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


// fush stdout before reading from stdin 
  if (f == stdin) {
    if (stdout->_flag&_IOLBF)
      fflush(stdout);
    if (stderr->_flag&_IOLBF)
      fflush(stderr);
  }

// if we have a dirty stream we flush it
  if ( (f->_flag &_IODIRTY) == _IODIRTY )
	 fflush(f);



  f->_cnt = _read(_fileno(f), f->_base, f->_flag & _IONBF ? 1 : f->_bufsiz  );
  f->_flag |= _IOAHEAD;

  if(__is_text_file(f) && f->_cnt>0)
  {
    /* truncate text file at Ctrl-Z */
    char *cz=memchr(f->_base, 0x1A, f->_cnt);
    if(cz)
    {
      int newcnt = cz - f->_base;
      _lseek(_fileno(f), -(f->_cnt - newcnt), SEEK_CUR);
      f->_cnt = newcnt;
    }
  }

  f->_ptr = f->_base;

  if (f->_flag & _IONBF) 
     f->_base = NULL; // statically allocated buffer for sprintf
  

//check for error
  if (f->_cnt <= 0) {
    if (f->_cnt == 0) {
      f->_flag |= _IOEOF;
    } else
      f->_flag |= _IOERR;
    f->_cnt = 0;

// should set errno 

    return EOF;
  }

  f->_cnt--;
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

