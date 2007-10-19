/*
 *  FOR.C - for internal batch command.
 *
 *
 *  History:
 *
 *    16-Jul-1998 (Hans B Pufal)
 *        Started.
 *
 *    16-Jul-1998 (John P Price)
 *        Seperated commands into individual files.
 *
 *    19-Jul-1998 (Hans B Pufal)
 *        Implementation of FOR.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        Added config.h include.
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Unicode and redirection safe!
 *
 *    01-Sep-1999 (Eric Kohl)
 *        Added help text.
 *
 *    23-Feb-2001 (Carl Nettelblad <cnettel@hem.passagen.se>)
 *        Implemented preservation of echo flag. Some other for related
 *        code in other files fixed, too.
 *
 *    28-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>


/*
 * Perform FOR command.
 *
 * First check syntax is correct : FOR %v IN ( <list> ) DO <command>
 *   v must be alphabetic, <command> must not be empty.
 *
 * If all is correct build a new bcontext structure which preserves
 *   the necessary information so that readbatchline can expand
 *   each the command prototype for each list element.
 *
 * You might look on a FOR as being a called batch file with one line
 *   per list element.
 */

INT cmd_for (LPTSTR cmd, LPTSTR param)
{
	LPBATCH_CONTEXT lpNew;
	LPTSTR pp;
	TCHAR  var;
	TCHAR szMsg[RC_STRING_MAX_SIZE];

#ifdef _DEBUG
	DebugPrintf (_T("cmd_for (\'%s\', \'%s\'\n"), cmd, param);
#endif

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_FOR_HELP1);
		return 0;
	}

	/* Check that first element is % then an alpha char followed by space */
	if ((*param != _T('%')) || !_istalpha (*(param + 1)) || !_istspace (*(param + 2)))
	{
		LoadString( CMD_ModuleHandle, STRING_FOR_ERROR, szMsg, RC_STRING_MAX_SIZE);
		error_syntax (szMsg);
		return 1;
	}

	param++;
	var = *param++;               /* Save FOR var name */

	while (_istspace (*param))
		param++;

	/* Check next element is 'IN' */
	if ((_tcsnicmp (param, _T("in"), 2) != 0) || !_istspace (*(param + 2)))
	{
		LoadString(CMD_ModuleHandle, STRING_FOR_ERROR1, szMsg, RC_STRING_MAX_SIZE);
		error_syntax(szMsg);
		return 1;
	}

	param += 2;
	while (_istspace (*param))
		param++;

	/* Folowed by a '(', find also matching ')' */
	if ((*param != _T('(')) || (NULL == (pp = _tcsrchr (param, _T(')')))))
	{
		LoadString(CMD_ModuleHandle, STRING_FOR_ERROR2, szMsg, RC_STRING_MAX_SIZE);
		error_syntax(szMsg);
		return 1;
	}

	*pp++ = _T('\0');
	param++;		/* param now points at null terminated list */

	while (_istspace (*pp))
		pp++;

	/* Check DO follows */
	if ((_tcsnicmp (pp, _T("do"), 2) != 0) || !_istspace (*(pp + 2)))
	{
		LoadString(CMD_ModuleHandle, STRING_FOR_ERROR3, szMsg, RC_STRING_MAX_SIZE);
		error_syntax(szMsg);
		return 1;
	}

	pp += 2;
	while (_istspace (*pp))
		pp++;

	/* Check that command tail is not empty */
	if (*pp == _T('\0'))
	{
		LoadString(CMD_ModuleHandle, STRING_FOR_ERROR4, szMsg, RC_STRING_MAX_SIZE);
		error_syntax(szMsg);
		return 1;
	}

	/* OK all is correct, build a bcontext.... */
	lpNew = (LPBATCH_CONTEXT)cmd_alloc (sizeof (BATCH_CONTEXT));

	lpNew->prev = bc;
	bc = lpNew;

	bc->hBatchFile = INVALID_HANDLE_VALUE;
	bc->ffind = NULL;
	bc->params = BatchParams (_T(""), param); /* Split out list */
	bc->shiftlevel = 0;
	bc->forvar = var;
	bc->forproto = _tcsdup (pp);
	if (bc->prev)
		bc->bEcho = bc->prev->bEcho;
	else
		bc->bEcho = bEcho;
		bc->In[0] = _T('\0');
		bc->Out[0] = _T('\0');
		bc->Err[0] = _T('\0');


	return 0;
}

/* EOF */
