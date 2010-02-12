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
#include "ff_string.h"

/*
 *	These will eventually be moved into a platform independent string
 *	library. Which will be optional. (To allow the use of system specific versions).
 */

/**
 *	@private
 *	@brief	Converts an ASCII string to lowercase.
 **/
void FF_tolower(FF_T_INT8 *string, FF_T_UINT32 strLen) {
	FF_T_UINT32 i;
	for(i = 0; i < strLen; i++) {
		if(string[i] >= 'A' && string[i] <= 'Z')
			string[i] += 32;
		if(string[i] == '\0') 
			break;
	}
}

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


/**
 *	@private
 *	@brief	Compares 2 strings for the specified length, and returns FF_TRUE is they are identical
 *			otherwise FF_FALSE is returned.
 *
 **/
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

/**
 *	@private
 *	@brief	A re-entrant Strtok function. No documentation is provided :P
 *			Use at your own risk. (This is for FullFAT's use only).
 **/
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
	
	memcpy(token, (string + tokenStart), (FF_T_UINT32)(tokenEnd - tokenStart));
	token[tokenEnd - tokenStart] = '\0';
	*tokenNumber += 1;

	return token;	
}


FF_T_BOOL FF_wildcompare(const FF_T_INT8 *pszWildCard, const FF_T_INT8 *pszString) {
    /* Check to see if the string contains the wild card */
    if (!memchr(pszWildCard, '*', strlen(pszWildCard)))
    {
        /* if it does not then do a straight string compare */
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
            /* Test for the wild card */
            if (*pszWildCard == '*')
            {
                /* Eat more than one */
                while (*pszWildCard == '*')
                {
                    pszWildCard++;
                }
                /* If there are more chars in the string */
                if (*pszWildCard)
                {
                    /* Search for the next char */
                    pszString = memchr(pszString, (int)*pszWildCard,  strlen(pszString));
                    /* if it does not exist then the strings don't match */
                    if (!pszString)
                    {
                        return FF_FALSE;
                    }
                    
                }
                else
                {
                    if (*pszWildCard)
                    {
                        /* continue */
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
                /* Fail if they don't match */
                if (*pszWildCard != *pszString)
                {
                    return FF_FALSE;
                }
            }
            /* Bump both pointers */
            pszWildCard++;
            pszString++;
        }
        /* fail if different lengths */
        if (*pszWildCard != *pszString)
        {
            return FF_FALSE;
        }
    }

    return FF_TRUE;
}

