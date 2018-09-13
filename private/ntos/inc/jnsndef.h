/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1992  Digital Equipment Corporation

Module Name:

    jnsndef.h

Abstract:

    This module is the header file that describes hardware addresses
    for the Jensen systems.

Author:

    David N. Cutler (davec) 26-Nov-1990
    Miche Baker-Harvey (miche) 13-May 1992

Revision History:

    2-Oct-1992 Rod Gamache [DEC]
      Add DEVICE_LEVEL.

    22-Jul-1992 Jeff McLeman (mcleman)
      Add three define QVA's for DMA control,
      Video memory and video control. These are the
      same addresses the FW wil use, so they can reside
      here. Add INT_CONTROL and VA for INTA.

    12-Jul-1992 Jeff McLeman (mcleman)
      Move PTRANSLATION_ENTRY to jnsndma.h

    07-Jul-1992 Jeff McLeman (mcleman)
      Add PTRANSLATION_ENTRY structure

--*/

#ifndef _JNSNDEF_
#define _JNSNDEF_

//
// Include the reference definitions.
//

#include "alpharef.h"

//
// Nothing actually changed here - MBH; device stuff changed below
//

//
// Define physical base addresses for system mapping.
//

#define LOCAL_MEMORY_PHYSICAL_BASE      ((ULONGLONG)0x000000000)
#define EISA_INTA_CYCLE_PHYSICAL_BASE   ((ULONGLONG)0x100000000)
#define FEPROM_0_PHYSICAL_BASE          ((ULONGLONG)0x180000000)
#define FEPROM_1_PHYSICAL_BASE          ((ULONGLONG)0x1A0000000)
#define COMBO_CHIP_PHYSICAL_BASE        ((ULONGLONG)0x1C0000000)
#define HAE_PHYSICAL_BASE               ((ULONGLONG)0x1D0000000)
#define SYSTEM_CTRL_REG_PHYSICAL_BASE   ((ULONGLONG)0x1E0000000)
#define SPARE_REGISTER_PHYSICAL_BASE    ((ULONGLONG)0x1F0000000)
#define EISA_MEMORY_PHYSICAL_BASE       ((ULONGLONG)0x200000000)
#define EISA_IO_PHYSICAL_BASE           ((ULONGLONG)0x300000000)

//
// Define interesting device addresses
//

#define SP_PHYSICAL_BASE ((ULONGLONG)0x1c007f000) // physical base of serial port 0

#define DMA_PHYSICAL_BASE EISA_IO_PHYSICAL_BASE // physical base of DMA control

#define RTCLOCK_PHYSICAL_BASE ((ULONGLONG)0x1c002e000)

//
// define VECTORS for Jensen
//

#define KEYBOARD_MOUSE_VECTOR 11

//
// define QVA's that are needed
//

#define DMA_VIRTUAL_BASE 0xB8000000   // virtual base of DMA control

#define VIDEO_CONTROL_VIRTUAL_BASE  0xB80003C0 // virtual base, video control

#define VIDEO_MEMORY_VIRTUAL_BASE  0xB00B8000 // virtual base of video memory

#define EISA_INTA_CYCLE_VIRTUAL_BASE 0xA0000000 // VA of intack

#define HAE_VIRTUAL_BASE 0xA0E80000  // VA of HAE register

//
// Define the size of the DMA translation table.
//
#define DMA_TRANSLATION_LIMIT 0x1000    // translation table limit

//
// Define pointer to DMA control registers.
//

#define DMA_CONTROL ((volatile PEISA_CONTROL)(DMA_VIRTUAL_BASE))


// Define a pointer to the Eisa INTA register
//

#define INT_CONTROL (volatile PINTACK_REGISTERS)EISA_INTA_CYCLE_VIRTUAL_BASE

//
// Define system time increment value.
//

#define TIME_INCREMENT (10 * 1000 * 10)  // Time increment in 100ns units

//
// EISA I/O access values
//

#define EISA_BIT_SHIFT   0x07		// Bits to shift address
#define EISA_BYTE_OFFSET 0x080		// Offset to next byte
#define EISA_SHORT_OFFSET 0x100		// Offset to next short
#define EISA_LONG_OFFSET 0x200		// Offset to next longword

#define	EISA_BYTE_LEN	 0x00		// Byte length
#define	EISA_WORD_LEN	 0x20		// Word length
#define EISA_TRIBYTE_LEN 0x40		// TriByte length
#define EISA_LONG_LEN	 0x60		// LONGWORD length

#define EISA_FIRST	0x000		// Byte, word and longword offsets
#define EISA_SECOND	0x080		// for EISA access.  To get the
#define EISA_THIRD	0x100		// 2nd word, use (WORD_LEN|SECOND)
#define EISA_FOURTH	0x180


//
// Combo chip I/O access values -the 82C106 is only byte addressable
//

#define COMBO_BIT_SHIFT     0x009               // Bits to shift address
#define COMBO_BYTE_LEN      0x000               // Byte length
#define COMBO_BYTE_OFFSET   0x200               // offset to next longword


//
// The bits which are set if this is a "QVA" - quasi virtual address,
// for getting to I/O space.  Returned by wrapper for MmMapIoSpace,
// used by I/O access routines.
//

#define BUS_QVA (QVA_ENABLE | 0x10000000)

#define EISA_QVA (QVA_ENABLE | 0x10000000)
#define COMBO_QVA  (QVA_ENABLE | 0x00000000)

#define IS_EISA_QVA(x)   (((ULONG)x & BUS_QVA) == EISA_QVA)
#define IS_COMBO_QVA(x) (((ULONG)x & BUS_QVA) == COMBO_QVA)



#endif // _JNSNDEF_

