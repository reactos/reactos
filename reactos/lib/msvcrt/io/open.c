/* $Id: open.c,v 1.14 2002/12/26 17:26:41 robd Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/io/open.c
 * PURPOSE:     Opens a file and translates handles to fileno
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

// rember to interlock the allocation of fileno when making this thread safe

// possibly store extra information at the handle

#include <windows.h>
#if !defined(NDEBUG) && defined(DBG)
#include <msvcrt/stdarg.h>
#endif
#include <msvcrt/io.h>
#include <msvcrt/fcntl.h>
#include <msvcrt/sys/stat.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/internal/file.h>
#include <msvcrt/string.h>
#include <msvcrt/share.h>
#include <msvcrt/errno.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>

//#define _OLD_BUILD_

#define STD_AUX_HANDLE 3
#define STD_PRINTER_HANDLE 4


/////////////////////////////////////////
#if 0 // from perl sources

#ifndef _INTPTR_T_DEFINED
typedef int		intptr_t;
#define _INTPTR_T_DEFINED
#endif

#ifndef _UINTPTR_T_DEFINED
typedef unsigned int	uintptr_t;
#define _UINTPTR_T_DEFINED
#endif

/*
 * Control structure for lowio file handles
 */
typedef struct {
    intptr_t osfhnd;/* underlying OS file HANDLE */
    char osfile;    /* attributes of file (e.g., open in text mode?) */
    char pipech;    /* one char buffer for handles opened on pipes */
    int lockinitflag;
    //CRITICAL_SECTION lock;
} ioinfo;

/*
 * Array of arrays of control structures for lowio files.
 */
//ioinfo* __pioinfo[];
//ioinfo* __pioinfo[] = { NULL };

#endif
/////////////////////////////////////////

typedef struct _fileno_modes_type
{
    HANDLE hFile;
    int mode;
    char pipech;    /* one char buffer for handles opened on pipes */
    int lockinitflag;
    /*CRITICAL_SECTION*/int lock;
    int fd;
} fileno_modes_type;

//static fileno_modes_type* fileno_modes = NULL;
fileno_modes_type* __pioinfo = NULL;

/////////////////////////////////////////
int maxfno = 0;

char __is_text_file(FILE* p)
{
   if ( p == NULL || __pioinfo == NULL )
     return FALSE;
   return (!((p)->_flag&_IOSTRG) && (__pioinfo[(p)->_file].mode&O_TEXT));
}

int _open(const char* _path, int _oflag,...)
{
#if !defined(NDEBUG) && defined(DBG)
   va_list arg;
   int pmode;
#endif
   HANDLE hFile;
   DWORD dwDesiredAccess = 0;
   DWORD dwShareMode = 0;
   DWORD dwCreationDistribution = 0;
   DWORD dwFlagsAndAttributes = 0;
   DWORD dwLastError;
   SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

#if !defined(NDEBUG) && defined(DBG)
   va_start(arg, _oflag);
   pmode = va_arg(arg, int);
#endif

//   DPRINT("_open('%s', %x, (%x))\n", _path, _oflag, pmode);

   if ((_oflag & S_IREAD ) == S_IREAD)
     dwShareMode = FILE_SHARE_READ;
   else if ((_oflag & S_IWRITE) == S_IWRITE) {
      dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
   }
   /*
    *
    * _O_BINARY   Opens file in binary (untranslated) mode. (See fopen for a description of binary mode.)
    * _O_TEXT   Opens file in text (translated) mode. (For more information, see Text and Binary Mode File I/O and fopen.)
    *
    * _O_APPEND   Moves file pointer to end of file before every write operation.
    */
#ifdef _OLD_BUILD_
   if ((_oflag & _O_RDWR) == _O_RDWR)
     dwDesiredAccess |= GENERIC_WRITE|GENERIC_READ;
   else if ((_oflag & O_RDONLY) == O_RDONLY)
     dwDesiredAccess |= GENERIC_READ;
   else if ((_oflag & _O_WRONLY) == _O_WRONLY)
     dwDesiredAccess |= GENERIC_WRITE ;
#else
   if ((_oflag & _O_WRONLY) == _O_WRONLY )
     dwDesiredAccess |= GENERIC_WRITE ;
   else if ((_oflag & _O_RDWR) == _O_RDWR )
     dwDesiredAccess |= GENERIC_WRITE|GENERIC_READ;
   else //if ((_oflag & O_RDONLY) == O_RDONLY)
     dwDesiredAccess |= GENERIC_READ;
#endif
   if (( _oflag & S_IREAD ) == S_IREAD)
     dwShareMode |= FILE_SHARE_READ;
   
   if (( _oflag & S_IWRITE ) == S_IWRITE)
     dwShareMode |= FILE_SHARE_WRITE;

   if (( _oflag & (_O_CREAT | _O_EXCL)) == (_O_CREAT | _O_EXCL))
     dwCreationDistribution |= CREATE_NEW;

   else if ((_oflag &  O_TRUNC ) == O_TRUNC) {
      if ((_oflag &  O_CREAT ) ==  O_CREAT)
    dwCreationDistribution |= CREATE_ALWAYS;
      else if ((_oflag & O_RDONLY ) != O_RDONLY)
    dwCreationDistribution |= TRUNCATE_EXISTING;
   }
   else if ((_oflag & _O_APPEND) == _O_APPEND)
     dwCreationDistribution |= OPEN_EXISTING;
   else if ((_oflag &  _O_CREAT) == _O_CREAT)
     dwCreationDistribution |= OPEN_ALWAYS;
   else
     dwCreationDistribution |= OPEN_EXISTING;

   if ((_oflag &  _O_RANDOM) == _O_RANDOM )
     dwFlagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;
   if ((_oflag &  _O_SEQUENTIAL) == _O_SEQUENTIAL)
     dwFlagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;
   if ((_oflag &  _O_TEMPORARY) == _O_TEMPORARY) {
     dwFlagsAndAttributes |= FILE_FLAG_DELETE_ON_CLOSE;
     DPRINT("FILE_FLAG_DELETE_ON_CLOSE\n");
   }
   if ((_oflag &  _O_SHORT_LIVED) == _O_SHORT_LIVED) {
     dwFlagsAndAttributes |= FILE_FLAG_DELETE_ON_CLOSE;
     DPRINT("FILE_FLAG_DELETE_ON_CLOSE\n");
   }
   if (_oflag & _O_NOINHERIT)
     sa.bInheritHandle = FALSE;
   hFile = CreateFileA(_path,
               dwDesiredAccess,
               dwShareMode, 
               &sa,
               dwCreationDistribution,
               dwFlagsAndAttributes,
               NULL);
   if (hFile == (HANDLE)-1) {
     dwLastError = GetLastError();
     if (dwLastError == ERROR_ALREADY_EXISTS) {
        DPRINT("ERROR_ALREADY_EXISTS\n");
        __set_errno(EEXIST);
     } else {
        DPRINT("%x\n", dwLastError);
        __set_errno(ENOFILE);
     }
     return -1;
   }
   DPRINT("OK\n");
   if (!(_oflag & (_O_TEXT|_O_BINARY))) {
       _oflag |= _fmode;
   }
   return __fileno_alloc(hFile,_oflag);
}


