/*

// TODO

 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         BtrFS FSD for ReactOS
 * FILE:            dll/shellext/shellbtrfs/reactos.cpp
 * PURPOSE:         ReactOS glue for Win8.1
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include <k32_vista.h>

/* Quick and dirty table for conversion */
FILE_INFORMATION_CLASS ConvertToFileInfo[MaximumFileInfoByHandleClass] =
{
    FileBasicInformation, FileStandardInformation, FileNameInformation, FileRenameInformation,
    FileDispositionInformation, FileAllocationInformation, FileEndOfFileInformation, FileStreamInformation,
    FileCompressionInformation, FileAttributeTagInformation, FileIdBothDirectoryInformation, (FILE_INFORMATION_CLASS)-1,
    FileIoPriorityHintInformation, FileRemoteProtocolInformation
};

/* Quick implementation, still going farther than Wine implementation */
BOOL
WINAPI
SetFileInformationByHandle(HANDLE hFile,
                           FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
                           LPVOID lpFileInformation,
                           DWORD dwBufferSize)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_INFORMATION_CLASS FileInfoClass;

    FileInfoClass = (FILE_INFORMATION_CLASS)-1;

    /* Attempt to convert the class */
    if (FileInformationClass < MaximumFileInfoByHandleClass)
    {
        FileInfoClass = ConvertToFileInfo[FileInformationClass];
    }

    /* If wrong, bail out */
    if (FileInfoClass == -1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* And set the information */
    Status = NtSetInformationFile(hFile, &IoStatusBlock, lpFileInformation,
                                  dwBufferSize, FileInfoClass);

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}
