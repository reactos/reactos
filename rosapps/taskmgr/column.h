/*
 *  ReactOS Task Manager
 *
 *  column.h
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
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
	
#ifndef __COLUMN_H
#define __COLUMN_H

#define COLUMN_IMAGENAME			0
#define COLUMN_PID					1
#define COLUMN_USERNAME				2
#define COLUMN_SESSIONID			3
#define COLUMN_CPUUSAGE				4
#define COLUMN_CPUTIME				5
#define COLUMN_MEMORYUSAGE			6
#define COLUMN_PEAKMEMORYUSAGE		7
#define COLUMN_MEMORYUSAGEDELTA		8
#define COLUMN_PAGEFAULTS			9
#define COLUMN_PAGEFAULTSDELTA		10
#define COLUMN_VIRTUALMEMORYSIZE	11
#define COLUMN_PAGEDPOOL			12
#define COLUMN_NONPAGEDPOOL			13
#define COLUMN_BASEPRIORITY			14
#define COLUMN_HANDLECOUNT			15
#define COLUMN_THREADCOUNT			16
#define COLUMN_USEROBJECTS			17
#define COLUMN_GDIOBJECTS			18
#define COLUMN_IOREADS				19
#define COLUMN_IOWRITES				20
#define COLUMN_IOOTHER				21
#define COLUMN_IOREADBYTES			22
#define COLUMN_IOWRITEBYTES			23
#define COLUMN_IOOTHERBYTES			24

extern	UINT	ColumnDataHints[25];

void ProcessPage_OnViewSelectColumns(void);
void AddColumns(void);
void SaveColumnSettings(void);
void UpdateColumnDataHints(void);

#endif // __COlUMN_H