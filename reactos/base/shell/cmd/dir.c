/*
 *  DIR.C - dir internal command.
 *
 *
 *  History:
 *
 *    01/29/97 (Tim Norman)
 *        started.
 *
 *    06/13/97 (Tim Norman)
 *      Fixed code.
 *
 *    07/12/97 (Tim Norman)
 *        Fixed bug that caused the root directory to be unlistable
 *
 *    07/12/97 (Marc Desrochers)
 *        Changed to use maxx, maxy instead of findxy()
 *
 *    06/08/98 (Rob Lake)
 *        Added compatibility for /w in dir
 *
 *    06/09/98 (Rob Lake)
 *        Compatibility for dir/s started
 *        Tested that program finds directories off root fine
 *
 *    06/10/98 (Rob Lake)
 *        do_recurse saves the cwd and also stores it in Root
 *        build_tree adds the cwd to the beginning of its' entries
 *        Program runs fine, added print_tree -- works fine.. as EXE,
 *        program won't work properly as COM.
 *
 *    06/11/98 (Rob Lake)
 *        Found problem that caused COM not to work
 *
 *    06/12/98 (Rob Lake)
 *        debugged...
 *        added free mem routine
 *
 *    06/13/98 (Rob Lake)
 *        debugged the free mem routine
 *        debugged whole thing some more
 *        Notes:
 *        ReadDir stores Root name and _Read_Dir does the hard work
 *        PrintDir prints Root and _Print_Dir does the hard work
 *        KillDir kills Root _after_ _Kill_Dir does the hard work
 *        Integrated program into DIR.C(this file) and made some same
 *        changes throughout
 *
 *    06/14/98 (Rob Lake)
 *        Cleaned up code a bit, added comments
 *
 *    06/16/98 (Rob Lake)
 *        Added error checking to my previously added routines
 *
 *    06/17/98 (Rob Lake)
 *        Rewrote recursive functions, again! Most other recursive
 *        functions are now obsolete -- ReadDir, PrintDir, _Print_Dir,
 *        KillDir and _Kill_Dir.  do_recurse does what PrintDir did
 *        and _Read_Dir did what it did before along with what _Print_Dir
 *        did.  Makes /s a lot faster!
 *        Reports 2 more files/dirs that MS-DOS actually reports
 *        when used in root directory(is this because dir defaults
 *        to look for read only files?)
 *        Added support for /b, /a and /l
 *        Made error message similar to DOS error messages
 *        Added help screen
 *
 *    06/20/98 (Rob Lake)
 *        Added check for /-(switch) to turn off previously defined
 *        switches.
 *        Added ability to check for DIRCMD in environment and
 *        process it
 *
 *    06/21/98 (Rob Lake)
 *        Fixed up /B
 *        Now can dir *.ext/X, no spaces!
 *
 *    06/29/98 (Rob Lake)
 *        error message now found in command.h
 *
 *    07/08/1998 (John P. Price)
 *        removed extra returns; closer to MSDOS
 *        fixed wide display so that an extra return is not displayed
 *        when there is five filenames in the last line.
 *
 *    07/12/98 (Rob Lake)
 *        Changed error messages
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *
 *    04-Dec-1998 (Eric Kohl)
 *        Converted source code to Win32, except recursive dir ("dir /s").
 *
 *    10-Dec-1998 (Eric Kohl)
 *        Fixed recursive dir ("dir /s").
 *
 *    14-Dec-1998 (Eric Kohl)
 *        Converted to Win32 directory functions and
 *        fixed some output bugs. There are still some more ;)
 *
 *    10-Jan-1999 (Eric Kohl)
 *        Added "/N" and "/4" options, "/O" is a dummy.
 *        Added locale support.
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Redirection safe!
 *
 *    01-Mar-1999 (Eric Kohl)
 *        Replaced all runtime io functions by their Win32 counterparts.
 *
 *    23-Feb-2001 (Carl Nettelblad <cnettel@hem.passagen.se>)
 *        dir /s now works in deeper trees
 *
 *    28-Jan-2004 (Michael Fritscher <michael@fritscher.net>)
 *        Fix for /p, so it is working under Windows in GUI-mode, too.
 *
 *    30-Apr-2004 (Filip Navara <xnavara@volny.cz>)
 *        Fix /w to print long names.
 *
 *    27-Feb-2005 (Konstantinos Paliouras <squarious@gmail.com>)
 *        Implemented all the switches that were missing, and made
 *        the ros dir very similar to windows dir. Major part of
 *        the code is rewritten. /p is removed, to be rewriten in
 *        the main cmd code.
 *
 *    1-Jul-2004 (Brandon Turner <turnerb7@msu.edu>)
 *        Added /p back in using ConOutPrintfPaging
 *
 *    3-feb-2007 (Paolo Devoti devotip at gmail)
 *        Removed variables formerly in use to handle pagination
 *        Pagination belongs to ConOutPrintfPaging
 *        Removed already commented out code of old pagination
 *
 *    25-Aug-2015 (Pierre Schweitzer)
 *        Implemented /R switch
 */

#include "precomp.h"

#ifdef INCLUDE_CMD_DIR



/* Time Field enumeration */
enum ETimeField
{
    TF_CREATIONDATE     = 0,
    TF_MODIFIEDDATE     = 1,
    TF_LASTACCESSEDDATE = 2
};

/* Ordered by enumeration */
enum EOrderBy
{
    ORDER_NAME      = 0,
    ORDER_SIZE      = 1,
    ORDER_DIRECTORY = 2,
    ORDER_EXTENSION = 3,
    ORDER_TIME      = 4
};

/* The struct for holding the switches */
typedef struct _DirSwitchesFlags
{
    BOOL bBareFormat;   /* Bare Format */
    BOOL bTSeperator;   /* Thousands seperator */
    BOOL bWideList;     /* Wide list format */
    BOOL bWideListColSort;  /* Wide list format but sorted by column */
    BOOL bLowerCase;    /* Uses lower case */
    BOOL bNewLongList;  /* New long list */
    BOOL bPause;        /* Pause per page */
    BOOL bUser;         /* Displays the owner of file */
    BOOL bRecursive;    /* Displays files in specified directory and all sub */
    BOOL bShortName;    /* Displays the sort name of files if exist */
    BOOL b4Digit;       /* Four digit year */
    BOOL bDataStreams;  /* Displays alternate data streams */
    struct
    {
        DWORD dwAttribVal;  /* The desired state of attribute */
        DWORD dwAttribMask; /* Which attributes to check */
    } stAttribs;            /* Displays files with this attributes only */
    struct
    {
        enum EOrderBy eCriteria[3]; /* Criterias used to order by */
        BOOL bCriteriaRev[3];       /* If the criteria is in reversed order */
        short sCriteriaCount;       /* The quantity of criterias */
    } stOrderBy;                    /* Ordered by criterias */
    struct
    {
        enum ETimeField eTimeField; /* The time field that will be used for */
    } stTimeField;                  /* The time field to display or use for sorting */
} DIRSWITCHFLAGS, *LPDIRSWITCHFLAGS;

typedef struct _DIRFINDSTREAMNODE
{
    WIN32_FIND_STREAM_DATA stStreamInfo;
    struct _DIRFINDSTREAMNODE *ptrNext;
} DIRFINDSTREAMNODE, *PDIRFINDSTREAMNODE;

typedef struct _DIRFINDINFO
{
    WIN32_FIND_DATA stFindInfo;
    PDIRFINDSTREAMNODE ptrHead;
} DIRFINDINFO, *PDIRFINDINFO;

typedef struct _DIRFINDLISTNODE
{
    DIRFINDINFO stInfo;
    struct _DIRFINDLISTNODE *ptrNext;
} DIRFINDLISTNODE, *PDIRFINDLISTNODE;

typedef BOOL
(WINAPI *PGETFREEDISKSPACEEX)(LPCTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);

/* Globally save the # of dirs, files and bytes,
 * probabaly later pass them to functions. Rob Lake  */
static ULONG recurse_dir_cnt;
static ULONG recurse_file_cnt;
static ULONGLONG recurse_bytes;

/*
 * help
 *
 * displays help screen for dir
 * Rob Lake
 */
static VOID
DirHelp(VOID)
{
    ConOutResPaging(TRUE, STRING_DIR_HELP1);
}

/*
 * DirReadParameters
 *
 * Parse the parameters and switches of the command line and exports them
 */
