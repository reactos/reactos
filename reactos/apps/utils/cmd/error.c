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
 *    24-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Redirection safe!
 *
 *    02-Feb-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Use FormatMessage() for error reports.
 */

#define WIN32_LEAN_AND_MEAN

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <stdarg.h>

#include "cmd.h"


#define INVALID_SWITCH			_T("Invalid switch - /%c\n")
#define TOO_MANY_PARAMETERS		_T("Too many parameters - %s\n")
#define PATH_NOT_FOUND			_T("Path not found\n")
#define FILE_NOT_FOUND			_T("File not found")
#define REQ_PARAM_MISSING		_T("Required parameter missing\n")
#define INVALID_DRIVE			_T("Invalid drive specification\n")
#define INVALID_PARAM_FORMAT	_T("Invalid parameter format - %s\n")
#define BADCOMMAND				_T("Bad command or filename\n")
#define OUT_OF_MEMORY			_T("Out of memory error.\n")
#define CANNOTPIPE				_T("Error!  Cannot pipe!  Cannot open temporary file!\n")

#define D_PAUSEMSG				_T("Press any key to continue . . .")



VOID ErrorMessage (DWORD dwErrorCode, LPTSTR szFormat, ...)
{
	TCHAR  szMessage[1024];
	LPTSTR szError;
	va_list arg_ptr;

	if (dwErrorCode == ERROR_SUCCESS)
		return;

	va_start (arg_ptr, szFormat);
	wvsprintf (szMessage, szFormat, arg_ptr);
	va_end (arg_ptr);

#if 1

	if (FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
					   NULL, dwErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					   (LPTSTR)&szError, 0, NULL))
	{
		ConErrPrintf (_T("%s %s\n"), szError, szMessage);
		LocalFree (szError);
		return;
	}
	else
	{
		ConErrPrintf (_T("Unknown error! Error code: 0x%lx\n"), dwErrorCode);
//		ConErrPrintf (_T("No error message available!\n"));
		return;
	}

#else

	switch (dwErrorCode)
	{
		case ERROR_FILE_NOT_FOUND:
			szError = _T("File not found --");
			break;

		case ERROR_PATH_NOT_FOUND:
			szError = _T("Path not found --");
			break;

		default:
			ConErrPrintf (_T("Unknown error! Error code: 0x%lx\n"), dwErrorCode);
			return;
	}

	ConErrPrintf (_T("%s %s\n"), szError, szMessage);
#endif
}



VOID error_invalid_switch (TCHAR ch)
{
	ConErrPrintf (INVALID_SWITCH, ch);
}


VOID error_too_many_parameters (LPTSTR s)
{
	ConErrPrintf (TOO_MANY_PARAMETERS, s);
}


VOID error_path_not_found (VOID)
{
	ConErrPrintf (PATH_NOT_FOUND);
}


VOID error_file_not_found (VOID)
{
	ConErrPrintf (FILE_NOT_FOUND);
}


VOID error_sfile_not_found (LPTSTR f)
{
	ConErrPrintf (FILE_NOT_FOUND _T(" - %s\n"), f);
}


VOID error_req_param_missing (VOID)
{
	ConErrPrintf (REQ_PARAM_MISSING);
}


VOID error_invalid_drive (VOID)
{
	ConErrPrintf (INVALID_DRIVE);
}


VOID error_bad_command (VOID)
{
	ConErrPrintf (BADCOMMAND);
}


VOID error_no_pipe (VOID)
{
	ConErrPrintf (CANNOTPIPE);
}


VOID error_out_of_memory (VOID)
{
	ConErrPrintf (OUT_OF_MEMORY);
}


VOID error_invalid_parameter_format (LPTSTR s)
{
	ConErrPrintf (INVALID_PARAM_FORMAT, s);
}


VOID error_syntax (LPTSTR s)
{
	if (s)
		ConErrPrintf (_T("Syntax error - %s\n"), s);
	else
		ConErrPrintf (_T("Syntax error.\n"));
}


VOID msg_pause (VOID)
{
	ConOutPuts (D_PAUSEMSG);
}
