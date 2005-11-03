/* $Id: readdir.c,v 1.7 2002/10/29 04:45:28 rex Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/dirent/readdir.c
 * PURPOSE:     Read directory
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              27/01/2002: Created
 *              13/02/2002: KJK::Hyperion: modified to use file descriptors
 */

#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stddef.h>
#include <dirent.h>
#include <psx/dirent.h>
#include <psx/debug.h>
#include <psx/errno.h>
#include <psx/safeobj.h>
#include <ddk/ntddk.h>

struct dirent *readdir(DIR *dirp)
{
 struct _Wdirent         *lpwdReturn;
 struct __internal_DIR   *pidData;
 ANSI_STRING               strFileName;
 UNICODE_STRING            wstrFileName;
 NTSTATUS                  nErrCode;

 /* call Unicode function */
 lpwdReturn = _Wreaddir(dirp);

 /* failure */
 if(lpwdReturn == 0)
  return (0);

 /* get the internal data object */
 pidData = ((struct __internal_DIR *)dirp);

 /* create NT Unicode string from the Unicode dirent's buffer */
 RtlInitUnicodeString(&wstrFileName, pidData->ent.de_unicode.d_name);

 /* HACK: make the ANSI string point to the same buffer where the Unicode string is stored */
 strFileName.Buffer = (PCSZ)&pidData->info.FileName[0];
 strFileName.Length = 0;
 strFileName.MaximumLength = MAX_PATH;

 /* convert the filename to ANSI */
 nErrCode = RtlUnicodeStringToAnsiString(&strFileName, &wstrFileName, FALSE);

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  errno = __status_to_errno(nErrCode);
  return (0);
 }

 /* make the ANSI dirent filename point to the ANSI buffer */
 pidData->ent.de_ansi.d_name = strFileName.Buffer;

 /* null-terminate the ANSI name */
 pidData->ent.de_ansi.d_name[strFileName.Length] = 0;

 /* success */
 return (&(pidData->ent.de_ansi));
}

int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result)
{
 errno = ENOSYS;
 return (0);
}

struct _Wdirent *_Wreaddir(DIR *dirp)
{
 HANDLE                    hFile;
 HANDLE                    hDir;
 OBJECT_ATTRIBUTES         oaFileAttribs;
 UNICODE_STRING            wstrFileName;
 FILE_INTERNAL_INFORMATION fiiInfo;
 IO_STATUS_BLOCK           isbStatus;
 NTSTATUS                  nErrCode;
 struct __internal_DIR    *pidData;

 /* check the "magic" signature */
 if(!__safeobj_validate(dirp, __IDIR_MAGIC))
 {
  errno = EINVAL;
  return (0);
 }

 /* get internal data */
 pidData = (struct __internal_DIR *)dirp;

 /* get handle */
 hDir = (HANDLE)fcntl(pidData->fildes, F_GETFH);

 /* failure */
 if(((int)hDir) == -1)
  return (0);

 /* read next directory entry */
 nErrCode = NtQueryDirectoryFile
 (
  hDir,
  NULL,
  NULL,
  NULL,
  &isbStatus,
  (PVOID)&pidData->info,
  sizeof(pidData->info) + sizeof(WCHAR) * (MAX_PATH - 1),
  FileDirectoryInformation,
  TRUE,
  NULL,
  FALSE
 );

 /* failure or EOF */
 if(!NT_SUCCESS(nErrCode))
 {
  if(nErrCode == (NTSTATUS)STATUS_NO_MORE_FILES)
   return (0);
  else
  {
   ERR("NtQueryDirectoryFile() failed with status 0x%08X", nErrCode);
   errno = __status_to_errno(nErrCode);
   return (0);
  }
 }
 
 /* null-terminate the filename, just in case */
 pidData->info.FileName[pidData->info.FileNameLength / sizeof(WCHAR)] = 0;

 INFO("this entry: %ls", pidData->info.FileName);

 /* file inodes are not returned by NtQueryDirectoryFile, we have to open every file */
 /* set file's object attributes */
 wstrFileName.Length = pidData->info.FileNameLength;
 wstrFileName.MaximumLength = sizeof(WCHAR) * MAX_PATH;
 wstrFileName.Buffer = &pidData->info.FileName[0];

 oaFileAttribs.Length = sizeof(OBJECT_ATTRIBUTES);
 oaFileAttribs.RootDirectory = hDir;
 oaFileAttribs.ObjectName = &wstrFileName;
 oaFileAttribs.Attributes = 0;
 oaFileAttribs.SecurityDescriptor = NULL;
 oaFileAttribs.SecurityQualityOfService = NULL;

 /* open the file */
 nErrCode = NtOpenFile
 (
  &hFile,
  FILE_READ_ATTRIBUTES | SYNCHRONIZE,
  &oaFileAttribs,
  &isbStatus,
  0,
  FILE_SYNCHRONOUS_IO_NONALERT
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtOpenFile() failed with status %#X", nErrCode);
  errno = __status_to_errno(nErrCode);
  return (0);
 }

 /* get the internal information for the file */
 nErrCode = NtQueryInformationFile
 (
  hFile,
  &isbStatus,
  &fiiInfo,
  sizeof(FILE_INTERNAL_INFORMATION),
  FileInternalInformation
 );

 /* close the handle (not needed anymore) */
 NtClose(hFile);

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtQueryInformationFile() failed with status %#X", nErrCode);
  errno = __status_to_errno(nErrCode);
  return (0);
 }

 /* return file inode */
 pidData->ent.de_unicode.d_ino = (ino_t)fiiInfo.IndexNumber.QuadPart;

 /* return file name */
 pidData->ent.de_unicode.d_name = &pidData->info.FileName[0];

 /* success */
 return &(pidData->ent.de_unicode);
}

int _Wreaddir_r(DIR *dirp, struct _Wdirent *entry, struct _Wdirent **result)
{
 errno = ENOSYS;
 return (0);
}

/* EOF */

