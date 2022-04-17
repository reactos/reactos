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
#include "internal/wine/msvcrt.h"

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
#define WX_TTY            0x40
#define WX_TEXT           0x80

/* values for exflag - it's used differently in msvcr90.dll*/
#define EF_UTF8           0x01
#define EF_UTF16          0x02
#define EF_CRIT_INIT      0x04
#define EF_UNK_UNICODE    0x08

static char utf8_bom[3] = { 0xef, 0xbb, 0xbf };
static char utf16_bom[2] = { 0xff, 0xfe };

/* FIXME: this should be allocated dynamically */
#define MSVCRT_MAX_FILES 2048
#define MSVCRT_FD_BLOCK_SIZE 32

#define MSVCRT_INTERNAL_BUFSIZ 4096

/*********************************************************************
 *		__pioinfo (MSVCRT.@)
 * array of pointers to ioinfo arrays [32]
 */
ioinfo * __pioinfo[MSVCRT_MAX_FILES/MSVCRT_FD_BLOCK_SIZE] = { 0 };

/*********************************************************************
 *		__badioinfo (MSVCRT.@)
 */
ioinfo __badioinfo = { INVALID_HANDLE_VALUE, WX_TEXT };

typedef struct {
    FILE file;
    CRITICAL_SECTION crit;
} file_crit;

FILE _iob[_IOB_ENTRIES] = { { 0 } };
static file_crit* MSVCRT_fstream[MSVCRT_MAX_FILES/MSVCRT_FD_BLOCK_SIZE] = { NULL };
static int MSVCRT_max_streams = 512, MSVCRT_stream_idx;

/* INTERNAL: process umask */
static int MSVCRT_umask = 0;

/* INTERNAL: static data for tmpnam and _wtmpname functions */
static int tmpnam_unique;

/* This critical section protects the MSVCRT_fstreams table
 * and MSVCRT_stream_idx from race conditions. It also
 * protects fd critical sections creation code.
 */
static CRITICAL_SECTION MSVCRT_file_cs;
static CRITICAL_SECTION_DEBUG MSVCRT_file_cs_debug =
{
    0, 0, &MSVCRT_file_cs,
    { &MSVCRT_file_cs_debug.ProcessLocksList, &MSVCRT_file_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": MSVCRT_file_cs") }
};
static CRITICAL_SECTION MSVCRT_file_cs = { &MSVCRT_file_cs_debug, -1, 0, 0, 0, 0 };
#define LOCK_FILES()    do { EnterCriticalSection(&MSVCRT_file_cs); } while (0)
#define UNLOCK_FILES()  do { LeaveCriticalSection(&MSVCRT_file_cs); } while (0)

static inline ioinfo* get_ioinfo_nolock(int fd)
{
    ioinfo *ret = NULL;
    if(fd>=0 && fd<MSVCRT_MAX_FILES)
        ret = __pioinfo[fd/MSVCRT_FD_BLOCK_SIZE];
    if(!ret)
        return &__badioinfo;

    return ret + (fd%MSVCRT_FD_BLOCK_SIZE);
}

static inline void init_ioinfo_cs(ioinfo *info)
{
    if(!(info->exflag & EF_CRIT_INIT)) {
        LOCK_FILES();
        if(!(info->exflag & EF_CRIT_INIT)) {
            InitializeCriticalSection(&info->crit);
            info->exflag |= EF_CRIT_INIT;
        }
        UNLOCK_FILES();
    }
}

ioinfo* get_ioinfo(int fd)
{
    ioinfo *ret = get_ioinfo_nolock(fd);
    if(ret == &__badioinfo)
        return ret;
    init_ioinfo_cs(ret);
    EnterCriticalSection(&ret->crit);
    return ret;
}

static inline BOOL alloc_pioinfo_block(int fd)
{
    ioinfo *block;
    int i;

    if(fd<0 || fd>=MSVCRT_MAX_FILES)
    {
        *_errno() = ENFILE;
        return FALSE;
    }

    block = calloc(MSVCRT_FD_BLOCK_SIZE, sizeof(ioinfo));
    if(!block)
    {
        WARN(":out of memory!\n");
        *_errno() = ENOMEM;
        return FALSE;
    }
    for(i=0; i<MSVCRT_FD_BLOCK_SIZE; i++)
        block[i].handle = INVALID_HANDLE_VALUE;
    if(InterlockedCompareExchangePointer((void**)&__pioinfo[fd/MSVCRT_FD_BLOCK_SIZE], block, NULL))
        free(block);
    return TRUE;
}

