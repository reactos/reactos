/* $Id: read.c,v 1.10 2003/07/11 17:25:16 royce Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/io/read.c
 * PURPOSE:     Reads a file
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/1998: Created
 */
#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>

/*
 * @implemented
 */
size_t	_read(int _fd, void *_buf, size_t _nbyte)
{
   DWORD _rbyte;
   
   if (!ReadFile(_get_osfhandle(_fd),_buf,_nbyte,&_rbyte,NULL))
     {
	return -1;
     }
   return (size_t)_rbyte;
}
