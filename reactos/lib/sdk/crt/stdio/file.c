/*
 * PROJECT:         ReactOS CRT library
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            lib/sdk/crt/stdio/file.c
 * PURPOSE:         File CRT functions
 * PROGRAMMERS:     Wine team
 *                  Ported to ReactOS by Aleksey Bragin (aleksey@reactos.org)
 */

/*********************************************
 * This file contains ReactOS changes!!
 * Don't blindly sync it with Wine code!
 *
 * If you break Unicode output on the console again, please update this counter:
 *   int hours_wasted_on_this = 42;
 *********************************************/

/*
 * msvcrt.dll file functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 * Copyright 2004 Eric Pouech
 * Copyright 2004 Juan Lang
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * TODO
 * Use the file flag hints O_SEQUENTIAL, O_RANDOM, O_SHORT_LIVED
 */

#include <precomp.h>
#include "wine/unicode.h"

#include <sys/utime.h>
#include <direct.h>

int *__p__fmode(void);
int *__p___mb_cur_max(void);

#ifdef feof
#undef feof
#endif
#ifdef _fileno
#undef _fileno
#endif
#ifdef ferror
#undef ferror
#endif
#ifdef clearerr
#undef clearerr
#endif

#undef getc
#undef getwc
#undef getchar
#undef getwchar
#undef putc
#undef putwc
#undef putchar
#undef putwchar

#undef vprintf
#undef vwprintf

/* _access() bit flags FIXME: incomplete */
/* defined in crt/io.h */

/* values for wxflag in file descriptor */
#define WX_OPEN           0x01
#define WX_ATEOF          0x02
#define WX_READEOF        0x04  /* like ATEOF, but for underlying file rather than buffer */
#define WX_READCR         0x08  /* underlying file is at \r */
#define WX_DONTINHERIT    0x10
#define WX_APPEND         0x20
#define WX_TEXT           0x80

/* FIXME: this should be allocated dynamically */
#define MAX_FILES 2048
#define FD_BLOCK_SIZE 64

typedef struct {
    HANDLE              handle;
    unsigned char       wxflag;
    char                unk1;
    BOOL                crit_init;
    CRITICAL_SECTION    crit;
} ioinfo;

/*********************************************************************
 *		__pioinfo (MSVCRT.@)
 * array of pointers to ioinfo arrays [64]
 */
ioinfo * __pioinfo[MAX_FILES/FD_BLOCK_SIZE] = { 0 };

/*********************************************************************
 *		__badioinfo (MSVCRT.@)
 */
ioinfo __badioinfo = { INVALID_HANDLE_VALUE, WX_TEXT };

static int fdstart = 3; /* first unallocated fd */
static int fdend = 3; /* highest allocated fd */

typedef struct {
    FILE file;
    CRITICAL_SECTION crit;
} file_crit;

FILE _iob[_IOB_ENTRIES] = { { 0 } };
static file_crit* fstream[MAX_FILES/FD_BLOCK_SIZE] = { NULL };
static int max_streams = 512, stream_idx;

/* INTERNAL: process umask */
static int MSVCRT_umask = 0;

/* INTERNAL: static data for tmpnam and _wtmpname functions */
static int tmpnam_unique;

/* This critical section protects the tables __pioinfo and fstreams,
 * and their related indexes, fdstart, fdend,
 * and stream_idx, from race conditions.
 * It doesn't protect against race conditions manipulating the underlying files
 * or flags; doing so would probably be better accomplished with per-file
 * protection, rather than locking the whole table for every change.
 */
static CRITICAL_SECTION file_cs;
#define LOCK_FILES()    do { EnterCriticalSection(&file_cs); } while (0)
#define UNLOCK_FILES()  do { LeaveCriticalSection(&file_cs); } while (0)

static inline ioinfo* get_ioinfo(int fd)
{
    ioinfo *ret = NULL;
    if(fd < MAX_FILES)
        ret = __pioinfo[fd/FD_BLOCK_SIZE];
    if(!ret)
        return &__badioinfo;

    return ret + (fd%FD_BLOCK_SIZE);
}

static inline FILE* get_file(int i)
{
    file_crit *ret;

    if(i >= max_streams)
        return NULL;

    if(i < _IOB_ENTRIES)
        return &_iob[i];

    ret = fstream[i/FD_BLOCK_SIZE];
    if(!ret) {
        fstream[i/FD_BLOCK_SIZE] = calloc(FD_BLOCK_SIZE, sizeof(file_crit));
        if(!fstream[i/FD_BLOCK_SIZE]) {
            ERR("out of memory\n");
            *_errno() = ENOMEM;
            return NULL;
        }

        ret = fstream[i/FD_BLOCK_SIZE] + (i%FD_BLOCK_SIZE);
    } else
        ret += i%FD_BLOCK_SIZE;

    return &ret->file;
}

static inline BOOL is_valid_fd(int fd)
{
    return fd >= 0 && fd < fdend && (get_ioinfo(fd)->wxflag & WX_OPEN);
}

/* INTERNAL: Get the HANDLE for a fd
 * This doesn't lock the table, because a failure will result in
 * INVALID_HANDLE_VALUE being returned, which should be handled correctly.  If
 * it returns a valid handle which is about to be closed, a subsequent call
 * will fail, most likely in a sane way.
 */
/*static*/ HANDLE fdtoh(int fd)
{
  if (!is_valid_fd(fd))
  {
    WARN(":fd (%d) - no handle!\n",fd);
    *__doserrno() = 0;
    *_errno() = EBADF;
    return INVALID_HANDLE_VALUE;
  }
  //if (get_ioinfo(fd)->handle == INVALID_HANDLE_VALUE)
      //FIXME("returning INVALID_HANDLE_VALUE for %d\n", fd);
  return get_ioinfo(fd)->handle;
}

/* INTERNAL: free a file entry fd */
static void free_fd(int fd)
{
  HANDLE old_handle;
  ioinfo *fdinfo;

  LOCK_FILES();
  fdinfo = get_ioinfo(fd);
  old_handle = fdinfo->handle;
  if(fdinfo != &__badioinfo)
  {
    fdinfo->handle = INVALID_HANDLE_VALUE;
    fdinfo->wxflag = 0;
  }
  TRACE(":fd (%d) freed\n",fd);
  if (fd < 3) /* don't use 0,1,2 for user files */
  {
    switch (fd)
    {
    case 0:
        if (GetStdHandle(STD_INPUT_HANDLE) == old_handle) SetStdHandle(STD_INPUT_HANDLE, 0);
        break;
    case 1:
        if (GetStdHandle(STD_OUTPUT_HANDLE) == old_handle) SetStdHandle(STD_OUTPUT_HANDLE, 0);
        break;
    case 2:
        if (GetStdHandle(STD_ERROR_HANDLE) == old_handle) SetStdHandle(STD_ERROR_HANDLE, 0);
        break;
    }
  }
  else
  {
    if (fd == fdend - 1)
      fdend--;
    if (fd < fdstart)
      fdstart = fd;
  }
  UNLOCK_FILES();
}

/* INTERNAL: Allocate an fd slot from a Win32 HANDLE, starting from fd */
/* caller must hold the files lock */
static int alloc_fd_from(HANDLE hand, int flag, int fd)
{
  ioinfo *fdinfo;

  if (fd >= MAX_FILES)
  {
    WARN(":files exhausted!\n");
    *_errno() = ENFILE;
    return -1;
  }

  fdinfo = get_ioinfo(fd);
  if(fdinfo == &__badioinfo) {
    int i;

    __pioinfo[fd/FD_BLOCK_SIZE] = calloc(FD_BLOCK_SIZE, sizeof(ioinfo));
    if(!__pioinfo[fd/FD_BLOCK_SIZE]) {
      WARN(":out of memory!\n");
      *_errno() = ENOMEM;
      return -1;
    }

    for(i=0; i<FD_BLOCK_SIZE; i++)
      __pioinfo[fd/FD_BLOCK_SIZE][i].handle = INVALID_HANDLE_VALUE;

    fdinfo = get_ioinfo(fd);
  }

  fdinfo->handle = hand;
  fdinfo->wxflag = WX_OPEN | (flag & (WX_DONTINHERIT | WX_APPEND | WX_TEXT));

  /* locate next free slot */
  if (fd == fdstart && fd == fdend)
    fdstart = fdend + 1;
  else
    while (fdstart < fdend &&
      get_ioinfo(fdstart)->handle != INVALID_HANDLE_VALUE)
      fdstart++;
  /* update last fd in use */
  if (fd >= fdend)
    fdend = fd + 1;
  TRACE("fdstart is %d, fdend is %d\n", fdstart, fdend);

  switch (fd)
  {
  case 0: SetStdHandle(STD_INPUT_HANDLE,  hand); break;
  case 1: SetStdHandle(STD_OUTPUT_HANDLE, hand); break;
  case 2: SetStdHandle(STD_ERROR_HANDLE,  hand); break;
  }

  return fd;
}

/* INTERNAL: Allocate an fd slot from a Win32 HANDLE */
/*static*/ int alloc_fd(HANDLE hand, int flag)
{
  int ret;

  LOCK_FILES();
  TRACE(":handle (%p) allocating fd (%d)\n",hand,fdstart);
  ret = alloc_fd_from(hand, flag, fdstart);
  UNLOCK_FILES();
  return ret;
}

/* INTERNAL: Allocate a FILE* for an fd slot */
/* caller must hold the files lock */
static FILE* alloc_fp(void)
{
  unsigned int i;
  FILE *file;

  for (i = 3; i < (unsigned int)max_streams; i++)
  {
    file = get_file(i);
    if (!file)
      return NULL;

    if (file->_flag == 0)
    {
      if (i == stream_idx) stream_idx++;
      return file;
    }
  }

  return NULL;
}

/* INTERNAL: initialize a FILE* from an open fd */
static int init_fp(FILE* file, int fd, unsigned stream_flags)
{
  TRACE(":fd (%d) allocating FILE*\n",fd);
  if (!is_valid_fd(fd))
  {
    WARN(":invalid fd %d\n",fd);
    *__doserrno() = 0;
    *_errno() = EBADF;
    return -1;
  }
  memset(file, 0, sizeof(*file));
  file->_file = fd;
  file->_flag = stream_flags;

  if(file<_iob || file>=_iob+_IOB_ENTRIES)
      InitializeCriticalSection(&((file_crit*)file)->crit);

  TRACE(":got FILE* (%p)\n",file);
  return 0;
}

/* INTERNAL: Create an inheritance data block (for spawned process)
 * The inheritance block is made of:
 *      00      int     nb of file descriptor (NBFD)
 *      04      char    file flags (wxflag): repeated for each fd
 *      4+NBFD  HANDLE  file handle: repeated for each fd
 */
unsigned create_io_inherit_block(WORD *size, BYTE **block)
{
  int         fd;
  char*       wxflag_ptr;
  HANDLE*     handle_ptr;
  ioinfo*     fdinfo;

  *size = sizeof(unsigned) + (sizeof(char) + sizeof(HANDLE)) * fdend;
  *block = calloc(*size, 1);
  if (!*block)
  {
    *size = 0;
    return FALSE;
  }
  wxflag_ptr = (char*)*block + sizeof(unsigned);
  handle_ptr = (HANDLE*)(wxflag_ptr + fdend * sizeof(char));

  *(unsigned*)*block = fdend;
  for (fd = 0; fd < fdend; fd++)
  {
    /* to be inherited, we need it to be open, and that DONTINHERIT isn't set */
    fdinfo = get_ioinfo(fd);
    if ((fdinfo->wxflag & (WX_OPEN | WX_DONTINHERIT)) == WX_OPEN)
    {
      *wxflag_ptr = fdinfo->wxflag;
      *handle_ptr = fdinfo->handle;
    }
    else
    {
      *wxflag_ptr = 0;
      *handle_ptr = INVALID_HANDLE_VALUE;
    }
    wxflag_ptr++; handle_ptr++;
  }
  return TRUE;
}