int __fileno_alloc(HANDLE hFile, int mode)
{
  int i;
  /* Check for bogus values */
  if (hFile < 0)
    return -1;

  for (i = 5; i < maxfno; i++) {
    if (__pioinfo[i].fd == -1 ) {
        __pioinfo[i].fd = i;
        __pioinfo[i].mode = mode;
        __pioinfo[i].hFile = hFile;
        return i;
    }
  }

  /* See if we need to expand the tables.  Check this BEFORE it might fail,
     so that when we hit the count'th request, we've already up'd it. */
  if (i == maxfno) {
    int oldcount = maxfno;
    fileno_modes_type* old_fileno_modes = __pioinfo;
    maxfno += 255;
    __pioinfo = (fileno_modes_type*)malloc(maxfno * sizeof(fileno_modes_type));
    if (old_fileno_modes != NULL) {
        memcpy(__pioinfo, old_fileno_modes, oldcount * sizeof(fileno_modes_type));
        free(old_fileno_modes);
    }
    memset(__pioinfo + oldcount, -1, (maxfno-oldcount)*sizeof(fileno_modes_type));
  }

  /* Fill in the value */
  __pioinfo[i].fd = i;
  __pioinfo[i].mode = mode;
  __pioinfo[i].hFile = hFile;
  return i;
}

void* filehnd(int fileno)
{
    if (fileno < 0 || fileno >= maxfno || __pioinfo[fileno].fd == -1) {
        return (void*)-1;
    }
    return __pioinfo[fileno].hFile;
}

int __fileno_setmode(int _fd, int _newmode)
{
    int m;
    if (_fd < 0 || _fd >= maxfno) {
        __set_errno(EBADF);
        return -1;
    }
    m = __pioinfo[_fd].mode;
    __pioinfo[_fd].mode = _newmode;
    return m;
}

int __fileno_getmode(int _fd)
{
    if (_fd < 0 || _fd >= maxfno) {
        __set_errno(EBADF);
        return -1;
    }
    return __pioinfo[_fd].mode;

}

int __fileno_close(int _fd)
{
    if (_fd < 0 || _fd >= maxfno) {
        __set_errno(EBADF);
        return -1;
    }
    __pioinfo[_fd].fd = -1;
    __pioinfo[_fd].hFile = (HANDLE)-1;
    return 0;
}

int _open_osfhandle(void* osfhandle, int flags)
{
    return __fileno_alloc((HANDLE)osfhandle, flags);
}

void* _get_osfhandle( int fileno )
{
    return filehnd(fileno);
}

