#include <msvcrti.h>


size_t fread(void *vptr, size_t size, size_t count, FILE *iop)
{
  unsigned char *ptr = (unsigned char *)vptr;
  size_t  to_read ,n_read;
  int c, copy;

  to_read = size * count;
  
  if (!OPEN4READING(iop))
    {
      __set_errno (EINVAL);
      return 0;
    }

  if (!__validfp (iop) )
    {
      __set_errno (EINVAL);
      return 0;
    }
  if (feof (iop) || ferror (iop))
    return 0;

  if (vptr == NULL || to_read == 0)
    return 0;

  if (iop->_base == NULL)
  {
    int c = _filbuf(iop);
    if (c == EOF)
      return 0;
    *ptr++ = c;
    if (--to_read == 0)
      return 1;
  }

  if (iop->_cnt > 0 && to_read > 0)
  {
     copy = min(iop->_cnt, to_read);
     memcpy(ptr, iop->_ptr, copy);
     ptr += copy;
     iop->_ptr += copy;
     iop->_cnt -= copy;
     to_read -= copy;
     if (to_read == 0)
       return count;
  }

  if (to_read > 0)
  {

    if (to_read >= iop->_bufsiz)
    {
       n_read = _read(_fileno(iop), ptr, to_read);
	   if (n_read < 0)
		  iop->_flag |= _IOERR;
	   else if (n_read == 0)
		  iop->_flag |= _IOEOF;
	   else
          to_read -= n_read;

       // the file buffer is empty and there is no read ahead information anymore.
       iop->_flag &= ~_IOAHEAD;
    }
    else
    {
       c = _filbuf(iop);
       if (c != EOF)
       {
	      *ptr++ = c;
	      to_read--;
          copy = min(iop->_cnt, to_read);
          memcpy(ptr, iop->_ptr, copy);
          iop->_ptr += copy;
          iop->_cnt -= copy;
          to_read -= copy;
       }
    }
  }
  return count - (to_read/size);
}
