#include "precomp.h"
#include <msvcrt/io.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>


/*
 * @implemented
 */
int _dup(int handle)
{
  HANDLE hFile;
  HANDLE hProcess = GetCurrentProcess();
  BOOL result;
  int fd;
  
  hFile = _get_osfhandle(handle);
	if (hFile == INVALID_HANDLE_VALUE) {
		__set_errno(EBADF);
		return -1;
	}
  result = DuplicateHandle(hProcess, 
	                   hFile, 
			   hProcess, 
			   &hFile, 
			   0, 
			   TRUE, 
			   DUPLICATE_SAME_ACCESS);
	if (result == FALSE) {
		_dosmaperr(GetLastError());
		return -1;
	}

  fd = __fileno_alloc(hFile, __fileno_getmode(handle));
  if (fd < 0)
  {
	  CloseHandle(hFile);
  }
  return fd;
}
