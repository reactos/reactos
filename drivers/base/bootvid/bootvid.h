/*
 * ReactOS Boot video driver
 *
 * Copyright (C) 2003 Casper S. Hornstroup
 * Copyright (C) 2004, 2005 Filip Navara
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 */

#ifndef _BOOTVID_H
#define _BOOTVID_H

#include <ntddk.h>
#include <windef.h>
#include <wingdi.h>
#include <ndk/ldrfuncs.h>

#define PALETTE_FADE_STEPS  15
#define PALETTE_FADE_TIME   20 * 10000 /* 20ms */

BOOLEAN NTAPI VidInitialize(BOOLEAN);
VOID NTAPI VidCleanUp(VOID);
VOID NTAPI VidResetDisplay(VOID);
VOID NTAPI VidBufferToScreenBlt(PUCHAR, ULONG, ULONG, ULONG, ULONG, ULONG);
VOID NTAPI VidScreenToBufferBlt(PUCHAR, ULONG, ULONG, ULONG, ULONG, ULONG);
VOID NTAPI VidBitBlt(PUCHAR, ULONG, ULONG);
VOID NTAPI VidSolidColorFill(ULONG, ULONG, ULONG, ULONG, ULONG);
VOID NTAPI VidDisplayString(PCSTR);

typedef BOOLEAN (NTAPI *PVID_INITIALIZE)(BOOLEAN);
typedef VOID (NTAPI *PVID_CLEAN_UP)(VOID);
typedef VOID (NTAPI *PVID_RESET_DISPLAY)(VOID);
typedef VOID (NTAPI *PVID_BUFFER_TO_SCREEN_BLT)(PUCHAR, ULONG, ULONG, ULONG, ULONG, ULONG);
typedef VOID (NTAPI *PVID_SCREEN_TO_BUFFER_BLT)(PUCHAR, ULONG, ULONG, ULONG, ULONG, LONG);
typedef VOID (NTAPI *PVID_BITBLT)(PUCHAR, ULONG, ULONG);
typedef VOID (NTAPI *PVID_SOLID_COLOR_FILL)(ULONG, ULONG, ULONG, ULONG, ULONG);
typedef VOID (NTAPI *PVID_DISPLAY_STRING)(PCSTR);

typedef struct _VID_FUNCTION_TABLE
{
   PVID_INITIALIZE Initialize;
   PVID_CLEAN_UP CleanUp;
   PVID_RESET_DISPLAY ResetDisplay;
   PVID_BUFFER_TO_SCREEN_BLT BufferToScreenBlt;
   PVID_SCREEN_TO_BUFFER_BLT ScreenToBufferBlt;
   PVID_BITBLT BitBlt;
   PVID_SOLID_COLOR_FILL SolidColorFill;
   PVID_DISPLAY_STRING DisplayString;
} VID_FUNCTION_TABLE, *PVID_FUNCTION_TABLE;

#endif /* _BOOTVID_H */
