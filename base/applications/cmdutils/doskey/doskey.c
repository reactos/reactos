/*
 * PROJECT:     ReactOS DosKey Command
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Provides history and command aliases management for
 *              command-line programs.
 * COPYRIGHT:   Copyright 2008 Christoph von Wittich
 *              Copyright 2013-2017 Hermès Bélusca-Maïto
 */

#include <stdio.h>
#include <wchar.h>
#include <locale.h>

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wincon.h>

/* Console API functions which are absent from wincon.h */
#define EXENAME_LENGTH (255 + 1)

VOID
WINAPI
ExpungeConsoleCommandHistoryW(LPCWSTR lpExeName);

DWORD
WINAPI
GetConsoleCommandHistoryW(LPWSTR lpHistory,
                          DWORD cbHistory,
                          LPCWSTR lpExeName);

DWORD
WINAPI
GetConsoleCommandHistoryLengthW(LPCWSTR lpExeName);

BOOL
WINAPI
SetConsoleNumberOfCommandsW(DWORD dwNumCommands,
                            LPCWSTR lpExeName);

#include "doskey.h"

#define MAX_STRING 2000
WCHAR szStringBuf[MAX_STRING];
LPWSTR pszExeName = L"cmd.exe";

static VOID SetInsert(DWORD dwFlag)
{
    /*
     * NOTE: Enabling the ENABLE_INSERT_MODE mode can also be done by calling
     * kernel32:SetConsoleCommandHistoryMode(CONSOLE_OVERSTRIKE) (deprecated).
     */
    DWORD dwMode;
    HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hConsole, &dwMode);
    dwMode |= ENABLE_EXTENDED_FLAGS;
    SetConsoleMode(hConsole, (dwMode & ~ENABLE_INSERT_MODE) | dwFlag);
}

static VOID PrintHistory(VOID)
{
    DWORD Length = GetConsoleCommandHistoryLengthW(pszExeName);
    PBYTE HistBuf;
    WCHAR *Hist;
    WCHAR *HistEnd;

    HistBuf = HeapAlloc(GetProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        Length);
    if (!HistBuf) return;
    Hist = (WCHAR *)HistBuf;
    HistEnd = (WCHAR *)&HistBuf[Length];

    if (GetConsoleCommandHistoryW(Hist, Length, pszExeName))
    {
        for (; Hist < HistEnd; Hist += wcslen(Hist) + 1)
        {
            wprintf(L"%s\n", Hist);
        }
    }

    HeapFree(GetProcessHeap(), 0, HistBuf);
}

static INT SetMacro(LPWSTR definition)
{
    WCHAR *name, *nameend, *text, temp;

    name = definition;
    while (*name == L' ')
        name++;

    /* error if no '=' found */
    if ((nameend = wcschr(name, L'=')) != NULL)
    {
        text = nameend + 1;
        while (*text == L' ')
            text++;

        while (nameend > name && nameend[-1] == L' ')
            nameend--;

        /* Split rest into name and substitute */
        temp = *nameend;
        *nameend = L'\0';
        /* Don't allow spaces in the name, since such a macro would be unusable */
        if (!wcschr(name, L' ') && AddConsoleAliasW(name, text, pszExeName))
            return 0;
        *nameend = temp;
    }

    LoadStringW(GetModuleHandle(NULL),
                IDS_INVALID_MACRO_DEF,
                szStringBuf,
                ARRAYSIZE(szStringBuf));
    wprintf(szStringBuf, definition);
    return 1;
}

static VOID PrintMacros(LPWSTR pszExeName, LPWSTR Indent)
{
    DWORD Length = GetConsoleAliasesLengthW(pszExeName);
    PBYTE AliasBuf;
    WCHAR *Alias;
    WCHAR *AliasEnd;

    AliasBuf = HeapAlloc(GetProcessHeap(),
                         HEAP_ZERO_MEMORY,
                         Length * sizeof(BYTE));
    if (!AliasBuf) return;
    Alias = (WCHAR *)AliasBuf;
    AliasEnd = (WCHAR *)&AliasBuf[Length];

    if (GetConsoleAliasesW(Alias, Length * sizeof(BYTE), pszExeName))
    {
        for (; Alias < AliasEnd; Alias += wcslen(Alias) + 1)
        {
            wprintf(L"%s%s\n", Indent, Alias);
        }
    }

    HeapFree(GetProcessHeap(), 0, AliasBuf);
}

static VOID PrintAllMacros(VOID)
{
    DWORD Length = GetConsoleAliasExesLength();
    PBYTE ExeNameBuf;
    WCHAR *ExeName;
    WCHAR *ExeNameEnd;

    ExeNameBuf = HeapAlloc(GetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           Length * sizeof(BYTE));
    if (!ExeNameBuf) return;
    ExeName = (WCHAR *)ExeNameBuf;
    ExeNameEnd = (WCHAR *)&ExeNameBuf[Length];

    if (GetConsoleAliasExesW(ExeName, Length * sizeof(BYTE)))
    {
        for (; ExeName < ExeNameEnd; ExeName += wcslen(ExeName) + 1)
        {
            wprintf(L"[%s]\n", ExeName);
            PrintMacros(ExeName, L"    ");
            wprintf(L"\n");
        }
    }

    HeapFree(GetProcessHeap(), 0, ExeNameBuf);
}

