#include <windows.h>
#include <stdio.h>
#include <tchar.h>

static VOID
PrintAlias (VOID)
{
    LPTSTR Aliases;
    LPTSTR ptr;
    DWORD len;

    len = GetConsoleAliasesLength(_T("cmd.exe"));
    if (len <= 0)
        return;

    /* allocate memory for an extra \0 char to make parsing easier */
    ptr = HeapAlloc(GetProcessHeap(), 0, (len + sizeof(TCHAR)));
    if (!ptr)
        return;

    Aliases = ptr;

    ZeroMemory(Aliases, len + sizeof(TCHAR));

    if (GetConsoleAliases(Aliases, len, _T("cmd.exe")) != 0)
    {
        while (*Aliases != '\0')
        {
            _tprintf(_T("%s\n"), Aliases);
            Aliases = Aliases + lstrlen(Aliases);
            Aliases++;
        }
    }
    HeapFree(GetProcessHeap(), 0 , ptr);
}

INT SetMacro (LPTSTR param)
{
    LPTSTR ptr, text;

    while (*param == _T(' '))
        param++;

    /* error if no '=' found */
    if ((ptr = _tcschr (param, _T('='))) == 0)
        return 1;

    text = ptr + 1;
    while (*text == _T(' '))
        text++;

    while (ptr > param && ptr[-1] == _T(' '))
        ptr--;

    /* Split rest into name and substitute */
    *ptr++ = _T('\0');

    if (*param == _T('\0') || _tcschr(param, _T(' ')))
        return 1;

    _tprintf(_T("%s, %s\n"), text, param);

    if (ptr[0] == _T('\0'))
        AddConsoleAlias(param, NULL, _T("cmd.exe"));
    else
        AddConsoleAlias(param, text, _T("cmd.exe"));

    return 0;
}

static VOID ReadFromFile(LPTSTR param)
{
    FILE* fp;
    TCHAR line[MAX_PATH];

    /* Skip the "/macrofile=" prefix */
    param += 11;

    fp = _tfopen(param, _T("r"));

    while ( _fgetts(line, MAX_PATH, fp) != NULL) 
    {
        /* Remove newline character */
        TCHAR *end = &line[_tcslen(line) - 1];
        if (*end == _T('\n'))
            *end = _T('\0');

        SetMacro(line);
    }

    fclose(fp);
    return;
}

int
_tmain (int argc, LPTSTR argv[])
{
    if (argc < 2)
        return 0;

    if (argv[1][0] == '/')
    {
        if (_tcsnicmp(argv[1], _T("/macrofile"), 10) == 0)
            ReadFromFile(argv[1]);
        if (_tcscmp(argv[1], _T("/macros")) == 0)
            PrintAlias();
    }
    else
    {
        /* Get the full command line using GetCommandLine().
           We can't just pass argv[1] here, because then a parameter like "gotoroot=cd \" wouldn't be passed completely. */
        TCHAR* szCommandLine = GetCommandLine();

        /* Skip the application name */
        if(*szCommandLine == '\"')
        {
            do
            {
                szCommandLine++;
            }
            while(*szCommandLine != '\"');
            szCommandLine++;
        }
        else
        {
            do
            {
                szCommandLine++;
            }
            while(*szCommandLine != ' ');
        }

        /* Skip the leading whitespace and pass the command line to SetMacro */
        SetMacro(szCommandLine + _tcsspn(szCommandLine, _T(" \t")));
    }

    return 0;
}
