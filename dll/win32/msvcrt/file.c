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
 */

#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <stdarg.h>
#include <sys/locking.h>
#include <sys/types.h>
#include <sys/utime.h>
#include <time.h>
#include <limits.h>

#include "windef.h"
#include "winbase.h"
#include "wincon.h"
#include "winternl.h"
#include "winnls.h"
#include "msvcrt.h"
#include "mtdll.h"
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

#undef _fstat
#undef _fstati64
#undef _stat
#undef _stati64
#undef _wstat
#undef _wstati64
#undef _fstat32
#undef _fstat32i64
#undef _stat32i64
#undef _stat32
#undef _wstat32
#undef _wstat32i64
#undef _fstat64i32
#undef _fstat64
#undef _stat64
#undef _stat64i32
#undef _wstat64i32
#undef _wstat64
int __cdecl _wstat64(const wchar_t*, struct _stat64*);

/* for stat mode, permissions apply to all,owner and group */
#define ALL_S_IREAD  (_S_IREAD  | (_S_IREAD  >> 3) | (_S_IREAD  >> 6))
#define ALL_S_IWRITE (_S_IWRITE | (_S_IWRITE >> 3) | (_S_IWRITE >> 6))
#define ALL_S_IEXEC  (_S_IEXEC  | (_S_IEXEC  >> 3) | (_S_IEXEC  >> 6))

/* _access() bit flags FIXME: incomplete */
#define MSVCRT_W_OK      0x02
#define MSVCRT_R_OK      0x04

/* values for wxflag in file descriptor */
#define WX_OPEN           0x01
#define WX_ATEOF          0x02
#define WX_READNL         0x04  /* read started with \n */
#define WX_PIPE           0x08
#define WX_DONTINHERIT    0x10
#define WX_APPEND         0x20
#define WX_TTY            0x40
#define WX_TEXT           0x80

static char utf8_bom[3] = { 0xef, 0xbb, 0xbf };
static char utf16_bom[2] = { 0xff, 0xfe };

#define MSVCRT_INTERNAL_BUFSIZ 4096

enum textmode
{
    TEXTMODE_ANSI,
    TEXTMODE_UTF8,
    TEXTMODE_UTF16LE,
};

#if _MSVCR_VER >= 140

#define MSVCRT_MAX_FILES 8192
#define MSVCRT_FD_BLOCK_SIZE 64

typedef struct {
    CRITICAL_SECTION    crit;
    HANDLE              handle;
    __int64             startpos;
    unsigned char       wxflag;
    char                textmode;
    char                lookahead[3];
    unsigned int unicode          : 1;
    unsigned int utf8translations : 1;
    unsigned int dbcsBufferUsed   : 1;
    char dbcsBuffer[MB_LEN_MAX];
} ioinfo;

/*********************************************************************
 *		__badioinfo (MSVCRT.@)
 */
ioinfo MSVCRT___badioinfo = { {0}, INVALID_HANDLE_VALUE, 0, WX_TEXT };
#else

#define MSVCRT_MAX_FILES 2048
#define MSVCRT_FD_BLOCK_SIZE 32

typedef struct {
    HANDLE              handle;
    unsigned char       wxflag;
    char                lookahead[3];
    int                 exflag;
    CRITICAL_SECTION    crit;
#if _MSVCR_VER >= 80
    char textmode : 7;
    char unicode : 1;
    char pipech2[2];
    __int64 startpos;
    BOOL utf8translations;
    char dbcsBuffer[1];
    BOOL dbcsBufferUsed;
#endif
} ioinfo;

/*********************************************************************
 *		__badioinfo (MSVCRT.@)
 */
ioinfo MSVCRT___badioinfo = { INVALID_HANDLE_VALUE, WX_TEXT };
#endif

/*********************************************************************
 *		__pioinfo (MSVCRT.@)
 * array of pointers to ioinfo arrays [32]
 */
ioinfo * MSVCRT___pioinfo[MSVCRT_MAX_FILES/MSVCRT_FD_BLOCK_SIZE] = { 0 };

#if _MSVCR_VER >= 80

#if _MSVCR_VER >= 140
static inline BOOL ioinfo_is_crit_init(ioinfo *info)
{
    return TRUE;
}

static inline void ioinfo_set_crit_init(ioinfo *info)
{
}
#else
static inline BOOL ioinfo_is_crit_init(ioinfo *info)
{
    return info->exflag & 1;
}

static inline void ioinfo_set_crit_init(ioinfo *info)
{
    info->exflag |= 1;
}
#endif

static inline enum textmode ioinfo_get_textmode(ioinfo *info)
{
    return info->textmode;
}

static inline void ioinfo_set_textmode(ioinfo *info, enum textmode mode)
{
    info->textmode = mode;
}

static inline void ioinfo_set_unicode(ioinfo *info, BOOL unicode)
{
    info->unicode = !!unicode;
}
#else

#define EF_UTF8           0x01
#define EF_UTF16          0x02
#define EF_CRIT_INIT      0x04
#define EF_UNK_UNICODE    0x08

static inline BOOL ioinfo_is_crit_init(ioinfo *info)
{
    return info->exflag & EF_CRIT_INIT;
}

static inline void ioinfo_set_crit_init(ioinfo *info)
{
    info->exflag |= EF_CRIT_INIT;
}

static inline enum textmode ioinfo_get_textmode(ioinfo *info)
{
    if (info->exflag & EF_UTF8)
        return TEXTMODE_UTF8;
    if (info->exflag & EF_UTF16)
        return TEXTMODE_UTF16LE;
    return TEXTMODE_ANSI;
}

static inline void ioinfo_set_textmode(ioinfo *info, enum textmode mode)
{
    info->exflag &= EF_CRIT_INIT | EF_UNK_UNICODE;
    switch (mode)
    {
        case TEXTMODE_ANSI:
            break;
        case TEXTMODE_UTF8:
            info->exflag |= EF_UTF8;
            break;
        case TEXTMODE_UTF16LE:
            info->exflag |= EF_UTF16;
            break;
    }
}

static inline void ioinfo_set_unicode(ioinfo *info, BOOL unicode)
{
    if (unicode)
        info->exflag |= EF_UNK_UNICODE;
    else
        info->exflag &= ~EF_UNK_UNICODE;
}
#endif

typedef struct {
    FILE file;
    CRITICAL_SECTION crit;
} file_crit;

#if _MSVCR_VER >= 140
file_crit MSVCRT__iob[_IOB_ENTRIES] = { 0 };

static FILE* iob_get_file(int i)
{
    return &MSVCRT__iob[i].file;
}

static CRITICAL_SECTION* file_get_cs(FILE *f)
{
    return &((file_crit*)f)->crit;
}
#else
FILE MSVCRT__iob[_IOB_ENTRIES] = { { 0 } };

static FILE* iob_get_file(int i)
{
    return &MSVCRT__iob[i];
}

static CRITICAL_SECTION* file_get_cs(FILE *f)
{
    if (f < iob_get_file(0) || f >= iob_get_file(_IOB_ENTRIES))
        return &((file_crit*)f)->crit;
    return NULL;
}
#endif

static file_crit* MSVCRT_fstream[MSVCRT_MAX_FILES/MSVCRT_FD_BLOCK_SIZE];
static int MSVCRT_max_streams = 512, MSVCRT_stream_idx;

/* INTERNAL: process umask */
static int MSVCRT_umask = 0;

/* INTERNAL: static data for tmpnam and _wtmpname functions */
static LONG tmpnam_unique;
static LONG tmpnam_s_unique;

#define TOUL(x) (ULONGLONG)(x)
static const ULONGLONG WCEXE = TOUL('e') << 32 | TOUL('x') << 16 | TOUL('e');
static const ULONGLONG WCBAT = TOUL('b') << 32 | TOUL('a') << 16 | TOUL('t');
static const ULONGLONG WCCMD = TOUL('c') << 32 | TOUL('m') << 16 | TOUL('d');
static const ULONGLONG WCCOM = TOUL('c') << 32 | TOUL('o') << 16 | TOUL('m');

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

static void msvcrt_stat64_to_stat(const struct _stat64 *buf64, struct _stat *buf)
{
    buf->st_dev   = buf64->st_dev;
    buf->st_ino   = buf64->st_ino;
    buf->st_mode  = buf64->st_mode;
    buf->st_nlink = buf64->st_nlink;
    buf->st_uid   = buf64->st_uid;
    buf->st_gid   = buf64->st_gid;
    buf->st_rdev  = buf64->st_rdev;
    buf->st_size  = buf64->st_size;
    buf->st_atime = buf64->st_atime;
    buf->st_mtime = buf64->st_mtime;
    buf->st_ctime = buf64->st_ctime;
}

static void msvcrt_stat64_to_stati64(const struct _stat64 *buf64, struct _stati64 *buf)
{
    buf->st_dev   = buf64->st_dev;
    buf->st_ino   = buf64->st_ino;
    buf->st_mode  = buf64->st_mode;
    buf->st_nlink = buf64->st_nlink;
    buf->st_uid   = buf64->st_uid;
    buf->st_gid   = buf64->st_gid;
    buf->st_rdev  = buf64->st_rdev;
    buf->st_size  = buf64->st_size;
    buf->st_atime = buf64->st_atime;
    buf->st_mtime = buf64->st_mtime;
    buf->st_ctime = buf64->st_ctime;
}

static void msvcrt_stat64_to_stat32(const struct _stat64 *buf64, struct _stat32 *buf)
{
    buf->st_dev   = buf64->st_dev;
    buf->st_ino   = buf64->st_ino;
    buf->st_mode  = buf64->st_mode;
    buf->st_nlink = buf64->st_nlink;
    buf->st_uid   = buf64->st_uid;
    buf->st_gid   = buf64->st_gid;
    buf->st_rdev  = buf64->st_rdev;
    buf->st_size  = buf64->st_size;
    buf->st_atime = buf64->st_atime;
    buf->st_mtime = buf64->st_mtime;
    buf->st_ctime = buf64->st_ctime;
}

static void msvcrt_stat64_to_stat64i32(const struct _stat64 *buf64, struct _stat64i32 *buf)
{
    buf->st_dev   = buf64->st_dev;
    buf->st_ino   = buf64->st_ino;
    buf->st_mode  = buf64->st_mode;
    buf->st_nlink = buf64->st_nlink;
    buf->st_uid   = buf64->st_uid;
    buf->st_gid   = buf64->st_gid;
    buf->st_rdev  = buf64->st_rdev;
    buf->st_size  = buf64->st_size;
    buf->st_atime = buf64->st_atime;
    buf->st_mtime = buf64->st_mtime;
    buf->st_ctime = buf64->st_ctime;
}

static void msvcrt_stat64_to_stat32i64(const struct _stat64 *buf64, struct _stat32i64 *buf)
{
    buf->st_dev   = buf64->st_dev;
    buf->st_ino   = buf64->st_ino;
    buf->st_mode  = buf64->st_mode;
    buf->st_nlink = buf64->st_nlink;
    buf->st_uid   = buf64->st_uid;
    buf->st_gid   = buf64->st_gid;
    buf->st_rdev  = buf64->st_rdev;
    buf->st_size  = buf64->st_size;
    buf->st_atime = buf64->st_atime;
    buf->st_mtime = buf64->st_mtime;
    buf->st_ctime = buf64->st_ctime;
}

static void time_to_filetime( __time64_t time, FILETIME *ft )
{
    /* 1601 to 1970 is 369 years plus 89 leap days */
    static const __int64 secs_1601_to_1970 = ((369 * 365 + 89) * (__int64)86400);

    __int64 ticks = (time + secs_1601_to_1970) * 10000000;
    ft->dwHighDateTime = ticks >> 32;
    ft->dwLowDateTime = ticks;
}

static inline ioinfo* get_ioinfo_nolock(int fd)
{
    ioinfo *ret = NULL;
    if(fd>=0 && fd<MSVCRT_MAX_FILES)
        ret = MSVCRT___pioinfo[fd/MSVCRT_FD_BLOCK_SIZE];
    if(!ret)
        return &MSVCRT___badioinfo;

    return ret + (fd%MSVCRT_FD_BLOCK_SIZE);
}

static inline void init_ioinfo_cs(ioinfo *info)
{
    if(!ioinfo_is_crit_init(info)) {
        LOCK_FILES();
        if(!ioinfo_is_crit_init(info)) {
            InitializeCriticalSection(&info->crit);
            ioinfo_set_crit_init(info);
        }
        UNLOCK_FILES();
    }
}

static inline ioinfo* get_ioinfo(int fd)
{
    ioinfo *ret = get_ioinfo_nolock(fd);
    if(ret == &MSVCRT___badioinfo)
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
    {
        block[i].handle = INVALID_HANDLE_VALUE;
        if (ioinfo_is_crit_init(&block[i]))
        {
            /* Initialize crit section on block allocation for _MSVC_VER >= 140,
             * ioinfo_is_crit_init() is always TRUE. */
            InitializeCriticalSection(&block[i].crit);
        }
    }
    if(InterlockedCompareExchangePointer((void**)&MSVCRT___pioinfo[fd/MSVCRT_FD_BLOCK_SIZE], block, NULL))
    {
        if (ioinfo_is_crit_init(&block[0]))
        {
            for(i = 0; i < MSVCRT_FD_BLOCK_SIZE; ++i)
                DeleteCriticalSection(&block[i].crit);
        }
        free(block);
    }
    return TRUE;
}

static inline ioinfo* get_ioinfo_alloc_fd(int fd)
{
    ioinfo *ret;

    ret = get_ioinfo(fd);
    if(ret != &MSVCRT___badioinfo)
        return ret;

    if(!alloc_pioinfo_block(fd))
        return &MSVCRT___badioinfo;

    return get_ioinfo(fd);
}

