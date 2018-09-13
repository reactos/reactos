/***
*atold.c - convert char string to long double
*
*   Copyright (c) 1989-1989, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   Converts a character string into a long double.
*
*Revision History:
*   03-09-89  WAJ   Initial version.
*   06-05-89  WAJ   Made changes for C6 compiler.
*   05-17-91  WAJ   Now uses long double.
*   07-22-91  GDP   Now uses _ULDOUBLE so that it can be used
*		    even if 'long double' is not supported.
*		    It also uses the C version of __strgtold()
*   04-30-92  GDP   Now calls _atoldbl
*   05-26-92       GWK     Windbg srcs
*
*******************************************************************************/

#include "mathsup.h"



/***
*_ULDOUBLE _atold( char * string ) - convert string to a long double
*
*Purpose:
*   _atold() recognizes an optional string of whitespace, then
*   an optional sign, then a string of digits optionally
*   containing a decimal point, then an optional e or E followed
*   by an optionally signed integer, and converts all this to
*   to a long double.  The first unrecognized character ends the string.
*
*Entry:
*   string - pointer to string to convert
*
*Exit:
*   returns long double value of character representation
*
*Exceptions:
*
*******************************************************************************/


_ULDOUBLE  _atold(char * string )
{
    _ULDOUBLE x;

    _atoldbl( (_ULDOUBLE *)&x, string);

    return( x );
}
