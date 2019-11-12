/*
 * PROJECT:         ReactOS Kernel - Vista+ APIs
 * LICENSE:         LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * FILE:            lib/drivers/ntoskrnl_vista/fsrtl.c
 * PURPOSE:         FsRtl functions of Vista+
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include <ntifs.h>
#include <ntdef.h>

FORCEINLINE
BOOLEAN
IsNullGuid(IN PGUID Guid)
{
    if (Guid->Data1 == 0 && Guid->Data2 == 0 && Guid->Data3 == 0 &&
        ((ULONG *)Guid->Data4)[0] == 0 && ((ULONG *)Guid->Data4)[1] == 0)
    {
        return TRUE;
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
IsEven(IN USHORT Digit)
{
    return ((Digit & 1) != 1);
}

NTSTATUS __stdcall compat_FsRtlValidateReparsePointBuffer(IN ULONG BufferLength, IN PREPARSE_DATA_BUFFER ReparseBuffer)
{
    USHORT DataLength;
    ULONG ReparseTag;
    PREPARSE_GUID_DATA_BUFFER GuidBuffer;

    /* Validate data size range */
    if (BufferLength < REPARSE_DATA_BUFFER_HEADER_SIZE || BufferLength > MAXIMUM_REPARSE_DATA_BUFFER_SIZE)
    {
        return STATUS_IO_REPARSE_DATA_INVALID;
    }

    GuidBuffer = (PREPARSE_GUID_DATA_BUFFER)ReparseBuffer;
    DataLength = ReparseBuffer->ReparseDataLength;
    ReparseTag = ReparseBuffer->ReparseTag;

    /* Validate size consistency */
    if (DataLength + REPARSE_DATA_BUFFER_HEADER_SIZE != BufferLength && DataLength + REPARSE_GUID_DATA_BUFFER_HEADER_SIZE != BufferLength)
    {
        return STATUS_IO_REPARSE_DATA_INVALID;
    }

    /* REPARSE_DATA_BUFFER is reserved for MS tags */
    if (DataLength + REPARSE_DATA_BUFFER_HEADER_SIZE == BufferLength && !IsReparseTagMicrosoft(ReparseTag))
    {
        return STATUS_IO_REPARSE_DATA_INVALID;
    }

    /* If that a GUID data buffer, its GUID cannot be null, and it cannot contain a MS tag */
    if (DataLength + REPARSE_GUID_DATA_BUFFER_HEADER_SIZE == BufferLength && ((!IsReparseTagMicrosoft(ReparseTag)
        && IsNullGuid(&GuidBuffer->ReparseGuid)) || (ReparseTag == IO_REPARSE_TAG_MOUNT_POINT || ReparseTag == IO_REPARSE_TAG_SYMLINK)))
    {
        return STATUS_IO_REPARSE_DATA_INVALID;
    }

    /* Check the data for MS non reserved tags */
    if (!(ReparseTag & 0xFFF0000) && ReparseTag != IO_REPARSE_TAG_RESERVED_ZERO && ReparseTag != IO_REPARSE_TAG_RESERVED_ONE)
    {
        /* If that's a mount point, validate the MountPointReparseBuffer branch */
        if (ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
        {
            /* We need information */
            if (DataLength >= REPARSE_DATA_BUFFER_HEADER_SIZE)
            {
                /* Substitue must be the first in row */
                if (!ReparseBuffer->MountPointReparseBuffer.SubstituteNameOffset)
                {
                    /* Substitude must be null-terminated */
                    if (ReparseBuffer->MountPointReparseBuffer.PrintNameOffset == ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength + sizeof(UNICODE_NULL))
                    {
                        /* There must just be the Offset/Length fields + buffer + 2 null chars */
                        if (DataLength == ReparseBuffer->MountPointReparseBuffer.PrintNameLength + ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength + (FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer) - FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.SubstituteNameOffset)) + 2 * sizeof(UNICODE_NULL))
                        {
                            return STATUS_SUCCESS;
                        }
                    }
                }
            }
        }
        else
        {
#define FIELDS_SIZE (FIELD_OFFSET(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) - FIELD_OFFSET(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.SubstituteNameOffset))

            /* If that's not a symlink, accept the MS tag as it */
            if (ReparseTag != IO_REPARSE_TAG_SYMLINK)
            {
                return STATUS_SUCCESS;
            }

            /* We need information */
            if (DataLength >= FIELDS_SIZE)
            {
                /* Validate lengths */
                if (ReparseBuffer->SymbolicLinkReparseBuffer.SubstituteNameLength && ReparseBuffer->SymbolicLinkReparseBuffer.PrintNameLength)
                {
                    /* Validate unicode strings */
                    if (IsEven(ReparseBuffer->SymbolicLinkReparseBuffer.SubstituteNameLength) && IsEven(ReparseBuffer->SymbolicLinkReparseBuffer.PrintNameLength) &&
                        IsEven(ReparseBuffer->SymbolicLinkReparseBuffer.SubstituteNameOffset) && IsEven(ReparseBuffer->SymbolicLinkReparseBuffer.PrintNameOffset))
                    {
                        if ((DataLength + REPARSE_DATA_BUFFER_HEADER_SIZE >= ReparseBuffer->SymbolicLinkReparseBuffer.SubstituteNameOffset + ReparseBuffer->SymbolicLinkReparseBuffer.SubstituteNameLength + FIELDS_SIZE + REPARSE_DATA_BUFFER_HEADER_SIZE)
                            && (DataLength + REPARSE_DATA_BUFFER_HEADER_SIZE >= ReparseBuffer->SymbolicLinkReparseBuffer.PrintNameLength + ReparseBuffer->SymbolicLinkReparseBuffer.PrintNameOffset + FIELDS_SIZE + REPARSE_DATA_BUFFER_HEADER_SIZE))
                        {
                            return STATUS_SUCCESS;
                        }
                    }
                }
            }
#undef FIELDS_SIZE
        }

        return STATUS_IO_REPARSE_DATA_INVALID;
    }

    return STATUS_IO_REPARSE_TAG_INVALID;
}