static inline ioinfo* get_ioinfo_alloc(int *fd)
{
    int i;

    *fd = -1;
    for(i=0; i<MSVCRT_MAX_FILES; i++)
    {
        ioinfo *info = get_ioinfo_nolock(i);

        if(info == &MSVCRT___badioinfo)
        {
            if(!alloc_pioinfo_block(i))
                return &MSVCRT___badioinfo;
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
    return &MSVCRT___badioinfo;
}

static inline void release_ioinfo(ioinfo *info)
{
    if(info!=&MSVCRT___badioinfo && ioinfo_is_crit_init(info))
        LeaveCriticalSection(&info->crit);
}

static inline FILE* msvcrt_get_file(int i)
{
    file_crit *ret;

    if(i >= MSVCRT_max_streams)
        return NULL;

    if(i < _IOB_ENTRIES)
        return iob_get_file(i);

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

  if(fdinfo != &MSVCRT___badioinfo)
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
  ioinfo_set_unicode(fdinfo, FALSE);
  ioinfo_set_textmode(fdinfo, TEXTMODE_ANSI);

  if (hand != MSVCRT_NO_CONSOLE)
  {
    switch (fdinfo-MSVCRT___pioinfo[0])
    {
    case 0: SetStdHandle(STD_INPUT_HANDLE,  hand); break;
    case 1: SetStdHandle(STD_OUTPUT_HANDLE, hand); break;
    case 2: SetStdHandle(STD_ERROR_HANDLE,  hand); break;
    }
  }
}

/* INTERNAL: Allocate an fd slot from a Win32 HANDLE */
static int msvcrt_alloc_fd(HANDLE hand, int flag)
{
    int fd;
    ioinfo *info = get_ioinfo_alloc(&fd);

    TRACE(":handle (%p) allocating fd (%d)\n", hand, fd);

    if(info == &MSVCRT___badioinfo)
        return -1;

    msvcrt_set_fd(info, hand, flag);
    release_ioinfo(info);
    return fd;
}

/* INTERNAL: Allocate a FILE* for an fd slot */
/* caller must hold the files lock */
static FILE* msvcrt_alloc_fp(void)
{
  int i = 0;
  FILE *file;

#if _MSVCR_VER >= 140
  i = 3;
#endif
  for (; i < MSVCRT_max_streams; i++)
  {
    file = msvcrt_get_file(i);
    if (!file)
      return NULL;

    if (file->_flag == 0)
    {
      if (i == MSVCRT_stream_idx)
      {
          CRITICAL_SECTION *cs = file_get_cs(file);
          if (cs)
          {
              InitializeCriticalSectionEx(cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
              cs->DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": file_crit.crit");
          }
          MSVCRT_stream_idx++;
      }
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

  TRACE(":got FILE* (%p)\n",file);
  return 0;
}

/* INTERNAL: Create an inheritance data block (for spawned process)
 * The inheritance block is made of:
 *      00      int     nb of file descriptor (NBFD)
 *      04      char    file flags (wxflag): repeated for each fd
 *      4+NBFD  HANDLE  file handle: repeated for each fd
 */
BOOL msvcrt_create_io_inherit_block(WORD *size, BYTE **block)
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
  int           i;
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
        if(fdinfo != &MSVCRT___badioinfo)
            msvcrt_set_fd(fdinfo, *handle_ptr, *wxflag_ptr);
        release_ioinfo(fdinfo);
      }

      wxflag_ptr++; handle_ptr++;
    }
  }

  fdinfo = get_ioinfo_alloc_fd(STDIN_FILENO);
  if (!(fdinfo->wxflag & WX_OPEN) || fdinfo->handle == INVALID_HANDLE_VALUE) {
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    DWORD flags = WX_OPEN | WX_TEXT;
    DWORD type = GetFileType(h);

    if (type == FILE_TYPE_UNKNOWN) {
        h = MSVCRT_NO_CONSOLE;
        flags |= WX_TTY;
    } else if ((type & 0xf) == FILE_TYPE_CHAR) {
        flags |= WX_TTY;
    } else if ((type & 0xf) == FILE_TYPE_PIPE) {
        flags |= WX_PIPE;
    }

    msvcrt_set_fd(fdinfo, h, flags);
  }
  release_ioinfo(fdinfo);

  fdinfo = get_ioinfo_alloc_fd(STDOUT_FILENO);
  if (!(fdinfo->wxflag & WX_OPEN) || fdinfo->handle == INVALID_HANDLE_VALUE) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD flags = WX_OPEN | WX_TEXT;
    DWORD type = GetFileType(h);

    if (type == FILE_TYPE_UNKNOWN) {
        h = MSVCRT_NO_CONSOLE;
        flags |= WX_TTY;
    } else if ((type & 0xf) == FILE_TYPE_CHAR) {
        flags |= WX_TTY;
    } else if ((type & 0xf) == FILE_TYPE_PIPE) {
        flags |= WX_PIPE;
    }

    msvcrt_set_fd(fdinfo, h, flags);
  }
  release_ioinfo(fdinfo);

  fdinfo = get_ioinfo_alloc_fd(STDERR_FILENO);
  if (!(fdinfo->wxflag & WX_OPEN) || fdinfo->handle == INVALID_HANDLE_VALUE) {
    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    DWORD flags = WX_OPEN | WX_TEXT;
    DWORD type = GetFileType(h);

    if (type == FILE_TYPE_UNKNOWN) {
        h = MSVCRT_NO_CONSOLE;
        flags |= WX_TTY;
    } else if ((type & 0xf) == FILE_TYPE_CHAR) {
        flags |= WX_TTY;
    } else if ((type & 0xf) == FILE_TYPE_PIPE) {
        flags |= WX_PIPE;
    }

    msvcrt_set_fd(fdinfo, h, flags);
  }
  release_ioinfo(fdinfo);

  TRACE(":handles (%p)(%p)(%p)\n", get_ioinfo_nolock(STDIN_FILENO)->handle,
        get_ioinfo_nolock(STDOUT_FILENO)->handle,
        get_ioinfo_nolock(STDERR_FILENO)->handle);

  for (i = 0; i < 3; i++)
  {
    FILE *f = iob_get_file(i);
    CRITICAL_SECTION *cs = file_get_cs(f);

    /* FILE structs for stdin/out/err are static and never deleted */
    f->_file = get_ioinfo_nolock(i)->handle == MSVCRT_NO_CONSOLE ?
        MSVCRT_NO_CONSOLE_FD : i;
    f->_tmpfname = NULL;
    f->_flag = (i == 0) ? _IOREAD : _IOWRT;

    if (cs)
    {
      InitializeCriticalSectionEx(cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
      cs->DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": file_crit.crit");
    }
  }
  MSVCRT_stream_idx = 3;
}