static BOOL
DirReadParam(LPTSTR Line,               /* [IN] The line with the parameters & switches */
             LPTSTR** params,           /* [OUT] The parameters after parsing */
             LPINT entries,             /* [OUT] The number of parameters after parsing */
             LPDIRSWITCHFLAGS lpFlags)  /* [IN/OUT] The flags after calculating switches */
{
    TCHAR cCurSwitch;   /* The current switch */
    TCHAR cCurChar;     /* Current examing character */
    TCHAR cCurUChar;    /* Current upper examing character */
    BOOL bNegative;     /* Negative switch */
    BOOL bPNegative;    /* Negative switch parameter */
    BOOL bIntoQuotes;   /* A flag showing if we are in quotes (") */
    LPTSTR ptrStart;    /* A pointer to the first character of a parameter */
    LPTSTR ptrEnd;      /* A pointer to the last character of a parameter */
    BOOL bOrderByNoPar; /* A flag to indicate /O with no switch parameter */
    LPTSTR temp;

    /* Initialize parameter array */
    *params = NULL;
    *entries = 0;

    /* Initialize variables; */
    cCurSwitch = _T(' ');
    bNegative = FALSE;
    bPNegative = FALSE;

    /* We suppose that switch parameters
       were given to avoid setting them to default
       if the switch was not given */
    bOrderByNoPar = FALSE;

    /* Main Loop (see README_DIR.txt) */
    /* scan the command line char per char, and we process its char */
    while (*Line)
    {
        /* we save current character as it is and its upper case */
        cCurChar = *Line;
        cCurUChar = _totupper(*Line);

        /* 1st section (see README_DIR.txt) */
        /* When a switch is expecting */
        if (cCurSwitch == _T('/'))
        {
            while (_istspace(*Line))
                Line++;

            bNegative = (*Line == _T('-'));
            Line += bNegative;

            cCurChar = *Line;
            cCurUChar = _totupper(*Line);

            if ((cCurUChar == _T('A')) ||(cCurUChar == _T('T')) || (cCurUChar == _T('O')))
            {
                /* If positive, prepare for parameters... if negative, reset to defaults */
                switch (cCurUChar)
                {
                case _T('A'):
                    lpFlags->stAttribs.dwAttribVal = 0L;
                    lpFlags->stAttribs.dwAttribMask = 0L;
                    if (bNegative)
                        lpFlags->stAttribs.dwAttribMask = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
                    break;
                case _T('T'):
                    if (bNegative)
                        lpFlags->stTimeField.eTimeField = TF_MODIFIEDDATE;
                    break;
                case _T('O'):
                    bOrderByNoPar = !bNegative;
                    lpFlags->stOrderBy.sCriteriaCount = 0;
                    break;
                }

                if (!bNegative)
                {
                    /* Positive switch, so it can take parameters. */
                    cCurSwitch = cCurUChar;
                    Line++;
                    /* Skip optional leading colon */
                    if (*Line == _T(':'))
                        Line++;
                    continue;
                }
            }
            else if (cCurUChar == _T('L'))
                lpFlags->bLowerCase = ! bNegative;
            else if (cCurUChar == _T('B'))
                lpFlags->bBareFormat = ! bNegative;
            else if (cCurUChar == _T('C'))
                lpFlags->bTSeperator = ! bNegative;
            else if (cCurUChar == _T('W'))
                lpFlags->bWideList = ! bNegative;
            else if (cCurUChar == _T('D'))
                lpFlags->bWideListColSort = ! bNegative;
            else if (cCurUChar == _T('N'))
                lpFlags->bNewLongList = ! bNegative;
            else if (cCurUChar == _T('P'))
                lpFlags->bPause = ! bNegative;
            else if (cCurUChar == _T('Q'))
                lpFlags->bUser = ! bNegative;
            else if (cCurUChar == _T('S'))
                lpFlags->bRecursive = ! bNegative;
            else if (cCurUChar == _T('X'))
                lpFlags->bShortName = ! bNegative;
            else if (cCurChar == _T('R'))
                lpFlags->bDataStreams = ! bNegative;
            else if (cCurChar == _T('4'))
                lpFlags->b4Digit = ! bNegative;
            else if (cCurChar == _T('?'))
            {
                DirHelp();
                return FALSE;
            }
            else
            {
                error_invalid_switch ((TCHAR)_totupper(*Line));
                return FALSE;
            }

            /* Make sure there's no extra characters at the end of the switch */
            if (Line[1] && Line[1] != _T('/') && !_istspace(Line[1]))
            {
                error_parameter_format(Line[1]);
                return FALSE;
            }

            cCurSwitch = _T(' ');
        }
        else if (cCurSwitch == _T(' '))
        {
            /* 2nd section (see README_DIR.txt) */
            /* We are expecting parameter or the unknown */

            if (cCurChar == _T('/'))
                cCurSwitch = _T('/');
            else if (_istspace(cCurChar))
                /* do nothing */;
            else
            {
                /* This is a file/directory name parameter. Find its end */
                ptrStart = Line;
                bIntoQuotes = FALSE;
                while (*Line)
                {
                    if (!bIntoQuotes && (*Line == _T('/') || _istspace(*Line)))
                        break;
                    bIntoQuotes ^= (*Line == _T('"'));
                    Line++;
                }
                ptrEnd = Line;

                /* Copy it to the entries list */
                temp = cmd_alloc((ptrEnd - ptrStart + 1) * sizeof (TCHAR));
                if (!temp)
                    return FALSE;
                memcpy(temp, ptrStart, (ptrEnd - ptrStart) * sizeof (TCHAR));
                temp[ptrEnd - ptrStart] = _T('\0');
                StripQuotes(temp);
                if (!add_entry(entries, params, temp))
                {
                    cmd_free(temp);
                    freep(*params);
                    return FALSE;
                }

                cmd_free(temp);
                continue;
            }
        }
        else
        {
            /* 3rd section (see README_DIR.txt) */
            /* We are waiting for switch parameters */

            /* Check if there are no more switch parameters */
            if ((cCurChar == _T('/')) || _istspace(cCurChar))
            {
                /* Wrong desicion path, reprocess current character */
                cCurSwitch = _T(' ');
                continue;
            }
            /* Process parameter switch */
            switch(cCurSwitch)
            {
            case _T('A'):   /* Switch parameters for /A (attributes filter) */
                if (cCurChar == _T('-'))
                    bPNegative = TRUE;
                else if (cCurUChar == _T('D'))
                {
                    lpFlags->stAttribs.dwAttribMask |= FILE_ATTRIBUTE_DIRECTORY;
                    if (bPNegative)
                        lpFlags->stAttribs.dwAttribVal &= ~FILE_ATTRIBUTE_DIRECTORY;
                    else
                        lpFlags->stAttribs.dwAttribVal |= FILE_ATTRIBUTE_DIRECTORY;
                }
                else if (cCurUChar == _T('R'))
                {
                    lpFlags->stAttribs.dwAttribMask |= FILE_ATTRIBUTE_READONLY;
                    if (bPNegative)
                        lpFlags->stAttribs.dwAttribVal &= ~FILE_ATTRIBUTE_READONLY;
                    else
                        lpFlags->stAttribs.dwAttribVal |= FILE_ATTRIBUTE_READONLY;
                }
                else if (cCurUChar == _T('H'))
                {
                    lpFlags->stAttribs.dwAttribMask |= FILE_ATTRIBUTE_HIDDEN;
                    if (bPNegative)
                        lpFlags->stAttribs.dwAttribVal &= ~FILE_ATTRIBUTE_HIDDEN;
                    else
                        lpFlags->stAttribs.dwAttribVal |= FILE_ATTRIBUTE_HIDDEN;
                }
                else if (cCurUChar == _T('A'))
                {
                    lpFlags->stAttribs.dwAttribMask |= FILE_ATTRIBUTE_ARCHIVE;
                    if (bPNegative)
                        lpFlags->stAttribs.dwAttribVal &= ~FILE_ATTRIBUTE_ARCHIVE;
                    else
                        lpFlags->stAttribs.dwAttribVal |= FILE_ATTRIBUTE_ARCHIVE;
                }
                else if (cCurUChar == _T('S'))
                {
                    lpFlags->stAttribs.dwAttribMask |= FILE_ATTRIBUTE_SYSTEM;
                    if (bPNegative)
                        lpFlags->stAttribs.dwAttribVal &= ~FILE_ATTRIBUTE_SYSTEM;
                    else
                        lpFlags->stAttribs.dwAttribVal |= FILE_ATTRIBUTE_SYSTEM;
                }
                else
                {
                    error_parameter_format((TCHAR)_totupper (*Line));
                    return FALSE;
                }
                break;
            case _T('T'):   /* Switch parameters for /T (time field) */
                if (cCurUChar == _T('C'))
                    lpFlags->stTimeField.eTimeField= TF_CREATIONDATE ;
                else if (cCurUChar == _T('A'))
                    lpFlags->stTimeField.eTimeField= TF_LASTACCESSEDDATE ;
                else if (cCurUChar == _T('W'))
                    lpFlags->stTimeField.eTimeField= TF_MODIFIEDDATE  ;
                else
                {
                    error_parameter_format((TCHAR)_totupper (*Line));
                    return FALSE;
                }
                break;
            case _T('O'):   /* Switch parameters for /O (order) */
                /* Ok a switch parameter was given */
                bOrderByNoPar = FALSE;

                if (cCurChar == _T('-'))
                    bPNegative = TRUE;
                else if (cCurUChar == _T('N'))
                {
                    if (lpFlags->stOrderBy.sCriteriaCount < 3) lpFlags->stOrderBy.sCriteriaCount++;
                    lpFlags->stOrderBy.bCriteriaRev[lpFlags->stOrderBy.sCriteriaCount - 1] = bPNegative;
                    lpFlags->stOrderBy.eCriteria[lpFlags->stOrderBy.sCriteriaCount - 1] = ORDER_NAME;
                }
                else if (cCurUChar == _T('S'))
                {
                    if (lpFlags->stOrderBy.sCriteriaCount < 3) lpFlags->stOrderBy.sCriteriaCount++;
                    lpFlags->stOrderBy.bCriteriaRev[lpFlags->stOrderBy.sCriteriaCount - 1] = bPNegative;
                    lpFlags->stOrderBy.eCriteria[lpFlags->stOrderBy.sCriteriaCount - 1] = ORDER_SIZE;
                }
                else if (cCurUChar == _T('G'))
                {
                    if (lpFlags->stOrderBy.sCriteriaCount < 3) lpFlags->stOrderBy.sCriteriaCount++;
                    lpFlags->stOrderBy.bCriteriaRev[lpFlags->stOrderBy.sCriteriaCount - 1] = bPNegative;
                    lpFlags->stOrderBy.eCriteria[lpFlags->stOrderBy.sCriteriaCount - 1] = ORDER_DIRECTORY;
                }
                else if (cCurUChar == _T('E'))
                {
                    if (lpFlags->stOrderBy.sCriteriaCount < 3) lpFlags->stOrderBy.sCriteriaCount++;
                    lpFlags->stOrderBy.bCriteriaRev[lpFlags->stOrderBy.sCriteriaCount - 1] = bPNegative;
                    lpFlags->stOrderBy.eCriteria[lpFlags->stOrderBy.sCriteriaCount - 1] = ORDER_EXTENSION;
                }
                else if (cCurUChar == _T('D'))
                {
                    if (lpFlags->stOrderBy.sCriteriaCount < 3) lpFlags->stOrderBy.sCriteriaCount++;
                    lpFlags->stOrderBy.bCriteriaRev[lpFlags->stOrderBy.sCriteriaCount - 1] = bPNegative;
                    lpFlags->stOrderBy.eCriteria[lpFlags->stOrderBy.sCriteriaCount - 1] = ORDER_TIME;
                }

                else
                {
                    error_parameter_format((TCHAR)_totupper (*Line));
                    return FALSE;
                }


            }
            /* We check if we calculated the negative value and realese the flag */
            if ((cCurChar != _T('-')) && bPNegative)
                bPNegative = FALSE;
        }

        Line++;
    }

    /* /O with no switch parameters acts like /O:GN */
    if (bOrderByNoPar)
    {
        lpFlags->stOrderBy.sCriteriaCount = 2;
        lpFlags->stOrderBy.eCriteria[0] = ORDER_DIRECTORY;
        lpFlags->stOrderBy.bCriteriaRev[0] = FALSE;
        lpFlags->stOrderBy.eCriteria[1] = ORDER_NAME;
        lpFlags->stOrderBy.bCriteriaRev[1] = FALSE;
    }

    return TRUE;
}

