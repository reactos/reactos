/*
 * PROJECT:     ReactOS
 * LICENSE:     GNU GPLv2 only as published by the Free Software Foundation
 * PURPOSE:     Implements tree.com command similar to Windows
 * PROGRAMMERS: Asif Bahrainwala (asif_bahrainwala@hotmail.com)
 */

#include <stdio.h>
#include <stdlib.h>

#include <windef.h>
#include <winbase.h>

#include <conutils.h>

#include "resource.h"

#define STR_MAX 2048

static VOID GetDirectoryStructure(PWSTR strPath, UINT width, PCWSTR prevLine);

/* If this flag is set to true, files will also be listed within the folder structure */
BOOL bShowFiles = FALSE;

/* If this flag is true, ASCII characters will be used instead of UNICODE ones */
BOOL bUseAscii = FALSE;

/**
* @name: HasSubFolder
*
* @param strPath
* Must specify folder name
*
* @return
* true if folder has sub-folders, else will return false
*/
static BOOL HasSubFolder(PCWSTR strPath1)
{
    BOOL ret = FALSE;
    WIN32_FIND_DATAW FindFileData;
    HANDLE hFind = NULL;
    static WCHAR strPath[STR_MAX] = L"";
    ZeroMemory(strPath, sizeof(strPath));

    wcscat(strPath, strPath1);
    wcscat(strPath, L"\\*.");

    hFind = FindFirstFileW(strPath, &FindFileData);
    do
    {
        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (wcscmp(FindFileData.cFileName, L".") == 0 ||
                wcscmp(FindFileData.cFileName, L"..") == 0 )
            {
                continue;
            }

            ret = TRUE;  // Sub-folder found
            break;
        }
    }
    while (FindNextFileW(hFind, &FindFileData));

    FindClose(hFind);
    return ret;
}

/**
 * @name: DrawTree
 *
 * @param strPath
 * Must specify folder name
 *
 * @param arrFolder
 * must be a list of folder names to be drawn in tree format
 *
 * @param width
 * specifies drawing distance for correct formatting of tree structure being drawn on console screen
 * used internally for adding spaces
 *
 * @param prevLine
 * used internally for formatting reasons
 *
 * @return
 * void
 */
static VOID DrawTree(PCWSTR strPath,
                     const WIN32_FIND_DATAW* arrFolder,
                     const size_t szArr,
                     UINT width,
                     PCWSTR prevLine,
                     BOOL drawfolder)
{
    BOOL bHasSubFolder = HasSubFolder(strPath);
    UINT i = 0;

    /* This will format the spaces required for correct formatting */
    for (i = 0; i < szArr; ++i)
    {
        PWSTR consoleOut = (PWSTR)malloc(STR_MAX * sizeof(WCHAR));
        UINT j = 0;
        static WCHAR str[STR_MAX];

        /* As we do not seem to have the _s functions properly set up, use the non-secure version for now */
        //wcscpy_s(consoleOut, STR_MAX, L"");
        //wcscpy_s(str, STR_MAX, L"");
        wcscpy(consoleOut, L"");
        wcscpy(str, L"");

        for (j = 0; j < width - 1; ++j)
        {
            /* If the previous line has '├' or '│' then the current line will
               add '│' to continue the connecting line */
            if (prevLine[j] == L'\x251C' || prevLine[j] == L'\x2502' ||
                prevLine[j] == L'+' || prevLine[j] == L'|')
            {
                if (!bUseAscii)
                {
                    wcscat(consoleOut, L"\x2502");
                }
                else
                {
                    wcscat(consoleOut, L"|");
                }
            }
            else
            {
                wcscat(consoleOut, L" ");
            }
        }

        if (szArr - 1 != i)
        {
            if (drawfolder)
            {
                /* Add '├───Folder name' (\xC3\xC4\xC4\xC4 or \x251C\x2500\x2500\x2500) */
                if (bUseAscii)
                    swprintf(str, L"+---%s", arrFolder[i].cFileName);
                else
                    swprintf(str, L"\x251C\x2500\x2500\x2500%s", arrFolder[i].cFileName);
            }
            else
            {
                if (bHasSubFolder)
                {
                    /* Add '│   FileName' (\xB3 or \x2502) */
                    // This line is added to connect the below-folder sub-structure
                    if (bUseAscii)
                        swprintf(str, L"|   %s", arrFolder[i].cFileName);
                    else
                        swprintf(str, L"\x2502   %s", arrFolder[i].cFileName);
                }
                else
                {
                    /* Add '    FileName' */
                    swprintf(str, L"    %s", arrFolder[i].cFileName);
                }
            }
        }
        else
        {
            if (drawfolder)
            {
                /* '└───Folder name' (\xC0\xC4\xC4\xC4 or \x2514\x2500\x2500\x2500) */
                if (bUseAscii)
                    swprintf(str, L"\\---%s", arrFolder[i].cFileName);
                else
                    swprintf(str, L"\x2514\x2500\x2500\x2500%s", arrFolder[i].cFileName);
            }
            else
            {
                if (bHasSubFolder)
                {
                    /* '│   FileName' (\xB3 or \x2502) */
                    if (bUseAscii)
                        swprintf(str, L"|   %s", arrFolder[i].cFileName);
                    else
                        swprintf(str, L"\x2502   %s", arrFolder[i].cFileName);
                }
                else
                {
                    /* '    FileName' */
                    swprintf(str, L"    %s", arrFolder[i].cFileName);
                }
            }
        }

        wcscat(consoleOut, str);
        ConPrintf(StdOut, L"%s\n", consoleOut);

        if (drawfolder)
        {
            PWSTR str = (PWSTR)malloc(STR_MAX * sizeof(WCHAR));
            ZeroMemory(str, STR_MAX * sizeof(WCHAR));

            wcscat(str, strPath);
            wcscat(str, L"\\");
            wcscat(str, arrFolder[i].cFileName);
            GetDirectoryStructure(str, width + 4, consoleOut);

            free(str);
        }
        free(consoleOut);
    }
}