static inline ioinfo* get_ioinfo_alloc_fd(int fd)
{
    ioinfo *ret;

    ret = get_ioinfo(fd);
    if(ret != &__badioinfo)
        return ret;

    if(!alloc_pioinfo_block(fd))
        return &__badioinfo;

    return get_ioinfo(fd);
}

static inline ioinfo* get_ioinfo_alloc(int *fd)
{
    int i;

    *fd = -1;
    for(i=0; i<MSVCRT_MAX_FILES; i++)
    {
        ioinfo *info = get_ioinfo_nolock(i);

        if(info == &__badioinfo)
        {
            if(!alloc_pioinfo_block(i))
                return &__badioinfo;
            info = get_ioinfo_nolock(i);
        }

        init_ioinfo_cs(info);
        if(TryEnterCriticalSection(&info->crit))
        {
            if(info->handle == INVALID_HANDLE_VALUE)
            {
                *fd = i;
                return info;
            }
            LeaveCriticalSection(&info->crit);
        }
    }

    WARN(":files exhausted!\n");
    *_errno() = ENFILE;
    return &__badioinfo;
}

void release_ioinfo(ioinfo *info)
{
    if(info!=&__badioinfo && info->exflag & EF_CRIT_INIT)
        LeaveCriticalSection(&info->crit);
}

static inline FILE* msvcrt_get_file(int i)
{
    file_crit *ret;

    if(i >= MSVCRT_max_streams)
        return NULL;

    if(i < _IOB_ENTRIES)
        return &_iob[i];

    ret = MSVCRT_fstream[i/MSVCRT_FD_BLOCK_SIZE];
    if(!ret) {
        MSVCRT_fstream[i/MSVCRT_FD_BLOCK_SIZE] = calloc(MSVCRT_FD_BLOCK_SIZE, sizeof(file_crit));
        if(!MSVCRT_fstream[i/MSVCRT_FD_BLOCK_SIZE]) {
            ERR("out of memory\n");
            *_errno() = ENOMEM;
            return NULL;
        }

        ret = MSVCRT_fstream[i/MSVCRT_FD_BLOCK_SIZE] + (i%MSVCRT_FD_BLOCK_SIZE);
    } else
        ret += i%MSVCRT_FD_BLOCK_SIZE;

    return &ret->file;
}

/* INTERNAL: free a file entry fd */
static void msvcrt_free_fd(int fd)
{
  ioinfo *fdinfo = get_ioinfo(fd);

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
  release_ioinfo(fdinfo);
}

static void msvcrt_set_fd(ioinfo *fdinfo, HANDLE hand, int flag)
{
  fdinfo->handle = hand;
  fdinfo->wxflag = WX_OPEN | (flag & (WX_DONTINHERIT | WX_APPEND | WX_TEXT | WX_PIPE | WX_TTY));
  fdinfo->lookahead[0] = '\n';
  fdinfo->lookahead[1] = '\n';
  fdinfo->lookahead[2] = '\n';
  fdinfo->exflag &= EF_CRIT_INIT;

  switch (fdinfo-__pioinfo[0])
  {
  case 0: SetStdHandle(STD_INPUT_HANDLE,  hand); break;
  case 1: SetStdHandle(STD_OUTPUT_HANDLE, hand); break;
  case 2: SetStdHandle(STD_ERROR_HANDLE,  hand); break;
  }
}

/* INTERNAL: Allocate an fd slot from a Win32 HANDLE */
/*static*/ int msvcrt_alloc_fd(HANDLE hand, int flag)
{
    int fd;
    ioinfo *info = get_ioinfo_alloc(&fd);

    TRACE(":handle (%p) allocating fd (%d)\n", hand, fd);

    if(info == &__badioinfo)
        return -1;

    msvcrt_set_fd(info, hand, flag);
    release_ioinfo(info);
    return fd;
}

/* INTERNAL: Allocate a FILE* for an fd slot */
/* caller must hold the files lock */
static FILE* msvcrt_alloc_fp(void)
{
  int i;
  FILE *file;

  for (i = 3; i < MSVCRT_max_streams; i++)
  {
    file = msvcrt_get_file(i);
    if (!file)
      return NULL;

    if (file->_flag == 0)
    {
      if (i == MSVCRT_stream_idx) MSVCRT_stream_idx++;
      return file;
    }
  }

  return NULL;
}

