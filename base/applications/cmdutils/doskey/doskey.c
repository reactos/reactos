#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "doskey.h"

#define MAX_STRING 2000
TCHAR szStringBuf[MAX_STRING];
LPTSTR pszExeName = _T("cmd.exe");

static VOID SetInsert(DWORD dwFlag)
{
    DWORD dwMode;
    HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hConsole, &dwMode);
    dwMode |= ENABLE_EXTENDED_FLAGS;
    SetConsoleMode(hConsole, (dwMode & ~ENABLE_INSERT_MODE) | dwFlag);
}

static VOID PrintHistory(VOID)
{
    DWORD Length = GetConsoleCommandHistoryLength(pszExeName);
    /* On Windows, the ANSI version of GetConsoleCommandHistory requires
     * a buffer twice as large as the actual history length. */
    BYTE HistBuf[Length * (sizeof(WCHAR) / sizeof(TCHAR))];
    TCHAR *Hist    = (TCHAR *)HistBuf;
    TCHAR *HistEnd = (TCHAR *)&HistBuf[Length];

    if (GetConsoleCommandHistory(Hist, sizeof HistBuf, pszExeName))
        for (; Hist < HistEnd; Hist += _tcslen(Hist) + 1)
            _tprintf(_T("%s\n"), Hist);
}

static INT SetMacro(LPTSTR definition)
{
    TCHAR *name, *nameend, *text, temp;

    name = definition;
    while (*name == _T(' '))
        name++;

    /* error if no '=' found */
    if ((nameend = _tcschr(name, _T('='))) != NULL)
    {
        text = nameend + 1;
        while (*text == _T(' '))
            text++;

        while (nameend > name && nameend[-1] == _T(' '))
            nameend--;

        /* Split rest into name and substitute */
        temp = *nameend;
        *nameend = _T('\0');
        /* Don't allow spaces in the name, since such a macro would be unusable */
        if (!_tcschr(name, _T(' ')) && AddConsoleAlias(name, text, pszExeName))
            return 0;
        *nameend = temp;
    }

    LoadString(GetModuleHandle(NULL), IDS_INVALID_MACRO_DEF, szStringBuf, MAX_STRING);
    _tprintf(szStringBuf, definition);
    return 1;
}

static VOID PrintMacros(LPTSTR pszExeName, LPTSTR Indent)
{
    DWORD Length = GetConsoleAliasesLength(pszExeName);
    BYTE AliasBuf[Length];
    TCHAR *Alias    = (TCHAR *)AliasBuf;
    TCHAR *AliasEnd = (TCHAR *)&AliasBuf[Length];

    if (GetConsoleAliases(Alias, sizeof AliasBuf, pszExeName))
        for (; Alias < AliasEnd; Alias += _tcslen(Alias) + 1)
            _tprintf(_T("%s%s\n"), Indent, Alias);
}

static VOID PrintAllMacros(VOID)
{
    DWORD Length = GetConsoleAliasExesLength();
    BYTE ExeNameBuf[Length];
    TCHAR *ExeName    = (TCHAR *)ExeNameBuf;
    TCHAR *ExeNameEnd = (TCHAR *)&ExeNameBuf[Length];

    if (GetConsoleAliasExes(ExeName, sizeof ExeNameBuf))
    {
        for (; ExeName < ExeNameEnd; ExeName += _tcslen(ExeName) + 1)
        {
            _tprintf(_T("[%s]\n"), ExeName);
            PrintMacros(ExeName, _T("    "));
            _tprintf(_T("\n"));
        }
    }
}

static VOID ReadFromFile(LPTSTR param)
{
    FILE* fp;
    TCHAR line[MAX_PATH];

    fp = _tfopen(param, _T("r"));
    if (!fp)
    {
#ifdef UNICODE
        _wperror(param);
#else
        perror(param);
#endif
        return;
    }

    while ( _fgetts(line, MAX_PATH, fp) != NULL) 
    {
        /* Remove newline character */
        TCHAR *end = &line[_tcslen(line) - 1];
        if (*end == _T('\n'))
            *end = _T('\0');

        if (*line)
            SetMacro(line);
    }

    fclose(fp);
    return;
}

