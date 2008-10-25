/*
 *  ALIAS.C - alias administration module.
 *
 *
 *  History:
 *
 *    02/02/1996 (Oliver Mueller)
 *        started.
 *
 *    02/03/1996 (Oliver Mueller)
 *        Added sorting algorithm and case sensitive substitution by using
 *        partstrupr().
 *
 *    27 Jul 1998  John P. Price
 *        added config.h include
 *        added ifdef's to disable aliases
 *
 *    09-Dec-1998 Eric Kohl
 *        Fixed crash when removing an alias in DeleteAlias().
 *        Added help text ("/?").
 *
 *    14-Jan-1998 Eric Kohl
 *        Clean up and Unicode safe!
 *
 *    24-Jan-1998 Eric Kohl
 *        Redirection safe!
 *
 *    02-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 *
 *    02-Feb-2008 (Christoph von Wittich) <christoph_vw@reactos.org>)
 *        rewrote alias handling for doskey compat
  */


#include <precomp.h>

#ifdef FEATURE_ALIASES

/* module internal functions */
/* strlwr only for first word in string */
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
	ptr = cmd_alloc(len + sizeof(TCHAR));
	if (!ptr)
		return;

	Aliases = ptr;

	ZeroMemory(Aliases, len + sizeof(TCHAR));

	if (GetConsoleAliases(Aliases, len, _T("cmd.exe")) != 0)
	{
		while (*Aliases != '\0')
		{
			ConOutPrintf(_T("%s\n"), Aliases);
			Aliases = Aliases + lstrlen(Aliases);
			Aliases++;
		}
	}
	cmd_free(ptr);
}

/* specified routines */
VOID ExpandAlias (LPTSTR cmd, INT maxlen)
{
	LPTSTR buffer;
	TCHAR* position;
	LPTSTR Token;
	LPTSTR tmp;

	tmp = cmd_alloc(maxlen);
	if (!tmp)
		return;
	_tcscpy(tmp, cmd);

	Token = _tcstok(tmp, _T(" ")); /* first part is the macro name */
	if (!Token)
	{
		cmd_free(tmp);
		return;
	}

	buffer = cmd_alloc(maxlen);
	if (!buffer)
	{
		cmd_free(tmp);
		return;
	}
	
	if (GetConsoleAlias(Token, buffer, maxlen, _T("cmd.exe")) == 0)
	{
		cmd_free(tmp);
		cmd_free(buffer);
		return;
	}

	Token = _tcstok (NULL, _T(" "));

	ZeroMemory(cmd, maxlen);
	position = _tcsstr(buffer, _T("$*"));
	if (position)
	{
		_tcsncpy(cmd, buffer, (INT) (position - buffer) - 1);
		if (Token)
		{
			_tcscat(cmd, _T(" "));
			_tcscat(cmd, Token);
		}
	}
	else
	{
		_tcscpy(cmd, buffer);
	}

	cmd_free(buffer);
	cmd_free(tmp);

}

INT CommandAlias (LPTSTR param)
{
	LPTSTR ptr;

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_ALIAS_HELP);
		return 0;
	}

   nErrorLevel = 0;

	if (param[0] == _T('\0'))
	{
		PrintAlias ();
		return 0;
	}

	nErrorLevel = 0;

	/* error if no '=' found */
	if ((ptr = _tcschr (param, _T('='))) == 0)
	{
		nErrorLevel = 1;
		return 1;
	}

	/* Split rest into name and substitute */
	*ptr++ = _T('\0');

	partstrlwr (param);

	if (ptr[0] == _T('\0'))
		AddConsoleAlias(param, NULL, _T("cmd.exe"));
	else
		AddConsoleAlias(param, ptr, _T("cmd.exe"));

	return 0;
}
#endif