/* INTERNAL: Set up all file descriptors,
 * as well as default streams (stdin, stderr and stdout)
 */
void msvcrt_init_io(void)
{
  STARTUPINFOA  si;
  unsigned int  i;
  ioinfo        *fdinfo;

  InitializeCriticalSection(&file_cs);
  file_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": file_cs");
  GetStartupInfoA(&si);
  if (si.cbReserved2 >= sizeof(unsigned int) && si.lpReserved2 != NULL)
  {
    BYTE*       wxflag_ptr;
    HANDLE*     handle_ptr;
    unsigned int count;

    count = *(unsigned*)si.lpReserved2;
    wxflag_ptr = si.lpReserved2 + sizeof(unsigned);
    handle_ptr = (HANDLE*)(wxflag_ptr + count);

    count = min(count, (si.cbReserved2 - sizeof(unsigned)) / (sizeof(HANDLE) + 1));
    count = min(count, MAX_FILES);
    for (i = 0; i < count; i++)
    {
      if ((*wxflag_ptr & WX_OPEN) && *handle_ptr != INVALID_HANDLE_VALUE)
        alloc_fd_from(*handle_ptr, *wxflag_ptr, i);

      wxflag_ptr++; handle_ptr++;
    }
    fdend = max( 3, count );
    for (fdstart = 3; fdstart < fdend; fdstart++)
        if (get_ioinfo(fdstart)->handle == INVALID_HANDLE_VALUE) break;
  }

  if(!__pioinfo[0])
      alloc_fd_from(INVALID_HANDLE_VALUE, 0, 3);

  fdinfo = get_ioinfo(0);
  if (!(fdinfo->wxflag & WX_OPEN) || fdinfo->handle == INVALID_HANDLE_VALUE)
  {
      HANDLE std = GetStdHandle(STD_INPUT_HANDLE);
#ifndef __REACTOS__
      if (std != INVALID_HANDLE_VALUE && DuplicateHandle(GetCurrentProcess(), std,
                                                         GetCurrentProcess(), &fdinfo->handle,
                                                         0, TRUE, DUPLICATE_SAME_ACCESS))
#else
          fdinfo->handle = std;
#endif
          fdinfo->wxflag = WX_OPEN | WX_TEXT;
  }

  fdinfo = get_ioinfo(1);
  if (!(fdinfo->wxflag & WX_OPEN) || fdinfo->handle == INVALID_HANDLE_VALUE)
  {
      HANDLE std = GetStdHandle(STD_OUTPUT_HANDLE);
#ifndef __REACTOS__
      if (std != INVALID_HANDLE_VALUE && DuplicateHandle(GetCurrentProcess(), std,
                                                         GetCurrentProcess(), &fdinfo->handle,
                                                         0, TRUE, DUPLICATE_SAME_ACCESS))
#else
          fdinfo->handle = std;
#endif
          fdinfo->wxflag = WX_OPEN | WX_TEXT;
  }

  fdinfo = get_ioinfo(2);
  if (!(fdinfo->wxflag & WX_OPEN) || fdinfo->handle == INVALID_HANDLE_VALUE)
  {
      HANDLE std = GetStdHandle(STD_ERROR_HANDLE);
#ifndef __REACTOS__
      if (std != INVALID_HANDLE_VALUE && DuplicateHandle(GetCurrentProcess(), std,
                                                         GetCurrentProcess(), &fdinfo->handle,
                                                         0, TRUE, DUPLICATE_SAME_ACCESS))
#else
          fdinfo->handle = std;
#endif
          fdinfo->wxflag = WX_OPEN | WX_TEXT;
  }

  TRACE(":handles (%p)(%p)(%p)\n", get_ioinfo(0)->handle,
	get_ioinfo(1)->handle, get_ioinfo(2)->handle);

  memset(_iob,0,3*sizeof(FILE));
  for (i = 0; i < 3; i++)
  {
    /* FILE structs for stdin/out/err are static and never deleted */
    _iob[i]._file = i;
    _iob[i]._tmpfname = NULL;
    _iob[i]._flag = (i == 0) ? _IOREAD : _IOWRT;
  }
  stream_idx = 3;
}

/* INTERNAL: Flush stdio file buffer */
static int flush_buffer(FILE* file)
{
  if(file->_bufsiz) {
        int cnt=file->_ptr-file->_base;
        if(cnt>0 && _write(file->_file, file->_base, cnt) != cnt) {
            file->_flag |= _IOERR;
            return EOF;
        }
        file->_ptr=file->_base;
        file->_cnt=file->_bufsiz;
  }
  return 0;
}

/* INTERNAL: Allocate stdio file buffer */
/*static*/ void alloc_buffer(FILE* file)
{
	file->_base = calloc(BUFSIZ,1);
	if(file->_base) {
		file->_bufsiz = BUFSIZ;
		file->_flag |= _IOMYBUF;
	} else {
		file->_base = (char*)(&file->_charbuf);
		/* put here 2 ??? */
		file->_bufsiz = sizeof(file->_charbuf);
	}
	file->_ptr = file->_base;
	file->_cnt = 0;
}

/* INTERNAL: Convert integer to base32 string (0-9a-v), 0 becomes "" */
static int int_to_base32(int num, char *str)
{
  char *p;
  int n = num;
  int digits = 0;

  while (n != 0)
  {
    n >>= 5;
    digits++;
  }
  p = str + digits;
  *p = 0;
  while (--p >= str)
  {
    *p = (num & 31) + '0';
    if (*p > '9')
      *p += ('a' - '0' - 10);
    num >>= 5;
  }

  return digits;
}

/* INTERNAL: wide character version of int_to_base32 */
static int int_to_base32_w(int num, wchar_t *str)
{
    wchar_t *p;
    int n = num;
    int digits = 0;

    while (n != 0)
    {
        n >>= 5;
        digits++;
    }
    p = str + digits;
    *p = 0;
    while (--p >= str)
    {
        *p = (num & 31) + '0';
        if (*p > '9')
            *p += ('a' - '0' - 10);
        num >>= 5;
    }

    return digits;
}

/* INTERNAL: Create a wide string from an ascii string */
wchar_t *msvcrt_wstrdupa(const char *str)
{
  const unsigned int len = strlen(str) + 1 ;
  wchar_t *wstr = malloc(len* sizeof (wchar_t));
  if (!wstr)
    return NULL;
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,str,len,wstr,len);
  return wstr;
}

/*********************************************************************
 *		__iob_func(MSVCRT.@)
 */
FILE * CDECL __iob_func(void)
{
 return &_iob[0];
}

/*********************************************************************
 *		_access (MSVCRT.@)
 */
int CDECL _access(const char *filename, int mode)
{
  DWORD attr = GetFileAttributesA(filename);

  TRACE("(%s,%d) %d\n",filename,mode,attr);

  if (!filename || attr == INVALID_FILE_ATTRIBUTES)
  {
    _dosmaperr(GetLastError());
    return -1;
  }
  if ((attr & FILE_ATTRIBUTE_READONLY) && (mode & W_OK))
  {
    _set_errno(ERROR_ACCESS_DENIED);
    return -1;
  }
  return 0;
}

/*********************************************************************
 *		_access_s (MSVCRT.@)
 */
int CDECL _access_s(const char *filename, int mode)
{
  if (!MSVCRT_CHECK_PMT(filename != NULL) ||
      !MSVCRT_CHECK_PMT((mode & ~(R_OK | W_OK)) == 0))
  {
     _set_errno(EINVAL);
     return -1;
  }

  return _access(filename, mode);
}

/*********************************************************************
 *		_waccess (MSVCRT.@)
 */
int CDECL _waccess(const wchar_t *filename, int mode)
{
  DWORD attr = GetFileAttributesW(filename);

  TRACE("(%s,%d) %d\n",debugstr_w(filename),mode,attr);

  if (!filename || attr == INVALID_FILE_ATTRIBUTES)
  {
    _dosmaperr(GetLastError());
    return -1;
  }
  if ((attr & FILE_ATTRIBUTE_READONLY) && (mode & W_OK))
  {
    _set_errno(ERROR_ACCESS_DENIED);
    return -1;
  }
  return 0;
}

/*********************************************************************
 *		_waccess_s (MSVCRT.@)
 */
int CDECL _waccess_s(const wchar_t *filename, int mode)
{
  if (!MSVCRT_CHECK_PMT(filename != NULL) ||
      !MSVCRT_CHECK_PMT((mode & ~(R_OK | W_OK)) == 0))
  {
     *_errno() = EINVAL;
     return -1;
  }

  return _waccess(filename, mode);
}

/*********************************************************************
 *		_chmod (MSVCRT.@)
 */
int CDECL _chmod(const char *path, int flags)
{
  DWORD oldFlags = GetFileAttributesA(path);

  if (oldFlags != INVALID_FILE_ATTRIBUTES)
  {
    DWORD newFlags = (flags & _S_IWRITE)? oldFlags & ~FILE_ATTRIBUTE_READONLY:
      oldFlags | FILE_ATTRIBUTE_READONLY;

    if (newFlags == oldFlags || SetFileAttributesA(path, newFlags))
      return 0;
  }
  _dosmaperr(GetLastError());
  return -1;
}

/*********************************************************************
 *		_wchmod (MSVCRT.@)
 */
int CDECL _wchmod(const wchar_t *path, int flags)
{
  DWORD oldFlags = GetFileAttributesW(path);

  if (oldFlags != INVALID_FILE_ATTRIBUTES)
  {
    DWORD newFlags = (flags & _S_IWRITE)? oldFlags & ~FILE_ATTRIBUTE_READONLY:
      oldFlags | FILE_ATTRIBUTE_READONLY;

    if (newFlags == oldFlags || SetFileAttributesW(path, newFlags))
      return 0;
  }
  _dosmaperr(GetLastError());
  return -1;
}

/*********************************************************************
 *		_unlink (MSVCRT.@)
 */
int CDECL _unlink(const char *path)
{
  TRACE("%s\n",debugstr_a(path));
  if(DeleteFileA(path))
    return 0;
  TRACE("failed (%d)\n",GetLastError());
  _dosmaperr(GetLastError());
  return -1;
}

/*********************************************************************
 *		_wunlink (MSVCRT.@)
 */
int CDECL _wunlink(const wchar_t *path)
{
  TRACE("(%s)\n",debugstr_w(path));
  if(DeleteFileW(path))
    return 0;
  TRACE("failed (%d)\n",GetLastError());
  _dosmaperr(GetLastError());
  return -1;
}

/* _flushall calls fflush which calls _flushall */
int CDECL fflush(FILE* file);

/*********************************************************************
 *		_flushall (MSVCRT.@)
 */
int CDECL _flushall(void)
{
  int i, num_flushed = 0;
  FILE *file;

  LOCK_FILES();
  for (i = 3; i < stream_idx; i++) {
    file = get_file(i);

    if (file->_flag)
    {
      if(file->_flag & _IOWRT) {
	fflush(file);
        num_flushed++;
      }
    }
  }
  UNLOCK_FILES();

  TRACE(":flushed (%d) handles\n",num_flushed);
  return num_flushed;
}

/*********************************************************************
 *		fflush (MSVCRT.@)
 */
int CDECL fflush(FILE* file)
{
    if(!file) {
        _flushall();
    } else if(file->_flag & _IOWRT) {
        int res;

        _lock_file(file);
        res = flush_buffer(file);
        _unlock_file(file);

        return res;
    }
    return 0;
}

/*********************************************************************
 *		_close (MSVCRT.@)
 */
