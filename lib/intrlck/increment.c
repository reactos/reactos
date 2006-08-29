/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/intrlck/increment.c
 * PURPOSE:     Inter lock increments
 * PROGRAMMERS: Copyright 1995 Martin von Loewis
 *              Copyright 1997 Onno Hovers
 */

#include <windows.h>

/************************************************************************
*           InterlockedIncrement                                        *
*                                                                       *
* InterlockedIncrement adds 1 to a long variable and returns            *
* the resulting incremented value.                                      *
*                                                                       *
************************************************************************/

LONG NTAPI
InterlockedIncrement(
	LPLONG lpAddend)
{
	return InterlockedExchangeAdd( lpAddend, 1 ) + 1;
}
