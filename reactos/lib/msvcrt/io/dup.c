/* $Id: dup.c,v 1.4 2002/09/08 10:22:50 chorns Exp $ */
#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>


int _dup(int handle)
{
  HANDLE hFile;
  HANDLE hProcess = GetCurrentProcess();
  BOOL result;
  int fd;
  
  hFile = _get_osfhandle(handle);
  if (hFile == INVALID_HANDLE_VALUE)
	  return -1;
  result = DuplicateHandle(hProcess, 
	                   hFile, 
			   hProcess, 
			   &hFile, 
			   0, 
			   TRUE, 
			   DUPLICATE_SAME_ACCESS);
  if (result == FALSE)
	  return -1;

  fd = __fileno_alloc(hFile, __fileno_getmode(handle));
  if (fd < 0)
  {
	  CloseHandle(hFile);
  }
  return fd;
}
