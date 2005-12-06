#include <precomp.h>

/*
 * @implemented
 */
int _dup(int handle)
{
  HANDLE hFile;
  HANDLE hProcess = GetCurrentProcess();
  BOOL result;
  int fd;
  int mode;

  hFile = (HANDLE)_get_osfhandle(handle);
	if (hFile == INVALID_HANDLE_VALUE) {
		__set_errno(EBADF);
		return -1;
	}
  mode = __fileno_getmode(handle);
  result = DuplicateHandle(hProcess,
	                   hFile,
			   hProcess,
			   &hFile,
			   0,
                           mode & FNOINHERIT ? FALSE : TRUE,
			   DUPLICATE_SAME_ACCESS);
	if (result == FALSE) {
		_dosmaperr(GetLastError());
		return -1;
	}

  fd = alloc_fd(hFile, mode);
  if (fd < 0)
  {
	  CloseHandle(hFile);
  }
  return fd;
}
