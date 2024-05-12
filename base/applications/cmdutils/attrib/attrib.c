/*
 * PROJECT:     ReactOS Attrib Command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Displays or changes file attributes recursively.
 * COPYRIGHT:   Copyright 1998-2019 Eric Kohl <eric.kohl@reactos.org>
 *              Copyright 2021 Doug Lyons <douglyons@douglyons.com>
 *              Copyright 2021-2023 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include <stdio.h>
#include <stdlib.h>

#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <winuser.h>
#include <strsafe.h>

#include <conutils.h>

#include "resource.h"

/* Enable to support extended attributes.
 * See https://ss64.com/nt/attrib.html for an exhaustive list. */
// TODO: If you enable this, translations need to be updated as well!
//#define EXTENDED_ATTRIBUTES

#define ALL_FILES_PATTERN   L"*.*" // It may also be possible to use L"*" (shorter)

CON_SCREEN StdOutScreen = INIT_CON_SCREEN(StdOut);

static VOID
ErrorMessage(
    _In_ DWORD dwErrorCode,
    _In_opt_ PCWSTR pszMsg,
    ...)
{
    INT Len;
    va_list arg_ptr;

    if (dwErrorCode == ERROR_SUCCESS)
        return;

    va_start(arg_ptr, pszMsg);
    Len = ConMsgPrintfV(StdErr,
                        FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        dwErrorCode,
                        LANG_USER_DEFAULT,
                        &arg_ptr);
    va_end(arg_ptr);

    /* Fall back just in case the error is not defined */
    if (Len <= 0)
        ConResPrintf(StdErr, STRING_CONSOLE_ERROR, dwErrorCode);

    /* Display the extra optional message if necessary */
    if (pszMsg)
        ConPrintf(StdErr, L"  %s\n", pszMsg);
}


/**
 * @brief   Displays attributes for the given file.
 * @return  Always TRUE (success).
 **/
static BOOL
PrintAttributes(
    _In_ PWIN32_FIND_DATAW pFindData,
    _In_ PCWSTR pszFullName,
    _Inout_opt_ PVOID Context)
{
    DWORD dwAttributes = pFindData->dwFileAttributes;

    UNREFERENCED_PARAMETER(Context);

    ConPrintf(StdOut,
#ifdef EXTENDED_ATTRIBUTES
              L"%c  %c%c%c  %c    %s\n",
#else
              L"%c  %c%c%c     %s\n",
#endif
              (dwAttributes & FILE_ATTRIBUTE_ARCHIVE)  ? L'A' : L' ',
              (dwAttributes & FILE_ATTRIBUTE_SYSTEM)   ? L'S' : L' ',
              (dwAttributes & FILE_ATTRIBUTE_HIDDEN)   ? L'H' : L' ',
              (dwAttributes & FILE_ATTRIBUTE_READONLY) ? L'R' : L' ',
#ifdef EXTENDED_ATTRIBUTES
              (dwAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED) ? L'I' : L' ',
#endif
              pszFullName);

    return TRUE;
}

typedef struct _ATTRIBS_MASKS
{
    DWORD dwMask;
    DWORD dwAttrib;
} ATTRIBS_MASKS, *PATTRIBS_MASKS;

/**
 * @brief   Changes attributes for the given file.
 * @return  TRUE if anything changed, FALSE otherwise.
 **/
static BOOL
ChangeAttributes(
    _In_ PWIN32_FIND_DATAW pFindData,
    _In_ PCWSTR pszFullName,
    _Inout_opt_ PVOID Context)
{
    PATTRIBS_MASKS AttribsMasks = (PATTRIBS_MASKS)Context;
    DWORD dwAttributes;

    dwAttributes = ((pFindData->dwFileAttributes & ~AttribsMasks->dwMask) | AttribsMasks->dwAttrib);
    return SetFileAttributesW(pszFullName, dwAttributes);
}


#define ENUM_RECURSE        0x01
#define ENUM_DIRECTORIES    0x02

