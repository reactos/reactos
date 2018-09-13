/*++ BUILD Version: 0005    // Increment this if a change has global effects

Copyright (c) 1990  Microsoft Corporation

Module Name:

    jazzdef.h

Abstract:

    This module is the header file that describes hardware addresses
    for the Jazz system.

Author:

    David N. Cutler (davec) 26-Nov-1990

Revision History:

--*/

#ifndef _JAZZDEF_
#define _JAZZDEF_

//
// Define physical base addresses for system mapping.
//

#define VIDEO_MEMORY_PHYSICAL_BASE 0x40000000 // physical base of video memory
#define VIDEO_CONTROL_PHYSICAL_BASE 0x60000000 // physical base of video control
#define CURSOR_CONTROL_PHYSICAL_BASE 0x60008000 // physical base of cursor control
#define VIDEO_ID_PHYSICAL_BASE 0x60010000 // physical base of video id register
#define VIDEO_RESET_PHYSICAL_BASE 0x60020000 // physical base of reset register
#define DEVICE_PHYSICAL_BASE 0x80000000 // physical base of device space
#define NET_PHYSICAL_BASE 0x80001000    // physical base of ethernet control
#define SCSI_PHYSICAL_BASE 0x80002000   // physical base os SCSI control
#define FLOPPY_PHYSICAL_BASE 0x80003000 // physical base of floppy control
#define RTCLOCK_PHYSICAL_BASE 0x80004000 // physical base of realtime clock
#define KEYBOARD_PHYSICAL_BASE 0x80005000  // physical base of keyboard control
#define MOUSE_PHYSICAL_BASE 0x80005000  // physical base of mouse control
#define SERIAL0_PHYSICAL_BASE 0x80006000 // physical base of serial port 0
#define SERIAL1_PHYSICAL_BASE 0x80007000 // physical base of serial port 1
#define PARALLEL_PHYSICAL_BASE 0x80008000 // physical base of parallel port
#define SOUND_PHYSICAL_BASE 0x8000C000  // physical base of sound control
#define EISA_CONTROL_PHYSICAL_BASE 0x90000000 // physical base of EISA control
#define EISA_MEMORY_PHYSICAL_BASE 0x91000000 // physical base of EISA memory
#define EISA_MEMORY_VERSION2_LOW 0x00000000  // physical base of EISA memory
#define EISA_MEMORY_VERSION2_HIGH 0x00000001  // with version 2 address chip
#define PROM_PHYSICAL_BASE 0xfff00000   // physical base of boot PROM

//
// Define virtual/physical base addresses for system mapping.
//

#define NVRAM_VIRTUAL_BASE 0xffff8000   // virtual base of nonvolatile RAM
#define NVRAM_PHYSICAL_BASE 0x80009000  // physical base of nonvolatile RAM

#define SP_VIRTUAL_BASE 0xffffa000      // virtual base of serial port 0
#define SP_PHYSICAL_BASE SERIAL0_PHYSICAL_BASE // physical base of serial port 0

#define DMA_VIRTUAL_BASE 0xffffc000     // virtual base of DMA control
#define DMA_PHYSICAL_BASE DEVICE_PHYSICAL_BASE // physical base of DMA control

#define INTERRUPT_VIRTUAL_BASE 0xffffd000 // virtual base of interrupt source
#define INTERRUPT_PHYSICAL_BASE 0xf0000000 // physical base of interrupt source

//
// Define the size of the DMA translation table.
//

#define DMA_TRANSLATION_LIMIT 0x2000    // translation table limit

//
// Define the maximum number of map registers allowed per allocation.
//

#define DMA_REQUEST_LIMIT (DMA_TRANSLATION_LIMIT/(sizeof(TRANSLATION_ENTRY) * 8))

//
// Define pointer to DMA control registers.
//

#define DMA_CONTROL ((volatile PDMA_REGISTERS)(DMA_VIRTUAL_BASE))

//
// Define DMA device channels.
//

#define SCSI_CHANNEL 0x0                // SCSI DMA channel number
#define FLOPPY_CHANNEL 0x1              // Floppy DMA channel
#define SOUND_CHANNEL_A 0x2             // Sound DMA channel A
#define SOUND_CHANNEL_B 0x3             // Sound DMA channel B

//
// Define DMA channel interrupt level.
//

#define DMA_LEVEL       3

//
// Define EISA NMI interrupt level.
//

#define EISA_NMI_LEVEL  6

//
// Define the minimum and maximum system time increment value in 100ns units.
//

#define MAXIMUM_INCREMENT (10 * 1000 * 10)
#define MINIMUM_INCREMENT (1 * 1000 * 10)

//
// Define Jazz clock levels.
//

#define CLOCK_LEVEL 7                   // Interval clock level
#define CLOCK_INTERVAL ((MAXIMUM_INCREMENT / (10 * 1000)) - 1) // Ms minus 1

#if defined(R3000)

#define EISA_DEVICE_LEVEL  8            // EISA bus interrupt level

#endif

#if defined(R4000)

#define EISA_DEVICE_LEVEL  5            // EISA bus interrupt level

#endif

#define CLOCK2_LEVEL CLOCK_LEVEL        //

//
// Define EISA device interrupt vectors.
//

#define EISA_VECTORS 32

#define IRQL10_VECTOR (10 + EISA_VECTORS) // Eisa interrupt request level 10
#define IRQL11_VECTOR (11 + EISA_VECTORS) // Eisa interrupt request level 11
#define IRQL12_VECTOR (12 + EISA_VECTORS) // Eisa interrupt request level 12
#define IRQL13_VECTOR (13 + EISA_VECTORS) // Eisa interrupt request level 13

#define MAXIMUM_EISA_VECTOR (15 + EISA_VECTORS) // maximum EISA vector

//
// Define I/O device interrupt level.
//

#define DEVICE_LEVEL 4                  // I/O device interrupt level

//
// Define device interrupt vectors.
//

#define DEVICE_VECTORS 16               // starting builtin device vector

#define PARALLEL_VECTOR (1 + DEVICE_VECTORS) // Parallel device interrupt vector
#define FLOPPY_VECTOR (2 + DEVICE_VECTORS) // Floppy device interrupt vector
#define SOUND_VECTOR (3 + DEVICE_VECTORS) // Sound device interrupt vector
#define VIDEO_VECTOR (4 + DEVICE_VECTORS) // video device interrupt vector
#define NET_VECTOR (5 + DEVICE_VECTORS)  // ethernet device interrupt vector
#define SCSI_VECTOR (6 + DEVICE_VECTORS) // SCSI device interrupt vector
#define KEYBOARD_VECTOR (7 + DEVICE_VECTORS) // Keyboard device interrupt vector
#define MOUSE_VECTOR (8 + DEVICE_VECTORS) // Mouse device interrupt vector
#define SERIAL0_VECTOR (9 + DEVICE_VECTORS) // Serial device 0 interrupt vector
#define SERIAL1_VECTOR (10 + DEVICE_VECTORS) // Serial device 1 interrupt vector

#define MAXIMUM_BUILTIN_VECTOR SERIAL1_VECTOR // maximum builtin vector

//
// Define the clock speed in megahetz for the SCSI protocol chips.
//

#define NCR_SCSI_CLOCK_SPEED        24
#define EMULEX_SCSI_CLOCK_SPEED     40

//
// PROM entry point definitions.
//
// Define base address of prom entry vector and prom entry macro.
//

#define PROM_BASE (KSEG1_BASE | 0x1fc00000)
#define PROM_ENTRY(x) (PROM_BASE + ((x) * 8))


#endif // _JAZZDEF_