/* Print either with or without paging, depending on /P switch */
static INT
DirPrintf(LPDIRSWITCHFLAGS lpFlags, LPTSTR szFormat, ...)
{
    INT iReturn = 0;
    va_list arg_ptr;
    va_start(arg_ptr, szFormat);
    if (lpFlags->bPause)
        iReturn = ConPrintfPaging(FALSE, szFormat, arg_ptr, STD_OUTPUT_HANDLE);
    else
        ConPrintf(szFormat, arg_ptr, STD_OUTPUT_HANDLE);
    va_end(arg_ptr);
    return iReturn;
}


/*
 * PrintDirectoryHeader
 *
 * print the header for the dir command
 */
static BOOL
PrintDirectoryHeader(LPTSTR szPath, LPDIRSWITCHFLAGS lpFlags)
{
    TCHAR szMsg[RC_STRING_MAX_SIZE];
    TCHAR szFullDir[MAX_PATH];
    TCHAR szRootName[MAX_PATH];
    TCHAR szVolName[80];
    LPTSTR pszFilePart;
    DWORD dwSerialNr;

    if (lpFlags->bBareFormat)
        return TRUE;

    if (GetFullPathName(szPath, sizeof(szFullDir) / sizeof(TCHAR), szFullDir, &pszFilePart) == 0)
    {
        ErrorMessage(GetLastError(), _T("Failed to build full directory path"));
        return FALSE;
    }

    if (pszFilePart != NULL)
    *pszFilePart = _T('\0');

    /* get the media ID of the drive */
    if (!GetVolumePathName(szFullDir, szRootName, sizeof(szRootName) / sizeof(TCHAR)) ||
        !GetVolumeInformation(szRootName, szVolName, 80, &dwSerialNr,
                              NULL, NULL, NULL, 0))
    {
        return TRUE;
    }

    /* print drive info */
    if (szVolName[0] != _T('\0'))
    {
        LoadString(CMD_ModuleHandle, STRING_DIR_HELP2, szMsg, ARRAYSIZE(szMsg));
        DirPrintf(lpFlags, szMsg, szRootName[0], szVolName);
    }
    else
    {
        LoadString(CMD_ModuleHandle, STRING_DIR_HELP3, szMsg, ARRAYSIZE(szMsg));
        DirPrintf(lpFlags, szMsg, szRootName[0]);
    }

    /* print the volume serial number if the return was successful */
    LoadString(CMD_ModuleHandle, STRING_DIR_HELP4, szMsg, ARRAYSIZE(szMsg));
    DirPrintf(lpFlags, szMsg, HIWORD(dwSerialNr), LOWORD(dwSerialNr));

    return TRUE;
}


static VOID
DirPrintFileDateTime(TCHAR *lpDate,
                     TCHAR *lpTime,
                     LPWIN32_FIND_DATA lpFile,
                     LPDIRSWITCHFLAGS lpFlags)
{
    FILETIME ft;
    SYSTEMTIME dt;

    /* Select the right time field */
    switch (lpFlags->stTimeField.eTimeField)
    {
        case TF_CREATIONDATE:
            if (!FileTimeToLocalFileTime(&lpFile->ftCreationTime, &ft))
                return;
            FileTimeToSystemTime(&ft, &dt);
            break;

        case TF_LASTACCESSEDDATE :
            if (!FileTimeToLocalFileTime(&lpFile->ftLastAccessTime, &ft))
                return;
            FileTimeToSystemTime(&ft, &dt);
            break;

        case TF_MODIFIEDDATE:
            if (!FileTimeToLocalFileTime(&lpFile->ftLastWriteTime, &ft))
                return;
            FileTimeToSystemTime(&ft, &dt);
            break;
    }

    FormatDate(lpDate, &dt, lpFlags->b4Digit);
    FormatTime(lpTime, &dt);
}

