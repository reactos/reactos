/* $Id: open.c,v 1.5 2002/10/29 04:45:31 rex Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/fcntl/open.c
 * PURPOSE:     Open a file
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              04/02/2002: Created
 */

#include <ddk/ntddk.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <psx/path.h>
#include <psx/debug.h>
#include <psx/errno.h>
#include <psx/pdata.h>

int open(const char *path, int oflag, ...)
{
 ANSI_STRING    strPath;
 UNICODE_STRING wstrPath;
 int            nRetVal;

 RtlInitAnsiString(&strPath, (PCSZ)path);
 RtlAnsiStringToUnicodeString(&wstrPath, &strPath, TRUE);

 nRetVal = _Wopen(wstrPath.Buffer, oflag);

 RtlFreeUnicodeString(&wstrPath);

 return (nRetVal);
}

int _Wopen(const wchar_t *path, int oflag, ...)
{
 OBJECT_ATTRIBUTES oaFileAttribs;
 IO_STATUS_BLOCK   isbStatus;
 UNICODE_STRING    wstrNativePath;
 NTSTATUS          nErrCode;
 ULONG             nDesiredAccess;
 ULONG             nCreateDisposition;
 ULONG             nCreateOptions;
 HANDLE            hFile;
#if 0
 mode_t            mFileMode;
#endif
 int               nFileNo;
 __fildes_t        fdDescriptor;

 /* translate file access flag */
 switch(oflag & O_ACCMODE)
 {
  case O_RDONLY:
  {
   nDesiredAccess = FILE_READ_ACCESS;
   nCreateOptions = 0;
   break;
  }

  case O_WRONLY:
  {
   nDesiredAccess = FILE_WRITE_ACCESS;
   nCreateOptions = FILE_NON_DIRECTORY_FILE; /* required by the specification */
   break;
  }

  case O_RDWR:
  {
   nDesiredAccess = FILE_READ_ACCESS | FILE_WRITE_ACCESS;
   nCreateOptions = FILE_NON_DIRECTORY_FILE; /* required by the specification */
   break;
  }

  default:
  {
   errno = EINVAL;
   return (-1);
  }

 }

 /* miscellaneous flags */
 if((oflag & _O_DIRFILE) == _O_DIRFILE)
  nCreateOptions |= FILE_DIRECTORY_FILE;

 /* creation disposition */
 if((oflag & O_CREAT) == O_CREAT)
  if((oflag & O_EXCL) == O_EXCL)
   nCreateDisposition = FILE_CREATE;    /* O_CREAT + O_EXCL: create file, fail if file exists */
  else
   nCreateDisposition = FILE_OPEN_IF;   /* O_CREAT: open file, create if file doesn't exist */
 else if((oflag & O_TRUNC) == O_TRUNC)
  nCreateDisposition = FILE_OVERWRITE;  /* O_TRUNC: truncate existing file */
 else
  nCreateDisposition = FILE_OPEN;       /* normal: open file, fail if file doesn't exist */

 /* lock the environment */
 __PdxAcquirePdataLock();

 /* convert the path into a native path */
 if(!__PdxPosixPathNameToNtPathName((LPWSTR)path, __PdxGetNativePathBuffer(), __PdxGetCurDir(), NULL))
 {
  __PdxReleasePdataLock();
  return (-1);
 }

 /* set file generic object attributes */
 oaFileAttribs.Length = sizeof(oaFileAttribs);
 oaFileAttribs.RootDirectory = __PdxGetRootHandle();
 oaFileAttribs.ObjectName = &wstrNativePath;
 oaFileAttribs.Attributes = OBJ_INHERIT; /* child processes inherit all file descriptors */
 oaFileAttribs.SecurityDescriptor = NULL;
 oaFileAttribs.SecurityQualityOfService = NULL;

 /* open or create the file */
 nErrCode = NtCreateFile
 (
  &hFile,
  nDesiredAccess | SYNCHRONIZE,
  &oaFileAttribs,
  &isbStatus,
  NULL,
  0,
  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
  nCreateDisposition,
  nCreateOptions | FILE_SYNCHRONOUS_IO_NONALERT,
  NULL,
  0
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtCreateFile() failed with status 0x%08X", nErrCode);
  __PdxReleasePdataLock();
  errno = __status_to_errno(nErrCode);
  return (-1);
 }

 /* initialize descriptor constructor */
 memset(&fdDescriptor, 0, sizeof(fdDescriptor));
 fdDescriptor.FileHandle = hFile;
 fdDescriptor.OpenFlags = oflag;

 /* allocate a new file descriptor */
 nFileNo = fcntl(0, F_NEWFD, &fdDescriptor);

 /* unlock the environment */
 __PdxReleasePdataLock();

 /* could not allocate the file descriptor */
 if(nFileNo < 0)
 {
  NtClose(hFile);
  return (-1);
 }

 /* return the file number */
 return (nFileNo);
}

int creat(const char *path, mode_t mode)
{
 return (open(path, O_WRONLY | O_CREAT | O_TRUNC, mode));
}

int _Wcreat(const wchar_t *path, mode_t mode)
{
 return (_Wopen(path, O_WRONLY | O_CREAT | O_TRUNC, mode));
}

/* EOF */

