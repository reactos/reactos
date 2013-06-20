#include <precomp.h>
#include <tchar.h>

#define stat64_to_stat(buf64, buf)   \
    do { \
    buf->st_dev   = (buf64)->st_dev;   \
    buf->st_ino   = (buf64)->st_ino;   \
    buf->st_mode  = (buf64)->st_mode;  \
    buf->st_nlink = (buf64)->st_nlink; \
    buf->st_uid   = (buf64)->st_uid;   \
    buf->st_gid   = (buf64)->st_gid;   \
    buf->st_rdev  = (buf64)->st_rdev;  \
    buf->st_size  = (_off_t)(buf64)->st_size;  \
    buf->st_atime = (time_t)(buf64)->st_atime; \
    buf->st_mtime = (time_t)(buf64)->st_mtime; \
    buf->st_ctime = (time_t)(buf64)->st_ctime; \
    } while (0)

int CDECL _tstat(const _TCHAR* path, struct _stat * buf)
{
  int ret;
  struct __stat64 buf64;

  ret = _tstat64(path, &buf64);
  if (!ret)
    stat64_to_stat(&buf64, buf);
  return ret;
}

int CDECL _tstati64(const _TCHAR* path, struct _stati64 * buf)
{
  int ret;
  struct __stat64 buf64;

  ret = _tstat64(path, &buf64);
  if (!ret)
    stat64_to_stat(&buf64, buf);
  return ret;
}

#ifndef _UNICODE

int CDECL _fstat(int fd, struct _stat* buf)
{ int ret;
  struct __stat64 buf64;

  ret = _fstat64(fd, &buf64);
  if (!ret)
      stat64_to_stat(&buf64, buf);
  return ret;
}

int CDECL _fstati64(int fd, struct _stati64* buf)
{
  int ret;
  struct __stat64 buf64;

  ret = _fstat64(fd, &buf64);
  if (!ret)
    stat64_to_stat(&buf64, buf);
  return ret;
}

#endif
