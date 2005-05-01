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
 *
 *    28-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc  
 */

#include "precomp.h"
#include "resource.h"

/*
#define PARAMETERF_ERROR	_T("Parameter format not correct - %c\n")
#define INVALID_SWITCH		_T("Invalid switch - /%c\n")
#define TOO_MANY_PARAMETERS	_T("Too many parameters - %s\n")
#define PATH_NOT_FOUND		_T("Path not found\n")
#define FILE_NOT_FOUND		_T("File not found\n")
#define REQ_PARAM_MISSING	_T("Required parameter missing\n")
#define INVALID_DRIVE		_T("Invalid drive specification\n")
#define INVALID_PARAM_FORMAT	_T("Invalid parameter format - %s\n")
#define BADCOMMAND		_T("Bad command or filename\n")
#define OUT_OF_MEMORY		_T("Out of memory error.\n")
#define CANNOTPIPE		_T("Error!  Cannot pipe!  Cannot open temporary file!\n")

#define D_PAUSEMSG		_T("Press any key to continue . . .")
*/


VOID ErrorMessage (DWORD dwErrorCode, LPTSTR szFormat, ...)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	TCHAR  szMessage[1024];
#ifndef __REACTOS__
	LPTSTR szError;
#endif
	va_list arg_ptr;

	if (dwErrorCode == ERROR_SUCCESS)
		return;

	if (szFormat)
	{
		va_start (arg_ptr, szFormat);
		_vstprintf (szMessage, szFormat, arg_ptr);
		va_end (arg_ptr);
	}

#ifndef __REACTOS__

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
		LoadString(GetModuleHandle(NULL), STRING_ERROR_ERROR1, szMsg, RC_STRING_MAX_SIZE);
		ConErrPrintf(szMsg, dwErrorCode);
		return;
	}

#else

	switch (dwErrorCode)
	{
		case ERROR_FILE_NOT_FOUND:
			LoadString(GetModuleHandle(NULL), STRING_ERROR_FILE_NOT_FOUND, szMsg, RC_STRING_MAX_SIZE);
			break;

		case ERROR_PATH_NOT_FOUND:
			LoadString(GetModuleHandle(NULL), STRING_ERROR_PATH_NOT_FOUND, szMsg, RC_STRING_MAX_SIZE);
			break;

		case ERROR_NOT_READY:
			LoadString(GetModuleHandle(NULL), STRING_ERROR_DRIVER_NOT_READY, szMsg, RC_STRING_MAX_SIZE);
			break;

		default:
			LoadString(GetModuleHandle(NULL), STRING_ERROR_ERROR1, szMsg, RC_STRING_MAX_SIZE);
			ConErrPrintf(szMsg);
			return;
	}

	if (szFormat)
		ConErrPrintf (_T("%s -- %s\n"), szMsg, szMessage);
	else
		ConErrPrintf (_T("%s\n"), szMsg);
#endif
}

VOID error_parameter_format(TCHAR ch)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	LoadString(GetModuleHandle(NULL), STRING_ERROR_PARAMETERF_ERROR, szMsg, RC_STRING_MAX_SIZE);
	ConErrPrintf(szMsg, ch);
}


VOID error_invalid_switch (TCHAR ch)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	LoadString(GetModuleHandle(NULL), STRING_ERROR_INVALID_SWITCH, szMsg, RC_STRING_MAX_SIZE);
	ConErrPrintf(szMsg, ch);
}


VOID error_too_many_parameters (LPTSTR s)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	LoadString(GetModuleHandle(NULL), STRING_ERROR_TOO_MANY_PARAMETERS, szMsg, RC_STRING_MAX_SIZE);
	ConErrPrintf(szMsg, s);
}


VOID error_path_not_found (VOID)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	LoadString(GetModuleHandle(NULL), STRING_ERROR_PATH_NOT_FOUND, szMsg, RC_STRING_MAX_SIZE);
	ConErrPrintf(szMsg);
}


VOID error_file_not_found (VOID)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	LoadString(GetModuleHandle(NULL), STRING_ERROR_FILE_NOT_FOUND, szMsg, RC_STRING_MAX_SIZE);
	ConErrPrintf(szMsg);
}


VOID error_sfile_not_found (LPTSTR f)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	LoadString(GetModuleHandle(NULL), STRING_ERROR_FILE_NOT_FOUND, szMsg, RC_STRING_MAX_SIZE);
	ConErrPrintf(_T("%s - %s\n"), szMsg, f);
}


VOID error_req_param_missing (VOID)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	LoadString(GetModuleHandle(NULL), STRING_ERROR_REQ_PARAM_MISSING, szMsg, RC_STRING_MAX_SIZE);
	ConErrPrintf(szMsg);
}


VOID error_invalid_drive (VOID)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	LoadString(GetModuleHandle(NULL), STRING_ERROR_INVALID_DRIVE, szMsg, RC_STRING_MAX_SIZE);
	ConErrPrintf(szMsg);
}


VOID error_bad_command (VOID)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	LoadString(GetModuleHandle(NULL), STRING_ERROR_BADCOMMAND, szMsg, RC_STRING_MAX_SIZE);
	ConErrPrintf(szMsg);
}


VOID error_no_pipe (VOID)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	LoadString(GetModuleHandle(NULL), STRING_ERROR_CANNOTPIPE, szMsg, RC_STRING_MAX_SIZE);
	ConErrPrintf(szMsg);
}


VOID error_out_of_memory (VOID)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	LoadString(GetModuleHandle(NULL), STRING_ERROR_OUT_OF_MEMORY, szMsg, RC_STRING_MAX_SIZE);
	ConErrPrintf(szMsg);
}


VOID error_invalid_parameter_format (LPTSTR s)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	LoadString(GetModuleHandle(NULL), STRING_ERROR_INVALID_PARAM_FORMAT, szMsg, RC_STRING_MAX_SIZE);
	ConErrPrintf(szMsg, s);
}


VOID error_syntax (LPTSTR s)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	LoadString(GetModuleHandle(NULL), STRING_ERROR_ERROR2, szMsg, RC_STRING_MAX_SIZE);

	if (s)
		ConErrPrintf(_T("%s - %s\n"), szMsg, s);
	else
		ConErrPrintf(_T("%s.\n"), szMsg);
}


VOID msg_pause (VOID)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	LoadString(GetModuleHandle(NULL), STRING_ERROR_D_PAUSEMSG, szMsg, RC_STRING_MAX_SIZE);
	ConOutPuts(szMsg);
}

/* EOF */
