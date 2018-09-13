/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    duoprom.h

Abstract:

    This module is the header file that describes physical and virtual
    address used by the PROM monitor and boot code.

Author:

    Lluis Abello (lluis) 5-Apr-1993

Revision History:

--*/

#ifndef _DUOPROM_
#define _DUOPROM_

//
// Define virtual/physical base address pairs for boot mapping.
//

#define DEVICE_VIRTUAL_BASE 0xe0000000  // virtual base of device space
#define DEVICE_PHYSICAL_BASE 0x80000000 // physical base of device space

#define VIDEO_CONTROL_VIRTUAL_BASE 0xe0200000 // virtual base of video control
#define VIDEO_CONTROL_PHYSICAL_BASE 0x60000000 // physical base of video control

#define EXTENDED_VIDEO_CONTROL_VIRTUAL_BASE 0xe0400000 // virtual base of extended video control
#define EXTENDED_VIDEO_CONTROL_PHYSICAL_BASE 0x60200000 // physical base of extended video control

#define VIDEO_MEMORY_VIRTUAL_BASE 0xe0800000 // virtual base of video memory
#define VIDEO_MEMORY_PHYSICAL_BASE 0x40000000 // physical base of video memory

#define PROM_VIRTUAL_BASE 0xe1000000    // virtual base of boot PROM
#define PROM_PHYSICAL_BASE 0xfff00000   // physical base of boot PROM

#define EEPROM_VIRTUAL_BASE PROM_VIRTUAL_BASE+0x40000  // virtual base of boot PROM
#define EEPROM_PHYSICAL_BASE 0xfff40000   // physical base of boot PROM

#define EISA_IO_VIRTUAL_BASE 0xe2000000 // virtual base of EISA I/O
#define EISA_EXTERNAL_IO_VIRTUAL_BASE 0xe4000000 // virtual base of EISA I/O
#define EISA_IO_PHYSICAL_BASE 0x90000000 // physical base of EISA I/O

#define EISA_MEMORY_VIRTUAL_BASE 0xe3000000 // virtual base of EISA memory
#define EISA_MEMORY_PHYSICAL_BASE 0x91000000 // physical base of EISA memory

#define PCR_VIRTUAL_BASE KiPcr          // virtual address of PCR
#define PCR_PHYSICAL_BASE 0x7ff000      // physical address of PCR

#undef SP_PHYSICAL_BASE
#define SP_VIRTUAL_BASE 0xffffa000      // virtual base of serial port
#define SP_PHYSICAL_BASE 0x80006000     // physical base of serial port

//
// Define boot code device virtual addresses.
//

#undef DMA_VIRTUAL_BASE
#define DMA_VIRTUAL_BASE 0xe0000000     // virtual base of DMA control

#define NET_VIRTUAL_BASE 0xe0001000     // virtual base of ethernet control

#define SCSI1_VIRTUAL_BASE 0xe0002000    // virtual base os SCSI control

#define SCSI2_VIRTUAL_BASE 0xe0003000  // virtual base of floppy control

#define RTC_VIRTUAL_BASE 0xe0004000     // virtual base of realtime clock

#define KEYBOARD_VIRTUAL_BASE 0xe0005000 // virtual base of keyboard control

#define COMPORT1_VIRTUAL_BASE 0xe0006000 // virtual base of comport 1 control

#define COMPORT2_VIRTUAL_BASE 0xe0007000 // virtual base of comport 2 control

#define PARALLEL_VIRTUAL_BASE 0xe0008000 // virtual base of parallel control

#define NVRAM_VIRTUAL_BASE 0xe0009000    // virtual base of NVRAM

#define FLASH_ENABLE_VIRTUAL_BASE 0xe000d000 // virtual base of FLASH EEPROM control

#define DIAGNOSTIC_VIRTUAL_BASE 0xe000e000 // virtual base of diagnostic control

#define INTERRUPT_VIRTUAL_BASE 0xe000f000  // virtual base of interrupt enable

#define VIDEO_CURSOR_VIRTUAL_BASE 0xe0208000 // virtual base of cursor control

#define VIDEO_ID_VIRTUAL_BASE 0xe0210000 // virtual base of video id register

#define VIDEO_RESET_VIRTUAL_BASE 0xe0220000 // virtual base of reset register

#define EXCLUSIVE_PAGE_VIRTUAL_BASE 0xc0000000 // virtual base of exclusive page

#define SHARED_PAGE_VIRTUAL_BASE 0xc0001000    // virtual base of shared page

#define EXCLUSIVE_PAGE_PHYSICAL_BASE 0x800000  // physical base of exclusive page

#define SHARED_PAGE_PHYSICAL_BASE    0x801000  // physical base of shared page


//
// Define base address and limit of DMA translation table.
//

#define DMA_TRANSLATION_BASE 0xa0001000 // translation table base address
#define DMA_TRANSLATION_LIMIT 0x1000    // translation table limit

//
// Define pointer to DMA control registers.
//

#define DMA_CONTROL ((volatile PDMA_REGISTERS)(DMA_VIRTUAL_BASE))

//
// Define pointer to interrupt source register.
//
#undef INTERRUPT_SOURCE
#define INTERRUPT_SOURCE (&DMA_CONTROL->LocalInterruptAcknowledge.Long)


//
// Define device interrupt identification values.
//

#define PARALLEL_DEVICE 0x4         // Parallel port device interrupt id
#define VIDEO_DEVICE 0xC            // Video device interrupt id
#define ETHERNET_DEVICE 0x10        // Ethernet device interrupt id
#define SCSI1_DEVICE 0x14           // SCSI device interrupt id
#define SCSI2_DEVICE 0x18           // SCSI device interrupt id
#define KEYBOARD_DEVICE 0x1C        // Keyboard device interrupt id
#define MOUSE_DEVICE 0x20           // Mouse device interrupt id
#define SERIAL0_DEVICE 0x24         // Serial port 0 device interrupt id
#define SERIAL1_DEVICE 0x28         // Serial port 1 device interrupt id

//
// Define low memory transfer vector address and TB index address.
//

#define TRANSFER_VECTOR (KSEG1_BASE + 0x400) // exception handler address

//
// Define TLB index to map Flash prom.
//
#define FLASH_PROM_TLB_INDEX 1

#endif // _DUOPROM_
