#include <precomp.h>
#include "wine/unicode.h"

#include <direct.h>
#include <tchar.h>
#include <sys/stat.h>

#ifndef _UNICODE
#include <string.h>
#endif

#ifndef _USE_STAT64
#define _USE_STAT64 1
#endif 

/* for stat mode, permissions apply to all,owner and group */
#define ALL_S_IREAD  (_S_IREAD  | (_S_IREAD  >> 3) | (_S_IREAD  >> 6))
#define ALL_S_IWRITE (_S_IWRITE | (_S_IWRITE >> 3) | (_S_IWRITE >> 6))
#define ALL_S_IEXEC  (_S_IEXEC  | (_S_IEXEC  >> 3) | (_S_IEXEC  >> 6))

#define EXE ('e' << 16 | 'x' << 8 | 'e')
#define BAT ('b' << 16 | 'a' << 8 | 't')
#define CMD ('c' << 16 | 'm' << 8 | 'd')
#define COM ('c' << 16 | 'o' << 8 | 'm')

#define TOUL(x) (ULONGLONG)(x)
#define WCEXE (TOUL('e') << 32 | TOUL('x') << 16 | TOUL('e'))
#define WCBAT (TOUL('b') << 32 | TOUL('a') << 16 | TOUL('t'))
#define WCCMD (TOUL('c') << 32 | TOUL('m') << 16 | TOUL('d'))
#define WCCOM (TOUL('c') << 32 | TOUL('o') << 16 | TOUL('m'))

#if _USE_STAT64
void stat64_to_stati64(const struct __stat64 *buf64, struct _stati64 *buf)
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

//int _tstati64(const TCHAR* path, struct _stati64 * buf)
//{
//  int ret;
//  struct __stat64 buf64;
//
//  ret = _tstat64(path, &buf64);
//  if (!ret)
//    stat64_to_stati64(&buf64, buf);
//  return ret;
//}

#endif

HANDLE fdtoh(int fd); //file.c

