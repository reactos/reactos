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

extern int _commode;

#ifndef _IOCOMMIT
#define _IOCOMMIT 0x4000
#endif

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
#define WX_READNL         0x04  /* read started with \n */
#define WX_READEOF        0x04  /* like ATEOF, but for underlying file rather than buffer */
#define WX_PIPE           0x08
#define WX_READCR         0x08  /* underlying file is at \r */
#define WX_DONTINHERIT    0x10
#define WX_APPEND         0x20
#define WX_NOSEEK         0x40
#define WX_TEXT           0x80

/* values for exflag - it's used differently in msvcr90.dll*/
#define EF_UTF8           0x01
#define EF_UTF16          0x02
#define EF_UNK_UNICODE    0x08

static char utf8_bom[3] = { 0xef, 0xbb, 0xbf };
static char utf16_bom[2] = { 0xff, 0xfe };

/* FIXME: this should be allocated dynamically */
#define MAX_FILES 2048
#define FD_BLOCK_SIZE 64

#define MSVCRT_INTERNAL_BUFSIZ 4096

/* ioinfo structure size is different in msvcrXX.dll's */
typedef struct {
    HANDLE              handle;
    unsigned char       wxflag;
    char                lookahead[3];
    int                 exflag;
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
static CRITICAL_SECTION_DEBUG file_cs_debug =
{
    0, 0, &file_cs,
    { &file_cs_debug.ProcessLocksList, &file_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": file_cs") }
};
static CRITICAL_SECTION file_cs = { &file_cs_debug, -1, 0, 0, 0, 0 };
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
  ioinfo *fdinfo;

  LOCK_FILES();
  fdinfo = get_ioinfo(fd);
  if(fdinfo != &__badioinfo)
  {
    fdinfo->handle = INVALID_HANDLE_VALUE;
    fdinfo->wxflag = 0;
  }
  TRACE(":fd (%d) freed\n",fd);

  if (fd < 3)
  {
    switch (fd)
    {
    case 0:
        SetStdHandle(STD_INPUT_HANDLE, 0);
        break;
    case 1:
        SetStdHandle(STD_OUTPUT_HANDLE, 0);
        break;
    case 2:
        SetStdHandle(STD_ERROR_HANDLE, 0);
        break;
    }
  }

  if (fd == fdend - 1)
    fdend--;
  if (fd < fdstart)
    fdstart = fd;
  UNLOCK_FILES();
}

/* INTERNAL: Allocate an fd slot from a Win32 HANDLE, starting from fd */
/* caller must hold the files lock */
static int set_fd(HANDLE hand, int flag, int fd)
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
  fdinfo->wxflag = WX_OPEN | (flag & (WX_DONTINHERIT | WX_APPEND | WX_TEXT | WX_PIPE | WX_NOSEEK));
  fdinfo->lookahead[0] = '\n';
  fdinfo->lookahead[1] = '\n';
  fdinfo->lookahead[2] = '\n';
  fdinfo->exflag = 0;

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
  ret = set_fd(hand, flag, fdstart);
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
  file->_ptr = file->_base = NULL;
  file->_cnt = 0;
  file->_file = fd;
  file->_flag = stream_flags;
  file->_tmpfname = NULL;

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
        set_fd(*handle_ptr, *wxflag_ptr, i);

      wxflag_ptr++; handle_ptr++;
    }
    fdend = max( 3, count );
    for (fdstart = 3; fdstart < fdend; fdstart++)
        if (get_ioinfo(fdstart)->handle == INVALID_HANDLE_VALUE) break;
  }

  fdinfo = get_ioinfo(STDIN_FILENO);
  if (!(fdinfo->wxflag & WX_OPEN) || fdinfo->handle == INVALID_HANDLE_VALUE) {
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    DWORD type = GetFileType(h);

    set_fd(h, WX_OPEN|WX_TEXT|((type&0xf)==FILE_TYPE_CHAR ? WX_NOSEEK : 0)
            |((type&0xf)==FILE_TYPE_PIPE ? WX_PIPE : 0), STDIN_FILENO);
  }

  fdinfo = get_ioinfo(STDOUT_FILENO);
  if (!(fdinfo->wxflag & WX_OPEN) || fdinfo->handle == INVALID_HANDLE_VALUE) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD type = GetFileType(h);

    set_fd(h, WX_OPEN|WX_TEXT|((type&0xf)==FILE_TYPE_CHAR ? WX_NOSEEK : 0)
            |((type&0xf)==FILE_TYPE_PIPE ? WX_PIPE : 0), STDOUT_FILENO);
  }

  fdinfo = get_ioinfo(STDERR_FILENO);
  if (!(fdinfo->wxflag & WX_OPEN) || fdinfo->handle == INVALID_HANDLE_VALUE) {
    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    DWORD type = GetFileType(h);

    set_fd(h, WX_OPEN|WX_TEXT|((type&0xf)==FILE_TYPE_CHAR ? WX_NOSEEK : 0)
            |((type&0xf)==FILE_TYPE_PIPE ? WX_PIPE : 0), STDERR_FILENO);
  }

  TRACE(":handles (%p)(%p)(%p)\n", get_ioinfo(STDIN_FILENO)->handle,
        get_ioinfo(STDOUT_FILENO)->handle,
        get_ioinfo(STDERR_FILENO)->handle);

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
  if(file->_flag & (_IOMYBUF | _USERBUF)) {
        int cnt=file->_ptr-file->_base;
        if(cnt>0 && _write(file->_file, file->_base, cnt) != cnt) {
            file->_flag |= _IOERR;
            return EOF;
        }
        file->_ptr=file->_base;
        file->_cnt=0;
  }
  return 0;
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

