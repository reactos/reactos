/*
 * SCREEN.C - screen internal command.
 *
 * clone from 4nt msgbox command
 *
 * 30 Aug 1999
 *     started - Paolo Pantaleo <paolopan@freemail.it>
 *
 *
 */

#include "precomp.h"

#ifdef INCLUDE_CMD_SCREEN


INT CommandScreen (LPTSTR cmd, LPTSTR param)
{
	SHORT x,y;
	BOOL bSkipText = FALSE;

	if (_tcsncmp (param, _T("/?"), 2) == 0)
	{
		ConOutPuts(_T(
		              "move cursor and optionally print text\n"
		              "\n"
		              "SCREEN row col [text]\n"
		              "\n"
		              "  row         row to wich move the cursor\n"
		              "  col         column to wich move the cursor"));
		return 0;
	}

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
		ConOutPrintf(_T("invalid value for	row"));
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
		ConErrPuts(_T("invalid value for col"));
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
