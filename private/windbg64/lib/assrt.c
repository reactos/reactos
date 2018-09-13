/***
*assrt.c - assertions needed for string conversion routines
*
*	Copyright (c) 1991-1991, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*   Make sure that the data types used by the string conversion
*   routines have the right size. If this file does not compile,
*   the type definitions in mathsup.h should change appropriately.
*
*Revision History:
*   07-25-91	    GDP     written
*   05-26-92       GWK     Windbg srcs
*
*******************************************************************************/


#include "mathsup.h"

static void assertion_test(void)
{
    sizeof(u_char) == 1 ? 0 : 1/0,
    sizeof(u_short)  == 2 ? 0 : 1/0,
    sizeof(u_long) == 4 ? 0 : 1/0,
    sizeof(s_char) == 1 ? 0 : 1/0,
    sizeof(s_short)  == 2 ? 0 : 1/0,
    sizeof(s_long) == 4 ? 0 : 1/0;
#ifdef _ULDSUPPORT
    sizeof(long double) == 10 ? 0 : 1/0;
#endif
}
