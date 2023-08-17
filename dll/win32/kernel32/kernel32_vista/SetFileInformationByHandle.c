/*

// TODO

 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         BtrFS FSD for ReactOS
 * FILE:            dll/shellext/shellbtrfs/reactos.cpp
 * PURPOSE:         ReactOS glue for Win8.1
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include <k32_vista.h>

/* Quick and dirty table for conversion
   from FILE_INFO_BY_HANDLE_CLASS to FILE_INFORMATION_CLASS */
static
FILE_INFORMATION_CLASS ConvertInfoToInformation[] =
{
    FileBasicInformation,
    FileStandardInformation,
    FileNameInformation,
    FileRenameInformation,
    FileDispositionInformation,
    FileAllocationInformation,
    FileEndOfFileInformation,
    FileStreamInformation,
    FileCompressionInformation,
    FileAttributeTagInformation,
    FileIdBothDirectoryInformation,
    0, // No match for FileIdBothDirectoryRestartInfo
    FileIoPriorityHintInformation, // Vista || ReactOS
    FileRemoteProtocolInformation, // Win7 || ReactOS
    FileFullDirectoryInformation,
    0, // No match for FileFullDirectoryRestartInfo
#if (NTDDI_VERSION >= NTDDI_WIN8) || defined(__REACTOS__)
    0, // No match for FileStorageInfo (Or is it FileStorageReserveIdInformation?)
    FileAlignmentInformation,
    FileIdInformation, // Win8 || ReactOS
    FileIdExtdDirectoryInformation, // Win8 || ReactOS
    0, // No match for FileIdExtdDirectoryRestartInfo
#endif
#if (NTDDI_VERSION >= NTDDI_WIN10_RS1) || defined(__REACTOS__)
    FileDispositionInformationEx, // Win10rs1 || ReactOS
    FileRenameInformationEx, // Win10rs1 || ReactOS
#endif
#if (NTDDI_VERSION >= NTDDI_WIN10_19H1) || defined(__REACTOS__)
    FileCaseSensitiveInformation, // Win10rs4 || ReactOS
    FileNormalizedNameInformation, // Vista || ReactOS
#endif
};
C_ASSERT(_countof(ConvertInfoToInformation) == MaximumFileInfoByHandleClass);

/* Quick implementation, still going farther than Wine implementation */
BOOL
WINAPI
SetFileInformationByHandle(
    _In_ HANDLE hFile,
    _In_range_(0, MaximumFileInfoByHandleClass - 1) FILE_INFO_BY_HANDLE_CLASS FileInfoClass,
    _In_ LPVOID lpFileInformation,
    _In_ DWORD dwBufferSize)
{
    FILE_INFORMATION_CLASS FileInformationClass;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    /* Attempt to convert the class */
    if (FileInfoClass >= 0 && FileInfoClass < MaximumFileInfoByHandleClass)
        FileInformationClass = ConvertInfoToInformation[FileInfoClass];
    else
        FileInformationClass = 0;

    /* If wrong, bail out */
    if (FileInformationClass == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* And set the information */
    Status = NtSetInformationFile(hFile, &IoStatusBlock, lpFileInformation,
                                  dwBufferSize, FileInformationClass);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}
