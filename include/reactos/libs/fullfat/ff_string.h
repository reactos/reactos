/*****************************************************************************
 *  FullFAT - High Performance, Thread-Safe Embedded FAT File-System         *
 *  Copyright (C) 2009  James Walmsley (james@worm.me.uk)                    *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 *  IMPORTANT NOTICE:                                                        *
 *  =================                                                        *
 *  Alternative Licensing is available directly from the Copyright holder,   *
 *  (James Walmsley). For more information consult LICENSING.TXT to obtain   *
 *  a Commercial license.                                                    *
 *                                                                           *
 *  See RESTRICTIONS.TXT for extra restrictions on the use of FullFAT.       *
 *                                                                           *
 *  Removing the above notice is illegal and will invalidate this license.   *
 *****************************************************************************
 *  See http://worm.me.uk/fullfat for more information.                      *
 *  Or  http://fullfat.googlecode.com/ for latest releases and the wiki.     *
 *****************************************************************************/

/**
 *	@file		ff_string.c
 *	@author		James Walmsley
 *	@ingroup	STRING
 *
 *	@defgroup	STRING	FullFAT String Library
 *	@brief		Portable String Library for FullFAT
 *
 *
 **/

#ifndef _FF_STRING_H_
#define _FF_STRING_H_

#include "ff_types.h"
#include "ff_config.h"
#include <string.h>

#ifdef WIN32
#define FF_stricmp	stricmp
#else
#define FF_stricmp strcasecmp
#endif

#ifdef FF_UNICODE_SUPPORT
void			FF_tolower		(FF_T_WCHAR *string, FF_T_UINT32 strLen);
void			FF_toupper		(FF_T_WCHAR *string, FF_T_UINT32 strLen);
FF_T_BOOL		FF_strmatch		(const FF_T_WCHAR *str1, const FF_T_WCHAR *str2, FF_T_UINT16 len);
FF_T_WCHAR		*FF_strtok		(const FF_T_WCHAR *string, FF_T_WCHAR *token, FF_T_UINT16 *tokenNumber, FF_T_BOOL *last, FF_T_UINT16 Length);
FF_T_BOOL		FF_wildcompare	(const FF_T_WCHAR *pszWildCard, const FF_T_WCHAR *pszString);

// ASCII to UTF16 and UTF16 to ASCII routines. -- These are lossy routines, and are only for converting ASCII to UTF-16
// and the equivalent back to ASCII. Do not use them for international text.
void FF_cstrtowcs(FF_T_WCHAR *wcsDest, const FF_T_INT8 *szpSource);
void FF_wcstocstr(FF_T_INT8 *szpDest, const FF_T_WCHAR *wcsSource);
void FF_cstrntowcs(FF_T_WCHAR *wcsDest, const FF_T_INT8 *szpSource, FF_T_UINT32 len);
void FF_wcsntocstr(FF_T_INT8 *szpDest, const FF_T_WCHAR *wcsSource, FF_T_UINT32 len);

#else
void			FF_tolower		(FF_T_INT8 *string, FF_T_UINT32 strLen);
void			FF_toupper		(FF_T_INT8 *string, FF_T_UINT32 strLen);
FF_T_BOOL		FF_strmatch		(const FF_T_INT8 *str1, const FF_T_INT8 *str2, FF_T_UINT16 len);
FF_T_INT8		*FF_strtok		(const FF_T_INT8 *string, FF_T_INT8 *token, FF_T_UINT16 *tokenNumber, FF_T_BOOL *last, FF_T_UINT16 Length);
FF_T_BOOL		FF_wildcompare	(const FF_T_INT8 *pszWildCard, const FF_T_INT8 *pszString);
#endif

#endif
