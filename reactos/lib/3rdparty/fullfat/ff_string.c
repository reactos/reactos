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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ff_string.h"
#include "ff_error.h"

#ifdef FF_UNICODE_SUPPORT
#include <wchar.h>
#include <wctype.h>
#endif

/*
 *	These will eventually be moved into a platform independent string
 *	library. Which will be optional. (To allow the use of system specific versions).
 */

#ifdef FF_UNICODE_SUPPORT

void FF_cstrntowcs(FF_T_WCHAR *wcsDest, const FF_T_INT8 *szpSource, FF_T_UINT32 len) {
	while(*szpSource && len--) {
		*wcsDest++ = *szpSource++;
	}
	*wcsDest = '\0';
}

void FF_cstrtowcs(FF_T_WCHAR *wcsDest, const FF_T_INT8 *szpSource) {
	while(*szpSource) {
		*wcsDest++ = (FF_T_WCHAR) *szpSource++;
	}
	*wcsDest = '\0';
}

void FF_wcstocstr(FF_T_INT8 *szpDest, const FF_T_WCHAR *wcsSource) {
	while(*wcsSource) {
		*szpDest++ = (FF_T_INT8) *wcsSource++;
	}
	*szpDest = '\0';
}

void FF_wcsntocstr(FF_T_INT8 *szpDest, const FF_T_WCHAR *wcsSource, FF_T_UINT32 len) {
	while(*wcsSource && len--) {
		*szpDest++ = (FF_T_INT8) *wcsSource++;
	}
	*szpDest = '\0';
}

#endif

/**
 *	@private
 *	@brief	Converts an ASCII string to lowercase.
 **/
#ifndef FF_UNICODE_SUPPORT
/**
 *	@private
 *	@brief	Converts an ASCII string to uppercase.
 **/
void FF_toupper(FF_T_INT8 *string, FF_T_UINT32 strLen) {
	FF_T_UINT32 i;
	for(i = 0; i < strLen; i++) {
		if(string[i] >= 'a' && string[i] <= 'z')
			string[i] -= 32;
		if(string[i] == '\0') 
			break;
	}
}
void FF_tolower(FF_T_INT8 *string, FF_T_UINT32 strLen) {
	FF_T_UINT32 i;
	for(i = 0; i < strLen; i++) {
		if(string[i] >= 'A' && string[i] <= 'Z')
			string[i] += 32;
		if(string[i] == '\0') 
			break;
	}
}

#else
void FF_toupper(FF_T_WCHAR *string, FF_T_UINT32 strLen) {
	FF_T_UINT32 i;
	for(i = 0; i < strLen; i++) {
		string[i] = towupper(string[i]);
	}
}
void FF_tolower(FF_T_WCHAR *string, FF_T_UINT32 strLen) {
	FF_T_UINT32 i;
	for(i = 0; i < strLen; i++) {
		string[i] = towlower(string[i]);
	}
}
#endif




/**
 *	@private
 *	@brief	Compares 2 strings for the specified length, and returns FF_TRUE is they are identical
 *			otherwise FF_FALSE is returned.
 *
 **/

#ifndef FF_UNICODE_SUPPORT
FF_T_BOOL FF_strmatch(const FF_T_INT8 *str1, const FF_T_INT8 *str2, FF_T_UINT16 len) {
	register FF_T_UINT16 i;
	register FF_T_INT8	char1, char2;

	if(!len) {
		if(strlen(str1) != strlen(str2)) {
			return FF_FALSE;
		}
		len = (FF_T_UINT16) strlen(str1);
	}
	
	for(i = 0; i < len; i++) {
		char1 = str1[i];
		char2 = str2[i];
		if(char1 >= 'A' && char1 <= 'Z') {
			char1 += 32;
		}
		if(char2 >= 'A' && char2 <= 'Z') {
			char2 += 32;
		}

		if(char1 != char2) {
			return FF_FALSE;
		}
	}

	return FF_TRUE;
}
#else

FF_T_BOOL FF_strmatch(const FF_T_WCHAR *str1, const FF_T_WCHAR *str2, FF_T_UINT16 len) {
	register FF_T_UINT16 i;
	register FF_T_WCHAR	char1, char2;

	if(!len) {
		if(wcslen(str1) != wcslen(str2)) {
			return FF_FALSE;
		}
		len = (FF_T_UINT16) wcslen(str1);
	}
	
	for(i = 0; i < len; i++) {
		char1 = towlower(str1[i]);
		char2 = towlower(str2[i]);
		if(char1 != char2) {
			return FF_FALSE;
		}
	}

	return FF_TRUE;
}
#endif

