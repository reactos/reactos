/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*****************************************************************************
 * Copyright 1998, Luiz Otavio L. Zorzella
 *           1999, Eric Pouech
 *
 * Purpose:   multimedia declarations (internal to WINMM & MMSYSTEM DLLs)
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
 *****************************************************************************
 */
#include "winemm.h"
#include "wine/mmsystem16.h"
#include "wownt32.h"

/* mmsystem (16 bit files) only functions */
void            MMDRV_Init16(void);
void 		MMSYSTEM_MMTIME16to32(LPMMTIME mmt32, const MMTIME16* mmt16);
void 		MMSYSTEM_MMTIME32to16(LPMMTIME16 mmt16, const MMTIME* mmt32);

typedef LONG			(*MCIPROC16)(DWORD, HDRVR16, WORD, DWORD, DWORD);

/* HANDLE16 -> HANDLE conversions */
#define HDRVR_32(h16)		((HDRVR)(ULONG_PTR)(h16))
#define HMIDI_32(h16)		((HMIDI)(ULONG_PTR)(h16))
#define HMIDIIN_32(h16)		((HMIDIIN)(ULONG_PTR)(h16))
#define HMIDIOUT_32(h16)	((HMIDIOUT)(ULONG_PTR)(h16))
#define HMIDISTRM_32(h16)	((HMIDISTRM)(ULONG_PTR)(h16))
#define HMIXER_32(h16)		((HMIXER)(ULONG_PTR)(h16))
#define HMIXEROBJ_32(h16)	((HMIXEROBJ)(ULONG_PTR)(h16))
#define HMMIO_32(h16)		((HMMIO)(ULONG_PTR)(h16))
#define HWAVE_32(h16)		((HWAVE)(ULONG_PTR)(h16))
#define HWAVEIN_32(h16)		((HWAVEIN)(ULONG_PTR)(h16))
#define HWAVEOUT_32(h16)	((HWAVEOUT)(ULONG_PTR)(h16))

/* HANDLE -> HANDLE16 conversions */
#define HDRVR_16(h32)		(LOWORD(h32))
#define HMIDI_16(h32)		(LOWORD(h32))
#define HMIDIIN_16(h32)		(LOWORD(h32))
#define HMIDIOUT_16(h32)	(LOWORD(h32))
#define HMIDISTRM_16(h32)	(LOWORD(h32))
#define HMIXER_16(h32)		(LOWORD(h32))
#define HMIXEROBJ_16(h32)	(LOWORD(h32))
#define HMMIO_16(h32)		(LOWORD(h32))
#define HWAVE_16(h32)		(LOWORD(h32))
#define HWAVEIN_16(h32)		(LOWORD(h32))
#define HWAVEOUT_16(h32)	(LOWORD(h32))
