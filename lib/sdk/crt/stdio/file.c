/*
 * PROJECT:         ReactOS CRT library
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            lib/sdk/crt/stdio/file.c
 * PURPOSE:         File CRT functions
 * PROGRAMMERS:     Wine team
 *                  Ported to ReactOS by Aleksey Bragin (aleksey@reactos.org)
 */

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
#define WX_DONTINHERIT    0x10
#define WX_APPEND         0x20
#define WX_TEXT           0x80

/* FIXME: this should be allocated dynamically */
#define MAX_FILES 2048

/* ReactOS: Use _FDINFO instead! */
typedef struct {
    HANDLE              handle;
    unsigned char       wxflag;
    DWORD               unkn[7]; /* critical section and init flag */       
} ioinfo;

ioinfo fdesc[MAX_FILES];

FILE _iob[3] = { { 0 } };

static int fdstart = 3; /* first unallocated fd */
static int fdend = 3; /* highest allocated fd */

static FILE* fstreams[2048];
static int   stream_idx;

/* INTERNAL: process umask */
static int MSVCRT_umask = 0;

/* INTERNAL: Static buffer for temp file name */
static char tmpname[MAX_PATH];

/* This critical section protects the tables fdesc and fstreams,
 * and their related indexes, fdstart, fdend,
 * and stream_idx, from race conditions.
 * It doesn't protect against race conditions manipulating the underlying files
 * or flags; doing so would probably be better accomplished with per-file
 * protection, rather than locking the whole table for every change.
 */
static CRITICAL_SECTION FILE_cs;
#define LOCK_FILES()    do { EnterCriticalSection(&FILE_cs); } while (0)
#define UNLOCK_FILES()  do { LeaveCriticalSection(&FILE_cs); } while (0)

static inline BOOL is_valid_fd(int fd)
{
  return fd >= 0 && fd < fdend && (fdesc[fd].wxflag & WX_OPEN);
}

/* INTERNAL: Get the HANDLE for a fd
 * This doesn't lock the table, because a failure will result in
 * INVALID_HANDLE_VALUE being returned, which should be handled correctly.  If
 * it returns a valid handle which is about to be closed, a subsequent call
 * will fail, most likely in a sane way.
 */
HANDLE fdtoh(int fd)
{
  if (!is_valid_fd(fd))
  {
    WARN(":fd (%d) - no handle!\n",fd);
    *__doserrno() = 0;
    *_errno() = EBADF;
    return INVALID_HANDLE_VALUE;
  }
  //if (fdesc[fd].handle == INVALID_HANDLE_VALUE) FIXME("wtf\n");
  return fdesc[fd].handle;
}

