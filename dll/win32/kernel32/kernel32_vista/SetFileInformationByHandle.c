/*
 * SetFileInformationByHandle
 *
 * Copyright 1993 John Burton
 * Copyright 1996, 2004 Alexandre Julliard
 * Copyright 2008 Jeff Zaroyko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "k32_vista.h"

#include <ndk/rtlfuncs.h>
#include <ndk/iofuncs.h>
#include <string.h>

#define NDEBUG
#include <debug.h>

#undef FIXME
#define FIXME DPRINT1

/* Taken from Wine kernelbase/file.c */

/**
 * @brief
 * Sets information for the file or device that a handle refers to. The Win32
 * class is translated to the matching NT set operation and the caller's
 * buffer is handed to NtSetInformationFile.
 *
 * @param[in]   hFile
 * Handle to the file or device to set information on.
 *
 * @param[in]   FileInformationClass
 * The class of information to set.
 *
 * @param[in]   lpFileInformation
 * Buffer holding the information to set. Its layout depends on the class.
 *
 * @param[in]   dwBufferSize
 * Size of the buffer, in bytes.
 *
 * @return
 * TRUE on success, FALSE on failure. Call GetLastError for the error code.
 *
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetFileInformationByHandle(_In_ HANDLE hFile,
                           _In_ FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
                           _In_reads_bytes_(dwBufferSize) LPVOID lpFileInformation,
                           _In_ DWORD dwBufferSize)
{
    NTSTATUS status;
    IO_STATUS_BLOCK io;

    switch (FileInformationClass)
    {
    /* These are query-only classes, the kernel has no matching set operation
     * to forward them to. */
    case FileNameInfo:
    case FileStreamInfo:
    case FileIdBothDirectoryInfo:
    case FileIdBothDirectoryRestartInfo:
    case FileFullDirectoryInfo:
    case FileFullDirectoryRestartInfo:
    case FileStorageInfo:
    case FileAlignmentInfo:
    case FileIdInfo:
    case FileIdExtdDirectoryInfo:
    case FileIdExtdDirectoryRestartInfo:
    /* These map to set classes that NtSetInformationFile does not implement on
     * ReactOS yet: FileDispositionInformationEx and FileIoPriorityHintInformation
     * both have no entry in IopSetOperationLength[], so there is nothing to
     * forward them to for the moment. */
    case FileDispositionInfoEx:
    case FileIoPriorityHintInfo:
        FIXME("(%p, %u, %p, %lu)\n", hFile, FileInformationClass, lpFileInformation, dwBufferSize);
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;

    case FileBasicInfo:
        status = NtSetInformationFile(hFile, &io, lpFileInformation, dwBufferSize, FileBasicInformation);
        break;

    case FileAllocationInfo:
        status = NtSetInformationFile(hFile, &io, lpFileInformation, dwBufferSize, FileAllocationInformation);
        break;

    case FileEndOfFileInfo:
        status = NtSetInformationFile(hFile, &io, lpFileInformation, dwBufferSize, FileEndOfFileInformation);
        break;

    case FileDispositionInfo:
        status = NtSetInformationFile(hFile, &io, lpFileInformation, dwBufferSize, FileDispositionInformation);
        break;

    case FileRenameInfo:
        {
            FILE_RENAME_INFO *source = lpFileInformation;
            FILE_RENAME_INFO *renameInfo;
            UNICODE_STRING ntName;
            ULONG length;

            /* The fixed part of the structure has to fit in the caller's
             * buffer before FileName can be read from it. */
            if (dwBufferSize < sizeof(FILE_RENAME_INFO))
            {
                SetLastError(ERROR_BAD_LENGTH);
                return FALSE;
            }

            /* A Win32 caller has no directory handle to pass in, Windows
             * rejects a non-NULL RootDirectory instead of interpreting it. */
            if (source->RootDirectory != NULL)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }

            /* A name that already is an NT path has to be forwarded as is:
             * RtlDosPathNameToNtPathName_U would take the leading backslash
             * for a rooted DOS path and prepend the current drive to it. */
            if ((dwBufferSize >= FIELD_OFFSET(FILE_RENAME_INFO, FileName) + 4 * sizeof(WCHAR)) &&
                source->FileName[0] == L'\\' && source->FileName[1] == L'?' &&
                source->FileName[2] == L'?' && source->FileName[3] == L'\\')
            {
                status = NtSetInformationFile(hFile, &io, lpFileInformation, dwBufferSize,
                                              FileRenameInformation);
                break;
            }

            if (!RtlDosPathNameToNtPathName_U(source->FileName, &ntName, NULL, NULL))
            {
                SetLastError(ERROR_PATH_NOT_FOUND);
                return FALSE;
            }

            /* The NT name is longer than the Win32 one the caller supplied,
             * so it needs a buffer of its own. */
            length = FIELD_OFFSET(FILE_RENAME_INFO, FileName) + ntName.Length + sizeof(WCHAR);
            renameInfo = HeapAlloc(GetProcessHeap(), 0, length);
            if (renameInfo != NULL)
            {
                renameInfo->ReplaceIfExists = source->ReplaceIfExists;
                renameInfo->RootDirectory = NULL;
                renameInfo->FileNameLength = ntName.Length;
                memcpy(renameInfo->FileName, ntName.Buffer, ntName.Length + sizeof(WCHAR));

                status = NtSetInformationFile(hFile, &io, renameInfo, length,
                                              FileRenameInformation);
                HeapFree(GetProcessHeap(), 0, renameInfo);
            }
            else
            {
                status = STATUS_NO_MEMORY;
            }

            RtlFreeUnicodeString(&ntName);
            break;
        }

    case FileStandardInfo:
    case FileCompressionInfo:
    case FileAttributeTagInfo:
    case FileRemoteProtocolInfo:
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    return TRUE;
}
