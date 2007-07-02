/*
 * DCI driver interface
 *
 * Copyright (C) 2001 Ove Kaaven
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

#ifndef __WINE_DCIDDI_H
#define __WINE_DCIDDI_H

#ifdef __cplusplus
extern "C" {
#endif

/* DCI Command Escape */
#define DCICOMMAND	3075
#define DCI_VERSION	0x0100

#define DCICREATEPRIMARYSURFACE		1
#define DCICREATEOFFSCREENSURFACE	2
#define DCICREATEOVERLAYSURFACE		3
#define DCIENUMSURFACE			4
#define DCIESCAPE			5

/* DCI Errors */
#define DCI_OK		                0
#define DCI_FAIL_GENERIC               -1
#define DCI_FAIL_UNSUPPORTEDVERSION    -2
#define DCI_FAIL_INVALIDSURFACE        -3
#define DCI_FAIL_UNSUPPORTED           -4


typedef int DCIRVAL; /* DCI callback return type */

/*****************************************************************************
 * Escape command structures
 */
typedef struct _DCICMD {
    DWORD dwCommand;
    DWORD dwParam1;
    DWORD dwParam2;
    DWORD dwVersion;
    DWORD dwReserved;
} DCICMD,*LPDCICMD;

typedef struct _DCISURFACEINFO {
    DWORD dwSize;
    DWORD dwDCICaps;
    DWORD dwCompression;
    DWORD dwMask[3];
    DWORD dwWidth;
    DWORD dwHeight;
    LONG  lStride;
    DWORD dwBitCount;
    ULONG_PTR dwOffSurface;
    WORD  wSelSurface;
    WORD  wReserved;
    DWORD dwReserved1;
    DWORD dwReserved2;
    DWORD dwReserved3;
    DCIRVAL (CALLBACK *BeginAccess)(LPVOID, LPRECT);
    void (CALLBACK *EndAccess)(LPVOID);
    void (CALLBACK *DestroySurface)(LPVOID);
} DCISURFACEINFO, *LPDCISURFACEINFO;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __WINE_DCIDDI_H */