/* INTERNAL: initialize a FILE* from an open fd */
static int msvcrt_init_fp(FILE* file, int fd, unsigned stream_flags)
{
  TRACE(":fd (%d) allocating FILE*\n",fd);
  if (!(get_ioinfo_nolock(fd)->wxflag & WX_OPEN))
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
  int         fd, last_fd;
  char*       wxflag_ptr;
  HANDLE*     handle_ptr;
  ioinfo*     fdinfo;

  for (last_fd=MSVCRT_MAX_FILES-1; last_fd>=0; last_fd--)
    if (get_ioinfo_nolock(last_fd)->handle != INVALID_HANDLE_VALUE)
      break;
  last_fd++;

  *size = sizeof(unsigned) + (sizeof(char) + sizeof(HANDLE)) * last_fd;
  *block = calloc(1, *size);
  if (!*block)
  {
    *size = 0;
    return FALSE;
  }
  wxflag_ptr = (char*)*block + sizeof(unsigned);
  handle_ptr = (HANDLE*)(wxflag_ptr + last_fd);

  *(unsigned*)*block = last_fd;
  for (fd = 0; fd < last_fd; fd++)
  {
    /* to be inherited, we need it to be open, and that DONTINHERIT isn't set */
    fdinfo = get_ioinfo_nolock(fd);
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
    count = min(count, MSVCRT_MAX_FILES);
    for (i = 0; i < count; i++)
    {
      if ((*wxflag_ptr & WX_OPEN) && GetFileType(*handle_ptr) != FILE_TYPE_UNKNOWN)
      {
        fdinfo = get_ioinfo_alloc_fd(i);
        if(fdinfo != &__badioinfo)
            msvcrt_set_fd(fdinfo, *handle_ptr, *wxflag_ptr);
        release_ioinfo(fdinfo);
      }

      wxflag_ptr++; handle_ptr++;
    }
  }

  fdinfo = get_ioinfo_alloc_fd(STDIN_FILENO);
  if (!(fdinfo->wxflag & WX_OPEN) || fdinfo->handle == INVALID_HANDLE_VALUE) {
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    DWORD type = GetFileType(h);

    msvcrt_set_fd(fdinfo, h, WX_OPEN|WX_TEXT|((type&0xf)==FILE_TYPE_CHAR ? WX_TTY : 0)
            |((type&0xf)==FILE_TYPE_PIPE ? WX_PIPE : 0));
  }
  release_ioinfo(fdinfo);

  fdinfo = get_ioinfo_alloc_fd(STDOUT_FILENO);
  if (!(fdinfo->wxflag & WX_OPEN) || fdinfo->handle == INVALID_HANDLE_VALUE) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD type = GetFileType(h);

    msvcrt_set_fd(fdinfo, h, WX_OPEN|WX_TEXT|((type&0xf)==FILE_TYPE_CHAR ? WX_TTY : 0)
            |((type&0xf)==FILE_TYPE_PIPE ? WX_PIPE : 0));
  }
  release_ioinfo(fdinfo);

  fdinfo = get_ioinfo_alloc_fd(STDERR_FILENO);
  if (!(fdinfo->wxflag & WX_OPEN) || fdinfo->handle == INVALID_HANDLE_VALUE) {
    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    DWORD type = GetFileType(h);

    msvcrt_set_fd(fdinfo, h, WX_OPEN|WX_TEXT|((type&0xf)==FILE_TYPE_CHAR ? WX_TTY : 0)
            |((type&0xf)==FILE_TYPE_PIPE ? WX_PIPE : 0));
  }
  release_ioinfo(fdinfo);

  TRACE(":handles (%p)(%p)(%p)\n", get_ioinfo_nolock(STDIN_FILENO)->handle,
        get_ioinfo_nolock(STDOUT_FILENO)->handle,
        get_ioinfo_nolock(STDERR_FILENO)->handle);

  memset(_iob,0,3*sizeof(FILE));
  for (i = 0; i < 3; i++)
  {
    /* FILE structs for stdin/out/err are static and never deleted */
    _iob[i]._file = i;
    _iob[i]._tmpfname = NULL;
    _iob[i]._flag = (i == 0) ? _IOREAD : _IOWRT;
  }
  MSVCRT_stream_idx = 3;
}

/* INTERNAL: Flush stdio file buffer */
static int msvcrt_flush_buffer(FILE* file)
{
    if((file->_flag & (_IOREAD|_IOWRT)) == _IOWRT &&
            file->_flag & (_IOMYBUF|_USERBUF)) {
        int cnt=file->_ptr-file->_base;
        if(cnt>0 && _write(file->_file, file->_base, cnt) != cnt) {
            file->_flag |= _IOERR;
            return EOF;
        }

        if(file->_flag & _IORW)
            file->_flag &= ~_IOWRT;

#ifdef __REACTOS__ /* CORE-11949 */
    file->_ptr=file->_base;
    file->_cnt=0;
#endif
    }

#ifndef __REACTOS__ /* CORE-11949 */
    file->_ptr=file->_base;
    file->_cnt=0;
#endif
    return 0;
}

