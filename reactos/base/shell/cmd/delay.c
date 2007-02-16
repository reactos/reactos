/*
 * DELAY.C - internal command.
 *
 * clone from 4nt delay command
 *
 * 30 Aug 1999
 *     started - Paolo Pantaleo <paolopan@freemail.it>
 *
 *
 */

#include <precomp.h>
#include "resource.h"

#ifdef INCLUDE_CMD_DELAY


INT CommandDelay (LPTSTR cmd, LPTSTR param)
{
	DWORD val;
	DWORD mul=1000;

	if (_tcsncmp (param, _T("/?"), 2) == 0)
	{
		ConOutResPaging(TRUE,STRING_DELAY_HELP);
		return 0;
	}

	nErrorLevel = 0;

	if (*param==0)
	{
		error_req_param_missing ();
		return 1;
	}

	if (_tcsnicmp(param,_T("/m"),2) == 0)
	{
		mul = 1;
		param += 2;
	}

	val = _ttoi(param);
	Sleep(val * mul);

	return 0;
}

#endif /* INCLUDE_CMD_DELAY */
