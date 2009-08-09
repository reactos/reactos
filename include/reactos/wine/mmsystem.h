/*
 * MMSYSTEM - Multimedia Wine Extension ... :-)
 *
 * Copyright (C) the Wine project
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

#ifndef __WINE_MMSYSTEM_H
#define __WINE_MMSYSTEM_H

#include_next <mmsystem.h>

#define DRV_SUCCESS		0x0001
#define DRV_FAILURE		0x0000
#define DRV_EXITAPPLICATION     0x000C

#define MAXWAVEDRIVERS	10
#define MAXMIDIDRIVERS	10
#define MAXAUXDRIVERS	10
#define MAXMCIDRIVERS	32
#define MAXMIXERDRIVERS	10

#define MCI_OPEN_DRIVER			0x0801
#define MCI_CLOSE_DRIVER		0x0802
#define MCI_SOUND                       0x0812

#define MCI_SOUND_NAME                  0x00000100L

typedef LPCSTR		HPCSTR;         /* a huge version of LPCSTR */

typedef struct tagMCI_SOUND_PARMSA {
    DWORD_PTR   dwCallback;
    LPCSTR      lpstrSoundName;
} MCI_SOUND_PARMSA, *LPMCI_SOUND_PARMSA;

typedef struct tagMCI_SOUND_PARMSW {
    DWORD_PTR   dwCallback;
    LPCWSTR     lpstrSoundName;
} MCI_SOUND_PARMSW, *LPMCI_SOUND_PARMSW;

#ifdef UNICODE
typedef MCI_SOUND_PARMSW MCI_SOUND_PARMS;
#else
typedef MCI_SOUND_PARMSA MCI_SOUND_PARMS;
#endif

typedef struct midievent_tag *LPMIDIEVENT;

DWORD WINAPI GetDriverFlags(HDRVR hDriver);

#endif  /* __WINE_WINNT_H */
