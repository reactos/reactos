/* PowerPC Functions from gatomic.c in glib
 *
 * GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * g_atomic_*: atomic operations.
 * Copyright (C) 2003 Sebastian Wilhelmi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/************************************************************************
 *           InterlockedCompareExchange
 *
 * Atomically compares Destination and Comperand, and if found equal exchanges
 * the value of Destination with Exchange
 *
 * RETURNS
 *	Prior value of value pointed to by Destination
 */

/*
 * LONG NTAPI InterlockedCompareExchange(LPLONG Destination, LONG Exchange, LONG Comperand)
 */

#include <windows.h>
LONG
NTAPI
InterlockedCompareExchange(
	LPLONG Destination,
	long Exchange,
	LONG Comperand)
{
	LONG ret;
	__asm__ __volatile__ (
		"sync\n"
		"1: lwarx   %0,0,%1\n"
		"   subf.   %0,%2,%0\n"
		"   bne     2f\n"
		"   stwcx.  %3,0,%1\n"
		"   bne-    1b\n"
		"2: isync"
		: "=&r" (ret)
		: "b" (Destination), "r" (Comperand), "r" (Exchange)
		: "cr0", "memory"); 
	return ret;
}
