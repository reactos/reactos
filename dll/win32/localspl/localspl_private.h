/*
 * Implementation of the Local Printmonitor: internal include file
 *
 * Copyright 2006 Detlef Riekenberg
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

#ifndef __WINE_LOCALSPL_PRIVATE__
#define __WINE_LOCALSPL_PRIVATE__


/* ## DLL-wide Globals ## */
extern HINSTANCE LOCALSPL_hInstance;
void setup_provider(void);

/* ## Resource-ID ## */
#define IDS_LOCALPORT       500
#define IDS_LOCALMONITOR    507

/* ## Reserved memorysize for the strings (in WCHAR) ## */
#define IDS_LOCALMONITOR_MAXLEN 64
#define IDS_LOCALPORT_MAXLEN 32

/* ## Type of Ports ## */
/* windows types */
#define PORT_IS_UNKNOWN  0
#define PORT_IS_LPT      1
#define PORT_IS_COM      2
#define PORT_IS_FILE     3
#define PORT_IS_FILENAME 4

/* wine extensions */
#define PORT_IS_WINE     5
#define PORT_IS_UNIXNAME 5
#define PORT_IS_PIPE     6
#define PORT_IS_CUPS     7
#define PORT_IS_LPR      8


/* ## Memory allocation functions ## */

static inline void * __WINE_ALLOC_SIZE(1) heap_alloc( size_t len )
{
    return HeapAlloc( GetProcessHeap(), 0, len );
}

static inline void * __WINE_ALLOC_SIZE(1) heap_alloc_zero( size_t len )
{
    return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len );
}

static inline BOOL heap_free( void *mem )
{
    return HeapFree( GetProcessHeap(), 0, mem );
}


#endif /* __WINE_LOCALSPL_PRIVATE__ */
