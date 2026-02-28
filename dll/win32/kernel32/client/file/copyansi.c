/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/copyansi.c
 * PURPOSE:         Copying files
 * PROGRAMMER:      Ariadne (ariadne@xs4all.nl)
 */

/* INCLUDES ****************************************************************/

#include <k32.h>

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
CopyFileExA(IN LPCSTR lpExistingFileName,
            IN LPCSTR lpNewFileName,
            IN LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
            IN LPVOID lpData OPTIONAL,
            IN LPBOOL pbCancel OPTIONAL,
            IN DWORD dwCopyFlags)
{
    BOOL Result = FALSE;
    UNICODE_STRING lpNewFileNameW;
    PUNICODE_STRING lpExistingFileNameW;

    lpExistingFileNameW = Basep8BitStringToStaticUnicodeString(lpExistingFileName);
    if (!lpExistingFileNameW)
    {
        return FALSE;
    }

    if (Basep8BitStringToDynamicUnicodeString(&lpNewFileNameW, lpNewFileName))
    {
        Result = CopyFileExW(lpExistingFileNameW->Buffer,
                             lpNewFileNameW.Buffer,
                             lpProgressRoutine,
                             lpData,
                             pbCancel,
                             dwCopyFlags);

        RtlFreeUnicodeString(&lpNewFileNameW);
    }

    return Result;
}


/*
 * @implemented
 */
BOOL
WINAPI
CopyFileA(IN LPCSTR lpExistingFileName,
          IN LPCSTR lpNewFileName,
          IN BOOL bFailIfExists)
{
    BOOL Result = FALSE;
    UNICODE_STRING lpNewFileNameW;
    PUNICODE_STRING lpExistingFileNameW;

    lpExistingFileNameW = Basep8BitStringToStaticUnicodeString(lpExistingFileName);
    if (!lpExistingFileNameW)
    {
        return FALSE;
    }

    if (Basep8BitStringToDynamicUnicodeString(&lpNewFileNameW, lpNewFileName))
    {
        Result = CopyFileExW(lpExistingFileNameW->Buffer,
                             lpNewFileNameW.Buffer,
                             NULL,
                             NULL,
                             NULL,
                             (bFailIfExists ? COPY_FILE_FAIL_IF_EXISTS : 0));

        RtlFreeUnicodeString(&lpNewFileNameW);
    }

    return Result;
}

/* EOF */
