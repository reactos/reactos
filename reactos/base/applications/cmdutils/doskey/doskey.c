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
			printf(_T("%s\n"), Aliases);
			Aliases = Aliases + lstrlen(Aliases);
			Aliases++;
		}
	}
	HeapFree(GetProcessHeap(), 0 , ptr);
}

INT SetMacro (LPTSTR param)
{
	LPTSTR ptr;

	while (*param == ' ')
		param++;

	/* error if no '=' found */
	if ((ptr = _tcschr (param, _T('='))) == 0)
	{
		return 1;
	}

	while (*param == ' ')
		param++;

	/* Split rest into name and substitute */
	*ptr++ = _T('\0');

	partstrlwr (param);

	if (ptr[0] == _T('\0'))
		AddConsoleAlias(param, NULL, _T("cmd.exe"));
	else
		AddConsoleAlias(param, ptr, _T("cmd.exe"));

	return 0;
}

static VOID ReadFromFile(LPTSTR param)
{
	FILE* fp;
	char line[MAX_PATH];

	/* FIXME */
	param += 11;

	fp = _tfopen(param,"r");
	while ( fgets(line, MAX_PATH, fp) != NULL) 
		SetMacro(line);

	fclose(fp);
	return;
}

int
main (int argc, char **argv)
{
	
	if (argc < 2)
		return 0;

	if (argv[1][0] == '/')
	{
		if (strnicmp(argv[1], "/macrofile", 10) == 0)
			ReadFromFile(argv[1]);
		if (stricmp(argv[1], "/macros") == 0)
			PrintAlias();
	}
	else
	{
		SetMacro(argv[1]);
	}

	

	return 0;
}

