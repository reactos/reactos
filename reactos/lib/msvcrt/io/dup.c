/* $Id: dup.c,v 1.5 2003/07/11 21:57:54 royce Exp $ */
#include <windows.h>
#include <msvcrt/io.h>
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