int CDECL _close(int fd)
{
  HANDLE hand;
  int ret;

  LOCK_FILES();
  hand = fdtoh(fd);
  TRACE(":fd (%d) handle (%p)\n",fd,hand);
  if (hand == INVALID_HANDLE_VALUE)
    ret = -1;
  else if (!CloseHandle(hand))
  {
    WARN(":failed-last error (%d)\n",GetLastError());
    _dosmaperr(GetLastError());
    ret = -1;
  }
  else
  {
    free_fd(fd);
    ret = 0;
  }
  UNLOCK_FILES();
  TRACE(":ok\n");
  return ret;
}

/*********************************************************************
 *		_commit (MSVCRT.@)
 */
int CDECL _commit(int fd)
{
  HANDLE hand = fdtoh(fd);

  TRACE(":fd (%d) handle (%p)\n",fd,hand);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  if (!FlushFileBuffers(hand))
  {
    if (GetLastError() == ERROR_INVALID_HANDLE)
    {
      /* FlushFileBuffers fails for console handles
       * so we ignore this error.
       */
      return 0;
    }
    TRACE(":failed-last error (%d)\n",GetLastError());
    _dosmaperr(GetLastError());
    return -1;
  }
  TRACE(":ok\n");
  return 0;
}

/*********************************************************************
 *		_dup2 (MSVCRT.@)
 * NOTES
 * MSDN isn't clear on this point, but the remarks for _pipe
 * indicate file descriptors duplicated with _dup and _dup2 are always
 * inheritable.
 */
int CDECL _dup2(int od, int nd)
{
  int ret;

  TRACE("(od=%d, nd=%d)\n", od, nd);
  LOCK_FILES();
  if (nd < MAX_FILES && nd >= 0 && is_valid_fd(od))
  {
    HANDLE handle;

    if (DuplicateHandle(GetCurrentProcess(), get_ioinfo(od)->handle,
     GetCurrentProcess(), &handle, 0, TRUE, DUPLICATE_SAME_ACCESS))
    {
      int wxflag = get_ioinfo(od)->wxflag & ~_O_NOINHERIT;

      if (is_valid_fd(nd))
        _close(nd);
      ret = alloc_fd_from(handle, wxflag, nd);
      if (ret == -1)
      {
        CloseHandle(handle);
        *_errno() = EMFILE;
      }
      else
      {
        /* _dup2 returns 0, not nd, on success */
        ret = 0;
      }
    }
    else
    {
      ret = -1;
      _dosmaperr(GetLastError());
    }
  }
  else
  {
    *_errno() = EBADF;
    ret = -1;
  }
  UNLOCK_FILES();
  return ret;
}

/*********************************************************************
 *		_dup (MSVCRT.@)
 */
int CDECL _dup(int od)
{
  int fd, ret;

  LOCK_FILES();
  fd = fdstart;
  if (_dup2(od, fd) == 0)
    ret = fd;
  else
    ret = -1;
  UNLOCK_FILES();
  return ret;
}

/*********************************************************************
 *		_eof (MSVCRT.@)
 */
int CDECL _eof(int fd)
{
  DWORD curpos,endpos;
  LONG hcurpos,hendpos;
  HANDLE hand = fdtoh(fd);

  TRACE(":fd (%d) handle (%p)\n",fd,hand);

  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  if (get_ioinfo(fd)->wxflag & WX_ATEOF) return TRUE;

  /* Otherwise we do it the hard way */
  hcurpos = hendpos = 0;
  curpos = SetFilePointer(hand, 0, &hcurpos, FILE_CURRENT);
  endpos = SetFilePointer(hand, 0, &hendpos, FILE_END);

  if (curpos == endpos && hcurpos == hendpos)
  {
    /* FIXME: shouldn't WX_ATEOF be set here? */
    return TRUE;
  }

  SetFilePointer(hand, curpos, &hcurpos, FILE_BEGIN);
  return FALSE;
}

/*********************************************************************
 *		_fcloseall (MSVCRT.@)
 */
int CDECL _fcloseall(void)
{
  int num_closed = 0, i;
  FILE *file;

  LOCK_FILES();
  for (i = 3; i < stream_idx; i++) {
    file = get_file(i);

    if (file->_flag && !fclose(file))
      num_closed++;
  }
  UNLOCK_FILES();

  TRACE(":closed (%d) handles\n",num_closed);
  return num_closed;
}

/* free everything on process exit */
void msvcrt_free_io(void)
{
    int i;

    _fcloseall();
    /* The Win32 _fcloseall() function explicitly doesn't close stdin,
     * stdout, and stderr (unlike GNU), so we need to fclose() them here
     * or they won't get flushed.
     */
    fclose(&_iob[0]);
    fclose(&_iob[1]);
    fclose(&_iob[2]);

    for(i=0; i<sizeof(__pioinfo)/sizeof(__pioinfo[0]); i++)
        free(__pioinfo[i]);

    for(i=0; i<sizeof(fstream)/sizeof(fstream[0]); i++)
        free(fstream[i]);

    file_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&file_cs);
}

/*********************************************************************
 *		_lseeki64 (MSVCRT.@)
 */
__int64 CDECL _lseeki64(int fd, __int64 offset, int whence)
{
  HANDLE hand = fdtoh(fd);
  LARGE_INTEGER ofs;

  TRACE(":fd (%d) handle (%p)\n",fd,hand);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  if (whence < 0 || whence > 2)
  {
    *_errno() = EINVAL;
    return -1;
  }

  TRACE(":fd (%d) to %s pos %s\n",
        fd,wine_dbgstr_longlong(offset),
        (whence==SEEK_SET)?"SEEK_SET":
        (whence==SEEK_CUR)?"SEEK_CUR":
        (whence==SEEK_END)?"SEEK_END":"UNKNOWN");

  /* The MoleBox protection scheme expects msvcrt to use SetFilePointer only,
   * so a LARGE_INTEGER offset cannot be passed directly via SetFilePointerEx. */
  ofs.QuadPart = offset;
  if ((ofs.u.LowPart = SetFilePointer(hand, ofs.u.LowPart, &ofs.u.HighPart, whence)) != INVALID_SET_FILE_POINTER ||
      GetLastError() == ERROR_SUCCESS)
  {
    get_ioinfo(fd)->wxflag &= ~(WX_ATEOF|WX_READEOF);
    /* FIXME: What if we seek _to_ EOF - is EOF set? */

    return ofs.QuadPart;
  }
  TRACE(":error-last error (%d)\n",GetLastError());
  _dosmaperr(GetLastError());
  return -1;
}

/*********************************************************************
 *		_lseek (MSVCRT.@)
 */
LONG CDECL _lseek(int fd, LONG offset, int whence)
{
    return (LONG)_lseeki64(fd, offset, whence);
}

/*********************************************************************
 *              _lock_file (MSVCRT.@)
 */
void CDECL _lock_file(FILE *file)
{
    if(file>=_iob && file<_iob+_IOB_ENTRIES)
        _lock(_STREAM_LOCKS+(file-_iob));
    /* ReactOS: string streams dont need to be locked */
    else if(!(file->_flag & _IOSTRG))
        EnterCriticalSection(&((file_crit*)file)->crit);
}

/*********************************************************************
 *              _unlock_file (MSVCRT.@)
 */
void CDECL _unlock_file(FILE *file)
{
    if(file>=_iob && file<_iob+_IOB_ENTRIES)
        _unlock(_STREAM_LOCKS+(file-_iob));
    /* ReactOS: string streams dont need to be locked */
    else if(!(file->_flag & _IOSTRG))
        LeaveCriticalSection(&((file_crit*)file)->crit);

}

/*********************************************************************
 *		_locking (MSVCRT.@)
 *
 * This is untested; the underlying LockFile doesn't work yet.
 */
int CDECL _locking(int fd, int mode, LONG nbytes)
{
  BOOL ret;
  DWORD cur_locn;
  HANDLE hand = fdtoh(fd);

  TRACE(":fd (%d) handle (%p)\n",fd,hand);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  if (mode < 0 || mode > 4)
  {
    *_errno() = EINVAL;
    return -1;
  }

  TRACE(":fd (%d) by 0x%08x mode %s\n",
        fd,nbytes,(mode==_LK_UNLCK)?"_LK_UNLCK":
        (mode==_LK_LOCK)?"_LK_LOCK":
        (mode==_LK_NBLCK)?"_LK_NBLCK":
        (mode==_LK_RLCK)?"_LK_RLCK":
        (mode==_LK_NBRLCK)?"_LK_NBRLCK":
                          "UNKNOWN");

  if ((cur_locn = SetFilePointer(hand, 0L, NULL, SEEK_CUR)) == INVALID_SET_FILE_POINTER)
  {
    FIXME ("Seek failed\n");
    *_errno() = EINVAL; /* FIXME */
    return -1;
  }
  if (mode == _LK_LOCK || mode == _LK_RLCK)
  {
    int nretry = 10;
    ret = 1; /* just to satisfy gcc */
    while (nretry--)
    {
      ret = LockFile(hand, cur_locn, 0L, nbytes, 0L);
      if (ret) break;
      Sleep(1);
    }
  }
  else if (mode == _LK_UNLCK)
    ret = UnlockFile(hand, cur_locn, 0L, nbytes, 0L);
  else
    ret = LockFile(hand, cur_locn, 0L, nbytes, 0L);
  /* FIXME - what about error settings? */
  return ret ? 0 : -1;
}

/*********************************************************************
 *		_fseeki64 (MSVCRT.@)
 */
int CDECL _fseeki64(FILE* file, __int64 offset, int whence)
{
  int ret;

  _lock_file(file);
  /* Flush output if needed */
  if(file->_flag & _IOWRT)
	flush_buffer(file);

  if(whence == SEEK_CUR && file->_flag & _IOREAD ) {
	offset -= file->_cnt;
	if (get_ioinfo(file->_file)->wxflag & WX_TEXT) {
		/* Black magic correction for CR removal */
		int i;
		for (i=0; i<file->_cnt; i++) {
			if (file->_ptr[i] == '\n')
				offset--;
		}
		/* Black magic when reading CR at buffer boundary*/
		if(get_ioinfo(file->_file)->wxflag & WX_READCR)
		    offset--;
	}
  }
  /* Discard buffered input */
  file->_cnt = 0;
  file->_ptr = file->_base;
  /* Reset direction of i/o */
  if(file->_flag & _IORW) {
        file->_flag &= ~(_IOREAD|_IOWRT);
  }
  /* Clear end of file flag */
  file->_flag &= ~_IOEOF;
  ret = (_lseeki64(file->_file,offset,whence) == -1)?-1:0;

  _unlock_file(file);
  return ret;
}

/*********************************************************************
 *		fseek (MSVCRT.@)
 */
int CDECL fseek(FILE* file, long offset, int whence)
{
    return _fseeki64( file, offset, whence );
}

/*********************************************************************
 *		_chsize (MSVCRT.@)
 */
int CDECL _chsize(int fd, long size)
{
    LONG cur, pos;
    HANDLE handle;
    BOOL ret = FALSE;

    TRACE("(fd=%d, size=%d)\n", fd, size);

    LOCK_FILES();

    handle = fdtoh(fd);
    if (handle != INVALID_HANDLE_VALUE)
    {
        /* save the current file pointer */
        cur = _lseek(fd, 0, SEEK_CUR);
        if (cur >= 0)
        {
            pos = _lseek(fd, size, SEEK_SET);
            if (pos >= 0)
            {
                ret = SetEndOfFile(handle);
                if (!ret) _dosmaperr(GetLastError());
            }

            /* restore the file pointer */
            _lseek(fd, cur, SEEK_SET);
        }
    }

    UNLOCK_FILES();
    return ret ? 0 : -1;
}

/*********************************************************************
 *		clearerr (MSVCRT.@)
 */
