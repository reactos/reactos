/*
 * PROJECT:        ReactOS
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * PURPOSE:        Implements tree.com functionality similar to Windows
 * PROGRAMMERS:    Asif Bahrainwala (asif_bahrainwala@hotmail.com)
 */

// Tree.cpp : Defines the entry point for the console application.
//
#include <Windows.h>
#include <stdio.h>

#define STR_MAX 2048

const wchar_t *HELP = L"\nGraphically displays the folder structure of a drive or path.  \n\nTREE [drive:][path] [/F] [/A]\n\n   /F   Display the names of the files in each folder.\n\n\n";
const wchar_t *INVALID = L"No subfolders exist";

static void DrawTree(const wchar_t* strPath, const WIN32_FIND_DATA *arrFolder, const size_t szArr, UINT width, const wchar_t *prevLine, BOOL drawfolder);
static void GetDirectoryStructure(wchar_t* strPath, UINT width, const wchar_t* prevLine);

BOOL bShowFiles = FALSE;  //if this flag is set to true, files will also be listed

/**
* @name: HasSubFolder
*
* @param strPath 
* Must specify folder name
*
* @return
* true if folder has sub folders, else will return false
*/
static BOOL HasSubFolder(const wchar_t *strPath1)
{
    BOOL ret = FALSE;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind = NULL;
    static wchar_t strPath[STR_MAX]= L"";
    ZeroMemory(strPath, sizeof(strPath));

    wcscat(strPath,strPath1);
    wcscat(strPath,L"\\*.");

    hFind=FindFirstFile(strPath, &FindFileData);
    do
    {
        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if(wcscmp(FindFileData.cFileName, L".")==0 ||
              wcscmp(FindFileData.cFileName, L"..")==0 )
            {
                continue;
            }

            ret=TRUE;  //found subfolder
            break;
        }
    }
    while(FindNextFile(hFind, &FindFileData));

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
static void DrawTree(const wchar_t* strPath, const WIN32_FIND_DATA *arrFolder, const size_t szArr, UINT width, const wchar_t *prevLine, BOOL drawfolder)
{
    BOOL bHasSubFolder = HasSubFolder(strPath);
    UINT i = 0;

    //this will format the spaces required for correct formatting
    for(i = 0; i < szArr; ++i)
    {
        wchar_t *consoleOut = (wchar_t*)malloc(sizeof(wchar_t) * STR_MAX);
        UINT j=0;
        static wchar_t str[STR_MAX];

        // As we do not seem to have the _s functions properly set up, use the non-secure version for now
        //wcscpy_s(consoleOut, STR_MAX, L"");
        //wcscpy_s(str, STR_MAX, L"");
        wcscpy(consoleOut, L"");
        wcscpy(str, L"");

        for(j=0;j<width-1;++j)
        {
            //if the previous line has '├' or '│' then the current line will add '│' to continue the connecting line
            if((BYTE)prevLine[j] == 195 || (BYTE)prevLine[j] == 179)
            {
                wchar_t a[]={179,0};
                wcscat(consoleOut,a);
            }
            else
            {
                wcscat(consoleOut,L" ");
            }
        }

        if(szArr - 1 != i)
        {
            if(drawfolder)
            {
                // will add '├───Folder name
                wsprintf(str, L"%c%c%c%c%s", 195, 196, 196, 196, (wchar_t*)arrFolder[i].cFileName);
            }
            else
            {
                if(bHasSubFolder)
                {
                    // will add '│   FileNamw'  //thie line is added to connect the belowfolder sub structure
                    wsprintf(str,L"%c   %s", 179, (wchar_t*)arrFolder[i].cFileName);
                }
                else
                {
                    // will add '    FileNamw'
                    wsprintf(str,L"     %s", (wchar_t*)arrFolder[i].cFileName);
                }
            }
        }
        else
        {
            if(drawfolder)
            {
                // '└───Folder name'
                wsprintf(str, L"%c%c%c%c%s", 192, 196, 196, 196, (wchar_t*)arrFolder[i].cFileName);
            }
            else
            {
                if(bHasSubFolder)
                {
                    // '│   FileName'
                    wsprintf(str,L"%c   %s", 179, (wchar_t*)arrFolder[i].cFileName);
                }
                else
                {
                    // '    FileName'
                    wsprintf(str,L"     %s", (wchar_t*)arrFolder[i].cFileName);
                }
            }
        }

        wcscat(consoleOut, str);
        wprintf(L"%s\n", consoleOut);

        if(drawfolder)
        {
          wchar_t *str = (wchar_t*)malloc(STR_MAX * sizeof(wchar_t));
          ZeroMemory(str, STR_MAX*sizeof(wchar_t));

          wcscat(str, strPath);
          wcscat(str, L"\\");
          wcscat(str, arrFolder[i].cFileName);
          GetDirectoryStructure(str, width+4, consoleOut);

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
static void GetDirectoryStructure(wchar_t* strPath, UINT width, const wchar_t* prevLine)
{
  WIN32_FIND_DATA FindFileData;
  HANDLE hFind = NULL;
  //DWORD err = 0;
  //will fill up with names of all sub folders
  WIN32_FIND_DATA *arrFolder = NULL;
  UINT arrFoldersz = 0;
  //will fill up with names of all sub folders
  WIN32_FIND_DATA *arrFile = NULL;
  UINT arrFilesz = 0;

  ZeroMemory(&FindFileData,sizeof(FindFileData));

  {
    static wchar_t tmp[STR_MAX]=L"";
    ZeroMemory(tmp,sizeof(tmp));
    wcscat(tmp,strPath);
    wcscat(tmp,L"\\*.*");

    hFind=FindFirstFile(tmp, &FindFileData);

    //err = GetLastError(); 
  }

  if(hFind == INVALID_HANDLE_VALUE)
    return;

  do
  {
    if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      if(wcscmp(FindFileData.cFileName, L".")==0 ||
        wcscmp(FindFileData.cFileName, L"..")==0 )
        continue;

      ++arrFoldersz;
      arrFolder=(WIN32_FIND_DATA*)realloc(arrFolder, arrFoldersz * sizeof(FindFileData));

      if(arrFolder == NULL) 
        exit(-1);

      arrFolder[arrFoldersz - 1] = FindFileData;

    }
    else
    {
      ++arrFilesz;
      arrFile=(WIN32_FIND_DATA*)realloc(arrFile, arrFilesz * sizeof(FindFileData));

      if(arrFile == NULL) 
        exit(-1);

      arrFile[arrFilesz - 1] = FindFileData;
    }
  }
  while(FindNextFile(hFind, &FindFileData));

  FindClose(hFind);

  if(bShowFiles)
  {
    DrawTree(strPath, arrFile, arrFilesz, width, prevLine, FALSE);  //will free(arrFile)
  }

  DrawTree(strPath, arrFolder, arrFoldersz, width, prevLine, TRUE);   //will free(arrFile)

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
int wmain( int argc, wchar_t *argv[])
{
    DWORD dwSerial = 0;
    wchar_t t=0;
    wchar_t *strPath = NULL;
    DWORD sz = 0;
    //wchar_t *context = NULL ;
    wchar_t *driveLetter = NULL;

    int i;
    
    for(i = 1; i < argc; ++i)   //parse the command line
    {
        if(wcscmp(argv[i], L"/?") == 0)
        {
            wprintf(HELP);  //will print help and exit after
            return 0;
        } 
        else if(wcscmp(argv[i],L"/F")==0 || wcscmp(argv[i],L"/f")==0)
        {
            bShowFiles=TRUE;  //if set to true, will populate all the files within the folder structure
        }
        else
        {
            //this must be path to some folder
            BOOL b=SetCurrentDirectoryW(argv[i]);  //will set the current directory for this executable
            if(b==FALSE)
            {
                wprintf(INVALID);
                return 1;
            }
        }
    }

    wprintf(L"Folder PATH listing\n");

    GetVolumeInformation(NULL, NULL, 0, &dwSerial, NULL, NULL, NULL, 0);
    wprintf(L"Volume serial number is %x:%x\n", dwSerial >> 16, dwSerial & 0xffff);

    sz = GetCurrentDirectory(1, &t);  //get the buffer size
    strPath = (wchar_t*)malloc(sizeof(wchar_t) * sz);     //must not return before calling delete[]

    GetCurrentDirectory(sz, strPath);  //get the current directory


    driveLetter = (wchar_t*)malloc(sizeof(wchar_t) * sz);  //get the drive letter , must not return before calling delete[]

    // As we do not seem to have the _s functions properly set up, use the non-secure version for now
    //wcscpy_s(driveLetter,sz,strPath);
    //wcstok_s(driveLetter,L":", &context); //parse for the drive letter
    wcscpy(driveLetter,strPath);
    wcstok(driveLetter, L":");

    wprintf(L"%s:.\n",driveLetter);

    free(driveLetter);

    GetDirectoryStructure(strPath, 1, L"          ");  //get the sub directories within this current folder

    free(strPath);
    wprintf(L"\n");

    return 0;
}
