/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
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
/* $Id$
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/genlist.h
 * PURPOSE:         Generic list functions
 * PROGRAMMER:      Eric Kohl
 */

#ifndef __GENLIST_H__
#define __GENLIST_H__

typedef struct _GENERIC_LIST_ENTRY
{
  LIST_ENTRY Entry;
  PVOID UserData;
  CHAR Text[1];
} GENERIC_LIST_ENTRY, *PGENERIC_LIST_ENTRY;


typedef struct _GENERIC_LIST
{
  LIST_ENTRY ListHead;

  SHORT Left;
  SHORT Top;
  SHORT Right;
  SHORT Bottom;

  PGENERIC_LIST_ENTRY CurrentEntry;
  PGENERIC_LIST_ENTRY BackupEntry;
} GENERIC_LIST, *PGENERIC_LIST;



PGENERIC_LIST
CreateGenericList(VOID);

VOID
DestroyGenericList(PGENERIC_LIST List,
		   BOOLEAN FreeUserData);

BOOLEAN
AppendGenericListEntry(PGENERIC_LIST List,
		       PCHAR Text,
		       PVOID UserData,
		       BOOLEAN Current);

VOID
DrawGenericList(PGENERIC_LIST List,
		SHORT Left,
		SHORT Top,
		SHORT Right,
		SHORT Bottom);

VOID
ScrollDownGenericList(PGENERIC_LIST List);

VOID
ScrollUpGenericList(PGENERIC_LIST List);

PGENERIC_LIST_ENTRY
GetGenericListEntry(PGENERIC_LIST List);

VOID
SaveGenericListState(PGENERIC_LIST List);

VOID
RestoreGenericListState(PGENERIC_LIST List);

#endif /* __GENLIST_H__ */

/* EOF */
