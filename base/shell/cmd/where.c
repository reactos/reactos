/*
 *  WHERE.C - file search functions.
 *
 *
 *  History:
 *
 *    07/15/95 (Tim Norman)
 *        started.
 *
 *    08/08/95 (Matt Rains)
 *        i have cleaned up the source code. changes now bring this source
 *        into guidelines for recommended programming practice.
 *
 *    12/12/95 (Steffan Kaiser & Tim Norman)
 *        added some patches to fix some things and make more efficient
 *
 *    1/6/96 (Tim Norman)
 *        fixed a stupid pointer mistake...
 *        Thanks to everyone who noticed it!
 *
 *    8/1/96 (Tim Norman)
 *        fixed a bug when getenv returns NULL
 *
 *    8/7/96 (Steffan Kaiser and Tim Norman)
 *        speed improvements and bug fixes
 *
 *    8/27/96 (Tim Norman)
 *        changed code to use pointers directly into PATH environment
 *        variable rather than making our own copy.  This saves some memory,
 *        but requires we write our own function to copy pathnames out of
 *        the variable.
 *
 *    12/23/96 (Aaron Kaufman)
 *        Fixed a bug in get_paths() that did not point to the first PATH
 *        in the environment variable.
 *
 *    7/12/97 (Tim Norman)
 *        Apparently, Aaron's bugfix got lost, so I fixed it again.
 *
 *    16 July 1998 (John P. Price)
 *        Added stand alone code.
 *
 *    17 July 1998 (John P. Price)
 *        Rewrote find_which to use searchpath function
 *
 *    24-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        fixed bug where didn't check all extensions when path was specified
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    30-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        fixed so that it find_which returns NULL if filename is not
 *        executable (does not have .bat, .com, or .exe extention).
 *        Before command would to execute any file with any extension (opps!)
 *
 *    03-Dec-1998 (Eric Kohl)
 *        Changed find_which().
 *
 *    07-Dec-1998 (Eric Kohl)
 *        Added ".CMD" extension.
 *        Replaced numeric constant by _NR_OF_EXTENSIONS.
 *
 *    26-Feb-1999 (Eric Kohl)
 *        Replaced find_which() by SearchForExecutable().
 *        Now files are searched using the right extension order.
 *
 *    20-Apr-1999 (Eric Kohl)
 *        Some minor changes and improvements.
 *
 *    10-Jul-2004 (Jens Collin <jens.collin@lakhei.com>)
 *        Fixed searching for files with specific extensions in PATHEXT order.
 *
 */

#include "precomp.h"


/* initial size of environment variable buffer */
#define ENV_BUFFER_SIZE  1024


/* searches for file using path info. */

BOOL
SearchForExecutableSingle (LPCTSTR pFileName, LPTSTR pFullName, LPTSTR pPathExt, LPTSTR pDirectory)
{
    TCHAR  szPathBuffer[CMDLINE_LENGTH], *pszPathEnd;
    LPTSTR s,f;
    /* initialize full name buffer */
    *pFullName = _T('\0');

    TRACE ("SearchForExecutableSingle: \'%s\' in dir: \'%s\'\n",
        debugstr_aw(pFileName), debugstr_aw(pDirectory));

    pszPathEnd = szPathBuffer;
    if (pDirectory != NULL)
    {
        _tcscpy(szPathBuffer, pDirectory);
        pszPathEnd += _tcslen(pszPathEnd);
        *pszPathEnd++ = _T('\\');
    }
    _tcscpy(pszPathEnd, pFileName);
    pszPathEnd += _tcslen(pszPathEnd);

    if (IsExistingFile (szPathBuffer))
    {
        TRACE ("Found: \'%s\'\n", debugstr_aw(szPathBuffer));
        _tcscpy (pFullName, szPathBuffer);
        return TRUE;
    }

    s = pPathExt;
    while (s && *s)
    {
        f = _tcschr (s, _T(';'));

        if (f)
        {
            _tcsncpy (pszPathEnd, s, (size_t)(f-s));
            pszPathEnd[f-s] = _T('\0');
            s = f + 1;
        }
        else
        {
            _tcscpy (pszPathEnd, s);
            s = NULL;
        }

        if (IsExistingFile (szPathBuffer))
        {
            TRACE ("Found: \'%s\'\n", debugstr_aw(szPathBuffer));
            _tcscpy (pFullName, szPathBuffer);
            return TRUE;
        }
    }
    return FALSE;
}


BOOL
SearchForExecutable (LPCTSTR pFileName, LPTSTR pFullName)
{
    static TCHAR pszDefaultPathExt[] = _T(".com;.exe;.bat;.cmd");
    LPTSTR pszPathExt, pszPath;
    LPTSTR pCh;
    DWORD  dwBuffer;
    TRACE ("SearchForExecutable: \'%s\'\n", debugstr_aw(pFileName));

    /* load environment varable PATHEXT */
    pszPathExt = (LPTSTR)cmd_alloc (ENV_BUFFER_SIZE * sizeof(TCHAR));
    dwBuffer = GetEnvironmentVariable (_T("PATHEXT"), pszPathExt, ENV_BUFFER_SIZE);
    if (dwBuffer > ENV_BUFFER_SIZE)
    {
        LPTSTR pszOldPathExt = pszPathExt;
        pszPathExt = (LPTSTR)cmd_realloc (pszPathExt, dwBuffer * sizeof (TCHAR));
        if (pszPathExt == NULL)
        {
            cmd_free(pszOldPathExt);
            return FALSE;
        }
        GetEnvironmentVariable (_T("PATHEXT"), pszPathExt, dwBuffer);
        _tcslwr(pszPathExt);
    }
    else if (0 == dwBuffer)
    {
        _tcscpy(pszPathExt, pszDefaultPathExt);
    }
    else
    {
        _tcslwr(pszPathExt);
    }

    /* Check if valid directly on specified path */
    if (SearchForExecutableSingle(pFileName, pFullName, pszPathExt, NULL))
    {
        cmd_free(pszPathExt);
        return TRUE;
    }

    /* If an explicit directory was given, stop here - no need to search PATH. */
    if (pFileName[1] == _T(':') || _tcschr(pFileName, _T('\\')))
    {
        cmd_free(pszPathExt);
        return FALSE;
    }

    /* load environment varable PATH into buffer */
    pszPath = (LPTSTR)cmd_alloc (ENV_BUFFER_SIZE * sizeof(TCHAR));
    dwBuffer = GetEnvironmentVariable (_T("PATH"), pszPath, ENV_BUFFER_SIZE);
    if (dwBuffer > ENV_BUFFER_SIZE)
    {
        LPTSTR pszOldPath = pszPath;
        pszPath = (LPTSTR)cmd_realloc (pszPath, dwBuffer * sizeof (TCHAR));
        if (pszPath == NULL)
        {
            cmd_free(pszOldPath);
            cmd_free(pszPathExt);
            return FALSE;
        }
        GetEnvironmentVariable (_T("PATH"), pszPath, dwBuffer);
    }

    TRACE ("SearchForExecutable(): Loaded PATH: %s\n", debugstr_aw(pszPath));

    /* search in PATH */
    pCh = _tcstok(pszPath, _T(";"));
    while (pCh)
    {
        if (SearchForExecutableSingle(pFileName, pFullName, pszPathExt, pCh))
        {
            cmd_free(pszPath);
            cmd_free(pszPathExt);
            return TRUE;
        }
        pCh = _tcstok(NULL, _T(";"));
    }

    cmd_free(pszPath);
    cmd_free(pszPathExt);
    return FALSE;
}

/* EOF */
