/*
 *  MISC.C - misc. functions.
 *
 *
 *  History:
 *
 *    07/12/98 (Rob Lake)
 *        started
 *
 *    07/13/98 (Rob Lake)
 *        moved functions in here
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    18-Dec-1998 (Eric Kohl)
 *        Changed split() to accept quoted arguments.
 *        Removed parse_firstarg().
 *
 *    23-Jan-1999 (Eric Kohl)
 *        Fixed an ugly bug in split(). In rare cases (last character
 *        of the string is a space) it ignored the NULL character and
 *        tried to add the following to the argument list.
 *
 *    28-Jan-1999 (Eric Kohl)
 *        FileGetString() seems to be working now.
 *
 *    06-Nov-1999 (Eric Kohl)
 *        Added PagePrompt() and FilePrompt().
 *
 *    30-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hardcoded strings in En.rc
 */

#include "precomp.h"

/*
 * get a character out-of-band and honor Ctrl-Break characters
 */
TCHAR
cgetchar (VOID)
{
    HANDLE hInput = GetStdHandle (STD_INPUT_HANDLE);
    INPUT_RECORD irBuffer;
    DWORD  dwRead;

    do
    {
        ReadConsoleInput (hInput, &irBuffer, 1, &dwRead);
        if ((irBuffer.EventType == KEY_EVENT) &&
            (irBuffer.Event.KeyEvent.bKeyDown != FALSE))
        {
            if (irBuffer.Event.KeyEvent.dwControlKeyState &
                 (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
            {
                if (irBuffer.Event.KeyEvent.wVirtualKeyCode == 'C')
                {
                    bCtrlBreak = TRUE;
                    break;
                }
            }
            else if ((irBuffer.Event.KeyEvent.wVirtualKeyCode == VK_SHIFT) ||
                     (irBuffer.Event.KeyEvent.wVirtualKeyCode == VK_MENU) ||
                     (irBuffer.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL))
            {
                // Nothing to do
            }
            else
            {
                break;
            }
        }
    }
    while (TRUE);

#ifndef _UNICODE
    return irBuffer.Event.KeyEvent.uChar.AsciiChar;
#else
    return irBuffer.Event.KeyEvent.uChar.UnicodeChar;
#endif /* _UNICODE */
}

/*
 * Takes a path in and returns it with the correct case of the letters
 */
VOID GetPathCase(IN LPCTSTR Path, OUT LPTSTR OutPath)
{
    SIZE_T i;
    SIZE_T cchPath = _tcslen(Path);
    TCHAR TempPath[MAX_PATH];
    LPTSTR pchTemp = TempPath;
    LPTSTR pchTempEnd = TempPath + _countof(TempPath);
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;

    *pchTemp = OutPath[0] = 0;

    for (i = 0; i < cchPath; ++i)
    {
        if (pchTemp + 1 >= pchTempEnd)
        {
            // On failure, copy the original path for an error message
            StringCchCopy(OutPath, MAX_PATH, Path);
            return;
        }

        if (Path[i] != _T('\\') && Path[i] != _T('/'))
        {
            *pchTemp++ = Path[i];
            *pchTemp = 0;
            if (i != cchPath - 1)
                continue;
        }

        /* Handle the base part of the path different.
           Because if you put it into findfirstfile, it will
           return your current folder */
        if (TempPath[0] && TempPath[1] == _T(':') && !TempPath[2]) /* "C:", "D:" etc. */
        {
            StringCchCat(OutPath, MAX_PATH, TempPath);
            StringCchCat(OutPath, MAX_PATH, _T("\\"));
            StringCchCat(TempPath, _countof(TempPath), _T("\\"));
        }
        else
        {
            hFind = FindFirstFile(TempPath, &FindFileData);
            if (hFind == INVALID_HANDLE_VALUE)
            {
                StringCchCopy(OutPath, MAX_PATH, Path);
                return;
            }
            FindClose(hFind);
            StringCchCat(OutPath, MAX_PATH, _T("\\"));
            StringCchCat(OutPath, MAX_PATH, FindFileData.cFileName);
            StringCchCat(OutPath, MAX_PATH, _T("\\"));
            StringCchCopy(TempPath, _countof(TempPath), OutPath);
        }
        pchTemp = TempPath + _tcslen(TempPath);
    }
}

/*
 * Check if Ctrl-Break was pressed during the last calls
 */

BOOL CheckCtrlBreak(INT mode)
{
    static BOOL bLeaveAll = FALSE; /* leave all batch files */
    TCHAR options[4]; /* Yes, No, All */
    TCHAR c;

    switch (mode)
    {
        case BREAK_OUTOFBATCH:
            bLeaveAll = FALSE;
            return FALSE;

        case BREAK_BATCHFILE:
        {
            if (bLeaveAll)
                return TRUE;

            if (!bCtrlBreak)
                return FALSE;

            LoadString(CMD_ModuleHandle, STRING_COPY_OPTION, options, ARRAYSIZE(options));

            ConOutResPuts(STRING_CANCEL_BATCH_FILE);
            do
            {
                c = _totupper(cgetchar());
            } while (!(_tcschr(options, c) || c == _T('\3')) || !c);

            ConOutChar(_T('\n'));

            if (c == options[1])
            {
                bCtrlBreak = FALSE; /* ignore */
                return FALSE;
            }

            /* leave all batch files */
            bLeaveAll = ((c == options[2]) || (c == _T('\3')));
            break;
        }

        case BREAK_INPUT:
            if (!bCtrlBreak)
                return FALSE;
            break;
    }

    /* state processed */
    return TRUE;
}

/* add new entry for new argument */
BOOL add_entry (LPINT ac, LPTSTR **arg, LPCTSTR entry)
{
    LPTSTR q;
    LPTSTR *oldarg;

    q = cmd_alloc ((_tcslen(entry) + 1) * sizeof (TCHAR));
    if (!q)
    {
        WARN("Cannot allocate memory for q!\n");
        return FALSE;
    }

    _tcscpy (q, entry);
    oldarg = *arg;
    *arg = cmd_realloc (oldarg, (*ac + 2) * sizeof (LPTSTR));
    if (!*arg)
    {
        WARN("Cannot reallocate memory for arg!\n");
        *arg = oldarg;
        cmd_free (q);
        return FALSE;
    }

    /* save new entry */
    (*arg)[*ac] = q;
    (*arg)[++(*ac)] = NULL;

    return TRUE;
}

static BOOL expand (LPINT ac, LPTSTR **arg, LPCTSTR pattern)
{
    HANDLE hFind;
    WIN32_FIND_DATA FindData;
    BOOL ok;
    LPCTSTR pathend;
    LPTSTR dirpart, fullname;

    pathend = _tcsrchr (pattern, _T('\\'));
    if (NULL != pathend)
    {
        dirpart = cmd_alloc((pathend - pattern + 2) * sizeof(TCHAR));
        if (!dirpart)
        {
            WARN("Cannot allocate memory for dirpart!\n");
            return FALSE;
        }
        memcpy(dirpart, pattern, pathend - pattern + 1);
        dirpart[pathend - pattern + 1] = _T('\0');
    }
    else
    {
        dirpart = NULL;
    }
    hFind = FindFirstFile (pattern, &FindData);
    if (INVALID_HANDLE_VALUE != hFind)
    {
        do
        {
            if (NULL != dirpart)
            {
                fullname = cmd_alloc((_tcslen(dirpart) + _tcslen(FindData.cFileName) + 1) * sizeof(TCHAR));
                if (!fullname)
                {
                    WARN("Cannot allocate memory for fullname!\n");
                    ok = FALSE;
                }
                else
                {
                    _tcscat (_tcscpy (fullname, dirpart), FindData.cFileName);
                    ok = add_entry(ac, arg, fullname);
                    cmd_free (fullname);
                }
            }
            else
            {
                ok = add_entry(ac, arg, FindData.cFileName);
            }
        } while (FindNextFile (hFind, &FindData) && ok);
        FindClose (hFind);
    }
    else
    {
        ok = add_entry(ac, arg, pattern);
    }

    if (NULL != dirpart)
    {
        cmd_free (dirpart);
    }

    return ok;
}

/*
 * split - splits a line up into separate arguments, delimiters
 *         are spaces and slashes ('/').
 */
LPTSTR *split (LPTSTR s, LPINT args, BOOL expand_wildcards, BOOL handle_plus)
{
    LPTSTR *arg;
    LPTSTR start;
    LPTSTR q;
    INT  ac;
    INT_PTR  len;

    arg = cmd_alloc (sizeof (LPTSTR));
    if (!arg)
    {
        WARN("Cannot allocate memory for arg!\n");
        return NULL;
    }
    *arg = NULL;

    ac = 0;
    while (*s)
    {
        BOOL bQuoted = FALSE;

        /* skip leading spaces */
        while (*s && (_istspace(*s) || _istcntrl(*s)))
            ++s;

        start = s;

        /* the first character can be '/' */
        if (*s == _T('/'))
            ++s;

        /* skip to next word delimiter or start of next option */
        while (_istprint(*s))
        {
            /* if quote (") then set bQuoted */
            bQuoted ^= (*s == _T('\"'));

            /* Check if we have unquoted text */
            if (!bQuoted)
            {
                /* check for separators */
                if (_istspace(*s) ||
                    (*s == _T('/')) ||
                    (handle_plus && (*s == _T('+'))))
                {
                    /* Make length at least one character */
                    if (s == start) s++;
                    break;
                }
            }

            ++s;
        }

        /* a word was found */
        if (s != start)
        {
            q = cmd_alloc (((len = s - start) + 1) * sizeof (TCHAR));
            if (!q)
            {
                WARN("Cannot allocate memory for q!\n");
                return NULL;
            }
            memcpy (q, start, len * sizeof (TCHAR));
            q[len] = _T('\0');
            StripQuotes(q);
            if (expand_wildcards && (_T('/') != *start) &&
                (NULL != _tcschr(q, _T('*')) || NULL != _tcschr(q, _T('?'))))
            {
                if (! expand(&ac, &arg, q))
                {
                    cmd_free (q);
                    freep (arg);
                    return NULL;
                }
            }
            else
            {
                if (! add_entry(&ac, &arg, q))
                {
                    cmd_free (q);
                    freep (arg);
                    return NULL;
                }
            }
            cmd_free (q);
        }
    }

    *args = ac;

    return arg;
}

/*
 * splitspace() is a function which uses JUST spaces as delimiters. split() uses "/" AND spaces.
 * The way it works is real similar to split(), search the difference ;)
 * splitspace is needed for commands such as "move" where paths as C:\this/is\allowed/ are allowed
 */
LPTSTR *splitspace (LPTSTR s, LPINT args)
{
    LPTSTR *arg;
    LPTSTR start;
    LPTSTR q;
    INT  ac;
    INT_PTR  len;

    arg = cmd_alloc (sizeof (LPTSTR));
    if (!arg)
    {
        WARN("Cannot allocate memory for arg!\n");
        return NULL;
    }
    *arg = NULL;

    ac = 0;
    while (*s)
    {
        BOOL bQuoted = FALSE;

        /* skip leading spaces */
        while (*s && (_istspace (*s) || _istcntrl (*s)))
            ++s;

        start = s;

        /* skip to next word delimiter or start of next option */
        while (_istprint(*s) && (bQuoted || !_istspace(*s)))
        {
            /* if quote (") then set bQuoted */
            bQuoted ^= (*s == _T('\"'));
            ++s;
        }

        /* a word was found */
        if (s != start)
        {
            q = cmd_alloc (((len = s - start) + 1) * sizeof (TCHAR));
            if (!q)
            {
                WARN("Cannot allocate memory for q!\n");
                return NULL;
            }
            memcpy (q, start, len * sizeof (TCHAR));
            q[len] = _T('\0');
            StripQuotes(q);
            if (! add_entry(&ac, &arg, q))
            {
                cmd_free (q);
                freep (arg);
                return NULL;
            }
            cmd_free (q);
        }
    }

    *args = ac;

    return arg;
}

/*
 * freep -- frees memory used for a call to split
 */
VOID freep (LPTSTR *p)
{
    LPTSTR *q;

    if (!p)
        return;

    q = p;
    while (*q)
        cmd_free(*q++);

    cmd_free(p);
}

LPTSTR _stpcpy (LPTSTR dest, LPCTSTR src)
{
    _tcscpy (dest, src);
    return (dest + _tcslen (src));
}

VOID
StripQuotes(TCHAR *in)
{
    TCHAR *out = in;
    for (; *in; in++)
    {
        if (*in != _T('"'))
            *out++ = *in;
    }
    *out = _T('\0');
}


/*
 * Checks if a path is valid (is accessible)
 */
BOOL IsValidPathName(IN LPCTSTR pszPath)
{
    BOOL  bResult;
    TCHAR szOldPath[MAX_PATH];

    GetCurrentDirectory(ARRAYSIZE(szOldPath), szOldPath);
    bResult = SetCurrentDirectory(pszPath);

    SetCurrentDirectory(szOldPath);

    return bResult;
}

/*
 * Checks if a file exists (is accessible)
 */
BOOL IsExistingFile(IN LPCTSTR pszPath)
{
    DWORD attr = GetFileAttributes(pszPath);
    return ((attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

BOOL IsExistingDirectory(IN LPCTSTR pszPath)
{
    DWORD attr = GetFileAttributes(pszPath);
    return ((attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY));
}


// See r874
BOOL __stdcall PagePrompt(PCON_PAGER Pager, DWORD Done, DWORD Total)
{
    SHORT iScreenWidth, iCursorY;
    INPUT_RECORD ir;

    ConOutResPuts(STRING_MISC_HELP1);

    RemoveBreakHandler();
    ConInDisable();

    do
    {
        ConInKey(&ir);
    }
    while ((ir.Event.KeyEvent.wVirtualKeyCode == VK_SHIFT) ||
           (ir.Event.KeyEvent.wVirtualKeyCode == VK_MENU) ||
           (ir.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL));

    AddBreakHandler();
    ConInEnable();

    /*
     * Get the screen width, erase the full line where the cursor is,
     * and move the cursor back to the beginning of the line.
     */
    GetScreenSize(&iScreenWidth, NULL);
    iCursorY = GetCursorY();
    SetCursorXY(0, iCursorY);
    while (iScreenWidth-- > 0) // Or call FillConsoleOutputCharacter ?
        ConOutChar(_T(' '));
    SetCursorXY(0, iCursorY);

    if ((ir.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) ||
        ((ir.Event.KeyEvent.wVirtualKeyCode == _T('C')) &&
         (ir.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))))
    {
        /* We break, output a newline */
        ConOutChar(_T('\n'));

        bCtrlBreak = TRUE;
        return FALSE;
    }

    return TRUE;
}


INT FilePromptYN (UINT resID)
{
    TCHAR szMsg[RC_STRING_MAX_SIZE];
//  TCHAR cKey = 0;
//  LPTSTR szKeys = _T("yna");

    TCHAR szIn[10];
    LPTSTR p;

    if (resID != 0)
        ConOutResPrintf (resID);

    /* preliminary fix */
    ConInString(szIn, 10);

    _tcsupr (szIn);
    for (p = szIn; _istspace (*p); p++)
        ;

    LoadString(CMD_ModuleHandle, STRING_CHOICE_OPTION, szMsg, ARRAYSIZE(szMsg));

    if (_tcsncmp(p, &szMsg[0], 1) == 0)
        return PROMPT_YES;
    else if (_tcsncmp(p, &szMsg[1], 1) == 0)
        return PROMPT_NO;
#if 0
    else if (*p == _T('\03'))
        return PROMPT_BREAK;
#endif

    return PROMPT_NO;

    /* unfinished solution */
#if 0
    RemoveBreakHandler();
    ConInDisable();

    do
    {
        ConInKey (&ir);
        cKey = _totlower (ir.Event.KeyEvent.uChar.AsciiChar);
        if (_tcschr (szKeys, cKey[0]) == NULL)
            cKey = 0;


    }
    while ((ir.Event.KeyEvent.wVirtualKeyCode == VK_SHIFT) ||
           (ir.Event.KeyEvent.wVirtualKeyCode == VK_MENU) ||
           (ir.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL));

    AddBreakHandler();
    ConInEnable();

    if ((ir.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) ||
        ((ir.Event.KeyEvent.wVirtualKeyCode == 'C') &&
         (ir.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))))
        return PROMPT_BREAK;

    return PROMPT_YES;
#endif
}


INT FilePromptYNA (UINT resID)
{
    TCHAR szMsg[RC_STRING_MAX_SIZE];
//  TCHAR cKey = 0;
//  LPTSTR szKeys = _T("yna");

    TCHAR szIn[10];
    LPTSTR p;

    if (resID != 0)
        ConOutResPrintf (resID);

    /* preliminary fix */
    ConInString(szIn, 10);

    _tcsupr (szIn);
    for (p = szIn; _istspace (*p); p++)
        ;

    LoadString(CMD_ModuleHandle, STRING_COPY_OPTION, szMsg, ARRAYSIZE(szMsg));

    if (_tcsncmp(p, &szMsg[0], 1) == 0)
        return PROMPT_YES;
    else if (_tcsncmp(p, &szMsg[1], 1) == 0)
        return PROMPT_NO;
    else if (_tcsncmp(p, &szMsg[2], 1) == 0)
        return PROMPT_ALL;
#if 0
    else if (*p == _T('\03'))
        return PROMPT_BREAK;
#endif

    return PROMPT_NO;

    /* unfinished solution */
#if 0
    RemoveBreakHandler();
    ConInDisable();

    do
    {
        ConInKey (&ir);
        cKey = _totlower (ir.Event.KeyEvent.uChar.AsciiChar);
        if (_tcschr (szKeys, cKey[0]) == NULL)
            cKey = 0;
    }
    while ((ir.Event.KeyEvent.wVirtualKeyCode == VK_SHIFT) ||
           (ir.Event.KeyEvent.wVirtualKeyCode == VK_MENU) ||
           (ir.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL));

    AddBreakHandler();
    ConInEnable();

    if ((ir.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) ||
        ((ir.Event.KeyEvent.wVirtualKeyCode == _T('C')) &&
         (ir.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))))
        return PROMPT_BREAK;

    return PROMPT_YES;
#endif
}

/* EOF */
