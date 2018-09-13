/** FILE: utiltext.c ******* Module Header ********************************
 *
 *  Control panel utility library routines. This file contains error
 *  reporting message routines, System Time/Date routines, and other
 *  misc. support routines.
 *
 * History:
 *  15:30 on Thur  25 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *
 *  Copyright (C) 1990-1992 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                                Include Files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Local App specific
#include "main.h"

#include <lzexpand.h>

//==========================================================================
//                                Definitions
//==========================================================================
#define MAX_RHS    256

//==========================================================================
//                            External Declarations
//==========================================================================


//==========================================================================
//                            Local Data Declarations
//==========================================================================

short wDateTime[7];                 /* values for first 7 date/time items */
//short wModulos[3] = {23, 59, 59 };  /* highest value for hour, minute, second */
short wPrevDateTime[7];             /* only repaint fields if nec */


//==========================================================================
//                            Local Function Prototypes
//==========================================================================

BOOL RootPath (LPTSTR lpchFile, LPTSTR lppath);
int AlertBox (HWND  hwndParent,
              LPTSTR pszCaption,
              LPTSTR pszText1,
              LPTSTR pszText2,
              unsigned style);

/* ** Post a message box */
int AlertBox (hwndParent, pszCaption, pszText1, pszText2, style)
HWND  hwndParent;
LPTSTR pszCaption;
LPTSTR pszText1;
LPTSTR pszText2;
unsigned style;
{
    TCHAR szMessage[256];

    wsprintf (szMessage, pszText1, pszText2);

    return MessageBox (hwndParent, szMessage, pszCaption, style);
}

int myatoi (LPTSTR pszInt)
{
    int   retval;
    TCHAR cSave;

    for (retval = 0; *pszInt; ++pszInt)
    {
        if ((cSave = (TCHAR) (*pszInt - TEXT('0'))) > (TCHAR) 9)
            break;
        retval = (int) (retval * 10 + (int)cSave);
    }
    return (retval);
}


