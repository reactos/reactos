/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __STDLIB_H
#define __STDLIB_H

#include <freeldr.h>

///////////////////////////////////////////////////////////////////////////////////////
//
// String Functions
//
///////////////////////////////////////////////////////////////////////////////////////
int		strlen(char *str);
char *	strcpy(char *dest, char *src);
char *	strncpy(char *dest, char *src, size_t count);
char *	strcat(char *dest, char *src);
char *	strncat(char *dst, const char *src, size_t n);
char *	strchr(const char *s, int c);
char *	strrchr(const char *s, int c);
int		strcmp(const char *string1, const char *string2);
int		stricmp(const char *string1, const char *string2);
int		strncmp(const char *string1, const char *string2, size_t length);
int		_strnicmp(const char *string1, const char *string2, size_t length);

///////////////////////////////////////////////////////////////////////////////////////
//
// Memory Functions
//
///////////////////////////////////////////////////////////////////////////////////////
int		RtlCompareMemory(const PVOID Source1, const PVOID Source2, ULONG Length);
VOID	RtlCopyMemory(PVOID Destination, const PVOID Source, ULONG Length);
VOID	RtlFillMemory(PVOID Destination, ULONG Length, UCHAR Fill);
VOID	RtlZeroMemory(PVOID Destination, ULONG Length);

#define	memcmp(buf1, buf2, count)	RtlCompareMemory(buf1, buf2, count)
#define	memcpy(dest, src, count)	RtlCopyMemory(dest, src,count)
#define	memset(dest, c, count)		RtlFillMemory(dest,count, c)

///////////////////////////////////////////////////////////////////////////////////////
//
// Standard Library Functions
//
///////////////////////////////////////////////////////////////////////////////////////
int		atoi(char *string);
char *	itoa(int value, char *string, int radix);
int		toupper(int c);
int		tolower(int c);

int		isspace(int c);
int		isdigit(int c);
int		isxdigit(int c);

char *	convert_to_ascii(char *buf, int c, ...);

void	putchar(int ch);		// Implemented in asmcode.S
int		kbhit(void);			// Implemented in asmcode.S
int		getch(void);			// Implemented in asmcode.S
int		getyear(void);			// Implemented in asmcode.S
int		getday(void);			// Implemented in asmcode.S
int		getmonth(void);			// Implemented in asmcode.S
int		gethour(void);			// Implemented in asmcode.S
int		getminute(void);		// Implemented in asmcode.S
int		getsecond(void);		// Implemented in asmcode.S

#ifndef max
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

///////////////////////////////////////////////////////////////////////////////////////
//
// Screen Output Functions
//
///////////////////////////////////////////////////////////////////////////////////////
void	print(char *str);
void	printf(char *fmt, ...);
void	sprintf(char *buffer, char *format, ...);

///////////////////////////////////////////////////////////////////////////////////////
//
// List Functions
//
///////////////////////////////////////////////////////////////////////////////////////

typedef struct _LIST_ITEM
{
	struct _LIST_ITEM*	ListPrev;
	struct _LIST_ITEM*	ListNext;
	
} LIST_ITEM, *PLIST_ITEM;

VOID		RtlListInitializeHead(PLIST_ITEM ListHead);							// Initializes a doubly linked list
VOID		RtlListInsertHead(PLIST_ITEM ListHead, PLIST_ITEM Entry);			// Inserts an entry at the head of the list
VOID		RtlListInsertTail(PLIST_ITEM ListHead, PLIST_ITEM Entry);			// Inserts an entry at the tail of the list
PLIST_ITEM	RtlListRemoveHead(PLIST_ITEM ListHead);								// Removes the entry at the head of the list
PLIST_ITEM	RtlListRemoveTail(PLIST_ITEM ListHead);								// Removes the entry at the tail of the list
PLIST_ITEM	RtlListGetHead(PLIST_ITEM ListHead);								// Returns the entry at the head of the list
PLIST_ITEM	RtlListGetTail(PLIST_ITEM ListHead);								// Returns the entry at the tail of the list
BOOL		RtlListIsEmpty(PLIST_ITEM ListHead);								// Indicates whether a doubly linked list is empty
ULONG		RtlListCountEntries(PLIST_ITEM ListHead);							// Counts the entries in a doubly linked list
PLIST_ITEM	RtlListGetPrevious(PLIST_ITEM ListEntry);							// Returns the previous item in the list
PLIST_ITEM	RtlListGetNext(PLIST_ITEM ListEntry);								// Returns the next item in the list
PLIST_ITEM	RtlListRemoveEntry(PLIST_ITEM ListEntry);							// Removes the entry from the list
VOID		RtlListInsertEntry(PLIST_ITEM InsertAfter, PLIST_ITEM ListEntry);	// Inserts a new list entry right after the specified one
VOID		RtlListMoveEntryPrevious(PLIST_ITEM ListEntry);						// Moves the list entry to before the previous entry
VOID		RtlListMoveEntryNext(PLIST_ITEM ListEntry);							// Moves the list entry to after the next entry


#endif  // defined __STDLIB_H
