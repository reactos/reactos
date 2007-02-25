/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/intrlck/decrement.c
 * PURPOSE:     Inter lock decrements
 * PROGRAMMERS: Copyright 1995 Martin von Loewis
 *              Copyright 1997 Onno Hovers
 */

#include <windows.h>

/************************************************************************
*           InterlockedDecrement                                        *
*                                                                       *
* InterlockedDecrement adds -1 to a long variable and returns           *
* the resulting decremented value.                                      *
*                                                                       *
************************************************************************/

LONG NTAPI
InterlockedDecrement(
	LPLONG lpAddend)
{
	return InterlockedExchangeAdd( lpAddend, -1 ) - 1;
}
