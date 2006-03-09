/*
 *  FreeLoader - registry.h
 *
 *  Copyright (C) 2001  Eric Kohl
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

#ifndef __REGISTRY_H
#define __REGISTRY_H

typedef struct _REG_KEY
{
  LIST_ENTRY KeyList;
  LIST_ENTRY SubKeyList;
  LIST_ENTRY ValueList;

  ULONG SubKeyCount;
  ULONG ValueCount;

  ULONG NameSize;
  PCHAR Name;

  /* default data */
  ULONG DataType;
  ULONG DataSize;
  PCHAR Data;
} KEY, *FRLDRHKEY, **PFRLDRHKEY;


typedef struct _REG_VALUE
{
  LIST_ENTRY ValueList;

  /* value name */
  ULONG NameSize;
  PCHAR Name;

  /* value data */
  ULONG DataType;
  ULONG DataSize;
  PCHAR Data;
} VALUE, *PVALUE;


#define ERROR_SUCCESS                    0L
#define ERROR_OUTOFMEMORY                14L
#define ERROR_INVALID_PARAMETER          87L
#define ERROR_MORE_DATA                  234L
#define ERROR_NO_MORE_ITEMS              259L

#define assert(x)

VOID
RegInitializeRegistry(VOID);

LONG
RegInitCurrentControlSet(BOOL LastKnownGood);


LONG
RegCreateKey(FRLDRHKEY ParentKey,
	     PCHAR KeyName,
	     PFRLDRHKEY Key);

LONG
RegDeleteKey(FRLDRHKEY Key,
	     PCHAR Name);

LONG
RegEnumKey(FRLDRHKEY Key,
	   ULONG Index,
	   PCHAR Name,
	   ULONG* NameSize);

LONG
RegOpenKey(FRLDRHKEY ParentKey,
	   PCHAR KeyName,
	   PFRLDRHKEY Key);


LONG
RegSetValue(FRLDRHKEY Key,
	    PCHAR ValueName,
	    ULONG Type,
	    PCHAR Data,
	    ULONG DataSize);

LONG
RegQueryValue(FRLDRHKEY Key,
	      PCHAR ValueName,
	      ULONG* Type,
	      PUCHAR Data,
	      ULONG* DataSize);

LONG
RegDeleteValue(FRLDRHKEY Key,
	       PCHAR ValueName);

LONG
RegEnumValue(FRLDRHKEY Key,
	     ULONG Index,
	     PCHAR ValueName,
	     ULONG* NameSize,
	     ULONG* Type,
	     PUCHAR Data,
	     ULONG* DataSize);

ULONG
RegGetSubKeyCount (FRLDRHKEY Key);

ULONG
RegGetValueCount (FRLDRHKEY Key);


BOOL
RegImportBinaryHive (PCHAR ChunkBase,
		     ULONG ChunkSize);

BOOL
RegExportBinaryHive (PCHAR KeyName,
		     PCHAR ChunkBase,
		     ULONG* ChunkSize);


#endif /* __REGISTRY_H */

/* EOF */

