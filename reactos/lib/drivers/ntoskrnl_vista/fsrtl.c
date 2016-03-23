/*
 * PROJECT:         ReactOS Kernel - Vista+ APIs
 * LICENSE:         GPL v2 - See COPYING in the top level directory
 * FILE:            lib/drivers/ntoskrnl_vista/fsrtl.c
 * PURPOSE:         FsRtl functions of Vista+
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include <ntdef.h>
#include <ntifs.h>

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlRemoveDotsFromPath(IN PWSTR OriginalString,
                        IN USHORT PathLength,
                        OUT USHORT *NewLength)
{
    USHORT Length, ReadPos, WritePos = 0;

    Length = PathLength / sizeof(WCHAR);

    if (Length == 3 && OriginalString[0] == '\\' && OriginalString[1] == '.' && OriginalString[2] == '.')
    {
        return STATUS_IO_REPARSE_DATA_INVALID;
    }

    if (Length == 2 && OriginalString[0] == '.' && OriginalString[1] == '.')
    {
        return STATUS_IO_REPARSE_DATA_INVALID;
    }

    if (Length > 2 && OriginalString[0] == '.' && OriginalString[1] == '.' && OriginalString[2] == '\\')
    {
        return STATUS_IO_REPARSE_DATA_INVALID;
    }

    if (Length > 0)
    {
        ReadPos = 0;

        for (; ReadPos < Length; ++WritePos)
        {
            for (; ReadPos < Length; ++ReadPos)
            {
                if (ReadPos < Length - 1 && OriginalString[ReadPos] == '\\' && OriginalString[ReadPos + 1] == '\\')
                {
                    continue;
                }

                if (OriginalString[ReadPos] != '.')
                {
                    break;
                }

                if (ReadPos == Length - 1)
                {
                    if (OriginalString[ReadPos - 1] == '\\')
                    {
                        if (WritePos > 1)
                        {
                            --WritePos;
                        }

                        continue;
                    }

                    OriginalString[WritePos] = '.';
                    ++WritePos;
                    continue;
                }

                if (OriginalString[ReadPos + 1] == '\\')
                {
                    if (OriginalString[ReadPos - 1] != '\\')
                    {
                        OriginalString[WritePos] = '.';
                        ++WritePos;
                        continue;
                    }
                }
                else
                {
                    if (OriginalString[ReadPos + 1] != '.' || OriginalString[ReadPos - 1] != '\\' ||
                        ((ReadPos != Length - 2) && OriginalString[ReadPos + 2] != '\\'))
                    {
                        OriginalString[WritePos] = '.';
                        ++WritePos;
                        continue;
                    }

                    for (WritePos -= 2; (SHORT)WritePos > 0 && OriginalString[WritePos] != '\\'; --WritePos);

                    if ((SHORT)WritePos < 0 || OriginalString[WritePos] != '\\')
                    {
                        return STATUS_IO_REPARSE_DATA_INVALID;
                    }

                    if (WritePos == 0 && ReadPos == Length - 2)
                    {
                        WritePos = 1;
                    }
                }

                ++ReadPos;
            }

            OriginalString[WritePos] = OriginalString[ReadPos];
            ++ReadPos;
        }
    }

    *NewLength = WritePos * sizeof(WCHAR);

    while (WritePos < Length)
    {
        OriginalString[WritePos++] = UNICODE_NULL;
    }

    return STATUS_SUCCESS;
}