void ErrMemDlg (HWND hParent)
{
    MessageBox (hParent, szErrMem, szCtlPanel, MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
}

extern LPTIME_ZONE_INFORMATION SelectedTimeZone;

void GetDateTime (void)
{
    SYSTEMTIME UniversalSystemTime;
    SYSTEMTIME SystemTime;

    GetSystemTime (&UniversalSystemTime);

    if (!SystemTimeToTzSpecificLocalTime(SelectedTimeZone,
                                         &UniversalSystemTime,
                                         &SystemTime))
    {
        GetLocalTime (&SystemTime);
    }

    wDateTime[HOUR]    = SystemTime.wHour;
    wDateTime[MINUTE]  = SystemTime.wMinute;
    wDateTime[SECOND]  = SystemTime.wSecond;
    wDateTime[MONTH]   = SystemTime.wMonth;
    wDateTime[DAY]     = SystemTime.wDay;
    wDateTime[YEAR]    = SystemTime.wYear;
    wDateTime[WEEKDAY] = SystemTime.wDayOfWeek;
}

void GetTime (void)
{
    SYSTEMTIME SystemTime;

    GetLocalTime (&SystemTime);

    wDateTime[HOUR]   = SystemTime.wHour;
    wDateTime[MINUTE] = SystemTime.wMinute;
    wDateTime[SECOND] = SystemTime.wSecond;
}

void GetDate (void)
{
    SYSTEMTIME SystemTime;

    GetLocalTime (&SystemTime);

    wDateTime[MONTH]   = SystemTime.wMonth;
    wDateTime[DAY]     = SystemTime.wDay;
    wDateTime[YEAR]    = SystemTime.wYear;
    wDateTime[WEEKDAY] = SystemTime.wDayOfWeek;
}

void SetDateTime (void)
{
    SetTime ();
}

void SetTime (void)
{
    SYSTEMTIME SystemTime;
    SYSTEMTIME UniversalSystemTime;

    SystemTime.wHour   = wDateTime[HOUR];
    SystemTime.wMinute = wDateTime[MINUTE];
    SystemTime.wSecond = wDateTime[SECOND];

    SystemTime.wMilliseconds = 0;

    SystemTime.wMonth  = wDateTime[MONTH];
    SystemTime.wDay    = wDateTime[DAY];
    SystemTime.wYear   = wDateTime[YEAR];

    SetLocalTime (&SystemTime);
}

void SetDate ()
{
    SetTime ();
}

int GetSection(LPTSTR lpFile, LPTSTR lpSection, LPHANDLE hSection, LPINT pSize)
{
    ULONG nCount;
    ULONG nSize;
    HANDLE  hLocal, hTemp;
    TCHAR  *pszSect;

    if (!(hLocal = AllocMem(nSize = 4096)))
        return (0);

    /* Now that a buffer exists, Enumerate all LHS of the section.  If the
     * buffer overflows, reallocate it and try again.
     */
    do
    {
        pszSect = (TCHAR *) hLocal;
        if (!lpFile)
            nCount = GetProfileString (lpSection, NULL, szNull, pszSect, nSize/sizeof(TCHAR));
        else
            nCount = GetPrivateProfileString (lpSection, NULL, szNull, pszSect,
                                              nSize/sizeof(TCHAR), lpFile);
        if (nCount != (nSize/sizeof(TCHAR))-2)
            break;

        if (!(hLocal = ReallocMem(hTemp=hLocal, nSize, nSize+2048)))
        {
            FreeMem (hTemp, nSize+2048);
            return(0);
        }
        nSize += 2048;

    } while (1) ;

    *hSection = hLocal;
    *pSize = nSize;
    return(nCount);
}


HANDLE StringToLocalHandle (LPTSTR lpStr)
{
    HANDLE hStr;
    LPTSTR pStr;

    if (!(hStr = LocalAlloc (LMEM_MOVEABLE, ByteCountOf(lstrlen (lpStr)+1))))
        return (NULL);

    if (!(pStr = LocalLock (hStr)))
    {
        LocalFree (hStr);
        return (NULL);
    }
    lstrcpy (pStr, lpStr);
    LocalUnlock (hStr);
    return (hStr);
}

HANDLE FindRHSIni (LPTSTR pFile, LPTSTR pSection, LPTSTR pRHS)
{
    HANDLE hSection;
    HANDLE hSide = NULL;
    TCHAR szRoot[MAX_RHS];
    TCHAR *pszSect;
    int   nSize;

    if (!pRHS || !(*pRHS) || !(GetSection (pFile, pSection, &hSection, &nSize)))
        goto Error1;

    pszSect = hSection;        /* not tested because GetSection did */

    while (*pszSect)                       /* while not eolist */
    {
    /* Must get the RHS for every LHS.  If it matches, copy LHS into buffer. */
        GetPrivateProfileString (pSection, pszSect, szNull, szRoot,
                                 CharSizeOf(szRoot), pFile);
        if (!(_tcsicmp (szRoot, pRHS)))
        {
            hSide = StringToLocalHandle (pszSect);
            break;
        }
        pszSect += lstrlen (pszSect) + 1; /* advance to next LHS */
    }

    FreeMem (hSection, nSize);
Error1:
    return (hSide);
}

LPTSTR BackslashTerm (LPTSTR pszPath)
{
    LPTSTR pszEnd;

    pszEnd = pszPath + lstrlen (pszPath);
    /* Get the end of the source directory   */

    switch (*CharPrev (pszPath, pszEnd))
    {
    case TEXT('\\'):
    case TEXT(':'):
        break;

    default:
        *pszEnd++ = TEXT('\\');
        *pszEnd = TEXT('\0');
    }
    return (pszEnd);
}

/* Copy file from one directory to another. */
int Copy (HWND hParent, TCHAR *szSrcFile, TCHAR *szDestFile)
{
    TCHAR    szSrcPath[PATHMAX],szDestPath[PATHMAX];
    OFSTRUCT ofstruct;
    int      fhW, fhR;
    long     cch;
    TCHAR    szErrMsg[128];
    BOOL     bErrMem = FALSE;
    FILETIME CreationTime;
    FILETIME LastAccessTime;
    FILETIME LastWriteTime;
    TCHAR    szStr[PATHMAX];


    /* get the real pathnames here */
    if (!RootPath (szSrcFile, szSrcPath)
                || !RootPath (szDestFile, szDestPath))
        return (COPY_DRIVEOPEN);   /* curdir drive door open; already hit abort */

    if ((fhR = LZOpenFile (szSrcPath, &ofstruct, OF_READ)) != -1)
    {
        /* check for illegal copy move */

#ifdef UNICODE
        MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, ofstruct.szPathName, -1, szStr, PATHMAX);
#else
        lstrcpy (szStr, ofstruct.szPathName);
#endif
        if (_tcsicmp (szStr, szDestPath) == 0)
        {
            LZClose (fhR);
            return (COPY_SELF);
        }

        if ((fhW = LZOpenFile (szDestPath, &ofstruct, OF_CREATE | OF_WRITE )) != -1)
        {
            cch = LZCopy (fhR, fhW);  /* LZCopy handles DOS opened files */
            LZClose (fhW);
            LZClose (fhR);
            if (cch >= 0)
            {
              HANDLE  fhR, fhW;

                fhR = MyOpenFile (szSrcPath, NULL, OF_READ);
                fhW = MyOpenFile (szDestPath, NULL, OF_READ);

                GetFileTime (fhR, &CreationTime, &LastAccessTime,
                                                            &LastWriteTime);
                SetFileTime (fhW, &CreationTime, &LastAccessTime,
                                                            &LastWriteTime);

                MyCloseFile (fhW);
                MyCloseFile (fhR);
            }
            else
            {
//                cch -= LZERROR_BADINHANDLE;
                return (cch);
            }
        }
        else
        {
            LZClose (fhR);
            bErrMem = !LoadString (hModule, ERRORS + 13, szErrMsg, CharSizeOf(szErrMsg));
            CharUpper (szDestPath);
            if (bErrMem)
                ErrMemDlg (hParent);
            else
                AlertBox (hParent, szCtlPanel, szErrMsg, szDestPath, MB_OK | MB_ICONEXCLAMATION);
            return (COPY_NOCREATE);
        }
    }
    else
    {
        /* system already threw up a "insert foo in drive a:" msg box */
        return (COPY_CANCEL);
    }
    return (TRUE);
}


BOOL RootPath (lpchFile, lppath)
LPTSTR lpchFile;        /* starting path/filename */
LPTSTR lppath;          /* buffer to fill with fuller pathname */
{

    if (MyOpenFile (lpchFile, lppath, OF_PARSE) != INVALID_HANDLE_VALUE)
       return (TRUE);
    else
       return (FALSE);
}


