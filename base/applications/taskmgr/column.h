/*
 *  ReactOS Task Manager
 *
 *  column.h
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#define COLUMN_IMAGENAME            0
#define COLUMN_PID                  1
#define COLUMN_USERNAME             2
#define COLUMN_SESSIONID            3
#define COLUMN_CPUUSAGE             4
#define COLUMN_CPUTIME              5
#define COLUMN_MEMORYUSAGE          6
#define COLUMN_PEAKMEMORYUSAGE      7
#define COLUMN_MEMORYUSAGEDELTA     8
#define COLUMN_PAGEFAULTS           9
#define COLUMN_PAGEFAULTSDELTA      10
#define COLUMN_VIRTUALMEMORYSIZE    11
#define COLUMN_PAGEDPOOL            12
#define COLUMN_NONPAGEDPOOL         13
#define COLUMN_BASEPRIORITY         14
#define COLUMN_HANDLECOUNT          15
#define COLUMN_THREADCOUNT          16
#define COLUMN_USEROBJECTS          17
#define COLUMN_GDIOBJECTS           18
#define COLUMN_IOREADS              19
#define COLUMN_IOWRITES             20
#define COLUMN_IOOTHER              21
#define COLUMN_IOREADBYTES          22
#define COLUMN_IOWRITEBYTES         23
#define COLUMN_IOOTHERBYTES         24
#define COLUMN_COMMANDLINE          25
#define COLUMN_NMAX                 26

/*
 * temporary fix:
 * Some macro IDS_* have different name from IDC_*
 * It would be better to unify thone name one day
 */
#define IDS_TAB_CPUUSAGE            IDS_TAB_CPU
#define IDS_TAB_MEMORYUSAGE         IDS_TAB_MEMUSAGE
#define IDS_TAB_PEAKMEMORYUSAGE     IDS_TAB_PEAKMEMUSAGE
#define IDS_TAB_MEMORYUSAGEDELTA    IDS_TAB_MEMDELTA
#define IDS_TAB_PAGEFAULTS          IDS_TAB_PAGEFAULT
#define IDS_TAB_PAGEFAULTSDELTA     IDS_TAB_PFDELTA
#define IDS_TAB_VIRTUALMEMORYSIZE   IDS_TAB_VMSIZE
#define IDS_TAB_NONPAGEDPOOL        IDS_TAB_NPPOOL
#define IDS_TAB_BASEPRIORITY        IDS_TAB_BASEPRI
#define IDS_TAB_HANDLECOUNT         IDS_TAB_HANDLES
#define IDS_TAB_THREADCOUNT         IDS_TAB_THREADS
#define IDS_TAB_USEROBJECTS         IDS_TAB_USERPBJECTS
#define IDS_TAB_IOWRITEBYTES        IDS_TAB_IOWRITESBYTES

typedef struct {
    DWORD   dwIdsName;
    DWORD   dwIdcCtrl;
    int     size;
    BOOL    bDefaults;
} PresetColumnEntry;


extern    UINT    ColumnDataHints[COLUMN_NMAX];
extern const PresetColumnEntry ColumnPresets[COLUMN_NMAX];

#define Column_ImageName            Columns[COLUMN_IMAGENAME]
#define Column_PID                  Columns[COLUMN_PID]
#define Column_CPUUsage             Columns[COLUMN_CPUUSAGE]
#define Column_CPUTime              Columns[COLUMN_CPUTIME]
#define Column_MemoryUsage          Columns[COLUMN_MEMORYUSAGE]
#define Column_MemoryUsageDelta     Columns[COLUMN_MEMORYUSAGEDELTA]
#define Column_PeakMemoryUsage      Columns[COLUMN_PEAKMEMORYUSAGE]
#define Column_PageFaults           Columns[COLUMN_PAGEFAULTS]
#define Column_USERObjects          Columns[COLUMN_USEROBJECTS]
#define Column_IOReads              Columns[COLUMN_IOREADS]
#define Column_IOReadBytes          Columns[COLUMN_IOREADBYTES]
#define Column_SessionID            Columns[COLUMN_SESSIONID]
#define Column_UserName             Columns[COLUMN_USERNAME]
#define Column_PageFaultsDelta      Columns[COLUMN_PAGEFAULTSDELTA]
#define Column_VirtualMemorySize    Columns[COLUMN_VIRTUALMEMORYSIZE]
#define Column_PagedPool            Columns[COLUMN_PAGEDPOOL]
#define Column_NonPagedPool         Columns[COLUMN_NONPAGEDPOOL]
#define Column_BasePriority         Columns[COLUMN_BASEPRIORITY]
#define Column_HandleCount          Columns[COLUMN_HANDLECOUNT]
#define Column_ThreadCount          Columns[COLUMN_THREADCOUNT]
#define Column_GDIObjects           Columns[COLUMN_GDIOBJECTS]
#define Column_IOWrites             Columns[COLUMN_IOWRITES]
#define Column_IOWriteBytes         Columns[COLUMN_IOWRITEBYTES]
#define Column_IOOther              Columns[COLUMN_IOOTHER]
#define Column_IOOtherBytes         Columns[COLUMN_IOOTHERBYTES]
#define Column_CommandLine          Columns[COLUMN_COMMANDLINE]

void ProcessPage_OnViewSelectColumns(void);
void AddColumns(void);
void SaveColumnSettings(void);
void UpdateColumnDataHints(void);
