/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/io/open.c
 * PURPOSE:     Opens a file and translates handles to fileno
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

// rember to interlock the allocation of fileno when making this thread safe

// possibly store extra information at the handle

#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/fcntl.h>
#include <msvcrt/sys/stat.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/internal/file.h>
#include <msvcrt/string.h>
#include <msvcrt/share.h>

typedef struct _fileno_modes_type
{
	HANDLE hFile;
	int mode;
	int fd;
} fileno_modes_type;

fileno_modes_type *fileno_modes = NULL;

int maxfno = 5;
int minfno = 5;

char __is_text_file(FILE *p)
{
   if ( p == NULL || fileno_modes == NULL )
     return FALSE;
   return (!((p)->_flag&_IOSTRG) && (fileno_modes[(p)->_file].mode&O_TEXT));
}


int __fileno_alloc(HANDLE hFile, int mode);


int _open(const char *_path, int _oflag,...)
{
   HANDLE hFile;
   DWORD dwDesiredAccess = 0;
   DWORD dwShareMode = 0;
   DWORD dwCreationDistribution = 0;
   DWORD dwFlagsAndAttributes = 0;

   if (( _oflag & S_IREAD ) == S_IREAD)
     dwShareMode = FILE_SHARE_READ;
   else if ( ( _oflag & S_IWRITE) == S_IWRITE ) {
      dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
   }

   /*
    *
    * _O_BINARY   Opens file in binary (untranslated) mode. (See fopen for a description of binary mode.)
    * _O_TEXT   Opens file in text (translated) mode. (For more information, see Text and Binary Mode File I/O and fopen.)
    *
    * _O_APPEND   Moves file pointer to end of file before every write operation.
    */
   if (( _oflag & _O_RDWR ) == _O_RDWR ) 
     dwDesiredAccess |= GENERIC_WRITE|GENERIC_READ ;
   else if (( _oflag & O_RDONLY ) == O_RDONLY ) 
     dwDesiredAccess |= GENERIC_READ ;
   else if (( _oflag & _O_WRONLY ) == _O_WRONLY )
     dwDesiredAccess |= GENERIC_WRITE ;

   if (( _oflag & S_IREAD ) == S_IREAD )
     dwShareMode |= FILE_SHARE_READ;
   
   if (( _oflag & S_IWRITE ) == S_IWRITE )
     dwShareMode |= FILE_SHARE_WRITE;	

   if (( _oflag & (_O_CREAT | _O_EXCL ) ) == (_O_CREAT | _O_EXCL) )
     dwCreationDistribution |= CREATE_NEW;

   else if (( _oflag &  O_TRUNC ) == O_TRUNC  ) {
      if (( _oflag &  O_CREAT ) ==  O_CREAT ) 
	dwCreationDistribution |= CREATE_ALWAYS;
      else if (( _oflag & O_RDONLY ) != O_RDONLY ) 
	dwCreationDistribution |= TRUNCATE_EXISTING;
   }
   else if (( _oflag & _O_APPEND ) == _O_APPEND )
     dwCreationDistribution |= OPEN_EXISTING;
   else if (( _oflag &  _O_CREAT ) == _O_CREAT )
     dwCreationDistribution |= OPEN_ALWAYS;
   else
     dwCreationDistribution |= OPEN_EXISTING;
   
   if (( _oflag &  _O_RANDOM ) == _O_RANDOM )
     dwFlagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;	
   if (( _oflag &  _O_SEQUENTIAL ) == _O_SEQUENTIAL )
     dwFlagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;
   
   if (( _oflag &  _O_TEMPORARY ) == _O_TEMPORARY )
     dwFlagsAndAttributes |= FILE_FLAG_DELETE_ON_CLOSE;
   
   if (( _oflag &  _O_SHORT_LIVED ) == _O_SHORT_LIVED )
     dwFlagsAndAttributes |= FILE_FLAG_DELETE_ON_CLOSE;
   
   hFile = CreateFileA(_path,
		       dwDesiredAccess,
		       dwShareMode,	
		       NULL, 
		       dwCreationDistribution,	
		       dwFlagsAndAttributes,
		       NULL);
   if (hFile == (HANDLE)-1)
     return -1;
   return  __fileno_alloc(hFile,_oflag);
}