void CDECL clearerr(FILE* file)
{
  TRACE(":file (%p) fd (%d)\n",file,file->_file);

  _lock_file(file);
  file->_flag &= ~(_IOERR | _IOEOF);
  _unlock_file(file);
}

/*********************************************************************
 *		rewind (MSVCRT.@)
 */
void CDECL rewind(FILE* file)
{
  TRACE(":file (%p) fd (%d)\n",file,file->_file);

  _lock_file(file);
  fseek(file, 0L, SEEK_SET);
  clearerr(file);
  _unlock_file(file);
}

static int get_flags(const wchar_t* mode, int *open_flags, int* stream_flags)
{
  int plus = strchrW(mode, '+') != NULL;

  switch(*mode++)
  {
  case 'R': case 'r':
    *open_flags = plus ? _O_RDWR : _O_RDONLY;
    *stream_flags = plus ? _IORW : _IOREAD;
    break;
  case 'W': case 'w':
    *open_flags = _O_CREAT | _O_TRUNC | (plus  ? _O_RDWR : _O_WRONLY);
    *stream_flags = plus ? _IORW : _IOWRT;
    break;
  case 'A': case 'a':
    *open_flags = _O_CREAT | _O_APPEND | (plus  ? _O_RDWR : _O_WRONLY);
    *stream_flags = plus ? _IORW : _IOWRT;
    break;
  default:
    _invalid_parameter(NULL, NULL, NULL, 0, 0);
    *_errno() = EINVAL;
    return -1;
  }

  while (*mode)
    switch (*mode++)
    {
    case 'B': case 'b':
      *open_flags |=  _O_BINARY;
      *open_flags &= ~_O_TEXT;
      break;
    case 'T': case 't':
      *open_flags |=  _O_TEXT;
      *open_flags &= ~_O_BINARY;
      break;
    case '+':
    case ' ':
      break;
    default:
      FIXME(":unknown flag %c not supported\n",mode[-1]);
    }
  return 0;
}

/*********************************************************************
 *		_fdopen (MSVCRT.@)
 */
FILE* CDECL _fdopen(int fd, const char *mode)
{
    FILE *ret;
    wchar_t *modeW = NULL;

    if (mode && !(modeW = msvcrt_wstrdupa(mode))) return NULL;

    ret = _wfdopen(fd, modeW);

    free(modeW);
    return ret;
}

/*********************************************************************
 *		_wfdopen (MSVCRT.@)
 */
FILE* CDECL _wfdopen(int fd, const wchar_t *mode)
{
  int open_flags, stream_flags;
  FILE* file;

  if (get_flags(mode, &open_flags, &stream_flags) == -1) return NULL;

  LOCK_FILES();
  if (!(file = alloc_fp()))
    file = NULL;
  else if (init_fp(file, fd, stream_flags) == -1)
  {
    file->_flag = 0;
    file = NULL;
  }
  else TRACE(":fd (%d) mode (%s) FILE* (%p)\n", fd, debugstr_w(mode), file);
  UNLOCK_FILES();

  return file;
}

/*********************************************************************
 *		_filelength (MSVCRT.@)
 */
LONG CDECL _filelength(int fd)
{
  LONG curPos = _lseek(fd, 0, SEEK_CUR);
  if (curPos != -1)
  {
    LONG endPos = _lseek(fd, 0, SEEK_END);
    if (endPos != -1)
    {
      if (endPos != curPos)
        _lseek(fd, curPos, SEEK_SET);
      return endPos;
    }
  }
  return -1;
}

/*********************************************************************
 *		_filelengthi64 (MSVCRT.@)
 */
__int64 CDECL _filelengthi64(int fd)
{
  __int64 curPos = _lseeki64(fd, 0, SEEK_CUR);
  if (curPos != -1)
  {
    __int64 endPos = _lseeki64(fd, 0, SEEK_END);
    if (endPos != -1)
    {
      if (endPos != curPos)
        _lseeki64(fd, curPos, SEEK_SET);
      return endPos;
    }
  }
  return -1;
}

/*********************************************************************
 *		_fileno (MSVCRT.@)
 */
int CDECL _fileno(FILE* file)
{
  TRACE(":FILE* (%p) fd (%d)\n",file,file->_file);
  return file->_file;
}

/*********************************************************************
 *		_get_osfhandle (MSVCRT.@)
 */
intptr_t CDECL _get_osfhandle(int fd)
{
  HANDLE hand = fdtoh(fd);
  TRACE(":fd (%d) handle (%p)\n",fd,hand);

  return (intptr_t)hand;
}

/*********************************************************************
 *		_isatty (MSVCRT.@)
 */
int CDECL _isatty(int fd)
{
  HANDLE hand = fdtoh(fd);

  TRACE(":fd (%d) handle (%p)\n",fd,hand);
  if (hand == INVALID_HANDLE_VALUE)
    return 0;

  return GetFileType(hand) == FILE_TYPE_CHAR? 1 : 0;
}

/*********************************************************************
 *		_mktemp (MSVCRT.@)
 */
char * CDECL _mktemp(char *pattern)
{
  int numX = 0;
  char *retVal = pattern;
  int id;
  char letter = 'a';

  while(*pattern)
    numX = (*pattern++ == 'X')? numX + 1 : 0;
  if (numX < 5)
    return NULL;
  pattern--;
  id = GetCurrentProcessId();
  numX = 6;
  while(numX--)
  {
    int tempNum = id / 10;
    *pattern-- = id - (tempNum * 10) + '0';
    id = tempNum;
  }
  pattern++;
  do
  {
    *pattern = letter++;
    if (GetFileAttributesA(retVal) == INVALID_FILE_ATTRIBUTES &&
        GetLastError() == ERROR_FILE_NOT_FOUND)
      return retVal;
  } while(letter <= 'z');
  return NULL;
}

/*********************************************************************
 *		_wmktemp (MSVCRT.@)
 */
wchar_t * CDECL _wmktemp(wchar_t *pattern)
{
  int numX = 0;
  wchar_t *retVal = pattern;
  int id;
  wchar_t letter = 'a';

  while(*pattern)
    numX = (*pattern++ == 'X')? numX + 1 : 0;
  if (numX < 5)
    return NULL;
  pattern--;
  id = GetCurrentProcessId();
  numX = 6;
  while(numX--)
  {
    int tempNum = id / 10;
    *pattern-- = id - (tempNum * 10) + '0';
    id = tempNum;
  }
  pattern++;
  do
  {
    if (GetFileAttributesW(retVal) == INVALID_FILE_ATTRIBUTES &&
        GetLastError() == ERROR_FILE_NOT_FOUND)
      return retVal;
    *pattern = letter++;
  } while(letter != '|');
  return NULL;
}

/*static*/ unsigned split_oflags(unsigned oflags)
{
    int         wxflags = 0;
    unsigned unsupp; /* until we support everything */

    if (oflags & _O_APPEND)              wxflags |= WX_APPEND;
    if (oflags & _O_BINARY)              {/* Nothing to do */}
    else if (oflags & _O_TEXT)           wxflags |= WX_TEXT;
    else if (*__p__fmode() & _O_BINARY)  {/* Nothing to do */}
    else                                        wxflags |= WX_TEXT; /* default to TEXT*/
    if (oflags & _O_NOINHERIT)           wxflags |= WX_DONTINHERIT;

    if ((unsupp = oflags & ~(
                    _O_BINARY|_O_TEXT|_O_APPEND|
                    _O_TRUNC|_O_EXCL|_O_CREAT|
                    _O_RDWR|_O_WRONLY|_O_TEMPORARY|
                    _O_NOINHERIT|
                    _O_SEQUENTIAL|_O_RANDOM|_O_SHORT_LIVED
                    )))
        ERR(":unsupported oflags 0x%04x\n",unsupp);

    return wxflags;
}

/*********************************************************************
 *              _pipe (MSVCRT.@)
 */
int CDECL _pipe(int *pfds, unsigned int psize, int textmode)
{
  int ret = -1;
  SECURITY_ATTRIBUTES sa;
  HANDLE readHandle, writeHandle;

  if (!pfds)
  {
    *_errno() = EINVAL;
    return -1;
  }

  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = !(textmode & _O_NOINHERIT);
  sa.lpSecurityDescriptor = NULL;
  if (CreatePipe(&readHandle, &writeHandle, &sa, psize))
  {
    unsigned int wxflags = split_oflags(textmode);
    int fd;

    LOCK_FILES();
    fd = alloc_fd(readHandle, wxflags);
    if (fd != -1)
    {
      pfds[0] = fd;
      fd = alloc_fd(writeHandle, wxflags);
      if (fd != -1)
      {
        pfds[1] = fd;
        ret = 0;
      }
      else
      {
        _close(pfds[0]);
        CloseHandle(writeHandle);
        *_errno() = EMFILE;
      }
    }
    else
    {
      CloseHandle(readHandle);
      CloseHandle(writeHandle);
      *_errno() = EMFILE;
    }
    UNLOCK_FILES();
  }
  else
    _dosmaperr(GetLastError());

  return ret;
}

/*********************************************************************
 *              _sopen_s (MSVCRT.@)
 */
int CDECL _sopen_s( int *fd, const char *path, int oflags, int shflags, int pmode )
{
  DWORD access = 0, creation = 0, attrib;
  DWORD sharing;
  int wxflag;
  HANDLE hand;
  SECURITY_ATTRIBUTES sa;

  TRACE("fd*: %p file: (%s) oflags: 0x%04x shflags: 0x%04x pmode: 0x%04x\n",
        fd, path, oflags, shflags, pmode);

  if (!fd)
  {
    MSVCRT_INVALID_PMT("null out fd pointer");
    *_errno() = EINVAL;
    return EINVAL;
  }

  *fd = -1;
  wxflag = split_oflags(oflags);
  switch (oflags & (_O_RDONLY | _O_WRONLY | _O_RDWR))
  {
  case _O_RDONLY: access |= GENERIC_READ; break;
  case _O_WRONLY: access |= GENERIC_WRITE; break;
  case _O_RDWR:   access |= GENERIC_WRITE | GENERIC_READ; break;
  }

  if (oflags & _O_CREAT)
  {
    if(pmode & ~(_S_IREAD | _S_IWRITE))
      FIXME(": pmode 0x%04x ignored\n", pmode);
    else
      WARN(": pmode 0x%04x ignored\n", pmode);

    if (oflags & _O_EXCL)
      creation = CREATE_NEW;
    else if (oflags & _O_TRUNC)
      creation = CREATE_ALWAYS;
    else
      creation = OPEN_ALWAYS;
  }
  else  /* no _O_CREAT */
  {
    if (oflags & _O_TRUNC)
      creation = TRUNCATE_EXISTING;
    else
      creation = OPEN_EXISTING;
  }

  switch( shflags )
  {
    case _SH_DENYRW:
      sharing = 0L;
      break;
    case _SH_DENYWR:
      sharing = FILE_SHARE_READ;
      break;
    case _SH_DENYRD:
      sharing = FILE_SHARE_WRITE;
      break;
    case _SH_DENYNO:
      sharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
      break;
    default:
      ERR( "Unhandled shflags 0x%x\n", shflags );
      return EINVAL;
  }
  attrib = FILE_ATTRIBUTE_NORMAL;

  if (oflags & _O_TEMPORARY)
  {
      attrib |= FILE_FLAG_DELETE_ON_CLOSE;
      access |= DELETE;
      sharing |= FILE_SHARE_DELETE;
  }

  sa.nLength              = sizeof( SECURITY_ATTRIBUTES );
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle       = (oflags & _O_NOINHERIT) ? FALSE : TRUE;

  hand = CreateFileA(path, access, sharing, &sa, creation, attrib, 0);
  if (hand == INVALID_HANDLE_VALUE)  {
    WARN(":failed-last error (%d)\n", GetLastError());
    _dosmaperr(GetLastError());
    return *_errno();
  }

  *fd = alloc_fd(hand, wxflag);

  TRACE(":fd (%d) handle (%p)\n", *fd, hand);
  return 0;
}

