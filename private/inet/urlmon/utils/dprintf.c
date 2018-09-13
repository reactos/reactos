/***
*dprintf.c - print formatted to debug port
*
*	Copyright (c) 1985-1991, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines w4dprintf() - print formatted data to debug port
*	defines w4vdprintf() - print formatted output to debug port, get data
*			       from an argument ptr instead of explicit args.
*******************************************************************************/

#include "dprintf.h"		// function prototypes

#define _W4DPRINTF_
#include "printf.h"
