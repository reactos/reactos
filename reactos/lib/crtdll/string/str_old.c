/*
 * string_old.c
 *
 * Oldnames from ANSI header string.h
 *
 * Some wrapper functions for those old name functions whose appropriate
 * equivalents are not simply underscore prefixed.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRENTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warrenties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.2 $
 * $Author: ariadne $
 * $Date: 1999/04/02 21:44:05 $
 *
 */

#include <crtdll/string.h>
#include <crtdll/wchar.h>

int
strcasecmp (const char* sz1, const char* sz2)
{
	return _stricmp (sz1, sz2);
}

int
strncasecmp	(const char* sz1, const char* sz2, size_t sizeMaxCompare)
{
	return strnicmp (sz1, sz2, sizeMaxCompare);
}

int
wcscmpi (const wchar_t* ws1, const wchar_t* ws2)
{
	//return wcsicmp (ws1, ws2);
	return 0;
}