/* INTERNAL: Allocate stdio file buffer */
/*static*/ BOOL alloc_buffer(FILE* file)
{
    if((file->_file==STDOUT_FILENO || file->_file==STDERR_FILENO)
            && _isatty(file->_file))
        return FALSE;

    file->_base = calloc(MSVCRT_INTERNAL_BUFSIZ,1);
    if(file->_base) {
        file->_bufsiz = MSVCRT_INTERNAL_BUFSIZ;
        file->_flag |= _IOMYBUF;
    } else {
        file->_base = (char*)(&file->_charbuf);
        file->_bufsiz = 2;
        file->_flag |= _IONBF;
    }
    file->_ptr = file->_base;
    file->_cnt = 0;
    return TRUE;
}

/* INTERNAL: Allocate temporary buffer for stdout and stderr */
static BOOL add_std_buffer(FILE *file)
{
    static char buffers[2][BUFSIZ];

    if((file->_file!=STDOUT_FILENO && file->_file!=STDERR_FILENO)
            || !_isatty(file->_file) || file->_bufsiz)
        return FALSE;

    file->_ptr = file->_base = buffers[file->_file == STDOUT_FILENO ? 0 : 1];
    file->_bufsiz = file->_cnt = BUFSIZ;
    return TRUE;
}

/* INTERNAL: Removes temporary buffer from stdout or stderr */
/* Only call this function when add_std_buffer returned TRUE */
static void remove_std_buffer(FILE *file)
{
    flush_buffer(file);
    file->_ptr = file->_base = NULL;
    file->_bufsiz = file->_cnt = 0;
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

/* INTERNAL: Flush all stream buffer */
static int flush_all_buffers(int mask)
{
  int i, num_flushed = 0;
  FILE *file;

  LOCK_FILES();
  for (i = 0; i < stream_idx; i++) {
    file = get_file(i);

    if (file->_flag)
    {
      if(file->_flag & mask) {
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
 *		_flushall (MSVCRT.@)
 */
int CDECL _flushall(void)
{
    return flush_all_buffers(_IOWRT | _IOREAD);
}

/*********************************************************************
 *		fflush (MSVCRT.@)
 */
int CDECL fflush(FILE* file)
{
    if(!file) {
        flush_all_buffers(_IOWRT);
    } else if(file->_flag & _IOWRT) {
        int res;

        _lock_file(file);
        res = flush_buffer(file);
        /* FIXME
        if(!res && (file->_flag & _IOCOMMIT))
            res = _commit(file->_file) ? EOF : 0;
        */
        _unlock_file(file);

        return res;
    } else if(file->_flag & _IOREAD) {
        _lock_file(file);
        file->_cnt = 0;
        file->_ptr = file->_base;
        _unlock_file(file);

        return 0;
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
      ret = set_fd(handle, wxflag, nd);
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
    unsigned int i;
    int j;

    _flushall();
    _fcloseall();

    for(i=0; i<sizeof(__pioinfo)/sizeof(__pioinfo[0]); i++)
        free(__pioinfo[i]);

    for(j=0; j<stream_idx; j++)
    {
        FILE *file = get_file(j);
        if(file<_iob || file>=_iob+_IOB_ENTRIES)
        {
            ((file_crit*)file)->crit.DebugInfo->Spare[0] = 0;
            DeleteCriticalSection(&((file_crit*)file)->crit);
        }
    }

    for(i=0; i<sizeof(fstream)/sizeof(fstream[0]); i++)
        free(fstream[i]);
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
      whence = SEEK_SET;
      offset += _ftelli64(file);
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
 *		_chsize_s (MSVCRT.@)
 */
int CDECL _chsize_s(int fd, __int64 size)
{
    __int64 cur, pos;
    HANDLE handle;
    BOOL ret = FALSE;

    TRACE("(fd=%d, size=%s)\n", fd, wine_dbgstr_longlong(size));

    if (!MSVCRT_CHECK_PMT(size >= 0)) return EINVAL;

    LOCK_FILES();

    handle = fdtoh(fd);
    if (handle != INVALID_HANDLE_VALUE)
    {
        /* save the current file pointer */
        cur = _lseeki64(fd, 0, SEEK_CUR);
        if (cur >= 0)
        {
            pos = _lseeki64(fd, size, SEEK_SET);
            if (pos >= 0)
            {
                ret = SetEndOfFile(handle);
                if (!ret) _dosmaperr(GetLastError());
            }

            /* restore the file pointer */
            _lseeki64(fd, cur, SEEK_SET);
        }
    }

    UNLOCK_FILES();
    return ret ? 0 : *_errno();
}

/*********************************************************************
 *		_chsize (MSVCRT.@)
 */
int CDECL _chsize(int fd, long size)
{
    /* _chsize_s returns errno on failure but _chsize should return -1 */
    return _chsize_s( fd, size ) == 0 ? 0 : -1;
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

  TRACE("%s\n", debugstr_w(mode));

  while(*mode == ' ') mode++;

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
    MSVCRT_INVALID_PMT(0, EINVAL);
    return -1;
  }

  *stream_flags |= _commode;

  while (*mode && *mode!=',')
    switch (*mode++)
    {
    case 'B': case 'b':
      *open_flags |=  _O_BINARY;
      *open_flags &= ~_O_TEXT;
      break;
    case 't':
      *open_flags |=  _O_TEXT;
      *open_flags &= ~_O_BINARY;
      break;
    case 'D':
      *open_flags |= _O_TEMPORARY;
      break;
    case 'T':
      *open_flags |= _O_SHORT_LIVED;
      break;
    case 'c':
      *stream_flags |= _IOCOMMIT;
      break;
    case 'n':
      *stream_flags &= ~_IOCOMMIT;
      break;
    case 'N':
      *open_flags |= _O_NOINHERIT;
      break;
    case '+':
    case ' ':
    case 'a':
    case 'w':
      break;
    case 'S':
    case 'R':
      FIXME("ignoring cache optimization flag: %c\n", mode[-1]);
      break;
    default:
      ERR("incorrect mode flag: %c\n", mode[-1]);
      break;
    }

  if(*mode == ',')
  {
    static const WCHAR ccs[] = {'c','c','s'};
    static const WCHAR utf8[] = {'u','t','f','-','8'};
    static const WCHAR utf16le[] = {'u','t','f','-','1','6','l','e'};
    static const WCHAR unicode[] = {'u','n','i','c','o','d','e'};

    mode++;
    while(*mode == ' ') mode++;
    if(!MSVCRT_CHECK_PMT(!strncmpW(ccs, mode, sizeof(ccs)/sizeof(ccs[0]))))
      return -1;
    mode += sizeof(ccs)/sizeof(ccs[0]);
    while(*mode == ' ') mode++;
    if(!MSVCRT_CHECK_PMT(*mode == '='))
        return -1;
    mode++;
    while(*mode == ' ') mode++;

    if(!strncmpiW(utf8, mode, sizeof(utf8)/sizeof(utf8[0])))
    {
      *open_flags |= _O_U8TEXT;
      mode += sizeof(utf8)/sizeof(utf8[0]);
    }
    else if(!strncmpiW(utf16le, mode, sizeof(utf16le)/sizeof(utf16le[0])))
    {
      *open_flags |= _O_U16TEXT;
      mode += sizeof(utf16le)/sizeof(utf16le[0]);
    }
    else if(!strncmpiW(unicode, mode, sizeof(unicode)/sizeof(unicode[0])))
    {
      *open_flags |= _O_WTEXT;
      mode += sizeof(unicode)/sizeof(unicode[0]);
    }
    else
    {
      MSVCRT_INVALID_PMT(0, EINVAL);
      return -1;
    }

    while(*mode == ' ') mode++;
  }

  if(!MSVCRT_CHECK_PMT(*mode == 0))
    return -1;
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
 *		_mktemp (MSVCRT.@)
 */
char * CDECL _mktemp(char *pattern)
{
  int numX = 0;
  char *retVal = pattern;
  int id;
  char letter = 'a';

  if(!pattern)
      return NULL;

  while(*pattern)
    numX = (*pattern++ == 'X')? numX + 1 : 0;
  if (numX < 6)
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
    if (GetFileAttributesA(retVal) == INVALID_FILE_ATTRIBUTES)
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
    else if (oflags & _O_WTEXT)          wxflags |= WX_TEXT;
    else if (oflags & _O_U16TEXT)        wxflags |= WX_TEXT;
    else if (oflags & _O_U8TEXT)         wxflags |= WX_TEXT;
    else if (*__p__fmode() & _O_BINARY)  {/* Nothing to do */}
    else                                        wxflags |= WX_TEXT; /* default to TEXT*/
    if (oflags & _O_NOINHERIT)           wxflags |= WX_DONTINHERIT;

    if ((unsupp = oflags & ~(
                    _O_BINARY|_O_TEXT|_O_APPEND|
                    _O_TRUNC|_O_EXCL|_O_CREAT|
                    _O_RDWR|_O_WRONLY|_O_TEMPORARY|
                    _O_NOINHERIT|
                    _O_SEQUENTIAL|_O_RANDOM|_O_SHORT_LIVED|
                    _O_WTEXT|_O_U16TEXT|_O_U8TEXT
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

static int check_bom(HANDLE h, int oflags, BOOL seek)
{
    char bom[sizeof(utf8_bom)];
    DWORD r;

    oflags &= ~(_O_WTEXT|_O_U16TEXT|_O_U8TEXT);

    if (!ReadFile(h, bom, sizeof(utf8_bom), &r, NULL))
        return oflags;

    if (r==sizeof(utf8_bom) && !memcmp(bom, utf8_bom, sizeof(utf8_bom))) {
        oflags |= _O_U8TEXT;
    }else if (r>=sizeof(utf16_bom) && !memcmp(bom, utf16_bom, sizeof(utf16_bom))) {
        if (seek && r>2)
            SetFilePointer(h, 2, NULL, FILE_BEGIN);
        oflags |= _O_U16TEXT;
    }else if (seek) {
        SetFilePointer(h, 0, NULL, FILE_BEGIN);
    }

    return oflags;
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

  if (!MSVCRT_CHECK_PMT( fd != NULL )) return EINVAL;

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
  sa.bInheritHandle       = !(oflags & _O_NOINHERIT);

  if ((oflags&(_O_WTEXT|_O_U16TEXT|_O_U8TEXT))
          && (creation==OPEN_ALWAYS || creation==OPEN_EXISTING)
          && !(access&GENERIC_READ))
  {
      hand = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
              &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
      if (hand != INVALID_HANDLE_VALUE)
      {
          oflags = check_bom(hand, oflags, FALSE);
          CloseHandle(hand);
      }
      else
          oflags &= ~(_O_WTEXT|_O_U16TEXT|_O_U8TEXT);
  }

  hand = CreateFileW(path, access, sharing, &sa, creation, attrib, 0);
  if (hand == INVALID_HANDLE_VALUE)  {
    WARN(":failed-last error (%d)\n",GetLastError());
    _dosmaperr(GetLastError());
    return *_errno();
  }

  if (oflags & (_O_WTEXT|_O_U16TEXT|_O_U8TEXT))
  {
      if ((access & GENERIC_WRITE) && (creation==CREATE_NEW
                  || creation==CREATE_ALWAYS || creation==TRUNCATE_EXISTING
                  || (creation==OPEN_ALWAYS && GetLastError()==ERROR_ALREADY_EXISTS)))
      {
          if (oflags & _O_U8TEXT)
          {
              DWORD written = 0, tmp;

              while(written!=sizeof(utf8_bom) && WriteFile(hand, (char*)utf8_bom+written,
                          sizeof(utf8_bom)-written, &tmp, NULL))
                  written += tmp;
              if (written != sizeof(utf8_bom)) {
                  WARN("error writing BOM\n");
                  CloseHandle(hand);
                  _dosmaperr(GetLastError());
                  return *_errno();
              }
          }
          else
          {
              DWORD written = 0, tmp;

              while(written!=sizeof(utf16_bom) && WriteFile(hand, (char*)utf16_bom+written,
                          sizeof(utf16_bom)-written, &tmp, NULL))
                  written += tmp;
              if (written != sizeof(utf16_bom))
              {
                  WARN("error writing BOM\n");
                  CloseHandle(hand);
                  _dosmaperr(GetLastError());
                  return *_errno();
              }
          }
      }
      else if (access & GENERIC_READ)
          oflags = check_bom(hand, oflags, TRUE);
  }

  *fd = alloc_fd(hand, wxflag);
  if (*fd == -1)
      return *_errno();

  if (oflags & _O_WTEXT)
      get_ioinfo(*fd)->exflag |= EF_UTF16|EF_UNK_UNICODE;
  else if (oflags & _O_U16TEXT)
      get_ioinfo(*fd)->exflag |= EF_UTF16;
  else if (oflags & _O_U8TEXT)
      get_ioinfo(*fd)->exflag |= EF_UTF8;

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
    __ms_va_list ap;

    __ms_va_start(ap, shflags);
    pmode = va_arg(ap, int);
    __ms_va_end(ap);
  }
  else
    pmode = 0;

  _wsopen_s(&fd, path, oflags, shflags, pmode);
  return fd;
}

/*********************************************************************
 *              _sopen_s (MSVCRT.@)
 */
int CDECL _sopen_s( int *fd, const char *path, int oflags, int shflags, int pmode )
{
    wchar_t *pathW;
    int ret;

    if(!MSVCRT_CHECK_PMT(path && (pathW = msvcrt_wstrdupa(path))))
        return EINVAL;

    ret = _wsopen_s(fd, pathW, oflags, shflags, pmode);
    free(pathW);
    return ret;
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
  DWORD flags;
  int fd;

  /* _O_RDONLY (0) always matches, so set the read flag
   * MFC's CStdioFile clears O_RDONLY (0)! if it wants to write to the
   * file, so set the write flag. It also only sets _O_TEXT if it wants
   * text - it never sets _O_BINARY.
   */
  /* don't let split_oflags() decide the mode if no mode is passed */
  if (!(oflags & (_O_BINARY | _O_TEXT)))
      oflags |= _O_BINARY;

  flags = GetFileType((HANDLE)handle);
  if (flags==FILE_TYPE_UNKNOWN && GetLastError()!=NO_ERROR)
  {
    _dosmaperr(GetLastError());
    return -1;
  }

  if (flags == FILE_TYPE_CHAR)
    flags = WX_NOSEEK;
  else if (flags == FILE_TYPE_PIPE)
    flags = WX_PIPE;
  else
    flags = 0;
  flags |= split_oflags(oflags);

  fd = alloc_fd((HANDLE)handle, flags);
  TRACE(":handle (%ld) fd (%d) flags 0x%08x\n", handle, fd, flags);
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

static inline int get_utf8_char_len(char ch)
{
    if((ch&0xf8) == 0xf0)
        return 4;
    else if((ch&0xf0) == 0xe0)
        return 3;
    else if((ch&0xe0) == 0xc0)
        return 2;
    return 1;
}

/*********************************************************************
 * (internal) read_utf8
 */
static int read_utf8(int fd, wchar_t *buf, unsigned int count)
{
    ioinfo *fdinfo = get_ioinfo(fd);
    HANDLE hand = fdinfo->handle;
    char min_buf[4], *readbuf, lookahead;
    DWORD readbuf_size, pos=0, num_read=1, char_len, i, j;

    /* make the buffer big enough to hold at least one character */
    /* read bytes have to fit to output and lookahead buffers */
    count /= 2;
    readbuf_size = count < 4 ? 4 : count;
    if(readbuf_size<=4 || !(readbuf = malloc(readbuf_size))) {
        readbuf_size = 4;
        readbuf = min_buf;
    }

    if(fdinfo->lookahead[0] != '\n') {
        readbuf[pos++] = fdinfo->lookahead[0];
        fdinfo->lookahead[0] = '\n';

        if(fdinfo->lookahead[1] != '\n') {
            readbuf[pos++] = fdinfo->lookahead[1];
            fdinfo->lookahead[1] = '\n';

            if(fdinfo->lookahead[2] != '\n') {
                readbuf[pos++] = fdinfo->lookahead[2];
                fdinfo->lookahead[2] = '\n';
            }
        }
    }

    /* NOTE: this case is broken in native dll, reading
     *        sometimes fails when small buffer is passed
     */
    if(count < 4) {
        if(!pos && !ReadFile(hand, readbuf, 1, &num_read, NULL)) {
            if (GetLastError() == ERROR_BROKEN_PIPE) {
                fdinfo->wxflag |= WX_ATEOF;
                return 0;
            }else {
                _dosmaperr(GetLastError());
                return -1;
            }
        }else if(!num_read) {
            fdinfo->wxflag |= WX_ATEOF;
            return 0;
        }else {
            pos++;
        }

        char_len = get_utf8_char_len(readbuf[0]);
        if(char_len>pos) {
            if(ReadFile(hand, readbuf+pos, char_len-pos, &num_read, NULL))
                pos += num_read;
        }

        if(readbuf[0] == '\n')
            fdinfo->wxflag |= WX_READNL;
        else
            fdinfo->wxflag &= ~WX_READNL;

        if(readbuf[0] == 0x1a) {
            fdinfo->wxflag |= WX_ATEOF;
            return 0;
        }

        if(readbuf[0] == '\r') {
            if(!ReadFile(hand, &lookahead, 1, &num_read, NULL) || num_read!=1)
                buf[0] = '\r';
            else if(lookahead == '\n')
                buf[0] = '\n';
            else {
                buf[0] = '\r';
                if(fdinfo->wxflag & (WX_PIPE | WX_NOSEEK))
                    fdinfo->lookahead[0] = lookahead;
                else
                    SetFilePointer(fdinfo->handle, -1, NULL, FILE_CURRENT);
            }
            return 2;
        }

        if(!(num_read = MultiByteToWideChar(CP_UTF8, 0, readbuf, pos, buf, count))) {
            _dosmaperr(GetLastError());
            return -1;
        }

        return num_read*2;
    }

    if(!ReadFile(hand, readbuf+pos, readbuf_size-pos, &num_read, NULL)) {
        if(pos) {
            num_read = 0;
        }else if(GetLastError() == ERROR_BROKEN_PIPE) {
            fdinfo->wxflag |= WX_ATEOF;
            if (readbuf != min_buf) free(readbuf);
            return 0;
        }else {
            _dosmaperr(GetLastError());
            if (readbuf != min_buf) free(readbuf);
            return -1;
        }
    }else if(!pos && !num_read) {
        fdinfo->wxflag |= WX_ATEOF;
        if (readbuf != min_buf) free(readbuf);
        return 0;
    }

    pos += num_read;
    if(readbuf[0] == '\n')
        fdinfo->wxflag |= WX_READNL;
    else
        fdinfo->wxflag &= ~WX_READNL;

    /* Find first byte of last character (may be incomplete) */
    for(i=pos-1; i>0 && i>pos-4; i--)
        if((readbuf[i]&0xc0) != 0x80)
            break;
    char_len = get_utf8_char_len(readbuf[i]);
    if(char_len+i <= pos)
        i += char_len;

    if(fdinfo->wxflag & (WX_PIPE | WX_NOSEEK)) {
        if(i < pos)
            fdinfo->lookahead[0] = readbuf[i];
        if(i+1 < pos)
            fdinfo->lookahead[1] = readbuf[i+1];
        if(i+2 < pos)
            fdinfo->lookahead[2] = readbuf[i+2];
    }else if(i < pos) {
        SetFilePointer(fdinfo->handle, i-pos, NULL, FILE_CURRENT);
    }
    pos = i;

    for(i=0, j=0; i<pos; i++) {
        if(readbuf[i] == 0x1a) {
            fdinfo->wxflag |= WX_ATEOF;
            break;
        }

        /* strip '\r' if followed by '\n' */
        if(readbuf[i] == '\r' && i+1==pos) {
            if(fdinfo->lookahead[0] != '\n' || !ReadFile(hand, &lookahead, 1, &num_read, NULL) || !num_read) {
                readbuf[j++] = '\r';
            }else if(lookahead == '\n' && j==0) {
                readbuf[j++] = '\n';
            }else {
                if(lookahead != '\n')
                    readbuf[j++] = '\r';

                if(fdinfo->wxflag & (WX_PIPE | WX_NOSEEK))
                    fdinfo->lookahead[0] = lookahead;
                else
                    SetFilePointer(fdinfo->handle, -1, NULL, FILE_CURRENT);
            }
        }else if(readbuf[i]!='\r' || readbuf[i+1]!='\n') {
            readbuf[j++] = readbuf[i];
        }
    }
    pos = j;

    if(!(num_read = MultiByteToWideChar(CP_UTF8, 0, readbuf, pos, buf, count))) {
        _dosmaperr(GetLastError());
        if (readbuf != min_buf) free(readbuf);
        return -1;
    }

    if (readbuf != min_buf) free(readbuf);
    return num_read*2;
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
    DWORD num_read, utf16;
    char *bufstart = buf;
    HANDLE hand = fdtoh(fd);
    ioinfo *fdinfo = get_ioinfo(fd);

    if (count == 0)
        return 0;

    if (fdinfo->wxflag & WX_ATEOF) {
        TRACE("already at EOF, returning 0\n");
        return 0;
    }
    /* Don't trace small reads, it gets *very* annoying */
    if (count > 4)
        TRACE(":fd (%d) handle (%p) buf (%p) len (%d)\n",fd,hand,buf,count);
    if (hand == INVALID_HANDLE_VALUE)
    {
        *_errno() = EBADF;
        return -1;
    }

    utf16 = (fdinfo->exflag & EF_UTF16) != 0;
    if (((fdinfo->exflag&EF_UTF8) || utf16) && count&1)
    {
        *_errno() = EINVAL;
        return -1;
    }

    if((fdinfo->wxflag&WX_TEXT) && (fdinfo->exflag&EF_UTF8))
        return read_utf8(fd, buf, count);

    if (fdinfo->lookahead[0]!='\n' || ReadFile(hand, bufstart, count, &num_read, NULL))
    {
        if (fdinfo->lookahead[0] != '\n')
        {
            bufstart[0] = fdinfo->lookahead[0];
            fdinfo->lookahead[0] = '\n';

            if (utf16)
            {
                bufstart[1] =  fdinfo->lookahead[1];
                fdinfo->lookahead[1] = '\n';
            }

            if(count>1+utf16 && ReadFile(hand, bufstart+1+utf16, count-1-utf16, &num_read, NULL))
                num_read += 1+utf16;
            else
                num_read = 1+utf16;
        }

        if(utf16 && (num_read&1))
        {
            /* msvcr90 uses uninitialized value from the buffer in this case */
            /* msvcrt ignores additional data */
            ERR("got odd number of bytes in UTF16 mode\n");
            num_read--;
        }

        if (count != 0 && num_read == 0)
        {
            fdinfo->wxflag |= WX_ATEOF;
            TRACE(":EOF %s\n",debugstr_an(buf,num_read));
        }
        else if (fdinfo->wxflag & WX_TEXT)
        {
            DWORD i, j;

            if (bufstart[0]=='\n' && (!utf16 || bufstart[1]==0))
                fdinfo->wxflag |= WX_READNL;
            else
                fdinfo->wxflag &= ~WX_READNL;

            for (i=0, j=0; i<num_read; i+=1+utf16)
            {
                /* in text mode, a ctrl-z signals EOF */
                if (bufstart[i]==0x1a && (!utf16 || bufstart[i+1]==0))
                {
                    fdinfo->wxflag |= WX_ATEOF;
                    TRACE(":^Z EOF %s\n",debugstr_an(buf,num_read));
                    break;
                }

                /* in text mode, strip \r if followed by \n */
                if (bufstart[i]=='\r' && (!utf16 || bufstart[i+1]==0) && i+1+utf16==num_read)
                {
                    char lookahead[2];
                    DWORD len;

                    lookahead[1] = '\n';
                    if (ReadFile(hand, lookahead, 1+utf16, &len, NULL) && len)
                    {
                        if(lookahead[0]=='\n' && (!utf16 || lookahead[1]==0) && j==0)
                        {
                            bufstart[j++] = '\n';
                            if(utf16) bufstart[j++] = 0;
                        }
                        else
                        {
                            if(lookahead[0]!='\n' || (utf16 && lookahead[1]!=0))
                            {
                                bufstart[j++] = '\r';
                                if(utf16) bufstart[j++] = 0;
                            }

                            if (fdinfo->wxflag & (WX_PIPE | WX_NOSEEK))
                            {
                                if (lookahead[0]=='\n' && (!utf16 || !lookahead[1]))
                                {
                                    bufstart[j++] = '\n';
                                    if (utf16) bufstart[j++] = 0;
                                }
                                else
                                {
                                    fdinfo->lookahead[0] = lookahead[0];
                                    fdinfo->lookahead[1] = lookahead[1];
                                }
                            }
                            else
                                SetFilePointer(fdinfo->handle, -1-utf16, NULL, FILE_CURRENT);
                        }
                    }
                    else
                    {
                        bufstart[j++] = '\r';
                        if(utf16) bufstart[j++] = 0;
                    }
                }
                else if((bufstart[i]!='\r' || (utf16 && bufstart[i+1]!=0))
                        || (bufstart[i+1+utf16]!='\n' || (utf16 && bufstart[i+3]!=0)))
                {
                    bufstart[j++] = bufstart[i];
                    if(utf16) bufstart[j++] = bufstart[i+1];
                }
            }
            num_read = j;
        }
    }
    else
    {
        if (GetLastError() == ERROR_BROKEN_PIPE)
        {
            TRACE(":end-of-pipe\n");
            fdinfo->wxflag |= WX_ATEOF;
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
    if(ret==_O_TEXT && (get_ioinfo(fd)->exflag & (EF_UTF8|EF_UTF16)))
        ret = _O_WTEXT;

    if(mode!=_O_TEXT && mode!=_O_BINARY && mode!=_O_WTEXT
                && mode!=_O_U16TEXT && mode!=_O_U8TEXT) {
        *_errno() = EINVAL;
        return -1;
    }

    if(mode == _O_BINARY) {
        get_ioinfo(fd)->wxflag &= ~WX_TEXT;
        get_ioinfo(fd)->exflag &= ~(EF_UTF8|EF_UTF16);
        return ret;
    }

    get_ioinfo(fd)->wxflag |= WX_TEXT;
    if(mode == _O_TEXT)
        get_ioinfo(fd)->exflag &= ~(EF_UTF8|EF_UTF16);
    else if(mode == _O_U8TEXT)
        get_ioinfo(fd)->exflag = (get_ioinfo(fd)->exflag & ~EF_UTF16) | EF_UTF8;
    else
        get_ioinfo(fd)->exflag = (get_ioinfo(fd)->exflag & ~EF_UTF8) | EF_UTF16;

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
    ioinfo *info = get_ioinfo(fd);
    HANDLE hand = info->handle;

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

    if (((info->exflag&EF_UTF8) || (info->exflag&EF_UTF16)) && count&1)
    {
        *_errno() = EINVAL;
        return -1;
    }

    /* If appending, go to EOF */
    if (info->wxflag & WX_APPEND)
        _lseek(fd, 0, FILE_END);

    if (!(info->wxflag & WX_TEXT))
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
        unsigned int i, j, nr_lf, size;
        char *p = NULL;
        const char *q;
        const char *s = buf, *buf_start = buf;

        if (!(info->exflag & (EF_UTF8|EF_UTF16)))
        {
            /* find number of \n */
            for (nr_lf=0, i=0; i<count; i++)
                if (s[i] == '\n')
                    nr_lf++;
            if (nr_lf)
            {
                size = count+nr_lf;
                if ((q = p = malloc(size)))
                {
                    for (s = buf, i = 0, j = 0; i < count; i++)
                    {
                        if (s[i] == '\n')
                            p[j++] = '\r';
                        p[j++] = s[i];
                    }
                }
                else
                {
                    FIXME("Malloc failed\n");
                    nr_lf = 0;
                    size = count;
                    q = buf;
                }
            }
            else
            {
                size = count;
                q = buf;
            }
        }
        else if (info->exflag & EF_UTF16)
        {
            for (nr_lf=0, i=0; i<count; i+=2)
                if (s[i]=='\n' && s[i+1]==0)
                    nr_lf += 2;
            if (nr_lf)
            {
                size = count+nr_lf;
                if ((q = p = malloc(size)))
                {
                    for (s=buf, i=0, j=0; i<count; i++)
                    {
                        if (s[i]=='\n' && s[i+1]==0)
                        {
                            p[j++] = '\r';
                            p[j++] = 0;
                        }
                        p[j++] = s[i++];
                        p[j++] = s[i];
                    }
                }
                else
                {
                    FIXME("Malloc failed\n");
                    nr_lf = 0;
                    size = count;
                    q = buf;
                }
            }
            else
            {
                size = count;
                q = buf;
            }
        }
        else
        {
            DWORD conv_len;

            for(nr_lf=0, i=0; i<count; i+=2)
                if (s[i]=='\n' && s[i+1]==0)
                    nr_lf++;

            conv_len = WideCharToMultiByte(CP_UTF8, 0, (WCHAR*)buf, count/2, NULL, 0, NULL, NULL);
            if(!conv_len) {
                _dosmaperr(GetLastError());
                free(p);
                return -1;
            }

            size = conv_len+nr_lf;
            if((p = malloc(count+nr_lf*2+size)))
            {
                for (s=buf, i=0, j=0; i<count; i++)
                {
                    if (s[i]=='\n' && s[i+1]==0)
                    {
                        p[j++] = '\r';
                        p[j++] = 0;
                    }
                    p[j++] = s[i++];
                    p[j++] = s[i];
                }
                q = p+count+nr_lf*2;
                WideCharToMultiByte(CP_UTF8, 0, (WCHAR*)p, count/2+nr_lf,
                        p+count+nr_lf*2, conv_len+nr_lf, NULL, NULL);
            }
            else
            {
                FIXME("Malloc failed\n");
                nr_lf = 0;
                size = count;
                q = buf;
            }
        }

        if (!WriteFile(hand, q, size, &num_written, NULL))
            num_written = -1;
        if(p)
            free(p);
        if (num_written != size)
        {
            TRACE("WriteFile (fd %d, hand %p) failed-last error (%d), num_written %d\n",
                    fd, hand, GetLastError(), num_written);
            *_errno() = ENOSPC;
            return s - buf_start;
        }
        return count;
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
    return file->_flag & _IOEOF;
}

/*********************************************************************
 *		ferror (MSVCRT.@)
 */
int CDECL ferror(FILE* file)
{
    return file->_flag & _IOERR;
}

/*********************************************************************
 *		_filbuf (MSVCRT.@)
 */
int CDECL _filbuf(FILE* file)
{
    unsigned char c;
    _lock_file(file);

    if(file->_flag & _IOSTRG) {
        _unlock_file(file);
        return EOF;
    }

    /* Allocate buffer if needed */
    if(!(file->_flag & (_IONBF | _IOMYBUF | _USERBUF)))
        alloc_buffer(file);

    if(!(file->_flag & _IOREAD)) {
        if(file->_flag & _IORW)
            file->_flag |= _IOREAD;
        else {
            _unlock_file(file);
            return EOF;
        }
    }

    if(!(file->_flag & (_IOMYBUF | _USERBUF))) {
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
 */
wint_t CDECL fgetwc(FILE* file)
{
    wint_t ret;
    int ch;

    _lock_file(file);

    if((get_ioinfo(file->_file)->exflag & (EF_UTF8 | EF_UTF16))
            || !(get_ioinfo(file->_file)->wxflag & WX_TEXT)) {
        char *p;

        for(p=(char*)&ret; (wint_t*)p<&ret+1; p++) {
            ch = fgetc(file);
            if(ch == EOF) {
                ret = WEOF;
                break;
            }
            *p = (char)ch;
        }
    }else {
        char mbs[MB_LEN_MAX];
        int len = 0;

        ch = fgetc(file);
        if(ch != EOF) {
            mbs[0] = (char)ch;
            if(isleadbyte((unsigned char)mbs[0])) {
                ch = fgetc(file);
                if(ch != EOF) {
                    mbs[1] = (char)ch;
                    len = 2;
                }
            }else {
                len = 1;
            }
        }

        if(!len || mbtowc(&ret, mbs, len)==-1)
            ret = WEOF;
    }

    _unlock_file(file);
    return ret;
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

    while(wrcnt) {
#ifndef __REACTOS__
        if(file->_cnt < 0) {
            WARN("negative file->_cnt value in %p\n", file);
            file->_flag |= MSVCRT__IOERR;
            break;
        } else
#endif
        if(file->_cnt) {
            int pcnt=(file->_cnt>wrcnt)? wrcnt: file->_cnt;
            memcpy(file->_ptr, ptr, pcnt);
            file->_cnt -= pcnt;
            file->_ptr += pcnt;
            written += pcnt;
            wrcnt -= pcnt;
            ptr = (const char*)ptr + pcnt;
        } else if((file->_flag & _IONBF)
                || ((file->_flag & (_IOMYBUF | _USERBUF)) && wrcnt >= file->_bufsiz)
                || (!(file->_flag & (_IOMYBUF | _USERBUF)) && wrcnt >= MSVCRT_INTERNAL_BUFSIZ)) {
            size_t pcnt;
            int bufsiz;

            if(file->_flag & _IONBF)
                bufsiz = 1;
            else if(!(file->_flag & (_IOMYBUF | _USERBUF)))
                bufsiz = MSVCRT_INTERNAL_BUFSIZ;
            else
                bufsiz = file->_bufsiz;

            pcnt = (wrcnt / bufsiz) * bufsiz;

            if(flush_buffer(file) == EOF)
                break;

            if(_write(file->_file, ptr, pcnt) <= 0) {
                file->_flag |= _IOERR;
                break;
            }
            written += pcnt;
            wrcnt -= pcnt;
            ptr = (const char*)ptr + pcnt;
        } else {
            if(_flsbuf(*(const char*)ptr, file) == EOF)
                break;
            written++;
            wrcnt--;
            ptr = (const char*)ptr + 1;
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
    if (!MSVCRT_CHECK_PMT(pFile != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(filename != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(mode != NULL)) return EINVAL;

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
  size_t pread=0;

  if(!rcnt)
	return 0;

  _lock_file(file);

  /* first buffered data */
  if(file->_cnt>0) {
	int pcnt= (rcnt>file->_cnt)? file->_cnt:rcnt;
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

  if(rcnt>0 && !(file->_flag & (_IONBF | _IOMYBUF | _USERBUF)))
      alloc_buffer(file);

  while(rcnt>0)
  {
    int i;
    if (!file->_cnt && rcnt<BUFSIZ && (file->_flag & (_IOMYBUF | _USERBUF))) {
      file->_cnt = _read(file->_file, file->_base, file->_bufsiz);
      file->_ptr = file->_base;
      i = (file->_cnt<rcnt) ? file->_cnt : rcnt;
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
    } else if (rcnt > INT_MAX) {
      i = _read(file->_file, ptr, INT_MAX);
    } else if (rcnt < BUFSIZ) {
      i = _read(file->_file, ptr, rcnt);
    } else {
      i = _read(file->_file, ptr, rcnt - BUFSIZ/2);
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

  TRACE(":path (%p) mode (%s) file (%p) fd (%d)\n", debugstr_w(path), debugstr_w(mode), file, file ? file->_file : -1);

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
    __int64 pos;

    _lock_file(file);
    pos = _telli64(file->_file);
    if(pos == -1) {
        _unlock_file(file);
        return -1;
    }
    if(file->_flag & (_IOMYBUF | _USERBUF))  {
        if(file->_flag & _IOWRT) {
            pos += file->_ptr - file->_base;

            if(get_ioinfo(file->_file)->wxflag & WX_TEXT) {
                char *p;

                for(p=file->_base; p<file->_ptr; p++)
                    if(*p == '\n')
                        pos++;
            }
        } else if(!file->_cnt) { /* nothing to do */
        } else if(_lseeki64(file->_file, 0, SEEK_END)==pos) {
            int i;

            pos -= file->_cnt;
            if(get_ioinfo(file->_file)->wxflag & WX_TEXT) {
                for(i=0; i<file->_cnt; i++)
                    if(file->_ptr[i] == '\n')
                        pos--;
            }
        } else {
            char *p;

            if(_lseeki64(file->_file, pos, SEEK_SET) != pos) {
                _unlock_file(file);
                return -1;
            }

            pos -= file->_bufsiz;
            pos += file->_ptr - file->_base;

            if(get_ioinfo(file->_file)->wxflag & WX_TEXT) {
                if(get_ioinfo(file->_file)->wxflag & WX_READNL)
                    pos--;

                for(p=file->_base; p<file->_ptr; p++)
                    if(*p == '\n')
                        pos++;
            }
        }
    }

    _unlock_file(file);
    return pos;
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
    *pos = _ftelli64(file);
    if(*pos == -1)
        return -1;
    return 0;
}

/*********************************************************************
 *		fputs (MSVCRT.@)
 */
int CDECL fputs(const char *s, FILE* file)
{
    size_t len = strlen(s);
    int ret;

    _lock_file(file);
    ret = fwrite(s, sizeof(*s), len, file) == len ? 0 : EOF;
    _unlock_file(file);
    return ret;
}

/*********************************************************************
 *		fputws (MSVCRT.@)
 */
int CDECL fputws(const wchar_t *s, FILE* file)
{
    size_t i, len = strlenW(s);
    BOOL tmp_buf;
    int ret;

    _lock_file(file);
    if (!(get_ioinfo(file->_file)->wxflag & WX_TEXT)) {
        ret = fwrite(s,sizeof(*s),len,file) == len ? 0 : EOF;
        _unlock_file(file);
        return ret;
    }

    tmp_buf = add_std_buffer(file);
    for (i=0; i<len; i++) {
        if(fputwc(s[i], file) == WEOF) {
            if(tmp_buf) remove_std_buffer(file);
            _unlock_file(file);
            return WEOF;
        }
    }

    if(tmp_buf) remove_std_buffer(file);
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
    if(!MSVCRT_CHECK_PMT(file != NULL)) return -1;
    if(!MSVCRT_CHECK_PMT(mode==_IONBF || mode==_IOFBF || mode==_IOLBF)) return -1;
    if(!MSVCRT_CHECK_PMT(mode==_IONBF || (size>=2 && size<=INT_MAX))) return -1;

    _lock_file(file);

    fflush(file);
    if(file->_flag & _IOMYBUF)
        free(file->_base);
    file->_flag &= ~(_IONBF | _IOMYBUF | _USERBUF);
    file->_cnt = 0;

    if(mode == _IONBF) {
        file->_flag |= _IONBF;
        file->_base = file->_ptr = (char*)&file->_charbuf;
        file->_bufsiz = 2;
    }else if(buf) {
        file->_base = file->_ptr = buf;
        file->_flag |= _USERBUF;
        file->_bufsiz = size;
    }else {
        file->_base = file->_ptr = malloc(size);
        if(!file->_base) {
            file->_bufsiz = 0;
            _unlock_file(file);
            return -1;
        }

        file->_flag |= _IOMYBUF;
        file->_bufsiz = size;
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
    p[size] = '\0';
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
        p[size] = '\0';
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
  fd = _open(filename, _O_CREAT | _O_BINARY | _O_RDWR | _O_TEMPORARY,
          _S_IREAD | _S_IWRITE);
  if (fd != -1 && (file = alloc_fp()))
  {
    if (init_fp(file, fd, _IORW) == -1)
    {
        file->_flag = 0;
        file = NULL;
    }
    else file->_tmpfname = _strdup(filename);
  }

  if(fd != -1 && !file)
      _close(fd);
  UNLOCK_FILES();
  return file;
}

/*********************************************************************
 *		ungetc (MSVCRT.@)
 */
int CDECL ungetc(int c, FILE * file)
{
    if(!MSVCRT_CHECK_PMT(file != NULL)) return EOF;

    if (c == EOF || !(file->_flag&_IOREAD ||
                (file->_flag&_IORW && !(file->_flag&_IOWRT))))
        return EOF;

    _lock_file(file);
    if((!(file->_flag & (_IONBF | _IOMYBUF | _USERBUF))
                && alloc_buffer(file))
            || (!file->_cnt && file->_ptr==file->_base))
        file->_ptr++;

    if(file->_ptr>file->_base) {
        file->_ptr--;
        if(file->_flag & _IOSTRG) {
            if(*file->_ptr != c) {
                file->_ptr++;
                _unlock_file(file);
                return EOF;
            }
        }else {
            *file->_ptr = c;
        }
        file->_cnt++;
        clearerr(file);
        file->_flag |= _IOREAD;
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

    if (wc == WEOF)
        return WEOF;

    _lock_file(file);

    if((get_ioinfo(file->_file)->exflag & (EF_UTF8 | EF_UTF16))
            || !(get_ioinfo(file->_file)->wxflag & WX_TEXT)) {
        unsigned char * pp = (unsigned char *)&mwc;
        int i;

        for(i=sizeof(wchar_t)-1;i>=0;i--) {
            if(pp[i] != ungetc(pp[i],file)) {
                _unlock_file(file);
                return WEOF;
            }
        }
    }else {
        char mbs[MB_LEN_MAX];
        int len;

        len = wctomb(mbs, mwc);
        if(len == -1) {
            _unlock_file(file);
            return WEOF;
        }

        for(len--; len>=0; len--) {
            if(mbs[len] != ungetc(mbs[len], file)) {
                _unlock_file(file);
                return WEOF;
            }
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
