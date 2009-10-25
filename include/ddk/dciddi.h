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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#define DCI_OK		0


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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __WINE_DCIDDI_H */
