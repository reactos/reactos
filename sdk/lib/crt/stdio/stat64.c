#include <precomp.h>
#include <tchar.h>
#include <direct.h>
#include <internal/wine/msvcrt.h>

ioinfo* get_ioinfo(int fd);
void release_ioinfo(ioinfo *info);

#define ALL_S_IREAD  (_S_IREAD  | (_S_IREAD  >> 3) | (_S_IREAD  >> 6))
#define ALL_S_IWRITE (_S_IWRITE | (_S_IWRITE >> 3) | (_S_IWRITE >> 6))
#define ALL_S_IEXEC  (_S_IEXEC  | (_S_IEXEC  >> 3) | (_S_IEXEC  >> 6))

#ifdef UNICODE
#define TCHAR4 ULONGLONG
#else
#define TCHAR4 ULONG
#endif

#define TCSIZE_BITS (sizeof(_TCHAR)*8)

#define EXE (((TCHAR4)('e')<<(2*TCSIZE_BITS)) | ((TCHAR4)('x')<<TCSIZE_BITS) | ((TCHAR4)('e')))
#define BAT (((TCHAR4)('b')<<(2*TCSIZE_BITS)) | ((TCHAR4)('a')<<TCSIZE_BITS) | ((TCHAR4)('t')))
#define CMD (((TCHAR4)('c')<<(2*TCSIZE_BITS)) | ((TCHAR4)('m')<<TCSIZE_BITS) | ((TCHAR4)('d')))
#define COM (((TCHAR4)('c')<<(2*TCSIZE_BITS)) | ((TCHAR4)('o')<<TCSIZE_BITS) | ((TCHAR4)('m')))

int CDECL _tstat64(const _TCHAR *path, struct __stat64 *buf)
{
  DWORD dw;
  WIN32_FILE_ATTRIBUTE_DATA hfi;
  unsigned short mode = ALL_S_IREAD;
  size_t plen;

  TRACE(":file (%s) buf(%p)\n",path,buf);

  plen = _tcslen(path);
  while (plen && path[plen-1]==__T(' '))
    plen--;

  if (plen && (plen<2 || path[plen-2]!=__T(':')) &&
          (path[plen-1]==__T(':') || path[plen-1]==__T('\\') || path[plen-1]==__T('/')))
  {
    *_errno() = ENOENT;
    return -1;
  }

  if (!GetFileAttributesEx(path, GetFileExInfoStandard, &hfi))
  {
      TRACE("failed (%d)\n",GetLastError());
      *_errno() = ENOENT;
      return -1;
  }

  memset(buf,0,sizeof(struct __stat64));

  /* FIXME: rdev isn't drive num, despite what the docs say-what is it?
     Bon 011120: This FIXME seems incorrect
                 Also a letter as first char isn't enough to be classified
		 as a drive letter
  */
  if (_istalpha(*path) && (*(path+1)==__T(':')))
    buf->st_dev = buf->st_rdev = _totupper(*path) - __T('A'); /* drive num */
  else
    buf->st_dev = buf->st_rdev = _getdrive() - 1;

  /* Dir, or regular file? */
  if (hfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    mode |= (_S_IFDIR | ALL_S_IEXEC);
  else
  {
    mode |= _S_IFREG;
    /* executable? */
    if (plen > 6 && path[plen-4] == __T('.'))  /* shortest exe: "\x.exe" */
    {

      TCHAR4 ext = (TCHAR4)_totlower(path[plen-1])
                   | ((TCHAR4)_totlower(path[plen-2]) << TCSIZE_BITS)
                   | ((TCHAR4)_totlower(path[plen-3]) << 2*TCSIZE_BITS);

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
  TRACE("%d %d 0x%08x%08x %d %d %d\n", buf->st_mode,buf->st_nlink,
        (int)(buf->st_size >> 32),(int)buf->st_size,
        (int)buf->st_atime,(int)buf->st_mtime,(int)buf->st_ctime);
  return 0;
}

#ifndef _UNICODE

int CDECL _fstat64(int fd, struct __stat64* buf)
{
  ioinfo *info = get_ioinfo(fd);
  DWORD dw;
  DWORD type;
  BY_HANDLE_FILE_INFORMATION hfi;

  TRACE(":fd (%d) stat (%p)\n", fd, buf);
  if (info->handle == INVALID_HANDLE_VALUE)
  {
    release_ioinfo(info);
    return -1;
  }

  if (!buf)
  {
    WARN(":failed-NULL buf\n");
    _dosmaperr(ERROR_INVALID_PARAMETER);
    release_ioinfo(info);
    return -1;
  }

  memset(&hfi, 0, sizeof(hfi));
  memset(buf, 0, sizeof(struct __stat64));
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
    if (!GetFileInformationByHandle(info->handle, &hfi))
    {
      WARN(":failed-last error (%d)\n",GetLastError());
      _dosmaperr(ERROR_INVALID_PARAMETER);
      release_ioinfo(info);
      return -1;
    }
    buf->st_mode = _S_IFREG | ALL_S_IREAD;
    if (!(hfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
      buf->st_mode |= ALL_S_IWRITE;
    buf->st_size  = ((__int64)hfi.nFileSizeHigh << 32) + hfi.nFileSizeLow;
    RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastAccessTime, &dw);
    buf->st_atime = dw;
    RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastWriteTime, &dw);
    buf->st_mtime = buf->st_ctime = dw;
    buf->st_nlink = (short)hfi.nNumberOfLinks;
  }
  TRACE(":dwFileAttributes = 0x%x, mode set to 0x%x\n",hfi.dwFileAttributes,
   buf->st_mode);
  release_ioinfo(info);
  return 0;
}

#endif
