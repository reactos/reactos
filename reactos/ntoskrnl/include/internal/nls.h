/*
 *  ReactOS kernel
 *  Copyright (C) 2003 Eric Kohl <ekohl@rz-online.de>
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

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_NLS_H
#define __NTOSKRNL_INCLUDE_INTERNAL_NLS_H

extern PVOID NlsSectionObject;

extern ULONG NlsAnsiTableOffset;
extern ULONG NlsOemTableOffset;
extern ULONG NlsUnicodeTableOffset;


extern PUSHORT NlsUnicodeUpcaseTable;
PUSHORT NlsUnicodeLowercaseTable;


VOID RtlpCreateDefaultNlsTables(VOID);

VOID RtlpImportAnsiCodePage(PUSHORT TableBase, ULONG Size);
VOID RtlpImportOemCodePage(PUSHORT TableBase, ULONG Size);
VOID RtlpImportUnicodeCasemap(PUSHORT TableBase, ULONG Size);
VOID RtlpCreateNlsSection(VOID);

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_NLS_H */

/* EOF */
