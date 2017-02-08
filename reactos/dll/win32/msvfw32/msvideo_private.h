/*
 * Copyright 1999 Marcus Meissner
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

#ifndef __WINE_MSVIDEO_PRIVATE_H
#define __WINE_MSVIDEO_PRIVATE_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <wingdi.h>
#include <vfw.h>

#include <wine/debug.h>

#define ICM_CHOOSE_COMPRESSOR 1

/* HIC struct (same layout as Win95 one) */
typedef struct tagWINE_HIC {
    DWORD		magic;		/* 00: 'Smag' */
    HANDLE		curthread;	/* 04: */
    DWORD		type;		/* 08: */
    DWORD		handler;	/* 0C: */
    HDRVR		hdrv;		/* 10: */
    DWORD		private;	/* 14:(handled by SendDriverMessage)*/
    DRIVERPROC  	driverproc;	/* 18:(handled by SendDriverMessage)*/
    DWORD		x1;		/* 1c: name? */
    WORD		x2;		/* 20: */
    DWORD		x3;		/* 22: */
					/* 26: */
    HIC                 hic;
    DWORD               driverId;
    struct tagWINE_HIC* next;
} WINE_HIC;

extern HMODULE MSVFW32_hModule DECLSPEC_HIDDEN;

#endif  /* __WINE_MSVIDEO_PRIVATE_H */