/*********************************************************************
 *		_isatty (MSVCRT.@)
 */
int CDECL _isatty(int fd)
{
    TRACE(":fd (%d)\n",fd);

    return get_ioinfo_nolock(fd)->wxflag & WX_TTY;
}

/* INTERNAL: Allocate stdio file buffer */
/*static*/ BOOL msvcrt_alloc_buffer(FILE* file)
{
    if((file->_file==STDOUT_FILENO || file->_file==STDERR_FILENO)
            && _isatty(file->_file))
        return FALSE;

    file->_base = calloc(1, MSVCRT_INTERNAL_BUFSIZ);
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
            || (file->_flag & (_IONBF | _IOMYBUF | _USERBUF))
            || !_isatty(file->_file))
        return FALSE;

    file->_ptr = file->_base = buffers[file->_file == STDOUT_FILENO ? 0 : 1];
    file->_bufsiz = file->_cnt = BUFSIZ;
    file->_flag |= _USERBUF;
    return TRUE;
}

/* INTERNAL: Removes temporary buffer from stdout or stderr */
/* Only call this function when add_std_buffer returned TRUE */
static void remove_std_buffer(FILE *file)
{
    msvcrt_flush_buffer(file);
    file->_ptr = file->_base = NULL;
    file->_bufsiz = file->_cnt = 0;
    file->_flag &= ~_USERBUF;
}

/* INTERNAL: Convert integer to base32 string (0-9a-v), 0 becomes "" */
static int msvcrt_int_to_base32(int num, char *str)
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

/* INTERNAL: wide character version of msvcrt_int_to_base32 */
static int msvcrt_int_to_base32_w(int num, wchar_t *str)
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

/*********************************************************************
 *		_commit (MSVCRT.@)
 */
int CDECL _commit(int fd)
{
    ioinfo *info = get_ioinfo(fd);
    int ret;

    TRACE(":fd (%d) handle (%p)\n", fd, info->handle);

    if (info->handle == INVALID_HANDLE_VALUE)
        ret = -1;
    else if (!FlushFileBuffers(info->handle))
    {
        if (GetLastError() == ERROR_INVALID_HANDLE)
        {
            /* FlushFileBuffers fails for console handles
             * so we ignore this error.
             */
            ret = 0;
        }
        else
        {
            TRACE(":failed-last error (%d)\n",GetLastError());
            _dosmaperr(GetLastError());
            ret = -1;
        }
    }
    else
    {
        TRACE(":ok\n");
        ret = 0;
    }

    release_ioinfo(info);
    return ret;
}

/* _flushall calls fflush which calls _flushall */
int CDECL fflush(FILE* file);

