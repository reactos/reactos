/*
 *  IF.C - if internal batch command.
 *
 *
 *  History:
 *
 *    16 Jul 1998 (Hans B Pufal)
 *        started.
 *
 *    16 Jul 1998 (John P Price)
 *        Seperated commands into individual files.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    07-Jan-1999 (Eric Kohl)
 *        Added help text ("if /?") and cleaned up.
 *
 *    21-Jan-1999 (Eric Kohl)
 *        Unicode and redirection ready!
 *
 *    01-Sep-1999 (Eric Kohl)
 *        Fixed help text.
 *
 *    17-Feb-2001 (ea)
 *        IF DEFINED variable command
 *
 *    28-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 *
 */

#include <precomp.h>

static INT GenericCmp(INT (*StringCmp)(LPCTSTR, LPCTSTR),
                      LPCTSTR Left, LPCTSTR Right)
{
	TCHAR *end;
	INT nLeft = _tcstol(Left, &end, 0);
	if (*end == _T('\0'))
	{
		INT nRight = _tcstol(Right, &end, 0);
		if (*end == _T('\0'))
		{
			/* both arguments are numeric */
			return (nLeft < nRight) ? -1 : (nLeft > nRight);
		}
	}
	return StringCmp(Left, Right);
}

BOOL ExecuteIf(PARSED_COMMAND *Cmd)
{
	INT result = FALSE; /* when set cause 'then' clause to be executed */
	LPTSTR param;

#if 0
	/* FIXME: need to handle IF /?; will require special parsing */
	TRACE ("cmd_if: (\'%s\')\n", debugstr_aw(param));

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_IF_HELP1);
		return 0;
	}
#endif

	if (Cmd->If.Operator == IF_CMDEXTVERSION)
	{
		/* IF CMDEXTVERSION n: check if Command Extensions version
		 * is greater or equal to n */
		DWORD n = _tcstoul(Cmd->If.RightArg, &param, 10);
		if (*param != _T('\0'))
		{
			error_syntax(Cmd->If.RightArg);
			return FALSE;
		}
		result = (2 >= n);
	}
	else if (Cmd->If.Operator == IF_DEFINED)
	{
		/* IF DEFINED var: check if environment variable exists */
		result = (GetEnvVarOrSpecial(Cmd->If.RightArg) != NULL);
	}
	else if (Cmd->If.Operator == IF_ERRORLEVEL)
	{
		/* IF ERRORLEVEL n: check if last exit code is greater or equal to n */
		INT n = _tcstol(Cmd->If.RightArg, &param, 10);
		if (*param != _T('\0'))
		{
			error_syntax(Cmd->If.RightArg);
			return FALSE;
		}
		result = (nErrorLevel >= n);
	}
	else if (Cmd->If.Operator == IF_EXIST)
	{
		/* IF EXIST filename: check if file exists (wildcards allowed) */
		WIN32_FIND_DATA f;
		HANDLE hFind;

		StripQuotes(Cmd->If.RightArg);

		hFind = FindFirstFile(Cmd->If.RightArg, &f);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			result = TRUE;
			FindClose(hFind);
		}
	}
	else
	{
		/* Do case-insensitive string comparisons if /I specified */
		INT (*StringCmp)(LPCTSTR, LPCTSTR) =
			(Cmd->If.Flags & IFFLAG_IGNORECASE) ? _tcsicmp : _tcscmp;

		if (Cmd->If.Operator == IF_STRINGEQ)
		{
			/* IF str1 == str2 */
			result = StringCmp(Cmd->If.LeftArg, Cmd->If.RightArg) == 0;
		}
		else
		{
			result = GenericCmp(StringCmp, Cmd->If.LeftArg, Cmd->If.RightArg);
			switch (Cmd->If.Operator)
			{
			case IF_EQU: result = (result == 0); break;
			case IF_NEQ: result = (result != 0); break;
			case IF_LSS: result = (result < 0); break;
			case IF_LEQ: result = (result <= 0); break;
			case IF_GTR: result = (result > 0); break;
			case IF_GEQ: result = (result >= 0); break;
			}
		}
	}

	if (result ^ ((Cmd->If.Flags & IFFLAG_NEGATE) != 0))
	{
		/* full condition was true, do the command */
		return ExecuteCommand(Cmd->Subcommands);
	}
	else
	{
		/* full condition was false, do the "else" command if there is one */
		if (Cmd->Subcommands->Next)
			return ExecuteCommand(Cmd->Subcommands->Next);
		return TRUE;
	}
}

/* EOF */
