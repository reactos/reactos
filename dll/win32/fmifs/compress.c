/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         File Management IFS Utility functions
 * PURPOSE:         Volume compression
 *
 * PROGRAMMERS:     Emanuele Aliberti
 */

#include "precomp.h"

/*
 * @implemented
 */
BOOLEAN
NTAPI
EnableVolumeCompression(
    IN PWCHAR DriveRoot,
    IN USHORT Compression)
{
    HANDLE hFile;
    DWORD RetBytes;
    BOOL Ret;

    hFile = CreateFileW(DriveRoot,
                        FILE_READ_DATA | FILE_WRITE_DATA,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS,
                        NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    Ret = DeviceIoControl(hFile,
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

/* EOF */
