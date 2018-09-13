#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "verpriv.h"
#include "wchar.h"

/* Determine if a file is in use by Windows
 */
BOOL FileInUse(LPWSTR lpszFilePath, LPWSTR lpszFileName)
{
    HANDLE hFile;
    BOOL bResult = FALSE;

    //
    // Attempt to open the file exclusively.
    //

    hFile = CreateFile(lpszFilePath,
                       GENERIC_WRITE | GENERIC_READ,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE) {

        //
        // If the last error is access denied,
        // then the file is in use by someone
        // else.  Return TRUE in this case.
        //

        if (GetLastError() == ERROR_SHARING_VIOLATION)
            bResult = TRUE;

    } else {

        //
        // CreateFile successfully opened the file.
        // Close the handle and return FALSE.
        //

        CloseHandle(hFile);
    }

    return bResult;
}


/* Take a Dir and Filename and make a full path from them
 */
DWORD MakeFileName(LPWSTR lpDst, LPWSTR lpDir, LPWSTR lpFile, int cchDst)
{
  DWORD wDirLen;
  WCHAR cTemp;

  wcsncpy(lpDst, lpDir, cchDst);
  lpDst[cchDst-1] = TEXT('\0');
  wDirLen=wcslen(lpDst);

  if ( wDirLen && (cTemp=*(lpDst+wDirLen-1))!=TEXT('\\') && cTemp!=TEXT(':'))
      lpDst[wDirLen++] = TEXT('\\');

  wcsncpy(lpDst+wDirLen, lpFile, cchDst - wDirLen);
  lpDst[cchDst-1] = TEXT('\0');

  return(wDirLen);
}


/* Given a filename and a list of directories, find the first directory
 * that contains the file, and copy it into the buffer.  Note that in the
 * library version, you can give an environment style path, but not in the
 * DLL version.
 */
INT
GetDirOfFile(LPWSTR lpszFileName,
    LPWSTR lpszPathName,
    DWORD wSize,
    LPWSTR *lplpszDirs)
{
  WCHAR szFileName[_MAX_PATH];
  HANDLE hfRes;
  INT nFileLen = 0;
  INT nPathLen = 0;
  BOOL bDoDefaultOpen = TRUE;
  LPWSTR *lplpFirstDir;
  LPWSTR lpszDir;

  nFileLen = wcslen(lpszFileName);

  for (lplpFirstDir=lplpszDirs; *lplpFirstDir && bDoDefaultOpen;
        ++lplpFirstDir)
    {
      lpszDir = *lplpFirstDir;

      if (nFileLen+wcslen(lpszDir) >= _MAX_PATH-1)
          continue;
      MakeFileName(szFileName, lpszDir, lpszFileName, ARRAYSIZE(szFileName));

TryOpen:
    nPathLen = 0;  // Re-init for this path.

    if ((hfRes = CreateFile(szFileName, GENERIC_READ,
            FILE_SHARE_READ, NULL, OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN, NULL)) != (HANDLE)-1)
        {
          CloseHandle(hfRes);
          for (lpszDir=szFileName; *lpszDir; lpszDir++)
              if (*lpszDir == TEXT('\\'))
                  nPathLen = (INT)(lpszDir - (LPWSTR)szFileName);

          /* This gets rid of the '\' if this is not the root of a drive
           */
          if (nPathLen <= 3)
              ++nPathLen;

          /* Account for the terminating NULL, and make sure wSize is in bounds
           * then NULL terminate the string in the appropriate place so that
           * we can just do an wcscpy.
           */
          --wSize;
          szFileName[(int)wSize<nPathLen ? wSize : nPathLen] = 0;
          wcscpy(lpszPathName, szFileName);

          return(nPathLen);
        }
    }

  if (bDoDefaultOpen)
    {
      bDoDefaultOpen = FALSE;
      wcscpy(szFileName, lpszFileName);
      goto TryOpen;
    }

  return(0);
}


#define GetWindowsDir(x,y,z) GetWindowsDirectory(y,z)
#define GetSystemDir(x,y,z) GetSystemDirectory(y,z)


