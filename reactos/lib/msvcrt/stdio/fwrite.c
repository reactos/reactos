#include <msvcrti.h>

#define NDEBUG
#include <msvcrtdbg.h>


size_t fwrite(const void *vptr, size_t size, size_t count, FILE *iop)
{
  size_t to_write, n_written;
  unsigned char *ptr = (unsigned char *)vptr;
  int copy;

  DPRINT("fwrite(%x, %d, %d, %x)\n", vptr, size, count, iop);

  to_write = size*count;
  if (!OPEN4WRITING(iop))
    {
      __set_errno (EINVAL);
      return 0;
    }

  if (iop == NULL)
    {
      __set_errno (EINVAL);
      return 0;
    }

  if (ferror (iop))
    return 0;
  if (vptr == NULL || to_write == 0)
    return 0;

  if (iop->_base == NULL && !(iop->_flag&_IONBF))
  {
     if (EOF == _flsbuf(*ptr++, iop))
        return 0;
     if (--to_write == 0)
        return 1;
  }

  if (iop->_flag & _IOLBF)
  {
     while (to_write > 0)
     {
        if (EOF == putc(*ptr++, iop))
	{
	   iop->_flag |= _IOERR;
	   break;
	}
	to_write--;
     }
  }
  else
  {
     if (iop->_cnt > 0 && to_write > 0)
     {
        copy = min(iop->_cnt, to_write);
        memcpy(iop->_ptr, ptr, copy);
        ptr += copy;
        iop->_ptr += copy;
        iop->_cnt -= copy;
        to_write -= copy;
        iop->_flag |= _IODIRTY;
     }

     if (to_write > 0)
     {
        // if the buffer is dirty it will have to be written now
        // otherwise the file pointer won't match anymore.
        fflush(iop);
        if (to_write >= iop->_bufsiz)
	{
	   while (to_write > 0)
	   {
	      n_written = _write(_fileno(iop), ptr, to_write);
	      if (n_written <= 0)
	      {
	         iop->_flag |= _IOERR;
		 break;
	      }
	      to_write -= n_written;
	      ptr += n_written;
	   }

           // check to see if this will work with in combination with ungetc

           // the file buffer is empty and there is no read ahead information anymore.
           iop->_flag &= ~_IOAHEAD;
	}
        else
	{
           if (EOF != _flsbuf(*ptr++, iop))
	   {
	      if (--to_write > 0)
	      {
                 memcpy(iop->_ptr, ptr, to_write);
                 iop->_ptr += to_write;
                 iop->_cnt -= to_write;
                 iop->_flag |= _IODIRTY;
	         return count;
	      }
	   }
	}
     }
  }

  return count - (to_write/size);
}