/*********************************************************************
 *              _sopen (MSVCRT.@)
 */
int CDECL _sopen( const char *path, int oflags, int shflags, ... )
{
  int pmode;
  int fd;

  if (oflags & _O_CREAT)
  {
    va_list ap;

    va_start(ap, shflags);
    pmode = va_arg(ap, int);
    va_end(ap);
  }
  else
    pmode = 0;

  _sopen_s(&fd, path, oflags, shflags, pmode);
  return fd;
}

/*********************************************************************
 *              _wsopen_s (MSVCRT.@)
 */
int CDECL _wsopen_s( int *fd, const wchar_t* path, int oflags, int shflags, int pmode )
{
  DWORD access = 0, creation = 0, attrib;
  SECURITY_ATTRIBUTES sa;
  DWORD sharing;
  int wxflag;
  HANDLE hand;

  TRACE("fd*: %p :file (%s) oflags: 0x%04x shflags: 0x%04x pmode: 0x%04x\n",
        fd, debugstr_w(path), oflags, shflags, pmode);

  if (!fd)
  {
    MSVCRT_INVALID_PMT("null out fd pointer");
    *_errno() = EINVAL;
    return EINVAL;
  }

  *fd = -1;
  wxflag = split_oflags(oflags);
  switch (oflags & (_O_RDONLY | _O_WRONLY | _O_RDWR))
  {
  case _O_RDONLY: access |= GENERIC_READ; break;
  case _O_WRONLY: access |= GENERIC_WRITE; break;
  case _O_RDWR:   access |= GENERIC_WRITE | GENERIC_READ; break;
  }

  if (oflags & _O_CREAT)
  {
    if(pmode & ~(_S_IREAD | _S_IWRITE))
      FIXME(": pmode 0x%04x ignored\n", pmode);
    else
      WARN(": pmode 0x%04x ignored\n", pmode);

    if (oflags & _O_EXCL)
      creation = CREATE_NEW;
    else if (oflags & _O_TRUNC)
      creation = CREATE_ALWAYS;
    else
      creation = OPEN_ALWAYS;
  }
  else  /* no _O_CREAT */
  {
    if (oflags & _O_TRUNC)
      creation = TRUNCATE_EXISTING;
    else
      creation = OPEN_EXISTING;
  }

  switch( shflags )
  {
    case _SH_DENYRW:
      sharing = 0L;
      break;
    case _SH_DENYWR:
      sharing = FILE_SHARE_READ;
      break;
    case _SH_DENYRD:
      sharing = FILE_SHARE_WRITE;
      break;
    case _SH_DENYNO:
      sharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
      break;
    default:
      ERR( "Unhandled shflags 0x%x\n", shflags );
      return EINVAL;
  }
  attrib = FILE_ATTRIBUTE_NORMAL;

  if (oflags & _O_TEMPORARY)
  {
      attrib |= FILE_FLAG_DELETE_ON_CLOSE;
      access |= DELETE;
      sharing |= FILE_SHARE_DELETE;
  }

  sa.nLength              = sizeof( SECURITY_ATTRIBUTES );
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle       = (oflags & _O_NOINHERIT) ? FALSE : TRUE;

  hand = CreateFileW(path, access, sharing, &sa, creation, attrib, 0);

  if (hand == INVALID_HANDLE_VALUE)  {
    WARN(":failed-last error (%d)\n",GetLastError());
    _dosmaperr(GetLastError());
    return *_errno();
  }

  *fd = alloc_fd(hand, wxflag);

  TRACE(":fd (%d) handle (%p)\n", *fd, hand);
  return 0;
}

/*********************************************************************
 *              _wsopen (MSVCRT.@)
 */
int CDECL _wsopen( const wchar_t *path, int oflags, int shflags, ... )
{
  int pmode;
  int fd;

  if (oflags & _O_CREAT)
  {
    va_list ap;

    va_start(ap, shflags);
    pmode = va_arg(ap, int);
    va_end(ap);
  }
  else
    pmode = 0;

  _wsopen_s(&fd, path, oflags, shflags, pmode);
  return fd;
}

/*********************************************************************
 *              _open (MSVCRT.@)
 */
int CDECL _open( const char *path, int flags, ... )
{
  va_list ap;

  if (flags & _O_CREAT)
  {
    int pmode;
    va_start(ap, flags);
    pmode = va_arg(ap, int);
    va_end(ap);
    return _sopen( path, flags, _SH_DENYNO, pmode );
  }
  else
    return _sopen( path, flags, _SH_DENYNO);
}

/*********************************************************************
 *              _wopen (MSVCRT.@)
 */
int CDECL _wopen(const wchar_t *path,int flags,...)
{
  va_list ap;

  if (flags & _O_CREAT)
  {
    int pmode;
    va_start(ap, flags);
    pmode = va_arg(ap, int);
    va_end(ap);
    return _wsopen( path, flags, _SH_DENYNO, pmode );
  }
  else
    return _wsopen( path, flags, _SH_DENYNO);
}

/*********************************************************************
 *		_creat (MSVCRT.@)
 */
int CDECL _creat(const char *path, int flags)
{
  int usedFlags = (flags & _O_TEXT)| _O_CREAT| _O_WRONLY| _O_TRUNC;
  return _open(path, usedFlags);
}

/*********************************************************************
 *		_wcreat (MSVCRT.@)
 */
int CDECL _wcreat(const wchar_t *path, int flags)
{
  int usedFlags = (flags & _O_TEXT)| _O_CREAT| _O_WRONLY| _O_TRUNC;
  return _wopen(path, usedFlags);
}

/*********************************************************************
 *		_open_osfhandle (MSVCRT.@)
 */
int CDECL _open_osfhandle(intptr_t handle, int oflags)
{
  int fd;

  /* _O_RDONLY (0) always matches, so set the read flag
   * MFC's CStdioFile clears O_RDONLY (0)! if it wants to write to the
   * file, so set the write flag. It also only sets _O_TEXT if it wants
   * text - it never sets _O_BINARY.
   */
  /* don't let split_oflags() decide the mode if no mode is passed */
  if (!(oflags & (_O_BINARY | _O_TEXT)))
      oflags |= _O_BINARY;

  fd = alloc_fd((HANDLE)handle, split_oflags(oflags));
  TRACE(":handle (%ld) fd (%d) flags 0x%08x\n", handle, fd, oflags);
  return fd;
}

/*********************************************************************
 *		_rmtmp (MSVCRT.@)
 */
int CDECL _rmtmp(void)
{
  int num_removed = 0, i;
  FILE *file;

  LOCK_FILES();
  for (i = 3; i < stream_idx; i++) {
    file = get_file(i);

    if (file->_tmpfname)
    {
      fclose(file);
      num_removed++;
    }
  }
  UNLOCK_FILES();

  if (num_removed)
    TRACE(":removed (%d) temp files\n",num_removed);
  return num_removed;
}

/*********************************************************************
 * (internal) read_i
 *
 * When reading \r as last character in text mode, read() positions
 * the file pointer on the \r character while getc() goes on to
 * the following \n
 */
static int read_i(int fd, void *buf, unsigned int count)
{
  DWORD num_read;
  char *bufstart = buf;
  HANDLE hand = fdtoh(fd);
  ioinfo *fdinfo = get_ioinfo(fd);

  if (count == 0)
    return 0;

  if (fdinfo->wxflag & WX_READEOF) {
     fdinfo->wxflag |= WX_ATEOF;
     TRACE("already at EOF, returning 0\n");
     return 0;
  }
  /* Don't trace small reads, it gets *very* annoying */
  if (count > 4)
    TRACE(":fd (%d) handle (%p) buf (%p) len (%d)\n",fd,hand,buf,count);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  /* Reading single bytes in O_TEXT mode makes things slow
   * So read big chunks
   */
    if (ReadFile(hand, bufstart, count, &num_read, NULL))
    {
        if (count != 0 && num_read == 0)
        {
            fdinfo->wxflag |= (WX_ATEOF|WX_READEOF);
            TRACE(":EOF %s\n",debugstr_an(buf,num_read));
        }
        else if (fdinfo->wxflag & WX_TEXT)
        {
            DWORD i, j;
            if (bufstart[num_read-1] == '\r')
            {
                if(count == 1)
                {
                    fdinfo->wxflag  &=  ~WX_READCR;
                    ReadFile(hand, bufstart, 1, &num_read, NULL);
                }
                else
                {
                    fdinfo->wxflag  |= WX_READCR;
                    num_read--;
                }
            }
	    else
	      fdinfo->wxflag  &=  ~WX_READCR;
            for (i=0, j=0; i<num_read; i++)
            {
                /* in text mode, a ctrl-z signals EOF */
                if (bufstart[i] == 0x1a)
                {
                    fdinfo->wxflag |= (WX_ATEOF|WX_READEOF);
                    TRACE(":^Z EOF %s\n",debugstr_an(buf,num_read));
                    break;
                }
                /* in text mode, strip \r if followed by \n.
                 * BUG: should save state across calls somehow, so CR LF that
                 * straddles buffer boundary gets recognized properly?
                 */
		if ((bufstart[i] != '\r')
                ||  ((i+1) < num_read && bufstart[i+1] != '\n'))
		    bufstart[j++] = bufstart[i];
            }
            num_read = j;
        }
    }
    else
    {
        if (GetLastError() == ERROR_BROKEN_PIPE)
        {
            TRACE(":end-of-pipe\n");
            fdinfo->wxflag |= (WX_ATEOF|WX_READEOF);
            return 0;
        }
        else
        {
            TRACE(":failed-last error (%d)\n",GetLastError());
            return -1;
        }
    }

  if (count > 4)
      TRACE("(%u), %s\n",num_read,debugstr_an(buf, num_read));
  return num_read;
}

/*********************************************************************
 *		_read (MSVCRT.@)
 */
int CDECL _read(int fd, void *buf, unsigned int count)
{
  int num_read;
  num_read = read_i(fd, buf, count);
  return num_read;
}

/*********************************************************************
 *		_setmode (MSVCRT.@)
 */
int CDECL _setmode(int fd,int mode)
{
  int ret = get_ioinfo(fd)->wxflag & WX_TEXT ? _O_TEXT : _O_BINARY;
  if (mode & (~(_O_TEXT|_O_BINARY)))
    FIXME("fd (%d) mode (0x%08x) unknown\n",fd,mode);
  if ((mode & _O_TEXT) == _O_TEXT)
    get_ioinfo(fd)->wxflag |= WX_TEXT;
  else
    get_ioinfo(fd)->wxflag &= ~WX_TEXT;
  return ret;
}

/*********************************************************************
 *		_tell (MSVCRT.@)
 */
long CDECL _tell(int fd)
{
  return _lseek(fd, 0, SEEK_CUR);
}

/*********************************************************************
 *		_telli64 (MSVCRT.@)
 */
__int64 CDECL _telli64(int fd)
{
  return _lseeki64(fd, 0, SEEK_CUR);
}

/*********************************************************************
 *		_tempnam (MSVCRT.@)
 */
char * CDECL _tempnam(const char *dir, const char *prefix)
{
  char tmpbuf[MAX_PATH];
  const char *tmp_dir = getenv("TMP");

  if (tmp_dir) dir = tmp_dir;

  TRACE("dir (%s) prefix (%s)\n",dir,prefix);
  if (GetTempFileNameA(dir,prefix,0,tmpbuf))
  {
    TRACE("got name (%s)\n",tmpbuf);
    DeleteFileA(tmpbuf);
    return _strdup(tmpbuf);
  }
  TRACE("failed (%d)\n",GetLastError());
  return NULL;
}

