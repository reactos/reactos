#include <crtdll/stdio.h>
#include <windows.h>
#include <crtdll/internal/file.h>


int getc(FILE *fp)
{
  if(fp->_cnt > 0) {
	fp->_cnt--;
     	return (int)*fp->_ptr++;
  } 
  else {
     	return _filbuf(fp);
  }
  return -1;
}