DWORD
APIENTRY
VerFindFileW(
        DWORD wFlags,
        LPWSTR lpszFileName,
        LPWSTR lpszWinDir,
        LPWSTR lpszAppDir,
        LPWSTR lpszCurDir,
        PUINT puCurDirLen,
        LPWSTR lpszDestDir,
        PUINT puDestDirLen
        )
{
  static WORD wSharedDirLen = 0;
  static WCHAR gszSharedDir[_MAX_PATH];

  WCHAR szSysDir[_MAX_PATH], cTemp;
  WCHAR szWinDir[_MAX_PATH];
  WCHAR szCurDir[_MAX_PATH];
  LPWSTR lpszDir, lpszDirs[4];
  WORD wDestLen, wWinLen, wRetVal = 0, wTemp;
  int nRet;

#ifdef WX86
  //  Save a copy of the 'from Wx86' flag and clear it.
  BOOLEAN UseKnownWx86Dll = NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll;
  NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = FALSE;
#endif

  /* We want to really look in the Windows directory; we don't trust the app
   */
  GetWindowsDir(lpszWinDir ? lpszWinDir : "", szWinDir, _MAX_PATH);
  lpszWinDir = szWinDir;

#ifdef WX86
  NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = UseKnownWx86Dll;
#endif

  if(!GetSystemDir( lpszWinDir, szSysDir, _MAX_PATH))
      wcscpy(szSysDir, lpszWinDir);

#ifdef WX86
  NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = FALSE;
#endif

  if (wFlags & VFFF_ISSHAREDFILE) {
     lpszDirs[0] = lpszWinDir;
     lpszDirs[1] = szSysDir;
     lpszDirs[2] = lpszAppDir;
  } else {
     lpszDirs[0] = lpszAppDir;
     lpszDirs[1] = lpszWinDir;
     lpszDirs[2] = szSysDir;
  }

  lpszDirs[3] = NULL;

  if (!(wTemp=(WORD)GetDirOfFile(lpszFileName, szCurDir, _MAX_PATH, lpszDirs)))
      *szCurDir = 0;
  if (*puCurDirLen > wTemp)
      wcscpy(lpszCurDir, szCurDir);
  else
      wRetVal |= VFF_BUFFTOOSMALL;
  *puCurDirLen = wTemp + 1;

  if (lpszDestDir)
    {
      if (wFlags & VFFF_ISSHAREDFILE)
        {
          if (!wSharedDirLen)
            {
              if ((wWinLen = (WORD)wcslen(lpszWinDir)) &&
                    *(lpszWinDir-1)==TEXT('\\'))
                {
                  if (szSysDir[wWinLen-1] == TEXT('\\'))
                      goto doCompare;
                }
              else if (szSysDir[wWinLen] == TEXT('\\'))
                {
doCompare:
                  cTemp = szSysDir[wWinLen];
                  szSysDir[wWinLen] = 0;
                  nRet = _wcsicmp(lpszWinDir, szSysDir);
                  szSysDir[wWinLen] = cTemp;
                  if(nRet)
                      goto doCopyWinDir;
                  wcscpy(gszSharedDir, szSysDir);
                }
              else
                {
doCopyWinDir:
                  wcscpy(gszSharedDir, lpszWinDir);
                }
              wSharedDirLen = (WORD)wcslen(gszSharedDir);
            }

          wDestLen = wSharedDirLen;
          lpszDir = gszSharedDir;
        }
      else
        {
          wDestLen = (WORD)wcslen(lpszAppDir);
          lpszDir = lpszAppDir;
        }

      if (*puDestDirLen > wDestLen)
        {
          wcscpy(lpszDestDir, lpszDir);

          if ((wWinLen = (WORD)wcslen(lpszDestDir)) &&
                *(lpszDestDir-1)==TEXT('\\'))
              lpszDestDir[wWinLen-1] = 0;

          if (_wcsicmp(lpszCurDir, lpszDestDir))
              wRetVal |= VFF_CURNEDEST;
        }
      else
          wRetVal |= VFF_BUFFTOOSMALL;
      *puDestDirLen = wDestLen + 1;
    }

  if (*szCurDir)
    {
      MakeFileName(szSysDir, szCurDir, lpszFileName, ARRAYSIZE(szSysDir));
      if (FileInUse(szSysDir, lpszFileName))
          wRetVal |= VFF_FILEINUSE;
    }

  return(wRetVal);
}


/*
 *  DWORD
 *  APIENTRY
 *  VerLanguageNameW(
 *      DWORD wLang,
 *      LPWSTR szLang,
 *      DWORD wSize)
 *
 *  This routine was moved to NLSLIB.LIB so that it uses the WINNLS.RC file.
 *  NLSLIB.LIB is part of KERNEL32.DLL.
 */