typedef BOOL
(*PENUMFILES_CALLBACK)(
    _In_ PWIN32_FIND_DATAW pFindData,
    _In_ PCWSTR pszFullName,
    _Inout_opt_ PVOID Context);

typedef struct _ENUMFILES_CTX
{
    /* Fixed data */
    _In_ PCWSTR FileName;
    _In_ DWORD Flags;

    /* Callback invoked on each enumerated file/directory */
    _In_ PENUMFILES_CALLBACK Callback;
    _In_ PVOID Context;

    /* Dynamic data */
    WIN32_FIND_DATAW findData;
    ULONG uReparseLevel;

    /* The full path buffer the function will act recursively */
    // PWSTR FullPath; // Use a relocated buffer once long paths become supported!
    size_t cchBuffer; // Buffer size
    WCHAR FullPathBuffer[MAX_PATH + _countof("\\" ALL_FILES_PATTERN)];

} ENUMFILES_CTX, *PENUMFILES_CTX;

/* Returns TRUE if anything is done, FALSE otherwise */
static BOOL
EnumFilesWorker(
    _Inout_ PENUMFILES_CTX EnumCtx,
    _Inout_ _off_t offFilePart) // Offset to the file name inside FullPathBuffer
{
    BOOL bFound = FALSE;
    HRESULT hRes;
    HANDLE hFind;
    PWSTR findFileName = EnumCtx->findData.cFileName;
    PWSTR pFilePart = EnumCtx->FullPathBuffer + offFilePart;
    size_t cchRemaining = EnumCtx->cchBuffer - offFilePart;

    /* Recurse over all subdirectories */
    if (EnumCtx->Flags & ENUM_RECURSE)
    {
        /* Append '*.*' */
        hRes = StringCchCopyW(pFilePart, cchRemaining, ALL_FILES_PATTERN);
        if (hRes != S_OK)
        {
            if (hRes == STRSAFE_E_INSUFFICIENT_BUFFER)
            {
                // TODO: If this fails, try to reallocate EnumCtx->FullPathBuffer by
                // increasing its size by _countof(EnumCtx->findData.cFileName) + 1
                // to satisfy this copy, as well as the one made in the loop below.
            }
            // else
            ConPrintf(StdErr, L"Directory level too deep: %s\n", EnumCtx->FullPathBuffer);
            return FALSE;
        }

        hFind = FindFirstFileW(EnumCtx->FullPathBuffer, &EnumCtx->findData);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            DWORD Error = GetLastError();
            if ((Error != ERROR_DIRECTORY) &&
                (Error != ERROR_SHARING_VIOLATION) &&
                (Error != ERROR_FILE_NOT_FOUND))
            {
                ErrorMessage(Error, EnumCtx->FullPathBuffer);
            }
            return FALSE;
        }

        do
        {
            BOOL bIsReparse;
            size_t offNewFilePart;

            if (!(EnumCtx->findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                continue;

            if (!wcscmp(findFileName, L".") || !wcscmp(findFileName, L".."))
                continue;

            /* Allow at most 2 levels of reparse points / symbolic links */
            bIsReparse = !!(EnumCtx->findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT);
            if (bIsReparse)
            {
                if (EnumCtx->uReparseLevel < 2)
                    EnumCtx->uReparseLevel++;
                else
                    continue;
            }

            hRes = StringCchPrintfExW(pFilePart, cchRemaining,
                                      NULL, &offNewFilePart, 0,
                                      L"%s\\", findFileName);
            /* Offset to the new file name part */
            offNewFilePart = EnumCtx->cchBuffer - offNewFilePart;

            bFound |= EnumFilesWorker(EnumCtx, offNewFilePart);

            /* Recalculate the file part pointer and the number of characters
             * remaining: the buffer may have been enlarged and relocated. */
            pFilePart = EnumCtx->FullPathBuffer + offFilePart;
            cchRemaining = EnumCtx->cchBuffer - offFilePart;

            /* If we went through a reparse point / symbolic link, decrease level */
            if (bIsReparse)
                EnumCtx->uReparseLevel--;
        }
        while (FindNextFileW(hFind, &EnumCtx->findData));
        FindClose(hFind);
    }

    /* Append the file name pattern to search for */
    hRes = StringCchCopyW(pFilePart, cchRemaining, EnumCtx->FileName);

    /* Search in the current directory */
    hFind = FindFirstFileW(EnumCtx->FullPathBuffer, &EnumCtx->findData);
    if (hFind == INVALID_HANDLE_VALUE)
        return bFound;

    do
    {
        BOOL bIsDir = !!(EnumCtx->findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        BOOL bExactMatch = (_wcsicmp(findFileName, EnumCtx->FileName) == 0);

        if (bIsDir && !(EnumCtx->Flags & ENUM_DIRECTORIES) && !bExactMatch)
            continue;

        if (!wcscmp(findFileName, L".") || !wcscmp(findFileName, L".."))
            continue;

        /* If we recursively enumerate files excluding directories,
         * exclude any directory from the enumeration. */
        if (bIsDir && !(EnumCtx->Flags & ENUM_DIRECTORIES) && (EnumCtx->Flags & ENUM_RECURSE))
            continue;

        StringCchCopyW(pFilePart, cchRemaining, findFileName);
        /* bFound = */ EnumCtx->Callback(&EnumCtx->findData, EnumCtx->FullPathBuffer, EnumCtx->Context);
        bFound = TRUE;
    }
    while (FindNextFileW(hFind, &EnumCtx->findData));
    FindClose(hFind);

    return bFound;
}

static BOOL
AttribEnumFiles(
    _In_ PCWSTR pszPath,
    _In_ PCWSTR pszFile,
    _In_ DWORD fFlags,
    _In_ PATTRIBS_MASKS AttribsMasks)
{
    ENUMFILES_CTX EnumContext = {0};
    size_t offFilePart;
    HRESULT hRes;

    EnumContext.FileName = pszFile;
    EnumContext.Flags    = fFlags;
    EnumContext.Callback = (AttribsMasks->dwMask == 0 ? PrintAttributes : ChangeAttributes);
    EnumContext.Context  = (AttribsMasks->dwMask == 0 ? NULL : AttribsMasks);

    /* Prepare the full file path buffer */
    EnumContext.cchBuffer = _countof(EnumContext.FullPathBuffer);
    hRes = StringCchCopyExW(EnumContext.FullPathBuffer,
                            EnumContext.cchBuffer,
                            pszPath,
                            NULL,
                            &offFilePart,
                            0);
    if (hRes != S_OK)
        return FALSE;

    /* Offset to the file name part */
    offFilePart = EnumContext.cchBuffer - offFilePart;
    if (EnumContext.FullPathBuffer[offFilePart - 1] != L'\\')
    {
        EnumContext.FullPathBuffer[offFilePart] = L'\\';
        EnumContext.FullPathBuffer[offFilePart + 1] = UNICODE_NULL;
        offFilePart++;
    }

    return EnumFilesWorker(&EnumContext, offFilePart);
}

int wmain(int argc, WCHAR *argv[])
{
    INT i;
    DWORD dwEnumFlags = 0;
    ATTRIBS_MASKS AttribsMasks = {0};
    BOOL bFound = FALSE;
    PWSTR pszFileName;
    WCHAR szFilePath[MAX_PATH + 2] = L""; // + 2 to reserve an extra path separator and a NULL-terminator.

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    /* Check for options and file specifications */
    for (i = 1; i < argc; i++)
    {
        if (*argv[i] == L'/')
        {
            /* Print help and bail out if needed */
            if (wcscmp(argv[i], L"/?") == 0)
            {
                ConResPuts(StdOut, STRING_ATTRIB_HELP);
                return 0;
            }
            else
            /* Retrieve the enumeration modes */
            if (_wcsicmp(argv[i], L"/s") == 0)
                dwEnumFlags |= ENUM_RECURSE;
            else if (_wcsicmp(argv[i], L"/d") == 0)
                dwEnumFlags |= ENUM_DIRECTORIES;
            else
            {
                /* Unknown option */
                ConResPrintf(StdErr, STRING_ERROR_INVALID_PARAM_FORMAT, argv[i]);
                return -1;
            }
        }
        else
        /* Build attributes and mask */
        if ((*argv[i] == L'+') || (*argv[i] == L'-'))
        {
            BOOL bAdd = (*argv[i] == L'+');

            if (wcslen(argv[i]) != 2)
            {
                ConResPrintf(StdErr, STRING_ERROR_INVALID_PARAM_FORMAT, argv[i]);
                return -1;
            }

            switch (towupper(argv[i][1]))
            {
                case L'A':
                    AttribsMasks.dwMask |= FILE_ATTRIBUTE_ARCHIVE;
                    if (bAdd)
                        AttribsMasks.dwAttrib |= FILE_ATTRIBUTE_ARCHIVE;
                    else
                        AttribsMasks.dwAttrib &= ~FILE_ATTRIBUTE_ARCHIVE;
                    break;

                case L'S':
                    AttribsMasks.dwMask |= FILE_ATTRIBUTE_SYSTEM;
                    if (bAdd)
                        AttribsMasks.dwAttrib |= FILE_ATTRIBUTE_SYSTEM;
                    else
                        AttribsMasks.dwAttrib &= ~FILE_ATTRIBUTE_SYSTEM;
                    break;

                case L'H':
                    AttribsMasks.dwMask |= FILE_ATTRIBUTE_HIDDEN;
                    if (bAdd)
                        AttribsMasks.dwAttrib |= FILE_ATTRIBUTE_HIDDEN;
                    else
                        AttribsMasks.dwAttrib &= ~FILE_ATTRIBUTE_HIDDEN;
                    break;

                case L'R':
                    AttribsMasks.dwMask |= FILE_ATTRIBUTE_READONLY;
                    if (bAdd)
                        AttribsMasks.dwAttrib |= FILE_ATTRIBUTE_READONLY;
                    else
                        AttribsMasks.dwAttrib &= ~FILE_ATTRIBUTE_READONLY;
                    break;

#ifdef EXTENDED_ATTRIBUTES
                case L'I':
                    AttribsMasks.dwMask |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
                    if (bAdd)
                        AttribsMasks.dwAttrib |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
                    else
                        AttribsMasks.dwAttrib &= ~FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
                    break;
#endif

                default:
                    ConResPrintf(StdErr, STRING_ERROR_INVALID_PARAM_FORMAT, argv[i]);
                    return -1;
            }
        }
        else
        {
            /* At least one file specification found */
            bFound = TRUE;
        }
    }

    /* If no file specification was found, operate on all files of the current directory */
    if (!bFound)
    {
        GetCurrentDirectoryW(_countof(szFilePath) - 2, szFilePath);
        pszFileName = ALL_FILES_PATTERN;

        bFound = AttribEnumFiles(szFilePath, pszFileName, dwEnumFlags, &AttribsMasks);
        if (!bFound)
            ConResPrintf(StdOut, STRING_FILE_NOT_FOUND, pszFileName);

        return 0;
    }

    /* Operate on each file specification */
    for (i = 1; i < argc; i++)
    {
        /* Skip options */
        if (*argv[i] == L'/' || *argv[i] == L'+' || *argv[i] == L'-')
            continue;

        GetFullPathNameW(argv[i], _countof(szFilePath) - 2, szFilePath, &pszFileName);
        if (pszFileName)
        {
            /* Move the file part so as to separate and NULL-terminate the directory */
            MoveMemory(pszFileName + 1, pszFileName,
                       sizeof(szFilePath) - (pszFileName -szFilePath + 1) * sizeof(*szFilePath));
            *pszFileName++ = UNICODE_NULL;
        }
        else
        {
            pszFileName = L"";
        }

        bFound = AttribEnumFiles(szFilePath, pszFileName, dwEnumFlags, &AttribsMasks);
        if (!bFound)
            ConResPrintf(StdOut, STRING_FILE_NOT_FOUND, argv[i]);
    }

    return 0;
}
