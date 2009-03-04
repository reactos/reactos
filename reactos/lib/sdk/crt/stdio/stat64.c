#include <precomp.h>
#include <tchar.h>
#include <direct.h>

HANDLE fdtoh(int fd); 

#define ALL_S_IREAD  (_S_IREAD  | (_S_IREAD  >> 3) | (_S_IREAD  >> 6))
#define ALL_S_IWRITE (_S_IWRITE | (_S_IWRITE >> 3) | (_S_IWRITE >> 6))
#define ALL_S_IEXEC  (_S_IEXEC  | (_S_IEXEC  >> 3) | (_S_IEXEC  >> 6))

#ifdef UNICODE
#define TCHAR4 ULONGLONG
#else
#define TCHAR4 ULONG
#endif

#define TCSIZE sizeof(_TCHAR)
#define TOULL(x) ((TCHAR4)(x))

#define EXE ((TOULL('e')<<(2*TCSIZE)) | (TOULL('x')<<TCSIZE) | (TOULL('e')))
#define BAT ((TOULL('b')<<(2*TCSIZE)) | (TOULL('a')<<TCSIZE) | (TOULL('t')))
#define CMD ((TOULL('c')<<(2*TCSIZE)) | (TOULL('m')<<TCSIZE) | (TOULL('d')))
#define COM ((TOULL('c')<<(2*TCSIZE)) | (TOULL('o')<<TCSIZE) | (TOULL('m')))

int CDECL _tstat64(const _TCHAR *path, struct __stat64 *buf)
{
  DWORD dw;
  WIN32_FILE_ATTRIBUTE_DATA hfi;
  unsigned short mode = ALL_S_IREAD;
  int plen;

  TRACE(":file (%s) buf(%p)\n",path,buf);

  if (!GetFileAttributesEx(path, GetFileExInfoStandard, &hfi))
  {
      TRACE("failed (%d)\n",GetLastError());
      _dosmaperr(ERROR_FILE_NOT_FOUND);
      return -1;
  }

  memset(buf,0,sizeof(struct __stat64));

  /* FIXME: rdev isn't drive num, despite what the docs say-what is it?
     Bon 011120: This FIXME seems incorrect
                 Also a letter as first char isn't enough to be classified
		 as a drive letter
  */
  if (isalpha(*path)&& (*(path+1)==':'))
    buf->st_dev = buf->st_rdev = _totupper(*path) - 'A'; /* drive num */
  else
    buf->st_dev = buf->st_rdev = _getdrive() - 1;

  plen = _tcslen(path);

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
      TCHAR4 ext = _totlower(path[plen-1]) | (_totlower(path[plen-2]) << TCSIZE) |
                                 (_totlower(path[plen-3]) << (2*TCSIZE));
      if (ext == EXE || ext == BAT || ext == CMD || ext == COM)
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
  TRACE("%d %d 0x%08lx%08lx %ld %ld %ld\n", buf->st_mode,buf->st_nlink,
        (long)(buf->st_size >> 32),(long)buf->st_size,
        (long)buf->st_atime,(long)buf->st_mtime,(long)buf->st_ctime);
  return 0;
}

#ifndef _UNICODE

int CDECL _fstat64(int fd, struct __stat64* buf)
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
    _dosmaperr(ERROR_INVALID_PARAMETER);
    return -1;
  }

  memset(&hfi, 0, sizeof(hfi));
  memset(buf, 0, sizeof(struct __stat64));
  type = GetFileType(hand);
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
    if (!GetFileInformationByHandle(hand, &hfi))
    {
      WARN(":failed-last error (%d)\n",GetLastError());
      _dosmaperr(ERROR_INVALID_PARAMETER);
      return -1;
    }
    buf->st_mode = _S_IFREG | _S_IREAD;
    if (!(hfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
      buf->st_mode |= _S_IWRITE;
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

#endif
