/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


int cntcr(char *bufp, int bufsiz);
int convert(char *endp, int bufsiz,int n);
int _writecnv(int fn, void *buf, size_t bufsiz);

int
_flsbuf(int c, FILE *f)
{
  char *base;
  int n, rn;
  char c1;
  int size;



  if (!OPEN4WRITING(f)) {
	__set_errno (EINVAL);
	return EOF;
  }

// no file associated with buffer
// this is a memory stream

  if ( _fileno(f) == -1 )
	return c;

  /* if the buffer is not yet allocated, allocate it */
  if ((base = f->_base) == NULL && (f->_flag & _IONBF) == 0)
  {
    size = 4096;
    if ((f->_base = base = malloc (size)) == NULL)
    {
      f->_flag |= _IONBF;
      f->_flag &= ~(_IOFBF|_IOLBF);
    }
    else
    {
      f->_flag |= _IOMYBUF;
      f->_cnt = f->_bufsiz = size;
      f->_ptr = base;
      rn = 0;
      if (f == stdout && _isatty (_fileno (stdout)))
	f->_flag |= _IOLBF;
    }
  }

  if (f->_flag & _IOLBF)
  {
    /* in line-buffering mode we get here on each character */
    *f->_ptr++ = c;
    rn = f->_ptr - base;
    if (c == '\n' || rn >= f->_bufsiz)
    {
      /* time for real flush */
      f->_ptr = base;
      f->_cnt = 0;
    }
    else
    {
      /* we got here because _cnt is wrong, so fix it */
      /* Negative _cnt causes all output functions
	to call _flsbuf for each character, thus realizing line-buffering */
      f->_cnt = -rn;
      return c;
    }
  }
  else if (f->_flag & _IONBF)
  {                   
    c1 = c;           
    rn = 1;           
    base = &c1;       
    f->_cnt = 0;
  }                   
  else /* _IOFBF */
  {
    rn = f->_ptr - base;
    f->_ptr = base;
    if ( (f->_flag & _IOAHEAD) == _IOAHEAD )
 	_lseek(_fileno(f),-(rn+f->_cnt), SEEK_CUR);
    f->_cnt = f->_bufsiz;
    f->_flag &= ~_IOAHEAD;
  }

 

  f->_flag &= ~_IODIRTY;
  while (rn > 0)
  {
    n = _write(_fileno(f), base, rn);
    if (n <= 0)
    {
      f->_flag |= _IOERR;
      return EOF;
    }
    rn -= n;
    base += n;
  }


  if ((f->_flag&(_IOLBF|_IONBF)) == 0)
  {
    f->_cnt--;
    *f->_ptr++ = c;
  }
  return c;
}

wint_t  _flswbuf(wchar_t c,FILE *fp)
{
	return (wint_t )_flsbuf((int)c,fp);
}


int _writecnv(int fn, void *buf, size_t siz)
{
	char *bufp = (char *)buf;
	int bufsiz = siz;

        char *tmp;
	int cr1 = 0;
	int cr2 = 0;

	int n;
		

	cr1 = cntcr(bufp,bufsiz);

	tmp = malloc(cr1);
	memcpy(tmp,bufp+bufsiz-cr1,cr1);
	cr2 = cntcr(tmp,cr1);
		
	convert(bufp,bufsiz-cr2,cr1-cr2);
	n = _write(fn, bufp, bufsiz + cr1);

	convert(tmp,cr1,cr2);
	n += _write(fn, tmp, cr1 + cr2);
	free(tmp);
	return n;	
	

}

int convert(char *endp, int bufsiz,int n)
{	
	endp = endp + bufsiz + n;
	while (bufsiz > 0) {
		*endp = *(endp  - n);
		if (*endp == '\n') {
			*endp--;
			n--;
			*endp = '\r';
		}
		endp--;
		bufsiz--;
	}
	return n;
}
int cntcr(char *bufp, int bufsiz)
{
	int cr = 0; 
	while (bufsiz > 0) {
		if (*bufp == '\n') 
			cr++;
		bufp++;
		bufsiz--;
	}

	return cr;
}
