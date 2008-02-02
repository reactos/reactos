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
			printf(_T("%s\n"), Aliases);
			Aliases = Aliases + lstrlen(Aliases);
			Aliases++;
		}
	}
	HeapFree(GetProcessHeap(), 0 , ptr);
}

int
main (int argc, char **argv)
{
	
	if (argc < 2)
		return 0;

	if (argv[1][0] == '/')
	{
		if (stricmp(argv[1], "/macros") == 0)
			PrintAlias();
	}
	else
	{
		/* FIXME */
	}

	

	return 0;
}