int _wopen(const wchar_t *_path, int _oflag,...)
{
   HANDLE hFile;
   DWORD dwDesiredAccess = 0;
   DWORD dwShareMode = 0;
   DWORD dwCreationDistribution = 0;
   DWORD dwFlagsAndAttributes = 0;

   if (( _oflag & S_IREAD ) == S_IREAD)
     dwShareMode = FILE_SHARE_READ;
   else if ( ( _oflag & S_IWRITE) == S_IWRITE ) {
      dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
   }

   /*
    *
    * _O_BINARY   Opens file in binary (untranslated) mode. (See fopen for a description of binary mode.)
    * _O_TEXT   Opens file in text (translated) mode. (For more information, see Text and Binary Mode File I/O and fopen.)
    * 
    * _O_APPEND   Moves file pointer to end of file before every write operation.
    */
   if (( _oflag & _O_RDWR ) == _O_RDWR )
     dwDesiredAccess |= GENERIC_WRITE|GENERIC_READ | FILE_READ_DATA |
                        FILE_WRITE_DATA | FILE_READ_ATTRIBUTES |
                        FILE_WRITE_ATTRIBUTES;
   else if (( _oflag & O_RDONLY ) == O_RDONLY )
     dwDesiredAccess |= GENERIC_READ | FILE_READ_DATA | FILE_READ_ATTRIBUTES
                     | FILE_WRITE_ATTRIBUTES;
   else if (( _oflag & _O_WRONLY ) == _O_WRONLY )
     dwDesiredAccess |= GENERIC_WRITE | FILE_WRITE_DATA |
                        FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES;

   if (( _oflag & S_IREAD ) == S_IREAD )
     dwShareMode |= FILE_SHARE_READ;
   
   if (( _oflag & S_IWRITE ) == S_IWRITE )
     dwShareMode |= FILE_SHARE_WRITE;

   if (( _oflag & (_O_CREAT | _O_EXCL ) ) == (_O_CREAT | _O_EXCL) )
     dwCreationDistribution |= CREATE_NEW;

   else if (( _oflag &  O_TRUNC ) == O_TRUNC  ) {
      if (( _oflag &  O_CREAT ) ==  O_CREAT )
	dwCreationDistribution |= CREATE_ALWAYS;
      else if (( _oflag & O_RDONLY ) != O_RDONLY )
	dwCreationDistribution |= TRUNCATE_EXISTING;
   }
   else if (( _oflag & _O_APPEND ) == _O_APPEND )
     dwCreationDistribution |= OPEN_EXISTING;
   else if (( _oflag &  _O_CREAT ) == _O_CREAT )
     dwCreationDistribution |= OPEN_ALWAYS;
   else
     dwCreationDistribution |= OPEN_EXISTING;
   
   if (( _oflag &  _O_RANDOM ) == _O_RANDOM )
     dwFlagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;
   if (( _oflag &  _O_SEQUENTIAL ) == _O_SEQUENTIAL )
     dwFlagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;
   
   if (( _oflag &  _O_TEMPORARY ) == _O_TEMPORARY )
     dwFlagsAndAttributes |= FILE_FLAG_DELETE_ON_CLOSE;
   
   if (( _oflag &  _O_SHORT_LIVED ) == _O_SHORT_LIVED )
     dwFlagsAndAttributes |= FILE_FLAG_DELETE_ON_CLOSE;
   
   hFile = CreateFileW(_path,
		       dwDesiredAccess,
		       dwShareMode,
		       NULL,
		       dwCreationDistribution,
		       dwFlagsAndAttributes,
		       NULL);
   if (hFile == (HANDLE)-1)
     return -1;
   return __fileno_alloc(hFile,_oflag);
}


int
__fileno_alloc(HANDLE hFile, int mode)
{
  int i;
  /* Check for bogus values */
  if (hFile < 0)
	return -1;

  for(i=minfno;i<maxfno;i++) {
	if (fileno_modes[i].fd == -1 ) {
		fileno_modes[i].fd = i;
		fileno_modes[i].mode = mode;
		fileno_modes[i].hFile = hFile;
		return i;
	}
  }

  /* See if we need to expand the tables.  Check this BEFORE it might fail,
     so that when we hit the count'th request, we've already up'd it. */
  if ( i == maxfno)
  {
    int oldcount = maxfno;
    fileno_modes_type *old_fileno_modes = fileno_modes;
	maxfno  += 255;
    fileno_modes = (fileno_modes_type *)malloc(maxfno * sizeof(fileno_modes_type));
    if ( old_fileno_modes != NULL )
    {
	memcpy(fileno_modes, old_fileno_modes, oldcount * sizeof(fileno_modes_type));
        free ( old_fileno_modes );
    }
    memset(fileno_modes + oldcount, 0, (maxfno-oldcount)*sizeof(fileno_modes));
  }

  /* Fill in the value */
  fileno_modes[i].fd = i;
  fileno_modes[i].mode = mode;
  fileno_modes[i].hFile = hFile;
  return i;
}

void *filehnd(int fileno)
{
	if ( fileno < 0 )
		return (void *)-1;
#define STD_AUX_HANDLE 3
#define STD_PRINTER_HANDLE 4

	switch(fileno)
	{
	case 0:
		return GetStdHandle(STD_INPUT_HANDLE);
	case 1:
		return GetStdHandle(STD_OUTPUT_HANDLE);
	case 2:
		return GetStdHandle(STD_ERROR_HANDLE);
	case 3:
		return GetStdHandle(STD_AUX_HANDLE);
	case 4:
		return GetStdHandle(STD_PRINTER_HANDLE);
	default:
		break;
	}

	if ( fileno >= maxfno )
		return (void *)-1;

	if ( fileno_modes[fileno].fd == -1 )
		return (void *)-1;
	return fileno_modes[fileno].hFile;
}

int __fileno_dup2( int handle1, int handle2 )
{
	if ( handle1 >= maxfno )
		return -1;

	if ( handle1 < 0 )
		return -1;
	if ( handle2 >= maxfno )
		return -1;

	if ( handle2 < 0 )
		return -1;

	memcpy(&fileno_modes[handle1],&fileno_modes[handle2],sizeof(fileno_modes));

	return handle1;
}

int __fileno_setmode(int _fd, int _newmode)
{
	int m;
	if ( _fd < minfno )
		return -1;

	if ( _fd >= maxfno )
		return -1;

	m = fileno_modes[_fd].mode;
	fileno_modes[_fd].mode = _newmode;
	return m;
}

int __fileno_getmode(int _fd)
{
	if ( _fd < minfno )
		return -1;

	if ( _fd >= maxfno )
		return -1;

	return fileno_modes[_fd].mode;

}


int __fileno_close(int _fd)
{
	if ( _fd < 0 )
		return -1;

	if ( _fd >= maxfno )
		return -1;

	fileno_modes[_fd].fd = -1;
	fileno_modes[_fd].hFile = (HANDLE)-1;
	return 0;
}

int _open_osfhandle (void *osfhandle, int flags )
{
	return __fileno_alloc((HANDLE)osfhandle, flags);
}

void *_get_osfhandle( int fileno )
{
	return filehnd(fileno);
}