/* INTERNAL: free a file entry fd */
static void free_fd(int fd)
{
  LOCK_FILES();
  fdesc[fd].handle = INVALID_HANDLE_VALUE;
  fdesc[fd].wxflag = 0;
  TRACE(":fd (%d) freed\n",fd);
  if (fd < 3) /* don't use 0,1,2 for user files */
  {
    switch (fd)
    {
    case 0: SetStdHandle(STD_INPUT_HANDLE,  NULL); break;
    case 1: SetStdHandle(STD_OUTPUT_HANDLE, NULL); break;
    case 2: SetStdHandle(STD_ERROR_HANDLE,  NULL); break;
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
  if (fd >= MAX_FILES)
  {
    WARN(":files exhausted!\n");
    return -1;
  }
  fdesc[fd].handle = hand;
  fdesc[fd].wxflag = WX_OPEN | (flag & (WX_DONTINHERIT | WX_APPEND | WX_TEXT));

  /* locate next free slot */
  if (fd == fdstart && fd == fdend)
    fdstart = fdend + 1;
  else
    while (fdstart < fdend &&
     fdesc[fdstart].handle != INVALID_HANDLE_VALUE)
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
/*static */int alloc_fd(HANDLE hand, int flag)
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

  for (i = 3; i < sizeof(fstreams) / sizeof(fstreams[0]); i++)
  {
    if (!fstreams[i] || fstreams[i]->_flag == 0)
    {
      if (!fstreams[i])
      {
        if (!(fstreams[i] = calloc(sizeof(FILE),1)))
          return NULL;
        if (i == stream_idx) stream_idx++;
      }
      return fstreams[i];
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
    if ((fdesc[fd].wxflag & (WX_OPEN | WX_DONTINHERIT)) == WX_OPEN)
    {
      *wxflag_ptr = fdesc[fd].wxflag;
      *handle_ptr = fdesc[fd].handle;
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
  int           i;

  InitializeCriticalSection(&FILE_cs);
  FILE_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": FILE_cs");
  GetStartupInfoA(&si);
  if (si.cbReserved2 != 0 && si.lpReserved2 != NULL)
  {
    char*       wxflag_ptr;
    HANDLE*     handle_ptr;

    fdend = *(unsigned*)si.lpReserved2;

    wxflag_ptr = (char*)(si.lpReserved2 + sizeof(unsigned));
    handle_ptr = (HANDLE*)(wxflag_ptr + fdend * sizeof(char));

    fdend = min(fdend, sizeof(fdesc) / sizeof(fdesc[0]));
    for (i = 0; i < fdend; i++)
    {
      if ((*wxflag_ptr & WX_OPEN) && *handle_ptr != INVALID_HANDLE_VALUE)
      {
        fdesc[i].wxflag  = *wxflag_ptr;
        fdesc[i].handle = *handle_ptr;
      }
      else
      {
        fdesc[i].wxflag  = 0;
        fdesc[i].handle = INVALID_HANDLE_VALUE;
      }
      wxflag_ptr++; handle_ptr++;
    }
    for (fdstart = 3; fdstart < fdend; fdstart++)
        if (fdesc[fdstart].handle == INVALID_HANDLE_VALUE) break;
  }

  if (!(fdesc[0].wxflag & WX_OPEN) || fdesc[0].handle == INVALID_HANDLE_VALUE)
  {
#ifndef __REACTOS__
    DuplicateHandle(GetCurrentProcess(), GetStdHandle(STD_INPUT_HANDLE),
                    GetCurrentProcess(), &fdesc[0].handle, 0, TRUE, 
                    DUPLICATE_SAME_ACCESS);
#else
      fdesc[0].handle = GetStdHandle(STD_INPUT_HANDLE);
      if (fdesc[0].handle == NULL)
         fdesc[0].handle = INVALID_HANDLE_VALUE;
#endif
    fdesc[0].wxflag = WX_OPEN | WX_TEXT;
  }
  if (!(fdesc[1].wxflag & WX_OPEN) || fdesc[1].handle == INVALID_HANDLE_VALUE)
  {
#ifndef __REACTOS__
      DuplicateHandle(GetCurrentProcess(), GetStdHandle(STD_OUTPUT_HANDLE),
                    GetCurrentProcess(), &fdesc[1].handle, 0, TRUE, 
                    DUPLICATE_SAME_ACCESS);
#else
      fdesc[1].handle = GetStdHandle(STD_OUTPUT_HANDLE);
      if (fdesc[1].handle == NULL)
         fdesc[1].handle = INVALID_HANDLE_VALUE;
#endif
    fdesc[1].wxflag = WX_OPEN | WX_TEXT;
  }
  if (!(fdesc[2].wxflag & WX_OPEN) || fdesc[2].handle == INVALID_HANDLE_VALUE)
  {
#ifndef __REACTOS__
      DuplicateHandle(GetCurrentProcess(), GetStdHandle(STD_ERROR_HANDLE),
                    GetCurrentProcess(), &fdesc[2].handle, 0, TRUE, 
                    DUPLICATE_SAME_ACCESS);
#else
      fdesc[2].handle = GetStdHandle(STD_ERROR_HANDLE);
      if (fdesc[2].handle == NULL)
         fdesc[2].handle = INVALID_HANDLE_VALUE;
#endif
    fdesc[2].wxflag = WX_OPEN | WX_TEXT;
  }

  TRACE(":handles (%p)(%p)(%p)\n",fdesc[0].handle,
	fdesc[1].handle,fdesc[2].handle);

  memset(_iob,0,3*sizeof(FILE));
  for (i = 0; i < 3; i++)
  {
    /* FILE structs for stdin/out/err are static and never deleted */
    fstreams[i] = &_iob[i];
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
static void alloc_buffer(FILE* file)
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
static void int_to_base32(int num, char *str)
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
}

/*********************************************************************
 *		__p__iob(MSVCRT.@)
 */
FILE * CDECL __p__iob(void)
{
 return _iob;
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
    _dosmaperr(ERROR_ACCESS_DENIED);
    return -1;
  }
  return 0;
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
    _dosmaperr(ERROR_ACCESS_DENIED);
    return -1;
  }
  return 0;
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

  LOCK_FILES();
  for (i = 3; i < stream_idx; i++)
    if (fstreams[i] && fstreams[i]->_flag)
    {
#if 0
      /* FIXME: flush, do not commit */
      if (_commit(i) == -1)
	if (fstreams[i])
	  fstreams[i]->_flag |= _IOERR;
#endif
      if(fstreams[i]->_flag & _IOWRT) {
	fflush(fstreams[i]);
        num_flushed++;
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
  	int res=flush_buffer(file);
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
  if (nd < MAX_FILES && is_valid_fd(od))
  {
    HANDLE handle;

    if (DuplicateHandle(GetCurrentProcess(), fdesc[od].handle,
     GetCurrentProcess(), &handle, 0, TRUE, DUPLICATE_SAME_ACCESS))
    {
      int wxflag = fdesc[od].wxflag & ~_O_NOINHERIT;

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

  if (fdesc[fd].wxflag & WX_ATEOF) return TRUE;

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

  LOCK_FILES();
  for (i = 3; i < stream_idx; i++)
    if (fstreams[i] && fstreams[i]->_flag &&
        !fclose(fstreams[i]))
      num_closed++;
  UNLOCK_FILES();

  TRACE(":closed (%d) handles\n",num_closed);
  return num_closed;
}

/* free everything on process exit */
void msvcrt_free_io(void)
{
    _fcloseall();
    /* The Win32 _fcloseall() function explicitly doesn't close stdin,
     * stdout, and stderr (unlike GNU), so we need to fclose() them here
     * or they won't get flushed.
     */
    fclose(&_iob[0]);
    fclose(&_iob[1]);
    fclose(&_iob[2]);
    FILE_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&FILE_cs);
}

/*********************************************************************
 *		_lseeki64 (MSVCRT.@)
 */
__int64 CDECL _lseeki64(int fd, __int64 offset, int whence)
{
  HANDLE hand = fdtoh(fd);
  LARGE_INTEGER ofs, ret;

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

  ofs.QuadPart = offset;
  if (SetFilePointerEx(hand, ofs, &ret, whence))
  {
    fdesc[fd].wxflag &= ~(WX_ATEOF|WX_READEOF);
    /* FIXME: What if we seek _to_ EOF - is EOF set? */

    return ret.QuadPart;
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
    return _lseeki64(fd, offset, whence);
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
 *		fseek (MSVCRT.@)
 */
int CDECL fseek(FILE* file, long offset, int whence)
{
  /* Flush output if needed */
  if(file->_flag & _IOWRT)
	flush_buffer(file);

  if(whence == SEEK_CUR && file->_flag & _IOREAD ) {
	offset -= file->_cnt;
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
  return (_lseek(file->_file,offset,whence) == -1)?-1:0;
}

/*********************************************************************
 *		_chsize (MSVCRT.@)
 */
int CDECL _chsize(int fd, long size)
{
    LONG cur, pos;
    HANDLE handle;
    BOOL ret = FALSE;

    TRACE("(fd=%d, size=%ld)\n", fd, size);

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
  file->_flag &= ~(_IOERR | _IOEOF);
}

/*********************************************************************
 *		rewind (MSVCRT.@)
 */
void CDECL rewind(FILE* file)
{
  TRACE(":file (%p) fd (%d)\n",file,file->_file);
  fseek(file, 0L, SEEK_SET);
  clearerr(file);
}

static int get_flags(const char* mode, int *open_flags, int* stream_flags)
{
  int plus = strchr(mode, '+') != NULL;

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
  else TRACE(":fd (%d) mode (%s) FILE* (%p)\n",fd,mode,file);
  UNLOCK_FILES();

  return file;
}

/*********************************************************************
 *		_wfdopen (MSVCRT.@)
 */
FILE* CDECL _wfdopen(int fd, const wchar_t *mode)
{
  unsigned mlen = strlenW(mode);
  char *modea = calloc(mlen + 1, 1);
  FILE* file = NULL;
  int open_flags, stream_flags;

  if (modea &&
      WideCharToMultiByte(CP_ACP,0,mode,mlen,modea,mlen,NULL,NULL))
  {
      if (get_flags(modea, &open_flags, &stream_flags) == -1)
      {
        free(modea);
        return NULL;
      }
      LOCK_FILES();
      if (!(file = alloc_fp()))
        file = NULL;
      else if (init_fp(file, fd, stream_flags) == -1)
      {
        file->_flag = 0;
        file = NULL;
      }
      else
      {
        if (file)
          rewind(file); /* FIXME: is this needed ??? */
        TRACE(":fd (%d) mode (%s) FILE* (%p)\n",fd,debugstr_w(mode),file);
      }
      UNLOCK_FILES();
  }
  free(modea);
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
 *		_futime (MSVCRT.@)
 */
int CDECL _futime(int fd, struct _utimbuf *t)
{
  HANDLE hand = fdtoh(fd);
  FILETIME at, wt;

  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  if (!t)
  {
    time_t currTime;
    time(&currTime);
    RtlSecondsSince1970ToTime(currTime, (LARGE_INTEGER *)&at);
    wt = at;
  }
  else
  {
    RtlSecondsSince1970ToTime(t->actime, (LARGE_INTEGER *)&at);
    if (t->actime == t->modtime)
      wt = at;
    else
      RtlSecondsSince1970ToTime(t->modtime, (LARGE_INTEGER *)&wt);
  }

  if (!SetFileTime(hand, NULL, &at, &wt))
  {
    _dosmaperr(GetLastError());
    return -1 ;
  }
  return 0;
}

/*********************************************************************
 *		_get_osfhandle (MSVCRT.@)
 */
intptr_t CDECL _get_osfhandle(int fd)
{
  HANDLE hand = fdtoh(fd);
  TRACE(":fd (%d) handle (%p)\n",fd,hand);

  return (long)(LONG_PTR)hand;
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

/*static */unsigned split_oflags(unsigned oflags)
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
 *              _sopen (MSVCRT.@)
 */
int CDECL _sopen( const char *path, int oflags, int shflags, ... )
{
  va_list ap;
  int pmode;
  DWORD access = 0, creation = 0, attrib;
  DWORD sharing;
  int wxflag = 0, fd;
  HANDLE hand;
  SECURITY_ATTRIBUTES sa;


  TRACE(":file (%s) oflags: 0x%04x shflags: 0x%04x\n",
        path, oflags, shflags);

  wxflag = split_oflags(oflags);
  switch (oflags & (_O_RDONLY | _O_WRONLY | _O_RDWR))
  {
  case _O_RDONLY: access |= GENERIC_READ; break;
  case _O_WRONLY: access |= GENERIC_WRITE; break;
  case _O_RDWR:   access |= GENERIC_WRITE | GENERIC_READ; break;
  }

  if (oflags & _O_CREAT)
  {
    va_start(ap, shflags);
      pmode = va_arg(ap, int);
    va_end(ap);

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
      return -1;
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
    WARN(":failed-last error (%d)\n",GetLastError());
    _dosmaperr(GetLastError());
    return -1;
  }

  fd = alloc_fd(hand, wxflag);

  TRACE(":fd (%d) handle (%p)\n",fd, hand);
  return fd;
}

/*********************************************************************
 *              _wsopen (MSVCRT.@)
 */
int CDECL _wsopen( const wchar_t* path, int oflags, int shflags, ... )
{
  const unsigned int len = strlenW(path);
  char *patha = calloc(len + 1,1);
  va_list ap;
  int pmode;

  va_start(ap, shflags);
  pmode = va_arg(ap, int);
  va_end(ap);

  if (patha && WideCharToMultiByte(CP_ACP,0,path,len,patha,len,NULL,NULL))
  {
    int retval = _sopen(patha,oflags,shflags,pmode);
    free(patha);
    return retval;
  }
  _dosmaperr(GetLastError());
  free(patha);
  return -1;
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
  const unsigned int len = strlenW(path);
  char *patha = calloc(len + 1,1);
  va_list ap;
  int pmode;

  va_start(ap, flags);
  pmode = va_arg(ap, int);
  va_end(ap);

  if (patha && WideCharToMultiByte(CP_ACP,0,path,len,patha,len,NULL,NULL))
  {
    int retval = _open(patha,flags,pmode);
    free(patha);
    return retval;
  }

  _dosmaperr(GetLastError());
  return -1;
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

  fd = alloc_fd((HANDLE)(LONG_PTR)handle, split_oflags(oflags));
  TRACE(":handle (%ld) fd (%d) flags 0x%08x\n", handle, fd, oflags);
  return fd;
}

/*********************************************************************
 *		_rmtmp (MSVCRT.@)
 */
int CDECL _rmtmp(void)
{
  int num_removed = 0, i;

  LOCK_FILES();
  for (i = 3; i < stream_idx; i++)
    if (fstreams[i] && fstreams[i]->_tmpfname)
    {
      fclose(fstreams[i]);
      num_removed++;
    }
  UNLOCK_FILES();

  if (num_removed)
    TRACE(":removed (%d) temp files\n",num_removed);
  return num_removed;
}

/*********************************************************************
 * (internal) remove_cr
 *
 * Translate all \r\n to \n inplace.
 * return the number of \r removed
 * Corner cases required by some apps:
 *   \r\r\n -> \r\n
 * BUG: should save state across calls somehow, so CR LF that
 * straddles buffer boundary gets recognized properly?
 */
static unsigned int remove_cr(char *buf, unsigned int count)
{
    unsigned int i, j;

    for (i=0, j=0; j < count; j++)
        if ((buf[j] != '\r') || ((j+1) < count && buf[j+1] != '\n'))
	    buf[i++] = buf[j];

    return count - i;
}

/*********************************************************************
 * (internal) read_i
 */
static int read_i(int fd, void *buf, unsigned int count)
{
  DWORD num_read;
  char *bufstart = buf;
  HANDLE hand = fdtoh(fd);

  if (fdesc[fd].wxflag & WX_READEOF) {
     fdesc[fd].wxflag |= WX_ATEOF;
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
        if (fdesc[fd].wxflag & WX_TEXT)
        {
            int i;
            /* in text mode, a ctrl-z signals EOF */
            for (i=0; i<num_read; i++)
            {
                if (bufstart[i] == 0x1a)
                {
                    num_read = i;
                    fdesc[fd].wxflag |= (WX_ATEOF|WX_READEOF);
                    TRACE(":^Z EOF %s\n",debugstr_an(buf,num_read));
                    break;
                }
            }
        }
        if (count != 0 && num_read == 0)
        {
            fdesc[fd].wxflag |= (WX_ATEOF|WX_READEOF);
            TRACE(":EOF %s\n",debugstr_an(buf,num_read));
        }
    }
    else
    {
        if (GetLastError() == ERROR_BROKEN_PIPE)
        {
            TRACE(":end-of-pipe\n");
            fdesc[fd].wxflag |= (WX_ATEOF|WX_READEOF);
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
  if (num_read>0 && fdesc[fd].wxflag & WX_TEXT)
  {
      num_read -= remove_cr(buf,num_read);
  }
  return num_read;
}

/*********************************************************************
 *		_setmode (MSVCRT.@)
 */
int CDECL _setmode(int fd,int mode)
{
  int ret = fdesc[fd].wxflag & WX_TEXT ? _O_TEXT : _O_BINARY;
  if (mode & (~(_O_TEXT|_O_BINARY)))
    FIXME("fd (%d) mode (0x%08x) unknown\n",fd,mode);
  if ((mode & _O_TEXT) == _O_TEXT)
    fdesc[fd].wxflag |= WX_TEXT;
  else
    fdesc[fd].wxflag &= ~WX_TEXT;
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
 *		_utime (MSVCRT.@)
 */
int CDECL _utime(const char* path, struct _utimbuf *t)
{
  int fd = _open(path, _O_WRONLY | _O_BINARY);

  if (fd > 0)
  {
    int retVal = _futime(fd, t);
    _close(fd);
    return retVal;
  }
  return -1;
}

/*********************************************************************
 *		_wutime (MSVCRT.@)
 */
int CDECL _wutime(const wchar_t* path, struct _utimbuf *t)
{
  int fd = _wopen(path, _O_WRONLY | _O_BINARY);

  if (fd > 0)
  {
    int retVal = _futime(fd, t);
    _close(fd);
    return retVal;
  }
  return -1;
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
  if (fdesc[fd].wxflag & WX_APPEND)
    _lseek(fd, 0, FILE_END);

  if (!(fdesc[fd].wxflag & WX_TEXT))
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
      const char *s = (const char *)buf, *buf_start = (const char *)buf;
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
              for (s = (const char *)buf, i = 0, j = 0; i < count; i++)
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
  len = _write(file->_file, &val, sizeof(val));
  if (len == sizeof(val)) return val;
  file->_flag |= _IOERR;
  return EOF;
}

/*********************************************************************
 *		fclose (MSVCRT.@)
 */
int CDECL fclose(FILE* file)
{
  int r, flag;

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
  /* Allocate buffer if needed */
  if(file->_bufsiz == 0 && !(file->_flag & _IONBF) ) {
	alloc_buffer(file);
  }
  if(!(file->_flag & _IOREAD)) {
	if(file->_flag & _IORW) {
		file->_flag |= _IOREAD;
	} else {
		return EOF;
	}
  }
  if(file->_flag & _IONBF) {
	unsigned char c;
        int r;
  	if ((r = read_i(file->_file,&c,1)) != 1) {
            file->_flag |= (r == 0) ? _IOEOF : _IOERR;
            return EOF;
	}
  	return c;
  } else {
	file->_cnt = read_i(file->_file, file->_base, file->_bufsiz);
	if(file->_cnt<=0) {
            file->_flag |= (file->_cnt == 0) ? _IOEOF : _IOERR;
            file->_cnt = 0;
            return EOF;
	}
	file->_cnt--;
	file->_ptr = file->_base+1;
	return *(unsigned char *)file->_base;
  }
}

/*********************************************************************
 *		fgetc (MSVCRT.@)
 */
int CDECL fgetc(FILE* file)
{
  unsigned char *i;
  unsigned int j;
  do {
    if (file->_cnt>0) {
      file->_cnt--;
      i = (unsigned char *)file->_ptr++;
      j = *i;
    } else
      j = _filbuf(file);
    if (!(fdesc[file->_file].wxflag & WX_TEXT)
    || ((j != '\r') || (file->_cnt && file->_ptr[0] != '\n')))
        return j;
  } while(1);
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

  while ((size >1) && (cc = fgetc(file)) != EOF && cc != '\n')
    {
      *s++ = (char)cc;
      size --;
    }
  if ((cc == EOF) && (s == buf_start)) /* If nothing read, return 0*/
  {
    TRACE(":nothing read\n");
    return NULL;
  }
  if ((cc != EOF) && (size > 1))
    *s++ = cc;
  *s = '\0';
  TRACE(":got %s\n", debugstr_a(buf_start));
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
  char c;

  if (!(fdesc[file->_file].wxflag & WX_TEXT))
    {
      wchar_t wc;
      int i,j;
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
            return WEOF;
          }
          wcp[i] = j;
        }
      }
      return wc;
    }
    
  c = fgetc(file);
  if ((*__p___mb_cur_max() > 1) && isleadbyte(c))
    {
      FIXME("Treat Multibyte characters\n");
    }
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
  int i, j, k;
  ch = (char *)&i;
  for (j=0; j<sizeof(int); j++) {
    k = fgetc(file);
    if (k == EOF) {
      file->_flag |= _IOEOF;
      return EOF;
    }
    ch[j] = k;
  }
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

  while ((size >1) && (cc = fgetwc(file)) != WEOF && cc != '\n')
    {
      *s++ = (char)cc;
      size --;
    }
  if ((cc == WEOF) && (s == buf_start)) /* If nothing read, return 0*/
  {
    TRACE(":nothing read\n");
    return NULL;
  }
  if ((cc != WEOF) && (size > 1))
    *s++ = cc;
  *s = 0;
  TRACE(":got %s\n", debugstr_w(buf_start));
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
  if(file->_cnt) {
	int pcnt=(file->_cnt>wrcnt)? wrcnt: file->_cnt;
	memcpy(file->_ptr, ptr, pcnt);
	file->_cnt -= pcnt;
	file->_ptr += pcnt;
	written = pcnt;
	wrcnt -= pcnt;
        ptr = (const char*)ptr + pcnt;
  } else if(!(file->_flag & _IOWRT)) {
	if(file->_flag & _IORW) {
		file->_flag |= _IOWRT;
	} else
		return 0;
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
  return written / size;
}

/*********************************************************************
 *		fputwc (MSVCRT.@)
 */
wint_t CDECL fputwc(wint_t wc, FILE* file)
{
  wchar_t mwc=wc;
  if (fwrite( &mwc, sizeof(mwc), 1, file) != 1)
    return WEOF;
  return wc;
}

/*********************************************************************
 *		_fputwchar (MSVCRT.@)
 */
wint_t CDECL _fputwchar(wint_t wc)
{
  return fputwc(wc, stdout);
}

/*********************************************************************
 *		_fsopen (MSVCRT.@)
 */
FILE * CDECL _fsopen(const char *path, const char *mode, int share)
{
  FILE* file;
  int open_flags, stream_flags, fd;

  TRACE("(%s,%s)\n",path,mode);

  /* map mode string to open() flags. "man fopen" for possibilities. */
  if (get_flags(mode, &open_flags, &stream_flags) == -1)
      return NULL;

  LOCK_FILES();
  fd = _sopen(path, open_flags, share, _S_IREAD | _S_IWRITE);
  if (fd < 0)
    file = NULL;
  else if ((file = alloc_fp()) && init_fp(file, fd, stream_flags)
   != -1)
    TRACE(":fd (%d) mode (%s) FILE* (%p)\n",fd,mode,file);
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
 *		_wfsopen (MSVCRT.@)
 */
FILE * CDECL _wfsopen(const wchar_t *path, const wchar_t *mode, int share)
{
  const unsigned int plen = strlenW(path), mlen = strlenW(mode);
  char *patha = calloc(plen + 1, 1);
  char *modea = calloc(mlen + 1, 1);

  TRACE("(%s,%s)\n",debugstr_w(path),debugstr_w(mode));

  if (patha && modea &&
      WideCharToMultiByte(CP_ACP,0,path,plen,patha,plen,NULL,NULL) &&
      WideCharToMultiByte(CP_ACP,0,mode,mlen,modea,mlen,NULL,NULL))
  {
    FILE *retval = _fsopen(patha,modea,share);
    free(patha);
    free(modea);
    return retval;
  }
  free(patha);
  free(modea);
  _dosmaperr(GetLastError());
  return NULL;
}

/*********************************************************************
 *		fopen (MSVCRT.@)
 */
FILE * CDECL fopen(const char *path, const char *mode)
{
    return _fsopen( path, mode, _SH_DENYNO );
}

/*********************************************************************
 *		_wfopen (MSVCRT.@)
 */
FILE * CDECL _wfopen(const wchar_t *path, const wchar_t *mode)
{
    return _wfsopen( path, mode, _SH_DENYNO );
}

/* fputc calls _flsbuf which calls fputc */
int CDECL _flsbuf(int c, FILE* file);

/*********************************************************************
 *		fputc (MSVCRT.@)
 */
int CDECL fputc(int c, FILE* file)
{
  if(file->_cnt>0) {
    *file->_ptr++=c;
    file->_cnt--;
    if (c == '\n')
    {
      int res = flush_buffer(file);
      return res ? res : c;
    }
    else
      return c & 0xff;
  } else {
    return _flsbuf(c, file);
  }
}

/*********************************************************************
 *		_flsbuf (MSVCRT.@)
 */
int CDECL _flsbuf(int c, FILE* file)
{
  /* Flush output buffer */
  if(file->_bufsiz == 0 && !(file->_flag & _IONBF)) {
	alloc_buffer(file);
  }
  if(!(file->_flag & _IOWRT)) {
	if(file->_flag & _IORW) {
		file->_flag |= _IOWRT;
	} else {
		return EOF;
	}
  }
  if(file->_bufsiz) {
        int res=flush_buffer(file);
	return res?res : fputc(c, file);
  } else {
	unsigned char cc=c;
        int len;
	len = _write(file->_file, &cc, 1);
        if (len == 1) return c & 0xff;
        file->_flag |= _IOERR;
        return EOF;
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
{ size_t rcnt=size * nmemb;
  size_t read=0;
  int pread=0;

  if(!rcnt)
	return 0;

  /* first buffered data */
  if(file->_cnt>0) {
	int pcnt= (rcnt>file->_cnt)? file->_cnt:rcnt;
	memcpy(ptr, file->_ptr, pcnt);
	file->_cnt -= pcnt;
	file->_ptr += pcnt;
	if (fdesc[file->_file].wxflag & WX_TEXT)
            pcnt -= remove_cr(ptr,pcnt);
	read += pcnt ;
	rcnt -= pcnt ;
        ptr = (char*)ptr + pcnt;
  } else if(!(file->_flag & _IOREAD )) {
	if(file->_flag & _IORW) {
		file->_flag |= _IOREAD;
	} else
            return 0;
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
      i = (file->_cnt<rcnt) ? file->_cnt : rcnt;
      /* If the buffer fill reaches eof but fread wouldn't, clear eof. */
      if (i > 0 && i < file->_cnt) {
        fdesc[file->_file].wxflag &= ~WX_ATEOF;
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
    if ( fdesc[file->_file].wxflag & WX_ATEOF)
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
  return read / size;
}

/*********************************************************************
 *		freopen (MSVCRT.@)
 *
 */
FILE* CDECL freopen(const char *path, const char *mode,FILE* file)
{
  int open_flags, stream_flags, fd;

  TRACE(":path (%p) mode (%s) file (%p) fd (%d)\n",path,mode,file,file->_file);

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
      fd = _open(path, open_flags, _S_IREAD | _S_IWRITE);
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
 *		wfreopen (MSVCRT.@)
 *
 */
FILE* CDECL _wfreopen(const wchar_t *path, const wchar_t *mode,FILE* file)
{
    FIXME("UNIMPLEMENTED stub!\n");
    return NULL;
}

/*********************************************************************
 *		fsetpos (MSVCRT.@)
 */
int CDECL fsetpos(FILE* file, const fpos_t *pos)
{
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

  return (_lseeki64(file->_file,*pos,SEEK_SET) == -1) ? -1 : 0;
}

/*********************************************************************
 *		ftell (MSVCRT.@)
 */
LONG CDECL ftell(FILE* file)
{
  int off=0;
  long pos;
  if(file->_bufsiz)  {
	if( file->_flag & _IOWRT ) {
		off = file->_ptr - file->_base;
	} else {
		off = -file->_cnt;
	}
  }
  pos = _tell(file->_file);
  if(pos == -1) return pos;
  return off + pos;
}

/*********************************************************************
 *		fgetpos (MSVCRT.@)
 */
int CDECL fgetpos(FILE* file, fpos_t *pos)
{
  /* This code has been lifted form the ftell function */
  int off=0;

  *pos = _lseeki64(file->_file,0,SEEK_CUR);

  if (*pos == -1) return -1;
  
  if(file->_bufsiz)  {
	if( file->_flag & _IOWRT ) {
		off = file->_ptr - file->_base;
	} else {
		off = -file->_cnt;
	}
  }
  *pos += off;
  
  return 0;
}

/*********************************************************************
 *		fputs (MSVCRT.@)
 */
int CDECL fputs(const char *s, FILE* file)
{
    size_t i, len = strlen(s);
    if (!(fdesc[file->_file].wxflag & WX_TEXT))
      return fwrite(s,sizeof(*s),len,file) == len ? 0 : EOF;
    for (i=0; i<len; i++)
      if (fputc(s[i], file) == EOF) 
	return EOF;
    return 0;
}

/*********************************************************************
 *		fputws (MSVCRT.@)
 */
int CDECL fputws(const wchar_t *s, FILE* file)
{
    size_t i, len = strlenW(s);
    if (!(fdesc[file->_file].wxflag & WX_TEXT))
      return fwrite(s,sizeof(*s),len,file) == len ? 0 : EOF;
    for (i=0; i<len; i++)
      {
        if ((s[i] == '\n') && (fputc('\r', file) == EOF))
	  return WEOF;
	if (fputwc(s[i], file) == WEOF)
	  return WEOF; 
      }
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

  for(cc = fgetc(stdin); cc != EOF && cc != '\n';
      cc = fgetc(stdin))
  if(cc != '\r') *buf++ = (char)cc;

  *buf = '\0';

  TRACE("got '%s'\n", buf_start);
  return buf_start;
}

/*********************************************************************
 *		_getws (MSVCRT.@)
 */
wchar_t* CDECL _getws(wchar_t* buf)
{
    wint_t cc;
    wchar_t* ws = buf;

    for (cc = fgetwc(stdin); cc != WEOF && cc != '\n';
         cc = fgetwc(stdin))
    {
        if (cc != '\r')
            *buf++ = (wchar_t)cc;
    }
    *buf = '\0';

    TRACE("got %s\n", debugstr_w(ws));
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
 *		puts (MSVCRT.@)
 */
int CDECL puts(const char *s)
{
    size_t len = strlen(s);
    if (fwrite(s,sizeof(*s),len,stdout) != len) return EOF;
    return fwrite("\n",1,1,stdout) == 1 ? 0 : EOF;
}

/*********************************************************************
 *		_putws (MSVCRT.@)
 */
int CDECL _putws(const wchar_t *s)
{
    static const wchar_t nl = '\n';
    size_t len = strlenW(s);
    if (fwrite(s,sizeof(*s),len,stdout) != len) return EOF;
    return fwrite(&nl,sizeof(nl),1,stdout) == 1 ? 0 : EOF;
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
  /* TODO: Check if file busy */
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
  static int unique;
  char tmpstr[16];
  char *p;
  int count;
  if (s == 0)
    s = tmpname;
  int_to_base32(GetCurrentProcessId(), tmpstr);
  p = s + sprintf(s, "\\s%s.", tmpstr);
  for (count = 0; count < TMP_MAX; count++)
  {
    int_to_base32(unique++, tmpstr);
    strcpy(p, tmpstr);
    if (GetFileAttributesA(s) == INVALID_FILE_ATTRIBUTES &&
        GetLastError() == ERROR_FILE_NOT_FOUND)
      break;
  }
  return s;
}

/*********************************************************************
 *		wtmpnam (MSVCRT.@)
 */
wchar_t * CDECL _wtmpnam(wchar_t *s)
{
    ERR("UNIMPLEMENTED!\n");
    return NULL;
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
 *		vfprintf (MSVCRT.@)
 */
int CDECL vfprintf(FILE* file, const char *format, va_list valist)
{
  char buf[2048], *mem = buf;
  int written, resize = sizeof(buf), retval;
  /* There are two conventions for vsnprintf failing:
   * Return -1 if we truncated, or
   * Return the number of bytes that would have been written
   * The code below handles both cases
   */
  while ((written = _vsnprintf(mem, resize, format, valist)) == -1 ||
          written > resize)
  {
    resize = (written == -1 ? resize * 2 : written + 1);
    if (mem != buf)
      free (mem);
    if (!(mem = malloc(resize)))
      return EOF;
  }
  retval = fwrite(mem, sizeof(*mem), written, file);
  if (mem != buf)
    free (mem);
  return retval;
}

/*********************************************************************
 *		vfwprintf (MSVCRT.@)
 * FIXME:
 * Is final char included in written (then resize is too big) or not
 * (then we must test for equality too)?
 */
int CDECL vfwprintf(FILE* file, const wchar_t *format, va_list valist)
{
  wchar_t buf[2048], *mem = buf;
  int written, resize = sizeof(buf) / sizeof(wchar_t), retval;
  /* See vfprintf comments */
  while ((written = _vsnwprintf(mem, resize, format, valist)) == -1 ||
          written > resize)
  {
    resize = (written == -1 ? resize * 2 : written + sizeof(wchar_t));
    if (mem != buf)
      free (mem);
    if (!(mem = malloc(resize*sizeof(*mem))))
      return EOF;
  }

  /* Check if outputting to a text-file */
  if (fdesc[file->_file].wxflag & WX_TEXT)
  {
      /* Convert each character and stop at the first invalid character. Behavior verified by tests under WinXP SP2 */
      char chMultiByte[MB_LEN_MAX];
      int nReturn;
      wchar_t *p;

      retval = 0;

      for (p = mem; *p; p++)
      {
          nReturn = wctomb(chMultiByte, *p);

          if(nReturn == -1)
              break;

          retval += fwrite(chMultiByte, 1, nReturn, file);
      }
  }
  else
  {
    retval = fwrite(mem, sizeof(*mem), written, file);
  }

  if (mem != buf)
    free (mem);

  return retval;
}

/*********************************************************************
 *		vprintf (MSVCRT.@)
 */
int CDECL vprintf(const char *format, va_list valist)
{
  return vfprintf(stdout,format,valist);
}

/*********************************************************************
 *		vwprintf (MSVCRT.@)
 */
int CDECL vwprintf(const wchar_t *format, va_list valist)
{
  return vfwprintf(stdout,format,valist);
}

/*********************************************************************
 *		fprintf (MSVCRT.@)
 */
int CDECL fprintf(FILE* file, const char *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = vfprintf(file, format, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		fwprintf (MSVCRT.@)
 */
int CDECL fwprintf(FILE* file, const wchar_t *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = vfwprintf(file, format, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		printf (MSVCRT.@)
 */
int CDECL printf(const char *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = vfprintf(stdout, format, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		ungetc (MSVCRT.@)
 */
int CDECL ungetc(int c, FILE * file)
{
	if (c == EOF)
		return EOF;
	if(file->_bufsiz == 0 && !(file->_flag & _IONBF)) {
		alloc_buffer(file);
		file->_ptr++;
	}
	if(file->_ptr>file->_base) {
		file->_ptr--;
		*file->_ptr=c;
		file->_cnt++;
		clearerr(file);
		return c;
	}
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
	for(i=sizeof(wchar_t)-1;i>=0;i--) {
		if(pp[i] != ungetc(pp[i],file))
			return WEOF;
	}
	return mwc;
}

/*********************************************************************
 *		wprintf (MSVCRT.@)
 */
int CDECL wprintf(const wchar_t *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = vwprintf(format, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_getmaxstdio (MSVCRT.@)
 */
int CDECL _getmaxstdio(void)
{
    FIXME("stub, always returns 512\n");
    return 512;
}

/*********************************************************************
 *		_setmaxstdio_ (MSVCRT.@)
 */
int CDECL _setmaxstdio(int newmax)
{
    int res;
    if( newmax > 2048)
      res = -1;
    else
      res = newmax;
    FIXME("stub: setting new maximum for number of simultaneously open files not implemented,returning %d\n",res);
    return res;
}

/*********************************************************************
 *		__pioinfo (MSVCRT.@)
 * FIXME: see MAX_FILES define.
 */
ioinfo * __pioinfo[] = { /* array of pointers to ioinfo arrays [64] */
    &fdesc[0 * 64], &fdesc[1 * 64], &fdesc[2 * 64],
    &fdesc[3 * 64], &fdesc[4 * 64], &fdesc[5 * 64],
    &fdesc[6 * 64], &fdesc[7 * 64], &fdesc[8 * 64],
    &fdesc[9 * 64], &fdesc[10 * 64], &fdesc[11 * 64],
    &fdesc[12 * 64], &fdesc[13 * 64], &fdesc[14 * 64],
    &fdesc[15 * 64], &fdesc[16 * 64], &fdesc[17 * 64],
    &fdesc[18 * 64], &fdesc[19 * 64], &fdesc[20 * 64],
    &fdesc[21 * 64], &fdesc[22 * 64], &fdesc[23 * 64],
    &fdesc[24 * 64], &fdesc[25 * 64], &fdesc[26 * 64],
    &fdesc[27 * 64], &fdesc[28 * 64], &fdesc[29 * 64],
    &fdesc[30 * 64], &fdesc[31 * 64]
} ;

/*********************************************************************
 *		__badioinfo (MSVCRT.@)
 */
ioinfo __badioinfo = { INVALID_HANDLE_VALUE, WX_TEXT, { 0, } };