/*********************************************************************
 *		_wtempnam (MSVCRT.@)
 */
wchar_t * CDECL _wtempnam(const wchar_t *dir, const wchar_t *prefix)
{
  wchar_t tmpbuf[MAX_PATH];

  TRACE("dir (%s) prefix (%s)\n",debugstr_w(dir),debugstr_w(prefix));
  if (GetTempFileNameW(dir,prefix,0,tmpbuf))
  {
    TRACE("got name (%s)\n",debugstr_w(tmpbuf));
    DeleteFileW(tmpbuf);
    return _wcsdup(tmpbuf);
  }
  TRACE("failed (%d)\n",GetLastError());
  return NULL;
}

/*********************************************************************
 *		_umask (MSVCRT.@)
 */
int CDECL _umask(int umask)
{
  int old_umask = umask;
  TRACE("(%d)\n",umask);
  MSVCRT_umask = umask;
  return old_umask;
}

/*********************************************************************
 *		_write (MSVCRT.@)
 */
int CDECL _write(int fd, const void* buf, unsigned int count)
{
  DWORD num_written;
  HANDLE hand = fdtoh(fd);

  /* Don't trace small writes, it gets *very* annoying */
#if 0
  if (count > 32)
    TRACE(":fd (%d) handle (%d) buf (%p) len (%d)\n",fd,hand,buf,count);
#endif
  if (hand == INVALID_HANDLE_VALUE)
    {
      *_errno() = EBADF;
      return -1;
    }

  /* If appending, go to EOF */
  if (get_ioinfo(fd)->wxflag & WX_APPEND)
    _lseek(fd, 0, FILE_END);

  if (!(get_ioinfo(fd)->wxflag & WX_TEXT))
    {
      if (WriteFile(hand, buf, count, &num_written, NULL)
	  &&  (num_written == count))
	return num_written;
      TRACE("WriteFile (fd %d, hand %p) failed-last error (%d)\n", fd,
       hand, GetLastError());
      *_errno() = ENOSPC;
    }
  else
  {
      unsigned int i, j, nr_lf;
      char *p = NULL;
      const char *q;
      const char *s = buf, *buf_start = buf;
      /* find number of \n ( without preceding \r ) */
      for ( nr_lf=0,i = 0; i <count; i++)
      {
          if (s[i]== '\n')
          {
              nr_lf++;
              /*if ((i >1) && (s[i-1] == '\r'))	nr_lf--; */
          }
      }
      if (nr_lf)
      {
          if ((q = p = malloc(count + nr_lf)))
          {
              for (s = buf, i = 0, j = 0; i < count; i++)
              {
                  if (s[i]== '\n')
                  {
                      p[j++] = '\r';
                      /*if ((i >1) && (s[i-1] == '\r'))j--;*/
                  }
                  p[j++] = s[i];
              }
          }
          else
          {
              FIXME("Malloc failed\n");
              nr_lf =0;
              q = buf;
          }
      }
      else
          q = buf;

      if ((WriteFile(hand, q, count+nr_lf, &num_written, NULL) == 0 ) || (num_written != count+nr_lf))
      {
          TRACE("WriteFile (fd %d, hand %p) failed-last error (%d), num_written %d\n",
           fd, hand, GetLastError(), num_written);
          *_errno() = ENOSPC;
          if(nr_lf)
              free(p);
          return s - buf_start;
      }
      else
      {
          if(nr_lf)
              free(p);
          return count;
      }
  }
  return -1;
}

/*********************************************************************
 *		_putw (MSVCRT.@)
 */
int CDECL _putw(int val, FILE* file)
{
  int len;

  _lock_file(file);
  len = _write(file->_file, &val, sizeof(val));
  if (len == sizeof(val)) {
    _unlock_file(file);
    return val;
  }

  file->_flag |= _IOERR;
  _unlock_file(file);
  return EOF;
}

/*********************************************************************
 *		fclose (MSVCRT.@)
 */
int CDECL fclose(FILE* file)
{
  int r, flag;

  _lock_file(file);
  flag = file->_flag;
  free(file->_tmpfname);
  file->_tmpfname = NULL;
  /* flush stdio buffers */
  if(file->_flag & _IOWRT)
      fflush(file);
  if(file->_flag & _IOMYBUF)
      free(file->_base);

  r=_close(file->_file);

  file->_flag = 0;
  _unlock_file(file);
  if(file<_iob || file>=_iob+_IOB_ENTRIES)
      DeleteCriticalSection(&((file_crit*)file)->crit);

  if(file == get_file(stream_idx-1)) {
    while(stream_idx>3 && !file->_flag) {
      stream_idx--;
      file = get_file(stream_idx-1);
    }
  }

  return ((r == -1) || (flag & _IOERR) ? EOF : 0);
}

/*********************************************************************
 *		feof (MSVCRT.@)
 */
int CDECL feof(FILE* file)
{
    int ret;

    _lock_file(file);
    ret = file->_flag & _IOEOF;
    _unlock_file(file);

    return ret;
}

/*********************************************************************
 *		ferror (MSVCRT.@)
 */