/**
 *	@private
 *	@brief	A re-entrant Strtok function. No documentation is provided :P
 *			Use at your own risk. (This is for FullFAT's use only).
 **/

#ifndef FF_UNICODE_SUPPORT
FF_T_INT8 *FF_strtok(const FF_T_INT8 *string, FF_T_INT8 *token, FF_T_UINT16 *tokenNumber, FF_T_BOOL *last, FF_T_UINT16 Length) {
	FF_T_UINT16 strLen = Length;
	FF_T_UINT16 i,y, tokenStart, tokenEnd = 0;

	i = 0;
	y = 0;

	if(string[i] == '\\' || string[i] == '/') {
		i++;
	}

	tokenStart = i;

	while(i < strLen) {
		if(string[i] == '\\' || string[i] == '/') {
			y++;
			if(y == *tokenNumber) {
				tokenStart = (FF_T_UINT16)(i + 1);
			}
			if(y == (*tokenNumber + 1)) {
				tokenEnd = i;
				break;
			}
		}
		i++;
	}

	if(!tokenEnd) {
		if(*last == FF_TRUE) {
			return NULL;
		} else {
			*last = FF_TRUE;
		}
		tokenEnd = i;
	}
	if((tokenEnd - tokenStart) < FF_MAX_FILENAME) {
		memcpy(token, (string + tokenStart), (FF_T_UINT32)(tokenEnd - tokenStart));
		token[tokenEnd - tokenStart] = '\0';
	} else {
		memcpy(token, (string + tokenStart), (FF_T_UINT32)(FF_MAX_FILENAME));
		token[FF_MAX_FILENAME-1] = '\0';
	}
	//token[tokenEnd - tokenStart] = '\0';
	*tokenNumber += 1;

	return token;	
}

#else
FF_T_WCHAR *FF_strtok(const FF_T_WCHAR *string, FF_T_WCHAR *token, FF_T_UINT16 *tokenNumber, FF_T_BOOL *last, FF_T_UINT16 Length) {
	FF_T_UINT16 strLen = Length;
	FF_T_UINT16 i,y, tokenStart, tokenEnd = 0;

	i = 0;
	y = 0;

	if(string[i] == '\\' || string[i] == '/') {
		i++;
	}

	tokenStart = i;

	while(i < strLen) {
		if(string[i] == '\\' || string[i] == '/') {
			y++;
			if(y == *tokenNumber) {
				tokenStart = (FF_T_UINT16)(i + 1);
			}
			if(y == (*tokenNumber + 1)) {
				tokenEnd = i;
				break;
			}
		}
		i++;
	}

	if(!tokenEnd) {
		if(*last == FF_TRUE) {
			return NULL;
		} else {
			*last = FF_TRUE;
		}
		tokenEnd = i;
	}
	if((tokenEnd - tokenStart) < FF_MAX_FILENAME) {
		memcpy(token, (string + tokenStart), (FF_T_UINT32)(tokenEnd - tokenStart) * sizeof(FF_T_WCHAR));
		token[tokenEnd - tokenStart] = '\0';
	} else {
		memcpy(token, (string + tokenStart), (FF_T_UINT32)(FF_MAX_FILENAME) * sizeof(FF_T_WCHAR));
		token[FF_MAX_FILENAME-1] = '\0';
	}
	//token[tokenEnd - tokenStart] = '\0';
	*tokenNumber += 1;

	return token;	
}
#endif

