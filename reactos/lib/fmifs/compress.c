/* $Id$
 *
 * COPYING:	See the top level directory
 * PROJECT:	ReactOS 
 * FILE:	reactos/lib/fmifs/compress.c
 * DESCRIPTION:	File management IFS utility functions
 * PROGRAMMER:	Emanuele Aliberti
 * UPDATED
 * 	1999-02-16 (Emanuele Aliberti)
 * 		Entry points added.
 */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <winioctl.h>
#include <fmifs.h>


/*
 * @implemented
 */
BOOL STDCALL
EnableVolumeCompression (PWCHAR DriveRoot,
			 USHORT Compression)
{
  HANDLE hFile = CreateFileW(DriveRoot,
                             FILE_READ_DATA | FILE_WRITE_DATA,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_BACKUP_SEMANTICS,
                             NULL);
  
  if(hFile != INVALID_HANDLE_VALUE)
  {
    DWORD RetBytes;
    BOOL Ret = DeviceIoControl(hFile,
                               FSCTL_SET_COMPRESSION,
                               &Compression,
                               sizeof(USHORT),
                               NULL,
                               0,
                               &RetBytes,
                               NULL);

    CloseHandle(hFile);

    return (Ret != 0);
  }
  
  return FALSE;
}

/* EOF */
