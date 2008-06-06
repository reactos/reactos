/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>
#include <sys/stat.h>
#include <direct.h>

/* for stat mode, permissions apply to all,owner and group */
#define ALL_S_IREAD  (S_IREAD  | (S_IREAD  >> 3) | (S_IREAD  >> 6))
#define ALL_S_IWRITE (S_IWRITE | (S_IWRITE >> 3) | (S_IWRITE >> 6))
#define ALL_S_IEXEC  (S_IEXEC  | (S_IEXEC  >> 3) | (S_IEXEC  >> 6))

/*
 * @implemented
 */
int _stat(const char* path, struct _stat* buffer)
{
  WIN32_FILE_ATTRIBUTE_DATA fileAttributeData;
  char* ext;

  if (!buffer)
  {
    __set_errno(EINVAL);
    return -1;
  }

  if (strchr(path, '*') || strchr(path, '?'))
  {
    __set_errno(ENOENT);
    return -1;
  }

  if (!GetFileAttributesExA(path, GetFileExInfoStandard, &fileAttributeData))
  {
    __set_errno(ENOENT);
    return -1;
  }

  memset (buffer, 0, sizeof(struct stat));

  buffer->st_ctime = FileTimeToUnixTime(&fileAttributeData.ftCreationTime,NULL);
  buffer->st_atime = FileTimeToUnixTime(&fileAttributeData.ftLastAccessTime,NULL);
  buffer->st_mtime = FileTimeToUnixTime(&fileAttributeData.ftLastWriteTime,NULL);

//  statbuf->st_dev = fd;
  buffer->st_size = fileAttributeData.nFileSizeLow;
  buffer->st_mode = S_IREAD;
  if (fileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    buffer->st_mode |= S_IFDIR;
  else
  {
    buffer->st_mode |= S_IFREG;
    ext = strrchr(path, '.');
    if (ext && (!_stricmp(ext, ".exe") ||
	        !_stricmp(ext, ".com") ||
		!_stricmp(ext, ".bat") ||
		!_stricmp(ext, ".cmd")))
      buffer->st_mode |= S_IEXEC;
  }
  if (!(fileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    buffer->st_mode |= S_IWRITE;

  return 0;
}

/*
 * @implemented
 */
int _stati64 (const char *path, struct _stati64 *buffer)
{
  WIN32_FILE_ATTRIBUTE_DATA fileAttributeData;
  char* ext;

  if (!buffer)
  {
    __set_errno(EINVAL);
    return -1;
  }

  if(strchr(path, '*') || strchr(path, '?'))
  {
    __set_errno(ENOENT);
    return -1;
  }

  if (!GetFileAttributesExA(path, GetFileExInfoStandard, &fileAttributeData))
  {
    __set_errno(ENOENT);
    return -1;
  }

  memset (buffer, 0, sizeof(struct _stati64));

  buffer->st_ctime = FileTimeToUnixTime(&fileAttributeData.ftCreationTime,NULL);
  buffer->st_atime = FileTimeToUnixTime(&fileAttributeData.ftLastAccessTime,NULL);
  buffer->st_mtime = FileTimeToUnixTime(&fileAttributeData.ftLastWriteTime,NULL);

//  statbuf->st_dev = fd;
  buffer->st_size = ((((__int64)fileAttributeData.nFileSizeHigh) << 16) << 16) +
             fileAttributeData.nFileSizeLow;
  buffer->st_mode = S_IREAD;
  if (fileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    buffer->st_mode |= S_IFDIR;
  else
  {
    buffer->st_mode |= S_IFREG;
    ext = strrchr(path, '.');
    if (ext && (!_stricmp(ext, ".exe") ||
	        !_stricmp(ext, ".com") ||
		!_stricmp(ext, ".bat") ||
		!_stricmp(ext, ".cmd")))
      buffer->st_mode |= S_IEXEC;
  }
  if (!(fileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    buffer->st_mode |= S_IWRITE;

  return 0;
}

/*********************************************************************
 *              _stat64 (MSVCRT.@)
 */
int CDECL _stat64(const char* path, struct __stat64 * buf)
{
    DWORD dw;
    WIN32_FILE_ATTRIBUTE_DATA hfi;
    unsigned short mode = ALL_S_IREAD;
    int plen;

    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &hfi))
    {
        ERR("failed (%d)\n",GetLastError());
        *_errno() = ERROR_FILE_NOT_FOUND;
        return -1;
    }
    memset(buf,0,sizeof(struct __stat64));
    
    /* FIXME: rdev isn't drive num, despite what the docs say-what is it?
    Bon 011120: This FIXME seems incorrect
    Also a letter as first char isn't enough to be classified
    as a drive letter
    */
    if (isalpha(*path)&& (*(path+1)==':'))
        buf->st_dev = buf->st_rdev = toupper(*path) - 'A'; /* drive num */
    else
        buf->st_dev = buf->st_rdev = _getdrive() - 1;
    
    plen = strlen(path);

    /* Dir, or regular file? */
    if ((hfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
        (path[plen-1] == '\\'))
        mode |= (S_IFDIR | ALL_S_IEXEC);
    else
    {
        mode |= S_IFREG;
        /* executable? */
        if (plen > 6 && path[plen-4] == '.')  /* shortest exe: "\x.exe" */
        {
            char* ext = strrchr(path, '.');
    if (ext && (!_stricmp(ext, ".exe") || !_stricmp(ext, ".com") || !_stricmp(ext, ".bat") ||
		!_stricmp(ext, ".cmd")))
        mode |= S_IEXEC;
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