int __fileno_dup2(int handle1, int handle2)
{
   HANDLE hProcess;
   BOOL result;
   if (handle1 >= maxfno || handle1 < 0 || handle2 >= maxfno || handle2 < 0) {
      __set_errno(EBADF);
      return -1;
   }
   if (__pioinfo[handle1].fd == -1) {
      __set_errno(EBADF);
      return -1;
   }
   if (handle1 == handle2)
      return handle1;
   if (__pioinfo[handle2].fd != -1) {
      _close(handle2);
   }
   hProcess = GetCurrentProcess();
   result = DuplicateHandle(hProcess, 
                __pioinfo[handle1].hFile, 
                hProcess, 
                &__pioinfo[handle2].hFile, 
                0, 
                TRUE,  
                DUPLICATE_SAME_ACCESS);
   if (result) {
      __pioinfo[handle2].fd = handle2;
      __pioinfo[handle2].mode = __pioinfo[handle1].mode;
      switch (handle2) {
      case 0:
         SetStdHandle(STD_INPUT_HANDLE, __pioinfo[handle2].hFile);
         break;
      case 1:
         SetStdHandle(STD_OUTPUT_HANDLE, __pioinfo[handle2].hFile);
         break;
      case 2:
         SetStdHandle(STD_ERROR_HANDLE, __pioinfo[handle2].hFile);
         break;
      case 3:
         SetStdHandle(STD_AUX_HANDLE, __pioinfo[handle2].hFile);
         break;
      case 4:
         SetStdHandle(STD_AUX_HANDLE, __pioinfo[handle2].hFile);
         break;
      }
      return handle1;
   } else {
      __set_errno(EMFILE);  // Is this the correct error no.?
      return -1;
   }
}


void* malloc(size_t sizeObject);

BOOL __fileno_init(void)
{
   ULONG count = 0, i;
   HANDLE* pFile;
   char* pmode;
   STARTUPINFO StInfo;

   GetStartupInfoA(&StInfo);
   if (StInfo.lpReserved2 && StInfo.cbReserved2 >= sizeof(ULONG)) {
      count = *(ULONG*)StInfo.lpReserved2;
/*
      if (sizeof(ULONG) + count * (sizeof(HANDLE) + sizeof(char)) != StInfo.cbReserved2)
      {
          count = 0;
      }
*/
   }
   maxfno = 255;
   while(count >= maxfno)
      maxfno += 255;

   {
#ifdef _OLD_BUILD_
       // why was this here ???? - robd.
       int result;
       result = malloc(50);
#endif
   }
   //__pioinfo = (fileno_modes_type*)malloc(sizeof(fileno_modes_type) * maxfno);
   __pioinfo = malloc(sizeof(fileno_modes_type) * maxfno);
   if (__pioinfo == NULL) {
       return FALSE;
   }
   memset(__pioinfo, -1, sizeof(fileno_modes_type) * maxfno);
   if (count) {
      pFile = (HANDLE*)(StInfo.lpReserved2 + sizeof(ULONG) + count * sizeof(char));
      pmode = (char*)(StInfo.lpReserved2 + sizeof(ULONG));
      for (i = 0; i <  count; i++) {
          if (*pFile != INVALID_HANDLE_VALUE) {
             __pioinfo[i].fd = i;
             __pioinfo[i].mode = ((*pmode << 8) & (_O_TEXT|_O_BINARY)) | (*pmode & _O_ACCMODE);
             __pioinfo[i].hFile = *pFile;
          }
          pFile++;
          pmode++;
      }
   }
   if (__pioinfo[0].fd == -1) {
      __pioinfo[0].fd = 0;
      __pioinfo[0].hFile = GetStdHandle(STD_INPUT_HANDLE);
      __pioinfo[0].mode = _O_RDONLY|_O_TEXT;
   }
   if (__pioinfo[1].fd == -1) {
      __pioinfo[1].fd = 1;
      __pioinfo[1].hFile = GetStdHandle(STD_OUTPUT_HANDLE);
      __pioinfo[1].mode = _O_WRONLY|_O_TEXT;
   }
   if (__pioinfo[2].fd == -1) {
      __pioinfo[2].fd = 2;
      __pioinfo[2].hFile = GetStdHandle(STD_ERROR_HANDLE);
      __pioinfo[2].mode = _O_WRONLY|_O_TEXT;
   }
   if (__pioinfo[3].fd == -1) {
      __pioinfo[3].fd = 3;
      __pioinfo[3].hFile = GetStdHandle(STD_AUX_HANDLE);
      __pioinfo[3].mode = _O_WRONLY|_O_TEXT;
   }
   if (__pioinfo[4].fd == -1) {
      __pioinfo[4].fd = 4;
      __pioinfo[4].hFile = GetStdHandle(STD_PRINTER_HANDLE);
      __pioinfo[4].mode = _O_WRONLY|_O_TEXT;
   }
   return TRUE;
}
