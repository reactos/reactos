#include <windows.h>
#include <stdio.h>
#include <tchar.h>

static VOID
partstrlwr (LPTSTR str)
{
    LPTSTR c = str;
    while (*c && !_istspace (*c) && *c != _T('='))
    {
        *c = _totlower (*c);
        c++;
    }
}

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
    LPTSTR ptr;

    while (*param == _T(' '))
        param++;

    /* error if no '=' found */
    if ((ptr = _tcschr (param, _T('='))) == 0)
        return 1;

    while (*param == _T(' '))
        param++;

    while (*ptr == _T(' '))
        ptr--;

    /* Split rest into name and substitute */
    *ptr++ = _T('\0');

    partstrlwr (param);

    _tprintf(_T("%s, %s\n"), ptr, param);

    if (ptr[0] == _T('\0'))
        AddConsoleAlias(param, NULL, _T("cmd.exe"));
    else
        AddConsoleAlias(param, ptr, _T("cmd.exe"));

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
        SetMacro(line);

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
        }
        else
        {
            do
            {
                szCommandLine++;
            }
            while(*szCommandLine != ' ');
        }

        /* Skip the trailing quotation mark/whitespace and pass the command line to SetMacro */
        SetMacro(++szCommandLine);
    }

    return 0;
}