/* Get the start and end of the next command-line argument. */
static BOOL GetArg(TCHAR **pStart, TCHAR **pEnd)
{
    BOOL bInQuotes = FALSE;
    TCHAR *p = *pEnd;
    p += _tcsspn(p, _T(" \t"));
    if (!*p)
        return FALSE;
    *pStart = p;
    do
    {
        if (!bInQuotes && (*p == _T(' ') || *p == _T('\t')))
            break;
        bInQuotes ^= (*p++ == _T('"'));
    } while (*p);
    *pEnd = p;
    return TRUE;
}

/* Remove starting and ending quotes from a string, if present */
static LPTSTR RemoveQuotes(LPTSTR str)
{
    TCHAR *end;
    if (*str == _T('"') && *(end = str + _tcslen(str) - 1) == _T('"'))
    {
        str++;
        *end = _T('\0');
    }
    return str;
}

int
_tmain(VOID)
{
    /* Get the full command line using GetCommandLine(). We can't just use argv,
     * because then a parameter like "gotoroot=cd \" wouldn't be passed completely. */
    TCHAR *pArgStart;
    TCHAR *pArgEnd = GetCommandLine();

    /* Skip the application name */
    GetArg(&pArgStart, &pArgEnd);

    while (GetArg(&pArgStart, &pArgEnd))
    {
        /* NUL-terminate this argument to make processing easier */
        TCHAR tmp = *pArgEnd;
        *pArgEnd = _T('\0');

        if (!_tcscmp(pArgStart, _T("/?")))
        {
            LoadString(GetModuleHandle(NULL), IDS_HELP, szStringBuf, MAX_STRING);
            _tprintf(szStringBuf);
            break;
        }
        else if (!_tcsnicmp(pArgStart, _T("/EXENAME="), 9))
        {
            pszExeName = RemoveQuotes(pArgStart + 9);
        }
        else if (!_tcsicmp(pArgStart, _T("/H")) ||
                 !_tcsicmp(pArgStart, _T("/HISTORY")))
        {
            PrintHistory();
        }
        else if (!_tcsnicmp(pArgStart, _T("/LISTSIZE="), 10))
        {
            SetConsoleNumberOfCommands(_ttoi(pArgStart + 10), pszExeName);
        }
        else if (!_tcsicmp(pArgStart, _T("/REINSTALL")))
        {
            ExpungeConsoleCommandHistory(pszExeName);
        }
        else if (!_tcsicmp(pArgStart, _T("/INSERT")))
        {
            SetInsert(ENABLE_INSERT_MODE);
        }
        else if (!_tcsicmp(pArgStart, _T("/OVERSTRIKE")))
        {
            SetInsert(0);
        }
        else if (!_tcsicmp(pArgStart, _T("/M")) ||
                 !_tcsicmp(pArgStart, _T("/MACROS")))
        {
            PrintMacros(pszExeName, _T(""));
        }
        else if (!_tcsnicmp(pArgStart, _T("/M:"),      3) ||
                 !_tcsnicmp(pArgStart, _T("/MACROS:"), 8))
        {
            LPTSTR exe = RemoveQuotes(_tcschr(pArgStart, _T(':')) + 1);
            if (!_tcsicmp(exe, _T("ALL")))
                PrintAllMacros();
            else
                PrintMacros(exe, _T(""));
        }
        else if (!_tcsnicmp(pArgStart, _T("/MACROFILE="), 11))
        {
            ReadFromFile(RemoveQuotes(pArgStart + 11));
        }
        else
        {
            /* This is the beginning of a macro definition. It includes
             * the entire remainder of the line, so first put back the
             * character that we replaced with NUL. */
            *pArgEnd = tmp;
            return SetMacro(pArgStart);
        }

        if (!tmp) break;
        pArgEnd++;
    }

    return 0;
}
