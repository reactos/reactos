#include "precomp.h"
#include <io.h>
#include <errno.h>
#include <internal/file.h>


/*
 * @implemented
 */
int _dup(int handle)
{
  HANDLE hFile;
  HANDLE hProcess = GetCurrentProcess();
  BOOL result;
  int fd;

  hFile = (HANDLE)_get_osfhandle(handle);
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

  fd = alloc_fd(hFile, __fileno_getmode(handle));
  if (fd < 0)
  {
	  CloseHandle(hFile);
  }
  return fd;
}
