#include <windows.h>
#include <crtdll/stdio.h>
#include <crtdll/wchar.h>
#include <crtdll/errno.h>
#include <crtdll/internal/file.h>

//getc can be a macro
#undef getc

int getc(FILE *fp)
{

// check for invalid stream

	if ( !__validfp (fp) ) {
		__set_errno(EINVAL);
		return EOF;
	}
// check for read access on stream

	if ( !OPEN4READING(fp) ) {
		__set_errno(EINVAL);
		return -1;
	}

	if(fp->_cnt > 0) {
		fp->_cnt--;
		return (int)*fp->_ptr++;
	} 
	else {
		return _filbuf(fp);
	}
	return -1;
}

// not exported

wint_t  getwc(FILE *fp)
{
	
 // might check on multi bytes if text mode
 
  if(fp->_cnt > 0) {
        fp->_cnt -= sizeof(wchar_t);
        return (wint_t )*((wchar_t *)(fp->_ptr))++;
  } 
  else {
	return _filwbuf(fp);
  }
  
  // never reached
  return -1;
}