#if _USE_STAT64
int CDECL _tstat64(const _TCHAR *path, struct _stat64 *buf)
#else
int CDECL _tstat64i32(const _TCHAR *path, struct _stat64i32 *buf)
#endif
{
  DWORD dw;
  WIN32_FILE_ATTRIBUTE_DATA hfi;
  unsigned short mode = ALL_S_IREAD;
  int plen;

  TRACE(":file (%s) buf(%p)\n",path,buf);

  if (!GetFileAttributesEx(path, GetFileExInfoStandard, &hfi))
  {
      TRACE("failed (%d)\n",GetLastError());
      __set_errno(ERROR_FILE_NOT_FOUND);
      return -1;
  }

  memset(buf,0,sizeof(struct __stat64));

  /* FIXME: rdev isn't drive num, despite what the docs say-what is it?
     Bon 011120: This FIXME seems incorrect
                 Also a letter as first char isn't enough to be classified
		 as a drive letter
  */
#ifndef _UNICODE
  if (isalpha(*path)&& (*(path+1)==':'))
    buf->st_dev = buf->st_rdev = toupper(*path) - 'A'; /* drive num */
#else
    if (iswalpha(*path))
    buf->st_dev = buf->st_rdev = toupperW(*path - 'A'); /* drive num */
#endif
  else
    buf->st_dev = buf->st_rdev = _getdrive() - 1;

#ifndef _UNICODE
  plen = strlen(path);
#else
  plen = strlenW(path);
#endif

  /* Dir, or regular file? */
  if ((hfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
      (path[plen-1] == '\\'))
    mode |= (_S_IFDIR | ALL_S_IEXEC);
  else
  {
    mode |= _S_IFREG;
    /* executable? */
    if (plen > 6 && path[plen-4] == '.')  /* shortest exe: "\x.exe" */
    {
#ifndef _UNICODE
      unsigned int ext = tolower(path[plen-1]) | (tolower(path[plen-2]) << 8) |
                                 (tolower(path[plen-3]) << 16);
      if (ext == EXE || ext == BAT || ext == CMD || ext == COM)
          mode |= ALL_S_IEXEC;
#else
      ULONGLONG ext = tolowerW(path[plen-1]) | (tolowerW(path[plen-2]) << 16) |
                               ((ULONGLONG)tolowerW(path[plen-3]) << 32);
      if (ext == WCEXE || ext == WCBAT || ext == WCCMD || ext == WCCOM)
        mode |= ALL_S_IEXEC;
#endif
    }
  }

  if (!(hfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    mode |= ALL_S_IWRITE;

  buf->st_mode  = mode;
  buf->st_nlink = 1;
#if _USE_STAT64
        buf->st_size =  buf->st_size  = ((__int64)hfi.nFileSizeHigh << 32) + hfi.nFileSizeLow;
#else
        buf->st_size = hfi.nFileSizeLow;
#endif
  RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastAccessTime, &dw);
  buf->st_atime = dw;
  RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastWriteTime, &dw);
  buf->st_mtime = buf->st_ctime = dw;
  TRACE("%d %d 0x%08lx%08lx %ld %ld %ld\n", buf->st_mode,buf->st_nlink,
        (long)(buf->st_size >> 16),(long)buf->st_size,
        (long)buf->st_atime,(long)buf->st_mtime,(long)buf->st_ctime);
  return 0;
}

/*********************************************************************
 *		_stat (MSVCRT.@)
 */
#if _USE_STAT64
int CDECL _tstat32i64(const _TCHAR *path,struct _stat32i64 *buf)
#else
int CDECL _tstat32(const _TCHAR *path,struct _stat32 *buf)
#endif
{
  DWORD dw;
  WIN32_FILE_ATTRIBUTE_DATA hfi;
  unsigned short mode = ALL_S_IREAD;
  int plen;

  TRACE(":file (%s) buf(%p)\n",path,buf);

  if (!GetFileAttributesEx(path, GetFileExInfoStandard, &hfi))
  {
      TRACE("failed (%d)\n",GetLastError());
      __set_errno(ERROR_FILE_NOT_FOUND);
      return -1;
  }

  memset(buf,0,sizeof(*buf));

  /* FIXME: rdev isn't drive num, despite what the docs say-what is it?
     Bon 011120: This FIXME seems incorrect
                 Also a letter as first char isn't enough to be classified
		 as a drive letter
  */
#ifndef _UNICODE
  if (isalpha(*path)&& (*(path+1)==':'))
    buf->st_dev = buf->st_rdev = toupper(*path) - 'A'; /* drive num */
#else
   if (iswalpha(*path))
    buf->st_dev = buf->st_rdev = toupperW(*path - 'A'); /* drive num */
#endif
  else
    buf->st_dev = buf->st_rdev = _getdrive() - 1;

#ifndef _UNICODE
  plen = strlen(path);
#else
  plen = strlenW(path);
#endif

  /* Dir, or regular file? */
  if ((hfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
      (path[plen-1] == '\\'))
    mode |= (_S_IFDIR | ALL_S_IEXEC);
  else
  {
    mode |= _S_IFREG;
    /* executable? */
    if (plen > 6 && path[plen-4] == '.')  /* shortest exe: "\x.exe" */
    {
#ifndef _UNICODE
      unsigned int ext = tolower(path[plen-1]) | (tolower(path[plen-2]) << 8) |
                                 (tolower(path[plen-3]) << 16);
      if (ext == EXE || ext == BAT || ext == CMD || ext == COM)
          mode |= ALL_S_IEXEC;
#else
      ULONGLONG ext = tolowerW(path[plen-1]) | (tolowerW(path[plen-2]) << 16) |
                               ((ULONGLONG)tolowerW(path[plen-3]) << 32);
      if (ext == WCEXE || ext == WCBAT || ext == WCCMD || ext == WCCOM)
        mode |= ALL_S_IEXEC;
#endif
    }
  }

  if (!(hfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    mode |= ALL_S_IWRITE;

  buf->st_mode  = mode;
  buf->st_nlink = 1;
#if _USE_STAT64
  buf->st_size  = ((__int32)hfi.nFileSizeHigh << 16) + hfi.nFileSizeLow;
#else
  buf->st_size = hfi.nFileSizeLow;
#endif
  RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastAccessTime, &dw);
  buf->st_atime = dw;
  RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastWriteTime, &dw);
  buf->st_mtime = buf->st_ctime = dw;
  TRACE("%d %d 0x%08lx%08lx %ld %ld %ld\n", buf->st_mode,buf->st_nlink,
        (long)(buf->st_size >> 16),(long)buf->st_size,
        (long)buf->st_atime,(long)buf->st_mtime,(long)buf->st_ctime);
  return 0;
}

#ifndef _UNICODE //No wide versions needed

#if _USE_STAT64
int CDECL _fstat64(int fd, struct _stat64* buf)
#else
int CDECL _fstat64i32(int fd, struct _stat64i32* buf)
#endif
{
  DWORD dw;
  DWORD type;
  BY_HANDLE_FILE_INFORMATION hfi;
  HANDLE hand = fdtoh(fd);

  TRACE(":fd (%d) stat (%p)\n",fd,buf);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  if (!buf)
  {
    WARN(":failed-NULL buf\n");
    __set_errno(ERROR_INVALID_PARAMETER);
    return -1;
  }

  memset(&hfi, 0, sizeof(hfi));
  memset(buf, 0, sizeof(struct __stat64));
  type = GetFileType(hand);
  if (type == FILE_TYPE_PIPE)
  {
    buf->st_dev = buf->st_rdev = fd;
    buf->st_mode = S_IFIFO;
    buf->st_nlink = 1;
  }
  else if (type == FILE_TYPE_CHAR)
  {
    buf->st_dev = buf->st_rdev = fd;
    buf->st_mode = S_IFCHR;
    buf->st_nlink = 1;
  }
  else /* FILE_TYPE_DISK etc. */
  {
    if (!GetFileInformationByHandle(hand, &hfi))
    {
      WARN(":failed-last error (%d)\n",GetLastError());
      __set_errno(ERROR_INVALID_PARAMETER);
      return -1;
    }
    buf->st_mode = S_IFREG | S_IREAD;
    if (!(hfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
      buf->st_mode |= S_IWRITE;
    buf->st_size  = ((__int64)hfi.nFileSizeHigh << 32) + hfi.nFileSizeLow;
    RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastAccessTime, &dw);
    buf->st_atime = dw;
    RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastWriteTime, &dw);
    buf->st_mtime = buf->st_ctime = dw;
    buf->st_nlink = hfi.nNumberOfLinks;
  }
  TRACE(":dwFileAttributes = 0x%x, mode set to 0x%x\n",hfi.dwFileAttributes,
   buf->st_mode);
  return 0;
}

/*********************************************************************
 *		_fstat (MSVCRT.@)
 */
#if _USE_STAT64
int CDECL _fstat32(int fd, struct _stat32* buf)
#else
int CDECL _fstat32i64(int fd, struct _stat32i64* buf)
#endif
{
  DWORD dw;
  DWORD type;
  BY_HANDLE_FILE_INFORMATION hfi;
  HANDLE hand = fdtoh(fd);

  TRACE(":fd (%d) stat (%p)\n",fd,buf);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  if (!buf)
  {
    WARN(":failed-NULL buf\n");
    __set_errno(ERROR_INVALID_PARAMETER);
    return -1;
  }

  memset(&hfi, 0, sizeof(hfi));
  memset(buf, 0, sizeof(struct _stat32));
  type = GetFileType(hand);
  if (type == FILE_TYPE_PIPE)
  {
    buf->st_dev = buf->st_rdev = fd;
    buf->st_mode = S_IFIFO;
    buf->st_nlink = 1;
  }
  else if (type == FILE_TYPE_CHAR)
  {
    buf->st_dev = buf->st_rdev = fd;
    buf->st_mode = S_IFCHR;
    buf->st_nlink = 1;
  }
  else /* FILE_TYPE_DISK etc. */
  {
    if (!GetFileInformationByHandle(hand, &hfi))
    {
      WARN(":failed-last error (%d)\n",GetLastError());
      __set_errno(ERROR_INVALID_PARAMETER);
      return -1;
    }
    buf->st_mode = S_IFREG | S_IREAD;
    if (!(hfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
      buf->st_mode |= S_IWRITE;
    buf->st_size  = ((__int32)hfi.nFileSizeHigh << 16) + hfi.nFileSizeLow;
    RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastAccessTime, &dw);
    buf->st_atime = dw;
    RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastWriteTime, &dw);
    buf->st_mtime = buf->st_ctime = dw;
    buf->st_nlink = hfi.nNumberOfLinks;
  }
  TRACE(":dwFileAttributes = 0x%x, mode set to 0x%x\n",hfi.dwFileAttributes,
   buf->st_mode);
  return 0;
}

#endif
