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

#include "config.h"

#ifdef INCLUDE_CMD_DELAY

#include <tchar.h>
#include <windows.h>
#include <stdlib.h>

#include "cmd.h"


INT CommandDelay (LPTSTR cmd, LPTSTR param)
{
	DWORD val;
	DWORD mul=1000;

	if (_tcsncmp (param, _T("/?"), 2) == 0)
	{
		ConOutPuts(_T(
		              "pause for n seconds or milliseconds"
		              "\n"
		              "DELAY [/m]n\n"
		              "\n"
		              "  /m          specifiy than n are milliseconds\n"
		              "              otherwise n are seconds"));
		return 0;
	}

	if (*param==0)
	{
		error_req_param_missing ();
		return 1;
	}

	if (_tcsnicmp(param,"/m",2) == 0)
	{
		mul = 1;
		param += 2;
	}

	val = atoi(param);
	Sleep(val*mul);

	return 0;
}

#endif /* INCLUDE_CMD_DELAY */
