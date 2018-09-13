/***
*printf.c - print formatted to stdout
*
*	Copyright (c) 1985-1991, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines w4printf() - print formatted data to stdout
*	defines w4vprintf() - print formatted output to stdout, get data
*			      from an argument ptr instead of explicit args.
*******************************************************************************/

#ifdef FLAT
#include "dprintf.h"		// function prototypes

#define _W4PRINTF_
#include "printf.h"
#endif