/* INTERNAL: Flush stdio file buffer */
static int msvcrt_flush_buffer(FILE* file)
{
    int ret = 0;

    if((file->_flag & (_IOREAD|_IOWRT)) == _IOWRT &&
            file->_flag & (_IOMYBUF|MSVCRT__USERBUF)) {
        int cnt=file->_ptr-file->_base;
        if(cnt>0 && _write(file->_file, file->_base, cnt) != cnt) {
            file->_flag |= _IOERR;
            ret = EOF;
        } else if(file->_flag & _IORW) {
            file->_flag &= ~_IOWRT;
        }
    }

    file->_ptr=file->_base;
    file->_cnt=0;
    return ret;
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
static BOOL msvcrt_alloc_buffer(FILE* file)
{
#if _MSVCR_VER >= 140
    if((file->_file==STDOUT_FILENO && _isatty(file->_file))
        || file->_file == STDERR_FILENO)
        return FALSE;
#else
    if((file->_file==STDOUT_FILENO || file->_file==STDERR_FILENO)
            && _isatty(file->_file))
        return FALSE;
#endif

    file->_base = calloc(1, MSVCRT_INTERNAL_BUFSIZ);
    if(file->_base) {
        file->_bufsiz = MSVCRT_INTERNAL_BUFSIZ;
        file->_flag |= _IOMYBUF;
    } else {
        file->_base = (char*)(&file->_charbuf);
        file->_bufsiz = 2;
        file->_flag |= MSVCRT__NOBUF;
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
            || (file->_flag & (MSVCRT__NOBUF | _IOMYBUF | MSVCRT__USERBUF))
            || !_isatty(file->_file))
        return FALSE;

    file->_ptr = file->_base = buffers[file->_file == STDOUT_FILENO ? 0 : 1];
    file->_bufsiz = file->_cnt = BUFSIZ;
    file->_flag |= MSVCRT__USERBUF;
    return TRUE;
}

/* INTERNAL: Removes temporary buffer from stdout or stderr */
/* Only call this function when add_std_buffer returned TRUE */
static void remove_std_buffer(FILE *file)
{
    msvcrt_flush_buffer(file);
    file->_ptr = file->_base = NULL;
    file->_bufsiz = file->_cnt = 0;
    file->_flag &= ~MSVCRT__USERBUF;
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

/*********************************************************************
 *		__iob_func  (MSVCRT.@)
 */
#undef __iob_func
FILE * CDECL __iob_func(void)
{
    return iob_get_file(0);
}

#if _MSVCR_VER >= 140
/*********************************************************************
 *		__acrt_iob_func(UCRTBASE.@)
 */
FILE * CDECL __acrt_iob_func(unsigned idx)
{
    return iob_get_file(idx);
}
#endif

/*********************************************************************
 *		_access (MSVCRT.@)
 */
int CDECL _access(const char *filename, int mode)
{
    wchar_t *filenameW = NULL;
    int ret;

    if (filename && !(filenameW = wstrdupa_utf8(filename))) return -1;
    ret = _waccess(filenameW, mode);
    free(filenameW);
    return ret;
}

/*********************************************************************
 *		_access_s (MSVCRT.@)
 */
int CDECL _access_s(const char *filename, int mode)
{
  if (!MSVCRT_CHECK_PMT(filename != NULL)) return *_errno();
  if (!MSVCRT_CHECK_PMT((mode & ~(MSVCRT_R_OK | MSVCRT_W_OK)) == 0)) return *_errno();

  if (_access(filename, mode) == -1)
    return *_errno();
  return 0;
}

/*********************************************************************
 *		_waccess (MSVCRT.@)
 */
int CDECL _waccess(const wchar_t *filename, int mode)
{
  DWORD attr = GetFileAttributesW(filename);

  TRACE("(%s,%d) %ld\n", debugstr_w(filename), mode, attr);

  if (!filename || attr == INVALID_FILE_ATTRIBUTES)
  {
    msvcrt_set_errno(GetLastError());
    return -1;
  }
  if ((attr & FILE_ATTRIBUTE_READONLY) && (mode & MSVCRT_W_OK))
  {
    msvcrt_set_errno(ERROR_ACCESS_DENIED);
    return -1;
  }
  return 0;
}

/*********************************************************************
 *		_waccess_s (MSVCRT.@)
 */
int CDECL _waccess_s(const wchar_t *filename, int mode)
{
  if (!MSVCRT_CHECK_PMT(filename != NULL)) return *_errno();
  if (!MSVCRT_CHECK_PMT((mode & ~(MSVCRT_R_OK | MSVCRT_W_OK)) == 0)) return *_errno();

  if (_waccess(filename, mode) == -1)
    return *_errno();
  return 0;
}

/*********************************************************************
 *		_chmod (MSVCRT.@)
 */
int CDECL _chmod(const char *path, int flags)
{
    wchar_t *pathW = NULL;
    int ret;

    if (path && !(pathW = wstrdupa_utf8(path))) return -1;
    ret = _wchmod(pathW, flags);
    free(pathW);
    return ret;
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
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_unlink (MSVCRT.@)
 */
int CDECL _unlink(const char *path)
{
    wchar_t *pathW = NULL;
    int ret;

    if (path && !(pathW = wstrdupa_utf8(path))) return -1;
    ret = _wunlink(pathW);
    free(pathW);
    return ret;
}

/*********************************************************************
 *		_wunlink (MSVCRT.@)
 */
int CDECL _wunlink(const wchar_t *path)
{
  TRACE("(%s)\n", debugstr_w(path));
  if(DeleteFileW(path))
    return 0;
  TRACE("failed (%ld)\n", GetLastError());
  msvcrt_set_errno(GetLastError());
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
            TRACE(":failed-last error (%ld)\n", GetLastError());
            msvcrt_set_errno(GetLastError());
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
    int ret;

    if(!file) {
        msvcrt_flush_all_buffers(_IOWRT);
        ret = 0;
    } else {
        _lock_file(file);
        ret = _fflush_nolock(file);
        _unlock_file(file);
    }

    return ret;
}

/*********************************************************************
 *		_fflush_nolock (MSVCRT.@)
 */
int CDECL _fflush_nolock(FILE* file)
{
    int res;

    if(!file) {
        msvcrt_flush_all_buffers(_IOWRT);
        return 0;
    }

    res = msvcrt_flush_buffer(file);
    if(!res && (file->_flag & MSVCRT__IOCOMMIT))
        res = _commit(file->_file) ? EOF : 0;
    return res;
}

/*********************************************************************
 *		_close (MSVCRT.@)
 */
int CDECL _close(int fd)
{
  ioinfo *info = get_ioinfo(fd);
  int ret;

  TRACE(":fd (%d) handle (%p)\n", fd, info->handle);

  if (fd == MSVCRT_NO_CONSOLE_FD) {
    *_errno() = EBADF;
    ret = -1;
  } else if (!MSVCRT_CHECK_PMT_ERR(info->wxflag & WX_OPEN, EBADF)) {
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
      WARN(":failed-last error (%ld)\n", GetLastError());
      msvcrt_set_errno(GetLastError());
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

  if (info_nd == &MSVCRT___badioinfo)
  {
      *_errno() = EBADF;
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
      msvcrt_set_errno(GetLastError());
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

    for(i=0; i<ARRAY_SIZE(MSVCRT___pioinfo); i++)
    {
        if(!MSVCRT___pioinfo[i])
            continue;

        for(j=0; j<MSVCRT_FD_BLOCK_SIZE; j++)
        {
            if(ioinfo_is_crit_init(&MSVCRT___pioinfo[i][j]))
                DeleteCriticalSection(&MSVCRT___pioinfo[i][j].crit);
        }
        free(MSVCRT___pioinfo[i]);
    }

    for(j=0; j<MSVCRT_stream_idx; j++)
    {
        FILE *file = msvcrt_get_file(j);
        CRITICAL_SECTION *cs = file_get_cs(file);

        if(cs)
        {
            cs->DebugInfo->Spare[0] = 0;
            DeleteCriticalSection(cs);
        }
    }

    for(i=0; i<ARRAY_SIZE(MSVCRT_fstream); i++)
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
    *_errno() = EBADF;
    release_ioinfo(info);
    return -1;
  }

  if (whence < 0 || whence > 2)
  {
    release_ioinfo(info);
    *_errno() = EINVAL;
    return -1;
  }

  TRACE(":fd (%d) to %#I64x pos %s\n",
          fd, offset, (whence == SEEK_SET) ? "SEEK_SET" :
          (whence == SEEK_CUR) ? "SEEK_CUR" :
          (whence == SEEK_END) ? "SEEK_END" : "UNKNOWN");

  /* The MoleBox protection scheme expects msvcrt to use SetFilePointer only,
   * so a LARGE_INTEGER offset cannot be passed directly via SetFilePointerEx. */
  ofs.QuadPart = offset;
  if ((ofs.u.LowPart = SetFilePointer(info->handle, ofs.u.LowPart, &ofs.u.HighPart, whence)) != INVALID_SET_FILE_POINTER ||
      GetLastError() == ERROR_SUCCESS)
  {
    info->wxflag &= ~WX_ATEOF;
    /* FIXME: What if we seek _to_ EOF - is EOF set? */

    release_ioinfo(info);
    return ofs.QuadPart;
  }
  release_ioinfo(info);
  TRACE(":error-last error (%ld)\n", GetLastError());
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_lseek (MSVCRT.@)
 */
__msvcrt_long CDECL _lseek(int fd, __msvcrt_long offset, int whence)
{
    return _lseeki64(fd, offset, whence);
}

/*********************************************************************
 *              _lock_file (MSVCRT.@)
 */
void CDECL _lock_file(FILE *file)
{
    CRITICAL_SECTION *cs = file_get_cs(file);
    if (!cs)
        _lock(_STREAM_LOCKS + (file - iob_get_file(0)));
    else
        EnterCriticalSection(cs);
}

/*********************************************************************
 *              _unlock_file (MSVCRT.@)
 */
void CDECL _unlock_file(FILE *file)
{
    CRITICAL_SECTION *cs = file_get_cs(file);
    if (!cs)
        _unlock(_STREAM_LOCKS + (file - iob_get_file(0)));
    else
        LeaveCriticalSection(cs);
}

/*********************************************************************
 *		_locking (MSVCRT.@)
 *
 * This is untested; the underlying LockFile doesn't work yet.
 */
int CDECL _locking(int fd, int mode, __msvcrt_long nbytes)
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

  TRACE(":fd (%d) by %#lx mode %s\n",
          fd, nbytes, (mode == _LK_UNLCK) ? "_LK_UNLCK" :
          (mode == _LK_LOCK) ? "_LK_LOCK" :
          (mode == _LK_NBLCK) ? "_LK_NBLCK" :
          (mode == _LK_RLCK) ? "_LK_RLCK" :
          (mode == _LK_NBRLCK) ? "_LK_NBRLCK" :
          "UNKNOWN");

  if ((cur_locn = SetFilePointer(info->handle, 0L, NULL, FILE_CURRENT)) == INVALID_SET_FILE_POINTER)
  {
    release_ioinfo(info);
    FIXME("Seek failed\n");
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
    ret = _fseeki64_nolock(file, offset, whence);
    _unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_fseeki64_nolock (MSVCRT.@)
 */
int CDECL _fseeki64_nolock(FILE* file, __int64 offset, int whence)
{
  int ret;

  if(whence == SEEK_CUR && file->_flag & _IOREAD ) {
      whence = SEEK_SET;
      offset += _ftelli64_nolock(file);
  }

  /* Flush output if needed */
  msvcrt_flush_buffer(file);
  /* Reset direction of i/o */
  if(file->_flag & _IORW) {
        file->_flag &= ~(_IOREAD|_IOWRT);
  }
  /* Clear end of file flag */
  file->_flag &= ~_IOEOF;
  ret = (_lseeki64(file->_file,offset,whence) == -1)?-1:0;

  return ret;
}

/*********************************************************************
 *		fseek (MSVCRT.@)
 */
int CDECL fseek(FILE* file, __msvcrt_long offset, int whence)
{
    return _fseeki64( file, offset, whence );
}

/*********************************************************************
 *		_fseek_nolock (MSVCRT.@)
 */
int CDECL _fseek_nolock(FILE* file, __msvcrt_long offset, int whence)
{
    return _fseeki64_nolock( file, offset, whence );
}

/*********************************************************************
 *		_chsize_s (MSVCRT.@)
 */
int CDECL _chsize_s(int fd, __int64 size)
{
    ioinfo *info;
    __int64 cur, pos;
    BOOL ret = FALSE;

    TRACE("(fd=%d, size=%#I64x)\n", fd, size);

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
                if (!ret) msvcrt_set_errno(GetLastError());
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
int CDECL _chsize(int fd, __msvcrt_long size)
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
 *		clearerr_s (MSVCRT.@)
 */
int CDECL clearerr_s(FILE* file)
{
  TRACE(":file (%p)\n",file);

  if (!MSVCRT_CHECK_PMT(file != NULL)) return EINVAL;

  _lock_file(file);
  file->_flag &= ~(_IOERR | _IOEOF);
  _unlock_file(file);
  return 0;
}

#if defined(__i386__)
/* Stack preserving thunk for rewind
 * needed for the UIO mod for Fallout: New Vegas
 */
__ASM_GLOBAL_FUNC(rewind_preserve_stack,
                  "pushl 4(%esp)\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  "call "__ASM_NAME("rewind") "\n\t"
                  "addl $4,%esp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                  "ret")
#endif

/*********************************************************************
 *		rewind (MSVCRT.@)
 */
void CDECL rewind(FILE* file)
{
  TRACE(":file (%p) fd (%d)\n",file,file->_file);

  _lock_file(file);
  _fseek_nolock(file, 0L, SEEK_SET);
  clearerr(file);
  _unlock_file(file);
}

static int msvcrt_get_flags(const wchar_t* mode, int *open_flags, int* stream_flags)
{
  int plus = wcschr(mode, '+') != NULL;

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

  *stream_flags |= MSVCRT__commode;

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
#if _MSVCR_VER>=140
    case 'x':
      if(!MSVCRT_CHECK_PMT((*open_flags & (_O_CREAT | _O_APPEND)) == _O_CREAT))
          return -1;
      *open_flags |= _O_EXCL;
      break;
#endif
    case 'D':
      *open_flags |= _O_TEMPORARY;
      break;
    case 'T':
      *open_flags |= _O_SHORT_LIVED;
      break;
    case 'c':
      *stream_flags |= MSVCRT__IOCOMMIT;
      break;
    case 'n':
      *stream_flags &= ~MSVCRT__IOCOMMIT;
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
      if (!(*open_flags & _O_RANDOM))
          *open_flags |= _O_SEQUENTIAL;
      break;
    case 'R':
      if (!(*open_flags & _O_SEQUENTIAL))
          *open_flags |= _O_RANDOM;
      break;
    default:
      ERR("incorrect mode flag: %c\n", mode[-1]);
      break;
    }

  if(*mode == ',')
  {
    mode++;
    while(*mode == ' ') mode++;
    if(!MSVCRT_CHECK_PMT(!wcsncmp(L"ccs", mode, 3)))
      return -1;
    mode += 3;
    while(*mode == ' ') mode++;
    if(!MSVCRT_CHECK_PMT(*mode == '='))
        return -1;
    mode++;
    while(*mode == ' ') mode++;

    if(!_wcsnicmp(L"utf-8", mode, 5))
    {
      *open_flags |= _O_U8TEXT;
      mode += 5;
    }
    else if(!_wcsnicmp(L"utf-16le", mode, 8))
    {
      *open_flags |= _O_U16TEXT;
      mode += 8;
    }
    else if(!_wcsnicmp(L"unicode", mode, 7))
    {
      *open_flags |= _O_WTEXT;
      mode += 7;
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
__msvcrt_long CDECL _filelength(int fd)
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
 *		_fstat64 (MSVCRT.@)
 */
int CDECL _fstat64(int fd, struct _stat64* buf)
{
  ioinfo *info = get_ioinfo(fd);
  DWORD dw;
  DWORD type;

  TRACE(":fd (%d) stat (%p)\n", fd, buf);
  if (info->handle == INVALID_HANDLE_VALUE)
  {
    release_ioinfo(info);
    return -1;
  }

  if (!buf)
  {
    WARN(":failed-NULL buf\n");
    msvcrt_set_errno(ERROR_INVALID_PARAMETER);
    release_ioinfo(info);
    return -1;
  }

  memset(buf, 0, sizeof(struct _stat64));
  type = GetFileType(info->handle);
  if (type == FILE_TYPE_PIPE)
  {
    buf->st_dev = buf->st_rdev = fd;
    buf->st_mode = _S_IFIFO;
    buf->st_nlink = 1;
  }
  else if (type == FILE_TYPE_CHAR)
  {
    buf->st_dev = buf->st_rdev = fd;
    buf->st_mode = _S_IFCHR;
    buf->st_nlink = 1;
  }
  else /* FILE_TYPE_DISK etc. */
  {
    FILE_BASIC_INFORMATION basic_info;
    FILE_STANDARD_INFORMATION std_info;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    if ((status = NtQueryInformationFile( info->handle, &io, &basic_info, sizeof(basic_info), FileBasicInformation )) ||
        (status = NtQueryInformationFile( info->handle, &io, &std_info, sizeof(std_info), FileStandardInformation )))
    {
      WARN(":failed-error %lx\n", status);
      msvcrt_set_errno(ERROR_INVALID_PARAMETER);
      release_ioinfo(info);
      return -1;
    }
    buf->st_mode = _S_IFREG | 0444;
    if (!(basic_info.FileAttributes & FILE_ATTRIBUTE_READONLY))
      buf->st_mode |= 0222;
    buf->st_size  = std_info.EndOfFile.QuadPart;
    RtlTimeToSecondsSince1970((LARGE_INTEGER *)&basic_info.LastAccessTime, &dw);
    buf->st_atime = dw;
    RtlTimeToSecondsSince1970((LARGE_INTEGER *)&basic_info.LastWriteTime, &dw);
    buf->st_mtime = buf->st_ctime = dw;
    buf->st_nlink = std_info.NumberOfLinks;
    TRACE(":dwFileAttributes = %#lx, mode set to %#x\n",
            basic_info.FileAttributes, buf->st_mode);
  }
  release_ioinfo(info);
  return 0;
}

/*********************************************************************
 *		_fstati64 (MSVCRT.@)
 */
int CDECL _fstati64(int fd, struct _stati64* buf)
{
  int ret;
  struct _stat64 buf64;

  ret = _fstat64(fd, &buf64);
  if (!ret)
    msvcrt_stat64_to_stati64(&buf64, buf);
  return ret;
}

/*********************************************************************
 *             _fstat (MSVCRT.@)
 */
int CDECL _fstat(int fd, struct _stat* buf)
{ int ret;
  struct _stat64 buf64;

  ret = _fstat64(fd, &buf64);
  if (!ret)
      msvcrt_stat64_to_stat(&buf64, buf);
  return ret;
}

/*********************************************************************
 *		_fstat32 (MSVCR80.@)
 */
int CDECL _fstat32(int fd, struct _stat32* buf)
{
    int ret;
    struct _stat64 buf64;

    ret = _fstat64(fd, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat32(&buf64, buf);
    return ret;
}

/*********************************************************************
 *		_fstat32i64 (MSVCR80.@)
 */
int CDECL _fstat32i64(int fd, struct _stat32i64* buf)
{
    int ret;
    struct _stat64 buf64;

    ret = _fstat64(fd, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat32i64(&buf64, buf);
    return ret;
}

/*********************************************************************
 *		_fstat64i32 (MSVCR80.@)
 */
int CDECL _fstat64i32(int fd, struct _stat64i32* buf)
{
    int ret;
    struct _stat64 buf64;

    ret = _fstat64(fd, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat64i32(&buf64, buf);
    return ret;
}

/*********************************************************************
 *		_futime64 (MSVCRT.@)
 */
int CDECL _futime64(int fd, struct __utimbuf64 *t)
{
  ioinfo *info = get_ioinfo(fd);
  FILETIME at, wt;

  if (!t)
  {
      time_to_filetime( _time64(NULL), &at );
      wt = at;
  }
  else
  {
      time_to_filetime( t->actime, &at );
      time_to_filetime( t->modtime, &wt );
  }

  if (!SetFileTime(info->handle, NULL, &at, &wt))
  {
    release_ioinfo(info);
    msvcrt_set_errno(GetLastError());
    return -1 ;
  }
  release_ioinfo(info);
  return 0;
}

/*********************************************************************
 *		_futime32 (MSVCRT.@)
 */
int CDECL _futime32(int fd, struct __utimbuf32 *t)
{
    if (t)
    {
        struct __utimbuf64 t64;
        t64.actime = t->actime;
        t64.modtime = t->modtime;
        return _futime64( fd, &t64 );
    }
    else
        return _futime64( fd, NULL );
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
 *		_mktemp_s (MSVCRT.@)
 */
int CDECL _mktemp_s(char *pattern, size_t size)
{
    DWORD len, wlen, xno, id;
    wchar_t *pathW;

    if(!MSVCRT_CHECK_PMT(pattern!=NULL))
        return EINVAL;

    for(len=0; len<size; len++)
        if(!pattern[len])
            break;
    if(!MSVCRT_CHECK_PMT(len!=size && len>=6)) {
        if(size)
            pattern[0] = 0;
        return EINVAL;
    }

    for(xno=1; xno<=6; xno++)
        if(!MSVCRT_CHECK_PMT(pattern[len-xno] == 'X'))
            return EINVAL;

    id = GetCurrentProcessId();
    for(xno=1; xno<6; xno++) {
        pattern[len-xno] = id%10 + '0';
        id /= 10;
    }

    if(!(pathW = wstrdupa_utf8(pattern))) return *_errno();
    wlen = wcslen(pathW);
    for(pathW[wlen-6]='a'; pathW[wlen-6]<='z'; pathW[wlen-6]++) {
        if(GetFileAttributesW(pathW) == INVALID_FILE_ATTRIBUTES) {
            pattern[len-6] = pathW[wlen-6];
            free(pathW);
            return 0;
        }
    }
    free(pathW);

    pattern[0] = 0;
    *_errno() = EEXIST;
    return EEXIST;
}

/*********************************************************************
 *		_mktemp (MSVCRT.@)
 */
char * CDECL _mktemp(char *pattern)
{
  wchar_t *pathW, *p;
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
  if (!(pathW = wstrdupa_utf8(retVal)))
    return NULL;
  p = pathW + wcslen(pathW) - 6;
  do
  {
    *p = letter++;
    if (GetFileAttributesW(pathW) == INVALID_FILE_ATTRIBUTES)
    {
      *pattern = *p;
      free(pathW);
      return retVal;
    }
  } while(letter <= 'z');
  free(pathW);
  return NULL;
}

/*********************************************************************
 *		_wmktemp_s (MSVCRT.@)
 */
int CDECL _wmktemp_s(wchar_t *pattern, size_t size)
{
    DWORD len, xno, id;

    if(!MSVCRT_CHECK_PMT(pattern!=NULL))
        return EINVAL;

    for(len=0; len<size; len++)
        if(!pattern[len])
            break;
    if(!MSVCRT_CHECK_PMT(len!=size && len>=6)) {
        if(size)
            pattern[0] = 0;
        return EINVAL;
    }

    for(xno=1; xno<=6; xno++)
        if(!MSVCRT_CHECK_PMT(pattern[len-xno] == 'X'))
            return EINVAL;

    id = GetCurrentProcessId();
    for(xno=1; xno<6; xno++) {
        pattern[len-xno] = id%10 + '0';
        id /= 10;
    }

    for(pattern[len-6]='a'; pattern[len-6]<='z'; pattern[len-6]++) {
        if(GetFileAttributesW(pattern) == INVALID_FILE_ATTRIBUTES)
            return 0;
    }

    pattern[0] = 0;
    *_errno() = EEXIST;
    return EEXIST;
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
    if (GetFileAttributesW(retVal) == INVALID_FILE_ATTRIBUTES)
      return retVal;
    *pattern = letter++;
  } while(letter != '|');
  return NULL;
}

static unsigned split_oflags(unsigned oflags)
{
    int wxflags = 0;
    unsigned unsupp; /* until we support everything */

    if (oflags & _O_APPEND)              wxflags |= WX_APPEND;
    if (oflags & _O_BINARY)              {/* Nothing to do */}
    else if (oflags & _O_TEXT)           wxflags |= WX_TEXT;
    else if (oflags & _O_WTEXT)          wxflags |= WX_TEXT;
    else if (oflags & _O_U16TEXT)        wxflags |= WX_TEXT;
    else if (oflags & _O_U8TEXT)         wxflags |= WX_TEXT;
    else
    {
        int fmode;
        _get_fmode(&fmode);
        if (!(fmode & _O_BINARY))        wxflags |= WX_TEXT; /* default to TEXT*/
    }
    if (oflags & _O_NOINHERIT)           wxflags |= WX_DONTINHERIT;

    if ((unsupp = oflags & ~(_O_BINARY | _O_TEXT | _O_APPEND | _O_TRUNC | _O_EXCL | _O_CREAT |
                    _O_RDWR | _O_WRONLY | _O_TEMPORARY | _O_NOINHERIT | _O_SEQUENTIAL |
                    _O_RANDOM | _O_SHORT_LIVED | _O_WTEXT | _O_U16TEXT | _O_U8TEXT)))
        ERR(":unsupported oflags %#x\n",unsupp);

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

    fd = msvcrt_alloc_fd(readHandle, wxflags|WX_PIPE);
    if (fd != -1)
    {
      pfds[0] = fd;
      fd = msvcrt_alloc_fd(writeHandle, wxflags|WX_PIPE);
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
    msvcrt_set_errno(GetLastError());

  return ret;
}

static int check_bom(HANDLE h, int oflags, BOOL seek)
{
    char bom[sizeof(utf8_bom)];
    DWORD r;

    if (!ReadFile(h, bom, sizeof(utf8_bom), &r, NULL))
        return oflags;

    if (r==sizeof(utf8_bom) && !memcmp(bom, utf8_bom, sizeof(utf8_bom))) {
        oflags = (oflags & ~(_O_WTEXT | _O_U16TEXT)) | _O_U8TEXT;
    }else if (r>=sizeof(utf16_bom) && !memcmp(bom, utf16_bom, sizeof(utf16_bom))) {
        if (seek && r>2)
            SetFilePointer(h, 2, NULL, FILE_BEGIN);
        oflags = (oflags & ~(_O_WTEXT | _O_U8TEXT)) | _O_U16TEXT;
    }else if (seek) {
        SetFilePointer(h, 0, NULL, FILE_BEGIN);
    }

    return oflags;
}

/*********************************************************************
 *              _wsopen_dispatch (UCRTBASE.@)
 */
int CDECL _wsopen_dispatch( const wchar_t* path, int oflags, int shflags, int pmode,
    int *fd, int secure )
{
  DWORD access = 0, creation = 0, attrib;
  SECURITY_ATTRIBUTES sa;
  DWORD sharing, type;
  int wxflag;
  HANDLE hand;

  TRACE("path: (%s) oflags: %#x shflags: %#x pmode: %#x fd*: %p secure: %d\n",
        debugstr_w(path), oflags, shflags, pmode, fd, secure);

  if (!MSVCRT_CHECK_PMT( fd != NULL )) return EINVAL;
  *fd = -1;
  if (!MSVCRT_CHECK_PMT(path != NULL)) return EINVAL;

  wxflag = split_oflags(oflags);
  switch (oflags & (_O_RDONLY | _O_WRONLY | _O_RDWR))
  {
  case _O_RDONLY: access |= GENERIC_READ; break;
  case _O_WRONLY: access |= GENERIC_WRITE; break;
  case _O_RDWR:   access |= GENERIC_WRITE | GENERIC_READ; break;
  }

  if (oflags & _O_CREAT)
  {
    if (secure && !MSVCRT_CHECK_PMT(!(pmode & ~(_S_IREAD | _S_IWRITE))))
      return EINVAL;

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
      ERR( "Unhandled shflags %#x\n", shflags );
      return EINVAL;
  }

  if (!(pmode & ~MSVCRT_umask & _S_IWRITE))
      attrib = FILE_ATTRIBUTE_READONLY;
  else
      attrib = FILE_ATTRIBUTE_NORMAL;

  if (oflags & _O_TEMPORARY)
  {
      attrib |= FILE_FLAG_DELETE_ON_CLOSE;
      access |= DELETE;
      sharing |= FILE_SHARE_DELETE;
  }

  if (oflags & _O_RANDOM)
      attrib |= FILE_FLAG_RANDOM_ACCESS;
  if (oflags & _O_SEQUENTIAL)
      attrib |= FILE_FLAG_SEQUENTIAL_SCAN;
  if (oflags & _O_SHORT_LIVED)
      attrib |= FILE_ATTRIBUTE_TEMPORARY;

  sa.nLength              = sizeof( SECURITY_ATTRIBUTES );
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle       = !(oflags & _O_NOINHERIT);

  if ((oflags & (_O_WTEXT | _O_U16TEXT | _O_U8TEXT))
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
  }

  hand = CreateFileW(path, access, sharing, &sa, creation, attrib, 0);
  if (hand == INVALID_HANDLE_VALUE)  {
    WARN(":failed-last error (%ld)\n", GetLastError());
    msvcrt_set_errno(GetLastError());
    return *_errno();
  }

  if (oflags & (_O_WTEXT | _O_U16TEXT | _O_U8TEXT))
  {
      LARGE_INTEGER size = {{0}};

      if ((access & GENERIC_WRITE) && (creation==OPEN_EXISTING || creation==OPEN_ALWAYS))
          GetFileSizeEx(hand, &size);

      if ((access & GENERIC_WRITE) && (creation==CREATE_NEW
                  || creation==CREATE_ALWAYS || creation==TRUNCATE_EXISTING
                  || ((creation==OPEN_EXISTING || creation==OPEN_ALWAYS) && !size.QuadPart)))
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
                  msvcrt_set_errno(GetLastError());
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
                  msvcrt_set_errno(GetLastError());
                  return *_errno();
              }
              oflags |= _O_U16TEXT;
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
      ioinfo_set_unicode(get_ioinfo_nolock(*fd), TRUE);

  if (oflags & _O_U16TEXT)
      ioinfo_set_textmode(get_ioinfo_nolock(*fd), TEXTMODE_UTF16LE);
  else if (oflags & _O_U8TEXT)
      ioinfo_set_textmode(get_ioinfo_nolock(*fd), TEXTMODE_UTF8);

  TRACE(":fd (%d) handle (%p)\n", *fd, hand);
  return 0;
}


/*********************************************************************
 *              _wsopen_s (MSVCRT.@)
 */
int CDECL _wsopen_s( int *fd, const wchar_t* path, int oflags, int shflags, int pmode )
{
    return _wsopen_dispatch( path, oflags, shflags, pmode, fd, 1 );
}

/*********************************************************************
 *              _wsopen (MSVCRT.@)
 */
int WINAPIV _wsopen( const wchar_t *path, int oflags, int shflags, ... )
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

  return _wsopen_dispatch(path, oflags, shflags, pmode, &fd, 0) ? -1 : fd;
}


/*********************************************************************
 *              _sopen_dispatch (UCRTBASE.@)
 */
int CDECL _sopen_dispatch( const char *path, int oflags, int shflags,
    int pmode, int *fd, int secure)
{
    wchar_t *pathW = NULL;
    int ret;

    if (!MSVCRT_CHECK_PMT(fd != NULL))
        return EINVAL;
    *fd = -1;
    if (path && !(pathW = wstrdupa_utf8(path))) return *_errno();

    ret = _wsopen_dispatch(pathW, oflags, shflags, pmode, fd, secure);
    free(pathW);
    return ret;
}

/*********************************************************************
 *              _sopen_s (MSVCRT.@)
 */
int CDECL _sopen_s( int *fd, const char *path, int oflags, int shflags, int pmode )
{
    return _sopen_dispatch(path, oflags, shflags, pmode, fd, 1);
}

/*********************************************************************
 *              _sopen (MSVCRT.@)
 */
int WINAPIV _sopen( const char *path, int oflags, int shflags, ... )
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

  return _sopen_dispatch(path, oflags, shflags, pmode, &fd, 0) ? -1 : fd;
}

/*********************************************************************
 *              _open (MSVCRT.@)
 */
int WINAPIV _open( const char *path, int flags, ... )
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
int WINAPIV _wopen(const wchar_t *path,int flags,...)
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
int CDECL _creat(const char *path, int pmode)
{
  int flags = _O_CREAT | _O_TRUNC | _O_RDWR;
  return _open(path, flags, pmode);
}

/*********************************************************************
 *		_wcreat (MSVCRT.@)
 */
int CDECL _wcreat(const wchar_t *path, int pmode)
{
  int flags = _O_CREAT | _O_TRUNC | _O_RDWR;
  return _wopen(path, flags, pmode);
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
    msvcrt_set_errno(GetLastError());
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
  TRACE(":handle (%Iu) fd (%d) flags %#lx\n", handle, fd, flags);
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
                msvcrt_set_errno(GetLastError());
                if (GetLastError() == ERROR_ACCESS_DENIED)
                    *_errno() = EBADF;
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
            msvcrt_set_errno(GetLastError());
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
            msvcrt_set_errno(GetLastError());
            if (GetLastError() == ERROR_ACCESS_DENIED)
                *_errno() = EBADF;
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
        msvcrt_set_errno(GetLastError());
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

    utf16 = ioinfo_get_textmode(fdinfo) == TEXTMODE_UTF16LE;
    if (ioinfo_get_textmode(fdinfo) != TEXTMODE_ANSI && count&1)
    {
        *_errno() = EINVAL;
        return -1;
    }

    if((fdinfo->wxflag&WX_TEXT) && ioinfo_get_textmode(fdinfo) == TEXTMODE_UTF8)
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
            TRACE(":failed-last error (%ld)\n", GetLastError());
            msvcrt_set_errno(GetLastError());
            if (GetLastError() == ERROR_ACCESS_DENIED)
                *_errno() = EBADF;
            return -1;
        }
    }

    if (count > 4)
        TRACE("(%lu), %s\n", num_read, debugstr_an(buf, num_read));
    return num_read;
}

/*********************************************************************
 *		_read (MSVCRT.@)
 */
int CDECL _read(int fd, void *buf, unsigned int count)
{
    ioinfo *info;
    int num_read;

    if(fd == MSVCRT_NO_CONSOLE_FD) {
        *_errno() = EBADF;
        return -1;
    }

    info = get_ioinfo(fd);
    num_read = read_i(fd, info, buf, count);
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

    if(ret==_O_TEXT && ioinfo_get_textmode(info) != TEXTMODE_ANSI)
        ret = _O_WTEXT;

    if(mode!=_O_TEXT && mode!=_O_BINARY && mode!=_O_WTEXT
                && mode!=_O_U16TEXT && mode!=_O_U8TEXT) {
        *_errno() = EINVAL;
        release_ioinfo(info);
        return -1;
    }

    if(info == &MSVCRT___badioinfo) {
        *_errno() = EBADF;
        return EOF;
    }

    if(mode == _O_BINARY) {
        info->wxflag &= ~WX_TEXT;
        ioinfo_set_textmode(info, TEXTMODE_ANSI);
        release_ioinfo(info);
        return ret;
    }

    info->wxflag |= WX_TEXT;
    if(mode == _O_TEXT)
        ioinfo_set_textmode(info, TEXTMODE_ANSI);
    else if(mode == _O_U8TEXT)
        ioinfo_set_textmode(info, TEXTMODE_UTF8);
    else
        ioinfo_set_textmode(info, TEXTMODE_UTF16LE);

    release_ioinfo(info);
    return ret;
}

/*********************************************************************
 *		_stat64 (MSVCRT.@)
 */
int CDECL _stat64(const char* path, struct _stat64 * buf)
{
    wchar_t *pathW = NULL;
    int ret;

    if (path && !(pathW = wstrdupa_utf8(path))) return -1;
    ret = _wstat64(pathW, buf);
    free(pathW);
    return ret;
}

/*********************************************************************
 *		_stati64 (MSVCRT.@)
 */
int CDECL _stati64(const char* path, struct _stati64 * buf)
{
  int ret;
  struct _stat64 buf64;

  ret = _stat64(path, &buf64);
  if (!ret)
    msvcrt_stat64_to_stati64(&buf64, buf);
  return ret;
}

/*********************************************************************
 *             _stat (MSVCRT.@)
 */
int CDECL _stat(const char* path, struct _stat * buf)
{
  int ret;
  struct _stat64 buf64;

  ret = _stat64( path, &buf64);
  if (!ret)
      msvcrt_stat64_to_stat(&buf64, buf);
  return ret;
}

#if _MSVCR_VER >= 80

/*********************************************************************
 *  _stat32 (MSVCR80.@)
 */
int CDECL _stat32(const char *path, struct _stat32 *buf)
{
    int ret;
    struct _stat64 buf64;

    ret = _stat64(path, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat32(&buf64, buf);
    return ret;
}

/*********************************************************************
 *  _stat32i64 (MSVCR80.@)
 */
int CDECL _stat32i64(const char *path, struct _stat32i64 *buf)
{
    int ret;
    struct _stat64 buf64;

    ret = _stat64(path, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat32i64(&buf64, buf);
    return ret;
}

/*********************************************************************
 * _stat64i32 (MSVCR80.@)
 */
int CDECL _stat64i32(const char* path, struct _stat64i32 *buf)
{
    int ret;
    struct _stat64 buf64;

    ret = _stat64(path, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat64i32(&buf64, buf);
    return ret;
}

#endif /* _MSVCR_VER >= 80 */

/*********************************************************************
 *		_wstat64 (MSVCRT.@)
 */
int CDECL _wstat64(const wchar_t* path, struct _stat64 * buf)
{
  DWORD dw;
  WIN32_FILE_ATTRIBUTE_DATA hfi;
  unsigned short mode = ALL_S_IREAD;
  int plen;

  TRACE(":file (%s) buf(%p)\n", debugstr_w(path), buf);

  plen = wcslen(path);
  while (plen && path[plen-1]==' ')
    plen--;

  if (plen==2 && path[1]==':')
  {
    *_errno() = ENOENT;
    return -1;
  }

#if _MSVCR_VER<140
  if (plen>=2 && path[plen-2]!=':' && (path[plen-1]=='\\' || path[plen-1]=='/'))
  {
    *_errno() = ENOENT;
    return -1;
  }
#endif

  if (!GetFileAttributesExW(path, GetFileExInfoStandard, &hfi))
  {
      TRACE("failed (%ld)\n", GetLastError());
      *_errno() = ENOENT;
      return -1;
  }

  memset(buf,0,sizeof(struct _stat64));

  /* FIXME: rdev isn't drive num, despite what the docs says-what is it? */
  if (iswalpha(*path) && path[1] == ':')
    buf->st_dev = buf->st_rdev = towupper(*path) - 'A'; /* drive num */
  else
    buf->st_dev = buf->st_rdev = _getdrive() - 1;

  /* Dir, or regular file? */
  if (hfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    mode |= (_S_IFDIR | ALL_S_IEXEC);
  else
  {
    mode |= _S_IFREG;
    /* executable? */
    if (plen > 6 && path[plen-4] == '.')  /* shortest exe: "\x.exe" */
    {
      ULONGLONG ext = towlower(path[plen-1]) | (towlower(path[plen-2]) << 16) |
                               ((ULONGLONG)towlower(path[plen-3]) << 32);
      if (ext == WCEXE || ext == WCBAT || ext == WCCMD || ext == WCCOM)
        mode |= ALL_S_IEXEC;
    }
  }

  if (!(hfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    mode |= ALL_S_IWRITE;

  buf->st_mode  = mode;
  buf->st_nlink = 1;
  buf->st_size  = ((__int64)hfi.nFileSizeHigh << 32) + hfi.nFileSizeLow;
  RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastAccessTime, &dw);
  buf->st_atime = dw;
  RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastWriteTime, &dw);
  buf->st_mtime = buf->st_ctime = dw;
  TRACE("%d %d %#I64x %I64d %I64d %I64d\n", buf->st_mode, buf->st_nlink,
          buf->st_size, buf->st_atime, buf->st_mtime, buf->st_ctime);
  return 0;
}

/*********************************************************************
 *		_wstati64 (MSVCRT.@)
 */
int CDECL _wstati64(const wchar_t* path, struct _stati64 * buf)
{
  int ret;
  struct _stat64 buf64;

  ret = _wstat64(path, &buf64);
  if (!ret)
    msvcrt_stat64_to_stati64(&buf64, buf);
  return ret;
}

/*********************************************************************
 *             _wstat (MSVCRT.@)
 */
int CDECL _wstat(const wchar_t* path, struct _stat * buf)
{
  int ret;
  struct _stat64 buf64;

  ret = _wstat64( path, &buf64 );
  if (!ret) msvcrt_stat64_to_stat(&buf64, buf);
  return ret;
}

#if _MSVCR_VER >= 80

/*********************************************************************
 *  _wstat32 (MSVCR80.@)
 */
int CDECL _wstat32(const wchar_t *path, struct _stat32 *buf)
{
    int ret;
    struct _stat64 buf64;

    ret = _wstat64(path, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat32(&buf64, buf);
    return ret;
}

/*********************************************************************
 *  _wstat32i64 (MSVCR80.@)
 */
int CDECL _wstat32i64(const wchar_t *path, struct _stat32i64 *buf)
{
    int ret;
    struct _stat64 buf64;

    ret = _wstat64(path, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat32i64(&buf64, buf);
    return ret;
}

/*********************************************************************
 * _wstat64i32 (MSVCR80.@)
 */
int CDECL _wstat64i32(const wchar_t *path, struct _stat64i32 *buf)
{
    int ret;
    struct _stat64 buf64;

    ret = _wstat64(path, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat64i32(&buf64, buf);
    return ret;
}

#endif /* _MSVCR_VER >= 80 */

/*********************************************************************
 *		_tell (MSVCRT.@)
 */
__msvcrt_long CDECL _tell(int fd)
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
    wchar_t *dirW = NULL, *prefixW = NULL, *retW;
    char *ret;

    if (dir && !(dirW = wstrdupa_utf8(dir))) return NULL;
    if (prefix && !(prefixW = wstrdupa_utf8(prefix)))
    {
        free(dirW);
        return NULL;
    }
    retW = _wtempnam(dirW, prefixW);
    free(dirW);
    free(prefixW);
    /* TODO: don't do the conversion */
    ret = astrdupw_utf8(retW);
    free(retW);
    return ret;
}

/*********************************************************************
 *		_wtempnam (MSVCRT.@)
 */
wchar_t * CDECL _wtempnam(const wchar_t *dir, const wchar_t *prefix)
{
  wchar_t tmpbuf[MAX_PATH];
  const wchar_t *tmp_dir = _wgetenv(L"TMP");

  if (tmp_dir) dir = tmp_dir;

  TRACE("dir (%s) prefix (%s)\n", debugstr_w(dir), debugstr_w(prefix));
  /* TODO: use whole prefix */
  if (GetTempFileNameW(dir,prefix,0,tmpbuf))
  {
    TRACE("got name (%s)\n", debugstr_w(tmpbuf));
    DeleteFileW(tmpbuf);
    return _wcsdup(tmpbuf);
  }
  TRACE("failed (%ld)\n", GetLastError());
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
 *		_utime64 (MSVCRT.@)
 */
int CDECL _utime64(const char* path, struct __utimbuf64 *t)
{
  int fd = _open(path, _O_WRONLY | _O_BINARY);

  if (fd > 0)
  {
    int retVal = _futime64(fd, t);
    _close(fd);
    return retVal;
  }
  return -1;
}

/*********************************************************************
 *		_utime32 (MSVCRT.@)
 */
int CDECL _utime32(const char* path, struct __utimbuf32 *t)
{
    if (t)
    {
        struct __utimbuf64 t64;
        t64.actime = t->actime;
        t64.modtime = t->modtime;
        return _utime64( path, &t64 );
    }
    else
        return _utime64( path, NULL );
}

/*********************************************************************
 *		_wutime64 (MSVCRT.@)
 */
int CDECL _wutime64(const wchar_t* path, struct __utimbuf64 *t)
{
  int fd = _wopen(path, _O_WRONLY | _O_BINARY);

  if (fd > 0)
  {
    int retVal = _futime64(fd, t);
    _close(fd);
    return retVal;
  }
  return -1;
}

/*********************************************************************
 *		_wutime32 (MSVCRT.@)
 */
int CDECL _wutime32(const wchar_t* path, struct __utimbuf32 *t)
{
    if (t)
    {
        struct __utimbuf64 t64;
        t64.actime = t->actime;
        t64.modtime = t->modtime;
        return _wutime64( path, &t64 );
    }
    else
        return _wutime64( path, NULL );
}

/*********************************************************************
 *		_write (MSVCRT.@)
 */
int CDECL _write(int fd, const void* buf, unsigned int count)
{
    ioinfo *info = get_ioinfo(fd);
    HANDLE hand = info->handle;
    DWORD num_written, i;
    BOOL console = FALSE;

    if (hand == INVALID_HANDLE_VALUE || fd == MSVCRT_NO_CONSOLE_FD)
    {
        *_errno() = EBADF;
        release_ioinfo(info);
        return -1;
    }

    if (ioinfo_get_textmode(info) != TEXTMODE_ANSI && count&1)
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
        if (!WriteFile(hand, buf, count, &num_written, NULL)
                ||  num_written != count)
        {
            TRACE("WriteFile (fd %d, hand %p) failed-last error (%ld)\n", fd,
                    hand, GetLastError());
            msvcrt_set_errno(GetLastError());
            if (GetLastError() == ERROR_ACCESS_DENIED)
                *_errno() = EBADF;
            num_written = -1;
        }

        release_ioinfo(info);
        return num_written;
    }

    if (_isatty(fd)) console = VerifyConsoleIoHandle(hand);
    for (i = 0; i < count;)
    {
        const char *s = buf;
        char lfbuf[2048];
        DWORD j = 0;

        if (ioinfo_get_textmode(info) == TEXTMODE_ANSI && console)
        {
            char conv[sizeof(lfbuf)];
            size_t len = 0;

#if _MSVCR_VER >= 80
            if (info->dbcsBufferUsed)
            {
                conv[j++] = info->dbcsBuffer[0];
                info->dbcsBufferUsed = FALSE;
                conv[j++] = s[i++];
                len++;
            }
#endif

            for (; i < count && j < sizeof(conv)-1 &&
                    len < (sizeof(lfbuf) - 1) / sizeof(WCHAR); i++, j++, len++)
            {
                if (isleadbyte((unsigned char)s[i]))
                {
                    conv[j++] = s[i++];

                    if (i == count)
                    {
#if _MSVCR_VER >= 80
                        info->dbcsBuffer[0] = conv[j-1];
                        info->dbcsBufferUsed = TRUE;
                        break;
#else
                        *_errno() = EINVAL;
                        release_ioinfo(info);
                        return -1;
#endif
                    }
                }
                else if (s[i] == '\n')
                {
                    conv[j++] = '\r';
                    len++;
                }
                conv[j] = s[i];
            }

            len = mbstowcs((WCHAR*)lfbuf, conv, len);
            if (len == -1)
            {
                msvcrt_set_errno(GetLastError());
                release_ioinfo(info);
                return -1;
            }
            j = len * 2;
        }
        else if (ioinfo_get_textmode(info) == TEXTMODE_ANSI)
        {
            for (j = 0; i < count && j < sizeof(lfbuf)-1; i++, j++)
            {
                if (s[i] == '\n')
                    lfbuf[j++] = '\r';
                lfbuf[j] = s[i];
            }
        }
        else if (ioinfo_get_textmode(info) == TEXTMODE_UTF16LE || console)
        {
            for (j = 0; i < count && j < sizeof(lfbuf)-3; i++, j++)
            {
                if (s[i] == '\n' && !s[i+1])
                {
                    lfbuf[j++] = '\r';
                    lfbuf[j++] = 0;
                }
                lfbuf[j++] = s[i++];
                lfbuf[j] = s[i];
            }
        }
        else
        {
            char conv[sizeof(lfbuf)/4];

            for (j = 0; i < count && j < sizeof(conv)-3; i++, j++)
            {
                if (s[i] == '\n' && !s[i+1])
                {
                    conv[j++] = '\r';
                    conv[j++] = 0;
                }
                conv[j++] = s[i++];
                conv[j] = s[i];
            }

            j = WideCharToMultiByte(CP_UTF8, 0, (WCHAR*)conv, j/2, lfbuf, sizeof(lfbuf), NULL, NULL);
            if (!j)
            {
                msvcrt_set_errno(GetLastError());
                release_ioinfo(info);
                return -1;
            }
        }

        if (console)
        {
            j = j/2;
            if (!WriteConsoleW(hand, lfbuf, j, &num_written, NULL))
                num_written = -1;
        }
        else if (!WriteFile(hand, lfbuf, j, &num_written, NULL))
        {
            num_written = -1;
        }

        if (num_written != j)
        {
            TRACE("WriteFile/WriteConsoleW (fd %d, hand %p) failed-last error (%ld)\n", fd,
                    hand, GetLastError());
            msvcrt_set_errno(GetLastError());
            if (GetLastError() == ERROR_ACCESS_DENIED)
                *_errno() = EBADF;
            release_ioinfo(info);
            return -1;
        }
    }

    release_ioinfo(info);
    return count;
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
  int ret;

  if (!MSVCRT_CHECK_PMT(file != NULL)) return EOF;

  _lock_file(file);
  ret = _fclose_nolock(file);
  _unlock_file(file);

  return ret;
}

/*********************************************************************
 *		_fclose_nolock (MSVCRT.@)
 */
int CDECL _fclose_nolock(FILE* file)
{
  int r, flag;

  if (!MSVCRT_CHECK_PMT(file != NULL)) return EOF;

  if(!(file->_flag & (_IOREAD | _IOWRT | _IORW)))
  {
      file->_flag = 0;
      return EOF;
  }

  flag = file->_flag;
  free(file->_tmpfname);
  file->_tmpfname = NULL;
  /* flush stdio buffers */
  if(file->_flag & _IOWRT)
      _fflush_nolock(file);
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
    unsigned char c;

    if(file->_flag & _IOSTRG)
        return EOF;

    /* Allocate buffer if needed */
    if(!(file->_flag & (MSVCRT__NOBUF | _IOMYBUF | MSVCRT__USERBUF)))
        msvcrt_alloc_buffer(file);

    if(!(file->_flag & _IOREAD)) {
        if(file->_flag & _IORW)
            file->_flag |= _IOREAD;
        else
            return EOF;
    }

    if(!(file->_flag & (_IOMYBUF | MSVCRT__USERBUF))) {
        int r;
        if ((r = _read(file->_file,&c,1)) != 1) {
            file->_flag |= (r == 0) ? _IOEOF : _IOERR;
            return EOF;
        }

        return c;
    } else {
        file->_cnt = _read(file->_file, file->_base, file->_bufsiz);
        if(file->_cnt<=0) {
            file->_flag |= (file->_cnt == 0) ? _IOEOF : _IOERR;
            file->_cnt = 0;
            return EOF;
        }

        file->_cnt--;
        file->_ptr = file->_base+1;
        c = *(unsigned char *)file->_base;
        return c;
    }
}

/*********************************************************************
 *		fgetc (MSVCRT.@)
 */
int CDECL fgetc(FILE* file)
{
    int ret;

    _lock_file(file);
    ret = _fgetc_nolock(file);
    _unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_fgetc_nolock (MSVCRT.@)
 */
int CDECL _fgetc_nolock(FILE* file)
{
  unsigned char *i;
  unsigned int j;

  if (file->_cnt>0) {
    file->_cnt--;
    i = (unsigned char *)file->_ptr++;
    j = *i;
  } else
    j = _filbuf(file);

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

  while ((size >1) && (cc = _fgetc_nolock(file)) != EOF && cc != '\n')
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

    _lock_file(file);
    ret = _fgetwc_nolock(file);
    _unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_fgetwc_nolock (MSVCRT.@)
 */
wint_t CDECL _fgetwc_nolock(FILE* file)
{
    wint_t ret;
    int ch;

    if(ioinfo_get_textmode(get_ioinfo_nolock(file->_file)) != TEXTMODE_ANSI
            || !(get_ioinfo_nolock(file->_file)->wxflag & WX_TEXT)) {
        char *p;

        for(p=(char*)&ret; (wint_t*)p<&ret+1; p++) {
            ch = _fgetc_nolock(file);
            if(ch == EOF) {
                ret = WEOF;
                break;
            }
            *p = (char)ch;
        }
    }else {
        char mbs[MB_LEN_MAX];
        int len = 0;

        ch = _fgetc_nolock(file);
        if(ch != EOF) {
            mbs[0] = (char)ch;
            if(isleadbyte((unsigned char)mbs[0])) {
                ch = _fgetc_nolock(file);
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
    k = _fgetc_nolock(file);
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
  wint_t cc = WEOF;
  wchar_t * buf_start = s;

  TRACE(":file(%p) fd (%d) str (%p) len (%d)\n",
        file,file->_file,s,size);

  _lock_file(file);

  while ((size >1) && (cc = _fgetwc_nolock(file)) != WEOF && cc != '\n')
    {
      *s++ = cc;
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
 *		_flsbuf (MSVCRT.@)
 */
int CDECL _flsbuf(int c, FILE* file)
{
    /* Flush output buffer */
    if(!(file->_flag & (MSVCRT__NOBUF | _IOMYBUF | MSVCRT__USERBUF))) {
        msvcrt_alloc_buffer(file);
    }

    if(!(file->_flag & _IOWRT)) {
        if(!(file->_flag & _IORW)) {
            file->_flag |= _IOERR;
            *_errno() = EBADF;
            return EOF;
        }
        file->_flag |= _IOWRT;
    }
    if(file->_flag & _IOREAD) {
        if(!(file->_flag & _IOEOF)) {
            file->_flag |= _IOERR;
            return EOF;
        }
        file->_cnt = 0;
        file->_ptr = file->_base;
        file->_flag &= ~(_IOREAD | _IOEOF);
    }

    if(file->_flag & (_IOMYBUF | MSVCRT__USERBUF)) {
        int res = 0;

        if(file->_cnt <= 0) {
            res = msvcrt_flush_buffer(file);
            if(res)
                return res;
            file->_flag |= _IOWRT;
            file->_cnt=file->_bufsiz;
        }
        *file->_ptr++ = c;
        file->_cnt--;
        return c&0xff;
    } else {
        unsigned char cc=c;
        int len;
        /* set _cnt to 0 for unbuffered FILEs */
        file->_cnt = 0;
        len = _write(file->_file, &cc, 1);
        if (len == 1)
            return c & 0xff;
        file->_flag |= _IOERR;
        return EOF;
    }
}

/*********************************************************************
 *		fwrite (MSVCRT.@)
 */
size_t CDECL fwrite(const void *ptr, size_t size, size_t nmemb, FILE* file)
{
    size_t ret;

    _lock_file(file);
    ret = _fwrite_nolock(ptr, size, nmemb, file);
    _unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_fwrite_nolock (MSVCRT.@)
 */
size_t CDECL _fwrite_nolock(const void *ptr, size_t size, size_t nmemb, FILE* file)
{
    size_t wrcnt=size * nmemb;
    int written = 0;
    if (size == 0)
        return 0;

    while(wrcnt) {
        if(file->_cnt < 0) {
            WARN("negative file->_cnt value in %p\n", file);
            file->_flag |= _IOERR;
            break;
        } else if(file->_cnt) {
            int pcnt=(file->_cnt>wrcnt)? wrcnt: file->_cnt;
            memcpy(file->_ptr, ptr, pcnt);
            file->_cnt -= pcnt;
            file->_ptr += pcnt;
            written += pcnt;
            wrcnt -= pcnt;
            ptr = (const char*)ptr + pcnt;
        } else if((file->_flag & MSVCRT__NOBUF)
                || ((file->_flag & (_IOMYBUF | MSVCRT__USERBUF)) && wrcnt >= file->_bufsiz)
                || (!(file->_flag & (_IOMYBUF | MSVCRT__USERBUF)) && wrcnt >= MSVCRT_INTERNAL_BUFSIZ)) {
            size_t pcnt;
            int bufsiz;

            if(file->_flag & MSVCRT__NOBUF)
                bufsiz = 1;
            else if(!(file->_flag & (_IOMYBUF | MSVCRT__USERBUF)))
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

    return written / size;
}

/*********************************************************************
 *		fputwc (MSVCRT.@)
 */
wint_t CDECL fputwc(wint_t wc, FILE* file)
{
    wint_t ret;

    _lock_file(file);
    ret = _fputwc_nolock(wc, file);
    _unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_fputwc_nolock (MSVCRT.@)
 */
wint_t CDECL _fputwc_nolock(wint_t wc, FILE* file)
{
    wchar_t mwc=wc;
    ioinfo *fdinfo;
    wint_t ret;

    fdinfo = get_ioinfo_nolock(file->_file);

    if((fdinfo->wxflag&WX_TEXT) && ioinfo_get_textmode(fdinfo) == TEXTMODE_ANSI) {
        char buf[MB_LEN_MAX];
        int char_len;

        char_len = wctomb(buf, mwc);
        if(char_len!=-1 && _fwrite_nolock(buf, char_len, 1, file)==1)
            ret = wc;
        else
            ret = WEOF;
    }else if(_fwrite_nolock(&mwc, sizeof(mwc), 1, file) == 1) {
        ret = wc;
    }else {
        ret = WEOF;
    }

    return ret;
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
    wchar_t *pathW = NULL, *modeW = NULL;
    FILE *ret;

    if (path && !(pathW = wstrdupa_utf8(path))) return NULL;
    if (mode && !(modeW = wstrdupa_utf8(mode)))
    {
        free(pathW);
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
    if (!MSVCRT_CHECK_PMT(pFile != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(filename != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(mode != NULL)) return EINVAL;

    *pFile = _wfopen(filename, mode);

    if(!*pFile)
        return *_errno();
    return 0;
}

/*********************************************************************
 *		fputc (MSVCRT.@)
 */
int CDECL fputc(int c, FILE* file)
{
    int ret;

    _lock_file(file);
    ret = _fputc_nolock(c, file);
    _unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_fputc_nolock (MSVCRT.@)
 */
int CDECL _fputc_nolock(int c, FILE* file)
{
  int res;

  if(file->_cnt>0) {
    *file->_ptr++=c;
    file->_cnt--;
    if (c == '\n')
    {
      res = msvcrt_flush_buffer(file);
      return res ? res : c;
    }
    else {
      return c & 0xff;
    }
  } else {
    res = _flsbuf(c, file);
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
    size_t ret;

    _lock_file(file);
    ret = _fread_nolock(ptr, size, nmemb, file);
    _unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_fread_nolock (MSVCRT.@)
 */
size_t CDECL _fread_nolock(void *ptr, size_t size, size_t nmemb, FILE* file)
{
  size_t rcnt=size * nmemb;
  size_t read=0;
  size_t pread=0;

  if(!rcnt)
	return 0;

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
        return 0;
    }
  }

  if(rcnt>0 && !(file->_flag & (MSVCRT__NOBUF | _IOMYBUF | MSVCRT__USERBUF)))
      msvcrt_alloc_buffer(file);

  while(rcnt>0)
  {
    int i;
    if (!file->_cnt && rcnt<file->_bufsiz && (file->_flag & (_IOMYBUF | MSVCRT__USERBUF))) {
      i = _read(file->_file, file->_base, file->_bufsiz);
      file->_ptr = file->_base;
      if (i != -1) {
          file->_cnt = i;
          if (i > rcnt) i = rcnt;
      }
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
    } else if (rcnt < (file->_bufsiz ? file->_bufsiz : MSVCRT_INTERNAL_BUFSIZ)) {
      i = _read(file->_file, ptr, rcnt);
    } else {
      i = _read(file->_file, ptr, rcnt - rcnt % (file->_bufsiz ? file->_bufsiz : MSVCRT_INTERNAL_BUFSIZ));
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
  return read / size;
}

#if _MSVCR_VER >= 80

/*********************************************************************
 *		fread_s (MSVCR80.@)
 */
size_t CDECL fread_s(void *buf, size_t buf_size, size_t elem_size,
        size_t count, FILE *stream)
{
    size_t ret;

    if(!MSVCRT_CHECK_PMT(stream != NULL)) {
        if(buf && buf_size)
            memset(buf, 0, buf_size);
        return 0;
    }
    if(!elem_size || !count) return 0;

    _lock_file(stream);
    ret = _fread_nolock_s(buf, buf_size, elem_size, count, stream);
    _unlock_file(stream);

    return ret;
}

/*********************************************************************
 *		_fread_nolock_s (MSVCR80.@)
 */
size_t CDECL _fread_nolock_s(void *buf, size_t buf_size, size_t elem_size,
        size_t count, FILE *stream)
{
    size_t bytes_left, buf_pos;

    TRACE("(%p %Iu %Iu %Iu %p)\n", buf, buf_size, elem_size, count, stream);

    if(!MSVCRT_CHECK_PMT(stream != NULL)) {
        if(buf && buf_size)
            memset(buf, 0, buf_size);
        return 0;
    }
    if(!elem_size || !count) return 0;
    if(!MSVCRT_CHECK_PMT(buf != NULL)) return 0;
    if(!MSVCRT_CHECK_PMT(SIZE_MAX/count >= elem_size)) return 0;

    bytes_left = elem_size*count;
    buf_pos = 0;
    while(bytes_left) {
        if(stream->_cnt > 0) {
            size_t size = bytes_left<stream->_cnt ? bytes_left : stream->_cnt;

            if(!MSVCRT_CHECK_PMT_ERR(size <= buf_size-buf_pos, ERANGE)) {
                memset(buf, 0, buf_size);
                return 0;
            }

            _fread_nolock((char*)buf+buf_pos, 1, size, stream);
            buf_pos += size;
            bytes_left -= size;
        }else {
            int c = _filbuf(stream);

            if(c == EOF)
                break;

            if(!MSVCRT_CHECK_PMT_ERR(buf_size != buf_pos, ERANGE)) {
                memset(buf, 0, buf_size);
                return 0;
            }

            ((char*)buf)[buf_pos++] = c;
            bytes_left--;
        }
    }

    return buf_pos/elem_size;
}

#endif /* _MSVCR_VER >= 80 */

/*********************************************************************
 *		_wfreopen (MSVCRT.@)
 *
 */
FILE* CDECL _wfreopen(const wchar_t *path, const wchar_t *mode, FILE* file)
{
    int open_flags, stream_flags, fd;

    TRACE(":path (%s) mode (%s) file (%p) fd (%d)\n", debugstr_w(path), debugstr_w(mode), file, file ? file->_file : -1);

    LOCK_FILES();
    if (file)
    {
        fclose(file);
        if (msvcrt_get_flags(mode, &open_flags, &stream_flags) == -1)
            file = NULL;
        else if((fd = _wopen(path, open_flags, _S_IREAD | _S_IWRITE)) < 0)
            file = NULL;
        else if(msvcrt_init_fp(file, fd, stream_flags) == -1)
        {
            file->_flag = 0;
            file = NULL;
        }
    }
    UNLOCK_FILES();
    return file;
}

/*********************************************************************
 *      _wfreopen_s (MSVCRT.@)
 */
int CDECL _wfreopen_s(FILE** pFile,
        const wchar_t *path, const wchar_t *mode, FILE* file)
{
    if (!MSVCRT_CHECK_PMT(pFile != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(path != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(mode != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(file != NULL)) return EINVAL;

    *pFile = _wfreopen(path, mode, file);

    if(!*pFile)
        return *_errno();
    return 0;
}

/*********************************************************************
 *      freopen (MSVCRT.@)
 *
 */
FILE* CDECL freopen(const char *path, const char *mode, FILE* file)
{
    FILE *ret;
    wchar_t *pathW = NULL, *modeW = NULL;

    if (path && !(pathW = wstrdupa_utf8(path))) return NULL;
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
 *      freopen_s (MSVCRT.@)
 */
errno_t CDECL freopen_s(FILE** pFile,
        const char *path, const char *mode, FILE* file)
{
    if (!MSVCRT_CHECK_PMT(pFile != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(path != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(mode != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(file != NULL)) return EINVAL;

    *pFile = freopen(path, mode, file);

    if(!*pFile)
        return *_errno();
    return 0;
}

/*********************************************************************
 *		fsetpos (MSVCRT.@)
 */
int CDECL fsetpos(FILE* file, fpos_t *pos)
{
    return _fseeki64(file,*pos,SEEK_SET);
}

/*********************************************************************
 *		_ftelli64 (MSVCRT.@)
 */
__int64 CDECL _ftelli64(FILE* file)
{
    __int64 ret;

    _lock_file(file);
    ret = _ftelli64_nolock(file);
    _unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_ftelli64_nolock (MSVCRT.@)
 */
__int64 CDECL _ftelli64_nolock(FILE* file)
{
    __int64 pos;

    pos = _telli64(file->_file);
    if(pos == -1)
        return -1;
    if(file->_flag & (_IOMYBUF | MSVCRT__USERBUF))  {
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

            if(_lseeki64(file->_file, pos, SEEK_SET) != pos)
                return -1;

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

    return pos;
}

/*********************************************************************
 *		ftell (MSVCRT.@)
 */
__msvcrt_long CDECL ftell(FILE* file)
{
  return _ftelli64(file);
}

#if _MSVCR_VER >= 80
/*********************************************************************
 *		_ftell_nolock (MSVCR80.@)
 */
__msvcrt_long CDECL _ftell_nolock(FILE* file)
{
  return _ftelli64_nolock(file);
}
#endif

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
    ret = _fwrite_nolock(s, sizeof(*s), len, file) == len ? 0 : EOF;
    _unlock_file(file);
    return ret;
}

/*********************************************************************
 *		fputws (MSVCRT.@)
 */
int CDECL fputws(const wchar_t *s, FILE* file)
{
    size_t i, len = wcslen(s);
    BOOL tmp_buf;
    int ret;

    _lock_file(file);
    if (!(get_ioinfo_nolock(file->_file)->wxflag & WX_TEXT)) {
        ret = _fwrite_nolock(s,sizeof(*s),len,file) == len ? 0 : EOF;
        _unlock_file(file);
        return ret;
    }

    tmp_buf = add_std_buffer(file);
    for (i=0; i<len; i++) {
        if(_fputwc_nolock(s[i], file) == WEOF) {
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
 *		gets_s (MSVCR80.@)
 */
char * CDECL gets_s(char *buf, size_t len)
{
    char *buf_start = buf;
    int cc;

    if (!MSVCRT_CHECK_PMT(buf != NULL)) return NULL;
    if (!MSVCRT_CHECK_PMT(len != 0)) return NULL;

    _lock_file(stdin);
    for(cc = _fgetc_nolock(stdin);
            len != 0 && cc != EOF && cc != '\n';
            cc = _fgetc_nolock(stdin))
    {
        if (cc != '\r')
        {
            *buf++ = (char)cc;
            len--;
        }
    }
    _unlock_file(stdin);

    if (!len)
    {
        *buf_start = 0;
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        return NULL;
    }

    if ((cc == EOF) && (buf_start == buf))
    {
        TRACE(":nothing read\n");
        return NULL;
    }
    *buf = '\0';

    TRACE("got '%s'\n", buf_start);
    return buf_start;
}

/*********************************************************************
 *              gets (MSVCRT.@)
 */
char * CDECL gets(char *buf)
{
    return gets_s(buf, -1);
}

/*********************************************************************
 *		_getws (MSVCRT.@)
 */
wchar_t* CDECL _getws(wchar_t* buf)
{
    wint_t cc;
    wchar_t* ws = buf;

    _lock_file(stdin);
    for (cc = _fgetwc_nolock(stdin); cc != WEOF && cc != '\n';
         cc = _fgetwc_nolock(stdin))
    {
        if (cc != '\r')
            *buf++ = (wchar_t)cc;
    }
    _unlock_file(stdin);

    if ((cc == WEOF) && (ws == buf))
    {
      TRACE(":nothing read\n");
      return NULL;
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
    int ret;

    _lock_file(stdout);
    if(_fwrite_nolock(s, sizeof(*s), len, stdout) != len) {
        _unlock_file(stdout);
        return EOF;
    }

    ret = _fwrite_nolock("\n",1,1,stdout) == 1 ? 0 : EOF;
    _unlock_file(stdout);
    return ret;
}

/*********************************************************************
 *		_putws (MSVCRT.@)
 */
int CDECL _putws(const wchar_t *s)
{
    int ret;

    _lock_file(stdout);
    ret = fputws(s, stdout);
    if(ret >= 0)
        ret = _fputwc_nolock('\n', stdout);
    _unlock_file(stdout);
    return ret >= 0 ? 0 : WEOF;
}

/*********************************************************************
 *		remove (MSVCRT.@)
 */
int CDECL remove(const char *path)
{
    return _unlink(path);
}

/*********************************************************************
 *		_wremove (MSVCRT.@)
 */
int CDECL _wremove(const wchar_t *path)
{
    return _wunlink(path);
}

/*********************************************************************
 *		rename (MSVCRT.@)
 */
int CDECL rename(const char *oldpath,const char *newpath)
{
    wchar_t *oldpathW = NULL, *newpathW = NULL;
    int ret;

    if (oldpath && !(oldpathW = wstrdupa_utf8(oldpath))) return -1;
    if (newpath && !(newpathW = wstrdupa_utf8(newpath)))
    {
        free(oldpathW);
        return -1;
    }
    ret = _wrename(oldpathW, newpathW);
    free(oldpathW);
    free(newpathW);
    return ret;
}

/*********************************************************************
 *		_wrename (MSVCRT.@)
 */
int CDECL _wrename(const wchar_t *oldpath,const wchar_t *newpath)
{
  TRACE(":from %s to %s\n", debugstr_w(oldpath), debugstr_w(newpath));
  if (MoveFileExW(oldpath, newpath, MOVEFILE_COPY_ALLOWED))
    return 0;
  TRACE(":failed (%ld)\n", GetLastError());
  msvcrt_set_errno(GetLastError());
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

    _fflush_nolock(file);
    if(file->_flag & _IOMYBUF)
        free(file->_base);
    file->_flag &= ~(MSVCRT__NOBUF | _IOMYBUF | MSVCRT__USERBUF);
    file->_cnt = 0;

    if(mode == _IONBF) {
        file->_flag |= MSVCRT__NOBUF;
        file->_base = file->_ptr = (char*)&file->_charbuf;
        file->_bufsiz = 2;
    }else if(buf) {
        file->_base = file->_ptr = buf;
        file->_flag |= MSVCRT__USERBUF;
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

static int tmpnam_helper(char *s, size_t size, LONG *tmpnam_unique, int tmp_max)
{
    char tmpstr[8];
    char *p = s;
    int digits;

    if (!MSVCRT_CHECK_PMT(s != NULL)) return EINVAL;

    if (size < 3) {
        if (size) *s = 0;
        *_errno() = ERANGE;
        return ERANGE;
    }
    *p++ = '\\';
    *p++ = 's';
    size -= 2;
    digits = msvcrt_int_to_base32(GetCurrentProcessId(), tmpstr);
    if (digits+1 > size) {
        *s = 0;
        *_errno() = ERANGE;
        return ERANGE;
    }
    memcpy(p, tmpstr, digits*sizeof(tmpstr[0]));
    p += digits;
    *p++ = '.';
    size -= digits+1;

    while(1) {
        while ((digits = *tmpnam_unique)+1 < tmp_max) {
            if (InterlockedCompareExchange(tmpnam_unique, digits+1, digits) == digits)
                break;
        }

        digits = msvcrt_int_to_base32(digits, tmpstr);
        if (digits+1 > size) {
            *s = 0;
            *_errno() = ERANGE;
            return ERANGE;
        }
        memcpy(p, tmpstr, digits*sizeof(tmpstr[0]));
        p[digits] = 0;

        if (GetFileAttributesA(s) == INVALID_FILE_ATTRIBUTES &&
                GetLastError() == ERROR_FILE_NOT_FOUND)
            break;
    }
    return 0;
}

int CDECL tmpnam_s(char *s, size_t size)
{
    return tmpnam_helper(s, size, &tmpnam_s_unique, TMP_MAX_S);
}

/*********************************************************************
 *		tmpnam (MSVCRT.@)
 */
char * CDECL tmpnam(char *s)
{
  if (!s) {
    thread_data_t *data = msvcrt_get_thread_data();

    if(!data->tmpnam_buffer)
      data->tmpnam_buffer = malloc(MAX_PATH);

    s = data->tmpnam_buffer;
  }

  return tmpnam_helper(s, -1, &tmpnam_unique, TMP_MAX) ? NULL : s;
}

static int wtmpnam_helper(wchar_t *s, size_t size, LONG *tmpnam_unique, int tmp_max)
{
    wchar_t tmpstr[8];
    wchar_t *p = s;
    int digits;

    if (!MSVCRT_CHECK_PMT(s != NULL)) return EINVAL;

    if (size < 3) {
        if (size) *s = 0;
        *_errno() = ERANGE;
        return ERANGE;
    }
    *p++ = '\\';
    *p++ = 's';
    size -= 2;
    digits = msvcrt_int_to_base32_w(GetCurrentProcessId(), tmpstr);
    if (digits+1 > size) {
        *s = 0;
        *_errno() = ERANGE;
        return ERANGE;
    }
    memcpy(p, tmpstr, digits*sizeof(tmpstr[0]));
    p += digits;
    *p++ = '.';
    size -= digits+1;

    while(1) {
        while ((digits = *tmpnam_unique)+1 < tmp_max) {
            if (InterlockedCompareExchange(tmpnam_unique, digits+1, digits) == digits)
                break;
        }

        digits = msvcrt_int_to_base32_w(digits, tmpstr);
        if (digits+1 > size) {
            *s = 0;
            *_errno() = ERANGE;
            return ERANGE;
        }
        memcpy(p, tmpstr, digits*sizeof(tmpstr[0]));
        p[digits] = 0;

        if (GetFileAttributesW(s) == INVALID_FILE_ATTRIBUTES &&
                GetLastError() == ERROR_FILE_NOT_FOUND)
            break;
    }
    return 0;
}

/*********************************************************************
 *              _wtmpnam_s (MSVCRT.@)
 */
int CDECL _wtmpnam_s(wchar_t *s, size_t size)
{
    return wtmpnam_helper(s, size, &tmpnam_s_unique, TMP_MAX_S);
}

/*********************************************************************
 *              _wtmpnam (MSVCRT.@)
 */
wchar_t * CDECL _wtmpnam(wchar_t *s)
{
    if (!s) {
        thread_data_t *data = msvcrt_get_thread_data();

        if(!data->wtmpnam_buffer)
            data->wtmpnam_buffer = malloc(sizeof(wchar_t[MAX_PATH]));

        s = data->wtmpnam_buffer;
    }

    return wtmpnam_helper(s, -1, &tmpnam_unique, TMP_MAX) ? NULL : s;
}

/*********************************************************************
 *		tmpfile (MSVCRT.@)
 */
FILE* CDECL tmpfile(void)
{
  char *filename = _tempnam(",", "t");
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
  free(filename);
  UNLOCK_FILES();
  return file;
}

/*********************************************************************
 *      tmpfile_s (MSVCRT.@)
 */
int CDECL tmpfile_s(FILE** file)
{
    if (!MSVCRT_CHECK_PMT(file != NULL)) return EINVAL;

    *file = tmpfile();
    return 0;
}

static int puts_clbk_file_a(void *file, int len, const char *str)
{
    return fwrite(str, sizeof(char), len, file);
}

static int puts_clbk_file_w(void *file, int len, const wchar_t *str)
{
    int i, ret;

    _lock_file(file);

    if(!(get_ioinfo_nolock(((FILE*)file)->_file)->wxflag & WX_TEXT)) {
        ret = _fwrite_nolock(str, sizeof(wchar_t), len, file);
        _unlock_file(file);
        return ret;
    }

    for(i=0; i<len; i++) {
        if(_fputwc_nolock(str[i], file) == WEOF) {
            _unlock_file(file);
            return -1;
        }
    }

    _unlock_file(file);
    return len;
}

static int vfprintf_helper(DWORD options, FILE* file, const char *format,
        _locale_t locale, va_list valist)
{
    printf_arg args_ctx[_ARGMAX+1];
    BOOL tmp_buf;
    int ret;

    if(!MSVCRT_CHECK_PMT( file != NULL )) return -1;
    if(!MSVCRT_CHECK_PMT( format != NULL )) return -1;

    if(options & MSVCRT_PRINTF_POSITIONAL_PARAMS) {
        memset(args_ctx, 0, sizeof(args_ctx));
        ret = create_positional_ctx_a(args_ctx, format, valist);
        if(ret < 0) {
            _invalid_parameter(NULL, NULL, NULL, 0, 0);
            *_errno() = EINVAL;
            return ret;
        } else if(!ret)
            options &= ~MSVCRT_PRINTF_POSITIONAL_PARAMS;
    }

    _lock_file(file);
    tmp_buf = add_std_buffer(file);
    ret = pf_printf_a(puts_clbk_file_a, file, format, locale, options,
            options & MSVCRT_PRINTF_POSITIONAL_PARAMS ? arg_clbk_positional : arg_clbk_valist,
            options & MSVCRT_PRINTF_POSITIONAL_PARAMS ? args_ctx : NULL, &valist);
    if(tmp_buf) remove_std_buffer(file);
    _unlock_file(file);

    return ret;
}

static int vfwprintf_helper(DWORD options, FILE* file, const wchar_t *format,
        _locale_t locale, va_list valist)
{
    printf_arg args_ctx[_ARGMAX+1];
    BOOL tmp_buf;
    int ret;

    if(!MSVCRT_CHECK_PMT( file != NULL )) return -1;
    if(!MSVCRT_CHECK_PMT( format != NULL )) return -1;

    if(options & MSVCRT_PRINTF_POSITIONAL_PARAMS) {
        memset(args_ctx, 0, sizeof(args_ctx));
        ret = create_positional_ctx_w(args_ctx, format, valist);
        if(ret < 0) {
            _invalid_parameter(NULL, NULL, NULL, 0, 0);
            *_errno() = EINVAL;
            return ret;
        } else if(!ret)
            options &= ~MSVCRT_PRINTF_POSITIONAL_PARAMS;
    }

    _lock_file(file);
    tmp_buf = add_std_buffer(file);
    ret = pf_printf_w(puts_clbk_file_w, file, format, locale, options,
            options & MSVCRT_PRINTF_POSITIONAL_PARAMS ? arg_clbk_positional : arg_clbk_valist,
            options & MSVCRT_PRINTF_POSITIONAL_PARAMS ? args_ctx : NULL, &valist);
    if(tmp_buf) remove_std_buffer(file);
    _unlock_file(file);

    return ret;
}

/*********************************************************************
 *    _vfprintf_s_l (MSVCRT.@)
 */
int CDECL _vfprintf_s_l(FILE* file, const char *format,
        _locale_t locale, va_list valist)
{
    return vfprintf_helper(MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER, file, format, locale, valist);
}

/*********************************************************************
 *    _vfwprintf_s_l (MSVCRT.@)
 */
int CDECL _vfwprintf_s_l(FILE* file, const wchar_t *format,
        _locale_t locale, va_list valist)
{
    return vfwprintf_helper(MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER, file, format, locale, valist);
}

/*********************************************************************
 *		vfprintf (MSVCRT.@)
 */
int CDECL vfprintf(FILE* file, const char *format, va_list valist)
{
    return vfprintf_helper(0, file, format, NULL, valist);
}

/*********************************************************************
 *		vfprintf_s (MSVCRT.@)
 */
int CDECL vfprintf_s(FILE* file, const char *format, va_list valist)
{
    return _vfprintf_s_l(file, format, NULL, valist);
}

/*********************************************************************
 *		vfwprintf (MSVCRT.@)
 */
int CDECL vfwprintf(FILE* file, const wchar_t *format, va_list valist)
{
    return vfwprintf_helper(0, file, format, NULL, valist);
}

/*********************************************************************
 *		vfwprintf_s (MSVCRT.@)
 */
int CDECL vfwprintf_s(FILE* file, const wchar_t *format, va_list valist)
{
    return _vfwprintf_s_l(file, format, NULL, valist);
}

#if _MSVCR_VER >= 140

/*********************************************************************
 *              __stdio_common_vfprintf (UCRTBASE.@)
 */
int CDECL _stdio_common_vfprintf(unsigned __int64 options, FILE *file, const char *format,
                                        _locale_t locale, va_list valist)
{
    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %#I64x not handled\n", options);

    return vfprintf_helper(options & UCRTBASE_PRINTF_MASK, file, format, locale, valist);
}

/*********************************************************************
 *              __stdio_common_vfprintf_p (UCRTBASE.@)
 */
int CDECL __stdio_common_vfprintf_p(unsigned __int64 options, FILE *file, const char *format,
                                          _locale_t locale, va_list valist)
{
    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %#I64x not handled\n", options);

    return vfprintf_helper((options & UCRTBASE_PRINTF_MASK) | MSVCRT_PRINTF_POSITIONAL_PARAMS
            | MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER, file, format, locale, valist);
}


/*********************************************************************
 *              __stdio_common_vfprintf_s (UCRTBASE.@)
 */
int CDECL __stdio_common_vfprintf_s(unsigned __int64 options, FILE *file, const char *format,
                                          _locale_t locale, va_list valist)
{
    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %#I64x not handled\n", options);

    return vfprintf_helper((options & UCRTBASE_PRINTF_MASK) | MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER,
            file, format, locale, valist);
}

/*********************************************************************
 *              __stdio_common_vfwprintf (UCRTBASE.@)
 */
int CDECL __stdio_common_vfwprintf(unsigned __int64 options, FILE *file, const wchar_t *format,
                                         _locale_t locale, va_list valist)
{
    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %#I64x not handled\n", options);

    return vfwprintf_helper(options & UCRTBASE_PRINTF_MASK, file, format, locale, valist);
}

/*********************************************************************
 *              __stdio_common_vfwprintf_p (UCRTBASE.@)
 */
int CDECL __stdio_common_vfwprintf_p(unsigned __int64 options, FILE *file, const wchar_t *format,
                                           _locale_t locale, va_list valist)
{
    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %#I64x not handled\n", options);

    return vfwprintf_helper((options & UCRTBASE_PRINTF_MASK) | MSVCRT_PRINTF_POSITIONAL_PARAMS
            | MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER, file, format, locale, valist);
}


/*********************************************************************
 *              __stdio_common_vfwprintf_s (UCRTBASE.@)
 */
int CDECL __stdio_common_vfwprintf_s(unsigned __int64 options, FILE *file, const wchar_t *format,
                                           _locale_t locale, va_list valist)
{
    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %#I64x not handled\n", options);

    return vfwprintf_helper((options & UCRTBASE_PRINTF_MASK) | MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER,
            file, format, locale, valist);
}

#endif /* _MSVCR_VER >= 140 */

/*********************************************************************
 *    _vfprintf_l (MSVCRT.@)
 */
int CDECL _vfprintf_l(FILE* file, const char *format,
        _locale_t locale, va_list valist)
{
    return vfprintf_helper(0, file, format, locale, valist);
}

/*********************************************************************
 *              _vfwprintf_l (MSVCRT.@)
 */
int CDECL _vfwprintf_l(FILE* file, const wchar_t *format,
        _locale_t locale, va_list valist)
{
    return vfwprintf_helper(0, file, format, locale, valist);
}

/*********************************************************************
 *    _vfprintf_p_l (MSVCRT.@)
 */
int CDECL _vfprintf_p_l(FILE* file, const char *format,
        _locale_t locale, va_list valist)
{
    return vfprintf_helper(MSVCRT_PRINTF_POSITIONAL_PARAMS | MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER,
            file, format, locale, valist);
}

/*********************************************************************
 *    _vfprintf_p (MSVCRT.@)
 */
int CDECL _vfprintf_p(FILE* file, const char *format, va_list valist)
{
    return _vfprintf_p_l(file, format, NULL, valist);
}

/*********************************************************************
 *    _vfwprintf_p_l (MSVCRT.@)
 */
int CDECL _vfwprintf_p_l(FILE* file, const wchar_t *format,
        _locale_t locale, va_list valist)
{
    return vfwprintf_helper(MSVCRT_PRINTF_POSITIONAL_PARAMS | MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER,
            file, format, locale, valist);
}

/*********************************************************************
 *    _vfwprintf_p (MSVCRT.@)
 */
int CDECL _vfwprintf_p(FILE* file, const wchar_t *format, va_list valist)
{
    return _vfwprintf_p_l(file, format, NULL, valist);
}

/*********************************************************************
 *		vprintf (MSVCRT.@)
 */
int CDECL vprintf(const char *format, va_list valist)
{
  return vfprintf(stdout,format,valist);
}

/*********************************************************************
 *		vprintf_s (MSVCRT.@)
 */
int CDECL vprintf_s(const char *format, va_list valist)
{
  return vfprintf_s(stdout,format,valist);
}

/*********************************************************************
 *		vwprintf (MSVCRT.@)
 */
int CDECL vwprintf(const wchar_t *format, va_list valist)
{
  return vfwprintf(stdout,format,valist);
}

/*********************************************************************
 *		vwprintf_s (MSVCRT.@)
 */
int CDECL vwprintf_s(const wchar_t *format, va_list valist)
{
  return vfwprintf_s(stdout,format,valist);
}

/*********************************************************************
 *		fprintf (MSVCRT.@)
 */
int WINAPIV fprintf(FILE* file, const char *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = vfprintf(file, format, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		fprintf_s (MSVCRT.@)
 */
int WINAPIV fprintf_s(FILE* file, const char *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = vfprintf_s(file, format, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *    _fprintf_l (MSVCRT.@)
 */
int WINAPIV _fprintf_l(FILE* file, const char *format, _locale_t locale, ...)
{
    va_list valist;
    int res;
    va_start(valist, locale);
    res = _vfprintf_l(file, format, locale, valist);
    va_end(valist);
    return res;
}


/*********************************************************************
 *    _fprintf_p (MSVCRT.@)
 */
int WINAPIV _fprintf_p(FILE* file, const char *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = _vfprintf_p_l(file, format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *    _fprintf_p_l (MSVCRT.@)
 */
int WINAPIV _fprintf_p_l(FILE* file, const char *format, _locale_t locale, ...)
{
    va_list valist;
    int res;
    va_start(valist, locale);
    res = _vfprintf_p_l(file, format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *    _fprintf_s_l (MSVCRT.@)
 */
int WINAPIV _fprintf_s_l(FILE* file, const char *format, _locale_t locale, ...)
{
    va_list valist;
    int res;
    va_start(valist, locale);
    res = _vfprintf_s_l(file, format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		fwprintf (MSVCRT.@)
 */
int WINAPIV fwprintf(FILE* file, const wchar_t *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = vfwprintf(file, format, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		fwprintf_s (MSVCRT.@)
 */
int WINAPIV fwprintf_s(FILE* file, const wchar_t *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = vfwprintf_s(file, format, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *              _fwprintf_l (MSVCRT.@)
 */
int WINAPIV _fwprintf_l(FILE* file, const wchar_t *format, _locale_t locale, ...)
{
    va_list valist;
    int res;
    va_start(valist, locale);
    res = _vfwprintf_l(file, format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *              _fwprintf_p (MSVCRT.@)
 */
int WINAPIV _fwprintf_p(FILE* file, const wchar_t *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = _vfwprintf_p_l(file, format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *              _fwprintf_p_l (MSVCRT.@)
 */
int WINAPIV _fwprintf_p_l(FILE* file, const wchar_t *format, _locale_t locale, ...)
{
    va_list valist;
    int res;
    va_start(valist, locale);
    res = _vfwprintf_p_l(file, format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *              _fwprintf_s_l (MSVCRT.@)
 */
int WINAPIV _fwprintf_s_l(FILE* file, const wchar_t *format, _locale_t locale, ...)
{
    va_list valist;
    int res;
    va_start(valist, locale);
    res = _vfwprintf_s_l(file, format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		printf (MSVCRT.@)
 */
int WINAPIV printf(const char *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = vfprintf(stdout, format, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		printf_s (MSVCRT.@)
 */
int WINAPIV printf_s(const char *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = vprintf_s(format, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		ungetc (MSVCRT.@)
 */
int CDECL ungetc(int c, FILE * file)
{
    int ret;

    if(!MSVCRT_CHECK_PMT(file != NULL)) return EOF;

    _lock_file(file);
    ret = _ungetc_nolock(c, file);
    _unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_ungetc_nolock (MSVCRT.@)
 */
int CDECL _ungetc_nolock(int c, FILE * file)
{
    if(!MSVCRT_CHECK_PMT(file != NULL)) return EOF;

    if (c == EOF || !(file->_flag&_IOREAD ||
                (file->_flag&_IORW && !(file->_flag&_IOWRT))))
        return EOF;

    if((!(file->_flag & (MSVCRT__NOBUF | _IOMYBUF | MSVCRT__USERBUF))
                && msvcrt_alloc_buffer(file))
            || (!file->_cnt && file->_ptr==file->_base))
        file->_ptr++;

    if(file->_ptr>file->_base) {
        file->_ptr--;
        if(file->_flag & _IOSTRG) {
            if(*file->_ptr != c) {
                file->_ptr++;
                return EOF;
            }
        }else {
            *file->_ptr = c;
        }
        file->_cnt++;
        file->_flag &= ~(_IOERR | _IOEOF);
        file->_flag |= _IOREAD;
        return c;
    }

    return EOF;
}

/*********************************************************************
 *              ungetwc (MSVCRT.@)
 */
wint_t CDECL ungetwc(wint_t wc, FILE * file)
{
    wint_t ret;

    if(!MSVCRT_CHECK_PMT(file != NULL)) return WEOF;

    _lock_file(file);
    ret = _ungetwc_nolock(wc, file);
    _unlock_file(file);

    return ret;
}

/*********************************************************************
 *              _ungetwc_nolock (MSVCRT.@)
 */
wint_t CDECL _ungetwc_nolock(wint_t wc, FILE * file)
{
    wchar_t mwc = wc;

    if(!MSVCRT_CHECK_PMT(file != NULL)) return WEOF;
    if (wc == WEOF)
        return WEOF;

    if(ioinfo_get_textmode(get_ioinfo_nolock(file->_file)) != TEXTMODE_ANSI
            || !(get_ioinfo_nolock(file->_file)->wxflag & WX_TEXT)) {
        unsigned char * pp = (unsigned char *)&mwc;
        int i;

        for(i=sizeof(wchar_t)-1;i>=0;i--) {
            if(pp[i] != _ungetc_nolock(pp[i],file))
                return WEOF;
        }
    }else {
        char mbs[MB_LEN_MAX];
        int len;

        len = wctomb(mbs, mwc);
        if(len == -1)
            return WEOF;

        for(len--; len>=0; len--) {
            if(mbs[len] != _ungetc_nolock(mbs[len], file))
                return WEOF;
        }
    }

    return mwc;
}

/*********************************************************************
 *		wprintf (MSVCRT.@)
 */
int WINAPIV wprintf(const wchar_t *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = vwprintf(format, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		wprintf_s (MSVCRT.@)
 */
int WINAPIV wprintf_s(const wchar_t *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = vwprintf_s(format, valist);
    va_end(valist);
    return res;
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

#if _MSVCR_VER >= 140
/*********************************************************************
 *		_get_stream_buffer_pointers (UCRTBASE.@)
 */
int CDECL _get_stream_buffer_pointers(FILE *file, char*** base,
                                             char*** ptr, int** count)
{
    if (base)
        *base = &file->_base;
    if (ptr)
        *ptr = &file->_ptr;
    if (count)
        *count = &file->_cnt;
    return 0;
}
#endif
