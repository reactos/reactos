#include <msvcrt/stdio.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>


size_t fread(void *vptr, size_t size, size_t count, FILE *iop)
{
  char *ptr = (char *)vptr;
  size_t  to_read ,n_read;

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

  while(iop->_cnt > 0 && to_read > 0 )
    {
      to_read--;
      *ptr++ = getc(iop);
    }

  // if the buffer is dirty it will have to be written now
  // otherwise the file pointer won't match anymore.

  fflush(iop);

  // check to see if this will work with in combination with ungetc

  n_read = _read(fileno(iop), ptr, to_read);
  if ( n_read != -1 )
    to_read -= n_read;

  // the file buffer is empty and there is no read ahead information anymore.
  iop->_flag &= ~_IOAHEAD;

  return count - (to_read/size);
}
