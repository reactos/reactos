/*
 * PROJECT:         ReactOS Kernel - Vista+ APIs
 * LICENSE:         LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * FILE:            lib/drivers/ntoskrnl_vista/fsrtl.c
 * PURPOSE:         FsRtl functions of Vista+
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include <ntdef.h>
#include <ntifs.h>

typedef struct _ECP_LIST
{
    ULONG Signature;
    ULONG Flags;
    LIST_ENTRY EcpList;
} ECP_LIST, *PECP_LIST;

typedef ULONG ECP_HEADER_FLAGS;

typedef struct _ECP_HEADER
{
    ULONG Signature;
    ULONG Spare;
    LIST_ENTRY ListEntry;
    GUID EcpType;
    PFSRTL_EXTRA_CREATE_PARAMETER_CLEANUP_CALLBACK CleanupCallback;
    ECP_HEADER_FLAGS Flags;
    ULONG Size;
    PVOID ListAllocatedFrom;
    PVOID Filter;
} ECP_HEADER, *PECP_HEADER;

#define ECP_HEADER_SIZE (sizeof(ECP_HEADER))

#define ECP_HEADER_TO_CONTEXT(H) ((PVOID)((ULONG_PTR)H + ECP_HEADER_SIZE))
#define ECP_CONTEXT_TO_HEADER(C) ((PECP_HEADER)((ULONG_PTR)C - ECP_HEADER_SIZE))

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlRemoveDotsFromPath(IN PWSTR OriginalString,
                        IN USHORT PathLength,
                        OUT USHORT *NewLength)
{
    USHORT Length, ReadPos, WritePos;

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

    for (ReadPos = 0, WritePos = 0; ReadPos < Length; ++WritePos)
    {
        for (; ReadPos > 0 && ReadPos < Length; ++ReadPos)
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

        if (ReadPos >= Length)
        {
            break;
        }

        OriginalString[WritePos] = OriginalString[ReadPos];
        ++ReadPos;
    }

    *NewLength = WritePos * sizeof(WCHAR);

    while (WritePos < Length)
    {
        OriginalString[WritePos++] = UNICODE_NULL;
    }

    return STATUS_SUCCESS;
}

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

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlValidateReparsePointBuffer(IN ULONG BufferLength,
                                IN PREPARSE_DATA_BUFFER ReparseBuffer)
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

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlGetEcpListFromIrp(IN PIRP Irp,
                       OUT PECP_LIST *EcpList)
{
    /* Call Io */
    return IoGetIrpExtraCreateParameter(Irp, EcpList);
}

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlGetNextExtraCreateParameter(IN PECP_LIST EcpList,
                                 IN PVOID CurrentEcpContext,
                                 OUT LPGUID NextEcpType OPTIONAL,
                                 OUT PVOID *NextEcpContext,
                                 OUT PULONG NextEcpContextSize OPTIONAL)
{
    PECP_HEADER CurrentEntry;

    /* If we have no context ... */
    if (CurrentEcpContext == NULL)
    {
        if (IsListEmpty(&EcpList->EcpList))
        {
            goto FailEmpty;
        }

        /* Simply consider first entry */
        CurrentEntry = CONTAINING_RECORD(EcpList->EcpList.Flink, ECP_HEADER, ListEntry);
    }
    else
    {
        /* Otherwise, consider the entry matching the given context */
        CurrentEntry = ECP_CONTEXT_TO_HEADER(CurrentEcpContext);

        /* Make sure we didn't reach the end */
        if (&CurrentEntry->ListEntry == &EcpList->EcpList)
        {
            goto FailEmpty;
        }
    }

    /* We must have an entry */
    if (CurrentEntry == NULL)
    {
        goto FailEmpty;
    }

    /* If caller wants a context, give it */
    if (NextEcpContext != NULL)
    {
        *NextEcpContext = ECP_HEADER_TO_CONTEXT(CurrentEntry);
    }

    /* Same for its size (which the size minus the header overhead) */
    if (NextEcpContextSize != NULL)
    {
         *NextEcpContextSize = CurrentEntry->Size - sizeof(ECP_HEADER);
    }

    /* And copy the type if asked to */
    if (NextEcpType != NULL)
    {
        RtlCopyMemory(NextEcpType, &CurrentEntry->EcpType, sizeof(GUID));
    }

    /* Job done */
    return STATUS_SUCCESS;

    /* Failure case: just zero everything */
FailEmpty:
    if (NextEcpContext != NULL)
    {
        *NextEcpContext = NULL;
    }

    if (NextEcpContextSize != NULL)
    {
        *NextEcpContextSize = 0;
    }

    if (NextEcpType != NULL)
    {
        RtlZeroMemory(NextEcpType, sizeof(GUID));
    }

    /* And return failure */
    return STATUS_NOT_FOUND;
}

