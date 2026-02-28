/*
 * Copyright (c) 2002, TransGaming Technologies Inc.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef WINE_MTDLL_H
#define WINE_MTDLL_H

void __cdecl _unlock( int locknum );
void __cdecl _lock( int locknum );

#define _SIGNAL_LOCK    1
#define _IOB_SCAN_LOCK  2
#define _TMPNAM_LOCK    3
#define _INPUT_LOCK     4
#define _OUTPUT_LOCK    5
#define _CSCANF_LOCK    6
#define _CPRINTF_LOCK   7
#define _CONIO_LOCK     8
#define _HEAP_LOCK      9
#define _BHEAP_LOCK          10 /* No longer used? */
#define _TIME_LOCK      11
#define _ENV_LOCK       12
#define _EXIT_LOCK1     13
#define _EXIT_LOCK2          14
#define _THREADDATA_LOCK     15 /* No longer used? */
#define _POPEN_LOCK     16
#define _LOCKTAB_LOCK   17
#define _OSFHND_LOCK    18
#define _SETLOCALE_LOCK 19
#define _LC_COLLATE_LOCK     20 /* No longer used? */
#define _LC_CTYPE_LOCK       21 /* No longer used? */
#define _LC_MONETARY_LOCK    22 /* No longer used? */
#define _LC_NUMERIC_LOCK     23 /* No longer used? */
#define _LC_TIME_LOCK        24 /* No longer used? */
#define _MB_CP_LOCK     25
#define _NLG_LOCK       26
#define _TYPEINFO_LOCK  27
#define _STREAM_LOCKS   28

/* Must match definition in msvcrt/stdio.h */
#define _IOB_ENTRIES    20

#define _LAST_STREAM_LOCK  (_STREAM_LOCKS+_IOB_ENTRIES-1)

#define _TOTAL_LOCKS        (_LAST_STREAM_LOCK+1)

#endif /* WINE_MTDLL_H */