/**
 * @name: GetDirectoryStructure
 *
 * @param strPath
 * Must specify folder name
 *
 * @param width
 * specifies drawing distance for correct formatting of tree structure being drawn on console screen
 *
 * @param prevLine
 * specifies the previous line written on console, is used for correct formatting
 * @return
 * void
 */
static VOID
GetDirectoryStructure(PWSTR strPath, UINT width, PCWSTR prevLine)
{
    WIN32_FIND_DATAW FindFileData;
    HANDLE hFind = NULL;
    //DWORD err = 0;
    /* Fill up with names of all sub-folders */
    WIN32_FIND_DATAW *arrFolder = NULL;
    UINT arrFoldersz = 0;
    /* Fill up with names of all sub-folders */
    WIN32_FIND_DATAW *arrFile = NULL;
    UINT arrFilesz = 0;

    ZeroMemory(&FindFileData, sizeof(FindFileData));

    {
        static WCHAR tmp[STR_MAX] = L"";
        ZeroMemory(tmp, sizeof(tmp));
        wcscat(tmp, strPath);
        wcscat(tmp, L"\\*.*");
        hFind = FindFirstFileW(tmp, &FindFileData);
        //err = GetLastError();
    }

    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do
    {
        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (wcscmp(FindFileData.cFileName, L".") == 0 ||
                wcscmp(FindFileData.cFileName, L"..") == 0)
                continue;

            ++arrFoldersz;
            arrFolder = (WIN32_FIND_DATAW*)realloc(arrFolder, arrFoldersz * sizeof(FindFileData));

            if (arrFolder == NULL)
                exit(-1);

            arrFolder[arrFoldersz - 1] = FindFileData;

        }
        else
        {
            ++arrFilesz;
            arrFile = (WIN32_FIND_DATAW*)realloc(arrFile, arrFilesz * sizeof(FindFileData));

            if(arrFile == NULL)
                exit(-1);

            arrFile[arrFilesz - 1] = FindFileData;
        }
    }
    while (FindNextFileW(hFind, &FindFileData));

    FindClose(hFind);

    if (bShowFiles)
    {
        /* Will free(arrFile) */
        DrawTree(strPath, arrFile, arrFilesz, width, prevLine, FALSE);
    }

    /* Will free(arrFile) */
    DrawTree(strPath, arrFolder, arrFoldersz, width, prevLine, TRUE);

    free(arrFolder);
    free(arrFile);
}

/**
* @name: main
* standard main functionality as required by C/C++ for application startup
*
* @return
* error /success value
*/
int wmain(int argc, WCHAR* argv[])
{
    DWORD dwSerial = 0;
    WCHAR t = 0;
    PWSTR strPath = NULL;
    DWORD sz = 0;
    //PWSTR context = NULL;
    PWSTR driveLetter = NULL;

    int i;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    /* Parse the command line */
    for (i = 1; i < argc; ++i)
    {
        if (argv[i][0] == L'-' || argv[i][0] == L'/')
        {
            switch (towlower(argv[i][1]))
            {
                case L'?':
                    /* Print help and exit after */
                    ConResPuts(StdOut, IDS_USAGE);
                    return 0;

                case L'f':
                    bShowFiles = TRUE;
                    break;

                case L'a':
                    bUseAscii = TRUE;
                    break;

                default:
                    break;
            }
        }
        else
        {
            /* This must be path to some folder */

            /* Set the current directory for this executable */
            BOOL b = SetCurrentDirectoryW(argv[i]);
            if (b == FALSE)
            {
                ConResPuts(StdOut, IDS_NO_SUBDIRECTORIES);
                return 1;
            }
        }
    }

    ConResPuts(StdOut, IDS_FOLDER_PATH);

    GetVolumeInformationW(NULL, NULL, 0, &dwSerial, NULL, NULL, NULL, 0);
    ConResPrintf(StdOut, IDS_VOL_SERIAL, dwSerial >> 16, dwSerial & 0xffff);

    /* get the buffer size */
    sz = GetCurrentDirectoryW(1, &t);
    /* must not return before calling delete[] */
    strPath = (PWSTR)malloc(sz * sizeof(WCHAR));

    /* get the current directory */
    GetCurrentDirectoryW(sz, strPath);

    /* get the drive letter , must not return before calling delete[] */
    driveLetter = (PWSTR)malloc(sz * sizeof(WCHAR));

    /* As we do not seem to have the _s functions properly set up, use the non-secure version for now */
    //wcscpy_s(driveLetter,sz,strPath);
    //wcstok_s(driveLetter,L":", &context); //parse for the drive letter
    wcscpy(driveLetter, strPath);
    wcstok(driveLetter, L":");

    ConPrintf(StdOut, L"%s:.\n", driveLetter);

    free(driveLetter);

    /* get the sub-directories within this current folder */
    GetDirectoryStructure(strPath, 1, L"          ");

    free(strPath);
    ConPuts(StdOut, L"\n");

    return 0;
}
