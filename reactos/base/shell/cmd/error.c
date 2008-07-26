/*
 *  ERROR.C - error reporting functions.
 *
 *
 *  History:
 *
 *    07/12/98 (Rob Lake)
 *        started
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    24-Jan-1999 (Eric Kohl)
 *        Redirection safe!
 *
 *    02-Feb-1999 (Eric Kohl)
 *        Use FormatMessage() for error reports.
 *
 *    28-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>


VOID ErrorMessage (DWORD dwErrorCode, LPTSTR szFormat, ...)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	TCHAR  szMessage[1024];
	LPTSTR szError;
	va_list arg_ptr;

	if (dwErrorCode == ERROR_SUCCESS)
		return;

  nErrorLevel = 1;

	if (szFormat)
	{
		va_start (arg_ptr, szFormat);
		_vstprintf (szMessage, szFormat, arg_ptr);
		va_end (arg_ptr);
	}

	if (FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
					   NULL, dwErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					   (LPTSTR)&szError, 0, NULL))
	{
		ConErrPrintf (_T("%s %s\n"), szError, szMessage);
		if(szError)
			LocalFree (szError);
		return;
	}

	/* Fall back just in case the error is not defined */
	if (szFormat)
		ConErrPrintf (_T("%s -- %s\n"), szMsg, szMessage);
	else
		ConErrPrintf (_T("%s\n"), szMsg);
}

VOID error_parameter_format(TCHAR ch)
{
	ConErrResPrintf(STRING_ERROR_PARAMETERF_ERROR, ch);
  nErrorLevel = 1;
}


VOID error_invalid_switch (TCHAR ch)
{
	ConErrResPrintf(STRING_ERROR_INVALID_SWITCH, ch);
  nErrorLevel = 1;
}


VOID error_too_many_parameters (LPTSTR s)
{
	ConErrResPrintf(STRING_ERROR_TOO_MANY_PARAMETERS, s);
  nErrorLevel = 1;
}


VOID error_path_not_found (VOID)
{
	ConErrResPuts(STRING_ERROR_PATH_NOT_FOUND);
	nErrorLevel = 1;
}


VOID error_file_not_found (VOID)
{
	ConErrResPuts(STRING_ERROR_FILE_NOT_FOUND);
	nErrorLevel = 1;
}


VOID error_sfile_not_found (LPTSTR f)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	LoadString(CMD_ModuleHandle, STRING_ERROR_FILE_NOT_FOUND, szMsg, RC_STRING_MAX_SIZE);
	ConErrPrintf(_T("%s - %s\n"), szMsg, f);
  nErrorLevel = 1;
}


VOID error_req_param_missing (VOID)
{
	ConErrResPuts(STRING_ERROR_REQ_PARAM_MISSING);
  nErrorLevel = 1;
}


VOID error_invalid_drive (VOID)
{
	ConErrResPuts(STRING_ERROR_INVALID_DRIVE);
  nErrorLevel = 1;
}


VOID error_bad_command (VOID)
{
	ConErrResPuts(STRING_ERROR_BADCOMMAND);
	nErrorLevel = 9009;
}


VOID error_no_pipe (VOID)
{
	ConErrResPuts(STRING_ERROR_CANNOTPIPE);
  nErrorLevel = 1;
}


VOID error_out_of_memory (VOID)
{
	ConErrResPuts(STRING_ERROR_OUT_OF_MEMORY);
  nErrorLevel = 1;
}


VOID error_invalid_parameter_format (LPTSTR s)
{
	ConErrResPuts(STRING_ERROR_INVALID_PARAM_FORMAT);
  nErrorLevel = 1;
}


VOID error_syntax (LPTSTR s)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	LoadString(CMD_ModuleHandle, STRING_ERROR_ERROR2, szMsg, RC_STRING_MAX_SIZE);

	if (s)
		ConErrPrintf(_T("%s - %s\n"), szMsg, s);
	else
		ConErrPrintf(_T("%s.\n"), szMsg);

  nErrorLevel = 1;
}


VOID msg_pause (VOID)
{
	ConOutResPuts(STRING_ERROR_D_PAUSEMSG);
}

/* EOF */