/* Remove starting and ending quotes from a string, if present */
static LPWSTR RemoveQuotes(LPWSTR str)
{
    WCHAR *end;
    if (*str == L'"' && *(end = str + wcslen(str) - 1) == L'"')
    {
        str++;
        *end = L'\0';
    }
    return str;
}

static VOID ReadFromFile(LPWSTR FileName)
{
    FILE* fp;
    WCHAR line[MAX_PATH];
    WCHAR ExeNameBuffer[EXENAME_LENGTH];
    LPWSTR pszOrgExeName = pszExeName;

    /* Open the file */
    fp = _wfopen(FileName, L"rt");
    if (!fp)
    {
        _wperror(FileName);
        return;
    }

    while (fgetws(line, ARRAYSIZE(line), fp) != NULL)
    {
        PWCHAR end;

        if (!*line)
            continue;

        /* Remove trailing newline character */
        end = &line[wcslen(line) - 1];
        if (*end == L'\n')
            *end = L'\0';

        if (!*line)
            continue;

        /* Check for any section redefining the current executable name */
        end = NULL;
        if (*line == L'[')
            end = wcschr(line, L']');

        if (end != NULL)
        {
            /* New section: change the current executable name */

            *end = L'\0'; // NULL-terminate it
            pszExeName = RemoveQuotes(line + 1);
            if (*pszExeName)
            {
                /* Capture the new executable name and truncate it if needed */
                end = &pszExeName[wcslen(pszExeName)];
                if (end - pszExeName >= EXENAME_LENGTH)
                    end = &pszExeName[EXENAME_LENGTH - 1];
                *end = L'\0'; // Truncate it
                wcscpy(ExeNameBuffer, pszExeName);
                pszExeName = ExeNameBuffer;
            }
            else
            {
                /* Restore the original current executable name */
                pszExeName = pszOrgExeName;
            }
        }
        else
        {
            /* Set the new macro for the current executable */
            SetMacro(line);
        }
    }

    /* Restore the original current executable name if it has changed */
    pszExeName = pszOrgExeName;

    /* Close the file and return */
    fclose(fp);
    return;
}

/* Get the start and end of the next command-line argument. */
static BOOL GetArg(WCHAR **pStart, WCHAR **pEnd)
{
    BOOL bInQuotes = FALSE;
    WCHAR *p = *pEnd;
    p += wcsspn(p, L" \t");
    if (!*p)
        return FALSE;
    *pStart = p;
    do
    {
        if (!bInQuotes && (*p == L' ' || *p == L'\t'))
            break;
        bInQuotes ^= (*p++ == L'"');
    } while (*p);
    *pEnd = p;
    return TRUE;
}

int
wmain(VOID)
{
    LPWSTR pArgStart, pArgEnd;

    setlocale(LC_ALL, "");

    /* Get the full command line using GetCommandLine(). We can't just use argv,
     * because then a parameter like "gotoroot=cd \" wouldn't be passed completely. */
    pArgEnd = GetCommandLineW();

    /* Skip the application name */
    GetArg(&pArgStart, &pArgEnd);

    while (GetArg(&pArgStart, &pArgEnd))
    {
        /* NULL-terminate this argument to make processing easier */
        WCHAR tmp = *pArgEnd;
        *pArgEnd = L'\0';

        if (!wcscmp(pArgStart, L"/?"))
        {
            LoadStringW(GetModuleHandle(NULL),
                        IDS_HELP,
                        szStringBuf,
                        ARRAYSIZE(szStringBuf));
            wprintf(szStringBuf);
            break;
        }
        else if (!_wcsnicmp(pArgStart, L"/EXENAME=", 9))
        {
            pszExeName = RemoveQuotes(pArgStart + 9);
        }
        else if (!_wcsicmp(pArgStart, L"/H") ||
                 !_wcsicmp(pArgStart, L"/HISTORY"))
        {
            PrintHistory();
        }
        else if (!_wcsnicmp(pArgStart, L"/LISTSIZE=", 10))
        {
            SetConsoleNumberOfCommandsW(_wtoi(pArgStart + 10), pszExeName);
        }
        else if (!_wcsicmp(pArgStart, L"/REINSTALL"))
        {
            ExpungeConsoleCommandHistoryW(pszExeName);
        }
        else if (!_wcsicmp(pArgStart, L"/INSERT"))
        {
            SetInsert(ENABLE_INSERT_MODE);
        }
        else if (!_wcsicmp(pArgStart, L"/OVERSTRIKE"))
        {
            SetInsert(0);
        }
        else if (!_wcsicmp(pArgStart, L"/M") ||
                 !_wcsicmp(pArgStart, L"/MACROS"))
        {
            PrintMacros(pszExeName, L"");
        }
        else if (!_wcsnicmp(pArgStart, L"/M:",      3) ||
                 !_wcsnicmp(pArgStart, L"/MACROS:", 8))
        {
            LPWSTR exe = RemoveQuotes(wcschr(pArgStart, L':') + 1);
            if (!_wcsicmp(exe, L"ALL"))
                PrintAllMacros();
            else
                PrintMacros(exe, L"");
        }
        else if (!_wcsnicmp(pArgStart, L"/MACROFILE=", 11))
        {
            ReadFromFile(RemoveQuotes(pArgStart + 11));
        }
        else
        {
            /* This is the beginning of a macro definition. It includes
             * the entire remainder of the line, so first put back the
             * character that we replaced with NULL. */
            *pArgEnd = tmp;
            return SetMacro(pArgStart);
        }

        if (!tmp) break;
        pArgEnd++;
    }

    return 0;
}
