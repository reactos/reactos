#include <crtdll/stdio.h>
#include <crtdll/stdlib.h>
#include <crtdll/string.h>
#include <crtdll/errno.h>
#include <crtdll/internal/file.h>

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
 	//if (!WRITE_STREAM(iop) )
	//{
	//	__set_errno (EINVAL);
	//	return 0;
	//}


	if (iop == NULL  )
	{
		__set_errno (EINVAL);
		return 0;
	}

	if (ferror (iop))
		return 0;
	if (vptr == NULL || to_write == 0)
		return 0;

 	
 	while(iop->_cnt > 0 ) {     
                to_write--;
                putc(*ptr++,iop);
        }
       
      
        n_written = _write(fileno(iop), ptr,to_write);
        if ( n_written != -1 )
        	to_write -= n_written;
        
        // check to see if this will work with in combination with ungetc
        
         return count - (to_write/size);      
  
}

#endif