INT
FormatDate(TCHAR *lpDate, LPSYSTEMTIME dt, BOOL b4Digit)
{
    /* Format date */
    WORD wYear = b4Digit ? dt->wYear : dt->wYear%100;
    switch (nDateFormat)
    {
        case 0: /* mmddyy */
        default:
            return _stprintf(lpDate, _T("%02d%c%02d%c%0*d"),
                    dt->wMonth, cDateSeparator,
                    dt->wDay, cDateSeparator,
                    b4Digit?4:2, wYear);
            break;

        case 1: /* ddmmyy */
            return _stprintf(lpDate, _T("%02d%c%02d%c%0*d"),
                    dt->wDay, cDateSeparator, dt->wMonth,
                    cDateSeparator, b4Digit?4:2, wYear);
            break;

        case 2: /* yymmdd */
            return _stprintf(lpDate, _T("%0*d%c%02d%c%02d"),
                    b4Digit?4:2, wYear, cDateSeparator,
                    dt->wMonth, cDateSeparator, dt->wDay);
            break;
    }
}

INT
FormatTime(TCHAR *lpTime, LPSYSTEMTIME dt)
{
    /* Format Time */
    switch (nTimeFormat)
    {
        case 0: /* 12 hour format */
        default:
            return _stprintf(lpTime,_T("%02d%c%02u %cM"),
                    (dt->wHour == 0 ? 12 : (dt->wHour <= 12 ? dt->wHour : dt->wHour - 12)),
                    cTimeSeparator,
                     dt->wMinute, (dt->wHour <= 11 ? _T('A') : _T('P')));
            break;

        case 1: /* 24 hour format */
            return _stprintf(lpTime, _T("%02d%c%02u"),
                    dt->wHour, cTimeSeparator, dt->wMinute);
            break;
    }
}


static VOID
GetUserDiskFreeSpace(LPCTSTR lpRoot,
                     PULARGE_INTEGER lpFreeSpace)
{
    PGETFREEDISKSPACEEX pGetFreeDiskSpaceEx;
    HINSTANCE hInstance;
    DWORD dwSecPerCl;
    DWORD dwBytPerSec;
    DWORD dwFreeCl;
    DWORD dwTotCl;
    ULARGE_INTEGER TotalNumberOfBytes, TotalNumberOfFreeBytes;

    lpFreeSpace->QuadPart = 0;

    hInstance = GetModuleHandle(_T("KERNEL32"));
    if (hInstance != NULL)
    {
        pGetFreeDiskSpaceEx = (PGETFREEDISKSPACEEX)GetProcAddress(hInstance,
#ifdef _UNICODE
                                                "GetDiskFreeSpaceExW");
#else
                                                "GetDiskFreeSpaceExA");
#endif
        if (pGetFreeDiskSpaceEx != NULL)
        {
            if (pGetFreeDiskSpaceEx(lpRoot, lpFreeSpace, &TotalNumberOfBytes, &TotalNumberOfFreeBytes) == TRUE)
                return;
        }
    }

    GetDiskFreeSpace(lpRoot,
                     &dwSecPerCl,
                     &dwBytPerSec,
                     &dwFreeCl,
                     &dwTotCl);

    lpFreeSpace->QuadPart = dwSecPerCl * dwBytPerSec * dwFreeCl;
}


/*
 * print_summary: prints dir summary
 * Added by Rob Lake 06/17/98 to compact code
 * Just copied Tim's Code and patched it a bit
 */
static INT
PrintSummary(LPTSTR szPath,
             ULONG ulFiles,
             ULONG ulDirs,
             ULONGLONG u64Bytes,
             LPDIRSWITCHFLAGS lpFlags,
             BOOL TotalSummary)
{
    TCHAR szMsg[RC_STRING_MAX_SIZE];
    TCHAR szBuffer[64];
    ULARGE_INTEGER uliFree;

    /* Here we check if we didn't find anything */
    if (!(ulFiles + ulDirs))
    {
        if (!lpFlags->bRecursive || (TotalSummary && lpFlags->bRecursive))
            error_file_not_found();
        return 1;
    }

    /* In bare format we don't print results */
    if (lpFlags->bBareFormat)
        return 0;

    /* Print recursive specific results */

    /* Take this code offline to fix /S does not print duoble info */
    if (TotalSummary && lpFlags->bRecursive)
    {
        ConvertULargeInteger(u64Bytes, szBuffer, sizeof(szBuffer), lpFlags->bTSeperator);
        LoadString(CMD_ModuleHandle, STRING_DIR_HELP5, szMsg, ARRAYSIZE(szMsg));
        DirPrintf(lpFlags, szMsg, ulFiles, szBuffer);
    }
    else
    {
        /* Print File Summary */
        /* Condition to print summary is:
        If we are not in bare format and if we have results! */
        ConvertULargeInteger(u64Bytes, szBuffer, 20, lpFlags->bTSeperator);
        LoadString(CMD_ModuleHandle, STRING_DIR_HELP8, szMsg, ARRAYSIZE(szMsg));
        DirPrintf(lpFlags, szMsg, ulFiles, szBuffer);
    }

    /* Print total directories and freespace */
    if (!lpFlags->bRecursive || TotalSummary)
    {
        GetUserDiskFreeSpace(szPath, &uliFree);
        ConvertULargeInteger(uliFree.QuadPart, szBuffer, sizeof(szBuffer), lpFlags->bTSeperator);
        LoadString(CMD_ModuleHandle, STRING_DIR_HELP6, szMsg, ARRAYSIZE(szMsg));
        DirPrintf(lpFlags, szMsg, ulDirs, szBuffer);
    }

    return 0;
}

/*
 * getExt
 *
 * Get the extension of a filename
 */
TCHAR* getExt(const TCHAR* file)
{
    static TCHAR *NoExt = _T("");
    TCHAR* lastdot = _tcsrchr(file, _T('.'));
    return (lastdot != NULL ? lastdot + 1 : NoExt);
}

/*
 * getName
 *
 * Get the name of the file without extension
 */
static LPTSTR
getName(const TCHAR* file, TCHAR * dest)
{
    INT_PTR iLen;
    LPTSTR end;

    /* Check for "." and ".." folders */
    if ((_tcscmp(file, _T(".")) == 0) ||
        (_tcscmp(file, _T("..")) == 0))
    {
        _tcscpy(dest,file);
        return dest;
    }

    end = _tcsrchr(file, _T('.'));
    if (!end)
        iLen = _tcslen(file);
    else
        iLen = (end - file);

    _tcsncpy(dest, file, iLen);
    *(dest + iLen) = _T('\0');

    return dest;
}


/*
 *  DirPrintNewList
 *
 * The function that prints in new style
 */
static VOID
DirPrintNewList(PDIRFINDINFO ptrFiles[],        /* [IN]Files' Info */
                DWORD dwCount,                  /* [IN] The quantity of files */
                TCHAR *szCurPath,               /* [IN] Full path of current directory */
                LPDIRSWITCHFLAGS lpFlags)       /* [IN] The flags used */
{
    DWORD i;
    TCHAR szSize[30];
    TCHAR szShortName[15];
    TCHAR szDate[20];
    TCHAR szTime[20];
    INT iSizeFormat;
    ULARGE_INTEGER u64FileSize;
    PDIRFINDSTREAMNODE ptrCurStream;

    for (i = 0; i < dwCount && !bCtrlBreak; i++)
    {
        /* Calculate size */
        if (ptrFiles[i]->stFindInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            /* Junction */
            iSizeFormat = -14;
            _tcscpy(szSize, _T("<JUNCTION>"));
        }
        else if (ptrFiles[i]->stFindInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            /* Directory */
            iSizeFormat = -14;
            _tcscpy(szSize, _T("<DIR>"));
        }
        else
        {
            /* File */
            iSizeFormat = 14;
            u64FileSize.HighPart = ptrFiles[i]->stFindInfo.nFileSizeHigh;
            u64FileSize.LowPart = ptrFiles[i]->stFindInfo.nFileSizeLow;
            ConvertULargeInteger(u64FileSize.QuadPart, szSize, 20, lpFlags->bTSeperator);
        }

        /* Calculate short name */
        szShortName[0] = _T('\0');
        if (lpFlags->bShortName)
            _stprintf(szShortName, _T(" %-12s"), ptrFiles[i]->stFindInfo.cAlternateFileName);

        /* Format date and time */
        DirPrintFileDateTime(szDate, szTime, &ptrFiles[i]->stFindInfo, lpFlags);

        /* Print the line */
        DirPrintf(lpFlags, _T("%10s  %-6s    %*s%s %s\n"),
                  szDate,
                  szTime,
                  iSizeFormat,
                  szSize,
                  szShortName,
                  ptrFiles[i]->stFindInfo.cFileName);

        /* Now, loop on the streams */
        ptrCurStream = ptrFiles[i]->ptrHead;
        while (ptrCurStream)
        {
            ConvertULargeInteger(ptrCurStream->stStreamInfo.StreamSize.QuadPart, szSize, 20, lpFlags->bTSeperator);

            /* Print the line */
            DirPrintf(lpFlags, _T("%10s  %-6s    %*s%s %s%s\n"),
                      L"",
                      L"",
                      16,
                      szSize,
                      L"",
                      ptrFiles[i]->stFindInfo.cFileName,
                      ptrCurStream->stStreamInfo.cStreamName);
            ptrCurStream = ptrCurStream->ptrNext;
        }
    }
}


