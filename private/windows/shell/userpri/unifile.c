/****************************************************************************/
/*                                                                          */
/*  MYFILE.C -                                                              */
/*                                                                          */
/*       My special Unicode workaround file for file I/O function calls     */
/*       from WIN32 Shell applications.                                     */
/*                                                                          */
/*       Created by      :       Diane K. Oh                                */
/*       On Date         :       June 11, 1992                              */
/*                                                                          */
/****************************************************************************/

#include <windows.h>
#include <stdlib.h>

#define MAX_UINT  (0xFFFE)


//*****************************************************************
//
//   MyOpenFile()
//
//   Purpose     : To simulate the effects of OpenFile(),
//                 _lcreat and _lopen in a Uniode environment.
//
//*****************************************************************

HANDLE MyOpenFile (LPTSTR lpszFile, TCHAR * lpszPath, DWORD fuMode)
{
  HANDLE   fh;
  DWORD    len;
  LPTSTR   lpszName;
  TCHAR    szPath[MAX_PATH];
  DWORD    accessMode  = 0;
  DWORD    shareMode   = 0;
  DWORD    createMode  = 0;
  DWORD    fileAttribs = FILE_ATTRIBUTE_NORMAL;


     if (!lpszFile)
        return (INVALID_HANDLE_VALUE);

   //  fuMode of OF_EXIST is looking for the full path name if exist

     if (fuMode & OF_EXIST)
     {
        len = SearchPath (NULL, lpszFile, NULL, MAX_PATH, szPath, &lpszName);

CopyPath:
        if (len)
        {
           if (lpszPath)
              lstrcpy (lpszPath, szPath);
           return ((HANDLE) 1);
        }
        else
           return (INVALID_HANDLE_VALUE);
     }

   //  fuMode of OF_PARSE is looking for the full path name by merging the
   //  current directory

     if (fuMode & OF_PARSE)
     {
        len = GetFullPathName (lpszFile, MAX_PATH, szPath, &lpszName);
        goto CopyPath;
     }

   //  set up all flags passed for create file.

     //  file access flag
     if (fuMode & OF_WRITE)
        accessMode = GENERIC_WRITE;
     else if (fuMode & OF_READWRITE)
        accessMode = GENERIC_READ | GENERIC_WRITE;
     else
        accessMode = GENERIC_READ;

     // file sharing flag
     if (fuMode & OF_SHARE_EXCLUSIVE)
        shareMode = 0;
     else if (fuMode & OF_SHARE_DENY_WRITE)
        shareMode = FILE_SHARE_READ;
     else if (fuMode & OF_SHARE_DENY_READ)
        shareMode = FILE_SHARE_WRITE;
     else
        shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

     // set file creation flag
     if (fuMode & OF_CREATE)
        createMode = CREATE_ALWAYS;
     else
        createMode = OPEN_EXISTING;

   // call CreateFile();

     fh = CreateFile (lpszFile, accessMode, shareMode,
                      NULL, createMode, fileAttribs, NULL);

     if (lpszPath)
        lstrcpy (lpszPath, lpszFile);

     return (fh);

} // end of MyOpenFile()

//*****************************************************************
//
//   MyCloseFile()
//
//   Purpose     : To simulate the effects of _lclose()
//                 in a Uniode environment.
//
//*****************************************************************

BOOL MyCloseFile (HANDLE  hFile)
{
#ifndef  WIN32
    return (!_lclose ((HFILE) hFile));
#else
    return (CloseHandle (hFile));
#endif

} // end of MyCloseFile()

//*****************************************************************
//
//   MyByteReadFile()
//
//   For Win16, will handle > 64k
//
//*****************************************************************

BOOL MyByteReadFile  (HANDLE  hFile, LPVOID  lpBuffer, DWORD  nBytes)
{
  DWORD   nBytesRead;
  LPVOID  hpBuffer = lpBuffer;
  DWORD   dwByteCount = nBytes;

#ifdef WIN32
    return (ReadFile (hFile, lpBuffer, nBytes, &nBytesRead, NULL));
#else
    while (dwByteCount > MAX_UINT)
    {
        if (_lread ((HFILE) hFile, hpBuffer, MAX_UINT) == MAX_UINT)
           return (FALSE);
        dwByteCount -= MAX_UINT;
        hpBuffer += MAX_UINT;
    }
    return (_lread ((HFILE) hFile, lpBuffer, dwBytesCount) == dwBytesCount);
#endif

} // end of MyByteReadFile()

