/*
 * SCREEN.C - screen internal command.
 *
 * clone from 4nt msgbox command
 *
 * 30 Aug 1999
 *     started - Paolo Pantaleo <paolopan@freemail.it>
 *
 *    30-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 *
 */

#include <precomp.h>

#ifdef INCLUDE_CMD_SCREEN


INT CommandScreen (LPTSTR param)
{
	SHORT x,y;
	BOOL bSkipText = FALSE;

	if (_tcsncmp (param, _T("/?"), 2) == 0)
	{
		ConOutResPaging(TRUE,STRING_SCREEN_HELP);
		return 0;
	}

	nErrorLevel = 0;

	//get row
	while(_istspace(*param))
		param++;

	if(!(*param))
	{
		error_req_param_missing ();
		return 1;
	}

	y = _ttoi(param);
	if (y<0 || y>(maxy-1))
	{
		ConOutResPuts(STRING_SCREEN_ROW);

		return 1;
	}

	//get col
	if(!(param = _tcschr(param,_T(' '))))
	{
		error_req_param_missing ();
		return 1;
	}

	while(_istspace(*param))
		param++;

	if(!(*param))
	{
		error_req_param_missing ();
		return 1;
	}

	x = _ttoi(param);
	if (x<0 || x>(maxx-1))
	{
		ConErrResPuts(STRING_SCREEN_COL);
		return 1;
	}

	//get text
	if(!(param = _tcschr(param,_T(' '))))
	{
		bSkipText = TRUE;
	}
	else
	{
		while(_istspace(*param))
			param++;

		if(!(*param))
		{
			bSkipText = TRUE;
		}
	}

	bIgnoreEcho = TRUE;

	if(bSkipText)
		x=0;


	SetCursorXY(x,y);

	if(!(bSkipText))
		ConOutPuts(param);

	return 0;
}

#endif /* INCLUDE_CMD_SCREEN */