/*
 *  DirPrintWideList
 *
 * The function that prints in wide list
 */
static VOID
DirPrintWideList(PDIRFINDINFO ptrFiles[],       /* [IN] Files' Info */
                 DWORD dwCount,                 /* [IN] The quantity of files */
                 TCHAR *szCurPath,              /* [IN] Full path of current directory */
                 LPDIRSWITCHFLAGS lpFlags)      /* [IN] The flags used */
{
    SHORT iScreenWidth;
    USHORT iColumns;
    USHORT iLines;
    UINT_PTR iLongestName;
    TCHAR szTempFname[MAX_PATH];
    DWORD i;
    DWORD j;
    DWORD temp;

    /* Calculate longest name */
    iLongestName = 1;
    for (i = 0; i < dwCount; i++)
    {
        if (ptrFiles[i]->stFindInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            /* Directories need 2 additinal characters for brackets */
            if ((_tcslen(ptrFiles[i]->stFindInfo.cFileName) + 2) > iLongestName)
                iLongestName = _tcslen(ptrFiles[i]->stFindInfo.cFileName) + 2;
        }
        else
        {
            if (_tcslen(ptrFiles[i]->stFindInfo.cFileName) > iLongestName)
                iLongestName = _tcslen(ptrFiles[i]->stFindInfo.cFileName);
        }
    }

    /* Count the highest number of columns */
    GetScreenSize(&iScreenWidth, 0);
    iColumns = (USHORT)(iScreenWidth / iLongestName);

    /* Check if there is enough space for spaces between names */
    if (((iLongestName * iColumns) + iColumns) >= (UINT)iScreenWidth)
        iColumns --;

    /* A last check at iColumns to avoid division by zero */
    if (!iColumns) iColumns = 1;

    /* Calculate the lines that will be printed */
    iLines = (USHORT)((dwCount + iColumns - 1) / iColumns);

    for (i = 0; i < iLines && !bCtrlBreak; i++)
    {
        for (j = 0; j < iColumns; j++)
        {
            if (lpFlags->bWideListColSort)
            {
                /* Print Column sorted */
                temp = (j * iLines) + i;
            }
            else
            {
                /* Print Line sorted */
                temp = (i * iColumns) + j;
            }

            if (temp >= dwCount) break;

            if (ptrFiles[temp]->stFindInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                _stprintf(szTempFname, _T("[%s]"), ptrFiles[temp]->stFindInfo.cFileName);
            else
                _stprintf(szTempFname, _T("%s"), ptrFiles[temp]->stFindInfo.cFileName);

            DirPrintf(lpFlags, _T("%-*s"), iLongestName + 1, szTempFname);
        }

        /* Add a new line after the last item in the column */
        DirPrintf(lpFlags, _T("\n"));
    }
}


/*
 *  DirPrintOldList
 *
 * The function that prints in old style
 */
static VOID
DirPrintOldList(PDIRFINDINFO ptrFiles[],        /* [IN] Files' Info */
                DWORD dwCount,                  /* [IN] The quantity of files */
                TCHAR * szCurPath,              /* [IN] Full path of current directory */
                LPDIRSWITCHFLAGS lpFlags)       /* [IN] The flags used */
{
    DWORD i;                        /* An indexer for "for"s */
    TCHAR szName[10];               /* The name of file */
    TCHAR szExt[5];                 /* The extension of file */
    TCHAR szDate[30],szTime[30];    /* Used to format time and date */
    TCHAR szSize[30];               /* The size of file */
    int iSizeFormat;                /* The format of size field */
    ULARGE_INTEGER u64FileSize;     /* The file size */

    for (i = 0; i < dwCount && !bCtrlBreak; i++)
    {
        /* Broke 8.3 format */
        if (*ptrFiles[i]->stFindInfo.cAlternateFileName )
        {
            /* If the file is long named then we read the alter name */
            getName( ptrFiles[i]->stFindInfo.cAlternateFileName, szName);
            _tcscpy(szExt, getExt( ptrFiles[i]->stFindInfo.cAlternateFileName));
        }
        else
        {
            /* If the file is not long name we read its original name */
            getName( ptrFiles[i]->stFindInfo.cFileName, szName);
            _tcscpy(szExt, getExt( ptrFiles[i]->stFindInfo.cFileName));
        }

        /* Calculate size */
        if (ptrFiles[i]->stFindInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            /* Directory, no size it's a directory*/
            iSizeFormat = -17;
            _tcscpy(szSize, _T("<DIR>"));
        }
        else
        {
            /* File */
            iSizeFormat = 17;
            u64FileSize.HighPart = ptrFiles[i]->stFindInfo.nFileSizeHigh;
            u64FileSize.LowPart = ptrFiles[i]->stFindInfo.nFileSizeLow;
            ConvertULargeInteger(u64FileSize.QuadPart, szSize, 20, lpFlags->bTSeperator);
        }

        /* Format date and time */
        DirPrintFileDateTime(szDate,szTime,&ptrFiles[i]->stFindInfo,lpFlags);

        /* Print the line */
        DirPrintf(lpFlags, _T("%-8s %-3s  %*s %s  %s\n"),
                  szName,       /* The file's 8.3 name */
                  szExt,        /* The file's 8.3 extension */
                  iSizeFormat,  /* print format for size column */
                  szSize,       /* The size of file or "<DIR>" for dirs */
                  szDate,       /* The date of file/dir */
                  szTime);      /* The time of file/dir */
    }
}

/*
 *  DirPrintBareList
 *
 * The function that prints in bare format
 */
static VOID
DirPrintBareList(PDIRFINDINFO ptrFiles[],       /* [IN] Files' Info */
                 DWORD dwCount,                 /* [IN] The number of files */
                 LPTSTR lpCurPath,              /* [IN] Full path of current directory */
                 LPDIRSWITCHFLAGS lpFlags)      /* [IN] The flags used */
{
    DWORD i;

    for (i = 0; i < dwCount && !bCtrlBreak; i++)
    {
        if ((_tcscmp(ptrFiles[i]->stFindInfo.cFileName, _T(".")) == 0) ||
            (_tcscmp(ptrFiles[i]->stFindInfo.cFileName, _T("..")) == 0))
        {
            /* at bare format we don't print "." and ".." folder */
            continue;
        }
        if (lpFlags->bRecursive)
        {
            /* at recursive mode we print full path of file */
            DirPrintf(lpFlags, _T("%s\\%s\n"), lpCurPath, ptrFiles[i]->stFindInfo.cFileName);
        }
        else
        {
            /* if we are not in recursive mode we print the file names */
            DirPrintf(lpFlags, _T("%s\n"), ptrFiles[i]->stFindInfo.cFileName);
        }
    }
}


/*
 * DirPrintFiles
 *
 * The functions that prints the files list
 */
static VOID
DirPrintFiles(PDIRFINDINFO ptrFiles[],      /* [IN] Files' Info */
              DWORD dwCount,                /* [IN] The quantity of files */
              TCHAR *szCurPath,             /* [IN] Full path of current directory */
              LPDIRSWITCHFLAGS lpFlags)     /* [IN] The flags used */
{
    TCHAR szMsg[RC_STRING_MAX_SIZE];
    TCHAR szTemp[MAX_PATH]; /* A buffer to format the directory header */

    /* Print trailing backslash for root directory of drive */
    _tcscpy(szTemp, szCurPath);
    if (_tcslen(szTemp) == 2 && szTemp[1] == _T(':'))
        _tcscat(szTemp, _T("\\"));

    /* Condition to print header:
       We are not printing in bare format
       and if we are in recursive mode... we must have results */
    if (!(lpFlags->bBareFormat ) && !((lpFlags->bRecursive) && (dwCount <= 0)))
    {
        LoadString(CMD_ModuleHandle, STRING_DIR_HELP7, szMsg, ARRAYSIZE(szMsg));
        if (DirPrintf(lpFlags, szMsg, szTemp))
            return;
    }

    if (lpFlags->bBareFormat)
    {
        /* Bare format */
        DirPrintBareList(ptrFiles, dwCount, szCurPath, lpFlags);
    }
    else if (lpFlags->bShortName)
    {
        /* New list style / Short names */
        DirPrintNewList(ptrFiles, dwCount, szCurPath, lpFlags);
    }
    else if (lpFlags->bWideListColSort || lpFlags->bWideList)
    {
        /* Wide list */
        DirPrintWideList(ptrFiles, dwCount, szCurPath, lpFlags);
    }
    else if (lpFlags->bNewLongList )
    {
        /* New list style*/
        DirPrintNewList(ptrFiles, dwCount, szCurPath, lpFlags);
    }
    else
    {
        /* If nothing is selected old list is the default */
        DirPrintOldList(ptrFiles, dwCount, szCurPath, lpFlags);
    }
}

/*
 * CompareFiles
 *
 * Compares 2 files based on the order criteria
 */
static BOOL
CompareFiles(PDIRFINDINFO lpFile1,      /* [IN] A pointer to WIN32_FIND_DATA of file 1 */
             PDIRFINDINFO lpFile2,      /* [IN] A pointer to WIN32_FIND_DATA of file 2 */
             LPDIRSWITCHFLAGS lpFlags)  /* [IN] The flags that we use to list */
{
  ULARGE_INTEGER u64File1;
  ULARGE_INTEGER u64File2;
  int i;
  long iComp = 0;   /* The comparison result */

    /* Calculate critiries by order given from user */
    for (i = 0;i < lpFlags->stOrderBy.sCriteriaCount;i++)
    {

        /* Calculate criteria */
        switch(lpFlags->stOrderBy.eCriteria[i])
        {
        case ORDER_SIZE:        /* Order by size /o:s */
            /* concat the 32bit integers to a 64bit */
            u64File1.LowPart = lpFile1->stFindInfo.nFileSizeLow;
            u64File1.HighPart = lpFile1->stFindInfo.nFileSizeHigh;
            u64File2.LowPart = lpFile2->stFindInfo.nFileSizeLow;
            u64File2.HighPart = lpFile2->stFindInfo.nFileSizeHigh;

            /* In case that differnce is too big for a long */
            if (u64File1.QuadPart < u64File2.QuadPart)
                iComp = -1;
            else if (u64File1.QuadPart > u64File2.QuadPart)
                iComp = 1;
            else
                iComp = 0;
            break;

        case ORDER_DIRECTORY:   /* Order by directory attribute /o:g */
            iComp = ((lpFile2->stFindInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)-
                (lpFile1->stFindInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
            break;

        case ORDER_EXTENSION:   /* Order by extension name /o:e */
            iComp = _tcsicmp(getExt(lpFile1->stFindInfo.cFileName),getExt(lpFile2->stFindInfo.cFileName));
            break;

        case ORDER_NAME:        /* Order by filename /o:n */
            iComp = _tcsicmp(lpFile1->stFindInfo.cFileName, lpFile2->stFindInfo.cFileName);
            break;

        case ORDER_TIME:        /* Order by file's time /o:t */
            /* We compare files based on the time field selected by /t */
            switch(lpFlags->stTimeField.eTimeField)
            {
            case TF_CREATIONDATE:
                /* concat the 32bit integers to a 64bit */
                u64File1.LowPart = lpFile1->stFindInfo.ftCreationTime.dwLowDateTime;
                u64File1.HighPart = lpFile1->stFindInfo.ftCreationTime.dwHighDateTime ;
                u64File2.LowPart = lpFile2->stFindInfo.ftCreationTime.dwLowDateTime;
                u64File2.HighPart = lpFile2->stFindInfo.ftCreationTime.dwHighDateTime ;
                break;
            case TF_LASTACCESSEDDATE :
                /* concat the 32bit integers to a 64bit */
                u64File1.LowPart = lpFile1->stFindInfo.ftLastAccessTime.dwLowDateTime;
                u64File1.HighPart = lpFile1->stFindInfo.ftLastAccessTime.dwHighDateTime ;
                u64File2.LowPart = lpFile2->stFindInfo.ftLastAccessTime.dwLowDateTime;
                u64File2.HighPart = lpFile2->stFindInfo.ftLastAccessTime.dwHighDateTime ;
                break;
            case TF_MODIFIEDDATE:
                /* concat the 32bit integers to a 64bit */
                u64File1.LowPart = lpFile1->stFindInfo.ftLastWriteTime.dwLowDateTime;
                u64File1.HighPart = lpFile1->stFindInfo.ftLastWriteTime.dwHighDateTime ;
                u64File2.LowPart = lpFile2->stFindInfo.ftLastWriteTime.dwLowDateTime;
                u64File2.HighPart = lpFile2->stFindInfo.ftLastWriteTime.dwHighDateTime ;
                break;
            }

            /* In case that differnce is too big for a long */
            if (u64File1.QuadPart < u64File2.QuadPart)
                iComp = -1;
            else if (u64File1.QuadPart > u64File2.QuadPart)
                iComp = 1;
            else
                iComp = 0;
            break;
        }

        /* Reverse if desired */
        if (lpFlags->stOrderBy.bCriteriaRev[i])
            iComp *= -1;

        /* If that criteria was enough for distinguishing
           the files/dirs,there is no need to calculate the others*/
        if (iComp != 0) break;
    }

    /* Translate the value of iComp to boolean */
    return iComp > 0;
}

/*
 * QsortFiles
 *
 * Sort files by the order criterias using quicksort method
 */
static VOID
QsortFiles(PDIRFINDINFO ptrArray[],         /* [IN/OUT] The array with file info pointers */
           int i,                           /* [IN]     The index of first item in array */
           int j,                           /* [IN]     The index to last item in array */
           LPDIRSWITCHFLAGS lpFlags)        /* [IN]     The flags that we will use to sort */
{
    PDIRFINDINFO lpTemp;   /* A temporary pointer */
    BOOL Way;

    if (i < j)
    {
        int First = i, Last = j, Temp;
        Way = TRUE;
        while (i != j)
        {
            if (Way == CompareFiles(ptrArray[i], ptrArray[j], lpFlags))
            {
                /* Swap the pointers of the array */
                lpTemp = ptrArray[i];
                ptrArray[i]= ptrArray[j];
                ptrArray[j] = lpTemp;

                /* Swap the indexes for inverting sorting */
                Temp = i;
                i = j;
                j =Temp;

                Way = !Way;
            }

            j += (!Way - Way);
        }

        QsortFiles(ptrArray,First, i-1, lpFlags);
        QsortFiles(ptrArray,i+1,Last, lpFlags);
    }
}

/*
 * DirList
 *
 * The functions that does everything except for printing results
 */
static INT
DirList(LPTSTR szPath,              /* [IN] The path that dir starts */
        LPDIRSWITCHFLAGS lpFlags)   /* [IN] The flags of the listing */
{
    BOOL fPoint;                        /* If szPath is a file with extension fPoint will be True*/
    HANDLE hSearch;                     /* The handle of the search */
    HANDLE hRecSearch;                  /* The handle for searching recursivly */
    HANDLE hStreams;                    /* The handle for alternate streams */
    WIN32_FIND_DATA wfdFileInfo;        /* The info of file that found */
    PDIRFINDINFO * ptrFileArray;        /* An array of pointers with all the files */
    PDIRFINDLISTNODE ptrStartNode;      /* The pointer to the first node */
    PDIRFINDLISTNODE ptrNextNode;       /* A pointer used for relatives refernces */
    TCHAR szFullPath[MAX_PATH];         /* The full path that we are listing with trailing \ */
    TCHAR szSubPath[MAX_PATH];
    LPTSTR pszFilePart;
    DWORD dwCount;                      /* A counter of files found in directory */
    DWORD dwCountFiles;                 /* Counter for files */
    DWORD dwCountDirs;                  /* Counter for directories */
    ULONGLONG u64CountBytes;            /* Counter for bytes */
    ULARGE_INTEGER u64Temp;             /* A temporary counter */
    WIN32_FIND_STREAM_DATA wfsdStreamInfo;
    PDIRFINDSTREAMNODE * ptrCurNode;    /* The pointer to the first stream */
    PDIRFINDSTREAMNODE ptrFreeNode;     /* The pointer used during cleanup */

    /* Initialize Variables */
    ptrStartNode = NULL;
    ptrNextNode = NULL;
    dwCount = 0;
    dwCountFiles = 0;
    dwCountDirs = 0;
    u64CountBytes = 0;
    fPoint= FALSE;

    /* Create szFullPath */
    if (GetFullPathName(szPath, sizeof(szFullPath) / sizeof(TCHAR), szFullPath, &pszFilePart) == 0)
    {
        _tcscpy (szFullPath, szPath);
        pszFilePart = NULL;
    }

    /* If no wildcard or file was specified and this is a directory, then
       display all files in it */
    if (pszFilePart == NULL || IsExistingDirectory(szFullPath))
    {
        pszFilePart = &szFullPath[_tcslen(szFullPath)];
        if (pszFilePart[-1] != _T('\\'))
            *pszFilePart++ = _T('\\');
        _tcscpy(pszFilePart, _T("*"));
    }

    /* Prepare the linked list, first node is allocated */
    ptrStartNode = cmd_alloc(sizeof(DIRFINDLISTNODE));
    if (ptrStartNode == NULL)
    {
        WARN("DEBUG: Cannot allocate memory for ptrStartNode!\n");
        return 1;   /* Error cannot allocate memory for 1st object */
    }
    ptrNextNode = ptrStartNode;

    /*Checking ir szPath is a File with/wout extension*/
    if (szPath[_tcslen(szPath) - 1] == _T('.'))
        fPoint= TRUE;

    /* Collect the results for the current folder */
    hSearch = FindFirstFile(szFullPath, &wfdFileInfo);
    if (hSearch != INVALID_HANDLE_VALUE)
    {
        do
        {
            /*If retrieved FileName has extension,and szPath doesnt have extension then JUMP the retrieved FileName*/
            if (_tcschr(wfdFileInfo.cFileName,_T('.'))&&(fPoint==TRUE))
            {
                continue;
            /* Here we filter all the specified attributes */
            }
            else if ((wfdFileInfo.dwFileAttributes & lpFlags->stAttribs.dwAttribMask )
                    == (lpFlags->stAttribs.dwAttribMask & lpFlags->stAttribs.dwAttribVal ))
            {
                ptrNextNode->ptrNext = cmd_alloc(sizeof(DIRFINDLISTNODE));
                if (ptrNextNode->ptrNext == NULL)
                {
                    WARN("DEBUG: Cannot allocate memory for ptrNextNode->ptrNext!\n");
                    while (ptrStartNode)
                    {
                        ptrNextNode = ptrStartNode->ptrNext;
                        while (ptrStartNode->stInfo.ptrHead)
                        {
                            ptrFreeNode = ptrStartNode->stInfo.ptrHead;
                            ptrStartNode->stInfo.ptrHead = ptrFreeNode->ptrNext;
                            cmd_free(ptrFreeNode);
                        }
                        cmd_free(ptrStartNode);
                        ptrStartNode = ptrNextNode;
                        dwCount--;
                    }
                    FindClose(hSearch);
                    return 1;
                }

                /* Copy the info of search at linked list */
                memcpy(&ptrNextNode->ptrNext->stInfo.stFindInfo,
                       &wfdFileInfo,
                       sizeof(WIN32_FIND_DATA));

                /* If lower case is selected do it here */
                if (lpFlags->bLowerCase)
                {
                    _tcslwr(ptrNextNode->ptrNext->stInfo.stFindInfo.cAlternateFileName);
                    _tcslwr(ptrNextNode->ptrNext->stInfo.stFindInfo.cFileName);
                }

                /* No streams (yet?) */
                ptrNextNode->ptrNext->stInfo.ptrHead = NULL;

                /* Alternate streams are only displayed with new long list */
                if (lpFlags->bNewLongList && lpFlags->bDataStreams)
                {
                    /* Try to get stream information */
                    hStreams = FindFirstStreamW(wfdFileInfo.cFileName, FindStreamInfoStandard, &wfsdStreamInfo, 0);
                    if (hStreams != INVALID_HANDLE_VALUE)
                    {
                        /* We totally ignore first stream. It contains data about ::$DATA */
                        ptrCurNode = &ptrNextNode->ptrNext->stInfo.ptrHead;
                        while (FindNextStreamW(hStreams, &wfsdStreamInfo))
                        {
                            *ptrCurNode = cmd_alloc(sizeof(DIRFINDSTREAMNODE));
                            if (*ptrCurNode == NULL)
                            {
                                WARN("DEBUG: Cannot allocate memory for *ptrCurNode!\n");
                                while (ptrStartNode)
                                {
                                    ptrNextNode = ptrStartNode->ptrNext;
                                    while (ptrStartNode->stInfo.ptrHead)
                                    {
                                        ptrFreeNode = ptrStartNode->stInfo.ptrHead;
                                        ptrStartNode->stInfo.ptrHead = ptrFreeNode->ptrNext;
                                        cmd_free(ptrFreeNode);
                                    }
                                    cmd_free(ptrStartNode);
                                    ptrStartNode = ptrNextNode;
                                    dwCount--;
                                }
                                FindClose(hStreams);
                                FindClose(hSearch);
                                return 1;
                            }

                            memcpy(&(*ptrCurNode)->stStreamInfo, &wfsdStreamInfo,
                                   sizeof(WIN32_FIND_STREAM_DATA));

                            /* If lower case is selected do it here */
                            if (lpFlags->bLowerCase)
                            {
                                _tcslwr((*ptrCurNode)->stStreamInfo.cStreamName);
                            }

                            ptrCurNode = &(*ptrCurNode)->ptrNext;
                        }

                         FindClose(hStreams);
                         *ptrCurNode = NULL;
                    }
                }

                /* Continue at next node at linked list */
                ptrNextNode = ptrNextNode->ptrNext;
                dwCount ++;

                /* Grab statistics */
                if (wfdFileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    /* Directory */
                    dwCountDirs++;
                }
                else
                {
                    /* File */
                    dwCountFiles++;
                    u64Temp.HighPart = wfdFileInfo.nFileSizeHigh;
                    u64Temp.LowPart = wfdFileInfo.nFileSizeLow;
                    u64CountBytes += u64Temp.QuadPart;
                }
            }
        } while (FindNextFile(hSearch, &wfdFileInfo));
        FindClose(hSearch);
    }

    /* Terminate list */
    ptrNextNode->ptrNext = NULL;

    /* Calculate and allocate space need for making an array of pointers */
    ptrFileArray = cmd_alloc(sizeof(PDIRFINDINFO) * dwCount);
    if (ptrFileArray == NULL)
    {
        WARN("DEBUG: Cannot allocate memory for ptrFileArray!\n");
        while (ptrStartNode)
        {
            ptrNextNode = ptrStartNode->ptrNext;
            while (ptrStartNode->stInfo.ptrHead)
            {
                ptrFreeNode = ptrStartNode->stInfo.ptrHead;
                ptrStartNode->stInfo.ptrHead = ptrFreeNode->ptrNext;
                cmd_free(ptrFreeNode);
            }
            cmd_free(ptrStartNode);
            ptrStartNode = ptrNextNode;
            dwCount --;
        }
        return 1;
    }

    /*
     * Create an array of pointers from the linked list
     * this will be used to sort and print data, rather than the list
     */
    ptrNextNode = ptrStartNode;
    dwCount = 0;
    while (ptrNextNode->ptrNext)
    {
        *(ptrFileArray + dwCount) = &ptrNextNode->ptrNext->stInfo;
        ptrNextNode = ptrNextNode->ptrNext;
        dwCount++;
    }

    /* Sort Data if requested*/
    if (lpFlags->stOrderBy.sCriteriaCount > 0)
        QsortFiles(ptrFileArray, 0, dwCount-1, lpFlags);

    /* Print Data */
    pszFilePart[-1] = _T('\0'); /* truncate to directory name only */
    DirPrintFiles(ptrFileArray, dwCount, szFullPath, lpFlags);
    pszFilePart[-1] = _T('\\');

    if (lpFlags->bRecursive)
    {
        PrintSummary(szFullPath,
                     dwCountFiles,
                     dwCountDirs,
                     u64CountBytes,
                     lpFlags,
                     FALSE);
    }

    /* Free array */
    cmd_free(ptrFileArray);
    /* Free linked list */
    while (ptrStartNode)
    {
        ptrNextNode = ptrStartNode->ptrNext;
        while (ptrStartNode->stInfo.ptrHead)
        {
            ptrFreeNode = ptrStartNode->stInfo.ptrHead;
            ptrStartNode->stInfo.ptrHead = ptrFreeNode->ptrNext;
            cmd_free(ptrFreeNode);
        }
        cmd_free(ptrStartNode);
        ptrStartNode = ptrNextNode;
        dwCount --;
    }

    if (CheckCtrlBreak(BREAK_INPUT))
        return 1;


    /* Add statistics to recursive statistics*/
    recurse_dir_cnt += dwCountDirs;
    recurse_file_cnt += dwCountFiles;
    recurse_bytes += u64CountBytes;

    /* Do the recursive job if requested
       the recursive is be done on ALL(indepent of their attribs)
       directoried of the current one.*/
    if (lpFlags->bRecursive)
    {
        /* The new search is involving any *.* file */
        memcpy(szSubPath, szFullPath, (pszFilePart - szFullPath) * sizeof(TCHAR));
        _tcscpy(&szSubPath[pszFilePart - szFullPath], _T("*.*"));

        hRecSearch = FindFirstFile (szSubPath, &wfdFileInfo);
        if (hRecSearch != INVALID_HANDLE_VALUE)
        {
            do
            {
                /* We search for directories other than "." and ".." */
                if ((_tcsicmp(wfdFileInfo.cFileName, _T(".")) != 0) &&
                    (_tcsicmp(wfdFileInfo.cFileName, _T("..")) != 0 ) &&
                    (wfdFileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    /* Concat the path and the directory to do recursive */
                    memcpy(szSubPath, szFullPath, (pszFilePart - szFullPath) * sizeof(TCHAR));
                    _tcscpy(&szSubPath[pszFilePart - szFullPath], wfdFileInfo.cFileName);
                    _tcscat(szSubPath, _T("\\"));
                    _tcscat(szSubPath, pszFilePart);

                    /* We do the same for the folder */
                    if (DirList(szSubPath, lpFlags) != 0)
                    {
                        FindClose(hRecSearch);
                        return 1;
                    }
                }
            } while(FindNextFile(hRecSearch, &wfdFileInfo));
        }
        FindClose(hRecSearch);
    }

    return 0;
}

/*
 * dir
 *
 * internal dir command
 */
INT
CommandDir(LPTSTR rest)
{
    TCHAR   dircmd[256];    /* A variable to store the DIRCMD enviroment variable */
    TCHAR   path[MAX_PATH];
    TCHAR   prev_volume[MAX_PATH];
    LPTSTR* params = NULL;
    LPTSTR  pszFilePart;
    INT     entries = 0;
    UINT    loop = 0;
    DIRSWITCHFLAGS stFlags;
    INT ret = 1;
    BOOL ChangedVolume;

    /* Initialize Switch Flags < Default switches are setted here!> */
    stFlags.b4Digit = TRUE;
    stFlags.bBareFormat = FALSE;
    stFlags.bDataStreams = FALSE;
    stFlags.bLowerCase = FALSE;
    stFlags.bNewLongList = TRUE;
    stFlags.bPause = FALSE;
    stFlags.bRecursive = FALSE;
    stFlags.bShortName = FALSE;
    stFlags.bTSeperator = TRUE;
    stFlags.bUser = FALSE;
    stFlags.bWideList = FALSE;
    stFlags.bWideListColSort = FALSE;
    stFlags.stTimeField.eTimeField = TF_MODIFIEDDATE;
    stFlags.stAttribs.dwAttribMask = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
    stFlags.stAttribs.dwAttribVal = 0L;
    stFlags.stOrderBy.sCriteriaCount = 0;

    nErrorLevel = 0;

    /* read the parameters from the DIRCMD environment variable */
    if (GetEnvironmentVariable (_T("DIRCMD"), dircmd, 256))
    {
        if (!DirReadParam(dircmd, &params, &entries, &stFlags))
        {
            nErrorLevel = 1;
            goto cleanup;
        }
    }

    /* read the parameters */
    if (!DirReadParam(rest, &params, &entries, &stFlags) || CheckCtrlBreak(BREAK_INPUT))
    {
        nErrorLevel = 1;
        goto cleanup;
    }

    /* default to current directory */
    if (entries == 0)
    {
        if (!add_entry(&entries, &params, _T("*")))
        {
            nErrorLevel = 1;
            goto cleanup;
        }
    }

    prev_volume[0] = _T('\0');

    /* Reset paging state */
    if (stFlags.bPause)
        ConOutPrintfPaging(TRUE, _T(""));

    for(loop = 0; loop < (UINT)entries; loop++)
    {
        if (CheckCtrlBreak(BREAK_INPUT))
        {
            nErrorLevel = 1;
            goto cleanup;
        }

        recurse_dir_cnt = 0L;
        recurse_file_cnt = 0L;
        recurse_bytes = 0;

        /* <Debug :>
           Uncomment this to show the final state of switch flags*/
        {
            int i;
            TRACE("Attributes mask/value %x/%x\n",stFlags.stAttribs.dwAttribMask,stFlags.stAttribs.dwAttribVal  );
            TRACE("(B) Bare format : %i\n", stFlags.bBareFormat );
            TRACE("(C) Thousand : %i\n", stFlags.bTSeperator );
            TRACE("(W) Wide list : %i\n", stFlags.bWideList );
            TRACE("(D) Wide list sort by column : %i\n", stFlags.bWideListColSort );
            TRACE("(L) Lowercase : %i\n", stFlags.bLowerCase );
            TRACE("(N) New : %i\n", stFlags.bNewLongList );
            TRACE("(O) Order : %i\n", stFlags.stOrderBy.sCriteriaCount );
            for (i =0;i<stFlags.stOrderBy.sCriteriaCount;i++)
                TRACE(" Order Criteria [%i]: %i (Reversed: %i)\n",i, stFlags.stOrderBy.eCriteria[i], stFlags.stOrderBy.bCriteriaRev[i] );
            TRACE("(P) Pause : %i\n", stFlags.bPause  );
            TRACE("(Q) Owner : %i\n", stFlags.bUser );
            TRACE("(R) Data stream : %i\n", stFlags.bDataStreams );
            TRACE("(S) Recursive : %i\n", stFlags.bRecursive );
            TRACE("(T) Time field : %i\n", stFlags.stTimeField.eTimeField );
            TRACE("(X) Short names : %i\n", stFlags.bShortName );
            TRACE("Parameter : %s\n", debugstr_aw(params[loop]) );
        }

        /* Print the drive header if the volume changed */
        ChangedVolume = TRUE;

        if (!stFlags.bBareFormat &&
            GetVolumePathName(params[loop], path, sizeof(path) / sizeof(TCHAR)))
        {
            if (!_tcscmp(path, prev_volume))
                ChangedVolume = FALSE;
            else
                _tcscpy(prev_volume, path);
        }
        else if (GetFullPathName(params[loop], sizeof(path) / sizeof(TCHAR), path, &pszFilePart) != 0)
        {
            if (pszFilePart != NULL)
                *pszFilePart = _T('\0');
        }
        else
        {
            _tcscpy(path, params[loop]);
        }

        if (ChangedVolume && !stFlags.bBareFormat)
        {
            if (!PrintDirectoryHeader (params[loop], &stFlags))
            {
                nErrorLevel = 1;
                goto cleanup;
            }
        }

        /* do the actual dir */
        if (DirList (params[loop], &stFlags))
        {
            nErrorLevel = 1;
            goto cleanup;
        }

        /* print the footer */
        PrintSummary(path,
                     recurse_file_cnt,
                     recurse_dir_cnt,
                     recurse_bytes,
                     &stFlags,
                     TRUE);
    }

    ret = 0;

cleanup:
    freep(params);

    return ret;
}

#endif

/* EOF */
