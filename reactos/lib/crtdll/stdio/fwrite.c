#include <msvcrt/stdio.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>

#if 0
size_t
fwrite(const void *p, size_t size, size_t count, FILE *iop)
{
  char *ptr = (char *)p;
  size_t to_write;


  to_write = size * count;
 


  while ( to_write > 0 ) {
	if ( putc(*ptr,iop) == EOF )
		break;
	to_write--;
	ptr++;
  }
	


  return count -to_write/size;
 
}


#else
size_t fwrite(const void *vptr, size_t size, size_t count, FILE *iop)
 {
 	size_t to_write, n_written;
 	char *ptr = (char *)vptr;
 	
 	to_write = size*count;
 	if (!OPEN4WRITING(iop) )
	{
		__set_errno (EINVAL);
		return 0;
	}


	if (iop == NULL  )
	{
		__set_errno (EINVAL);
		return 0;
	}

	if (ferror (iop))
		return 0;
	if (vptr == NULL || to_write == 0)
		return 0;

 	
 	while(iop->_cnt > 0 && to_write > 0 ) {     
                to_write--;
                putc(*ptr++,iop);
        }

	// if the buffer is dirty it will have to be written now
	// otherwise the file pointer won't match anymore.
  
	fflush(iop);       
      
        n_written = _write(fileno(iop), ptr,to_write);
        if ( n_written != -1 )
        	to_write -= n_written;
        
        // check to see if this will work with in combination with ungetc


	// the file buffer is empty and there is no read ahead information anymore.
	
	iop->_flag &= ~_IOAHEAD;
        
        return count - (to_write/size);      
  
}

#endif
