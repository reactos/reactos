/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Unicode Prefix implementation
 * FILE:              lib/rtl/unicodeprfx.c
 */

#include "rtl.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
* @unimplemented
*/
PUNICODE_PREFIX_TABLE_ENTRY
STDCALL
RtlFindUnicodePrefix (
	PUNICODE_PREFIX_TABLE PrefixTable,
	PUNICODE_STRING FullName,
	ULONG CaseInsensitiveIndex
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
VOID
STDCALL
RtlInitializeUnicodePrefix (
	PUNICODE_PREFIX_TABLE PrefixTable
	)
{
	UNIMPLEMENTED;
}

/*
* @unimplemented
*/
BOOLEAN
STDCALL
RtlInsertUnicodePrefix (
	PUNICODE_PREFIX_TABLE PrefixTable,
	PUNICODE_STRING Prefix,
	PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
* @unimplemented
*/
PUNICODE_PREFIX_TABLE_ENTRY
STDCALL
RtlNextUnicodePrefix (
	PUNICODE_PREFIX_TABLE PrefixTable,
	BOOLEAN Restart
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
VOID
STDCALL
RtlRemoveUnicodePrefix (
	PUNICODE_PREFIX_TABLE PrefixTable,
	PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry
	)
{
	UNIMPLEMENTED;
}

/* EOF */
