/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            include/kernel32/atom.h
 * PURPOSE:         Include file for lib/kernel32/atom.c
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */
/********************************************************************


This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.


********************************************************************/
 
#ifndef Atoms__h
#define Atoms__h

#include <windows.h>
//#include <ctype.h>
//#include <types.h>


typedef unsigned long ATOMID;

typedef struct {
	ATOMID	q;		/* what a string 'hashes' to 	*/
	long   	idx;		/* index into data table	*/
	long	refcnt;		/* how many clients have it 	*/
	long	idsize;		/* space used by this slot	*/
} ATOMENTRY;
typedef ATOMENTRY *LPATOMENTRY;

typedef struct {
	ATOMENTRY 	*AtomTable;	/* pointer to table data 	*/
	WCHAR		*AtomData;	/* pointer to name data 	*/
	unsigned long	 TableSize;	/* number items in this table   */
	unsigned long	 DataSize;	/* space used by string data    */
	LPVOID		 lpDrvData;
} ATOMTABLE;
typedef ATOMTABLE *LPATOMTABLE;

#endif /* Atoms__h */
