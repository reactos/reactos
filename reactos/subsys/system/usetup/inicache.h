/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/inicache.h
 * PURPOSE:         INI file parser that caches contents of INI file in memory
 * PROGRAMMER:      Royce Mitchell III
 *                  Eric Kohl
 */

#ifndef __INICACHE_H__
#define __INICACHE_H__


typedef struct _INICACHEKEY
{
  PWCHAR Name;
  PWCHAR Value;

  struct _INICACHEKEY *Next;
  struct _INICACHEKEY *Prev;
} INICACHEKEY, *PINICACHEKEY;


typedef struct _INICACHESECTION
{
  PWCHAR Name;

  PINICACHEKEY FirstKey;
  PINICACHEKEY LastKey;

  struct _INICACHESECTION *Next;
  struct _INICACHESECTION *Prev;
} INICACHESECTION, *PINICACHESECTION;


typedef struct _INICACHE
{
  PINICACHESECTION FirstSection;
  PINICACHESECTION LastSection;
} INICACHE, *PINICACHE;


typedef struct _PINICACHEITERATOR
{
  PINICACHESECTION Section;
  PINICACHEKEY Key;
} INICACHEITERATOR, *PINICACHEITERATOR;


/* FUNCTIONS ****************************************************************/

NTSTATUS
IniCacheLoad(PINICACHE *Cache,
	     PUNICODE_STRING FileName);

VOID
IniCacheDestroy(PINICACHE Cache);

PINICACHESECTION
IniCacheGetSection(PINICACHE Cache,
		   PWCHAR Name);

NTSTATUS
IniCacheGetKey(PINICACHESECTION Section,
	       PWCHAR KeyName,
	       PWCHAR *KeyValue);



PINICACHEITERATOR
IniCacheFindFirstValue(PINICACHESECTION Section,
		       PWCHAR *KeyName,
		       PWCHAR *KeyValue);

BOOLEAN
IniCacheFindNextValue(PINICACHEITERATOR Iterator,
		      PWCHAR *KeyName,
		      PWCHAR *KeyValue);

VOID
IniCacheFindClose(PINICACHEITERATOR Iterator);


#endif /* __INICACHE_H__ */

/* EOF */
