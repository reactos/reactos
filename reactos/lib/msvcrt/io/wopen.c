/* $Id: wopen.c,v 1.3 2003/07/11 21:57:54 royce Exp $
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


/*
 * @implemented
 */
int _wopen(const wchar_t* _path, int _oflag, ...)
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
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

#if !defined(NDEBUG) && defined(DBG)
    va_start(arg, _oflag);
    pmode = va_arg(arg, int);
#endif

//    DPRINT("_wopen('%S', %x, (%x))\n", _path, _oflag, pmode);

    if ((_oflag & S_IREAD) == S_IREAD)
        dwShareMode = FILE_SHARE_READ;
    else if ( ( _oflag & S_IWRITE) == S_IWRITE) {
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    }

   /*
    *
    * _O_BINARY   Opens file in binary (untranslated) mode. (See fopen for a description of binary mode.)
    * _O_TEXT   Opens file in text (translated) mode. (For more information, see Text and Binary Mode File I/O and fopen.)
    * 
    * _O_APPEND   Moves file pointer to end of file before every write operation.
    */
#if 0
    if ((_oflag & _O_RDWR) == _O_RDWR)
        dwDesiredAccess |= GENERIC_WRITE|GENERIC_READ | FILE_READ_DATA |
                           FILE_WRITE_DATA | FILE_READ_ATTRIBUTES |
                           FILE_WRITE_ATTRIBUTES;
    else if ((_oflag & O_RDONLY) == O_RDONLY)
        dwDesiredAccess |= GENERIC_READ | FILE_READ_DATA | FILE_READ_ATTRIBUTES |
                           FILE_WRITE_ATTRIBUTES;
    else if ((_oflag & _O_WRONLY) == _O_WRONLY)
        dwDesiredAccess |= GENERIC_WRITE | FILE_WRITE_DATA |
                           FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES;
#else
    if ((_oflag & _O_WRONLY) == _O_WRONLY)
        dwDesiredAccess |= GENERIC_WRITE | FILE_WRITE_DATA |
                           FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES;
    else if ((_oflag & _O_RDWR) == _O_RDWR)
        dwDesiredAccess |= GENERIC_WRITE|GENERIC_READ | FILE_READ_DATA |
                           FILE_WRITE_DATA | FILE_READ_ATTRIBUTES |
                           FILE_WRITE_ATTRIBUTES;
    else //if ((_oflag & O_RDONLY) == O_RDONLY)
        dwDesiredAccess |= GENERIC_READ | FILE_READ_DATA | FILE_READ_ATTRIBUTES |
                           FILE_WRITE_ATTRIBUTES;
#endif

    if ((_oflag & S_IREAD) == S_IREAD)
        dwShareMode |= FILE_SHARE_READ;

    if ((_oflag & S_IWRITE) == S_IWRITE)
        dwShareMode |= FILE_SHARE_WRITE;

    if ((_oflag & (_O_CREAT | _O_EXCL)) == (_O_CREAT | _O_EXCL))
        dwCreationDistribution |= CREATE_NEW;

    else if ((_oflag &  O_TRUNC) == O_TRUNC) {
        if ((_oflag &  O_CREAT) ==  O_CREAT)
            dwCreationDistribution |= CREATE_ALWAYS;
        else if ((_oflag & O_RDONLY) != O_RDONLY)
            dwCreationDistribution |= TRUNCATE_EXISTING;
    }
    else if ((_oflag & _O_APPEND) == _O_APPEND)
        dwCreationDistribution |= OPEN_EXISTING;
    else if ((_oflag &  _O_CREAT) == _O_CREAT)
        dwCreationDistribution |= OPEN_ALWAYS;
    else
        dwCreationDistribution |= OPEN_EXISTING;

    if ((_oflag &  _O_RANDOM) == _O_RANDOM)
        dwFlagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;
    if ((_oflag &  _O_SEQUENTIAL) == _O_SEQUENTIAL)
        dwFlagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;

    if ((_oflag &  _O_TEMPORARY) == _O_TEMPORARY)
        dwFlagsAndAttributes |= FILE_FLAG_DELETE_ON_CLOSE;

    if ((_oflag &  _O_SHORT_LIVED) == _O_SHORT_LIVED)
        dwFlagsAndAttributes |= FILE_FLAG_DELETE_ON_CLOSE;

    if (_oflag & _O_NOINHERIT)
        sa.bInheritHandle = FALSE;

    hFile = CreateFileW(_path,
               dwDesiredAccess,
               dwShareMode,
               &sa,
               dwCreationDistribution,
               dwFlagsAndAttributes,
               NULL);
    if (hFile == (HANDLE)-1)
        return -1;
    return __fileno_alloc(hFile,_oflag);
}

/*
 * @implemented
 */
int _wsopen(wchar_t* path, int access, int shflag, int mode)
{
    return _wopen((path), (access)|(shflag), (mode));
}
