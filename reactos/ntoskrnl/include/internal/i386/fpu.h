/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_I386_FPU_H
#define __NTOSKRNL_INCLUDE_INTERNAL_I386_FPU_H

#define FN_CONTROL_WORD        0x0
#define FN_STATUS_WORD         0x4
#define FN_TAG_WORD            0x8
#define FN_DATA_SELECTOR       0x18
#define FN_CR0_NPX_STATE       0x20C
#define SIZEOF_FX_SAVE_AREA    528

#ifndef __ASM__

#include <internal/i386/ke.h>

typedef struct _FNSAVE_FORMAT {
	ULONG ControlWord;
	ULONG StatusWord;
	ULONG TagWord;
	ULONG ErrorOffset;
	ULONG ErrorSelector;
	ULONG DataOffset;
	ULONG DataSelector;
	UCHAR RegisterArea[80];
} FNSAVE_FORMAT, *PFNSAVE_FORMAT;

typedef struct _FXSAVE_FORMAT {
	USHORT ControlWord;
	USHORT StatusWord;
	USHORT TagWord;
	USHORT ErrorOpcode;
	ULONG ErrorOffset;
	ULONG ErrorSelector;
	ULONG DataOffset;
	ULONG DataSelector;
	ULONG MXCsr;
	ULONG MXCsrMask;
	UCHAR RegisterArea[128];
	UCHAR Reserved3[128];
	UCHAR Reserved4[224];
	UCHAR Align16Byte[8];
} FXSAVE_FORMAT, *PFXSAVE_FORMAT;

typedef struct _FX_SAVE_AREA {
	union {
		FNSAVE_FORMAT FnArea;
		FXSAVE_FORMAT FxArea;
	} U;
	ULONG NpxSavedCpu;
	ULONG Cr0NpxState;
} FX_SAVE_AREA, *PFX_SAVE_AREA;


extern ULONG HardwareMathSupport;

VOID
KiCheckFPU(VOID);

NTSTATUS
KiHandleFpuFault(PKTRAP_FRAME Tf, ULONG ExceptionNr);

VOID
KiFloatingSaveAreaToFxSaveArea(PFX_SAVE_AREA FxSaveArea, CONST FLOATING_SAVE_AREA *FloatingSaveArea);

BOOL
KiContextToFxSaveArea(PFX_SAVE_AREA FxSaveArea, PCONTEXT Context);

#endif /* !__ASM__ */

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_I386_FPU_H */