/* INTERNAL: Flush all stream buffer */
static int msvcrt_flush_all_buffers(int mask)
{
  int i, num_flushed = 0;
  FILE *file;

  LOCK_FILES();
  for (i = 0; i < MSVCRT_stream_idx; i++) {
    file = msvcrt_get_file(i);

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
    return msvcrt_flush_all_buffers(_IOWRT | _IOREAD);
}

/*********************************************************************
 *		fflush (MSVCRT.@)
 */
int CDECL fflush(FILE* file)
{
    if(!file) {
        msvcrt_flush_all_buffers(_IOWRT);
    } else if(file->_flag & _IOWRT) {
        int res;

        _lock_file(file);
        res = msvcrt_flush_buffer(file);
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
  ioinfo *info = get_ioinfo(fd);
  int ret;

  TRACE(":fd (%d) handle (%p)\n", fd, info->handle);
  if (!(info->wxflag & WX_OPEN)) {
    ret = -1;
  } else if (fd == STDOUT_FILENO &&
          info->handle == get_ioinfo_nolock(STDERR_FILENO)->handle) {
    msvcrt_free_fd(fd);
    ret = 0;
  } else if (fd == STDERR_FILENO &&
          info->handle == get_ioinfo_nolock(STDOUT_FILENO)->handle) {
    msvcrt_free_fd(fd);
    ret = 0;
  } else {
    ret = CloseHandle(info->handle) ? 0 : -1;
    msvcrt_free_fd(fd);
    if (ret) {
      WARN(":failed-last error (%d)\n",GetLastError());
      _dosmaperr(GetLastError());
      ret = -1;
    }
  }
  release_ioinfo(info);
  return ret;
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
  ioinfo *info_od, *info_nd;
  int ret;

  TRACE("(od=%d, nd=%d)\n", od, nd);

  if (od < nd)
  {
    info_od = get_ioinfo(od);
    info_nd = get_ioinfo_alloc_fd(nd);
  }
  else
  {
    info_nd = get_ioinfo_alloc_fd(nd);
    info_od = get_ioinfo(od);
  }

  if (info_nd == &__badioinfo)
  {
      ret = -1;
  }
  else if (info_od->wxflag & WX_OPEN)
  {
    HANDLE handle;

    if (DuplicateHandle(GetCurrentProcess(), info_od->handle,
     GetCurrentProcess(), &handle, 0, TRUE, DUPLICATE_SAME_ACCESS))
    {
      int wxflag = info_od->wxflag & ~WX_DONTINHERIT;

      if (info_nd->wxflag & WX_OPEN)
        _close(nd);

      msvcrt_set_fd(info_nd, handle, wxflag);
      /* _dup2 returns 0, not nd, on success */
      ret = 0;
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

  release_ioinfo(info_od);
  release_ioinfo(info_nd);
  return ret;
}

/*********************************************************************
 *		_dup (MSVCRT.@)
 */
int CDECL _dup(int od)
{
  int fd, ret;
  ioinfo *info = get_ioinfo_alloc(&fd);

  if (_dup2(od, fd) == 0)
    ret = fd;
  else
    ret = -1;
  release_ioinfo(info);
  return ret;
}

/*********************************************************************
 *		_eof (MSVCRT.@)
 */
int CDECL _eof(int fd)
{
  ioinfo *info = get_ioinfo(fd);
  DWORD curpos,endpos;
  LONG hcurpos,hendpos;

  TRACE(":fd (%d) handle (%p)\n", fd, info->handle);

  if (info->handle == INVALID_HANDLE_VALUE)
  {
    release_ioinfo(info);
    return -1;
  }

  if (info->wxflag & WX_ATEOF)
  {
      release_ioinfo(info);
      return TRUE;
  }

  /* Otherwise we do it the hard way */
  hcurpos = hendpos = 0;
  curpos = SetFilePointer(info->handle, 0, &hcurpos, FILE_CURRENT);
  endpos = SetFilePointer(info->handle, 0, &hendpos, FILE_END);

  if (curpos == endpos && hcurpos == hendpos)
  {
    /* FIXME: shouldn't WX_ATEOF be set here? */
    release_ioinfo(info);
    return TRUE;
  }

  SetFilePointer(info->handle, curpos, &hcurpos, FILE_BEGIN);
  release_ioinfo(info);
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
  for (i = 3; i < MSVCRT_stream_idx; i++) {
    file = msvcrt_get_file(i);

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
    {
        if(!__pioinfo[i])
            continue;

        for(j=0; j<MSVCRT_FD_BLOCK_SIZE; j++)
        {
            if(__pioinfo[i][j].exflag & EF_CRIT_INIT)
                DeleteCriticalSection(&__pioinfo[i][j].crit);
        }
        free(__pioinfo[i]);
    }

    for(j=0; j<MSVCRT_stream_idx; j++)
    {
        FILE *file = msvcrt_get_file(j);
        if(file<_iob || file>=_iob+_IOB_ENTRIES)
        {
            ((file_crit*)file)->crit.DebugInfo->Spare[0] = 0;
            DeleteCriticalSection(&((file_crit*)file)->crit);
        }
    }

    for(i=0; i<sizeof(MSVCRT_fstream)/sizeof(MSVCRT_fstream[0]); i++)
        free(MSVCRT_fstream[i]);
}

/*********************************************************************
 *		_lseeki64 (MSVCRT.@)
 */
__int64 CDECL _lseeki64(int fd, __int64 offset, int whence)
{
  ioinfo *info = get_ioinfo(fd);
  LARGE_INTEGER ofs;

  TRACE(":fd (%d) handle (%p)\n", fd, info->handle);

  if (info->handle == INVALID_HANDLE_VALUE)
  {
    release_ioinfo(info);
    return -1;
  }

  if (whence < 0 || whence > 2)
  {
    release_ioinfo(info);
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
  if ((ofs.u.LowPart = SetFilePointer(info->handle, ofs.u.LowPart, &ofs.u.HighPart, whence)) != INVALID_SET_FILE_POINTER ||
      GetLastError() == ERROR_SUCCESS)
  {
    info->wxflag &= ~(WX_ATEOF|WX_READEOF);
    /* FIXME: What if we seek _to_ EOF - is EOF set? */

    release_ioinfo(info);
    return ofs.QuadPart;
  }
  release_ioinfo(info);
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
  ioinfo *info = get_ioinfo(fd);
  BOOL ret;
  DWORD cur_locn;

  TRACE(":fd (%d) handle (%p)\n", fd, info->handle);
  if (info->handle == INVALID_HANDLE_VALUE)
  {
    release_ioinfo(info);
    return -1;
  }

  if (mode < 0 || mode > 4)
  {
    release_ioinfo(info);
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

  if ((cur_locn = SetFilePointer(info->handle, 0L, NULL, FILE_CURRENT)) == INVALID_SET_FILE_POINTER)
  {
    release_ioinfo(info);
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
      ret = LockFile(info->handle, cur_locn, 0L, nbytes, 0L);
      if (ret) break;
      Sleep(1);
    }
  }
  else if (mode == _LK_UNLCK)
    ret = UnlockFile(info->handle, cur_locn, 0L, nbytes, 0L);
  else
    ret = LockFile(info->handle, cur_locn, 0L, nbytes, 0L);
  /* FIXME - what about error settings? */
  release_ioinfo(info);
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
	msvcrt_flush_buffer(file);

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
    ioinfo *info;
    __int64 cur, pos;
    BOOL ret = FALSE;

    TRACE("(fd=%d, size=%s)\n", fd, wine_dbgstr_longlong(size));

    if (!MSVCRT_CHECK_PMT(size >= 0)) return EINVAL;

    info = get_ioinfo(fd);
    if (info->handle != INVALID_HANDLE_VALUE)
    {
        /* save the current file pointer */
        cur = _lseeki64(fd, 0, SEEK_CUR);
        if (cur >= 0)
        {
            pos = _lseeki64(fd, size, SEEK_SET);
            if (pos >= 0)
            {
                ret = SetEndOfFile(info->handle);
                if (!ret) _dosmaperr(GetLastError());
            }

            /* restore the file pointer */
            _lseeki64(fd, cur, SEEK_SET);
        }
    }

    release_ioinfo(info);
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

static int msvcrt_get_flags(const wchar_t* mode, int *open_flags, int* stream_flags)
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

  if (msvcrt_get_flags(mode, &open_flags, &stream_flags) == -1) return NULL;

  LOCK_FILES();
  if (!(file = msvcrt_alloc_fp()))
    file = NULL;
  else if (msvcrt_init_fp(file, fd, stream_flags) == -1)
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
  HANDLE hand = get_ioinfo_nolock(fd)->handle;
  TRACE(":fd (%d) handle (%p)\n",fd,hand);

  if(hand == INVALID_HANDLE_VALUE)
      *_errno() = EBADF;
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
    int wxflags = 0;
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

    fd = msvcrt_alloc_fd(readHandle, wxflags);
    if (fd != -1)
    {
      pfds[0] = fd;
      fd = msvcrt_alloc_fd(writeHandle, wxflags);
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
  DWORD sharing, type;
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

  if (oflags & (_O_WTEXT | _O_U16TEXT | _O_U8TEXT))
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

  type = GetFileType(hand);
  if (type == FILE_TYPE_CHAR)
      wxflag |= WX_TTY;
  else if (type == FILE_TYPE_PIPE)
      wxflag |= WX_PIPE;

  *fd = msvcrt_alloc_fd(hand, wxflag);
  if (*fd == -1)
      return *_errno();

  if (oflags & _O_WTEXT)
      get_ioinfo_nolock(*fd)->exflag |= EF_UTF16|EF_UNK_UNICODE;
  else if (oflags & _O_U16TEXT)
      get_ioinfo_nolock(*fd)->exflag |= EF_UTF16;
  else if (oflags & _O_U8TEXT)
      get_ioinfo_nolock(*fd)->exflag |= EF_UTF8;

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
    flags = WX_TTY;
  else if (flags == FILE_TYPE_PIPE)
    flags = WX_PIPE;
  else
    flags = 0;
  flags |= split_oflags(oflags);

  fd = msvcrt_alloc_fd((HANDLE)handle, flags);
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
  for (i = 3; i < MSVCRT_stream_idx; i++) {
    file = msvcrt_get_file(i);

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
static int read_utf8(ioinfo *fdinfo, wchar_t *buf, unsigned int count)
{
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
                if(fdinfo->wxflag & (WX_PIPE | WX_TTY))
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

    if(fdinfo->wxflag & (WX_PIPE | WX_TTY)) {
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

                if(fdinfo->wxflag & (WX_PIPE | WX_TTY))
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
static int read_i(int fd, ioinfo *fdinfo, void *buf, unsigned int count)
{
    DWORD num_read, utf16;
    char *bufstart = buf;

    if (count == 0)
        return 0;

    if (fdinfo->wxflag & WX_ATEOF) {
        TRACE("already at EOF, returning 0\n");
        return 0;
    }
    /* Don't trace small reads, it gets *very* annoying */
    if (count > 4)
        TRACE(":fd (%d) handle (%p) buf (%p) len (%d)\n", fd, fdinfo->handle, buf, count);
    if (fdinfo->handle == INVALID_HANDLE_VALUE)
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
        return read_utf8(fdinfo, buf, count);

    if (fdinfo->lookahead[0]!='\n' || ReadFile(fdinfo->handle, bufstart, count, &num_read, NULL))
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

            if(count>1+utf16 && ReadFile(fdinfo->handle, bufstart+1+utf16, count-1-utf16, &num_read, NULL))
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
                    if (ReadFile(fdinfo->handle, lookahead, 1+utf16, &len, NULL) && len)
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

                            if (fdinfo->wxflag & (WX_PIPE | WX_TTY))
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
    ioinfo *info = get_ioinfo(fd);
    int num_read = read_i(fd, info, buf, count);
    release_ioinfo(info);
    return num_read;
}

/*********************************************************************
 *		_setmode (MSVCRT.@)
 */
int CDECL _setmode(int fd,int mode)
{
    ioinfo *info = get_ioinfo(fd);
    int ret = info->wxflag & WX_TEXT ? _O_TEXT : _O_BINARY;
    if(ret==_O_TEXT && (info->exflag & (EF_UTF8|EF_UTF16)))
        ret = _O_WTEXT;

    if(mode!=_O_TEXT && mode!=_O_BINARY && mode!=_O_WTEXT
                && mode!=_O_U16TEXT && mode!=_O_U8TEXT) {
        *_errno() = EINVAL;
        release_ioinfo(info);
        return -1;
    }

    if(info == &__badioinfo) {
        *_errno() = EBADF;
        return EOF;
    }

    if(mode == _O_BINARY) {
        info->wxflag &= ~WX_TEXT;
        info->exflag &= ~(EF_UTF8|EF_UTF16);
        release_ioinfo(info);
        return ret;
    }

    info->wxflag |= WX_TEXT;
    if(mode == _O_TEXT)
        info->exflag &= ~(EF_UTF8|EF_UTF16);
    else if(mode == _O_U8TEXT)
        info->exflag = (info->exflag & ~EF_UTF16) | EF_UTF8;
    else
        info->exflag = (info->exflag & ~EF_UTF8) | EF_UTF16;

    release_ioinfo(info);
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
  int old_umask = MSVCRT_umask;
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
        release_ioinfo(info);
        return -1;
    }

    if (((info->exflag&EF_UTF8) || (info->exflag&EF_UTF16)) && count&1)
    {
        *_errno() = EINVAL;
        release_ioinfo(info);
        return -1;
    }

    /* If appending, go to EOF */
    if (info->wxflag & WX_APPEND)
        _lseek(fd, 0, FILE_END);

    if (!(info->wxflag & WX_TEXT))
    {
        if (WriteFile(hand, buf, count, &num_written, NULL)
                &&  (num_written == count))
        {
            release_ioinfo(info);
            return num_written;
        }
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
                release_ioinfo(info);
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
        release_ioinfo(info);
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

    release_ioinfo(info);
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

  if(file == msvcrt_get_file(MSVCRT_stream_idx-1)) {
    while(MSVCRT_stream_idx>3 && !file->_flag) {
      MSVCRT_stream_idx--;
      file = msvcrt_get_file(MSVCRT_stream_idx-1);
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
        msvcrt_alloc_buffer(file);

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
        if ((r = _read(file->_file,&c,1)) != 1) {
            file->_flag |= (r == 0) ? _IOEOF : _IOERR;
            _unlock_file(file);
            return EOF;
        }

        _unlock_file(file);
        return c;
    } else {
        file->_cnt = _read(file->_file, file->_base, file->_bufsiz);
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

    if((get_ioinfo_nolock(file->_file)->exflag & (EF_UTF8 | EF_UTF16))
            || !(get_ioinfo_nolock(file->_file)->wxflag & WX_TEXT)) {
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

            if(msvcrt_flush_buffer(file) == EOF)
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
    if (!(stream->_flag & _IOSTRG) && get_ioinfo_nolock(stream->_file)->wxflag & WX_TEXT)
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
  if (msvcrt_get_flags(mode, &open_flags, &stream_flags) == -1)
      return NULL;

  LOCK_FILES();
  fd = _wsopen(path, open_flags, share, _S_IREAD | _S_IWRITE);
  if (fd < 0)
    file = NULL;
  else if ((file = msvcrt_alloc_fp()) && msvcrt_init_fp(file, fd, stream_flags)
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
      res = msvcrt_flush_buffer(file);
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
      msvcrt_alloc_buffer(file);

  while(rcnt>0)
  {
    int i;
    if (!file->_cnt && rcnt<BUFSIZ && (file->_flag & (_IOMYBUF | _USERBUF))) {
      file->_cnt = _read(file->_file, file->_base, file->_bufsiz);
      file->_ptr = file->_base;
      i = (file->_cnt<rcnt) ? file->_cnt : rcnt;
      /* If the buffer fill reaches eof but fread wouldn't, clear eof. */
      if (i > 0 && i < file->_cnt) {
        get_ioinfo_nolock(file->_file)->wxflag &= ~WX_ATEOF;
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
    if (get_ioinfo_nolock(file->_file)->wxflag & WX_ATEOF)
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

  TRACE(":path (%s) mode (%s) file (%p) fd (%d)\n", debugstr_w(path), debugstr_w(mode), file, file ? file->_file : -1);

  LOCK_FILES();
  if (!file || ((fd = file->_file) < 0))
    file = NULL;
  else
  {
    fclose(file);
    /* map mode string to open() flags. "man fopen" for possibilities. */
    if (msvcrt_get_flags(mode, &open_flags, &stream_flags) == -1)
      file = NULL;
    else
    {
      fd = _wopen(path, open_flags, _S_IREAD | _S_IWRITE);
      if (fd < 0)
        file = NULL;
      else if (msvcrt_init_fp(file, fd, stream_flags) == -1)
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
	msvcrt_flush_buffer(file);

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

            if(get_ioinfo_nolock(file->_file)->wxflag & WX_TEXT) {
                char *p;

                for(p=file->_base; p<file->_ptr; p++)
                    if(*p == '\n')
                        pos++;
            }
        } else if(!file->_cnt) { /* nothing to do */
        } else if(_lseeki64(file->_file, 0, SEEK_END)==pos) {
            int i;

            pos -= file->_cnt;
            if(get_ioinfo_nolock(file->_file)->wxflag & WX_TEXT) {
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

            if(get_ioinfo_nolock(file->_file)->wxflag & WX_TEXT) {
                if(get_ioinfo_nolock(file->_file)->wxflag & WX_READNL)
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
    if (!(get_ioinfo_nolock(file->_file)->wxflag & WX_TEXT)) {
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

  msvcrt_int_to_base32(GetCurrentProcessId(), tmpstr);
  p = s + sprintf(s, "\\s%s.", tmpstr);
  for (count = 0; count < TMP_MAX; count++)
  {
    size = msvcrt_int_to_base32(tmpnam_unique++, tmpstr);
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

    msvcrt_int_to_base32_w(GetCurrentProcessId(), tmpstr);
    p = s + _snwprintf(s, MAX_PATH, format, tmpstr);
    for (count = 0; count < TMP_MAX; count++)
    {
        size = msvcrt_int_to_base32_w(tmpnam_unique++, tmpstr);
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
  if (fd != -1 && (file = msvcrt_alloc_fp()))
  {
    if (msvcrt_init_fp(file, fd, _IORW) == -1)
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
                && msvcrt_alloc_buffer(file))
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

    if((get_ioinfo_nolock(file->_file)->exflag & (EF_UTF8 | EF_UTF16))
            || !(get_ioinfo_nolock(file->_file)->wxflag & WX_TEXT)) {
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
    return MSVCRT_max_streams;
}

/*********************************************************************
 *		_setmaxstdio (MSVCRT.@)
 */
int CDECL _setmaxstdio(int newmax)
{
    TRACE("%d\n", newmax);

    if(newmax<_IOB_ENTRIES || newmax>MSVCRT_MAX_FILES || newmax<MSVCRT_stream_idx)
        return -1;

    MSVCRT_max_streams = newmax;
    return MSVCRT_max_streams;
}