/*
	A Wild-Card Comparator Library function, Provided by Adam Fullerton.
	This can be extended or altered to improve or advance wildCard matching
	of the FF_FindFirst() and FF_FindNext() API's.
*/
#ifdef FF_FINDAPI_ALLOW_WILDCARDS
/*FF_T_BOOL FF_wildcompare(const FF_T_INT8 *pszWildCard, const FF_T_INT8 *pszString) {
    // Check to see if the string contains the wild card 
    if (!memchr(pszWildCard, '*', strlen(pszWildCard)))
    {
        // if it does not then do a straight string compare
        if (strcmp(pszWildCard, pszString))
        {
            return FF_FALSE;
        }
    }
    else
    {
        while ((*pszWildCard)
        &&     (*pszString))
        {
            // Test for the wild card 
            if (*pszWildCard == '*')
            {
                // Eat more than one 
                while (*pszWildCard == '*')
                {
                    pszWildCard++;
                }
                // If there are more chars in the string
                if (*pszWildCard)
                {
                    // Search for the next char
                    pszString = memchr(pszString, (int)*pszWildCard,  strlen(pszString));
                    // if it does not exist then the strings don't match
                    if (!pszString)
                    {
                        return FF_FALSE;
                    }
                    
                }
                else
                {
                    if (*pszWildCard)
                    {
                        // continue
                        break;      
                    }
                    else
                    {
                        return FF_TRUE;
                    }
                }
            }
            else 
            {
                // Fail if they don't match 
                if (*pszWildCard != *pszString)
                {
                    return FF_FALSE;
                }
            }
            // Bump both pointers
            pszWildCard++;
            pszString++;
        }
        // fail if different lengths 
        if (*pszWildCard != *pszString)
        {
            return FF_FALSE;
        }
    }

    return FF_TRUE;
}*/
/*
	This is a better Wild-card compare function, that works perfectly, and is much more efficient.
	This function was contributed by one of our commercial customers.
*/
#ifdef FF_UNICODE_SUPPORT
FF_T_BOOL FF_wildcompare(const FF_T_WCHAR *pszWildCard, const FF_T_WCHAR *pszString) {
    register const FF_T_WCHAR *pszWc 	= NULL;
	register const FF_T_WCHAR *pszStr 	= NULL;	// Encourage the string pointers to be placed in memory.
    do {
        if ( *pszWildCard == '*' ) {
			while(*(1 + pszWildCard++) == '*'); // Eat up multiple '*''s
			pszWc = (pszWildCard - 1);
            pszStr = pszString;
        }
		if (*pszWildCard == '?' && !*pszString) {
			return FF_FALSE;	// False when the string is ended, yet a ? charachter is demanded.
		}
#ifdef FF_WILDCARD_CASE_INSENSITIVE
        if (*pszWildCard != '?' && tolower(*pszWildCard) != tolower(*pszString)) {
#else
		if (*pszWildCard != '?' && *pszWildCard != *pszString) {
#endif
			if (pszWc == NULL) {
				return FF_FALSE;
			}
            pszWildCard = pszWc;
            pszString = pszStr++;
        }
    } while ( *pszWildCard++ && *pszString++ );

	while(*pszWildCard == '*') {
		pszWildCard++;
	}

	if(!*(pszWildCard - 1)) {	// WildCard is at the end. (Terminated)
		return FF_TRUE;	// Therefore this must be a match.
	}

	return FF_FALSE;	// If not, then return FF_FALSE!
}
#else
FF_T_BOOL FF_wildcompare(const FF_T_INT8 *pszWildCard, const FF_T_INT8 *pszString) {
    register const FF_T_INT8 *pszWc 	= NULL;
	register const FF_T_INT8 *pszStr 	= NULL;	// Encourage the string pointers to be placed in memory.
    do {
        if ( *pszWildCard == '*' ) {
			while(*(1 + pszWildCard++) == '*'); // Eat up multiple '*''s
			pszWc = (pszWildCard - 1);
            pszStr = pszString;
        }
		if (*pszWildCard == '?' && !*pszString) {
			return FF_FALSE;	// False when the string is ended, yet a ? charachter is demanded.
		}
#ifdef FF_WILDCARD_CASE_INSENSITIVE
        if (*pszWildCard != '?' && tolower(*pszWildCard) != tolower(*pszString)) {
#else
		if (*pszWildCard != '?' && *pszWildCard != *pszString) {
#endif
			if (pszWc == NULL) {
				return FF_FALSE;
			}
            pszWildCard = pszWc;
            pszString = pszStr++;
        }
    } while ( *pszWildCard++ && *pszString++ );

	while(*pszWildCard == '*') {
		pszWildCard++;
	}

	if(!*(pszWildCard - 1)) {	// WildCard is at the end. (Terminated)
		return FF_TRUE;	// Therefore this must be a match.
	}

	return FF_FALSE;	// If not, then return FF_FALSE!
}
#endif

#endif
