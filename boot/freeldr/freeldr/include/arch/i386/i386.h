/*
 *  FreeLoader
 *
 *  Copyright (C) 2003  Eric Kohl
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

// This is needed because headers define wrong one for ReactOS
#undef KIP0PCRADDRESS
#define KIP0PCRADDRESS                      0xffdff000

/* Bits to shift to convert a Virtual Address into an Offset in the Page Table */
#define PFN_SHIFT 12

/* Bits to shift to convert a Virtual Address into an Offset in the Page Directory */
#define PDE_SHIFT 22
#define PDE_SHIFT_PAE 18

/* Converts a Physical Address Pointer into a Page Frame Number */
#define PaPtrToPfn(p) \
    (((ULONG_PTR)&p) >> PFN_SHIFT)

/* Converts a Physical Address into a Page Frame Number */
#define PaToPfn(p) \
    ((p) >> PFN_SHIFT)

#define STARTUP_BASE                0xC0000000

#define LowMemPageTableIndex        0
#define StartupPageTableIndex       (STARTUP_BASE >> 22)
#define HalPageTableIndex           (HAL_BASE >> 22)

typedef struct _PAGE_DIRECTORY_X86
{
    HARDWARE_PTE Pde[1024];
} PAGE_DIRECTORY_X86, *PPAGE_DIRECTORY_X86;

void __cdecl i386DivideByZero(void);
void __cdecl i386DebugException(void);
void __cdecl i386NMIException(void);
void __cdecl i386Breakpoint(void);
void __cdecl i386Overflow(void);
void __cdecl i386BoundException(void);
void __cdecl i386InvalidOpcode(void);
void __cdecl i386FPUNotAvailable(void);
void __cdecl i386DoubleFault(void);
void __cdecl i386CoprocessorSegment(void);
void __cdecl i386InvalidTSS(void);
void __cdecl i386SegmentNotPresent(void);
void __cdecl i386StackException(void);
void __cdecl i386GeneralProtectionFault(void);
void __cdecl i386PageFault(void);
void __cdecl i386CoprocessorError(void);
void __cdecl i386AlignmentCheck(void);
void __cdecl i386MachineCheck(void);

/* EOF */