int CDECL ferror(FILE* file)
{
    int ret;

    _lock_file(file);
    ret = file->_flag & _IOERR;
    _unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_filbuf (MSVCRT.@)
 */
int CDECL _filbuf(FILE* file)
{
    unsigned char c;
    _lock_file(file);

    /* Allocate buffer if needed */
    if(file->_bufsiz == 0 && !(file->_flag & _IONBF))
        alloc_buffer(file);

    if(!(file->_flag & _IOREAD)) {
        if(file->_flag & _IORW)
            file->_flag |= _IOREAD;
        else {
            _unlock_file(file);
            return EOF;
        }
    }

    if(file->_flag & _IONBF) {
        int r;
        if ((r = read_i(file->_file,&c,1)) != 1) {
            file->_flag |= (r == 0) ? _IOEOF : _IOERR;
            _unlock_file(file);
            return EOF;
        }

        _unlock_file(file);
        return c;
    } else {
        file->_cnt = read_i(file->_file, file->_base, file->_bufsiz);
        if(file->_cnt<=0) {
            file->_flag |= (file->_cnt == 0) ? _IOEOF : _IOERR;
            file->_cnt = 0;
            _unlock_file(file);
            return EOF;
        }

        file->_cnt--;
        file->_ptr = file->_base+1;
        c = *(unsigned char *)file->_base;
        _unlock_file(file);
        return c;
    }
}

/*********************************************************************
 *		fgetc (MSVCRT.@)
 */
int CDECL fgetc(FILE* file)
{
  unsigned char *i;
  unsigned int j;

  _lock_file(file);
  if (file->_cnt>0) {
    file->_cnt--;
    i = (unsigned char *)file->_ptr++;
    j = *i;
  } else
    j = _filbuf(file);

  _unlock_file(file);
  return j;
}

/*********************************************************************
 *		_fgetchar (MSVCRT.@)
 */
int CDECL _fgetchar(void)
{
  return fgetc(stdin);
}

/*********************************************************************
 *		fgets (MSVCRT.@)
 */
char * CDECL fgets(char *s, int size, FILE* file)
{
  int    cc = EOF;
  char * buf_start = s;

  TRACE(":file(%p) fd (%d) str (%p) len (%d)\n",
	file,file->_file,s,size);

  _lock_file(file);

  while ((size >1) && (cc = fgetc(file)) != EOF && cc != '\n')
    {
      *s++ = (char)cc;
      size --;
    }
  if ((cc == EOF) && (s == buf_start)) /* If nothing read, return 0*/
  {
    TRACE(":nothing read\n");
    _unlock_file(file);
    return NULL;
  }
  if ((cc != EOF) && (size > 1))
    *s++ = cc;
  *s = '\0';
  TRACE(":got %s\n", debugstr_a(buf_start));
  _unlock_file(file);
  return buf_start;
}

/*********************************************************************
 *		fgetwc (MSVCRT.@)
 *
 * In _O_TEXT mode, multibyte characters are read from the file, dropping
 * the CR from CR/LF combinations
 */
wint_t CDECL fgetwc(FILE* file)
{
  int c;

  _lock_file(file);
  if (!(get_ioinfo(file->_file)->wxflag & WX_TEXT))
    {
      wchar_t wc;
      unsigned int i;
      int j;
      char *chp, *wcp;
      wcp = (char *)&wc;
      for(i=0; i<sizeof(wc); i++)
      {
        if (file->_cnt>0)
        {
          file->_cnt--;
          chp = file->_ptr++;
          wcp[i] = *chp;
        }
        else
        {
          j = _filbuf(file);
          if(file->_cnt<=0)
          {
            file->_flag |= (file->_cnt == 0) ? _IOEOF : _IOERR;
            file->_cnt = 0;

            _unlock_file(file);
            return WEOF;
          }
          wcp[i] = j;
        }
      }

      _unlock_file(file);
      return wc;
    }

  c = fgetc(file);
  if ((__mb_cur_max > 1) && isleadbyte(c))
    {
      FIXME("Treat Multibyte characters\n");
    }

  _unlock_file(file);
  if (c == EOF)
    return WEOF;
  else
    return (wint_t)c;
}

/*********************************************************************
 *		_getw (MSVCRT.@)
 */
int CDECL _getw(FILE* file)
{
  char *ch;
  int i, k;
  unsigned int j;
  ch = (char *)&i;

  _lock_file(file);
  for (j=0; j<sizeof(int); j++) {
    k = fgetc(file);
    if (k == EOF) {
      file->_flag |= _IOEOF;
      _unlock_file(file);
      return EOF;
    }
    ch[j] = k;
  }

  _unlock_file(file);
  return i;
}

/*********************************************************************
 *		getwc (MSVCRT.@)
 */
wint_t CDECL getwc(FILE* file)
{
  return fgetwc(file);
}

/*********************************************************************
 *		_fgetwchar (MSVCRT.@)
 */
wint_t CDECL _fgetwchar(void)
{
  return fgetwc(stdin);
}

/*********************************************************************
 *		getwchar (MSVCRT.@)
 */
wint_t CDECL getwchar(void)
{
  return _fgetwchar();
}

/*********************************************************************
 *              fgetws (MSVCRT.@)
 */
wchar_t * CDECL fgetws(wchar_t *s, int size, FILE* file)
{
  int    cc = WEOF;
  wchar_t * buf_start = s;

  TRACE(":file(%p) fd (%d) str (%p) len (%d)\n",
        file,file->_file,s,size);

  _lock_file(file);

  while ((size >1) && (cc = fgetwc(file)) != WEOF && cc != '\n')
    {
      *s++ = (char)cc;
      size --;
    }
  if ((cc == WEOF) && (s == buf_start)) /* If nothing read, return 0*/
  {
    TRACE(":nothing read\n");
    _unlock_file(file);
    return NULL;
  }
  if ((cc != WEOF) && (size > 1))
    *s++ = cc;
  *s = 0;
  TRACE(":got %s\n", debugstr_w(buf_start));
  _unlock_file(file);
  return buf_start;
}

/*********************************************************************
 *		fwrite (MSVCRT.@)
 */
size_t CDECL fwrite(const void *ptr, size_t size, size_t nmemb, FILE* file)
{
    size_t wrcnt=size * nmemb;
    int written = 0;
    if (size == 0)
        return 0;

    _lock_file(file);
    if(file->_cnt) {
        int pcnt=((unsigned)file->_cnt>wrcnt)? wrcnt: file->_cnt;
        memcpy(file->_ptr, ptr, pcnt);
        file->_cnt -= pcnt;
        file->_ptr += pcnt;
        written = pcnt;
        wrcnt -= pcnt;
        ptr = (const char*)ptr + pcnt;
    } else if(!(file->_flag & _IOWRT)) {
        if(file->_flag & _IORW) {
            file->_flag |= _IOWRT;
        } else {
            _unlock_file(file);
            return 0;
        }
    }
    if(wrcnt) {
        /* Flush buffer */
        int res=flush_buffer(file);
        if(!res) {
            int pwritten = _write(file->_file, ptr, wrcnt);
            if (pwritten <= 0)
            {
                file->_flag |= _IOERR;
                pwritten=0;
            }
            written += pwritten;
        }
    }

    _unlock_file(file);
    return written / size;
}

/*********************************************************************
 *		fputwc (MSVCRT.@)
 * FORKED for ReactOS, don't sync with Wine!
 * References:
 *   - http://jira.reactos.org/browse/CORE-6495
 *   - http://bugs.winehq.org/show_bug.cgi?id=8598
 */
wint_t CDECL fputwc(wchar_t c, FILE* stream)
{
    /* If this is a real file stream (and not some temporary one for
       sprintf-like functions), check whether it is opened in text mode.
       In this case, we have to perform an implicit conversion to ANSI. */
    if (!(stream->_flag & _IOSTRG) && get_ioinfo(stream->_file)->wxflag & WX_TEXT)
    {
        /* Convert to multibyte in text mode */
        char mbc[MB_LEN_MAX];
        int mb_return;

        mb_return = wctomb(mbc, c);

        if(mb_return == -1)
            return WEOF;

        /* Output all characters */
        if (fwrite(mbc, mb_return, 1, stream) != 1)
            return WEOF;
    }
    else
    {
        if (fwrite(&c, sizeof(c), 1, stream) != 1)
            return WEOF;
    }

    return c;
}

/*********************************************************************
 *		_fputwchar (MSVCRT.@)
 */
wint_t CDECL _fputwchar(wint_t wc)
{
  return fputwc(wc, stdout);
}

/*********************************************************************
 *		_wfsopen (MSVCRT.@)
 */
FILE * CDECL _wfsopen(const wchar_t *path, const wchar_t *mode, int share)
{
  FILE* file;
  int open_flags, stream_flags, fd;

  TRACE("(%s,%s)\n", debugstr_w(path), debugstr_w(mode));

  /* map mode string to open() flags. "man fopen" for possibilities. */
  if (get_flags(mode, &open_flags, &stream_flags) == -1)
      return NULL;

  LOCK_FILES();
  fd = _wsopen(path, open_flags, share, _S_IREAD | _S_IWRITE);
  if (fd < 0)
    file = NULL;
  else if ((file = alloc_fp()) && init_fp(file, fd, stream_flags)
   != -1)
    TRACE(":fd (%d) mode (%s) FILE* (%p)\n", fd, debugstr_w(mode), file);
  else if (file)
  {
    file->_flag = 0;
    file = NULL;
  }

  TRACE(":got (%p)\n",file);
  if (fd >= 0 && !file)
    _close(fd);
  UNLOCK_FILES();
  return file;
}

/*********************************************************************
 *		_fsopen (MSVCRT.@)
 */
FILE * CDECL _fsopen(const char *path, const char *mode, int share)
{
    FILE *ret;
    wchar_t *pathW = NULL, *modeW = NULL;

    if (path && !(pathW = msvcrt_wstrdupa(path))) {
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        *_errno() = EINVAL;
        return NULL;
    }
    if (mode && !(modeW = msvcrt_wstrdupa(mode)))
    {
        free(pathW);
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        *_errno() = EINVAL;
        return NULL;
    }

    ret = _wfsopen(pathW, modeW, share);

    free(pathW);
    free(modeW);
    return ret;
}

/*********************************************************************
 *		fopen (MSVCRT.@)
 */
FILE * CDECL fopen(const char *path, const char *mode)
{
    return _fsopen( path, mode, _SH_DENYNO );
}

/*********************************************************************
 *              fopen_s (MSVCRT.@)
 */
int CDECL fopen_s(FILE** pFile,
        const char *filename, const char *mode)
{
    if (!MSVCRT_CHECK_PMT(pFile != NULL) || !MSVCRT_CHECK_PMT(filename != NULL) ||
        !MSVCRT_CHECK_PMT(mode != NULL)) {
        *_errno() = EINVAL;
        return EINVAL;
    }

    *pFile = fopen(filename, mode);

    if(!*pFile)
        return *_errno();
    return 0;
}

/*********************************************************************
 *		_wfopen (MSVCRT.@)
 */
FILE * CDECL _wfopen(const wchar_t *path, const wchar_t *mode)
{
    return _wfsopen( path, mode, _SH_DENYNO );
}

/*********************************************************************
 *		_wfopen_s (MSVCRT.@)
 */
int CDECL _wfopen_s(FILE** pFile, const wchar_t *filename,
        const wchar_t *mode)
{
    if (!MSVCRT_CHECK_PMT(pFile != NULL) || !MSVCRT_CHECK_PMT(filename != NULL) ||
        !MSVCRT_CHECK_PMT(mode != NULL)) {
        *_errno() = EINVAL;
        return EINVAL;
    }

    *pFile = _wfopen(filename, mode);

    if(!*pFile)
        return *_errno();
    return 0;
}

/* fputc calls _flsbuf which calls fputc */
int CDECL _flsbuf(int c, FILE* file);

/*********************************************************************
 *		fputc (MSVCRT.@)
 */
int CDECL fputc(int c, FILE* file)
{
  int res;

  _lock_file(file);
  if(file->_cnt>0) {
    *file->_ptr++=c;
    file->_cnt--;
    if (c == '\n')
    {
      res = flush_buffer(file);
      _unlock_file(file);
      return res ? res : c;
    }
    else {
      _unlock_file(file);
      return c & 0xff;
    }
  } else {
    res = _flsbuf(c, file);
    _unlock_file(file);
    return res;
  }
}

/*********************************************************************
 *		_fputchar (MSVCRT.@)
 */
int CDECL _fputchar(int c)
{
  return fputc(c, stdout);
}

/*********************************************************************
 *		fread (MSVCRT.@)
 */
size_t CDECL fread(void *ptr, size_t size, size_t nmemb, FILE* file)
{
  size_t rcnt=size * nmemb;
  size_t read=0;
  int pread=0;

  if(!rcnt)
	return 0;

  _lock_file(file);

  /* first buffered data */
  if(file->_cnt>0) {
	int pcnt= (rcnt>(unsigned int)file->_cnt)? file->_cnt:rcnt;
	memcpy(ptr, file->_ptr, pcnt);
	file->_cnt -= pcnt;
	file->_ptr += pcnt;
	read += pcnt ;
	rcnt -= pcnt ;
        ptr = (char*)ptr + pcnt;
  } else if(!(file->_flag & _IOREAD )) {
	if(file->_flag & _IORW) {
		file->_flag |= _IOREAD;
	} else {
        _unlock_file(file);
        return 0;
    }
  }
  while(rcnt>0)
  {
    int i;
    /* Fill the buffer on small reads.
     * TODO: Use a better buffering strategy.
     */
    if (!file->_cnt && size*nmemb <= BUFSIZ/2 && !(file->_flag & _IONBF)) {
      if (file->_bufsiz == 0) {
        alloc_buffer(file);
      }
      file->_cnt = _read(file->_file, file->_base, file->_bufsiz);
      file->_ptr = file->_base;
      i = ((unsigned int)file->_cnt<rcnt) ? file->_cnt : rcnt;
      /* If the buffer fill reaches eof but fread wouldn't, clear eof. */
      if (i > 0 && i < file->_cnt) {
        get_ioinfo(file->_file)->wxflag &= ~WX_ATEOF;
        file->_flag &= ~_IOEOF;
      }
      if (i > 0) {
        memcpy(ptr, file->_ptr, i);
        file->_cnt -= i;
        file->_ptr += i;
      }
    } else {
      i = _read(file->_file,ptr, rcnt);
    }
    pread += i;
    rcnt -= i;
    ptr = (char *)ptr+i;
    /* expose feof condition in the flags
     * MFC tests file->_flag for feof, and doesn't call feof())
     */
    if (get_ioinfo(file->_file)->wxflag & WX_ATEOF)
        file->_flag |= _IOEOF;
    else if (i == -1)
    {
        file->_flag |= _IOERR;
        pread = 0;
        rcnt = 0;
    }
    if (i < 1) break;
  }
  read+=pread;
  _unlock_file(file);
  return read / size;
}

/*********************************************************************
 *		_wfreopen (MSVCRT.@)
 *
 */
FILE* CDECL _wfreopen(const wchar_t *path, const wchar_t *mode, FILE* file)
{
  int open_flags, stream_flags, fd;

  TRACE(":path (%p) mode (%s) file (%p) fd (%d)\n", debugstr_w(path), debugstr_w(mode), file, file->_file);

  LOCK_FILES();
  if (!file || ((fd = file->_file) < 0) || fd > fdend)
    file = NULL;
  else
  {
    fclose(file);
    /* map mode string to open() flags. "man fopen" for possibilities. */
    if (get_flags(mode, &open_flags, &stream_flags) == -1)
      file = NULL;
    else
    {
      fd = _wopen(path, open_flags, _S_IREAD | _S_IWRITE);
      if (fd < 0)
        file = NULL;
      else if (init_fp(file, fd, stream_flags) == -1)
      {
          file->_flag = 0;
          WARN(":failed-last error (%d)\n",GetLastError());
          _dosmaperr(GetLastError());
          file = NULL;
      }
    }
  }
  UNLOCK_FILES();
  return file;
}

/*********************************************************************
 *      freopen (MSVCRT.@)
 *
 */
FILE* CDECL freopen(const char *path, const char *mode, FILE* file)
{
    FILE *ret;
    wchar_t *pathW = NULL, *modeW = NULL;

    if (path && !(pathW = msvcrt_wstrdupa(path))) return NULL;
    if (mode && !(modeW = msvcrt_wstrdupa(mode)))
    {
        free(pathW);
        return NULL;
    }

    ret = _wfreopen(pathW, modeW, file);

    free(pathW);
    free(modeW);
    return ret;
}

/*********************************************************************
 *		fsetpos (MSVCRT.@)
 */
int CDECL fsetpos(FILE* file, const fpos_t *pos)
{
  int ret;

  _lock_file(file);
  /* Note that all this has been lifted 'as is' from fseek */
  if(file->_flag & _IOWRT)
	flush_buffer(file);

  /* Discard buffered input */
  file->_cnt = 0;
  file->_ptr = file->_base;

  /* Reset direction of i/o */
  if(file->_flag & _IORW) {
        file->_flag &= ~(_IOREAD|_IOWRT);
  }

  ret = (_lseeki64(file->_file,*pos,SEEK_SET) == -1) ? -1 : 0;
  _unlock_file(file);
  return ret;
}

/*********************************************************************
 *		_ftelli64 (MSVCRT.@)
 */
__int64 CDECL _ftelli64(FILE* file)
{
    /* TODO: just call fgetpos and return lower half of result */
    int off=0;
    __int64 pos;

    _lock_file(file);
    pos = _telli64(file->_file);
    if(pos == -1) {
        _unlock_file(file);
        return -1;
    }
    if(file->_bufsiz)  {
        if( file->_flag & _IOWRT ) {
            off = file->_ptr - file->_base;
        } else {
            off = -file->_cnt;
            if (get_ioinfo(file->_file)->wxflag & WX_TEXT) {
                /* Black magic correction for CR removal */
                int i;
                for (i=0; i<file->_cnt; i++) {
                    if (file->_ptr[i] == '\n')
                        off--;
                }
                /* Black magic when reading CR at buffer boundary*/
                if(get_ioinfo(file->_file)->wxflag & WX_READCR)
                    off--;
            }
        }
    }

    _unlock_file(file);
    return off + pos;
}

/*********************************************************************
 *		ftell (MSVCRT.@)
 */
LONG CDECL ftell(FILE* file)
{
  return (LONG)_ftelli64(file);
}

/*********************************************************************
 *		fgetpos (MSVCRT.@)
 */
int CDECL fgetpos(FILE* file, fpos_t *pos)
{
    int off=0;

    _lock_file(file);
    *pos = _lseeki64(file->_file,0,SEEK_CUR);
    if(*pos == -1) {
        _unlock_file(file);
        return -1;
    }
    if(file->_bufsiz)  {
        if( file->_flag & _IOWRT ) {
            off = file->_ptr - file->_base;
        } else {
            off = -file->_cnt;
            if (get_ioinfo(file->_file)->wxflag & WX_TEXT) {
                /* Black magic correction for CR removal */
                int i;
                for (i=0; i<file->_cnt; i++) {
                    if (file->_ptr[i] == '\n')
                        off--;
                }
                /* Black magic when reading CR at buffer boundary*/
                if(get_ioinfo(file->_file)->wxflag & WX_READCR)
                    off--;
            }
        }
    }
    *pos += off;
    _unlock_file(file);
    return 0;
}

/*********************************************************************
 *		fputs (MSVCRT.@)
 */
int CDECL fputs(const char *s, FILE* file)
{
    size_t i, len = strlen(s);
    int ret;

    _lock_file(file);
    if (!(get_ioinfo(file->_file)->wxflag & WX_TEXT)) {
      ret = fwrite(s,sizeof(*s),len,file) == len ? 0 : EOF;
      _unlock_file(file);
      return ret;
    }
    for (i=0; i<len; i++)
      if (fputc(s[i], file) == EOF)  {
        _unlock_file(file);
        return EOF;
      }

    _unlock_file(file);
    return 0;
}

/*********************************************************************
 *		fputws (MSVCRT.@)
 */
int CDECL fputws(const wchar_t *s, FILE* file)
{
    size_t i, len = strlenW(s);
    int ret;

    _lock_file(file);
    if (!(get_ioinfo(file->_file)->wxflag & WX_TEXT)) {
        ret = fwrite(s,sizeof(*s),len,file) == len ? 0 : EOF;
        _unlock_file(file);
        return ret;
    }
    for (i=0; i<len; i++) {
        if (((s[i] == '\n') && (fputc('\r', file) == EOF))
                || fputwc(s[i], file) == WEOF) {
            _unlock_file(file);
            return WEOF;
        }
    }

    _unlock_file(file);
    return 0;
}

/*********************************************************************
 *		getchar (MSVCRT.@)
 */
int CDECL getchar(void)
{
  return fgetc(stdin);
}

/*********************************************************************
 *		getc (MSVCRT.@)
 */
int CDECL getc(FILE* file)
{
  return fgetc(file);
}

/*********************************************************************
 *		gets (MSVCRT.@)
 */
char * CDECL gets(char *buf)
{
  int    cc;
  char * buf_start = buf;

  _lock_file(stdin);
  for(cc = fgetc(stdin); cc != EOF && cc != '\n';
      cc = fgetc(stdin))
  if(cc != '\r') *buf++ = (char)cc;

  *buf = '\0';

  TRACE("got '%s'\n", buf_start);
  _unlock_file(stdin);
  return buf_start;
}

/*********************************************************************
 *		_getws (MSVCRT.@)
 */
wchar_t* CDECL _getws(wchar_t* buf)
{
    wint_t cc;
    wchar_t* ws = buf;

    _lock_file(stdin);
    for (cc = fgetwc(stdin); cc != WEOF && cc != '\n';
         cc = fgetwc(stdin))
    {
        if (cc != '\r')
            *buf++ = (wchar_t)cc;
    }
    *buf = '\0';

    TRACE("got %s\n", debugstr_w(ws));
    _unlock_file(stdin);
    return ws;
}

/*********************************************************************
 *		putc (MSVCRT.@)
 */
int CDECL putc(int c, FILE* file)
{
  return fputc(c, file);
}

/*********************************************************************
 *		putchar (MSVCRT.@)
 */
int CDECL putchar(int c)
{
  return fputc(c, stdout);
}

/*********************************************************************
 *		_putwch (MSVCRT.@)
 */
wint_t CDECL _putwch(wchar_t c)
{
  return fputwc(c, stdout);
}

/*********************************************************************
 *		puts (MSVCRT.@)
 */
int CDECL puts(const char *s)
{
    size_t len = strlen(s);
    int ret;

    _lock_file(stdout);
    if(fwrite(s, sizeof(*s), len, stdout) != len) {
        _unlock_file(stdout);
        return EOF;
    }

    ret = fwrite("\n",1,1,stdout) == 1 ? 0 : EOF;
    _unlock_file(stdout);
    return ret;
}

/*********************************************************************
 *		_putws (MSVCRT.@)
 */
int CDECL _putws(const wchar_t *s)
{
    static const wchar_t nl = '\n';
    size_t len = strlenW(s);
    int ret;

    _lock_file(stdout);
    if(fwrite(s, sizeof(*s), len, stdout) != len) {
        _unlock_file(stdout);
        return EOF;
    }

    ret = fwrite(&nl,sizeof(nl),1,stdout) == 1 ? 0 : EOF;
    _unlock_file(stdout);
    return ret;
}

/*********************************************************************
 *		remove (MSVCRT.@)
 */
int CDECL remove(const char *path)
{
  TRACE("(%s)\n",path);
  if (DeleteFileA(path))
    return 0;
  TRACE(":failed (%d)\n",GetLastError());
  _dosmaperr(GetLastError());
  return -1;
}

/*********************************************************************
 *		_wremove (MSVCRT.@)
 */
int CDECL _wremove(const wchar_t *path)
{
  TRACE("(%s)\n",debugstr_w(path));
  if (DeleteFileW(path))
    return 0;
  TRACE(":failed (%d)\n",GetLastError());
  _dosmaperr(GetLastError());
  return -1;
}

/*********************************************************************
 *		rename (MSVCRT.@)
 */
int CDECL rename(const char *oldpath,const char *newpath)
{
  TRACE(":from %s to %s\n",oldpath,newpath);
  if (MoveFileExA(oldpath, newpath, MOVEFILE_COPY_ALLOWED))
    return 0;
  TRACE(":failed (%d)\n",GetLastError());
  _dosmaperr(GetLastError());
  return -1;
}

/*********************************************************************
 *		_wrename (MSVCRT.@)
 */
int CDECL _wrename(const wchar_t *oldpath,const wchar_t *newpath)
{
  TRACE(":from %s to %s\n",debugstr_w(oldpath),debugstr_w(newpath));
  if (MoveFileExW(oldpath, newpath, MOVEFILE_COPY_ALLOWED))
    return 0;
  TRACE(":failed (%d)\n",GetLastError());
  _dosmaperr(GetLastError());
  return -1;
}

/*********************************************************************
 *		setvbuf (MSVCRT.@)
 */
int CDECL setvbuf(FILE* file, char *buf, int mode, size_t size)
{
  _lock_file(file);
  if(file->_bufsiz) {
	free(file->_base);
	file->_bufsiz = 0;
	file->_cnt = 0;
  }
  if(mode == _IOFBF) {
	file->_flag &= ~_IONBF;
  	file->_base = file->_ptr = buf;
  	if(buf) {
		file->_bufsiz = size;
	}
  } else {
	file->_flag |= _IONBF;
  }
  _unlock_file(file);
  return 0;
}

/*********************************************************************
 *		setbuf (MSVCRT.@)
 */
void CDECL setbuf(FILE* file, char *buf)
{
  setvbuf(file, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}

/*********************************************************************
 *		tmpnam (MSVCRT.@)
 */
char * CDECL tmpnam(char *s)
{
  char tmpstr[16];
  char *p;
  int count, size;

  if (!s) {
    thread_data_t *data = msvcrt_get_thread_data();

    if(!data->tmpnam_buffer)
      data->tmpnam_buffer = malloc(MAX_PATH);

    s = data->tmpnam_buffer;
  }

  int_to_base32(GetCurrentProcessId(), tmpstr);
  p = s + sprintf(s, "\\s%s.", tmpstr);
  for (count = 0; count < TMP_MAX; count++)
  {
    size = int_to_base32(tmpnam_unique++, tmpstr);
    memcpy(p, tmpstr, size);
    if (GetFileAttributesA(s) == INVALID_FILE_ATTRIBUTES &&
        GetLastError() == ERROR_FILE_NOT_FOUND)
      break;
  }
  return s;
}

/*********************************************************************
 *              _wtmpnam (MSVCRT.@)
 */
wchar_t * CDECL _wtmpnam(wchar_t *s)
{
    static const wchar_t format[] = {'\\','s','%','s','.',0};
    wchar_t tmpstr[16];
    wchar_t *p;
    int count, size;
    if (!s) {
        thread_data_t *data = msvcrt_get_thread_data();

        if(!data->wtmpnam_buffer)
            data->wtmpnam_buffer = malloc(sizeof(wchar_t[MAX_PATH]));

        s = data->wtmpnam_buffer;
    }

    int_to_base32_w(GetCurrentProcessId(), tmpstr);
    p = s + _snwprintf(s, MAX_PATH, format, tmpstr);
    for (count = 0; count < TMP_MAX; count++)
    {
        size = int_to_base32_w(tmpnam_unique++, tmpstr);
        memcpy(p, tmpstr, size*sizeof(wchar_t));
        if (GetFileAttributesW(s) == INVALID_FILE_ATTRIBUTES &&
                GetLastError() == ERROR_FILE_NOT_FOUND)
            break;
    }
    return s;
}

/*********************************************************************
 *		tmpfile (MSVCRT.@)
 */
FILE* CDECL tmpfile(void)
{
  char *filename = tmpnam(NULL);
  int fd;
  FILE* file = NULL;

  LOCK_FILES();
  fd = _open(filename, _O_CREAT | _O_BINARY | _O_RDWR | _O_TEMPORARY);
  if (fd != -1 && (file = alloc_fp()))
  {
    if (init_fp(file, fd, _O_RDWR) == -1)
    {
        file->_flag = 0;
        file = NULL;
    }
    else file->_tmpfname = _strdup(filename);
  }
  UNLOCK_FILES();
  return file;
}

/*********************************************************************
 *		ungetc (MSVCRT.@)
 */
int CDECL ungetc(int c, FILE * file)
{
    if (c == EOF)
        return EOF;

    _lock_file(file);
    if(file->_bufsiz == 0) {
        alloc_buffer(file);
        file->_ptr++;
    }
    if(file->_ptr>file->_base) {
        file->_ptr--;
        *file->_ptr=c;
        file->_cnt++;
        clearerr(file);
        _unlock_file(file);
        return c;
    }

    _unlock_file(file);
    return EOF;
}

/*********************************************************************
 *              ungetwc (MSVCRT.@)
 */
wint_t CDECL ungetwc(wint_t wc, FILE * file)
{
    wchar_t mwc = wc;
    char * pp = (char *)&mwc;
    int i;

    _lock_file(file);
    for(i=sizeof(wchar_t)-1;i>=0;i--) {
        if(pp[i] != ungetc(pp[i],file)) {
            _unlock_file(file);
            return WEOF;
        }
    }

    _unlock_file(file);
    return mwc;
}



/*********************************************************************
 *		_getmaxstdio (MSVCRT.@)
 */
int CDECL _getmaxstdio(void)
{
    return max_streams;
}

/*********************************************************************
 *		_setmaxstdio (MSVCRT.@)
 */
int CDECL _setmaxstdio(int newmax)
{
    TRACE("%d\n", newmax);

    if(newmax<_IOB_ENTRIES || newmax>MAX_FILES || newmax<stream_idx)
        return -1;

    max_streams = newmax;
    return max_streams;
}