//*****************************************************************
//
//   MyAnsiReadFile()
//
//   Purpose     : To simulate the effects of _lread() in a Unicode
//                 environment by reading into an ANSI buffer and
//                 then converting to Uniode text.
//
//*****************************************************************

BOOL MyAnsiReadFile  (HANDLE  hFile, UINT uCodePage, LPVOID  lpBuffer, DWORD  nBytes)
{
  LPSTR   lpAnsi;
  BOOL    value;

    if (!(lpAnsi = (LPSTR) LocalAlloc (LPTR, nBytes + 1)))
    {
       MessageBox (GetFocus (), NULL, NULL, MB_OK);
       return (FALSE);
    }

    value = MyByteReadFile (hFile, lpAnsi, nBytes);

    if (value)
       MultiByteToWideChar (uCodePage, MB_PRECOMPOSED, lpAnsi, nBytes, lpBuffer, nBytes);

    LocalFree (lpAnsi);

    return (value);

} // end of MyAnsiReadFile()

//*****************************************************************
//
//   MyByteWriteFile()
//
//   For Win16, will handle > 64k
//
//*****************************************************************

BOOL MyByteWriteFile (HANDLE hFile, LPVOID lpBuffer, DWORD nBytes)
{
  DWORD   nBytesWritten;
  LPVOID  hpBuffer = lpBuffer;
  DWORD   dwByteCount = nBytes;

#ifdef WIN32
    return (WriteFile (hFile, lpBuffer, nBytes, &nBytesWritten, NULL));
#else
    while (dwByteCount > MAX_UINT)
    {
        if (_lwrite ((HFILE) hFile, hpBuffer, MAX_UINT) == MAX_UINT)
           return (FALSE);
        dwByteCount -= MAX_UINT;
        hpBuffer += MAX_UINT;
    }
    return (_lwrite ((HFILE) hFile, lpBuffer, dwBytesCount) == dwBytesCount);
#endif

} // end of MyByteWriteFile()

//*****************************************************************
//
//   MyAnsiWriteFile()
//
//   Purpose     : To simulate the effects of _lread() in a Unicode
//                 environment by converting to ANSI buffer and
//                 writing out the ANSI text.
//
//*****************************************************************

BOOL MyAnsiWriteFile (HANDLE  hFile, UINT uCodePage, LPVOID lpBuffer, DWORD nChars)
{
  LPSTR   lpAnsi;
  int     nBytes;
  BOOL    Done, fDefCharUsed;

    nBytes = WideCharToMultiByte (uCodePage, 0, (LPWSTR) lpBuffer, nChars, NULL, 0, NULL, &fDefCharUsed);
    if (!(lpAnsi = (LPSTR) LocalAlloc (LPTR, nBytes + 1)))
    {
       MessageBox (GetFocus (), NULL, NULL, MB_OK);
       return (FALSE);
    }

    WideCharToMultiByte (uCodePage, 0, (LPWSTR) lpBuffer, nChars, lpAnsi, nBytes, NULL, &fDefCharUsed);

    Done = MyByteWriteFile (hFile, lpAnsi, nBytes);

    LocalFree (lpAnsi);

    return (Done);

} // end of MyAnsiWriteFile()

//*****************************************************************
//
//   MyFileSeek()
//
//   Purpose     : To simulate the effects of _lseek() in a Unicode
//                 environment.
//
//*****************************************************************

LONG MyFileSeek (HANDLE hFile, LONG lDistanceToMove, DWORD dwMoveMethod)
{
#ifdef WIN32
    return (SetFilePointer (hFile, lDistanceToMove, NULL, dwMoveMethod));
#else
    return (_llseek ((HFILE) hFile, lDistanceToMove, (int)dwMoveMethod));
#endif

} // end of MyFileSeek()


